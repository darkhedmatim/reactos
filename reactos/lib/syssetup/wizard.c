/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: wizard.c,v 1.5.2.1 2004/08/31 14:53:36 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           System setup
 * FILE:              lib/syssetup/wizard.c
 * PROGRAMER:         Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <commctrl.h>

#include <string.h>
#include <tchar.h>

#include <syssetup.h>

#include "globals.h"
#include "resource.h"


/* GLOBALS ******************************************************************/

static SETUPDATA SetupData;


/* FUNCTIONS ****************************************************************/

static VOID
CenterWindow(HWND hWnd)
{
  HWND hWndParent;
  RECT rcParent;
  RECT rcWindow;

  hWndParent = GetParent(hWnd);
  if (hWndParent == NULL)
    hWndParent = GetDesktopWindow();

  GetWindowRect(hWndParent, &rcParent);
  GetWindowRect(hWnd, &rcWindow);

  SetWindowPos(hWnd,
	       HWND_TOP,
	       ((rcParent.right - rcParent.left) - (rcWindow.right - rcWindow.left)) / 2,
	       ((rcParent.bottom - rcParent.top) - (rcWindow.bottom - rcWindow.top)) / 2,
	       0,
	       0,
	       SWP_NOSIZE);
}


BOOL CALLBACK
WelcomeDlgProc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
  switch (uMsg)
    {
      case WM_INITDIALOG:
        {
          HWND hwndControl;
          DWORD dwStyle;

          hwndControl = GetParent(hwndDlg);

          /* Center the wizard window */
          CenterWindow (hwndControl);

          /* Hide the system menu */
          dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
          SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);

          /* Hide and disable the 'Cancel' button */
          hwndControl = GetDlgItem(GetParent(hwndDlg), IDCANCEL);
          ShowWindow (hwndControl, SW_HIDE);
          EnableWindow (hwndControl, FALSE);
        }
        break;


      case WM_NOTIFY:
        {
          LPNMHDR lpnm = (LPNMHDR)lParam;

          switch (lpnm->code)
            {
              case PSN_SETACTIVE:
                /* Enable the Next button */
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
                break;

              default:
                break;
            }
        }
        break;

      default:
        break;
    }

  return FALSE;
}


BOOL CALLBACK
OwnerPageDlgProc(HWND hwndDlg,
                 UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam)
{
  TCHAR OwnerName[51];
  TCHAR OwnerOrganization[51];
  HKEY hKey;
  LPNMHDR lpnm;

  switch (uMsg)
    {
      case WM_INITDIALOG:
        {
          SendDlgItemMessage(hwndDlg, IDC_OWNERNAME, EM_LIMITTEXT, 50, 0);
          SendDlgItemMessage(hwndDlg, IDC_OWNERORGANIZATION, EM_LIMITTEXT, 50, 0);

          /* Set focus to owner name */
          SetFocus(GetDlgItem(hwndDlg, IDC_OWNERNAME));
        }
        break;


      case WM_NOTIFY:
        {
          lpnm = (LPNMHDR)lParam;

          switch (lpnm->code)
            {
              case PSN_SETACTIVE:
                /* Enable the Back and Next buttons */
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                break;

              case PSN_WIZNEXT:
                OwnerName[0] = 0;
                if (GetDlgItemText(hwndDlg, IDC_OWNERNAME, OwnerName, 50) == 0)
                {
                  MessageBox(hwndDlg,
                             _T("Setup cannot continue until you enter your name."),
                             _T("ReactOS Setup"),
                             MB_ICONERROR | MB_OK);
                  SetWindowLong(hwndDlg, DWL_MSGRESULT, -1);
                  return TRUE;
                }

                OwnerOrganization[0] = 0;
                GetDlgItemText(hwndDlg, IDC_OWNERORGANIZATION, OwnerOrganization, 50);

                RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             _T("Software\\Microsoft\\Windows NT\\CurrentVersion"),
                             0,
                             KEY_ALL_ACCESS,
                             &hKey);
                /* FIXME: check error code */

                RegSetValueEx(hKey,
                              _T("RegisteredOwner"),
                              0,
                              REG_SZ,
                              OwnerName,
                              (_tcslen(OwnerName) + 1) * sizeof(TCHAR));
                /* FIXME: check error code */

                RegSetValueEx(hKey,
                              _T("RegisteredOrganization"),
                              0,
                              REG_SZ,
                              OwnerOrganization,
                              (_tcslen(OwnerOrganization) + 1) * sizeof(TCHAR));
                /* FIXME: check error code */

                RegCloseKey(hKey);
                break;

              default:
                break;
            }
        }
        break;

      default:
        break;
    }

  return FALSE;
}


BOOL CALLBACK
ComputerPageDlgProc(HWND hwndDlg,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam)
{
  TCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
  TCHAR Password1[15];
  TCHAR Password2[15];
  DWORD Length;
  LPNMHDR lpnm;

  switch (uMsg)
    {
      case WM_INITDIALOG:
        {
          /* Retrieve current computer name */
          Length = MAX_COMPUTERNAME_LENGTH + 1;
          GetComputerName(ComputerName, &Length);

          /* Display current computer name */
          SetDlgItemText(hwndDlg, IDC_COMPUTERNAME, ComputerName);

          /* Set text limits */
          SendDlgItemMessage(hwndDlg, IDC_COMPUTERNAME, EM_LIMITTEXT, 64, 0);
          SendDlgItemMessage(hwndDlg, IDC_ADMINPASSWORD1, EM_LIMITTEXT, 14, 0);
          SendDlgItemMessage(hwndDlg, IDC_ADMINPASSWORD2, EM_LIMITTEXT, 14, 0);

          /* Set focus to computer name */
          SetFocus(GetDlgItem(hwndDlg, IDC_COMPUTERNAME));
        }
        break;


      case WM_NOTIFY:
        {
          lpnm = (LPNMHDR)lParam;

          switch (lpnm->code)
            {
              case PSN_SETACTIVE:
                /* Enable the Back and Next buttons */
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                break;

              case PSN_WIZNEXT:
                if (GetDlgItemText(hwndDlg, IDC_COMPUTERNAME, ComputerName, 64) == 0)
                {
                  MessageBox(hwndDlg,
                             _T("Setup cannot continue until you enter the name of your computer."),
                             _T("ReactOS Setup"),
                             MB_ICONERROR | MB_OK);
                  SetWindowLong(hwndDlg, DWL_MSGRESULT, -1);
                  return TRUE;
                }

                /* FIXME: check computer name for invalid characters */

                if (!SetComputerName(ComputerName))
                {
                  MessageBox(hwndDlg,
                             _T("Setup failed to set the computer name."),
                             _T("ReactOS Setup"),
                             MB_ICONERROR | MB_OK);
                  SetWindowLong(hwndDlg, DWL_MSGRESULT, -1);
                  return TRUE;
                }

                /* Check admin passwords */
                GetDlgItemText(hwndDlg, IDC_ADMINPASSWORD1, Password1, 15);
                GetDlgItemText(hwndDlg, IDC_ADMINPASSWORD2, Password2, 15);
                if (_tcscmp(Password1, Password2))
                {
                  MessageBox(hwndDlg,
                             _T("The passwords you entered do not match. Please enter "\
                                "the desired password again."),
                             _T("ReactOS Setup"),
                             MB_ICONERROR | MB_OK);
                  SetWindowLong(hwndDlg, DWL_MSGRESULT, -1);
                  return TRUE;
                }

                /* FIXME: check password for invalid characters */

                /* FIXME: Set admin password */
                break;

              default:
                break;
            }
        }
        break;

      default:
        break;
    }

  return FALSE;
}


BOOL CALLBACK
LocalePageDlgProc(HWND hwndDlg,
                  UINT uMsg,
                  WPARAM wParam,
                  LPARAM lParam)
{
  PSETUPDATA SetupData;

  /* Retrieve pointer to the global setup data */
  SetupData = (PSETUPDATA)GetWindowLong (hwndDlg, GWL_USERDATA);

  switch (uMsg)
    {
      case WM_INITDIALOG:
        {
          /* Save pointer to the global setup data */
          SetupData = (PSETUPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
          SetWindowLong(hwndDlg, GWL_USERDATA, (LONG)SetupData);

        }
        break;


      case WM_NOTIFY:
        {
          LPNMHDR lpnm = (LPNMHDR)lParam;

          switch (lpnm->code)
            {
              case PSN_SETACTIVE:
                /* Enable the Back and Next buttons */
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                break;

              case PSN_WIZNEXT:
                break;

              default:
                break;
            }
        }
        break;

      default:
        break;
    }

  return FALSE;
}


BOOL CALLBACK
FinishDlgProc(HWND hwndDlg,
              UINT uMsg,
              WPARAM wParam,
              LPARAM lParam)
{

  switch (uMsg)
    {
      case WM_INITDIALOG:
        break;

      case WM_NOTIFY:
        {
          LPNMHDR lpnm = (LPNMHDR)lParam;

          switch (lpnm->code)
            {
              case PSN_SETACTIVE:
                /* Enable the correct buttons on for the active page */
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_FINISH);
                break;

              case PSN_WIZBACK:
                /* Handle a Back button click, if necessary */
                break;

              case PSN_WIZFINISH:
                /* Handle a Finish button click, if necessary */
                break;

              default:
                break;
            }
        }
        break;

      default:
        break;
    }

  return FALSE;
}


VOID
InstallWizard(VOID)
{
  PROPSHEETHEADER psh;
  HPROPSHEETPAGE ahpsp[5];
  PROPSHEETPAGE psp;
//  SHAREDWIZDATA wizdata;

  /* Clear setup data */
  ZeroMemory(&SetupData, sizeof(SETUPDATA));

  /* Create the Welcome page */
  ZeroMemory (&psp, sizeof(PROPSHEETPAGE));
  psp.dwSize = sizeof(PROPSHEETPAGE);
  psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
  psp.hInstance = hDllInstance;
  psp.lParam = (LPARAM)&SetupData;
  psp.pfnDlgProc = WelcomeDlgProc;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_WELCOMEPAGE);
  ahpsp[0] = CreatePropertySheetPage(&psp);

  /* Create the Owner page */
  psp.dwFlags = PSP_DEFAULT; // | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
//  psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TITLE1);
//  psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_SUBTITLE1);
  psp.pszTemplate = MAKEINTRESOURCE(IDD_OWNERPAGE);
  psp.pfnDlgProc = OwnerPageDlgProc;
  ahpsp[1] = CreatePropertySheetPage(&psp);

  /* Create the Computer page */
  psp.dwFlags = PSP_DEFAULT; // | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
//  psp.pszHeaderTitle =    MAKEINTRESOURCE(IDS_TITLE2);
//  psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_SUBTITLE2);
  psp.pfnDlgProc = ComputerPageDlgProc;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_COMPUTERPAGE);
  ahpsp[2] = CreatePropertySheetPage(&psp);


  /* Create the Finish page */
  psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
  psp.pfnDlgProc = FinishDlgProc;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_FINISHPAGE);
  ahpsp[3] = CreatePropertySheetPage(&psp);

  /* Create the property sheet */
  psh.dwSize = sizeof(PROPSHEETHEADER);
  psh.dwFlags = PSH_WIZARD; //97 | PSH_WATERMARK | PSH_HEADER;
  psh.hInstance = hDllInstance;
  psh.hwndParent = NULL;
  psh.nPages = 4;
  psh.nStartPage = 0;
  psh.phpage = ahpsp;
//  psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);
//  psh.pszbmHeader = MAKEINTRESOURCE(IDB_HEADER);

  /* Display the wizard */
  PropertySheet(&psh);
}

/* EOF */

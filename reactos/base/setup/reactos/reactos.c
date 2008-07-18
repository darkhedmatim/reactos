/*
 *  ReactOS applications
 *  Copyright (C) 2004-2008 ReactOS Team
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
/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS GUI first stage setup application
 * FILE:        subsys/system/reactos/reactos.c
 * PROGRAMMERS: Eric Kohl
 *              Matthias Kupfer
 */

#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <setupapi.h>

#include "resource.h"

/* GLOBALS ******************************************************************/

HFONT hTitleFont;

struct
{
	LONG DestDiskNumber; // physical disk
	LONG DestPartNumber; // partition on disk
	LONG DestPartSize; // if partition doesn't exist, size of partition
	LONG FSType; // file system type on partition 
	LONG MBRInstallType; // install bootloader
	LONG FormatPart; // type of format the partition
	WCHAR SelectedLangId[4]; // selected language
	WCHAR InstallationDirectory[MAX_PATH]; // installation directory on hdd
	WCHAR DefaultLang[20]; // default language
	WCHAR DefaultKBLayout[20]; // default keyboard layout
	BOOLEAN RepairUpdateFlag; // flag for update/repair an installed reactos
} SetupData;

TCHAR abort_msg[512],abort_title[64];
BOOL isUnattend;

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

static HFONT
CreateTitleFont(VOID)
{
  NONCLIENTMETRICS ncm;
  LOGFONT LogFont;
  HDC hdc;
  INT FontSize;
  HFONT hFont;

  ncm.cbSize = sizeof(NONCLIENTMETRICS);
  SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

  LogFont = ncm.lfMessageFont;
  LogFont.lfWeight = FW_BOLD;
  _tcscpy(LogFont.lfFaceName, _T("MS Shell Dlg"));

  hdc = GetDC(NULL);
  FontSize = 12;
  LogFont.lfHeight = 0 - GetDeviceCaps (hdc, LOGPIXELSY) * FontSize / 72;
  hFont = CreateFontIndirect(&LogFont);
  ReleaseDC(NULL, hdc);

  return hFont;
}

static INT_PTR CALLBACK
StartDlgProc(HWND hwndDlg,
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

          	dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
	        SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);
		
		/* Hide and disable the 'Cancel' button at the moment,
		 * we use this button to cancel the setup process
		 * like F3 in usetup
		 */
		hwndControl = GetDlgItem(GetParent(hwndDlg), IDCANCEL);
		ShowWindow (hwndControl, SW_HIDE);
		EnableWindow (hwndControl, FALSE);
	        
		/* Set title font */
		SendDlgItemMessage(hwndDlg,
                             IDC_STARTTITLE,
                             WM_SETFONT,
                             (WPARAM)hTitleFont,
                             (LPARAM)TRUE);
}
	  break;
	  case WM_NOTIFY:
	  {
          LPNMHDR lpnm = (LPNMHDR)lParam;

		  switch (lpnm->code)
		  {		
		      case PSN_SETACTIVE: // Only "Finish" for closing the App
			PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_FINISH);
			//PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
			break;
		      default:
			break;
		  }
	  break;
	  default:
	  	break;
	  }

  }
  return FALSE;
}

static INT_PTR CALLBACK
LangSelDlgProc(HWND hwndDlg,
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

          	dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
	        SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);
	        
		hwndControl = GetDlgItem(GetParent(hwndDlg), IDCANCEL);
		ShowWindow (hwndControl, SW_SHOW);
		EnableWindow (hwndControl, TRUE);

		/* Set title font */
		/*SendDlgItemMessage(hwndDlg,
                             IDC_STARTTITLE,
                             WM_SETFONT,
                             (WPARAM)hTitleFont,
                             (LPARAM)TRUE);*/
}
	  break;
	  case WM_NOTIFY:
	  {
          LPNMHDR lpnm = (LPNMHDR)lParam;

		  switch (lpnm->code)
		  {		
		      case PSN_SETACTIVE:
			PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
			break;
		      case PSN_QUERYCANCEL:
	       		SetWindowLong(hwndDlg, DWL_MSGRESULT,MessageBox(GetParent(hwndDlg), abort_msg, abort_title, MB_YESNO | MB_ICONQUESTION) != IDYES);
			return TRUE;
		      default:
			break;
		  }
	  break;
	  default:
	  	break;
	  }

  }
  return FALSE;
}

static INT_PTR CALLBACK
TypeDlgProc(HWND hwndDlg,
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

          	dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
	        SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);
		
		CheckDlgButton(hwndDlg, IDC_INSTALL, BST_CHECKED);
	        
		/* Set title font */
		/*SendDlgItemMessage(hwndDlg,
                             IDC_STARTTITLE,
                             WM_SETFONT,
                             (WPARAM)hTitleFont,
                             (LPARAM)TRUE);*/
}
	  break;
	  case WM_NOTIFY:
	  {
          LPNMHDR lpnm = (LPNMHDR)lParam;

		  switch (lpnm->code)
		  {		
		      case PSN_SETACTIVE:
			PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
			break;
		      case PSN_QUERYCANCEL:
	       		SetWindowLong(hwndDlg, DWL_MSGRESULT,MessageBox(GetParent(hwndDlg), abort_msg, abort_title, MB_YESNO | MB_ICONQUESTION) != IDYES);
			return TRUE;
		      default:
			break;
		  }
	  break;
	  default:
	  	break;
	  }

  }
  return FALSE;
}

static INT_PTR CALLBACK
DeviceDlgProc(HWND hwndDlg,
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

          	dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
	        SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);
		
		/* Set title font */
		/*SendDlgItemMessage(hwndDlg,
                             IDC_STARTTITLE,
                             WM_SETFONT,
                             (WPARAM)hTitleFont,
                             (LPARAM)TRUE);*/
}
	  break;
	  case WM_NOTIFY:
	  {
          LPNMHDR lpnm = (LPNMHDR)lParam;

		  switch (lpnm->code)
		  {		
		      case PSN_SETACTIVE:
			PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
			break;
		      case PSN_QUERYCANCEL:
	       		SetWindowLong(hwndDlg, DWL_MSGRESULT,MessageBox(GetParent(hwndDlg), abort_msg, abort_title, MB_YESNO | MB_ICONQUESTION) != IDYES);
			return TRUE;
		      default:
			break;
		  }
	  break;
	  default:
	  	break;
	  }

  }
  return FALSE;
}

static INT_PTR CALLBACK
DriveDlgProc(HWND hwndDlg,
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

          	dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
	        SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);
		
		CheckDlgButton(hwndDlg, IDC_INSTFREELDR, BST_CHECKED);
		/* Set title font */
		/*SendDlgItemMessage(hwndDlg,
                             IDC_STARTTITLE,
                             WM_SETFONT,
                             (WPARAM)hTitleFont,
                             (LPARAM)TRUE);*/
}
	  break;
	  case WM_NOTIFY:
	  {
          LPNMHDR lpnm = (LPNMHDR)lParam;

		  switch (lpnm->code)
		  {		
		      case PSN_SETACTIVE:
			PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
			break;
		      case PSN_QUERYCANCEL:
	       		SetWindowLong(hwndDlg, DWL_MSGRESULT,MessageBox(GetParent(hwndDlg), abort_msg, abort_title, MB_YESNO | MB_ICONQUESTION) != IDYES);
			return TRUE;
		      default:
			break;
		  }
	  break;
	  default:
	  	break;
	  }

  }
  return FALSE;
}

static INT_PTR CALLBACK
ProcessDlgProc(HWND hwndDlg,
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

          	dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
	        SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);
		
		hwndControl = GetDlgItem(GetParent(hwndDlg), IDCANCEL);
		ShowWindow (hwndControl, SW_HIDE);
		EnableWindow (hwndControl, FALSE);

		/* Set title font */
		/*SendDlgItemMessage(hwndDlg,
                             IDC_STARTTITLE,
                             WM_SETFONT,
                             (WPARAM)hTitleFont,
                             (LPARAM)TRUE);*/
}
	  break;
	  case WM_NOTIFY:
	  {
          LPNMHDR lpnm = (LPNMHDR)lParam;

		  switch (lpnm->code)
		  {		
		      case PSN_SETACTIVE: 
			PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT );
			break;
		      default:
			break;
		  }
	  break;
	  default:
	  	break;
	  }

  }
  return FALSE;
}

static INT_PTR CALLBACK
RestartDlgProc(HWND hwndDlg,
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

          	dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
	        SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);
		
		/* Set title font */
		/*SendDlgItemMessage(hwndDlg,
                             IDC_STARTTITLE,
                             WM_SETFONT,
                             (WPARAM)hTitleFont,
                             (LPARAM)TRUE);*/
	  }
	  break;
	  case WM_TIMER:
	  {
           INT Position;
           HWND hWndProgress;

           hWndProgress = GetDlgItem(hwndDlg, IDC_RESTART_PROGRESS);
           Position = SendMessage(hWndProgress, PBM_GETPOS, 0, 0);
           if (Position == 300)
           {
             KillTimer(hwndDlg, 1);
             PropSheet_PressButton(GetParent(hwndDlg), PSBTN_FINISH);
           }
           else
           {
             SendMessage(hWndProgress, PBM_SETPOS, Position + 1, 0);
           }
          return TRUE;
	  }
	  case WM_DESTROY:
	  	return TRUE;
	  case WM_NOTIFY:
	  {
          LPNMHDR lpnm = (LPNMHDR)lParam;

		  switch (lpnm->code)
		  {		
		      case PSN_SETACTIVE: // Only "Finish" for closing the App
			PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_FINISH);
	                SendDlgItemMessage(hwndDlg, IDC_RESTART_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, 300));
        	        SendDlgItemMessage(hwndDlg, IDC_RESTART_PROGRESS, PBM_SETPOS, 0, 0);
	                SetTimer(hwndDlg, 1, 50, NULL);
			break;
		      default:
			break;
		  }
	  break;
	  default:
	  	break;
	  }

  }
  return FALSE;
}

BOOL isUnattendSetup()
{
	WCHAR szPath[MAX_PATH];
	HINF hUnattendedInf;
	INFCONTEXT InfContext;
	TCHAR szValue[MAX_PATH];
	DWORD LineLength;
	//HKEY hKey;
	BOOL result = 0;

	GetCurrentDirectoryW(MAX_PATH, szPath); // FIXME

	wcscat(szPath, L"\\unattend.inf");
	hUnattendedInf = SetupOpenInfFileW(szPath, NULL, INF_STYLE_OLDNT, NULL);
	if (hUnattendedInf != INVALID_HANDLE_VALUE)
	{
		if (SetupFindFirstLine(hUnattendedInf, _T("Unattend"),
			_T("UnattendSetupEnabled"),&InfContext))
		{
			if (SetupGetStringField(&InfContext, 1, szValue,
			sizeof(szValue) / sizeof(TCHAR), &LineLength) &&
			(_tcsicmp(szValue, _T("yes"))==0))
			{
				result = 1; // unattendSetup enabled
				// read values and store in SetupData
			}
		}
    		SetupCloseInfFile(hUnattendedInf);
  	}
	return result;
}

int WINAPI
WinMain(HINSTANCE hInst,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine,
	int nCmdShow)
{
  PROPSHEETHEADER psh;
  HPROPSHEETPAGE ahpsp[7];
  PROPSHEETPAGE psp = {0};
  UINT nPages = 0;
  isUnattend = isUnattendSetup();

  if (!isUnattend)
  {

  LoadString(hInst,IDS_ABORTSETUP, abort_msg, sizeof(abort_msg)/sizeof(TCHAR));
  LoadString(hInst,IDS_ABORTSETUP2, abort_title,sizeof(abort_title)/sizeof(TCHAR));

  /* Create the Start page, until setup is working */
  psp.dwSize = sizeof(PROPSHEETPAGE);
  psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
  psp.hInstance = hInst;
  psp.lParam = 0;
  psp.pfnDlgProc = StartDlgProc;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_STARTPAGE);
  ahpsp[nPages++] = CreatePropertySheetPage(&psp);

  /* Create language selection page */
  psp.dwSize = sizeof(PROPSHEETPAGE);
  psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
  psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_LANGTITLE);
  psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_LANGSUBTITLE);
  psp.hInstance = hInst;
  psp.lParam = 0;
  psp.pfnDlgProc = LangSelDlgProc;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_LANGSELPAGE);
  ahpsp[nPages++] = CreatePropertySheetPage(&psp);
  // Change language with "SetThreadLocale(langid)"

  /* Create install type selection page */
  psp.dwSize = sizeof(PROPSHEETPAGE);
  psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
  psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TYPETITLE);
  psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_TYPESUBTITLE);
  psp.hInstance = hInst;
  psp.lParam = 0;
  psp.pfnDlgProc = TypeDlgProc;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_TYPEPAGE);
  ahpsp[nPages++] = CreatePropertySheetPage(&psp);

  /* Create device settings page */
  psp.dwSize = sizeof(PROPSHEETPAGE);
  psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
  psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_DEVICETITLE);
  psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_DEVICESUBTITLE);
  psp.hInstance = hInst;
  psp.lParam = 0;
  psp.pfnDlgProc = DeviceDlgProc;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_DEVICEPAGE);
  ahpsp[nPages++] = CreatePropertySheetPage(&psp);

  /* Create install device settings page / boot method / install directory*/
  psp.dwSize = sizeof(PROPSHEETPAGE);
  psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
  psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_DRIVETITLE);
  psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_DRIVESUBTITLE);
  psp.hInstance = hInst;
  psp.lParam = 0;
  psp.pfnDlgProc = DriveDlgProc;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_DRIVEPAGE);
  ahpsp[nPages++] = CreatePropertySheetPage(&psp);
  
  }

  /* Create installation progress page */
  psp.dwSize = sizeof(PROPSHEETPAGE);
  psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
  psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_PROCESSTITLE);
  psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_PROCESSSUBTITLE);
  psp.hInstance = hInst;
  psp.lParam = 0;
  psp.pfnDlgProc = ProcessDlgProc;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_PROCESSPAGE);
  ahpsp[nPages++] = CreatePropertySheetPage(&psp);

  if (!isUnattend)
  {
  /* Create finish to reboot page */
  psp.dwSize = sizeof(PROPSHEETPAGE);
  psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
  psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_RESTARTTITLE);
  psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_RESTARTSUBTITLE);
  psp.hInstance = hInst;
  psp.lParam = 0;
  psp.pfnDlgProc = RestartDlgProc;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_RESTARTPAGE);
  ahpsp[nPages++] = CreatePropertySheetPage(&psp);
  }

  /* Create the property sheet */
  psh.dwSize = sizeof(PROPSHEETHEADER);
  psh.dwFlags = PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
  psh.hInstance = hInst;
  psh.hwndParent = NULL;
  psh.nPages = nPages;
  psh.nStartPage = 0;
  psh.phpage = ahpsp;
  psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);
  psh.pszbmHeader = MAKEINTRESOURCE(IDB_HEADER);

  /* Create title font */
  hTitleFont = CreateTitleFont();

  /* Display the wizard */
  PropertySheet(&psh);

  DeleteObject(hTitleFont);

  return 0;

}

/* EOF */

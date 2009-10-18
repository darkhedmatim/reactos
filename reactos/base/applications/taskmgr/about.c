/*
 *  ReactOS Task Manager
 *
 *  about.cpp
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <precomp.h>

INT_PTR CALLBACK AboutDialogWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void OnAbout(void)
{
    DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_ABOUTBOX), hMainWnd, AboutDialogWndProc);
}

INT_PTR CALLBACK
AboutDialogWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND   hLicenseEditWnd;
    WCHAR  strLicense[0x1000];

    switch (message)
    {
    case WM_INITDIALOG:

        hLicenseEditWnd = GetDlgItem(hDlg, IDC_LICENSE_EDIT);

        LoadStringW(hInst, IDS_LICENSE, strLicense, 0x1000);

        SetWindowTextW(hLicenseEditWnd, strLicense);

        return TRUE;

    case WM_COMMAND:

        if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
        {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }

        break;
    }

    return 0;
}

/* $Id$
 *
 * PROJECT:         ReactOS Console Configuration DLL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/console/colors.c
 * PURPOSE:         displays colors dialog
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@student.tugraz.at)
 */

#include "console.h"

static COLORREF s_Colors[] =
{
	RGB(0, 0, 0),
	RGB(0, 0, 128),
	RGB(0, 128, 0),
	RGB(0, 128, 128),
	RGB(128, 0, 0),
	RGB(128, 0, 128),
	RGB(128, 128, 0),
	RGB(192, 192, 192),
	RGB(128, 128, 128),
	RGB(0, 0, 255),
	RGB(0, 255, 0),
	RGB(0, 255, 255),
	RGB(255, 0, 0),
	RGB(255, 0, 255),
	RGB(255, 255, 0),
	RGB(255, 255, 255)
};



static
BOOL
PaintStaticControls(HWND hwndDlg, PConsoleInfo pConInfo, LPDRAWITEMSTRUCT drawItem)
{
	HBRUSH hBrush;
	DWORD index;

	index = drawItem->CtlID - IDC_STATIC_COLOR1;
	hBrush = CreateSolidBrush(s_Colors[index]);
	if (!hBrush)
	{
		return FALSE;
	}

	FillRect(drawItem->hDC, &drawItem->rcItem, hBrush);
    DeleteObject((HGDIOBJ)hBrush);
	if (pConInfo->ActiveStaticControl == index)
	{
		DrawFocusRect(drawItem->hDC, &drawItem->rcItem);
	}
	return TRUE;
}

INT_PTR 
CALLBACK
ColorsProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
	PConsoleInfo pConInfo;
	LPNMUPDOWN lpnmud;
    LPPSHNOTIFY lppsn;
	LPDRAWITEMSTRUCT drawItem;
	DWORD red = -1;
	DWORD green = -1;
	DWORD blue = -1;

	pConInfo = (PConsoleInfo) GetWindowLongPtr(hwndDlg, DWLP_USER);

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			pConInfo = (PConsoleInfo) ((LPPROPSHEETPAGE)lParam)->lParam;
			SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pConInfo);
			SendMessage(GetDlgItem(hwndDlg, IDC_RADIO_SCREEN_BACKGROUND), BM_SETCHECK, BST_CHECKED, 0);
			SendMessage(GetDlgItem(hwndDlg, IDC_UPDOWN_COLOR_RED), UDM_SETRANGE, 0, (LPARAM)MAKELONG(255, 0));
			SendMessage(GetDlgItem(hwndDlg, IDC_UPDOWN_COLOR_GREEN), UDM_SETRANGE, 0, (LPARAM)MAKELONG(255, 0));
			SendMessage(GetDlgItem(hwndDlg, IDC_UPDOWN_COLOR_BLUE), UDM_SETRANGE, 0, (LPARAM)MAKELONG(255, 0));
			InvalidateRect(hwndDlg, NULL, TRUE);
			return TRUE;
		}
		case WM_DRAWITEM:
		{
			drawItem = (LPDRAWITEMSTRUCT)lParam;
			if (drawItem->CtlID >= IDC_STATIC_COLOR1 && drawItem->CtlID <= IDC_STATIC_COLOR16)
			{
				return PaintStaticControls(hwndDlg, pConInfo, drawItem);
			}
			else if (drawItem->CtlID == IDC_STATIC_SCREEN_COLOR || drawItem->CtlID == IDC_STATIC_POPUP_COLOR)
			{
				PaintText(drawItem, pConInfo);
				return TRUE;
			}
		}
		case WM_NOTIFY:
		{
			lpnmud = (LPNMUPDOWN) lParam;
			lppsn = (LPPSHNOTIFY) lParam; 

			if (lppsn->hdr.code == PSN_APPLY)
			{
				if (!pConInfo->AppliedConfig)
				{
					ApplyConsoleInfo(hwndDlg, pConInfo);
				}
				else
				{
					/* options have already been applied */
					SetWindowLong(hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR);
					return TRUE;
				}
				return TRUE;
			}			

			if (lpnmud->hdr.idFrom == IDC_UPDOWN_COLOR_RED)
			{
				red = lpnmud->iPos;
			}
			else if (lpnmud->hdr.idFrom == IDC_UPDOWN_COLOR_GREEN)
			{
				green = lpnmud->iPos;
			}
			else if (lpnmud->hdr.idFrom == IDC_UPDOWN_COLOR_BLUE)
			{
				blue = lpnmud->iPos;
			}
			else
			{
				break;
			}

			if (red == -1)
			{
				red = SendMessage(GetDlgItem(hwndDlg, IDC_UPDOWN_COLOR_RED), UDM_GETPOS, 0, 0);
				if (HIWORD(red))
				{
					//TODO: handle error
					break;
				}
				red = LOBYTE(red);
			}

			if (green == -1)
			{
				green = SendMessage(GetDlgItem(hwndDlg, IDC_UPDOWN_COLOR_GREEN), UDM_GETPOS, 0, 0);
				if (HIWORD(green))
				{
					//TODO: handle error
					break;
				}
				green = LOBYTE(green);
			}

			if (blue == -1)
			{
				blue = SendMessage(GetDlgItem(hwndDlg, IDC_UPDOWN_COLOR_BLUE), UDM_GETPOS, 0, 0);
				if (HIWORD(blue))
				{
					//TODO: handle error
					break;
				}
				blue = LOBYTE(blue);
			}
			s_Colors[pConInfo->ActiveStaticControl] = RGB(red, green, blue);
			InvalidateRect(hwndDlg, NULL, TRUE); //FIXME
			break;
		}
		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDC_RADIO_SCREEN_TEXT:
				{
					SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_RED, GetRValue(pConInfo->ScreenText), FALSE);
					SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_GREEN, GetGValue(pConInfo->ScreenText), FALSE);					
					SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_BLUE, GetBValue(pConInfo->ScreenText), FALSE);
					InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_SCREEN_COLOR), NULL, TRUE);
					InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_POPUP_COLOR), NULL, TRUE);
					break;
				}
				case IDC_RADIO_SCREEN_BACKGROUND:
				{
					SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_RED, GetRValue(pConInfo->ScreenBackground), FALSE);
					SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_GREEN, GetGValue(pConInfo->ScreenBackground), FALSE);					
					SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_BLUE, GetBValue(pConInfo->ScreenBackground), FALSE);
					InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_SCREEN_COLOR), NULL, TRUE);
					InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_POPUP_COLOR), NULL, TRUE);
					break;
				}
				case IDC_RADIO_POPUP_TEXT:
				{
					SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_RED, GetRValue(pConInfo->PopupText), FALSE);
					SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_GREEN, GetGValue(pConInfo->PopupText), FALSE);					
					SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_BLUE, GetBValue(pConInfo->PopupText), FALSE);
					InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_SCREEN_COLOR), NULL, TRUE);
					InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_POPUP_COLOR), NULL, TRUE);
					break;
				}
				case IDC_RADIO_POPUP_BACKGROUND:
				{
					SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_RED, GetRValue(pConInfo->PopupBackground), FALSE);
					SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_GREEN, GetGValue(pConInfo->PopupBackground), FALSE);					
					SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_BLUE, GetBValue(pConInfo->PopupBackground), FALSE);
					InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_SCREEN_COLOR), NULL, TRUE);
					InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_POPUP_COLOR), NULL, TRUE);
					break;
				}
			}
			if (HIWORD(wParam) == STN_CLICKED && LOWORD(wParam) >= IDC_STATIC_COLOR1 && LOWORD(wParam) <= IDC_STATIC_COLOR16)
			{
				DWORD index = LOWORD(wParam) - IDC_STATIC_COLOR1;

				SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_RED, GetRValue(s_Colors[index]), FALSE);
				SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_GREEN, GetGValue(s_Colors[index]), FALSE);
				SetDlgItemInt(hwndDlg, IDC_EDIT_COLOR_BLUE, GetBValue(s_Colors[index]), FALSE);

				/* update global struct */
				if (SendMessage(GetDlgItem(hwndDlg, IDC_RADIO_SCREEN_TEXT), BM_GETCHECK, 0, 0) & BST_CHECKED)
				{
					pConInfo->ScreenText = s_Colors[index];
				}
				else if (SendMessage(GetDlgItem(hwndDlg, IDC_RADIO_SCREEN_BACKGROUND), BM_GETCHECK, 0, 0) & BST_CHECKED)					
				{
					pConInfo->ScreenBackground = s_Colors[index];
				}
				else if (SendMessage(GetDlgItem(hwndDlg, IDC_RADIO_POPUP_TEXT), BM_GETCHECK, 0, 0) & BST_CHECKED)					
				{
					pConInfo->PopupText = s_Colors[index];
				}
				else if (SendMessage(GetDlgItem(hwndDlg, IDC_RADIO_POPUP_BACKGROUND), BM_GETCHECK, 0, 0) & BST_CHECKED)					
				{
					pConInfo->PopupBackground = s_Colors[index];
				}
				InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + pConInfo->ActiveStaticControl), NULL, TRUE);
				InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_COLOR1 + index), NULL, TRUE);
				InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_SCREEN_COLOR), NULL, TRUE);
				InvalidateRect(GetDlgItem(hwndDlg, IDC_STATIC_POPUP_COLOR), NULL, TRUE);
				pConInfo->ActiveStaticControl = index;
				break;
			}
		}


		default:
			break;
	}

	return FALSE;
}

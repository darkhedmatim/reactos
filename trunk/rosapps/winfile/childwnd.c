/*
 *  ReactOS winfile
 *
 *  childwnd.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
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

#ifdef _MSC_VER
#include "stdafx.h"
#else
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
#endif
    
#include <windowsx.h>
#include <ctype.h>
#include <assert.h>
#define ASSERT assert

#include "main.h"
#include "childwnd.h"
#include "framewnd.h"
#include "treeview.h"
#include "listview.h"
#include "dialogs.h"
#include "utils.h"
#include "run.h"
#include "trace.h"


#ifdef _NO_EXTENSIONS
//#define	COLOR_SPLITBAR		WHITE_BRUSH
#define	COLOR_SPLITBAR		LTGRAY_BRUSH
#else
#define	COLOR_SPLITBAR		LTGRAY_BRUSH
#endif

////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//


////////////////////////////////////////////////////////////////////////////////
// Local module support methods
//
/*
#ifndef _NO_EXTENSIONS

void set_header(Pane* pane)
{
	HD_ITEM item;
	int scroll_pos = GetScrollPos(pane->hWnd, SB_HORZ);
	int i=0, x=0;

	item.mask = HDI_WIDTH;
	item.cxy = 0;

	for(; x+pane->widths[i]<scroll_pos && i<COLUMNS; i++) {
		x += pane->widths[i];
		Header_SetItem(pane->hwndHeader, i, &item);
	}

	if (i < COLUMNS) {
		x += pane->widths[i];
		item.cxy = x - scroll_pos;
		Header_SetItem(pane->hwndHeader, i++, &item);

		for(; i<COLUMNS; i++) {
			item.cxy = pane->widths[i];
			x += pane->widths[i];
			Header_SetItem(pane->hwndHeader, i, &item);
		}
	}
}

static LRESULT pane_notify(Pane* pane, NMHDR* pnmh)
{
	switch(pnmh->code) {
	case HDN_TRACK:
	case HDN_ENDTRACK:
		{
		HD_NOTIFY* phdn = (HD_NOTIFY*) pnmh;
		int idx = phdn->iItem;
		int dx = phdn->pitem->cxy - pane->widths[idx];
		int i;

		RECT clnt;
		GetClientRect(pane->hWnd, &clnt);
		 // move immediate to simulate HDS_FULLDRAG (for now [04/2000] not realy needed with WINELIB)
		Header_SetItem(pane->hwndHeader, idx, phdn->pitem);
		pane->widths[idx] += dx;
		for (i = idx; ++i <= COLUMNS; )
			pane->positions[i] += dx;
		{
		int scroll_pos = GetScrollPos(pane->hWnd, SB_HORZ);
		RECT rt_scr = {pane->positions[idx+1]-scroll_pos, 0, clnt.right, clnt.bottom};
		RECT rt_clip = {pane->positions[idx]-scroll_pos, 0, clnt.right, clnt.bottom};
		if (rt_scr.left < 0) rt_scr.left = 0;
		if (rt_clip.left < 0) rt_clip.left = 0;
		ScrollWindowEx(pane->hWnd, dx, 0, &rt_scr, &rt_clip, 0, 0, SW_INVALIDATE);
		rt_clip.right = pane->positions[idx+1];
		RedrawWindow(pane->hWnd, &rt_clip, 0, RDW_INVALIDATE|RDW_UPDATENOW);
		if (pnmh->code == HDN_ENDTRACK) {
			ListBox_SetHorizontalExtent(pane->hWnd, pane->positions[COLUMNS]);
			if (GetScrollPos(pane->hWnd, SB_HORZ) != scroll_pos)
				set_header(pane);
		}
		}
		}
		return FALSE;
	case HDN_DIVIDERDBLCLICK:
		{
		HD_NOTIFY* phdn = (HD_NOTIFY*) pnmh;
		HD_ITEM item;
		calc_single_width(pane, phdn->iItem);
		item.mask = HDI_WIDTH;
		item.cxy = pane->widths[phdn->iItem];
		Header_SetItem(pane->hwndHeader, phdn->iItem, &item);
		InvalidateRect(pane->hWnd, 0, TRUE);
		break;
		}
	}
	return 0;
}

#endif

static BOOL pane_command(Pane* pane, UINT cmd)
{
	switch(cmd) {
	case ID_VIEW_NAME:
		if (pane->visible_cols) {
			pane->visible_cols = 0;
			calc_widths(pane, TRUE);
#ifndef _NO_EXTENSIONS
			set_header(pane);
#endif
			InvalidateRect(pane->hWnd, 0, TRUE);
			CheckMenuItem(Globals.hMenuView, ID_VIEW_NAME, MF_BYCOMMAND|MF_CHECKED);
//			CheckMenuItem(Globals.hMenuView, ID_VIEW_ALL_ATTRIBUTES, MF_BYCOMMAND);
//			CheckMenuItem(Globals.hMenuView, ID_VIEW_SELECTED_ATTRIBUTES, MF_BYCOMMAND);
		}
		break;
#if 0
	case ID_VIEW_ALL_ATTRIBUTES:
		if (pane->visible_cols != COL_ALL) {
			pane->visible_cols = COL_ALL;
			calc_widths(pane, TRUE);
#ifndef _NO_EXTENSIONS
			set_header(pane);
#endif
			InvalidateRect(pane->hWnd, 0, TRUE);
			CheckMenuItem(Globals.hMenuView, ID_VIEW_NAME, MF_BYCOMMAND);
//			CheckMenuItem(Globals.hMenuView, ID_VIEW_ALL_ATTRIBUTES, MF_BYCOMMAND|MF_CHECKED);
//			CheckMenuItem(Globals.hMenuView, ID_VIEW_SELECTED_ATTRIBUTES, MF_BYCOMMAND);
		}
		break;
#ifndef _NO_EXTENSIONS
	case ID_PREFERED_SIZES: {
		calc_widths(pane, TRUE);
		set_header(pane);
		InvalidateRect(pane->hWnd, 0, TRUE);
		break;}
#endif
#endif
		// TODO: more command ids...
	default:
		return FALSE;
	}
	return TRUE;
}
 */
////////////////////////////////////////////////////////////////////////////////
//
//HWND hSplitWnd;                  // Splitter Bar Control Window
//
//    hSplitWnd = CreateWindow(szFrameClass, "splitter window", WS_VISIBLE|WS_CHILD,
//                            CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 
//                            Globals.hMainWnd, (HMENU)SPLIT_WINDOW, hInstance, NULL);
//    if (!hSplitWnd)
//        return FALSE;
//    hTreeWnd = CreateTreeView(Globals.hMDIClient, "c:\\foobar.txt");
//    if (!hTreeWnd)
//        return FALSE;
//    hListWnd = CreateListView(Globals.hMDIClient, "");
//    if (!hListWnd)
//        return FALSE;
//
////////////////////////////////////////////////////////////////////////////////

static void draw_splitbar(HWND hWnd, int x)
{
	RECT rt;
	HDC hdc = GetDC(hWnd);

	GetClientRect(hWnd, &rt);
	rt.left = x - SPLIT_WIDTH/2;
	rt.right = x + SPLIT_WIDTH/2+1;
	InvertRect(hdc, &rt);
	ReleaseDC(hWnd, hdc);
}

#if 1
static void OnPaint(HWND hWnd, ChildWnd* pChildWnd)
{
    HBRUSH lastBrush;
	PAINTSTRUCT ps;
	RECT rt;

	BeginPaint(hWnd, &ps);
	GetClientRect(hWnd, &rt);
    lastBrush = SelectObject(ps.hdc, (HBRUSH)GetStockObject(COLOR_SPLITBAR));
    Rectangle(ps.hdc, rt.left, rt.top-1, rt.right, rt.bottom+1);
    SelectObject(ps.hdc, lastBrush);
//    rt.top = rt.bottom - GetSystemMetrics(SM_CYHSCROLL);
//    FillRect(ps.hdc, &rt, GetStockObject(BLACK_BRUSH));
	EndPaint(hWnd, &ps);
}
#else
static void OnPaint(HWND hWnd, ChildWnd* pChildWnd)
{
    HBRUSH lastBrush;
	PAINTSTRUCT ps;
	RECT rt;

	GetClientRect(hWnd, &rt);
	BeginPaint(hWnd, &ps);

    lastBrush = SelectObject(ps.hdc, (HBRUSH)GetStockObject(WHITE_BRUSH));
    Rectangle(ps.hdc, rt.left, rt.top-1, rt.right, rt.bottom+1);
    SelectObject(ps.hdc, lastBrush);
    rt.top = rt.bottom - GetSystemMetrics(SM_CYHSCROLL);
    FillRect(ps.hdc, &rt, GetStockObject(BLACK_BRUSH));
/*
    rt.left = pChildWnd->nSplitPos-SPLIT_WIDTH/2;
    rt.right = pChildWnd->nSplitPos+SPLIT_WIDTH/2+1;
	lastBrush = SelectBrush(ps.hdc, (HBRUSH)GetStockObject(COLOR_SPLITBAR));
	Rectangle(ps.hdc, rt.left, rt.top-1, rt.right, rt.bottom+1);
	SelectObject(ps.hdc, lastBrush);
#ifdef _NO_EXTENSIONS
	rt.top = rt.bottom - GetSystemMetrics(SM_CYHSCROLL);
	FillRect(ps.hdc, &rt, GetStockObject(BLACK_BRUSH));
#endif
 */
	EndPaint(hWnd, &ps);
}
#endif

//void ResizeWnd(ChildWnd* child, int cx, int cy);
static void ResizeWnd(ChildWnd* child, int cx, int cy)
{
	HDWP hdwp = BeginDeferWindowPos(4);
	RECT rt = {0, 0, cx, cy};

	cx = child->nSplitPos + SPLIT_WIDTH/2;
#ifndef _NO_EXTENSIONS
	{
		WINDOWPOS wp;
		HD_LAYOUT hdl = {&rt, &wp};
		Header_Layout(child->left.hwndHeader, &hdl);
		DeferWindowPos(hdwp, child->left.hwndHeader, wp.hwndInsertAfter,
						wp.x-1, wp.y, child->nSplitPos-SPLIT_WIDTH/2+1, wp.cy, wp.flags);
		DeferWindowPos(hdwp, child->right.hwndHeader, wp.hwndInsertAfter,
						rt.left+cx+1, wp.y, wp.cx-cx+2, wp.cy, wp.flags);
	}
#endif
	DeferWindowPos(hdwp, child->left.hWnd, 0, rt.left, rt.top, child->nSplitPos-SPLIT_WIDTH/2-rt.left, rt.bottom-rt.top, SWP_NOZORDER|SWP_NOACTIVATE);
	DeferWindowPos(hdwp, child->right.hWnd, 0, rt.left+cx+1, rt.top, rt.right-cx, rt.bottom-rt.top, SWP_NOZORDER|SWP_NOACTIVATE);
	EndDeferWindowPos(hdwp);
}

static void OnSize(ChildWnd* pChildWnd, WPARAM wParam, LPARAM lParam)
{
    if (wParam != SIZE_MINIMIZED) {
		ResizeWnd(pChildWnd, LOWORD(lParam), HIWORD(lParam));
    }
}

void OnFileMove(HWND hWnd)
{
	struct ExecuteDialog dlg = {{0}};
    if (DialogBoxParam(Globals.hInstance, MAKEINTRESOURCE(IDD_DIALOG_FILE_MOVE), hWnd, MoveFileWndProc, (LPARAM)&dlg) == IDOK) {
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: _CmdWndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes WM_COMMAND messages for the main frame window.
//
//

static BOOL _CmdWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	UINT cmd = LOWORD(wParam);
	//HWND hChildWnd;

    if (1) {
		switch (cmd) {
/*
//        case ID_FILE_MOVE:
//            OnFileMove(hWnd);
//            break;
        case ID_FILE_COPY:
        case ID_FILE_COPY_CLIPBOARD:
        case ID_FILE_DELETE:
        case ID_FILE_RENAME:
        case ID_FILE_PROPERTIES:
        case ID_FILE_COMPRESS:
        case ID_FILE_UNCOMPRESS:
            break;
//        case ID_FILE_RUN:
//            OnFileRun();
//            break;
        case ID_FILE_PRINT:
        case ID_FILE_ASSOCIATE:
        case ID_FILE_CREATE_DIRECTORY:
        case ID_FILE_SEARCH:
        case ID_FILE_SELECT_FILES:
            break;
 */
        case ID_FILE_EXIT:
            SendMessage(hWnd, WM_CLOSE, 0, 0);
            break;
/*
        case ID_DISK_COPY_DISK:
			break;
        case ID_DISK_LABEL_DISK:
			break;
        case ID_DISK_CONNECT_NETWORK_DRIVE:
            MapNetworkDrives(hWnd, TRUE);
			break;
        case ID_DISK_DISCONNECT_NETWORK_DRIVE:
            MapNetworkDrives(hWnd, FALSE);
			break;
        case ID_DISK_SHARE_AS:
			break;
        case ID_DISK_STOP_SHARING:
			break;
        case ID_DISK_SELECT_DRIVE:
			break;
 */
/*
        case ID_TREE_EXPAND_ONE_LEVEL:
        case ID_TREE_EXPAND_ALL:
        case ID_TREE_EXPAND_BRANCH:
        case ID_TREE_COLLAPSE_BRANCH:
            MessageBeep(-1);
			break;
 */
        case ID_VIEW_BY_FILE_TYPE:
			{
			struct ExecuteDialog dlg = {{0}};
            if (DialogBoxParam(Globals.hInstance, MAKEINTRESOURCE(IDD_DIALOG_VIEW_TYPE), hWnd, ViewFileTypeWndProc, (LPARAM)&dlg) == IDOK) {
            }
			}
			break;
        case ID_OPTIONS_CONFIRMATION:
			{
			struct ExecuteDialog dlg = {{0}};
            if (DialogBoxParam(Globals.hInstance, MAKEINTRESOURCE(IDD_DIALOG_OPTIONS_CONFIRMATON), hWnd, OptionsConfirmationWndProc, (LPARAM)&dlg) == IDOK) {
            }
			}
            break;
		case ID_WINDOW_NEW_WINDOW:
            CreateChildWindow(-1);
//			{
//			ChildWnd* pChildWnd = alloc_child_window(path);
//			if (!create_child_window(pChildWnd))
//				free(pChildWnd);
//			}
			break;
		default:
//  			return DefMDIChildProc(hWnd, message, wParam, lParam);
            return FALSE;
            break;
		}
	}
	return TRUE;
}


//
//  FUNCTION: ChildWndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the child windows.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int last_split;

	ChildWnd* pChildWnd = (ChildWnd*)GetWindowLong(hWnd, GWL_USERDATA);
	ASSERT(pChildWnd);

	switch(message) {
		case WM_CREATE:
	        CreateTreeWnd(pChildWnd->hWnd, &pChildWnd->left, IDW_TREE_LEFT);
            CreateListWnd(pChildWnd->hWnd, &pChildWnd->right, IDW_TREE_RIGHT, pChildWnd->szPath);
            //create_tree_window(pChildWnd->hWnd, &pChildWnd->left, IDW_TREE_LEFT, IDW_HEADER_LEFT, pChildWnd->szPath);
            //create_list_window(pChildWnd->hWnd, &pChildWnd->right, IDW_TREE_RIGHT, IDW_HEADER_RIGHT);
            return 0;
			break;

		case WM_PAINT:
            OnPaint(hWnd, pChildWnd);
			break;

		case WM_NCDESTROY:
//			free_child_window(pChildWnd);
			SetWindowLong(hWnd, GWL_USERDATA, 0);
			break;

		case WM_SETCURSOR:
			if (LOWORD(lParam) == HTCLIENT) {
				POINT pt;
				GetCursorPos(&pt);
				ScreenToClient(hWnd, &pt);
				if (pt.x>=pChildWnd->nSplitPos-SPLIT_WIDTH/2 && pt.x<pChildWnd->nSplitPos+SPLIT_WIDTH/2+1) {
					SetCursor(LoadCursor(0, IDC_SIZEWE));
					return TRUE;
				}
			}
			goto def;

		case WM_LBUTTONDOWN: {
			RECT rt;
			int x = LOWORD(lParam);

			GetClientRect(hWnd, &rt);
			if (x>=pChildWnd->nSplitPos-SPLIT_WIDTH/2 && x<pChildWnd->nSplitPos+SPLIT_WIDTH/2+1) {
				last_split = pChildWnd->nSplitPos;
#ifdef _NO_EXTENSIONS
				draw_splitbar(hWnd, last_split);
#endif
				SetCapture(hWnd);
			}
			break;}

		case WM_LBUTTONUP:
			if (GetCapture() == hWnd) {
#ifdef _NO_EXTENSIONS
				RECT rt;
				int x = LOWORD(lParam);
				draw_splitbar(hWnd, last_split);
				last_split = -1;
				GetClientRect(hWnd, &rt);
				pChildWnd->nSplitPos = x;
				ResizeWnd(pChildWnd, rt.right, rt.bottom);
#endif
				ReleaseCapture();
			}
			break;

#ifdef _NO_EXTENSIONS
		case WM_CAPTURECHANGED:
			if (GetCapture()==hWnd && last_split>=0)
				draw_splitbar(hWnd, last_split);
			break;
#endif

		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE)
				if (GetCapture() == hWnd) {
					RECT rt;
#ifdef _NO_EXTENSIONS
					draw_splitbar(hWnd, last_split);
#else
					pChildWnd->nSplitPos = last_split;
#endif
					GetClientRect(hWnd, &rt);
					ResizeWnd(pChildWnd, rt.right, rt.bottom);
					last_split = -1;
					ReleaseCapture();
					SetCursor(LoadCursor(0, IDC_ARROW));
				}
			break;

		case WM_MOUSEMOVE:
			if (GetCapture() == hWnd) {
				RECT rt;
				int x = LOWORD(lParam);

#ifdef _NO_EXTENSIONS
				HDC hdc = GetDC(hWnd);
				GetClientRect(hWnd, &rt);

				rt.left = last_split-SPLIT_WIDTH/2;
				rt.right = last_split+SPLIT_WIDTH/2+1;
				InvertRect(hdc, &rt);

				last_split = x;
				rt.left = x-SPLIT_WIDTH/2;
				rt.right = x+SPLIT_WIDTH/2+1;
				InvertRect(hdc, &rt);

				ReleaseDC(hWnd, hdc);
#else
				GetClientRect(hWnd, &rt);

				if (x>=0 && x<rt.right) {
					pChildWnd->nSplitPos = x;
					ResizeWnd(pChildWnd, rt.right, rt.bottom);
					rt.left = x-SPLIT_WIDTH/2;
					rt.right = x+SPLIT_WIDTH/2+1;
					InvalidateRect(hWnd, &rt, FALSE);
					UpdateWindow(pChildWnd->left.hWnd);
					UpdateWindow(hWnd);
					UpdateWindow(pChildWnd->right.hWnd);
				}
#endif
			}
			break;

#ifndef _NO_EXTENSIONS
		case WM_GETMINMAXINFO:
			DefMDIChildProc(hWnd, message, wParam, lParam);

			{LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;

			lpmmi->ptMaxTrackSize.x <<= 1;//2*GetSystemMetrics(SM_CXSCREEN) / SM_CXVIRTUALSCREEN
			lpmmi->ptMaxTrackSize.y <<= 1;//2*GetSystemMetrics(SM_CYSCREEN) / SM_CYVIRTUALSCREEN
			break;}
#endif

		case WM_SETFOCUS:
			SetCurrentDirectory(pChildWnd->szPath);
			SetFocus(pChildWnd->nFocusPanel? pChildWnd->right.hWnd: pChildWnd->left.hWnd);
			break;

	case WM_DISPATCH_COMMAND:
        if (_CmdWndProc(hWnd, message, wParam, lParam)) break;
        if (1) {
            return SendMessage(pChildWnd->right.hWnd, message, wParam, lParam);
        } else {
            return SendMessage(pChildWnd->left.hWnd, message, wParam, lParam);
        }
		break;

	case WM_COMMAND:
/*
        if (!SendMessage(pChildWnd->right.hWnd, message, wParam, lParam)) {
            return DefMDIChildProc(hWnd, message, wParam, lParam);
        } else {
    		return _CmdWndProc(hWnd, message, wParam, lParam);
//  		return DefMDIChildProc(hWnd, message, wParam, lParam);
        }
		break;
 */
        if (_CmdWndProc(hWnd, message, wParam, lParam)) break;
			return DefMDIChildProc(hWnd, message, wParam, lParam);

//        if (LOWORD(wParam) > ID_CMD_FIRST && LOWORD(wParam) < ID_CMD_LAST) {
//            if (!SendMessage(pChildWnd->right.hWnd, message, wParam, lParam)) {
//                return DefMDIChildProc(hWnd, message, wParam, lParam);
//            }
//        } else {
//    		return _CmdWndProc(hWnd, message, wParam, lParam);
//        }
		break;
/*
		case WM_COMMAND:
            {
			Pane* pane = GetFocus()==pChildWnd->left.hWnd? &pChildWnd->left: &pChildWnd->right;

			switch(HIWORD(wParam)) {
				case LBN_SELCHANGE: {
					int idx = ListBox_GetCurSel(pane->hWnd);
					Entry* entry = (Entry*) ListBox_GetItemData(pane->hWnd, idx);

					if (pane == &pChildWnd->left)
						set_curdir(pChildWnd, entry);
					else
						pane->cur = entry;
					break;}

				case LBN_DBLCLK:
					activate_entry(pChildWnd, pane);
					break;
			}
			}
			break;

		case WM_DISPATCH_COMMAND: {
			Pane* pane = GetFocus()==pChildWnd->left.hWnd? &pChildWnd->left: &pChildWnd->right;

			switch(LOWORD(wParam)) {
				case ID_WINDOW_NEW_WINDOW:
                    //CreateChildWindow(pChildWnd->szPath);
                    CreateChildWindow(-1);
//                    {
//					ChildWnd* new_child = alloc_child_window(pChildWnd->szPath);
//					if (!create_child_window(new_child))
//						free(new_child);
//                    }
					break;
#if 0
				case ID_REFRESH:
					scan_entry(pChildWnd, pane->cur);
					break;
				case ID_ACTIVATE:
					activate_entry(pChildWnd, pane);
					break;
#endif
				case ID_WINDOW_CASCADE:
					SendMessage(Globals.hMDIClient, WM_MDICASCADE, 0, 0);
					break;
				case ID_WINDOW_TILE_HORZ:
					SendMessage(Globals.hMDIClient, WM_MDITILE, MDITILE_HORIZONTAL, 0);
					break;
				case ID_WINDOW_TILE_VERT:
					SendMessage(Globals.hMDIClient, WM_MDITILE, MDITILE_VERTICAL, 0);
					break;
				case ID_WINDOW_ARRANGE_ICONS:
					SendMessage(Globals.hMDIClient, WM_MDIICONARRANGE, 0, 0);
					break;
				default:
					return pane_command(pane, LOWORD(wParam));
			}

			return TRUE;}
 */
//#ifndef _NO_EXTENSIONS
		case WM_NOTIFY: {
            int idCtrl = (int)wParam; 
			//NMHDR* pnmh = (NMHDR*)lParam;
			//return pane_notify(pnmh->idFrom==IDW_HEADER_LEFT? &pChildWnd->left: &pChildWnd->right, pnmh);
            if (idCtrl == IDW_TREE_LEFT) {
                if ((((LPNMHDR)lParam)->code) == TVN_SELCHANGED) {
                    Entry* entry = (Entry*)((NMTREEVIEW*)lParam)->itemNew.lParam;
                    if (entry != NULL) {
                        //RefreshList(pChildWnd->right.hWnd, entry);
                        //void set_curdir(ChildWnd* child, Entry* entry)
                        set_curdir(pChildWnd, entry);
                    }
                }
                if (!SendMessage(pChildWnd->left.hWnd, message, wParam, lParam)) {
                    return DefMDIChildProc(hWnd, message, wParam, lParam);
                }
            }
            if (idCtrl == IDW_TREE_RIGHT) {
                if (!SendMessage(pChildWnd->right.hWnd, message, wParam, lParam)) {
                    return DefMDIChildProc(hWnd, message, wParam, lParam);
                }
            }
                        }
//#endif
            break;

		case WM_SIZE:
            if (wParam != SIZE_MINIMIZED) {
                OnSize(pChildWnd, wParam, lParam);
            }
            // fall through
		default: def:
			return DefMDIChildProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

ATOM RegisterChildWnd(HINSTANCE hInstance, int res_id)
{
	WNDCLASSEX wcFrame = {
		sizeof(WNDCLASSEX),
		CS_HREDRAW | CS_VREDRAW/*style*/,
		FrameWndProc,
		0/*cbClsExtra*/,
		0/*cbWndExtra*/,
		hInstance,
		LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINFILE)),
		LoadCursor(0, IDC_ARROW),
		0/*hbrBackground*/,
		0/*lpszMenuName*/,
		szFrameClass,
		(HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_WINFILE), IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED)
	};
	ATOM hFrameWndClass = RegisterClassEx(&wcFrame); // register frame window class
    return hFrameWndClass;
}

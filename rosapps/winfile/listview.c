/*
 *  ReactOS winfile
 *
 *  listview.c
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
    
#include <shellapi.h>
//#include <winspool.h>
#include <windowsx.h>
#include <shellapi.h>
#include <ctype.h>
#include <assert.h>
#define ASSERT assert

#include "main.h"
#include "listview.h"
//#include "entries.h"
#include "utils.h"

#include "trace.h"

// Global Variables:
extern HINSTANCE hInst;


static void init_output(HWND hWnd)
{
	TCHAR b[16];
	HFONT old_font;
	HDC hdc = GetDC(hWnd);

	if (GetNumberFormat(LOCALE_USER_DEFAULT, 0, _T("1000"), 0, b, 16) > 4)
		Globals.num_sep = b[1];
	else
		Globals.num_sep = _T('.');

	old_font = SelectFont(hdc, Globals.hFont);
	GetTextExtentPoint32(hdc, _T(" "), 1, &Globals.spaceSize);
	SelectFont(hdc, old_font);
	ReleaseDC(hWnd, hdc);
}

static void AddEntryToList(HWND hwndLV, int idx, Entry* entry)
{ 
    LVITEM item;

    item.mask = LVIF_TEXT | LVIF_PARAM; 
    item.iItem = 0;//idx; 
    item.iSubItem = 0; 
    item.state = 0; 
    item.stateMask = 0; 
//    item.pszText = entry->data.cFileName; 
    item.pszText = LPSTR_TEXTCALLBACK; 
    item.cchTextMax = strlen(entry->data.cFileName); 
    item.iImage = 0; 
//    item.iImage = I_IMAGECALLBACK; 
    item.lParam = (LPARAM)entry;
#if (_WIN32_IE >= 0x0300)
    item.iIndent = 0;
#endif
    ListView_InsertItem(hwndLV, &item);
}

// insert listctrl entries after index idx
static void InsertListEntries(Pane* pane, Entry* parent, int idx)
{
	Entry* entry = parent;

	if (!entry)
		return;
	ShowWindow(pane->hWnd, SW_HIDE);

    if (idx == -1) {
    }
    idx = 0;

	for(; entry; entry=entry->next) {
#ifndef _LEFT_FILES
		if (entry->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
#endif
        //ListBox_InsertItemData(pane->hWnd, idx, entry);
        AddEntryToList(pane->hWnd, idx, entry); 
        ++idx;
	}
	ShowWindow(pane->hWnd, SW_SHOW);
}

static void CreateListColumns(HWND hWndListView)
{
    char szText[50];
    int index;
    LV_COLUMN lvC;
 
    // Create columns.
    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvC.fmt = LVCFMT_LEFT;
    lvC.cx = 175;
    lvC.pszText = szText;

    // Load the column labels from the resource file.
    for (index = 0; index < 4; index++) {
        lvC.iSubItem = index;
        LoadString(hInst, IDS_LIST_COLUMN_FIRST + index, szText, sizeof(szText));
        if (ListView_InsertColumn(hWndListView, index, &lvC) == -1) {
            // TODO: handle failure condition...
            break;
        }
    }
}

static HWND CreateListView(HWND hwndParent, int id) 
{ 
    RECT rcClient;  // dimensions of client area 
    HWND hwndLV;    // handle to list view control 

    // Get the dimensions of the parent window's client area, and create the list view control. 
    GetClientRect(hwndParent, &rcClient); 
    hwndLV = CreateWindowEx(0, WC_LISTVIEW, "List View", 
//        WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT | LVS_NOCOLUMNHEADER, 
        WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT, 
//        WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_LIST | LVS_NOCOLUMNHEADER,
        0, 0, rcClient.right, rcClient.bottom, 
        hwndParent, (HMENU)id, hInst, NULL); 

    // Initialize the image list, and add items to the control. 
/*
    if (!InitListViewImageLists(hwndLV) || 
            !InitListViewItems(hwndLV, lpszPathName)) { 
        DestroyWindow(hwndLV); 
        return FALSE; 
    } 
 */
    ListView_SetExtendedListViewStyle(hwndLV,  LVS_EX_FULLROWSELECT);
    CreateListColumns(hwndLV);

    return hwndLV;
} 

// OnGetDispInfo - processes the LVN_GETDISPINFO 
// notification message. 
 
void OnGetDispInfo(NMLVDISPINFO* plvdi)
{
    static char buffer[200];

//    LVITEM* pItem = &(plvdi->item);
//    Entry* entry = (Entry*)pItem->lParam;
    Entry* entry = (Entry*)plvdi->item.lParam;
    ASSERT(entry);

    switch (plvdi->item.iSubItem) {
    case 0:
        plvdi->item.pszText = entry->data.cFileName; 
//    item.cchTextMax = strlen(entry->data.cFileName); 
//        plvdi->item.pszText = rgPetInfo[plvdi->item.iItem].szKind;
        break;
    case 1:
        if (entry->bhfi_valid) {
            //entry->bhfi.nFileSizeLow;
            //entry->bhfi.nFileSizeHigh;

            //entry->bhfi.ftCreationTime

            wsprintf(buffer, "%u", entry->bhfi.nFileSizeLow);
            plvdi->item.pszText = buffer;
        } else {
            plvdi->item.pszText = "unknown";
        }
        break;
    case 2:
        plvdi->item.pszText = "TODO:";
        break;
    case 3:
            //entry->bhfi.dwFileAttributes
//        plvdi->item.pszText = "rhsa";
        strcpy(buffer, " ");
        if (entry->bhfi.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) strcat(buffer, "a"); else strcat(buffer, " ");
        if (entry->bhfi.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) strcat(buffer, "c"); else strcat(buffer, " ");
        if (entry->bhfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) strcat(buffer, "d"); else strcat(buffer, " ");
        if (entry->bhfi.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) strcat(buffer, "e"); else strcat(buffer, " ");
        if (entry->bhfi.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) strcat(buffer, "h"); else strcat(buffer, " ");
        if (entry->bhfi.dwFileAttributes & FILE_ATTRIBUTE_NORMAL) strcat(buffer, "n"); else strcat(buffer, " ");
        if (entry->bhfi.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE) strcat(buffer, "o"); else strcat(buffer, " ");
        if (entry->bhfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY) strcat(buffer, "r"); else strcat(buffer, " ");
        if (entry->bhfi.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) strcat(buffer, "p"); else strcat(buffer, " ");
        if (entry->bhfi.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) strcat(buffer, "f"); else strcat(buffer, " ");
        if (entry->bhfi.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) strcat(buffer, "s"); else strcat(buffer, " ");
        if (entry->bhfi.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY) strcat(buffer, "t"); else strcat(buffer, " ");
        plvdi->item.pszText = buffer;
/*
FILE_ATTRIBUTE_ARCHIVE The file or directory is an archive file. Applications use this attribute to mark files for backup or removal. 
FILE_ATTRIBUTE_COMPRESSED The file or directory is compressed. For a file, this means that all of the data in the file is compressed. For a directory, this means that compression is the default for newly created files and subdirectories. 
FILE_ATTRIBUTE_DIRECTORY The handle identifies a directory. 
FILE_ATTRIBUTE_ENCRYPTED The file or directory is encrypted. For a file, this means that all data in the file is encrypted. For a directory, this means that encryption is the default for newly created files and subdirectories. 
FILE_ATTRIBUTE_HIDDEN The file or directory is hidden. It is not included in an ordinary directory listing. 
FILE_ATTRIBUTE_NORMAL The file has no other attributes. This attribute is valid only if used alone. 
FILE_ATTRIBUTE_OFFLINE The file data is not immediately available. This attribute indicates that the file data has been physically moved to offline storage. This attribute is used by Remote Storage, the hierarchical storage management software in Windows 2000. Applications should not arbitrarily change this attribute. 
FILE_ATTRIBUTE_READONLY The file or directory is read-only. Applications can read the file but cannot write to it or delete it. In the case of a directory, applications cannot delete it. 
FILE_ATTRIBUTE_REPARSE_POINT The file has an associated reparse point. 
FILE_ATTRIBUTE_SPARSE_FILE The file is a sparse file. 
FILE_ATTRIBUTE_SYSTEM The file or directory is part of the operating system or is used exclusively by the operating system. 
FILE_ATTRIBUTE_TEMPORARY The file is being used for temporary storage. File systems attempt to keep all the data in memory for quicker access, rather than flushing the data back to mass storage. A temporary file should be deleted by the application as soon as it is no longer needed.
 */
        break;
    default:
        break;
    }
} 
/*
typedef struct _BY_HANDLE_FILE_INFORMATION {
  DWORD    dwFileAttributes; 
  FILETIME ftCreationTime; 
  FILETIME ftLastAccessTime; 
  FILETIME ftLastWriteTime; 
  DWORD    dwVolumeSerialNumber; 
  DWORD    nFileSizeHigh; 
  DWORD    nFileSizeLow; 
  DWORD    nNumberOfLinks; 
  DWORD    nFileIndexHigh; 
  DWORD    nFileIndexLow; 
} BY_HANDLE_FILE_INFORMATION, *PBY_HANDLE_FILE_INFORMATION;  
 */ 


 // OnEndLabelEdit - processes the LVN_ENDLABELEDIT 
 // notification message. 
 // Returns TRUE if the label is changed, or FALSE otherwise. 

BOOL OnEndLabelEdit(NMLVDISPINFO* plvdi)
{ 
    if (plvdi->item.iItem == -1) 
        return FALSE; 
 
    // Copy the new label text to the application-defined structure. 
//    lstrcpyn(rgPetInfo[plvdi->item.iItem].szKind, plvdi->item.pszText, 10);
    
    return TRUE;
    // To make a more robust application you should send an EM_LIMITTEXT
    // message to the edit control to prevent the user from entering too
    // many characters in the field. 
 } 


////////////////////////////////////////////////////////////////////////////////
static WNDPROC g_orgListWndProc;

static LRESULT CALLBACK ListWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	ChildWnd* child = (ChildWnd*)GetWindowLong(GetParent(hWnd), GWL_USERDATA);
	Pane* pane = (Pane*)GetWindowLong(hWnd, GWL_USERDATA);
	ASSERT(child);

	switch (message) {
/*
        case WM_CREATE:
            //CreateListView(hWnd);
            return 0;
 */
        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code) { 
            case LVN_GETDISPINFO: 
                OnGetDispInfo((NMLVDISPINFO*)lParam); 
                break; 
            case LVN_ENDLABELEDIT: 
                return OnEndLabelEdit((NMLVDISPINFO*)lParam);
                break;
            }
			break;
//            return 0;

		case WM_SETFOCUS:
			child->nFocusPanel = pane==&child->right? 1: 0;
			ListBox_SetSel(hWnd, TRUE, 1);
			//TODO: check menu items
			break;

		case WM_KEYDOWN:
			if (wParam == VK_TAB) {
				//TODO: SetFocus(Globals.hDriveBar)
				SetFocus(child->nFocusPanel? child->left.hWnd: child->right.hWnd);
			}
	}
	return CallWindowProc(g_orgListWndProc, hWnd, message, wParam, lParam);
}


void CreateListWnd(HWND parent, Pane* pane, int id, LPSTR lpszPathName)
{
//	static int s_init = 0;
	Entry* entry = pane->root;

	pane->treePane = 0;
#if 1
    pane->hWnd = CreateListView(parent, id);
#else
	pane->hWnd = CreateWindow(_T("ListBox"), _T(""), WS_CHILD|WS_VISIBLE|WS_HSCROLL|WS_VSCROLL|
								LBS_DISABLENOSCROLL|LBS_NOINTEGRALHEIGHT|LBS_OWNERDRAWFIXED|LBS_NOTIFY,
								0, 0, 0, 0, parent, (HMENU)id, Globals.hInstance, 0);
#endif
	SetWindowLong(pane->hWnd, GWL_USERDATA, (LPARAM)pane);
	g_orgListWndProc = SubclassWindow(pane->hWnd, ListWndProc);
	SendMessage(pane->hWnd, WM_SETFONT, (WPARAM)Globals.hFont, FALSE);

	 // insert entries into listbox
	if (entry)
		InsertListEntries(pane, entry, -1);

	 // calculate column widths
//	if (!s_init) {
//		s_init = 1;
//		init_output(pane->hWnd);
//	}
//	calc_widths(pane, TRUE);
}


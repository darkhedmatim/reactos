/*
 *  ReactOS winfile
 *
 *  main.c
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

#include "winfile.h"
#include "about.h"
#include "mdiclient.h"


////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//

WINFILE_GLOBALS Globals;

HINSTANCE hInst;                            // current instance

TCHAR szTitle[MAX_LOADSTRING];              // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];
TCHAR szFrameClass[MAX_LOADSTRING];
//TCHAR szChildClass[MAX_LOADSTRING];


////////////////////////////////////////////////////////////////////////////////
//
//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	char path[MAX_PATH];
    int nParts[3];
	ChildWnd* child;

	WNDCLASSEX wcFrame = {
		sizeof(WNDCLASSEX),
		0/*style*/,
		WndProc,
		0/*cbClsExtra*/,
		0/*cbWndExtra*/,
		hInstance,
		LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINFILE)),
		LoadCursor(0, IDC_ARROW),
		0/*hbrBackground*/,
		0/*lpszMenuName*/,
		szWindowClass,
		(HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_WINFILE), IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED)
	};
	ATOM hWndClass = RegisterClassEx(&wcFrame); // register frame window class

	WNDCLASS wcChild = {
		CS_CLASSDC|CS_DBLCLKS|CS_VREDRAW,
		ChildWndProc,
		0/*cbClsExtra*/,
		0/*cbWndExtra*/,
		hInstance,
		0/*hIcon*/,
		LoadCursor(0, IDC_ARROW),
		0/*hbrBackground*/,
		0/*lpszMenuName*/,
		szFrameClass
	};

	ATOM hChildClass = RegisterClass(&wcChild); // register child windows class

	HMENU hMenuFrame = LoadMenu(hInstance, MAKEINTRESOURCE(IDC_WINFILE));
	HMENU hMenuWindow = GetSubMenu(hMenuFrame, GetMenuItemCount(hMenuFrame)-2);

	CLIENTCREATESTRUCT ccs = {
		hMenuWindow, IDW_FIRST_CHILD
	};

	INITCOMMONCONTROLSEX icc = {
		sizeof(INITCOMMONCONTROLSEX),
		ICC_BAR_CLASSES
	};

//	TCHAR path[MAX_PATH];

	HDC hdc = GetDC(0);

//	hMenuFrame = hMenuFrame;
	Globals.hMenuView = GetSubMenu(hMenuFrame, 3);
	Globals.hMenuOptions = GetSubMenu(hMenuFrame, 4);
	Globals.hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINFILE));
	Globals.hFont = CreateFont(-MulDiv(8,GetDeviceCaps(hdc,LOGPIXELSY),72), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, _T("MS Sans Serif"));
	ReleaseDC(0, hdc);

	Globals.hMainWnd = CreateWindowEx(0, (LPCTSTR)(int)hWndClass, szTitle,
		            WS_OVERLAPPEDWINDOW,
					CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
					0/*hWndParent*/, hMenuFrame, hInstance, 0/*lpParam*/);
    if (!Globals.hMainWnd) {
        return FALSE;
    }

	Globals.hMDIClient = CreateWindowEx(0, _T("MDICLIENT"), NULL,
//	Globals.hMDIClient = CreateWindowEx(0, (LPCTSTR)(int)hChildClass, NULL,
					WS_CHILD|WS_CLIPCHILDREN|WS_VSCROLL|WS_HSCROLL|WS_VISIBLE|WS_BORDER,
					0, 0, 0, 0,
					Globals.hMainWnd, 0, hInstance, &ccs);
    if (!Globals.hMDIClient) {
        return FALSE;
    }

	InitCommonControlsEx(&icc);

	{
		TBBUTTON drivebarBtn = {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP};
		int btn = 1;
		PTSTR p;

		Globals.hDriveBar = CreateToolbarEx(Globals.hMainWnd, WS_CHILD|WS_VISIBLE|CCS_NOMOVEY|TBSTYLE_LIST,
					IDW_DRIVEBAR, 2, hInstance, IDB_DRIVEBAR, &drivebarBtn,
					1, 16, 13, 16, 13, sizeof(TBBUTTON));
//		CheckMenuItem(hMenuFrame, ID_OPTIONS_DRIVEBAR, MF_BYCOMMAND|MF_CHECKED);
		CheckMenuItem(Globals.hMenuOptions, ID_OPTIONS_DRIVEBAR, MF_BYCOMMAND|MF_CHECKED);
		GetLogicalDriveStrings(BUFFER_LEN, Globals.drives);
		drivebarBtn.fsStyle = TBSTYLE_BUTTON;
#ifndef _NO_EXTENSIONS
		 // register windows drive root strings
		SendMessage(Globals.hDriveBar, TB_ADDSTRING, 0, (LPARAM)Globals.drives);
#endif
		drivebarBtn.idCommand = ID_DRIVE_FIRST;
		for( p = Globals.drives; *p; ) {
#ifdef _NO_EXTENSIONS
			 // insert drive letter
			TCHAR b[3] = { tolower(*p) };
			SendMessage(Globals.hDriveBar, TB_ADDSTRING, 0, (LPARAM)b);
#endif
			switch(GetDriveType(p)) {
			case DRIVE_REMOVABLE:	drivebarBtn.iBitmap = 1;	break;
			case DRIVE_CDROM:		drivebarBtn.iBitmap = 3;	break;
			case DRIVE_REMOTE:		drivebarBtn.iBitmap = 4;	break;
			case DRIVE_RAMDISK:		drivebarBtn.iBitmap = 5;	break;
			default:/*DRIVE_FIXED*/	drivebarBtn.iBitmap = 2;
			}
			SendMessage(Globals.hDriveBar, TB_INSERTBUTTON, btn++, (LPARAM)&drivebarBtn);
			drivebarBtn.idCommand++;
			drivebarBtn.iString++;
			while(*p++);
		}
	}
	{
		TBBUTTON toolbarBtns[] = {
			{0, 0, 0, TBSTYLE_SEP},
			{0, ID_WINDOW_NEW_WINDOW, TBSTATE_ENABLED, TBSTYLE_BUTTON},
			{1, ID_WINDOW_CASCADE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
			{2, ID_WINDOW_TILE_HORZ, TBSTATE_ENABLED, TBSTYLE_BUTTON},
			{3, ID_WINDOW_TILE_VERT, TBSTATE_ENABLED, TBSTYLE_BUTTON},
			{4, 2/*TODO: ID_...*/, TBSTATE_ENABLED, TBSTYLE_BUTTON},
			{5, 2/*TODO: ID_...*/, TBSTATE_ENABLED, TBSTYLE_BUTTON},
		};

		Globals.hToolBar = CreateToolbarEx(Globals.hMainWnd, WS_CHILD|WS_VISIBLE,
			IDW_TOOLBAR, 2, hInstance, IDB_TOOLBAR, toolbarBtns,
			sizeof(toolbarBtns)/sizeof(TBBUTTON), 16, 15, 16, 15, sizeof(TBBUTTON));
		CheckMenuItem(Globals.hMenuOptions, ID_OPTIONS_TOOLBAR, MF_BYCOMMAND|MF_CHECKED);
	}

    // Create the status bar
    Globals.hStatusBar = CreateStatusWindow(WS_VISIBLE|WS_CHILD|WS_CLIPSIBLINGS|SBT_NOBORDERS, 
                                    "", Globals.hMainWnd, STATUS_WINDOW);
    if (!Globals.hStatusBar)
        return FALSE;
	CheckMenuItem(Globals.hMenuOptions, ID_OPTIONS_STATUSBAR, MF_BYCOMMAND|MF_CHECKED);

    // Create the status bar panes
    nParts[0] = 100;
    nParts[1] = 210;
    nParts[2] = 400;
    SendMessage(Globals.hStatusBar, SB_SETPARTS, 3, (long)nParts);

#if 1
	//Globals.hstatusbar = CreateStatusWindow(WS_CHILD|WS_VISIBLE, 0, Globals.Globals.hMainWnd, IDW_STATUSBAR);
	//CheckMenuItem(Globals.Globals.hMenuOptions, ID_OPTIONS_STATUSBAR, MF_BYCOMMAND|MF_CHECKED);

/* CreateStatusWindow does not accept WS_BORDER
/*
	Globals.hstatusbar = CreateWindowEx(WS_EX_NOPARENTNOTIFY, STATUSCLASSNAME, 0,
					WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_BORDER|CCS_NODIVIDER, 0,0,0,0,
					Globals.Globals.hMainWnd, (HMENU)IDW_STATUSBAR, hinstance, 0);*/

	 //TODO: read paths and window placements from registry

	GetCurrentDirectory(MAX_PATH, path);
	child = alloc_child_window(path);

	child->pos.showCmd = SW_SHOWMAXIMIZED;
	child->pos.rcNormalPosition.left = 0;
	child->pos.rcNormalPosition.top = 0;
	child->pos.rcNormalPosition.right = 320;
	child->pos.rcNormalPosition.bottom = 280;

	if (!create_child_window(child))
		free(child);

	SetWindowPlacement(child->hwnd, &child->pos);
	Globals.himl = ImageList_LoadBitmap(Globals.hInstance, MAKEINTRESOURCE(IDB_IMAGES), 16, 0, RGB(0,255,0));
	Globals.prescan_node = FALSE;
#endif
        
/*
    hSplitWnd = CreateWindow(szFrameClass, "splitter window", WS_VISIBLE|WS_CHILD,
                            CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 
                            Globals.hMainWnd, (HMENU)SPLIT_WINDOW, hInstance, NULL);
    if (!hSplitWnd)
        return FALSE;
    hTreeWnd = CreateTreeView(Globals.hMDIClient, "c:\\foobar.txt");
    if (!hTreeWnd)
        return FALSE;

    hListWnd = CreateListView(Globals.hMDIClient, "");
    if (!hListWnd)
        return FALSE;
 */
    ShowWindow(Globals.hMainWnd, nCmdShow);
    UpdateWindow(Globals.hMainWnd);
    return TRUE;
}




#ifdef _NO_EXTENSIONS

static int g_foundPrevInstance = 0;

// search for already running win[e]files
static BOOL CALLBACK EnumWndProc(HWND hwnd, LPARAM lparam)
{
	TCHAR cls[128];

	GetClassName(hwnd, cls, 128);
	if (!lstrcmp(cls, (LPCTSTR)lparam)) {
		g_foundPrevInstance++;
		return FALSE;
	}
	return TRUE;
}

#endif

void ExitInstance()
{
    if (Globals.himl)
	    ImageList_Destroy(Globals.himl);
}


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    MSG msg;
    HACCEL hAccel;

    hInst = hInstance; // Store instance handle in our global variable

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_WINFILE, szWindowClass, MAX_LOADSTRING);
    LoadString(hInstance, IDC_WINFILE_FRAME, szFrameClass, MAX_LOADSTRING);
    
#ifdef _NO_EXTENSIONS
	// allow only one running instance
	EnumWindows(EnumWndProc, (LPARAM)szWindowClass);
	if (g_foundPrevInstance)
		return 1;
#endif

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }

    hAccel = LoadAccelerators(hInstance, (LPCTSTR)IDC_WINFILE);

    // Main message loop:
#if 0
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
#else
	while (GetMessage(&msg, (HWND)NULL, 0, 0)) { 
        if (!TranslateMDISysAccel(Globals.hMDIClient, &msg) && 
            !TranslateAccelerator(Globals.hMainWnd/*hwndFrame*/, hAccel, &msg)) { 
            TranslateMessage(&msg); 
            DispatchMessage(&msg); 
		} 
	} 
#endif
	ExitInstance();
    return msg.wParam;
}



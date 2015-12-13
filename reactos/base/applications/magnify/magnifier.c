/*
 * PROJECT:     ReactOS Magnify
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/magnify/magnifier.c
 * PURPOSE:     Magnification of parts of the screen.
 * COPYRIGHT:   Copyright 2007 Marc Piulachs <marc.piulachs@codexchange.net>
 *
 */

/* TODO: AppBar */
#include "magnifier.h"

#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <winnls.h>

#include "resource.h"

const TCHAR szWindowClass[] = TEXT("MAGNIFIER");

#define MAX_LOADSTRING 100

/* Global Variables */
HINSTANCE hInst;
HWND hMainWnd;

TCHAR szTitle[MAX_LOADSTRING];

#define TIMER_SPEED   1
#define REPAINT_SPEED   100

DWORD lastTicks = 0;

HWND hDesktopWindow = NULL;

/* Current magnified area */
POINT cp;

/* Last positions */
POINT pMouse;
POINT pCaret;
POINT pFocus;

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    AboutProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    OptionsProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    WarningProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    HACCEL hAccelTable;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    switch (GetUserDefaultUILanguage())
    {
        case MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT):
          SetProcessDefaultLayout(LAYOUT_RTL);
          break;

        default:
          break;
    }

    /* Initialize global strings */
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    /* Perform application initialization */
    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MAGNIFIER));

    /* Main message loop */
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = WndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = hInstance;
    wc.hIcon            = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName     = MAKEINTRESOURCE(IDC_MAGNIFIER);
    wc.lpszClassName    = szWindowClass;

    return RegisterClass(&wc);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    RECT rcWorkArea;
    hInst = hInstance; // Store instance handle in our global variable

    SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0);

    /* Create the Window */
    hMainWnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_PALETTEWINDOW,
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        (rcWorkArea.right - rcWorkArea.left) * 2 / 3,
        200,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (!hMainWnd)
        return FALSE;

    ShowWindow(hMainWnd, bStartMinimized ? SW_MINIMIZE : nCmdShow);
    UpdateWindow(hMainWnd);

    if (bShowWarning)
        DialogBox(hInstance, MAKEINTRESOURCE(IDD_WARNINGDIALOG), hMainWnd, WarningProc);

    return TRUE;
}

void Refresh()
{
    if (!IsIconic(hMainWnd))
    {
        /* Invalidate the client area forcing a WM_PAINT message */
        InvalidateRgn(hMainWnd, NULL, TRUE);
    }
}

void GetBestOverlapWithMonitors(LPRECT rect)
{
    int rcLeft, rcTop;
    int rcWidth, rcHeight;
    RECT rcMon;
    HMONITOR hMon = MonitorFromRect(rect, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info;
    info.cbSize = sizeof(info);

    GetMonitorInfo(hMon, &info);

    rcMon = info.rcMonitor;

    rcLeft = rect->left;
    rcTop = rect->top;
    rcWidth  = (rect->right - rect->left);
    rcHeight = (rect->bottom - rect->top);

    if (rcLeft < rcMon.left)
       rcLeft = rcMon.left;

    if (rcTop < rcMon.top)
       rcTop = rcMon.top;

    if (rcLeft > (rcMon.right - rcWidth))
        rcLeft = (rcMon.right - rcWidth);

    if (rcTop > (rcMon.bottom - rcHeight))
        rcTop = (rcMon.bottom - rcHeight);

    OffsetRect(rect, (rcLeft-rect->left), (rcTop-rect->top));
}

void Draw(HDC aDc)
{
    HDC desktopHdc = NULL;
    HDC HdcStretch;
    HANDLE hOld;
    HBITMAP HbmpStrech;

    RECT sourceRect, intersectedRect;
    RECT targetRect, appRect;
    DWORD rop = SRCCOPY;
    CURSORINFO cinfo;
    ICONINFO iinfo;

    int AppWidth, AppHeight;
    LONG blitAreaWidth, blitAreaHeight;

    if (bInvertColors)
        rop = NOTSRCCOPY;

    desktopHdc = GetDC(0);

    GetClientRect(hMainWnd, &appRect);
    GetWindowRect(hDesktopWindow, &sourceRect);

    ZeroMemory(&cinfo, sizeof(cinfo));
    ZeroMemory(&iinfo, sizeof(iinfo));
    cinfo.cbSize = sizeof(cinfo);
    GetCursorInfo(&cinfo);
    GetIconInfo(cinfo.hCursor, &iinfo);

    targetRect = appRect;
    ClientToScreen(hMainWnd, (POINT*)&targetRect.left);
    ClientToScreen(hMainWnd, (POINT*)&targetRect.right);

    AppWidth  = (appRect.right - appRect.left);
    AppHeight = (appRect.bottom - appRect.top);

    blitAreaWidth  = AppWidth / iZoom;
    blitAreaHeight = AppHeight / iZoom;

    sourceRect.left = (cp.x) - (blitAreaWidth /2);
    sourceRect.top = (cp.y) - (blitAreaHeight /2);
    sourceRect.right = sourceRect.left + blitAreaWidth;
    sourceRect.bottom = sourceRect.top + blitAreaHeight;

    GetBestOverlapWithMonitors(&sourceRect);

     /* Create a memory DC compatible with client area DC */
    HdcStretch = CreateCompatibleDC(desktopHdc);

    /* Create a bitmap compatible with the client area DC */
    HbmpStrech = CreateCompatibleBitmap(
        desktopHdc,
        blitAreaWidth,
        blitAreaHeight);

    /* Select our bitmap in memory DC and save the old one */
    hOld = SelectObject(HdcStretch , HbmpStrech);

    /* Paint the screen bitmap to our in memory DC */
    BitBlt(
        HdcStretch,
        0,
        0,
        blitAreaWidth,
        blitAreaHeight,
        desktopHdc,
        sourceRect.left,
        sourceRect.top,
        rop);

    if (IntersectRect(&intersectedRect, &sourceRect, &targetRect))
    {
        OffsetRect(&intersectedRect, -sourceRect.left, -sourceRect.top);
        FillRect(HdcStretch, &intersectedRect, GetStockObject(DC_BRUSH));
    }

    /* Draw the mouse pointer in the right position */
    DrawIcon(
        HdcStretch ,
        pMouse.x - iinfo.xHotspot - sourceRect.left, // - 10,
        pMouse.y - iinfo.yHotspot - sourceRect.top, // - 10,
        cinfo.hCursor);

    /* Blast the stretched image from memory DC to window DC */
    StretchBlt(
        aDc,
        0,
        0,
        AppWidth,
        AppHeight,
        HdcStretch,
        0,
        0,
        blitAreaWidth,
        blitAreaHeight,
        SRCCOPY | NOMIRRORBITMAP);

    /* Cleanup */
    if (iinfo.hbmMask)
        DeleteObject(iinfo.hbmMask);
    if (iinfo.hbmColor)
        DeleteObject(iinfo.hbmColor);
    SelectObject(HdcStretch, hOld);
    DeleteObject (HbmpStrech);
    DeleteDC(HdcStretch);
    ReleaseDC(hDesktopWindow, desktopHdc);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId;

    switch (message)
    {
        case WM_TIMER:
        {
            BOOL hasMoved = FALSE;

            if (bFollowMouse)
            {
                POINT pNewMouse;

                //Get current mouse position
                GetCursorPos (&pNewMouse);

                //If mouse has moved ...
                if (((pMouse.x != pNewMouse.x) || (pMouse.y != pNewMouse.y)))
                {
                    //Update to new position
                    pMouse = pNewMouse;
                    cp = pNewMouse;
                    hasMoved = TRUE;
                }
            }
            
            if (bFollowCaret && !hasMoved)
            {
                POINT pNewCaret;
                HWND hwnd2;

                //Get caret position
                HWND hwnd1 = GetForegroundWindow ();
                DWORD a = GetWindowThreadProcessId(hwnd1, NULL);
                DWORD b = GetCurrentThreadId();
                AttachThreadInput (a, b, TRUE);
                hwnd2 = GetFocus();

                GetCaretPos( &pNewCaret);
                ClientToScreen (hwnd2, (LPPOINT) &pNewCaret);
                AttachThreadInput (a, b, FALSE);

                if (((pCaret.x != pNewCaret.x) || (pCaret.y != pNewCaret.y)))
                {
                    //Update to new position
                    pCaret = pNewCaret;
                    cp = pNewCaret;
                    hasMoved = TRUE;
                }
            }

            if (bFollowFocus && !hasMoved)
            {
                POINT pNewFocus;
                RECT controlRect;

                //Get current control focus
                HWND hwnd3 = GetFocus ();
                GetWindowRect (hwnd3 , &controlRect);
                pNewFocus.x = controlRect.left;
                pNewFocus.y = controlRect.top;

                if(((pFocus.x != pNewFocus.x) || (pFocus.y != pNewFocus.y)))
                {
                    //Update to new position
                    pFocus = pNewFocus;
                    cp = pNewFocus;
                    hasMoved = TRUE;
                }
            }

            if(!hasMoved)
            {
                DWORD newTicks = GetTickCount();
                DWORD elapsed = (newTicks - lastTicks);
                if(elapsed > REPAINT_SPEED)
                {
                    hasMoved = TRUE;
                }
            }

            if(hasMoved)
            {
                lastTicks = GetTickCount();
                Refresh();
            }
        }
        break;

        case WM_COMMAND:
        {
            wmId = LOWORD(wParam);
            /* Parse the menu selections */
            switch (wmId)
            {
                case IDM_OPTIONS:
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOGOPTIONS), hWnd, OptionsProc);
                    break;
                case IDM_ABOUT:
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, AboutProc);
                    break;
                case IDM_EXIT:
                    DestroyWindow(hWnd);
                    break;
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT PaintStruct;
            HDC dc;
            dc = BeginPaint(hWnd, &PaintStruct);
            Draw(dc);
            EndPaint(hWnd, &PaintStruct);
            break;
        }

        case WM_ERASEBKGND:
            // handle WM_ERASEBKGND by simply returning non-zero because we did all the drawing in WM_PAINT.
            break;

        case WM_DESTROY:
            /* Save settings to registry */
            SaveSettings();
            KillTimer(hWnd , 1);
            PostQuitMessage(0);
            break;

        case WM_CREATE:
            /* Load settings from registry */
            LoadSettings();

            /* Get the desktop window */
            hDesktopWindow = GetDesktopWindow();

            /* Set the timer */
            SetTimer (hWnd , 1, TIMER_SPEED , NULL);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

INT_PTR CALLBACK AboutProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
        case WM_INITDIALOG:
            return (INT_PTR)TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;
    }

    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK OptionsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
        case WM_INITDIALOG:
        {
            // Add the zoom items...
            SendDlgItemMessage(hDlg, IDC_ZOOM, CB_ADDSTRING, 0, (LPARAM)("1"));
            SendDlgItemMessage(hDlg, IDC_ZOOM, CB_ADDSTRING, 0, (LPARAM)("2"));
            SendDlgItemMessage(hDlg, IDC_ZOOM, CB_ADDSTRING, 0, (LPARAM)("3"));
            SendDlgItemMessage(hDlg, IDC_ZOOM, CB_ADDSTRING, 0, (LPARAM)("4"));
            SendDlgItemMessage(hDlg, IDC_ZOOM, CB_ADDSTRING, 0, (LPARAM)("5"));
            SendDlgItemMessage(hDlg, IDC_ZOOM, CB_ADDSTRING, 0, (LPARAM)("6"));
            SendDlgItemMessage(hDlg, IDC_ZOOM, CB_ADDSTRING, 0, (LPARAM)("7"));
            SendDlgItemMessage(hDlg, IDC_ZOOM, CB_ADDSTRING, 0, (LPARAM)("8"));

            SendDlgItemMessage(hDlg, IDC_ZOOM, CB_SETCURSEL, iZoom - 1, 0);

            if (bFollowMouse)
                SendDlgItemMessage(hDlg,IDC_FOLLOWMOUSECHECK,BM_SETCHECK , wParam ,0);

            if (bFollowFocus)
                SendDlgItemMessage(hDlg,IDC_FOLLOWKEYBOARDCHECK,BM_SETCHECK , wParam ,0);

            if (bFollowCaret)
                SendDlgItemMessage(hDlg,IDC_FOLLOWTEXTEDITINGCHECK,BM_SETCHECK , wParam ,0);

            if (bInvertColors)
                SendDlgItemMessage(hDlg,IDC_INVERTCOLORSCHECK,BM_SETCHECK , wParam ,0);

            if (bStartMinimized)
                SendDlgItemMessage(hDlg,IDC_STARTMINIMIZEDCHECK,BM_SETCHECK , wParam ,0);

            if (bShowMagnifier)
                SendDlgItemMessage(hDlg,IDC_SHOWMAGNIFIERCHECK,BM_SETCHECK , wParam ,0);

            return (INT_PTR)TRUE;
        }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
            case IDOK:
            case IDCANCEL:
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;

            case IDC_BUTTON_HELP:
                /* Unimplemented */
                MessageBox(hDlg , TEXT("Magnifier help not available yet!") , TEXT("Help") , MB_OK);
                break;
            case IDC_ZOOM:
                if (HIWORD(wParam) == CBN_SELCHANGE)
                {
                    HWND hCombo = GetDlgItem(hDlg,IDC_ZOOM);

                    /* Get index of current selection and the text of that selection */
                    iZoom = SendMessage( hCombo, CB_GETCURSEL, (WPARAM) wParam, (LPARAM) lParam ) + 1;

                    /* Update the magnifier UI */
                    Refresh();
                }
                break;
            case IDC_INVERTCOLORSCHECK:
                bInvertColors = IsDlgButtonChecked(hDlg, IDC_INVERTCOLORSCHECK);
                Refresh();
                break;
            case IDC_FOLLOWMOUSECHECK:
                bFollowMouse = IsDlgButtonChecked(hDlg, IDC_FOLLOWMOUSECHECK);
                break;
            case IDC_FOLLOWKEYBOARDCHECK:
                bFollowFocus = IsDlgButtonChecked(hDlg, IDC_FOLLOWKEYBOARDCHECK);
                break;
            case IDC_FOLLOWTEXTEDITINGCHECK:
                bFollowCaret = IsDlgButtonChecked(hDlg, IDC_FOLLOWTEXTEDITINGCHECK);
                break;
            case IDC_STARTMINIMIZEDCHECK:
                bStartMinimized = IsDlgButtonChecked(hDlg, IDC_STARTMINIMIZEDCHECK);
                break;
            case IDC_SHOWMAGNIFIER:
                bShowMagnifier = IsDlgButtonChecked(hDlg, IDC_SHOWMAGNIFIERCHECK);
                if (bShowMagnifier)
                    ShowWindow (hMainWnd , SW_SHOW);
                else
                    ShowWindow (hMainWnd , SW_HIDE);
                break;
        }
    }

    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK WarningProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
            return (INT_PTR)TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_SHOWWARNINGCHECK:
                    bShowWarning = !IsDlgButtonChecked(hDlg, IDC_SHOWWARNINGCHECK);
                    break;
            }
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;
    }

    return (INT_PTR)FALSE;
}

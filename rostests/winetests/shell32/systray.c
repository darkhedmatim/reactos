/* Unit tests for systray
 *
 * Copyright 2007 Mikolaj Zalewski
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#define _WIN32_IE 0x600
#include <assert.h>
#include <stdarg.h>

#include <windows.h>

#include "wine/test.h"


static HWND hMainWnd;
static BOOL (WINAPI *pShell_NotifyIconW)(DWORD,PNOTIFYICONDATAW);

static void test_cbsize(void)
{
    NOTIFYICONDATAA nidA;
    BOOL ret;

    if (pShell_NotifyIconW)
    {
        NOTIFYICONDATAW nidW;

        ZeroMemory(&nidW, sizeof(nidW));
        nidW.cbSize = NOTIFYICONDATAW_V1_SIZE;
        nidW.hWnd = hMainWnd;
        nidW.uID = 1;
        nidW.uFlags = NIF_ICON|NIF_MESSAGE;
        nidW.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        nidW.uCallbackMessage = WM_USER+17;
        ret = pShell_NotifyIconW(NIM_ADD, &nidW);
        if (ret)
        {
            /* using an invalid cbSize does work */
            nidW.cbSize = 3;
            nidW.hWnd = hMainWnd;
            nidW.uID = 1;
            ret = pShell_NotifyIconW(NIM_DELETE, &nidW);
            ok( ret || broken(!ret), /* nt4 */ "NIM_DELETE failed!\n");
            /* as icon doesn't exist anymore - now there will be an error */
            nidW.cbSize = sizeof(nidW);
            ok(!pShell_NotifyIconW(NIM_DELETE, &nidW) != !ret, "The icon was not deleted\n");
        }
        else win_skip( "Shell_NotifyIconW not working\n" );  /* win9x */
    }

    /* same for Shell_NotifyIconA */
    ZeroMemory(&nidA, sizeof(nidA));
    nidA.cbSize = NOTIFYICONDATAA_V1_SIZE;
    nidA.hWnd = hMainWnd;
    nidA.uID = 1;
    nidA.uFlags = NIF_ICON|NIF_MESSAGE;
    nidA.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    nidA.uCallbackMessage = WM_USER+17;
    ok(Shell_NotifyIconA(NIM_ADD, &nidA), "NIM_ADD failed!\n");

    /* using an invalid cbSize does work */
    nidA.cbSize = 3;
    nidA.hWnd = hMainWnd;
    nidA.uID = 1;
    ret = Shell_NotifyIconA(NIM_DELETE, &nidA);
    ok( ret || broken(!ret),  /* win9x */ "NIM_DELETE failed!\n");
    /* as icon doesn't exist anymore - now there will be an error */
    nidA.cbSize = sizeof(nidA);
    ok(!Shell_NotifyIconA(NIM_DELETE, &nidA) != !ret, "The icon was not deleted\n");
}

START_TEST(systray)
{
    WNDCLASSA wc;
    MSG msg;
    RECT rc;
    HMODULE hshell32;

    hshell32 = GetModuleHandleA("shell32.dll");
    pShell_NotifyIconW = (void*)GetProcAddress(hshell32, "Shell_NotifyIconW");

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hIcon = NULL;
    wc.hCursor = LoadCursorA(NULL, IDC_IBEAM);
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "MyTestWnd";
    wc.lpfnWndProc = DefWindowProc;
    RegisterClassA(&wc);

    hMainWnd = CreateWindowExA(0, "MyTestWnd", "Blah", WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 680, 260, NULL, NULL, GetModuleHandleA(NULL), 0);
    GetClientRect(hMainWnd, &rc);
    ShowWindow(hMainWnd, SW_SHOW);

    test_cbsize();

    PostQuitMessage(0);
    while(GetMessageA(&msg,0,0,0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    DestroyWindow(hMainWnd);
}

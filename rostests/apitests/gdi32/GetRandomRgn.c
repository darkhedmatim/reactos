/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for ...
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>

#define CLIPRGN 1
#define METARGN 2
#define APIRGN  3
#define SYSRGN  4
#define RGN5    5

#define ok_int(x, exp) \
    ok((x) == (exp), "Failed test in line %d: value %s expected 0x%x, got 0x%x\n", \
       (int)__LINE__, #x, (int)(exp), (int)(x))

#define ok_long(x, exp) \
    ok((x) == (exp), "Failed test in line %d: value %s expected 0x%x, got 0x%x\n", \
       (int)__LINE__, #x, (int)(exp), (int)(x))

HWND ghwnd;
HDC ghdcWindow;

void Test_GetRandomRgn_Params()
{
    HDC hdc;
    HRGN hrgn;
    INT ret;

    hdc = CreateCompatibleDC(0);
    if (!hdc)
    {
        printf("Coun't create a dc\n");
        return;
    }

    hrgn = CreateRectRgn(11, 17, 23, 42);
    if (!hrgn)
    {
        printf("Coun't create a region\n");
        return;
    }

    SetLastError(0xbadbad00);
    ret = GetRandomRgn(NULL, NULL, 0);
    ok_int(ret, -1);
    ok_long(GetLastError(), 0xbadbad00);

    SetLastError(0xbadbad00);
    ret = GetRandomRgn(NULL, NULL, CLIPRGN);
    ok_int(ret, -1);
    ok_long(GetLastError(), 0xbadbad00);

    SetLastError(0xbadbad00);
    ret = GetRandomRgn(NULL, hrgn, 0);
    ok_int(ret, -1);
    ok_long(GetLastError(), 0xbadbad00);

    SetLastError(0xbadbad00);
    ret = GetRandomRgn(NULL, hrgn, CLIPRGN);
    ok_int(ret, -1);
    ok_long(GetLastError(), 0xbadbad00);

    SetLastError(0xbadbad00);
    ret = GetRandomRgn(hdc, NULL, 0);
    ok_int(ret, 0);
    ok_long(GetLastError(), 0xbadbad00);

    SetLastError(0xbadbad00);
    ret = GetRandomRgn(hdc, NULL, CLIPRGN);
    ok_int(ret, 0);
    ok_long(GetLastError(), 0xbadbad00);

    SetLastError(0xbadbad00);
    ret = GetRandomRgn(hdc, hrgn, 0);
    ok_int(ret, 0);
    ok_long(GetLastError(), 0xbadbad00);

    SetLastError(0xbadbad00);
    ret = GetRandomRgn(hdc, hrgn, 5);
    ok_int(ret, 1);
    ok_long(GetLastError(), 0xbadbad00);

    SetLastError(0xbadbad00);
    ret = GetRandomRgn(hdc, hrgn, 6);
    ok_int(ret, 0);
    ok_long(GetLastError(), 0xbadbad00);

    SetLastError(0xbadbad00);
    ret = GetRandomRgn(hdc, hrgn, 27);
    ok_int(ret, 0);
    ok_long(GetLastError(), 0xbadbad00);

    SetLastError(0xbadbad00);
    ret = GetRandomRgn(hdc, hrgn, -1);
    ok_int(ret, 0);
    ok_long(GetLastError(), 0xbadbad00);

    SetLastError(0xbadbad00);
    ret = GetRandomRgn(hdc, hrgn, CLIPRGN);
    ok_int(ret, 0);
    ok_long(GetLastError(), 0xbadbad00);

    SetLastError(0xbadbad00);
    ret = GetRandomRgn((HDC)0x123, hrgn, CLIPRGN);
    ok_int(ret, -1);
    ok_long(GetLastError(), 0xbadbad00);

    DeleteObject(hrgn);
    DeleteDC(hdc);
}

void Test_GetRandomRgn_CLIPRGN()
{
    HDC hdc;
    HRGN hrgn1, hrgn2;
    INT ret;
    RECT rect;

    hrgn1 = CreateRectRgn(11, 17, 23, 42);
    if (!hrgn1)
    {
        printf("Coun't create a region\n");
        return;
    }

    hdc = CreateCompatibleDC(0);
    if (!hdc)
    {
        printf("Coun't create a dc\n");
        return;
    }

    ret = GetRandomRgn(hdc, hrgn1, CLIPRGN);
    ok_int(ret, 0);
    GetRgnBox(hrgn1, &rect);
    ok_long(rect.left, 11);
    ok_long(rect.top, 17);
    ok_long(rect.right, 23);
    ok_long(rect.bottom, 42);

    hrgn2 = CreateRectRgn(1, 2, 3, 4);
    SelectClipRgn(hdc, hrgn2);
    DeleteObject(hrgn2);
    ret = GetRandomRgn(hdc, hrgn1, CLIPRGN);
    ok_int(ret, 1);
    GetRgnBox(hrgn1, &rect);
    ok_long(rect.left, 1);
    ok_long(rect.top, 2);
    ok_long(rect.right, 3);
    ok_long(rect.bottom, 4);

    hrgn2 = CreateRectRgn(2, 3, 4, 5);
    SelectClipRgn(ghdcWindow, hrgn2);
    DeleteObject(hrgn2);
    ret = GetRandomRgn(ghdcWindow, hrgn1, CLIPRGN);
    ok_int(ret, 1);
    GetRgnBox(hrgn1, &rect);
    ok_long(rect.left, 2);
    ok_long(rect.top, 3);
    ok_long(rect.right, 4);
    ok_long(rect.bottom, 5);

    MoveWindow(ghwnd, 200, 400, 100, 100, 0);

    ret = GetRandomRgn(ghdcWindow, hrgn1, CLIPRGN);
    ok_int(ret, 1);
    GetRgnBox(hrgn1, &rect);
    ok_long(rect.left, 2);
    ok_long(rect.top, 3);
    ok_long(rect.right, 4);
    ok_long(rect.bottom, 5);


    DeleteObject(hrgn1);
    DeleteDC(hdc);
}

void Test_GetRandomRgn_METARGN()
{
}

void Test_GetRandomRgn_APIRGN()
{
}

void Test_GetRandomRgn_SYSRGN()
{
    HDC hdc;
    HRGN hrgn1, hrgn2;
    INT ret;
    RECT rect, rect2;
    HBITMAP hbmp;

    hrgn1 = CreateRectRgn(11, 17, 23, 42);
    if (!hrgn1)
    {
        printf("Coun't create a region\n");
        return;
    }

    hdc = CreateCompatibleDC(0);
    if (!hdc)
    {
        printf("Coun't create a dc\n");
        return;
    }

    ret = GetRandomRgn(hdc, hrgn1, SYSRGN);
    ok_int(ret, 1);
    GetRgnBox(hrgn1, &rect);
    ok_long(rect.left, 0);
    ok_long(rect.top, 0);
    ok_long(rect.right, 1);
    ok_long(rect.bottom, 1);

    hrgn2 = CreateRectRgn(1, 2, 3, 4);
    SelectClipRgn(hdc, hrgn2);
    DeleteObject(hrgn2);
    ret = GetRandomRgn(hdc, hrgn1, SYSRGN);
    ok_int(ret, 1);
    GetRgnBox(hrgn1, &rect);
    ok_long(rect.left, 0);
    ok_long(rect.top, 0);
    ok_long(rect.right, 1);
    ok_long(rect.bottom, 1);

    hbmp = CreateCompatibleBitmap(hdc, 4, 7);
    SelectObject(hdc, hbmp);
    ret = GetRandomRgn(hdc, hrgn1, SYSRGN);
    ok_int(ret, 1);
    GetRgnBox(hrgn1, &rect);
    ok_long(rect.left, 0);
    ok_long(rect.top, 0);
    ok_long(rect.right, 4);
    ok_long(rect.bottom, 7);
    DeleteObject(hbmp);

    MoveWindow(ghwnd, 100, 100, 100, 100, 0);
    ret = GetRandomRgn(ghdcWindow, hrgn1, SYSRGN);
    ok_int(ret, 1);
    GetRgnBox(hrgn1, &rect);
    DPtoLP(ghdcWindow, (LPPOINT)&rect, 2);
    ok_long(rect.left, 104);
    ok_long(rect.top, 124);
    ok_long(rect.right, 209);
    ok_long(rect.bottom, 196);

    MoveWindow(ghwnd, 200, 400, 200, 200, 0);

    ret = GetRandomRgn(ghdcWindow, hrgn1, SYSRGN);
    ok_int(ret, 1);
    GetRgnBox(hrgn1, &rect2);
    DPtoLP(ghdcWindow, (LPPOINT)&rect2, 2);
    ok_long(rect2.left, rect.left + 100);
    ok_long(rect2.top, rect.top + 300);
    ok_long(rect2.right, rect.right + 200 - 13);
    ok_long(rect2.bottom, rect.bottom + 400);


    DeleteObject(hrgn1);
    DeleteDC(hdc);

}

void Test_GetRandomRgn_RGN5()
{
    HDC hdc;
    HRGN hrgn1, hrgn2;
    INT ret;
    RECT rect, rect2;
    HBITMAP hbmp;

    hrgn1 = CreateRectRgn(11, 17, 23, 42);
    if (!hrgn1)
    {
        printf("Coun't create a region\n");
        return;
    }

    hdc = CreateCompatibleDC(0);
    if (!hdc)
    {
        printf("Coun't create a dc\n");
        return;
    }

    ret = GetRandomRgn(hdc, hrgn1, RGN5);
    ok_int(ret, 1);
    GetRgnBox(hrgn1, &rect);
    ok_long(rect.left, 0);
    ok_long(rect.top, 0);
    ok_long(rect.right, 1);
    ok_long(rect.bottom, 1);

    hrgn2 = CreateRectRgn(1, 2, 3, 4);
    SelectClipRgn(hdc, hrgn2);
    DeleteObject(hrgn2);
    ret = GetRandomRgn(hdc, hrgn1, RGN5);
    ok_int(ret, 1);
    GetRgnBox(hrgn1, &rect);
    ok_long(rect.left, 0);
    ok_long(rect.top, 0);
    ok_long(rect.right, 1);
    ok_long(rect.bottom, 1);

    hbmp = CreateCompatibleBitmap(hdc, 4, 7);
    SelectObject(hdc, hbmp);
    ret = GetRandomRgn(hdc, hrgn1, SYSRGN);
    ok_int(ret, 1);
    GetRgnBox(hrgn1, &rect);
    ok_long(rect.left, 0);
    ok_long(rect.top, 0);
    ok_long(rect.right, 4);
    ok_long(rect.bottom, 7);
    DeleteObject(hbmp);

    MoveWindow(ghwnd, 100, 100, 100, 100, 0);
    ret = GetRandomRgn(ghdcWindow, hrgn1, RGN5);
    ok_int(ret, 1);
    GetRgnBox(hrgn1, &rect);
    DPtoLP(ghdcWindow, (LPPOINT)&rect, 2);
    ok_long(rect.left, 104);
    ok_long(rect.top, 124);
    ok_long(rect.right, 209);
    ok_long(rect.bottom, 196);

    MoveWindow(ghwnd, 200, 400, 200, 200, 0);

    ret = GetRandomRgn(ghdcWindow, hrgn1, RGN5);
    ok_int(ret, 1);
    GetRgnBox(hrgn1, &rect2);
    DPtoLP(ghdcWindow, (LPPOINT)&rect2, 2);
    ok_long(rect2.left, rect.left + 100);
    ok_long(rect2.top, rect.top + 300);
    ok_long(rect2.right, rect.right + 200 - 13);
    ok_long(rect2.bottom, rect.bottom + 400);


    DeleteObject(hrgn1);
    DeleteDC(hdc);
}

START_TEST(GetRandomRgn)
{

	/* Create a window */
	ghwnd = CreateWindowW(L"BUTTON", L"TestWindow", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	                      100, 100, 100, 100, NULL, NULL, 0, 0);
	ghdcWindow = GetDC(ghwnd);
    if (!ghdcWindow)
    {
        printf("No window dc\n");
        return;
    }

    Test_GetRandomRgn_Params();
    Test_GetRandomRgn_CLIPRGN();
    Test_GetRandomRgn_METARGN();
    Test_GetRandomRgn_APIRGN();
    Test_GetRandomRgn_SYSRGN();
    Test_GetRandomRgn_RGN5();

}


/* Unit test suite for edit control.
 *
 * Copyright 2004 Vitaliy Margolen
 * Copyright 2005 C. Scott Ananian
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

#include <assert.h>
#include <windows.h>
#include <commctrl.h>

#include "wine/test.h"

#ifndef ES_COMBO
#define ES_COMBO 0x200
#endif

#define ID_EDITTESTDBUTTON 0x123
#define ID_EDITTEST2 99
#define MAXLEN 200

struct edit_notify {
    int en_change, en_maxtext, en_update;
};

static struct edit_notify notifications;

static BOOL (WINAPI *pEndMenu) (void);
static BOOL (WINAPI *pGetMenuBarInfo)(HWND,LONG,LONG,PMENUBARINFO);

static void init_function_pointers(void)
{
    HMODULE hdll = GetModuleHandleA("user32");

    pEndMenu = (void*)GetProcAddress(hdll, "EndMenu");
    pGetMenuBarInfo = (void*)GetProcAddress(hdll, "GetMenuBarInfo");
}

static INT_PTR CALLBACK multi_edit_dialog_proc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    static int num_ok_commands = 0;
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            HWND hedit = GetDlgItem(hdlg, 1000);
            SetFocus(hedit);
            switch (lparam)
            {
                /* test cases related to bug 12319 */
                case 0:
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 1:
                    PostMessageA(hedit, WM_CHAR, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 2:
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hedit, WM_CHAR, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;

                /* test cases for pressing enter */
                case 3:
                    num_ok_commands = 0;
                    PostMessageA(hedit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 1);
                    break;

                default:
                    break;
            }
            break;
        }

        case WM_COMMAND:
            if (HIWORD(wparam) != BN_CLICKED)
                break;

            switch (LOWORD(wparam))
            {
                case IDOK:
                    num_ok_commands++;
                    break;

                default:
                    break;
            }
            break;

        case WM_USER:
        {
            HWND hfocus = GetFocus();
            HWND hedit = GetDlgItem(hdlg, 1000);
            HWND hedit2 = GetDlgItem(hdlg, 1001);
            HWND hedit3 = GetDlgItem(hdlg, 1002);

            if (wparam != 0xdeadbeef)
                break;

            switch (lparam)
            {
                case 0:
                    if (hfocus == hedit)
                        EndDialog(hdlg, 1111);
                    else if (hfocus == hedit2)
                        EndDialog(hdlg, 2222);
                    else if (hfocus == hedit3)
                        EndDialog(hdlg, 3333);
                    else
                        EndDialog(hdlg, 4444);
                    break;
                case 1:
                    if ((hfocus == hedit) && (num_ok_commands == 0))
                        EndDialog(hdlg, 11);
                    else
                        EndDialog(hdlg, 22);
                    break;
                default:
                    EndDialog(hdlg, 5555);
            }
            break;
        }

        case WM_CLOSE:
            EndDialog(hdlg, 333);
            break;

        default:
            break;
    }

    return FALSE;
}

static INT_PTR CALLBACK edit_dialog_proc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            HWND hedit = GetDlgItem(hdlg, 1000);
            SetFocus(hedit);
            switch (lparam)
            {
                /* from bug 11841 */
                case 0:
                    PostMessageA(hedit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
                    break;
                case 1:
                    PostMessageA(hedit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
                    break;
                case 2:
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 1);
                    break;

                /* more test cases for WM_CHAR */
                case 3:
                    PostMessageA(hedit, WM_CHAR, VK_ESCAPE, 0x10001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 4:
                    PostMessageA(hedit, WM_CHAR, VK_RETURN, 0x1c0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 5:
                    PostMessageA(hedit, WM_CHAR, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;

                /* more test cases for WM_KEYDOWN + WM_CHAR */
                case 6:
                    PostMessageA(hedit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
                    PostMessageA(hedit, WM_CHAR, VK_ESCAPE, 0x10001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 7:
                    PostMessageA(hedit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
                    PostMessageA(hedit, WM_CHAR, VK_RETURN, 0x1c0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 1);
                    break;
                case 8:
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hedit, WM_CHAR, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 1);
                    break;

                /* multiple tab tests */
                case 9:
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 2);
                    break;
                case 10:
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 2);
                    break;

                default:
                    break;
            }
            break;
        }

        case WM_COMMAND:
            if (HIWORD(wparam) != BN_CLICKED)
                break;

            switch (LOWORD(wparam))
            {
                case IDOK:
                    EndDialog(hdlg, 111);
                    break;

                case IDCANCEL:
                    EndDialog(hdlg, 222);
                    break;

                default:
                    break;
            }
            break;

        case WM_USER:
        {
            int len;
            HWND hok = GetDlgItem(hdlg, IDOK);
            HWND hcancel = GetDlgItem(hdlg, IDCANCEL);
            HWND hedit = GetDlgItem(hdlg, 1000);
            HWND hfocus = GetFocus();

            if (wparam != 0xdeadbeef)
                break;

            switch (lparam)
            {
                case 0:
                    len = SendMessageA(hedit, WM_GETTEXTLENGTH, 0, 0);
                    if (len == 0)
                        EndDialog(hdlg, 444);
                    else
                        EndDialog(hdlg, 555);
                    break;

                case 1:
                    len = SendMessageA(hedit, WM_GETTEXTLENGTH, 0, 0);
                    if ((hfocus == hok) && len == 0)
                        EndDialog(hdlg, 444);
                    else
                        EndDialog(hdlg, 555);
                    break;

                case 2:
                    if (hfocus == hok)
                        EndDialog(hdlg, 11);
                    else if (hfocus == hcancel)
                        EndDialog(hdlg, 22);
                    else if (hfocus == hedit)
                        EndDialog(hdlg, 33);
                    else
                        EndDialog(hdlg, 44);
                    break;

                default:
                    EndDialog(hdlg, 555);
            }
            break;
        }

        case WM_CLOSE:
            EndDialog(hdlg, 333);
            break;

        default:
            break;
    }

    return FALSE;
}

static INT_PTR CALLBACK edit_singleline_dialog_proc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            HWND hedit = GetDlgItem(hdlg, 1000);
            SetFocus(hedit);
            switch (lparam)
            {
                /* test cases for WM_KEYDOWN */
                case 0:
                    PostMessageA(hedit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
                    break;
                case 1:
                    PostMessageA(hedit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
                    break;
                case 2:
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 1);
                    break;

                /* test cases for WM_CHAR */
                case 3:
                    PostMessageA(hedit, WM_CHAR, VK_ESCAPE, 0x10001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 4:
                    PostMessageA(hedit, WM_CHAR, VK_RETURN, 0x1c0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 5:
                    PostMessageA(hedit, WM_CHAR, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;

                /* test cases for WM_KEYDOWN + WM_CHAR */
                case 6:
                    PostMessageA(hedit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
                    PostMessageA(hedit, WM_CHAR, VK_ESCAPE, 0x10001);
                    break;
                case 7:
                    PostMessageA(hedit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
                    PostMessageA(hedit, WM_CHAR, VK_RETURN, 0x1c0001);
                    break;
                case 8:
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hedit, WM_CHAR, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 1);
                    break;

                default:
                    break;
            }
            break;
        }

        case WM_COMMAND:
            if (HIWORD(wparam) != BN_CLICKED)
                break;

            switch (LOWORD(wparam))
            {
                case IDOK:
                    EndDialog(hdlg, 111);
                    break;

                case IDCANCEL:
                    EndDialog(hdlg, 222);
                    break;

                default:
                    break;
            }
            break;

        case WM_USER:
        {
            HWND hok = GetDlgItem(hdlg, IDOK);
            HWND hedit = GetDlgItem(hdlg, 1000);
            HWND hfocus = GetFocus();
            int len = SendMessageA(hedit, WM_GETTEXTLENGTH, 0, 0);

            if (wparam != 0xdeadbeef)
                break;

            switch (lparam)
            {
                case 0:
                    if ((hfocus == hedit) && len == 0)
                        EndDialog(hdlg, 444);
                    else
                        EndDialog(hdlg, 555);
                    break;

                case 1:
                    if ((hfocus == hok) && len == 0)
                        EndDialog(hdlg, 444);
                    else
                        EndDialog(hdlg, 555);
                    break;

                default:
                    EndDialog(hdlg, 55);
            }
            break;
        }

        case WM_CLOSE:
            EndDialog(hdlg, 333);
            break;

        default:
            break;
    }

    return FALSE;
}

static INT_PTR CALLBACK edit_wantreturn_dialog_proc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            HWND hedit = GetDlgItem(hdlg, 1000);
            SetFocus(hedit);
            switch (lparam)
            {
                /* test cases for WM_KEYDOWN */
                case 0:
                    PostMessageA(hedit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
                    break;
                case 1:
                    PostMessageA(hedit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 2:
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 1);
                    break;

                /* test cases for WM_CHAR */
                case 3:
                    PostMessageA(hedit, WM_CHAR, VK_ESCAPE, 0x10001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 4:
                    PostMessageA(hedit, WM_CHAR, VK_RETURN, 0x1c0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 2);
                    break;
                case 5:
                    PostMessageA(hedit, WM_CHAR, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;

                /* test cases for WM_KEYDOWN + WM_CHAR */
                case 6:
                    PostMessageA(hedit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
                    PostMessageA(hedit, WM_CHAR, VK_ESCAPE, 0x10001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 0);
                    break;
                case 7:
                    PostMessageA(hedit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
                    PostMessageA(hedit, WM_CHAR, VK_RETURN, 0x1c0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 2);
                    break;
                case 8:
                    PostMessageA(hedit, WM_KEYDOWN, VK_TAB, 0xf0001);
                    PostMessageA(hedit, WM_CHAR, VK_TAB, 0xf0001);
                    PostMessageA(hdlg, WM_USER, 0xdeadbeef, 1);
                    break;

                default:
                    break;
            }
            break;
        }

        case WM_COMMAND:
            if (HIWORD(wparam) != BN_CLICKED)
                break;

            switch (LOWORD(wparam))
            {
                case IDOK:
                    EndDialog(hdlg, 111);
                    break;

                case IDCANCEL:
                    EndDialog(hdlg, 222);
                    break;

                default:
                    break;
            }
            break;

        case WM_USER:
        {
            HWND hok = GetDlgItem(hdlg, IDOK);
            HWND hedit = GetDlgItem(hdlg, 1000);
            HWND hfocus = GetFocus();
            int len = SendMessageA(hedit, WM_GETTEXTLENGTH, 0, 0);

            if (wparam != 0xdeadbeef)
                break;

            switch (lparam)
            {
                case 0:
                    if ((hfocus == hedit) && len == 0)
                        EndDialog(hdlg, 444);
                    else
                        EndDialog(hdlg, 555);
                    break;

                case 1:
                    if ((hfocus == hok) && len == 0)
                        EndDialog(hdlg, 444);
                    else
                        EndDialog(hdlg, 555);
                    break;

                case 2:
                    if ((hfocus == hedit) && len == 2)
                        EndDialog(hdlg, 444);
                    else
                        EndDialog(hdlg, 555);
                    break;

                default:
                    EndDialog(hdlg, 55);
            }
            break;
        }

        case WM_CLOSE:
            EndDialog(hdlg, 333);
            break;

        default:
            break;
    }

    return FALSE;
}

static HINSTANCE hinst;
static HWND hwndET2;
static const char szEditTest2Class[] = "EditTest2Class";
static const char szEditTest3Class[] = "EditTest3Class";
static const char szEditTest4Class[] = "EditTest4Class";
static const char szEditTextPositionClass[] = "EditTextPositionWindowClass";

static HWND create_editcontrol (DWORD style, DWORD exstyle)
{
    HWND handle;

    handle = CreateWindowExA(exstyle,
			  "EDIT",
			  "Test Text",
			  style,
			  10, 10, 300, 300,
			  NULL, NULL, hinst, NULL);
    ok (handle != NULL, "CreateWindow EDIT Control failed\n");
    assert (handle);
    if (winetest_interactive)
	ShowWindow (handle, SW_SHOW);
    return handle;
}

static HWND create_child_editcontrol (DWORD style, DWORD exstyle)
{
    HWND parentWnd;
    HWND editWnd;
    RECT rect;
    BOOL b;
    
    rect.left = 0;
    rect.top = 0;
    rect.right = 300;
    rect.bottom = 300;
    b = AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    ok(b, "AdjustWindowRect failed\n");
    
    parentWnd = CreateWindowExA(0,
                            szEditTextPositionClass,
                            "Edit Test",
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            rect.right - rect.left, rect.bottom - rect.top,
                            NULL, NULL, hinst, NULL);
    ok (parentWnd != NULL, "CreateWindow EDIT Test failed\n");
    assert(parentWnd);

    editWnd = CreateWindowExA(exstyle,
                            "EDIT",
                            "Test Text",
                            WS_CHILD | style,
                            0, 0, 300, 300,
                            parentWnd, NULL, hinst, NULL);
    ok (editWnd != NULL, "CreateWindow EDIT Test Text failed\n");
    assert(editWnd);
    if (winetest_interactive)
        ShowWindow (parentWnd, SW_SHOW);
    return editWnd;
}

static void destroy_child_editcontrol (HWND hwndEdit)
{
    if (GetParent(hwndEdit))
        DestroyWindow(GetParent(hwndEdit));
    else {
        trace("Edit control has no parent!\n");
        DestroyWindow(hwndEdit);
    }
}

static LONG get_edit_style (HWND hwnd)
{
    return GetWindowLongA( hwnd, GWL_STYLE ) & (
	ES_LEFT |
/* FIXME: not implemented
	ES_CENTER |
	ES_RIGHT |
	ES_OEMCONVERT |
*/
	ES_MULTILINE |
	ES_UPPERCASE |
	ES_LOWERCASE |
	ES_PASSWORD |
	ES_AUTOVSCROLL |
	ES_AUTOHSCROLL |
	ES_NOHIDESEL |
	ES_COMBO |
	ES_READONLY |
	ES_WANTRETURN |
	ES_NUMBER
	);
}

static void set_client_height(HWND Wnd, unsigned Height)
{
    RECT ClientRect, WindowRect;

    GetWindowRect(Wnd, &WindowRect);
    GetClientRect(Wnd, &ClientRect);
    SetWindowPos(Wnd, NULL, 0, 0,
                 WindowRect.right - WindowRect.left,
                 Height + (WindowRect.bottom - WindowRect.top) -
                 (ClientRect.bottom - ClientRect.top),
                 SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);

    /* Workaround for a bug in Windows' edit control
       (multi-line mode) */
    GetWindowRect(Wnd, &WindowRect);             
    SetWindowPos(Wnd, NULL, 0, 0,
                 WindowRect.right - WindowRect.left + 1,
                 WindowRect.bottom - WindowRect.top + 1,
                 SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    SetWindowPos(Wnd, NULL, 0, 0,
                 WindowRect.right - WindowRect.left,
                 WindowRect.bottom - WindowRect.top,
                 SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);

    GetClientRect(Wnd, &ClientRect);
    ok(ClientRect.bottom - ClientRect.top == Height,
        "The client height should be %d, but is %d\n",
        Height, ClientRect.bottom - ClientRect.top);
}

static void test_edit_control_1(void)
{
    HWND hwEdit;
    MSG msMessage;
    int i;
    LONG r;

    msMessage.message = WM_KEYDOWN;

    trace("EDIT: Single line\n");
    hwEdit = create_editcontrol(ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    r = get_edit_style(hwEdit);
    ok(r == (ES_AUTOVSCROLL | ES_AUTOHSCROLL), "Wrong style expected 0xc0 got: 0x%x\n", r);
    for (i=0;i<65535;i++)
    {
	msMessage.wParam = i;
	r = SendMessageA(hwEdit, WM_GETDLGCODE, 0, (LPARAM) &msMessage);
	ok(r == (DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS),
            "Expected DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS got %x\n", r);
    }
    DestroyWindow (hwEdit);

    trace("EDIT: Single line want returns\n");
    hwEdit = create_editcontrol(ES_WANTRETURN | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    r = get_edit_style(hwEdit);
    ok(r == (ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN), "Wrong style expected 0x10c0 got: 0x%x\n", r);
    for (i=0;i<65535;i++)
    {
	msMessage.wParam = i;
	r = SendMessageA(hwEdit, WM_GETDLGCODE, 0, (LPARAM) &msMessage);
	ok(r == (DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS),
            "Expected DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS got %x\n", r);
    }
    DestroyWindow (hwEdit);

    trace("EDIT: Multiline line\n");
    hwEdit = create_editcontrol(ES_MULTILINE | WS_VSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    r = get_edit_style(hwEdit);
    ok(r == (ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE), "Wrong style expected 0xc4 got: 0x%x\n", r);
    for (i=0;i<65535;i++)
    {
	msMessage.wParam = i;
	r = SendMessageA(hwEdit, WM_GETDLGCODE, 0, (LPARAM) &msMessage);
	ok(r == (DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTALLKEYS | DLGC_WANTARROWS),
            "Expected DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTALLKEYS | DLGC_WANTARROWS got %x\n", r);
    }
    DestroyWindow (hwEdit);

    trace("EDIT: Multi line want returns\n");
    hwEdit = create_editcontrol(ES_MULTILINE | WS_VSCROLL | ES_WANTRETURN | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    r = get_edit_style(hwEdit);
    ok(r == (ES_WANTRETURN | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE), "Wrong style expected 0x10c4 got: 0x%x\n", r);
    for (i=0;i<65535;i++)
    {
	msMessage.wParam = i;
	r = SendMessageA(hwEdit, WM_GETDLGCODE, 0, (LPARAM) &msMessage);
	ok(r == (DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTALLKEYS | DLGC_WANTARROWS),
            "Expected DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTALLKEYS | DLGC_WANTARROWS got %x\n", r);
    }
    DestroyWindow (hwEdit);
}

/* WM_SETTEXT is implemented by selecting all text, and then replacing the
 * selection.  This test checks that the first 'select all' doesn't generate
 * an UPDATE message which can escape and (via a handler) change the
 * selection, which would cause WM_SETTEXT to break.  This old bug
 * was fixed 18-Mar-2005; we check here to ensure it doesn't regress.
 */
static void test_edit_control_2(void)
{
    HWND hwndMain, phwnd;
    char szLocalString[MAXLEN];
    LONG r, w = 150, h = 50;
    POINT cpos;

    /* Create main and edit windows. */
    hwndMain = CreateWindowA(szEditTest2Class, "ET2", WS_OVERLAPPEDWINDOW,
                            0, 0, 200, 200, NULL, NULL, hinst, NULL);
    assert(hwndMain);
    if (winetest_interactive)
        ShowWindow (hwndMain, SW_SHOW);

    hwndET2 = CreateWindowA("EDIT", NULL,
                           WS_CHILD|WS_BORDER|ES_LEFT|ES_AUTOHSCROLL,
                           0, 0, w, h, /* important this not be 0 size. */
                           hwndMain, (HMENU) ID_EDITTEST2, hinst, NULL);
    assert(hwndET2);
    if (winetest_interactive)
        ShowWindow (hwndET2, SW_SHOW);

    trace("EDIT: SETTEXT atomicity\n");
    /* Send messages to "type" in the word 'foo'. */
    r = SendMessageA(hwndET2, WM_CHAR, 'f', 1);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);
    r = SendMessageA(hwndET2, WM_CHAR, 'o', 1);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);
    r = SendMessageA(hwndET2, WM_CHAR, 'o', 1);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);
    /* 'foo' should have been changed to 'bar' by the UPDATE handler. */
    GetWindowTextA(hwndET2, szLocalString, MAXLEN);
    ok(strcmp(szLocalString, "bar")==0,
       "Wrong contents of edit: %s\n", szLocalString);

    /* try setting the caret before it's visible */
    r = SetCaretPos(0, 0);
    todo_wine ok(0 == r, "SetCaretPos succeeded unexpectedly, expected: 0, got: %d\n", r);
    phwnd = SetFocus(hwndET2);
    ok(phwnd != NULL, "SetFocus failed unexpectedly, expected non-zero, got NULL\n");
    r = SetCaretPos(0, 0);
    ok(1 == r, "SetCaretPos failed unexpectedly, expected: 1, got: %d\n", r);
    r = GetCaretPos(&cpos);
    ok(1 == r, "GetCaretPos failed unexpectedly, expected: 1, got: %d\n", r);
    ok(cpos.x == 0 && cpos.y == 0, "Wrong caret position, expected: (0,0), got: (%d,%d)\n", cpos.x, cpos.y);
    r = SetCaretPos(-1, -1);
    ok(1 == r, "SetCaretPos failed unexpectedly, expected: 1, got: %d\n", r);
    r = GetCaretPos(&cpos);
    ok(1 == r, "GetCaretPos failed unexpectedly, expected: 1, got: %d\n", r);
    ok(cpos.x == -1 && cpos.y == -1, "Wrong caret position, expected: (-1,-1), got: (%d,%d)\n", cpos.x, cpos.y);
    r = SetCaretPos(w << 1, h << 1);
    ok(1 == r, "SetCaretPos failed unexpectedly, expected: 1, got: %d\n", r);
    r = GetCaretPos(&cpos);
    ok(1 == r, "GetCaretPos failed unexpectedly, expected: 1, got: %d\n", r);
    ok(cpos.x == (w << 1) && cpos.y == (h << 1), "Wrong caret position, expected: (%d,%d), got: (%d,%d)\n", w << 1, h << 1, cpos.x, cpos.y);
    r = SetCaretPos(w, h);
    ok(1 == r, "SetCaretPos failed unexpectedly, expected: 1, got: %d\n", r);
    r = GetCaretPos(&cpos);
    ok(1 == r, "GetCaretPos failed unexpectedly, expected: 1, got: %d\n", r);
    ok(cpos.x == w && cpos.y == h, "Wrong caret position, expected: (%d,%d), got: (%d,%d)\n", w, h, cpos.x, cpos.y);
    r = SetCaretPos(w - 1, h - 1);
    ok(1 == r, "SetCaretPos failed unexpectedly, expected: 1, got: %d\n", r);
    r = GetCaretPos(&cpos);
    ok(1 == r, "GetCaretPos failed unexpectedly, expected: 1, got: %d\n", r);
    ok(cpos.x == (w - 1) && cpos.y == (h - 1), "Wrong caret position, expected: (%d,%d), got: (%d,%d)\n", w - 1, h - 1, cpos.x, cpos.y);

    /* OK, done! */
    DestroyWindow (hwndET2);
    DestroyWindow (hwndMain);
}

static void ET2_check_change(void) {
   char szLocalString[MAXLEN];
   /* This EN_UPDATE handler changes any 'foo' to 'bar'. */
   GetWindowTextA(hwndET2, szLocalString, MAXLEN);
   if (strcmp(szLocalString, "foo")==0) {
       strcpy(szLocalString, "bar");
       SendMessageA(hwndET2, WM_SETTEXT, 0, (LPARAM) szLocalString);
   }
   /* always leave the cursor at the end. */
   SendMessageA(hwndET2, EM_SETSEL, MAXLEN - 1, MAXLEN - 1);
}
static void ET2_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    if (id==ID_EDITTEST2 && codeNotify == EN_UPDATE)
        ET2_check_change();
}
static LRESULT CALLBACK ET2_WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch (iMsg) {
    case WM_COMMAND:
        ET2_OnCommand(hwnd, LOWORD(wParam), (HWND)lParam, HIWORD(wParam));
        break;
    }
    return DefWindowProcA(hwnd, iMsg, wParam, lParam);
}

static void zero_notify(void)
{
    notifications.en_change = 0;
    notifications.en_maxtext = 0;
    notifications.en_update = 0;
}

#define test_notify(enchange, enmaxtext, enupdate) \
    ok(notifications.en_change == enchange, "expected %d EN_CHANGE notifications, " \
    "got %d\n", enchange, notifications.en_change); \
    ok(notifications.en_maxtext == enmaxtext, "expected %d EN_MAXTEXT notifications, " \
    "got %d\n", enmaxtext, notifications.en_maxtext); \
    ok(notifications.en_update == enupdate, "expected %d EN_UPDATE notifications, " \
    "got %d\n", enupdate, notifications.en_update)


static LRESULT CALLBACK edit3_wnd_procA(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_COMMAND:
            switch (HIWORD(wParam)) {
                case EN_MAXTEXT:
                    notifications.en_maxtext++;
                    break;
                case EN_UPDATE:
                    notifications.en_update++;
                    break;
                case EN_CHANGE:
                    notifications.en_change++;
                    break;
            }
            break;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

/* Test behaviour of WM_SETTEXT, WM_REPLACESEL and notificatisons sent in response
 * to these messages.
 */
static void test_edit_control_3(void)
{
    HWND hWnd;
    HWND hParent;
    HDC hDC;
    int len, dpi;
    static const char *str = "this is a long string.";
    static const char *str2 = "this is a long string.\r\nthis is a long string.\r\nthis is a long string.\r\nthis is a long string.";

    hDC = GetDC(NULL);
    dpi = GetDeviceCaps(hDC, LOGPIXELSY);
    ReleaseDC(NULL, hDC);

    trace("EDIT: Test notifications\n");

    hParent = CreateWindowExA(0,
              szEditTest3Class,
              NULL,
              0,
              CW_USEDEFAULT, CW_USEDEFAULT, 10, 10,
              NULL, NULL, NULL, NULL);
    assert(hParent);

    trace("EDIT: Single line, no ES_AUTOHSCROLL\n");
    hWnd = CreateWindowExA(0,
              "EDIT",
              NULL,
              0,
              10, 10, 50, 50,
              hParent, NULL, NULL, NULL);
    assert(hWnd);

    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) > len, "text should have been truncated\n");
    test_notify(1, 1, 1);

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)"a");
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(1 == len, "wrong text length, expected 1, got %d\n", len);
    test_notify(1, 0, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    test_notify(1, 0, 1);

    len = SendMessageA(hWnd, EM_GETSEL, 0, 0);
    ok(LOWORD(len)==0, "Unexpected start position for selection %d\n", LOWORD(len));
    ok(HIWORD(len)==0, "Unexpected end position for selection %d\n", HIWORD(len));
    SendMessageA(hParent, WM_SETFOCUS, 0, (LPARAM)hWnd);
    len = SendMessageA(hWnd, EM_GETSEL, 0, 0);
    ok(LOWORD(len)==0, "Unexpected start position for selection %d\n", LOWORD(len));
    ok(HIWORD(len)==0, "Unexpected end position for selection %d\n", HIWORD(len));

    SendMessageA(hWnd, EM_SETLIMITTEXT, 5, 0);

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(5 == len, "text should have been truncated to limit, expected 5, got %d\n", len);
    test_notify(1, 1, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    test_notify(1, 0, 1);

    DestroyWindow(hWnd);

    trace("EDIT: Single line, ES_AUTOHSCROLL\n");
    hWnd = CreateWindowExA(0,
              "EDIT",
              NULL,
              ES_AUTOHSCROLL,
              10, 10, 50, 50,
              hParent, NULL, NULL, NULL);
    assert(hWnd);

    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    test_notify(1, 0, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    test_notify(1, 0, 1);

    SendMessageA(hWnd, EM_SETLIMITTEXT, 5, 0);

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(5 == len, "text should have been truncated to limit, expected 5, got %d\n", len);
    test_notify(1, 1, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    test_notify(1, 0, 1);

    DestroyWindow(hWnd);

    trace("EDIT: Multline, no ES_AUTOHSCROLL, no ES_AUTOVSCROLL\n");
    hWnd = CreateWindowExA(0,
              "EDIT",
              NULL,
              ES_MULTILINE,
              10, 10, (50 * dpi) / 96, (50 * dpi) / 96,
              hParent, NULL, NULL, NULL);
    assert(hWnd);

    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(0 == len, "text should have been truncated, expected 0, got %d\n", len);
    test_notify(1, 1, 1);

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)"a");
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(1 == SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0), "wrong text length, expected 1, got %d\n", len);
    test_notify(1, 0, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    test_notify(0, 0, 0);

    SendMessageA(hWnd, EM_SETLIMITTEXT, 5, 0);

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(5 == len, "text should have been truncated to limit, expected 5, got %d\n", len);
    test_notify(1, 1, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    test_notify(0, 0, 0);

    DestroyWindow(hWnd);

    trace("EDIT: Multline, ES_AUTOHSCROLL, no ES_AUTOVSCROLL\n");
    hWnd = CreateWindowExA(0,
              "EDIT",
              NULL,
              ES_MULTILINE | ES_AUTOHSCROLL,
              10, 10, (50 * dpi) / 96, (50 * dpi) / 96,
              hParent, NULL, NULL, NULL);
    assert(hWnd);

    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(0 == len, "text should have been truncated, expected 0, got %d\n", len);
    test_notify(1, 1, 1);

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)"a");
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(1 == SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0), "wrong text length, expected 1, got %d\n", len);
    test_notify(1, 0, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str2) == len, "text shouldn't have been truncated\n");
    test_notify(0, 0, 0);

    SendMessageA(hWnd, EM_SETLIMITTEXT, 5, 0);

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(5 == len, "text should have been truncated to limit, expected 5, got %d\n", len);
    test_notify(1, 1, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str2) == len, "text shouldn't have been truncated\n");
    test_notify(0, 0, 0);

    DestroyWindow(hWnd);

    trace("EDIT: Multline, ES_AUTOHSCROLL and ES_AUTOVSCROLL\n");
    hWnd = CreateWindowExA(0,
              "EDIT",
              NULL,
              ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
              10, 10, 50, 50,
              hParent, NULL, NULL, NULL);
    assert(hWnd);

    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str2) == len, "text shouldn't have been truncated\n");
    test_notify(1, 0, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str2) == len, "text shouldn't have been truncated\n");
    test_notify(0, 0, 0);

    SendMessageA(hWnd, EM_SETLIMITTEXT, 5, 0);

    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)"");
    zero_notify();
    SendMessageA(hWnd, EM_REPLACESEL, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(5 == len, "text should have been truncated to limit, expected 5, got %d\n", len);
    test_notify(1, 1, 1);

    zero_notify();
    SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str2);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str2) == len, "text shouldn't have been truncated\n");
    test_notify(0, 0, 0);

    DestroyWindow(hWnd);
}

/* Test EM_CHARFROMPOS and EM_POSFROMCHAR
 */
static void test_edit_control_4(void)
{
    HWND hwEdit;
    int lo, hi, mid;
    int ret;
    int i;

    trace("EDIT: Test EM_CHARFROMPOS and EM_POSFROMCHAR\n");
    hwEdit = create_editcontrol(ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    SendMessageA(hwEdit, WM_SETTEXT, 0, (LPARAM) "aa");
    lo = LOWORD(SendMessageA(hwEdit, EM_POSFROMCHAR, 0, 0));
    hi = LOWORD(SendMessageA(hwEdit, EM_POSFROMCHAR, 1, 0));
    mid = lo + (hi - lo) / 2;

    for (i = lo; i < mid; i++) {
       ret = LOWORD(SendMessageA(hwEdit, EM_CHARFROMPOS, 0, i));
       ok(0 == ret, "expected 0 got %d\n", ret);
    }
    for (i = mid; i <= hi; i++) {
       ret = LOWORD(SendMessageA(hwEdit, EM_CHARFROMPOS, 0, i));
       ok(1 == ret, "expected 1 got %d\n", ret);
    }
    ret = SendMessageA(hwEdit, EM_POSFROMCHAR, 2, 0);
    ok(-1 == ret, "expected -1 got %d\n", ret);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(ES_RIGHT | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    SendMessageA(hwEdit, WM_SETTEXT, 0, (LPARAM) "aa");
    lo = LOWORD(SendMessageA(hwEdit, EM_POSFROMCHAR, 0, 0));
    hi = LOWORD(SendMessageA(hwEdit, EM_POSFROMCHAR, 1, 0));
    mid = lo + (hi - lo) / 2;

    for (i = lo; i < mid; i++) {
       ret = LOWORD(SendMessageA(hwEdit, EM_CHARFROMPOS, 0, i));
       ok(0 == ret, "expected 0 got %d\n", ret);
    }
    for (i = mid; i <= hi; i++) {
       ret = LOWORD(SendMessageA(hwEdit, EM_CHARFROMPOS, 0, i));
       ok(1 == ret, "expected 1 got %d\n", ret);
    }
    ret = SendMessageA(hwEdit, EM_POSFROMCHAR, 2, 0);
    ok(-1 == ret, "expected -1 got %d\n", ret);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(ES_CENTER | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    SendMessageA(hwEdit, WM_SETTEXT, 0, (LPARAM) "aa");
    lo = LOWORD(SendMessageA(hwEdit, EM_POSFROMCHAR, 0, 0));
    hi = LOWORD(SendMessageA(hwEdit, EM_POSFROMCHAR, 1, 0));
    mid = lo + (hi - lo) / 2;

    for (i = lo; i < mid; i++) {
       ret = LOWORD(SendMessageA(hwEdit, EM_CHARFROMPOS, 0, i));
       ok(0 == ret, "expected 0 got %d\n", ret);
    }
    for (i = mid; i <= hi; i++) {
       ret = LOWORD(SendMessageA(hwEdit, EM_CHARFROMPOS, 0, i));
       ok(1 == ret, "expected 1 got %d\n", ret);
    }
    ret = SendMessageA(hwEdit, EM_POSFROMCHAR, 2, 0);
    ok(-1 == ret, "expected -1 got %d\n", ret);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    SendMessageA(hwEdit, WM_SETTEXT, 0, (LPARAM) "aa");
    lo = LOWORD(SendMessageA(hwEdit, EM_POSFROMCHAR, 0, 0));
    hi = LOWORD(SendMessageA(hwEdit, EM_POSFROMCHAR, 1, 0));
    mid = lo + (hi - lo) / 2 +1;

    for (i = lo; i < mid; i++) {
       ret = LOWORD(SendMessageA(hwEdit, EM_CHARFROMPOS, 0, i));
       ok((0 == ret || 1 == ret /* Vista */), "expected 0 or 1 got %d\n", ret);
    }
    for (i = mid; i <= hi; i++) {
       ret = LOWORD(SendMessageA(hwEdit, EM_CHARFROMPOS, 0, i));
       ok(1 == ret, "expected 1 got %d\n", ret);
    }
    ret = SendMessageA(hwEdit, EM_POSFROMCHAR, 2, 0);
    ok(-1 == ret, "expected -1 got %d\n", ret);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(ES_MULTILINE | ES_RIGHT | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    SendMessageA(hwEdit, WM_SETTEXT, 0, (LPARAM) "aa");
    lo = LOWORD(SendMessageA(hwEdit, EM_POSFROMCHAR, 0, 0));
    hi = LOWORD(SendMessageA(hwEdit, EM_POSFROMCHAR, 1, 0));
    mid = lo + (hi - lo) / 2 +1;

    for (i = lo; i < mid; i++) {
       ret = LOWORD(SendMessageA(hwEdit, EM_CHARFROMPOS, 0, i));
       ok((0 == ret || 1 == ret /* Vista */), "expected 0 or 1 got %d\n", ret);
    }
    for (i = mid; i <= hi; i++) {
       ret = LOWORD(SendMessageA(hwEdit, EM_CHARFROMPOS, 0, i));
       ok(1 == ret, "expected 1 got %d\n", ret);
    }
    ret = SendMessageA(hwEdit, EM_POSFROMCHAR, 2, 0);
    ok(-1 == ret, "expected -1 got %d\n", ret);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(ES_MULTILINE | ES_CENTER | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    SendMessageA(hwEdit, WM_SETTEXT, 0, (LPARAM) "aa");
    lo = LOWORD(SendMessageA(hwEdit, EM_POSFROMCHAR, 0, 0));
    hi = LOWORD(SendMessageA(hwEdit, EM_POSFROMCHAR, 1, 0));
    mid = lo + (hi - lo) / 2 +1;

    for (i = lo; i < mid; i++) {
       ret = LOWORD(SendMessageA(hwEdit, EM_CHARFROMPOS, 0, i));
       ok((0 == ret || 1 == ret /* Vista */), "expected 0 or 1 got %d\n", ret);
    }
    for (i = mid; i <= hi; i++) {
       ret = LOWORD(SendMessageA(hwEdit, EM_CHARFROMPOS, 0, i));
       ok(1 == ret, "expected 1 got %d\n", ret);
    }
    ret = SendMessageA(hwEdit, EM_POSFROMCHAR, 2, 0);
    ok(-1 == ret, "expected -1 got %d\n", ret);
    DestroyWindow(hwEdit);
}

/* Test if creating edit control without ES_AUTOHSCROLL and ES_AUTOVSCROLL
 * truncates text that doesn't fit.
 */
static void test_edit_control_5(void)
{
    static const char *str = "test\r\ntest";
    HWND parentWnd;
    HWND hWnd;
    int len;
    RECT rc1 = { 10, 10, 11, 11};
    RECT rc;

    /* first show that a non-child won't do for this test */
    hWnd = CreateWindowExA(0,
              "EDIT",
              str,
              0,
              10, 10, 1, 1,
              NULL, NULL, NULL, NULL);
    assert(hWnd);
    /* size of non-child edit control is (much) bigger than requested */
    GetWindowRect( hWnd, &rc);
    ok( rc.right - rc.left > 20, "size of the window (%d) is smaller than expected\n",
            rc.right - rc.left);
    DestroyWindow(hWnd);
    /* so create a parent, and give it edit controls children to test with */
    parentWnd = CreateWindowExA(0,
                            szEditTextPositionClass,
                            "Edit Test", WS_VISIBLE |
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            250, 250,
                            NULL, NULL, hinst, NULL);
    assert(parentWnd);
    ShowWindow( parentWnd, SW_SHOW);
    /* single line */
    hWnd = CreateWindowExA(0,
              "EDIT",
              str, WS_VISIBLE | WS_BORDER |
              WS_CHILD,
              rc1.left, rc1.top, rc1.right - rc1.left, rc1.bottom - rc1.top,
              parentWnd, NULL, NULL, NULL);
    assert(hWnd);
    GetClientRect( hWnd, &rc);
    ok( rc.right == rc1.right - rc1.left && rc.bottom == rc1.bottom - rc1.top,
            "Client rectangle not the expected size (%d,%d,%d,%d)\n",
            rc.left, rc.top, rc.right, rc.bottom);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    DestroyWindow(hWnd);
    /* multi line */
    hWnd = CreateWindowExA(0,
              "EDIT",
              str,
              WS_CHILD | ES_MULTILINE,
              rc1.left, rc1.top, rc1.right - rc1.left, rc1.bottom - rc1.top,
              parentWnd, NULL, NULL, NULL);
    assert(hWnd);
    GetClientRect( hWnd, &rc);
    ok( rc.right == rc1.right - rc1.left && rc.bottom == rc1.bottom - rc1.top,
            "Client rectangle not the expected size (%d,%d,%d,%d)\n",
            rc.left, rc.top, rc.right, rc.bottom);
    len = SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ok(lstrlenA(str) == len, "text shouldn't have been truncated\n");
    DestroyWindow(hWnd);
}

/* Test WM_GETTEXT processing
 * after destroy messages
 */
static void test_edit_control_6(void)
{
    static const char *str = "test\r\ntest";
    char buf[MAXLEN];
    LONG ret;
    HWND hWnd;

    hWnd = CreateWindowExA(0,
              "EDIT",
              "Test",
              0,
              10, 10, 1, 1,
              NULL, NULL, hinst, NULL);
    assert(hWnd);

    ret = SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)str);
    ok(ret == TRUE, "Expected %d, got %d\n", TRUE, ret);
    ret = SendMessageA(hWnd, WM_GETTEXT, MAXLEN, (LPARAM)buf);
    ok(ret == strlen(str), "Expected %s, got len %d\n", str, ret);
    ok(!strcmp(buf, str), "Expected %s, got %s\n", str, buf);
    buf[0] = 0;
    ret = SendMessageA(hWnd, WM_DESTROY, 0, 0);
    ok(ret == 0, "Expected 0, got %d\n", ret);
    ret = SendMessageA(hWnd, WM_GETTEXT, MAXLEN, (LPARAM)buf);
    ok(ret == strlen(str), "Expected %s, got len %d\n", str, ret);
    ok(!strcmp(buf, str), "Expected %s, got %s\n", str, buf);
    buf[0] = 0;
    ret = SendMessageA(hWnd, WM_NCDESTROY, 0, 0);
    ok(ret == 0, "Expected 0, got %d\n", ret);
    ret = SendMessageA(hWnd, WM_GETTEXT, MAXLEN, (LPARAM)buf);
    ok(ret == 0, "Expected 0, got len %d\n", ret);
    ok(!strcmp(buf, ""), "Expected empty string, got %s\n", buf);

    DestroyWindow(hWnd);
}

static void test_edit_control_limittext(void)
{
    HWND hwEdit;
    DWORD r;

    /* Test default limit for single-line control */
    trace("EDIT: buffer limit for single-line\n");
    hwEdit = create_editcontrol(ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    r = SendMessageA(hwEdit, EM_GETLIMITTEXT, 0, 0);
    ok(r == 30000, "Incorrect default text limit, expected 30000 got %u\n", r);
    SendMessageA(hwEdit, EM_SETLIMITTEXT, 0, 0);
    r = SendMessageA(hwEdit, EM_GETLIMITTEXT, 0, 0);
    ok( r == 2147483646, "got limit %u (expected 2147483646)\n", r);
    DestroyWindow(hwEdit);

    /* Test default limit for multi-line control */
    trace("EDIT: buffer limit for multi-line\n");
    hwEdit = create_editcontrol(ES_MULTILINE | WS_VSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);
    r = SendMessageA(hwEdit, EM_GETLIMITTEXT, 0, 0);
    ok(r == 30000, "Incorrect default text limit, expected 30000 got %u\n", r);
    SendMessageA(hwEdit, EM_SETLIMITTEXT, 0, 0);
    r = SendMessageA(hwEdit, EM_GETLIMITTEXT, 0, 0);
    ok( r == 4294967295U, "got limit %u (expected 4294967295)\n", r);
    DestroyWindow(hwEdit);
}

/* Test EM_SCROLL */
static void test_edit_control_scroll(void)
{
    static const char *single_line_str = "a";
    static const char *multiline_str = "Test\r\nText";
    HWND hwEdit;
    LONG ret;

    /* Check the return value when EM_SCROLL doesn't scroll
     * anything. Should not return true unless any lines were actually
     * scrolled. */
    hwEdit = CreateWindowA(
              "EDIT",
              single_line_str,
              WS_VSCROLL | ES_MULTILINE,
              1, 1, 100, 100,
              NULL, NULL, hinst, NULL);

    assert(hwEdit);

    ret = SendMessageA(hwEdit, EM_SCROLL, SB_PAGEDOWN, 0);
    ok(!ret, "Returned %x, expected 0.\n", ret);

    ret = SendMessageA(hwEdit, EM_SCROLL, SB_PAGEUP, 0);
    ok(!ret, "Returned %x, expected 0.\n", ret);

    ret = SendMessageA(hwEdit, EM_SCROLL, SB_LINEUP, 0);
    ok(!ret, "Returned %x, expected 0.\n", ret);

    ret = SendMessageA(hwEdit, EM_SCROLL, SB_LINEDOWN, 0);
    ok(!ret, "Returned %x, expected 0.\n", ret);

    DestroyWindow (hwEdit);

    /* SB_PAGEDOWN while at the beginning of a buffer with few lines
       should not cause EM_SCROLL to return a negative value of
       scrolled lines that would put us "before" the beginning. */
    hwEdit = CreateWindowA(
                "EDIT",
                multiline_str,
                WS_VSCROLL | ES_MULTILINE,
                0, 0, 100, 100,
                NULL, NULL, hinst, NULL);
    assert(hwEdit);

    ret = SendMessageA(hwEdit, EM_SCROLL, SB_PAGEDOWN, 0);
    ok(!ret, "Returned %x, expected 0.\n", ret);

    DestroyWindow (hwEdit);
}

static void test_margins(void)
{
    HWND hwEdit;
    RECT old_rect, new_rect;
    INT old_right_margin;
    DWORD old_margins, new_margins;

    hwEdit = create_editcontrol(WS_BORDER | ES_AUTOHSCROLL | ES_AUTOVSCROLL, 0);

    old_margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    old_right_margin = HIWORD(old_margins);

    /* Check if setting the margins works */

    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN, MAKELONG(10, 0));
    new_margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(new_margins) == 10, "Wrong left margin: %d\n", LOWORD(new_margins));
    ok(HIWORD(new_margins) == old_right_margin, "Wrong right margin: %d\n", HIWORD(new_margins));

    SendMessageA(hwEdit, EM_SETMARGINS, EC_RIGHTMARGIN, MAKELONG(0, 10));
    new_margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(new_margins) == 10, "Wrong left margin: %d\n", LOWORD(new_margins));
    ok(HIWORD(new_margins) == 10, "Wrong right margin: %d\n", HIWORD(new_margins));

    /* The size of the rectangle must decrease if we increase the margin */

    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(5, 5));
    SendMessageA(hwEdit, EM_GETRECT, 0, (LPARAM)&old_rect);
    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(15, 20));
    SendMessageA(hwEdit, EM_GETRECT, 0, (LPARAM)&new_rect);
    ok(new_rect.left == old_rect.left + 10, "The left border of the rectangle is wrong\n");
    ok(new_rect.right == old_rect.right - 15, "The right border of the rectangle is wrong\n");
    ok(new_rect.top == old_rect.top, "The top border of the rectangle must not change\n");
    ok(new_rect.bottom == old_rect.bottom, "The bottom border of the rectangle must not change\n");

    /* If we set the margin to same value as the current margin,
       the rectangle must not change */

    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(10, 10));
    old_rect.left = 1;
    old_rect.right = 99;
    old_rect.top = 1;
    old_rect.bottom = 99;
    SendMessageA(hwEdit, EM_SETRECT, 0, (LPARAM)&old_rect);
    SendMessageA(hwEdit, EM_GETRECT, 0, (LPARAM)&old_rect);
    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(10, 10));
    SendMessageA(hwEdit, EM_GETRECT, 0, (LPARAM)&new_rect);
    ok(new_rect.left == old_rect.left, "The left border of the rectangle has changed\n");
    ok(new_rect.right == old_rect.right, "The right border of the rectangle has changed\n");
    ok(new_rect.top == old_rect.top, "The top border of the rectangle has changed\n");
    ok(new_rect.bottom == old_rect.bottom, "The bottom border of the rectangle has changed\n");

    DestroyWindow (hwEdit);
}

static INT CALLBACK find_font_proc(const LOGFONTA *elf, const TEXTMETRICA *ntm, DWORD type, LPARAM lParam)
{
    return 0;
}

static void test_margins_font_change(void)
{
    HWND hwEdit;
    DWORD margins, font_margins;
    LOGFONTA lf;
    HFONT hfont, hfont2;
    HDC hdc = GetDC(0);

    if(EnumFontFamiliesA(hdc, "Arial", find_font_proc, 0))
    {
        trace("Arial not found - skipping font change margin tests\n");
        ReleaseDC(0, hdc);
        return;
    }
    ReleaseDC(0, hdc);

    hwEdit = create_child_editcontrol(0, 0);

    SetWindowPos(hwEdit, NULL, 10, 10, 1000, 100, SWP_NOZORDER | SWP_NOACTIVATE);

    memset(&lf, 0, sizeof(lf));
    strcpy(lf.lfFaceName, "Arial");
    lf.lfHeight = 16;
    lf.lfCharSet = DEFAULT_CHARSET;
    hfont = CreateFontIndirectA(&lf);
    lf.lfHeight = 30;
    hfont2 = CreateFontIndirectA(&lf);

    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont, 0);
    font_margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(font_margins) != 0, "got %d\n", LOWORD(font_margins));
    ok(HIWORD(font_margins) != 0, "got %d\n", HIWORD(font_margins));

    /* With 'small' edit controls, test that the margin doesn't get set */
    SetWindowPos(hwEdit, NULL, 10, 10, 16, 100, SWP_NOZORDER | SWP_NOACTIVATE);
    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(0,0));
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont, 0);
    margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) == 0 || broken(LOWORD(margins) == LOWORD(font_margins)), /* win95 */
       "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) == 0 || broken(HIWORD(margins) == HIWORD(font_margins)), /* win95 */
       "got %d\n", HIWORD(margins));

    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(1,0));
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont, 0);
    margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) == 1 || broken(LOWORD(margins) == LOWORD(font_margins)), /* win95 */
       "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) == 0 || broken(HIWORD(margins) == HIWORD(font_margins)), /* win95 */
       "got %d\n", HIWORD(margins));

    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(1,1));
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont, 0);
    margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) == 1 || broken(LOWORD(margins) == LOWORD(font_margins)), /* win95 */
       "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) == 1 || broken(HIWORD(margins) == HIWORD(font_margins)), /* win95 */
       "got %d\n", HIWORD(margins));

    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(EC_USEFONTINFO,EC_USEFONTINFO));
    margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) == 1 || broken(LOWORD(margins) == LOWORD(font_margins)), /* win95 */
       "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) == 1 || broken(HIWORD(margins) == HIWORD(font_margins)), /* win95 */
       "got %d\n", HIWORD(margins));

    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont2, 0);
    margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) == 1 || broken(LOWORD(margins) != 1 && LOWORD(margins) != LOWORD(font_margins)), /* win95 */
       "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) == 1 || broken(HIWORD(margins) != 1 && HIWORD(margins) != HIWORD(font_margins)), /* win95 */
       "got %d\n", HIWORD(margins));

    /* Above a certain size threshold then the margin is updated */
    SetWindowPos(hwEdit, NULL, 10, 10, 1000, 100, SWP_NOZORDER | SWP_NOACTIVATE);
    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(1,0));
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont, 0);
    margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) == LOWORD(font_margins), "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) == HIWORD(font_margins), "got %d\n", HIWORD(margins)); 

    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(1,1));
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont, 0);
    margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) == LOWORD(font_margins), "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) == HIWORD(font_margins), "got %d\n", HIWORD(margins)); 

    SendMessageA(hwEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(EC_USEFONTINFO,EC_USEFONTINFO));
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont, 0);
    margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) == LOWORD(font_margins), "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) == HIWORD(font_margins), "got %d\n", HIWORD(margins)); 
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM)hfont2, 0);
    margins = SendMessageA(hwEdit, EM_GETMARGINS, 0, 0);
    ok(LOWORD(margins) != LOWORD(font_margins) || broken(LOWORD(margins) == LOWORD(font_margins)), /* win98 */
       "got %d\n", LOWORD(margins));
    ok(HIWORD(margins) != HIWORD(font_margins), "got %d\n", HIWORD(margins)); 

    SendMessageA(hwEdit, WM_SETFONT, 0, 0);

    DeleteObject(hfont2);
    DeleteObject(hfont);
    destroy_child_editcontrol(hwEdit);

}

#define edit_pos_ok(exp, got, txt) \
    ok(exp == got, "wrong " #txt " expected %d got %d\n", exp, got);

#define check_pos(hwEdit, set_height, test_top, test_height, test_left) \
do { \
    RECT format_rect; \
    int left_margin; \
    set_client_height(hwEdit, set_height); \
    SendMessageA(hwEdit, EM_GETRECT, 0, (LPARAM) &format_rect); \
    left_margin = LOWORD(SendMessageA(hwEdit, EM_GETMARGINS, 0, 0)); \
    edit_pos_ok(test_top, format_rect.top, vertical position); \
    edit_pos_ok((int)test_height, format_rect.bottom - format_rect.top, height); \
    edit_pos_ok(test_left, format_rect.left - left_margin, left); \
} while(0)

static void test_text_position_style(DWORD style)
{
    HWND hwEdit;
    HFONT font, oldFont;
    HDC dc;
    TEXTMETRICA metrics;
    INT b, bm, b2, b3;
    BOOL xb, single_line = !(style & ES_MULTILINE);

    b = GetSystemMetrics(SM_CYBORDER) + 1;
    b2 = 2 * b;
    b3 = 3 * b;
    bm = b2 - 1;

    /* Get a stock font for which we can determine the metrics */
    font = GetStockObject(SYSTEM_FONT);
    ok (font != NULL, "GetStockObjcet SYSTEM_FONT failed\n");
    dc = GetDC(NULL);
    ok (dc != NULL, "GetDC() failed\n");
    oldFont = SelectObject(dc, font);
    xb = GetTextMetricsA(dc, &metrics);
    ok (xb, "GetTextMetrics failed\n");
    SelectObject(dc, oldFont);
    ReleaseDC(NULL, dc);

    /* Windows' edit control has some bugs in multi-line mode:
     * - Sometimes the format rectangle doesn't get updated
     *   (see workaround in set_client_height())
     * - If the height of the control is smaller than the height of a text
     *   line, the format rectangle is still as high as a text line
     *   (higher than the client rectangle) and the caret is not shown
     */

    /* Edit controls that are in a parent window */

    hwEdit = create_child_editcontrol(style | WS_VISIBLE, 0);
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM) font, FALSE);
    if (single_line)
    check_pos(hwEdit, metrics.tmHeight -  1, 0, metrics.tmHeight - 1, 0);
    check_pos(hwEdit, metrics.tmHeight     , 0, metrics.tmHeight    , 0);
    check_pos(hwEdit, metrics.tmHeight +  1, 0, metrics.tmHeight    , 0);
    check_pos(hwEdit, metrics.tmHeight +  2, 0, metrics.tmHeight    , 0);
    check_pos(hwEdit, metrics.tmHeight + 10, 0, metrics.tmHeight    , 0);
    destroy_child_editcontrol(hwEdit);

    hwEdit = create_child_editcontrol(style | WS_BORDER | WS_VISIBLE, 0);
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM) font, FALSE);
    if (single_line)
    check_pos(hwEdit, metrics.tmHeight -  1, 0, metrics.tmHeight - 1, b);
    check_pos(hwEdit, metrics.tmHeight     , 0, metrics.tmHeight    , b);
    check_pos(hwEdit, metrics.tmHeight +  1, 0, metrics.tmHeight    , b);
    check_pos(hwEdit, metrics.tmHeight + bm, 0, metrics.tmHeight    , b);
    check_pos(hwEdit, metrics.tmHeight + b2, b, metrics.tmHeight    , b);
    check_pos(hwEdit, metrics.tmHeight + b3, b, metrics.tmHeight    , b);
    destroy_child_editcontrol(hwEdit);

    hwEdit = create_child_editcontrol(style | WS_VISIBLE, WS_EX_CLIENTEDGE);
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM) font, FALSE);
    if (single_line)
    check_pos(hwEdit, metrics.tmHeight -  1, 0, metrics.tmHeight - 1, 1);
    check_pos(hwEdit, metrics.tmHeight     , 0, metrics.tmHeight    , 1);
    check_pos(hwEdit, metrics.tmHeight +  1, 0, metrics.tmHeight    , 1);
    check_pos(hwEdit, metrics.tmHeight +  2, 1, metrics.tmHeight    , 1);
    check_pos(hwEdit, metrics.tmHeight + 10, 1, metrics.tmHeight    , 1);
    destroy_child_editcontrol(hwEdit);

    hwEdit = create_child_editcontrol(style | WS_BORDER | WS_VISIBLE, WS_EX_CLIENTEDGE);
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM) font, FALSE);
    if (single_line)
    check_pos(hwEdit, metrics.tmHeight -  1, 0, metrics.tmHeight - 1, 1);
    check_pos(hwEdit, metrics.tmHeight     , 0, metrics.tmHeight    , 1);
    check_pos(hwEdit, metrics.tmHeight +  1, 0, metrics.tmHeight    , 1);
    check_pos(hwEdit, metrics.tmHeight +  2, 1, metrics.tmHeight    , 1);
    check_pos(hwEdit, metrics.tmHeight + 10, 1, metrics.tmHeight    , 1);
    destroy_child_editcontrol(hwEdit);


    /* Edit controls that are popup windows */

    hwEdit = create_editcontrol(style | WS_POPUP, 0);
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM) font, FALSE);
    if (single_line)
    check_pos(hwEdit, metrics.tmHeight -  1, 0, metrics.tmHeight - 1, 0);
    check_pos(hwEdit, metrics.tmHeight     , 0, metrics.tmHeight    , 0);
    check_pos(hwEdit, metrics.tmHeight +  1, 0, metrics.tmHeight    , 0);
    check_pos(hwEdit, metrics.tmHeight +  2, 0, metrics.tmHeight    , 0);
    check_pos(hwEdit, metrics.tmHeight + 10, 0, metrics.tmHeight    , 0);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(style | WS_POPUP | WS_BORDER, 0);
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM) font, FALSE);
    if (single_line)
    check_pos(hwEdit, metrics.tmHeight -  1, 0, metrics.tmHeight - 1, b);
    check_pos(hwEdit, metrics.tmHeight     , 0, metrics.tmHeight    , b);
    check_pos(hwEdit, metrics.tmHeight +  1, 0, metrics.tmHeight    , b);
    check_pos(hwEdit, metrics.tmHeight + bm, 0, metrics.tmHeight    , b);
    check_pos(hwEdit, metrics.tmHeight + b2, b, metrics.tmHeight    , b);
    check_pos(hwEdit, metrics.tmHeight + b3, b, metrics.tmHeight    , b);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(style | WS_POPUP, WS_EX_CLIENTEDGE);
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM) font, FALSE);
    if (single_line)
    check_pos(hwEdit, metrics.tmHeight -  1, 0, metrics.tmHeight - 1, 1);
    check_pos(hwEdit, metrics.tmHeight     , 0, metrics.tmHeight    , 1);
    check_pos(hwEdit, metrics.tmHeight +  1, 0, metrics.tmHeight    , 1);
    check_pos(hwEdit, metrics.tmHeight +  2, 1, metrics.tmHeight    , 1);
    check_pos(hwEdit, metrics.tmHeight + 10, 1, metrics.tmHeight    , 1);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(style | WS_POPUP | WS_BORDER, WS_EX_CLIENTEDGE);
    SendMessageA(hwEdit, WM_SETFONT, (WPARAM) font, FALSE);
    if (single_line)
    check_pos(hwEdit, metrics.tmHeight -  1, 0, metrics.tmHeight - 1, 1);
    check_pos(hwEdit, metrics.tmHeight     , 0, metrics.tmHeight    , 1);
    check_pos(hwEdit, metrics.tmHeight +  1, 0, metrics.tmHeight    , 1);
    check_pos(hwEdit, metrics.tmHeight +  2, 1, metrics.tmHeight    , 1);
    check_pos(hwEdit, metrics.tmHeight + 10, 1, metrics.tmHeight    , 1);
    DestroyWindow(hwEdit);
}

static void test_text_position(void)
{
    trace("EDIT: Text position (Single line)\n");
    test_text_position_style(ES_AUTOHSCROLL | ES_AUTOVSCROLL);
    trace("EDIT: Text position (Multi line)\n");
    test_text_position_style(ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL);
}

static void test_espassword(void)
{
    HWND hwEdit;
    LONG r;
    char buffer[1024];
    const char* password = "secret";

    hwEdit = create_editcontrol(ES_PASSWORD, 0);
    r = get_edit_style(hwEdit);
    ok(r == ES_PASSWORD, "Wrong style expected ES_PASSWORD got: 0x%x\n", r);
    /* set text */
    r = SendMessageA(hwEdit , WM_SETTEXT, 0, (LPARAM) password);
    ok(r == TRUE, "Expected: %d, got: %d\n", TRUE, r);

    /* select all, cut (ctrl-x) */
    SendMessageA(hwEdit, EM_SETSEL, 0, -1);
    r = SendMessageA(hwEdit, WM_CHAR, 24, 0);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);

    /* get text */
    r = SendMessageA(hwEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    ok(r == strlen(password), "Expected: %s, got len %d\n", password, r);
    ok(strcmp(buffer, password) == 0, "expected %s, got %s\n", password, buffer);

    r = OpenClipboard(hwEdit);
    ok(r == TRUE, "expected %d, got %d\n", TRUE, r);
    r = EmptyClipboard();
    ok(r == TRUE, "expected %d, got %d\n", TRUE, r);
    r = CloseClipboard();
    ok(r == TRUE, "expected %d, got %d\n", TRUE, r);

    /* select all, copy (ctrl-c) and paste (ctrl-v) */
    SendMessageA(hwEdit, EM_SETSEL, 0, -1);
    r = SendMessageA(hwEdit, WM_CHAR, 3, 0);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);
    r = SendMessageA(hwEdit, WM_CHAR, 22, 0);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessageA(hwEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    ok(r == 0, "Expected: 0, got: %d\n", r);
    ok(strcmp(buffer, "") == 0, "expected empty string, got %s\n", buffer);

    DestroyWindow (hwEdit);
}

static void test_undo(void)
{
    HWND hwEdit;
    LONG r;
    DWORD cpMin, cpMax;
    char buffer[1024];
    const char* text = "undo this";

    hwEdit = create_editcontrol(0, 0);
    r = get_edit_style(hwEdit);
    ok(0 == r, "Wrong style expected 0x%x got: 0x%x\n", 0, r);

    /* set text */
    r = SendMessageA(hwEdit , WM_SETTEXT, 0, (LPARAM) text);
    ok(TRUE == r, "Expected: %d, got: %d\n", TRUE, r);

    /* select all, */
    cpMin = cpMax = 0xdeadbeef;
    SendMessageA(hwEdit, EM_SETSEL, 0, -1);
    r = SendMessageA(hwEdit, EM_GETSEL, (WPARAM) &cpMin, (LPARAM) &cpMax);
    ok((strlen(text) << 16) == r, "Unexpected length %d\n", r);
    ok(0 == cpMin, "Expected: %d, got %d\n", 0, cpMin);
    ok(9 == cpMax, "Expected: %d, got %d\n", 9, cpMax);

    /* cut (ctrl-x) */
    r = SendMessageA(hwEdit, WM_CHAR, 24, 0);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessageA(hwEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    ok(0 == r, "Expected: %d, got len %d\n", 0, r);
    ok(0 == strcmp(buffer, ""), "expected %s, got %s\n", "", buffer);

    /* undo (ctrl-z) */
    r = SendMessageA(hwEdit, WM_CHAR, 26, 0);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessageA(hwEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    ok(strlen(text) == r, "Unexpected length %d\n", r);
    ok(0 == strcmp(buffer, text), "expected %s, got %s\n", text, buffer);

    /* undo again (ctrl-z) */
    r = SendMessageA(hwEdit, WM_CHAR, 26, 0);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessageA(hwEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
    ok(r == 0, "Expected: %d, got len %d\n", 0, r);
    ok(0 == strcmp(buffer, ""), "expected %s, got %s\n", "", buffer);

    DestroyWindow (hwEdit);
}

static void test_enter(void)
{
    HWND hwEdit;
    LONG r;
    char buffer[16];

    /* multiline */
    hwEdit = create_editcontrol(ES_MULTILINE, 0);
    r = get_edit_style(hwEdit);
    ok(ES_MULTILINE == r, "Wrong style expected ES_MULTILINE got: 0x%x\n", r);

    /* set text */
    r = SendMessageA(hwEdit , WM_SETTEXT, 0, (LPARAM) "");
    ok(TRUE == r, "Expected: %d, got: %d\n", TRUE, r);

    r = SendMessageA(hwEdit, WM_CHAR, VK_RETURN, 0);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessageA(hwEdit, WM_GETTEXT, 16, (LPARAM) buffer);
    ok(2 == r, "Expected: %d, got len %d\n", 2, r);
    ok(0 == strcmp(buffer, "\r\n"), "expected \"\\r\\n\", got \"%s\"\n", buffer);

    DestroyWindow (hwEdit);

    /* single line */
    hwEdit = create_editcontrol(0, 0);
    r = get_edit_style(hwEdit);
    ok(0 == r, "Wrong style expected 0x%x got: 0x%x\n", 0, r);

    /* set text */
    r = SendMessageA(hwEdit , WM_SETTEXT, 0, (LPARAM) "");
    ok(TRUE == r, "Expected: %d, got: %d\n", TRUE, r);

    r = SendMessageA(hwEdit, WM_CHAR, VK_RETURN, 0);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessageA(hwEdit, WM_GETTEXT, 16, (LPARAM) buffer);
    ok(0 == r, "Expected: %d, got len %d\n", 0, r);
    ok(0 == strcmp(buffer, ""), "expected \"\", got \"%s\"\n", buffer);

    DestroyWindow (hwEdit);

    /* single line with ES_WANTRETURN */
    hwEdit = create_editcontrol(ES_WANTRETURN, 0);
    r = get_edit_style(hwEdit);
    ok(ES_WANTRETURN == r, "Wrong style expected ES_WANTRETURN got: 0x%x\n", r);

    /* set text */
    r = SendMessageA(hwEdit , WM_SETTEXT, 0, (LPARAM) "");
    ok(TRUE == r, "Expected: %d, got: %d\n", TRUE, r);

    r = SendMessageA(hwEdit, WM_CHAR, VK_RETURN, 0);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessageA(hwEdit, WM_GETTEXT, 16, (LPARAM) buffer);
    ok(0 == r, "Expected: %d, got len %d\n", 0, r);
    ok(0 == strcmp(buffer, ""), "expected \"\", got \"%s\"\n", buffer);

    DestroyWindow (hwEdit);
}

static void test_tab(void)
{
    HWND hwEdit;
    LONG r;
    char buffer[16];

    /* multiline */
    hwEdit = create_editcontrol(ES_MULTILINE, 0);
    r = get_edit_style(hwEdit);
    ok(ES_MULTILINE == r, "Wrong style expected ES_MULTILINE got: 0x%x\n", r);

    /* set text */
    r = SendMessageA(hwEdit , WM_SETTEXT, 0, (LPARAM) "");
    ok(TRUE == r, "Expected: %d, got: %d\n", TRUE, r);

    r = SendMessageA(hwEdit, WM_CHAR, VK_TAB, 0);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessageA(hwEdit, WM_GETTEXT, 16, (LPARAM) buffer);
    ok(1 == r, "Expected: %d, got len %d\n", 1, r);
    ok(0 == strcmp(buffer, "\t"), "expected \"\\t\", got \"%s\"\n", buffer);

    DestroyWindow (hwEdit);

    /* single line */
    hwEdit = create_editcontrol(0, 0);
    r = get_edit_style(hwEdit);
    ok(0 == r, "Wrong style expected 0x%x got: 0x%x\n", 0, r);

    /* set text */
    r = SendMessageA(hwEdit , WM_SETTEXT, 0, (LPARAM) "");
    ok(TRUE == r, "Expected: %d, got: %d\n", TRUE, r);

    r = SendMessageA(hwEdit, WM_CHAR, VK_TAB, 0);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);

    /* get text */
    buffer[0] = 0;
    r = SendMessageA(hwEdit, WM_GETTEXT, 16, (LPARAM) buffer);
    ok(0 == r, "Expected: %d, got len %d\n", 0, r);
    ok(0 == strcmp(buffer, ""), "expected \"\", got \"%s\"\n", buffer);

    DestroyWindow (hwEdit);
}

static void test_edit_dialog(void)
{
    int r;

    /* from bug 11841 */
    r = DialogBoxParamA(hinst, "EDIT_READONLY_DIALOG", NULL, edit_dialog_proc, 0);
    ok(333 == r, "Expected %d, got %d\n", 333, r);
    r = DialogBoxParamA(hinst, "EDIT_READONLY_DIALOG", NULL, edit_dialog_proc, 1);
    ok(111 == r, "Expected %d, got %d\n", 111, r);
    r = DialogBoxParamA(hinst, "EDIT_READONLY_DIALOG", NULL, edit_dialog_proc, 2);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* more tests for WM_CHAR */
    r = DialogBoxParamA(hinst, "EDIT_READONLY_DIALOG", NULL, edit_dialog_proc, 3);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_READONLY_DIALOG", NULL, edit_dialog_proc, 4);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_READONLY_DIALOG", NULL, edit_dialog_proc, 5);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* more tests for WM_KEYDOWN + WM_CHAR */
    r = DialogBoxParamA(hinst, "EDIT_READONLY_DIALOG", NULL, edit_dialog_proc, 6);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_READONLY_DIALOG", NULL, edit_dialog_proc, 7);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_READONLY_DIALOG", NULL, edit_dialog_proc, 8);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests with an editable edit control */
    r = DialogBoxParamA(hinst, "EDIT_DIALOG", NULL, edit_dialog_proc, 0);
    ok(333 == r, "Expected %d, got %d\n", 333, r);
    r = DialogBoxParamA(hinst, "EDIT_DIALOG", NULL, edit_dialog_proc, 1);
    ok(111 == r, "Expected %d, got %d\n", 111, r);
    r = DialogBoxParamA(hinst, "EDIT_DIALOG", NULL, edit_dialog_proc, 2);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_CHAR */
    r = DialogBoxParamA(hinst, "EDIT_DIALOG", NULL, edit_dialog_proc, 3);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_DIALOG", NULL, edit_dialog_proc, 4);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_DIALOG", NULL, edit_dialog_proc, 5);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_KEYDOWN + WM_CHAR */
    r = DialogBoxParamA(hinst, "EDIT_DIALOG", NULL, edit_dialog_proc, 6);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_DIALOG", NULL, edit_dialog_proc, 7);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_DIALOG", NULL, edit_dialog_proc, 8);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* multiple tab tests */
    r = DialogBoxParamA(hinst, "EDIT_DIALOG", NULL, edit_dialog_proc, 9);
    ok(22 == r, "Expected %d, got %d\n", 22, r);
    r = DialogBoxParamA(hinst, "EDIT_DIALOG", NULL, edit_dialog_proc, 10);
    ok(33 == r, "Expected %d, got %d\n", 33, r);
}

static void test_multi_edit_dialog(void)
{
    int r;

    /* test for multiple edit dialogs (bug 12319) */
    r = DialogBoxParamA(hinst, "MULTI_EDIT_DIALOG", NULL, multi_edit_dialog_proc, 0);
    ok(2222 == r, "Expected %d, got %d\n", 2222, r);
    r = DialogBoxParamA(hinst, "MULTI_EDIT_DIALOG", NULL, multi_edit_dialog_proc, 1);
    ok(1111 == r, "Expected %d, got %d\n", 1111, r);
    r = DialogBoxParamA(hinst, "MULTI_EDIT_DIALOG", NULL, multi_edit_dialog_proc, 2);
    ok(2222 == r, "Expected %d, got %d\n", 2222, r);
    r = DialogBoxParamA(hinst, "MULTI_EDIT_DIALOG", NULL, multi_edit_dialog_proc, 3);
    ok(11 == r, "Expected %d, got %d\n", 11, r);
}

static void test_wantreturn_edit_dialog(void)
{
    int r;

    /* tests for WM_KEYDOWN */
    r = DialogBoxParamA(hinst, "EDIT_WANTRETURN_DIALOG", NULL, edit_wantreturn_dialog_proc, 0);
    ok(333 == r, "Expected %d, got %d\n", 333, r);
    r = DialogBoxParamA(hinst, "EDIT_WANTRETURN_DIALOG", NULL, edit_wantreturn_dialog_proc, 1);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_WANTRETURN_DIALOG", NULL, edit_wantreturn_dialog_proc, 2);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_CHAR */
    r = DialogBoxParamA(hinst, "EDIT_WANTRETURN_DIALOG", NULL, edit_wantreturn_dialog_proc, 3);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_WANTRETURN_DIALOG", NULL, edit_wantreturn_dialog_proc, 4);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_WANTRETURN_DIALOG", NULL, edit_wantreturn_dialog_proc, 5);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_KEYDOWN + WM_CHAR */
    r = DialogBoxParamA(hinst, "EDIT_WANTRETURN_DIALOG", NULL, edit_wantreturn_dialog_proc, 6);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_WANTRETURN_DIALOG", NULL, edit_wantreturn_dialog_proc, 7);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_WANTRETURN_DIALOG", NULL, edit_wantreturn_dialog_proc, 8);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
}

static void test_singleline_wantreturn_edit_dialog(void)
{
    int r;

    /* tests for WM_KEYDOWN */
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_DIALOG", NULL, edit_singleline_dialog_proc, 0);
    ok(222 == r, "Expected %d, got %d\n", 222, r);
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_DIALOG", NULL, edit_singleline_dialog_proc, 1);
    ok(111 == r, "Expected %d, got %d\n", 111, r);
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_DIALOG", NULL, edit_singleline_dialog_proc, 2);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_CHAR */
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_DIALOG", NULL, edit_singleline_dialog_proc, 3);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_DIALOG", NULL, edit_singleline_dialog_proc, 4);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_DIALOG", NULL, edit_singleline_dialog_proc, 5);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_KEYDOWN + WM_CHAR */
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_DIALOG", NULL, edit_singleline_dialog_proc, 6);
    ok(222 == r, "Expected %d, got %d\n", 222, r);
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_DIALOG", NULL, edit_singleline_dialog_proc, 7);
    ok(111 == r, "Expected %d, got %d\n", 111, r);
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_DIALOG", NULL, edit_singleline_dialog_proc, 8);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_KEYDOWN */
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, edit_singleline_dialog_proc, 0);
    ok(222 == r, "Expected %d, got %d\n", 222, r);
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, edit_singleline_dialog_proc, 1);
    ok(111 == r, "Expected %d, got %d\n", 111, r);
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, edit_singleline_dialog_proc, 2);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_CHAR */
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, edit_singleline_dialog_proc, 3);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, edit_singleline_dialog_proc, 4);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, edit_singleline_dialog_proc, 5);
    ok(444 == r, "Expected %d, got %d\n", 444, r);

    /* tests for WM_KEYDOWN + WM_CHAR */
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, edit_singleline_dialog_proc, 6);
    ok(222 == r, "Expected %d, got %d\n", 222, r);
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, edit_singleline_dialog_proc, 7);
    ok(111 == r, "Expected %d, got %d\n", 111, r);
    r = DialogBoxParamA(hinst, "EDIT_SINGLELINE_WANTRETURN_DIALOG", NULL, edit_singleline_dialog_proc, 8);
    ok(444 == r, "Expected %d, got %d\n", 444, r);
}

static int child_edit_wmkeydown_num_messages = 0;
static INT_PTR CALLBACK child_edit_wmkeydown_proc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_DESTROY:
        case WM_NCDESTROY:
            break;

        default:
            child_edit_wmkeydown_num_messages++;
            break;
    }

    return FALSE;
}

static void test_child_edit_wmkeydown(void)
{
    HWND hwEdit, hwParent;
    int r;

    hwEdit = create_child_editcontrol(0, 0);
    hwParent = GetParent(hwEdit);
    SetWindowLongPtrA(hwParent, GWLP_WNDPROC, (LONG_PTR)child_edit_wmkeydown_proc);
    r = SendMessageA(hwEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(1 == r, "expected 1, got %d\n", r);
    ok(0 == child_edit_wmkeydown_num_messages, "expected 0, got %d\n", child_edit_wmkeydown_num_messages);
    destroy_child_editcontrol(hwEdit);
}

static int got_en_setfocus = 0;
static int got_wm_capturechanged = 0;
static LRESULT (CALLBACK *p_edit_proc)(HWND, UINT, WPARAM, LPARAM);

static LRESULT CALLBACK edit4_wnd_procA(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_COMMAND:
            switch (HIWORD(wParam)) {
                case EN_SETFOCUS:
                    got_en_setfocus = 1;
                    break;
            }
            break;
        case WM_CAPTURECHANGED:
            if (hWnd != (HWND)lParam)
            {
                got_wm_capturechanged = 1;
                pEndMenu();
            }
            break;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

static LRESULT CALLBACK edit_proc_proxy(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_ENTERIDLE: {
            MENUBARINFO mbi;
            BOOL ret;
            HWND ctx_menu = (HWND)lParam;

            memset(&mbi, 0, sizeof(mbi));
            mbi.cbSize = sizeof(mbi);
            SetLastError(0xdeadbeef);
            ret = pGetMenuBarInfo(ctx_menu, OBJID_CLIENT, 0, &mbi);
            ok(ret || broken(!ret && GetLastError()==ERROR_INVALID_WINDOW_HANDLE) /* NT */,
                    "GetMenuBarInfo failed\n");
            if (ret)
            {
                ok(mbi.hMenu != NULL, "mbi.hMenu = NULL\n");
                ok(!mbi.hwndMenu, "mbi.hwndMenu != NULL\n");
                ok(mbi.fBarFocused, "mbi.fBarFocused = FALSE\n");
                ok(mbi.fFocused, "mbi.fFocused = FALSE\n");
            }

            memset(&mbi, 0, sizeof(mbi));
            mbi.cbSize = sizeof(mbi);
            SetLastError(0xdeadbeef);
            ret = pGetMenuBarInfo(ctx_menu, OBJID_CLIENT, 1, &mbi);
            ok(ret || broken(!ret && GetLastError()==ERROR_INVALID_WINDOW_HANDLE) /* NT */,
                    "GetMenuBarInfo failed\n");
            if (ret)
            {
                ok(mbi.hMenu != NULL, "mbi.hMenu = NULL\n");
                ok(!mbi.hwndMenu, "mbi.hwndMenu != NULL\n");
                ok(mbi.fBarFocused, "mbi.fBarFocused = FALSE\n");
                ok(!mbi.fFocused, "mbi.fFocused = TRUE\n");
            }

            pEndMenu();
            break;
        }
    }
    return p_edit_proc(hWnd, msg, wParam, lParam);
}

static void test_contextmenu(void)
{
    HWND hwndMain, hwndEdit;

    hwndMain = CreateWindowA(szEditTest4Class, "ET4", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                            0, 0, 200, 200, NULL, NULL, hinst, NULL);
    assert(hwndMain);

    hwndEdit = CreateWindowA("EDIT", NULL,
                           WS_CHILD|WS_BORDER|WS_VISIBLE|ES_LEFT|ES_AUTOHSCROLL,
                           0, 0, 150, 50, /* important this not be 0 size. */
                           hwndMain, (HMENU) ID_EDITTEST2, hinst, NULL);
    assert(hwndEdit);

    SetFocus(NULL);
    SetCapture(hwndMain);
    SendMessageA(hwndEdit, WM_CONTEXTMENU, (WPARAM)hwndEdit, MAKEWORD(10, 10));
    ok(got_en_setfocus, "edit box didn't get focused\n");
    ok(got_wm_capturechanged, "main window capture did not change\n");

    if (pGetMenuBarInfo)
    {
        p_edit_proc = (void*)SetWindowLongPtrA(hwndEdit, GWLP_WNDPROC, (ULONG_PTR)edit_proc_proxy);
        SendMessageA(hwndEdit, WM_CONTEXTMENU, (WPARAM)hwndEdit, MAKEWORD(10, 10));
    }

    DestroyWindow (hwndEdit);
    DestroyWindow (hwndMain);
}

static BOOL RegisterWindowClasses (void)
{
    WNDCLASSA test2;
    WNDCLASSA test3;
    WNDCLASSA test4;
    WNDCLASSA text_position;

    test2.style = 0;
    test2.lpfnWndProc = ET2_WndProc;
    test2.cbClsExtra = 0;
    test2.cbWndExtra = 0;
    test2.hInstance = hinst;
    test2.hIcon = NULL;
    test2.hCursor = LoadCursorA (NULL, (LPCSTR)IDC_ARROW);
    test2.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    test2.lpszMenuName = NULL;
    test2.lpszClassName = szEditTest2Class;
    if (!RegisterClassA(&test2)) return FALSE;

    test3.style = 0;
    test3.lpfnWndProc = edit3_wnd_procA;
    test3.cbClsExtra = 0;
    test3.cbWndExtra = 0;
    test3.hInstance = hinst;
    test3.hIcon = 0;
    test3.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    test3.hbrBackground = GetStockObject(WHITE_BRUSH);
    test3.lpszMenuName = NULL;
    test3.lpszClassName = szEditTest3Class;
    if (!RegisterClassA(&test3)) return FALSE;

    test4.style = 0;
    test4.lpfnWndProc = edit4_wnd_procA;
    test4.cbClsExtra = 0;
    test4.cbWndExtra = 0;
    test4.hInstance = hinst;
    test4.hIcon = NULL;
    test4.hCursor = LoadCursorA (NULL, (LPCSTR)IDC_ARROW);
    test4.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    test4.lpszMenuName = NULL;
    test4.lpszClassName = szEditTest4Class;
    if (!RegisterClassA(&test4)) return FALSE;

    text_position.style = CS_HREDRAW | CS_VREDRAW;
    text_position.cbClsExtra = 0;
    text_position.cbWndExtra = 0;
    text_position.hInstance = hinst;
    text_position.hIcon = NULL;
    text_position.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    text_position.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    text_position.lpszMenuName = NULL;
    text_position.lpszClassName = szEditTextPositionClass;
    text_position.lpfnWndProc = DefWindowProcA;
    if (!RegisterClassA(&text_position)) return FALSE;

    return TRUE;
}

static void UnregisterWindowClasses (void)
{
    UnregisterClassA(szEditTest2Class, hinst);
    UnregisterClassA(szEditTest3Class, hinst);
    UnregisterClassA(szEditTest4Class, hinst);
    UnregisterClassA(szEditTextPositionClass, hinst);
}

static void test_fontsize(void)
{
    HWND hwEdit;
    HFONT hfont;
    HDC hDC;
    LOGFONTA lf;
    LONG r;
    char szLocalString[MAXLEN];
    int dpi;

    hDC = GetDC(NULL);
    dpi = GetDeviceCaps(hDC, LOGPIXELSY);
    ReleaseDC(NULL, hDC);

    memset(&lf,0,sizeof(LOGFONTA));
    strcpy(lf.lfFaceName,"Arial");
    lf.lfHeight = -300; /* taller than the edit box */
    lf.lfWeight = 500;
    hfont = CreateFontIndirectA(&lf);

    trace("EDIT: Oversized font (Multi line)\n");
    hwEdit= CreateWindowA("EDIT", NULL, ES_MULTILINE|ES_AUTOHSCROLL,
                           0, 0, (150 * dpi) / 96, (50 * dpi) / 96, NULL, NULL,
                           hinst, NULL);

    SendMessageA(hwEdit,WM_SETFONT,(WPARAM)hfont,0);

    if (winetest_interactive)
        ShowWindow (hwEdit, SW_SHOW);

    r = SendMessageA(hwEdit, WM_CHAR, 'A', 1);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);
    r = SendMessageA(hwEdit, WM_CHAR, 'B', 1);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);
    r = SendMessageA(hwEdit, WM_CHAR, 'C', 1);
    ok(1 == r, "Expected: %d, got: %d\n", 1, r);

    GetWindowTextA(hwEdit, szLocalString, MAXLEN);
    ok(strcmp(szLocalString, "ABC")==0,
       "Wrong contents of edit: %s\n", szLocalString);

    r = SendMessageA(hwEdit, EM_POSFROMCHAR,0,0);
    ok(r != -1,"EM_POSFROMCHAR failed index 0\n");
    r = SendMessageA(hwEdit, EM_POSFROMCHAR,1,0);
    ok(r != -1,"EM_POSFROMCHAR failed index 1\n");
    r = SendMessageA(hwEdit, EM_POSFROMCHAR,2,0);
    ok(r != -1,"EM_POSFROMCHAR failed index 2\n");
    r = SendMessageA(hwEdit, EM_POSFROMCHAR,3,0);
    ok(r == -1,"EM_POSFROMCHAR succeeded index 3\n");

    DestroyWindow (hwEdit);
    DeleteObject(hfont);
}

struct dialog_mode_messages
{
    int wm_getdefid, wm_close, wm_command, wm_nextdlgctl;
};

static struct dialog_mode_messages dm_messages;

static void zero_dm_messages(void)
{
    dm_messages.wm_command      = 0;
    dm_messages.wm_close        = 0;
    dm_messages.wm_getdefid     = 0;
    dm_messages.wm_nextdlgctl   = 0;
}

#define test_dm_messages(wmcommand, wmclose, wmgetdefid, wmnextdlgctl) \
    ok(dm_messages.wm_command == wmcommand, "expected %d WM_COMMAND messages, " \
    "got %d\n", wmcommand, dm_messages.wm_command); \
    ok(dm_messages.wm_close == wmclose, "expected %d WM_CLOSE messages, " \
    "got %d\n", wmclose, dm_messages.wm_close); \
    ok(dm_messages.wm_getdefid == wmgetdefid, "expected %d WM_GETDIFID messages, " \
    "got %d\n", wmgetdefid, dm_messages.wm_getdefid);\
    ok(dm_messages.wm_nextdlgctl == wmnextdlgctl, "expected %d WM_NEXTDLGCTL messages, " \
    "got %d\n", wmnextdlgctl, dm_messages.wm_nextdlgctl)

static LRESULT CALLBACK dialog_mode_wnd_proc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch (iMsg)
    {
        case WM_COMMAND:
            dm_messages.wm_command++;
            break;
        case DM_GETDEFID:
            dm_messages.wm_getdefid++;
            return MAKELONG(ID_EDITTESTDBUTTON, DC_HASDEFID);
        case WM_NEXTDLGCTL:
            dm_messages.wm_nextdlgctl++;
            break;
        case WM_CLOSE:
            dm_messages.wm_close++;
            break;
    }

    return DefWindowProcA(hwnd, iMsg, wParam, lParam);
}

static void test_dialogmode(void)
{
    HWND hwEdit, hwParent, hwButton;
    MSG msg= {0};
    int len, r;
    hwEdit = create_child_editcontrol(ES_MULTILINE, 0);

    r = SendMessageA(hwEdit, WM_CHAR, VK_RETURN, 0x1c0001);
    ok(1 == r, "expected 1, got %d\n", r);
    len = SendMessageA(hwEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(11 == len, "expected 11, got %d\n", len);

    r = SendMessageA(hwEdit, WM_GETDLGCODE, 0, 0);
    ok(0x8d == r, "expected 0x8d, got 0x%x\n", r);

    r = SendMessageA(hwEdit, WM_CHAR, VK_RETURN, 0x1c0001);
    ok(1 == r, "expected 1, got %d\n", r);
    len = SendMessageA(hwEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(13 == len, "expected 13, got %d\n", len);

    r = SendMessageA(hwEdit, WM_GETDLGCODE, 0, (LPARAM)&msg);
    ok(0x8d == r, "expected 0x8d, got 0x%x\n", r);
    r = SendMessageA(hwEdit, WM_CHAR, VK_RETURN, 0x1c0001);
    ok(1 == r, "expected 1, got %d\n", r);
    len = SendMessageA(hwEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(13 == len, "expected 13, got %d\n", len);

    r = SendMessageA(hwEdit, WM_CHAR, VK_RETURN, 0x1c0001);
    ok(1 == r, "expected 1, got %d\n", r);
    len = SendMessageA(hwEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(13 == len, "expected 13, got %d\n", len);

    destroy_child_editcontrol(hwEdit);

    hwEdit = create_editcontrol(ES_MULTILINE, 0);

    r = SendMessageA(hwEdit, WM_CHAR, VK_RETURN, 0x1c0001);
    ok(1 == r, "expected 1, got %d\n", r);
    len = SendMessageA(hwEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(11 == len, "expected 11, got %d\n", len);

    msg.hwnd = hwEdit;
    msg.message = WM_KEYDOWN;
    msg.wParam = VK_BACK;
    msg.lParam = 0xe0001;
    r = SendMessageA(hwEdit, WM_GETDLGCODE, VK_BACK, (LPARAM)&msg);
    ok(0x8d == r, "expected 0x8d, got 0x%x\n", r);

    r = SendMessageA(hwEdit, WM_CHAR, VK_RETURN, 0x1c0001);
    ok(1 == r, "expected 1, got %d\n", r);
    len = SendMessageA(hwEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(11 == len, "expected 11, got %d\n", len);

    DestroyWindow(hwEdit);

    hwEdit = create_child_editcontrol(0, 0);
    hwParent = GetParent(hwEdit);
    SetWindowLongPtrA(hwParent, GWLP_WNDPROC, (LONG_PTR)dialog_mode_wnd_proc);

    zero_dm_messages();
    r = SendMessageA(hwEdit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
    ok(1 == r, "expected 1, got %d\n", r);
    test_dm_messages(0, 0, 0, 0);
    zero_dm_messages();

    r = SendMessageA(hwEdit, WM_KEYDOWN, VK_TAB, 0xf0001);
    ok(1 == r, "expected 1, got %d\n", r);
    test_dm_messages(0, 0, 0, 0);
    zero_dm_messages();

    msg.hwnd = hwEdit;
    msg.message = WM_KEYDOWN;
    msg.wParam = VK_TAB;
    msg.lParam = 0xf0001;
    r = SendMessageA(hwEdit, WM_GETDLGCODE, VK_TAB, (LPARAM)&msg);
    ok(0x89 == r, "expected 0x89, got 0x%x\n", r);
    test_dm_messages(0, 0, 0, 0);
    zero_dm_messages();

    r = SendMessageA(hwEdit, WM_KEYDOWN, VK_TAB, 0xf0001);
    ok(1 == r, "expected 1, got %d\n", r);
    test_dm_messages(0, 0, 0, 0);
    zero_dm_messages();

    destroy_child_editcontrol(hwEdit);

    hwEdit = create_child_editcontrol(ES_MULTILINE, 0);
    hwParent = GetParent(hwEdit);
    SetWindowLongPtrA(hwParent, GWLP_WNDPROC, (LONG_PTR)dialog_mode_wnd_proc);

    r = SendMessageA(hwEdit, WM_KEYDOWN, VK_TAB, 0xf0001);
    ok(1 == r, "expected 1, got %d\n", r);
    test_dm_messages(0, 0, 0, 0);
    zero_dm_messages();

    msg.hwnd = hwEdit;
    msg.message = WM_KEYDOWN;
    msg.wParam = VK_ESCAPE;
    msg.lParam = 0x10001;
    r = SendMessageA(hwEdit, WM_GETDLGCODE, VK_ESCAPE, (LPARAM)&msg);
    ok(0x8d == r, "expected 0x8d, got 0x%x\n", r);
    test_dm_messages(0, 0, 0, 0);
    zero_dm_messages();

    r = SendMessageA(hwEdit, WM_KEYDOWN, VK_ESCAPE, 0x10001);
    ok(1 == r, "expected 1, got %d\n", r);
    test_dm_messages(0, 0, 0, 0);
    zero_dm_messages();

    r = SendMessageA(hwEdit, WM_KEYDOWN, VK_TAB, 0xf0001);
    ok(1 == r, "expected 1, got %d\n", r);
    test_dm_messages(0, 0, 0, 1);
    zero_dm_messages();

    r = SendMessageA(hwEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(1 == r, "expected 1, got %d\n", r);
    test_dm_messages(0, 0, 1, 0);
    zero_dm_messages();

    hwButton = CreateWindowA("BUTTON", "OK", WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON,
        100, 100, 50, 20, hwParent, (HMENU)ID_EDITTESTDBUTTON, hinst, NULL);
    ok(hwButton!=NULL, "CreateWindow failed with error code %d\n", GetLastError());

    r = SendMessageA(hwEdit, WM_KEYDOWN, VK_RETURN, 0x1c0001);
    ok(1 == r, "expected 1, got %d\n", r);
    test_dm_messages(0, 0, 1, 1);
    zero_dm_messages();

    DestroyWindow(hwButton);
    destroy_child_editcontrol(hwEdit);
}

static void test_EM_GETHANDLE(void)
{
    static const char str0[] = "untouched";
    static const char str1[] = "1111+1111+1111#";
    static const char str2[] = "2222-2222-2222-2222#";
    static const char str3[] = "3333*3333*3333*3333*3333#";
    CHAR    current[42];
    HWND    hEdit;
    HLOCAL  hmem;
    HLOCAL  hmem2;
    HLOCAL  halloc;
    char    *buffer;
    int     len;
    int     r;

    trace("EDIT: EM_GETHANDLE\n");

    /* EM_GETHANDLE is not supported for a single line edit control */
    hEdit = create_editcontrol(WS_BORDER, 0);
    ok(hEdit != NULL, "got %p (expected != NULL)\n", hEdit);

    hmem = (HGLOBAL) SendMessageA(hEdit, EM_GETHANDLE, 0, 0);
    ok(hmem == NULL, "got %p (expected NULL)\n", hmem);
    DestroyWindow(hEdit);


    /* EM_GETHANDLE needs a multiline edit control */
    hEdit = create_editcontrol(WS_BORDER | ES_MULTILINE, 0);
    ok(hEdit != NULL, "got %p (expected != NULL)\n", hEdit);

    /* set some text */
    r = SendMessageA(hEdit, WM_SETTEXT, 0, (LPARAM)str1);
    len = SendMessageA(hEdit, WM_GETTEXTLENGTH, 0, 0);
    ok((r == 1) && (len == lstrlenA(str1)), "got %d and %d (expected 1 and %d)\n", r, len, lstrlenA(str1));

    lstrcpyA(current, str0);
    r = SendMessageA(hEdit, WM_GETTEXT, sizeof(current), (LPARAM)current);
    ok((r == lstrlenA(str1)) && !lstrcmpA(current, str1),
        "got %d and \"%s\" (expected %d and \"%s\")\n", r, current, lstrlenA(str1), str1);

    hmem = (HGLOBAL) SendMessageA(hEdit, EM_GETHANDLE, 0, 0);
    ok(hmem != NULL, "got %p (expected != NULL)\n", hmem);
    /* The buffer belongs to the app now. According to MSDN, the app has to LocalFree the
       buffer, LocalAlloc a new buffer and pass it to the edit control with EM_SETHANDLE. */

    buffer = LocalLock(hmem);
    ok(buffer != NULL, "got %p (expected != NULL)\n", buffer);
    len = lstrlenA(buffer);
    ok((len == lstrlenA(str1)) && !lstrcmpA(buffer, str1),
        "got %d and \"%s\" (expected %d and \"%s\")\n", len, buffer, lstrlenA(str1), str1);
    LocalUnlock(hmem);

    /* use LocalAlloc first to get a different handle */
    halloc = LocalAlloc(LMEM_MOVEABLE, 42);
    ok(halloc != NULL, "got %p (expected != NULL)\n", halloc);
    /* prepare our new memory */
    buffer = LocalLock(halloc);
    ok(buffer != NULL, "got %p (expected != NULL)\n", buffer);
    lstrcpyA(buffer, str2);
    LocalUnlock(halloc);

    /* LocalFree the old memory handle before EM_SETHANDLE the new handle */
    LocalFree(hmem);
    /* use LocalAlloc after the LocalFree to likely consume the handle */
    hmem2 = LocalAlloc(LMEM_MOVEABLE, 42);
    ok(hmem2 != NULL, "got %p (expected != NULL)\n", hmem2);

    SendMessageA(hEdit, EM_SETHANDLE, (WPARAM)halloc, 0);

    len = SendMessageA(hEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(len == lstrlenA(str2), "got %d (expected %d)\n", len, lstrlenA(str2));

    lstrcpyA(current, str0);
    r = SendMessageA(hEdit, WM_GETTEXT, sizeof(current), (LPARAM)current);
    ok((r == lstrlenA(str2)) && !lstrcmpA(current, str2),
        "got %d and \"%s\" (expected %d and \"%s\")\n", r, current, lstrlenA(str2), str2);

    /* set a different text */
    r = SendMessageA(hEdit, WM_SETTEXT, 0, (LPARAM)str3);
    len = SendMessageA(hEdit, WM_GETTEXTLENGTH, 0, 0);
    ok((r == 1) && (len == lstrlenA(str3)), "got %d and %d (expected 1 and %d)\n", r, len, lstrlenA(str3));

    lstrcpyA(current, str0);
    r = SendMessageA(hEdit, WM_GETTEXT, sizeof(current), (LPARAM)current);
    ok((r == lstrlenA(str3)) && !lstrcmpA(current, str3),
        "got %d and \"%s\" (expected %d and \"%s\")\n", r, current, lstrlenA(str3), str3);

    LocalFree(hmem2);
    DestroyWindow(hEdit);

    /* Some apps have bugs ... */
    hEdit = create_editcontrol(WS_BORDER | ES_MULTILINE, 0);

    /* set some text */
    r = SendMessageA(hEdit, WM_SETTEXT, 0, (LPARAM)str1);
    len = SendMessageA(hEdit, WM_GETTEXTLENGTH, 0, 0);
    ok((r == 1) && (len == lstrlenA(str1)), "got %d and %d (expected 1 and %d)\n", r, len, lstrlenA(str1));

    /* everything is normal up to EM_GETHANDLE */
    hmem = (HGLOBAL) SendMessageA(hEdit, EM_GETHANDLE, 0, 0);
    /* Some messages still work while other messages fail.
       After LocalFree the memory handle, messages can crash the app */

    /* A buggy editor used EM_GETHANDLE twice */
    hmem2 = (HGLOBAL) SendMessageA(hEdit, EM_GETHANDLE, 0, 0);
    ok(hmem2 == hmem, "got %p (expected %p)\n", hmem2, hmem);

    /* Let the edit control free the memory handle */
    SendMessageA(hEdit, EM_SETHANDLE, (WPARAM)hmem2, 0);

    DestroyWindow(hEdit);
}


START_TEST(edit)
{
    BOOL b;

    init_function_pointers();

    hinst = GetModuleHandleA(NULL);
    b = RegisterWindowClasses();
    ok (b, "RegisterWindowClasses failed\n");
    if (!b) return;

    test_edit_control_1();
    test_edit_control_2();
    test_edit_control_3();
    test_edit_control_4();
    test_edit_control_5();
    test_edit_control_6();
    test_edit_control_limittext();
    test_edit_control_scroll();
    test_margins();
    test_margins_font_change();
    test_text_position();
    test_espassword();
    test_undo();
    test_enter();
    test_tab();
    test_edit_dialog();
    test_multi_edit_dialog();
    test_wantreturn_edit_dialog();
    test_singleline_wantreturn_edit_dialog();
    test_child_edit_wmkeydown();
    test_fontsize();
    test_dialogmode();
    if (pEndMenu)
        test_contextmenu();
    else
        win_skip("EndMenu is not available\n");

    test_EM_GETHANDLE();

    UnregisterWindowClasses();
}

/*
 * PROJECT:         ReactOS
 * LICENSE:         LGPL
 * FILE:            dll/win32/winent.drv/wnd.c
 * PURPOSE:         Windows related functions
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <stdarg.h>
#include <stdio.h>
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#define NTOS_USER_MODE
#include <ndk/ntndk.h>
#include <winddi.h>
#include <win32k/ntgdityp.h>
#include "ntrosgdi.h"
#include "wine/rosuser.h"
#include "wine/list.h"
#include "winent.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rosuserdrv);

static struct list wnd_data_list = LIST_INIT( wnd_data_list );
static CRITICAL_SECTION wnd_data_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &wnd_data_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": wnd_data_cs") }
};
static CRITICAL_SECTION wnd_data_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

/* FUNCTIONS **************************************************************/

VOID CDECL RosDrv_UpdateZOrder(HWND hwnd, RECT *rect)
{
        FIXME("hwnd %x rect (%d, %d)-(%d,%d)\n", hwnd,
            rect->top, rect->left, rect->bottom, rect->right);

        SERVER_START_REQ( update_window_zorder )
        {
            req->window      = wine_server_user_handle( hwnd );
            req->rect.left   = rect->left;
            req->rect.top    = rect->top;
            req->rect.right  = rect->right;
            req->rect.bottom = rect->bottom;
            wine_server_call( req );
        }
        SERVER_END_REQ;
}

struct ntdrv_win_data *associate_create( HWND hwnd )
{
    struct ntdrv_win_data *data;

    /* Insert our mapping */
    data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct ntdrv_win_data));
    if(!data) return NULL;

    data->hwnd = hwnd;

    EnterCriticalSection(&wnd_data_cs);
    list_add_tail(&wnd_data_list, &data->entry);
    LeaveCriticalSection(&wnd_data_cs);

    return data;
}

VOID associate_destroy( struct ntdrv_win_data *data )
{
    EnterCriticalSection(&wnd_data_cs);
    list_remove( &data->entry );
    LeaveCriticalSection(&wnd_data_cs);

    HeapFree( GetProcessHeap(), 0, data);
}

struct ntdrv_win_data *associate_find( HWND hwnd )
{
    struct ntdrv_win_data *item;
    BOOL found = FALSE;

    EnterCriticalSection(&wnd_data_cs);

    LIST_FOR_EACH_ENTRY( item, &wnd_data_list, struct ntdrv_win_data, entry )
        if (item->hwnd == hwnd)
        {
            found = TRUE;
            break;
        }

    LeaveCriticalSection(&wnd_data_cs);

    if (!found) item = NULL;

    return item;
}

/***********************************************************************
 *		NTDRV_get_win_data
 *
 * Return the private data structure associated with a window.
 */
struct ntdrv_win_data *NTDRV_get_win_data( HWND hwnd )
{
    struct ntdrv_win_data *data;

    if (!hwnd) return NULL;

    data = associate_find( hwnd );

    return data;
}


/***********************************************************************
 *		NTDRV_create_win_data
 *
 * Create a private data window structure for an existing window.
 */
struct ntdrv_win_data *NTDRV_create_win_data( HWND hwnd )
{
    struct ntdrv_win_data *data;
    DWORD style, ex_style;
    HWND parent;

    if (!(parent = GetAncestor( hwnd, GA_PARENT ))) return NULL;  /* desktop */

    /* don't create win data for HWND_MESSAGE windows */
    if (parent != GetDesktopWindow() && !GetAncestor( parent, GA_PARENT )) return NULL;

    data = associate_create(hwnd);
    if (!data) return NULL;

    GetWindowRect( hwnd, &data->window_rect );
    MapWindowPoints( 0, parent, (POINT *)&data->window_rect, 2 );
    data->whole_rect = data->window_rect;
    GetClientRect( hwnd, &data->client_rect );
    MapWindowPoints( hwnd, parent, (POINT *)&data->client_rect, 2 );

    if (parent == GetDesktopWindow())
    {
        TRACE( "win %p window %s whole %s client %s\n",
               hwnd, wine_dbgstr_rect( &data->window_rect ),
               wine_dbgstr_rect( &data->whole_rect ), wine_dbgstr_rect( &data->client_rect ));

        style = GetWindowLongW( data->hwnd, GWL_STYLE );
        ex_style = GetWindowLongW( data->hwnd, GWL_EXSTYLE );

        /* Inform window manager about window rect in screen coords */
        SwmAddWindow(hwnd, &data->window_rect, style, ex_style);
        data->whole_window = (PVOID)1;
    }

    return data;
}

/* initialize the desktop window id in the desktop manager process */
struct ntdrv_win_data *NTDRV_create_desktop_win_data( HWND hwnd )
{
    struct ntdrv_win_data *data;

    data = associate_create(hwnd);
    if (!data) return NULL;

    /* Mark it as being a whole window */
    data->whole_window = (PVOID)1;

    return data;
}

/***********************************************************************
 *		NTDRV_destroy_win_data
 *
 * Deletes a private data window structure for an existing window.
 */
void NTDRV_destroy_win_data( HWND hwnd )
{
    struct ntdrv_win_data *data = NTDRV_get_win_data(hwnd);
    if (!data) return;

    /* Remove property and free its data */
    associate_destroy( data );

    /* Inform window manager */
    SwmRemoveWindow( hwnd );
}


/* EOF */

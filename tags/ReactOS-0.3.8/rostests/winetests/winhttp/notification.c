/*
 * test status notifications
 *
 * Copyright 2008 Hans Leidekker for CodeWeavers
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

#include <stdarg.h>
#include <stdlib.h>
#include <windef.h>
#include <winbase.h>
#include <winhttp.h>

#include "wine/test.h"

static const WCHAR user_agent[] = {'w','i','n','e','t','e','s','t',0};

enum api
{
    winhttp_connect = 1,
    winhttp_open_request,
    winhttp_send_request,
    winhttp_receive_response,
    winhttp_close_handle
};

struct notification
{
    enum api function;      /* api responsible for notification */
    unsigned int status;    /* status received */
    int todo;
};

struct info
{
    enum api function;
    const struct notification *test;
    unsigned int count;
    unsigned int index;
};

static void CALLBACK check_notification( HINTERNET handle, DWORD_PTR context, DWORD status, LPVOID buffer, DWORD buflen )
{
    struct info *info = (struct info *)context;
    unsigned int i = info->index;

    if (status == WINHTTP_CALLBACK_STATUS_HANDLE_CREATED)
    {
        DWORD size = sizeof(struct info *);
        WinHttpQueryOption( handle, WINHTTP_OPTION_CONTEXT_VALUE, &info, &size );
    }
    ok(i < info->count, "unexpected notification 0x%08x\n", status);
    if (i >= info->count) return;
    if (!info->test[i].todo)
    {
        ok(info->test[i].status == status, "expected status 0x%08x got 0x%08x\n", info->test[i].status, status);
        ok(info->test[i].function == info->function, "expected function %u got %u\n", info->test[i].function, info->function);
    }
    else todo_wine
    {
        ok(info->test[i].status == status, "expected status 0x%08x got 0x%08x\n", info->test[i].status, status);
        ok(info->test[i].function == info->function, "expected function %u got %u\n", info->test[i].function, info->function);
    }
    info->index++;
}

static const struct notification cache_test[] =
{
    { winhttp_connect,          WINHTTP_CALLBACK_STATUS_HANDLE_CREATED, 0 },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED, 0 },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, 0 },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, 0 },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, 0 },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, 0 },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST, 0 },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT, 0 },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, 0 },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, 0 },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION, 0 },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED, 0 },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, 0 },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, 1 },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, 1 }
};

static void test_connection_cache( void )
{
    static const WCHAR codeweavers[] = {'w','w','w','.','c','o','d','e','w','e','a','v','e','r','s','.','c','o','m',0};

    HANDLE ses, con, req;
    DWORD size, status;
    BOOL ret;
    struct info info, *context = &info;

    info.test  = cache_test;
    info.count = sizeof(cache_test) / sizeof(cache_test[0]);
    info.index = 0;

    ses = WinHttpOpen( user_agent, 0, NULL, NULL, 0 );
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    WinHttpSetStatusCallback( ses, check_notification, WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS | WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );

    ret = WinHttpSetOption( ses, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok(ret, "failed to set context value %u\n", GetLastError());

    info.function = winhttp_connect;
    con = WinHttpConnect( ses, codeweavers, 0, 0 );
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    info.function = winhttp_open_request;
    req = WinHttpOpenRequest( con, NULL, NULL, NULL, NULL, NULL, 0 );
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    info.function = winhttp_send_request;
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    ok(ret, "failed to send request %u\n", GetLastError());

    info.function = winhttp_receive_response;
    ret = WinHttpReceiveResponse( req, NULL );
    ok(ret, "failed to receive response %u\n", GetLastError());

    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok(ret, "failed unexpectedly %u\n", GetLastError());
    ok(status == 200, "request failed unexpectedly %u\n", status);

    info.function = winhttp_close_handle;
    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );
    WinHttpCloseHandle( ses );

    Sleep(2000); /* make sure connection is evicted from cache */

    info.index = 0;

    ses = WinHttpOpen( user_agent, 0, NULL, NULL, 0 );
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    WinHttpSetStatusCallback( ses, check_notification, WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS | WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );

    ret = WinHttpSetOption( ses, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok(ret, "failed to set context value %u\n", GetLastError());

    info.function = winhttp_connect;
    con = WinHttpConnect( ses, codeweavers, 0, 0 );
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    info.function = winhttp_open_request;
    req = WinHttpOpenRequest( con, NULL, NULL, NULL, NULL, NULL, 0 );
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSetOption( req, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok(ret, "failed to set context value %u\n", GetLastError());

    info.function = winhttp_send_request;
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    ok(ret, "failed to send request %u\n", GetLastError());

    info.function = winhttp_receive_response;
    ret = WinHttpReceiveResponse( req, NULL );
    ok(ret, "failed to receive response %u\n", GetLastError());

    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok(ret, "failed unexpectedly %u\n", GetLastError());
    ok(status == 200, "request failed unexpectedly %u\n", status);

    info.function = winhttp_close_handle;
    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );
    WinHttpCloseHandle( ses );
}

static const struct notification redirect_test[] =
{
    { winhttp_connect,          WINHTTP_CALLBACK_STATUS_HANDLE_CREATED, 0 },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED, 0 },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, 0 },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, 0 },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, 0 },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, 0 },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST, 0 },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT, 0 },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, 0 },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, 0 },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_REDIRECT, 0 },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, 0 },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, 0 },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, 0 },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, 0 },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_SENDING_REQUEST, 0 },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_REQUEST_SENT, 0 },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, 0 },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, 0 },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION, 0 },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED, 0 },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, 0 },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, 1 },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, 1 }
};

static void test_redirect( void )
{
    static const WCHAR codeweavers[] = {'c','o','d','e','w','e','a','v','e','r','s','.','c','o','m',0};

    HANDLE ses, con, req;
    DWORD size, status;
    BOOL ret;
    struct info info, *context = &info;

    info.test  = redirect_test;
    info.count = sizeof(redirect_test) / sizeof(redirect_test[0]);
    info.index = 0;

    ses = WinHttpOpen( user_agent, 0, NULL, NULL, 0 );
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    WinHttpSetStatusCallback( ses, check_notification, WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS | WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );

    ret = WinHttpSetOption( ses, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok(ret, "failed to set context value %u\n", GetLastError());

    info.function = winhttp_connect;
    con = WinHttpConnect( ses, codeweavers, 0, 0 );
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    info.function = winhttp_open_request;
    req = WinHttpOpenRequest( con, NULL, NULL, NULL, NULL, NULL, 0 );
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    info.function = winhttp_send_request;
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );

    info.function = winhttp_receive_response;
    ret = WinHttpReceiveResponse( req, NULL );
    ok(ret, "failed to receive response %u\n", GetLastError());

    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok(ret, "failed unexpectedly %u\n", GetLastError());
    ok(status == 200, "request failed unexpectedly %u\n", status);

    info.function = winhttp_close_handle;
    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );
    WinHttpCloseHandle( ses );
}

START_TEST (notification)
{
    test_connection_cache();
    test_redirect();
}

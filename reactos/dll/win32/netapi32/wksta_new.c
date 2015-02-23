/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         NetAPI DLL
 * FILE:            reactos/dll/win32/netapi32/wksta_new.c
 * PURPOSE:         Workstation service interface code
 *
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include "netapi32.h"
#include "wkssvc_c.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

/* FUNCTIONS *****************************************************************/

void __RPC_FAR * __RPC_USER midl_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}


handle_t __RPC_USER
WKSSVC_IDENTIFY_HANDLE_bind(WKSSVC_IDENTIFY_HANDLE pszSystemName)
{
    handle_t hBinding = NULL;
    LPWSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("WKSSVC_IDENTIFY_HANDLE_bind() called\n");

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      pszSystemName,
                                      L"\\pipe\\wkssvc",
                                      NULL,
                                      &pszStringBinding);
    if (status)
    {
        TRACE("RpcStringBindingCompose returned 0x%x\n", status);
        return NULL;
    }

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBindingW(pszStringBinding,
                                          &hBinding);
    if (status)
    {
        TRACE("RpcBindingFromStringBinding returned 0x%x\n", status);
    }

    status = RpcStringFreeW(&pszStringBinding);
    if (status)
    {
//        TRACE("RpcStringFree returned 0x%x\n", status);
    }

    return hBinding;
}


void __RPC_USER
WKSSVC_IDENTIFY_HANDLE_unbind(WKSSVC_IDENTIFY_HANDLE pszSystemName,
                              handle_t hBinding)
{
    RPC_STATUS status;

    TRACE("WKSSVC_IDENTIFY_HANDLE_unbind() called\n");

    status = RpcBindingFree(&hBinding);
    if (status)
    {
        TRACE("RpcBindingFree returned 0x%x\n", status);
    }
}


handle_t __RPC_USER
WKSSVC_IMPERSONATE_HANDLE_bind(WKSSVC_IMPERSONATE_HANDLE pszSystemName)
{
    handle_t hBinding = NULL;
    LPWSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("WKSSVC_IMPERSONATE_HANDLE_bind() called\n");

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      pszSystemName,
                                      L"\\pipe\\wkssvc",
                                      NULL,
                                      &pszStringBinding);
    if (status)
    {
        TRACE("RpcStringBindingCompose returned 0x%x\n", status);
        return NULL;
    }

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBindingW(pszStringBinding,
                                          &hBinding);
    if (status)
    {
        TRACE("RpcBindingFromStringBinding returned 0x%x\n", status);
    }

    status = RpcStringFreeW(&pszStringBinding);
    if (status)
    {
//        TRACE("RpcStringFree returned 0x%x\n", status);
    }

    return hBinding;
}


void __RPC_USER
WKSSVC_IMPERSONATE_HANDLE_unbind(WKSSVC_IMPERSONATE_HANDLE pszSystemName,
                                 handle_t hBinding)
{
    RPC_STATUS status;

    TRACE("WKSSVC_IMPERSONATE_HANDLE_unbind() called\n");

    status = RpcBindingFree(&hBinding);
    if (status)
    {
        TRACE("RpcBindingFree returned 0x%x\n", status);
    }
}


NET_API_STATUS
NET_API_FUNCTION
NetGetJoinInformation(
    LPCWSTR Server,
    LPWSTR *Name,
    PNETSETUP_JOIN_STATUS type)
{
    NET_API_STATUS status;

    FIXME("Stub %s %p %p\n", wine_dbgstr_w(Server), Name, type);

    if (Name == NULL || type == NULL)
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        status = NetrGetJoinInformation((LPWSTR)Server,
                                        Name,
                                        type);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}


NET_API_STATUS
WINAPI
NetWkstaGetInfo(
    LMSTR servername,
    DWORD level,
    LPBYTE *bufptr)
{
    NET_API_STATUS status;

    *bufptr = NULL;

    RpcTryExcept
    {
        status = NetrWkstaGetInfo(servername,
                                  level,
                                  (LPWKSTA_INFO)bufptr);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept;

    return status;
}

/* EOF */

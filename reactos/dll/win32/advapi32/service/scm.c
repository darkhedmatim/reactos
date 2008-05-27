/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/service/scm.c
 * PURPOSE:         Service control manager functions
 * PROGRAMMER:      Emanuele Aliberti
 *                  Eric Kohl
 * UPDATE HISTORY:
 *  19990413 EA created
 *  19990515 EA
 */

/* INCLUDES ******************************************************************/

#include <advapi32.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(advapi);


/* FUNCTIONS *****************************************************************/

handle_t BindingHandle = NULL;

VOID
HandleBind(VOID)
{
    LPWSTR pszStringBinding;
    RPC_STATUS status;

    if (BindingHandle != NULL)
        return;

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      NULL,
                                      L"\\pipe\\ntsvcs",
                                      NULL,
                                      &pszStringBinding);
    if (status)
    {
        TRACE("RpcStringBindingCompose returned 0x%x\n", status);
        return;
    }

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBindingW(pszStringBinding,
                                          &BindingHandle);
    if (status)
    {
        TRACE("RpcBindingFromStringBinding returned 0x%x\n", status);
    }

    status = RpcStringFreeW(&pszStringBinding);
    if (status)
    {
        TRACE("RpcStringFree returned 0x%x\n", status);
    }
}


#if 0
static VOID
HandleUnbind(VOID)
{
    RPC_STATUS status;

    if (BindingHandle == NULL)
        return;

    status = RpcBindingFree(&BindingHandle);
    if (status)
    {
        TRACE("RpcBindingFree returned 0x%x\n", status);
    }
}
#endif


/**********************************************************************
 *  ChangeServiceConfig2A
 *
 * @implemented
 */
BOOL WINAPI
ChangeServiceConfig2A(SC_HANDLE hService,
                      DWORD dwInfoLevel,
                      LPVOID lpInfo)
{
    SC_RPC_CONFIG_INFOA Info;
    DWORD dwError;

    TRACE("ChangeServiceConfig2A() called\n");

    /* Fill relevent field of the Info structure */
    Info.dwInfoLevel = dwInfoLevel;
    switch (dwInfoLevel)
    {
        case SERVICE_CONFIG_DESCRIPTION:
            Info.psd = (LPSERVICE_DESCRIPTIONA)&lpInfo;
            break;

        case SERVICE_CONFIG_FAILURE_ACTIONS:
            Info.psfa = (LPSERVICE_FAILURE_ACTIONSA)&lpInfo;
            break;

        default:
            WARN("Unknown info level 0x%lx\n", dwInfoLevel);
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
    }

    if (lpInfo == NULL)
        return TRUE;

    HandleBind();

    dwError = RChangeServiceConfig2A(BindingHandle,
                                     (SC_RPC_HANDLE)hService,
                                     Info);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("RChangeServiceConfig2A() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  ChangeServiceConfig2W
 *
 * @implemented
 */
BOOL WINAPI
ChangeServiceConfig2W(SC_HANDLE hService,
                      DWORD dwInfoLevel,
                      LPVOID lpInfo)
{
    SC_RPC_CONFIG_INFOW Info;
    DWORD dwError;

    TRACE("ChangeServiceConfig2W() called\n");

    /* Fill relevent field of the Info structure */
    Info.dwInfoLevel = dwInfoLevel;
    switch (dwInfoLevel)
    {
        case SERVICE_CONFIG_DESCRIPTION:
        {
            Info.psd = (LPSERVICE_DESCRIPTIONW)&lpInfo;
            break;
        }

        case SERVICE_CONFIG_FAILURE_ACTIONS:
            Info.psfa = (LPSERVICE_FAILURE_ACTIONSW)&lpInfo;
            break;

        default:
            WARN("Unknown info level 0x%lx\n", dwInfoLevel);
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
    }

    if (lpInfo == NULL)
        return TRUE;

    HandleBind();

    dwError = RChangeServiceConfig2W(BindingHandle,
                                     (SC_RPC_HANDLE)hService,
                                     Info);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("RChangeServiceConfig2W() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  ChangeServiceConfigA
 *
 * @implemented
 */
BOOL STDCALL
ChangeServiceConfigA(SC_HANDLE hService,
                     DWORD dwServiceType,
                     DWORD dwStartType,
                     DWORD dwErrorControl,
                     LPCSTR lpBinaryPathName,
                     LPCSTR lpLoadOrderGroup,
                     LPDWORD lpdwTagId,
                     LPCSTR lpDependencies,
                     LPCSTR lpServiceStartName,
                     LPCSTR lpPassword,
                     LPCSTR lpDisplayName)
{
    DWORD dwError;
    DWORD dwDependenciesLength = 0;
    DWORD dwLength;
    LPSTR lpStr;

    TRACE("ChangeServiceConfigA() called\n");

    /* Calculate the Dependencies length*/
    if (lpDependencies != NULL)
    {
        lpStr = (LPSTR)lpDependencies;
        while (*lpStr)
        {
            dwLength = strlen(lpStr) + 1;
            dwDependenciesLength += dwLength;
            lpStr = lpStr + dwLength;
        }
        dwDependenciesLength++;
    }

    /* FIXME: Encrypt the password */

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = RChangeServiceConfigA(BindingHandle,
                                    (SC_RPC_HANDLE)hService,
                                    dwServiceType,
                                    dwStartType,
                                    dwErrorControl,
                                    (LPSTR)lpBinaryPathName,
                                    (LPSTR)lpLoadOrderGroup,
                                    lpdwTagId,
                                    (LPSTR)lpDependencies,
                                    dwDependenciesLength,
                                    (LPSTR)lpServiceStartName,
                                    NULL,              /* FIXME: lpPassword */
                                    0,                 /* FIXME: dwPasswordLength */
                                    (LPSTR)lpDisplayName);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("RChangeServiceConfigA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  ChangeServiceConfigW
 *
 * @implemented
 */
BOOL STDCALL
ChangeServiceConfigW(SC_HANDLE hService,
                     DWORD dwServiceType,
                     DWORD dwStartType,
                     DWORD dwErrorControl,
                     LPCWSTR lpBinaryPathName,
                     LPCWSTR lpLoadOrderGroup,
                     LPDWORD lpdwTagId,
                     LPCWSTR lpDependencies,
                     LPCWSTR lpServiceStartName,
                     LPCWSTR lpPassword,
                     LPCWSTR lpDisplayName)
{
    DWORD dwError;
    DWORD dwDependenciesLength = 0;
    DWORD dwLength;
    LPWSTR lpStr;

    TRACE("ChangeServiceConfigW() called\n");

    /* Calculate the Dependencies length*/
    if (lpDependencies != NULL)
    {
        lpStr = (LPWSTR)lpDependencies;
        while (*lpStr)
        {
            dwLength = wcslen(lpStr) + 1;
            dwDependenciesLength += dwLength;
            lpStr = lpStr + dwLength;
        }
        dwDependenciesLength++;
    }

    /* FIXME: Encrypt the password */

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = RChangeServiceConfigW(BindingHandle,
                                    (SC_RPC_HANDLE)hService,
                                    dwServiceType,
                                    dwStartType,
                                    dwErrorControl,
                                    (LPWSTR)lpBinaryPathName,
                                    (LPWSTR)lpLoadOrderGroup,
                                    lpdwTagId,
                                    (LPBYTE)lpDependencies,
                                    dwDependenciesLength,
                                    (LPWSTR)lpServiceStartName,
                                    NULL,              /* FIXME: lpPassword */
                                    0,                 /* FIXME: dwPasswordLength */
                                    (LPWSTR)lpDisplayName);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("RChangeServiceConfigW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  CloseServiceHandle
 *
 * @implemented
 */
BOOL STDCALL
CloseServiceHandle(SC_HANDLE hSCObject)
{
    DWORD dwError;

    TRACE("CloseServiceHandle() called\n");

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = RCloseServiceHandle(BindingHandle,
                                  (LPSC_RPC_HANDLE)&hSCObject);
    if (dwError)
    {
        ERR("RCloseServiceHandle() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    TRACE("CloseServiceHandle() done\n");

    return TRUE;
}


/**********************************************************************
 *  ControlService
 *
 * @implemented
 */
BOOL STDCALL
ControlService(SC_HANDLE hService,
               DWORD dwControl,
               LPSERVICE_STATUS lpServiceStatus)
{
    DWORD dwError;

    TRACE("ControlService(%x, %x, %p)\n",
           hService, dwControl, lpServiceStatus);

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = RControlService(BindingHandle,
                              (SC_RPC_HANDLE)hService,
                              dwControl,
                              lpServiceStatus);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("RControlService() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    TRACE("ControlService() done\n");

    return TRUE;
}


/**********************************************************************
 *  ControlServiceEx
 *
 * @unimplemented
 */
BOOL STDCALL
ControlServiceEx(IN SC_HANDLE hService,
                 IN DWORD dwControl,
                 IN DWORD dwInfoLevel,
                 IN OUT PVOID pControlParams)
{
    FIXME("ControlServiceEx(0x%p, 0x%x, 0x%x, 0x%p) UNIMPLEMENTED!\n",
            hService, dwControl, dwInfoLevel, pControlParams);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}



/**********************************************************************
 *  CreateServiceA
 *
 * @implemented
 */
SC_HANDLE STDCALL
CreateServiceA(SC_HANDLE hSCManager,
               LPCSTR lpServiceName,
               LPCSTR lpDisplayName,
               DWORD dwDesiredAccess,
               DWORD dwServiceType,
               DWORD dwStartType,
               DWORD dwErrorControl,
               LPCSTR lpBinaryPathName,
               LPCSTR lpLoadOrderGroup,
               LPDWORD lpdwTagId,
               LPCSTR lpDependencies,
               LPCSTR lpServiceStartName,
               LPCSTR lpPassword)
{
    SC_HANDLE RetVal = NULL;
    LPWSTR lpServiceNameW = NULL;
    LPWSTR lpDisplayNameW = NULL;
    LPWSTR lpBinaryPathNameW = NULL;
    LPWSTR lpLoadOrderGroupW = NULL;
    LPWSTR lpDependenciesW = NULL;
    LPWSTR lpServiceStartNameW = NULL;
    LPWSTR lpPasswordW = NULL;
    DWORD dwDependenciesLength = 0;
    DWORD dwLength;
    int len;
    LPSTR lpStr;

    if (lpServiceName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpServiceName, -1, NULL, 0);
        lpServiceNameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!lpServiceNameW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        MultiByteToWideChar(CP_ACP, 0, lpServiceName, -1, lpServiceNameW, len);
    }

    if (lpDisplayName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpDisplayName, -1, NULL, 0);
        lpDisplayNameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!lpDisplayNameW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        MultiByteToWideChar(CP_ACP, 0, lpDisplayName, -1, lpDisplayNameW, len);
    }

    if (lpBinaryPathName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpBinaryPathName, -1, NULL, 0);
        lpBinaryPathNameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!lpBinaryPathNameW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        MultiByteToWideChar(CP_ACP, 0, lpBinaryPathName, -1, lpBinaryPathNameW, len);
    }

    if (lpLoadOrderGroup)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpLoadOrderGroup, -1, NULL, 0);
        lpLoadOrderGroupW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!lpLoadOrderGroupW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        MultiByteToWideChar(CP_ACP, 0, lpLoadOrderGroup, -1, lpLoadOrderGroupW, len);
    }

    if (lpDependencies)
    {
        lpStr = (LPSTR)lpDependencies;
        while (*lpStr)
        {
            dwLength = strlen(lpStr) + 1;
            dwDependenciesLength += dwLength;
            lpStr = lpStr + dwLength;
        }
        dwDependenciesLength++;

        lpDependenciesW = HeapAlloc(GetProcessHeap(), 0, dwDependenciesLength * sizeof(WCHAR));
        if (!lpDependenciesW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        MultiByteToWideChar(CP_ACP, 0, lpDependencies, -1, lpDependenciesW, dwDependenciesLength);
    }

    if (lpServiceStartName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpServiceStartName, -1, NULL, 0);
        lpServiceStartNameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!lpServiceStartNameW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        MultiByteToWideChar(CP_ACP, 0, lpServiceStartName, -1, lpServiceStartNameW, len);
    }

    if (lpPassword)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpPassword, -1, NULL, 0);
        lpPasswordW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!lpPasswordW)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        MultiByteToWideChar(CP_ACP, 0, lpPassword, -1, lpPasswordW, len);
    }

    RetVal = CreateServiceW(hSCManager,
                            lpServiceNameW,
                            lpDisplayNameW,
                            dwDesiredAccess,
                            dwServiceType,
                            dwStartType,
                            dwErrorControl,
                            lpBinaryPathNameW,
                            lpLoadOrderGroupW,
                            lpdwTagId,
                            lpDependenciesW,
                            lpServiceStartNameW,
                            lpPasswordW);

cleanup:
    if (lpServiceNameW !=NULL)
        HeapFree(GetProcessHeap(), 0, lpServiceNameW);

    if (lpDisplayNameW != NULL)
        HeapFree(GetProcessHeap(), 0, lpDisplayNameW);

    if (lpBinaryPathNameW != NULL)
        HeapFree(GetProcessHeap(), 0, lpBinaryPathNameW);

    if (lpLoadOrderGroupW != NULL)
        HeapFree(GetProcessHeap(), 0, lpLoadOrderGroupW);

    if (lpDependenciesW != NULL)
        HeapFree(GetProcessHeap(), 0, lpDependenciesW);

    if (lpServiceStartNameW != NULL)
        HeapFree(GetProcessHeap(), 0, lpServiceStartNameW);

    if (lpPasswordW != NULL)
        HeapFree(GetProcessHeap(), 0, lpPasswordW);

    return RetVal;
}


/**********************************************************************
 *  CreateServiceW
 *
 * @implemented
 */
SC_HANDLE STDCALL
CreateServiceW(SC_HANDLE hSCManager,
               LPCWSTR lpServiceName,
               LPCWSTR lpDisplayName,
               DWORD dwDesiredAccess,
               DWORD dwServiceType,
               DWORD dwStartType,
               DWORD dwErrorControl,
               LPCWSTR lpBinaryPathName,
               LPCWSTR lpLoadOrderGroup,
               LPDWORD lpdwTagId,
               LPCWSTR lpDependencies,
               LPCWSTR lpServiceStartName,
               LPCWSTR lpPassword)
{
    SC_HANDLE hService = NULL;
    DWORD dwDependenciesLength = 0;
    DWORD dwError;
    DWORD dwLength;
    LPWSTR lpStr;

    TRACE("CreateServiceW() called\n");

    /* Calculate the Dependencies length*/
    if (lpDependencies != NULL)
    {
        lpStr = (LPWSTR)lpDependencies;
        while (*lpStr)
        {
            dwLength = wcslen(lpStr) + 1;
            dwDependenciesLength += dwLength;
            lpStr = lpStr + dwLength;
        }
        dwDependenciesLength++;
    }

    /* FIXME: Encrypt the password */

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = RCreateServiceW(BindingHandle,
                              (SC_RPC_HANDLE)hSCManager,
                              (LPWSTR)lpServiceName,
                              (LPWSTR)lpDisplayName,
                              dwDesiredAccess,
                              dwServiceType,
                              dwStartType,
                              dwErrorControl,
                              (LPWSTR)lpBinaryPathName,
                              (LPWSTR)lpLoadOrderGroup,
                              lpdwTagId,
                              (LPBYTE)lpDependencies,
                              dwDependenciesLength,
                              (LPWSTR)lpServiceStartName,
                              NULL,              /* FIXME: lpPassword */
                              0,                 /* FIXME: dwPasswordLength */
                              (SC_RPC_HANDLE *)&hService);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("RCreateServiceW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return NULL;
    }

    return hService;
}


/**********************************************************************
 *  DeleteService
 *
 * @implemented
 */
BOOL STDCALL
DeleteService(SC_HANDLE hService)
{
    DWORD dwError;

    TRACE("DeleteService(%x)\n", hService);

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = RDeleteService(BindingHandle,
                             (SC_RPC_HANDLE)hService);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("RDeleteService() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  EnumDependentServicesA
 *
 * @implemented
 */
BOOL STDCALL
EnumDependentServicesA(SC_HANDLE hService,
                       DWORD dwServiceState,
                       LPENUM_SERVICE_STATUSA lpServices,
                       DWORD cbBufSize,
                       LPDWORD pcbBytesNeeded,
                       LPDWORD lpServicesReturned)
{
    LPENUM_SERVICE_STATUSA lpStatusPtr;
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwCount;

    TRACE("EnumServicesStatusA() called\n");

    HandleBind();

    dwError = REnumDependentServicesA(BindingHandle,
                                      (SC_RPC_HANDLE)hService,
                                      dwServiceState,
                                      (LPBYTE)lpServices,
                                      cbBufSize,
                                      pcbBytesNeeded,
                                      lpServicesReturned);

    lpStatusPtr = (LPENUM_SERVICE_STATUSA)lpServices;
    for (dwCount = 0; dwCount < *lpServicesReturned; dwCount++)
    {
        if (lpStatusPtr->lpServiceName)
            lpStatusPtr->lpServiceName =
                (LPSTR)((ULONG_PTR)lpServices + (ULONG_PTR)lpStatusPtr->lpServiceName);

        if (lpStatusPtr->lpDisplayName)
            lpStatusPtr->lpDisplayName =
                (LPSTR)((ULONG_PTR)lpServices + (ULONG_PTR)lpStatusPtr->lpDisplayName);

        lpStatusPtr++;
    }

    if (dwError != ERROR_SUCCESS)
    {
        ERR("REnumDependentServicesA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    TRACE("EnumDependentServicesA() done\n");

    return TRUE;
}


/**********************************************************************
 *  EnumDependentServicesW
 *
 * @implemented
 */
BOOL STDCALL
EnumDependentServicesW(SC_HANDLE hService,
                       DWORD dwServiceState,
                       LPENUM_SERVICE_STATUSW lpServices,
                       DWORD cbBufSize,
                       LPDWORD pcbBytesNeeded,
                       LPDWORD lpServicesReturned)
{
    LPENUM_SERVICE_STATUSW lpStatusPtr;
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwCount;

    TRACE("EnumServicesStatusW() called\n");

    HandleBind();

    dwError = REnumDependentServicesW(BindingHandle,
                                      (SC_RPC_HANDLE)hService,
                                      dwServiceState,
                                      (LPBYTE)lpServices,
                                      cbBufSize,
                                      pcbBytesNeeded,
                                      lpServicesReturned);

    lpStatusPtr = (LPENUM_SERVICE_STATUSW)lpServices;
    for (dwCount = 0; dwCount < *lpServicesReturned; dwCount++)
    {
        if (lpStatusPtr->lpServiceName)
            lpStatusPtr->lpServiceName =
                (LPWSTR)((ULONG_PTR)lpServices + (ULONG_PTR)lpStatusPtr->lpServiceName);

        if (lpStatusPtr->lpDisplayName)
            lpStatusPtr->lpDisplayName =
                (LPWSTR)((ULONG_PTR)lpServices + (ULONG_PTR)lpStatusPtr->lpDisplayName);

        lpStatusPtr++;
    }

    if (dwError != ERROR_SUCCESS)
    {
        ERR("REnumDependentServicesW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    TRACE("EnumDependentServicesW() done\n");

    return TRUE;
}


/**********************************************************************
 *  EnumServiceGroupW
 *
 * @unimplemented
 */
BOOL
STDCALL
EnumServiceGroupW(
    SC_HANDLE               hSCManager,
    DWORD                   dwServiceType,
    DWORD                   dwServiceState,
    LPENUM_SERVICE_STATUSW  lpServices,
    DWORD                   cbBufSize,
    LPDWORD                 pcbBytesNeeded,
    LPDWORD                 lpServicesReturned,
    LPDWORD                 lpResumeHandle,
    LPCWSTR                 lpGroup)
{
    FIXME("EnumServiceGroupW is unimplemented\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *  EnumServicesStatusA
 *
 * @implemented
 */
BOOL STDCALL
EnumServicesStatusA(SC_HANDLE hSCManager,
                    DWORD dwServiceType,
                    DWORD dwServiceState,
                    LPENUM_SERVICE_STATUSA lpServices,
                    DWORD cbBufSize,
                    LPDWORD pcbBytesNeeded,
                    LPDWORD lpServicesReturned,
                    LPDWORD lpResumeHandle)
{
    LPENUM_SERVICE_STATUSA lpStatusPtr;
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwCount;

    TRACE("EnumServicesStatusA() called\n");

    HandleBind();

    dwError = REnumServicesStatusA(BindingHandle,
                                   (SC_RPC_HANDLE)hSCManager,
                                   dwServiceType,
                                   dwServiceState,
                                   (LPBYTE)lpServices,
                                   cbBufSize,
                                   pcbBytesNeeded,
                                   lpServicesReturned,
                                   lpResumeHandle);

    lpStatusPtr = (LPENUM_SERVICE_STATUSA)lpServices;
    for (dwCount = 0; dwCount < *lpServicesReturned; dwCount++)
    {
        if (lpStatusPtr->lpServiceName)
            lpStatusPtr->lpServiceName =
                (LPSTR)((ULONG_PTR)lpServices + (ULONG_PTR)lpStatusPtr->lpServiceName);

        if (lpStatusPtr->lpDisplayName)
            lpStatusPtr->lpDisplayName =
                (LPSTR)((ULONG_PTR)lpServices + (ULONG_PTR)lpStatusPtr->lpDisplayName);

        lpStatusPtr++;
    }

    if (dwError != ERROR_SUCCESS)
    {
        ERR("REnumServicesStatusA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    TRACE("EnumServicesStatusA() done\n");

    return TRUE;
}


/**********************************************************************
 *  EnumServicesStatusW
 *
 * @implemented
 */
BOOL STDCALL
EnumServicesStatusW(SC_HANDLE hSCManager,
                    DWORD dwServiceType,
                    DWORD dwServiceState,
                    LPENUM_SERVICE_STATUSW lpServices,
                    DWORD cbBufSize,
                    LPDWORD pcbBytesNeeded,
                    LPDWORD lpServicesReturned,
                    LPDWORD lpResumeHandle)
{
    LPENUM_SERVICE_STATUSW lpStatusPtr;
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwCount;

    TRACE("EnumServicesStatusW() called\n");

    HandleBind();

    dwError = REnumServicesStatusW(BindingHandle,
                                   (SC_RPC_HANDLE)hSCManager,
                                   dwServiceType,
                                   dwServiceState,
                                   (LPBYTE)lpServices,
                                   cbBufSize,
                                   pcbBytesNeeded,
                                   lpServicesReturned,
                                   lpResumeHandle);

    lpStatusPtr = (LPENUM_SERVICE_STATUSW)lpServices;
    for (dwCount = 0; dwCount < *lpServicesReturned; dwCount++)
    {
        if (lpStatusPtr->lpServiceName)
            lpStatusPtr->lpServiceName =
                (LPWSTR)((ULONG_PTR)lpServices + (ULONG_PTR)lpStatusPtr->lpServiceName);

        if (lpStatusPtr->lpDisplayName)
            lpStatusPtr->lpDisplayName =
                (LPWSTR)((ULONG_PTR)lpServices + (ULONG_PTR)lpStatusPtr->lpDisplayName);

        lpStatusPtr++;
    }

    if (dwError != ERROR_SUCCESS)
    {
        ERR("REnumServicesStatusW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    TRACE("EnumServicesStatusW() done\n");

    return TRUE;
}


/**********************************************************************
 *  EnumServicesStatusExA
 *
 * @implemented
 */
BOOL STDCALL
EnumServicesStatusExA(SC_HANDLE hSCManager,
                      SC_ENUM_TYPE InfoLevel,
                      DWORD dwServiceType,
                      DWORD dwServiceState,
                      LPBYTE lpServices,
                      DWORD cbBufSize,
                      LPDWORD pcbBytesNeeded,
                      LPDWORD lpServicesReturned,
                      LPDWORD lpResumeHandle,
                      LPCSTR pszGroupName)
{
    LPENUM_SERVICE_STATUS_PROCESSA lpStatusPtr;
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwCount;

    TRACE("EnumServicesStatusExA() called\n");

    HandleBind();

    dwError = REnumServicesStatusExA(BindingHandle,
                                     (SC_RPC_HANDLE)hSCManager,
                                     InfoLevel,
                                     dwServiceType,
                                     dwServiceState,
                                     (LPBYTE)lpServices,
                                     cbBufSize,
                                     pcbBytesNeeded,
                                     lpServicesReturned,
                                     lpResumeHandle,
                                     (LPSTR)pszGroupName);

    if (dwError == ERROR_MORE_DATA)
    {
        WARN("Required buffer size %ul\n", *pcbBytesNeeded);
        SetLastError(dwError);
        return FALSE;
    }
    else if (dwError == ERROR_SUCCESS)
    {
        lpStatusPtr = (LPENUM_SERVICE_STATUS_PROCESSA)lpServices;
        for (dwCount = 0; dwCount < *lpServicesReturned; dwCount++)
        {
            if (lpStatusPtr->lpServiceName)
                lpStatusPtr->lpServiceName =
                    (LPSTR)((ULONG_PTR)lpServices + (ULONG_PTR)lpStatusPtr->lpServiceName);

            if (lpStatusPtr->lpDisplayName)
                lpStatusPtr->lpDisplayName =
                    (LPSTR)((ULONG_PTR)lpServices + (ULONG_PTR)lpStatusPtr->lpDisplayName);

            lpStatusPtr++;
        }
    }
    else
    {
        ERR("REnumServicesStatusExA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    TRACE("EnumServicesStatusExA() done\n");

    return TRUE;
}


/**********************************************************************
 *  EnumServicesStatusExW
 *
 * @implemented
 */
BOOL STDCALL
EnumServicesStatusExW(SC_HANDLE hSCManager,
                      SC_ENUM_TYPE InfoLevel,
                      DWORD dwServiceType,
                      DWORD dwServiceState,
                      LPBYTE lpServices,
                      DWORD cbBufSize,
                      LPDWORD pcbBytesNeeded,
                      LPDWORD lpServicesReturned,
                      LPDWORD lpResumeHandle,
                      LPCWSTR pszGroupName)
{
    LPENUM_SERVICE_STATUS_PROCESSW lpStatusPtr;
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwCount;

    TRACE("EnumServicesStatusExW() called\n");

    HandleBind();

    dwError = REnumServicesStatusExW(BindingHandle,
                                     (SC_RPC_HANDLE)hSCManager,
                                     InfoLevel,
                                     dwServiceType,
                                     dwServiceState,
                                     (LPBYTE)lpServices,
                                     cbBufSize,
                                     pcbBytesNeeded,
                                     lpServicesReturned,
                                     lpResumeHandle,
                                     (LPWSTR)pszGroupName);

    if (dwError == ERROR_MORE_DATA)
    {
        WARN("Required buffer size %ul\n", *pcbBytesNeeded);
        SetLastError(dwError);
        return FALSE;
    }
    else if (dwError == ERROR_SUCCESS)
    {
        lpStatusPtr = (LPENUM_SERVICE_STATUS_PROCESSW)lpServices;
        for (dwCount = 0; dwCount < *lpServicesReturned; dwCount++)
        {
            if (lpStatusPtr->lpServiceName)
                lpStatusPtr->lpServiceName =
                    (LPWSTR)((ULONG_PTR)lpServices + (ULONG_PTR)lpStatusPtr->lpServiceName);

            if (lpStatusPtr->lpDisplayName)
                lpStatusPtr->lpDisplayName =
                    (LPWSTR)((ULONG_PTR)lpServices + (ULONG_PTR)lpStatusPtr->lpDisplayName);

            lpStatusPtr++;
        }
    }
    else
    {
        ERR("REnumServicesStatusExW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    TRACE("EnumServicesStatusExW() done\n");

    return TRUE;
}


/**********************************************************************
 *  GetServiceDisplayNameA
 *
 * @implemented
 */
BOOL STDCALL
GetServiceDisplayNameA(SC_HANDLE hSCManager,
                       LPCSTR lpServiceName,
                       LPSTR lpDisplayName,
                       LPDWORD lpcchBuffer)
{
    DWORD dwError;

    TRACE("GetServiceDisplayNameA() called\n");

    HandleBind();

    dwError = RGetServiceDisplayNameA(BindingHandle,
                                      (SC_RPC_HANDLE)hSCManager,
                                      (LPSTR)lpServiceName,
                                      lpDisplayName,
                                      lpcchBuffer);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("RGetServiceDisplayNameA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    (*lpcchBuffer)--;

    return TRUE;
}


/**********************************************************************
 *  GetServiceDisplayNameW
 *
 * @implemented
 */
BOOL STDCALL
GetServiceDisplayNameW(SC_HANDLE hSCManager,
                       LPCWSTR lpServiceName,
                       LPWSTR lpDisplayName,
                       LPDWORD lpcchBuffer)
{
    DWORD dwError;

    TRACE("GetServiceDisplayNameW() called\n");

    HandleBind();

    dwError = RGetServiceDisplayNameW(BindingHandle,
                                      (SC_RPC_HANDLE)hSCManager,
                                      (LPWSTR)lpServiceName,
                                      lpDisplayName,
                                      lpcchBuffer);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("RGetServiceDisplayNameW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    (*lpcchBuffer)--;

    return TRUE;
}


/**********************************************************************
 *  GetServiceKeyNameA
 *
 * @implemented
 */
BOOL STDCALL
GetServiceKeyNameA(SC_HANDLE hSCManager,
                   LPCSTR lpDisplayName,
                   LPSTR lpServiceName,
                   LPDWORD lpcchBuffer)
{
    DWORD dwError;

    TRACE("GetServiceKeyNameA() called\n");

    HandleBind();

    dwError = RGetServiceKeyNameA(BindingHandle,
                                  (SC_RPC_HANDLE)hSCManager,
                                  (LPSTR)lpDisplayName,
                                  lpServiceName,
                                  lpcchBuffer);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("RGetServiceKeyNameA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    (*lpcchBuffer)--;

    return TRUE;
}


/**********************************************************************
 *  GetServiceKeyNameW
 *
 * @implemented
 */
BOOL STDCALL
GetServiceKeyNameW(SC_HANDLE hSCManager,
                   LPCWSTR lpDisplayName,
                   LPWSTR lpServiceName,
                   LPDWORD lpcchBuffer)
{
    DWORD dwError;

    TRACE("GetServiceKeyNameW() called\n");

    HandleBind();

    dwError = RGetServiceKeyNameW(BindingHandle,
                                  (SC_RPC_HANDLE)hSCManager,
                                  (LPWSTR)lpDisplayName,
                                  lpServiceName,
                                  lpcchBuffer);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("RGetServiceKeyNameW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    (*lpcchBuffer)--;

    return TRUE;
}


/**********************************************************************
 *  LockServiceDatabase
 *
 * @implemented
 */
SC_LOCK STDCALL
LockServiceDatabase(SC_HANDLE hSCManager)
{
    SC_LOCK hLock;
    DWORD dwError;

    TRACE("LockServiceDatabase(%x)\n", hSCManager);

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = RLockServiceDatabase(BindingHandle,
                                   (SC_RPC_HANDLE)hSCManager,
                                   (SC_RPC_LOCK *)&hLock);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("RLockServiceDatabase() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return NULL;
    }

    TRACE("hLock = %p\n", hLock);

    return hLock;
}


static VOID
WaitForSCManager(VOID)
{
    HANDLE hEvent;

    TRACE("WaitForSCManager() called\n");

    /* Try to open the existing event */
    hEvent = OpenEventW(SYNCHRONIZE,
                        FALSE,
                        L"SvcctrlStartEvent_A3752DX");
    if (hEvent == NULL)
    {
        if (GetLastError() != ERROR_FILE_NOT_FOUND)
            return;

        /* Try to create a new event */
        hEvent = CreateEventW(NULL,
                              TRUE,
                              FALSE,
                              L"SvcctrlStartEvent_A3752DX");
        if (hEvent == NULL)
        {
            /* Try to open the existing event again */
            hEvent = OpenEventW(SYNCHRONIZE,
                                FALSE,
                                L"SvcctrlStartEvent_A3752DX");
            if (hEvent == NULL)
                return;
        }
    }

    /* Wait for 3 minutes */
    WaitForSingleObject(hEvent, 180000);
    CloseHandle(hEvent);

    TRACE("ScmWaitForSCManager() done\n");
}


/**********************************************************************
 *  OpenSCManagerA
 *
 * @implemented
 */
SC_HANDLE STDCALL
OpenSCManagerA(LPCSTR lpMachineName,
               LPCSTR lpDatabaseName,
               DWORD dwDesiredAccess)
{
    SC_HANDLE hScm = NULL;
    DWORD dwError;

    TRACE("OpenSCManagerA(%s, %s, %lx)\n",
           lpMachineName, lpDatabaseName, dwDesiredAccess);

    WaitForSCManager();

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = ROpenSCManagerA(BindingHandle,
                              (LPSTR)lpMachineName,
                              (LPSTR)lpDatabaseName,
                              dwDesiredAccess,
                              (SC_RPC_HANDLE *)&hScm);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("ROpenSCManagerA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return NULL;
    }

    TRACE("hScm = %p\n", hScm);

    return hScm;
}


/**********************************************************************
 *  OpenSCManagerW
 *
 * @implemented
 */
SC_HANDLE STDCALL
OpenSCManagerW(LPCWSTR lpMachineName,
               LPCWSTR lpDatabaseName,
               DWORD dwDesiredAccess)
{
    SC_HANDLE hScm = NULL;
    DWORD dwError;

    TRACE("OpenSCManagerW(%S, %S, %lx)\n",
           lpMachineName, lpDatabaseName, dwDesiredAccess);

    WaitForSCManager();

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = ROpenSCManagerW(BindingHandle,
                              (LPWSTR)lpMachineName,
                              (LPWSTR)lpDatabaseName,
                              dwDesiredAccess,
                              (SC_RPC_HANDLE *)&hScm);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("ROpenSCManagerW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return NULL;
    }

    TRACE("hScm = %p\n", hScm);

    return hScm;
}


/**********************************************************************
 *  OpenServiceA
 *
 * @implemented
 */
SC_HANDLE STDCALL
OpenServiceA(SC_HANDLE hSCManager,
             LPCSTR lpServiceName,
             DWORD dwDesiredAccess)
{
    SC_HANDLE hService = NULL;
    DWORD dwError;

    TRACE("OpenServiceA(%p, %s, %lx)\n",
           hSCManager, lpServiceName, dwDesiredAccess);

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = ROpenServiceA(BindingHandle,
                            (SC_RPC_HANDLE)hSCManager,
                            (LPSTR)lpServiceName,
                            dwDesiredAccess,
                            (SC_RPC_HANDLE *)&hService);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("ROpenServiceA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return NULL;
    }

    TRACE("hService = %p\n", hService);

    return hService;
}


/**********************************************************************
 *  OpenServiceW
 *
 * @implemented
 */
SC_HANDLE STDCALL
OpenServiceW(SC_HANDLE hSCManager,
             LPCWSTR lpServiceName,
             DWORD dwDesiredAccess)
{
    SC_HANDLE hService = NULL;
    DWORD dwError;

    TRACE("OpenServiceW(%p, %S, %lx)\n",
           hSCManager, lpServiceName, dwDesiredAccess);

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = ROpenServiceW(BindingHandle,
                            (SC_RPC_HANDLE)hSCManager,
                            (LPWSTR)lpServiceName,
                            dwDesiredAccess,
                            (SC_RPC_HANDLE *)&hService);
    if (dwError != ERROR_SUCCESS)
    {
        if (dwError == ERROR_SERVICE_DOES_NOT_EXIST)
            WARN("ROpenServiceW() failed (Error %lu)\n", dwError);
        else
            ERR("ROpenServiceW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return NULL;
    }

    TRACE("hService = %p\n", hService);

    return hService;
}


/**********************************************************************
 *  QueryServiceConfigA
 *
 * @implemented
 */
BOOL STDCALL
QueryServiceConfigA(SC_HANDLE hService,
                    LPQUERY_SERVICE_CONFIGA lpServiceConfig,
                    DWORD cbBufSize,
                    LPDWORD pcbBytesNeeded)
{
    DWORD dwError;

    TRACE("QueryServiceConfigA(%p, %p, %lu, %p)\n",
           hService, lpServiceConfig, cbBufSize, pcbBytesNeeded);

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = RQueryServiceConfigA(BindingHandle,
                                   (SC_RPC_HANDLE)hService,
                                   lpServiceConfig,
                                   cbBufSize,
                                   pcbBytesNeeded);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("RQueryServiceConfigA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    /* Adjust the pointers */
    if (lpServiceConfig->lpBinaryPathName)
        lpServiceConfig->lpBinaryPathName =
            (LPSTR)((ULONG_PTR)lpServiceConfig +
                    (ULONG_PTR)lpServiceConfig->lpBinaryPathName);

    if (lpServiceConfig->lpLoadOrderGroup)
        lpServiceConfig->lpLoadOrderGroup =
            (LPSTR)((ULONG_PTR)lpServiceConfig +
                    (ULONG_PTR)lpServiceConfig->lpLoadOrderGroup);

    if (lpServiceConfig->lpDependencies)
        lpServiceConfig->lpDependencies =
            (LPSTR)((ULONG_PTR)lpServiceConfig +
                    (ULONG_PTR)lpServiceConfig->lpDependencies);

    if (lpServiceConfig->lpServiceStartName)
        lpServiceConfig->lpServiceStartName =
            (LPSTR)((ULONG_PTR)lpServiceConfig +
                    (ULONG_PTR)lpServiceConfig->lpServiceStartName);

    if (lpServiceConfig->lpDisplayName)
        lpServiceConfig->lpDisplayName =
           (LPSTR)((ULONG_PTR)lpServiceConfig +
                   (ULONG_PTR)lpServiceConfig->lpDisplayName);

    TRACE("QueryServiceConfigA() done\n");

    return TRUE;
}


/**********************************************************************
 *  QueryServiceConfigW
 *
 * @implemented
 */
BOOL STDCALL
QueryServiceConfigW(SC_HANDLE hService,
                    LPQUERY_SERVICE_CONFIGW lpServiceConfig,
                    DWORD cbBufSize,
                    LPDWORD pcbBytesNeeded)
{
    DWORD dwError;

    TRACE("QueryServiceConfigW(%p, %p, %lu, %p)\n",
           hService, lpServiceConfig, cbBufSize, pcbBytesNeeded);

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = RQueryServiceConfigW(BindingHandle,
                                   (SC_RPC_HANDLE)hService,
                                   (LPBYTE)lpServiceConfig,
                                   cbBufSize,
                                   pcbBytesNeeded);
    if (dwError != ERROR_SUCCESS)
    {
        if (dwError == ERROR_INSUFFICIENT_BUFFER)
            WARN("RQueryServiceConfigW() failed (Error %lu)\n", dwError);
        else
            ERR("RQueryServiceConfigW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    /* Adjust the pointers */
    if (lpServiceConfig->lpBinaryPathName)
        lpServiceConfig->lpBinaryPathName =
            (LPWSTR)((ULONG_PTR)lpServiceConfig +
                     (ULONG_PTR)lpServiceConfig->lpBinaryPathName);

    if (lpServiceConfig->lpLoadOrderGroup)
        lpServiceConfig->lpLoadOrderGroup =
            (LPWSTR)((ULONG_PTR)lpServiceConfig +
                     (ULONG_PTR)lpServiceConfig->lpLoadOrderGroup);

    if (lpServiceConfig->lpDependencies)
        lpServiceConfig->lpDependencies =
            (LPWSTR)((ULONG_PTR)lpServiceConfig +
                     (ULONG_PTR)lpServiceConfig->lpDependencies);

    if (lpServiceConfig->lpServiceStartName)
        lpServiceConfig->lpServiceStartName =
            (LPWSTR)((ULONG_PTR)lpServiceConfig +
                     (ULONG_PTR)lpServiceConfig->lpServiceStartName);

    if (lpServiceConfig->lpDisplayName)
        lpServiceConfig->lpDisplayName =
           (LPWSTR)((ULONG_PTR)lpServiceConfig +
                    (ULONG_PTR)lpServiceConfig->lpDisplayName);

    TRACE("QueryServiceConfigW() done\n");

    return TRUE;
}


/**********************************************************************
 *  QueryServiceConfig2A
 *
 * @implemented
 */
BOOL STDCALL
QueryServiceConfig2A(SC_HANDLE hService,
                     DWORD dwInfoLevel,
                     LPBYTE lpBuffer,
                     DWORD cbBufSize,
                     LPDWORD pcbBytesNeeded)
{
    DWORD dwError;

    TRACE("QueryServiceConfig2A(%p, %lu, %p, %lu, %p)\n",
           hService, dwInfoLevel, lpBuffer, cbBufSize, pcbBytesNeeded);

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = RQueryServiceConfig2A(BindingHandle,
                                    (SC_RPC_HANDLE)hService,
                                    dwInfoLevel,
                                    lpBuffer,
                                    cbBufSize,
                                    pcbBytesNeeded);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("RQueryServiceConfig2A() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    switch (dwInfoLevel)
    {
        case SERVICE_CONFIG_DESCRIPTION:
            {
                LPSERVICE_DESCRIPTIONA lpPtr = (LPSERVICE_DESCRIPTIONA)lpBuffer;

                if (lpPtr->lpDescription != NULL)
                    lpPtr->lpDescription =
                        (LPSTR)((UINT_PTR)lpPtr + (UINT_PTR)lpPtr->lpDescription);
            }
            break;

        case SERVICE_CONFIG_FAILURE_ACTIONS:
            {
                LPSERVICE_FAILURE_ACTIONSA lpPtr = (LPSERVICE_FAILURE_ACTIONSA)lpBuffer;

                if (lpPtr->lpRebootMsg != NULL)
                    lpPtr->lpRebootMsg =
                        (LPSTR)((UINT_PTR)lpPtr + (UINT_PTR)lpPtr->lpRebootMsg);

                if (lpPtr->lpCommand != NULL)
                    lpPtr->lpCommand =
                        (LPSTR)((UINT_PTR)lpPtr + (UINT_PTR)lpPtr->lpCommand);

                if (lpPtr->lpsaActions != NULL)
                    lpPtr->lpsaActions =
                        (SC_ACTION*)((UINT_PTR)lpPtr + (UINT_PTR)lpPtr->lpsaActions);
            }
            break;

        default:
            ERR("Unknown info level 0x%lx\n", dwInfoLevel);
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
    }

    TRACE("QueryServiceConfig2A() done\n");

    return TRUE;
}


/**********************************************************************
 *  QueryServiceConfig2W
 *
 * @implemented
 */
BOOL STDCALL
QueryServiceConfig2W(SC_HANDLE hService,
                     DWORD dwInfoLevel,
                     LPBYTE lpBuffer,
                     DWORD cbBufSize,
                     LPDWORD pcbBytesNeeded)
{
    DWORD dwError;

    TRACE("QueryServiceConfig2W(%p, %lu, %p, %lu, %p)\n",
           hService, dwInfoLevel, lpBuffer, cbBufSize, pcbBytesNeeded);

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = RQueryServiceConfig2W(BindingHandle,
                                    (SC_RPC_HANDLE)hService,
                                    dwInfoLevel,
                                    lpBuffer,
                                    cbBufSize,
                                    pcbBytesNeeded);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("RQueryServiceConfig2W() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    switch (dwInfoLevel)
    {
        case SERVICE_CONFIG_DESCRIPTION:
            {
                LPSERVICE_DESCRIPTIONW lpPtr = (LPSERVICE_DESCRIPTIONW)lpBuffer;

                if (lpPtr->lpDescription != NULL)
                    lpPtr->lpDescription =
                        (LPWSTR)((UINT_PTR)lpPtr + (UINT_PTR)lpPtr->lpDescription);
            }
            break;

        case SERVICE_CONFIG_FAILURE_ACTIONS:
            {
                LPSERVICE_FAILURE_ACTIONSW lpPtr = (LPSERVICE_FAILURE_ACTIONSW)lpBuffer;

                if (lpPtr->lpRebootMsg != NULL)
                    lpPtr->lpRebootMsg =
                        (LPWSTR)((UINT_PTR)lpPtr + (UINT_PTR)lpPtr->lpRebootMsg);

                if (lpPtr->lpCommand != NULL)
                    lpPtr->lpCommand =
                        (LPWSTR)((UINT_PTR)lpPtr + (UINT_PTR)lpPtr->lpCommand);

                if (lpPtr->lpsaActions != NULL)
                    lpPtr->lpsaActions =
                        (SC_ACTION*)((UINT_PTR)lpPtr + (UINT_PTR)lpPtr->lpsaActions);
            }
            break;

        default:
            WARN("Unknown info level 0x%lx\n", dwInfoLevel);
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
    }

    TRACE("QueryServiceConfig2W() done\n");

    return TRUE;
}


/**********************************************************************
 *  QueryServiceLockStatusA
 *
 * @implemented
 */
BOOL STDCALL
QueryServiceLockStatusA(SC_HANDLE hSCManager,
                        LPQUERY_SERVICE_LOCK_STATUSA lpLockStatus,
                        DWORD cbBufSize,
                        LPDWORD pcbBytesNeeded)
{
    DWORD dwError;

    TRACE("QueryServiceLockStatusA() called\n");

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = RQueryServiceLockStatusA(BindingHandle,
                                       (SC_RPC_HANDLE)hSCManager,
                                       lpLockStatus,
                                       cbBufSize,
                                       pcbBytesNeeded);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("RQueryServiceLockStatusA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    if (lpLockStatus->lpLockOwner != NULL)
    {
        lpLockStatus->lpLockOwner =
            (LPSTR)((UINT_PTR)lpLockStatus + (UINT_PTR)lpLockStatus->lpLockOwner);
    }

    TRACE("QueryServiceLockStatusA() done\n");

    return TRUE;
}


/**********************************************************************
 *  QueryServiceLockStatusW
 *
 * @implemented
 */
BOOL STDCALL
QueryServiceLockStatusW(SC_HANDLE hSCManager,
                        LPQUERY_SERVICE_LOCK_STATUSW lpLockStatus,
                        DWORD cbBufSize,
                        LPDWORD pcbBytesNeeded)
{
    DWORD dwError;

    TRACE("QueryServiceLockStatusW() called\n");

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = RQueryServiceLockStatusW(BindingHandle,
                                       (SC_RPC_HANDLE)hSCManager,
                                       lpLockStatus,
                                       cbBufSize,
                                       pcbBytesNeeded);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("RQueryServiceLockStatusW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    if (lpLockStatus->lpLockOwner != NULL)
    {
        lpLockStatus->lpLockOwner =
            (LPWSTR)((UINT_PTR)lpLockStatus + (UINT_PTR)lpLockStatus->lpLockOwner);
    }

    TRACE("QueryServiceLockStatusW() done\n");

    return TRUE;
}


/**********************************************************************
 *  QueryServiceObjectSecurity
 *
 * @implemented
 */
BOOL STDCALL
QueryServiceObjectSecurity(SC_HANDLE hService,
                           SECURITY_INFORMATION dwSecurityInformation,
                           PSECURITY_DESCRIPTOR lpSecurityDescriptor,
                           DWORD cbBufSize,
                           LPDWORD pcbBytesNeeded)
{
    DWORD dwError;

    TRACE("QueryServiceObjectSecurity(%p, %lu, %p)\n",
           hService, dwSecurityInformation, lpSecurityDescriptor);

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = RQueryServiceObjectSecurity(BindingHandle,
                                          (SC_RPC_HANDLE)hService,
                                          dwSecurityInformation,
                                          (LPBYTE)lpSecurityDescriptor,
                                          cbBufSize,
                                          pcbBytesNeeded);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("QueryServiceObjectSecurity() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}

/**********************************************************************
 *  SetServiceObjectSecurity
 *
 * @implemented
 */
BOOL STDCALL
SetServiceObjectSecurity(SC_HANDLE hService,
                         SECURITY_INFORMATION dwSecurityInformation,
                         PSECURITY_DESCRIPTOR lpSecurityDescriptor)
{
    PSECURITY_DESCRIPTOR SelfRelativeSD = NULL;
    ULONG Length;
    NTSTATUS Status;
    DWORD dwError;

    Length = 0;
    Status = RtlMakeSelfRelativeSD(lpSecurityDescriptor,
                                   SelfRelativeSD,
                                   &Length);
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    SelfRelativeSD = HeapAlloc(GetProcessHeap(), 0, Length);
    if (SelfRelativeSD == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    Status = RtlMakeSelfRelativeSD(lpSecurityDescriptor,
                                   SelfRelativeSD,
                                   &Length);
    if (!NT_SUCCESS(Status))
    {
        HeapFree(GetProcessHeap(), 0, SelfRelativeSD);
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = RSetServiceObjectSecurity(BindingHandle,
                                        (SC_RPC_HANDLE)hService,
                                        dwSecurityInformation,
                                        (LPBYTE)SelfRelativeSD,
                                        Length);

    HeapFree(GetProcessHeap(), 0, SelfRelativeSD);

    if (dwError != ERROR_SUCCESS)
    {
        ERR("RServiceObjectSecurity() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  QueryServiceStatus
 *
 * @implemented
 */
BOOL STDCALL
QueryServiceStatus(SC_HANDLE hService,
                   LPSERVICE_STATUS lpServiceStatus)
{
    DWORD dwError;

    TRACE("QueryServiceStatus(%p, %p)\n",
           hService, lpServiceStatus);

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = RQueryServiceStatus(BindingHandle,
                                  (SC_RPC_HANDLE)hService,
                                  lpServiceStatus);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("RQueryServiceStatus() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  QueryServiceStatusEx
 *
 * @implemented
 */
BOOL STDCALL
QueryServiceStatusEx(SC_HANDLE hService,
                     SC_STATUS_TYPE InfoLevel,
                     LPBYTE lpBuffer,
                     DWORD cbBufSize,
                     LPDWORD pcbBytesNeeded)
{
    DWORD dwError;

    TRACE("QueryServiceStatusEx() called\n");

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = RQueryServiceStatusEx(BindingHandle,
                                    (SC_RPC_HANDLE)hService,
                                    InfoLevel,
                                    lpBuffer,
                                    cbBufSize,
                                    pcbBytesNeeded);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("RQueryServiceStatusEx() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  StartServiceA
 *
 * @implemented
 */
BOOL STDCALL
StartServiceA(SC_HANDLE hService,
              DWORD dwNumServiceArgs,
              LPCSTR *lpServiceArgVectors)
{
    DWORD dwError;

    dwError = RStartServiceA(BindingHandle,
                             (SC_RPC_HANDLE)hService,
                             dwNumServiceArgs,
                             (LPSTRING_PTRSA)lpServiceArgVectors);


    if (dwError != ERROR_SUCCESS)
    {
        ERR("RStartServiceA() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  StartServiceW
 *
 * @implemented
 */
BOOL STDCALL
StartServiceW(SC_HANDLE hService,
              DWORD dwNumServiceArgs,
              LPCWSTR *lpServiceArgVectors)
{
    DWORD dwError;

    dwError = RStartServiceW(BindingHandle,
                             (SC_RPC_HANDLE)hService,
                             dwNumServiceArgs,
                             (LPSTRING_PTRSW)lpServiceArgVectors);

    if (dwError != ERROR_SUCCESS)
    {
        ERR("RStartServiceW() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  UnlockServiceDatabase
 *
 * @implemented
 */
BOOL STDCALL
UnlockServiceDatabase(SC_LOCK ScLock)
{
    DWORD dwError;

    TRACE("UnlockServiceDatabase(%x)\n", ScLock);

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = RUnlockServiceDatabase(BindingHandle,
                                     (SC_RPC_LOCK)ScLock);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("RUnlockServiceDatabase() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}


/**********************************************************************
 *  NotifyBootConfigStatus
 *
 * @implemented
 */
BOOL STDCALL
NotifyBootConfigStatus(BOOL BootAcceptable)
{
    DWORD dwError;

    TRACE("NotifyBootConfigStatus()\n");

    HandleBind();

    /* Call to services.exe using RPC */
    dwError = RNotifyBootConfigStatus(BindingHandle,
                                      NULL,
                                      BootAcceptable);
    if (dwError != ERROR_SUCCESS)
    {
        ERR("NotifyBootConfigStatus() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    return TRUE;
}

/* EOF */

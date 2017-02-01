/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Print Processors
 * COPYRIGHT:   Copyright 2015-2016 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"
#include <prtprocenv.h>

static void
_MarshallUpDatatypesInfo(PDATATYPES_INFO_1W pDatatypesInfo1)
{
    // Replace relative offset addresses in the output by absolute pointers.
    pDatatypesInfo1->pName = (PWSTR)((ULONG_PTR)pDatatypesInfo1->pName + (ULONG_PTR)pDatatypesInfo1);
}

static void
_MarshallUpPrintProcessorInfo(PPRINTPROCESSOR_INFO_1W pPrintProcessorInfo1)
{
    // Replace relative offset addresses in the output by absolute pointers.
    pPrintProcessorInfo1->pName = (PWSTR)((ULONG_PTR)pPrintProcessorInfo1->pName + (ULONG_PTR)pPrintProcessorInfo1);
}

BOOL WINAPI
AddPrintProcessorW(PWSTR pName, PWSTR pEnvironment, PWSTR pPathName, PWSTR pPrintProcessorName)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
DeletePrintProcessorW(PWSTR pName, PWSTR pEnvironment, PWSTR pPrintProcessorName)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
EnumPrintProcessorDatatypesA(PSTR pName, LPSTR pPrintProcessorName, DWORD Level, PBYTE pDatatypes, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
EnumPrintProcessorDatatypesW(PWSTR pName, LPWSTR pPrintProcessorName, DWORD Level, PBYTE pDatatypes, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD dwErrorCode;
    DWORD i;
    PBYTE p = pDatatypes;

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcEnumPrintProcessorDatatypes(pName, pPrintProcessorName, Level, pDatatypes, cbBuf, pcbNeeded, pcReturned);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcEnumPrintProcessorDatatypes failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    if (dwErrorCode == ERROR_SUCCESS)
    {
        // Replace relative offset addresses in the output by absolute pointers.
        for (i = 0; i < *pcReturned; i++)
        {
            _MarshallUpDatatypesInfo((PDATATYPES_INFO_1W)p);
            p += sizeof(DATATYPES_INFO_1W);
        }
    }

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
EnumPrintProcessorsW(PWSTR pName, PWSTR pEnvironment, DWORD Level, PBYTE pPrintProcessorInfo, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD dwErrorCode;
    DWORD i;
    PBYTE p = pPrintProcessorInfo;

    // Choose our current environment if the caller didn't give any.
    if (!pEnvironment)
        pEnvironment = (PWSTR)wszCurrentEnvironment;

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcEnumPrintProcessors(pName, pEnvironment, Level, pPrintProcessorInfo, cbBuf, pcbNeeded, pcReturned);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
    }
    RpcEndExcept;

    if (dwErrorCode == ERROR_SUCCESS)
    {
        // Replace relative offset addresses in the output by absolute pointers.
        for (i = 0; i < *pcReturned; i++)
        {
            _MarshallUpPrintProcessorInfo((PPRINTPROCESSOR_INFO_1W)p);
            p += sizeof(PRINTPROCESSOR_INFO_1W);
        }
    }

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
GetPrintProcessorDirectoryA(PSTR pName, PSTR pEnvironment, DWORD Level, PBYTE pPrintProcessorInfo, DWORD cbBuf, PDWORD pcbNeeded)
{
    BOOL bReturnValue = FALSE;
    DWORD cch;
    PWSTR pwszName = NULL;
    PWSTR pwszEnvironment = NULL;
    PWSTR pwszPrintProcessorInfo = NULL;

    if (pName)
    {
        // Convert pName to a Unicode string pwszName.
        cch = strlen(pName);

        pwszName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(WCHAR));
        if (!pwszName)
        {
            ERR("HeapAlloc failed for pwszName with last error %lu!\n", GetLastError());
            goto Cleanup;
        }

        MultiByteToWideChar(CP_ACP, 0, pName, -1, pwszName, cch + 1);
    }

    if (pEnvironment)
    {
        // Convert pEnvironment to a Unicode string pwszEnvironment.
        cch = strlen(pEnvironment);

        pwszEnvironment = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(WCHAR));
        if (!pwszEnvironment)
        {
            ERR("HeapAlloc failed for pwszEnvironment with last error %lu!\n", GetLastError());
            goto Cleanup;
        }

        MultiByteToWideChar(CP_ACP, 0, pEnvironment, -1, pwszEnvironment, cch + 1);
    }

    if (cbBuf && pPrintProcessorInfo)
    {
        // Allocate a temporary buffer for the Unicode result.
        // We can just go with cbBuf here. The user should have set it based on pcbNeeded returned in a previous call and our
        // pcbNeeded is the same for the A and W functions.
        pwszPrintProcessorInfo = HeapAlloc(hProcessHeap, 0, cbBuf);
        if (!pwszPrintProcessorInfo)
        {
            ERR("HeapAlloc failed for pwszPrintProcessorInfo with last error %lu!\n", GetLastError());
            goto Cleanup;
        }
    }

    bReturnValue = GetPrintProcessorDirectoryW(pwszName, pwszEnvironment, Level, (PBYTE)pwszPrintProcessorInfo, cbBuf, pcbNeeded);

    if (bReturnValue)
    {
        // Convert pwszPrintProcessorInfo to an ANSI string pPrintProcessorInfo.
        WideCharToMultiByte(CP_ACP, 0, pwszPrintProcessorInfo, -1, (PSTR)pPrintProcessorInfo, cbBuf, NULL, NULL);
    }

Cleanup:
    if (pwszName)
        HeapFree(hProcessHeap, 0, pwszName);

    if (pwszEnvironment)
        HeapFree(hProcessHeap, 0, pwszEnvironment);

    if (pwszPrintProcessorInfo)
        HeapFree(hProcessHeap, 0, pwszPrintProcessorInfo);

    return bReturnValue;
}

BOOL WINAPI
GetPrintProcessorDirectoryW(PWSTR pName, PWSTR pEnvironment, DWORD Level, PBYTE pPrintProcessorInfo, DWORD cbBuf, PDWORD pcbNeeded)
{
    DWORD dwErrorCode;

    // Sanity checks
    if (Level != 1)
    {
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    // Choose our current environment if the caller didn't give any.
    if (!pEnvironment)
        pEnvironment = (PWSTR)wszCurrentEnvironment;

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcGetPrintProcessorDirectory(pName, pEnvironment, Level, pPrintProcessorInfo, cbBuf, pcbNeeded);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
    }
    RpcEndExcept;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

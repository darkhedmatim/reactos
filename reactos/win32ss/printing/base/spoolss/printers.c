/*
 * PROJECT:     ReactOS Spooler Router
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Printers and printing
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"


BOOL WINAPI
ClosePrinter(HANDLE hPrinter)
{
    BOOL bReturnValue;
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // FIXME: Call FindClosePrinterChangeNotification for all created change notifications (according to MSDN).

    // Call CloseHandle of the Print Provider.
    bReturnValue = pHandle->pPrintProvider->PrintProvider.fpClosePrinter(pHandle->hPrinter);

    // Free our handle information.
    DllFreeSplMem(pHandle);

    return bReturnValue;
}

BOOL WINAPI
EndDocPrinter(HANDLE hPrinter)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpEndDocPrinter(pHandle->hPrinter);
}

BOOL WINAPI
EndPagePrinter(HANDLE hPrinter)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpEndPagePrinter(pHandle->hPrinter);
}

BOOL WINAPI
EnumPrintersW(DWORD Flags, PWSTR Name, DWORD Level, PBYTE pPrinterEnum, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    BOOL bReturnValue;
    DWORD cbCallBuffer;
    DWORD cbNeeded;
    DWORD dwReturned;
    PBYTE pCallBuffer;
    PSPOOLSS_PRINT_PROVIDER pPrintProvider;
    PLIST_ENTRY pEntry;

    // Sanity checks.
    if ((cbBuf && !pPrinterEnum) || !pcbNeeded || !pcReturned)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Begin counting.
    *pcbNeeded = 0;
    *pcReturned = 0;

    // At the beginning, we have the full buffer available.
    cbCallBuffer = cbBuf;
    pCallBuffer = pPrinterEnum;

    // Loop through all Print Provider.
    for (pEntry = PrintProviderList.Flink; pEntry != &PrintProviderList; pEntry = pEntry->Flink)
    {
        pPrintProvider = CONTAINING_RECORD(pEntry, SPOOLSS_PRINT_PROVIDER, Entry);

        // Call the EnumPrinters function of this Print Provider.
        bReturnValue = pPrintProvider->PrintProvider.fpEnumPrinters(Flags, Name, Level, pCallBuffer, cbCallBuffer, &cbNeeded, &dwReturned);

        // Add the returned counts to the total values.
        *pcbNeeded += cbNeeded;
        *pcReturned += dwReturned;

        // Reduce the available buffer size for the next call without risking an underflow.
        if (cbNeeded < cbCallBuffer)
            cbCallBuffer -= cbNeeded;
        else
            cbCallBuffer = 0;

        // Advance the buffer if the caller provided it.
        if (pCallBuffer)
            pCallBuffer += cbNeeded;
    }

    return bReturnValue;
}

BOOL WINAPI
GetPrinterDriverW(HANDLE hPrinter, PWSTR pEnvironment, DWORD Level, PBYTE pDriverInfo, DWORD cbBuf, PDWORD pcbNeeded)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpGetPrinterDriver(pHandle->hPrinter, pEnvironment, Level, pDriverInfo, cbBuf, pcbNeeded);
}

BOOL WINAPI
GetPrinterW(HANDLE hPrinter, DWORD Level, PBYTE pPrinter, DWORD cbBuf, PDWORD pcbNeeded)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpGetPrinter(pHandle->hPrinter, Level, pPrinter, cbBuf, pcbNeeded);
}

BOOL WINAPI
OpenPrinterW(PWSTR pPrinterName, PHANDLE phPrinter, PPRINTER_DEFAULTSW pDefault)
{
    BOOL bReturnValue;
    HANDLE hPrinter;
    PLIST_ENTRY pEntry;
    PSPOOLSS_PRINTER_HANDLE pHandle;
    PSPOOLSS_PRINT_PROVIDER pPrintProvider;

    // Sanity checks.
    if (!pPrinterName || !phPrinter)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Loop through all Print Providers to find one able to open this Printer.
    for (pEntry = PrintProviderList.Flink; pEntry != &PrintProviderList; pEntry = pEntry->Flink)
    {
        pPrintProvider = CONTAINING_RECORD(pEntry, SPOOLSS_PRINT_PROVIDER, Entry);

        bReturnValue = pPrintProvider->PrintProvider.fpOpenPrinter(pPrinterName, &hPrinter, pDefault);
        if (bReturnValue == ROUTER_SUCCESS)
        {
            // This Print Provider has opened this Printer.
            // Store this information and return a handle.
            pHandle = DllAllocSplMem(sizeof(SPOOLSS_PRINTER_HANDLE));
            if (!pHandle)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                ERR("DllAllocSplMem failed with error %lu!\n", GetLastError());
                return FALSE;
            }

            pHandle->pPrintProvider = pPrintProvider;
            pHandle->hPrinter = hPrinter;
            *phPrinter = (HANDLE)pHandle;

            SetLastError(ERROR_SUCCESS);
            return TRUE;
        }
        else if (bReturnValue == ROUTER_STOP_ROUTING)
        {
            ERR("A Print Provider returned ROUTER_STOP_ROUTING for Printer \"%S\"!\n", pPrinterName);
            return FALSE;
        }
    }

    // We found no Print Provider able to open this Printer.
    return FALSE;
}

BOOL WINAPI
ReadPrinter(HANDLE hPrinter, PVOID pBuf, DWORD cbBuf, PDWORD pNoBytesRead)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpReadPrinter(pHandle->hPrinter, pBuf, cbBuf, pNoBytesRead);
}

DWORD WINAPI
StartDocPrinterW(HANDLE hPrinter, DWORD Level, PBYTE pDocInfo)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpStartDocPrinter(pHandle->hPrinter, Level, pDocInfo);
}

BOOL WINAPI
StartPagePrinter(HANDLE hPrinter)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpStartPagePrinter(pHandle->hPrinter);
}

BOOL WINAPI
WritePrinter(HANDLE hPrinter, PVOID pBuf, DWORD cbBuf, PDWORD pcWritten)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpWritePrinter(pHandle->hPrinter, pBuf, cbBuf, pcWritten);
}

BOOL WINAPI
XcvDataW(HANDLE hXcv, PCWSTR pszDataName, PBYTE pInputData, DWORD cbInputData, PBYTE pOutputData, DWORD cbOutputData, PDWORD pcbOutputNeeded, PDWORD pdwStatus)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hXcv;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpXcvData(pHandle->hPrinter, pszDataName, pInputData, cbInputData, pOutputData, cbOutputData, pcbOutputNeeded, pdwStatus);
}

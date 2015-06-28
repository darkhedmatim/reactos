/*
 * PROJECT:     ReactOS Local Spooler
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Main functions
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

// Global Variables
WCHAR wszSpoolDirectory[MAX_PATH];
DWORD cchSpoolDirectory;

// Global Constants
const WCHAR wszCurrentEnvironment[] =
#if defined(_X86_)
    L"Windows NT x86";
#elif defined(_AMD64_)
    L"Windows x64";
#elif defined(_ARM_)
    L"Windows ARM";
#else
    #error Unsupported architecture
#endif

const WCHAR wszDefaultDocumentName[] = L"Local Downlevel Document";

const WCHAR* wszPrintProviderInfo[3] = {
    L"Windows NT Local Print Providor",     // Name
    L"Windows NT Local Printers",           // Description
    L"Locally connected Printers"           // Comment
};

// Local Constants
static const PRINTPROVIDOR _PrintProviderFunctions = {
    LocalOpenPrinter,                           // fpOpenPrinter
    LocalSetJob,                                // fpSetJob
    LocalGetJob,                                // fpGetJob
    LocalEnumJobs,                              // fpEnumJobs
    NULL,                                       // fpAddPrinter
    NULL,                                       // fpDeletePrinter
    NULL,                                       // fpSetPrinter
    NULL,                                       // fpGetPrinter
    LocalEnumPrinters,                          // fpEnumPrinters
    NULL,                                       // fpAddPrinterDriver
    NULL,                                       // fpEnumPrinterDrivers
    NULL,                                       // fpGetPrinterDriver
    NULL,                                       // fpGetPrinterDriverDirectory
    NULL,                                       // fpDeletePrinterDriver
    NULL,                                       // fpAddPrintProcessor
    LocalEnumPrintProcessors,                   // fpEnumPrintProcessors
    LocalGetPrintProcessorDirectory,            // fpGetPrintProcessorDirectory
    NULL,                                       // fpDeletePrintProcessor
    LocalEnumPrintProcessorDatatypes,           // fpEnumPrintProcessorDatatypes
    LocalStartDocPrinter,                       // fpStartDocPrinter
    LocalStartPagePrinter,                      // fpStartPagePrinter
    LocalWritePrinter,                          // fpWritePrinter
    LocalEndPagePrinter,                        // fpEndPagePrinter
    NULL,                                       // fpAbortPrinter
    NULL,                                       // fpReadPrinter
    LocalEndDocPrinter,                         // fpEndDocPrinter
    LocalAddJob,                                // fpAddJob
    LocalScheduleJob,                           // fpScheduleJob
    NULL,                                       // fpGetPrinterData
    NULL,                                       // fpSetPrinterData
    NULL,                                       // fpWaitForPrinterChange
    LocalClosePrinter,                          // fpClosePrinter
    NULL,                                       // fpAddForm
    NULL,                                       // fpDeleteForm
    NULL,                                       // fpGetForm
    NULL,                                       // fpSetForm
    NULL,                                       // fpEnumForms
    NULL,                                       // fpEnumMonitors
    NULL,                                       // fpEnumPorts
    NULL,                                       // fpAddPort
    NULL,                                       // fpConfigurePort
    NULL,                                       // fpDeletePort
    NULL,                                       // fpCreatePrinterIC
    NULL,                                       // fpPlayGdiScriptOnPrinterIC
    NULL,                                       // fpDeletePrinterIC
    NULL,                                       // fpAddPrinterConnection
    NULL,                                       // fpDeletePrinterConnection
    NULL,                                       // fpPrinterMessageBox
    NULL,                                       // fpAddMonitor
    NULL,                                       // fpDeleteMonitor
    NULL,                                       // fpResetPrinter
    NULL,                                       // fpGetPrinterDriverEx
    NULL,                                       // fpFindFirstPrinterChangeNotification
    NULL,                                       // fpFindClosePrinterChangeNotification
    NULL,                                       // fpAddPortEx
    NULL,                                       // fpShutDown
    NULL,                                       // fpRefreshPrinterChangeNotification
    NULL,                                       // fpOpenPrinterEx
    NULL,                                       // fpAddPrinterEx
    NULL,                                       // fpSetPort
    NULL,                                       // fpEnumPrinterData
    NULL,                                       // fpDeletePrinterData
    NULL,                                       // fpClusterSplOpen
    NULL,                                       // fpClusterSplClose
    NULL,                                       // fpClusterSplIsAlive
    NULL,                                       // fpSetPrinterDataEx
    NULL,                                       // fpGetPrinterDataEx
    NULL,                                       // fpEnumPrinterDataEx
    NULL,                                       // fpEnumPrinterKey
    NULL,                                       // fpDeletePrinterDataEx
    NULL,                                       // fpDeletePrinterKey
    NULL,                                       // fpSeekPrinter
    NULL,                                       // fpDeletePrinterDriverEx
    NULL,                                       // fpAddPerMachineConnection
    NULL,                                       // fpDeletePerMachineConnection
    NULL,                                       // fpEnumPerMachineConnections
    NULL,                                       // fpXcvData
    NULL,                                       // fpAddPrinterDriverEx
    NULL,                                       // fpSplReadPrinter
    NULL,                                       // fpDriverUnloadComplete
    NULL,                                       // fpGetSpoolFileInfo
    NULL,                                       // fpCommitSpoolData
    NULL,                                       // fpCloseSpoolFileHandle
    NULL,                                       // fpFlushPrinter
    NULL,                                       // fpSendRecvBidiData
    NULL,                                       // fpAddDriverCatalog
};

static void
_GetSpoolDirectory()
{
    const WCHAR wszSpoolPath[] = L"\\spool";
    const DWORD cchSpoolPath = _countof(wszSpoolPath) - 1;

    // Get the system directory and append the "spool" subdirectory.
    // Forget about length checks here. If this doesn't fit into MAX_PATH, our OS has more serious problems...
    cchSpoolDirectory = GetSystemDirectoryW(wszSpoolDirectory, MAX_PATH);
    CopyMemory(&wszSpoolDirectory[cchSpoolDirectory], wszSpoolPath, (cchSpoolPath + 1) * sizeof(WCHAR));
    cchSpoolDirectory += cchSpoolPath;
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            _GetSpoolDirectory();
            InitializePrintProcessorList();
            InitializePrinterList();
            InitializeGlobalJobList();
            break;
    }

    return TRUE;
}

BOOL WINAPI
InitializePrintProvidor(LPPRINTPROVIDOR pPrintProvidor, DWORD cbPrintProvidor, LPWSTR pFullRegistryPath)
{
    CopyMemory(pPrintProvidor, &_PrintProviderFunctions, min(cbPrintProvidor, sizeof(PRINTPROVIDOR)));

    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

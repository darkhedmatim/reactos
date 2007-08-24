/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/start.c
 * PURPOSE:     Start a service
 * COPYRIGHT:   Copyright 2005-2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

static BOOL
DoStartService(PMAIN_WND_INFO Info,
               HWND hProgDlg)
{
    SC_HANDLE hSCManager;
    SC_HANDLE hSc;
    SERVICE_STATUS_PROCESS ServiceStatus;
    DWORD BytesNeeded = 0;
    INT ArgCount = 0;
    DWORD dwStartTickCount, dwOldCheckPoint;

    /* open handle to the SCM */
    hSCManager = OpenSCManager(NULL,
                               NULL,
                               SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL)
    {
        GetError();
        return FALSE;
    }

    /* get a handle to the service requested for starting */
    hSc = OpenService(hSCManager,
                      Info->CurrentService->lpServiceName,
                      SERVICE_ALL_ACCESS);
    if (hSc == NULL)
    {
        GetError();
        return FALSE;
    }

    /* start the service opened */
    if (! StartService(hSc,
                       ArgCount,
                       NULL))
    {
        GetError();
        return FALSE;
    }

    /* query the state of the service */
    if (! QueryServiceStatusEx(hSc,
                               SC_STATUS_PROCESS_INFO,
                               (LPBYTE)&ServiceStatus,
                               sizeof(SERVICE_STATUS_PROCESS),
                               &BytesNeeded))
    {
        GetError();
        return FALSE;
    }

    /* Save the tick count and initial checkpoint. */
    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ServiceStatus.dwCheckPoint;

    /* loop whilst service is not running */
    /* FIXME: needs more control adding. 'Loop' is temparary */
    while (ServiceStatus.dwCurrentState != SERVICE_RUNNING)
    {
        DWORD dwWaitTime;

        dwWaitTime = ServiceStatus.dwWaitHint / 10;

        if( dwWaitTime < 500 )
            dwWaitTime = 500;
        else if ( dwWaitTime > 5000 )
            dwWaitTime = 5000;

        IncrementProgressBar(hProgDlg);

        /* wait before checking status */
        Sleep(ServiceStatus.dwWaitHint / 8);

        /* check status again */
        if (! QueryServiceStatusEx(hSc,
                                   SC_STATUS_PROCESS_INFO,
                                   (LPBYTE)&ServiceStatus,
                                   sizeof(SERVICE_STATUS_PROCESS),
                                   &BytesNeeded))
        {
            GetError();
            return FALSE;
        }

        if (ServiceStatus.dwCheckPoint > dwOldCheckPoint)
        {
            /* The service is making progress. increment the progress bar */
            IncrementProgressBar(hProgDlg);
            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ServiceStatus.dwCheckPoint;
        }
        else
        {
            if(GetTickCount() - dwStartTickCount > ServiceStatus.dwWaitHint)
            {
                /* No progress made within the wait hint */
                break;
            }
        }
    }

    CloseServiceHandle(hSc);

    if (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
    {
        CompleteProgressBar(hProgDlg);
        Sleep(1000);
        return TRUE;
    }
    else
        return FALSE;
}


BOOL
DoStart(PMAIN_WND_INFO Info)
{
    HWND hProgDlg;
    BOOL bRet = FALSE;

    hProgDlg = CreateProgressDialog(Info->hMainWnd,
                                    Info->CurrentService->lpServiceName);

    if (hProgDlg)
    {
        bRet = DoStartService(Info,
                              hProgDlg);

        SendMessage(hProgDlg,
                    WM_DESTROY,
                    0,
                    0);
    }

    return bRet;
}

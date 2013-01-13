/*
 * Copyright (C) 2002 Andreas Mohr
 * Copyright (C) 2002 Shachar Shemesh
 * Copyright (C) 2013 Edijs Kolesnikovics
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* Based on the Wine "bootup" handler application
 *
 * This app handles the various "hooks" windows allows for applications to perform
 * as part of the bootstrap process. Theses are roughly devided into three types.
 * Knowledge base articles that explain this are 137367, 179365, 232487 and 232509.
 * Also, 119941 has some info on grpconv.exe
 * The operations performed are (by order of execution):
 *
 * Startup (before the user logs in)
 * - Services (NT, ?semi-synchronous?, not implemented yet)
 * - HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\RunServicesOnce (9x, asynch)
 * - HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\RunServices (9x, asynch)
 *
 * After log in
 * - HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\RunOnce (all, synch)
 * - HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Run (all, asynch)
 * - HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run (all, asynch)
 * - Startup folders (all, ?asynch?, no imp)
 * - HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\RunOnce (all, asynch)
 *
 * Somewhere in there is processing the RunOnceEx entries (also no imp)
 */

#include "precomp.h"

EXTERN_C HRESULT WINAPI SHCreateSessionKey(REGSAM samDesired, PHKEY phKey);

#define INVALID_RUNCMD_RETURN -1
/**
 * This function runs the specified command in the specified dir.
 * [in,out] cmdline - the command line to run. The function may change the passed buffer.
 * [in] dir - the dir to run the command in. If it is NULL, then the current dir is used.
 * [in] wait - whether to wait for the run program to finish before returning.
 * [in] minimized - Whether to ask the program to run minimized.
 *
 * Returns:
 * If running the process failed, returns INVALID_RUNCMD_RETURN. Use GetLastError to get the error code.
 * If wait is FALSE - returns 0 if successful.
 * If wait is TRUE - returns the program's return value.
 */
static int runCmd(LPWSTR cmdline, LPCWSTR dir, BOOL wait, BOOL minimized)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    DWORD exit_code = 0;
    WCHAR szCmdLineExp[MAX_PATH+1] = L"\0";

    ExpandEnvironmentStringsW(cmdline, szCmdLineExp, sizeof(szCmdLineExp) / sizeof(WCHAR));

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    if (minimized)
    {
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_MINIMIZE;
    }
    memset(&info, 0, sizeof(info));

    if (!CreateProcessW(NULL, szCmdLineExp, NULL, NULL, FALSE, 0, NULL, dir, &si, &info))
    {
        printf("Failed to run command (%ld)\n", GetLastError());

        return INVALID_RUNCMD_RETURN;
    }

    printf("Successfully ran command\n");

    if (wait)
    {   /* wait for the process to exit */
        WaitForSingleObject(info.hProcess, INFINITE);
        GetExitCodeProcess(info.hProcess, &exit_code);
    }

    CloseHandle(info.hThread);
    CloseHandle(info.hProcess);

    return exit_code;
}


/**
 * Process a "Run" type registry key.
 * hkRoot is the HKEY from which "Software\Microsoft\Windows\CurrentVersion" is
 *      opened.
 * szKeyName is the key holding the actual entries.
 * bDelete tells whether we should delete each value right before executing it.
 * bSynchronous tells whether we should wait for the prog to complete before
 *      going on to the next prog.
 */
static BOOL ProcessRunKeys(HKEY hkRoot, LPCWSTR szKeyName, BOOL bDelete,
        BOOL bSynchronous)
{
    static const WCHAR WINKEY_NAME[]={'S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\',
        'C','u','r','r','e','n','t','V','e','r','s','i','o','n',0};
    HKEY hkWin = NULL, hkRun=NULL;
    LONG res = ERROR_SUCCESS;
    DWORD i, nMaxCmdLine = 0, nMaxValue = 0;
    WCHAR *szCmdLine = NULL;
    WCHAR *szValue = NULL;

    if (hkRoot == HKEY_LOCAL_MACHINE)
        wprintf(L"processing %s entries under HKLM\n", szKeyName);
    else
        wprintf(L"processing %s entries under HKCU\n", szKeyName);

    if ((res = RegOpenKeyExW(hkRoot, WINKEY_NAME, 0, KEY_READ, &hkWin)) != ERROR_SUCCESS)
    {
        printf("RegOpenKey failed on Software\\Microsoft\\Windows\\CurrentVersion (%ld)\n",
                res);

        goto end;
    }

    if ((res = RegOpenKeyExW(hkWin, szKeyName, 0, bDelete?KEY_ALL_ACCESS:KEY_READ, &hkRun))!=
            ERROR_SUCCESS)
    {
        if (res == ERROR_FILE_NOT_FOUND)
        {
            printf("Key doesn't exist - nothing to be done\n");

            res = ERROR_SUCCESS;
        }
        else
            printf("RegOpenKey failed on run key (%ld)\n", res);

        goto end;
    }

    if ((res = RegQueryInfoKeyW(hkRun, NULL, NULL, NULL, NULL, NULL, NULL, &i, &nMaxValue,
                    &nMaxCmdLine, NULL, NULL)) != ERROR_SUCCESS)
    {
        printf("Couldn't query key info (%ld)\n", res);

        goto end;
    }

    if (i == 0)
    {
        printf("No commands to execute.\n");

        res = ERROR_SUCCESS;
        goto end;
    }

    if ((szCmdLine = malloc(nMaxCmdLine)) == NULL)
    {
        printf("Couldn't allocate memory for the commands to be executed\n");

        res = ERROR_NOT_ENOUGH_MEMORY;
        goto end;
    }

    if ((szValue = malloc((++nMaxValue)*sizeof(*szValue))) == NULL)
    {
        printf("Couldn't allocate memory for the value names\n");

        free(szCmdLine);
        res = ERROR_NOT_ENOUGH_MEMORY;
        goto end;
    }

    while(i > 0)
    {
        DWORD nValLength=nMaxValue, nDataLength=nMaxCmdLine;
        DWORD type;

        --i;

        if ((res = RegEnumValueW(hkRun, i, szValue, &nValLength, 0, &type,
                        (LPBYTE)szCmdLine, &nDataLength)) != ERROR_SUCCESS)
        {
            printf("Couldn't read in value %ld - %ld\n", i, res);

            continue;
        }

        /* safe mode - force to run if prefixed with asterisk */
        if (GetSystemMetrics(SM_CLEANBOOT) && (szValue[0] != L'*')) continue;

        if (bDelete && (res = RegDeleteValueW(hkRun, szValue)) != ERROR_SUCCESS)
        {
            printf("Couldn't delete value - %ld, %ld. Running command anyways.\n", i, res);
        }

        if (type != REG_SZ)
        {
            printf("Incorrect type of value #%ld (%ld)\n", i, type);

            continue;
        }

        if ((res = runCmd(szCmdLine, NULL, bSynchronous, FALSE)) == INVALID_RUNCMD_RETURN)
        {
            printf("Error running cmd #%ld (%ld)\n", i, GetLastError());
        }

        printf("Done processing cmd #%ld\n", i);
    }

    free(szValue);
    free(szCmdLine);
    res = ERROR_SUCCESS;

end:
    if (hkRun != NULL)
        RegCloseKey(hkRun);
    if (hkWin != NULL)
        RegCloseKey(hkWin);

    printf("done\n");

    return res == ERROR_SUCCESS ? TRUE : FALSE;
}


int
ProcessStartupItems(VOID)
{
    BOOL bNormalBoot = GetSystemMetrics(SM_CLEANBOOT) == 0; /* Perform the operations that are performed every boot */
    /* First, set the current directory to SystemRoot */
    TCHAR gen_path[MAX_PATH];
    DWORD res;
    HKEY hSessionKey, hKey;
    HRESULT hr;

    res = GetWindowsDirectory(gen_path, sizeof(gen_path));

    if (res == 0)
    {
        printf("Couldn't get the windows directory - error %ld\n",
            GetLastError());

        return 100;
    }

    if (!SetCurrentDirectory(gen_path))
    {
        wprintf(L"Cannot set the dir to %s (%ld)\n", gen_path, GetLastError());

        return 100;
    }

    hr = SHCreateSessionKey(KEY_WRITE, &hSessionKey);
    if (SUCCEEDED(hr))
    {
        LONG Error;
        DWORD dwDisp;

        Error = RegCreateKeyEx(hSessionKey, L"StartupHasBeenRun", 0, NULL, REG_OPTION_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisp);
        RegCloseKey(hSessionKey);
        if (Error == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            if (dwDisp == REG_OPENED_EXISTING_KEY)
            {
                /* Startup programs has already been run */
                return 0;
            }
        }
    }

    /* Perform the operations by order checking if policy allows it, checking if this is not Safe Mode,
     * stopping if one fails, skipping if necessary.
    */
    res = TRUE;
    if (res && (SHRestricted(REST_NOLOCALMACHINERUNONCE) == 0))
        res = ProcessRunKeys(HKEY_LOCAL_MACHINE, L"RunOnce", TRUE, TRUE);

    if (res && bNormalBoot && (SHRestricted(REST_NOLOCALMACHINERUN) == 0))
        res = ProcessRunKeys(HKEY_LOCAL_MACHINE, L"Run", FALSE, FALSE);

    if (res && bNormalBoot && (SHRestricted(REST_NOCURRENTUSERRUNONCE) == 0))
        res = ProcessRunKeys(HKEY_CURRENT_USER, L"Run", FALSE, FALSE);

    if (res && bNormalBoot && (SHRestricted(REST_NOCURRENTUSERRUNONCE) == 0))
        res = ProcessRunKeys(HKEY_CURRENT_USER, L"RunOnce", TRUE, FALSE);

    printf("Operation done\n");

    return res ? 0 : 101;
}

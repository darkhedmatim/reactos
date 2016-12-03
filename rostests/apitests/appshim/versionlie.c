/*
 * Copyright 2015 Mark Jansen
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

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#ifdef __REACTOS__
#include <ntndk.h>
#else
#include <winternl.h>
#endif
#include <stdio.h>
#include "wine/test.h"

typedef struct tagHOOKAPI {
    PCSTR LibraryName;
    PCSTR FunctionName;
    PVOID ReplacementFunction;
    PVOID OriginalFunction;
    PVOID Unk1;
    PVOID Unk2;
} HOOKAPI, *PHOOKAPI;

static BOOL (WINAPI* pSdbGetAppPatchDir)(PVOID,LPWSTR,DWORD);
static PHOOKAPI (WINAPI* pGetHookAPIs)(LPCSTR,LPCWSTR,PDWORD);


static DWORD g_WinVersion;
#define WINVER_WINXP   0x0501


typedef struct VersionLieInfo
{
    DWORD FullVersion;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformId;
    WORD wServicePackMajor;
    WORD wServicePackMinor;
} VersionLieInfo;

typedef BOOL(WINAPI* GETVERSIONEXAPROC)(LPOSVERSIONINFOEXA);
typedef BOOL(WINAPI* GETVERSIONEXWPROC)(LPOSVERSIONINFOEXW);
typedef DWORD(WINAPI* GETVERSIONPROC)(void);

static void expect_shim_imp(PHOOKAPI hook, PCSTR library, PCSTR function, PCSTR shim, int* same)
{
    int lib = lstrcmpA(library, hook->LibraryName);
    int fn = lstrcmpA(function, hook->FunctionName);
    winetest_ok(lib == 0, "Expected LibrarayName to be %s, was: %s for %s\n", library, hook->LibraryName, shim);
    winetest_ok(fn == 0, "Expected FunctionName to be %s, was: %s for %s\n", function, hook->FunctionName, shim);
    *same = (lib == 0 && fn == 0);
}

static void verify_shima_imp(PHOOKAPI hook, const VersionLieInfo* info, PCSTR shim, int same)
{
    OSVERSIONINFOEXA v1 = { sizeof(v1), 0 }, v2 = { sizeof(v2), 0 };
    BOOL ok1, ok2;

    if (!same)
    {
        skip("Skipping implementation tests for %s\n", shim);
        return;
    }

    while (v1.dwOSVersionInfoSize)
    {
        ok1 = GetVersionExA((LPOSVERSIONINFOA)&v1), ok2;
        hook->OriginalFunction = GetVersionExA;

        ok2 = ((GETVERSIONEXAPROC)hook->ReplacementFunction)(&v2);

        winetest_ok(ok1 == ok2, "Expected ok1 to equal ok2, was: %i, %i for %s\n", ok1, ok2, shim);
        if (ok1 && ok2)
        {
            char szCSDVersion[128] = "";
            winetest_ok(v1.dwOSVersionInfoSize == v2.dwOSVersionInfoSize, "Expected dwOSVersionInfoSize to be equal, was: %u, %u for %s\n", v1.dwOSVersionInfoSize, v2.dwOSVersionInfoSize, shim);
            winetest_ok(info->dwMajorVersion == v2.dwMajorVersion, "Expected dwMajorVersion to be equal, was: %u, %u for %s\n", info->dwMajorVersion, v2.dwMajorVersion, shim);
            winetest_ok(info->dwMinorVersion == v2.dwMinorVersion, "Expected dwMinorVersion to be equal, was: %u, %u for %s\n", info->dwMinorVersion, v2.dwMinorVersion, shim);
            winetest_ok(info->dwBuildNumber == v2.dwBuildNumber, "Expected dwBuildNumber to be equal, was: %u, %u for %s\n", info->dwBuildNumber, v2.dwBuildNumber, shim);
            winetest_ok(info->dwPlatformId == v2.dwPlatformId, "Expected dwPlatformId to be equal, was: %u, %u for %s\n", info->dwPlatformId, v2.dwPlatformId, shim);

            if (info->wServicePackMajor)
                sprintf(szCSDVersion, "Service Pack %u", info->wServicePackMajor);
            winetest_ok(lstrcmpA(szCSDVersion, v2.szCSDVersion) == 0, "Expected szCSDVersion to be equal, was: %s, %s for %s\n", szCSDVersion, v2.szCSDVersion, shim);

            if (v1.dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXA))
            {
                if (info->dwPlatformId != VER_PLATFORM_WIN32_WINDOWS)
                {
                    winetest_ok(info->wServicePackMajor == v2.wServicePackMajor, "Expected wServicePackMajor to be equal, was: %i, %i for %s\n", info->wServicePackMajor, v2.wServicePackMajor, shim);
                    winetest_ok(info->wServicePackMinor == v2.wServicePackMinor, "Expected wServicePackMinor to be equal, was: %i, %i for %s\n", info->wServicePackMinor, v2.wServicePackMinor, shim);
                }
                else
                {
                    winetest_ok(v1.wServicePackMajor == v2.wServicePackMajor, "Expected wServicePackMajor to be equal, was: %i, %i for %s\n", v1.wServicePackMajor, v2.wServicePackMajor, shim);
                    winetest_ok(v1.wServicePackMinor == v2.wServicePackMinor, "Expected wServicePackMinor to be equal, was: %i, %i for %s\n", v1.wServicePackMinor, v2.wServicePackMinor, shim);
                }
                winetest_ok(v1.wSuiteMask == v2.wSuiteMask, "Expected wSuiteMask to be equal, was: %i, %i for %s\n", v1.wSuiteMask, v2.wSuiteMask, shim);
                winetest_ok(v1.wProductType == v2.wProductType, "Expected wProductType to be equal, was: %i, %i for %s\n", v1.wProductType, v2.wProductType, shim);
                winetest_ok(v1.wReserved == v2.wReserved, "Expected wReserved to be equal, was: %i, %i for %s\n", v1.wReserved, v2.wReserved, shim);
            }
            else
            {
                winetest_ok(v1.wServicePackMajor == 0 && v2.wServicePackMajor == 0, "Expected wServicePackMajor to be 0, was: %i, %i for %s\n", v1.wServicePackMajor, v2.wServicePackMajor, shim);
                winetest_ok(v1.wServicePackMinor == 0 && v2.wServicePackMinor == 0, "Expected wServicePackMinor to be 0, was: %i, %i for %s\n", v1.wServicePackMinor, v2.wServicePackMinor, shim);
                winetest_ok(v1.wSuiteMask == 0 && v2.wSuiteMask == 0, "Expected wSuiteMask to be 0, was: %i, %i for %s\n", v1.wSuiteMask, v2.wSuiteMask, shim);
                winetest_ok(v1.wProductType == 0 && v2.wProductType == 0, "Expected wProductType to be 0, was: %i, %i for %s\n", v1.wProductType, v2.wProductType, shim);
                winetest_ok(v1.wReserved == 0 && v2.wReserved == 0, "Expected wReserved to be 0, was: %i, %i for %s\n", v1.wReserved, v2.wReserved, shim);
            }
        }

        ZeroMemory(&v1, sizeof(v1));
        ZeroMemory(&v2, sizeof(v2));
        if (v1.dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXA))
            v1.dwOSVersionInfoSize = v2.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    }
}

static void verify_shimw_imp(PHOOKAPI hook, const VersionLieInfo* info, PCSTR shim, int same, int first_might_be_broken)
{
    OSVERSIONINFOEXW v1 = { sizeof(v1), 0 }, v2 = { sizeof(v2), 0 };
    BOOL ok1, ok2, first = TRUE;

    if (!same)
    {
        skip("Skipping implementation tests for %s\n", shim);
        return;
    }

    while (v1.dwOSVersionInfoSize)
    {
        ok1 = GetVersionExW((LPOSVERSIONINFOW)&v1), ok2;
        hook->OriginalFunction = GetVersionExW;

        ok2 = ((GETVERSIONEXWPROC)hook->ReplacementFunction)(&v2);

        if (first_might_be_broken && first && ok1 == TRUE && ok2 == FALSE)
        {
            skip("Skipping first check because 0x%x is (falsely) not accepted by the shim %s\n", sizeof(v1), shim);
        }
        else
        {
            winetest_ok(ok1 == ok2, "Expected ok1 to equal ok2, was: %i, %i for %s(first:%d)\n", ok1, ok2, shim, first);
        }
        if (ok1 && ok2)
        {
            static const WCHAR szCSDFMT[] = {'S','e','r','v','i','c','e',' ','P','a','c','k',' ','%','u',0};
            WCHAR szCSDVersion[128] = { 0 };
            winetest_ok(v1.dwOSVersionInfoSize == v2.dwOSVersionInfoSize, "Expected dwOSVersionInfoSize to be equal, was: %u, %u for %s\n", v1.dwOSVersionInfoSize, v2.dwOSVersionInfoSize, shim);
            winetest_ok(info->dwMajorVersion == v2.dwMajorVersion, "Expected dwMajorVersion to be equal, was: %u, %u for %s\n", info->dwMajorVersion, v2.dwMajorVersion, shim);
            winetest_ok(info->dwMinorVersion == v2.dwMinorVersion, "Expected dwMinorVersion to be equal, was: %u, %u for %s\n", info->dwMinorVersion, v2.dwMinorVersion, shim);
            winetest_ok(info->dwBuildNumber == v2.dwBuildNumber, "Expected dwBuildNumber to be equal, was: %u, %u for %s\n", info->dwBuildNumber, v2.dwBuildNumber, shim);
            winetest_ok(info->dwPlatformId == v2.dwPlatformId, "Expected dwPlatformId to be equal, was: %u, %u for %s\n", info->dwPlatformId, v2.dwPlatformId, shim);

            if (info->wServicePackMajor)
                swprintf(szCSDVersion, szCSDFMT, info->wServicePackMajor);
            winetest_ok(lstrcmpW(szCSDVersion, v2.szCSDVersion) == 0, "Expected szCSDVersion to be equal, was: %s, %s for %s\n", wine_dbgstr_w(szCSDVersion), wine_dbgstr_w(v2.szCSDVersion), shim);

            if (v1.dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXW))
            {
                if (info->dwPlatformId != VER_PLATFORM_WIN32_WINDOWS)
                {
                    winetest_ok(info->wServicePackMajor == v2.wServicePackMajor, "Expected wServicePackMajor to be equal, was: %i, %i for %s\n", info->wServicePackMajor, v2.wServicePackMajor, shim);
                    winetest_ok(info->wServicePackMinor == v2.wServicePackMinor, "Expected wServicePackMinor to be equal, was: %i, %i for %s\n", info->wServicePackMinor, v2.wServicePackMinor, shim);
                }
                else
                {
                    winetest_ok(v1.wServicePackMajor == v2.wServicePackMajor, "Expected wServicePackMajor to be equal, was: %i, %i for %s\n", v1.wServicePackMajor, v2.wServicePackMajor, shim);
                    winetest_ok(v1.wServicePackMinor == v2.wServicePackMinor, "Expected wServicePackMinor to be equal, was: %i, %i for %s\n", v1.wServicePackMinor, v2.wServicePackMinor, shim);
                }
                winetest_ok(v1.wSuiteMask == v2.wSuiteMask, "Expected wSuiteMask to be equal, was: %i, %i for %s\n", v1.wSuiteMask, v2.wSuiteMask, shim);
                winetest_ok(v1.wProductType == v2.wProductType, "Expected wProductType to be equal, was: %i, %i for %s\n", v1.wProductType, v2.wProductType, shim);
                winetest_ok(v1.wReserved == v2.wReserved, "Expected wReserved to be equal, was: %i, %i for %s\n", v1.wReserved, v2.wReserved, shim);
            }
            else
            {
                winetest_ok(v1.wServicePackMajor == 0 && v2.wServicePackMajor == 0, "Expected wServicePackMajor to be 0, was: %i, %i for %s\n", v1.wServicePackMajor, v2.wServicePackMajor, shim);
                winetest_ok(v1.wServicePackMinor == 0 && v2.wServicePackMinor == 0, "Expected wServicePackMinor to be 0, was: %i, %i for %s\n", v1.wServicePackMinor, v2.wServicePackMinor, shim);
                winetest_ok(v1.wSuiteMask == 0 && v2.wSuiteMask == 0, "Expected wSuiteMask to be 0, was: %i, %i for %s\n", v1.wSuiteMask, v2.wSuiteMask, shim);
                winetest_ok(v1.wProductType == 0 && v2.wProductType == 0, "Expected wProductType to be 0, was: %i, %i for %s\n", v1.wProductType, v2.wProductType, shim);
                winetest_ok(v1.wReserved == 0 && v2.wReserved == 0, "Expected wReserved to be 0, was: %i, %i for %s\n", v1.wReserved, v2.wReserved, shim);
            }
        }

        ZeroMemory(&v1, sizeof(v1));
        ZeroMemory(&v2, sizeof(v2));
        if (v1.dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXW))
            v1.dwOSVersionInfoSize = v2.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
        first = FALSE;
    }
}

static void verify_shim_imp(PHOOKAPI hook, const VersionLieInfo* info, PCSTR shim, int same)
{
    DWORD ver;
    if (!same)
    {
        skip("Skipping implementation tests for %s\n", shim);
        return;
    }
    ver = ((GETVERSIONPROC)hook->ReplacementFunction)();
    winetest_ok(info->FullVersion == ver, "Expected GetVersion to return %u, was: %u for %s\n", info->FullVersion, ver, shim);
}


#define expect_shim  (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : expect_shim_imp
#define verify_shima  (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : verify_shima_imp
#define verify_shimw  (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : verify_shimw_imp
#define verify_shim  (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : verify_shim_imp




static void run_test(LPCSTR shim, const VersionLieInfo* info)
{
    DWORD num_shims = 0;
    WCHAR wide_shim[50] = { 0 };
    PHOOKAPI hook;
    DWORD ver;
    MultiByteToWideChar(CP_ACP, 0, shim, -1, wide_shim, 50);
    hook = pGetHookAPIs("", wide_shim, &num_shims);
    ver = (info->dwMajorVersion << 8) | info->dwMinorVersion;
    if (hook == NULL)
    {
        skip("Skipping tests for layers (%s) not present in this os (0x%x)\n", shim, g_WinVersion);
        return;
    }
    ok(hook != NULL, "Expected hook to be a valid pointer for %s\n", shim);
    if (info->dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
    {
        ok(num_shims == 3, "Expected num_shims to be 3, was: %u for %s\n", num_shims, shim);
        if (hook && num_shims == 3)
        {
            int off = info->dwBuildNumber == 0x040A08AE ? 0 : 1, same = 0;
            expect_shim(hook + ((0+off)%3), "KERNEL32.DLL", "GetVersionExA", shim, &same);
            verify_shima(hook + ((0+off)%3), info, shim, same);
            expect_shim(hook + ((1+off)%3), "KERNEL32.DLL", "GetVersionExW", shim, &same);
            verify_shimw(hook + ((1+off)%3), info, shim, same, 0);
            expect_shim(hook + ((2+off)%3), "KERNEL32.DLL", "GetVersion", shim, &same);
            verify_shim(hook + ((2+off)%3), info, shim, same);
        }
    }
    else
    {
        int shimnum_ok = num_shims == 4 || ((ver < WINVER_WINXP) && (num_shims == 3));
        ok(shimnum_ok, "Expected num_shims to be 4%s, was: %u for %s\n", ((ver < WINVER_WINXP) ? " or 3":""), num_shims, shim);
        if (hook && shimnum_ok)
        {
            int same = 0;
            expect_shim(hook + 0, "KERNEL32.DLL", "GetVersionExA", shim, &same);
            verify_shima(hook + 0, info, shim, same);
            expect_shim(hook + 1, "KERNEL32.DLL", "GetVersionExW", shim, &same);
            verify_shimw(hook + 1, info, shim, same, 0);
            expect_shim(hook + 2, "KERNEL32.DLL", "GetVersion", shim, &same);
            verify_shim(hook + 2, info, shim, same);
            if (num_shims == 4)
            {
                expect_shim(hook + 3, "NTDLL.DLL", "RtlGetVersion", shim, &same);
                verify_shimw(hook + 3, info, shim, same, 1);
            }
        }
    }
}


VersionLieInfo g_Win95 = { 0xC3B60004, 4, 0, 950, VER_PLATFORM_WIN32_WINDOWS, 0, 0 };
VersionLieInfo g_WinNT4SP5 = { 0x05650004, 4, 0, 1381, VER_PLATFORM_WIN32_NT, 5, 0 };
VersionLieInfo g_Win98 = { 0xC0000A04, 4, 10, 0x040A08AE, VER_PLATFORM_WIN32_WINDOWS, 0, 0 };

VersionLieInfo g_Win2000 = { 0x08930005, 5, 0, 2195, VER_PLATFORM_WIN32_NT, 0, 0 };
VersionLieInfo g_Win2000SP1 = { 0x08930005, 5, 0, 2195, VER_PLATFORM_WIN32_NT, 1, 0 };
VersionLieInfo g_Win2000SP2 = { 0x08930005, 5, 0, 2195, VER_PLATFORM_WIN32_NT, 2, 0 };
VersionLieInfo g_Win2000SP3 = { 0x08930005, 5, 0, 2195, VER_PLATFORM_WIN32_NT, 3, 0 };

VersionLieInfo g_WinXP = { 0x0a280105, 5, 1, 2600, VER_PLATFORM_WIN32_NT, 0, 0 };
VersionLieInfo g_WinXPSP1 = { 0x0a280105, 5, 1, 2600, VER_PLATFORM_WIN32_NT, 1, 0 };
VersionLieInfo g_WinXPSP2 = { 0x0a280105, 5, 1, 2600, VER_PLATFORM_WIN32_NT, 2, 0 };
VersionLieInfo g_WinXPSP3 = { 0x0a280105, 5, 1, 2600, VER_PLATFORM_WIN32_NT, 3, 0 };

VersionLieInfo g_Win2k3RTM = { 0x0ece0205, 5, 2, 3790, VER_PLATFORM_WIN32_NT, 0, 0 };
VersionLieInfo g_Win2k3SP1 = { 0x0ece0205, 5, 2, 3790, VER_PLATFORM_WIN32_NT, 1, 0 };

VersionLieInfo g_WinVistaRTM = { 0x17700006, 6, 0, 6000, VER_PLATFORM_WIN32_NT, 0, 0 };
VersionLieInfo g_WinVistaSP1 = { 0x17710006, 6, 0, 6001, VER_PLATFORM_WIN32_NT, 1, 0 };
VersionLieInfo g_WinVistaSP2 = { 0x17720006, 6, 0, 6002, VER_PLATFORM_WIN32_NT, 2, 0 };

VersionLieInfo g_Win7RTM = { 0x1db00106, 6, 1, 7600, VER_PLATFORM_WIN32_NT, 0, 0 };

DWORD get_host_winver(void)
{
    RTL_OSVERSIONINFOEXW rtlinfo = {0};
    void (__stdcall* pRtlGetVersion)(RTL_OSVERSIONINFOEXW*);
    pRtlGetVersion = (void (__stdcall*)(RTL_OSVERSIONINFOEXW*))GetProcAddress(GetModuleHandleA("ntdll"), "RtlGetVersion");

    rtlinfo.dwOSVersionInfoSize = sizeof(rtlinfo);
    pRtlGetVersion(&rtlinfo);
    return (rtlinfo.dwMajorVersion << 8) | rtlinfo.dwMinorVersion;
}

START_TEST(versionlie)
{
    HMODULE dll = LoadLibraryA("apphelp.dll");
    WCHAR buf[MAX_PATH];
    WCHAR aclayers[] = {'\\','a','c','l','a','y','e','r','s','.','d','l','l',0};
    pSdbGetAppPatchDir = (void*)GetProcAddress(dll, "SdbGetAppPatchDir");
    if (!pSdbGetAppPatchDir)
    {
        skip("apphelp.dll not loaded, or does not export SdbGetAppPatchDir\n");
        return;
    }

    pSdbGetAppPatchDir(NULL, buf, MAX_PATH);
    lstrcatW(buf, aclayers);
    dll = LoadLibraryW(buf);
    pGetHookAPIs = (void*)GetProcAddress(dll, "GetHookAPIs");

    if (!pGetHookAPIs)
    {
        skip("aclayers.dll not loaded, or does not export GetHookAPIs\n");
        return;
    }
    g_WinVersion = get_host_winver();

    run_test("Win95VersionLie", &g_Win95);
    run_test("WinNT4SP5VersionLie", &g_WinNT4SP5);
    run_test("Win98VersionLie", &g_Win98);
    run_test("Win2000VersionLie", &g_Win2000);
    run_test("Win2000SP1VersionLie", &g_Win2000SP1);
    run_test("Win2000SP2VersionLie", &g_Win2000SP2);
    run_test("Win2000SP3VersionLie", &g_Win2000SP3);
    run_test("WinXPVersionLie", &g_WinXP);
    run_test("WinXPSP1VersionLie", &g_WinXPSP1);
    run_test("WinXPSP2VersionLie", &g_WinXPSP2);
    run_test("WinXPSP3VersionLie", &g_WinXPSP3);
    run_test("Win2k3RTMVersionLie", &g_Win2k3RTM);
    run_test("Win2k3SP1VersionLie", &g_Win2k3SP1);
    run_test("VistaRTMVersionLie", &g_WinVistaRTM);
    run_test("VistaSP1VersionLie", &g_WinVistaSP1);
    run_test("VistaSP2VersionLie", &g_WinVistaSP2);
    run_test("Win7RTMVersionLie", &g_Win7RTM);
}

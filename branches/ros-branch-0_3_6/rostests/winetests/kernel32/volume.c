/*
 * Unit test suite
 *
 * Copyright 2006 Stefan Leichter
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

#include "wine/test.h"
#include "winbase.h"

static HINSTANCE hdll;
static BOOL (WINAPI * pGetVolumeNameForVolumeMountPointA)(LPCSTR, LPSTR, DWORD);
static BOOL (WINAPI * pGetVolumeNameForVolumeMountPointW)(LPCWSTR, LPWSTR, DWORD);
static HANDLE (WINAPI *pFindFirstVolumeA)(LPSTR,DWORD);
static BOOL (WINAPI *pFindNextVolumeA)(HANDLE,LPSTR,DWORD);
static BOOL (WINAPI *pFindVolumeClose)(HANDLE);

/* ############################### */

static void test_query_dos_deviceA(void)
{
    char drivestr[] = "a:";
    char *p, buffer[2000];
    DWORD ret;
    BOOL found = FALSE;

    for (;drivestr[0] <= 'z'; drivestr[0]++) {
        ret = QueryDosDeviceA( drivestr, buffer, sizeof(buffer));
        if(ret) {
            for (p = buffer; *p; p++) *p = toupper(*p);
            if (strstr(buffer, "HARDDISK") || strstr(buffer, "RAMDISK")) found = TRUE;
        }
    }
    todo_wine ok(found, "expected at least one devicename to contain HARDDISK or RAMDISK\n");
}

static void test_FindFirstVolume(void)
{
    char volume[51];
    HANDLE handle;

    if (!pFindFirstVolumeA) {
        skip("FindFirstVolumeA not found\n");
        return;
    }

    handle = pFindFirstVolumeA( volume, 0 );
    ok( handle == INVALID_HANDLE_VALUE, "succeeded with short buffer\n" );
    ok( GetLastError() == ERROR_MORE_DATA ||  /* XP */
        GetLastError() == ERROR_FILENAME_EXCED_RANGE,  /* Vista */
        "wrong error %u\n", GetLastError() );
    handle = pFindFirstVolumeA( volume, 49 );
    ok( handle == INVALID_HANDLE_VALUE, "succeeded with short buffer\n" );
    ok( GetLastError() == ERROR_FILENAME_EXCED_RANGE, "wrong error %u\n", GetLastError() );
    handle = pFindFirstVolumeA( volume, 51 );
    ok( handle != INVALID_HANDLE_VALUE, "failed err %u\n", GetLastError() );
    if (handle != INVALID_HANDLE_VALUE)
    {
        do
        {
            ok( strlen(volume) == 49, "bad volume name %s\n", volume );
            ok( !memcmp( volume, "\\\\?\\Volume{", 11 ), "bad volume name %s\n", volume );
            ok( !memcmp( volume + 47, "}\\", 2 ), "bad volume name %s\n", volume );
        } while (pFindNextVolumeA( handle, volume, MAX_PATH ));
        ok( GetLastError() == ERROR_NO_MORE_FILES, "wrong error %u\n", GetLastError() );
        pFindVolumeClose( handle );
    }
}

static void test_GetVolumeNameForVolumeMountPointA(void)
{
    BOOL ret;
    char volume[MAX_PATH], path[] = "c:\\";
    DWORD len = sizeof(volume);

    /* not present before w2k */
    if (!pGetVolumeNameForVolumeMountPointA) {
        skip("GetVolumeNameForVolumeMountPointA not found\n");
        return;
    }

    ret = pGetVolumeNameForVolumeMountPointA(path, volume, 0);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointA succeeded\n");

    if (0) { /* these crash on XP */
    ret = pGetVolumeNameForVolumeMountPointA(path, NULL, len);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointA succeeded\n");

    ret = pGetVolumeNameForVolumeMountPointA(NULL, volume, len);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointA succeeded\n");
    }

    ret = pGetVolumeNameForVolumeMountPointA(path, volume, len);
    ok(ret == TRUE, "GetVolumeNameForVolumeMountPointA failed\n");
}

static void test_GetVolumeNameForVolumeMountPointW(void)
{
    BOOL ret;
    WCHAR volume[MAX_PATH], path[] = {'c',':','\\',0};
    DWORD len = sizeof(volume) / sizeof(WCHAR);

    /* not present before w2k */
    if (!pGetVolumeNameForVolumeMountPointW) {
        skip("GetVolumeNameForVolumeMountPointW not found\n");
        return;
    }

    ret = pGetVolumeNameForVolumeMountPointW(path, volume, 0);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointA succeeded\n");

    if (0) { /* these crash on XP */
    ret = pGetVolumeNameForVolumeMountPointW(path, NULL, len);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointW succeeded\n");

    ret = pGetVolumeNameForVolumeMountPointW(NULL, volume, len);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointW succeeded\n");
    }

    ret = pGetVolumeNameForVolumeMountPointW(path, volume, len);
    ok(ret == TRUE, "GetVolumeNameForVolumeMountPointW failed\n");
}

START_TEST(volume)
{
    hdll = GetModuleHandleA("kernel32.dll");
    pGetVolumeNameForVolumeMountPointA = (void *) GetProcAddress(hdll, "GetVolumeNameForVolumeMountPointA");
    pGetVolumeNameForVolumeMountPointW = (void *) GetProcAddress(hdll, "GetVolumeNameForVolumeMountPointW");
    pFindFirstVolumeA = (void *) GetProcAddress(hdll, "FindFirstVolumeA");
    pFindNextVolumeA = (void *) GetProcAddress(hdll, "FindNextVolumeA");
    pFindVolumeClose = (void *) GetProcAddress(hdll, "FindVolumeClose");

    test_query_dos_deviceA();
    test_FindFirstVolume();
    test_GetVolumeNameForVolumeMountPointA();
    test_GetVolumeNameForVolumeMountPointW();
}

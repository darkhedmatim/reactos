/*
 * comctl32 MRU unit tests
 *
 * Copyright (C) 2004 Jon Griffiths <jon_p_griffiths@yahoo.com>
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

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "commctrl.h"
#include "shlwapi.h"

#include "wine/test.h"

/* Keys for testing MRU functions */
#define REG_TEST_BASEKEYA    "Software\\Wine"
#define REG_TEST_BASESUBKEYA "Test"
#define REG_TEST_KEYA    REG_TEST_BASEKEYA "\\" REG_TEST_BASESUBKEYA
#define REG_TEST_SUBKEYA "MRUTest"
#define REG_TEST_FULLKEY REG_TEST_KEYA "\\" REG_TEST_SUBKEYA

/* Undocumented MRU structures & functions */
typedef struct tagCREATEMRULISTA
{
    DWORD   cbSize;
    DWORD   nMaxItems;
    DWORD   dwFlags;
    HKEY    hKey;
    LPCSTR  lpszSubKey;
    PROC    lpfnCompare;
} CREATEMRULISTA, *LPCREATEMRULISTA;

#define MRUF_STRING_LIST  0
#define MRUF_BINARY_LIST  1
#define MRUF_DELAYED_SAVE 2

#define LIST_SIZE 3 /* Max entries for each mru */

static CREATEMRULISTA mruA =
{
    sizeof(CREATEMRULISTA),
    LIST_SIZE,
    0,
    NULL,
    REG_TEST_SUBKEYA,
    NULL
};

static HMODULE hComctl32;
static HANDLE (WINAPI *pCreateMRUListA)(LPCREATEMRULISTA);
static void   (WINAPI *pFreeMRUList)(HANDLE);
static INT    (WINAPI *pAddMRUStringA)(HANDLE,LPCSTR);
static INT    (WINAPI *pEnumMRUList)(HANDLE,INT,LPVOID,DWORD);
static INT    (WINAPI *pEnumMRUListW)(HANDLE,INT,LPVOID,DWORD);
static HANDLE (WINAPI *pCreateMRUListLazyA)(LPCREATEMRULISTA, DWORD, DWORD, DWORD);
static INT    (WINAPI *pFindMRUData)(HANDLE, LPCVOID, DWORD, LPINT);
static INT    (WINAPI *pAddMRUData)(HANDLE, LPCVOID, DWORD);
/*
static INT    (WINAPI *pFindMRUStringA)(HANDLE,LPCSTR,LPINT);
*/


static void InitPointers(void)
{
    pCreateMRUListA = (void*)GetProcAddress(hComctl32,(LPCSTR)151);
    pFreeMRUList    = (void*)GetProcAddress(hComctl32,(LPCSTR)152);
    pAddMRUStringA  = (void*)GetProcAddress(hComctl32,(LPCSTR)153);
    pEnumMRUList    = (void*)GetProcAddress(hComctl32,(LPCSTR)154);
    pCreateMRUListLazyA = (void*)GetProcAddress(hComctl32,(LPCSTR)157);
    pAddMRUData     = (void*)GetProcAddress(hComctl32,(LPCSTR)167);
    pFindMRUData    = (void*)GetProcAddress(hComctl32,(LPCSTR)169);
    pEnumMRUListW   = (void*)GetProcAddress(hComctl32,(LPCSTR)403);
}

/* Based on RegDeleteTreeW from dlls/advapi32/registry.c */
static LSTATUS mru_RegDeleteTreeA(HKEY hKey, LPCSTR lpszSubKey)
{
    LONG ret;
    DWORD dwMaxSubkeyLen, dwMaxValueLen;
    DWORD dwMaxLen, dwSize;
    CHAR szNameBuf[MAX_PATH], *lpszName = szNameBuf;
    HKEY hSubKey = hKey;

    if(lpszSubKey)
    {
        ret = RegOpenKeyExA(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);
        if (ret) return ret;
    }

    /* Get highest length for keys, values */
    ret = RegQueryInfoKeyA(hSubKey, NULL, NULL, NULL, NULL,
            &dwMaxSubkeyLen, NULL, NULL, &dwMaxValueLen, NULL, NULL, NULL);
    if (ret) goto cleanup;

    dwMaxSubkeyLen++;
    dwMaxValueLen++;
    dwMaxLen = max(dwMaxSubkeyLen, dwMaxValueLen);
    if (dwMaxLen > sizeof(szNameBuf)/sizeof(CHAR))
    {
        /* Name too big: alloc a buffer for it */
        if (!(lpszName = HeapAlloc( GetProcessHeap(), 0, dwMaxLen*sizeof(CHAR))))
        {
            ret = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }


    /* Recursively delete all the subkeys */
    while (TRUE)
    {
        dwSize = dwMaxLen;
        if (RegEnumKeyExA(hSubKey, 0, lpszName, &dwSize, NULL,
                          NULL, NULL, NULL)) break;

        ret = mru_RegDeleteTreeA(hSubKey, lpszName);
        if (ret) goto cleanup;
    }

    if (lpszSubKey)
        ret = RegDeleteKeyA(hKey, lpszSubKey);
    else
        while (TRUE)
        {
            dwSize = dwMaxLen;
            if (RegEnumValueA(hKey, 0, lpszName, &dwSize,
                  NULL, NULL, NULL, NULL)) break;

            ret = RegDeleteValueA(hKey, lpszName);
            if (ret) goto cleanup;
        }

cleanup:
    /* Free buffer if allocated */
    if (lpszName != szNameBuf)
        HeapFree( GetProcessHeap(), 0, lpszName);
    if(lpszSubKey)
        RegCloseKey(hSubKey);
    return ret;
}

static BOOL create_reg_entries(void)
{
    HKEY hKey = NULL;

    ok(!RegCreateKeyA(HKEY_CURRENT_USER, REG_TEST_FULLKEY, &hKey),
       "Couldn't create test key \"%s\"\n", REG_TEST_KEYA);
    if (!hKey) return FALSE;
    RegCloseKey(hKey);
    return TRUE;
}

static void delete_reg_entries(void)
{
    HKEY hKey;

    if (RegOpenKeyExA(HKEY_CURRENT_USER, REG_TEST_BASEKEYA, 0, KEY_ALL_ACCESS,
                      &hKey))
        return;
    mru_RegDeleteTreeA(hKey, REG_TEST_BASESUBKEYA);
    RegCloseKey(hKey);
}

static void check_reg_entries(const char *mrulist, const char**items)
{
    char buff[128];
    HKEY hKey = NULL;
    DWORD type, size, ret;
    unsigned int i;

    ok(!RegOpenKeyA(HKEY_CURRENT_USER, REG_TEST_FULLKEY, &hKey),
       "Couldn't open test key \"%s\"\n", REG_TEST_FULLKEY);
    if (!hKey) return;

    type = REG_SZ;
    size = sizeof(buff);
    buff[0] = '\0';
    ret = RegQueryValueExA(hKey, "MRUList", NULL, &type, (LPBYTE)buff, &size);

    ok(!ret && buff[0], "Checking MRU: got %d from RegQueryValueExW\n", ret);
    if(ret || !buff[0]) return;

    ok(strcmp(buff, mrulist) == 0, "Checking MRU: Expected list %s, got %s\n",
       mrulist, buff);
    if(strcmp(buff, mrulist)) return;

    for (i = 0; i < strlen(mrulist); i++)
    {
        char name[2];
        name[0] = mrulist[i];
        name[1] = '\0';
        type = REG_SZ;
        size = sizeof(buff);
        buff[0] = '\0';
        ret = RegQueryValueExA(hKey, name, NULL, &type, (LPBYTE)buff, &size);
        ok(!ret && buff[0],
           "Checking MRU item %d ('%c'): got %d from RegQueryValueExW\n",
           i, mrulist[i], ret);
        if(ret || !buff[0]) return;
        ok(!strcmp(buff, items[mrulist[i]-'a']),
           "Checking MRU item %d ('%c'): expected \"%s\", got \"%s\"\n",
           i, mrulist[i], buff, items[mrulist[i] - 'a']);
    }
}

static INT CALLBACK cmp_mru_strA(LPCVOID data1, LPCVOID data2)
{
    return lstrcmpiA(data1, data2);
}

static HANDLE create_mruA(HKEY hKey, DWORD flags, PROC cmp)
{
    mruA.dwFlags = flags;
    mruA.lpfnCompare = cmp;
    mruA.hKey = hKey;

    SetLastError(0);
    return pCreateMRUListA(&mruA);
}

static void test_MRUListA(void)
{
    const char *checks[LIST_SIZE+1];
    HANDLE hMRU;
    HKEY hKey;
    INT iRet;

    if (!pCreateMRUListA || !pFreeMRUList || !pAddMRUStringA || !pEnumMRUList)
    {
        skip("MRU entry points not found\n");
        return;
    }

    if (0)
    {
    /* Create (NULL) - crashes native */
    hMRU = pCreateMRUListA(NULL);
    }

    /* Create (size too small) */
    mruA.cbSize = sizeof(mruA) - 2;
    hMRU = create_mruA(NULL, MRUF_STRING_LIST, (PROC)cmp_mru_strA);
    ok (!hMRU && !GetLastError(),
        "CreateMRUListA(too small) expected NULL,0 got %p,%d\n",
        hMRU, GetLastError());
    mruA.cbSize = sizeof(mruA);

    /* Create (size too big) */
    mruA.cbSize = sizeof(mruA) + 2;
    hMRU = create_mruA(NULL, MRUF_STRING_LIST, (PROC)cmp_mru_strA);
    ok (!hMRU && !GetLastError(),
        "CreateMRUListA(too big) expected NULL,0 got %p,%d\n",
        hMRU, GetLastError());
    mruA.cbSize = sizeof(mruA);

    /* Create (NULL hKey) */
    hMRU = create_mruA(NULL, MRUF_STRING_LIST, (PROC)cmp_mru_strA);
    ok (!hMRU && !GetLastError(),
        "CreateMRUListA(NULL key) expected NULL,0 got %p,%d\n",
        hMRU, GetLastError());

    /* Create (NULL name) */
    mruA.lpszSubKey = NULL;
    hMRU = create_mruA(NULL, MRUF_STRING_LIST, (PROC)cmp_mru_strA);
    ok (!hMRU && !GetLastError(),
        "CreateMRUListA(NULL name) expected NULL,0 got %p,%d\n",
        hMRU, GetLastError());
    mruA.lpszSubKey = REG_TEST_SUBKEYA;

    /* Create a string MRU */
    ok(!RegCreateKeyA(HKEY_CURRENT_USER, REG_TEST_KEYA, &hKey),
       "Couldn't create test key \"%s\"\n", REG_TEST_KEYA);
    if (!hKey)
        return;
    hMRU = create_mruA(hKey, MRUF_STRING_LIST, (PROC)cmp_mru_strA);
    ok(hMRU && !GetLastError(),
       "CreateMRUListA(string) expected non-NULL,0 got %p,%d\n",
       hMRU, GetLastError());

    if (hMRU)
    {
        char buffer[255];
        checks[0] = "Test 1";
        checks[1] = "Test 2";
        checks[2] = "Test 3";
        checks[3] = "Test 4";

        /* Add (NULL list) */
        SetLastError(0);
        iRet = pAddMRUStringA(NULL, checks[0]);
        ok(iRet == -1 && !GetLastError(),
           "AddMRUStringA(NULL list) expected -1,0 got %d,%d\n",
           iRet, GetLastError());

        /* Add (NULL string) */
        if (0)
        {
	/* Some native versions crash when passed NULL or fail to SetLastError()  */
        SetLastError(0);
        iRet = pAddMRUStringA(hMRU, NULL);
        ok(iRet == 0 && GetLastError() == ERROR_INVALID_PARAMETER,
           "AddMRUStringA(NULL str) expected 0,ERROR_INVALID_PARAMETER got %d,%d\n",
           iRet, GetLastError());
        }

        /* Add 3 strings. Check the registry is correct after each add */
        SetLastError(0);
        iRet = pAddMRUStringA(hMRU, checks[0]);
        ok(iRet == 0 && !GetLastError(),
           "AddMRUStringA(1) expected 0,0 got %d,%d\n",
           iRet, GetLastError());
        check_reg_entries("a", checks);

        SetLastError(0);
        iRet = pAddMRUStringA(hMRU, checks[1]);
        ok(iRet == 1 && !GetLastError(),
           "AddMRUStringA(2) expected 1,0 got %d,%d\n",
           iRet, GetLastError());
        check_reg_entries("ba", checks);

        SetLastError(0);
        iRet = pAddMRUStringA(hMRU, checks[2]);
        ok(iRet == 2 && !GetLastError(),
           "AddMRUStringA(2) expected 2,0 got %d,%d\n",
           iRet, GetLastError());
        check_reg_entries("cba", checks);

        /* Add a duplicate of the 2nd string - it should move to the front,
         * but keep the same index in the registry.
         */
        SetLastError(0);
        iRet = pAddMRUStringA(hMRU, checks[1]);
        ok(iRet == 1 && !GetLastError(),
           "AddMRUStringA(re-add 1) expected 1,0 got %d,%d\n",
           iRet, GetLastError());
        check_reg_entries("bca", checks);

        /* Add a new string - replaces the oldest string + moves to the front */
        SetLastError(0);
        iRet = pAddMRUStringA(hMRU, checks[3]);
        ok(iRet == 0 && !GetLastError(),
           "AddMRUStringA(add new) expected 0,0 got %d,%d\n",
           iRet, GetLastError());
        checks[0] = checks[3];
        check_reg_entries("abc", checks);

        /* NULL buffer = get list size */
        iRet = pEnumMRUList(hMRU, 0, NULL, 0);
        ok(iRet == 3 || iRet == -1 /* Vista */, "EnumMRUList expected %d or -1, got %d\n", LIST_SIZE, iRet);

        /* negative item pos = get list size */
        iRet = pEnumMRUList(hMRU, -1, NULL, 0);
        ok(iRet == 3 || iRet == -1 /* Vista */, "EnumMRUList expected %d or -1, got %d\n", LIST_SIZE, iRet);

        /* negative item pos = get list size */
        iRet = pEnumMRUList(hMRU, -5, NULL, 0);
        ok(iRet == 3 || iRet == -1 /* Vista */, "EnumMRUList expected %d or -1, got %d\n", LIST_SIZE, iRet);

        /* negative item pos = get list size */
        iRet = pEnumMRUList(hMRU, -1, buffer, 255);
        ok(iRet == 3, "EnumMRUList expected %d, got %d\n", LIST_SIZE, iRet);

        /* negative item pos = get list size */
        iRet = pEnumMRUList(hMRU, -5, buffer, 255);
        ok(iRet == 3, "EnumMRUList expected %d, got %d\n", LIST_SIZE, iRet);

        /* check entry 0 */
        buffer[0] = 0;
        iRet = pEnumMRUList(hMRU, 0, buffer, 255);
        ok(iRet == lstrlen(checks[3]), "EnumMRUList expected %d, got %d\n", lstrlen(checks[3]), iRet);
        ok(strcmp(buffer, checks[3]) == 0, "EnumMRUList expected %s, got %s\n", checks[3], buffer);

        /* check entry 0 with a too small buffer */
        buffer[0] = 0;   /* overwritten with 'T' */
        buffer[1] = 'A'; /* overwritten with 0   */
        buffer[2] = 'A'; /* unchanged */
        buffer[3] = 0;   /* unchanged */
        iRet = pEnumMRUList(hMRU, 0, buffer, 2);
        ok(iRet == lstrlen(checks[3]), "EnumMRUList expected %d, got %d\n", lstrlen(checks[3]), iRet);
        ok(strcmp(buffer, "T") == 0, "EnumMRUList expected %s, got %s\n", "T", buffer);
        /* make sure space after buffer has old values */
        ok(buffer[2] == 'A', "EnumMRUList expected %02x, got %02x\n", 'A', buffer[2]);

        /* check entry 1 */
        buffer[0] = 0;
        iRet = pEnumMRUList(hMRU, 1, buffer, 255);
        ok(iRet == lstrlen(checks[1]), "EnumMRUList expected %d, got %d\n", lstrlen(checks[1]), iRet);
        ok(strcmp(buffer, checks[1]) == 0, "EnumMRUList expected %s, got %s\n", checks[1], buffer);

        /* check entry 2 */
        buffer[0] = 0;
        iRet = pEnumMRUList(hMRU, 2, buffer, 255);
        ok(iRet == lstrlen(checks[2]), "EnumMRUList expected %d, got %d\n", lstrlen(checks[2]), iRet);
        ok(strcmp(buffer, checks[2]) == 0, "EnumMRUList expected %s, got %s\n", checks[2], buffer);

        /* check out of bounds entry 3 */
        strcpy(buffer, "dummy");
        iRet = pEnumMRUList(hMRU, 3, buffer, 255);
        ok(iRet == -1, "EnumMRUList expected %d, got %d\n", -1, iRet);
        ok(strcmp(buffer, "dummy") == 0, "EnumMRUList expected unchanged buffer %s, got %s\n", "dummy", buffer);

        /* Finished with this MRU */
        pFreeMRUList(hMRU);
    }

    /* FreeMRUList(NULL) crashes on Win98 OSR0 */
}

static void test_CreateMRUListLazyA(void)
{
    HANDLE hMRU;
    HKEY hKey;
    CREATEMRULISTA listA = { 0 };

    if (!pCreateMRUListLazyA || !pFreeMRUList)
    {
        win_skip("CreateMRUListLazyA or FreeMRUList entry points not found\n");
        return;
    }

    /* wrong size */
    listA.cbSize = sizeof(listA) + 1;
    hMRU = pCreateMRUListLazyA(&listA, 0, 0, 0);
    ok(hMRU == NULL, "Expected NULL handle, got %p\n", hMRU);
    listA.cbSize = 4;
    hMRU = pCreateMRUListLazyA(&listA, 0, 0, 0);
    ok(hMRU == NULL, "Expected NULL handle, got %p\n", hMRU);
    /* NULL hKey */
    listA.cbSize = sizeof(listA);
    listA.hKey = NULL;
    hMRU = pCreateMRUListLazyA(&listA, 0, 0, 0);
    ok(hMRU == NULL, "Expected NULL handle, got %p\n", hMRU);
    /* NULL subkey */
    ok(!RegCreateKeyA(HKEY_CURRENT_USER, REG_TEST_KEYA, &hKey),
       "Couldn't create test key \"%s\"\n", REG_TEST_KEYA);
    listA.cbSize = sizeof(listA);
    listA.hKey = hKey;
    listA.lpszSubKey = NULL;
    hMRU = pCreateMRUListLazyA(&listA, 0, 0, 0);
    ok(hMRU == NULL || broken(hMRU != NULL), /* Win9x */
       "Expected NULL handle, got %p\n", hMRU);
    if (hMRU) pFreeMRUList(hMRU);
}

static void test_EnumMRUList(void)
{
    if (!pEnumMRUList || !pEnumMRUListW)
    {
        win_skip("EnumMRUListA/EnumMRUListW entry point not found\n");
        return;
    }

    /* NULL handle */
    if (0)
    {
        /* crashes on NT4, passed on Win2k, XP, 2k3, Vista, 2k8 */
        pEnumMRUList(NULL, 0, NULL, 0);
        pEnumMRUListW(NULL, 0, NULL, 0);
    }
}

static void test_FindMRUData(void)
{
    INT iRet;

    if (!pFindMRUData)
    {
        win_skip("FindMRUData entry point not found\n");
        return;
    }

    /* NULL handle */
    iRet = pFindMRUData(NULL, NULL, 0, NULL);
    ok(iRet == -1, "FindMRUData expected -1, got %d\n", iRet);
}

static void test_AddMRUData(void)
{
    INT iRet;

    if (!pAddMRUData)
    {
        win_skip("AddMRUData entry point not found\n");
        return;
    }

    /* NULL handle */
    iRet = pFindMRUData(NULL, NULL, 0, NULL);
    ok(iRet == -1, "AddMRUData expected -1, got %d\n", iRet);
}

START_TEST(mru)
{
    hComctl32 = GetModuleHandleA("comctl32.dll");

    delete_reg_entries();
    if (!create_reg_entries())
        return;

    InitPointers();

    test_MRUListA();
    test_CreateMRUListLazyA();
    test_EnumMRUList();
    test_FindMRUData();
    test_AddMRUData();

    delete_reg_entries();
}

/*
 * tests for Microsoft Installer functionality
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
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

#define _WIN32_MSI 300

#include <stdio.h>
#include <windows.h>
#include <msi.h>
#include <msiquery.h>
#include <sddl.h>

#include "wine/test.h"

static BOOL (WINAPI *pConvertSidToStringSidA)(PSID, LPSTR*);

static INSTALLSTATE (WINAPI *pMsiGetComponentPathA)
    (LPCSTR, LPCSTR, LPSTR, DWORD*);
static UINT (WINAPI *pMsiGetFileHashA)
    (LPCSTR, DWORD, PMSIFILEHASHINFO);
static UINT (WINAPI *pMsiGetProductInfoExA)
    (LPCSTR, LPCSTR, MSIINSTALLCONTEXT, LPCSTR, LPSTR, LPDWORD);
static UINT (WINAPI *pMsiOpenPackageExA)
    (LPCSTR, DWORD, MSIHANDLE*);
static UINT (WINAPI *pMsiOpenPackageExW)
    (LPCWSTR, DWORD, MSIHANDLE*);
static UINT (WINAPI *pMsiQueryComponentStateA)
    (LPCSTR, LPCSTR, MSIINSTALLCONTEXT, LPCSTR, INSTALLSTATE*);
static INSTALLSTATE (WINAPI *pMsiUseFeatureExA)
    (LPCSTR, LPCSTR ,DWORD, DWORD );

static void init_functionpointers(void)
{
    HMODULE hmsi = GetModuleHandleA("msi.dll");
    HMODULE hadvapi32 = GetModuleHandleA("advapi32.dll");

#define GET_PROC(dll, func) \
    p ## func = (void *)GetProcAddress(dll, #func); \
    if(!p ## func) \
      trace("GetProcAddress(%s) failed\n", #func);

    GET_PROC(hmsi, MsiGetComponentPathA)
    GET_PROC(hmsi, MsiGetFileHashA)
    GET_PROC(hmsi, MsiGetProductInfoExA)
    GET_PROC(hmsi, MsiOpenPackageExA)
    GET_PROC(hmsi, MsiOpenPackageExW)
    GET_PROC(hmsi, MsiQueryComponentStateA)
    GET_PROC(hmsi, MsiUseFeatureExA)

    GET_PROC(hadvapi32, ConvertSidToStringSidA)

#undef GET_PROC
}

static void test_usefeature(void)
{
    INSTALLSTATE r;

    if (!pMsiUseFeatureExA)
    {
        skip("MsiUseFeatureExA not implemented\n");
        return;
    }

    r = MsiQueryFeatureState(NULL,NULL);
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = MsiQueryFeatureState("{9085040-6000-11d3-8cfe-0150048383c9}" ,NULL);
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = pMsiUseFeatureExA(NULL,NULL,0,0);
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = pMsiUseFeatureExA(NULL, "WORDVIEWFiles", -2, 1 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = pMsiUseFeatureExA("{90850409-6000-11d3-8cfe-0150048383c9}", 
                         NULL, -2, 0 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = pMsiUseFeatureExA("{9085040-6000-11d3-8cfe-0150048383c9}", 
                         "WORDVIEWFiles", -2, 0 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = pMsiUseFeatureExA("{0085040-6000-11d3-8cfe-0150048383c9}", 
                         "WORDVIEWFiles", -2, 0 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = pMsiUseFeatureExA("{90850409-6000-11d3-8cfe-0150048383c9}", 
                         "WORDVIEWFiles", -2, 1 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");
}

static void test_null(void)
{
    MSIHANDLE hpkg;
    UINT r;
    HKEY hkey;
    DWORD dwType, cbData;
    LPBYTE lpData = NULL;
    INSTALLSTATE state;

    r = pMsiOpenPackageExW(NULL, 0, &hpkg);
    ok( r == ERROR_INVALID_PARAMETER,"wrong error\n");

    state = MsiQueryProductStateW(NULL);
    ok( state == INSTALLSTATE_INVALIDARG, "wrong return\n");

    r = MsiEnumFeaturesW(NULL,0,NULL,NULL);
    ok( r == ERROR_INVALID_PARAMETER,"wrong error\n");

    r = MsiConfigureFeatureW(NULL, NULL, 0);
    ok( r == ERROR_INVALID_PARAMETER, "wrong error\n");

    r = MsiConfigureFeatureA("{00000000-0000-0000-0000-000000000000}", NULL, 0);
    ok( r == ERROR_INVALID_PARAMETER, "wrong error\n");

    r = MsiConfigureFeatureA("{00000000-0000-0000-0000-000000000000}", "foo", 0);
    ok( r == ERROR_INVALID_PARAMETER, "wrong error %d\n", r);

    r = MsiConfigureFeatureA("{00000000-0000-0000-0000-000000000000}", "foo", INSTALLSTATE_DEFAULT);
    ok( r == ERROR_UNKNOWN_PRODUCT, "wrong error %d\n", r);

    /* make sure empty string to MsiGetProductInfo is not a handle to default registry value, saving and restoring the
     * necessary registry values */

    /* empty product string */
    r = RegOpenKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", &hkey);
    ok( r == ERROR_SUCCESS, "wrong error %d\n", r);

    r = RegQueryValueExA(hkey, NULL, 0, &dwType, lpData, &cbData);
    ok ( r == ERROR_SUCCESS || r == ERROR_FILE_NOT_FOUND, "wrong error %d\n", r);
    if ( r == ERROR_SUCCESS )
    {
        lpData = HeapAlloc(GetProcessHeap(), 0, cbData);
        if (!lpData)
            skip("Out of memory\n");
        else
        {
            r = RegQueryValueExA(hkey, NULL, 0, &dwType, lpData, &cbData);
            ok ( r == ERROR_SUCCESS, "wrong error %d\n", r);
        }
    }

    r = RegSetValueA(hkey, NULL, REG_SZ, "test", strlen("test"));
    ok( r == ERROR_SUCCESS, "wrong error %d\n", r);

    r = MsiGetProductInfoA("", "", NULL, NULL);
    ok ( r == ERROR_INVALID_PARAMETER, "wrong error %d\n", r);

    if (lpData)
    {
        r = RegSetValueExA(hkey, NULL, 0, dwType, lpData, cbData);
        ok ( r == ERROR_SUCCESS, "wrong error %d\n", r);

        HeapFree(GetProcessHeap(), 0, lpData);
    }
    else
    {
        r = RegDeleteValueA(hkey, NULL);
        ok ( r == ERROR_SUCCESS, "wrong error %d\n", r);
    }

    r = RegCloseKey(hkey);
    ok( r == ERROR_SUCCESS, "wrong error %d\n", r);

    /* empty attribute */
    r = RegCreateKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{F1C3AF50-8B56-4A69-A00C-00773FE42F30}", &hkey);
    ok( r == ERROR_SUCCESS, "wrong error %d\n", r);

    r = RegSetValueA(hkey, NULL, REG_SZ, "test", strlen("test"));
    ok( r == ERROR_SUCCESS, "wrong error %d\n", r);

    r = MsiGetProductInfoA("{F1C3AF50-8B56-4A69-A00C-00773FE42F30}", "", NULL, NULL);
    ok ( r == ERROR_UNKNOWN_PROPERTY, "wrong error %d\n", r);

    r = RegCloseKey(hkey);
    ok( r == ERROR_SUCCESS, "wrong error %d\n", r);

    r = RegDeleteKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{F1C3AF50-8B56-4A69-A00C-00773FE42F30}");
    ok( r == ERROR_SUCCESS, "wrong error %d\n", r);
}

static void test_getcomponentpath(void)
{
    INSTALLSTATE r;
    char buffer[0x100];
    DWORD sz;

    if(!pMsiGetComponentPathA)
        return;

    r = pMsiGetComponentPathA( NULL, NULL, NULL, NULL );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return value\n");

    r = pMsiGetComponentPathA( "bogus", "bogus", NULL, NULL );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return value\n");

    r = pMsiGetComponentPathA( "bogus", "{00000000-0000-0000-000000000000}", NULL, NULL );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return value\n");

    sz = sizeof buffer;
    buffer[0]=0;
    r = pMsiGetComponentPathA( "bogus", "{00000000-0000-0000-000000000000}", buffer, &sz );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return value\n");

    r = pMsiGetComponentPathA( "{00000000-78E1-11D2-B60F-006097C998E7}",
        "{00000000-0000-0000-0000-000000000000}", buffer, &sz );
    ok( r == INSTALLSTATE_UNKNOWN, "wrong return value\n");

    r = pMsiGetComponentPathA( "{00000409-78E1-11D2-B60F-006097C998E7}",
        "{00000000-0000-0000-0000-00000000}", buffer, &sz );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return value\n");

    r = pMsiGetComponentPathA( "{00000409-78E1-11D2-B60F-006097C998E7}",
        "{029E403D-A86A-1D11-5B5B0006799C897E}", buffer, &sz );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return value\n");

    r = pMsiGetComponentPathA( "{00000000-78E1-11D2-B60F-006097C9987e}",
                            "{00000000-A68A-11d1-5B5B-0006799C897E}", buffer, &sz );
    ok( r == INSTALLSTATE_UNKNOWN, "wrong return value\n");
}

static void create_file(LPCSTR name, LPCSTR data, DWORD size)
{
    HANDLE file;
    DWORD written;

    file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Failure to open file %s\n", name);
    WriteFile(file, data, strlen(data), &written, NULL);

    if (size)
    {
        SetFilePointer(file, size, NULL, FILE_BEGIN);
        SetEndOfFile(file);
    }

    CloseHandle(file);
}

#define HASHSIZE sizeof(MSIFILEHASHINFO)

static const struct
{
    LPCSTR data;
    DWORD size;
    MSIFILEHASHINFO hash;
} hash_data[] =
{
    { "abc", 0,
      { HASHSIZE,
        { 0x98500190, 0xb04fd23c, 0x7d3f96d6, 0x727fe128 },
      },
    },

    { "C:\\Program Files\\msitest\\caesar\n", 0,
      { HASHSIZE,
        { 0x2b566794, 0xfd42181b, 0x2514d6e4, 0x5768b4e2 },
      },
    },

    { "C:\\Program Files\\msitest\\caesar\n", 500,
      { HASHSIZE,
        { 0x58095058, 0x805efeff, 0x10f3483e, 0x0147d653 },
      },
    },
};

static void test_MsiGetFileHash(void)
{
    const char name[] = "msitest.bin";
    UINT r;
    MSIFILEHASHINFO hash;
    DWORD i;

    if (!pMsiGetFileHashA)
    {
        skip("MsiGetFileHash not implemented\n");
        return;
    }

    hash.dwFileHashInfoSize = sizeof(MSIFILEHASHINFO);

    /* szFilePath is NULL */
    r = pMsiGetFileHashA(NULL, 0, &hash);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* szFilePath is empty */
    r = pMsiGetFileHashA("", 0, &hash);
    ok(r == ERROR_PATH_NOT_FOUND || r == ERROR_BAD_PATHNAME,
       "Expected ERROR_PATH_NOT_FOUND or ERROR_BAD_PATHNAME, got %d\n", r);

    /* szFilePath is nonexistent */
    r = pMsiGetFileHashA(name, 0, &hash);
    ok(r == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", r);

    /* dwOptions is non-zero */
    r = pMsiGetFileHashA(name, 1, &hash);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* pHash.dwFileHashInfoSize is not correct */
    hash.dwFileHashInfoSize = 0;
    r = pMsiGetFileHashA(name, 0, &hash);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* pHash is NULL */
    r = pMsiGetFileHashA(name, 0, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    for (i = 0; i < sizeof(hash_data) / sizeof(hash_data[0]); i++)
    {
        create_file(name, hash_data[i].data, hash_data[i].size);

        memset(&hash, 0, sizeof(MSIFILEHASHINFO));
        hash.dwFileHashInfoSize = sizeof(MSIFILEHASHINFO);

        r = pMsiGetFileHashA(name, 0, &hash);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!memcmp(&hash, &hash_data[i].hash, HASHSIZE), "Hash incorrect\n");

        DeleteFile(name);
    }
}

/* copied from dlls/msi/registry.c */
static BOOL squash_guid(LPCWSTR in, LPWSTR out)
{
    DWORD i,n=1;
    GUID guid;

    if (FAILED(CLSIDFromString((LPOLESTR)in, &guid)))
        return FALSE;

    for(i=0; i<8; i++)
        out[7-i] = in[n++];
    n++;
    for(i=0; i<4; i++)
        out[11-i] = in[n++];
    n++;
    for(i=0; i<4; i++)
        out[15-i] = in[n++];
    n++;
    for(i=0; i<2; i++)
    {
        out[17+i*2] = in[n++];
        out[16+i*2] = in[n++];
    }
    n++;
    for( ; i<8; i++)
    {
        out[17+i*2] = in[n++];
        out[16+i*2] = in[n++];
    }
    out[32]=0;
    return TRUE;
}

static void create_test_guid(LPSTR prodcode, LPSTR squashed)
{
    WCHAR guidW[MAX_PATH];
    WCHAR squashedW[MAX_PATH];
    GUID guid;
    HRESULT hr;
    int size;

    hr = CoCreateGuid(&guid);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);

    size = StringFromGUID2(&guid, (LPOLESTR)guidW, MAX_PATH);
    ok(size == 39, "Expected 39, got %d\n", hr);

    WideCharToMultiByte(CP_ACP, 0, guidW, size, prodcode, MAX_PATH, NULL, NULL);
    squash_guid(guidW, squashedW);
    WideCharToMultiByte(CP_ACP, 0, squashedW, -1, squashed, MAX_PATH, NULL, NULL);
}

static void get_user_sid(LPSTR *usersid)
{
    HANDLE token;
    BYTE buf[1024];
    DWORD size;
    PTOKEN_USER user;

    OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token);
    size = sizeof(buf);
    GetTokenInformation(token, TokenUser, (void *)buf, size, &size);
    user = (PTOKEN_USER)buf;
    pConvertSidToStringSidA(user->User.Sid, usersid);
}

static void test_MsiQueryProductState(void)
{
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR keypath[MAX_PATH*2];
    LPSTR usersid;
    INSTALLSTATE state;
    LONG res;
    HKEY userkey, localkey, props;
    HKEY prodkey;
    DWORD data;

    create_test_guid(prodcode, prod_squashed);
    get_user_sid(&usersid);

    /* NULL prodcode */
    state = MsiQueryProductStateA(NULL);
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);

    /* empty prodcode */
    state = MsiQueryProductStateA("");
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);

    /* garbage prodcode */
    state = MsiQueryProductStateA("garbage");
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);

    /* guid without brackets */
    state = MsiQueryProductStateA("6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D");
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);

    /* guid with brackets */
    state = MsiQueryProductStateA("{6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D}");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    /* same length as guid, but random */
    state = MsiQueryProductStateA("A938G02JF-2NF3N93-VN3-2NNF-3KGKALDNF93");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    /* MSIINSTALLCONTEXT_USERUNMANAGED */

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user product key exists */
    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\");
    lstrcatA(keypath, prodcode);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &localkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* local uninstall key exists */
    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    data = 1;
    res = RegSetValueExA(localkey, "WindowsInstaller", 0, REG_DWORD, (const BYTE *)&data, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* WindowsInstaller value exists */
    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    RegDeleteValueA(localkey, "WindowsInstaller");
    RegDeleteKeyA(localkey, "");

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &localkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* local product key exists */
    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegCreateKeyA(localkey, "InstallProperties", &props);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* install properties key exists */
    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    data = 1;
    res = RegSetValueExA(props, "WindowsInstaller", 0, REG_DWORD, (const BYTE *)&data, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* WindowsInstaller value exists */
    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_DEFAULT, "Expected INSTALLSTATE_DEFAULT, got %d\n", state);

    data = 2;
    res = RegSetValueExA(props, "WindowsInstaller", 0, REG_DWORD, (const BYTE *)&data, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* WindowsInstaller value is not 1 */
    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_DEFAULT, "Expected INSTALLSTATE_DEFAULT, got %d\n", state);

    RegDeleteKeyA(userkey, "");

    /* user product key does not exist */
    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);

    RegDeleteValueA(props, "WindowsInstaller");
    RegDeleteKeyA(props, "");
    RegCloseKey(props);
    RegDeleteKeyA(localkey, "");
    RegCloseKey(localkey);
    RegDeleteKeyA(userkey, "");
    RegCloseKey(userkey);

    /* MSIINSTALLCONTEXT_USERMANAGED */

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_ADVERTISED,
       "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &localkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_ADVERTISED,
       "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegCreateKeyA(localkey, "InstallProperties", &props);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_ADVERTISED,
       "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    data = 1;
    res = RegSetValueExA(props, "WindowsInstaller", 0, REG_DWORD, (const BYTE *)&data, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* WindowsInstaller value exists */
    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_DEFAULT, "Expected INSTALLSTATE_DEFAULT, got %d\n", state);

    RegDeleteValueA(props, "WindowsInstaller");
    RegDeleteKeyA(props, "");
    RegCloseKey(props);
    RegDeleteKeyA(localkey, "");
    RegCloseKey(localkey);
    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);

    /* MSIINSTALLCONTEXT_MACHINE */

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, "S-1-5-18\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &localkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_ADVERTISED,
       "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegCreateKeyA(localkey, "InstallProperties", &props);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_ADVERTISED,
       "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    data = 1;
    res = RegSetValueExA(props, "WindowsInstaller", 0, REG_DWORD, (const BYTE *)&data, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* WindowsInstaller value exists */
    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_DEFAULT, "Expected INSTALLSTATE_DEFAULT, got %d\n", state);

    RegDeleteValueA(props, "WindowsInstaller");
    RegDeleteKeyA(props, "");
    RegCloseKey(props);
    RegDeleteKeyA(localkey, "");
    RegCloseKey(localkey);
    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);

    LocalFree(usersid);
}

static const char table_enc85[] =
"!$%&'()*+,-.0123456789=?@ABCDEFGHIJKLMNO"
"PQRSTUVWXYZ[]^_`abcdefghijklmnopqrstuvwx"
"yz{}~";

/*
 *  Encodes a base85 guid given a GUID pointer
 *  Caller should provide a 21 character buffer for the encoded string.
 *
 *  returns TRUE if successful, FALSE if not
 */
static BOOL encode_base85_guid( GUID *guid, LPWSTR str )
{
    unsigned int x, *p, i;

    p = (unsigned int*) guid;
    for( i=0; i<4; i++ )
    {
        x = p[i];
        *str++ = table_enc85[x%85];
        x = x/85;
        *str++ = table_enc85[x%85];
        x = x/85;
        *str++ = table_enc85[x%85];
        x = x/85;
        *str++ = table_enc85[x%85];
        x = x/85;
        *str++ = table_enc85[x%85];
    }
    *str = 0;

    return TRUE;
}

static void compose_base85_guid(LPSTR component, LPSTR comp_base85, LPSTR squashed)
{
    WCHAR guidW[MAX_PATH];
    WCHAR base85W[MAX_PATH];
    WCHAR squashedW[MAX_PATH];
    GUID guid;
    HRESULT hr;
    int size;

    hr = CoCreateGuid(&guid);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);

    size = StringFromGUID2(&guid, (LPOLESTR)guidW, MAX_PATH);
    ok(size == 39, "Expected 39, got %d\n", hr);

    WideCharToMultiByte(CP_ACP, 0, guidW, size, component, MAX_PATH, NULL, NULL);
    encode_base85_guid(&guid, base85W);
    WideCharToMultiByte(CP_ACP, 0, base85W, -1, comp_base85, MAX_PATH, NULL, NULL);
    squash_guid(guidW, squashedW);
    WideCharToMultiByte(CP_ACP, 0, squashedW, -1, squashed, MAX_PATH, NULL, NULL);
}

static void test_MsiQueryFeatureState(void)
{
    HKEY userkey, localkey, compkey;
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR component[MAX_PATH];
    CHAR comp_base85[MAX_PATH];
    CHAR comp_squashed[MAX_PATH];
    CHAR keypath[MAX_PATH*2];
    INSTALLSTATE state;
    LPSTR usersid;
    LONG res;

    create_test_guid(prodcode, prod_squashed);
    compose_base85_guid(component, comp_base85, comp_squashed);
    get_user_sid(&usersid);

    /* NULL prodcode */
    state = MsiQueryFeatureStateA(NULL, "feature");
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);

    /* empty prodcode */
    state = MsiQueryFeatureStateA("", "feature");
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);

    /* garbage prodcode */
    state = MsiQueryFeatureStateA("garbage", "feature");
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);

    /* guid without brackets */
    state = MsiQueryFeatureStateA("6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D", "feature");
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);

    /* guid with brackets */
    state = MsiQueryFeatureStateA("{6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D}", "feature");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    /* same length as guid, but random */
    state = MsiQueryFeatureStateA("A938G02JF-2NF3N93-VN3-2NNF-3KGKALDNF93", "feature");
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);

    /* NULL szFeature */
    state = MsiQueryFeatureStateA(prodcode, NULL);
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);

    /* empty szFeature */
    state = MsiQueryFeatureStateA(prodcode, "");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    /* feature key does not exist yet */
    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Features\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* feature key exists */
    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    res = RegSetValueExA(userkey, "feature", 0, REG_SZ, (const BYTE *)"", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);
    lstrcatA(keypath, "\\Features");

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &localkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegSetValueExA(localkey, "feature", 0, REG_SZ, (const BYTE *)"aaaaaaaaaaaaaaaaaaa", 20);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_BADCONFIG, "Expected INSTALLSTATE_BADCONFIG, got %d\n", state);

    res = RegSetValueExA(localkey, "feature", 0, REG_SZ, (const BYTE *)"aaaaaaaaaaaaaaaaaaaa", 21);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegSetValueExA(localkey, "feature", 0, REG_SZ, (const BYTE *)"aaaaaaaaaaaaaaaaaaaaa", 22);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegSetValueExA(localkey, "feature", 0, REG_SZ, (const BYTE *)comp_base85, 21);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &compkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"", 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"apple", 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteValueA(compkey, "");
    RegDeleteValueA(localkey, "feature");
    RegDeleteValueA(userkey, "feature");
    RegDeleteKeyA(userkey, "");
    RegCloseKey(compkey);
    RegCloseKey(localkey);
    RegCloseKey(userkey);
}

static void test_MsiQueryComponentState(void)
{
    HKEY compkey, prodkey;
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR component[MAX_PATH];
    CHAR comp_base85[MAX_PATH];
    CHAR comp_squashed[MAX_PATH];
    CHAR keypath[MAX_PATH];
    INSTALLSTATE state;
    LPSTR usersid;
    LONG res;
    UINT r;

    static const INSTALLSTATE MAGIC_ERROR = 0xdeadbeef;

    if (!pMsiQueryComponentStateA)
    {
        skip("MsiQueryComponentStateA not implemented\n");
        return;
    }

    create_test_guid(prodcode, prod_squashed);
    compose_base85_guid(component, comp_base85, comp_squashed);
    get_user_sid(&usersid);

    /* NULL szProductCode */
    state = MAGIC_ERROR;
    r = pMsiQueryComponentStateA(NULL, NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(state == MAGIC_ERROR, "Expected 0xdeadbeef, got %d\n", state);

    /* empty szProductCode */
    state = MAGIC_ERROR;
    r = pMsiQueryComponentStateA("", NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(state == MAGIC_ERROR, "Expected 0xdeadbeef, got %d\n", state);

    /* random szProductCode */
    state = MAGIC_ERROR;
    r = pMsiQueryComponentStateA("random", NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(state == MAGIC_ERROR, "Expected 0xdeadbeef, got %d\n", state);

    /* GUID-length szProductCode */
    state = MAGIC_ERROR;
    r = pMsiQueryComponentStateA("DJANE93KNDNAS-2KN2NR93KMN3LN13=L1N3KDE", NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(state == MAGIC_ERROR, "Expected 0xdeadbeef, got %d\n", state);

    /* GUID-length with brackets */
    state = MAGIC_ERROR;
    r = pMsiQueryComponentStateA("{JANE93KNDNAS-2KN2NR93KMN3LN13=L1N3KD}", NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(state == MAGIC_ERROR, "Expected 0xdeadbeef, got %d\n", state);

    /* actual GUID */
    state = MAGIC_ERROR;
    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(state == MAGIC_ERROR, "Expected 0xdeadbeef, got %d\n", state);

    state = MAGIC_ERROR;
    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(state == MAGIC_ERROR, "Expected 0xdeadbeef, got %d\n", state);

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MAGIC_ERROR;
    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);

    /* create local system product key */
    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\S-1-5-18\\Products\\");
    lstrcatA(keypath, prod_squashed);
    lstrcatA(keypath, "\\InstallProperties");

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* local system product key exists */
    state = MAGIC_ERROR;
    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(state == MAGIC_ERROR, "Expected 0xdeadbeef, got %d\n", state);

    res = RegSetValueExA(prodkey, "LocalPackage", 0, REG_SZ, (const BYTE *)"msitest.msi", 11);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* LocalPackage value exists */
    state = MAGIC_ERROR;
    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\S-1-5-18\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &compkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* component key exists */
    state = MAGIC_ERROR;
    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"", 0);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* component\product exists */
    state = MAGIC_ERROR;
    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(state == INSTALLSTATE_NOTUSED, "Expected INSTALLSTATE_NOTUSED, got %d\n", state);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"hi", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MAGIC_ERROR;
    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_MACHINE, component, &state);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    RegDeleteValueA(prodkey, "LocalPackage");
    RegDeleteKeyA(prodkey, "");
    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);
    RegCloseKey(compkey);

    /* MSIINSTALLCONTEXT_USERUNMANAGED */

    state = MAGIC_ERROR;
    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, component, &state);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(state == MAGIC_ERROR, "Expected 0xdeadbeef, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MAGIC_ERROR;
    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, component, &state);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);
    lstrcatA(keypath, "\\InstallProperties");

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegSetValueExA(prodkey, "LocalPackage", 0, REG_SZ, (const BYTE *)"msitest.msi", 11);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    RegCloseKey(prodkey);

    state = MAGIC_ERROR;
    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, component, &state);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &compkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* component key exists */
    state = MAGIC_ERROR;
    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, component, &state);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"", 0);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* component\product exists */
    state = MAGIC_ERROR;
    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, component, &state);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(state == INSTALLSTATE_NOTUSED, "Expected INSTALLSTATE_NOTUSED, got %d\n", state);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"hi", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MAGIC_ERROR;
    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERUNMANAGED, component, &state);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    /* MSIINSTALLCONTEXT_USERMANAGED */

    state = MAGIC_ERROR;
    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERMANAGED, component, &state);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(state == MAGIC_ERROR, "Expected 0xdeadbeef, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MAGIC_ERROR;
    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERMANAGED, component, &state);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(state == MAGIC_ERROR, "Expected 0xdeadbeef, got %d\n", state);

    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MAGIC_ERROR;
    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERMANAGED, component, &state);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);
    lstrcatA(keypath, "\\InstallProperties");

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegSetValueExA(prodkey, "ManagedLocalPackage", 0, REG_SZ, (const BYTE *)"msitest.msi", 11);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MAGIC_ERROR;
    r = pMsiQueryComponentStateA(prodcode, NULL, MSIINSTALLCONTEXT_USERMANAGED, component, &state);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);

    RegDeleteValueA(prodkey, "LocalPackage");
    RegDeleteValueA(prodkey, "ManagedLocalPackage");
    RegDeleteKeyA(prodkey, "");
    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteKeyA(compkey, "");
    RegCloseKey(prodkey);
    RegCloseKey(compkey);
}

static void test_MsiGetComponentPath(void)
{
    HKEY compkey, prodkey, installprop;
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR component[MAX_PATH];
    CHAR comp_base85[MAX_PATH];
    CHAR comp_squashed[MAX_PATH];
    CHAR keypath[MAX_PATH];
    CHAR path[MAX_PATH];
    INSTALLSTATE state;
    LPSTR usersid;
    DWORD size, val;
    LONG res;

    create_test_guid(prodcode, prod_squashed);
    compose_base85_guid(component, comp_base85, comp_squashed);
    get_user_sid(&usersid);

    /* NULL szProduct */
    size = MAX_PATH;
    state = MsiGetComponentPathA(NULL, component, path, &size);
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* NULL szComponent */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, NULL, path, &size);
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* NULL lpPathBuf */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, NULL, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* NULL pcchBuf */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, NULL);
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    /* all params valid */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\S-1-5-18\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &compkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* local system component key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"C:\\imapath", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* product value exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %d\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\S-1-5-18\\Products\\");
    lstrcatA(keypath, prod_squashed);
    lstrcatA(keypath, "\\InstallProperties");

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &installprop);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    val = 1;
    res = RegSetValueExA(installprop, "WindowsInstaller", 0, REG_DWORD, (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* install properties key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %d\n", size);

    create_file("C:\\imapath", "C:\\imapath", 11);

    /* file exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %d\n", size);

    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteKeyA(compkey, "");
    RegDeleteValueA(installprop, "WindowsInstaller");
    RegDeleteKeyA(installprop, "");
    RegCloseKey(compkey);
    RegCloseKey(installprop);
    DeleteFileA("C:\\imapath");

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &compkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user managed component key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"C:\\imapath", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* product value exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %d\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\S-1-5-18\\Products\\");
    lstrcatA(keypath, prod_squashed);
    lstrcatA(keypath, "\\InstallProperties");

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &installprop);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    val = 1;
    res = RegSetValueExA(installprop, "WindowsInstaller", 0, REG_DWORD, (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* install properties key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %d\n", size);

    create_file("C:\\imapath", "C:\\imapath", 11);

    /* file exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %d\n", size);

    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteKeyA(compkey, "");
    RegDeleteValueA(installprop, "WindowsInstaller");
    RegDeleteKeyA(installprop, "");
    RegCloseKey(compkey);
    RegCloseKey(installprop);
    DeleteFileA("C:\\imapath");

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user managed product key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &compkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user managed component key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"C:\\imapath", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* product value exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %d\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\S-1-5-18\\Products\\");
    lstrcatA(keypath, prod_squashed);
    lstrcatA(keypath, "\\InstallProperties");

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &installprop);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    val = 1;
    res = RegSetValueExA(installprop, "WindowsInstaller", 0, REG_DWORD, (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* install properties key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %d\n", size);

    create_file("C:\\imapath", "C:\\imapath", 11);

    /* file exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %d\n", size);

    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteKeyA(prodkey, "");
    RegDeleteKeyA(compkey, "");
    RegDeleteValueA(installprop, "WindowsInstaller");
    RegDeleteKeyA(installprop, "");
    RegCloseKey(prodkey);
    RegCloseKey(compkey);
    RegCloseKey(installprop);
    DeleteFileA("C:\\imapath");

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user unmanaged product key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &compkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user unmanaged component key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"C:\\imapath", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* product value exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %d\n", size);

    create_file("C:\\imapath", "C:\\imapath", 11);

    /* file exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %d\n", size);

    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteKeyA(prodkey, "");
    RegDeleteKeyA(compkey, "");
    RegCloseKey(prodkey);
    RegCloseKey(compkey);
    RegCloseKey(installprop);
    DeleteFileA("C:\\imapath");

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* local classes product key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\S-1-5-18\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &compkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* local user component key exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok(size == MAX_PATH, "Expected size to be unchanged, got %d\n", size);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"C:\\imapath", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* product value exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %d\n", size);

    create_file("C:\\imapath", "C:\\imapath", 11);

    /* file exists */
    size = MAX_PATH;
    state = MsiGetComponentPathA(prodcode, component, path, &size);
    ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    ok(!lstrcmpA(path, "C:\\imapath"), "Expected C:\\imapath, got %s\n", path);
    ok(size == 10, "Expected 10, got %d\n", size);

    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteKeyA(prodkey, "");
    RegDeleteKeyA(compkey, "");
    RegCloseKey(prodkey);
    RegCloseKey(compkey);
    DeleteFileA("C:\\imapath");
}

static void test_MsiGetProductCode(void)
{
    HKEY compkey, prodkey;
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR prodcode2[MAX_PATH];
    CHAR prod2_squashed[MAX_PATH];
    CHAR component[MAX_PATH];
    CHAR comp_base85[MAX_PATH];
    CHAR comp_squashed[MAX_PATH];
    CHAR keypath[MAX_PATH];
    CHAR product[MAX_PATH];
    LPSTR usersid;
    LONG res;
    UINT r;

    create_test_guid(prodcode, prod_squashed);
    create_test_guid(prodcode2, prod2_squashed);
    compose_base85_guid(component, comp_base85, comp_squashed);
    get_user_sid(&usersid);

    /* szComponent is NULL */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(NULL, product);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(product, "prod"), "Expected product to be unchanged, got %s\n", product);

    /* szComponent is empty */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA("", product);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(product, "prod"), "Expected product to be unchanged, got %s\n", product);

    /* garbage szComponent */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA("garbage", product);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(product, "prod"), "Expected product to be unchanged, got %s\n", product);

    /* guid without brackets */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA("6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D", product);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(product, "prod"), "Expected product to be unchanged, got %s\n", product);

    /* guid with brackets */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA("{6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D}", product);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(!lstrcmpA(product, "prod"), "Expected product to be unchanged, got %s\n", product);

    /* same length as guid, but random */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA("A938G02JF-2NF3N93-VN3-2NNF-3KGKALDNF93", product);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(product, "prod"), "Expected product to be unchanged, got %s\n", product);

    /* all params correct, szComponent not published */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(!lstrcmpA(product, "prod"), "Expected product to be unchanged, got %s\n", product);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &compkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user unmanaged component key exists */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(!lstrcmpA(product, "prod"), "Expected product to be unchanged, got %s\n", product);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"C:\\imapath", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* product value exists */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode), "Expected %s, got %s\n", prodcode, product);

    res = RegSetValueExA(compkey, prod2_squashed, 0, REG_SZ, (const BYTE *)"C:\\another", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user managed product key of first product exists */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode), "Expected %s, got %s\n", prodcode, product);

    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);

    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user unmanaged product key exists */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode), "Expected %s, got %s\n", prodcode, product);

    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* local classes product key exists */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode), "Expected %s, got %s\n", prodcode, product);

    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod2_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user managed product key of second product exists */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode2), "Expected %s, got %s\n", prodcode2, product);

    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);
    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteValueA(compkey, prod2_squashed);
    RegDeleteKeyA(compkey, "");
    RegCloseKey(compkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\S-1-5-18\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &compkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* local user component key exists */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(!lstrcmpA(product, "prod"), "Expected product to be unchanged, got %s\n", product);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"C:\\imapath", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* product value exists */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode), "Expected %s, got %s\n", prodcode, product);

    res = RegSetValueExA(compkey, prod2_squashed, 0, REG_SZ, (const BYTE *)"C:\\another", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user managed product key of first product exists */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode), "Expected %s, got %s\n", prodcode, product);

    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user unmanaged product key exists */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode), "Expected %s, got %s\n", prodcode, product);

    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* local classes product key exists */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode), "Expected %s, got %s\n", prodcode, product);

    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod2_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user managed product key of second product exists */
    lstrcpyA(product, "prod");
    r = MsiGetProductCodeA(component, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode2), "Expected %s, got %s\n", prodcode2, product);

    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);
    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteValueA(compkey, prod2_squashed);
    RegDeleteKeyA(compkey, "");
    RegCloseKey(compkey);
}

static void test_MsiEnumClients(void)
{
    HKEY compkey;
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR prodcode2[MAX_PATH];
    CHAR prod2_squashed[MAX_PATH];
    CHAR component[MAX_PATH];
    CHAR comp_base85[MAX_PATH];
    CHAR comp_squashed[MAX_PATH];
    CHAR product[MAX_PATH];
    CHAR keypath[MAX_PATH];
    LPSTR usersid;
    LONG res;
    UINT r;

    create_test_guid(prodcode, prod_squashed);
    create_test_guid(prodcode2, prod2_squashed);
    compose_base85_guid(component, comp_base85, comp_squashed);
    get_user_sid(&usersid);

    /* NULL szComponent */
    product[0] = '\0';
    r = MsiEnumClientsA(NULL, 0, product);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(product, ""), "Expected product to be unchanged, got %s\n", product);

    /* empty szComponent */
    product[0] = '\0';
    r = MsiEnumClientsA("", 0, product);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(product, ""), "Expected product to be unchanged, got %s\n", product);

    /* NULL lpProductBuf */
    r = MsiEnumClientsA(component, 0, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* all params correct, component missing */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 0, product);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(!lstrcmpA(product, ""), "Expected product to be unchanged, got %s\n", product);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &compkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user unmanaged component key exists */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 0, product);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(!lstrcmpA(product, ""), "Expected product to be unchanged, got %s\n", product);

    /* index > 0, no products exist */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 1, product);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(product, ""), "Expected product to be unchanged, got %s\n", product);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"C:\\imapath", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* product value exists */
    r = MsiEnumClientsA(component, 0, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode), "Expected %s, got %s\n", prodcode, product);

    /* try index 0 again */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 0, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode), "Expected %s, got %s\n", prodcode, product);

    /* try index 1, second product value does not exist */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 1, product);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(product, ""), "Expected product to be unchanged, got %s\n", product);

    res = RegSetValueExA(compkey, prod2_squashed, 0, REG_SZ, (const BYTE *)"C:\\another", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* try index 1, second product value does exist */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 1, product);
    todo_wine
    {
        ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
        ok(!lstrcmpA(product, ""), "Expected product to be unchanged, got %s\n", product);
    }

    /* start the enumeration over */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 0, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode) || !lstrcmpA(product, prodcode2),
       "Expected %s or %s, got %s\n", prodcode, prodcode2, product);

    /* correctly query second product */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 1, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode) || !lstrcmpA(product, prodcode2),
       "Expected %s or %s, got %s\n", prodcode, prodcode2, product);

    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteValueA(compkey, prod2_squashed);
    RegDeleteKeyA(compkey, "");
    RegCloseKey(compkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\");
    lstrcatA(keypath, "Installer\\UserData\\S-1-5-18\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &compkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user local component key exists */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 0, product);
    ok(r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r);
    ok(!lstrcmpA(product, ""), "Expected product to be unchanged, got %s\n", product);

    /* index > 0, no products exist */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 1, product);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(product, ""), "Expected product to be unchanged, got %s\n", product);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"C:\\imapath", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* product value exists */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 0, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode), "Expected %s, got %s\n", prodcode, product);

    /* try index 0 again */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 0, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* try index 1, second product value does not exist */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 1, product);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(product, ""), "Expected product to be unchanged, got %s\n", product);

    res = RegSetValueExA(compkey, prod2_squashed, 0, REG_SZ, (const BYTE *)"C:\\another", 10);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* try index 1, second product value does exist */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 1, product);
    todo_wine
    {
        ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
        ok(!lstrcmpA(product, ""), "Expected product to be unchanged, got %s\n", product);
    }

    /* start the enumeration over */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 0, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode) || !lstrcmpA(product, prodcode2),
       "Expected %s or %s, got %s\n", prodcode, prodcode2, product);

    /* correctly query second product */
    product[0] = '\0';
    r = MsiEnumClientsA(component, 1, product);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(product, prodcode) || !lstrcmpA(product, prodcode2),
       "Expected %s or %s, got %s\n", prodcode, prodcode2, product);

    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteValueA(compkey, prod2_squashed);
    RegDeleteKeyA(compkey, "");
    RegCloseKey(compkey);
}

static void get_version_info(LPSTR path, LPSTR *vercheck, LPDWORD verchecksz,
                             LPSTR *langcheck, LPDWORD langchecksz)
{
    LPSTR version;
    VS_FIXEDFILEINFO *ffi;
    DWORD size = GetFileVersionInfoSizeA(path, NULL);
    USHORT *lang;

    version = HeapAlloc(GetProcessHeap(), 0, size);
    GetFileVersionInfoA(path, 0, size, version);

    VerQueryValueA(version, "\\", (LPVOID *)&ffi, &size);
    *vercheck = HeapAlloc(GetProcessHeap(), 0, MAX_PATH);
    sprintf(*vercheck, "%d.%d.%d.%d", HIWORD(ffi->dwFileVersionMS),
            LOWORD(ffi->dwFileVersionMS), HIWORD(ffi->dwFileVersionLS),
            LOWORD(ffi->dwFileVersionLS));
    *verchecksz = lstrlenA(*vercheck);

    VerQueryValue(version, "\\VarFileInfo\\Translation", (void **)&lang, &size);
    *langcheck = HeapAlloc(GetProcessHeap(), 0, MAX_PATH);
    sprintf(*langcheck, "%d", *lang);
    *langchecksz = lstrlenA(*langcheck);

    HeapFree(GetProcessHeap(), 0, version);
}

static void test_MsiGetFileVersion(void)
{
    UINT r;
    DWORD versz, langsz;
    char version[MAX_PATH];
    char lang[MAX_PATH];
    char path[MAX_PATH];
    LPSTR vercheck, langcheck;
    DWORD verchecksz, langchecksz;

    /* NULL szFilePath */
    versz = MAX_PATH;
    langsz = MAX_PATH;
    lstrcpyA(version, "version");
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA(NULL, version, &versz, lang, &langsz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(version, "version"),
       "Expected version to be unchanged, got %s\n", version);
    ok(versz == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, versz);
    ok(!lstrcmpA(lang, "lang"),
       "Expected lang to be unchanged, got %s\n", lang);
    ok(langsz == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, langsz);

    /* empty szFilePath */
    versz = MAX_PATH;
    langsz = MAX_PATH;
    lstrcpyA(version, "version");
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA("", version, &versz, lang, &langsz);
    ok(r == ERROR_FILE_NOT_FOUND,
       "Expected ERROR_FILE_NOT_FOUND, got %d\n", r);
    ok(!lstrcmpA(version, "version"),
       "Expected version to be unchanged, got %s\n", version);
    ok(versz == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, versz);
    ok(!lstrcmpA(lang, "lang"),
       "Expected lang to be unchanged, got %s\n", lang);
    ok(langsz == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, langsz);

    /* nonexistent szFilePath */
    versz = MAX_PATH;
    langsz = MAX_PATH;
    lstrcpyA(version, "version");
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA("nonexistent", version, &versz, lang, &langsz);
    ok(r == ERROR_FILE_NOT_FOUND,
       "Expected ERROR_FILE_NOT_FOUND, got %d\n", r);
    ok(!lstrcmpA(version, "version"),
       "Expected version to be unchanged, got %s\n", version);
    ok(versz == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, versz);
    ok(!lstrcmpA(lang, "lang"),
       "Expected lang to be unchanged, got %s\n", lang);
    ok(langsz == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, langsz);

    /* nonexistent szFilePath, valid lpVersionBuf, NULL pcchVersionBuf */
    versz = MAX_PATH;
    langsz = MAX_PATH;
    lstrcpyA(version, "version");
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA("nonexistent", version, NULL, lang, &langsz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(version, "version"),
       "Expected version to be unchanged, got %s\n", version);
    ok(versz == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, versz);
    ok(!lstrcmpA(lang, "lang"),
       "Expected lang to be unchanged, got %s\n", lang);
    ok(langsz == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, langsz);

    /* nonexistent szFilePath, valid lpLangBuf, NULL pcchLangBuf */
    versz = MAX_PATH;
    langsz = MAX_PATH;
    lstrcpyA(version, "version");
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA("nonexistent", version, &versz, lang, NULL);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(version, "version"),
       "Expected version to be unchanged, got %s\n", version);
    ok(versz == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, versz);
    ok(!lstrcmpA(lang, "lang"),
       "Expected lang to be unchanged, got %s\n", lang);
    ok(langsz == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, langsz);

    /* nonexistent szFilePath, valid lpVersionBuf, pcchVersionBuf is zero */
    versz = 0;
    langsz = MAX_PATH;
    lstrcpyA(version, "version");
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA("nonexistent", version, &versz, lang, &langsz);
    ok(r == ERROR_FILE_NOT_FOUND,
       "Expected ERROR_FILE_NOT_FOUND, got %d\n", r);
    ok(!lstrcmpA(version, "version"),
       "Expected version to be unchanged, got %s\n", version);
    ok(versz == 0, "Expected 0, got %d\n", versz);
    ok(!lstrcmpA(lang, "lang"),
       "Expected lang to be unchanged, got %s\n", lang);
    ok(langsz == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, langsz);

    /* nonexistent szFilePath, valid lpLangBuf, pcchLangBuf is zero */
    versz = MAX_PATH;
    langsz = 0;
    lstrcpyA(version, "version");
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA("nonexistent", version, &versz, lang, &langsz);
    ok(r == ERROR_FILE_NOT_FOUND,
       "Expected ERROR_FILE_NOT_FOUND, got %d\n", r);
    ok(!lstrcmpA(version, "version"),
       "Expected version to be unchanged, got %s\n", version);
    ok(versz == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, versz);
    ok(!lstrcmpA(lang, "lang"),
       "Expected lang to be unchanged, got %s\n", lang);
    ok(langsz == 0, "Expected 0, got %d\n", langsz);

    /* nonexistent szFilePath, rest NULL */
    r = MsiGetFileVersionA("nonexistent", NULL, NULL, NULL, NULL);
    ok(r == ERROR_FILE_NOT_FOUND,
       "Expected ERROR_FILE_NOT_FOUND, got %d\n", r);

    create_file("ver.txt", "ver.txt", 20);

    /* file exists, no version information */
    versz = MAX_PATH;
    langsz = MAX_PATH;
    lstrcpyA(version, "version");
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA("ver.txt", version, &versz, lang, &langsz);
    ok(versz == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, versz);
    ok(!lstrcmpA(version, "version"),
       "Expected version to be unchanged, got %s\n", version);
    ok(langsz == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, langsz);
    ok(!lstrcmpA(lang, "lang"),
       "Expected lang to be unchanged, got %s\n", lang);
    ok(r == ERROR_FILE_INVALID,
       "Expected ERROR_FILE_INVALID, got %d\n", r);

    DeleteFileA("ver.txt");

    /* relative path, has version information */
    versz = MAX_PATH;
    langsz = MAX_PATH;
    lstrcpyA(version, "version");
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA("kernel32.dll", version, &versz, lang, &langsz);
    todo_wine
    {
        ok(r == ERROR_FILE_NOT_FOUND,
           "Expected ERROR_FILE_NOT_FOUND, got %d\n", r);
        ok(!lstrcmpA(version, "version"),
           "Expected version to be unchanged, got %s\n", version);
        ok(versz == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, versz);
        ok(!lstrcmpA(lang, "lang"),
           "Expected lang to be unchanged, got %s\n", lang);
        ok(langsz == MAX_PATH, "Expected %d, got %d\n", MAX_PATH, langsz);
    }

    GetSystemDirectoryA(path, MAX_PATH);
    lstrcatA(path, "\\kernel32.dll");

    get_version_info(path, &vercheck, &verchecksz, &langcheck, &langchecksz);

    /* absolute path, has version information */
    versz = MAX_PATH;
    langsz = MAX_PATH;
    lstrcpyA(version, "version");
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA(path, version, &versz, lang, &langsz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(versz == verchecksz, "Expected %d, got %d\n", verchecksz, versz);
    ok(!lstrcmpA(lang, langcheck), "Expected %s, got %s\n", langcheck, lang);
    ok(langsz == langchecksz, "Expected %d, got %d\n", langchecksz, langsz);
    ok(!lstrcmpA(version, vercheck),
        "Expected %s, got %s\n", vercheck, version);

    /* only check version */
    versz = MAX_PATH;
    lstrcpyA(version, "version");
    r = MsiGetFileVersionA(path, version, &versz, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(versz == verchecksz, "Expected %d, got %d\n", verchecksz, versz);
    ok(!lstrcmpA(version, vercheck),
       "Expected %s, got %s\n", vercheck, version);

    /* only check language */
    langsz = MAX_PATH;
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA(path, NULL, NULL, lang, &langsz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(lang, langcheck), "Expected %s, got %s\n", langcheck, lang);
    ok(langsz == langchecksz, "Expected %d, got %d\n", langchecksz, langsz);

    /* get pcchVersionBuf */
    versz = MAX_PATH;
    r = MsiGetFileVersionA(path, NULL, &versz, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(versz == verchecksz, "Expected %d, got %d\n", verchecksz, versz);

    /* get pcchLangBuf */
    langsz = MAX_PATH;
    r = MsiGetFileVersionA(path, NULL, NULL, NULL, &langsz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(langsz == langchecksz, "Expected %d, got %d\n", langchecksz, langsz);

    /* pcchVersionBuf not big enough */
    versz = 5;
    lstrcpyA(version, "version");
    r = MsiGetFileVersionA(path, version, &versz, NULL, NULL);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!strncmp(version, vercheck, 4),
       "Expected first 4 characters of %s, got %s\n", vercheck, version);
    ok(versz == verchecksz, "Expected %d, got %d\n", verchecksz, versz);

    /* pcchLangBuf not big enough */
    langsz = 3;
    lstrcpyA(lang, "lang");
    r = MsiGetFileVersionA(path, NULL, NULL, lang, &langsz);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!strncmp(lang, langcheck, 2),
       "Expected first character of %s, got %s\n", langcheck, lang);
    ok(langsz == langchecksz, "Expected %d, got %d\n", langchecksz, langsz);

    HeapFree(GetProcessHeap(), 0, vercheck);
    HeapFree(GetProcessHeap(), 0, langcheck);
}

static void test_MsiGetProductInfo(void)
{
    UINT r;
    LONG res;
    HKEY propkey, source;
    HKEY prodkey, localkey;
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR packcode[MAX_PATH];
    CHAR pack_squashed[MAX_PATH];
    CHAR buf[MAX_PATH];
    CHAR keypath[MAX_PATH];
    LPSTR usersid;
    DWORD sz, val = 42;

    create_test_guid(prodcode, prod_squashed);
    create_test_guid(packcode, pack_squashed);
    get_user_sid(&usersid);

    /* NULL szProduct */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(NULL, INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    /* empty szProduct */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA("", INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    /* garbage szProduct */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA("garbage", INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    /* guid without brackets */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA("6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D",
                           INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    /* guid with brackets */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA("{6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D}",
                           INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    /* same length as guid, but random */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA("A938G02JF-2NF3N93-VN3-2NNF-3KGKALDNF93",
                           INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    /* not installed, NULL szAttribute */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, NULL, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    /* not installed, NULL lpValueBuf */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINK, NULL, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    /* not installed, NULL pcchValueBuf */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINK, buf, NULL);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    /* created guid cannot possibly be an installed product code */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* managed product code exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &localkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* local user product code exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* both local and managed product code exist */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegCreateKeyA(localkey, "InstallProperties", &propkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallProperties key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %d\n", sz);

    res = RegSetValueExA(propkey, "HelpLink", 0, REG_SZ, (LPBYTE)"link", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* HelpLink value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "link"), "Expected \"link\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    /* pcchBuf is NULL */
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINK, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* lpValueBuf is NULL */
    sz = MAX_PATH;
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINK, NULL, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    /* lpValueBuf is NULL, pcchValueBuf is too small */
    sz = 2;
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINK, NULL, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    /* lpValueBuf is NULL, pcchValueBuf is too small */
    sz = 2;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to remain unchanged, got \"%s\"\n", buf);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    /* lpValueBuf is NULL, pcchValueBuf is exactly 4 */
    sz = 4;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"),
       "Expected buf to remain unchanged, got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(propkey, "IMadeThis", 0, REG_SZ, (LPBYTE)"random", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* random property not supported by MSI, value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, "IMadeThis", buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected \"apple\", got \"%s\"\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    RegDeleteValueA(propkey, "IMadeThis");
    RegDeleteValueA(propkey, "HelpLink");
    RegDeleteKeyA(propkey, "");
    RegDeleteKeyA(localkey, "");
    RegDeleteKeyA(prodkey, "");
    RegCloseKey(propkey);
    RegCloseKey(localkey);
    RegCloseKey(prodkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user product key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected \"apple\", got \"%s\"\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &localkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* local user product key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected \"apple\", got \"%s\"\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegCreateKeyA(localkey, "InstallProperties", &propkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallProperties key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %d\n", sz);

    res = RegSetValueExA(propkey, "HelpLink", 0, REG_SZ, (LPBYTE)"link", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* HelpLink value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "link"), "Expected \"link\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    RegDeleteValueA(propkey, "HelpLink");
    RegDeleteKeyA(propkey, "");
    RegDeleteKeyA(localkey, "");
    RegDeleteKeyA(prodkey, "");
    RegCloseKey(propkey);
    RegCloseKey(localkey);
    RegCloseKey(prodkey);

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* classes product key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected \"apple\", got \"%s\"\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &localkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* local user product key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected \"apple\", got \"%s\"\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegCreateKeyA(localkey, "InstallProperties", &propkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallProperties key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected \"apple\", got \"%s\"\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    RegDeleteKeyA(propkey, "");
    RegDeleteKeyA(localkey, "");
    RegCloseKey(propkey);
    RegCloseKey(localkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, "S-1-5-18\\\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &localkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Local System product key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
        "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected \"apple\", got \"%s\"\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegCreateKeyA(localkey, "InstallProperties", &propkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallProperties key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %d\n", sz);

    res = RegSetValueExA(propkey, "HelpLink", 0, REG_SZ, (LPBYTE)"link", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* HelpLink value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "link"), "Expected \"link\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(propkey, "HelpLink", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* HelpLink type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "DisplayName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* DisplayName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_INSTALLEDPRODUCTNAME, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "name"), "Expected \"name\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(propkey, "DisplayName", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* DisplayName type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_INSTALLEDPRODUCTNAME, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "DisplayVersion", 0, REG_SZ, (LPBYTE)"1.1.1", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* DisplayVersion value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_VERSIONSTRING, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "1.1.1"), "Expected \"1.1.1\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %d\n", sz);

    res = RegSetValueExA(propkey, "DisplayVersion", 0,
                         REG_DWORD, (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* DisplayVersion type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_VERSIONSTRING, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "HelpTelephone", 0, REG_SZ, (LPBYTE)"tele", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* HelpTelephone value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPTELEPHONE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "tele"), "Expected \"tele\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(propkey, "HelpTelephone", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* HelpTelephone type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_HELPTELEPHONE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "InstallLocation", 0, REG_SZ, (LPBYTE)"loc", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallLocation value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_INSTALLLOCATION, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "loc"), "Expected \"loc\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %d\n", sz);

    res = RegSetValueExA(propkey, "InstallLocation", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallLocation type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_INSTALLLOCATION, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "InstallSource", 0, REG_SZ, (LPBYTE)"source", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallSource value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_INSTALLSOURCE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "source"), "Expected \"source\", got \"%s\"\n", buf);
    ok(sz == 6, "Expected 6, got %d\n", sz);

    res = RegSetValueExA(propkey, "InstallSource", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallSource type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_INSTALLSOURCE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "InstallDate", 0, REG_SZ, (LPBYTE)"date", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallDate value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_INSTALLDATE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "date"), "Expected \"date\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(propkey, "InstallDate", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallDate type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_INSTALLDATE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "Publisher", 0, REG_SZ, (LPBYTE)"pub", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Publisher value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PUBLISHER, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "pub"), "Expected \"pub\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %d\n", sz);

    res = RegSetValueExA(propkey, "Publisher", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Publisher type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PUBLISHER, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "LocalPackage", 0, REG_SZ, (LPBYTE)"pack", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* LocalPackage value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_LOCALPACKAGE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "pack"), "Expected \"pack\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(propkey, "LocalPackage", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* LocalPackage type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_LOCALPACKAGE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "UrlInfoAbout", 0, REG_SZ, (LPBYTE)"about", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* UrlInfoAbout value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_URLINFOABOUT, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "about"), "Expected \"about\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %d\n", sz);

    res = RegSetValueExA(propkey, "UrlInfoAbout", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* UrlInfoAbout type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_URLINFOABOUT, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "UrlUpdateInfo", 0, REG_SZ, (LPBYTE)"info", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* UrlUpdateInfo value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_URLUPDATEINFO, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "info"), "Expected \"info\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(propkey, "UrlUpdateInfo", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* UrlUpdateInfo type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_URLUPDATEINFO, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "VersionMinor", 0, REG_SZ, (LPBYTE)"1", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* VersionMinor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_VERSIONMINOR, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "1"), "Expected \"1\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %d\n", sz);

    res = RegSetValueExA(propkey, "VersionMinor", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* VersionMinor type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_VERSIONMINOR, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "VersionMajor", 0, REG_SZ, (LPBYTE)"1", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* VersionMajor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_VERSIONMAJOR, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "1"), "Expected \"1\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %d\n", sz);

    res = RegSetValueExA(propkey, "VersionMajor", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* VersionMajor type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_VERSIONMAJOR, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "ProductID", 0, REG_SZ, (LPBYTE)"id", 3);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductID value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PRODUCTID, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "id"), "Expected \"id\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "ProductID", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductID type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PRODUCTID, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "RegCompany", 0, REG_SZ, (LPBYTE)"comp", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* RegCompany value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_REGCOMPANY, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "comp"), "Expected \"comp\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(propkey, "RegCompany", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* RegCompany type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_REGCOMPANY, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "RegOwner", 0, REG_SZ, (LPBYTE)"own", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* RegOwner value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_REGOWNER, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "own"), "Expected \"own\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %d\n", sz);

    res = RegSetValueExA(propkey, "RegOwner", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* RegOwner type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_REGOWNER, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "InstanceType", 0, REG_SZ, (LPBYTE)"type", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstanceType value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_INSTANCETYPE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %d\n", sz);

    res = RegSetValueExA(propkey, "InstanceType", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstanceType type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_INSTANCETYPE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %d\n", sz);

    res = RegSetValueExA(prodkey, "InstanceType", 0, REG_SZ, (LPBYTE)"type", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstanceType value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_INSTANCETYPE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "type"), "Expected \"type\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(prodkey, "InstanceType", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstanceType type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_INSTANCETYPE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "Transforms", 0, REG_SZ, (LPBYTE)"tforms", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Transforms value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_TRANSFORMS, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %d\n", sz);

    res = RegSetValueExA(propkey, "Transforms", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Transforms type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_TRANSFORMS, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %d\n", sz);

    res = RegSetValueExA(prodkey, "Transforms", 0, REG_SZ, (LPBYTE)"tforms", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Transforms value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_TRANSFORMS, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "tforms"), "Expected \"tforms\", got \"%s\"\n", buf);
    ok(sz == 6, "Expected 6, got %d\n", sz);

    res = RegSetValueExA(prodkey, "Transforms", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Transforms type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_TRANSFORMS, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "Language", 0, REG_SZ, (LPBYTE)"lang", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Language value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_LANGUAGE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %d\n", sz);

    res = RegSetValueExA(propkey, "Language", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Language type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_LANGUAGE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %d\n", sz);

    res = RegSetValueExA(prodkey, "Language", 0, REG_SZ, (LPBYTE)"lang", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Language value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_LANGUAGE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "lang"), "Expected \"lang\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(prodkey, "Language", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Language type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_LANGUAGE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "ProductName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PRODUCTNAME, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %d\n", sz);

    res = RegSetValueExA(propkey, "ProductName", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductName type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PRODUCTNAME, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %d\n", sz);

    res = RegSetValueExA(prodkey, "ProductName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PRODUCTNAME, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "name"), "Expected \"name\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(prodkey, "ProductName", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductName type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PRODUCTNAME, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "Assignment", 0, REG_SZ, (LPBYTE)"at", 3);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Assignment value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_ASSIGNMENTTYPE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %d\n", sz);

    res = RegSetValueExA(propkey, "Assignment", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Assignment type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_ASSIGNMENTTYPE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %d\n", sz);

    res = RegSetValueExA(prodkey, "Assignment", 0, REG_SZ, (LPBYTE)"at", 3);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Assignment value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_ASSIGNMENTTYPE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "at"), "Expected \"at\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(prodkey, "Assignment", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Assignment type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_ASSIGNMENTTYPE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "PackageCode", 0, REG_SZ, (LPBYTE)"code", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* PackageCode value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PACKAGECODE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %d\n", sz);

    res = RegSetValueExA(propkey, "PackageCode", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* PackageCode type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PACKAGECODE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %d\n", sz);

    res = RegSetValueExA(prodkey, "PackageCode", 0, REG_SZ, (LPBYTE)"code", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* PackageCode value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PACKAGECODE, buf, &sz);
    ok(r == ERROR_BAD_CONFIGURATION,
       "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(!lstrcmpA(buf, "code"), "Expected \"code\", got \"%s\"\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "PackageCode", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* PackageCode type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PACKAGECODE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(prodkey, "PackageCode", 0, REG_SZ, (LPBYTE)pack_squashed, 33);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* PackageCode value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PACKAGECODE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, packcode), "Expected \"%s\", got \"%s\"\n", packcode, buf);
    ok(sz == 38, "Expected 38, got %d\n", sz);

    res = RegSetValueExA(propkey, "Version", 0, REG_SZ, (LPBYTE)"ver", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Version value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_VERSION, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %d\n", sz);

    res = RegSetValueExA(propkey, "Version", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Version type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_VERSION, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %d\n", sz);

    res = RegSetValueExA(prodkey, "Version", 0, REG_SZ, (LPBYTE)"ver", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Version value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_VERSION, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "ver"), "Expected \"ver\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %d\n", sz);

    res = RegSetValueExA(prodkey, "Version", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Version type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_VERSION, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "ProductIcon", 0, REG_SZ, (LPBYTE)"ico", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductIcon value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PRODUCTICON, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %d\n", sz);

    res = RegSetValueExA(propkey, "ProductIcon", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductIcon type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PRODUCTICON, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %d\n", sz);

    res = RegSetValueExA(prodkey, "ProductIcon", 0, REG_SZ, (LPBYTE)"ico", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductIcon value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PRODUCTICON, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "ico"), "Expected \"ico\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %d\n", sz);

    res = RegSetValueExA(prodkey, "ProductIcon", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductIcon type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PRODUCTICON, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegCreateKeyA(prodkey, "SourceList", &source);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegSetValueExA(source, "PackageName", 0, REG_SZ, (LPBYTE)"packname", 9);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PACKAGENAME, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "packname"), "Expected \"packname\", got \"%s\"\n", buf);
    ok(sz == 8, "Expected 8, got %d\n", sz);

    res = RegSetValueExA(source, "PackageName", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* PackageName type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_PACKAGENAME, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "AuthorizedLUAApp", 0, REG_SZ, (LPBYTE)"auth", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Authorized value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_AUTHORIZED_LUA_APP, buf, &sz);
    if (r != ERROR_UNKNOWN_PROPERTY)
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
        ok(sz == 0, "Expected 0, got %d\n", sz);
    }

    res = RegSetValueExA(propkey, "AuthorizedLUAApp", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* AuthorizedLUAApp type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_AUTHORIZED_LUA_APP, buf, &sz);
    if (r != ERROR_UNKNOWN_PROPERTY)
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
        ok(sz == 0, "Expected 0, got %d\n", sz);
    }

    res = RegSetValueExA(prodkey, "AuthorizedLUAApp", 0, REG_SZ, (LPBYTE)"auth", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Authorized value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_AUTHORIZED_LUA_APP, buf, &sz);
    if (r != ERROR_UNKNOWN_PROPERTY)
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(buf, "auth"), "Expected \"auth\", got \"%s\"\n", buf);
        ok(sz == 4, "Expected 4, got %d\n", sz);
    }

    res = RegSetValueExA(prodkey, "AuthorizedLUAApp", 0, REG_DWORD,
                         (const BYTE *)&val, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* AuthorizedLUAApp type is REG_DWORD */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiGetProductInfoA(prodcode, INSTALLPROPERTY_AUTHORIZED_LUA_APP, buf, &sz);
    if (r != ERROR_UNKNOWN_PROPERTY)
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(buf, "42"), "Expected \"42\", got \"%s\"\n", buf);
        ok(sz == 2, "Expected 2, got %d\n", sz);
    }

    RegDeleteValueA(propkey, "HelpLink");
    RegDeleteValueA(propkey, "DisplayName");
    RegDeleteValueA(propkey, "DisplayVersion");
    RegDeleteValueA(propkey, "HelpTelephone");
    RegDeleteValueA(propkey, "InstallLocation");
    RegDeleteValueA(propkey, "InstallSource");
    RegDeleteValueA(propkey, "InstallDate");
    RegDeleteValueA(propkey, "Publisher");
    RegDeleteValueA(propkey, "LocalPackage");
    RegDeleteValueA(propkey, "UrlInfoAbout");
    RegDeleteValueA(propkey, "UrlUpdateInfo");
    RegDeleteValueA(propkey, "VersionMinor");
    RegDeleteValueA(propkey, "VersionMajor");
    RegDeleteValueA(propkey, "ProductID");
    RegDeleteValueA(propkey, "RegCompany");
    RegDeleteValueA(propkey, "RegOwner");
    RegDeleteValueA(propkey, "InstanceType");
    RegDeleteValueA(propkey, "Transforms");
    RegDeleteValueA(propkey, "Language");
    RegDeleteValueA(propkey, "ProductName");
    RegDeleteValueA(propkey, "Assignment");
    RegDeleteValueA(propkey, "PackageCode");
    RegDeleteValueA(propkey, "Version");
    RegDeleteValueA(propkey, "ProductIcon");
    RegDeleteValueA(propkey, "AuthorizedLUAApp");
    RegDeleteKeyA(propkey, "");
    RegDeleteKeyA(localkey, "");
    RegDeleteValueA(prodkey, "InstanceType");
    RegDeleteValueA(prodkey, "Transforms");
    RegDeleteValueA(prodkey, "Language");
    RegDeleteValueA(prodkey, "ProductName");
    RegDeleteValueA(prodkey, "Assignment");
    RegDeleteValueA(prodkey, "PackageCode");
    RegDeleteValueA(prodkey, "Version");
    RegDeleteValueA(prodkey, "ProductIcon");
    RegDeleteValueA(prodkey, "AuthorizedLUAApp");
    RegDeleteValueA(source, "PackageName");
    RegDeleteKeyA(source, "");
    RegDeleteKeyA(prodkey, "");
    RegCloseKey(propkey);
    RegCloseKey(localkey);
    RegCloseKey(source);
    RegCloseKey(prodkey);
}

static void test_MsiGetProductInfoEx(void)
{
    UINT r;
    LONG res;
    HKEY propkey, userkey;
    HKEY prodkey, localkey;
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR packcode[MAX_PATH];
    CHAR pack_squashed[MAX_PATH];
    CHAR buf[MAX_PATH];
    CHAR keypath[MAX_PATH];
    LPSTR usersid;
    DWORD sz;

    if (!pMsiGetProductInfoExA)
    {
        skip("MsiGetProductInfoExA is not available\n");
        return;
    }

    create_test_guid(prodcode, prod_squashed);
    create_test_guid(packcode, pack_squashed);
    get_user_sid(&usersid);

    /* NULL szProductCode */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(NULL, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_PRODUCTSTATE, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    /* empty szProductCode */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA("", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_PRODUCTSTATE, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    /* garbage szProductCode */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA("garbage", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_PRODUCTSTATE, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    /* guid without brackets */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA("6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D", usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_PRODUCTSTATE, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    /* guid with brackets */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA("{6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D}", usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_PRODUCTSTATE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    /* szValue is non-NULL while pcchValue is NULL */
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_PRODUCTSTATE, buf, NULL);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);

    /* dwContext is out of range */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid, 42,
                              INSTALLPROPERTY_PRODUCTSTATE, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    /* szProperty is NULL */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              NULL, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    /* szProperty is empty */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              "", buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    /* szProperty is not a valid property */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              "notvalid", buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    /* same length as guid, but random */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA("A938G02JF-2NF3N93-VN3-2NNF-3KGKALDNF93", usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_PRODUCTSTATE, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    /* MSIINSTALLCONTEXT_USERUNMANAGED */

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &localkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* local user product key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_PRODUCTSTATE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegCreateKeyA(localkey, "InstallProperties", &propkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallProperties key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_PRODUCTSTATE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "LocalPackage", 0, REG_SZ, (LPBYTE)"local", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* LocalPackage value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_PRODUCTSTATE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "5"), "Expected \"5\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %d\n", sz);

    RegDeleteValueA(propkey, "LocalPackage");

    /* LocalPackage value must exist */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "LocalPackage", 0, REG_SZ, (LPBYTE)"local", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* LocalPackage exists, but HelpLink does not exist */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %d\n", sz);

    res = RegSetValueExA(propkey, "HelpLink", 0, REG_SZ, (LPBYTE)"link", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* HelpLink value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "link"), "Expected \"link\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(propkey, "HelpTelephone", 0, REG_SZ, (LPBYTE)"phone", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* HelpTelephone value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_HELPTELEPHONE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "phone"), "Expected \"phone\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %d\n", sz);

    /* szValue and pcchValue are NULL */
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_HELPTELEPHONE, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* pcchValue is exactly 5 */
    sz = 5;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_HELPTELEPHONE, buf, &sz);
    ok(r == ERROR_MORE_DATA,
       "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 10, "Expected 10, got %d\n", sz);

    /* szValue is NULL, pcchValue is exactly 5 */
    sz = 5;
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_HELPTELEPHONE, NULL, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 10, "Expected 10, got %d\n", sz);

    /* szValue is NULL, pcchValue is MAX_PATH */
    sz = MAX_PATH;
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_HELPTELEPHONE, NULL, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 10, "Expected 10, got %d\n", sz);

    /* pcchValue is exactly 0 */
    sz = 0;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_HELPTELEPHONE, buf, &sz);
    ok(r == ERROR_MORE_DATA,
       "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected \"apple\", got \"%s\"\n", buf);
    ok(sz == 10, "Expected 10, got %d\n", sz);

    res = RegSetValueExA(propkey, "notvalid", 0, REG_SZ, (LPBYTE)"invalid", 8);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* szProperty is not a valid property */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              "notvalid", buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "InstallDate", 0, REG_SZ, (LPBYTE)"date", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallDate value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_INSTALLDATE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "date"), "Expected \"date\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(propkey, "DisplayName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* DisplayName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_INSTALLEDPRODUCTNAME, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "name"), "Expected \"name\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(propkey, "InstallLocation", 0, REG_SZ, (LPBYTE)"loc", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallLocation value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_INSTALLLOCATION, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "loc"), "Expected \"loc\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %d\n", sz);

    res = RegSetValueExA(propkey, "InstallSource", 0, REG_SZ, (LPBYTE)"source", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallSource value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_INSTALLSOURCE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "source"), "Expected \"source\", got \"%s\"\n", buf);
    ok(sz == 6, "Expected 6, got %d\n", sz);

    res = RegSetValueExA(propkey, "LocalPackage", 0, REG_SZ, (LPBYTE)"local", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* LocalPackage value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_LOCALPACKAGE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "local"), "Expected \"local\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %d\n", sz);

    res = RegSetValueExA(propkey, "Publisher", 0, REG_SZ, (LPBYTE)"pub", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Publisher value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_PUBLISHER, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "pub"), "Expected \"pub\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %d\n", sz);

    res = RegSetValueExA(propkey, "URLInfoAbout", 0, REG_SZ, (LPBYTE)"about", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* URLInfoAbout value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_URLINFOABOUT, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "about"), "Expected \"about\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %d\n", sz);

    res = RegSetValueExA(propkey, "URLUpdateInfo", 0, REG_SZ, (LPBYTE)"update", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* URLUpdateInfo value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_URLUPDATEINFO, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "update"), "Expected \"update\", got \"%s\"\n", buf);
    ok(sz == 6, "Expected 6, got %d\n", sz);

    res = RegSetValueExA(propkey, "VersionMinor", 0, REG_SZ, (LPBYTE)"2", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* VersionMinor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_VERSIONMINOR, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "2"), "Expected \"2\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %d\n", sz);

    res = RegSetValueExA(propkey, "VersionMajor", 0, REG_SZ, (LPBYTE)"3", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* VersionMajor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_VERSIONMAJOR, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "3"), "Expected \"3\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %d\n", sz);

    res = RegSetValueExA(propkey, "DisplayVersion", 0, REG_SZ, (LPBYTE)"3.2.1", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* DisplayVersion value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_VERSIONSTRING, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "3.2.1"), "Expected \"3.2.1\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %d\n", sz);

    res = RegSetValueExA(propkey, "ProductID", 0, REG_SZ, (LPBYTE)"id", 3);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductID value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_PRODUCTID, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "id"), "Expected \"id\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "RegCompany", 0, REG_SZ, (LPBYTE)"comp", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* RegCompany value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_REGCOMPANY, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "comp"), "Expected \"comp\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(propkey, "RegOwner", 0, REG_SZ, (LPBYTE)"owner", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* RegOwner value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_REGOWNER, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "owner"), "Expected \"owner\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %d\n", sz);

    res = RegSetValueExA(propkey, "Transforms", 0, REG_SZ, (LPBYTE)"trans", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Transforms value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_TRANSFORMS, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "Language", 0, REG_SZ, (LPBYTE)"lang", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Language value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_LANGUAGE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "ProductName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_PRODUCTNAME, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "AssignmentType", 0, REG_SZ, (LPBYTE)"type", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* FIXME */

    /* AssignmentType value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_ASSIGNMENTTYPE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "PackageCode", 0, REG_SZ, (LPBYTE)"code", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* PackageCode value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_PACKAGECODE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "Version", 0, REG_SZ, (LPBYTE)"ver", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Version value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_VERSION, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "ProductIcon", 0, REG_SZ, (LPBYTE)"icon", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductIcon value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_PRODUCTICON, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "PackageName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* PackageName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_PACKAGENAME, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "AuthorizedLUAApp", 0, REG_SZ, (LPBYTE)"auth", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* AuthorizedLUAApp value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_AUTHORIZED_LUA_APP, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    RegDeleteValueA(propkey, "AuthorizedLUAApp");
    RegDeleteValueA(propkey, "PackageName");
    RegDeleteValueA(propkey, "ProductIcon");
    RegDeleteValueA(propkey, "Version");
    RegDeleteValueA(propkey, "PackageCode");
    RegDeleteValueA(propkey, "AssignmentType");
    RegDeleteValueA(propkey, "ProductName");
    RegDeleteValueA(propkey, "Language");
    RegDeleteValueA(propkey, "Transforms");
    RegDeleteValueA(propkey, "RegOwner");
    RegDeleteValueA(propkey, "RegCompany");
    RegDeleteValueA(propkey, "ProductID");
    RegDeleteValueA(propkey, "DisplayVersion");
    RegDeleteValueA(propkey, "VersionMajor");
    RegDeleteValueA(propkey, "VersionMinor");
    RegDeleteValueA(propkey, "URLUpdateInfo");
    RegDeleteValueA(propkey, "URLInfoAbout");
    RegDeleteValueA(propkey, "Publisher");
    RegDeleteValueA(propkey, "LocalPackage");
    RegDeleteValueA(propkey, "InstallSource");
    RegDeleteValueA(propkey, "InstallLocation");
    RegDeleteValueA(propkey, "DisplayName");
    RegDeleteValueA(propkey, "InstallDate");
    RegDeleteValueA(propkey, "HelpTelephone");
    RegDeleteValueA(propkey, "HelpLink");
    RegDeleteValueA(propkey, "LocalPackage");
    RegDeleteKeyA(propkey, "");
    RegCloseKey(propkey);
    RegDeleteKeyA(localkey, "");
    RegCloseKey(localkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user product key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_PRODUCTSTATE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    RegDeleteKeyA(userkey, "");
    RegCloseKey(userkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_PRODUCTSTATE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "1"), "Expected \"1\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %d\n", sz);

    res = RegSetValueExA(prodkey, "HelpLink", 0, REG_SZ, (LPBYTE)"link", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* HelpLink value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "HelpTelephone", 0, REG_SZ, (LPBYTE)"phone", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* HelpTelephone value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_HELPTELEPHONE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "InstallDate", 0, REG_SZ, (LPBYTE)"date", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallDate value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_INSTALLDATE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "DisplayName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* DisplayName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_INSTALLEDPRODUCTNAME, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "InstallLocation", 0, REG_SZ, (LPBYTE)"loc", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallLocation value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_INSTALLLOCATION, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "InstallSource", 0, REG_SZ, (LPBYTE)"source", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallSource value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_INSTALLSOURCE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "LocalPackage", 0, REG_SZ, (LPBYTE)"local", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* LocalPackage value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_LOCALPACKAGE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "Publisher", 0, REG_SZ, (LPBYTE)"pub", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Publisher value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_PUBLISHER, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "URLInfoAbout", 0, REG_SZ, (LPBYTE)"about", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* URLInfoAbout value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_URLINFOABOUT, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "URLUpdateInfo", 0, REG_SZ, (LPBYTE)"update", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* URLUpdateInfo value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_URLUPDATEINFO, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "VersionMinor", 0, REG_SZ, (LPBYTE)"2", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* VersionMinor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_VERSIONMINOR, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "VersionMajor", 0, REG_SZ, (LPBYTE)"3", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* VersionMajor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_VERSIONMAJOR, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "DisplayVersion", 0, REG_SZ, (LPBYTE)"3.2.1", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* DisplayVersion value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_VERSIONSTRING, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "ProductID", 0, REG_SZ, (LPBYTE)"id", 3);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductID value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_PRODUCTID, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "RegCompany", 0, REG_SZ, (LPBYTE)"comp", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* RegCompany value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_REGCOMPANY, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "RegOwner", 0, REG_SZ, (LPBYTE)"owner", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* RegOwner value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_REGOWNER, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "Transforms", 0, REG_SZ, (LPBYTE)"trans", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Transforms value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_TRANSFORMS, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "trans"), "Expected \"trans\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %d\n", sz);

    res = RegSetValueExA(prodkey, "Language", 0, REG_SZ, (LPBYTE)"lang", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Language value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_LANGUAGE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "lang"), "Expected \"lang\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(prodkey, "ProductName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_PRODUCTNAME, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "name"), "Expected \"name\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(prodkey, "AssignmentType", 0, REG_SZ, (LPBYTE)"type", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* FIXME */

    /* AssignmentType value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_ASSIGNMENTTYPE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %d\n", sz);

    res = RegSetValueExA(prodkey, "PackageCode", 0, REG_SZ, (LPBYTE)"code", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* FIXME */

    /* PackageCode value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_PACKAGECODE, buf, &sz);
    todo_wine
    {
        ok(r == ERROR_BAD_CONFIGURATION,
           "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
        ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
        ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);
    }

    res = RegSetValueExA(prodkey, "Version", 0, REG_SZ, (LPBYTE)"ver", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Version value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_VERSION, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "ver"), "Expected \"ver\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %d\n", sz);

    res = RegSetValueExA(prodkey, "ProductIcon", 0, REG_SZ, (LPBYTE)"icon", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductIcon value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_PRODUCTICON, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "icon"), "Expected \"icon\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(prodkey, "PackageName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* PackageName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_PACKAGENAME, buf, &sz);
    todo_wine
    {
        ok(r == ERROR_UNKNOWN_PRODUCT,
           "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
        ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
        ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);
    }

    res = RegSetValueExA(prodkey, "AuthorizedLUAApp", 0, REG_SZ, (LPBYTE)"auth", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* AuthorizedLUAApp value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              INSTALLPROPERTY_AUTHORIZED_LUA_APP, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "auth"), "Expected \"auth\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    RegDeleteValueA(prodkey, "AuthorizedLUAApp");
    RegDeleteValueA(prodkey, "PackageName");
    RegDeleteValueA(prodkey, "ProductIcon");
    RegDeleteValueA(prodkey, "Version");
    RegDeleteValueA(prodkey, "PackageCode");
    RegDeleteValueA(prodkey, "AssignmentType");
    RegDeleteValueA(prodkey, "ProductName");
    RegDeleteValueA(prodkey, "Language");
    RegDeleteValueA(prodkey, "Transforms");
    RegDeleteValueA(prodkey, "RegOwner");
    RegDeleteValueA(prodkey, "RegCompany");
    RegDeleteValueA(prodkey, "ProductID");
    RegDeleteValueA(prodkey, "DisplayVersion");
    RegDeleteValueA(prodkey, "VersionMajor");
    RegDeleteValueA(prodkey, "VersionMinor");
    RegDeleteValueA(prodkey, "URLUpdateInfo");
    RegDeleteValueA(prodkey, "URLInfoAbout");
    RegDeleteValueA(prodkey, "Publisher");
    RegDeleteValueA(prodkey, "LocalPackage");
    RegDeleteValueA(prodkey, "InstallSource");
    RegDeleteValueA(prodkey, "InstallLocation");
    RegDeleteValueA(prodkey, "DisplayName");
    RegDeleteValueA(prodkey, "InstallDate");
    RegDeleteValueA(prodkey, "HelpTelephone");
    RegDeleteValueA(prodkey, "HelpLink");
    RegDeleteValueA(prodkey, "LocalPackage");
    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);

    /* MSIINSTALLCONTEXT_USERMANAGED */

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &localkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* local user product key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_PRODUCTSTATE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegCreateKeyA(localkey, "InstallProperties", &propkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallProperties key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_PRODUCTSTATE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "ManagedLocalPackage", 0, REG_SZ, (LPBYTE)"local", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ManagedLocalPackage value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_PRODUCTSTATE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "5"), "Expected \"5\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %d\n", sz);

    res = RegSetValueExA(propkey, "HelpLink", 0, REG_SZ, (LPBYTE)"link", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* HelpLink value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "link"), "Expected \"link\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(propkey, "HelpTelephone", 0, REG_SZ, (LPBYTE)"phone", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* HelpTelephone value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_HELPTELEPHONE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "phone"), "Expected \"phone\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %d\n", sz);

    res = RegSetValueExA(propkey, "InstallDate", 0, REG_SZ, (LPBYTE)"date", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallDate value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_INSTALLDATE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "date"), "Expected \"date\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(propkey, "DisplayName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* DisplayName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_INSTALLEDPRODUCTNAME, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "name"), "Expected \"name\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(propkey, "InstallLocation", 0, REG_SZ, (LPBYTE)"loc", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallLocation value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_INSTALLLOCATION, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "loc"), "Expected \"loc\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %d\n", sz);

    res = RegSetValueExA(propkey, "InstallSource", 0, REG_SZ, (LPBYTE)"source", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallSource value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_INSTALLSOURCE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "source"), "Expected \"source\", got \"%s\"\n", buf);
    ok(sz == 6, "Expected 6, got %d\n", sz);

    res = RegSetValueExA(propkey, "LocalPackage", 0, REG_SZ, (LPBYTE)"local", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* LocalPackage value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_LOCALPACKAGE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "local"), "Expected \"local\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %d\n", sz);

    res = RegSetValueExA(propkey, "Publisher", 0, REG_SZ, (LPBYTE)"pub", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Publisher value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_PUBLISHER, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "pub"), "Expected \"pub\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %d\n", sz);

    res = RegSetValueExA(propkey, "URLInfoAbout", 0, REG_SZ, (LPBYTE)"about", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* URLInfoAbout value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_URLINFOABOUT, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "about"), "Expected \"about\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %d\n", sz);

    res = RegSetValueExA(propkey, "URLUpdateInfo", 0, REG_SZ, (LPBYTE)"update", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* URLUpdateInfo value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_URLUPDATEINFO, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "update"), "Expected \"update\", got \"%s\"\n", buf);
    ok(sz == 6, "Expected 6, got %d\n", sz);

    res = RegSetValueExA(propkey, "VersionMinor", 0, REG_SZ, (LPBYTE)"2", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* VersionMinor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_VERSIONMINOR, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "2"), "Expected \"2\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %d\n", sz);

    res = RegSetValueExA(propkey, "VersionMajor", 0, REG_SZ, (LPBYTE)"3", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* VersionMajor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_VERSIONMAJOR, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "3"), "Expected \"3\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %d\n", sz);

    res = RegSetValueExA(propkey, "DisplayVersion", 0, REG_SZ, (LPBYTE)"3.2.1", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* DisplayVersion value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_VERSIONSTRING, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "3.2.1"), "Expected \"3.2.1\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %d\n", sz);

    res = RegSetValueExA(propkey, "ProductID", 0, REG_SZ, (LPBYTE)"id", 3);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductID value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_PRODUCTID, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "id"), "Expected \"id\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "RegCompany", 0, REG_SZ, (LPBYTE)"comp", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* RegCompany value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_REGCOMPANY, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "comp"), "Expected \"comp\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(propkey, "RegOwner", 0, REG_SZ, (LPBYTE)"owner", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* RegOwner value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_REGOWNER, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "owner"), "Expected \"owner\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %d\n", sz);

    res = RegSetValueExA(propkey, "Transforms", 0, REG_SZ, (LPBYTE)"trans", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Transforms value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_TRANSFORMS, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "Language", 0, REG_SZ, (LPBYTE)"lang", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Language value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_LANGUAGE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "ProductName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_PRODUCTNAME, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "AssignmentType", 0, REG_SZ, (LPBYTE)"type", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* FIXME */

    /* AssignmentType value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_ASSIGNMENTTYPE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "PackageCode", 0, REG_SZ, (LPBYTE)"code", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* PackageCode value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_PACKAGECODE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "Version", 0, REG_SZ, (LPBYTE)"ver", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Version value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_VERSION, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "ProductIcon", 0, REG_SZ, (LPBYTE)"icon", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductIcon value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_PRODUCTICON, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "PackageName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* PackageName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_PACKAGENAME, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "AuthorizedLUAApp", 0, REG_SZ, (LPBYTE)"auth", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* AuthorizedLUAApp value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_AUTHORIZED_LUA_APP, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    RegDeleteValueA(propkey, "AuthorizedLUAApp");
    RegDeleteValueA(propkey, "PackageName");
    RegDeleteValueA(propkey, "ProductIcon");
    RegDeleteValueA(propkey, "Version");
    RegDeleteValueA(propkey, "PackageCode");
    RegDeleteValueA(propkey, "AssignmentType");
    RegDeleteValueA(propkey, "ProductName");
    RegDeleteValueA(propkey, "Language");
    RegDeleteValueA(propkey, "Transforms");
    RegDeleteValueA(propkey, "RegOwner");
    RegDeleteValueA(propkey, "RegCompany");
    RegDeleteValueA(propkey, "ProductID");
    RegDeleteValueA(propkey, "DisplayVersion");
    RegDeleteValueA(propkey, "VersionMajor");
    RegDeleteValueA(propkey, "VersionMinor");
    RegDeleteValueA(propkey, "URLUpdateInfo");
    RegDeleteValueA(propkey, "URLInfoAbout");
    RegDeleteValueA(propkey, "Publisher");
    RegDeleteValueA(propkey, "LocalPackage");
    RegDeleteValueA(propkey, "InstallSource");
    RegDeleteValueA(propkey, "InstallLocation");
    RegDeleteValueA(propkey, "DisplayName");
    RegDeleteValueA(propkey, "InstallDate");
    RegDeleteValueA(propkey, "HelpTelephone");
    RegDeleteValueA(propkey, "HelpLink");
    RegDeleteValueA(propkey, "ManagedLocalPackage");
    RegDeleteKeyA(propkey, "");
    RegCloseKey(propkey);
    RegDeleteKeyA(localkey, "");
    RegCloseKey(localkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user product key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_PRODUCTSTATE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "1"), "Expected \"1\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %d\n", sz);

    RegDeleteKeyA(userkey, "");
    RegCloseKey(userkey);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* current user product key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_PRODUCTSTATE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "HelpLink", 0, REG_SZ, (LPBYTE)"link", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* HelpLink value exists, user product key does not exist */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegSetValueExA(userkey, "HelpLink", 0, REG_SZ, (LPBYTE)"link", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* HelpLink value exists, user product key does exist */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(userkey, "HelpTelephone", 0, REG_SZ, (LPBYTE)"phone", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* HelpTelephone value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_HELPTELEPHONE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(userkey, "InstallDate", 0, REG_SZ, (LPBYTE)"date", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallDate value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_INSTALLDATE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(userkey, "DisplayName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* DisplayName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_INSTALLEDPRODUCTNAME, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(userkey, "InstallLocation", 0, REG_SZ, (LPBYTE)"loc", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallLocation value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_INSTALLLOCATION, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(userkey, "InstallSource", 0, REG_SZ, (LPBYTE)"source", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallSource value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_INSTALLSOURCE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(userkey, "LocalPackage", 0, REG_SZ, (LPBYTE)"local", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* LocalPackage value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_LOCALPACKAGE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(userkey, "Publisher", 0, REG_SZ, (LPBYTE)"pub", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Publisher value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_PUBLISHER, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(userkey, "URLInfoAbout", 0, REG_SZ, (LPBYTE)"about", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* URLInfoAbout value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_URLINFOABOUT, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(userkey, "URLUpdateInfo", 0, REG_SZ, (LPBYTE)"update", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* URLUpdateInfo value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_URLUPDATEINFO, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(userkey, "VersionMinor", 0, REG_SZ, (LPBYTE)"2", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* VersionMinor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_VERSIONMINOR, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(userkey, "VersionMajor", 0, REG_SZ, (LPBYTE)"3", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* VersionMajor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_VERSIONMAJOR, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(userkey, "DisplayVersion", 0, REG_SZ, (LPBYTE)"3.2.1", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* DisplayVersion value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_VERSIONSTRING, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(userkey, "ProductID", 0, REG_SZ, (LPBYTE)"id", 3);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductID value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_PRODUCTID, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(userkey, "RegCompany", 0, REG_SZ, (LPBYTE)"comp", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* RegCompany value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_REGCOMPANY, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(userkey, "RegOwner", 0, REG_SZ, (LPBYTE)"owner", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* RegOwner value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_REGOWNER, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(userkey, "Transforms", 0, REG_SZ, (LPBYTE)"trans", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Transforms value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_TRANSFORMS, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "trans"), "Expected \"trans\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %d\n", sz);

    res = RegSetValueExA(userkey, "Language", 0, REG_SZ, (LPBYTE)"lang", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Language value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_LANGUAGE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "lang"), "Expected \"lang\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(userkey, "ProductName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_PRODUCTNAME, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "name"), "Expected \"name\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(userkey, "AssignmentType", 0, REG_SZ, (LPBYTE)"type", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* FIXME */

    /* AssignmentType value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_ASSIGNMENTTYPE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %d\n", sz);

    res = RegSetValueExA(userkey, "PackageCode", 0, REG_SZ, (LPBYTE)"code", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* FIXME */

    /* PackageCode value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_PACKAGECODE, buf, &sz);
    todo_wine
    {
        ok(r == ERROR_BAD_CONFIGURATION,
           "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
        ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
        ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);
    }

    res = RegSetValueExA(userkey, "Version", 0, REG_SZ, (LPBYTE)"ver", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Version value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_VERSION, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "ver"), "Expected \"ver\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %d\n", sz);

    res = RegSetValueExA(userkey, "ProductIcon", 0, REG_SZ, (LPBYTE)"icon", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductIcon value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_PRODUCTICON, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "icon"), "Expected \"icon\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(userkey, "PackageName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* PackageName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_PACKAGENAME, buf, &sz);
    todo_wine
    {
        ok(r == ERROR_UNKNOWN_PRODUCT,
           "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
        ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
        ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);
    }

    res = RegSetValueExA(userkey, "AuthorizedLUAApp", 0, REG_SZ, (LPBYTE)"auth", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* AuthorizedLUAApp value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERMANAGED,
                              INSTALLPROPERTY_AUTHORIZED_LUA_APP, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "auth"), "Expected \"auth\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    RegDeleteValueA(userkey, "AuthorizedLUAApp");
    RegDeleteValueA(userkey, "PackageName");
    RegDeleteValueA(userkey, "ProductIcon");
    RegDeleteValueA(userkey, "Version");
    RegDeleteValueA(userkey, "PackageCode");
    RegDeleteValueA(userkey, "AssignmentType");
    RegDeleteValueA(userkey, "ProductName");
    RegDeleteValueA(userkey, "Language");
    RegDeleteValueA(userkey, "Transforms");
    RegDeleteValueA(userkey, "RegOwner");
    RegDeleteValueA(userkey, "RegCompany");
    RegDeleteValueA(userkey, "ProductID");
    RegDeleteValueA(userkey, "DisplayVersion");
    RegDeleteValueA(userkey, "VersionMajor");
    RegDeleteValueA(userkey, "VersionMinor");
    RegDeleteValueA(userkey, "URLUpdateInfo");
    RegDeleteValueA(userkey, "URLInfoAbout");
    RegDeleteValueA(userkey, "Publisher");
    RegDeleteValueA(userkey, "LocalPackage");
    RegDeleteValueA(userkey, "InstallSource");
    RegDeleteValueA(userkey, "InstallLocation");
    RegDeleteValueA(userkey, "DisplayName");
    RegDeleteValueA(userkey, "InstallDate");
    RegDeleteValueA(userkey, "HelpTelephone");
    RegDeleteValueA(userkey, "HelpLink");
    RegDeleteKeyA(userkey, "");
    RegCloseKey(userkey);
    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);

    /* MSIINSTALLCONTEXT_MACHINE */

    /* szUserSid is non-NULL */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, usersid,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_PRODUCTSTATE, buf, &sz);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\S-1-5-18\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &localkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* local system product key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_PRODUCTSTATE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegCreateKeyA(localkey, "InstallProperties", &propkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallProperties key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_PRODUCTSTATE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "LocalPackage", 0, REG_SZ, (LPBYTE)"local", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* LocalPackage value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_PRODUCTSTATE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "5"), "Expected \"5\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %d\n", sz);

    res = RegSetValueExA(propkey, "HelpLink", 0, REG_SZ, (LPBYTE)"link", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* HelpLink value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "link"), "Expected \"link\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(propkey, "HelpTelephone", 0, REG_SZ, (LPBYTE)"phone", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* HelpTelephone value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_HELPTELEPHONE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "phone"), "Expected \"phone\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %d\n", sz);

    res = RegSetValueExA(propkey, "InstallDate", 0, REG_SZ, (LPBYTE)"date", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallDate value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_INSTALLDATE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "date"), "Expected \"date\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(propkey, "DisplayName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* DisplayName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_INSTALLEDPRODUCTNAME, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "name"), "Expected \"name\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(propkey, "InstallLocation", 0, REG_SZ, (LPBYTE)"loc", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallLocation value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_INSTALLLOCATION, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "loc"), "Expected \"loc\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %d\n", sz);

    res = RegSetValueExA(propkey, "InstallSource", 0, REG_SZ, (LPBYTE)"source", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallSource value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_INSTALLSOURCE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "source"), "Expected \"source\", got \"%s\"\n", buf);
    ok(sz == 6, "Expected 6, got %d\n", sz);

    res = RegSetValueExA(propkey, "LocalPackage", 0, REG_SZ, (LPBYTE)"local", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* LocalPackage value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_LOCALPACKAGE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "local"), "Expected \"local\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %d\n", sz);

    res = RegSetValueExA(propkey, "Publisher", 0, REG_SZ, (LPBYTE)"pub", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Publisher value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_PUBLISHER, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "pub"), "Expected \"pub\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %d\n", sz);

    res = RegSetValueExA(propkey, "URLInfoAbout", 0, REG_SZ, (LPBYTE)"about", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* URLInfoAbout value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_URLINFOABOUT, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "about"), "Expected \"about\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %d\n", sz);

    res = RegSetValueExA(propkey, "URLUpdateInfo", 0, REG_SZ, (LPBYTE)"update", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* URLUpdateInfo value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_URLUPDATEINFO, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "update"), "Expected \"update\", got \"%s\"\n", buf);
    ok(sz == 6, "Expected 6, got %d\n", sz);

    res = RegSetValueExA(propkey, "VersionMinor", 0, REG_SZ, (LPBYTE)"2", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* VersionMinor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_VERSIONMINOR, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "2"), "Expected \"2\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %d\n", sz);

    res = RegSetValueExA(propkey, "VersionMajor", 0, REG_SZ, (LPBYTE)"3", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* VersionMajor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_VERSIONMAJOR, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "3"), "Expected \"3\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %d\n", sz);

    res = RegSetValueExA(propkey, "DisplayVersion", 0, REG_SZ, (LPBYTE)"3.2.1", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* DisplayVersion value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_VERSIONSTRING, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "3.2.1"), "Expected \"3.2.1\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %d\n", sz);

    res = RegSetValueExA(propkey, "ProductID", 0, REG_SZ, (LPBYTE)"id", 3);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductID value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_PRODUCTID, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "id"), "Expected \"id\", got \"%s\"\n", buf);
    ok(sz == 2, "Expected 2, got %d\n", sz);

    res = RegSetValueExA(propkey, "RegCompany", 0, REG_SZ, (LPBYTE)"comp", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* RegCompany value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_REGCOMPANY, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "comp"), "Expected \"comp\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(propkey, "RegOwner", 0, REG_SZ, (LPBYTE)"owner", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* RegOwner value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_REGOWNER, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "owner"), "Expected \"owner\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %d\n", sz);

    res = RegSetValueExA(propkey, "Transforms", 0, REG_SZ, (LPBYTE)"trans", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Transforms value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_TRANSFORMS, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "Language", 0, REG_SZ, (LPBYTE)"lang", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Language value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_LANGUAGE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "ProductName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_PRODUCTNAME, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "AssignmentType", 0, REG_SZ, (LPBYTE)"type", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* FIXME */

    /* AssignmentType value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_ASSIGNMENTTYPE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "PackageCode", 0, REG_SZ, (LPBYTE)"code", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* PackageCode value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_PACKAGECODE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "Version", 0, REG_SZ, (LPBYTE)"ver", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Version value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_VERSION, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "ProductIcon", 0, REG_SZ, (LPBYTE)"icon", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductIcon value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_PRODUCTICON, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "PackageName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* PackageName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_PACKAGENAME, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(propkey, "AuthorizedLUAApp", 0, REG_SZ, (LPBYTE)"auth", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* AuthorizedLUAApp value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_AUTHORIZED_LUA_APP, buf, &sz);
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    RegDeleteValueA(propkey, "AuthorizedLUAApp");
    RegDeleteValueA(propkey, "PackageName");
    RegDeleteValueA(propkey, "ProductIcon");
    RegDeleteValueA(propkey, "Version");
    RegDeleteValueA(propkey, "PackageCode");
    RegDeleteValueA(propkey, "AssignmentType");
    RegDeleteValueA(propkey, "ProductName");
    RegDeleteValueA(propkey, "Language");
    RegDeleteValueA(propkey, "Transforms");
    RegDeleteValueA(propkey, "RegOwner");
    RegDeleteValueA(propkey, "RegCompany");
    RegDeleteValueA(propkey, "ProductID");
    RegDeleteValueA(propkey, "DisplayVersion");
    RegDeleteValueA(propkey, "VersionMajor");
    RegDeleteValueA(propkey, "VersionMinor");
    RegDeleteValueA(propkey, "URLUpdateInfo");
    RegDeleteValueA(propkey, "URLInfoAbout");
    RegDeleteValueA(propkey, "Publisher");
    RegDeleteValueA(propkey, "LocalPackage");
    RegDeleteValueA(propkey, "InstallSource");
    RegDeleteValueA(propkey, "InstallLocation");
    RegDeleteValueA(propkey, "DisplayName");
    RegDeleteValueA(propkey, "InstallDate");
    RegDeleteValueA(propkey, "HelpTelephone");
    RegDeleteValueA(propkey, "HelpLink");
    RegDeleteValueA(propkey, "LocalPackage");
    RegDeleteKeyA(propkey, "");
    RegCloseKey(propkey);
    RegDeleteKeyA(localkey, "");
    RegCloseKey(localkey);

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* local classes product key exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_PRODUCTSTATE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "1"), "Expected \"1\", got \"%s\"\n", buf);
    ok(sz == 1, "Expected 1, got %d\n", sz);

    res = RegSetValueExA(prodkey, "HelpLink", 0, REG_SZ, (LPBYTE)"link", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* HelpLink value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_HELPLINK, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "HelpTelephone", 0, REG_SZ, (LPBYTE)"phone", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* HelpTelephone value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_HELPTELEPHONE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "InstallDate", 0, REG_SZ, (LPBYTE)"date", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallDate value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_INSTALLDATE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "DisplayName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* DisplayName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_INSTALLEDPRODUCTNAME, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "InstallLocation", 0, REG_SZ, (LPBYTE)"loc", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallLocation value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_INSTALLLOCATION, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "InstallSource", 0, REG_SZ, (LPBYTE)"source", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* InstallSource value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_INSTALLSOURCE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "LocalPackage", 0, REG_SZ, (LPBYTE)"local", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* LocalPackage value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_LOCALPACKAGE, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "Publisher", 0, REG_SZ, (LPBYTE)"pub", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Publisher value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_PUBLISHER, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "URLInfoAbout", 0, REG_SZ, (LPBYTE)"about", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* URLInfoAbout value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_URLINFOABOUT, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "URLUpdateInfo", 0, REG_SZ, (LPBYTE)"update", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* URLUpdateInfo value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_URLUPDATEINFO, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "VersionMinor", 0, REG_SZ, (LPBYTE)"2", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* VersionMinor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_VERSIONMINOR, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "VersionMajor", 0, REG_SZ, (LPBYTE)"3", 2);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* VersionMajor value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_VERSIONMAJOR, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "DisplayVersion", 0, REG_SZ, (LPBYTE)"3.2.1", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* DisplayVersion value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_VERSIONSTRING, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "ProductID", 0, REG_SZ, (LPBYTE)"id", 3);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductID value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_PRODUCTID, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "RegCompany", 0, REG_SZ, (LPBYTE)"comp", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* RegCompany value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_REGCOMPANY, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "RegOwner", 0, REG_SZ, (LPBYTE)"owner", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* RegOwner value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_REGOWNER, buf, &sz);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);

    res = RegSetValueExA(prodkey, "Transforms", 0, REG_SZ, (LPBYTE)"trans", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Transforms value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_TRANSFORMS, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "trans"), "Expected \"trans\", got \"%s\"\n", buf);
    ok(sz == 5, "Expected 5, got %d\n", sz);

    res = RegSetValueExA(prodkey, "Language", 0, REG_SZ, (LPBYTE)"lang", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Language value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_LANGUAGE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "lang"), "Expected \"lang\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(prodkey, "ProductName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_PRODUCTNAME, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "name"), "Expected \"name\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(prodkey, "AssignmentType", 0, REG_SZ, (LPBYTE)"type", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* FIXME */

    /* AssignmentType value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_ASSIGNMENTTYPE, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(sz == 0, "Expected 0, got %d\n", sz);

    res = RegSetValueExA(prodkey, "PackageCode", 0, REG_SZ, (LPBYTE)"code", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* FIXME */

    /* PackageCode value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_PACKAGECODE, buf, &sz);
    todo_wine
    {
        ok(r == ERROR_BAD_CONFIGURATION,
           "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
        ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
        ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);
    }

    res = RegSetValueExA(prodkey, "Version", 0, REG_SZ, (LPBYTE)"ver", 4);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Version value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_VERSION, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "ver"), "Expected \"ver\", got \"%s\"\n", buf);
    ok(sz == 3, "Expected 3, got %d\n", sz);

    res = RegSetValueExA(prodkey, "ProductIcon", 0, REG_SZ, (LPBYTE)"icon", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* ProductIcon value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_PRODUCTICON, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "icon"), "Expected \"icon\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    res = RegSetValueExA(prodkey, "PackageName", 0, REG_SZ, (LPBYTE)"name", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* PackageName value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_PACKAGENAME, buf, &sz);
    todo_wine
    {
        ok(r == ERROR_UNKNOWN_PRODUCT,
           "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
        ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
        ok(sz == MAX_PATH, "Expected MAX_PATH, got %d\n", sz);
    }

    res = RegSetValueExA(prodkey, "AuthorizedLUAApp", 0, REG_SZ, (LPBYTE)"auth", 5);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* AuthorizedLUAApp value exists */
    sz = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = pMsiGetProductInfoExA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE,
                              INSTALLPROPERTY_AUTHORIZED_LUA_APP, buf, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "auth"), "Expected \"auth\", got \"%s\"\n", buf);
    ok(sz == 4, "Expected 4, got %d\n", sz);

    RegDeleteValueA(prodkey, "AuthorizedLUAApp");
    RegDeleteValueA(prodkey, "PackageName");
    RegDeleteValueA(prodkey, "ProductIcon");
    RegDeleteValueA(prodkey, "Version");
    RegDeleteValueA(prodkey, "PackageCode");
    RegDeleteValueA(prodkey, "AssignmentType");
    RegDeleteValueA(prodkey, "ProductName");
    RegDeleteValueA(prodkey, "Language");
    RegDeleteValueA(prodkey, "Transforms");
    RegDeleteValueA(prodkey, "RegOwner");
    RegDeleteValueA(prodkey, "RegCompany");
    RegDeleteValueA(prodkey, "ProductID");
    RegDeleteValueA(prodkey, "DisplayVersion");
    RegDeleteValueA(prodkey, "VersionMajor");
    RegDeleteValueA(prodkey, "VersionMinor");
    RegDeleteValueA(prodkey, "URLUpdateInfo");
    RegDeleteValueA(prodkey, "URLInfoAbout");
    RegDeleteValueA(prodkey, "Publisher");
    RegDeleteValueA(prodkey, "LocalPackage");
    RegDeleteValueA(prodkey, "InstallSource");
    RegDeleteValueA(prodkey, "InstallLocation");
    RegDeleteValueA(prodkey, "DisplayName");
    RegDeleteValueA(prodkey, "InstallDate");
    RegDeleteValueA(prodkey, "HelpTelephone");
    RegDeleteValueA(prodkey, "HelpLink");
    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);
}

START_TEST(msi)
{
    init_functionpointers();

    test_usefeature();
    test_null();
    test_getcomponentpath();
    test_MsiGetFileHash();

    if (!pConvertSidToStringSidA)
        skip("ConvertSidToStringSidA not implemented\n");
    else
    {
        /* These tests rely on get_user_sid that needs ConvertSidToStringSidA */
        test_MsiQueryProductState();
        test_MsiQueryFeatureState();
        test_MsiQueryComponentState();
        test_MsiGetComponentPath();
        test_MsiGetProductCode();
        test_MsiEnumClients();
        test_MsiGetProductInfo();
        test_MsiGetProductInfoEx();
    }

    test_MsiGetFileVersion();
}

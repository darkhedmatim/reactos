/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for WinHttpOpen
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#define UNICODE
#include <winsock2.h>
#include <wine/test.h>
#include <winhttp.h>

struct hostent *(WINAPI *pgethostbyname)(const char *);
int (WINAPI *pWSACancelBlockingCall)(void);
int (WINAPI *pWSAGetLastError)(void);

HINTERNET (WINAPI *pWinHttpOpen)(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD);
BOOL (WINAPI *pWinHttpCloseHandle)(HINTERNET);

static
PVOID
GetProc(
    PCSTR FunctionName)
{
    HMODULE ModuleHandle;

    ModuleHandle = GetModuleHandle(L"ws2_32");
    if (!ModuleHandle)
        return NULL;
    return GetProcAddress(ModuleHandle, FunctionName);
}

#define PROC(name) (p##name = GetProc(#name))

static
BOOLEAN
IsWinsockLoaded(VOID)
{
    return GetModuleHandle(L"ws2_32") != NULL;
}

static
BOOLEAN
IsWininetLoaded(VOID)
{
    return GetModuleHandle(L"wininet") != NULL;
}

static
BOOLEAN
IsWinsockInitialized(VOID)
{
    struct hostent *Hostent;

    if (!PROC(gethostbyname) || !PROC(WSAGetLastError))
        return FALSE;

    Hostent = pgethostbyname("localhost");
    if (!Hostent)
        ok_dec(pWSAGetLastError(), WSANOTINITIALISED);
    return Hostent != NULL;
}

static
BOOLEAN
AreLegacyFunctionsSupported(VOID)
{
    int Error;

    if (!PROC(WSACancelBlockingCall) || !PROC(WSAGetLastError))
        return FALSE;

    Error = pWSACancelBlockingCall();
    ok(Error == SOCKET_ERROR, "Error = %d\n", Error);
    ok(pWSAGetLastError() == WSAEOPNOTSUPP ||
       pWSAGetLastError() == WSAEINVAL, "WSAGetLastError = %d\n", pWSAGetLastError());

    return pWSAGetLastError() != WSAEOPNOTSUPP;
}

START_TEST(WinHttpOpen)
{
    HMODULE ModuleHandle;
    HINTERNET InternetHandle;
    BOOL Success;
    
    ok(!IsWinsockLoaded(), "Winsock loaded on startup\n");
    ok(!IsWinsockInitialized(), "Winsock initialized on startup\n");
    ok(!IsWininetLoaded(), "Wininet loaded on startup\n");

    ModuleHandle = GetModuleHandle(L"winhttp");
    ok_ptr(ModuleHandle, NULL);
    ModuleHandle = LoadLibrary(L"winhttp");
    ok(ModuleHandle != NULL, "LoadLibrary failed, error %lu\n", GetLastError());

    pWinHttpOpen = (PVOID)GetProcAddress(ModuleHandle, "WinHttpOpen");
    pWinHttpCloseHandle = (PVOID)GetProcAddress(ModuleHandle, "WinHttpCloseHandle");

    ok(!IsWinsockLoaded(), "Winsock loaded after winhttp load\n");
    ok(!IsWinsockInitialized(), "Winsock initialized after winhttp load\n");
    ok(!IsWininetLoaded(), "Wininet loaded after winhttp load\n");

    InternetHandle = pWinHttpOpen(NULL, WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0);
    ok(InternetHandle != NULL, "InternetHandle = NULL\n");

    if (InternetHandle != NULL)
    {
        ok(IsWinsockLoaded(), "Winsock not loaded after WinHttpOpen\n");
        ok(IsWinsockInitialized(), "Winsock not initialized after WinHttpOpen\n");
        ok(!IsWininetLoaded(), "Wininet loaded after WinHttpOpen\n");
        ok(AreLegacyFunctionsSupported(), "Winsock initialized with version 2\n");
        Success = pWinHttpCloseHandle(InternetHandle);
        ok(Success, "WinHttpCloseHandle failed, error %lu\n", GetLastError());
    }

    ok(IsWinsockLoaded(), "Winsock unloaded after handle close\n");
    ok(IsWinsockInitialized(), "Winsock uninitialized after handle close\n");

    FreeLibrary(ModuleHandle);

    ok(IsWinsockLoaded(), "Winsock unloaded after winhttp unload\n");
    trace("Winsock %sinitialized after winhttp unload (should be uninitialized in 2003, still initialized in 7)\n",
          IsWinsockInitialized() ? "" : "un");
}

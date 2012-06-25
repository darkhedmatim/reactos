/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for WSAStartup
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#define WIN32_NO_STATUS
#include <winsock2.h>
#include <wine/test.h>
#include <pseh/pseh2.h>
#include <ndk/rtlfuncs.h>
#include <ndk/mmfuncs.h>

#define StartSeh()              ExceptionStatus = STATUS_SUCCESS; _SEH2_TRY {
#define EndSeh(ExpectedStatus)  } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) { ExceptionStatus = _SEH2_GetExceptionCode(); } _SEH2_END; ok(ExceptionStatus == ExpectedStatus, "Exception %lx, expected %lx\n", ExceptionStatus, ExpectedStatus)

static
PVOID
AllocateGuarded(
    SIZE_T SizeRequested)
{
    NTSTATUS Status;
    SIZE_T Size = PAGE_ROUND_UP(SizeRequested + PAGE_SIZE);
    PVOID VirtualMemory = NULL;
    PCHAR StartOfBuffer;

    Status = NtAllocateVirtualMemory(NtCurrentProcess(), &VirtualMemory, 0, &Size, MEM_RESERVE, PAGE_NOACCESS);

    if (!NT_SUCCESS(Status))
        return NULL;

    Size -= PAGE_SIZE;
    if (Size)
    {
        Status = NtAllocateVirtualMemory(NtCurrentProcess(), &VirtualMemory, 0, &Size, MEM_COMMIT, PAGE_READWRITE);
        if (!NT_SUCCESS(Status))
        {
            Size = 0;
            Status = NtFreeVirtualMemory(NtCurrentProcess(), &VirtualMemory, &Size, MEM_RELEASE);
            ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
            return NULL;
        }
    }

    StartOfBuffer = VirtualMemory;
    StartOfBuffer += Size - SizeRequested;

    return StartOfBuffer;
}

static
VOID
FreeGuarded(
    PVOID Pointer)
{
    NTSTATUS Status;
    PVOID VirtualMemory = (PVOID)PAGE_ROUND_DOWN((SIZE_T)Pointer);
    SIZE_T Size = 0;

    Status = NtFreeVirtualMemory(NtCurrentProcess(), &VirtualMemory, &Size, MEM_RELEASE);
    ok(Status == STATUS_SUCCESS, "Status = %lx\n", Status);
}

static
BOOLEAN
IsInitialized(VOID)
{
    struct hostent *Hostent;

    Hostent = gethostbyname("localhost");
    if (!Hostent)
        ok_dec(WSAGetLastError(), WSANOTINITIALISED);
    return Hostent != NULL;
}

static
BOOLEAN
AreLegacyFunctionsSupported(VOID)
{
    int Error;

    Error = WSACancelBlockingCall();
    ok(Error == SOCKET_ERROR, "Error = %d\n", Error);
    ok(WSAGetLastError() == WSAEOPNOTSUPP ||
       WSAGetLastError() == WSAEINVAL, "WSAGetLastError = %d\n", WSAGetLastError());

    return WSAGetLastError() != WSAEOPNOTSUPP;
}

START_TEST(WSAStartup)
{
    NTSTATUS ExceptionStatus;
    LPWSADATA WsaData;
    int Error;
    struct
    {
        WORD Version;
        BOOLEAN ExpectedSuccess;
        WORD ExpectedVersion;
    } Tests[] =
    {
        { MAKEWORD(0, 0),   FALSE, MAKEWORD(2, 2) },
        { MAKEWORD(0, 9),   FALSE, MAKEWORD(2, 2) },
        { MAKEWORD(1, 0),   TRUE },
        { MAKEWORD(1, 1),   TRUE },
        { MAKEWORD(1, 2),   TRUE, MAKEWORD(1, 1) },
        { MAKEWORD(1, 15),  TRUE, MAKEWORD(1, 1) },
        { MAKEWORD(1, 255), TRUE, MAKEWORD(1, 1) },
        { MAKEWORD(2, 0),   TRUE },
        { MAKEWORD(2, 1),   TRUE },
        { MAKEWORD(2, 2),   TRUE },
        { MAKEWORD(2, 3),   TRUE, MAKEWORD(2, 2) },
        { MAKEWORD(2, 15),  TRUE, MAKEWORD(2, 2) },
        { MAKEWORD(2, 255), TRUE, MAKEWORD(2, 2) },
        { MAKEWORD(3, 0),   TRUE, MAKEWORD(2, 2) },
        { MAKEWORD(3, 1),   TRUE, MAKEWORD(2, 2) },
        { MAKEWORD(3, 2),   TRUE, MAKEWORD(2, 2) },
        { MAKEWORD(3, 3),   TRUE, MAKEWORD(2, 2) },
        { MAKEWORD(3, 15),  TRUE, MAKEWORD(2, 2) },
        { MAKEWORD(3, 255), TRUE, MAKEWORD(2, 2) },
        { MAKEWORD(15, 255), TRUE, MAKEWORD(2, 2) },
        { MAKEWORD(255, 255), TRUE, MAKEWORD(2, 2) },
    };
    const INT TestCount = sizeof(Tests) / sizeof(Tests[0]);
    INT i;

    ok(!IsInitialized(), "Winsock unexpectedly initialized\n");

    /* parameter checks */
    StartSeh()
        Error = WSAStartup(0, NULL);
        ok_dec(Error, WSAVERNOTSUPPORTED);
        ok_dec(WSAGetLastError(), WSANOTINITIALISED);
    EndSeh(STATUS_SUCCESS);
    StartSeh()
        Error = WSAStartup(MAKEWORD(2, 2), NULL);
        ok_dec(Error, WSAEFAULT);
        ok_dec(WSAGetLastError(), WSANOTINITIALISED);
    EndSeh(STATUS_SUCCESS);
    ok(!IsInitialized(), "Winsock unexpectedly initialized\n");

    WsaData = AllocateGuarded(sizeof(*WsaData));
    if (!WsaData)
    {
        skip("Out of memory\n");
        return;
    }

    /* test different version */
    for (i = 0; i < TestCount; i++)
    {
        trace("%d: version %d.%d\n",
              i, LOBYTE(Tests[i].Version), HIBYTE(Tests[i].Version));
        FillMemory(WsaData, sizeof(*WsaData), 0x55);
        Error = WSANO_RECOVERY;
        StartSeh()
            Error = WSAStartup(Tests[i].Version, WsaData);
        EndSeh(STATUS_SUCCESS);
        if (Error)
        {
            ok(!Tests[i].ExpectedSuccess, "WSAStartup failed unexpectedly\n");
            ok_dec(Error, WSAVERNOTSUPPORTED);
            ok_dec(WSAGetLastError(), WSANOTINITIALISED);
            ok(!IsInitialized(), "Winsock unexpectedly initialized\n");
        }
        else
        {
            ok(Tests[i].ExpectedSuccess, "WSAStartup succeeded unexpectedly\n");
            ok_dec(WSAGetLastError(), 0);
            ok(IsInitialized(), "Winsock not initialized despite success\n");
            if (LOBYTE(Tests[i].Version) < 2)
                ok(AreLegacyFunctionsSupported(), "Legacy function failed\n");
            else
                ok(!AreLegacyFunctionsSupported(), "Legacy function succeeded\n");
            WSACleanup();
            ok(!IsInitialized(), "Winsock still initialized after cleanup\n");
        }
        if (Tests[i].ExpectedVersion)
            ok_hex(WsaData->wVersion, Tests[i].ExpectedVersion);
        else
            ok_hex(WsaData->wVersion, Tests[i].Version);
        ok_hex(WsaData->wHighVersion, MAKEWORD(2, 2));
        ok_str(WsaData->szDescription, "WinSock 2.0");
        ok_str(WsaData->szSystemStatus, "Running");
        if (LOBYTE(WsaData->wVersion) >= 2)
        {
            ok_dec(WsaData->iMaxSockets, 0);
            ok_dec(WsaData->iMaxUdpDg, 0);
        }
        else
        {
            ok_dec(WsaData->iMaxSockets, 32767);
            ok_dec(WsaData->iMaxUdpDg, 65467);
        }
        ok_ptr(WsaData->lpVendorInfo, (PVOID)0x5555555555555555ULL);
    }

    /* upgrade the version */
    FillMemory(WsaData, sizeof(*WsaData), 0x55);
    Error = WSAStartup(MAKEWORD(1, 1), WsaData);
    ok_dec(Error, 0);
    ok_dec(WSAGetLastError(), 0);
    ok_hex(WsaData->wVersion, MAKEWORD(1, 1));
    ok_hex(WsaData->wHighVersion, MAKEWORD(2, 2));
    ok_str(WsaData->szDescription, "WinSock 2.0");
    ok_str(WsaData->szSystemStatus, "Running");
    ok_dec(WsaData->iMaxSockets, 32767);
    ok_dec(WsaData->iMaxUdpDg, 65467);
    ok_ptr(WsaData->lpVendorInfo, (PVOID)0x5555555555555555ULL);
    ok(IsInitialized(), "Winsock not initialized\n");
    if (!Error)
    {
        ok(AreLegacyFunctionsSupported(), "Legacy function failed\n");
        FillMemory(WsaData, sizeof(*WsaData), 0x55);
        Error = WSAStartup(MAKEWORD(2, 2), WsaData);
        ok_dec(Error, 0);
        ok_hex(WsaData->wVersion, MAKEWORD(2, 2));
        ok_hex(WsaData->wHighVersion, MAKEWORD(2, 2));
        ok_str(WsaData->szDescription, "WinSock 2.0");
        ok_str(WsaData->szSystemStatus, "Running");
        ok_dec(WsaData->iMaxSockets, 0);
        ok_dec(WsaData->iMaxUdpDg, 0);
        ok_ptr(WsaData->lpVendorInfo, (PVOID)0x5555555555555555ULL);
        if (!Error)
        {
            ok(AreLegacyFunctionsSupported(), "Legacy function failed\n");
            WSACleanup();
            ok(IsInitialized(), "Winsock prematurely uninitialized\n");
        }
        WSACleanup();
        ok(!IsInitialized(), "Winsock still initialized after cleanup\n");
    }

    /* downgrade the version */
    FillMemory(WsaData, sizeof(*WsaData), 0x55);
    Error = WSAStartup(MAKEWORD(2, 2), WsaData);
    ok_dec(Error, 0);
    ok_dec(WSAGetLastError(), 0);
    ok_hex(WsaData->wVersion, MAKEWORD(2, 2));
    ok_hex(WsaData->wHighVersion, MAKEWORD(2, 2));
    ok_str(WsaData->szDescription, "WinSock 2.0");
    ok_str(WsaData->szSystemStatus, "Running");
    ok_dec(WsaData->iMaxSockets, 0);
    ok_dec(WsaData->iMaxUdpDg, 0);
    ok_ptr(WsaData->lpVendorInfo, (PVOID)0x5555555555555555ULL);
    ok(IsInitialized(), "Winsock not initialized\n");
    if (!Error)
    {
        ok(!AreLegacyFunctionsSupported(), "Legacy function succeeded\n");
        FillMemory(WsaData, sizeof(*WsaData), 0x55);
        Error = WSAStartup(MAKEWORD(1, 1), WsaData);
        ok_dec(Error, 0);
        ok_hex(WsaData->wVersion, MAKEWORD(1, 1));
        ok_hex(WsaData->wHighVersion, MAKEWORD(2, 2));
        ok_str(WsaData->szDescription, "WinSock 2.0");
        ok_str(WsaData->szSystemStatus, "Running");
        ok_dec(WsaData->iMaxSockets, 32767);
        ok_dec(WsaData->iMaxUdpDg, 65467);
        ok_ptr(WsaData->lpVendorInfo, (PVOID)0x5555555555555555ULL);
        if (!Error)
        {
            ok(AreLegacyFunctionsSupported(), "Legacy function failed\n");
            WSACleanup();
            ok(IsInitialized(), "Winsock prematurely uninitialized\n");
        }
        WSACleanup();
        ok(!IsInitialized(), "Winsock still initialized after cleanup\n");
    }

    FreeGuarded(WsaData);
}

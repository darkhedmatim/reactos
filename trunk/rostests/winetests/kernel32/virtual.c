/*
 * Unit test suite for Virtual* family of APIs.
 *
 * Copyright 2004 Dmitry Timoshkov
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
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winerror.h"
#include "wine/test.h"

#define NUM_THREADS 4
#define MAPPING_SIZE 0x100000

static HINSTANCE hkernel32;
static LPVOID (WINAPI *pVirtualAllocEx)(HANDLE, LPVOID, SIZE_T, DWORD, DWORD);
static BOOL   (WINAPI *pVirtualFreeEx)(HANDLE, LPVOID, SIZE_T, DWORD);
static UINT   (WINAPI *pGetWriteWatch)(DWORD,LPVOID,SIZE_T,LPVOID*,ULONG_PTR*,ULONG*);
static UINT   (WINAPI *pResetWriteWatch)(LPVOID,SIZE_T);
static NTSTATUS (WINAPI *pNtAreMappedFilesTheSame)(PVOID,PVOID);

/* ############################### */

static HANDLE create_target_process(const char *arg)
{
    char **argv;
    char cmdline[MAX_PATH];
    PROCESS_INFORMATION pi;
    BOOL ret;
    STARTUPINFO si = { 0 };
    si.cb = sizeof(si);

    winetest_get_mainargs( &argv );
    sprintf(cmdline, "%s %s %s", argv[0], argv[1], arg);
    ret = CreateProcess(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "error: %u\n", GetLastError());
    ret = CloseHandle(pi.hThread);
    ok(ret, "error %u\n", GetLastError());
    return pi.hProcess;
}

static void test_VirtualAllocEx(void)
{
    const unsigned int alloc_size = 1<<15;
    char *src, *dst;
    SIZE_T bytes_written = 0, bytes_read = 0, i;
    void *addr1, *addr2;
    BOOL b;
    DWORD old_prot;
    MEMORY_BASIC_INFORMATION info;
    HANDLE hProcess;

    /* not exported in all windows-versions  */
    if ((!pVirtualAllocEx) || (!pVirtualFreeEx)) {
        win_skip("Virtual{Alloc,Free}Ex not available\n");
        return;
    }

    hProcess = create_target_process("sleep");
    ok(hProcess != NULL, "Can't start process\n");

    SetLastError(0xdeadbeef);
    addr1 = pVirtualAllocEx(hProcess, NULL, alloc_size, MEM_COMMIT,
                           PAGE_EXECUTE_READWRITE);
    if (!addr1 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {   /* Win9x */
        win_skip("VirtualAllocEx not implemented\n");
        TerminateProcess(hProcess, 0);
        CloseHandle(hProcess);
        return;
    }

    src = VirtualAlloc( NULL, alloc_size, MEM_COMMIT, PAGE_READWRITE );
    dst = VirtualAlloc( NULL, alloc_size, MEM_COMMIT, PAGE_READWRITE );
    for (i = 0; i < alloc_size; i++)
        src[i] = i & 0xff;

    ok(addr1 != NULL, "VirtualAllocEx error %u\n", GetLastError());
    b = WriteProcessMemory(hProcess, addr1, src, alloc_size, &bytes_written);
    ok(b && (bytes_written == alloc_size), "%lu bytes written\n",
       bytes_written);
    b = ReadProcessMemory(hProcess, addr1, dst, alloc_size, &bytes_read);
    ok(b && (bytes_read == alloc_size), "%lu bytes read\n", bytes_read);
    ok(!memcmp(src, dst, alloc_size), "Data from remote process differs\n");

    /* test invalid source buffers */

    b = VirtualProtect( src + 0x2000, 0x2000, PAGE_NOACCESS, &old_prot );
    ok( b, "VirtualProtect failed error %u\n", GetLastError() );
    b = WriteProcessMemory(hProcess, addr1, src, alloc_size, &bytes_written);
    ok( !b, "WriteProcessMemory succeeded\n" );
    ok( GetLastError() == ERROR_NOACCESS ||
        GetLastError() == ERROR_PARTIAL_COPY, /* vista */
        "wrong error %u\n", GetLastError() );
    ok( bytes_written == 0, "%lu bytes written\n", bytes_written );
    b = ReadProcessMemory(hProcess, addr1, src, alloc_size, &bytes_read);
    ok( !b, "ReadProcessMemory succeeded\n" );
    ok( GetLastError() == ERROR_NOACCESS, "wrong error %u\n", GetLastError() );
    ok( bytes_read == 0, "%lu bytes written\n", bytes_read );

    b = VirtualProtect( src, 0x2000, PAGE_NOACCESS, &old_prot );
    ok( b, "VirtualProtect failed error %u\n", GetLastError() );
    b = WriteProcessMemory(hProcess, addr1, src, alloc_size, &bytes_written);
    ok( !b, "WriteProcessMemory succeeded\n" );
    ok( GetLastError() == ERROR_NOACCESS ||
        GetLastError() == ERROR_PARTIAL_COPY, /* vista */
        "wrong error %u\n", GetLastError() );
    ok( bytes_written == 0, "%lu bytes written\n", bytes_written );
    b = ReadProcessMemory(hProcess, addr1, src, alloc_size, &bytes_read);
    ok( !b, "ReadProcessMemory succeeded\n" );
    ok( GetLastError() == ERROR_NOACCESS, "wrong error %u\n", GetLastError() );
    ok( bytes_read == 0, "%lu bytes written\n", bytes_read );

    b = pVirtualFreeEx(hProcess, addr1, 0, MEM_RELEASE);
    ok(b != 0, "VirtualFreeEx, error %u\n", GetLastError());

    VirtualFree( src, 0, MEM_FREE );
    VirtualFree( dst, 0, MEM_FREE );

    /*
     * The following tests parallel those in test_VirtualAlloc()
     */

    SetLastError(0xdeadbeef);
    addr1 = pVirtualAllocEx(hProcess, 0, 0, MEM_RESERVE, PAGE_NOACCESS);
    ok(addr1 == NULL, "VirtualAllocEx should fail on zero-sized allocation\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER /* NT */ ||
       GetLastError() == ERROR_NOT_ENOUGH_MEMORY, /* Win9x */
        "got %u, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    addr1 = pVirtualAllocEx(hProcess, 0, 0xFFFC, MEM_RESERVE, PAGE_NOACCESS);
    ok(addr1 != NULL, "VirtualAllocEx failed\n");

    /* test a not committed memory */
    memset(&info, 'q', sizeof(info));
    ok(VirtualQueryEx(hProcess, addr1, &info, sizeof(info)) == sizeof(info), "VirtualQueryEx failed\n");
    ok(info.BaseAddress == addr1, "%p != %p\n", info.BaseAddress, addr1);
    ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
    ok(info.AllocationProtect == PAGE_NOACCESS, "%x != PAGE_NOACCESS\n", info.AllocationProtect);
    ok(info.RegionSize == 0x10000, "%lx != 0x10000\n", info.RegionSize);
    ok(info.State == MEM_RESERVE, "%x != MEM_RESERVE\n", info.State);
    /* NT reports Protect == 0 for a not committed memory block */
    ok(info.Protect == 0 /* NT */ ||
       info.Protect == PAGE_NOACCESS, /* Win9x */
        "%x != PAGE_NOACCESS\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "%x != MEM_PRIVATE\n", info.Type);

    SetLastError(0xdeadbeef);
    ok(!VirtualProtectEx(hProcess, addr1, 0xFFFC, PAGE_READONLY, &old_prot),
       "VirtualProtectEx should fail on a not committed memory\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS /* NT */ ||
       GetLastError() == ERROR_INVALID_PARAMETER, /* Win9x */
        "got %u, expected ERROR_INVALID_ADDRESS\n", GetLastError());

    addr2 = pVirtualAllocEx(hProcess, addr1, 0x1000, MEM_COMMIT, PAGE_NOACCESS);
    ok(addr1 == addr2, "VirtualAllocEx failed\n");

    /* test a committed memory */
    ok(VirtualQueryEx(hProcess, addr1, &info, sizeof(info)) == sizeof(info),
        "VirtualQueryEx failed\n");
    ok(info.BaseAddress == addr1, "%p != %p\n", info.BaseAddress, addr1);
    ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
    ok(info.AllocationProtect == PAGE_NOACCESS, "%x != PAGE_NOACCESS\n", info.AllocationProtect);
    ok(info.RegionSize == 0x1000, "%lx != 0x1000\n", info.RegionSize);
    ok(info.State == MEM_COMMIT, "%x != MEM_COMMIT\n", info.State);
    /* this time NT reports PAGE_NOACCESS as well */
    ok(info.Protect == PAGE_NOACCESS, "%x != PAGE_NOACCESS\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "%x != MEM_PRIVATE\n", info.Type);

    /* this should fail, since not the whole range is committed yet */
    SetLastError(0xdeadbeef);
    ok(!VirtualProtectEx(hProcess, addr1, 0xFFFC, PAGE_READONLY, &old_prot),
        "VirtualProtectEx should fail on a not committed memory\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS /* NT */ ||
       GetLastError() == ERROR_INVALID_PARAMETER, /* Win9x */
        "got %u, expected ERROR_INVALID_ADDRESS\n", GetLastError());

    old_prot = 0;
    ok(VirtualProtectEx(hProcess, addr1, 0x1000, PAGE_READONLY, &old_prot), "VirtualProtectEx failed\n");
    ok(old_prot == PAGE_NOACCESS, "wrong old protection: got %04x instead of PAGE_NOACCESS\n", old_prot);

    old_prot = 0;
    ok(VirtualProtectEx(hProcess, addr1, 0x1000, PAGE_READWRITE, &old_prot), "VirtualProtectEx failed\n");
    ok(old_prot == PAGE_READONLY, "wrong old protection: got %04x instead of PAGE_READONLY\n", old_prot);

    ok(!pVirtualFreeEx(hProcess, addr1, 0x10000, 0),
       "VirtualFreeEx should fail with type 0\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
        "got %u, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    ok(pVirtualFreeEx(hProcess, addr1, 0x10000, MEM_DECOMMIT), "VirtualFreeEx failed\n");

    /* if the type is MEM_RELEASE, size must be 0 */
    ok(!pVirtualFreeEx(hProcess, addr1, 1, MEM_RELEASE),
       "VirtualFreeEx should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
        "got %u, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    ok(pVirtualFreeEx(hProcess, addr1, 0, MEM_RELEASE), "VirtualFreeEx failed\n");

    TerminateProcess(hProcess, 0);
    CloseHandle(hProcess);
}

static void test_VirtualAlloc(void)
{
    void *addr1, *addr2;
    DWORD old_prot;
    MEMORY_BASIC_INFORMATION info;

    SetLastError(0xdeadbeef);
    addr1 = VirtualAlloc(0, 0, MEM_RESERVE, PAGE_NOACCESS);
    ok(addr1 == NULL, "VirtualAlloc should fail on zero-sized allocation\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER /* NT */ ||
       GetLastError() == ERROR_NOT_ENOUGH_MEMORY, /* Win9x */
        "got %d, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    addr1 = VirtualAlloc(0, 0xFFFC, MEM_RESERVE, PAGE_NOACCESS);
    ok(addr1 != NULL, "VirtualAlloc failed\n");

    /* test a not committed memory */
    ok(VirtualQuery(addr1, &info, sizeof(info)) == sizeof(info),
        "VirtualQuery failed\n");
    ok(info.BaseAddress == addr1, "%p != %p\n", info.BaseAddress, addr1);
    ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
    ok(info.AllocationProtect == PAGE_NOACCESS, "%x != PAGE_NOACCESS\n", info.AllocationProtect);
    ok(info.RegionSize == 0x10000, "%lx != 0x10000\n", info.RegionSize);
    ok(info.State == MEM_RESERVE, "%x != MEM_RESERVE\n", info.State);
    /* NT reports Protect == 0 for a not committed memory block */
    ok(info.Protect == 0 /* NT */ ||
       info.Protect == PAGE_NOACCESS, /* Win9x */
        "%x != PAGE_NOACCESS\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "%x != MEM_PRIVATE\n", info.Type);

    SetLastError(0xdeadbeef);
    ok(!VirtualProtect(addr1, 0xFFFC, PAGE_READONLY, &old_prot),
       "VirtualProtect should fail on a not committed memory\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS /* NT */ ||
       GetLastError() == ERROR_INVALID_PARAMETER, /* Win9x */
        "got %d, expected ERROR_INVALID_ADDRESS\n", GetLastError());

    addr2 = VirtualAlloc(addr1, 0x1000, MEM_COMMIT, PAGE_NOACCESS);
    ok(addr1 == addr2, "VirtualAlloc failed\n");

    /* test a committed memory */
    ok(VirtualQuery(addr1, &info, sizeof(info)) == sizeof(info),
        "VirtualQuery failed\n");
    ok(info.BaseAddress == addr1, "%p != %p\n", info.BaseAddress, addr1);
    ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
    ok(info.AllocationProtect == PAGE_NOACCESS, "%x != PAGE_NOACCESS\n", info.AllocationProtect);
    ok(info.RegionSize == 0x1000, "%lx != 0x1000\n", info.RegionSize);
    ok(info.State == MEM_COMMIT, "%x != MEM_COMMIT\n", info.State);
    /* this time NT reports PAGE_NOACCESS as well */
    ok(info.Protect == PAGE_NOACCESS, "%x != PAGE_NOACCESS\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "%x != MEM_PRIVATE\n", info.Type);

    /* this should fail, since not the whole range is committed yet */
    SetLastError(0xdeadbeef);
    ok(!VirtualProtect(addr1, 0xFFFC, PAGE_READONLY, &old_prot),
        "VirtualProtect should fail on a not committed memory\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS /* NT */ ||
       GetLastError() == ERROR_INVALID_PARAMETER, /* Win9x */
        "got %d, expected ERROR_INVALID_ADDRESS\n", GetLastError());

    ok(VirtualProtect(addr1, 0x1000, PAGE_READONLY, &old_prot), "VirtualProtect failed\n");
    ok(old_prot == PAGE_NOACCESS,
        "wrong old protection: got %04x instead of PAGE_NOACCESS\n", old_prot);

    ok(VirtualProtect(addr1, 0x1000, PAGE_READWRITE, &old_prot), "VirtualProtect failed\n");
    ok(old_prot == PAGE_READONLY,
        "wrong old protection: got %04x instead of PAGE_READONLY\n", old_prot);

    ok(VirtualQuery(addr1, &info, sizeof(info)) == sizeof(info),
        "VirtualQuery failed\n");
    ok(info.RegionSize == 0x1000, "%lx != 0x1000\n", info.RegionSize);
    ok(info.State == MEM_COMMIT, "%x != MEM_COMMIT\n", info.State);
    ok(info.Protect == PAGE_READWRITE, "%x != PAGE_READWRITE\n", info.Protect);
    memset( addr1, 0x55, 20 );
    ok( *(DWORD *)addr1 == 0x55555555, "wrong data %x\n", *(DWORD *)addr1 );

    addr2 = VirtualAlloc( addr1, 0x1000, MEM_RESET, PAGE_NOACCESS );
    ok( addr2 == addr1 || broken( !addr2 && GetLastError() == ERROR_INVALID_PARAMETER), /* win9x */
        "VirtualAlloc failed err %u\n", GetLastError() );
    ok( *(DWORD *)addr1 == 0x55555555 || *(DWORD *)addr1 == 0, "wrong data %x\n", *(DWORD *)addr1 );
    if (addr2)
    {
        ok(VirtualQuery(addr1, &info, sizeof(info)) == sizeof(info),
           "VirtualQuery failed\n");
        ok(info.RegionSize == 0x1000, "%lx != 0x1000\n", info.RegionSize);
        ok(info.State == MEM_COMMIT, "%x != MEM_COMMIT\n", info.State);
        ok(info.Protect == PAGE_READWRITE, "%x != PAGE_READWRITE\n", info.Protect);

        addr2 = VirtualAlloc( (char *)addr1 + 0x1000, 0x1000, MEM_RESET, PAGE_NOACCESS );
        ok( (char *)addr2 == (char *)addr1 + 0x1000, "VirtualAlloc failed\n" );

        ok(VirtualQuery(addr2, &info, sizeof(info)) == sizeof(info),
           "VirtualQuery failed\n");
        ok(info.RegionSize == 0xf000, "%lx != 0xf000\n", info.RegionSize);
        ok(info.State == MEM_RESERVE, "%x != MEM_RESERVE\n", info.State);
        ok(info.Protect == 0, "%x != 0\n", info.Protect);

        addr2 = VirtualAlloc( (char *)addr1 + 0xf000, 0x2000, MEM_RESET, PAGE_NOACCESS );
        ok( !addr2, "VirtualAlloc failed\n" );
        ok( GetLastError() == ERROR_INVALID_ADDRESS, "wrong error %u\n", GetLastError() );
    }

    /* invalid protection values */
    SetLastError(0xdeadbeef);
    addr2 = VirtualAlloc(NULL, 0x1000, MEM_RESERVE, 0);
    ok(!addr2, "VirtualAlloc succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    addr2 = VirtualAlloc(NULL, 0x1000, MEM_COMMIT, 0);
    ok(!addr2, "VirtualAlloc succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    addr2 = VirtualAlloc(addr1, 0x1000, MEM_COMMIT, PAGE_READONLY | PAGE_EXECUTE);
    ok(!addr2, "VirtualAlloc succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ok(!VirtualProtect(addr1, 0x1000, PAGE_READWRITE | PAGE_EXECUTE_WRITECOPY, &old_prot),
       "VirtualProtect succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ok(!VirtualProtect(addr1, 0x1000, 0, &old_prot), "VirtualProtect succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ok(!VirtualFree(addr1, 0x10000, 0), "VirtualFree should fail with type 0\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
        "got %d, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    ok(VirtualFree(addr1, 0x10000, MEM_DECOMMIT), "VirtualFree failed\n");

    /* if the type is MEM_RELEASE, size must be 0 */
    ok(!VirtualFree(addr1, 1, MEM_RELEASE), "VirtualFree should fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
        "got %d, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    ok(VirtualFree(addr1, 0, MEM_RELEASE), "VirtualFree failed\n");
}

static void test_MapViewOfFile(void)
{
    static const char testfile[] = "testfile.xxx";
    const char *name;
    HANDLE file, mapping, map2;
    void *ptr, *ptr2, *addr;
    MEMORY_BASIC_INFORMATION info;
    BOOL ret;

    SetLastError(0xdeadbeef);
    file = CreateFileA( testfile, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "CreateFile error %u\n", GetLastError() );
    SetFilePointer( file, 4096, NULL, FILE_BEGIN );
    SetEndOfFile( file );

    /* read/write mapping */

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_READWRITE, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping error %u\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAPE_READ error %u\n", GetLastError() );
    UnmapViewOfFile( ptr );

    /* this fails on win9x but succeeds on NT */
    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_COPY, 0, 0, 4096 );
    if (ptr) UnmapViewOfFile( ptr );
    else ok( GetLastError() == ERROR_INVALID_PARAMETER, "Wrong error %d\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, 0, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile 0 error %u\n", GetLastError() );
    UnmapViewOfFile( ptr );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_WRITE error %u\n", GetLastError() );
    UnmapViewOfFile( ptr );

    ret = DuplicateHandle( GetCurrentProcess(), mapping, GetCurrentProcess(), &map2,
                           FILE_MAP_READ|FILE_MAP_WRITE, FALSE, 0 );
    ok( ret, "DuplicateHandle failed error %u\n", GetLastError());
    ptr = MapViewOfFile( map2, FILE_MAP_WRITE, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_WRITE error %u\n", GetLastError() );
    UnmapViewOfFile( ptr );
    CloseHandle( map2 );

    ret = DuplicateHandle( GetCurrentProcess(), mapping, GetCurrentProcess(), &map2,
                           FILE_MAP_READ, FALSE, 0 );
    ok( ret, "DuplicateHandle failed error %u\n", GetLastError());
    ptr = MapViewOfFile( map2, FILE_MAP_WRITE, 0, 0, 4096 );
    if (!ptr)
    {
        ok( GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %d\n", GetLastError() );
        CloseHandle( map2 );
        ret = DuplicateHandle( GetCurrentProcess(), mapping, GetCurrentProcess(), &map2, 0, FALSE, 0 );
        ok( ret, "DuplicateHandle failed error %u\n", GetLastError());
        ptr = MapViewOfFile( map2, 0, 0, 0, 4096 );
        ok( !ptr, "MapViewOfFile succeeded\n" );
        ok( GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %d\n", GetLastError() );
        CloseHandle( map2 );
        ret = DuplicateHandle( GetCurrentProcess(), mapping, GetCurrentProcess(), &map2,
                               FILE_MAP_READ, FALSE, 0 );
        ok( ret, "DuplicateHandle failed error %u\n", GetLastError());
        ptr = MapViewOfFile( map2, 0, 0, 0, 4096 );
        ok( ptr != NULL, "MapViewOfFile NO_ACCESS error %u\n", GetLastError() );
    }
    else win_skip( "no access checks on win9x\n" );

    UnmapViewOfFile( ptr );
    CloseHandle( map2 );
    CloseHandle( mapping );

    /* read-only mapping */

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping error %u\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ error %u\n", GetLastError() );
    UnmapViewOfFile( ptr );

    /* this fails on win9x but succeeds on NT */
    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_COPY, 0, 0, 4096 );
    if (ptr) UnmapViewOfFile( ptr );
    else ok( GetLastError() == ERROR_INVALID_PARAMETER, "Wrong error %d\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, 0, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile 0 error %u\n", GetLastError() );
    UnmapViewOfFile( ptr );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 4096 );
    ok( !ptr, "MapViewOfFile FILE_MAP_WRITE succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %d\n", GetLastError() );
    CloseHandle( mapping );

    /* copy-on-write mapping */

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_WRITECOPY, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping error %u\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ error %u\n", GetLastError() );
    UnmapViewOfFile( ptr );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_COPY, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_COPY error %u\n", GetLastError() );
    UnmapViewOfFile( ptr );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, 0, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile 0 error %u\n", GetLastError() );
    UnmapViewOfFile( ptr );

    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 4096 );
    ok( !ptr, "MapViewOfFile FILE_MAP_WRITE succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %d\n", GetLastError() );
    CloseHandle( mapping );

    /* no access mapping */

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_NOACCESS, 0, 4096, NULL );
    /* fails on NT but succeeds on win9x */
    if (!mapping) ok( GetLastError() == ERROR_INVALID_PARAMETER, "Wrong error %d\n", GetLastError() );
    else
    {
        SetLastError(0xdeadbeef);
        ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
        ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ error %u\n", GetLastError() );
        UnmapViewOfFile( ptr );

        SetLastError(0xdeadbeef);
        ptr = MapViewOfFile( mapping, FILE_MAP_COPY, 0, 0, 4096 );
        ok( !ptr, "MapViewOfFile FILE_MAP_COPY succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "Wrong error %d\n", GetLastError() );

        SetLastError(0xdeadbeef);
        ptr = MapViewOfFile( mapping, 0, 0, 0, 4096 );
        ok( ptr != NULL, "MapViewOfFile 0 error %u\n", GetLastError() );
        UnmapViewOfFile( ptr );

        SetLastError(0xdeadbeef);
        ptr = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 4096 );
        ok( !ptr, "MapViewOfFile FILE_MAP_WRITE succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "Wrong error %d\n", GetLastError() );

        CloseHandle( mapping );
    }

    CloseHandle( file );

    /* now try read-only file */

    SetLastError(0xdeadbeef);
    file = CreateFileA( testfile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "CreateFile error %u\n", GetLastError() );

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_READWRITE, 0, 4096, NULL );
    ok( !mapping, "CreateFileMapping PAGE_READWRITE succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %d\n", GetLastError() );

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_WRITECOPY, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping PAGE_WRITECOPY error %u\n", GetLastError() );
    CloseHandle( mapping );

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping PAGE_READONLY error %u\n", GetLastError() );
    CloseHandle( mapping );
    CloseHandle( file );

    /* now try no access file */

    SetLastError(0xdeadbeef);
    file = CreateFileA( testfile, 0, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "CreateFile error %u\n", GetLastError() );

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_READWRITE, 0, 4096, NULL );
    ok( !mapping, "CreateFileMapping PAGE_READWRITE succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %d\n", GetLastError() );

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_WRITECOPY, 0, 4096, NULL );
    ok( !mapping, "CreateFileMapping PAGE_WRITECOPY succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %d\n", GetLastError() );

    SetLastError(0xdeadbeef);
    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY, 0, 4096, NULL );
    ok( !mapping, "CreateFileMapping PAGE_READONLY succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %d\n", GetLastError() );

    CloseHandle( file );
    DeleteFileA( testfile );

    SetLastError(0xdeadbeef);
    name = "Local\\Foo";
    file = CreateFileMapping( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, name );
    /* nt4 doesn't have Local\\ */
    if (!file && GetLastError() == ERROR_PATH_NOT_FOUND)
    {
        name = "Foo";
        file = CreateFileMapping( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, name );
    }
    ok( file != 0, "CreateFileMapping PAGE_READWRITE error %u\n", GetLastError() );

    SetLastError(0xdeadbeef);
    mapping = OpenFileMapping( FILE_MAP_READ, FALSE, name );
    ok( mapping != 0, "OpenFileMapping FILE_MAP_READ error %u\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 0 );
    if (!ptr)
    {
        ok( GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %d\n", GetLastError() );
        SetLastError(0xdeadbeef);
        ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
        ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ error %u\n", GetLastError() );
        SetLastError(0xdeadbeef);
        ok( VirtualQuery( ptr, &info, sizeof(info) ) == sizeof(info),
            "VirtualQuery error %u\n", GetLastError() );
        ok( info.BaseAddress == ptr, "%p != %p\n", info.BaseAddress, ptr );
        ok( info.AllocationBase == ptr, "%p != %p\n", info.AllocationBase, ptr );
        ok( info.AllocationProtect == PAGE_READONLY, "%x != PAGE_READONLY\n", info.AllocationProtect );
        ok( info.RegionSize == 4096, "%lx != 4096\n", info.RegionSize );
        ok( info.State == MEM_COMMIT, "%x != MEM_COMMIT\n", info.State );
        ok( info.Protect == PAGE_READONLY, "%x != PAGE_READONLY\n", info.Protect );
    }
    else win_skip( "no access checks on win9x\n" );
    UnmapViewOfFile( ptr );
    CloseHandle( mapping );

    SetLastError(0xdeadbeef);
    mapping = OpenFileMapping( FILE_MAP_WRITE, FALSE, name );
    ok( mapping != 0, "OpenFileMapping FILE_MAP_WRITE error %u\n", GetLastError() );
    SetLastError(0xdeadbeef);
    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
    if (!ptr)
    {
        ok( GetLastError() == ERROR_ACCESS_DENIED, "Wrong error %d\n", GetLastError() );
        SetLastError(0xdeadbeef);
        ptr = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 0 );
        ok( ptr != NULL, "MapViewOfFile FILE_MAP_WRITE error %u\n", GetLastError() );
        SetLastError(0xdeadbeef);
        ok( VirtualQuery( ptr, &info, sizeof(info) ) == sizeof(info),
            "VirtualQuery error %u\n", GetLastError() );
        ok( info.BaseAddress == ptr, "%p != %p\n", info.BaseAddress, ptr );
        ok( info.AllocationBase == ptr, "%p != %p\n", info.AllocationBase, ptr );
        ok( info.AllocationProtect == PAGE_READWRITE, "%x != PAGE_READWRITE\n", info.AllocationProtect );
        ok( info.RegionSize == 4096, "%lx != 4096\n", info.RegionSize );
        ok( info.State == MEM_COMMIT, "%x != MEM_COMMIT\n", info.State );
        ok( info.Protect == PAGE_READWRITE, "%x != PAGE_READWRITE\n", info.Protect );
    }
    else win_skip( "no access checks on win9x\n" );
    UnmapViewOfFile( ptr );
    CloseHandle( mapping );

    CloseHandle( file );

    /* read/write mapping with SEC_RESERVE */
    mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_RESERVE, 0, MAPPING_SIZE, NULL);
    ok(mapping != INVALID_HANDLE_VALUE, "CreateFileMappingA failed with error %d\n", GetLastError());

    ptr = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, 0);
    ok(ptr != NULL, "MapViewOfFile failed with error %d\n", GetLastError());

    ptr2 = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, 0);
    /* on NT ptr != ptr2 but on Win9x ptr == ptr2 */
    ok(ptr2 != NULL, "MapViewOfFile failed with error %d\n", GetLastError());
    trace("mapping same section resulted in views %p and %p\n", ptr, ptr2);

    ret = VirtualQuery(ptr, &info, sizeof(info));
    ok(ret, "VirtualQuery failed with error %d\n", GetLastError());
    ok(info.BaseAddress == ptr, "BaseAddress should have been %p but was %p instead\n", ptr, info.BaseAddress);
    ok(info.AllocationBase == ptr, "AllocationBase should have been %p but was %p instead\n", ptr, info.AllocationBase);
    ok(info.RegionSize == MAPPING_SIZE, "RegionSize should have been 0x%x but was 0x%x\n", MAPPING_SIZE, (unsigned int)info.RegionSize);
    ok(info.State == MEM_RESERVE, "State should have been MEM_RESERVE instead of 0x%x\n", info.State);
    if (info.Type == MEM_PRIVATE)  /* win9x is different for uncommitted mappings */
    {
        ok(info.AllocationProtect == PAGE_NOACCESS,
           "AllocationProtect should have been PAGE_NOACCESS but was 0x%x\n", info.AllocationProtect);
        ok(info.Protect == PAGE_NOACCESS,
           "Protect should have been PAGE_NOACCESS instead of 0x%x\n", info.Protect);
    }
    else
    {
        ok(info.AllocationProtect == PAGE_READWRITE,
           "AllocationProtect should have been PAGE_READWRITE but was 0x%x\n", info.AllocationProtect);
        ok(info.Protect == 0, "Protect should have been 0 instead of 0x%x\n", info.Protect);
        ok(info.Type == MEM_MAPPED, "Type should have been MEM_MAPPED instead of 0x%x\n", info.Type);
    }

    if (ptr != ptr2)
    {
        ret = VirtualQuery(ptr2, &info, sizeof(info));
        ok(ret, "VirtualQuery failed with error %d\n", GetLastError());
        ok(info.BaseAddress == ptr2,
           "BaseAddress should have been %p but was %p instead\n", ptr2, info.BaseAddress);
        ok(info.AllocationBase == ptr2,
           "AllocationBase should have been %p but was %p instead\n", ptr2, info.AllocationBase);
        ok(info.AllocationProtect == PAGE_READWRITE,
           "AllocationProtect should have been PAGE_READWRITE but was 0x%x\n", info.AllocationProtect);
        ok(info.RegionSize == MAPPING_SIZE,
           "RegionSize should have been 0x%x but was 0x%x\n", MAPPING_SIZE, (unsigned int)info.RegionSize);
        ok(info.State == MEM_RESERVE,
           "State should have been MEM_RESERVE instead of 0x%x\n", info.State);
        ok(info.Protect == 0,
           "Protect should have been 0 instead of 0x%x\n", info.Protect);
        ok(info.Type == MEM_MAPPED,
           "Type should have been MEM_MAPPED instead of 0x%x\n", info.Type);
    }

    ptr = VirtualAlloc(ptr, 0x10000, MEM_COMMIT, PAGE_READONLY);
    ok(ptr != NULL, "VirtualAlloc failed with error %d\n", GetLastError());

    ret = VirtualQuery(ptr, &info, sizeof(info));
    ok(ret, "VirtualQuery failed with error %d\n", GetLastError());
    ok(info.BaseAddress == ptr, "BaseAddress should have been %p but was %p instead\n", ptr, info.BaseAddress);
    ok(info.AllocationBase == ptr, "AllocationBase should have been %p but was %p instead\n", ptr, info.AllocationBase);
    ok(info.RegionSize == 0x10000, "RegionSize should have been 0x10000 but was 0x%x\n", (unsigned int)info.RegionSize);
    ok(info.State == MEM_COMMIT, "State should have been MEM_RESERVE instead of 0x%x\n", info.State);
    ok(info.Protect == PAGE_READONLY, "Protect should have been 0 instead of 0x%x\n", info.Protect);
    if (info.Type == MEM_PRIVATE)  /* win9x is different for uncommitted mappings */
    {
        ok(info.AllocationProtect == PAGE_NOACCESS,
           "AllocationProtect should have been PAGE_NOACCESS but was 0x%x\n", info.AllocationProtect);
    }
    else
    {
        ok(info.AllocationProtect == PAGE_READWRITE,
           "AllocationProtect should have been PAGE_READWRITE but was 0x%x\n", info.AllocationProtect);
        ok(info.Type == MEM_MAPPED, "Type should have been MEM_MAPPED instead of 0x%x\n", info.Type);
    }

    /* shows that the VirtualAlloc above affects the mapping, not just the
     * virtual memory in this process - it also affects all other processes
     * with a view of the mapping, but that isn't tested here */
    if (ptr != ptr2)
    {
        ret = VirtualQuery(ptr2, &info, sizeof(info));
        ok(ret, "VirtualQuery failed with error %d\n", GetLastError());
        ok(info.BaseAddress == ptr2,
           "BaseAddress should have been %p but was %p instead\n", ptr2, info.BaseAddress);
        ok(info.AllocationBase == ptr2,
           "AllocationBase should have been %p but was %p instead\n", ptr2, info.AllocationBase);
        ok(info.AllocationProtect == PAGE_READWRITE,
           "AllocationProtect should have been PAGE_READWRITE but was 0x%x\n", info.AllocationProtect);
        ok(info.RegionSize == 0x10000,
           "RegionSize should have been 0x10000 but was 0x%x\n", (unsigned int)info.RegionSize);
        ok(info.State == MEM_COMMIT,
           "State should have been MEM_RESERVE instead of 0x%x\n", info.State);
        ok(info.Protect == PAGE_READWRITE,
           "Protect should have been 0 instead of 0x%x\n", info.Protect);
        ok(info.Type == MEM_MAPPED, "Type should have been MEM_MAPPED instead of 0x%x\n", info.Type);
    }

    addr = VirtualAlloc( ptr, MAPPING_SIZE, MEM_RESET, PAGE_READONLY );
    ok( addr == ptr || broken(!addr && GetLastError() == ERROR_INVALID_PARAMETER), /* win9x */
        "VirtualAlloc failed with error %u\n", GetLastError() );

    ret = VirtualFree( ptr, 0x10000, MEM_DECOMMIT );
    ok( !ret || broken(ret) /* win9x */, "VirtualFree succeeded\n" );
    if (!ret)
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "VirtualFree failed with %u\n", GetLastError() );

    ret = UnmapViewOfFile(ptr2);
    ok(ret, "UnmapViewOfFile failed with error %d\n", GetLastError());
    ret = UnmapViewOfFile(ptr);
    ok(ret, "UnmapViewOfFile failed with error %d\n", GetLastError());
    CloseHandle(mapping);

    addr = VirtualAlloc(NULL, 0x10000, MEM_COMMIT, PAGE_READONLY );
    ok( addr != NULL, "VirtualAlloc failed with error %u\n", GetLastError() );

    SetLastError(0xdeadbeef);
    ok( !UnmapViewOfFile(addr), "UnmapViewOfFile should fail on VirtualAlloc mem\n" );
    ok( GetLastError() == ERROR_INVALID_ADDRESS,
        "got %u, expected ERROR_INVALID_ADDRESS\n", GetLastError());
    SetLastError(0xdeadbeef);
    ok( !UnmapViewOfFile((char *)addr + 0x3000), "UnmapViewOfFile should fail on VirtualAlloc mem\n" );
    ok( GetLastError() == ERROR_INVALID_ADDRESS,
        "got %u, expected ERROR_INVALID_ADDRESS\n", GetLastError());
    SetLastError(0xdeadbeef);
    ok( !UnmapViewOfFile((void *)0xdeadbeef), "UnmapViewOfFile should fail on VirtualAlloc mem\n" );
    ok( GetLastError() == ERROR_INVALID_ADDRESS,
       "got %u, expected ERROR_INVALID_ADDRESS\n", GetLastError());

    ok( VirtualFree(addr, 0, MEM_RELEASE), "VirtualFree failed\n" );
}

static DWORD (WINAPI *pNtMapViewOfSection)( HANDLE handle, HANDLE process, PVOID *addr_ptr,
                                            ULONG zero_bits, SIZE_T commit_size,
                                            const LARGE_INTEGER *offset_ptr, SIZE_T *size_ptr,
                                            ULONG inherit, ULONG alloc_type, ULONG protect );
static DWORD (WINAPI *pNtUnmapViewOfSection)( HANDLE process, PVOID addr );

static void test_NtMapViewOfSection(void)
{
    HANDLE hProcess;

    static const char testfile[] = "testfile.xxx";
    static const char data[] = "test data for NtMapViewOfSection";
    char buffer[sizeof(data)];
    HANDLE file, mapping;
    void *ptr;
    BOOL ret;
    DWORD status, written;
    SIZE_T size, result;
    LARGE_INTEGER offset;

    pNtMapViewOfSection = (void *)GetProcAddress( GetModuleHandle("ntdll.dll"), "NtMapViewOfSection" );
    pNtUnmapViewOfSection = (void *)GetProcAddress( GetModuleHandle("ntdll.dll"), "NtUnmapViewOfSection" );
    if (!pNtMapViewOfSection || !pNtUnmapViewOfSection)
    {
        win_skip( "NtMapViewOfSection not available\n" );
        return;
    }

    file = CreateFileA( testfile, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "Failed to create test file\n" );
    WriteFile( file, data, sizeof(data), &written, NULL );
    SetFilePointer( file, 4096, NULL, FILE_BEGIN );
    SetEndOfFile( file );

    /* read/write mapping */

    mapping = CreateFileMappingA( file, NULL, PAGE_READWRITE, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping failed\n" );

    hProcess = create_target_process("sleep");
    ok(hProcess != NULL, "Can't start process\n");

    ptr = NULL;
    size = 0;
    offset.QuadPart = 0;
    status = pNtMapViewOfSection( mapping, hProcess, &ptr, 0, 0, &offset, &size, 1, 0, PAGE_READWRITE );
    ok( !status, "NtMapViewOfSection failed status %x\n", status );

    ret = ReadProcessMemory( hProcess, ptr, buffer, sizeof(buffer), &result );
    ok( ret, "ReadProcessMemory failed\n" );
    ok( result == sizeof(buffer), "ReadProcessMemory didn't read all data (%lx)\n", result );
    ok( !memcmp( buffer, data, sizeof(buffer) ), "Wrong data read\n" );

    status = pNtUnmapViewOfSection( hProcess, ptr );
    ok( !status, "NtUnmapViewOfSection failed status %x\n", status );

    CloseHandle( mapping );
    CloseHandle( file );
    DeleteFileA( testfile );

    TerminateProcess(hProcess, 0);
    CloseHandle(hProcess);
}

static void test_NtAreMappedFilesTheSame(void)
{
    static const char testfile[] = "testfile.xxx";
    HANDLE file, file2, mapping, map2;
    void *ptr, *ptr2;
    NTSTATUS status;
    char path[MAX_PATH];

    if (!pNtAreMappedFilesTheSame)
    {
        win_skip( "NtAreMappedFilesTheSame not available\n" );
        return;
    }

    file = CreateFileA( testfile, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
                        NULL, CREATE_ALWAYS, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "CreateFile error %u\n", GetLastError() );
    SetFilePointer( file, 4096, NULL, FILE_BEGIN );
    SetEndOfFile( file );

    mapping = CreateFileMappingA( file, NULL, PAGE_READWRITE, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping error %u\n", GetLastError() );

    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ error %u\n", GetLastError() );

    file2 = CreateFileA( testfile, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
                         NULL, OPEN_EXISTING, 0, 0 );
    ok( file2 != INVALID_HANDLE_VALUE, "CreateFile error %u\n", GetLastError() );

    map2 = CreateFileMappingA( file2, NULL, PAGE_READONLY, 0, 4096, NULL );
    ok( map2 != 0, "CreateFileMapping error %u\n", GetLastError() );
    ptr2 = MapViewOfFile( map2, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr2 != NULL, "MapViewOfFile FILE_MAP_READ error %u\n", GetLastError() );
    status = pNtAreMappedFilesTheSame( ptr, ptr2 );
    ok( status == STATUS_NOT_SAME_DEVICE, "NtAreMappedFilesTheSame returned %x\n", status );
    UnmapViewOfFile( ptr2 );

    ptr2 = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr2 != NULL, "MapViewOfFile FILE_MAP_READ error %u\n", GetLastError() );
    status = pNtAreMappedFilesTheSame( ptr, ptr2 );
    ok( status == STATUS_NOT_SAME_DEVICE, "NtAreMappedFilesTheSame returned %x\n", status );
    UnmapViewOfFile( ptr2 );
    CloseHandle( map2 );

    map2 = CreateFileMappingA( file, NULL, PAGE_READONLY, 0, 4096, NULL );
    ok( map2 != 0, "CreateFileMapping error %u\n", GetLastError() );
    ptr2 = MapViewOfFile( map2, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr2 != NULL, "MapViewOfFile FILE_MAP_READ error %u\n", GetLastError() );
    status = pNtAreMappedFilesTheSame( ptr, ptr2 );
    ok( status == STATUS_NOT_SAME_DEVICE, "NtAreMappedFilesTheSame returned %x\n", status );
    UnmapViewOfFile( ptr2 );
    CloseHandle( map2 );
    CloseHandle( file2 );

    status = pNtAreMappedFilesTheSame( ptr, ptr );
    ok( status == STATUS_NOT_SAME_DEVICE, "NtAreMappedFilesTheSame returned %x\n", status );

    status = pNtAreMappedFilesTheSame( ptr, (char *)ptr + 30 );
    ok( status == STATUS_NOT_SAME_DEVICE, "NtAreMappedFilesTheSame returned %x\n", status );

    status = pNtAreMappedFilesTheSame( ptr, GetModuleHandleA("kernel32.dll") );
    ok( status == STATUS_NOT_SAME_DEVICE, "NtAreMappedFilesTheSame returned %x\n", status );

    status = pNtAreMappedFilesTheSame( ptr, (void *)0xdeadbeef );
    ok( status == STATUS_CONFLICTING_ADDRESSES || status == STATUS_INVALID_ADDRESS,
        "NtAreMappedFilesTheSame returned %x\n", status );

    status = pNtAreMappedFilesTheSame( ptr, NULL );
    ok( status == STATUS_INVALID_ADDRESS, "NtAreMappedFilesTheSame returned %x\n", status );

    status = pNtAreMappedFilesTheSame( ptr, (void *)GetProcessHeap() );
    ok( status == STATUS_CONFLICTING_ADDRESSES, "NtAreMappedFilesTheSame returned %x\n", status );

    status = pNtAreMappedFilesTheSame( NULL, NULL );
    ok( status == STATUS_INVALID_ADDRESS, "NtAreMappedFilesTheSame returned %x\n", status );

    ptr2 = VirtualAlloc( NULL, 0x10000, MEM_COMMIT, PAGE_READWRITE );
    ok( ptr2 != NULL, "VirtualAlloc error %u\n", GetLastError() );
    status = pNtAreMappedFilesTheSame( ptr, ptr2 );
    ok( status == STATUS_CONFLICTING_ADDRESSES, "NtAreMappedFilesTheSame returned %x\n", status );
    VirtualFree( ptr2, 0, MEM_RELEASE );

    UnmapViewOfFile( ptr );
    CloseHandle( mapping );
    CloseHandle( file );

    status = pNtAreMappedFilesTheSame( GetModuleHandleA("ntdll.dll"),
                                       GetModuleHandleA("kernel32.dll") );
    ok( status == STATUS_NOT_SAME_DEVICE, "NtAreMappedFilesTheSame returned %x\n", status );
    status = pNtAreMappedFilesTheSame( GetModuleHandleA("kernel32.dll"),
                                       GetModuleHandleA("kernel32.dll") );
    ok( status == STATUS_SUCCESS, "NtAreMappedFilesTheSame returned %x\n", status );
    status = pNtAreMappedFilesTheSame( GetModuleHandleA("kernel32.dll"),
                                       (char *)GetModuleHandleA("kernel32.dll") + 4096 );
    ok( status == STATUS_SUCCESS, "NtAreMappedFilesTheSame returned %x\n", status );

    GetSystemDirectoryA( path, MAX_PATH );
    strcat( path, "\\kernel32.dll" );
    file = CreateFileA( path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "CreateFile error %u\n", GetLastError() );

    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY, 0, 4096, NULL );
    ok( mapping != 0, "CreateFileMapping error %u\n", GetLastError() );
    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 4096 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ error %u\n", GetLastError() );
    status = pNtAreMappedFilesTheSame( ptr, GetModuleHandleA("kernel32.dll") );
    ok( status == STATUS_NOT_SAME_DEVICE, "NtAreMappedFilesTheSame returned %x\n", status );
    UnmapViewOfFile( ptr );
    CloseHandle( mapping );

    mapping = CreateFileMappingA( file, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, NULL );
    ok( mapping != 0, "CreateFileMapping error %u\n", GetLastError() );
    ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
    ok( ptr != NULL, "MapViewOfFile FILE_MAP_READ error %u\n", GetLastError() );
    status = pNtAreMappedFilesTheSame( ptr, GetModuleHandleA("kernel32.dll") );
    todo_wine
    ok( status == STATUS_SUCCESS, "NtAreMappedFilesTheSame returned %x\n", status );

    file2 = CreateFileA( path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
    ok( file2 != INVALID_HANDLE_VALUE, "CreateFile error %u\n", GetLastError() );
    map2 = CreateFileMappingA( file2, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, NULL );
    ok( map2 != 0, "CreateFileMapping error %u\n", GetLastError() );
    ptr2 = MapViewOfFile( map2, FILE_MAP_READ, 0, 0, 0 );
    ok( ptr2 != NULL, "MapViewOfFile FILE_MAP_READ error %u\n", GetLastError() );
    status = pNtAreMappedFilesTheSame( ptr, ptr2 );
    ok( status == STATUS_SUCCESS, "NtAreMappedFilesTheSame returned %x\n", status );
    UnmapViewOfFile( ptr2 );
    CloseHandle( map2 );
    CloseHandle( file2 );

    UnmapViewOfFile( ptr );
    CloseHandle( mapping );

    CloseHandle( file );
    DeleteFileA( testfile );
}

static void test_CreateFileMapping(void)
{
    HANDLE handle, handle2;

    /* test case sensitivity */

    SetLastError(0xdeadbeef);
    handle = CreateFileMappingA( INVALID_HANDLE_VALUE, NULL, SEC_COMMIT | PAGE_READWRITE, 0, 0x1000,
                                 "Wine Test Mapping");
    ok( handle != NULL, "CreateFileMapping failed with error %u\n", GetLastError());
    ok( GetLastError() == 0, "wrong error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    handle2 = CreateFileMappingA( INVALID_HANDLE_VALUE, NULL, SEC_COMMIT | PAGE_READWRITE, 0, 0x1000,
                                  "Wine Test Mapping");
    ok( handle2 != NULL, "CreateFileMapping failed with error %d\n", GetLastError());
    ok( GetLastError() == ERROR_ALREADY_EXISTS, "wrong error %u\n", GetLastError());
    CloseHandle( handle2 );

    SetLastError(0xdeadbeef);
    handle2 = CreateFileMappingA( INVALID_HANDLE_VALUE, NULL, SEC_COMMIT | PAGE_READWRITE, 0, 0x1000,
                                 "WINE TEST MAPPING");
    ok( handle2 != NULL, "CreateFileMapping failed with error %d\n", GetLastError());
    ok( GetLastError() == 0, "wrong error %u\n", GetLastError());
    CloseHandle( handle2 );

    SetLastError(0xdeadbeef);
    handle2 = OpenFileMappingA( FILE_MAP_ALL_ACCESS, FALSE, "Wine Test Mapping");
    ok( handle2 != NULL, "OpenFileMapping failed with error %d\n", GetLastError());
    CloseHandle( handle2 );

    SetLastError(0xdeadbeef);
    handle2 = OpenFileMappingA( FILE_MAP_ALL_ACCESS, FALSE, "WINE TEST MAPPING");
    ok( !handle2, "OpenFileMapping succeeded\n");
    ok( GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_INVALID_NAME /* win9x */,
        "wrong error %u\n", GetLastError());

    CloseHandle( handle );
}

static void test_IsBadReadPtr(void)
{
    BOOL ret;
    void *ptr = (void *)0xdeadbeef;
    char stackvar;

    ret = IsBadReadPtr(NULL, 0);
    ok(ret == FALSE, "Expected IsBadReadPtr to return FALSE, got %d\n", ret);

    ret = IsBadReadPtr(NULL, 1);
    ok(ret == TRUE, "Expected IsBadReadPtr to return TRUE, got %d\n", ret);

    ret = IsBadReadPtr(ptr, 0);
    ok(ret == FALSE, "Expected IsBadReadPtr to return FALSE, got %d\n", ret);

    ret = IsBadReadPtr(ptr, 1);
    ok(ret == TRUE, "Expected IsBadReadPtr to return TRUE, got %d\n", ret);

    ret = IsBadReadPtr(&stackvar, 0);
    ok(ret == FALSE, "Expected IsBadReadPtr to return FALSE, got %d\n", ret);

    ret = IsBadReadPtr(&stackvar, sizeof(char));
    ok(ret == FALSE, "Expected IsBadReadPtr to return FALSE, got %d\n", ret);
}

static void test_IsBadWritePtr(void)
{
    BOOL ret;
    void *ptr = (void *)0xdeadbeef;
    char stackval;

    ret = IsBadWritePtr(NULL, 0);
    ok(ret == FALSE, "Expected IsBadWritePtr to return FALSE, got %d\n", ret);

    ret = IsBadWritePtr(NULL, 1);
    ok(ret == TRUE, "Expected IsBadWritePtr to return TRUE, got %d\n", ret);

    ret = IsBadWritePtr(ptr, 0);
    ok(ret == FALSE, "Expected IsBadWritePtr to return FALSE, got %d\n", ret);

    ret = IsBadWritePtr(ptr, 1);
    ok(ret == TRUE, "Expected IsBadWritePtr to return TRUE, got %d\n", ret);

    ret = IsBadWritePtr(&stackval, 0);
    ok(ret == FALSE, "Expected IsBadWritePtr to return FALSE, got %d\n", ret);

    ret = IsBadWritePtr(&stackval, sizeof(char));
    ok(ret == FALSE, "Expected IsBadWritePtr to return FALSE, got %d\n", ret);
}

static void test_IsBadCodePtr(void)
{
    BOOL ret;
    void *ptr = (void *)0xdeadbeef;
    char stackval;

    ret = IsBadCodePtr(NULL);
    ok(ret == TRUE, "Expected IsBadCodePtr to return TRUE, got %d\n", ret);

    ret = IsBadCodePtr(ptr);
    ok(ret == TRUE, "Expected IsBadCodePtr to return TRUE, got %d\n", ret);

    ret = IsBadCodePtr((void *)&stackval);
    ok(ret == FALSE, "Expected IsBadCodePtr to return FALSE, got %d\n", ret);
}

static void test_write_watch(void)
{
    char *base;
    DWORD ret, size, old_prot;
    MEMORY_BASIC_INFORMATION info;
    void *results[64];
    ULONG_PTR count;
    ULONG pagesize;

    if (!pGetWriteWatch || !pResetWriteWatch)
    {
        win_skip( "GetWriteWatch not supported\n" );
        return;
    }

    size = 0x10000;
    base = VirtualAlloc( 0, size, MEM_RESERVE | MEM_COMMIT | MEM_WRITE_WATCH, PAGE_READWRITE );
    if (!base &&
        (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_NOT_SUPPORTED))
    {
        win_skip( "MEM_WRITE_WATCH not supported\n" );
        return;
    }
    ok( base != NULL, "VirtualAlloc failed %u\n", GetLastError() );
    ret = VirtualQuery( base, &info, sizeof(info) );
    ok(ret, "VirtualQuery failed %u\n", GetLastError());
    ok( info.BaseAddress == base, "BaseAddress %p instead of %p\n", info.BaseAddress, base );
    ok( info.AllocationProtect == PAGE_READWRITE, "wrong AllocationProtect %x\n", info.AllocationProtect );
    ok( info.RegionSize == size, "wrong RegionSize 0x%lx\n", info.RegionSize );
    ok( info.State == MEM_COMMIT, "wrong State 0x%x\n", info.State );
    ok( info.Protect == PAGE_READWRITE, "wrong Protect 0x%x\n", info.Protect );
    ok( info.Type == MEM_PRIVATE, "wrong Type 0x%x\n", info.Type );

    count = 64;
    SetLastError( 0xdeadbeef );
    ret = pGetWriteWatch( 0, NULL, size, results, &count, &pagesize );
    ok( ret == ~0u, "GetWriteWatch succeeded %u\n", ret );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        broken( GetLastError() == 0xdeadbeef ), /* win98 */
        "wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = pGetWriteWatch( 0, GetModuleHandle(0), size, results, &count, &pagesize );
    if (ret)
    {
        ok( ret == ~0u, "GetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    }
    else  /* win98 */
    {
        ok( count == 0, "wrong count %lu\n", count );
    }

    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 0, "wrong count %lu\n", count );

    base[pagesize + 1] = 0x44;

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 1, "wrong count %lu\n", count );
    ok( results[0] == base + pagesize, "wrong result %p\n", results[0] );

    count = 64;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 1, "wrong count %lu\n", count );
    ok( results[0] == base + pagesize, "wrong result %p\n", results[0] );

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 0, "wrong count %lu\n", count );

    base[2*pagesize + 3] = 0x11;
    base[4*pagesize + 8] = 0x11;

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 2, "wrong count %lu\n", count );
    ok( results[0] == base + 2*pagesize, "wrong result %p\n", results[0] );
    ok( results[1] == base + 4*pagesize, "wrong result %p\n", results[1] );

    count = 64;
    ret = pGetWriteWatch( 0, base + 3*pagesize, 2*pagesize, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 1, "wrong count %lu\n", count );
    ok( results[0] == base + 4*pagesize, "wrong result %p\n", results[0] );

    ret = pResetWriteWatch( base, 3*pagesize );
    ok( !ret, "pResetWriteWatch failed %u\n", GetLastError() );

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 1, "wrong count %lu\n", count );
    ok( results[0] == base + 4*pagesize, "wrong result %p\n", results[0] );

    *(DWORD *)(base + 2*pagesize - 2) = 0xdeadbeef;

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 3, "wrong count %lu\n", count );
    ok( results[0] == base + pagesize, "wrong result %p\n", results[0] );
    ok( results[1] == base + 2*pagesize, "wrong result %p\n", results[1] );
    ok( results[2] == base + 4*pagesize, "wrong result %p\n", results[2] );

    count = 1;
    ret = pGetWriteWatch( WRITE_WATCH_FLAG_RESET, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 1, "wrong count %lu\n", count );
    ok( results[0] == base + pagesize, "wrong result %p\n", results[0] );

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 2, "wrong count %lu\n", count );
    ok( results[0] == base + 2*pagesize, "wrong result %p\n", results[0] );
    ok( results[1] == base + 4*pagesize, "wrong result %p\n", results[1] );

    /* changing protections doesn't affect watches */

    ret = VirtualProtect( base, 3*pagesize, PAGE_READONLY, &old_prot );
    ok( ret, "VirtualProtect failed error %u\n", GetLastError() );
    ok( old_prot == PAGE_READWRITE, "wrong old prot %x\n", old_prot );

    ret = VirtualQuery( base, &info, sizeof(info) );
    ok(ret, "VirtualQuery failed %u\n", GetLastError());
    ok( info.BaseAddress == base, "BaseAddress %p instead of %p\n", info.BaseAddress, base );
    ok( info.RegionSize == 3*pagesize, "wrong RegionSize 0x%lx\n", info.RegionSize );
    ok( info.State == MEM_COMMIT, "wrong State 0x%x\n", info.State );
    ok( info.Protect == PAGE_READONLY, "wrong Protect 0x%x\n", info.Protect );

    ret = VirtualProtect( base, 3*pagesize, PAGE_READWRITE, &old_prot );
    ok( ret, "VirtualProtect failed error %u\n", GetLastError() );
    ok( old_prot == PAGE_READONLY, "wrong old prot %x\n", old_prot );

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 2, "wrong count %lu\n", count );
    ok( results[0] == base + 2*pagesize, "wrong result %p\n", results[0] );
    ok( results[1] == base + 4*pagesize, "wrong result %p\n", results[1] );

    ret = VirtualQuery( base, &info, sizeof(info) );
    ok(ret, "VirtualQuery failed %u\n", GetLastError());
    ok( info.BaseAddress == base, "BaseAddress %p instead of %p\n", info.BaseAddress, base );
    ok( info.RegionSize == size, "wrong RegionSize 0x%lx\n", info.RegionSize );
    ok( info.State == MEM_COMMIT, "wrong State 0x%x\n", info.State );
    ok( info.Protect == PAGE_READWRITE, "wrong Protect 0x%x\n", info.Protect );

    /* some invalid parameter tests */

    SetLastError( 0xdeadbeef );
    count = 0;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    if (ret)
    {
        ok( ret == ~0u, "GetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        ret = pGetWriteWatch( 0, base, size, results, NULL, &pagesize );
        ok( ret == ~0u, "GetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == ERROR_NOACCESS, "wrong error %u\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        count = 64;
        ret = pGetWriteWatch( 0, base, size, results, &count, NULL );
        ok( ret == ~0u, "GetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == ERROR_NOACCESS, "wrong error %u\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        count = 64;
        ret = pGetWriteWatch( 0, base, size, NULL, &count, &pagesize );
        ok( ret == ~0u, "GetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == ERROR_NOACCESS, "wrong error %u\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        count = 0;
        ret = pGetWriteWatch( 0, base, size, NULL, &count, &pagesize );
        ok( ret == ~0u, "GetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        count = 64;
        ret = pGetWriteWatch( 0xdeadbeef, base, size, results, &count, &pagesize );
        ok( ret == ~0u, "GetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        count = 64;
        ret = pGetWriteWatch( 0, base, 0, results, &count, &pagesize );
        ok( ret == ~0u, "GetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        count = 64;
        ret = pGetWriteWatch( 0, base, size * 2, results, &count, &pagesize );
        ok( ret == ~0u, "GetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        count = 64;
        ret = pGetWriteWatch( 0, base + size - pagesize, pagesize + 1, results, &count, &pagesize );
        ok( ret == ~0u, "GetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        ret = pResetWriteWatch( base, 0 );
        ok( ret == ~0u, "ResetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        ret = pResetWriteWatch( GetModuleHandle(0), size );
        ok( ret == ~0u, "ResetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );
    }
    else  /* win98 is completely different */
    {
        SetLastError( 0xdeadbeef );
        count = 64;
        ret = pGetWriteWatch( 0, base, size, NULL, &count, &pagesize );
        ok( ret == ERROR_INVALID_PARAMETER, "GetWriteWatch succeeded %u\n", ret );
        ok( GetLastError() == 0xdeadbeef, "wrong error %u\n", GetLastError() );

        count = 0;
        ret = pGetWriteWatch( 0, base, size, NULL, &count, &pagesize );
        ok( !ret, "GetWriteWatch failed %u\n", ret );

        count = 64;
        ret = pGetWriteWatch( 0xdeadbeef, base, size, results, &count, &pagesize );
        ok( !ret, "GetWriteWatch failed %u\n", ret );

        count = 64;
        ret = pGetWriteWatch( 0, base, 0, results, &count, &pagesize );
        ok( !ret, "GetWriteWatch failed %u\n", ret );

        ret = pResetWriteWatch( base, 0 );
        ok( !ret, "ResetWriteWatch failed %u\n", ret );

        ret = pResetWriteWatch( GetModuleHandle(0), size );
        ok( !ret, "ResetWriteWatch failed %u\n", ret );
    }

    VirtualFree( base, 0, MEM_FREE );

    base = VirtualAlloc( 0, size, MEM_RESERVE | MEM_WRITE_WATCH, PAGE_READWRITE );
    ok( base != NULL, "VirtualAlloc failed %u\n", GetLastError() );
    VirtualFree( base, 0, MEM_FREE );

    base = VirtualAlloc( 0, size, MEM_WRITE_WATCH, PAGE_READWRITE );
    ok( !base, "VirtualAlloc succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %u\n", GetLastError() );

    /* initial protect doesn't matter */

    base = VirtualAlloc( 0, size, MEM_RESERVE | MEM_WRITE_WATCH, PAGE_NOACCESS );
    ok( base != NULL, "VirtualAlloc failed %u\n", GetLastError() );
    base = VirtualAlloc( base, size, MEM_COMMIT, PAGE_NOACCESS );
    ok( base != NULL, "VirtualAlloc failed %u\n", GetLastError() );

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 0, "wrong count %lu\n", count );

    ret = VirtualProtect( base, 6*pagesize, PAGE_READWRITE, &old_prot );
    ok( ret, "VirtualProtect failed error %u\n", GetLastError() );
    ok( old_prot == PAGE_NOACCESS, "wrong old prot %x\n", old_prot );

    base[5*pagesize + 200] = 3;

    ret = VirtualProtect( base, 6*pagesize, PAGE_NOACCESS, &old_prot );
    ok( ret, "VirtualProtect failed error %u\n", GetLastError() );
    ok( old_prot == PAGE_READWRITE, "wrong old prot %x\n", old_prot );

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 1, "wrong count %lu\n", count );
    ok( results[0] == base + 5*pagesize, "wrong result %p\n", results[0] );

    ret = VirtualFree( base, size, MEM_DECOMMIT );
    ok( ret, "VirtualFree failed %u\n", GetLastError() );

    count = 64;
    ret = pGetWriteWatch( 0, base, size, results, &count, &pagesize );
    ok( !ret, "GetWriteWatch failed %u\n", GetLastError() );
    ok( count == 1 || broken(count == 0), /* win98 */
        "wrong count %lu\n", count );
    if (count) ok( results[0] == base + 5*pagesize, "wrong result %p\n", results[0] );

    VirtualFree( base, 0, MEM_FREE );
}

START_TEST(virtual)
{
    int argc;
    char **argv;
    argc = winetest_get_mainargs( &argv );

    if (argc >= 3)
    {
        if (!strcmp(argv[2], "sleep"))
        {
            Sleep(5000); /* spawned process runs for at most 5 seconds */
            return;
        }
        while (1)
        {
            void *mem;
            BOOL ret;
            mem = VirtualAlloc(NULL, 1<<20, MEM_COMMIT|MEM_RESERVE,
                               PAGE_EXECUTE_READWRITE);
            ok(mem != NULL, "VirtualAlloc failed %u\n", GetLastError());
            if (mem == NULL) break;
            ret = VirtualFree(mem, 0, MEM_RELEASE);
            ok(ret, "VirtualFree failed %u\n", GetLastError());
            if (!ret) break;
        }
        return;
    }

    hkernel32 = GetModuleHandleA("kernel32.dll");
    pVirtualAllocEx = (void *) GetProcAddress(hkernel32, "VirtualAllocEx");
    pVirtualFreeEx = (void *) GetProcAddress(hkernel32, "VirtualFreeEx");
    pGetWriteWatch = (void *) GetProcAddress(hkernel32, "GetWriteWatch");
    pResetWriteWatch = (void *) GetProcAddress(hkernel32, "ResetWriteWatch");
    pNtAreMappedFilesTheSame = (void *)GetProcAddress( GetModuleHandle("ntdll.dll"),
                                                       "NtAreMappedFilesTheSame" );

    test_VirtualAllocEx();
    test_VirtualAlloc();
    test_MapViewOfFile();
    test_NtMapViewOfSection();
    test_NtAreMappedFilesTheSame();
    test_CreateFileMapping();
    test_IsBadReadPtr();
    test_IsBadWritePtr();
    test_IsBadCodePtr();
    test_write_watch();
}

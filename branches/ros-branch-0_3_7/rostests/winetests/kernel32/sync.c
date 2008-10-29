/*
 * Synchronization tests
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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <windef.h>
#include <winbase.h>

#include "wine/test.h"

static void test_signalandwait(void)
{
    DWORD (WINAPI *pSignalObjectAndWait)(HANDLE, HANDLE, DWORD, BOOL);
    HMODULE kernel32;
    DWORD r;
    int i;
    HANDLE event[2], maxevents[MAXIMUM_WAIT_OBJECTS], semaphore[2], file;

    kernel32 = GetModuleHandle("kernel32");
    pSignalObjectAndWait = (void*) GetProcAddress(kernel32, "SignalObjectAndWait");

    if (!pSignalObjectAndWait)
        return;

    /* invalid parameters */
    r = pSignalObjectAndWait(NULL, NULL, 0, 0);
    if (r == ERROR_INVALID_FUNCTION)
    {
        trace("SignalObjectAndWait not implemented, skipping tests\n");
        return; /* Win98/ME */
    }
    ok( r == WAIT_FAILED, "should fail\n");

    event[0] = CreateEvent(NULL, 0, 0, NULL);
    event[1] = CreateEvent(NULL, 1, 1, NULL);

    ok( event[0] && event[1], "failed to create event flags\n");

    r = pSignalObjectAndWait(event[0], NULL, 0, FALSE);
    ok( r == WAIT_FAILED, "should fail\n");

    r = pSignalObjectAndWait(NULL, event[0], 0, FALSE);
    ok( r == WAIT_FAILED, "should fail\n");


    /* valid parameters */
    r = pSignalObjectAndWait(event[0], event[1], 0, FALSE);
    ok( r == WAIT_OBJECT_0, "should succeed\n");

    /* event[0] is now signalled */
    r = pSignalObjectAndWait(event[0], event[0], 0, FALSE);
    ok( r == WAIT_OBJECT_0, "should succeed\n");

    /* event[0] is not signalled */
    r = WaitForSingleObject(event[0], 0);
    ok( r == WAIT_TIMEOUT, "event was signalled\n");

    r = pSignalObjectAndWait(event[0], event[0], 0, FALSE);
    ok( r == WAIT_OBJECT_0, "should succeed\n");

    /* clear event[1] and check for a timeout */
    ok(ResetEvent(event[1]), "failed to clear event[1]\n");
    r = pSignalObjectAndWait(event[0], event[1], 0, FALSE);
    ok( r == WAIT_TIMEOUT, "should timeout\n");

    CloseHandle(event[0]);
    CloseHandle(event[1]);

    /* create the maximum number of events and make sure 
     * we can wait on that many */
    for (i=0; i<MAXIMUM_WAIT_OBJECTS; i++)
    {
        maxevents[i] = CreateEvent(NULL, 1, 1, NULL);
        ok( maxevents[i] != 0, "should create enough events\n");
    }
    r = WaitForMultipleObjects(MAXIMUM_WAIT_OBJECTS, maxevents, 0, 0);
    ok( r != WAIT_FAILED && r != WAIT_TIMEOUT, "should succeed\n");

    for (i=0; i<MAXIMUM_WAIT_OBJECTS; i++)
        if (maxevents[i]) CloseHandle(maxevents[i]);

    /* semaphores */
    semaphore[0] = CreateSemaphore( NULL, 0, 1, NULL );
    semaphore[1] = CreateSemaphore( NULL, 1, 1, NULL );
    ok( semaphore[0] && semaphore[1], "failed to create semaphore\n");

    r = pSignalObjectAndWait(semaphore[0], semaphore[1], 0, FALSE);
    ok( r == WAIT_OBJECT_0, "should succeed\n");

    r = pSignalObjectAndWait(semaphore[0], semaphore[1], 0, FALSE);
    ok( r == WAIT_FAILED, "should fail\n");

    r = ReleaseSemaphore(semaphore[0],1,NULL);
    ok( r == FALSE, "should fail\n");

    r = ReleaseSemaphore(semaphore[1],1,NULL);
    ok( r == TRUE, "should succeed\n");

    CloseHandle(semaphore[0]);
    CloseHandle(semaphore[1]);

    /* try a registry key */
    file = CreateFile("x", GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    r = pSignalObjectAndWait(file, file, 0, FALSE);
    ok( r == WAIT_FAILED, "should fail\n");
    ok( ERROR_INVALID_HANDLE == GetLastError(), "should return invalid handle error\n");
    CloseHandle(file);
}

static void test_mutex(void)
{
    DWORD wait_ret;
    BOOL ret;
    HANDLE hCreated;
    HANDLE hOpened;

    hCreated = CreateMutex(NULL, FALSE, "WineTestMutex");
    ok(hCreated != NULL, "CreateMutex failed with error %d\n", GetLastError());
    wait_ret = WaitForSingleObject(hCreated, INFINITE);
    ok(wait_ret == WAIT_OBJECT_0, "WaitForSingleObject failed with error 0x%08x\n", wait_ret);

    /* yes, opening with just READ_CONTROL access allows us to successfully
     * call ReleaseMutex */
    hOpened = OpenMutex(READ_CONTROL, FALSE, "WineTestMutex");
    ok(hOpened != NULL, "OpenMutex failed with error %d\n", GetLastError());
    ret = ReleaseMutex(hOpened);
    todo_wine ok(ret, "ReleaseMutex failed with error %d\n", GetLastError());
    ret = ReleaseMutex(hCreated);
    todo_wine ok(!ret && (GetLastError() == ERROR_NOT_OWNER),
        "ReleaseMutex should have failed with ERROR_NOT_OWNER instead of %d\n", GetLastError());

    CloseHandle(hOpened);
    CloseHandle(hCreated);
}

static void test_slist(void)
{
    struct item
    {
        SLIST_ENTRY entry;
        int value;
    } item1, item2, item3, *pitem;

    SLIST_HEADER slist_header, test_header;
    PSLIST_ENTRY entry;
    USHORT size;

    VOID (WINAPI *pInitializeSListHead)(PSLIST_HEADER);
    USHORT (WINAPI *pQueryDepthSList)(PSLIST_HEADER);
    PSLIST_ENTRY (WINAPI *pInterlockedFlushSList)(PSLIST_HEADER);
    PSLIST_ENTRY (WINAPI *pInterlockedPopEntrySList)(PSLIST_HEADER);
    PSLIST_ENTRY (WINAPI *pInterlockedPushEntrySList)(PSLIST_HEADER,PSLIST_ENTRY);
    HMODULE kernel32;

    kernel32 = GetModuleHandle("KERNEL32.DLL");
    pInitializeSListHead = (void*) GetProcAddress(kernel32, "InitializeSListHead");
    pQueryDepthSList = (void*) GetProcAddress(kernel32, "QueryDepthSList");
    pInterlockedFlushSList = (void*) GetProcAddress(kernel32, "InterlockedFlushSList");
    pInterlockedPopEntrySList = (void*) GetProcAddress(kernel32, "InterlockedPopEntrySList");
    pInterlockedPushEntrySList = (void*) GetProcAddress(kernel32, "InterlockedPushEntrySList");
    if (pInitializeSListHead == NULL ||
        pQueryDepthSList == NULL ||
        pInterlockedFlushSList == NULL ||
        pInterlockedPopEntrySList == NULL ||
        pInterlockedPushEntrySList == NULL)
    {
        skip("some required slist entrypoints were not found, skipping tests\n");
        return;
    }

    memset(&test_header, 0, sizeof(test_header));
    memset(&slist_header, 0xFF, sizeof(slist_header));
    pInitializeSListHead(&slist_header);
    ok(memcmp(&test_header, &slist_header, sizeof(SLIST_HEADER)) == 0,
        "InitializeSListHead didn't zero-fill list header\n");
    size = pQueryDepthSList(&slist_header);
    ok(size == 0, "initially created slist has size %d, expected 0\n", size);

    item1.value = 1;
    ok(pInterlockedPushEntrySList(&slist_header, &item1.entry) == NULL,
        "previous entry in empty slist wasn't NULL\n");
    size = pQueryDepthSList(&slist_header);
    ok(size == 1, "slist with 1 item has size %d\n", size);

    item2.value = 2;
    entry = pInterlockedPushEntrySList(&slist_header, &item2.entry);
    ok(entry != NULL, "previous entry in non-empty slist was NULL\n");
    if (entry != NULL)
    {
        pitem = (struct item*) entry;
        ok(pitem->value == 1, "previous entry in slist wasn't the one added\n");
    }
    size = pQueryDepthSList(&slist_header);
    ok(size == 2, "slist with 2 items has size %d\n", size);

    item3.value = 3;
    entry = pInterlockedPushEntrySList(&slist_header, &item3.entry);
    ok(entry != NULL, "previous entry in non-empty slist was NULL\n");
    if (entry != NULL)
    {
        pitem = (struct item*) entry;
        ok(pitem->value == 2, "previous entry in slist wasn't the one added\n");
    }
    size = pQueryDepthSList(&slist_header);
    ok(size == 3, "slist with 3 items has size %d\n", size);

    entry = pInterlockedPopEntrySList(&slist_header);
    ok(entry != NULL, "entry shouldn't be NULL\n");
    if (entry != NULL)
    {
        pitem = (struct item*) entry;
        ok(pitem->value == 3, "unexpected entry removed\n");
    }
    size = pQueryDepthSList(&slist_header);
    ok(size == 2, "slist with 2 items has size %d\n", size);

    entry = pInterlockedFlushSList(&slist_header);
    size = pQueryDepthSList(&slist_header);
    ok(size == 0, "flushed slist should be empty, size is %d\n", size);
    if (size == 0)
    {
        ok(pInterlockedPopEntrySList(&slist_header) == NULL,
            "popping empty slist didn't return NULL\n");
    }
    ok(((struct item*)entry)->value == 2, "item 2 not in front of list\n");
    ok(((struct item*)entry->Next)->value == 1, "item 1 not at the back of list\n");
}

static void test_event_security(void)
{
    HANDLE handle;
    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;
    ACL acl;

    /* no sd */
    handle = CreateEventA(NULL, FALSE, FALSE, __FILE__ ": Test Event");
    ok(handle != NULL, "CreateEventW with blank sd failed with error %d\n", GetLastError());
    CloseHandle(handle);

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;

    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);

    /* blank sd */
    handle = CreateEventA(&sa, FALSE, FALSE, __FILE__ ": Test Event");
    ok(handle != NULL, "CreateEventW with blank sd failed with error %d\n", GetLastError());
    CloseHandle(handle);

    /* sd with NULL dacl */
    SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
    handle = CreateEventA(&sa, FALSE, FALSE, __FILE__ ": Test Event");
    ok(handle != NULL, "CreateEventW with blank sd failed with error %d\n", GetLastError());
    CloseHandle(handle);

    /* sd with empty dacl */
    InitializeAcl(&acl, sizeof(acl), ACL_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, &acl, FALSE);
    handle = CreateEventA(&sa, FALSE, FALSE, __FILE__ ": Test Event");
    ok(handle != NULL, "CreateEventW with blank sd failed with error %d\n", GetLastError());
    CloseHandle(handle);
}

static HANDLE sem = 0;

static void CALLBACK iocp_callback(DWORD dwErrorCode, DWORD dwNumberOfBytesTransferred, LPOVERLAPPED lpOverlapped)
{
    ReleaseSemaphore(sem, 1, NULL);
}

static BOOL WINAPI (*p_BindIoCompletionCallback)( HANDLE FileHandle, LPOVERLAPPED_COMPLETION_ROUTINE Function, ULONG Flags) = NULL;

static void test_iocp_callback(void)
{
    char temp_path[MAX_PATH];
    char filename[MAX_PATH];
    DWORD ret;
    BOOL retb;
    static const char prefix[] = "pfx";
    HANDLE hFile;
    HMODULE hmod = GetModuleHandleA("kernel32.dll");
    DWORD bytesWritten;
    const char *buffer = "12345678123456781234567812345678";
    OVERLAPPED overlapped;

    p_BindIoCompletionCallback = (void*)GetProcAddress(hmod, "BindIoCompletionCallback");
    if(!p_BindIoCompletionCallback) {
        skip("BindIoCompletionCallback not found in this DLL\n");
        return;
    }

    sem = CreateSemaphore(NULL, 0, 1, NULL);
    ok(sem != INVALID_HANDLE_VALUE, "Creating a semaphore failed\n");

    ret = GetTempPathA(MAX_PATH, temp_path);
    ok(ret != 0, "GetTempPathA error %d\n", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH\n");

    ret = GetTempFileNameA(temp_path, prefix, 0, filename);
    ok(ret != 0, "GetTempFileNameA error %d\n", GetLastError());

    hFile = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA: error %d\n", GetLastError());

    retb = p_BindIoCompletionCallback(hFile, iocp_callback, 0);
    ok(retb == FALSE, "BindIoCompletionCallback succeeded on a file that wasn't created with FILE_FLAG_OVERLAPPED\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Last error is %d\n", GetLastError());

    ret = CloseHandle(hFile);
    ok( ret, "CloseHandle: error %d\n", GetLastError());
    ret = DeleteFileA(filename);
    ok( ret, "DeleteFileA: error %d\n", GetLastError());

    hFile = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_OVERLAPPED, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA: error %d\n", GetLastError());

    retb = p_BindIoCompletionCallback(hFile, iocp_callback, 0);
    ok(retb == TRUE, "BindIoCompletionCallback failed\n");

    memset(&overlapped, 0, sizeof(overlapped));
    retb = WriteFile(hFile, (const void *) buffer, 4, &bytesWritten, &overlapped);
    ok(retb == TRUE || GetLastError() == ERROR_IO_PENDING, "WriteFile failed, lastError = %d\n", GetLastError());

    ret = WaitForSingleObject(sem, 5000);
    ok(ret == WAIT_OBJECT_0, "Wait for the IO completion callback failed\n");
    CloseHandle(sem);

    retb = p_BindIoCompletionCallback(hFile, iocp_callback, 0);
    ok(retb == FALSE, "BindIoCompletionCallback succeeded when setting the same callback on the file again\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Last error is %d\n", GetLastError());
    retb = p_BindIoCompletionCallback(hFile, NULL, 0);
    ok(retb == FALSE, "BindIoCompletionCallback succeeded when setting the callback to NULL\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Last error is %d\n", GetLastError());

    ret = CloseHandle(hFile);
    ok( ret, "CloseHandle: error %d\n", GetLastError());
    ret = DeleteFileA(filename);
    ok( ret, "DeleteFileA: error %d\n", GetLastError());

    hFile = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_OVERLAPPED, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA: error %d\n", GetLastError());
    retb = p_BindIoCompletionCallback(hFile, NULL, 0);
    ok(retb == TRUE, "BindIoCompletionCallback failed with a NULL callback(first time set)\n");
    ret = CloseHandle(hFile);
    ok( ret, "CloseHandle: error %d\n", GetLastError());
    ret = DeleteFileA(filename);
    ok( ret, "DeleteFileA: error %d\n", GetLastError());

    /* win2k3 requires the Flags parameter to be zero */
    SetLastError(0xdeadbeef);
    hFile = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_OVERLAPPED, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA: error %d\n", GetLastError());
    retb = p_BindIoCompletionCallback(hFile, iocp_callback, 12345);
    if (!retb)
        ok(GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    else
        ok(retb == TRUE, "BindIoCompletionCallback failed with Flags != 0\n");
    ret = CloseHandle(hFile);
    ok( ret, "CloseHandle: error %d\n", GetLastError());
    ret = DeleteFileA(filename);
    ok( ret, "DeleteFileA: error %d\n", GetLastError());

    retb = p_BindIoCompletionCallback(NULL, iocp_callback, 0);
    ok(retb == FALSE, "BindIoCompletionCallback succeeded on a NULL file\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Last error is %d\n", GetLastError());
}

START_TEST(sync)
{
    test_signalandwait();
    test_mutex();
    test_slist();
    test_event_security();
    test_iocp_callback();
}

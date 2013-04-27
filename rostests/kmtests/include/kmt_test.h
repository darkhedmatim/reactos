/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite test framework declarations
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

/* Inspired by Wine C unit tests, Copyright (C) 2002 Alexandre Julliard
 * Inspired by ReactOS kernel-mode regression tests,
 *                                Copyright (C) Aleksey Bragin, Filip Navara
 */

#ifndef _KMTEST_TEST_H_
#define _KMTEST_TEST_H_

#include <kmt_platform.h>

typedef VOID KMT_TESTFUNC(VOID);
typedef KMT_TESTFUNC *PKMT_TESTFUNC;

typedef struct
{
    const char *TestName;
    KMT_TESTFUNC *TestFunction;
} KMT_TEST, *PKMT_TEST;

typedef const KMT_TEST CKMT_TEST, *PCKMT_TEST;

extern const KMT_TEST TestList[];

typedef struct
{
    volatile LONG Successes;
    volatile LONG Failures;
    volatile LONG Skipped;
    volatile LONG LogBufferLength;
    LONG LogBufferMaxLength;
    CHAR LogBuffer[ANYSIZE_ARRAY];
} KMT_RESULTBUFFER, *PKMT_RESULTBUFFER;

#ifndef KMT_STANDALONE_DRIVER

/* usermode call-back mechanism */

/* list of supported operations */
typedef enum _KMT_CALLBACK_INFORMATION_CLASS
{
    QueryVirtualMemory
} KMT_CALLBACK_INFORMATION_CLASS, *PKMT_CALLBACK_INFORMATION_CLASS;

/* TODO: "response" is a little generic */
typedef union _KMT_RESPONSE
{
    MEMORY_BASIC_INFORMATION MemInfo;
} KMT_RESPONSE, *PKMT_RESPONSE;

/* this struct is sent from driver to usermode */
typedef struct _KMT_CALLBACK_REQUEST_PACKET
{
    ULONG RequestId;
    KMT_CALLBACK_INFORMATION_CLASS OperationClass;
    PVOID Parameters;
} KMT_CALLBACK_REQUEST_PACKET, *PKMT_CALLBACK_REQUEST_PACKET;

PKMT_RESPONSE KmtUserModeCallback(KMT_CALLBACK_INFORMATION_CLASS Operation, PVOID Parameters);
VOID KmtFreeCallbackResponse(PKMT_RESPONSE Response);

//macro to simplify using the mechanism
#define Test_NtQueryVirtualMemory(BaseAddress, Size, AllocationType, ProtectionType)            \
    do {                                                                                        \
    PKMT_RESPONSE NtQueryTest = KmtUserModeCallback(QueryVirtualMemory, BaseAddress);           \
    if (NtQueryTest != NULL)                                                                    \
    {                                                                                           \
        ok_eq_hex(NtQueryTest->MemInfo.Protect, ProtectionType);                                \
        ok_eq_hex(NtQueryTest->MemInfo.State, AllocationType);                                  \
        ok_eq_size(NtQueryTest->MemInfo.RegionSize, Size);                                      \
        KmtFreeCallbackResponse(NtQueryTest);                                                   \
    }                                                                                           \
    } while (0)                                                                                 \

#endif

#ifdef KMT_STANDALONE_DRIVER
#define KMT_KERNEL_MODE

typedef NTSTATUS (KMT_IRP_HANDLER)(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IoStackLocation);
typedef KMT_IRP_HANDLER *PKMT_IRP_HANDLER;

NTSTATUS KmtRegisterIrpHandler(IN UCHAR MajorFunction, IN PDEVICE_OBJECT DeviceObject OPTIONAL, IN PKMT_IRP_HANDLER IrpHandler);
NTSTATUS KmtUnregisterIrpHandler(IN UCHAR MajorFunction, IN PDEVICE_OBJECT DeviceObject OPTIONAL, IN PKMT_IRP_HANDLER IrpHandler);

typedef NTSTATUS (KMT_MESSAGE_HANDLER)(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG ControlCode,
    IN PVOID Buffer OPTIONAL,
    IN SIZE_T InLength,
    IN OUT PSIZE_T OutLength);
typedef KMT_MESSAGE_HANDLER *PKMT_MESSAGE_HANDLER;

NTSTATUS KmtRegisterMessageHandler(IN ULONG ControlCode OPTIONAL, IN PDEVICE_OBJECT DeviceObject OPTIONAL, IN PKMT_MESSAGE_HANDLER MessageHandler);
NTSTATUS KmtUnregisterMessageHandler(IN ULONG ControlCode OPTIONAL, IN PDEVICE_OBJECT DeviceObject OPTIONAL, IN PKMT_MESSAGE_HANDLER MessageHandler);

typedef enum
{
    TESTENTRY_NO_CREATE_DEVICE = 1,
    TESTENTRY_NO_REGISTER_DISPATCH = 2,
    TESTENTRY_NO_REGISTER_UNLOAD = 4,
    TESTENTRY_NO_EXCLUSIVE_DEVICE = 8,
} KMT_TESTENTRY_FLAGS;

NTSTATUS TestEntry(IN PDRIVER_OBJECT DriverObject, IN PCUNICODE_STRING RegistryPath, OUT PCWSTR *DeviceName, IN OUT INT *Flags);
VOID TestUnload(IN PDRIVER_OBJECT DriverObject);
#endif /* defined KMT_STANDALONE_DRIVER */

#ifdef KMT_KERNEL_MODE
/* Device Extension layout */
typedef struct
{
    PKMT_RESULTBUFFER ResultBuffer;
    PMDL Mdl;
} KMT_DEVICE_EXTENSION, *PKMT_DEVICE_EXTENSION;

extern BOOLEAN KmtIsCheckedBuild;
extern BOOLEAN KmtIsMultiProcessorBuild;
extern PCSTR KmtMajorFunctionNames[];
extern PDRIVER_OBJECT KmtDriverObject;

VOID KmtSetIrql(IN KIRQL NewIrql);
BOOLEAN KmtAreInterruptsEnabled(VOID);
ULONG KmtGetPoolTag(PVOID Memory);
#elif defined KMT_USER_MODE
DWORD KmtRunKernelTest(IN PCSTR TestName);

VOID KmtLoadDriver(IN PCWSTR ServiceName, IN BOOLEAN RestartIfRunning);
VOID KmtUnloadDriver(VOID);
VOID KmtOpenDriver(VOID);
VOID KmtCloseDriver(VOID);

DWORD KmtSendToDriver(IN DWORD ControlCode);
DWORD KmtSendStringToDriver(IN DWORD ControlCode, IN PCSTR String);
DWORD KmtSendWStringToDriver(IN DWORD ControlCode, IN PCWSTR String);
DWORD KmtSendBufferToDriver(IN DWORD ControlCode, IN OUT PVOID Buffer OPTIONAL, IN DWORD InLength, IN OUT PDWORD OutLength);
#else /* if !defined KMT_KERNEL_MODE && !defined KMT_USER_MODE */
#error either KMT_KERNEL_MODE or KMT_USER_MODE must be defined
#endif /* !defined KMT_KERNEL_MODE && !defined KMT_USER_MODE */

extern PKMT_RESULTBUFFER ResultBuffer;

#ifdef __GNUC__
/* TODO: GCC doesn't understand %wZ :( */
#define KMT_FORMAT(type, fmt, first) /*__attribute__((__format__(type, fmt, first)))*/
#elif !defined __GNUC__
#define KMT_FORMAT(type, fmt, first)
#endif /* !defined __GNUC__ */

#define START_TEST(name) VOID Test_##name(VOID)

#ifndef KMT_STRINGIZE
#define KMT_STRINGIZE(x) #x
#endif /* !defined KMT_STRINGIZE */
#define ok(test, ...)                ok_(test,   __FILE__, __LINE__, __VA_ARGS__)
#define trace(...)                   trace_(     __FILE__, __LINE__, __VA_ARGS__)
#define skip(test, ...)              skip_(test, __FILE__, __LINE__, __VA_ARGS__)

#define ok_(test, file, line, ...)   KmtOk(test,   file ":" KMT_STRINGIZE(line), __VA_ARGS__)
#define trace_(file, line, ...)      KmtTrace(     file ":" KMT_STRINGIZE(line), __VA_ARGS__)
#define skip_(test, file, line, ...) KmtSkip(test, file ":" KMT_STRINGIZE(line), __VA_ARGS__)

BOOLEAN KmtVOk(INT Condition, PCSTR FileAndLine, PCSTR Format, va_list Arguments)   KMT_FORMAT(ms_printf, 3, 0);
BOOLEAN KmtOk(INT Condition, PCSTR FileAndLine, PCSTR Format, ...)                  KMT_FORMAT(ms_printf, 3, 4);
VOID KmtVTrace(PCSTR FileAndLine, PCSTR Format, va_list Arguments)                  KMT_FORMAT(ms_printf, 2, 0);
VOID KmtTrace(PCSTR FileAndLine, PCSTR Format, ...)                                 KMT_FORMAT(ms_printf, 2, 3);
BOOLEAN KmtVSkip(INT Condition, PCSTR FileAndLine, PCSTR Format, va_list Arguments) KMT_FORMAT(ms_printf, 3, 0);
BOOLEAN KmtSkip(INT Condition, PCSTR FileAndLine, PCSTR Format, ...)                KMT_FORMAT(ms_printf, 3, 4);
PVOID KmtAllocateGuarded(SIZE_T SizeRequested);
VOID KmtFreeGuarded(PVOID Pointer);

#ifdef KMT_KERNEL_MODE
#define ok_irql(irql)                       ok(KeGetCurrentIrql() == irql, "IRQL is %d, expected %d\n", KeGetCurrentIrql(), irql)
#endif /* defined KMT_KERNEL_MODE */
#define ok_eq_print(value, expected, spec)  ok((value) == (expected), #value " = " spec ", expected " spec "\n", value, expected)
#define ok_eq_pointer(value, expected)      ok_eq_print(value, expected, "%p")
#define ok_eq_int(value, expected)          ok_eq_print(value, expected, "%d")
#define ok_eq_uint(value, expected)         ok_eq_print(value, expected, "%u")
#define ok_eq_long(value, expected)         ok_eq_print(value, expected, "%ld")
#define ok_eq_ulong(value, expected)        ok_eq_print(value, expected, "%lu")
#define ok_eq_longlong(value, expected)     ok_eq_print(value, expected, "%I64d")
#define ok_eq_ulonglong(value, expected)    ok_eq_print(value, expected, "%I64u")
#ifndef _WIN64
#define ok_eq_size(value, expected)         ok_eq_print(value, (SIZE_T)(expected), "%lu")
#define ok_eq_longptr(value, expected)      ok_eq_print(value, (LONG_PTR)(expected), "%ld")
#define ok_eq_ulongptr(value, expected)     ok_eq_print(value, (ULONG_PTR)(expected), "%lu")
#elif defined _WIN64
#define ok_eq_size(value, expected)         ok_eq_print(value, (SIZE_T)(expected), "%I64u")
#define ok_eq_longptr(value, expected)      ok_eq_print(value, (LONG_PTR)(expected), "%I64d")
#define ok_eq_ulongptr(value, expected)     ok_eq_print(value, (ULONG_PTR)(expected), "%I64u")
#endif /* defined _WIN64 */
#define ok_eq_hex(value, expected)          ok_eq_print(value, expected, "0x%08lx")
#define ok_bool_true(value, desc)           ok((value) == TRUE, desc " FALSE, expected TRUE\n")
#define ok_bool_false(value, desc)          ok((value) == FALSE, desc " TRUE, expected FALSE\n")
#define ok_eq_bool(value, expected)         ok((value) == (expected), #value " = %s, expected %s\n",    \
                                                (value) ? "TRUE" : "FALSE",                             \
                                                (expected) ? "TRUE" : "FALSE")
#define ok_eq_str(value, expected)          ok(!strcmp(value, expected), #value " = \"%s\", expected \"%s\"\n", value, expected)
#define ok_eq_wstr(value, expected)         ok(!wcscmp(value, expected), #value " = \"%ls\", expected \"%ls\"\n", value, expected)
#define ok_eq_tag(value, expected)          ok_eq_print(value, expected, "0x%08lx")

#define KMT_MAKE_CODE(ControlCode)  CTL_CODE(FILE_DEVICE_UNKNOWN,           \
                                             0xC00 + (ControlCode),         \
                                             METHOD_BUFFERED,               \
                                             FILE_ANY_ACCESS)

#define MICROSECOND     10
#define MILLISECOND     (1000 * MICROSECOND)
#define SECOND          (1000 * MILLISECOND)

#define KmtInvalidPointer ((PVOID)0x5555555555555555ULL)

#define KmtStartSeh()                               \
    ExceptionStatus = STATUS_SUCCESS;               \
    _SEH2_TRY                                       \
    {
#define KmtEndSeh(ExpectedStatus)                   \
    }                                               \
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)         \
    {                                               \
        ExceptionStatus = _SEH2_GetExceptionCode(); \
    } _SEH2_END;                                    \
    ok_eq_hex(ExceptionStatus, ExpectedStatus)

#if defined KMT_DEFINE_TEST_FUNCTIONS

#if defined KMT_KERNEL_MODE
BOOLEAN KmtIsCheckedBuild;
BOOLEAN KmtIsMultiProcessorBuild;
PCSTR KmtMajorFunctionNames[] =
{
    "Create",
    "CreateNamedPipe",
    "Close",
    "Read",
    "Write",
    "QueryInformation",
    "SetInformation",
    "QueryEa",
    "SetEa",
    "FlushBuffers",
    "QueryVolumeInformation",
    "SetVolumeInformation",
    "DirectoryControl",
    "FileSystemControl",
    "DeviceControl",
    "InternalDeviceControl/Scsi",
    "Shutdown",
    "LockControl",
    "Cleanup",
    "CreateMailslot",
    "QuerySecurity",
    "SetSecurity",
    "Power",
    "SystemControl",
    "DeviceChange",
    "QueryQuota",
    "SetQuota",
    "Pnp/PnpPower"
};

VOID KmtSetIrql(IN KIRQL NewIrql)
{
    KIRQL Irql = KeGetCurrentIrql();
    if (Irql > NewIrql)
        KeLowerIrql(NewIrql);
    else if (Irql < NewIrql)
        KeRaiseIrql(NewIrql, &Irql);
}

BOOLEAN KmtAreInterruptsEnabled(VOID)
{
    return (__readeflags() & (1 << 9)) != 0;
}

typedef struct _POOL_HEADER
{
    union
    {
        struct
        {
#ifdef _M_AMD64
            USHORT PreviousSize:8;
            USHORT PoolIndex:8;
            USHORT BlockSize:8;
            USHORT PoolType:8;
#else
            USHORT PreviousSize:9;
            USHORT PoolIndex:7;
            USHORT BlockSize:9;
            USHORT PoolType:7;
#endif
        };
        ULONG Ulong1;
    };
#ifdef _M_AMD64
    ULONG PoolTag;
#endif
    union
    {
#ifdef _M_AMD64
        PEPROCESS ProcessBilled;
#else
        ULONG PoolTag;
#endif
        struct
        {
            USHORT AllocatorBackTraceIndex;
            USHORT PoolTagHash;
        };
    };
} POOL_HEADER, *PPOOL_HEADER;

ULONG KmtGetPoolTag(PVOID Memory)
{
    PPOOL_HEADER Header;

    /* it's not so easy for allocations of PAGE_SIZE */
    if (((ULONG_PTR)Memory & (PAGE_SIZE - 1)) == 0)
        return 'TooL';

    Header = Memory;
    Header--;

    return Header->PoolTag;
}

INT __cdecl KmtVSNPrintF(PSTR Buffer, SIZE_T BufferMaxLength, PCSTR Format, va_list Arguments) KMT_FORMAT(ms_printf, 3, 0);
#elif defined KMT_USER_MODE
static PKMT_RESULTBUFFER KmtAllocateResultBuffer(SIZE_T ResultBufferSize)
{
    PKMT_RESULTBUFFER Buffer = HeapAlloc(GetProcessHeap(), 0, ResultBufferSize);
    if (!Buffer)
        return NULL;

    Buffer->Successes = 0;
    Buffer->Failures = 0;
    Buffer->Skipped = 0;
    Buffer->LogBufferLength = 0;
    Buffer->LogBufferMaxLength = (ULONG)ResultBufferSize - FIELD_OFFSET(KMT_RESULTBUFFER, LogBuffer);

    return Buffer;
}

static VOID KmtFreeResultBuffer(PKMT_RESULTBUFFER Buffer)
{
    HeapFree(GetProcessHeap(), 0, Buffer);
}

#define KmtVSNPrintF vsnprintf
#endif /* defined KMT_USER_MODE */

PKMT_RESULTBUFFER ResultBuffer = NULL;

static VOID KmtAddToLogBuffer(PKMT_RESULTBUFFER Buffer, PCSTR String, SIZE_T Length)
{
    LONG OldLength;
    LONG NewLength;

    if (!Buffer)
        return;

    do
    {
        OldLength = Buffer->LogBufferLength;
        NewLength = OldLength + (ULONG)Length;
        if (NewLength > Buffer->LogBufferMaxLength)
            return;
    } while (InterlockedCompareExchange(&Buffer->LogBufferLength, NewLength, OldLength) != OldLength);

    memcpy(&Buffer->LogBuffer[OldLength], String, Length);
}

KMT_FORMAT(ms_printf, 5, 0)
static SIZE_T KmtXVSNPrintF(PSTR Buffer, SIZE_T BufferMaxLength, PCSTR FileAndLine, PCSTR Prepend, PCSTR Format, va_list Arguments)
{
    SIZE_T BufferLength = 0;
    SIZE_T Length;

    if (FileAndLine)
    {
        PCSTR Slash;
        Slash = strrchr(FileAndLine, '\\');
        if (Slash)
            FileAndLine = Slash + 1;
        Slash = strrchr(FileAndLine, '/');
        if (Slash)
            FileAndLine = Slash + 1;

        Length = min(BufferMaxLength, strlen(FileAndLine));
        memcpy(Buffer, FileAndLine, Length);
        Buffer += Length;
        BufferLength += Length;
        BufferMaxLength -= Length;
    }
    if (Prepend)
    {
        Length = min(BufferMaxLength, strlen(Prepend));
        memcpy(Buffer, Prepend, Length);
        Buffer += Length;
        BufferLength += Length;
        BufferMaxLength -= Length;
    }
    if (Format)
    {
        Length = KmtVSNPrintF(Buffer, BufferMaxLength, Format, Arguments);
        /* vsnprintf can return more than maxLength, we don't want to do that */
        BufferLength += min(Length, BufferMaxLength);
    }
    return BufferLength;
}

KMT_FORMAT(ms_printf, 5, 6)
static SIZE_T KmtXSNPrintF(PSTR Buffer, SIZE_T BufferMaxLength, PCSTR FileAndLine, PCSTR Prepend, PCSTR Format, ...)
{
    SIZE_T BufferLength;
    va_list Arguments;
    va_start(Arguments, Format);
    BufferLength = KmtXVSNPrintF(Buffer, BufferMaxLength, FileAndLine, Prepend, Format, Arguments);
    va_end(Arguments);
    return BufferLength;
}

VOID KmtFinishTest(PCSTR TestName)
{
    CHAR MessageBuffer[512];
    SIZE_T MessageLength;

    if (!ResultBuffer)
        return;

    MessageLength = KmtXSNPrintF(MessageBuffer, sizeof MessageBuffer, NULL, NULL,
                                    "%s: %ld tests executed (0 marked as todo, %ld failures), %ld skipped.\n",
                                    TestName,
                                    ResultBuffer->Successes + ResultBuffer->Failures,
                                    ResultBuffer->Failures,
                                    ResultBuffer->Skipped);
    KmtAddToLogBuffer(ResultBuffer, MessageBuffer, MessageLength);
}

BOOLEAN KmtVOk(INT Condition, PCSTR FileAndLine, PCSTR Format, va_list Arguments)
{
    CHAR MessageBuffer[512];
    SIZE_T MessageLength;

    if (!ResultBuffer)
        return Condition != 0;

    if (Condition)
    {
        InterlockedIncrement(&ResultBuffer->Successes);

        if (0/*KmtReportSuccess*/)
        {
            MessageLength = KmtXSNPrintF(MessageBuffer, sizeof MessageBuffer, FileAndLine, ": Test succeeded\n", NULL);
            KmtAddToLogBuffer(ResultBuffer, MessageBuffer, MessageLength);
        }
    }
    else
    {
        InterlockedIncrement(&ResultBuffer->Failures);
        MessageLength = KmtXVSNPrintF(MessageBuffer, sizeof MessageBuffer, FileAndLine, ": Test failed: ", Format, Arguments);
        KmtAddToLogBuffer(ResultBuffer, MessageBuffer, MessageLength);
    }

    return Condition != 0;
}

BOOLEAN KmtOk(INT Condition, PCSTR FileAndLine, PCSTR Format, ...)
{
    BOOLEAN Ret;
    va_list Arguments;
    va_start(Arguments, Format);
    Ret = KmtVOk(Condition, FileAndLine, Format, Arguments);
    va_end(Arguments);
    return Ret;
}

VOID KmtVTrace(PCSTR FileAndLine, PCSTR Format, va_list Arguments)
{
    CHAR MessageBuffer[512];
    SIZE_T MessageLength;

    MessageLength = KmtXVSNPrintF(MessageBuffer, sizeof MessageBuffer, FileAndLine, ": ", Format, Arguments);
    KmtAddToLogBuffer(ResultBuffer, MessageBuffer, MessageLength);
}

VOID KmtTrace(PCSTR FileAndLine, PCSTR Format, ...)
{
    va_list Arguments;
    va_start(Arguments, Format);
    KmtVTrace(FileAndLine, Format, Arguments);
    va_end(Arguments);
}

BOOLEAN KmtVSkip(INT Condition, PCSTR FileAndLine, PCSTR Format, va_list Arguments)
{
    CHAR MessageBuffer[512];
    SIZE_T MessageLength;

    if (!ResultBuffer)
        return !Condition;

    if (!Condition)
    {
        InterlockedIncrement(&ResultBuffer->Skipped);
        MessageLength = KmtXVSNPrintF(MessageBuffer, sizeof MessageBuffer, FileAndLine, ": Tests skipped: ", Format, Arguments);
        KmtAddToLogBuffer(ResultBuffer, MessageBuffer, MessageLength);
    }

    return !Condition;
}

BOOLEAN KmtSkip(INT Condition, PCSTR FileAndLine, PCSTR Format, ...)
{
    BOOLEAN Ret;
    va_list Arguments;
    va_start(Arguments, Format);
    Ret = KmtVSkip(Condition, FileAndLine, Format, Arguments);
    va_end(Arguments);
    return Ret;
}

PVOID KmtAllocateGuarded(SIZE_T SizeRequested)
{
    NTSTATUS Status;
    SIZE_T Size = PAGE_ROUND_UP(SizeRequested + PAGE_SIZE);
    PVOID VirtualMemory = NULL;
    PCHAR StartOfBuffer;

    Status = ZwAllocateVirtualMemory(ZwCurrentProcess(), &VirtualMemory, 0, &Size, MEM_RESERVE, PAGE_NOACCESS);

    if (!NT_SUCCESS(Status))
        return NULL;

    Size -= PAGE_SIZE;
    Status = ZwAllocateVirtualMemory(ZwCurrentProcess(), &VirtualMemory, 0, &Size, MEM_COMMIT, PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        Size = 0;
        Status = ZwFreeVirtualMemory(ZwCurrentProcess(), &VirtualMemory, &Size, MEM_RELEASE);
        ok_eq_hex(Status, STATUS_SUCCESS);
        return NULL;
    }

    StartOfBuffer = VirtualMemory;
    StartOfBuffer += Size - SizeRequested;

    return StartOfBuffer;
}

VOID KmtFreeGuarded(PVOID Pointer)
{
    NTSTATUS Status;
    PVOID VirtualMemory = (PVOID)PAGE_ROUND_DOWN((SIZE_T)Pointer);
    SIZE_T Size = 0;

    Status = ZwFreeVirtualMemory(ZwCurrentProcess(), &VirtualMemory, &Size, MEM_RELEASE);
    ok_eq_hex(Status, STATUS_SUCCESS);
}

#endif /* defined KMT_DEFINE_TEST_FUNCTIONS */

#endif /* !defined _KMTEST_TEST_H_ */

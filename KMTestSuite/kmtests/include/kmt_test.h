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

#include <stdarg.h>

typedef VOID KMT_TESTFUNC(VOID);

typedef struct
{
    const char *TestName;
    KMT_TESTFUNC *TestFunction;
} KMT_TEST, *PKMT_TEST;

typedef const KMT_TEST CKMT_TEST, *PCKMT_TEST;

extern const KMT_TEST TestList[];

typedef struct {
    volatile LONG Successes;
    volatile LONG Failures;
    volatile LONG LogBufferLength;
    LONG LogBufferMaxLength;
    CHAR LogBuffer[ANYSIZE_ARRAY];
} KMT_RESULTBUFFER, *PKMT_RESULTBUFFER;

extern PKMT_RESULTBUFFER ResultBuffer;

#define START_TEST(name) VOID Test_##name(VOID)

#define KMT_STRINGIZE(x) #x
#define ok(test, ...)               ok_(test, __FILE__, __LINE__, __VA_ARGS__)
#define trace(...)                  trace_(   __FILE__, __LINE__, __VA_ARGS__)

#define ok_(test, file, line, ...)  KmtOk(test, file ":" KMT_STRINGIZE(line), __VA_ARGS__)
#define trace_(file, line, ...)     KmtTrace(   file ":" KMT_STRINGIZE(line), __VA_ARGS__)

VOID KmtVOk(INT Condition, PCSTR FileAndLine, PCSTR Format, va_list Arguments);
VOID KmtOk(INT Condition, PCSTR FileAndLine, PCSTR Format, ...);
VOID KmtVTrace(PCSTR FileAndLine, PCSTR Format, va_list Arguments);
VOID KmtTrace(PCSTR FileAndLine, PCSTR Format, ...);

#ifdef KMT_KERNEL_MODE
#define ok_irql(irql)                       ok(KeGetCurrentIrql() == irql, "IRQL is %d, expected %d\n", KeGetCurrentIrql(), irql)
#endif /* defined KMT_KERNEL_MODE */
#define ok_eq_print(value, expected, spec)  ok(value == expected, #value " = " spec ", expected " spec "\n", value, expected)
#define ok_eq_pointer(value, expected)      ok_eq_print(value, expected, "%p")
#define ok_eq_int(value, expected)          ok_eq_print(value, expected, "%d")
#define ok_eq_uint(value, expected)         ok_eq_print(value, expected, "%u")
#define ok_eq_long(value, expected)         ok_eq_print(value, expected, "%ld")
#define ok_eq_ulong(value, expected)        ok_eq_print(value, expected, "%lu")
#define ok_eq_hex(value, expected)          ok_eq_print(value, expected, "0x%08lx")
#define ok_bool_true(value, desc)           ok(value == TRUE, desc " FALSE, expected TRUE\n")
#define ok_bool_false(value, desc)          ok(value == FALSE, desc " TRUE, expected FALSE\n")
#define ok_eq_str(value, expected)          ok(!strcmp(value, expected), #value " = \"%s\", expected \"%s\"\n", value, expected)
#define ok_eq_wstr(value, expected)         ok(!wcscmp(value, expected), #value " = \"%ls\", expected \"%ls\"\n", value, expected)

#if defined KMT_DEFINE_TEST_FUNCTIONS
PKMT_RESULTBUFFER ResultBuffer = NULL;

#if defined KMT_USER_MODE
static PKMT_RESULTBUFFER KmtAllocateResultBuffer(SIZE_T LogBufferMaxLength)
{
    PKMT_RESULTBUFFER Buffer = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(KMT_RESULTBUFFER, LogBuffer[LogBufferMaxLength]));

    Buffer->Successes = 0;
    Buffer->Failures = 0;
    Buffer->LogBufferLength = 0;
    Buffer->LogBufferMaxLength = LogBufferMaxLength;

    return Buffer;
}

static VOID KmtFreeResultBuffer(PKMT_RESULTBUFFER Buffer)
{
    HeapFree(GetProcessHeap(), 0, Buffer);
}
#endif /* defined KMT_USER_MODE */

static VOID KmtAddToLogBuffer(PKMT_RESULTBUFFER Buffer, PCSTR String, SIZE_T Length)
{
    LONG OldLength;
    LONG NewLength;

    do
    {
        OldLength = Buffer->LogBufferLength;
        NewLength = OldLength + Length;
        if (NewLength > Buffer->LogBufferMaxLength)
        {
            /* TODO: indicate failure somehow */
            __debugbreak();
            return;
        }
    } while (InterlockedCompareExchange(&Buffer->LogBufferLength, NewLength, OldLength) != OldLength);

    memcpy(&Buffer->LogBuffer[OldLength], String, Length);
}

#ifdef KMT_KERNEL_MODE
INT __cdecl KmtVSNPrintF(PSTR Buffer, SIZE_T BufferMaxLength, PCSTR Format, va_list Arguments);
#elif defined KMT_USER_MODE
#define KmtVSNPrintF vsnprintf
#endif /* defined KMT_USER_MODE */

static SIZE_T KmtXVSNPrintF(PSTR Buffer, SIZE_T BufferMaxLength, PCSTR Prepend1, PCSTR Prepend2, PCSTR Format, va_list Arguments)
{
    SIZE_T BufferLength = 0;
    SIZE_T Length;

    if (Prepend1)
    {
        SIZE_T Length = min(BufferMaxLength, strlen(Prepend1));
        memcpy(Buffer, Prepend1, Length);
        Buffer += Length;
        BufferLength += Length;
        BufferMaxLength -= Length;
    }
    if (Prepend2)
    {
        SIZE_T Length = min(BufferMaxLength, strlen(Prepend2));
        memcpy(Buffer, Prepend2, Length);
        Buffer += Length;
        BufferLength += Length;
        BufferMaxLength -= Length;
    }
    Length = KmtVSNPrintF(Buffer, BufferMaxLength, Format, Arguments);
    /* vsnprintf can return more than maxLength, we don't want to do that */
    BufferLength += min(Length, BufferMaxLength);
    return BufferLength;
}

static SIZE_T KmtXSNPrintF(PSTR Buffer, SIZE_T BufferMaxLength, PCSTR Prepend1, PCSTR Prepend2, PCSTR Format, ...)
{
    SIZE_T BufferLength;
    va_list Arguments;
    va_start(Arguments, Format);
    BufferLength = KmtXVSNPrintF(Buffer, BufferMaxLength, Prepend1, Prepend2, Format, Arguments);
    va_end(Arguments);
    return BufferLength;
}

VOID KmtFinishTest(PCSTR TestName)
{
    CHAR MessageBuffer[512];
    SIZE_T MessageLength;

    MessageLength = KmtXSNPrintF(MessageBuffer, sizeof MessageBuffer, NULL, NULL,
                                    "%s: %d tests executed (0 marked as todo, %d failures), 0 skipped.\n",
                                    TestName,
                                    ResultBuffer->Successes + ResultBuffer->Failures,
                                    ResultBuffer->Failures);
    KmtAddToLogBuffer(ResultBuffer, MessageBuffer, MessageLength);
}

VOID KmtVOk(INT Condition, PCSTR FileAndLine, PCSTR Format, va_list Arguments)
{
    CHAR MessageBuffer[512];
    SIZE_T MessageLength;

    if (Condition)
    {
        InterlockedIncrement(&ResultBuffer->Successes);

        if (0/*KmtReportSuccess*/)
        {
            MessageLength = KmtXSNPrintF(MessageBuffer, sizeof MessageBuffer, FileAndLine, ": Test succeeded\n", "");
            KmtAddToLogBuffer(ResultBuffer, MessageBuffer, MessageLength);
        }
    }
    else
    {
        InterlockedIncrement(&ResultBuffer->Failures);
        MessageLength = KmtXVSNPrintF(MessageBuffer, sizeof MessageBuffer, FileAndLine, ": Test failed: ", Format, Arguments);
        KmtAddToLogBuffer(ResultBuffer, MessageBuffer, MessageLength);
    }
}

VOID KmtOk(INT Condition, PCSTR FileAndLine, PCSTR Format, ...)
{
    va_list Arguments;
    va_start(Arguments, Format);
    KmtVOk(Condition, FileAndLine, Format, Arguments);
    va_end(Arguments);
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

#endif /* defined KMT_DEFINE_TEST_FUNCTIONS */

#endif /* !defined _KMTEST_TEST_H_ */

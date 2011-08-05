/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite platform declarations
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#ifndef _KMTEST_PLATFORM_H_
#define _KMTEST_PLATFORM_H_

#if defined KMT_KERNEL_MODE || defined KMT_STANDALONE_DRIVER
#include <ntddk.h>
#include <ntifs.h>
#include <ndk/ntndk.h>
#include <ntstrsafe.h>

#elif defined KMT_USER_MODE
#define WIN32_LEAN_AND_MEAN
#define WIN32_NO_STATUS
#define UNICODE
#include <windows.h>
#include <ndk/ntndk.h>
#include <strsafe.h>
#include <winioctl.h>

#ifdef KMT_EMULATE_KERNEL
#define ok_irql(i)
#ifdef __GNUC__
#define KIRQL __attribute__((__unused__)) int
#elif !defined __GNUC__
#define KIRQL int
#endif /* !defined __GNUC__ */
typedef const UCHAR CUCHAR, *PCUCHAR;
typedef ULONG LOGICAL, *PLOGICAL;

#undef KeRaiseIrql
#define KeRaiseIrql(new, old)
#undef KeLowerIrql
#define KeLowerIrql(i)
#define ExAllocatePool(type, size)              HeapAlloc(GetProcessHeap(), 0, size)
#define ExAllocatePoolWithTag(type, size, tag)  HeapAlloc(GetProcessHeap(), 0, size)
#define ExFreePool(p)                           HeapFree(GetProcessHeap(), 0, p)
#define ExFreePoolWithTag(p, tag)               HeapFree(GetProcessHeap(), 0, p)
#define RtlCopyMemoryNonTemporal                RtlCopyMemory
#define RtlPrefetchMemoryNonTemporal(s, l)
#endif /* defined KMT_EMULATE_KERNEL */

#endif /* defined KMT_USER_MODE */

#include <pseh/pseh2.h>
#include <limits.h>

#endif /* !defined _KMTEST_PLATFORM_H_ */

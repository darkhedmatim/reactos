/* $Id: rtl.h,v 1.17 2000/06/29 23:35:22 dwelch Exp $
 *
 */

#ifndef __INCLUDE_NTDLL_RTL_H
#define __INCLUDE_NTDLL_RTL_H

#include <napi/teb.h>

typedef struct _CRITICAL_SECTION_DEBUG {
    WORD   Type;
    WORD   CreatorBackTraceIndex;
    struct _CRITICAL_SECTION *CriticalSection;
    LIST_ENTRY ProcessLocksList;
    DWORD EntryCount;
    DWORD ContentionCount;
    DWORD Depth;
    PVOID OwnerBackTrace[ 5 ];
} CRITICAL_SECTION_DEBUG, *PCRITICAL_SECTION_DEBUG;

typedef struct _CRITICAL_SECTION {
    PCRITICAL_SECTION_DEBUG DebugInfo;
    LONG LockCount;
    LONG RecursionCount;
    HANDLE OwningThread;
    HANDLE LockSemaphore;
    DWORD Reserved;
} CRITICAL_SECTION, *PCRITICAL_SECTION, *LPCRITICAL_SECTION;


/*
 * Preliminary data type!!
 *
 * This definition is not finished yet. It will change in the future.
 */
typedef struct _RTL_USER_PROCESS_INFO
{
	ULONG		Unknown1;		// 0x00
	HANDLE		ProcessHandle;		// 0x04
	HANDLE		ThreadHandle;		// 0x08
	CLIENT_ID	ClientId;		// 0x0C
	ULONG		Unknown5;		// 0x14
	LONG		StackZeroBits;		// 0x18
	LONG		StackReserved;		// 0x1C
	LONG		StackCommit;		// 0x20
	ULONG		Unknown9;		// 0x24
// more data ... ???
} RTL_USER_PROCESS_INFO, *PRTL_USER_PROCESS_INFO;



#define HEAP_BASE (0xa0000000)

VOID
STDCALL
RtlDeleteCriticalSection (
	LPCRITICAL_SECTION	lpCriticalSection
	);

VOID
STDCALL
RtlEnterCriticalSection (
	LPCRITICAL_SECTION	lpCriticalSection
	);

VOID
STDCALL
RtlInitializeCriticalSection (
	LPCRITICAL_SECTION	pcritical
	);

VOID
STDCALL
RtlLeaveCriticalSection (
	LPCRITICAL_SECTION	lpCriticalSection
	);

BOOLEAN
STDCALL
RtlTryEntryCriticalSection (
	LPCRITICAL_SECTION	lpCriticalSection
	);

UINT
STDCALL
RtlCompactHeap (
	HANDLE	heap,
	DWORD	flags
	);

BOOLEAN
STDCALL
RtlEqualComputerName (
	IN	PUNICODE_STRING	ComputerName1,
	IN	PUNICODE_STRING	ComputerName2
	);

BOOLEAN
STDCALL
RtlEqualDomainName (
	IN	PUNICODE_STRING	DomainName1,
	IN	PUNICODE_STRING	DomainName2
	);

VOID
STDCALL
RtlEraseUnicodeString (
	IN	PUNICODE_STRING	String
	);

NTSTATUS
STDCALL
RtlLargeIntegerToChar (
	IN	PLARGE_INTEGER	Value,
	IN	ULONG		Base,
	IN	ULONG		Length,
	IN OUT	PCHAR		String
	);


/* Path functions */

ULONG
STDCALL
RtlDetermineDosPathNameType_U (
	PWSTR Path
	);

BOOLEAN
STDCALL
RtlDoesFileExists_U (
	PWSTR FileName
	);

BOOLEAN
STDCALL
RtlDosPathNameToNtPathName_U (
	PWSTR		dosname,
	PUNICODE_STRING	ntname,
	PWSTR		*shortname,
	PCURDIR		nah
	);

ULONG
STDCALL
RtlDosSearchPath_U (
	WCHAR *sp,
	WCHAR *name,
	WCHAR *ext,
	ULONG buf_sz,
	WCHAR *buffer,
	WCHAR **shortname
	);

ULONG
STDCALL
RtlGetCurrentDirectory_U (
	ULONG MaximumLength,
	PWSTR Buffer
	);

ULONG
STDCALL
RtlGetFullPathName_U (
	WCHAR *dosname,
	ULONG size,
	WCHAR *buf,
	WCHAR **shortname
	);

ULONG
STDCALL
RtlGetLongestNtPathLength (
	VOID
	);

ULONG
STDCALL
RtlIsDosDeviceName_U (
	PWSTR DeviceName
	);

NTSTATUS
STDCALL
RtlSetCurrentDirectory_U (
	PUNICODE_STRING name
	);

/* Environment functions */
VOID
STDCALL
RtlAcquirePebLock (
	VOID
	);

VOID
STDCALL
RtlReleasePebLock (
	VOID
	);

NTSTATUS
STDCALL
RtlCreateEnvironment (
	BOOLEAN	Inherit,
	PVOID	*Environment
	);

VOID
STDCALL
RtlDestroyEnvironment (
	PVOID	Environment
	);

NTSTATUS
STDCALL
RtlExpandEnvironmentStrings_U (
	PVOID		Environment,
	PUNICODE_STRING	Source,
	PUNICODE_STRING	Destination,
	PULONG		Length
	);

NTSTATUS
STDCALL
RtlQueryEnvironmentVariable_U (
	PVOID		Environment,
	PUNICODE_STRING	Name,
	PUNICODE_STRING	Value
	);

VOID
STDCALL
RtlSetCurrentEnvironment (
	PVOID	NewEnvironment,
	PVOID	*OldEnvironment
	);

NTSTATUS
STDCALL
RtlSetEnvironmentVariable (
	PVOID		*Environment,
	PUNICODE_STRING	Name,
	PUNICODE_STRING	Value
	);

NTSTATUS
STDCALL
RtlCreateUserThread (
	IN	HANDLE			ProcessHandle,
	IN	PSECURITY_DESCRIPTOR	SecurityDescriptor,
	IN	BOOLEAN			CreateSuspended,
	IN	LONG			StackZeroBits,
	IN OUT	PULONG			StackReserved,
	IN OUT	PULONG			StackCommit,
	IN	PTHREAD_START_ROUTINE	StartAddress,
	IN	PVOID			Parameter,
	IN OUT	PHANDLE			ThreadHandle,
	IN OUT	PCLIENT_ID		ClientId
	);

NTSTATUS
STDCALL
RtlFreeUserThreadStack (
	IN	HANDLE	ProcessHandle,
	IN	HANDLE	ThreadHandle
	);

/*
 * Preliminary prototype!!
 *
 * This prototype is not finished yet. It will change in the future.
 */
NTSTATUS
STDCALL
RtlCreateUserProcess (
	PUNICODE_STRING			CommandLine,
	ULONG				Unknown2,
	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters,	// verified
	PSECURITY_DESCRIPTOR		ProcessSd,
	PSECURITY_DESCRIPTOR		ThreadSd,
	BOOL				bInheritHandles,
	DWORD				dwCreationFlags,
	ULONG				Unknown8,
	ULONG				Unknown9,
	PRTL_USER_PROCESS_INFO		ProcessInfo		// verified
	);

NTSTATUS
STDCALL
RtlCreateProcessParameters (
	IN OUT	PRTL_USER_PROCESS_PARAMETERS	*ProcessParameters,
	IN	PUNICODE_STRING	CommandLine,
	IN	PUNICODE_STRING	DllPath,
	IN	PUNICODE_STRING	CurrentDirectory,
	IN	PUNICODE_STRING	ImagePathName,
	IN	PVOID		Environment,
	IN	PUNICODE_STRING	WindowTitle,
	IN	PUNICODE_STRING	DesktopInfo,
	IN	PUNICODE_STRING	ShellInfo,
	IN	PUNICODE_STRING	RuntimeData
	);

PRTL_USER_PROCESS_PARAMETERS
STDCALL
RtlDeNormalizeProcessParams (
	IN OUT	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters
	);

VOID
STDCALL
RtlDestroyProcessParameters (
	IN OUT	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters
	);

PRTL_USER_PROCESS_PARAMETERS
STDCALL
RtlNormalizeProcessParams (
	IN OUT	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters
	);

NTSTATUS
STDCALL
RtlLocalTimeToSystemTime (
	PLARGE_INTEGER	LocalTime,
	PLARGE_INTEGER	SystemTime
	);

NTSTATUS
STDCALL
RtlSystemTimeToLocalTime (
	PLARGE_INTEGER	SystemTime,
	PLARGE_INTEGER	LocalTime
	);

#endif /* __INCLUDE_NTDLL_RTL_H */

/* EOF */

/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            include/internal/ldr.h
 * PURPOSE:         Header for loader module
 */

#ifndef __INCLUDE_INTERNAL_LDR_H
#define __INCLUDE_INTERNAL_LDR_H

#include <pe.h>

NTSTATUS
LdrLoadDriver (
	IN	PUNICODE_STRING	Filename
	);
NTSTATUS
LdrLoadInitialProcess (
	VOID
	);
VOID
LdrLoadAutoConfigDrivers (
	VOID
	);
VOID
LdrInitModuleManagement (
	VOID
	);
NTSTATUS
LdrProcessDriver (
	IN	PVOID	ModuleLoadBase
	);
NTSTATUS
LdrpMapSystemDll (
	HANDLE	ProcessHandle,
	PVOID	* LdrStartupAddress
	);
PIMAGE_NT_HEADERS
STDCALL
RtlImageNtHeader (
	IN	PVOID	BaseAddress
	);
PVOID
LdrpGetSystemDllEntryPoint (
	VOID
	);
NTSTATUS
LdrpMapImage (
	HANDLE	ProcessHandle, 
	HANDLE	SectionHandle,
	PVOID	* ImageBase
	);

#endif /* __INCLUDE_INTERNAL_LDR_H */

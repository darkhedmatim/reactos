/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/include/internal/xhal.h
 * PURPOSE:         HAL Callbacks
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/
#ifndef __INCLUDE_INTERNAL_XHAL_H
#define __INCLUDE_INTERNAL_XHAL_H

#include <ddk/ntdddisk.h>

NTSTATUS
FASTCALL
xHalQueryDriveLayout(
    IN PUNICODE_STRING DeviceName,
    OUT PDRIVE_LAYOUT_INFORMATION *LayoutInfo
);

#undef HalExamineMBR
VOID 
FASTCALL
HalExamineMBR(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG MBRTypeIdentifier,
    OUT PVOID *Buffer
);

VOID 
FASTCALL
xHalIoAssignDriveLetters(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PSTRING NtDeviceName,
    OUT PUCHAR NtSystemPath,
    OUT PSTRING NtSystemPathString
);

NTSTATUS 
FASTCALL
xHalIoReadPartitionTable(
    PDEVICE_OBJECT DeviceObject,
    ULONG SectorSize,
    BOOLEAN ReturnRecognizedPartitions,
    PDRIVE_LAYOUT_INFORMATION *PartitionBuffer
);

NTSTATUS 
FASTCALL
xHalIoSetPartitionInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG PartitionNumber,
    IN ULONG PartitionType
);

NTSTATUS 
FASTCALL
xHalIoWritePartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfHeads,
    IN PDRIVE_LAYOUT_INFORMATION PartitionBuffer
);

#endif

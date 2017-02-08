/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NTFS FS library
 * FILE:        lib/fslib/btrfslib/btrfslib.c
 * PURPOSE:     BTRFS lib
 * PROGRAMMERS: Pierre Schweitzer
 */
#include "btrfslib.h"

#define NDEBUG
#include <debug.h>

NTSTATUS NTAPI
BtrfsFormat(IN PUNICODE_STRING DriveRoot,
            IN FMIFS_MEDIA_FLAG MediaFlag,
            IN PUNICODE_STRING Label,
            IN BOOLEAN QuickFormat,
            IN ULONG ClusterSize,
            IN PFMIFSCALLBACK Callback)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}


NTSTATUS WINAPI
BtrfsChkdsk(IN PUNICODE_STRING DriveRoot,
            IN BOOLEAN FixErrors,
            IN BOOLEAN Verbose,
            IN BOOLEAN CheckOnlyIfDirty,
            IN BOOLEAN ScanDrive,
            IN PFMIFSCALLBACK Callback)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

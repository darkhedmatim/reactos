/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/include/ramdisk.h
 * PURPOSE:         Header file for ramdisk support
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#ifndef _RAMDISK_
#define _RAMDISK_

//
// Ramdisk Routines
//
VOID
NTAPI
RamDiskInit(
    IN PCHAR CmdLine
);

VOID
NTAPI
RamDiskSwitchFromBios(
    VOID
);

VOID
NTAPI
RamDiskCheckForVirtualFile(
    VOID
);

#endif

/*
 *  FreeLoader
 *
 *  Copyright (C) 2003  Eric Kohl
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

#ifndef __MEMORY_H
#include "mm.h"
#endif

VOID PcMachInit(const char *CmdLine);

VOID PcBeep(VOID);

VOID PcConsPutChar(int Ch);
BOOLEAN PcConsKbHit(VOID);
int PcConsGetCh(VOID);

VOID PcVideoClearScreen(UCHAR Attr);
VIDEODISPLAYMODE PcVideoSetDisplayMode(char *DisplayMode, BOOLEAN Init);
VOID PcVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth);
ULONG PcVideoGetBufferSize(VOID);
VOID PcVideoSetTextCursorPosition(UCHAR X, UCHAR Y);
VOID PcVideoHideShowTextCursor(BOOLEAN Show);
VOID PcVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y);
VOID PcVideoCopyOffScreenBufferToVRAM(PVOID Buffer);
BOOLEAN PcVideoIsPaletteFixed(VOID);
VOID PcVideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue);
VOID PcVideoGetPaletteColor(UCHAR Color, UCHAR* Red, UCHAR* Green, UCHAR* Blue);
VOID PcVideoSync(VOID);
VOID PcVideoPrepareForReactOS(IN BOOLEAN Setup);
VOID PcPrepareForReactOS(IN BOOLEAN Setup);

PFREELDR_MEMORY_DESCRIPTOR PcMemGetMemoryMap(ULONG *MemoryMapSize);

BOOLEAN PcDiskGetBootPath(OUT PCHAR BootPath, IN ULONG Size);
BOOLEAN PcDiskReadLogicalSectors(UCHAR DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer);
BOOLEAN PcDiskGetPartitionEntry(UCHAR DriveNumber, ULONG PartitionNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry);
BOOLEAN PcDiskGetDriveGeometry(UCHAR DriveNumber, PGEOMETRY DriveGeometry);
ULONG PcDiskGetCacheableBlockCount(UCHAR DriveNumber);

TIMEINFO* PcGetTime(VOID);

BOOLEAN PcInitializeBootDevices(VOID);
PCONFIGURATION_COMPONENT_DATA PcHwDetect(VOID);
VOID PcHwIdle(VOID);

extern BIOS_MEMORY_MAP PcBiosMemoryMap[];
extern ULONG PcBiosMapCount;

/* EOF */

/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

	
#include <freeldr.h>
#include <arch.h>
#include <miscboot.h>
#include <rtl.h>
#include <fs.h>
#include <ui.h>
#include <inifile.h>
#include <disk.h>
#include <video.h>

VOID LoadAndBootBootSector(PUCHAR OperatingSystemName)
{
	PFILE	FilePointer;
	UCHAR	SettingName[80];
	UCHAR	SettingValue[80];
	U32		SectionId;
	UCHAR	FileName[260];
	U32		BytesRead;

	// Find all the message box settings and run them
	UiShowMessageBoxesInSection(OperatingSystemName);

	// Try to open the operating system section in the .ini file
	if (!IniOpenSection(OperatingSystemName, &SectionId))
	{
		sprintf(SettingName, "Section [%s] not found in freeldr.ini.\n", OperatingSystemName);
		UiMessageBox(SettingName);
		return;
	}

	if (!IniReadSettingByName(SectionId, "BootDrive", SettingValue, 80))
	{
		UiMessageBox("Boot drive not specified for selected OS!");
		return;
	}

	BootDrive = atoi(SettingValue);

	BootPartition = 0;
	if (IniReadSettingByName(SectionId, "BootPartition", SettingValue, 80))
	{
		BootPartition = atoi(SettingValue);
	}

	if (!IniReadSettingByName(SectionId, "BootSectorFile", FileName, 260))
	{
		UiMessageBox("Boot sector file not specified for selected OS!");
		return;
	}

	if (!OpenDiskDrive(BootDrive, BootPartition))
	{
		UiMessageBox("Failed to open boot drive.");
		return;
	}

	FilePointer = OpenFile(FileName);
	if (FilePointer == NULL)
	{
		strcat(FileName, " not found.");
		UiMessageBox(FileName);
		return;
	}

	// Read boot sector
	if (!ReadFile(FilePointer, 512, &BytesRead, (void*)0x7c00) || (BytesRead != 512))
	{
		return;
	}

	// Check for validity
	if (*((U16*)(0x7c00 + 0x1fe)) != 0xaa55)
	{
		UiMessageBox("Invalid boot sector magic (0xaa55)");
		return;
	}

	VideoClearScreen();
	VideoShowTextCursor();
	// Don't stop the floppy drive motor when we
	// are just booting a bootsector, or drive, or partition.
	// If we were to stop the floppy motor then
	// the BIOS wouldn't be informed and if the
	// next read is to a floppy then the BIOS will
	// still think the motor is on and this will
	// result in a read error.
	//StopFloppyMotor();
	ChainLoadBiosBootSectorCode();
}

VOID LoadAndBootPartition(PUCHAR OperatingSystemName)
{
	UCHAR					SettingName[80];
	UCHAR					SettingValue[80];
	U32						SectionId;
	PARTITION_TABLE_ENTRY	PartitionTableEntry;

	// Find all the message box settings and run them
	UiShowMessageBoxesInSection(OperatingSystemName);

	// Try to open the operating system section in the .ini file
	if (!IniOpenSection(OperatingSystemName, &SectionId))
	{
		sprintf(SettingName, "Section [%s] not found in freeldr.ini.\n", OperatingSystemName);
		UiMessageBox(SettingName);
		return;
	}

	// Read the boot drive
	if (!IniReadSettingByName(SectionId, "BootDrive", SettingValue, 80))
	{
		UiMessageBox("Boot drive not specified for selected OS!");
		return;
	}

	BootDrive = atoi(SettingValue);

	// Read the boot partition
	if (!IniReadSettingByName(SectionId, "BootPartition", SettingValue, 80))
	{
		UiMessageBox("Boot partition not specified for selected OS!");
		return;
	}

	BootPartition = atoi(SettingValue);

	// Get the partition table entry
	if (!DiskGetPartitionEntry(BootDrive, BootPartition, &PartitionTableEntry))
	{
		return;
	}

	// Now try to read the partition boot sector
	// If this fails then abort
	if (!DiskReadLogicalSectors(BootDrive, PartitionTableEntry.SectorCountBeforePartition, 1, (PVOID)0x7C00))
	{
		return;
	}

	// Check for validity
	if (*((U16*)(0x7c00 + 0x1fe)) != 0xaa55)
	{
		UiMessageBox("Invalid boot sector magic (0xaa55)");
		return;
	}

	VideoClearScreen();
	VideoShowTextCursor();
	// Don't stop the floppy drive motor when we
	// are just booting a bootsector, or drive, or partition.
	// If we were to stop the floppy motor then
	// the BIOS wouldn't be informed and if the
	// next read is to a floppy then the BIOS will
	// still think the motor is on and this will
	// result in a read error.
	//StopFloppyMotor();
	ChainLoadBiosBootSectorCode();
}

VOID LoadAndBootDrive(PUCHAR OperatingSystemName)
{
	UCHAR	SettingName[80];
	UCHAR	SettingValue[80];
	U32		SectionId;

	// Find all the message box settings and run them
	UiShowMessageBoxesInSection(OperatingSystemName);

	// Try to open the operating system section in the .ini file
	if (!IniOpenSection(OperatingSystemName, &SectionId))
	{
		sprintf(SettingName, "Section [%s] not found in freeldr.ini.\n", OperatingSystemName);
		UiMessageBox(SettingName);
		return;
	}

	if (!IniReadSettingByName(SectionId, "BootDrive", SettingValue, 80))
	{
		UiMessageBox("Boot drive not specified for selected OS!");
		return;
	}

	BootDrive = atoi(SettingValue);

	// Now try to read the boot sector (or mbr)
	// If this fails then abort
	if (!DiskReadLogicalSectors(BootDrive, 0, 1, (PVOID)0x7C00))
	{
		return;
	}

	// Check for validity
	if (*((U16*)(0x7c00 + 0x1fe)) != 0xaa55)
	{
		UiMessageBox("Invalid boot sector magic (0xaa55)");
		return;
	}

	VideoClearScreen();
	VideoShowTextCursor();
	// Don't stop the floppy drive motor when we
	// are just booting a bootsector, or drive, or partition.
	// If we were to stop the floppy motor then
	// the BIOS wouldn't be informed and if the
	// next read is to a floppy then the BIOS will
	// still think the motor is on and this will
	// result in a read error.
	//StopFloppyMotor();
	ChainLoadBiosBootSectorCode();
}

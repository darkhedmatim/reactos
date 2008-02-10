/*
 *  ReactOS kernel
 *  Copyright (C) 2008 ReactOS Team
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
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystem/ntfs/dispatch.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMER:       Pierre Schweitzer
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "ntfs.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *****************************************************************/


/* FUNCTIONS ****************************************************************/

/*
 * FUNCTION: This function manages IRP for various major functions
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 *           Irp = IRP to be passed to internal functions
 * RETURNS: Status of I/O Request
 */
NTSTATUS NTAPI
NtfsFsdDispatch(PDEVICE_OBJECT DeviceObject,
                PIRP Irp)
{
  PNTFS_IRP_CONTEXT IrpContext = NULL;
  NTSTATUS Status = STATUS_UNSUCCESSFUL;
  
  TRACE_(NTFS, "NtfsFsdDispatch()\n");
  
  FsRtlEnterFileSystem();
  ASSERT(DeviceObject);
  ASSERT(Irp);
  
  NtfsIsIrpTopLevel(Irp);
  
  IrpContext = NtfsAllocateIrpContext(DeviceObject, Irp);
  if (IrpContext)
  {
    switch (IrpContext->MajorFunction)
    {
      case IRP_MJ_QUERY_VOLUME_INFORMATION:
      {
        Status = NtfsQueryVolumeInformation(IrpContext);
        break;
      }
      case IRP_MJ_SET_VOLUME_INFORMATION:
      {
        Status = NtfsSetVolumeInformation(IrpContext);
        break;
      }
    }
  }
  else
    Status = STATUS_INSUFFICIENT_RESOURCES;
	
  Irp->IoStatus.Status = Status;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);
	
	if (IrpContext)
    ExFreePoolWithTag(IrpContext, TAG('N', 'I', 'R', 'P'));
	
  IoSetTopLevelIrp(NULL);
  FsRtlExitFileSystem();
  return Status;
}

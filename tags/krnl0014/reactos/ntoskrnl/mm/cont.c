/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/cont.c
 * PURPOSE:         Manages continuous memory
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

PVOID MmAllocateContiguousMemory(ULONG NumberOfBytes,
				 PHYSICAL_ADDRESS HighestAcceptableAddress)
{
   UNIMPLEMENTED;
}

VOID MmFreeContiguousMemory(PVOID BaseAddress)
{
   UNIMPLEMENTED;
}

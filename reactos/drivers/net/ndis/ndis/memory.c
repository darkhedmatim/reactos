/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/memory.c
 * PURPOSE:     Memory management routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <ndissys.h>


/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisAllocateMemoryWithTag(
    OUT PVOID   *VirtualAddress,
    IN  UINT    Length,
    IN  ULONG   Tag)
/*
 * FUNCTION:  Allocates a block of memory, with a 32-bit tag
 * ARGUMENTS:
 *   VirtualAddress = a pointer to the returned memory block
 *   Length         = the number of requested bytes
 *   Tag            = 32-bit pool tag 
 * NOTES:
 *    NDIS 5.0
 */
{
    PVOID Block;

    /* Plain nonpaged memory with tag */
    Block           = ExAllocatePoolWithTag(NonPagedPool, Length, Tag);
    *VirtualAddress = Block;
    if (!Block)
        return NDIS_STATUS_FAILURE;

	return NDIS_STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisCreateLookaheadBufferFromSharedMemory(
    IN  PVOID   pSharedMemory,
    IN  UINT    LookaheadLength,
    OUT PVOID   *pLookaheadBuffer)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisDestroyLookaheadBufferFromSharedMemory(
    IN  PVOID   pLookaheadBuffer)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMoveFromMappedMemory(
    OUT PVOID   Destination,
    IN  PVOID   Source,
    IN  ULONG   Length)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMoveMappedMemory(
    OUT PVOID   Destination,
    IN  PVOID   Source,
    IN  ULONG   Length)
{
    RtlCopyMemory(Destination,Source,Length);
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMoveToMappedMemory(
    OUT PVOID   Destination,
    IN  PVOID   Source,
    IN  ULONG   Length)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMUpdateSharedMemory(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  ULONG                   Length,
    IN  PVOID                   VirtualAddress,
    IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisAllocateMemory(
    OUT PVOID                   *VirtualAddress,
    IN  UINT                    Length,
    IN  UINT                    MemoryFlags,
    IN  NDIS_PHYSICAL_ADDRESS   HighestAcceptableAddress)
/*
 * FUNCTION: Allocates a block of memory
 * ARGUMENTS:
 *     VirtualAddress           = Address of buffer to place virtual
 *                                address of the allocated memory
 *     Length                   = Size of the memory block to allocate
 *     MemoryFlags              = Flags to specify special restrictions
 *     HighestAcceptableAddress = Specifies -1
 */
{
  if (MemoryFlags & NDIS_MEMORY_NONCACHED) 
    {
      *VirtualAddress = MmAllocateNonCachedMemory(Length);
      if(!*VirtualAddress)
        return NDIS_STATUS_FAILURE;

      return NDIS_STATUS_SUCCESS;
    }

  if (MemoryFlags & NDIS_MEMORY_CONTIGUOUS) 
    {
      *VirtualAddress = MmAllocateContiguousMemory(Length, HighestAcceptableAddress);
      if(!*VirtualAddress)
        return NDIS_STATUS_FAILURE;

      return NDIS_STATUS_SUCCESS;
    }

  /* Plain nonpaged memory */
  *VirtualAddress = ExAllocatePool(NonPagedPool, Length);
  if (!*VirtualAddress)
    return NDIS_STATUS_FAILURE;

  return NDIS_STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisFreeMemory(
    IN  PVOID   VirtualAddress,
    IN  UINT    Length,
    IN  UINT    MemoryFlags)
/*
 * FUNCTION: Frees a memory block allocated with NdisAllocateMemory
 * ARGUMENTS:
 *     VirtualAddress = Pointer to the base virtual address of the allocated memory
 *     Length         = Size of the allocated memory block as passed to NdisAllocateMemory
 *     MemoryFlags    = Memory flags passed to NdisAllocateMemory
 */
{
  if (MemoryFlags & NDIS_MEMORY_NONCACHED) 
    {
      MmFreeNonCachedMemory(VirtualAddress, Length);
      return;
    }

  if (MemoryFlags & NDIS_MEMORY_CONTIGUOUS) 
    {
      MmFreeContiguousMemory(VirtualAddress);
      return;
    }

  ExFreePool(VirtualAddress);
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisImmediateReadSharedMemory(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       SharedMemoryAddress,
    OUT PUCHAR      Buffer,
    IN  ULONG       Length)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisImmediateWriteSharedMemory(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       SharedMemoryAddress,
    IN  PUCHAR      Buffer,
    IN  ULONG       Length)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMAllocateSharedMemory(
    IN	NDIS_HANDLE             MiniportAdapterHandle,
    IN	ULONG                   Length,
    IN	BOOLEAN                 Cached,
    OUT	PVOID                   *VirtualAddress,
    OUT	PNDIS_PHYSICAL_ADDRESS  PhysicalAddress)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMAllocateSharedMemoryAsync(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  ULONG       Length,
    IN  BOOLEAN     Cached,
    IN  PVOID       Context)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMFreeSharedMemory(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  ULONG                   Length,
    IN  BOOLEAN                 Cached,
    IN  PVOID                   VirtualAddress,
    IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress)
{
    UNIMPLEMENTED
}

/* EOF */

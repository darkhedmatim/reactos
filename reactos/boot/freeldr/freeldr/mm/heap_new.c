/*
 *  FreeLoader
 *  Copyright (C) 2011 Timo Kreuzer (timo.kreuzer@reactos.org)
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

#include <freeldr.h>
#include <debug.h>

DBG_DEFAULT_CHANNEL(HEAP);

#define DEFAULT_HEAP_SIZE (1024 * 1024)
#define TEMP_HEAP_SIZE (1024 * 1024)

PVOID FrLdrDefaultHeap;
PVOID FrLdrTempHeap;

typedef struct _BLOCK_DATA
{
    ULONG_PTR Flink;
    ULONG_PTR Blink;
} BLOCK_DATA, *PBLOCK_DATA;

typedef struct _HEAP_BLOCK
{
    USHORT Size;
    USHORT PreviousSize;
    ULONG Tag;
    BLOCK_DATA Data[];
} HEAP_BLOCK, *PHEAP_BLOCK;

typedef struct _HEAP
{
    SIZE_T MaximumSize;
    SIZE_T CurrentAllocBytes;
    SIZE_T MaxAllocBytes;
    ULONG NumAllocs;
    ULONG NumFrees;
    SIZE_T LargestAllocation;
    ULONGLONG AllocationTime;
    ULONGLONG FreeTime;
    ULONG_PTR TerminatingBlock;
    HEAP_BLOCK Blocks;
} HEAP, *PHEAP;

PVOID
HeapCreate(
    SIZE_T MaximumSize,
    TYPE_OF_MEMORY MemoryType)
{
    PHEAP Heap;
    PHEAP_BLOCK Block;
    SIZE_T Remaining;
    USHORT PreviousSize;
    TRACE("HeapCreate(MemoryType=%ld)\n", MemoryType);

    /* Allocate some memory for the heap */
    MaximumSize = ALIGN_UP_BY(MaximumSize, MM_PAGE_SIZE);
    Heap = MmAllocateMemoryWithType(MaximumSize, MemoryType);
    if (!Heap)
    {
        ERR("HEAP: Failed to allocate heap of size 0x%lx, Type\n",
            MaximumSize, MemoryType);
        return NULL;
    }

    /* Initialize the heap header */
    Heap->MaximumSize = MaximumSize;
    Heap->CurrentAllocBytes = 0;
    Heap->MaxAllocBytes = 0;
    Heap->NumAllocs = 0;
    Heap->NumFrees = 0;
    Heap->LargestAllocation = 0;

    /* Calculate what's left to process */
    Remaining = (MaximumSize - sizeof(HEAP)) / sizeof(HEAP_BLOCK);
    TRACE("Remaining = %ld\n", Remaining);

    /* Substract 2 for the terminating entry (header + free entry) */
    Remaining -= 2;

    Block = &Heap->Blocks;
    PreviousSize = 0;

    /* Create free blocks */
    while (Remaining > 1)
    {
        /* Initialize this free block */
        Block->Size = (USHORT)min(MAXUSHORT, Remaining - 1);
        Block->PreviousSize = PreviousSize;
        Block->Tag = 0;
        Block->Data[0].Flink = (Block - &Heap->Blocks) + Block->Size + 1;
        Block->Data[0].Blink = (Block - &Heap->Blocks) - 1 - PreviousSize;

        /* Substract current block size from remainder */
        Remaining -= (Block->Size + 1);

        /* Go to next block */
        PreviousSize = Block->Size;
        Block = Block + Block->Size + 1;

        TRACE("Remaining = %ld\n", Remaining);
    }

    /* Now finish with a terminating block */
    Heap->TerminatingBlock = Block - &Heap->Blocks;
    Block->Size = 0;
    Block->PreviousSize = PreviousSize;
    Block->Tag = 'dnE#';
    Block->Data[0].Flink = 0;
    Block->Data[0].Blink = (Block - &Heap->Blocks) - 1 - PreviousSize;
    Heap->Blocks.Data[0].Blink = Heap->TerminatingBlock;

    return Heap;
}

VOID
HeapDestroy(
    PVOID HeapHandle)
{
    PHEAP Heap = HeapHandle;

    /* Mark all pages as firmware temporary, so they are free for the kernel */
    MmMarkPagesInLookupTable(PageLookupTableAddress,
                             (ULONG_PTR)Heap / MM_PAGE_SIZE,
                             (PFN_COUNT)(Heap->MaximumSize / MM_PAGE_SIZE),
                             LoaderFirmwareTemporary);
}

VOID
HeapRelease(
    PVOID HeapHandle)
{
    PHEAP Heap = HeapHandle;
    PHEAP_BLOCK Block;
    PUCHAR StartAddress, EndAddress;
    PFN_COUNT FreePages, AllFreePages = 0;
    TRACE("HeapRelease(%p)\n", HeapHandle);

    /* Loop all heap chunks */
    for (Block = &Heap->Blocks;
         Block->Size != 0;
         Block = Block + 1 + Block->Size)
    {
        /* Continue, if its not free */
        if (Block->Tag != 0) continue;

        /* Calculate page aligned start address of the free region */
        StartAddress = ALIGN_UP_POINTER_BY(Block->Data, PAGE_SIZE);

        /* Walk over adjacent free blocks */
        while (Block->Tag == 0) Block = Block + Block->Size + 1;

        /* Check if this was the last block */
        if (Block->Size == 0)
        {
            /* Align the end address up to cover the end of the heap */
            EndAddress = ALIGN_UP_POINTER_BY(Block->Data, PAGE_SIZE);
        }
        else
        {
            /* Align the end address down to not cover any allocations */
            EndAddress = ALIGN_DOWN_POINTER_BY(Block->Data, PAGE_SIZE);
        }

        /* Check if we have free pages */
        if (EndAddress > StartAddress)
        {
            /* Calculate the size of the free region in pages */
            FreePages = (PFN_COUNT)((EndAddress - StartAddress) / MM_PAGE_SIZE);
            AllFreePages += FreePages;

            /* Now mark the pages free */
            MmMarkPagesInLookupTable(PageLookupTableAddress,
                                     (ULONG_PTR)StartAddress / MM_PAGE_SIZE,
                                     FreePages,
                                     LoaderFree);
        }

        /* bail out, if it was the last block */
        if (Block->Size == 0) break;
    }

    ERR("HeapRelease() done, freed %ld pages\n", AllFreePages);
}

VOID
HeapCleanupAll(VOID)
{
    PHEAP Heap;

    Heap = FrLdrDefaultHeap;
    ERR("Heap statistics for default heap:\n"
          "CurrentAlloc=0x%lx, MaxAlloc=0x%lx, LargestAllocation=0x%lx\n"
          "NumAllocs=%ld, NumFrees=%ld\n",
          Heap->CurrentAllocBytes, Heap->MaxAllocBytes, Heap->LargestAllocation,
          Heap->NumAllocs, Heap->NumFrees);
    ERR("AllocTime = %I64d, FreeTime = %I64d, sum = %I64d\n",
        Heap->AllocationTime, Heap->FreeTime, Heap->AllocationTime + Heap->FreeTime);


    /* Release fre pages */
    HeapRelease(FrLdrDefaultHeap);

    Heap = FrLdrTempHeap;
    ERR("Heap statistics for temp heap:\n"
          "CurrentAlloc=0x%lx, MaxAlloc=0x%lx, LargestAllocation=0x%lx\n"
          "NumAllocs=%ld, NumFrees=%ld\n",
          Heap->CurrentAllocBytes, Heap->MaxAllocBytes, Heap->LargestAllocation,
          Heap->NumAllocs, Heap->NumFrees);

    /* Destroy the heap */
    HeapDestroy(FrLdrTempHeap);
}

static VOID
HeapRemoveFreeList(
    PHEAP Heap,
    PHEAP_BLOCK Block)
{
    PHEAP_BLOCK Previous, Next;

    Next = &Heap->Blocks + Block->Data[0].Flink;
    Previous = &Heap->Blocks + Block->Data[0].Blink;
    ASSERT((Next->Tag == 0) || (Next->Tag == 'dnE#'));
    ASSERT(Next->Data[0].Blink == Block - &Heap->Blocks);
    ASSERT((Previous->Tag == 0) || (Previous->Tag == 'dnE#'));
    ASSERT(Previous->Data[0].Flink == Block - &Heap->Blocks);

    Next->Data[0].Blink = Previous - &Heap->Blocks;
    Previous->Data[0].Flink = Next - &Heap->Blocks;
}

static VOID
HeapInsertFreeList(
    PHEAP Heap,
    PHEAP_BLOCK FreeBlock)
{
    PHEAP_BLOCK ListHead, NextBlock;
    ASSERT(FreeBlock->Tag == 0);

    /* Terminating block serves as free list head */
    ListHead = &Heap->Blocks + Heap->TerminatingBlock;

    for (NextBlock = &Heap->Blocks + ListHead->Data[0].Flink;
         NextBlock < FreeBlock;
         NextBlock = &Heap->Blocks + NextBlock->Data[0].Flink);

    FreeBlock->Data[0].Flink = NextBlock - &Heap->Blocks;
    FreeBlock->Data[0].Blink = NextBlock->Data[0].Blink;
    NextBlock->Data[0].Blink = FreeBlock - &Heap->Blocks;
    NextBlock = &Heap->Blocks + FreeBlock->Data[0].Blink;
    NextBlock->Data[0].Flink = FreeBlock - &Heap->Blocks;
}

PVOID
HeapAllocate(
    PVOID HeapHandle,
    SIZE_T ByteSize,
    ULONG Tag)
{
    PHEAP Heap = HeapHandle;
    PHEAP_BLOCK Block, NextBlock;
    USHORT BlockSize, Remaining;
    ULONGLONG Time = __rdtsc();

    /* Check if the allocation is too large */
    if ((ByteSize +  sizeof(HEAP_BLOCK)) > MAXUSHORT * sizeof(HEAP_BLOCK))
    {
        ERR("HEAP: Allocation of 0x%lx bytes too large\n", ByteSize);
        return NULL;
    }

    /* We need a proper tag */
    if (Tag == 0) Tag = 'enoN';

    /* Calculate alloc size */
    BlockSize = (USHORT)((ByteSize + sizeof(HEAP_BLOCK) - 1) / sizeof(HEAP_BLOCK));

    /* Walk the free block list */
    Block = &Heap->Blocks + Heap->TerminatingBlock;
    for (Block = &Heap->Blocks + Block->Data[0].Flink;
         Block->Size != 0;
         Block = &Heap->Blocks + Block->Data[0].Flink)
    {
        ASSERT(Block->Tag == 0);

        /* Continue, if its too small */
        if (Block->Size < BlockSize) continue;

        /* This block is just fine, use it */
        Block->Tag = Tag;

        /* Remove this entry from the free list */
        HeapRemoveFreeList(Heap, Block);

        /* Calculate the remaining size */
        Remaining = Block->Size - BlockSize;

        /* Check if the remaining space is large enough for a new block */
        if (Remaining > 1)
        {
            /* Make the allocated block as large as neccessary */
            Block->Size = BlockSize;

            /* Get pointer to the new block */
            NextBlock = Block + 1 + BlockSize;

            /* Make it a free block */
            NextBlock->Tag = 0;
            NextBlock->Size = Remaining - 1;
            NextBlock->PreviousSize = BlockSize;
            BlockSize = NextBlock->Size;
            HeapInsertFreeList(Heap, NextBlock);

            /* Advance to the next block */
            NextBlock = NextBlock + 1 + BlockSize;
        }
        else
        {
            /* Not enough left, use the full block */
            BlockSize = Block->Size;

            /* Get the next block */
            NextBlock = Block + 1 + BlockSize;
        }

        /* Update the next blocks back link */
        NextBlock->PreviousSize = BlockSize;

        /* Update heap usage */
        Heap->NumAllocs++;
        Heap->CurrentAllocBytes += Block->Size * sizeof(HEAP_BLOCK);
        Heap->MaxAllocBytes = max(Heap->MaxAllocBytes, Heap->CurrentAllocBytes);
        Heap->LargestAllocation = max(Heap->LargestAllocation,
                                      Block->Size * sizeof(HEAP_BLOCK));
        Heap->AllocationTime += (__rdtsc() - Time);

        TRACE("HeapAllocate(%p, %ld, %.4s) -> return %p\n",
              HeapHandle, ByteSize, &Tag, Block->Data);

        /* Return pointer to the data */
        return Block->Data;
    }

    /* We found nothing */
    WARN("HEAP: nothing suitable found for 0x%lx bytes\n", ByteSize);
    return NULL;
}

VOID
HeapFree(
    PVOID HeapHandle,
    PVOID Pointer,
    ULONG Tag)
{
    PHEAP Heap = HeapHandle;
    PHEAP_BLOCK Block, PrevBlock, NextBlock;
    ULONGLONG Time = __rdtsc();
    TRACE("HeapFree(%p, %p)\n", HeapHandle, Pointer);
    ASSERT(Tag != 'dnE#');

    /* Check if the block is really inside this heap */
    if ((Pointer < (PVOID)(Heap + 1)) ||
        (Pointer > (PVOID)((PUCHAR)Heap + Heap->MaximumSize)))
    {
        ERR("HEAP: trying to free %p outside of heap %p\n", Pointer, Heap);
        ASSERT(FALSE);
    }

    Block = ((PHEAP_BLOCK)Pointer) - 1;

    /* Check if the tag matches */
    if ((Tag && (Block->Tag != Tag)) || (Block->Tag == 0))
    {
        ERR("HEAP: Bad tag! Pointer=%p: block tag '%.4s', requested '%.4s', size=0x%lx\n",
            Pointer, &Block->Tag, &Tag, Block->Size);
        ASSERT(FALSE);
    }

    /* Mark as free */
    Block->Tag = 0;

    /* Update heap usage */
    Heap->NumFrees++;
    Heap->CurrentAllocBytes -= Block->Size * sizeof(HEAP_BLOCK);

    /* Get pointers to the next and previous block */
    PrevBlock = Block - Block->PreviousSize - 1;
    NextBlock = Block + Block->Size + 1;

    /* Check if next block is free */
    if ((NextBlock->Tag == 0) &&
        ((Block->Size + NextBlock->Size + 1) <= MAXUSHORT))
    {
        /* Merge next block into current */
        Block->Size += NextBlock->Size + 1;
        HeapRemoveFreeList(Heap, NextBlock);

        NextBlock = Block + Block->Size + 1;
    }

    /* Check if there is a block before and it's free */
    if ((Block->PreviousSize != 0) && (PrevBlock->Tag == 0) &&
        ((PrevBlock->Size + Block->Size + 1) <= MAXUSHORT))
    {
        /* Merge current block into previous */
        PrevBlock->Size += Block->Size + 1;
        Block = PrevBlock;
    }
    else
    {
        /* Insert the entry into the free list */
        HeapInsertFreeList(Heap, Block);
    }

    /* Update the next block's back link */
    NextBlock->PreviousSize = Block->Size;

    Heap->FreeTime += (__rdtsc() - Time);
}


/* Wrapper functions *********************************************************/

VOID
MmInitializeHeap(PVOID PageLookupTable)
{
    TRACE("MmInitializeHeap()\n");

    /* Create the default heap */
    FrLdrDefaultHeap = HeapCreate(DEFAULT_HEAP_SIZE, LoaderOsloaderHeap);
    ASSERT(FrLdrDefaultHeap);

    /* Create a temporary heap */
    FrLdrTempHeap = HeapCreate(TEMP_HEAP_SIZE, LoaderFirmwareTemporary);
    ASSERT(FrLdrTempHeap);

    TRACE("MmInitializeHeap() done, default heap %p, temp heap %p\n",
          FrLdrDefaultHeap, FrLdrTempHeap);
}

PVOID
MmHeapAlloc(SIZE_T MemorySize)
{
    return HeapAllocate(FrLdrDefaultHeap, MemorySize, 'pHmM');
}

VOID
MmHeapFree(PVOID MemoryPointer)
{
    HeapFree(FrLdrDefaultHeap, MemoryPointer, 'pHmM');
}

PVOID
NTAPI
ExAllocatePoolWithTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag)
{
    return HeapAllocate(FrLdrDefaultHeap, NumberOfBytes, Tag);
}

PVOID
NTAPI
ExAllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes)
{
    return HeapAllocate(FrLdrDefaultHeap, NumberOfBytes, 0);
}

VOID
NTAPI
ExFreePool(
    IN PVOID P)
{
    HeapFree(FrLdrDefaultHeap, P, 0);
}

VOID
NTAPI
ExFreePoolWithTag(
  IN PVOID P,
  IN ULONG Tag)
{
    HeapFree(FrLdrDefaultHeap, P, Tag);
}

PVOID
NTAPI
RtlAllocateHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN SIZE_T Size)
{
    PVOID ptr;

    ptr = HeapAllocate(FrLdrDefaultHeap, Size, ' ltR');
    if (ptr && (Flags & HEAP_ZERO_MEMORY))
    {
        RtlZeroMemory(ptr, Size);
    }

    return ptr;
}

BOOLEAN
NTAPI
RtlFreeHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID HeapBase)
{
    HeapFree(FrLdrDefaultHeap, HeapBase, ' ltR');
    return TRUE;
}


/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/heappage.c
 * PURPOSE:         RTL Page Heap implementation
 * PROGRAMMERS:     Copyright 2011 Aleksey Bragin
 */

/* Useful references:
    http://msdn.microsoft.com/en-us/library/ms220938(VS.80).aspx
    http://blogs.msdn.com/b/jiangyue/archive/2010/03/16/windows-heap-overrun-monitoring.aspx
*/

/* INCLUDES *****************************************************************/

#include <rtl.h>
#include <heap.h>

#define NDEBUG
#include <debug.h>

/* TYPES **********************************************************************/

typedef struct _DPH_BLOCK_INFORMATION
{
     ULONG StartStamp;
     PVOID Heap;
     ULONG RequestedSize;
     ULONG ActualSize;
     union
     {
          LIST_ENTRY FreeQueue;
          SINGLE_LIST_ENTRY FreePushList;
          WORD TraceIndex;
     };
     PVOID StackTrace;
     ULONG EndStamp;
} DPH_BLOCK_INFORMATION, *PDPH_BLOCK_INFORMATION;

typedef struct _DPH_HEAP_BLOCK
{
     union
     {
          struct _DPH_HEAP_BLOCK *pNextAlloc;
          LIST_ENTRY AvailableEntry;
          RTL_BALANCED_LINKS TableLinks;
     };
     PUCHAR pUserAllocation;
     PUCHAR pVirtualBlock;
     ULONG nVirtualBlockSize;
     ULONG nVirtualAccessSize;
     ULONG nUserRequestedSize;
     ULONG nUserActualSize;
     PVOID UserValue;
     ULONG UserFlags;
     PRTL_TRACE_BLOCK StackTrace;
     LIST_ENTRY AdjacencyEntry;
     PUCHAR pVirtualRegion;
} DPH_HEAP_BLOCK, *PDPH_HEAP_BLOCK;

typedef struct _DPH_HEAP_ROOT
{
     ULONG Signature;
     ULONG HeapFlags;
     PRTL_CRITICAL_SECTION HeapCritSect;
     ULONG nRemoteLockAcquired;

     PDPH_HEAP_BLOCK pVirtualStorageListHead;
     PDPH_HEAP_BLOCK pVirtualStorageListTail;
     ULONG nVirtualStorageRanges;
     ULONG nVirtualStorageBytes;

     RTL_AVL_TABLE BusyNodesTable;
     PDPH_HEAP_BLOCK NodeToAllocate;
     ULONG nBusyAllocations;
     ULONG nBusyAllocationBytesCommitted;

     PDPH_HEAP_BLOCK pFreeAllocationListHead;
     PDPH_HEAP_BLOCK pFreeAllocationListTail;
     ULONG nFreeAllocations;
     ULONG nFreeAllocationBytesCommitted;

     LIST_ENTRY AvailableAllocationHead;
     ULONG nAvailableAllocations;
     ULONG nAvailableAllocationBytesCommitted;

     PDPH_HEAP_BLOCK pUnusedNodeListHead;
     PDPH_HEAP_BLOCK pUnusedNodeListTail;
     ULONG nUnusedNodes;
     ULONG nBusyAllocationBytesAccessible;
     PDPH_HEAP_BLOCK pNodePoolListHead;
     PDPH_HEAP_BLOCK pNodePoolListTail;
     ULONG nNodePools;
     ULONG nNodePoolBytes;

     LIST_ENTRY NextHeap;
     ULONG ExtraFlags;
     ULONG Seed;
     PVOID NormalHeap;
     PRTL_TRACE_BLOCK CreateStackTrace;
     PVOID FirstThread;
} DPH_HEAP_ROOT, *PDPH_HEAP_ROOT;

/* GLOBALS ********************************************************************/

BOOLEAN RtlpPageHeapEnabled = FALSE;
ULONG RtlpDphGlobalFlags;
ULONG RtlpPageHeapSizeRangeStart, RtlpPageHeapSizeRangeEnd;
ULONG RtlpPageHeapDllRangeStart, RtlpPageHeapDllRangeEnd;
WCHAR RtlpDphTargetDlls[512];

ULONG RtlpDphBreakOptions;
ULONG RtlpDphDebugOptions;

LIST_ENTRY RtlpDphPageHeapList;
BOOLEAN RtlpDphPageHeapListInitialized;
RTL_CRITICAL_SECTION RtlpDphPageHeapListLock;
ULONG RtlpDphPageHeapListLength;
UNICODE_STRING RtlpDphTargetDllsUnicode;

RTL_CRITICAL_SECTION RtlpDphDelayedFreeQueueLock;
LIST_ENTRY RtlpDphDelayedFreeQueue;
SLIST_HEADER RtlpDphDelayedTemporaryPushList;
ULONG RtlpDphMemoryUsedByDelayedFreeBlocks;
ULONG RtlpDphNumberOfDelayedFreeBlocks;

/* Counters */
LONG RtlpDphCounter;
LONG RtlpDphAllocFails;
LONG RtlpDphReleaseFails;
LONG RtlpDphFreeFails;
LONG RtlpDphProtectFails;

#define DPH_RESERVE_SIZE 0x100000
#define DPH_POOL_SIZE 0x4000
#define DPH_FREE_LIST_MINIMUM 8

/* RtlpDphBreakOptions */
#define DPH_BREAK_ON_RESERVE_FAIL 0x01
#define DPH_BREAK_ON_COMMIT_FAIL  0x02
#define DPH_BREAK_ON_RELEASE_FAIL 0x04
#define DPH_BREAK_ON_FREE_FAIL    0x08
#define DPH_BREAK_ON_PROTECT_FAIL 0x10

/* RtlpDphDebugOptions */
#define DPH_DEBUG_INTERNAL_VALIDATE 0x01
#define DPH_DEBUG_VERBOSE           0x04

/* DPH ExtraFlags */
#define DPH_EXTRA_CHECK_CORRUPTED_BLOCKS 0x10

/* Fillers */
#define DPH_FILL 0xEEEEEEEE
#define DPH_FILL_START_STAMP_1 0xABCDBBBB
#define DPH_FILL_START_STAMP_2 0xABCDBBBA
#define DPH_FILL_END_STAMP_1   0xDCBABBBB
#define DPH_FILL_BLOCK_END     0xD0

/* Validation info flags */
#define DPH_VALINFO_BAD_START_STAMP      0x01
#define DPH_VALINFO_BAD_END_STAMP        0x02
#define DPH_VALINFO_BAD_POINTER          0x04
#define DPH_VALINFO_BAD_PREFIX_PATTERN   0x08
#define DPH_VALINFO_BAD_SUFFIX_PATTERN   0x10
#define DPH_VALINFO_EXCEPTION            0x20
#define DPH_VALINFO_1                    0x40
#define DPH_VALINFO_BAD_INFIX_PATTERN    0x80
#define DPH_VALINFO_ALREADY_FREED        0x100
#define DPH_VALINFO_CORRUPTED_AFTER_FREE 0x200

/* Signatures */
#define DPH_SIGNATURE 0xFFEEDDCC

/* Biased pointer macros */
#define IS_BIASED_POINTER(ptr) ((ULONG_PTR)(ptr) & 1)
#define POINTER_REMOVE_BIAS(ptr) ((ULONG_PTR)(ptr) & ~(ULONG_PTR)1)
#define POINTER_ADD_BIAS(ptr) ((ULONG_PTR)(ptr) & 1)

/* FUNCTIONS ******************************************************************/

BOOLEAN NTAPI
RtlpDphGrowVirtual(PDPH_HEAP_ROOT DphRoot, SIZE_T Size);

BOOLEAN NTAPI
RtlpDphIsNormalFreeHeapBlock(PVOID Block, PULONG ValidationInformation, BOOLEAN CheckFillers);

VOID NTAPI
RtlpDphReportCorruptedBlock(PDPH_HEAP_ROOT DphRoot, ULONG Reserved, PVOID Block, ULONG ValidationInfo);


PVOID NTAPI
RtlpDphPointerFromHandle(PVOID Handle)
{
    PHEAP NormalHeap = (PHEAP)Handle;
    PDPH_HEAP_ROOT DphHeap = (PDPH_HEAP_ROOT)((PUCHAR)Handle + PAGE_SIZE);

    if (NormalHeap->ForceFlags & HEAP_FLAG_PAGE_ALLOCS)
    {
        if (DphHeap->Signature == DPH_SIGNATURE)
            return DphHeap;
    }

    DPRINT1("heap handle with incorrect signature\n");
    DbgBreakPoint();
    return NULL;
}

VOID NTAPI
RtlpDphEnterCriticalSection(PDPH_HEAP_ROOT DphRoot, ULONG Flags)
{
    if (Flags & HEAP_NO_SERIALIZE)
    {
        /* More complex scenario */
        if (!RtlTryEnterCriticalSection(DphRoot->HeapCritSect))
        {
            if (!DphRoot->nRemoteLockAcquired)
            {
                DPRINT1("multithreaded access in HEAP_NO_SERIALIZE heap\n");
                DbgBreakPoint();

                /* Clear out the no serialize flag */
                DphRoot->HeapFlags &= ~HEAP_NO_SERIALIZE;
            }

            /* Enter the heap's critical section */
            RtlEnterCriticalSection(DphRoot->HeapCritSect);
        }
    }
    else
    {
        /* Just enter the heap's critical section */
        RtlEnterCriticalSection(DphRoot->HeapCritSect);
    }
}

VOID NTAPI
RtlpDphLeaveCriticalSection(PDPH_HEAP_ROOT DphRoot)
{
    /* Just leave the heap's critical section */
    RtlLeaveCriticalSection(DphRoot->HeapCritSect);
}


VOID NTAPI
RtlpDphPreProcessing(PDPH_HEAP_ROOT DphRoot, ULONG Flags)
{
    RtlpDphEnterCriticalSection(DphRoot, Flags);

    /* FIXME: Validate integrity, internal lists if necessary */
}

NTSTATUS NTAPI
RtlpSecMemFreeVirtualMemory(HANDLE Process, PVOID *Base, PSIZE_T Size, ULONG Type)
{
    NTSTATUS Status;
    //PVOID *SavedBase = Base;
    //PSIZE_T SavedSize = Size;

    /* Free the memory */
    Status = ZwFreeVirtualMemory(Process, Base, Size, Type);

    /* Flush secure memory cache if needed and retry freeing */
#if 0
    if (Status == STATUS_INVALID_PAGE_PROTECTION &&
        Process == NtCurrentProcess() &&
        RtlFlushSecureMemoryCache(*SavedBase, *SavedSize))
    {
        Status = ZwFreeVirtualMemory(NtCurrentProcess(), SavedBase, SavedSize, Type);
    }
#endif

    return Status;
}

NTSTATUS NTAPI
RtlpDphAllocateVm(PVOID *Base, SIZE_T Size, ULONG Type, ULONG Protection)
{
    NTSTATUS Status;
    Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                     Base,
                                     0,
                                     &Size,
                                     Type,
                                     Protection);

    /* Check for failures */
    if (!NT_SUCCESS(Status))
    {
        if (Type == MEM_RESERVE)
        {
            _InterlockedIncrement(&RtlpDphCounter);
            if (RtlpDphBreakOptions & DPH_BREAK_ON_RESERVE_FAIL)
            {
                DPRINT1("Page heap: AllocVm (%p, %p, %x) failed with %x \n", Base, Size, Type, Status);
                DbgBreakPoint();
                return Status;
            }
        }
        else
        {
            _InterlockedIncrement(&RtlpDphAllocFails);
            if (RtlpDphBreakOptions & DPH_BREAK_ON_COMMIT_FAIL)
            {
                DPRINT1("Page heap: AllocVm (%p, %p, %x) failed with %x \n", Base, Size, Type, Status);
                DbgBreakPoint();
                return Status;
            }
        }
    }

    return Status;
}

NTSTATUS NTAPI
RtlpDphFreeVm(PVOID Base, SIZE_T Size, ULONG Type)
{
    NTSTATUS Status;

    /* Free the memory */
    Status = RtlpSecMemFreeVirtualMemory(NtCurrentProcess(), &Base, &Size, Type);

    /* Log/report failures */
    if (!NT_SUCCESS(Status))
    {
        if (Type == MEM_RELEASE)
        {
            _InterlockedIncrement(&RtlpDphReleaseFails);
            if (RtlpDphBreakOptions & DPH_BREAK_ON_RELEASE_FAIL)
            {
                DPRINT1("Page heap: FreeVm (%p, %p, %x) failed with %x \n", Base, Size, Type, Status);
                DbgBreakPoint();
                return Status;
            }
        }
        else
        {
            _InterlockedIncrement(&RtlpDphFreeFails);
            if (RtlpDphBreakOptions & DPH_BREAK_ON_FREE_FAIL)
            {
                DPRINT1("Page heap: FreeVm (%p, %p, %x) failed with %x \n", Base, Size, Type, Status);
                DbgBreakPoint();
                return Status;
            }
        }
    }

    return Status;
}

NTSTATUS NTAPI
RtlpDphProtectVm(PVOID Base, SIZE_T Size, ULONG Protection)
{
    NTSTATUS Status;
    ULONG OldProtection;

    /* Change protection */
    Status = ZwProtectVirtualMemory(NtCurrentProcess(), &Base, &Size, Protection, &OldProtection);

    /* Log/report failures */
    if (!NT_SUCCESS(Status))
    {
        _InterlockedIncrement(&RtlpDphProtectFails);
        if (RtlpDphBreakOptions & DPH_BREAK_ON_PROTECT_FAIL)
        {
            DPRINT1("Page heap: ProtectVm (%p, %p, %x) failed with %x \n", Base, Size, Protection, Status);
            DbgBreakPoint();
            return Status;
        }
    }

    return Status;
}

VOID NTAPI
RtlpDphPlaceOnPoolList(PDPH_HEAP_ROOT DphRoot, PDPH_HEAP_BLOCK DphNode)
{
    /* DphNode is being added to the tail of the list */
    DphNode->pNextAlloc = NULL;

    /* Add it to the tail of the linked list */
    if (DphRoot->pNodePoolListTail)
        DphRoot->pNodePoolListTail->pNextAlloc = DphNode;
    else
        DphRoot->pNodePoolListHead = DphNode;
    DphRoot->pNodePoolListTail = DphNode;

    /* Update byte counts taking in account this new node */
    DphRoot->nNodePools++;
    DphRoot->nNodePoolBytes += DphNode->nVirtualBlockSize;
}

VOID NTAPI
RtlpDphPlaceOnVirtualList(PDPH_HEAP_ROOT DphRoot, PDPH_HEAP_BLOCK DphNode)
{
    /* Add it to the head of the virtual list */
    DphNode->pNextAlloc = DphRoot->pVirtualStorageListHead;
    if (!DphRoot->pVirtualStorageListHead)
        DphRoot->pVirtualStorageListTail = DphNode;
    DphRoot->pVirtualStorageListHead = DphNode;

    /* Update byte counts taking in account this new node */
    DphRoot->nVirtualStorageRanges++;
    DphRoot->nVirtualStorageBytes += DphNode->nVirtualBlockSize;
}

PDPH_HEAP_BLOCK NTAPI
RtlpDphTakeNodeFromUnusedList(PDPH_HEAP_ROOT DphRoot)
{
    PDPH_HEAP_BLOCK Node = DphRoot->pUnusedNodeListHead;
    PDPH_HEAP_BLOCK Next;

    /* Take the first entry */
    if (!Node) return NULL;

    /* Remove that entry (Node) from the list */
    Next = Node->pNextAlloc;
    if (DphRoot->pUnusedNodeListHead == Node) DphRoot->pUnusedNodeListHead = Next;
    if (DphRoot->pUnusedNodeListTail == Node) DphRoot->pUnusedNodeListTail = NULL;

    /* Decrease amount of unused nodes */
    DphRoot->nUnusedNodes--;

    return Node;
}

VOID NTAPI
RtlpDphReturnNodeToUnusedList(PDPH_HEAP_ROOT DphRoot,
                              PDPH_HEAP_BLOCK Node)
{
    /* Add it back to the head of the unused list */
    Node->pNextAlloc = DphRoot->pUnusedNodeListHead;
    if (!DphRoot->pUnusedNodeListHead) DphRoot->pUnusedNodeListTail = Node;
    DphRoot->pUnusedNodeListHead = Node;

    /* Increase amount of unused nodes */
    DphRoot->nUnusedNodes++;
}

VOID NTAPI
RtlpDphRemoveFromAvailableList(PDPH_HEAP_ROOT DphRoot,
                               PDPH_HEAP_BLOCK Node)
{
    /* Make sure Adjacency list pointers are biased */
    ASSERT(IS_BIASED_POINTER(Node->AdjacencyEntry.Flink));
    ASSERT(IS_BIASED_POINTER(Node->AdjacencyEntry.Blink));

    /* Remove it from the list */
    RemoveEntryList(&Node->AvailableEntry);

    /* Decrease heap counters */
    DphRoot->nAvailableAllocations--;
    DphRoot->nAvailableAllocationBytesCommitted -= Node->nVirtualBlockSize;

    /* Remove bias from the AdjacencyEntry pointer */
    POINTER_REMOVE_BIAS(Node->AdjacencyEntry.Flink);
    POINTER_REMOVE_BIAS(Node->AdjacencyEntry.Blink);
}

VOID NTAPI
RtlpDphRemoveFromFreeList(PDPH_HEAP_ROOT DphRoot,
                          PDPH_HEAP_BLOCK Node,
                          PDPH_HEAP_BLOCK Prev)
{
    PDPH_HEAP_BLOCK Next;

    /* Detach it from the list */
    Next = Node->pNextAlloc;
    if (DphRoot->pFreeAllocationListHead == Node)
        DphRoot->pFreeAllocationListHead = Next;
    if (DphRoot->pFreeAllocationListTail == Node)
        DphRoot->pFreeAllocationListTail = Prev;
    if (Prev) Prev->pNextAlloc = Next;

    /* Decrease heap counters */
    DphRoot->nFreeAllocations--;
    DphRoot->nFreeAllocationBytesCommitted -= Node->nVirtualBlockSize;

    Node->StackTrace = NULL;
}

VOID NTAPI
RtlpDphCoalesceNodeIntoAvailable(PDPH_HEAP_ROOT DphRoot,
                                 PDPH_HEAP_BLOCK Node)
{
    PDPH_HEAP_BLOCK NodeEntry, PrevNode = NULL, NextNode;
    PLIST_ENTRY AvailListHead;
    PLIST_ENTRY CurEntry;

    /* Update heap counters */
    DphRoot->nAvailableAllocationBytesCommitted += Node->nVirtualBlockSize;
    DphRoot->nAvailableAllocations++;

    /* Find where to put this node according to its virtual address */
    AvailListHead = &DphRoot->AvailableAllocationHead;
    CurEntry = AvailListHead->Flink;

    while (CurEntry != AvailListHead)
    {
        NodeEntry = CONTAINING_RECORD(CurEntry, DPH_HEAP_BLOCK, AvailableEntry);

        if (NodeEntry->pVirtualBlock >= Node->pVirtualBlock)
        {
            PrevNode = NodeEntry;
            break;
        }
        CurEntry = CurEntry->Flink;
    }

    /* Did we find a node to insert our one after? */
    if (!PrevNode)
    {
        /* No, just add to the head of the list then */
        InsertHeadList(AvailListHead, &Node->AvailableEntry);
    }
    else
    {
        /* Check the previous node and merge if possible */
        if (PrevNode->pVirtualBlock + PrevNode->nVirtualBlockSize == Node->pVirtualBlock)
        {
            /* They are adjacent - merge! */
            PrevNode->nVirtualBlockSize += Node->nVirtualBlockSize;
            RtlpDphReturnNodeToUnusedList(DphRoot, Node);
            DphRoot->nAvailableAllocations--;

            Node = PrevNode;
        }
        else
        {
            /* Insert after PrevNode */
            InsertTailList(&PrevNode->AvailableEntry, &Node->AvailableEntry);
        }
    }

    /* Now check the next entry after our one */
    if (Node->AvailableEntry.Flink != AvailListHead)
    {
        NextNode = CONTAINING_RECORD(Node->AvailableEntry.Flink, DPH_HEAP_BLOCK, AvailableEntry);;
        /* Node is not at the tail of the list, check if it's adjacent */
        if (Node->pVirtualBlock + Node->nVirtualBlockSize == NextNode->pVirtualBlock)
        {
            /* They are adjacent - merge! */
            Node->nVirtualBlockSize += NextNode->nVirtualBlockSize;
            Node->pNextAlloc = NextNode->pNextAlloc;
            RtlpDphReturnNodeToUnusedList(DphRoot, NextNode);
            DphRoot->nAvailableAllocations--;
        }
    }
}

VOID NTAPI
RtlpDphCoalesceFreeIntoAvailable(PDPH_HEAP_ROOT DphRoot,
                                 ULONG LeaveOnFreeList)
{
    PDPH_HEAP_BLOCK Node = DphRoot->pFreeAllocationListHead, Next;
    SIZE_T FreeAllocations = DphRoot->nFreeAllocations;

    /* Make sure requested size is not too big */
    ASSERT(FreeAllocations >= LeaveOnFreeList);

    while (Node)
    {
        FreeAllocations--;
        if (FreeAllocations <= LeaveOnFreeList) break;

        /* Get the next pointer, because it may be changed after following two calls */
        Next = Node->pNextAlloc;

        /* Remove it from the free list */
        RtlpDphRemoveFromFreeList(DphRoot, Node, NULL);

        /* And put into the available */
        RtlpDphCoalesceNodeIntoAvailable(DphRoot, Node);

        Node = Next;
    }
}

VOID NTAPI
RtlpDphAddNewPool(PDPH_HEAP_ROOT DphRoot, PDPH_HEAP_BLOCK NodeBlock, PVOID Virtual, SIZE_T Size, BOOLEAN PlaceOnPool)
{
    PDPH_HEAP_BLOCK DphNode, DphStartNode;
    ULONG NodeCount;

    NodeCount = (Size >> 6) - 1;
    DphStartNode = Virtual;

    /* Set pNextAlloc for all blocks */
    for (DphNode = Virtual; NodeCount > 0; DphNode++, NodeCount--)
        DphNode->pNextAlloc = DphNode + 1;

    /* and the last one */
    DphNode->pNextAlloc = NULL;

    /* Add it to the tail of unused node list */
    if (DphRoot->pUnusedNodeListTail)
        DphRoot->pUnusedNodeListTail->pNextAlloc = DphStartNode;
    else
        DphRoot->pUnusedNodeListHead = DphStartNode;

    DphRoot->pUnusedNodeListTail = DphNode;

    /* Increase counters */
    DphRoot->nUnusedNodes += NodeCount;

    /* Check if we need to place it on the pool list */
    if (PlaceOnPool)
    {
        /* Get a node from the unused list */
        DphNode = RtlpDphTakeNodeFromUnusedList(DphRoot);
        ASSERT(DphNode);

        /* Set its virtual block values */
        DphNode->pVirtualBlock = Virtual;
        DphNode->nVirtualBlockSize = Size;

        /* Place it on the pool list */
        RtlpDphPlaceOnPoolList(DphRoot, DphNode);
    }
}

PDPH_HEAP_BLOCK NTAPI
RtlpDphSearchAvailableMemoryListForBestFit(PDPH_HEAP_ROOT DphRoot,
                                           SIZE_T Size)
{
    PLIST_ENTRY CurEntry;
    PDPH_HEAP_BLOCK Node;

    CurEntry = DphRoot->AvailableAllocationHead.Flink;

    while (TRUE)
    {
        /* If we reached end of the list - return right away */
        if (CurEntry == &DphRoot->AvailableAllocationHead) return NULL;

        /* Get the current available node */
        Node = CONTAINING_RECORD(CurEntry, DPH_HEAP_BLOCK, AvailableEntry);

        /* Check its size */
        if (Node->nVirtualBlockSize >= Size) break;

        /* Move to the next available entry */
        CurEntry = CurEntry->Flink;
    }

    /* Make sure Adjacency list pointers are biased */
    ASSERT(IS_BIASED_POINTER(Node->AdjacencyEntry.Flink));
    ASSERT(IS_BIASED_POINTER(Node->AdjacencyEntry.Blink));

    return Node;
}

PDPH_HEAP_BLOCK NTAPI
RtlpDphFindAvailableMemory(PDPH_HEAP_ROOT DphRoot,
                           SIZE_T Size,
                           BOOLEAN Grow)
{
    PDPH_HEAP_BLOCK Node;
    ULONG NewSize;

    /* Find an available best fitting node */
    Node = RtlpDphSearchAvailableMemoryListForBestFit(DphRoot, Size);

    /* If that didn't work, try to search a smaller one in the loop */
    while (!Node)
    {
        /* Break if the free list becomes too small */
        if (DphRoot->nFreeAllocations <= DPH_FREE_LIST_MINIMUM) break;

        /* Calculate a new free list size */
        NewSize = DphRoot->nFreeAllocations >> 1;
        if (NewSize < DPH_FREE_LIST_MINIMUM) NewSize = DPH_FREE_LIST_MINIMUM;

        /* Coalesce free into available */
        RtlpDphCoalesceFreeIntoAvailable(DphRoot, NewSize);

        /* Try to find an available best fitting node again */
        Node = RtlpDphSearchAvailableMemoryListForBestFit(DphRoot, Size);
    }

    /* If Node is NULL, then we could fix the situation only by
       growing the available VM size */
    if (!Node && Grow)
    {
        /* Grow VM size, if it fails - return failure directly */
        if (!RtlpDphGrowVirtual(DphRoot, Size)) return NULL;

        /* Try to find an available best fitting node again */
        Node = RtlpDphSearchAvailableMemoryListForBestFit(DphRoot, Size);

        if (!Node)
        {
            /* Do the last attempt: coalesce all free into available (if Size fits there) */
            if (DphRoot->nFreeAllocationBytesCommitted + DphRoot->nAvailableAllocationBytesCommitted >= Size)
            {
                /* Coalesce free into available */
                RtlpDphCoalesceFreeIntoAvailable(DphRoot, 0);

                /* Try to find an available best fitting node again */
                Node = RtlpDphSearchAvailableMemoryListForBestFit(DphRoot, Size);
            }
        }
    }

    /* Return node we found */
    return Node;
}

PDPH_HEAP_BLOCK NTAPI
RtlpDphAllocateNode(PDPH_HEAP_ROOT DphRoot)
{
    PDPH_HEAP_BLOCK Node;
    NTSTATUS Status;
    SIZE_T Size = DPH_POOL_SIZE, SizeVirtual;
    PVOID Ptr;

    /* Check for the easy case */
    if (DphRoot->pUnusedNodeListHead)
    {
        /* Just take a node from this list */
        Node = RtlpDphTakeNodeFromUnusedList(DphRoot);
        ASSERT(Node);
        return Node;
    }

    /* There is a need to make free space */
    Node = RtlpDphFindAvailableMemory(DphRoot, DPH_POOL_SIZE, FALSE);

    if (!DphRoot->pUnusedNodeListHead && !Node)
    {
        /* Retry with a smaller request */
        Size = PAGE_SIZE;
        Node = RtlpDphFindAvailableMemory(DphRoot, PAGE_SIZE, FALSE);
    }

    if (!DphRoot->pUnusedNodeListHead)
    {
        if (Node)
        {
            RtlpDphRemoveFromAvailableList(DphRoot, Node);
            Ptr = Node->pVirtualBlock;
            SizeVirtual = Node->nVirtualBlockSize;
        }
        else
        {
            /* No free space, need to alloc a new VM block */
            Size = DPH_POOL_SIZE;
            SizeVirtual = DPH_RESERVE_SIZE;
            Status = RtlpDphAllocateVm(&Ptr, SizeVirtual, MEM_COMMIT, PAGE_NOACCESS);

            if (!NT_SUCCESS(Status))
            {
                /* Retry with a smaller size */
                SizeVirtual = 0x10000;
                Status = RtlpDphAllocateVm(&Ptr, SizeVirtual, MEM_COMMIT, PAGE_NOACCESS);
                if (!NT_SUCCESS(Status)) return NULL;
            }
        }

        /* VM is allocated at this point, set protection */
        Status = RtlpDphProtectVm(Ptr, Size, PAGE_READWRITE);
        if (!NT_SUCCESS(Status))
        {
            if (Node)
            {
                RtlpDphCoalesceNodeIntoAvailable(DphRoot, Node);
            }
            else
            {
                //RtlpDphFreeVm();
                ASSERT(FALSE);
            }

            return NULL;
        }

        /* Zero the memory */
        if (Node) RtlZeroMemory(Ptr, Size);

        /* Add a new pool based on this VM */
        RtlpDphAddNewPool(DphRoot, Node, Ptr, Size, TRUE);

        if (Node)
        {
            if (Node->nVirtualBlockSize > Size)
            {
                Node->pVirtualBlock += Size;
                Node->nVirtualBlockSize -= Size;

                RtlpDphCoalesceNodeIntoAvailable(DphRoot, Node);
            }
            else
            {
                RtlpDphReturnNodeToUnusedList(DphRoot, Node);
            }
        }
        else
        {
            /* The new VM block was just allocated a few code lines ago,
               so initialize it */
            Node = RtlpDphTakeNodeFromUnusedList(DphRoot);
            Node->pVirtualBlock = Ptr;
            Node->nVirtualBlockSize = SizeVirtual;
            RtlpDphPlaceOnVirtualList(DphRoot, Node);

            Node = RtlpDphTakeNodeFromUnusedList(DphRoot);
            Node->pVirtualBlock = (PUCHAR)Ptr + Size;
            Node->nVirtualBlockSize = SizeVirtual - Size;
            RtlpDphPlaceOnVirtualList(DphRoot, Node);

            /* Coalesce them into available list */
            RtlpDphCoalesceNodeIntoAvailable(DphRoot, Node);
        }
    }

    return RtlpDphTakeNodeFromUnusedList(DphRoot);
}

BOOLEAN NTAPI
RtlpDphGrowVirtual(PDPH_HEAP_ROOT DphRoot,
                   SIZE_T Size)
{
    PDPH_HEAP_BLOCK Node, AvailableNode;
    PVOID Base;
    SIZE_T VirtualSize;
    NTSTATUS Status;

    /* Start with allocating a couple of nodes */
    Node = RtlpDphAllocateNode(DphRoot);
    if (!Node) return FALSE;

    AvailableNode = RtlpDphAllocateNode(DphRoot);
    if (!AvailableNode)
    {
        /* Free the allocated node and return failure */
        RtlpDphReturnNodeToUnusedList(DphRoot, Node);
        return FALSE;
    }

    /* Calculate size of VM to allocate by rounding it up */
    VirtualSize = (Size + 0xFFFF) & 0xFFFF0000;
    if (VirtualSize < DPH_RESERVE_SIZE)
        VirtualSize = DPH_RESERVE_SIZE;

    /* Allocate the virtual memory */
    Status = RtlpDphAllocateVm(&Base, VirtualSize, MEM_RESERVE, PAGE_NOACCESS);
    if (!NT_SUCCESS(Status))
    {
        /* Free the allocated node and return failure */
        RtlpDphReturnNodeToUnusedList(DphRoot, Node);
        RtlpDphReturnNodeToUnusedList(DphRoot, AvailableNode);
        return FALSE;
    }

    /* Set up our two nodes describing this VM */
    Node->pVirtualBlock = Base;
    Node->nVirtualBlockSize = VirtualSize;
    AvailableNode->pVirtualBlock = Base;
    AvailableNode->nVirtualBlockSize = VirtualSize;

    /* Add them to virtual and available lists respectively */
    RtlpDphPlaceOnVirtualList(DphRoot, Node);
    RtlpDphCoalesceNodeIntoAvailable(DphRoot, AvailableNode);

    /* Return success */
    return TRUE;
}

RTL_GENERIC_COMPARE_RESULTS
NTAPI
RtlpDphCompareNodeForTable(IN PRTL_AVL_TABLE Table,
                           IN PVOID FirstStruct,
                           IN PVOID SecondStruct)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return 0;
}

PVOID
NTAPI
RtlpDphAllocateNodeForTable(IN PRTL_AVL_TABLE Table,
                            IN CLONG ByteSize)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return NULL;
}

VOID
NTAPI
RtlpDphFreeNodeForTable(IN PRTL_AVL_TABLE Table,
                        IN PVOID Buffer)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
}

NTSTATUS NTAPI
RtlpDphInitializeDelayedFreeQueue()
{
    NTSTATUS Status;

    Status = RtlInitializeCriticalSection(&RtlpDphDelayedFreeQueueLock);
    if (!NT_SUCCESS(Status))
    {
        // TODO: Log this error!
        DPRINT1("Failure initializing delayed free queue critical section\n");
        return Status;
    }

    /* Initialize lists */
    InitializeListHead(&RtlpDphDelayedFreeQueue);
    RtlInitializeSListHead(&RtlpDphDelayedTemporaryPushList);

    /* Reset counters */
    RtlpDphMemoryUsedByDelayedFreeBlocks = 0;
    RtlpDphNumberOfDelayedFreeBlocks = 0;

    return Status;
}

VOID NTAPI
RtlpDphFreeDelayedBlocksFromHeap(PDPH_HEAP_ROOT DphRoot,
                                 PHEAP NormalHeap)
{
    PLIST_ENTRY Current, Next;
    PDPH_BLOCK_INFORMATION BlockInfo;
    ULONG ValidationInfo;

    /* The original routine seems to use a temporary SList to put blocks to be freed,
       then it releases the lock and frees the blocks. But let's make it simple for now */

    /* Acquire the delayed free queue lock */
    RtlEnterCriticalSection(&RtlpDphDelayedFreeQueueLock);

    /* Traverse the list */
    Current = RtlpDphDelayedFreeQueue.Flink;
    while (Current != &RtlpDphDelayedFreeQueue);
    {
        /* Get the next entry pointer */
        Next = Current->Flink;

        BlockInfo = CONTAINING_RECORD(Current, DPH_BLOCK_INFORMATION, FreeQueue);

        /* Check if it belongs to the same heap */
        if (BlockInfo->Heap == DphRoot)
        {
            /* Remove it from the list */
            RemoveEntryList(Current);

            /* Reset its heap to NULL */
            BlockInfo->Heap = NULL;

            if (!RtlpDphIsNormalFreeHeapBlock(BlockInfo + 1, &ValidationInfo, TRUE))
            {
                RtlpDphReportCorruptedBlock(DphRoot, 10, BlockInfo + 1, ValidationInfo);
            }

            /* Decrement counters */
            RtlpDphMemoryUsedByDelayedFreeBlocks -= BlockInfo->ActualSize;
            RtlpDphNumberOfDelayedFreeBlocks--;

            /* Free the normal heap */
            RtlFreeHeap (NormalHeap, 0, BlockInfo);
        }

        /* Move to the next one */
        Current = Next;
    }

    /* Release the delayed free queue lock */
    RtlLeaveCriticalSection(&RtlpDphDelayedFreeQueueLock);
}

NTSTATUS NTAPI
RtlpDphTargetDllsLogicInitialize()
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

VOID NTAPI
RtlpDphInternalValidatePageHeap(PDPH_HEAP_ROOT DphRoot, PVOID Address, ULONG Value)
{
    UNIMPLEMENTED;
}

VOID NTAPI
RtlpDphReportCorruptedBlock(PDPH_HEAP_ROOT DphRoot,
                            ULONG Reserved,
                            PVOID Block,
                            ULONG ValidationInfo)
{
    //RtlpDphGetBlockSizeFromCorruptedBlock();

    if (ValidationInfo & DPH_VALINFO_CORRUPTED_AFTER_FREE)
    {
        DPRINT1("block corrupted after having been freed\n");
    }

    if (ValidationInfo & DPH_VALINFO_ALREADY_FREED)
    {
        DPRINT1("block already freed\n");
    }

    if (ValidationInfo & DPH_VALINFO_BAD_INFIX_PATTERN)
    {
        DPRINT1("corrupted infix pattern for freed block\n");
    }

    if (ValidationInfo & DPH_VALINFO_BAD_POINTER)
    {
        DPRINT1("corrupted heap pointer or using wrong heap\n");
    }

    if (ValidationInfo & DPH_VALINFO_BAD_SUFFIX_PATTERN)
    {
        DPRINT1("corrupted suffix pattern\n");
    }

    if (ValidationInfo & DPH_VALINFO_BAD_PREFIX_PATTERN)
    {
        DPRINT1("corrupted prefix pattern\n");
    }

    if (ValidationInfo & DPH_VALINFO_BAD_START_STAMP)
    {
        DPRINT1("corrupted start stamp\n");
    }

    if (ValidationInfo & DPH_VALINFO_BAD_END_STAMP)
    {
        DPRINT1("corrupted end stamp\n");
    }

    if (ValidationInfo & DPH_VALINFO_EXCEPTION)
    {
        DPRINT1("exception raised while verifying block\n");
    }

    DPRINT1("Corrupted heap block %p\n", Block);
}

BOOLEAN NTAPI
RtlpDphIsPageHeapBlock(PDPH_HEAP_ROOT DphRoot,
                       PVOID Block,
                       PULONG ValidationInformation,
                       BOOLEAN CheckFillers)
{
    PDPH_BLOCK_INFORMATION BlockInfo;
    BOOLEAN SomethingWrong = FALSE;
    PUCHAR Byte, Start, End;

    ASSERT(ValidationInformation != NULL);
    *ValidationInformation = 0;

    // _SEH2_TRY {
    BlockInfo = (PDPH_BLOCK_INFORMATION)Block - 1;

    /* Check stamps */
    if (BlockInfo->StartStamp != DPH_FILL_START_STAMP_1)
    {
        *ValidationInformation |= DPH_VALINFO_BAD_START_STAMP;
        SomethingWrong = TRUE;

        /* Check if it has an alloc/free mismatch */
        if (BlockInfo->StartStamp == DPH_FILL_START_STAMP_2)
        {
            /* Notify respectively */
            *ValidationInformation = 0x101;
        }
    }

    if (BlockInfo->EndStamp != DPH_FILL_END_STAMP_1)
    {
        *ValidationInformation |= DPH_VALINFO_BAD_END_STAMP;
        SomethingWrong = TRUE;
    }

    /* Check root heap pointer */
    if (BlockInfo->Heap != DphRoot)
    {
        *ValidationInformation |= DPH_VALINFO_BAD_POINTER;
        SomethingWrong = TRUE;
    }

    /* Check other fillers if requested */
    if (CheckFillers)
    {
        /* Check space after the block */
        Start = (PUCHAR)Block + BlockInfo->RequestedSize;
        End = (PUCHAR)ROUND_UP(Start, PAGE_SIZE);
        for (Byte = Start; Byte < End; Byte++)
        {
            if (*Byte != DPH_FILL_BLOCK_END)
            {
                *ValidationInformation |= DPH_VALINFO_BAD_SUFFIX_PATTERN;
                SomethingWrong = TRUE;
                break;
            }
        }
    }

    return (SomethingWrong == FALSE);
}

BOOLEAN NTAPI
RtlpDphIsNormalFreeHeapBlock(PVOID Block,
                             PULONG ValidationInformation,
                             BOOLEAN CheckFillers)
{
    ASSERT(ValidationInformation != NULL);

    UNIMPLEMENTED;
    *ValidationInformation = 0;
    return TRUE;
}

NTSTATUS NTAPI
RtlpDphProcessStartupInitialization()
{
    NTSTATUS Status;
    PTEB Teb = NtCurrentTeb();

    /* Initialize the DPH heap list and its critical section */
    InitializeListHead(&RtlpDphPageHeapList);
    Status = RtlInitializeCriticalSection(&RtlpDphPageHeapListLock);
    if (!NT_SUCCESS(Status))
    {
        ASSERT(FALSE);
        return Status;
    }

    /* Initialize delayed-free queue */
    Status = RtlpDphInitializeDelayedFreeQueue();
    if (!NT_SUCCESS(Status)) return Status;

    /* Initialize the target dlls string */
    RtlInitUnicodeString(&RtlpDphTargetDllsUnicode, RtlpDphTargetDlls);
    Status = RtlpDphTargetDllsLogicInitialize();

    /* Per-process DPH init is done */
    RtlpDphPageHeapListInitialized = TRUE;

    DPRINT1("Page heap: pid 0x%X: page heap enabled with flags 0x%X.\n", Teb->ClientId.UniqueProcess, RtlpDphGlobalFlags);

    return Status;
}

HANDLE NTAPI
RtlpPageHeapCreate(ULONG Flags,
                   PVOID Addr,
                   SIZE_T TotalSize,
                   SIZE_T CommitSize,
                   PVOID Lock,
                   PRTL_HEAP_PARAMETERS Parameters)
{
    PVOID Base;
    PHEAP HeapPtr;
    PDPH_HEAP_ROOT DphRoot;
    PDPH_HEAP_BLOCK DphNode;
    ULONG MemSize;
    NTSTATUS Status;
    LARGE_INTEGER PerfCounter;

    /* Check for a DPH bypass flag */
    if ((ULONG_PTR)Parameters == -1) return NULL;

    /* Make sure no user-allocated stuff was provided */
    if (Addr || Lock) return NULL;

    /* Allocate minimum amount of virtual memory */
    MemSize = DPH_RESERVE_SIZE;
    Status = RtlpDphAllocateVm(&Base, MemSize, MEM_COMMIT, PAGE_NOACCESS);
    if (!NT_SUCCESS(Status))
    {
        ASSERT(FALSE);
        return NULL;
    }

    /* Set protection */
    Status = RtlpDphProtectVm(Base, 2*PAGE_SIZE + DPH_POOL_SIZE, PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        //RtlpDphFreeVm(Base, 0, 0, 0);
        ASSERT(FALSE);
        return NULL;
    }

    /* Start preparing the 1st page. Fill it with the default filler */
    RtlFillMemory(Base, PAGE_SIZE, DPH_FILL);

    /* Set flags in the "HEAP" structure */
    HeapPtr = (PHEAP)Base;
    HeapPtr->Flags = Flags | HEAP_FLAG_PAGE_ALLOCS;
    HeapPtr->ForceFlags = Flags | HEAP_FLAG_PAGE_ALLOCS;

    /* Set 1st page to read only now */
    Status = RtlpDphProtectVm(Base, PAGE_SIZE, PAGE_READONLY);
    if (!NT_SUCCESS(Status))
    {
        ASSERT(FALSE);
        return NULL;
    }

    /* 2nd page is the real DPH root block */
    DphRoot = (PDPH_HEAP_ROOT)((PCHAR)Base + PAGE_SIZE);

    /* Initialize the DPH root */
    DphRoot->Signature = DPH_SIGNATURE;
    DphRoot->HeapFlags = Flags;
    DphRoot->HeapCritSect = (PRTL_CRITICAL_SECTION)((PCHAR)DphRoot + DPH_POOL_SIZE);
    DphRoot->ExtraFlags = RtlpDphGlobalFlags;

    ZwQueryPerformanceCounter(&PerfCounter, NULL);
    DphRoot->Seed = PerfCounter.LowPart;

    RtlInitializeCriticalSection(DphRoot->HeapCritSect);

    /* Create a normal heap for this paged heap */
    DphRoot->NormalHeap = RtlCreateHeap(Flags, NULL, TotalSize, CommitSize, NULL, (PRTL_HEAP_PARAMETERS)-1);
    if (!DphRoot->NormalHeap)
    {
        ASSERT(FALSE);
        return NULL;
    }

    /* 3rd page: a pool for DPH allocations */
    RtlpDphAddNewPool(DphRoot, NULL, DphRoot + 1, DPH_POOL_SIZE - sizeof(DPH_HEAP_ROOT), FALSE);

    /* Allocate internal heap blocks. For the root */
    DphNode = RtlpDphAllocateNode(DphRoot);
    ASSERT(DphNode != NULL);
    DphNode->pVirtualBlock = (PUCHAR)DphRoot;
    DphNode->nVirtualBlockSize = DPH_POOL_SIZE;
    RtlpDphPlaceOnPoolList(DphRoot, DphNode);

    /* For the memory we allocated as a whole */
    DphNode = RtlpDphAllocateNode(DphRoot);
    ASSERT(DphNode != NULL);
    DphNode->pVirtualBlock = Base;
    DphNode->nVirtualBlockSize = MemSize;
    RtlpDphPlaceOnVirtualList(DphRoot, DphNode);

    /* For the remaining part */
    DphNode = RtlpDphAllocateNode(DphRoot);
    ASSERT(DphNode != NULL);
    DphNode->pVirtualBlock = (PUCHAR)Base + 2*PAGE_SIZE + DPH_POOL_SIZE;
    DphNode->nVirtualBlockSize = MemSize - (2*PAGE_SIZE + DPH_POOL_SIZE);
    RtlpDphCoalesceNodeIntoAvailable(DphRoot, DphNode);

    //DphRoot->CreateStackTrace = RtlpDphLogStackTrace(1);

    /* Initialize AVL-based busy nodes table */
    RtlInitializeGenericTableAvl(&DphRoot->BusyNodesTable,
                                 RtlpDphCompareNodeForTable,
                                 RtlpDphAllocateNodeForTable,
                                 RtlpDphFreeNodeForTable,
                                 NULL);

    /* Initialize per-process startup info */
    if (!RtlpDphPageHeapListInitialized) RtlpDphProcessStartupInitialization();

    /* Acquire the heap list lock */
    RtlEnterCriticalSection(&RtlpDphPageHeapListLock);

    /* Insert this heap to the tail of the global list */
    InsertTailList(&RtlpDphPageHeapList, &DphRoot->NextHeap);

    /* Note we increased the size of the list */
    RtlpDphPageHeapListLength++;

    /* Release the heap list lock */
    RtlLeaveCriticalSection(&RtlpDphPageHeapListLock);

    if (RtlpDphDebugOptions & DPH_DEBUG_VERBOSE)
    {
        DPRINT1("Page heap: process 0x%X created heap @ %p (%p, flags 0x%X)\n",
            NtCurrentTeb()->ClientId.UniqueProcess, (PUCHAR)DphRoot - PAGE_SIZE, DphRoot->NormalHeap, DphRoot->ExtraFlags);
    }

    /* Perform internal validation if required */
    if (RtlpDphDebugOptions & DPH_DEBUG_INTERNAL_VALIDATE)
        RtlpDphInternalValidatePageHeap(DphRoot, NULL, 0);

    return (PUCHAR)DphRoot - PAGE_SIZE;
}

PVOID NTAPI
RtlpPageHeapDestroy(HANDLE HeapPtr)
{
    PDPH_HEAP_ROOT DphRoot;
    PDPH_HEAP_BLOCK Node, Next;
    PHEAP NormalHeap;
    ULONG Value;

    /* Check if it's not a process heap */
    if (HeapPtr == RtlGetProcessHeap())
    {
        DbgBreakPoint();
        return NULL;
    }

    /* Get pointer to the heap root */
    DphRoot = RtlpDphPointerFromHandle(HeapPtr);
    if (!DphRoot) return NULL;

    RtlpDphPreProcessing(DphRoot, DphRoot->HeapFlags);

    /* Get the pointer to the normal heap */
    NormalHeap = DphRoot->NormalHeap;

    /* Free the delayed-free blocks */
    RtlpDphFreeDelayedBlocksFromHeap(DphRoot, NormalHeap);

    /* Go through the busy blocks */
    Node = RtlEnumerateGenericTableAvl(&DphRoot->BusyNodesTable, TRUE);

    while (Node)
    {
        if (!(DphRoot->ExtraFlags & DPH_EXTRA_CHECK_CORRUPTED_BLOCKS))
        {
            if (!RtlpDphIsPageHeapBlock(DphRoot, Node->pUserAllocation, &Value, TRUE))
            {
                RtlpDphReportCorruptedBlock(DphRoot, 3, Node->pUserAllocation, Value);
            }
        }

        /* FIXME: Call AV notification */
        //AVrfInternalHeapFreeNotification();

        /* Go to the next node */
        Node = RtlEnumerateGenericTableAvl(&DphRoot->BusyNodesTable, FALSE);
    }

    /* Acquire the global heap list lock */
    RtlEnterCriticalSection(&RtlpDphPageHeapListLock);

    /* Remove the entry and decrement the global counter */
    RemoveEntryList(&DphRoot->NextHeap);
    RtlpDphPageHeapListLength--;

    /* Release the global heap list lock */
    RtlLeaveCriticalSection(&RtlpDphPageHeapListLock);

    /* Leave and delete this heap's critical section */
    RtlLeaveCriticalSection(DphRoot->HeapCritSect);
    RtlDeleteCriticalSection(DphRoot->HeapCritSect);

    /* Now go through all virtual list nodes and release the VM */
    Node = DphRoot->pVirtualStorageListHead;
    while (Node)
    {
        Next = Node->pNextAlloc;
        /* Release the memory without checking result */
        RtlpDphFreeVm(Node->pVirtualBlock, 0, MEM_RELEASE);
        Node = Next;
    }

    /* Destroy the normal heap */
    RtlDestroyHeap(NormalHeap);

    /* Report success */
    if (RtlpDphDebugOptions & DPH_DEBUG_VERBOSE)
        DPRINT1("Page heap: process 0x%X destroyed heap @ %p (%p)\n", NtCurrentTeb()->ClientId.UniqueProcess, HeapPtr, NormalHeap);

    return NULL;
}

PVOID NTAPI
RtlpPageHeapAllocate(IN PVOID HeapPtr,
                     IN ULONG Flags,
                     IN SIZE_T Size)
{
    return NULL;
}

BOOLEAN NTAPI
RtlpPageHeapFree(HANDLE HeapPtr,
                 ULONG Flags,
                 PVOID Ptr)
{
    return FALSE;
}

PVOID NTAPI
RtlpPageHeapReAllocate(HANDLE HeapPtr,
                       ULONG Flags,
                       PVOID Ptr,
                       SIZE_T Size)
{
    return NULL;
}

BOOLEAN NTAPI
RtlpPageHeapGetUserInfo(PVOID HeapHandle,
                        ULONG Flags,
                        PVOID BaseAddress,
                        PVOID *UserValue,
                        PULONG UserFlags)
{
    return FALSE;
}

BOOLEAN NTAPI
RtlpPageHeapSetUserValue(PVOID HeapHandle,
                         ULONG Flags,
                         PVOID BaseAddress,
                         PVOID UserValue)
{
    return FALSE;
}

BOOLEAN
NTAPI
RtlpPageHeapSetUserFlags(PVOID HeapHandle,
                         ULONG Flags,
                         PVOID BaseAddress,
                         ULONG UserFlagsReset,
                         ULONG UserFlagsSet)
{
    return FALSE;
}

SIZE_T NTAPI
RtlpPageHeapSize(HANDLE HeapPtr,
                 ULONG Flags,
                 PVOID Ptr)
{
    return 0;
}

/* EOF */

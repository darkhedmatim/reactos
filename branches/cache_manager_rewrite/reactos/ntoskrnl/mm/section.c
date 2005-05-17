/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/section.c
 * PURPOSE:         Implements section objects
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#include <reactos/exeformat.h>

/* TYPES *********************************************************************/

typedef struct
{
   PSECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;
   ULONG Offset;
   BOOLEAN WasDirty;
   BOOLEAN Private;
}
MM_SECTION_PAGEOUT_CONTEXT;

/* GLOBALS *******************************************************************/

extern PVOID CcCacheViewBase;
extern ULONG CcCacheViewArrayCount;
extern PCACHE_VIEW CcCacheViewArray;
extern PMEMORY_AREA CcCacheViewMemoryArea;

#ifndef ROUND_DOWN
#define ROUND_DOWN(X,Y) ((X) & ~((Y) - 1))
#endif

POBJECT_TYPE EXPORTED MmSectionObjectType = NULL;

static FAST_MUTEX ImageSectionObjectLock;
static LIST_ENTRY ImageSectionObjectListHead;
static ULONG ImageSectionObjectCount;
static PMM_IMAGE_SECTION_OBJECT ImageSectionObjectNext;

static FAST_MUTEX DataSectionObjectLock;
static LIST_ENTRY DataSectionObjectListHead;
static ULONG DataSectionObjectCount;

static KTIMER MmspWorkerThreadTimer;


static GENERIC_MAPPING MmpSectionMapping = {
         STANDARD_RIGHTS_READ | SECTION_MAP_READ | SECTION_QUERY,
         STANDARD_RIGHTS_WRITE | SECTION_MAP_WRITE,
         STANDARD_RIGHTS_EXECUTE | SECTION_MAP_EXECUTE,
         SECTION_ALL_ACCESS};

#define TAG_MM_SECTION_SEGMENT   TAG('M', 'M', 'S', 'S')
#define TAG_SECTION_PAGE_TABLE   TAG('M', 'S', 'P', 'T')

#define PAGE_FROM_SSE(E)         ((E) & 0xFFFFF000)
#define PFN_FROM_SSE(E)          ((E) >> PAGE_SHIFT)
#define IS_SWAP_FROM_SSE(E)      ((E) & 0x00000001)
#define MAX_SHARE_COUNT          0x7FF
#define MAKE_SSE(P, C)           ((P) | ((C) << 1))
#define SWAPENTRY_FROM_SSE(E)    ((E) >> 1)
#define MAKE_SWAP_SSE(S)         (((S) << 1) | 0x1)

static const INFORMATION_CLASS_INFO ExSectionInfoClass[] =
{
  ICI_SQ_SAME( sizeof(SECTION_BASIC_INFORMATION), sizeof(ULONG), ICIF_QUERY ), /* SectionBasicInformation */
  ICI_SQ_SAME( sizeof(SECTION_IMAGE_INFORMATION), sizeof(ULONG), ICIF_QUERY ), /* SectionImageInformation */
};

/* FUNCTIONS *****************************************************************/

ULONG
MmSharePage(PFN_TYPE Pfn);

ULONG
MmUnsharePage(PFN_TYPE Pfn);

ULONG
MmGetShareCountPage(PFN_TYPE Pfn);

ULONG
MmGetPageEntrySectionSegment(PMM_SECTION_SEGMENT Segment,
                             ULONG Offset);

/* Note: Mmsp prefix denotes "Memory Manager Section Private". */

/*
 * FUNCTION:  Waits in kernel mode up to ten seconds for an MM_PAGEOP event.
 * ARGUMENTS: PMM_PAGEOP which event we should wait for.
 * RETURNS:   Status of the wait.
 */
static NTSTATUS
MmspWaitForPageOpCompletionEvent(PMM_PAGEOP PageOp)
{
   LARGE_INTEGER Timeout;
#ifdef __GNUC__ /* TODO: Use other macro to check for suffix to use? */

   Timeout.QuadPart = -100000000LL; // 10 sec
#else

   Timeout.QuadPart = -100000000; // 10 sec
#endif

   return KeWaitForSingleObject(&PageOp->CompletionEvent, 0, KernelMode, FALSE, &Timeout);
}


/*
 * FUNCTION:  Sets the page op completion event and releases the page op.
 * ARGUMENTS: PMM_PAGEOP.
 * RETURNS:   In shorter time than it takes you to even read this
 *            description, so don't even think about geting a mug of coffee.
 */
static void
MmspCompleteAndReleasePageOp(PMM_PAGEOP PageOp)
{
   KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
   MmReleasePageOp(PageOp);
}


/*
 * FUNCTION:  Waits in kernel mode indefinitely for a file object lock.
 * ARGUMENTS: PFILE_OBJECT to wait for.
 * RETURNS:   Status of the wait.
 */
static NTSTATUS
MmspWaitForFileLock(PFILE_OBJECT File)
{
   return KeWaitForSingleObject(&File->Lock, 0, KernelMode, FALSE, NULL);
}


VOID
MmFreePageTablesSectionSegment(PMM_SECTION_SEGMENT Segment)
{
   ULONG i;
   if (Segment->Length > NR_SECTION_PAGE_TABLES * PAGE_SIZE)
   {
      for (i = 0; i < NR_SECTION_PAGE_TABLES; i++)
      {
         if (Segment->PageDirectory.PageTables[i] != NULL)
         {
            ExFreePool(Segment->PageDirectory.PageTables[i]);
         }
      }
   }
}

VOID 
MmFreeImageSectionSegments(PSECTION_OBJECT_POINTERS SectionObjectPointer)
{
   if (SectionObjectPointer->ImageSectionObject != NULL)
   {
      PMM_IMAGE_SECTION_OBJECT ImageSectionObject;
      PMM_SECTION_SEGMENT SectionSegments;
      ULONG NrSegments;
      ULONG i, Offset, Length, Entry;

      ImageSectionObject = (PMM_IMAGE_SECTION_OBJECT)SectionObjectPointer->ImageSectionObject;
      NrSegments = ImageSectionObject->NrSegments;
      SectionSegments = ImageSectionObject->Segments;
      for (i = 0; i < NrSegments; i++)
      {
         if (SectionSegments[i].ReferenceCount != 0)
         {
            DPRINT1("Image segment %d still referenced (was %d)\n", i,
                    SectionSegments[i].ReferenceCount);
            KEBUGCHECK(0);
         }
         Length = PAGE_ROUND_UP(SectionSegments[i].Length);
	 for (Offset = 0; Offset < Length; Offset += PAGE_SIZE)
	 {
	    Entry = MmGetPageEntrySectionSegment(&SectionSegments[i], Offset);
	    if (Entry != 0)
	    {
	       if (IS_SWAP_FROM_SSE(Entry)) 
	       {
	          KEBUGCHECK(0);
	       }
	       else if (MmGetShareCountPage(PFN_FROM_SSE(Entry)) != 0)
	       {
	          DPRINT1("%d %x\n", i, Offset, MmGetShareCountPage(PFN_FROM_SSE(Entry)));
	          KEBUGCHECK(0);
	       }
	       else
	       {
	          MmReleasePageMemoryConsumer(MC_USER, PFN_FROM_SSE(Entry));
	       }
	    }
	 }
         MmFreePageTablesSectionSegment(&SectionSegments[i]);
      }
      RemoveEntryList(&ImageSectionObject->ListEntry);
      ImageSectionObjectCount--;
      SectionObjectPointer->ImageSectionObject = NULL;
      if (ImageSectionObjectNext == ImageSectionObject)
      {
         ImageSectionObjectNext = NULL;
      }
      ObDereferenceObject(ImageSectionObject->FileObject);
      ExFreePool(ImageSectionObject->Segments);
      ExFreePool(ImageSectionObject);
   }
}

VOID
MmLockSectionSegment(PMM_SECTION_SEGMENT Segment)
{
   ExAcquireFastMutex(&Segment->Lock);
}

VOID
MmUnlockSectionSegment(PMM_SECTION_SEGMENT Segment)
{
   ExReleaseFastMutex(&Segment->Lock);
}

VOID
MmFreeDataSectionSegments(PSECTION_OBJECT_POINTERS SectionObjectPointer)
{
   if (SectionObjectPointer->DataSectionObject != NULL)
   {
      PMM_SECTION_SEGMENT Segment;
      ULONG Offset;
      ULONG Length;
      ULONG Entry;
      PFN_TYPE Pfn;
      LARGE_INTEGER FileOffset;
      KEVENT Event;
      IO_STATUS_BLOCK Iosb;
      NTSTATUS Status;
      BOOL FoundPageOp;
      PMM_PAGEOP PageOp;
   
      UCHAR MdlBase[sizeof(MDL) + sizeof(ULONG)];
      PMDL Mdl = (PMDL)MdlBase;

      Segment = (PMM_SECTION_SEGMENT)SectionObjectPointer->DataSectionObject;

      DPRINT("%x %wZ\n", &Segment->FileObject, &Segment->FileObject->FileName);

      MmLockSectionSegment(Segment);

      if (Segment->ReferenceCount != 0)
      {
         DPRINT1("Data segment still referenced\n");
         KEBUGCHECK(0);
      }
      Length = PAGE_ROUND_UP(Segment->Length);
      Offset = 0;
      FoundPageOp = FALSE;
      while (Offset < Length)
      {
	 while (NULL != (PageOp = MmCheckForPageOp(CcCacheViewMemoryArea, NULL, NULL, Segment, Offset)))
	 {
            MmUnlockSectionSegment(Segment);
            Status = MmspWaitForPageOpCompletionEvent(PageOp);
            if (Status != STATUS_SUCCESS)
            {
               DPRINT1("Failed to wait for page op, status = %x\n", Status);
               KEBUGCHECK(0);
            }
            MmLockSectionSegment(Segment);
            MmspCompleteAndReleasePageOp(PageOp);
	    FoundPageOp = TRUE;
	 }
	 if (FoundPageOp)
	 {
	    FoundPageOp = FALSE;
            Length = PAGE_ROUND_UP(Segment->Length);
            Offset = 0;
            continue;
	 }

         Entry = MmGetPageEntrySectionSegment(Segment, Offset);
	 if (Entry != 0)
	 {
	    if (IS_SWAP_FROM_SSE(Entry)) 
	    {
	       KEBUGCHECK(0);
	    }
	    else if (MmGetShareCountPage(PFN_FROM_SSE(Entry)) != 0)
	    {
	       DPRINT1("%d %x\n", Offset, MmGetShareCountPage(PFN_FROM_SSE(Entry)));
	       KEBUGCHECK(0);
	    }
	    else
	    {
	       if (Entry & 0x2)
	       {
	          DPRINT1("Releasing dirty page at offset %d from %wZ\n", Offset, &Segment->FileObject->FileName);

                  MmInitializeMdl(Mdl, NULL, PAGE_SIZE);
		  Pfn = PFN_FROM_SSE(Entry);
                  MmBuildMdlFromPages(Mdl, &Pfn);
                  FileOffset.QuadPart = Offset * PAGE_SIZE;
                  KeInitializeEvent(&Event, NotificationEvent, FALSE);
                  Status = IoSynchronousPageWrite(Segment->FileObject,
                                                  Mdl,
                                                  &FileOffset,
                                                  &Event,
                                                  &Iosb);
                  if (Status == STATUS_PENDING)
                  {
                     KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
                     Status = Iosb.Status;
		  }
                  MmUnmapLockedPages(Mdl->MappedSystemVa, Mdl);            
	       }
	       MmReleasePageMemoryConsumer(MC_USER, PFN_FROM_SSE(Entry));
	    }
	 }
	 Offset += PAGE_SIZE;
      }
      MmUnlockSectionSegment(Segment);
      RemoveEntryList(&Segment->ListEntry);
      DataSectionObjectCount--;
      SectionObjectPointer->DataSectionObject = NULL;
//      if (DataSectionObjectNext == Segment)
//      {
//         DataSectionObjectNext = NULL;
//      }
      ObDereferenceObject(Segment->FileObject);
      MmFreePageTablesSectionSegment(Segment);
      ExFreePool(Segment);
   }
}

VOID
MmChangeSectionSegmentSize(PMM_SECTION_SEGMENT Segment,
		           ULONG NewLength)
{
   PSECTION_PAGE_TABLE Table;
   ULONG i, j;
   ULONG Start;
  
   if (PAGE_ROUND_UP(NewLength) > PAGE_ROUND_UP(Segment->Length))
   {
      /* segment must be expand */
      if (PAGE_ROUND_UP(NewLength) > NR_SECTION_PAGE_TABLES * PAGE_SIZE &&
          PAGE_ROUND_UP(Segment->Length) <= NR_SECTION_PAGE_TABLES * PAGE_SIZE)
      {
         Table = (PSECTION_PAGE_TABLE)&Segment->PageDirectory;
         for (i = 0; i < NR_SECTION_PAGE_ENTRIES; i++)
         {
            if (Table->Entry[i])
	    {
	       Table = ExAllocatePoolWithTag(NonPagedPool, sizeof(SECTION_PAGE_TABLE),
                                             TAG_SECTION_PAGE_TABLE); 
	       if (Table == NULL)
	       {
	          KEBUGCHECK(0);
	       }
	       memcpy(Table, Segment->PageDirectory.PageTables, sizeof(SECTION_PAGE_TABLE));
	       memset(Segment->PageDirectory.PageTables, 0, sizeof(SECTION_PAGE_TABLE));
	       Segment->PageDirectory.PageTables[0] = Table;
	       break;
	    }
	 }
      }
      Segment->RawLength = NewLength;
      Segment->Length = PAGE_ROUND_UP(NewLength);
   }
   else if (PAGE_ROUND_UP(NewLength) < PAGE_ROUND_UP(Segment->Length))
   {
      /* must be shrink */
      if (PAGE_ROUND_UP(NewLength) > NR_SECTION_PAGE_TABLES * PAGE_SIZE)
      {
         for (i = PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET(PAGE_ROUND_UP(NewLength)); 
	      i < NR_SECTION_PAGE_TABLES; 
	      i++)
	 {
	    Table = Segment->PageDirectory.PageTables[i];
	    if (Table)
	    {
               if (i > PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET(PAGE_ROUND_UP(Segment->Length)))
	       {
	          KEBUGCHECK(0);
	       }
               if (i == PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET(PAGE_ROUND_UP(NewLength)) &&
		   PAGE_ROUND_UP(NewLength) % (NR_SECTION_PAGE_TABLES * PAGE_SIZE))
	       {
	          Start = PAGE_ROUND_UP(NewLength) % (NR_SECTION_PAGE_TABLES * PAGE_SIZE);
	       }
	       else
	       {
	          Start = 0;
	       }
	       for (j = Start; j < NR_SECTION_PAGE_TABLES; j++)
	       {
                  if(Table->Entry[j])
		  {
		     KEBUGCHECK(0);
		  }
	       }
	       if (i > PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET(PAGE_ROUND_UP(NewLength)))
	       {
	          ExFreePool(Table);
                  Segment->PageDirectory.PageTables[i] = NULL;
	       }
	    }
	 }
      }
      else
      {
         for (i = PAGE_ROUND_UP(NewLength); i < NR_SECTION_PAGE_TABLES; i++)
	 {
	    if (Segment->PageDirectory.PageTables[i])
	    {
	       KEBUGCHECK(0);
	    }
	 }
      }
      if (PAGE_ROUND_UP(NewLength) <= NR_SECTION_PAGE_TABLES * PAGE_SIZE &&
	  PAGE_ROUND_UP(Segment->Length) > NR_SECTION_PAGE_TABLES * PAGE_SIZE)
      {
         Table = Segment->PageDirectory.PageTables[0];
	 memcpy(Segment->PageDirectory.PageTables, Table->Entry, sizeof(SECTION_PAGE_TABLE));
	 ExFreePool(Table);
      }
   }
   else
   {
      /* nothing to do */
   }
   Segment->RawLength = NewLength;
   Segment->Length = PAGE_ROUND_UP(NewLength);
}
     

VOID
MmSetPageEntrySectionSegment(PMM_SECTION_SEGMENT Segment,
                             ULONG Offset,
                             ULONG Entry)
{
   PSECTION_PAGE_TABLE Table;
   ULONG DirectoryOffset;
   ULONG TableOffset;

   if (Segment->Length <= NR_SECTION_PAGE_TABLES * PAGE_SIZE)
   {
      Table = (PSECTION_PAGE_TABLE)&Segment->PageDirectory;
   }
   else
   {
      DirectoryOffset = PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET(Offset);
      Table = Segment->PageDirectory.PageTables[DirectoryOffset];
      if (Table == NULL)
      {
         Table =
            Segment->PageDirectory.PageTables[DirectoryOffset] =
               ExAllocatePoolWithTag(NonPagedPool, sizeof(SECTION_PAGE_TABLE),
                                     TAG_SECTION_PAGE_TABLE);
         if (Table == NULL)
         {
            KEBUGCHECK(0);
         }
         memset(Table, 0, sizeof(SECTION_PAGE_TABLE));
         DPRINT("Table %x\n", Table);
      }
   }
   TableOffset = PAGE_TO_SECTION_PAGE_TABLE_OFFSET(Offset);
   Table->Entry[TableOffset] = Entry;
}


ULONG
MmGetPageEntrySectionSegment(PMM_SECTION_SEGMENT Segment,
                             ULONG Offset)
{
   PSECTION_PAGE_TABLE Table;
   ULONG Entry;
   ULONG DirectoryOffset;
   ULONG TableOffset;

   DPRINT("MmGetPageEntrySection(Segment %x, Offset %x)\n", Segment, Offset);

   if (Segment->Length <= NR_SECTION_PAGE_TABLES * PAGE_SIZE)
   {
      Table = (PSECTION_PAGE_TABLE)&Segment->PageDirectory;
   }
   else
   {
      DirectoryOffset = PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET(Offset);
      Table = Segment->PageDirectory.PageTables[DirectoryOffset];
      DPRINT("Table %x\n", Table);
      if (Table == NULL)
      {
         return(0);
      }
   }
   TableOffset = PAGE_TO_SECTION_PAGE_TABLE_OFFSET(Offset);
   Entry = Table->Entry[TableOffset];
   return(Entry);
}

VOID
MmSharePageEntrySectionSegment(PMM_SECTION_SEGMENT Segment,
                               ULONG Offset)
{
   ULONG Entry;

   Entry = MmGetPageEntrySectionSegment(Segment, Offset);
   if (Entry == 0)
   {
      DPRINT1("Entry == 0 for MmSharePageEntrySectionSegment\n");
      KEBUGCHECK(0);
   }
   if (IS_SWAP_FROM_SSE(Entry))
   {
      KEBUGCHECK(0);
   }
   MmSharePage(PFN_FROM_SSE(Entry));
}

BOOLEAN
MmUnsharePageEntrySectionSegment(PSECTION_OBJECT Section,
                                 PMM_SECTION_SEGMENT Segment,
                                 ULONG Offset,
                                 BOOLEAN Dirty,
                                 BOOLEAN PageOut)
{
   ULONG Entry;
   ULONG ShareCount;
   PFN_TYPE Page;

   Entry = MmGetPageEntrySectionSegment(Segment, Offset);
   if (Entry == 0)
   {
      DPRINT1("%x\n", Offset);
      DPRINT1("Entry == 0 for MmUnsharePageEntrySectionSegment\n");
      KEBUGCHECK(0);
   }
   if (IS_SWAP_FROM_SSE(Entry))
   {
      KEBUGCHECK(0);
   }
   Page = PFN_FROM_SSE(Entry);
   /*
    * If we reducing the share count of this entry to zero then set the entry
    * to zero and tell the cache the page is no longer mapped.
    */
   if ((ShareCount = MmUnsharePage(Page)) == 0)
   {
      PFILE_OBJECT FileObject;
      SWAPENTRY SavedSwapEntry;
      BOOLEAN IsImageSection;
      ULONG FileOffset;

      FileOffset = Offset + Segment->FileOffset;

      IsImageSection = Section->AllocationAttributes & SEC_IMAGE ? TRUE : FALSE;

      FileObject = Section->FileObject;
      SavedSwapEntry = MmGetSavedSwapEntryPage(Page);

      if (PageOut)
      {
	 MmSetPageEntrySectionSegment(Segment, Offset, 0);
         MmReleasePageMemoryConsumer(MC_USER, Page);
      }
      else
      {
         if (Section->AllocationAttributes & SEC_IMAGE &&
	     !(Segment->Characteristics & IMAGE_SCN_MEM_SHARED))
	 {
	    if (SavedSwapEntry)
	    {
	       KEBUGCHECK(0);
	    }
	    if (Dirty || (Entry & 0x2))
	    {
	       KEBUGCHECK(0);
	    }
	 }
	 else if (!(Section->AllocationAttributes & SEC_IMAGE) &&
	          Section->FileObject != NULL)
	 {
	    if (SavedSwapEntry)
	    {
	       KEBUGCHECK(0);
	    }
	    if (Dirty && !(Entry & 0x2))
	    {
	       MmSetPageEntrySectionSegment(Segment, Offset, Entry | 0x2);
	    }
	 }
	 else if(Segment->Flags & MM_PAGEFILE_SEGMENT ||
                 Segment->Characteristics & IMAGE_SCN_MEM_SHARED)
	 {
	    if (Dirty && !(Entry & 0x2))
	    {
	       MmSetPageEntrySectionSegment(Segment, Offset, Entry | 0x2);
	    }
	 }
	 else
	 {
	    KEBUGCHECK(0);
            if (SavedSwapEntry == 0)
            {
               if (!PageOut &&
                   ((Segment->Flags & MM_PAGEFILE_SEGMENT) ||
                    (Segment->Characteristics & IMAGE_SCN_MEM_SHARED)))
               {
                  /*
                   * FIXME:
                   *   Try to page out this page and set the swap entry
                   *   within the section segment. There exist no rmap entry
                   *   for this page. The pager thread can't page out a
                   *   page without a rmap entry.
                   */
//                MmSetPageEntrySectionSegment(Segment, Offset, Entry);
               }
               else
               {
                  if (IsImageSection)
	          {
                     if (PageOut)
	             {
	                MmSetPageEntrySectionSegment(Segment, Offset, 0);
                        MmReleasePageMemoryConsumer(MC_USER, Page);
	             }
	             else
	             {
//	                MmSetPageEntrySectionSegment(Segment, Offset, Entry);	    
	             }
	          }
	          else if (Section->FileObject != NULL)
	          {
	             if (PageOut && !Dirty && !(Entry & 0x2))
	             {
	                MmSetPageEntrySectionSegment(Segment, Offset, 0);
                        MmReleasePageMemoryConsumer(MC_USER, Page);
	             }
	             else
	             {
	                if (Dirty && !(Entry & 0x2))
		        {
		           MmSetPageEntrySectionSegment(Segment, Offset, Entry | 0x2);
		        }
	             }      
	          }
	          else
	          {
                     MmSetPageEntrySectionSegment(Segment, Offset, 0);
                     MmReleasePageMemoryConsumer(MC_USER, Page);
	          }
               }
            }
            else
            {
               if ((Segment->Flags & MM_PAGEFILE_SEGMENT) ||
                   (Segment->Characteristics & IMAGE_SCN_MEM_SHARED))
               {
	          CHECKPOINT1;
                  if (!PageOut)
                  {
                     if (Dirty)
                     {
                        /*
                         * FIXME:
                         *   We hold all locks. Nobody can do something with the current 
                         *   process and the current segment (also not within an other process).
                         */
                        NTSTATUS Status;
                        Status = MmWriteToSwapPage(SavedSwapEntry, Page);
                        if (!NT_SUCCESS(Status))
                        {
                           DPRINT1("MM: Failed to write to swap page (Status was 0x%.8X)\n", Status);
                           KEBUGCHECK(0);
                        }
                     }
                     MmSetPageEntrySectionSegment(Segment, Offset, MAKE_SWAP_SSE(SavedSwapEntry));
                     MmSetSavedSwapEntryPage(Page, 0);
                  }
	          else
	          {
	             CHECKPOINT1;
//                   MmSetPageEntrySectionSegment(Segment, Offset, 0);	    
	          }
                  MmReleasePageMemoryConsumer(MC_USER, Page);
               }
               else
               {
                  DPRINT1("Found a swapentry for a non private page in an image or data file sgment\n");
                  KEBUGCHECK(0);
               }
            }
         }
      }
   }
   else
   {
      if (Dirty && !(Entry & 0x2))
      {
         MmSetPageEntrySectionSegment(Segment, Offset, Entry | 0x2);
      }
   }
   return (ShareCount > 0);
}

VOID
MmspReleasePages(ULONG PageCount, PPFN_TYPE Pfns)
{
   ULONG i;

   for (i = 0; i < PageCount; i++)
   {
      MmReleasePageMemoryConsumer(MC_USER, Pfns[i]);
   }
}

NTSTATUS
MmspRequestPages(ULONG PageCount, PPFN_TYPE Pfns)
{
   ULONG i;
   NTSTATUS Status;

   for (i = 0; i < PageCount; i++)
   {
      Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Pfns[i]);
      if (!NT_SUCCESS(Status))
      {
         MmspReleasePages(i, Pfns);
	 return Status;
      }
   }
   return STATUS_SUCCESS;
}

NTSTATUS
MmspRawReadPages(PFILE_OBJECT FileObject,
		 ULONG SectorSize,
		 PLARGE_INTEGER FileOffset,
		 ULONG ReadSize,
		 PPFN_TYPE Pfns)
{
   ULONG PageCount = PAGE_ROUND_UP(ReadSize) / PAGE_SIZE;
   ULONG MdlSize;
   ULONG MdlOffset;
   PMDL Mdl;
   PULONG MdlPages;
   ULONG i;
   KEVENT Event;
   IO_STATUS_BLOCK Iosb;
   PFN_TYPE Pfn;
   NTSTATUS Status;
   LARGE_INTEGER ReadOffset;
   ULONG Offset;

   DPRINT("MmspRawReadPages(%x, %x, %x (%I64x), %x, %x)\n",
           FileObject, SectorSize, FileOffset, FileOffset->QuadPart, ReadSize, Pfns);
   DPRINT("%wZ\n", &FileObject->FileName);

   if (FileOffset->u.LowPart % SectorSize == 0 && (PageCount * PAGE_SIZE) % SectorSize == 0)
   {
      DPRINT("%d\n", PageCount);
      Pfn = 0;
      MdlSize = ROUND_UP(ReadSize, SectorSize);
      Mdl = alloca(MmSizeOfMdl(NULL, MdlSize));
      MmInitializeMdl(Mdl, NULL, MdlSize);
      Mdl->MdlFlags |= (MDL_PAGES_LOCKED | MDL_IO_PAGE_READ);
      MdlPages = (PULONG)(Mdl + 1);
      ReadOffset.QuadPart = FileOffset->QuadPart;
      for (i = 0; i < PageCount; i++)
      {
         DPRINT("%x\n", Pfns[i]);
         *MdlPages++ = Pfns[i];
      }
   }
   else
   {
      Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Pfn);
      if (!NT_SUCCESS(Status))
      {
         return Status;
      }
      ReadOffset.u.HighPart = FileOffset->u.HighPart;
      Offset = ReadOffset.u.LowPart = ROUND_DOWN(FileOffset->u.LowPart, SectorSize);
      MdlSize = ROUND_UP(FileOffset->QuadPart + ReadSize, SectorSize) - ReadOffset.QuadPart;
      MdlOffset = PAGE_SIZE - (FileOffset->u.LowPart - Offset);
      Mdl = alloca(MmSizeOfMdl((PVOID)MdlOffset, MdlSize));
      MmInitializeMdl(Mdl, (PVOID)MdlOffset, MdlSize);
      Mdl->MdlFlags |= (MDL_PAGES_LOCKED | MDL_IO_PAGE_READ);
      MdlPages = (PULONG)(Mdl + 1);

      if (Offset != FileOffset->u.LowPart)
      {
         *MdlPages++ = Pfn;
	 if (FileOffset->u.LowPart - Offset > PAGE_SIZE)
	 {
	    MdlSize -= PAGE_SIZE;
	    Offset += PAGE_SIZE;
	 }
	 else
	 {
	    MdlSize -= (FileOffset->u.LowPart - Offset);
	    Offset = FileOffset->u.LowPart;
	 }
      }
      i = 0;
      while (MdlSize > 0)
      {
         if (i < PageCount)
	 {
	    *MdlPages++ = Pfns[i++];
	 }
	 else
	 {
	    *MdlPages++ = Pfn;
	 }
	 if (MdlSize > PAGE_SIZE)
	 {
	    MdlSize -= PAGE_SIZE;
	 }
	 else
	 {
	    MdlSize = 0;
	 }
      }
   }
   KeInitializeEvent(&Event, NotificationEvent, FALSE);
   Status = IoPageRead(FileObject, Mdl, &ReadOffset, &Event, &Iosb);
   if (Status == STATUS_PENDING)
   {
      KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
      Status = Iosb.Status;
   }
   if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA)
   {
      MmUnmapLockedPages(Mdl->MappedSystemVa, Mdl);
   }
   if (Pfn)
   {
      MmReleasePageMemoryConsumer(MC_USER, Pfn);
   }
   return Status;
}

NTSTATUS
MmspReadSectionSegmentPages(PSECTION_DATA SectionData,
			    ULONG SegOffset,
			    ULONG PageCount,
			    PPFN_TYPE Pages)
{
   NTSTATUS Status;
   ULONG SectorSize;
   LARGE_INTEGER FileOffset;
   ULONG Length;

   Status = MmspRequestPages(PageCount, Pages);
   if (!NT_SUCCESS(Status))
   {
      return Status;
   }

   SectorSize = SectionData->Segment->BytesPerSector;
   if (SectorSize == 0)
   {
      SectorSize = 512;
   }

   FileOffset.QuadPart = SegOffset + SectionData->Segment->FileOffset;
   Length = PageCount * PAGE_SIZE;
   if (SegOffset > SectionData->Segment->RawLength)
   {
      KEBUGCHECK(0);
   }
   if (Length + SegOffset > SectionData->Segment->RawLength)
   {
      Length = SectionData->Segment->RawLength - SegOffset;
   }
   

   Status = MmspRawReadPages(SectionData->Section->FileObject,
                             SectorSize,
			     &FileOffset,
			     Length,
			     Pages);
   if (!NT_SUCCESS(Status) && SectionData->Segment->BytesPerSector == 0)
   {
      NTSTATUS tmpStatus;
      FILE_FS_SIZE_INFORMATION FileFsSize;

      tmpStatus = IoQueryVolumeInformation(SectionData->Section->FileObject,
                                           FileFsSizeInformation,
				           sizeof(FILE_FS_SIZE_INFORMATION),
                                           &FileFsSize,
				           NULL);

      if (NT_SUCCESS(tmpStatus))
      {
         DPRINT("%d\n", FileFsSize.BytesPerSector);
         SectionData->Segment->BytesPerSector = FileFsSize.BytesPerSector;
	 if (FileFsSize.BytesPerSector != 512)
	 {
            Status = MmspRawReadPages(SectionData->Section->FileObject,
                                      FileFsSize.BytesPerSector,
			              &FileOffset,
			              PageCount * PAGE_SIZE,
			              Pages);
	 }
      }
   }
   if (!NT_SUCCESS(Status))
   {
      MmspReleasePages(PageCount, Pages);
   }
   return Status;
}

/*****************************************************************************************************/

NTSTATUS
MmspNotPresentFaultPhysMemSectionView(PMADDRESS_SPACE AddressSpace,
				      MEMORY_AREA* MemoryArea,
				      PVOID Address,
				      BOOLEAN Locked)
{
   PVOID PAddress;
   ULONG Offset;
   PFN_TYPE Pfn;
   NTSTATUS Status;
   PMM_SECTION_SEGMENT Segment;
   PMM_REGION Region;

   PAddress = MM_ROUND_DOWN(Address, PAGE_SIZE);
   Offset = (ULONG_PTR)PAddress - (ULONG_PTR)MemoryArea->StartingAddress;

   Segment = MemoryArea->Data.SectionData.Segment;
   Region = MmFindRegion(MemoryArea->StartingAddress,
                         &MemoryArea->Data.SectionData.RegionListHead,
                         Address, NULL);

   /* 
    * FIXME:
    *   It is possible, that a phys mem section has a swap entry or is COW protected ?
    */

   /*
    * Just map the desired physical page
    */
   Pfn = (Offset + MemoryArea->Data.SectionData.ViewOffset) >> PAGE_SHIFT;
   Status = MmCreateVirtualMapping(AddressSpace->Process,
                                   Address,
                                   Region->Protect,
                                   &Pfn,
                                   1);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("MmCreateVirtualMapping failed, not out of memory\n");
      KEBUGCHECK(0);
      return(Status);
   }

   /*
    * Don't add an rmap entry since the page mapped could be for
    * anything.
    */
   if (Locked)
   {
      MmLockPage(Pfn);
   }

   /*
    * Cleanup and release locks
    */
   DPRINT("Address 0x%.8X\n", Address);
   return(STATUS_SUCCESS);
}

/*****************************************************************************************************/

NTSTATUS
MmspNotPresentFaultImageSectionView(PMADDRESS_SPACE AddressSpace,
				    MEMORY_AREA* MemoryArea,
				    PVOID Address,
				    BOOLEAN Locked)
{
   PVOID PAddress;
   ULONG SectionOffset;
   ULONG SegmentOffset;
   PSECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;
   PMM_REGION Region;
   ULONG Attributes;
   ULONG Entry;
   PMM_PAGEOP PageOp[4];
//   PMM_PAGEOP tmpPageOp;
   PVOID RegionBase;
   NTSTATUS Status = STATUS_SUCCESS;
   BOOL HasSwapEntry;
   PFN_TYPE Pfn[4];
   ULONG PageCount;
   ULONG i;

   DPRINT("MmspNotPresentFaultImageSectionView(%x, %x, %x, %d)\n", 
           AddressSpace, MemoryArea, Address, Locked);

   PAddress = MM_ROUND_DOWN(Address, PAGE_SIZE);
   SegmentOffset = (ULONG_PTR)PAddress - (ULONG_PTR)MemoryArea->StartingAddress;
   SectionOffset = SegmentOffset + MemoryArea->Data.SectionData.ViewOffset;

   Segment = MemoryArea->Data.SectionData.Segment;
   Section = MemoryArea->Data.SectionData.Section;
   Region = MmFindRegion(MemoryArea->StartingAddress,
                         &MemoryArea->Data.SectionData.RegionListHead,
                         Address, &RegionBase);

   DPRINT("SegmentOffset %x, SectionOffset %x, Address %x, StartingAddress %x\n", SegmentOffset, SectionOffset, Address, MemoryArea->StartingAddress);
   /*
    * Lock the segment
    */
   MmLockSectionSegment(Segment);

   /*
    * Check if this page needs to be mapped COW
    */
   if ((Segment->WriteCopy /*|| MemoryArea->Data.SectionData.WriteCopyView*/) &&
       (Region->Protect == PAGE_READWRITE ||
       Region->Protect == PAGE_EXECUTE_READWRITE))
   {
      Attributes = Region->Protect == PAGE_READWRITE ? PAGE_READONLY : PAGE_EXECUTE_READ;
   }
   else
   {
      Attributes = Region->Protect;
   }

   /*
    * Get or create a page operation descriptor
    */
   PageOp[0] = MmGetPageOp(MemoryArea, NULL, 0, Segment, SegmentOffset, MM_PAGEOP_PAGEIN, FALSE);
   if (PageOp[0] == NULL)
   {
      DPRINT1("MmGetPageOp failed\n");
      KEBUGCHECK(0);
   }

   /*
    * Check if someone else is already handling this fault, if so wait
    * for them
    */
   if (PageOp[0]->Thread != PsGetCurrentThread())
   {
      MmUnlockSectionSegment(Segment);
      MmUnlockAddressSpace(AddressSpace);
      Status = MmspWaitForPageOpCompletionEvent(PageOp[0]);
      /*
       * Check for various strange conditions
       */
      if (Status != STATUS_SUCCESS)
      {
         DPRINT1("Failed to wait for page op, status = %x\n", Status);
         KEBUGCHECK(0);
      }
      if (PageOp[0]->Status == STATUS_PENDING)
      {
         DPRINT1("Woke for page op before completion\n");
         KEBUGCHECK(0);
      }
      MmLockAddressSpace(AddressSpace);
      /*
       * If this wasn't a pagein then restart the operation
       */
      if (PageOp[0]->OpType != MM_PAGEOP_PAGEIN)
      {
         MmspCompleteAndReleasePageOp(PageOp[0]);
         DPRINT("Address 0x%.8X\n", Address);
         return(STATUS_MM_RESTART_OPERATION);
      }

      /*
       * If the thread handling this fault has failed then we don't retry
       */
      if (!NT_SUCCESS(PageOp[0]->Status))
      {
         Status = PageOp[0]->Status;
         MmspCompleteAndReleasePageOp(PageOp[0]);
         DPRINT("Address 0x%.8X\n", Address);
         return(Status);
      }
      MmLockSectionSegment(Segment);
      /*
       * If the completed fault was for another address space then set the
       * page in this one.
       */
      if (!MmIsPagePresent(AddressSpace->Process, Address))
      {
         HasSwapEntry = MmIsPageSwapEntry(AddressSpace->Process, (PVOID)PAddress);
	 if (!HasSwapEntry)
	 {
             Entry = MmGetPageEntrySectionSegment(Segment, SegmentOffset);
             Pfn[0] = PFN_FROM_SSE(Entry);
	 }
         if (HasSwapEntry || Pfn[0] == 0)
         {
            /*
             * The page was a private page in another or in our address space
             */
            MmUnlockSectionSegment(Segment);
            MmspCompleteAndReleasePageOp(PageOp[0]);
            return(STATUS_MM_RESTART_OPERATION);
         }

         MmSharePageEntrySectionSegment(Segment, SegmentOffset);

         /* FIXME: Should we call MmCreateVirtualMappingUnsafe if
          * (Section->AllocationAttributes & SEC_PHYSICALMEMORY) is true?
          */
         Status = MmCreateVirtualMapping(MemoryArea->Process,
                                         Address,
                                         Attributes,
                                         &Pfn[0],
                                         1);
         if (!NT_SUCCESS(Status))
         {
            DbgPrint("Unable to create virtual mapping\n");
            KEBUGCHECK(0);
         }
         MmInsertRmap(Pfn[0], MemoryArea->Process, (PVOID)PAddress);
      }
      if (Locked)
      {
         MmLockPage(Pfn[0]);
      }
      MmUnlockSectionSegment(Segment);
      MmspCompleteAndReleasePageOp(PageOp[0]);
      DPRINT("Address 0x%.8X\n", Address);
      return(STATUS_SUCCESS);
   }

   /* 
    * Check for swapped out private page 
    */
   if (MmIsPageSwapEntry(AddressSpace->Process, (PVOID)PAddress))
   {
      /*
       * Must be private page we have swapped out.
       */
      SWAPENTRY SwapEntry;

      /*
       * Sanity check
       */
      if (Segment->Flags & MM_PAGEFILE_SEGMENT)
      {
         DPRINT1("Found a swaped out private page in a pagefile section.\n");
         KEBUGCHECK(0);
      }

      MmUnlockSectionSegment(Segment);
      MmDeletePageFileMapping(AddressSpace->Process, (PVOID)PAddress, &SwapEntry);

      MmUnlockAddressSpace(AddressSpace);
      Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Pfn[0]);
      if (!NT_SUCCESS(Status))
      {
         KEBUGCHECK(0);
      }

      Status = MmReadFromSwapPage(SwapEntry, Pfn[0]);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("MmReadFromSwapPage failed, status = %x\n", Status);
         KEBUGCHECK(0);
      }
      MmLockAddressSpace(AddressSpace);
      Status = MmCreateVirtualMapping(AddressSpace->Process,
                                      Address,
                                      Region->Protect,
                                      &Pfn[0],
                                      1);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("MmCreateVirtualMapping failed, not out of memory\n");
         KEBUGCHECK(0);
         return(Status);
      }

      /*
       * Store the swap entry for later use.
       */
      MmSetSavedSwapEntryPage(Pfn[0], SwapEntry);

      /*
       * Add the page to the process's working set
       */
      MmInsertRmap(Pfn[0], AddressSpace->Process, (PVOID)PAddress);

      /*
       * Finish the operation
       */
      if (Locked)
      {
         MmLockPage(Pfn[0]);
      }
      PageOp[0]->Status = STATUS_SUCCESS;
      MmspCompleteAndReleasePageOp(PageOp[0]);
      DPRINT("Address 0x%.8X\n", Address);
      return(STATUS_SUCCESS);
   }

   /*
    * Get the entry corresponding to the offset within the section
    */
   Entry = MmGetPageEntrySectionSegment(Segment, SegmentOffset);

   /*
    * Map anonymous memory for BSS sections
    */
   if (Segment->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ||
       (Entry == 0 && SegmentOffset >= PAGE_ROUND_UP(Segment->RawLength)))
   {
      MmUnlockSectionSegment(Segment);
      MmUnlockAddressSpace(AddressSpace);
      Status = MmRequestPageMemoryConsumer(MC_USER, FALSE, &Pfn[0]);
      if (!NT_SUCCESS(Status))
      {
         KEBUGCHECK(0);
      }
      MmLockAddressSpace(AddressSpace);
      Status = MmCreateVirtualMappingUnsafe(AddressSpace->Process,
                                            Address,
                                            Region->Protect,
                                            &Pfn[0],
                                            1);
      if (!NT_SUCCESS(Status))
      {
         DPRINT("MmCreateVirtualMappingUnsafe failed, not out of memory\n");
         KEBUGCHECK(0);
         return(Status);
      }
      MmInsertRmap(Pfn[0], AddressSpace->Process, (PVOID)PAddress);
      if (Locked)
      {
         MmLockPageUnsafe(Pfn[0]);
      }

      /*
       * Cleanup and release locks
       */
      PageOp[0]->Status = STATUS_SUCCESS;
      MmspCompleteAndReleasePageOp(PageOp[0]);
      DPRINT("Address 0x%.8X\n", Address);
      return(STATUS_SUCCESS);
   }
   
   if (Entry == 0)
   {
      /*
       * If the entry is zero (and it can't change because we have
       * locked the segment) then we need to load the page.
       */
      PageCount = 1;
      while (PageCount < 4)
      {
	 if (SegmentOffset + PageCount * PAGE_SIZE < Segment->RawLength &&
	     SegmentOffset + PageCount * PAGE_SIZE < RegionBase + Region->Length - MemoryArea->StartingAddress &&
	     0 == MmGetPageEntrySectionSegment(Segment, SegmentOffset + PageCount * PAGE_SIZE))
	 {
	    PageOp[PageCount] = MmGetPageOp(MemoryArea, 0, 0, Segment, SegmentOffset + PageCount * PAGE_SIZE, MM_PAGEOP_PAGEIN, TRUE);
	    if (PageOp[PageCount])
	    {
	      PageCount++;
	      continue;
	    }
	 }
#if 0
	 if (Offset >= PAGE_SIZE &&
	     MemoryArea->StartingAddress + Offset - PAGE_SIZE >= RegionBase &&
	     0 == MmGetPageEntrySectionSegment(Segment, Offset - PAGE_SIZE))
	 {
	    tmpPageOp = MmGetPageOp(MemoryArea, 0, 0, Segment, Offset - PAGE_SIZE, MM_PAGEOP_PAGEIN, TRUE);
	    if (tmpPageOp)
	    {
	       memmove(&PageOp[1], &PageOp[0], sizeof(PMM_PAGEOP) * PageCount);
	       PageOp[0] = tmpPageOp;
	       PageCount++;
	       Offset -= PAGE_SIZE;
	       PAddress -= PAGE_SIZE;
	       continue;
	    }
	 }
#endif
	 break;
      }
      DPRINT("%d %wZ\n", PageCount, &MemoryArea->Data.SectionData.Section->FileObject->FileName);
      
      /*
       * Release all our locks and read in the page from disk
       */
      MmUnlockSectionSegment(Segment);
      MmUnlockAddressSpace(AddressSpace);

      if (SegmentOffset >= PAGE_ROUND_UP(Segment->RawLength))
      {
         for (i = 0; i < PageCount; i++)
	 {
            Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Pfn[i]);
            if (!NT_SUCCESS(Status))
            {
               DPRINT1("MmRequestPageMemoryConsumer failed (Status %x)\n", Status);
            }
	 }
      }
      else
      {
         Status = MmspReadSectionSegmentPages(&MemoryArea->Data.SectionData, SegmentOffset, PageCount, Pfn);
         if (!NT_SUCCESS(Status))
         {
            DPRINT1("mspReadSectionSegmentPages failed (Status %x)\n", Status);
         }
      }
      /*
       * Relock the address space
       */
      MmLockAddressSpace(AddressSpace);
      if (!NT_SUCCESS(Status))
      {
         /*
          * FIXME: What do we know in this case?
          */
         /*
          * Cleanup and release locks
          */
         for (i = 0; i < PageCount; i++)
	 {
            PageOp[i]->Status = Status;
            MmspCompleteAndReleasePageOp(PageOp[i]);
	 }
         DPRINT("Address 0x%.8X\n", Address);
         return(Status);
      }
      /*
       * Relock the segment
       */
      MmLockSectionSegment(Segment);

      /*
       * Check the entry. No one should change the status of a page
       * that has a pending page-in.
       */
      for (i = 0; i < PageCount; i++)
      {
         Entry = MmGetPageEntrySectionSegment(Segment, SegmentOffset + i * PAGE_SIZE);
         if (Entry != 0)
         {
            DbgPrint("Someone changed ppte entry while we slept\n");
            KEBUGCHECK(0);
         }

         /*
          * Mark the offset within the section as having valid, in-memory
          * data
          */
         Entry = MAKE_SSE(Pfn[i] << PAGE_SHIFT, 0);
         MmSharePage(Pfn[i]);
         MmSetPageEntrySectionSegment(Segment, SegmentOffset + i * PAGE_SIZE, Entry);
      }
      Status = MmCreateVirtualMapping(AddressSpace->Process,
                                      MemoryArea->StartingAddress + SegmentOffset,
                                      Attributes,
                                      Pfn,
                                      PageCount);
      MmUnlockSectionSegment(Segment);

      if (!NT_SUCCESS(Status))
      {
         DbgPrint("Unable to create virtual mapping\n");
         KEBUGCHECK(0);
      }
      for (i = 0; i < PageCount; i++)
      {
         MmInsertRmap(Pfn[i], AddressSpace->Process, (PVOID)PAddress + i * PAGE_SIZE);
      }

      if (Locked)
      {
//         MmLockPage(Pfn[((ULONG_PTR)PAddress - (ULONG_PTR)(MemoryArea->StartingAddress + Offset)) / PAGE_SIZE]);
         MmLockPage(Pfn[0]);
      }
      for(i = 0; i < PageCount; i++)
      {
	 PageOp[i]->Status = STATUS_SUCCESS;
         MmspCompleteAndReleasePageOp(PageOp[i]);
      }
      DPRINT("%x %x %x %x Address 0x%.8X\n", *(PULONG)Address, Region->Protect, Attributes, MemoryArea->StartingAddress + SegmentOffset, Address);
      return(STATUS_SUCCESS);
   }
   else if (IS_SWAP_FROM_SSE(Entry))
   {
      SWAPENTRY SwapEntry;

      SwapEntry = SWAPENTRY_FROM_SSE(Entry);

      /*
       * Release all our locks and read in the page from disk
       */
      MmUnlockSectionSegment(Segment);

      MmUnlockAddressSpace(AddressSpace);

      Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Pfn[0]);
      if (!NT_SUCCESS(Status))
      {
         KEBUGCHECK(0);
      }

      Status = MmReadFromSwapPage(SwapEntry, Pfn[0]);
      if (!NT_SUCCESS(Status))
      {
         KEBUGCHECK(0);
      }

      /*
       * Relock the address space and segment
       */
      MmLockAddressSpace(AddressSpace);
      MmLockSectionSegment(Segment);

      /*
       * Check the entry. No one should change the status of a page
       * that has a pending page-in.
       */
      if (Entry != MmGetPageEntrySectionSegment(Segment, SegmentOffset))
      {
         DbgPrint("Someone changed ppte entry while we slept\n");
         KEBUGCHECK(0);
      }

      /*
       * Mark the offset within the section as having valid, in-memory
       * data
       */
      Entry = MAKE_SSE(Pfn[0] << PAGE_SHIFT, 0);
      MmSharePage(Pfn[0]);
      MmSetPageEntrySectionSegment(Segment, SegmentOffset, Entry);
      MmUnlockSectionSegment(Segment);

      /*
       * Save the swap entry.
       */
      MmSetSavedSwapEntryPage(Pfn[0], SwapEntry);
      Status = MmCreateVirtualMapping(AddressSpace->Process,
                                      Address,
                                      Region->Protect,
                                      &Pfn[0],
                                      1);
      if (!NT_SUCCESS(Status))
      {
         DbgPrint("Unable to create virtual mapping\n");
         KEBUGCHECK(0);
      }
      MmInsertRmap(Pfn[0], AddressSpace->Process, (PVOID)PAddress);
      if (Locked)
      {
         MmLockPage(Pfn[0]);
      }
      PageOp[0]->Status = STATUS_SUCCESS;
      MmspCompleteAndReleasePageOp(PageOp[0]);
      DPRINT("Address 0x%.8X\n", Address);
      return(STATUS_SUCCESS);
   }
   else
   {
      /*
       * If the section offset is already in-memory and valid then just
       * take another reference to the page
       */

      Pfn[0] = PFN_FROM_SSE(Entry);

      MmSharePageEntrySectionSegment(Segment, SegmentOffset);
      MmUnlockSectionSegment(Segment);

      Status = MmCreateVirtualMapping(AddressSpace->Process,
                                      Address,
                                      Attributes,
                                      &Pfn[0],
                                      1);
      if (!NT_SUCCESS(Status))
      {
         DbgPrint("Unable to create virtual mapping\n");
         KEBUGCHECK(0);
      }
      MmInsertRmap(Pfn[0], AddressSpace->Process, (PVOID)PAddress);
      if (Locked)
      {
         MmLockPage(Pfn[0]);
      }
      PageOp[0]->Status = STATUS_SUCCESS;
      MmspCompleteAndReleasePageOp(PageOp[0]);
      DPRINT("Address 0x%.8X\n", Address);
      return(STATUS_SUCCESS);
   }
}

/*****************************************************************************************************/

NTSTATUS
MmspNotPresentFaultDataFileSectionView(PMADDRESS_SPACE AddressSpace,
                                       MEMORY_AREA* MemoryArea,
                                       PVOID Address,
                                       BOOLEAN Locked)
{
   PVOID PAddress;
   PVOID StartingAddress = NULL;
   ULONG Offset;
   PSECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;
   PMM_REGION Region;
   ULONG Attributes;
   PMM_PAGEOP PageOp[4];
   NTSTATUS Status;
   ULONG Entry;
   PFN_TYPE Pfn[4];
   PSECTION_DATA SectionData = NULL;
   ULONG PageCount;
   ULONG i;
   PVOID RegionBase;

   DPRINT("MmspNotPresentFaultDataFileSectionView(%x %x %x %d)\n",
           AddressSpace, MemoryArea, Address, Locked);

   PAddress = MM_ROUND_DOWN(Address, PAGE_SIZE);

   if (MemoryArea->Type == MEMORY_AREA_CACHE_SEGMENT)
   {
      ULONG Index = ((ULONG_PTR)PAddress - (ULONG_PTR)CcCacheViewBase) / CACHE_VIEW_SIZE;
      SectionData = &CcCacheViewArray[Index].SectionData;
      StartingAddress = CcCacheViewArray[Index].BaseAddress;

   }
   else if (MemoryArea->Type == MEMORY_AREA_SECTION_VIEW)
   {
      SectionData = &MemoryArea->Data.SectionData;
      StartingAddress = MemoryArea->StartingAddress;
   }
   else
   {
      KEBUGCHECK(0);
   }

   Offset = (ULONG_PTR)PAddress - (ULONG_PTR)StartingAddress + SectionData->ViewOffset;
   Region = MmFindRegion(StartingAddress,
                         &SectionData->RegionListHead,
                         Address, &RegionBase);
   Segment = SectionData->Segment;
   Section = SectionData->Section;

   DPRINT("%x\n", Offset);
   /*
    * Lock the segment
    */
   MmLockSectionSegment(Segment);

   /*
    * Check if this page needs to be mapped COW
    */
   if ((Segment->WriteCopy /*|| MemoryArea->Data.SectionData.WriteCopyView*/) &&
      (Region->Protect == PAGE_READWRITE || Region->Protect == PAGE_EXECUTE_READWRITE))
   {
      Attributes = Region->Protect == PAGE_READWRITE ? PAGE_READONLY : PAGE_EXECUTE_READ;
   }
   else
   {
      Attributes = Region->Protect;
   }

   if (MmIsPageSwapEntry(AddressSpace->Process, (PVOID)PAddress))
   {
      /* 
       * FIXME:
       */
      KEBUGCHECK(0);
   }

   /*
    * Get or create a page operation descriptor
    */
   PageOp[0] = MmGetPageOp(MemoryArea, NULL, 0, Segment, Offset, MM_PAGEOP_PAGEIN, FALSE);
   if (PageOp[0] == NULL)
   {
      DPRINT1("MmGetPageOp failed\n");
      KEBUGCHECK(0);
   }
   
   /*
    * Check if someone else is already handling this fault, if so wait
    * for them
    */
   if (PageOp[0]->Thread != PsGetCurrentThread())
   {
      MmUnlockSectionSegment(Segment);
      MmUnlockAddressSpace(AddressSpace);
      Status = MmspWaitForPageOpCompletionEvent(PageOp[0]);
      /*
       * Check for various strange conditions
       */
      if (Status != STATUS_SUCCESS)
      {
         DPRINT1("Failed to wait for page op, status = %x\n", Status);
         KEBUGCHECK(0);
      }
      if (PageOp[0]->Status == STATUS_PENDING)
      {
         DPRINT1("Woke for page op before completion\n");
         KEBUGCHECK(0);
      }
      MmLockAddressSpace(AddressSpace);
      /*
       * If this wasn't a pagein then restart the operation
       */
      if (PageOp[0]->OpType != MM_PAGEOP_PAGEIN)
      {
         MmspCompleteAndReleasePageOp(PageOp[0]);
         DPRINT("Address 0x%.8X\n", Address);
         return(STATUS_MM_RESTART_OPERATION);
      }

      /*
       * If the thread handling this fault has failed then we don't retry
       */
      if (!NT_SUCCESS(PageOp[0]->Status))
      {
         Status = PageOp[0]->Status;
         MmspCompleteAndReleasePageOp(PageOp[0]);
         DPRINT("Address 0x%.8X\n", Address);
         return(Status);
      }
      MmLockSectionSegment(Segment);
      /*
       * If the completed fault was for another address space then set the
       * page in this one.
       */
      if (!MmIsPagePresent(AddressSpace->Process, Address))
      {
         Entry = MmGetPageEntrySectionSegment(Segment, Offset);
	 if (IS_SWAP_FROM_SSE(Entry))
	 {
	    KEBUGCHECK(0);
	 }
	 Pfn[0] = PFN_FROM_SSE(Entry);
	 if (Pfn[0] == 0) 
	 {
	    KEBUGCHECK(0);
	 }

         MmSharePageEntrySectionSegment(Segment, Offset);
         Status = MmCreateVirtualMapping(AddressSpace->Process,
                                         Address,
                                         Attributes,
                                         &Pfn[0],
                                         1);
         if (!NT_SUCCESS(Status))
         {
            DbgPrint("Unable to create virtual mapping\n");
            KEBUGCHECK(0);
         }
         MmInsertRmap(Pfn[0], AddressSpace->Process, (PVOID)PAddress);
      }
      if (Locked)
      {
         MmLockPage(Pfn[0]);
      }
      MmUnlockSectionSegment(Segment);
      PageOp[0]->Status = STATUS_SUCCESS;
      MmspCompleteAndReleasePageOp(PageOp[0]);
      DPRINT("Address 0x%.8X\n", Address);
      return(STATUS_SUCCESS);
   }

   /*
    * Get the entry corresponding to the offset within the section
    */
   Entry = MmGetPageEntrySectionSegment(Segment, Offset);

   DPRINT("Entry %x\n", Entry);

   if (IS_SWAP_FROM_SSE(Entry))
   {
      KEBUGCHECK(0);
   }

   if (Entry == 0)
   {
      /*
       * If the entry is zero (and it can't change because we have
       * locked the segment) then we need to load the page.
       */
      PageCount = 1;
      while (PageCount < 4)
      {
         if (Offset + PageCount * PAGE_SIZE < Segment->RawLength &&
	     Offset + PageCount * PAGE_SIZE < RegionBase + Region->Length - StartingAddress &&
	     0 == MmGetPageEntrySectionSegment(Segment, Offset + PageCount * PAGE_SIZE))
	 {
	    PageOp[PageCount] = MmGetPageOp(MemoryArea, 0, 0, Segment, Offset + PageCount * PAGE_SIZE, MM_PAGEOP_PAGEIN, TRUE);
	    if (PageOp[PageCount])
	    {
	       PageCount++;
	       continue;
	    }
	 }
         break;
      }

      DPRINT("%d\n", PageCount);
      /*
       * Release all our locks and read in the page from disk
       */
      MmUnlockSectionSegment(Segment);
      MmUnlockAddressSpace(AddressSpace);

      Status = MmspReadSectionSegmentPages(SectionData, Offset, PageCount, Pfn);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("MiReadPage failed (Status %x)\n", Status);
      }
      MmLockAddressSpace(AddressSpace);
      if (!NT_SUCCESS(Status))
      {
         /*
          * FIXME: What do we know in this case?
          */
         /*
          * Cleanup and release locks
          */
	 for (i = 0; i < PageCount; i++)
	 {
            PageOp[i]->Status = Status;
            MmspCompleteAndReleasePageOp(PageOp[i]);
	 }
         DPRINT("Address 0x%.8X\n", Address);
         return(Status);
      }
      /*
       * Relock the segment
       */
      MmLockSectionSegment(Segment);

      /*
       * Check the entry. No one should change the status of a page
       * that has a pending page-in.
       */
      for (i = 0; i < PageCount; i++)
      {
         Entry = MmGetPageEntrySectionSegment(Segment, Offset + i * PAGE_SIZE);
         if (Entry != 0)
         {
            DPRINT1("Someone changed ppte entry while we slept\n");
            KEBUGCHECK(0);
         }

         /*
          * Mark the offset within the section as having valid, in-memory
          * data
          */
         Entry = MAKE_SSE(Pfn[i] << PAGE_SHIFT, 0);
	 ASSERT (Entry);
         MmSharePage(Pfn[i]);
         MmSetPageEntrySectionSegment(Segment, Offset + i * PAGE_SIZE, Entry);
      }
      Status = MmCreateVirtualMapping(AddressSpace->Process,
                                      PAddress,
                                      Attributes,
                                      Pfn,
                                      PageCount);
      MmUnlockSectionSegment(Segment);

      if (!NT_SUCCESS(Status))
      {
         DbgPrint("Unable to create virtual mapping\n");
         KEBUGCHECK(0);
      }
      for (i = 0; i < PageCount; i++)
      {
         MmInsertRmap(Pfn[i], AddressSpace->Process, (PVOID)PAddress + i * PAGE_SIZE);
         if (i == 0 && Locked)
         {
            MmLockPage(Pfn[0]);
         }
         PageOp[i]->Status = STATUS_SUCCESS;
         MmspCompleteAndReleasePageOp(PageOp[i]);
      }
      DPRINT("Address 0x%.8X\n", Address);
      return(STATUS_SUCCESS);
   }
   else
   {
      /*
       * If the section offset is already in-memory and valid then just
       * take another reference to the page
       */

      Pfn[0] = PFN_FROM_SSE(Entry);

      MmSharePageEntrySectionSegment(Segment, Offset);
      MmUnlockSectionSegment(Segment);
      Status = MmCreateVirtualMapping(AddressSpace->Process,
                                      PAddress,
                                      Attributes,
                                      &Pfn[0],
                                      1);
      if (!NT_SUCCESS(Status))
      {
         DbgPrint("Unable to create virtual mapping\n");
         KEBUGCHECK(0);
      }
      MmInsertRmap(Pfn[0], AddressSpace->Process, (PVOID)PAddress);
      if (Locked)
      {
         MmLockPage(Pfn[0]);
      }
      PageOp[0]->Status = STATUS_SUCCESS;
      MmspCompleteAndReleasePageOp(PageOp[0]);
      DPRINT("Address 0x%.8X\n", Address);
      return(STATUS_SUCCESS);
   }
}

/*****************************************************************************************************/
NTSTATUS
MmspNotPresentFaultPageFileSectionView(PMADDRESS_SPACE AddressSpace,
				       MEMORY_AREA* MemoryArea,
				       PVOID Address,
				       BOOLEAN Locked)
{
   PVOID PAddress;
   ULONG Offset;
   PSECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;
   PMM_REGION Region;
   ULONG Attributes;
   ULONG Entry;
   ULONG Entry1;
   NTSTATUS Status;
   PMM_PAGEOP PageOp;
   PFN_TYPE Pfn;
   
   PAddress = MM_ROUND_DOWN(Address, PAGE_SIZE);
   Offset = (ULONG_PTR)PAddress - (ULONG_PTR)MemoryArea->StartingAddress + MemoryArea->Data.SectionData.ViewOffset;

   Segment = MemoryArea->Data.SectionData.Segment;
   Section = MemoryArea->Data.SectionData.Section;
   Region = MmFindRegion(MemoryArea->StartingAddress,
                         &MemoryArea->Data.SectionData.RegionListHead,
                         Address, NULL);
   /*
    * Lock the segment
    */
   MmLockSectionSegment(Segment);

   if (MmIsPageSwapEntry(AddressSpace->Process, (PVOID)PAddress))
   {
      KEBUGCHECK(0);
   }
      
   /*
    * Check if this page needs to be mapped COW
    */
   if ((Segment->WriteCopy /*|| MemoryArea->Data.SectionData.WriteCopyView*/) &&
       (Region->Protect == PAGE_READWRITE ||
       Region->Protect == PAGE_EXECUTE_READWRITE))
   {
      Attributes = Region->Protect == PAGE_READWRITE ? PAGE_READONLY : PAGE_EXECUTE_READ;
   }
   else
   {
      Attributes = Region->Protect;
   }

   /*
    * Get or create a page operation descriptor
    */
   PageOp = MmGetPageOp(MemoryArea, NULL, 0, Segment, Offset, MM_PAGEOP_PAGEIN, FALSE);
   if (PageOp == NULL)
   {
      DPRINT1("MmGetPageOp failed\n");
      KEBUGCHECK(0);
   }

   /*
    * Check if someone else is already handling this fault, if so wait
    * for them
    */
   if (PageOp->Thread != PsGetCurrentThread())
   {
      MmUnlockSectionSegment(Segment);
      MmUnlockAddressSpace(AddressSpace);
      Status = MmspWaitForPageOpCompletionEvent(PageOp);
      /*
       * Check for various strange conditions
       */
      if (Status != STATUS_SUCCESS)
      {
         DPRINT1("Failed to wait for page op, status = %x\n", Status);
         KEBUGCHECK(0);
      }
      if (PageOp->Status == STATUS_PENDING)
      {
         DPRINT1("Woke for page op before completion\n");
         KEBUGCHECK(0);
      }
      MmLockAddressSpace(AddressSpace);
      /*
       * If this wasn't a pagein then restart the operation
       */
      if (PageOp->OpType != MM_PAGEOP_PAGEIN)
      {
         MmspCompleteAndReleasePageOp(PageOp);
         DPRINT("Address 0x%.8X\n", Address);
         return(STATUS_MM_RESTART_OPERATION);
      }

      /*
       * If the thread handling this fault has failed then we don't retry
       */
      if (!NT_SUCCESS(PageOp->Status))
      {
         Status = PageOp->Status;
         MmspCompleteAndReleasePageOp(PageOp);
         DPRINT("Address 0x%.8X\n", Address);
         return(Status);
      }
      MmLockSectionSegment(Segment);
      /*
       * If the completed fault was for another address space then set the
       * page in this one.
       */
      if (!MmIsPagePresent(AddressSpace->Process, Address))
      {
         Entry = MmGetPageEntrySectionSegment(Segment, Offset);
	 if (IS_SWAP_FROM_SSE(Entry))
	 {
	    KEBUGCHECK(0);
	 }
	 Pfn = PFN_FROM_SSE(Entry);
	 if (Pfn == 0) 
	 {
	    KEBUGCHECK(0);
	 }

         Pfn = PFN_FROM_SSE(Entry);

         MmSharePageEntrySectionSegment(Segment, Offset);

         Status = MmCreateVirtualMapping(MemoryArea->Process,
                                         Address,
                                         Attributes,
                                         &Pfn,
                                         1);
         if (!NT_SUCCESS(Status))
         {
            DbgPrint("Unable to create virtual mapping\n");
            KEBUGCHECK(0);
         }
         MmInsertRmap(Pfn, MemoryArea->Process, (PVOID)PAddress);
      }
      if (Locked)
      {
         MmLockPage(Pfn);
      }
      MmUnlockSectionSegment(Segment);
      MmspCompleteAndReleasePageOp(PageOp);
      DPRINT("Address 0x%.8X\n", Address);
      return(STATUS_SUCCESS);
   }
   /*
    * Get the entry corresponding to the offset within the section
    */
   Entry = MmGetPageEntrySectionSegment(Segment, Offset);

   if (Entry == 0)
   {
      /*
       * If the entry is zero (and it can't change because we have
       * locked the segment) then we need to load the page.
       */
      /*
       * Release all our locks and read in the page from disk
       */
      MmUnlockSectionSegment(Segment);
      MmUnlockAddressSpace(AddressSpace);

      Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Pfn);
      if (!NT_SUCCESS(Status))
      {
         DPRINT("MmRequestPageMemoryConsumer failed (Status %x)\n", Status);
         /*
          * FIXME: What do we know in this case?
          */
         /*
          * Cleanup and release locks
          */
         MmLockAddressSpace(AddressSpace);
         PageOp->Status = Status;
         MmspCompleteAndReleasePageOp(PageOp);
         DPRINT("Address 0x%.8X\n", Address);
         return(Status);
      }
      /*
       * Relock the address space and segment
       */
      MmLockAddressSpace(AddressSpace);
      MmLockSectionSegment(Segment);

      /*
       * Check the entry. No one should change the status of a page
       * that has a pending page-in.
       */
      Entry = MmGetPageEntrySectionSegment(Segment, Offset);
      if (Entry != 0)
      {
         DbgPrint("Someone changed ppte entry while we slept\n");
         KEBUGCHECK(0);
      }

      /*
       * Mark the offset within the section as having valid, in-memory
       * data
       */
      Entry = MAKE_SSE(Pfn << PAGE_SHIFT, 0);
      DPRINT("%x\n", Pfn);
      MmSharePage(Pfn);
      MmSetPageEntrySectionSegment(Segment, Offset, Entry);
      MmUnlockSectionSegment(Segment);
      Status = MmCreateVirtualMapping(AddressSpace->Process,
                                      PAddress,
                                      Attributes,
                                      &Pfn,
                                      1);

      if (!NT_SUCCESS(Status))
      {
         DbgPrint("Unable to create virtual mapping\n");
         KEBUGCHECK(0);
      }
      MmInsertRmap(Pfn, AddressSpace->Process, PAddress);

      if (Locked)
      {
         MmLockPage(Pfn);
      }
      PageOp->Status = STATUS_SUCCESS;
      MmspCompleteAndReleasePageOp(PageOp);
      
      DPRINT("Address 0x%.8X\n", Address);
      return(STATUS_SUCCESS);
   }
   else if (IS_SWAP_FROM_SSE(Entry))
   {
      SWAPENTRY SwapEntry;

      SwapEntry = SWAPENTRY_FROM_SSE(Entry);

      /*
      * Release all our locks and read in the page from disk
      */
      MmUnlockSectionSegment(Segment);

      MmUnlockAddressSpace(AddressSpace);

      Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &Pfn);
      if (!NT_SUCCESS(Status))
      {
         KEBUGCHECK(0);
      }

      Status = MmReadFromSwapPage(SwapEntry, Pfn);
      if (!NT_SUCCESS(Status))
      {
         KEBUGCHECK(0);
      }

      /*
       * Relock the address space and segment
       */
      MmLockAddressSpace(AddressSpace);
      MmLockSectionSegment(Segment);

      /*
       * Check the entry. No one should change the status of a page
       * that has a pending page-in.
       */
      Entry1 = MmGetPageEntrySectionSegment(Segment, Offset);
      if (Entry != Entry1)
      {
         DbgPrint("Someone changed ppte entry while we slept\n");
         KEBUGCHECK(0);
      }

      /*
       * Mark the offset within the section as having valid, in-memory
       * data
       */
      Entry = MAKE_SSE(Pfn << PAGE_SHIFT, 0);
      MmSharePage(Pfn);
      MmSetPageEntrySectionSegment(Segment, Offset, Entry);
      MmUnlockSectionSegment(Segment);

      /*
       * Save the swap entry.
       */
      MmSetSavedSwapEntryPage(Pfn, SwapEntry);
      Status = MmCreateVirtualMapping(AddressSpace->Process,
                                      PAddress,
                                      Region->Protect,
                                      &Pfn,
                                      1);
      if (!NT_SUCCESS(Status))
      {
         DbgPrint("Unable to create virtual mapping\n");
         KEBUGCHECK(0);
      }
      MmInsertRmap(Pfn, AddressSpace->Process, (PVOID)PAddress);
      if (Locked)
      {
         MmLockPage(Pfn);
      }
      PageOp->Status = STATUS_SUCCESS;
      MmspCompleteAndReleasePageOp(PageOp);
      DPRINT("Address 0x%.8X\n", Address);
      return(STATUS_SUCCESS);
   }
   else
   {
      /*
       * If the section offset is already in-memory and valid then just
       * take another reference to the page
       */

      Pfn = PFN_FROM_SSE(Entry);

      MmSharePageEntrySectionSegment(Segment, Offset);
      MmUnlockSectionSegment(Segment);

      Status = MmCreateVirtualMapping(AddressSpace->Process,
                                      PAddress,
                                      Attributes,
                                      &Pfn,
                                      1);
      if (!NT_SUCCESS(Status))
      {
         DbgPrint("Unable to create virtual mapping\n");
         KEBUGCHECK(0);
      }
      MmInsertRmap(Pfn, AddressSpace->Process, (PVOID)PAddress);
      if (Locked)
      {
         MmLockPage(Pfn);
      }
      PageOp->Status = STATUS_SUCCESS;
      MmspCompleteAndReleasePageOp(PageOp);
      DPRINT("Address 0x%.8X\n", Address);
      return(STATUS_SUCCESS);
   }
}

/*****************************************************************************************************/

NTSTATUS
MmNotPresentFaultSectionView(PMADDRESS_SPACE AddressSpace,
                             MEMORY_AREA* MemoryArea,
                             PVOID Address,
                             BOOLEAN Locked)
{
   /*
    * There is a window between taking the page fault and locking the
    * address space when another thread could load the page so we check
    * that.
    */
   if (MmIsPagePresent(AddressSpace->Process, Address))
   {
      if (Locked)
      {
         MmLockPage(MmGetPfnForProcess(AddressSpace->Process, Address));
      }
      return(STATUS_SUCCESS);
   }
   if (MemoryArea->Type == MEMORY_AREA_CACHE_SEGMENT)
   {
      return MmspNotPresentFaultDataFileSectionView(AddressSpace, MemoryArea, Address, Locked);
   }
   if (MemoryArea->Data.SectionData.Section->AllocationAttributes & SEC_PHYSICALMEMORY)
   {
      return MmspNotPresentFaultPhysMemSectionView(AddressSpace, MemoryArea, Address, Locked);
   }
   else if (MemoryArea->Data.SectionData.Section->AllocationAttributes & SEC_IMAGE)
   {
      return MmspNotPresentFaultImageSectionView(AddressSpace, MemoryArea, Address, Locked);
   }
   else if (MemoryArea->Data.SectionData.Section->FileObject != NULL)
   {
      return MmspNotPresentFaultDataFileSectionView(AddressSpace, MemoryArea, Address, Locked);
   }
   else
   {
      return MmspNotPresentFaultPageFileSectionView(AddressSpace, MemoryArea, Address, Locked);
   }
}

NTSTATUS
MmAccessFaultSectionView(PMADDRESS_SPACE AddressSpace,
                         MEMORY_AREA* MemoryArea,
                         PVOID Address,
                         BOOLEAN Locked)
{
   PMM_SECTION_SEGMENT Segment;
   PSECTION_OBJECT Section;
   PFN_TYPE OldPage;
   PFN_TYPE NewPage;
   NTSTATUS Status;
   PVOID PAddress;
   ULONG Offset;
   PMM_PAGEOP PageOp;
   PMM_REGION Region;
   ULONG Entry;

   DPRINT("MmAccessFaultSectionView(%x, %x, %x, %x)\n", AddressSpace, MemoryArea, Address, Locked);

   /*
    * Check if the page has been paged out or has already been set readwrite
    */
   if (!MmIsPagePresent(AddressSpace->Process, Address) ||
         MmGetPageProtect(AddressSpace->Process, Address) & PAGE_READWRITE)
   {
      DPRINT1("Address 0x%.8X\n", Address);
      return(STATUS_SUCCESS);
   }

   if (MemoryArea->Type != MEMORY_AREA_SECTION_VIEW)
   {
      KEBUGCHECK(0);
   }
   /*
    * Find the offset of the page
    */
   PAddress = MM_ROUND_DOWN(Address, PAGE_SIZE);
   Offset = (ULONG_PTR)PAddress - (ULONG_PTR)MemoryArea->StartingAddress;

   Segment = MemoryArea->Data.SectionData.Segment;
   Section = MemoryArea->Data.SectionData.Section;
   Region = MmFindRegion(MemoryArea->StartingAddress,
                         &MemoryArea->Data.SectionData.RegionListHead,
                         Address, NULL);
   /*
    * Lock the segment
    */
   MmLockSectionSegment(Segment);

   OldPage = MmGetPfnForProcess(NULL, Address);
   Entry = MmGetPageEntrySectionSegment(Segment, Offset + MemoryArea->Data.SectionData.ViewOffset);

   MmUnlockSectionSegment(Segment);

   /*
    * Check if we are doing COW
    */
   if (!((Segment->WriteCopy /*|| MemoryArea->Data.SectionData.WriteCopyView*/) &&
         (Region->Protect == PAGE_READWRITE ||
          Region->Protect == PAGE_EXECUTE_READWRITE)))
   {
      DPRINT1("Address 0x%.8X\n", Address);
      return STATUS_UNSUCCESSFUL;
   }

   if (IS_SWAP_FROM_SSE(Entry) ||
       PFN_FROM_SSE(Entry) != OldPage)
   {
      /* This is a private page. We must only change the page protection. */
      MmSetPageProtect(AddressSpace->Process, PAddress, Region->Protect);
      return(STATUS_SUCCESS);
   }

   /*
    * Get or create a pageop
    */
   PageOp = MmGetPageOp(MemoryArea, NULL, 0, Segment, Offset,
                        MM_PAGEOP_ACCESSFAULT, FALSE);
   if (PageOp == NULL)
   {
      DPRINT1("MmGetPageOp failed\n");
      KEBUGCHECK(0);
   }

   /*
    * Wait for any other operations to complete
    */
   if (PageOp->Thread != PsGetCurrentThread())
   {
      MmUnlockAddressSpace(AddressSpace);
      Status = MmspWaitForPageOpCompletionEvent(PageOp);
      /*
      * Check for various strange conditions
      */
      if (Status == STATUS_TIMEOUT)
      {
         DPRINT1("Failed to wait for page op, status = %x\n", Status);
         KEBUGCHECK(0);
      }
      if (PageOp->Status == STATUS_PENDING)
      {
         DPRINT1("Woke for page op before completion\n");
         KEBUGCHECK(0);
      }
      /*
      * Restart the operation
      */
      MmLockAddressSpace(AddressSpace);
      MmspCompleteAndReleasePageOp(PageOp);
      DPRINT("Address 0x%.8X\n", Address);
      return(STATUS_MM_RESTART_OPERATION);
   }

   /*
    * Release locks now we have the pageop
    */
   MmUnlockAddressSpace(AddressSpace);

   /*
    * Allocate a page
    */
   Status = MmRequestPageMemoryConsumer(MC_USER, TRUE, &NewPage);
   if (!NT_SUCCESS(Status))
   {
      KEBUGCHECK(0);
   }

   /*
    * Copy the old page
    */
   MiCopyFromUserPage(NewPage, PAddress);

   /*
    * Delete the old entry.
    */
   MmDeleteVirtualMapping(AddressSpace->Process, Address, FALSE, NULL, NULL);

   /*
    * Set the PTE to point to the new page
    */
   MmLockAddressSpace(AddressSpace);
   Status = MmCreateVirtualMapping(AddressSpace->Process,
                                   Address,
                                   Region->Protect,
                                   &NewPage,
                                   1);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("MmCreateVirtualMapping failed, not out of memory\n");
      KEBUGCHECK(0);
      return(Status);
   }
   if (!NT_SUCCESS(Status))
   {
      DbgPrint("Unable to create virtual mapping\n");
      KEBUGCHECK(0);
   }
   if (Locked)
   {
      MmLockPage(NewPage);
      MmUnlockPage(OldPage);
   }

   /*
    * Unshare the old page.
    */
   MmDeleteRmap(OldPage, AddressSpace->Process, PAddress);
   MmInsertRmap(NewPage, AddressSpace->Process, PAddress);
   MmLockSectionSegment(Segment);
   MmUnsharePageEntrySectionSegment(Section, Segment, Offset, FALSE, FALSE);
   MmUnlockSectionSegment(Segment);

   PageOp->Status = STATUS_SUCCESS;
   MmspCompleteAndReleasePageOp(PageOp);
   DPRINT("Address 0x%.8X\n", Address);
   return(STATUS_SUCCESS);
}

VOID
MmPageOutDeleteMapping(PVOID Context, PEPROCESS Process, PVOID Address)
{
   MM_SECTION_PAGEOUT_CONTEXT* PageOutContext;
   BOOL WasDirty;
   PFN_TYPE Page;

   PageOutContext = (MM_SECTION_PAGEOUT_CONTEXT*)Context;
   MmDeleteVirtualMapping(Process,
                          Address,
                          FALSE,
                          &WasDirty,
                          &Page);
   if (WasDirty)
   {
      PageOutContext->WasDirty = TRUE;
   }
   if (!PageOutContext->Private)
   {
      MmUnsharePageEntrySectionSegment(PageOutContext->Section,
                                       PageOutContext->Segment,
                                       PageOutContext->Offset,
                                       PageOutContext->WasDirty,
                                       TRUE);
   }
   else
   {
      MmReleasePageMemoryConsumer(MC_USER, Page);
   }

   DPRINT("PhysicalAddress %I64x, Address %x\n", Page, Address);
}

NTSTATUS
MmPageOutSectionView(PMADDRESS_SPACE AddressSpace,
                     MEMORY_AREA* MemoryArea,
                     PVOID Address,
                     PMM_PAGEOP PageOp)
{
   PFN_TYPE Page;
   MM_SECTION_PAGEOUT_CONTEXT Context;
   SWAPENTRY SwapEntry;
   ULONG Entry;
   ULONG FileOffset;
   NTSTATUS Status;
   PFILE_OBJECT FileObject;
   BOOLEAN IsImageSection;

   Address = (PVOID)PAGE_ROUND_DOWN(Address);

   /*
    * Get the segment and section.
    */
   Context.Segment = MemoryArea->Data.SectionData.Segment;
   Context.Section = MemoryArea->Data.SectionData.Section;

   Context.Offset = (ULONG_PTR)Address - (ULONG_PTR)MemoryArea->StartingAddress;
   FileOffset = Context.Offset + Context.Segment->FileOffset;

   IsImageSection = Context.Section->AllocationAttributes & SEC_IMAGE ? TRUE : FALSE;

   FileObject = Context.Section->FileObject;

   /*
    * This should never happen since mappings of physical memory are never
    * placed in the rmap lists.
    */
   if (Context.Section->AllocationAttributes & SEC_PHYSICALMEMORY)
   {
      DPRINT1("Trying to page out from physical memory section address 0x%X "
              "process %d\n", Address,
              AddressSpace->Process ? AddressSpace->Process->UniqueProcessId : 0);
      KEBUGCHECK(0);
   }

   /*
    * Get the section segment entry and the physical address.
    */
   Entry = MmGetPageEntrySectionSegment(Context.Segment, Context.Offset);
   if (!MmIsPagePresent(AddressSpace->Process, Address))
   {
      DPRINT1("Trying to page out not-present page at (%d,0x%.8X).\n",
              AddressSpace->Process ? AddressSpace->Process->UniqueProcessId : 0, Address);
      KEBUGCHECK(0);
   }
   Page = MmGetPfnForProcess(AddressSpace->Process, Address);
   SwapEntry = MmGetSavedSwapEntryPage(Page);

   /*
    * Prepare the context structure for the rmap delete call.
    */
   Context.WasDirty = Entry & 0x2 ? TRUE : FALSE;
   if (Context.Segment->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ||
       IS_SWAP_FROM_SSE(Entry) ||
       PFN_FROM_SSE(Entry) != Page)
   {
      Context.Private = TRUE;
   }
   else
   {
      Context.Private = FALSE;
   }

   /*
    * Take an additional reference to the page.
    */
   MmReferencePage(Page);

   MmDeleteAllRmaps(Page, (PVOID)&Context, MmPageOutDeleteMapping);

   /*
    * If this wasn't a private page then we should have reduced the entry to
    * zero by deleting all the rmaps.
    */
   if (!Context.Private && MmGetPageEntrySectionSegment(Context.Segment, Context.Offset) != 0)
   {
      if (!(Context.Segment->Flags & MM_PAGEFILE_SEGMENT) &&
          !(Context.Segment->Characteristics & IMAGE_SCN_MEM_SHARED))
      {
         KEBUGCHECK(0);
      }
   }

   /*
    * If the page wasn't dirty then we can just free it as for a readonly page.
    * Since we unmapped all the mappings above we know it will not suddenly
    * become dirty.
    * If the page is from a pagefile section and has no swap entry,
    * we can't free the page at this point.
    */
   SwapEntry = MmGetSavedSwapEntryPage(Page);
   if (Context.Segment->Flags & MM_PAGEFILE_SEGMENT)
   {
      if (Context.Private)
      {
         DPRINT1("Found a %s private page (address %x) in a pagefile segment.\n",
                 Context.WasDirty ? "dirty" : "clean", Address);
         KEBUGCHECK(0);
      }
      if (!Context.WasDirty && SwapEntry != 0)
      {
         MmSetSavedSwapEntryPage(Page, 0);
         MmSetPageEntrySectionSegment(Context.Segment, Context.Offset, MAKE_SWAP_SSE(SwapEntry));
         MmReleasePageMemoryConsumer(MC_USER, Page);
         PageOp->Status = STATUS_SUCCESS;
         MmspCompleteAndReleasePageOp(PageOp);
         return(STATUS_SUCCESS);
      }
   }
   else if (Context.Segment->Characteristics & IMAGE_SCN_MEM_SHARED)
   {
      if (Context.Private)
      {
         DPRINT1("Found a %s private page (address %x) in a shared section segment.\n",
                 Context.WasDirty ? "dirty" : "clean", Address);
         KEBUGCHECK(0);
      }
      if (!Context.WasDirty || SwapEntry != 0)
      {
         MmSetSavedSwapEntryPage(Page, 0);
         if (SwapEntry != 0)
         {
            MmSetPageEntrySectionSegment(Context.Segment, Context.Offset, MAKE_SWAP_SSE(SwapEntry));
         }
         MmReleasePageMemoryConsumer(MC_USER, Page);
         PageOp->Status = STATUS_SUCCESS;
         MmspCompleteAndReleasePageOp(PageOp);
         return(STATUS_SUCCESS);
      }
   }
   else if (!Context.WasDirty && !Context.Private)
   {
      if (SwapEntry != 0)
      {
         DPRINT1("Found a swap entry for a non dirty, non private and not direct mapped page (address %x)\n",
                 Address);
         KEBUGCHECK(0);
      }
      MmReleasePageMemoryConsumer(MC_USER, Page);
      PageOp->Status = STATUS_SUCCESS;
      MmspCompleteAndReleasePageOp(PageOp);
      return(STATUS_SUCCESS);
   }
   else if (!Context.WasDirty && Context.Private && SwapEntry != 0)
   {
      MmSetSavedSwapEntryPage(Page, 0);
      Status = MmCreatePageFileMapping(AddressSpace->Process,
                                       Address,
                                       SwapEntry);
      if (!NT_SUCCESS(Status))
      {
         KEBUGCHECK(0);
      }
      MmReleasePageMemoryConsumer(MC_USER, Page);
      PageOp->Status = STATUS_SUCCESS;
      MmspCompleteAndReleasePageOp(PageOp);
      return(STATUS_SUCCESS);
   }

   /*
    * If necessary, allocate an entry in the paging file for this page
    */
   if (SwapEntry == 0)
   {
      SwapEntry = MmAllocSwapPage();
      if (SwapEntry == 0)
      {
         MmShowOutOfSpaceMessagePagingFile();

         /*
          * For private pages restore the old mappings.
          */
         if (Context.Private)
         {
            Status = MmCreateVirtualMapping(MemoryArea->Process,
                                            Address,
                                            MemoryArea->Attributes,
                                            &Page,
                                            1);
            MmSetDirtyPage(MemoryArea->Process, Address);
            MmInsertRmap(Page,
                         MemoryArea->Process,
                         Address);
         }
         else
         {
            /*
             * For non-private pages if the page wasn't direct mapped then
             * set it back into the section segment entry so we don't loose
             * our copy. Otherwise it will be handled by the cache manager.
             */
            Status = MmCreateVirtualMapping(MemoryArea->Process,
                                            Address,
                                            MemoryArea->Attributes,
                                            &Page,
                                            1);
            MmSetDirtyPage(MemoryArea->Process, Address);
            MmInsertRmap(Page,
                         MemoryArea->Process,
                         Address);
            Entry = MAKE_SSE(Page << PAGE_SHIFT, 0);
            MmSharePage(Page);
            MmSetPageEntrySectionSegment(Context.Segment, Context.Offset, Entry);
         }
         PageOp->Status = STATUS_UNSUCCESSFUL;
         MmspCompleteAndReleasePageOp(PageOp);
         return(STATUS_PAGEFILE_QUOTA);
      }
   }

   /*
    * Write the page to the pagefile
    */
   Status = MmWriteToSwapPage(SwapEntry, Page);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("MM: Failed to write to swap page (Status was 0x%.8X)\n",
              Status);
      /*
       * As above: undo our actions.
       * FIXME: Also free the swap page.
       */
      if (Context.Private)
      {
         Status = MmCreateVirtualMapping(MemoryArea->Process,
                                         Address,
                                         MemoryArea->Attributes,
                                         &Page,
                                         1);
         MmSetDirtyPage(MemoryArea->Process, Address);
         MmInsertRmap(Page,
                      MemoryArea->Process,
                      Address);
      }
      else
      {
         Status = MmCreateVirtualMapping(MemoryArea->Process,
                                         Address,
                                         MemoryArea->Attributes,
                                         &Page,
                                         1);
         MmSetDirtyPage(MemoryArea->Process, Address);
         MmInsertRmap(Page,
                      MemoryArea->Process,
                      Address);
         Entry = MAKE_SSE(Page << PAGE_SHIFT, 0);
         MmSharePage(Page);
         MmSetPageEntrySectionSegment(Context.Segment, Context.Offset, Entry);
      }
      PageOp->Status = STATUS_UNSUCCESSFUL;
      MmspCompleteAndReleasePageOp(PageOp);
      return(STATUS_UNSUCCESSFUL);
   }

   /*
    * Otherwise we have succeeded.
    */
   DPRINT("MM: Wrote section page 0x%.8X to swap!\n", Page << PAGE_SHIFT);
   MmSetSavedSwapEntryPage(Page, 0);
   if (Context.Segment->Flags & MM_PAGEFILE_SEGMENT ||
         Context.Segment->Characteristics & IMAGE_SCN_MEM_SHARED)
   {
      MmSetPageEntrySectionSegment(Context.Segment, Context.Offset, MAKE_SWAP_SSE(SwapEntry));
   }
   else
   {
      MmReleasePageMemoryConsumer(MC_USER, Page);
   }

   if (Context.Private)
   {
      Status = MmCreatePageFileMapping(MemoryArea->Process,
                                       Address,
                                       SwapEntry);
      if (!NT_SUCCESS(Status))
      {
         KEBUGCHECK(0);
      }
   }
   else
   {
      Entry = MAKE_SWAP_SSE(SwapEntry);
      MmSetPageEntrySectionSegment(Context.Segment, Context.Offset, Entry);
   }

   PageOp->Status = STATUS_SUCCESS;
   MmspCompleteAndReleasePageOp(PageOp);
   return(STATUS_SUCCESS);
}

NTSTATUS
MmWritePageSectionView(PMADDRESS_SPACE AddressSpace,
                       PMEMORY_AREA MemoryArea,
                       PVOID Address,
                       PMM_PAGEOP PageOp)
{
   ULONG Offset;
   PSECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;
   PFN_TYPE Page;
   SWAPENTRY SwapEntry;
   ULONG Entry;
   BOOLEAN Private;
   NTSTATUS Status;
   PFILE_OBJECT FileObject;
   PBCB Bcb = NULL;
   BOOLEAN DirectMapped;
   BOOLEAN IsImageSection;

   Address = (PVOID)PAGE_ROUND_DOWN(Address);

   Offset = (ULONG_PTR)Address - (ULONG_PTR)MemoryArea->StartingAddress;

   /*
    * Get the segment and section.
    */
   Segment = MemoryArea->Data.SectionData.Segment;
   Section = MemoryArea->Data.SectionData.Section;
   IsImageSection = Section->AllocationAttributes & SEC_IMAGE ? TRUE : FALSE;

   FileObject = Section->FileObject;
   DirectMapped = FALSE;
   if (FileObject != NULL &&
         !(Segment->Characteristics & IMAGE_SCN_MEM_SHARED))
   {
      Bcb = FileObject->SectionObjectPointer->SharedCacheMap;

      /*
       * If the file system is letting us go directly to the cache and the
       * memory area was mapped at an offset in the file which is page aligned
       * then note this is a direct mapped page.
       */
      if ((Offset + MemoryArea->Data.SectionData.ViewOffset % PAGE_SIZE) == 0 &&
            (Offset + PAGE_SIZE <= Segment->RawLength || !IsImageSection))
      {
         DirectMapped = TRUE;
      }
   }

   /*
    * This should never happen since mappings of physical memory are never
    * placed in the rmap lists.
    */
   if (Section->AllocationAttributes & SEC_PHYSICALMEMORY)
   {
      DPRINT1("Trying to write back page from physical memory mapped at %X "
              "process %d\n", Address,
              AddressSpace->Process ? AddressSpace->Process->UniqueProcessId : 0);
      KEBUGCHECK(0);
   }

   /*
    * Get the section segment entry and the physical address.
    */
   Entry = MmGetPageEntrySectionSegment(Segment, Offset);
   if (!MmIsPagePresent(AddressSpace->Process, Address))
   {
      DPRINT1("Trying to page out not-present page at (%d,0x%.8X).\n",
              AddressSpace->Process ? AddressSpace->Process->UniqueProcessId : 0, Address);
      KEBUGCHECK(0);
   }
   Page = MmGetPfnForProcess(AddressSpace->Process, Address);
   SwapEntry = MmGetSavedSwapEntryPage(Page);

   /*
    * Check for a private (COWed) page.
    */
   if (Segment->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ||
         IS_SWAP_FROM_SSE(Entry) ||
         PFN_FROM_SSE(Entry) != Page)
   {
      Private = TRUE;
   }
   else
   {
      Private = FALSE;
   }

   /*
    * Speculatively set all mappings of the page to clean.
    */
   MmSetCleanAllRmaps(Page);

   /*
    * If this page was direct mapped from the cache then the cache manager
    * will take care of writing it back to disk.
    */
   if (DirectMapped && !Private)
   {
      ASSERT(SwapEntry == 0);
#if 1
      KEBUGCHECK(0);
#else
      CcRosMarkDirtyCacheSegment(Bcb, Offset + MemoryArea->Data.SectionData.ViewOffset);
#endif
      PageOp->Status = STATUS_SUCCESS;
      MmspCompleteAndReleasePageOp(PageOp);
      return(STATUS_SUCCESS);
   }

   /*
    * If necessary, allocate an entry in the paging file for this page
    */
   if (SwapEntry == 0)
   {
      SwapEntry = MmAllocSwapPage();
      if (SwapEntry == 0)
      {
         MmSetDirtyAllRmaps(Page);
         PageOp->Status = STATUS_UNSUCCESSFUL;
         MmspCompleteAndReleasePageOp(PageOp);
         return(STATUS_PAGEFILE_QUOTA);
      }
      MmSetSavedSwapEntryPage(Page, SwapEntry);
   }

   /*
    * Write the page to the pagefile
    */
   Status = MmWriteToSwapPage(SwapEntry, Page);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("MM: Failed to write to swap page (Status was 0x%.8X)\n",
              Status);
      MmSetDirtyAllRmaps(Page);
      PageOp->Status = STATUS_UNSUCCESSFUL;
      MmspCompleteAndReleasePageOp(PageOp);
      return(STATUS_UNSUCCESSFUL);
   }

   /*
    * Otherwise we have succeeded.
    */
   DPRINT("MM: Wrote section page 0x%.8X to swap!\n", Page << PAGE_SHIFT);
   PageOp->Status = STATUS_SUCCESS;
   MmspCompleteAndReleasePageOp(PageOp);
   return(STATUS_SUCCESS);
}

VOID STATIC
MmAlterViewAttributes(PMADDRESS_SPACE AddressSpace,
                      PVOID BaseAddress,
                      ULONG RegionSize,
                      ULONG OldType,
                      ULONG OldProtect,
                      ULONG NewType,
                      ULONG NewProtect)
{
   PMEMORY_AREA MemoryArea;
   PMM_SECTION_SEGMENT Segment;
   BOOL DoCOW = FALSE;
   ULONG i;

   MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, BaseAddress);
   Segment = MemoryArea->Data.SectionData.Segment;

   if ((Segment->WriteCopy /*|| MemoryArea->Data.SectionData.WriteCopyView*/) &&
         (NewProtect == PAGE_READWRITE || NewProtect == PAGE_EXECUTE_READWRITE))
   {
      DoCOW = TRUE;
   }

   if (OldProtect != NewProtect)
   {
      for (i = 0; i < PAGE_ROUND_UP(RegionSize) / PAGE_SIZE; i++)
      {
         PVOID Address = (char*)BaseAddress + (i * PAGE_SIZE);
         ULONG Protect = NewProtect;

         /*
          * If we doing COW for this segment then check if the page is
          * already private.
          */
         if (DoCOW && MmIsPagePresent(AddressSpace->Process, Address))
         {
            ULONG Offset;
            ULONG Entry;
            PFN_TYPE Page;

            Offset = (ULONG_PTR)Address - (ULONG_PTR)MemoryArea->StartingAddress;
            Entry = MmGetPageEntrySectionSegment(Segment, Offset);
            Page = MmGetPfnForProcess(AddressSpace->Process, Address);

            Protect = PAGE_READONLY;
            if (Segment->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA ||
                  IS_SWAP_FROM_SSE(Entry) ||
                  PFN_FROM_SSE(Entry) != Page)
            {
               Protect = NewProtect;
            }
         }

         if (MmIsPagePresent(AddressSpace->Process, Address))
         {
            MmSetPageProtect(AddressSpace->Process, Address,
                             Protect);
         }
      }
   }
}

NTSTATUS
MmProtectSectionView(PMADDRESS_SPACE AddressSpace,
                     PMEMORY_AREA MemoryArea,
                     PVOID BaseAddress,
                     ULONG Length,
                     ULONG Protect,
                     PULONG OldProtect)
{
   PMM_REGION Region;
   NTSTATUS Status;
   ULONG_PTR MaxLength;

   MaxLength = (ULONG_PTR)MemoryArea->EndingAddress - (ULONG_PTR)BaseAddress;
   if (Length > MaxLength)
      Length = MaxLength;

   Region = MmFindRegion(MemoryArea->StartingAddress,
                         &MemoryArea->Data.SectionData.RegionListHead,
                         BaseAddress, NULL);
   *OldProtect = Region->Protect;
   Status = MmAlterRegion(AddressSpace, MemoryArea->StartingAddress,
                          &MemoryArea->Data.SectionData.RegionListHead,
                          BaseAddress, Length, Region->Type, Protect,
                          MmAlterViewAttributes);

   return(Status);
}

NTSTATUS STDCALL
MmQuerySectionView(PMEMORY_AREA MemoryArea,
                   PVOID Address,
                   PMEMORY_BASIC_INFORMATION Info,
                   PULONG ResultLength)
{
   PMM_REGION Region;
   PVOID RegionBaseAddress;
   PSECTION_OBJECT Section;
   PLIST_ENTRY CurrentEntry;
   PMEMORY_AREA CurrentMArea;
   KIRQL oldIrql;

   Region = MmFindRegion((PVOID)MemoryArea->StartingAddress,
                         &MemoryArea->Data.SectionData.RegionListHead,
                         Address, &RegionBaseAddress);
   if (Region == NULL)
   {
      return STATUS_UNSUCCESSFUL;
   }
   Section = MemoryArea->Data.SectionData.Section;
   if (Section->AllocationAttributes & SEC_IMAGE)
   {
      KeAcquireSpinLock(&Section->ViewListLock, &oldIrql);
      CurrentEntry = Section->ViewListHead.Flink;
      Info->AllocationBase = NULL;
      while (CurrentEntry != &Section->ViewListHead)
      {
         CurrentMArea = CONTAINING_RECORD(CurrentEntry, MEMORY_AREA, Data.SectionData.ViewListEntry);
         CurrentEntry = CurrentEntry->Flink;
         if (Info->AllocationBase == NULL)
         {
            Info->AllocationBase = CurrentMArea->StartingAddress;
         }
         else if (CurrentMArea->StartingAddress < Info->AllocationBase)
         {
            Info->AllocationBase = CurrentMArea->StartingAddress;
         }
      }
      KeReleaseSpinLock(&Section->ViewListLock, oldIrql);
      Info->BaseAddress = RegionBaseAddress;
      Info->AllocationProtect = MemoryArea->Attributes;
      Info->Type = MEM_IMAGE;
   }
   else
   {
      Info->BaseAddress = RegionBaseAddress;
      Info->AllocationBase = MemoryArea->StartingAddress;
      Info->AllocationProtect = MemoryArea->Attributes;
      Info->Type = MEM_MAPPED;
   }
   Info->RegionSize = PAGE_ROUND_UP((ULONG_PTR)MemoryArea->EndingAddress -
                                    (ULONG_PTR)MemoryArea->StartingAddress);
   Info->State = MEM_COMMIT;
   Info->Protect = Region->Protect;

   *ResultLength = sizeof(MEMORY_BASIC_INFORMATION);
   return(STATUS_SUCCESS);
}

VOID
MmpFreePageFileSegment(PMM_SECTION_SEGMENT Segment)
{
   ULONG Length;
   ULONG Offset;
   ULONG Entry;
   ULONG SavedSwapEntry;
   PFN_TYPE Page;

   Page = 0;

   Length = PAGE_ROUND_UP(Segment->Length);
   for (Offset = 0; Offset < Length; Offset += PAGE_SIZE)
   {
      Entry = MmGetPageEntrySectionSegment(Segment, Offset);
      if (Entry)
      {
         if (IS_SWAP_FROM_SSE(Entry))
         {
            MmFreeSwapPage(SWAPENTRY_FROM_SSE(Entry));
         }
         else
         {
            Page = PFN_FROM_SSE(Entry);
            SavedSwapEntry = MmGetSavedSwapEntryPage(Page);
            if (SavedSwapEntry != 0)
            {
               MmSetSavedSwapEntryPage(Page, 0);
               MmFreeSwapPage(SavedSwapEntry);
            }
            MmReleasePageMemoryConsumer(MC_USER, Page);
         }
         MmSetPageEntrySectionSegment(Segment, Offset, 0);
      }
   }
}

VOID STDCALL
MmpDeleteSection(PVOID ObjectBody)
{
   PSECTION_OBJECT Section = (PSECTION_OBJECT)ObjectBody;

   DPRINT("MmpDeleteSection(ObjectBody %x)\n", ObjectBody);
   if (Section->AllocationAttributes & SEC_IMAGE)
   {
      ULONG i;
      ULONG NrSegments;
      ULONG RefCount;
      PMM_SECTION_SEGMENT SectionSegments;

      /*
       * NOTE: Section->ImageSection can be NULL for short time
       * during the section creating. If we fail for some reason
       * until the image section is properly initialized we shouldn't
       * process further here.
       */
      if (Section->ImageSection == NULL)
         return;

      SectionSegments = Section->ImageSection->Segments;
      NrSegments = Section->ImageSection->NrSegments;

      for (i = 0; i < NrSegments; i++)
      {
         if (SectionSegments[i].Characteristics & IMAGE_SCN_MEM_SHARED)
         {
            MmLockSectionSegment(&SectionSegments[i]);
         }
         RefCount = InterlockedDecrementUL(&SectionSegments[i].ReferenceCount);
         if (SectionSegments[i].Characteristics & IMAGE_SCN_MEM_SHARED)
         {
            if (RefCount == 0)
            {
               MmpFreePageFileSegment(&SectionSegments[i]);
            }
            MmUnlockSectionSegment(&SectionSegments[i]);
         }
      }
      ExAcquireFastMutex(&ImageSectionObjectLock);
      Section->ImageSection->RefCount--;
      ExReleaseFastMutex(&ImageSectionObjectLock);
   }
   else
   {
      /*
       * NOTE: Section->Segment can be NULL for short time
       * during the section creating.
       */
      if (Section->Segment == NULL)
         return;

      if (Section->Segment->Flags & MM_PAGEFILE_SEGMENT)
      {
         MmpFreePageFileSegment(Section->Segment);
         MmFreePageTablesSectionSegment(Section->Segment);
         ExFreePool(Section->Segment);
         Section->Segment = NULL;
      }
      else
      {
         InterlockedDecrementUL(&Section->Segment->ReferenceCount);
      }
      if (Section->FileObject != NULL)
      {
//         CcRosDereferenceCache(Section->FileObject);
      }
   }
   if (Section->FileObject != NULL)
   {
      ObDereferenceObject(Section->FileObject);
      Section->FileObject = NULL;
   }
}

VOID STDCALL
MmpCloseSection(PVOID ObjectBody,
                ULONG HandleCount)
{
   DPRINT("MmpCloseSection(OB %x, HC %d) RC %d\n",
           ObjectBody, HandleCount, ObGetObjectPointerCount(ObjectBody));
}

NTSTATUS STDCALL
MmpCreateSection(PVOID ObjectBody,
                 PVOID Parent,
                 PWSTR RemainingPath,
                 POBJECT_ATTRIBUTES ObjectAttributes)
{
   DPRINT("MmpCreateSection(ObjectBody %x, Parent %x, RemainingPath %S)\n",
          ObjectBody, Parent, RemainingPath);

   if (RemainingPath == NULL)
   {
      return(STATUS_SUCCESS);
   }

   if (wcschr(RemainingPath+1, L'\\') != NULL)
   {
      return(STATUS_UNSUCCESSFUL);
   }
   return(STATUS_SUCCESS);
}

NTSTATUS INIT_FUNCTION
MmCreatePhysicalMemorySection(VOID)
{
   PSECTION_OBJECT PhysSection;
   NTSTATUS Status;
   OBJECT_ATTRIBUTES Obj;
   UNICODE_STRING Name = ROS_STRING_INITIALIZER(L"\\Device\\PhysicalMemory");
   LARGE_INTEGER SectionSize;

   /*
    * Create the section mapping physical memory
    */
   SectionSize.QuadPart = 0xFFFFFFFF;
   InitializeObjectAttributes(&Obj,
                              &Name,
                              OBJ_PERMANENT,
                              NULL,
                              NULL);
   Status = MmCreateSection(&PhysSection,
                            SECTION_ALL_ACCESS,
                            &Obj,
                            &SectionSize,
                            PAGE_EXECUTE_READWRITE,
                            0,
                            NULL,
                            NULL);
   if (!NT_SUCCESS(Status))
   {
      DbgPrint("Failed to create PhysicalMemory section\n");
      KEBUGCHECK(0);
   }
   PhysSection->AllocationAttributes |= SEC_PHYSICALMEMORY;

   return(STATUS_SUCCESS);
}


VOID 
MmInitSectionImplementation2(VOID)
{
   LARGE_INTEGER DueTime;
   HANDLE ThreadHandle;
   CLIENT_ID ThreadId;
   NTSTATUS Status;

   DueTime.QuadPart = -1;
   KeInitializeTimerEx(&MmspWorkerThreadTimer, SynchronizationTimer);

VOID STDCALL
MmspWorkerThread(PVOID);

   Status = PsCreateSystemThread(&ThreadHandle,
				 THREAD_ALL_ACCESS,
				 NULL,
				 NULL,
				 &ThreadId,
				 MmspWorkerThread,
				 NULL);

   KeSetTimerEx(&MmspWorkerThreadTimer, DueTime, 5000, NULL);
}

NTSTATUS INIT_FUNCTION
MmInitSectionImplementation(VOID)
{
   MmSectionObjectType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));

   RtlInitUnicodeString(&MmSectionObjectType->TypeName, L"Section");

   MmSectionObjectType->Tag = TAG('S', 'E', 'C', 'T');
   MmSectionObjectType->TotalObjects = 0;
   MmSectionObjectType->TotalHandles = 0;
   MmSectionObjectType->PeakObjects = 0;
   MmSectionObjectType->PeakHandles = 0;
   MmSectionObjectType->PagedPoolCharge = 0;
   MmSectionObjectType->NonpagedPoolCharge = sizeof(SECTION_OBJECT);
   MmSectionObjectType->Mapping = &MmpSectionMapping;
   MmSectionObjectType->Dump = NULL;
   MmSectionObjectType->Open = NULL;
   MmSectionObjectType->Close = MmpCloseSection;
   MmSectionObjectType->Delete = MmpDeleteSection;
   MmSectionObjectType->Parse = NULL;
   MmSectionObjectType->Security = NULL;
   MmSectionObjectType->QueryName = NULL;
   MmSectionObjectType->OkayToClose = NULL;
   MmSectionObjectType->Create = MmpCreateSection;
   MmSectionObjectType->DuplicationNotify = NULL;

   /*
    * NOTE: Do not register the section object type here because
    * the object manager it not initialized yet!
    * The section object type will be created in ObInit().
    */
   ObpCreateTypeObject(MmSectionObjectType);

   InitializeListHead(&ImageSectionObjectListHead);
   ImageSectionObjectNext = NULL;
   ImageSectionObjectCount = 0;
   ExInitializeFastMutex(&ImageSectionObjectLock);

   InitializeListHead(&DataSectionObjectListHead);
   DataSectionObjectCount = 0;
   ExInitializeFastMutex(&DataSectionObjectLock);



   return(STATUS_SUCCESS);
}

NTSTATUS
MmCreatePageFileSection(PSECTION_OBJECT *SectionObject,
                        ACCESS_MASK DesiredAccess,
                        POBJECT_ATTRIBUTES ObjectAttributes,
                        PLARGE_INTEGER UMaximumSize,
                        ULONG SectionPageProtection,
                        ULONG AllocationAttributes)
/*
 * Create a section which is backed by the pagefile
 */
{
   LARGE_INTEGER MaximumSize;
   PSECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;
   NTSTATUS Status;

   if (UMaximumSize == NULL)
   {
      return(STATUS_UNSUCCESSFUL);
   }
   MaximumSize = *UMaximumSize;

   /*
    * Create the section
    */
   Status = ObCreateObject(ExGetPreviousMode(),
                           MmSectionObjectType,
                           ObjectAttributes,
                           ExGetPreviousMode(),
                           NULL,
                           sizeof(SECTION_OBJECT),
                           0,
                           0,
                           (PVOID*)(PVOID)&Section);
   if (!NT_SUCCESS(Status))
   {
      return(Status);
   }

   /*
    * Initialize it
    */
   Section->SectionPageProtection = SectionPageProtection;
   Section->AllocationAttributes = AllocationAttributes;
   Section->Segment = NULL;
   InitializeListHead(&Section->ViewListHead);
   KeInitializeSpinLock(&Section->ViewListLock);
   Section->FileObject = NULL;
   Section->MaximumSize = MaximumSize;
   Segment = ExAllocatePoolWithTag(NonPagedPool, sizeof(MM_SECTION_SEGMENT),
                                   TAG_MM_SECTION_SEGMENT);
   if (Segment == NULL)
   {
      ObDereferenceObject(Section);
      return(STATUS_NO_MEMORY);
   }
   Section->Segment = Segment;
   Segment->ReferenceCount = 1;
   ExInitializeFastMutex(&Segment->Lock);
   Segment->FileOffset = 0;
   Segment->Protection = SectionPageProtection;
   Segment->RawLength = MaximumSize.u.LowPart;
   Segment->Length = PAGE_ROUND_UP(MaximumSize.u.LowPart);
   Segment->Flags = MM_PAGEFILE_SEGMENT;
   Segment->WriteCopy = FALSE;
   RtlZeroMemory(&Segment->PageDirectory, sizeof(SECTION_PAGE_DIRECTORY));
   Segment->VirtualAddress = 0;
   Segment->Characteristics = 0;
   *SectionObject = Section;
   return(STATUS_SUCCESS);
}


NTSTATUS
MmCreateDataFileSection(PSECTION_OBJECT *SectionObject,
                        ACCESS_MASK DesiredAccess,
                        POBJECT_ATTRIBUTES ObjectAttributes,
                        PLARGE_INTEGER UMaximumSize,
                        ULONG SectionPageProtection,
                        ULONG AllocationAttributes,
                        PFILE_OBJECT FileObject,
			BOOLEAN CacheManager)
/*
 * Create a section backed by a data file
 */
{
   PSECTION_OBJECT Section;
   NTSTATUS Status;
   LARGE_INTEGER MaximumSize;
   PMM_SECTION_SEGMENT Segment;
   PMM_SECTION_SEGMENT tmpSegment;
   ULONG FileAccess;
   ULONG Length;
   FILE_STANDARD_INFORMATION FileInfo;

   DPRINT("%x MmCreateDataFileSection called for %wZ, CacheManager = %d\n", FileObject, &FileObject->FileName, CacheManager);

   /*
    * Create the section
    */
   Status = ObCreateObject(ExGetPreviousMode(),
                           MmSectionObjectType,
                           ObjectAttributes,
                           ExGetPreviousMode(),
                           NULL,
                           sizeof(SECTION_OBJECT),
                           0,
                           0,
                           (PVOID*)(PVOID)&Section);
   if (!NT_SUCCESS(Status))
   {
      return(Status);
   }

   /*
    * Initialize it
    */
   Section->SectionPageProtection = SectionPageProtection;
   Section->AllocationAttributes = AllocationAttributes;
   Section->Segment = NULL;
   InitializeListHead(&Section->ViewListHead);
   KeInitializeSpinLock(&Section->ViewListLock);

   /*
    * Check file access required
    */
   if (SectionPageProtection & PAGE_READWRITE ||
         SectionPageProtection & PAGE_EXECUTE_READWRITE)
   {
      FileAccess = FILE_READ_DATA | FILE_WRITE_DATA;
   }
   else
   {
      FileAccess = FILE_READ_DATA;
   }

   /*
    * Reference the file object
    */
   Status = ObReferenceObjectByPointer(FileObject,
                                       FileAccess,
                                       IoFileObjectType,
                                       UserMode);
   if (!NT_SUCCESS(Status))
   {
      ObDereferenceObject(Section);
      return(Status);
   }

   if (CacheManager)
   {
      if (UMaximumSize == NULL || UMaximumSize->QuadPart == 0)
      {
         ObDereferenceObject(Section);
         ObDereferenceObject(FileObject);
	 return STATUS_FILE_INVALID;
      }
      DPRINT("%I64x\n", UMaximumSize->QuadPart);
      MaximumSize = *UMaximumSize;
   }
   else
   {
      /*
       * FIXME: This is propably not entirely correct. We can't look into
       * the standard FCB header because it might not be initialized yet
       * (as in case of the EXT2FS driver by Manoj Paul Joseph where the
       * standard file information is filled on first request).
       */
      Status = IoQueryFileInformation(FileObject,
                                      FileStandardInformation,
				      sizeof(FILE_STANDARD_INFORMATION),
				      &FileInfo,
				      &Length);
      if (!NT_SUCCESS(Status))
      {
         ObDereferenceObject(Section);
         ObDereferenceObject(FileObject);
         return Status;
      }
      if (FileInfo.Directory)
      {
         ObDereferenceObject(Section);
         ObDereferenceObject(FileObject);
	 return STATUS_FILE_INVALID;
      }
  
      /*
       * FIXME: Revise this once a locking order for file size changes is
       * decided
       */
      if (UMaximumSize != NULL)
      {
         MaximumSize = *UMaximumSize;
      }
      else
      {
         MaximumSize = FileInfo.EndOfFile;
         /* Mapping zero-sized files isn't allowed. */
         if (MaximumSize.QuadPart == 0)
         {
            ObDereferenceObject(Section);
            ObDereferenceObject(FileObject);
            return STATUS_FILE_INVALID;
         }
      }

      if (MaximumSize.QuadPart > FileInfo.EndOfFile.QuadPart)
      {
         Status = IoSetInformation(FileObject,
                                   FileAllocationInformation,
                                   sizeof(LARGE_INTEGER),
				   &MaximumSize);
         if (!NT_SUCCESS(Status))
         {
            ObDereferenceObject(Section);
            ObDereferenceObject(FileObject);
            return(STATUS_SECTION_NOT_EXTENDED);
         }
      }
   }
   
   ASSERT(FileObject->SectionObjectPointer);

   ExAcquireFastMutex(&DataSectionObjectLock);
   Segment = FileObject->SectionObjectPointer->DataSectionObject;
   if (Segment != NULL)
   {
      CHECKPOINT;
      Segment->ReferenceCount++;
   }
   ExReleaseFastMutex(&DataSectionObjectLock);

   if (Segment == NULL)
   {
      Segment = ExAllocatePoolWithTag(NonPagedPool, sizeof(MM_SECTION_SEGMENT),
                                      TAG_MM_SECTION_SEGMENT);
      if (Segment == NULL)
      {
         ObDereferenceObject(Section);
         ObDereferenceObject(FileObject);
         return(STATUS_NO_MEMORY);
      }
      Section->Segment = Segment;
      CHECKPOINT;
      Segment->ReferenceCount = 1;
      ExInitializeFastMutex(&Segment->Lock);
      Segment->FileOffset = 0;
      Segment->Protection = SectionPageProtection;
      Segment->Flags = MM_DATAFILE_SEGMENT;
      Segment->Characteristics = 0;
      Segment->WriteCopy = FALSE;
      Segment->FileObject = FileObject;
      Segment->BytesPerSector = 0;
      if (AllocationAttributes & SEC_RESERVE)
      {
         Segment->Length = Segment->RawLength = 0;
      }
      else
      {
         Segment->RawLength = MaximumSize.u.LowPart;
         Segment->Length = PAGE_ROUND_UP(Segment->RawLength);
      }
      Segment->VirtualAddress = 0;
      RtlZeroMemory(&Segment->PageDirectory, sizeof(SECTION_PAGE_DIRECTORY));

      /*
       * Lock the file
       */
      Status = MmspWaitForFileLock(FileObject);
      if (Status != STATUS_SUCCESS)
      {
         ExFreePool(Segment);
         ObDereferenceObject(Section);
         ObDereferenceObject(FileObject);
         return(Status);
      }

      /*
       * Set the lock before assigning the segment to the file object
       */
      ExAcquireFastMutex(&DataSectionObjectLock);
      ExAcquireFastMutex(&Segment->Lock);

      tmpSegment = InterlockedCompareExchangePointer(&FileObject->SectionObjectPointer->DataSectionObject,
	                                             Segment, NULL);
      if (tmpSegment != NULL)
      {
         CHECKPOINT;
         MmUnlockSectionSegment(Segment);
	 ExFreePool(Segment);
	 Segment = tmpSegment;
         MmLockSectionSegment(Segment);
	 CHECKPOINT;
	 Segment->ReferenceCount++;
	 Section->Segment = Segment;

         if (MaximumSize.u.LowPart > Segment->RawLength &&
            !(AllocationAttributes & SEC_RESERVE))
         {
            Segment->RawLength = MaximumSize.u.LowPart;
            Segment->Length = PAGE_ROUND_UP(Segment->RawLength);
         }
      }
      else
      {
	 InsertHeadList(&DataSectionObjectListHead, &Segment->ListEntry);
         ObReferenceObject(FileObject);
      }
   }
   else
   {
      /*
       * Lock the file
       */
      Status = MmspWaitForFileLock(FileObject);
      if (Status != STATUS_SUCCESS)
      {
         ExAcquireFastMutex(&DataSectionObjectLock);
         Segment->ReferenceCount--;
         ExReleaseFastMutex(&DataSectionObjectLock);
         ObDereferenceObject(Section);
         ObDereferenceObject(FileObject);
         return(Status);
      }

      Section->Segment = Segment;
      ExAcquireFastMutex(&DataSectionObjectLock);
      MmLockSectionSegment(Segment);

      if (MaximumSize.u.LowPart > Segment->RawLength &&
            !(AllocationAttributes & SEC_RESERVE))
      {
         Segment->RawLength = MaximumSize.u.LowPart;
         Segment->Length = PAGE_ROUND_UP(Segment->RawLength);
      }
      DPRINT("%x %x\n", Segment->RawLength, Segment->Length);
   }
   MmUnlockSectionSegment(Segment);
   Section->FileObject = FileObject;
   Section->MaximumSize = MaximumSize;
   ExReleaseFastMutex(&DataSectionObjectLock);
//   CcRosReferenceCache(FileObject);
   KeSetEvent((PVOID)&FileObject->Lock, IO_NO_INCREMENT, FALSE);
   *SectionObject = Section;
   return(STATUS_SUCCESS);
}

/*
 TODO: not that great (declaring loaders statically, having to declare all of
 them, having to keep them extern, etc.), will fix in the future
*/
extern NTSTATUS NTAPI PeFmtCreateSection
(
 IN CONST VOID * FileHeader,
 IN SIZE_T FileHeaderSize,
 IN PFILE_OBJECT FileObject,
 OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
 OUT PULONG Flags,
 IN PEXEFMT_CB_READ_FILE ReadFileCb,
 IN PEXEFMT_CB_ALLOCATE_SEGMENTS AllocateSegmentsCb
);

extern NTSTATUS NTAPI ElfFmtCreateSection
(
 IN CONST VOID * FileHeader,
 IN SIZE_T FileHeaderSize,
 IN PFILE_OBJECT FileObject,
 OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
 OUT PULONG Flags,
 IN PEXEFMT_CB_READ_FILE ReadFileCb,
 IN PEXEFMT_CB_ALLOCATE_SEGMENTS AllocateSegmentsCb
);

/* TODO: this is a standard DDK/PSDK macro */
#ifndef RTL_NUMBER_OF
#define RTL_NUMBER_OF(ARR_) (sizeof(ARR_) / sizeof((ARR_)[0]))
#endif

static PEXEFMT_LOADER ExeFmtpLoaders[] =
{
 PeFmtCreateSection,
 ElfFmtCreateSection
};

static
PMM_SECTION_SEGMENT
NTAPI
ExeFmtpAllocateSegments(IN ULONG NrSegments)
{
 SIZE_T SizeOfSegments;
 PMM_SECTION_SEGMENT Segments;

 /* TODO: check for integer overflow */
 SizeOfSegments = sizeof(MM_SECTION_SEGMENT) * NrSegments;

 Segments = ExAllocatePoolWithTag(NonPagedPool,
                                  SizeOfSegments,
                                  TAG_MM_SECTION_SEGMENT);

 if(Segments)
  RtlZeroMemory(Segments, SizeOfSegments);

 return Segments;
}

static
NTSTATUS
NTAPI
ExeFmtpReadFile(IN PFILE_OBJECT FileObject,
		ULONG SectorSize,
                IN PLARGE_INTEGER Offset,
                IN ULONG Length,
                OUT PVOID * Data,
                OUT PVOID * AllocBase,
                OUT PULONG ReadSize)
{
   NTSTATUS Status;
   LARGE_INTEGER FileOffset;
   ULONG AdjustOffset;
   ULONG OffsetAdjustment;
   ULONG BufferSize;
   ULONG UsedSize;
   PVOID Buffer;
   PPFN_TYPE Pages;
   ULONG i;

   ASSERT_IRQL_LESS(DISPATCH_LEVEL);

   if(Length == 0)
   {
      KEBUGCHECK(STATUS_INVALID_PARAMETER_4);
   }

   FileOffset = *Offset;

   /* Negative/special offset: it cannot be used in this context */
   if(FileOffset.u.HighPart < 0)
   {
      KEBUGCHECK(STATUS_INVALID_PARAMETER_5);
   }

   ASSERT(PAGE_SIZE <= MAXULONG);
   AdjustOffset = PAGE_ROUND_DOWN(FileOffset.u.LowPart);
   OffsetAdjustment = FileOffset.u.LowPart - AdjustOffset;
   FileOffset.u.LowPart = AdjustOffset;

   BufferSize = Length + OffsetAdjustment;
   BufferSize = PAGE_ROUND_UP(BufferSize);

   /*
    * It's ok to use paged pool, because this is a temporary buffer only used in
    * the loading of executables. The assumption is that MmCreateSection is
    * always called at low IRQLs and that these buffers don't survive a brief
    * initialization phase
    */

   /* 
    * FIXME:
    *   Changed to non paged pool, because we doesn't probe and lock the pages 
    */
   Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                  BufferSize,
                                  TAG('M', 'm', 'X', 'r'));

   UsedSize = 0;

   Pages = alloca(BufferSize / PAGE_SIZE * sizeof(PFN_TYPE));
   for(i = 0; i < BufferSize / PAGE_SIZE; i++)
   {
      Pages[i] = MmGetPfnForProcess(NULL, Buffer + i * PAGE_SIZE);
   }
   Status = MmspRawReadPages(FileObject,
                             SectorSize,
			     &FileOffset, 
			     BufferSize,
			     Pages);
   UsedSize = BufferSize;
   if(NT_SUCCESS(Status) && UsedSize < OffsetAdjustment)
   {
      Status = STATUS_IN_PAGE_ERROR;
      ASSERT(!NT_SUCCESS(Status));
   }

   if(NT_SUCCESS(Status))
   {
      *Data = (PVOID)((ULONG_PTR)Buffer + OffsetAdjustment);
      *AllocBase = Buffer;
      *ReadSize = UsedSize - OffsetAdjustment;
   }
   else
   {
      ExFreePool(Buffer);
   }

   return Status;
}

#ifdef NASSERT
# define MmspAssertSegmentsSorted(OBJ_) ((void)0)
# define MmspAssertSegmentsNoOverlap(OBJ_) ((void)0)
# define MmspAssertSegmentsPageAligned(OBJ_) ((void)0)
#else
static
VOID
NTAPI
MmspAssertSegmentsSorted(IN PMM_IMAGE_SECTION_OBJECT ImageSectionObject)
{
   ULONG i;

   for( i = 1; i < ImageSectionObject->NrSegments; ++ i )
   {
      ASSERT(ImageSectionObject->Segments[i].VirtualAddress >=
             ImageSectionObject->Segments[i - 1].VirtualAddress);
   }
}

static
VOID
NTAPI
MmspAssertSegmentsNoOverlap(IN PMM_IMAGE_SECTION_OBJECT ImageSectionObject)
{
   ULONG i;

   MmspAssertSegmentsSorted(ImageSectionObject);

   for( i = 0; i < ImageSectionObject->NrSegments; ++ i )
   {
      ASSERT(ImageSectionObject->Segments[i].Length > 0);

      if(i > 0)
      {
         ASSERT(ImageSectionObject->Segments[i].VirtualAddress >=
                (ImageSectionObject->Segments[i - 1].VirtualAddress +
                 ImageSectionObject->Segments[i - 1].Length));
      }
   }
}

static
VOID
NTAPI
MmspAssertSegmentsPageAligned(IN PMM_IMAGE_SECTION_OBJECT ImageSectionObject)
{
   ULONG i;

   for( i = 0; i < ImageSectionObject->NrSegments; ++ i )
   {
      ASSERT((ImageSectionObject->Segments[i].VirtualAddress % PAGE_SIZE) == 0);
      ASSERT((ImageSectionObject->Segments[i].Length % PAGE_SIZE) == 0);
   }
}
#endif

static
int
__cdecl
MmspCompareSegments(const void * x,
                    const void * y)
{
   PMM_SECTION_SEGMENT Segment1 = (PMM_SECTION_SEGMENT)x;
   PMM_SECTION_SEGMENT Segment2 = (PMM_SECTION_SEGMENT)y;

   return
      (Segment1->VirtualAddress - Segment2->VirtualAddress) >>
      ((sizeof(ULONG_PTR) - sizeof(int)) * 8);
}

/*
 * Ensures an image section's segments are sorted in memory
 */
static
VOID
NTAPI
MmspSortSegments(IN OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
                 IN ULONG Flags)
{
   if (Flags & EXEFMT_LOAD_ASSUME_SEGMENTS_SORTED)
   {
      MmspAssertSegmentsSorted(ImageSectionObject);
   }
   else
   {
      qsort(ImageSectionObject->Segments,
            ImageSectionObject->NrSegments,
            sizeof(ImageSectionObject->Segments[0]),
            MmspCompareSegments);
   }
}


/*
 * Ensures an image section's segments don't overlap in memory and don't have
 * gaps and don't have a null size. We let them map to overlapping file regions,
 * though - that's not necessarily an error
 */
static
BOOLEAN
NTAPI
MmspCheckSegmentBounds
(
 IN OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
 IN ULONG Flags
)
{
   ULONG i;

   if (Flags & EXEFMT_LOAD_ASSUME_SEGMENTS_NO_OVERLAP)
   {
      MmspAssertSegmentsNoOverlap(ImageSectionObject);
      return TRUE;
   }

   ASSERT(ImageSectionObject->NrSegments >= 1);

   for ( i = 0; i < ImageSectionObject->NrSegments; ++ i )
   {
      if(ImageSectionObject->Segments[i].Length == 0)
      {
         return FALSE;
      }

      if(i > 0)
      {
         /*
          * TODO: relax the limitation on gaps. For example, gaps smaller than a
          * page could be OK (Windows seems to be OK with them), and larger gaps
          * could lead to image sections spanning several discontiguous regions
          * (NtMapViewOfSection could then refuse to map them, and they could
          * e.g. only be allowed as parameters to NtCreateProcess, like on UNIX)
          */
         if ((ImageSectionObject->Segments[i - 1].VirtualAddress +
              ImageSectionObject->Segments[i - 1].Length) !=
              ImageSectionObject->Segments[i].VirtualAddress)
         {
            return FALSE;
         }
      }
   }

   return TRUE;
}

/*
 * Merges and pads an image section's segments until they all are page-aligned
 * and have a size that is a multiple of the page size
 */
static
BOOLEAN
NTAPI
MmspPageAlignSegments
(
 IN OUT PMM_IMAGE_SECTION_OBJECT ImageSectionObject,
 IN ULONG Flags
)
{
   ULONG i;
   ULONG LastSegment;
   BOOLEAN Initialized;

   if (Flags & EXEFMT_LOAD_ASSUME_SEGMENTS_PAGE_ALIGNED)
   {
      MmspAssertSegmentsPageAligned(ImageSectionObject);
      return TRUE;
   }

   Initialized = FALSE;
   LastSegment = 0;

   for ( i = 0; i < ImageSectionObject->NrSegments; ++ i )
   {
      PMM_SECTION_SEGMENT EffectiveSegment = &ImageSectionObject->Segments[LastSegment];

      /*
       * The first segment requires special handling
       */
      if (i == 0)
      {
         ULONG_PTR VirtualAddress;
         ULONG_PTR VirtualOffset;

         VirtualAddress = EffectiveSegment->VirtualAddress;

         /* Round down the virtual address to the nearest page */
         EffectiveSegment->VirtualAddress = PAGE_ROUND_DOWN(VirtualAddress);

         /* Round up the virtual size to the nearest page */
         EffectiveSegment->Length = PAGE_ROUND_UP(VirtualAddress + EffectiveSegment->Length) -
                                    EffectiveSegment->VirtualAddress;

         /* Adjust the raw address and size */
         VirtualOffset = VirtualAddress - EffectiveSegment->VirtualAddress;

         if (EffectiveSegment->FileOffset < VirtualOffset)
         {
            return FALSE;
         }

         /*
          * Garbage in, garbage out: unaligned base addresses make the file
          * offset point in curious and odd places, but that's what we were
          * asked for
          */
         EffectiveSegment->FileOffset -= VirtualOffset;
         EffectiveSegment->RawLength += VirtualOffset;
      }
      else
      {
         PMM_SECTION_SEGMENT Segment = &ImageSectionObject->Segments[i];
         ULONG_PTR EndOfEffectiveSegment;

         EndOfEffectiveSegment = EffectiveSegment->VirtualAddress + EffectiveSegment->Length;
         ASSERT((EndOfEffectiveSegment % PAGE_SIZE) == 0);

         /*
          * The current segment begins exactly where the current effective
          * segment ended, therefore beginning a new effective segment
          */
         if (EndOfEffectiveSegment == Segment->VirtualAddress)
         {
            LastSegment ++;
            ASSERT(LastSegment <= i);
            ASSERT(LastSegment < ImageSectionObject->NrSegments);

            EffectiveSegment = &ImageSectionObject->Segments[LastSegment];

            /*
             * Copy the current segment. If necessary, the effective segment
             * will be expanded later
             */
            *EffectiveSegment = *Segment;

            /*
             * Page-align the virtual size. We know for sure the virtual address
             * already is
             */
            ASSERT((EffectiveSegment->VirtualAddress % PAGE_SIZE) == 0);
            EffectiveSegment->Length = PAGE_ROUND_UP(EffectiveSegment->Length);
         }
         /*
          * The current segment is still part of the current effective segment:
          * extend the effective segment to reflect this
          */
         else if (EndOfEffectiveSegment > Segment->VirtualAddress)
         {
            static const ULONG FlagsToProtection[16] =
            {
               PAGE_NOACCESS,
               PAGE_READONLY,
               PAGE_READWRITE,
               PAGE_READWRITE,
               PAGE_EXECUTE_READ,
               PAGE_EXECUTE_READ,
               PAGE_EXECUTE_READWRITE,
               PAGE_EXECUTE_READWRITE,
               PAGE_WRITECOPY,
               PAGE_WRITECOPY,
               PAGE_WRITECOPY,
               PAGE_WRITECOPY,
               PAGE_EXECUTE_WRITECOPY,
               PAGE_EXECUTE_WRITECOPY,
               PAGE_EXECUTE_WRITECOPY,
               PAGE_EXECUTE_WRITECOPY
            };

            unsigned ProtectionFlags;

            /*
             * Extend the file size
             */

            /* Unaligned segments must be contiguous within the file */
            if (Segment->FileOffset != (EffectiveSegment->FileOffset +
                                        EffectiveSegment->RawLength))
            {
               return FALSE;
            }

            EffectiveSegment->RawLength += Segment->RawLength;

            /*
             * Extend the virtual size
             */
            ASSERT(PAGE_ROUND_UP(Segment->VirtualAddress + Segment->Length) > EndOfEffectiveSegment);

            EffectiveSegment->Length = PAGE_ROUND_UP(Segment->VirtualAddress + Segment->Length) -
                                       EffectiveSegment->VirtualAddress;

            /*
             * Merge the protection
             */
            EffectiveSegment->Protection |= Segment->Protection;

            /* Clean up redundance */
            ProtectionFlags = 0;

            if(EffectiveSegment->Protection & PAGE_IS_READABLE)
               ProtectionFlags |= 1 << 0;

            if(EffectiveSegment->Protection & PAGE_IS_WRITABLE)
               ProtectionFlags |= 1 << 1;

            if(EffectiveSegment->Protection & PAGE_IS_EXECUTABLE)
               ProtectionFlags |= 1 << 2;

            if(EffectiveSegment->Protection & PAGE_IS_WRITECOPY)
               ProtectionFlags |= 1 << 3;

            ASSERT(ProtectionFlags < 16);
            EffectiveSegment->Protection = FlagsToProtection[ProtectionFlags];

            /* If a segment was required to be shared and cannot, fail */
            if(!(Segment->Protection & PAGE_IS_WRITECOPY) &&
               EffectiveSegment->Protection & PAGE_IS_WRITECOPY)
            {
               return FALSE;
            }
         }
         /*
          * We assume no holes between segments at this point
          */
         else
         {
            ASSERT(FALSE);
         }
      }
   }

   return TRUE;
}

NTSTATUS
ExeFmtpCreateImageSection(PFILE_OBJECT FileObject,
                          PMM_IMAGE_SECTION_OBJECT ImageSectionObject)
{
   FILE_FS_SIZE_INFORMATION FileFsSize;
   LARGE_INTEGER Offset;
   PVOID FileHeader;
   PVOID FileHeaderBuffer;
   ULONG FileHeaderSize;
   ULONG Flags;
   ULONG OldNrSegments;
   NTSTATUS Status;
   ULONG i;

   Status = IoQueryVolumeInformation(FileObject,
                                     FileFsSizeInformation,
				     sizeof(FILE_FS_SIZE_INFORMATION),
                                     &FileFsSize,
				     NULL);
   if (!NT_SUCCESS(Status))
   {
      return Status;
   }

   /*
    * Read the beginning of the file (2 pages). Should be enough to contain
    * all (or most) of the headers
    */
   Offset.QuadPart = 0;

   /* FIXME: use FileObject instead of FileHandle */
   Status = ExeFmtpReadFile (FileObject,
                             FileFsSize.BytesPerSector,
                             &Offset,
                             PAGE_SIZE * 2,
                             &FileHeader,
                             &FileHeaderBuffer,
                             &FileHeaderSize);

   if (!NT_SUCCESS(Status))
      return Status;

   if (FileHeaderSize == 0)
   {
      ExFreePool(FileHeaderBuffer);
      return STATUS_UNSUCCESSFUL;
   }

   /*
    * Look for a loader that can handle this executable
    */
   for (i = 0; i < RTL_NUMBER_OF(ExeFmtpLoaders); ++ i)
   {
      RtlZeroMemory(ImageSectionObject, sizeof(*ImageSectionObject));
      ImageSectionObject->BytesPerSector = FileFsSize.BytesPerSector;
      Flags = 0;

      /* FIXME: use FileObject instead of FileHandle */
      Status = ExeFmtpLoaders[i](FileHeader,
                                 FileHeaderSize,
                                 FileObject,
                                 ImageSectionObject,
                                 &Flags,
                                 ExeFmtpReadFile,
                                 ExeFmtpAllocateSegments);

      if (!NT_SUCCESS(Status))
      {
         if (ImageSectionObject->Segments)
         {
            ExFreePool(ImageSectionObject->Segments);
            ImageSectionObject->Segments = NULL;
         }
      }

      if (Status != STATUS_ROS_EXEFMT_UNKNOWN_FORMAT)
         break;
   }

   ExFreePool(FileHeaderBuffer);

   /*
    * No loader handled the format
    */
   if (Status == STATUS_ROS_EXEFMT_UNKNOWN_FORMAT)
   {
      Status = STATUS_INVALID_IMAGE_FORMAT;
      ASSERT(!NT_SUCCESS(Status));
   }

   if (!NT_SUCCESS(Status))
      return Status;

   ASSERT(ImageSectionObject->Segments != NULL);

   /*
    * Some defaults
    */
   /* FIXME? are these values platform-dependent? */
   if(ImageSectionObject->StackReserve == 0)
      ImageSectionObject->StackReserve = 0x40000;

   if(ImageSectionObject->StackCommit == 0)
      ImageSectionObject->StackCommit = 0x1000;

   if(ImageSectionObject->ImageBase == 0)
   {
      if(ImageSectionObject->ImageCharacteristics & IMAGE_FILE_DLL)
         ImageSectionObject->ImageBase = 0x10000000;
      else
         ImageSectionObject->ImageBase = 0x00400000;
   }

   /*
    * And now the fun part: fixing the segments
    */

   /* Sort them by virtual address */
   MmspSortSegments(ImageSectionObject, Flags);

   /* Ensure they don't overlap in memory */
   if (!MmspCheckSegmentBounds(ImageSectionObject, Flags))
      return STATUS_INVALID_IMAGE_FORMAT;

   /* Ensure they are aligned */
   OldNrSegments = ImageSectionObject->NrSegments;

   if (!MmspPageAlignSegments(ImageSectionObject, Flags))
      return STATUS_INVALID_IMAGE_FORMAT;

   /* Trim them if the alignment phase merged some of them */
   if (ImageSectionObject->NrSegments < OldNrSegments)
   {
      PMM_SECTION_SEGMENT Segments;
      SIZE_T SizeOfSegments;

      SizeOfSegments = sizeof(MM_SECTION_SEGMENT) * ImageSectionObject->NrSegments;

      Segments = ExAllocatePoolWithTag(NonPagedPool,
                                       SizeOfSegments,
                                       TAG_MM_SECTION_SEGMENT);

      if (Segments == NULL)
         return STATUS_INSUFFICIENT_RESOURCES;

      RtlCopyMemory(Segments, ImageSectionObject->Segments, SizeOfSegments);
      ExFreePool(ImageSectionObject->Segments);
      ImageSectionObject->Segments = Segments;
   }

   /* And finish their initialization */
   for ( i = 0; i < ImageSectionObject->NrSegments; ++ i )
   {
      ExInitializeFastMutex(&ImageSectionObject->Segments[i].Lock);
      ImageSectionObject->Segments[i].ReferenceCount = 1;

      RtlZeroMemory(&ImageSectionObject->Segments[i].PageDirectory,
                    sizeof(ImageSectionObject->Segments[i].PageDirectory));
      ImageSectionObject->Segments[i].BytesPerSector = FileFsSize.BytesPerSector;
   }

   ASSERT(NT_SUCCESS(Status));
   return Status;
}

NTSTATUS
MmCreateImageSection(PSECTION_OBJECT *SectionObject,
                     ACCESS_MASK DesiredAccess,
                     POBJECT_ATTRIBUTES ObjectAttributes,
                     PLARGE_INTEGER UMaximumSize,
                     ULONG SectionPageProtection,
                     ULONG AllocationAttributes,
                     PFILE_OBJECT FileObject)
{
   PSECTION_OBJECT Section;
   NTSTATUS Status;
   PMM_SECTION_SEGMENT SectionSegments;
   PMM_IMAGE_SECTION_OBJECT ImageSectionObject;
   PMM_IMAGE_SECTION_OBJECT tmpImageSectionObject;
   ULONG i;
   ULONG FileAccess = 0;

   /*
    * Specifying a maximum size is meaningless for an image section
    */
   if (UMaximumSize != NULL)
   {
      return(STATUS_INVALID_PARAMETER_4);
   }

   /*
    * Check file access required
    */
   if (SectionPageProtection & PAGE_READWRITE ||
       SectionPageProtection & PAGE_EXECUTE_READWRITE)
   {
      FileAccess = FILE_READ_DATA | FILE_WRITE_DATA;
   }
   else
   {
      FileAccess = FILE_READ_DATA;
   }

   /*
    * Reference the file object
    */
   Status = ObReferenceObjectByPointer(FileObject,
                                       FileAccess,
                                       IoFileObjectType,
                                       UserMode);
   if (!NT_SUCCESS(Status))
   {
      return Status;
   }

   /*
    * Create the section
    */
   Status = ObCreateObject (ExGetPreviousMode(),
                            MmSectionObjectType,
                            ObjectAttributes,
                            ExGetPreviousMode(),
                            NULL,
                            sizeof(SECTION_OBJECT),
                            0,
                            0,
                            (PVOID*)(PVOID)&Section);
   if (!NT_SUCCESS(Status))
   {
      ObDereferenceObject(FileObject);
      return(Status);
   }

   /*
    * Initialize it
    */
   Section->SectionPageProtection = SectionPageProtection;
   Section->AllocationAttributes = AllocationAttributes;
   DPRINT("%x\n", Section->AllocationAttributes);
   InitializeListHead(&Section->ViewListHead);
   KeInitializeSpinLock(&Section->ViewListLock);

   ASSERT (FileObject->SectionObjectPointer);

   ExAcquireFastMutex(&ImageSectionObjectLock);
   ImageSectionObject = FileObject->SectionObjectPointer->ImageSectionObject;
   if (ImageSectionObject != NULL)
   {
      ImageSectionObject->RefCount++;
   }
   ExReleaseFastMutex(&ImageSectionObjectLock);

   if (ImageSectionObject == NULL)
   {
      NTSTATUS StatusExeFmt;

      if (FileObject->SectionObjectPointer->SharedCacheMap)
      {
         IO_STATUS_BLOCK Iosb;

         CHECKPOINT;
         CcFlushCache(FileObject->SectionObjectPointer, NULL, 0, &Iosb);
	 if (!NT_SUCCESS(Iosb.Status))
	 {
            DPRINT1("%x\n", Iosb.Status);
	 }
      }

      ImageSectionObject = ExAllocatePoolWithTag(NonPagedPool, sizeof(MM_IMAGE_SECTION_OBJECT), TAG_MM_SECTION_SEGMENT);
      if (ImageSectionObject == NULL)
      {
         ObDereferenceObject(FileObject);
         ObDereferenceObject(Section);
         return(STATUS_NO_MEMORY);
      }

      StatusExeFmt = ExeFmtpCreateImageSection(FileObject, ImageSectionObject);

      if (!NT_SUCCESS(StatusExeFmt))
      {
         if(ImageSectionObject->Segments != NULL)
	 {
            ExFreePool(ImageSectionObject->Segments);
	 }

         ExFreePool(ImageSectionObject);
         ObDereferenceObject(Section);
         ObDereferenceObject(FileObject);
         return(StatusExeFmt);
      }

      Section->ImageSection = ImageSectionObject;
      ASSERT(ImageSectionObject->Segments);

      /*
       * Lock the file
       */
      Status = MmspWaitForFileLock(FileObject);
      if (!NT_SUCCESS(Status))
      {
         ExFreePool(ImageSectionObject->Segments);
         ExFreePool(ImageSectionObject);
         ObDereferenceObject(Section);
         ObDereferenceObject(FileObject);
         return(Status);
      }
   
      ExAcquireFastMutex(&ImageSectionObjectLock);
      tmpImageSectionObject = InterlockedCompareExchangePointer(&FileObject->SectionObjectPointer->ImageSectionObject,
                                                                ImageSectionObject, NULL);
      if (tmpImageSectionObject != NULL)
      {
         tmpImageSectionObject->RefCount++;
      }
      else
      {
         ImageSectionObject->RefCount++;
	 InsertHeadList(&ImageSectionObjectListHead, &ImageSectionObject->ListEntry);
	 ImageSectionObjectCount++;
	 ImageSectionObject->FileObject = FileObject;
         ObReferenceObject(FileObject);

      }
      ExReleaseFastMutex(&ImageSectionObjectLock);

      if (NULL != tmpImageSectionObject)
      {
         /*
          * An other thread has initialized the some image in the background
          */
         ExFreePool(ImageSectionObject->Segments);
         ExFreePool(ImageSectionObject);
         ImageSectionObject = tmpImageSectionObject;
         Section->ImageSection = ImageSectionObject;
         SectionSegments = ImageSectionObject->Segments;

         for (i = 0; i < ImageSectionObject->NrSegments; i++)
         {
            InterlockedIncrementUL(&SectionSegments[i].ReferenceCount);
         }
      }
      else
      {
      }
   }
   else
   {
      /*
       * Lock the file
       */
      Status = MmspWaitForFileLock(FileObject);
      if (Status != STATUS_SUCCESS)
      {
         ObDereferenceObject(Section);
         ObDereferenceObject(FileObject);
         ExAcquireFastMutex(&ImageSectionObjectLock);
         ImageSectionObject->RefCount--;
         ExReleaseFastMutex(&ImageSectionObjectLock);
         return(Status);
      }

      Section->ImageSection = ImageSectionObject;
      SectionSegments = ImageSectionObject->Segments;

      /*
       * Otherwise just reference all the section segments
       */
      for (i = 0; i < ImageSectionObject->NrSegments; i++)
      {
         InterlockedIncrementUL(&SectionSegments[i].ReferenceCount);
      }
   }
   Section->FileObject = FileObject;
//   CcRosReferenceCache(FileObject);
   KeSetEvent((PVOID)&FileObject->Lock, IO_NO_INCREMENT, FALSE);
   *SectionObject = Section;
   DPRINT("%x\n", Section->AllocationAttributes);
   return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
NtCreateSection (OUT PHANDLE SectionHandle,
                 IN ACCESS_MASK DesiredAccess,
                 IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
                 IN PLARGE_INTEGER MaximumSize OPTIONAL,
                 IN ULONG SectionPageProtection OPTIONAL,
                 IN ULONG AllocationAttributes,
                 IN HANDLE FileHandle OPTIONAL)
{
   LARGE_INTEGER SafeMaximumSize;
   PSECTION_OBJECT SectionObject;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;

   PreviousMode = ExGetPreviousMode();

   if(MaximumSize != NULL && PreviousMode != KernelMode)
   {
     _SEH_TRY
     {
       ProbeForRead(MaximumSize,
                    sizeof(LARGE_INTEGER),
                    sizeof(ULONG));
       /* make a copy on the stack */
       SafeMaximumSize = *MaximumSize;
       MaximumSize = &SafeMaximumSize;
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;

     if(!NT_SUCCESS(Status))
     {
       return Status;
     }
   }

   /*
    * Check the protection
    */
   if ((SectionPageProtection & PAGE_FLAGS_VALID_FROM_USER_MODE) !=
         SectionPageProtection)
   {
      return(STATUS_INVALID_PAGE_PROTECTION);
   }

   Status = MmCreateSection(&SectionObject,
                            DesiredAccess,
                            ObjectAttributes,
                            MaximumSize,
                            SectionPageProtection,
                            AllocationAttributes,
                            FileHandle,
                            NULL);

   if (NT_SUCCESS(Status))
   {
      Status = ObInsertObject ((PVOID)SectionObject,
                               NULL,
                               DesiredAccess,
                               0,
                               NULL,
                               SectionHandle);
      ObDereferenceObject(SectionObject);
   }

   return Status;
}


/**********************************************************************
 * NAME
 *  NtOpenSection
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *  SectionHandle
 *
 *  DesiredAccess
 *
 *  ObjectAttributes
 *
 * RETURN VALUE
 *
 * REVISIONS
 */
NTSTATUS STDCALL
NtOpenSection(PHANDLE   SectionHandle,
              ACCESS_MASK  DesiredAccess,
              POBJECT_ATTRIBUTES ObjectAttributes)
{
   HANDLE hSection;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;

   PreviousMode = ExGetPreviousMode();

   if(PreviousMode != KernelMode)
   {
     _SEH_TRY
     {
       ProbeForWrite(SectionHandle,
                     sizeof(HANDLE),
                     sizeof(ULONG));
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;

     if(!NT_SUCCESS(Status))
     {
       return Status;
     }
   }

   Status = ObOpenObjectByName(ObjectAttributes,
                               MmSectionObjectType,
                               NULL,
                               PreviousMode,
                               DesiredAccess,
                               NULL,
                               &hSection);

   if(NT_SUCCESS(Status))
   {
     _SEH_TRY
     {
       *SectionHandle = hSection;
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;
   }

   return(Status);
}

NTSTATUS STATIC
MmMapViewOfSegment(PEPROCESS Process,
                   PMADDRESS_SPACE AddressSpace,
                   PSECTION_OBJECT Section,
                   PMM_SECTION_SEGMENT Segment,
                   PVOID* BaseAddress,
                   ULONG ViewSize,
                   ULONG Protect,
                   ULONG ViewOffset,
                   BOOL TopDown)
{
   PMEMORY_AREA MArea;
   NTSTATUS Status;
   KIRQL oldIrql;
   PHYSICAL_ADDRESS BoundaryAddressMultiple;

   BoundaryAddressMultiple.QuadPart = 0;

   Status = MmCreateMemoryArea(Process,
                               AddressSpace,
                               MEMORY_AREA_SECTION_VIEW,
                               BaseAddress,
                               ViewSize,
                               Protect,
                               &MArea,
                               FALSE,
                               TopDown,
                               BoundaryAddressMultiple);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Mapping between 0x%.8X and 0x%.8X failed (%X).\n",
              (*BaseAddress), (char*)(*BaseAddress) + ViewSize, Status);
      return(Status);
   }

   KeAcquireSpinLock(&Section->ViewListLock, &oldIrql);
   InsertTailList(&Section->ViewListHead,
                  &MArea->Data.SectionData.ViewListEntry);
   KeReleaseSpinLock(&Section->ViewListLock, oldIrql);

   ObReferenceObjectByPointer((PVOID)Section,
                              SECTION_MAP_READ,
                              NULL,
                              ExGetPreviousMode());
   MArea->Data.SectionData.Segment = Segment;
   MArea->Data.SectionData.Section = Section;
   MArea->Data.SectionData.ViewOffset = ViewOffset;
//   MArea->Data.SectionData.WriteCopyView = FALSE;
   MmInitialiseRegion(&MArea->Data.SectionData.RegionListHead,
                      ViewSize, 0, Protect);

   return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME       EXPORTED
 * NtMapViewOfSection
 *
 * DESCRIPTION
 * Maps a view of a section into the virtual address space of a
 * process.
 *
 * ARGUMENTS
 * SectionHandle
 *  Handle of the section.
 *
 * ProcessHandle
 *  Handle of the process.
 *
 * BaseAddress
 *  Desired base address (or NULL) on entry;
 *  Actual base address of the view on exit.
 *
 * ZeroBits
 *  Number of high order address bits that must be zero.
 *
 * CommitSize
 *  Size in bytes of the initially committed section of
 *  the view.
 *
 * SectionOffset
 *  Offset in bytes from the beginning of the section
 *  to the beginning of the view.
 *
 * ViewSize
 *  Desired length of map (or zero to map all) on entry
 *  Actual length mapped on exit.
 *
 * InheritDisposition
 *  Specified how the view is to be shared with
 *  child processes.
 *
 * AllocateType
 *  Type of allocation for the pages.
 *
 * Protect
 *  Protection for the committed region of the view.
 *
 * RETURN VALUE
 *  Status.
 *
 * @implemented
 */
NTSTATUS STDCALL
NtMapViewOfSection(IN HANDLE SectionHandle,
                   IN HANDLE ProcessHandle,
                   IN OUT PVOID* BaseAddress  OPTIONAL,
                   IN ULONG ZeroBits  OPTIONAL,
                   IN ULONG CommitSize,
                   IN OUT PLARGE_INTEGER SectionOffset  OPTIONAL,
                   IN OUT PULONG ViewSize,
                   IN SECTION_INHERIT InheritDisposition,
                   IN ULONG AllocationType  OPTIONAL,
                   IN ULONG Protect)
{
   PVOID SafeBaseAddress;
   LARGE_INTEGER SafeSectionOffset;
   ULONG SafeViewSize;
   PSECTION_OBJECT Section;
   PEPROCESS Process;
   KPROCESSOR_MODE PreviousMode;
   PMADDRESS_SPACE AddressSpace;
   NTSTATUS Status = STATUS_SUCCESS;

   PreviousMode = ExGetPreviousMode();

   if(PreviousMode != KernelMode)
   {
     SafeBaseAddress = NULL;
     SafeSectionOffset.QuadPart = 0;
     SafeViewSize = 0;

     _SEH_TRY
     {
       if(BaseAddress != NULL)
       {
         ProbeForWrite(BaseAddress,
                       sizeof(PVOID),
                       sizeof(ULONG));
         SafeBaseAddress = *BaseAddress;
       }
       if(SectionOffset != NULL)
       {
         ProbeForWrite(SectionOffset,
                       sizeof(LARGE_INTEGER),
                       sizeof(ULONG));
         SafeSectionOffset = *SectionOffset;
       }
       ProbeForWrite(ViewSize,
                     sizeof(ULONG),
                     sizeof(ULONG));
       SafeViewSize = *ViewSize;
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;

     if(!NT_SUCCESS(Status))
     {
       return Status;
     }
   }
   else
   {
     SafeBaseAddress = (BaseAddress != NULL ? *BaseAddress : NULL);
     SafeSectionOffset.QuadPart = (SectionOffset != NULL ? SectionOffset->QuadPart : 0);
     SafeViewSize = (ViewSize != NULL ? *ViewSize : 0);
   }

   Status = ObReferenceObjectByHandle(ProcessHandle,
                                      PROCESS_VM_OPERATION,
                                      PsProcessType,
                                      PreviousMode,
                                      (PVOID*)(PVOID)&Process,
                                      NULL);
   if (!NT_SUCCESS(Status))
   {
      return(Status);
   }

   AddressSpace = &Process->AddressSpace;

   Status = ObReferenceObjectByHandle(SectionHandle,
                                      SECTION_MAP_READ,
                                      MmSectionObjectType,
                                      PreviousMode,
                                      (PVOID*)(PVOID)&Section,
                                      NULL);
   if (!(NT_SUCCESS(Status)))
   {
      DPRINT("ObReference failed rc=%x\n",Status);
      ObDereferenceObject(Process);
      return(Status);
   }

   Status = MmMapViewOfSection(Section,
                               Process,
                               (BaseAddress != NULL ? &SafeBaseAddress : NULL),
                               ZeroBits,
                               CommitSize,
                               (SectionOffset != NULL ? &SafeSectionOffset : NULL),
                               (ViewSize != NULL ? &SafeViewSize : NULL),
                               InheritDisposition,
                               AllocationType,
                               Protect);

   ObDereferenceObject(Section);
   ObDereferenceObject(Process);

   if(NT_SUCCESS(Status))
   {
     /* copy parameters back to the caller */
     _SEH_TRY
     {
       if(BaseAddress != NULL)
       {
         *BaseAddress = SafeBaseAddress;
       }
       if(SectionOffset != NULL)
       {
         *SectionOffset = SafeSectionOffset;
       }
       if(ViewSize != NULL)
       {
         *ViewSize = SafeViewSize;
       }
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;
   }

   return(Status);
}

VOID STATIC
MmFreeSectionPage(PVOID Context, MEMORY_AREA* MemoryArea, PVOID Address,
                  PFN_TYPE Page, SWAPENTRY SwapEntry, BOOLEAN Dirty)
{
   PMEMORY_AREA MArea;
   ULONG Entry;
//   PFILE_OBJECT FileObject;
//   PBCB Bcb;
   ULONG Offset;
   SWAPENTRY SavedSwapEntry;
   PMM_PAGEOP PageOp;
   NTSTATUS Status;
   PSECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;

   MArea = (PMEMORY_AREA)Context;

   Address = (PVOID)PAGE_ROUND_DOWN(Address);

   Offset = ((ULONG_PTR)Address - (ULONG_PTR)MArea->StartingAddress) +
            MemoryArea->Data.SectionData.ViewOffset;

   Section = MArea->Data.SectionData.Section;
   Segment = MArea->Data.SectionData.Segment;

   PageOp = MmCheckForPageOp(MArea, NULL, NULL, Segment, Offset);

   while (PageOp)
   {
      MmUnlockSectionSegment(Segment);
      MmUnlockAddressSpace(&MArea->Process->AddressSpace);

      Status = MmspWaitForPageOpCompletionEvent(PageOp);
      if (Status != STATUS_SUCCESS)
      {
         DPRINT1("Failed to wait for page op, status = %x\n", Status);
         KEBUGCHECK(0);
      }

      MmLockAddressSpace(&MArea->Process->AddressSpace);
      MmLockSectionSegment(Segment);
      MmspCompleteAndReleasePageOp(PageOp);
      PageOp = MmCheckForPageOp(MArea, NULL, NULL, Segment, Offset);
   }

   Entry = MmGetPageEntrySectionSegment(Segment, Offset);
   if (SwapEntry != 0)
   {
      /*
       * Sanity check
       */
      if (Segment->Flags & MM_PAGEFILE_SEGMENT)
      {
         DPRINT1("Found a swap entry for a page in a pagefile section.\n");
         KEBUGCHECK(0);
      }
      MmFreeSwapPage(SwapEntry);
   }
   else if (Page != 0)
   {
      if (IS_SWAP_FROM_SSE(Entry) ||
          Page != PFN_FROM_SSE(Entry))
      {
         /*
          * Sanity check
          */
         if (Segment->Flags & MM_PAGEFILE_SEGMENT)
         {
            DPRINT1("Found a private page in a pagefile section.\n");
            KEBUGCHECK(0);
         }
         /*
          * Just dereference private pages
          */
         SavedSwapEntry = MmGetSavedSwapEntryPage(Page);
         if (SavedSwapEntry != 0)
         {
            MmFreeSwapPage(SavedSwapEntry);
            MmSetSavedSwapEntryPage(Page, 0);
         }
         MmDeleteRmap(Page, MArea->Process, Address);
         MmReleasePageMemoryConsumer(MC_USER, Page);
      }
      else
      {
         MmDeleteRmap(Page, MArea->Process, Address);
	 DPRINT("%x\n", Address);
         MmUnsharePageEntrySectionSegment(Section, Segment, Offset, Dirty, FALSE);
      }
   }
}

NTSTATUS
MmUnmapViewOfSegment(PMADDRESS_SPACE AddressSpace,
                     PVOID BaseAddress)
{
   NTSTATUS Status;
   PMEMORY_AREA MemoryArea;
   PSECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;
   KIRQL oldIrql;
   PLIST_ENTRY CurrentEntry;
   PMM_REGION CurrentRegion;
   PLIST_ENTRY RegionListHead;

   MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace,
                                            BaseAddress);
   if (MemoryArea == NULL)
   {
      return(STATUS_UNSUCCESSFUL);
   }

   MemoryArea->DeleteInProgress = TRUE;
   Section = MemoryArea->Data.SectionData.Section;
   Segment = MemoryArea->Data.SectionData.Segment;

   MmLockSectionSegment(Segment);
   KeAcquireSpinLock(&Section->ViewListLock, &oldIrql);
   RemoveEntryList(&MemoryArea->Data.SectionData.ViewListEntry);
   KeReleaseSpinLock(&Section->ViewListLock, oldIrql);

   RegionListHead = &MemoryArea->Data.SectionData.RegionListHead;
   while (!IsListEmpty(RegionListHead))
   {
      CurrentEntry = RemoveHeadList(RegionListHead);
      CurrentRegion = CONTAINING_RECORD(CurrentEntry, MM_REGION, RegionListEntry);
      ExFreePool(CurrentRegion);
   }

   if (Section->AllocationAttributes & SEC_PHYSICALMEMORY)
   {
      Status = MmFreeMemoryArea(AddressSpace,
                                MemoryArea,
                                NULL,
                                NULL);
   }
   else
   {
      Status = MmFreeMemoryArea(AddressSpace,
                                MemoryArea,
                                MmFreeSectionPage,
                                MemoryArea);
   }
   MmUnlockSectionSegment(Segment);
   ObDereferenceObject(Section);
   return(STATUS_SUCCESS);
}

/*
 * @implemented
 */
NTSTATUS STDCALL
MmUnmapViewOfSection(PEPROCESS Process,
                     PVOID BaseAddress)
{
   NTSTATUS Status;
   PMEMORY_AREA MemoryArea;
   PMADDRESS_SPACE AddressSpace;
   PSECTION_OBJECT Section;

   DPRINT("Opening memory area Process %x BaseAddress %x\n",
          Process, BaseAddress);

   ASSERT(Process);

   AddressSpace = &Process->AddressSpace;
   MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace,
                                            BaseAddress);
   if (MemoryArea == NULL ||
       MemoryArea->Type != MEMORY_AREA_SECTION_VIEW ||
       MemoryArea->DeleteInProgress)
   {
      return STATUS_NOT_MAPPED_VIEW;
   }

   Section = MemoryArea->Data.SectionData.Section;

   if (Section->AllocationAttributes & SEC_IMAGE)
   {
      ULONG i;
      ULONG NrSegments;
      PMM_IMAGE_SECTION_OBJECT ImageSectionObject;
      PMM_SECTION_SEGMENT SectionSegments;
      PVOID ImageBaseAddress = 0;
      PMM_SECTION_SEGMENT Segment;

      Segment = MemoryArea->Data.SectionData.Segment;
      ImageSectionObject = Section->ImageSection;
      SectionSegments = ImageSectionObject->Segments;
      NrSegments = ImageSectionObject->NrSegments;

      /* Search for the current segment within the section segments
       * and calculate the image base address */
      for (i = 0; i < NrSegments; i++)
      {
         if (!(SectionSegments[i].Characteristics & IMAGE_SCN_TYPE_NOLOAD))
         {
            if (Segment == &SectionSegments[i])
            {
               ImageBaseAddress = (char*)BaseAddress - (ULONG_PTR)SectionSegments[i].VirtualAddress;
               break;
            }
         }
      }
      if (i >= NrSegments)
      {
         KEBUGCHECK(0);
      }

      for (i = 0; i < NrSegments; i++)
      {
         if (!(SectionSegments[i].Characteristics & IMAGE_SCN_TYPE_NOLOAD))
         {
            PVOID SBaseAddress = (PVOID)
                                 ((char*)ImageBaseAddress + (ULONG_PTR)SectionSegments[i].VirtualAddress);

            Status = MmUnmapViewOfSegment(AddressSpace, SBaseAddress);
         }
      }
   }
   else
   {
      Status = MmUnmapViewOfSegment(AddressSpace, BaseAddress);
   }
   return(STATUS_SUCCESS);
}

/**********************************************************************
 * NAME       EXPORTED
 * NtUnmapViewOfSection
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 * ProcessHandle
 *
 * BaseAddress
 *
 * RETURN VALUE
 * Status.
 *
 * REVISIONS
 */
NTSTATUS STDCALL
NtUnmapViewOfSection (HANDLE ProcessHandle,
                      PVOID BaseAddress)
{
   PEPROCESS Process;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status;

   DPRINT("NtUnmapViewOfSection(ProcessHandle %x, BaseAddress %x)\n",
          ProcessHandle, BaseAddress);

   PreviousMode = ExGetPreviousMode();

   DPRINT("Referencing process\n");
   Status = ObReferenceObjectByHandle(ProcessHandle,
                                      PROCESS_VM_OPERATION,
                                      PsProcessType,
                                      PreviousMode,
                                      (PVOID*)(PVOID)&Process,
                                      NULL);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("ObReferenceObjectByHandle failed (Status %x)\n", Status);
      return(Status);
   }

   MmLockAddressSpace(&Process->AddressSpace);
   Status = MmUnmapViewOfSection(Process, BaseAddress);
   MmUnlockAddressSpace(&Process->AddressSpace);

   ObDereferenceObject(Process);

   return Status;
}


/**
 * Queries the information of a section object.
 *
 * @param SectionHandle
 *        Handle to the section object. It must be opened with SECTION_QUERY
 *        access.
 * @param SectionInformationClass
 *        Index to a certain information structure. Can be either
 *        SectionBasicInformation or SectionImageInformation. The latter
 *        is valid only for sections that were created with the SEC_IMAGE
 *        flag.
 * @param SectionInformation
 *        Caller supplies storage for resulting information.
 * @param Length
 *        Size of the supplied storage.
 * @param ResultLength
 *        Data written.
 *
 * @return Status.
 *
 * @implemented
 */
NTSTATUS STDCALL
NtQuerySection(IN HANDLE SectionHandle,
               IN SECTION_INFORMATION_CLASS SectionInformationClass,
               OUT PVOID SectionInformation,
               IN ULONG SectionInformationLength,
               OUT PULONG ResultLength  OPTIONAL)
{
   PSECTION_OBJECT Section;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;

   PreviousMode = ExGetPreviousMode();

   DefaultQueryInfoBufferCheck(SectionInformationClass,
                               ExSectionInfoClass,
                               SectionInformation,
                               SectionInformationLength,
                               ResultLength,
                               PreviousMode,
                               &Status);

   if(!NT_SUCCESS(Status))
   {
     DPRINT1("NtQuerySection() failed, Status: 0x%x\n", Status);
     return Status;
   }

   Status = ObReferenceObjectByHandle(SectionHandle,
                                      SECTION_QUERY,
                                      MmSectionObjectType,
                                      PreviousMode,
                                      (PVOID*)(PVOID)&Section,
                                      NULL);
   if (NT_SUCCESS(Status))
   {
      switch (SectionInformationClass)
      {
         case SectionBasicInformation:
         {
            PSECTION_BASIC_INFORMATION Sbi = (PSECTION_BASIC_INFORMATION)SectionInformation;

            _SEH_TRY
            {
               Sbi->Attributes = Section->AllocationAttributes;
               if (Section->AllocationAttributes & SEC_IMAGE)
               {
                  Sbi->BaseAddress = 0;
                  Sbi->Size.QuadPart = 0;
               }
               else
               {
                  Sbi->BaseAddress = (PVOID)Section->Segment->VirtualAddress;
                  Sbi->Size.QuadPart = Section->Segment->Length;
               }

               if (ResultLength != NULL)
               {
                  *ResultLength = sizeof(SECTION_BASIC_INFORMATION);
               }
               Status = STATUS_SUCCESS;
            }
            _SEH_HANDLE
            {
               Status = _SEH_GetExceptionCode();
            }
            _SEH_END;

            break;
         }

         case SectionImageInformation:
         {
            PSECTION_IMAGE_INFORMATION Sii = (PSECTION_IMAGE_INFORMATION)SectionInformation;

            _SEH_TRY
            {
               memset(Sii, 0, sizeof(SECTION_IMAGE_INFORMATION));
               if (Section->AllocationAttributes & SEC_IMAGE)
               {
                  PMM_IMAGE_SECTION_OBJECT ImageSectionObject;
                  ImageSectionObject = Section->ImageSection;

                  Sii->EntryPoint = ImageSectionObject->EntryPoint;
                  Sii->StackReserve = ImageSectionObject->StackReserve;
                  Sii->StackCommit = ImageSectionObject->StackCommit;
                  Sii->Subsystem = ImageSectionObject->Subsystem;
                  Sii->MinorSubsystemVersion = ImageSectionObject->MinorSubsystemVersion;
                  Sii->MajorSubsystemVersion = ImageSectionObject->MajorSubsystemVersion;
                  Sii->Characteristics = ImageSectionObject->ImageCharacteristics;
                  Sii->ImageNumber = ImageSectionObject->Machine;
                  Sii->Executable = ImageSectionObject->Executable;
               }

               if (ResultLength != NULL)
               {
                  *ResultLength = sizeof(SECTION_IMAGE_INFORMATION);
               }
               Status = STATUS_SUCCESS;
            }
            _SEH_HANDLE
            {
               Status = _SEH_GetExceptionCode();
            }
            _SEH_END;

            break;
         }
      }

      ObDereferenceObject(Section);
   }

   return(Status);
}


/**
 * Extends size of file backed section.
 *
 * @param SectionHandle
 *        Handle to the section object. It must be opened with
 *        SECTION_EXTEND_SIZE access.
 * @param NewMaximumSize
 *        New maximum size of the section in bytes.
 *
 * @return Status.
 *
 * @todo Move the actual code to internal function MmExtendSection.
 * @unimplemented
 */
NTSTATUS STDCALL
NtExtendSection(IN HANDLE SectionHandle,
                IN PLARGE_INTEGER NewMaximumSize)
{
   LARGE_INTEGER SafeNewMaximumSize;
   PSECTION_OBJECT Section;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;

   PreviousMode = ExGetPreviousMode();

   if(PreviousMode != KernelMode)
   {
     _SEH_TRY
     {
       ProbeForRead(NewMaximumSize,
                    sizeof(LARGE_INTEGER),
                    sizeof(ULONG));
       /* make a copy on the stack */
       SafeNewMaximumSize = *NewMaximumSize;
       NewMaximumSize = &SafeNewMaximumSize;
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;

     if(!NT_SUCCESS(Status))
     {
       return Status;
     }
   }

   Status = ObReferenceObjectByHandle(SectionHandle,
                                      SECTION_EXTEND_SIZE,
                                      MmSectionObjectType,
                                      PreviousMode,
                                      (PVOID*)&Section,
                                      NULL);
   if (!NT_SUCCESS(Status))
   {
      return Status;
   }

   if (!(Section->AllocationAttributes & SEC_FILE))
   {
      ObfDereferenceObject(Section);
      return STATUS_INVALID_PARAMETER;
   }

   /*
    * - Acquire file extneding resource.
    * - Check if we're not resizing the section below it's actual size!
    * - Extend segments if needed.
    * - Set file information (FileAllocationInformation) to the new size.
    * - Release file extending resource.
    */

   ObDereferenceObject(Section);

   return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS STDCALL
MmChangeSectionSize(PSECTION_OBJECT Section,
		    PLARGE_INTEGER NewMaxSize)
{
   PMM_SECTION_SEGMENT Segment;
   ULONG Offset;
   ULONG Length;
   ULONG Entry;
   PFN_TYPE Pfn;
   ULONG ShareCount;

   DPRINT("MmChangeSectionSize\n");

   if (Section->AllocationAttributes & (SEC_IMAGE|SEC_PHYSICALMEMORY) ||
       Section->FileObject == NULL)
   {
      KEBUGCHECK(0);
   }
   if (NewMaxSize->QuadPart > PAGE_ROUND_DOWN(0xffffffff))
   {
      KEBUGCHECK(0);
   }
   DPRINT("%wZ\n", &Section->Segment->FileObject->FileName);
   Segment = Section->Segment;
   MmLockSectionSegment(Segment);
   DPRINT("Segment->Length %x, NewMaxSize %I64x\n" , Segment->Length, NewMaxSize->QuadPart); 
   if (Segment->ReferenceCount > 1)
   {
      DPRINT1("%d\n", Segment->ReferenceCount);
//      KEBUGCHECK(0);
   }
   if (PAGE_ROUND_UP(Segment->Length) > PAGE_ROUND_UP(NewMaxSize->u.LowPart))
   {
      Length = PAGE_ROUND_UP(Segment->Length);
      for (Offset = PAGE_ROUND_UP(NewMaxSize->u.LowPart); Offset < Length; Offset += PAGE_SIZE)
      {
         Entry = MmGetPageEntrySectionSegment(Segment, Offset);
         if (Entry != 0)
         {
            if (IS_SWAP_FROM_SSE(Entry)) 
	    {
	       KEBUGCHECK(0);
	    }
	    else if (MmCheckForPageOp(CcCacheViewMemoryArea, NULL, NULL, Segment, Offset))
	    {
	       /* page operation in progress */
	       KEBUGCHECK(0);
	    }
	    else
	    {
	       Pfn = PFN_FROM_SSE(Entry);
	       if (0 != (ShareCount = MmGetShareCountPage(Pfn)))
	       {
                  /* page is mapped */
	          DPRINT1("%d\n", ShareCount);
	          KEBUGCHECK(0);
	       }
	       else
	       {
	          MmSetPageEntrySectionSegment(Segment, Offset, 0);
		  DPRINT("%x %x\n", Offset, Pfn);
                  MmReleasePageMemoryConsumer(MC_USER, Pfn);
	       }
	    }
         }
      }
   }
   MmChangeSectionSegmentSize(Segment, NewMaxSize->u.LowPart);

   Section->MaximumSize.QuadPart = NewMaxSize->QuadPart;
   MmChangeSectionSegmentSize(Segment, NewMaxSize->u.LowPart);

   MmUnlockSectionSegment(Segment);

   DPRINT("MmChangeSectionSize done\n");

   return STATUS_SUCCESS;
}

/**********************************************************************
 * NAME       INTERNAL
 *  MmAllocateSection@4
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *  Length
 *
 * RETURN VALUE
 *
 * NOTE
 *  Code taken from ntoskrnl/mm/special.c.
 *
 * REVISIONS
 */
PVOID STDCALL
MmAllocateSection (IN ULONG Length, PVOID BaseAddress)
{
   PVOID Result;
   MEMORY_AREA* marea;
   NTSTATUS Status;
   ULONG i;
   PMADDRESS_SPACE AddressSpace;
   PHYSICAL_ADDRESS BoundaryAddressMultiple;

   DPRINT("MmAllocateSection(Length %x)\n",Length);

   BoundaryAddressMultiple.QuadPart = 0;

   AddressSpace = MmGetKernelAddressSpace();
   Result = BaseAddress;
   MmLockAddressSpace(AddressSpace);
   Status = MmCreateMemoryArea (NULL,
                                AddressSpace,
                                MEMORY_AREA_SYSTEM,
                                &Result,
                                Length,
                                0,
                                &marea,
                                FALSE,
                                FALSE,
                                BoundaryAddressMultiple);
   MmUnlockAddressSpace(AddressSpace);

   if (!NT_SUCCESS(Status))
   {
      return (NULL);
   }
   DPRINT("Result %p\n",Result);
   for (i = 0; i < PAGE_ROUND_UP(Length) / PAGE_SIZE; i++)
   {
      PFN_TYPE Page;

      Status = MmRequestPageMemoryConsumer(MC_NPPOOL, TRUE, &Page);
      if (!NT_SUCCESS(Status))
      {
         DbgPrint("Unable to allocate page\n");
         KEBUGCHECK(0);
      }
      Status = MmCreateVirtualMapping (NULL,
                                       (PVOID)((ULONG_PTR)Result + (i * PAGE_SIZE)),
                                       PAGE_READWRITE,
                                       &Page,
                                       1);
      if (!NT_SUCCESS(Status))
      {
         DbgPrint("Unable to create virtual mapping\n");
         KEBUGCHECK(0);
      }
   }
   return ((PVOID)Result);
}


/**********************************************************************
 * NAME       EXPORTED
 * MmMapViewOfSection
 *
 * DESCRIPTION
 * Maps a view of a section into the virtual address space of a
 * process.
 *
 * ARGUMENTS
 * Section
 *  Pointer to the section object.
 *
 * ProcessHandle
 *  Pointer to the process.
 *
 * BaseAddress
 *  Desired base address (or NULL) on entry;
 *  Actual base address of the view on exit.
 *
 * ZeroBits
 *  Number of high order address bits that must be zero.
 *
 * CommitSize
 *  Size in bytes of the initially committed section of
 *  the view.
 *
 * SectionOffset
 *  Offset in bytes from the beginning of the section
 *  to the beginning of the view.
 *
 * ViewSize
 *  Desired length of map (or zero to map all) on entry
 *  Actual length mapped on exit.
 *
 * InheritDisposition
 *  Specified how the view is to be shared with
 *  child processes.
 *
 * AllocationType
 *  Type of allocation for the pages.
 *
 * Protect
 *  Protection for the committed region of the view.
 *
 * RETURN VALUE
 * Status.
 *
 * @implemented
 */
NTSTATUS STDCALL
MmMapViewOfSection(IN PVOID SectionObject,
                   IN PEPROCESS Process,
                   IN OUT PVOID *BaseAddress,
                   IN ULONG ZeroBits,
                   IN ULONG CommitSize,
                   IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
                   IN OUT PULONG ViewSize,
                   IN SECTION_INHERIT InheritDisposition,
                   IN ULONG AllocationType,
                   IN ULONG Protect)
{
   PSECTION_OBJECT Section;
   PMADDRESS_SPACE AddressSpace;
   ULONG ViewOffset;
   NTSTATUS Status = STATUS_SUCCESS;

   ASSERT(Process);

   Section = (PSECTION_OBJECT)SectionObject;
   AddressSpace = &Process->AddressSpace;

   MmLockAddressSpace(AddressSpace);

   if (Section->AllocationAttributes & SEC_IMAGE)
   {
      ULONG i;
      ULONG NrSegments;
      ULONG_PTR ImageBase;
      ULONG ImageSize;
      PMM_IMAGE_SECTION_OBJECT ImageSectionObject;
      PMM_SECTION_SEGMENT SectionSegments;

      ImageSectionObject = Section->ImageSection;
      SectionSegments = ImageSectionObject->Segments;
      NrSegments = ImageSectionObject->NrSegments;


      ImageBase = (ULONG_PTR)*BaseAddress;
      if (ImageBase == 0)
      {
         ImageBase = ImageSectionObject->ImageBase;
      }

      ImageSize = 0;
      for (i = 0; i < NrSegments; i++)
      {
         if (!(SectionSegments[i].Characteristics & IMAGE_SCN_TYPE_NOLOAD))
         {
            ULONG_PTR MaxExtent;
            MaxExtent = (ULONG_PTR)SectionSegments[i].VirtualAddress +
                        SectionSegments[i].Length;
            ImageSize = max(ImageSize, MaxExtent);
         }
      }

      /* Check there is enough space to map the section at that point. */
      if (MmLocateMemoryAreaByRegion(AddressSpace, (PVOID)ImageBase,
                                     PAGE_ROUND_UP(ImageSize)) != NULL)
      {
         /* Fail if the user requested a fixed base address. */
         if ((*BaseAddress) != NULL)
         {
            MmUnlockAddressSpace(AddressSpace);
            return(STATUS_UNSUCCESSFUL);
         }
         /* Otherwise find a gap to map the image. */
         ImageBase = (ULONG_PTR)MmFindGap(AddressSpace, PAGE_ROUND_UP(ImageSize), PAGE_SIZE, FALSE);
         if (ImageBase == 0)
         {
            MmUnlockAddressSpace(AddressSpace);
            return(STATUS_UNSUCCESSFUL);
         }
      }

      for (i = 0; i < NrSegments; i++)
      {
         if (!(SectionSegments[i].Characteristics & IMAGE_SCN_TYPE_NOLOAD))
         {
            PVOID SBaseAddress = (PVOID)
                                 ((char*)ImageBase + (ULONG_PTR)SectionSegments[i].VirtualAddress);
            MmLockSectionSegment(&SectionSegments[i]);
            Status = MmMapViewOfSegment(Process,
                                        AddressSpace,
                                        Section,
                                        &SectionSegments[i],
                                        &SBaseAddress,
                                        SectionSegments[i].Length,
                                        SectionSegments[i].Protection,
                                        0,
                                        FALSE);
            MmUnlockSectionSegment(&SectionSegments[i]);
            if (!NT_SUCCESS(Status))
            {
               MmUnlockAddressSpace(AddressSpace);
               return(Status);
            }
         }
      }

      *BaseAddress = (PVOID)ImageBase;
   }
   else
   {
      if (ViewSize == NULL)
      {
         /* Following this pointer would lead to us to the dark side */
         /* What to do? Bugcheck? Return status? Do the mambo? */
         KEBUGCHECK(MEMORY_MANAGEMENT);
      }

      if (SectionOffset == NULL)
      {
         ViewOffset = 0;
      }
      else
      {
         ViewOffset = SectionOffset->u.LowPart;
      }

      if ((ViewOffset % PAGE_SIZE) != 0)
      {
         MmUnlockAddressSpace(AddressSpace);
         return(STATUS_MAPPED_ALIGNMENT);
      }

      if ((*ViewSize) == 0)
      {
         (*ViewSize) = Section->MaximumSize.u.LowPart - ViewOffset;
      }
      else if (((*ViewSize)+ViewOffset) > Section->MaximumSize.u.LowPart)
      {
         (*ViewSize) = Section->MaximumSize.u.LowPart - ViewOffset;
      }

      MmLockSectionSegment(Section->Segment);
      Status = MmMapViewOfSegment(Process,
                                  AddressSpace,
                                  Section,
                                  Section->Segment,
                                  BaseAddress,
                                  *ViewSize,
                                  Protect,
                                  ViewOffset,
                                  (AllocationType & MEM_TOP_DOWN));
      MmUnlockSectionSegment(Section->Segment);
      if (!NT_SUCCESS(Status))
      {
         MmUnlockAddressSpace(AddressSpace);
         return(Status);
      }
   }

   MmUnlockAddressSpace(AddressSpace);

   return(STATUS_SUCCESS);
}

/*
 * @unimplemented
 */
BOOLEAN STDCALL
MmCanFileBeTruncated (IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
                      IN PLARGE_INTEGER   NewFileSize)
{
   UNIMPLEMENTED;
   return (FALSE);
}


/*
 * @unimplemented
 */
BOOLEAN STDCALL
MmDisableModifiedWriteOfSection (DWORD Unknown0)
{
   UNIMPLEMENTED;
   return (FALSE);
}

/*
 * @implemented
 */
BOOLEAN STDCALL
MmFlushImageSection (IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
                     IN MMFLUSH_TYPE   FlushType)
{
   PMM_IMAGE_SECTION_OBJECT ImageSectionObject = SectionObjectPointer->ImageSectionObject; 
   PMM_SECTION_SEGMENT Segment = SectionObjectPointer->DataSectionObject;
   BOOLEAN Result = FALSE;
   switch(FlushType)
   {
      case MmFlushForDelete:
         Result = TRUE;
         ExAcquireFastMutex(&ImageSectionObjectLock);
         if (ImageSectionObject)
	 {
	    if(ImageSectionObject->RefCount == 0)
	    {
               DPRINT1("%x %d\n", ImageSectionObject, ImageSectionObject->RefCount);
               MmFreeImageSectionSegments(SectionObjectPointer);
	    }
	    else
	    {
	       Result = FALSE;
	    }
	 }
         ExReleaseFastMutex(&ImageSectionObjectLock);
#if 1
         ExAcquireFastMutex(&DataSectionObjectLock);
	 if (Segment)
	 {
            DPRINT("%d %wZ\n", Segment->ReferenceCount, &Segment->FileObject->FileName);
	    if (Segment->ReferenceCount == 0)
	    {
	       MmFreeDataSectionSegments(SectionObjectPointer);
	    }
	    else
	    {
//	       Result = FALSE;
	    }
	 }
         ExReleaseFastMutex(&DataSectionObjectLock);
#endif
	 if (Result)
	 {
#if 1
//	     KEBUGCHECK(0);
#else
	     CcRosSetRemoveOnClose(SectionObjectPointer);
#endif
	 }
         break;
      case MmFlushForWrite:
	 Result = TRUE;
         ExAcquireFastMutex(&ImageSectionObjectLock);
         if (ImageSectionObject)
	 {
	    if (ImageSectionObject->RefCount == 0)
	    {
	       MmFreeImageSectionSegments(SectionObjectPointer);
	    }
	    else
	    {
	       Result = FALSE;
	    }
	 }
         ExReleaseFastMutex(&ImageSectionObjectLock);
#if 0
         ExAcquireFastMutex(&DataSectionObjectLock);
	 if (Segment)
	 {
	    CHECKPOINT;
	    if (Segment->ReferenceCount == 0)
	    {
	       MmFreeDataSectionSegments(SectionObjectPointer);
	    }
	    else
	    {
	       Result = FALSE;
	    }
	 }
         ExReleaseFastMutex(&DataSectionObjectLock);
#endif
         break;
   }
   return Result;
}

/*
 * @unimplemented
 */
BOOLEAN STDCALL
MmForceSectionClosed (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
    IN BOOLEAN                  DelayClose)
{
   UNIMPLEMENTED;
   return (FALSE);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
MmMapViewInSystemSpace (IN PVOID SectionObject,
                        OUT PVOID * MappedBase,
                        IN OUT PULONG ViewSize)
{
   PSECTION_OBJECT Section;
   PMADDRESS_SPACE AddressSpace;
   NTSTATUS Status;

   DPRINT("MmMapViewInSystemSpace() called\n");

   Section = (PSECTION_OBJECT)SectionObject;
   AddressSpace = MmGetKernelAddressSpace();

   MmLockAddressSpace(AddressSpace);


   if ((*ViewSize) == 0)
   {
      (*ViewSize) = Section->MaximumSize.u.LowPart;
   }
   else if ((*ViewSize) > Section->MaximumSize.u.LowPart)
   {
      (*ViewSize) = Section->MaximumSize.u.LowPart;
   }

   MmLockSectionSegment(Section->Segment);


   Status = MmMapViewOfSegment(NULL,
                               AddressSpace,
                               Section,
                               Section->Segment,
                               MappedBase,
                               *ViewSize,
                               PAGE_READWRITE,
                               0,
                               FALSE);

   MmUnlockSectionSegment(Section->Segment);
   MmUnlockAddressSpace(AddressSpace);

   return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
MmMapViewInSessionSpace (
    IN PVOID Section,
    OUT PVOID *MappedBase,
    IN OUT PSIZE_T ViewSize
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
MmUnmapViewInSystemSpace (IN PVOID MappedBase)
{
   PMADDRESS_SPACE AddressSpace;
   NTSTATUS Status;

   DPRINT("MmUnmapViewInSystemSpace() called\n");

   AddressSpace = MmGetKernelAddressSpace();

   Status = MmUnmapViewOfSegment(AddressSpace, MappedBase);

   return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
MmUnmapViewInSessionSpace (
    IN PVOID MappedBase
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
MmSetBankedSection (DWORD Unknown0,
                    DWORD Unknown1,
                    DWORD Unknown2,
                    DWORD Unknown3,
                    DWORD Unknown4,
                    DWORD Unknown5)
{
   UNIMPLEMENTED;
   return (STATUS_NOT_IMPLEMENTED);
}


/**********************************************************************
 * NAME       EXPORTED
 *  MmCreateSection@
 *
 * DESCRIPTION
 *  Creates a section object.
 *
 * ARGUMENTS
 * SectionObject (OUT)
 *  Caller supplied storage for the resulting pointer
 *  to a SECTION_OBJECT instance;
 *
 * DesiredAccess
 *  Specifies the desired access to the section can be a
 *  combination of:
 *   STANDARD_RIGHTS_REQUIRED |
 *   SECTION_QUERY   |
 *   SECTION_MAP_WRITE  |
 *   SECTION_MAP_READ  |
 *   SECTION_MAP_EXECUTE
 *
 * ObjectAttributes [OPTIONAL]
 *  Initialized attributes for the object can be used
 *  to create a named section;
 *
 * MaximumSize
 *  Maximizes the size of the memory section. Must be
 *  non-NULL for a page-file backed section.
 *  If value specified for a mapped file and the file is
 *  not large enough, file will be extended.
 *
 * SectionPageProtection
 *  Can be a combination of:
 *   PAGE_READONLY |
 *   PAGE_READWRITE |
 *   PAGE_WRITEONLY |
 *   PAGE_WRITECOPY
 *
 * AllocationAttributes
 *  Can be a combination of:
 *   SEC_IMAGE |
 *   SEC_RESERVE
 *
 * FileHandle
 *  Handle to a file to create a section mapped to a file
 *  instead of a memory backed section;
 *
 * File
 *  Unknown.
 *
 * RETURN VALUE
 *  Status.
 *
 * @implemented
 */
NTSTATUS STDCALL
MmCreateSection (OUT PSECTION_OBJECT  * SectionObject,
                 IN ACCESS_MASK  DesiredAccess,
                 IN POBJECT_ATTRIBUTES ObjectAttributes     OPTIONAL,
                 IN PLARGE_INTEGER  MaximumSize,
                 IN ULONG   SectionPageProtection,
                 IN ULONG   AllocationAttributes,
                 IN HANDLE   FileHandle   OPTIONAL,
                 IN PFILE_OBJECT  FileObject   OPTIONAL)
{
   NTSTATUS Status;
   ULONG FileAccess;
   BOOLEAN FileObjectCreated;

   if (AllocationAttributes & SEC_IMAGE &&
       FileHandle == NULL &&
       FileObject == NULL)
   {
      return STATUS_INVALID_PARAMETER;
   }
   
   if (FileHandle && FileObject == NULL)
   {
      if (SectionPageProtection & PAGE_READWRITE ||
          SectionPageProtection & PAGE_EXECUTE_READWRITE)
      {
         FileAccess = FILE_READ_DATA | FILE_WRITE_DATA;
      }
      else
      {
         FileAccess = FILE_READ_DATA;
      }

      Status = ObReferenceObjectByHandle(FileHandle,
	                                 FileAccess,
					 IoFileObjectType,
					 UserMode,
					 (PVOID*)&FileObject,
					 NULL);
      if (!NT_SUCCESS(Status))
      {
         return Status;
      }
      FileObjectCreated = TRUE;
   }
   else
   {
      FileObjectCreated = FALSE;
   }

   if (AllocationAttributes & SEC_IMAGE)
   {
      Status = MmCreateImageSection(SectionObject,
                                    DesiredAccess,
                                    ObjectAttributes,
                                    MaximumSize,
                                    SectionPageProtection,
                                    AllocationAttributes,
                                    FileObject);
   }
   else if (FileObject != NULL)
   {
      DPRINT("%wZ\n", &FileObject->FileName);
      Status = MmCreateDataFileSection(SectionObject,
	                               DesiredAccess,
				       ObjectAttributes,
				       MaximumSize,
				       SectionPageProtection,
				       AllocationAttributes,
				       FileObject,
				       FALSE);
   }
   else
   {
      Status = MmCreatePageFileSection(SectionObject,
                                       DesiredAccess,
                                       ObjectAttributes,
                                       MaximumSize,
                                       SectionPageProtection,
                                       AllocationAttributes);
   }
   if (FileObjectCreated)
   {
      ObDereferenceObject(FileObject);
   }
   return Status;
}


NTSTATUS
MmRosTrimImageSectionObjects(ULONG Target, ULONG Priority, PULONG NrFreed)
{
   PMM_IMAGE_SECTION_OBJECT current;
   ULONG NrSegments;
   ULONG Length;
   ULONG i;
   ULONG Offset;
   ULONG Entry;
   PMM_SECTION_SEGMENT SectionSegments;

   DPRINT("MmRosTrimImageSectionObjects\n");


   ExAcquireFastMutex(&ImageSectionObjectLock);
   if (ImageSectionObjectCount > 0)
   {
      if (ImageSectionObjectNext == NULL)
      {
         ImageSectionObjectNext = CONTAINING_RECORD(ImageSectionObjectListHead.Blink, MM_IMAGE_SECTION_OBJECT, ListEntry);
      }
      current = ImageSectionObjectNext;


      do 
      {
         if (current->RefCount == 0)
         {
            NrSegments = current->NrSegments;
            SectionSegments = current->Segments;
            for (i = 0; i < NrSegments && *NrFreed < Target; i++)
            {
               Length = PAGE_ROUND_UP(SectionSegments[i].Length);
	       for (Offset = 0; Offset < Length && *NrFreed < Target; Offset += PAGE_SIZE)
	       {
	          Entry = MmGetPageEntrySectionSegment(&SectionSegments[i], Offset);
	          if (Entry != 0)
	          {
	             if (IS_SWAP_FROM_SSE(Entry)) 
	             {
	                KEBUGCHECK(0);
	             }
	             else if (MmGetShareCountPage(PFN_FROM_SSE(Entry)) != 0)
	             {
	                DPRINT1("%d %x\n", i, Offset, MmGetShareCountPage(PFN_FROM_SSE(Entry)));
	                KEBUGCHECK(0);
	             }
	             else
	             {
	                MmReleasePageMemoryConsumer(MC_USER, PFN_FROM_SSE(Entry));
		        MmSetPageEntrySectionSegment(&SectionSegments[i], Offset, 0);
		        (*NrFreed)++;
	             }
	          }
	       }
	    }
	    if (*NrFreed >= Target)
	    {
	       break;
	    }
	 }

         if (current->ListEntry.Blink == &ImageSectionObjectListHead)
         {
            current = CONTAINING_RECORD(ImageSectionObjectListHead.Blink, MM_IMAGE_SECTION_OBJECT, ListEntry);
         }
         else
         {
            current = CONTAINING_RECORD(current->ListEntry.Blink, MM_IMAGE_SECTION_OBJECT, ListEntry);
         }
      } 
      while (*NrFreed < Target && current != ImageSectionObjectNext);
      ImageSectionObjectNext = current;
   }
   ExReleaseFastMutex(&ImageSectionObjectLock);

   DPRINT("MmRosTrimImageSectionObjects done\n");

   return STATUS_SUCCESS;
}

NTSTATUS
MmspWriteDataSectionPage(PMM_SECTION_SEGMENT Segment,
			 ULONG Offset,
			 PMM_PAGEOP PageOp)
{
   ULONG Entry;
   PFN_TYPE Pfn;
   LARGE_INTEGER FileOffset;
   IO_STATUS_BLOCK Iosb;
   NTSTATUS Status;
   KEVENT Event;
   UCHAR MdlBase[sizeof(MDL) + sizeof(ULONG)];
   PMDL Mdl = (PMDL)MdlBase;

   Entry = MmGetPageEntrySectionSegment(Segment, Offset);
   Pfn = PFN_FROM_SSE(Entry);

   if (MmGetShareCountPage(Pfn))
   {
      MmSetCleanAllRmaps(Pfn);
   }
   MmSetPageEntrySectionSegment(Segment, Offset, Entry & ~0x2);

   MmInitializeMdl(Mdl, NULL, PAGE_SIZE);
   MmBuildMdlFromPages(Mdl, &Pfn);
   Mdl->MdlFlags |= (MDL_PAGES_LOCKED | MDL_WRITE_OPERATION);

   FileOffset.QuadPart = Offset * PAGE_SIZE;

   KeInitializeEvent(&Event, NotificationEvent, FALSE);
   Status = IoSynchronousPageWrite(Segment->FileObject,
                                   Mdl,
                                   &FileOffset,
                                   &Event,
                                   &Iosb);
   if (Status == STATUS_PENDING)
   {
      KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
      Status = Iosb.Status;
   }
   MmUnmapLockedPages(Mdl->MappedSystemVa, Mdl);            
   if (!NT_SUCCESS(Status))
   {
      MmSetPageEntrySectionSegment(Segment, Offset, Entry | 0x2);
   }
   PageOp->Status = Status;
   MmspCompleteAndReleasePageOp(PageOp);
   return Status;
}

typedef struct _PAGE_IO_CONTEXT
{
   PMM_SECTION_SEGMENT Segment;
   ULONG Offset;
   ULONG PageCount;
   PMM_PAGEOP PageOp[16];
   WORK_QUEUE_ITEM WorkQueueItem; 
} PAGE_IO_CONTEXT, *PPAGE_IO_CONTEXT;

VOID STDCALL
MmspWriteDataSectionPages(PVOID Context)
{
   PMDL Mdl;
   ULONG i;
   PPFN_TYPE Pfn;
   ULONG Offset;
   ULONG Entry;
   LARGE_INTEGER FileOffset;
   KEVENT Event;
   IO_STATUS_BLOCK Iosb;
   NTSTATUS Status;

   DPRINT("MmspWriteDataSectionPages\n");
   
   Mdl = alloca(MmSizeOfMdl(NULL, ((PPAGE_IO_CONTEXT)Context)->PageCount * PAGE_SIZE));
   Pfn = alloca(((PPAGE_IO_CONTEXT)Context)->PageCount * sizeof(PFN_TYPE));

   Offset = ((PPAGE_IO_CONTEXT)Context)->Offset;
   for (i = 0; i < ((PPAGE_IO_CONTEXT)Context)->PageCount; i++)
   {
      Entry = MmGetPageEntrySectionSegment(((PPAGE_IO_CONTEXT)Context)->Segment, Offset);
      DPRINT("Offset %x,  Entry %x\n", Offset, Entry);
      ASSERT (!IS_SWAP_FROM_SSE(Entry));
      ASSERT (PAGE_FROM_SSE(Entry));
      ASSERT ((Entry & 0x2));

      Pfn[i] = PFN_FROM_SSE(Entry);
      MmSetPageEntrySectionSegment(((PPAGE_IO_CONTEXT)Context)->Segment, Offset, Entry & ~0x2);
      if (MmGetShareCountPage(Pfn[i]))
      {
         MmSetCleanAllRmaps(Pfn[i]);
      }
      Offset += PAGE_SIZE;
   }

   MmInitializeMdl(Mdl, NULL, ((PPAGE_IO_CONTEXT)Context)->PageCount * PAGE_SIZE);
   MmBuildMdlFromPages(Mdl, Pfn);

   FileOffset.u.HighPart = 0;
   FileOffset.u.LowPart = ((PPAGE_IO_CONTEXT)Context)->Offset;

   KeInitializeEvent(&Event, NotificationEvent, FALSE);
   Status = IoSynchronousPageWrite(((PPAGE_IO_CONTEXT)Context)->Segment->FileObject,
                                   Mdl,
                                   &FileOffset,
                                   &Event,
                                   &Iosb);
   if (Status == STATUS_PENDING)
   {
      KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
      Status = Iosb.Status;
   }
   if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA)
   {
      MmUnmapLockedPages(Mdl->MappedSystemVa, Mdl);
   }
   Offset = ((PPAGE_IO_CONTEXT)Context)->Offset;
   for (i = 0; i < ((PPAGE_IO_CONTEXT)Context)->PageCount; i++)
   {
      if (!NT_SUCCESS(Status))
      {
         Entry = MmGetPageEntrySectionSegment(((PPAGE_IO_CONTEXT)Context)->Segment, Offset);
         MmSetPageEntrySectionSegment(((PPAGE_IO_CONTEXT)Context)->Segment, Offset, Entry | 0x2);
	 Offset += PAGE_SIZE;
      }
      ((PPAGE_IO_CONTEXT)Context)->PageOp[i]->Status = Status;
      MmspCompleteAndReleasePageOp(((PPAGE_IO_CONTEXT)Context)->PageOp[i]);
   }
   ExFreePool(Context);
}

VOID STDCALL
MmspWorkerThread(PVOID Pointer)
{
   PLIST_ENTRY entry;
   PMM_SECTION_SEGMENT current;
   ULONG Entry;
   ULONG i;
   PMM_PAGEOP PageOp;
   PPAGE_IO_CONTEXT Context = NULL;
   PFN_TYPE Pfn;


   while(1)
   {
      KeWaitForSingleObject(&MmspWorkerThreadTimer,
	                   0,
			   KernelMode,
			   FALSE,
			   NULL);

      DPRINT("MmspWorkerThread\n");

      ExAcquireFastMutex(&DataSectionObjectLock);
      
      entry = DataSectionObjectListHead.Flink;
      while (entry != &DataSectionObjectListHead)
      {
         current = CONTAINING_RECORD(entry, MM_SECTION_SEGMENT, ListEntry);
         MmLockSectionSegment(current);
	 entry = entry->Flink;
         Context = NULL;
	 for (i = 0; i < PAGE_ROUND_UP(current->Length)/PAGE_SIZE; i++)
	 {
            Entry = MmGetPageEntrySectionSegment(current, i * PAGE_SIZE);
	    Pfn = PFN_FROM_SSE(Entry);

	    if (!IS_SWAP_FROM_SSE(Entry) && Pfn && !(Entry & 0x2) && MmGetShareCountPage(Pfn) && MmIsDirtyPageRmap(Pfn))
	    {
	       Entry |= 0x2;
	       MmSetPageEntrySectionSegment(current, i * PAGE_SIZE, Entry);
	    }
            if (!IS_SWAP_FROM_SSE(Entry) && Pfn && (Entry & 0x2))
	    {
	       PageOp = MmGetPageOp(CcCacheViewMemoryArea, NULL, NULL, current, i * PAGE_SIZE, MM_PAGEOP_PAGESYNCH, TRUE);
               if (PageOp)
	       {
		  if (Context == NULL)
		  {
		     Context = ExAllocatePool(NonPagedPool, sizeof(PAGE_IO_CONTEXT));
		     Context->Segment = current;
		     Context->Offset = i * PAGE_SIZE;
		     Context->PageCount = 0;
		     ExInitializeWorkItem(&Context->WorkQueueItem, MmspWriteDataSectionPages, Context);
		  }
		  Context->PageOp[Context->PageCount++] = PageOp;
		  if (Context->PageCount == 16)
		  {
                     ExQueueWorkItem(&Context->WorkQueueItem, HyperCriticalWorkQueue);
		     Context = NULL;
		  }
	       }
	       else
	       {
		  if (Context)
		  {
                     ExQueueWorkItem(&Context->WorkQueueItem, HyperCriticalWorkQueue);
		     Context = NULL;
		  }
	       }
	    }
            else
	    {
	       if (Context)
	       {
                  ExQueueWorkItem(&Context->WorkQueueItem, HyperCriticalWorkQueue);
		  Context = NULL;
	       }
	    }
	 }
         if (Context)
	 {
            ExQueueWorkItem(&Context->WorkQueueItem, HyperCriticalWorkQueue);
	    Context = NULL;
	 }
	 
	 MmUnlockSectionSegment(current);
      }
      ExReleaseFastMutex(&DataSectionObjectLock);
   }
}

ULONG MmGetPageEntryForProcess(PEPROCESS Process, PVOID Address);

NTSTATUS STDCALL
MmMapViewInSystemCache(PCACHE_VIEW CacheView)
{
   KIRQL oldIrql;
   PSECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;

   DPRINT("MmMapViewInSystemCache(%x)\n", CacheView);

   Section = CacheView->SectionData.Section;
   Segment = CacheView->SectionData.Segment;

   MmLockSectionSegment(Segment);

   KeAcquireSpinLock(&Section->ViewListLock, &oldIrql);
   InsertTailList(&Section->ViewListHead,
                  &CacheView->SectionData.ViewListEntry);
   KeReleaseSpinLock(&Section->ViewListLock, oldIrql);

   ObReferenceObjectByPointer((PVOID)Section,
                              SECTION_MAP_READ|SECTION_MAP_WRITE,
                              NULL,
                              KernelMode);

   MmInitialiseRegion(&CacheView->SectionData.RegionListHead,
                      CACHE_VIEW_SIZE, 0, PAGE_READWRITE);

   MmUnlockSectionSegment(Segment);
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL
MmUnmapViewInSystemCache(PCACHE_VIEW CacheView)
{
   PSECTION_OBJECT Section;
   PMM_SECTION_SEGMENT Segment;
   KIRQL oldIrql;
   PLIST_ENTRY CurrentEntry;
   PMM_REGION CurrentRegion;
   PLIST_ENTRY RegionListHead;
   BOOL Dirty;
   ULONG Offset;
   PFN_TYPE Pfn;

   ASSERT(CacheView);

   Section = CacheView->SectionData.Section;
   Segment = CacheView->SectionData.Segment;

   DPRINT("%x %x\n", CacheView->BaseAddress, CacheView->SectionData.ViewOffset);

   MmLockSectionSegment(Segment);
   KeAcquireSpinLock(&Section->ViewListLock, &oldIrql);
   RemoveEntryList(&CacheView->SectionData.ViewListEntry);
   KeReleaseSpinLock(&Section->ViewListLock, oldIrql);

   RegionListHead = &CacheView->SectionData.RegionListHead;
   while (!IsListEmpty(RegionListHead))
   {
      CurrentEntry = RemoveHeadList(RegionListHead);
      CurrentRegion = CONTAINING_RECORD(CurrentEntry, MM_REGION, RegionListEntry);
      ExFreePool(CurrentRegion);
   }

   for (Offset = 0; Offset < CACHE_VIEW_SIZE; Offset += PAGE_SIZE)
   {
      if (MmIsPageSwapEntry(NULL, (PVOID)CacheView->BaseAddress + Offset))
      {
         KEBUGCHECK(0);
      }
      Pfn = 0;
      MmDeleteVirtualMapping(NULL, (PVOID)CacheView->BaseAddress + Offset, FALSE, &Dirty, &Pfn);
      if (Pfn)
      {
         MmDeleteRmap(Pfn, NULL, (PVOID)CacheView->BaseAddress + Offset);
         MmUnsharePageEntrySectionSegment(Section, Segment, CacheView->SectionData.ViewOffset + Offset, Dirty, FALSE);
      }
   }
   MmUnlockSectionSegment(Segment);
   ObDereferenceObject(Section);
   return(STATUS_SUCCESS);
}

NTSTATUS
MmFlushDataFileSection(PSECTION_OBJECT Section, PLARGE_INTEGER StartOffset, ULONG Length)
{
   PMM_SECTION_SEGMENT Segment;
   LARGE_INTEGER Offset;
   PPAGE_IO_CONTEXT Context = NULL;
   ULONG Entry, i;
   PMM_PAGEOP PageOp;
   PFN_TYPE Pfn;

   Segment = Section->Segment;

   if (StartOffset)
   {
      Offset.QuadPart = PAGE_ROUND_DOWN(StartOffset->QuadPart);
      Length = PAGE_ROUND_UP(StartOffset->QuadPart + Length);
   }
   else
   {
      Offset.QuadPart = 0;
      Length = PAGE_ROUND_UP(Segment->Length);
   }

   MmLockSectionSegment(Segment);

   Context = NULL;
   for (i = Offset.u.LowPart / PAGE_SIZE; i < Length/PAGE_SIZE; i++)
   {
       Entry = MmGetPageEntrySectionSegment(Segment, i * PAGE_SIZE);
       Pfn = PFN_FROM_SSE(Entry);
       if (!IS_SWAP_FROM_SSE(Entry) && Pfn && !(Entry & 0x2) && MmGetShareCountPage(Pfn) && MmIsDirtyPageRmap(Pfn))
       {
          Entry|=0x2;
	  MmSetPageEntrySectionSegment(Segment, i * PAGE_SIZE, Entry);
       }
       if (!IS_SWAP_FROM_SSE(Entry) && Pfn && (Entry & 0x2))
       {
	  PageOp = MmGetPageOp(CcCacheViewMemoryArea, NULL, NULL, Segment, i * PAGE_SIZE, MM_PAGEOP_PAGESYNCH, TRUE);
          if (PageOp)
	  {
	     if (Context == NULL)
	     {
		Context = ExAllocatePool(NonPagedPool, sizeof(PAGE_IO_CONTEXT));
		Context->Segment = Segment;
		Context->Offset = i * PAGE_SIZE;
		Context->PageCount = 0;
	     }
	     Context->PageOp[Context->PageCount++] = PageOp;
	     if (Context->PageCount == 16)
	     {
		MmUnlockSectionSegment(Segment);
                MmspWriteDataSectionPages(Context);
		Context = NULL;
		MmLockSectionSegment(Segment);
	     }
	  }
	  else
	  {
	     CHECKPOINT;
	     if (Context)
	     {
		MmUnlockSectionSegment(Segment);
                MmspWriteDataSectionPages(Context);
		Context = NULL;
		MmLockSectionSegment(Segment);
	     }
	  }
       }
       else
       {
	  if (Context)
	  {
	     MmUnlockSectionSegment(Segment);
             MmspWriteDataSectionPages(Context);
	     Context = NULL;
	     MmLockSectionSegment(Segment);
	  }
       }
   }
   MmUnlockSectionSegment(Segment);
   if (Context)
   {
      MmspWriteDataSectionPages(Context);
   }
   return STATUS_SUCCESS;
}

/* EOF */

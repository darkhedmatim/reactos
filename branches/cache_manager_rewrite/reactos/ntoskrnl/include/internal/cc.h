#ifndef __INCLUDE_INTERNAL_CC_H
#define __INCLUDE_INTERNAL_CC_H

/* $Id$ */
#include <ddk/ntifs.h>
#include <reactos/bugcodes.h>

#define CACHE_VIEW_SIZE	(128 * 1024) // 128kB

struct _BCB;

typedef struct
{
   SECTION_DATA SectionData;
   PVOID BaseAddress;
   ULONG RefCount;
   struct _BCB* Bcb;
   LIST_ENTRY ListEntry;
} CACHE_VIEW, *PCACHE_VIEW;

typedef struct _BCB
{
  PFILE_OBJECT FileObject;
  CC_FILE_SIZES FileSizes;
  BOOLEAN PinAccess;
  PCACHE_MANAGER_CALLBACKS CallBacks;
  PVOID LazyWriterContext;
  ULONG RefCount;
  PCACHE_VIEW CacheView[2048];
  PVOID LargeCacheView;
  PSECTION_OBJECT Section;
} BCB, *PBCB;


typedef struct _INTERNAL_BCB
{
  PBCB Bcb;
  ULONG Index;
//  CSHORT RefCount; /* (At offset 0x34 on WinNT4) */
} INTERNAL_BCB, *PINTERNAL_BCB;

VOID STDCALL
CcMdlReadCompleteDev (IN PMDL		MdlChain,
		      IN PDEVICE_OBJECT	DeviceObject);

VOID
CcInitView(VOID);


VOID CcInit(VOID);


VOID 
CcInitCacheZeroPage(VOID);

NTSTATUS
CcRosFlushDirtyPages(ULONG Target, PULONG Count);

VOID 
CcRosDereferenceCache(PFILE_OBJECT FileObject);

VOID 
CcRosReferenceCache(PFILE_OBJECT FileObject);

VOID 
CcRosSetRemoveOnClose(PSECTION_OBJECT_POINTERS SectionObjectPointer);


/*
 * Macro for generic cache manage bugchecking. Note that this macro assumes
 * that the file name including extension is always longer than 4 characters.
 */
#define KEBUGCHECKCC \
    KEBUGCHECKEX(CACHE_MANAGER, \
    (*(DWORD*)(__FILE__ + sizeof(__FILE__) - 4) << 16) | \
    (__LINE__ & 0xFFFF), 0, 0, 0)

#endif

/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Rfsd File System Driver for WinNT/2K/XP
 * FILE:             cmcb.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://rfsd.yeah.net
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "rfsdfs.h"

/* GLOBALS ***************************************************************/

extern PRFSD_GLOBAL RfsdGlobal;

/* DEFINITIONS *************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RfsdAcquireForLazyWrite)
#pragma alloc_text(PAGE, RfsdReleaseFromLazyWrite)
#pragma alloc_text(PAGE, RfsdAcquireForReadAhead)
#pragma alloc_text(PAGE, RfsdReleaseFromReadAhead)
#pragma alloc_text(PAGE, RfsdNoOpRelease)
#pragma alloc_text(PAGE, RfsdNoOpRelease)
#endif


BOOLEAN
RfsdAcquireForLazyWrite (
        IN PVOID    Context,
        IN BOOLEAN  Wait)
{
    //
    // On a readonly filesystem this function still has to exist but it
    // doesn't need to do anything.
    
    PRFSD_FCB    Fcb;
    
    Fcb = (PRFSD_FCB) Context;
    
    ASSERT(Fcb != NULL);
    
    ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
        (Fcb->Identifier.Size == sizeof(RFSD_FCB)));

    RfsdPrint((DBG_INFO, "RfsdAcquireForLazyWrite: %s %s %s\n",
        RfsdGetCurrentProcessName(),
        "ACQUIRE_FOR_LAZY_WRITE",
        Fcb->AnsiFileName.Buffer        ));

    if (!IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY)) {
        RfsdPrint(( DBG_INFO, "RfsdAcquireForLazyWrite: Key=%x,%xh %S\n", 
			Fcb->RfsdMcb->Key.k_dir_id, Fcb->RfsdMcb->Key.k_objectid, Fcb->RfsdMcb->ShortName.Buffer ));

        if(!ExAcquireResourceSharedLite(
            &Fcb->PagingIoResource, Wait)) {
            return FALSE;
        }
    }

    ASSERT(IoGetTopLevelIrp() == NULL);

    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    return TRUE;
}

VOID
RfsdReleaseFromLazyWrite (IN PVOID Context)
{
    //
    // On a readonly filesystem this function still has to exist but it
    // doesn't need to do anything.
    PRFSD_FCB Fcb;
    
    Fcb = (PRFSD_FCB) Context;
    
    ASSERT(Fcb != NULL);
    
    ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
        (Fcb->Identifier.Size == sizeof(RFSD_FCB)));

    RfsdPrint((DBG_INFO, "RfsdReleaseFromLazyWrite: %s %s %s\n",
        RfsdGetCurrentProcessName(),
        "RELEASE_FROM_LAZY_WRITE",
        Fcb->AnsiFileName.Buffer
        ));

    if (!IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY)) {
        RfsdPrint(( DBG_INFO, "RfsdReleaseFromLazyWrite: Inode=%x%xh %S\n", 
			Fcb->RfsdMcb->Key.k_dir_id, Fcb->RfsdMcb->Key.k_objectid, Fcb->RfsdMcb->ShortName.Buffer ));

        ExReleaseResource(&Fcb->PagingIoResource);
    }

    ASSERT(IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    IoSetTopLevelIrp( NULL );

}

BOOLEAN
RfsdAcquireForReadAhead (IN PVOID    Context,
             IN BOOLEAN  Wait)
{
    PRFSD_FCB    Fcb;
    
    Fcb = (PRFSD_FCB) Context;
    
    ASSERT(Fcb != NULL);
    
    ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
        (Fcb->Identifier.Size == sizeof(RFSD_FCB)));

    RfsdPrint(( DBG_INFO, "RfsdAcquireForReadAhead: Inode=%x,%xh %S\n", 
			Fcb->RfsdMcb->Key.k_dir_id, Fcb->RfsdMcb->Key.k_objectid, Fcb->RfsdMcb->ShortName.Buffer ));

    if (!ExAcquireResourceShared(
        &Fcb->MainResource, Wait  ))
        return FALSE;

    ASSERT(IoGetTopLevelIrp() == NULL);

    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    return TRUE;
}

VOID
RfsdReleaseFromReadAhead (IN PVOID Context)
{
    PRFSD_FCB Fcb;
    
    Fcb = (PRFSD_FCB) Context;
    
    ASSERT(Fcb != NULL);
    
    ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
        (Fcb->Identifier.Size == sizeof(RFSD_FCB)));

    RfsdPrint(( DBG_INFO, "RfsdReleaseFromReadAhead: Inode=%x,%xh %S\n", 
			Fcb->RfsdMcb->Key.k_dir_id, Fcb->RfsdMcb->Key.k_objectid, Fcb->RfsdMcb->ShortName.Buffer ));

    IoSetTopLevelIrp( NULL );

    ExReleaseResource(&Fcb->MainResource);
}

BOOLEAN
RfsdNoOpAcquire (
    IN PVOID Fcb,
    IN BOOLEAN Wait
    )
{
    UNREFERENCED_PARAMETER( Fcb );
    UNREFERENCED_PARAMETER( Wait );

    //
    //  This is a kludge because Cc is really the top level.  We it
    //  enters the file system, we will think it is a resursive call
    //  and complete the request with hard errors or verify.  It will
    //  have to deal with them, somehow....
    //

    ASSERT(IoGetTopLevelIrp() == NULL);

    IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    return TRUE;
}

VOID
RfsdNoOpRelease (
    IN PVOID Fcb
    )
{
    //
    //  Clear the kludge at this point.
    //

    ASSERT(IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

    IoSetTopLevelIrp( NULL );

    UNREFERENCED_PARAMETER( Fcb );

    return;
}

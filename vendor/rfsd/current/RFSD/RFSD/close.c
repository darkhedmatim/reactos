/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Rfsd File System Driver for WinNT/2K/XP
 * FILE:             close.c
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
#pragma alloc_text(PAGE, RfsdClose)
#pragma alloc_text(PAGE, RfsdQueueCloseRequest)
#pragma alloc_text(PAGE, RfsdDeQueueCloseRequest)
#endif

NTSTATUS
RfsdClose (IN PRFSD_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status = STATUS_SUCCESS;
    PRFSD_VCB       Vcb;
    BOOLEAN         VcbResourceAcquired = FALSE;
    PFILE_OBJECT    FileObject;
    PRFSD_FCB       Fcb;
    BOOLEAN         FcbResourceAcquired = FALSE;
    PRFSD_CCB       Ccb;
    BOOLEAN         FreeVcb = FALSE;

    __try
    {
        ASSERT(IrpContext != NULL);
        
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;

        if (DeviceObject == RfsdGlobal->DeviceObject)
        {
            Status = STATUS_SUCCESS;
            __leave;
        }
        
        Vcb = (PRFSD_VCB) DeviceObject->DeviceExtension;
        
        ASSERT(Vcb != NULL);
        
        ASSERT((Vcb->Identifier.Type == RFSDVCB) &&
            (Vcb->Identifier.Size == sizeof(RFSD_VCB)));

        ASSERT(IsMounted(Vcb));

        if (!ExAcquireResourceExclusiveLite(
            &Vcb->MainResource,
            IrpContext->IsSynchronous )) {
            RfsdPrint((DBG_INFO, "RfsdClose: PENDING ... Vcb: %xh/%xh\n",
                                 Vcb->OpenFileHandleCount, Vcb->ReferenceCount));

            Status = STATUS_PENDING;
            __leave;
        }
        
        VcbResourceAcquired = TRUE;

        FileObject = IrpContext->FileObject;

        if (IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DELAY_CLOSE)) {
            Fcb = IrpContext->Fcb;
            Ccb = IrpContext->Ccb;
        } else {
            Fcb = (PRFSD_FCB) FileObject->FsContext;
        
            if (!Fcb)
            {
                Status = STATUS_SUCCESS;
                __leave;
            }

            ASSERT(Fcb != NULL);

            Ccb = (PRFSD_CCB) FileObject->FsContext2;
        }
        
        if (Fcb->Identifier.Type == RFSDVCB) {

            Vcb->ReferenceCount--;

            if (!Vcb->ReferenceCount && FlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING))
            {
                FreeVcb = TRUE;
            }

            if (Ccb) {
                RfsdFreeCcb(Ccb);

                if (FileObject) {
                    FileObject->FsContext2 = Ccb = NULL;
                }
            }
            
            Status = STATUS_SUCCESS;
            
            __leave;
        }
        
        if (Fcb->Identifier.Type != RFSDFCB || Fcb->Identifier.Size != sizeof(RFSD_FCB)) {

#if DBG
            RfsdPrint((DBG_ERROR, "RfsdClose: Strange IRP_MJ_CLOSE by system!\n"));
            ExAcquireResourceExclusiveLite(
                &RfsdGlobal->CountResource,
                TRUE );

            RfsdGlobal->IRPCloseCount++;

            ExReleaseResourceForThreadLite(
                &RfsdGlobal->CountResource,
                ExGetCurrentResourceThread() );
#endif
            __leave;
        }

        ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
            (Fcb->Identifier.Size == sizeof(RFSD_FCB)));

/*        
        if ( (!IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) && 
             (!IsFlagOn(Fcb->Flags, FCB_PAGE_FILE))  )
*/
        {
            if (!ExAcquireResourceExclusiveLite(
                &Fcb->MainResource,
                IrpContext->IsSynchronous )) {
                Status = STATUS_PENDING;
                __leave;
            }
            
            FcbResourceAcquired = TRUE;
        }
        
        if (!Ccb) {
            Status = STATUS_SUCCESS;
            __leave;
        }

        ASSERT((Ccb->Identifier.Type == RFSDCCB) &&
            (Ccb->Identifier.Size == sizeof(RFSD_CCB)));
        
        Fcb->ReferenceCount--;
        Vcb->ReferenceCount--;
        
        if (!Vcb->ReferenceCount && IsFlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING)) {
            FreeVcb = TRUE;
        }

        RfsdPrint((DBG_INFO, "RfsdClose: OpenHandleCount: %u ReferenceCount: %u %s\n",
            Fcb->OpenHandleCount, Fcb->ReferenceCount, Fcb->AnsiFileName.Buffer ));

        if (Ccb) {

            RfsdFreeCcb(Ccb);

            if (FileObject) {
                FileObject->FsContext2 = Ccb = NULL;
            }
        }

        if (!Fcb->ReferenceCount)  {
            //
            // Remove Fcb from Vcb->FcbList ...
            //

            RemoveEntryList(&Fcb->Next);

            RfsdFreeFcb(Fcb);

            FcbResourceAcquired = FALSE;
        }
        
        Status = STATUS_SUCCESS;

    } __finally {

        if (FcbResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread() );
        }

        if (VcbResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread()  );
        }
        
        if (!IrpContext->ExceptionInProgress) {
            if (Status == STATUS_PENDING) {

                RfsdQueueCloseRequest(IrpContext);
/*                

                Status = STATUS_SUCCESS;

                if (IrpContext->Irp != NULL)
                {
                    IrpContext->Irp->IoStatus.Status = Status;
                    
                    RfsdCompleteRequest(
                        IrpContext->Irp,
                        !IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_REQUEUED),
                        (CCHAR)
                        (NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT)
                        );
                    
                    IrpContext->Irp = NULL;
                }
*/
            } else {

                RfsdCompleteIrpContext(IrpContext, Status);

                if (FreeVcb) {

                    ExAcquireResourceExclusiveLite(
                        &RfsdGlobal->Resource, TRUE );

                    RfsdClearVpbFlag(Vcb->Vpb, VPB_MOUNTED);

                    RfsdRemoveVcb(Vcb);

                    ExReleaseResourceForThreadLite(
                        &RfsdGlobal->Resource,
                        ExGetCurrentResourceThread() );

                    RfsdFreeVcb(Vcb);
                }
            }
        }
    }
    
    return Status;
}

VOID
RfsdQueueCloseRequest (IN PRFSD_IRP_CONTEXT IrpContext)
{
    ASSERT(IrpContext);
    
    ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
        (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
    
    if (!IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DELAY_CLOSE)) {
        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_DELAY_CLOSE);

        IrpContext->Fcb = (PRFSD_FCB) IrpContext->FileObject->FsContext;
        IrpContext->Ccb = (PRFSD_CCB) IrpContext->FileObject->FsContext2;

        IrpContext->FileObject = NULL;
    }

    // IsSynchronous means we can block (so we don't requeue it)
    IrpContext->IsSynchronous = TRUE;
    
    ExInitializeWorkItem(
        &IrpContext->WorkQueueItem,
        RfsdDeQueueCloseRequest,
        IrpContext);
    
    ExQueueWorkItem(&IrpContext->WorkQueueItem, CriticalWorkQueue);
}

VOID
RfsdDeQueueCloseRequest (IN PVOID Context)
{
    PRFSD_IRP_CONTEXT IrpContext;
    
    IrpContext = (PRFSD_IRP_CONTEXT) Context;
    
    ASSERT(IrpContext);
    
    ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
        (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
    
    __try {

        __try {

            FsRtlEnterFileSystem();
            RfsdClose(IrpContext);

        } __except (RfsdExceptionFilter(IrpContext, GetExceptionInformation())) {

            RfsdExceptionHandler(IrpContext);
        }

    } __finally {

        FsRtlExitFileSystem();
    }
}

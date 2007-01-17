/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Rfsd File System Driver for WinNT/2K/XP
 * FILE:             flush.c
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
#pragma alloc_text(PAGE, RfsdFlushFile)
#pragma alloc_text(PAGE, RfsdFlushFiles)
#pragma alloc_text(PAGE, RfsdFlushVolume)
#pragma alloc_text(PAGE, RfsdFlush)
#endif


NTSTATUS
RfsdFlushCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt  )

{
    if (Irp->PendingReturned)
        IoMarkIrpPending( Irp );


    if (Irp->IoStatus.Status == STATUS_INVALID_DEVICE_REQUEST)
        Irp->IoStatus.Status = STATUS_SUCCESS;

    return STATUS_SUCCESS;
}

NTSTATUS
RfsdFlushFiles (IN PRFSD_VCB Vcb, BOOLEAN bShutDown)
{
    IO_STATUS_BLOCK    IoStatus;

    PRFSD_FCB       Fcb;
    PLIST_ENTRY     ListEntry;

    if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY) ||
        IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED)) {
        return STATUS_SUCCESS;
    }

    RfsdPrint((DBG_INFO, "Flushing Files ...\n"));

    // Flush all Fcbs in Vcb list queue.
    {
        for (ListEntry = Vcb->FcbList.Flink;
             ListEntry != &Vcb->FcbList;
             ListEntry = ListEntry->Flink ) {

            Fcb = CONTAINING_RECORD(ListEntry, RFSD_FCB, Next);

            if (ExAcquireResourceExclusiveLite(
                &Fcb->MainResource,
                TRUE )) {

                IoStatus.Status = RfsdFlushFile(Fcb);
/*
                if (bShutDown)
                    IoStatus.Status = RfsdPurgeFile(Fcb, TRUE);
                else
                    IoStatus.Status = RfsdFlushFile(Fcb);
*/
                ExReleaseResourceForThreadLite(
                    &Fcb->MainResource,
                    ExGetCurrentResourceThread());
            }
        }
    }

    return IoStatus.Status;
}

NTSTATUS
RfsdFlushVolume (IN PRFSD_VCB Vcb, BOOLEAN bShutDown)
{
    IO_STATUS_BLOCK    IoStatus;
   
    if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY) ||
        IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED)) {
        return STATUS_SUCCESS;
    }

    RfsdPrint((DBG_INFO, "RfsdFlushVolume: Flushing Vcb ...\n"));

    ExAcquireSharedStarveExclusive(&Vcb->PagingIoResource, TRUE);
    ExReleaseResource(&Vcb->PagingIoResource);

    CcFlushCache(&(Vcb->SectionObject), NULL, 0, &IoStatus);

    return IoStatus.Status;       
}

NTSTATUS
RfsdFlushFile (IN PRFSD_FCB Fcb)
{
    IO_STATUS_BLOCK    IoStatus;

    ASSERT(Fcb != NULL);
        
    ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
        (Fcb->Identifier.Size == sizeof(RFSD_FCB)));

    if (IsDirectory(Fcb))
        return STATUS_SUCCESS;

    RfsdPrint((DBG_INFO, "RfsdFlushFile: Flushing File Key=%x,%xh %S ...\n", 
		Fcb->RfsdMcb->Key.k_dir_id, Fcb->RfsdMcb->Key.k_objectid, Fcb->RfsdMcb->ShortName.Buffer));
/*
    {
        ULONG ResShCnt, ResExCnt; 
        ResShCnt = ExIsResourceAcquiredSharedLite(&Fcb->PagingIoResource);
        ResExCnt = ExIsResourceAcquiredExclusiveLite(&Fcb->PagingIoResource);

        RfsdPrint((DBG_INFO, "RfsdFlushFile: PagingIoRes: %xh:%xh\n", ResShCnt, ResExCnt));
    }
*/
    CcFlushCache(&(Fcb->SectionObject), NULL, 0, &IoStatus);

    ClearFlag(Fcb->Flags, FCB_FILE_MODIFIED);

    return IoStatus.Status;
}

NTSTATUS
RfsdFlush (IN PRFSD_IRP_CONTEXT IrpContext)
{
    NTSTATUS                Status;

    PIRP                    Irp;
    PIO_STACK_LOCATION      IrpSp;

    PRFSD_VCB               Vcb;
    PRFSD_FCBVCB            FcbOrVcb;
    PFILE_OBJECT            FileObject;

    PDEVICE_OBJECT          DeviceObject;

    BOOLEAN                 MainResourceAcquired = FALSE;

    __try {

        ASSERT(IrpContext);
    
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;
        
        //
        // This request is not allowed on the main device object
        //
        if (DeviceObject == RfsdGlobal->DeviceObject) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            __leave;
        }
        
        Vcb = (PRFSD_VCB) DeviceObject->DeviceExtension;
       
        ASSERT(Vcb != NULL);

        ASSERT((Vcb->Identifier.Type == RFSDVCB) &&
            (Vcb->Identifier.Size == sizeof(RFSD_VCB)));

        ASSERT(IsMounted(Vcb));

        if ( IsFlagOn(Vcb->Flags, VCB_READ_ONLY) ||
             IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED)) {
            Status =  STATUS_SUCCESS;
            __leave;
        }

        Irp = IrpContext->Irp;
    
        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        FileObject = IrpContext->FileObject;
        
        FcbOrVcb = (PRFSD_FCBVCB) FileObject->FsContext;
        
        ASSERT(FcbOrVcb != NULL);

        if (!ExAcquireResourceExclusiveLite(
                &FcbOrVcb->MainResource,
                IrpContext->IsSynchronous )) {
            Status = STATUS_PENDING;
            __leave;
        }
            
        MainResourceAcquired = TRUE;

        if (FcbOrVcb->Identifier.Type == RFSDVCB) {

            Status = RfsdFlushFiles((PRFSD_VCB)(FcbOrVcb), FALSE);

            if (NT_SUCCESS(Status)) {
                __leave;
            }

            Status = RfsdFlushVolume((PRFSD_VCB)(FcbOrVcb), FALSE);

            if (NT_SUCCESS(Status) && IsFlagOn(Vcb->StreamObj->Flags, FO_FILE_MODIFIED)) {
                ClearFlag(Vcb->StreamObj->Flags, FO_FILE_MODIFIED);
            }

        } else if (FcbOrVcb->Identifier.Type == RFSDFCB) {

            Status = RfsdFlushFile((PRFSD_FCB)(FcbOrVcb));

            if (NT_SUCCESS(Status) && IsFlagOn(FileObject->Flags, FO_FILE_MODIFIED)) {
                ClearFlag(FileObject->Flags, FO_FILE_MODIFIED);
            }
        }

    } __finally {

        if (MainResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &FcbOrVcb->MainResource,
                ExGetCurrentResourceThread() );
        }

        if (!IrpContext->ExceptionInProgress) {

            if (!IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) {

                // Call the disk driver to flush the physial media.
                NTSTATUS DriverStatus;
                PIO_STACK_LOCATION NextIrpSp;

                NextIrpSp = IoGetNextIrpStackLocation(Irp);

                *NextIrpSp = *IrpSp;

                IoSetCompletionRoutine( Irp,
                                        RfsdFlushCompletionRoutine,
                                        NULL,
                                        TRUE,
                                        TRUE,
                                        TRUE );

                DriverStatus = IoCallDriver(Vcb->TargetDeviceObject, Irp);

                Status = (DriverStatus == STATUS_INVALID_DEVICE_REQUEST) ?
                         Status : DriverStatus;

                IrpContext->Irp = Irp = NULL;
            }

            RfsdCompleteIrpContext(IrpContext, Status);
        }
    }

    return Status;
}
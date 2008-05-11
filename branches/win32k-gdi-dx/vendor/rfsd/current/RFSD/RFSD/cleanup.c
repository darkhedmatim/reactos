/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Rfsd File System Driver for WinNT/2K/XP
 * FILE:             cleanup.c
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
#pragma alloc_text(PAGE, RfsdCleanup)
#endif


NTSTATUS
RfsdCleanup (IN PRFSD_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status = STATUS_SUCCESS;
    PRFSD_VCB       Vcb;
    BOOLEAN         VcbResourceAcquired = FALSE;
    PFILE_OBJECT    FileObject;
    PRFSD_FCB       Fcb;
    BOOLEAN         FcbResourceAcquired = FALSE;
    BOOLEAN         FcbPagingIoAcquired = FALSE;
    PRFSD_CCB       Ccb;
    PIRP            Irp;

    __try {

        ASSERT(IrpContext != NULL);
        
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
        
        if (DeviceObject == RfsdGlobal->DeviceObject) {
            Status = STATUS_SUCCESS;
            __leave;
        }
        
        Vcb = (PRFSD_VCB) DeviceObject->DeviceExtension;
        
        ASSERT(Vcb != NULL);
        
        ASSERT((Vcb->Identifier.Type == RFSDVCB) &&
            (Vcb->Identifier.Size == sizeof(RFSD_VCB)));

        if (!IsFlagOn(Vcb->Flags, VCB_INITIALIZED)) {
            Status = STATUS_SUCCESS;
            __leave;
        }

        if (!ExAcquireResourceExclusiveLite(
                 &Vcb->MainResource,
                 IrpContext->IsSynchronous
                 ))  {
            Status = STATUS_PENDING;
            __leave;
        }

        VcbResourceAcquired = TRUE;
        
        FileObject = IrpContext->FileObject;
        
        Fcb = (PRFSD_FCB) FileObject->FsContext;
        
        if (!Fcb) {
            Status = STATUS_SUCCESS;
            __leave;
        }
        
        if (Fcb->Identifier.Type == RFSDVCB) {
            if (IsFlagOn(Vcb->Flags, VCB_VOLUME_LOCKED) &&
                (Vcb->LockFile == FileObject) ) {
                ClearFlag(Vcb->Flags, VCB_VOLUME_LOCKED);
                Vcb->LockFile = NULL;

                RfsdClearVpbFlag(Vcb->Vpb, VPB_LOCKED);
            }

            Vcb->OpenHandleCount--;

            if (!Vcb->OpenHandleCount) {
                IoRemoveShareAccess(FileObject, &Vcb->ShareAccess);
            }

            Status = STATUS_SUCCESS;
            __leave;
        }
        
        ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
            (Fcb->Identifier.Size == sizeof(RFSD_FCB)));

/*
        if ( !IsFlagOn(Vcb->Flags, VCB_READ_ONLY) &&
             !IsFlagOn(Fcb->Flags, FCB_PAGE_FILE))
*/
        {
            if (!ExAcquireResourceExclusiveLite(
                     &Fcb->MainResource,
                     IrpContext->IsSynchronous
                     ))
            {
                Status = STATUS_PENDING;
                __leave;
            }

            FcbResourceAcquired = TRUE;
        }
        
        Ccb = (PRFSD_CCB) FileObject->FsContext2;

        if (!Ccb) {
            Status = STATUS_SUCCESS;
            __leave;
        }

        if (IsFlagOn(FileObject->Flags, FO_CLEANUP_COMPLETE)) {
            if ( IsFlagOn(FileObject->Flags, FO_FILE_MODIFIED) &&
                 IsFlagOn(Vcb->Flags, VCB_FLOPPY_DISK) &&
                 !IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED) ) {
                Status = RfsdFlushFile(Fcb);
            }
            __leave;
        }
        
        ASSERT((Ccb->Identifier.Type == RFSDCCB) &&
            (Ccb->Identifier.Size == sizeof(RFSD_CCB)));        
        Irp = IrpContext->Irp;

        Fcb->OpenHandleCount--;

        if (!IsFlagOn(FileObject->Flags, FO_CACHE_SUPPORTED )) {
            Fcb->NonCachedOpenCount--;
        }

        Vcb->OpenFileHandleCount--;

        if (IsFlagOn(Fcb->Flags, FCB_DELETE_ON_CLOSE))  {
            SetFlag(Fcb->Flags, FCB_DELETE_PENDING);

            if (IsDirectory(Fcb)) {
                FsRtlNotifyFullChangeDirectory(
                                            Vcb->NotifySync,
                                            &Vcb->NotifyList,
                                            Fcb,
                                            NULL,
                                            FALSE,
                                            FALSE,
                                            0,
                                            NULL,
                                            NULL,
                                            NULL );
            }
        }

        if (IsDirectory(Fcb)) {

            FsRtlNotifyCleanup(
                Vcb->NotifySync,
                &Vcb->NotifyList,
                Ccb   );

        } else {

            //
            // Drop any byte range locks this process may have on the file.
            //

            FsRtlFastUnlockAll(
                &Fcb->FileLockAnchor,
                FileObject,
                IoGetRequestorProcess(Irp),
                NULL  );

            //
            // If there are no byte range locks owned by other processes on the
            // file the fast I/O read/write functions doesn't have to check for
            // locks so we set IsFastIoPossible to FastIoIsPossible again.
            //
            if (!FsRtlGetNextFileLock(&Fcb->FileLockAnchor, TRUE)) {
                if (Fcb->Header.IsFastIoPossible != FastIoIsPossible) {
                    RfsdPrint((
                        DBG_INFO, ": %-16.16s %-31s %s\n",
                        RfsdGetCurrentProcessName(),
                        "FastIoIsPossible",
                        Fcb->AnsiFileName.Buffer
                        ));

                    Fcb->Header.IsFastIoPossible = FastIoIsPossible;
                }
            }
        }

        if ( IsFlagOn( FileObject->Flags, FO_CACHE_SUPPORTED) &&
             (Fcb->NonCachedOpenCount != 0) &&
             (Fcb->NonCachedOpenCount == Fcb->ReferenceCount) &&
             (Fcb->SectionObject.DataSectionObject != NULL)) {

            if( !IsFlagOn(Vcb->Flags, VCB_READ_ONLY) &&
                !IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED)) {
                CcFlushCache(&Fcb->SectionObject, NULL, 0, NULL);
            }

            ExAcquireResourceExclusive(&(Fcb->PagingIoResource), TRUE);
            ExReleaseResource(&(Fcb->PagingIoResource));

            CcPurgeCacheSection( &Fcb->SectionObject,
                                 NULL,
                                 0,
                                 FALSE );
        }

        if (Fcb->OpenHandleCount == 0) {

            if (IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING)) {
                //
                //  Have to delete this file...
                //

                if (!ExAcquireResourceExclusiveLite(
                         &Fcb->PagingIoResource,
                         IrpContext->IsSynchronous
                         )) {
                    Status = STATUS_PENDING;
                    __leave;
                }

                FcbPagingIoAcquired = TRUE;
DbgBreak();
#if DISABLED
                Status = RfsdDeleteFile(IrpContext, Vcb, Fcb);

                if (NT_SUCCESS(Status)) {
                    if (IsDirectory(Fcb)) {
                        RfsdNotifyReportChange( IrpContext, Vcb, Fcb,
                                                FILE_NOTIFY_CHANGE_DIR_NAME,
                                                FILE_ACTION_REMOVED );
                    } else {
                        RfsdNotifyReportChange( IrpContext, Vcb, Fcb,
                                                FILE_NOTIFY_CHANGE_FILE_NAME,
                                                FILE_ACTION_REMOVED );
                    }
                }
#endif
                if (CcIsFileCached(FileObject)) {

                    CcSetFileSizes(FileObject, 
                            (PCC_FILE_SIZES)(&(Fcb->Header.AllocationSize)));
                    SetFlag(FileObject->Flags, FO_FILE_MODIFIED);
                }

                if (FcbPagingIoAcquired) {
                    ExReleaseResourceForThreadLite(
                        &Fcb->PagingIoResource,
                        ExGetCurrentResourceThread() );

                    FcbPagingIoAcquired = FALSE;
                }
            }
        }

        if (!IsDirectory(Fcb) && FileObject->PrivateCacheMap) {
            RfsdPrint((DBG_INFO, "RfsdCleanup: CcUninitializeCacheMap is called for %s.\n",
                                  Fcb->AnsiFileName.Buffer ));

            CcUninitializeCacheMap(
                    FileObject,
                    (PLARGE_INTEGER)(&(Fcb->Header.FileSize)),
                    NULL );
        }

        if (!Fcb->OpenHandleCount) {
            IoRemoveShareAccess(FileObject, &Fcb->ShareAccess);
        }

        RfsdPrint((DBG_INFO, "RfsdCleanup: OpenCount: %u ReferCount: %u %s\n",
            Fcb->OpenHandleCount, Fcb->ReferenceCount, Fcb->AnsiFileName.Buffer ));

        Status = STATUS_SUCCESS;

        if (FileObject) {
            SetFlag(FileObject->Flags, FO_CLEANUP_COMPLETE);
        }

    } __finally {
       
        if (FcbPagingIoAcquired) {
            ExReleaseResourceForThreadLite(
                &Fcb->PagingIoResource,
                ExGetCurrentResourceThread() );
        }

        if (FcbResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread() );
        }
        
        if (VcbResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread());
        }
        
        if (!IrpContext->ExceptionInProgress) {
            if (Status == STATUS_PENDING) {
                RfsdQueueRequest(IrpContext);
            } else {
                IrpContext->Irp->IoStatus.Status = Status;
                RfsdCompleteIrpContext(IrpContext, Status);
            }
        }
    }
    
    return Status;
}

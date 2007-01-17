/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Rfsd File System Driver for WinNT/2K/XP
 * FILE:             shutdown.c
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
#pragma alloc_text(PAGE, RfsdShutDown)
#endif

NTSTATUS
RfsdShutDown (IN PRFSD_IRP_CONTEXT IrpContext)
{
    NTSTATUS                Status;

    PKEVENT                 Event;

    PIRP                    Irp;
    PIO_STACK_LOCATION      IrpSp;

    PRFSD_VCB               Vcb;
    PLIST_ENTRY             ListEntry;

    BOOLEAN                 GlobalResourceAcquired = FALSE;

    __try {

        ASSERT(IrpContext);
    
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));

        Status = STATUS_SUCCESS;

        Irp = IrpContext->Irp;
    
        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        if (!ExAcquireResourceExclusiveLite(
                &RfsdGlobal->Resource,
                IrpContext->IsSynchronous )) {
            Status = STATUS_PENDING;
            __leave;
        }
            
        GlobalResourceAcquired = TRUE;

        Event = ExAllocatePool( NonPagedPool, sizeof(KEVENT));
        KeInitializeEvent(Event, NotificationEvent, FALSE );

        for (ListEntry = RfsdGlobal->VcbList.Flink;
             ListEntry != &(RfsdGlobal->VcbList);
             ListEntry = ListEntry->Flink ) {

            Vcb = CONTAINING_RECORD(ListEntry, RFSD_VCB, Next);

            if (ExAcquireResourceExclusiveLite(
                &Vcb->MainResource,
                TRUE )) {

                Status = RfsdFlushFiles(Vcb, TRUE);
                if(!NT_SUCCESS(Status)) {
                    DbgBreak();
                }

                Status = RfsdFlushVolume(Vcb, TRUE);

                if(!NT_SUCCESS(Status)) {
                    DbgBreak();
                }

                RfsdDiskShutDown(Vcb);

                ExReleaseResourceForThreadLite(
                    &Vcb->MainResource,
                    ExGetCurrentResourceThread());
            }
        }

/*
        IoUnregisterFileSystem(RfsdGlobal->DeviceObject);
*/
    } __finally {

        if (GlobalResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &RfsdGlobal->Resource,
                ExGetCurrentResourceThread() );
        }

        if (!IrpContext->ExceptionInProgress) {
            if (Status == STATUS_PENDING) {
                RfsdQueueRequest(IrpContext);
            } else {
                RfsdCompleteIrpContext(IrpContext, Status);
            }
        }
    }

    return Status;
}
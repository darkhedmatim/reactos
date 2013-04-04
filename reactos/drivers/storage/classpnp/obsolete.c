/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    obsolete.c

Abstract:

    THESE ARE EXPORTED CLASSPNP FUNCTIONS (and their subroutines)
    WHICH ARE NOW OBSOLETE.
    BUT WE NEED TO KEEP THEM AROUND FOR LEGACY REASONS.

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#include "classp.h"
#include "debug.h"

PIRP NTAPI ClassRemoveCScanList(IN PCSCAN_LIST List);
VOID NTAPI ClasspInitializeCScanList(IN PCSCAN_LIST List);

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, ClassDeleteSrbLookasideList)
    #pragma alloc_text(PAGE, ClassInitializeSrbLookasideList)
#endif

typedef struct _CSCAN_LIST_ENTRY {
    LIST_ENTRY Entry;
    ULONGLONG BlockNumber;
} CSCAN_LIST_ENTRY, *PCSCAN_LIST_ENTRY;

/*
 *  ClassSplitRequest
 *
 *      This is a legacy exported function.
 *      It is called by storage miniport driver that have their own
 *      StartIo routine when the transfer size is too large for the hardware.
 *      We map it to our new read/write handler.
 */
VOID NTAPI ClassSplitRequest(IN PDEVICE_OBJECT Fdo, IN PIRP Irp, IN ULONG MaximumBytes)
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExt->PrivateFdoData;

    if (MaximumBytes > fdoData->HwMaxXferLen) {
        DBGERR(("ClassSplitRequest - driver requesting split to size that "
                "hardware is unable to handle!\n"));
    }

    if (MaximumBytes < fdoData->HwMaxXferLen){
        DBGWARN(("ClassSplitRequest - driver requesting smaller HwMaxXferLen "
                 "than required"));
        fdoData->HwMaxXferLen = MAX(MaximumBytes, PAGE_SIZE);
    }

    ServiceTransferRequest(Fdo, Irp);
} 

/*++////////////////////////////////////////////////////////////////////////////

ClassIoCompleteAssociated()

Routine Description:

    This routine executes when the port driver has completed a request.
    It looks at the SRB status in the completing SRB and if not success
    it checks for valid request sense buffer information. If valid, the
    info is used to update status with more precise message of type of
    error. This routine deallocates the SRB.  This routine is used for
    requests which were build by split request.  After it has processed
    the request it decrements the Irp count in the master Irp.  If the
    count goes to zero then the master Irp is completed.

Arguments:

    Fdo - Supplies the functional device object which represents the target.

    Irp - Supplies the Irp which has completed.

    Context - Supplies a pointer to the SRB.

Return Value:

    NT status

--*/
NTSTATUS
NTAPI
ClassIoCompleteAssociated(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb = Context;

    PIRP originalIrp = Irp->AssociatedIrp.MasterIrp;
    LONG irpCount;

    NTSTATUS status;
    BOOLEAN retry;

    DBGWARN(("ClassIoCompleteAssociated is OBSOLETE !"));

    //
    // Check SRB status for success of completing request.
    //

    if (SRB_STATUS(srb->SrbStatus) != SRB_STATUS_SUCCESS) {

        ULONG retryInterval;

        DebugPrint((2,"ClassIoCompleteAssociated: IRP %p, SRB %p", Irp, srb));

        //
        // Release the queue if it is frozen.
        //

        if (srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) {
            ClassReleaseQueue(Fdo);
        }

        retry = ClassInterpretSenseInfo(
                    Fdo,
                    srb,
                    irpStack->MajorFunction,
                    irpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL ?
                        irpStack->Parameters.DeviceIoControl.IoControlCode :
                        0,
                    MAXIMUM_RETRIES -
                        ((ULONG)(ULONG_PTR)irpStack->Parameters.Others.Argument4),
                    &status,
                    &retryInterval);

        //
        // If the status is verified required and the this request
        // should bypass verify required then retry the request.
        //

        if (irpStack->Flags & SL_OVERRIDE_VERIFY_VOLUME &&
            status == STATUS_VERIFY_REQUIRED) {

            status = STATUS_IO_DEVICE_ERROR;
            retry = TRUE;
        }

        if (retry && ((*(PCHAR*)&irpStack->Parameters.Others.Argument4)--)) {

            //
            // Retry request. If the class driver has supplied a StartIo,
            // call it directly for retries.
            //

            DebugPrint((1, "Retry request %p\n", Irp));

            if (PORT_ALLOCATED_SENSE(fdoExtension, srb)) {
                FREE_PORT_ALLOCATED_SENSE_BUFFER(fdoExtension, srb);
            }

            RetryRequest(Fdo, Irp, srb, TRUE, retryInterval);

            return STATUS_MORE_PROCESSING_REQUIRED;
        }

    } else {

        //
        // Set status for successful request.
        //

        status = STATUS_SUCCESS;

    } // end if (SRB_STATUS(srb->SrbStatus) ...

    //
    // Return SRB to list.
    //

    if (PORT_ALLOCATED_SENSE(fdoExtension, srb)) {
        FREE_PORT_ALLOCATED_SENSE_BUFFER(fdoExtension, srb);
    }

    ClassFreeOrReuseSrb(fdoExtension, srb);

    //
    // Set status in completing IRP.
    //

    Irp->IoStatus.Status = status;

    DebugPrint((2, "ClassIoCompleteAssociated: Partial xfer IRP %p\n", Irp));

    //
    // Get next stack location. This original request is unused
    // except to keep track of the completing partial IRPs so the
    // stack location is valid.
    //

    irpStack = IoGetNextIrpStackLocation(originalIrp);

    //
    // Update status only if error so that if any partial transfer
    // completes with error, then the original IRP will return with
    // error. If any of the asynchronous partial transfer IRPs fail,
    // with an error then the original IRP will return 0 bytes transfered.
    // This is an optimization for successful transfers.
    //

    if (!NT_SUCCESS(status)) {

        originalIrp->IoStatus.Status = status;
        originalIrp->IoStatus.Information = 0;

        //
        // Set the hard error if necessary.
        //

        if (IoIsErrorUserInduced(status)) {

            //
            // Store DeviceObject for filesystem.
            //

            IoSetHardErrorOrVerifyDevice(originalIrp, Fdo);
        }
    }

    //
    // Decrement and get the count of remaining IRPs.
    //

    irpCount = InterlockedDecrement(
                    (PLONG)&irpStack->Parameters.Others.Argument1);

    DebugPrint((2, "ClassIoCompleteAssociated: Partial IRPs left %d\n",
                irpCount));

    //
    // Ensure that the irpCount doesn't go negative.  This was happening once
    // because classpnp would get confused if it ran out of resources when
    // splitting the request.
    //

    ASSERT(irpCount >= 0);

    if (irpCount == 0) {

        //
        // All partial IRPs have completed.
        //

        DebugPrint((2,
                 "ClassIoCompleteAssociated: All partial IRPs complete %p\n",
                 originalIrp));

        if (fdoExtension->CommonExtension.DriverExtension->InitData.ClassStartIo) {

            //
            // Acquire a separate copy of the remove lock so the debugging code
            // works okay and we don't have to hold up the completion of this
            // irp until after we start the next packet(s).
            //

            KIRQL oldIrql;
            UCHAR uniqueAddress;
            ClassAcquireRemoveLock(Fdo, (PIRP)&uniqueAddress);
            ClassReleaseRemoveLock(Fdo, originalIrp);
            ClassCompleteRequest(Fdo, originalIrp, IO_DISK_INCREMENT);

            KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
            IoStartNextPacket(Fdo, FALSE);
            KeLowerIrql(oldIrql);

            ClassReleaseRemoveLock(Fdo, (PIRP)&uniqueAddress);

        } else {

            //
            // just complete this request
            //

            ClassReleaseRemoveLock(Fdo, originalIrp);
            ClassCompleteRequest(Fdo, originalIrp, IO_DISK_INCREMENT);

        }

    }

    //
    // Deallocate IRP and indicate the I/O system should not attempt any more
    // processing.
    //

    IoFreeIrp(Irp);
    return STATUS_MORE_PROCESSING_REQUIRED;

} // end ClassIoCompleteAssociated()

/*++////////////////////////////////////////////////////////////////////////////

RetryRequest()

Routine Description:

    This is a wrapper around the delayed retry DPC routine, RetryRequestDPC.
    This reinitializes the necessary fields, queues the request, and sets
    a timer to call the DPC if someone hasn't already done so.

Arguments:

    DeviceObject - Supplies the device object associated with this request.

    Irp - Supplies the request to be retried.

    Srb - Supplies a Pointer to the SCSI request block to be retied.

    Assocaiated - Indicates this is an assocatied Irp created by split request.

    RetryInterval - How long, in seconds, before retrying the request.

Return Value:

    None

--*/
VOID
NTAPI
RetryRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PSCSI_REQUEST_BLOCK Srb,
    BOOLEAN Associated,
    ULONG RetryInterval
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION nextIrpStack = IoGetNextIrpStackLocation(Irp);
    ULONG transferByteCount;

    // This function is obsolete but is still used by some of our class drivers.
    // DBGWARN(("RetryRequest is OBSOLETE !"));

    //
    // Determine the transfer count of the request.  If this is a read or a
    // write then the transfer count is in the Irp stack.  Otherwise assume
    // the MDL contains the correct length.  If there is no MDL then the
    // transfer length must be zero.
    //

    if (currentIrpStack->MajorFunction == IRP_MJ_READ ||
        currentIrpStack->MajorFunction == IRP_MJ_WRITE) {

        transferByteCount = currentIrpStack->Parameters.Read.Length;

    } else if (Irp->MdlAddress != NULL) {

        //
        // Note this assumes that only read and write requests are spilt and
        // other request do not need to be.  If the data buffer address in
        // the MDL and the SRB don't match then transfer length is most
        // likely incorrect.
        //

        ASSERT(Srb->DataBuffer == MmGetMdlVirtualAddress(Irp->MdlAddress));
        transferByteCount = Irp->MdlAddress->ByteCount;

    } else {

        transferByteCount = 0;
    }

    //
    // this is a safety net.  this should not normally be hit, since we are
    // not guaranteed to be an fdoExtension
    //

    ASSERT(!TEST_FLAG(Srb->SrbFlags, SRB_FLAGS_FREE_SENSE_BUFFER));

    //
    // Reset byte count of transfer in SRB Extension.
    //

    Srb->DataTransferLength = transferByteCount;

    //
    // Zero SRB statuses.
    //

    Srb->SrbStatus = Srb->ScsiStatus = 0;

    //
    // Set the no disconnect flag, disable synchronous data transfers and
    // disable tagged queuing. This fixes some errors.
    // NOTE: Cannot clear these flags, just add to them
    //

    SET_FLAG(Srb->SrbFlags, SRB_FLAGS_DISABLE_DISCONNECT);
    SET_FLAG(Srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
    CLEAR_FLAG(Srb->SrbFlags, SRB_FLAGS_QUEUE_ACTION_ENABLE);

    Srb->QueueTag = SP_UNTAGGED;

    //
    // Set up major SCSI function.
    //

    nextIrpStack->MajorFunction = IRP_MJ_SCSI;

    //
    // Save SRB address in next stack for port driver.
    //

    nextIrpStack->Parameters.Scsi.Srb = Srb;


    IoSetCompletionRoutine(Irp, ClassIoComplete, Srb, TRUE, TRUE, TRUE);

    {
        LARGE_INTEGER retry100ns;
        retry100ns.QuadPart = RetryInterval;  // seconds
        retry100ns.QuadPart *= (LONGLONG)1000 * 1000 * 10;

        ClassRetryRequest(DeviceObject, Irp, retry100ns);
    }
    return;
} // end RetryRequest()

/*++

ClassBuildRequest()

Routine Description:

    This routine allocates an SRB for the specified request then calls
    ClasspBuildRequestEx to create a SCSI operation to read or write the device.

    If no SRB is available then the request will be queued to be issued later
    when requests are available.  Drivers which do not want the queueing
    behavior should allocate the SRB themselves and call ClasspBuildRequestEx
    to issue it.

Arguments:

    Fdo - Supplies the functional device object associated with this request.

    Irp - Supplies the request to be retried.

Note:

    If the IRP is for a disk transfer, the byteoffset field
    will already have been adjusted to make it relative to
    the beginning of the disk.


Return Value:

    NT Status

--*/
NTSTATUS
NTAPI
ClassBuildRequest(
    PDEVICE_OBJECT Fdo,
    PIRP Irp
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;

    PSCSI_REQUEST_BLOCK srb;

    // This function is obsolete, but still called by CDROM.SYS .
    // DBGWARN(("ClassBuildRequest is OBSOLETE !"));

    //
    // Allocate an Srb.
    //

    srb = ClasspAllocateSrb(fdoExtension);

    if(srb == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ClasspBuildRequestEx(fdoExtension, Irp, srb);
    return STATUS_SUCCESS;

} // end ClassBuildRequest()

VOID
NTAPI
ClasspBuildRequestEx(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

ClasspBuildRequestEx()

Routine Description:

    This routine allocates and builds an Srb for a read or write request.
    The block address and length are supplied by the Irp. The retry count
    is stored in the current stack for use by ClassIoComplete which
    processes these requests when they complete.  The Irp is ready to be
    passed to the port driver when this routine returns.

Arguments:

    FdoExtension - Supplies the device extension associated with this request.

    Irp - Supplies the request to be issued.

    Srb - Supplies an SRB to be used for the request.

Note:

    If the IRP is for a disk transfer, the byteoffset field
    will already have been adjusted to make it relative to
    the beginning of the disk.


Return Value:

    NT Status

--*/
{
    PIO_STACK_LOCATION  currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION  nextIrpStack = IoGetNextIrpStackLocation(Irp);

    LARGE_INTEGER       startingOffset = currentIrpStack->Parameters.Read.ByteOffset;

    PCDB                cdb;
    ULONG               logicalBlockAddress;
    USHORT              transferBlocks;

    // This function is obsolete, but still called by CDROM.SYS .
    // DBGWARN(("ClasspBuildRequestEx is OBSOLETE !"));

    //
    // Prepare the SRB.
    //

    RtlZeroMemory(Srb, sizeof(SCSI_REQUEST_BLOCK));

    //
    // Calculate relative sector address.
    //

    logicalBlockAddress =
        (ULONG)(Int64ShrlMod32(startingOffset.QuadPart,
                               FdoExtension->SectorShift));

    //
    // Write length to SRB.
    //

    Srb->Length = sizeof(SCSI_REQUEST_BLOCK);

    //
    // Set up IRP Address.
    //

    Srb->OriginalRequest = Irp;

    //
    // Set up target ID and logical unit number.
    //

    Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    Srb->DataBuffer = MmGetMdlVirtualAddress(Irp->MdlAddress);

    //
    // Save byte count of transfer in SRB Extension.
    //

    Srb->DataTransferLength = currentIrpStack->Parameters.Read.Length;

    //
    // Initialize the queue actions field.
    //

    Srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;

    //
    // Queue sort key is Relative Block Address.
    //

    Srb->QueueSortKey = logicalBlockAddress;

    //
    // Indicate auto request sense by specifying buffer and size.
    //

    Srb->SenseInfoBuffer = FdoExtension->SenseData;
    Srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;

    //
    // Set timeout value of one unit per 64k bytes of data.
    //

    Srb->TimeOutValue = ((Srb->DataTransferLength + 0xFFFF) >> 16) *
                        FdoExtension->TimeOutValue;

    //
    // Zero statuses.
    //

    Srb->SrbStatus = Srb->ScsiStatus = 0;
    Srb->NextSrb = 0;

    //
    // Indicate that 10-byte CDB's will be used.
    //

    Srb->CdbLength = 10;

    //
    // Fill in CDB fields.
    //

    cdb = (PCDB)Srb->Cdb;

    transferBlocks = (USHORT)(currentIrpStack->Parameters.Read.Length >>
                              FdoExtension->SectorShift);

    //
    // Move little endian values into CDB in big endian format.
    //

    cdb->CDB10.LogicalBlockByte0 = ((PFOUR_BYTE)&logicalBlockAddress)->Byte3;
    cdb->CDB10.LogicalBlockByte1 = ((PFOUR_BYTE)&logicalBlockAddress)->Byte2;
    cdb->CDB10.LogicalBlockByte2 = ((PFOUR_BYTE)&logicalBlockAddress)->Byte1;
    cdb->CDB10.LogicalBlockByte3 = ((PFOUR_BYTE)&logicalBlockAddress)->Byte0;

    cdb->CDB10.TransferBlocksMsb = ((PFOUR_BYTE)&transferBlocks)->Byte1;
    cdb->CDB10.TransferBlocksLsb = ((PFOUR_BYTE)&transferBlocks)->Byte0;

    //
    // Set transfer direction flag and Cdb command.
    //

    if (currentIrpStack->MajorFunction == IRP_MJ_READ) {

        DebugPrint((3, "ClassBuildRequest: Read Command\n"));

        SET_FLAG(Srb->SrbFlags, SRB_FLAGS_DATA_IN);
        cdb->CDB10.OperationCode = SCSIOP_READ;

    } else {

        DebugPrint((3, "ClassBuildRequest: Write Command\n"));

        SET_FLAG(Srb->SrbFlags, SRB_FLAGS_DATA_OUT);
        cdb->CDB10.OperationCode = SCSIOP_WRITE;
    }

    //
    // If this is not a write-through request, then allow caching.
    //

    if (!(currentIrpStack->Flags & SL_WRITE_THROUGH)) {

        SET_FLAG(Srb->SrbFlags, SRB_FLAGS_ADAPTER_CACHE_ENABLE);

    } else {

        //
        // If write caching is enable then force media access in the
        // cdb.
        //

        if (FdoExtension->DeviceFlags & DEV_WRITE_CACHE) {
            cdb->CDB10.ForceUnitAccess = TRUE;
        }
    }

    if(TEST_FLAG(Irp->Flags, (IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO))) {
        SET_FLAG(Srb->SrbFlags, SRB_CLASS_FLAGS_PAGING);
    }

    //
    // OR in the default flags from the device object.
    //

    SET_FLAG(Srb->SrbFlags, FdoExtension->SrbFlags);

    //
    // Set up major SCSI function.
    //

    nextIrpStack->MajorFunction = IRP_MJ_SCSI;

    //
    // Save SRB address in next stack for port driver.
    //

    nextIrpStack->Parameters.Scsi.Srb = Srb;

    //
    // Save retry count in current IRP stack.
    //

    currentIrpStack->Parameters.Others.Argument4 = (PVOID)MAXIMUM_RETRIES;

    //
    // Set up IoCompletion routine address.
    //

    IoSetCompletionRoutine(Irp, ClassIoComplete, Srb, TRUE, TRUE, TRUE);

}

VOID NTAPI ClasspInsertCScanList(IN PLIST_ENTRY ListHead, IN PCSCAN_LIST_ENTRY Entry)
{
    PCSCAN_LIST_ENTRY t;

    DBGWARN(("ClasspInsertCScanList is OBSOLETE !"));

    //
    // Iterate through the list.  Insert this entry in the sorted list in 
    // order (after other requests for the same block).  At each stop if 
    // blockNumber(Entry) >= blockNumber(t) then move on.
    //

    for(t = (PCSCAN_LIST_ENTRY) ListHead->Flink; 
        t != (PCSCAN_LIST_ENTRY) ListHead; 
        t = (PCSCAN_LIST_ENTRY) t->Entry.Flink) {

        if(Entry->BlockNumber < t->BlockNumber) {

            //
            // Set the pointers in entry to the right location.
            //

            Entry->Entry.Flink = &(t->Entry);
            Entry->Entry.Blink = t->Entry.Blink;

            //
            // Set the pointers in the surrounding elements to refer to us.
            //

            t->Entry.Blink->Flink = &(Entry->Entry);
            t->Entry.Blink = &(Entry->Entry);
            return;
        }
    }

    //
    // Insert this entry at the tail of the list.  If the list was empty this 
    // will also be the head of the list.
    //

    InsertTailList(ListHead, &(Entry->Entry));

}

VOID NTAPI ClassInsertCScanList(IN PCSCAN_LIST List, IN PIRP Irp, IN ULONGLONG BlockNumber, IN BOOLEAN LowPriority)
/*++

Routine Description:

    This routine inserts an entry into the CScan list based on it's block number
    and priority.  It is assumed that the caller is providing synchronization
    to the access of the list.

    Low priority requests are always scheduled to run on the next sweep across
    the disk.  Normal priority requests will be inserted into the current or 
    next sweep based on the standard C-SCAN algorithm.

Arguments:

    List - the list to insert into
    
    Irp - the irp to be inserted.
    
    BlockNumber - the block number for this request.
    
    LowPriority - indicates that the request is lower priority and should be 
                  done on the next sweep across the disk.    

Return Value:

    none
    
--*/                      
{
    PCSCAN_LIST_ENTRY entry = (PCSCAN_LIST_ENTRY)Irp->Tail.Overlay.DriverContext;

    DBGWARN(("ClassInsertCScanList is OBSOLETE !"));

    //
    // Set the block number in the entry.  We need this to keep the list sorted.
    //
    entry->BlockNumber = BlockNumber;

    //
    // If it's a normal priority request and further down the disk than our 
    // current position then insert this entry into the current sweep.
    //

    if((LowPriority != TRUE) && (BlockNumber > List->BlockNumber)) {
        ClasspInsertCScanList(&(List->CurrentSweep), entry);
    } else {
        ClasspInsertCScanList(&(List->NextSweep), entry);
    }
    return;
}

VOID NTAPI ClassFreeOrReuseSrb(IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
                               IN PSCSI_REQUEST_BLOCK Srb)
/*++

Routine Description:

    This routine will attempt to reuse the provided SRB to start a blocked 
    read/write request.  
    If there is no need to reuse the request it will be returned
    to the SRB lookaside list.

Arguments:

    Fdo - the device extension
    
    Srb - the SRB which is to be reused or freed.
    
Return Value:

    none.
    
--*/            

{
    PCLASS_PRIVATE_FDO_DATA privateData = FdoExtension->PrivateFdoData;
    PCOMMON_DEVICE_EXTENSION commonExt = &FdoExtension->CommonExtension;
    //KIRQL oldIrql;
    //PIRP blockedIrp;

    // This function is obsolete, but still called by DISK.SYS .
    // DBGWARN(("ClassFreeOrReuseSrb is OBSOLETE !"));

    //
    // safety net.  this should never occur.  if it does, it's a potential
    // memory leak.
    //
    ASSERT(!TEST_FLAG(Srb->SrbFlags, SRB_FLAGS_FREE_SENSE_BUFFER));
   
    if (commonExt->IsSrbLookasideListInitialized){
        /*
         *  Put the SRB back in our lookaside list.
         *
         *  BUGBUG - Some class drivers use ClassIoComplete
         *            to complete SRBs that they themselves allocated.
         *            So we may be putting a "foreign" SRB
         *            (e.g. with a different pool tag) into our lookaside list.
         */
        ClasspFreeSrb(FdoExtension, Srb);
    }
    else {
        DBGERR(("ClassFreeOrReuseSrb: someone is trying to use an uninitialized SrbLookasideList !!!"));;
        ExFreePool(Srb);
    }
}

/*++////////////////////////////////////////////////////////////////////////////

ClassDeleteSrbLookasideList()

Routine Description:

    This routine deletes a lookaside listhead for srbs, and should be called
    only during the final removal.
    
    If called at other times, the caller is responsible for
    synchronization and removal issues.

Arguments:

    CommonExtension - Pointer to the CommonExtension containing the listhead.

Return Value:

    None

--*/
VOID NTAPI ClassDeleteSrbLookasideList(IN PCOMMON_DEVICE_EXTENSION CommonExtension)
{
    PAGED_CODE();

    // This function is obsolete, but is still called by some of our code.
    // DBGWARN(("ClassDeleteSrbLookasideList is OBSOLETE !"));

    if (CommonExtension->IsSrbLookasideListInitialized){
        CommonExtension->IsSrbLookasideListInitialized = FALSE;
        ExDeleteNPagedLookasideList(&CommonExtension->SrbLookasideList);
    }
    else {
        DBGWARN(("ClassDeleteSrbLookasideList: attempt to delete uninitialized or freed srblookasidelist"));
    }
} 

/*++////////////////////////////////////////////////////////////////////////////

ClassInitializeSrbLookasideList()

Routine Description:

    This routine sets up a lookaside listhead for srbs, and should be called
    only from the ClassInitDevice() routine to prevent race conditions.
    
    If called from other locations, the caller is responsible for
    synchronization and removal issues.

Arguments:

    CommonExtension - Pointer to the CommonExtension containing the listhead.

    NumberElements  - Supplies the maximum depth of the lookaside list.

    
Note:

    The Windows 2000 version of classpnp did not return any status value from
    this call.

--*/

VOID NTAPI ClassInitializeSrbLookasideList(IN PCOMMON_DEVICE_EXTENSION CommonExtension,
                                           IN ULONG NumberElements)
{
    PAGED_CODE();

    // This function is obsolete, but still called by DISK.SYS .
    // DBGWARN(("ClassInitializeSrbLookasideList is OBSOLETE !"));

    ASSERT(!CommonExtension->IsSrbLookasideListInitialized);
    if (!CommonExtension->IsSrbLookasideListInitialized){

        ExInitializeNPagedLookasideList(&CommonExtension->SrbLookasideList,
                                        NULL,
                                        NULL,
                                        NonPagedPool,
                                        sizeof(SCSI_REQUEST_BLOCK),
                                        '$scS',
                                        (USHORT)NumberElements);

        CommonExtension->IsSrbLookasideListInitialized = TRUE;
    }
    
} 

VOID NTAPI ClasspInitializeCScanList(IN PCSCAN_LIST List)
{
    PAGED_CODE();
    RtlZeroMemory(List, sizeof(CSCAN_LIST));
    InitializeListHead(&(List->CurrentSweep));
    InitializeListHead(&(List->NextSweep));
}

VOID NTAPI ClasspStartNextSweep(PCSCAN_LIST List)
{
    ASSERT(IsListEmpty(&(List->CurrentSweep)) == TRUE);

    //
    // If the next sweep is empty then there's nothing to do.
    //

    if(IsListEmpty(&(List->NextSweep))) {
        return;
    }

    //
    // Copy the next sweep list head into the current sweep list head.
    //

    List->CurrentSweep = List->NextSweep;

    //
    // Unlink the next sweep list from the list head now that we have a copy 
    // of it.
    //

    InitializeListHead(&(List->NextSweep));

    //
    // Update the next sweep list to point back to the current sweep list head.
    //

    List->CurrentSweep.Flink->Blink = &(List->CurrentSweep);
    List->CurrentSweep.Blink->Flink = &(List->CurrentSweep);

    return;
}

PIRP NTAPI ClassRemoveCScanList(IN PCSCAN_LIST List)
{
    PCSCAN_LIST_ENTRY entry;

    //
    // If the current sweep is empty then promote the next sweep.
    //

    if(IsListEmpty(&(List->CurrentSweep))) {
        ClasspStartNextSweep(List);
    }

    //
    // If the current sweep is still empty then we're done.
    //

    if(IsListEmpty(&(List->CurrentSweep))) {
        return NULL;
    }

    //
    // Remove the head entry from the current sweep.  Record it's block number
    // so that nothing before it on the disk gets into the current sweep.
    //

    entry = (PCSCAN_LIST_ENTRY) RemoveHeadList(&(List->CurrentSweep));

    List->BlockNumber = entry->BlockNumber;

    return CONTAINING_RECORD(entry, IRP, Tail.Overlay.DriverContext);
}

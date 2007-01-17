/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Rfsd File System Driver for WinNT/2K/XP
 * FILE:             read.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://rfsd.yeah.net
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "rfsdfs.h"

/* GLOBALS ***************************************************************/

extern PRFSD_GLOBAL RfsdGlobal;

/* DEFINITIONS *************************************************************/

NTSTATUS
RfsdReadComplete (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdReadFile (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdReadVolume (IN PRFSD_IRP_CONTEXT IrpContext);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RfsdCompleteIrpContext)
#pragma alloc_text(PAGE, RfsdCopyRead)
#pragma alloc_text(PAGE, RfsdRead)
#pragma alloc_text(PAGE, RfsdReadVolume)
#pragma alloc_text(PAGE, RfsdReadInode)
#pragma alloc_text(PAGE, RfsdReadFile)
#pragma alloc_text(PAGE, RfsdReadComplete)

#endif

/* FUNCTIONS *************************************************************/


/** Proxy to CcCopyRead, which simply asserts the success of the IoStatus. */
BOOLEAN 
RfsdCopyRead(
    IN PFILE_OBJECT  FileObject,
    IN PLARGE_INTEGER  FileOffset,
    IN ULONG  Length,
    IN BOOLEAN  Wait,
    OUT PVOID  Buffer,
    OUT PIO_STATUS_BLOCK  IoStatus
    )
{
    BOOLEAN bRet;
    bRet=  CcCopyRead(FileObject,
                FileOffset,
                Length,
                Wait,
                Buffer,
                IoStatus    );

    if (bRet) {
        ASSERT(NT_SUCCESS(IoStatus->Status));
    }

    return bRet;
/*
    PVOID Bcb = NULL;
    PVOID Buf = NULL;

    if (CcMapData(  FileObject,
                    FileOffset,
                    Length,
                    Wait,
                    &Bcb,
                    &Buf    )) {
        RtlCopyMemory(Buffer,  Buf, Length);
        IoStatus->Status = STATUS_SUCCESS;
        IoStatus->Information = Length;
        CcUnpinData(Bcb);
        return TRUE;

    } else {
        // IoStatus->Status = STATUS_
        return FALSE;
    }
*/
}

NTSTATUS
RfsdReadVolume (IN PRFSD_IRP_CONTEXT IrpContext)
{
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;

    PRFSD_VCB           Vcb;
    PRFSD_CCB           Ccb;
    PRFSD_FCBVCB        FcbOrVcb;
    PFILE_OBJECT        FileObject;

    PDEVICE_OBJECT      DeviceObject;

    PIRP                Irp;
    PIO_STACK_LOCATION  IoStackLocation;

    ULONG               Length;
    LARGE_INTEGER       ByteOffset;

    BOOLEAN             PagingIo;
    BOOLEAN             Nocache;
    BOOLEAN             SynchronousIo;
    BOOLEAN             MainResourceAcquired = FALSE;
    BOOLEAN             PagingIoResourceAcquired = FALSE;

    PUCHAR              Buffer = NULL;
    PRFSD_BDL           rfsd_bdl = NULL;

    __try {

        ASSERT(IrpContext);
        
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
    
        Vcb = (PRFSD_VCB) DeviceObject->DeviceExtension;

        ASSERT(Vcb != NULL);
        
        ASSERT((Vcb->Identifier.Type == RFSDVCB) &&
            (Vcb->Identifier.Size == sizeof(RFSD_VCB)));
        
        FileObject = IrpContext->FileObject;

        FcbOrVcb = (PRFSD_FCBVCB) FileObject->FsContext;
        
        ASSERT(FcbOrVcb);
        
        if (!(FcbOrVcb->Identifier.Type == RFSDVCB && (PVOID)FcbOrVcb == (PVOID)Vcb)) {

            Status = STATUS_INVALID_DEVICE_REQUEST;
            __leave;
        }

        Ccb = (PRFSD_CCB) FileObject->FsContext2;

        Irp = IrpContext->Irp;
            
        Irp->IoStatus.Information = 0;

        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
            
        Length = IoStackLocation->Parameters.Read.Length;
        ByteOffset = IoStackLocation->Parameters.Read.ByteOffset;
            
        PagingIo = (Irp->Flags & IRP_PAGING_IO ? TRUE : FALSE);
        Nocache = (Irp->Flags & IRP_NOCACHE ? TRUE : FALSE);
        SynchronousIo = (FileObject->Flags & FO_SYNCHRONOUS_IO ? TRUE : FALSE);
            
        if (Length == 0) {

            Irp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            __leave;
        }

        if (Ccb != NULL) {

            if(!IsFlagOn(Ccb->Flags, CCB_ALLOW_EXTENDED_DASD_IO)) {
                if (ByteOffset.QuadPart + Length > Vcb->Header.FileSize.QuadPart) {
                    Length = (ULONG)(Vcb->Header.FileSize.QuadPart - ByteOffset.QuadPart);
                }
            }

            {
                RFSD_BDL BlockArray;

                if ((ByteOffset.LowPart & (SECTOR_SIZE - 1)) ||
                   (Length & (SECTOR_SIZE - 1)) ) {
                    Status = STATUS_INVALID_PARAMETER;
                    __leave;
                }

                Status = RfsdLockUserBuffer(
                    IrpContext->Irp,
                    Length,
                    IoReadAccess );
                
                if (!NT_SUCCESS(Status)) {
                    __leave;
                }

                BlockArray.Irp = NULL;
                BlockArray.Lba = ByteOffset.QuadPart;;
                BlockArray.Offset = 0;
                BlockArray.Length = Length;

                Status = RfsdReadWriteBlocks(IrpContext,
                                    Vcb,
                                    &BlockArray,
                                    Length,
                                    1,
                                    FALSE   );
                Irp = IrpContext->Irp;

                __leave;
            }
        }
            
        if (Nocache &&
            ( (ByteOffset.LowPart & (SECTOR_SIZE - 1)) ||
              (Length & (SECTOR_SIZE - 1)) )) {
            DbgBreak();

            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }

        if (FlagOn(IrpContext->MinorFunction, IRP_MN_DPC)) {
            ClearFlag(IrpContext->MinorFunction, IRP_MN_DPC);
            Status = STATUS_PENDING;
            __leave;
        }
        
        if (!PagingIo) {
            if (!ExAcquireResourceSharedLite(
                &Vcb->MainResource,
                IrpContext->IsSynchronous )) {
                Status = STATUS_PENDING;
                __leave;
            }
            
            MainResourceAcquired = TRUE;

        } else {

            if (!ExAcquireResourceSharedLite(
                &Vcb->PagingIoResource,
                IrpContext->IsSynchronous ))
            {
                Status = STATUS_PENDING;
                __leave;
            }
            
            PagingIoResourceAcquired = TRUE;
        }
        
    
        if (ByteOffset.QuadPart >=
            Vcb->PartitionInformation.PartitionLength.QuadPart  ) {
            Irp->IoStatus.Information = 0;
            Status = STATUS_END_OF_FILE;
            __leave;
        }

        if (!Nocache) {

            if ((ByteOffset.QuadPart + Length) >
                    Vcb->PartitionInformation.PartitionLength.QuadPart ){
                Length = (ULONG) (
                    Vcb->PartitionInformation.PartitionLength.QuadPart -
                    ByteOffset.QuadPart);
                Length &= ~((ULONG)SECTOR_SIZE - 1);
            }

            if (FlagOn(IrpContext->MinorFunction, IRP_MN_MDL)) {

                CcMdlRead(
                    Vcb->StreamObj,
                    &ByteOffset,
                    Length,
                    &Irp->MdlAddress,
                    &Irp->IoStatus );
                
                Status = Irp->IoStatus.Status;

            } else {

                Buffer = RfsdGetUserBuffer(Irp);
                    
                if (Buffer == NULL) {
                    DbgBreak();
                    Status = STATUS_INVALID_USER_BUFFER;
                    __leave;
                }

                if (!CcCopyRead(
                    Vcb->StreamObj,
                    (PLARGE_INTEGER)&ByteOffset,
                    Length,
                    IrpContext->IsSynchronous,
                    Buffer,
                    &Irp->IoStatus )) {
                    Status = STATUS_PENDING;
                    __leave;
                }
                
                Status = Irp->IoStatus.Status;
            }

        } else {

            if ((ByteOffset.QuadPart + Length) >
                    Vcb->PartitionInformation.PartitionLength.QuadPart ) {
                Length = (ULONG) (
                    Vcb->PartitionInformation.PartitionLength.QuadPart -
                    ByteOffset.QuadPart);

                Length &= ~((ULONG)SECTOR_SIZE - 1);
            }

            Status = RfsdLockUserBuffer(
                IrpContext->Irp,
                Length,
                IoWriteAccess );
                
            if (!NT_SUCCESS(Status)) {
                __leave;
            }

#if DBG
            Buffer = RfsdGetUserBuffer(Irp);
#endif
            rfsd_bdl = ExAllocatePool(PagedPool, sizeof(RFSD_BDL));

            if (!rfsd_bdl)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                __leave;
            }

            rfsd_bdl->Irp = NULL;
            rfsd_bdl->Lba = ByteOffset.QuadPart;
            rfsd_bdl->Length = Length;
            rfsd_bdl->Offset = 0;

            Status = RfsdReadWriteBlocks(IrpContext,
                                Vcb,
                                rfsd_bdl,
                                Length,
                                1,
                                FALSE   );

            if (NT_SUCCESS(Status)) {
                Irp->IoStatus.Information = Length;
            }

            Irp = IrpContext->Irp;

            if (!Irp)
                __leave;
        }

    } __finally {

        if (PagingIoResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Vcb->PagingIoResource,
                ExGetCurrentResourceThread());
        }
        
        if (MainResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread());
        }

        if (rfsd_bdl)
            ExFreePool(rfsd_bdl);

        if (!IrpContext->ExceptionInProgress) {

            if (Irp) {

                if (Status == STATUS_PENDING &&
                    !IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_REQUEUED)) {

                    Status = RfsdLockUserBuffer(
                        IrpContext->Irp,
                        Length,
                        IoWriteAccess );
                    
                    if (NT_SUCCESS(Status)) {
                        Status = RfsdQueueRequest(IrpContext);
                    } else {
                        RfsdCompleteIrpContext(IrpContext, Status);
                    }

                } else {

                    if (NT_SUCCESS(Status)) {

                        if (!PagingIo) {

                            if (SynchronousIo) {

                                FileObject->CurrentByteOffset.QuadPart =
                                    ByteOffset.QuadPart + Irp->IoStatus.Information;
                            }

                            FileObject->Flags |= FO_FILE_FAST_IO_READ;
                        }
                    }

                    RfsdCompleteIrpContext(IrpContext, Status);;
                }

            } else {
                RfsdFreeIrpContext(IrpContext);
            }
        }
    }

    return Status;
}


// [mark] read some goop [from the file pt'd to by inode -- from whatever blocks buildbdl makes] into the buffer
NTSTATUS
RfsdReadInode (
            IN PRFSD_IRP_CONTEXT    IrpContext,				// [may be null]
            IN PRFSD_VCB            Vcb,
            IN PRFSD_KEY_IN_MEMORY  Key,					// Key that identifies the data on disk to be read.  This is simply forwarded through to BuildBDL. (NOTE: IN THIS CASE, THE OFFSET AND TYPE FIELDS MATTER)
            IN PRFSD_INODE          Inode,					// a filled Inode / stat data structure
            IN ULONGLONG            Offset,					// User's requested offset to read within the file (relative to the file)
            IN OUT PVOID            Buffer,					// buffer to read out to
            IN ULONG                Size,					// size of destination buffer
            OUT PULONG              dwRet )					// some kind of size [probably bytes read?]
{
    PRFSD_BDL   Bdl     = NULL;
    ULONG       blocks, i;
    NTSTATUS    Status = STATUS_UNSUCCESSFUL;
    IO_STATUS_BLOCK IoStatus;

    ULONGLONG   FileSize;
    ULONGLONG   AllocSize;	

    if (dwRet) {
        *dwRet = 0;
    }

    //
    // Calculate the inode size
    //

    FileSize = (ULONGLONG) Inode->i_size;

	// TODO: temporary hack to get correct alloc size for dir tails... but i doubt 8 works in all cases :-)  [what i should really be using is the size of the direct item in the block header!]
    // AllocSize = CEILING_ALIGNED(FileSize, (ULONGLONG)Vcb->BlockSize);
	// AllocSize = CEILING_ALIGNED(FileSize, (ULONGLONG) 8);
	AllocSize = CEILING_ALIGNED(FileSize, (ULONGLONG) 1);		// temp hack to ensure that i'll read out EXACTLY the # of bytes he requested

    //
    // Check inputed parameters: Offset / Size
    //

    if (Offset >= AllocSize) {

        RfsdPrint((DBG_ERROR, "RfsdReadInode: beyond the file range.\n"));
        return STATUS_SUCCESS;
    }

    if (Offset + Size > AllocSize) {

        Size = (ULONG)(AllocSize - Offset);
    }


//-----------------------------	  
	
    //
    // Build the scatterred block ranges to be read
    //

    Status = RfsdBuildBDL2(
		Vcb, Key, Inode, 
		&(blocks), &(Bdl)
		);

    if (!NT_SUCCESS(Status)) {
        goto errorout;
    }

    if (blocks <= 0) {
        Status = STATUS_SUCCESS;
        goto errorout;
    }

	
	{
	  ULONGLONG bufferPos = 0;

      for(i = 0; i < blocks; i++) {
		  if ( // The block is needed for the user's requested contents
			   // (The user's requested offset lies within the block, or the block's start is within the user's requested range)
			  ( (Offset >= Bdl[i].Offset) && (Offset < (Bdl[i].Offset + Bdl[i].Length)) ) ||		// The user's offset is within the block's range
			  ( (Bdl[i].Offset >= Offset) && (Bdl[i].Offset < (Offset + Size)) )					// The block's offset is within the user's range
		      )
		  {
			  ULONGLONG	offsetIntoBlock = max(Offset - Bdl[i].Offset, 0);			 
			  ULONGLONG	offsetFromDisk	= Bdl[i].Lba + offsetIntoBlock;			  
			  ULONGLONG bufferPos		= (Bdl[i].Offset + offsetIntoBlock) - Offset;
			  ULONGLONG	lengthToRead	= min(Size - bufferPos, Bdl[i].Length - offsetIntoBlock);

			  ASSERT(bufferPos < Size);

			  IoStatus.Information = 0;				

			  RfsdCopyRead(
					  Vcb->StreamObj, 
					  (PLARGE_INTEGER) (&offsetFromDisk),		// offset (relative to partition)
					  (ULONG) lengthToRead,							// length to read
					  PIN_WAIT,									// 
					  (PVOID)((PUCHAR)Buffer + bufferPos),	// buffer to read into
					  &IoStatus   );

			  Status = IoStatus.Status;
		  }
      }

	}

errorout:

    if (Bdl)				ExFreePool(Bdl);

    if (NT_SUCCESS(Status)) {

        if (dwRet) *dwRet = Size;
    }

    return Status;
}


NTSTATUS
RfsdReadFile(IN PRFSD_IRP_CONTEXT IrpContext)
{
    NTSTATUS            Status = STATUS_UNSUCCESSFUL;

    PRFSD_VCB           Vcb;
    PRFSD_FCB           Fcb;
    PRFSD_CCB           Ccb;
    PFILE_OBJECT        FileObject;
    PFILE_OBJECT        CacheObject;

    PDEVICE_OBJECT      DeviceObject;

    PIRP                Irp;
    PIO_STACK_LOCATION  IoStackLocation;

    ULONG               Length;
    ULONG               ReturnedLength;
    LARGE_INTEGER       ByteOffset;

    BOOLEAN             PagingIo;
    BOOLEAN             Nocache;
    BOOLEAN             SynchronousIo;
    BOOLEAN             MainResourceAcquired = FALSE;
    BOOLEAN             PagingIoResourceAcquired = FALSE;

    PUCHAR              Buffer;

    __try {

        ASSERT(IrpContext);
        
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
    
        Vcb = (PRFSD_VCB) DeviceObject->DeviceExtension;
        
        ASSERT(Vcb != NULL);
        
        ASSERT((Vcb->Identifier.Type == RFSDVCB) &&
            (Vcb->Identifier.Size == sizeof(RFSD_VCB)));
        
        FileObject = IrpContext->FileObject;
        
        Fcb = (PRFSD_FCB) FileObject->FsContext;
        
        ASSERT(Fcb);
    
        ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
            (Fcb->Identifier.Size == sizeof(RFSD_FCB)));		

        Ccb = (PRFSD_CCB) FileObject->FsContext2;

        Irp = IrpContext->Irp;
        
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        
        Length = IoStackLocation->Parameters.Read.Length;
        ByteOffset = IoStackLocation->Parameters.Read.ByteOffset;
        
		DbgPrint("$$$ " __FUNCTION__ " on key: %x,%xh to read %i bytes at the offset %xh in the file\n", 
			Fcb->RfsdMcb->Key.k_dir_id, Fcb->RfsdMcb->Key.k_objectid,
			Length, ByteOffset.QuadPart);

        PagingIo = (Irp->Flags & IRP_PAGING_IO ? TRUE : FALSE);
        Nocache = (Irp->Flags & IRP_NOCACHE ? TRUE : FALSE);
        SynchronousIo = (FileObject->Flags & FO_SYNCHRONOUS_IO ? TRUE : FALSE);

/*
        if (IsFlagOn(Fcb->Flags, FCB_FILE_DELETED)) {
            Status = STATUS_FILE_DELETED;
            __leave;
        }

        if (IsFlagOn(Fcb->Flags, FCB_DELETE_PENDING)) {
            Status = STATUS_DELETE_PENDING;
            __leave;
        }
*/

        if (Length == 0) {
            Irp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            __leave;
        }

        if (Nocache &&
           (ByteOffset.LowPart & (SECTOR_SIZE - 1) ||
            Length & (SECTOR_SIZE - 1))) {
            Status = STATUS_INVALID_PARAMETER;
            DbgBreak();
            __leave;
        }

        if (FlagOn(IrpContext->MinorFunction, IRP_MN_DPC)) {
            ClearFlag(IrpContext->MinorFunction, IRP_MN_DPC);
            Status = STATUS_PENDING;
            DbgBreak();
            __leave;
        }
        
        if (!PagingIo) {
            if (!ExAcquireResourceSharedLite(
                &Fcb->MainResource,
                IrpContext->IsSynchronous )) {
                Status = STATUS_PENDING;
                __leave;
            }
            
            MainResourceAcquired = TRUE;

            if (!FsRtlCheckLockForReadAccess(
                &Fcb->FileLockAnchor,
                Irp         )) {
                Status = STATUS_FILE_LOCK_CONFLICT;
                __leave;
            }
        } else {
            if (!ExAcquireResourceSharedLite(
                &Fcb->PagingIoResource,
                IrpContext->IsSynchronous )) {
                Status = STATUS_PENDING;
                __leave;
            }
            
            PagingIoResourceAcquired = TRUE;
        }
        
		if (!Nocache) {
			// Attempt cached access...

            if ((ByteOffset.QuadPart + (LONGLONG)Length) >
                Fcb->Header.FileSize.QuadPart ) {
                if (ByteOffset.QuadPart >= (Fcb->Header.FileSize.QuadPart)) {
                    Irp->IoStatus.Information = 0;
                    Status = STATUS_END_OF_FILE;
                    __leave;
                }

                Length =
                    (ULONG)(Fcb->Header.FileSize.QuadPart - ByteOffset.QuadPart);

            }

            ReturnedLength = Length;

            if (IsDirectory(Fcb)) {
                __leave;
            }

            {
                if (FileObject->PrivateCacheMap == NULL) {
                    CcInitializeCacheMap(
                        FileObject,
                        (PCC_FILE_SIZES)(&Fcb->Header.AllocationSize),
                        FALSE,
                        &RfsdGlobal->CacheManagerCallbacks,
                        Fcb );
                }

                CacheObject = FileObject;
            }

            if (FlagOn(IrpContext->MinorFunction, IRP_MN_MDL)) {
                CcMdlRead(
                    CacheObject,
                    (&ByteOffset),
                    Length,
                    &Irp->MdlAddress,
                    &Irp->IoStatus );
                
                Status = Irp->IoStatus.Status;

            } else {
                Buffer = RfsdGetUserBuffer(Irp);
                
                if (Buffer == NULL) {
                    Status = STATUS_INVALID_USER_BUFFER;
                    DbgBreak();
                    __leave;
                }
                
                if (!CcCopyRead(
                    CacheObject,						// the file object (representing the open operation performed by the thread)
                    (PLARGE_INTEGER)&ByteOffset,		// starting offset IN THE FILE, from where the read should be performed
                    Length,								// number of bytes requested in the read operation
                    IrpContext->IsSynchronous,
                    Buffer,								// < buffer to read the contents to
                    &Irp->IoStatus )) {
                    Status = STATUS_PENDING;
                    DbgBreak();
                    __leave;
                }
                
                Status = Irp->IoStatus.Status;
            }

        } else {
			// Attempt access without the cache...

            if ((ByteOffset.QuadPart + (LONGLONG)Length) > Fcb->Header.AllocationSize.QuadPart) {

                if (ByteOffset.QuadPart >= Fcb->Header.AllocationSize.QuadPart) {
                    Irp->IoStatus.Information = 0;
                    Status = STATUS_END_OF_FILE;
                    DbgBreak();
                    __leave;
                }

                Length =
                     (ULONG)(Fcb->Header.AllocationSize.QuadPart- ByteOffset.QuadPart);
            }

            ReturnedLength = Length;

            /* lock the user buffer into MDL and make them paged-in */
            Status = RfsdLockUserBuffer(
                        IrpContext->Irp,
                        Length,
                        IoWriteAccess );
                
            if (NT_SUCCESS(Status)) {

                /* Zero the total buffer */
                PVOID SystemVA = RfsdGetUserBuffer(IrpContext->Irp);
                if (SystemVA) {

                    RtlZeroMemory(SystemVA, Length);

                    RfsdPrint((DBG_INFO, "RfsdReadFile: Zero read buffer: Offset=%I64xh Size=%xh ... \n",
                               ByteOffset.QuadPart, Length));
                }

            } else {
                __leave;
            }

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = Length;

            
            Status = RfsdReadInode(
                        IrpContext,
                        Vcb,
                        &(Fcb->RfsdMcb->Key),
                        Fcb->Inode,
                        ByteOffset.QuadPart,
                        RfsdGetUserBuffer(IrpContext->Irp),		//  NOTE: Ext2fsd just passes NULL for the buffer, and relies on the initial cache call to retrieve tha data.  We'll instead be explicitly putting it into the user's buffer, via a much different mechanism.
                        Length,
                        &ReturnedLength);

            Irp = IrpContext->Irp;

        }

    } __finally {

        if (PagingIoResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Fcb->PagingIoResource,
                ExGetCurrentResourceThread());
        }
        
        if (MainResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Fcb->MainResource,
                ExGetCurrentResourceThread());
        }
        
        if (!IrpContext->ExceptionInProgress) {
            if (Irp) {
                if (Status == STATUS_PENDING) {

                    Status = RfsdLockUserBuffer(
                        IrpContext->Irp,
                        Length,
                        IoWriteAccess );
                    
                    if (NT_SUCCESS(Status)) {
                        Status = RfsdQueueRequest(IrpContext);
                    } else {
                        RfsdCompleteIrpContext(IrpContext, Status);
                    }
                } else {
                    if (NT_SUCCESS(Status)) {
                        if (!PagingIo) {
                            if (SynchronousIo) {
                                FileObject->CurrentByteOffset.QuadPart =
                                    ByteOffset.QuadPart + Irp->IoStatus.Information;
                            }

                            FileObject->Flags |= FO_FILE_FAST_IO_READ;
                        }
                    }

                    RfsdCompleteIrpContext(IrpContext, Status);
                }
            } else {
                RfsdFreeIrpContext(IrpContext);
            }
        }
    }
    
    return Status;

}

NTSTATUS
RfsdReadComplete (IN PRFSD_IRP_CONTEXT IrpContext)
{
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    PFILE_OBJECT    FileObject;
    PIRP            Irp;
    
    __try {

        ASSERT(IrpContext);
        
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
        
        FileObject = IrpContext->FileObject;
        
        Irp = IrpContext->Irp;
        
        CcMdlReadComplete(FileObject, Irp->MdlAddress);
        
        Irp->MdlAddress = NULL;
        
        Status = STATUS_SUCCESS;

    } __finally {

        if (!IrpContext->ExceptionInProgress) {
            RfsdCompleteIrpContext(IrpContext, Status);
        }
    }
    
    return Status;
}


NTSTATUS
RfsdRead (IN PRFSD_IRP_CONTEXT IrpContext)
{
    NTSTATUS            Status;
    PRFSD_VCB           Vcb;
    PRFSD_FCBVCB        FcbOrVcb;
    PDEVICE_OBJECT      DeviceObject;
    PFILE_OBJECT        FileObject;
    BOOLEAN             bCompleteRequest;
    
    ASSERT(IrpContext);
    
    ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
        (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));

    __try {

        if (FlagOn(IrpContext->MinorFunction, IRP_MN_COMPLETE)) {
			// Caller wants to tell the Cache Manager that a previously allocated MDL can be freed.
            Status =  RfsdReadComplete(IrpContext);
            bCompleteRequest = FALSE;

        } else {

            DeviceObject = IrpContext->DeviceObject;

            if (DeviceObject == RfsdGlobal->DeviceObject) {
                Status = STATUS_INVALID_DEVICE_REQUEST;
                bCompleteRequest = TRUE;
                __leave;
            }

            Vcb = (PRFSD_VCB) DeviceObject->DeviceExtension;

            if (Vcb->Identifier.Type != RFSDVCB ||
                Vcb->Identifier.Size != sizeof(RFSD_VCB) ) {
                Status = STATUS_INVALID_DEVICE_REQUEST;
                bCompleteRequest = TRUE;

                __leave;
            }

            if (IsFlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING)) {

                Status = STATUS_TOO_LATE;
                bCompleteRequest = TRUE;
                __leave;
            }

            FileObject = IrpContext->FileObject;
            
            FcbOrVcb = (PRFSD_FCBVCB) FileObject->FsContext;

            if (FcbOrVcb->Identifier.Type == RFSDVCB) {
                Status = RfsdReadVolume(IrpContext);
                bCompleteRequest = FALSE;
            } else if (FcbOrVcb->Identifier.Type == RFSDFCB) {
                Status = RfsdReadFile(IrpContext);
                bCompleteRequest = FALSE;
            } else {
                RfsdPrint((DBG_ERROR, "RfsdRead: INVALID PARAMETER ... \n"));
                DbgBreak();

                Status = STATUS_INVALID_PARAMETER;
                bCompleteRequest = TRUE;
            }
        }

    } __finally {
        if (bCompleteRequest) {
            RfsdCompleteIrpContext(IrpContext, Status);
        }
    }
    
    return Status;
}

/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Rfsd File System Driver for WinNT/2K/XP
 * FILE:             fsctl.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://rfsd.yeah.net
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "rfsdfs.h"

/* GLOBALS ***************************************************************/

extern PRFSD_GLOBAL RfsdGlobal;

/* DEFINITIONS *************************************************************/

BOOLEAN
RfsdIsMediaWriteProtected (
    IN PRFSD_IRP_CONTEXT  IrpContext,
    IN PDEVICE_OBJECT     TargetDevice);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RfsdSetVpbFlag)
#pragma alloc_text(PAGE, RfsdClearVpbFlag)
#pragma alloc_text(PAGE, RfsdIsHandleCountZero)
#pragma alloc_text(PAGE, RfsdLockVcb)
#pragma alloc_text(PAGE, RfsdLockVolume)
#pragma alloc_text(PAGE, RfsdUnlockVcb)
#pragma alloc_text(PAGE, RfsdUnlockVolume)
#pragma alloc_text(PAGE, RfsdAllowExtendedDasdIo)
#pragma alloc_text(PAGE, RfsdUserFsRequest)
#pragma alloc_text(PAGE, RfsdIsMediaWriteProtected)
#pragma alloc_text(PAGE, RfsdMountVolume)
#pragma alloc_text(PAGE, RfsdCheckDismount)
#pragma alloc_text(PAGE, RfsdPurgeVolume)
#pragma alloc_text(PAGE, RfsdPurgeFile)
#pragma alloc_text(PAGE, RfsdDismountVolume)
#pragma alloc_text(PAGE, RfsdIsVolumeMounted)
#pragma alloc_text(PAGE, RfsdVerifyVolume)
#pragma alloc_text(PAGE, RfsdFileSystemControl)
#endif


VOID
RfsdSetVpbFlag (
        IN PVPB     Vpb,
        IN USHORT   Flag )
{
    KIRQL OldIrql;
    
    IoAcquireVpbSpinLock(&OldIrql);
    
    Vpb->Flags |= Flag;
    
    IoReleaseVpbSpinLock(OldIrql);
}

VOID
RfsdClearVpbFlag (
          IN PVPB     Vpb,
          IN USHORT   Flag )
{
    KIRQL OldIrql;
    
    IoAcquireVpbSpinLock(&OldIrql);
    
    Vpb->Flags &= ~Flag;
    
    IoReleaseVpbSpinLock(OldIrql);
}

BOOLEAN
RfsdIsHandleCountZero(IN PRFSD_VCB Vcb)
{
    PRFSD_FCB   Fcb;
    PLIST_ENTRY List;

    for( List = Vcb->FcbList.Flink;
         List != &Vcb->FcbList;
         List = List->Flink )  {

        Fcb = CONTAINING_RECORD(List, RFSD_FCB, Next);

        ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
               (Fcb->Identifier.Size == sizeof(RFSD_FCB)));

        RfsdPrint((DBG_INFO, "RfsdIsHandleCountZero: Key:%x,%xh File:%S OpenHandleCount=%xh\n",
			Fcb->RfsdMcb->Key.k_dir_id, Fcb->RfsdMcb->Key.k_objectid, Fcb->RfsdMcb->ShortName.Buffer, Fcb->OpenHandleCount));

        if (Fcb->OpenHandleCount) {
            return FALSE;
        }
    }

    return TRUE;
}

NTSTATUS
RfsdLockVcb (IN PRFSD_VCB    Vcb,
             IN PFILE_OBJECT FileObject)
{
    NTSTATUS           Status;
   
    __try {

        if (FlagOn(Vcb->Flags, VCB_VOLUME_LOCKED)) {
            RfsdPrint((DBG_INFO, "RfsdLockVolume: Volume is already locked.\n"));
            
            Status = STATUS_ACCESS_DENIED;
            
            __leave;
        }
        
        if (Vcb->OpenFileHandleCount > (ULONG)(FileObject ? 1 : 0)) {
            RfsdPrint((DBG_INFO, "RfsdLockVcb: There are still opened files.\n"));
            
            Status = STATUS_ACCESS_DENIED;
            
            __leave;
        }
        
        if (!RfsdIsHandleCountZero(Vcb)) {
            RfsdPrint((DBG_INFO, "RfsdLockVcb: Thare are still opened files.\n"));
            
            Status = STATUS_ACCESS_DENIED;
            
            __leave;
        }
        
        SetFlag(Vcb->Flags, VCB_VOLUME_LOCKED);
        
        RfsdSetVpbFlag(Vcb->Vpb, VPB_LOCKED);

        Vcb->LockFile = FileObject;
        
        RfsdPrint((DBG_INFO, "RfsdLockVcb: Volume locked.\n"));
        
        Status = STATUS_SUCCESS;

    } __finally {
        // Nothing
    }
    
    return Status;
}


NTSTATUS
RfsdLockVolume (IN PRFSD_IRP_CONTEXT IrpContext)
{
    PIO_STACK_LOCATION IrpSp;
    PDEVICE_OBJECT  DeviceObject;
    PRFSD_VCB       Vcb;
    NTSTATUS        Status;
    BOOLEAN VcbResourceAcquired = FALSE;
   
    __try {

        ASSERT(IrpContext != NULL);
        
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
        
        Status = STATUS_UNSUCCESSFUL;

        //
        // This request is not allowed on the main device object
        //
        if (DeviceObject == RfsdGlobal->DeviceObject) {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }
        
        Vcb = (PRFSD_VCB) DeviceObject->DeviceExtension;
        
        ASSERT(Vcb != NULL);

        ASSERT((Vcb->Identifier.Type == RFSDVCB) &&
            (Vcb->Identifier.Size == sizeof(RFSD_VCB)));

        ASSERT(IsMounted(Vcb));

        IrpSp = IoGetCurrentIrpStackLocation(IrpContext->Irp);

#if (_WIN32_WINNT >= 0x0500)
        CcWaitForCurrentLazyWriterActivity();
#endif
        ExAcquireResourceExclusiveLite(
            &Vcb->MainResource,
            TRUE            );
        
        VcbResourceAcquired = TRUE;

        Status = RfsdLockVcb(Vcb, IrpSp->FileObject);        

    } __finally {

        if (VcbResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread()
                );
        }
        
        if (!IrpContext->ExceptionInProgress) {
            RfsdCompleteIrpContext(IrpContext,  Status);
        }
    }
    
    return Status;
}

NTSTATUS
RfsdUnlockVcb ( IN PRFSD_VCB    Vcb,
                IN PFILE_OBJECT FileObject )
{
    NTSTATUS        Status;

    __try {

        if (FileObject && FileObject->FsContext != Vcb) {

            Status = STATUS_NOT_LOCKED;
            __leave;
        }
       
        if (!FlagOn(Vcb->Flags, VCB_VOLUME_LOCKED)) {
            RfsdPrint((DBG_ERROR, ": RfsdUnlockVcb: Volume is not locked.\n"));
            Status = STATUS_NOT_LOCKED;
            __leave;
        }

        if (Vcb->LockFile == FileObject) {

            ClearFlag(Vcb->Flags, VCB_VOLUME_LOCKED);
        
            RfsdClearVpbFlag(Vcb->Vpb, VPB_LOCKED);
        
            RfsdPrint((DBG_INFO, "RfsdUnlockVcb: Volume unlocked.\n"));
        
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_NOT_LOCKED;
        }

    } __finally {
        // Nothing
    }

    return Status;
}

NTSTATUS
RfsdUnlockVolume (
         IN PRFSD_IRP_CONTEXT IrpContext
         )
{
    PIO_STACK_LOCATION IrpSp;
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status;
    PRFSD_VCB       Vcb;
    BOOLEAN         VcbResourceAcquired = FALSE;
    
    __try {

        ASSERT(IrpContext != NULL);
        
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
        
        //
        // This request is not allowed on the main device object
        //
        if (DeviceObject == RfsdGlobal->DeviceObject) {
            Status = STATUS_INVALID_PARAMETER;
            __leave;
        }
        
        Vcb = (PRFSD_VCB) DeviceObject->DeviceExtension;
        
        ASSERT(Vcb != NULL);
        
        ASSERT((Vcb->Identifier.Type == RFSDVCB) &&
            (Vcb->Identifier.Size == sizeof(RFSD_VCB)));

        ASSERT(IsMounted(Vcb));

        IrpSp = IoGetCurrentIrpStackLocation(IrpContext->Irp);

        ExAcquireResourceExclusiveLite(
            &Vcb->MainResource,
            TRUE );
        
        VcbResourceAcquired = TRUE;

        Status = RfsdUnlockVcb(Vcb, IrpSp->FileObject);

    } __finally {

        if (VcbResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread()
                );
        }
        
        if (!IrpContext->ExceptionInProgress) {
            RfsdCompleteIrpContext(IrpContext,  Status);
        }
    }
    
    return Status;
}


NTSTATUS
RfsdInvalidateVolumes ( IN PRFSD_IRP_CONTEXT IrpContext )
{
    NTSTATUS            Status;
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpSp;

    HANDLE              Handle;

    PLIST_ENTRY         ListEntry;

    PFILE_OBJECT        FileObject;
    PDEVICE_OBJECT      DeviceObject;
    BOOLEAN             GlobalResourceAcquired = FALSE;

    LUID Privilege = {SE_TCB_PRIVILEGE, 0};

    __try {

        Irp   = IrpContext->Irp;
        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        if (!SeSinglePrivilegeCheck(Privilege, Irp->RequestorMode)) {
            Status = STATUS_PRIVILEGE_NOT_HELD;
            __leave;
        }

        if (
#ifndef _GNU_NTIFS_
        IrpSp->Parameters.FileSystemControl.InputBufferLength
#else
        ((PEXTENDED_IO_STACK_LOCATION)(IrpSp))->
        Parameters.FileSystemControl.InputBufferLength
#endif
            != sizeof(HANDLE)) {

            Status = STATUS_INVALID_PARAMETER;

            __leave;
        }

        Handle = *(PHANDLE)Irp->AssociatedIrp.SystemBuffer;

        Status = ObReferenceObjectByHandle( Handle,
                                            0,
                                            *IoFileObjectType,
                                            KernelMode,
                                            &FileObject,
                                            NULL );

        if (!NT_SUCCESS(Status)) {
            __leave;
        } else {
            ObDereferenceObject(FileObject);
            DeviceObject = FileObject->DeviceObject;
        }

        RfsdPrint((DBG_INFO, "RfsdInvalidateVolumes: FileObject=%xh ...\n", FileObject));

        ExAcquireResourceExclusiveLite(
            &RfsdGlobal->Resource,
            TRUE );

        GlobalResourceAcquired = TRUE;

        ListEntry = RfsdGlobal->VcbList.Flink;

        while (ListEntry != &RfsdGlobal->VcbList)  {

            PRFSD_VCB Vcb;

            Vcb = CONTAINING_RECORD(ListEntry, RFSD_VCB, Next);

            ListEntry = ListEntry->Flink;

            RfsdPrint((DBG_INFO, "RfsdInvalidateVolumes: Vcb=%xh Vcb->Vpb=%xh...\n", Vcb, Vcb->Vpb));
            if (Vcb->Vpb && (Vcb->Vpb->RealDevice == DeviceObject))
            {
                ExAcquireResourceExclusive(&Vcb->MainResource, TRUE);
                RfsdPrint((DBG_INFO, "RfsdInvalidateVolumes: RfsdPurgeVolume...\n"));
                RfsdPurgeVolume(Vcb, FALSE);
                ClearFlag(Vcb->Flags, VCB_MOUNTED);
                ExReleaseResource(&Vcb->MainResource);
        
                //
                // Vcb is still attached on the list ......
                //

                if (ListEntry->Blink == &Vcb->Next)
                {
                    RfsdPrint((DBG_INFO, "RfsdInvalidateVolumes: RfsdCheckDismount...\n"));
                    RfsdCheckDismount(IrpContext, Vcb, FALSE);
                }
            }
        }

    } __finally {

        if (GlobalResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &RfsdGlobal->Resource,
                ExGetCurrentResourceThread() );
        }

        RfsdCompleteIrpContext(IrpContext, Status);
    }

    return Status;
}

NTSTATUS
RfsdAllowExtendedDasdIo(IN PRFSD_IRP_CONTEXT IrpContext)
{
    PIO_STACK_LOCATION IrpSp;
    PRFSD_VCB Vcb;
    PRFSD_CCB Ccb;

    IrpSp = IoGetCurrentIrpStackLocation(IrpContext->Irp);

    Vcb = (PRFSD_VCB) IrpSp->FileObject->FsContext;
    Ccb = (PRFSD_CCB) IrpSp->FileObject->FsContext2;

    ASSERT(Vcb != NULL);
        
    ASSERT((Vcb->Identifier.Type == RFSDVCB) &&
           (Vcb->Identifier.Size == sizeof(RFSD_VCB)));

    ASSERT(IsMounted(Vcb));

    if (Ccb) {
        SetFlag(Ccb->Flags, CCB_ALLOW_EXTENDED_DASD_IO);

        RfsdCompleteIrpContext(IrpContext, STATUS_SUCCESS);

        return STATUS_SUCCESS;
    } else {
        return STATUS_INVALID_PARAMETER;
    }
}


NTSTATUS
RfsdUserFsRequest (IN PRFSD_IRP_CONTEXT IrpContext)
{
    PIRP                Irp;
    PIO_STACK_LOCATION  IoStackLocation;
    ULONG               FsControlCode;
    NTSTATUS            Status;
    
    ASSERT(IrpContext);
    
    ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
        (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
    
    Irp = IrpContext->Irp;
    
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    
#ifndef _GNU_NTIFS_
    FsControlCode =
        IoStackLocation->Parameters.FileSystemControl.FsControlCode;
#else
    FsControlCode = ((PEXTENDED_IO_STACK_LOCATION)
        IoStackLocation)->Parameters.FileSystemControl.FsControlCode;
#endif
    
    switch (FsControlCode) {

    case FSCTL_LOCK_VOLUME:
        Status = RfsdLockVolume(IrpContext);
        break;
        
    case FSCTL_UNLOCK_VOLUME:
        Status = RfsdUnlockVolume(IrpContext);
        break;
        
    case FSCTL_DISMOUNT_VOLUME:
        Status = RfsdDismountVolume(IrpContext);
        break;
        
    case FSCTL_IS_VOLUME_MOUNTED:
        Status = RfsdIsVolumeMounted(IrpContext);
        break;

    case FSCTL_INVALIDATE_VOLUMES:
        Status = RfsdInvalidateVolumes(IrpContext);
        break;

#if (_WIN32_WINNT >= 0x0500)
    case FSCTL_ALLOW_EXTENDED_DASD_IO:
        Status = RfsdAllowExtendedDasdIo(IrpContext);
        break;
#endif //(_WIN32_WINNT >= 0x0500)
        
    default:

        RfsdPrint((DBG_ERROR, "RfsdUserFsRequest: Invalid User Request: %xh.\n", FsControlCode));
        Status = STATUS_INVALID_DEVICE_REQUEST;

        RfsdCompleteIrpContext(IrpContext,  Status);
    }
    
    return Status;
}

BOOLEAN
RfsdIsMediaWriteProtected (
    IN PRFSD_IRP_CONTEXT   IrpContext,
    IN PDEVICE_OBJECT TargetDevice
    )
{
    PIRP            Irp;
    KEVENT          Event;
    NTSTATUS        Status;
    IO_STATUS_BLOCK IoStatus;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_IS_WRITABLE,
                                         TargetDevice,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         FALSE,
                                         &Event,
                                         &IoStatus );

    if (Irp == NULL) {
        return FALSE;
    }

    SetFlag(IoGetNextIrpStackLocation(Irp)->Flags, SL_OVERRIDE_VERIFY_VOLUME);

    Status = IoCallDriver(TargetDevice, Irp);

    if (Status == STATUS_PENDING) {

        (VOID) KeWaitForSingleObject( &Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER)NULL );

        Status = IoStatus.Status;
    }

    return (BOOLEAN)(Status == STATUS_MEDIA_WRITE_PROTECTED);
}

NTSTATUS
RfsdMountVolume (IN PRFSD_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT              MainDeviceObject;
    BOOLEAN                     GlobalDataResourceAcquired = FALSE;
    PIRP                        Irp;
    PIO_STACK_LOCATION          IoStackLocation;
    PDEVICE_OBJECT              TargetDeviceObject;
    NTSTATUS                    Status = STATUS_UNRECOGNIZED_VOLUME;
    PDEVICE_OBJECT              VolumeDeviceObject = NULL;
    PRFSD_VCB                   Vcb;
    PRFSD_SUPER_BLOCK           RfsdSb = NULL;
    ULONG                       dwBytes;
    DISK_GEOMETRY               DiskGeometry;
    
    __try {


        ASSERT(IrpContext != NULL);
        
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
        
        MainDeviceObject = IrpContext->DeviceObject;

        //
        //  Make sure we can wait.
        //

        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

        //
        // This request is only allowed on the main device object
        //
        if (MainDeviceObject != RfsdGlobal->DeviceObject) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            __leave;
        }
        
        ExAcquireResourceExclusiveLite(
            &(RfsdGlobal->Resource),
            TRUE );
        
        GlobalDataResourceAcquired = TRUE;
        
        if (FlagOn(RfsdGlobal->Flags, RFSD_UNLOAD_PENDING)) {
            Status = STATUS_UNRECOGNIZED_VOLUME;
            __leave;
        }
        
        Irp = IrpContext->Irp;
        
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        
        TargetDeviceObject =
            IoStackLocation->Parameters.MountVolume.DeviceObject;

        dwBytes = sizeof(DISK_GEOMETRY);
        Status = RfsdDiskIoControl(
            TargetDeviceObject,
            IOCTL_DISK_GET_DRIVE_GEOMETRY,
            NULL,
            0,
            &DiskGeometry,
            &dwBytes );
        
        if (!NT_SUCCESS(Status)) {
            __leave;
        }
        
        Status = IoCreateDevice(
            MainDeviceObject->DriverObject,
            sizeof(RFSD_VCB),
            NULL,
            FILE_DEVICE_DISK_FILE_SYSTEM,
            0,
            FALSE,
            &VolumeDeviceObject );
        
        if (!NT_SUCCESS(Status)) {
            __leave;
        }

        VolumeDeviceObject->StackSize = (CCHAR)(TargetDeviceObject->StackSize + 1);

        if (TargetDeviceObject->AlignmentRequirement > 
            VolumeDeviceObject->AlignmentRequirement) {

            VolumeDeviceObject->AlignmentRequirement = 
                TargetDeviceObject->AlignmentRequirement;
        }

        (IoStackLocation->Parameters.MountVolume.Vpb)->DeviceObject =
            VolumeDeviceObject;
        
        Vcb = (PRFSD_VCB) VolumeDeviceObject->DeviceExtension;

        RtlZeroMemory(Vcb, sizeof(RFSD_VCB));
        
        Vcb->Identifier.Type = RFSDVCB;
        Vcb->Identifier.Size = sizeof(RFSD_VCB);

        Vcb->TargetDeviceObject = TargetDeviceObject;
        Vcb->DiskGeometry = DiskGeometry;

        RfsdSb = RfsdLoadSuper(Vcb, FALSE);

        if (RfsdSb) {
            if ( SuperblockContainsMagicKey(RfsdSb) ) {
                RfsdPrint((DBG_INFO, "Volume of ReiserFS rfsd file system is found.\n"));
                Status = STATUS_SUCCESS;
            } else  {
                Status = STATUS_UNRECOGNIZED_VOLUME;
            }
        }

        if (!NT_SUCCESS(Status)) {
            __leave;
        }
		
		Vcb->BlockSize = RfsdSb->s_blocksize;		// NOTE: FFS also does this here, since LoadGroup is not called in the non-ext2 drivers
		Vcb->GroupDesc = NULL;						// NOTE: GroupDesc is not used for ReiserFS.  Setting it to NULL will keep other code from barfing.
		// Vcb->SectorBits = RFSDLog(SECTOR_SIZE);	// NOTE: SectorBits are unused for ReiserFS
        
		Status = RfsdInitializeVcb(IrpContext, Vcb, RfsdSb, TargetDeviceObject, 
                    VolumeDeviceObject, IoStackLocation->Parameters.MountVolume.Vpb);

        if (NT_SUCCESS(Status))  {
            if (RfsdIsMediaWriteProtected(IrpContext, TargetDeviceObject)) {
                SetFlag(Vcb->Flags, VCB_WRITE_PROTECTED);
            } else {
                ClearFlag(Vcb->Flags, VCB_WRITE_PROTECTED);
            }

            SetFlag(Vcb->Flags, VCB_MOUNTED);

            RfsdInsertVcb(Vcb);

            ClearFlag(VolumeDeviceObject->Flags, DO_DEVICE_INITIALIZING);
        }

    } __finally {

        if (GlobalDataResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &RfsdGlobal->Resource,
                ExGetCurrentResourceThread() );
        }

        if (!NT_SUCCESS(Status)) {

            if (RfsdSb) {
                ExFreePool(RfsdSb);
            }

            if (VolumeDeviceObject) {
                IoDeleteDevice(VolumeDeviceObject);
            }
        }
        
        if (!IrpContext->ExceptionInProgress) {
            if (NT_SUCCESS(Status)) {
                ClearFlag(VolumeDeviceObject->Flags, DO_DEVICE_INITIALIZING);
            }
            RfsdCompleteIrpContext(IrpContext,  Status);
        }
    }
    
    return Status;
}


NTSTATUS
RfsdVerifyVolume (IN PRFSD_IRP_CONTEXT IrpContext)
{

    PDEVICE_OBJECT          DeviceObject;
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;
    PRFSD_SUPER_BLOCK       rfsd_sb = NULL;
    PRFSD_VCB               Vcb;
    BOOLEAN                 VcbResourceAcquired = FALSE;
    BOOLEAN                 GlobalResourceAcquired = FALSE;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IoStackLocation;
    ULONG                   ChangeCount;
    ULONG                   dwReturn;
    
    __try {

        ASSERT(IrpContext != NULL);
        
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

        ExAcquireResourceExclusiveLite(
            &RfsdGlobal->Resource,
            TRUE );
        
        GlobalResourceAcquired = TRUE;
        
        Vcb = (PRFSD_VCB) DeviceObject->DeviceExtension;
        
        ASSERT(Vcb != NULL);
        
        ASSERT((Vcb->Identifier.Type == RFSDVCB) &&
            (Vcb->Identifier.Size == sizeof(RFSD_VCB)));

        ExAcquireResourceExclusiveLite(
            &Vcb->MainResource,
            TRUE );
        
        VcbResourceAcquired = TRUE;
        
        if (!FlagOn(Vcb->TargetDeviceObject->Flags, DO_VERIFY_VOLUME)) {
            Status = STATUS_SUCCESS;
            __leave;
        }

        if (!IsMounted(Vcb)) {
            Status = STATUS_WRONG_VOLUME;
            __leave;
        }

        dwReturn = sizeof(ULONG);
        Status = RfsdDiskIoControl(
                Vcb->TargetDeviceObject,
                IOCTL_DISK_CHECK_VERIFY,
                NULL,
                0,
                &ChangeCount,
                &dwReturn );

        if (ChangeCount != Vcb->ChangeCount) {
            Status = STATUS_WRONG_VOLUME;
            __leave;
        }
        
        Irp = IrpContext->Irp;

        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        
        rfsd_sb = RfsdLoadSuper(Vcb, TRUE);

		// FUTURE: use the volume name and uuid from the extended superblock to make this happen.
		// NOTE: The magic check will have to use the same thing as mount did!
        if ((rfsd_sb != NULL) /*&& (rfsd_sb->s_magic == RFSD_SUPER_MAGIC) &&
            (memcmp(rfsd_sb->s_uuid, SUPER_BLOCK->s_uuid, 16) == 0) &&
            (memcmp(rfsd_sb->s_volume_name, SUPER_BLOCK->s_volume_name, 16) ==0)*/) {
            ClearFlag(Vcb->TargetDeviceObject->Flags, DO_VERIFY_VOLUME);

            if (RfsdIsMediaWriteProtected(IrpContext, Vcb->TargetDeviceObject)) {
                SetFlag(Vcb->Flags, VCB_WRITE_PROTECTED);
            } else {
                ClearFlag(Vcb->Flags, VCB_WRITE_PROTECTED);
            }
                
            RfsdPrint((DBG_INFO, "RfsdVerifyVolume: Volume verify succeeded.\n"));

            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_WRONG_VOLUME;

            RfsdPurgeVolume(Vcb, FALSE);
             
            SetFlag(Vcb->Flags, VCB_DISMOUNT_PENDING);
            
            ClearFlag(Vcb->TargetDeviceObject->Flags, DO_VERIFY_VOLUME);
            
            RfsdPrint((DBG_INFO, "RfsdVerifyVolume: Volume verify failed.\n"));
        }

    } __finally {

        if (rfsd_sb)
            ExFreePool(rfsd_sb);

        if (VcbResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread()
                );
        }

        if (GlobalResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &RfsdGlobal->Resource,
                ExGetCurrentResourceThread() );
        }
        
        if (!IrpContext->ExceptionInProgress) {
            RfsdCompleteIrpContext(IrpContext,  Status);
        }
    }
    
    return Status;
}


NTSTATUS
RfsdIsVolumeMounted (IN PRFSD_IRP_CONTEXT IrpContext)
{
    ASSERT(IrpContext);

    ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
           (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));

    return RfsdVerifyVolume(IrpContext);
}


NTSTATUS
RfsdDismountVolume (IN PRFSD_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT  DeviceObject;
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    PRFSD_VCB       Vcb;
    BOOLEAN         VcbResourceAcquired = FALSE;
    
    __try {

        ASSERT(IrpContext != NULL);
        
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

        ExAcquireResourceExclusiveLite(
            &Vcb->MainResource,
            TRUE );
        
        VcbResourceAcquired = TRUE;

        if ( IsFlagOn(Vcb->Flags, VCB_DISMOUNT_PENDING)) {
            Status = STATUS_VOLUME_DISMOUNTED;
            __leave;
        }

/*        
        if (!FlagOn(Vcb->Flags, VCB_VOLUME_LOCKED)) {
            RfsdPrint((DBG_ERROR, "RfsdDismount: Volume is not locked.\n"));
            
            Status = STATUS_ACCESS_DENIED;
           
            __leave;
        }
*/
#if DISABLED
        RfsdFlushFiles(Vcb, FALSE);

        RfsdFlushVolume(Vcb, FALSE);

        RfsdPurgeVolume(Vcb, TRUE);
#endif
        ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread() );

        VcbResourceAcquired = FALSE;

        RfsdCheckDismount(IrpContext, Vcb, TRUE);

        RfsdPrint((DBG_INFO, "RfsdDismount: Volume dismount pending.\n"));

        Status = STATUS_SUCCESS;

    } __finally {

        if (VcbResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread()
                );
        }
        
        if (!IrpContext->ExceptionInProgress) {
            RfsdCompleteIrpContext(IrpContext,  Status);
        }
    }
    
    return Status;
}

BOOLEAN
RfsdCheckDismount (
    IN PRFSD_IRP_CONTEXT IrpContext,
    IN PRFSD_VCB         Vcb,
    IN BOOLEAN           bForce   )
{
    KIRQL   Irql;
    PVPB    Vpb = Vcb->Vpb;
    BOOLEAN bDeleted = FALSE;
    ULONG   UnCleanCount = 0;

    ExAcquireResourceExclusiveLite(
        &RfsdGlobal->Resource, TRUE );

    ExAcquireResourceExclusiveLite(
        &Vcb->MainResource, TRUE );

    if ((IrpContext->MajorFunction == IRP_MJ_CREATE) &&
        (IrpContext->RealDevice == Vcb->RealDevice)) {
        UnCleanCount = 3;
    } else {
        UnCleanCount = 2;
    }

    IoAcquireVpbSpinLock (&Irql);

    if ((Vpb->ReferenceCount == UnCleanCount) || bForce) {

        if ((Vpb->ReferenceCount != UnCleanCount) && bForce) {
            KdPrint(("RfsdCheckDismount: force dismount ...\n"));
        }

        ClearFlag( Vpb->Flags, VPB_MOUNTED );
        ClearFlag( Vpb->Flags, VPB_LOCKED );

        if ((Vcb->RealDevice != Vpb->RealDevice) &&
            (Vcb->RealDevice->Vpb == Vpb)) {
            SetFlag( Vcb->RealDevice->Flags, DO_DEVICE_INITIALIZING );
            SetFlag( Vpb->Flags, VPB_PERSISTENT );
        }

        RfsdRemoveVcb(Vcb);

        ClearFlag(Vpb->Flags, VPB_MOUNTED);
        SetFlag(Vcb->Flags, VCB_DISMOUNT_PENDING);

        Vpb->DeviceObject = NULL;

        bDeleted = TRUE;
    }

    IoReleaseVpbSpinLock(Irql);

    ExReleaseResourceForThreadLite(
        &Vcb->MainResource,
        ExGetCurrentResourceThread() );

    ExReleaseResourceForThreadLite(
        &RfsdGlobal->Resource,
        ExGetCurrentResourceThread() );

    if (bDeleted) {
        KdPrint(("RfsdCheckDismount: now free the vcb ...\n"));
        RfsdFreeVcb(Vcb);
    }

    return bDeleted;
}


NTSTATUS
RfsdPurgeVolume (IN PRFSD_VCB Vcb,
                 IN BOOLEAN  FlushBeforePurge )
{
    PRFSD_FCB       Fcb;
    LIST_ENTRY      FcbList;
    PLIST_ENTRY     ListEntry;
    PFCB_LIST_ENTRY FcbListEntry;
   
    __try {

        ASSERT(Vcb != NULL);
        
        ASSERT((Vcb->Identifier.Type == RFSDVCB) &&
            (Vcb->Identifier.Size == sizeof(RFSD_VCB)));
        
        if ( IsFlagOn(Vcb->Flags, VCB_READ_ONLY) ||
             IsFlagOn(Vcb->Flags, VCB_WRITE_PROTECTED)) {
            FlushBeforePurge = FALSE;
        }

        FcbListEntry= NULL;
        InitializeListHead(&FcbList);
        
        for (ListEntry = Vcb->FcbList.Flink;
             ListEntry != &Vcb->FcbList;
             ListEntry = ListEntry->Flink  ) {

            Fcb = CONTAINING_RECORD(ListEntry, RFSD_FCB, Next);

            Fcb->ReferenceCount++;

            RfsdPrint((DBG_INFO, "RfsdPurgeVolume: %s refercount=%xh\n", Fcb->AnsiFileName.Buffer, Fcb->ReferenceCount));
            
            FcbListEntry = ExAllocatePool(PagedPool, sizeof(FCB_LIST_ENTRY));

            if (FcbListEntry) {

                FcbListEntry->Fcb = Fcb;
            
                InsertTailList(&FcbList, &FcbListEntry->Next);
            } else {
                RfsdPrint((DBG_ERROR, "RfsdPurgeVolume: Error allocating FcbListEntry ...\n"));
            }
        }
        
        while (!IsListEmpty(&FcbList)) {

            ListEntry = RemoveHeadList(&FcbList);
            
            FcbListEntry = CONTAINING_RECORD(ListEntry, FCB_LIST_ENTRY, Next);
            
            Fcb = FcbListEntry->Fcb;

            if (ExAcquireResourceExclusiveLite(
                &Fcb->MainResource,
                TRUE )) {

                RfsdPurgeFile(Fcb, FlushBeforePurge);

                if (!Fcb->OpenHandleCount && Fcb->ReferenceCount == 1) {
                    RemoveEntryList(&Fcb->Next);
                    RfsdFreeFcb(Fcb);
                } else {
                    ExReleaseResourceForThreadLite(
                        &Fcb->MainResource,
                        ExGetCurrentResourceThread());
                }
            }
           
            ExFreePool(FcbListEntry);
        }

        if (FlushBeforePurge) {
            ExAcquireSharedStarveExclusive(&Vcb->PagingIoResource, TRUE);
            ExReleaseResource(&Vcb->PagingIoResource);

            CcFlushCache(&Vcb->SectionObject, NULL, 0, NULL);
        }

        if (Vcb->SectionObject.ImageSectionObject) {
            MmFlushImageSection(&Vcb->SectionObject, MmFlushForWrite);
        }
    
        if (Vcb->SectionObject.DataSectionObject) {
            CcPurgeCacheSection(&Vcb->SectionObject, NULL, 0, FALSE);
        }
        
        RfsdPrint((DBG_INFO, "RfsdPurgeVolume: Volume flushed and purged.\n"));

    } __finally {
        // Nothing
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RfsdPurgeFile ( IN PRFSD_FCB Fcb,
                IN BOOLEAN  FlushBeforePurge )
{
    IO_STATUS_BLOCK    IoStatus;

    ASSERT(Fcb != NULL);
        
    ASSERT((Fcb->Identifier.Type == RFSDFCB) &&
        (Fcb->Identifier.Size == sizeof(RFSD_FCB)));

    
    if( !IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY) && FlushBeforePurge &&
        !IsFlagOn(Fcb->Vcb->Flags, VCB_WRITE_PROTECTED)) {

        RfsdPrint((DBG_INFO, "RfsdPurgeFile: CcFlushCache on %s.\n", 
                             Fcb->AnsiFileName.Buffer));

        ExAcquireSharedStarveExclusive(&Fcb->PagingIoResource, TRUE);
        ExReleaseResource(&Fcb->PagingIoResource);

        CcFlushCache(&Fcb->SectionObject, NULL, 0, &IoStatus);

        ClearFlag(Fcb->Flags, FCB_FILE_MODIFIED);
    }
    
    if (Fcb->SectionObject.ImageSectionObject) {

        RfsdPrint((DBG_INFO, "RfsdPurgeFile: MmFlushImageSection on %s.\n", 
                             Fcb->AnsiFileName.Buffer));
    
        MmFlushImageSection(&Fcb->SectionObject, MmFlushForWrite);
    }
    
    if (Fcb->SectionObject.DataSectionObject) {

        RfsdPrint((DBG_INFO, "RfsdPurgeFile: CcPurgeCacheSection on %s.\n",
                             Fcb->AnsiFileName.Buffer));

        CcPurgeCacheSection(&Fcb->SectionObject, NULL, 0, FALSE);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
RfsdFileSystemControl (IN PRFSD_IRP_CONTEXT IrpContext)
{
    NTSTATUS    Status;
    
    ASSERT(IrpContext);
    
    ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
        (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
    
    switch (IrpContext->MinorFunction) {

        case IRP_MN_USER_FS_REQUEST:
            Status = RfsdUserFsRequest(IrpContext);
            break;
        
        case IRP_MN_MOUNT_VOLUME:
            Status = RfsdMountVolume(IrpContext);
            break;
        
        case IRP_MN_VERIFY_VOLUME:
            Status = RfsdVerifyVolume(IrpContext);
            break;
        
        default:

            RfsdPrint((DBG_ERROR, "RfsdFilsSystemControl: Invalid Device Request.\n"));
            Status = STATUS_INVALID_DEVICE_REQUEST;
            RfsdCompleteIrpContext(IrpContext,  Status);
    }
    
    return Status;
}

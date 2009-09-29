#include <ntifs.h>
#include <ntdddisk.h>
#include <debug.h>
#include <pseh/pseh2.h>

#include "fullfat.h"

#include <fat.h>
#include <fatstruc.h>

#define Add2Ptr(P,I,T) ((T)((PUCHAR)(P) + (I)))
#define PtrOffset(B,O) ((ULONG)((ULONG_PTR)(O) - (ULONG_PTR)(B)))

#define TAG_CCB  'BCCV'
#define TAG_FCB  'BCFV'
#define TAG_IRP  'PRIV'
#define TAG_VFAT 'TAFV'


/* Global resource acquire/release */
#define FatAcquireExclusiveGlobal(IrpContext) \
( \
    ExAcquireResourceExclusiveLite(&FatGlobalData.Resource, \
                                   (IrpContext)->Flags & IRPCONTEXT_CANWAIT) \
)

#define FatAcquireSharedGlobal(IrpContext) \
( \
    ExAcquireResourceSharedLite(&FatGlobalData.Resource, \
                                (IrpContext)->Flags & IRPCONTEXT_CANWAIT) \
)

#define FatReleaseGlobal(IrpContext) \
{ \
    ExReleaseResourceLite(&(FatGlobalData.Resource)); \
}


/*  ------------------------------------------------------  shutdown.c  */

DRIVER_DISPATCH FatShutdown;
NTSTATUS NTAPI
FatShutdown(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  --------------------------------------------------------  volume.c  */

NTSTATUS NTAPI
FatQueryVolumeInfo(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS NTAPI
FatSetVolumeInfo(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  ------------------------------------------------------  blockdev.c  */
NTSTATUS
NTAPI
FatPerformLboIo(
    IN PFAT_IRP_CONTEXT IrpContext,
    IN PLARGE_INTEGER Offset,
    IN SIZE_T Length);

NTSTATUS
FatPerformVirtualNonCachedIo(
    IN PFAT_IRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PLARGE_INTEGER Offset,
    IN SIZE_T Length);

PVOID
FatMapUserBuffer(
    IN OUT PIRP Irp);

/*  -----------------------------------------------------------  dir.c  */

NTSTATUS NTAPI
FatDirectoryControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

VOID NTAPI
FatCreateRootDcb(IN PFAT_IRP_CONTEXT IrpContext,
                 IN PVCB Vcb);

/*  --------------------------------------------------------  create.c  */

NTSTATUS NTAPI
FatCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);


/*  ---------------------------------------------------------  close.c  */

NTSTATUS NTAPI
FatClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  -------------------------------------------------------  cleanup.c  */

NTSTATUS NTAPI
FatCleanup(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  ---------------------------------------------------------  fastio.c  */

VOID
FatInitFastIoRoutines(PFAST_IO_DISPATCH FastIoDispatch);

BOOLEAN NTAPI
FatAcquireForLazyWrite(IN PVOID Context,
                        IN BOOLEAN Wait);

VOID NTAPI
FatReleaseFromLazyWrite(IN PVOID Context);

BOOLEAN NTAPI
FatAcquireForReadAhead(IN PVOID Context,
                        IN BOOLEAN Wait);

VOID NTAPI
FatReleaseFromReadAhead(IN PVOID Context);

BOOLEAN NTAPI
FatNoopAcquire(IN PVOID Context,
               IN BOOLEAN Wait);

VOID NTAPI
FatNoopRelease(IN PVOID Context);

/* ---------------------------------------------------------  fastfat.c */

PFAT_IRP_CONTEXT NTAPI
FatBuildIrpContext(PIRP Irp, BOOLEAN CanWait);

VOID NTAPI
FatDestroyIrpContext(PFAT_IRP_CONTEXT IrpContext);

VOID
NTAPI
FatQueueRequest(IN PFAT_IRP_CONTEXT IrpContext,
                IN PFAT_OPERATION_HANDLER OperationHandler);

VOID NTAPI
FatCompleteRequest(PFAT_IRP_CONTEXT IrpContext OPTIONAL,
                   PIRP Irp OPTIONAL,
                   NTSTATUS Status);

BOOLEAN NTAPI
FatAcquireExclusiveVcb(IN PFAT_IRP_CONTEXT IrpContext,
                       IN PVCB Vcb);

VOID NTAPI
FatReleaseVcb(IN PFAT_IRP_CONTEXT IrpContext,
              IN PVCB Vcb);

TYPE_OF_OPEN
NTAPI
FatDecodeFileObject(IN PFILE_OBJECT FileObject,
                    OUT PVCB *Vcb,
                    OUT PFCB *FcbOrDcb,
                    OUT PCCB *Ccb);

/* --------------------------------------------------------- fullfat.c */

FF_T_SINT32
FatWriteBlocks(FF_T_UINT8 *pBuffer, FF_T_UINT32 SectorAddress, FF_T_UINT32 Count, void *pParam);

FF_T_SINT32
FatReadBlocks(FF_T_UINT8 *pBuffer, FF_T_UINT32 SectorAddress, FF_T_UINT32 Count, void *pParam);

/* ---------------------------------------------------------  lock.c */

NTSTATUS NTAPI
FatLockControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  ---------------------------------------------------------  fsctl.c  */

NTSTATUS NTAPI
FatFileSystemControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  ---------------------------------------------------------  finfo.c  */

NTSTATUS NTAPI FatQueryInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS NTAPI FatSetInformation(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  ---------------------------------------------------------  iface.c  */

NTSTATUS
NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);

/*  -----------------------------------------------------------  fat.c  */
PVOID
FatPinPage(
    PFAT_PAGE_CONTEXT Context,
    LONGLONG ByteOffset);

PVOID
FatPinNextPage(
    PFAT_PAGE_CONTEXT Context);

NTSTATUS
FatInitializeVcb(
    IN PFAT_IRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb);

VOID
FatUninitializeVcb(
    IN PVCB Vcb);

ULONG
FatScanFat(
    IN PFCB Fcb,
    IN LONGLONG Vbo, OUT PLONGLONG Lbo,
    IN OUT PLONGLONG Length,
    OUT PULONG Index,
    IN BOOLEAN CanWait);

/*  ------------------------------------------------------  device.c  */

NTSTATUS NTAPI
FatDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS
FatPerformDevIoCtrl(PDEVICE_OBJECT DeviceObject,
                    ULONG ControlCode,
                    PVOID InputBuffer,
                    ULONG InputBufferSize,
                    PVOID OutputBuffer,
                    ULONG OutputBufferSize,
                    BOOLEAN Override);

/*  ------------------------------------------------------  direntry.c  */
VOID
FatFindDirent(IN OUT PFAT_FIND_DIRENT_CONTEXT Context,
              OUT PDIR_ENTRY* Dirent,
              OUT PUNICODE_STRING LongFileName OPTIONAL);

VOID
FatEnumerateDirents(IN OUT PFAT_ENUM_DIRENT_CONTEXT Context,
                    IN SIZE_T Offset);

VOID
FatQueryFileTimes(OUT PLARGE_INTEGER FileTimes,
                  IN PDIR_ENTRY Dirent);

/*  -----------------------------------------------------------  fcb.c  */
PFCB
FatLookupFcbByName(
	IN PFCB ParentFcb,
	IN PUNICODE_STRING Name);

BOOLEAN
FatLinkFcbNames(
	IN PFCB ParentFcb,
	IN PFCB Fcb);

VOID
FatUnlinkFcbNames(
	IN PFCB ParentFcb,
	IN PFCB Fcb);

NTSTATUS
FatCreateFcb(
    OUT PFCB* CreatedFcb,
    IN PFAT_IRP_CONTEXT IrpContext,
    IN PFCB ParentFcb,
	IN PDIR_ENTRY Dirent,
    IN PUNICODE_STRING FileName,
	IN PUNICODE_STRING LongFileName OPTIONAL);

NTSTATUS
FatOpenFcb(
    OUT PFCB* Fcb,
    IN PFAT_IRP_CONTEXT IrpContext,
    IN PFCB ParentFcb,
    IN PUNICODE_STRING FileName);

/*  ------------------------------------------------------------  rw.c  */

NTSTATUS NTAPI
FatRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS NTAPI
FatWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);

/*  ------------------------------------------------------------- flush.c  */

NTSTATUS NTAPI
FatFlushBuffers(PDEVICE_OBJECT DeviceObject, PIRP Irp);


/* EOF */

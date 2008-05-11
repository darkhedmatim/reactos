

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

//
// RfsdBlock.c
//

// TODO move allocate and load block here

NTSTATUS
RfsdFindItemHeaderInBlock(
	 IN PRFSD_VCB			Vcb,
	 IN PRFSD_KEY_IN_MEMORY	pKey,						// The key to match against
	 IN PUCHAR				pBlockBuffer,				// A filled disk block, provided by the caller
	 OUT PRFSD_ITEM_HEAD*	ppTargetItemHeader,			// A pointer to a PRFSD_ITEM_HEAD.  The PRFSD_ITEM_HEAD will point to the item head matching Key, or NULL if there was no such item head in the given block.
	 IN	RFSD_KEY_COMPARISON (*fpComparisonFunction)(PRFSD_KEY_IN_MEMORY, PRFSD_KEY_IN_MEMORY)
	 );

NTSTATUS
RfsdLoadItem(
	IN	  PRFSD_VCB				Vcb,
	IN   PRFSD_KEY_IN_MEMORY	pItemKey,					// The key of the item to find
	OUT  PRFSD_ITEM_HEAD*		ppMatchingItemHeader,
	OUT  PUCHAR*				ppItemBuffer,				
	OUT  PUCHAR*				ppBlockBuffer,				// Block buffer, which backs the other output data structures.  The caller must free this (even in the case of an error)!
	OUT	 PULONG					pBlockNumber,				// The ordinal disk block number at which the item was found
	IN	RFSD_KEY_COMPARISON (*fpComparisonFunction)(PRFSD_KEY_IN_MEMORY, PRFSD_KEY_IN_MEMORY)
	);
//
// Block.c
//

NTSTATUS
RfsdLockUserBuffer (
        IN PIRP             Irp,
        IN ULONG            Length,
        IN LOCK_OPERATION   Operation);
PVOID
RfsdGetUserBuffer (IN PIRP Irp);


NTSTATUS
RfsdReadWriteBlocks(
        IN PRFSD_IRP_CONTEXT IrpContext,
        IN PRFSD_VCB        Vcb,
        IN PRFSD_BDL        RfsdBDL,
        IN ULONG            Length,
        IN ULONG            Count,
        IN BOOLEAN          bVerify );

PUCHAR
RfsdAllocateAndLoadBlock(
	IN	PRFSD_VCB			Vcb,
	IN	ULONG				BlockIndex );

NTSTATUS
RfsdReadSync(
        IN PRFSD_VCB        Vcb,
        IN ULONGLONG        Offset,
        IN ULONG            Length,
        OUT PVOID           Buffer,
        IN BOOLEAN          bVerify );

NTSTATUS
RfsdReadDisk(
         IN PRFSD_VCB       Vcb,
         IN ULONGLONG       Offset,
         IN ULONG           Size,
         IN PVOID           Buffer,
         IN BOOLEAN         bVerify  );

NTSTATUS 
RfsdDiskIoControl (
        IN PDEVICE_OBJECT   DeviceOjbect,
        IN ULONG            IoctlCode,
        IN PVOID            InputBuffer,
        IN ULONG            InputBufferSize,
        IN OUT PVOID        OutputBuffer,
        IN OUT PULONG       OutputBufferSize );

VOID
RfsdMediaEjectControl (
        IN PRFSD_IRP_CONTEXT IrpContext,
        IN PRFSD_VCB Vcb,
        IN BOOLEAN bPrevent );

NTSTATUS
RfsdDiskShutDown(PRFSD_VCB Vcb);


//
// Cleanup.c
//
NTSTATUS
RfsdCleanup (IN PRFSD_IRP_CONTEXT IrpContext);

//
// Close.c
//
NTSTATUS
RfsdClose (IN PRFSD_IRP_CONTEXT IrpContext);

VOID
RfsdQueueCloseRequest (IN PRFSD_IRP_CONTEXT IrpContext);

VOID
RfsdDeQueueCloseRequest (IN PVOID Context);

//
// Cmcb.c
//

BOOLEAN
RfsdAcquireForLazyWrite (
        IN PVOID    Context,
        IN BOOLEAN  Wait );
VOID
RfsdReleaseFromLazyWrite (IN PVOID Context);

BOOLEAN
RfsdAcquireForReadAhead (
        IN PVOID    Context,
        IN BOOLEAN  Wait );

BOOLEAN
RfsdNoOpAcquire (
        IN PVOID Fcb,
        IN BOOLEAN Wait );

VOID
RfsdNoOpRelease (IN PVOID Fcb    );

VOID
RfsdReleaseFromReadAhead (IN PVOID Context);

//
// Create.c
//

PRFSD_FCB
RfsdSearchFcbList(
        IN PRFSD_VCB    Vcb,
        IN ULONG        inode);

NTSTATUS
RfsdScanDir (IN PRFSD_VCB       Vcb,
         IN PRFSD_MCB           ParentMcb,				// Mcb of the directory to be scanned
         IN PUNICODE_STRING     FileName,				// Short file name (not necisarilly null-terminated!)
         IN OUT PULONG          Index,					// Offset (in bytes) of the dentry relative to the start of the directory listing
         IN OUT PRFSD_DENTRY_HEAD rfsd_dir);			// Directory entry of the found item

NTSTATUS
RfsdLookupFileName (
        IN PRFSD_VCB            Vcb,
        IN PUNICODE_STRING      FullFileName,
        IN PRFSD_MCB            ParentMcb,
        OUT PRFSD_MCB *         RfsdMcb,
        IN OUT PRFSD_INODE      Inode);

NTSTATUS
RfsdCreateFile(
        IN PRFSD_IRP_CONTEXT IrpContext,
        IN PRFSD_VCB Vcb );

NTSTATUS
RfsdCreateVolume(
        IN PRFSD_IRP_CONTEXT IrpContext, 
        IN PRFSD_VCB Vcb );

NTSTATUS
RfsdCreate (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdCreateInode(
        IN PRFSD_IRP_CONTEXT   IrpContext,
        IN PRFSD_VCB           Vcb,
        IN PRFSD_FCB           pParentFcb,
        IN ULONG               Type,
        IN ULONG               FileAttr,
        IN PUNICODE_STRING     FileName);

#if DISABLED
NTSTATUS
RfsdSupersedeOrOverWriteFile(
        IN PRFSD_IRP_CONTEXT IrpContext,
        IN PRFSD_VCB Vcb,
        IN PRFSD_FCB Fcb,
        IN ULONG     Disposition);
#endif

//
// Debug.c
//

#define DBG_VITAL 0
#define DBG_ERROR 1
#define DBG_USER  2
#define DBG_TRACE 3
#define DBG_INFO  4
#define DBG_FUNC  5

#if DBG
#define RfsdPrint(arg)          RfsdPrintf   arg
#define RfsdPrintNoIndent(arg)  RfsdNIPrintf arg

#define RfsdCompleteRequest(Irp, bPrint, PriorityBoost) \
        RfsdDbgPrintComplete(Irp, bPrint); \
        IoCompleteRequest(Irp, PriorityBoost)

#else

#define RfsdPrint(arg)

#define RfsdCompleteRequest(Irp, bPrint, PriorityBoost) \
        IoCompleteRequest(Irp, PriorityBoost)

#endif // DBG

VOID
__cdecl
RfsdPrintf(
    LONG  DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    );

VOID
__cdecl
RfsdNIPrintf(
    LONG  DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    );

extern ULONG ProcessNameOffset;

#define RfsdGetCurrentProcessName() ( \
    (PUCHAR) PsGetCurrentProcess() + ProcessNameOffset \
)

ULONG 
RfsdGetProcessNameOffset (VOID);

VOID
RfsdDbgPrintCall (
        IN PDEVICE_OBJECT   DeviceObject,
        IN PIRP             Irp );

VOID
RfsdDbgPrintComplete (
        IN PIRP Irp,
        IN BOOLEAN bPrint
        );

PUCHAR
RfsdNtStatusToString (IN NTSTATUS Status );

//
// Devctl.c
//

NTSTATUS
RfsdDeviceControlNormal (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdPrepareToUnload (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdDeviceControl (IN PRFSD_IRP_CONTEXT IrpContext);

//
// Dirctl.c
//

ULONG
RfsdGetInfoLength(IN FILE_INFORMATION_CLASS  FileInformationClass);

ULONG
RfsdProcessDirEntry(
			IN PRFSD_VCB         Vcb,
            IN FILE_INFORMATION_CLASS  FileInformationClass,
            IN __u32		 Key_ParentID,
			IN __u32		 Key_ObjectID,
            IN PVOID         Buffer,
            IN ULONG         UsedLength,
            IN ULONG         Length,
            IN ULONG         FileIndex,
            IN PUNICODE_STRING   pName,
            IN BOOLEAN       Single,
			IN PVOID		 pPreviousEntry	);

NTSTATUS
RfsdQueryDirectory (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdNotifyChangeDirectory (
        IN PRFSD_IRP_CONTEXT IrpContext
        );

VOID
RfsdNotifyReportChange (
        IN PRFSD_IRP_CONTEXT IrpContext,
        IN PRFSD_VCB         Vcb,
        IN PRFSD_FCB         Fcb,
        IN ULONG             Filter,
        IN ULONG             Action
        );

NTSTATUS
RfsdDirectoryControl (IN PRFSD_IRP_CONTEXT IrpContext);

BOOLEAN
RfsdIsDirectoryEmpty (
        IN PRFSD_VCB Vcb,
        IN PRFSD_FCB Fcb
        );

//
// Dispatch.c
//

NTSTATUS
RfsdQueueRequest (IN PRFSD_IRP_CONTEXT IrpContext);

VOID
RfsdDeQueueRequest (IN PVOID Context);

NTSTATUS
RfsdDispatchRequest (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdBuildRequest (
        IN PDEVICE_OBJECT   DeviceObject,
        IN PIRP             Irp
        );

//
// Except.c
//

NTSTATUS
RfsdExceptionFilter (
        IN PRFSD_IRP_CONTEXT    IrpContext,
        IN PEXCEPTION_POINTERS ExceptionPointer
        );

NTSTATUS
RfsdExceptionHandler (IN PRFSD_IRP_CONTEXT IrpContext);


//
// Rfsd.c
//

PRFSD_SUPER_BLOCK
RfsdLoadSuper(
        IN PRFSD_VCB Vcb,
        IN BOOLEAN   bVerify
        );

BOOLEAN
RfsdSaveSuper(
        IN PRFSD_IRP_CONTEXT    IrpContext,
        IN PRFSD_VCB            Vcb
        );

BOOLEAN
RfsdLoadGroup(IN PRFSD_VCB Vcb);

BOOLEAN
RfsdSaveGroup(
        IN PRFSD_IRP_CONTEXT    IrpContext,
        IN PRFSD_VCB            Vcb,
        IN ULONG                Group
        );

BOOLEAN
RfsdGetInodeLba (IN PRFSD_VCB   Vcb,
         IN __u32 DirectoryID,
		 IN __u32 ParentID,
         OUT PLONGLONG offset);

BOOLEAN
RfsdLoadInode (IN PRFSD_VCB Vcb,
			   IN PRFSD_KEY_IN_MEMORY pKey,
			   IN OUT PRFSD_INODE Inode);

BOOLEAN
RfsdLoadInode2 (IN PRFSD_VCB Vcb,
			   IN __u32 a,
			   IN __u32 b,
			   IN OUT PRFSD_INODE Inode);
BOOLEAN
RfsdSaveInode (
        IN PRFSD_IRP_CONTEXT IrpContext,
        IN PRFSD_VCB Vcb,
        IN ULONG inode,
        IN PRFSD_INODE Inode
        );

BOOLEAN
RfsdLoadBlock (
        IN PRFSD_VCB Vcb,
        IN ULONG     dwBlk,
        IN PVOID     Buffer
        );

BOOLEAN
RfsdSaveBlock (
        IN PRFSD_IRP_CONTEXT    IrpContext,
        IN PRFSD_VCB            Vcb,
        IN ULONG                dwBlk,
        IN PVOID                Buf
        );

BOOLEAN
RfsdSaveBuffer(
        IN PRFSD_IRP_CONTEXT    IrpContext,
        IN PRFSD_VCB            Vcb,
        IN LONGLONG             Offset,
        IN ULONG                Size,
        IN PVOID                Buf
        );

NTSTATUS
RfsdGetBlock(
    IN PRFSD_IRP_CONTEXT    IrpContext,
    IN PRFSD_VCB            Vcb,
    IN ULONG                dwContent,
    IN ULONG                Index,
    IN ULONG                Layer,
    IN BOOLEAN              bAlloc,
    OUT PULONG              pBlock
    );

NTSTATUS
RfsdBlockMap(
    IN PRFSD_IRP_CONTEXT    IrpContext,
    IN PRFSD_VCB            Vcb,
    IN ULONG                InodeNo,
    IN PRFSD_INODE          Inode,
    IN ULONG                Index,
    IN BOOLEAN              bAlloc,
    OUT PULONG              pBlock
    );

NTSTATUS
RfsdBuildBDL2(	
	IN  PRFSD_VCB				Vcb,
	IN  PRFSD_KEY_IN_MEMORY		pKey,
	IN	PRFSD_INODE				pInode,
	OUT	PULONG					out_Count,
	OUT PRFSD_BDL*				out_ppBdl  );

NTSTATUS
RfsdBuildBDL( 
    IN PRFSD_IRP_CONTEXT    IrpContext,
    IN PRFSD_VCB            Vcb,
    IN PRFSD_KEY_IN_MEMORY  InodeNo,
    IN PRFSD_INODE          Inode,
    IN ULONGLONG            Offset, 
    IN ULONG                Size, 
    IN BOOLEAN              bAlloc,
    OUT PRFSD_BDL *         Bdls,
    OUT PULONG              Count
    );

NTSTATUS
RfsdNewBlock(
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB Vcb,
    ULONG     GroupHint,
    ULONG     BlockHint,  
    PULONG    dwRet );

NTSTATUS
RfsdFreeBlock(
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB Vcb,
    ULONG     Block );

NTSTATUS
RfsdExpandBlock(
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB Vcb,
    PRFSD_FCB   Fcb,
    ULONG dwContent,
    ULONG Index,
    ULONG layer,
    BOOLEAN bNew,
    ULONG *dwRet );


NTSTATUS
RfsdExpandInode(
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB Vcb,
    PRFSD_FCB Fcb,
    ULONG *dwRet );

NTSTATUS
RfsdNewInode(
            PRFSD_IRP_CONTEXT IrpContext,
            PRFSD_VCB Vcb,
            ULONG GroupHint,
            ULONG mode,
            PULONG Inode );

BOOLEAN
RfsdFreeInode(
            PRFSD_IRP_CONTEXT IrpContext,
            PRFSD_VCB Vcb,
            ULONG Inode,
            ULONG Type );

NTSTATUS
RfsdAddEntry (
         IN PRFSD_IRP_CONTEXT   IrpContext,
         IN PRFSD_VCB           Vcb,
         IN PRFSD_FCB           Dcb,
         IN ULONG               FileType,
         IN ULONG               Inode,
         IN PUNICODE_STRING     FileName );

NTSTATUS
RfsdRemoveEntry (
         IN PRFSD_IRP_CONTEXT   IrpContext,
         IN PRFSD_VCB           Vcb,
         IN PRFSD_FCB           Dcb,
         IN ULONG               FileType,
         IN ULONG               Inode );

NTSTATUS
RfsdSetParentEntry (
         IN PRFSD_IRP_CONTEXT   IrpContext,
         IN PRFSD_VCB           Vcb,
         IN PRFSD_FCB           Dcb,
         IN ULONG               OldParent,
         IN ULONG               NewParent );


NTSTATUS
RfsdTruncateBlock(
         IN PRFSD_IRP_CONTEXT IrpContext,
         IN PRFSD_VCB Vcb,
         IN PRFSD_FCB Fcb,
         IN ULONG   dwContent,
         IN ULONG   Index,
         IN ULONG   layer,
         OUT BOOLEAN *bFreed );

NTSTATUS
RfsdTruncateInode(
         IN PRFSD_IRP_CONTEXT IrpContext,
         IN PRFSD_VCB   Vcb,
         IN PRFSD_FCB   Fcb );

BOOLEAN
RfsdAddMcbEntry (
    IN PRFSD_VCB Vcb,
    IN LONGLONG  Lba,
    IN LONGLONG  Length );

VOID
RfsdRemoveMcbEntry (
    IN PRFSD_VCB Vcb,
    IN LONGLONG  Lba,
    IN LONGLONG  Length );

BOOLEAN
RfsdLookupMcbEntry (
    IN PRFSD_VCB    Vcb,
    IN LONGLONG     Offset,
    OUT PLONGLONG   Lba OPTIONAL,
    OUT PLONGLONG   Length OPTIONAL,
    OUT PLONGLONG   RunStart OPTIONAL,
    OUT PLONGLONG   RunLength OPTIONAL,
    OUT PULONG      Index OPTIONAL );

BOOLEAN
SuperblockContainsMagicKey(PRFSD_SUPER_BLOCK sb);

__u32
ConvertKeyTypeUniqueness(__u32 k_uniqueness);

void
FillInMemoryKey(
	IN		PRFSD_KEY_ON_DISK		pKeyOnDisk, 
	IN		RFSD_KEY_VERSION		KeyVersion, 
	IN OUT	PRFSD_KEY_IN_MEMORY		pKeyInMemory );

RFSD_KEY_VERSION DetermineOnDiskKeyFormat(const PRFSD_KEY_ON_DISK key);

RFSD_KEY_COMPARISON
CompareShortKeys(
	IN		PRFSD_KEY_IN_MEMORY		a,
	IN		PRFSD_KEY_IN_MEMORY		b		);

RFSD_KEY_COMPARISON
CompareKeysWithoutOffset(
	IN		PRFSD_KEY_IN_MEMORY		a,
	IN		PRFSD_KEY_IN_MEMORY		b		);

RFSD_KEY_COMPARISON
CompareKeys(
	IN		PRFSD_KEY_IN_MEMORY		a,
	IN		PRFSD_KEY_IN_MEMORY		b	);

NTSTATUS
NavigateToLeafNode(
	IN	PRFSD_VCB					Vcb,
	IN	PRFSD_KEY_IN_MEMORY			Key,				
	IN	ULONG						StartingBlockNumber,	
	OUT	PULONG						out_NextBlockNumber );

NTSTATUS
RfsdParseFilesystemTree(
			IN	PRFSD_VCB					Vcb,
			IN	PRFSD_KEY_IN_MEMORY			Key,						// Key to search for.
			IN	ULONG						StartingBlockNumber,		// Block number of an internal or leaf node, to start the search from			
			IN	RFSD_CALLBACK(fpDirectoryCallback),					// A function ptr to trigger on hitting a matching leaf block
			IN  PVOID						Context
			);


NTSTATUS
_NavigateToLeafNode(
	IN	PRFSD_VCB					Vcb,
	IN	PRFSD_KEY_IN_MEMORY			Key,				
	IN	ULONG						StartingBlockNumber,	
	OUT	PULONG						out_NextBlockNumber,
	IN	BOOLEAN						ReturnOnFirstMatch,
	IN	RFSD_KEY_COMPARISON			(*fpComparisonFunction)(PRFSD_KEY_IN_MEMORY, PRFSD_KEY_IN_MEMORY),
	RFSD_CALLBACK(fpDirectoryCallback),
	IN	PVOID						pContext					
	);


//
// Fastio.c
//

BOOLEAN
RfsdFastIoCheckIfPossible (
              IN PFILE_OBJECT         FileObject,
              IN PLARGE_INTEGER       FileOffset,
              IN ULONG                Length,
              IN BOOLEAN              Wait,
              IN ULONG                LockKey,
              IN BOOLEAN              CheckForReadOperation,
              OUT PIO_STATUS_BLOCK    IoStatus,
              IN PDEVICE_OBJECT       DeviceObject );


BOOLEAN
RfsdFastIoRead (IN PFILE_OBJECT FileObject,
        IN PLARGE_INTEGER       FileOffset,
        IN ULONG                Length,
        IN BOOLEAN              Wait,
        IN ULONG                LockKey,
        OUT PVOID               Buffer,
        OUT PIO_STATUS_BLOCK    IoStatus,
        IN PDEVICE_OBJECT       DeviceObject);

BOOLEAN
RfsdFastIoWrite (
        IN PFILE_OBJECT         FileObject,
        IN PLARGE_INTEGER       FileOffset,
        IN ULONG                Length,
        IN BOOLEAN              Wait,
        IN ULONG                LockKey,
        OUT PVOID               Buffer,
        OUT PIO_STATUS_BLOCK    IoStatus,
        IN PDEVICE_OBJECT       DeviceObject);

BOOLEAN
RfsdFastIoQueryBasicInfo (
              IN PFILE_OBJECT             FileObject,
              IN BOOLEAN                  Wait,
              OUT PFILE_BASIC_INFORMATION Buffer,
              OUT PIO_STATUS_BLOCK        IoStatus,
              IN PDEVICE_OBJECT           DeviceObject);

BOOLEAN
RfsdFastIoQueryStandardInfo (
                 IN PFILE_OBJECT                 FileObject,
                 IN BOOLEAN                      Wait,
                 OUT PFILE_STANDARD_INFORMATION  Buffer,
                 OUT PIO_STATUS_BLOCK            IoStatus,
                 IN PDEVICE_OBJECT               DeviceObject);

BOOLEAN
RfsdFastIoLock (
           IN PFILE_OBJECT         FileObject,
           IN PLARGE_INTEGER       FileOffset,
           IN PLARGE_INTEGER       Length,
           IN PEPROCESS            Process,
           IN ULONG                Key,
           IN BOOLEAN              FailImmediately,
           IN BOOLEAN              ExclusiveLock,
           OUT PIO_STATUS_BLOCK    IoStatus,
           IN PDEVICE_OBJECT       DeviceObject
           );

BOOLEAN
RfsdFastIoUnlockSingle (
               IN PFILE_OBJECT         FileObject,
               IN PLARGE_INTEGER       FileOffset,
               IN PLARGE_INTEGER       Length,
               IN PEPROCESS            Process,
               IN ULONG                Key,
               OUT PIO_STATUS_BLOCK    IoStatus,
               IN PDEVICE_OBJECT       DeviceObject
               );

BOOLEAN
RfsdFastIoUnlockAll (
            IN PFILE_OBJECT         FileObject,
            IN PEPROCESS            Process,
            OUT PIO_STATUS_BLOCK    IoStatus,
            IN PDEVICE_OBJECT       DeviceObject
            );

BOOLEAN
RfsdFastIoUnlockAllByKey (
             IN PFILE_OBJECT         FileObject,
             IN PEPROCESS            Process,
             IN ULONG                Key,
             OUT PIO_STATUS_BLOCK    IoStatus,
             IN PDEVICE_OBJECT       DeviceObject
             );


BOOLEAN
RfsdFastIoQueryNetworkOpenInfo (
     IN PFILE_OBJECT                     FileObject,
     IN BOOLEAN                          Wait,
     OUT PFILE_NETWORK_OPEN_INFORMATION  Buffer,
     OUT PIO_STATUS_BLOCK                IoStatus,
     IN PDEVICE_OBJECT                   DeviceObject );

BOOLEAN
RfsdFastIoQueryNetworkOpenInfo (
                IN PFILE_OBJECT                     FileObject,
                IN BOOLEAN                          Wait,
                OUT PFILE_NETWORK_OPEN_INFORMATION  Buffer,
                OUT PIO_STATUS_BLOCK                IoStatus,
                IN PDEVICE_OBJECT                   DeviceObject);


//
// FileInfo.c
//


NTSTATUS
RfsdQueryInformation (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdSetInformation (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdExpandFile (
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB Vcb,
    PRFSD_FCB Fcb,
    PLARGE_INTEGER AllocationSize );

NTSTATUS
RfsdTruncateFile (
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB Vcb,
    PRFSD_FCB Fcb,
    PLARGE_INTEGER AllocationSize );

NTSTATUS
RfsdSetDispositionInfo(
            PRFSD_IRP_CONTEXT IrpContext,
            PRFSD_VCB Vcb,
            PRFSD_FCB Fcb,
            BOOLEAN bDelete);

NTSTATUS
RfsdSetRenameInfo(
            PRFSD_IRP_CONTEXT IrpContext,
            PRFSD_VCB Vcb,
            PRFSD_FCB Fcb );

NTSTATUS
RfsdDeleteFile(
        PRFSD_IRP_CONTEXT IrpContext,
        PRFSD_VCB Vcb,
        PRFSD_FCB Fcb );


//
// Flush.c
//

NTSTATUS
RfsdFlushFiles (IN PRFSD_VCB Vcb, BOOLEAN bShutDown);

NTSTATUS
RfsdFlushVolume (IN PRFSD_VCB Vcb, BOOLEAN bShutDown);

NTSTATUS
RfsdFlushFile (IN PRFSD_FCB Fcb);

NTSTATUS
RfsdFlush (IN PRFSD_IRP_CONTEXT IrpContext);


//
// Fsctl.c
//


VOID
RfsdSetVpbFlag (IN PVPB     Vpb,
        IN USHORT   Flag );

VOID
RfsdClearVpbFlag (IN PVPB     Vpb,
          IN USHORT   Flag );

BOOLEAN
RfsdCheckDismount (
    IN PRFSD_IRP_CONTEXT IrpContext,
    IN PRFSD_VCB         Vcb,
    IN BOOLEAN           bForce   );

NTSTATUS
RfsdPurgeVolume (IN PRFSD_VCB Vcb,
         IN BOOLEAN  FlushBeforePurge);

NTSTATUS
RfsdPurgeFile (IN PRFSD_FCB Fcb,
           IN BOOLEAN  FlushBeforePurge);

BOOLEAN
RfsdIsHandleCountZero(IN PRFSD_VCB Vcb);

NTSTATUS
RfsdLockVcb (IN PRFSD_VCB    Vcb,
             IN PFILE_OBJECT FileObject);

NTSTATUS
RfsdLockVolume (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdUnlockVcb (IN PRFSD_VCB    Vcb,
               IN PFILE_OBJECT FileObject);

NTSTATUS
RfsdUnlockVolume (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdAllowExtendedDasdIo(IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdUserFsRequest (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdMountVolume (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdVerifyVolume (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdIsVolumeMounted (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdDismountVolume (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdFileSystemControl (IN PRFSD_IRP_CONTEXT IrpContext);


//
// Init.c
//

BOOLEAN
RfsdQueryParameters (IN PUNICODE_STRING  RegistryPath); 

VOID
DriverUnload (IN PDRIVER_OBJECT DriverObject);


//
// Lock.c
//

NTSTATUS
RfsdLockControl (IN PRFSD_IRP_CONTEXT IrpContext);

//
// Memory.c
//

PRFSD_IRP_CONTEXT
RfsdAllocateIrpContext (IN PDEVICE_OBJECT   DeviceObject,
            IN PIRP             Irp );

VOID
RfsdFreeIrpContext (IN PRFSD_IRP_CONTEXT IrpContext);


PRFSD_FCB
RfsdAllocateFcb (IN PRFSD_VCB   Vcb,
         IN PRFSD_MCB           RfsdMcb,
         IN PRFSD_INODE         Inode );

VOID
RfsdFreeFcb (IN PRFSD_FCB Fcb);

PRFSD_CCB
RfsdAllocateCcb (VOID);

VOID
RfsdFreeMcb (IN PRFSD_MCB Mcb);

PRFSD_FCB
RfsdCreateFcbFromMcb(PRFSD_VCB Vcb, PRFSD_MCB Mcb);

VOID
RfsdFreeCcb (IN PRFSD_CCB Ccb);

PRFSD_MCB
RfsdAllocateMcb ( PRFSD_VCB,
                  PUNICODE_STRING FileName,
                  ULONG FileAttr);

PRFSD_MCB
RfsdSearchMcbTree(  PRFSD_VCB Vcb,
                    PRFSD_MCB RfsdMcb,
                    PRFSD_KEY_IN_MEMORY Key);

PRFSD_MCB
RfsdSearchMcb(  PRFSD_VCB Vcb, PRFSD_MCB Parent,
                PUNICODE_STRING FileName);

BOOLEAN
RfsdGetFullFileName( PRFSD_MCB Mcb, 
                     PUNICODE_STRING FileName);

VOID
RfsdRefreshMcb(PRFSD_VCB Vcb, PRFSD_MCB Mcb);

VOID
RfsdAddMcbNode( PRFSD_VCB Vcb,
                PRFSD_MCB Parent,
                PRFSD_MCB Child );

BOOLEAN
RfsdDeleteMcbNode(
                PRFSD_VCB Vcb, 
                PRFSD_MCB McbTree,
                PRFSD_MCB RfsdMcb);

VOID
RfsdFreeMcbTree(PRFSD_MCB McbTree);

#if DISABLED
BOOLEAN
RfsdCheckSetBlock( PRFSD_IRP_CONTEXT IrpContext,
                   PRFSD_VCB Vcb, ULONG Block);
#endif

#if DISABLED
BOOLEAN
RfsdCheckBitmapConsistency( PRFSD_IRP_CONTEXT IrpContext,
                            PRFSD_VCB Vcb);
#endif

VOID
RfsdInsertVcb(PRFSD_VCB Vcb);

VOID
RfsdRemoveVcb(PRFSD_VCB Vcb);

NTSTATUS
RfsdInitializeVcb(
            PRFSD_IRP_CONTEXT IrpContext, 
            PRFSD_VCB Vcb, 
            PRFSD_SUPER_BLOCK RfsdSb,
            PDEVICE_OBJECT TargetDevice,
            PDEVICE_OBJECT VolumeDevice,
            PVPB Vpb                   );

VOID
RfsdFreeVcb (IN PRFSD_VCB Vcb );


VOID
RfsdRepinBcb (
    IN PRFSD_IRP_CONTEXT IrpContext,
    IN PBCB Bcb );

VOID
RfsdUnpinRepinnedBcbs (
    IN PRFSD_IRP_CONTEXT IrpContext);


NTSTATUS
RfsdCompleteIrpContext (
    IN PRFSD_IRP_CONTEXT IrpContext,
    IN NTSTATUS Status );

VOID
RfsdSyncUninitializeCacheMap (
    IN PFILE_OBJECT FileObject    );

//
// Misc.c
//

/** Returns the length of a string (not including a terminating null), or MaximumLength if no terminator is found within MaximumLength characters. */
static inline USHORT RfsdStringLength(PUCHAR buffer, USHORT MaximumLength)
{
	USHORT i = 0;
	while ((buffer[i] != '\0')  && (i < MaximumLength))  { i++; }
	return i;
}

ULONG
RfsdLog2(ULONG Value);

LARGE_INTEGER
RfsdSysTime (IN ULONG i_time);

ULONG
RfsdInodeTime (IN LARGE_INTEGER SysTime);

ULONG
RfsdOEMToUnicodeSize(
        IN PANSI_STRING Oem
        );

NTSTATUS
RfsdOEMToUnicode(
        IN OUT PUNICODE_STRING Oem,
        IN POEM_STRING         Unicode
        );

ULONG
RfsdUnicodeToOEMSize(
        IN PUNICODE_STRING Unicode
        );

NTSTATUS
RfsdUnicodeToOEM (
        IN OUT POEM_STRING Oem,
        IN PUNICODE_STRING Unicode
        );

//
// nls/nls_rtl.c
//

int
RfsdLoadAllNls();

VOID
RfsdUnloadAllNls();

//
// Pnp.c
//

NTSTATUS
RfsdPnp(IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdPnpQueryRemove(
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB         Vcb      );

NTSTATUS
RfsdPnpRemove(
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB         Vcb      );

NTSTATUS
RfsdPnpCancelRemove(
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB Vcb              );

NTSTATUS
RfsdPnpSurpriseRemove(
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB Vcb              );


//
// Read.c
//

BOOLEAN 
RfsdCopyRead(
    IN PFILE_OBJECT  FileObject,
    IN PLARGE_INTEGER  FileOffset,
    IN ULONG  Length,
    IN BOOLEAN  Wait,
    OUT PVOID  Buffer,
    OUT PIO_STATUS_BLOCK  IoStatus   );


NTSTATUS
RfsdReadInode (
    IN PRFSD_IRP_CONTEXT    IrpContext,
    IN PRFSD_VCB            Vcb,
    IN PRFSD_KEY_IN_MEMORY  Key,
    IN PRFSD_INODE          Inode,
    IN ULONGLONG            Offset,
    IN PVOID                Buffer,
    IN ULONG                Size,
    OUT PULONG              dwReturn
    );

NTSTATUS
RfsdRead (IN PRFSD_IRP_CONTEXT IrpContext);

//
// Shutdown.c
//

NTSTATUS
RfsdShutDown (IN PRFSD_IRP_CONTEXT IrpContext);

//
// Volinfo.c
//

NTSTATUS
RfsdQueryVolumeInformation (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdSetVolumeInformation (IN PRFSD_IRP_CONTEXT IrpContext);


//
// Write.c
//
#if 0
NTSTATUS
RfsdWriteInode (
    IN PRFSD_IRP_CONTEXT    IrpContext,
    IN PRFSD_VCB            Vcb,
    IN ULONG                InodeNo,
    IN PRFSD_INODE          Inode,
    IN ULONGLONG            Offset,
    IN PVOID                Buffer,
    IN ULONG                Size,
    IN BOOLEAN              bWriteToDisk,
    OUT PULONG              dwReturn
    );

VOID
RfsdStartFloppyFlushDpc (
    PRFSD_VCB   Vcb,
    PRFSD_FCB   Fcb,
    PFILE_OBJECT FileObject );

BOOLEAN
RfsdZeroHoles (
    IN PRFSD_IRP_CONTEXT IrpContext,
    IN PRFSD_VCB Vcb,
    IN PFILE_OBJECT FileObject,
    IN LONGLONG Offset,
    IN LONGLONG Count );

NTSTATUS
RfsdWrite (IN PRFSD_IRP_CONTEXT IrpContext);
#endif

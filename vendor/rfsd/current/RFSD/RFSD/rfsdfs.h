/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Rfsd File System Driver for WinNT/2K/XP
 * FILE:             Rfsdfs.h
 * PURPOSE:          Header file: rfsd structures
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://rfsd.yeah.net
 * UPDATE HISTORY: 
 */

#ifndef _RFSD_HEADER_
#define _RFSD_HEADER_

/* INCLUDES *************************************************************/

#include <linux/module.h>

#include "fsparameters.h"

#include <ntdddisk.h>

#pragma pack(1)

/* DEBUG ****************************************************************/
#if DBG
	//Equivalent to DbgBreakPoint()
	#define DbgBreak()   __asm { int 3 }
#else
    #define DbgBreak()
#endif

/* STRUCTS & CONSTS******************************************************/

#define RFSDFSD_VERSION  "0.24"

//
// RfsdFsd build options
//

// To build read-only driver

#define RFSD_READ_ONLY  TRUE


// To support driver dynamics unload

#define RFSD_UNLOAD     TRUE

//
// Constants
//

#define RFSD_BLOCK_TYPES                (0x04)

#define MAXIMUM_RECORD_LENGTH           (0x10000)

#define SECTOR_BITS                     (Vcb->SectorBits)
#define SECTOR_SIZE                     (Vcb->DiskGeometry.BytesPerSector)
#define DEFAULT_SECTOR_SIZE             (0x200)

#define READ_AHEAD_GRANULARITY          (0x10000)

#define SUPER_BLOCK                     (Vcb->SuperBlock)

#define BLOCK_SIZE                      (Vcb->BlockSize)
#define BLOCK_BITS                      (SUPER_BLOCK->s_log_block_size + 10)

#define INODES_COUNT                    (Vcb->SuperBlock->s_inodes_count)

#define INODES_PER_GROUP                (SUPER_BLOCK->s_inodes_per_group)
#define BLOCKS_PER_GROUP                (SUPER_BLOCK->s_blocks_per_group)
#define TOTAL_BLOCKS                    (SUPER_BLOCK->s_blocks_count)

#define RFSD_FIRST_DATA_BLOCK           (SUPER_BLOCK->s_first_data_block)



#define CEILING_ALIGNED(A, B) (((A) + (B) - 1) & (~((B) - 1)))


// The __SLINE__ macro evaluates to a string with the line of the program from which it is called.
// (Note that this requires two levels of macro indirection...)
#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __SLINE__ __STR1__(__LINE__)


/* File System Releated *************************************************/

#define DRIVER_NAME      "RfsdFsd"
#define DEVICE_NAME     L"\\RfsdFsd"

// Registry

#define PARAMETERS_KEY      L"\\Parameters"

#define WRITING_SUPPORT     L"WritingSupport"
#define CHECKING_BITMAP     L"CheckingBitmap"
#define EXT3_FORCEWRITING   L"Ext3ForceWriting"
#define EXT3_CODEPAGE       L"CodePage"

// To support rfsdfsd unload routine
#if RFSD_UNLOAD

#define DOS_DEVICE_NAME L"\\DosDevices\\RfsdFsd"

//
// Private IOCTL to make the driver ready to unload
//
#define IOCTL_PREPARE_TO_UNLOAD \
CTL_CODE(FILE_DEVICE_UNKNOWN, 2048, METHOD_NEITHER, FILE_WRITE_ACCESS)

#endif // RFSD_UNLOAD

#ifndef SetFlag
#define SetFlag(x,f)    ((x) |= (f))
#endif

#ifndef ClearFlag
#define ClearFlag(x,f)  ((x) &= ~(f))
#endif

#define IsFlagOn(a,b) ((BOOLEAN)(FlagOn(a,b) == b))

#define RfsdRaiseStatus(IRPCONTEXT,STATUS) {  \
    (IRPCONTEXT)->ExceptionCode = (STATUS); \
    ExRaiseStatus( (STATUS) );                \
}

#define RfsdNormalizeAndRaiseStatus(IRPCONTEXT,STATUS) {                        \
    /* (IRPCONTEXT)->ExceptionStatus = (STATUS);  */                            \
    if ((STATUS) == STATUS_VERIFY_REQUIRED) { ExRaiseStatus((STATUS)); }        \
    ExRaiseStatus(FsRtlNormalizeNtstatus((STATUS),STATUS_UNEXPECTED_IO_ERROR)); \
}

//
// Define IsEndofFile for read and write operations
//

#define FILE_WRITE_TO_END_OF_FILE       0xffffffff
#define FILE_USE_FILE_POINTER_POSITION  0xfffffffe

#define IsEndOfFile(Pos) ((Pos.LowPart == FILE_WRITE_TO_END_OF_FILE) && \
                          (Pos.HighPart == FILE_USE_FILE_POINTER_POSITION ))

#define IsDirectory(Fcb) IsFlagOn(Fcb->RfsdMcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY)

//
// Bug Check Codes Definitions
//

#define RFSD_FILE_SYSTEM   (FILE_SYSTEM)

#define RFSD_BUGCHK_BLOCK               (0x00010000)
#define RFSD_BUGCHK_CLEANUP             (0x00020000)
#define RFSD_BUGCHK_CLOSE               (0x00030000)
#define RFSD_BUGCHK_CMCB                (0x00040000)
#define RFSD_BUGCHK_CREATE              (0x00050000)
#define RFSD_BUGCHK_DEBUG               (0x00060000)
#define RFSD_BUGCHK_DEVCTL              (0x00070000)
#define RFSD_BUGCHK_DIRCTL              (0x00080000)
#define RFSD_BUGCHK_DISPATCH            (0x00090000)
#define RFSD_BUGCHK_EXCEPT              (0x000A0000)
#define RFSD_BUGCHK_RFSD                (0x000B0000)
#define RFSD_BUGCHK_FASTIO              (0x000C0000)
#define RFSD_BUGCHK_FILEINFO            (0x000D0000)
#define RFSD_BUGCHK_FLUSH               (0x000E0000)
#define RFSD_BUGCHK_FSCTL               (0x000F0000)
#define RFSD_BUGCHK_INIT                (0x00100000)
#define RFSD_BUGCHK_LOCK                (0x0011000)
#define RFSD_BUGCHK_MEMORY              (0x0012000)
#define RFSD_BUGCHK_MISC                (0x0013000)
#define RFSD_BUGCHK_READ                (0x00140000)
#define RFSD_BUGCHK_SHUTDOWN            (0x00150000)
#define RFSD_BUGCHK_VOLINFO             (0x00160000)
#define RFSD_BUGCHK_WRITE               (0x00170000)

#define RFSD_BUGCHK_LAST                (0x00170000)

#define RfsdBugCheck(A,B,C,D) { KeBugCheckEx(RFSD_FILE_SYSTEM, A | __LINE__, B, C, D ); }


/* Rfsd file system definions *******************************************/

//
// The second extended file system magic number
//

#define RFSD_MIN_BLOCK      1024
#define RFSD_MIN_FRAG       1024

//
// Inode flags (Linux uses octad number, but why ? strange!!!)
//

#define S_IFMT   0x0F000            /* 017 0000 */
#define S_IFSOCK 0x0C000            /* 014 0000 */
#define S_IFLNK  0x0A000            /* 012 0000 */
#define S_IFREG  0x08000            /* 010 0000 */
#define S_IFBLK  0x06000            /* 006 0000 */
#define S_IFDIR  0x04000            /* 004 0000 */
#define S_IFCHR  0x02000            /* 002 0000 */
#define S_IFIFO  0x01000            /* 001 0000 */
#define S_ISUID  0x00800            /* 000 4000 */
#define S_ISGID  0x00400            /* 000 2000 */
#define S_ISVTX  0x00200            /* 000 1000 */

#define S_ISREG(m)      (((m) & S_IFMT) == S_IFREG)
#define S_ISSOCK(m)     (((m) & S_IFMT) == S_IFSOCK)
#define S_ISLNK(m)      (((m) & S_IFMT) == S_IFLNK)
#define S_ISFIL(m)      (((m) & S_IFMT) == S_IFFIL)
#define S_ISBLK(m)      (((m) & S_IFMT) == S_IFBLK)
#define S_ISDIR(m)      (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)      (((m) & S_IFMT) == S_IFCHR)
#define S_ISFIFO(m)     (((m) & S_IFMT) == S_IFIFO)

#define S_IPERMISSION_MASK 0x1FF /*  */

#define S_IRWXU  0x1C0              /* 0 0700 */
#define S_IRUSR  0x100              /* 0 0400 */
#define S_IWUSR  0x080              /* 0 0200 */
#define S_IXUSR  0x040              /* 0 0100 */

#define S_IRWXG  0x038              /* 0 0070 */
#define S_IRGRP  0x020              /* 0 0040 */
#define S_IWGRP  0x010              /* 0 0020 */
#define S_IXGRP  0x008              /* 0 0010 */

#define S_IRWXO  0x007              /* 0 0007 */
#define S_IROTH  0x004              /* 0 0004 */
#define S_IWOTH  0x002              /* 0 0002 */
#define S_IXOTH  0x001              /* 0 0001 */

#define S_IRWXUGO   (S_IRWXU|S_IRWXG|S_IRWXO)
#define S_IALLUGO   (S_ISUID|S_ISGID|S_ISVTX|S_IRWXUGO)
#define S_IRUGO     (S_IRUSR|S_IRGRP|S_IROTH)
#define S_IWUGO     (S_IWUSR|S_IWGRP|S_IWOTH)
#define S_IXUGO     (S_IXUSR|S_IXGRP|S_IXOTH)

#define S_ISREADABLE(m)    (((m) & S_IPERMISSION_MASK) == (S_IRUSR | S_IRGRP | S_IROTH))
#define S_ISWRITABLE(m)    (((m) & S_IPERMISSION_MASK) == (S_IWUSR | S_IWGRP | S_IWOTH))

#define RfsdSetReadable(m) (m) = ((m) | (S_IRUSR | S_IRGRP | S_IROTH))
#define RfsdSetWritable(m) (m) = ((m) | (S_IWUSR | S_IWGRP | S_IWOTH))

#define RfsdSetReadOnly(m) (m) = ((m) & (~(S_IWUSR | S_IWGRP | S_IWOTH)))
#define RfsdIsReadOnly(m)  (!((m) & (S_IWUSR | S_IWGRP | S_IWOTH)))

//
//  Inode state bits
//

#define I_DIRTY_SYNC		1 /* Not dirty enough for O_DATASYNC */
#define I_DIRTY_DATASYNC	2 /* Data-related inode changes pending */
#define I_DIRTY_PAGES		4 /* Data-related inode changes pending */
#define I_LOCK			8
#define I_FREEING		16
#define I_CLEAR			32

#define I_DIRTY (I_DIRTY_SYNC | I_DIRTY_DATASYNC | I_DIRTY_PAGES)


//
// RfsdFsd Driver Definitions
//

//
// RFSD_IDENTIFIER_TYPE
//
// Identifiers used to mark the structures
//

typedef enum _RFSD_IDENTIFIER_TYPE {
    RFSDFGD  = ':DGF',
    RFSDVCB  = ':BCV',
    RFSDFCB  = ':BCF',
    RFSDCCB  = ':BCC',
    RFSDICX  = ':XCI',
    RFSDFSD  = ':DSF',
    RFSDMCB  = ':BCM'
} RFSD_IDENTIFIER_TYPE;

//
// RFSD_IDENTIFIER
//
// Header used to mark the structures
//
typedef struct _RFSD_IDENTIFIER {
    RFSD_IDENTIFIER_TYPE     Type;
    ULONG                    Size;
} RFSD_IDENTIFIER, *PRFSD_IDENTIFIER;


#define NodeType(Ptr) (*((RFSD_IDENTIFIER_TYPE *)(Ptr)))

typedef struct _RFSD_MCB  RFSD_MCB, *PRFSD_MCB;


typedef PVOID   PBCB;

//
// REPINNED_BCBS List
//

#define RFSD_REPINNED_BCBS_ARRAY_SIZE         (8)

typedef struct _RFSD_REPINNED_BCBS {

    //
    //  A pointer to the next structure contains additional repinned bcbs
    //

    struct _RFSD_REPINNED_BCBS *Next;

    //
    //  A fixed size array of pinned bcbs.  Whenever a new bcb is added to
    //  the repinned bcb structure it is added to this array.  If the
    //  array is already full then another repinned bcb structure is allocated
    //  and pointed to with Next.
    //

    PBCB Bcb[ RFSD_REPINNED_BCBS_ARRAY_SIZE ];

} RFSD_REPINNED_BCBS, *PRFSD_REPINNED_BCBS;


#define CODEPAGE_MAXLEN     0x20

//
// RFSD_GLOBAL_DATA
//
// Data that is not specific to a mounted volume
//

typedef struct _RFSD_GLOBAL {
    
    // Identifier for this structure
    RFSD_IDENTIFIER             Identifier;
    
    // Syncronization primitive for this structure
    ERESOURCE                   Resource;

    // Syncronization primitive for Counting
    ERESOURCE                   CountResource;

    // Syncronization primitive for LookAside Lists
    ERESOURCE                   LAResource;
    
    // Table of pointers to the fast I/O entry points
    FAST_IO_DISPATCH            FastIoDispatch;
    
    // Table of pointers to the Cache Manager callbacks
    CACHE_MANAGER_CALLBACKS     CacheManagerCallbacks;
    CACHE_MANAGER_CALLBACKS     CacheManagerNoOpCallbacks;
    
    // Pointer to the driver object
    PDRIVER_OBJECT              DriverObject;
    
    // Pointer to the main device object
    PDEVICE_OBJECT              DeviceObject;
    
    // List of mounted volumes
    LIST_ENTRY                  VcbList;

    // Look Aside table of IRP_CONTEXT, FCB, MCB, CCB
    USHORT                      MaxDepth;
    NPAGED_LOOKASIDE_LIST       RfsdIrpContextLookasideList;
    NPAGED_LOOKASIDE_LIST       RfsdFcbLookasideList;
    NPAGED_LOOKASIDE_LIST       RfsdCcbLookasideList;
    PAGED_LOOKASIDE_LIST        RfsdMcbLookasideList;

    // Mcb Count ...
    USHORT                      McbAllocated;

#if DBG
    // Fcb Count
    USHORT                      FcbAllocated;

    // IRP_MJ_CLOSE : FCB
    USHORT                      IRPCloseCount;
#endif
    
    // Global flags for the driver
    ULONG                       Flags;

    // User specified codepage name
    struct {
        WCHAR                   UniName[CODEPAGE_MAXLEN];
        UCHAR                   AnsiName[CODEPAGE_MAXLEN];
        struct nls_table *      PageTable;
    } CodePage;
    
} RFSD_GLOBAL, *PRFSD_GLOBAL;

#define PAGE_TABLE RfsdGlobal->CodePage.PageTable

//
// Flags for RFSD_GLOBAL_DATA
//
#define RFSD_UNLOAD_PENDING     0x00000001
#define RFSD_SUPPORT_WRITING    0x00000002
#define EXT3_FORCE_WRITING      0x00000004
#define RFSD_CHECKING_BITMAP    0x00000008

//
// Driver Extension define
//
typedef struct {
    RFSD_GLOBAL RfsdGlobal;
} RFSDFS_EXT, *PRFSDFS_EXT;


typedef struct _RFSD_FCBVCB {
    
    // FCB header required by NT
    FSRTL_COMMON_FCB_HEADER         CommonFCBHeader;
    SECTION_OBJECT_POINTERS         SectionObject;
    ERESOURCE                       MainResource;
    ERESOURCE                       PagingIoResource;
    // end FCB header required by NT
    
    // Identifier for this structure
    RFSD_IDENTIFIER                 Identifier;
} RFSD_FCBVCB, *PRFSD_FCBVCB;

//
// RFSD_VCB Volume Control Block
//
// Data that represents a mounted logical volume
// It is allocated as the device extension of the volume device object
//
typedef struct _RFSD_VCB {
    
    // FCB header required by NT
    // The VCB is also used as an FCB for file objects
    // that represents the volume itself
    FSRTL_COMMON_FCB_HEADER     Header;
    SECTION_OBJECT_POINTERS     SectionObject;
    ERESOURCE                   MainResource;
    ERESOURCE                   PagingIoResource;
    // end FCB header required by NT
    
    // Identifier for this structure
    RFSD_IDENTIFIER             Identifier;
    
    LIST_ENTRY                  Next;
    
    // Share Access for the file object
    SHARE_ACCESS                ShareAccess;

    // Incremented on IRP_MJ_CREATE, decremented on IRP_MJ_CLEANUP
    // for files on this volume.
    ULONG                       OpenFileHandleCount;
    
    // Incremented on IRP_MJ_CREATE, decremented on IRP_MJ_CLOSE
    // for both files on this volume and open instances of the
    // volume itself.
    ULONG                       ReferenceCount;
    ULONG                       OpenHandleCount;

    //
    // Disk change count
    //

    ULONG                       ChangeCount;
    
    // Pointer to the VPB in the target device object
    PVPB                        Vpb;

    // The FileObject of Volume used to lock the volume
    PFILE_OBJECT                LockFile;

    // List of FCBs for open files on this volume
    LIST_ENTRY                  FcbList;

    // List of IRPs pending on directory change notify requests
    LIST_ENTRY                  NotifyList;

    // Pointer to syncronization primitive for this list
    PNOTIFY_SYNC                NotifySync;
    
    // This volumes device object
    PDEVICE_OBJECT              DeviceObject;
    
    // The physical device object (the disk)
    PDEVICE_OBJECT              TargetDeviceObject;

    // The physical device object (the disk)
    PDEVICE_OBJECT              RealDevice;
    
    // Information about the physical device object
    DISK_GEOMETRY               DiskGeometry;
    PARTITION_INFORMATION       PartitionInformation;
    
    PRFSD_SUPER_BLOCK           SuperBlock;
    PVOID						GroupDesc;		// (NOTE: unused in ReiserFS, but preserved in order to minimize changes to existing code)
//  PVOID                       GroupDescBcb;

    // Number of Group Decsciptions
    ULONG                       NumOfGroups;
/*
    // Bitmap Block per group
    PRTL_BITMAP                 BlockBitMaps;
    PRTL_BITMAP                 InodeBitMaps;
*/
    // Block / Cluster size
    ULONG                       BlockSize;

    // Sector size in bits  (NOTE: unused in ReiserFS)
    //ULONG                       SectorBits;
    
    ULONG                       dwData[RFSD_BLOCK_TYPES];
    ULONG                       dwMeta[RFSD_BLOCK_TYPES];

    // Flags for the volume
    ULONG                       Flags;

    // Streaming File Object
    PFILE_OBJECT                StreamObj;

    // Resource Lock for Mcb
    ERESOURCE                   McbResource;

    // Dirty Mcbs of modifications for volume stream
    LARGE_MCB                   DirtyMcbs;

    // Entry of Mcb Tree (Root Node)
    PRFSD_MCB                   McbTree;
    LIST_ENTRY                  McbList;
    
} RFSD_VCB, *PRFSD_VCB;

//
// Flags for RFSD_VCB
//
#define VCB_INITIALIZED         0x00000001
#define VCB_VOLUME_LOCKED       0x00000002
#define VCB_MOUNTED             0x00000004
#define VCB_DISMOUNT_PENDING    0x00000008
#define VCB_READ_ONLY           0x00000010

#define VCB_WRITE_PROTECTED     0x10000000
#define VCB_FLOPPY_DISK         0x20000000
#define VCB_REMOVAL_PREVENTED   0x40000000
#define VCB_REMOVABLE_MEDIA     0x80000000


#define IsMounted(Vcb)    (IsFlagOn(Vcb->Flags, VCB_MOUNTED))

//
// RFSD_FCB File Control Block
//
// Data that represents an open file
// There is a single instance of the FCB for every open file
//
typedef struct _RFSD_FCB {
    
    // FCB header required by NT
    FSRTL_COMMON_FCB_HEADER         Header;
    SECTION_OBJECT_POINTERS         SectionObject;
    ERESOURCE                       MainResource;
    ERESOURCE                       PagingIoResource;
    // end FCB header required by NT
    
    // Identifier for this structure
    RFSD_IDENTIFIER                 Identifier;
    
    // List of FCBs for this volume
    LIST_ENTRY                      Next;
    
    // Share Access for the file object
    SHARE_ACCESS                    ShareAccess;

    // List of byte-range locks for this file
    FILE_LOCK                       FileLockAnchor;

    // Incremented on IRP_MJ_CREATE, decremented on IRP_MJ_CLEANUP
    ULONG                           OpenHandleCount;
    
    // Incremented on IRP_MJ_CREATE, decremented on IRP_MJ_CLOSE
    ULONG                           ReferenceCount;

    // Incremented on IRP_MJ_CREATE, decremented on IRP_MJ_CLEANUP
    // But only for Files with FO_NO_INTERMEDIATE_BUFFERING flag
    ULONG                           NonCachedOpenCount;

    // Flags for the FCB
    ULONG                           Flags;
    
    // Pointer to the inode  / stat data structure
    PRFSD_INODE                     Inode;

    // Hint block for next allocation
    ULONG                           BlkHint;
    
    // Vcb

    PRFSD_VCB                       Vcb;

    // Mcb Node ...
    PRFSD_MCB                       RfsdMcb;

    // Full Path Name
    UNICODE_STRING                  LongName;

#if DBG
    // The Ansi Filename for debugging
    OEM_STRING                      AnsiFileName;   
#endif


} RFSD_FCB, *PRFSD_FCB;


//
// Flags for RFSD_FCB
//
#define FCB_FROM_POOL               0x00000001
#define FCB_PAGE_FILE               0x00000002
#define FCB_DELETE_ON_CLOSE         0x00000004
#define FCB_DELETE_PENDING          0x00000008
#define FCB_FILE_DELETED            0x00000010
#define FCB_FILE_MODIFIED           0x00000020

// Mcb Node

struct _RFSD_MCB {

    // Identifier for this structure
    RFSD_IDENTIFIER                 Identifier;

    // Flags
    ULONG                           Flags;

    // Link List Info

    PRFSD_MCB                       Parent; // Parent
    PRFSD_MCB                       Child;  // Children
    PRFSD_MCB                       Next;   // Brothers

    // Mcb Node Info

    // -> Fcb
    PRFSD_FCB                       RfsdFcb;

    // Short name
    UNICODE_STRING                  ShortName;

    // Inode number (ReiserFS uses 128-bit keys instead of inode numbers)
	RFSD_KEY_IN_MEMORY				Key;

    // Dir entry offset in parent (relative to the start of the directory listing)
    ULONG                           DeOffset;

    // File attribute
    ULONG                           FileAttr;

    // List Link to Vcb->McbList
    LIST_ENTRY                      Link;
};

//
// Flags for MCB
//
#define MCB_FROM_POOL               0x00000001
#define MCB_IN_TREE                 0x00000002
#define MCB_IN_USE                  0x00000004

#define IsMcbUsed(Mcb) IsFlagOn(Mcb->Flags, MCB_IN_USE)

//
// RFSD_CCB Context Control Block
//
// Data that represents one instance of an open file
// There is one instance of the CCB for every instance of an open file
//
typedef struct _RFSD_CCB {
    
    // Identifier for this structure
    RFSD_IDENTIFIER  Identifier;

    // Flags
    ULONG             Flags;
    
    // State that may need to be maintained
    ULONG             CurrentByteOffset;
    UNICODE_STRING    DirectorySearchPattern;
    
} RFSD_CCB, *PRFSD_CCB;

//
// Flags for CCB
//

#define CCB_FROM_POOL               0x00000001

#define CCB_ALLOW_EXTENDED_DASD_IO  0x80000000

//
// RFSD_IRP_CONTEXT
//
// Used to pass information about a request between the drivers functions
//
typedef struct _RFSD_IRP_CONTEXT {
    
    // Identifier for this structure
    RFSD_IDENTIFIER     Identifier;
    
    // Pointer to the IRP this request describes
    PIRP                Irp;

    // Flags
    ULONG               Flags;
    
    // The major and minor function code for the request
    UCHAR               MajorFunction;
    UCHAR               MinorFunction;
    
    // The device object
    PDEVICE_OBJECT      DeviceObject;

    // The real device object
    PDEVICE_OBJECT      RealDevice;

    // The file object
    PFILE_OBJECT        FileObject;

    PRFSD_FCB           Fcb;
    PRFSD_CCB           Ccb;
    
    // If the request is synchronous (we are allowed to block)
    BOOLEAN             IsSynchronous;
    
    // If the request is top level
    BOOLEAN             IsTopLevel;
    
    // Used if the request needs to be queued for later processing
    WORK_QUEUE_ITEM     WorkQueueItem;
    
    // If an exception is currently in progress
    BOOLEAN             ExceptionInProgress;
    
    // The exception code when an exception is in progress
    NTSTATUS            ExceptionCode;

    // Repinned BCBs List
    RFSD_REPINNED_BCBS  Repinned;
    
} RFSD_IRP_CONTEXT, *PRFSD_IRP_CONTEXT;


#define IRP_CONTEXT_FLAG_FROM_POOL       (0x00000001)
#define IRP_CONTEXT_FLAG_WAIT            (0x00000002)
#define IRP_CONTEXT_FLAG_WRITE_THROUGH   (0x00000004)
#define IRP_CONTEXT_FLAG_FLOPPY          (0x00000008)
#define IRP_CONTEXT_FLAG_RECURSIVE_CALL  (0x00000010)
#define IRP_CONTEXT_FLAG_DISABLE_POPUPS  (0x00000020)
#define IRP_CONTEXT_FLAG_DEFERRED        (0x00000040)
#define IRP_CONTEXT_FLAG_VERIFY_READ     (0x00000080)
#define IRP_CONTEXT_STACK_IO_CONTEXT     (0x00000100)
#define IRP_CONTEXT_FLAG_REQUEUED        (0x00000200)
#define IRP_CONTEXT_FLAG_USER_IO         (0x00000400)
#define IRP_CONTEXT_FLAG_DELAY_CLOSE     (0x00000800)

//
// RFSD_ALLOC_HEADER
//
// In the checked version of the driver this header is put in the beginning of
// every memory allocation
//
typedef struct _RFSD_ALLOC_HEADER {
    RFSD_IDENTIFIER Identifier;
} RFSD_ALLOC_HEADER, *PRFSD_ALLOC_HEADER;

typedef struct _FCB_LIST_ENTRY {
    PRFSD_FCB    Fcb;
    LIST_ENTRY   Next;
} FCB_LIST_ENTRY, *PFCB_LIST_ENTRY;


// Block Description List
typedef struct _RFSD_BDL {
    ULONGLONG    Lba;
    ULONGLONG    Offset;
    ULONG        Length;
    PIRP         Irp;
} RFSD_BDL, *PRFSD_BDL;

#pragma pack()


/* FUNCTIONS DECLARATION *****************************************************/

//
//  The following macro is used to determine if an FSD thread can block
//  for I/O or wait for a resource.  It returns TRUE if the thread can
//  block and FALSE otherwise.  This attribute can then be used to call
//  the FSD & FSP common work routine with the proper wait value.
//

#define CanRfsdWait(IRP) IoIsOperationSynchronous(Irp)

#include "protos.h"

#endif /* _RFSD_HEADER_ */

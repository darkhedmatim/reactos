#ifndef _WINIOCTL_
#define _WINIOCTL_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4201)
#pragma warning(disable:4820)
#endif

#define HIST_NO_OF_BUCKETS               24
#define HISTOGRAM_BUCKET_SIZE            sizeof(HISTOGRAM_BUCKET)
#define DISK_HISTOGRAM_SIZE              sizeof(DISK_HISTOGRAM)

#ifndef _NTDDSTOR_H_
#define IOCTL_STORAGE_BASE               FILE_DEVICE_MASS_STORAGE
#define IOCTL_STORAGE_CHECK_VERIFY       CTL_CODE(IOCTL_STORAGE_BASE, 0x0200, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_CHECK_VERIFY2      CTL_CODE(IOCTL_STORAGE_BASE, 0x0200, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STORAGE_MEDIA_REMOVAL      CTL_CODE(IOCTL_STORAGE_BASE, 0x0201, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_EJECT_MEDIA        CTL_CODE(IOCTL_STORAGE_BASE, 0x0202, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_LOAD_MEDIA         CTL_CODE(IOCTL_STORAGE_BASE, 0x0203, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_LOAD_MEDIA2        CTL_CODE(IOCTL_STORAGE_BASE, 0x0203, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STORAGE_RESERVE            CTL_CODE(IOCTL_STORAGE_BASE, 0x0204, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_RELEASE            CTL_CODE(IOCTL_STORAGE_BASE, 0x0205, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_FIND_NEW_DEVICES   CTL_CODE(IOCTL_STORAGE_BASE, 0x0206, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_EJECTION_CONTROL   CTL_CODE(IOCTL_STORAGE_BASE, 0x0250, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STORAGE_MCN_CONTROL        CTL_CODE(IOCTL_STORAGE_BASE, 0x0251, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STORAGE_GET_MEDIA_TYPES    CTL_CODE(IOCTL_STORAGE_BASE, 0x0300, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STORAGE_GET_MEDIA_TYPES_EX CTL_CODE(IOCTL_STORAGE_BASE, 0x0301, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STORAGE_RESET_BUS          CTL_CODE(IOCTL_STORAGE_BASE, 0x0400, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_RESET_DEVICE       CTL_CODE(IOCTL_STORAGE_BASE, 0x0401, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_STORAGE_GET_DEVICE_NUMBER  CTL_CODE(IOCTL_STORAGE_BASE, 0x0420, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STORAGE_PREDICT_FAILURE    CTL_CODE(IOCTL_STORAGE_BASE, 0x0440, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#define IOCTL_DISK_BASE                  FILE_DEVICE_DISK
#define IOCTL_DISK_GET_DRIVE_GEOMETRY    CTL_CODE(IOCTL_DISK_BASE,0,METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_GET_PARTITION_INFO    CTL_CODE(IOCTL_DISK_BASE,1,METHOD_BUFFERED,FILE_READ_ACCESS)
#define IOCTL_DISK_SET_PARTITION_INFO    CTL_CODE(IOCTL_DISK_BASE,2,METHOD_BUFFERED,FILE_READ_ACCESS|FILE_WRITE_ACCESS)
#define IOCTL_DISK_GET_DRIVE_LAYOUT      CTL_CODE(IOCTL_DISK_BASE,3,METHOD_BUFFERED,FILE_READ_ACCESS)
#define IOCTL_DISK_SET_DRIVE_LAYOUT      CTL_CODE(IOCTL_DISK_BASE,4,METHOD_BUFFERED,FILE_READ_ACCESS|FILE_WRITE_ACCESS)
#define IOCTL_DISK_VERIFY                CTL_CODE(IOCTL_DISK_BASE,5,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_DISK_FORMAT_TRACKS         CTL_CODE(IOCTL_DISK_BASE,6,METHOD_BUFFERED,FILE_READ_ACCESS|FILE_WRITE_ACCESS)
#define IOCTL_DISK_REASSIGN_BLOCKS       CTL_CODE(IOCTL_DISK_BASE,7,METHOD_BUFFERED,FILE_READ_ACCESS|FILE_WRITE_ACCESS)
#define IOCTL_DISK_PERFORMANCE           CTL_CODE(IOCTL_DISK_BASE,8,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_DISK_IS_WRITABLE           CTL_CODE(IOCTL_DISK_BASE,9,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_DISK_LOGGING               CTL_CODE(IOCTL_DISK_BASE,10,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_DISK_FORMAT_TRACKS_EX      CTL_CODE(IOCTL_DISK_BASE,11,METHOD_BUFFERED,FILE_READ_ACCESS|FILE_WRITE_ACCESS)
#define IOCTL_DISK_HISTOGRAM_STRUCTURE   CTL_CODE(IOCTL_DISK_BASE,12,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_DISK_HISTOGRAM_DATA        CTL_CODE(IOCTL_DISK_BASE,13,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_DISK_HISTOGRAM_RESET       CTL_CODE(IOCTL_DISK_BASE,14,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_DISK_REQUEST_STRUCTURE     CTL_CODE(IOCTL_DISK_BASE,15,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_DISK_REQUEST_DATA          CTL_CODE(IOCTL_DISK_BASE,16,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_DISK_GET_PARTITION_INFO_EX CTL_CODE(IOCTL_DISK_BASE,0x12,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_DISK_SET_PARTITION_INFO_EX CTL_CODE(IOCTL_DISK_BASE,0x13,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_DISK_GET_DRIVE_LAYOUT_EX   CTL_CODE(IOCTL_DISK_BASE,0x14,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_DISK_SET_DRIVE_LAYOUT_EX   CTL_CODE(IOCTL_DISK_BASE,0x15,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_DISK_CREATE_DISK           CTL_CODE(IOCTL_DISK_BASE,0x16,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_DISK_GET_LENGTH_INFO       CTL_CODE(IOCTL_DISK_BASE,0x17,METHOD_BUFFERED,FILE_READ_ACCESS)
#define IOCTL_DISK_PERFORMANCE_OFF       CTL_CODE(IOCTL_DISK_BASE,0x18,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_DISK_GET_DRIVE_GEOMETRY_EX CTL_CODE(IOCTL_DISK_BASE,0x28,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_DISK_GROW_PARTITION        CTL_CODE(IOCTL_DISK_BASE,0x34,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_DISK_GET_CACHE_INFORMATION CTL_CODE(IOCTL_DISK_BASE,0x35,METHOD_BUFFERED,FILE_READ_ACCESS)
#define IOCTL_DISK_SET_CACHE_INFORMATION CTL_CODE(IOCTL_DISK_BASE,0x36,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_DISK_DELETE_DRIVE_LAYOUT   CTL_CODE(IOCTL_DISK_BASE,0x40,METHOD_BUFFERED,FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_DISK_UPDATE_PROPERTIES CTL_CODE(IOCTL_DISK_BASE,0x50,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_DISK_CHECK_VERIFY      CTL_CODE(IOCTL_DISK_BASE,0x200,METHOD_BUFFERED,FILE_READ_ACCESS)
#define IOCTL_DISK_MEDIA_REMOVAL     CTL_CODE(IOCTL_DISK_BASE,0x201,METHOD_BUFFERED,FILE_READ_ACCESS)
#define IOCTL_DISK_EJECT_MEDIA       CTL_CODE(IOCTL_DISK_BASE,0x202,METHOD_BUFFERED,FILE_READ_ACCESS)
#define IOCTL_DISK_LOAD_MEDIA        CTL_CODE(IOCTL_DISK_BASE,0x203,METHOD_BUFFERED,FILE_READ_ACCESS)
#define IOCTL_DISK_RESERVE           CTL_CODE(IOCTL_DISK_BASE,0x204,METHOD_BUFFERED,FILE_READ_ACCESS)
#define IOCTL_DISK_RELEASE           CTL_CODE(IOCTL_DISK_BASE,0x205,METHOD_BUFFERED,FILE_READ_ACCESS)
#define IOCTL_DISK_FIND_NEW_DEVICES  CTL_CODE(IOCTL_DISK_BASE,0x206,METHOD_BUFFERED,FILE_READ_ACCESS)
#define IOCTL_DISK_REMOVE_DEVICE     CTL_CODE(IOCTL_DISK_BASE,0x207,METHOD_BUFFERED,FILE_READ_ACCESS)
#define IOCTL_DISK_GET_MEDIA_TYPES   CTL_CODE(IOCTL_DISK_BASE,0x300,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_DISK_UPDATE_DRIVE_SIZE CTL_CODE(IOCTL_DISK_BASE, 0x0032, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_SERIAL_LSRMST_INSERT   CTL_CODE(FILE_DEVICE_SERIAL_PORT,31,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_VOLUME_BASE ((DWORD)'V')
#define IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS CTL_CODE(IOCTL_VOLUME_BASE, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_LOCK_VOLUME            CTL_CODE(FILE_DEVICE_FILE_SYSTEM,6,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define FSCTL_UNLOCK_VOLUME          CTL_CODE(FILE_DEVICE_FILE_SYSTEM,7,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define FSCTL_PIPE_IMPERSONATE       CTL_CODE(FILE_DEVICE_NAMED_PIPE, 7, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DISMOUNT_VOLUME        CTL_CODE(FILE_DEVICE_FILE_SYSTEM,8,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define FSCTL_MOUNT_DBLS_VOLUME      CTL_CODE(FILE_DEVICE_FILE_SYSTEM,13,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define FSCTL_GET_COMPRESSION        CTL_CODE(FILE_DEVICE_FILE_SYSTEM,15,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define FSCTL_SET_COMPRESSION        CTL_CODE(FILE_DEVICE_FILE_SYSTEM,16,METHOD_BUFFERED,FILE_READ_DATA|FILE_WRITE_DATA)
#define FSCTL_READ_COMPRESSION       CTL_CODE(FILE_DEVICE_FILE_SYSTEM,17,METHOD_NEITHER,FILE_READ_DATA)
#define FSCTL_WRITE_COMPRESSION      CTL_CODE(FILE_DEVICE_FILE_SYSTEM,18,METHOD_NEITHER,FILE_WRITE_DATA)
#define FSCTL_GET_NTFS_VOLUME_DATA   CTL_CODE(FILE_DEVICE_FILE_SYSTEM,25,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define FSCTL_GET_NTFS_FILE_RECORD   CTL_CODE(FILE_DEVICE_FILE_SYSTEM,26,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define FSCTL_GET_VOLUME_BITMAP      CTL_CODE(FILE_DEVICE_FILE_SYSTEM,27,METHOD_NEITHER,FILE_ANY_ACCESS)
#define FSCTL_GET_RETRIEVAL_POINTERS CTL_CODE(FILE_DEVICE_FILE_SYSTEM,28,METHOD_NEITHER,FILE_ANY_ACCESS)
#define FSCTL_MOVE_FILE              CTL_CODE(FILE_DEVICE_FILE_SYSTEM,29,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define FSCTL_GET_REPARSE_POINT      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 42, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SET_REPARSE_POINT      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 41, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DELETE_REPARSE_POINT   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 43, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SET_SPARSE             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 49, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

#ifndef _DEVIOCTL_
#define _DEVIOCTL_

#define DEVICE_TYPE DWORD
#define FILE_DEVICE_BEEP              1
#define FILE_DEVICE_CD_ROM            2
#define FILE_DEVICE_CD_ROM_FILE_SYSTEM 3
#define FILE_DEVICE_CONTROLLER        4
#define FILE_DEVICE_DATALINK          5
#define FILE_DEVICE_DFS               6
#define FILE_DEVICE_DISK              7
#define FILE_DEVICE_DISK_FILE_SYSTEM  8
#define FILE_DEVICE_FILE_SYSTEM       9
#define FILE_DEVICE_INPORT_PORT       10
#define FILE_DEVICE_KEYBOARD          11
#define FILE_DEVICE_MAILSLOT          12
#define FILE_DEVICE_MIDI_IN           13
#define FILE_DEVICE_MIDI_OUT          14
#define FILE_DEVICE_MOUSE             15
#define FILE_DEVICE_MULTI_UNC_PROVIDER 16
#define FILE_DEVICE_NAMED_PIPE        17
#define FILE_DEVICE_NETWORK           18
#define FILE_DEVICE_NETWORK_BROWSER   19
#define FILE_DEVICE_NETWORK_FILE_SYSTEM 20
#define FILE_DEVICE_NULL              21
#define FILE_DEVICE_PARALLEL_PORT     22
#define FILE_DEVICE_PHYSICAL_NETCARD  23
#define FILE_DEVICE_PRINTER           24
#define FILE_DEVICE_SCANNER           25
#define FILE_DEVICE_SERIAL_MOUSE_PORT 26
#define FILE_DEVICE_SERIAL_PORT       27
#define FILE_DEVICE_SCREEN            28
#define FILE_DEVICE_SOUND             29
#define FILE_DEVICE_STREAMS           30
#define FILE_DEVICE_TAPE              31
#define FILE_DEVICE_TAPE_FILE_SYSTEM  32
#define FILE_DEVICE_TRANSPORT         33
#define FILE_DEVICE_UNKNOWN           34
#define FILE_DEVICE_VIDEO             35
#define FILE_DEVICE_VIRTUAL_DISK      36
#define FILE_DEVICE_WAVE_IN           37
#define FILE_DEVICE_WAVE_OUT          38
#define FILE_DEVICE_8042_PORT         39
#define FILE_DEVICE_NETWORK_REDIRECTOR  40
#define FILE_DEVICE_BATTERY           41
#define FILE_DEVICE_BUS_EXTENDER      42
#define FILE_DEVICE_MODEM             43
#define FILE_DEVICE_VDM               44
#define FILE_DEVICE_MASS_STORAGE      45
#define FILE_DEVICE_SMB               46
#define FILE_DEVICE_KS                47
#define FILE_DEVICE_CHANGER           48
#define FILE_DEVICE_SMARTCARD         49
#define FILE_DEVICE_ACPI              50
#define FILE_DEVICE_DVD               51
#define FILE_DEVICE_FULLSCREEN_VIDEO  52
#define FILE_DEVICE_DFS_FILE_SYSTEM   53
#define FILE_DEVICE_DFS_VOLUME        54
#define FILE_DEVICE_SERENUM           55
#define FILE_DEVICE_TERMSRV           56
#define FILE_DEVICE_KSEC              57

/*  Also in ddk/winddk.h */
#define FILE_ANY_ACCESS        0x00000000
#define FILE_SPECIAL_ACCESS    FILE_ANY_ACCESS
#define FILE_READ_ACCESS       0x00000001
#define FILE_WRITE_ACCESS      0x00000002

#define METHOD_BUFFERED               0
#define METHOD_IN_DIRECT              1
#define METHOD_OUT_DIRECT             2
#define METHOD_NEITHER                3

#define CTL_CODE(t,f,m,a)                (((t)<<16)|((a)<<14)|((f)<<2)|(m))
#define DEVICE_TYPE_FROM_CTL_CODE(c)     (((DWORD)((c)&0xffff0000))>>16)

#endif /* _DEVIOCTL_ */

#define PARTITION_ENTRY_UNUSED        0
#define PARTITION_FAT_12              1
#define PARTITION_XENIX_1             2
#define PARTITION_XENIX_2             3
#define PARTITION_FAT_16              4
#define PARTITION_EXTENDED            5
#define PARTITION_HUGE                6
#define PARTITION_IFS                 7
#define PARTITION_FAT32               0x0B
#define PARTITION_FAT32_XINT13        0x0C
#define PARTITION_XINT13              0x0E
#define PARTITION_XINT13_EXTENDED     0x0F
#define PARTITION_PREP                0x41
#define PARTITION_LDM                 0x42
#define PARTITION_UNIX                0x63
#define PARTITION_NTFT                0x80
#define VALID_NTFT                    0xC0
#ifdef __REACTOS__
#define PARTITION_OLD_LINUX               0x43
#define PARTITION_LINUX                   0x83
#define PARTITION_FREEBSD                 0xA5
#define PARTITION_OPENBSD                 0xA6
#define PARTITION_NETBSD                  0xA9
#endif
#define SERIAL_LSRMST_ESCAPE          0
#define SERIAL_LSRMST_LSR_DATA        1
#define SERIAL_LSRMST_LSR_NODATA      2
#define SERIAL_LSRMST_MST             3
/* Device GUIDs */
#ifdef DEFINE_GUID

DEFINE_GUID(GUID_DEVINTERFACE_COMPORT, 0x86E0D1E0L, 0x8089,
 0x11D0, 0x9C, 0xE4, 0x08, 0x00, 0x3E, 0x30, 0x1F, 0x73);
DEFINE_GUID(GUID_DEVINTERFACE_SERENUM_BUS_ENUMERATOR, 0x4D36E978L, 0xE325,
 0x11CE, 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18);

/* obsolete GUID names */
#define GUID_CLASS_COMPORT          GUID_DEVINTERFACE_COMPORT
#define GUID_SERENUM_BUS_ENUMERATOR GUID_DEVINTERFACE_SERENUM_BUS_ENUMERATOR

#endif /* DEFINE_GUID */

#define DISK_LOGGING_START   0
#define DISK_LOGGING_STOP    1
#define DISK_LOGGING_DUMP    2
#define DISK_BINNING         3
typedef WORD BAD_TRACK_NUMBER,*PBAD_TRACK_NUMBER;
typedef enum _BIN_TYPES {RequestSize,RequestLocation} BIN_TYPES;
typedef struct _BIN_RANGE {
    LARGE_INTEGER StartValue;
    LARGE_INTEGER Length;
} BIN_RANGE,*PBIN_RANGE;
typedef struct _BIN_COUNT {
    BIN_RANGE BinRange;
    DWORD BinCount;
} BIN_COUNT,*PBIN_COUNT;
typedef struct _BIN_RESULTS {
    DWORD NumberOfBins;
    BIN_COUNT BinCounts[1];
} BIN_RESULTS,*PBIN_RESULTS;
typedef enum _PARTITION_STYLE {
  PARTITION_STYLE_MBR,
  PARTITION_STYLE_GPT,
  PARTITION_STYLE_RAW
} PARTITION_STYLE;
typedef struct {
  GUID DiskId;
  DWORD MaxPartitionCount;
} CREATE_DISK_GPT,*PCREATE_DISK_GPT;
typedef struct {
  DWORD Signature;
} CREATE_DISK_MBR,*PCREATE_DISK_MBR;
typedef struct {
  PARTITION_STYLE PartitionStyle;
  _ANONYMOUS_UNION union {
    CREATE_DISK_MBR Mbr;
    CREATE_DISK_GPT Gpt;
  };
} CREATE_DISK,*PCREATE_DISK;
typedef enum {
  EqualPriority,
  KeepPrefetchedData,
  KeepReadData
} DISK_CACHE_RETENTION_PRIORITY;
typedef struct _DISK_CACHE_INFORMATION {
  BOOLEAN ParametersSavable;
  BOOLEAN ReadCacheEnabled;
  BOOLEAN WriteCacheEnabled;
  DISK_CACHE_RETENTION_PRIORITY ReadRetentionPriority;
  DISK_CACHE_RETENTION_PRIORITY WriteRetentionPriority;
  WORD DisablePrefetchTransferLength;
  BOOLEAN PrefetchScalar;
  _ANONYMOUS_UNION union {
    struct {
      WORD Minimum;
      WORD Maximum;
      WORD MaximumBlocks;
    } ScalarPrefetch;
    struct {
      WORD Minimum;
      WORD Maximum;
    } BlockPrefetch;
  };
} DISK_CACHE_INFORMATION,*PDISK_CACHE_INFORMATION;
typedef enum _DETECTION_TYPE {
  DetectNone,
  DetectInt13,
  DetectExInt13
} DETECTION_TYPE;
typedef struct _DISK_INT13_INFO {
  WORD DriveSelect;
  DWORD MaxCylinders;
  WORD SectorsPerTrack;
  WORD MaxHeads;
  WORD NumberDrives;
} DISK_INT13_INFO,*PDISK_INT13_INFO;
typedef struct _DISK_EX_INT13_INFO {
  WORD ExBufferSize;
  WORD ExFlags;
  DWORD ExCylinders;
  DWORD ExHeads;
  DWORD ExSectorsPerTrack;
  DWORD64 ExSectorsPerDrive;
  WORD ExSectorSize;
  WORD ExReserved;
} DISK_EX_INT13_INFO,*PDISK_EX_INT13_INFO;
typedef struct _DISK_DETECTION_INFO {
  DWORD SizeOfDetectInfo;
  DETECTION_TYPE DetectionType;
  _ANONYMOUS_UNION union {
    _ANONYMOUS_STRUCT struct {
      DISK_INT13_INFO Int13;
      DISK_EX_INT13_INFO ExInt13;
    };
  };
} DISK_DETECTION_INFO,*PDISK_DETECTION_INFO;
typedef enum _MEDIA_TYPE {
  Unknown,
  F5_1Pt2_512,
  F3_1Pt44_512,
  F3_2Pt88_512,
  F3_20Pt8_512,
  F3_720_512,
  F5_360_512,
  F5_320_512,
  F5_320_1024,
  F5_180_512,
  F5_160_512,
  RemovableMedia,
  FixedMedia,
  F3_120M_512,
  F3_640_512,
  F5_640_512,
  F5_720_512,
  F3_1Pt2_512,
  F3_1Pt23_1024,
  F5_1Pt23_1024,
  F3_128Mb_512,
  F3_230Mb_512,
  F8_256_128,
  F3_200Mb_512,
  F3_240M_512,
  F3_32M_512
} MEDIA_TYPE,*PMEDIA_TYPE;
typedef struct _DISK_GEOMETRY {
  LARGE_INTEGER Cylinders;
  MEDIA_TYPE MediaType;
  DWORD TracksPerCylinder;
  DWORD SectorsPerTrack;
  DWORD BytesPerSector;
} DISK_GEOMETRY,*PDISK_GEOMETRY;
typedef struct _DISK_GEOMETRY_EX {
  DISK_GEOMETRY Geometry;
  LARGE_INTEGER DiskSize;
  BYTE Data[1];
} DISK_GEOMETRY_EX,*PDISK_GEOMETRY_EX;
typedef struct _DISK_GROW_PARTITION {
  DWORD PartitionNumber;
  LARGE_INTEGER BytesToGrow;
} DISK_GROW_PARTITION, *PDISK_GROW_PARTITION;
typedef struct _DISK_PARTITION_INFO {
  DWORD SizeOfPartitionInfo;
  PARTITION_STYLE PartitionStyle;
  _ANONYMOUS_UNION union {
    struct {
      DWORD Signature;
    } Mbr;
    struct {
      GUID DiskId;
    } Gpt;
  };
} DISK_PARTITION_INFO,*PDISK_PARTITION_INFO;
typedef struct _DISK_PERFORMANCE {
    LARGE_INTEGER BytesRead;
    LARGE_INTEGER BytesWritten;
    LARGE_INTEGER ReadTime;
    LARGE_INTEGER WriteTime;
    LARGE_INTEGER IdleTime;
    DWORD ReadCount;
    DWORD WriteCount;
    DWORD QueueDepth;
    DWORD SplitCount;
    LARGE_INTEGER QueryTime;
    DWORD   StorageDeviceNumber;
    WCHAR   StorageManagerName[8];
} DISK_PERFORMANCE, *PDISK_PERFORMANCE;
typedef struct _DISK_RECORD {
  LARGE_INTEGER ByteOffset;
  LARGE_INTEGER StartTime;
  LARGE_INTEGER EndTime;
  PVOID VirtualAddress;
  DWORD NumberOfBytes;
  BYTE DeviceNumber;
  BOOLEAN ReadRequest;
} DISK_RECORD,*PDISK_RECORD;
typedef struct _DISK_LOGGING {
  BYTE Function;
  PVOID BufferAddress;
  DWORD BufferSize;
} DISK_LOGGING,*PDISK_LOGGING;
typedef struct DiskQuotaUserInformation {
  LONGLONG QuotaUsed;
  LONGLONG QuotaThreshold;
  LONGLONG QuotaLimit;
} DISKQUOTA_USER_INFORMATION,*PDISKQUOTA_USER_INFORMATION;
typedef struct _FORMAT_PARAMETERS {
  MEDIA_TYPE MediaType;
  DWORD StartCylinderNumber;
  DWORD EndCylinderNumber;
  DWORD StartHeadNumber;
  DWORD EndHeadNumber;
} FORMAT_PARAMETERS,*PFORMAT_PARAMETERS;
typedef struct _FORMAT_EX_PARAMETERS {
  MEDIA_TYPE MediaType;
  DWORD StartCylinderNumber;
  DWORD EndCylinderNumber;
  DWORD StartHeadNumber;
  DWORD EndHeadNumber;
  WORD FormatGapLength;
  WORD SectorsPerTrack;
  WORD SectorNumber[1];
} FORMAT_EX_PARAMETERS,*PFORMAT_EX_PARAMETERS;
typedef struct {
  LARGE_INTEGER Length;
} GET_LENGTH_INFORMATION;
typedef struct _HISTOGRAM_BUCKET {
  DWORD Reads;
  DWORD Writes;
} HISTOGRAM_BUCKET,*PHISTOGRAM_BUCKET;
typedef struct _DISK_HISTOGRAM {
  LARGE_INTEGER DiskSize;
  LARGE_INTEGER Start;
  LARGE_INTEGER End;
  LARGE_INTEGER Average;
  LARGE_INTEGER AverageRead;
  LARGE_INTEGER AverageWrite;
  DWORD Granularity;
  DWORD Size;
  DWORD ReadCount;
  DWORD WriteCount;
  PHISTOGRAM_BUCKET Histogram;
} DISK_HISTOGRAM,*PDISK_HISTOGRAM;
typedef struct _DISK_EXTENT {
  DWORD DiskNumber;
  LARGE_INTEGER StartingOffset;
  LARGE_INTEGER ExtentLength;
} DISK_EXTENT,*PDISK_EXTENT;
typedef struct _VOLUME_DISK_EXTENTS {
  DWORD NumberOfDiskExtents;
  DISK_EXTENT Extents[1];
} VOLUME_DISK_EXTENTS,*PVOLUME_DISK_EXTENTS;
typedef struct _PARTITION_INFORMATION {
  LARGE_INTEGER StartingOffset;
  LARGE_INTEGER PartitionLength;
  DWORD HiddenSectors;
  DWORD PartitionNumber;
  BYTE PartitionType;
  BOOLEAN BootIndicator;
  BOOLEAN RecognizedPartition;
  BOOLEAN RewritePartition;
} PARTITION_INFORMATION,*PPARTITION_INFORMATION;
typedef struct _DRIVE_LAYOUT_INFORMATION {
  DWORD PartitionCount;
  DWORD Signature;
  PARTITION_INFORMATION PartitionEntry[1];
} DRIVE_LAYOUT_INFORMATION, *PDRIVE_LAYOUT_INFORMATION;
typedef struct _DRIVE_LAYOUT_INFORMATION_GPT {
  GUID DiskId;
  LARGE_INTEGER StartingUsableOffset;
  LARGE_INTEGER UsableLength;
  ULONG MaxPartitionCount;
} DRIVE_LAYOUT_INFORMATION_GPT,*PDRIVE_LAYOUT_INFORMATION_GPT;
typedef struct _DRIVE_LAYOUT_INFORMATION_MBR {
  ULONG Signature;
} DRIVE_LAYOUT_INFORMATION_MBR, *PDRIVE_LAYOUT_INFORMATION_MBR;
typedef struct _PARTITION_INFORMATION_MBR {
  BYTE PartitionType;
  BOOLEAN BootIndicator;
  BOOLEAN RecognizedPartition;
  DWORD HiddenSectors;
} PARTITION_INFORMATION_MBR;
typedef struct _PARTITION_INFORMATION_GPT {
  GUID PartitionType;
  GUID PartitionId;
  DWORD64 Attributes;
  WCHAR Name[36];
} PARTITION_INFORMATION_GPT;
typedef struct _PARTITION_INFORMATION_EX {
  PARTITION_STYLE PartitionStyle;
  LARGE_INTEGER StartingOffset;
  LARGE_INTEGER PartitionLength;
  DWORD PartitionNumber;
  BOOLEAN RewritePartition;
  _ANONYMOUS_UNION union {
    PARTITION_INFORMATION_MBR Mbr;
    PARTITION_INFORMATION_GPT Gpt;
  };
} PARTITION_INFORMATION_EX;
typedef struct _DRIVE_LAYOUT_INFORMATION_EX {
  DWORD PartitionStyle;
  DWORD PartitionCount;
  _ANONYMOUS_UNION union {
    DRIVE_LAYOUT_INFORMATION_MBR Mbr;
    DRIVE_LAYOUT_INFORMATION_GPT Gpt;
  };
  PARTITION_INFORMATION_EX PartitionEntry[1];
} DRIVE_LAYOUT_INFORMATION_EX,*PDRIVE_LAYOUT_INFORMATION_EX;
typedef struct {
  HANDLE FileHandle;
  LARGE_INTEGER StartingVcn;
  LARGE_INTEGER StartingLcn;
  DWORD ClusterCount;
} MOVE_FILE_DATA,*PMOVE_FILE_DATA;
typedef struct _PERF_BIN {
  DWORD NumberOfBins;
  DWORD TypeOfBin;
  BIN_RANGE BinsRanges[1];
} PERF_BIN,*PPERF_BIN;

#ifndef _NTDDSTOR_H_
typedef struct _PREVENT_MEDIA_REMOVAL {
  BOOLEAN PreventMediaRemoval;
} PREVENT_MEDIA_REMOVAL,*PPREVENT_MEDIA_REMOVAL;
#endif

typedef struct RETRIEVAL_POINTERS_BUFFER {
  DWORD ExtentCount;
  LARGE_INTEGER StartingVcn;
  struct {
    LARGE_INTEGER NextVcn;
    LARGE_INTEGER Lcn;
  } Extents[1];
} RETRIEVAL_POINTERS_BUFFER,*PRETRIEVAL_POINTERS_BUFFER;
typedef struct _REASSIGN_BLOCKS {
  WORD Reserved;
  WORD Count;
  DWORD BlockNumber[1];
} REASSIGN_BLOCKS,*PREASSIGN_BLOCKS;
typedef struct _SET_PARTITION_INFORMATION {
  BYTE PartitionType;
} SET_PARTITION_INFORMATION,*PSET_PARTITION_INFORMATION;
typedef struct {
  LARGE_INTEGER StartingLcn;
} STARTING_LCN_INPUT_BUFFER,*PSTARTING_LCN_INPUT_BUFFER;
typedef struct {
  LARGE_INTEGER StartingVcn;
} STARTING_VCN_INPUT_BUFFER,*PSTARTING_VCN_INPUT_BUFFER;
typedef struct _VERIFY_INFORMATION {
  LARGE_INTEGER StartingOffset;
  DWORD Length;
} VERIFY_INFORMATION,*PVERIFY_INFORMATION;
typedef struct {
  LARGE_INTEGER StartingLcn;
  LARGE_INTEGER BitmapSize;
  BYTE Buffer[1];
} VOLUME_BITMAP_BUFFER,*PVOLUME_BITMAP_BUFFER;
typedef struct {
  LARGE_INTEGER VolumeSerialNumber;
  LARGE_INTEGER NumberSectors;
  LARGE_INTEGER TotalClusters;
  LARGE_INTEGER FreeClusters;
  LARGE_INTEGER TotalReserved;
  DWORD BytesPerSector;
  DWORD BytesPerCluster;
  DWORD BytesPerFileRecordSegment;
  DWORD ClustersPerFileRecordSegment;
  LARGE_INTEGER MftValidDataLength;
  LARGE_INTEGER MftStartLcn;
  LARGE_INTEGER Mft2StartLcn;
  LARGE_INTEGER MftZoneStart;
  LARGE_INTEGER MftZoneEnd;
} NTFS_VOLUME_DATA_BUFFER, *PNTFS_VOLUME_DATA_BUFFER;
typedef struct {
  ULONG ByteCount;
  USHORT MajorVersion;
  USHORT MinorVersion;
} NTFS_EXTENDED_VOLUME_DATA, *PNTFS_EXTENDED_VOLUME_DATA;
typedef struct {
  LARGE_INTEGER FileReferenceNumber;
} NTFS_FILE_RECORD_INPUT_BUFFER, *PNTFS_FILE_RECORD_INPUT_BUFFER;
typedef struct {
  LARGE_INTEGER FileReferenceNumber;
  ULONG FileRecordLength;
  UCHAR FileRecordBuffer[1];
} NTFS_FILE_RECORD_OUTPUT_BUFFER, *PNTFS_FILE_RECORD_OUTPUT_BUFFER;

#ifdef __REACTOS__
#define IsRecognizedPartition(t)\
  (((t&PARTITION_NTFT)&&((t&~VALID_NTFT)==PARTITION_FAT_12))||\
    ((t&PARTITION_NTFT)&&((t&~VALID_NTFT)==PARTITION_FAT_16))||\
    ((t&PARTITION_NTFT)&&((t&~VALID_NTFT)==PARTITION_IFS))||\
    ((t&PARTITION_NTFT)&&((t&~VALID_NTFT)==PARTITION_HUGE))||\
    ((t&PARTITION_NTFT)&&((t&~VALID_NTFT)==PARTITION_FAT32))||\
    ((t&PARTITION_NTFT)&&((t&~VALID_NTFT)==PARTITION_FAT32_XINT13))||\
    ((t&PARTITION_NTFT)&&((t&~VALID_NTFT)==PARTITION_XINT13))||\
    ((t&~PARTITION_NTFT)==PARTITION_FAT_12)||\
    ((t&~PARTITION_NTFT)==PARTITION_FAT_16)||\
    ((t&~PARTITION_NTFT)==PARTITION_IFS)||\
    ((t&~PARTITION_NTFT)==PARTITION_HUGE)||\
    ((t&~PARTITION_NTFT)==PARTITION_FAT32)||\
    ((t&~PARTITION_NTFT)==PARTITION_FAT32_XINT13)||\
    ((t&~PARTITION_NTFT)==PARTITION_XINT13)||\
    ((t&~PARTITION_NTFT)==PARTITION_LINUX)||\
    ((t&~PARTITION_NTFT)==PARTITION_OLD_LINUX)||\
    ((t&~PARTITION_NTFT)==PARTITION_FREEBSD)||\
    ((t&~PARTITION_NTFT)==PARTITION_OPENBSD)||\
    ((t&~PARTITION_NTFT)==PARTITION_NETBSD))
#else
#define IsRecognizedPartition(t)\
  (((t&PARTITION_NTFT)&&((t&~VALID_NTFT)==PARTITION_FAT_12))||\
    ((t&PARTITION_NTFT)&&((t&~VALID_NTFT)==PARTITION_FAT_16))||\
    ((t&PARTITION_NTFT)&&((t&~VALID_NTFT)==PARTITION_IFS))||\
    ((t&PARTITION_NTFT)&&((t&~VALID_NTFT)==PARTITION_HUGE))||\
    ((t&PARTITION_NTFT)&&((t&~VALID_NTFT)==PARTITION_FAT32))||\
    ((t&PARTITION_NTFT)&&((t&~VALID_NTFT)==PARTITION_FAT32_XINT13))||\
    ((t&PARTITION_NTFT)&&((t&~VALID_NTFT)==PARTITION_XINT13))||\
    ((t&~PARTITION_NTFT)==PARTITION_FAT_12)||\
    ((t&~PARTITION_NTFT)==PARTITION_FAT_16)||\
    ((t&~PARTITION_NTFT)==PARTITION_IFS)||\
    ((t&~PARTITION_NTFT)==PARTITION_HUGE)||\
    ((t&~PARTITION_NTFT)==PARTITION_FAT32)||\
    ((t&~PARTITION_NTFT)==PARTITION_FAT32_XINT13)||\
    ((t&~PARTITION_NTFT)==PARTITION_XINT13))
#endif
#define IsContainerPartition(t)\
  (((t&PARTITION_NTFT)&&((t&~VALID_NTFT)==PARTITION_EXTENDED))||\
    ((t&PARTITION_NTFT)&&((t&~VALID_NTFT)==PARTITION_XINT13_EXTENDED))||\
    ((t&~PARTITION_NTFT)==PARTITION_EXTENDED)||\
    ((t&~PARTITION_NTFT)==PARTITION_XINT13_EXTENDED))

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef __cplusplus
}
#endif
#endif /* _WINIOCTL_ */

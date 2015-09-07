/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/include/bl.h
 * PURPOSE:         Main Boot Library Header
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
*/

#ifndef _BL_H
#define _BL_H

/* INCLUDES ******************************************************************/

/* C Headers */
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>

/* NT Base Headers */
#include <ntifs.h>

/* NDK Headers */
#include <ntndk.h>

/* UEFI Headers */
#include <Uefi.h>
#include <DevicePath.h>
#include <LoadedImage.h>
#include <GraphicsOutput.h>
#include <UgaDraw.h>

/* DEFINES *******************************************************************/

#define BL_APPLICATION_FLAG_CONVERTED_FROM_EFI          0x01

#define BL_APP_ENTRY_SIGNATURE                          "BTAPENT"

#define BOOT_APPLICATION_SIGNATURE_1                    'TOOB'
#define BOOT_APPLICATION_SIGNATURE_2                    ' PPA'

#define BOOT_MEMORY_TRANSLATION_TYPE_PHYSICAL           0
#define BOOT_MEMORY_TRANSLATION_TYPE_VIRTUAL            1

#define BOOT_APPLICATION_VERSION                        2
#define BL_MEMORY_DATA_VERSION                          1
#define BL_RETURN_ARGUMENTS_VERSION                     1
#define BL_FIRMWARE_DESCRIPTOR_VERSION                  2

#define BL_APPLICATION_ENTRY_FLAG_NO_GUID               0x01
#define BL_APPLICATION_ENTRY_REBOOT_ON_ERROR            0x20

#define BL_CONTEXT_PAGING_ON                            1
#define BL_CONTEXT_INTERRUPTS_ON                        2

#define BL_MM_FLAG_USE_FIRMWARE_FOR_MEMORY_MAP_BUFFERS  0x01
#define BL_MM_FLAG_REQUEST_COALESCING                   0x02

#define BL_MM_ADD_DESCRIPTOR_COALESCE_FLAG              0x01
#define BL_MM_ADD_DESCRIPTOR_TRUNCATE_FLAG              0x02
#define BL_MM_ADD_DESCRIPTOR_NEVER_COALESCE_FLAG        0x10
#define BL_MM_ADD_DESCRIPTOR_NEVER_TRUNCATE_FLAG        0x20
#define BL_MM_ADD_DESCRIPTOR_UPDATE_LIST_POINTER_FLAG   0x2000

#define BL_MM_REQUEST_DEFAULT_TYPE                      1
#define BL_MM_REQUEST_TOP_DOWN_TYPE                     2

#define BL_MM_REMOVE_VIRTUAL_REGION_FLAG                0x80000000

#define BL_LIBRARY_FLAG_NO_DISPLAY                      0x01
#define BL_LIBRARY_FLAG_REINITIALIZE                    0x02
#define BL_LIBRARY_FLAG_REINITIALIZE_ALL                0x04
#define BL_LIBRARY_FLAG_ZERO_HEAP_ALLOCATIONS_ON_FREE   0x10
#define BL_LIBRARY_FLAG_INITIALIZATION_COMPLETED        0x20
#define BL_LIBRARY_FLAG_NO_GRAPHICS_CONSOLE             0x800

#define BL_DISPLAY_GRAPHICS_FORCED_VIDEO_MODE_FLAG      0x01
#define BL_DISPLAY_GRAPHICS_FORCED_HIGH_RES_MODE_FLAG   0x02

#define BL_FS_REGISTER_AT_HEAD_FLAG                     1

#define BL_MEMORY_CLASS_SHIFT                           28

/* ENUMERATIONS **************************************************************/

typedef enum _BL_COLOR
{
    Black,
    Blue,
    Green,
    Cyan,
    Red,
    Magenta,
    Brown,
    LtGray,
    Gray,
    LtBlue,
    LtGreen,
    LtCyan,
    LtRed,
    LtMagenta,
    Yellow,
    White
} BL_COLOR, *PBL_COLOR;

typedef enum _BL_MEMORY_DESCRIPTOR_TYPE
{
    BlMdPhysical,
    BlMdVirtual,
} BL_MEMORY_DESCRIPTOR_TYPE;

typedef enum _BL_TRANSLATION_TYPE
{
    BlNone,
    BlVirtual,
    BlPae,
    BlMax
} BL_TRANSLATION_TYPE;

typedef enum _BL_ARCH_MODE
{
    BlProtectedMode,
    BlRealMode
} BL_ARCH_MODE;

//
// Boot Device Types
//
typedef enum _BL_DEVICE_TYPE
{
    LocalDevice = 0,
    PartitionDevice = 2,
    UdpDevice = 4,
    HardDiskDevice = 6
} BL_DEVICE_TYPE;

//
// Local Device Types
//
typedef enum _BL_LOCAL_DEVICE_TYPE
{
    FloppyDevice = 1,
    CdRomDevice = 2,
    RamDiskDevice = 3,
} BL_LOCAL_DEVICE_TYPE;

//
// Partition types
//
typedef enum _BL_PARTITION_TYPE
{
    GptPartition,
    MbrPartition,
    RawPartition,
} BL_PARTITION_TYPE;

//
// File Path Types
//
typedef enum _BL_PATH_TYPE
{
    EfiPath = 4
} BL_PATH_TYPE;

//
// Classes of Memory
//
typedef enum _BL_MEMORY_CLASS
{
    BlLoaderClass = 0xD,
    BlApplicationClass,
    BlSystemClass
} BL_MEMORY_CLASS;

//
// Types of Memory
//
typedef enum _BL_MEMORY_TYPE
{
    //
    // Loader Memory
    //
    BlLoaderMemory = 0xD0000002,
    BlLoaderHeap = 0xD0000005,
    BlLoaderPageDirectory = 0xD0000006,
    BlLoaderReferencePage = 0xD0000007,
    BlLoaderRamDisk = 0xD0000008,
    BlLoaderData = 0xD000000A,
    BlLoaderSelfMap = 0xD000000F,

    //
    // Application Memory
    //
    BlApplicationData = 0xE0000004,

    //
    // System Memory
    //
    BlConventionalMemory = 0xF0000001,
    BlUnusableMemory = 0xF0000002,
    BlReservedMemory = 0xF0000003,
    BlEfiBootMemory = 0xF0000004,
    BlEfiRuntimeMemory = 0xF0000006,
    BlAcpiReclaimMemory = 0xF0000008,
    BlAcpiNvsMemory = 0xF0000009,
    BlDeviceIoMemory = 0xF000000A,
    BlDevicePortMemory = 0xF000000B,
    BlPalMemory = 0xF000000C,
} BL_MEMORY_TYPE;

typedef enum _BL_MEMORY_ATTR
{
    //
    // Memory Caching Attributes
    //
    BlMemoryUncached =          0x00000001,
    BlMemoryWriteCombined =     0x00000002,
    BlMemoryWriteThrough =      0x00000004,
    BlMemoryWriteBack =         0x00000008,
    BlMemoryUncachedExported =  0x00000010,
    BlMemoryValidCacheAttributes            = BlMemoryUncached | BlMemoryWriteCombined | BlMemoryWriteThrough | BlMemoryWriteBack | BlMemoryUncachedExported,
    BlMemoryValidCacheAttributeMask         = 0x000000FF,

    //
    // Memory Protection Attributes
    //
    BlMemoryWriteProtected =    0x00000100,
    BlMemoryReadProtected =     0x00000200,
    BlMemoryExecuteProtected =  0x00000400,
    BlMemoryValidProtectionAttributes       = BlMemoryWriteProtected | BlMemoryReadProtected | BlMemoryExecuteProtected,
    BlMemoryValidProtectionAttributeMask    = 0x0000FF00,

    //
    // Memory Allocation Attributes
    //
    BlMemoryNonFixed =          0x00020000,
    BlMemoryFixed =             0x00040000,
    BlMemoryValidAllocationAttributes       = BlMemoryNonFixed | BlMemoryFixed,
    BlMemoryValidAllocationAttributeMask    = 0x00FF0000,

    //
    // Memory Type Attributes
    //
    BlMemoryRuntime =           0x01000000,
    BlMemoryCoalesced =         0x02000000,
    BlMemoryUpdate =            0x04000000,
    BlMemoryNonFirmware =       0x08000000,
    BlMemorySpecial =           0x20000000,
    BlMemoryFirmware =          0x80000000,
    BlMemoryValidTypeAttributes             = BlMemoryRuntime | BlMemoryCoalesced | BlMemoryUpdate | BlMemoryNonFirmware | BlMemorySpecial | BlMemoryFirmware,
    BlMemoryValidTypeAttributeMask          = 0xFF000000,
} BL_MEMORY_ATTR;

/* CALLBACKS *****************************************************************/

typedef
NTSTATUS
(*PBL_FS_INIT_CALLBACK) (
    VOID
    );

typedef
NTSTATUS
(*PBL_FS_DESTROY_CALLBACK) (
    VOID
    );

typedef
NTSTATUS
(*PBL_FS_MOUNT_CALLBACK) (
    VOID
    );

typedef
NTSTATUS
(*PBL_FS_PURGE_CALLBACK) (
    VOID
    );

typedef
NTSTATUS
(*PBL_FILE_DESTROY_CALLBACK) (
    _In_ PVOID Entry
    );

struct _BL_TEXT_CONSOLE;
struct _BL_DISPLAY_STATE;
typedef
NTSTATUS
(*PCONSOLE_DESTRUCT) (
    _In_ struct _BL_TEXT_CONSOLE* Console
    );

typedef
NTSTATUS
(*PCONSOLE_REINITIALIZE) (
    _In_ struct _BL_TEXT_CONSOLE* Console
    );

typedef
NTSTATUS
(*PCONSOLE_GET_TEXT_STATE) (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _Out_ struct _BL_DISPLAY_STATE* TextState
    );

typedef
NTSTATUS
(*PCONSOLE_SET_TEXT_STATE) (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _In_ ULONG Flags,
    _In_ struct _BL_DISPLAY_STATE* TextState
    );

typedef
NTSTATUS
(*PCONSOLE_GET_TEXT_RESOLUTION) (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _Out_ PULONG TextResolution
    );

typedef
NTSTATUS
(*PCONSOLE_SET_TEXT_RESOLUTION) (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _In_ ULONG NewTextResolution,
    _Out_ PULONG OldTextResolution
    );

typedef
NTSTATUS
(*PCONSOLE_CLEAR_TEXT) (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _In_ ULONG Attribute
    );

typedef
NTSTATUS
(*PCONSOLE_WRITE_TEXT) (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _In_ PCHAR Text,
    _In_ ULONG Attribute
    );

/* DATA STRUCTURES ***********************************************************/

typedef struct _BL_LIBRARY_PARAMETERS
{
    ULONG LibraryFlags;
    ULONG TranslationType;
    ULONG MinimumAllocationCount;
    ULONG MinimumHeapSize;
    ULONG HeapAllocationAttributes;
    PWCHAR ApplicationBaseDirectory;
    ULONG DescriptorCount;
    PWCHAR FontBaseDirectory;
} BL_LIBRARY_PARAMETERS, *PBL_LIBRARY_PARAMETERS;

/* This should eventually go into a more public header */
typedef struct _BOOT_APPLICATION_PARAMETER_BLOCK
{
    /* This header tells the library what image we're dealing with */
    ULONG Signature[2];
    ULONG Version;
    ULONG Size;
    ULONG ImageType;
    ULONG MemoryTranslationType;

    /* Where is the image located */
    ULONGLONG ImageBase;
    ULONG ImageSize;

    /* Offset to BL_MEMORY_DATA */
    ULONG MemoryDataOffset;

    /* Offset to BL_APPLICATION_ENTRY */
    ULONG AppEntryOffset;

    /* Offset to BL_DEVICE_DESCRPIPTOR */
    ULONG BootDeviceOffset;

    /* Offset to BL_FIRMWARE_PARAMETERS */
    ULONG FirmwareParametersOffset;

    /* Offset to BL_RETURN_ARGUMENTS */
    ULONG ReturnArgumentsOffset;
} BOOT_APPLICATION_PARAMETER_BLOCK, *PBOOT_APPLICATION_PARAMETER_BLOCK;

typedef struct _BL_MEMORY_DATA
{
    ULONG Version;
    ULONG MdListOffset;
    ULONG DescriptorCount;
    ULONG DescriptorSize;
    ULONG DescriptorOffset;
} BL_MEMORY_DATA, *PBL_MEMORY_DATA;

typedef struct _BL_FIRMWARE_DESCRIPTOR
{
    ULONG Version;
    ULONG Unknown;
    EFI_HANDLE ImageHandle;
    EFI_SYSTEM_TABLE *SystemTable;
} BL_FIRMWARE_DESCRIPTOR, *PBL_FIRMWARE_DESCRIPTOR;

typedef struct _BL_RETURN_ARGUMENTS
{
    ULONG Version;
    NTSTATUS Status;
    ULONG ReturnArgumentData[5];
} BL_RETURN_ARGUMENTS, *PBL_RETURN_ARGUMENTS;

typedef struct _BL_MEMORY_DESCRIPTOR
{
    LIST_ENTRY ListEntry;
    union
    {
        struct
        {
            ULONGLONG BasePage;
            ULONGLONG VirtualPage;
        };
        struct
        {
            ULONGLONG BaseAddress;
            ULONGLONG VirtualAddress;
        };
    };
    ULONGLONG PageCount;
    ULONG Flags;
    BL_MEMORY_TYPE Type;
} BL_MEMORY_DESCRIPTOR, *PBL_MEMORY_DESCRIPTOR;

typedef struct _BL_BCD_OPTION
{
    ULONG Type;
    ULONG DataOffset;
    ULONG DataSize;
    ULONG ListOffset;
    ULONG NextEntryOffset;
    ULONG Empty;
} BL_BCD_OPTION, *PBL_BCD_OPTION;

typedef struct _BL_APPLICATION_ENTRY
{
    CHAR Signature[8];
    ULONG Flags;
    GUID Guid;
    ULONG Unknown[4];
    BL_BCD_OPTION BcdData;
} BL_APPLICATION_ENTRY, *PBL_APPLICATION_ENTRY;

typedef struct _BL_HARDDISK_DEVICE
{
    ULONG PartitionType;
    union
    {
        struct
        {
            ULONG PartitionSignature;
        } Mbr;

        struct
        {
            GUID PartitionSignature;
        } Gpt;

        struct
        {
            ULONG DiskNumber;
        } Raw;
    };
} BL_HARDDISK_DEVICE;

typedef struct _BL_LOCAL_DEVICE
{
    ULONG Type;
    union
    {
        struct
        {
            ULONG DriveNumber;
        } FloppyDisk;

        BL_HARDDISK_DEVICE HardDisk;

        struct
        {
            PHYSICAL_ADDRESS ImageBase;
            LARGE_INTEGER ImageSize;
            ULONG ImageOffset;
        } RamDisk;
    };
} BL_LOCAL_DEVICE;

typedef struct _BL_DEVICE_DESCRIPTOR
{
    ULONG Size;
    ULONG Flags;
    DEVICE_TYPE DeviceType;
    ULONG Unknown;
    union
    {
        BL_LOCAL_DEVICE Local;

        struct
        {
            ULONG Unknown;
        } Remote;

        struct
        {
            union
            {
                ULONG PartitionNumber;
            } Mbr;

            union
            {
                GUID PartitionGuid;
            } Gpt;

            BL_LOCAL_DEVICE Disk;
        } Partition;
    };
} BL_DEVICE_DESCRIPTOR, *PBL_DEVICE_DESCRIPTOR;

typedef struct _BL_FILE_PATH_DESCRIPTOR
{
    ULONG Version;
    ULONG Length;
    ULONG PathType;
    UCHAR Path[ANYSIZE_ARRAY];
} BL_FILE_PATH_DESCRIPTOR, *PBL_FILE_PATH_DESCRIPTOR;

typedef struct _BL_WINDOWS_LOAD_OPTIONS
{
    CHAR Signature[8];
    ULONG Version;
    ULONG Length;
    ULONG OsPathOffset;
    WCHAR LoadOptions[ANYSIZE_ARRAY];
} BL_WINDOWS_LOAD_OPTIONS, *PBL_WINDOWS_LOAD_OPTIONS;

typedef struct _BL_ARCH_CONTEXT
{
    BL_ARCH_MODE Mode;
    BL_TRANSLATION_TYPE TranslationType;
    ULONG ContextFlags;
} BL_ARCH_CONTEXT, *PBL_ARCH_CONTEXT;

typedef struct _BL_MEMORY_DESCRIPTOR_LIST
{
    LIST_ENTRY ListHead;
    PLIST_ENTRY First;
    PLIST_ENTRY This;
    ULONG Type;
} BL_MEMORY_DESCRIPTOR_LIST, *PBL_MEMORY_DESCRIPTOR_LIST;

typedef struct _BL_ADDRESS_RANGE
{
    ULONGLONG Minimum;
    ULONGLONG Maximum;
} BL_ADDRESS_RANGE, *PBL_ADDRESS_RANGE;

typedef struct _BL_FILE_ENTRY
{
    ULONG DeviceIndex;
    PBL_FILE_DESTROY_CALLBACK DestroyCallback;
} BL_FILE_ENTRY, *PBL_FILE_ENTRY;

typedef struct _BL_DEVICE_ENTRY
{
    ULONG ReferenceCount;
} BL_DEVICE_ENTRY, *PBL_DEVICE_ENTRY;

typedef struct _BL_FILE_SYSTEM_ENTRY
{
    LIST_ENTRY ListEntry;
    PBL_FS_INIT_CALLBACK InitCallback;
    PBL_FS_DESTROY_CALLBACK DestroyCallback;
    PBL_FS_MOUNT_CALLBACK MountCallback;
    PBL_FS_PURGE_CALLBACK PurgeCallback;
} BL_FILE_SYSTEM_ENTRY, *PBL_FILE_SYSTEM_ENTRY;

typedef struct _BL_FILE_SYSTEM_REGISTRATION_TABLE
{
    PBL_FS_INIT_CALLBACK Init;
    PBL_FS_DESTROY_CALLBACK Destroy;
    PBL_FS_MOUNT_CALLBACK Mount;
    PBL_FS_PURGE_CALLBACK Purge;
} BL_FILE_SYSTEM_REGISTRATION_TABLE;

typedef struct _BL_DISPLAY_STATE
{
    ULONG BgColor;
    ULONG FgColor;
    ULONG XPos;
    ULONG YPos;
    ULONG CursorVisible;
} BL_DISPLAY_STATE, *PBL_DISPLAY_STATE;

typedef struct _BL_DISPLAY_MODE
{
    ULONG HRes;
    ULONG VRes;
    ULONG HRes2;
} BL_DISPLAY_MODE, *PBL_DISPLAY_MODE;

typedef struct _BL_TEXT_CONSOLE_VTABLE
{
    PCONSOLE_DESTRUCT Destruct;
    PCONSOLE_REINITIALIZE Reinitialize;
    PCONSOLE_GET_TEXT_STATE GetTextState;
    PCONSOLE_SET_TEXT_STATE SetTextState;
    PCONSOLE_GET_TEXT_RESOLUTION GetTextResolution;
    PCONSOLE_SET_TEXT_RESOLUTION SetTextResolution;
    PCONSOLE_CLEAR_TEXT ClearText;
    PCONSOLE_WRITE_TEXT WriteText;
} BL_TEXT_CONSOLE_VTABLE, *PBL_TEXT_CONSOLE_VTABLE;

typedef struct _BL_GRAPHICS_CONSOLE_VTABLE
{
    BL_TEXT_CONSOLE_VTABLE Text;
    /// more for graphics ///
} BL_GRAPHICS_CONSOLE_VTABLE, *PBL_GRAPHICS_CONSOLE_VTABLE;

typedef struct _BL_TEXT_CONSOLE
{
    PBL_TEXT_CONSOLE_VTABLE Callbacks;
    BL_DISPLAY_STATE State;
    BL_DISPLAY_MODE DisplayMode;
    BOOLEAN Active;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* Protocol;
    ULONG Mode;
    EFI_SIMPLE_TEXT_OUTPUT_MODE OldMode;
} BL_TEXT_CONSOLE, *PBL_TEXT_CONSOLE;

typedef enum _BL_GRAPHICS_CONSOLE_TYPE
{
    BlGopConsole,
    BlUgaConsole
} BL_GRAPHICS_CONSOLE_TYPE;

typedef struct _BL_GRAPHICS_CONSOLE
{
    BL_TEXT_CONSOLE TextConsole;
    BL_DISPLAY_MODE DisplayMode;
    ULONG PixelDepth;
    ULONG FgColor;
    ULONG BgColor;
    BL_DISPLAY_MODE OldDisplayMode;
    ULONG OldPixelDepth;
    EFI_HANDLE Handle;
    BL_GRAPHICS_CONSOLE_TYPE Type;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* Protocol;
    PVOID FrameBuffer;
    ULONG FrameBufferSize;
    ULONG PixelsPerScanLine;
    ULONG Mode;
    ULONG OldMode;
} BL_GRAPHICS_CONSOLE, *PBL_GRAPHICS_CONSOLE;

typedef struct _BL_REMOTE_CONSOLE
{
    BL_TEXT_CONSOLE TextConsole;
} BL_REMOTE_CONSOLE, *PBL_REMOTE_CONSOLE;

/* INLINE ROUTINES ***********************************************************/

FORCEINLINE
VOID
BlSetupDefaultParameters (
    _Out_ PBL_LIBRARY_PARAMETERS LibraryParameters
    )
{
    BL_LIBRARY_PARAMETERS DefaultParameters =
    {
        0x20,
        BlVirtual,
        1024,
        2 * 1024 * 1024,
        0,
        NULL,
        0,
        NULL
    };

    /* Copy the defaults */
    RtlCopyMemory(LibraryParameters, &DefaultParameters, sizeof(*LibraryParameters));
}

FORCEINLINE
VOID
MmMdInitializeListHead (
    _In_ PBL_MEMORY_DESCRIPTOR_LIST List
    )
{
    /* Initialize the list */
    InitializeListHead(&List->ListHead);
    List->First = &List->ListHead;
    List->This = NULL;
}

/* INITIALIZATION ROUTINES ***************************************************/

NTSTATUS
BlInitializeLibrary(
    _In_ PBOOT_APPLICATION_PARAMETER_BLOCK BootAppParameters,
    _In_ PBL_LIBRARY_PARAMETERS LibraryParameters
    );

NTSTATUS
BlpArchInitialize (
    _In_ ULONG Phase
    );

NTSTATUS
BlpFwInitialize (
    _In_ ULONG Phase,
    _In_ PBL_FIRMWARE_DESCRIPTOR FirmwareParameters
    );

NTSTATUS
BlpMmInitialize (
    _In_ PBL_MEMORY_DATA MemoryData,
    _In_ BL_TRANSLATION_TYPE TranslationType,
    _In_ PBL_LIBRARY_PARAMETERS LibraryParameters
    );

NTSTATUS
MmBaInitialize (
    VOID
    );

NTSTATUS
MmPaInitialize (
    _In_ PBL_MEMORY_DATA MemoryData,
    _In_ ULONG MinimumPages
    );

NTSTATUS
MmArchInitialize (
    _In_ ULONG Phase,
    _In_ PBL_MEMORY_DATA MemoryData,
    _In_ BL_TRANSLATION_TYPE TranslationType,
    _In_ BL_TRANSLATION_TYPE LibraryTranslationType
    );

NTSTATUS
MmHaInitialize (
    _In_ ULONG HeapSize,
    _In_ ULONG HeapAttributes
    );

VOID
MmMdInitialize (
    _In_ ULONG Phase,
    _In_ PBL_LIBRARY_PARAMETERS LibraryParameters
    );

NTSTATUS
BlpDeviceInitialize (
    VOID
    );

NTSTATUS
BlpIoInitialize (
    VOID
    );

NTSTATUS
BlpFileInitialize (
    VOID
    );

NTSTATUS
FatInitialize (
    VOID
    );

NTSTATUS
BlpDisplayInitialize (
    _In_ ULONG Flags
    );

VOID
BlDestroyLibrary (
    VOID
    );

/* FIRMWARE ROUTINES *********************************************************/

VOID
EfiPrintf (
    _In_ PWCHAR Format,
    ...
    );

NTSTATUS
EfiAllocatePages (
    _In_ ULONG Type,
    _In_ ULONG Pages,
    _Inout_ EFI_PHYSICAL_ADDRESS* Memory
    );

NTSTATUS
EfiStall (
    _In_ ULONG StallTime
    );

NTSTATUS
EfiConOutQueryMode (
    _In_ SIMPLE_TEXT_OUTPUT_INTERFACE *TextInterface,
    _In_ ULONG Mode,
    _In_ UINTN* Columns,
    _In_ UINTN* Rows
    );

NTSTATUS
EfiConOutSetMode (
    _In_ SIMPLE_TEXT_OUTPUT_INTERFACE *TextInterface,
    _In_ ULONG Mode
    );

VOID
EfiConOutReadCurrentMode (
    _In_ SIMPLE_TEXT_OUTPUT_INTERFACE *TextInterface,
    _Out_ EFI_SIMPLE_TEXT_OUTPUT_MODE* Mode
    );

NTSTATUS
EfiConOutSetAttribute (
    _In_ SIMPLE_TEXT_OUTPUT_INTERFACE *TextInterface,
    _In_ ULONG Attribute
    );

NTSTATUS
EfiConOutSetCursorPosition (
    _In_ SIMPLE_TEXT_OUTPUT_INTERFACE *TextInterface,
    _In_ ULONG Column,
    _In_ ULONG Row
    );

NTSTATUS
EfiConOutEnableCursor (
    _In_ SIMPLE_TEXT_OUTPUT_INTERFACE *TextInterface,
    _In_ BOOLEAN Visible
    );

NTSTATUS
EfiLocateHandleBuffer (
    _In_ EFI_LOCATE_SEARCH_TYPE SearchType,
    _In_ EFI_GUID *Protocol,
    _Inout_ PULONG HandleCount,
    _Inout_ EFI_HANDLE** Buffer
    );

NTSTATUS
EfiOpenProtocol (
    _In_ EFI_HANDLE Handle,
    _In_ EFI_GUID *Protocol,
    _Out_ PVOID* Interface
    );

NTSTATUS
EfiCloseProtocol (
    _In_ EFI_HANDLE Handle,
    _In_ EFI_GUID *Protocol
    );

NTSTATUS
EfiGopGetCurrentMode (
    _In_ EFI_GRAPHICS_OUTPUT_PROTOCOL *GopInterface,
    _Out_ UINTN* Mode,
    _Out_ EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Information
    );

NTSTATUS
EfiGopSetMode (
    _In_ EFI_GRAPHICS_OUTPUT_PROTOCOL *GopInterface,
    _In_ ULONG Mode
    );

VOID
EfiGopGetFrameBuffer (
    _In_ EFI_GRAPHICS_OUTPUT_PROTOCOL *GopInterface,
    _Out_ PHYSICAL_ADDRESS* FrameBuffer,
    _Out_ UINTN *FrameBufferSize
    );

VOID
EfiResetSystem (
    _In_ EFI_RESET_TYPE ResetType
    );

/* PLATFORM TIMER ROUTINES ***************************************************/

NTSTATUS
BlpTimeCalibratePerformanceCounter (
    VOID
    );

/* UTILITY ROUTINES **********************************************************/

EFI_STATUS
EfiGetEfiStatusCode(
    _In_ NTSTATUS Status
    );

NTSTATUS
EfiGetNtStatusCode (
    _In_ EFI_STATUS EfiStatus
    );

NTSTATUS
BlUtlInitialize (
    VOID
    );

VOID
BlFwReboot (
    VOID
    );

/* BCD ROUTINES **************************************************************/

ULONG
BlGetBootOptionSize (
    _In_ PBL_BCD_OPTION BcdOption
    );

/* CONTEXT ROUTINES **********************************************************/

VOID
BlpArchSwitchContext (
    _In_ BL_ARCH_MODE NewMode
    );

/* MEMORY DESCRIPTOR ROUTINES ************************************************/

VOID
MmMdFreeList(
    _In_ PBL_MEMORY_DESCRIPTOR_LIST MdList
    );

PBL_MEMORY_DESCRIPTOR
MmMdInitByteGranularDescriptor (
    _In_ ULONG Flags,
    _In_ BL_MEMORY_TYPE Type,
    _In_ ULONGLONG BasePage,
    _In_ ULONGLONG VirtualPage,
    _In_ ULONGLONG PageCount
    );

VOID
MmMdFreeGlobalDescriptors (
    VOID
    );

NTSTATUS
MmMdAddDescriptorToList (
    _In_ PBL_MEMORY_DESCRIPTOR_LIST MdList,
    _In_ PBL_MEMORY_DESCRIPTOR MemoryDescriptor,
    _In_ ULONG Flags
    );

VOID
MmMdRemoveDescriptorFromList (
    _In_ PBL_MEMORY_DESCRIPTOR_LIST MdList,
    _In_ PBL_MEMORY_DESCRIPTOR Entry
    );

BOOLEAN
MmMdFindSatisfyingRegion (
    _In_ PBL_MEMORY_DESCRIPTOR Descriptor,
    _Out_ PBL_MEMORY_DESCRIPTOR NewDescriptor,
    _In_ ULONGLONG Pages,
    _In_ PBL_ADDRESS_RANGE BaseRange,
    _In_ PBL_ADDRESS_RANGE VirtualRange,
    _In_ BOOLEAN TopDown,
    _In_ BL_MEMORY_TYPE MemoryType,
    _In_ ULONG Flags,
    _In_ ULONG Alignment
    );

NTSTATUS
MmMdRemoveRegionFromMdlEx (
    __in PBL_MEMORY_DESCRIPTOR_LIST MdList,
    __in ULONG Flags,
    __in ULONGLONG BasePage,
    __in ULONGLONG PageCount,
    __in PBL_MEMORY_DESCRIPTOR_LIST NewMdList
    );

NTSTATUS
MmMdFreeDescriptor (
    _In_ PBL_MEMORY_DESCRIPTOR MemoryDescriptor
    );

/* PAGE ALLOCATOR ROUTINES ***************************************************/

NTSTATUS
MmPapAllocatePagesInRange (
    _Inout_ PULONG PhysicalAddress,
    _In_ BL_MEMORY_TYPE MemoryType,
    _In_ ULONGLONG Pages,
    _In_ ULONG Attributes,
    _In_ ULONG Alignment,
    _In_opt_ PBL_ADDRESS_RANGE Range,
    _In_ ULONG Type
    );

NTSTATUS
MmFwGetMemoryMap (
    _Out_ PBL_MEMORY_DESCRIPTOR_LIST MemoryMap,
    _In_ ULONG Flags
    );

/* VIRTUAL MEMORY ROUTINES ***************************************************/

NTSTATUS
BlMmMapPhysicalAddressEx (
    _In_ PVOID* VirtualAddress,
    _In_ ULONG Attributes,
    _In_ ULONGLONG Size,
    _In_ PHYSICAL_ADDRESS PhysicalAddress
    );

/* HEAP ALLOCATOR ROUTINES ***************************************************/

PVOID
BlMmAllocateHeap (
    _In_ ULONG Size
    );

NTSTATUS
BlMmFreeHeap (
    _In_ PVOID Buffer
    );

/* DISPLAY ROUTINES **********************************************************/

VOID
BlDisplayGetTextCellResolution (
    _Out_ PULONG TextWidth,
    _Out_ PULONG TextHeight
    );

/* TExT CONSOLE ROUTINES *****************************************************/

NTSTATUS
ConsoleTextLocalDestruct (
    _In_ struct _BL_TEXT_CONSOLE* Console
    );

NTSTATUS
ConsoleTextLocalReinitialize (
    _In_ struct _BL_TEXT_CONSOLE* Console
    );

NTSTATUS
ConsoleTextBaseGetTextState (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _Out_ PBL_DISPLAY_STATE TextState
    );

NTSTATUS
ConsoleTextLocalSetTextState (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _In_ ULONG Flags,
    _In_ PBL_DISPLAY_STATE TextState
    );

NTSTATUS
ConsoleTextBaseGetTextResolution (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _Out_ PULONG TextResolution
    );

NTSTATUS
ConsoleTextLocalSetTextResolution (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _In_ ULONG NewTextResolution,
    _Out_ PULONG OldTextResolution
    );

NTSTATUS
ConsoleTextLocalClearText (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _In_ ULONG Attribute
    );

NTSTATUS
ConsoleTextLocalWriteText (
    _In_ struct _BL_TEXT_CONSOLE* Console,
    _In_ PCHAR Text,
    _In_ ULONG Attribute
    );

NTSTATUS
ConsoleTextLocalConstruct (
    _In_ PBL_TEXT_CONSOLE TextConsole,
    _In_ BOOLEAN Activate
    );

BOOLEAN
ConsolepFindResolution (
    _In_ PBL_DISPLAY_MODE Mode,
    _In_ PBL_DISPLAY_MODE List,
    _In_ ULONG MaxIndex
    );

VOID
ConsoleFirmwareTextClose (
    _In_ PBL_TEXT_CONSOLE TextConsole
    );

NTSTATUS
ConsoleFirmwareTextOpen (
    _In_ PBL_TEXT_CONSOLE TextConsole
    );

NTSTATUS
ConsoleFirmwareTextSetState (
    _In_ PBL_TEXT_CONSOLE TextConsole,
    _In_ UCHAR Mask,
    _In_ PBL_DISPLAY_STATE State
    );

NTSTATUS
ConsoleGraphicalConstruct (
    _In_ PBL_GRAPHICS_CONSOLE GraphicsConsole
    );

NTSTATUS
ConsoleCreateRemoteConsole (
    _In_ PBL_TEXT_CONSOLE* TextConsole
    );

NTSTATUS
ConsoleEfiGraphicalOpenProtocol (
    _In_ PBL_GRAPHICS_CONSOLE GraphicsConsole,
    _In_ BL_GRAPHICS_CONSOLE_TYPE Type
    );

VOID
ConsoleFirmwareGraphicalClose (
    _In_ PBL_GRAPHICS_CONSOLE GraphicsConsole
    );

NTSTATUS
ConsoleFirmwareGraphicalEnable (
    _In_ PBL_GRAPHICS_CONSOLE GraphicsConsole
    );

NTSTATUS
ConsoleEfiUgaOpen (
    _In_ PBL_GRAPHICS_CONSOLE GraphicsConsole
    );

VOID
ConsoleEfiUgaClose (
    _In_ PBL_GRAPHICS_CONSOLE GraphicsConsole
    );

VOID
ConsoleEfiGopClose (
    _In_ PBL_GRAPHICS_CONSOLE GraphicsConsole
    );

NTSTATUS
ConsoleEfiGopOpen (
    _In_ PBL_GRAPHICS_CONSOLE GraphicsConsole
    );

NTSTATUS
ConsoleEfiGopEnable (
    _In_ PBL_GRAPHICS_CONSOLE GraphicsConsole
    );

NTSTATUS
ConsoleEfiUgaSetResolution  (
    _In_ PBL_GRAPHICS_CONSOLE GraphicsConsole,
    _In_ PBL_DISPLAY_MODE DisplayMode,
    _In_ ULONG DisplayModeCount
    );

extern ULONG MmDescriptorCallTreeCount;
extern ULONG BlpApplicationFlags;
extern BL_LIBRARY_PARAMETERS BlpLibraryParameters;
extern BL_TRANSLATION_TYPE  MmTranslationType;
extern PBL_ARCH_CONTEXT CurrentExecutionContext;
extern PBL_DEVICE_DESCRIPTOR BlpBootDevice;
extern BL_APPLICATION_ENTRY BlpApplicationEntry;
extern SIMPLE_TEXT_OUTPUT_INTERFACE *EfiConOut;
extern EFI_GUID EfiGraphicsOutputProtocol;
extern EFI_GUID EfiUgaDrawProtocol;
extern EFI_GUID EfiLoadedImageProtocol;
extern EFI_GUID EfiDevicePathProtocol;
extern EFI_GUID EfiSimpleTextInputExProtocol;
extern ULONG ConsoleGraphicalResolutionListFlags;
extern BL_DISPLAY_MODE ConsoleGraphicalResolutionList[];
extern BL_DISPLAY_MODE ConsoleTextResolutionList[];
extern ULONG ConsoleGraphicalResolutionListSize;
extern PVOID DspRemoteInputConsole;
#endif

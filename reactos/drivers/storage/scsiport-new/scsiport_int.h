/*
 * SCSI_PORT_TIMER_STATES
 *
 * DESCRIPTION
 *	An enumeration containing the states in the timer DFA
 */

#define VERSION "0.0.3"

#ifndef PAGE_ROUND_UP
#define PAGE_ROUND_UP(x) ( (((ULONG_PTR)x)%PAGE_SIZE) ? ((((ULONG_PTR)x)&(~(PAGE_SIZE-1)))+PAGE_SIZE) : ((ULONG_PTR)x) )
#endif
#ifndef ROUND_UP
#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#endif

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))

/* Defines how many logical unit arrays will be in a device extension */
#define LUS_NUMBER 8

/* Flags */
#define SCSI_PORT_SCAN_IN_PROGRESS 0x8000

typedef enum _SCSI_PORT_TIMER_STATES
{
  IDETimerIdle,
  IDETimerCmdWait,
  IDETimerResetWaitForBusyNegate,
  IDETimerResetWaitForDrdyAssert
} SCSI_PORT_TIMER_STATES;


typedef struct _SCSI_PORT_DEVICE_BASE
{
  LIST_ENTRY List;

  PVOID MappedAddress;
  ULONG NumberOfBytes;
  SCSI_PHYSICAL_ADDRESS IoAddress;
  ULONG SystemIoBusNumber;
} SCSI_PORT_DEVICE_BASE, *PSCSI_PORT_DEVICE_BASE;

typedef struct _SCSI_REQUEST_BLOCK_INFO
{
    LIST_ENTRY Requests;
} SCSI_REQUEST_BLOCK_INFO, *PSCSI_REQUEST_BLOCK_INFO;

typedef struct _SCSI_PORT_LUN_EXTENSION
{
  UCHAR PathId;
  UCHAR TargetId;
  UCHAR Lun;

  ULONG Flags;

  struct _SCSI_PORT_LUN_EXTENSION *Next;

  BOOLEAN DeviceClaimed;
  PDEVICE_OBJECT DeviceObject;

  INQUIRYDATA InquiryData;

  KDEVICE_QUEUE DeviceQueue;

  SCSI_REQUEST_BLOCK_INFO SrbInfo;

  /* More data? */

  UCHAR MiniportLunExtension[1]; /* must be the last entry */
} SCSI_PORT_LUN_EXTENSION, *PSCSI_PORT_LUN_EXTENSION;

/* Structures for inquiries support */

typedef struct _SCSI_LUN_INFO
{
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    BOOLEAN DeviceClaimed;
    PVOID DeviceObject;
    struct _SCSI_LUN_INFO *Next;
    UCHAR InquiryData[INQUIRYDATABUFFERSIZE];
} SCSI_LUN_INFO, *PSCSI_LUN_INFO;

typedef struct _SCSI_BUS_SCAN_INFO
{
    USHORT Length;
    UCHAR LogicalUnitsCount;
    UCHAR BusIdentifier;
    PSCSI_LUN_INFO LunInfo;
} SCSI_BUS_SCAN_INFO, *PSCSI_BUS_SCAN_INFO;

typedef struct _BUSES_CONFIGURATION_INFORMATION
{
    UCHAR NumberOfBuses;
    PSCSI_BUS_SCAN_INFO BusScanInfo[1];
} BUSES_CONFIGURATION_INFORMATION, *PBUSES_CONFIGURATION_INFORMATION;

/*
 * SCSI_PORT_DEVICE_EXTENSION
 *
 * DESCRIPTION
 *	First part of the port objects device extension. The second
 *	part is the miniport-specific device extension.
 */

typedef struct _SCSI_PORT_DEVICE_EXTENSION
{
  ULONG Length;
  ULONG MiniPortExtensionSize;
  PPORT_CONFIGURATION_INFORMATION PortConfig;
  PBUSES_CONFIGURATION_INFORMATION BusesConfig;
  ULONG PortNumber;

  KSPIN_LOCK IrpLock;
  KSPIN_LOCK SpinLock;
  PKINTERRUPT Interrupt;
  PIRP                   CurrentIrp;
  ULONG IrpFlags;

  SCSI_PORT_TIMER_STATES TimerState;
  LONG                   TimerCount;

  LIST_ENTRY DeviceBaseListHead;

  ULONG LunExtensionSize;
  PSCSI_PORT_LUN_EXTENSION LunExtensionList[LUS_NUMBER];

  ULONG SrbExtensionSize;

  PIO_SCSI_CAPABILITIES PortCapabilities;

  PDEVICE_OBJECT DeviceObject;
  PCONTROLLER_OBJECT ControllerObject;

  PHW_STARTIO HwStartIo;
  PHW_INTERRUPT HwInterrupt;

  PSCSI_REQUEST_BLOCK OriginalSrb;
  SCSI_REQUEST_BLOCK InternalSrb;
  SENSE_DATA InternalSenseData;

  /* DMA related stuff */
  PADAPTER_OBJECT AdapterObject;
  ULONG MapRegisterCount;

  PHYSICAL_ADDRESS PhysicalAddress;
  PVOID VirtualAddress;
  ULONG CommonBufferLength;

  UCHAR MiniPortDeviceExtension[1]; /* must be the last entry */
} SCSI_PORT_DEVICE_EXTENSION, *PSCSI_PORT_DEVICE_EXTENSION;


#pragma once

#include <ntddk.h>
#define NDEBUG
#include <debug.h>
#include <usbdi.h>
#include <hubbusif.h>
#include <usbbusif.h>
#include <usbioctl.h>
#include <usbiodef.h>
#include <usb.h>
#include <usbdlib.h>
#include <stdio.h>
#include <wdmguid.h>
#include <classpnp.h>
#include <scsi.h>

#define USB_STOR_TAG 'sbsu'
#define USB_MAXCHILDREN              (16)

NTSTATUS NTAPI
IoAttachDeviceToDeviceStackSafe(
  IN PDEVICE_OBJECT SourceDevice,
  IN PDEVICE_OBJECT TargetDevice,
  OUT PDEVICE_OBJECT *AttachedToDeviceObject);

typedef struct __COMMON_DEVICE_EXTENSION__
{
    BOOLEAN IsFDO;

}USBSTOR_COMMON_DEVICE_EXTENSION, *PUSBSTOR_COMMON_DEVICE_EXTENSION;

typedef struct
{
    USBSTOR_COMMON_DEVICE_EXTENSION Common;                                                      // common device extension

    PDEVICE_OBJECT FunctionalDeviceObject;                                               // functional device object
    PDEVICE_OBJECT PhysicalDeviceObject;                                                 // physical device object
    PDEVICE_OBJECT LowerDeviceObject;                                                    // lower device object
    USB_BUS_INTERFACE_USBDI_V2 BusInterface;                                             // bus interface of device
    PUSB_DEVICE_DESCRIPTOR DeviceDescriptor;                                             // usb device descriptor
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;                               // usb configuration descriptor
    PUSBD_INTERFACE_INFORMATION InterfaceInformation;                                    // usb interface information
    USBD_CONFIGURATION_HANDLE ConfigurationHandle;                                       // usb configuration handle
    UCHAR BulkInPipeIndex;                                                               // bulk in pipe index
    UCHAR BulkOutPipeIndex;                                                              // bulk out pipe index
    UCHAR MaxLUN;                                                                        // max lun for device
    PDEVICE_OBJECT ChildPDO[16];                                                         // max 16 child pdo devices
}FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;

typedef struct
{
    USBSTOR_COMMON_DEVICE_EXTENSION Common;
    PDEVICE_OBJECT LowerDeviceObject;                                                    // points to FDO
    UCHAR LUN;                                                                           // lun id
    PVOID InquiryData;                                                                   // USB SCSI inquiry data
    UCHAR Claimed;                                                                       // indicating if it has been claimed by upper driver
}PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;

//
// max lun command identifier
//
#define USB_BULK_GET_MAX_LUN             0xFE

#include <pshpack1.h>
typedef struct
{
    ULONG Signature;                                                 // CBW signature
    ULONG Tag;                                                       // CBW Tag of operation
    ULONG DataTransferLength;                                        // data transfer length
    UCHAR Flags;                                                     // CBW Flags endpoint direction
    UCHAR LUN;                                                       // lun unit
    UCHAR CommandBlockLength;                                        // Command block length
    UCHAR CommandBlock[16];
}CBW, *PCBW;

C_ASSERT(sizeof(CBW) == 31);


#define CBW_SIGNATURE 0x43425355
#define MAX_LUN 0xF

typedef struct
{
    ULONG Signature;                                                 // CSW signature
    ULONG Tag;                                                       // CSW tag
    ULONG DataResidue;                                               // CSW data transfer diff
    UCHAR Status;                                                    // CSW status
}CSW, *PCSW;

//--------------------------------------------------------------------------------------------------------------------------------------------
//
// UFI INQUIRY command
//
typedef struct
{
    UCHAR Code;                                                      // operation code 0x12
    UCHAR LUN;                                                       // lun address
    UCHAR PageCode;                                                  // product data information, always 0x00
    UCHAR Reserved;                                                  // reserved 0x00
    UCHAR AllocationLength;                                          // length of inquiry data to be returned, default 36 bytes
    UCHAR Reserved1[7];                                              //reserved bytes 0x00
}UFI_INQUIRY_CMD, *PUFI_INQUIRY_CMD;

C_ASSERT(sizeof(UFI_INQUIRY_CMD) == 12);

#define UFI_INQUIRY_CMD_LEN 0x6

//
// UFI INQUIRY command response
//
typedef struct
{
    UCHAR DeviceType;                                                // device type
    UCHAR RMB;                                                       // removable media bit
    UCHAR Version;                                                   // contains version 0x00
    UCHAR Format;                                                    // response format
    UCHAR Length;                                                    // additional length
    UCHAR Reserved[3];                                               // reserved
    UCHAR Vendor[8];                                                 // vendor identification string
    UCHAR Product[16];                                               // product identification string
    UCHAR Revision[4];                                               // product revision code
}UFI_INQUIRY_RESPONSE, *PUFI_INQUIRY_RESPONSE;

C_ASSERT(sizeof(UFI_INQUIRY_RESPONSE) == 36);

//--------------------------------------------------------------------------------------------------------------------------------------------
//
// UFI read capacity cmd
//
typedef struct
{
    UCHAR Code;                                                      // operation code 0x25
    UCHAR LUN;                                                       // lun address
    UCHAR LBA[4];                                                   // logical block address, should be zero
    UCHAR Reserved1[2];                                              // reserved 0x00
    UCHAR PMI;                                                       // PMI = 0x00
    UCHAR Reserved2[3];                                              // reserved 0x00
}UFI_CAPACITY_CMD, *PUFI_CAPACITY_CMD;

C_ASSERT(sizeof(UFI_CAPACITY_CMD) == 12);

#define UFI_CAPACITY_CMD_LEN 0xA //FIXME support length 16 too if requested

//
// UFI Read Capcacity command response
//
typedef struct
{
    ULONG LastLogicalBlockAddress;                                   // last logical block address
    ULONG BlockLength;                                               // block length in bytes
}UFI_CAPACITY_RESPONSE, *PUFI_CAPACITY_RESPONSE;

C_ASSERT(sizeof(UFI_CAPACITY_RESPONSE) == 8);

#define HTONS(n) (((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00) >> 8))
#define NTOHS(n) (((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00) >> 8))

#define HTONL(n) (((((unsigned long)(n) & 0xFF)) << 24) | \
                  ((((unsigned long)(n) & 0xFF00)) << 8) | \
                  ((((unsigned long)(n) & 0xFF0000)) >> 8) | \
                  ((((unsigned long)(n) & 0xFF000000)) >> 24))


#define NTOHL(n) (((((unsigned long)(n) & 0xFF)) << 24) | \
                  ((((unsigned long)(n) & 0xFF00)) << 8) | \
                  ((((unsigned long)(n) & 0xFF0000)) >> 8) | \
                  ((((unsigned long)(n) & 0xFF000000)) >> 24))



//--------------------------------------------------------------------------------------------------------------------------------------------
//
// UFI sense mode cmd
//
typedef struct
{
    UCHAR Code;                                                      // operation code
    UCHAR LUN;                                                       // lun address
    UCHAR PageCode:6;                                                // page code selector
    UCHAR PC:2;                                                      // type of parameters to be returned
    UCHAR Reserved[4];                                               // reserved 0x00
    USHORT AllocationLength;                                         // parameters length
    UCHAR Reserved1[3];
}UFI_SENSE_CMD, *PUFI_SENSE_CMD;

C_ASSERT(sizeof(UFI_SENSE_CMD) == 12);

#define UFI_SENSE_CMD_LEN (6)

typedef struct
{
    USHORT ModeDataLength;                                           // length of parameters for sense cmd
    UCHAR MediumTypeCode;                                            // 00 for mass storage, 0x94 for floppy
    UCHAR WP:1;                                                      // write protect bit
    UCHAR Reserved1:2;                                                // reserved 00
    UCHAR DPOFUA:1;                                                  // should be zero
    UCHAR Reserved2:4;                                               // reserved
    UCHAR Reserved[4];                                               // reserved
}UFI_MODE_PARAMETER_HEADER, *PUFI_MODE_PARAMETER_HEADER;


C_ASSERT(sizeof(UFI_MODE_PARAMETER_HEADER) == 8);

typedef struct
{
    UCHAR PC;
    UCHAR PageLength;
    UCHAR Reserved1;
    UCHAR ITM;
    UCHAR Flags;
    UCHAR Reserved[3];
}UFI_TIMER_PROTECT_PAGE, *PUFI_TIMER_PROTECT_PAGE;
C_ASSERT(sizeof(UFI_TIMER_PROTECT_PAGE) == 8);

//---------------------------------------------------------------------
//
// fdo.c routines
//
NTSTATUS
USBSTOR_FdoHandlePnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp);

//---------------------------------------------------------------------
//
// pdo.c routines
//
NTSTATUS
USBSTOR_PdoHandlePnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp);

NTSTATUS
USBSTOR_CreatePDO(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PDEVICE_OBJECT *ChildDeviceObject);

//---------------------------------------------------------------------
//
// misc.c routines
//
NTSTATUS
NTAPI
USBSTOR_SyncForwardIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp);

NTSTATUS
NTAPI
USBSTOR_GetBusInterface(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PUSB_BUS_INTERFACE_USBDI_V2 BusInterface);

PVOID
AllocateItem(
    IN POOL_TYPE PoolType, 
    IN ULONG ItemSize);

VOID
FreeItem(
    IN PVOID Item);

NTSTATUS
USBSTOR_SyncUrbRequest(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PURB UrbRequest);

NTSTATUS
USBSTOR_GetMaxLUN(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFDO_DEVICE_EXTENSION DeviceExtension);

NTSTATUS
NTAPI
USBSTOR_SyncForwardIrpCompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp, 
    PVOID Context);


//---------------------------------------------------------------------
//
// descriptor.c routines
//

NTSTATUS
USBSTOR_GetDescriptors(
    IN PDEVICE_OBJECT DeviceObject);

NTSTATUS
USBSTOR_SelectConfigurationAndInterface(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFDO_DEVICE_EXTENSION DeviceExtension);

NTSTATUS
USBSTOR_GetPipeHandles(
    IN PFDO_DEVICE_EXTENSION DeviceExtension);

//---------------------------------------------------------------------
//
// scsi.c routines
//
NTSTATUS
USBSTOR_SendInquiryCmd(
    IN PDEVICE_OBJECT DeviceObject);

NTSTATUS
USBSTOR_SendCapacityCmd(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PREAD_CAPACITY_DATA_EX CapacityDataEx,
    OUT PREAD_CAPACITY_DATA CapacityData);

NTSTATUS
USBSTOR_SendModeSenseCmd(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PSCSI_REQUEST_BLOCK Request,
    OUT PULONG TransferBufferLength);

//---------------------------------------------------------------------
//
// disk.c routines
//
NTSTATUS
USBSTOR_HandleInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
USBSTOR_HandleDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);



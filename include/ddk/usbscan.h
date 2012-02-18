/*
 * usbscan.h
 *
 * USB scanner definitions
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if (NTDDI_VERSION >= NTDDI_WIN2K)

#pragma pack(push,8)

#ifndef MAX_NUM_PIPES
#define MAX_NUM_PIPES                     8
#endif

#define BULKIN_FLAG                       0x80

typedef struct _DRV_VERSION {
  _Out_ ULONG major;
  _Out_ ULONG minor;
  _Out_ ULONG internal;
} DRV_VERSION, *PDRV_VERSION;

typedef struct _IO_BLOCK {
  _In_ ULONG uOffset;
  _In_ ULONG uLength;
  _Inout_updates_bytes_(uLength) PUCHAR pbyData;
  _In_ ULONG uIndex;
} IO_BLOCK, *PIO_BLOCK;

typedef struct _IO_BLOCK_EX {
  _In_ ULONG uOffset;
  _In_ ULONG uLength;
  _Inout_updates_bytes_(uLength) PUCHAR pbyData;
  _In_ ULONG uIndex;
  _In_ UCHAR bRequest;
  _In_ UCHAR bmRequestType;
  _In_ UCHAR fTransferDirectionIn;
} IO_BLOCK_EX, *PIO_BLOCK_EX;

typedef struct _CHANNEL_INFO {
  _Out_ ULONG EventChannelSize;
  _Out_ ULONG uReadDataAlignment;
  _Out_ ULONG uWriteDataAlignment;
}CHANNEL_INFO, *PCHANNEL_INFO;

typedef enum _PIPE_TYPE {
  EVENT_PIPE,
  READ_DATA_PIPE,
  WRITE_DATA_PIPE,
  ALL_PIPE
} PIPE_TYPE;

typedef struct _USBSCAN_GET_DESCRIPTOR {
  _In_ UCHAR DescriptorType;
  _In_ UCHAR Index;
  _In_ USHORT LanguageId;
} USBSCAN_GET_DESCRIPTOR, *PUSBSCAN_GET_DESCRIPTOR;

typedef struct _DEVICE_DESCRIPTOR {
  _Out_ USHORT usVendorId;
  _Out_ USHORT usProductId;
  _Out_ USHORT usBcdDevice;
  _Out_ USHORT usLanguageId;
} DEVICE_DESCRIPTOR, *PDEVICE_DESCRIPTOR;

typedef enum _RAW_PIPE_TYPE {
  USBSCAN_PIPE_CONTROL,
  USBSCAN_PIPE_ISOCHRONOUS,
  USBSCAN_PIPE_BULK,
  USBSCAN_PIPE_INTERRUPT
} RAW_PIPE_TYPE;

typedef struct _USBSCAN_PIPE_INFORMATION {
  USHORT MaximumPacketSize;
  UCHAR EndpointAddress;
  UCHAR Interval;
  RAW_PIPE_TYPE PipeType;
} USBSCAN_PIPE_INFORMATION, *PUSBSCAN_PIPE_INFORMATION;

typedef struct _USBSCAN_PIPE_CONFIGURATION {
  _Out_ ULONG NumberOfPipes;
  _Out_writes_(NumberOfPipes) USBSCAN_PIPE_INFORMATION PipeInfo[MAX_NUM_PIPES];
} USBSCAN_PIPE_CONFIGURATION, *PUSBSCAN_PIPE_CONFIGURATION;

#if (NTDDI_VERSION >= NTDDI_WINXP)
typedef struct _USBSCAN_TIMEOUT {
  IN ULONG TimeoutRead;
  IN ULONG TimeoutWrite;
  IN ULONG TimeoutEvent;
} USBSCAN_TIMEOUT, *PUSBSCAN_TIMEOUT;
#endif

#define FILE_DEVICE_USB_SCAN              0x8000
#define IOCTL_INDEX                       0x0800

#define IOCTL_GET_VERSION \
  CTL_CODE(FILE_DEVICE_USB_SCAN, IOCTL_INDEX + 0, METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_CANCEL_IO \
  CTL_CODE(FILE_DEVICE_USB_SCAN, IOCTL_INDEX + 1, METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_WAIT_ON_DEVICE_EVENT \
  CTL_CODE(FILE_DEVICE_USB_SCAN, IOCTL_INDEX + 2, METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_READ_REGISTERS \
  CTL_CODE(FILE_DEVICE_USB_SCAN, IOCTL_INDEX + 3, METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_WRITE_REGISTERS \
  CTL_CODE(FILE_DEVICE_USB_SCAN, IOCTL_INDEX + 4, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_GET_CHANNEL_ALIGN_RQST \
  CTL_CODE(FILE_DEVICE_USB_SCAN, IOCTL_INDEX + 5, METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_GET_DEVICE_DESCRIPTOR \
  CTL_CODE(FILE_DEVICE_USB_SCAN, IOCTL_INDEX + 6, METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_RESET_PIPE \
  CTL_CODE(FILE_DEVICE_USB_SCAN, IOCTL_INDEX + 7, METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_GET_USB_DESCRIPTOR \
  CTL_CODE(FILE_DEVICE_USB_SCAN, IOCTL_INDEX + 8, METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SEND_USB_REQUEST \
  CTL_CODE(FILE_DEVICE_USB_SCAN, IOCTL_INDEX + 9, METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_GET_PIPE_CONFIGURATION \
  CTL_CODE(FILE_DEVICE_USB_SCAN, IOCTL_INDEX + 10,METHOD_BUFFERED,FILE_ANY_ACCESS)

#if (NTDDI_VERSION >= NTDDI_WINXP)
#define IOCTL_SET_TIMEOUT \
  CTL_CODE(FILE_DEVICE_USB_SCAN, IOCTL_INDEX + 11,METHOD_BUFFERED,FILE_ANY_ACCESS)
#endif

#pragma pack(pop)

#endif // (NTDDI_VERSION >= NTDDI_WIN2K)

#ifdef __cplusplus
}
#endif

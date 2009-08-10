#ifndef SYSAUDIO_H__
#define SYSAUDIO_H__

#include <ntifs.h>
#include <ntddk.h>
#include <portcls.h>
#include <ks.h>
#include <ksmedia.h>
#include <math.h>
#define NDEBUG
#include <debug.h>
#include <stdio.h>

typedef struct
{
    ULONG MaxPinInstanceCount;              // maximum times a audio irp pin can be instantiated
    HANDLE PinHandle;                       // handle to audio irp pin
    ULONG References;                       // number of clients having a reference to this audio irp pin
    KSPIN_DATAFLOW DataFlow;                // specifies data flow
    KSPIN_COMMUNICATION Communication;      // pin type
}PIN_INFO;

typedef struct
{
    LIST_ENTRY Entry;                                  // device entry for KsAudioDeviceList
    UNICODE_STRING DeviceName;                         // symbolic link of audio device

    HANDLE Handle;                          // handle to audio sub device
    PFILE_OBJECT FileObject;                // file objecto to audio sub device

    ULONG NumberOfPins;                     // number of pins of audio device
    PIN_INFO * Pins;                        // array of PIN_INFO

}KSAUDIO_DEVICE_ENTRY, *PKSAUDIO_DEVICE_ENTRY;

typedef struct
{
    KSDEVICE_HEADER KsDeviceHeader;                     // ks streaming header - must always be first item in device extension
    PDEVICE_OBJECT PhysicalDeviceObject;                // pdo
    PDEVICE_OBJECT NextDeviceObject;                    // lower device object
    ULONG NumberOfKsAudioDevices;                       // number of audio devices

    LIST_ENTRY KsAudioDeviceList;                       // audio device list
    PVOID KsAudioNotificationEntry;                     // ks audio notification hook
    PVOID EchoCancelNotificationEntry;                  // ks echo cancel notification hook
    KSPIN_LOCK Lock;                                    // audio device list mutex

    PFILE_OBJECT KMixerFileObject;                      // mixer file object
    HANDLE KMixerHandle;                                // mixer file handle

}SYSAUDIODEVEXT, *PSYSAUDIODEVEXT;

// struct DISPATCH_CONTEXT
//
// This structure is used to dispatch read / write / device io requests
// It is stored in the file object FsContext2 member
// Note: FsContext member is reserved for ks object header

typedef struct
{
    HANDLE Handle;                                       // audio irp pin handle
    ULONG PinId;                                         // pin id of device
    PKSAUDIO_DEVICE_ENTRY AudioEntry;                 // pointer to audio device entry

    HANDLE hMixerPin;                                    // handle to mixer pin
}DISPATCH_CONTEXT, *PDISPATCH_CONTEXT;

// struct PIN_WORKER_CONTEXT
//
// This structure holds all information required
// to create audio irp pin, mixer pin and virtual sysaudio pin
//
typedef struct
{
    PIRP Irp;
    BOOL CreateRealPin;
    BOOL CreateMixerPin;
    PKSAUDIO_DEVICE_ENTRY Entry;
    KSPIN_CONNECT * PinConnect;
    PDISPATCH_CONTEXT DispatchContext;
    PSYSAUDIODEVEXT DeviceExtension;
    PKSDATAFORMAT_WAVEFORMATEX MixerFormat;
    PIO_WORKITEM WorkItem;
}PIN_WORKER_CONTEXT, *PPIN_WORKER_CONTEXT;

typedef struct
{
    PIO_WORKITEM WorkItem;
    PKSAUDIO_DEVICE_ENTRY DeviceEntry;
}FILTER_WORKER_CONTEXT, *PFILTER_WORKER_CONTEXT;


NTSTATUS
SysAudioAllocateDeviceHeader(
    IN SYSAUDIODEVEXT *DeviceExtension);

NTSTATUS
SysAudioRegisterDeviceInterfaces(
    IN PDEVICE_OBJECT DeviceObject);

NTSTATUS
SysAudioRegisterNotifications(
    IN PDRIVER_OBJECT  DriverObject,
    IN PDEVICE_OBJECT DeviceObject);

NTSTATUS
SysAudioHandleProperty(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

NTSTATUS
SysAudioOpenKMixer(
    IN SYSAUDIODEVEXT *DeviceExtension);

NTSTATUS
OpenDevice(
    IN PUNICODE_STRING DeviceName,
    IN PHANDLE HandleOut,
    IN PFILE_OBJECT * FileObjectOut);

PKSAUDIO_DEVICE_ENTRY
GetListEntry(
    IN PLIST_ENTRY Head,
    IN ULONG Index);

NTSTATUS
CreateSysAudioPin(
    IN PIRP Irp);

ULONG
GetDeviceCount(
    PSYSAUDIODEVEXT DeviceExtension,
    BOOL WaveIn);

#endif

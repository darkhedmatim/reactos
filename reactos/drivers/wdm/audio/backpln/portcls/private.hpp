/*
    PortCls FDO Extension

    by Andrew Greenwood
*/

#ifndef PORTCLS_PRIVATE_H
#define PORTCLS_PRIVATE_H

//#define _KS_NO_ANONYMOUS_STRUCTURES_
#define PC_IMPLEMENTATION

#include <ntddk.h>
#include <portcls.h>
#define NDEBUG
#include <debug.h>

#include <dmusicks.h>
#include <kcom.h>
#include "interfaces.hpp"
#include <ks.h>
#include <ksmedia.h>
#include <intrin.h>

#define TAG_PORTCLASS 'SLCP'

#define PC_ASSERT(exp) \
  (VOID)((!(exp)) ? \
    RtlAssert((PVOID) #exp, (PVOID)__FILE__, __LINE__, NULL ), FALSE : TRUE)

#define PC_ASSERT_IRQL(x) PC_ASSERT(KeGetCurrentIrql() <= (x))
#define PC_ASSERT_IRQL_EQUAL(x) PC_ASSERT(KeGetCurrentIrql()==(x))

extern
"C"
NTSTATUS
NTAPI
PortClsCreate(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

extern
"C"
NTSTATUS
NTAPI
PortClsPnp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

extern
"C"
NTSTATUS
NTAPI
PortClsPower(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

extern
"C"
NTSTATUS
NTAPI
PortClsSysControl(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTSTATUS
NewMiniportDMusUART(
    OUT PMINIPORT* OutMiniport,
    IN  REFCLSID ClassId);

NTSTATUS
NewMiniportFmSynth(
    OUT PMINIPORT* OutMiniport,
    IN  REFCLSID ClassId);

NTSTATUS
NewPortDMus(
    OUT PPORT* OutPort);

NTSTATUS
NewPortTopology(
    OUT PPORT* OutPort);

NTSTATUS
NewPortWaveCyclic(
    OUT PPORT* OutPort);

NTSTATUS
NewPortWavePci(
    OUT PPORT* OutPort);

NTSTATUS
NewIDrmPort(
    OUT PDRMPORT2 *OutPort);

NTSTATUS
NewPortClsVersion(
    OUT PPORTCLSVERSION * OutVersion);

NTSTATUS
NewPortFilterWaveCyclic(
    OUT IPortFilterWaveCyclic ** OutFilter);

NTSTATUS
NewPortPinWaveCyclic(
    OUT IPortPinWaveCyclic ** OutPin);

NTSTATUS 
NewPortFilterWavePci(
    OUT IPortFilterWavePci ** OutFilter);

NTSTATUS
NewPortPinWavePci(
    OUT IPortPinWavePci ** OutPin);

PDEVICE_OBJECT
GetDeviceObjectFromWaveCyclic(
    IPortWavePci* iface);

PDEVICE_OBJECT
GetDeviceObjectFromPortWavePci(
    IPortWavePci* iface);

PMINIPORTWAVEPCI
GetWavePciMiniport(
    PPORTWAVEPCI Port);


NTSTATUS 
NewPortFilterDMus(
    OUT PPORTFILTERDMUS * OutFilter);


NTSTATUS
NewPortPinDMus(
    OUT PPORTPINDMUS * OutPin);

VOID
GetDMusMiniport(
    IN IPortDMus * iface, 
    IN PMINIPORTDMUS * Miniport,
    IN PMINIPORTMIDI * MidiMiniport);

#if (NTDDI_VERSION >= NTDDI_VISTA)

NTSTATUS 
NewPortFilterWaveRT(
    OUT IPortFilterWaveRT ** OutFilter);

NTSTATUS
NewPortPinWaveRT(
    OUT IPortPinWaveRT ** OutPin);

PMINIPORTWAVERT
GetWaveRTMiniport(
    IN IPortWaveRT* iface);

PDEVICE_OBJECT
GetDeviceObjectFromPortWaveRT(
    IPortWaveRT* iface);


NTSTATUS
NewPortWaveRTStream(
    PPORTWAVERTSTREAM *OutStream);

NTSTATUS
NewPortWaveRT(
    OUT PPORT* OutPort);


#endif

NTSTATUS 
NewPortFilterTopology(
    OUT IPortFilterTopology ** OutFilter);

PMINIPORTTOPOLOGY
GetTopologyMiniport(
    PPORTTOPOLOGY Port);

NTSTATUS
NTAPI
NewDispatchObject(
    IN PIRP Irp,
    IN IIrpTarget * Target,
    IN ULONG ObjectCreateItemCount,
    IN PKSOBJECT_CREATE_ITEM ObjectCreateItem);

PMINIPORTWAVECYCLIC
GetWaveCyclicMiniport(
    IN IPortWaveCyclic* iface);

PVOID
AllocateItem(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag);

VOID
FreeItem(
    IN PVOID Item,
    IN ULONG Tag);

NTSTATUS
NTAPI
NewIrpQueue(
    IN IIrpQueue **Queue);

NTSTATUS
NTAPI
TopologyPropertyHandler(
    IN PIRP Irp,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data);

NTSTATUS
NTAPI
PinPropertyHandler(
    IN PIRP Irp,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data);

extern
"C"
NTSTATUS
NTAPI
PcDmaMasterDescription(
    IN PRESOURCELIST ResourceList OPTIONAL,
    IN BOOLEAN ScatterGather,
    IN BOOLEAN Dma32BitAddresses,
    IN BOOLEAN IgnoreCount,
    IN BOOLEAN Dma64BitAddresses,
    IN DMA_WIDTH DmaWidth,
    IN DMA_SPEED DmaSpeed,
    IN ULONG MaximumLength,
    IN ULONG DmaPort,
    OUT PDEVICE_DESCRIPTION DeviceDescription);

extern
"C"
NTSTATUS
NTAPI
PcDmaSlaveDescription(
    IN PRESOURCELIST  ResourceList OPTIONAL,
    IN ULONG DmaIndex,
    IN BOOL DemandMode,
    IN ULONG AutoInitialize,
    IN DMA_SPEED DmaSpeed,
    IN ULONG MaximumLength,
    IN ULONG DmaPort,
    OUT PDEVICE_DESCRIPTION DeviceDescription);

extern
"C"
NTSTATUS
NTAPI
PcCreateSubdeviceDescriptor(
    OUT SUBDEVICE_DESCRIPTOR ** OutSubdeviceDescriptor,
    IN ULONG InterfaceCount,
    IN GUID * InterfaceGuids,
    IN ULONG IdentifierCount,
    IN KSIDENTIFIER *Identifier,
    IN ULONG FilterPropertiesCount,
    IN KSPROPERTY_SET * FilterProperties,
    IN ULONG Unknown1,
    IN ULONG Unknown2,
    IN ULONG PinPropertiesCount,
    IN KSPROPERTY_SET * PinProperties,
    IN ULONG EventSetCount,
    IN KSEVENT_SET * EventSet,
    IN PPCFILTER_DESCRIPTOR FilterDescription);

extern
"C"
NTSTATUS
NTAPI
PcValidateConnectRequest(
    IN PIRP Irp,
    IN KSPIN_FACTORY * Descriptor,
    OUT PKSPIN_CONNECT* Connect);

NTSTATUS
NTAPI
PcCreateItemDispatch(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTSTATUS
NTAPI
PcPropertyHandler(
    IN PIRP Irp,
    IN PSUBDEVICE_DESCRIPTOR Descriptor);

NTSTATUS
NTAPI
FastPropertyHandler(
    IN PFILE_OBJECT  FileObject,
    IN PKSPROPERTY UNALIGNED  Property,
    IN ULONG  PropertyLength,
    IN OUT PVOID UNALIGNED  Data,
    IN ULONG  DataLength,
    OUT PIO_STATUS_BLOCK  IoStatus,
    IN ULONG  PropertySetsCount,
    IN const KSPROPERTY_SET *PropertySet,
    IN PSUBDEVICE_DESCRIPTOR Descriptor,
    IN ISubdevice *SubDevice);

PDEVICE_OBJECT
GetDeviceObject(
    IPortWaveCyclic* iface);

VOID
NTAPI
PcIoTimerRoutine(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PVOID  Context);

NTSTATUS
NTAPI
NewIUnregisterSubdevice(
    OUT PUNREGISTERSUBDEVICE *OutDevice);

NTSTATUS
NTAPI
NewIUnregisterPhysicalConnection(
    OUT PUNREGISTERPHYSICALCONNECTION *OutConnection);

#define DEFINE_KSPROPERTY_PINPROPOSEDATAFORMAT(PinSet,\
    PropGeneral, PropInstances, PropIntersection)\
DEFINE_KSPROPERTY_TABLE(PinSet) {\
    DEFINE_KSPROPERTY_ITEM_PIN_CINSTANCES(PropInstances),\
    DEFINE_KSPROPERTY_ITEM_PIN_CTYPES(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_DATAFLOW(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_DATARANGES(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_DATAINTERSECTION(PropIntersection),\
    DEFINE_KSPROPERTY_ITEM_PIN_INTERFACES(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_MEDIUMS(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_COMMUNICATION(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_GLOBALCINSTANCES(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_NECESSARYINSTANCES(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_PHYSICALCONNECTION(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_CATEGORY(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_NAME(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_CONSTRAINEDDATARANGES(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_PROPOSEDATAFORMAT(PropGeneral)\
}

typedef struct
{
    KSDEVICE_HEADER KsDeviceHeader;
    PDEVICE_OBJECT PhysicalDeviceObject;
    PDEVICE_OBJECT PrevDeviceObject;
    PCPFNSTARTDEVICE StartDevice;
    ULONG_PTR Unused[4];
    IAdapterPowerManagement * AdapterPowerManagement;
    ULONG MaxSubDevices;
    KSOBJECT_CREATE_ITEM * CreateItems;

    IResourceList* resources;

    LIST_ENTRY TimerList;
    KSPIN_LOCK TimerListLock;

} PCLASS_DEVICE_EXTENSION, *PPCLASS_DEVICE_EXTENSION;


typedef struct
{
    KSSTREAM_HEADER Header;
    PIRP Irp;
}CONTEXT_WRITE, *PCONTEXT_WRITE;

typedef struct
{
    PVOID Pin;
    PIO_WORKITEM WorkItem;
    PIRP Irp;
}CLOSESTREAM_CONTEXT, *PCLOSESTREAM_CONTEXT;

typedef struct
{
    LIST_ENTRY Entry;
    PIO_TIMER_ROUTINE pTimerRoutine;
    PVOID Context;
}TIMER_CONTEXT, *PTIMER_CONTEXT;

#endif

/*
* PROJECT:     ReactOS Universal Audio Class Driver
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        drivers/usb/usbaudio/pin.c
* PURPOSE:     USB Audio device driver.
* PROGRAMMERS:
*              Johannes Anderwald (johannes.anderwald@reactos.org)
*/

#include "usbaudio.h"

#define PACKET_COUNT 10


NTSTATUS
GetMaxPacketSizeForInterface(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor,
    KSPIN_DATAFLOW DataFlow)
{
    PUSB_COMMON_DESCRIPTOR CommonDescriptor;
    PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor;

    /* loop descriptors */
    CommonDescriptor = (PUSB_COMMON_DESCRIPTOR)((ULONG_PTR)InterfaceDescriptor + InterfaceDescriptor->bLength);
    ASSERT(InterfaceDescriptor->bNumEndpoints > 0);
    while (CommonDescriptor)
    {
        if (CommonDescriptor->bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE)
        {
            EndpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR)CommonDescriptor;
            return EndpointDescriptor->wMaxPacketSize;
        }

        if (CommonDescriptor->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE)
        {
            /* reached next interface descriptor */
            break;
        }

        if ((ULONG_PTR)CommonDescriptor + CommonDescriptor->bLength >= ((ULONG_PTR)ConfigurationDescriptor + ConfigurationDescriptor->wTotalLength))
            break;

        CommonDescriptor = (PUSB_COMMON_DESCRIPTOR)((ULONG_PTR)CommonDescriptor + CommonDescriptor->bLength);
    }

    /* default to 100 */
    return 100;
}

NTSTATUS
UsbAudioAllocCaptureUrbIso(
    IN USBD_PIPE_HANDLE PipeHandle,
    IN ULONG MaxPacketSize,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    OUT PURB * OutUrb)
{
    PURB Urb;
    ULONG UrbSize;
    ULONG Index;

    /* calculate urb size*/
    UrbSize = GET_ISO_URB_SIZE(PACKET_COUNT);

    /* allocate urb */
    Urb = AllocFunction(UrbSize);
    if (!Urb)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* init urb */
    Urb->UrbIsochronousTransfer.Hdr.Function = URB_FUNCTION_ISOCH_TRANSFER;
    Urb->UrbIsochronousTransfer.Hdr.Length = UrbSize;
    Urb->UrbIsochronousTransfer.PipeHandle = PipeHandle;
    Urb->UrbIsochronousTransfer.TransferFlags = USBD_TRANSFER_DIRECTION_IN | USBD_START_ISO_TRANSFER_ASAP;
    Urb->UrbIsochronousTransfer.TransferBufferLength = BufferLength;
    Urb->UrbIsochronousTransfer.TransferBuffer = Buffer;
    Urb->UrbIsochronousTransfer.NumberOfPackets = PACKET_COUNT;

    for (Index = 0; Index < PACKET_COUNT; Index++)
    {
        Urb->UrbIsochronousTransfer.IsoPacket[Index].Offset = Index * MaxPacketSize;
    }

    *OutUrb = Urb;
    return STATUS_SUCCESS;

}

NTSTATUS
UsbAudioSetMuteOff(
    IN PKSPIN Pin)
{
    PURB Urb;
    PVOID SampleRateBuffer;
    PPIN_CONTEXT PinContext;
    NTSTATUS Status;

    /* allocate sample rate buffer */
    SampleRateBuffer = AllocFunction(sizeof(ULONG));
    if (!SampleRateBuffer)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* allocate urb */
    Urb = AllocFunction(sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST));
    if (!Urb)
    {
        /* no memory */
        FreeFunction(SampleRateBuffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* FIXME: determine controls and format urb */
    UsbBuildVendorRequest(Urb,
        URB_FUNCTION_CLASS_INTERFACE,
        sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
        USBD_TRANSFER_DIRECTION_OUT,
        0,
        0x01,
        0x100,
        0x300,
        SampleRateBuffer,
        NULL,
        1,
        NULL);

    /* get pin context */
    PinContext = Pin->Context;

    /* submit urb */
    Status = SubmitUrbSync(PinContext->LowerDevice, Urb);

    DPRINT1("UsbAudioSetMuteOff Pin %p Status %x\n", Pin, Status);
    FreeFunction(Urb);
    FreeFunction(SampleRateBuffer);
    return Status;
}

NTSTATUS
UsbAudioSetVolume(
    IN PKSPIN Pin)
{
    PURB Urb;
    PUCHAR SampleRateBuffer;
    PPIN_CONTEXT PinContext;
    NTSTATUS Status;

    /* allocate sample rate buffer */
    SampleRateBuffer = AllocFunction(sizeof(ULONG));
    if (!SampleRateBuffer)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* allocate urb */
    Urb = AllocFunction(sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST));
    if (!Urb)
    {
        /* no memory */
        FreeFunction(SampleRateBuffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* FIXME: determine controls and format urb */
    UsbBuildVendorRequest(Urb,
        URB_FUNCTION_CLASS_INTERFACE,
        sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
        USBD_TRANSFER_DIRECTION_OUT,
        0,
        0x01,
        0x200,
        0x300,
        SampleRateBuffer,
        NULL,
        2,
        NULL);

    /* get pin context */
    PinContext = Pin->Context;

    SampleRateBuffer[0] = 0xC2;
    SampleRateBuffer[1] = 0xFE;

    /* submit urb */
    Status = SubmitUrbSync(PinContext->LowerDevice, Urb);

    DPRINT1("UsbAudioSetVolume Pin %p Status %x\n", Pin, Status);
    FreeFunction(Urb);
    FreeFunction(SampleRateBuffer);
    return Status;
}

NTSTATUS
UsbAudioSetFormat(
    IN PKSPIN Pin)
{
    PURB Urb;
    PUCHAR SampleRateBuffer;
    PPIN_CONTEXT PinContext;
    NTSTATUS Status;
    PKSDATAFORMAT_WAVEFORMATEX WaveFormatEx;

    /* allocate sample rate buffer */
    SampleRateBuffer = AllocFunction(sizeof(ULONG));
    if (!SampleRateBuffer)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (IsEqualGUIDAligned(&Pin->ConnectionFormat->MajorFormat, &KSDATAFORMAT_TYPE_AUDIO) &&
        IsEqualGUIDAligned(&Pin->ConnectionFormat->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM) &&
        IsEqualGUIDAligned(&Pin->ConnectionFormat->Specifier, &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX))
    {
        WaveFormatEx = (PKSDATAFORMAT_WAVEFORMATEX)Pin->ConnectionFormat;
        SampleRateBuffer[2] = (WaveFormatEx->WaveFormatEx.nSamplesPerSec >> 16) & 0xFF;
        SampleRateBuffer[1] = (WaveFormatEx->WaveFormatEx.nSamplesPerSec >> 8) & 0xFF;
        SampleRateBuffer[0] = (WaveFormatEx->WaveFormatEx.nSamplesPerSec >> 0) & 0xFF;

        /* TODO: verify connection format */
    }
    else
    {
        /* not supported yet*/
        UNIMPLEMENTED;
        FreeFunction(SampleRateBuffer);
        return STATUS_INVALID_PARAMETER;
    }

    /* allocate urb */
    Urb = AllocFunction(sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST));
    if (!Urb)
    {
        /* no memory */
        FreeFunction(SampleRateBuffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* FIXME: determine controls and format urb */
    UsbBuildVendorRequest(Urb,
        URB_FUNCTION_CLASS_ENDPOINT,
        sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
        USBD_TRANSFER_DIRECTION_OUT,
        0,
        0x01, // SET_CUR
        0x100,
        0x81, //FIXME bEndpointAddress
        SampleRateBuffer,
        NULL,
        3,
        NULL);

    /* get pin context */
    PinContext = Pin->Context;

    /* submit urb */
    Status = SubmitUrbSync(PinContext->LowerDevice, Urb);

    DPRINT1("USBAudioPinSetDataFormat Pin %p Status %x\n", Pin, Status);
    FreeFunction(Urb);
    FreeFunction(SampleRateBuffer);
    return Status;
}

NTSTATUS
USBAudioSelectAudioStreamingInterface(
    IN PPIN_CONTEXT PinContext,
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor)
{
    PURB Urb;
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    NTSTATUS Status;

    /* grab interface descriptor */
    InterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(ConfigurationDescriptor, ConfigurationDescriptor, -1, -1, USB_DEVICE_CLASS_AUDIO, -1, -1);
    if (!InterfaceDescriptor)
    {
        /* no such interface */
        return STATUS_INVALID_PARAMETER;
    }

    /* FIXME selects the first interface with audio streaming and non zero num of endpoints */
    while (InterfaceDescriptor != NULL)
    {
        if (InterfaceDescriptor->bInterfaceSubClass == 0x02 /* AUDIO_STREAMING */ && InterfaceDescriptor->bNumEndpoints > 0) 
        {
            break;
        }
        InterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(ConfigurationDescriptor, (PVOID)((ULONG_PTR)InterfaceDescriptor + InterfaceDescriptor->bLength), -1, -1, USB_DEVICE_CLASS_AUDIO, -1, -1);
    }

    if (!InterfaceDescriptor)
    {
        /* no such interface */
        return STATUS_INVALID_PARAMETER;
    }

    Urb = AllocFunction(GET_SELECT_INTERFACE_REQUEST_SIZE(InterfaceDescriptor->bNumEndpoints));
    if (!Urb)
    {
        /* no memory */
        return USBD_STATUS_INSUFFICIENT_RESOURCES;
    }

     /* now prepare interface urb */
     UsbBuildSelectInterfaceRequest(Urb, GET_SELECT_INTERFACE_REQUEST_SIZE(InterfaceDescriptor->bNumEndpoints), DeviceExtension->ConfigurationHandle, InterfaceDescriptor->bInterfaceNumber, InterfaceDescriptor->bAlternateSetting);

     /* now select the interface */
     Status = SubmitUrbSync(DeviceExtension->LowerDevice, Urb);

     DPRINT1("USBAudioSelectAudioStreamingInterface Status %x UrbStatus %x\n", Status, Urb->UrbSelectInterface.Hdr.Status);

     /* did it succeeed */
     if (NT_SUCCESS(Status))
     {
         /* free old interface info */
         if (DeviceExtension->InterfaceInfo)
         {
             /* free old info */
             FreeFunction(DeviceExtension->InterfaceInfo);
         }

         /* alloc interface info */
         DeviceExtension->InterfaceInfo = AllocFunction(Urb->UrbSelectInterface.Interface.Length);
         if (DeviceExtension->InterfaceInfo == NULL)
         {
             /* no memory */
             FreeFunction(Urb);
             return STATUS_INSUFFICIENT_RESOURCES;
         }

         /* copy interface info */
         RtlCopyMemory(DeviceExtension->InterfaceInfo, &Urb->UrbSelectInterface.Interface, Urb->UrbSelectInterface.Interface.Length);
         PinContext->InterfaceDescriptor = InterfaceDescriptor;
     }

     /* free urb */
     FreeFunction(Urb);
     return Status;
}

VOID
NTAPI
CaptureGateOnWorkItem(
    _In_ PVOID Context)
{
    PKSPIN Pin;
    PPIN_CONTEXT PinContext;
    PKSGATE Gate;
    ULONG Count;

    /* get pin */
    Pin = Context;

    /* get pin context */
    PinContext = Pin->Context;

    do
    {
        /* acquire processing mutex */
        KsPinAcquireProcessingMutex(Pin);

        /* get pin control gate */
        Gate = KsPinGetAndGate(Pin);

        /* turn input on */
        KsGateTurnInputOn(Gate);

        /* schedule processing */
        KsPinAttemptProcessing(Pin, TRUE);

        /* release processing mutex */
        KsPinReleaseProcessingMutex(Pin);

        /* decrement worker count */
        Count = KsDecrementCountedWorker(PinContext->CaptureWorker);
    } while (Count);
}



VOID
CaptureInitializeUrbAndIrp(
    IN PKSPIN Pin,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PURB Urb;
    PUCHAR TransferBuffer;
    ULONG Index;
    PPIN_CONTEXT PinContext;

    /* get pin context */
    PinContext = Pin->Context;

    /* backup urb and transferbuffer */
    Urb = Irp->Tail.Overlay.DriverContext[0];
    TransferBuffer = Urb->UrbIsochronousTransfer.TransferBuffer;

    /* initialize irp */
    IoInitializeIrp(Irp, IoSizeOfIrp(PinContext->DeviceExtension->LowerDevice->StackSize), PinContext->DeviceExtension->LowerDevice->StackSize);

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;
    Irp->Flags = 0;
    Irp->UserBuffer = NULL;
    Irp->Tail.Overlay.DriverContext[0] = Urb;
    Irp->Tail.Overlay.DriverContext[1] = NULL;

    /* init stack location */
    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->DeviceObject = PinContext->DeviceExtension->LowerDevice;
    IoStack->Parameters.Others.Argument1 = Urb;
    IoStack->Parameters.Others.Argument2 = NULL;
    IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    IoSetCompletionRoutine(Irp, UsbAudioCaptureComplete, Pin, TRUE, TRUE, TRUE);

    RtlZeroMemory(Urb, GET_ISO_URB_SIZE(PACKET_COUNT));

    /* init urb */
    Urb->UrbIsochronousTransfer.Hdr.Function = URB_FUNCTION_ISOCH_TRANSFER;
    Urb->UrbIsochronousTransfer.Hdr.Length = GET_ISO_URB_SIZE(10);
    Urb->UrbIsochronousTransfer.PipeHandle = PinContext->DeviceExtension->InterfaceInfo->Pipes[0].PipeHandle;
    Urb->UrbIsochronousTransfer.TransferFlags = USBD_TRANSFER_DIRECTION_IN | USBD_START_ISO_TRANSFER_ASAP;
    Urb->UrbIsochronousTransfer.TransferBufferLength = PinContext->DeviceExtension->InterfaceInfo->Pipes[0].MaximumPacketSize * 10;
    Urb->UrbIsochronousTransfer.TransferBuffer = TransferBuffer;
    Urb->UrbIsochronousTransfer.NumberOfPackets = PACKET_COUNT;
    Urb->UrbIsochronousTransfer.StartFrame = 0;

    for (Index = 0; Index < PACKET_COUNT; Index++)
    {
        Urb->UrbIsochronousTransfer.IsoPacket[Index].Offset = Index * PinContext->DeviceExtension->InterfaceInfo->Pipes[0].MaximumPacketSize;
    }
}


VOID
NTAPI
CaptureAvoidPipeStarvationWorker(
    _In_ PVOID Context)
{
    PKSPIN Pin;
    PPIN_CONTEXT PinContext;
    KIRQL OldLevel;
    PLIST_ENTRY CurEntry;
    PIRP Irp;

    /* get pin */
    Pin = Context;

    /* get pin context */
    PinContext = Pin->Context;

    /* acquire spin lock */
    KeAcquireSpinLock(&PinContext->IrpListLock, &OldLevel);

    if (!IsListEmpty(&PinContext->IrpListHead))
    {
        /* sanity check */
        ASSERT(!IsListEmpty(&PinContext->IrpListHead));

        /* remove entry from list */
        CurEntry = RemoveHeadList(&PinContext->IrpListHead);

        /* release lock */
        KeReleaseSpinLock(&PinContext->IrpListLock, OldLevel);

        /* get irp offset */
        Irp = (PIRP)CONTAINING_RECORD(CurEntry, IRP, Tail.Overlay.ListEntry);

        /* reinitialize irp and urb */
        CaptureInitializeUrbAndIrp(Pin, Irp);

        KsDecrementCountedWorker(PinContext->StarvationWorker);

        /* call driver */
        IoCallDriver(PinContext->DeviceExtension->LowerDevice, Irp);
    }
    else
    {
        /* release lock */
        KeReleaseSpinLock(&PinContext->IrpListLock, OldLevel);

        KsDecrementCountedWorker(PinContext->StarvationWorker);
    }
}



NTSTATUS
InitCapturePin(
    IN PKSPIN Pin)
{
    NTSTATUS Status;
    ULONG Index;
    ULONG BufferSize;
    ULONG MaximumPacketSize;
    PIRP Irp;
    PURB Urb;
    PPIN_CONTEXT PinContext;
    PIO_STACK_LOCATION IoStack;
    PKSALLOCATOR_FRAMING_EX Framing;
    PKSGATE Gate;


    /* set sample rate */
    Status = UsbAudioSetFormat(Pin);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        return Status;
    }

    /* get pin context */
    PinContext = Pin->Context;

    /* lets get maximum packet size */
    MaximumPacketSize = GetMaxPacketSizeForInterface(PinContext->DeviceExtension->ConfigurationDescriptor, PinContext->InterfaceDescriptor, Pin->DataFlow);

    /* initialize work item for capture worker */
    ExInitializeWorkItem(&PinContext->CaptureWorkItem, CaptureGateOnWorkItem, (PVOID)Pin);

    /* register worker */
    Status = KsRegisterCountedWorker(CriticalWorkQueue, &PinContext->CaptureWorkItem, &PinContext->CaptureWorker);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        return Status;
    }

    /* initialize work item */
    ExInitializeWorkItem(&PinContext->StarvationWorkItem, CaptureAvoidPipeStarvationWorker, (PVOID)Pin);

    /* register worker */
    Status = KsRegisterCountedWorker(CriticalWorkQueue, &PinContext->StarvationWorkItem, &PinContext->StarvationWorker);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        KsUnregisterWorker(PinContext->CaptureWorker);
    }

    /* lets edit framing struct */
    Framing = (PKSALLOCATOR_FRAMING_EX)Pin->Descriptor->AllocatorFraming;
    Framing->FramingItem[0].PhysicalRange.MinFrameSize =
        Framing->FramingItem[0].PhysicalRange.MaxFrameSize =
        Framing->FramingItem[0].FramingRange.Range.MinFrameSize =
        Framing->FramingItem[0].FramingRange.Range.MaxFrameSize =
    MaximumPacketSize;

    /* calculate buffer size 8 irps * 10 iso packets * max packet size */
    BufferSize = 8 * PACKET_COUNT * MaximumPacketSize;

    /* allocate pin capture buffer */
    PinContext->BufferSize = BufferSize;
    PinContext->Buffer = AllocFunction(BufferSize);
    if (!PinContext->Buffer)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    KsAddItemToObjectBag(Pin->Bag, PinContext->Buffer, ExFreePool);

    /* init irps */
    for (Index = 0; Index < 8; Index++)
    {
        /* allocate irp */
        Irp = AllocFunction(IoSizeOfIrp(PinContext->DeviceExtension->LowerDevice->StackSize));
        if (!Irp)
        {
            /* no memory */
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* initialize irp */
        IoInitializeIrp(Irp, IoSizeOfIrp(PinContext->DeviceExtension->LowerDevice->StackSize), PinContext->DeviceExtension->LowerDevice->StackSize);
        
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        Irp->IoStatus.Information = 0;
        Irp->Flags = 0;
        Irp->UserBuffer = NULL;
        
        IoStack = IoGetNextIrpStackLocation(Irp);
        IoStack->DeviceObject = PinContext->DeviceExtension->LowerDevice;
        IoStack->Parameters.Others.Argument2 = NULL;
        IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

        IoSetCompletionRoutine(Irp, UsbAudioCaptureComplete, Pin, TRUE, TRUE, TRUE);

        /* insert into irp list */
        InsertTailList(&PinContext->IrpListHead, &Irp->Tail.Overlay.ListEntry);

        /* add to object bag*/
        KsAddItemToObjectBag(Pin->Bag, Irp, ExFreePool);

        /* FIXME select correct pipe handle */
        Status = UsbAudioAllocCaptureUrbIso(PinContext->DeviceExtension->InterfaceInfo->Pipes[0].PipeHandle, 
                                            MaximumPacketSize,
                                            &PinContext->Buffer[MaximumPacketSize * PACKET_COUNT * Index],
                                            MaximumPacketSize * PACKET_COUNT,
                                            &Urb);

        DPRINT1("InitCapturePin Irp %p Urb %p\n", Irp, Urb);

        if (NT_SUCCESS(Status))
        {
            /* get next stack location */
            IoStack = IoGetNextIrpStackLocation(Irp);

            /* store urb */
            IoStack->Parameters.Others.Argument1 = Urb;
            Irp->Tail.Overlay.DriverContext[0] = Urb;
        }
        else
        {
            /* failed */
            return Status;
        }
    }

    /* get process control gate */
    Gate = KsPinGetAndGate(Pin);

    /* turn input off */
    KsGateTurnInputOff(Gate);

    return Status;
}

NTSTATUS
InitStreamPin(
    IN PKSPIN Pin)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}




NTSTATUS
NTAPI
USBAudioPinCreate(
    _In_ PKSPIN Pin,
    _In_ PIRP Irp)
{
    PKSFILTER Filter;
    PFILTER_CONTEXT FilterContext;
    PPIN_CONTEXT PinContext;
    NTSTATUS Status;

    Filter = KsPinGetParentFilter(Pin);
    if (Filter == NULL)
    {
        /* invalid parameter */
        return STATUS_INVALID_PARAMETER;
    }

    /* get filter context */
    FilterContext = Filter->Context;

    /* allocate pin context */
    PinContext = AllocFunction(sizeof(PIN_CONTEXT));
    if (!PinContext)
    {
        /* no memory*/
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* init pin context */
    PinContext->DeviceExtension = FilterContext->DeviceExtension;
    PinContext->LowerDevice = FilterContext->LowerDevice;
    InitializeListHead(&PinContext->IrpListHead);
    InitializeListHead(&PinContext->DoneIrpListHead);
    KeInitializeSpinLock(&PinContext->IrpListLock);

    /* store pin context*/
    Pin->Context = PinContext;

    /* lets edit allocator framing struct */
    Status = _KsEdit(Pin->Bag, (PVOID*)&Pin->Descriptor, sizeof(KSPIN_DESCRIPTOR_EX), sizeof(KSPIN_DESCRIPTOR_EX), USBAUDIO_TAG);
    if (NT_SUCCESS(Status))
    {
        Status = _KsEdit(Pin->Bag, (PVOID*)&Pin->Descriptor->AllocatorFraming, sizeof(KSALLOCATOR_FRAMING_EX), sizeof(KSALLOCATOR_FRAMING_EX), USBAUDIO_TAG);
        ASSERT(Status == STATUS_SUCCESS);
    }

    /* FIXME move to build filter topology*/
    UsbAudioSetMuteOff(Pin);
    UsbAudioSetVolume(Pin);

    /* select streaming interface */
    Status = USBAudioSelectAudioStreamingInterface(PinContext, PinContext->DeviceExtension, PinContext->DeviceExtension->ConfigurationDescriptor);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        return Status;
    }

    if (Pin->DataFlow == KSPIN_DATAFLOW_OUT)
    {
        /* init capture pin */
        Status = InitCapturePin(Pin);
    }
    else
    {
        /* audio streaming pin*/
        Status = InitStreamPin(Pin);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
USBAudioPinClose(
    _In_ PKSPIN Pin,
    _In_ PIRP Irp)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
UsbAudioCaptureComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    PKSPIN Pin;
    PPIN_CONTEXT PinContext;
    KIRQL OldLevel;
    PURB Urb;

    /* get pin context */
    Pin = Context;
    PinContext = Pin->Context;

    /* get urb */
    Urb = Irp->Tail.Overlay.DriverContext[0];

    /* acquire lock */
    KeAcquireSpinLock(&PinContext->IrpListLock, &OldLevel);

    if (!NT_SUCCESS(Urb->UrbIsochronousTransfer.Hdr.Status))
    {
        //DPRINT("UsbAudioCaptureComplete Irp %p Urb %p Status %x Packet Status %x\n", Irp, Urb, Urb->UrbIsochronousTransfer.Hdr.Status, Urb->UrbIsochronousTransfer.IsoPacket[0].Status);

        /* insert entry into ready list */
        InsertTailList(&PinContext->IrpListHead, &Irp->Tail.Overlay.ListEntry);

        /* release lock */
        KeReleaseSpinLock(&PinContext->IrpListLock, OldLevel);

        KsIncrementCountedWorker(PinContext->StarvationWorker);
    }
    else
    {
        /* insert entry into done list */
        InsertTailList(&PinContext->DoneIrpListHead, &Irp->Tail.Overlay.ListEntry);

        /* release lock */
        KeReleaseSpinLock(&PinContext->IrpListLock, OldLevel);

        KsIncrementCountedWorker(PinContext->CaptureWorker);
    }

    /* done */
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
PinCaptureProcess(
    IN PKSPIN Pin)
{
    PKSSTREAM_POINTER LeadingStreamPointer;
    KIRQL OldLevel;
    PPIN_CONTEXT PinContext;
    PLIST_ENTRY CurEntry;
    PIRP Irp;
    PIO_STACK_LOCATION IoStack;
    PURB Urb;
    PUCHAR TransferBuffer, OutBuffer;
    ULONG Offset, Length;
    NTSTATUS Status;
    PKSGATE Gate;

    //DPRINT1("PinCaptureProcess\n");
    LeadingStreamPointer = KsPinGetLeadingEdgeStreamPointer(Pin, KSSTREAM_POINTER_STATE_LOCKED);
    if (LeadingStreamPointer == NULL)
    {
        /* get process control gate */
        Gate = KsPinGetAndGate(Pin);

        /* shutdown processing */
        KsGateTurnInputOff(Gate);

        return STATUS_SUCCESS;
    }

    /* get pin context */
    PinContext = Pin->Context;

    /* acquire spin lock */
    KeAcquireSpinLock(&PinContext->IrpListLock, &OldLevel);

    while (!IsListEmpty(&PinContext->DoneIrpListHead))
    {
        /* remove entry from list */
        CurEntry = RemoveHeadList(&PinContext->DoneIrpListHead);

        /* release lock */
        KeReleaseSpinLock(&PinContext->IrpListLock, OldLevel);

        /* get irp offset */
        Irp = (PIRP)CONTAINING_RECORD(CurEntry, IRP, Tail.Overlay.ListEntry);

        /* get urb from irp */
        IoStack = IoGetNextIrpStackLocation(Irp);
        Urb = (PURB)Irp->Tail.Overlay.DriverContext[0];
        ASSERT(Urb);

        Offset = PtrToUlong(Irp->Tail.Overlay.DriverContext[1]);

        /* get transfer buffer */
        TransferBuffer = Urb->UrbIsochronousTransfer.TransferBuffer;

        /* get target buffer */
        OutBuffer = (PUCHAR)LeadingStreamPointer->StreamHeader->Data;

        /* calculate length */
        Length = min(LeadingStreamPointer->OffsetOut.Count - LeadingStreamPointer->StreamHeader->DataUsed, Urb->UrbIsochronousTransfer.TransferBufferLength - Offset);

        /* FIXME copy each packet extra */
        /* copy audio bytes */
        RtlCopyMemory((PUCHAR)&OutBuffer[LeadingStreamPointer->StreamHeader->DataUsed], &TransferBuffer[Offset], Length);

        //DPRINT1("Irp %p Urb %p OutBuffer %p TransferBuffer %p Offset %lu Remaining %lu TransferBufferLength %lu Length %lu\n", Irp, Urb, OutBuffer, TransferBuffer, Offset, LeadingStreamPointer->OffsetOut.Remaining, Urb->UrbIsochronousTransfer.TransferBufferLength, Length);

        /* adjust streampointer */
        LeadingStreamPointer->StreamHeader->DataUsed += Length;

        if (Length == LeadingStreamPointer->OffsetOut.Remaining)
        {
            KsStreamPointerAdvanceOffsetsAndUnlock(LeadingStreamPointer, 0, Length, TRUE);

            /* acquire spin lock */
            KeAcquireSpinLock(&PinContext->IrpListLock, &OldLevel);

            /* adjust offset */
            Irp->Tail.Overlay.DriverContext[1] = UlongToPtr(Length);

            /* reinsert into processed list */
            InsertHeadList(&PinContext->DoneIrpListHead, &Irp->Tail.Overlay.ListEntry);

            /* release lock */
            KeReleaseSpinLock(&PinContext->IrpListLock, OldLevel);

            LeadingStreamPointer = KsPinGetLeadingEdgeStreamPointer(Pin, KSSTREAM_POINTER_STATE_LOCKED);
            if (LeadingStreamPointer == NULL)
            {
                /* no more work to be done*/
                return STATUS_PENDING;
            }
            else
            {
                /* resume work on this irp */
                continue;
            }
        }
        else
        {
            Status = KsStreamPointerAdvanceOffsets(LeadingStreamPointer, 0, Length, FALSE);
            ASSERT(Length == Urb->UrbIsochronousTransfer.TransferBufferLength - Offset);
        }


        /* acquire spin lock */
        KeAcquireSpinLock(&PinContext->IrpListLock, &OldLevel);

        InsertTailList(&PinContext->IrpListHead, &Irp->Tail.Overlay.ListEntry);
    }

    while (!IsListEmpty(&PinContext->IrpListHead))
    {
        /* remove entry from list */
        CurEntry = RemoveHeadList(&PinContext->IrpListHead);

        /* release lock */
        KeReleaseSpinLock(&PinContext->IrpListLock, OldLevel);

        /* get irp offset */
        Irp = (PIRP)CONTAINING_RECORD(CurEntry, IRP, Tail.Overlay.ListEntry);

        /* reinitialize irp and urb */
        CaptureInitializeUrbAndIrp(Pin, Irp);

        IoCallDriver(PinContext->DeviceExtension->LowerDevice, Irp);

        /* acquire spin lock */
        KeAcquireSpinLock(&PinContext->IrpListLock, &OldLevel);

    }

    /* release lock */
    KeReleaseSpinLock(&PinContext->IrpListLock, OldLevel);

    if (LeadingStreamPointer != NULL)
        KsStreamPointerUnlock(LeadingStreamPointer, FALSE);

    /* get process control gate */
    Gate = KsPinGetAndGate(Pin);

    /* shutdown processing */
    KsGateTurnInputOff(Gate);

    return STATUS_PENDING;
}


NTSTATUS
NTAPI
USBAudioPinProcess(
    _In_ PKSPIN Pin)
{
    NTSTATUS Status;

    if (Pin->DataFlow == KSPIN_DATAFLOW_OUT)
    {
        Status = PinCaptureProcess(Pin);
    }
    else
    {
        UNIMPLEMENTED;
        Status = STATUS_NOT_IMPLEMENTED;
    }

    return Status;
}


VOID
NTAPI
USBAudioPinReset(
    _In_ PKSPIN Pin)
{
    UNIMPLEMENTED
}

NTSTATUS
NTAPI
USBAudioPinSetDataFormat(
    _In_ PKSPIN Pin,
    _In_opt_ PKSDATAFORMAT OldFormat,
    _In_opt_ PKSMULTIPLE_ITEM OldAttributeList,
    _In_ const KSDATARANGE* DataRange,
    _In_opt_ const KSATTRIBUTE_LIST* AttributeRange)
{
    if (OldFormat == NULL)
    {
        /* TODO: verify connection format */
        UNIMPLEMENTED
        return STATUS_SUCCESS;
    }

    return UsbAudioSetFormat(Pin);
}

NTSTATUS
StartCaptureIsocTransfer(
    IN PKSPIN Pin)
{
    PPIN_CONTEXT PinContext;
    PLIST_ENTRY CurEntry;
    PIRP Irp;
    KIRQL OldLevel;

    /* get pin context */
    PinContext = Pin->Context;

    /* acquire spin lock */
    KeAcquireSpinLock(&PinContext->IrpListLock, &OldLevel);

    while(!IsListEmpty(&PinContext->IrpListHead))
    {
        /* remove entry from list */
        CurEntry = RemoveHeadList(&PinContext->IrpListHead);

        /* get irp offset */
        Irp = (PIRP)CONTAINING_RECORD(CurEntry, IRP, Tail.Overlay.ListEntry);

        /* release lock */
        KeReleaseSpinLock(&PinContext->IrpListLock, OldLevel);

        /* reinitialize irp and urb */
        CaptureInitializeUrbAndIrp(Pin, Irp);

        DPRINT("StartCaptureIsocTransfer Irp %p\n", Irp);
        IoCallDriver(PinContext->DeviceExtension->LowerDevice, Irp);

        /* acquire spin lock */
        KeAcquireSpinLock(&PinContext->IrpListLock, &OldLevel);

    }

    /* release lock */
    KeReleaseSpinLock(&PinContext->IrpListLock, OldLevel);

    return STATUS_SUCCESS;
}

NTSTATUS
CapturePinStateChange(
    _In_ PKSPIN Pin,
    _In_ KSSTATE ToState,
    _In_ KSSTATE FromState)
{
    NTSTATUS Status;

    if (FromState != ToState)
    {
        if (ToState)
        {
            if (ToState == KSSTATE_PAUSE)
            {
                if (FromState == KSSTATE_RUN)
                {
                    /* wait until pin processing is finished*/
                }
            }
            else
            {
                if (ToState == KSSTATE_RUN)
                {
                    Status = StartCaptureIsocTransfer(Pin);
                }
            }
        }
    }
    return Status;
}


NTSTATUS
NTAPI
USBAudioPinSetDeviceState(
    _In_ PKSPIN Pin,
    _In_ KSSTATE ToState,
    _In_ KSSTATE FromState)
{
    NTSTATUS Status;

    if (Pin->DataFlow == KSPIN_DATAFLOW_OUT)
    {
        /* handle capture state changes */
        Status = CapturePinStateChange(Pin, ToState, FromState);
    }
    else
    {
        UNIMPLEMENTED
        Status = STATUS_NOT_IMPLEMENTED;
    }

    return Status;
}

/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbohci/usb_queue.cpp
 * PURPOSE:     USB OHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbohci.h"
#include "hardware.h"

class CUSBQueue : public IUSBQueue
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    STDMETHODIMP_(ULONG) AddRef()
    {
        InterlockedIncrement(&m_Ref);
        return m_Ref;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        InterlockedDecrement(&m_Ref);

        if (!m_Ref)
        {
            delete this;
            return 0;
        }
        return m_Ref;
    }

    // com
    virtual NTSTATUS Initialize(IN PUSBHARDWAREDEVICE Hardware, PDMA_ADAPTER AdapterObject, IN PDMAMEMORYMANAGER MemManager, IN OPTIONAL PKSPIN_LOCK Lock);
    virtual ULONG GetPendingRequestCount();
    virtual NTSTATUS AddUSBRequest(IUSBRequest * Request);
    virtual NTSTATUS CancelRequests();
    virtual NTSTATUS CreateUSBRequest(IUSBRequest **OutRequest);
    virtual VOID TransferDescriptorCompletionCallback(ULONG TransferDescriptorLogicalAddress);

    // local functions
    BOOLEAN IsTransferDescriptorInEndpoint(IN POHCI_ENDPOINT_DESCRIPTOR EndpointDescriptor, IN ULONG TransferDescriptorLogicalAddress);
    NTSTATUS FindTransferDescriptorInEndpoint(IN POHCI_ENDPOINT_DESCRIPTOR EndpointDescriptor, IN ULONG TransferDescriptorLogicalAddress, OUT POHCI_ENDPOINT_DESCRIPTOR *OutEndpointDescriptor, OUT POHCI_ENDPOINT_DESCRIPTOR *OutPreviousEndpointDescriptor);
    NTSTATUS FindTransferDescriptorInInterruptHeadEndpoints(IN ULONG TransferDescriptorLogicalAddress, OUT POHCI_ENDPOINT_DESCRIPTOR *OutEndpointDescriptor, OUT POHCI_ENDPOINT_DESCRIPTOR *OutPreviousEndpointDescriptor);

    VOID CleanupEndpointDescriptor(POHCI_ENDPOINT_DESCRIPTOR EndpointDescriptor, POHCI_ENDPOINT_DESCRIPTOR PreviousEndpointDescriptor);
    POHCI_ENDPOINT_DESCRIPTOR FindInterruptEndpointDescriptor(UCHAR InterruptInterval);
    VOID PrintEndpointList(POHCI_ENDPOINT_DESCRIPTOR EndpointDescriptor);
    VOID LinkEndpoint(POHCI_ENDPOINT_DESCRIPTOR HeadEndpointDescriptor, POHCI_ENDPOINT_DESCRIPTOR EndpointDescriptor);

    // constructor / destructor
    CUSBQueue(IUnknown *OuterUnknown){}
    virtual ~CUSBQueue(){}

protected:
    LONG m_Ref;                                                                         // reference count
    KSPIN_LOCK m_Lock;                                                                  // list lock
    PUSBHARDWAREDEVICE m_Hardware;                                                      // hardware
    POHCI_ENDPOINT_DESCRIPTOR m_BulkHeadEndpointDescriptor;                             // bulk head descriptor
    POHCI_ENDPOINT_DESCRIPTOR m_ControlHeadEndpointDescriptor;                          // control head descriptor
    POHCI_ENDPOINT_DESCRIPTOR m_IsoHeadEndpointDescriptor;                              // isochronous head descriptor
    POHCI_ENDPOINT_DESCRIPTOR * m_InterruptEndpoints;
};

//=================================================================================================
// COM
//
NTSTATUS
STDMETHODCALLTYPE
CUSBQueue::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PUNKNOWN(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
CUSBQueue::Initialize(
    IN PUSBHARDWAREDEVICE Hardware,
    IN PDMA_ADAPTER AdapterObject,
    IN PDMAMEMORYMANAGER MemManager,
    IN OPTIONAL PKSPIN_LOCK Lock)
{
    //
    // get bulk endpoint descriptor
    //
    Hardware->GetBulkHeadEndpointDescriptor(&m_BulkHeadEndpointDescriptor);

    //
    // get control endpoint descriptor
    //
    Hardware->GetControlHeadEndpointDescriptor(&m_ControlHeadEndpointDescriptor);

    //
    Hardware->GetIsochronousHeadEndpointDescriptor(&m_IsoHeadEndpointDescriptor);

    //
    // get interrupt endpoints
    //
    Hardware->GetInterruptEndpointDescriptors(&m_InterruptEndpoints);

    //
    // initialize spinlock
    //
    KeInitializeSpinLock(&m_Lock);

    //
    // store hardware
    //
    m_Hardware = Hardware;

    return STATUS_SUCCESS;
}

ULONG
CUSBQueue::GetPendingRequestCount()
{
    //
    // Loop through the pending list and iterrate one for each QueueHead that
    // has a IRP to complete.
    //

    return 0;
}

VOID
CUSBQueue::LinkEndpoint(
    POHCI_ENDPOINT_DESCRIPTOR HeadEndpointDescriptor,
    POHCI_ENDPOINT_DESCRIPTOR EndpointDescriptor)
{
    POHCI_ENDPOINT_DESCRIPTOR CurrentEndpointDescriptor = HeadEndpointDescriptor;

    //
    // get last descriptor in queue
    //
    while(CurrentEndpointDescriptor->NextDescriptor)
    {
        //
        // move to last descriptor
        //
        CurrentEndpointDescriptor = (POHCI_ENDPOINT_DESCRIPTOR)CurrentEndpointDescriptor->NextDescriptor;
    }

    //
    // link endpoints
    //
    CurrentEndpointDescriptor->NextPhysicalEndpoint = EndpointDescriptor->PhysicalAddress.LowPart;
    CurrentEndpointDescriptor->NextDescriptor = EndpointDescriptor;

}

NTSTATUS
CUSBQueue::AddUSBRequest(
    IUSBRequest * Request)
{
    NTSTATUS Status;
    ULONG Type;
    KIRQL OldLevel;
    POHCI_ENDPOINT_DESCRIPTOR HeadDescriptor;
    POHCI_ENDPOINT_DESCRIPTOR Descriptor;
    POHCI_ISO_TD CurrentDescriptor;
    ULONG FrameNumber;

    DPRINT("CUSBQueue::AddUSBRequest\n");

    //
    // sanity check
    //
    ASSERT(Request != NULL);

    //
    // get request type
    //
    Type = Request->GetTransferType();

    //
    // add extra reference which is released when the request is completed
    //
    Request->AddRef();

    //
    // get transfer descriptors
    //
    Status = Request->GetEndpointDescriptor(&Descriptor);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to get transfer descriptor
        //
        DPRINT1("CUSBQueue::AddUSBRequest GetEndpointDescriptor failed with %x\n", Status);

        //
        // release reference
        //
        Request->Release();
        return Status;
    }

    //
    // check type
    //
    if (Type == USB_ENDPOINT_TYPE_BULK)
    {
        //
        // get head descriptor
        //
        HeadDescriptor = m_BulkHeadEndpointDescriptor;
    }
    else if (Type == USB_ENDPOINT_TYPE_CONTROL)
    {
        //
        // get head descriptor
        //
        HeadDescriptor = m_ControlHeadEndpointDescriptor;
    }
    else if (Type == USB_ENDPOINT_TYPE_INTERRUPT)
    {
        //
        // get head descriptor
        //
        HeadDescriptor = FindInterruptEndpointDescriptor(Request->GetInterval());
        ASSERT(HeadDescriptor);
    }
    else if (Type == USB_ENDPOINT_TYPE_ISOCHRONOUS)
    {
        //
        // get head descriptor
        //
        HeadDescriptor = m_IsoHeadEndpointDescriptor;

        //
        // get current frame number
        //
        m_Hardware->GetCurrentFrameNumber(&FrameNumber);

        //
        // increment frame number
        //
        FrameNumber++;

        //
        // apply frame number to iso transfer descriptors
        //
        CurrentDescriptor = (POHCI_ISO_TD)Descriptor->HeadLogicalDescriptor;

        DPRINT1("ISO: NextFrameNumber %x\n", FrameNumber);

        while(CurrentDescriptor)
        {
            //
            // set current frame number
            //
            CurrentDescriptor->Flags |= OHCI_ITD_SET_STARTING_FRAME(FrameNumber);

            //
            // move to next frame number
            //
            FrameNumber++;

            //
            // move to next descriptor
            //
            CurrentDescriptor = CurrentDescriptor->NextLogicalDescriptor;
        }
    }

    //
    // insert endpoint at end
    //
    LinkEndpoint(HeadDescriptor, Descriptor);

    //
    // set descriptor active
    //
    Descriptor->Flags &= ~OHCI_ENDPOINT_SKIP;

    if (Type == USB_ENDPOINT_TYPE_CONTROL || Type == USB_ENDPOINT_TYPE_BULK)
    {
        //
        // notify hardware of our request
        //
        m_Hardware->HeadEndpointDescriptorModified(Type);
    }


    return STATUS_SUCCESS;
}

NTSTATUS
CUSBQueue::CancelRequests()
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
CUSBQueue::CreateUSBRequest(
    IUSBRequest **OutRequest)
{
    PUSBREQUEST UsbRequest;
    NTSTATUS Status;

    *OutRequest = NULL;
    Status = InternalCreateUSBRequest(&UsbRequest);

    if (NT_SUCCESS(Status))
    {
        *OutRequest = UsbRequest;
    }

    return Status;
}

NTSTATUS
CUSBQueue::FindTransferDescriptorInEndpoint(
    IN POHCI_ENDPOINT_DESCRIPTOR EndpointDescriptor,
    IN ULONG TransferDescriptorLogicalAddress,
    OUT POHCI_ENDPOINT_DESCRIPTOR *OutEndpointDescriptor,
    OUT POHCI_ENDPOINT_DESCRIPTOR *OutPreviousEndpointDescriptor)
{
    POHCI_ENDPOINT_DESCRIPTOR LastDescriptor = EndpointDescriptor;


    //
    // skip first endpoint head
    //
    EndpointDescriptor = (POHCI_ENDPOINT_DESCRIPTOR)EndpointDescriptor->NextDescriptor;

    while(EndpointDescriptor)
    {
        //
        // check if the transfer descriptor is inside the list
        //
        if (IsTransferDescriptorInEndpoint(EndpointDescriptor, TransferDescriptorLogicalAddress))
        {
            //
            // found endpoint
            //
            *OutEndpointDescriptor = EndpointDescriptor;
            *OutPreviousEndpointDescriptor = LastDescriptor;

            //
            // done
            //
            return STATUS_SUCCESS;
        }

        //
        // store last endpoint
        //
        LastDescriptor = EndpointDescriptor;

        //
        // move to next
        //
        EndpointDescriptor = (POHCI_ENDPOINT_DESCRIPTOR)EndpointDescriptor->NextDescriptor;
    }

    //
    // failed to endpoint
    //
    return STATUS_NOT_FOUND;
}

NTSTATUS
CUSBQueue::FindTransferDescriptorInInterruptHeadEndpoints(IN ULONG TransferDescriptorLogicalAddress, OUT POHCI_ENDPOINT_DESCRIPTOR *OutEndpointDescriptor, OUT POHCI_ENDPOINT_DESCRIPTOR *OutPreviousEndpointDescriptor)
{
    ULONG Index;
    NTSTATUS Status;

    //
    // search descriptor in endpoint list
    //
    for(Index = 0; Index < OHCI_STATIC_ENDPOINT_COUNT; Index++)
    {
        //
        // is it in current endpoint
        //
        Status = FindTransferDescriptorInEndpoint(m_InterruptEndpoints[Index], TransferDescriptorLogicalAddress, OutEndpointDescriptor, OutPreviousEndpointDescriptor);
        if (NT_SUCCESS(Status))
        {
            //
            // found transfer descriptor
            //
            return STATUS_SUCCESS;
        }
    }

    //
    // not found
    //
    return STATUS_NOT_FOUND;
}

BOOLEAN
CUSBQueue::IsTransferDescriptorInEndpoint(
    IN POHCI_ENDPOINT_DESCRIPTOR EndpointDescriptor,
    IN ULONG TransferDescriptorLogicalAddress)
{
    POHCI_GENERAL_TD Descriptor;

    //
    // get first general transfer descriptor
    //
    Descriptor = (POHCI_GENERAL_TD)EndpointDescriptor->HeadLogicalDescriptor;

    //
    // sanity check
    //
    ASSERT(Descriptor);

    do
    {
        if (Descriptor->PhysicalAddress.LowPart == TransferDescriptorLogicalAddress)
        {
            //
            // found descriptor
            //
            return TRUE;
        }

        //
        // move to next
        //
        Descriptor = (POHCI_GENERAL_TD)Descriptor->NextLogicalDescriptor;
    }while(Descriptor);


    //
    // no descriptor found
    //
    return FALSE;
}

VOID
CUSBQueue::CleanupEndpointDescriptor(
    POHCI_ENDPOINT_DESCRIPTOR EndpointDescriptor,
    POHCI_ENDPOINT_DESCRIPTOR PreviousEndpointDescriptor)
{
    PUSBREQUEST Request;

    //
    // FIXME: verify unlinking process
    //
    PreviousEndpointDescriptor->NextDescriptor = EndpointDescriptor->NextDescriptor;
    PreviousEndpointDescriptor->NextPhysicalEndpoint = EndpointDescriptor->NextPhysicalEndpoint;

    //
    // get corresponding request
    //
    Request = PUSBREQUEST(EndpointDescriptor->Request);
    ASSERT(Request);

    //
    // notify of completion
    //
    Request->CompletionCallback(EndpointDescriptor);

    //
    // free endpoint descriptor
    //
    Request->FreeEndpointDescriptor(EndpointDescriptor);

    //
    // FIXME: check if complete
    //
    //ASSERT(Request->IsRequestComplete());

    //
    // release request
    //
    Request->Release();

}
VOID
CUSBQueue::PrintEndpointList(
    POHCI_ENDPOINT_DESCRIPTOR EndpointDescriptor)
{
    DPRINT1("CUSBQueue::PrintEndpointList HeadEndpoint %p Logical %x\n", EndpointDescriptor, EndpointDescriptor->PhysicalAddress.LowPart);

    //
    // get first general transfer descriptor
    //
    EndpointDescriptor = (POHCI_ENDPOINT_DESCRIPTOR)EndpointDescriptor->NextDescriptor;

    while(EndpointDescriptor)
    {
        DPRINT1("    CUSBQueue::PrintEndpointList Endpoint %p Logical %x\n", EndpointDescriptor, EndpointDescriptor->PhysicalAddress.LowPart);

        //
        // move to next
        //
        EndpointDescriptor = (POHCI_ENDPOINT_DESCRIPTOR)EndpointDescriptor->NextDescriptor;
    }
}

VOID
CUSBQueue::TransferDescriptorCompletionCallback(
    ULONG TransferDescriptorLogicalAddress)
{
    POHCI_ENDPOINT_DESCRIPTOR EndpointDescriptor, PreviousEndpointDescriptor;
    NTSTATUS Status;

    DPRINT("CUSBQueue::TransferDescriptorCompletionCallback transfer descriptor %x\n", TransferDescriptorLogicalAddress);

    //
    // find transfer descriptor in control list
    //
    Status = FindTransferDescriptorInEndpoint(m_ControlHeadEndpointDescriptor, TransferDescriptorLogicalAddress, &EndpointDescriptor, &PreviousEndpointDescriptor);
    if (NT_SUCCESS(Status))
    {
        //
        // cleanup endpoint
        //
        CleanupEndpointDescriptor(EndpointDescriptor, PreviousEndpointDescriptor);

        //
        // done
        //
        return;
    }

    //
    // find transfer descriptor in bulk list
    //
    Status = FindTransferDescriptorInEndpoint(m_BulkHeadEndpointDescriptor, TransferDescriptorLogicalAddress, &EndpointDescriptor, &PreviousEndpointDescriptor);
    if (NT_SUCCESS(Status))
    {
        //
        // cleanup endpoint
        //
        CleanupEndpointDescriptor(EndpointDescriptor, PreviousEndpointDescriptor);

        //
        // done
        //
        return;
    }

    //
    // find transfer descriptor in interrupt list
    //
    Status = FindTransferDescriptorInInterruptHeadEndpoints(TransferDescriptorLogicalAddress, &EndpointDescriptor, &PreviousEndpointDescriptor);
    if (NT_SUCCESS(Status))
    {
        //
        // cleanup endpoint
        //
        CleanupEndpointDescriptor(EndpointDescriptor, PreviousEndpointDescriptor);

        //
        // done
        //
        return;
    }


    //
    // hardware reported dead endpoint completed
    //
    DPRINT("CUSBQueue::TransferDescriptorCompletionCallback invalid transfer descriptor %x\n", TransferDescriptorLogicalAddress);
    ASSERT(FALSE);

}

POHCI_ENDPOINT_DESCRIPTOR
CUSBQueue::FindInterruptEndpointDescriptor(
    UCHAR InterruptInterval)
{
    ULONG Index = 0;
    ULONG Power = 1;

    //
    // sanity check
    //
    ASSERT(InterruptInterval < OHCI_BIGGEST_INTERVAL);

    //
    // find interrupt index
    //
    while (Power <= OHCI_BIGGEST_INTERVAL / 2)
    {
        //
        // is current interval greater
        //
        if (Power * 2 > InterruptInterval)
            break;

        //
        // increment power
        //
        Power *= 2;

        //
        // move to next interrupt
        //
        Index++;
    }

    DPRINT("InterruptInterval %lu Selected InterruptIndex %lu Choosen Interval %lu\n", InterruptInterval, Index, Power);

    //
    // return endpoint
    //
    return m_InterruptEndpoints[Index];
}

NTSTATUS
CreateUSBQueue(
    PUSBQUEUE *OutUsbQueue)
{
    PUSBQUEUE This;

    //
    // allocate controller
    //
    This = new(NonPagedPool, TAG_USBOHCI) CUSBQueue(0);
    if (!This)
    {
        //
        // failed to allocate
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // add reference count
    //
    This->AddRef();

    //
    // return result
    //
    *OutUsbQueue = (PUSBQUEUE)This;

    //
    // done
    //
    return STATUS_SUCCESS;
}

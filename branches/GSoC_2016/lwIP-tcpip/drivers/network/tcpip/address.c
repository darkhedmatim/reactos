/*
 * PROJECT:         ReactOS tcpip.sys
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            drivers/network/tcpip/address.c
 * PURPOSE:         tcpip.sys: addresses abstraction
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

typedef struct
{
    LIST_ENTRY ListEntry;
    TDI_ADDRESS_IP RemoteAddress;
    PIRP Irp;
    PVOID Buffer;
    ULONG BufferLength;
    PTDI_CONNECTION_INFORMATION ReturnInfo;
} RECEIVE_DATAGRAM_REQUEST;

/* The pool tags we will use for all of our allocation */
#define TAG_ADDRESS_FILE 'FrdA'
#define TAG_TCP_CONTEXT  'TCPx'
#define TAG_TCP_REQUEST  'TCPr'

/* The list of shared addresses */
static KSPIN_LOCK AddressListLock;
static LIST_ENTRY AddressListHead;

/* implementation in testing */
NTSTATUS
PrepareIrpForCancel(
	PIRP Irp,
	PDRIVER_CANCEL CancelRoutine,
	UCHAR CancelMode,
	UCHAR PendingMode
)
{
	PIO_STACK_LOCATION IrpSp;
	PTCP_CONTEXT Context;
	PTCP_REQUEST Request;
	KIRQL OldIrql;
	
	DPRINT1("Prepare for cancel\n");
	
	IoAcquireCancelSpinLock(&OldIrql);
	
	if (!Irp->Cancel)
	{
		Request = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Request), TAG_TCP_REQUEST);
		if (!Request)
		{
			DPRINT1("Allocation failed, out of memory\n");
			IoReleaseCancelSpinLock(OldIrql);
			
			return STATUS_NO_MEMORY;
		}
		
		IrpSp = IoGetCurrentIrpStackLocation(Irp);
		Context = (PTCP_CONTEXT)IrpSp->FileObject->FsContext;
		
		Request->PendingIrp = Irp;
		Request->Context = Context;
		Request->CancelMode = CancelMode;
		Request->PendingMode = PendingMode;
		
		KeAcquireSpinLockAtDpcLevel(&Context->RequestListLock);
		InsertTailList(&Context->RequestListHead, &Request->ListEntry);
		KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
		
		IoSetCancelRoutine(Irp, CancelRoutine);
		
		IoReleaseCancelSpinLock(OldIrql);
		
		DPRINT1("Prepared for cancel\n");

		return STATUS_SUCCESS;
	}
	
	DPRINT1("Already cancelled\n");
	
	IoReleaseCancelSpinLock(OldIrql);
	
	Irp->IoStatus.Status = STATUS_CANCELLED;
	Irp->IoStatus.Information = 0;
	
	return STATUS_CANCELLED;
}

/* implementation in testing */
/* Does not dequeue the request, simply marks it as cancelled */
VOID
NTAPI
CancelRequestRoutine(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ _IRQL_uses_cancel_ struct _IRP *Irp
)
{
	PIO_STACK_LOCATION IrpSp;
	PTCP_CONTEXT Context;
	PADDRESS_FILE AddressFile;
	PLIST_ENTRY Head;
	PLIST_ENTRY Entry;
	PTCP_REQUEST Request;
	KIRQL OldIrql;
	
	IoReleaseCancelSpinLock(Irp->CancelIrql);
	
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	DPRINT1("IRP Cancel on IRP at %08x, with MinorFunction %08x\n", Irp, IrpSp->MinorFunction);
	switch ((ULONG)IrpSp->FileObject->FsContext2)
	{
		case TDI_TRANSPORT_ADDRESS_FILE :
			goto DGRAM_CANCEL;
		case TDI_CONNECTION_FILE :
			Context = (PTCP_CONTEXT)IrpSp->FileObject->FsContext;
			goto TCP_CANCEL;
		default :
			DPRINT1("Cancellation error\n");
			goto FINISH;
	}
	
TCP_CANCEL:
	Irp->IoStatus.Status = STATUS_CANCELLED;
	Irp->IoStatus.Information = 0;
	
	KeAcquireSpinLock(&Context->RequestListLock, &OldIrql);
	
	AddressFile = Context->AddressFile;
	DPRINT1("AddressFile at %08x\n", AddressFile);
	Head = &Context->RequestListHead;
	Entry = Head->Flink;
	while (Entry != Head)
	{
		Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
		if (Request->PendingIrp == Irp)
		{
			switch (Request->CancelMode)
			{
				case TCP_REQUEST_CANCEL_MODE_ABORT :
					if (Context->lwip_tcp_pcb == AddressFile->lwip_tcp_pcb)
					{
						AddressFile->lwip_tcp_pcb = NULL;
					}
					tcp_abort(Context->lwip_tcp_pcb);
					Context->lwip_tcp_pcb = NULL;
					
					RemoveEntryList(Entry);
					ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
					if (!IsListEmpty(&Context->RequestListHead))
					{
						DPRINT1("Aborting PCB with outstanding requests\n");
					}
					
					break;
					
				case TCP_REQUEST_CANCEL_MODE_CLOSE :
					if (Context->lwip_tcp_pcb == AddressFile->lwip_tcp_pcb)
					{
						AddressFile->lwip_tcp_pcb = NULL;
					}
					tcp_close(Context->lwip_tcp_pcb);
					Context->lwip_tcp_pcb = NULL;
					
					RemoveEntryList(Entry);
					ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
					if (!IsListEmpty(&Context->RequestListHead))
					{
						DPRINT1("Closing PCB with outstanding requests\n");
					}
					
					break;
					
				case TCP_REQUEST_CANCEL_MODE_PRESERVE :
					RemoveEntryList(Entry);
					ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
					break;
				default :
					DPRINT1("Invalid request cancel mode\n");
					break;
			}
			KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
			goto FINISH;
		}
		Entry = Entry->Flink;
	}
	DPRINT1("Matching TCP_REQUEST not found\n");
	KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
	goto FINISH;
	
DGRAM_CANCEL:
	DPRINT1("DGRAM_CANCEL\n");
	
FINISH:
	DPRINT1("\n Minor Function: %08x\n", IrpSp->MinorFunction);

	IoAcquireCancelSpinLock(&OldIrql);
	IoSetCancelRoutine(Irp, NULL);
	IoReleaseCancelSpinLock(OldIrql);
	
	IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
	
	DPRINT1("\n  CancelRequestRoutine Exiting\n");
	
	return;
}

void
TcpIpInitializeAddresses(void)
{
    KeInitializeSpinLock(&AddressListLock);
    InitializeListHead(&AddressListHead);
}

static
BOOLEAN
AddrIsUnspecified(
    _In_ PTDI_ADDRESS_IP Address)
{
    return ((Address->in_addr == 0) || (Address->in_addr == 0xFFFFFFFF));
}

/* Implementation in testing */
void
lwip_tcp_err_callback(
	void *arg,
	err_t err)
{
	PADDRESS_FILE AddressFile;
	PTCP_CONTEXT Context;
	PTCP_REQUEST Request;
	PLIST_ENTRY Head;
	PLIST_ENTRY HeadNest;
	PLIST_ENTRY EntryNest;
	PLIST_ENTRY Entry;
	PLIST_ENTRY Temp;
	PIRP Irp;
	KIRQL OldIrql;
	KIRQL CancelIrql;
	
	NTSTATUS Status;
	
	switch (err)
	{
		case ERR_ABRT :
			DPRINT1("lwIP socket aborted\n");
			break;
		case ERR_RST :
			DPRINT1("lwIP socket reset\n");
			break;
		case ERR_CLSD :
			DPRINT1("lwIP socket closed\n");
			break;
		case ERR_CONN :
			DPRINT1("lwIP connection failed\n");
			break;
		case ERR_ARG :
			DPRINT1("lwIP invalid arguments\n");
			break;
		case ERR_IF :
			DPRINT1("Low=level error\n");
			break;
		default :
			DPRINT1("Unsupported lwIP error code: %d\n", err);
			break;
	}
	
	Status = STATUS_ADDRESS_CLOSED;
	
	switch (*((UCHAR*)arg))
	{
		case TDI_TRANSPORT_ADDRESS_FILE :
			AddressFile = (PADDRESS_FILE)arg;
			DPRINT1("AddressFile at %08x\n", AddressFile);
			KeAcquireSpinLock(&AddressFile->ContextListLock, &OldIrql);
			
			Head = &AddressFile->ContextListHead;
			Entry = Head->Flink;
			while (Entry != Head)
			{
				Context = CONTAINING_RECORD(Entry, TCP_CONTEXT, ListEntry);
				KeAcquireSpinLockAtDpcLevel(&Context->RequestListLock);
				
				HeadNest = &Context->RequestListHead;
				EntryNest = HeadNest->Flink;
				while (EntryNest != HeadNest)
				{
					Request = CONTAINING_RECORD(EntryNest, TCP_REQUEST, ListEntry);
					Irp = Request->PendingIrp;
					if (Irp)
					{
						if (Irp->Cancel)
						{
							IoAcquireCancelSpinLock(&CancelIrql);
							Irp->Cancel = FALSE;
							IoSetCancelRoutine(Irp, NULL);
							IoReleaseCancelSpinLock(CancelIrql);
						}
						Irp->IoStatus.Status = Status;
						Irp->IoStatus.Information = 0;
						IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
					}
					Temp = EntryNest->Flink;
					RemoveEntryList(EntryNest);
					ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
					EntryNest = Temp;
				}
				
				KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
				
				if (Context->lwip_tcp_pcb != AddressFile->lwip_tcp_pcb)
				{
					tcp_close(Context->lwip_tcp_pcb);
				}
				Context->lwip_tcp_pcb = NULL;
				
				Entry = Entry->Flink;
			}
			
			KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
			
			AddressFile->lwip_tcp_pcb = NULL;
			
			return;
		case TDI_CONNECTION_FILE :
			Context = (PTCP_CONTEXT)arg;
			DPRINT1("AddressFile at %08x\n", Context->AddressFile);
			KeAcquireSpinLockAtDpcLevel(&Context->RequestListLock);
				
			HeadNest = &Context->RequestListHead;
			EntryNest = HeadNest->Flink;
			while (EntryNest != HeadNest)
			{
				Request = CONTAINING_RECORD(EntryNest, TCP_REQUEST, ListEntry);
				Irp = Request->PendingIrp;
				if (Irp)
				{
					if (Irp->Cancel)
					{
						IoAcquireCancelSpinLock(&CancelIrql);
						Irp->Cancel = FALSE;
						IoSetCancelRoutine(Irp, NULL);
						IoReleaseCancelSpinLock(CancelIrql);
					}
					Irp->IoStatus.Status = Status;
					Irp->IoStatus.Information = 0;
					IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
				}
				Temp = EntryNest->Flink;
				RemoveEntryList(EntryNest);
				ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
				EntryNest = Temp;
			}
			
			KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
			
			if (Context->lwip_tcp_pcb == Context->AddressFile->lwip_tcp_pcb);
			{
				Context->AddressFile->lwip_tcp_pcb = NULL;
			}
			Context->lwip_tcp_pcb = NULL;
			
			return;
		default :
			DPRINT1("Invalid argument: %08x\n", arg);
			return;
	}
}

static
BOOLEAN
ReceiveDatagram(
    ADDRESS_FILE* AddressFile,
    struct pbuf *p,
    ip_addr_t *addr,
    u16_t port)
{
    KIRQL OldIrql;
    LIST_ENTRY* ListEntry;
    RECEIVE_DATAGRAM_REQUEST* Request;
    ip_addr_t RequestAddr;
    BOOLEAN Result = FALSE;

    NT_ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    DPRINT1("Receiving datagram for addr 0x%08x on port %u.\n", ip4_addr_get_u32(addr), port);

    /* Block any cancellation that could occur */
    IoAcquireCancelSpinLock(&OldIrql);
    KeAcquireSpinLockAtDpcLevel(&AddressFile->RequestLock);

    ListEntry = AddressFile->RequestListHead.Flink;
    while (ListEntry != &AddressFile->RequestListHead)
    {
        Request = CONTAINING_RECORD(ListEntry, RECEIVE_DATAGRAM_REQUEST, ListEntry);
        ListEntry = ListEntry->Flink;

        ip4_addr_set_u32(&RequestAddr, Request->RemoteAddress.in_addr);

        if ((RequestAddr.addr == IPADDR_ANY) ||
                (ip_addr_cmp(&RequestAddr, addr) &&
                        ((Request->RemoteAddress.sin_port == lwip_htons(port)) || !port)))
        {
            PTA_IP_ADDRESS ReturnAddress;
            PIRP Irp;

            DPRINT1("Found a corresponding IRP.\n");

            Irp = Request->Irp;

            /* We found a request for this one */
            IoSetCancelRoutine(Irp, NULL);
            RemoveEntryList(&Request->ListEntry);
            Result = TRUE;

            KeReleaseSpinLockFromDpcLevel(&AddressFile->RequestLock);
            IoReleaseCancelSpinLock(OldIrql);

            /* In case of UDP, lwip provides a pbuf directly pointing to the data.
             * In other case, we must skip the IP header */
            Irp->IoStatus.Information = pbuf_copy_partial(
                p,
                Request->Buffer,
                Request->BufferLength,
                0);
            ReturnAddress = Request->ReturnInfo->RemoteAddress;
            ReturnAddress->Address->AddressLength = TDI_ADDRESS_LENGTH_IP;
            ReturnAddress->Address->AddressType = TDI_ADDRESS_TYPE_IP;
            ReturnAddress->Address->Address->sin_port = lwip_htons(port);
            ReturnAddress->Address->Address->in_addr = ip4_addr_get_u32(addr);
            RtlZeroMemory(ReturnAddress->Address->Address->sin_zero,
                sizeof(ReturnAddress->Address->Address->sin_zero));

            if (Request->BufferLength < p->tot_len)
                Irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
            else
                Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

            ExFreePoolWithTag(Request, TAG_TCP_REQUEST);

            /* Start again from the beginning */
            IoAcquireCancelSpinLock(&OldIrql);
            KeAcquireSpinLockAtDpcLevel(&AddressFile->RequestLock);
        }
    }

    KeReleaseSpinLockFromDpcLevel(&AddressFile->RequestLock);
    IoReleaseCancelSpinLock(OldIrql);

    return Result;
}

static
void
lwip_udp_ReceiveDatagram_callback(
    void *arg,
    struct udp_pcb *pcb,
    struct pbuf *p,
    ip_addr_t *addr,
    u16_t port)
{
    UNREFERENCED_PARAMETER(pcb);

    ReceiveDatagram(arg, p, addr, port);
    pbuf_free(p);
}

/* implementation in testing */
static
err_t
lwip_tcp_accept_callback(
	void *arg,
	struct tcp_pcb *newpcb,
	err_t err)
{
	KIRQL OldIrql;
	PADDRESS_FILE AddressFile;
	PTCP_CONTEXT Context;
	PTCP_REQUEST Request;
	PIRP Irp;
	PIO_STACK_LOCATION IrpSp;
	
	PLIST_ENTRY Head;
	PLIST_ENTRY Entry;
	
	DPRINT1("lwIP TCP Accept Callback\n");
	
	IoAcquireCancelSpinLock(&OldIrql);
	AddressFile = (PADDRESS_FILE)arg;
	KeAcquireSpinLockAtDpcLevel(&AddressFile->ContextListLock);
	
	Head = &AddressFile->ContextListHead;
	Entry = Head->Flink;
	while (Entry != Head)
	{
		Context = CONTAINING_RECORD(Entry, TCP_CONTEXT, ListEntry);
		if (Context->TcpState == TCP_STATE_LISTENING)
		{
			KeReleaseSpinLockFromDpcLevel(&AddressFile->ContextListLock);
			KeAcquireSpinLockAtDpcLevel(&Context->RequestListLock);
			
			Context->lwip_tcp_pcb = newpcb;
			Context->TcpState = TCP_STATE_ACCEPTED;
			tcp_accepted(AddressFile->lwip_tcp_pcb);
			Head = &Context->RequestListHead;
			Entry = Head->Flink;
		
			Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
			Irp = Request->PendingIrp;
			IrpSp = IoGetCurrentIrpStackLocation(Irp);
			if (!Irp || IrpSp->MinorFunction != TDI_LISTEN)
			{
				DPRINT1("Received callback for canceld TDI_LISTEN\n");
				RemoveEntryList(Entry);
				Entry = Entry->Flink;
				ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
				return ERR_ABRT;
			}
			
			Request->PendingIrp = NULL;
			
			IoSetCancelRoutine(Irp, NULL);
			Irp->Cancel = FALSE;
			
			IoReleaseCancelSpinLock(OldIrql);
			
			Irp->IoStatus.Status = STATUS_SUCCESS;
			IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
			
			return ERR_OK;
		}
		Entry = Entry->Flink;
	}
	
	KeReleaseSpinLockFromDpcLevel(&AddressFile->ContextListLock);
	IoReleaseCancelSpinLock(OldIrql);
	
	return ERR_ABRT;
}

/* implementation in testing */
static
err_t
lwip_tcp_sent_callback(
	void *arg,
	struct tcp_pcb *tpcb,
	u16_t len
)
{
	PTCP_REQUEST Request;
	PTCP_CONTEXT Context;
	PLIST_ENTRY Head;
	PLIST_ENTRY Entry;
	PLIST_ENTRY Temp;
	PIRP Irp;
	PIO_STACK_LOCATION IrpSp;
	KIRQL OldIrql;
	
	PVOID Buffer;
	PTDI_REQUEST_KERNEL_SEND RequestInfo;
	UINT Len;
	
	err_t lwip_err;
	
	DPRINT1("lwIP TCP Sent Callback\n");
	
	Context = (PTCP_CONTEXT)arg;
	IoAcquireCancelSpinLock(&OldIrql);
	KeAcquireSpinLockAtDpcLevel(&Context->RequestListLock);
	
	Head = &Context->RequestListHead;
	Entry = Head->Flink;
	while (Entry != Head)
	{
		Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
		if (Request->PendingMode == TCP_REQUEST_PENDING_SEND)
		{
			Irp = Request->PendingIrp;
			Entry = Entry->Flink;
			goto FOUND;
		}
		Entry = Entry->Flink;
	}
	DPRINT1("Matching TDI_SEND request not found\n");
	KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
	IoReleaseCancelSpinLock(OldIrql);
	return ERR_ABRT;
FOUND:
	if (!Irp)
	{
		DPRINT1("Callback on cancelled IRP\n");
		RemoveEntryList(&Request->ListEntry);
		ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
		KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
		IoReleaseCancelSpinLock(OldIrql);
		return ERR_ABRT;
	}
	
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	Context = (PTCP_CONTEXT)IrpSp->FileObject->FsContext;
	
	IoSetCancelRoutine(Irp, NULL);
	Irp->Cancel = FALSE;
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = len;
	
	RemoveEntryList(&Request->ListEntry);
	
	ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
	
	lwip_err = ERR_OK;
	while (Entry != Head)
	{
		Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
		if (Request->PendingMode == TCP_REQUEST_PENDING_SEND)
		{
			Irp = Request->PendingIrp;
			if (!Irp)
			{
				Temp = Entry->Flink;
				RemoveEntryList(Entry);
				ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
				Entry = Temp;
				continue;
			}
			NdisQueryBuffer(Irp->MdlAddress, &Buffer, &Len);
			IrpSp = IoGetCurrentIrpStackLocation(Irp);
			RequestInfo = (PTDI_REQUEST_KERNEL_SEND)&IrpSp->Parameters;
			lwip_err = tcp_write(Context->lwip_tcp_pcb, Buffer, RequestInfo->SendLength, 0);
			break;
		}
		Entry = Entry->Flink;
	}
	
	KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
	IoReleaseCancelSpinLock(OldIrql);
	
	IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
	
	return lwip_err;
}

/* This implementation does not take into account any flags */
static
err_t
lwip_tcp_receive_callback(
	void *arg,
	struct tcp_pcb *tpcb,
	struct pbuf *p,
	err_t err
)
{
	PIRP Irp;
	PNDIS_BUFFER Buffer;
	PTCP_CONTEXT Context;
	PTCP_REQUEST Request;
	PIO_STACK_LOCATION IrpSp;
	PTDI_REQUEST_KERNEL_RECEIVE ReceiveInfo;
	KIRQL OldIrql;
	
	INT CopiedLength;
	INT RemainingDestBytes;
	INT RemainingSrceBytes;
	UCHAR *CurrentDestLocation;
	UCHAR *CurrentSrceLocation;
	PLIST_ENTRY Head;
	PLIST_ENTRY Entry;
	NTSTATUS Status;
	
	DPRINT1("lwIP TCP Receive Callback\n");
	
	if (err != ERR_OK)
	{
		DPRINT1("lwIP Error %d\n", err);
		return ERR_ABRT;
	}
	
	IoAcquireCancelSpinLock(&OldIrql);
	
	Context = (PTCP_CONTEXT)arg;
	DPRINT1("Context at %08x\n", Context);
	DPRINT1("AddressFile at %08x\n", Context->AddressFile);
	if (!(Context->TcpState & TCP_STATE_RECEIVING))
	{
		DPRINT1("Receive callback on connection that is not currently receiving\n");
		IoReleaseCancelSpinLock(OldIrql);
		return ERR_ARG;
	}
	KeAcquireSpinLockAtDpcLevel(&Context->RequestListLock);
	Head = &Context->RequestListHead;
	Entry = Head->Flink;
	Irp = NULL;
	while (Entry != Head)
	{
		Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
		if (!Request->PendingIrp)
		{
			RemoveEntryList(Entry);
			Entry = Entry->Flink;
			ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
			continue;
		}
		if (Request->PendingMode == TCP_REQUEST_PENDING_RECEIVE)
		{
			Irp = Request->PendingIrp;
			Entry = Entry->Flink;
			break;
		}
		Entry = Entry->Flink;
	}
	if (!Irp)
	{
		DPRINT1("Receive callback on cancelled IRP\n");
		KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
		IoReleaseCancelSpinLock(OldIrql);
		return ERR_ABRT;
	}
	DPRINT1("IRP at %08x\n", Irp);
	
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	DPRINT1("IrpSp: %08x\n", IrpSp);
	
	IoSetCancelRoutine(Irp, NULL);
	Irp->Cancel = FALSE;
	
	if (Context->lwip_tcp_pcb != tpcb)
	{
		DPRINT1("Receive tcp_pcb mismatch\n");
		tcp_abort(tpcb);
		
		RemoveEntryList(&Request->ListEntry);
		ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
		
		KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
		IoReleaseCancelSpinLock(OldIrql);
		
		Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
		
		return ERR_ABRT;
	}
	if (!(Context->TcpState & (TCP_STATE_RECEIVING|TCP_STATE_ACCEPTED|TCP_STATE_CONNECTED)))
	{
		DPRINT1("Invalid TCP state: %08x\n", Context->TcpState);
		
		KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
		IoReleaseCancelSpinLock(OldIrql);
		
		return ERR_ABRT;
	}
	
	Buffer = (PNDIS_BUFFER)Irp->MdlAddress;
	ReceiveInfo = (PTDI_REQUEST_KERNEL_RECEIVE)&IrpSp->Parameters;
	
	DPRINT1("NDIS Buffer: %08x\n", Buffer);
	
	if (Buffer->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA|MDL_SOURCE_IS_NONPAGED_POOL))
	{
		DPRINT1("Option 0\n");
		CurrentDestLocation = Buffer->MappedSystemVa;
	}
	else
	{
		DPRINT1("Option 1\n");
		CurrentDestLocation = MmMapLockedPages(Buffer, KernelMode);
	}
//	CurrentDestLocation = MmGetSystemAddressForMdl(Buffer);
	RemainingDestBytes = Buffer->ByteCount;
//	NdisQueryBuffer(Buffer, &CurrentDestLocation, &RemainingDestBytes);
	
	if (p)
	{
		DPRINT1("\n  PTDI_REQUEST_KERNEL_RECEIVE->ReceiveLength = %d\n  NDIS_BUFFER length = %d\n  pbuf->tot_len = %d\n",
			ReceiveInfo->ReceiveLength,
			RemainingDestBytes,
			p->tot_len);
	}
	else
	{
		DPRINT1("\n  The pbuf pointer p is NULL\n");
	}
	
	if (!p)
	{
		CopiedLength = 0;
		Status = STATUS_ADDRESS_CLOSED;
		goto BAD;
	}
	
	if (RemainingDestBytes <= p->len)
	{
		RtlCopyMemory(CurrentDestLocation, p->payload, RemainingDestBytes);
		CopiedLength = RemainingDestBytes;
		Status = STATUS_SUCCESS;
		goto RETURN;
	}
	else
	{
		CopiedLength = 0;
		RemainingSrceBytes = p->len;
		CurrentSrceLocation = p->payload;
		
		while (1)
		{
			if (RemainingSrceBytes < RemainingDestBytes)
			{
				RtlCopyMemory(CurrentDestLocation, CurrentSrceLocation, RemainingSrceBytes);
				
				CopiedLength += p->len;
				RemainingDestBytes -= p->len;
				CurrentDestLocation += p->len;
				
				if (p->next)
				{
					RemainingSrceBytes = p->next->len;
					CurrentSrceLocation = p->next->payload;
					
					pbuf_free(p);
					p = p->next;
					
					continue;
				}
				else
				{
					Status = STATUS_SUCCESS;
					goto RETURN;
				}
			}
			else
			{
				RtlCopyMemory(CurrentDestLocation, CurrentSrceLocation, RemainingDestBytes);
				CopiedLength += RemainingDestBytes;
				
				Status = STATUS_SUCCESS;
				goto RETURN;
			}
		}
	}
	
RETURN:
	DPRINT1("Receive CopiedLength = %d\n", CopiedLength);
	
	tcp_recved(tpcb, CopiedLength);
	
	while (Entry != Head)
	{
		Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
		if (Request->PendingMode == TCP_REQUEST_PENDING_RECEIVE)
		{
			tcp_recv(tpcb, lwip_tcp_receive_callback);
			break;
		}
	}
	
BAD:
	RemoveEntryList(&Request->ListEntry);
	ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
	
	KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
	IoReleaseCancelSpinLock(OldIrql);
	
	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = CopiedLength;
	IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
	
	return ERR_OK;
}

static
u8_t
lwip_raw_ReceiveDatagram_callback(
    void *arg,
    struct raw_pcb *pcb,
    struct pbuf *p,
    ip_addr_t *addr)
{
    BOOLEAN Result;
    ADDRESS_FILE* AddressFile = arg;

    UNREFERENCED_PARAMETER(pcb);

    /* If this is for ICMP, only process the "echo received" packets.
     * The rest is processed by lwip. */
    if (AddressFile->Protocol == IPPROTO_ICMP)
    {
        /* See icmp_input */
        s16_t hlen;
        struct ip_hdr *iphdr;

        iphdr = (struct ip_hdr *)p->payload;
        hlen = IPH_HL(iphdr) * 4;

        /* Adjust the pbuf to skip the IP header */
        if (pbuf_header(p, -hlen))
            return FALSE;

        if (*((u8_t*)p->payload) != ICMP_ER)
        {
            pbuf_header(p, hlen);
            return FALSE;
        }

        pbuf_header(p, hlen);
    }

    Result = ReceiveDatagram(arg, p, addr, 0);

    if (Result)
        pbuf_free(p);

    return Result;
}

/* implementation in testing */
static
err_t
lwip_tcp_connected_callback(
	void *arg,
	struct tcp_pcb *tpcb,
	err_t err)
{
	PTCP_REQUEST Request;
	PTCP_CONTEXT Context;
	PIRP Irp;
	KIRQL OldIrql;
	PLIST_ENTRY Head;
	PLIST_ENTRY Entry;
	
	DPRINT1("lwIP TCP Connected Callback\n");
	
	IoAcquireCancelSpinLock(&OldIrql);
	
	Context = (PTCP_CONTEXT)arg;
	Head = &Context->RequestListHead;
	Entry = Head->Flink;
	if (Head == Entry)
	{
		DPRINT1("Request list is empty\n");
		return ERR_ARG;
	}
	Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
	Irp = Request->PendingIrp;
	if (!Irp)
	{
		DPRINT1("Callback on cancelled IRP\n");
		
		KeAcquireSpinLockAtDpcLevel(&Request->Context->RequestListLock);
		
		RemoveEntryList(&Request->ListEntry);
		ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
		
		KeReleaseSpinLockFromDpcLevel(&Request->Context->RequestListLock);
		IoReleaseCancelSpinLock(OldIrql);
		
		return ERR_ABRT;
	}
	
	KeAcquireSpinLockAtDpcLevel(&Context->RequestListLock);
	
	if (Context->TcpState != TCP_STATE_CONNECTING)
	{
		DPRINT1("Invalid TCP state: %d\n", Context->TcpState);
		
		KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
		IoReleaseCancelSpinLock(OldIrql);
		
		return ERR_ABRT;
	}
	
	Context->TcpState = TCP_STATE_CONNECTED;
	
	IoSetCancelRoutine(Irp, NULL);
	Irp->Cancel = FALSE;
	
	RemoveEntryList(&Request->ListEntry);
	ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
	
	KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
	IoReleaseCancelSpinLock(OldIrql);
	
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
	
	return ERR_OK;
}

/* implementation in testing */
NTSTATUS
TcpIpCreateAddress(
    _Inout_ PIRP Irp,
    _In_ PTDI_ADDRESS_IP Address,
    _In_ IPPROTO Protocol
)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PADDRESS_FILE AddressFile;
    LIST_ENTRY* ListEntry;
    KIRQL OldIrql;
	
    USHORT Port = 1;

    /* See if this port is already taken, and find a free one if needed. */
    KeAcquireSpinLock(&AddressListLock, &OldIrql);

    ListEntry = AddressListHead.Flink;
    while (ListEntry != &AddressListHead)
    {
        AddressFile = CONTAINING_RECORD(ListEntry, ADDRESS_FILE, ListEntry);

        if (Address->sin_port)
        {
            if ((AddressFile->Protocol == Protocol) &&
                    (AddressFile->Address.sin_port == Address->sin_port))
            {
                if (IrpSp->Parameters.Create.ShareAccess)
                {
                    /* Good, we found the shared address we were looking for */
                    InterlockedIncrement(&AddressFile->RefCount);
                    KeReleaseSpinLock(&AddressListLock, OldIrql);
                    goto Success;
                }

                KeReleaseSpinLock(&AddressListLock, OldIrql);
                return STATUS_ADDRESS_ALREADY_EXISTS;
            }
        }
        else if ((AddressFile->Address.sin_port == lwip_htons(Port))
                && AddressFile->Protocol == Protocol)
        {
            Port++;
            if (Port == 0)
            {
                /* Oh no. Already 65535 ports occupied! */
                DPRINT1("No more free ports for protocol %d!\n", Protocol);
                KeReleaseSpinLock(&AddressListLock, OldIrql);
                return STATUS_TOO_MANY_ADDRESSES;
            }

            /* We must start anew to check again the previous entries in the list */
            ListEntry = &AddressListHead;
        }
        ListEntry = ListEntry->Flink;
    }

    if (!AddrIsUnspecified(Address))
    {
        /* Find the local interface for this address */
        struct netif* lwip_netif = netif_list;
        ip_addr_t IpAddr;

        ip4_addr_set_u32(&IpAddr, Address->in_addr);
        while (lwip_netif)
        {
//			DPRINT1("Comparing against address %lx\n", lwip_netif->ip_addr.addr);
            if (ip_addr_cmp(&IpAddr, &lwip_netif->ip_addr))
            {
                break;
            }
            lwip_netif = lwip_netif->next;
        }

        if (!lwip_netif)
        {
            DPRINT1("Cound not find an interface for address 0x%08x\n", AddressFile->Address.in_addr);
            KeReleaseSpinLock(&AddressListLock, OldIrql);
            return STATUS_INVALID_ADDRESS;
        }
    }

    /* Allocate our new address file */
    AddressFile = ExAllocatePoolWithTag(NonPagedPool, sizeof(*AddressFile), TAG_ADDRESS_FILE);
    if (!AddressFile)
    {
		KeReleaseSpinLock(&AddressListLock, OldIrql);
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(AddressFile, sizeof(*AddressFile));
	AddressFile->Type = TDI_TRANSPORT_ADDRESS_FILE;
    AddressFile->RefCount = 1;
    RtlCopyMemory(&AddressFile->Address, Address, sizeof(*Address));
    AddressFile->Protocol = Protocol;
    if (!Address->sin_port)
        AddressFile->Address.sin_port = lwip_htons(Port);

    /* Initialize the datagram request stuff */
    KeInitializeSpinLock(&AddressFile->RequestLock);
    InitializeListHead(&AddressFile->RequestListHead);

    /* Give it an entity ID and open a PCB if needed. */
    switch (Protocol)
    {
        case IPPROTO_TCP:
		{
			ip_addr_t IpAddr;
            ip4_addr_set_u32(&IpAddr, AddressFile->Address.in_addr);
            InsertEntityInstance(CO_TL_ENTITY, &AddressFile->Instance);
			AddressFile->ContextCount = 0;
			AddressFile->lwip_tcp_pcb = tcp_new();
			tcp_arg(AddressFile->lwip_tcp_pcb, AddressFile);
			tcp_err(AddressFile->lwip_tcp_pcb, lwip_tcp_err_callback);
            break;
		}
        case IPPROTO_UDP:
        {
            ip_addr_t IpAddr;
            ip4_addr_set_u32(&IpAddr, AddressFile->Address.in_addr);
            InsertEntityInstance(CL_TL_ENTITY, &AddressFile->Instance);
            AddressFile->lwip_udp_pcb = udp_new();
            udp_bind(AddressFile->lwip_udp_pcb, &IpAddr, lwip_ntohs(AddressFile->Address.sin_port));
            ip_set_option(AddressFile->lwip_udp_pcb, SOF_BROADCAST);
            /* Register our recv handler to lwip */
            udp_recv(
                AddressFile->lwip_udp_pcb,
                lwip_udp_ReceiveDatagram_callback,
                AddressFile);
            break;
        }
        default:
        {
            ip_addr_t IpAddr;
            ip4_addr_set_u32(&IpAddr, AddressFile->Address.in_addr);
            if (Protocol == IPPROTO_ICMP)
                InsertEntityInstance(ER_ENTITY, &AddressFile->Instance);
            else
                InsertEntityInstance(CL_TL_ENTITY, &AddressFile->Instance);
            AddressFile->lwip_raw_pcb = raw_new(Protocol);
            raw_bind(AddressFile->lwip_raw_pcb, &IpAddr);
            ip_set_option(AddressFile->lwip_raw_pcb, SOF_BROADCAST);
            /* Register our recv handler for lwip */
            raw_recv(
                AddressFile->lwip_raw_pcb,
                lwip_raw_ReceiveDatagram_callback,
                AddressFile);
            break;
        }
    }

    /* Insert it into the list. */
    InsertTailList(&AddressListHead, &AddressFile->ListEntry);
    KeReleaseSpinLock(&AddressListLock, OldIrql);

Success:
    IrpSp->FileObject->FsContext = AddressFile;
    IrpSp->FileObject->FsContext2 = (PVOID)TDI_TRANSPORT_ADDRESS_FILE;
	
    return STATUS_SUCCESS;
}

/* implementation in testing */
NTSTATUS
TcpIpCreateContext(
	_Inout_ PIRP Irp,
	_In_ PTDI_ADDRESS_IP Address,
	_In_ IPPROTO Protocol
)
{
	PIO_STACK_LOCATION IrpSp;
	PTCP_CONTEXT Context;
	
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	
	Context = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Context), TAG_TCP_CONTEXT);
	if (!Context)
	{
		return STATUS_NO_MEMORY;
	}
	
	if (Protocol != IPPROTO_TCP)
	{
		DPRINT1("Creating connection context for non-TCP protocoln");
		return STATUS_INVALID_PARAMETER;
	}
	Context->Type = TDI_CONNECTION_FILE;
	Context->Protocol = Protocol;
	RtlCopyMemory(&Context->RequestAddress, Address, sizeof(*Address));
	KeInitializeSpinLock(&Context->RequestListLock);
	InitializeListHead(&Context->RequestListHead);
	Context->lwip_tcp_pcb = NULL;
	Context->TcpState = TCP_STATE_CREATED;
	
	IrpSp->FileObject->FsContext = (PVOID)Context;
	IrpSp->FileObject->FsContext2 = (PVOID)TDI_CONNECTION_FILE;
	
	return STATUS_SUCCESS;
}

/* implementation in testing */
NTSTATUS
TcpIpCloseAddress(
    _Inout_ ADDRESS_FILE* AddressFile
)
{
    KIRQL OldIrql;
	
	err_t lwip_err;

    /* Lock the global address list */
    KeAcquireSpinLock(&AddressListLock, &OldIrql);

    if (InterlockedDecrement(&AddressFile->RefCount) != 0)
    {
        /* There are still some open handles for this address */
		DPRINT1("TcpIpCloseAddress on address with open handles\n");
        KeReleaseSpinLock(&AddressListLock, OldIrql);
        return STATUS_SUCCESS;
    }

    /* remove the lwip pcb */
	switch (AddressFile->Protocol)
	{
		case IPPROTO_UDP :
			udp_remove(AddressFile->lwip_udp_pcb);
			break;
		case IPPROTO_TCP :
			if (AddressFile->ContextCount != 0)
			{
				DPRINT1("Calling close on TCP address with open contexts\n");
				KeReleaseSpinLock(&AddressListLock, OldIrql);
				return STATUS_INVALID_PARAMETER;
			}
			if (AddressFile->lwip_tcp_pcb)
			{
				lwip_err = tcp_close(AddressFile->lwip_tcp_pcb);
				if (lwip_err != ERR_OK)
				{
					DPRINT1("lwIP tcp_close error: %d", lwip_err);
					KeReleaseSpinLock(&AddressListLock, OldIrql);
					return STATUS_UNSUCCESSFUL;
				}
			}
			break;
		case IPPROTO_RAW :
			raw_remove(AddressFile->lwip_raw_pcb);
			break;
		default :
			DPRINT1("Unknown protocol\n");
			KeReleaseSpinLock(&AddressListLock, OldIrql);
			return STATUS_INVALID_ADDRESS;
	}

    /* Remove from the list and free the structure */
    RemoveEntryList(&AddressFile->ListEntry);
    KeReleaseSpinLock(&AddressListLock, OldIrql);

    RemoveEntityInstance(&AddressFile->Instance);

    ExFreePoolWithTag(AddressFile, TAG_ADDRESS_FILE);

    return STATUS_SUCCESS;
}

/* implementation in testing */
NTSTATUS
TcpIpCloseContext(
	_In_ PTCP_CONTEXT Context)
{
	err_t lwip_err;
	PLIST_ENTRY Head;
	PLIST_ENTRY Entry;
	PTCP_REQUEST Request;
	PIRP Irp;
	PIO_STACK_LOCATION IrpSp;
	
	KIRQL OldIrql;
	
	if (Context->Protocol != IPPROTO_TCP)
	{
		DPRINT1("Invalid protocol\n");
		return STATUS_INVALID_PARAMETER;
	}
	if (Context->AddressFile)
	{
		DPRINT1("Context retains association\n");
		return STATUS_UNSUCCESSFUL;
	}
	
	KeAcquireSpinLock(&Context->RequestListLock, &OldIrql);
	
	if (Context->lwip_tcp_pcb)
	{
		lwip_err = tcp_close(Context->lwip_tcp_pcb);
		if (lwip_err != ERR_OK)
		{
			DPRINT1("lwIP tcp_close error: %d", lwip_err);
			KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
			return STATUS_UNSUCCESSFUL;
		}
	}
	
	Head = &Context->RequestListHead;
	Entry = Head->Flink;
	while (Entry != Head)
	{
		Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
		Irp = Request->PendingIrp;
		if (Irp)
		{
			IrpSp = IoGetCurrentIrpStackLocation(Irp);
			if (IrpSp->FileObject->FsContext != Context)
			{
				DPRINT1("IRP context mismatch\n");
				KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
				return STATUS_UNSUCCESSFUL;
			}
			
			DPRINT1("Unexpected outstanding request on context\n");
			KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
			return STATUS_UNSUCCESSFUL;
		}
		Entry = Entry->Flink;
		ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
	}
	
	KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
	
	ExFreePoolWithTag(Context, TAG_TCP_CONTEXT);
	
	return STATUS_SUCCESS;
}

static
NTSTATUS
ExtractAddressFromList(
    _In_ PTRANSPORT_ADDRESS AddressList,
    _Out_ PTDI_ADDRESS_IP Address)
{
    PTA_ADDRESS CurrentAddress;
    INT i;

    CurrentAddress = &AddressList->Address[0];

    /* We can only use IP addresses. Search the list until we find one */
    for (i = 0; i < AddressList->TAAddressCount; i++)
    {
        if (CurrentAddress->AddressType == TDI_ADDRESS_TYPE_IP)
        {
            if (CurrentAddress->AddressLength == TDI_ADDRESS_LENGTH_IP)
            {
                /* This is an IPv4 address */
                RtlCopyMemory(Address, &CurrentAddress->Address[0], CurrentAddress->AddressLength);
                return STATUS_SUCCESS;
            }
        }
        CurrentAddress = (PTA_ADDRESS)&CurrentAddress->Address[CurrentAddress->AddressLength];
    }
    return STATUS_INVALID_ADDRESS;
}

static
VOID
NTAPI
CancelReceiveDatagram(
    _Inout_ struct _DEVICE_OBJECT* DeviceObject,
    _Inout_ _IRQL_uses_cancel_ struct _IRP *Irp)
{
    PIO_STACK_LOCATION IrpSp;
    ADDRESS_FILE* AddressFile;
    RECEIVE_DATAGRAM_REQUEST* Request;
    LIST_ENTRY* ListEntry;
    KIRQL OldIrql;

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    AddressFile = IrpSp->FileObject->FsContext;

    /* Find this IRP in the list of requests */
    KeAcquireSpinLock(&AddressFile->RequestLock, &OldIrql);
    ListEntry = AddressFile->RequestListHead.Flink;
    while (ListEntry != &AddressFile->RequestListHead)
    {
        Request = CONTAINING_RECORD(ListEntry, RECEIVE_DATAGRAM_REQUEST, ListEntry);
        if (Request->Irp == Irp)
            break;
        ListEntry = ListEntry->Flink;
    }

    /* We must have found it */
    NT_ASSERT(ListEntry != &AddressFile->RequestListHead);

    RemoveEntryList(&Request->ListEntry);
    KeReleaseSpinLock(&AddressFile->RequestLock, OldIrql);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
}

/* implementation in testing */
NTSTATUS
TcpIpConnect(
	_Inout_ PIRP Irp)
{
	PIO_STACK_LOCATION IrpSp;
	PTCP_CONTEXT Context;
	PTDI_REQUEST_KERNEL_CONNECT Parameters;
	PTRANSPORT_ADDRESS RemoteTransportAddress;
	
	struct sockaddr *SocketAddressRemote;
	struct sockaddr_in *SocketAddressInRemote;
	
	err_t lwip_err;
	
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	
	/* Check this is really a connection file */
	if ((ULONG_PTR)IrpSp->FileObject->FsContext2 != TDI_CONNECTION_FILE)
	{
		DPRINT1("File object not a connection context\n");
		return STATUS_FILE_INVALID;
	}
	
	Context = IrpSp->FileObject->FsContext;
	DPRINT1("AddressFile at %08x\n", Context->AddressFile);
	if (Context->AddressFile->Protocol != IPPROTO_TCP)
	{
		DPRINT1("Received TDI_CONNECT for a non-TCP protocol\n");
		return STATUS_INVALID_ADDRESS;
	}
	if (Context->TcpState != TCP_STATE_BOUND)
	{
		DPRINT1("Connecting on address that's not bound\n");
		return STATUS_INVALID_ADDRESS;
	}
	
	Parameters = (PTDI_REQUEST_KERNEL_CONNECT)&IrpSp->Parameters;
		
	RemoteTransportAddress = Parameters->RequestConnectionInformation->RemoteAddress;
	
	SocketAddressRemote = (struct sockaddr *)&RemoteTransportAddress->Address[0];
	SocketAddressInRemote = (struct sockaddr_in *)&SocketAddressRemote->sa_data;
	
	DPRINT1("\n Remote Address\n  Address: %08x\n  Port: %04x\n",
		SocketAddressInRemote->sin_addr.s_addr,
		SocketAddressInRemote->sin_port);
	DPRINT1("\n Local Address\n  Address: %08x\n  Port: %04x\n",
		Context->lwip_tcp_pcb->local_ip,
		Context->lwip_tcp_pcb->local_port);
	
	lwip_err = tcp_connect(Context->lwip_tcp_pcb,
		(ip_addr_t *)&SocketAddressInRemote->sin_addr.s_addr,
		SocketAddressInRemote->sin_port,
		lwip_tcp_connected_callback);
	switch (lwip_err)
	{
		case (ERR_VAL) :
		{
			DPRINT1("lwip ERR_VAL\n");
			return STATUS_INVALID_PARAMETER;
		}
		case (ERR_ISCONN) :
		{
			DPRINT1("lwip ERR_ISCONN\n");
			return STATUS_CONNECTION_ACTIVE;
		}
		case (ERR_RTE) :
		{
			/* several errors look right here */
			DPRINT1("lwip ERR_RTE\n");
			return STATUS_NETWORK_UNREACHABLE;
		}
		case (ERR_BUF) :
		{
			/* use correct error once NDIS errors are included
				this return value means local port unavailable */
			DPRINT1("lwip ERR_BUF\n");
			return STATUS_ADDRESS_ALREADY_EXISTS;
		}
		case (ERR_USE) :
		{
			/* STATUS_CONNECTION_ACTIVE maybe? */
			DPRINT1("lwip ERR_USE\n");
			return STATUS_ADDRESS_ALREADY_EXISTS;
		}
		case (ERR_MEM) :
		{
			DPRINT1("lwip ERR_MEM\n");
			return STATUS_NO_MEMORY;
		}
		case (ERR_OK) :
		{
			PrepareIrpForCancel(
				Irp,
				CancelRequestRoutine,
				TCP_REQUEST_CANCEL_MODE_ABORT,
				TCP_REQUEST_PENDING_GENERAL);
			tcp_arg(Context->lwip_tcp_pcb, Context);
			Context->TcpState = TCP_STATE_CONNECTING;
			return STATUS_PENDING;
		}
		default :
		{
			/* unknown return value */
			DPRINT1("lwip unknown return code\n");
			return STATUS_NOT_IMPLEMENTED;
		}
	}
}

/* Implementation in testing */
NTSTATUS
TcpIpAssociateAddress(
	_Inout_ PIRP Irp)
{
	PIO_STACK_LOCATION IrpSp;
	PTCP_CONTEXT Context;
	PADDRESS_FILE AddressFile;
	PTDI_REQUEST_KERNEL_ASSOCIATE RequestInfo;
	PFILE_OBJECT FileObject;
	
	err_t lwip_err;
	NTSTATUS Status;
	KIRQL OldIrql;
	
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	
	if (IrpSp->FileObject->FsContext2 != (PVOID)TDI_CONNECTION_FILE)
	{
		DPRINT1("Associating something that is not a TDI_CONNECTION_FILE\n");
		Status = STATUS_INVALID_PARAMETER;
		goto LEAVE;
	}
	Context = IrpSp->FileObject->FsContext;
	
	if (Context->lwip_tcp_pcb || Context->Protocol != IPPROTO_TCP)
	{
		DPRINT1("Connection context is not a new TCP context\n");
		Status = STATUS_INVALID_PARAMETER;
		goto LEAVE;
	}
	
	/* Get address file */
	RequestInfo = (PTDI_REQUEST_KERNEL_ASSOCIATE)&IrpSp->Parameters;
	
	Status = ObReferenceObjectByHandle(
		RequestInfo->AddressHandle,
		0,
		*IoFileObjectType,
		KernelMode,
		(PVOID*)&FileObject,
		NULL);
	if (Status != STATUS_SUCCESS)
	{
		DPRINT1("Reference by handle failed with status 0x%08x\n", Status);
		return Status;
	}
	
	if (FileObject->FsContext2 != (PVOID)TDI_TRANSPORT_ADDRESS_FILE)
	{
		DPRINT1("File object is not an address file\n");
		ObDereferenceObject(FileObject);
		return STATUS_INVALID_PARAMETER;
	}
	AddressFile = FileObject->FsContext;
	if (AddressFile->Protocol != IPPROTO_TCP)
	{
		DPRINT1("TCP socket association with non-TCP handle\n");
		ObDereferenceObject(FileObject);
		return STATUS_INVALID_PARAMETER;
	}
	
	if (AddressFile->Address.in_addr == 0)
	{
		// TODO: should really look through address file list for an interface
		AddressFile->Address.in_addr = 0x0100007f;
	}
	
	KeAcquireSpinLock(&AddressFile->ContextListLock, &OldIrql);
	
	Context->AddressFile = AddressFile;
	InsertTailList(&AddressFile->ContextListHead,
		&Context->ListEntry);
	AddressFile->ContextCount++;
	Context->lwip_tcp_pcb = AddressFile->lwip_tcp_pcb;
	
	if (AddressFile->ContextListHead.Flink == &Context->ListEntry)
	{
		DPRINT1("lwIP tcp_bind\n");
		lwip_err = tcp_bind(
			Context->lwip_tcp_pcb,
			(ip_addr_t *)&AddressFile->Address.in_addr,
			AddressFile->Address.sin_port);
		ip_set_option(Context->lwip_tcp_pcb, SOF_BROADCAST);
		DPRINT1("lwip error %d\n TCP PCB:\n  Local Address: %08x\n  Local Port: %04x\n  Remote Address: %08x\n  Remote Port: %04x\n",
			lwip_err,
			Context->lwip_tcp_pcb->local_ip,
			Context->lwip_tcp_pcb->local_port,
			Context->lwip_tcp_pcb->remote_ip,
			Context->lwip_tcp_pcb->remote_port);
		if (lwip_err != ERR_OK)
		{
			switch (lwip_err)
			{
				case (ERR_BUF) :
				{
					DPRINT1("lwIP ERR_BUFF\n");
					Status = STATUS_NO_MEMORY;
					goto LEAVE;
				}
				case (ERR_VAL) :
				{
					DPRINT1("lwIP ERR_VAL\n");
					Status = STATUS_INVALID_PARAMETER;
					goto LEAVE;
				}
				case (ERR_USE) :
				{
					DPRINT1("lwIP ERR_USE\n");
					Status = STATUS_ADDRESS_ALREADY_EXISTS;
					goto LEAVE;
				}
				case (ERR_OK) :
				{
					DPRINT1("lwIP ERR_OK\n");
					break;
				}
				default :
				{
					DPRINT1("lwIP unexpected error\n");
					Status = STATUS_NOT_IMPLEMENTED;
					goto LEAVE;
				}
			}
		}
	}
	
	DPRINT1("TcpIpAssociateAddress Exiting\n");
	
	Context->TcpState = TCP_STATE_BOUND;
	
LEAVE:
	KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
	
	return Status;
}

/* Implementation in testing */
NTSTATUS
TcpIpDisassociateAddress(
	_Inout_ PIRP Irp)
{
	PIO_STACK_LOCATION IrpSp;
	PTCP_CONTEXT Context;
	PADDRESS_FILE AddressFile;
	
	KIRQL OldIrql;
	PLIST_ENTRY Head;
	PLIST_ENTRY Entry;
	PTCP_REQUEST Request;
	
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	if ((ULONG)IrpSp->FileObject->FsContext2 != TDI_CONNECTION_FILE)
	{
		DPRINT1("Disassociating something that is not a connection context\n");
		return STATUS_FILE_INVALID;
	}
	
	Context = IrpSp->FileObject->FsContext;
	AddressFile = Context->AddressFile;
	DPRINT1("AddressFile at %08x\n", AddressFile);
	DPRINT1("Context at %08x\n", Context);
	
	if (AddressFile->Protocol != IPPROTO_TCP)
	{
		DPRINT1("Received TDI_DISASSOCIATE_ADDRESS for non-TCP protocol\n");
		return STATUS_INVALID_ADDRESS;
	}
	if (Context->Protocol != IPPROTO_TCP)
	{
		DPRINT1("Address File and Context have mismatching protocols\n");
		return STATUS_INVALID_ADDRESS;
	}
	
	KeAcquireSpinLock(&AddressFile->ContextListLock, &OldIrql);
	
	if (AddressFile->lwip_tcp_pcb == Context->lwip_tcp_pcb)
	{
		Context->lwip_tcp_pcb = NULL;
	}
	
	KeAcquireSpinLockAtDpcLevel(&Context->RequestListLock);
	if (!(IsListEmpty(&Context->RequestListHead)))
	{
		DPRINT1("Disassociating context with outstanding requests\n");
		Head = &Context->RequestListHead;
		Entry = Head->Flink;
		while (Entry != Head)
		{
			Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
			if (Request->PendingIrp)
			{
				IrpSp = IoGetCurrentIrpStackLocation(Request->PendingIrp);
				DPRINT1("Pending IRP Control Code: %08x\n", IrpSp->MinorFunction);
			}
			else
			{
				DPRINT1("IRP is NULL\n");
			}
		}
	}
	KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
	
	RemoveEntryList(&Context->ListEntry);
	AddressFile->ContextCount--;
	Context->AddressFile = NULL;
	
	KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
	
	if (AddressFile->ContextCount == 0)
	{
		return TcpIpCloseAddress(AddressFile);
	}
	
	return STATUS_SUCCESS;
}

/* Implementation in testing */
NTSTATUS
TcpIpListen(
	_Inout_ PIRP Irp)
{
	PIO_STACK_LOCATION IrpSp;
	PADDRESS_FILE AddressFile;
	PTCP_CONTEXT Context;
	
	struct tcp_pcb *lpcb;
	KIRQL OldIrql;
	
	IrpSp = IoGetCurrentIrpStackLocation(Irp);

	/* Check this is really a context file */
	if ((ULONG_PTR)IrpSp->FileObject->FsContext2 != TDI_CONNECTION_FILE)
	{
		DPRINT1("Not a connection context\n");
		return STATUS_FILE_INVALID;
	}
	/* Get context file */
	Context = IrpSp->FileObject->FsContext;
	if (!(Context->TcpState & TCP_STATE_BOUND))
	{
		DPRINT1("Received TDI_LISTEN for a context that has not been associated\n");
		// TODO: better return code
		return STATUS_UNSUCCESSFUL;
	}
	
	AddressFile = Context->AddressFile;
	DPRINT1("AddresFile at %08x\n", AddressFile);
	
	KeAcquireSpinLock(&AddressFile->ContextListLock, &OldIrql);
	if (AddressFile->ContextListHead.Flink == &Context->ListEntry)
	{
		lpcb = tcp_listen(Context->lwip_tcp_pcb);
		AddressFile->lwip_tcp_pcb = lpcb;
		Context->lwip_tcp_pcb = lpcb;
		if (lpcb == NULL)
		{
			/* tcp_listen returning NULL can mean
				either INVALID_ADDRESS or NO_MEMORY
				if SO_REUSE is enabled in lwip options */
			DPRINT1("lwip tcp_listen error\n");
			return STATUS_INVALID_ADDRESS;
		}
		tcp_arg(lpcb, AddressFile);
		tcp_accept(lpcb, lwip_tcp_accept_callback);
	}
	KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
	
	PrepareIrpForCancel(
		Irp,
		CancelRequestRoutine,
		TCP_REQUEST_CANCEL_MODE_CLOSE,
		TCP_REQUEST_PENDING_GENERAL);
	Context->TcpState = TCP_STATE_LISTENING;
	return STATUS_PENDING;
}

NTSTATUS
TcpIpSend(
 _Inout_ PIRP Irp)
{
	PIO_STACK_LOCATION IrpSp;
	PTDI_REQUEST_KERNEL_SEND Request;
	PTCP_CONTEXT Context;
	PTCP_REQUEST TcpRequest;
	PVOID Buffer;
	UINT Len;
	
	err_t lwip_err;
	PLIST_ENTRY Head;
	PLIST_ENTRY Entry;
	PLIST_ENTRY Temp;
	KIRQL OldIrql;
	
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	Request = (PTDI_REQUEST_KERNEL_SEND)&IrpSp->Parameters;
	
	if (IrpSp->FileObject->FsContext2 != (PVOID)TDI_CONNECTION_FILE)
	{
		DPRINT1("TcpIpSend without a TDI_CONNECTION_FILE\n");
		return STATUS_INVALID_PARAMETER;
	}
	Context = IrpSp->FileObject->FsContext;
	
	if (!(Context->TcpState & (TCP_STATE_ACCEPTED|TCP_STATE_CONNECTED)))
	{
		DPRINT1("Invalid TCP state: %d\n", Context->TcpState);
		// TODO: better return code
		return STATUS_UNSUCCESSFUL;
	}
	if (!Irp->MdlAddress)
	{
		DPRINT1("TcpIpSend Empty\n");
		return STATUS_INVALID_PARAMETER;
	}
	NdisQueryBuffer(Irp->MdlAddress, &Buffer, &Len);
	
	DPRINT1("\n  PTDI_REQUEST_KERNEL_SEND->SendLength = %d\n  NDIS_BUFFER Length = %d\n",
		Request->SendLength,
		Len);
	
	if (!Context->lwip_tcp_pcb)
	{
		DPRINT1("TcpIpSend with no lwIP tcp_pcb\n");
		return STATUS_INVALID_PARAMETER;
	}
	
	KeAcquireSpinLock(&Context->RequestListLock, &OldIrql);
	
	Head = &Context->RequestListHead;
	Entry = Head->Flink;
	while (Entry != Head)
	{
		TcpRequest = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
		if (!TcpRequest->PendingIrp)
		{
			Temp = Entry->Flink;
			RemoveEntryList(Entry);
			Entry = Temp;
			ExFreePoolWithTag(TcpRequest, TAG_TCP_REQUEST);
			continue;
		}
		if (TcpRequest->PendingMode == TCP_REQUEST_PENDING_SEND)
		{
			goto LEAVE;
		}
		Entry = Entry->Flink;
	}
	
	tcp_arg(Context->lwip_tcp_pcb, Context);
	tcp_sent(Context->lwip_tcp_pcb, lwip_tcp_sent_callback);
	
	lwip_err = tcp_write(Context->lwip_tcp_pcb, Buffer, Request->SendLength, 0);
	switch (lwip_err)
	{
		case ERR_OK:
			break;
		case ERR_MEM:
			DPRINT1("lwIP ERR_MEM\n");
			KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
			return STATUS_NO_MEMORY;
		case ERR_ARG:
			DPRINT1("lwIP ERR_ARG\n");
			KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
			return STATUS_INVALID_PARAMETER;
		case ERR_CONN:
			DPRINT1("lwIP ERR_CONN\n");
			KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
			return STATUS_CONNECTION_ACTIVE;
		default:
			DPRINT1("Unknwon lwIP Error: %d\n", lwip_err);
			KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
			return STATUS_NOT_IMPLEMENTED;
	}
	
LEAVE:
	PrepareIrpForCancel(
		Irp,
		CancelRequestRoutine,
		TCP_REQUEST_CANCEL_MODE_PRESERVE,
		TCP_REQUEST_PENDING_SEND);
	KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
	return STATUS_PENDING;
}

NTSTATUS
TcpIpReceive(
	_Inout_ PIRP Irp)
{
	PIO_STACK_LOCATION IrpSp;
	PTCP_CONTEXT Context;
	
	PTDI_REQUEST_KERNEL_RECEIVE RequestInfo;
	
	DPRINT1("Receive IRP Address: %08x\n", Irp);
	
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	
	if (IrpSp->FileObject->FsContext2 != (PVOID)TDI_CONNECTION_FILE)
	{
		DPRINT1("TcpIpReceive on something that is not a connection context\n");
		return STATUS_INVALID_PARAMETER;
	}
	Context = IrpSp->FileObject->FsContext;
	DPRINT1("Receive Context Address: %08x\n", Context);
	DPRINT1("Receive Address File Address: %08x\n", Context->AddressFile);
	if (Context->TcpState & (TCP_STATE_CONNECTED|TCP_STATE_ACCEPTED))
	{
		RequestInfo = (PTDI_REQUEST_KERNEL_RECEIVE)&IrpSp->Parameters;
		DPRINT1("\n  Request Length = %d\n", RequestInfo->ReceiveLength);
		
		PrepareIrpForCancel(
			Irp,
			CancelRequestRoutine,
			TCP_REQUEST_CANCEL_MODE_PRESERVE,
			TCP_REQUEST_PENDING_RECEIVE);
		
		if (!(Context->TcpState & TCP_STATE_RECEIVING))
		{
			tcp_arg(Context->lwip_tcp_pcb, Context);
			tcp_recv(Context->lwip_tcp_pcb, lwip_tcp_receive_callback);
			Context->TcpState |= TCP_STATE_RECEIVING;
		}
		
		return STATUS_PENDING;
	}
	DPRINT1("Invalid TCP state: %d\n", Context->TcpState);
	// TODO: better return error
	return STATUS_UNSUCCESSFUL;
}

NTSTATUS
TcpIpReceiveDatagram(
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ADDRESS_FILE *AddressFile;
    RECEIVE_DATAGRAM_REQUEST* Request = NULL;
    PTDI_REQUEST_KERNEL_RECEIVEDG RequestInfo;
    NTSTATUS Status;
    KIRQL OldIrql;

    /* Check this is really an address file */
    if ((ULONG_PTR)IrpSp->FileObject->FsContext2 != TDI_TRANSPORT_ADDRESS_FILE)
    {
        Status = STATUS_FILE_INVALID;
        goto Failure;
    }

    /* Get the address file */
    AddressFile = IrpSp->FileObject->FsContext;

    if (AddressFile->Protocol == IPPROTO_TCP)
    {
        /* TCP has no such thing as datagrams */
        DPRINT1("Received TDI_RECEIVE_DATAGRAM for a TCP adress file.\n");
        Status = STATUS_INVALID_ADDRESS;
        goto Failure;
    }

    /* Queue the request */
    Request = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Request), TAG_TCP_REQUEST);
    if (!Request)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Failure;
    }
    RtlZeroMemory(Request, sizeof(*Request));
    Request->Irp = Irp;

    /* Get details about what we should be receiving */
    RequestInfo = (PTDI_REQUEST_KERNEL_RECEIVEDG)&IrpSp->Parameters;

    /* Get the address */
    if (RequestInfo->ReceiveDatagramInformation->RemoteAddressLength != 0)
    {
        Status = ExtractAddressFromList(
            RequestInfo->ReceiveDatagramInformation->RemoteAddress,
            &Request->RemoteAddress);
        if (!NT_SUCCESS(Status))
            goto Failure;
    }

    DPRINT1("Queuing datagram receive on address 0x%08x, port %u.\n",
        Request->RemoteAddress.in_addr, Request->RemoteAddress.sin_port);

    /* Get the buffer */
    Request->Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
    Request->BufferLength = MmGetMdlByteCount(Irp->MdlAddress);

    Request->ReturnInfo = RequestInfo->ReturnDatagramInformation;

    /* Prepare for potential cancellation */
    IoAcquireCancelSpinLock(&OldIrql);
    IoSetCancelRoutine(Irp, CancelReceiveDatagram);
    IoReleaseCancelSpinLock(OldIrql);

    /* Mark pending */
    Irp->IoStatus.Status = STATUS_PENDING;
    IoMarkIrpPending(Irp);

    /* We're ready to go */
    ExInterlockedInsertTailList(
        &AddressFile->RequestListHead,
        &Request->ListEntry,
        &AddressFile->RequestLock);

    return STATUS_PENDING;

Failure:
    if (Request)
        ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    return Status;
}


NTSTATUS
TcpIpSendDatagram(
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ADDRESS_FILE *AddressFile;
    PTDI_REQUEST_KERNEL_SENDDG RequestInfo;
    NTSTATUS Status;
    ip_addr_t IpAddr;
    u16_t Port;
    PVOID Buffer;
    ULONG BufferLength;
    struct pbuf* p = NULL;
    err_t lwip_error;

    /* Check this is really an address file */
    if ((ULONG_PTR)IrpSp->FileObject->FsContext2 != TDI_TRANSPORT_ADDRESS_FILE)
    {
        Status = STATUS_FILE_INVALID;
        goto Finish;
    }

    /* Get the address file */
    AddressFile = IrpSp->FileObject->FsContext;

    if (AddressFile->Protocol == IPPROTO_TCP)
    {
        /* TCP has no such thing as datagrams */
        DPRINT1("Received TDI_SEND_DATAGRAM for a TCP adress file.\n");
        Status = STATUS_INVALID_ADDRESS;
        goto Finish;
    }

    /* Get details about what we should be receiving */
    RequestInfo = (PTDI_REQUEST_KERNEL_SENDDG)&IrpSp->Parameters;

    /* Get the address */
    if (RequestInfo->SendDatagramInformation->RemoteAddressLength != 0)
    {
        TDI_ADDRESS_IP Address;
        Status = ExtractAddressFromList(
            RequestInfo->SendDatagramInformation->RemoteAddress,
            &Address);
        if (!NT_SUCCESS(Status))
            goto Finish;
        ip4_addr_set_u32(&IpAddr, Address.in_addr);
        Port = lwip_ntohs(Address.sin_port);
    }
    else
    {
        ip_addr_set_any(&IpAddr);
        Port = 0;
    }

    DPRINT1("Sending datagram to address 0x%08x, port %u\n", ip4_addr_get_u32(&IpAddr), lwip_ntohs(Port));

    /* Get the buffer */
    Buffer = MmGetSystemAddressForMdl(Irp->MdlAddress);
    BufferLength = MmGetMdlByteCount(Irp->MdlAddress);
    p = pbuf_alloc(PBUF_RAW, BufferLength, PBUF_REF);
    if (!p)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }
    p->payload = Buffer;

    /* Send it for real */
    switch (AddressFile->Protocol)
    {
        case IPPROTO_UDP:
            if (((ip4_addr_get_u32(&IpAddr) == IPADDR_ANY) ||
                    (ip4_addr_get_u32(&IpAddr) == IPADDR_BROADCAST)) &&
                    (Port == 67) && (AddressFile->Address.in_addr == 0))
            {
                struct netif* lwip_netif = netif_list;

                /*
                 * This is a DHCP packet for an address file with address 0.0.0.0.
                 * Try to find an ethernet interface with no address set,
                 * and send the packet through it.
                 */
                while (lwip_netif != NULL)
                {
                    if (ip4_addr_get_u32(&lwip_netif->ip_addr) == 0)
                        break;
                    lwip_netif = lwip_netif->next;
                }

                if (lwip_netif == NULL)
                {
                    /* Do a regular send. (This will most likely fail) */
                    lwip_error = udp_sendto(AddressFile->lwip_udp_pcb, p, &IpAddr, Port);
                }
                else
                {
                    /* We found an interface with address being 0.0.0.0 */
                    lwip_error = udp_sendto_if(AddressFile->lwip_udp_pcb, p, &IpAddr, Port, lwip_netif);
                }
            }
            else
            {
                lwip_error = udp_sendto(AddressFile->lwip_udp_pcb, p, &IpAddr, Port);
            }
            break;
        default:
            lwip_error = raw_sendto(AddressFile->lwip_raw_pcb, p, &IpAddr);
            break;
    }

    switch (lwip_error)
    {
        case ERR_OK:
            Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = BufferLength;
            break;
        case ERR_MEM:
        case ERR_BUF:
            DPRINT1("Received ERR_MEM from lwip.\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        case ERR_RTE:
            DPRINT1("Received ERR_RTE from lwip.\n");
            Status = STATUS_INVALID_ADDRESS;
            break;
        case ERR_VAL:
            DPRINT1("Received ERR_VAL from lwip.\n");
            Status = STATUS_INVALID_PARAMETER;
            break;
        default:
            DPRINT1("Received error %d from lwip.\n", lwip_error);
            Status = STATUS_UNEXPECTED_NETWORK_ERROR;
    }

Finish:
    if (p)
        pbuf_free(p);
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    return Status;
}

NTSTATUS
AddressSetIpDontFragment(
    _In_ TDIEntityID ID,
    _In_ PVOID InBuffer,
    _In_ ULONG BufferSize)
{
    /* Silently ignore.
     * lwip doesn't have such thing, and already tries to fragment data as less as possible */
    return STATUS_SUCCESS;
}

NTSTATUS
AddressSetTtl(
    _In_ TDIEntityID ID,
    _In_ PVOID InBuffer,
    _In_ ULONG BufferSize)
{
    ADDRESS_FILE* AddressFile;
    TCPIP_INSTANCE* Instance;
    NTSTATUS Status;
    ULONG Value;

    if (BufferSize < sizeof(ULONG))
        return STATUS_BUFFER_TOO_SMALL;

    /* Get the address file */
    Status = GetInstance(ID, &Instance);
    if (!NT_SUCCESS(Status))
        return Status;

    AddressFile = CONTAINING_RECORD(Instance, ADDRESS_FILE, Instance);

    /* Get the value */
    Value = *((ULONG*)InBuffer);

    switch (AddressFile->Protocol)
    {
        case IPPROTO_TCP:
            DPRINT1("TCP not supported yet.\n");
            break;
        case IPPROTO_UDP:
            AddressFile->lwip_udp_pcb->ttl = Value;
            break;
        default:
            AddressFile->lwip_raw_pcb->ttl = Value;
            break;
    }

    return STATUS_SUCCESS;
}

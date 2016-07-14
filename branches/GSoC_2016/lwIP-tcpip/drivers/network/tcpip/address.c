/*
 * PROJECT:         ReactOS tcpip.sys
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            drivers/network/tcpip/address.c
 * PURPOSE:         tcpip.sys: addresses abstraction
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

volatile long int PcbCount;
volatile long int AddrFileCount;
volatile long int ContextCount;

struct tcp_pcb *PCBList[128];
PADDRESS_FILE AddrFileList[128];
PTCP_CONTEXT ContextList[128];

KSPIN_LOCK PCBListLock;
KSPIN_LOCK AddrFileListLock;
KSPIN_LOCK ContextListLock;

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
#define TAG_DGRAM_REQST  'DgRq'
#define TAG_TCP_CONTEXT  'TCPx'
#define TAG_TCP_REQUEST  'TCPr'

/* The list of shared addresses */
static KSPIN_LOCK AddressListLock;
static LIST_ENTRY AddressListHead;

/* implementation in testing */
/* Must already hold the Context->RequestListLock */
/* Context should be in ->FileObject->FsContext */
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
	
	if (!Irp->Cancel)
	{
		Request = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Request), TAG_TCP_REQUEST);
		if (!Request)
		{
			DPRINT1("Allocation failed, out of memory\n");
			return STATUS_NO_MEMORY;
		}
		
		IrpSp = IoGetCurrentIrpStackLocation(Irp);
		Context = (PTCP_CONTEXT)IrpSp->FileObject->FsContext;
		
		Request->PendingIrp = Irp;
		Request->Context = Context;
		Request->CancelMode = CancelMode;
		Request->PendingMode = PendingMode;
		
		IoSetCancelRoutine(Irp, CancelRoutine);
		InsertTailList(&Context->RequestListHead, &Request->ListEntry);
		
		return STATUS_SUCCESS;
	}
	
	DPRINT1("Already cancelled\n");
	
	Irp->IoStatus.Status = STATUS_CANCELLED;
	Irp->IoStatus.Information = 0;
	
	return STATUS_CANCELLED;
}

/* implementation in testing */
/* TODO: get rid of datagram support, or merge datagram stuff into here */
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
	int i;
	
	IoReleaseCancelSpinLock(Irp->CancelIrql);
	
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
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
	KeAcquireSpinLock(&Context->RequestListLock, &OldIrql);
	
	Irp->IoStatus.Status = STATUS_CANCELLED;
	Irp->IoStatus.Information = 0;
	IoSetCancelRoutine(Irp, NULL);
	
	AddressFile = Context->AddressFile;
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
					tcp_arg(Context->lwip_tcp_pcb, NULL);
					tcp_abort(Context->lwip_tcp_pcb);
					KeAcquireSpinLockAtDpcLevel(&PCBListLock);
					DPRINT1("Deleting PCB at %p\n", Context->lwip_tcp_pcb);
					for (i=0; i<PcbCount; i++) {
						if (PCBList[i] == Context->lwip_tcp_pcb) {
							PCBList[i] = PCBList[i+1];
							PCBList[i+1] = NULL;
						} else if (PCBList[i] == NULL) {
							PCBList[i] = PCBList[i+1];
							PCBList[i+1] = NULL;
						}
					}
					PcbCount--;
					KeReleaseSpinLockFromDpcLevel(&PCBListLock);
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
					KeAcquireSpinLockAtDpcLevel(&PCBListLock);
					DPRINT1("Deleting PCB at %p\n", Context->lwip_tcp_pcb);
					for (i=0; i<PcbCount; i++) {
						if (PCBList[i] == Context->lwip_tcp_pcb) {
							PCBList[i] = PCBList[i+1];
							PCBList[i+1] = NULL;
						} else if (PCBList[i] == NULL) {
							PCBList[i] = PCBList[i+1];
							PCBList[i+1] = NULL;
						}
					}
					PcbCount--;
					KeReleaseSpinLockFromDpcLevel(&PCBListLock);
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
	IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
}

void
TcpIpInitializeAddresses(void)
{
	KeInitializeSpinLock(&AddressListLock);
	InitializeListHead(&AddressListHead);
	
	PcbCount = 0;
	AddrFileCount = 0;
	ContextCount = 0;
	
	RtlZeroMemory(PCBList, sizeof(PCBList));
	RtlZeroMemory(AddrFileList, sizeof(AddrFileList));
	RtlZeroMemory(ContextList, sizeof(ContextList));
	
	KeInitializeSpinLock(&PCBListLock);
	KeInitializeSpinLock(&AddrFileListLock);
	KeInitializeSpinLock(&ContextListLock);
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
	PLIST_ENTRY RequestHead;
	PLIST_ENTRY RequestEntry;
	PLIST_ENTRY Entry;
	PIRP Irp;
	KIRQL OldIrql;
	
	PIO_STACK_LOCATION IrpSp;
	
	NTSTATUS Status;
	int i;
	
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
			DPRINT1("Low-level error\n");
			break;
		default :
			DPRINT1("Unsupported lwIP error code: %d\n", err);
			break;
	}
	
	Status = STATUS_ADDRESS_CLOSED;
	
	if (!arg)
	{
		/* We did the cancelling ourselves, no need to worry about deallocations */
		return;
	}
	
	/* This switch relies on UCHAR Type being the first member in
			_ADDRESS_FILE
			_TCP_CONTEXT
		This works assuming the compiler never adds padding before the first struct member
		*/
	switch (*((UCHAR*)arg))
	{
		case TDI_TRANSPORT_ADDRESS_FILE :
			AddressFile = (PADDRESS_FILE)arg;
			KeAcquireSpinLock(&AddressFile->ContextListLock, &OldIrql);
			
			Head = &AddressFile->ContextListHead;
			Entry = Head->Flink;
			while (Entry != Head)
			{
				Context = CONTAINING_RECORD(Entry, TCP_CONTEXT, ListEntry);
				KeAcquireSpinLockAtDpcLevel(&Context->RequestListLock);
				
				RequestHead = &Context->RequestListHead;
				RequestEntry = RequestHead->Flink;
				while (RequestEntry != RequestHead)
				{
					Request = CONTAINING_RECORD(RequestEntry, TCP_REQUEST, ListEntry);
					Irp = Request->PendingIrp;
					if (Irp)
					{
						IrpSp = IoGetCurrentIrpStackLocation(Irp);
						RequestEntry = RequestEntry->Flink;
						RemoveEntryList(&Request->ListEntry);
						ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
						continue;
					}
					RequestEntry = RequestEntry->Flink;
					RemoveEntryList(&Request->ListEntry);
					ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
				}
				
				if (Context->lwip_tcp_pcb != AddressFile->lwip_tcp_pcb)
				{
					tcp_arg(Context->lwip_tcp_pcb, NULL);
					tcp_abort(Context->lwip_tcp_pcb);
					KeAcquireSpinLockAtDpcLevel(&PCBListLock);
					DPRINT1("Deleting PCB at %p\n", Context->lwip_tcp_pcb);
					for (i=0; i<PcbCount; i++) {
						if (PCBList[i] == Context->lwip_tcp_pcb) {
							PCBList[i] = PCBList[i+1];
							PCBList[i+1] = NULL;
						} else if (PCBList[i] == NULL) {
							PCBList[i] = PCBList[i+1];
							PCBList[i+1] = NULL;
						}
					}
					PcbCount--;
					KeReleaseSpinLockFromDpcLevel(&PCBListLock);
				}
				Context->lwip_tcp_pcb = NULL;
				
				KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
				
				Entry = Entry->Flink;
			}
			
			AddressFile->lwip_tcp_pcb = NULL;
			
			KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
			
			return;
		case TDI_CONNECTION_FILE :
			Context = (PTCP_CONTEXT)arg;
			KeAcquireSpinLock(&Context->RequestListLock, &OldIrql);
			
			RequestHead = &Context->RequestListHead;
			RequestEntry = RequestHead->Flink;
			while (RequestEntry != RequestHead)
			{
				Request = CONTAINING_RECORD(RequestEntry, TCP_REQUEST, ListEntry);
				Irp = Request->PendingIrp;
				if (Irp)
				{
					IrpSp = IoGetCurrentIrpStackLocation(Irp);
					RequestEntry = RequestEntry->Flink;
					RemoveEntryList(&Request->ListEntry);
					ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
					continue;
				}
				RequestEntry = RequestEntry->Flink;
				RemoveEntryList(&Request->ListEntry);
				ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
			}
			
			if (Context->AddressFile && 
				(Context->lwip_tcp_pcb == Context->AddressFile->lwip_tcp_pcb))
			{
				Context->AddressFile->lwip_tcp_pcb = NULL;
			}
			Context->lwip_tcp_pcb = NULL;
			
			KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
			
			return;
		default :
			DPRINT1("Invalid argument: %p\n", arg);
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
	KeAcquireSpinLock(&AddressFile->RequestLock, &OldIrql);

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

			KeReleaseSpinLock(&AddressFile->RequestLock, OldIrql);

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

			ExFreePoolWithTag(Request, TAG_DGRAM_REQST);

			/* Start again from the beginning */
			KeAcquireSpinLock(&AddressFile->RequestLock, &OldIrql);
		}
	}

	KeReleaseSpinLock(&AddressFile->RequestLock, OldIrql);

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
	
	AddressFile = (PADDRESS_FILE)arg;
	KeAcquireSpinLock(&AddressFile->ContextListLock, &OldIrql);
	
	Head = &AddressFile->ContextListHead;
	Entry = Head->Flink;
	while (Entry != Head)
	{
		Context = CONTAINING_RECORD(Entry, TCP_CONTEXT, ListEntry);
		KeAcquireSpinLockAtDpcLevel(&Context->RequestListLock);
		if (Context->TcpState == TCP_STATE_LISTENING)
		{
			
			Context->lwip_tcp_pcb = newpcb;
			tcp_err(Context->lwip_tcp_pcb, lwip_tcp_err_callback);
			KeAcquireSpinLockAtDpcLevel(&PCBListLock);
			PCBList[PcbCount] = newpcb;
			PcbCount++;
			KeReleaseSpinLockFromDpcLevel(&PCBListLock);
			DPRINT1("\n    PCB Count: %d\n", PcbCount);
			Context->TcpState = TCP_STATE_ACCEPTED;
			tcp_accepted(AddressFile->lwip_tcp_pcb);
			Head = &Context->RequestListHead;
			Entry = Head->Flink;
		
			Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
			Irp = Request->PendingIrp;
			if (!Irp)
			{
				DPRINT1("Received accept callback for cancelled TDI_LISTEN\n");
				RemoveEntryList(Entry);
				ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
				KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
				KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
				return ERR_ABRT;
			}
			IrpSp = IoGetCurrentIrpStackLocation(Irp);
			if (IrpSp->MinorFunction != TDI_LISTEN)
			{
				DPRINT1("The IRP is not a listening request, something is seriously wrong\n");
				/* this should maybe clean out the entire context, 
					since this should never happen */
				RemoveEntryList(Entry);
				
				KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
				KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
				
				ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
				
				Irp->IoStatus.Status = STATUS_CANCELLED;
				Irp->IoStatus.Information = 0;
				IoSetCancelRoutine(Irp, NULL);
				
				IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
				
				return ERR_ABRT;
			}
			
			Request->PendingIrp = NULL;
			
			IoSetCancelRoutine(Irp, NULL);
			Irp->Cancel = FALSE;
			
			KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
			KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
			
			Irp->IoStatus.Status = STATUS_SUCCESS;
			IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
			return ERR_OK;
		}
		KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
		Entry = Entry->Flink;
	}
	
	KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
	DPRINT1("Did not find a valid TDI_LISTEN\n");
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
	
	Context = (PTCP_CONTEXT)arg;
	KeAcquireSpinLock(&Context->RequestListLock, &OldIrql);
	
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
	KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
	return ERR_ABRT;
	
FOUND:
	if (!Irp)
	{
		DPRINT1("Callback on cancelled IRP\n");
		RemoveEntryList(&Request->ListEntry);
		ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
		KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
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
	
	KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
	
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
	KIRQL OldIrql;
	
	INT CopiedLength;
	INT RemainingDestBytes;
	INT RemainingSrceBytes;
	UCHAR *CurrentDestLocation;
	UCHAR *CurrentSrceLocation;
	PLIST_ENTRY Head;
	PLIST_ENTRY Entry;
	NTSTATUS Status;
	err_t lwip_err;
	int i;
	
	/* This error is currently unimplemented in lwIP */
/*	if (err != ERR_OK)
	{
		DPRINT1("lwIP Error %d\n", err);
		return ERR_ABRT;
	}*/
	
	Context = (PTCP_CONTEXT)arg;
	KeAcquireSpinLock(&Context->RequestListLock, &OldIrql);
	if (!(Context->TcpState & TCP_STATE_RECEIVING)) {
		DPRINT1("Receive callback on connection that is not currently receiving\n");
		KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
		return ERR_ARG;
	}
	Head = &Context->RequestListHead;
	Entry = Head->Flink;
	Irp = NULL;
	while (Entry != Head) {
		Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
		if (!Request->PendingIrp) {
			Entry = Entry->Flink;
			RemoveEntryList(&Request->ListEntry);
			ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
			continue;
		}
		if (Request->PendingMode == TCP_REQUEST_PENDING_RECEIVE) {
			Irp = Request->PendingIrp;
			Entry = Entry->Flink;
			RemoveEntryList(&Request->ListEntry);
			ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
			break;
		}
		Entry = Entry->Flink;
	}
	if (!Irp) {
		DPRINT1("Receive callback on cancelled IRP\n");
		KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
		return ERR_ABRT;
	}
	
	/* IRP found, clear out the CANCEL flag */
	IoSetCancelRoutine(Irp, NULL);
	Irp->Cancel = FALSE;
	
	if (Context->lwip_tcp_pcb != tpcb) {
		DPRINT1("Receive tcp_pcb mismatch\nAborting PCB at %p\n", tpcb);
		tcp_arg(tpcb, NULL);
		tcp_abort(tpcb);
		KeAcquireSpinLockAtDpcLevel(&PCBListLock);
		DPRINT1("Deleting PCB at %p\n", tpcb);
		for (i=0; i<PcbCount; i++) {
			if (PCBList[i] == tpcb) {
				PCBList[i] = PCBList[i+1];
				PCBList[i+1] = NULL;
			} else if (PCBList[i] == NULL) {
				PCBList[i] = PCBList[i+1];
				PCBList[i+1] = NULL;
			}
		}
		PcbCount--;
		KeReleaseSpinLockFromDpcLevel(&PCBListLock);
		
		// TODO: better return code
		Status = STATUS_UNSUCCESSFUL;
		CopiedLength = 0;
		lwip_err = ERR_ABRT;
		goto BAD;
	}
	if (!(Context->TcpState & TCP_STATE_RECEIVING) ||
		!(Context->TcpState & (TCP_STATE_CONNECTED|TCP_STATE_ACCEPTED))) {
		DPRINT1("Invalid TCP state: %08x\n", Context->TcpState);
		// TODO: better return code
		Status = STATUS_UNSUCCESSFUL;
		lwip_err = ERR_ABRT;
		goto BAD;
	}
	
	Buffer = (PNDIS_BUFFER)Irp->MdlAddress;
	
	NdisQueryBuffer(Buffer, &CurrentDestLocation, &RemainingDestBytes);
	
	if (!p) {
		/* I believe this means the pcb was closed */
		DPRINT1("NULL pbuf pointer\n");
		CopiedLength = 0;
		Status = STATUS_ADDRESS_CLOSED;
		goto BAD;
	}
	
	if (RemainingDestBytes <= p->len) {
		RtlCopyMemory(CurrentDestLocation, p->payload, RemainingDestBytes);
		CopiedLength = RemainingDestBytes;
		Status = STATUS_SUCCESS;
		goto RETURN;
	} else {
		CopiedLength = 0;
		RemainingSrceBytes = p->len;
		CurrentSrceLocation = p->payload;
		
		while (1) {
			if (RemainingSrceBytes < RemainingDestBytes) {
				RtlCopyMemory(CurrentDestLocation, CurrentSrceLocation, RemainingSrceBytes);
				
				CopiedLength += p->len;
				RemainingDestBytes -= p->len;
				CurrentDestLocation += p->len;
				
				if (p->next) {
					RemainingSrceBytes = p->next->len;
					CurrentSrceLocation = p->next->payload;
					pbuf_free(p);
					p = p->next;
					continue;
				} else {
					Status = STATUS_SUCCESS;
					goto RETURN;
				}
			} else {
				RtlCopyMemory(CurrentDestLocation, CurrentSrceLocation, RemainingDestBytes);
				CopiedLength += RemainingDestBytes;
				Status = STATUS_SUCCESS;
				goto RETURN;
			}
		}
	}
	
RETURN:
	tcp_recved(tpcb, CopiedLength);
	while (Entry != Head) {
		Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
		if (!Request->PendingIrp) {
			Entry = Entry->Flink;
			RemoveEntryList(&Request->ListEntry);
			ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
			continue;
		}
		if (Request->PendingMode == TCP_REQUEST_PENDING_RECEIVE) {
			tcp_recv(Context->lwip_tcp_pcb, lwip_tcp_receive_callback);
			break;
		}
		Entry = Entry->Flink;
	}
	lwip_err = ERR_OK;
	
BAD:
	KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
	
	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = CopiedLength;
	IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
	
	return lwip_err;
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
	err_t lwip_err;
	
	Context = (PTCP_CONTEXT)arg;
	KeAcquireSpinLock(&Context->RequestListLock, &OldIrql);
	Head = &Context->RequestListHead;
	if (IsListEmpty(Head))
	{
		DPRINT1("Request list is empty\n");
		KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
		return ERR_ARG;
	}
	Entry = RemoveHeadList(Head);
	Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
	Irp = Request->PendingIrp;
	if (!Irp)
	{
		DPRINT1("Callback on cancelled IRP\n");
		ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
		KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
		return ERR_ABRT;
	}
	IoSetCancelRoutine(Irp, NULL);
	Irp->Cancel = FALSE;
	if (Context->TcpState != TCP_STATE_CONNECTING)
	{
		DPRINT1("Invalid TCP state: %d\n", Context->TcpState);
		ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
		Irp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
		return ERR_ABRT;
	}
	
	Context->TcpState = TCP_STATE_CONNECTED;
	
	KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
	
	lwip_err = ERR_OK;
	
	ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
	
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
	
	return lwip_err;
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
				DPRINT1("Address taken\n");
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
	KeAcquireSpinLockAtDpcLevel(&AddrFileListLock);
	AddrFileList[AddrFileCount] = AddressFile;
	AddrFileCount++;
	KeReleaseSpinLockFromDpcLevel(&AddrFileListLock);
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
			KeAcquireSpinLockAtDpcLevel(&PCBListLock);
			PCBList[PcbCount] = AddressFile->lwip_tcp_pcb;
			PcbCount++;
			KeReleaseSpinLockFromDpcLevel(&PCBListLock);
			DPRINT1("\n    PCB Count: %d\n", PcbCount);
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
	KIRQL OldIrql;
	
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	
	if (Protocol != IPPROTO_TCP)
	{
		DPRINT1("Creating connection context for non-TCP protocoln");
		return STATUS_INVALID_PARAMETER;
	}
	
	Context = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Context), TAG_TCP_CONTEXT);
	KeAcquireSpinLock(&ContextListLock, &OldIrql);
	ContextList[ContextCount] = Context;
	ContextCount++;
	KeReleaseSpinLock(&ContextListLock, OldIrql);
	if (!Context)
	{
		return STATUS_NO_MEMORY;
	}
	Context->Type = TDI_CONNECTION_FILE;
	Context->TcpState = TCP_STATE_CREATED;
	Context->Protocol = Protocol;
	RtlCopyMemory(&Context->RequestAddress, Address, sizeof(*Address));
	Context->AddressFile = NULL;
	KeInitializeSpinLock(&Context->RequestListLock);
	InitializeListHead(&Context->RequestListHead);
	Context->lwip_tcp_pcb = NULL;
	
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
	NTSTATUS Status;
	
	err_t lwip_err;
	int i;

	/* Lock the global address list */
	KeAcquireSpinLock(&AddressListLock, &OldIrql);
	KeAcquireSpinLockAtDpcLevel(&AddressFile->ContextListLock);

	InterlockedDecrement(&AddressFile->RefCount);
	if (AddressFile->RefCount != 0)
	{
		/* There are still some open handles for this address */
		DPRINT1("TcpIpCloseAddress on address with %d open handles\n", AddressFile->RefCount);
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
				// Should never happen due to how callers are written. 
				KeReleaseSpinLockFromDpcLevel(&AddressFile->ContextListLock);
				KeReleaseSpinLock(&AddressListLock, OldIrql);
				return STATUS_INVALID_PARAMETER;
			}
			if (AddressFile->lwip_tcp_pcb)
			{
				if (AddressFile->lwip_tcp_pcb->state == LISTEN)
				{
					lwip_err = tcp_close(AddressFile->lwip_tcp_pcb);
				}
				else
				{
					tcp_arg(AddressFile->lwip_tcp_pcb, NULL);
					tcp_abort(AddressFile->lwip_tcp_pcb);
					lwip_err = ERR_OK;
				}
				KeAcquireSpinLockAtDpcLevel(&PCBListLock);
				DPRINT1("Deleting PCB at %p\n", AddressFile->lwip_tcp_pcb);
				for (i=0; i<PcbCount; i++) {
					if (PCBList[i] == AddressFile->lwip_tcp_pcb) {
						PCBList[i] = PCBList[i+1];
						PCBList[i+1] = NULL;
					} else if (PCBList[i] == NULL) {
						PCBList[i] = PCBList[i+1];
						PCBList[i+1] = NULL;
					}
				}
				PcbCount--;
				KeReleaseSpinLockFromDpcLevel(&PCBListLock);
				if (lwip_err != ERR_OK)
				{
					DPRINT1("lwIP tcp_close error: %d", lwip_err);
					// TODO: better return code
					Status = STATUS_UNSUCCESSFUL;
				}
				Status = STATUS_SUCCESS;
				break;
			}
			break;
		case IPPROTO_RAW :
			raw_remove(AddressFile->lwip_raw_pcb);
			Status = STATUS_SUCCESS;
			break;
		default :
			DPRINT1("Unknown protocol. Something is seriously wrong.\n");
			// Should never happen due to how callers are written. 
			// This case is particularly bad since we don't know what needs
			// to be deallocated at all. 
			Status = STATUS_INVALID_ADDRESS;
			break;
	}

	/* Remove from the list and free the structure */
	RemoveEntryList(&AddressFile->ListEntry);
	RemoveEntityInstance(&AddressFile->Instance);
	
	KeReleaseSpinLockFromDpcLevel(&AddressFile->ContextListLock);

	ExFreePoolWithTag(AddressFile, TAG_ADDRESS_FILE);
	KeAcquireSpinLockAtDpcLevel(&AddrFileListLock);
	for (i=0; i<AddrFileCount; i++) {
		if (AddrFileList[i] == NULL) {
			AddrFileList[i] = AddrFileList[i+1];
			AddrFileList[i+1] = NULL;
		} else if (AddrFileList[i] == AddressFile) {
			AddrFileList[i] = AddrFileList[i+1];
			AddrFileList[i+1] = NULL;
		}
	}
	AddrFileCount--;
	KeReleaseSpinLockFromDpcLevel(&AddrFileListLock);
	
	KeReleaseSpinLock(&AddressListLock, OldIrql);

	return Status;
}

/* implementation in testing */
NTSTATUS
TcpIpCloseContext(
	_In_ PTCP_CONTEXT Context)
{
	err_t lwip_err;
	PLIST_ENTRY Head;
	PLIST_ENTRY Entry;
	PADDRESS_FILE AddressFile;
	PTCP_REQUEST Request;
	PIRP Irp;
	PIO_STACK_LOCATION IrpSp;
	
	KIRQL OldIrql;
	NTSTATUS Status;
	int i;
	
	AddressFile = Context->AddressFile;
	
	KeAcquireSpinLock(&AddressFile->ContextListLock, &OldIrql);
	KeAcquireSpinLockAtDpcLevel(&Context->RequestListLock);
	
	if (Context->Protocol != IPPROTO_TCP)
	{
		DPRINT1("Invalid protocol\n");
		KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
		KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
		return STATUS_INVALID_PARAMETER;
	}
	if (!(Context->TcpState & TCP_STATE_DISASSOCIATED))
	{
		DPRINT1("Context retains association\n");
		KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
		KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
		return STATUS_UNSUCCESSFUL;
	}
	
	if (Context->lwip_tcp_pcb == AddressFile->lwip_tcp_pcb)
	{
		Context->lwip_tcp_pcb = NULL;
	}
	
	if (Context->lwip_tcp_pcb)
	{
		if (Context->lwip_tcp_pcb == AddressFile->lwip_tcp_pcb)
		{
			Context->lwip_tcp_pcb = NULL;
		}
		if (Context->TcpState == TCP_STATE_LISTENING)
		{
			lwip_err = tcp_close(Context->lwip_tcp_pcb);
		}
		else
		{
			tcp_arg(Context->lwip_tcp_pcb, NULL);
			tcp_abort(Context->lwip_tcp_pcb);
			lwip_err = ERR_OK;
		}
		KeAcquireSpinLockAtDpcLevel(&PCBListLock);
		DPRINT1("Deleting PCB at %p\n", Context->lwip_tcp_pcb);
		for (i=0; i<PcbCount; i++) {
			if (PCBList[i] == Context->lwip_tcp_pcb) {
				PCBList[i] = PCBList[i+1];
				PCBList[i+1] = NULL;
			} else if (PCBList[i] == NULL) {
				PCBList[i] = PCBList[i+1];
				PCBList[i+1] = NULL;
			}
		}
		PcbCount--;
		KeReleaseSpinLockFromDpcLevel(&PCBListLock);
		Context->lwip_tcp_pcb = NULL;
		if (lwip_err != ERR_OK)
		{
			DPRINT1("lwIP tcp_close error: %d", lwip_err);
			KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
			KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
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
				KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
				KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
				return STATUS_UNSUCCESSFUL;
			}
			
			DPRINT1("Unexpected outstanding request on context\n");
			KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
			KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
			return STATUS_UNSUCCESSFUL;
		}
		Entry = Entry->Flink;
		ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
	}
	
	KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
	
	ExFreePoolWithTag(Context, TAG_TCP_CONTEXT);
	KeAcquireSpinLockAtDpcLevel(&ContextListLock);
	for (i=0; i<ContextCount; i++) {
		if (ContextList[i] == NULL) {
			ContextList[i] = ContextList[i+1];
			ContextList[i+1] = NULL;
		} else if (ContextList[i] == Context) {
			ContextList[i] = ContextList[i+1];
			ContextList[i+1] = NULL;
		}
	}
	ContextCount--;
	KeReleaseSpinLockFromDpcLevel(&ContextListLock);
	
	if (AddressFile->ContextCount == 0)
	{
		DPRINT1("Closing Address File at %p from TcpIpCloseContext on Context at %p",
			AddressFile, Context);
	
		KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
		Status = TcpIpCloseAddress(AddressFile);
	}
	else
	{
		KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
		Status = STATUS_SUCCESS;
	}
	
	return Status;
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

	ExFreePoolWithTag(Request, TAG_DGRAM_REQST);
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
	KIRQL OldIrql;
	
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
	KeAcquireSpinLock(&Context->RequestListLock, &OldIrql);
	if (Context->AddressFile->Protocol != IPPROTO_TCP)
	{
		DPRINT1("Received TDI_CONNECT for a non-TCP protocol\n");
		KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
		return STATUS_INVALID_ADDRESS;
	}
	if (Context->TcpState != TCP_STATE_BOUND)
	{
		DPRINT1("Connecting on address that's not bound\n");
		KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
		return STATUS_INVALID_ADDRESS;
	}
	
	Parameters = (PTDI_REQUEST_KERNEL_CONNECT)&IrpSp->Parameters;
		
	RemoteTransportAddress = Parameters->RequestConnectionInformation->RemoteAddress;
	
	SocketAddressRemote = (struct sockaddr *)&RemoteTransportAddress->Address[0];
	SocketAddressInRemote = (struct sockaddr_in *)&SocketAddressRemote->sa_data;
	
	lwip_err = tcp_connect(Context->lwip_tcp_pcb,
		(ip_addr_t *)&SocketAddressInRemote->sin_addr.s_addr,
		SocketAddressInRemote->sin_port,
		lwip_tcp_connected_callback);
	switch (lwip_err)
	{
		case (ERR_VAL) :
		{
			DPRINT1("lwip ERR_VAL\n");
			KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
			return STATUS_INVALID_PARAMETER;
		}
		case (ERR_ISCONN) :
		{
			DPRINT1("lwip ERR_ISCONN\n");
			KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
			return STATUS_CONNECTION_ACTIVE;
		}
		case (ERR_RTE) :
		{
			/* several errors look right here */
			DPRINT1("lwip ERR_RTE\n");
			KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
			return STATUS_NETWORK_UNREACHABLE;
		}
		case (ERR_BUF) :
		{
			/* use correct error once NDIS errors are included
				this return value means local port unavailable */
			DPRINT1("lwip ERR_BUF\n");
			KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
			return STATUS_ADDRESS_ALREADY_EXISTS;
		}
		case (ERR_USE) :
		{
			/* STATUS_CONNECTION_ACTIVE maybe? */
			DPRINT1("lwip ERR_USE\n");
			KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
			return STATUS_ADDRESS_ALREADY_EXISTS;
		}
		case (ERR_MEM) :
		{
			DPRINT1("lwip ERR_MEM\n");
			KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
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
			KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
			return STATUS_PENDING;
		}
		default :
		{
			/* unknown return value */
			DPRINT1("lwip unknown return code\n");
			KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
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
	KeAcquireSpinLock(&AddressFile->ContextListLock, &OldIrql);
	if (AddressFile->Protocol != IPPROTO_TCP)
	{
		DPRINT1("TCP socket association with non-TCP handle\n");
		KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
		ObDereferenceObject(FileObject);
		return STATUS_INVALID_PARAMETER;
	}
	if (AddressFile->Address.in_addr == 0)
	{
		// TODO: should really look through address file list for an interface
		AddressFile->Address.in_addr = 0x0100007f;
	}
	
	if (IrpSp->FileObject->FsContext2 != (PVOID)TDI_CONNECTION_FILE)
	{
		DPRINT1("Associating something that is not a TDI_CONNECTION_FILE\n");
		KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
		return STATUS_INVALID_PARAMETER;
	}
	Context = IrpSp->FileObject->FsContext;
	KeAcquireSpinLockAtDpcLevel(&Context->RequestListLock);
	if (Context->lwip_tcp_pcb || Context->Protocol != IPPROTO_TCP)
	{
		DPRINT1("Connection context is not a new TCP context\n");
		KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
		KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
		return STATUS_INVALID_PARAMETER;
	}
	
	Context->AddressFile = AddressFile;
	InsertTailList(&AddressFile->ContextListHead,
		&Context->ListEntry);
	AddressFile->ContextCount++;
	Context->lwip_tcp_pcb = AddressFile->lwip_tcp_pcb;
	
	if (AddressFile->ContextListHead.Flink == &Context->ListEntry)
	{
		lwip_err = tcp_bind(
			Context->lwip_tcp_pcb,
			(ip_addr_t *)&AddressFile->Address.in_addr,
			AddressFile->Address.sin_port);
		ip_set_option(Context->lwip_tcp_pcb, SOF_BROADCAST);
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
					break;
				}
				default :
				{
					DPRINT1("lwIP unexpected error\n");
					// TODO: better return code
					Status = STATUS_UNSUCCESSFUL;
					goto LEAVE;
				}
			}
		}
	}
	
	Context->TcpState = TCP_STATE_BOUND;
	
LEAVE:
	KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
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
	
	KeAcquireSpinLock(&AddressFile->ContextListLock, &OldIrql);
	KeAcquireSpinLockAtDpcLevel(&Context->RequestListLock);
	
	if (AddressFile->Protocol != IPPROTO_TCP)
	{
		DPRINT1("Received TDI_DISASSOCIATE_ADDRESS for non-TCP protocol\n");
		KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
		KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
		return STATUS_INVALID_ADDRESS;
	}
	if (Context->Protocol != IPPROTO_TCP)
	{
		DPRINT1("Address File and Context have mismatching protocols\n");
		KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
		KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
		return STATUS_INVALID_ADDRESS;
	}
	
	if (AddressFile->lwip_tcp_pcb == Context->lwip_tcp_pcb)
	{
		Context->lwip_tcp_pcb = NULL;
	}
	
	if (!(IsListEmpty(&Context->RequestListHead)))
	{
		DPRINT1("Disassociating context with outstanding requests\n");
		Head = &Context->RequestListHead;
		Entry = Head->Flink;
		while (Entry != Head)
		{
			Request = CONTAINING_RECORD(Entry, TCP_REQUEST, ListEntry);
			Entry = Entry->Flink;
			if (!Request->PendingIrp)
			{
				RemoveEntryList(&Request->ListEntry);
				ExFreePoolWithTag(Request, TAG_TCP_REQUEST);
			}
		}
	}
	
	RemoveEntryList(&Context->ListEntry);
	AddressFile->ContextCount--;
	Context->TcpState |= TCP_STATE_DISASSOCIATED;
	
	KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
	KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
	
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
	AddressFile = Context->AddressFile;
	KeAcquireSpinLock(&AddressFile->ContextListLock, &OldIrql);
	KeAcquireSpinLockAtDpcLevel(&Context->RequestListLock);
	if (!(Context->TcpState & TCP_STATE_BOUND))
	{
		DPRINT1("Received TDI_LISTEN for a context that has not been associated\n");
		// TODO: better return code
		KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
		KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
		return STATUS_UNSUCCESSFUL;
	}
	
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
			KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
			KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
			return STATUS_INVALID_ADDRESS;
		}
		tcp_arg(lpcb, AddressFile);
		tcp_accept(lpcb, lwip_tcp_accept_callback);
	}
	
	PrepareIrpForCancel(
		Irp,
		CancelRequestRoutine,
		TCP_REQUEST_CANCEL_MODE_CLOSE,
		TCP_REQUEST_PENDING_GENERAL);
	Context->TcpState = TCP_STATE_LISTENING;
	KeReleaseSpinLockFromDpcLevel(&Context->RequestListLock);
	KeReleaseSpinLock(&AddressFile->ContextListLock, OldIrql);
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
	KeAcquireSpinLock(&Context->RequestListLock, &OldIrql);
	
	if (!(Context->TcpState & (TCP_STATE_ACCEPTED|TCP_STATE_CONNECTED)))
	{
		DPRINT1("Invalid TCP state: %d\n", Context->TcpState);
		KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
		// TODO: better return code
		return STATUS_UNSUCCESSFUL;
	}
	if (!Irp->MdlAddress)
	{
		DPRINT1("TcpIpSend Empty\n");
		KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
		return STATUS_INVALID_PARAMETER;
	}
	NdisQueryBuffer(Irp->MdlAddress, &Buffer, &Len);
	
	if (!Context->lwip_tcp_pcb)
	{
		DPRINT1("TcpIpSend with no lwIP tcp_pcb\n");
		KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
		return STATUS_INVALID_PARAMETER;
	}
	
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
	KIRQL OldIrql;
	
	PTDI_REQUEST_KERNEL_RECEIVE RequestInfo;
	
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	
	if (IrpSp->FileObject->FsContext2 != (PVOID)TDI_CONNECTION_FILE)
	{
		DPRINT1("TcpIpReceive on something that is not a connection context\n");
		return STATUS_INVALID_PARAMETER;
	}
	Context = IrpSp->FileObject->FsContext;
	KeAcquireSpinLock(&Context->RequestListLock, &OldIrql);
	//DPRINT1("Receive Context Address: %08x\n", Context);
	//DPRINT1("Receive Address File Address: %08x\n", Context->AddressFile);
	if (Context->TcpState & (TCP_STATE_CONNECTED|TCP_STATE_ACCEPTED))
	{
		RequestInfo = (PTDI_REQUEST_KERNEL_RECEIVE)&IrpSp->Parameters;
		//DPRINT1("\n  Request Length = %d\n", RequestInfo->ReceiveLength);
		
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
		
		KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
		return STATUS_PENDING;
	}
	KeReleaseSpinLock(&Context->RequestListLock, OldIrql);
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
	Request = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Request), TAG_DGRAM_REQST);
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

	/* Mark pending */
	Irp->IoStatus.Status = STATUS_PENDING;
	IoMarkIrpPending(Irp);
	IoSetCancelRoutine(Irp, CancelReceiveDatagram);

	/* We're ready to go */
	ExInterlockedInsertTailList(
		&AddressFile->RequestListHead,
		&Request->ListEntry,
		&AddressFile->RequestLock);

	return STATUS_PENDING;

Failure:
	if (Request)
		ExFreePoolWithTag(Request, TAG_DGRAM_REQST);
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

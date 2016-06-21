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

/* The list of shared addresses */
static KSPIN_LOCK AddressListLock;
static LIST_ENTRY AddressListHead;

/* implementation in testing */
NTSTATUS
PrepareIrpForCancel(
	PIRP Irp,
	PDRIVER_CANCEL CancelRoutine
)
{
	PIO_STACK_LOCATION IrpSp;
	PADDRESS_FILE AddressFile;
	KIRQL OldIrql;
	
	DPRINT1("Prepare for cancel\n");
	
	IoAcquireCancelSpinLock(&OldIrql);
	
	if (!Irp->Cancel)
	{
		IrpSp = IoGetCurrentIrpStackLocation(Irp);
		AddressFile = (PADDRESS_FILE)IrpSp->FileObject->FsContext;
		AddressFile->ConnectionContext->PendingIrp = Irp;
		IoSetCancelRoutine(Irp, CancelRoutine);
		
		IoReleaseCancelSpinLock(OldIrql);
		return STATUS_SUCCESS;
	}
	
	IoReleaseCancelSpinLock(OldIrql);
	
	Irp->IoStatus.Status = STATUS_CANCELLED;
	Irp->IoStatus.Information = 0;
	
	return Irp->IoStatus.Status;
}

/* implementation in testing */
VOID
NTAPI
CancelRequestRoutine(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ _IRQL_uses_cancel_ struct _IRP *Irp
)
{
	PIO_STACK_LOCATION IrpSp;
	PADDRESS_FILE AddressFile;
	UCHAR MinorFunction;
	KIRQL OldIrql;
	
	DPRINT1("IRP Cancel\n");
	
	IoReleaseCancelSpinLock(Irp->CancelIrql);
	
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	AddressFile = (PADDRESS_FILE)IrpSp->FileObject->FsContext;
	MinorFunction = IrpSp->MinorFunction;
	
	Irp->IoStatus.Status = STATUS_CANCELLED;
	Irp->IoStatus.Information = 0;
	
	switch(MinorFunction)
	{
		case TDI_RECEIVE:
			DPRINT1("TDI_RECEIVE Cancel\n");
			goto TCP_CANCEL;
		case TDI_RECEIVE_DATAGRAM:
			DPRINT1("TDI_RECEIVE_DATAGRAM cancelling is handled within TcpIpReceiveDatagram()\n");
			goto DGRAM_CANCEL;
		case TDI_SEND:
			DPRINT1("TDI_SEND Cancel\n");
			goto TCP_CANCEL;
		case TDI_SEND_DATAGRAM:
			DPRINT1("TDI_SEND_DATAGRAM Cancel\n");
			goto DGRAM_CANCEL;
		case TDI_LISTEN:
			DPRINT1("TDI_LISTEN Cancel\n");
			goto TCP_CANCEL;
		case TDI_CONNECT:
			DPRINT1("TDI_CONNECT Cancel\n");
			goto TCP_CANCEL;
		case TDI_DISCONNECT:
			DPRINT1("TDI_DISCONNECT Cancel\n");
			goto TCP_CANCEL;
		default:
			DPRINT1("Invalid MinorFunction for cancelling IRP\n");
			return;
	}
	
TCP_CANCEL:
	if (AddressFile->ConnectionContext)
	{
		if (AddressFile->ConnectionContext->lwip_tcp_pcb)
		{
			tcp_abort(AddressFile->ConnectionContext->lwip_tcp_pcb);
			AddressFile->ConnectionContext->PendingIrp = NULL;
			AddressFile->ConnectionContext->lwip_tcp_pcb = NULL;
			DPRINT1("TCP_CANCEL\n");
		}
	}
	goto FINISH;
	
DGRAM_CANCEL:
	DPRINT1("DGRAM_CANCEL\n");
	
FINISH:
	IoAcquireCancelSpinLock(&OldIrql);
	IoSetCancelRoutine(Irp, NULL);
	IoReleaseCancelSpinLock(OldIrql);
	
	IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
	
	DPRINT1("\n  CancelRequestRoutine\n    Exiting\n");
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

            ExFreePoolWithTag(Request, TAG_ADDRESS_FILE);

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
	PIRP Irp;
	PIO_STACK_LOCATION IrpSp;
	KIRQL OldIrql;
	PADDRESS_FILE AddressFile;
	
	DPRINT1("lwIP TCP Accept Callback\n");
	
	AddressFile = (PADDRESS_FILE)arg;
	Irp = AddressFile->ConnectionContext->PendingIrp;
	if (!Irp)
	{
		DPRINT1("Callback on canceled IRP\n");
		return ERR_ABRT;
	}
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	
	if (AddressFile != IrpSp->FileObject->FsContext)
	{
		DPRINT1("AddressFile Mismatch\n");
		tcp_abort(AddressFile->ConnectionContext->lwip_tcp_pcb);
		AddressFile->ConnectionContext->lwip_tcp_pcb = NULL;
		return ERR_ABRT;
	}
	
	IoAcquireCancelSpinLock(&OldIrql);
	IoSetCancelRoutine(Irp, NULL);
	Irp->Cancel = FALSE;
	IoReleaseCancelSpinLock(OldIrql);
	
	AddressFile->ConnectionContext->lwip_tcp_pcb = newpcb;
	
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
	
	return ERR_OK;
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
	PADDRESS_FILE AddressFile;
	PIRP Irp;
	KIRQL OldIrql;
	
	DPRINT1("lwIP TCP Sent Callback\n");
	
	AddressFile = (PADDRESS_FILE)arg;
	Irp = AddressFile->ConnectionContext->PendingIrp;
	if (!Irp)
	{
		DPRINT1("Callback on canceled IRP\n");
		return ERR_ABRT;
	}
	
	IoAcquireCancelSpinLock(&OldIrql);
	IoSetCancelRoutine(Irp, NULL);
	Irp->Cancel = FALSE;
	IoReleaseCancelSpinLock(OldIrql);
	
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = len;
	IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
	
	return ERR_OK;
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
	PADDRESS_FILE AddressFile;
	PIO_STACK_LOCATION IrpSp;
	PTDI_REQUEST_KERNEL_RECEIVE ReceiveInfo;
	KIRQL OldIrql;
	
	INT CopiedLength;
	INT RemainingDestBytes;
	INT RemainingSrceBytes;
	UCHAR *CurrentDestLocation;
	UCHAR *CurrentSrceLocation;
	
	DPRINT1("lwIP TCP Receive Callback\n");
	
	AddressFile = (PADDRESS_FILE)arg;
	Irp = AddressFile->ConnectionContext->PendingIrp;
	if (!Irp)
	{
		DPRINT1("Callback on canceled IRP\n");
		return ERR_ABRT;
	}
	
	IoAcquireCancelSpinLock(&OldIrql);
	IoSetCancelRoutine(Irp, NULL);
	Irp->Cancel = FALSE;
	IoReleaseCancelSpinLock(OldIrql);
	
	Buffer = (PNDIS_BUFFER)Irp->MdlAddress;
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	ReceiveInfo = (PTDI_REQUEST_KERNEL_RECEIVE)&IrpSp->Parameters;
	
	NdisQueryBuffer(Buffer, &CurrentDestLocation, &RemainingDestBytes);
	
	DPRINT1("\n  PTDI_REQUEST_KERNEL_RECEIVE->ReceiveLength = %d\n  NDIS_BUFFER length = %d\n  pbuf->tot_len = %d\n",
		ReceiveInfo->ReceiveLength,
		RemainingDestBytes,
		p->tot_len);
	
	if (RemainingDestBytes <= p->len)
	{
		RtlCopyMemory(CurrentDestLocation, p->payload, RemainingDestBytes);
		CopiedLength = RemainingDestBytes;
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
					goto RETURN;
				}
			}
			else
			{
				RtlCopyMemory(CurrentDestLocation, CurrentSrceLocation, RemainingDestBytes);
				CopiedLength += RemainingDestBytes;
				
				goto RETURN;
			}
		}
	}
	
RETURN:
	DPRINT1("Receive CopiedLength = %d\n", CopiedLength);

	Irp->IoStatus.Status = STATUS_SUCCESS;
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
lwip_tcp_Connected_callback(
	void *arg,
	struct tcp_pcb *tpcb,
	err_t err)
{
	PADDRESS_FILE AddressFile;
	PIRP Irp;
	KIRQL OldIrql;
	
	DPRINT1("lwIP TCP Connected Callback\n");
	
	AddressFile = (PADDRESS_FILE)arg;
	Irp = AddressFile->ConnectionContext->PendingIrp;
	if (!Irp)
	{
		DPRINT1("Callback on canceled IRP\n");
		return ERR_ABRT;
	}
	
	IoAcquireCancelSpinLock(&OldIrql);
	IoSetCancelRoutine(Irp, NULL);
	Irp->Cancel = FALSE;
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
	PTCP_CONTEXT TcpContext;
    LIST_ENTRY* ListEntry;
    KIRQL OldIrql;
	
//	ULONG *temp;
	
    USHORT Port = 1;
	
/*	temp = (ULONG*)Address;
	DPRINT1("\n TcpIpCreateAddress Input Dump\n  %08x %08x %08x %08x\n",
		temp[3], temp[2],
		temp[1], temp[0]);
		
	temp = IrpSp->FileObject->FsContext;
	if (temp) {
		DPRINT1("\n IrpSp Dump\n  %08x %08x %08x %08x\n  %08x %08x %08x %08x\n",
			temp[7], temp[6], temp[5], temp[4],
			temp[3], temp[2], temp[1], temp[0]);
	}*/

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
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(AddressFile, sizeof(*AddressFile));
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
			
			TcpContext
				= ExAllocatePoolWithTag(NonPagedPool, sizeof(TCP_CONTEXT), TAG_ADDRESS_FILE);
			InitializeListHead(&TcpContext->ListEntry);
			TcpContext->AddressFile = AddressFile;
			TcpContext->Protocol = IPPROTO_TCP;
			TcpContext->PendingIrp = NULL;
			TcpContext->lwip_tcp_pcb = tcp_new();
			AddressFile->ConnectionContext = TcpContext;
			AddressFile->ContextCount = 0;
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

//	ULONG *temp;
	
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	
/*	temp = (ULONG*)Address;
	DPRINT1("\n TcpIpCreateAddress Input Dump\n  %08x %08x %08x %08x\n",
		temp[3], temp[2],
		temp[1], temp[0]);
	if (IrpSp->FileObject && IrpSp->FileObject->FsContext)
	{
		temp = IrpSp->FileObject->FsContext;
		if (temp) {
			DPRINT1("\n IrpSp Dump\n  %08x %08x %08x %08x\n  %08x %08x %08x %08x\n",
				temp[7], temp[6], temp[5], temp[4],
				temp[3], temp[2], temp[1], temp[0]);
		}
	}*/
	
	Context = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Context), TAG_ADDRESS_FILE);
	if (!Context)
	{
		return STATUS_NO_MEMORY;
	}
	
	if (Protocol != IPPROTO_TCP)
	{
		DPRINT1("Creating connection context for non-TCP protocoln");
		return STATUS_INVALID_PARAMETER;
	}
	Context->Protocol = Protocol;
	RtlCopyMemory(&Context->RequestAddress, Address, sizeof(*Address));
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
	LIST_ENTRY *Entry;
	LIST_ENTRY *Temp;
	LIST_ENTRY *Head;
	PTCP_CONTEXT CurrentContext;
	
    KIRQL OldIrql;

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
    if (AddressFile->Protocol == IPPROTO_UDP)
        udp_remove(AddressFile->lwip_udp_pcb);
	else if (AddressFile->Protocol == IPPROTO_TCP && AddressFile->ConnectionContext)
	{
		if (AddressFile->ContextCount != 0)
		{
			Head = &AddressFile->ConnectionContext->ListEntry;
			Entry = Head->Flink;
			while (Entry != Head)
			{
				Temp = Entry->Flink;
				
				CurrentContext = CONTAINING_RECORD(Entry, TCP_CONTEXT, ListEntry);
				RemoveEntryList(Entry);
				ExFreePoolWithTag(CurrentContext, TAG_ADDRESS_FILE);
				AddressFile->ContextCount--;
				Entry = Temp;
			}
			if (AddressFile->ContextCount != 0)
			{
				DPRINT1("Still contexts remaining after deallocation\n");
				return STATUS_UNSUCCESSFUL;
			}
		}
		if (AddressFile->ConnectionContext->lwip_tcp_pcb)
		{
			tcp_close(AddressFile->ConnectionContext->lwip_tcp_pcb);
			AddressFile->ConnectionContext->lwip_tcp_pcb = NULL;
		}
		ExFreePoolWithTag(AddressFile->ConnectionContext, TAG_ADDRESS_FILE);
    }
	else if (AddressFile->Protocol == IPPROTO_RAW)
        raw_remove(AddressFile->lwip_raw_pcb);

    /* Remove from the list and free the structure */
    RemoveEntryList(&AddressFile->ListEntry);
    KeReleaseSpinLock(&AddressListLock, OldIrql);

    RemoveEntityInstance(&AddressFile->Instance);

    ExFreePoolWithTag(AddressFile, TAG_ADDRESS_FILE);

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

    ExFreePoolWithTag(Request, TAG_ADDRESS_FILE);
}

/* implementation in testing */
NTSTATUS
TcpIpConnect(
	_Inout_ PIRP Irp)
{
	PIO_STACK_LOCATION IrpSp;
	ADDRESS_FILE *AddressFile;
	PTDI_REQUEST_KERNEL_CONNECT Parameters;
	PTRANSPORT_ADDRESS RemoteTransportAddress;
	
	struct sockaddr *SocketAddressRemote;
	struct sockaddr_in *SocketAddressInRemote;
	
	err_t lwip_err;
	
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	
	/* Check this is really a connection file */
	if ((ULONG_PTR)IrpSp->FileObject->FsContext2 != TDI_TRANSPORT_ADDRESS_FILE)
	{
		DPRINT1("File object not an address file\n");
		return STATUS_FILE_INVALID;
	}
	
	AddressFile = IrpSp->FileObject->FsContext;
	if (AddressFile->Protocol != IPPROTO_TCP)
	{
		DPRINT1("Received TDI_CONNECT for a non-TCP protocol\n");
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
		AddressFile->ConnectionContext->lwip_tcp_pcb->local_ip,
		AddressFile->ConnectionContext->lwip_tcp_pcb->local_port);
	
	lwip_err = tcp_connect(AddressFile->ConnectionContext->lwip_tcp_pcb,
		(ip_addr_t *)&SocketAddressInRemote->sin_addr.s_addr,
		SocketAddressInRemote->sin_port,
		lwip_tcp_Connected_callback);
//	DPRINT1("lwip error %d\n", lwip_err);
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
//			DPRINT1("lwip ERR_OK\n");
			PrepareIrpForCancel(Irp, CancelRequestRoutine);
			tcp_arg(AddressFile->ConnectionContext->lwip_tcp_pcb, AddressFile);
			break;
		}
		default :
		{
			/* unknown return value */
			DPRINT1("lwip unknown return code\n");
			return STATUS_NOT_IMPLEMENTED;
		}
	}
	
	return STATUS_PENDING;
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
	KIRQL OldIrql;
	
	err_t lwip_err;
	NTSTATUS Status;
	
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

	KeAcquireSpinLock(&AddressFile->RequestLock, &OldIrql);
	
	if (AddressFile->Address.in_addr == 0)
	{
		// should really look through address file list for an interface
		AddressFile->Address.in_addr = 0x0100007f;
	}
	
/*	DPRINT1("\n TDI Address\n  Port: %04x\n  Address: %08x\n",
		AddressFile->Address.sin_port,
		AddressFile->Address.in_addr);*/
	
	Context->AddressFile = AddressFile;
	Context->lwip_tcp_pcb = AddressFile->ConnectionContext->lwip_tcp_pcb;
	InsertTailList(&AddressFile->ConnectionContext->ListEntry,
		&Context->ListEntry);
	AddressFile->ContextCount++;
	
	if (AddressFile->ContextCount <= 1)
	{
		/* Finally calling into lwip to perform socket bind */
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
	
	KeReleaseSpinLock(&AddressFile->RequestLock, OldIrql);

	IrpSp->FileObject->FsContext = AddressFile;
	IrpSp->FileObject->FsContext2 = (PVOID)TDI_TRANSPORT_ADDRESS_FILE;
	
	Status = STATUS_SUCCESS;
LEAVE:
	return Status;
}


/* Implementation in testing */
NTSTATUS
TcpIpDisassociateAddress(
	_Inout_ PIRP Irp)
{
	PIO_STACK_LOCATION IrpSp;
	PTCP_CONTEXT Context;
	PTCP_CONTEXT CurrentContext;
	PADDRESS_FILE AddressFile;
	KIRQL OldIrql;
	LIST_ENTRY *Head;
	LIST_ENTRY *Entry;
	
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	if ((ULONG)IrpSp->FileObject->FsContext2 != TDI_TRANSPORT_ADDRESS_FILE)
	{
		return STATUS_FILE_INVALID;
	}
	
	AddressFile = IrpSp->FileObject->FsContext;
	if (AddressFile->Protocol != IPPROTO_TCP)
	{
		DPRINT1("Received TDI_DISASSOCIATE_ADDRESS for non-TCP protocol\n");
		return STATUS_INVALID_ADDRESS;
	}
	
	Context = AddressFile->ConnectionContext;
	if (!Context)
	{
		DPRINT1("No connection context\n");
		return STATUS_INVALID_PARAMETER;
	}
	
	if (Context->Protocol != IPPROTO_TCP)
	{
		DPRINT1("Address File and Context have mismatching protocols\n");
		return STATUS_INVALID_ADDRESS;
	}
	
	KeAcquireSpinLock(&AddressFile->RequestLock, &OldIrql);
	
	Head = &AddressFile->ConnectionContext->ListEntry;
	Entry = Head->Flink;
	if (Entry == Head)
	{
		DPRINT1("Disassociating from empty context list\n");
		KeReleaseSpinLock(&AddressFile->RequestLock, OldIrql);
		return STATUS_INVALID_ADDRESS;
	}
	if (AddressFile->ContextCount < 1)
	{
		DPRINT1("Invalid ContextCount\n");
		KeReleaseSpinLock(&AddressFile->RequestLock, OldIrql);
		return STATUS_INVALID_ADDRESS;
	}
	CurrentContext = CONTAINING_RECORD(Entry, TCP_CONTEXT, ListEntry);
	RemoveEntryList(Entry);
	ExFreePoolWithTag(CurrentContext, TAG_ADDRESS_FILE);
	AddressFile->ContextCount--;
	
	KeReleaseSpinLock(&AddressFile->RequestLock, OldIrql);
	
	return STATUS_SUCCESS;
}

/* Implementation in testing */
NTSTATUS
TcpIpListen(
	_Inout_ PIRP Irp)
{
	PIO_STACK_LOCATION IrpSp;
	PADDRESS_FILE AddressFile;
	PTCP_CONTEXT ConnectionContext;
	
	struct tcp_pcb *lpcb;
	
	IrpSp = IoGetCurrentIrpStackLocation(Irp);

	/* Check this is really an address file */
	if ((ULONG_PTR)IrpSp->FileObject->FsContext2 != TDI_TRANSPORT_ADDRESS_FILE)
	{
		DPRINT1("Not an address file\n");
		return STATUS_FILE_INVALID;
	}
	
	/* Get address file */
	AddressFile = IrpSp->FileObject->FsContext;
	ConnectionContext = AddressFile->ConnectionContext;
	if (ConnectionContext->Protocol != IPPROTO_TCP)
	{
		DPRINT1("Received TDI_LISTEN for a non-TCP protocol\n");
		return STATUS_INVALID_ADDRESS;
	}
	
	PrepareIrpForCancel(Irp, CancelRequestRoutine);
	if (AddressFile->ContextCount <= 1)
	{
		/* Call down into lwip to initiate a listen */
		lpcb = tcp_listen(ConnectionContext->lwip_tcp_pcb);
		DPRINT1("lwip tcp_listen returned\n");
		if (lpcb == NULL)
		{
			/* tcp_listen returning NULL can mean
				either INVALID_ADDRESS or NO_MEMORY
				if SO_REUSE is enabled in lwip options */
			DPRINT1("lwip tcp_listen error\n");
			return STATUS_INVALID_ADDRESS;
		}
		else
		{
			ConnectionContext->lwip_tcp_pcb = lpcb;
		}
		tcp_accept(ConnectionContext->lwip_tcp_pcb, lwip_tcp_accept_callback);
		tcp_arg(ConnectionContext->lwip_tcp_pcb, AddressFile);
	}
	
	return STATUS_PENDING;
}

NTSTATUS
TcpIpSend(
 _Inout_ PIRP Irp)
{
	PIO_STACK_LOCATION IrpSp;
	PTDI_REQUEST_KERNEL_SEND Request;
	PADDRESS_FILE AddressFile;
	PTCP_CONTEXT Context;
	PVOID Buffer;
	UINT Len;
	
	err_t lwip_err;
	
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	Request = (PTDI_REQUEST_KERNEL_SEND)&IrpSp->Parameters;
	
	if (IrpSp->FileObject->FsContext2 != (PVOID)TDI_TRANSPORT_ADDRESS_FILE)
	{
		DPRINT1("TcpIpSend without a TDI_TRANSPORT_ADDRESS_FILE\n");
		return STATUS_INVALID_PARAMETER;
	}
	AddressFile = IrpSp->FileObject->FsContext;
	
	if (!AddressFile->ConnectionContext)
	{
		DPRINT1("TcpIpSend with no TCP_CONTEXT\n");
		return STATUS_INVALID_PARAMETER;
	}
	Context = AddressFile->ConnectionContext;
	
	if (!Irp->MdlAddress)
	{
		DPRINT1("TcpIpSendEmpty\n");
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
	
	lwip_err = tcp_write(Context->lwip_tcp_pcb, Buffer, Request->SendLength, 0);
	switch (lwip_err)
	{
		case ERR_OK:
			DPRINT1("lwIP ERR_OK\n");
			break;
		case ERR_MEM:
			DPRINT1("lwIP ERR_MEM\n");
			return STATUS_NO_MEMORY;
		case ERR_ARG:
			DPRINT1("lwIP ERR_ARG\n");
			return STATUS_INVALID_PARAMETER;
		case ERR_CONN:
			DPRINT1("lwIP ERR_CONN\n");
			return STATUS_CONNECTION_ACTIVE;
		default:
			DPRINT1("Unknwon lwIP Error: %d\n", lwip_err);
			return STATUS_NOT_IMPLEMENTED;
	}
	
	PrepareIrpForCancel(Irp, CancelRequestRoutine);
	tcp_arg(Context->lwip_tcp_pcb, AddressFile);
	tcp_sent(Context->lwip_tcp_pcb, lwip_tcp_sent_callback);
	
	return STATUS_PENDING;
}

NTSTATUS
TcpIpReceive(
	_Inout_ PIRP Irp)
{
	PIO_STACK_LOCATION IrpSp;
	PADDRESS_FILE AddressFile;
	PTCP_CONTEXT Context;
	
	PTDI_REQUEST_KERNEL_RECEIVE RequestInfo;
	
	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	
	if (IrpSp->FileObject->FsContext2 != (PVOID)TDI_TRANSPORT_ADDRESS_FILE)
	{
		DPRINT1("TcpIpReceive on something that is not an address file\n");
		return STATUS_INVALID_PARAMETER;
	}
	AddressFile = IrpSp->FileObject->FsContext;
	
	if (!AddressFile->ConnectionContext)
	{
		DPRINT1("Receiving on TCP address file with no connection context\n");
		return STATUS_INVALID_ADDRESS;
	}
	Context = AddressFile->ConnectionContext;
	
	if (!Context->lwip_tcp_pcb)
	{
		DPRINT1("Connection context does not contain a lwIP tcp_pcb\n");
		return STATUS_INVALID_ADDRESS;
	}
	
	RequestInfo = (PTDI_REQUEST_KERNEL_RECEIVE)&IrpSp->Parameters;
	DPRINT1("\n  Request Length = %d\n", RequestInfo->ReceiveLength);
	
	PrepareIrpForCancel(Irp, CancelRequestRoutine);
	tcp_recv(Context->lwip_tcp_pcb, lwip_tcp_receive_callback);
	tcp_arg(Context->lwip_tcp_pcb, AddressFile);
	
	return STATUS_PENDING;
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
    Request = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Request), TAG_ADDRESS_FILE);
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
        ExFreePoolWithTag(Request, TAG_ADDRESS_FILE);
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

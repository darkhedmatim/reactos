/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        transport/tcp/event.c
 * PURPOSE:     Transmission Control Protocol
 * PROGRAMMERS: Cameron Gutman (cameron.gutman@reactos.org)
 */

#include "precomp.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/api.h"

#include "rosip.h"

static const char * const tcp_state_str[] = {
  "CLOSED",      
  "LISTEN",      
  "SYN_SENT",    
  "SYN_RCVD",    
  "ESTABLISHED", 
  "FIN_WAIT_1",  
  "FIN_WAIT_2",  
  "CLOSE_WAIT",  
  "CLOSING",     
  "LAST_ACK",    
  "TIME_WAIT"   
};

VOID
FlushAllQueues(PCONNECTION_ENDPOINT Connection, NTSTATUS Status)
{
    PTCP_COMPLETION_ROUTINE Complete;
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;
    
    ReferenceObject(Connection);
    
    DbgPrint("Flushing recv/all with status: 0x%x\n", Status);
    
    while ((Entry = ExInterlockedRemoveHeadList(&Connection->ReceiveRequest, &Connection->Lock))) {
        Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
        
        TI_DbgPrint(DEBUG_TCP,
                    ("Completing Receive request: %x %x\n",
                     Bucket->Request, Status));
        
        Bucket->Status = Status;
        Bucket->Information = 0;
        
        Complete = Bucket->Request.RequestNotifyObject;
        
        Complete(Bucket->Request.RequestContext, Bucket->Status, Bucket->Information);
        
        ExFreePoolWithTag(Bucket, TDI_BUCKET_TAG);
    }
    
    if (Status == STATUS_SUCCESS) Status = STATUS_FILE_CLOSED;
    
    while ((Entry = ExInterlockedRemoveHeadList(&Connection->ListenRequest, &Connection->Lock)))
    {
        Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
        
        Bucket->Status = Status;
        Bucket->Information = 0;
        
        Complete = Bucket->Request.RequestNotifyObject;
        
        Complete(Bucket->Request.RequestContext, Bucket->Status, Bucket->Information);
        
        ExFreePoolWithTag(Bucket, TDI_BUCKET_TAG);
    }
    
    while ((Entry = ExInterlockedRemoveHeadList(&Connection->SendRequest, &Connection->Lock)))
    {
        Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );    
        
        TI_DbgPrint(DEBUG_TCP,
                    ("Completing Send request: %x %x\n",
                     Bucket->Request, Status));
        
        Bucket->Status = Status;
        Bucket->Information = 0;
        
        Complete = Bucket->Request.RequestNotifyObject;
        
        Complete(Bucket->Request.RequestContext, Bucket->Status, Bucket->Information);
        
        ExFreePoolWithTag(Bucket, TDI_BUCKET_TAG);
    }
    
    while ((Entry = ExInterlockedRemoveHeadList(&Connection->ConnectRequest, &Connection->Lock)))
    {
        Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
        
        Bucket->Status = Status;
        Bucket->Information = 0;
        
        Complete = Bucket->Request.RequestNotifyObject;
        
        Complete(Bucket->Request.RequestContext, Bucket->Status, Bucket->Information);
        
        ExFreePoolWithTag(Bucket, TDI_BUCKET_TAG);
    }
    
    DereferenceObject(Connection);
}

VOID
TCPFinEventHandler(void *arg, err_t err)
{
    PCONNECTION_ENDPOINT Connection = arg;
    
    FlushAllQueues(Connection, TCPTranslateError(err));
    
    /* We're already closed so we don't want to call lwip_close */
    Connection->SocketContext = NULL;
}
    
VOID
TCPAcceptEventHandler(void *arg, struct tcp_pcb *newpcb)
{
    PCONNECTION_ENDPOINT Connection = arg;
    PTCP_COMPLETION_ROUTINE Complete;
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;
    PIRP Irp;
    NTSTATUS Status;
    KIRQL OldIrql;
    
    DbgPrint("[IP, TCPAcceptEventHandler] Called\n");
    
    ReferenceObject(Connection);
    
    while ((Entry = ExInterlockedRemoveHeadList(&Connection->ListenRequest, &Connection->Lock)))
    {
        PIO_STACK_LOCATION IrpSp;
        
        Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
        
        Irp = Bucket->Request.RequestContext;
        IrpSp = IoGetCurrentIrpStackLocation( Irp );
        
        TI_DbgPrint(DEBUG_TCP,("[IP, TCPAcceptEventHandler] Getting the socket\n"));
        
        Status = TCPCheckPeerForAccept(newpcb,
                                       (PTDI_REQUEST_KERNEL)&IrpSp->Parameters);
        
        TI_DbgPrint(DEBUG_TCP,("Socket: Status: %x\n", Status));
        
        Bucket->Status = Status;
        Bucket->Information = 0;
        
        DbgPrint("[IP, TCPAcceptEventHandler] Associated with: 0x%x\n",
            Bucket->AssociatedEndpoint->SocketContext);
        
        DbgPrint("[IP, TCPAcceptEventHandler] Completing accept event %x\n", Status);
        
        Complete = Bucket->Request.RequestNotifyObject;
        
        if (Status == STATUS_SUCCESS)
        {
            DbgPrint("[IP, TCPAcceptEventHandler] newpcb->state = %s, newpcb->id = %d\n",
                tcp_state_str[newpcb->state], newpcb->identifier);

            LockObject(Bucket->AssociatedEndpoint, &OldIrql);
            Bucket->AssociatedEndpoint->SocketContext = newpcb;
            DbgPrint("[IP, TCPAcceptEventHandler] LibTCPAccept coming up\n");
            
            LibTCPAccept(newpcb, Bucket->AssociatedEndpoint);

            DbgPrint("[IP, TCPAcceptEventHandler] Trying to unlock Bucket->AssociatedEndpoint\n");
            UnlockObject(Bucket->AssociatedEndpoint, OldIrql);
        }
        
        DbgPrint("[IP, TCPAcceptEventHandler] Done!\n");
        
        Complete(Bucket->Request.RequestContext,
                    Bucket->Status, Bucket->Information);
            
        ExFreePoolWithTag(Bucket, TDI_BUCKET_TAG);
    }
    
    DereferenceObject(Connection);
}

VOID
TCPSendEventHandler(void *arg, u16_t space)
{
    PCONNECTION_ENDPOINT Connection = arg;
    PTCP_COMPLETION_ROUTINE Complete;
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;
    PIRP Irp;
    NTSTATUS Status;
    PMDL Mdl;
    
    DbgPrint("[IP, TCPSendEventHandler] Called\n");
    
    ReferenceObject(Connection);

    while ((Entry = ExInterlockedRemoveHeadList(&Connection->SendRequest, &Connection->Lock)))
    {
        UINT SendLen = 0;
        PVOID SendBuffer = 0;
        
        Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
        
        Irp = Bucket->Request.RequestContext;
        Mdl = Irp->MdlAddress;
        
        TI_DbgPrint(DEBUG_TCP,
                    ("Getting the user buffer from %x\n", Mdl));
        
        NdisQueryBuffer( Mdl, &SendBuffer, &SendLen );
        
        TI_DbgPrint(DEBUG_TCP,
                    ("Writing %d bytes to %x\n", SendLen, SendBuffer));
        
        TI_DbgPrint(DEBUG_TCP, ("Connection: %x\n", Connection));
        TI_DbgPrint
        (DEBUG_TCP,
         ("Connection->SocketContext: %x\n",
          Connection->SocketContext));
        
        Status = TCPTranslateError(LibTCPSend(Connection->SocketContext,
                                              SendBuffer,
                                              SendLen));
        
        TI_DbgPrint(DEBUG_TCP,("TCP Bytes: %d\n", SendLen));
        
        if( Status == STATUS_PENDING )
        {
            ExInterlockedInsertHeadList(&Connection->SendRequest,
                                        &Bucket->Entry,
                                        &Connection->Lock);
            break;
        }
        else
        {
            TI_DbgPrint(DEBUG_TCP,
                        ("Completing Send request: %x %x\n",
                         Bucket->Request, Status));
            
            Bucket->Status = Status;
            Bucket->Information = (Bucket->Status == STATUS_SUCCESS) ? SendLen : 0;
            
            DbgPrint("Completing send req %x\n", Status);
            
            Complete = Bucket->Request.RequestNotifyObject;
            
            Complete(Bucket->Request.RequestContext, Bucket->Status, Bucket->Information);
            
            ExFreePoolWithTag(Bucket, TDI_BUCKET_TAG);
        }
    }
    
    DereferenceObject(Connection);

    DbgPrint("[IP, TCPSendEventHandler] Leaving\n");
}

u32_t
TCPRecvEventHandler(void *arg, struct pbuf *p)
{
    PCONNECTION_ENDPOINT Connection = arg;
    PTCP_COMPLETION_ROUTINE Complete;
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;
    PIRP Irp;
    PMDL Mdl;
    UINT Received = 0;
    UINT RecvLen;
    PUCHAR RecvBuffer;
    
    ASSERT(p);
    
    ReferenceObject(Connection);
    
    DbgPrint("[IP, TCPRecvEventHandler] Called\n");
    
    if ((Entry = ExInterlockedRemoveHeadList(&Connection->ReceiveRequest, &Connection->Lock)))
    {
        Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
        
        Irp = Bucket->Request.RequestContext;
        Mdl = Irp->MdlAddress;
        
        TI_DbgPrint(DEBUG_TCP,
                    ("[IP, TCPRecvEventHandler] Getting the user buffer from %x\n", Mdl));
        
        NdisQueryBuffer( Mdl, &RecvBuffer, &RecvLen );
        
        TI_DbgPrint(DEBUG_TCP,
                    ("[IP, TCPRecvEventHandler] Reading %d bytes to %x\n", RecvLen, RecvBuffer));
        
        TI_DbgPrint(DEBUG_TCP, ("Connection: %x\n", Connection));
        TI_DbgPrint
        (DEBUG_TCP,
         ("[IP, TCPRecvEventHandler] Connection->SocketContext: %x\n",
          Connection->SocketContext));
        TI_DbgPrint(DEBUG_TCP, ("[IP, TCPRecvEventHandler] RecvBuffer: %x\n", RecvBuffer));
        
        RecvLen = MIN(p->tot_len, RecvLen);
        
        for (Received = 0; Received < RecvLen; Received += p->len, p = p->next)
        {
            DbgPrint("[IP, TCPRecvEventHandler] 0x%x: Copying %d bytes to 0x%x from 0x%x\n",
                p, p->len, ((PUCHAR)RecvBuffer) + Received, p->payload);
            
            RtlCopyMemory(RecvBuffer + Received, p->payload, p->len);
        }
        
        TI_DbgPrint(DEBUG_TCP,("TCP Bytes: %d\n", Received));
        
        Bucket->Status = STATUS_SUCCESS;
        Bucket->Information = Received;
        
        Complete = Bucket->Request.RequestNotifyObject;
        
        Complete(Bucket->Request.RequestContext, Bucket->Status, Bucket->Information);
        
        ExFreePoolWithTag(Bucket, TDI_BUCKET_TAG);
    }

    DereferenceObject(Connection);

    DbgPrint("[IP, TCPRecvEventHandler] Leaving\n");
    
    return Received;
}

VOID
TCPConnectEventHandler(void *arg, err_t err)
{
    PCONNECTION_ENDPOINT Connection = arg;
    PTCP_COMPLETION_ROUTINE Complete;
    PTDI_BUCKET Bucket;
    PLIST_ENTRY Entry;
    
    DbgPrint("[IP, TCPConnectEventHandler] Called\n");
    
    ReferenceObject(Connection);
    
    while ((Entry = ExInterlockedRemoveHeadList(&Connection->ConnectRequest, &Connection->Lock)))
    {
        
        Bucket = CONTAINING_RECORD( Entry, TDI_BUCKET, Entry );
        
        Bucket->Status = TCPTranslateError(err);
        Bucket->Information = 0;
        
        DbgPrint("[IP, TCPConnectEventHandler] Completing connection request! (0x%x)\n", err);
        
        Complete = Bucket->Request.RequestNotifyObject;
        
        Complete(Bucket->Request.RequestContext, Bucket->Status, Bucket->Information);
        
        ExFreePoolWithTag(Bucket, TDI_BUCKET_TAG);
    }

    DbgPrint("[IP, TCPConnectEventHandler] Done\n");
    
    DereferenceObject(Connection);
}

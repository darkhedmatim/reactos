/* $Id: reply.c,v 1.7 2001/06/23 19:13:33 phreak Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/reply.c
 * PURPOSE:         Communication mechanism
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/port.h>
#include <internal/dbg.h>
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define TAG_LPC_MESSAGE   TAG('L', 'P', 'C', 'M')

/* FUNCTIONS *****************************************************************/

/**********************************************************************
 * NAME
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
NTSTATUS STDCALL
EiReplyOrRequestPort (IN	PEPORT		Port, 
		      IN	PLPC_MESSAGE	LpcReply, 
		      IN	ULONG		MessageType,
		      IN	PEPORT		Sender)
{
   KIRQL oldIrql;
   PQUEUEDMESSAGE MessageReply;
   
   if (Port == NULL)
     {
       KeBugCheck(0);
     }

   MessageReply = ExAllocatePoolWithTag(NonPagedPool, sizeof(QUEUEDMESSAGE),
					TAG_LPC_MESSAGE);
   MessageReply->Sender = Sender;
   
   if (LpcReply != NULL)
     {
	memcpy(&MessageReply->Message, LpcReply, LpcReply->MessageSize);
     }
   
   MessageReply->Message.Cid.UniqueProcess = PsGetCurrentProcessId();
   MessageReply->Message.Cid.UniqueThread = PsGetCurrentThreadId();
   MessageReply->Message.MessageType = MessageType;
   MessageReply->Message.MessageId = InterlockedIncrement(&EiNextLpcMessageId);
   
   KeAcquireSpinLock(&Port->Lock, &oldIrql);
   EiEnqueueMessagePort(Port, MessageReply);
   KeReleaseSpinLock(&Port->Lock, oldIrql);
   
   return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							EXPORTED
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
NTSTATUS STDCALL
NtReplyPort (IN	HANDLE		PortHandle,
	     IN	PLPC_MESSAGE	LpcReply)
{
   NTSTATUS Status;
   PEPORT Port;
   
   DPRINT("NtReplyPort(PortHandle %x, LpcReply %x)\n", PortHandle, LpcReply);
   
   Status = ObReferenceObjectByHandle(PortHandle,
				      PORT_ALL_ACCESS,   /* AccessRequired */
				      ExPortType,
				      UserMode,
				      (PVOID*)&Port,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("NtReplyPort() = %x\n", Status);
	return(Status);
     }
   
   Status = EiReplyOrRequestPort(Port->OtherPort, 
				 LpcReply, 
				 LPC_REPLY,
				 Port);
   KeReleaseSemaphore( &Port->OtherPort->Semaphore, IO_NO_INCREMENT, 1, FALSE );
   
   ObDereferenceObject(Port);
   
   return(Status);
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtReplyWaitReceivePortEx
 *	
 * DESCRIPTION
 *	Can be used with waitable ports.
 *	Present only in w2k+.
 *	
 * ARGUMENTS
 * 	PortHandle
 * 	PortId
 * 	LpcReply
 * 	LpcMessage
 * 	Timeout
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
NTSTATUS STDCALL
NtReplyWaitReceivePortEx(IN  HANDLE		PortHandle,
			 OUT PULONG		PortId,
			 IN  PLPC_MESSAGE	LpcReply,     
			 OUT PLPC_MESSAGE	LpcMessage,
			 IN  PLARGE_INTEGER	Timeout)
{
   NTSTATUS Status;
   PEPORT Port;
   KIRQL oldIrql;
   PQUEUEDMESSAGE Request;
   BOOLEAN Disconnected;
   LARGE_INTEGER to;
   
   DPRINT("NtReplyWaitReceivePortEx(PortHandle %x, LpcReply %x, "
	  "LpcMessage %x)\n", PortHandle, LpcReply, LpcMessage);
   
   Status = ObReferenceObjectByHandle(PortHandle,
				      PORT_ALL_ACCESS,
				      ExPortType,
				      UserMode,
				      (PVOID*)&Port,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	DPRINT1("NtReplyWaitReceivePortEx() = %x\n", Status);
	return(Status);
     }
   if( Port->State == EPORT_DISCONNECTED )
     {
       // if the port is disconnected, force the timeout to be 0
       // so we don't wait for new messages, because there won't be
       // any, only try to remove any existing messages
       Disconnected = TRUE;
       to.QuadPart = 0;
       Timeout = &to;
     }
   else Disconnected = FALSE;
   
   /*
    * Send the reply, only if port is connected
    */
   if (LpcReply != NULL && !Disconnected)
     {
	Status = EiReplyOrRequestPort(Port->OtherPort, 
				      LpcReply,
				      LPC_REPLY,
				      Port);
	KeReleaseSemaphore( &Port->OtherPort->Semaphore, IO_NO_INCREMENT, 1, FALSE);
	
	if (!NT_SUCCESS(Status))
	  {
	     ObDereferenceObject(Port);
	     DPRINT1("NtReplyWaitReceivePortEx() = %x\n", Status);
	     return(Status);
	  }
     }
   
   /*
    * Want for a message to be received
    */
   Status = KeWaitForSingleObject(&Port->Semaphore,
				  UserRequest,
				  UserMode,
				  FALSE,
				  Timeout);
   if( Status == STATUS_TIMEOUT )
     {
       // if the port is disconnected, and there are no remaining messages,
       // return STATUS_PORT_DISCONNECTED
       ObDereferenceObject( Port );
       return Disconnected ? STATUS_PORT_DISCONNECTED : STATUS_TIMEOUT;
     }
   
   if (!NT_SUCCESS(Status))
     {
       DPRINT1("NtReplyWaitReceivePortEx() = %x\n", Status);
       ObDereferenceObject( Port );
       return(Status);
     }

   /*
    * Dequeue the message
    */
   KeAcquireSpinLock(&Port->Lock, &oldIrql);
   Request = EiDequeueMessagePort(Port);
   
   assert( Request );
   memcpy(LpcMessage, &Request->Message, Request->Message.MessageSize);
   if (Request->Message.MessageType == LPC_CONNECTION_REQUEST)
     {
       EiEnqueueConnectMessagePort(Port, Request);
       KeReleaseSpinLock(&Port->Lock, oldIrql);
     }
   else
     {
       KeReleaseSpinLock(&Port->Lock, oldIrql);
       ExFreePool(Request);
     }
   
   /*
    * Dereference the port
    */
   ObDereferenceObject(Port);
   return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME						EXPORTED
 *	NtReplyWaitReceivePort
 *	
 * DESCRIPTION
 *	Can be used with waitable ports.
 *	
 * ARGUMENTS
 * 	PortHandle
 * 	PortId
 * 	LpcReply
 * 	LpcMessage
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
NTSTATUS STDCALL
NtReplyWaitReceivePort (IN  HANDLE		PortHandle,
			OUT PULONG		PortId,
			IN  PLPC_MESSAGE	LpcReply,     
			OUT PLPC_MESSAGE	LpcMessage)
{
	return NtReplyWaitReceivePortEx (
			PortHandle,
			PortId,
			LpcReply,
			LpcMessage,
			NULL
			);
}

/**********************************************************************
 * NAME
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
NTSTATUS STDCALL
NtReplyWaitReplyPort (HANDLE		PortHandle,
		      PLPC_MESSAGE	ReplyMessage)
{
   UNIMPLEMENTED;
}


/* EOF */

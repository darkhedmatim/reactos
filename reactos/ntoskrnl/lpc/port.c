/* $Id: port.c,v 1.20 2004/10/31 20:27:08 ea Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/port.c
 * PURPOSE:         Communication mechanism
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *
 *	2000-06-04 (ea)
 *		ntoskrnl/nt/port.c moved in ntoskrnl/lpc/port.c
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>


/* GLOBALS *******************************************************************/

POBJECT_TYPE	ExPortType = NULL;
ULONG		LpcpNextMessageId = 0; /* 0 is not a valid ID */
FAST_MUTEX	LpcpLock; /* global internal sync in LPC facility */

static GENERIC_MAPPING ExpPortMapping = {
	STANDARD_RIGHTS_READ,
	STANDARD_RIGHTS_WRITE,
	0,
	PORT_ALL_ACCESS};

/* FUNCTIONS *****************************************************************/


NTSTATUS INIT_FUNCTION
NiInitPort (VOID)
{
   ExPortType = ExAllocatePoolWithTag(NonPagedPool,sizeof(OBJECT_TYPE),TAG_OBJECT_TYPE);
   
   RtlRosInitUnicodeStringFromLiteral(&ExPortType->TypeName,L"Port");
   
   ExPortType->Tag = TAG('L', 'P', 'R', 'T');
   ExPortType->MaxObjects = ULONG_MAX;
   ExPortType->MaxHandles = ULONG_MAX;
   ExPortType->TotalObjects = 0;
   ExPortType->TotalHandles = 0;
   ExPortType->PagedPoolCharge = 0;
   ExPortType->NonpagedPoolCharge = sizeof(EPORT);
   ExPortType->Mapping = &ExpPortMapping;
   ExPortType->Dump = NULL;
   ExPortType->Open = NULL;
   ExPortType->Close = NiClosePort;
   ExPortType->Delete = NiDeletePort;
   ExPortType->Parse = NULL;
   ExPortType->Security = NULL;
   ExPortType->QueryName = NULL;
   ExPortType->OkayToClose = NULL;
   ExPortType->Create = NiCreatePort;
   ExPortType->DuplicationNotify = NULL;

   ObpCreateTypeObject(ExPortType);
   
   LpcpNextMessageId = 0;

   ExInitializeFastMutex (& LpcpLock);
   
   return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							INTERNAL
 *	NiInitializePort/3
 *	
 * DESCRIPTION
 *	Initialize the EPORT object attributes. The Port
 *	object enters the inactive state.
 *
 * ARGUMENTS
 *	Port	Pointer to an EPORT object to initialize.
 *	Type	connect (RQST), or communication port (COMM)
 *	Parent	OPTIONAL connect port a communication port
 *		is created from
 *
 * RETURN VALUE
 *	STATUS_SUCCESS if initialization succedeed. An error code
 *	otherwise.
 */
NTSTATUS STDCALL
NiInitializePort (IN OUT  PEPORT Port,
		  IN      USHORT Type,
		  IN      PEPORT Parent OPTIONAL)
{
  if ((Type != EPORT_TYPE_SERVER_RQST_PORT) &&
      (Type != EPORT_TYPE_SERVER_COMM_PORT) &&
      (Type != EPORT_TYPE_CLIENT_COMM_PORT))
  {
	  return STATUS_INVALID_PARAMETER_2;
  }
  memset (Port, 0, sizeof(EPORT));
  KeInitializeSpinLock (& Port->Lock);
  KeInitializeSemaphore( &Port->Semaphore, 0, LONG_MAX );
  Port->RequestPort = Parent;
  Port->OtherPort = NULL;
  Port->QueueLength = 0;
  Port->ConnectQueueLength = 0;
  Port->Type = Type;
  Port->State = EPORT_INACTIVE;
  InitializeListHead (& Port->QueueListHead);
  InitializeListHead (& Port->ConnectQueueListHead);
 
  return (STATUS_SUCCESS);
}


/* MISCELLANEA SYSTEM SERVICES */


/**********************************************************************
 * NAME							SYSTEM
 *	NtImpersonateClientOfPort/2
 *	
 * DESCRIPTION
 *
 * ARGUMENTS
 *	PortHandle,
 *	ClientMessage
 *
 * RETURN VALUE
 */
NTSTATUS STDCALL
NtImpersonateClientOfPort (HANDLE		PortHandle,
			   PLPC_MESSAGE	ClientMessage)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}

/* EOF */

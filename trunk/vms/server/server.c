/* $Id$
 *
 * server.c - VMS Enviroment Subsystem Server - Initialization
 * 
 * ReactOS Operating System
 * 
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 */
#include "vmssrv.h"

//#define NDEBUG
#include <debug.h>

HANDLE VmsApiPort = NULL;

/**********************************************************************
 * NAME							PRIVATE
 * 	VmsStaticServerThread/1
 */
VOID STDCALL VmsStaticServerThread (PVOID x)
{
	NTSTATUS Status = STATUS_SUCCESS;
	LPC_MAX_MESSAGE Request;
	PPORT_MESSAGE Reply = NULL;
	ULONG MessageType = 0;

	while (TRUE)
	{
		Status = NtReplyWaitReceivePort (VmsApiPort,
						 0,
						 Reply,
						 (PPORT_MESSAGE) & Request);
		if(NT_SUCCESS(Status))
		{
			MessageType = Request.Header.u2.s2.Type;
			DPRINT("VMS: %s received a message (Type=%d)\n",
					__FUNCTION__, MessageType);
			switch(MessageType)
			{
			default:
				continue;
			}
		}else{
			DPRINT("VMS: %s: NtReplyWaitReceivePort failed (Status=%08lx)\n",
					__FUNCTION__, Status);
		}
	}
}

/*=====================================================================
 * 	PUBLIC API
 *===================================================================*/

NTSTATUS STDCALL ServerDllInitialization (ULONG ArgumentCount,
					  LPWSTR *Argument)
{
	return CsrAddStaticServerThread (VmsStaticServerThread);
}

/* EOF */

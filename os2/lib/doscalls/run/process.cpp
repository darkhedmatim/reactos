/* $Id: process.cpp,v 1.4 2002/07/26 00:23:13 robertk Exp $
*/
/*
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS OS/2 sub system
 * FILE:             dll/process.cpp
 * PURPOSE:          Kernelservices for OS/2 apps
 * PROGRAMMER:       Robert K. nonvolatil@yahoo.de
 * REVISION HISTORY:
 *  13-03-2002  Created
 *	25-07-2002	Work to make it compile	
 */


#define INCL_DOSPROCESS
#define INCL_DOSERRORS
#include "ros2.h"


APIRET STDCALL DosSleep(ULONG msec)
{
	NT::NTSTATUS stat;
	NT::TIME Interv;
	Interv.QuadPart= -(10000 * msec);
	stat = NT::NtDelayExecution( TRUE, &Interv );
	return 0;
}


/* $Id: process.cpp,v 1.4 2002/07/26 00:23:13 robertk Exp $ */
/* Terminates the current thread or the current Process.
	Decission is made by action 
	FIXME:	move this code to OS2.EXE */
VOID APIENTRY DosExit(ULONG action, ULONG result)
{
	// decide what to do
	if( action == EXIT_THREAD)
	{
		NT::NtTerminateThread( NULL, result );
	}
	else	// EXIT_PROCESS
	{
		NT::NtTerminateProcess( NULL, result );
	}
}


APIRET STDCALL DosCreateThread(PTID ptid, PFNTHREAD pfn,
                                   ULONG param, ULONG flag, ULONG cbStack)
{
	return ERROR_CALL_NOT_IMPLEMENTED;
}

/* EOF */

/* $Id: nt.c,v 1.12 2004/08/15 16:39:09 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/nt.c
 * PURPOSE:         Initialization of system call interfaces
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID INIT_FUNCTION
NtInit(VOID)
{
   NtInitializeEventImplementation();
   NtInitializeEventPairImplementation();
   NtInitializeMutantImplementation();
   NtInitializeSemaphoreImplementation();
   NtInitializeTimerImplementation();
   NiInitPort();
   NtInitializeProfileImplementation();
}

/* EOF */

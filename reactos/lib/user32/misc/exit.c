/* $Id: exit.c,v 1.1 2002/10/20 14:52:45 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/misc/exit.c
 * PURPOSE:         Shutdown related functions
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 */

#include <windows.h>
//#include <user32.h>

#include <ntdll/csr.h>



WINBOOL STDCALL
ExitWindowsEx(UINT uFlags,
	      DWORD dwReserved)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;

  Request.Type = CSRSS_EXIT_REACTOS;
  Request.Data.ExitReactosRequest.Flags = uFlags;
  Request.Data.ExitReactosRequest.Reserved = dwReserved;

  Status = CsrClientCallServer(&Request,
			       &Reply,
			       sizeof(CSRSS_API_REQUEST),
			       sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return(FALSE);
    }

  return(TRUE);
}


WINBOOL STDCALL
RegisterServicesProcess(DWORD ServicesProcessId)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;

  Request.Type = CSRSS_REGISTER_SERVICES_PROCESS;
  Request.Data.RegisterServicesProcessRequest.ProcessId = ServicesProcessId;

  Status = CsrClientCallServer(&Request,
			       &Reply,
			       sizeof(CSRSS_API_REQUEST),
			       sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return(FALSE);
    }

  return(TRUE);
}

/* EOF */

/* $Id: ioctrl.c,v 1.22 2003/11/28 17:17:44 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/ioctrl.c
 * PURPOSE:         Device IO control
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 *                  Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *                  Filled in ZwDeviceIoControlFile 22/02/99
 *                  Fixed IO method handling 08/03/99
 *                  Added APC support 05/11/99
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/io.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS STDCALL
NtDeviceIoControlFile (IN HANDLE DeviceHandle,
		       IN HANDLE Event OPTIONAL,
		       IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL,
		       IN PVOID UserApcContext OPTIONAL,
		       OUT PIO_STATUS_BLOCK IoStatusBlock,
		       IN ULONG IoControlCode,
		       IN PVOID InputBuffer,
		       IN ULONG InputBufferLength OPTIONAL,
		       OUT PVOID OutputBuffer,
		       IN ULONG OutputBufferLength OPTIONAL)
{
  IO_STATUS_BLOCK SafeIoStatusBlock;
  NTSTATUS Status;
  PFILE_OBJECT FileObject;
  PDEVICE_OBJECT DeviceObject;
  PIRP Irp;
  PIO_STACK_LOCATION StackPtr;
  PKEVENT EventObject;

  DPRINT("NtDeviceIoControlFile(DeviceHandle %x Event %x UserApcRoutine %x "
         "UserApcContext %x IoStatusBlock %x IoControlCode %x "
         "InputBuffer %x InputBufferLength %x OutputBuffer %x "
         "OutputBufferLength %x)\n",
         DeviceHandle,Event,UserApcRoutine,UserApcContext,IoStatusBlock,
         IoControlCode,InputBuffer,InputBufferLength,OutputBuffer,
         OutputBufferLength);

  if (IoStatusBlock == NULL)
    return STATUS_ACCESS_VIOLATION;

  Status = ObReferenceObjectByHandle (DeviceHandle,
				      FILE_READ_DATA | FILE_WRITE_DATA,
				      IoFileObjectType,
				      KernelMode,
				      (PVOID *) &FileObject,
				      NULL);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  if (Event != NULL)
    {
      Status = ObReferenceObjectByHandle (Event,
                                          SYNCHRONIZE,
                                          ExEventObjectType,
                                          UserMode,
                                          (PVOID*)&EventObject,
                                          NULL);
      if (!NT_SUCCESS(Status))
	{
	  ObDereferenceObject (FileObject);
	  return Status;
	}
     }
   else
     {
       EventObject = &FileObject->Event;
       KeResetEvent (EventObject);
     }

  DeviceObject = FileObject->DeviceObject;

  Irp = IoBuildDeviceIoControlRequest (IoControlCode,
				       DeviceObject,
				       InputBuffer,
				       InputBufferLength,
				       OutputBuffer,
				       OutputBufferLength,
				       FALSE,
				       EventObject,
				       &SafeIoStatusBlock);

  /* Trigger FileObject/Event dereferencing */
  Irp->Tail.Overlay.OriginalFileObject = FileObject;

  Irp->Overlay.AsynchronousParameters.UserApcRoutine = UserApcRoutine;
  Irp->Overlay.AsynchronousParameters.UserApcContext = UserApcContext;

  StackPtr = IoGetNextIrpStackLocation(Irp);
  StackPtr->FileObject = FileObject;
  StackPtr->DeviceObject = DeviceObject;
  StackPtr->Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
  StackPtr->Parameters.DeviceIoControl.OutputBufferLength = OutputBufferLength;

  Status = IoCallDriver(DeviceObject,Irp);
  if (Status == STATUS_PENDING && (FileObject->Flags & FO_SYNCHRONOUS_IO))
    {
      BOOLEAN Alertable;

      Alertable = (FileObject->Flags & FO_ALERTABLE_IO) ? TRUE : FALSE;
      Status = KeWaitForSingleObject (EventObject,
				      Executive,
				      UserMode,
				      Alertable,
				      NULL);
      if (Status != STATUS_WAIT_0)
	{
	  /* Wait failed. */
	  return Status;
	}

      Status = SafeIoStatusBlock.Status;
    }

  IoStatusBlock->Status = SafeIoStatusBlock.Status;
  IoStatusBlock->Information = SafeIoStatusBlock.Information;

  return Status;
}

/* EOF */

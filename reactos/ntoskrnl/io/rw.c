/* $Id: rw.c,v 1.47 2003/11/08 16:43:02 ekohl Exp $
 *
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/io/rw.c
 * PURPOSE:        Implements read/write APIs
 * PROGRAMMER:     David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                 30/05/98: Created
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/io.h>
#include <internal/ob.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/


/**********************************************************************
 * NAME							EXPORTED
 *	NtReadFile
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * @implemented
 */
NTSTATUS STDCALL
NtReadFile (IN HANDLE FileHandle,
	    IN HANDLE Event OPTIONAL,
	    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	    IN PVOID ApcContext OPTIONAL,
	    OUT PIO_STATUS_BLOCK IoStatusBlock,
	    OUT PVOID Buffer,
	    IN ULONG Length,
	    IN PLARGE_INTEGER ByteOffset OPTIONAL,
	    IN PULONG Key OPTIONAL)
{
  NTSTATUS Status;
  PFILE_OBJECT FileObject;
  PIRP Irp;
  PIO_STACK_LOCATION StackPtr;
  PKEVENT EventObject = NULL;

  DPRINT("NtReadFile(FileHandle %x Buffer %x Length %x ByteOffset %x, "
	 "IoStatusBlock %x)\n", FileHandle, Buffer, Length, ByteOffset,
	 IoStatusBlock);

  if (IoStatusBlock == NULL)
    return STATUS_ACCESS_VIOLATION;

  Status = ObReferenceObjectByHandle(FileHandle,
				     FILE_READ_DATA,
				     IoFileObjectType,
				     UserMode,
				     (PVOID*)&FileObject,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  if (ByteOffset == NULL)
    {
      ByteOffset = &FileObject->CurrentByteOffset;
    }

  if (Event != NULL)
    {
      Status = ObReferenceObjectByHandle(Event,
					 SYNCHRONIZE,
					 ExEventObjectType,
					 UserMode,
					 (PVOID*)&EventObject,
					 NULL);
      if (!NT_SUCCESS(Status))
	{
	  ObDereferenceObject(FileObject);
	  return(Status);
	}
    }
  else
    {
      EventObject = &FileObject->Event;
      KeResetEvent(EventObject);
    }

  Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
				     FileObject->DeviceObject,
				     Buffer,
				     Length,
				     ByteOffset,
				     EventObject,
				     IoStatusBlock);

  /* Trigger FileObject/Event dereferencing */
  Irp->Tail.Overlay.OriginalFileObject = FileObject;

  Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
  Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;

  StackPtr = IoGetNextIrpStackLocation(Irp);
  StackPtr->FileObject = FileObject;
  if (Key != NULL)
    {
      StackPtr->Parameters.Read.Key = *Key;
    }
  else
    {
      StackPtr->Parameters.Read.Key = 0;
    }

  Status = IoCallDriver(FileObject->DeviceObject, Irp);
  if (Event == NULL && Status == STATUS_PENDING && (FileObject->Flags & FO_SYNCHRONOUS_IO))
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
	  return(Status);
	}

      Status = IoStatusBlock->Status;
    }

  return Status;
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtWriteFile
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * @implemented
 */
NTSTATUS STDCALL
NtWriteFile (IN HANDLE FileHandle,
	     IN HANDLE EventHandle OPTIONAL,
	     IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	     IN PVOID ApcContext OPTIONAL,
	     OUT PIO_STATUS_BLOCK IoStatusBlock,
	     IN PVOID Buffer,
	     IN ULONG Length,
	     IN PLARGE_INTEGER ByteOffset OPTIONAL,
	     IN PULONG Key OPTIONAL)
{
  NTSTATUS Status;
  PFILE_OBJECT FileObject;
  PIRP Irp;
  PIO_STACK_LOCATION StackPtr;
  PKEVENT EventObject = NULL;

  DPRINT("NtWriteFile(FileHandle %x Buffer %x Length %x ByteOffset %x, "
	 "IoStatusBlock %x)\n", FileHandle, Buffer, Length, ByteOffset,
	 IoStatusBlock);

  if (IoStatusBlock == NULL)
    return STATUS_ACCESS_VIOLATION;

  Status = ObReferenceObjectByHandle(FileHandle,
				     FILE_READ_DATA,
				     IoFileObjectType,
				     UserMode,
				     (PVOID*)&FileObject,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  if (ByteOffset == NULL)
    {
      ByteOffset = &FileObject->CurrentByteOffset;
    }

  if (EventHandle != NULL)
    {
      Status = ObReferenceObjectByHandle(EventHandle,
					 SYNCHRONIZE,
					 ExEventObjectType,
					 UserMode,
					 (PVOID*)&EventObject,
					 NULL);
      if (!NT_SUCCESS(Status))
	{
	  ObDereferenceObject(FileObject);
	  return(Status);
	}
    }
  else
    {
      EventObject = &FileObject->Event;
      KeResetEvent(EventObject);
    }

  Irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE,
				     FileObject->DeviceObject,
				     Buffer,
				     Length,
				     ByteOffset,
				     EventObject,
				     IoStatusBlock);

   /* Trigger FileObject/Event dereferencing */
   Irp->Tail.Overlay.OriginalFileObject = FileObject;

  Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
  Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;
  
  StackPtr = IoGetNextIrpStackLocation(Irp);
  StackPtr->FileObject = FileObject;
  if (Key != NULL)
    {
      StackPtr->Parameters.Write.Key = *Key;
    }
  else
    {
      StackPtr->Parameters.Write.Key = 0;
    }

  Status = IoCallDriver(FileObject->DeviceObject, Irp);
  if (Event == NULL && Status == STATUS_PENDING && (FileObject->Flags & FO_SYNCHRONOUS_IO))
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
	  return(Status);
	}

      Status = IoStatusBlock->Status;
    }

  return Status;
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtReadFileScatter
 *	
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 */
NTSTATUS
STDCALL
NtReadFileScatter (
	IN	HANDLE			FileHandle,
	IN	HANDLE			Event			OPTIONAL,
	IN	PIO_APC_ROUTINE		UserApcRoutine		OPTIONAL,
	IN	PVOID			UserApcContext		OPTIONAL,
	OUT	PIO_STATUS_BLOCK	UserIoStatusBlock,
	IN	FILE_SEGMENT_ELEMENT	BufferDescription [],
	IN	ULONG			BufferLength,
	IN	PLARGE_INTEGER		ByteOffset,
	IN	PULONG			Key			OPTIONAL
	)
{
	UNIMPLEMENTED;
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtWriteFileGather
 *	
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 */
NTSTATUS
STDCALL
NtWriteFileGather (
	IN	HANDLE			FileHandle,
	IN	HANDLE			Event OPTIONAL,
	IN	PIO_APC_ROUTINE		ApcRoutine		OPTIONAL,
	IN	PVOID			ApcContext		OPTIONAL,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	FILE_SEGMENT_ELEMENT	BufferDescription [],
	IN	ULONG			BufferLength,
	IN	PLARGE_INTEGER		ByteOffset,
	IN	PULONG			Key			OPTIONAL
	)
{
	UNIMPLEMENTED;
}


/* EOF */

/*
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/io/rw.c
 * PURPOSE:        Implements read/write APIs
 * PROGRAMMER:     David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                 30/05/98: Created
 */

/* INCLUDES ****************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/io.h>
#include <internal/string.h>
#include <internal/ob.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

NTSTATUS NtReadFile(HANDLE FileHandle,
                    HANDLE EventHandle,
		    PIO_APC_ROUTINE ApcRoutine,
		    PVOID ApcContext,
		    PIO_STATUS_BLOCK IoStatusBlock,
		    PVOID Buffer,
		    ULONG Length,
		    PLARGE_INTEGER ByteOffset,
		    PULONG Key)
{
   return(ZwReadFile(FileHandle,
		     EventHandle,
		     ApcRoutine,
		     ApcContext,
		     IoStatusBlock,
		     Buffer,
		     Length,
		     ByteOffset,
		     Key));
}

NTSTATUS ZwReadFile(HANDLE FileHandle,
                    HANDLE EventHandle,
		    PIO_APC_ROUTINE ApcRoutine,
		    PVOID ApcContext,
		    PIO_STATUS_BLOCK IoStatusBlock,
		    PVOID Buffer,
		    ULONG Length,
		    PLARGE_INTEGER ByteOffset,
		    PULONG Key)
{
   NTSTATUS Status;
   COMMON_BODY_HEADER* hdr = ObGetObjectByHandle(FileHandle);
   PFILE_OBJECT FileObject = (PFILE_OBJECT)hdr;
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   KEVENT Event;
   
   DPRINT("ZwReadFile(FileHandle %x Buffer %x Length %x ByteOffset %x, "
	  "IoStatusBlock %x)\n",
	  FileHandle,Buffer,Length,ByteOffset,IoStatusBlock);
   
   Status = ObReferenceObjectByHandle(FileHandle,
				      FILE_READ_DATA,
				      NULL,
				      UserMode,
				      &FileObject,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   if (ByteOffset==NULL)
     {
	ByteOffset = &(FileObject->CurrentByteOffset);
     }
   
   KeInitializeEvent(&Event,NotificationEvent,FALSE);
   DPRINT("FileObject %x\n",FileObject);
   Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
				      FileObject->DeviceObject,
				      Buffer,
				      Length,
				      ByteOffset,
				      &Event,
				      IoStatusBlock);

   StackPtr = IoGetNextIrpStackLocation(Irp);
   StackPtr->FileObject = FileObject;
   StackPtr->Parameters.Read.Length = Length;
   if (ByteOffset!=NULL)
   {
        StackPtr->Parameters.Read.ByteOffset.LowPart = ByteOffset->LowPart;
        StackPtr->Parameters.Read.ByteOffset.HighPart = ByteOffset->HighPart;
   }
   else
   {
        StackPtr->Parameters.Read.ByteOffset.LowPart = 0;
        StackPtr->Parameters.Read.ByteOffset.HighPart = 0;
   }
   if (Key!=NULL)
   {
         StackPtr->Parameters.Read.Key = *Key;
   }
   else
   {
        StackPtr->Parameters.Read.Key = 0;
   }
   
   DPRINT("FileObject->DeviceObject %x\n",FileObject->DeviceObject);
   Status = IoCallDriver(FileObject->DeviceObject,Irp);
   if (NT_SUCCESS(Status))
     {
       KeWaitForSingleObject(&Event,Executive,KernelMode,FALSE,NULL);
       Status = Irp->IoStatus.Status;
       if (NT_SUCCESS(Status))
         {
           if (FileObject->DeviceObject->Flags&DO_BUFFERED_IO)
             {
               memcpy(Buffer,Irp->AssociatedIrp.SystemBuffer,Length);
             }
         }
     }
   return(Status);
}

NTSTATUS NtWriteFile(HANDLE FileHandle,
		     HANDLE EventHandle,
		     PIO_APC_ROUTINE ApcRoutine,
		     PVOID ApcContext,
		     PIO_STATUS_BLOCK IoStatusBlock,
		     PVOID Buffer,
		     ULONG Length,
		     PLARGE_INTEGER ByteOffset,
		     PULONG Key)
{
   return(ZwWriteFile(FileHandle,
		      EventHandle,
		      ApcRoutine,
		      ApcContext,
		      IoStatusBlock,
		      Buffer,
		      Length,
		      ByteOffset,
		      Key));
}

NTSTATUS ZwWriteFile(HANDLE FileHandle,
		     HANDLE EventHandle,
		     PIO_APC_ROUTINE ApcRoutine,
		     PVOID ApcContext,
		     PIO_STATUS_BLOCK IoStatusBlock,
		     PVOID Buffer,
		     ULONG Length,
		     PLARGE_INTEGER ByteOffset,
		     PULONG Key)
{
   NTSTATUS Status;
   COMMON_BODY_HEADER* hdr = ObGetObjectByHandle(FileHandle);
   PFILE_OBJECT FileObject = (PFILE_OBJECT)hdr;
   PIRP Irp;
   PIO_STACK_LOCATION StackPtr;
   KEVENT Event;
   
   if (hdr==NULL)
     {
	return(STATUS_INVALID_HANDLE);
     }
   
   KeInitializeEvent(&Event,NotificationEvent,FALSE);
   Irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE,
				      FileObject->DeviceObject,
				      Buffer,
				      Length,
				      ByteOffset,
				      &Event,
				      IoStatusBlock);
   DPRINT("FileObject->DeviceObject %x\n",FileObject->DeviceObject);
   Status = IoCallDriver(FileObject->DeviceObject,Irp);
   if (Status==STATUS_PENDING && (FileObject->Flags & FO_SYNCHRONOUS_IO))
     {
	KeWaitForSingleObject(&Event,Executive,KernelMode,FALSE,NULL);
        Status = Irp->IoStatus.Status;
     }
   return(Status);
}

NTSTATUS STDCALL NtReadFileScatter(IN HANDLE FileHandle, 
				   IN HANDLE Event OPTIONAL, 
				   IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL, 
				   IN  PVOID UserApcContext OPTIONAL, 
				   OUT PIO_STATUS_BLOCK UserIoStatusBlock, 
				   IN FILE_SEGMENT_ELEMENT BufferDescription[], 
				   IN ULONG BufferLength, 
				   IN PLARGE_INTEGER ByteOffset, 
				   IN PULONG Key OPTIONAL)
{
   return(ZwReadFileScatter(FileHandle,
			    Event,
			    UserApcRoutine,
			    UserApcContext,
			    UserIoStatusBlock,
			    BufferDescription,
			    BufferLength,
			    ByteOffset,
			    Key));
}

NTSTATUS STDCALL ZwReadFileScatter(IN HANDLE FileHandle, 
				   IN HANDLE Event OPTIONAL, 
				   IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL, 
				   IN  PVOID UserApcContext OPTIONAL, 
				   OUT PIO_STATUS_BLOCK UserIoStatusBlock, 
				   IN FILE_SEGMENT_ELEMENT BufferDescription[],
				   IN ULONG BufferLength, 
				   IN PLARGE_INTEGER ByteOffset, 
				   IN PULONG Key OPTIONAL)
{
   UNIMPLEMENTED;
}


NTSTATUS STDCALL NtWriteFileGather(IN HANDLE FileHandle, 
				   IN HANDLE Event OPTIONAL, 
				   IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, 
				   IN PVOID ApcContext OPTIONAL, 
				   OUT PIO_STATUS_BLOCK IoStatusBlock,
				   IN FILE_SEGMENT_ELEMENT BufferDescription[],
				   IN ULONG BufferLength, 
				   IN PLARGE_INTEGER ByteOffset, 
				   IN PULONG Key OPTIONAL)
{
   return(ZwWriteFileGather(FileHandle,
			    Event,
			    ApcRoutine,
			    ApcContext,
			    IoStatusBlock,
			    BufferDescription,
			    BufferLength,
			    ByteOffset,
			    Key));
}

NTSTATUS STDCALL ZwWriteFileGather(IN HANDLE FileHandle, 
				   IN HANDLE Event OPTIONAL, 
				   IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, 
				   IN PVOID ApcContext OPTIONAL, 
				   OUT PIO_STATUS_BLOCK IoStatusBlock,
				   IN FILE_SEGMENT_ELEMENT BufferDescription[],
				   IN ULONG BufferLength, 
				   IN PLARGE_INTEGER ByteOffset, 
				   IN PULONG Key OPTIONAL)
{
   UNIMPLEMENTED;
}

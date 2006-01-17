/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/file.c
 * PURPOSE:         I/O File Object & NT File Handle Access/Managment of Files.
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

extern GENERIC_MAPPING IopFileMapping;

NTSTATUS
STDCALL
SeSetWorldSecurityDescriptor(SECURITY_INFORMATION SecurityInformation,
                             PSECURITY_DESCRIPTOR SecurityDescriptor,
                             PULONG BufferLength);

/* INTERNAL FUNCTIONS ********************************************************/

/*
 * NAME       INTERNAL
 *  IopCreateFile
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
IopCreateFile(PVOID ObjectBody,
              PVOID Parent,
              PWSTR RemainingPath,
              POBJECT_CREATE_INFORMATION ObjectCreateInfo)
{
  PDEVICE_OBJECT DeviceObject;
  PFILE_OBJECT FileObject = (PFILE_OBJECT) ObjectBody;
  POBJECT_TYPE ParentObjectType;
  NTSTATUS Status;

  DPRINT("IopCreateFile(ObjectBody 0x%p, Parent 0x%p, RemainingPath %S)\n",
         ObjectBody,
         Parent,
         RemainingPath);

  if (NULL == Parent)
    {
      /* This is probably an attempt to create a meta fileobject (eg. for FAT)
         for the cache manager, so return STATUS_SUCCESS */
      DPRINT("Parent object was NULL\n");
      return(STATUS_SUCCESS);
    }

  ParentObjectType = BODY_TO_HEADER(Parent)->Type;

  if (ParentObjectType != IoDeviceObjectType &&
      ParentObjectType != IoFileObjectType)
    {
      DPRINT("Parent [%wZ] is a %S which is neither a file type nor a device type ; remaining path = %S\n",
             &HEADER_TO_OBJECT_NAME(BODY_TO_HEADER(Parent))->Name,
             BODY_TO_HEADER(Parent)->Type->Name.Buffer,
             RemainingPath);
      return(STATUS_UNSUCCESSFUL);
    }

  Status = ObReferenceObjectByPointer(Parent,
                                      STANDARD_RIGHTS_REQUIRED,
                                      ParentObjectType,
                                      UserMode);
  if (!NT_SUCCESS(Status))
    {
      CPRINT("Failed to reference parent object 0x%p\n", Parent);
      return(Status);
    }

  if (ParentObjectType == IoDeviceObjectType)
    {
      /* Parent is a devce object */
      DeviceObject = IoGetAttachedDevice((PDEVICE_OBJECT)Parent);
      DPRINT("DeviceObject 0x%p\n", DeviceObject);

      if (RemainingPath == NULL)
        {
          FileObject->Flags = FileObject->Flags | FO_DIRECT_DEVICE_OPEN;
          FileObject->FileName.Buffer = 0;
          FileObject->FileName.Length = FileObject->FileName.MaximumLength = 0;
        }
      else
        {
          if ((DeviceObject->DeviceType != FILE_DEVICE_FILE_SYSTEM)
              && (DeviceObject->DeviceType != FILE_DEVICE_DISK)
              && (DeviceObject->DeviceType != FILE_DEVICE_CD_ROM)
              && (DeviceObject->DeviceType != FILE_DEVICE_TAPE)
              && (DeviceObject->DeviceType != FILE_DEVICE_NETWORK)
              && (DeviceObject->DeviceType != FILE_DEVICE_NAMED_PIPE)
              && (DeviceObject->DeviceType != FILE_DEVICE_MAILSLOT))
            {
              CPRINT("Device was wrong type\n");
              return(STATUS_UNSUCCESSFUL);
            }

          if (DeviceObject->DeviceType != FILE_DEVICE_NETWORK
              && (DeviceObject->DeviceType != FILE_DEVICE_NAMED_PIPE)
              && (DeviceObject->DeviceType != FILE_DEVICE_MAILSLOT))
            {
              if (!(DeviceObject->Vpb->Flags & VPB_MOUNTED))
                {
                  DPRINT("Mount the logical volume\n");
                  Status = IoMountVolume(DeviceObject, FALSE);
                  DPRINT("Status %x\n", Status);
                  if (!NT_SUCCESS(Status))
                    {
                      CPRINT("Failed to mount logical volume (Status %x)\n", Status);
                      return(Status);
                    }
                }
              DeviceObject = DeviceObject->Vpb->DeviceObject;
              DPRINT("FsDeviceObject %lx\n", DeviceObject);
            }
          RtlCreateUnicodeString(&FileObject->FileName, RemainingPath);
        }
    }
  else
    {
      /* Parent is a file object */
      if (RemainingPath == NULL)
        {
          CPRINT("Device is unnamed\n");
          return STATUS_UNSUCCESSFUL;
        }

      DeviceObject = ((PFILE_OBJECT)Parent)->DeviceObject;
      DPRINT("DeviceObject 0x%p\n", DeviceObject);

      FileObject->RelatedFileObject = (PFILE_OBJECT)Parent;

      RtlCreateUnicodeString(&FileObject->FileName, RemainingPath);
    }

  DPRINT("FileObject->FileName %wZ\n",
         &FileObject->FileName);
  FileObject->DeviceObject = DeviceObject;
  DPRINT("FileObject 0x%p DeviceObject 0x%p\n",
         FileObject,
         DeviceObject);
  FileObject->Vpb = DeviceObject->Vpb;
  FileObject->Type = IO_TYPE_FILE;

  return(STATUS_SUCCESS);
}

VOID
STDCALL
IopDeleteFile(PVOID ObjectBody)
{
    PFILE_OBJECT FileObject = (PFILE_OBJECT)ObjectBody;
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    NTSTATUS Status;
    KEVENT Event;
    PDEVICE_OBJECT DeviceObject;

    DPRINT("IopDeleteFile()\n");

    if (FileObject->DeviceObject)
    {
        /* Check if this is a direct open or not */
        if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
        {
            DeviceObject = IoGetAttachedDevice(FileObject->DeviceObject);
        }
        else
        {
            DeviceObject = IoGetRelatedDeviceObject(FileObject);
        }

        /* Clear and set up Events */
        KeClearEvent(&FileObject->Event);
        KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

        /* Allocate an IRP */
        Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

        /* Set it up */
        Irp->UserEvent = &Event;
        Irp->UserIosb = &Irp->IoStatus;
        Irp->Tail.Overlay.Thread = PsGetCurrentThread();
        Irp->Tail.Overlay.OriginalFileObject = FileObject;
        Irp->Flags = IRP_CLOSE_OPERATION | IRP_SYNCHRONOUS_API;

        /* Set up Stack Pointer Data */
        StackPtr = IoGetNextIrpStackLocation(Irp);
        StackPtr->MajorFunction = IRP_MJ_CLOSE;
        StackPtr->DeviceObject = DeviceObject;
        StackPtr->FileObject = FileObject;

        /* Call the FS Driver */
        Status = IoCallDriver(DeviceObject, Irp);

        /* Wait for completion */
        if (Status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        }
        IoFreeIrp(Irp);

    }

    /* Clear the file name */
    if (FileObject->FileName.Buffer)
    {
       ExFreePool(FileObject->FileName.Buffer);
       FileObject->FileName.Buffer = NULL;
    }

    /* Free the completion context */
    if (FileObject->CompletionContext)
    {
       ObDereferenceObject(FileObject->CompletionContext->Port);
       ExFreePool(FileObject->CompletionContext);
    }
}

NTSTATUS
STDCALL
IopSecurityFile(PVOID ObjectBody,
                SECURITY_OPERATION_CODE OperationCode,
                SECURITY_INFORMATION SecurityInformation,
                PSECURITY_DESCRIPTOR SecurityDescriptor,
                PULONG BufferLength,
                PSECURITY_DESCRIPTOR *OldSecurityDescriptor,    
                POOL_TYPE PoolType,
                PGENERIC_MAPPING GenericMapping)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION StackPtr;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    ULONG MajorFunction;
    PIRP Irp;
    BOOLEAN LocalEvent = FALSE;
    KEVENT Event;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("IopSecurityFile() called\n");

    FileObject = (PFILE_OBJECT)ObjectBody;

    if (OperationCode == QuerySecurityDescriptor)
    {
        MajorFunction = IRP_MJ_QUERY_SECURITY;
        DPRINT("Query security descriptor\n");
    }
    else if (OperationCode == DeleteSecurityDescriptor)
    {
        DPRINT("Delete\n");
        return STATUS_SUCCESS;
    }
    else if (OperationCode == AssignSecurityDescriptor)
    {
        /* If this is a direct open, we can assign it */
        if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
        {
            /* Get the Device Object */
            DPRINT("here\n");
            DeviceObject = IoGetAttachedDevice(FileObject->DeviceObject);

            /* Assign the Security Descriptor */
            DeviceObject->SecurityDescriptor = SecurityDescriptor;
        }
        return STATUS_SUCCESS;
    }
    else
    {
        MajorFunction = IRP_MJ_SET_SECURITY;
        DPRINT("Set security descriptor\n");

        /* If this is a direct open, we can set it */
        if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
        {
            DPRINT1("Set SD unimplemented for Devices\n");
            return STATUS_SUCCESS;
        }
    }

    /* Get the Device Object */
    DPRINT1("FileObject: %p\n", FileObject);
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Use File Object event */
        KeClearEvent(&FileObject->Event);
    }
    else
    {
        /* Use local event */
        KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
        LocalEvent = TRUE;
    }

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

    /* Set the IRP */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->RequestorMode = ExGetPreviousMode();
    Irp->UserIosb = &IoStatusBlock;
    Irp->UserEvent = (LocalEvent) ? &Event : NULL;
    Irp->Flags = (LocalEvent) ? IRP_SYNCHRONOUS_API : 0;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    /* Set Stack Parameters */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = MajorFunction;
    StackPtr->FileObject = FileObject;

    /* Set Parameters */
    if (OperationCode == QuerySecurityDescriptor)
    {
        StackPtr->Parameters.QuerySecurity.SecurityInformation = SecurityInformation;
        StackPtr->Parameters.QuerySecurity.Length = *BufferLength;
        Irp->UserBuffer = SecurityDescriptor;
    }
    else
    {
        StackPtr->Parameters.SetSecurity.SecurityInformation = SecurityInformation;
        StackPtr->Parameters.SetSecurity.SecurityDescriptor = SecurityDescriptor;
    }

    ObReferenceObject(FileObject);

    /* Call the Driver */
    Status = IoCallDriver(FileObject->DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        if (LocalEvent)
        {
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  KernelMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = IoStatusBlock.Status;
        }
        else
        {
            KeWaitForSingleObject(&FileObject->Event,
                                  Executive,
                                  KernelMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = FileObject->FinalStatus;
        }
    }

    /* This Driver doesn't implement Security, so try to give it a default */
    if (Status == STATUS_INVALID_DEVICE_REQUEST)
    {
        if (OperationCode == QuerySecurityDescriptor)
        {
            /* Set a World Security Descriptor */
            Status = SeSetWorldSecurityDescriptor(SecurityInformation,
                                                  SecurityDescriptor,
                                                  BufferLength);
        }
        else
        {
            /* It wasn't a query, so just fake success */
            Status = STATUS_SUCCESS;
        }
    }
    else if (OperationCode == QuerySecurityDescriptor)
    {
        /* Return length */
        *BufferLength = IoStatusBlock.Information;
    }

    /* Return Status */
    return Status;
}

NTSTATUS
STDCALL
IopQueryNameFile(PVOID ObjectBody,
                 POBJECT_NAME_INFORMATION ObjectNameInfo,
                 ULONG Length,
                 PULONG ReturnLength)
{
    POBJECT_NAME_INFORMATION LocalInfo;
    PFILE_OBJECT FileObject;
    ULONG LocalReturnLength;
    NTSTATUS Status;

    DPRINT1("IopQueryNameFile() called\n");

    FileObject = (PFILE_OBJECT)ObjectBody;

    /* Allocate Buffer */
    LocalInfo = ExAllocatePool(PagedPool,
                               sizeof(OBJECT_NAME_INFORMATION) +
                               MAX_PATH * sizeof(WCHAR));
    if (LocalInfo == NULL) return STATUS_INSUFFICIENT_RESOURCES;

    /* Query the name */
    Status = ObQueryNameString(FileObject->DeviceObject,
                               LocalInfo,
                               MAX_PATH * sizeof(WCHAR),
                               &LocalReturnLength);
    if (!NT_SUCCESS (Status))
    {
        ExFreePool (LocalInfo);
        return Status;
    }
    DPRINT ("Device path: %wZ\n", &LocalInfo->Name);
    
    /* Write Device Path */
    Status = RtlAppendUnicodeStringToString(&ObjectNameInfo->Name,
                                            &(LocalInfo)->Name);

    /* Query the File name */
    Status = IoQueryFileInformation(FileObject,
                                    FileNameInformation,
                                    LocalReturnLength,
                                    LocalInfo,
                                    NULL);
    if (Status != STATUS_SUCCESS)
    {
        ExFreePool(LocalInfo);
        return Status;
    }

    /* Write the Name */
    Status = RtlAppendUnicodeToString(&ObjectNameInfo->Name,
                                      ((PFILE_NAME_INFORMATION)LocalInfo)->FileName);
    DPRINT ("Total path: %wZ\n", &ObjectNameInfo->Name);

    /* Free buffer and return */
    ExFreePool(LocalInfo);
    return Status;
}

VOID
STDCALL
IopCloseFile(PVOID ObjectBody,
             ULONG HandleCount)
{
    PFILE_OBJECT FileObject = (PFILE_OBJECT)ObjectBody;
    KEVENT Event;
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject;

    DPRINT("IopCloseFile()\n");

    if (HandleCount > 1 || FileObject->DeviceObject == NULL) return;

    /* Check if this is a direct open or not */
    if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
    {
        DeviceObject = IoGetAttachedDevice(FileObject->DeviceObject);
    }
    else
    {
        DeviceObject = IoGetRelatedDeviceObject(FileObject);
    }

    /* Clear and set up Events */
    KeClearEvent(&FileObject->Event);
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    /* Allocate an IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

    /* Set it up */
    Irp->UserEvent = &Event;
    Irp->UserIosb = &Irp->IoStatus;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Flags = IRP_CLOSE_OPERATION | IRP_SYNCHRONOUS_API;

    /* Set up Stack Pointer Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_CLEANUP;
    StackPtr->FileObject = FileObject;

    /* Call the FS Driver */
    Status = IoCallDriver(DeviceObject, Irp);

    /* Wait for completion */
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
    }
    IoFreeIrp(Irp);
}

NTSTATUS 
STDCALL
IopDeviceFsIoControl(IN HANDLE DeviceHandle,
                     IN HANDLE Event OPTIONAL,
                     IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL,
                     IN PVOID UserApcContext OPTIONAL,
                     OUT PIO_STATUS_BLOCK IoStatusBlock,
                     IN ULONG IoControlCode,
                     IN PVOID InputBuffer,
                     IN ULONG InputBufferLength OPTIONAL,
                     OUT PVOID OutputBuffer,
                     IN ULONG OutputBufferLength OPTIONAL,
                     BOOLEAN IsDevIoCtl)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    PKEVENT EventObject = NULL;
    BOOLEAN LocalEvent = FALSE;
    ULONG AccessType;
    OBJECT_HANDLE_INFORMATION HandleInformation;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();

    DPRINT("IopDeviceFsIoControl(DeviceHandle 0x%p Event 0x%p UserApcRoutine 0x%p "
           "UserApcContext 0x%p IoStatusBlock 0x%p IoControlCode %x "
           "InputBuffer 0x%p InputBufferLength %x OutputBuffer 0x%p "
           "OutputBufferLength 0x%p)\n",
           DeviceHandle,Event,UserApcRoutine,UserApcContext,IoStatusBlock,
           IoControlCode,InputBuffer,InputBufferLength,OutputBuffer,
           OutputBufferLength);

    AccessType = IO_METHOD_FROM_CTL_CODE(IoControlCode);

    if (PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWrite(IoStatusBlock,
                          sizeof(IO_STATUS_BLOCK),
                          sizeof(ULONG));

            /* probe the input and output buffers if needed */
            if (AccessType == METHOD_BUFFERED)
            {
                if (OutputBuffer != NULL)
                {
                    ProbeForWrite(OutputBuffer,
                                  OutputBufferLength,
                                  1);
                }
                else
                {
                    /* make sure the caller can't fake this as we depend on this */
                    OutputBufferLength = 0;
                }
            }

            if (AccessType != METHOD_NEITHER)
            {
                if (InputBuffer != NULL)
                {
                    ProbeForRead(InputBuffer,
                                 InputBufferLength,
                                 1);
                }
                else
                {
                    /* make sure the caller can't fake this as we depend on this */
                    InputBufferLength = 0;
                }
            }
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if (!NT_SUCCESS(Status))
        {
            DPRINT("Probing the buffers failed!\n");
            return Status;
        }
    }

    /* Don't check for access rights right now, KernelMode can do anything */
    Status = ObReferenceObjectByHandle(DeviceHandle,
                                       0,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID *) &FileObject,
                                       &HandleInformation);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check for sufficient access rights */
    if (PreviousMode != KernelMode)
    {
        ACCESS_MASK DesiredAccess = (ACCESS_MASK)((IoControlCode >> 14) & 3);
        if (DesiredAccess != FILE_ANY_ACCESS &&
            !RtlAreAllAccessesGranted(HandleInformation.GrantedAccess,
                                      (ACCESS_MASK)((IoControlCode >> 14) & 3)))
        {
            DPRINT1("Insufficient access rights! Granted: 0x%x Desired: 0x%x\n",
                    HandleInformation.GrantedAccess, (ACCESS_MASK)((IoControlCode >> 14) & 3));
            ObDereferenceObject (FileObject);
            return STATUS_ACCESS_DENIED;
        }
    }

    /* Check for an event */
    if (Event)
    {
        /* Reference it */
        Status = ObReferenceObjectByHandle(Event,
                                           EVENT_MODIFY_STATE,
                                           ExEventObjectType,
                                           PreviousMode,
                                           (PVOID*)&EventObject,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            ObDereferenceObject (FileObject);
            return Status;
        }
        
        /* Clear it */
        KeClearEvent(EventObject);
    }

    /* Check if this is a direct open or not */
    if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
    {
        DeviceObject = IoGetAttachedDevice(FileObject->DeviceObject);
    }
    else
    {
        DeviceObject = IoGetRelatedDeviceObject(FileObject);
    }

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Use File Object event */
        KeClearEvent(&FileObject->Event);
    }
    else
    {
        /* Use local event */
        LocalEvent = TRUE;
    }

    /* Build the IRP */
    Irp = IoBuildDeviceIoControlRequest(IoControlCode,
                                        DeviceObject,
                                        InputBuffer,
                                        InputBufferLength,
                                        OutputBuffer,
                                        OutputBufferLength,
                                        FALSE,
                                        EventObject,
                                        IoStatusBlock);

    if (Irp == NULL)
    {
        DPRINT1("IoBuildDeviceIoControlRequest failed!\n");

        if (EventObject != NULL)
        {
            ObDereferenceObject (EventObject);
        }

        ObDereferenceObject (FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Set some extra settings */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->RequestorMode = PreviousMode;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = UserApcRoutine;
    Irp->Overlay.AsynchronousParameters.UserApcContext = UserApcContext;
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->FileObject = FileObject;
    StackPtr->MajorFunction = IsDevIoCtl ? 
                              IRP_MJ_DEVICE_CONTROL : IRP_MJ_FILE_SYSTEM_CONTROL;
    
    /* Call the Driver */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        if (!LocalEvent)
        {
            KeWaitForSingleObject(&FileObject->Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = FileObject->FinalStatus;
        }
    }

    /* Return the Status */
    return Status;
}
                      
/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoCheckQuerySetFileInformation(IN FILE_INFORMATION_CLASS FileInformationClass,
                               IN ULONG Length,
                               IN BOOLEAN SetOperation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoCheckQuotaBufferValidity(IN PFILE_QUOTA_INFORMATION QuotaBuffer,
                           IN ULONG QuotaLength,
                           OUT PULONG ErrorOffset)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * NAME       EXPORTED
 *  IoCreateFile@56
 *
 * DESCRIPTION
 *  Either causes a new file or directory to be created, or it
 *  opens an existing file, device, directory or volume, giving
 *  the caller a handle for the file object. This handle can be
 *  used by subsequent calls to manipulate data within the file
 *  or the file object's state of attributes.
 *
 * ARGUMENTS
 * FileHandle (OUT)
 *  Points to a variable which receives the file handle
 *  on return;
 *
 * DesiredAccess
 *  Desired access to the file;
 *
 * ObjectAttributes
 *  Structure describing the file;
 *
 * IoStatusBlock (OUT)
 *  Receives information about the operation on return;
 *
 * AllocationSize [OPTIONAL]
 *  Initial size of the file in bytes;
 *
 * FileAttributes
 *  Attributes to create the file with;
 *
 * ShareAccess
 *  Type of shared access the caller would like to the
 *  file;
 *
 * CreateDisposition
 *  Specifies what to do, depending on whether the
 *  file already exists;
 *
 * CreateOptions
 *  Options for creating a new file;
 *
 * EaBuffer [OPTIONAL]
 *  Undocumented;
 *
 * EaLength
 *  Undocumented;
 *
 * CreateFileType
 *  Type of file (normal, named pipe, mailslot) to create;
 *
 * ExtraCreateParameters [OPTIONAL]
 *  Additional creation data for named pipe and mailsots;
 *
 * Options
 *  Undocumented.
 *
 * RETURN VALUE
 *  Status
 *
 * NOTE
 *  Prototype taken from Bo Branten's ntifs.h v15.
 *  Description taken from old NtCreateFile's which is
 *  now a wrapper of this call.
 *
 * REVISIONS
 *
 * @implemented
 */
NTSTATUS
STDCALL
IoCreateFile(OUT PHANDLE  FileHandle,
             IN ACCESS_MASK  DesiredAccess,
             IN POBJECT_ATTRIBUTES ObjectAttributes,
             OUT PIO_STATUS_BLOCK IoStatusBlock,
             IN PLARGE_INTEGER  AllocationSize  OPTIONAL,
             IN ULONG   FileAttributes,
             IN ULONG   ShareAccess,
             IN ULONG   CreateDisposition,
             IN ULONG   CreateOptions,
             IN PVOID   EaBuffer  OPTIONAL,
             IN ULONG   EaLength,
             IN CREATE_FILE_TYPE CreateFileType,
             IN PVOID   ExtraCreateParameters OPTIONAL,
             IN ULONG   Options)
{
   PFILE_OBJECT  FileObject = NULL;
   PDEVICE_OBJECT DeviceObject;
   PIRP   Irp;
   PEXTENDED_IO_STACK_LOCATION StackLoc;
   IO_SECURITY_CONTEXT  SecurityContext;
   KPROCESSOR_MODE      AccessMode;
   HANDLE               LocalHandle;
   LARGE_INTEGER        SafeAllocationSize;
   PVOID                SystemEaBuffer = NULL;
   NTSTATUS  Status = STATUS_SUCCESS;
   AUX_DATA AuxData;
   ACCESS_STATE AccessState;

   DPRINT("IoCreateFile(FileHandle 0x%p, DesiredAccess %x, "
          "ObjectAttributes 0x%p ObjectAttributes->ObjectName->Buffer %S)\n",
          FileHandle,DesiredAccess,ObjectAttributes,
          ObjectAttributes->ObjectName->Buffer);

   ASSERT_IRQL(PASSIVE_LEVEL);

   if (IoStatusBlock == NULL || FileHandle == NULL)
     return STATUS_ACCESS_VIOLATION;

   LocalHandle = 0;

   if(Options & IO_NO_PARAMETER_CHECKING)
     AccessMode = KernelMode;
   else
     AccessMode = ExGetPreviousMode();

   if(AccessMode != KernelMode)
   {
     _SEH_TRY
     {
       ProbeForWriteHandle(FileHandle);
       ProbeForWrite(IoStatusBlock,
                     sizeof(IO_STATUS_BLOCK),
                     sizeof(ULONG));
       if(AllocationSize != NULL)
       {
         SafeAllocationSize = ProbeForReadLargeInteger(AllocationSize);
       }
       else
         SafeAllocationSize.QuadPart = 0;

       if(EaBuffer != NULL && EaLength > 0)
       {
         ProbeForRead(EaBuffer,
                      EaLength,
                      sizeof(ULONG));

         /* marshal EaBuffer */
         SystemEaBuffer = ExAllocatePool(NonPagedPool,
                                         EaLength);
         if(SystemEaBuffer == NULL)
         {
           Status = STATUS_INSUFFICIENT_RESOURCES;
           _SEH_LEAVE;
         }

         RtlCopyMemory(SystemEaBuffer,
                       EaBuffer,
                       EaLength);
       }
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;

     if(!NT_SUCCESS(Status))
     {
       return Status;
     }
   }
   else
   {
     if(AllocationSize != NULL)
       SafeAllocationSize = *AllocationSize;
     else
       SafeAllocationSize.QuadPart = 0;

     if(EaBuffer != NULL && EaLength > 0)
     {
       SystemEaBuffer = EaBuffer;
     }
   }

   if(Options & IO_CHECK_CREATE_PARAMETERS)
   {
     DPRINT1("FIXME: IO_CHECK_CREATE_PARAMETERS not yet supported!\n");
   }

   /* First try to open an existing named object */
   Status = ObOpenObjectByName(ObjectAttributes,
                               NULL,
                               NULL,
                               AccessMode,
                               DesiredAccess,
                               NULL,
                               &LocalHandle);

   if (NT_SUCCESS(Status))
   {
      OBJECT_CREATE_INFORMATION ObjectCreateInfo;
      OBJECT_ATTRIBUTES tmpObjectAttributes;
      UNICODE_STRING ObjectName;

      Status = ObReferenceObjectByHandle(LocalHandle,
                                         DesiredAccess,
                                         NULL,
                                         AccessMode,
                                         (PVOID*)&DeviceObject,
                                         NULL);
      ZwClose(LocalHandle);
      if (!NT_SUCCESS(Status))
      {
         return Status;
      }
      if (BODY_TO_HEADER(DeviceObject)->Type != IoDeviceObjectType)
      {
         ObDereferenceObject (DeviceObject);
         return STATUS_OBJECT_NAME_COLLISION;
      }

      Status = ObpCaptureObjectAttributes(ObjectAttributes,
                                          AccessMode,
                                          NULL,
                                          &ObjectCreateInfo,
                                          &ObjectName);
      if (!NT_SUCCESS(Status))
      {
         ObDereferenceObject (DeviceObject);
         return Status;
      }
         
      InitializeObjectAttributes(&tmpObjectAttributes,
                                 NULL,
                                 ObjectCreateInfo.Attributes & OBJ_INHERIT,
                                 0,
                                 NULL);
      ObpReleaseCapturedAttributes(&ObjectCreateInfo);
      if (ObjectName.Buffer) ExFreePool(ObjectName.Buffer);

      
      /* FIXME: wt... */
      Status = ObCreateObject(KernelMode,
                              IoFileObjectType,
                              &tmpObjectAttributes,
                              KernelMode,
                              NULL,
                              sizeof(FILE_OBJECT),
                              0,
                              0,
                              (PVOID*)&FileObject);

   
      /* Set File Object Data */
      FileObject->DeviceObject = IoGetAttachedDevice(DeviceObject); 
      FileObject->Vpb = FileObject->DeviceObject->Vpb;

      /* HACK */
      FileObject->Flags |= FO_DIRECT_DEVICE_OPEN;
      DPRINT("%wZ\n", ObjectAttributes->ObjectName);

      ObDereferenceObject (DeviceObject);
   }


   if (FileObject == NULL)
   {
      Status = ObCreateObject(AccessMode,
                              IoFileObjectType,
                              ObjectAttributes,
                              AccessMode,
                              NULL,
                              sizeof(FILE_OBJECT),
                              0,
                              0,
                              (PVOID*)&FileObject);
      if (!NT_SUCCESS(Status))
      {
         DPRINT("ObCreateObject() failed! (Status %lx)\n", Status);
         return Status;
      }
   }
   RtlMapGenericMask(&DesiredAccess,
                     &BODY_TO_HEADER(FileObject)->Type->TypeInfo.GenericMapping);

   Status = ObInsertObject ((PVOID)FileObject,
                            NULL,
                            DesiredAccess,
                            0,
                            NULL,
                            &LocalHandle);
   if (!NT_SUCCESS(Status))
     {
       DPRINT("ObInsertObject() failed! (Status %lx)\n", Status);
       ObMakeTemporaryObject(FileObject);
       ObDereferenceObject (FileObject);
       return Status;
     }

   if (CreateOptions & FILE_SYNCHRONOUS_IO_ALERT)
     {
       FileObject->Flags |= (FO_ALERTABLE_IO | FO_SYNCHRONOUS_IO);
     }
   if (CreateOptions & FILE_SYNCHRONOUS_IO_NONALERT)
     {
       FileObject->Flags |= FO_SYNCHRONOUS_IO;
     }

   if (CreateOptions & FILE_NO_INTERMEDIATE_BUFFERING)
     FileObject->Flags |= FO_NO_INTERMEDIATE_BUFFERING;

    /* 
     * FIXME: We should get the access state from Ob once this function becomes
     * a parse routine once the Ob is refactored.
     */
   SeCreateAccessState(&AccessState, &AuxData, FILE_ALL_ACCESS, NULL);

   SecurityContext.SecurityQos = NULL; /* ?? */
   SecurityContext.AccessState = &AccessState;
   SecurityContext.DesiredAccess = DesiredAccess;
   SecurityContext.FullCreateOptions = 0; /* ?? */

   KeInitializeEvent(&FileObject->Lock, SynchronizationEvent, TRUE);
   KeInitializeEvent(&FileObject->Event, NotificationEvent, FALSE);

   DPRINT("FileObject 0x%p\n", FileObject);
   DPRINT("FileObject->DeviceObject 0x%p\n", FileObject->DeviceObject);
   /*
    * Create a new IRP to hand to
    * the FS driver: this may fail
    * due to resource shortage.
    */
   Irp = IoAllocateIrp(FileObject->DeviceObject->StackSize, FALSE);
   if (Irp == NULL)
     {
       ZwClose(LocalHandle);
       return STATUS_UNSUCCESSFUL;
     }

   //trigger FileObject/Event dereferencing
   Irp->Tail.Overlay.OriginalFileObject = FileObject;
   Irp->RequestorMode = AccessMode;
   Irp->UserIosb = IoStatusBlock;
   Irp->AssociatedIrp.SystemBuffer = SystemEaBuffer;
   Irp->Tail.Overlay.AuxiliaryBuffer = NULL;
   Irp->Tail.Overlay.Thread = PsGetCurrentThread();
   Irp->UserEvent = &FileObject->Event;
   Irp->Overlay.AllocationSize = SafeAllocationSize;

   /*
    * Get the stack location for the new
    * IRP and prepare it.
    */
   StackLoc = (PEXTENDED_IO_STACK_LOCATION)IoGetNextIrpStackLocation(Irp);
   StackLoc->MinorFunction = 0;
   StackLoc->Flags = (UCHAR)Options;
   StackLoc->Control = 0;
   StackLoc->DeviceObject = FileObject->DeviceObject;
   StackLoc->FileObject = FileObject;

   switch (CreateFileType)
     {
       default:
       case CreateFileTypeNone:
         StackLoc->MajorFunction = IRP_MJ_CREATE;
         StackLoc->Parameters.Create.SecurityContext = &SecurityContext;
         StackLoc->Parameters.Create.Options = (CreateOptions & FILE_VALID_OPTION_FLAGS);
         StackLoc->Parameters.Create.Options |= (CreateDisposition << 24);
         StackLoc->Parameters.Create.FileAttributes = (USHORT)FileAttributes;
         StackLoc->Parameters.Create.ShareAccess = (USHORT)ShareAccess;
         StackLoc->Parameters.Create.EaLength = SystemEaBuffer != NULL ? EaLength : 0;
         break;

      case CreateFileTypeNamedPipe:
        StackLoc->MajorFunction = IRP_MJ_CREATE_NAMED_PIPE;
        StackLoc->Parameters.CreatePipe.SecurityContext = &SecurityContext;
        StackLoc->Parameters.CreatePipe.Options = (CreateOptions & FILE_VALID_OPTION_FLAGS);
        StackLoc->Parameters.CreatePipe.Options |= (CreateDisposition << 24);
        StackLoc->Parameters.CreatePipe.ShareAccess = (USHORT)ShareAccess;
        StackLoc->Parameters.CreatePipe.Parameters = ExtraCreateParameters;
        break;

      case CreateFileTypeMailslot:
        StackLoc->MajorFunction = IRP_MJ_CREATE_MAILSLOT;
        StackLoc->Parameters.CreateMailslot.SecurityContext = &SecurityContext;
        StackLoc->Parameters.CreateMailslot.Options = (CreateOptions & FILE_VALID_OPTION_FLAGS);
        StackLoc->Parameters.CreateMailslot.Options |= (CreateDisposition << 24);
        StackLoc->Parameters.CreateMailslot.ShareAccess = (USHORT)ShareAccess;
        StackLoc->Parameters.CreateMailslot.Parameters = ExtraCreateParameters;
        break;
     }

   /*
    * Now call the driver and
    * possibly wait if it can
    * not complete the request
    * immediately.
    */
   Status = IofCallDriver(FileObject->DeviceObject, Irp );
   DPRINT("Status :%x\n", Status);
   
   if (Status == STATUS_PENDING)
     {
       KeWaitForSingleObject(&FileObject->Event,
                             Executive,
                             AccessMode,
                             FALSE,
                             NULL);
       Status = IoStatusBlock->Status;
     }
   if (!NT_SUCCESS(Status))
     {
       DPRINT("Failing create request with status %x\n", Status);
       FileObject->DeviceObject = NULL;
       FileObject->Vpb = NULL;

       ZwClose(LocalHandle);
     }
   else
     {
       _SEH_TRY
         {
           *FileHandle = LocalHandle;
         }
       _SEH_HANDLE
         {
           Status = _SEH_GetExceptionCode();
         }
       _SEH_END;
     }

   /* cleanup EABuffer if captured */
   if(AccessMode != KernelMode && SystemEaBuffer != NULL)
   {
     ExFreePool(SystemEaBuffer);
   }

   ASSERT_IRQL(PASSIVE_LEVEL);

   DPRINT("Finished IoCreateFile() (*FileHandle) 0x%p\n", (*FileHandle));

   return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoCreateFileSpecifyDeviceObjectHint(OUT PHANDLE FileHandle,
                                    IN ACCESS_MASK DesiredAccess,
                                    IN POBJECT_ATTRIBUTES ObjectAttributes,
                                    OUT PIO_STATUS_BLOCK IoStatusBlock,
                                    IN PLARGE_INTEGER AllocationSize OPTIONAL,
                                    IN ULONG FileAttributes,
                                    IN ULONG ShareAccess,
                                    IN ULONG Disposition,
                                    IN ULONG CreateOptions,
                                    IN PVOID EaBuffer OPTIONAL,
                                    IN ULONG EaLength,
                                    IN CREATE_FILE_TYPE CreateFileType,
                                    IN PVOID ExtraCreateParameters OPTIONAL,
                                    IN ULONG Options,
                                    IN PVOID DeviceObject)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * NAME       EXPORTED
 *  IoCreateStreamFileObject@8
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 * FileObject
 *  ?
 *
 * DeviceObject
 *  ?
 *
 * RETURN VALUE
 *
 * NOTE
 *
 * REVISIONS
 *
 * @implemented
 */
PFILE_OBJECT 
STDCALL
IoCreateStreamFileObject(PFILE_OBJECT FileObject,
                         PDEVICE_OBJECT DeviceObject)
{
    PFILE_OBJECT CreatedFileObject;
    NTSTATUS Status;
    
    /* FIXME: This function should call ObInsertObject. The "Lite" version 
       doesnt. This function is also called from IoCreateFile for some 
       reason. These hacks need to be removed.
    */

    DPRINT("IoCreateStreamFileObject(FileObject 0x%p, DeviceObject 0x%p)\n",
            FileObject, DeviceObject);
    PAGED_CODE();

    /* Create the File Object */
    Status = ObCreateObject(KernelMode,
                            IoFileObjectType,
                            NULL,
                            KernelMode,
                            NULL,
                            sizeof(FILE_OBJECT),
                            0,
                            0,
                            (PVOID*)&CreatedFileObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not create FileObject\n");
        return (NULL);
    }

    /* Choose Device Object */
    if (FileObject) DeviceObject = FileObject->DeviceObject;
    DPRINT("DeviceObject 0x%p\n", DeviceObject);
    
    /* HACK */
    DeviceObject = IoGetAttachedDevice(DeviceObject);
    
    /* Set File Object Data */
    CreatedFileObject->DeviceObject = DeviceObject; 
    CreatedFileObject->Vpb = DeviceObject->Vpb;
    CreatedFileObject->Type = IO_TYPE_FILE;
    CreatedFileObject->Flags |= FO_STREAM_FILE;

    /* Initialize Lock and Event */
    KeInitializeEvent(&CreatedFileObject->Event, NotificationEvent, FALSE);
    KeInitializeEvent(&CreatedFileObject->Lock, SynchronizationEvent, TRUE);

    /* Return file */
    return CreatedFileObject;
}

/*
 * @unimplemented
 */
PFILE_OBJECT
STDCALL
IoCreateStreamFileObjectEx(IN PFILE_OBJECT FileObject OPTIONAL,
                           IN PDEVICE_OBJECT DeviceObject OPTIONAL,
                           OUT PHANDLE FileObjectHandle OPTIONAL)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
PFILE_OBJECT
STDCALL
IoCreateStreamFileObjectLite(IN PFILE_OBJECT FileObject OPTIONAL,
                             IN PDEVICE_OBJECT DeviceObject OPTIONAL)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @implemented
 */
PGENERIC_MAPPING
STDCALL
IoGetFileObjectGenericMapping(VOID)
{
    return &IopFileMapping;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
IoIsFileOriginRemote(IN PFILE_OBJECT FileObject)
{
    /* Return the flag status */
    return (FileObject->Flags & FO_REMOTE_ORIGIN);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoQueryFileDosDeviceName(IN PFILE_OBJECT FileObject,
                         OUT POBJECT_NAME_INFORMATION *ObjectNameInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
IoQueryFileInformation(IN PFILE_OBJECT FileObject,
                       IN FILE_INFORMATION_CLASS FileInformationClass,
                       IN ULONG Length,
                       OUT PVOID FileInformation,
                       OUT PULONG ReturnedLength)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    PIO_STACK_LOCATION StackPtr;
    BOOLEAN LocalEvent = FALSE;
    KEVENT Event;
    NTSTATUS Status;

    ASSERT(FileInformation != NULL);

    Status = ObReferenceObjectByPointer(FileObject,
                                        FILE_READ_ATTRIBUTES,
                                        IoFileObjectType,
                                        KernelMode);
    if (!NT_SUCCESS(Status)) return(Status);

    DPRINT("FileObject 0x%p\n", FileObject);

    /* Get the Device Object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Use File Object event */
        KeClearEvent(&FileObject->Event);
    }
    else
    {
        /* Use local event */
        KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
        LocalEvent = TRUE;
    }

    /* Allocate the IRP */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

    /* Set the IRP */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->RequestorMode = KernelMode;
    Irp->AssociatedIrp.SystemBuffer = FileInformation;
    Irp->UserIosb = &IoStatusBlock;
    Irp->UserEvent = (LocalEvent) ? &Event : NULL;
    Irp->Flags = (LocalEvent) ? IRP_SYNCHRONOUS_API : 0;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    /* Set the Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_QUERY_INFORMATION;
    StackPtr->FileObject = FileObject;

    /* Set Parameters */
    StackPtr->Parameters.QueryFile.FileInformationClass = FileInformationClass;
    StackPtr->Parameters.QueryFile.Length = Length;

    /* Call the Driver */
    Status = IoCallDriver(FileObject->DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        if (LocalEvent)
        {
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  KernelMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = IoStatusBlock.Status;
        }
        else
        {
            KeWaitForSingleObject(&FileObject->Event,
                                  Executive,
                                  KernelMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = FileObject->FinalStatus;
        }
    }


    /* Return the Length and Status. ReturnedLength is NOT optional */
    *ReturnedLength = IoStatusBlock.Information;
    return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoSetFileOrigin(IN PFILE_OBJECT FileObject,
                IN BOOLEAN Remote)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/**
 * @name NtCancelIoFile
 *
 * Cancel all pending I/O operations in the current thread for specified
 * file object.
 *
 * @param FileHandle
 *        Handle to file object to cancel requests for. No specific
 *        access rights are needed.
 * @param IoStatusBlock
 *        Pointer to status block which is filled with final completition
 *        status on successful return.
 *
 * @return Status.
 *
 * @implemented
 */
NTSTATUS
STDCALL
NtCancelIoFile(IN HANDLE FileHandle,
               OUT PIO_STATUS_BLOCK IoStatusBlock)
{
   PFILE_OBJECT FileObject;
   PETHREAD Thread;
   PIRP Irp;
   KIRQL OldIrql;
   BOOLEAN OurIrpsInList = FALSE;
   LARGE_INTEGER Interval;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;

   PAGED_CODE();

   PreviousMode = KeGetPreviousMode();

   if (PreviousMode != KernelMode)
   {
      _SEH_TRY
      {
         ProbeForWrite(IoStatusBlock,
                       sizeof(IO_STATUS_BLOCK),
                       sizeof(ULONG));
      }
      _SEH_HANDLE
      {
         Status = _SEH_GetExceptionCode();
      }
      _SEH_END;

      if (!NT_SUCCESS(Status)) return Status;
   }

   Status = ObReferenceObjectByHandle(FileHandle,
                                      0,
                                      IoFileObjectType,
                                      PreviousMode,
                                      (PVOID*)&FileObject,
                                      NULL);
   if (!NT_SUCCESS(Status))
      return Status;

   /* IRP cancellations are synchronized at APC_LEVEL. */
   OldIrql = KfRaiseIrql(APC_LEVEL);

   /*
    * Walk the list of active IRPs and cancel the ones that belong to
    * our file object.
    */

   Thread = PsGetCurrentThread();

   LIST_FOR_EACH(Irp, &Thread->IrpList, IRP, ThreadListEntry)
   {
      if (Irp->Tail.Overlay.OriginalFileObject == FileObject)
      {
         IoCancelIrp(Irp);
         /* Don't break here, we want to cancel all IRPs for the file object. */
         OurIrpsInList = TRUE;
      }
   }

   KfLowerIrql(OldIrql);

   while (OurIrpsInList)
   {
      OurIrpsInList = FALSE;

      /* Wait a short while and then look if all our IRPs were completed. */
      Interval.QuadPart = -1000000; /* 100 milliseconds */
      KeDelayExecutionThread(KernelMode, FALSE, &Interval);

      OldIrql = KfRaiseIrql(APC_LEVEL);

      /*
       * Look in the list if all IRPs for the specified file object
       * are completed (or cancelled). If someone sends a new IRP
       * for our file object while we're here we can happily loop
       * forever.
       */

      LIST_FOR_EACH(Irp, &Thread->IrpList, IRP, ThreadListEntry)           
      {
         if (Irp->Tail.Overlay.OriginalFileObject == FileObject)
         {
            OurIrpsInList = TRUE;
            break;
         }
      }

      KfLowerIrql(OldIrql);
   }

   _SEH_TRY
   {
      IoStatusBlock->Status = STATUS_SUCCESS;
      IoStatusBlock->Information = 0;
      Status = STATUS_SUCCESS;
   }
   _SEH_HANDLE
   {
      Status = STATUS_UNSUCCESSFUL;
   }
   _SEH_END;

   ObDereferenceObject(FileObject);

   return Status;
}

/*
 * NAME       EXPORTED
 * NtCreateFile@44
 *
 * DESCRIPTION
 * Entry point to call IoCreateFile with
 * default parameters.
 *
 * ARGUMENTS
 *  See IoCreateFile.
 *
 * RETURN VALUE
 *  See IoCreateFile.
 *
 * REVISIONS
 *  2000-03-25 (ea)
 *   Code originally in NtCreateFile moved in IoCreateFile.
 *
 * @implemented
 */
NTSTATUS
STDCALL
NtCreateFile(PHANDLE FileHandle,
             ACCESS_MASK DesiredAccess,
             POBJECT_ATTRIBUTES ObjectAttributes,
             PIO_STATUS_BLOCK IoStatusBlock,
             PLARGE_INTEGER AllocateSize,
             ULONG FileAttributes,
             ULONG ShareAccess,
             ULONG CreateDisposition,
             ULONG CreateOptions,
             PVOID EaBuffer,
             ULONG EaLength)
{
    /* Call the I/O Function */
    return IoCreateFile(FileHandle,
                        DesiredAccess,
                        ObjectAttributes,
                        IoStatusBlock,
                        AllocateSize,
                        FileAttributes,
                        ShareAccess,
                        CreateDisposition,
                        CreateOptions,
                        EaBuffer,
                        EaLength,
                        CreateFileTypeNone,
                        NULL,
                        0);
}

NTSTATUS
STDCALL
NtCreateMailslotFile(OUT PHANDLE FileHandle,
                     IN ACCESS_MASK DesiredAccess,
                     IN POBJECT_ATTRIBUTES ObjectAttributes,
                     OUT PIO_STATUS_BLOCK IoStatusBlock,
                     IN ULONG CreateOptions,
                     IN ULONG MailslotQuota,
                     IN ULONG MaxMessageSize,
                     IN PLARGE_INTEGER TimeOut)
{
    MAILSLOT_CREATE_PARAMETERS Buffer;

    DPRINT("NtCreateMailslotFile(FileHandle 0x%p, DesiredAccess %x, "
           "ObjectAttributes 0x%p)\n",
           FileHandle,DesiredAccess,ObjectAttributes);

    PAGED_CODE();

    /* Check for Timeout */
    if (TimeOut != NULL)
    {
        if (KeGetPreviousMode() != KernelMode)
        {
            NTSTATUS Status = STATUS_SUCCESS;

            _SEH_TRY
            {
                Buffer.ReadTimeout = ProbeForReadLargeInteger(TimeOut);
            }
            _SEH_HANDLE
            {
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;

            if (!NT_SUCCESS(Status)) return Status;
        }
        else
        {
            Buffer.ReadTimeout = *TimeOut;
        }

        Buffer.TimeoutSpecified = TRUE;
    }
    else
    {
        Buffer.TimeoutSpecified = FALSE;
    }

    /* Set Settings */
    Buffer.MailslotQuota = MailslotQuota;
    Buffer.MaximumMessageSize = MaxMessageSize;

    /* Call I/O */
    return IoCreateFile(FileHandle,
                        DesiredAccess,
                        ObjectAttributes,
                        IoStatusBlock,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_CREATE,
                        CreateOptions,
                        NULL,
                        0,
                        CreateFileTypeMailslot,
                        (PVOID)&Buffer,
                        0);
}

NTSTATUS
STDCALL
NtCreateNamedPipeFile(PHANDLE FileHandle,
                      ACCESS_MASK DesiredAccess,
                      POBJECT_ATTRIBUTES ObjectAttributes,
                      PIO_STATUS_BLOCK IoStatusBlock,
                      ULONG ShareAccess,
                      ULONG CreateDisposition,
                      ULONG CreateOptions,
                      ULONG NamedPipeType,
                      ULONG ReadMode,
                      ULONG CompletionMode,
                      ULONG MaximumInstances,
                      ULONG InboundQuota,
                      ULONG OutboundQuota,
                      PLARGE_INTEGER DefaultTimeout)
{
    NAMED_PIPE_CREATE_PARAMETERS Buffer;

    DPRINT("NtCreateNamedPipeFile(FileHandle 0x%p, DesiredAccess %x, "
           "ObjectAttributes 0x%p)\n",
            FileHandle,DesiredAccess,ObjectAttributes);

    PAGED_CODE();

    /* Check for Timeout */
    if (DefaultTimeout != NULL)
    {
        if (KeGetPreviousMode() != KernelMode)
        {
            NTSTATUS Status = STATUS_SUCCESS;

            _SEH_TRY
            {
                Buffer.DefaultTimeout = ProbeForReadLargeInteger(DefaultTimeout);
            }
            _SEH_HANDLE
            {
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;

            if (!NT_SUCCESS(Status)) return Status;
        }
        else
        {
            Buffer.DefaultTimeout = *DefaultTimeout;
        }

        Buffer.TimeoutSpecified = TRUE;
    }
    else
        Buffer.TimeoutSpecified = FALSE;

    /* Set Settings */
    Buffer.NamedPipeType = NamedPipeType;
    Buffer.ReadMode = ReadMode;
    Buffer.CompletionMode = CompletionMode;
    Buffer.MaximumInstances = MaximumInstances;
    Buffer.InboundQuota = InboundQuota;
    Buffer.OutboundQuota = OutboundQuota;

    /* Call I/O */
    return IoCreateFile(FileHandle,
                        DesiredAccess,
                        ObjectAttributes,
                        IoStatusBlock,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        ShareAccess,
                        CreateDisposition,
                        CreateOptions,
                        NULL,
                        0,
                        CreateFileTypeNamedPipe,
                        (PVOID)&Buffer,
                        0);
}

/*
 * NAME       EXPORTED
 * NtDeleteFile@4
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 * ObjectAttributes
 *  ?
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * @unimplemented
 */
NTSTATUS
STDCALL
NtDeleteFile(IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    UNIMPLEMENTED;
    return(STATUS_NOT_IMPLEMENTED);
}

/*
 * @implemented
 */
NTSTATUS 
STDCALL
NtDeviceIoControlFile(IN HANDLE DeviceHandle,
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
    /* Call the Generic Function */
    return IopDeviceFsIoControl(DeviceHandle,
                                Event,
                                UserApcRoutine,
                                UserApcContext,
                                IoStatusBlock,
                                IoControlCode,
                                InputBuffer,
                                InputBufferLength,
                                OutputBuffer,
                                OutputBufferLength,
                                TRUE);
}

/*
 * @implemented
 */
NTSTATUS 
STDCALL
NtFsControlFile(IN HANDLE DeviceHandle,
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
    return IopDeviceFsIoControl(DeviceHandle,
                                Event,
                                UserApcRoutine,
                                UserApcContext,
                                IoStatusBlock,
                                IoControlCode,
                                InputBuffer,
                                InputBufferLength,
                                OutputBuffer,
                                OutputBufferLength,
                                FALSE);
}

NTSTATUS
STDCALL
NtFlushWriteBuffer(VOID)
{
    PAGED_CODE();

    KeFlushWriteBuffer();
    return STATUS_SUCCESS;
}

/*
 * FUNCTION: Flushes cached file data to disk
 * ARGUMENTS:
 *       FileHandle = Points to the file
 *  IoStatusBlock = Caller must supply storage to receive the result of
 *                       the flush buffers operation. The information field is
 *                       set to number of bytes flushed to disk.
 * RETURNS: Status
 * REMARKS: This function maps to the win32 FlushFileBuffers
 */
NTSTATUS
STDCALL
NtFlushBuffersFile(IN  HANDLE FileHandle,
                   OUT PIO_STATUS_BLOCK IoStatusBlock)
{
    PFILE_OBJECT FileObject = NULL;
    PIRP Irp;
    PIO_STACK_LOCATION StackPtr;
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_OBJECT DeviceObject;
    KEVENT Event;
    BOOLEAN LocalEvent = FALSE;
    ACCESS_MASK DesiredAccess = FILE_WRITE_DATA;
    OBJECT_HANDLE_INFORMATION ObjectHandleInfo;
    KPROCESSOR_MODE PreviousMode;

    PAGED_CODE();

    PreviousMode = KeGetPreviousMode();

    if (PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWrite(IoStatusBlock,
                          sizeof(IO_STATUS_BLOCK),
                          sizeof(ULONG));
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Get the File Object */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       0,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       &ObjectHandleInfo);
    if (!NT_SUCCESS(Status)) return(Status);

    /* check if the handle has either FILE_WRITE_DATA or FILE_APPEND_DATA was
       granted. However, if this is a named pipe, make sure we don't ask for
       FILE_APPEND_DATA as it interferes with the FILE_CREATE_PIPE_INSTANCE
       access right! */
    if (!(FileObject->Flags & FO_NAMED_PIPE))
        DesiredAccess |= FILE_APPEND_DATA;
    if (!RtlAreAnyAccessesGranted(ObjectHandleInfo.GrantedAccess,
                                  DesiredAccess))
    {
        ObDereferenceObject(FileObject);
        return STATUS_ACCESS_DENIED;
    }

    /* Check if this is a direct open or not */
    if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
    {
        DeviceObject = IoGetAttachedDevice(FileObject->DeviceObject);
    }
    else
    {
        DeviceObject = IoGetRelatedDeviceObject(FileObject);
    }

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Use File Object event */
        KeClearEvent(&FileObject->Event);
    }
    else
    {
        /* Use local event */
        KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
        LocalEvent = TRUE;
    }

    /* Allocate the IRP */
    if (!(Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE)))
    {
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Set up the IRP */
    Irp->Flags = (LocalEvent) ? IRP_SYNCHRONOUS_API : 0;
    Irp->RequestorMode = PreviousMode;
    Irp->UserIosb = IoStatusBlock;
    Irp->UserEvent = (LocalEvent) ? &Event : NULL;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Tail.Overlay.OriginalFileObject = FileObject;

    /* Set up Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_FLUSH_BUFFERS;
    StackPtr->FileObject = FileObject;

    /* Call the Driver */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        if (LocalEvent)
        {
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = IoStatusBlock->Status;
        }
        else
        {
            KeWaitForSingleObject(&FileObject->Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = FileObject->FinalStatus;
        }
    }

    /* Return the Status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
NtNotifyChangeDirectoryFile(IN HANDLE FileHandle,
                            IN HANDLE Event OPTIONAL,
                            IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
                            IN PVOID ApcContext OPTIONAL,
                            OUT PIO_STATUS_BLOCK IoStatusBlock,
                            OUT PVOID Buffer,
                            IN ULONG BufferSize,
                            IN ULONG CompletionFilter,
                            IN BOOLEAN WatchTree)
{
   PIRP Irp;
   PDEVICE_OBJECT DeviceObject;
   PFILE_OBJECT FileObject;
   PIO_STACK_LOCATION IoStack;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;

   DPRINT("NtNotifyChangeDirectoryFile()\n");

   PAGED_CODE();

   PreviousMode = ExGetPreviousMode();

   if(PreviousMode != KernelMode)
   {
     _SEH_TRY
     {
       ProbeForWrite(IoStatusBlock,
                     sizeof(IO_STATUS_BLOCK),
                     sizeof(ULONG));
       if(BufferSize != 0)
       {
         ProbeForWrite(Buffer,
                       BufferSize,
                       sizeof(ULONG));
       }
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;

     if(!NT_SUCCESS(Status))
     {
       return Status;
     }
   }

    Status = ObReferenceObjectByHandle(FileHandle,
                                       FILE_LIST_DIRECTORY,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID *)&FileObject,
                                       NULL);
    if (Status != STATUS_SUCCESS) return(Status);


   DeviceObject = FileObject->DeviceObject;


   Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
   if (Irp==NULL)
     {
       ObDereferenceObject(FileObject);
       return STATUS_UNSUCCESSFUL;
     }

   if (Event == NULL)
     {
       Event = &FileObject->Event;
     }

   /* Trigger FileObject/Event dereferencing */
   Irp->Tail.Overlay.OriginalFileObject = FileObject;
   Irp->RequestorMode = PreviousMode;
   Irp->UserIosb = IoStatusBlock;
   Irp->Tail.Overlay.Thread = PsGetCurrentThread();
   Irp->UserEvent = Event;
   KeResetEvent( Event );
   Irp->UserBuffer = Buffer;
   Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
   Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;

   IoStack = IoGetNextIrpStackLocation(Irp);

   IoStack->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
   IoStack->MinorFunction = IRP_MN_NOTIFY_CHANGE_DIRECTORY;
   IoStack->Flags = 0;
   IoStack->Control = 0;
   IoStack->DeviceObject = DeviceObject;
   IoStack->FileObject = FileObject;

   if (WatchTree)
     {
       IoStack->Flags = SL_WATCH_TREE;
     }

   IoStack->Parameters.NotifyDirectory.CompletionFilter = CompletionFilter;
   IoStack->Parameters.NotifyDirectory.Length = BufferSize;

   Status = IoCallDriver(FileObject->DeviceObject,Irp);

   /* FIXME: Should we wait here or not for synchronously opened files? */

   return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtLockFile(IN HANDLE FileHandle,
           IN HANDLE EventHandle OPTIONAL,
           IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
           IN PVOID ApcContext OPTIONAL,
           OUT PIO_STATUS_BLOCK IoStatusBlock,
           IN PLARGE_INTEGER ByteOffset,
           IN PLARGE_INTEGER Length,
           IN ULONG  Key,
           IN BOOLEAN FailImmediately,
           IN BOOLEAN ExclusiveLock)
{
    PFILE_OBJECT FileObject = NULL;
    PLARGE_INTEGER LocalLength = NULL;
    PIRP Irp = NULL;
    PIO_STACK_LOCATION StackPtr;
    PDEVICE_OBJECT DeviceObject;
    PKEVENT Event = NULL;
    BOOLEAN LocalEvent = FALSE;
    KPROCESSOR_MODE PreviousMode;
    LARGE_INTEGER CapturedByteOffset, CapturedLength;
    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_HANDLE_INFORMATION HandleInformation;

    PAGED_CODE();

    PreviousMode = KeGetPreviousMode();

    CapturedByteOffset.QuadPart = 0;
    CapturedLength.QuadPart = 0;

    /* Get File Object */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       0,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       &HandleInformation);
    if (!NT_SUCCESS(Status)) return Status;

    if (PreviousMode != KernelMode)
    {
        /* Must have either FILE_READ_DATA or FILE_WRITE_DATA access unless
           we're in KernelMode! */
        if (!(HandleInformation.GrantedAccess & (FILE_WRITE_DATA | FILE_READ_DATA)))
        {
            DPRINT1("Invalid access rights\n");
            ObDereferenceObject(FileObject);
            return STATUS_ACCESS_DENIED;
        }

        _SEH_TRY
        {
            ProbeForWrite(IoStatusBlock,
                          sizeof(IO_STATUS_BLOCK),
                          sizeof(ULONG));
            CapturedByteOffset = ProbeForReadLargeInteger(ByteOffset);
            CapturedLength = ProbeForReadLargeInteger(Length);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if (!NT_SUCCESS(Status))
        {
            ObDereferenceObject(FileObject);
            return Status;
        }
    }
    else
    {
        CapturedByteOffset = *ByteOffset;
        CapturedLength = *Length;
    }

    /* Get Event Object */
    if (EventHandle)
    {
        Status = ObReferenceObjectByHandle(EventHandle,
                                           EVENT_MODIFY_STATE,
                                           ExEventObjectType,
                                           PreviousMode,
                                           (PVOID *)&Event,
                                           NULL);
        if (Status != STATUS_SUCCESS) return(Status);
        KeClearEvent(Event);
    }

    /* Check if this is a direct open or not */
    if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
    {
        DeviceObject = IoGetAttachedDevice(FileObject->DeviceObject);
    }
    else
    {
        DeviceObject = IoGetRelatedDeviceObject(FileObject);
    }

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Use File Object event */
        KeClearEvent(&FileObject->Event);
    }
    else
    {
        LocalEvent = TRUE;
    }

    /* Allocate the IRP */
    if (!(Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE)))
    {
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Allocate local buffer */  
    LocalLength = ExAllocatePoolWithTag(NonPagedPool,
                                        sizeof(LARGE_INTEGER),
                                        TAG_LOCK);
    if (!LocalLength)
    {
        IoFreeIrp(Irp);
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    *LocalLength = CapturedLength;
    
    /* Set up the IRP */
    Irp->RequestorMode = PreviousMode;
    Irp->UserIosb = IoStatusBlock;
    Irp->UserEvent = Event;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Tail.Overlay.OriginalFileObject = FileObject;

    /* Set up Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_LOCK_CONTROL;
    StackPtr->MinorFunction = IRP_MN_LOCK;
    StackPtr->FileObject = FileObject;
    
    /* Set Parameters */
    StackPtr->Parameters.LockControl.Length = LocalLength;
    StackPtr->Parameters.LockControl.ByteOffset = CapturedByteOffset;
    StackPtr->Parameters.LockControl.Key = Key;

    /* Set Flags */
    if (FailImmediately) StackPtr->Flags = SL_FAIL_IMMEDIATELY;
    if (ExclusiveLock) StackPtr->Flags |= SL_EXCLUSIVE_LOCK;

    /* Call the Driver */
    FileObject->LockOperation = TRUE;
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        if (!LocalEvent)
        {
            KeWaitForSingleObject(&FileObject->Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = FileObject->FinalStatus;
        }
    }

    /* Return the Status */
    return Status;
}

/*
 * NAME       EXPORTED
 *  NtOpenFile@24
 *
 * DESCRIPTION
 *  Opens an existing file (simpler than NtCreateFile).
 *
 * ARGUMENTS
 * FileHandle (OUT)
 *  Variable that receives the file handle on return;
 *
 * DesiredAccess
 *  Access desired by the caller to the file;
 *
 * ObjectAttributes
 *  Structue describing the file to be opened;
 *
 * IoStatusBlock (OUT)
 *  Receives details about the result of the
 *  operation;
 *
 * ShareAccess
 *  Type of shared access the caller requires;
 *
 * OpenOptions
 *  Options for the file open.
 *
 * RETURN VALUE
 *  Status.
 *
 * NOTE
 *  Undocumented.
 *
 * @implemented
 */
NTSTATUS
STDCALL
NtOpenFile(PHANDLE FileHandle,
           ACCESS_MASK DesiredAccess,
           POBJECT_ATTRIBUTES ObjectAttributes,
           PIO_STATUS_BLOCK IoStatusBlock,
           ULONG ShareAccess,
           ULONG OpenOptions)
{
    /* Call the I/O Function */
    return IoCreateFile(FileHandle,
                        DesiredAccess,
                        ObjectAttributes,
                        IoStatusBlock,
                        NULL,
                        0,
                        ShareAccess,
                        FILE_OPEN,
                        OpenOptions,
                        NULL,
                        0,
                        CreateFileTypeNone,
                        NULL,
                        0);
}

NTSTATUS
IopQueryAttributesFile(IN POBJECT_ATTRIBUTES ObjectAttributes,
                       IN FILE_INFORMATION_CLASS FileInformationClass,
                       OUT PVOID FileInformation)
{
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    NTSTATUS Status;
    KPROCESSOR_MODE AccessMode;
    UNICODE_STRING ObjectName;
    OBJECT_CREATE_INFORMATION ObjectCreateInfo;
    OBJECT_ATTRIBUTES LocalObjectAttributes;
    ULONG BufferSize;
    union
    {
        FILE_BASIC_INFORMATION BasicInformation;
        FILE_NETWORK_OPEN_INFORMATION NetworkOpenInformation;
    }LocalFileInformation;

    if (FileInformationClass == FileBasicInformation)
    {
        BufferSize = sizeof(FILE_BASIC_INFORMATION);
    }
    else if (FileInformationClass == FileNetworkOpenInformation)
    {
        BufferSize = sizeof(FILE_NETWORK_OPEN_INFORMATION);
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

    AccessMode = ExGetPreviousMode();

    if (AccessMode != KernelMode)
    {
        Status = STATUS_SUCCESS;
        _SEH_TRY
        {
            ProbeForWrite(FileInformation,
                          BufferSize,
                          sizeof(ULONG));
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        if (NT_SUCCESS(Status))
        {
            Status = ObpCaptureObjectAttributes(ObjectAttributes,
                                                AccessMode,
                                                NULL,
                                                &ObjectCreateInfo,
                                                &ObjectName);
        }
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
        InitializeObjectAttributes(&LocalObjectAttributes,
                                   &ObjectName,
                                   ObjectCreateInfo.Attributes,
                                   ObjectCreateInfo.RootDirectory,
                                   ObjectCreateInfo.SecurityDescriptor);
    }

    /* Open the file */
    Status = ZwOpenFile(&FileHandle,
                        SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                        AccessMode == KernelMode ? ObjectAttributes : &LocalObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (AccessMode != KernelMode)
    {
        ObpReleaseCapturedAttributes(&ObjectCreateInfo);
        ExFreePool(ObjectName.Buffer);
    }
    if (!NT_SUCCESS (Status))
    {
        DPRINT ("ZwOpenFile() failed (Status %lx)\n", Status);
        return Status;
    }

    /* Get file attributes */
    Status = ZwQueryInformationFile(FileHandle,
                                    &IoStatusBlock,
                                    AccessMode == KernelMode ? FileInformation : &LocalFileInformation,
                                    BufferSize,
                                    FileInformationClass);
    if (!NT_SUCCESS (Status))
    {
        DPRINT ("ZwQueryInformationFile() failed (Status %lx)\n", Status);
    }
    ZwClose(FileHandle);

    if (NT_SUCCESS(Status) && AccessMode != KernelMode)
    {
        _SEH_TRY
        {
            memcpy(FileInformation, &LocalFileInformation, BufferSize);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }
    return Status;
}

NTSTATUS
STDCALL
NtQueryAttributesFile(IN POBJECT_ATTRIBUTES ObjectAttributes,
                      OUT PFILE_BASIC_INFORMATION FileInformation)
{
    return IopQueryAttributesFile(ObjectAttributes,
                                  FileBasicInformation,
                                  FileInformation);
}

static NTSTATUS NTAPI
IopQueryDirectoryFileCompletion(IN PDEVICE_OBJECT DeviceObject,
				IN PIRP Irp,
				IN PVOID Context)
{
    ASSERT (Context);

    DPRINT("IopQueryDirectoryFileCompletion was called for \'%wZ\'\n", Context);

    ExFreePool(Context);

    return STATUS_SUCCESS;
}


/*
 * @implemented
 *
 * FUNCTION: Queries a directory file.
 * ARGUMENTS:
 *   FileHandle = Handle to a directory file
 *        EventHandle  = Handle to the event signaled on completion
 *   ApcRoutine = Asynchroneous procedure callback, called on completion
 *   ApcContext = Argument to the apc.
 *   IoStatusBlock = Caller supplies storage for extended status information.
 *   FileInformation = Caller supplies storage for the resulting information.
 *
 *  FileNameInformation    FILE_NAMES_INFORMATION
 *  FileDirectoryInformation   FILE_DIRECTORY_INFORMATION
 *  FileFullDirectoryInformation  FILE_FULL_DIRECTORY_INFORMATION
 *  FileBothDirectoryInformation FILE_BOTH_DIR_INFORMATION
 *
 *   Length = Size of the storage supplied
 *   FileInformationClass = Indicates the type of information requested.
 *   ReturnSingleEntry = Specify true if caller only requests the first
 *                            directory found.
 *   FileName = Initial directory name to query, that may contain wild
 *                   cards.
 *        RestartScan = Number of times the action should be repeated
 * RETURNS: Status [ STATUS_SUCCESS, STATUS_ACCESS_DENIED, STATUS_INSUFFICIENT_RESOURCES,
 *       STATUS_INVALID_PARAMETER, STATUS_INVALID_DEVICE_REQUEST, STATUS_BUFFER_OVERFLOW,
 *       STATUS_INVALID_INFO_CLASS, STATUS_NO_SUCH_FILE, STATUS_NO_MORE_FILES ]
 */
NTSTATUS
STDCALL
NtQueryDirectoryFile(IN HANDLE FileHandle,
                     IN HANDLE PEvent OPTIONAL,
                     IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
                     IN PVOID ApcContext OPTIONAL,
                     OUT PIO_STATUS_BLOCK IoStatusBlock,
                     OUT PVOID FileInformation,
                     IN ULONG Length,
                     IN FILE_INFORMATION_CLASS FileInformationClass,
                     IN BOOLEAN ReturnSingleEntry,
                     IN PUNICODE_STRING FileName OPTIONAL,
                     IN BOOLEAN RestartScan)
{
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject = NULL;
    PIO_STACK_LOCATION StackPtr;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN LocalEvent = FALSE;
    PKEVENT Event = NULL;
    PUNICODE_STRING SearchPattern = NULL;

    DPRINT("NtQueryDirectoryFile()\n");
    PAGED_CODE();

    /* Validate User-Mode Buffers */
    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWrite(IoStatusBlock,
                          sizeof(IO_STATUS_BLOCK),
                          sizeof(ULONG));
            ProbeForWrite(FileInformation,
                          Length,
                          sizeof(ULONG));
            if (FileName)
            {
                UNICODE_STRING CapturedFileName;

                CapturedFileName = ProbeForReadUnicodeString(FileName);
                ProbeForRead(CapturedFileName.Buffer,
                             CapturedFileName.MaximumLength,
                             1);
                SearchPattern = ExAllocatePool(NonPagedPool, CapturedFileName.Length + sizeof(WCHAR) + sizeof(UNICODE_STRING));
                if (SearchPattern == NULL)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    _SEH_LEAVE;
                }
                SearchPattern->Buffer = (PWCHAR)((ULONG_PTR)SearchPattern + sizeof(UNICODE_STRING));
                SearchPattern->MaximumLength = CapturedFileName.Length + sizeof(WCHAR);
                RtlCopyUnicodeString(SearchPattern, &CapturedFileName);
            }
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status)) 
        {
            goto Cleanup;
        }
    }

    /* Get File Object */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       FILE_LIST_DIRECTORY,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID *)&FileObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
    	goto Cleanup;
    }

    /* Get Event Object */
    if (PEvent)
    {
        Status = ObReferenceObjectByHandle(PEvent,
                                           EVENT_MODIFY_STATE,
                                           ExEventObjectType,
                                           PreviousMode,
                                           (PVOID *)&Event,
                                           NULL);
        if (!NT_SUCCESS(Status)) 
        {
            goto Cleanup;
        }

        KeClearEvent(Event);
    }

    /* Check if this is a direct open or not */
    if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
    {
        DeviceObject = IoGetAttachedDevice(FileObject->DeviceObject);
    }
    else
    {
        DeviceObject = IoGetRelatedDeviceObject(FileObject);
    }

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Use File Object event */
        KeClearEvent(&FileObject->Event);
    }
    else
    {
        LocalEvent = TRUE;
    }

    /* Allocate the IRP */
    if (!(Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE)))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    /* Set up the IRP */
    Irp->RequestorMode = PreviousMode;
    Irp->UserIosb = IoStatusBlock;
    Irp->UserEvent = Event;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->UserBuffer = FileInformation;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
    Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;

    /* Set up Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->FileObject = FileObject;
    StackPtr->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
    StackPtr->MinorFunction = IRP_MN_QUERY_DIRECTORY;

    /* Set Parameters */
    StackPtr->Parameters.QueryDirectory.FileInformationClass = FileInformationClass;
    StackPtr->Parameters.QueryDirectory.FileName = SearchPattern ? SearchPattern : FileName;
    StackPtr->Parameters.QueryDirectory.FileIndex = 0;
    StackPtr->Parameters.QueryDirectory.Length = Length;
    StackPtr->Flags = 0;
    if (RestartScan) StackPtr->Flags = SL_RESTART_SCAN;
    if (ReturnSingleEntry) StackPtr->Flags |= SL_RETURN_SINGLE_ENTRY;

    if (SearchPattern)
    {
        IoSetCompletionRoutine(Irp,
                               IopQueryDirectoryFileCompletion,
			       SearchPattern,
			       TRUE,
			       TRUE,
			       TRUE);
    }

    /* Call the Driver */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        if (!LocalEvent)
        {
            KeWaitForSingleObject(&FileObject->Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = FileObject->FinalStatus;
        }
    }


    return Status;

Cleanup:
    if (FileObject != NULL)
    {
        ObDereferenceObject(FileObject);
    }
    if (Event != NULL)
    {
        ObDereferenceObject(Event);
    }
    if (SearchPattern != NULL)
    {
        ExFreePool(SearchPattern);
    }

    /* Return the Status */
    return Status;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtQueryEaFile(IN HANDLE FileHandle,
              OUT PIO_STATUS_BLOCK IoStatusBlock,
              OUT PVOID Buffer,
              IN ULONG Length,
              IN BOOLEAN ReturnSingleEntry,
              IN PVOID EaList OPTIONAL,
              IN ULONG EaListLength,
              IN PULONG EaIndex OPTIONAL,
              IN BOOLEAN RestartScan)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
STDCALL
NtQueryFullAttributesFile(IN POBJECT_ATTRIBUTES ObjectAttributes,
                          OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation)
{
  return IopQueryAttributesFile(ObjectAttributes,
                                FileNetworkOpenInformation,
                                FileInformation);
}

/*
 * @implemented
 */
NTSTATUS STDCALL
NtQueryInformationFile(HANDLE FileHandle,
                       PIO_STATUS_BLOCK IoStatusBlock,
                       PVOID FileInformation,
                       ULONG Length,
                       FILE_INFORMATION_CLASS FileInformationClass)
{
    OBJECT_HANDLE_INFORMATION HandleInformation;
    PFILE_OBJECT FileObject;
    NTSTATUS Status;
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    PIO_STACK_LOCATION StackPtr;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    KEVENT Event;
    BOOLEAN LocalEvent = FALSE;
    BOOLEAN Failed = FALSE;

    ASSERT(IoStatusBlock != NULL);
    ASSERT(FileInformation != NULL);

    DPRINT("NtQueryInformationFile(Handle 0x%p StatBlk 0x%p FileInfo 0x%p Length %d "
           "Class %d)\n", FileHandle, IoStatusBlock, FileInformation,
            Length, FileInformationClass);

    /* Reference the Handle */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       0,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID *)&FileObject,
                                       &HandleInformation);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check information class specific access rights */
    switch (FileInformationClass)
    {
        case FileBasicInformation:
            if (!(HandleInformation.GrantedAccess & FILE_READ_ATTRIBUTES))
                Failed = TRUE;
            break;

        case FilePositionInformation:
            if (!(HandleInformation.GrantedAccess & (FILE_READ_DATA | FILE_WRITE_DATA)) ||
                !(FileObject->Flags & FO_SYNCHRONOUS_IO))
                Failed = TRUE;
            break;

        default:
            break;
    }

    if (Failed)
    {
        DPRINT1("NtQueryInformationFile() returns STATUS_ACCESS_DENIED!\n");
        ObDereferenceObject(FileObject);
        return STATUS_ACCESS_DENIED;
    }

    if (FileInformationClass == FilePositionInformation)
    {
       if (Length < sizeof(FILE_POSITION_INFORMATION))
       {
          Status = STATUS_BUFFER_OVERFLOW;
       }
       else
       {
          _SEH_TRY
          {
             ((PFILE_POSITION_INFORMATION)FileInformation)->CurrentByteOffset = FileObject->CurrentByteOffset;
             IoStatusBlock->Information = sizeof(FILE_POSITION_INFORMATION);
             Status = IoStatusBlock->Status = STATUS_SUCCESS;
          }
          _SEH_HANDLE
          {
             Status = _SEH_GetExceptionCode();
          }
          _SEH_END;
       }
       ObDereferenceObject(FileObject);
       return Status;
    }

    DPRINT("FileObject 0x%p\n", FileObject);

    /* Check if this is a direct open or not */
    if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
    {
        DeviceObject = IoGetAttachedDevice(FileObject->DeviceObject);
    }
    else
    {
        DeviceObject = IoGetRelatedDeviceObject(FileObject);
    }

    if (FileInformationClass == FileAlignmentInformation)
    {
       if (Length < sizeof(FILE_ALIGNMENT_INFORMATION))
       {
          Status = STATUS_BUFFER_OVERFLOW;
       }
       else
       {
          _SEH_TRY
          {
             ((PFILE_ALIGNMENT_INFORMATION)FileInformation)->AlignmentRequirement = DeviceObject->AlignmentRequirement;
             IoStatusBlock->Information = sizeof(FILE_ALIGNMENT_INFORMATION);
             Status = IoStatusBlock->Status = STATUS_SUCCESS;
          }
          _SEH_HANDLE
          {
             Status = _SEH_GetExceptionCode();
          }
          _SEH_END;
       }
       ObDereferenceObject(FileObject);
       return Status;
    }

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Use File Object event */
        KeClearEvent(&FileObject->Event);
    }
    else
    {
        /* Use local event */
        KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
        LocalEvent = TRUE;
    }

    /* Allocate the IRP */
    if (!(Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE)))
    {
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Allocate the System Buffer */
    if (!(Irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                                                  Length,
                                                                  TAG_SYSB)))
    {
        IoFreeIrp(Irp);
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Set up the IRP */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->RequestorMode = PreviousMode;
    Irp->UserIosb = IoStatusBlock;
    Irp->UserEvent = (LocalEvent) ? &Event : NULL;
    Irp->UserBuffer = FileInformation;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER | IRP_INPUT_OPERATION;
    Irp->Flags |= (LocalEvent) ? IRP_SYNCHRONOUS_API : 0;

    /* Set up Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_QUERY_INFORMATION;
    StackPtr->FileObject = FileObject;

    /* Set the Parameters */
    StackPtr->Parameters.QueryFile.FileInformationClass = FileInformationClass;
    StackPtr->Parameters.QueryFile.Length = Length;

    /* Call the Driver */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        if (LocalEvent)
        {
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = IoStatusBlock->Status;
        }
        else
        {
            KeWaitForSingleObject(&FileObject->Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = FileObject->FinalStatus;
        }
    }

    /* Return the Status */
    return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtQueryQuotaInformationFile(IN HANDLE FileHandle,
                            OUT PIO_STATUS_BLOCK IoStatusBlock,
                            OUT PVOID Buffer,
                            IN ULONG Length,
                            IN BOOLEAN ReturnSingleEntry,
                            IN PVOID SidList OPTIONAL,
                            IN ULONG SidListLength,
                            IN PSID StartSid OPTIONAL,
                            IN BOOLEAN RestartScan)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * NAME       EXPORTED
 * NtReadFile
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
NTSTATUS
STDCALL
NtReadFile(IN HANDLE FileHandle,
           IN HANDLE Event OPTIONAL,
           IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
           IN PVOID ApcContext OPTIONAL,
           OUT PIO_STATUS_BLOCK IoStatusBlock,
           OUT PVOID Buffer,
           IN ULONG Length,
           IN PLARGE_INTEGER ByteOffset OPTIONAL,
           IN PULONG Key OPTIONAL)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject;
    PIRP Irp = NULL;
    PDEVICE_OBJECT DeviceObject;
    PIO_STACK_LOCATION StackPtr;
    KPROCESSOR_MODE PreviousMode;
    PKEVENT EventObject = NULL;
    LARGE_INTEGER CapturedByteOffset;
    ULONG CapturedKey = 0;
    BOOLEAN Synchronous = FALSE;

    DPRINT("NtReadFile(FileHandle 0x%p Buffer 0x%p Length %x ByteOffset 0x%p, "
           "IoStatusBlock 0x%p)\n", FileHandle, Buffer, Length, ByteOffset,
            IoStatusBlock);

    PAGED_CODE();
    
    PreviousMode = KeGetPreviousMode();
    CapturedByteOffset.QuadPart = 0;

    /* Validate User-Mode Buffers */
    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWrite(IoStatusBlock,
                          sizeof(IO_STATUS_BLOCK),
                          sizeof(ULONG));
            ProbeForWrite(Buffer,
                          Length,
                          1);
            if (ByteOffset != NULL)
            {
                CapturedByteOffset = ProbeForReadLargeInteger(ByteOffset);
            }
            if (Key != NULL)
            {
                CapturedKey = ProbeForReadUlong(Key);
            }
            /* FIXME - probe other pointers and capture information */
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }
    else
    {
        if (ByteOffset != NULL)
        {
            CapturedByteOffset = *ByteOffset;
        }
        if (Key != NULL)
        {
            CapturedKey = *Key;
        }
    }

    /* Get File Object */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       FILE_READ_DATA,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        if (ByteOffset == NULL ||
            (CapturedByteOffset.u.LowPart == FILE_USE_FILE_POINTER_POSITION &&
             CapturedByteOffset.u.HighPart == -1))
        {
            /* Use the Current Byte OFfset */
            CapturedByteOffset = FileObject->CurrentByteOffset;
        }

        Synchronous = TRUE;
    }
    else if (ByteOffset == NULL && !(FileObject->Flags & FO_NAMED_PIPE))
    {
        /* a valid ByteOffset is required if asynch. op. */
        DPRINT1("NtReadFile: missing ByteOffset for asynch. op (0x%p)\n",
                ByteOffset);

        ObDereferenceObject(FileObject);
        return STATUS_INVALID_PARAMETER;
    }

    /* Check for event */
    if (Event)
    {
        /* Reference it */
        Status = ObReferenceObjectByHandle(Event,
                                           EVENT_MODIFY_STATE,
                                           ExEventObjectType,
                                           PreviousMode,
                                           (PVOID*)&EventObject,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            ObDereferenceObject(FileObject);
            return Status;
        }
        KeClearEvent(EventObject);
    }

    /* Check if this is a direct open or not */
    if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
    {
        DeviceObject = IoGetAttachedDevice(FileObject->DeviceObject);
    }
    else
    {
        DeviceObject = IoGetRelatedDeviceObject(FileObject);
    }

    KeClearEvent(&FileObject->Event);

    /* Create the IRP */
    _SEH_TRY
    {
        Irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                           DeviceObject,
                                           Buffer,
                                           Length,
                                           &CapturedByteOffset,
                                           EventObject,
                                           IoStatusBlock);

        if (Irp == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    /* Cleanup if IRP Allocation Failed */
    if (!NT_SUCCESS(Status))
    {
        if (Event) ObDereferenceObject(EventObject);
        ObDereferenceObject(FileObject);
        return Status;
    }

    /* Set up IRP Data */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->RequestorMode = PreviousMode;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
    Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;
    Irp->Flags |= IRP_READ_OPERATION;
#if 0
    /* FIXME:
     *    Vfat doesn't handle non cached files correctly.
     */     
    if (FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) Irp->Flags |= IRP_NOCACHE;
#endif      

    /* Setup Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->FileObject = FileObject;
    StackPtr->Parameters.Read.Key = CapturedKey;

    /* Call the Driver */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        if (Synchronous)
        {
            KeWaitForSingleObject(&FileObject->Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = FileObject->FinalStatus;
        }
    }

    /* Return the Status */
    return Status;
}

/*
 * NAME       EXPORTED
 * NtReadFileScatter
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
NtReadFileScatter(IN HANDLE FileHandle,
                  IN HANDLE Event OPTIONAL,
                  IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL,
                  IN PVOID UserApcContext OPTIONAL,
                  OUT PIO_STATUS_BLOCK UserIoStatusBlock,
                  IN FILE_SEGMENT_ELEMENT BufferDescription [],
                  IN ULONG BufferLength,
                  IN PLARGE_INTEGER  ByteOffset,
                  IN PULONG Key OPTIONAL)
{
    UNIMPLEMENTED;
    return(STATUS_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtSetEaFile(IN HANDLE FileHandle,
            IN PIO_STATUS_BLOCK IoStatusBlock,
            IN PVOID EaBuffer,
            IN ULONG EaBufferSize)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
NtSetInformationFile(HANDLE FileHandle,
                     PIO_STATUS_BLOCK IoStatusBlock,
                     PVOID FileInformation,
                     ULONG Length,
                     FILE_INFORMATION_CLASS FileInformationClass)
{
    OBJECT_HANDLE_INFORMATION HandleInformation;
    PIO_STACK_LOCATION StackPtr;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    KEVENT Event;
    BOOLEAN LocalEvent = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    BOOLEAN Failed = FALSE;

    DPRINT("NtSetInformationFile(Handle 0x%p StatBlk 0x%p FileInfo 0x%p Length %d "
           "Class %d)\n", FileHandle, IoStatusBlock, FileInformation,
            Length, FileInformationClass);

    if (PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            if (IoStatusBlock != NULL)
            {
                ProbeForWrite(IoStatusBlock,
                              sizeof(IO_STATUS_BLOCK),
                              sizeof(ULONG));
            }
            
            if (Length != 0)
            {
                ProbeForRead(FileInformation,
                             Length,
                             1);
            }
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }
    else
    {
        ASSERT(IoStatusBlock != NULL);
        ASSERT(FileInformation != NULL);
    }

    /* Get the file object from the file handle */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       0,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID *)&FileObject,
                                       &HandleInformation);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check information class specific access rights */
    switch (FileInformationClass)
    {
        case FileBasicInformation:
            if (!(HandleInformation.GrantedAccess & FILE_WRITE_ATTRIBUTES))
                Failed = TRUE;
            break;

        case FileDispositionInformation:
            if (!(HandleInformation.GrantedAccess & DELETE))
                Failed = TRUE;
            break;

        case FilePositionInformation:
            if (!(HandleInformation.GrantedAccess & (FILE_READ_DATA | FILE_WRITE_DATA)) ||
                !(FileObject->Flags & FO_SYNCHRONOUS_IO))
                Failed = TRUE;
            break;

        case FileEndOfFileInformation:
            if (!(HandleInformation.GrantedAccess & FILE_WRITE_DATA))
                Failed = TRUE;
            break;

        default:
            break;
    }

    if (Failed)
    {
        DPRINT1("NtSetInformationFile() returns STATUS_ACCESS_DENIED!\n");
        ObDereferenceObject(FileObject);
        return STATUS_ACCESS_DENIED;
    }

    DPRINT("FileObject 0x%p\n", FileObject);

    if (FileInformationClass == FilePositionInformation)
    {
       if (Length < sizeof(FILE_POSITION_INFORMATION))
       {
          Status = STATUS_BUFFER_OVERFLOW;
       }
       else
       {
          _SEH_TRY
          {
             FileObject->CurrentByteOffset = ((PFILE_POSITION_INFORMATION)FileInformation)->CurrentByteOffset;
             IoStatusBlock->Information = 0;
             Status = IoStatusBlock->Status = STATUS_SUCCESS;
          }
          _SEH_HANDLE
          {
             Status = _SEH_GetExceptionCode();
          }
          _SEH_END;
       }
       ObDereferenceObject(FileObject);
       return Status;
    }

    /* FIXME: Later, we can implement a lot of stuff here and avoid a driver call */
    /* Handle IO Completion Port quickly */
    if (FileInformationClass == FileCompletionInformation)
    {
        PVOID Queue;
        PFILE_COMPLETION_INFORMATION CompletionInfo = FileInformation;
        PIO_COMPLETION_CONTEXT Context;
        
        if (FileObject->Flags & FO_SYNCHRONOUS_IO || FileObject->CompletionContext != NULL)
        {
            Status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            if (Length < sizeof(FILE_COMPLETION_INFORMATION))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                /* Reference the Port */
                Status = ObReferenceObjectByHandle(CompletionInfo->Port, /* FIXME - protect with SEH! */
                                                   IO_COMPLETION_MODIFY_STATE,
                                                   IoCompletionType,
                                                   PreviousMode,
                                                   (PVOID*)&Queue,
                                                   NULL);
                if (NT_SUCCESS(Status))
                {
                    /* Allocate the Context */
                    Context = ExAllocatePoolWithTag(PagedPool,
                                                    sizeof(IO_COMPLETION_CONTEXT),
                                                    TAG('I', 'o', 'C', 'p'));

                    if (Context != NULL)
                    {
                        /* Set the Data */
                        Context->Key = CompletionInfo->Key; /* FIXME - protect with SEH! */
                        Context->Port = Queue;
                        
                        if (InterlockedCompareExchangePointer(&FileObject->CompletionContext,
                                                              Context,
                                                              NULL) != NULL)
                        {
                            /* someone else set the completion port in the
                               meanwhile, fail */
                            ExFreePool(Context);
                            ObDereferenceObject(Queue);
                            Status = STATUS_INVALID_PARAMETER;
                        }
                    }
                    else
                    {
                        /* Dereference the Port now */
                        ObDereferenceObject(Queue);
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
            }
        }

        /* Complete the I/O */
        ObDereferenceObject(FileObject);
        return Status;
    }

    /* Check if this is a direct open or not */
    if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
    {
        DeviceObject = IoGetAttachedDevice(FileObject->DeviceObject);
    }
    else
    {
        DeviceObject = IoGetRelatedDeviceObject(FileObject);
    }

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Use File Object event */
        KeClearEvent(&FileObject->Event);
    }
    else
    {
        /* Use local event */
        KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
        LocalEvent = TRUE;
    }

    /* Allocate the IRP */
    if (!(Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE)))
    {
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Allocate the System Buffer */
    if (!(Irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                                                  Length,
                                                                  TAG_SYSB)))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto failfreeirp;
    }

    /* Copy the data inside */
    if (PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            /* no need to probe again */
            RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
                          FileInformation,
                          Length);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        
        if (!NT_SUCCESS(Status))
        {
            ExFreePoolWithTag(Irp->AssociatedIrp.SystemBuffer,
                              TAG_SYSB);
            Irp->AssociatedIrp.SystemBuffer = NULL;
failfreeirp:
            IoFreeIrp(Irp);
            ObDereferenceObject(FileObject);
            return Status;
        }
    }
    else
    {
        RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
                      FileInformation,
                      Length);
    }

    /* Set up the IRP */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->RequestorMode = PreviousMode;
    Irp->UserIosb = IoStatusBlock;
    Irp->UserEvent = (LocalEvent) ? &Event : NULL;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
    Irp->Flags |= (LocalEvent) ? IRP_SYNCHRONOUS_API : 0;

    /* Set up Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_SET_INFORMATION;
    StackPtr->FileObject = FileObject;

    /* Set the Parameters */
    StackPtr->Parameters.SetFile.FileInformationClass = FileInformationClass;
    StackPtr->Parameters.SetFile.Length = Length;

    /* Call the Driver */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        if (LocalEvent)
        {
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            _SEH_TRY
            {
                Status = IoStatusBlock->Status;
            }
            _SEH_HANDLE
            {
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
        }
        else
        {
            KeWaitForSingleObject(&FileObject->Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            _SEH_TRY
            {
                Status = FileObject->FinalStatus;
            }
            _SEH_HANDLE
            {
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
        }
    }

    /* Return the Status */
    return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtSetQuotaInformationFile(HANDLE FileHandle,
                          PIO_STATUS_BLOCK IoStatusBlock,
                          PVOID Buffer,
                          ULONG BufferLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtUnlockFile(IN  HANDLE FileHandle,
             OUT PIO_STATUS_BLOCK IoStatusBlock,
             IN  PLARGE_INTEGER ByteOffset,
             IN  PLARGE_INTEGER Length,
             IN  ULONG Key OPTIONAL)
{
    PFILE_OBJECT FileObject = NULL;
    PLARGE_INTEGER LocalLength = NULL;
    PIRP Irp = NULL;
    PIO_STACK_LOCATION StackPtr;
    PDEVICE_OBJECT DeviceObject;
    KEVENT Event;
    BOOLEAN LocalEvent = FALSE;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status = STATUS_SUCCESS;
    LARGE_INTEGER CapturedByteOffset, CapturedLength;
    OBJECT_HANDLE_INFORMATION HandleInformation;

    PAGED_CODE();

    PreviousMode = KeGetPreviousMode();

    CapturedByteOffset.QuadPart = 0;
    CapturedLength.QuadPart = 0;

    /* Get File Object */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       0,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       &HandleInformation);
    if (!NT_SUCCESS(Status)) return Status;

    if (PreviousMode != KernelMode)
    {
        /* Must have either FILE_READ_DATA or FILE_WRITE_DATA access unless we're
           in KernelMode! */
        if (!(HandleInformation.GrantedAccess & (FILE_WRITE_DATA | FILE_READ_DATA)))
        {
            DPRINT1("Invalid access rights\n");
            ObDereferenceObject(FileObject);
            return STATUS_ACCESS_DENIED;
        }

        _SEH_TRY
        {
            ProbeForWrite(IoStatusBlock,
                          sizeof(IO_STATUS_BLOCK),
                          sizeof(ULONG));
            CapturedByteOffset = ProbeForReadLargeInteger(ByteOffset);
            CapturedLength = ProbeForReadLargeInteger(Length);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if (!NT_SUCCESS(Status))
        {
            ObDereferenceObject(FileObject);
            return Status;
        }
    }
    else
    {
        CapturedByteOffset = *ByteOffset;
        CapturedLength = *Length;
    }

    /* Check if this is a direct open or not */
    if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
    {
        DeviceObject = IoGetAttachedDevice(FileObject->DeviceObject);
    }
    else
    {
        DeviceObject = IoGetRelatedDeviceObject(FileObject);
    }

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        /* Use File Object event */
        KeClearEvent(&FileObject->Event);
    }
    else
    {
        /* Use local event */
        KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
        LocalEvent = TRUE;
    }

    /* Allocate the IRP */
    if (!(Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE)))
    {
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Allocate local buffer */  
    LocalLength = ExAllocatePoolWithTag(NonPagedPool,
                                        sizeof(LARGE_INTEGER),
                                        TAG_LOCK);
    if (!LocalLength)
    {
        IoFreeIrp(Irp);
        ObDereferenceObject(FileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    *LocalLength = CapturedLength;
    
    /* Set up the IRP */
    Irp->Flags = (LocalEvent) ? IRP_SYNCHRONOUS_API : 0;
    Irp->RequestorMode = PreviousMode;
    Irp->UserIosb = IoStatusBlock;
    Irp->UserEvent = (LocalEvent) ? &Event : NULL;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Tail.Overlay.OriginalFileObject = FileObject;

    /* Set up Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->MajorFunction = IRP_MJ_LOCK_CONTROL;
    StackPtr->MinorFunction = IRP_MN_UNLOCK_SINGLE;
    StackPtr->FileObject = FileObject;
    
    /* Set Parameters */
    StackPtr->Parameters.LockControl.Length = LocalLength;
    StackPtr->Parameters.LockControl.ByteOffset = CapturedByteOffset;
    StackPtr->Parameters.LockControl.Key = Key;

    /* Call the Driver */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        if (LocalEvent)
        {
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = IoStatusBlock->Status;
        }
        else
        {
            KeWaitForSingleObject(&FileObject->Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = FileObject->FinalStatus;
        }
    }

    /* Return the Status */
    return Status;
}

/*
 * NAME       EXPORTED
 * NtWriteFile
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
NTSTATUS
STDCALL
NtWriteFile (IN HANDLE FileHandle,
             IN HANDLE Event OPTIONAL,
             IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
             IN PVOID ApcContext OPTIONAL,
             OUT PIO_STATUS_BLOCK IoStatusBlock,
             IN PVOID Buffer,
             IN ULONG Length,
             IN PLARGE_INTEGER ByteOffset OPTIONAL, /* NOT optional for asynch. operations! */
             IN PULONG Key OPTIONAL)
{
    OBJECT_HANDLE_INFORMATION ObjectHandleInfo;
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject;
    PIRP Irp = NULL;
    PDEVICE_OBJECT DeviceObject;
    PIO_STACK_LOCATION StackPtr;
    KPROCESSOR_MODE PreviousMode;
    BOOLEAN Synchronous = FALSE;
    PKEVENT EventObject = NULL;
    LARGE_INTEGER CapturedByteOffset;
    ULONG CapturedKey = 0;
    ACCESS_MASK DesiredAccess = FILE_WRITE_DATA;

    DPRINT("NtWriteFile(FileHandle 0x%p Buffer 0x%p Length %x ByteOffset 0x%p, "
            "IoStatusBlock 0x%p)\n", FileHandle, Buffer, Length, ByteOffset,
            IoStatusBlock);

    PAGED_CODE();

    PreviousMode = KeGetPreviousMode();
    CapturedByteOffset.QuadPart = 0;

    /* Get File Object */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       0,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       &ObjectHandleInfo);
    if (!NT_SUCCESS(Status)) return Status;

    /* If this is a named pipe, make sure we don't ask for FILE_APPEND_DATA as it
       overlaps with the FILE_CREATE_PIPE_INSTANCE access right! */
    if (!(FileObject->Flags & FO_NAMED_PIPE))
        DesiredAccess |= FILE_APPEND_DATA;

    /* Validate User-Mode Buffers */
    if(PreviousMode != KernelMode)
    {
        /* check if the handle has either FILE_WRITE_DATA or FILE_APPEND_DATA was
           granted. */
        if (!RtlAreAnyAccessesGranted(ObjectHandleInfo.GrantedAccess,
                                      DesiredAccess))
        {
            ObDereferenceObject(FileObject);
            return STATUS_ACCESS_DENIED;
        }

        _SEH_TRY
        {
            ProbeForWrite(IoStatusBlock,
                          sizeof(IO_STATUS_BLOCK),
                          sizeof(ULONG));

            ProbeForRead(Buffer,
                         Length,
                         1);
            if (ByteOffset != NULL)
            {
                CapturedByteOffset = ProbeForReadLargeInteger(ByteOffset);
            }

            if (Key != NULL)
            {
                CapturedKey = ProbeForReadUlong(Key);
            }
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }
    else
    {
        if (ByteOffset != NULL)
        {
            CapturedByteOffset = *ByteOffset;
        }
        if (Key != NULL)
        {
            CapturedKey = *Key;
        }
    }

    /* check if this is an append operation */
    if ((ObjectHandleInfo.GrantedAccess & DesiredAccess) == FILE_APPEND_DATA)
    {
        /* Give the drivers something to understand */
        CapturedByteOffset.u.LowPart = FILE_WRITE_TO_END_OF_FILE;
        CapturedByteOffset.u.HighPart = -1;
    }

    /* Check if we should use Sync IO or not */
    if (FileObject->Flags & FO_SYNCHRONOUS_IO)
    {
        if (ByteOffset == NULL ||
            (CapturedByteOffset.u.LowPart == FILE_USE_FILE_POINTER_POSITION &&
             CapturedByteOffset.u.HighPart == -1))
        {
            /* Use the Current Byte OFfset */
            CapturedByteOffset = FileObject->CurrentByteOffset;
        }

        Synchronous = TRUE;
    }
    else if (ByteOffset == NULL && !(FileObject->Flags & FO_NAMED_PIPE))
    {
        /* a valid ByteOffset is required if asynch. op. */
        DPRINT1("NtReadFile: missing ByteOffset for asynch. op (0x%p)\n",
                ByteOffset);

        ObDereferenceObject(FileObject);
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if we got an event */
    if (Event)
    {
        /* Reference it */
        Status = ObReferenceObjectByHandle(Event,
                                           EVENT_MODIFY_STATE,
                                           ExEventObjectType,
                                           PreviousMode,
                                           (PVOID*)&EventObject,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            ObDereferenceObject(FileObject);
            return Status;
        }
        KeClearEvent(EventObject);
    }

    /* Check if this is a direct open or not */
    if (FileObject->Flags & FO_DIRECT_DEVICE_OPEN)
    {
        DeviceObject = IoGetAttachedDevice(FileObject->DeviceObject);
    }
    else
    {
        DeviceObject = IoGetRelatedDeviceObject(FileObject);
    }

    KeClearEvent(&FileObject->Event);

    /* Build the IRP */
    _SEH_TRY
    {
        Irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE,
                                           DeviceObject,
                                           Buffer,
                                           Length,
                                           &CapturedByteOffset,
                                           EventObject,
                                           IoStatusBlock);
        if (Irp == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    /* Cleanup on failure */
    if (!NT_SUCCESS(Status))
    {
        if (Event)
        {
            ObDereferenceObject(&EventObject);
        }
        ObDereferenceObject(FileObject);
        return Status;
    }

   /* Set up IRP Data */
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->RequestorMode = PreviousMode;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = ApcRoutine;
    Irp->Overlay.AsynchronousParameters.UserApcContext = ApcContext;
    Irp->Flags |= IRP_WRITE_OPERATION;
#if 0    
    /* FIXME:
     *    Vfat doesn't handle non cached files correctly.
     */     
    if (FileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING) Irp->Flags |= IRP_NOCACHE;
#endif    

    /* Setup Stack Data */
    StackPtr = IoGetNextIrpStackLocation(Irp);
    StackPtr->FileObject = FileObject;
    StackPtr->Parameters.Write.Key = CapturedKey;
    if (FileObject->Flags & FO_WRITE_THROUGH) StackPtr->Flags = SL_WRITE_THROUGH;

    /* Call the Driver */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        if (Synchronous)
        {
            KeWaitForSingleObject(&FileObject->Event,
                                  Executive,
                                  PreviousMode,
                                  FileObject->Flags & FO_ALERTABLE_IO,
                                  NULL);
            Status = FileObject->FinalStatus;
        }
    }

    /* Return the Status */
    return Status;
}

/*
 * NAME       EXPORTED
 * NtWriteFileGather
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
NtWriteFileGather(IN HANDLE FileHandle,
                  IN HANDLE Event OPTIONAL,
                  IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL,
                  IN PVOID UserApcContext OPTIONAL,
                  OUT PIO_STATUS_BLOCK UserIoStatusBlock,
                  IN FILE_SEGMENT_ELEMENT BufferDescription [],
                  IN ULONG BufferLength,
                  IN PLARGE_INTEGER  ByteOffset,
                  IN PULONG Key OPTIONAL)
{
    UNIMPLEMENTED;
    return(STATUS_NOT_IMPLEMENTED);
}
/* EOF */

/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/utils.c
 * PURPOSE:         Utility/Helper Support Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

ULONG PciDebugPortsCount;

RTL_RANGE_LIST PciIsaBitExclusionList;
RTL_RANGE_LIST PciVgaAndIsaBitExclusionList;

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
PciUnicodeStringStrStr(IN PUNICODE_STRING InputString,
                       IN PCUNICODE_STRING EqualString,
                       IN BOOLEAN CaseInSensitive)
{
    UNICODE_STRING PartialString;
    LONG EqualChars, TotalChars;

    /* Build a partial string with the smaller substring */
    PartialString.Length = EqualString->Length;
    PartialString.MaximumLength = InputString->MaximumLength;;
    PartialString.Buffer = InputString->Buffer;

    /* Check how many characters that need comparing */
    EqualChars = 0;
    TotalChars = (InputString->Length - EqualString->Length) / sizeof(WCHAR);

    /* If the substring is bigger, just fail immediately */
    if (TotalChars < 0) return FALSE;

    /* Keep checking each character */
    while (!RtlEqualUnicodeString(EqualString, &PartialString, CaseInSensitive))
    {
        /* Continue checking until all the required characters are equal */
        PartialString.Buffer++;
        PartialString.MaximumLength -= sizeof(WCHAR);
        if (++EqualChars > TotalChars) return FALSE;
    }

    /* The string is equal */
    return TRUE;
}

BOOLEAN
NTAPI
PciStringToUSHORT(IN PWCHAR String,
                  OUT PUSHORT Value)
{
    USHORT Short;
    ULONG Low, High, Length;
    WCHAR Char;

    /* Initialize everything to zero */
    Short = 0;
    Length = 0;
    while (TRUE)
    {
        /* Get the character and set the high byte based on the previous one */
        Char = *String++;
        High = 16 * Short;

        /* Check for numbers */
        if ( Char >= '0' && Char <= '9' )
        {
            /* Convert them to a byte */
            Low = Char - '0';
        }
        else if ( Char >= 'A' && Char <= 'F' )
        {
            /* Convert upper-case hex letters into a byte */
            Low = Char - '7';
        }
        else if ( Char >= 'a' && Char <= 'f' )
        {
            /* Convert lower-case hex letters into a byte */
            Low = Char - 'W';
        }
        else
        {
            /* Invalid string, fail the conversion */
            return FALSE;
        }

        /* Combine the high and low byte */
        Short = High | Low;

        /* If 4 letters have been reached, the 16-bit integer should exist */
        if (++Length >= 4)
        {
            /* Return it to the caller */
            *Value = Short;
            return TRUE;
        }
    }
}

BOOLEAN
NTAPI
PciIsSuiteVersion(IN USHORT SuiteMask)
{
    ULONGLONG Mask = 0;
    RTL_OSVERSIONINFOEXW VersionInfo;

    /* Initialize the version information */
    RtlZeroMemory(&VersionInfo, sizeof(RTL_OSVERSIONINFOEXW));
    VersionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
    VersionInfo.wSuiteMask = SuiteMask;

    /* Set the comparison mask and return if the passed suite mask matches */
    VER_SET_CONDITION(Mask, VER_SUITENAME, VER_AND);
    return NT_SUCCESS(RtlVerifyVersionInfo(&VersionInfo, VER_SUITENAME, Mask));
}

BOOLEAN
NTAPI
PciIsDatacenter(VOID)
{
    BOOLEAN Result;
    PVOID Value;
    ULONG ResultLength;
    NTSTATUS Status;

    /* Assume this isn't Datacenter */
    Result = FALSE;

    /* First, try opening the setup key */
    Status = PciGetRegistryValue(L"",
                                 L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\setupdd",
                                 0,
                                 REG_BINARY,
                                 &Value,
                                 &ResultLength);
    if (!NT_SUCCESS(Status))
    {
        /* This is not an in-progress Setup boot, so query the suite version */
        Result = PciIsSuiteVersion(VER_SUITE_DATACENTER);
    }
    else
    {
        /* This scenario shouldn't happen yet, since SetupDD isn't used */
        UNIMPLEMENTED;
        while (TRUE);
    }

    /* Return if this is Datacenter or not */
    return Result;
}

BOOLEAN
NTAPI
PciOpenKey(IN PWCHAR KeyName,
           IN HANDLE RootKey,
           IN ACCESS_MASK DesiredAccess,
           OUT PHANDLE KeyHandle,
           OUT PNTSTATUS KeyStatus)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyString;
    PAGED_CODE();

    /* Initialize the object attributes */
    RtlInitUnicodeString(&KeyString, KeyName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyString,
                               OBJ_CASE_INSENSITIVE,
                               RootKey,
                               NULL);

    /* Open the key, returning a boolean, and the status, if requested */
    Status = ZwOpenKey(KeyHandle, DesiredAccess, &ObjectAttributes);
    if (KeyStatus) *KeyStatus = Status;
    return NT_SUCCESS(Status);
}

NTSTATUS
NTAPI
PciGetRegistryValue(IN PWCHAR ValueName,
                    IN PWCHAR KeyName,
                    IN HANDLE RootHandle,
                    IN ULONG Type,
                    OUT PVOID *OutputBuffer,
                    OUT PULONG OutputLength)
{
    NTSTATUS Status;
    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo;
    ULONG NeededLength, ActualLength;
    UNICODE_STRING ValueString;
    HANDLE KeyHandle;
    BOOLEAN Result;

    /* So we know what to free at the end of the body */
    PartialInfo = NULL;
    KeyHandle = NULL;
    do
    {
        /* Open the key by name, rooted off the handle passed */
        Result = PciOpenKey(KeyName,
                            RootHandle,
                            KEY_QUERY_VALUE,
                            &KeyHandle,
                            &Status);
        if (!Result) break;

        /* Query for the size that's needed for the value that was passed in */
        RtlInitUnicodeString(&ValueString, ValueName);
        Status = ZwQueryValueKey(KeyHandle,
                                 &ValueString,
                                 KeyValuePartialInformation,
                                 NULL,
                                 0,
                                 &NeededLength);
        ASSERT(!NT_SUCCESS(Status));
        if (Status != STATUS_BUFFER_TOO_SMALL) break;

        /* Allocate an appropriate buffer for the size that was returned */
        ASSERT(NeededLength != 0);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        PartialInfo = ExAllocatePoolWithTag(PagedPool,
                                            NeededLength,
                                            PCI_POOL_TAG);
        if (!PartialInfo) break;

        /* Query the actual value information now that the size is known */
        Status = ZwQueryValueKey(KeyHandle,
                                 &ValueString,
                                 KeyValuePartialInformation,
                                 PartialInfo,
                                 NeededLength,
                                 &ActualLength);
        if (!NT_SUCCESS(Status)) break;

        /* Make sure it's of the type that the caller expects */
        Status = STATUS_INVALID_PARAMETER;
        if (PartialInfo->Type != Type) break;

        /* Subtract the registry-specific header, to get the data size */
        ASSERT(NeededLength == ActualLength);
        NeededLength -= sizeof(KEY_VALUE_PARTIAL_INFORMATION);

        /* Allocate a buffer to hold the data and return it to the caller */
        Status = STATUS_INSUFFICIENT_RESOURCES;
        *OutputBuffer = ExAllocatePoolWithTag(PagedPool,
                                              NeededLength,
                                              PCI_POOL_TAG);
        if (!*OutputBuffer) break;

        /* Copy the data into the buffer and return its length to the caller */
        RtlCopyMemory(*OutputBuffer, PartialInfo->Data, NeededLength);
        if (OutputLength) *OutputLength = NeededLength;
        Status = STATUS_SUCCESS;
    } while (0);

    /* Close any opened keys and free temporary allocations */
    if (KeyHandle) ZwClose(KeyHandle);
    if (PartialInfo) ExFreePoolWithTag(PartialInfo, 0);
    return Status;
}

NTSTATUS
NTAPI
PciBuildDefaultExclusionLists(VOID)
{
    ULONG Start;
    NTSTATUS Status;
    ASSERT(PciIsaBitExclusionList.Count == 0);
    ASSERT(PciVgaAndIsaBitExclusionList.Count == 0);

    /* Initialize the range lists */
    RtlInitializeRangeList(&PciIsaBitExclusionList);
    RtlInitializeRangeList(&PciVgaAndIsaBitExclusionList);

    /* Loop x86 I/O ranges */
    for (Start = 0x100; Start <= 0xFEFF; Start += 0x400)
    {
        /* Add the ISA I/O ranges */
        Status = RtlAddRange(&PciIsaBitExclusionList,
                             Start,
                             Start + 0x2FF,
                             0,
                             RTL_RANGE_LIST_ADD_IF_CONFLICT,
                             NULL,
                             NULL);
        if (!NT_SUCCESS(Status)) break;

        /* Add the ISA I/O ranges */
        Status = RtlAddRange(&PciVgaAndIsaBitExclusionList,
                             Start,
                             Start + 0x2AF,
                             0,
                             RTL_RANGE_LIST_ADD_IF_CONFLICT,
                             NULL,
                             NULL);
        if (!NT_SUCCESS(Status)) break;

        /* Add the VGA I/O range for Monochrome Video */
        Status = RtlAddRange(&PciVgaAndIsaBitExclusionList,
                             Start + 0x2BC,
                             Start + 0x2BF,
                             0,
                             RTL_RANGE_LIST_ADD_IF_CONFLICT,
                             NULL,
                             NULL);
        if (!NT_SUCCESS(Status)) break;

        /* Add the VGA I/O range for certain CGA adapters */
        Status = RtlAddRange(&PciVgaAndIsaBitExclusionList,
                             Start + 0x2E0,
                             Start + 0x2FF,
                             0,
                             RTL_RANGE_LIST_ADD_IF_CONFLICT,
                             NULL,
                             NULL);
        if (!NT_SUCCESS(Status)) break;

        /* Success, ranges added done */
    };

    RtlFreeRangeList(&PciIsaBitExclusionList);
    RtlFreeRangeList(&PciVgaAndIsaBitExclusionList);
    return Status;
}

PPCI_FDO_EXTENSION
NTAPI
PciFindParentPciFdoExtension(IN PDEVICE_OBJECT DeviceObject,
                             IN PKEVENT Lock)
{
    PPCI_FDO_EXTENSION DeviceExtension;
    PPCI_PDO_EXTENSION SearchExtension, FoundExtension;

    /* Assume we'll find nothing */
    SearchExtension = DeviceObject->DeviceExtension;
    FoundExtension = NULL;

    /* Check if a lock was specified */
    if (Lock)
    {
        /* Wait for the lock to be released */
        KeEnterCriticalRegion();
        KeWaitForSingleObject(Lock, Executive, KernelMode, FALSE, NULL);
    }

    /* Now search for the extension */
    DeviceExtension = (PPCI_FDO_EXTENSION)PciFdoExtensionListHead.Next;
    while (DeviceExtension)
    {
        /* Acquire this device's lock */
        KeEnterCriticalRegion();
        KeWaitForSingleObject(&DeviceExtension->ChildListLock,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        /* Scan all children PDO, stop when no more PDOs, or found it */
        for (FoundExtension = DeviceExtension->ChildPdoList;
             FoundExtension && (FoundExtension != SearchExtension);
             FoundExtension = FoundExtension->Next);

        /* If we found it, break out */
        if (FoundExtension) break;

        /* Release this device's lock */
        KeSetEvent(&DeviceExtension->ChildListLock, IO_NO_INCREMENT, FALSE);
        KeLeaveCriticalRegion();

        /* Move to the next device */
        DeviceExtension = (PPCI_FDO_EXTENSION)DeviceExtension->List.Next;
    }

    /* Check if we had acquired a lock previously */
    if (Lock)
    {
        /* Release it */
        KeSetEvent(Lock, IO_NO_INCREMENT, FALSE);
        KeLeaveCriticalRegion();
    }

    /* Return which extension was found, if any */
    return DeviceExtension;
}

VOID
NTAPI
PciInsertEntryAtTail(IN PSINGLE_LIST_ENTRY ListHead,
                     IN PPCI_FDO_EXTENSION DeviceExtension,
                     IN PKEVENT Lock)
{
    PSINGLE_LIST_ENTRY NextEntry;
    PAGED_CODE();

    /* Check if a lock was specified */
    if (Lock)
    {
        /* Wait for the lock to be released */
        KeEnterCriticalRegion();
        KeWaitForSingleObject(Lock, Executive, KernelMode, FALSE, NULL);
    }

    /* Loop the list until we get to the end, then insert this entry there */
    for (NextEntry = ListHead; NextEntry->Next; NextEntry = NextEntry->Next);
    NextEntry->Next = &DeviceExtension->List;

    /* Check if we had acquired a lock previously */
    if (Lock)
    {
        /* Release it */
        KeSetEvent(Lock, IO_NO_INCREMENT, FALSE);
        KeLeaveCriticalRegion();
    }
}

VOID
NTAPI
PciInsertEntryAtHead(IN PSINGLE_LIST_ENTRY ListHead,
                     IN PSINGLE_LIST_ENTRY Entry,
                     IN PKEVENT Lock)
{
    PAGED_CODE();

    /* Check if a lock was specified */
    if (Lock)
    {
        /* Wait for the lock to be released */
        KeEnterCriticalRegion();
        KeWaitForSingleObject(Lock, Executive, KernelMode, FALSE, NULL);
    }

    /* Make the entry point to the current head and make the head point to it */
    Entry->Next = ListHead->Next;
    ListHead->Next = Entry;

    /* Check if we had acquired a lock previously */
    if (Lock)
    {
        /* Release it */
        KeSetEvent(Lock, IO_NO_INCREMENT, FALSE);
        KeLeaveCriticalRegion();
    }
}

VOID
NTAPI
PcipLinkSecondaryExtension(IN PSINGLE_LIST_ENTRY List,
                           IN PVOID Lock,
                           IN PPCI_SECONDARY_EXTENSION SecondaryExtension,
                           IN PCI_SIGNATURE ExtensionType,
                           IN PVOID Destructor)
{
    PAGED_CODE();

    /* Setup the extension data, and insert it into the primary's list */
    SecondaryExtension->ExtensionType = ExtensionType;
    SecondaryExtension->Destructor = Destructor;
    PciInsertEntryAtHead(List, &SecondaryExtension->List, Lock);
}

NTSTATUS
NTAPI
PciGetDeviceProperty(IN PDEVICE_OBJECT DeviceObject,
                     IN DEVICE_REGISTRY_PROPERTY DeviceProperty,
                     OUT PVOID *OutputBuffer)
{
    NTSTATUS Status;
    ULONG BufferLength, ResultLength;
    PVOID Buffer;
    do
    {
        /* Query the requested property size */
        Status = IoGetDeviceProperty(DeviceObject,
                                     DeviceProperty,
                                     0,
                                     NULL,
                                     &BufferLength);
        if (Status != STATUS_BUFFER_TOO_SMALL)
        {
            /* Call should've failed with buffer too small! */
            DPRINT1("PCI - Unexpected status from GetDeviceProperty, saw %08X, expected %08X.\n",
                    Status,
                    STATUS_BUFFER_TOO_SMALL);
            *OutputBuffer = NULL;
            ASSERTMSG(FALSE, "PCI Successfully did the impossible!");
            break;
        }

        /* Allocate the required buffer */
        Buffer = ExAllocatePoolWithTag(PagedPool, BufferLength, 'BicP');
        if (!Buffer)
        {
            /* No memory, fail the request */
            DPRINT1("PCI - Failed to allocate DeviceProperty buffer (%d bytes).\n", BufferLength);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        /* Do the actual property query call */
        Status = IoGetDeviceProperty(DeviceObject,
                                     DeviceProperty,
                                     BufferLength,
                                     Buffer,
                                     &ResultLength);
        if (!NT_SUCCESS(Status)) break;

        /* Return the buffer to the caller */
        ASSERT(BufferLength == ResultLength);
        *OutputBuffer = Buffer;
        return STATUS_SUCCESS;
    } while (FALSE);

    /* Failure path */
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
PciSendIoctl(IN PDEVICE_OBJECT DeviceObject,
             IN ULONG IoControlCode,
             IN PVOID InputBuffer,
             IN ULONG InputBufferLength,
             IN PVOID OutputBuffer,
             IN ULONG OutputBufferLength)
{
    PIRP Irp;
    NTSTATUS Status;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    PDEVICE_OBJECT AttachedDevice;
    PAGED_CODE();

    /* Initialize the pending IRP event */
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    /* Get a reference to the root PDO (ACPI) */
    AttachedDevice = IoGetAttachedDeviceReference(DeviceObject);
    if (!AttachedDevice) return STATUS_INVALID_PARAMETER;

    /* Build the requested IOCTL IRP */
    Irp = IoBuildDeviceIoControlRequest(IoControlCode,
                                        AttachedDevice,
                                        InputBuffer,
                                        InputBufferLength,
                                        OutputBuffer,
                                        OutputBufferLength,
                                        0,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    /* Send the IOCTL to the driver */
    Status = IoCallDriver(AttachedDevice, Irp);
    if (Status == STATUS_PENDING)
    {
        /* Wait for a response */
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = Irp->IoStatus.Status;
    }

    /* Take away the reference we took and return the result to the caller */
    ObDereferenceObject(AttachedDevice);
    return Status;
}

PPCI_SECONDARY_EXTENSION
NTAPI
PciFindNextSecondaryExtension(IN PSINGLE_LIST_ENTRY ListHead,
                              IN PCI_SIGNATURE ExtensionType)
{
    PSINGLE_LIST_ENTRY NextEntry;
    PPCI_SECONDARY_EXTENSION Extension;

    /* Scan the list */
    for (NextEntry = ListHead; NextEntry; NextEntry = NextEntry->Next)
    {
        /* Grab each extension and check if it's the one requested */
        Extension = CONTAINING_RECORD(NextEntry, PCI_SECONDARY_EXTENSION, List);
        if (Extension->ExtensionType == ExtensionType) return Extension;
    }

    /* Nothing was found */
    return NULL;
}

ULONGLONG
NTAPI
PciGetHackFlags(IN USHORT VendorId,
                IN USHORT DeviceId,
                IN USHORT SubVendorId,
                IN USHORT SubSystemId,
                IN UCHAR RevisionId)
{
    PPCI_HACK_ENTRY HackEntry;
    ULONGLONG HackFlags;
    ULONG LastWeight, MatchWeight;
    ULONG EntryFlags;

    /* Initialize the variables before looping */
    LastWeight = 0;
    HackFlags = 0;
    ASSERT(PciHackTable);

    /* Scan the hack table */
    for (HackEntry = PciHackTable;
         HackEntry->VendorID != PCI_INVALID_VENDORID;
         ++HackEntry)
    {
        /* Check if there's an entry for this device */
        if ((HackEntry->DeviceID == DeviceId) &&
            (HackEntry->VendorID == VendorId))
        {
            /* This is a basic match */
            EntryFlags = HackEntry->Flags;
            MatchWeight = 1;

            /* Does the entry have revision information? */
            if (EntryFlags & PCI_HACK_HAS_REVISION_INFO)
            {
                /* Check if the revision matches, if so, this is a better match */
                if (HackEntry->RevisionID != RevisionId) continue;
                MatchWeight = 3;
            }

            /* Does the netry have subsystem information? */
            if (EntryFlags & PCI_HACK_HAS_SUBSYSTEM_INFO)
            {
                /* Check if it matches, if so, this is the best possible match */
                if ((HackEntry->SubVendorID != SubVendorId) ||
                    (HackEntry->SubSystemID != SubSystemId))
                {
                    continue;
                }
                MatchWeight += 4;
            }

            /* Is this the best match yet? */
            if (MatchWeight > LastWeight)
            {
                /* This is the best match for now, use this as the hack flags */
                HackFlags = HackEntry->HackFlags;
                LastWeight = MatchWeight;
            }
        }
    }

    /* Return the best match */
    return HackFlags;
}

BOOLEAN
NTAPI
PciIsCriticalDeviceClass(IN UCHAR BaseClass,
                         IN UCHAR SubClass)
{
    /* Check for system or bridge devices */
    if (BaseClass == PCI_CLASS_BASE_SYSTEM_DEV)
    {
        /* Interrupt controlers are critical */
        return SubClass == PCI_SUBCLASS_SYS_INTERRUPT_CTLR;
    }
    else if (BaseClass == PCI_CLASS_BRIDGE_DEV)
    {
        /* ISA Bridges are critical */
        return SubClass == PCI_SUBCLASS_BR_ISA;
    }
    else
    {
        /* All display controllers are critical */
        return BaseClass == PCI_CLASS_DISPLAY_CTLR;
    }
}

PPCI_PDO_EXTENSION
NTAPI
PciFindPdoByFunction(IN PPCI_FDO_EXTENSION DeviceExtension,
                     IN ULONG FunctionNumber,
                     IN PPCI_COMMON_HEADER PciData)
{
    KIRQL Irql;
    PPCI_PDO_EXTENSION PdoExtension;

    /* Get the current IRQL when this call was made */
    Irql = KeGetCurrentIrql();

    /* Is this a low-IRQL call? */
    if (Irql < DISPATCH_LEVEL)
    {
        /* Acquire this device's lock */
        KeEnterCriticalRegion();
        KeWaitForSingleObject(&DeviceExtension->ChildListLock,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    /* Loop every child PDO */
    for (PdoExtension = DeviceExtension->ChildPdoList;
         PdoExtension;
         PdoExtension = PdoExtension->Next)
    {
        /* Find only enumerated PDOs */
        if (!PdoExtension->ReportedMissing)
        {
            /* Check if the function number and header data matches */
            if ((FunctionNumber == PdoExtension->Slot.u.AsULONG) &&
                (PdoExtension->VendorId == PciData->VendorID) &&
                (PdoExtension->DeviceId == PciData->DeviceID) &&
                (PdoExtension->RevisionId == PciData->RevisionID))
            {
                /* This is considered to be the same PDO */
                break;
            }
        }
    }

    /* Was this a low-IRQL call? */
    if (Irql < DISPATCH_LEVEL)
    {
        /* Release this device's lock */
        KeSetEvent(&DeviceExtension->ChildListLock, IO_NO_INCREMENT, FALSE);
        KeLeaveCriticalRegion();
    }

    /* If the search found something, this is non-NULL, otherwise it's NULL */
    return PdoExtension;
}

BOOLEAN
NTAPI
PciIsDeviceOnDebugPath(IN PPCI_PDO_EXTENSION DeviceExtension)
{
    PAGED_CODE();

    /* Check for too many, or no, debug ports */
    ASSERT(PciDebugPortsCount <= MAX_DEBUGGING_DEVICES_SUPPORTED);
    if (!PciDebugPortsCount) return FALSE;

    /* eVb has not been able to test such devices yet */
    UNIMPLEMENTED;
    while (TRUE);
}

NTSTATUS
NTAPI
PciGetBiosConfig(IN PPCI_PDO_EXTENSION DeviceExtension,
                 OUT PPCI_COMMON_HEADER PciData)
{
    HANDLE KeyHandle, SubKeyHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName, KeyValue;
    WCHAR Buffer[32];
    WCHAR DataBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + PCI_COMMON_HDR_LENGTH];
    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = (PVOID)DataBuffer;
    NTSTATUS Status;
    ULONG ResultLength;
    PAGED_CODE();

    /* Open the PCI key */
    Status = IoOpenDeviceRegistryKey(DeviceExtension->ParentFdoExtension->
                                     PhysicalDeviceObject,
                                     TRUE,
                                     KEY_ALL_ACCESS,
                                     &KeyHandle);
    if (!NT_SUCCESS(Status)) return Status;

    /* Create a volatile BIOS configuration key */
    RtlInitUnicodeString(&KeyName, L"BiosConfig");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_KERNEL_HANDLE,
                               KeyHandle,
                               NULL);
    Status = ZwCreateKey(&SubKeyHandle,
                         KEY_READ,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         NULL);
    ZwClose(KeyHandle);
    if (!NT_SUCCESS(Status)) return Status;

    /* Create the key value based on the device and function number */
    swprintf(Buffer,
             L"DEV_%02x&FUN_%02x",
             DeviceExtension->Slot.u.bits.DeviceNumber,
             DeviceExtension->Slot.u.bits.FunctionNumber);
    RtlInitUnicodeString(&KeyValue, Buffer);

    /* Query the value information (PCI BIOS configuration header) */
    Status = ZwQueryValueKey(SubKeyHandle,
                             &KeyValue,
                             KeyValuePartialInformation,
                             PartialInfo,
                             sizeof(DataBuffer),
                             &ResultLength);
    ZwClose(SubKeyHandle);
    if (!NT_SUCCESS(Status)) return Status;

    /* If any information was returned, go ahead and copy its data */
    ASSERT(PartialInfo->DataLength == PCI_COMMON_HDR_LENGTH);
    RtlCopyMemory(PciData, PartialInfo->Data, PCI_COMMON_HDR_LENGTH);
    return Status;
}

NTSTATUS
NTAPI
PciSaveBiosConfig(IN PPCI_PDO_EXTENSION DeviceExtension,
                  IN PPCI_COMMON_HEADER PciData)
{
    HANDLE KeyHandle, SubKeyHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName, KeyValue;
    WCHAR Buffer[32];
    NTSTATUS Status;
    PAGED_CODE();

    /* Open the PCI key */
    Status = IoOpenDeviceRegistryKey(DeviceExtension->ParentFdoExtension->
                                     PhysicalDeviceObject,
                                     TRUE,
                                     KEY_READ | KEY_WRITE,
                                     &KeyHandle);
    if (!NT_SUCCESS(Status)) return Status;

    /* Create a volatile BIOS configuration key */
    RtlInitUnicodeString(&KeyName, L"BiosConfig");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_KERNEL_HANDLE,
                               KeyHandle,
                               NULL);
    Status = ZwCreateKey(&SubKeyHandle,
                         KEY_READ | KEY_WRITE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         NULL);
    ZwClose(KeyHandle);
    if (!NT_SUCCESS(Status)) return Status;

    /* Create the key value based on the device and function number */
    swprintf(Buffer,
             L"DEV_%02x&FUN_%02x",
             DeviceExtension->Slot.u.bits.DeviceNumber,
             DeviceExtension->Slot.u.bits.FunctionNumber);
    RtlInitUnicodeString(&KeyValue, Buffer);

    /* Set the value data (the PCI BIOS configuration header) */
    Status = ZwSetValueKey(SubKeyHandle,
                           &KeyValue,
                           0,
                           REG_BINARY,
                           PciData,
                           PCI_COMMON_HDR_LENGTH);
    ZwClose(SubKeyHandle);
    return Status;
}

UCHAR
NTAPI
PciReadDeviceCapability(IN PPCI_PDO_EXTENSION DeviceExtension,
                        IN UCHAR Offset,
                        IN ULONG CapabilityId,
                        OUT PPCI_CAPABILITIES_HEADER Buffer,
                        IN ULONG Length)
{
    ULONG CapabilityCount = 0;

    /* If the device has no capabilility list, fail */
    if (!Offset) return 0;

    /* Validate a PDO with capabilities, a valid buffer, and a valid length */
    ASSERT(DeviceExtension->ExtensionType == PciPdoExtensionType);
    ASSERT(DeviceExtension->CapabilitiesPtr != 0);
    ASSERT(Buffer);
    ASSERT(Length >= sizeof(PCI_CAPABILITIES_HEADER));

    /* Loop all capabilities */
    while (Offset)
    {
        /* Make sure the pointer is spec-aligned and spec-sized */
        ASSERT((Offset >= PCI_COMMON_HDR_LENGTH) && ((Offset & 0x3) == 0));

        /* Read the capability header */
        PciReadDeviceConfig(DeviceExtension,
                            Buffer,
                            Offset,
                            sizeof(PCI_CAPABILITIES_HEADER));

        /* Check if this is the capability being looked up */
        if ((Buffer->CapabilityID == CapabilityId) || !(CapabilityId))
        {
            /* Check if was at a valid offset and length */
            if ((Offset) && (Length > sizeof(PCI_CAPABILITIES_HEADER)))
            {
                /* Sanity check */
                ASSERT(Length <= (sizeof(PCI_COMMON_CONFIG) - Offset));

                /* Now read the whole capability data into the buffer */
                PciReadDeviceConfig(DeviceExtension,
                                    (PVOID)((ULONG_PTR)Buffer +
                                    sizeof(PCI_CAPABILITIES_HEADER)),
                                    Offset + sizeof(PCI_CAPABILITIES_HEADER),
                                    Length - sizeof(PCI_CAPABILITIES_HEADER));
            }

            /* Return the offset where the capability was found */
            return Offset;
        }

        /* Try the next capability instead */
        CapabilityCount++;
        Offset = Buffer->Next;

        /* There can't be more than 48 capabilities (256 bytes max) */
        if (CapabilityCount > 48)
        {
            /* Fail, since this is basically a broken PCI device */
            DPRINT1("PCI device %p capabilities list is broken.\n", DeviceExtension);
            return 0;
        }
    }

    /* Capability wasn't found, fail */
    return 0;
}

BOOLEAN
NTAPI
PciCanDisableDecodes(IN PPCI_PDO_EXTENSION DeviceExtension,
                     IN PPCI_COMMON_HEADER Config,
                     IN ULONGLONG HackFlags,
                     IN BOOLEAN ForPowerDown)
{
    UCHAR BaseClass, SubClass;
    BOOLEAN IsVga;

    /* Is there a device extension or should the PCI header be used? */
    if (DeviceExtension)
    {
        /* Never disable decodes for a debug PCI Device */
        if (DeviceExtension->OnDebugPath) return FALSE;

        /* Hack flags will be obtained from the extension, not the caller */
        ASSERT(HackFlags == 0);

        /* Get hacks and classification from the device extension */
        HackFlags = DeviceExtension->HackFlags;
        SubClass = DeviceExtension->SubClass;
        BaseClass = DeviceExtension->BaseClass;
    }
    else
    {
        /* There must be a PCI header, go read the classification information */
        ASSERT(Config != NULL);
        BaseClass = Config->BaseClass;
        SubClass = Config->SubClass;
    }

    /* Check for hack flags that prevent disabling the decodes */
    if (HackFlags & (PCI_HACK_PRESERVE_COMMAND |
                     PCI_HACK_CB_SHARE_CMD_BITS |
                     PCI_HACK_DONT_DISABLE_DECODES))
    {
        /* Don't do it */
        return FALSE;
    }

    /* Is this a VGA adapter? */
    if ((BaseClass == PCI_CLASS_DISPLAY_CTLR) &&
        (SubClass == PCI_SUBCLASS_VID_VGA_CTLR))
    {
        /* Never disable decodes if this is for power down */
        return ForPowerDown;
    }

    /* Check for legacy devices */
    if (BaseClass == PCI_CLASS_PRE_20)
    {
        /* Never disable video adapter cards if this is for power down */
        if (SubClass == PCI_SUBCLASS_PRE_20_VGA) return ForPowerDown;
    }
    else if (BaseClass == PCI_CLASS_DISPLAY_CTLR)
    {
        /* Never disable VGA adapters if this is for power down */
        if (SubClass == PCI_SUBCLASS_VID_VGA_CTLR) return ForPowerDown;
    }
    else if (BaseClass == PCI_CLASS_BRIDGE_DEV)
    {
        /* Check for legacy bridges */
        if ((SubClass == PCI_SUBCLASS_BR_ISA) ||
            (SubClass == PCI_SUBCLASS_BR_EISA) ||
            (SubClass == PCI_SUBCLASS_BR_MCA) ||
            (SubClass == PCI_SUBCLASS_BR_HOST) ||
            (SubClass == PCI_SUBCLASS_BR_OTHER))
        {
            /* Never disable these */
            return FALSE;
        }
        else if ((SubClass == PCI_SUBCLASS_BR_PCI_TO_PCI) ||
                 (SubClass == PCI_SUBCLASS_BR_CARDBUS))
        {
            /* This is a supported bridge, but does it have a VGA card? */
            if (!DeviceExtension)
            {
                /* Read the bridge control flag from the PCI header */
                IsVga = Config->u.type1.BridgeControl & PCI_ENABLE_BRIDGE_VGA;
            }
            else
            {
                /* Read the cached flag in the device extension */
                IsVga = DeviceExtension->Dependent.type1.VgaBitSet;
            }

            /* Never disable VGA adapters if this is for power down */
            if (IsVga) return ForPowerDown;
        }
    }

    /* Finally, never disable decodes if there's no power management */
    return !(HackFlags & PCI_HACK_NO_PM_CAPS);
}

ULONG_PTR
NTAPI
PciExecuteCriticalSystemRoutine(IN ULONG_PTR IpiContext)
{
    PPCI_IPI_CONTEXT Context = (PPCI_IPI_CONTEXT)IpiContext;

    /* Check if the IPI is already running */
    if (!InterlockedDecrement(&Context->RunCount))
    {
        /* Nope, this is the first instance, so execute the IPI function */
        Context->Function(Context->PdoExtension, Context->Context);

        /* Notify anyone that was spinning that they can stop now */
        Context->Barrier = 0;
    }
    else
    {
        /* Spin until it has finished running */
        while (Context->Barrier);
    }

    /* Done */
    return 0;
}

BOOLEAN
NTAPI
PciIsSlotPresentInParentMethod(IN PPCI_PDO_EXTENSION PdoExtension,
                               IN ULONG Method)
{
    BOOLEAN FoundSlot;
    PACPI_METHOD_ARGUMENT Argument;
    ACPI_EVAL_INPUT_BUFFER InputBuffer;
    PACPI_EVAL_OUTPUT_BUFFER OutputBuffer;
    ULONG i, Length;
    NTSTATUS Status;
    PAGED_CODE();

    /* Assume slot is not part of the parent method */
    FoundSlot = FALSE;

    /* Allocate a 2KB buffer for the method return parameters */
    Length = sizeof(ACPI_EVAL_OUTPUT_BUFFER) + 2048;
    OutputBuffer = ExAllocatePoolWithTag(PagedPool, Length, 'BicP');
    if (OutputBuffer)
    {
        /* Clear out the output buffer */
        RtlZeroMemory(OutputBuffer, Length);

        /* Initialize the input buffer with the method requested */
        InputBuffer.Signature = 0;
        *(PULONG)InputBuffer.MethodName = Method;
        InputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;

        /* Send it to the ACPI driver */
        Status = PciSendIoctl(PdoExtension->ParentFdoExtension->PhysicalDeviceObject,
                              IOCTL_ACPI_EVAL_METHOD,
                              &InputBuffer,
                              sizeof(ACPI_EVAL_INPUT_BUFFER),
                              OutputBuffer,
                              Length);
        if (NT_SUCCESS(Status))
        {
            /* Scan all output arguments */
            for (i = 0; i < OutputBuffer->Count; i++)
            {
                /* Make sure it's an integer */
                Argument = &OutputBuffer->Argument[i];
                if (Argument->Type != ACPI_METHOD_ARGUMENT_INTEGER) continue;

                /* Check if the argument matches this PCI slot structure */
                if (Argument->Argument == ((PdoExtension->Slot.u.bits.DeviceNumber) |
                                           ((PdoExtension->Slot.u.bits.FunctionNumber) << 16)))
                {
                    /* This slot has been found, return it */
                    FoundSlot = TRUE;
                    break;
                }
            }
        }

        /* Finished with the buffer, free it */
        ExFreePoolWithTag(OutputBuffer, 0);
    }

    /* Return if the slot was found */
    return FoundSlot;
}

VOID
NTAPI
PciDecodeEnable(IN PPCI_PDO_EXTENSION PdoExtension,
                IN BOOLEAN Enable,
                OUT PUSHORT Command)
{
    USHORT CommandValue;

    /*
     * If decodes are being disabled, make sure it's allowed, and in both cases,
     * make sure that a hackflag isn't preventing touching the decodes at all.
     */
    if (((Enable) || (PciCanDisableDecodes(PdoExtension, 0, 0, 0))) &&
        !(PdoExtension->HackFlags & PCI_HACK_PRESERVE_COMMAND))
    {
        /* Did the caller already have a command word? */
        if (Command)
        {
            /* Use the caller's */
            CommandValue = *Command;
        }
        else
        {
            /* Otherwise, read the current command */
            PciReadDeviceConfig(PdoExtension,
                                &Command,
                                FIELD_OFFSET(PCI_COMMON_HEADER, Command),
                                sizeof(USHORT));
        }

        /* Turn off decodes by default */
        CommandValue &= ~(PCI_ENABLE_IO_SPACE |
                          PCI_ENABLE_MEMORY_SPACE |
                          PCI_ENABLE_BUS_MASTER);

        /* If requested, enable the decodes that were enabled at init time */
        if (Enable) CommandValue |= PdoExtension->CommandEnables &
                                    (PCI_ENABLE_IO_SPACE |
                                     PCI_ENABLE_MEMORY_SPACE |
                                     PCI_ENABLE_BUS_MASTER);

        /* Update the command word */
        PciWriteDeviceConfig(PdoExtension,
                             &CommandValue,
                             FIELD_OFFSET(PCI_COMMON_HEADER, Command),
                             sizeof(USHORT));
    }
}

/* EOF */

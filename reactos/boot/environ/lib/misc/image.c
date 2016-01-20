/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/misc/image.c
 * PURPOSE:         Boot Library Image Routines
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"
#include <bcd.h>

/* DATA VARIABLES ************************************************************/

ULONG IapAllocatedTableEntries;
ULONG IapTableEntries;
PVOID* IapImageTable;

/* FUNCTIONS *****************************************************************/

NTSTATUS
ImgpGetFileSize (
    _In_ PBL_IMG_FILE File,
    _Out_ PULONG FileSize
    )
{
    NTSTATUS Status;
    ULONG Size;
    BL_FILE_INFORMATION FileInformation;

    /* Check if the file was memory mapped */
    if (File->Flags & BL_IMG_MEMORY_FILE)
    {
        /* Just read the size of the mapping */
        Size = File->FileSize;
    }
    else
    {
        /* Do file I/O to get the file size */
        Status = BlFileGetInformation(File->FileId,
                                      &FileInformation);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        /* We only support files less than 4GB in the Image Mapped */
        Size = FileInformation.Size;
        if (FileInformation.Size > ULONG_MAX)
        {
            return STATUS_NOT_SUPPORTED;
        }
    }

    /* Return the size and success */
    *FileSize = Size;
    return STATUS_SUCCESS;
}

NTSTATUS
ImgpReadAtFileOffset (
    _In_ PBL_IMG_FILE File,
    _In_ ULONG Size,
    _In_ ULONGLONG ByteOffset,
    _In_ PVOID Buffer,
    _Out_ PULONG BytesReturned
    )
{
    NTSTATUS Status;

    /* Check what if this is a mapped file or not */
    if (File->Flags & BL_IMG_MEMORY_FILE)
    {
        /* Check if the boundaries are within the file size */
        if ((ByteOffset + Size) <= File->FileSize)
        {
            /* Yep, copy into the caller-supplied buffer */
            RtlCopyMemory(Buffer,
                          (PVOID)((ULONG_PTR)File->BaseAddress + (ULONG_PTR)ByteOffset),
                          Size);

            /* If caller wanted to know, return the size copied */
            if (BytesReturned)
            {
                *BytesReturned = Size;
            }

            /* All good */
            Status = STATUS_SUCCESS;
        }
        else
        {
            /* Doesn't fit */
            Status = STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        /* Issue the file I/O instead */
        Status = BlFileReadAtOffsetEx(File->FileId,
                                      Size,
                                      ByteOffset,
                                      Buffer,
                                      BytesReturned,
                                      0);
    }

    /* Return the final status */
    return Status;
}

NTSTATUS
ImgpOpenFile (
    _In_ ULONG DeviceId,
    _In_ PWCHAR FileName,
    _In_ ULONG Flags,
    _Out_ PBL_IMG_FILE NewFile
    )
{
    NTSTATUS Status;
    ULONG FileSize;
    ULONGLONG RemoteFileSize;
    PVOID RemoteFileAddress;
    ULONG FileId;

    /* First, try to see if BD has this file remotely */
    Status = BlBdPullRemoteFile(FileName,
                                &RemoteFileAddress,
                                &RemoteFileSize);
    if (NT_SUCCESS(Status))
    {
        /* Yep, get the file size and make sure it's < 4GB */
        FileSize = RemoteFileSize;
        if (RemoteFileSize <= ULONG_MAX)
        {
            /* Remember this is a memory mapped remote file */
            NewFile->Flags |= (BL_IMG_MEMORY_FILE | BL_IMG_REMOTE_FILE);
            NewFile->FileSize = FileSize;
            NewFile->BaseAddress = RemoteFileAddress;
            goto Quickie;
        }
    }

    /* Use File I/O instead */
    Status = BlFileOpen(DeviceId,
                        FileName,
                        BL_FILE_READ_ACCESS,
                        &FileId);
    if (!NT_SUCCESS(Status))
    {
        /* Bail out on failure */
        return Status;
    }

    /* Make sure nobody thinks this is a memory file */
    NewFile->Flags &= ~BL_IMG_MEMORY_FILE;
    NewFile->FileId = FileId;

Quickie:
    /* Set common data for both memory and I/O based file */
    NewFile->Flags |= BL_IMG_VALID_FILE;
    NewFile->FileName = FileName;
    return Status;
}

NTSTATUS
ImgpCloseFile (
    _In_ PBL_IMG_FILE File
    )
{
    NTSTATUS Status;

    /* Make sure this is a valid file, otherwise no-op */
    Status = STATUS_SUCCESS;
    if (File->Flags & BL_IMG_VALID_FILE)
    {
        /* Is this a memory mapped file? */
        if (!(File->Flags & BL_IMG_MEMORY_FILE))
        {
            /* Nope, close the file handle */
            return BlFileClose(File->FileId);
        }

        /* Is this a remote file? */
        if (File->Flags & BL_IMG_REMOTE_FILE)
        {
            /* Then only free the memory in that scenario */
            EfiPrintf(L"TODO\r\n");
            //return MmPapFreePages(File->BaseAddress, TRUE);
        }
    }

    /* Return the final status */
    return Status;
}

NTSTATUS
BlImgUnallocateImageBuffer (
    _In_ PVOID ImageBase,
    _In_ ULONG ImageSize,
    _In_ ULONG ImageFlags
    )
{
    EfiPrintf(L"leaking the shit out of %p\r\n", ImageBase);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
BlImgAllocateImageBuffer (
    _Inout_ PVOID* ImageBuffer,
    _In_ ULONG MemoryType,
    _In_ ULONGLONG ImageSize,
    _In_ ULONG Flags
    )
{
    ULONG Attributes;
    ULONGLONG Pages, Size;
    PVOID MappedBase, CurrentBuffer;
    NTSTATUS Status;
    PHYSICAL_ADDRESS PhysicalAddress;

    /* Read and reset the current buffer address */
    CurrentBuffer = *ImageBuffer;
    *ImageBuffer = NULL;

    /* Align the image size to page */
    Size = ROUND_TO_PAGES(ImageSize);

    /* Not sure what this attribute does yet */
    Attributes = 0;
    if (Flags & BL_LOAD_IMG_UNKNOWN_BUFFER_FLAG)
    {
        Attributes = 0x10000;
    }

    /* Check if the caller wants a virtual buffer */
    if (Flags & BL_LOAD_IMG_VIRTUAL_BUFFER)
    {
        /* Set the physical address to the current buffer */
        PhysicalAddress.QuadPart = (ULONG_PTR)CurrentBuffer;
        Pages = Size >> PAGE_SHIFT;

        /* Allocate the physical pages */
        Status = BlMmAllocatePhysicalPages(&PhysicalAddress,
                                           Pages,
                                           MemoryType,
                                           Attributes,
                                           0);
        if (!NT_SUCCESS(Status))
        {
            /* If that failed, remove allocation attributes */
            PhysicalAddress.QuadPart = 0;
            Attributes &= ~BlMemoryValidAllocationAttributeMask,
            Status = BlMmAllocatePhysicalPages(&PhysicalAddress,
                                               Pages,
                                               MemoryType,
                                               Attributes,
                                               0);
        }

        /* Check if either attempts succeeded */
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        /* Now map the physical buffer at the address requested */
        MappedBase = (PVOID)PhysicalAddress.LowPart;
        Status = BlMmMapPhysicalAddressEx(&MappedBase,
                                          BlMemoryFixed,
                                          Size,
                                          PhysicalAddress);
        if (!NT_SUCCESS(Status))
        {
            /* Free on failure if needed */
            BlMmFreePhysicalPages(PhysicalAddress);
            return Status;
        }
    }
    else
    {
        /* Otherwise, allocate raw physical pages */
        MappedBase = CurrentBuffer;
        Pages = Size >> PAGE_SHIFT;
        Status = MmPapAllocatePagesInRange(&MappedBase,
                                           MemoryType,
                                           Pages,
                                           Attributes,
                                           0,
                                           NULL,
                                           0);
        if (!NT_SUCCESS(Status))
        {
            /* If that failed, try without allocation attributes */
            MappedBase = NULL;
            Attributes &= ~BlMemoryValidAllocationAttributeMask,
            Status = MmPapAllocatePagesInRange(&MappedBase,
                                               MemoryType,
                                               Pages,
                                               Attributes,
                                               0,
                                               NULL,
                                               0);
        }

        /* Check if either attempts succeeded */
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    /* Success path, returned allocated address */
    *ImageBuffer = MappedBase;
    return STATUS_SUCCESS;
}

NTSTATUS
BlImgLoadImageWithProgress2 (
    _In_ ULONG DeviceId,
    _In_ BL_MEMORY_TYPE MemoryType,
    _In_ PWCHAR FileName,
    _Inout_ PVOID* MappedBase,
    _Inout_ PULONG MappedSize,
    _In_ ULONG ImageFlags,
    _In_ BOOLEAN ShowProgress,
    _Out_opt_ PUCHAR* HashBuffer,
    _Out_opt_ PULONG HashSize
    )
{
    NTSTATUS Status;
    PVOID BaseAddress, Buffer;
    ULONG RemainingLength, CurrentSize, ImageSize, ReadSize;
    BOOLEAN ComputeSignature, ComputeHash, Completed;
    BL_IMG_FILE FileHandle;
    ULONGLONG ByteOffset;
    PHYSICAL_ADDRESS PhysicalAddress;

    /* Initialize variables */
    BaseAddress = 0;
    ImageSize = 0;
    Completed = FALSE;
    RtlZeroMemory(&FileHandle, sizeof(FileHandle));

    /* Check for missing parameters */
    if (!MappedBase)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }
    if (!FileName)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }
    if (!MappedSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Check if the image buffer is being provided */
    if (ImageFlags & BL_LOAD_IMG_EXISTING_BUFFER)
    {
        /* An existing base must already exist */
        if (!(*MappedBase))
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Quickie;
        }
    }

    /* Check of a hash is being requested */
    if (ImageFlags & BL_LOAD_IMG_COMPUTE_HASH)
    {
        /* Make sure we can return the hash */
        if (!HashBuffer)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Quickie;
        }
        if (!HashSize)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Quickie;
        }
    }

    /* Check for invalid combination of parameters */
    if ((ImageFlags & BL_LOAD_IMG_COMPUTE_HASH) && (ImageFlags & 0x270))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Initialize hash if requested by caller */
    if (HashBuffer)
    {
        *HashBuffer = 0;
    }

    /* Do the same for the hash size */
    if (HashSize)
    {
        *HashSize = 0;
    }

    /* Open the image file */
    Status = ImgpOpenFile(DeviceId, FileName, DeviceId, &FileHandle);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Error opening file: %lx\r\n", Status);
        goto Quickie;
    }

    /* Get the size of the image */
    Status = ImgpGetFileSize(&FileHandle, &ImageSize);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Error getting file size: %lx\r\n", Status);
        goto Quickie;
    }

    /* Read the current base address */
    BaseAddress = *MappedBase;
    if (ImageFlags & BL_LOAD_IMG_EXISTING_BUFFER)
    {
        /* Check if the current buffer is too small */
        if (*MappedSize < ImageSize)
        {
            /* Return the required size of the buffer */
            *MappedSize = ImageSize;
            Status = STATUS_BUFFER_TOO_SMALL;
        }
    }
    else
    {
        /* A buffer was not provided, allocate one ourselves */
        Status = BlImgAllocateImageBuffer(&BaseAddress,
                                          MemoryType,
                                          ImageSize,
                                          ImageFlags);
    }

    /* Bail out if allocation failed */
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Set the initial byte offset and length to read */
    RemainingLength = ImageSize;
    ByteOffset = 0;
    Buffer = BaseAddress;

    /* Update the initial progress */
    Completed = FALSE;
    if (ShowProgress)
    {
        BlUtlUpdateProgress(0, &Completed);
        ShowProgress &= (Completed != 0) - 1;
    }

    /* Set the chunk size for each read */
    ReadSize = 0x100000;
    if (ReadSize > ImageSize)
    {
        ReadSize = ImageSize;
    }

    /* Check if we should compute hash and/or signatures */
    ComputeSignature = ImageFlags & BL_LOAD_IMG_COMPUTE_SIGNATURE;
    if ((ComputeSignature) || (ImageFlags & BL_LOAD_IMG_COMPUTE_HASH))
    {
        ComputeHash = TRUE;
        // todo: crypto is hard
    }

    /* Begin the read loop */
    while (RemainingLength)
    {
        /* Check if we've got more than a chunk left to read */
        if (RemainingLength > ReadSize)
        {
            /* Read a chunk*/
            CurrentSize = ReadSize;
        }
        else
        {
            /* Read only what's left */
            CurrentSize = RemainingLength;
        }

        /* Read the chunk */
        Status = ImgpReadAtFileOffset(&FileHandle,
                                      CurrentSize,
                                      ByteOffset,
                                      Buffer,
                                      0);
        if (!NT_SUCCESS(Status))
        {
            goto Quickie;
        }

        /* Check if we need to compute the hash of this chunk */
        if (ComputeHash)
        {
            // todo: crypto is hard
        }

        /* Update our position and read information */
        Buffer = (PVOID)((ULONG_PTR)Buffer + CurrentSize);
        RemainingLength -= CurrentSize;
        ByteOffset += CurrentSize;

        /* Check if we should update the progress bar */
        if (ShowProgress)
        {
            /* Compute new percentage completed, check if we're done */
            BlUtlUpdateProgress(100 - 100 * RemainingLength / ImageSize,
                                &Completed);
            ShowProgress &= (Completed != 0) - 1;
        }
    }

    /* Is the read fully complete? We need to finalize the hash if requested */
    if (ComputeHash != RemainingLength)
    {
        // todo: CRYPTO IS HARD
    }

    /* Success path, return back the buffer and the size of the image */
    *MappedBase = BaseAddress;
    *MappedSize = ImageSize;

Quickie:
    /* Close the file handle */
    ImgpCloseFile(&FileHandle);

    /* Check if we failed and had allocated a buffer */
    if (!(NT_SUCCESS(Status)) &&
        (BaseAddress) &&
        !(ImageFlags & BL_LOAD_IMG_EXISTING_BUFFER))
    {
        /* Check what kind of buffer we had allocated */
        if (ImageFlags & BL_LOAD_IMG_VIRTUAL_BUFFER)
        {
            /* Unmap and free the virtual buffer */
            PhysicalAddress.QuadPart = (ULONG_PTR)BaseAddress;
            BlMmUnmapVirtualAddressEx(BaseAddress, ImageSize);
            BlMmFreePhysicalPages(PhysicalAddress);
        }
        else
        {
            /* Free the physical buffer */
            //MmPapFreePages(VirtualAddress, 1);
            EfiPrintf(L"Leaking memory\r\n");
        }
    }

    /* If we hadn't gotten to 100% yet, do it now */
    if (ShowProgress)
    {
        BlUtlUpdateProgress(100, &Completed);
    }

    /* Return the final status */
    return Status;
}

PIMAGE_SECTION_HEADER 
BlImgFindSection (
    _In_ PVOID ImageBase,
    _In_ ULONG ImageSize
    )
{
    PIMAGE_SECTION_HEADER FoundSection;
    ULONG i;
    PIMAGE_SECTION_HEADER SectionHeader;
    PIMAGE_NT_HEADERS NtHeader;
    NTSTATUS Status;

    /* Assume failure */
    FoundSection = NULL;

    /* Make sure the image is valid */
    Status = RtlImageNtHeaderEx(0, ImageBase, ImageSize, &NtHeader);
    if (NT_SUCCESS(Status))
    {
        /* Get the first section and loop through them all */
        SectionHeader = IMAGE_FIRST_SECTION(NtHeader);
        for (i = 0; i < NtHeader->FileHeader.NumberOfSections; i++)
        {
            /* Check if this is the resource section */
            if (!_stricmp((PCCH)SectionHeader->Name, ".rsrc"))
            {
                /* Yep, we're done */
                FoundSection = SectionHeader;
                break;
            }

            /* Nope, keep going */
            SectionHeader++;
        }
    }

    /* Return the matching section */
    return FoundSection;
}

VOID
BlImgQueryCodeIntegrityBootOptions (
    _In_ PBL_LOADED_APPLICATION_ENTRY ApplicationEntry,
    _Out_ PBOOLEAN IntegrityChecksDisabled, 
    _Out_ PBOOLEAN TestSigning
    )
{
    
    NTSTATUS Status;
    BOOLEAN Value;

    /* Check if /DISABLEINTEGRITYCHECKS is on */
    Status = BlGetBootOptionBoolean(ApplicationEntry->BcdData,
                                    BcdLibraryBoolean_DisableIntegrityChecks,
                                    &Value);
    *IntegrityChecksDisabled = NT_SUCCESS(Status) && (Value);

    /* Check if /TESTSIGNING is on */
    Status = BlGetBootOptionBoolean(ApplicationEntry->BcdData,
                                    BcdLibraryBoolean_AllowPrereleaseSignatures,
                                    &Value);
    *TestSigning = NT_SUCCESS(Status) && (Value);
}

NTSTATUS
BlImgUnLoadImage (
    _In_ PVOID ImageBase,
    _In_ ULONG ImageSize,
    _In_ ULONG ImageFlags
    )
{
    /* Check for missing parameters */
    if (!(ImageSize) || !(ImageBase))
    {
        /* Bail out */
        return STATUS_INVALID_PARAMETER;
    }

    /* Unallocate the image buffer */
    return BlImgUnallocateImageBuffer(ImageBase, ImageSize, ImageFlags);
}

NTSTATUS
ImgpLoadPEImage (
    _In_ PBL_IMG_FILE ImageFile,
    _In_ BL_MEMORY_TYPE MemoryType,
    _Inout_ PVOID* ImageBase,
    _Out_ PULONG ImageSize,
    _Inout_opt_ PVOID Hash,
    _In_ ULONG Flags
    )
{
    NTSTATUS Status;
    ULONG FileSize, HeaderSize;
    PVOID ImageBuffer;
    BL_IMG_FILE LocalFileBuffer;
    PBL_IMG_FILE LocalFile;
    PVOID VirtualAddress;
    ULONGLONG VirtualSize;
    PHYSICAL_ADDRESS PhysicalAddress;
    PIMAGE_NT_HEADERS NtHeaders;

    /* Initialize locals */
    LocalFile = NULL;
    ImageBuffer = NULL;
    FileSize = 0;

    /* Get the size of the image */
    Status = ImgpGetFileSize(ImageFile, &FileSize);
    if (!NT_SUCCESS(Status))
    {
        return STATUS_FILE_INVALID;
    }

    /* Allocate a flat buffer for it */
    Status = BlImgAllocateImageBuffer(&ImageBuffer, BlLoaderData, FileSize, 0);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Read the whole file flat for now */
    Status = ImgpReadAtFileOffset(ImageFile, FileSize, 0, ImageBuffer, NULL);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Build a local file handle */
    LocalFile = &LocalFileBuffer;
    LocalFileBuffer.FileName = ImageFile->FileName;
    LocalFileBuffer.Flags = BL_IMG_MEMORY_FILE | BL_IMG_VALID_FILE;
    LocalFileBuffer.BaseAddress = ImageBuffer;
    LocalFileBuffer.FileSize = FileSize;

    /* Get the NT headers of the file */
    Status = RtlImageNtHeaderEx(0, ImageBuffer, FileSize, &NtHeaders);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Check if we should validate the machine type */
    if (Flags & BL_LOAD_PE_IMG_CHECK_MACHINE)
    {
        /* Is it different than our current machine type? */
#if _M_AMD64
        if (NtHeaders->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64)
#else
        if (NtHeaders->FileHeader.Machine != IMAGE_FILE_MACHINE_I386)
#endif
        {
            /* Is it x86 (implying we are x64) ? */
            if (NtHeaders->FileHeader.Machine == IMAGE_FILE_MACHINE_I386)
            {
                /* Return special error code */
                Status = STATUS_INVALID_IMAGE_WIN_32;
            }
            else if (NtHeaders->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
            {
                /* Otherwise, it's x64 but we are x86 */
                Status = STATUS_INVALID_IMAGE_WIN_64;
            }
            else
            {
                /* Or it's ARM or something... */
                Status = STATUS_INVALID_IMAGE_FORMAT;
            }

            /* Return with the distinguished error code */
            goto Quickie;
        }
    }

    /* Check if we should validate the subsystem */
    if (Flags & BL_LOAD_PE_IMG_CHECK_SUBSYSTEM)
    {
        /* It must be a Windows boot Application */
        if (NtHeaders->OptionalHeader.Subsystem !=
            IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION)
        {
            Status = STATUS_INVALID_IMAGE_FORMAT;
            goto Quickie;
        }
    }

    /* Check if we should validate the /INTEGRITYCHECK flag */
    if (Flags & BL_LOAD_PE_IMG_CHECK_FORCED_INTEGRITY)
    {
        /* Check if it's there */
        if (!(NtHeaders->OptionalHeader.DllCharacteristics &
              IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY))
        {
            /* Nope, fail otherwise */
            Status = STATUS_INVALID_IMAGE_FORMAT;
            goto Quickie;
        }
    }

    /* Check if we should compute the image hash */
    if ((Flags & BL_LOAD_PE_IMG_COMPUTE_HASH) || (Hash))
    {
        EfiPrintf(L"No hash support\r\n");
    }

    /* Read the current base address, if any */
    VirtualAddress = *ImageBase;

    /* Get the virtual size of the image */
    VirtualSize = NtHeaders->OptionalHeader.SizeOfImage;

    /* Safely align the virtual size to a page */
    Status = RtlULongLongAdd(VirtualSize,
                             PAGE_SIZE - 1,
                             &VirtualSize);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }
    VirtualSize = ALIGN_DOWN_BY(VirtualSize, PAGE_SIZE);

    /* Make sure the image isn't larger than 4GB */
    if (VirtualSize > ULONG_MAX)
    {
        Status = STATUS_INVALID_IMAGE_FORMAT;
        goto Quickie;
    }

    /* Check if we have a buffer already */
    if (Flags & BL_LOAD_IMG_EXISTING_BUFFER)
    {
        /* Check if it's too small */
        if (*ImageSize < VirtualSize)
        {
            /* Fail, letting the caller know how big to make it */
            *ImageSize = VirtualSize;
            Status = STATUS_BUFFER_TOO_SMALL;
        }
    }
    else
    {
        /* Allocate the buffer with the flags and type the caller wants */
        Status = BlImgAllocateImageBuffer(&VirtualAddress,
                                          MemoryType,
                                          VirtualSize,
                                          Flags);
    }

    /* Bail out if allocation failed, or existing buffer is too small */
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Read the size of the headers */
    HeaderSize = NtHeaders->OptionalHeader.SizeOfHeaders;
    if (VirtualSize < HeaderSize)
    {
        /* Bail out if they're bigger than the image! */
        Status = STATUS_INVALID_IMAGE_FORMAT;
        goto Quickie;
    }

    /* Now read the header into the buffer */
    Status = ImgpReadAtFileOffset(LocalFile, HeaderSize, 0, VirtualAddress, NULL);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Get the NT headers of the file */
    Status = RtlImageNtHeaderEx(0, VirtualAddress, HeaderSize, &NtHeaders);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    EfiPrintf(L"MORE PE TODO: %lx\r\n", NtHeaders->OptionalHeader.AddressOfEntryPoint);
    EfiStall(100000000);

Quickie:
    /* Check if we had an image buffer allocated */
    if ((ImageBuffer) && (FileSize))
    {
        /* Free it */
        BlImgUnallocateImageBuffer(ImageBuffer, FileSize, 0);
    }

    /* Check if we had a local file handle */
    if (LocalFile)
    {
        /* Close it */
        ImgpCloseFile(LocalFile);
    }

    /* Check if this is the failure path */
    if (!NT_SUCCESS(Status))
    {
        /* Check if we had started mapping in the image already */
        if ((VirtualAddress) && !(Flags & BL_LOAD_PE_IMG_EXISTING_BUFFER))
        {
            /* Into a virtual buffer? */
            if (Flags & BL_LOAD_PE_IMG_VIRTUAL_BUFFER)
            {
                /* Unmap and free it */
                BlMmUnmapVirtualAddressEx(VirtualAddress, VirtualSize);
                PhysicalAddress.QuadPart = (ULONG_PTR)VirtualAddress;
                BlMmFreePhysicalPages(PhysicalAddress);
            }
            else
            {
                /* Into a physical buffer -- free it */
                EfiPrintf(L"Leaking physical pages\r\n");
               // MmPapFreePages(VirtualAddress, TRUE);
            }
        }
    }

    /* Return back to caller */
    return Status;
}

NTSTATUS
BlImgLoadPEImageEx (
    _In_ ULONG DeviceId,
    _In_ BL_MEMORY_TYPE MemoryType,
    _In_ PWCHAR Path,
    _Out_ PVOID* ImageBase,
    _Out_ PULONG ImageSize,
    _Out_ PVOID Hash,
    _In_ ULONG Flags
    )
{
    BL_IMG_FILE ImageFile;
    NTSTATUS Status;

    /* Initialize the image file structure */
    ImageFile.Flags = 0;
    ImageFile.FileName = NULL;

    /* Check if the required parameter are missing */
    if (!(ImageBase) || !(Path))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* If we are loading a pre-allocated image, make sure we have it */
    if ((Flags & BL_LOAD_IMG_EXISTING_BUFFER) && (!(*ImageBase) || !(ImageSize)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Load the file from disk */
    Status = ImgpOpenFile(DeviceId, Path, 0, &ImageFile);
    if (NT_SUCCESS(Status))
    {
        /* If that worked, do the PE parsing */
        Status = ImgpLoadPEImage(&ImageFile,
                                 MemoryType,
                                 ImageBase,
                                 ImageSize,
                                 Hash,
                                 Flags);
    }

    /* Close the image file and return back to caller */
    ImgpCloseFile(&ImageFile);
    return Status;
}

NTSTATUS
BlImgLoadBootApplication (
    _In_ PBL_LOADED_APPLICATION_ENTRY BootEntry,
    _Out_ PULONG AppHandle
    )
{
    NTSTATUS Status;
    PULONGLONG AllowedList;
    ULONGLONG AllowedCount;
    ULONG i, DeviceId, ImageSize, Flags, ListSize;
    LARGE_INTEGER Frequency;
    PVOID UnlockCode, ImageBase;
    PBL_DEVICE_DESCRIPTOR Device, BitLockerDevice;
    PWCHAR Path;
    PBL_APPLICATION_ENTRY AppEntry;
    PBL_IMG_FILE ImageFile;
    BOOLEAN DisableIntegrity, TestSigning;
    UCHAR Hash[64];
    PBL_IMAGE_APPLICATION_ENTRY ImageAppEntry;

    /* Initialize all locals */
    BitLockerDevice = NULL;
    UnlockCode = NULL;
    ImageFile = NULL;
    DeviceId = -1;
    Device = NULL;
    ImageAppEntry = NULL;
    Path = NULL;
    ImageSize = 0;
    ImageBase = NULL;

    /* Check for "allowed in-memory settings" */
    Status = BlpGetBootOptionIntegerList(BootEntry->BcdData,
                                         BcdLibraryIntegerList_AllowedInMemorySettings,
                                         &AllowedList,
                                         &AllowedCount,
                                         TRUE);
    if (Status == STATUS_SUCCESS)
    {
        /* Loop through the list of allowed setting */
        for (i = 0; i < AllowedCount; i++)
        {
            /* Find the super undocumented one */
            if (AllowedList[i] == BcdLibraryInteger_UndocumentedMagic)
            {
                /* If it's present, append the current perf frequence to it */
                BlTimeQueryPerformanceCounter(&Frequency);
                BlAppendBootOptionInteger(BootEntry,
                                          BcdLibraryInteger_UndocumentedMagic,
                                          Frequency.QuadPart);
            }
        }
    }

#if BL_BITLOCKER_SUPPORT
    /* Do bitlocker stuff */
    Status = BlFveSecureBootUnlockBootDevice(BootEntry, &BitLockerDevice, &UnlockCode);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }
#endif

    /* Get the device on which this application is on*/
    Status = BlGetBootOptionDevice(BootEntry->BcdData,
                                   BcdLibraryDevice_ApplicationDevice,
                                   &Device,
                                   NULL);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Get the path of the application */
    Status = BlGetBootOptionString(BootEntry->BcdData,
                                   BcdLibraryString_ApplicationPath,
                                   &Path);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Open the device */
    Status = BlpDeviceOpen(Device,
                           BL_DEVICE_READ_ACCESS,
                           0,
                           &DeviceId);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Check for integrity BCD options */
    BlImgQueryCodeIntegrityBootOptions(BootEntry,
                                       &DisableIntegrity,
                                       &TestSigning);

#if BL_TPM_SUPPORT
    RtlZeroMemory(&Context, sizeof(Context);
    Context.BootEntry = BootEntry;
    BlEnNotifyEvent(0x10000003, &Context);
#endif

    /* Enable signing and hashing checks if integrity is enabled */
    Flags = 0;
    if (!DisableIntegrity)
    {
        Flags = 0x8070;
    }

    /* Now call the PE loader to load the image */
    Status = BlImgLoadPEImageEx(DeviceId,
                                BlLoaderMemory,
                                Path,
                                &ImageBase,
                                &ImageSize,
                                Hash,
                                Flags);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

#if BL_KD_SUPPORT
    /* Check if we should notify the debugger of load */
    if (BdDebugTransitions)
    {
        /* Initialize it */
        BdForceDebug = 1;
        Status = BlBdInitialize();
        if (NT_SUCCESS(Status))
        {
            /* Check if it's enabled */
            if (BlBdDebuggerEnabled())
            {
                /* Send it an image load notification */
                BdDebuggerNotPresent = FALSE;
                RtlInitUnicodeString(&PathString, Path);
                BlBdLoadImageSymbols(&PathString, ImageBase);
            }
        }
    }
#endif

#if BL_BITLOCKER_SUPPORT
    /* Do bitlocker stuff */
    Status = BlSecureBootCheckPolicyOnFveDevice(BitLockerDevice);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }
#endif

#if BL_BITLOCKER_SUPPORT
    /* Do bitlocker stuff */
    Status = BlFveSecureBootCheckpointBootApp(BootEntry, BitLockerDevice, Hash, UnlockCode);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }
#endif

    /* Get the BCD option size */
    ListSize = BlGetBootOptionListSize(BootEntry->BcdData);

    /* Allocate an entry with all the BCD options */
    AppEntry = BlMmAllocateHeap(ListSize + sizeof(*AppEntry));
    if (!AppEntry)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Zero it out */
    RtlZeroMemory(AppEntry, sizeof(AppEntry));

    /* Initialize it */
    strcpy(AppEntry->Signature, "BTAPENT");
    AppEntry->Guid = BootEntry->Guid;
    AppEntry->Flags = BootEntry->Flags;

    /* Copy the BCD options */
    RtlCopyMemory(&AppEntry->BcdData, BootEntry->BcdData, ListSize);

    /* Allocate the image entry */
    ImageAppEntry = BlMmAllocateHeap(sizeof(*ImageAppEntry));
    if (!ImageAppEntry)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Initialize it */
    ImageAppEntry->ImageBase = ImageBase;
    ImageAppEntry->ImageSize = ImageSize;
    ImageAppEntry->AppEntry = AppEntry;

    /* Check if this is the first entry */
    if (!IapTableEntries)
    {
        /* Allocate two entries */
        IapAllocatedTableEntries = 0;
        IapTableEntries = 2;
        IapImageTable = BlMmAllocateHeap(IapTableEntries * sizeof(PVOID));
        if (!IapImageTable)
        {
            Status = STATUS_NO_MEMORY;
            goto Quickie;
        }

        /* Zero out the entries for now */
        RtlZeroMemory(IapImageTable, sizeof(IapTableEntries * sizeof(PVOID)));
    }

    /* Set this entry into the table */
    Status = BlTblSetEntry(&IapImageTable,
                           &IapTableEntries,
                           ImageAppEntry,
                           AppHandle,
                           TblDoNotPurgeEntry);

Quickie:
    /* Is the device open? Close it if so */
    if (DeviceId != 1)
    {
        BlDeviceClose(DeviceId);
    }

    /* Is there an allocated device? Free it */
    if (Device)
    {
        BlMmFreeHeap(Device);
    }

    /* Is there an allocated path? Free it */
    if (Path)
    {
        BlMmFreeHeap(Path);
    }

    /* Is there a bitlocker device? Free it */
    if (BitLockerDevice)
    {
        BlMmFreeHeap(BitLockerDevice);
    }

    /* Is there a bitlocker unlock code? Free it */
    if (UnlockCode)
    {
        BlMmFreeHeap(UnlockCode);
    }

    /* Did we succeed in creating an entry? */
    if (NT_SUCCESS(Status))
    {
        /* Remember there's one more in the table */
        IapAllocatedTableEntries++;

        /* Return success */
        return Status;
    }

    /* Did we load an image after all? */
    if (ImageBase)
    {
        /* Unload it */
        BlImgUnLoadImage(ImageBase, ImageSize, 0);
    }

    /* Did we allocate an app entry? Free it */
    if (AppEntry)
    {
        BlMmFreeHeap(AppEntry);
    }

    /* Do we have an image file entry?  Free it */
    if (ImageFile)
    {
        BlMmFreeHeap(ImageFile);
    }

    /* Do we no longer have a single entry in the table? */
    if (!(IapAllocatedTableEntries) && (IapImageTable))
    {
        /* Free and destroy the table */
        BlMmFreeHeap(IapImageTable);
        IapTableEntries = 0;
        IapImageTable = NULL;
    }

    /* Return the failure code */
    return Status;
}

NTSTATUS
BlpPdParseReturnArguments (
    _In_ PBL_RETURN_ARGUMENTS ReturnArguments
    )
{
    /* Check if any custom data was returned */
    if (ReturnArguments->DataPage == 0)
    {
        /* Nope, nothing to do */
        return STATUS_SUCCESS;
    }

    /* Yes, we have to parse it */
    EfiPrintf(L"Return arguments not supported\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
ImgArchEfiStartBootApplication (
    _In_ PBL_APPLICATION_ENTRY AppEntry,
    _In_ PVOID ImageBase,
    _In_ ULONG ImageSize,
    _In_ PBL_RETURN_ARGUMENTS ReturnArguments
    )
{
    /* Not yet implemented. This is the last step! */
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
BlImgStartBootApplication (
    _In_ ULONG AppHandle,
    _Inout_opt_ PBL_RETURN_ARGUMENTS ReturnArguments
    )
{
    PBL_IMAGE_APPLICATION_ENTRY ImageAppEntry;
    BL_RETURN_ARGUMENTS LocalReturnArgs;
    PBL_FILE_SYSTEM_ENTRY FileSystem;
    PLIST_ENTRY NextEntry, ListHead;
    NTSTATUS Status;

    /* Check if we don't have an argument structure */
    if (!ReturnArguments)
    {
        /* Initialize a local copy and use it instead */
        LocalReturnArgs.Version = BL_RETURN_ARGUMENTS_VERSION;
        LocalReturnArgs.Status = STATUS_SUCCESS;
        LocalReturnArgs.Flags = 0;
        LocalReturnArgs.DataPage = 0;
        LocalReturnArgs.DataSize = 0;
        ReturnArguments = &LocalReturnArgs;
    }

    /* Make sure the handle index is valid */
    if (IapTableEntries <= AppHandle)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Get the entry for this handle, making sure it exists */
    ImageAppEntry = IapImageTable[AppHandle];
    if (!ImageAppEntry)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Loop the registered file systems */
    ListHead = &RegisteredFileSystems;
    NextEntry = RegisteredFileSystems.Flink;
    while (NextEntry != ListHead)
    {
        /* Get the filesystem entry */
        FileSystem = CONTAINING_RECORD(NextEntry,
                                       BL_FILE_SYSTEM_ENTRY,
                                       ListEntry);

        /* See if it has a purge callback */
        if (FileSystem->PurgeCallback)
        {
            /* Call it */
            FileSystem->PurgeCallback();
        }

        /* Move to the next entry */
        NextEntry = NextEntry->Flink;
    }

    /* TODO  -- flush the block I/O cache too */
    //BlockIoPurgeCache();

    /* Call into EFI land to start the boot application */
    Status = ImgArchEfiStartBootApplication(ImageAppEntry->AppEntry,
                                            ImageAppEntry->ImageBase,
                                            ImageAppEntry->ImageSize,
                                            ReturnArguments);

    /* Parse any arguments we got on the way back */
    BlpPdParseReturnArguments(ReturnArguments);

#if BL_BITLOCKER_SUPPORT
    /* Bitlocker stuff */
    FvebpCheckAllPartitions(TRUE);
#endif

#if BL_TPM_SUPPORT
    /* Notify a TPM/SI event */
    BlEnNotifyEvent(0x10000005, NULL);
#endif

    /* Reset the display */
    BlpDisplayReinitialize();

    /* TODO -- reset ETW */
    //BlpLogInitialize();

    /* All done */
    return Status;
}

NTSTATUS
BlImgUnloadBootApplication (
    _In_ ULONG AppHandle
    )
{
    PBL_IMAGE_APPLICATION_ENTRY ImageAppEntry;
    NTSTATUS Status;

    /* Make sure the handle index is valid */
    if (IapTableEntries <= AppHandle)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Get the entry for this handle, making sure it exists */
    ImageAppEntry = IapImageTable[AppHandle];
    if (!ImageAppEntry)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Unload the image */
    Status = BlImgUnLoadImage(ImageAppEntry->ImageBase,
                              ImageAppEntry->ImageSize,
                              0);
    if (NT_SUCCESS(Status))
    {
        /* Normalize the success code */
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* Normalize the failure code */
        Status = STATUS_MEMORY_NOT_ALLOCATED;
    }

    /* Free the entry and the image entry as well */
    BlMmFreeHeap(ImageAppEntry->AppEntry);
    BlMmFreeHeap(ImageAppEntry);

    /* Clear the handle */
    IapImageTable[AppHandle] = NULL;

    /* Free one entry */
    if (!(--IapAllocatedTableEntries))
    {
        /* There are no more, so get rid of the table itself */
        BlMmFreeHeap(IapImageTable);
        IapImageTable = NULL;
        IapTableEntries = 0;
    }

    /* All good */
    return Status;
}

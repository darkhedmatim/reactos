/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Functions for mapping files and sections
 * FILE:              subsys/win32k/eng/device.c
 * PROGRAMER:         Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

HANDLE ghSystem32Directory;
HSEMAPHORE ghsemModuleList;
LIST_ENTRY gleModulelist = {&gleModulelist, &gleModulelist};

INIT_FUNCTION
NTSTATUS
NTAPI
InitMappingImpl(VOID)
{
    ghsemModuleList = EngCreateSemaphore();
    if (!ghsemModuleList) return STATUS_NO_MEMORY;
    return STATUS_SUCCESS;
}

PVOID
NTAPI
EngCreateSection(
    IN ULONG fl,
    IN SIZE_T cjSize,
    IN ULONG ulTag)
{
    NTSTATUS Status;
    PENGSECTION pSection;
    PVOID pvSectionObject;
    LARGE_INTEGER liSize;

    /* Allocate a section object */
    pSection = EngAllocMem(0, sizeof(ENGSECTION), 'stsU');
    if (!pSection) return NULL;

    liSize.QuadPart = cjSize;
    Status = MmCreateSection(&pvSectionObject,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &liSize,
                             PAGE_READWRITE,
                             SEC_COMMIT,
                             NULL,
                             NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create a section Status=0x%x\n", Status);
        EngFreeMem(pSection);
        return NULL;
    }

    /* Set the fields of the section */
    pSection->ulTag = ulTag;
    pSection->pvSectionObject = pvSectionObject;
    pSection->pvMappedBase = NULL;
    pSection->cjViewSize = cjSize;

    return pSection;
}


BOOL
APIENTRY
EngMapSection(
    IN PVOID pvSection,
    IN BOOL bMap,
    IN HANDLE hProcess,
    OUT PVOID* pvBaseAddress)
{
    NTSTATUS Status;
    PENGSECTION pSection = pvSection;
    PEPROCESS pepProcess;

    /* Get a pointer to the process */
    Status = ObReferenceObjectByHandle(hProcess,
                                       PROCESS_VM_OPERATION,
                                       NULL,
                                       KernelMode,
                                       (PVOID*)&pepProcess,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not access process %p, Status=0x%lx\n", hProcess, Status);
        return FALSE;
    }

    if (bMap)
    {
        /* Make sure the section isn't already mapped */
        ASSERT(pSection->pvMappedBase == NULL);

        /* Map the section into the process address space */
        Status = MmMapViewOfSection(pSection->pvSectionObject,
                                    pepProcess,
                                    &pSection->pvMappedBase,
                                    0,
                                    pSection->cjViewSize,
                                    NULL,
                                    &pSection->cjViewSize,
                                    0,
                                    0,
                                    PAGE_READWRITE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to map a section Status=0x%x\n", Status);
        }
    }
    else
    {
        /* Make sure the section is mapped */
        ASSERT(pSection->pvMappedBase);

        /* Unmap the section from the process address space */
        Status = MmUnmapViewOfSection(pepProcess, pSection->pvMappedBase);
        if (NT_SUCCESS(Status))
        {
            pSection->pvMappedBase = NULL;
        }
        else
        {
            DPRINT1("Failed to unmap a section @ &p Status=0x%x\n",
                    pSection->pvMappedBase, Status);
        }
    }

    /* Dereference the process */
    ObDereferenceObject(pepProcess);

    /* Set the new mapping base and return bool status */
    *pvBaseAddress = pSection->pvMappedBase;
    return NT_SUCCESS(Status);
}

BOOL
APIENTRY
EngFreeSectionMem(
    IN PVOID pvSection OPTIONAL,
    IN PVOID pvMappedBase OPTIONAL)
{
    NTSTATUS Status;
    PENGSECTION pSection = pvSection;
    BOOL bResult = TRUE;

    /* Did the caller give us a mapping base? */
    if (pvMappedBase)
    {
        Status = MmUnmapViewInSessionSpace(pvMappedBase);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("MmUnmapViewInSessionSpace failed: 0x%lx\n", Status);
            bResult = FALSE;
        }
    }

    /* Check if we should free the section as well */
    if (pSection)
    {
        /* Dereference the kernel section */
        ObDereferenceObject(pSection->pvSectionObject);

        /* Finally free the section memory itself */
        EngFreeMem(pSection);
    }

    return bResult;
}

PVOID
APIENTRY
EngAllocSectionMem(
    OUT PVOID *ppvSection,
    IN ULONG fl,
    IN SIZE_T cjSize,
    IN ULONG ulTag)
{
    NTSTATUS Status;
    PENGSECTION pSection;

    /* Check parameter */
    if (cjSize == 0) return NULL;

    /* Allocate a section object */
    pSection = EngCreateSection(fl, cjSize, ulTag);
    if (!pSection)
    {
        *ppvSection = NULL;
        return NULL;
    }

    /* Map the section in session space */
    Status = MmMapViewInSessionSpace(pSection->pvSectionObject,
                                     &pSection->pvMappedBase,
                                     &pSection->cjViewSize);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to map a section Status=0x%x\n", Status);
        *ppvSection = NULL;
        EngFreeSectionMem(pSection, NULL);
        return NULL;
    }

    if (fl & FL_ZERO_MEMORY)
    {
        RtlZeroMemory(pSection->pvMappedBase, cjSize);
    }

    /* Set section pointer and return base address */
    *ppvSection = pSection;
    return pSection->pvMappedBase;
}


PFILEVIEW
NTAPI
EngLoadModuleEx(
    LPWSTR pwsz,
    ULONG cjSizeOfModule,
    FLONG fl)
{
    PFILEVIEW pFileView = NULL;
    PFONTFILEVIEW pffv;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hRootDir;
    UNICODE_STRING ustrFileName;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION FileInformation;
    HANDLE hFile = NULL, hSection;
    NTSTATUS Status;
    LARGE_INTEGER liSize;
    PLIST_ENTRY ple;
    ULONG cjSize, flPageProtect;
    ACCESS_MASK amAccess;

    /* Acquire module list lock */
    EngAcquireSemaphore(ghsemModuleList);

    /* Loop the list of loaded modules */
    for (ple = gleModulelist.Flink; ple != &gleModulelist; ple = ple->Flink)
    {
        pFileView = CONTAINING_RECORD(ple, FILEVIEW, leLink);
        if (_wcsnicmp(pFileView->pwszPath, pwsz, MAX_PATH) == 0)
        {
            /* Increment reference count and leave */
            pFileView->cRefs++;
            goto cleanup;
        }
    }

    /* Use system32 root dir or absolute path */
    hRootDir = fl & FVF_SYSTEMROOT ? ghSystem32Directory : NULL;

    /* Set access mask and page protection */
    if (fl & FVF_WRITE)
    {
        amAccess = FILE_READ_DATA|FILE_WRITE_DATA;
        flPageProtect =  PAGE_EXECUTE_READWRITE;
    }
    else
    {
        amAccess = FILE_READ_DATA;
        flPageProtect = PAGE_EXECUTE_READ;
    }

    /* Initialize unicode string and object attributes */
    RtlInitUnicodeString(&ustrFileName, pwsz);
    InitializeObjectAttributes(&ObjectAttributes,
                               &ustrFileName,
                               OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,
                               hRootDir,
                               NULL);

    /* Now open the file */
    Status = ZwCreateFile(&hFile,
                          amAccess,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_OPEN,
                          FILE_NON_DIRECTORY_FILE,
                          NULL,
                          0);

    if (!NT_SUCCESS(Status))
    {
        pFileView = NULL;
        goto cleanup;
    }

    /* Check if this is a font file */
    if (fl & FVF_FONTFILE)
    {
        /* Allocate a FONTFILEVIEW structure */
        cjSize = sizeof(FONTFILEVIEW) + ustrFileName.Length + sizeof(WCHAR);
        pffv = EngAllocMem(0, cjSize, 'vffG');
        pFileView = (PFILEVIEW)pffv;
        if (!pffv)
        {
            Status = STATUS_NO_MEMORY;
            goto cleanup;
        }

        /* Initialize extended fields */
        pffv->pwszPath = (PWSTR)&pffv[1];
        pffv->ulRegionSize = 0;
        pffv->cKRefCount = 0;
        pffv->cRefCountFD = 0;
        pffv->pvSpoolerBase = NULL;
        pffv->dwSpoolerPid = 0;
    }
    else
    {
        /* Allocate a FILEVIEW structure */
        cjSize = sizeof(FILEVIEW) + ustrFileName.Length + sizeof(WCHAR);
        pFileView = EngAllocMem(0, cjSize, 'liFg');
        if (!pFileView)
        {
            Status = STATUS_NO_MEMORY;
            goto cleanup;
        }

        pFileView->pwszPath = (PWSTR)&pFileView[1];
    }

    /* Initialize the structure */
    pFileView->cRefs = 1;
    pFileView->pvKView = NULL;
    pFileView->pvViewFD = NULL;
    pFileView->cjView = 0;

    /* Copy the file name */
    wcscpy(pFileView->pwszPath, pwsz);

    /* Query the last write time */
    FileInformation.LastWriteTime.QuadPart = 0;
    Status = ZwQueryInformationFile(hFile,
                                    &IoStatusBlock,
                                    &FileInformation,
                                    sizeof(FILE_BASIC_INFORMATION),
                                    FileBasicInformation);
    pFileView->LastWriteTime = FileInformation.LastWriteTime;

    /* Create a section from the file */
    liSize.QuadPart = cjSizeOfModule;
    Status = ZwCreateSection(&hSection,
                             SECTION_ALL_ACCESS,
                             NULL,
                             cjSizeOfModule ? &liSize : NULL,
                             flPageProtect,
                             SEC_COMMIT,
                             hFile);

    if (NT_SUCCESS(Status))
    {
        Status = ObReferenceObjectByHandle(hSection,
                                           SECTION_ALL_ACCESS,
                                           MmSectionObjectType,
                                           KernelMode,
                                           &pFileView->pSection,
                                           NULL);
        ZwClose(hSection);
    }

    if (!NT_SUCCESS(Status))
    {
        EngFreeMem(pFileView);
        pFileView = NULL;
        goto cleanup;
    }

    /* Insert the structure into the list */
    InsertTailList(&gleModulelist, &pFileView->leLink);

cleanup:
    /* Close the file handle */
    if (hFile) ZwClose(hFile);

    /* Release module list lock */
    EngReleaseSemaphore(ghsemModuleList);

    return pFileView;
}

HANDLE
APIENTRY
EngLoadModule(LPWSTR pwsz)
{
    /* Forward to EngLoadModuleEx */
    return EngLoadModuleEx(pwsz, 0, FVF_SYSTEMROOT);
}

HANDLE
APIENTRY
EngLoadModuleForWrite(
	IN LPWSTR pwsz,
	IN ULONG  cjSizeOfModule)
{
    /* Forward to EngLoadModuleEx */
    return EngLoadModuleEx(pwsz, cjSizeOfModule, FVF_WRITE|FVF_SYSTEMROOT);
}

PVOID
APIENTRY
EngMapModule(
	IN  HANDLE h,
	OUT PULONG pulSize)
{
    PFILEVIEW pFileView = (PFILEVIEW)h;
    NTSTATUS Status;

    pFileView->cjView = 0;

    /* Map the section in session space */
    Status = MmMapViewInSessionSpace(pFileView->pSection,
                                     &pFileView->pvKView,
                                     &pFileView->cjView);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to map a section Status=0x%x\n", Status);
        *pulSize = 0;
        return NULL;
    }

    *pulSize = pFileView->cjView;
    return pFileView->pvKView;
}

VOID
APIENTRY
EngFreeModule(IN HANDLE h)
{
    PFILEVIEW pFileView = (PFILEVIEW)h;
    NTSTATUS Status;

    /* Acquire module list lock */
    EngAcquireSemaphore(ghsemModuleList);

    /* Decrement reference count and check if its 0 */
    if (--pFileView->cRefs == 0)
    {
        /* Check if the section was mapped */
        if (pFileView->pvKView)
        {
            /* Unmap the section */
            Status = MmUnmapViewInSessionSpace(pFileView->pvKView);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("MmUnmapViewInSessionSpace failed: 0x%lx\n", Status);
                ASSERT(FALSE);
            }
        }

        /* Dereference the section */
        ObDereferenceObject(pFileView->pSection);

        /* Remove the entry from the list */
        RemoveEntryList(&pFileView->leLink);

        /* Free the file view memory */
        EngFreeMem(pFileView);
    }

    /* Release module list lock */
    EngReleaseSemaphore(ghsemModuleList);
}

PVOID
APIENTRY
EngMapFile(
    IN LPWSTR pwsz,
    IN ULONG cjSize,
    OUT ULONG_PTR *piFile)
{
    HANDLE hModule;
    PVOID pvBase;

    /* Load the file */
    hModule = EngLoadModuleEx(pwsz, 0, 0);
    if (!hModule)
    {
        *piFile = 0;
        return NULL;
    }

    /* Map the file */
    pvBase = EngMapModule(hModule, &cjSize);
    if (!pvBase)
    {
        EngFreeModule(hModule);
        hModule = NULL;
    }

    /* Set iFile and return mapped base */
    *piFile = (ULONG_PTR)hModule;
    return pvBase;
}

BOOL
APIENTRY
EngUnmapFile(
    IN ULONG_PTR iFile)
{
    HANDLE hModule = (HANDLE)iFile;

    EngFreeModule(hModule);

    return TRUE;
}


BOOL
APIENTRY
EngMapFontFileFD(
	IN  ULONG_PTR iFile,
	OUT PULONG    *ppjBuf,
	OUT ULONG     *pcjBuf)
{
    PFONTFILEVIEW pffv = (PFONTFILEVIEW)iFile;
    NTSTATUS Status = STATUS_SUCCESS;

    // should be exclusively accessing this file!

    /* Increment reference count and check if its the 1st */
    if (++pffv->cRefCountFD == 1)
    {
        /* Map the file into the address space of CSRSS */
        Status = MmMapViewOfSection(pffv->pSection,
                                    gpepCSRSS,
                                    &pffv->pvViewFD,
                                    0,
                                    0,
                                    NULL,
                                    &pffv->cjView,
                                    ViewUnmap,
                                    0,
                                    PAGE_READONLY);
    }

    /* Return base address and size */
    if (ppjBuf) *ppjBuf = pffv->pvViewFD;
    if (pcjBuf) *pcjBuf = pffv->cjView;

    return NT_SUCCESS(Status);
}

VOID
APIENTRY
EngUnmapFontFileFD(
    IN ULONG_PTR iFile)
{
    PFONTFILEVIEW pffv = (PFONTFILEVIEW)iFile;
    NTSTATUS Status = STATUS_SUCCESS;

    // should be exclusively accessing this file!

    /* Decrement reference count and check if we reached 0 */
    if (--pffv->cRefCountFD == 0)
    {
        /* Unmap the file from the address space of CSRSS */
        Status = MmUnmapViewOfSection(gpepCSRSS, pffv->pvViewFD);
    }
}

BOOL
APIENTRY
EngMapFontFile(
	ULONG_PTR iFile,
	PULONG    *ppjBuf,
	ULONG     *pcjBuf)
{
    // www.osr.com/ddk/graphics/gdifncs_3up3.htm
    return EngMapFontFileFD(iFile, ppjBuf, pcjBuf);
}

VOID
APIENTRY
EngUnmapFontFile(
    IN ULONG_PTR iFile)
{
    // www.osr.com/ddk/graphics/gdifncs_09wn.htm
    EngUnmapFontFileFD(iFile);
}

/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
 *  Copyright (C) 2008-2009  Herv� Poussineau  <hpoussin@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>

#define NDEBUG
#include <debug.h>

DBG_DEFAULT_CHANNEL(FILESYSTEM);

/* GLOBALS ********************************************************************/

#define TAG_DEVICE_NAME 'NDsF'
#define TAG_DEVICE 'vDsF'

typedef struct tagFILEDATA
{
    ULONG DeviceId;
    ULONG ReferenceCount;
    const DEVVTBL* FuncTable;
    const DEVVTBL* FileFuncTable;
    VOID* Specific;
} FILEDATA;

typedef struct tagDEVICE
{
    LIST_ENTRY ListEntry;
    const DEVVTBL* FuncTable;
    CHAR* Prefix;
    ULONG DeviceId;
    ULONG ReferenceCount;
} DEVICE;

static FILEDATA FileData[MAX_FDS];
static LIST_ENTRY DeviceListHead;

/* ARC FUNCTIONS **************************************************************/

ARC_STATUS ArcOpen(CHAR* Path, OPENMODE OpenMode, ULONG* FileId)
{
    ARC_STATUS Status;
    ULONG Count, i;
    PLIST_ENTRY pEntry;
    DEVICE* pDevice;
    CHAR* DeviceName;
    CHAR* FileName;
    CHAR* p;
    CHAR* q;
    SIZE_T Length;
    OPENMODE DeviceOpenMode;
    ULONG DeviceId;

    /* Print status message */
    TRACE("Opening file '%s'...\n", Path);

    *FileId = MAX_FDS;

    /* Search last ')', which delimits device and path */
    FileName = strrchr(Path, ')');
    if (!FileName)
        return EINVAL;
    FileName++;

    /* Count number of "()", which needs to be replaced by "(0)" */
    Count = 0;
    for (p = Path; p != FileName; p++)
    {
        if (*p == '(' && *(p + 1) == ')')
            Count++;
    }

    /* Duplicate device name, and replace "()" by "(0)" (if required) */
    Length = FileName - Path + Count;
    if (Count != 0)
    {
        DeviceName = FrLdrTempAlloc(FileName - Path + Count, TAG_DEVICE_NAME);
        if (!DeviceName)
            return ENOMEM;
        for (p = Path, q = DeviceName; p != FileName; p++)
        {
            *q++ = *p;
            if (*p == '(' && *(p + 1) == ')')
                *q++ = '0';
        }
    }
    else
    {
        DeviceName = Path;
    }

    /* Search for the device */
    if (OpenMode == OpenReadOnly || OpenMode == OpenWriteOnly)
        DeviceOpenMode = OpenMode;
    else
        DeviceOpenMode = OpenReadWrite;

    pEntry = DeviceListHead.Flink;
    while (pEntry != &DeviceListHead)
    {
        pDevice = CONTAINING_RECORD(pEntry, DEVICE, ListEntry);
        if (strncmp(pDevice->Prefix, DeviceName, Length) == 0)
        {
            /* OK, device found. It is already opened? */
            if (pDevice->ReferenceCount == 0)
            {
                /* Search some room for the device */
                for (DeviceId = 0; DeviceId < MAX_FDS; DeviceId++)
                {
                    if (!FileData[DeviceId].FuncTable)
                        break;
                }
                if (DeviceId == MAX_FDS)
                    return EMFILE;

                /* Try to open the device */
                FileData[DeviceId].FuncTable = pDevice->FuncTable;
                Status = pDevice->FuncTable->Open(pDevice->Prefix, DeviceOpenMode, &DeviceId);
                if (Status != ESUCCESS)
                {
                    FileData[DeviceId].FuncTable = NULL;
                    return Status;
                }
                else if (!*FileName)
                {
                    /* Done, caller wanted to open the raw device */
                    *FileId = DeviceId;
                    pDevice->ReferenceCount++;
                    return ESUCCESS;
                }

                /* Try to detect the file system */
#ifndef _M_ARM
                FileData[DeviceId].FileFuncTable = IsoMount(DeviceId);
                if (!FileData[DeviceId].FileFuncTable)
#endif
                    FileData[DeviceId].FileFuncTable = FatMount(DeviceId);
#ifndef _M_ARM
                if (!FileData[DeviceId].FileFuncTable)
                    FileData[DeviceId].FileFuncTable = NtfsMount(DeviceId);
                if (!FileData[DeviceId].FileFuncTable)
                    FileData[DeviceId].FileFuncTable = Ext2Mount(DeviceId);
#endif
#if defined(_M_IX86) || defined(_M_AMD64)
                if (!FileData[DeviceId].FileFuncTable)
                    FileData[DeviceId].FileFuncTable = PxeMount(DeviceId);
#endif
                if (!FileData[DeviceId].FileFuncTable)
                {
                    /* Error, unable to detect file system */
                    pDevice->FuncTable->Close(DeviceId);
                    FileData[DeviceId].FuncTable = NULL;
                    return ENODEV;
                }

                pDevice->DeviceId = DeviceId;
            }
            else
            {
                DeviceId = pDevice->DeviceId;
            }
            pDevice->ReferenceCount++;
            break;
        }
        pEntry = pEntry->Flink;
    }
    if (pEntry == &DeviceListHead)
        return ENODEV;

    /* At this point, device is found and opened. Its file id is stored
     * in DeviceId, and FileData[DeviceId].FileFuncTable contains what
     * needs to be called to open the file */

    /* Search some room for the device */
    for (i = 0; i < MAX_FDS; i++)
    {
        if (!FileData[i].FuncTable)
            break;
    }
    if (i == MAX_FDS)
        return EMFILE;

    /* Skip leading backslash, if any */
    if (*FileName == '\\')
        FileName++;

    /* Open the file */
    FileData[i].FuncTable = FileData[DeviceId].FileFuncTable;
    FileData[i].DeviceId = DeviceId;
    *FileId = i;
    Status = FileData[i].FuncTable->Open(FileName, OpenMode, FileId);
    if (Status != ESUCCESS)
    {
        FileData[i].FuncTable = NULL;
        *FileId = MAX_FDS;
    }
    return Status;
}

ARC_STATUS ArcClose(ULONG FileId)
{
    ARC_STATUS Status;

    if (FileId >= MAX_FDS || !FileData[FileId].FuncTable)
        return EBADF;

    Status = FileData[FileId].FuncTable->Close(FileId);

    if (Status == ESUCCESS)
    {
        FileData[FileId].FuncTable = NULL;
        FileData[FileId].Specific = NULL;
        FileData[FileId].DeviceId = -1;
    }
    return Status;
}

ARC_STATUS ArcRead(ULONG FileId, VOID* Buffer, ULONG N, ULONG* Count)
{
    if (FileId >= MAX_FDS || !FileData[FileId].FuncTable)
        return EBADF;
    return FileData[FileId].FuncTable->Read(FileId, Buffer, N, Count);
}

ARC_STATUS ArcSeek(ULONG FileId, LARGE_INTEGER* Position, SEEKMODE SeekMode)
{
    if (FileId >= MAX_FDS || !FileData[FileId].FuncTable)
        return EBADF;
    return FileData[FileId].FuncTable->Seek(FileId, Position, SeekMode);
}

ARC_STATUS ArcGetFileInformation(ULONG FileId, FILEINFORMATION* Information)
{
    if (FileId >= MAX_FDS || !FileData[FileId].FuncTable)
        return EBADF;
    return FileData[FileId].FuncTable->GetFileInformation(FileId, Information);
}

/* FUNCTIONS ******************************************************************/

VOID FileSystemError(PCSTR ErrorString)
{
    ERR("%s\n", ErrorString);
    UiMessageBox(ErrorString);
}

PFILE FsOpenFile(PCSTR FileName)
{
    CHAR FullPath[MAX_PATH] = "";
    ULONG FileId;
    ARC_STATUS Status;

    //
    // Print status message
    //
    TRACE("Opening file '%s'...\n", FileName);

    //
    // Check whether FileName is a full path
    // and if not, create a full file name.
    //
    // See ArcOpen: Search last ')', which delimits device and path.
    //
    if (strrchr(FileName, ')') == NULL)
    {
        /* This is not a full path. Use the current (i.e. boot) device. */
        MachDiskGetBootPath(FullPath, sizeof(FullPath));

        /* Append a path separator if needed */
        if (FileName[0] != '\\' && FileName[0] != '/')
            strcat(FullPath, "\\");
    }
    // Append (or just copy) the remaining file name.
    strcat(FullPath, FileName);

    //
    // Open the file
    //
    Status = ArcOpen(FullPath, OpenReadOnly, &FileId);

    //
    // Check for success
    //
    if (Status == ESUCCESS)
        return (PFILE)FileId;
    else
        return (PFILE)0;
}

VOID FsCloseFile(PFILE FileHandle)
{
    ULONG FileId = (ULONG)FileHandle;

    //
    // Close the handle. Do not check for error,
    // this function is supposed to always succeed.
    //
    ArcClose(FileId);
}

/*
 * ReadFile()
 * returns number of bytes read or EOF
 */
BOOLEAN FsReadFile(PFILE FileHandle, ULONG BytesToRead, ULONG* BytesRead, PVOID Buffer)
{
    ULONG FileId = (ULONG)FileHandle;

    //
    // Read the file
    //
    return (ArcRead(FileId, Buffer, BytesToRead, BytesRead) == ESUCCESS);
}

BOOLEAN FsGetFileInformation(PFILE FileHandle, FILEINFORMATION* Information)
{
    ULONG FileId = (ULONG)FileHandle;

    //
    // Get file information
    //
    return (ArcGetFileInformation(FileId, Information) == ESUCCESS);
}

ULONG FsGetFileSize(PFILE FileHandle)
{
    ULONG FileId = (ULONG)FileHandle;
    FILEINFORMATION Information;
    ARC_STATUS Status;

    //
    // Query file informations
    //
    Status = ArcGetFileInformation(FileId, &Information);

    //
    // Check for error
    //
    if (Status != ESUCCESS || Information.EndingAddress.HighPart != 0)
        return 0;

    //
    // Return file size
    //
    return Information.EndingAddress.LowPart;
}

VOID FsSetFilePointer(PFILE FileHandle, ULONG NewFilePointer)
{
    ULONG FileId = (ULONG)FileHandle;
    LARGE_INTEGER Position;

    //
    // Set file position. Do not check for error,
    // this function is supposed to always succeed.
    //
    Position.HighPart = 0;
    Position.LowPart = NewFilePointer;
    ArcSeek(FileId, &Position, SeekAbsolute);
}

/*
 * FsGetNumPathParts()
 * This function parses a path in the form of dir1\dir2\file1.ext
 * and returns the number of parts it has (i.e. 3 - dir1,dir2,file1.ext)
 */
ULONG FsGetNumPathParts(PCSTR Path)
{
    size_t i;
    ULONG  num;

    for (i = 0, num = 0; i < strlen(Path); i++)
    {
        if ((Path[i] == '\\') || (Path[i] == '/'))
        {
            num++;
        }
    }
    num++;

    TRACE("FsGetNumPathParts() Path = %s NumPathParts = %d\n", Path, num);

    return num;
}

/*
 * FsGetFirstNameFromPath()
 * This function parses a path in the form of dir1\dir2\file1.ext
 * and puts the first name of the path (e.g. "dir1") in buffer
 * compatible with the MSDOS directory structure
 */
VOID FsGetFirstNameFromPath(PCHAR Buffer, PCSTR Path)
{
    size_t i;

    // Copy all the characters up to the end of the
    // string or until we hit a '\' character
    // and put them in Buffer
    for (i = 0; i < strlen(Path); i++)
    {
        if ((Path[i] == '\\') || (Path[i] == '/'))
        {
            break;
        }
        else
        {
            Buffer[i] = Path[i];
        }
    }

    Buffer[i] = 0;

    TRACE("FsGetFirstNameFromPath() Path = %s FirstName = %s\n", Path, Buffer);
}

VOID FsRegisterDevice(CHAR* Prefix, const DEVVTBL* FuncTable)
{
    DEVICE* pNewEntry;
    SIZE_T Length;

    TRACE("FsRegisterDevice() Prefix = %s\n", Prefix);

    Length = strlen(Prefix) + 1;
    pNewEntry = FrLdrTempAlloc(sizeof(DEVICE) + Length, TAG_DEVICE);
    if (!pNewEntry)
        return;
    pNewEntry->FuncTable = FuncTable;
    pNewEntry->ReferenceCount = 0;
    pNewEntry->Prefix = (CHAR*)(pNewEntry + 1);
    memcpy(pNewEntry->Prefix, Prefix, Length);

    InsertHeadList(&DeviceListHead, &pNewEntry->ListEntry);
}

LPCWSTR FsGetServiceName(ULONG FileId)
{
    if (FileId >= MAX_FDS || !FileData[FileId].FuncTable)
        return NULL;
    return FileData[FileId].FuncTable->ServiceName;
}

VOID FsSetDeviceSpecific(ULONG FileId, VOID* Specific)
{
    if (FileId >= MAX_FDS || !FileData[FileId].FuncTable)
        return;
    FileData[FileId].Specific = Specific;
}

VOID* FsGetDeviceSpecific(ULONG FileId)
{
    if (FileId >= MAX_FDS || !FileData[FileId].FuncTable)
        return NULL;
    return FileData[FileId].Specific;
}

ULONG FsGetDeviceId(ULONG FileId)
{
    if (FileId >= MAX_FDS)
        return (ULONG)-1;
    return FileData[FileId].DeviceId;
}

VOID FsInit(VOID)
{
    ULONG i;

    RtlZeroMemory(FileData, sizeof(FileData));
    for (i = 0; i < MAX_FDS; i++)
        FileData[i].DeviceId = (ULONG)-1;

    InitializeListHead(&DeviceListHead);

    // FIXME: Retrieve the current boot device with MachDiskGetBootPath
    // and store it somewhere in order to not call again and again this
    // function.
}

#include "npfs.h"

PNP_FCB
NTAPI
NpFindPrefix(IN PUNICODE_STRING Name,
             IN ULONG CaseInsensitiveIndex,
             IN PUNICODE_STRING Prefix)
{
    PUNICODE_PREFIX_TABLE_ENTRY Entry;
    PNP_FCB Fcb;
    PAGED_CODE();

    Entry = RtlFindUnicodePrefix(&NpVcb->PrefixTable,
                                 Name,
                                 CaseInsensitiveIndex);
    if (!Entry) KeBugCheckEx(NPFS_FILE_SYSTEM, 0x100065u, 0, 0, 0);

    Fcb = CONTAINING_RECORD(Entry, NP_FCB, PrefixTableEntry);

    Prefix->Length = Name->Length - Fcb->FullName.Length;
    Prefix->MaximumLength = Prefix->Length;
    Prefix->Buffer = &Name->Buffer[Fcb->FullName.Length / sizeof(WCHAR)];

    if ((Prefix->Length) && (Prefix->Buffer[0] == OBJ_NAME_PATH_SEPARATOR))
    {
        Prefix->Length -= sizeof(WCHAR);
        Prefix->MaximumLength -= sizeof(WCHAR);
        ++Prefix->Buffer;
    }

    return Fcb;
}

NTSTATUS
NTAPI
NpFindRelativePrefix(IN PNP_DCB Dcb,
                     IN PUNICODE_STRING Name,
                     IN ULONG CaseInsensitiveIndex,
                     IN PUNICODE_STRING Prefix,
                     OUT PNP_FCB *FoundFcb)
{
    PWCHAR Buffer;
    PNP_FCB Fcb;
    UNICODE_STRING RootName;
    USHORT Length, MaximumLength;
    PAGED_CODE();

    Length = Name->Length;
    MaximumLength = Length + sizeof(OBJ_NAME_PATH_SEPARATOR) + sizeof(UNICODE_NULL);
    if (MaximumLength < Length) return STATUS_INVALID_PARAMETER;

    ASSERT(Dcb->NodeType == NPFS_NTC_ROOT_DCB);

    Buffer = ExAllocatePoolWithTag(PagedPool, MaximumLength, 'nFpN');
    if (!Buffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *Buffer = OBJ_NAME_PATH_SEPARATOR;
    RtlCopyMemory(Buffer + 1, Name->Buffer, Length);
    Buffer[(Length / sizeof(WCHAR)) + 1] = UNICODE_NULL;

    RootName.Length = Length + sizeof(OBJ_NAME_PATH_SEPARATOR);
    RootName.MaximumLength = MaximumLength;
    RootName.Buffer = Buffer;

    Fcb = NpFindPrefix(&RootName, CaseInsensitiveIndex, Prefix);

    ExFreePool(Buffer);

    Prefix->Buffer = &Name->Buffer[(Length - Prefix->Length) / sizeof(WCHAR)];
    *FoundFcb = Fcb;

    return STATUS_SUCCESS;
}


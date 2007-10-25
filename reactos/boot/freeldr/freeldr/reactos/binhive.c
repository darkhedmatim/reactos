/*
 *  FreeLoader
 *
 *  Copyright (C) 2001  Rex Jolliff
 *  Copyright (C) 2001  Eric Kohl
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <freeldr.h>
#include <cmlib.h>
#include <debug.h>

/* FUNCTIONS ****************************************************************/

static PVOID
NTAPI
CmpAllocate (SIZE_T Size, BOOLEAN Paged, ULONG Tag)
{
  return MmAllocateMemory(Size);
}


static VOID
NTAPI
CmpFree (PVOID Ptr, IN ULONG Quota)
{
  return MmFreeMemory(Ptr);
}


static BOOLEAN
CmiAllocateHashTableCell (PHHIVE Hive,
			  PHCELL_INDEX HBOffset,
			  ULONG SubKeyCount)
{
  PHASH_TABLE_CELL HashCell;
  ULONG NewHashSize;

  NewHashSize = sizeof(HASH_TABLE_CELL) +
		(SubKeyCount * sizeof(HASH_RECORD));
  *HBOffset = HvAllocateCell (Hive, NewHashSize, Stable, HCELL_NIL);
  if (*HBOffset == HCELL_NIL)
    {
      return FALSE;
    }

  HashCell = (PHASH_TABLE_CELL)HvGetCell (Hive, *HBOffset);
  HashCell->Id = REG_HASH_TABLE_CELL_ID;
  HashCell->HashTableSize = SubKeyCount;

  return TRUE;
}


static BOOLEAN
CmiAddKeyToParentHashTable (PHHIVE Hive,
			    HCELL_INDEX Parent,
			    PCM_KEY_NODE NewKeyCell,
			    HCELL_INDEX NKBOffset)
{
  PHASH_TABLE_CELL HashBlock;
  PCM_KEY_NODE ParentKeyCell;
  ULONG i;

  ParentKeyCell = (PVOID)HvGetCell (Hive, Parent);
  HashBlock = (PVOID)HvGetCell (Hive, ParentKeyCell->SubKeyLists[Stable]);

  for (i = 0; i < HashBlock->HashTableSize; i++)
    {
      if (HashBlock->Table[i].KeyOffset == 0)
	{
	  HashBlock->Table[i].KeyOffset = NKBOffset;
	  memcpy (&HashBlock->Table[i].HashValue,
		  NewKeyCell->Name,
		  min(NewKeyCell->NameSize, sizeof(ULONG)));
	  ParentKeyCell->SubKeyCounts[Stable]++;
	  return TRUE;
	}
    }

  return FALSE;
}


static BOOLEAN
CmiAllocateValueListCell (PHHIVE Hive,
			  PHCELL_INDEX ValueListOffset,
			  ULONG ValueCount)
{
  ULONG ValueListSize;

  ValueListSize = sizeof(VALUE_LIST_CELL) +
		  (ValueCount * sizeof(HCELL_INDEX));
  *ValueListOffset = HvAllocateCell (Hive, ValueListSize, Stable, HCELL_NIL);
  if (*ValueListOffset == HCELL_NIL)
    {
      DbgPrint((DPRINT_REGISTRY, "HvAllocateCell() failed\n"));
      return FALSE;
    }

  return TRUE;
}


static BOOLEAN
CmiAllocateValueCell(PHHIVE Hive,
		     PCM_KEY_VALUE *ValueCell,
		     HCELL_INDEX *ValueCellOffset,
		     PWCHAR ValueName)
{
  PCM_KEY_VALUE NewValueCell;
  ULONG NameSize;
  BOOLEAN Packable = TRUE;
  ULONG i;

  NameSize = (ValueName == NULL) ? 0 : wcslen (ValueName);
  for (i = 0; i < NameSize; i++)
    {
      if (ValueName[i] & 0xFF00)
        {
          NameSize *= sizeof(WCHAR);
          Packable = FALSE;
          break;
        }
    }
  *ValueCellOffset = HvAllocateCell (Hive, sizeof(CM_KEY_VALUE) + NameSize, Stable, HCELL_NIL);
  if (*ValueCellOffset == HCELL_NIL)
    {
      DbgPrint((DPRINT_REGISTRY, "CmiAllocateCell() failed\n"));
      return FALSE;
    }

  NewValueCell = (PCM_KEY_VALUE) HvGetCell (Hive, *ValueCellOffset);
  NewValueCell->Id = REG_VALUE_CELL_ID;
  NewValueCell->NameSize = NameSize;
  NewValueCell->Flags = 0;
  if (NameSize > 0)
    {
      if (Packable)
        {
          for (i = 0; i < NameSize; i++)
            {
              ((PCHAR)NewValueCell->Name)[i] = (CHAR)ValueName[i];
            }
          NewValueCell->Flags |= REG_VALUE_NAME_PACKED;
        }
      else
        {
          memcpy (NewValueCell->Name,
	          ValueName,
	          NameSize);
        }
    }
  NewValueCell->DataType = 0;
  NewValueCell->DataSize = 0;
  NewValueCell->DataOffset = -1;

  *ValueCell = NewValueCell;

  return TRUE;
}


static BOOLEAN
CmiAddValueToKeyValueList(PHHIVE Hive,
			  HCELL_INDEX KeyCellOffset,
			  HCELL_INDEX ValueCellOffset)
{
  PVALUE_LIST_CELL ValueListCell;
  PCM_KEY_NODE KeyCell;

  KeyCell = (PCM_KEY_NODE)HvGetCell (Hive, KeyCellOffset);
  if (KeyCell == NULL)
    {
      DbgPrint((DPRINT_REGISTRY, "HvGetCell() failed\n"));
      return FALSE;
    }

  ValueListCell = (PVALUE_LIST_CELL)HvGetCell (Hive, KeyCell->ValueList.List);
  if (ValueListCell == NULL)
    {
      DbgPrint((DPRINT_REGISTRY, "HvGetCell() failed\n"));
      return FALSE;
    }

  ValueListCell->ValueOffset[KeyCell->ValueList.Count] = ValueCellOffset;
  KeyCell->ValueList.Count++;

  return TRUE;
}

static BOOLEAN
CmiExportValue (PHHIVE Hive,
		HCELL_INDEX KeyCellOffset,
		FRLDRHKEY Key,
		PVALUE Value)
{
  HCELL_INDEX ValueCellOffset;
  HCELL_INDEX DataCellOffset;
  PCM_KEY_VALUE ValueCell;
  PVOID DataCell;
  ULONG DataSize;
  ULONG DataType;
  PCHAR Data;

  DbgPrint((DPRINT_REGISTRY, "CmiExportValue('%S') called\n",
	   (Value == NULL) ? "<default>" : (PCHAR)Value->Name));
  DbgPrint((DPRINT_REGISTRY, "DataSize %lu\n",
	   (Value == NULL) ? Key->DataSize : Value->DataSize));

  /* Allocate value cell */
  if (!CmiAllocateValueCell(Hive, &ValueCell, &ValueCellOffset, (Value == NULL) ? NULL : Value->Name))
    {
      return FALSE;
    }

  if (!CmiAddValueToKeyValueList(Hive, KeyCellOffset, ValueCellOffset))
    {
      return FALSE;
    }

  if (Value == NULL)
    {
      DataType = Key->DataType;
      DataSize = Key->DataSize;
      Data = Key->Data;
    }
  else
    {
      DataType = Value->DataType;
      DataSize = Value->DataSize;
      Data = Value->Data;
    }

  if (DataSize <= sizeof(HCELL_INDEX))
    {
      ValueCell->DataSize = DataSize | REG_DATA_IN_OFFSET;
      ValueCell->DataType = DataType;
      memcpy (&ValueCell->DataOffset,
	      &Data,
	      DataSize);
    }
  else
    {
      /* Allocate data cell */
      DataCellOffset = HvAllocateCell (Hive, DataSize, Stable, HCELL_NIL);
      if (DataCellOffset == HCELL_NIL)
	{
	  return FALSE;
	}

      ValueCell->DataOffset = DataCellOffset;
      ValueCell->DataSize = DataSize;
      ValueCell->DataType = DataType;

      DataCell = (PVOID)HvGetCell (Hive, DataCellOffset);
      memcpy (DataCell,
	      Data,
	      DataSize);
    }

  return TRUE;
}


static BOOLEAN
CmiExportSubKey (PHHIVE Hive,
		 HCELL_INDEX Parent,
		 FRLDRHKEY ParentKey,
		 FRLDRHKEY Key)
{
  HCELL_INDEX NKBOffset;
  PCM_KEY_NODE NewKeyCell;
  ULONG KeyCellSize;
  ULONG SubKeyCount;
  ULONG ValueCount;
  PLIST_ENTRY Entry;
  FRLDRHKEY SubKey;
  PVALUE Value;
  BOOLEAN Packable = TRUE;
  ULONG i;
  ULONG NameSize;

  DbgPrint((DPRINT_REGISTRY, "CmiExportSubKey('%S') called\n", Key->Name));

  /* Don't export links */
  if (Key->DataType == REG_LINK)
    return TRUE;

  NameSize = (Key->NameSize - sizeof(WCHAR)) / sizeof(WCHAR);
  for (i = 0; i < NameSize; i++)
    {
      if (Key->Name[i] & 0xFF00)
        {
          Packable = FALSE;
          NameSize *= sizeof(WCHAR);
          break;
        }
    }

  /* Allocate key cell */
  KeyCellSize = sizeof(CM_KEY_NODE) + NameSize;
  NKBOffset = HvAllocateCell (Hive, KeyCellSize, Stable, HCELL_NIL);
  if (NKBOffset == HCELL_NIL)
    {
      DbgPrint((DPRINT_REGISTRY, "HvAllocateCell() failed\n"));
      return FALSE;
    }

  /* Initialize key cell */
  NewKeyCell = (PCM_KEY_NODE) HvGetCell (Hive, NKBOffset);
  NewKeyCell->Id = REG_KEY_CELL_ID;
  NewKeyCell->Flags = 0;
  NewKeyCell->LastWriteTime.QuadPart = 0ULL;
  NewKeyCell->Parent = Parent;
  NewKeyCell->SubKeyCounts[Stable] = 0;
  NewKeyCell->SubKeyLists[Stable] = -1;
  NewKeyCell->ValueList.Count = 0;
  NewKeyCell->ValueList.List = -1;
  NewKeyCell->SecurityKeyOffset = -1;
  NewKeyCell->ClassNameOffset = -1;
  NewKeyCell->NameSize = NameSize;
  NewKeyCell->ClassSize = 0;
  if (Packable)
    {
      for (i = 0; i < NameSize; i++)
        {
          ((PCHAR)NewKeyCell->Name)[i] = (CHAR)Key->Name[i];
        }
      NewKeyCell->Flags |= REG_KEY_NAME_PACKED;

    }
  else
    {
      memcpy (NewKeyCell->Name,
	      Key->Name,
	      NameSize);
    }

  /* Add key cell to the parent key's hash table */
  if (!CmiAddKeyToParentHashTable (Hive,
				   Parent,
				   NewKeyCell,
				   NKBOffset))
    {
      DbgPrint((DPRINT_REGISTRY, "CmiAddKeyToParentHashTable() failed\n"));
      return FALSE;
    }

  ValueCount = RegGetValueCount (Key);
  DbgPrint((DPRINT_REGISTRY, "ValueCount: %u\n", ValueCount));
  if (ValueCount > 0)
    {
      /* Allocate value list cell */
      if (!CmiAllocateValueListCell (Hive,
				     &NewKeyCell->ValueList.List,
				     ValueCount))
	{
	  DbgPrint((DPRINT_REGISTRY, "CmiAllocateValueListCell() failed\n"));
	  return FALSE;
	}

      if (Key->DataSize != 0)
	{
	  if (!CmiExportValue (Hive, NKBOffset, Key, NULL))
	    return FALSE;
	}

      /* Enumerate values */
      Entry = Key->ValueList.Flink;
      while (Entry != &Key->ValueList)
	{
	  Value = CONTAINING_RECORD(Entry,
				    VALUE,
				    ValueList);

	  if (!CmiExportValue (Hive, NKBOffset, Key, Value))
	    return FALSE;

	  Entry = Entry->Flink;
	}
    }

  SubKeyCount = RegGetSubKeyCount (Key);
  DbgPrint((DPRINT_REGISTRY, "SubKeyCount: %u\n", SubKeyCount));
  if (SubKeyCount > 0)
    {
      /* Allocate hash table cell */
      if (!CmiAllocateHashTableCell (Hive,
				     &NewKeyCell->SubKeyLists[Stable],
				     SubKeyCount))
	{
	  DbgPrint((DPRINT_REGISTRY, "CmiAllocateHashTableCell() failed\n"));
	  return FALSE;
	}

      /* Enumerate subkeys */
      Entry = Key->SubKeyList.Flink;
      while (Entry != &Key->SubKeyList)
	{
	  SubKey = CONTAINING_RECORD(Entry,
				     KEY,
				     KeyList);

	  if (!CmiExportSubKey (Hive, NKBOffset, Key, SubKey))
	    return FALSE;

	  Entry = Entry->Flink;
	}
    }

  return TRUE;
}


static BOOLEAN
CmiExportHive (PHHIVE Hive,
	       PCWSTR KeyName)
{
  PCM_KEY_NODE KeyCell;
  FRLDRHKEY Key;
  ULONG SubKeyCount;
  ULONG ValueCount;
  PLIST_ENTRY Entry;
  FRLDRHKEY SubKey;
  PVALUE Value;

  DbgPrint((DPRINT_REGISTRY, "CmiExportHive(%x, '%S') called\n", Hive, KeyName));

  if (RegOpenKey (NULL, KeyName, &Key) != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_REGISTRY, "RegOpenKey() failed\n"));
      return FALSE;
    }

  KeyCell = (PCM_KEY_NODE)HvGetCell (Hive, Hive->BaseBlock->RootCell);
  if (KeyCell == NULL)
    {
      DbgPrint((DPRINT_REGISTRY, "HvGetCell() failed\n"));
      return FALSE;
    }

  ValueCount = RegGetValueCount (Key);
  DbgPrint((DPRINT_REGISTRY, "ValueCount: %u\n", ValueCount));
  if (ValueCount > 0)
    {
      /* Allocate value list cell */
      if (!CmiAllocateValueListCell (Hive,
				     &KeyCell->ValueList.List,
				     ValueCount))
	{
	  DbgPrint((DPRINT_REGISTRY, "CmiAllocateValueListCell() failed\n"));
	  return FALSE;
	}

      if (Key->DataSize != 0)
	{
	  if (!CmiExportValue (Hive, Hive->BaseBlock->RootCell, Key, NULL))
	    return FALSE;
	}

      /* Enumerate values */
      Entry = Key->ValueList.Flink;
      while (Entry != &Key->ValueList)
	{
	  Value = CONTAINING_RECORD(Entry,
				    VALUE,
				    ValueList);

	  if (!CmiExportValue (Hive, Hive->BaseBlock->RootCell, Key, Value))
	    return FALSE;

	  Entry = Entry->Flink;
	}
    }

  SubKeyCount = RegGetSubKeyCount (Key);
  DbgPrint((DPRINT_REGISTRY, "SubKeyCount: %u\n", SubKeyCount));
  if (SubKeyCount > 0)
    {
      /* Allocate hash table cell */
      if (!CmiAllocateHashTableCell (Hive,
				     &KeyCell->SubKeyLists[Stable],
				     SubKeyCount))
	{
	  DbgPrint((DPRINT_REGISTRY, "CmiAllocateHashTableCell() failed\n"));
	  return FALSE;
	}

      /* Enumerate subkeys */
      Entry = Key->SubKeyList.Flink;
      while (Entry != &Key->SubKeyList)
	{
	  SubKey = CONTAINING_RECORD(Entry,
				     KEY,
				     KeyList);

	  if (!CmiExportSubKey (Hive, Hive->BaseBlock->RootCell, Key, SubKey))
	    return FALSE;

	  Entry = Entry->Flink;
	}
    }

  return TRUE;
}


static BOOLEAN
RegImportValue (PHHIVE Hive,
		PCM_KEY_VALUE ValueCell,
		FRLDRHKEY Key)
{
  PVOID DataCell;
  PWCHAR wName;
  LONG Error;
  ULONG DataSize;
  ULONG i;

  if (ValueCell->Id != REG_VALUE_CELL_ID)
    {
      DbgPrint((DPRINT_REGISTRY, "Invalid key cell!\n"));
      return FALSE;
    }

  if (ValueCell->Flags & REG_VALUE_NAME_PACKED)
    {
      wName = MmAllocateMemory ((ValueCell->NameSize + 1)*sizeof(WCHAR));
      for (i = 0; i < ValueCell->NameSize; i++)
        {
          wName[i] = ((PCHAR)ValueCell->Name)[i];
        }
      wName[ValueCell->NameSize] = 0;
    }
  else
    {
      wName = MmAllocateMemory (ValueCell->NameSize + sizeof(WCHAR));
      memcpy (wName,
	      ValueCell->Name,
	      ValueCell->NameSize);
      wName[ValueCell->NameSize / sizeof(WCHAR)] = 0;
    }

  DataSize = ValueCell->DataSize & REG_DATA_SIZE_MASK;

  DbgPrint((DPRINT_REGISTRY, "ValueName: '%S'\n", wName));
  DbgPrint((DPRINT_REGISTRY, "DataSize: %u\n", DataSize));

  if (DataSize <= sizeof(HCELL_INDEX) && (ValueCell->DataSize & REG_DATA_IN_OFFSET))
    {
      Error = RegSetValue(Key,
			  wName,
			  ValueCell->DataType,
			  (PCHAR)&ValueCell->DataOffset,
			  DataSize);
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_REGISTRY, "RegSetValue() failed!\n"));
	  MmFreeMemory (wName);
	  return FALSE;
	}
    }
  else
    {
      DataCell = (PVOID)HvGetCell (Hive, ValueCell->DataOffset);
      DbgPrint((DPRINT_REGISTRY, "DataCell: %x\n", DataCell));

      Error = RegSetValue (Key,
			   wName,
			   ValueCell->DataType,
			   DataCell,
			   DataSize);

      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_REGISTRY, "RegSetValue() failed!\n"));
	  MmFreeMemory (wName);
	  return FALSE;
	}
    }

  MmFreeMemory (wName);

  return TRUE;
}


static BOOLEAN
RegImportSubKey(PHHIVE Hive,
		PCM_KEY_NODE KeyCell,
		FRLDRHKEY ParentKey)
{
  PHASH_TABLE_CELL HashCell;
  PCM_KEY_NODE SubKeyCell;
  PVALUE_LIST_CELL ValueListCell;
  PCM_KEY_VALUE ValueCell = NULL;
  PWCHAR wName;
  FRLDRHKEY SubKey;
  LONG Error;
  ULONG i;


  DbgPrint((DPRINT_REGISTRY, "KeyCell: %x\n", KeyCell));
  DbgPrint((DPRINT_REGISTRY, "KeyCell->Id: %x\n", KeyCell->Id));
  if (KeyCell->Id != REG_KEY_CELL_ID)
    {
      DbgPrint((DPRINT_REGISTRY, "Invalid key cell id!\n"));
      return FALSE;
    }

  if (KeyCell->Flags & REG_KEY_NAME_PACKED)
    {
      wName = MmAllocateMemory ((KeyCell->NameSize + 1) * sizeof(WCHAR));
      for (i = 0; i < KeyCell->NameSize; i++)
        {
          wName[i] = ((PCHAR)KeyCell->Name)[i];
        }
      wName[KeyCell->NameSize] = 0;
    }
  else
    {
      wName = MmAllocateMemory (KeyCell->NameSize + sizeof(WCHAR));
      memcpy (wName,
	      KeyCell->Name,
	      KeyCell->NameSize);
      wName[KeyCell->NameSize/sizeof(WCHAR)] = 0;
    }

  DbgPrint((DPRINT_REGISTRY, "KeyName: '%S'\n", wName));

  /* Create new sub key */
  Error = RegCreateKey (ParentKey,
			wName,
			&SubKey);
  MmFreeMemory (wName);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_REGISTRY, "RegCreateKey() failed!\n"));
      return FALSE;
    }
  DbgPrint((DPRINT_REGISTRY, "Subkeys: %u\n", KeyCell->SubKeyCounts));
  DbgPrint((DPRINT_REGISTRY, "Values: %u\n", KeyCell->ValueList.Count));

  /* Enumerate and add values */
  if (KeyCell->ValueList.Count > 0)
    {
      ValueListCell = (PVALUE_LIST_CELL) HvGetCell (Hive, KeyCell->ValueList.List);
      DbgPrint((DPRINT_REGISTRY, "ValueListCell: %x\n", ValueListCell));

      for (i = 0; i < KeyCell->ValueList.Count; i++)
	{
	  DbgPrint((DPRINT_REGISTRY, "ValueOffset[%d]: %x\n", i, ValueListCell->ValueOffset[i]));

	  ValueCell = (PCM_KEY_VALUE) HvGetCell (Hive, ValueListCell->ValueOffset[i]);

	  DbgPrint((DPRINT_REGISTRY, "ValueCell[%d]: %x\n", i, ValueCell));

	  if (!RegImportValue(Hive, ValueCell, SubKey))
	    return FALSE;
	}
    }

  /* Enumerate and add subkeys */
  if (KeyCell->SubKeyCounts[Stable] > 0)
    {
      HashCell = (PHASH_TABLE_CELL) HvGetCell (Hive, KeyCell->SubKeyLists[Stable]);
      DbgPrint((DPRINT_REGISTRY, "HashCell: %x\n", HashCell));
      DbgPrint((DPRINT_REGISTRY, "SubKeyCounts: %x\n", KeyCell->SubKeyCounts));

      for (i = 0; i < KeyCell->SubKeyCounts[Stable]; i++)
	{
	  DbgPrint((DPRINT_REGISTRY, "KeyOffset[%d]: %x\n", i, HashCell->Table[i].KeyOffset));

	  SubKeyCell = (PCM_KEY_NODE) HvGetCell (Hive, HashCell->Table[i].KeyOffset);

	  DbgPrint((DPRINT_REGISTRY, "SubKeyCell[%d]: %x\n", i, SubKeyCell));

	  if (!RegImportSubKey(Hive, SubKeyCell, SubKey))
	    return FALSE;
	}
    }

  return TRUE;
}


BOOLEAN
RegImportBinaryHive(PCHAR ChunkBase,
		    ULONG ChunkSize)
{
  PCM_KEY_NODE KeyCell;
  PHASH_TABLE_CELL HashCell;
  PCM_KEY_NODE SubKeyCell;
  FRLDRHKEY SystemKey;
  ULONG i;
  LONG Error;
  PEREGISTRY_HIVE CmHive;
  PHHIVE Hive;
  NTSTATUS Status;

  DbgPrint((DPRINT_REGISTRY, "RegImportBinaryHive(%x, %u) called\n",ChunkBase,ChunkSize));

  CmHive = CmpAllocate(sizeof(EREGISTRY_HIVE), TRUE, 0);
  Status = HvInitialize (&CmHive->Hive,
                         HINIT_FLAT,
                         0,
                         0,
                         ChunkBase, 
                         CmpAllocate,
                         CmpFree,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         1,
                         NULL);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint((DPRINT_REGISTRY, "Invalid hive id!\n"));
      return FALSE;
    }

  Hive = &CmHive->Hive;
  KeyCell = (PCM_KEY_NODE)HvGetCell (Hive, Hive->BaseBlock->RootCell);
  DbgPrint((DPRINT_REGISTRY, "KeyCell: %x\n", KeyCell));
  DbgPrint((DPRINT_REGISTRY, "KeyCell->Id: %x\n", KeyCell->Id));
  if (KeyCell->Id != REG_KEY_CELL_ID)
    {
      DbgPrint((DPRINT_REGISTRY, "Invalid key cell id!\n"));
      return FALSE;
    }

  DbgPrint((DPRINT_REGISTRY, "Subkeys: %u\n", KeyCell->SubKeyCounts));
  DbgPrint((DPRINT_REGISTRY, "Values: %u\n", KeyCell->ValueList.Count));

  /* Open 'System' key */
  Error = RegOpenKey(NULL,
		     L"\\Registry\\Machine\\SYSTEM",
		     &SystemKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_REGISTRY, "Failed to open 'system' key!\n"));
      return FALSE;
    }

  /* Enumerate and add subkeys */
  if (KeyCell->SubKeyCounts[Stable] > 0)
    {
      HashCell = (PHASH_TABLE_CELL)HvGetCell (Hive, KeyCell->SubKeyLists[Stable]);
      DbgPrint((DPRINT_REGISTRY, "HashCell: %x\n", HashCell));
      DbgPrint((DPRINT_REGISTRY, "SubKeyCounts: %x\n", KeyCell->SubKeyCounts[Stable]));

      for (i = 0; i < KeyCell->SubKeyCounts[Stable]; i++)
	{
	  DbgPrint((DPRINT_REGISTRY, "KeyOffset[%d]: %x\n", i, HashCell->Table[i].KeyOffset));

	  SubKeyCell = (PCM_KEY_NODE)HvGetCell (Hive, HashCell->Table[i].KeyOffset);

	  DbgPrint((DPRINT_REGISTRY, "SubKeyCell[%d]: %x\n", i, SubKeyCell));

	  if (!RegImportSubKey(Hive, SubKeyCell, SystemKey))
	    return FALSE;
	}
    }

  return TRUE;
}


static VOID
CmiWriteHive(PHHIVE Hive,
             PCHAR ChunkBase,
             ULONG* ChunkSize)
{
  PHBIN Bin;
  ULONG i, Size;

  /* Write hive header */
  memcpy (ChunkBase, Hive->BaseBlock, HV_BLOCK_SIZE);
  Size = HV_BLOCK_SIZE;

  Bin = NULL;
  for (i = 0; i < Hive->Storage[Stable].Length; i++)
    {
      if (Hive->Storage[Stable].BlockList[i].BinAddress != (ULONG_PTR)Bin)
	{
	  Bin = (PHBIN)Hive->Storage[Stable].BlockList[i].BinAddress;
	  memcpy (ChunkBase + (i + 1) * HV_BLOCK_SIZE,
	          Bin, Bin->Size);
	  Size += Bin->Size;
	}
    }

  DbgPrint((DPRINT_REGISTRY, "ChunkSize: %x\n", Size));

  *ChunkSize = Size;
}


BOOLEAN
RegExportBinaryHive(PCWSTR KeyName,
		    PCHAR ChunkBase,
		    ULONG* ChunkSize)
{
  PEREGISTRY_HIVE CmHive;
  PHHIVE Hive;
  NTSTATUS Status;

  DbgPrint((DPRINT_REGISTRY, "Creating binary hardware hive\n"));

  CmHive = CmpAllocate(sizeof(EREGISTRY_HIVE), TRUE, 0);
  Status = HvInitialize (&CmHive->Hive,
                         HINIT_CREATE,
                         0,
                         0,
                         0,
                         CmpAllocate,
                         CmpFree,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         0,
                         NULL);
  Hive = &CmHive->Hive;
  if (!NT_SUCCESS(Status))
    {
      return FALSE;
    }

  /* Init root key cell */
  if (!CmCreateRootNode (Hive, KeyName))
    {
      HvFree (Hive);
      return FALSE;
    }

  if (!CmiExportHive (Hive, KeyName))
    {
      HvFree (Hive);
      return FALSE;
    }

  CmiWriteHive (Hive, ChunkBase, ChunkSize);

  HvFree (Hive);

  return TRUE;
}

/* EOF */

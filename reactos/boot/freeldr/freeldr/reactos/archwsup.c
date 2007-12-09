/*
 * PROJECT:         ReactOS Boot Loader (FreeLDR)
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/freeldr/reactos/archwsup.c
 * PURPOSE:         Routines for ARC Hardware Tree and Configuration Data
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

extern CHAR reactos_arc_hardware_data[];
ULONG FldrpHwHeapLocation;
PCONFIGURATION_COMPONENT_DATA FldrArcHwTreeRoot;

/* FUNCTIONS ******************************************************************/

PVOID
NTAPI
FldrpHwHeapAlloc(IN ULONG Size)
{
    PVOID Buffer;
    
    /* Return a block of memory from the ARC Hardware Heap */
    Buffer = &reactos_arc_hardware_data[FldrpHwHeapLocation];
    
    /* Increment the heap location */
    FldrpHwHeapLocation += Size;
    if (FldrpHwHeapLocation > HW_MAX_ARC_HEAP_SIZE) Buffer = NULL;
    
    /* Return the buffer */
    return Buffer;
}

VOID
NTAPI
FldrSetComponentInformation(IN PCONFIGURATION_COMPONENT_DATA ComponentData,
                            IN IDENTIFIER_FLAG Flags,
                            IN ULONG Key,
                            IN ULONG Affinity)
{
    LONG Error;
    PCONFIGURATION_COMPONENT Component = &ComponentData->ComponentEntry;

    /* Set component information */
    Component->Flags = Flags;
    Component->Version = 0;
    Component->Revision = 0;
    //Component->Key = Key; // HACK: We store the registry key here
    Component->AffinityMask = Affinity;
    
    /* Set the value */
    Error = RegSetValue((FRLDRHKEY)Component->Key,
                        L"Component Information",
                        REG_BINARY,
                        (PVOID)&Component->Flags,
                        FIELD_OFFSET(CONFIGURATION_COMPONENT, ConfigurationDataLength) -
                        FIELD_OFFSET(CONFIGURATION_COMPONENT, Flags));
    if (Error != ERROR_SUCCESS)
    {
        DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", Error));
    }
}

VOID
NTAPI
FldrSetIdentifier(IN PCONFIGURATION_COMPONENT_DATA ComponentData,
                  IN PWCHAR IdentifierString)
{
    LONG Error;
    ULONG IdentifierLength;
    PCONFIGURATION_COMPONENT Component = &ComponentData->ComponentEntry;
    PCHAR Identifier;
    
    /* Allocate memory for the identifier */
    /* WARNING: This should be ASCII data */
    IdentifierLength = (wcslen(IdentifierString) + 1) * sizeof(WCHAR);
    Identifier = FldrpHwHeapAlloc(IdentifierLength);
    if (!Identifier) return;
    
    /* Copy the identifier */
    RtlCopyMemory(Identifier, IdentifierString, IdentifierLength);
    
    /* Set component information */
    Component->IdentifierLength = IdentifierLength;
    Component->Identifier = Identifier;

    /* Set the key */
    Error = RegSetValue((FRLDRHKEY)Component->Key,
                        L"Identifier",
                        REG_SZ,
                        (PCHAR)IdentifierString,
                        IdentifierLength);
    if (Error != ERROR_SUCCESS)
    {
        DbgPrint((DPRINT_HWDETECT, "RegSetValue() failed (Error %u)\n", Error));
        return;
    }
}

VOID
NTAPI
FldrCreateSystemKey(OUT PCONFIGURATION_COMPONENT_DATA *SystemNode)
{
    LONG Error;
    PCONFIGURATION_COMPONENT Component;
    
    /* Allocate the root */
    FldrArcHwTreeRoot = FldrpHwHeapAlloc(sizeof(CONFIGURATION_COMPONENT_DATA));
    if (!FldrArcHwTreeRoot) return;
    
    /* Set it up */
    Component = &FldrArcHwTreeRoot->ComponentEntry;
    Component->Class = SystemClass;
    Component->Type = MaximumType;
    Component->Version = 0;
    Component->Key = 0;
    Component->AffinityMask = 0;
    Component->ConfigurationDataLength = 0;
    Component->Identifier = 0;
    Component->IdentifierLength = 0;
    
    /* Return the node */
    *SystemNode = FldrArcHwTreeRoot;

    /* Create the key */
    Error = RegCreateKey(NULL,
                         L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System",
                         (FRLDRHKEY*)&Component->Key);
    if (Error != ERROR_SUCCESS)
    {
        DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", Error));
        return;
    }
}

VOID
NTAPI
FldrCreateComponentKey(IN PCONFIGURATION_COMPONENT_DATA SystemNode,
                       IN PWCHAR BusName,
                       IN ULONG BusNumber,
                       IN CONFIGURATION_CLASS Class,
                       IN CONFIGURATION_TYPE Type,
                       OUT PCONFIGURATION_COMPONENT_DATA *ComponentKey)
{
    LONG Error;
    WCHAR Buffer[80];
    PCONFIGURATION_COMPONENT_DATA ComponentData;
    PCONFIGURATION_COMPONENT Component;

    /* Allocate the node for this component */
    ComponentData = FldrpHwHeapAlloc(sizeof(CONFIGURATION_COMPONENT_DATA));
    if (!ComponentData) return;
    
    /* Now save our parent */
    ComponentData->Parent = SystemNode;
    
    /* Now we need to figure out if the parent already has a child entry */
    if (SystemNode->Child)
    {
        /* It does, so we'll be a sibling of the child instead */
        SystemNode->Child->Sibling = ComponentData;
    }
    else
    {
        /* It doesn't, so we will be the first child */
        SystemNode->Child = ComponentData;
    }
    
    /* Set us up */
    Component = &ComponentData->ComponentEntry;
    Component->Class = Class;
    Component->Type = Type;
    
    /* Return the child */
    *ComponentKey = ComponentData;
       
    /* Build the key name */
    swprintf(Buffer, L"%s\\%u", BusName, BusNumber);
    
    /* Create the key */
    Error = RegCreateKey((FRLDRHKEY)SystemNode->ComponentEntry.Key,
                         Buffer,
                         (FRLDRHKEY*)&Component->Key);
    if (Error != ERROR_SUCCESS)
    {
        DbgPrint((DPRINT_HWDETECT, "RegCreateKey() failed (Error %u)\n", Error));
        return;
    }    
}

VOID
NTAPI
FldrSetConfigurationData(IN PCONFIGURATION_COMPONENT_DATA ComponentData,
                         IN PVOID Data,
                         IN ULONG Size)
{
    LONG Error;
    PCONFIGURATION_COMPONENT Component = &ComponentData->ComponentEntry;
    PVOID ConfigurationData;

    /* Allocate a buffer from the hardware heap */
    ConfigurationData = FldrpHwHeapAlloc(Size);
    if (!ConfigurationData) return;

    /* Copy component information */
    RtlCopyMemory(ConfigurationData, Data, Size);
        
    /* Set component information */
    ComponentData->ConfigurationData = ConfigurationData;
    Component->ConfigurationDataLength = Size;
        
    /* Set 'Configuration Data' value */
    Error = RegSetValue((FRLDRHKEY)Component->Key,
                        L"Configuration Data",
                        REG_FULL_RESOURCE_DESCRIPTOR,
                        Data,
                        Size);
    if (Error != ERROR_SUCCESS)
    {
        DbgPrint((DPRINT_HWDETECT,
                  "RegSetValue(Configuration Data) failed (Error %u)\n",
                  Error));
    }    
}

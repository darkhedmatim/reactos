/*
 * PROJECT:        ReactOS Kernel
 * LICENSE:        GNU GPLv2 only as published by the Free Software Foundation
 * PURPOSE:        To Implement AHCI Miniport driver targeting storport NT 5.2
 * PROGRAMMERS:    Aman Priyadarshi (aman.eureka@gmail.com)
 */

#include "storahci.h"

BOOLEAN AhciAdapterReset(
  __in      PAHCI_ADAPTER_EXTENSION             adapterExtension
);

VOID AhciZeroMemory(
  __in      PCHAR                               buffer,
  __in      ULONG                               bufferSize
);

/**
 * @name AhciPortInitialize
 * @implemented
 *
 * Initialize port by setting up PxCLB & PxFB Registers
 *
 * @param portExtension
 *
 * @return
 * Return true if intialization was successful
 */
BOOLEAN AhciPortInitialize(
  __in      PAHCI_PORT_EXTENSION                portExtension
)
{
    ULONG mappedLength;
    PAHCI_MEMORY_REGISTERS abar;
    PAHCI_ADAPTER_EXTENSION adapterExtension;
    STOR_PHYSICAL_ADDRESS commandListPhysical, receivedFISPhysical;

    StorPortDebugPrint(0, "AhciPortInitialize()\n");

    adapterExtension = portExtension->AdapterExtension;
    abar = adapterExtension->ABAR_Address;
    portExtension->Port = &abar->PortList[portExtension->PortNumber];

    commandListPhysical = StorPortGetPhysicalAddress(adapterExtension, NULL, portExtension->CommandList, &mappedLength);
    if (mappedLength == 0 || (commandListPhysical.LowPart % 1024) != 0){
        StorPortDebugPrint(0, "\tcommandListPhysical mappedLength:%d\n", mappedLength);
        return FALSE;
    }

    receivedFISPhysical = StorPortGetPhysicalAddress(adapterExtension, NULL, portExtension->ReceivedFIS, &mappedLength);
    if (mappedLength == 0 || (receivedFISPhysical.LowPart % 256) != 0){
        StorPortDebugPrint(0, "\treceivedFISPhysical mappedLength:%d\n", mappedLength);
        return FALSE;
    }

    // 10.1.2 For each implemented port, system software shall allocate memory for and program:
    //  PxCLB and PxCLBU (if CAP.S64A is set to ‘1’)
    //  PxFB and PxFBU (if CAP.S64A is set to ‘1’)
    //Note: Assuming 32bit support only
    StorPortWriteRegisterUlong(adapterExtension, &portExtension->Port->CLB, commandListPhysical.LowPart);
    StorPortWriteRegisterUlong(adapterExtension, &portExtension->Port->FB, receivedFISPhysical.LowPart);

    return TRUE;
}// -- AhciPortInitialize();

/**
 * @name AhciAllocateResourceForAdapter
 * @implemented
 *
 * Allocate memory from poll for required pointers
 *
 * @param adapterExtension
 * @param ConfigInfo
 *
 * @return
 * return TRUE if allocation was successful
 */
BOOLEAN AhciAllocateResourceForAdapter(
  __in      PAHCI_ADAPTER_EXTENSION             adapterExtension,
  __in      PPORT_CONFIGURATION_INFORMATION     ConfigInfo
)
{
    PVOID portsExtension = NULL;
    PCHAR nonCachedExtension;
    ULONG portCount, portImplemented, status, index, NCS, AlignedNCS, nonCachedExtensionSize, currentCount;

    StorPortDebugPrint(0, "AhciAllocateResourceForAdapter()\n");

    // 3.1.1 NCS = CAP[12:08] -> Align 
    NCS = (adapterExtension->CAP & 0xF00) >> 8;
    AlignedNCS = ((NCS/8) + 1) * 8;

    // get port count -- Number of set bits in `adapterExtension->PortImplemented`
    portCount = 0;
    portImplemented = adapterExtension->PortImplemented;
    while(portImplemented > 0)
    {
        portCount++;
        portImplemented &= (portImplemented-1);
    }
    StorPortDebugPrint(0, "\tPort Count: %d\n", portCount);

    nonCachedExtensionSize =    sizeof(AHCI_COMMAND_HEADER) * AlignedNCS + //should be 1K aligned
                                sizeof(AHCI_RECEIVED_FIS);
    //align nonCachedExtensionSize to 1K
    nonCachedExtensionSize = (((nonCachedExtensionSize - 1) / 0x400) + 1) * 0x400;
    nonCachedExtensionSize *= portCount;

    adapterExtension->NonCachedExtension = StorPortGetUncachedExtension(adapterExtension, ConfigInfo, nonCachedExtensionSize);
    if (adapterExtension->NonCachedExtension == NULL) {
        StorPortDebugPrint(0, "\tadapterExtension->NonCachedExtension == NULL\n");
        return FALSE;
    }

    nonCachedExtension = (PCHAR)adapterExtension->NonCachedExtension;

    AhciZeroMemory(nonCachedExtension, nonCachedExtensionSize);
    

    // allocate memory for port extension
    /* --> Allocate memory for port extension, but right now it is returning STOR_STATUS_NOT_IMPLEMENTED
    so, for testing purpose, I allocated during driver entry itself.
    status = StorPortAllocatePool(
                    adapterExtension, 
                    portCount * sizeof(AHCI_PORT_EXTENSION),
                    AHCI_POOL_TAG, 
                    (PVOID*)&portsExtension);

    if (status != STOR_STATUS_SUCCESS){
        StorPortDebugPrint(0, "\tstatus : %x\n", status);
        return FALSE;
    }

    AhciZeroMemory((PCHAR)portsExtension, portCount * sizeof(AHCI_PORT_EXTENSION));
    */
    nonCachedExtensionSize /= portCount;
    currentCount = 0;
    for (index = 0; index < 32; index++)
    {
        if ((adapterExtension->PortImplemented & (1<<index)) != 0)
        {
            //adapterExtension->PortExtension[index] = (PAHCI_PORT_EXTENSION)((PCHAR)portsExtension + sizeof(AHCI_PORT_EXTENSION) * currentCount);

            adapterExtension->PortExtension[index].PortNumber = index;
            adapterExtension->PortExtension[index].AdapterExtension = adapterExtension;
            adapterExtension->PortExtension[index].CommandList = (PAHCI_COMMAND_HEADER)(nonCachedExtension + (currentCount*nonCachedExtensionSize));
            adapterExtension->PortExtension[index].ReceivedFIS = (PAHCI_RECEIVED_FIS)((PCHAR)adapterExtension->PortExtension[index].CommandList + sizeof(AHCI_COMMAND_HEADER) * AlignedNCS);
            currentCount++;
        }
    }

    return TRUE;
}// -- AhciAllocateResourceForAdapter();

/**
 * @name AhciHwInitialize
 * @implemented
 *
 * initializes the HBA and finds all devices that are of interest to the miniport driver.
 *
 * @param adapterExtension
 *
 * @return
 * return TRUE if intialization was successful
 */
BOOLEAN AhciHwInitialize(
  __in      PVOID                               AdapterExtension
)
{
    PAHCI_ADAPTER_EXTENSION adapterExtension;

    StorPortDebugPrint(0, "AhciHwInitialize()\n");

    adapterExtension = (PAHCI_ADAPTER_EXTENSION)AdapterExtension;

    return TRUE;
}// -- AhciHwInitialize();

/**
 * @name AhciHwInterrupt
 * @implemented
 *
 * The Storport driver calls the HwStorInterrupt routine after the HBA generates an interrupt request.
 *
 * @param adapterExtension
 *
 * @return
 * return TRUE Indicates that an interrupt was pending on adapter.
 * return FALSE Indicates the interrupt was not ours.
 */
BOOLEAN AhciHwInterrupt(
  __in      PVOID                               AdapterExtension
)
{
    PAHCI_ADAPTER_EXTENSION adapterExtension;

    StorPortDebugPrint(0, "AhciHwInterrupt()\n");

    adapterExtension = (PAHCI_ADAPTER_EXTENSION)AdapterExtension;

    return TRUE;
}// -- AhciHwInterrupt();

/**
 * @name AhciHwStartIo
 * @implemented
 *
 * The Storport driver calls the HwStorStartIo routine one time for each incoming I/O request.
 *
 * @param adapterExtension
 * @param Srb
 *
 * @return
 * return TRUE if the request was accepted
 * return FALSE if the request must be submitted later
 */
BOOLEAN AhciHwStartIo(
  __in      PVOID                               AdapterExtension,
  __in      PSCSI_REQUEST_BLOCK                 Srb

)
{
    UCHAR function;
    PAHCI_ADAPTER_EXTENSION adapterExtension;

    StorPortDebugPrint(0, "AhciHwStartIo()\n");

    adapterExtension = (PAHCI_ADAPTER_EXTENSION)AdapterExtension;
    function = Srb->Function;

    return TRUE;
}// -- AhciHwStartIo();

/**
 * @name AhciHwResetBus
 * @implemented
 *
 * The HwStorResetBus routine is called by the port driver to clear error conditions.
 *
 * @param adapterExtension
 * @param PathId
 *
 * @return
 * return TRUE if bus was successfully reset
 */
BOOLEAN AhciHwResetBus(
  __in      PVOID                               AdapterExtension,
  __in      ULONG                               PathId

)
{
    PAHCI_ADAPTER_EXTENSION adapterExtension;

    StorPortDebugPrint(0, "AhciHwResetBus()\n");

    adapterExtension = (PAHCI_ADAPTER_EXTENSION)AdapterExtension;

    return TRUE;
}// -- AhciHwResetBus();

/**
 * @name AhciHwFindAdapter
 * @implemented
 *
 * The HwStorFindAdapter routine uses the supplied configuration to determine whether a specific
 * HBA is supported and, if it is, to return configuration information about that adapter.
 *
 *  10.1 Platform Communication
 *  http://www.intel.in/content/dam/www/public/us/en/documents/technical-specifications/serial-ata-ahci-spec-rev1_2.pdf

 * @param DeviceExtension
 * @param HwContext
 * @param BusInformation
 * @param ArgumentString
 * @param ConfigInfo
 * @param Reserved3
 *
 * @return
 *      SP_RETURN_FOUND
 *          Indicates that a supported HBA was found and that the HBA-relevant configuration information was successfully determined and set in the PORT_CONFIGURATION_INFORMATION structure.
 *
 *      SP_RETURN_ERROR
 *          Indicates that an HBA was found but there was an error obtaining the configuration information. If possible, such an error should be logged with StorPortLogError.
 *
 *      SP_RETURN_BAD_CONFIG
 *          Indicates that the supplied configuration information was invalid for the adapter.
 *
 *      SP_RETURN_NOT_FOUND
 *          Indicates that no supported HBA was found for the supplied configuration information.
 *
 * @remarks Called by Storport.
 */
ULONG AhciHwFindAdapter(
  __in      PVOID                               AdapterExtension,
  __in      PVOID                               HwContext,
  __in      PVOID                               BusInformation,
  __in      PVOID                               ArgumentString,
  __inout   PPORT_CONFIGURATION_INFORMATION     ConfigInfo,
  __in      PBOOLEAN                            Reserved3
)
{
    ULONG ghc;
    ULONG index;
    ULONG portCount, portImplemented;
    ULONG pci_cfg_len;
    UCHAR pci_cfg_buf[0x30];
    
    PAHCI_MEMORY_REGISTERS abar;
    PPCI_COMMON_CONFIG pciConfigData;
    PAHCI_ADAPTER_EXTENSION adapterExtension;

    StorPortDebugPrint(0, "AhciHwFindAdapter()\n");

    adapterExtension = (PAHCI_ADAPTER_EXTENSION)AdapterExtension;
    adapterExtension->SlotNumber = ConfigInfo->SlotNumber;
    adapterExtension->SystemIoBusNumber = ConfigInfo->SystemIoBusNumber;
    
    // get PCI configuration header
    pci_cfg_len = StorPortGetBusData(
                        adapterExtension,
                        PCIConfiguration,
                        adapterExtension->SystemIoBusNumber,
                        adapterExtension->SlotNumber,
                        (PVOID)pci_cfg_buf,
                        (ULONG)0x30);

    if (pci_cfg_len != 0x30){
        StorPortDebugPrint(0, "\tpci_cfg_len != 0x30 :: %d", pci_cfg_len);
        return SP_RETURN_ERROR;//Not a valid device at the given bus number
    }

    pciConfigData = (PPCI_COMMON_CONFIG)pci_cfg_buf;
    adapterExtension->VendorID = pciConfigData->VendorID;
    adapterExtension->DeviceID = pciConfigData->DeviceID;
    adapterExtension->RevisionID = pciConfigData->RevisionID;
    // The last PCI base address register (BAR[5], header offset 0x24) points to the AHCI base memory, it’s called ABAR (AHCI Base Memory Register).
    adapterExtension->AhciBaseAddress = pciConfigData->u.type0.BaseAddresses[5] & (0xFFFFFFF0);

    StorPortDebugPrint(0, "\tVendorID:%d  DeviceID:%d  RevisionID:%d\n", adapterExtension->VendorID, adapterExtension->DeviceID, adapterExtension->RevisionID);

    // 2.1.11
    abar = NULL;
    if (ConfigInfo->NumberOfAccessRanges > 0)
    {
        for (index = 0; index < ConfigInfo->NumberOfAccessRanges; index++)
        {
            if ((*(ConfigInfo->AccessRanges))[index].RangeStart.QuadPart == adapterExtension->AhciBaseAddress)
            {
                abar = (PAHCI_MEMORY_REGISTERS)StorPortGetDeviceBase(
                                adapterExtension,
                                ConfigInfo->AdapterInterfaceType,
                                ConfigInfo->SystemIoBusNumber,
                                (*(ConfigInfo->AccessRanges))[index].RangeStart,
                                (*(ConfigInfo->AccessRanges))[index].RangeLength,
                                (BOOLEAN)!(*(ConfigInfo->AccessRanges))[index].RangeInMemory);
                break;
            }
        }
    }

    if (abar == NULL){
        StorPortDebugPrint(0, "\tabar == NULL\n");
        return SP_RETURN_ERROR; // corrupted information supplied
    }

    adapterExtension->ABAR_Address = abar;
    adapterExtension->CAP = StorPortReadRegisterUlong(adapterExtension, &abar->CAP);
    adapterExtension->CAP2 = StorPortReadRegisterUlong(adapterExtension, &abar->CAP2);
    adapterExtension->Version = StorPortReadRegisterUlong(adapterExtension, &abar->VS);

    // 10.1.2
    // 1. Indicate that system software is AHCI aware by setting GHC.AE to ‘1’.
    // 3.1.2 -- AE bit is read-write only if CAP.SAM is '0'
    ghc = StorPortReadRegisterUlong(adapterExtension, &abar->GHC);
    // AE := Highest Significant bit of GHC
    if ((ghc & (0x1<<31)) == 1)//Hmm, controller was already in power state
    {
        // reset controller to have it in know state
        StorPortDebugPrint(0, "\tAE Already set, Reset()\n");
        if (!AhciAdapterReset(adapterExtension)){
            StorPortDebugPrint(0, "\tReset Failed!\n");
            return SP_RETURN_ERROR;// reset failed
        }
    }

    ghc = 0x1<<31;// only AE=1
    StorPortWriteRegisterUlong(adapterExtension, &abar->GHC, ghc);

    adapterExtension->IS = abar->IS;
    adapterExtension->PortImplemented = StorPortReadRegisterUlong(adapterExtension, &abar->PI);

    if (adapterExtension->PortImplemented == 0){
        StorPortDebugPrint(0, "\tadapterExtension->PortImplemented == 0\n");
        return SP_RETURN_ERROR;
    }

    ConfigInfo->MaximumTransferLength = 128 * 1024;//128 KB
    ConfigInfo->NumberOfPhysicalBreaks = 0x21;
    ConfigInfo->MaximumNumberOfTargets = 1;
    ConfigInfo->MaximumNumberOfLogicalUnits = 1;
    ConfigInfo->ResetTargetSupported = TRUE;
    ConfigInfo->NumberOfBuses = 32;
    ConfigInfo->SynchronizationModel = StorSynchronizeFullDuplex;
    ConfigInfo->ScatterGather = TRUE;

    // Turn IE -- Interrupt Enabled
    ghc |= 0x2;
    StorPortWriteRegisterUlong(adapterExtension, &abar->GHC, ghc);
    
    // allocate necessary resource for each port
    if (!AhciAllocateResourceForAdapter(adapterExtension, ConfigInfo)){
        StorPortDebugPrint(0, "\tAhciAllocateResourceForAdapter() == FALSE\n");
        return SP_RETURN_ERROR;
    }

    for (index = 0; index < 32; index++)
    {
        if ((adapterExtension->PortImplemented & (1<<index)) != 0)
            AhciPortInitialize(&adapterExtension->PortExtension[index]);
    }

    return SP_RETURN_FOUND;
}// -- AhciHwFindAdapter();

/**
 * @name DriverEntry
 * @implemented
 *
 * Initial Entrypoint for storahci miniport driver
 *
 * @param DriverObject
 * @param RegistryPath
 *
 * @return
 * NT_STATUS in case of driver loaded successfully.
 */
ULONG DriverEntry(
  __in      PVOID                               DriverObject,
  __in      PVOID                               RegistryPath
)
{
    HW_INITIALIZATION_DATA hwInitializationData;
    ULONG i, status;

    StorPortDebugPrint(0, "Storahci Loaded 10023\n");

    // initialize the hardware data structure
    AhciZeroMemory((PCHAR)&hwInitializationData, sizeof(HW_INITIALIZATION_DATA));

    // set size of hardware initialization structure
    hwInitializationData.HwInitializationDataSize = sizeof(HW_INITIALIZATION_DATA);

    // identity required miniport entry point routines
    hwInitializationData.HwStartIo = AhciHwStartIo;
    hwInitializationData.HwResetBus = AhciHwResetBus;
    hwInitializationData.HwInterrupt = AhciHwInterrupt;
    hwInitializationData.HwInitialize = AhciHwInitialize;
    hwInitializationData.HwFindAdapter = AhciHwFindAdapter;

    // adapter specific information
    hwInitializationData.NeedPhysicalAddresses = TRUE;
    hwInitializationData.TaggedQueuing = TRUE;
    hwInitializationData.AutoRequestSense = TRUE;
    hwInitializationData.MultipleRequestPerLu = TRUE;

    hwInitializationData.NumberOfAccessRanges = 6;
    hwInitializationData.AdapterInterfaceType = PCIBus;
    hwInitializationData.MapBuffers = STOR_MAP_NON_READ_WRITE_BUFFERS;

    // set required extension sizes
    hwInitializationData.SrbExtensionSize = sizeof(AHCI_SRB_EXTENSION);
    hwInitializationData.DeviceExtensionSize = sizeof(AHCI_ADAPTER_EXTENSION);
    
    // register our hw init data
    status = StorPortInitialize(
                    DriverObject,
                    RegistryPath,
                    &hwInitializationData,
                    NULL);
    StorPortDebugPrint(0, "\tstatus:%x\n", status);
    return status;
}// -- DriverEntry();

/**
 * @name AhciAdapterReset
 * @implemented
 *
 * 10.4.3 HBA Reset
 * If the HBA becomes unusable for multiple ports, and a software reset or port reset does not correct the
 * problem, software may reset the entire HBA by setting GHC.HR to ‘1’. When software sets the GHC.HR
 * bit to ‘1’, the HBA shall perform an internal reset action. The bit shall be cleared to ‘0’ by the HBA when
 * the reset is complete. A software write of ‘0’ to GHC.HR shall have no effect. To perform the HBA reset,
 * software sets GHC.HR to ‘1’ and may poll until this bit is read to be ‘0’, at which point software knows that
 * the HBA reset has completed.
 * If the HBA has not cleared GHC.HR to ‘0’ within 1 second of software setting GHC.HR to ‘1’, the HBA is in
 * a hung or locked state. 
 *
 * @param adapterExtension
 *
 * @return
 * TRUE in case AHCI Controller RESTARTED successfully. i.e GHC.HR == 0
 */
BOOLEAN AhciAdapterReset(
  __in      PAHCI_ADAPTER_EXTENSION             adapterExtension
)
{
    ULONG ghc, ticks;
    PAHCI_MEMORY_REGISTERS abar = NULL;

    StorPortDebugPrint(0, "AhciAdapterReset()\n");

    abar = adapterExtension->ABAR_Address;
    if (abar == NULL) // basic sanity
        return FALSE;

    // HR -- Very first bit (lowest significant)
    ghc = 1;
    StorPortWriteRegisterUlong(adapterExtension, &abar->GHC, ghc);

    for (ticks = 0; (ticks < 50) &&
                    (StorPortReadRegisterUlong(adapterExtension, &abar->GHC) == 1);
                    StorPortStallExecution(20000), ticks++);

    if (ticks == 50) { //1 second
        StorPortDebugPrint(0, "\tDevice Timeout\n");
        return FALSE;
    }

    return TRUE;
}// -- AhciAdapterReset();

/**
 * @name AhciZeroMemory
 * @implemented
 *
 * Clear buffer by filling zeros
 *
 * @param buffer
 */
VOID AhciZeroMemory(
  __in      PCHAR                               buffer,
  __in      ULONG                               bufferSize
)
{
    ULONG i;
    for (i = 0; i < bufferSize; i++)
        buffer[i] = 0;
}// -- AhciZeroMemory();

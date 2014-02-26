/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            bios.c
 * PURPOSE:         VDM BIOS Support Library
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "callback.h"

#include "bios.h"

#include "bop.h"
#include "rom.h"

/* PRIVATE VARIABLES **********************************************************/

static BOOLEAN Bios32Loaded = FALSE;

static CALLBACK16 __BiosContext;

/* BOP Identifiers */
#define BOP_BIOSINIT    0x00    // Windows NTVDM (SoftPC) BIOS calls BOP 0x00
                                // to let the virtual machine initialize itself
                                // the IVT and its hardware.

/* PRIVATE FUNCTIONS **********************************************************/

static VOID WINAPI BiosInitBop(LPWORD Stack)
{
    /* Load the second part of the Windows NTVDM BIOS image */
    LPCWSTR BiosFileName = L"bios1.rom";
    PVOID   BiosLocation = (PVOID)TO_LINEAR(BIOS_SEGMENT, 0x0000);
    DWORD   BiosSize = 0;
    BOOLEAN Success;

    DPRINT1("You are loading Windows NTVDM BIOS!");

    /* Initialize a private callback context */
    InitializeContext(&__BiosContext, BIOS_SEGMENT, 0x0000);

    Success = LoadRom(BiosFileName, BiosLocation, &BiosSize);
    DPRINT1("BIOS loading %s ; GetLastError() = %u\n", Success ? "succeeded" : "failed", GetLastError());

    if (Success == FALSE) return;

    // DisplayMessage(L"First bytes at 0x%p: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n"
                       // L"3 last bytes at 0x%p: 0x%02x 0x%02x 0x%02x",
        // BiosLocation,
        // *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 0),
        // *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 1),
        // *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 2),
        // *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 3),
        // *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 4),
        // *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 5),
        // *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 6),
        // *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 7),
        // *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 8),
        // *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 9),

         // (PVOID)((ULONG_PTR)BiosLocation + BiosSize - 2),
        // *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + BiosSize - 2),
        // *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + BiosSize - 1),
        // *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + BiosSize - 0));

    /* Initialize IVT and hardware */

    /* Load VGA BIOS */
    // Success = LoadRom(L"v7vga.rom", (PVOID)0xC0000, &BiosSize);
    // DPRINT1("VGA BIOS loading %s ; GetLastError() = %u\n", Success ? "succeeded" : "failed", GetLastError());

    ///////////// MUST BE DONE AFTER IVT INITIALIZATION !! /////////////////////

    /* Load some ROMs */
    Success = LoadRom(L"boot.bin", (PVOID)0xE0000, &BiosSize);
    DPRINT1("Test ROM loading %s ; GetLastError() = %u\n", Success ? "succeeded" : "failed", GetLastError());

    SearchAndInitRoms(&__BiosContext);
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN BiosInitialize(IN LPCWSTR BiosFileName,
                       IN HANDLE  ConsoleInput,
                       IN HANDLE  ConsoleOutput)
{
    /* Register the BIOS support BOPs */
    RegisterBop(BOP_BIOSINIT, BiosInitBop);

    if (BiosFileName)
    {
        PVOID   BiosLocation = NULL;
        DWORD   BiosSize = 0;
        BOOLEAN Success = LoadBios(BiosFileName, &BiosLocation, &BiosSize);

        DPRINT1("BIOS loading %s ; GetLastError() = %u\n", Success ? "succeeded" : "failed", GetLastError());

        if (Success == FALSE) return FALSE;

        DisplayMessage(L"First bytes at 0x%p: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n"
                       L"3 last bytes at 0x%p: 0x%02x 0x%02x 0x%02x",
            BiosLocation,
            *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 0),
            *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 1),
            *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 2),
            *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 3),
            *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 4),
            *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 5),
            *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 6),
            *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 7),
            *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 8),
            *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + 9),

             (PVOID)((ULONG_PTR)BiosLocation + BiosSize - 2),
            *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + BiosSize - 2),
            *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + BiosSize - 1),
            *(PCHAR)((ULONG_PTR)REAL_TO_PHYS(BiosLocation) + BiosSize - 0));

        DisplayMessage(L"POST at 0x%p: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x",
                       TO_LINEAR(getCS(), getIP()),
                       *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(getCS(), getIP()) + 0),
                       *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(getCS(), getIP()) + 1),
                       *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(getCS(), getIP()) + 2),
                       *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(getCS(), getIP()) + 3),
                       *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(getCS(), getIP()) + 4));

        /* Boot it up */

        /*
         * The CPU is already in reset-mode so that
         * CS:IP points to F000:FFF0 as required.
         */
        DisplayMessage(L"CS=0x%p ; IP=0x%p", getCS(), getIP());
        // setCS(0xF000);
        // setIP(0xFFF0);

        return TRUE;
    }
    else
    {
        Bios32Loaded = Bios32Initialize(ConsoleInput, ConsoleOutput);
        return Bios32Loaded;
    }
}

VOID BiosCleanup(VOID)
{
    if (Bios32Loaded) Bios32Cleanup();
}

/* EOF */

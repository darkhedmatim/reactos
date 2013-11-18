/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            emulator.h
 * PURPOSE:         Minimal x86 machine emulator for the VDM (header file)
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _EMULATOR_H_
#define _EMULATOR_H_

/* INCLUDES *******************************************************************/

#include "ntvdm.h"
#include <fast486.h>

/* DEFINES ********************************************************************/

/* FLAGS */
#define EMULATOR_FLAG_CF (1 << 0)
#define EMULATOR_FLAG_PF (1 << 2)
#define EMULATOR_FLAG_AF (1 << 4)
#define EMULATOR_FLAG_ZF (1 << 6)
#define EMULATOR_FLAG_SF (1 << 7)
#define EMULATOR_FLAG_TF (1 << 8)
#define EMULATOR_FLAG_IF (1 << 9)
#define EMULATOR_FLAG_DF (1 << 10)
#define EMULATOR_FLAG_OF (1 << 11)
#define EMULATOR_FLAG_NT (1 << 14)
#define EMULATOR_FLAG_RF (1 << 16)
#define EMULATOR_FLAG_VM (1 << 17)
#define EMULATOR_FLAG_AC (1 << 18)
#define EMULATOR_FLAG_VIF (1 << 19)
#define EMULATOR_FLAG_VIP (1 << 20)
#define EMULATOR_FLAG_ID (1 << 21)

#define STACK_COUNTER   0
#define STACK_INT_NUM   1
#define STACK_IP        2
#define STACK_CS        3
#define STACK_FLAGS     4

enum
{
    EMULATOR_EXCEPTION_DIVISION_BY_ZERO,
    EMULATOR_EXCEPTION_DEBUG,
    EMULATOR_EXCEPTION_NMI,
    EMULATOR_EXCEPTION_BREAKPOINT,
    EMULATOR_EXCEPTION_OVERFLOW,
    EMULATOR_EXCEPTION_BOUND,
    EMULATOR_EXCEPTION_INVALID_OPCODE,
    EMULATOR_EXCEPTION_NO_FPU,
    EMULATOR_EXCEPTION_DOUBLE_FAULT,
    EMULATOR_EXCEPTION_FPU_SEGMENT,
    EMULATOR_EXCEPTION_INVALID_TSS,
    EMULATOR_EXCEPTION_NO_SEGMENT,
    EMULATOR_EXCEPTION_STACK_SEGMENT,
    EMULATOR_EXCEPTION_GPF,
    EMULATOR_EXCEPTION_PAGE_FAULT
};

extern FAST486_STATE EmulatorContext;

/* FUNCTIONS ******************************************************************/

VOID WINAPI EmulatorReadMemory
(
    PFAST486_STATE State,
    ULONG Address,
    PVOID Buffer,
    ULONG Size
);

VOID WINAPI EmulatorWriteMemory
(
    PFAST486_STATE State,
    ULONG Address,
    PVOID Buffer,
    ULONG Size
);

VOID WINAPI EmulatorReadIo
(
    PFAST486_STATE State,
    ULONG Port,
    PVOID Buffer,
    ULONG DataCount,
    UCHAR DataSize
);

VOID WINAPI EmulatorWriteIo
(
    PFAST486_STATE State,
    ULONG Port,
    PVOID Buffer,
    ULONG DataCount,
    UCHAR DataSize
);

UCHAR WINAPI EmulatorIntAcknowledge
(
    PFAST486_STATE State
);

BOOLEAN EmulatorInitialize(VOID);
VOID EmulatorExecute(WORD Segment, WORD Offset);
VOID EmulatorInterrupt(BYTE Number);
VOID EmulatorInterruptSignal(VOID);
VOID EmulatorStep(VOID);
VOID EmulatorCleanup(VOID);
VOID EmulatorSetA20(BOOLEAN Enabled);

#endif // _EMULATOR_H_

/* EOF */

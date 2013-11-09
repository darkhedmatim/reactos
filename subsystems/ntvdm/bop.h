/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            bop.h
 * PURPOSE:         BIOS Operation Handlers
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef _BOP_H_
#define _BOP_H_

/* DEFINES ********************************************************************/

/* BOP Identifiers */
#define EMULATOR_BOP            0xC4C4

#define EMULATOR_CTRL_BOP       0xFF    // Control BOP Handler
         #define CTRL_BOP_DEFLT 0x00    // Default Control BOP Function
         #define CTRL_BOP_INT32 0xFF    // 32-bit Interrupt dispatcher

#define EMULATOR_MAX_BOP_NUM    0xFF + 1

/* 32-bit Interrupt Identifiers */
#define EMULATOR_MAX_INT_NUM    0xFF + 1

/* FUNCTIONS ******************************************************************/

typedef VOID (WINAPI *EMULATOR_BOP_PROC)(LPWORD Stack);
typedef VOID (WINAPI *EMULATOR_INT32_PROC)(LPWORD Stack);

VOID WINAPI ControlBop(LPWORD Stack);

VOID WINAPI RegisterInt32(BYTE IntNumber, EMULATOR_INT32_PROC IntHandler);
VOID WINAPI EmulatorBiosOperation(PFAST486_STATE State, UCHAR BopCode);

#endif // _BOP_H_

/* EOF */

/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         386/486 CPU Emulation Library
 * FILE:            opcodes.h
 * PURPOSE:         Opcode handlers. (header file)
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _OPCODES_H_
#define _OPCODES_H_

/* DEFINES ********************************************************************/

#ifndef FASTCALL
#define FASTCALL __fastcall
#endif

#define SOFT386_NUM_OPCODE_HANDLERS 256

typedef BOOLEAN (FASTCALL *SOFT386_OPCODE_HANDLER_PROC)(PSOFT386_STATE, UCHAR);

extern
SOFT386_OPCODE_HANDLER_PROC
Soft386OpcodeHandlers[SOFT386_NUM_OPCODE_HANDLERS];

BOOLEAN
FASTCALL
Soft386OpcodePrefix
(
    PSOFT386_STATE State,
    UCHAR Opcode
);

BOOLEAN
FASTCALL
Soft386OpcodeIncrement
(
    PSOFT386_STATE State,
    UCHAR Opcode
);

BOOLEAN
FASTCALL
Soft386OpcodeDecrement
(
    PSOFT386_STATE State,
    UCHAR Opcode
);

BOOLEAN
FASTCALL
Soft386OpcodePushReg
(
    PSOFT386_STATE State,
    UCHAR Opcode
);

BOOLEAN
FASTCALL
Soft386OpcodePopReg
(
    PSOFT386_STATE State,
    UCHAR Opcode
);

BOOLEAN
FASTCALL
Soft386OpcodeNop
(
    PSOFT386_STATE State,
    UCHAR Opcode
);

BOOLEAN
FASTCALL
Soft386OpcodeExchangeEax
(
    PSOFT386_STATE State,
    UCHAR Opcode
);

BOOLEAN
FASTCALL
Soft386OpcodeShortConditionalJmp
(
    PSOFT386_STATE State,
    UCHAR Opcode
);

BOOLEAN
FASTCALL
Soft386OpcodeClearCarry
(
    PSOFT386_STATE State,
    UCHAR Opcode
);

BOOLEAN
FASTCALL
Soft386OpcodeSetCarry
(
    PSOFT386_STATE State,
    UCHAR Opcode
);

BOOLEAN
FASTCALL
Soft386OpcodeComplCarry
(
    PSOFT386_STATE State,
    UCHAR Opcode
);

BOOLEAN
FASTCALL
Soft386OpcodeClearInt
(
    PSOFT386_STATE State,
    UCHAR Opcode
);

BOOLEAN
FASTCALL
Soft386OpcodeSetInt
(
    PSOFT386_STATE State,
    UCHAR Opcode
);

BOOLEAN
FASTCALL
Soft386OpcodeClearDir
(
    PSOFT386_STATE State,
    UCHAR Opcode
);

BOOLEAN
FASTCALL
Soft386OpcodeSetDir
(
    PSOFT386_STATE State,
    UCHAR Opcode
);

BOOLEAN
FASTCALL
Soft386OpcodeHalt
(
    PSOFT386_STATE State,
    UCHAR Opcode
);

BOOLEAN
FASTCALL
Soft386OpcodeInByte
(
    PSOFT386_STATE State,
    UCHAR Opcode
);

BOOLEAN
FASTCALL
Soft386OpcodeIn
(
    PSOFT386_STATE State,
    UCHAR Opcode
);

BOOLEAN
FASTCALL
Soft386OpcodeOutByte
(
    PSOFT386_STATE State,
    UCHAR Opcode
);

BOOLEAN
FASTCALL
Soft386OpcodeOut
(
    PSOFT386_STATE State,
    UCHAR Opcode
);

BOOLEAN
FASTCALL
Soft386OpcodeShortJump
(
    PSOFT386_STATE State,
    UCHAR Opcode
);

#endif // _OPCODES_H_

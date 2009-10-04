/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
#ifndef __NTOSKRNL_INCLUDE_INTERNAL_POWERPC_KE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_POWERPC_KE_H

#include <ndk/powerpc/ketypes.h>

/* Possible values for KTHREAD's NpxState */
#define KPCR_BASE 0xff000000
#define NPX_STATE_INVALID   0x01
#define NPX_STATE_VALID     0x02
#define NPX_STATE_DIRTY     0x04

#ifndef __ASM__

typedef struct _KIRQ_TRAPFRAME
{
} KIRQ_TRAPFRAME, *PKIRQ_TRAPFRAME;

extern ULONG KePPCCacheAlignment;

#define IMAGE_FILE_MACHINE_ARCHITECTURE IMAGE_FILE_MACHINE_POWERPC

//
// Macros for getting and setting special purpose registers in portable code
//
#define KeGetContextPc(Context) \
    ((Context)->Dr0)

#define KeSetContextPc(Context, ProgramCounter) \
    ((Context)->Dr0 = (ProgramCounter))

#define KeGetTrapFramePc(TrapFrame) \
    ((TrapFrame)->Dr0)

#define KeGetContextReturnRegister(Context) \
    ((Context)->Gpr3)

#define KeSetContextReturnRegister(Context, ReturnValue) \
    ((Context)->Gpr3 = (ReturnValue))

#define KePPCRdmsr(msr,val1,val2) __asm__ __volatile__("mfmsr 3")

#define KePPCWrmsr(msr,val1,val2) __asm__ __volatile__("mtmsr 3")

#define PPC_MIN_CACHE_LINE_SIZE 32

FORCEINLINE struct _KPCR * NTHALAPI KeGetCurrentKPCR(
    VOID)
{
    return (struct _KPCR *)__readfsdword(0x1c);
}

#ifdef _NTOSKRNL_ /* FIXME: Move flags above to NDK instead of here */
VOID
NTAPI
KiThreadStartup(PKSYSTEM_ROUTINE SystemRoutine,
                PKSTART_ROUTINE StartRoutine,
                PVOID StartContext,
                BOOLEAN UserThread,
                KTRAP_FRAME TrapFrame);
#endif

#endif /* __ASM__ */

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_POWERPC_KE_H */

/* EOF */

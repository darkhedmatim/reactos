/* $Id: kd.h,v 1.5 2002/02/02 20:12:45 ekohl Exp $
 *
 * kernel debugger prototypes
 */

#ifndef __INCLUDE_INTERNAL_KERNEL_DEBUGGER_H
#define __INCLUDE_INTERNAL_KERNEL_DEBUGGER_H

#include <internal/ke.h>

typedef enum _KD_CONTINUE_TYPE
{
  kdContinue = 0,
  kdDoNotHandleException,
  kdHandleException
} KD_CONTINUE_TYPE;

ULONG 
KdpPrintString (PANSI_STRING String);

VOID 
DebugLogWrite(PCH String);
VOID
DebugLogInit(VOID);
VOID
DebugLogInit2(VOID);

VOID
KdInit1(VOID);

VOID
KdInit2(VOID);

VOID
KdPutChar(UCHAR Value);

UCHAR
KdGetChar(VOID);

VOID
KdGdbStubInit();

VOID
KdGdbDebugPrint (LPSTR Message);

VOID
KdDebugPrint (LPSTR Message);

KD_CONTINUE_TYPE
KdEnterDebuggerException(PEXCEPTION_RECORD ExceptionRecord,
  PCONTEXT Context,
	PKTRAP_FRAME TrapFrame);

#endif /* __INCLUDE_INTERNAL_KERNEL_DEBUGGER_H */

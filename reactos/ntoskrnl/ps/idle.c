/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/idle.c
 * PURPOSE:         Using idle time
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, PsInitIdleThread)
#endif

/* FUNCTIONS *****************************************************************/

/** System idle thread procedure
 *
 */
VOID STDCALL
PsIdleThreadMain(PVOID Context)
{
   KIRQL oldlvl;

   PKPRCB Prcb = KeGetCurrentPrcb();

   for(;;)
     {
       if (Prcb->DpcData[0].DpcQueueDepth > 0)
	 {
	   KeRaiseIrql(DISPATCH_LEVEL,&oldlvl);
	   KiDispatchInterrupt();
	   KeLowerIrql(oldlvl);
	 }

       NtYieldExecution();

       Ke386HaltProcessor();
     }
}

/*
 * HACK-O-RAMA
 * Antique vestigial code left alive for the sole purpose of First/Idle Thread
 * creation until I can merge my fix for properly creating them.
 */
NTSTATUS
NTAPI
PsInitializeIdleOrFirstThread(PEPROCESS Process,
                              PETHREAD* ThreadPtr,
                              PKSTART_ROUTINE StartRoutine,
                              KPROCESSOR_MODE AccessMode,
                              BOOLEAN First)
{
    PETHREAD Thread;
    PVOID KernelStack;

    Thread = ExAllocatePool(NonPagedPool, sizeof(ETHREAD));
    RtlZeroMemory(Thread, sizeof(ETHREAD));
    Thread->ThreadsProcess = Process;
    if (First)
    {
        KernelStack = P0BootStack;
    }
    else
    {
        KernelStack = (PVOID)((ULONG_PTR)MmCreateKernelStack(FALSE) +
                              KERNEL_STACK_SIZE);
    }
    KeInitializeThread(&Process->Pcb,
                       &Thread->Tcb,
                       PspSystemThreadStartup,
                       StartRoutine,
                       NULL,
                       NULL,
                       NULL,
                       KernelStack);
    InitializeListHead(&Thread->IrpList);
    *ThreadPtr = Thread;
    return STATUS_SUCCESS;
}

/*
 * HACK-O-RAMA
 * Antique vestigial code left alive for the sole purpose of First/Idle Thread
 * creation until I can merge my fix for properly creating them.
 */
VOID
INIT_FUNCTION
NTAPI
PsInitIdleThread(VOID)
{
    PETHREAD IdleThread;
    KIRQL oldIrql;

    PsInitializeIdleOrFirstThread(PsIdleProcess,
                                  &IdleThread,
                                  PsIdleThreadMain,
                                  KernelMode,
                                  FALSE);

    oldIrql = KiAcquireDispatcherLock ();
    KiReadyThread(&IdleThread->Tcb);
    KiReleaseDispatcherLock(oldIrql);

    KeGetCurrentPrcb()->IdleThread = &IdleThread->Tcb;
    KeSetPriorityThread(&IdleThread->Tcb, LOW_PRIORITY);
    KeSetAffinityThread(&IdleThread->Tcb, 1 << 0);
}

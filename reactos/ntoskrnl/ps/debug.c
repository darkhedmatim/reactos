/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/debug.c
 * PURPOSE:         Thread managment
 * 
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 *                  Phillip Susi
 */


/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

/* Thread "Set/Get Context" Context Structure */
typedef struct _GET_SET_CTX_CONTEXT {
    KAPC Apc;
    KEVENT Event;
    CONTEXT Context;
} GET_SET_CTX_CONTEXT, *PGET_SET_CTX_CONTEXT;


/* FUNCTIONS ***************************************************************/

/*
 * FUNCTION: This routine is called by an APC sent by NtGetContextThread to
 * copy the context of a thread into a buffer.
 */
VOID 
STDCALL
PspGetOrSetContextKernelRoutine(PKAPC Apc,
                                PKNORMAL_ROUTINE* NormalRoutine,
                                PVOID* NormalContext,
                                PVOID* SystemArgument1,
                                PVOID* SystemArgument2)
{
    PGET_SET_CTX_CONTEXT GetSetContext;
    PKEVENT Event;
    PCONTEXT Context;
   
    /* Get the Context Structure */
    GetSetContext = CONTAINING_RECORD(Apc, GET_SET_CTX_CONTEXT, Apc);
    Context = &GetSetContext->Context;
    Event = &GetSetContext->Event;

    /* Check if it's a set or get */
    if (SystemArgument1) {
        
        /* Get the Context */ 
        KeTrapFrameToContext(KeGetCurrentThread()->TrapFrame, Context);
        
    } else {
        
        /* Set the Context */
        KeContextToTrapFrame(Context, KeGetCurrentThread()->TrapFrame);
    }
   
    /* Notify the Native API that we are done */
    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
}

NTSTATUS 
STDCALL
NtGetContextThread(IN HANDLE ThreadHandle,
                   OUT PCONTEXT ThreadContext)
{
    PETHREAD Thread;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    GET_SET_CTX_CONTEXT GetSetContext;
    NTSTATUS Status = STATUS_SUCCESS;
  
    PAGED_CODE();

    /* Check the buffer to be OK */
    if(PreviousMode != KernelMode) {
    
        _SEH_TRY {
            
            ProbeForWrite(ThreadContext,
                          sizeof(CONTEXT),
                          sizeof(ULONG));
        } _SEH_HANDLE {
            
            Status = _SEH_GetExceptionCode();
            
        } _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }
  
    /* Get the Thread Object */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_GET_CONTEXT,
                                       PsThreadType,
                                       PreviousMode,
                                       (PVOID*)&Thread,
                                       NULL);
    
    /* Check success */
    if(NT_SUCCESS(Status)) {
        
        /* Check if we're running in the same thread */
        if(Thread == PsGetCurrentThread()) {
      
            /*
             * I don't know if trying to get your own context makes much
             * sense but we can handle it more efficently.
             */
            KeTrapFrameToContext(Thread->Tcb.TrapFrame, &GetSetContext.Context);
        
        } else {
            
            /* Use an APC... Initialize the Event */
            KeInitializeEvent(&GetSetContext.Event,
                              NotificationEvent,
                              FALSE);
            
            /* Initialize the APC */
            KeInitializeApc(&GetSetContext.Apc,
                            &Thread->Tcb,
                            OriginalApcEnvironment,
                            PspGetOrSetContextKernelRoutine,
                            NULL,
                            NULL,
                            KernelMode,
                            NULL);
            
            /* Queue it as a Get APC */
            if (!KeInsertQueueApc(&GetSetContext.Apc,
                                  (PVOID)1,
                                  NULL,
                                  IO_NO_INCREMENT)) {
                
                Status = STATUS_THREAD_IS_TERMINATING;
            
            } else {
            
                /* Wait for the APC to complete */
                Status = KeWaitForSingleObject(&GetSetContext.Event,
                                               0,
                                               KernelMode,
                                               FALSE,
                                               NULL);
            }
        }
        
        /* Dereference the thread */
        ObDereferenceObject(Thread);
    
        /* Check for success and return the Context */
        if(NT_SUCCESS(Status)) {
      
            _SEH_TRY {
                
                *ThreadContext = GetSetContext.Context;
          
            } _SEH_HANDLE {
                
                Status = _SEH_GetExceptionCode();
            
            } _SEH_END;          
        }
    }
  
    /* Return status */        
    return Status;
}

NTSTATUS 
STDCALL
NtSetContextThread(IN HANDLE ThreadHandle,
                   IN PCONTEXT ThreadContext)
{
    PETHREAD Thread;
    GET_SET_CTX_CONTEXT GetSetContext;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
  
    PAGED_CODE();
  
    /* Check the buffer to be OK */
    if(PreviousMode != KernelMode) {
    
        _SEH_TRY {
            
            ProbeForRead(ThreadContext,
                         sizeof(CONTEXT),
                         sizeof(ULONG));
            
            GetSetContext.Context = *ThreadContext;
            ThreadContext = &GetSetContext.Context;
            
        } _SEH_HANDLE {
            
            Status = _SEH_GetExceptionCode();
        } _SEH_END;
    
        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Get the Thread Object */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_SET_CONTEXT,
                                       PsThreadType,
                                       PreviousMode,
                                       (PVOID*)&Thread,
                                       NULL);
    
    /* Check success */
    if(NT_SUCCESS(Status)) {
        
        /* Check if we're running in the same thread */
        if(Thread == PsGetCurrentThread()) {
      
            /*
             * I don't know if trying to get your own context makes much
             * sense but we can handle it more efficently.
             */
            KeContextToTrapFrame(&GetSetContext.Context, Thread->Tcb.TrapFrame);
        
        } else {
            
            /* Use an APC... Initialize the Event */
            KeInitializeEvent(&GetSetContext.Event,
                              NotificationEvent,
                              FALSE);
            
            /* Initialize the APC */
            KeInitializeApc(&GetSetContext.Apc,
                            &Thread->Tcb,
                            OriginalApcEnvironment,
                            PspGetOrSetContextKernelRoutine,
                            NULL,
                            NULL,
                            KernelMode,
                            NULL);
            
            /* Queue it as a Get APC */
            if (!KeInsertQueueApc(&GetSetContext.Apc,
                                  NULL,
                                  NULL,
                                  IO_NO_INCREMENT)) {
                
                Status = STATUS_THREAD_IS_TERMINATING;
            
            } else {
            
                /* Wait for the APC to complete */
                Status = KeWaitForSingleObject(&GetSetContext.Event,
                                               0,
                                               KernelMode,
                                               FALSE,
                                               NULL);
            }
        }
        
        /* Dereference the thread */
        ObDereferenceObject(Thread);
    }
  
    /* Return status */        
    return Status;
}

/* EOF */

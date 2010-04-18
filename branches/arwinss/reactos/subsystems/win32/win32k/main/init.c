/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/gre/init.c
 * PURPOSE:         Driver Initialization
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

/* System service call table */
#include <include/napi.h>

#include "object.h"
#include <handle.h>
#include <user.h>

#define NDEBUG
#include <debug.h>

void init_directories(void);
NTSTATUS FASTCALL InitDcImpl(VOID);

/* GLOBALS *******************************************************************/

PGDI_HANDLE_TABLE GdiHandleTable = NULL;
PSECTION_OBJECT GdiTableSection = NULL;
LIST_ENTRY GlobalDriverListHead;

/* PRIVATE FUNCTIONS *********************************************************/

NTSTATUS
APIENTRY
Win32kProcessCallout(PEPROCESS Process,
                     BOOLEAN Create)
{
    PPROCESSINFO Win32Process;
    NTSTATUS Status;
    struct handle_table *handles;

    DPRINT("Enter Win32kProcessCallback\n");

    /* Get the Win32 Process */
    Win32Process = PsGetProcessWin32Process(Process);
    DPRINT("Win32Process %p, Create %d\n", Win32Process, Create);
    if (Create && !Win32Process)
    {
        DPRINT("Creating W32 process PID:%d at IRQ level: %lu\n", Process->UniqueProcessId, KeGetCurrentIrql());

        /* Allocate one if needed */
        /* FIXME - lock the process */
        Win32Process = ExAllocatePoolWithTag(NonPagedPool,
            sizeof(PROCESSINFO), 'p23W');

        if (!Win32Process) return STATUS_NO_MEMORY;

        RtlZeroMemory(Win32Process, sizeof(PROCESSINFO));

        PsSetProcessWin32Process(Process, Win32Process);
        Win32Process->peProcess = Process;
        /* FIXME - unlock the process */

        /* Create an idle event */
        Status = ZwCreateEvent(&Win32Process->idle_event_handle, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, TRUE);
        if (!NT_SUCCESS(Status)) DPRINT1("Creating idle event failed with status 0x%08X\n", Status);

        /* Get a pointer to the object itself */
        Status = ObReferenceObjectByHandle(Win32Process->idle_event_handle,
                                           EVENT_ALL_ACCESS,
                                           NULL,
                                           KernelMode,
                                           (PVOID*)&Win32Process->idle_event,
                                           NULL);
        if (!NT_SUCCESS(Status)) ZwClose(Win32Process->idle_event_handle);

        list_init(&Win32Process->Classes);
        Win32Process->handles = alloc_handle_table(Win32Process, 0);
        connect_process_winstation(Win32Process);
    }
    else
    {
        DPRINT("Destroying W32 process PID:%d at IRQ level: %lu\n", Process->UniqueProcessId, KeGetCurrentIrql());

        UserEnterExclusive();

        /* Delete its handles table */
        handles = Win32Process->handles;
        Win32Process->handles = NULL;
        if (handles) release_object(handles);

        /* Destroy its classes */
        destroy_process_classes(Win32Process);

        if (Win32Process->idle_event)
        {
            ObDereferenceObject(Win32Process->idle_event);
            ZwClose(Win32Process->idle_event_handle);
        }

        UserLeave();

        /* Destroy all user->kernel handle mapping */
        GDI_CleanupHandleMapping();
    }

    DPRINT("Leave Win32kProcessCallback\n");
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kThreadCallout(PETHREAD Thread,
                    PSW32THREADCALLOUTTYPE Type)
{
    struct _EPROCESS *Process;
    PTHREADINFO Win32Thread;
    PPROCESSINFO Win32Process;

    DPRINT("Enter Win32kThreadCallback, current thread id %d, process id %d\n", PsGetCurrentThread()->Tcb.Teb->ClientId.UniqueThread, PsGetCurrentThread()->Tcb.Teb->ClientId.UniqueProcess);

    Process = Thread->ThreadsProcess;

    /* Get the Win32 Thread and Process */
    Win32Thread = PsGetThreadWin32Thread(Thread);
    Win32Process = PsGetProcessWin32Process(Process);
    DPRINT("Win32 thread %p, process %p\n", Win32Thread, Win32Process);
    /* Allocate one if needed */
    if (!Win32Thread)
    {
        /* FIXME - lock the process */
        Win32Thread = ExAllocatePoolWithTag(NonPagedPool,
                                            sizeof(THREADINFO),
                                            't23W');

        if (!Win32Thread)
            return STATUS_NO_MEMORY;

        RtlZeroMemory(Win32Thread, sizeof(THREADINFO));

        PsSetThreadWin32Thread(Thread, Win32Thread);
        /* FIXME - unlock the process */
    }
    if (Type == PsW32ThreadCalloutInitialize)
    {
        DPRINT("Creating W32 thread TID:%d PID:%d at IRQ level: %lu. Win32Process %p, desktop %x\n",
            Thread->Tcb.Teb->ClientId.UniqueThread, Thread->Tcb.Teb->ClientId.UniqueProcess, KeGetCurrentIrql(), Win32Process, Win32Process->desktop);

        Win32Thread->process = Win32Process;
        Win32Thread->peThread = Thread;
        Win32Thread->desktop = Win32Process->desktop;
        Win32Thread->KeyboardLayout = UserGetDefaultKeyBoardLayout();
    }
    else
    {
        DPRINT("Destroying W32 thread TID:%d at IRQ level: %lu\n", Thread->Tcb.Teb->ClientId.UniqueThread, KeGetCurrentIrql());

        /* USER thread-level cleanup */
        UserEnterExclusive();
            //cleanup_clipboard_thread();
            destroy_thread_windows(Win32Thread);
            //free_msg_queue(Win32Thread); // FIXME!
            close_thread_desktop(Win32Thread);
        UserLeave();

        PsSetThreadWin32Thread(Thread, NULL);
    }

    DPRINT("Leave Win32kThreadCallback\n");

    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kGlobalAtomTableCallout(VOID)
{
    DPRINT("Win32kGlobalAtomTableCallout() is UNIMPLEMENTED\n");
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kPowerEventCallout(PWIN32_POWEREVENT_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kPowerStateCallout(PWIN32_POWERSTATE_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kJobCallout(PWIN32_JOBCALLOUT_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kDesktopOpenProcedure(PWIN32_OPENMETHOD_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kDesktopOkToCloseProcedure(PWIN32_OKAYTOCLOSEMETHOD_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kDesktopCloseProcedure(PWIN32_CLOSEMETHOD_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

VOID
APIENTRY
Win32kDesktopDeleteProcedure(PWIN32_DELETEMETHOD_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
}

NTSTATUS
APIENTRY
Win32kWindowStationOkToCloseProcedure(PWIN32_OKAYTOCLOSEMETHOD_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kWindowStationCloseProcedure(PWIN32_CLOSEMETHOD_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

VOID
APIENTRY
Win32kWindowStationDeleteProcedure(PWIN32_DELETEMETHOD_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
}

NTSTATUS
APIENTRY
Win32kWindowStationParseProcedure(PWIN32_PARSEMETHOD_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kWindowStationOpenProcedure(PWIN32_OPENMETHOD_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
Win32kWin32DataCollectionProcedure(PEPROCESS Process,
                                   PVOID Callback,
                                   PVOID Context)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
NtGdiFlushUserBatch(VOID)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/* DRIVER ENTRYPOINT *********************************************************/

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
{
    WIN32_CALLOUTS_FPNS CalloutData;

    DPRINT1("Win32k initialization: DO %p, RegPath %wZ\n", DriverObject,
        RegistryPath);

    /* Add system service table */
    if (!KeAddSystemServiceTable(Win32kSSDT,
                                 NULL,
                                 Win32kNumberOfSysCalls,
                                 Win32kSSPT,
                                 1))
    {
        DPRINT1("Error adding system service table!\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* Set up Win32 callouts */
    CalloutData.ProcessCallout = Win32kProcessCallout;
    CalloutData.ThreadCallout = Win32kThreadCallout;
    CalloutData.GlobalAtomTableCallout = Win32kGlobalAtomTableCallout;
    CalloutData.PowerEventCallout = Win32kPowerEventCallout;
    CalloutData.PowerStateCallout = Win32kPowerStateCallout;
    CalloutData.JobCallout = Win32kJobCallout;
    CalloutData.BatchFlushRoutine = NtGdiFlushUserBatch;
    CalloutData.DesktopOpenProcedure = Win32kDesktopOpenProcedure;
    CalloutData.DesktopOkToCloseProcedure = Win32kDesktopOkToCloseProcedure;
    CalloutData.DesktopCloseProcedure = Win32kDesktopCloseProcedure;
    CalloutData.DesktopDeleteProcedure = Win32kDesktopDeleteProcedure;
    CalloutData.WindowStationOkToCloseProcedure = Win32kWindowStationOkToCloseProcedure;
    CalloutData.WindowStationCloseProcedure = Win32kWindowStationCloseProcedure;
    CalloutData.WindowStationDeleteProcedure = Win32kWindowStationDeleteProcedure;
    CalloutData.WindowStationParseProcedure = Win32kWindowStationParseProcedure;
    CalloutData.WindowStationOpenProcedure = Win32kWindowStationOpenProcedure;
    CalloutData.Win32DataCollectionProcedure = Win32kWin32DataCollectionProcedure;

    /* Register them */
    PsEstablishWin32Callouts(&CalloutData);

    /* Initialize a list of loaded drivers in Win32 subsystem */
    InitializeListHead(&GlobalDriverListHead);

    /* Initialize user implementation */
    UserInitialize();

    /* Init object directories implementation */
    init_directories();

    /* Initialize GDI objects implementation */
    GdiHandleTable = GDIOBJ_iAllocHandleTable(&GdiTableSection);
    if (GdiHandleTable == NULL)
    {
        DPRINT1("Failed to initialize the GDI handle table.\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* Initialize handle-mapping */
    GDI_InitHandleMapping();

    /* Initialize window manager */
    SwmInitialize();

    /* Create stock objects */
    CreateStockBitmap();
    PALETTE_Init();

    /* Init video driver implementation */
    InitDcImpl();

    return STATUS_SUCCESS;
}

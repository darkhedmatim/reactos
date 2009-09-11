/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/interrupt.cpp
 * PURPOSE:         portcls interrupt object
 * PROGRAMMER:      Johannes Anderwald
 */


#include "private.hpp"

typedef struct
{
    LIST_ENTRY ListEntry;
    PINTERRUPTSYNCROUTINE SyncRoutine;
    PVOID DynamicContext;
}SYNC_ENTRY, *PSYNC_ENTRY;

class CInterruptSync : public IInterruptSync
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    STDMETHODIMP_(ULONG) AddRef()
    {
        InterlockedIncrement(&m_Ref);
        return m_Ref;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        InterlockedDecrement(&m_Ref);

        if (!m_Ref)
        {
            delete this;
            return 0;
        }
        return m_Ref;
    }
    IMP_IInterruptSync;
    CInterruptSync(IUnknown *OuterUnknown){}
    virtual ~CInterruptSync(){}

public:

    KSPIN_LOCK m_Lock;
    LIST_ENTRY m_ServiceRoutines;
    PKINTERRUPT m_Interrupt;
    INTERRUPTSYNCMODE m_Mode;
    PRESOURCELIST m_ResourceList;
    ULONG m_ResourceIndex;

    PINTERRUPTSYNCROUTINE m_SyncRoutine;
    PVOID m_DynamicContext;

    LONG m_Ref;

    friend BOOLEAN NTAPI CInterruptSynchronizedRoutine(IN PVOID  ServiceContext);
    friend BOOLEAN NTAPI IInterruptServiceRoutine(IN PKINTERRUPT  Interrupt, IN PVOID  ServiceContext);
};


//---------------------------------------------------------------
// IUnknown methods
//



NTSTATUS
NTAPI
CInterruptSync::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    DPRINT("CInterruptSync::QueryInterface: this %p\n", this);

    if (IsEqualGUIDAligned(refiid, IID_IInterruptSync) ||
        IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PUNKNOWN(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }

    DPRINT1("CInterruptSync::QueryInterface: this %p UNKNOWN interface requested\n", this);
    return STATUS_UNSUCCESSFUL;
}

//---------------------------------------------------------------
// CInterruptSync methods
//


BOOLEAN
NTAPI
CInterruptSynchronizedRoutine(
    IN PVOID  ServiceContext)
{
    CInterruptSync * This = (CInterruptSync*)ServiceContext;
    DPRINT("CInterruptSynchronizedRoutine this %p SyncRoutine %p Context %p\n", This, This->m_SyncRoutine, This->m_DynamicContext);
    return This->m_SyncRoutine(This, This->m_DynamicContext);
}

NTSTATUS
NTAPI
CInterruptSync::CallSynchronizedRoutine(
    IN PINTERRUPTSYNCROUTINE Routine,
    IN PVOID DynamicContext)
{
    KIRQL OldIrql;

    DPRINT("CInterruptSync::CallSynchronizedRoutine this %p Routine %p DynamicContext %p Irql %x Interrupt %p\n", this, Routine, DynamicContext, KeGetCurrentIrql(), m_Interrupt);

    if (!m_Interrupt)
    {
        DPRINT("CInterruptSync_CallSynchronizedRoutine %p no interrupt connected\n", this);
        if (KeGetCurrentIrql() > DISPATCH_LEVEL)
            return STATUS_UNSUCCESSFUL;

        KeAcquireSpinLock(&m_Lock, &OldIrql);
        m_SyncRoutine = Routine;
        m_DynamicContext = DynamicContext;
        CInterruptSynchronizedRoutine((PVOID)this);
        KeReleaseSpinLock(&m_Lock, OldIrql);

        return STATUS_SUCCESS;
    }

    m_SyncRoutine = Routine;
    m_DynamicContext = DynamicContext;

    if (KeSynchronizeExecution(m_Interrupt, CInterruptSynchronizedRoutine, (PVOID)this))
        return STATUS_SUCCESS;
    else
        return STATUS_UNSUCCESSFUL;
}

PKINTERRUPT
NTAPI
CInterruptSync::GetKInterrupt()
{
    DPRINT("CInterruptSynchronizedRoutine\n");

    return m_Interrupt;
}

BOOLEAN
NTAPI
IInterruptServiceRoutine(
    IN PKINTERRUPT  Interrupt,
    IN PVOID  ServiceContext)
{
    PLIST_ENTRY CurEntry;
    PSYNC_ENTRY Entry;
    NTSTATUS Status;
    BOOL Success;

    //DPRINT("IInterruptServiceRoutine Mode %u\n", m_Mode);

    CInterruptSync * This = (CInterruptSync*)ServiceContext;

    if (This->m_Mode == InterruptSyncModeNormal)
    {
        CurEntry = This->m_ServiceRoutines.Flink;
        while (CurEntry != &This->m_ServiceRoutines)
        {
            Entry = CONTAINING_RECORD(CurEntry, SYNC_ENTRY, ListEntry);
            Status = Entry->SyncRoutine((CInterruptSync*)This, Entry->DynamicContext);
            if (NT_SUCCESS(Status))
            {
                return TRUE;
            }
            CurEntry = CurEntry->Flink;
        }
        return FALSE;
    }
    else if (This->m_Mode == InterruptSyncModeAll)
    {
        CurEntry = This->m_ServiceRoutines.Flink;
        while (CurEntry != &This->m_ServiceRoutines)
        {
            Entry = CONTAINING_RECORD(CurEntry, SYNC_ENTRY, ListEntry);
            Status = Entry->SyncRoutine((CInterruptSync*)This, Entry->DynamicContext);
            CurEntry = CurEntry->Flink;
        }
        DPRINT("Returning TRUE with mode InterruptSyncModeAll\n");
        return TRUE; //FIXME
    }
    else if (This->m_Mode == InterruptSyncModeRepeat)
    {
        do
        {
            Success = FALSE;
            CurEntry = This->m_ServiceRoutines.Flink;
            while (CurEntry != &This->m_ServiceRoutines)
            {
                Entry = CONTAINING_RECORD(CurEntry, SYNC_ENTRY, ListEntry);
                Status = Entry->SyncRoutine((CInterruptSync*)This, Entry->DynamicContext);
                if (NT_SUCCESS(Status))
                    Success = TRUE;
                CurEntry = CurEntry->Flink;
            }
        }while(Success);
        DPRINT("Returning TRUE with mode InterruptSyncModeRepeat\n");
        return TRUE; //FIXME
    }
    else
    {
        DPRINT("Unknown mode %u\n", This->m_Mode);
        return FALSE; //FIXME
    }
}


NTSTATUS
NTAPI
CInterruptSync::Connect()
{
    NTSTATUS Status;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;

    DPRINT("CInterruptSync::Connect\n");
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    Descriptor = m_ResourceList->FindTranslatedEntry(CmResourceTypeInterrupt, m_ResourceIndex);
    if (!Descriptor)
        return STATUS_UNSUCCESSFUL;

    if (IsListEmpty(&m_ServiceRoutines))
        return STATUS_UNSUCCESSFUL;

    Status = IoConnectInterrupt(&m_Interrupt, 
                                IInterruptServiceRoutine,
                                (PVOID)this,
                                &m_Lock,
                                Descriptor->u.Interrupt.Vector,
                                Descriptor->u.Interrupt.Level,
                                Descriptor->u.Interrupt.Level,
                                (KINTERRUPT_MODE)(Descriptor->Flags & CM_RESOURCE_INTERRUPT_LATCHED),
                                (Descriptor->Flags != CM_RESOURCE_INTERRUPT_LATCHED),
                                Descriptor->u.Interrupt.Affinity,
                                FALSE);

    DPRINT("CInterruptSync::Connect result %x\n", Status);
    return Status;
}


VOID
NTAPI
CInterruptSync::Disconnect()
{
    DPRINT("CInterruptSync::Disconnect\n");
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!m_Interrupt)
    {
        DPRINT("CInterruptSync_Disconnect %p no interrupt connected\n", this);
        return;
    }

    IoDisconnectInterrupt(m_Interrupt);
    m_Interrupt = NULL;
}

NTSTATUS
NTAPI
CInterruptSync::RegisterServiceRoutine(
    IN      PINTERRUPTSYNCROUTINE   Routine,
    IN      PVOID                   DynamicContext,
    IN      BOOLEAN                 First)
{
    PSYNC_ENTRY NewEntry;

    DPRINT("CInterruptSync::RegisterServiceRoutine\n");
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    NewEntry = (PSYNC_ENTRY)AllocateItem(NonPagedPool, sizeof(SYNC_ENTRY), TAG_PORTCLASS);
    if (!NewEntry)
        return STATUS_INSUFFICIENT_RESOURCES;

    NewEntry->SyncRoutine = Routine;
    NewEntry->DynamicContext = DynamicContext;

    if (First)
        InsertHeadList(&m_ServiceRoutines, &NewEntry->ListEntry);
    else
        InsertTailList(&m_ServiceRoutines, &NewEntry->ListEntry);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PcNewInterruptSync(
    OUT PINTERRUPTSYNC* OutInterruptSync,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  PRESOURCELIST ResourceList,
    IN  ULONG ResourceIndex,
    IN  INTERRUPTSYNCMODE Mode)
{
    CInterruptSync * This;
    NTSTATUS Status;

    DPRINT("PcNewInterruptSync entered OutInterruptSync %p OuterUnknown %p ResourceList %p ResourceIndex %u Mode %d\n", 
            OutInterruptSync, OuterUnknown, ResourceList, ResourceIndex, Mode);

    if (!OutInterruptSync || !ResourceList || Mode < InterruptSyncModeNormal || Mode > InterruptSyncModeRepeat)
        return STATUS_INVALID_PARAMETER;

    if (ResourceIndex > ResourceList->NumberOfEntriesOfType(CmResourceTypeInterrupt))
        return STATUS_INVALID_PARAMETER;

    This = new(NonPagedPool, TAG_PORTCLASS)CInterruptSync(OuterUnknown);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = This->QueryInterface(IID_IInterruptSync, (PVOID*)OutInterruptSync);

    if (!NT_SUCCESS(Status))
    {
        delete This;
        return Status;
    }

    ResourceList->AddRef();

    //
    // initialize object
    //
    This->m_Mode = Mode;
    This->m_ResourceIndex = ResourceIndex;
    This->m_ResourceList = ResourceList;
    InitializeListHead(&This->m_ServiceRoutines);
    KeInitializeSpinLock(&This->m_Lock);

    return Status;
}

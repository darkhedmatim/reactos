/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/i386/kiinit.c
 * PURPOSE:         Kernel Initialization for x86 CPUs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define REQUIRED_FEATURE_BITS (KF_RDTSC|KF_CR4|KF_CMPXCHG8B|KF_XMMI|KF_XMMI64| \
                               KF_NX_BIT)

/* GLOBALS *******************************************************************/

/* Function pointer for early debug prints */
ULONG (*FrLdrDbgPrint)(const char *Format, ...);

/* Spinlocks used only on X86 */
KSPIN_LOCK KiFreezeExecutionLock;

/* BIOS Memory Map. Not NTLDR-compliant yet */
extern ULONG KeMemoryMapRangeCount;
extern ADDRESS_RANGE KeMemoryMap[64];

KIPCR KiInitialPcr;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KiInitMachineDependent(VOID)
{
#if 0
    ULONG Protect;
    ULONG CpuCount;
    BOOLEAN FbCaching = FALSE;
    NTSTATUS Status;
    ULONG ReturnLength;
    ULONG i, Affinity, Sample = 0;
    PFX_SAVE_AREA FxSaveArea;
    ULONG MXCsrMask = 0xFFBF;
    ULONG Dummy[4];
    KI_SAMPLE_MAP Samples[4];
    PKI_SAMPLE_MAP CurrentSample = Samples;

    /* Check for large page support */
    if (KeFeatureBits & KF_LARGE_PAGE)
    {
        /* FIXME: Support this */
        DPRINT1("Large Page support detected but not yet taken advantage of!\n");
    }

    /* Check for global page support */
    if (KeFeatureBits & KF_GLOBAL_PAGE)
    {
        /* Do an IPI to enable it on all CPUs */
        CpuCount = KeNumberProcessors;
        KeIpiGenericCall(Ki386EnableGlobalPage, (ULONG_PTR)&CpuCount);
    }

    /* Check for PAT and/or MTRR support */
    if (KeFeatureBits & (KF_PAT | KF_MTRR))
    {
        /* Query the HAL to make sure we can use it */
        Status = HalQuerySystemInformation(HalFrameBufferCachingInformation,
                                           sizeof(BOOLEAN),
                                           &FbCaching,
                                           &ReturnLength);
        if ((NT_SUCCESS(Status)) && (FbCaching))
        {
            /* We can't, disable it */
            KeFeatureBits &= ~(KF_PAT | KF_MTRR);
        }
    }

    /* Check for PAT support and enable it */
    if (KeFeatureBits & KF_PAT) KiInitializePAT();

    /* Assume no errata for now */
    SharedUserData->ProcessorFeatures[PF_FLOATING_POINT_PRECISION_ERRATA] = 0;

    /* Check if we have an NPX */
    if (KeI386NpxPresent)
    {
        /* Loop every CPU */
        i = KeActiveProcessors;
        for (Affinity = 1; i; Affinity <<= 1)
        {
            /* Check if this is part of the set */
            if (i & Affinity)
            {
                /* Run on this CPU */
                i &= ~Affinity;
                KeSetSystemAffinityThread(Affinity);

                /* Detect FPU errata */
                if (KiIsNpxErrataPresent())
                {
                    /* Disable NPX support */
                    KeI386NpxPresent = FALSE;
                    SharedUserData->
                        ProcessorFeatures[PF_FLOATING_POINT_PRECISION_ERRATA] =
                        TRUE;
                    break;
                }
            }
        }
    }

    /* If there's no NPX, then we're emulating the FPU */
    SharedUserData->ProcessorFeatures[PF_FLOATING_POINT_EMULATED] =
        !KeI386NpxPresent;

    /* Check if there's no NPX, so that we can disable associated features */
    if (!KeI386NpxPresent)
    {
        /* Remove NPX-related bits */
        KeFeatureBits &= ~(KF_XMMI64 | KF_XMMI | KF_FXSR | KF_MMX);

        /* Disable kernel flags */
        KeI386FxsrPresent = KeI386XMMIPresent = FALSE;

        /* Disable processor features that might've been set until now */
        SharedUserData->ProcessorFeatures[PF_FLOATING_POINT_PRECISION_ERRATA] =
        SharedUserData->ProcessorFeatures[PF_XMMI64_INSTRUCTIONS_AVAILABLE]   =
        SharedUserData->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE]     =
        SharedUserData->ProcessorFeatures[PF_3DNOW_INSTRUCTIONS_AVAILABLE]    =
        SharedUserData->ProcessorFeatures[PF_MMX_INSTRUCTIONS_AVAILABLE] = 0;
    }

    /* Check for CR4 support */
    if (KeFeatureBits & KF_CR4)
    {
        /* Do an IPI call to enable the Debug Exceptions */
        CpuCount = KeNumberProcessors;
        KeIpiGenericCall(Ki386EnableDE, (ULONG_PTR)&CpuCount);
    }

    /* Check if FXSR was found */
    if (KeFeatureBits & KF_FXSR)
    {
        /* Do an IPI call to enable the FXSR */
        CpuCount = KeNumberProcessors;
        KeIpiGenericCall(Ki386EnableFxsr, (ULONG_PTR)&CpuCount);

        /* Check if XMM was found too */
        if (KeFeatureBits & KF_XMMI)
        {
            /* Do an IPI call to enable XMMI exceptions */
            CpuCount = KeNumberProcessors;
            KeIpiGenericCall(Ki386EnableXMMIExceptions, (ULONG_PTR)&CpuCount);

            /* FIXME: Implement and enable XMM Page Zeroing for Mm */

            /* Patch the RtlPrefetchMemoryNonTemporal routine to enable it */
            Protect = MmGetPageProtect(NULL, RtlPrefetchMemoryNonTemporal);
            MmSetPageProtect(NULL,
                             RtlPrefetchMemoryNonTemporal,
                             Protect | PAGE_IS_WRITABLE);
            *(PCHAR)RtlPrefetchMemoryNonTemporal = 0x90;
            MmSetPageProtect(NULL, RtlPrefetchMemoryNonTemporal, Protect);
        }
    }

    /* Check for, and enable SYSENTER support */
    KiRestoreFastSyscallReturnState();

    /* Loop every CPU */
    i = KeActiveProcessors;
    for (Affinity = 1; i; Affinity <<= 1)
    {
        /* Check if this is part of the set */
        if (i & Affinity)
        {
            /* Run on this CPU */
            i &= ~Affinity;
            KeSetSystemAffinityThread(Affinity);

            /* Reset MHz to 0 for this CPU */
            KeGetCurrentPrcb()->MHz = 0;

            /* Check if we can use RDTSC */
            if (KeFeatureBits & KF_RDTSC)
            {
                /* Start sampling loop */
                for (;;)
                {
                    /* Do a dummy CPUID to start the sample */
                    CPUID(Dummy, 0);

                    /* Fill out the starting data */
                    CurrentSample->PerfStart = KeQueryPerformanceCounter(NULL);
                    CurrentSample->TSCStart = __rdtsc();
                    CurrentSample->PerfFreq.QuadPart = -50000;

                    /* Sleep for this sample */
                    KeDelayExecutionThread(KernelMode,
                                           FALSE,
                                           &CurrentSample->PerfFreq);

                    /* Do another dummy CPUID */
                    CPUID(Dummy, 0);

                    /* Fill out the ending data */
                    CurrentSample->PerfEnd =
                        KeQueryPerformanceCounter(&CurrentSample->PerfFreq);
                    CurrentSample->TSCEnd = __rdtsc();

                    /* Calculate the differences */
                    CurrentSample->PerfDelta = CurrentSample->PerfEnd.QuadPart -
                                               CurrentSample->PerfStart.QuadPart;
                    CurrentSample->TSCDelta = CurrentSample->TSCEnd -
                                              CurrentSample->TSCStart;

                    /* Compute CPU Speed */
                    CurrentSample->MHz = (ULONG)((CurrentSample->TSCDelta *
                                                  CurrentSample->
                                                  PerfFreq.QuadPart + 500000) /
                                                 (CurrentSample->PerfDelta *
                                                  1000000));

                    /* Check if this isn't the first sample */
                    if (Sample)
                    {
                        /* Check if we got a good precision within 1MHz */
                        if ((CurrentSample->MHz == CurrentSample[-1].MHz) ||
                            (CurrentSample->MHz == CurrentSample[-1].MHz + 1) ||
                            (CurrentSample->MHz == CurrentSample[-1].MHz - 1))
                        {
                            /* We did, stop sampling */
                            break;
                        }
                    }

                    /* Move on */
                    CurrentSample++;
                    Sample++;

                    if (Sample == sizeof(Samples) / sizeof(Samples[0]))
                    {
                        /* Restart */
                        CurrentSample = Samples;
                        Sample = 0;
                    }
                }

                /* Save the CPU Speed */
                KeGetCurrentPrcb()->MHz = CurrentSample[-1].MHz;
            }

            /* Check if we have MTRR */
            if (KeFeatureBits & KF_MTRR)
            {
                /* Then manually initialize MTRR for the CPU */
                KiInitializeMTRR(i ? FALSE : TRUE);
            }

            /* Check if we have AMD MTRR and initialize it for the CPU */
            if (KeFeatureBits & KF_AMDK6MTRR) KiAmdK6InitializeMTRR();

            /* Check if this is a buggy Pentium and apply the fixup if so */
            if (KiI386PentiumLockErrataPresent) KiI386PentiumLockErrataFixup();

            /* Check if the CPU supports FXSR */
            if (KeFeatureBits & KF_FXSR)
            {
                /* Get the current thread NPX state */
                FxSaveArea = (PVOID)
                             ((ULONG_PTR)KeGetCurrentThread()->InitialStack -
                             NPX_FRAME_LENGTH);

                /* Clear initial MXCsr mask */
                FxSaveArea->U.FxArea.MXCsrMask = 0;

                /* Save the current NPX State */
#ifdef __GNUC__
                asm volatile("fxsave %0\n\t" : "=m" (*FxSaveArea));
#else
                __asm fxsave [FxSaveArea]
#endif
                /* Check if the current mask doesn't match the reserved bits */
                if (FxSaveArea->U.FxArea.MXCsrMask != 0)
                {
                    /* Then use whatever it's holding */
                    MXCsrMask = FxSaveArea->U.FxArea.MXCsrMask;
                }

                /* Check if nobody set the kernel-wide mask */
                if (!KiMXCsrMask)
                {
                    /* Then use the one we calculated above */
                    KiMXCsrMask = MXCsrMask;
                }
                else
                {
                    /* Was it set to the same value we found now? */
                    if (KiMXCsrMask != MXCsrMask)
                    {
                        /* No, something is definitely wrong */
                        KEBUGCHECKEX(MULTIPROCESSOR_CONFIGURATION_NOT_SUPPORTED,
                                     KF_FXSR,
                                     KiMXCsrMask,
                                     MXCsrMask,
                                     0);
                    }
                }

                /* Now set the kernel mask */
                KiMXCsrMask &= MXCsrMask;
            }
        }
    }

    /* Return affinity back to where it was */
    KeRevertToUserAffinityThread();

    /* NT allows limiting the duration of an ISR with a registry key */
    if (KiTimeLimitIsrMicroseconds)
    {
        /* FIXME: TODO */
        DPRINT1("ISR Time Limit not yet supported\n");
    }
#endif
}

VOID
NTAPI
KiInitializePcr(IN PKIPCR Pcr,
                IN ULONG ProcessorNumber,
                IN PKTHREAD IdleThread,
                IN PVOID DpcStack)
{
    KDESCRIPTOR GdtDescriptor = {{0},0,0}, IdtDescriptor = {{0},0,0};
    PKGDTENTRY64 TssEntry;
    USHORT Tr = 0;

    /* Zero out the PCR */
    RtlZeroMemory(Pcr, PAGE_SIZE);

    /* Set pointers to ourselves */
    Pcr->Self = (PKPCR)Pcr;
    Pcr->CurrentPrcb = &Pcr->Prcb;

    /* Set the PCR Version */
    Pcr->MajorVersion = PCR_MAJOR_VERSION;
    Pcr->MinorVersion = PCR_MINOR_VERSION;

    /* Set the PRCB Version */
    Pcr->Prcb.MajorVersion = 1;
    Pcr->Prcb.MinorVersion = 1;

    /* Set the Build Type */
    Pcr->Prcb.BuildType = 0;
#ifndef CONFIG_SMP
    Pcr->Prcb.BuildType |= PRCB_BUILD_UNIPROCESSOR;
#endif
#ifdef DBG
    Pcr->Prcb.BuildType |= PRCB_BUILD_DEBUG;
#endif

    /* Set the Processor Number and current Processor Mask */
    Pcr->Prcb.Number = (UCHAR)ProcessorNumber;
    Pcr->Prcb.SetMember = 1 << ProcessorNumber;

    /* Get GDT and IDT descriptors */
    __sgdt(&GdtDescriptor.Limit);
    __sidt(&IdtDescriptor.Limit);
    Pcr->GdtBase = (PVOID)GdtDescriptor.Base;
    Pcr->IdtBase = (PKIDTENTRY)IdtDescriptor.Base;

    /* Get TSS Selector */
    __str(&Tr);
    ASSERT(Tr == KGDT_TSS);

    /* Get TSS Entry */
    TssEntry = KiGetGdtEntry(Pcr->GdtBase, Tr);

    /* Get the KTSS itself */
    Pcr->TssBase = KiGetGdtDescriptorBase(TssEntry);

    Pcr->Prcb.RspBase = Pcr->TssBase->Rsp0; // FIXME

    /* Set DPC Stack */
    Pcr->Prcb.DpcStack = DpcStack;

    /* Setup the processor set */
    Pcr->Prcb.MultiThreadProcessorSet = Pcr->Prcb.SetMember;

    /* Clear DR6/7 to cleanup bootloader debugging */
    Pcr->Prcb.ProcessorState.SpecialRegisters.KernelDr6 = 0;
    Pcr->Prcb.ProcessorState.SpecialRegisters.KernelDr7 = 0;

    /* Set the Current Thread */
    Pcr->Prcb.CurrentThread = IdleThread;

    /* Start us out at PASSIVE_LEVEL */
    Pcr->Irql = PASSIVE_LEVEL;
    KeSetCurrentIrql(PASSIVE_LEVEL);

}

VOID
NTAPI
KiInitializeKernel(IN PKPROCESS InitProcess,
                   IN PKTHREAD InitThread,
                   IN PVOID IdleStack,
                   IN PKPRCB Prcb,
                   IN CCHAR Number,
                   IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG FeatureBits;
    ULONG PageDirectory[2];
    PVOID DpcStack;

    /* Detect and set the CPU Type */
    KiSetProcessorType();

    /* Initialize the Power Management Support for this PRCB */
//    PoInitializePrcb(Prcb);

    /* Get the processor features for the CPU */
    FeatureBits = KiGetFeatureBits();

    /* Check if we support all needed features */
    if ((FeatureBits & REQUIRED_FEATURE_BITS) != REQUIRED_FEATURE_BITS)
    {
        /* If not, bugcheck system */
        DPRINT1("CPU doesn't have needed features! Has: 0x%x, required: 0x%x\n",
                FeatureBits, REQUIRED_FEATURE_BITS);
        KeBugCheck(0);
    }

    /* Set the default NX policy (opt-in) */
    SharedUserData->NXSupportPolicy = NX_SUPPORT_POLICY_OPTIN;

    /* Check if NPX is always on */
    if (strstr(KeLoaderBlock->LoadOptions, "NOEXECUTE=ALWAYSON"))
    {
        /* Set it always on */
        SharedUserData->NXSupportPolicy = NX_SUPPORT_POLICY_ALWAYSON;
        FeatureBits |= KF_NX_ENABLED;
    }
    else if (strstr(KeLoaderBlock->LoadOptions, "NOEXECUTE=OPTOUT"))
    {
        /* Set it in opt-out mode */
        SharedUserData->NXSupportPolicy = NX_SUPPORT_POLICY_OPTOUT;
        FeatureBits |= KF_NX_ENABLED;
    }
    else if ((strstr(KeLoaderBlock->LoadOptions, "NOEXECUTE=OPTIN")) ||
             (strstr(KeLoaderBlock->LoadOptions, "NOEXECUTE")))
    {
        /* Set the feature bits */
        FeatureBits |= KF_NX_ENABLED;
    }
    else if ((strstr(KeLoaderBlock->LoadOptions, "NOEXECUTE=ALWAYSOFF")) ||
             (strstr(KeLoaderBlock->LoadOptions, "EXECUTE")))
    {
        /* Set disabled mode */
        SharedUserData->NXSupportPolicy = NX_SUPPORT_POLICY_ALWAYSOFF;
        FeatureBits |= KF_NX_DISABLED;
    }

    /* Save feature bits */
    Prcb->FeatureBits = FeatureBits;

    /* Initialize the CPU features */
    KiInitializeCpuFeatures();

    /* Save CPU state */
    KiSaveProcessorControlState(&Prcb->ProcessorState);

    /* Get cache line information for this CPU */
    KiGetCacheInformation();

    /* Initialize spinlocks and DPC data */
    KiInitSpinLocks(Prcb, Number);

    /* Check if this is the Boot CPU */
    if (Number == 0)
    {
        /* Set Node Data */
        KeNodeBlock[0] = &KiNode0;
        Prcb->ParentNode = KeNodeBlock[0];
        KeNodeBlock[0]->ProcessorMask = Prcb->SetMember;

        /* Set boot-level flags */
        KeI386NpxPresent = TRUE;
        KeI386CpuType = Prcb->CpuType;
        KeI386CpuStep = Prcb->CpuStep;
        KeProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
        KeProcessorLevel = (USHORT)Prcb->CpuType;
        if (Prcb->CpuID) KeProcessorRevision = Prcb->CpuStep;
        KeFeatureBits = FeatureBits;
        KeI386FxsrPresent = (KeFeatureBits & KF_FXSR) ? TRUE : FALSE;
        KeI386XMMIPresent = (KeFeatureBits & KF_XMMI) ? TRUE : FALSE;

        /* Set the current MP Master KPRCB to the Boot PRCB */
        Prcb->MultiThreadSetMaster = Prcb;

        /* Lower to APC_LEVEL */
        KeLowerIrql(APC_LEVEL);

        /* Initialize some spinlocks */
        KeInitializeSpinLock(&KiFreezeExecutionLock);

        /* Initialize portable parts of the OS */
        KiInitSystem();

        /* Initialize the Idle Process and the Process Listhead */
        InitializeListHead(&KiProcessListHead);
        PageDirectory[0] = 0;
        PageDirectory[1] = 0;
        KeInitializeProcess(InitProcess,
                            0,
                            0xFFFFFFFF,
                            PageDirectory,
                            FALSE);
        InitProcess->QuantumReset = MAXCHAR;
    }
    else
    {
        /* FIXME */
        DPRINT1("SMP Boot support not yet present\n");
    }

    /* HACK for MmUpdatePageDir */
    ((PETHREAD)InitThread)->ThreadsProcess = (PEPROCESS)InitProcess;

    /* Setup the Idle Thread */
    KeInitializeThread(InitProcess,
                       InitThread,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       IdleStack);

    InitThread->NextProcessor = Number;
    InitThread->Priority = HIGH_PRIORITY;
    InitThread->State = Running;
    InitThread->Affinity = 1 << Number;
    InitThread->WaitIrql = DISPATCH_LEVEL;
    InitProcess->ActiveProcessors = 1 << Number;

    /* Set basic CPU Features that user mode can read */
    SharedUserData->ProcessorFeatures[PF_MMX_INSTRUCTIONS_AVAILABLE] =
        (KeFeatureBits & KF_MMX) ? TRUE: FALSE;
    SharedUserData->ProcessorFeatures[PF_COMPARE_EXCHANGE_DOUBLE] =
        (KeFeatureBits & KF_CMPXCHG8B) ? TRUE: FALSE;
    SharedUserData->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE] =
        ((KeFeatureBits & KF_FXSR) && (KeFeatureBits & KF_XMMI)) ? TRUE: FALSE;
    SharedUserData->ProcessorFeatures[PF_XMMI64_INSTRUCTIONS_AVAILABLE] =
        ((KeFeatureBits & KF_FXSR) && (KeFeatureBits & KF_XMMI64)) ? TRUE: FALSE;
    SharedUserData->ProcessorFeatures[PF_3DNOW_INSTRUCTIONS_AVAILABLE] =
        (KeFeatureBits & KF_3DNOW) ? TRUE: FALSE;
    SharedUserData->ProcessorFeatures[PF_RDTSC_INSTRUCTION_AVAILABLE] =
        (KeFeatureBits & KF_RDTSC) ? TRUE: FALSE;

    /* Set up the thread-related fields in the PRCB */
    Prcb->CurrentThread = InitThread;
    Prcb->NextThread = NULL;
    Prcb->IdleThread = InitThread;

    /* Initialize the Kernel Executive */
    ExpInitializeExecutive(Number, LoaderBlock);

    /* Only do this on the boot CPU */
    if (Number == 0)
    {
        /* Calculate the time reciprocal */
        KiTimeIncrementReciprocal =
            KiComputeReciprocal(KeMaximumIncrement,
                                &KiTimeIncrementShiftCount);

        /* Update DPC Values in case they got updated by the executive */
        Prcb->MaximumDpcQueueDepth = KiMaximumDpcQueueDepth;
        Prcb->MinimumDpcRate = KiMinimumDpcRate;
        Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;

        /* Allocate the DPC Stack */
        DpcStack = MmCreateKernelStack(FALSE, 0);
        if (!DpcStack) KeBugCheckEx(NO_PAGES_AVAILABLE, 1, 0, 0, 0);
        Prcb->DpcStack = DpcStack;

        /* Allocate the IOPM save area. */
//        Ki386IopmSaveArea = ExAllocatePoolWithTag(PagedPool,
//                                                  PAGE_SIZE * 2,
//                                                  TAG('K', 'e', ' ', ' '));
//        if (!Ki386IopmSaveArea)
//        {
//            /* Bugcheck. We need this for V86/VDM support. */
//            KeBugCheckEx(NO_PAGES_AVAILABLE, 2, PAGE_SIZE * 2, 0, 0);
//        }
    }

    /* Raise to Dispatch */
    KfRaiseIrql(DISPATCH_LEVEL);

    /* Set the Idle Priority to 0. This will jump into Phase 1 */
    KeSetPriorityThread(InitThread, 0);

    /* If there's no thread scheduled, put this CPU in the Idle summary */
    KiAcquirePrcbLock(Prcb);
    if (!Prcb->NextThread) KiIdleSummary |= 1 << Number;
    KiReleasePrcbLock(Prcb);

    /* Raise back to HIGH_LEVEL and clear the PRCB for the loader block */
    KfRaiseIrql(HIGH_LEVEL);
    LoaderBlock->Prcb = 0;
}

VOID
NTAPI
KiSystemStartup(IN ULONG_PTR Dummy,
                IN PROS_LOADER_PARAMETER_BLOCK LoaderBlock)
{
    FrLdrDbgPrint = ((PLOADER_PARAMETER_BLOCK)Dummy)->u.I386.CommonDataArea;
    FrLdrDbgPrint("Hello from KiSystemStartup!!!\n");

    /* HACK, because freeldr maps page 0 */
    MiAddressToPte((PVOID)0)->u.Hard.Valid = 0;

    KiSystemStartupReal((PLOADER_PARAMETER_BLOCK)Dummy);

//    KiRosPrepareForSystemStartup(Dummy, LoaderBlock);
}


VOID
NTAPI
KiSystemStartupReal(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG Cpu;
    PKTHREAD InitialThread;
    ULONG64 InitialStack;
    PKIPCR Pcr;

    /* Save the loader block */
    KeLoaderBlock = LoaderBlock;

    /* Get the current CPU number */
    Cpu = KeNumberProcessors++;

    /* Set active processors */
    KeActiveProcessors |= 1 << Cpu;

    /* LoaderBlock initialization for Cpu 0 */
    if (Cpu == 0)
    {
        /* Set the initial stack, idle thread and process */
        LoaderBlock->KernelStack = (ULONG_PTR)P0BootStack;
        LoaderBlock->Thread = (ULONG_PTR)&KiInitialThread;
        LoaderBlock->Process = (ULONG_PTR)&KiInitialProcess.Pcb;
        LoaderBlock->Prcb = (ULONG_PTR)&KiInitialPcr.Prcb;
    }

    /* Get Pcr from loader block */
    Pcr = CONTAINING_RECORD(LoaderBlock->Prcb, KIPCR, Prcb);

    /* Set the PRCB for this Processor */
    KiProcessorBlock[Cpu] = &Pcr->Prcb;

    /* Set GS base */
    __writemsr(X86_MSR_GSBASE, (ULONG64)Pcr);
    __writemsr(X86_MSR_KERNEL_GSBASE, (ULONG64)Pcr);

    /* Load Ring 3 selectors for DS/ES/FS */
    Ke386SetDs(KGDT_64_DATA | RPL_MASK);
    Ke386SetEs(KGDT_64_DATA | RPL_MASK);
    Ke386SetFs(KGDT_32_R3_TEB | RPL_MASK);

    /* LDT is unused */
//    __lldt(0);

    /* Align stack to 16 bytes */
    LoaderBlock->KernelStack &= ~(16 - 1);

    /* Save the initial thread and stack */
    InitialStack = LoaderBlock->KernelStack; // Checkme
    InitialThread = (PKTHREAD)LoaderBlock->Thread;

    /* Clean the APC List Head */
    InitializeListHead(&InitialThread->ApcState.ApcListHead[KernelMode]);

    /* Set us as the current process */
    InitialThread->ApcState.Process = (PVOID)LoaderBlock->Process;

    /* Initialize the PCR */
    KiInitializePcr(Pcr, Cpu, InitialThread, KiDoubleFaultStack);

    /* Initial setup for the boot CPU */
    if (Cpu == 0)
    {
        /* Setup the TSS descriptors and entries */
        KiInitializeTss(Pcr->TssBase, InitialStack);

        /* Setup the IDT */
        KeInitExceptions();

        /* HACK: misuse this function to pass a function pointer to kdcom */
        KdDebuggerInitialize1((PVOID)FrLdrDbgPrint);

        /* Initialize debugging system */
        KdInitSystem(0, KeLoaderBlock);

        /* Check for break-in */
        if (KdPollBreakIn()) DbgBreakPointWithStatus(DBG_STATUS_CONTROL_C);

        /* Hack! Wait for the debugger! */
        //while (!KdPollBreakIn());

    }

    DPRINT("Pcr = %p, Gdt = %p, Idt = %p, Tss = %p\n",
           Pcr, Pcr->GdtBase, Pcr->IdtBase, Pcr->TssBase);

    /* Initialize the Processor with HAL */
    HalInitializeProcessor(Cpu, KeLoaderBlock);

    /* Loop until we can release the freeze lock */
    do
    {
        /* Loop until execution can continue */
        while (*(volatile PKSPIN_LOCK*)&KiFreezeExecutionLock == (PVOID)1);
    } while(InterlockedBitTestAndSet64((PLONG64)&KiFreezeExecutionLock, 0));

    /* Raise to HIGH_LEVEL */
    KfRaiseIrql(HIGH_LEVEL);

    /* Switch to new kernel stack and start kernel bootstrapping */
    KiSetupStackAndInitializeKernel(&KiInitialProcess.Pcb,
                                    InitialThread,
                                    (PVOID)InitialStack,
                                    &Pcr->Prcb,
                                    (CCHAR)Cpu,
                                    KeLoaderBlock);
}


VOID
NTAPI
KiInitializeKernelAndGotoIdleLoop(IN PKPROCESS InitProcess,
                                  IN PKTHREAD InitThread,
                                  IN PVOID IdleStack,
                                  IN PKPRCB Prcb,
                                  IN CCHAR Number,
                                  IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
//    DbgBreakPointWithStatus(0);

    /* Initialize kernel */
    KiInitializeKernel(InitProcess,
                       InitThread,
                       IdleStack,
                       Prcb,
                       Number,
                       KeLoaderBlock);

    /* Set the priority of this thread to 0 */
    InitThread->Priority = 0;

    /* Force interrupts enabled and lower IRQL back to DISPATCH_LEVEL */
    _enable();
    KeLowerIrql(DISPATCH_LEVEL);

    /* Set the right wait IRQL */
    InitThread->WaitIrql = DISPATCH_LEVEL;

    /* Jump into the idle loop */
    KiIdleLoop();
}

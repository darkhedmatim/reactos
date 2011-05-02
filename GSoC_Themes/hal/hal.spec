@ fastcall -arch=arm ExAcquireFastMutex(ptr)
@ fastcall -arch=arm ExReleaseFastMutex(ptr)
@ fastcall -arch=i386 ExAcquireFastMutex(ptr) ntoskrnl.ExiAcquireFastMutex
@ fastcall -arch=i386 ExReleaseFastMutex(ptr) ntoskrnl.ExiReleaseFastMutex
@ fastcall -arch=i386 ExTryToAcquireFastMutex(ptr) ntoskrnl.ExiTryToAcquireFastMutex
@ fastcall -arch=arm ExTryToAcquireFastMutex(ptr)
@ fastcall HalClearSoftwareInterrupt(long)
@ fastcall HalRequestSoftwareInterrupt(long)
@ fastcall HalSystemVectorDispatchEntry(long long long)
@ fastcall -arch=i386,arm KeAcquireInStackQueuedSpinLock(ptr ptr)
@ fastcall -arch=i386,arm KeAcquireInStackQueuedSpinLockRaiseToSynch(ptr ptr)
@ fastcall -arch=i386,arm KeAcquireSpinLockRaiseToSynch(ptr)
@ fastcall -arch=i386,arm KeAcquireQueuedSpinLock(ptr)
@ fastcall -arch=i386,arm KeAcquireQueuedSpinLockRaiseToSynch(ptr)
@ fastcall -arch=i386,arm KeReleaseInStackQueuedSpinLock(ptr)
@ fastcall -arch=i386,arm KeReleaseQueuedSpinLock(ptr long)
@ fastcall -arch=i386,arm KeTryToAcquireQueuedSpinLock(long ptr)
@ fastcall -arch=i386,arm KeTryToAcquireQueuedSpinLockRaiseToSynch(long ptr)
@ fastcall -arch=i386,arm KfAcquireSpinLock(ptr)
@ fastcall -arch=i386,arm KfLowerIrql(long)
@ fastcall -arch=i386,arm KfRaiseIrql(long)
@ fastcall -arch=i386,arm KfReleaseSpinLock(ptr long)
@ stdcall HalAcquireDisplayOwnership(ptr)
@ stdcall HalAdjustResourceList(ptr)
@ stdcall HalAllProcessorsStarted()
@ stdcall HalAllocateAdapterChannel(ptr ptr long ptr)
@ stdcall HalAllocateCommonBuffer(ptr long ptr long)
@ stdcall HalAllocateCrashDumpRegisters(ptr ptr)
@ stdcall HalAssignSlotResources(ptr ptr ptr ptr long long long ptr)
@ stdcall HalBeginSystemInterrupt(long long ptr)
@ stdcall HalCalibratePerformanceCounter(ptr long long)
@ stdcall HalDisableSystemInterrupt(long long)
@ stdcall HalDisplayString(str)
@ stdcall HalEnableSystemInterrupt(long long long)
@ stdcall HalEndSystemInterrupt(long long)
@ stdcall HalFlushCommonBuffer(long long long long long)
@ stdcall HalFreeCommonBuffer(ptr long long long ptr long)
@ stdcall HalGetAdapter(ptr ptr)
@ stdcall HalGetBusData(long long long ptr long)
@ stdcall HalGetBusDataByOffset(long long long ptr long long)
@ stdcall HalGetEnvironmentVariable(str long str)
@ fastcall -arch=arm HalGetInterruptSource()
@ stdcall HalGetInterruptVector(long long long long ptr ptr)
@ stdcall -arch=i386 HalHandleNMI(ptr)
@ stdcall HalInitSystem(long ptr)
@ stdcall HalInitializeProcessor(long ptr)
@ stdcall HalMakeBeep(long)
@ stdcall HalProcessorIdle()
@ stdcall HalPutDmaAdapter(ptr)
@ stdcall HalQueryDisplayParameters(ptr ptr ptr ptr)
@ stdcall HalQueryRealTimeClock(ptr)
@ stdcall HalReadDmaCounter(ptr)
@ stdcall HalReportResourceUsage()
@ stdcall HalRequestIpi(long)
@ stdcall HalReturnToFirmware(long)
@ stdcall HalSetBusData(long long long ptr long)
@ stdcall HalSetBusDataByOffset(long long long ptr long long)
@ stdcall HalSetDisplayParameters(long long)
@ stdcall HalSetEnvironmentVariable(str str)
@ stdcall HalSetProfileInterval(long)
@ stdcall HalSetRealTimeClock(ptr)
@ stdcall HalSetTimeIncrement(long)
@ stdcall HalStartNextProcessor(ptr ptr)
@ stdcall HalStartProfileInterrupt(long)
@ stdcall HalStopProfileInterrupt(long)
@ fastcall -arch=arm HalSweepIcache()
@ fastcall -arch=arm HalSweepDcache()
@ stdcall HalTranslateBusAddress(long long long long ptr ptr)
@ stdcall IoFlushAdapterBuffers(ptr ptr ptr ptr long long)
@ stdcall IoFreeAdapterChannel(ptr)
@ stdcall IoFreeMapRegisters(ptr ptr long)
@ stdcall IoMapTransfer(ptr ptr ptr ptr ptr long)
@ stdcall -arch=i386,x86_64 IoAssignDriveLetters(ptr str ptr ptr) HalpAssignDriveLetters
@ stdcall -arch=i386,x86_64 IoReadPartitionTable(ptr long long ptr) HalpReadPartitionTable
@ stdcall -arch=i386,x86_64 IoSetPartitionInformation(ptr long long long) HalpSetPartitionInformation
@ stdcall -arch=i386,x86_64 IoWritePartitionTable(ptr long long long ptr) HalpWritePartitionTable
@ stdcall -arch=i386,arm KeAcquireSpinLock(ptr ptr)
@ extern KdComPortInUse
@ stdcall KeFlushWriteBuffer()
@ stdcall -arch=i386,arm KeGetCurrentIrql()
@ stdcall -arch=i386,arm KeLowerIrql(long)
@ stdcall KeQueryPerformanceCounter(ptr)
@ stdcall -arch=i386,arm KeRaiseIrql(long ptr)
@ stdcall -arch=i386,arm KeRaiseIrqlToDpcLevel()
@ stdcall -arch=i386,arm KeRaiseIrqlToSynchLevel()
@ stdcall -arch=i386,arm KeReleaseSpinLock(ptr long)
@ stdcall KeStallExecutionProcessor(long)
@ stdcall -arch=i386,arm READ_PORT_BUFFER_UCHAR(ptr ptr long)
@ stdcall -arch=i386,arm READ_PORT_BUFFER_ULONG(ptr ptr long)
@ stdcall -arch=i386,arm READ_PORT_BUFFER_USHORT(ptr ptr long)
@ stdcall -arch=i386,arm READ_PORT_UCHAR(ptr)
@ stdcall -arch=i386,arm READ_PORT_ULONG(ptr)
@ stdcall -arch=i386,arm READ_PORT_USHORT(ptr)
@ stdcall -arch=i386,arm WRITE_PORT_BUFFER_UCHAR(ptr ptr long)
@ stdcall -arch=i386,arm WRITE_PORT_BUFFER_ULONG(ptr ptr long)
@ stdcall -arch=i386,arm WRITE_PORT_BUFFER_USHORT(ptr ptr long)
@ stdcall -arch=i386,arm WRITE_PORT_UCHAR(ptr long)
@ stdcall -arch=i386,arm WRITE_PORT_ULONG(ptr long)
@ stdcall -arch=i386,arm WRITE_PORT_USHORT(ptr long)
@ stdcall -arch=x86_64 HalInitializeBios(long ptr)

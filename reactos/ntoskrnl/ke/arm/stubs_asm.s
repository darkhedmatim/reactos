#include <internal/arm/asmmacro.S>

//
// Exceptions
//
GENERATE_ARM_STUB _abnormal_termination 
GENERATE_ARM_STUB _except_handler2 
GENERATE_ARM_STUB _except_handler3
GENERATE_ARM_STUB _global_unwind2  
GENERATE_ARM_STUB _local_unwind2  
GENERATE_ARM_STUB RtlGetCallersAddress 
GENERATE_ARM_STUB RtlUnwind 
GENERATE_ARM_STUB RtlpGetExceptionAddress
GENERATE_ARM_STUB RtlDispatchException
GENERATE_ARM_STUB RtlpGetStackLimits
GENERATE_ARM_STUB DbgBreakPointWithStatus 
GENERATE_ARM_STUB KeRaiseUserException 
GENERATE_ARM_STUB KdpGdbStubInit
GENERATE_ARM_STUB NtRaiseException

//
// Driver ISRs
//
GENERATE_ARM_STUB KeConnectInterrupt 
GENERATE_ARM_STUB KeDisconnectInterrupt 
GENERATE_ARM_STUB KiPassiveRelease 
GENERATE_ARM_STUB KiInterruptTemplate 
GENERATE_ARM_STUB KiUnexpectedInterrupt  
GENERATE_ARM_STUB KeInitializeInterrupt 
GENERATE_ARM_STUB KeSynchronizeExecution 

//
// User Mode Support
//
GENERATE_ARM_STUB KiInitializeUserApc
GENERATE_ARM_STUB KiSwapProcess
GENERATE_ARM_STUB KeSwitchKernelStack
GENERATE_ARM_STUB RtlCreateUserThread
GENERATE_ARM_STUB RtlInitializeContext
GENERATE_ARM_STUB KeUserModeCallback 
GENERATE_ARM_STUB NtCallbackReturn
GENERATE_ARM_STUB NtContinue

//
// Non-ARM Functionality
//
GENERATE_ARM_STUB NtSetLdtEntries
GENERATE_ARM_STUB NtVdmControl 

//
// Ke Arch-Specific Helpers
//
GENERATE_ARM_STUB KeDisableInterrupts
GENERATE_ARM_STUB KeFlushEntireTb 
GENERATE_ARM_STUB KeGetRecommendedSharedDataAlignment 
GENERATE_ARM_STUB KeIcacheFlushCount 
GENERATE_ARM_STUB KeInvalidateAllCaches
GENERATE_ARM_STUB KeQueryActiveProcessors 
GENERATE_ARM_STUB KeSaveStateForHibernate 
GENERATE_ARM_STUB KeSetDmaIoCoherency 

#include <ddk/ntddk.h>

#define STUB(x) void x(void) { \
				UNICODE_STRING UnicodeString; \
                                RtlInitUnicodeString(&UnicodeString,\
                                L"NTDLL: Stub for "#x"\n"); \
                                NtDisplayString(&UnicodeString); }


// ?Allocate@CBufferAllocator@@UAEPAXK@Z
STUB(PropertyLengthAsVariant)
STUB(RtlCompareVariants)
STUB(RtlConvertPropertyToVariant)
STUB(RtlConvertVariantToProperty)
STUB(CsrAllocateCaptureBuffer)
STUB(CsrAllocateCapturePointer)
STUB(CsrAllocateMessagePointer)
STUB(CsrCaptureMessageBuffer)
STUB(CsrCaptureMessageString)
STUB(CsrCaptureTimeout)
STUB(CsrClientCallServer)
STUB(CsrClientConnectToServer)
STUB(CsrFreeCaptureBuffer)
STUB(CsrIdentifyAlertableThread)
STUB(CsrNewThread)
STUB(CsrProbeForRead)
STUB(CsrProbeForWrite)
STUB(CsrSetPriorityClass)
STUB(DbgBreakPoint)
STUB(DbgPrompt)
STUB(DbgSsHandleKmApiMsg)
STUB(DbgSsInitialize)
STUB(DbgUiConnectToDbg)
STUB(DbgUiContinue)
STUB(DbgUiWaitStateChange)
STUB(DbgUserBreakPoint)
STUB(KiRaiseUserExceptionDispatcher)
STUB(KiUserApcDispatcher)
STUB(KiUserCallbackDispatcher)
STUB(KiUserExceptionDispatcher)

STUB(LdrDisableThreadCalloutsForDll)
STUB(LdrEnumResources)
STUB(LdrFindEntryForAddress)
STUB(LdrFindResourceDirectory_U)

STUB(LdrGetDllHandle)
STUB(LdrGetProcedureAddress)
STUB(LdrInitializeThunk)
STUB(LdrProcessRelocationBlock)
STUB(LdrQueryImageFileExecutionOptions)
STUB(LdrQueryProcessModuleInformation)
STUB(LdrShutdownProcess)
STUB(LdrShutdownThread)
STUB(LdrVerifyImageMatchesChecksum)
STUB(NPXEMULATORTABLE)
STUB(PfxFindPrefix)
STUB(PfxInitialize)
STUB(PfxInsertPrefix)
STUB(PfxRemovePrefix)
STUB(RestoreEm87Context)
STUB(RtlValidateProcessHeaps)
STUB(RtlWalkHeap)
STUB(RtlZeroHeap)
STUB(RtlpNtCreateKey)
STUB(RtlpNtEnumerateSubKey)
STUB(RtlpNtMakeTemporaryKey)
STUB(RtlpNtOpenKey)
STUB(RtlpNtQueryValueKey)
STUB(RtlpNtSetValueKey)
STUB(RtlpUnWaitCriticalSection)
STUB(RtlpWaitForCriticalSection)
STUB(RtlxAnsiStringToUnicodeSize)
STUB(RtlxOemStringToUnicodeSize)
STUB(RtlxUnicodeStringToAnsiSize)
STUB(RtlxUnicodeStringToOemSize)
STUB(SaveEm87Context)
STUB(_CIpow)
STUB(__eCommonExceptions)
STUB(__eEmulatorInit)
STUB(__eF2XM1)
STUB(__eFABS)
STUB(__eFADD32)
STUB(__eFADD64)
STUB(__eFADDPreg)
STUB(__eFADDreg)
STUB(__eFADDtop)
STUB(__eFCHS)
STUB(__eFCOM)
STUB(__eFCOM32)
STUB(__eFCOM64)
STUB(__eFCOMP)
STUB(__eFCOMP32)
STUB(__eFCOMP64)
STUB(__eFCOMPP)
STUB(__eFCOS)
STUB(__eFDECSTP)
STUB(__eFDIV32)
STUB(__eFDIV64)
STUB(__eFDIVPreg)
STUB(__eFDIVR32)
STUB(__eFDIVR64)
STUB(__eFDIVRPreg)
STUB(__eFDIVRreg)
STUB(__eFDIVRtop)
STUB(__eFDIVreg)
STUB(__eFDIVtop)
STUB(__eFFREE)
STUB(__eFIADD16)
STUB(__eFIADD32)
STUB(__eFICOM16)
STUB(__eFICOM32)
STUB(__eFICOMP16)
STUB(__eFICOMP32)
STUB(__eFIDIV16)
STUB(__eFIDIV32)
STUB(__eFIDIVR16)
STUB(__eFIDIVR32)
STUB(__eFILD16)
STUB(__eFILD32)
STUB(__eFILD64)
STUB(__eFIMUL16)
STUB(__eFIMUL32)
STUB(__eFINCSTP)
STUB(__eFINIT)
STUB(__eFIST16)
STUB(__eFIST32)
STUB(__eFISTP16)
STUB(__eFISTP32)
STUB(__eFISTP64)
STUB(__eFISUB16)
STUB(__eFISUB32)
STUB(__eFISUBR16)
STUB(__eFISUBR32)
STUB(__eFLD1)
STUB(__eFLD32)
STUB(__eFLD64)
STUB(__eFLD80)
STUB(__eFLDCW)
STUB(__eFLDENV)
STUB(__eFLDL2E)
STUB(__eFLDLN2)
STUB(__eFLDPI)
STUB(__eFLDZ)
STUB(__eFMUL32)
STUB(__eFMUL64)
STUB(__eFMULPreg)
STUB(__eFMULreg)
STUB(__eFMULtop)
STUB(__eFPATAN)
STUB(__eFPREM)
STUB(__eFPREM1)
STUB(__eFPTAN)
STUB(__eFRNDINT)
STUB(__eFRSTOR)
STUB(__eFSAVE)
STUB(__eFSCALE)
STUB(__eFSIN)
STUB(__eFSQRT)
STUB(__eFST)
STUB(__eFST32)
STUB(__eFST64)
STUB(__eFSTCW)
STUB(__eFSTENV)
STUB(__eFSTP)
STUB(__eFSTP32)
STUB(__eFSTP64)
STUB(__eFSTP80)
STUB(__eFSTSW)
STUB(__eFSUB32)
STUB(__eFSUB64)
STUB(__eFSUBPreg)
STUB(__eFSUBR32)
STUB(__eFSUBR64)
STUB(__eFSUBRPreg)
STUB(__eFSUBRreg)
STUB(__eFSUBRtop)
STUB(__eFSUBreg)
STUB(__eFSUBtop)
STUB(__eFTST)
STUB(__eFUCOM)
STUB(__eFUCOMP)
STUB(__eFUCOMPP)
STUB(__eFXAM)
STUB(__eFXCH)
STUB(__eFXTRACT)
STUB(__eFYL2X)
STUB(__eFYL2XP1)
STUB(__eGetStatusWord)
STUB(__isascii)
STUB(__iscsym)
STUB(__iscsymf)
STUB(__toascii)
STUB(_alldiv)
STUB(_allmul)
STUB(_alloca_probe)
STUB(_allrem)
STUB(_allshl)
STUB(_allshr)
STUB(_atoi64)
STUB(_aulldiv)
STUB(_aullrem)
STUB(_aullshr)
//STUB(_chkstk)
STUB(_fltused)
STUB(_ftol)
STUB(_i64toa)
STUB(_i64tow)
STUB(_itoa)
STUB(_itow)
STUB(_ltoa)
STUB(_ltow)
STUB(_snprintf)
STUB(_snwprintf)
STUB(_ultoa)
STUB(_ultow)
STUB(_vsnprintf)
STUB(_wtoi)
STUB(_wtoi64)
STUB(_wtol)
STUB(abs)
STUB(atan)
STUB(ceil)
STUB(cos)
STUB(fabs)
STUB(floor)
STUB(iswalpha)
STUB(iswctype)
STUB(labs)
STUB(log)
STUB(mbstowcs)
STUB(pow)
STUB(qsort)
STUB(sin)
STUB(sqrt)
STUB(sscanf)
STUB(swprintf)
STUB(tan)


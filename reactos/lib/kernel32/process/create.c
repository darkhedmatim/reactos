/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/process/create.c
 * PURPOSE:         Process functions
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 *                  Ariadne ( ariadne@xs4all.nl)
 */

/* INCLUDES ****************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

#define CMD_STRING L"cmd /c "

extern __declspec(noreturn)
VOID
CALLBACK 
ConsoleControlDispatcher(DWORD CodeAndFlag);

/* INTERNAL FUNCTIONS *******************************************************/

_SEH_FILTER(BaseExceptionFilter)
{
    EXCEPTION_POINTERS *ExceptionInfo = _SEH_GetExceptionPointers();
    LONG ExceptionDisposition = EXCEPTION_EXECUTE_HANDLER;

    if (GlobalTopLevelExceptionFilter != NULL)
    {
        _SEH_TRY
        {
            ExceptionDisposition = GlobalTopLevelExceptionFilter(ExceptionInfo);
        }
        _SEH_HANDLE
        {
            ExceptionDisposition = UnhandledExceptionFilter(ExceptionInfo);
        }
        _SEH_END;
    }

    return ExceptionDisposition;
}

VOID
STDCALL
BaseProcessStartup(PPROCESS_START_ROUTINE lpStartAddress)
{
    UINT uExitCode = 0;

    DPRINT("BaseProcessStartup(..) - setting up exception frame.\n");

    _SEH_TRY
    {
        /* Set our Start Address */
        NtSetInformationThread(NtCurrentThread(),
                               ThreadQuerySetWin32StartAddress,
                               &lpStartAddress,
                               sizeof(PPROCESS_START_ROUTINE));
        
        /* Call the Start Routine */
        uExitCode = (lpStartAddress)();
    }
    _SEH_EXCEPT(BaseExceptionFilter)
    {
        /* Get the SEH Error */
        uExitCode = _SEH_GetExceptionCode();
    }
    _SEH_END;

    /* Exit the Process with our error */
    ExitProcess(uExitCode);
}

/*
 * Tells CSR that a new process was created
 */
NTSTATUS
STDCALL
BasepNotifyCsrOfCreation(ULONG dwCreationFlags,
                         IN HANDLE ProcessId,
                         IN ULONG SubsystemType,
                         OUT PHANDLE ConsoleHandle,
                         OUT PHANDLE InputHandle,
                         OUT PHANDLE OutputHandle)
{
    ULONG Request = CREATE_PROCESS;
    CSR_API_MESSAGE CsrRequest;
    NTSTATUS Status;
    
    DPRINT("BasepNotifyCsrOfCreation\n");
     
    /* Some hacks (heck, this whole API is a hack) */
    if (SubsystemType == IMAGE_SUBSYSTEM_WINDOWS_GUI)
    {
        dwCreationFlags = (dwCreationFlags &~ CREATE_NEW_CONSOLE) |
                           DETACHED_PROCESS;
    }
    else if (SubsystemType == IMAGE_SUBSYSTEM_WINDOWS_CUI)
    {
        dwCreationFlags |= CREATE_NEW_CONSOLE;
    }
    
    /* Fill out the request */
    CsrRequest.Data.CreateProcessRequest.NewProcessId = ProcessId;
    CsrRequest.Data.CreateProcessRequest.Flags = dwCreationFlags;
    CsrRequest.Data.CreateProcessRequest.CtrlDispatcher = ConsoleControlDispatcher;
    CsrRequest.Data.CreateProcessRequest.InputHandle = 0;
    CsrRequest.Data.CreateProcessRequest.OutputHandle = 0;
    CsrRequest.Data.CreateProcessRequest.Console = 0;
    
    /* Call CSR */
    Status = CsrClientCallServer(&CsrRequest,
                                 NULL,
                                 MAKE_CSR_API(Request, CSR_NATIVE),
                                 sizeof(CSR_API_MESSAGE));
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(CsrRequest.Status))
    {
        DPRINT1("Failed to tell csrss about new process\n");
        return CsrRequest.Status;
    }
    /* Return Handles */
    *ConsoleHandle = CsrRequest.Data.CreateProcessRequest.Console;
    *InputHandle = CsrRequest.Data.CreateProcessRequest.InputHandle;
    *OutputHandle = CsrRequest.Data.CreateProcessRequest.OutputHandle;
    DPRINT("CSR Created: %lx %lx\n", *InputHandle, *OutputHandle);
    
    /* REturn Success */
    return STATUS_SUCCESS;
}

/*
 * Creates the first Thread in a Proces
 */
HANDLE
STDCALL
BasepCreateFirstThread(HANDLE ProcessHandle,
                       LPSECURITY_ATTRIBUTES lpThreadAttributes,
                       PSECTION_IMAGE_INFORMATION SectionImageInfo,
                       PCLIENT_ID ClientId)
{
    OBJECT_ATTRIBUTES LocalObjectAttributes;
    POBJECT_ATTRIBUTES ObjectAttributes;
    CONTEXT Context;
    INITIAL_TEB InitialTeb;
    NTSTATUS Status;
    HANDLE hThread;
    
    DPRINT("BasepCreateFirstThread. hProcess: %lx\n", ProcessHandle);

    /* Create the Thread's Stack */
    BasepCreateStack(ProcessHandle,
                     SectionImageInfo->MaximumStackSize,
                     SectionImageInfo->CommittedStackSize,
                     &InitialTeb);
                     
    /* Create the Thread's Context */
    BasepInitializeContext(&Context,
                           NtCurrentPeb(),
                           SectionImageInfo->TransferAddress,
                           InitialTeb.StackBase,
                           0);
    
    /* Convert the thread attributes */
    ObjectAttributes = BasepConvertObjectAttributes(&LocalObjectAttributes,
                                                    lpThreadAttributes,
                                                    NULL);
    
    /* Create the Kernel Thread Object */
    Status = NtCreateThread(&hThread,
                            THREAD_ALL_ACCESS,
                            ObjectAttributes,
                            ProcessHandle,
                            ClientId,
                            &Context,
                            &InitialTeb,
                            TRUE);
   
    /* Success */
    return hThread;
}

/*
 * Converts ANSI to Unicode Environment
 */
PVOID
STDCALL
BasepConvertUnicodeEnvironment(IN PVOID lpEnvironment)
{
    PCHAR pcScan;
    SIZE_T EnvSize = 0;
    ANSI_STRING AnsiEnv;
    UNICODE_STRING UnicodeEnv;
    NTSTATUS Status;
    
    DPRINT("BasepConvertUnicodeEnvironment\n");

    /* Scan the environment to calculate its Unicode size */
    AnsiEnv.Buffer = pcScan = lpEnvironment;
    while (*pcScan) while (*pcScan++);

    /* Create our ANSI String */
    AnsiEnv.Length = (ULONG_PTR)pcScan - (ULONG_PTR)lpEnvironment + 1;
    AnsiEnv.MaximumLength = AnsiEnv.Length + 1;
    
    /* Allocate memory for the Unicode Environment */
    UnicodeEnv.Buffer = NULL;
    EnvSize = AnsiEnv.MaximumLength * sizeof(WCHAR);
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     (PVOID)&UnicodeEnv.Buffer,
                                     0,
                                     &EnvSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    /* Failure */
    if (!NT_SUCCESS(Status))
    {
        SetLastError(Status);
        return NULL;
    }
        
    /* Use the allocated size */
    UnicodeEnv.MaximumLength = EnvSize;
    
    /* Convert */
    RtlAnsiStringToUnicodeString(&UnicodeEnv, &AnsiEnv, FALSE);
    return UnicodeEnv.Buffer;
}

/*
 * Converts a Win32 Priority Class to NT
 */
ULONG
STDCALL
BasepConvertPriorityClass(IN ULONG dwCreationFlags)
{
    ULONG ReturnClass;
    
    if(dwCreationFlags & IDLE_PRIORITY_CLASS)
    {    
        ReturnClass = PROCESS_PRIORITY_CLASS_IDLE;
    }
    else if(dwCreationFlags & BELOW_NORMAL_PRIORITY_CLASS)
    {
        ReturnClass = PROCESS_PRIORITY_CLASS_BELOW_NORMAL;
    }
    else if(dwCreationFlags & NORMAL_PRIORITY_CLASS)
    {
        ReturnClass = PROCESS_PRIORITY_CLASS_NORMAL;
    }
    else if(dwCreationFlags & ABOVE_NORMAL_PRIORITY_CLASS)
    {
        ReturnClass = PROCESS_PRIORITY_CLASS_ABOVE_NORMAL;
    }
    else if(dwCreationFlags & HIGH_PRIORITY_CLASS)
    {
        ReturnClass = PROCESS_PRIORITY_CLASS_HIGH;
    }
    else if(dwCreationFlags & REALTIME_PRIORITY_CLASS)
    {
        /* Check for Privilege First */
        if (BasepCheckRealTimePrivilege())
        {
            ReturnClass = PROCESS_PRIORITY_CLASS_REALTIME;
        }
        else
        {
            ReturnClass = PROCESS_PRIORITY_CLASS_HIGH;
        }
    }
    else
    {
        ReturnClass = 0 /* FIXME */;
    }
    
    return ReturnClass;
}

/*
 * Duplicates a standard handle and writes it where requested.
 */
VOID
STDCALL
BasepDuplicateAndWriteHandle(IN HANDLE ProcessHandle,
                             IN HANDLE StandardHandle,
                             IN PHANDLE Address)
{
    NTSTATUS Status;
    HANDLE DuplicatedHandle;
    ULONG Dummy;
    
    DPRINT("BasepDuplicateAndWriteHandle. hProcess: %lx, Handle: %lx,"
           "Address: %p\n", ProcessHandle, StandardHandle, Address);
            
    /* Don't touch Console Handles */
    if (IsConsoleHandle(StandardHandle)) return;
    
    /* Duplicate the handle */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               StandardHandle,
                               ProcessHandle,
                               &DuplicatedHandle,
                               DUPLICATE_SAME_ACCESS | DUPLICATE_SAME_ATTRIBUTES,
                               0,
                               0);
    if (NT_SUCCESS(Status))
    {
        /* Write it */
        NtWriteVirtualMemory(ProcessHandle,
                             Address,
                             &DuplicatedHandle,
                             sizeof(HANDLE),
                             &Dummy);
    }
}

LPWSTR
STDCALL
BasepGetDllPath(LPWSTR FullPath,
                PVOID Environment)
{
    /* FIXME: Not yet implemented */
    return NULL;
}

VOID
STDCALL
BasepCopyHandles(IN PRTL_USER_PROCESS_PARAMETERS Params,
                 IN PRTL_USER_PROCESS_PARAMETERS PebParams,
                 IN BOOL InheritHandles)
{
    /* Copy the handle if we are inheriting or if it's a console handle */
    if (InheritHandles || IsConsoleHandle(PebParams->StandardInput))
    {
        Params->StandardInput = PebParams->StandardInput;
    }
    if (InheritHandles || IsConsoleHandle(PebParams->StandardOutput))
    {
        Params->StandardOutput = PebParams->StandardOutput;
    }
    if (InheritHandles || IsConsoleHandle(PebParams->StandardError))
    {
        Params->StandardError = PebParams->StandardError;
    }
}

NTSTATUS
STDCALL
BasepInitializeEnvironment(HANDLE ProcessHandle,
                           PPEB Peb,
                           LPWSTR ApplicationPathName,
                           LPWSTR lpCurrentDirectory,
                           LPWSTR lpCommandLine,
                           LPVOID Environment,
                           LPSTARTUPINFOW StartupInfo,
                           DWORD CreationFlags,
                           BOOL InheritHandles,
                           HANDLE hInput,
                           HANDLE hOutput)
{
    WCHAR FullPath[MAX_PATH];
    LPWSTR Remaining;
    LPWSTR DllPathString;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    PRTL_USER_PROCESS_PARAMETERS RemoteParameters = NULL;
    UNICODE_STRING DllPath, ImageName, CommandLine, CurrentDirectory;
    UINT RetVal;
    NTSTATUS Status;
    PWCHAR ScanChar;
    ULONG EnviroSize;
    ULONG Size;
    UNICODE_STRING Desktop, Shell, Runtime, Title;
    PPEB OurPeb = NtCurrentPeb();
    
    DPRINT("BasepInitializeEnvironment\n");
    
    /* Get the full path name */
    RetVal = GetFullPathNameW(ApplicationPathName,
                              MAX_PATH,
                              FullPath,
                              &Remaining);
    DPRINT("ApplicationPathName: %S, FullPath: %S\n", ApplicationPathName, 
            FullPath);
                                  
    /* Get the DLL Path */
    DllPathString = BasepGetDllPath(FullPath, Environment);
    
    /* Initialize Strings */
    RtlInitUnicodeString(&DllPath, DllPathString);
    RtlInitUnicodeString(&ImageName, FullPath);
    RtlInitUnicodeString(&CommandLine, lpCommandLine);
    RtlInitUnicodeString(&CurrentDirectory, lpCurrentDirectory);
   
    /* Initialize more Strings from the Startup Info */
    if (StartupInfo->lpDesktop)
    {
        RtlInitUnicodeString(&Desktop, StartupInfo->lpDesktop);
    }
    else
    {
        RtlInitUnicodeString(&Desktop, L"");
    }
    if (StartupInfo->lpReserved)
    {
        RtlInitUnicodeString(&Shell, StartupInfo->lpReserved);
    }
    else
    {
        RtlInitUnicodeString(&Shell, L"");
    }
    if (StartupInfo->lpTitle)
    {
        RtlInitUnicodeString(&Title, StartupInfo->lpTitle);
    }
    else
    {
        RtlInitUnicodeString(&Title, L"");
    }
    
    /* This one is special because the length can differ */
    Runtime.Buffer = (LPWSTR)StartupInfo->lpReserved2;
    Runtime.MaximumLength = Runtime.Length = StartupInfo->cbReserved2;
    
    /* Create the Parameter Block */
    DPRINT("Creating Process Parameters: %wZ %wZ %wZ %wZ %wZ %wZ %wZ\n",
            &ImageName, &DllPath, &CommandLine, &Desktop, &Title, &Shell,
            &Runtime);
    Status = RtlCreateProcessParameters(&ProcessParameters,
                                        &ImageName,
                                        &DllPath,
                                        lpCurrentDirectory ? 
                                        &CurrentDirectory : NULL,
                                        &CommandLine,
                                        Environment,
                                        &Title,
                                        &Desktop,
                                        &Shell,
                                        &Runtime);
                                        
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create process parameters!\n");
        return Status;
    }
    
    /* Check if we got an environment. If not, use ours */
    if (Environment)
    {
        /* Save pointer and start lookup */
        Environment = ScanChar = ProcessParameters->Environment;
    }
    else
    {
        /* Save pointer and start lookup */
        Environment = ScanChar = OurPeb->ProcessParameters->Environment;
    }
    
    /* Find the environment size */
    if (ScanChar)
    {
        while (*ScanChar) while (*ScanChar++);
        
        /* Calculate the size of the block */
        EnviroSize = (ULONG)((ULONG_PTR)ScanChar - (ULONG_PTR)Environment);
        DPRINT("EnvironmentSize %ld\n", EnviroSize);

        /* Allocate and Initialize new Environment Block */
        Size = EnviroSize;
        ProcessParameters->Environment = NULL;
        Status = ZwAllocateVirtualMemory(ProcessHandle,
                                         (PVOID*)&ProcessParameters->Environment,
                                         0,
                                         &Size,
                                         MEM_COMMIT,
                                         PAGE_READWRITE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to allocate Environment Block\n");
            return(Status);
        }
        
        /* Write the Environment Block */
        ZwWriteVirtualMemory(ProcessHandle,
                             ProcessParameters->Environment,
                             Environment,
                             EnviroSize,
                             NULL);
    }
    
    /* Write new parameters */
    ProcessParameters->StartingX = StartupInfo->dwX;
    ProcessParameters->StartingY = StartupInfo->dwY;
    ProcessParameters->CountX = StartupInfo->dwXSize;
    ProcessParameters->CountY = StartupInfo->dwYSize;
    ProcessParameters->CountCharsX = StartupInfo->dwXCountChars;
    ProcessParameters->CountCharsY = StartupInfo->dwYCountChars;
    ProcessParameters->FillAttribute = StartupInfo->dwFillAttribute;
    ProcessParameters->WindowFlags = StartupInfo->dwFlags;
    ProcessParameters->ShowWindowFlags = StartupInfo->wShowWindow;
        
    /* Write the handles only if we have to */
    if (StartupInfo->dwFlags & STARTF_USESTDHANDLES)
    {
        ProcessParameters->StandardInput = StartupInfo->hStdInput;
        ProcessParameters->StandardOutput = StartupInfo->hStdOutput;
        ProcessParameters->StandardError = StartupInfo->hStdError;
    }
        
    /* Use Special Flags for ConDllInitialize in Kernel32 */
    if (CreationFlags & DETACHED_PROCESS)
    {
        ProcessParameters->ConsoleHandle = HANDLE_DETACHED_PROCESS;
    }
    else if (CreationFlags & CREATE_NO_WINDOW)
    {
        ProcessParameters->ConsoleHandle = HANDLE_CREATE_NO_WINDOW;
    }
    else if (CreationFlags & CREATE_NEW_CONSOLE)
    {
        ProcessParameters->ConsoleHandle = HANDLE_CREATE_NEW_CONSOLE;
    }
    else
    {
        /* Inherit our Console Handle */
        ProcessParameters->ConsoleHandle = OurPeb->ProcessParameters->ConsoleHandle;
        
        /* Is the shell trampling on our Handles? */
        if (!(StartupInfo->dwFlags & 
              (STARTF_USESTDHANDLES | STARTF_USEHOTKEY | STARTF_SHELLPRIVATE)))
        {
            /* Use handles from PEB, if inheriting or they are console */ 
            BasepCopyHandles(ProcessParameters,
                             OurPeb->ProcessParameters,
                             InheritHandles);
       }
    }
    
    /* Also set the Console Flag */
    if (CreationFlags & CREATE_NEW_PROCESS_GROUP)
    {
        ProcessParameters->ConsoleFlags = 1;
    }
    
    /* Allocate memory for the parameter block */
    Size = ProcessParameters->Length;
    Status = NtAllocateVirtualMemory(ProcessHandle,
                                     (PVOID*)&RemoteParameters,
                                     0,
                                     &Size,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to allocate Parameters Block\n");
        return(Status);
    }
    
    /* Set the allocated size */
    ProcessParameters->MaximumLength = Size;
    
    /* Handle some Parameter Flags */
    ProcessParameters->ConsoleFlags = (CreationFlags & CREATE_NEW_PROCESS_GROUP);
    ProcessParameters->Flags |= (CreationFlags & PROFILE_USER) ?
                                 PPF_PROFILE_USER : 0;
    ProcessParameters->Flags |= (CreationFlags & PROFILE_KERNEL) ? 
                                 PPF_PROFILE_KERNEL : 0;    
    ProcessParameters->Flags |= (CreationFlags & PROFILE_SERVER) ?
                                 PPF_PROFILE_SERVER : 0;
    ProcessParameters->Flags |= (NtCurrentPeb()->ProcessParameters->Flags &
                                 PPF_DISABLE_HEAP_CHECKS);
                                 
    /* 
     * FIXME: Our console init stuff is messy. See my comment in kernel32's
     * DllMain.
     */
    if (!ProcessParameters->StandardInput) ProcessParameters->StandardInput = hInput;
    if (!ProcessParameters->StandardOutput) ProcessParameters->StandardOutput = hOutput;
    if (!ProcessParameters->StandardError) ProcessParameters->StandardError = hOutput;
    
    /* Write the Parameter Block */
    Status = NtWriteVirtualMemory(ProcessHandle,
                                  RemoteParameters,
                                  ProcessParameters,
                                  ProcessParameters->Length,
                                  NULL);
                                  
    /* Write the PEB Pointer */
    Status = NtWriteVirtualMemory(ProcessHandle,
                                  &Peb->ProcessParameters,
                                  &RemoteParameters,
                                  sizeof(PVOID),
                                  NULL);
                                  
    /* Cleanup */
    RtlFreeHeap(GetProcessHeap(), 0, DllPath.Buffer);
    RtlDestroyProcessParameters(ProcessParameters);

    DPRINT("Completed\n");
    return STATUS_SUCCESS;
}

/* FUNCTIONS ****************************************************************/

/*
 * FUNCTION: The CreateProcess function creates a new process and its
 * primary thread. The new process executes the specified executable file
 * ARGUMENTS:
 *
 *     lpApplicationName = Pointer to name of executable module
 *     lpCommandLine = Pointer to command line string
 *     lpProcessAttributes = Process security attributes
 *     lpThreadAttributes = Thread security attributes
 *     bInheritHandles = Handle inheritance flag
 *     dwCreationFlags = Creation flags
 *     lpEnvironment = Pointer to new environment block
 *     lpCurrentDirectory = Pointer to current directory name
 *     lpStartupInfo = Pointer to startup info
 *     lpProcessInformation = Pointer to process information
 *
 * @implemented
 */
BOOL
STDCALL
CreateProcessA(LPCSTR lpApplicationName,
               LPSTR lpCommandLine,
               LPSECURITY_ATTRIBUTES lpProcessAttributes,
               LPSECURITY_ATTRIBUTES lpThreadAttributes,
               BOOL bInheritHandles,
               DWORD dwCreationFlags,
               LPVOID lpEnvironment,
               LPCSTR lpCurrentDirectory,
               LPSTARTUPINFOA lpStartupInfo,
               LPPROCESS_INFORMATION lpProcessInformation)
{    
    PUNICODE_STRING CommandLine = NULL;
    UNICODE_STRING DummyString;
    UNICODE_STRING LiveCommandLine;
    UNICODE_STRING ApplicationName;
    UNICODE_STRING CurrentDirectory;
    BOOL bRetVal;
    STARTUPINFOW StartupInfo;

    DPRINT("dwCreationFlags %x, lpEnvironment %x, lpCurrentDirectory %x, "
            "lpStartupInfo %x, lpProcessInformation %x\n",
            dwCreationFlags, lpEnvironment, lpCurrentDirectory,
            lpStartupInfo, lpProcessInformation);
    
    /* Copy Startup Info */
    DPRINT("Foo\n");
    RtlMoveMemory(&StartupInfo, lpStartupInfo, sizeof(*lpStartupInfo));
    DPRINT("Foo\n");
    
    /* Initialize all strings to nothing */
    LiveCommandLine.Buffer = NULL;
    DummyString.Buffer = NULL;
    ApplicationName.Buffer = NULL;
    CurrentDirectory.Buffer = NULL;
    StartupInfo.lpDesktop = NULL;
    StartupInfo.lpReserved = NULL;
    StartupInfo.lpTitle = NULL;
    
    /* Convert the Command line */
    if (lpCommandLine)
    {
        DPRINT("Foo\n");
        /* If it's too long, then we'll have a problem */
        if ((strlen(lpCommandLine) + 1) * sizeof(WCHAR) <
            NtCurrentTeb()->StaticUnicodeString.MaximumLength)
        {
            /* Cache it in the TEB */
            DPRINT("Foo\n");
            CommandLine = Basep8BitStringToCachedUnicodeString(lpCommandLine);
        }
        else
        {
            /* Use a dynamic version */
            DPRINT("Foo\n");
            Basep8BitStringToLiveUnicodeString(&LiveCommandLine, 
                                               lpCommandLine);
        }
    }
    else
    {
        DPRINT("Foo\n");
        /* The logic below will use CommandLine, so we must make it valid */
        CommandLine = &DummyString;
    }
    
    /* Convert the Name and Directory */
    if (lpApplicationName)
    {
        DPRINT("Foo\n");
        Basep8BitStringToLiveUnicodeString(&ApplicationName, 
                                           lpApplicationName);
    }
    if (lpCurrentDirectory)
    {
        DPRINT("Foo\n");
        Basep8BitStringToLiveUnicodeString(&CurrentDirectory, 
                                           lpCurrentDirectory);
    }
    
    /* Now convert Startup Strings */
    if (lpStartupInfo->lpReserved)
    {
        DPRINT("Foo\n");
        BasepAnsiStringToHeapUnicodeString(lpStartupInfo->lpReserved,
                                           &StartupInfo.lpReserved);
    }
    if (lpStartupInfo->lpDesktop)
    {
        DPRINT("Foo\n");
        BasepAnsiStringToHeapUnicodeString(lpStartupInfo->lpDesktop,
                                           &StartupInfo.lpDesktop);
    }
    if (lpStartupInfo->lpTitle)
    {
        DPRINT("Foo\n");
        BasepAnsiStringToHeapUnicodeString(lpStartupInfo->lpTitle,
                                           &StartupInfo.lpTitle);
    }

    /* Call the Unicode function */
    bRetVal = CreateProcessW(ApplicationName.Buffer,
                             LiveCommandLine.Buffer ? 
                             LiveCommandLine.Buffer : CommandLine->Buffer,
                             lpProcessAttributes,
                             lpThreadAttributes,
                             bInheritHandles,
                             dwCreationFlags,
                             lpEnvironment,
                             CurrentDirectory.Buffer,
                             &StartupInfo,
                             lpProcessInformation);

    /* Clean up */
    RtlFreeUnicodeString(&ApplicationName);
    RtlFreeUnicodeString(&LiveCommandLine);
    RtlFreeUnicodeString(&CurrentDirectory);
    RtlFreeHeap(GetProcessHeap(), 0, StartupInfo.lpDesktop);
    RtlFreeHeap(GetProcessHeap(), 0, StartupInfo.lpReserved);
    RtlFreeHeap(GetProcessHeap(), 0, StartupInfo.lpTitle);

    /* Return what Unicode did */
    return bRetVal;
}

/*
 * @implemented
 */
BOOL
STDCALL
CreateProcessW(LPCWSTR lpApplicationName,
               LPWSTR lpCommandLine,
               LPSECURITY_ATTRIBUTES lpProcessAttributes,
               LPSECURITY_ATTRIBUTES lpThreadAttributes,
               BOOL bInheritHandles,
               DWORD dwCreationFlags,
               LPVOID lpEnvironment,
               LPCWSTR lpCurrentDirectory,
               LPSTARTUPINFOW lpStartupInfo,
               LPPROCESS_INFORMATION lpProcessInformation)
{
    NTSTATUS Status;
    PROCESS_PRIORITY_CLASS PriorityClass;
    BOOLEAN FoundQuotes = FALSE;
    BOOLEAN QuotesNeeded = FALSE;
    BOOLEAN CmdLineIsAppName = FALSE;
    UNICODE_STRING ApplicationName;
    OBJECT_ATTRIBUTES LocalObjectAttributes;
    POBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hSection, hProcess, hThread;
    SECTION_IMAGE_INFORMATION SectionImageInfo;
    LPWSTR CurrentDirectory = NULL;
    LPWSTR CurrentDirectoryPart;
    PROCESS_BASIC_INFORMATION ProcessBasicInfo;
    STARTUPINFOW StartupInfo;
    ULONG Dummy;
    LPWSTR QuotedCmdLine = NULL;
    LPWSTR ScanString;
    LPWSTR NullBuffer;
    LPWSTR NameBuffer = NULL;
    WCHAR SaveChar;
    ULONG RetVal;
    UINT Error;
    BOOLEAN SearchDone = FALSE;
    HANDLE hConsole, hInput, hOutput;
    CLIENT_ID ClientId;
    PPEB OurPeb = NtCurrentPeb();
    PPEB RemotePeb;
    
    DPRINT("CreateProcessW: lpApplicationName: %S lpCommandLine: %S"
           " lpEnvironment: %p lpCurrentDirectory: %S dwCreationFlags: %lx\n",
           lpApplicationName, lpCommandLine, lpEnvironment, lpCurrentDirectory,
           dwCreationFlags);
    
    /* Flags we don't handle yet */
    if (dwCreationFlags & CREATE_SEPARATE_WOW_VDM)
    {
        DPRINT1("CREATE_SEPARATE_WOW_VDM not handled\n");
    }
    if (dwCreationFlags & CREATE_SHARED_WOW_VDM)
    {
        DPRINT1("CREATE_SHARED_WOW_VDM not handled\n");
    }
    if (dwCreationFlags & CREATE_FORCEDOS)
    {
        DPRINT1("CREATE_FORCEDOS not handled\n");
    }
    
    /* Fail on this flag, it's only valid with the WithLogonW function */
    if (dwCreationFlags & CREATE_WITH_USERPROFILE)
    {
        DPRINT1("Invalid flag used\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    /* This combination is illegal (see MSDN) */
    if ((dwCreationFlags & (DETACHED_PROCESS | CREATE_NEW_CONSOLE)) ==
        (DETACHED_PROCESS | CREATE_NEW_CONSOLE))
    {
        DPRINT1("Invalid flag combo used\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    /* Another illegal combo */
    if ((dwCreationFlags & (CREATE_SEPARATE_WOW_VDM | CREATE_SHARED_WOW_VDM)) ==
        (CREATE_SEPARATE_WOW_VDM | CREATE_SHARED_WOW_VDM))
    {
        DPRINT1("Invalid flag combo used\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    DPRINT("Foo\n");
    /* 
     * We're going to modify and mask out flags and stuff in lpStartupInfo,
     * so we'll use our own local copy for that.
     */
    StartupInfo = *lpStartupInfo;
    DPRINT("Foo\n");    
    /* FIXME: Use default Separate/Shared VDM Flag */
    
    /* If we are inside a Job, use Separate VDM so it won't escape the Job */
    if (!(dwCreationFlags & CREATE_SEPARATE_WOW_VDM))
    {
        if (NtIsProcessInJob(NtCurrentProcess(), NULL))
        {
            /* Remove the shared flag and add the separate flag. */
            dwCreationFlags = (dwCreationFlags &~ CREATE_SHARED_WOW_VDM) | 
                                                  CREATE_SEPARATE_WOW_VDM;
        }
    }
    DPRINT("Foo\n");
    /* 
     * According to some sites, ShellExecuteEx uses an undocumented flag to
     * send private handle data (such as HMONITOR or HICON). See:
     * www.catch22.net/tuts/undoc01.asp. This implies that we can't use the
     * standard handles anymore since we'd be overwriting this private data
     */
    if ((StartupInfo.dwFlags & STARTF_USESTDHANDLES) && 
        (StartupInfo.dwFlags & (STARTF_USEHOTKEY | STARTF_SHELLPRIVATE)))
    {
        StartupInfo.dwFlags &= ~STARTF_USESTDHANDLES;
    }
    DPRINT("Foo\n");
    /* Start by zeroing out the fields */
    RtlZeroMemory(lpProcessInformation, sizeof(PROCESS_INFORMATION));
    DPRINT("Foo\n");
    /* Easy stuff first, convert the process priority class */
    PriorityClass.Foreground = FALSE;
    PriorityClass.PriorityClass = BasepConvertPriorityClass(dwCreationFlags);
      DPRINT("Foo\n");
    /* Convert the environment */
    if(lpEnvironment && !(dwCreationFlags & CREATE_UNICODE_ENVIRONMENT))
    {
        lpEnvironment = BasepConvertUnicodeEnvironment(lpEnvironment);
        if (!lpEnvironment) return FALSE;
    }
DPRINT("Foo\n");
    /* Get the application name and do all the proper formating necessary */
GetAppName:
    /* See if we have an application name (oh please let us have one!) */
    if (!lpApplicationName)
    {
        /* The fun begins */
        NameBuffer = RtlAllocateHeap(GetProcessHeap(), 
                                     0,
                                     MAX_PATH * sizeof(WCHAR));
        
        /* This is all we have to work with :( */
        lpApplicationName = lpCommandLine;
        
        /* Initialize our friends at the beginning */
        NullBuffer = (LPWSTR)lpApplicationName;
        ScanString = (LPWSTR)lpApplicationName;
        
        /* We will start by looking for a quote */
        if (*ScanString == L'\"')
        {
             /* That was quick */
             SearchDone = TRUE;
             
             /* Advance past quote */
             ScanString++;
             lpApplicationName = ScanString;             
             DPRINT("Foo\n");
             /* Find the closing quote */
             while (*ScanString)
             {
                 if (*ScanString == L'\"')
                 {
                     /* Found it */
                     NullBuffer = ScanString;
                     FoundQuotes = TRUE;
                     break;
                 }
                 DPRINT("Foo\n");
                 /* Keep looking */
                 ScanString++;
                 NullBuffer = ScanString;
             }
        }
        else
        {
            /* No quotes, so we'll be looking for white space */
        WhiteScan:   
            /* Reset the pointer */
            lpApplicationName = lpCommandLine;
            DPRINT("Foo\n");
            /* Find whitespace of Tab */
            while (*ScanString)
            {
                if (*ScanString == ' ' || *ScanString == '\t')
                {
                    /* Found it */
                    NullBuffer = ScanString;
                    break;
                }
                    DPRINT("Foo\n");
                /* Keep looking */
                ScanString++;
                NullBuffer = ScanString;
            }
        }
                 
        /* Set the Null Buffer */
        SaveChar = *NullBuffer;
        *NullBuffer = UNICODE_NULL;
                
        /* Do a search for the file */
        DPRINT("Ready for SearchPathW: %S\n", lpApplicationName);
        RetVal = SearchPathW(NULL,
                             lpApplicationName,
                             L".exe",
                             MAX_PATH,
                             NameBuffer,
                             NULL) * sizeof(WCHAR);
               
        /* Did it find something? */
        if (RetVal)
        {
            /* Get file attributes */
            ULONG Attributes = GetFileAttributesW(NameBuffer);
            if (Attributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                /* Give it a length of 0 to fail, this was a directory. */
                RetVal = 0;
            }
            else
            {
                /* It's a file! */
                RetVal += sizeof(WCHAR);
            }
        }
                
        /* Now check if we have a file, and if the path size is OK */
        if (!RetVal || RetVal >= (MAX_PATH / sizeof(WCHAR)))
        {
            ULONG PathType;
            HANDLE hFile;
            
            /* We failed, try to get the Path Type */
            DPRINT("SearchPathW failed. Retval: %ld\n", RetVal);
            PathType = RtlDetermineDosPathNameType_U(lpApplicationName);
                    
            /* If it's not relative, try to get the error */
            if (PathType != RELATIVE_PATH)
            {
                /* This should fail, and give us a detailed LastError */
                hFile = CreateFileW(lpApplicationName,
                                    GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);
                                        
                /* Did it actually NOT fail? */
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    /* Fake the error */
                    CloseHandle(hFile);
                    SetLastErrorByStatus(STATUS_OBJECT_NAME_NOT_FOUND);
                }
            }
            else
            {
                /* Immediately set the error */
                SetLastErrorByStatus(STATUS_OBJECT_NAME_NOT_FOUND);
            }
                    
            /* Did we already fail once? */
            if (Error)
            {
                SetLastError(Error);
            }
            else
            {
                /* Not yet, cache it */
                Error = GetLastError();
            }
                    
            /* Put back the command line */
            *NullBuffer = SaveChar;
            lpApplicationName = NameBuffer;
                    
            /* 
             * If the search isn't done and we still have cmdline
             * then start over. Ex: c:\ha ha ha\haha.exe
             */
            if (*ScanString && !SearchDone)
            {
                /* Move in the buffer */
                ScanString++;
                NullBuffer = ScanString;
                
                /* We will have to add a quote, since there is a space*/
                QuotesNeeded = TRUE;
                    
                /* And we will also fake the fact we found one */
                FoundQuotes = TRUE;
                    
                /* Start over */
                goto WhiteScan;
            }
                
            /* We totally failed */
            return FALSE;
        }
                
        /* Put back the command line */
        *NullBuffer = SaveChar;
        lpApplicationName = NameBuffer;
        DPRINT("SearchPathW suceeded (%ld): %S\n", RetVal, NameBuffer);
    }
    else if (!lpCommandLine || *lpCommandLine == UNICODE_NULL)
    {
        /* We have an app name (good!) but no command line */
        CmdLineIsAppName = TRUE;
        lpCommandLine = (LPWSTR)lpApplicationName;
    }
     
    /* At this point the name has been toyed with enough to be openable */
    Status = BasepMapFile(lpApplicationName, &hSection, &ApplicationName);
    
    /* Check for failure */
    if (!NT_SUCCESS(Status))
    {
        /* Could be a non-PE File */
        switch (Status)
        {
            /* Check if the Kernel tells us it's not even valid MZ */
            case STATUS_INVALID_IMAGE_NE_FORMAT:
            case STATUS_INVALID_IMAGE_PROTECT:
            case STATUS_INVALID_IMAGE_NOT_MZ:
            
            /* If it's a DOS app, use VDM */
            //if ((BasepCheckDosApp(&ApplicationName)))
            {
                DPRINT1("Launching VDM...\n");
                RtlFreeHeap(GetProcessHeap(), 0, NameBuffer);
                RtlFreeHeap(GetProcessHeap(), 0, ApplicationName.Buffer);
                return CreateProcessW(L"ntvdm.exe",
                                      (LPWSTR)lpApplicationName,
                                      lpProcessAttributes,
                                      lpThreadAttributes,
                                      bInheritHandles,
                                      dwCreationFlags,
                                      lpEnvironment,
                                      lpCurrentDirectory,
                                      lpStartupInfo,
                                      lpProcessInformation);    
            }
            
            /* It's a batch file */
            LPWSTR BatchCommandLine;
            ULONG CmdLineLength;
            UNICODE_STRING CommandLineString;
            LPWSTR TempBuffer;
            PWCHAR Extension = 
            &ApplicationName.Buffer[ApplicationName.Length / sizeof(WCHAR) - 4];
            
            /* Make sure the extensions are correct */
            if (_wcsnicmp(Extension, L".bat", 4) && _wcsnicmp(Extension, L".cmd", 4))
            {
                SetLastError(ERROR_BAD_EXE_FORMAT);
                return FALSE;
            }
            
            /* Calculate the length of the command line */
            CmdLineLength = wcslen(CMD_STRING) + wcslen(lpCommandLine) + 1;
            
            /* If we found quotes, then add them into the length size */
            if (CmdLineIsAppName || FoundQuotes) CmdLineLength += 2;
            CmdLineLength *= sizeof(WCHAR);
            
            /* Allocate space for the new command line */
            BatchCommandLine = RtlAllocateHeap(GetProcessHeap(),
                                               0,
                                               CmdLineLength);
                                              
            /* Build it */
            wcscpy(BatchCommandLine, CMD_STRING);
            if (CmdLineIsAppName || FoundQuotes)
            {
                wcscat(BatchCommandLine, L"\"");
            }
            wcscat(BatchCommandLine, lpCommandLine);
            if (CmdLineIsAppName || FoundQuotes)
            {
                wcscat(BatchCommandLine, L"\"");
            }
            
            /* Create it as a Unicode String */
            RtlInitUnicodeString(&CommandLineString, BatchCommandLine);
            
            /* Set the command line to this */
            lpCommandLine = CommandLineString.Buffer;
            lpApplicationName = NULL;
            
            /* Free memory */
            RtlFreeHeap(GetProcessHeap(), 0, TempBuffer);
            RtlFreeHeap(GetProcessHeap(), 0, ApplicationName.Buffer);
            ApplicationName.Buffer = NULL;
            TempBuffer = NULL;
            goto GetAppName;
            break;
            
            case STATUS_INVALID_IMAGE_WIN_16:
            
                /* It's a Win16 Image, use VDM */
                DPRINT1("Launching VDM...\n");
                RtlFreeHeap(GetProcessHeap(), 0, NameBuffer);
                RtlFreeHeap(GetProcessHeap(), 0, ApplicationName.Buffer);
                return CreateProcessW(L"ntvdm.exe",
                                      (LPWSTR)lpApplicationName,
                                      lpProcessAttributes,
                                      lpThreadAttributes,
                                      bInheritHandles,
                                      dwCreationFlags,
                                      lpEnvironment,
                                      lpCurrentDirectory,
                                      lpStartupInfo,
                                      lpProcessInformation);    

            default:
                /* Invalid Image Type */
                SetLastError(ERROR_BAD_EXE_FORMAT);
                return FALSE;
        }
    }
    
    /* Use our desktop if we didn't get any */
    if (!StartupInfo.lpDesktop)
    {
        StartupInfo.lpDesktop = OurPeb->ProcessParameters->DesktopInfo.Buffer;
    }
    
    /* FIXME: Check if Application is allowed to run */
    
    /* FIXME: Allow CREATE_SEPARATE only for WOW Apps, once we have that. */
    
    /* Get some information about the executable */
    Status = ZwQuerySection(hSection,
                            SectionImageInformation,
                            &SectionImageInfo,
                            sizeof(SectionImageInfo),
                            NULL);
    if(!NT_SUCCESS(Status))
    {
        NtClose(hSection);
        DPRINT1("Unable to get SectionImageInformation, status 0x%x\n", Status);
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    /* Don't execute DLLs */
    if (SectionImageInfo.ImageCharacteristics & IMAGE_FILE_DLL)
    {
        NtClose(hSection);
        DPRINT1("Can't execute a DLL\n");
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return FALSE;
    }
    
    /* FIXME: Check for Debugger */
    
    /* FIXME: Check if Machine Type and SubSys Version Match */

    /* We don't support POSIX or anything else for now */
    if (IMAGE_SUBSYSTEM_WINDOWS_GUI != SectionImageInfo.SubsystemType && 
        IMAGE_SUBSYSTEM_WINDOWS_CUI != SectionImageInfo.SubsystemType)
    {
        NtClose(hSection);
        DPRINT1("Invalid subsystem %d\n", SectionImageInfo.SubsystemType);
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return FALSE;
    }

    /* Initialize the process object attributes */
    ObjectAttributes = BasepConvertObjectAttributes(&LocalObjectAttributes, 
                                                    lpProcessAttributes,
                                                    NULL);
   
    /* Create the Process */
    Status = NtCreateProcess(&hProcess,
                             PROCESS_ALL_ACCESS,
                             ObjectAttributes,
                             NtCurrentProcess(),
                             bInheritHandles,
                             hSection,
                             NULL,
                             NULL);
    if(!NT_SUCCESS(Status))
    {
        NtClose(hSection);
        DPRINT1("Unable to create process, status 0x%x\n", Status);
        SetLastErrorByStatus(Status);
        return FALSE;
    }
    
    /* Set new class */
    Status = NtSetInformationProcess(hProcess,
                                     ProcessPriorityClass,
                                     &PriorityClass,
                                     sizeof(PROCESS_PRIORITY_CLASS));
    if(!NT_SUCCESS(Status))
    {
        NtClose(hProcess);
        NtClose(hSection);
        DPRINT1("Unable to set new process priority, status 0x%x\n", Status);
        SetLastErrorByStatus(Status);
        return FALSE;
    }
    
    /* Set Error Mode */
    if (dwCreationFlags & CREATE_DEFAULT_ERROR_MODE)
    {
        ULONG ErrorMode = SEM_FAILCRITICALERRORS;
        NtSetInformationProcess(hProcess,
                                ProcessDefaultHardErrorMode,
                                &ErrorMode,
                                sizeof(ULONG));
    }

    /* Convert the directory to a full path */
    if (lpCurrentDirectory)
    {
        /* Allocate a buffer */
        CurrentDirectory = RtlAllocateHeap(GetProcessHeap(),
                                           0,
                                           MAX_PATH * sizeof(WCHAR) + 2);
                                           
        /* Get the length */
        if (GetFullPathNameW(lpCurrentDirectory,
                             MAX_PATH,
                             CurrentDirectory,
                             &CurrentDirectoryPart) > MAX_PATH)
        {
            DPRINT1("Directory name too long\n");
            SetLastError(ERROR_DIRECTORY);
            return FALSE;
        }
    }
    
    /* Insert quotes if needed */
    if (QuotesNeeded || CmdLineIsAppName)
    {
        /* Allocate a buffer */
        QuotedCmdLine = RtlAllocateHeap(GetProcessHeap(), 
                                        0,
                                        (wcslen(lpCommandLine) + 2 + 1) * 
                                        sizeof(WCHAR));
                                        
        /* Copy the first quote */
        wcscpy(QuotedCmdLine, L"\"");
        
        /* Save a null char */
        if (QuotesNeeded)
        {
            SaveChar = *NullBuffer;
            *NullBuffer = UNICODE_NULL;
        }
        
        /* Add the command line and the finishing quote */
        wcscat(QuotedCmdLine, lpCommandLine);
        wcscat(QuotedCmdLine, L"\"");
        
        /* Add the null char */
        if (QuotesNeeded)
        {
            *NullBuffer = SaveChar;
            wcscat(QuotedCmdLine, NullBuffer);
        }
        
        DPRINT("Quoted CmdLine: %S\n", QuotedCmdLine);
    }
    
    /* Get the Process Information */
    Status = NtQueryInformationProcess(hProcess,
                                       ProcessBasicInformation,
                                       &ProcessBasicInfo,
                                       sizeof(ProcessBasicInfo),
                                       NULL);

    /* Notify CSRSS */
    Status = BasepNotifyCsrOfCreation(dwCreationFlags,
                                      (HANDLE)ProcessBasicInfo.UniqueProcessId,
                                      SectionImageInfo.SubsystemType,
                                      &hConsole,
                                      &hInput,
                                      &hOutput);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSR Notification Failed");
        SetLastErrorByStatus(Status);
        return FALSE;
    }
    
    /* Create Process Environment */
    RemotePeb = ProcessBasicInfo.PebBaseAddress;
    Status = BasepInitializeEnvironment(hProcess,
                                        RemotePeb,
                                        (LPWSTR)lpApplicationName,
                                        CurrentDirectory,
                                        (QuotesNeeded || CmdLineIsAppName) ?
                                        QuotedCmdLine : lpCommandLine,
                                        lpEnvironment,
                                        lpStartupInfo,
                                        dwCreationFlags,
                                        bInheritHandles,
                                        hInput,
                                        hOutput);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not initialize Process Environment\n");
        SetLastErrorByStatus(Status);
        return FALSE;
    }
    
    /* Close the section */
    NtClose(hSection);
    hSection = NULL;

    /* Duplicate the handles if needed */
    if (!bInheritHandles && !(StartupInfo.dwFlags & STARTF_USESTDHANDLES) &&
        SectionImageInfo.SubsystemType == IMAGE_SUBSYSTEM_WINDOWS_CUI)
    {
        PRTL_USER_PROCESS_PARAMETERS RemoteParameters;
        
        /* Get the remote parameters */
        Status = NtReadVirtualMemory(hProcess,
                                     &RemotePeb->ProcessParameters,
                                     &RemoteParameters,
                                     sizeof(PVOID),
                                     NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to read memory\n");
            return FALSE;
        }
        
        /* Duplicate and write the handles */
        BasepDuplicateAndWriteHandle(hProcess,
                                     OurPeb->ProcessParameters->StandardInput,
                                     &RemoteParameters->StandardInput);
        BasepDuplicateAndWriteHandle(hProcess,
                                     OurPeb->ProcessParameters->StandardOutput,
                                     &RemoteParameters->StandardOutput);
        BasepDuplicateAndWriteHandle(hProcess,
                                     OurPeb->ProcessParameters->StandardError,
                                     &RemoteParameters->StandardError);
    }
        
    /* Create the first thread */
    DPRINT("Creating thread for process (EntryPoint = 0x%.08x)\n",
            SectionImageInfo.TransferAddress);
    hThread = BasepCreateFirstThread(hProcess,
                                     lpThreadAttributes,
                                     &SectionImageInfo,
                                     &ClientId);

    if (hThread == NULL)
    {
        DPRINT1("Could not create Initial Thread\n");
        return FALSE;
    }
    
    if (!(dwCreationFlags & CREATE_SUSPENDED))
    {
        NtResumeThread(hThread, &Dummy);
    }
    
    /* Cleanup Environment */    
    if (lpEnvironment && !(dwCreationFlags & CREATE_UNICODE_ENVIRONMENT))
    {
        RtlDestroyEnvironment(lpEnvironment);
    }

    /* Return Data */        
    lpProcessInformation->dwProcessId = (DWORD)ClientId.UniqueProcess;
    lpProcessInformation->dwThreadId = (DWORD)ClientId.UniqueThread;
    lpProcessInformation->hProcess = hProcess;
    lpProcessInformation->hThread = hThread;
    DPRINT("hThread[%lx]: %lx inside hProcess[%lx]: %lx\n", hThread,
            ClientId.UniqueThread, ClientId.UniqueProcess, hProcess);
    hProcess =  hThread = NULL;
            
    /* De-allocate heap strings */
    if (NameBuffer) RtlFreeHeap(GetProcessHeap(), 0, NameBuffer);
    if (ApplicationName.Buffer)
        RtlFreeHeap(GetProcessHeap(), 0, ApplicationName.Buffer);
    if (CurrentDirectory) RtlFreeHeap(GetProcessHeap(), 0, CurrentDirectory);
    if (QuotedCmdLine) RtlFreeHeap(GetProcessHeap(), 0, QuotedCmdLine);

    /* Kill any handles still alive */
    if (hSection) NtClose(hSection);
    if (hThread)
    {
        /* We don't know any more details then this */
        NtTerminateProcess(hProcess, STATUS_UNSUCCESSFUL);
        NtClose(hThread);
    }
    if (hProcess) NtClose(hProcess);

    /* Return Success */
    return TRUE;
}

/* EOF */

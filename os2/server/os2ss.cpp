/* $Id: os2ss.cpp,v 1.2 2003/01/07 16:23:12 robd Exp $
 *
 * reactos/subsys/csrss/api/process.c
 *
 * "\windows\ApiPort" port process management functions
 *
 * ReactOS Operating System
 */
 // TODO: Rewrite the whole file. This is just a copy

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <ntos/synch.h>

extern "C" {
BOOL CsrServerInitialization(ULONG ArgumentCount, PWSTR *ArgumentArray);
VOID DisplayString(LPCWSTR lpwString);
//BOOL STDCALL CsrServerInitialization (ULONG ArgumentCount, PWSTR *ArgumentArray);
//VOID STDCALL DisplayString(LPCWSTR lpwString);
//VOID STDCALL PrintString (char* fmt, ...);
//NTSTATUS STDCALL NtDisplayString(IN PUNICODE_STRING DisplayString);

void
DisplayString(LPCWSTR lpwString)
{
  UNICODE_STRING us;

  RtlInitUnicodeString(&us, lpwString);
  NtDisplayString(&us);
}

/*
void
PrintString(char* fmt,...)
{
  char buffer[512];
  va_list ap;
  UNICODE_STRING UnicodeString;
  ANSI_STRING AnsiString;

  va_start(ap, fmt);
  vsprintf(buffer, fmt, ap);
  va_end(ap);

  RtlInitAnsiString(&AnsiString, buffer);
  RtlAnsiStringToUnicodeString(&UnicodeString,
			       &AnsiString,
			       TRUE);
  NtDisplayString(&UnicodeString);
  RtlFreeUnicodeString(&UnicodeString);
}
 */

}

/* server variables */

int NumProcesses;



/* Native image's entry point */

void NtProcessStartup (PPEB Peb)
{
   PRTL_USER_PROCESS_PARAMETERS ProcParams;
   PWSTR ArgBuffer;
   PWSTR *argv;
   ULONG argc = 0;
   int i = 0;
   int afterlastspace = 0;
   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE CsrssInitEvent;
   UNICODE_STRING UnicodeString;
   NTSTATUS Status;

   ProcParams = RtlNormalizeProcessParams (Peb->ProcessParameters);

   argv = (PWSTR *)RtlAllocateHeap (Peb->ProcessHeap,
                                    0, 512 * sizeof(PWSTR));
   ArgBuffer = (PWSTR)RtlAllocateHeap (Peb->ProcessHeap,
                                       0,
                                       ProcParams->CommandLine.Length + sizeof(WCHAR));
   memcpy (ArgBuffer,
           ProcParams->CommandLine.Buffer,
           ProcParams->CommandLine.Length + sizeof(WCHAR));

   while (ArgBuffer[i])
     {
        if (ArgBuffer[i] == L' ')
          {
             argc++;
             ArgBuffer[i] = L'\0';
             argv[argc-1] = &(ArgBuffer[afterlastspace]);
             i++;
             while (ArgBuffer[i] == L' ')
                i++;
             afterlastspace = i;
          }
        else
          {
             i++;
          }
     }

   if (ArgBuffer[afterlastspace] != L'\0')
     {
        argc++;
        ArgBuffer[i] = L'\0';
        argv[argc-1] = &(ArgBuffer[afterlastspace]);
     }
   
   RtlInitUnicodeString(&UnicodeString,
                        L"\\CsrssInitDone");
   InitializeObjectAttributes(&ObjectAttributes,
                              &UnicodeString,
                              EVENT_ALL_ACCESS,
                              0,
                              NULL);
   Status = NtOpenEvent(&CsrssInitEvent,
                        EVENT_ALL_ACCESS,
                        &ObjectAttributes);
   if (!NT_SUCCESS(Status))
     {
        DbgPrint("CSR: Failed to open csrss notification event\n");
     }
   if (CsrServerInitialization (argc, argv) == TRUE)
     {

        NtSetEvent(CsrssInitEvent,
                   NULL);
        
        RtlFreeHeap (Peb->ProcessHeap,
                     0, argv);
        RtlFreeHeap (Peb->ProcessHeap,
                     0,
                     ArgBuffer);

        /* terminate the current thread only */
        NtTerminateThread( NtCurrentThread(), 0 );
     }
   else
     {
        DisplayString( L"CSR: Subsystem initialization failed.\n" );

        RtlFreeHeap (Peb->ProcessHeap,
                     0, argv);
        RtlFreeHeap (Peb->ProcessHeap,
                     0,
                     ArgBuffer);

        /*
         * Tell SM we failed.
         */
        NtTerminateProcess( NtCurrentProcess(), 0 );
   }
}

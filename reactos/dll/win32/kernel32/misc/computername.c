/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * PURPOSE:         Computer name functions
 * FILE:            lib/kernel32/misc/computername.c
 * PROGRAMER:       Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>


/* FUNCTIONS *****************************************************************/

static BOOL GetComputerNameFromRegistry( LPWSTR RegistryKey,
					 LPWSTR ValueNameStr,
					 LPWSTR lpBuffer,
					 LPDWORD nSize ) {
    PKEY_VALUE_PARTIAL_INFORMATION KeyInfo;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    HANDLE KeyHandle;
    ULONG KeyInfoSize;
    ULONG ReturnSize;
    NTSTATUS Status;

    RtlInitUnicodeString (&KeyName,RegistryKey);
    InitializeObjectAttributes (&ObjectAttributes,
				&KeyName,
				OBJ_CASE_INSENSITIVE,
				NULL,
				NULL);
    Status = ZwOpenKey (&KeyHandle,
			KEY_READ,
			&ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
	SetLastErrorByStatus (Status);
	return FALSE;
    }

    KeyInfoSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
	*nSize * sizeof(WCHAR);
    KeyInfo = RtlAllocateHeap (RtlGetProcessHeap (),
			       0,
			       KeyInfoSize);
    if (KeyInfo == NULL)
    {
	ZwClose (KeyHandle);
	SetLastError (ERROR_OUTOFMEMORY);
	return FALSE;
    }

    RtlInitUnicodeString (&ValueName,ValueNameStr);

    Status = ZwQueryValueKey (KeyHandle,
			      &ValueName,
			      KeyValuePartialInformation,
			      KeyInfo,
			      KeyInfoSize,
			      &ReturnSize);
    if (!NT_SUCCESS(Status))
    {
	RtlFreeHeap (RtlGetProcessHeap (),
		     0,
		     KeyInfo);
	ZwClose (KeyHandle);
	SetLastErrorByStatus (Status);
	return FALSE;
    }

    if( *nSize > (KeyInfo->DataLength / sizeof(WCHAR)) ) {
	*nSize = KeyInfo->DataLength / sizeof(WCHAR);
	lpBuffer[*nSize] = 0;
    }

    RtlCopyMemory (lpBuffer,
		   KeyInfo->Data,
		   *nSize * sizeof(WCHAR));

    RtlFreeHeap (RtlGetProcessHeap (),
		 0,
		 KeyInfo)
;
    ZwClose (KeyHandle);

    return TRUE;
}

/*
 * @implemented
 */
BOOL STDCALL
GetComputerNameExW (
    COMPUTER_NAME_FORMAT NameType,
    LPWSTR lpBuffer,
    LPDWORD nSize
    )
{
    UNICODE_STRING ResultString;
    UNICODE_STRING DomainPart;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    NTSTATUS Status;

    switch( NameType ) {
    case ComputerNameNetBIOS:
	return GetComputerNameFromRegistry
	    ( L"\\Registry\\Machine\\System\\CurrentControlSet"
	      L"\\Control\\ComputerName\\ComputerName",
	      L"ComputerName",
	      lpBuffer,
	      nSize );

    case ComputerNameDnsDomain:
	return GetComputerNameFromRegistry
	    ( L"\\Registry\\Machine\\System\\CurrentControlSet"
	      L"\\Services\\Tcpip\\Parameters",
	      L"Domain",
	      lpBuffer,
	      nSize );

    case ComputerNameDnsFullyQualified:
        ResultString.Length = 0;
        ResultString.MaximumLength = (USHORT)*nSize * sizeof(WCHAR);
        ResultString.Buffer = lpBuffer;

        RtlZeroMemory(QueryTable, sizeof(QueryTable));
        RtlInitUnicodeString(&DomainPart, NULL);
        QueryTable[0].Name = L"HostName";
        QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        QueryTable[0].EntryContext = &DomainPart;
	
        Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                        L"\\Registry\\Machine\\System"
                                        L"\\CurrentControlSet\\Services\\Tcpip"
                                        L"\\Parameters",
                                        QueryTable, NULL, NULL);

        if( NT_SUCCESS(Status) ) {
            RtlAppendUnicodeStringToString(&ResultString, &DomainPart);
            RtlAppendUnicodeToString(&ResultString, L".");
            RtlFreeUnicodeString(&DomainPart);

            RtlInitUnicodeString(&DomainPart, NULL);
            QueryTable[0].Name = L"Domain";
            QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
            QueryTable[0].EntryContext = &DomainPart;

            Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                            L"\\Registry\\Machine\\System"
                                            L"\\CurrentControlSet\\Services\\Tcpip"
                                            L"\\Parameters",
                                            QueryTable, NULL, NULL);

            if( NT_SUCCESS(Status) ) {
                RtlAppendUnicodeStringToString(&ResultString, &DomainPart);
                RtlFreeUnicodeString(&DomainPart);
                *nSize = ResultString.Length / sizeof(WCHAR);
                return TRUE;
            }
        }
	return FALSE;

    case ComputerNameDnsHostname:
	return GetComputerNameFromRegistry
	    ( L"\\Registry\\Machine\\System\\CurrentControlSet"
	      L"\\Services\\Tcpip\\Parameters",
	      L"Hostname",
	      lpBuffer,
	      nSize );

    case ComputerNamePhysicalDnsDomain:
	return GetComputerNameFromRegistry
	    ( L"\\Registry\\Machine\\System\\CurrentControlSet"
	      L"\\Services\\Tcpip\\Parameters",
	      L"Domain",
	      lpBuffer,
	      nSize );

	/* XXX Redo these */
    case ComputerNamePhysicalDnsFullyQualified:
	return GetComputerNameExW( ComputerNameDnsFullyQualified,
				   lpBuffer, nSize );
    case ComputerNamePhysicalDnsHostname:
	return GetComputerNameExW( ComputerNameDnsHostname,
				   lpBuffer, nSize );
    case ComputerNamePhysicalNetBIOS:
	return GetComputerNameExW( ComputerNameNetBIOS,
				   lpBuffer, nSize );

    case ComputerNameMax:
	return FALSE;
    }

    return FALSE;
}

/*
 * @implemented
 */
BOOL
STDCALL
GetComputerNameExA (
    COMPUTER_NAME_FORMAT NameType,
    LPSTR lpBuffer,
    LPDWORD nSize
    )
{
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    BOOL Result;
    PWCHAR TempBuffer = RtlAllocateHeap( RtlGetProcessHeap(), 0, *nSize * sizeof(WCHAR) );

    if( !TempBuffer ) {
	return ERROR_OUTOFMEMORY;
    }

    AnsiString.MaximumLength = (USHORT)*nSize;
    AnsiString.Length = 0;
    AnsiString.Buffer = lpBuffer;

    Result = GetComputerNameExW( NameType, TempBuffer, nSize );

    if( Result ) {
	UnicodeString.MaximumLength = (USHORT)*nSize * sizeof(WCHAR);
	UnicodeString.Length = (USHORT)*nSize * sizeof(WCHAR);
	UnicodeString.Buffer = TempBuffer;

	RtlUnicodeStringToAnsiString (&AnsiString,
				      &UnicodeString,
				      FALSE);
    }

    RtlFreeHeap( RtlGetProcessHeap(), 0, TempBuffer );

    return Result;
}

/*
 * @implemented
 */
BOOL STDCALL
GetComputerNameA (LPSTR lpBuffer,
		  LPDWORD lpnSize)
{
    return GetComputerNameExA( ComputerNameNetBIOS, lpBuffer, lpnSize );
}


/*
 * @implemented
 */
BOOL STDCALL
GetComputerNameW (LPWSTR lpBuffer,
		  LPDWORD lpnSize)
{
    return GetComputerNameExW( ComputerNameNetBIOS, lpBuffer, lpnSize );
}


/*
 * @implemented
 */
static BOOL
IsValidComputerName (
    COMPUTER_NAME_FORMAT NameType,
    LPCWSTR lpComputerName)
{
  PWCHAR p;
  ULONG Length;

  /* FIXME: do verification according to NameType */

  Length = 0;
  p = (PWCHAR)lpComputerName;
  while (*p != 0)
    {
      if ((!iswctype (*p, _ALPHA) && !iswctype (*p, _DIGIT)) ||
	    *p == L'!' ||
	    *p == L'@' ||
	    *p == L'#' ||
	    *p == L'$' ||
	    *p == L'%' ||
	    *p == L'^' ||
	    *p == L'&' ||
	    *p == L'\'' ||
	    *p == L')' ||
	    *p == L'(' ||
	    *p == L'.' ||
	    *p == L'_' ||
	    *p == L'{' ||
	    *p == L'}' ||
	    *p == L'~')
	return FALSE;

      Length++;
      p++;
    }

  if (Length == 0 ||
      Length > MAX_COMPUTERNAME_LENGTH)
    return FALSE;

  return TRUE;
}


static BOOL SetComputerNameToRegistry(
    LPCWSTR RegistryKey,
    LPCWSTR ValueNameStr,
    LPCWSTR lpBuffer)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    HANDLE KeyHandle;
    NTSTATUS Status;

    RtlInitUnicodeString (&KeyName, RegistryKey);
    InitializeObjectAttributes (&ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL );

    Status = NtOpenKey (&KeyHandle,
        KEY_WRITE,
        &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus (Status);
        return FALSE;
    }

    RtlInitUnicodeString (&ValueName, ValueNameStr);

    Status = NtSetValueKey (KeyHandle,
        &ValueName,
        0,
        REG_SZ,
        (PVOID)lpBuffer,
        (wcslen (lpBuffer) + 1) * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
    {
        ZwClose (KeyHandle);
        SetLastErrorByStatus (Status);
        return FALSE;
    }

    NtFlushKey (KeyHandle);
    ZwClose (KeyHandle);

    return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
SetComputerNameA (LPCSTR lpComputerName)
{
    return SetComputerNameExA( ComputerNamePhysicalNetBIOS, lpComputerName );
}


/*
 * @implemented
 */
BOOL STDCALL
SetComputerNameW (LPCWSTR lpComputerName)
{
    return SetComputerNameExW( ComputerNamePhysicalNetBIOS, lpComputerName );
}


/*
 * @implemented
 */
BOOL STDCALL
SetComputerNameExA (
    COMPUTER_NAME_FORMAT NameType,
    LPCSTR lpBuffer)
{
    UNICODE_STRING Buffer;
    BOOL bResult;

    RtlCreateUnicodeStringFromAsciiz (&Buffer,
				    (LPSTR)lpBuffer);

    bResult = SetComputerNameExW (NameType, Buffer.Buffer);

    RtlFreeUnicodeString (&Buffer);

    return bResult;
}


/*
 * @implemented
 */
BOOL STDCALL
SetComputerNameExW (
    COMPUTER_NAME_FORMAT NameType,
    LPCWSTR lpBuffer)
{
  if (!IsValidComputerName (NameType, lpBuffer))
    {
      SetLastError (ERROR_INVALID_PARAMETER);
      return FALSE;
    }

  switch( NameType ) {
    case ComputerNamePhysicalDnsDomain:
      return SetComputerNameToRegistry
        ( L"\\Registry\\Machine\\System\\CurrentControlSet"
          L"\\Services\\Tcpip\\Parameters",
          L"Domain",
          lpBuffer );

    case ComputerNamePhysicalDnsHostname:
      return SetComputerNameToRegistry
        ( L"\\Registry\\Machine\\System\\CurrentControlSet"
          L"\\Services\\Tcpip\\Parameters",
          L"Hostname",
          lpBuffer );

    case ComputerNamePhysicalNetBIOS:
      return SetComputerNameToRegistry
        ( L"\\Registry\\Machine\\System\\CurrentControlSet"
          L"\\Control\\ComputerName\\ComputerName",
          L"ComputerName",
          lpBuffer );

    default:
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
  }
}

/* EOF */

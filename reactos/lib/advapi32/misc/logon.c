/* $Id: logon.c,v 1.7 2004/07/07 08:41:47 gvg Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/advapi32/misc/logon.c
 * PURPOSE:     Logon functions
 * PROGRAMMER:  Eric Kohl
 */

#define NTOS_MODE_USER
#include <ntos.h>
#include <windows.h>
#include <string.h>

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
BOOL STDCALL
CreateProcessAsUserA (HANDLE hToken,
		      LPCSTR lpApplicationName,
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
  NTSTATUS Status;

  /* Create the process with a suspended main thread */
  if (!CreateProcessA (lpApplicationName,
		       lpCommandLine,
		       lpProcessAttributes,
		       lpThreadAttributes,
		       bInheritHandles,
		       dwCreationFlags | CREATE_SUSPENDED,
		       lpEnvironment,
		       lpCurrentDirectory,
		       lpStartupInfo,
		       lpProcessInformation))
    {
      return FALSE;
    }

  /* Set the new process token */
  Status = NtSetInformationProcess (lpProcessInformation->hProcess,
				    ProcessAccessToken,
				    (PVOID)&hToken,
				    sizeof (HANDLE));
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  /* Resume the main thread */
  if (!(dwCreationFlags & CREATE_SUSPENDED))
    {
      ResumeThread (lpProcessInformation->hThread);
    }

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
CreateProcessAsUserW (HANDLE hToken,
		      LPCWSTR lpApplicationName,
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

  /* Create the process with a suspended main thread */
  if (!CreateProcessW (lpApplicationName,
		       lpCommandLine,
		       lpProcessAttributes,
		       lpThreadAttributes,
		       bInheritHandles,
		       dwCreationFlags | CREATE_SUSPENDED,
		       lpEnvironment,
		       lpCurrentDirectory,
		       lpStartupInfo,
		       lpProcessInformation))
    {
      return FALSE;
    }

  /* Set the new process token */
  Status = NtSetInformationProcess (lpProcessInformation->hProcess,
				    ProcessAccessToken,
				    (PVOID)&hToken,
				    sizeof (HANDLE));
  if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError (Status));
      return FALSE;
    }

  /* Resume the main thread */
  if (!(dwCreationFlags & CREATE_SUSPENDED))
    {
      ResumeThread (lpProcessInformation->hThread);
    }

  return TRUE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
LogonUserA (LPSTR lpszUsername,
	    LPSTR lpszDomain,
	    LPSTR lpszPassword,
	    DWORD dwLogonType,
	    DWORD dwLogonProvider,
	    PHANDLE phToken)
{
  return FALSE;
}


static BOOL STDCALL
SamGetUserSid (LPCWSTR UserName,
	       PSID *Sid)
{
  PSID lpSid;
  DWORD dwLength;
  HKEY hUsersKey;
  HKEY hUserKey;

  if (Sid != NULL)
    *Sid = NULL;

  /* Open the Users key */
  if (RegOpenKeyExW (HKEY_LOCAL_MACHINE,
		     L"SAM\\SAM\\Domains\\Account\\Users",
		     0,
		     KEY_READ,
		     &hUsersKey))
    {
      DPRINT1 ("Failed to open Users key! (Error %lu)\n", GetLastError());
      return FALSE;
    }

  /* Open the user key */
  if (RegOpenKeyExW (hUsersKey,
		     UserName,
		     0,
		     KEY_READ,
		     &hUserKey))
    {
      if (GetLastError () == ERROR_FILE_NOT_FOUND)
	{
	  DPRINT1 ("Invalid user name!\n");
	  SetLastError (ERROR_NO_SUCH_USER);
	}
      else
	{
	  DPRINT1 ("Failed to open user key! (Error %lu)\n", GetLastError());
	}

      RegCloseKey (hUsersKey);
      return FALSE;
    }

  RegCloseKey (hUsersKey);

  /* Get SID size */
  dwLength = 0;
  if (RegQueryValueExW (hUserKey,
			L"Sid",
			NULL,
			NULL,
			NULL,
			&dwLength))
    {
      DPRINT1 ("Failed to read the SID size! (Error %lu)\n", GetLastError());
      RegCloseKey (hUserKey);
      return FALSE;
    }

  /* Allocate sid buffer */
  DPRINT ("Required SID buffer size: %lu\n", dwLength);
  lpSid = (PSID)RtlAllocateHeap (RtlGetProcessHeap (),
				 0,
				 dwLength);
  if (lpSid == NULL)
    {
      DPRINT1 ("Failed to allocate SID buffer!\n");
      RegCloseKey (hUserKey);
      return FALSE;
    }

  /* Read sid */
  if (RegQueryValueExW (hUserKey,
			L"Sid",
			NULL,
			NULL,
			(LPBYTE)lpSid,
			&dwLength))
    {
      DPRINT1 ("Failed to read the SID! (Error %lu)\n", GetLastError());
      RtlFreeHeap (RtlGetProcessHeap (),
		   0,
		   lpSid);
      RegCloseKey (hUserKey);
      return FALSE;
    }

  RegCloseKey (hUserKey);

  *Sid = lpSid;

  return TRUE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
LogonUserW (LPWSTR lpszUsername,
	    LPWSTR lpszDomain,
	    LPWSTR lpszPassword,
	    DWORD dwLogonType,
	    DWORD dwLogonProvider,
	    PHANDLE phToken)
{
  /* FIXME shouldn't use hard-coded list of privileges */
  static struct
    {
      LPCWSTR PrivName;
      DWORD Attributes;
    }
  DefaultPrivs[] =
    {
      { L"SeUnsolicitedInputPrivilege", 0 },
      { L"SeMachineAccountPrivilege", 0 },
      { L"SeSecurityPrivilege", 0 },
      { L"SeTakeOwnershipPrivilege", 0 },
      { L"SeLoadDriverPrivilege", 0 },
      { L"SeSystemProfilePrivilege", 0 },
      { L"SeSystemtimePrivilege", 0 },
      { L"SeProfileSingleProcessPrivilege", 0 },
      { L"SeIncreaseBasePriorityPrivilege", 0 },
      { L"SeCreatePagefilePrivilege", 0 },
      { L"SeBackupPrivilege", 0 },
      { L"SeRestorePrivilege", 0 },
      { L"SeShutdownPrivilege", 0 },
      { L"SeDebugPrivilege", 0 },
      { L"SeSystemEnvironmentPrivilege", 0 },
      { L"SeChangeNotifyPrivilege", SE_PRIVILEGE_ENABLED | SE_PRIVILEGE_ENABLED_BY_DEFAULT },
      { L"SeRemoteShutdownPrivilege", 0 },
      { L"SeUndockPrivilege", 0 },
      { L"SeEnableDelegationPrivilege", 0 },
      { L"SeImpersonatePrivilege", SE_PRIVILEGE_ENABLED | SE_PRIVILEGE_ENABLED_BY_DEFAULT },
      { L"SeCreateGlobalPrivilege", SE_PRIVILEGE_ENABLED | SE_PRIVILEGE_ENABLED_BY_DEFAULT }
    };
  OBJECT_ATTRIBUTES ObjectAttributes;
  SECURITY_QUALITY_OF_SERVICE Qos;
  TOKEN_USER TokenUser;
  TOKEN_OWNER TokenOwner;
  TOKEN_PRIMARY_GROUP TokenPrimaryGroup;
  TOKEN_GROUPS TokenGroups;
//  PTOKEN_GROUPS TokenGroups;
  PTOKEN_PRIVILEGES TokenPrivileges;
  TOKEN_DEFAULT_DACL TokenDefaultDacl;
  LARGE_INTEGER ExpirationTime;
  LUID AuthenticationId;
  TOKEN_SOURCE TokenSource;
  PSID UserSid;
  ACL Dacl;
  NTSTATUS Status;
  SID_IDENTIFIER_AUTHORITY SystemAuthority = {SECURITY_NT_AUTHORITY};
  unsigned i;

  Qos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
  Qos.ImpersonationLevel = SecurityAnonymous;
  Qos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
  Qos.EffectiveOnly = FALSE;

  ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
  ObjectAttributes.RootDirectory = NULL;
  ObjectAttributes.ObjectName = NULL;
  ObjectAttributes.Attributes = 0;
  ObjectAttributes.SecurityDescriptor = NULL;
  ObjectAttributes.SecurityQualityOfService = &Qos;

//  AuthenticationId = SYSTEM_LUID;
  AuthenticationId.LowPart = 1000;
  AuthenticationId.HighPart = 0;
  ExpirationTime.QuadPart = -1;

  /* Get the user SID from the registry */
  if (!SamGetUserSid (lpszUsername, &UserSid))
    {
      DPRINT ("SamGetUserSid() failed\n");
      RtlAllocateAndInitializeSid (&SystemAuthority,
				   5,
				   SECURITY_NT_NON_UNIQUE_RID,
				   0x12345678,
				   0x12345678,
				   0x12345678,
				   DOMAIN_USER_RID_ADMIN,
				   SECURITY_NULL_RID,
				   SECURITY_NULL_RID,
				   SECURITY_NULL_RID,
				   &UserSid);
    }

  TokenUser.User.Sid = UserSid;
  TokenUser.User.Attributes = 0;

//  TokenGroups = NULL;
  TokenGroups.GroupCount = 1;
  TokenGroups.Groups[0].Sid = UserSid; /* FIXME */
  TokenGroups.Groups[0].Attributes = SE_GROUP_ENABLED;

  TokenPrivileges = RtlAllocateHeap(GetProcessHeap(), 0,
                                    sizeof(TOKEN_PRIVILEGES)
                                    + sizeof(DefaultPrivs) / sizeof(DefaultPrivs[0])
                                      * sizeof(LUID_AND_ATTRIBUTES));
  if (NULL == TokenPrivileges)
    {
      RtlFreeSid (UserSid);
      SetLastError(ERROR_OUTOFMEMORY);
      return FALSE;
    }
  TokenPrivileges->PrivilegeCount = 0;
  for (i = 0; i < sizeof(DefaultPrivs) / sizeof(DefaultPrivs[0]); i++)
    {
      if (! LookupPrivilegeValueW(NULL, DefaultPrivs[i].PrivName,
                                  &TokenPrivileges->Privileges[TokenPrivileges->PrivilegeCount].Luid))
        {
          DPRINT1("Can't set privilege %S\n", DefaultPrivs[i].PrivName);
        }
      else
        {
          TokenPrivileges->Privileges[TokenPrivileges->PrivilegeCount].Attributes = DefaultPrivs[i].Attributes;
          TokenPrivileges->PrivilegeCount++;
        }
    }

  TokenOwner.Owner = UserSid;

  TokenPrimaryGroup.PrimaryGroup = UserSid;

  Status = RtlCreateAcl (&Dacl, sizeof(ACL), ACL_REVISION);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeHeap(GetProcessHeap(), 0, TokenPrivileges);
      RtlFreeSid (UserSid);
      return FALSE;
    }

  TokenDefaultDacl.DefaultDacl = &Dacl;

  memcpy (TokenSource.SourceName,
	  "**ANON**",
	  8);
  Status = NtAllocateLocallyUniqueId (&TokenSource.SourceIdentifier);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeSid (UserSid);
      return FALSE;
    }

  Status = NtCreateToken (phToken,
			  TOKEN_ALL_ACCESS,
			  &ObjectAttributes,
			  TokenPrimary,
			  &AuthenticationId,
			  &ExpirationTime,
			  &TokenUser,
//			  TokenGroups,
			  &TokenGroups,
			  TokenPrivileges,
			  &TokenOwner,
			  &TokenPrimaryGroup,
			  &TokenDefaultDacl,
			  &TokenSource);

  RtlFreeHeap(GetProcessHeap(), 0, TokenPrivileges);
  RtlFreeSid (UserSid);

  return NT_SUCCESS(Status);
}

/* EOF */

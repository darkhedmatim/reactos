/* $Id: stubs.c,v 1.1 2003/11/24 14:25:28 weiden Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/msgina/stubs.c
 * PURPOSE:         msgina.dll stubs
 * PROGRAMMER:      Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * NOTES:           If you implement a function, remove it from this file
 * UPDATE HISTORY:
 *      24-11-2003  Created
 */
#include <windows.h>
#include <WinWlx.h>

#define UNIMPLEMENTED \
  DbgPrint("MSGINA:  %s at %s:%d is UNIMPLEMENTED!\n",__FUNCTION__,__FILE__,__LINE__)


DWORD WINAPI
ShellShutdownDialog(
    HWND hParent,
    DWORD Unknown,
    BOOL bHideLogoff)
{
  /* Return values:
   * 0x00: Cancelled/Help
   * 0x01: Log off user
   * 0x02: Shutdown
   * 0x04: Reboot
   * 0x10: Standby
   * 0x40: Hibernate
   */
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL WINAPI
WlxActivateUserShell(
	PVOID pWlxContext,
	PWSTR pszDesktopName,
	PWSTR pszMprLogonScript,
	PVOID pEnvironment)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
VOID WINAPI
WlxDisplayLockedNotice(
	PVOID pWlxContext)
{
  UNIMPLEMENTED;
  return;
}


/*
 * @unimplemented
 */
VOID WINAPI
WlxDisplaySASNotice(
	PVOID pWlxContext)
{
  UNIMPLEMENTED;
  return;
}


/*
 * @unimplemented
 */
BOOL WINAPI
WlxInitialize(
	LPWSTR lpWinsta,
	HANDLE hWlx,
	PVOID  pvReserved,
	PVOID  pWinlogonFunctions,
	PVOID  *pWlxContext)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
WlxIsLockOk(
	PVOID pWlxContext)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
WlxIsLogoffOk(
	PVOID pWlxContext)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
int WINAPI
WlxLoggedOnSAS(
	PVOID pWlxContext,
	DWORD dwSasType,
	PVOID pReserved)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
int WINAPI
WlxLoggedOutSAS(
	PVOID                pWlxContext,
	DWORD                dwSasType,
	PLUID                pAuthenticationId,
	PSID                 pLogonSid,
	PDWORD               pdwOptions,
	PHANDLE              phToken,
	PWLX_MPR_NOTIFY_INFO pNprNotifyInfo,
	PVOID                *pProfile)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
VOID WINAPI
WlxLogoff(
	PVOID pWlxContext)
{
  UNIMPLEMENTED;
  return;
}


/*
 * @unimplemented
 */
BOOL WINAPI
WlxNegotiate(
	DWORD  dwWinlogonVersion,
	PDWORD pdwDllVersion)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
VOID WINAPI
WlxShutdown(
	PVOID pWlxContext,
	DWORD ShutdownType)
{
  UNIMPLEMENTED;
  return;
}


/*
 * @unimplemented
 */
int WINAPI
WlxWkstaLockedSAS(
	PVOID pWlxContext,
	DWORD dwSasType)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
BOOL WINAPI
WlxScreenSaverNotify(
	PVOID pWlxContext,
	BOOL  *pSecure)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
WlxStartApplication(
	PVOID pWlxContext,
	PWSTR pszDesktopName,
	PVOID pEnvironment,
	PWSTR pszCmdLine)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
WlxDisplayStatusMessage(
	PVOID pWlxContext,
	HDESK hDesktop,
	DWORD dwOptions,
	PWSTR pTitle,
	PWSTR pMessage)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
WlxGetStatusMessage(
	PVOID pWlxContext,
	DWORD *pdwOptions,
	PWSTR pMessage,
	DWORD dwBufferSize)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
WlxNetworkProviderLoad(
	PVOID                pWlxContext,
	PWLX_MPR_NOTIFY_INFO pNprNotifyInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
WlxRemoveStatusMessage(
	PVOID pWlxContext)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
VOID WINAPI
WlxDisconnectNotify(
	PVOID pWlxContext)
{
  UNIMPLEMENTED;
  return;
}


/*
 * @unimplemented
 */
BOOL WINAPI
WlxGetConsoleSwitchCredentials(
	PVOID pWlxContext,
	PVOID pCredInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}


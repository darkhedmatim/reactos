/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
/* $Id: display.c,v 1.7 2003/07/21 01:59:51 royce Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/misc/dde.c
 * PURPOSE:         DDE
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>
#include <debug.h>
#include <rosrtl/devmode.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
WINBOOL STDCALL
EnumDisplayDevicesA(
  LPCSTR lpDevice,
  DWORD iDevNum,
  PDISPLAY_DEVICE lpDisplayDevice,
  DWORD dwFlags)
{
  WINBOOL rc;
  UNICODE_STRING Device;
  if ( !RtlCreateUnicodeStringFromAsciiz ( &Device, (PCSZ)lpDevice ) )
    {
      SetLastError ( ERROR_OUTOFMEMORY );
      return FALSE;
    }

  rc = NtUserEnumDisplayDevices (
    &Device,
    iDevNum,
    lpDisplayDevice,
    dwFlags );

  RtlFreeUnicodeString ( &Device );

  return rc;
}


/*
 * @implemented
 */
WINBOOL
STDCALL
EnumDisplayDevicesW(
  LPCWSTR lpDevice,
  DWORD iDevNum,
  PDISPLAY_DEVICE lpDisplayDevice,
  DWORD dwFlags)
{
  UNICODE_STRING Device;
  WINBOOL rc;

  RtlInitUnicodeString ( &Device, lpDevice );

  rc = NtUserEnumDisplayDevices (
    &Device,
    iDevNum,
    lpDisplayDevice,
    dwFlags );

  RtlFreeUnicodeString ( &Device );

  return rc;
}


/*
 * @implemented
 */
WINBOOL
STDCALL
EnumDisplayMonitors(
  HDC hdc,
  LPCRECT lprcClip,
  MONITORENUMPROC lpfnEnum,
  LPARAM dwData)
{
  return NtUserEnumDisplayMonitors ( hdc, lprcClip, lpfnEnum, dwData );
}


/*
 * @implemented
 */
WINBOOL
STDCALL
EnumDisplaySettingsExA(
  LPCSTR lpszDeviceName,
  DWORD iModeNum,
  LPDEVMODEA lpDevMode,
  DWORD dwFlags)
{
  WINBOOL rc;
  UNICODE_STRING DeviceName;
  DEVMODEW DevModeW;

  if ( !RtlCreateUnicodeStringFromAsciiz ( &DeviceName, (PCSZ)lpszDeviceName ) )
    {
      SetLastError ( ERROR_OUTOFMEMORY );
      return FALSE;
    }

  RosRtlDevModeA2W ( &DevModeW, lpDevMode );

  rc = NtUserEnumDisplaySettings ( &DeviceName, iModeNum, &DevModeW, dwFlags );

  RtlFreeUnicodeString ( &DeviceName );

  return rc;
}


/*
 * @implemented
 */
WINBOOL
STDCALL
EnumDisplaySettingsA(
  LPCSTR lpszDeviceName,
  DWORD iModeNum,
  LPDEVMODEA lpDevMode)
{
	return EnumDisplaySettingsExA ( lpszDeviceName, iModeNum, lpDevMode, 0 );
}


/*
 * @implemented
 */
WINBOOL
STDCALL
EnumDisplaySettingsExW(
  LPCWSTR lpszDeviceName,
  DWORD iModeNum,
  LPDEVMODEW lpDevMode,
  DWORD dwFlags)
{
  WINBOOL rc;
  UNICODE_STRING DeviceName;

  RtlInitUnicodeString ( &DeviceName, lpszDeviceName );

  rc = NtUserEnumDisplaySettings ( &DeviceName, iModeNum, lpDevMode, dwFlags );

  RtlFreeUnicodeString ( &DeviceName );

  return rc;
}


/*
 * @implemented
 */
WINBOOL
STDCALL
EnumDisplaySettingsW(
  LPCWSTR lpszDeviceName,
  DWORD iModeNum,
  LPDEVMODEW lpDevMode)
{
	return EnumDisplaySettingsExW ( lpszDeviceName, iModeNum, lpDevMode, 0 );
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
GetMonitorInfoA(
  HMONITOR hMonitor,
  LPMONITORINFO lpmi)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
GetMonitorInfoW(
  HMONITOR hMonitor,
  LPMONITORINFO lpmi)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
LONG
STDCALL
ChangeDisplaySettingsExA(
  LPCSTR lpszDeviceName,
  LPDEVMODEA lpDevMode,
  HWND hwnd,
  DWORD dwflags,
  LPVOID lParam)
{
  LONG rc;
  UNICODE_STRING DeviceName;
  DEVMODEW DevModeW;

  if ( !RtlCreateUnicodeStringFromAsciiz ( &DeviceName, (PCSZ)lpszDeviceName ) )
    {
      SetLastError ( ERROR_OUTOFMEMORY );
      return DISP_CHANGE_BADPARAM; /* FIXME what to return? */
    }

  RosRtlDevModeA2W ( &DevModeW, lpDevMode );

  rc = NtUserChangeDisplaySettings ( &DeviceName, &DevModeW, hwnd, dwflags, lParam );

  RtlFreeUnicodeString ( &DeviceName );

  return rc;
}


/*
 * @implemented
 */
LONG
STDCALL
ChangeDisplaySettingsA(
  LPDEVMODEA lpDevMode,
  DWORD dwflags)
{
  return ChangeDisplaySettingsExA ( NULL, lpDevMode, NULL, dwflags, 0 );
}


/*
 * @implemented
 */
LONG
STDCALL
ChangeDisplaySettingsExW(
  LPCWSTR lpszDeviceName,
  LPDEVMODEW lpDevMode,
  HWND hwnd,
  DWORD dwflags,
  LPVOID lParam)
{
  LONG rc;
  UNICODE_STRING DeviceName;

  RtlInitUnicodeString ( &DeviceName, lpszDeviceName );

  rc = NtUserChangeDisplaySettings ( &DeviceName, lpDevMode, hwnd, dwflags, lParam );

  RtlFreeUnicodeString ( &DeviceName );

  return rc;
}


/*
 * @implemented
 */
LONG
STDCALL
ChangeDisplaySettingsW(
  LPDEVMODEW lpDevMode,
  DWORD dwflags)
{
  return ChangeDisplaySettingsExW ( NULL, lpDevMode, NULL, dwflags, 0 );
}

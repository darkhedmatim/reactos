/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Win32 Graphical Subsystem (WIN32K)
 * FILE:            include/win32k/rosuser.h
 * PURPOSE:         Win32 Shared USER Types for RosUser*
 * PROGRAMMER:      Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES ******************************************************************/

#ifndef __WIN32K_ROSUSER_H
#define __WIN32K_ROSUSER_H

/* DEFINES *******************************************************************/

/* ENUMERATIONS **************************************************************/

/* TYPES *********************************************************************/

/* FUNCTIONS *****************************************************************/

#if 0
BOOL
NTAPI
RosUserEnumDisplayMonitors(
    HDC hdc,
    LPRECT rect,
    MONITORENUMPROC proc,
    LPARAM lp
);

BOOL
NTAPI
RosUserGetMonitorInfo(
    HMONITOR handle,
    LPMONITORINFO info
);
#endif

BOOL NTAPI
RosUserGetCursorPos( LPPOINT pt );

BOOL NTAPI
RosUserSetCursorPos( INT x, INT y );

BOOL NTAPI
RosUserClipCursor( LPCRECT clip );

void NTAPI
RosUserSetCursor( ICONINFO* IconInfo );

VOID APIENTRY
RosUserCreateCursorIcon(ICONINFO* IconInfoUnsafe,
                        HCURSOR Handle);

VOID APIENTRY
RosUserDestroyCursorIcon(ICONINFO* IconInfoUnsafe,
                         HCURSOR Handle);

LONG
APIENTRY
RosUserChangeDisplaySettings(
   PUNICODE_STRING lpszDeviceName,
   LPDEVMODEW lpDevMode,
   HWND hwnd,
   DWORD dwflags,
   LPVOID lParam);

INT
APIENTRY
RosUserEnumDisplayMonitors(
   OPTIONAL OUT HMONITOR *hMonitorList,
   OPTIONAL OUT PRECTL monitorRectList,
   OPTIONAL IN DWORD listSize);

NTSTATUS
APIENTRY
RosUserEnumDisplaySettings(
   PUNICODE_STRING pusDeviceName,
   DWORD iModeNum,
   LPDEVMODEW lpDevMode,
   DWORD dwFlags );

BOOL
APIENTRY
RosUserGetMonitorInfo(
   IN HMONITOR hMonitor,
   OUT LPMONITORINFO pMonitorInfo);

HKL 
APIENTRY
RosUserGetKeyboardLayout(
   DWORD dwThreadId);

BOOL
APIENTRY
RosUserGetKeyboardLayoutName(
   LPWSTR lpszName);

HKL
APIENTRY
RosUserLoadKeyboardLayoutEx(
   IN HANDLE Handle,
   IN DWORD offTable,
   IN PUNICODE_STRING puszKeyboardName,
   IN HKL hKL,
   IN PUNICODE_STRING puszKLID,
   IN DWORD dwKLID,
   IN UINT Flags);

HKL
APIENTRY
RosUserActivateKeyboardLayout(
   HKL hKl,
   ULONG Flags);

BOOL
APIENTRY
RosUserUnloadKeyboardLayout(
   HKL hKl);

DWORD
APIENTRY
RosUserVkKeyScanEx(
   WCHAR wChar,
   HKL hKeyboardLayout,
   BOOL UsehKL );

DWORD
APIENTRY
RosUserGetKeyNameText( LONG lParam, LPWSTR lpString, int nSize );

int
APIENTRY
RosUserToUnicodeEx(
   UINT wVirtKey,
   UINT wScanCode,
   PBYTE lpKeyState,
   LPWSTR pwszBuff,
   int cchBuff,
   UINT wFlags,
   HKL dwhkl );

UINT
APIENTRY
RosUserMapVirtualKeyEx( UINT Code, UINT Type, DWORD keyboardId, HKL dwhkl );

SHORT
APIENTRY
RosUserGetAsyncKeyState(
   INT key);

BOOL
APIENTRY
RosUserSetAsyncKeyboardState(BYTE key_state_table[]);

BOOL
APIENTRY
RosUserGetAsyncKeyboardState(BYTE key_state_table[]);

VOID NTAPI
RosUserConnectCsrss(VOID);

VOID NTAPI
SwmAddWindow(HWND hWnd, RECT *WindowRect);

VOID NTAPI
SwmAddDesktopWindow(HWND hWnd, UINT Width, UINT Height);

BOOL NTAPI
SwmDefineCursor(HWND hWnd, HCURSOR hCursor);

VOID NTAPI
SwmRemoveWindow(HWND hWnd);

VOID NTAPI
SwmSetForeground(HWND hWnd);

VOID NTAPI
SwmPosChanging(HWND hWnd, const RECT *WindowRect);

VOID NTAPI
SwmPosChanged(HWND hWnd, const RECT *WindowRect, const RECT *OldRect, HWND hWndAfter, UINT SwpFlags);

HWND NTAPI
SwmGetWindowFromPoint(LONG x, LONG y);

VOID NTAPI
SwmShowWindow(HWND hWnd, BOOLEAN Show, UINT SwpFlags);

#endif /* __WIN32K_NTUSER_H */

#ifndef __WIN32K_CARET_H
#define __WIN32K_CARET_H

#include <ddk/ntddk.h>
#include <napi/win32.h>

#define IDCARETTIMER (0xffff)
#define ThrdCaretInfo(x) (PTHRDCARETINFO)((PW32THREAD)(x + 1))

/* a copy of this structure is in lib/user32/include/user32.h */
typedef struct _THRDCARETINFO
{
  HWND hWnd;
  HBITMAP Bitmap;
  POINT Pos;
  SIZE Size;
  BYTE Visible;
  BYTE Showing;
} THRDCARETINFO, *PTHRDCARETINFO;

BOOL FASTCALL
IntDestroyCaret(PW32THREAD Win32Thread);

BOOL FASTCALL
IntSetCaretBlinkTime(UINT uMSeconds);

BOOL FASTCALL
IntSetCaretPos(int X, int Y);

BOOL FASTCALL
IntSwitchCaretShowing(PVOID Info);

VOID FASTCALL
IntDrawCaret(HWND hWnd);

#endif /* __WIN32K_CARET_H */

/* EOF */

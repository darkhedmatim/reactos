#ifndef _WINABLE_H
#define _WINABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#if (_WIN32_WINNT < 0x0403)
#define INPUT_MOUSE 0x00000000
#define INPUT_KEYBOARD 0x00000001
#define INPUT_HARDWARE 0x00000002
#endif /* (_WIN32_WINNT < 0x04030) */
#if (WINVER < 0x0500)
#define CHILDID_SELF 0
#define OBJID_WINDOW 0x00000000
#define OBJID_SYSMENU 0xFFFFFFFF
#define OBJID_TITLEBAR 0xFFFFFFFE
#define OBJID_MENU 0xFFFFFFFD
#define OBJID_CLIENT 0xFFFFFFFC
#define OBJID_VSCROLL 0xFFFFFFFB
#define OBJID_HSCROLL 0xFFFFFFFA
#define OBJID_SIZEGRIP 0xFFFFFFF9
#define OBJID_CARET 0xFFFFFFF8
#define OBJID_CURSOR 0xFFFFFFF7
#define OBJID_ALERT 0xFFFFFFF6
#define OBJID_SOUND 0xFFFFFFF5
#define GUI_CARETBLINKING 0x00000001
#define GUI_INMOVESIZE 0x00000002
#define GUI_INMENUMODE 0x00000004
#define GUI_SYSTEMMENUMODE 0x00000008
#define GUI_POPUPMENUMODE 0x00000010
#define WINEVENT_OUTOFCONTEXT 0x0000
#define WINEVENT_SKIPOWNTHREAD 0x0001
#define WINEVENT_SKIPOWNPROCESS 0x0002
#define WINEVENT_INCONTEXT 0x0004
#endif /* (WINVER < 0x0500) */
#if (_WIN32_WINNT < 0x0403)
typedef struct tagMOUSEINPUT {
	LONG dx;
	LONG dy;
	DWORD mouseData;
	DWORD dwFlags;
	DWORD time;
	ULONG_PTR dwExtraInfo;
} MOUSEINPUT,*PMOUSEINPUT;
typedef struct tagKEYBDINPUT {
	WORD wVk;
	WORD wScan;
	DWORD dwFlags;
	DWORD time;
	ULONG_PTR dwExtraInfo;
} KEYBDINPUT,*PKEYBDINPUT;
typedef struct tagHARDWAREINPUT {
	DWORD uMsg;
	WORD wParamL;
	WORD wParamH;
} HARDWAREINPUT,*PHARDWAREINPUT;
typedef struct tagINPUT {
	DWORD type;
	_ANONYMOUS_UNION union {
		MOUSEINPUT mi;
		KEYBDINPUT ki;
		HARDWAREINPUT hi;
	} DUMMYUNIONNAME;
} INPUT,*PINPUT,*LPINPUT;
#endif /* (_WIN32_WINNT < 0x0403) */
#if (WINVER < 0x0500)
typedef VOID (*WINEVENTPROC)(HWINEVENTHOOK,DWORD,HWND,LONG,LONG,DWORD,DWORD);
typedef struct tagGUITHREADINFO {
	DWORD cbSize;
	DWORD flags;
	HWND hwndActive;
	HWND hwndFocus;
	HWND hwndCapture;
	HWND hwndMenuOwner;
	HWND hwndMoveSize;
	HWND hwndCaret;
	RECT rcCaret;
} GUITHREADINFO,*PGUITHREADINFO,*LPGUITHREADINFO;
#endif /* (WINVER < 0x0500) */

BOOL WINAPI BlockInput(BOOL);
#if (_WIN32_WINNT < 0x0403)
UINT WINAPI SendInput(UINT,LPINPUT,int);
#endif
#if (WINVER < 0x0500)
BOOL WINAPI GetGUIThreadInfo(DWORD,LPGUITHREADINFO);
void WINAPI NotifyWinEvent(DWORD,HWND,LONG,LONG);
HWINEVENTHOOK WINAPI SetWinEventHook(UINT,UINT,HMODULE,WINEVENTPROC,DWORD,DWORD,UINT);
BOOL WINAPI UnhookWinEvent(HWINEVENTHOOK);
#endif /* (WINVER < 0x0500) */

#ifdef __cplusplus
}
#endif
#endif

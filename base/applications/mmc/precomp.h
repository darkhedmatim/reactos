#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <commctrl.h>
#include <tchar.h>

#include "resource.h"

#define WM_USER_CLOSE_CHILD (WM_USER + 1)


/* console.c */

BOOL
RegisterMMCWndClasses(VOID);

VOID
UnregisterMMCWndClasses(VOID);

HWND
CreateConsoleWindow(IN LPCTSTR lpFileName OPTIONAL,
                    int nCmdShow);

/* misc.c */

INT
LengthOfStrResource(IN HINSTANCE hInst,
                    IN UINT uID);

DWORD
LoadAndFormatString(IN HINSTANCE hInstance,
                    IN UINT uID,
                    OUT LPTSTR *lpTarget,
                    ...);

/* mmc.c */

extern HINSTANCE hAppInstance;
extern HANDLE hAppHeap;
extern HWND hwndMainConsole;
extern HWND hwndMDIClient;

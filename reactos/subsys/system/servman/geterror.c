
#include "servman.h"
/* temp file for debugging */

VOID GetError(VOID)
{
    LPVOID lpMsgBuf;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
                  (LPTSTR) &lpMsgBuf,
                   0,
                   NULL );

        MessageBox(NULL, lpMsgBuf, _T("Error!"), MB_OK | MB_ICONERROR);

        LocalFree(lpMsgBuf);
}


VOID DisplayString(PTCHAR Msg)
{
    MessageBox(NULL, Msg, _T("Error!"), MB_OK | MB_ICONERROR);

}

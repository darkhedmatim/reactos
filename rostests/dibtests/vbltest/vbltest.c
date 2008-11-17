/*
 * Tests various blit and blend operations with different src
 * bit depths and scaling where possbile.
 * 
 * Created by Gregor Schneider <grschneider AT gmail DOT com>, November 2008
*/

#include <windows.h>
#include <tchar.h>

#define CURRENT_BMPS 4
#define SCALE 1.5
#define OFFSET 5

HINSTANCE hInst;
TCHAR szWindowClass[] = _T("testclass");

static LRESULT CALLBACK 
WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HBITMAP hbmList[CURRENT_BMPS];

    switch (message)
    {
        case WM_CREATE:
        {
            hbmList[0] = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(100), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
            hbmList[1] = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(400), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
            hbmList[2] = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(800), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
            hbmList[3] = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(2400), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
            break;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc, hdcMem;
            BITMAP bitmap;
            BLENDFUNCTION bfunc;
            int x = 0, y = 0, i;

            hdc = BeginPaint(hWnd, &ps);
            hdcMem = CreateCompatibleDC(hdc);

            bfunc.AlphaFormat = AC_SRC_ALPHA;
            bfunc.BlendFlags = 0;
            bfunc.BlendOp = AC_SRC_OVER;
            bfunc.SourceConstantAlpha = 128;

            for(i = 0; i < CURRENT_BMPS; i++)
            {
                y = 0;
                SelectObject(hdcMem, hbmList[i]);
                GetObject(hbmList[i], sizeof(BITMAP), &bitmap);
                
                /* bit blt */
                BitBlt(hdc, x, y, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);
                y += bitmap.bmHeight + OFFSET;

                /* stretch blt, org size */
                StretchBlt(hdc, x, y, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
                y += bitmap.bmHeight + OFFSET;

                /* stretch blt, scaled */
                StretchBlt(hdc, x, y, bitmap.bmWidth*SCALE, bitmap.bmHeight*SCALE, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
                y += bitmap.bmHeight*SCALE + OFFSET;

                /* transparent blt, transparency: grey */
                TransparentBlt(hdc, x, y, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, 128*256*256+128*256+128);
                y += bitmap.bmHeight + OFFSET;

                /* transparent blt, transparency: grey, scaled */
                TransparentBlt(hdc, x, y, bitmap.bmWidth*SCALE, bitmap.bmHeight*SCALE, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, 128*256*256+128*256+128);
                y += bitmap.bmHeight*SCALE + OFFSET;

                /* alpha blend, org size */
                AlphaBlend(hdc, x, y, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, bfunc);
                y += bitmap.bmHeight + OFFSET;

                /* alpha blend, scaled */
                AlphaBlend(hdc, x, y, bitmap.bmWidth*SCALE, bitmap.bmHeight*SCALE, hdcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight, bfunc);

                x += bitmap.bmWidth*SCALE + OFFSET;
            }

            DeleteDC(hdcMem);
            EndPaint(hWnd, &ps);
            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


static ATOM
MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = WndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = NULL;
    wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName  = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm       = NULL;

    return RegisterClassEx(&wcex);
}


static BOOL
InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance;

   hWnd = CreateWindowEx(0,
                         szWindowClass,
                         _T("Various blit and blend operations"),
                         WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         640,
                         640,
                         NULL,
                         NULL,
                         hInstance,
                         NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}


int WINAPI
_tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
    MSG msg;

    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

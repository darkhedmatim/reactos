/*
 * Unit test suite for imagelist control.
 *
 * Copyright 2004 Michael Stefaniuc
 * Copyright 2002 Mike McCormack for CodeWeavers
 * Copyright 2007 Dmitry Timoshkov
 * Copyright 2009 Owen Rudge for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define COBJMACROS
#define CONST_VTABLE

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "objbase.h"
#include "commctrl.h" /* must be included after objbase.h to get ImageList_Write */
#include "initguid.h"
#include "commoncontrols.h"
#include "shellapi.h"

#include "wine/test.h"
#include "v6util.h"

#undef VISIBLE

#ifdef VISIBLE
#define WAIT Sleep (1000)
#define REDRAW(hwnd) RedrawWindow (hwnd, NULL, 0, RDW_UPDATENOW)
#else
#define WAIT
#define REDRAW(hwnd)
#endif

#define IMAGELIST_MAGIC (('L' << 8) | 'I')

#include "pshpack2.h"
/* Header used by ImageList_Read() and ImageList_Write() */
typedef struct _ILHEAD
{
    USHORT	usMagic;
    USHORT	usVersion;
    WORD	cCurImage;
    WORD	cMaxImage;
    WORD	cGrow;
    WORD	cx;
    WORD	cy;
    COLORREF	bkcolor;
    WORD	flags;
    SHORT	ovls[4];
} ILHEAD;
#include "poppack.h"

static HIMAGELIST (WINAPI *pImageList_Create)(int, int, UINT, int, int);
static int (WINAPI *pImageList_Add)(HIMAGELIST, HBITMAP, HBITMAP);
static BOOL (WINAPI *pImageList_DrawIndirect)(IMAGELISTDRAWPARAMS*);
static BOOL (WINAPI *pImageList_SetImageCount)(HIMAGELIST,UINT);
static HRESULT (WINAPI *pImageList_CoCreateInstance)(REFCLSID,const IUnknown *,
    REFIID,void **);
static HRESULT (WINAPI *pHIMAGELIST_QueryInterface)(HIMAGELIST,REFIID,void **);

static HINSTANCE hinst;

/* These macros build cursor/bitmap data in 4x4 pixel blocks */
#define B(x,y) ((x?0xf0:0)|(y?0xf:0))
#define ROW1(a,b,c,d,e,f,g,h) B(a,b),B(c,d),B(e,f),B(g,h)
#define ROW32(a,b,c,d,e,f,g,h) ROW1(a,b,c,d,e,f,g,h), ROW1(a,b,c,d,e,f,g,h), \
  ROW1(a,b,c,d,e,f,g,h), ROW1(a,b,c,d,e,f,g,h)
#define ROW2(a,b,c,d,e,f,g,h,i,j,k,l) ROW1(a,b,c,d,e,f,g,h),B(i,j),B(k,l)
#define ROW48(a,b,c,d,e,f,g,h,i,j,k,l) ROW2(a,b,c,d,e,f,g,h,i,j,k,l), \
  ROW2(a,b,c,d,e,f,g,h,i,j,k,l), ROW2(a,b,c,d,e,f,g,h,i,j,k,l), \
  ROW2(a,b,c,d,e,f,g,h,i,j,k,l)

static const BYTE empty_bits[48*48/8];

static const BYTE icon_bits[32*32/8] =
{
  ROW32(0,0,0,0,0,0,0,0),
  ROW32(0,0,1,1,1,1,0,0),
  ROW32(0,1,1,1,1,1,1,0),
  ROW32(0,1,1,0,0,1,1,0),
  ROW32(0,1,1,0,0,1,1,0),
  ROW32(0,1,1,1,1,1,1,0),
  ROW32(0,0,1,1,1,1,0,0),
  ROW32(0,0,0,0,0,0,0,0)
};

static const BYTE bitmap_bits[48*48/8] =
{
  ROW48(0,0,0,0,0,0,0,0,0,0,0,0),
  ROW48(0,1,1,1,1,1,1,1,1,1,1,0),
  ROW48(0,1,1,0,0,0,0,0,0,1,1,0),
  ROW48(0,1,0,0,0,0,0,0,1,0,1,0),
  ROW48(0,1,0,0,0,0,0,1,0,0,1,0),
  ROW48(0,1,0,0,0,0,1,0,0,0,1,0),
  ROW48(0,1,0,0,0,1,0,0,0,0,1,0),
  ROW48(0,1,0,0,1,0,0,0,0,0,1,0),
  ROW48(0,1,0,1,0,0,0,0,0,0,1,0),
  ROW48(0,1,1,0,0,0,0,0,0,1,1,0),
  ROW48(0,1,1,1,1,1,1,1,1,1,1,0),
  ROW48(0,0,0,0,0,0,0,0,0,0,0,0)
};

static HIMAGELIST createImageList(int cx, int cy)
{
    /* Create an ImageList and put an image into it */
    HIMAGELIST himl = ImageList_Create(cx, cy, ILC_COLOR, 1, 1);
    HBITMAP hbm = CreateBitmap(48, 48, 1, 1, bitmap_bits);
    ImageList_Add(himl, hbm, NULL);
    return himl;
}

static HWND create_a_window(void)
{
    char className[] = "bmwnd";
    char winName[]   = "Test Bitmap";
    HWND hWnd;
    static int registered = 0;

    if (!registered)
    {
        WNDCLASSA cls;

        cls.style         = CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
        cls.lpfnWndProc   = DefWindowProcA;
        cls.cbClsExtra    = 0;
        cls.cbWndExtra    = 0;
        cls.hInstance     = 0;
        cls.hIcon         = LoadIconA (0, IDI_APPLICATION);
        cls.hCursor       = LoadCursorA (0, IDC_ARROW);
        cls.hbrBackground = GetStockObject (WHITE_BRUSH);
        cls.lpszMenuName  = 0;
        cls.lpszClassName = className;

        RegisterClassA (&cls);
        registered = 1;
    }

    /* Setup window */
    hWnd = CreateWindowA (className, winName,
       WS_OVERLAPPEDWINDOW ,
       CW_USEDEFAULT, CW_USEDEFAULT, 300, 300, 0,
       0, hinst, 0);

#ifdef VISIBLE
    ShowWindow (hWnd, SW_SHOW);
#endif
    REDRAW(hWnd);
    WAIT;

    return hWnd;
}

static HDC show_image(HWND hwnd, HIMAGELIST himl, int idx, int size,
                      LPCSTR loc, BOOL clear)
{
    HDC hdc = NULL;
#ifdef VISIBLE
    if (!himl) return NULL;

    SetWindowText(hwnd, loc);
    hdc = GetDC(hwnd);
    ImageList_Draw(himl, idx, hdc, 0, 0, ILD_TRANSPARENT);

    REDRAW(hwnd);
    WAIT;

    if (clear)
    {
        BitBlt(hdc, 0, 0, size, size, hdc, size+1, size+1, SRCCOPY);
        ReleaseDC(hwnd, hdc);
        hdc = NULL;
    }
#endif /* VISIBLE */
    return hdc;
}

/* Useful for checking differences */
#if 0
static void dump_bits(const BYTE *p, const BYTE *q, int size)
{
  int i, j;

  size /= 8;

  for (i = 0; i < size * 2; i++)
  {
      printf("|");
      for (j = 0; j < size; j++)
          printf("%c%c", p[j] & 0xf0 ? 'X' : ' ', p[j] & 0xf ? 'X' : ' ');
      printf(" -- ");
      for (j = 0; j < size; j++)
          printf("%c%c", q[j] & 0xf0 ? 'X' : ' ', q[j] & 0xf ? 'X' : ' ');
      printf("|\n");
      p += size * 4;
      q += size * 4;
  }
  printf("\n");
}
#endif

static void check_bits(HWND hwnd, HIMAGELIST himl, int idx, int size,
                       const BYTE *checkbits, LPCSTR loc)
{
#ifdef VISIBLE
    BYTE bits[100*100/8];
    COLORREF c;
    HDC hdc;
    int x, y, i = -1;

    if (!himl) return;

    memset(bits, 0, sizeof(bits));
    hdc = show_image(hwnd, himl, idx, size, loc, FALSE);

    c = GetPixel(hdc, 0, 0);

    for (y = 0; y < size; y ++)
    {
        for (x = 0; x < size; x++)
        {
            if (!(x & 0x7)) i++;
            if (GetPixel(hdc, x, y) != c) bits[i] |= (0x80 >> (x & 0x7));
        }
    }

    BitBlt(hdc, 0, 0, size, size, hdc, size+1, size+1, SRCCOPY);
    ReleaseDC(hwnd, hdc);

    ok (memcmp(bits, checkbits, (size * size)/8) == 0,
        "%s: bits different\n", loc);
    if (memcmp(bits, checkbits, (size * size)/8))
        dump_bits(bits, checkbits, size);
#endif /* VISIBLE */
}

static void testHotspot (void)
{
    struct hotspot {
        int dx;
        int dy;
    };

#define SIZEX1 47
#define SIZEY1 31
#define SIZEX2 11
#define SIZEY2 17
#define HOTSPOTS_MAX 4       /* Number of entries in hotspots */
    static const struct hotspot hotspots[HOTSPOTS_MAX] = {
        { 10, 7 },
        { SIZEX1, SIZEY1 },
        { -9, -8 },
        { -7, 35 }
    };
    int i, j, ret;
    HIMAGELIST himl1 = createImageList(SIZEX1, SIZEY1);
    HIMAGELIST himl2 = createImageList(SIZEX2, SIZEY2);
    HWND hwnd = create_a_window();


    for (i = 0; i < HOTSPOTS_MAX; i++) {
        for (j = 0; j < HOTSPOTS_MAX; j++) {
            int dx1 = hotspots[i].dx;
            int dy1 = hotspots[i].dy;
            int dx2 = hotspots[j].dx;
            int dy2 = hotspots[j].dy;
            int correctx, correcty, newx, newy;
            char loc[256];
            HIMAGELIST himlNew;
            POINT ppt;

            ret = ImageList_BeginDrag(himl1, 0, dx1, dy1);
            ok(ret != 0, "BeginDrag failed for { %d, %d }\n", dx1, dy1);
            sprintf(loc, "BeginDrag (%d,%d)\n", i, j);
            show_image(hwnd, himl1, 0, max(SIZEX1, SIZEY1), loc, TRUE);

            /* check merging the dragged image with a second image */
            ret = ImageList_SetDragCursorImage(himl2, 0, dx2, dy2);
            ok(ret != 0, "SetDragCursorImage failed for {%d, %d}{%d, %d}\n",
                    dx1, dy1, dx2, dy2);
            sprintf(loc, "SetDragCursorImage (%d,%d)\n", i, j);
            show_image(hwnd, himl2, 0, max(SIZEX2, SIZEY2), loc, TRUE);

            /* check new hotspot, it should be the same like the old one */
            himlNew = ImageList_GetDragImage(NULL, &ppt);
            ok(ppt.x == dx1 && ppt.y == dy1,
                    "Expected drag hotspot [%d,%d] got [%d,%d]\n",
                    dx1, dy1, ppt.x, ppt.y);
            /* check size of new dragged image */
            ImageList_GetIconSize(himlNew, &newx, &newy);
            correctx = max(SIZEX1, max(SIZEX2 + dx2, SIZEX1 - dx2));
            correcty = max(SIZEY1, max(SIZEY2 + dy2, SIZEY1 - dy2));
            ok(newx == correctx && newy == correcty,
                    "Expected drag image size [%d,%d] got [%d,%d]\n",
                    correctx, correcty, newx, newy);
            sprintf(loc, "GetDragImage (%d,%d)\n", i, j);
            show_image(hwnd, himlNew, 0, max(correctx, correcty), loc, TRUE);
            ImageList_EndDrag();
        }
    }
#undef SIZEX1
#undef SIZEY1
#undef SIZEX2
#undef SIZEY2
#undef HOTSPOTS_MAX
    ImageList_Destroy(himl2);
    ImageList_Destroy(himl1);
    DestroyWindow(hwnd);
}

static BOOL DoTest1(void)
{
    HIMAGELIST himl ;

    HICON hicon1 ;
    HICON hicon2 ;
    HICON hicon3 ;

    /* create an imagelist to play with */
    himl = ImageList_Create(84, 84, ILC_COLOR16, 0, 3);
    ok(himl!=0,"failed to create imagelist\n");

    /* load the icons to add to the image list */
    hicon1 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon1 != 0, "no hicon1\n");
    hicon2 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon2 != 0, "no hicon2\n");
    hicon3 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon3 != 0, "no hicon3\n");

    /* remove when nothing exists */
    ok(!ImageList_Remove(himl,0),"removed nonexistent icon\n");
    /* removing everything from an empty imagelist should succeed */
    ok(ImageList_RemoveAll(himl),"removed nonexistent icon\n");

    /* add three */
    ok(0==ImageList_AddIcon(himl, hicon1),"failed to add icon1\n");
    ok(1==ImageList_AddIcon(himl, hicon2),"failed to add icon2\n");
    ok(2==ImageList_AddIcon(himl, hicon3),"failed to add icon3\n");

    /* remove an index out of range */
    ok(!ImageList_Remove(himl,4711),"removed nonexistent icon\n");

    /* remove three */
    ok(ImageList_Remove(himl,0),"can't remove 0\n");
    ok(ImageList_Remove(himl,0),"can't remove 0\n");
    ok(ImageList_Remove(himl,0),"can't remove 0\n");

    /* remove one extra */
    ok(!ImageList_Remove(himl,0),"removed nonexistent icon\n");

    /* check SetImageCount/GetImageCount */
    if (pImageList_SetImageCount)
    {
        ok(pImageList_SetImageCount(himl, 3), "couldn't increase image count\n");
        ok(ImageList_GetImageCount(himl) == 3, "invalid image count after increase\n");
        ok(pImageList_SetImageCount(himl, 1), "couldn't decrease image count\n");
        ok(ImageList_GetImageCount(himl) == 1, "invalid image count after decrease to 1\n");
        ok(pImageList_SetImageCount(himl, 0), "couldn't decrease image count\n");
        ok(ImageList_GetImageCount(himl) == 0, "invalid image count after decrease to 0\n");
    }
    else
    {
        skip("skipped ImageList_SetImageCount tests\n");
    }

    /* destroy it */
    ok(ImageList_Destroy(himl),"destroy imagelist failed\n");

    ok(DestroyIcon(hicon1),"icon 1 wasn't deleted\n");
    ok(DestroyIcon(hicon2),"icon 2 wasn't deleted\n");
    ok(DestroyIcon(hicon3),"icon 3 wasn't deleted\n");

    return TRUE;
}

static BOOL DoTest2(void)
{
    HIMAGELIST himl ;

    HICON hicon1 ;
    HICON hicon2 ;
    HICON hicon3 ;

    /* create an imagelist to play with */
    himl = ImageList_Create(84, 84, ILC_COLOR16, 0, 3);
    ok(himl!=0,"failed to create imagelist\n");

    /* load the icons to add to the image list */
    hicon1 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon1 != 0, "no hicon1\n");
    hicon2 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon2 != 0, "no hicon2\n");
    hicon3 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon3 != 0, "no hicon3\n");

    /* add three */
    ok(0==ImageList_AddIcon(himl, hicon1),"failed to add icon1\n");
    ok(1==ImageList_AddIcon(himl, hicon2),"failed to add icon2\n");
    ok(2==ImageList_AddIcon(himl, hicon3),"failed to add icon3\n");

    /* destroy it */
    ok(ImageList_Destroy(himl),"destroy imagelist failed\n");

    ok(DestroyIcon(hicon1),"icon 1 wasn't deleted\n");
    ok(DestroyIcon(hicon2),"icon 2 wasn't deleted\n");
    ok(DestroyIcon(hicon3),"icon 3 wasn't deleted\n");

    return TRUE;
}

static BOOL DoTest3(void)
{
    HIMAGELIST himl;

    HBITMAP hbm1;
    HBITMAP hbm2;
    HBITMAP hbm3;

    IMAGELISTDRAWPARAMS imldp;
    HDC hdc;
    HWND hwndfortest;

    if (!pImageList_DrawIndirect)
    {
        win_skip("ImageList_DrawIndirect not available, skipping test\n");
        return TRUE;
    }

    hwndfortest = create_a_window();
    hdc = GetDC(hwndfortest);
    ok(hdc!=NULL, "couldn't get DC\n");

    /* create an imagelist to play with */
    himl = ImageList_Create(48, 48, ILC_COLOR16, 0, 3);
    ok(himl!=0,"failed to create imagelist\n");

    /* load the icons to add to the image list */
    hbm1 = CreateBitmap(48, 48, 1, 1, bitmap_bits);
    ok(hbm1 != 0, "no bitmap 1\n");
    hbm2 = CreateBitmap(48, 48, 1, 1, bitmap_bits);
    ok(hbm2 != 0, "no bitmap 2\n");
    hbm3 = CreateBitmap(48, 48, 1, 1, bitmap_bits);
    ok(hbm3 != 0, "no bitmap 3\n");

    /* add three */
    ok(0==ImageList_Add(himl, hbm1, 0),"failed to add bitmap 1\n");
    ok(1==ImageList_Add(himl, hbm2, 0),"failed to add bitmap 2\n");

    if (pImageList_SetImageCount)
    {
        ok(pImageList_SetImageCount(himl,3),"Setimage count failed\n");
        /*ok(2==ImageList_Add(himl, hbm3, NULL),"failed to add bitmap 3\n"); */
        ok(ImageList_Replace(himl, 2, hbm3, 0),"failed to replace bitmap 3\n");
    }

    memset(&imldp, 0, sizeof (imldp));
    ok(!pImageList_DrawIndirect(&imldp), "zero data succeeded!\n");
    imldp.cbSize = sizeof (imldp);
    ok(!pImageList_DrawIndirect(&imldp), "zero hdc succeeded!\n");
    imldp.hdcDst = hdc;
    ok(!pImageList_DrawIndirect(&imldp),"zero himl succeeded!\n");
    imldp.himl = himl;
    if (!pImageList_DrawIndirect(&imldp))
    {
      /* Earlier versions of native comctl32 use a smaller structure */
      imldp.cbSize -= 3 * sizeof(DWORD);
      ok(pImageList_DrawIndirect(&imldp),"DrawIndirect should succeed\n");
    }
    REDRAW(hwndfortest);
    WAIT;

    imldp.fStyle = SRCCOPY;
    imldp.rgbBk = CLR_DEFAULT;
    imldp.rgbFg = CLR_DEFAULT;
    imldp.y = 100;
    imldp.x = 100;
    ok(pImageList_DrawIndirect(&imldp),"should succeed\n");
    imldp.i ++;
    ok(pImageList_DrawIndirect(&imldp),"should succeed\n");
    imldp.i ++;
    ok(pImageList_DrawIndirect(&imldp),"should succeed\n");
    imldp.i ++;
    ok(!pImageList_DrawIndirect(&imldp),"should fail\n");

    /* remove three */
    ok(ImageList_Remove(himl, 0), "removing 1st bitmap\n");
    ok(ImageList_Remove(himl, 0), "removing 2nd bitmap\n");
    ok(ImageList_Remove(himl, 0), "removing 3rd bitmap\n");

    /* destroy it */
    ok(ImageList_Destroy(himl),"destroy imagelist failed\n");

    /* bitmaps should not be deleted by the imagelist */
    ok(DeleteObject(hbm1),"bitmap 1 can't be deleted\n");
    ok(DeleteObject(hbm2),"bitmap 2 can't be deleted\n");
    ok(DeleteObject(hbm3),"bitmap 3 can't be deleted\n");

    ReleaseDC(hwndfortest, hdc);
    DestroyWindow(hwndfortest);

    return TRUE;
}

static void testMerge(void)
{
    HIMAGELIST himl1, himl2, hmerge;
    HICON hicon1;
    HWND hwnd = create_a_window();

    himl1 = ImageList_Create(32,32,0,0,3);
    ok(himl1 != NULL,"failed to create himl1\n");

    himl2 = ImageList_Create(32,32,0,0,3);
    ok(himl2 != NULL,"failed to create himl2\n");

    hicon1 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon1 != NULL, "failed to create hicon1\n");

    if (!himl1 || !himl2 || !hicon1)
        return;

    ok(0==ImageList_AddIcon(himl2, hicon1),"add icon1 to himl2 failed\n");
    check_bits(hwnd, himl2, 0, 32, icon_bits, "add icon1 to himl2");

    /* If himl1 has no images, merge still succeeds */
    hmerge = ImageList_Merge(himl1, -1, himl2, 0, 0, 0);
    ok(hmerge != NULL, "merge himl1,-1 failed\n");
    check_bits(hwnd, hmerge, 0, 32, empty_bits, "merge himl1,-1");
    if (hmerge) ImageList_Destroy(hmerge);

    hmerge = ImageList_Merge(himl1, 0, himl2, 0, 0, 0);
    ok(hmerge != NULL,"merge himl1,0 failed\n");
    check_bits(hwnd, hmerge, 0, 32, empty_bits, "merge himl1,0");
    if (hmerge) ImageList_Destroy(hmerge);

    /* Same happens if himl2 is empty */
    ImageList_Destroy(himl2);
    himl2 = ImageList_Create(32,32,0,0,3);
    ok(himl2 != NULL,"failed to recreate himl2\n");
    if (!himl2)
        return;

    hmerge = ImageList_Merge(himl1, -1, himl2, -1, 0, 0);
    ok(hmerge != NULL, "merge himl2,-1 failed\n");
    check_bits(hwnd, hmerge, 0, 32, empty_bits, "merge himl2,-1");
    if (hmerge) ImageList_Destroy(hmerge);

    hmerge = ImageList_Merge(himl1, -1, himl2, 0, 0, 0);
    ok(hmerge != NULL, "merge himl2,0 failed\n");
    check_bits(hwnd, hmerge, 0, 32, empty_bits, "merge himl2,0");
    if (hmerge) ImageList_Destroy(hmerge);

    /* Now try merging an image with itself */
    ok(0==ImageList_AddIcon(himl2, hicon1),"re-add icon1 to himl2 failed\n");

    hmerge = ImageList_Merge(himl2, 0, himl2, 0, 0, 0);
    ok(hmerge != NULL, "merge himl2 with itself failed\n");
    check_bits(hwnd, hmerge, 0, 32, empty_bits, "merge himl2 with itself");
    if (hmerge) ImageList_Destroy(hmerge);

    /* Try merging 2 different image lists */
    ok(0==ImageList_AddIcon(himl1, hicon1),"add icon1 to himl1 failed\n");

    hmerge = ImageList_Merge(himl1, 0, himl2, 0, 0, 0);
    ok(hmerge != NULL, "merge himl1 with himl2 failed\n");
    check_bits(hwnd, hmerge, 0, 32, empty_bits, "merge himl1 with himl2");
    if (hmerge) ImageList_Destroy(hmerge);

    hmerge = ImageList_Merge(himl1, 0, himl2, 0, 8, 16);
    ok(hmerge != NULL, "merge himl1 with himl2 8,16 failed\n");
    check_bits(hwnd, hmerge, 0, 32, empty_bits, "merge himl1 with himl2, 8,16");
    if (hmerge) ImageList_Destroy(hmerge);

    ImageList_Destroy(himl1);
    ImageList_Destroy(himl2);
    DestroyIcon(hicon1);
    DestroyWindow(hwnd);
}

/*********************** imagelist storage test ***************************/

#define BMP_CX 48

struct my_IStream
{
    IStream is;
    char *iml_data; /* written imagelist data */
    ULONG iml_data_size;
};

static HRESULT STDMETHODCALLTYPE Test_Stream_QueryInterface(
    IStream* This,
    REFIID riid,
    void** ppvObject)
{
    assert(0);
    return E_NOTIMPL;
}

static ULONG STDMETHODCALLTYPE Test_Stream_AddRef(
    IStream* This)
{
    assert(0);
    return 2;
}

static ULONG STDMETHODCALLTYPE Test_Stream_Release(
    IStream* This)
{
    assert(0);
    return 1;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_Read(
    IStream* This,
    void* pv,
    ULONG cb,
    ULONG* pcbRead)
{
    assert(0);
    return E_NOTIMPL;
}

static BOOL allocate_storage(struct my_IStream *my_is, ULONG add)
{
    my_is->iml_data_size += add;

    if (!my_is->iml_data)
        my_is->iml_data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, my_is->iml_data_size);
    else
        my_is->iml_data = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, my_is->iml_data, my_is->iml_data_size);

    return my_is->iml_data ? TRUE : FALSE;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_Write(
    IStream* This,
    const void* pv,
    ULONG cb,
    ULONG* pcbWritten)
{
    struct my_IStream *my_is = (struct my_IStream *)This;
    ULONG current_iml_data_size = my_is->iml_data_size;

    if (!allocate_storage(my_is, cb)) return E_FAIL;

    memcpy(my_is->iml_data + current_iml_data_size, pv, cb);
    if (pcbWritten) *pcbWritten = cb;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_Seek(
    IStream* This,
    LARGE_INTEGER dlibMove,
    DWORD dwOrigin,
    ULARGE_INTEGER* plibNewPosition)
{
    assert(0);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_SetSize(
    IStream* This,
    ULARGE_INTEGER libNewSize)
{
    assert(0);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_CopyTo(
    IStream* This,
    IStream* pstm,
    ULARGE_INTEGER cb,
    ULARGE_INTEGER* pcbRead,
    ULARGE_INTEGER* pcbWritten)
{
    assert(0);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_Commit(
    IStream* This,
    DWORD grfCommitFlags)
{
    assert(0);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_Revert(
    IStream* This)
{
    assert(0);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_LockRegion(
    IStream* This,
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType)
{
    assert(0);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_UnlockRegion(
    IStream* This,
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD dwLockType)
{
    assert(0);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_Stat(
    IStream* This,
    STATSTG* pstatstg,
    DWORD grfStatFlag)
{
    assert(0);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE Test_Stream_Clone(
    IStream* This,
    IStream** ppstm)
{
    assert(0);
    return E_NOTIMPL;
}

static const IStreamVtbl Test_Stream_Vtbl =
{
    Test_Stream_QueryInterface,
    Test_Stream_AddRef,
    Test_Stream_Release,
    Test_Stream_Read,
    Test_Stream_Write,
    Test_Stream_Seek,
    Test_Stream_SetSize,
    Test_Stream_CopyTo,
    Test_Stream_Commit,
    Test_Stream_Revert,
    Test_Stream_LockRegion,
    Test_Stream_UnlockRegion,
    Test_Stream_Stat,
    Test_Stream_Clone
};

static struct my_IStream Test_Stream = { { &Test_Stream_Vtbl }, 0, 0 };

static INT DIB_GetWidthBytes( int width, int bpp )
{
    int words;

    switch (bpp)
    {
	case 1:  words = (width + 31) / 32; break;
	case 4:  words = (width + 7) / 8; break;
	case 8:  words = (width + 3) / 4; break;
	case 15:
	case 16: words = (width + 1) / 2; break;
	case 24: words = (width * 3 + 3)/4; break;
	case 32: words = width; break;

        default:
            words=0;
            trace("Unknown depth %d, please report.\n", bpp );
            assert(0);
            break;
    }
    return 4 * words;
}

static void check_bitmap_data(const char *bm_data, ULONG bm_data_size,
                              INT width, INT height, INT bpp,
                              const char *comment)
{
    const BITMAPFILEHEADER *bmfh = (const BITMAPFILEHEADER *)bm_data;
    const BITMAPINFOHEADER *bmih = (const BITMAPINFOHEADER *)(bm_data + sizeof(*bmfh));
    ULONG hdr_size, image_size;

    hdr_size = sizeof(*bmfh) + sizeof(*bmih);
    if (bmih->biBitCount <= 8) hdr_size += (1 << bpp) * sizeof(RGBQUAD);

    ok(bmfh->bfType == (('M' << 8) | 'B'), "wrong bfType 0x%02x\n", bmfh->bfType);
    ok(bmfh->bfSize == hdr_size, "wrong bfSize 0x%02x\n", bmfh->bfSize);
    ok(bmfh->bfReserved1 == 0, "wrong bfReserved1 0x%02x\n", bmfh->bfReserved1);
    ok(bmfh->bfReserved2 == 0, "wrong bfReserved2 0x%02x\n", bmfh->bfReserved2);
    ok(bmfh->bfOffBits == hdr_size, "wrong bfOffBits 0x%02x\n", bmfh->bfOffBits);

    ok(bmih->biSize == sizeof(*bmih), "wrong biSize %d\n", bmih->biSize);
    ok(bmih->biWidth == width, "wrong biWidth %d (expected %d)\n", bmih->biWidth, width);
    ok(bmih->biHeight == height, "wrong biHeight %d (expected %d)\n", bmih->biHeight, height);
    ok(bmih->biPlanes == 1, "wrong biPlanes %d\n", bmih->biPlanes);
    ok(bmih->biBitCount == bpp, "wrong biBitCount %d\n", bmih->biBitCount);

    image_size = DIB_GetWidthBytes(bmih->biWidth, bmih->biBitCount) * bmih->biHeight;
    ok(bmih->biSizeImage == image_size, "wrong biSizeImage %u\n", bmih->biSizeImage);
#if 0
{
    char fname[256];
    FILE *f;
    sprintf(fname, "bmp_%s.bmp", comment);
    f = fopen(fname, "wb");
    fwrite(bm_data, 1, bm_data_size, f);
    fclose(f);
}
#endif
}

static void check_ilhead_data(const char *ilh_data, INT cx, INT cy, INT cur, INT max)
{
    const ILHEAD *ilh = (const ILHEAD *)ilh_data;

    ok(ilh->usMagic == IMAGELIST_MAGIC, "wrong usMagic %4x (expected %02x)\n", ilh->usMagic, IMAGELIST_MAGIC);
    ok(ilh->usVersion == 0x101, "wrong usVersion %x (expected 0x101)\n", ilh->usVersion);
    ok(ilh->cCurImage == cur, "wrong cCurImage %d (expected %d)\n", ilh->cCurImage, cur);
    ok(ilh->cMaxImage == max, "wrong cMaxImage %d (expected %d)\n", ilh->cMaxImage, max);
    ok(ilh->cGrow == 4, "wrong cGrow %d (expected 4)\n", ilh->cGrow);
    ok(ilh->cx == cx, "wrong cx %d (expected %d)\n", ilh->cx, cx);
    ok(ilh->cy == cy, "wrong cy %d (expected %d)\n", ilh->cy, cy);
    ok(ilh->bkcolor == CLR_NONE, "wrong bkcolor %x\n", ilh->bkcolor);
    ok(ilh->flags == ILC_COLOR24, "wrong flags %04x\n", ilh->flags);
    ok(ilh->ovls[0] == -1 ||
       ilh->ovls[0] == 0, /* win95 */
       "wrong ovls[0] %04x\n", ilh->ovls[0]);
    ok(ilh->ovls[1] == -1 ||
       ilh->ovls[1] == 0, /* win95 */
       "wrong ovls[1] %04x\n", ilh->ovls[1]);
    ok(ilh->ovls[2] == -1 ||
       ilh->ovls[2] == 0, /* win95 */
       "wrong ovls[2] %04x\n", ilh->ovls[2]);
    ok(ilh->ovls[3] == -1 ||
       ilh->ovls[3] == 0, /* win95 */
       "wrong ovls[3] %04x\n", ilh->ovls[3]);
}

static HBITMAP create_bitmap(INT cx, INT cy, COLORREF color, const char *comment)
{
    HDC hdc;
    char bmibuf[sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD)];
    BITMAPINFO *bmi = (BITMAPINFO *)bmibuf;
    HBITMAP hbmp, hbmp_old;
    HBRUSH hbrush;
    RECT rc = { 0, 0, cx, cy };

    hdc = CreateCompatibleDC(0);

    memset(bmi, 0, sizeof(*bmi));
    bmi->bmiHeader.biSize = sizeof(bmi->bmiHeader);
    bmi->bmiHeader.biHeight = cx;
    bmi->bmiHeader.biWidth = cy;
    bmi->bmiHeader.biBitCount = 24;
    bmi->bmiHeader.biPlanes = 1;
    bmi->bmiHeader.biCompression = BI_RGB;
    hbmp = CreateDIBSection(hdc, bmi, DIB_RGB_COLORS, NULL, NULL, 0);

    hbmp_old = SelectObject(hdc, hbmp);

    hbrush = CreateSolidBrush(color);
    FillRect(hdc, &rc, hbrush);
    DeleteObject(hbrush);

    DrawText(hdc, comment, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, hbmp_old);
    DeleteDC(hdc);

    return hbmp;
}

static void image_list_init(HIMAGELIST himl)
{
    HBITMAP hbm;
    char comment[16];
    INT n = 1;

#define add_bitmap(grey) \
    sprintf(comment, "%d", n++); \
    hbm = create_bitmap(BMP_CX, BMP_CX, RGB((grey),(grey),(grey)), comment); \
    ImageList_Add(himl, hbm, NULL);

    add_bitmap(255); add_bitmap(170); add_bitmap(85); add_bitmap(0);
    add_bitmap(0); add_bitmap(85); add_bitmap(170); add_bitmap(255);
    add_bitmap(255); add_bitmap(170); add_bitmap(85); add_bitmap(0);
    add_bitmap(0); add_bitmap(85); add_bitmap(170); add_bitmap(255);
    add_bitmap(255); add_bitmap(170); add_bitmap(85); add_bitmap(0);
    add_bitmap(0); add_bitmap(85); add_bitmap(170); add_bitmap(255);
#undef add_bitmap
}

#define iml_clear_stream_data() \
    HeapFree(GetProcessHeap(), 0, Test_Stream.iml_data); \
    Test_Stream.iml_data = NULL; \
    Test_Stream.iml_data_size = 0;

static void check_iml_data(HIMAGELIST himl, INT cx, INT cy, INT cur, INT max,
                           INT width, INT height, INT bpp, const char *comment)
{
    INT ret, cxx, cyy;

    ret = ImageList_GetImageCount(himl);
    ok(ret == cur, "expected cur %d got %d\n", cur, ret);

    ret = ImageList_GetIconSize(himl, &cxx, &cyy);
    ok(ret, "ImageList_GetIconSize failed\n");
    ok(cxx == cx, "wrong cx %d (expected %d)\n", cxx, cx);
    ok(cyy == cy, "wrong cy %d (expected %d)\n", cyy, cy);

    iml_clear_stream_data();
    ret = ImageList_Write(himl, &Test_Stream.is);
    ok(ret, "ImageList_Write failed\n");

    ok(Test_Stream.iml_data != 0, "ImageList_Write didn't write any data\n");
    ok(Test_Stream.iml_data_size > sizeof(ILHEAD), "ImageList_Write wrote not enough data\n");

    check_ilhead_data(Test_Stream.iml_data, cx, cy, cur, max);
    check_bitmap_data(Test_Stream.iml_data + sizeof(ILHEAD),
                      Test_Stream.iml_data_size - sizeof(ILHEAD),
                      width, height, bpp, comment);
}

static void test_imagelist_storage(void)
{
    HIMAGELIST himl;
    BOOL ret;

    himl = ImageList_Create(BMP_CX, BMP_CX, ILC_COLOR24, 1, 1);
    ok(himl != 0, "ImageList_Create failed\n");

    check_iml_data(himl, BMP_CX, BMP_CX, 0, 2, BMP_CX * 4, BMP_CX * 1, 24, "empty");

    image_list_init(himl);
    check_iml_data(himl, BMP_CX, BMP_CX, 24, 27, BMP_CX * 4, BMP_CX * 7, 24, "orig");

    ret = ImageList_Remove(himl, 4);
    ok(ret, "ImageList_Remove failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 23, 27, BMP_CX * 4, BMP_CX * 7, 24, "1");

    ret = ImageList_Remove(himl, 5);
    ok(ret, "ImageList_Remove failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 22, 27, BMP_CX * 4, BMP_CX * 7, 24, "2");

    ret = ImageList_Remove(himl, 6);
    ok(ret, "ImageList_Remove failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 21, 27, BMP_CX * 4, BMP_CX * 7, 24, "3");

    ret = ImageList_Remove(himl, 7);
    ok(ret, "ImageList_Remove failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 20, 27, BMP_CX * 4, BMP_CX * 7, 24, "4");

    ret = ImageList_Remove(himl, -2);
    ok(!ret, "ImageList_Remove(-2) should fail\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 20, 27, BMP_CX * 4, BMP_CX * 7, 24, "5");

    ret = ImageList_Remove(himl, 20);
    ok(!ret, "ImageList_Remove(20) should fail\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 20, 27, BMP_CX * 4, BMP_CX * 7, 24, "6");

    ret = ImageList_Remove(himl, -1);
    ok(ret, "ImageList_Remove(-1) failed\n");
    check_iml_data(himl, BMP_CX, BMP_CX, 0, 4, BMP_CX * 4, BMP_CX * 1, 24, "7");

    ret = ImageList_Destroy(himl);
    ok(ret, "ImageList_Destroy failed\n");

    iml_clear_stream_data();
}

static void test_shell_imagelist(void)
{
    BOOL (WINAPI *pSHGetImageList)(INT, REFIID, void**);
    IImageList *iml = NULL;
    HMODULE hShell32;
    HRESULT hr;
    int out = 0;
    RECT rect;
    int cx, cy;

    /* Try to load function from shell32 */
    hShell32 = LoadLibrary("shell32.dll");
    pSHGetImageList = (void*)GetProcAddress(hShell32, (LPCSTR) 727);

    if (!pSHGetImageList)
    {
        win_skip("SHGetImageList not available, skipping test\n");
        return;
    }

    /* Get system image list */
    hr = (pSHGetImageList)(SHIL_SYSSMALL, &IID_IImageList, (void**)&iml);

    ok(SUCCEEDED(hr), "SHGetImageList failed, hr=%x\n", hr);

    if (hr != S_OK)
        return;

    IImageList_GetImageCount(iml, &out);
    ok(out > 0, "IImageList_GetImageCount returned out <= 0\n");

    /* Fetch the small icon size */
    cx = GetSystemMetrics(SM_CXSMICON);
    cy = GetSystemMetrics(SM_CYSMICON);

    /* Check icon size matches */
    IImageList_GetImageRect(iml, 0, &rect);
    ok(((rect.right == cx) && (rect.bottom == cy)),
                 "IImageList_GetImageRect returned r:%d,b:%d\n",
                 rect.right, rect.bottom);

    IImageList_Release(iml);
    FreeLibrary(hShell32);
}

static HBITMAP create_test_bitmap(HDC hdc, int bpp, UINT32 pixel1, UINT32 pixel2)
{
    HBITMAP hBitmap;
    UINT32 *buffer = NULL;
    BITMAPINFO bitmapInfo = {{sizeof(BITMAPINFOHEADER), 2, 1, 1, bpp, BI_RGB,
                                0, 0, 0, 0, 0}};

    hBitmap = CreateDIBSection(hdc, &bitmapInfo, DIB_RGB_COLORS, (void**)&buffer, NULL, 0);
    ok(hBitmap != NULL && buffer != NULL, "CreateDIBSection failed.\n");

    if(!hBitmap || !buffer)
    {
        DeleteObject(hBitmap);
        return NULL;
    }

    buffer[0] = pixel1;
    buffer[1] = pixel2;

    return hBitmap;
}

static BOOL colour_match(UINT32 x, UINT32 y)
{
    const INT32 tolerance = 8;

    const INT32 dr = abs((INT32)(x & 0x000000FF) - (INT32)(y & 0x000000FF));
    const INT32 dg = abs((INT32)((x & 0x0000FF00) >> 8) - (INT32)((y & 0x0000FF00) >> 8));
    const INT32 db = abs((INT32)((x & 0x00FF0000) >> 16) - (INT32)((y & 0x00FF0000) >> 16));

    return (dr <= tolerance && dg <= tolerance && db <= tolerance);
}

static void check_ImageList_DrawIndirect(IMAGELISTDRAWPARAMS *ildp, UINT32 *bits,
                                         UINT32 expected, int line)
{
    bits[0] = 0x00FFFFFF;
    pImageList_DrawIndirect(ildp);
    ok(colour_match(bits[0], expected),
       "ImageList_DrawIndirect: Pixel %08X, Expected a close match to %08X from line %d\n",
       bits[0] & 0x00FFFFFF, expected, line);
}


static void check_ImageList_DrawIndirect_fStyle(HDC hdc, HIMAGELIST himl, UINT32 *bits, int i,
                                                UINT fStyle, UINT32 expected, int line)
{
    IMAGELISTDRAWPARAMS ildp = {sizeof(IMAGELISTDRAWPARAMS), himl, i, hdc,
        0, 0, 0, 0, 0, 0, CLR_NONE, CLR_NONE, fStyle, 0, ILS_NORMAL, 0, 0x00000000};
    check_ImageList_DrawIndirect(&ildp, bits, expected, line);
}

static void check_ImageList_DrawIndirect_ILD_ROP(HDC hdc, HIMAGELIST himl, UINT32 *bits, int i,
                                                DWORD dwRop, UINT32 expected, int line)
{
    IMAGELISTDRAWPARAMS ildp = {sizeof(IMAGELISTDRAWPARAMS), himl, i, hdc,
        0, 0, 0, 0, 0, 0, CLR_NONE, CLR_NONE, ILD_IMAGE | ILD_ROP, dwRop, ILS_NORMAL, 0, 0x00000000};
    check_ImageList_DrawIndirect(&ildp, bits, expected, line);
}

static void check_ImageList_DrawIndirect_fState(HDC hdc, HIMAGELIST himl, UINT32 *bits, int i, UINT fStyle,
                                                UINT fState, DWORD Frame, UINT32 expected, int line)
{
    IMAGELISTDRAWPARAMS ildp = {sizeof(IMAGELISTDRAWPARAMS), himl, i, hdc,
        0, 0, 0, 0, 0, 0, CLR_NONE, CLR_NONE, fStyle, 0, fState, Frame, 0x00000000};
    check_ImageList_DrawIndirect(&ildp, bits, expected, line);
}

static void check_ImageList_DrawIndirect_broken(HDC hdc, HIMAGELIST himl, UINT32 *bits, int i,
                                                UINT fStyle, UINT fState, DWORD Frame, UINT32 expected,
                                                UINT32 broken_expected, int line)
{
    IMAGELISTDRAWPARAMS ildp = {sizeof(IMAGELISTDRAWPARAMS), himl, i, hdc,
        0, 0, 0, 0, 0, 0, CLR_NONE, CLR_NONE, fStyle, 0, fState, Frame, 0x00000000};
    bits[0] = 0x00FFFFFF;
    pImageList_DrawIndirect(&ildp);
    ok(colour_match(bits[0], expected) ||
       broken(colour_match(bits[0], broken_expected)),
       "ImageList_DrawIndirect: Pixel %08X, Expected a close match to %08X from line %d\n",
       bits[0] & 0x00FFFFFF, expected, line);
}

static void test_ImageList_DrawIndirect(void)
{
    HIMAGELIST himl = NULL;
    int ret;
    HDC hdcDst = NULL;
    HBITMAP hbmOld = NULL, hbmDst = NULL;
    HBITMAP hbmMask = NULL, hbmInverseMask = NULL;
    HBITMAP hbmImage = NULL, hbmAlphaImage = NULL, hbmTransparentImage = NULL;
    int iImage = -1, iAlphaImage = -1, iTransparentImage = -1;
    UINT32 *bits = 0;
    UINT32 maskBits = 0x00000000, inverseMaskBits = 0xFFFFFFFF;

    BITMAPINFO bitmapInfo = {{sizeof(BITMAPINFOHEADER), 2, 1, 1, 32, BI_RGB,
                                0, 0, 0, 0, 0}};

    hdcDst = CreateCompatibleDC(0);
    ok(hdcDst != 0, "CreateCompatibleDC(0) failed to return a valid DC\n");
    if (!hdcDst)
        return;

    hbmMask = CreateBitmap(2, 1, 1, 1, &maskBits);
    ok(hbmMask != 0, "CreateBitmap failed\n");
    if(!hbmMask) goto cleanup;

    hbmInverseMask = CreateBitmap(2, 1, 1, 1, &inverseMaskBits);
    ok(hbmInverseMask != 0, "CreateBitmap failed\n");
    if(!hbmInverseMask) goto cleanup;

    himl = pImageList_Create(2, 1, ILC_COLOR32, 0, 1);
    ok(himl != 0, "ImageList_Create failed\n");
    if(!himl) goto cleanup;

    /* Add a no-alpha image */
    hbmImage = create_test_bitmap(hdcDst, 32, 0x00ABCDEF, 0x00ABCDEF);
    if(!hbmImage) goto cleanup;

    iImage = pImageList_Add(himl, hbmImage, hbmMask);
    ok(iImage != -1, "ImageList_Add failed\n");
    if(iImage == -1) goto cleanup;

    /* Add an alpha image */
    hbmAlphaImage = create_test_bitmap(hdcDst, 32, 0x89ABCDEF, 0x89ABCDEF);
    if(!hbmAlphaImage) goto cleanup;

    iAlphaImage = pImageList_Add(himl, hbmAlphaImage, hbmMask);
    ok(iAlphaImage != -1, "ImageList_Add failed\n");
    if(iAlphaImage == -1) goto cleanup;

    /* Add a transparent alpha image */
    hbmTransparentImage = create_test_bitmap(hdcDst, 32, 0x00ABCDEF, 0x89ABCDEF);
    if(!hbmTransparentImage) goto cleanup;

    iTransparentImage = pImageList_Add(himl, hbmTransparentImage, hbmMask);
    ok(iTransparentImage != -1, "ImageList_Add failed\n");
    if(iTransparentImage == -1) goto cleanup;

    /* 32-bit Tests */
    bitmapInfo.bmiHeader.biBitCount = 32;
    hbmDst = CreateDIBSection(hdcDst, &bitmapInfo, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
    ok (hbmDst && bits, "CreateDIBSection failed to return a valid bitmap and buffer\n");
    if (!hbmDst || !bits)
        goto cleanup;
    hbmOld = SelectObject(hdcDst, hbmDst);

    check_ImageList_DrawIndirect_fStyle(hdcDst, himl, bits, iImage, ILD_NORMAL, 0x00ABCDEF, __LINE__);
    check_ImageList_DrawIndirect_fStyle(hdcDst, himl, bits, iImage, ILD_TRANSPARENT, 0x00ABCDEF, __LINE__);
    todo_wine check_ImageList_DrawIndirect_broken(hdcDst, himl, bits, iAlphaImage, ILD_BLEND25, ILS_NORMAL, 0, 0x00E8F1FA, 0x00D4D9DD, __LINE__);
    todo_wine check_ImageList_DrawIndirect_broken(hdcDst, himl, bits, iAlphaImage, ILD_BLEND50, ILS_NORMAL, 0, 0x00E8F1FA, 0x00B4BDC4, __LINE__);
    check_ImageList_DrawIndirect_fStyle(hdcDst, himl, bits, iImage, ILD_MASK, 0x00ABCDEF, __LINE__);
    check_ImageList_DrawIndirect_fStyle(hdcDst, himl, bits, iImage, ILD_IMAGE, 0x00ABCDEF, __LINE__);
    check_ImageList_DrawIndirect_fStyle(hdcDst, himl, bits, iImage, ILD_PRESERVEALPHA, 0x00ABCDEF, __LINE__);

    check_ImageList_DrawIndirect_fStyle(hdcDst, himl, bits, iAlphaImage, ILD_NORMAL, 0x00D3E5F7, __LINE__);
    check_ImageList_DrawIndirect_fStyle(hdcDst, himl, bits, iAlphaImage, ILD_TRANSPARENT, 0x00D3E5F7, __LINE__);
    todo_wine
    {
        check_ImageList_DrawIndirect_broken(hdcDst, himl, bits, iAlphaImage, ILD_BLEND25, ILS_NORMAL, 0, 0x00E8F1FA, 0x009DA8B1, __LINE__);
        check_ImageList_DrawIndirect_broken(hdcDst, himl, bits, iAlphaImage, ILD_BLEND50, ILS_NORMAL, 0, 0x00E8F1FA, 0x008C99A3, __LINE__);

    }
    check_ImageList_DrawIndirect_fStyle(hdcDst, himl, bits, iAlphaImage, ILD_MASK, 0x00D3E5F7, __LINE__);
    check_ImageList_DrawIndirect_fStyle(hdcDst, himl, bits, iAlphaImage, ILD_IMAGE, 0x00D3E5F7, __LINE__);
    todo_wine check_ImageList_DrawIndirect_fStyle(hdcDst, himl, bits, iAlphaImage, ILD_PRESERVEALPHA, 0x005D6F81, __LINE__);

    check_ImageList_DrawIndirect_fStyle(hdcDst, himl, bits, iTransparentImage, ILD_NORMAL, 0x00FFFFFF, __LINE__);

    check_ImageList_DrawIndirect_ILD_ROP(hdcDst, himl, bits, iImage, SRCCOPY, 0x00ABCDEF, __LINE__);
    check_ImageList_DrawIndirect_ILD_ROP(hdcDst, himl, bits, iImage, SRCINVERT, 0x00543210, __LINE__);

    /* ILD_ROP is ignored when the image has an alpha channel */
    check_ImageList_DrawIndirect_ILD_ROP(hdcDst, himl, bits, iAlphaImage, SRCCOPY, 0x00D3E5F7, __LINE__);
    check_ImageList_DrawIndirect_ILD_ROP(hdcDst, himl, bits, iAlphaImage, SRCINVERT, 0x00D3E5F7, __LINE__);

    todo_wine check_ImageList_DrawIndirect_fState(hdcDst, himl, bits, iImage, ILD_NORMAL, ILS_SATURATE, 0, 0x00CCCCCC, __LINE__);
    todo_wine check_ImageList_DrawIndirect_broken(hdcDst, himl, bits, iAlphaImage, ILD_NORMAL, ILS_SATURATE, 0, 0x00AFAFAF, 0x00F0F0F0, __LINE__);

    check_ImageList_DrawIndirect_fState(hdcDst, himl, bits, iImage, ILD_NORMAL, ILS_GLOW, 0, 0x00ABCDEF, __LINE__);
    check_ImageList_DrawIndirect_fState(hdcDst, himl, bits, iImage, ILD_NORMAL, ILS_SHADOW, 0, 0x00ABCDEF, __LINE__);

    check_ImageList_DrawIndirect_fState(hdcDst, himl, bits, iImage, ILD_NORMAL, ILS_ALPHA, 127, 0x00D5E6F7, __LINE__);
    check_ImageList_DrawIndirect_broken(hdcDst, himl, bits, iAlphaImage, ILD_NORMAL, ILS_ALPHA, 127, 0x00E9F2FB, 0x00AEB7C0, __LINE__);
    todo_wine check_ImageList_DrawIndirect_broken(hdcDst, himl, bits, iAlphaImage, ILD_NORMAL, ILS_NORMAL, 127, 0x00E9F2FB, 0x00D3E5F7, __LINE__);

cleanup:

    if(hbmOld)
        SelectObject(hdcDst, hbmOld);
    if(hbmDst)
        DeleteObject(hbmDst);

    if(hdcDst)
        DeleteDC(hdcDst);

    if(hbmMask)
        DeleteObject(hbmMask);
    if(hbmInverseMask)
        DeleteObject(hbmInverseMask);

    if(hbmImage)
        DeleteObject(hbmImage);
    if(hbmAlphaImage)
        DeleteObject(hbmAlphaImage);
    if(hbmTransparentImage)
        DeleteObject(hbmTransparentImage);

    if(himl)
    {
        ret = ImageList_Destroy(himl);
        ok(ret, "ImageList_Destroy failed\n");
    }
}

static void test_iimagelist(void)
{
    IImageList *imgl;
    HIMAGELIST himl;
    HRESULT hr;
    ULONG ret;

    if (!pHIMAGELIST_QueryInterface)
    {
        win_skip("XP imagelist functions not available\n");
        return;
    }

    /* test reference counting on destruction */
    imgl = (IImageList*)createImageList(32, 32);
    ret = IUnknown_AddRef(imgl);
    ok(ret == 2, "Expected 2, got %d\n", ret);
    ret = ImageList_Destroy((HIMAGELIST)imgl);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ret = ImageList_Destroy((HIMAGELIST)imgl);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ret = ImageList_Destroy((HIMAGELIST)imgl);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);

    imgl = (IImageList*)createImageList(32, 32);
    ret = IUnknown_AddRef(imgl);
    ok(ret == 2, "Expected 2, got %d\n", ret);
    ret = ImageList_Destroy((HIMAGELIST)imgl);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ret = IImageList_Release(imgl);
    ok(ret == 0, "Expected 0, got %d\n", ret);
    ret = ImageList_Destroy((HIMAGELIST)imgl);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);

    if (!pImageList_CoCreateInstance)
    {
        win_skip("Vista imagelist functions not available\n");
        return;
    }

    hr = pImageList_CoCreateInstance(&CLSID_ImageList, NULL, &IID_IImageList, (void **) &imgl);
    ok(SUCCEEDED(hr), "ImageList_CoCreateInstance failed, hr=%x\n", hr);

    if (hr == S_OK)
        IImageList_Release(imgl);

    himl = createImageList(32, 32);

    if (!himl)
        return;

    hr = (pHIMAGELIST_QueryInterface)(himl, &IID_IImageList, (void **) &imgl);
    ok(SUCCEEDED(hr), "HIMAGELIST_QueryInterface failed, hr=%x\n", hr);

    if (hr == S_OK)
        IImageList_Release(imgl);

    ImageList_Destroy(himl);
}

static void testHotspot_v6(void)
{
    struct hotspot {
        int dx;
        int dy;
    };

#define SIZEX1 47
#define SIZEY1 31
#define SIZEX2 11
#define SIZEY2 17
#define HOTSPOTS_MAX 4       /* Number of entries in hotspots */
    static const struct hotspot hotspots[HOTSPOTS_MAX] = {
        { 10, 7 },
        { SIZEX1, SIZEY1 },
        { -9, -8 },
        { -7, 35 }
    };
    int i, j;
    HIMAGELIST himl1 = createImageList(SIZEX1, SIZEY1);
    HIMAGELIST himl2 = createImageList(SIZEX2, SIZEY2);
    IImageList *imgl1, *imgl2;
    HRESULT hr;

    /* cast to IImageList */
    imgl1 = (IImageList *) himl1;
    imgl2 = (IImageList *) himl2;

    for (i = 0; i < HOTSPOTS_MAX; i++) {
        for (j = 0; j < HOTSPOTS_MAX; j++) {
            int dx1 = hotspots[i].dx;
            int dy1 = hotspots[i].dy;
            int dx2 = hotspots[j].dx;
            int dy2 = hotspots[j].dy;
            int correctx, correcty, newx, newy;
            char loc[256];
            IImageList *imglNew;
            POINT ppt;

            hr = IImageList_BeginDrag(imgl1, 0, dx1, dy1);
            ok(SUCCEEDED(hr), "BeginDrag failed for { %d, %d }\n", dx1, dy1);
            sprintf(loc, "BeginDrag (%d,%d)\n", i, j);

            /* check merging the dragged image with a second image */
            hr = IImageList_SetDragCursorImage(imgl2, (IUnknown *) imgl2, 0, dx2, dy2);
            ok(SUCCEEDED(hr), "SetDragCursorImage failed for {%d, %d}{%d, %d}\n",
                    dx1, dy1, dx2, dy2);
            sprintf(loc, "SetDragCursorImage (%d,%d)\n", i, j);

            /* check new hotspot, it should be the same like the old one */
            hr = IImageList_GetDragImage(imgl2, NULL, &ppt, &IID_IImageList, (PVOID *) &imglNew);
            ok(SUCCEEDED(hr), "GetDragImage failed\n");
            ok(ppt.x == dx1 && ppt.y == dy1,
                    "Expected drag hotspot [%d,%d] got [%d,%d]\n",
                    dx1, dy1, ppt.x, ppt.y);
            /* check size of new dragged image */
            IImageList_GetIconSize(imglNew, &newx, &newy);
            correctx = max(SIZEX1, max(SIZEX2 + dx2, SIZEX1 - dx2));
            correcty = max(SIZEY1, max(SIZEY2 + dy2, SIZEY1 - dy2));
            ok(newx == correctx && newy == correcty,
                    "Expected drag image size [%d,%d] got [%d,%d]\n",
                    correctx, correcty, newx, newy);
            sprintf(loc, "GetDragImage (%d,%d)\n", i, j);
            IImageList_EndDrag(imgl2);
        }
    }
#undef SIZEX1
#undef SIZEY1
#undef SIZEX2
#undef SIZEY2
#undef HOTSPOTS_MAX
    IImageList_Release(imgl2);
    IImageList_Release(imgl1);
}

static void DoTest1_v6(void)
{
    IImageList *imgl;
    HIMAGELIST himl;
    HRESULT hr;

    HICON hicon1;
    HICON hicon2;
    HICON hicon3;

    int ret = 0;

    /* create an imagelist to play with */
    himl = ImageList_Create(84, 84, ILC_COLOR16, 0, 3);
    ok(himl != 0,"failed to create imagelist\n");

    imgl = (IImageList *) himl;

    /* load the icons to add to the image list */
    hicon1 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon1 != 0, "no hicon1\n");
    hicon2 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon2 != 0, "no hicon2\n");
    hicon3 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon3 != 0, "no hicon3\n");

    /* remove when nothing exists */
    hr = IImageList_Remove(imgl, 0);
    ok(!(SUCCEEDED(hr)), "removed nonexistent icon\n");

    /* removing everything from an empty imagelist should succeed */
    hr = IImageList_Remove(imgl, -1);
    ok(SUCCEEDED(hr), "removed nonexistent icon\n");

    /* add three */
    ok(SUCCEEDED(IImageList_ReplaceIcon(imgl, -1, hicon1, &ret)) && (ret == 0),"failed to add icon1\n");
    ok(SUCCEEDED(IImageList_ReplaceIcon(imgl, -1, hicon2, &ret)) && (ret == 1),"failed to add icon2\n");
    ok(SUCCEEDED(IImageList_ReplaceIcon(imgl, -1, hicon3, &ret)) && (ret == 2),"failed to add icon3\n");

    /* remove an index out of range */
    ok(FAILED(IImageList_Remove(imgl, 4711)),"removed nonexistent icon\n");

    /* remove three */
    ok(SUCCEEDED(IImageList_Remove(imgl,0)),"can't remove 0\n");
    ok(SUCCEEDED(IImageList_Remove(imgl,0)),"can't remove 0\n");
    ok(SUCCEEDED(IImageList_Remove(imgl,0)),"can't remove 0\n");

    /* remove one extra */
    ok(FAILED(IImageList_Remove(imgl, 0)),"removed nonexistent icon\n");

    /* check SetImageCount/GetImageCount */
    ok(SUCCEEDED(IImageList_SetImageCount(imgl, 3)), "couldn't increase image count\n");
    ok(SUCCEEDED(IImageList_GetImageCount(imgl, &ret)) && (ret == 3), "invalid image count after increase\n");
    ok(SUCCEEDED(IImageList_SetImageCount(imgl, 1)), "couldn't decrease image count\n");
    ok(SUCCEEDED(IImageList_GetImageCount(imgl, &ret)) && (ret == 1), "invalid image count after decrease to 1\n");
    ok(SUCCEEDED(IImageList_SetImageCount(imgl, 0)), "couldn't decrease image count\n");
    ok(SUCCEEDED(IImageList_GetImageCount(imgl, &ret)) && (ret == 0), "invalid image count after decrease to 0\n");

    /* destroy it */
    ok(SUCCEEDED(IImageList_Release(imgl)),"release imagelist failed\n");

    ok(DestroyIcon(hicon1),"icon 1 wasn't deleted\n");
    ok(DestroyIcon(hicon2),"icon 2 wasn't deleted\n");
    ok(DestroyIcon(hicon3),"icon 3 wasn't deleted\n");
}

static void DoTest3_v6(void)
{
    IImageList *imgl;
    HIMAGELIST himl;

    HBITMAP hbm1;
    HBITMAP hbm2;
    HBITMAP hbm3;

    IMAGELISTDRAWPARAMS imldp;
    HWND hwndfortest;
    HDC hdc;
    int ret;

    hwndfortest = create_a_window();
    hdc = GetDC(hwndfortest);
    ok(hdc!=NULL, "couldn't get DC\n");

    /* create an imagelist to play with */
    himl = ImageList_Create(48, 48, ILC_COLOR16, 0, 3);
    ok(himl!=0,"failed to create imagelist\n");

    imgl = (IImageList *) himl;

    /* load the icons to add to the image list */
    hbm1 = CreateBitmap(48, 48, 1, 1, bitmap_bits);
    ok(hbm1 != 0, "no bitmap 1\n");
    hbm2 = CreateBitmap(48, 48, 1, 1, bitmap_bits);
    ok(hbm2 != 0, "no bitmap 2\n");
    hbm3 = CreateBitmap(48, 48, 1, 1, bitmap_bits);
    ok(hbm3 != 0, "no bitmap 3\n");

    /* add three */
    ok(SUCCEEDED(IImageList_Add(imgl, hbm1, 0, &ret)) && (ret == 0), "failed to add bitmap 1\n");
    ok(SUCCEEDED(IImageList_Add(imgl, hbm2, 0, &ret)) && (ret == 1), "failed to add bitmap 2\n");

    ok(SUCCEEDED(IImageList_SetImageCount(imgl, 3)), "Setimage count failed\n");
    ok(SUCCEEDED(IImageList_Replace(imgl, 2, hbm3, 0)), "failed to replace bitmap 3\n");

    memset(&imldp, 0, sizeof (imldp));
    ok(FAILED(IImageList_Draw(imgl, &imldp)), "zero data succeeded!\n");

    imldp.cbSize = sizeof (imldp);
    imldp.hdcDst = hdc;
    imldp.himl = himl;

    if (FAILED(IImageList_Draw(imgl, &imldp)))
    {
       /* Earlier versions of native comctl32 use a smaller structure */
       imldp.cbSize -= 3 * sizeof(DWORD);
       ok(SUCCEEDED(IImageList_Draw(imgl, &imldp)), "should succeed\n");
    }

    REDRAW(hwndfortest);
    WAIT;

    imldp.fStyle = SRCCOPY;
    imldp.rgbBk = CLR_DEFAULT;
    imldp.rgbFg = CLR_DEFAULT;
    imldp.y = 100;
    imldp.x = 100;
    ok(SUCCEEDED(IImageList_Draw(imgl, &imldp)), "should succeed\n");
    imldp.i ++;
    ok(SUCCEEDED(IImageList_Draw(imgl, &imldp)), "should succeed\n");
    imldp.i ++;
    ok(SUCCEEDED(IImageList_Draw(imgl, &imldp)), "should succeed\n");
    imldp.i ++;
    ok(FAILED(IImageList_Draw(imgl, &imldp)), "should fail\n");

    /* remove three */
    ok(SUCCEEDED(IImageList_Remove(imgl, 0)), "removing 1st bitmap\n");
    ok(SUCCEEDED(IImageList_Remove(imgl, 0)), "removing 2nd bitmap\n");
    ok(SUCCEEDED(IImageList_Remove(imgl, 0)), "removing 3rd bitmap\n");

    /* destroy it */
    ok(SUCCEEDED(IImageList_Release(imgl)), "release imagelist failed\n");

    /* bitmaps should not be deleted by the imagelist */
    ok(DeleteObject(hbm1),"bitmap 1 can't be deleted\n");
    ok(DeleteObject(hbm2),"bitmap 2 can't be deleted\n");
    ok(DeleteObject(hbm3),"bitmap 3 can't be deleted\n");

    ReleaseDC(hwndfortest, hdc);
    DestroyWindow(hwndfortest);
}

static void testMerge_v6(void)
{
    HIMAGELIST himl1, himl2;
    IImageList *imgl1, *imgl2, *merge;
    HICON hicon1;
    HWND hwnd = create_a_window();
    HRESULT hr;
    int ret;

    himl1 = ImageList_Create(32,32,0,0,3);
    ok(himl1 != NULL,"failed to create himl1\n");

    himl2 = ImageList_Create(32,32,0,0,3);
    ok(himl2 != NULL,"failed to create himl2\n");

    hicon1 = CreateIcon(hinst, 32, 32, 1, 1, icon_bits, icon_bits);
    ok(hicon1 != NULL, "failed to create hicon1\n");

    if (!himl1 || !himl2 || !hicon1)
        return;

    /* cast to IImageList */
    imgl1 = (IImageList *) himl1;
    imgl2 = (IImageList *) himl2;

    ok(SUCCEEDED(IImageList_ReplaceIcon(imgl2, -1, hicon1, &ret)) && (ret == 0),"add icon1 to himl2 failed\n");

    /* If himl1 has no images, merge still succeeds */
    hr = IImageList_Merge(imgl1, -1, (IUnknown *) imgl2, 0, 0, 0, &IID_IImageList, (void **) &merge);
    ok(SUCCEEDED(hr), "merge himl1,-1 failed\n");
    if (SUCCEEDED(hr)) IImageList_Release(merge);

    hr = IImageList_Merge(imgl1, 0, (IUnknown *) imgl2, 0, 0, 0, &IID_IImageList, (void **) &merge);
    ok(SUCCEEDED(hr), "merge himl1,0 failed\n");
    if (SUCCEEDED(hr)) IImageList_Release(merge);

    /* Same happens if himl2 is empty */
    IImageList_Release(imgl2);
    himl2 = ImageList_Create(32,32,0,0,3);
    ok(himl2 != NULL,"failed to recreate himl2\n");

    imgl2 = (IImageList *) himl2;

    hr = IImageList_Merge(imgl1, -1, (IUnknown *) imgl2, -1, 0, 0, &IID_IImageList, (void **) &merge);
    ok(SUCCEEDED(hr), "merge himl2,-1 failed\n");
    if (SUCCEEDED(hr)) IImageList_Release(merge);

    hr = IImageList_Merge(imgl1, -1, (IUnknown *) imgl2, 0, 0, 0, &IID_IImageList, (void **) &merge);
    ok(SUCCEEDED(hr), "merge himl2,0 failed\n");
    if (SUCCEEDED(hr)) IImageList_Release(merge);

    /* Now try merging an image with itself */
    ok(SUCCEEDED(IImageList_ReplaceIcon(imgl2, -1, hicon1, &ret)) && (ret == 0),"re-add icon1 to himl2 failed\n");

    hr = IImageList_Merge(imgl2, 0, (IUnknown *) imgl2, 0, 0, 0, &IID_IImageList, (void **) &merge);
    ok(SUCCEEDED(hr), "merge himl2 with itself failed\n");
    if (SUCCEEDED(hr)) IImageList_Release(merge);

    /* Try merging 2 different image lists */
    ok(SUCCEEDED(IImageList_ReplaceIcon(imgl1, -1, hicon1, &ret)) && (ret == 0),"add icon1 to himl1 failed\n");

    hr = IImageList_Merge(imgl1, 0, (IUnknown *) imgl2, 0, 0, 0, &IID_IImageList, (void **) &merge);
    ok(SUCCEEDED(hr), "merge himl1 with himl2 failed\n");
    if (SUCCEEDED(hr)) IImageList_Release(merge);

    hr = IImageList_Merge(imgl1, 0, (IUnknown *) imgl2, 0, 8, 16, &IID_IImageList, (void **) &merge);
    ok(SUCCEEDED(hr), "merge himl1 with himl2 8,16 failed\n");
    if (SUCCEEDED(hr)) IImageList_Release(merge);

    IImageList_Release(imgl1);
    IImageList_Release(imgl2);

    DestroyIcon(hicon1);
    DestroyWindow(hwnd);
}

START_TEST(imagelist)
{
    ULONG_PTR ctx_cookie;
    HANDLE hCtx;

    HMODULE hComCtl32 = GetModuleHandle("comctl32.dll");
    pImageList_Create = NULL;   /* These are not needed for non-v6.0 tests*/
    pImageList_Add = NULL;
    pImageList_DrawIndirect = (void*)GetProcAddress(hComCtl32, "ImageList_DrawIndirect");
    pImageList_SetImageCount = (void*)GetProcAddress(hComCtl32, "ImageList_SetImageCount");

    hinst = GetModuleHandleA(NULL);

    InitCommonControls();

    testHotspot();
    DoTest1();
    DoTest2();
    DoTest3();
    testMerge();
    test_imagelist_storage();

    FreeLibrary(hComCtl32);

    /* Now perform v6 tests */

    if (!load_v6_module(&ctx_cookie, &hCtx))
        return;

    /* Reload comctl32 */
    hComCtl32 = LoadLibraryA("comctl32.dll");
    pImageList_Create = (void*)GetProcAddress(hComCtl32, "ImageList_Create");
    pImageList_Add = (void*)GetProcAddress(hComCtl32, "ImageList_Add");
    pImageList_DrawIndirect = (void*)GetProcAddress(hComCtl32, "ImageList_DrawIndirect");
    pImageList_SetImageCount = (void*)GetProcAddress(hComCtl32, "ImageList_SetImageCount");
    pImageList_CoCreateInstance = (void*)GetProcAddress(hComCtl32, "ImageList_CoCreateInstance");
    pHIMAGELIST_QueryInterface = (void*)GetProcAddress(hComCtl32, "HIMAGELIST_QueryInterface");

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    /* Do v6.0 tests */
    test_ImageList_DrawIndirect();
    test_shell_imagelist();
    test_iimagelist();

    testHotspot_v6();
    DoTest1_v6();
    DoTest3_v6();
    testMerge_v6();

    CoUninitialize();

    unload_v6_module(ctx_cookie, hCtx);
}

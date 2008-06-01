#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif


typedef PGDI_TABLE_ENTRY (CALLBACK * GDIQUERYPROC) (void);

/* GDI handle table can hold 0x4000 handles */
#define GDI_HANDLE_COUNT 0x10000
#define GDI_GLOBAL_PROCESS (0x0)

/* Handle Masks and shifts */
#define GDI_HANDLE_INDEX_MASK (GDI_HANDLE_COUNT - 1)
#define GDI_HANDLE_TYPE_MASK  0x007f0000
#define GDI_HANDLE_STOCK_MASK 0x00800000
#define GDI_HANDLE_REUSE_MASK 0xff000000
#define GDI_HANDLE_REUSECNT_SHIFT 24


#define GDI_OBJECT_TYPE_DC          0x00010000
#define GDI_OBJECT_TYPE_REGION      0x00040000
#define GDI_OBJECT_TYPE_BITMAP      0x00050000
#define GDI_OBJECT_TYPE_PALETTE     0x00080000
#define GDI_OBJECT_TYPE_FONT        0x000a0000
#define GDI_OBJECT_TYPE_BRUSH       0x00100000
#define GDI_OBJECT_TYPE_EMF         0x00210000
#define GDI_OBJECT_TYPE_PEN         0x00300000
#define GDI_OBJECT_TYPE_EXTPEN      0x00500000
#define GDI_OBJECT_TYPE_COLORSPACE  0x00090000
#define GDI_OBJECT_TYPE_METADC      0x00660000
#define GDI_OBJECT_TYPE_METAFILE    0x00260000
#define GDI_OBJECT_TYPE_ENHMETAFILE 0x00460000
/* Following object types made up for ROS */
#define GDI_OBJECT_TYPE_ENHMETADC   0x00740000
#define GDI_OBJECT_TYPE_MEMDC       0x00750000
#define GDI_OBJECT_TYPE_DCE         0x00770000
#define GDI_OBJECT_TYPE_DONTCARE    0x007f0000
/** Not really an object type. Forces GDI_FreeObj to be silent. */
#define GDI_OBJECT_TYPE_SILENT      0x80000000



/* Number Representation */

typedef LONG FIX;




HDC WINAPI GdiConvertBitmap(HDC hdc);
HBRUSH WINAPI GdiConvertBrush(HBRUSH hbr);
HDC WINAPI GdiConvertDC(HDC hdc);
HFONT WINAPI GdiConvertFont(HFONT hfont);
HPALETTE WINAPI GdiConvertPalette(HPALETTE hpal);
HRGN WINAPI GdiConvertRegion(HRGN hregion);
HBRUSH WINAPI GdiGetLocalBrush(HBRUSH hbr);
HDC WINAPI GdiGetLocalDC(HDC hdc);
BOOL WINAPI GdiDeleteLocalDC(HDC hdc);
BOOL WINAPI GdiReleaseLocalDC(HDC hdc);
BOOL WINAPI GdiSetAttrs(HDC hdc);




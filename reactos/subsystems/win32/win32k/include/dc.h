#ifndef __WIN32K_DC_H
#define __WIN32K_DC_H

typedef struct _DC
{
    BASEOBJECT    BaseObject;

    PPDEVOBJ     pPDevice;

    PSURFACE     pBitmap;
    PBRUSHGDI    pFillBrush;
    PBRUSHGDI    pLineBrush;
    COLORREF     crForegroundClr;
    COLORREF     crBackgroundClr;
    HPALETTE     hPalette;

    /* Origins and extents */
    RECT         rcDcRect; /* Relative to Vport */
    RECT         rcVport;
    POINT        ptBrushOrg;

    /* Combined clipping region */
    struct region *Clipping;
    CLIPOBJ      *CombinedClip;
    PSWM_WINDOW  pWindow;
} DC, *PDC;

#define  DC_Lock(hDC)  \
  ((PDC) GDIOBJ_LockObj ((HGDIOBJ) hDC, GDI_OBJECT_TYPE_DC))
#define  DC_Unlock(pDC)  \
  GDIOBJ_UnlockObjByPtr ((PBASEOBJECT)pDC)

VOID APIENTRY RosGdiUpdateClipping(PDC pDC);

#endif

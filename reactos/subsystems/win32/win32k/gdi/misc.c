/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         LGPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/gdi/bitmap.c
 * PURPOSE:         ReactOS GDI misc support functions
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

extern PDEVOBJ PrimarySurface;

/* PUBLIC FUNCTIONS **********************************************************/

BOOL APIENTRY RosGdiEllipse( HDC physDev, INT left, INT top, INT right, INT bottom )
{
    PDC pDC;

    /* Get a pointer to the DC */
    pDC = DC_Lock(physDev);

    /* Add DC origin */
    left += pDC->rcVport.left + pDC->rcDcRect.left;
    top += pDC->rcVport.top + pDC->rcDcRect.top;
    right += pDC->rcVport.left + pDC->rcDcRect.left;
    bottom += pDC->rcVport.top + pDC->rcDcRect.top;

    GreEllipse(pDC, left, top, right, bottom);

    /* Release the object */
    DC_Unlock(pDC);

    return TRUE;
}

INT APIENTRY RosGdiExtEscape( HDC physDev, INT escape, INT in_count, LPCVOID in_data,
                            INT out_count, LPVOID out_data )
{
    UNIMPLEMENTED;
    return 0;
}

BOOL APIENTRY RosGdiExtFloodFill( HDC physDev, INT x, INT y, COLORREF color,
                     UINT fillType )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY RosGdiExtTextOut( HDC physDev, INT x, INT y, UINT flags,
                   const RECT *lprect, LPCWSTR wstr, UINT count,
                   const INT *lpDx, gsCacheEntryFormat *formatEntry,
                   AA_Type aa_type)
{
    PDC pDC;

    /* Get a pointer to the DC */
    pDC = DC_Lock(physDev);

    /* Call GRE routine */
    GreTextOut(pDC, x, y, flags, lprect, wstr, count, lpDx, formatEntry, aa_type);

    /* Release the object */
    DC_Unlock(pDC);

    return TRUE;
}

BOOL APIENTRY RosGdiLineTo( HDC physDev, INT x1, INT y1, INT x2, INT y2 )
{
    PDC pDC;
    RECTL scrRect;
    POINT pt[2];

    DPRINT("LineTo: (%d,%d)-(%d,%d)\n", x1, y1, x2, y2);

    /* Get a pointer to the DC */
    pDC = DC_Lock(physDev);

    /* Set points */
    pt[0].x = x1; pt[0].y = y1;
    pt[1].x = x2; pt[1].y = y2;

    /* Add DC origin */
    pt[0].x += pDC->rcVport.left + pDC->rcDcRect.left;
    pt[0].y += pDC->rcVport.top + pDC->rcDcRect.top;
    pt[1].x += pDC->rcVport.left + pDC->rcDcRect.left;
    pt[1].y += pDC->rcVport.top + pDC->rcDcRect.top;

    /* Update the pen */
    GreUpdateBrush(pDC->pLineBrush, pDC);

    GreLineTo(&pDC->pBitmap->SurfObj,
              pDC->CombinedClip,
              &(pDC->pLineBrush->BrushObj),
              pt[0].x, pt[0].y,
              pt[1].x, pt[1].y,
              &scrRect,
              0);

    /* Release the object */
    DC_Unlock(pDC);

    return TRUE;
}

BOOL APIENTRY RosGdiArc( HDC physDev, INT left, INT top, INT right, INT bottom,
            INT xstart, INT ystart, INT xend, INT yend, ARCTYPE arc )
{
    PDC pDC;

    /* Get a pointer to the DC */
    pDC = DC_Lock(physDev);

    /* Add DC origin */
    left += pDC->rcVport.left + pDC->rcDcRect.left;
    top += pDC->rcVport.top + pDC->rcDcRect.top;
    right += pDC->rcVport.left + pDC->rcDcRect.left;
    bottom += pDC->rcVport.top + pDC->rcDcRect.top;
    xstart += pDC->rcVport.left + pDC->rcDcRect.left;
    ystart += pDC->rcVport.top + pDC->rcDcRect.top;
    xend += pDC->rcVport.left + pDC->rcDcRect.left;
    yend += pDC->rcVport.top + pDC->rcDcRect.top;

    GrepArc(pDC, left, top, right, bottom, xstart, ystart, xend, yend, arc);

    /* Release the object */
    DC_Unlock(pDC);

    return TRUE;
}

BOOL APIENTRY RosGdiPolyPolygon( HDC physDev, const POINT* pt, const INT* counts, UINT polygons)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY RosGdiPolyPolyline( HDC physDev, const POINT* pt, const DWORD* counts, DWORD polylines )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY RosGdiPolygon( HDC physDev, const POINT* pUserBuffer, INT count )
{
    PDC pDC;
    NTSTATUS Status = STATUS_SUCCESS;
    POINT pStackBuf[16];
    POINT *pPoints = pStackBuf;
    RECTL rcBound;
    ULONG i;

    /* Get a pointer to the DC */
    pDC = DC_Lock(physDev);

    /* Capture the points buffer */
    _SEH2_TRY
    {
        ProbeForRead(pUserBuffer, count * sizeof(POINT), 1);

        /* Use pool allocated buffer if data doesn't fit */
        if (count > sizeof(*pStackBuf) / sizeof(POINT))
            pPoints = ExAllocatePool(PagedPool, sizeof(POINT) * count);

        /* Copy points data */
        RtlCopyMemory(pPoints, pUserBuffer, count * sizeof(POINT));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        /* Release the object */
        DC_Unlock(pDC);

        /* Free the buffer if it was allocated */
        if (pPoints != pStackBuf) ExFreePool(pPoints);

        /* Return failure */
        return FALSE;
    }

    /* Calculate bounding rect and offset points data */
    pPoints[0].x += pDC->rcDcRect.left + pDC->rcVport.left;
    pPoints[0].y += pDC->rcDcRect.top + pDC->rcVport.top;

    rcBound.left   = pPoints[0].x;
    rcBound.right  = pPoints[0].x;
    rcBound.top    = pPoints[0].y;
    rcBound.bottom = pPoints[0].y;

    for (i=1; i<count; i++)
    {
        pPoints[i].x += pDC->rcDcRect.left + pDC->rcVport.left;
        pPoints[i].y += pDC->rcDcRect.top + pDC->rcVport.top;

        rcBound.left   = min(rcBound.left, pPoints[i].x);
        rcBound.right  = max(rcBound.right, pPoints[i].x);
        rcBound.top    = min(rcBound.top, pPoints[i].y);
        rcBound.bottom = max(rcBound.bottom, pPoints[i].y);
    }

    /* Draw the polygon */
    GrePolygon(pDC, pPoints, count, &rcBound);

    /* Release the object */
    DC_Unlock(pDC);

    /* Free the buffer if it was allocated */
    if (pPoints != pStackBuf) ExFreePool(pPoints);

    /* Return success */
    return TRUE;
}

BOOL APIENTRY RosGdiPolyline( HDC physDev, const POINT* pt, INT count )
{
    PDC pDC;

    /* Get a pointer to the DC */
    pDC = DC_Lock(physDev);

    /* Draw the polyline */
    GrePolyline(pDC, pt, count);

    /* Release the object */
    DC_Unlock(pDC);

    return TRUE;
}

UINT APIENTRY RosGdiRealizeDefaultPalette( HDC physDev )
{
    UNIMPLEMENTED;
    return FALSE;
}

UINT APIENTRY RosGdiRealizePalette( HDC physDev, HPALETTE hpal, BOOL primary )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY RosGdiRectangle(HDC physDev, PRECT rc)
{
    PDC pDC;

    /* Get a pointer to the DC */
    pDC = DC_Lock(physDev);

    /* Draw the rectangle */
    GreRectangle(pDC, rc->left, rc->top, rc->right, rc->bottom);

    /* Release the object */
    DC_Unlock(pDC);

    return TRUE;
}

BOOL APIENTRY RosGdiRoundRect( HDC physDev, INT left, INT top, INT right,
                  INT bottom, INT ell_width, INT ell_height )
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY RosGdiSwapBuffers(HDC physDev)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL APIENTRY RosGdiUnrealizePalette( HPALETTE hpal )
{
    UNIMPLEMENTED;
    return FALSE;
}

/* EOF */

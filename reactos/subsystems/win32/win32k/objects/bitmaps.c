/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$ */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

#define IN_RECT(r,x,y) \
( \
 (x) >= (r).left && \
 (y) >= (r).top && \
 (x) < (r).right && \
 (y) < (r).bottom \
)

HBITMAP APIENTRY
IntGdiCreateBitmap(
    INT  Width,
    INT  Height,
    UINT  Planes,
    UINT  BitsPixel,
    IN OPTIONAL LPBYTE pBits)
{
   HBITMAP hBitmap;
   SIZEL Size;
   LONG WidthBytes;

   /* NOTE: Windows also doesn't store nr. of planes separately! */
   BitsPixel = BITMAPOBJ_GetRealBitsPixel(BitsPixel * Planes);

   /* Check parameters */
   if (BitsPixel == 0 || Width <= 0 || Width >= 0x8000000 || Height == 0)
   {
      DPRINT1("Width = %d, Height = %d BitsPixel = %d\n", Width, Height, BitsPixel);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
   }

   WidthBytes = BITMAPOBJ_GetWidthBytes(Width, BitsPixel);

   Size.cx = Width;
   Size.cy = abs(Height);

   /* Make sure that cjBits will not overflow */
   if ((ULONGLONG)WidthBytes * Size.cy >= 0x100000000ULL)
   {
      DPRINT1("Width = %d, Height = %d BitsPixel = %d\n", Width, Height, BitsPixel);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
   }

   /* Create the bitmap object. */
   hBitmap = IntCreateBitmap(Size, WidthBytes,
                             BitmapFormat(BitsPixel, BI_RGB),
                             (Height < 0 ? BMF_TOPDOWN : 0) |
                             (NULL == pBits ? 0 : BMF_NOZEROINIT), NULL);
   if (!hBitmap)
   {
      DPRINT("IntGdiCreateBitmap: returned 0\n");
      return 0;
   }

   PBITMAPOBJ bmp = BITMAPOBJ_LockBitmap( hBitmap );
   if (bmp == NULL)
   {
      NtGdiDeleteObject(hBitmap);
      return NULL;
   }

   bmp->flFlags = BITMAPOBJ_IS_APIBITMAP;
   bmp->hDC = NULL; // Fixme

   if (NULL != pBits)
   {
       IntSetBitmapBits(bmp, bmp->SurfObj.cjBits, pBits);
   }

   BITMAPOBJ_UnlockBitmap( bmp );

   DPRINT("IntGdiCreateBitmap : %dx%d, %d BPP colors, topdown %d, returning %08x\n",
          Size.cx, Size.cy, BitsPixel, (Height < 0 ? 1 : 0), hBitmap);

   return hBitmap;
}


HBITMAP APIENTRY
NtGdiCreateBitmap(
    INT  Width,
    INT  Height,
    UINT  Planes,
    UINT  BitsPixel,
    IN OPTIONAL LPBYTE pUnsafeBits)
{
   if (pUnsafeBits)
   {
      BOOL Hit = FALSE;
      UINT cjBits = BITMAPOBJ_GetWidthBytes(Width, BitsPixel) * abs(Height);

      _SEH_TRY
      {
         ProbeForRead(pUnsafeBits, cjBits, 1);
      }
      _SEH_HANDLE
      {
         Hit = TRUE;
      }
      _SEH_END

      if (Hit) return 0;
   }

   return IntGdiCreateBitmap(Width, Height, Planes, BitsPixel, pUnsafeBits);
}

BOOL INTERNAL_CALL
BITMAP_Cleanup(PVOID ObjectBody)
{
	PBITMAPOBJ pBmp = (PBITMAPOBJ)ObjectBody;
	if (pBmp->SurfObj.pvBits != NULL &&
		(pBmp->flFlags & BITMAPOBJ_IS_APIBITMAP))
	{
		if (pBmp->dib == NULL)
		{
			ExFreePool(pBmp->SurfObj.pvBits);
		}
		else
		{
			EngFreeUserMem(pBmp->SurfObj.pvBits);
		}
		if (pBmp->hDIBPalette != NULL)
		{
			NtGdiDeleteObject(pBmp->hDIBPalette);
		}
	}

	if (NULL != pBmp->BitsLock)
	{
		ExFreePoolWithTag(pBmp->BitsLock, TAG_BITMAPOBJ);
		pBmp->BitsLock = NULL;
	}

	if (pBmp->dib)
		ExFreePoolWithTag(pBmp->dib, TAG_DIB);

	return TRUE;
}


HBITMAP FASTCALL
IntCreateCompatibleBitmap(
	PDC Dc,
	INT Width,
	INT Height)
{
	HBITMAP Bmp;

	/* MS doc says if width or height is 0, return 1-by-1 pixel, monochrome bitmap */
	if (0 == Width || 0 == Height)
	{
		Bmp = NtGdiGetStockObject(DEFAULT_BITMAP);
	}
	else
	{
		Bmp = IntGdiCreateBitmap(abs(Width), abs(Height), 1, Dc->w.bitsPerPixel, NULL);
	}

	return Bmp;
}

HBITMAP APIENTRY
NtGdiCreateCompatibleBitmap(
	HDC hDC,
	INT Width,
	INT Height)
{
	HBITMAP Bmp;
	PDC Dc;

	if ( Width <= 0 || Height <= 0 || (Width * Height) > 0x3FFFFFFF )
	{
           SetLastWin32Error(ERROR_INVALID_PARAMETER);
           return NULL;
        }

	Dc = DC_LockDc(hDC);

	DPRINT("NtGdiCreateCompatibleBitmap(%04x,%d,%d, bpp:%d) = \n", hDC, Width, Height, Dc->w.bitsPerPixel);

	if (NULL == Dc)
	{
		SetLastWin32Error(ERROR_INVALID_HANDLE);
		return NULL;
	}

	Bmp = IntCreateCompatibleBitmap(Dc, Width, Height);

	DPRINT ("\t\t%04x\n", Bmp);
	DC_UnlockDc(Dc);
	return Bmp;
}

BOOL APIENTRY
NtGdiGetBitmapDimension(
	HBITMAP  hBitmap,
	LPSIZE  Dimension)
{
	PBITMAPOBJ  bmp;
	BOOL Ret = TRUE;

	if (hBitmap == NULL)
		return FALSE;

	bmp = BITMAPOBJ_LockBitmap(hBitmap);
	if (bmp == NULL)
	{
		SetLastWin32Error(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	_SEH_TRY
	{
		ProbeForWrite(Dimension, sizeof(SIZE), 1);
		*Dimension = bmp->dimension;
	}
	_SEH_HANDLE
	{
		Ret = FALSE;
	}
	_SEH_END

	BITMAPOBJ_UnlockBitmap(bmp);

	return Ret;
}

COLORREF APIENTRY
NtGdiGetPixel(HDC hDC, INT XPos, INT YPos)
{
	PDC dc = NULL;
	COLORREF Result = (COLORREF)CLR_INVALID; // default to failure
	BOOL bInRect = FALSE;
	BITMAPOBJ *BitmapObject;
	SURFOBJ *SurfaceObject;
	HPALETTE Pal = 0;
	XLATEOBJ *XlateObj;
	HBITMAP hBmpTmp;

	dc = DC_LockDc (hDC);

	if ( !dc )
	{
		SetLastWin32Error(ERROR_INVALID_HANDLE);
		return Result;
	}
	if (dc->DC_Type == DC_TYPE_INFO)
	{
		DC_UnlockDc(dc);
		return Result;
	}
	XPos += dc->ptlDCOrig.x;
	YPos += dc->ptlDCOrig.y;
	if ( IN_RECT(dc->CombinedClip->rclBounds,XPos,YPos) )
	{
		bInRect = TRUE;
		BitmapObject = BITMAPOBJ_LockBitmap(dc->w.hBitmap);
		SurfaceObject = &BitmapObject->SurfObj;
		if ( BitmapObject )
		{
			Pal = BitmapObject->hDIBPalette;
			if (!Pal) Pal = pPrimarySurface->DevInfo.hpalDefault;

			/* FIXME: Verify if it shouldn't be PAL_BGR! */
			XlateObj = (XLATEOBJ*)IntEngCreateXlate ( PAL_RGB, 0, NULL, Pal );
			if ( XlateObj )
			{
				// check if this DC has a DIB behind it...
				if ( SurfaceObject->pvScan0 ) // STYPE_BITMAP == SurfaceObject->iType
				{
					ASSERT ( SurfaceObject->lDelta );
					Result = XLATEOBJ_iXlate(XlateObj,
						DibFunctionsForBitmapFormat[SurfaceObject->iBitmapFormat].DIB_GetPixel ( SurfaceObject, XPos, YPos ) );
				}
				EngDeleteXlate(XlateObj);
			}
			BITMAPOBJ_UnlockBitmap(BitmapObject);
		}
	}
	DC_UnlockDc(dc);

	// if Result is still CLR_INVALID, then the "quick" method above didn't work
	if ( bInRect && Result == CLR_INVALID )
	{
		// FIXME: create a 1x1 32BPP DIB, and blit to it
		HDC hDCTmp = NtGdiCreateCompatibleDC(hDC);
		if ( hDCTmp )
		{
			static const BITMAPINFOHEADER bih = { sizeof(BITMAPINFOHEADER), 1, 1, 1, 32, BI_RGB, 0, 0, 0, 0, 0 };
			BITMAPINFO bi;
			RtlMoveMemory ( &(bi.bmiHeader), &bih, sizeof(bih) );
			hBmpTmp = NtGdiCreateDIBitmapInternal(hDC,
                                                  bi.bmiHeader.biWidth,
                                                  bi.bmiHeader.biHeight,
                                                  0,
                                                  NULL,
                                                  &bi,
                                                  DIB_RGB_COLORS,
                                                  bi.bmiHeader.biBitCount,
                                                  bi.bmiHeader.biSizeImage,
                                                  0,
                                                  0);

			//HBITMAP hBmpTmp = IntGdiCreateBitmap ( 1, 1, 1, 32, NULL);
			if ( hBmpTmp )
			{
				HBITMAP hBmpOld = (HBITMAP)NtGdiSelectBitmap ( hDCTmp, hBmpTmp );
				if ( hBmpOld )
				{
					PBITMAPOBJ bmpobj;

					NtGdiBitBlt ( hDCTmp, 0, 0, 1, 1, hDC, XPos, YPos, SRCCOPY, 0, 0 );
					NtGdiSelectBitmap ( hDCTmp, hBmpOld );

					// our bitmap is no longer selected, so we can access it's stuff...
					bmpobj = BITMAPOBJ_LockBitmap ( hBmpTmp );
					if ( bmpobj )
					{
						Result = *(COLORREF*)bmpobj->SurfObj.pvScan0;
						BITMAPOBJ_UnlockBitmap ( bmpobj );
					}
				}
				NtGdiDeleteObject ( hBmpTmp );
			}
			NtGdiDeleteObjectApp ( hDCTmp );
		}
	}

	return Result;
}


LONG APIENTRY
IntGetBitmapBits(
	PBITMAPOBJ bmp,
	DWORD Bytes,
	OUT PBYTE Bits)
{
	LONG ret;

	ASSERT(Bits);

	/* Don't copy more bytes than the buffer has */
	Bytes = min(Bytes, bmp->SurfObj.cjBits);

#if 0
	/* FIXME: Call DDI CopyBits here if available  */
	if(bmp->DDBitmap)
	{
		DPRINT("Calling device specific BitmapBits\n");
		if(bmp->DDBitmap->funcs->pBitmapBits)
		{
			ret = bmp->DDBitmap->funcs->pBitmapBits(hbitmap, bits, count, DDB_GET);
		}
		else
		{
			ERR_(bitmap)("BitmapBits == NULL??\n");
			ret = 0;
		}
	}
	else
#endif
	{
		RtlCopyMemory(Bits, bmp->SurfObj.pvBits, Bytes);
		ret = Bytes;
	}
	return ret;
}

LONG APIENTRY
NtGdiGetBitmapBits(HBITMAP  hBitmap,
                   ULONG  Bytes,
                   OUT OPTIONAL PBYTE pUnsafeBits)
{
	PBITMAPOBJ  bmp;
	LONG  ret;

	if (pUnsafeBits != NULL && Bytes == 0)
	{
		return 0;
	}

	bmp = BITMAPOBJ_LockBitmap (hBitmap);
	if (!bmp)
	{
		SetLastWin32Error(ERROR_INVALID_HANDLE);
		return 0;
	}

	/* If the bits vector is null, the function should return the read size */
	if (pUnsafeBits == NULL)
	{
		ret = bmp->SurfObj.cjBits;
		BITMAPOBJ_UnlockBitmap (bmp);
		return ret;
	}

	/* Don't copy more bytes than the buffer has */
	Bytes = min(Bytes, bmp->SurfObj.cjBits);

	_SEH_TRY
	{
		ProbeForWrite(pUnsafeBits, Bytes, 1);
		ret = IntGetBitmapBits(bmp, Bytes, pUnsafeBits);
	}
	_SEH_HANDLE
	{
		ret = 0;
	}
	_SEH_END

	BITMAPOBJ_UnlockBitmap (bmp);

	return  ret;
}


LONG APIENTRY
IntSetBitmapBits(
	PBITMAPOBJ bmp,
	DWORD  Bytes,
	IN PBYTE Bits)
{
	LONG ret;

	/* Don't copy more bytes than the buffer has */
	Bytes = min(Bytes, bmp->SurfObj.cjBits);

#if 0
	/* FIXME: call DDI specific function here if available  */
	if(bmp->DDBitmap)
	{
		DPRINT ("Calling device specific BitmapBits\n");
		if (bmp->DDBitmap->funcs->pBitmapBits)
		{
			ret = bmp->DDBitmap->funcs->pBitmapBits(hBitmap, (void *) Bits, Bytes, DDB_SET);
		}
		else
		{
			DPRINT ("BitmapBits == NULL??\n");
			ret = 0;
		}
	}
	else
#endif
	{
		RtlCopyMemory(bmp->SurfObj.pvBits, Bits, Bytes);
		ret = Bytes;
	}

	return ret;
}


LONG APIENTRY
NtGdiSetBitmapBits(
	HBITMAP  hBitmap,
	DWORD  Bytes,
	IN PBYTE pUnsafeBits)
{
	LONG ret;
	PBITMAPOBJ bmp;

	if (pUnsafeBits == NULL || Bytes == 0)
	{
		return 0;
	}

	bmp = BITMAPOBJ_LockBitmap(hBitmap);
	if (bmp == NULL)
	{
		SetLastWin32Error(ERROR_INVALID_HANDLE);
		return 0;
	}

	_SEH_TRY
	{
		ProbeForRead(pUnsafeBits, Bytes, 1);
		ret = IntSetBitmapBits(bmp, Bytes, pUnsafeBits);
	}
	_SEH_HANDLE
	{
		ret = 0;
	}
	_SEH_END

	BITMAPOBJ_UnlockBitmap(bmp);

	return ret;
}

BOOL APIENTRY
NtGdiSetBitmapDimension(
	HBITMAP  hBitmap,
	INT  Width,
	INT  Height,
	LPSIZE  Size)
{
	PBITMAPOBJ  bmp;
	BOOL Ret = TRUE;

	if (hBitmap == NULL)
		return FALSE;

	bmp = BITMAPOBJ_LockBitmap(hBitmap);
	if (bmp == NULL)
	{
		SetLastWin32Error(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	if (Size)
	{
		_SEH_TRY
		{
			ProbeForWrite(Size, sizeof(SIZE), 1);
			*Size = bmp->dimension;
		}
		_SEH_HANDLE
		{
			Ret = FALSE;
		}
		_SEH_END
	}

	/* The dimension is changed even if writing the old value failed */
	bmp->dimension.cx = Width;
	bmp->dimension.cy = Height;

	BITMAPOBJ_UnlockBitmap (bmp);

	return Ret;
}

BOOL APIENTRY
GdiSetPixelV(
	HDC  hDC,
	INT  X,
	INT  Y,
	COLORREF  Color)
{
	HBRUSH NewBrush = NtGdiCreateSolidBrush(Color, NULL);
	HGDIOBJ OldBrush;

	if (NewBrush == NULL)
		return(FALSE);
	OldBrush = NtGdiSelectBrush(hDC, NewBrush);
	if (OldBrush == NULL)
	{
		NtGdiDeleteObject(NewBrush);
		return(FALSE);
	}
	NtGdiPatBlt(hDC, X, Y, 1, 1, PATCOPY);
	NtGdiSelectBrush(hDC, OldBrush);
	NtGdiDeleteObject(NewBrush);
	return TRUE;
}

COLORREF APIENTRY
NtGdiSetPixel(
	HDC  hDC,
	INT  X,
	INT  Y,
	COLORREF  Color)
{
   	DPRINT("0 NtGdiSetPixel X %ld Y %ld C %ld\n",X,Y,Color);

	if (GdiSetPixelV(hDC,X,Y,Color))
	{
		Color = NtGdiGetPixel(hDC,X,Y);
		DPRINT("1 NtGdiSetPixel X %ld Y %ld C %ld\n",X,Y,Color);
		return Color;
	}

	Color = ((COLORREF) CLR_INVALID);
	DPRINT("2 NtGdiSetPixel X %ld Y %ld C %ld\n",X,Y,Color);
	return Color;
}


/*  Internal Functions  */

UINT FASTCALL
BITMAPOBJ_GetRealBitsPixel(UINT nBitsPixel)
{
	if (nBitsPixel <= 1)
		return 1;
	if (nBitsPixel <= 4)
		return 4;
	if (nBitsPixel <= 8)
		return 8;
	if (nBitsPixel <= 16)
		return 16;
	if (nBitsPixel <= 24)
		return 24;
	if (nBitsPixel <= 32)
		return 32;

	return 0;
}

INT FASTCALL
BITMAPOBJ_GetWidthBytes (INT bmWidth, INT bpp)
{
#if 0
	switch(bpp)
	{
	case 1:
		return 2 * ((bmWidth+15) >> 4);

	case 24:
		bmWidth *= 3; /* fall through */
	case 8:
		return bmWidth + (bmWidth & 1);

	case 32:
		return bmWidth * 4;

	case 16:
	case 15:
		return bmWidth * 2;

	case 4:
		return 2 * ((bmWidth+3) >> 2);

	default:
		DPRINT ("stub");
	}

	return -1;
#endif

	return ((bmWidth * bpp + 15) & ~15) >> 3;
}

HBITMAP FASTCALL
BITMAPOBJ_CopyBitmap(HBITMAP  hBitmap)
{
	HBITMAP  res;
	BITMAP  bm;
	BITMAPOBJ *Bitmap, *resBitmap;
	SIZEL Size;

	if (hBitmap == NULL)
	{
		return 0;
	}

	Bitmap = GDIOBJ_LockObj(hBitmap, GDI_OBJECT_TYPE_BITMAP);
	if (Bitmap == NULL)
	{
		return 0;
	}

	BITMAP_GetObject(Bitmap, sizeof(BITMAP), (PVOID)&bm);
	bm.bmBits = NULL;
	if (Bitmap->SurfObj.lDelta >= 0)
		bm.bmHeight = -bm.bmHeight;

	Size.cx = abs(bm.bmWidth);
	Size.cy = abs(bm.bmHeight);
	res = IntCreateBitmap(Size,
	                      bm.bmWidthBytes,
	                      BitmapFormat(bm.bmBitsPixel * bm.bmPlanes, BI_RGB),
	                      (bm.bmHeight < 0 ? BMF_TOPDOWN : 0) | BMF_NOZEROINIT,
	                      NULL);

	if(res)
	{
		PBYTE buf;

		resBitmap = GDIOBJ_LockObj(res, GDI_OBJECT_TYPE_BITMAP);
		if (resBitmap)
		{
			buf = ExAllocatePoolWithTag (PagedPool, bm.bmWidthBytes * abs(bm.bmHeight), TAG_BITMAP);
			if (buf == NULL)
			{
				GDIOBJ_UnlockObjByPtr((POBJ)resBitmap);
				GDIOBJ_UnlockObjByPtr((POBJ)Bitmap);
				NtGdiDeleteObject(res);
				return 0;
			}
			IntGetBitmapBits (Bitmap, bm.bmWidthBytes * abs(bm.bmHeight), buf);
			IntSetBitmapBits (resBitmap, bm.bmWidthBytes * abs(bm.bmHeight), buf);
			ExFreePoolWithTag (buf,TAG_BITMAP);
			resBitmap->flFlags = Bitmap->flFlags;
			GDIOBJ_UnlockObjByPtr((POBJ)resBitmap);
		}
		else
		{
			NtGdiDeleteObject(res);
			res = NULL;
		}
	}

	GDIOBJ_UnlockObjByPtr((POBJ)Bitmap);

	return  res;
}

INT APIENTRY
BITMAP_GetObject(BITMAPOBJ * bmp, INT Count, LPVOID buffer)
{
	if ((UINT)Count < sizeof(BITMAP)) return 0;

	if(bmp->dib)
	{
		if((UINT)Count < sizeof(DIBSECTION))
		{
			Count = sizeof(BITMAP);
		}
		else
		{
			Count = sizeof(DIBSECTION);
		}
		if (buffer)
		{
			memcpy(buffer, bmp->dib, Count);
		}
		return Count;
	}
	else
	{
		Count = sizeof(BITMAP);
		if (buffer)
		{
			BITMAP Bitmap;

			Count = sizeof(BITMAP);
			Bitmap.bmType = 0;
			Bitmap.bmWidth = bmp->SurfObj.sizlBitmap.cx;
			Bitmap.bmHeight = bmp->SurfObj.sizlBitmap.cy;
			Bitmap.bmWidthBytes = abs(bmp->SurfObj.lDelta);
			Bitmap.bmPlanes = 1;
			Bitmap.bmBitsPixel = BitsPerFormat(bmp->SurfObj.iBitmapFormat);
			Bitmap.bmBits = NULL; /* not set according to wine test, confirmed in win2k */
			memcpy(buffer, &Bitmap, Count);
		}
		return Count;
	}
}

/*
 * @implemented
 */
HDC
APIENTRY
NtGdiGetDCforBitmap(
    IN HBITMAP hsurf)
{
  HDC hDC = NULL;
  PBITMAPOBJ bmp = BITMAPOBJ_LockBitmap( hsurf );
  if (bmp)
  {
    hDC = bmp->hDC;
    BITMAPOBJ_UnlockBitmap( bmp );
  }
  return hDC;
}


/* EOF */

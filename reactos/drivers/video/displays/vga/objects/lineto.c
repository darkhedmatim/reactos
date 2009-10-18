/*
 *  ReactOS VGA driver
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
 *
 * $Id$
 */

#include "../vgaddi.h"
#include "../vgavideo/vgavideo.h"

/*
 * Draw a line from top-left to bottom-right
 */
static void FASTCALL
vgaNWtoSE(CLIPOBJ* Clip, BRUSHOBJ* Brush, LONG x, LONG y, LONG deltax, LONG deltay)
{
  int i;
  int error;
  BOOLEAN EnumMore;
  PRECTL ClipRect;
  RECT_ENUM RectEnum;
  ULONG Pixel = Brush->iSolidColor;
  LONG delta;

  CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_RIGHTDOWN, 0);
  EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
  ClipRect = RectEnum.arcl;
  delta = max(deltax, deltay);
  i = 0;
  error = delta/2;
  while (i < delta && (ClipRect < RectEnum.arcl + RectEnum.c || EnumMore))
    {
      while ((ClipRect < RectEnum.arcl + RectEnum.c /* there's still a current clip rect */
              && (ClipRect->bottom <= y             /* but it's above us */
                  || (ClipRect->top <= y && ClipRect->right <= x))) /* or to the left of us */
             || EnumMore)                           /* no current clip rect, but rects left */
	{
	  /* Skip to the next clip rect */
	  if (RectEnum.arcl + RectEnum.c <= ClipRect)
	    {
	      EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
	      ClipRect = RectEnum.arcl;
	    }
	  else
	    {
	      ClipRect++;
	    }
	}
      if ( ClipRect < RectEnum.arcl + RectEnum.c ) /* If there's no current clip rect we're done */
	{
	  if (ClipRect->left <= x && ClipRect->top <= y)
	    {
	      vgaPutPixel ( x, y, Pixel );
	    }
	  if ( deltax < deltay )
	    {
	      y++;
	      error += deltax;
	      if ( error >= deltay )
		{
		  x++;
		  error -= deltay;
		}
	    }
	  else
	    {
	      x++;
	      error += deltay;
	      if ( error >= deltax )
		{
		  y++;
		  error -= deltax;
		}
	    }
	  i++;
	}
    }
}

static void FASTCALL
vgaSWtoNE(CLIPOBJ* Clip, BRUSHOBJ* Brush, LONG x, LONG y, LONG deltax, LONG deltay)
{
  int i;
  int error;
  BOOLEAN EnumMore;
  PRECTL ClipRect;
  RECT_ENUM RectEnum;
  ULONG Pixel = Brush->iSolidColor;
  LONG delta;

  CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_RIGHTUP, 0);
  EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
  ClipRect = RectEnum.arcl;
  delta = max(deltax, deltay);
  i = 0;
  error = delta/2;
  while (i < delta && (ClipRect < RectEnum.arcl + RectEnum.c || EnumMore))
    {
      while ((ClipRect < RectEnum.arcl + RectEnum.c
              && (y < ClipRect->top
                  || (y < ClipRect->bottom && ClipRect->right <= x)))
             || EnumMore)
	{
	  if (RectEnum.arcl + RectEnum.c <= ClipRect)
	    {
	      EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
	      ClipRect = RectEnum.arcl;
	    }
	  else
	    {
	      ClipRect++;
	    }
	}
      if (ClipRect < RectEnum.arcl + RectEnum.c)
	{
	  if (ClipRect->left <= x && y < ClipRect->bottom)
	    {
	      vgaPutPixel(x, y, Pixel);
	    }
	  if (deltax < deltay)
	    {
	      y--;
	      error = error + deltax;
	      if (deltay <= error)
		{
		  x++;
		  error = error - deltay;
		}
	    }
	  else
	    {
	      x++;
	      error = error + deltay;
	      if (deltax <= error)
		{
	  	  y--;
		  error = error - deltax;
		}
	    }
	  i++;
	}
    }
}

static void FASTCALL
vgaNEtoSW(CLIPOBJ* Clip, BRUSHOBJ* Brush, LONG x, LONG y, LONG deltax, LONG deltay)
{
  int i;
  int error;
  BOOLEAN EnumMore;
  PRECTL ClipRect;
  RECT_ENUM RectEnum;
  ULONG Pixel = Brush->iSolidColor;
  LONG delta;

  CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_LEFTDOWN, 0);
  EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
  ClipRect = RectEnum.arcl;
  delta = max(deltax, deltay);
  i = 0;
  error = delta/2;
  while (i < delta && (ClipRect < RectEnum.arcl + RectEnum.c || EnumMore))
    {
      while ((ClipRect < RectEnum.arcl + RectEnum.c
              && (ClipRect->bottom <= y
                  || (ClipRect->top <= y && x < ClipRect->left)))
             || EnumMore)
	{
	  if (RectEnum.arcl + RectEnum.c <= ClipRect)
	    {
	      EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
	      ClipRect = RectEnum.arcl;
	    }
	  else
	    {
	      ClipRect++;
	    }
	}
      if (ClipRect < RectEnum.arcl + RectEnum.c)
	{
	  if (x < ClipRect->right && ClipRect->top <= y)
	    {
	      vgaPutPixel(x, y, Pixel);
	    }
	  if (deltax < deltay)
	    {
	      y++;
	      error = error + deltax;
	      if (deltay <= error)
		{
		  x--;
		  error = error - deltay;
		}
	    }
	  else
	    {
	      x--;
	      error = error + deltay;
	      if (deltax <= error)
		{
	  	  y++;
		  error = error - deltax;
		}
	    }
	  i++;
	}
    }
}

static void FASTCALL
vgaSEtoNW(CLIPOBJ* Clip, BRUSHOBJ* Brush, LONG x, LONG y, LONG deltax, LONG deltay)
{
  int i;
  int error;
  BOOLEAN EnumMore;
  PRECTL ClipRect;
  RECT_ENUM RectEnum;
  ULONG Pixel = Brush->iSolidColor;
  LONG delta;

  CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_LEFTUP, 0);
  EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
  ClipRect = RectEnum.arcl;
  delta = max(deltax, deltay);
  i = 0;
  error = delta/2;
  while (i < delta && (ClipRect < RectEnum.arcl + RectEnum.c || EnumMore))
    {
      while ((ClipRect < RectEnum.arcl + RectEnum.c
              && (y < ClipRect->top
                  || (y < ClipRect->bottom && x < ClipRect->left)))
             || EnumMore)
	{
	  if (RectEnum.arcl + RectEnum.c <= ClipRect)
	    {
	      EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
	      ClipRect = RectEnum.arcl;
	    }
	  else
	    {
	      ClipRect++;
	    }
	}
      if (ClipRect < RectEnum.arcl + RectEnum.c)
	{
	  if (x < ClipRect->right && y < ClipRect->bottom)
	    {
	      vgaPutPixel(x, y, Pixel);
	    }
	  if (deltax < deltay)
	    {
	      y--;
	      error = error + deltax;
	      if (deltay <= error)
		{
		  x--;
		  error = error - deltay;
		}
	    }
	  else
	    {
	      x--;
	      error = error + deltay;
	      if (deltax <= error)
		{
	  	  y--;
		  error = error - deltax;
		}
	    }
	  i++;
	}
    }
}

/*
 * FIXME: Use Mix to perform ROPs
 * FIXME: Non-solid Brush
 */
BOOL STDCALL
DrvLineTo(SURFOBJ *DestObj,
	  CLIPOBJ *Clip,
	  BRUSHOBJ *Brush,
	  LONG x1,
	  LONG y1,
	  LONG x2,
	  LONG y2,
	  RECTL *RectBounds,
	  MIX mix)
{
  LONG x, y, deltax, deltay, xchange, ychange, hx, vy;
  ULONG i;
  ULONG Pixel = Brush->iSolidColor;
  RECT_ENUM RectEnum;
  BOOL EnumMore;

  x = x1;
  y = y1;
  deltax = x2 - x1;
  deltay = y2 - y1;

  if (deltax < 0)
    {
      xchange = -1;
      deltax = - deltax;
      hx = x2+1;
      //x--;
    }
  else
    {
      xchange = 1;
      hx = x1;
    }

  if (deltay < 0)
    {
      ychange = -1;
      deltay = - deltay;
      vy = y2+1;
      //y--;
    }
  else
    {
      ychange = 1;
      vy = y1;
    }

  if (y1 == y2)
    {
      CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_RIGHTDOWN, 0);
      do
	{
	  EnumMore = CLIPOBJ_bEnum(Clip, sizeof(RectEnum), (PVOID) &RectEnum);
	  for (i = 0; i < RectEnum.c && RectEnum.arcl[i].top <= y1; i++)
	    {
	      if (y1 < RectEnum.arcl[i].bottom &&
	          RectEnum.arcl[i].left <= hx + deltax &&
	          hx < RectEnum.arcl[i].right)
		{

		  vgaHLine(max(hx, RectEnum.arcl[i].left), y1,
		           min(hx + deltax, RectEnum.arcl[i].right)
		           -max(hx, RectEnum.arcl[i].left), Pixel);
		}
	    }
	}
      while (EnumMore);
    }
  else if (x1 == x2)
    {
      CLIPOBJ_cEnumStart(Clip, FALSE, CT_RECTANGLES, CD_RIGHTDOWN, 0);
      do
	{
	  EnumMore = CLIPOBJ_bEnum(Clip, (ULONG) sizeof(RectEnum), (PVOID) &RectEnum);
	  for (i = 0; i < RectEnum.c; i++)
	    {
	      if (RectEnum.arcl[i].left <= x1 &&
                  x1 < RectEnum.arcl[i].right &&
	          RectEnum.arcl[i].top <= vy + deltay &&
	          vy < RectEnum.arcl[i].bottom)
		{
		  vgaVLine(x1,
		           max(vy, RectEnum.arcl[i].top),
		           min(vy + deltay, RectEnum.arcl[i].bottom)
		           - max(vy, RectEnum.arcl[i].top),
		           Pixel);
		}
	    }
	}
      while (EnumMore);
    }
  else
    {
      if (0 < xchange)
	{
	  if (0 < ychange)
	    {
	      vgaNWtoSE(Clip, Brush, x, y, deltax, deltay);
	    }
	  else
	    {
	      vgaSWtoNE(Clip, Brush, x, y, deltax, deltay);
	    }
	}
      else
	{
	  if (0 < ychange)
	    {
	      vgaNEtoSW(Clip, Brush, x, y, deltax, deltay);
	    }
	  else
	    {
	      vgaSEtoNW(Clip, Brush, x, y, deltax, deltay);
	    }
	}
    }

  return TRUE;
}

/* EOF */

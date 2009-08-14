/*
 * Copyright 2005 Ge van Geldorp (gvg@reactos.com).
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * This is a code generator. It outputs C code which can be compiled using the
 * standard build tools. It is used to generate code for the DIB Blt routines.
 * There are a lot of possible Blt routines (256 rop codes, for 6 different
 * bit depths). It is possible to use a generic Blt routine which handles all
 * rop codes and all depths. The drawback is that it will be relatively slow.
 * The other extreme is to write (generate) a separate Blt routine for each
 * rop code/depth combination. This will result in a extremely large amount
 * of code. So, we opt for something in between: named rops get their own
 * routine, unnamed rops are handled by a generic routine.
 * Basically, what happens is that generic code which looks like:
 *
 * for (...)
 *   {
 *     if (CondA)
 *       {
 *         doSomethingA;
 *       }
 *     for (...)
 *       {
 *         if (CondB)
 *           {
 *             doSomethingB;
 *           }
 *         else
 *           {
 *             doSomethingElseB;
 *           }
 *       }
 *   }
 *
 * is specialized for named rops to look like:
 *
 * if (condC)
 *   {
 *     if (condD)
 *       {
 *         for (...)
 *           {
 *             for (...)
 *               {
 *                 pumpSomeBytes;
 *               }
 *           }
 *       }
 *     else
 *       {
 *         for (...)
 *           {
 *             for (...)
 *               {
 *                 pumpSomeBytesSlightlyDifferentE;
 *               }
 *           }
 *       }
 *   }
 *
 * i.e. we make the inner loops as tight as possible.
 * Another optimization is to try to load/store 32 alligned bits at a time from
 * video memory. Accessing video memory from the CPU is slooooooow, so let's
 * try to do this as little as possible, even if that means we have to do some
 * extra operations using main memory.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NDEBUG

#define USES_DEST(RopCode)    ((((RopCode) & 0xaa) >> 1) != ((RopCode) & 0x55))
#define USES_SOURCE(RopCode)  ((((RopCode) & 0xcc) >> 2) != ((RopCode) & 0x33))
#define USES_PATTERN(RopCode) ((((RopCode) & 0xf0) >> 4) != ((RopCode) & 0x0f))

#ifdef NDEBUG
#define MARK(Out)
#else
#define MARK(Out) Output((Out), "/* Generated by %s line %d*/\n", \
                         __FUNCTION__, __LINE__);
#endif

#define ROPCODE_BLACKNESS   0x00
#define ROPCODE_NOTSRCERASE 0x11
#define ROPCODE_NOTSRCCOPY  0x33
#define ROPCODE_SRCERASE    0x44
#define ROPCODE_DSTINVERT   0x55
#define ROPCODE_PATINVERT   0x5a
#define ROPCODE_SRCINVERT   0x66
#define ROPCODE_SRCAND      0x88
#define ROPCODE_NOOP        0xaa
#define ROPCODE_MERGEPAINT  0xbb
#define ROPCODE_MERGECOPY   0xc0
#define ROPCODE_SRCCOPY     0xcc
#define ROPCODE_SRCPAINT    0xee
#define ROPCODE_PATCOPY     0xf0
#define ROPCODE_PATPAINT    0xfb
#define ROPCODE_WHITENESS   0xff

#define ROPCODE_GENERIC     256 /* Special case */

typedef struct _ROPINFO
  {
    unsigned RopCode;
    const char *Name;
    const char *Operation;
    int UsesDest;
    int UsesSource;
    int UsesPattern;
  }
ROPINFO, *PROPINFO;

#define FLAG_PATTERNSURFACE      0x01
#define FLAG_TRIVIALXLATE        0x02
#define FLAG_BOTTOMUP            0x04
#define FLAG_FORCENOUSESSOURCE   0x08
#define FLAG_FORCERAWSOURCEAVAIL 0x10

static PROPINFO
FindRopInfo(unsigned RopCode)
{
  static ROPINFO KnownCodes[] =
    {
      { ROPCODE_BLACKNESS,   "BLACKNESS",  "0",            0, 0, 0 },
      { ROPCODE_NOTSRCERASE, "NOTSRCERASE","~(D | S)",     1, 1, 0 },
      { ROPCODE_NOTSRCCOPY,  "NOTSRCCOPY", "~S",           0, 1, 0 },
      { ROPCODE_SRCERASE,    "SRCERASE",   "(~D) & S",     1, 1, 0 },
      { ROPCODE_DSTINVERT,   "DSTINVERT",  "~D",           1, 0, 0 },
      { ROPCODE_PATINVERT,   "PATINVERT",  "D ^ P",        1, 0, 1 },
      { ROPCODE_SRCINVERT,   "SRCINVERT",  "D ^ S",        1, 1, 0 },
      { ROPCODE_SRCAND,      "SRCAND",     "D & S",        1, 1, 0 },
      { ROPCODE_NOOP,        "NOOP",       "D",            1, 0, 0 },
      { ROPCODE_MERGEPAINT,  "MERGEPAINT", "D | (~S)",     1, 1, 0 },
      { ROPCODE_MERGECOPY,   "MERGECOPY",  "S & P",        0, 1, 1 },
      { ROPCODE_SRCCOPY,     "SRCCOPY",    "S",            0, 1, 0 },
      { ROPCODE_SRCPAINT,    "SRCPAINT",   "D | S",        1, 1, 0 },
      { ROPCODE_PATCOPY,     "PATCOPY",    "P",            0, 0, 1 },
      { ROPCODE_PATPAINT,    "PATPAINT",   "D | (~S) | P", 1, 1, 1 },
      { ROPCODE_WHITENESS,   "WHITENESS",  "0xffffffff",   0, 0, 0 },
      { ROPCODE_GENERIC,     NULL,         NULL,           1, 1, 1 }
    };
  unsigned Index;

  for (Index = 0; Index < sizeof(KnownCodes) / sizeof(KnownCodes[0]); Index++)
    {
      if (RopCode == KnownCodes[Index].RopCode)
        {
          return KnownCodes + Index;
        }
    }

  return NULL;
}

static void
Output(FILE *Out, const char *Fmt, ...)
{
  static unsigned Indent = 0;
  static int AtBOL = 1;
  unsigned n;
  va_list Args;

  if (NULL != strchr(Fmt, '{') && 0 != Indent)
    {
      Indent += 2;
    }
  else if (NULL != strchr(Fmt, '}'))
    {
      Indent -= 2;
    }
  if (AtBOL)
    {
      for (n = 0; n < Indent; n++)
        {
          putc(' ', Out);
        }
      AtBOL = 0;
    }

  va_start(Args, Fmt);
  vfprintf(Out, Fmt, Args);
  va_end(Args);

  if (NULL != strchr(Fmt, '{'))
    {
      Indent += 2;
    }
  else if (NULL != strchr(Fmt, '}') && 0 != Indent)
    {
      Indent -= 2;
    }
  AtBOL = '\n' == Fmt[strlen(Fmt) - 1];
}

static void
PrintRoutineName(FILE *Out, unsigned Bpp, PROPINFO RopInfo)
{
  if (NULL != RopInfo && ROPCODE_GENERIC != RopInfo->RopCode)
    {
      Output(Out, "DIB_%uBPP_BitBlt_%s", Bpp, RopInfo->Name);
    }
  else
    {
      Output(Out, "DIB_%uBPP_BitBlt_Generic", Bpp);
    }
}

static void
CreateShiftTables(FILE *Out)
{
  Output(Out, "\n");
  Output(Out, "static unsigned Shift1Bpp[] =\n");
  Output(Out, "{\n");
  Output(Out, "0,\n");
  Output(Out, "24, 25, 26, 27, 28, 29, 30, 31, 16, 17, 18, 19, 20, 21, 22, 23,\n");
  Output(Out, "8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7\n");
  Output(Out, "};\n");
  Output(Out, "static unsigned Shift4Bpp[] =\n");
  Output(Out, "{\n");
  Output(Out, "0,\n");
  Output(Out, "24, 28, 16, 20, 8, 12, 0, 4\n");
  Output(Out, "};\n");
  Output(Out, "static unsigned Shift8Bpp[] =\n");
  Output(Out, "{\n");
  Output(Out, "0,\n");
  Output(Out, "24, 16, 8, 0\n");
  Output(Out, "};\n");
  Output(Out, "static unsigned Shift16Bpp[] =\n");
  Output(Out, "{\n");
  Output(Out, "0,\n");
  Output(Out, "16, 0\n");
  Output(Out, "};\n");
}

static void
CreateOperation(FILE *Out, unsigned Bpp, PROPINFO RopInfo, unsigned SourceBpp,
                unsigned Bits)
{
  const char *Cast;
  const char *Dest;
  const char *Template;

  MARK(Out);
  if (32 == Bits)
    {
      Cast = "";
      Dest = "*DestPtr";
    }
  else if (16 == Bpp)
    {
      Cast = "(USHORT) ";
      Dest = "*((PUSHORT) DestPtr)";
    }
  else
    {
      Cast = "(UCHAR) ";
      Dest = "*((PUCHAR) DestPtr)";
    }
  Output(Out, "%s = ", Dest);
  if (ROPCODE_GENERIC == RopInfo->RopCode)
    {
      Output(Out, "%sDIB_DoRop(BltInfo->Rop4, %s, Source, Pattern)",
             Cast, Dest);
    }
  else
    {
      Template = RopInfo->Operation;
      while ('\0' != *Template)
        {
          switch(*Template)
            {
            case 'S':
              Output(Out, "%sSource", Cast);
              break;
            case 'P':
              Output(Out, "%sPattern", Cast);
              break;
            case 'D':
              Output(Out, "%s", Dest);
              break;
            default:
              Output(Out, "%c", *Template);
              break;
            }
          Template++;
        }
    }
}

static void
CreateBase(FILE *Out, int Source, int Flags, unsigned Bpp)
{
  const char *What = (Source ? "Source" : "Dest");

  MARK(Out);
  Output(Out, "%sBase = (char *) BltInfo->%sSurface->pvScan0 +\n", What, What);
  if (0 == (Flags & FLAG_BOTTOMUP))
    {
      if (Source)
        {
          Output(Out, "             BltInfo->SourcePoint.y *\n");
        }
      else
        {
          Output(Out, "           BltInfo->DestRect.top *\n");
        }
    }
  else
    {
      if (Source)
        {
          Output(Out, "             (BltInfo->SourcePoint.y +\n");
          Output(Out, "              BltInfo->DestRect.bottom -\n");
          Output(Out, "              BltInfo->DestRect.top - 1) *\n");
        }
      else
        {
          Output(Out, "           (BltInfo->DestRect.bottom - 1) *\n");
        }
    }
  Output(Out, "           %sBltInfo->%sSurface->lDelta +\n",
         Source ? "  " : "", What);
  if (Source)
    {
      Output(Out, "             %sBltInfo->SourcePoint.x",
             16 < Bpp ? "" : "((");
    }
  else
    {
      Output(Out, "           BltInfo->DestRect.left");
    }
  if (Bpp < 8)
    {
      Output(Out, " / %u", 8 / Bpp);
    }
  else if (8 < Bpp)
    {
      Output(Out, " * %u", Bpp / 8);
    }
  if (Source && Bpp <= 16)
    {
      Output(Out, ") & ~ 0x3)");
    }
  Output(Out, ";\n", Bpp / 8);
  if (Source && Bpp <= 16)
    {
      Output(Out, "BaseSourcePixels = %u - (BltInfo->SourcePoint.x & 0x%x);\n",
             32 / Bpp, 32 / Bpp - 1);
    }
}

static void
CreateGetSource(FILE *Out, unsigned Bpp, PROPINFO RopInfo, int Flags,
                unsigned SourceBpp, unsigned Shift)
{
  const char *AssignOp;
  const char *Before;
  char After[8];

  MARK(Out);
  if (0 == Shift)
    {
      AssignOp = "=";
      Before = "";
      After[0] = '\0';
    }
  else
    {
      AssignOp = "|=";
      Before = "(";
      sprintf(After, ") << %u", Shift);
    }

  if (ROPCODE_SRCCOPY != RopInfo->RopCode ||
      0 == (Flags & FLAG_TRIVIALXLATE) || Bpp != SourceBpp)
    {
      if (0 == (Flags & FLAG_FORCERAWSOURCEAVAIL) && SourceBpp <= 16)
        {
          Output(Out, "if (0 == SourcePixels)\n");
          Output(Out, "{\n");
          Output(Out, "RawSource = *SourcePtr++;\n");
          Output(Out, "SourcePixels = %u;\n", 32 / SourceBpp);
          Output(Out, "}\n");
        }
      Output(Out, "Source %s (%s", AssignOp, Before);
      if (0 == (Flags & FLAG_TRIVIALXLATE))
        {
          Output(Out, "XLATEOBJ_iXlate(BltInfo->XlateSourceToDest, %s",
                 16 < SourceBpp ? "" : "(");
        }
      if (32 == SourceBpp)
        {
          Output(Out, "*SourcePtr++");
        }
      else if (24 == SourceBpp)
        {
          Output(Out, "*(PUSHORT) SourcePtr + (*((PBYTE) SourcePtr + 2) << 16)");
        }
      else
        {
          Output(Out, "RawSource >> Shift%uBpp[SourcePixels]", SourceBpp);
        }
      if (0 == (Flags & FLAG_TRIVIALXLATE))
        {
          if (16 < SourceBpp)
            {
              Output(Out, ")");
            }
          else
            {
              Output(Out, ") & 0x%x)", (1 << SourceBpp) - 1);
            }
        }
      if (32 == Bpp)
        {
          Output(Out, ")%s;\n", After);
        }
      else
        {
          Output(Out, " & 0x%x)%s;\n", (1 << Bpp) - 1, After);
        }
      if (SourceBpp <= 16)
        {
          Output(Out, "SourcePixels--;\n");
        }
      else if (24 == SourceBpp)
        {
          Output(Out, "SourcePtr = (PULONG)((char *) SourcePtr + 3);\n");
        }
    }
}

static void
CreateCounts(FILE *Out, unsigned Bpp)
{
  MARK(Out);
  if (32 != Bpp)
    {
      if (8 < Bpp)
        {
          Output(Out, "LeftCount = ((ULONG_PTR) DestBase >> 1) & 0x01;\n");
        }
      else
        {
          Output(Out, "LeftCount = (ULONG_PTR) DestBase & 0x03;\n");
          Output(Out, "if (BltInfo->DestRect.right - BltInfo->DestRect.left < "
                      "LeftCount)\n");
          Output(Out, "{\n");
          Output(Out, "LeftCount = BltInfo->DestRect.right - "
                      "BltInfo->DestRect.left;\n");
          Output(Out, "}\n");
        }
      Output(Out, "CenterCount = (BltInfo->DestRect.right - BltInfo->DestRect.left -\n");
      Output(Out, "               LeftCount) / %u;\n", 32 / Bpp);
      Output(Out, "RightCount = (BltInfo->DestRect.right - BltInfo->DestRect.left -\n");
      Output(Out, "              LeftCount - %u * CenterCount);\n", 32 / Bpp);
    }
  else
    {
      Output(Out, "CenterCount = BltInfo->DestRect.right - BltInfo->DestRect.left;\n");
    }
}

static void
CreateSetSinglePixel(FILE *Out, unsigned Bpp, PROPINFO RopInfo, int Flags,
                     unsigned SourceBpp)
{
  if (RopInfo->UsesSource && 0 == (Flags & FLAG_FORCENOUSESSOURCE))
    {
      CreateGetSource(Out, Bpp, RopInfo, Flags, SourceBpp, 0);
      MARK(Out);
    }
  if (RopInfo->UsesPattern && 0 != (Flags & FLAG_PATTERNSURFACE))
    {
      Output(Out, "Pattern = DIB_GetSourceIndex(BltInfo->PatternSurface, PatternX, PatternY);\n");
      Output(Out, "if (BltInfo->PatternSurface->sizlBitmap.cx <= ++PatternX)\n");
      Output(Out, "{\n");
      Output(Out, "PatternX -= BltInfo->PatternSurface->sizlBitmap.cx;\n");
      Output(Out, "}\n");
    }
  if ((RopInfo->UsesSource && 0 == (Flags & FLAG_FORCENOUSESSOURCE) &&
       Bpp != SourceBpp) ||
      (RopInfo->UsesPattern && 0 != (Flags & FLAG_PATTERNSURFACE)))
    {
      Output(Out, "\n");
    }
  CreateOperation(Out, Bpp, RopInfo, SourceBpp, 16);
  Output(Out, ";\n");
  MARK(Out);
  Output(Out, "\n");
  Output(Out, "DestPtr = (PULONG)((char *) DestPtr + %u);\n", Bpp / 8);
}

static void
CreateBitCase(FILE *Out, unsigned Bpp, PROPINFO RopInfo, int Flags,
              unsigned SourceBpp)
{
  unsigned Partial;

  MARK(Out);
  if (RopInfo->UsesSource)
    {
      if (0 == (Flags & FLAG_FORCENOUSESSOURCE))
        {
          CreateBase(Out, 1, Flags, SourceBpp);
        }
      CreateBase(Out, 0, Flags, Bpp);
      CreateCounts(Out, Bpp);
      MARK(Out);
    }
  if (RopInfo->UsesPattern && 0 != (Flags & FLAG_PATTERNSURFACE))
    {
      if (0 == (Flags & FLAG_BOTTOMUP))
        {
          Output(Out, "PatternY = (BltInfo->DestRect.top - BltInfo->BrushOrigin.y) %%\n");
          Output(Out, "           BltInfo->PatternSurface->sizlBitmap.cy;\n");
        }
      else
        {
          Output(Out, "PatternY = (BltInfo->DestRect.bottom - 1 -\n");
          Output(Out, "            BltInfo->BrushOrigin.y) %%\n");
          Output(Out, "           BltInfo->PatternSurface->sizlBitmap.cy;\n");
        }
    }
  if (ROPCODE_SRCCOPY == RopInfo->RopCode &&
      0 != (Flags & FLAG_TRIVIALXLATE) && Bpp == SourceBpp)
    {
      Output(Out, "CenterCount = %u * (BltInfo->DestRect.right -\n", Bpp >> 3);
      Output(Out, "                   BltInfo->DestRect.left);\n");
    }
  if (RopInfo->UsesPattern && 0 != (Flags & FLAG_PATTERNSURFACE))
    {
      Output(Out, "BasePatternX = (BltInfo->DestRect.left - BltInfo->BrushOrigin.x) %%\n");
      Output(Out, "           BltInfo->PatternSurface->sizlBitmap.cx;\n");
    }

  Output(Out, "for (LineIndex = 0; LineIndex < LineCount; LineIndex++)\n");
  Output(Out, "{\n");
  if (ROPCODE_SRCCOPY != RopInfo->RopCode ||
      0 == (Flags & FLAG_TRIVIALXLATE) || Bpp != SourceBpp)
    {
      if (RopInfo->UsesSource && 0 == (Flags & FLAG_FORCENOUSESSOURCE))
        {
          Output(Out, "SourcePtr = (PULONG) SourceBase;\n");
          if (SourceBpp <= 16)
            {
            Output(Out, "RawSource = *SourcePtr++;\n");
            Output(Out, "SourcePixels = BaseSourcePixels;\n");
            }
        }
      Output(Out, "DestPtr = (PULONG) DestBase;\n");
    }

  if (RopInfo->UsesPattern && 0 != (Flags & FLAG_PATTERNSURFACE))
   {
    Output(Out, "PatternX = BasePatternX;\n");
   }

  if (ROPCODE_SRCCOPY == RopInfo->RopCode &&
      0 != (Flags & FLAG_TRIVIALXLATE) && Bpp == SourceBpp)
    {
      Output(Out, "RtlMoveMemory(DestBase, SourceBase, CenterCount);\n");
      Output(Out, "\n");
    }
  else
    {
      Output(Out, "\n");
      if (32 != Bpp)
        {
          if (16 == Bpp)
            {
            Output(Out, "if (0 != LeftCount)\n");
            }
          else
            {
            Output(Out, "for (i = 0; i < LeftCount; i++)\n");
            }
          Output(Out, "{\n");
          CreateSetSinglePixel(Out, Bpp, RopInfo,
                               (16 == Bpp ? Flags | FLAG_FORCERAWSOURCEAVAIL :
                               Flags), SourceBpp);
          MARK(Out);
          Output(Out, "}\n");
          Output(Out, "\n");
        }
      Output(Out, "for (i = 0; i < CenterCount; i++)\n");
      Output(Out, "{\n");
      if (RopInfo->UsesSource && 0 == (Flags & FLAG_FORCENOUSESSOURCE))
        {
          for (Partial = 0; Partial < 32 / Bpp; Partial++)
            {
              CreateGetSource(Out, Bpp, RopInfo, Flags, SourceBpp,
                              Partial * Bpp);
              MARK(Out);
            }
          Output(Out, "\n");
        }
      if (RopInfo->UsesPattern && 0 != (Flags & FLAG_PATTERNSURFACE))
        {
          for (Partial = 0; Partial < 32 / Bpp; Partial++)
            {
              if (0 == Partial)
                {
                  Output(Out, "Pattern = DIB_GetSourceIndex(BltInfo->PatternSurface, PatternX, PatternY);\n");
                }
              else
                {
                  Output(Out, "Pattern |= DIB_GetSourceIndex(BltInfo->PatternSurface, PatternX, PatternY) << %u;\n", Partial * Bpp);
                }
              Output(Out, "if (BltInfo->PatternSurface->sizlBitmap.cx <= ++PatternX)\n");
              Output(Out, "{\n");
              Output(Out, "PatternX -= BltInfo->PatternSurface->sizlBitmap.cx;\n");
              Output(Out, "}\n");
            }
          Output(Out, "\n");
        }
      CreateOperation(Out, Bpp, RopInfo, SourceBpp, 32);
      Output(Out, ";\n");
      MARK(Out);
      Output(Out, "\n");
      Output(Out, "DestPtr++;\n");
      Output(Out, "}\n");
      Output(Out, "\n");
      if (32 != Bpp)
        {
          if (16 == Bpp)
            {
              Output(Out, "if (0 != RightCount)\n");
            }
          else
            {
              Output(Out, "for (i = 0; i < RightCount; i++)\n");
            }
          Output(Out, "{\n");
          CreateSetSinglePixel(Out, Bpp, RopInfo, Flags, SourceBpp);
          MARK(Out);
          Output(Out, "}\n");
          Output(Out, "\n");
        }
      if (RopInfo->UsesPattern && 0 != (Flags & FLAG_PATTERNSURFACE))
        {
          if (0 == (Flags & FLAG_BOTTOMUP))
            {
              Output(Out, "if (BltInfo->PatternSurface->sizlBitmap.cy <= ++PatternY)\n");
              Output(Out, "{\n");
              Output(Out, "PatternY -= BltInfo->PatternSurface->sizlBitmap.cy;\n");
              Output(Out, "}\n");
            }
          else
            {
              Output(Out, "if (0 == PatternY--)\n");
              Output(Out, "{\n");
              Output(Out, "PatternY = BltInfo->PatternSurface->sizlBitmap.cy - 1;\n");
              Output(Out, "}\n");
            }
        }
    }
  if (RopInfo->UsesSource && 0 == (Flags & FLAG_FORCENOUSESSOURCE))
    {
      Output(Out, "SourceBase %c= BltInfo->SourceSurface->lDelta;\n",
             0 == (Flags & FLAG_BOTTOMUP) ? '+' : '-');
    }
  Output(Out, "DestBase %c= BltInfo->DestSurface->lDelta;\n",
         0 == (Flags & FLAG_BOTTOMUP) ? '+' : '-');
  Output(Out, "}\n");
}

static void
CreateActionBlock(FILE *Out, unsigned Bpp, PROPINFO RopInfo,
                  int Flags)
{
  static unsigned SourceBpp[ ] =
    { 1, 4, 8, 16, 24, 32 };
  unsigned BppIndex;

  MARK(Out);
  if (RopInfo->UsesSource)
    {
      if (ROPCODE_GENERIC == RopInfo->RopCode)
        {
          Output(Out, "if (UsesSource)\n");
          Output(Out, "{\n");
        }
      Output(Out, "switch (BltInfo->SourceSurface->iBitmapFormat)\n");
      Output(Out, "{\n");
      for (BppIndex = 0;
           BppIndex < sizeof(SourceBpp) / sizeof(unsigned);
           BppIndex++)
        {
          Output(Out, "case BMF_%uBPP:\n", SourceBpp[BppIndex]);
          Output(Out, "{\n");
          if (Bpp == SourceBpp[BppIndex])
            {
              Output(Out, "if (NULL == BltInfo->XlateSourceToDest ||\n");
              Output(Out, "    0 != (BltInfo->XlateSourceToDest->flXlate & XO_TRIVIAL))\n");
              Output(Out, "{\n");
              Output(Out, "if (BltInfo->DestRect.top < BltInfo->SourcePoint.y)\n");
              Output(Out, "{\n");
              CreateBitCase(Out, Bpp, RopInfo,
                            Flags | FLAG_TRIVIALXLATE,
                            SourceBpp[BppIndex]);
              MARK(Out);
              Output(Out, "}\n");
              Output(Out, "else\n");
              Output(Out, "{\n");
              CreateBitCase(Out, Bpp, RopInfo,
                            Flags | FLAG_BOTTOMUP | FLAG_TRIVIALXLATE,
                            SourceBpp[BppIndex]);
              MARK(Out);
              Output(Out, "}\n");
              Output(Out, "}\n");
              Output(Out, "else\n");
              Output(Out, "{\n");
              Output(Out, "if (BltInfo->DestRect.top < BltInfo->SourcePoint.y)\n");
              Output(Out, "{\n");
              CreateBitCase(Out, Bpp, RopInfo, Flags, SourceBpp[BppIndex]);
              MARK(Out);
              Output(Out, "}\n");
              Output(Out, "else\n");
              Output(Out, "{\n");
              CreateBitCase(Out, Bpp, RopInfo,
                            Flags | FLAG_BOTTOMUP,
                            SourceBpp[BppIndex]);
              MARK(Out);
              Output(Out, "}\n");
              Output(Out, "}\n");
            }
          else
            {
              CreateBitCase(Out, Bpp, RopInfo, Flags,
                            SourceBpp[BppIndex]);
              MARK(Out);
            }
          Output(Out, "break;\n");
          Output(Out, "}\n");
        }
      Output(Out, "}\n");
      if (ROPCODE_GENERIC == RopInfo->RopCode)
        {
          Output(Out, "}\n");
          Output(Out, "else\n");
          Output(Out, "{\n");
          CreateBitCase(Out, Bpp, RopInfo, Flags | FLAG_FORCENOUSESSOURCE, 0);
          MARK(Out);
          Output(Out, "}\n");
        }
    }
  else
    {
      CreateBitCase(Out, Bpp, RopInfo, Flags, 0);
    }
}

static void
CreatePrimitive(FILE *Out, unsigned Bpp, PROPINFO RopInfo)
{
  int First;
  unsigned Partial;

  MARK(Out);
  Output(Out, "\n");
  Output(Out, "static void\n");
  PrintRoutineName(Out, Bpp, RopInfo);
  Output(Out, "(PBLTINFO BltInfo)\n");
  Output(Out, "{\n");
  if (ROPCODE_BLACKNESS == RopInfo->RopCode)
    {
      Output(Out, "DIB_%uBPP_ColorFill(BltInfo->DestSurface, "
                  "&BltInfo->DestRect, 0x0);\n", Bpp);
    }
  else if (ROPCODE_WHITENESS == RopInfo->RopCode)
    {
      Output(Out, "DIB_%uBPP_ColorFill(BltInfo->DestSurface, "
                  "&BltInfo->DestRect, ~0);\n", Bpp);
    }
  else if (ROPCODE_NOOP == RopInfo->RopCode)
    {
      Output(Out, "return;\n");
    }
  else
    {
      Output(Out, "ULONG LineIndex, LineCount;\n");
      Output(Out, "ULONG i;\n");
      if (RopInfo->UsesPattern)
        {
          Output(Out, "ULONG PatternX =0, PatternY = 0, BasePatternX = 0;\n");
        }
      First = 1;
      if (RopInfo->UsesSource)
        {
          Output(Out, "ULONG Source = 0");
          First = 0;
        }
      if (RopInfo->UsesPattern)
        {
          Output(Out, "%s Pattern = 0", First ? "ULONG" : ",");
          First = 0;
        }
      if (! First)
        {
          Output(Out, ";\n");
        }
      Output(Out, "char *DestBase;\n");
      Output(Out, "PULONG DestPtr;\n");
      if (RopInfo->UsesSource)
        {
          Output(Out, "char *SourceBase;\n");
          Output(Out, "PULONG SourcePtr;\n");
          Output(Out, "ULONG RawSource;\n");
          Output(Out, "unsigned SourcePixels, BaseSourcePixels;\n");
        }
      if (32 == Bpp)
        {
          Output(Out, "ULONG CenterCount;\n");
        }
      else
        {
          Output(Out, "ULONG LeftCount, CenterCount, RightCount;\n");
        }
      if (ROPCODE_GENERIC == RopInfo->RopCode)
        {
          Output(Out, "BOOLEAN UsesDest, UsesSource, UsesPattern;\n");
          Output(Out, "\n");
          Output(Out, "UsesDest = ROP4_USES_DEST(BltInfo->Rop4);\n");
          Output(Out, "UsesSource = ROP4_USES_SOURCE(BltInfo->Rop4);\n");
          Output(Out, "UsesPattern = ROP4_USES_PATTERN(BltInfo->Rop4);\n");
        }
      Output(Out, "\n");
      if (! RopInfo->UsesSource)
        {
          CreateBase(Out, 0, 0, Bpp);
          CreateCounts(Out, Bpp);
          MARK(Out);
        }
      Output(Out, "LineCount = BltInfo->DestRect.bottom - BltInfo->DestRect.top;\n");

      Output(Out, "\n");
      if (RopInfo->UsesPattern)
        {
          if (ROPCODE_GENERIC == RopInfo->RopCode)
            {
              Output(Out, "if (UsesPattern && NULL != BltInfo->PatternSurface)\n");
            }
          else
            {
              Output(Out, "if (NULL != BltInfo->PatternSurface)\n");
            }
          Output(Out, "{\n");
          CreateActionBlock(Out, Bpp, RopInfo, FLAG_PATTERNSURFACE);
          MARK(Out);
          Output(Out, "}\n");
          Output(Out, "else\n");
          Output(Out, "{\n");
          if (ROPCODE_GENERIC == RopInfo->RopCode)
            {
              Output(Out, "if (UsesPattern)\n");
              Output(Out, "{\n");
            }
          for (Partial = 0; Partial < 32 / Bpp; Partial++)
            {
              if (0 == Partial)
                {
                  Output(Out, "if (!BltInfo->Brush)\n");
                  Output(Out, "{\n");
                  Output(Out, "Pattern = 0;\n");
                  Output(Out, "}\n");
                  Output(Out, "else\n");
                  Output(Out, "{\n");
                  Output(Out, "Pattern = BltInfo->Brush->iSolidColor");
                }
              else
                {
                  Output(Out, "          (BltInfo->Brush->iSolidColor << %d)",
                         Partial * Bpp);
                }
              if (32 / Bpp <= Partial + 1)
                {
                  Output(Out, ";\n");
                  Output(Out, "}\n");
                }
              else
                {
                  Output(Out, " |\n");
                }
            }
          if (ROPCODE_PATINVERT == RopInfo->RopCode ||
              ROPCODE_MERGECOPY == RopInfo->RopCode)
            {
              Output(Out, "if (0 == Pattern)\n");
              Output(Out, "{\n");
              if (ROPCODE_MERGECOPY == RopInfo->RopCode)
                {
                  Output(Out, "DIB_%uBPP_ColorFill(BltInfo->DestSurface, "
                              "&BltInfo->DestRect, 0x0);\n", Bpp);
                }
              Output(Out, "return;\n");
              Output(Out, "}\n");
            }
          else if (ROPCODE_PATPAINT == RopInfo->RopCode)
            {
              Output(Out, "if ((~0) == Pattern)\n");
              Output(Out, "{\n");
              Output(Out, "DIB_%uBPP_ColorFill(BltInfo->DestSurface, "
                          "&BltInfo->DestRect, ~0);\n", Bpp);
              Output(Out, "return;\n");
              Output(Out, "}\n");
            }
          if (ROPCODE_GENERIC == RopInfo->RopCode)
            {
              Output(Out, "}\n");
            }
          CreateActionBlock(Out, Bpp, RopInfo, 0);
          MARK(Out);
          Output(Out, "}\n");
        }
      else
        {
          CreateActionBlock(Out, Bpp, RopInfo, 0);
          MARK(Out);
        }
    }
  Output(Out, "}\n");
}

static void
CreateTable(FILE *Out, unsigned Bpp)
{
  unsigned RopCode;

  MARK(Out);
  Output(Out, "\n");
  Output(Out, "static void (*PrimitivesTable[256])(PBLTINFO) =\n");
  Output(Out, "{\n");
  for (RopCode = 0; RopCode < 256; RopCode++)
    {
      PrintRoutineName(Out, Bpp, FindRopInfo(RopCode));
      if (RopCode < 255)
        {
          Output(Out, ",");
        }
      Output(Out, "\n");
    }
  Output(Out, "};\n");
}

static void
CreateBitBlt(FILE *Out, unsigned Bpp)
{
  MARK(Out);
  Output(Out, "\n");
  Output(Out, "BOOLEAN\n");
  Output(Out, "DIB_%uBPP_BitBlt(PBLTINFO BltInfo)\n", Bpp);
  Output(Out, "{\n");
  Output(Out, "PrimitivesTable[BltInfo->Rop4 & 0xff](BltInfo);\n");
  Output(Out, "\n");
  Output(Out, "return TRUE;\n");
  Output(Out, "}\n");
}

static void
Generate(char *OutputDir, unsigned Bpp)
{
  FILE *Out;
  unsigned RopCode;
  PROPINFO RopInfo;
  char *FileName;

  FileName = malloc(strlen(OutputDir) + 12);
  if (NULL == FileName)
    {
      fprintf(stderr, "Out of memory\n");
      exit(1);
    }
  strcpy(FileName, OutputDir);
  if ('/' != FileName[strlen(FileName) - 1])
    {
      strcat(FileName, "/");
    }
  sprintf(FileName + strlen(FileName), "dib%ugen.c", Bpp);

  Out = fopen(FileName, "w");
  free(FileName);
  if (NULL == Out)
    {
      perror("Error opening output file");
      exit(1);
    }

  MARK(Out);
  Output(Out, "/* This is a generated file. Please do not edit */\n");
  Output(Out, "\n");
  Output(Out, "#include <win32k.h>\n");
  CreateShiftTables(Out);

  RopInfo = FindRopInfo(ROPCODE_GENERIC);
  CreatePrimitive(Out, Bpp, RopInfo);
  for (RopCode = 0; RopCode < 256; RopCode++)
    {
      RopInfo = FindRopInfo(RopCode);
      if (NULL != RopInfo)
        {
          CreatePrimitive(Out, Bpp, RopInfo);
        }
    }
  CreateTable(Out, Bpp);
  CreateBitBlt(Out, Bpp);

  fclose(Out);
}

int
main(int argc, char *argv[])
{
  unsigned Index;
  static unsigned DestBpp[] =
    { 8, 16, 32 };

  if (argc < 2)
    return 0;

  for (Index = 0; Index < sizeof(DestBpp) / sizeof(DestBpp[0]); Index++)
    {
      Generate(argv[1], DestBpp[Index]);
    }

  return 0;
}

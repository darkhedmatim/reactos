#pragma once

#include <include/engobjects.h>

/* GDI logical font object */
typedef struct _LFONT TEXTOBJ, *PTEXTOBJ;

/*  Internal interface  */

#define  TEXTOBJ_UnlockText(pBMObj) GDIOBJ_vUnlockObject ((POBJ)pBMObj)
NTSTATUS FASTCALL TextIntCreateFontIndirect(CONST LPLOGFONTW lf, HFONT *NewFont);
BOOL FASTCALL InitFontSupport(VOID);
INT FASTCALL FontGetObject(PTEXTOBJ TextObj, INT Count, PVOID Buffer);

BOOL
NTAPI
GreExtTextOutW(
    IN HDC,
    IN INT,
    IN INT,
    IN UINT,
    IN OPTIONAL RECTL*,
    IN LPWSTR,
    IN INT,
    IN OPTIONAL LPINT,
    IN DWORD);

/* $Id$
 *
 * reactos/lib/gdi32/misc/stubs.c
 *
 * GDI32.DLL Stubs for ANSI functions
 *
 * When you implement one of these functions,
 * remove its stub from this file.
 *
 */

#include "precomp.h"

#define UNIMPLEMENTED DbgPrint("GDI32: %s is unimplemented, please try again later.\n", __FUNCTION__);




/*
 * @unimplemented
 */
DWORD
WINAPI
GetCharacterPlacementA(
	HDC		hDc,
	LPCSTR		a1,
	int		a2,
	int		a3,
	LPGCP_RESULTSA	a4,
	DWORD		a5
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
int
WINAPI
StartDocA(
	HDC		hdc,
	CONST DOCINFOA	*lpdi
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
PolyTextOutA(
	HDC			hdc,
	CONST POLYTEXTA		*a1,
	int			a2
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GetLogColorSpaceA(
	HCOLORSPACE		a0,
	LPLOGCOLORSPACEA	a1,
	DWORD			a2
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
GetICMProfileA(
	HDC		hdc,
	LPDWORD pBufSize,
	LPSTR		pszFilename
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
SetICMProfileA(
	HDC	a0,
	LPSTR	a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
int
WINAPI
EnumICMProfilesA(
	HDC		a0,
	ICMENUMPROCA	a1,
	LPARAM		a2
	)
{
  /*
   * FIXME - call NtGdiEnumICMProfiles with NULL for lpstrBuffer
   * to find out how big a buffer we need. Then allocate that buffer
   * and call NtGdiEnumICMProfiles again to have the buffer filled.
   *
   * Finally, step through the buffer ( MULTI-SZ recommended for format ),
   * and convert each string to ANSI, calling the user's callback function
   * until we run out of strings or the user returns FALSE
   */

  UNIMPLEMENTED;
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
wglUseFontBitmapsA(
	HDC		a0,
	DWORD		a1,
	DWORD		a2,
	DWORD		a3
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
wglUseFontOutlinesA(
	HDC			a0,
	DWORD			a1,
	DWORD			a2,
	DWORD			a3,
	FLOAT			a4,
	FLOAT			a5,
	int			a6,
	LPGLYPHMETRICSFLOAT	a7
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
UpdateICMRegKeyA(
	DWORD	a0,
	LPSTR	a1,
	LPSTR	a2,
	UINT	a3
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/*
 * @implemented
 */
UINT
WINAPI
GetStringBitmapA(HDC hdc,
                 LPSTR psz,
                 BOOL DoCall,
                 UINT cj,
                 BYTE *lpSB)
{

    NTSTATUS Status;
    PWSTR pwsz;
    UINT retValue = 0;

    if (DoCall)
    {
        Status = HEAP_strdupA2W ( &pwsz, psz );
        if ( !NT_SUCCESS (Status) )
        {
            SetLastError (RtlNtStatusToDosError(Status));
        }
        else
        {
            retValue = NtGdiGetStringBitmapW(hdc, pwsz, 1, lpSB, cj);
            HEAP_free ( pwsz );
        }
    }

    return retValue;

}


/* EOF */

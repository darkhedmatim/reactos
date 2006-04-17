/*
 *             MAPI basics
 *
 * Copyright 2001 CodeWeavers Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "objbase.h"
#include "mapix.h"
#include "mapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mapi);

LONG MAPI_ObjectCount = 0;

/***********************************************************************
 *              DllMain (MAPI32.init)
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("(%p,%ld,%p)\n", hinstDLL, fdwReason, fImpLoad);

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_PROCESS_DETACH:
	TRACE("DLL_PROCESS_DETACH: %ld objects remaining\n", MAPI_ObjectCount);
	break;
    }
    return TRUE;
}

/***********************************************************************
 * DllCanUnloadNow (MAPI32.28)
 *
 * Determine if this dll can be unloaded from the callers address space.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  S_OK, if the dll can be unloaded,
 *  S_FALSE, otherwise.
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return MAPI_ObjectCount == 0 ? S_OK : S_FALSE;
}

HRESULT WINAPI MAPIInitialize(LPVOID init)
{
    FIXME("(%p) Stub\n", init);
    return SUCCESS_SUCCESS;
}

ULONG WINAPI MAPILogon(ULONG uiparam, LPSTR profile, LPSTR password,
    FLAGS flags, ULONG reserved, LPLHANDLE session)
{
    FIXME("(0x%08lx %s %p 0x%08lx 0x%08lx %p) Stub\n", uiparam,
          debugstr_a(profile), password, flags, reserved, session);

    if (session) *session = 1;
    return SUCCESS_SUCCESS;
}

ULONG WINAPI MAPILogoff(LHANDLE session, ULONG uiparam, FLAGS flags,
    ULONG reserved )
{
    FIXME("(0x%08lx 0x%08lx 0x%08lx 0x%08lx) Stub\n", session,
          uiparam, flags, reserved);
    return SUCCESS_SUCCESS;
}

HRESULT WINAPI MAPILogonEx(ULONG_PTR uiparam, LPWSTR profile,
    LPWSTR password, ULONG flags, LPMAPISESSION *session)
{
    FIXME("(0x%08lx %s %p 0x%08lx %p) Stub\n", uiparam,
          debugstr_w(profile), password, flags, session);
    return SUCCESS_SUCCESS;
}

VOID WINAPI MAPIUninitialize(void)
{
    FIXME("Stub\n");
}

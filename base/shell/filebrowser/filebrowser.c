/*
 * ReactOS Explorer
 *
 * Copyright 2014 Giannis Adamopoulos
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <shobjidl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <stdio.h>

typedef HRESULT (WINAPI *SH_OPEN_NEW_FRAME)(LPITEMIDLIST pidl, IUnknown *paramC, long param10, long param14);

int _tmain(int argc, _TCHAR* argv[])
{
    EXPLORER_CMDLINE_PARSE_RESULTS parseResults = { 0 };

    if (SHExplorerParseCmdLine(&parseResults))
        return SHCreateFromDesktop(&parseResults);

    return 0;
}

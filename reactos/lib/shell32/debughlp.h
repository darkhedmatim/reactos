/*
 * Definitions for debugging
 *
 * Copyright 1998, 2002 Juergen Schmied
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

#ifndef __WINE_SHELL32_DEBUGHLP_H
#define __WINE_SHELL32_DEBUGHLP_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "shlobj.h"

extern void pdump (LPCITEMIDLIST pidl);
extern BOOL pcheck (LPCITEMIDLIST pidl);
extern const char * shdebugstr_guid( const struct _GUID *id );

#endif /* __WINE_SHELL32_DEBUGHLP_H */

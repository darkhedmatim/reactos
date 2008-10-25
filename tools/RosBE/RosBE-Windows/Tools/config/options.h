/*
 * PROJECT:     RosBE Options Dialog
 * LICENSE:     GNU General Public License v2. (see LICENSE.txt)
 * FILE:        Tools/config/options.h
 * PURPOSE:     Configuring RosBE
 * COPYRIGHT:   Copyright 2007 Maarten Bosma
 *              Copyright 2007 Pierre Schweitzer
 *
 */

#include <windows.h>
#include <stdio.h>
#include <shlobj.h>
#include <wchar.h>
#include "resources.h"

#define MINGWVERSION L"\\4.1.3"

typedef struct _SETTINGS
{
    WCHAR logdir[MAX_PATH];
    WCHAR objdir[MAX_PATH];
    WCHAR outdir[MAX_PATH];
    WCHAR mingwpath[MAX_PATH];
    INT foreground;
    INT background;
    BOOL showtime;
    BOOL useccache;
    BOOL strip;
    BOOL nostrip;
    BOOL writelog;
    BOOL objstate;
    BOOL outstate;
}
SETTINGS, *PSETTINGS;

wchar_t *wcsset(wchar_t *string, wchar_t c);

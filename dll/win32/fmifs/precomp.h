/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         File Management IFS Utility functions
 * FILE:            reactos/dll/win32/fmifs/precomp.h
 * PURPOSE:         Win32 FMIFS API Library Header
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Herv� Poussineau (hpoussin@reactos.org)
 */

#ifndef _FMIFS_PCH_
#define _FMIFS_PCH_

/* INCLUDES ******************************************************************/

#define WIN32_NO_STATUS
#define NTOS_MODE_USER
#define UNICODE
#define _UNICODE

#include <stdio.h>

/* PSDK/NDK Headers */
#include <windef.h>
#include <winbase.h>
#include <ndk/rtlfuncs.h>

/* FMIFS Public Header */
#include <fmifs/fmifs.h>

extern LIST_ENTRY ProviderListHead;

typedef struct _IFS_PROVIDER
{
    LIST_ENTRY ListEntry;

    CHKDSKEX ChkdskEx;
    PVOID Extend;
    FORMATEX FormatEx;

    WCHAR Name[1];
} IFS_PROVIDER, *PIFS_PROVIDER;

/* init.c */
PIFS_PROVIDER
GetProvider(
    IN PWCHAR FileSytem);

#endif /* _FMIFS_PCH_ */

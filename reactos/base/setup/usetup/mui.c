/*
 *  ReactOS kernel
 *  Copyright (C) 2008 ReactOS Team
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
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/mui.c
 * PURPOSE:         Text-mode setup
 * PROGRAMMER:      
 */

#include "usetup.h"
#include "errorcode.h"
#include "mui.h"

#define NDEBUG
#include <debug.h>

#include "lang/en-US.h"
#include "lang/de-DE.h"
#include "lang/el-GR.h"
#include "lang/es-ES.h"
#include "lang/fr-FR.h"
#include "lang/it-IT.h"
#include "lang/pl-PL.h"
#include "lang/ru-RU.h"
#include "lang/sv-SE.h"
#include "lang/uk-UA.h"

static MUI_LANGUAGE LanguageList[] =
{
    {
        L"00000409",        /* The Language ID */
        L"00000409",        /* Default Keyboard Layout for this language */
        L"1252",            /* ANSI Codepage */
        L"437",             /* OEM Codepage */
        L"10000",           /* MAC Codepage */
        L"English",         /* Language Name , not used just to make things easier when updating this file */
        enUSPages,          /* Translated page strings  */
        enUSErrorEntries    /* Translated error strings */
    },
    {
        L"0000040C",
        L"0000040C",
        L"1252",
        L"850",
        L"10000",
        L"French",
        frFRPages,
        frFRErrorEntries
    },
    {
        L"00000407",
        L"00000407",
        L"1252",
        L"850",
        L"10000",
        L"German",
        deDEPages,
        deDEErrorEntries
    },
    {
        L"00000408",
        L"00000409",
        L"1253",
        L"737",
        L"10006",
        L"Greek",
        elGRPages,
        elGRErrorEntries
    },
    {
        L"00000410",
        L"00000410",
        L"1252",
        L"850",
        L"10000",
        L"Italian",
        itITPages,
        itITErrorEntries
    },
    {
        L"00000419",
        L"00000419",
        L"1251",
        L"866",
        L"10007",
        L"Russian",
        ruRUPages,
        ruRUErrorEntries
    },
    {
        L"0000040A",
        L"0000040A",
        L"1252",
        L"850",
        L"10000",
        L"Spanish",
        esESPages,
        esESErrorEntries
    },
    {
        L"00000415",
        L"00000415",
        L"1250",
        L"852",
        L"10029",
        L"Polish",
        plPLPages,
        plPLErrorEntries
    },
    {
        L"0000041D",
        L"0000041D",
        L"1252",
        L"850",
        L"10000",
        L"Swedish",
        svSEPages,
        svSEErrorEntries
    },
    {
        L"00000422",
        L"00000422",
        L"1251",
        L"866",
        L"10017",
        L"Ukrainian",
        ukUAPages,
        ukUAErrorEntries
    },
    {
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    }
};

extern
VOID
PopupError(PCHAR Text,
	   PCHAR Status,
	   PINPUT_RECORD Ir,
	   ULONG WaitEvent);

static
MUI_ENTRY *
FindMUIEntriesOfPage (ULONG PageNumber)
{
    ULONG muiIndex = 0;
    ULONG lngIndex = 0;
    MUI_PAGE * Pages = NULL;

    do
    {
        /* First we search the language list till we find current selected language messages */
        if (_wcsicmp(LanguageList[lngIndex].LanguageID , SelectedLanguageId) == 0)
        {
            /* Get all available pages for this language */
            Pages = LanguageList[lngIndex].MuiPages;

            do
            {
                /* Get page messages */
                if (Pages[muiIndex].Number == PageNumber)
                    return Pages[muiIndex].MuiEntry;

                muiIndex++;
            }
            while (Pages[muiIndex].MuiEntry != NULL);
        }

        lngIndex++;
    }
    while (LanguageList[lngIndex].MuiPages != NULL);

    return NULL;
}

static
MUI_ERROR *
FindMUIErrorEntries ()
{
    ULONG lngIndex = 0;

    do
    {
        /* First we search the language list till we find current selected language messages */
        if (_wcsicmp(LanguageList[lngIndex].LanguageID , SelectedLanguageId) == 0)
        {
            /* Get all available error messages for this language */
            return LanguageList[lngIndex].MuiErrors;
        }

        lngIndex++;
    }
    while (LanguageList[lngIndex].MuiPages != NULL);

    return NULL;
}

VOID
MUIDefaultKeyboardLayout(WCHAR * KeyboardLayout)
{
    ULONG lngIndex = 0;
    do
    {
        /* First we search the language list till we find current selected language messages */
        if (_wcsicmp(LanguageList[lngIndex].LanguageID , SelectedLanguageId) == 0)
        {
            /* Get all available error messages for this language */
            wcscpy(KeyboardLayout, LanguageList[lngIndex].LanguageKeyboardLayoutID);
            return;
        }

        lngIndex++;
    }
    while (LanguageList[lngIndex].MuiPages != NULL);

    KeyboardLayout[0] = L'\0';
}

VOID
MUIDisplayPage(ULONG page)
{
    MUI_ENTRY * entry;
    int index;
    int flags;

    entry = FindMUIEntriesOfPage (page);
    if (!entry)
    {
        PopupError("Error: Failed to find translated page",
                   NULL,
                   NULL,
                   POPUP_WAIT_NONE);
        return;
    }

    index = 0;
    do
    {
        flags = entry[index].Flags;
        switch(flags)
        {
            case TEXT_NORMAL:
                CONSOLE_SetTextXY(entry[index].X, entry[index].Y, entry[index].Buffer);
                break;
            case TEXT_HIGHLIGHT:
                CONSOLE_SetHighlightedTextXY(entry[index].X, entry[index].Y, entry[index].Buffer);
                break;
            case TEXT_UNDERLINE:
                CONSOLE_SetUnderlinedTextXY(entry[index].X, entry[index].Y, entry[index].Buffer);
                break;
            case TEXT_STATUS:
                CONSOLE_SetStatusText(entry[index].Buffer);
                break;
            default:
                break;
        }
        index++;
    }
    while (entry[index].Buffer != NULL);
}

VOID
MUIDisplayError(ULONG ErrorNum, PINPUT_RECORD Ir, ULONG WaitEvent)
{
    MUI_ERROR * entry;

    if (ErrorNum >= ERROR_LAST_ERROR_CODE)
    {
        PopupError("Pnvalid error number provided",
                   "Press ENTER to continue",
                   Ir,
                   POPUP_WAIT_ENTER);

        return;
    }

    entry = FindMUIErrorEntries ();
    if (!entry)
    {
        PopupError("Error: Failed to find translated error message",
                   NULL,
                   NULL,
                   POPUP_WAIT_NONE);
        return;
    }

    PopupError(entry[ErrorNum].ErrorText,
               entry[ErrorNum].ErrorStatus,
               Ir,
               WaitEvent);
}

static BOOLEAN
AddCodepageToRegistry(PWCHAR ACPage, PWCHAR OEMCPage, PWCHAR MACCPage)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    HANDLE KeyHandle;
    NTSTATUS Status;

    // Open the nls codepage key
    RtlInitUnicodeString(&KeyName,
		                 L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\NLS\\CodePage");
    InitializeObjectAttributes(&ObjectAttributes,
			                   &KeyName,
			                   OBJ_CASE_INSENSITIVE,
			                   NULL,
			                   NULL);
    Status =  NtOpenKey(&KeyHandle,
		                KEY_ALL_ACCESS,
		                &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenKey() failed (Status %lx)\n", Status);
        return FALSE;
    }

    // Set ANSI codepage
    RtlInitUnicodeString(&ValueName, L"ACP");
    Status = NtSetValueKey(KeyHandle,
			                &ValueName,
			                0,
			                REG_SZ,
			                (PVOID)ACPage,
			                4 * sizeof(PWCHAR));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
        NtClose(KeyHandle);
        return FALSE;
    }

    // Set OEM codepage
    RtlInitUnicodeString(&ValueName, L"OEMCP");
    Status = NtSetValueKey(KeyHandle,
			               &ValueName,
			               0,
			               REG_SZ,
			               (PVOID)OEMCPage,
			               3 * sizeof(PWCHAR));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
        NtClose(KeyHandle);
        return FALSE;
    }

    // Set MAC codepage
    RtlInitUnicodeString(&ValueName, L"MACCP");
    Status = NtSetValueKey(KeyHandle,
			               &ValueName,
			               0,
			               REG_SZ,
			               (PVOID)MACCPage,
			               5 * sizeof(PWCHAR));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
        NtClose(KeyHandle);
        return FALSE;
    }

    NtClose(KeyHandle);

    return TRUE;
}

BOOLEAN
AddCodePage(VOID)
{
    ULONG lngIndex = 0;
    do
    {
        if (_wcsicmp(LanguageList[lngIndex].LanguageID , SelectedLanguageId) == 0)
        {
            return AddCodepageToRegistry(LanguageList[lngIndex].ACPage,
                                         LanguageList[lngIndex].OEMCPage,
                                         LanguageList[lngIndex].MACCPage);
        }

        lngIndex++;
    }
    while (LanguageList[lngIndex].MuiPages != NULL);
	return FALSE;
}

/* EOF */

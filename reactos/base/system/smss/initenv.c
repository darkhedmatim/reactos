/*
 * initenv.c - Environment initialization
 *
 * ReactOS Operating System
 *
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.
 *
 * --------------------------------------------------------------------
 */

#include "smss.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS */

PWSTR SmSystemEnvironment = NULL;


/* FUNCTIONS */

NTSTATUS
SmCreateEnvironment(VOID)
{
    return RtlCreateEnvironment(FALSE, &SmSystemEnvironment);
}


static NTSTATUS
SmpSetEnvironmentVariable(IN PVOID Context,
                          IN PWSTR ValueName,
                          IN PVOID ValueData)
{
    UNICODE_STRING EnvVariable;
    UNICODE_STRING EnvValue;

    RtlInitUnicodeString(&EnvVariable,
                         ValueName);
    RtlInitUnicodeString(&EnvValue,
                         (PWSTR)ValueData);
    return RtlSetEnvironmentVariable(Context,
                              &EnvVariable,
                              &EnvValue);
}


static NTSTATUS STDCALL
SmpEnvironmentQueryRoutine(IN PWSTR ValueName,
                           IN ULONG ValueType,
                           IN PVOID ValueData,
                           IN ULONG ValueLength,
                           IN PVOID Context,
                           IN PVOID EntryContext)
{
    DPRINT("ValueName '%S'  Type %lu  Length %lu\n", ValueName, ValueType, ValueLength);

    if (ValueType != REG_SZ && ValueType != REG_EXPAND_SZ)
        return STATUS_SUCCESS;

    DPRINT("ValueData '%S'\n", (PWSTR)ValueData);
    return SmpSetEnvironmentVariable(Context,ValueName,ValueData);
}


NTSTATUS
SmSetEnvironmentVariables(VOID)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    WCHAR ValueBuffer[MAX_PATH];
    NTSTATUS Status;

    /*
     * The following environment variables must be set prior to reading
     * other variables from the registry.
     *
     * Variables (example):
     *    SystemRoot = "C:\reactos"
     *    SystemDrive = "C:"
     */

    /* Copy system root into value buffer */
    wcscpy(ValueBuffer,
           SharedUserData->NtSystemRoot);

    /* Set SystemRoot = "C:\reactos" */
    SmpSetEnvironmentVariable(&SmSystemEnvironment, L"SystemRoot", ValueBuffer);

    /* Cut off trailing path */
    ValueBuffer[2] = 0;

    /* Set SystemDrive = "C:" */
    SmpSetEnvironmentVariable(&SmSystemEnvironment, L"SystemDrive", ValueBuffer);

    /* Read system environment from the registry. */
    RtlZeroMemory(&QueryTable,
                  sizeof(QueryTable));

    QueryTable[0].QueryRoutine = SmpEnvironmentQueryRoutine;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
                                    L"Session Manager\\Environment",
                                    QueryTable,
                                    &SmSystemEnvironment,
                                    SmSystemEnvironment);

    return Status;
}

/**********************************************************************
 *  Set environment variables from registry
 */
NTSTATUS
SmUpdateEnvironment(VOID)
{
    /* TODO */
    return STATUS_SUCCESS;
}

/* EOF */

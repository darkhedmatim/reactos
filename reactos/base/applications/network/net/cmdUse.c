/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:            base/applications/network/net/cmdUse.c
 * PURPOSE:
 *
 * PROGRAMMERS:     Pierre Schweitzer
 */

#include "net.h"

static
DWORD
EnumerateConnections(LPCWSTR Local)
{
    DWORD dRet;
    HANDLE hEnum;
    LPNETRESOURCE lpRes;
    DWORD dSize = 0x1000;
    DWORD dCount = -1;
    LPNETRESOURCE lpCur;

    ConPrintf(StdOut, L"%s\t\t\t%s\t\t\t\t%s\n", L"Local", L"Remote", L"Provider");

    dRet = WNetOpenEnum(RESOURCE_CONNECTED, RESOURCETYPE_DISK, 0, NULL, &hEnum);
    if (dRet != WN_SUCCESS)
    {
        return 1;
    }

    lpRes = HeapAlloc(GetProcessHeap(), 0, dSize);
    if (!lpRes)
    {
        WNetCloseEnum(hEnum);
        return 1;
    }

    do
    {
        dSize = 0x1000;
        dCount = -1;

        memset(lpRes, 0, dSize);
        dRet = WNetEnumResource(hEnum, &dCount, lpRes, &dSize);
        if (dRet == WN_SUCCESS || dRet == WN_MORE_DATA)
        {
            lpCur = lpRes;
            for (; dCount; dCount--)
            {
                if (!Local || wcsicmp(lpCur->lpLocalName, Local) == 0)
                {
                    ConPrintf(StdOut, L"%s\t\t\t%s\t\t%s\n", lpCur->lpLocalName, lpCur->lpRemoteName, lpCur->lpProvider);
                }

                lpCur++;
            }
        }
    } while (dRet != WN_NO_MORE_ENTRIES);

    HeapFree(GetProcessHeap(), 0, lpRes);
    WNetCloseEnum(hEnum);

    return 0;
}

static
VOID
PrintError(DWORD Status)
{
    LPWSTR Buffer;

    ConResPrintf(StdErr, IDS_ERROR_SYSTEM_ERROR, Status);

    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, Status, 0, (LPWSTR)&Buffer, 0, NULL))
    {
        ConPrintf(StdErr, L"\n%s", Buffer);
        LocalFree(Buffer);
    }
}

INT
cmdUse(
    INT argc,
    WCHAR **argv)
{
    DWORD Status, Len;

    if (argc == 2)
    {
        Status = EnumerateConnections(NULL);
        if (Status == NO_ERROR)
            ConResPrintf(StdOut, IDS_ERROR_NO_ERROR);
        else
            PrintError(Status);

        return 0;
    }
    else if (argc == 3)
    {
        Len = wcslen(argv[2]);
        if (Len != 2)
        {
            ConResPrintf(StdErr, IDS_ERROR_INVALID_OPTION_VALUE, L"DeviceName");
            return 1;
        }

        if (!iswalpha(argv[2][0]) || argv[2][1] != L':')
        {
            ConResPrintf(StdErr, IDS_ERROR_INVALID_OPTION_VALUE, L"DeviceName");
            return 1;
        }

        Status = EnumerateConnections(argv[2]);
        if (Status == NO_ERROR)
            ConResPrintf(StdOut, IDS_ERROR_NO_ERROR);
        else
            PrintError(Status);

        return 0;
    }

    Len = wcslen(argv[2]);
    if (Len != 1 && Len != 2)
    {
        ConResPrintf(StdErr, IDS_ERROR_INVALID_OPTION_VALUE, L"DeviceName");
        return 1;
    }

    if (Len == 2 && argv[2][1] != L':')
    {
        ConResPrintf(StdErr, IDS_ERROR_INVALID_OPTION_VALUE, L"DeviceName");
        return 1;
    }

    if (argv[2][0] != L'*' && !iswalpha(argv[2][0]))
    {
        ConResPrintf(StdErr, IDS_ERROR_INVALID_OPTION_VALUE, L"DeviceName");
        return 1;
    }

    if (wcsicmp(argv[3], L"/DELETE") == 0)
    {
        if (argv[2][0] == L'*')
        {
            ConResPrintf(StdErr, IDS_ERROR_INVALID_OPTION_VALUE, L"DeviceName");
            return 1;
        }

        return WNetCancelConnection2(argv[2], CONNECT_UPDATE_PROFILE, FALSE);
    }
    else
    {
        BOOL Persist = FALSE;
        NETRESOURCE lpNet;
        WCHAR Access[256];
        DWORD OutFlags = 0, Size = ARRAYSIZE(Access);

        Len = wcslen(argv[3]);
        if (Len < 4)
        {
            ConResPrintf(StdErr, IDS_ERROR_INVALID_OPTION_VALUE, L"Name");
            return 1;
        }

        if (argv[3][0] != L'\\' || argv[3][1] != L'\\')
        {
            ConResPrintf(StdErr, IDS_ERROR_INVALID_OPTION_VALUE, L"Name");
            return 1;
        }

        if (argc > 4)
        {
            LPWSTR Cpy;
            Len = wcslen(argv[4]);
            if (Len > 12)
            {
                Cpy = HeapAlloc(GetProcessHeap(), 0, (Len + 1) * sizeof(WCHAR));
                if (Cpy)
                {
                    INT i;
                    for (i = 0; i < Len; ++i)
                        Cpy[i] = towupper(argv[4][i]);

                    if (wcsstr(Cpy, L"/PERSISTENT:") == Cpy)
                    {
                        LPWSTR Arg = Cpy + 12;
                        if (Len == 14 && Arg[0] == 'N' && Arg[1] == 'O')
                        {
                            Persist = FALSE;
                        }
                        else if (Len == 15 && Arg[0] == 'Y' && Arg[1] == 'E' && Arg[2] == 'S')
                        {
                            Persist = TRUE;
                        }
                        else
                        {
                            HeapFree(GetProcessHeap(), 0, Cpy);
                            ConResPrintf(StdErr, IDS_ERROR_INVALID_OPTION_VALUE, L"Persistent");
                            return 1;
                        }
                    }
                    HeapFree(GetProcessHeap(), 0, Cpy);
                }
            }

        }

        lpNet.dwType = RESOURCETYPE_DISK;
        lpNet.lpLocalName = (argv[2][0] != L'*') ? argv[2] : NULL;
        lpNet.lpRemoteName = argv[3];
        lpNet.lpProvider = NULL;

        Status = WNetUseConnection(NULL, &lpNet, NULL, NULL, CONNECT_REDIRECT | (Persist ? CONNECT_UPDATE_PROFILE : 0), Access, &Size, &OutFlags);
        if (argv[2][0] == L'*' && Status == NO_ERROR && OutFlags == CONNECT_LOCALDRIVE)
            ConResPrintf(StdOut, IDS_USE_NOW_CONNECTED, argv[3], Access);
        else if (Status != NO_ERROR)
            PrintError(Status);

        return Status;
    }
}

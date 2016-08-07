/*
 * PROJECT:         input.dll
 * FILE:            dll/cpl/input/layout_list.c
 * PURPOSE:         input.dll
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "layout_list.h"


static LAYOUT_LIST_NODE *_LayoutList = NULL;


static LAYOUT_LIST_NODE*
LayoutList_Append(DWORD dwId, DWORD dwSpecialId, const WCHAR *pszName, const WCHAR *pszFile)
{
    LAYOUT_LIST_NODE *pCurrent;
    LAYOUT_LIST_NODE *pNew;

    if (pszName == NULL || pszFile == NULL)
        return NULL;

    pCurrent = _LayoutList;

    pNew = (LAYOUT_LIST_NODE*)malloc(sizeof(LAYOUT_LIST_NODE));
    if (pNew == NULL)
        return NULL;

    memset(pNew, 0, sizeof(LAYOUT_LIST_NODE));

    pNew->pszName = DublicateString(pszName);
    if (pNew->pszName == NULL)
    {
        free(pNew);
        return NULL;
    }

    pNew->pszFile = DublicateString(pszFile);
    if (pNew->pszFile == NULL)
    {
        free(pNew->pszName);
        free(pNew);
        return NULL;
    }

    pNew->dwId = dwId;
    pNew->dwSpecialId = dwSpecialId;

    if (pCurrent == NULL)
    {
        _LayoutList = pNew;
    }
    else
    {
        while (pCurrent->pNext != NULL)
        {
            pCurrent = pCurrent->pNext;
        }

        pNew->pPrev = pCurrent;
        pCurrent->pNext = pNew;
    }

    return pNew;
}


VOID
LayoutList_Destroy(VOID)
{
    LAYOUT_LIST_NODE *pCurrent;

    if (_LayoutList == NULL)
        return;

    pCurrent = _LayoutList;

    while (pCurrent != NULL)
    {
        LAYOUT_LIST_NODE *pNext = pCurrent->pNext;

        free(pCurrent->pszName);
        free(pCurrent->pszFile);
        free(pCurrent);

        pCurrent = pNext;
    }

    _LayoutList = NULL;
}


VOID
LayoutList_Create(VOID)
{
    WCHAR szSystemDirectory[MAX_PATH];
    WCHAR szLayoutId[MAX_PATH];
    DWORD dwIndex = 0;
    DWORD dwSize;
    HKEY hKey;

    GetSystemDirectoryW(szSystemDirectory, ARRAYSIZE(szSystemDirectory));

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts",
                      0,
                      KEY_ENUMERATE_SUB_KEYS,
                      &hKey) != ERROR_SUCCESS)
    {
        return;
    }

    dwSize = sizeof(szLayoutId);

    while (RegEnumKeyExW(hKey, dwIndex, szLayoutId, &dwSize,
                         NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        HKEY hLayoutKey;

        if (RegOpenKeyExW(hKey,
                          szLayoutId,
                          0,
                          KEY_QUERY_VALUE,
                          &hLayoutKey) == ERROR_SUCCESS)
        {
            WCHAR szBuffer[MAX_PATH];

            dwSize = sizeof(szBuffer);

            if (RegQueryValueExW(hLayoutKey,
                                 L"Layout File",
                                 NULL, NULL,
                                 (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS)
            {
                WCHAR szFilePath[MAX_PATH];

                StringCchPrintfW(szFilePath, ARRAYSIZE(szFilePath),
                                 L"%s\\%s", szSystemDirectory, szBuffer);

                if (GetFileAttributes(szFilePath) != INVALID_FILE_ATTRIBUTES)
                {
                    DWORD dwSpecialId = 0;
                    WCHAR *pszEnd;

                    dwSize = sizeof(szBuffer);

                    if (RegQueryValueExW(hLayoutKey,
                                         L"Layout Id",
                                         NULL, NULL,
                                         (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS)
                    {
                        dwSpecialId = wcstoul(szBuffer, &pszEnd, 16);
                    }

                    dwSize = sizeof(szBuffer);

                    if (RegQueryValueExW(hLayoutKey,
                                         L"Layout Text",
                                         NULL, NULL,
                                         (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS)
                    {
                        DWORD dwLayoutId;

                        dwLayoutId = wcstoul(szLayoutId, &pszEnd, 16);

                        LayoutList_Append(dwLayoutId,
                                          dwSpecialId,
                                          szBuffer,
                                          szFilePath);
                    }
                }
            }

            RegCloseKey(hLayoutKey);
        }

        dwSize = sizeof(szLayoutId);
        ++dwIndex;
    }

    RegCloseKey(hKey);
}


WCHAR*
LayoutList_GetNameByHkl(HKL hkl)
{
    LAYOUT_LIST_NODE *pCurrent;

    if ((HIWORD(hkl) & 0xF000) == 0xF000)
    {
        DWORD dwSpecialId = (HIWORD(hkl) & 0x0FFF);

        for (pCurrent = _LayoutList; pCurrent != NULL; pCurrent = pCurrent->pNext)
        {
            if (dwSpecialId == pCurrent->dwSpecialId)
            {
                return pCurrent->pszName;
            }
        }
    }
    else
    {
        for (pCurrent = _LayoutList; pCurrent != NULL; pCurrent = pCurrent->pNext)
        {
            if (HIWORD(hkl) == LOWORD(pCurrent->dwId))
            {
                return pCurrent->pszName;
            }
        }
    }

    return NULL;
}


LAYOUT_LIST_NODE*
LayoutList_Get(VOID)
{
    return _LayoutList;
}

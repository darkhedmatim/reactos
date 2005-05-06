/*
 * UNIXFS - Shell namespace extension for the unix filesystem
 *
 * Copyright (C) 2005 Michael Jung
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

#include <stdio.h>
#include <stdarg.h>
#include <dirent.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "wine/debug.h"

#include "shell32_main.h"
#include "shfldr.h"
#include "shresdef.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

const GUID CLSID_UnixFolder = {0xcc702eb2, 0x7dc5, 0x11d9, {0xc6, 0x87, 0x00, 0x04, 0x23, 0x8a, 0x01, 0xcd}};

#define ADJUST_THIS(c,m,p) ((c*)(((long)p)-(long)&(((c*)0)->lp##m##Vtbl)))
#define STATIC_CAST(i,p) ((i*)&p->lp##i##Vtbl)

/******************************************************************************
 * UNIXFS_path_to_pidl [Internal]
 *
 * PARAMS
 *  path  [I] An absolute unix path 
 *  ppidl [O] The corresponding ITEMIDLIST. Release with SHFree/ILFree
 *  
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE, invalid params, path not absolute or out of memory
 *
 * NOTES
 *  'path' has to be an absolute unix filesystem path starting and
 *  ending with a slash ('/'). Currently, only directories (no files)
 *  are accepted.
 */
static BOOL UNIXFS_path_to_pidl(char *path, LPITEMIDLIST *ppidl) {
    LPITEMIDLIST pidl;
    int cSubDirs, cPidlLen;
    char *pSlash, *pSubDir;

    TRACE("path=%s, ppidl=%p", debugstr_a(path), ppidl);
    
    /* Fail, if no absolute path given */
    if (!ppidl || !path || path[0] != '/') return FALSE;
   
    /* Count the number of sub-directories in the path */
    cSubDirs = 0;
    pSlash = strchr(path, '/');
    while (pSlash) {
        cSubDirs++;
        pSlash = strchr(pSlash+1, '/');
    }
   
    /* Allocate enough memory to hold the path */
    cPidlLen = strlen(path) + cSubDirs * sizeof(USHORT) + sizeof(USHORT);
    *ppidl = pidl = (LPITEMIDLIST)SHAlloc(cPidlLen);
    if (!pidl) return FALSE;

    /* Start with a SHITEMID for the root directory */
    pidl->mkid.cb = 3;
    pidl->mkid.abID[0] = '/';
    pidl = ILGetNext(pidl);

    /* Append SHITEMIDs for the sub-directories */
    pSubDir = path + 1;
    pSlash = strchr(pSubDir, '/');
    while (pSlash) {
        pidl->mkid.cb = (USHORT)(pSlash+3-pSubDir);
        memcpy(pidl->mkid.abID, pSubDir, pidl->mkid.cb);
        pSubDir = pSlash + 1;
        pSlash = strchr(pSubDir, '/');
        pidl = ILGetNext(pidl);
    }
    pidl->mkid.cb = 0; /* Terminate the ITEMIDLIST */
   
    /* Path doesn't end with a '/' */
    if (*pSubDir) 
        WARN("Path '%s' not in canonical form.\n", path);
    
    return TRUE;
}

/******************************************************************************
 * UNIXFS_pidl_to_path [Internal]
 *
 * Construct the unix path that corresponds to a fully qualified ITEMIDLIST
 *
 * PARAMS
 *  pidl [I] ITEMIDLIST that specifies the absolute location of the folder
 *  path [O] The corresponding unix path as a zero terminated ascii string
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE, pidl doesn't specify a unix path or out of memory
 */
static BOOL UNIXFS_pidl_to_path(LPCITEMIDLIST pidl, PSZ *path) {
    LPCITEMIDLIST current = pidl, root;
    DWORD dwPathLen;
    char *pNextDir;

    TRACE("(pidl=%p, path=%p)\n", pidl, path);
    
    *path = NULL;

    /* Find the UnixFolderClass root */
    while (current->mkid.cb) {
        if (current->mkid.cb < sizeof(GUID)+4) return FALSE;
        if (IsEqualIID(&CLSID_UnixFolder, &current->mkid.abID[2])) break;
        current = ILGetNext(current);
    }
    if (!current->mkid.cb) return FALSE;
    root = ILGetNext(current);

    /* Determine the path's length bytes */
    dwPathLen = 1; /* For the terminating '\0' */
    current = root;
    while (current->mkid.cb) {
        dwPathLen += current->mkid.cb - sizeof(USHORT);
        current = ILGetNext(current);
    };

    /* Build the path */
    *path = pNextDir = SHAlloc(dwPathLen);
    if (!path) {
        WARN("SHAlloc failed!\n");
        return FALSE;
    }
    current = root;
    while (current->mkid.cb) {
        memcpy(pNextDir, current->mkid.abID, current->mkid.cb - sizeof(USHORT));
        pNextDir += current->mkid.cb - sizeof(USHORT);
        current = ILGetNext(current);
    }
    *pNextDir='\0';
    
    TRACE("resulting path: %s\n", *path);
    return TRUE;
}

/******************************************************************************
 * UNIXFS_build_subfolder_pidls [Internal]
 *
 * Builds an array of subfolder PIDLs relative to a unix directory
 *
 * PARAMS
 *  path   [I] Name of a unix directory as a zero terminated ascii string
 *  apidl  [O] The array of PIDLs
 *  pCount [O] Size of apidl
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE, path is not a valid unix directory or out of memory
 *
 * NOTES
 *  The array of PIDLs and each PIDL are allocated with SHAlloc. You'll have
 *  to release each PIDL as well as the array itself with SHFree.
 */
static BOOL UNIXFS_build_subfolder_pidls(const char *path, LPITEMIDLIST **apidl, DWORD *pCount)
{
    DIR *dir;
    struct dirent *pSubDir;
    DWORD cSubDirs, i;
    USHORT sLen;

    TRACE("(path=%s, apidl=%p, pCount=%p)\n", path, apidl, pCount);
    
    *apidl = NULL;
    *pCount = 0;
   
    /* Special case for 'My UNIX Filesystem' shell folder:
     * The unix root directory is the only child of 'My UNIX Filesystem'. 
     */
    if (!strlen(path)) {
        LPSHITEMID pid;
        
        pid = (LPSHITEMID)SHAlloc(1 + 2 * sizeof(USHORT));
        pid->cb = 3;
        pid->abID[0] = '/';
        memset(((PBYTE)pid)+pid->cb, 0, sizeof(USHORT));
        
        *apidl = SHAlloc(sizeof(LPITEMIDLIST));
        (*apidl)[0] = (LPITEMIDLIST)pid;
        *pCount = 1;
    
        return TRUE;
    }
    
    dir = opendir(path);
    if (!dir) {
        WARN("Failed to open directory '%s'.\n", path);
        return FALSE;
    }

    /* Count number of sub directories.
     */
    for (cSubDirs = 0, pSubDir = readdir(dir); pSubDir; pSubDir = readdir(dir)) { 
        if (strcmp(pSubDir->d_name, ".") && 
            strcmp(pSubDir->d_name, "..") && 
            pSubDir->d_type == DT_DIR) 
        {
            cSubDirs++;
        }
    }

    /* If there are no subdirectories, we are done. */
    if (cSubDirs == 0) {
        closedir(dir);
        return TRUE;
    }

    /* Allocate the array of PIDLs */
    *apidl = SHAlloc(cSubDirs * sizeof(LPITEMIDLIST));
    if (!apidl) {
        WARN("SHAlloc failed!\n");
        return FALSE;
    }
   
    /* Allocate and initialize one SHITEMID per sub-directory. */
    for (rewinddir(dir), pSubDir = readdir(dir), i = 0; pSubDir; pSubDir = readdir(dir)) {
        LPSHITEMID pid;
            
        if (!strcmp(pSubDir->d_name, ".") ||
            !strcmp(pSubDir->d_name, "..") || 
            pSubDir->d_type != DT_DIR) 
        {
            continue;
        }
    
        sLen = strlen(pSubDir->d_name)+1; /* For the trailing '/' */
        pid = (LPSHITEMID)SHAlloc(sLen + 2 * sizeof(USHORT));
        if (!pid) {
            WARN("SHAlloc failed!\n");
            return FALSE;
        }

        pid->cb = (USHORT)(sLen + sizeof(USHORT));
        memcpy(pid->abID, pSubDir->d_name, sLen-1);
        pid->abID[sLen-1] = '/';
        memset(((PBYTE)pid)+pid->cb, 0, sizeof(USHORT));

        (*apidl)[i++] = (LPITEMIDLIST)pid;
    }
    
    *pCount = i;    
    closedir(dir);

    return TRUE;
}

/******************************************************************************
 * UnixFolderIcon 
 *
 * Singleton class, which is used by the shell to extract icons to represent
 * folders in tree- and listviews. Currently, all this singleton does is to
 * provide the shell with the absolute path to "shell32.dll" and with the 
 * indices of the closed and opened folder icons in the resources of this dll.
 */

/* UnixFolderIcon object layout and typedef.
 */
typedef struct _UnixFolderIcon {
    const IExtractIconWVtbl *lpIExtractIconWVtbl;
} UnixFolderIcon;

static HRESULT WINAPI UnixFolderIcon_IExtractIconW_QueryInterface(IExtractIconW *iface, REFIID riid, 
    void **ppv) 
{
    TRACE("(iface=%p, riid=%p, ppv=%p)\n", iface, riid, ppv);
    
    if (!ppv) return E_INVALIDARG;
    
    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IExtractIconW, riid))
    {
        *ppv = iface;
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IExtractIconW_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI UnixFolderIcon_IExtractIconW_AddRef(IExtractIconW *iface) {
    TRACE("(iface=%p)\n", iface);
    return 2;
}

static ULONG WINAPI UnixFolderIcon_IExtractIconW_Release(IExtractIconW *iface) {
    TRACE("(iface=%p)\n", iface);
    return 1;
}

static HRESULT WINAPI UnixFolderIcon_IExtractIconW_GetIconLocation(IExtractIconW *iface, 
    UINT uFlags, LPWSTR szIconFile, UINT cchMax, INT* piIndex, UINT* pwFlags)
{
    TRACE("(iface=%p, uFlags=%u, szIconFile=%s, cchMax=%u, piIndex=%p, pwFlags=%p)\n",
            iface, uFlags, debugstr_w(szIconFile), cchMax, piIndex, pwFlags);
    
    lstrcpynW(szIconFile, swShell32Name, cchMax);
    *piIndex = (uFlags & GIL_OPENICON) ? 4 : 3;
    *pwFlags = 0;

    return S_OK;
}

static HRESULT WINAPI UnixFolderIcon_IExtractIconW_Extract(
    IExtractIconW *iface, LPCWSTR pszFile, UINT nIconIndex, HICON* phiconLarge, HICON* phiconSmall, 
    UINT nIconSize)
{
    TRACE("(iface=%p, pszFile=%s, nIconIndex=%u, phiconLarge=%p, phiconSmall=%p, nIconSize=%u)"
          "stub\n", iface, debugstr_w(pszFile), nIconIndex, phiconLarge, phiconSmall, nIconSize);

    return E_NOTIMPL;
}

/* VTable for the IExtractIconW interface of the UnixFolderIcon class. 
 */
static const IExtractIconWVtbl UnixFolderIcon_IExtractIconW_Vtbl = {
    UnixFolderIcon_IExtractIconW_QueryInterface,
    UnixFolderIcon_IExtractIconW_AddRef,
    UnixFolderIcon_IExtractIconW_Release,
    UnixFolderIcon_IExtractIconW_GetIconLocation,
    UnixFolderIcon_IExtractIconW_Extract
};

/* The singleton instance
 */
UnixFolderIcon UnixFolderIconSingleton = { &UnixFolderIcon_IExtractIconW_Vtbl };

/******************************************************************************
 * UnixFolder
 *
 * Class whose heap based instances represent unix filesystem directories.
 */

static shvheader GenericSFHeader[] = {
    {IDS_SHV_COLUMN1, SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
    {IDS_SHV_COLUMN2, SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN3, SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN4, SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 12},
    {IDS_SHV_COLUMN5, SHCOLSTATE_TYPE_STR  | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 5}
};

#define GENERICSHELLVIEWCOLUMNS 5

/* UnixFolder object layout and typedef.
 */
typedef struct _UnixFolder {
    const IShellFolder2Vtbl  *lpIShellFolder2Vtbl;
    const IPersistFolderVtbl *lpIPersistFolderVtbl;
    ULONG m_cRef;
    CHAR *m_pszPath;
    LPITEMIDLIST m_pidlLocation;
    LPITEMIDLIST *m_apidlSubDirs;
    DWORD m_cSubDirs;
} UnixFolder;

static void UnixFolder_Destroy(UnixFolder *pUnixFolder) {
    DWORD i;

    TRACE("(pUnixFolder=%p)\n", pUnixFolder);
    
    if (pUnixFolder->m_apidlSubDirs) 
        for (i=0; i < pUnixFolder->m_cSubDirs; i++) 
            SHFree(pUnixFolder->m_apidlSubDirs[i]);
    SHFree(pUnixFolder->m_apidlSubDirs);
    SHFree(pUnixFolder->m_pszPath);
    ILFree(pUnixFolder->m_pidlLocation);
    SHFree(pUnixFolder);
}

static HRESULT WINAPI UnixFolder_IShellFolder2_QueryInterface(IShellFolder2 *iface, REFIID riid, 
    void **ppv) 
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IShellFolder2, iface);
        
    TRACE("(iface=%p, riid=%p, ppv=%p)\n", iface, riid, ppv);
    
    if (!ppv) return E_INVALIDARG;
    
    if (IsEqualIID(&IID_IUnknown, riid) || IsEqualIID(&IID_IShellFolder, riid) || 
        IsEqualIID(&IID_IShellFolder2, riid)) 
    {
        *ppv = &This->lpIShellFolder2Vtbl;
    } else if (IsEqualIID(&IID_IPersistFolder, riid) || IsEqualIID(&IID_IPersist, riid)) {
        *ppv = &This->lpIPersistFolderVtbl;
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI UnixFolder_IShellFolder2_AddRef(IShellFolder2 *iface) {
    UnixFolder *This = ADJUST_THIS(UnixFolder, IShellFolder2, iface);

    TRACE("(iface=%p)\n", iface);

    return InterlockedIncrement(&This->m_cRef);
}

static ULONG WINAPI UnixFolder_IShellFolder2_Release(IShellFolder2 *iface) {
    UnixFolder *This = ADJUST_THIS(UnixFolder, IShellFolder2, iface);
    ULONG cRef;
    
    TRACE("(iface=%p)\n", iface);

    cRef = InterlockedDecrement(&This->m_cRef);
    
    if (!cRef) 
        UnixFolder_Destroy(This);

    return cRef;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_ParseDisplayName(IShellFolder2* iface, HWND hwndOwner, 
    LPBC pbcReserved, LPOLESTR lpszDisplayName, ULONG* pchEaten, LPITEMIDLIST* ppidl, 
    ULONG* pdwAttributes)
{
    int cPathLen;
    char *pszAnsiPath;
    BOOL result;

    TRACE("(iface=%p, hwndOwner=%p, pbcReserved=%p, lpszDisplayName=%s, pchEaten=%p, ppidl=%p, "
          "pdwAttributes=%p) stub\n", iface, hwndOwner, pbcReserved, debugstr_w(lpszDisplayName), 
          pchEaten, ppidl, pdwAttributes);

    cPathLen = lstrlenW(lpszDisplayName);
    pszAnsiPath = (char*)SHAlloc(cPathLen+1);
    WideCharToMultiByte(CP_ACP, 0, lpszDisplayName, -1, pszAnsiPath, cPathLen+1, NULL, NULL);

    result = UNIXFS_path_to_pidl(pszAnsiPath, ppidl);

    SHFree(pszAnsiPath);
    
    return result ? S_OK : E_FAIL;
}

static IUnknown *UnixSubFolderIterator_Construct(UnixFolder *pUnixFolder);

static HRESULT WINAPI UnixFolder_IShellFolder2_EnumObjects(IShellFolder2* iface, HWND hwndOwner, 
    SHCONTF grfFlags, IEnumIDList** ppEnumIDList)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IShellFolder2, iface);
    IUnknown *newIterator;
    HRESULT hr;
    
    TRACE("(iface=%p, hwndOwner=%p, grfFlags=%08lx, ppEnumIDList=%p)\n", 
            iface, hwndOwner, grfFlags, ppEnumIDList);

    newIterator = UnixSubFolderIterator_Construct(This);
    hr = IUnknown_QueryInterface(newIterator, &IID_IEnumIDList, (void**)ppEnumIDList);
    IUnknown_Release(newIterator);
    
    return hr;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_BindToObject(IShellFolder2* iface, LPCITEMIDLIST pidl,
    LPBC pbcReserved, REFIID riid, void** ppvOut)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IShellFolder2, iface);
    IPersistFolder *persistFolder;
    LPITEMIDLIST pidlSubFolder;
    HRESULT hr;
        
    TRACE("(iface=%p, pidl=%p, pbcReserver=%p, riid=%p, ppvOut=%p)\n", 
            iface, pidl, pbcReserved, riid, ppvOut);

    hr = UnixFolder_Constructor(NULL, &IID_IPersistFolder, (void**)&persistFolder);
    if (!SUCCEEDED(hr)) return hr;
    hr = IPersistFolder_QueryInterface(persistFolder, riid, (void**)ppvOut);
    
    pidlSubFolder = ILCombine(This->m_pidlLocation, pidl);
    IPersistFolder_Initialize(persistFolder, pidlSubFolder);
    IPersistFolder_Release(persistFolder);
    ILFree(pidlSubFolder);

    return hr;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_BindToStorage(IShellFolder2* This, LPCITEMIDLIST pidl, 
    LPBC pbcReserved, REFIID riid, void** ppvObj)
{
    TRACE("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_CompareIDs(IShellFolder2* iface, LPARAM lParam, 
    LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    BOOL isEmpty1, isEmpty2;
    HRESULT hr = E_FAIL;
    LPITEMIDLIST firstpidl;
    IShellFolder2 *psf;
    int compare;

    TRACE("(iface=%p, lParam=%ld, pidl1=%p, pidl2=%p)\n", iface, lParam, pidl1, pidl2);
    
    isEmpty1 = !pidl1 || !pidl1->mkid.cb;
    isEmpty2 = !pidl2 || !pidl2->mkid.cb;

    if (isEmpty1 && isEmpty2) 
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
    else if (isEmpty1)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD)-1);
    else if (isEmpty2)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD)1);

    compare = CompareStringA(LOCALE_USER_DEFAULT, NORM_IGNORECASE, pidl1->mkid.abID, pidl1->mkid.cb, 
                             pidl2->mkid.abID, pidl2->mkid.cb);
    if ((compare == CSTR_LESS_THAN) || (compare == CSTR_GREATER_THAN)) 
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD)(compare == CSTR_LESS_THAN)?-1:1);

    if (pidl1->mkid.cb < pidl2->mkid.cb)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD)-1);
    else if (pidl1->mkid.cb > pidl2->mkid.cb)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, (WORD)1);

    firstpidl = ILCloneFirst(pidl1);
    pidl1 = ILGetNext(pidl1);
    pidl2 = ILGetNext(pidl2);
    
    hr = IShellFolder2_BindToObject(iface, firstpidl, NULL, &IID_IShellFolder, (LPVOID*)&psf);
    if (SUCCEEDED(hr)) {
        hr = IShellFolder_CompareIDs(psf, lParam, pidl1, pidl2);
        IShellFolder2_Release(psf);
    }

    ILFree(firstpidl);
    return hr;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_CreateViewObject(IShellFolder2* iface, HWND hwndOwner,
    REFIID riid, void** ppv)
{
    HRESULT hr = E_INVALIDARG;
        
    TRACE("(iface=%p, hwndOwner=%p, riid=%p, ppv=%p) stub\n", iface, hwndOwner, riid, ppv);
    
    if (!ppv) return E_INVALIDARG;
    *ppv = NULL;
    
    if (IsEqualIID(&IID_IShellView, riid)) {
        LPSHELLVIEW pShellView;
        
        pShellView = IShellView_Constructor((IShellFolder*)iface);
        if (pShellView) {
            hr = IShellView_QueryInterface(pShellView, riid, ppv);
            IShellView_Release(pShellView);
        }
    } 
    
    return hr;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_GetAttributesOf(IShellFolder2* iface, UINT cidl, 
    LPCITEMIDLIST* apidl, SFGAOF* rgfInOut)
{
    TRACE("(iface=%p, cidl=%u, apidl=%p, rgfInOut=%p) semi-stub\n", iface, cidl, apidl, rgfInOut);
    
    *rgfInOut = *rgfInOut & (SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_HASSUBFOLDER);
            
    return S_OK;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_GetUIObjectOf(IShellFolder2* iface, HWND hwndOwner, 
    UINT cidl, LPCITEMIDLIST* apidl, REFIID riid, UINT* prgfInOut, void** ppvOut)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IShellFolder2, iface);
    
    TRACE("(iface=%p, hwndOwner=%p, cidl=%d, apidl=%p, riid=%s, prgfInOut=%p, ppv=%p)\n",
        iface, hwndOwner, cidl, apidl, debugstr_guid(riid), prgfInOut, ppvOut);

    if (IsEqualIID(&IID_IContextMenu, riid)) {
        *ppvOut = ISvItemCm_Constructor((IShellFolder*)iface, This->m_pidlLocation, apidl, cidl);
        return S_OK;
    } else if (IsEqualIID(&IID_IDataObject, riid)) {
        *ppvOut = IDataObject_Constructor(hwndOwner, This->m_pidlLocation, apidl, cidl);
        return S_OK;
    } else if (IsEqualIID(&IID_IExtractIconA, riid)) {
        FIXME("IExtractIconA\n");
        return E_FAIL;
    } else if (IsEqualIID(&IID_IExtractIconW, riid)) {
        *ppvOut = &UnixFolderIconSingleton;
        return S_OK;
    } else if (IsEqualIID(&IID_IDropTarget, riid)) {
        FIXME("IDropTarget\n");
        return E_FAIL;
    } else if (IsEqualIID(&IID_IShellLinkW, riid)) {
        FIXME("IShellLinkW\n");
        return E_FAIL;
    } else if (IsEqualIID(&IID_IShellLinkA, riid)) {
        FIXME("IShellLinkA\n");
        return E_FAIL;
    } else {
        FIXME("Unknown interface %s in GetUIObjectOf\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
}

static HRESULT WINAPI UnixFolder_IShellFolder2_GetDisplayNameOf(IShellFolder2* iface, 
    LPCITEMIDLIST pidl, SHGDNF uFlags, STRRET* lpName)
{
    LPCSHITEMID pSHItem = (LPCSHITEMID)pidl;
    char szName[MAX_PATH];

    lpName->uType = STRRET_CSTR;
    if (!pidl->mkid.cb) {
        strcpy(lpName->u.cStr, "");
        return S_OK;
    }

    memcpy(szName, pSHItem->abID, pSHItem->cb-sizeof(USHORT));
    szName[pSHItem->cb-sizeof(USHORT)] = '\0';
    
    TRACE("(iface=%p, pidl=%p, uFlags=%lx, lpName=%p)\n", iface, pidl, uFlags, lpName);
   
    if ((uFlags & SHGDN_FORPARSING) && !(uFlags & SHGDN_INFOLDER)) {
        STRRET strSubfolderName;
        IShellFolder *pSubFolder;
        HRESULT hr;
        LPITEMIDLIST pidlFirst;

        pidlFirst = ILCloneFirst(pidl);
        if (!pidlFirst) {
            WARN("ILCloneFirst failed!\n");
            return E_FAIL;
        }

        hr = IShellFolder_BindToObject(iface, pidlFirst, NULL, &IID_IShellFolder, 
                                       (void**)&pSubFolder);
        if (!SUCCEEDED(hr)) {
            WARN("BindToObject failed!\n");
            ILFree(pidlFirst);
            return hr;
        }

        ILFree(pidlFirst);
        
        hr = IShellFolder_GetDisplayNameOf(pSubFolder, ILGetNext(pidl), uFlags, &strSubfolderName);
        if (!SUCCEEDED(hr)) {
            WARN("GetDisplayNameOf failed!\n");
            return hr;
        }
        
        snprintf(lpName->u.cStr, MAX_PATH, "%s%s", szName, strSubfolderName.u.cStr);

        IShellFolder_Release(pSubFolder);
    } else {
        size_t len;
        strcpy(lpName->u.cStr, szName);
        len = strlen(lpName->u.cStr);
        if (len > 1) lpName->u.cStr[len-1] = '\0';
    }

    return S_OK;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_SetNameOf(IShellFolder2* This, HWND hwnd, 
    LPCITEMIDLIST pidl, LPCOLESTR lpszName, SHGDNF uFlags, LPITEMIDLIST* ppidlOut)
{
    TRACE("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_EnumSearches(IShellFolder2* iface, 
    IEnumExtraSearch **ppEnum) 
{
    TRACE("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_GetDefaultColumn(IShellFolder2* iface, 
    DWORD dwReserved, ULONG *pSort, ULONG *pDisplay) 
{
    TRACE("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_GetDefaultColumnState(IShellFolder2* iface, 
    UINT iColumn, SHCOLSTATEF *pcsFlags)
{
    TRACE("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_GetDefaultSearchGUID(IShellFolder2* iface, 
    GUID *pguid)
{
    TRACE("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_GetDetailsEx(IShellFolder2* iface, 
    LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    TRACE("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_GetDetailsOf(IShellFolder2* iface, 
    LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *psd)
{
    HRESULT hr = E_FAIL;
        
    TRACE("(iface=%p, pidl=%p, iColumn=%d, psd=%p) stub\n", iface, pidl, iColumn, psd);
    
    if (!psd || iColumn >= GENERICSHELLVIEWCOLUMNS)
        return E_INVALIDARG;

    if (!pidl) {
        psd->fmt = GenericSFHeader[iColumn].fmt;
        psd->cxChar = GenericSFHeader[iColumn].cxChar;
        psd->str.uType = STRRET_CSTR;
        LoadStringA(shell32_hInstance, GenericSFHeader[iColumn].colnameid, psd->str.u.cStr, MAX_PATH);
        return S_OK;
    } else {
        switch (iColumn) {
            case 0:
                hr = IShellFolder2_GetDisplayNameOf(iface, pidl, SHGDN_NORMAL|SHGDN_INFOLDER, &psd->str);
                break;
        }
        psd->str.uType = STRRET_CSTR;
    }
    
    return hr;
}

static HRESULT WINAPI UnixFolder_IShellFolder2_MapColumnToSCID(IShellFolder2* iface, UINT iColumn,
    SHCOLUMNID *pscid) 
{
    TRACE("stub\n");
    return E_NOTIMPL;
}

/* VTable for UnixFolder's IShellFolder2 interface.
 */
static const IShellFolder2Vtbl UnixFolder_IShellFolder2_Vtbl = {
    UnixFolder_IShellFolder2_QueryInterface,
    UnixFolder_IShellFolder2_AddRef,
    UnixFolder_IShellFolder2_Release,
    UnixFolder_IShellFolder2_ParseDisplayName,
    UnixFolder_IShellFolder2_EnumObjects,
    UnixFolder_IShellFolder2_BindToObject,
    UnixFolder_IShellFolder2_BindToStorage,
    UnixFolder_IShellFolder2_CompareIDs,
    UnixFolder_IShellFolder2_CreateViewObject,
    UnixFolder_IShellFolder2_GetAttributesOf,
    UnixFolder_IShellFolder2_GetUIObjectOf,
    UnixFolder_IShellFolder2_GetDisplayNameOf,
    UnixFolder_IShellFolder2_SetNameOf,
    UnixFolder_IShellFolder2_GetDefaultSearchGUID,
    UnixFolder_IShellFolder2_EnumSearches,
    UnixFolder_IShellFolder2_GetDefaultColumn,
    UnixFolder_IShellFolder2_GetDefaultColumnState,
    UnixFolder_IShellFolder2_GetDetailsEx,
    UnixFolder_IShellFolder2_GetDetailsOf,
    UnixFolder_IShellFolder2_MapColumnToSCID
};

static HRESULT WINAPI UnixFolder_IPersistFolder_QueryInterface(IPersistFolder* This, REFIID riid, 
    void** ppvObject)
{
    return UnixFolder_IShellFolder2_QueryInterface(
                (IShellFolder2*)ADJUST_THIS(UnixFolder, IPersistFolder, This), riid, ppvObject);
}

static ULONG WINAPI UnixFolder_IPersistFolder_AddRef(IPersistFolder* This)
{
    return UnixFolder_IShellFolder2_AddRef(
                (IShellFolder2*)ADJUST_THIS(UnixFolder, IPersistFolder, This));
}

static ULONG WINAPI UnixFolder_IPersistFolder_Release(IPersistFolder* This)
{
    return UnixFolder_IShellFolder2_Release(
                (IShellFolder2*)ADJUST_THIS(UnixFolder, IPersistFolder, This));
}

static HRESULT WINAPI UnixFolder_IPersistFolder_GetClassID(IPersistFolder* This, CLSID* pClassID)
{
    TRACE("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI UnixFolder_IPersistFolder_Initialize(IPersistFolder* iface, LPCITEMIDLIST pidl)
{
    UnixFolder *This = ADJUST_THIS(UnixFolder, IPersistFolder, iface);
    
    TRACE("(iface=%p, pidl=%p)\n", iface, pidl);

    This->m_pidlLocation = ILClone(pidl);
    UNIXFS_pidl_to_path(pidl, &This->m_pszPath);
    UNIXFS_build_subfolder_pidls(This->m_pszPath, &This->m_apidlSubDirs, &This->m_cSubDirs);
    
    return S_OK;
}

/* VTable for UnixFolder's IPersistFolder interface.
 */
static const IPersistFolderVtbl UnixFolder_IPersistFolder_Vtbl = {
    UnixFolder_IPersistFolder_QueryInterface,
    UnixFolder_IPersistFolder_AddRef,
    UnixFolder_IPersistFolder_Release,
    UnixFolder_IPersistFolder_GetClassID,
    UnixFolder_IPersistFolder_Initialize
};

/******************************************************************************
 * UnixFolder_Constructor [Internal]
 *
 * PARAMS
 *  pUnkOuter [I] Outer class for aggregation. Currently ignored.
 *  riid      [I] Interface asked for by the client.
 *  ppv       [O] Pointer to an riid interface to the UnixFolder object.
 *
 * NOTES
 *  This is the only function exported from shfldr_unixfs.c. It's called from
 *  shellole.c's default class factory and thus has to exhibit a LPFNCREATEINSTANCE
 *  compatible signature.
 */
HRESULT WINAPI UnixFolder_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv) {
    HRESULT hr;
    UnixFolder *pUnixFolder;
    
    TRACE("(pUnkOuter=%p, riid=%p, ppv=%p)\n", pUnkOuter, riid, ppv);

    pUnixFolder = SHAlloc((ULONG)sizeof(UnixFolder));
    pUnixFolder->lpIShellFolder2Vtbl = &UnixFolder_IShellFolder2_Vtbl;
    pUnixFolder->lpIPersistFolderVtbl = &UnixFolder_IPersistFolder_Vtbl;
    pUnixFolder->m_cRef = 0;
    pUnixFolder->m_pszPath = NULL;
    pUnixFolder->m_apidlSubDirs = NULL;
    pUnixFolder->m_cSubDirs = 0;

    UnixFolder_IShellFolder2_AddRef(STATIC_CAST(IShellFolder2, pUnixFolder));
    hr = UnixFolder_IShellFolder2_QueryInterface(STATIC_CAST(IShellFolder2, pUnixFolder), riid, ppv);
    UnixFolder_IShellFolder2_Release(STATIC_CAST(IShellFolder2, pUnixFolder));
    return hr;
}

/******************************************************************************
 * UnixSubFolderIterator
 *
 * Class whose heap based objects represent iterators over the sub-directories
 * of a given UnixFolder object. 
 */

/* UnixSubFolderIterator object layout and typedef.
 */
typedef struct _UnixSubFolderIterator {
    const IEnumIDListVtbl *lpIEnumIDListVtbl;
    ULONG m_cRef;
    UnixFolder *m_pUnixFolder;
    ULONG m_cIdx;
} UnixSubFolderIterator;

static void UnixSubFolderIterator_Destroy(UnixSubFolderIterator *iterator) {
    TRACE("(iterator=%p)\n", iterator);
        
    UnixFolder_IShellFolder2_Release((IShellFolder2*)iterator->m_pUnixFolder);
    SHFree(iterator);
}

static HRESULT WINAPI UnixSubFolderIterator_IEnumIDList_QueryInterface(IEnumIDList* iface, 
    REFIID riid, void** ppv)
{
    TRACE("(iface=%p, riid=%p, ppv=%p)\n", iface, riid, ppv);
    
    if (!ppv) return E_INVALIDARG;
    
    if (IsEqualIID(&IID_IUnknown, riid) || IsEqualIID(&IID_IEnumIDList, riid)) {
        *ppv = iface;
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IEnumIDList_AddRef(iface);
    return S_OK;
}
                            
static ULONG WINAPI UnixSubFolderIterator_IEnumIDList_AddRef(IEnumIDList* iface)
{
    UnixSubFolderIterator *This = ADJUST_THIS(UnixSubFolderIterator, IEnumIDList, iface);

    TRACE("(iface=%p)\n", iface);
   
    return InterlockedIncrement(&This->m_cRef);
}

static ULONG WINAPI UnixSubFolderIterator_IEnumIDList_Release(IEnumIDList* iface)
{
    UnixSubFolderIterator *This = ADJUST_THIS(UnixSubFolderIterator, IEnumIDList, iface);
    ULONG cRef;
    
    TRACE("(iface=%p)\n", iface);

    cRef = InterlockedDecrement(&This->m_cRef);
    
    if (!cRef) 
        UnixSubFolderIterator_Destroy(This);

    return cRef;
}

static HRESULT WINAPI UnixSubFolderIterator_IEnumIDList_Next(IEnumIDList* iface, ULONG celt, 
    LPITEMIDLIST* rgelt, ULONG* pceltFetched)
{
    UnixSubFolderIterator *This = ADJUST_THIS(UnixSubFolderIterator, IEnumIDList, iface);
    ULONG i;

    TRACE("(iface=%p, celt=%ld, rgelt=%p, pceltFetched=%p)\n", iface, celt, rgelt, pceltFetched);
   
    for (i=0; (i < celt) && (This->m_cIdx < This->m_pUnixFolder->m_cSubDirs); i++, This->m_cIdx++) {
        rgelt[i] = ILClone(This->m_pUnixFolder->m_apidlSubDirs[This->m_cIdx]);
    }

    if (pceltFetched)
        *pceltFetched = i;

    return i == celt ? S_OK : S_FALSE;
}

static HRESULT WINAPI UnixSubFolderIterator_IEnumIDList_Skip(IEnumIDList* iface, ULONG celt)
{
    UnixSubFolderIterator *This = ADJUST_THIS(UnixSubFolderIterator, IEnumIDList, iface);
    
    TRACE("(iface=%p, celt=%ld)\n", iface, celt);

    if (This->m_cIdx + celt > This->m_pUnixFolder->m_cSubDirs) {
        This->m_cIdx = This->m_pUnixFolder->m_cSubDirs;
        return S_FALSE;
    } else {
        This->m_cIdx += celt;
        return S_OK;
    }
}

static HRESULT WINAPI UnixSubFolderIterator_IEnumIDList_Reset(IEnumIDList* iface)
{
    UnixSubFolderIterator *This = ADJUST_THIS(UnixSubFolderIterator, IEnumIDList, iface);
        
    TRACE("(iface=%p)\n", iface);

    This->m_cIdx = 0;
    
    return S_OK;
}

static HRESULT WINAPI UnixSubFolderIterator_IEnumIDList_Clone(IEnumIDList* This, 
    IEnumIDList** ppenum)
{
    TRACE("stub\n");
    return E_NOTIMPL;
}

/* VTable for UnixSubFolderIterator's IEnumIDList interface.
 */
static const IEnumIDListVtbl UnixSubFolderIterator_IEnumIDList_Vtbl = {
    UnixSubFolderIterator_IEnumIDList_QueryInterface,
    UnixSubFolderIterator_IEnumIDList_AddRef,
    UnixSubFolderIterator_IEnumIDList_Release,
    UnixSubFolderIterator_IEnumIDList_Next,
    UnixSubFolderIterator_IEnumIDList_Skip,
    UnixSubFolderIterator_IEnumIDList_Reset,
    UnixSubFolderIterator_IEnumIDList_Clone
};

static IUnknown *UnixSubFolderIterator_Construct(UnixFolder *pUnixFolder) {
    UnixSubFolderIterator *iterator;

    TRACE("(pUnixFolder=%p)\n", pUnixFolder);
    
    iterator = SHAlloc((ULONG)sizeof(UnixSubFolderIterator));
    iterator->lpIEnumIDListVtbl = &UnixSubFolderIterator_IEnumIDList_Vtbl;
    iterator->m_cRef = 0;
    iterator->m_cIdx = 0;
    iterator->m_pUnixFolder = pUnixFolder;

    UnixSubFolderIterator_IEnumIDList_AddRef((IEnumIDList*)iterator);
    UnixFolder_IShellFolder2_AddRef((IShellFolder2*)pUnixFolder);
    
    return (IUnknown*)iterator;
}

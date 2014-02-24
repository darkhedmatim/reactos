/*
* Shell Menu Band
*
* Copyright 2014 David Quintana
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
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
*/
#pragma once

class CMenuBand;
class CMenuFocusManager;

class CMenuToolbarBase
{
private:
    HWND        m_hwnd;        // May be the pager

protected:
    CMenuBand * m_menuBand;
    HWND        m_hwndToolbar;
    DWORD       m_dwMenuFlags;
    INT         m_hotItem;
    WNDPROC     m_SubclassOld;
    BOOL        m_hasIdealSize;
    SIZE        m_idealSize;
    BOOL        m_usePager;

private:
    static LRESULT CALLBACK s_SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
    CMenuToolbarBase(CMenuBand *menuBand, BOOL usePager);
    virtual ~CMenuToolbarBase() {}

    HRESULT IsWindowOwner(HWND hwnd);
    HRESULT CreateToolbar(HWND hwndParent, DWORD dwFlags);
    HRESULT GetWindow(HWND *phwnd);
    HRESULT ShowWindow(BOOL fShow);
    HRESULT Close();

    virtual HRESULT FillToolbar() = 0;
    virtual HRESULT PopupItem(UINT uItem) = 0;
    virtual HRESULT HasSubMenu(UINT uItem) = 0;
    virtual HRESULT OnContextMenu(NMMOUSE * rclick) = 0;
    virtual HRESULT OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult);

    HRESULT PopupSubMenu(UINT itemId, UINT index, IShellMenu* childShellMenu);
    HRESULT PopupSubMenu(UINT index, HMENU menu);
    HRESULT DoContextMenu(IContextMenu* contextMenu);

    HRESULT ChangeHotItem(DWORD changeType);
    HRESULT OnHotItemChange(const NMTBHOTITEM * hot);

    HRESULT GetIdealSize(SIZE& size);
    HRESULT SetPosSize(int x, int y, int cx, int cy);

    void InvalidateDraw();

protected:
    LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

class CMenuStaticToolbar :
    public CMenuToolbarBase
{
private:
    HMENU m_hmenu;

public:
    CMenuStaticToolbar(CMenuBand *menuBand);
    virtual ~CMenuStaticToolbar() {}

    HRESULT SetMenu(HMENU hmenu, HWND hwnd, DWORD dwFlags);
    HRESULT GetMenu(HMENU *phmenu, HWND *phwnd, DWORD *pdwFlags);

    virtual HRESULT FillToolbar();
    virtual HRESULT PopupItem(UINT uItem);
    virtual HRESULT HasSubMenu(UINT uItem);
    virtual HRESULT OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult);
    virtual HRESULT OnContextMenu(NMMOUSE * rclick);

};

class CMenuSFToolbar :
    public CMenuToolbarBase
{
private:
    IShellFolder * m_shellFolder;
    LPCITEMIDLIST  m_idList;
    HKEY           m_hKey;

public:
    CMenuSFToolbar(CMenuBand *menuBand);
    virtual ~CMenuSFToolbar();

    HRESULT SetShellFolder(IShellFolder *psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags);
    HRESULT GetShellFolder(DWORD *pdwFlags, LPITEMIDLIST *ppidl, REFIID riid, void **ppv);

    virtual HRESULT FillToolbar();
    virtual HRESULT PopupItem(UINT uItem);
    virtual HRESULT HasSubMenu(UINT uItem);
    virtual HRESULT OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult);
    virtual HRESULT OnContextMenu(NMMOUSE * rclick);

private:
    LPITEMIDLIST GetPidlFromId(UINT uItem, INT* pIndex = NULL);
};

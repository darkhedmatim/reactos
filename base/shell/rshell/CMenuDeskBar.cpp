/*
 * Shell Menu Desk Bar
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
#include "precomp.h"
#include <atlwin.h>
#include <shlwapi_undoc.h>

#include "CMenuDeskBar.h"

WINE_DEFAULT_DEBUG_CHANNEL(CMenuDeskBar);

const static GUID CGID_MenuDeskBar = { 0x5C9F0A12, 0x959E, 0x11D0, { 0xA3, 0xA4, 0x00, 0xA0, 0xC9, 0x08, 0x26, 0x36 } };

extern "C"
HRESULT WINAPI CMenuDeskBar_Constructor(REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

    CMenuDeskBar * deskbar = new CComObject<CMenuDeskBar>();

    if (!deskbar)
        return E_OUTOFMEMORY;

    HRESULT hr = deskbar->QueryInterface(riid, ppv);

    if (FAILED_UNEXPECTEDLY(hr))
        deskbar->Release();

    return hr;
}

CMenuDeskBar::CMenuDeskBar() :
    m_Client(NULL),
    m_Banner(NULL),
    m_Shown(FALSE)
{
}

CMenuDeskBar::~CMenuDeskBar()
{
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::Initialize(THIS)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetWindow(HWND *lphwnd)
{
    if (lphwnd == NULL)
        return E_POINTER;
    *lphwnd = m_hWnd;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::OnFocusChangeIS(IUnknown *punkObj, BOOL fSetFocus)
{
    CComPtr<IInputObjectSite> ios;

    HRESULT hr = m_Client->QueryInterface(IID_PPV_ARG(IInputObjectSite, &ios));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return ios->OnFocusChangeIS(punkObj, fSetFocus);
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds,
    OLECMD prgCmds [], OLECMDTEXT *pCmdText)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
    DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    if (IsEqualIID(*pguidCmdGroup, CGID_MenuDeskBar))
    {
        switch (nCmdID)
        {
        case 2: // refresh
            return S_OK;
        case 3: // load complete
            return S_OK;
        case 4: // set font metrics
            return S_OK;
        }
    }
    if (IsEqualIID(*pguidCmdGroup, CGID_Explorer))
    {
    }
    else if (IsEqualIID(*pguidCmdGroup, IID_IDeskBarClient))
    {
        switch (nCmdID)
        {
        case 0:
            // hide current band
            break;
        case 2:
            break;
        case 3:
            break;
        }
    }
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    if (IsEqualGUID(guidService, SID_SMenuPopup) ||
        IsEqualGUID(guidService, SID_SMenuBandParent) ||
        IsEqualGUID(guidService, SID_STopLevelBrowser))
    {
        return this->QueryInterface(riid, ppvObject);
    }

    if (IsEqualGUID(guidService, SID_SMenuBandBottom) ||
        IsEqualGUID(guidService, SID_SMenuBandBottomSelected) ||
        IsEqualGUID(guidService, SID_SMenuBandChild))
    {
        if (m_Client == NULL)
            return E_NOINTERFACE;

        return IUnknown_QueryService(m_Client, guidService, riid, ppvObject);
    }


    if (m_Site == NULL)
        return E_NOINTERFACE;

    return IUnknown_QueryService(m_Site, guidService, riid, ppvObject);
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    return IUnknown_UIActivateIO(m_Client, fActivate, lpMsg);
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::HasFocusIO()
{
    CComPtr<IInputObject> io;

    HRESULT hr = m_Client->QueryInterface(IID_PPV_ARG(IInputObject, &io));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return io->HasFocusIO();
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::TranslateAcceleratorIO(LPMSG lpMsg)
{
    CComPtr<IInputObject> io;

    HRESULT hr = m_Client->QueryInterface(IID_PPV_ARG(IInputObject, &io));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return io->TranslateAcceleratorIO(lpMsg);
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetClient(IUnknown *punkClient)
{
    CComPtr<IDeskBarClient> pDeskBandClient;
    HRESULT hr;

    m_Client.Release();

    if (punkClient == NULL)
        return S_OK;

    if (m_hWnd == NULL)
    {
        Create(NULL);
    }

    hr = punkClient->QueryInterface(IID_PPV_ARG(IUnknown, &m_Client));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = m_Client->QueryInterface(IID_PPV_ARG(IDeskBarClient, &pDeskBandClient));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pDeskBandClient->SetDeskBarSite(static_cast<IDeskBar*>(this));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return IUnknown_GetWindow(m_Client, &m_ClientWindow);
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetClient(IUnknown **ppunkClient)
{
    if (ppunkClient == NULL)
        return E_POINTER;

    if (!m_Client)
        return E_FAIL;

    return m_Client->QueryInterface(IID_PPV_ARG(IUnknown, ppunkClient));
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::OnPosRectChangeDB(LPRECT prc)
{
    if (prc == NULL)
        return E_POINTER;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetSite(IUnknown *pUnkSite)
{
    // Windows closes the bar if this is called when the bar is shown

    if (m_Shown)
        _CloseBar();

    m_Site = pUnkSite;

    IUnknown_QueryService(m_Site, SID_SMenuPopup, IID_PPV_ARG(IMenuPopup, &m_SubMenuParent));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetSite(REFIID riid, void **ppvSite)
{
    if (m_Site == NULL)
        return E_FAIL;

    return m_Site->QueryInterface(riid, ppvSite);
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags)
{
    HRESULT hr;
    CComPtr<IOleCommandTarget> oct;
    CComPtr<IInputObject> io;
    CComPtr<IDeskBand> band;
    CComPtr<IDeskBarClient> dbc;

    if (m_hWnd == NULL)
        return E_FAIL;

    hr = IUnknown_QueryService(m_Client, SID_SMenuBandChild, IID_PPV_ARG(IOleCommandTarget, &oct));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = m_Client->QueryInterface(IID_PPV_ARG(IDeskBarClient, &dbc));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    // Windows calls this, but it appears to be unimplemented?
    hr = dbc->SetModeDBC(1);
    // Allow it to fail with E_NOTIMPL.

    // No clue about the arg, using anything != 0
    hr = dbc->UIActivateDBC(TRUE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    RECT rc = { 0 };
    hr = dbc->GetSize(0, &rc);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    // Unknown meaning
    const int CMD = 19;
    const int CMD_EXEC_OPT = 0;

    hr = IUnknown_QueryServiceExec(m_Client, SID_SMenuBandChild, &CLSID_MenuBand, CMD, CMD_EXEC_OPT, NULL, NULL);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    ::AdjustWindowRect(&rc, ::GetWindowLong(m_hWnd, GWL_STYLE), FALSE);
    ::OffsetRect(&rc, -rc.left, -rc.top);

    if (m_Banner != NULL)
    {
        BITMAP bm;
        ::GetObject(m_Banner, sizeof(bm), &bm);
        rc.right += bm.bmWidth;
    }

    RECT rcWorkArea;
    GetWindowRect(GetDesktopWindow(), &rcWorkArea);
    int waHeight = rcWorkArea.bottom - rcWorkArea.top;

    int x = ppt->x;
    int y = ppt->y;
    int cx = rc.right - rc.left;
    int cy = rc.bottom - rc.top;

    switch (dwFlags & 0xFF000000)
    {
    case MPPF_BOTTOM:
        x = ppt->x;
        y = ppt->y - rc.bottom;
        break;
    case MPPF_RIGHT:
        x = ppt->x + rc.left;
        y = ppt->y + rc.top;
        break;
    case MPPF_TOP | MPPF_ALIGN_LEFT:
        x = ppt->x - rc.right;
        y = ppt->y + rc.top;
        break;
    case MPPF_TOP | MPPF_ALIGN_RIGHT:
        x = ppt->x;
        y = ppt->y + rc.top;
        break;
    }

    if (x + cx > rcWorkArea.right)
    {
        // FIXME: Works, but it's oversimplified.
        x = prcExclude->left - cx;
        dwFlags = (dwFlags & (~MPPF_TOP)) | MPPF_LEFT;
    }

    if (y < rcWorkArea.top)
    {
        y = rcWorkArea.top;
    }

    if (cy > waHeight)
    {
        cy = waHeight;
    }

    if (y + cy > rcWorkArea.bottom)
    {
        y = rcWorkArea.bottom - cy;
    }

    this->SetWindowPos(HWND_TOPMOST, x, y, cx, cy, SWP_SHOWWINDOW | SWP_NOACTIVATE);

    m_ShowFlags = dwFlags;
    m_Shown = true;

    // HACK: The bar needs to be notified of the size AFTER it is shown.
    // Quick & dirty way of getting it done.
    BOOL bHandled;
    _OnSize(WM_SIZE, 0, 0, bHandled);

    UIActivateIO(TRUE, NULL);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetIconSize(THIS_ DWORD iIcon)
{
    HRESULT hr;
    m_IconSize = iIcon;

    // Unknown meaning (set flags? set icon size?)
    const int CMD = 16;
    const int CMD_EXEC_OPT = iIcon ? 0 : 2; // seems to work

    hr = IUnknown_QueryServiceExec(m_Client, SID_SMenuBandChild, &CLSID_MenuBand, CMD, CMD_EXEC_OPT, NULL, NULL);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    BOOL bHandled;
    _OnSize(WM_SIZE, 0, 0, bHandled);

    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetIconSize(THIS_ DWORD* piIcon)
{
    if (piIcon)
        *piIcon = m_IconSize;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetBitmap(THIS_ HBITMAP hBitmap)
{
    m_Banner = hBitmap;

    BOOL bHandled;
    _OnSize(WM_SIZE, 0, 0, bHandled);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetBitmap(THIS_ HBITMAP* phBitmap)
{
    if (phBitmap)
        *phBitmap = m_Banner;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetSubMenu(IMenuPopup *pmp, BOOL fSet)
{
    // Called by the MenuBand to assign itself as the logical child of the DeskBar

    if (fSet)
    {
        m_SubMenuChild = pmp;
    }
    else
    {
        if (m_SubMenuChild)
        {
            if (pmp == m_SubMenuChild)
            {
                m_SubMenuChild = NULL;
            }
        }
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::OnSelect(DWORD dwSelectType)
{
    /* As far as I can tell, the submenu hierarchy looks like this:
     *
     * The DeskBar's Child is the Band it contains.
     * The DeskBar's Parent is the SID_SMenuPopup of the Site.
     *
     * The Band's Child is the IMenuPopup of the child submenu.
     * The Band's Parent is the SID_SMenuPopup of the Site (the DeskBar).
     *
     * When the DeskBar receives a selection event:
     * If it requires closing the window, it will notify the Child (Band) using CancelLevel.
     * If it has to spread upwards (everything but CancelLevel), it will notify the Parent.
     *
     * When the Band receives a selection event, this is where it gets fuzzy:
     * In which cases does it call the Parent? Probably not CancelLevel.
     * In which cases does it call the Child?
     * How does it react to calls?
     *
     */

    switch (dwSelectType)
    {
    case MPOS_EXECUTE:
    case MPOS_FULLCANCEL:
    case MPOS_CANCELLEVEL:

        _CloseBar();

        if (dwSelectType == MPOS_CANCELLEVEL)
            return S_OK;

    case MPOS_SELECTLEFT:
    case MPOS_SELECTRIGHT:
    case MPOS_CHILDTRACKING:
        if (m_SubMenuParent)
            return m_SubMenuParent->OnSelect(dwSelectType);
        break;
    }

    return S_OK;
}

HRESULT CMenuDeskBar::_CloseBar()
{
    CComPtr<IDeskBarClient> dbc;
    HRESULT hr;

    m_Shown = false;

    if (m_SubMenuChild)
    {
        hr = m_SubMenuChild->OnSelect(MPOS_CANCELLEVEL);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    hr = m_Client->QueryInterface(IID_PPV_ARG(IDeskBarClient, &dbc));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = dbc->UIActivateDBC(FALSE);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    SetWindowPos(NULL, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOMOVE);

    return UIActivateIO(FALSE, NULL);
}

BOOL CMenuDeskBar::_IsSubMenuParent(HWND hwnd)
{
    CComPtr<IMenuPopup> popup = m_SubMenuParent;

    while (popup)
    {
        HRESULT hr;
        CComPtr<IOleWindow> window;

        hr = popup->QueryInterface(IID_PPV_ARG(IOleWindow, &window));
        if (FAILED_UNEXPECTEDLY(hr))
            return FALSE;

        HWND parent;

        hr = window->GetWindow(&parent);
        if (SUCCEEDED(hr) && hwnd == parent)
            return TRUE;

        popup = NULL;
        hr = IUnknown_GetSite(window, IID_PPV_ARG(IMenuPopup, &popup));
        if (FAILED(hr))
            return FALSE;
    }

    return FALSE;
}

LRESULT CMenuDeskBar::_OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (m_Client)
    {
        RECT rc;

        GetClientRect(&rc);

        if (m_Banner != NULL)
        {
            BITMAP bm;
            ::GetObject(m_Banner, sizeof(bm), &bm);
            rc.left += bm.bmWidth;
        }

        ::SetWindowPos(m_ClientWindow, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 0);
    }

    return 0;
}

LRESULT CMenuDeskBar::_OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (!m_Client)
        return 0;

    CComPtr<IWinEventHandler> winEventHandler;
    HRESULT hr = m_Client->QueryInterface(IID_PPV_ARG(IWinEventHandler, &winEventHandler));
    if (FAILED_UNEXPECTEDLY(hr))
        return 0;

    if (winEventHandler)
    {
        LRESULT result;
        hr = winEventHandler->OnWinEvent(NULL, uMsg, wParam, lParam, &result);
        if (FAILED_UNEXPECTEDLY(hr))
            return 0;
        return result;
    }

    return 0;
}

LRESULT CMenuDeskBar::_OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    bHandled = FALSE;

    if (m_Banner && !m_IconSize)
    {
        BITMAP bm;
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(&ps);

        HDC hdcMem = ::CreateCompatibleDC(hdc);
        HGDIOBJ hbmOld = ::SelectObject(hdcMem, m_Banner);

        ::GetObject(m_Banner, sizeof(bm), &bm);

        RECT rc;
        if (!GetClientRect(&rc))
            WARN("GetClientRect failed\n");

        const int bx = bm.bmWidth;
        const int by = bm.bmHeight;
        const int cy = rc.bottom;

        TRACE("Painting banner: %d by %d\n", bm.bmWidth, bm.bmHeight);

        if (!::StretchBlt(hdc, 0, 0, bx, cy - by, hdcMem, 0, 0, bx, 1, SRCCOPY))
            WARN("StretchBlt failed\n");

        if (!::BitBlt(hdc, 0, cy - by, bx, by, hdcMem, 0, 0, SRCCOPY))
            WARN("BitBlt failed\n");

        ::SelectObject(hdcMem, hbmOld);
        ::DeleteDC(hdcMem);

        EndPaint(&ps);
    }

    return TRUE;
}

LRESULT CMenuDeskBar::_OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    // BUG in ReactOS: WM_ACTIVATE/WA_INACTIVE makes no sense with lParam==hWnd
    if (LOWORD(wParam) != 0 || reinterpret_cast<HWND>(lParam) == m_hWnd)
    {
        return 0;
    }

    // HACK! I just want it to work !!!
    CComPtr<IDeskBar> db;
    HRESULT hr = IUnknown_QueryService(m_Client, SID_SMenuBandChild, IID_PPV_ARG(IDeskBar, &db));
    if (FAILED_UNEXPECTEDLY(hr))
        return 0;

    CComPtr<IUnknown> punk;

    hr = db->GetClient(&punk);
    if (FAILED_UNEXPECTEDLY(hr))
        return 0;

    if (!punk && m_Shown)
    {
        if (!_IsSubMenuParent(reinterpret_cast<HWND>(lParam)))
        {
            OnSelect(MPOS_FULLCANCEL);
        }
    }

    return 0;
}

LRESULT CMenuDeskBar::_OnMouseActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    return MA_NOACTIVATE;
}

LRESULT CMenuDeskBar::_OnAppActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (wParam == 0 && m_Shown)
    {
        OnSelect(MPOS_FULLCANCEL);
    }
    return 0;
}

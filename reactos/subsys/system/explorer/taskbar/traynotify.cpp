/*
 * Copyright 2003 Martin Fuchs
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


 //
 // Explorer clone
 //
 // traynotify.cpp
 //
 // Martin Fuchs, 22.08.2003
 //


#include "../utility/utility.h"

#include "../explorer.h"
#include "../globals.h"

#include "traynotify.h"


#include "../notifyhook/notifyhook.h"

NotifyHook::NotifyHook()
 :	WM_GETMODULEPATH(InstallNotifyHook())
{
}

NotifyHook::~NotifyHook()
{
	DeinstallNotifyHook();
}

void NotifyHook::GetModulePath(HWND hwnd, HWND hwndCallback)
{
	PostMessage(hwnd, WM_GETMODULEPATH, (WPARAM)hwndCallback, 0);
}

bool NotifyHook::ModulePathCopyData(LPARAM lparam, HWND* phwnd, String& path)
{
	char buffer[MAX_PATH];

	int l = GetWindowModulePathCopyData(lparam, phwnd, buffer, MAX_PATH);

	if (l) {
		path.assign(buffer, l);
		return true;
	} else
		return false;
}


NotifyIconIndex::NotifyIconIndex(NOTIFYICONDATA* pnid)
{
	_hWnd = pnid->hWnd;
	_uID = pnid->uID;

	 // special handling for windows task manager
	if ((int)_uID < 0)
		_uID = 0;
}

NotifyIconIndex::NotifyIconIndex()
{
	_hWnd = 0;
	_uID = 0;
}


NotifyInfo::NotifyInfo()
{
	_idx = -1;
	_hIcon = 0;
	_dwState = 0;
	_uCallbackMessage = 0;
	_version = 0;
}


 // WCHAR versions von NOTIFYICONDATA
#define	NID_SIZE_W6	 sizeof(NOTIFYICONDATAW)										// _WIN32_IE = 0x600
#define	NID_SIZE_W5	(sizeof(NOTIFYICONDATAW)-sizeof(GUID))							// _WIN32_IE = 0x500
#define	NID_SIZE_W3	(sizeof(NOTIFYICONDATAW)-sizeof(GUID)-(128-64)*sizeof(WCHAR))	// _WIN32_IE < 0x500

 // CHAR versions von NOTIFYICONDATA
#define	NID_SIZE_A6	 sizeof(NOTIFYICONDATAA)
#define	NID_SIZE_A5	(sizeof(NOTIFYICONDATAA)-sizeof(GUID))
#define	NID_SIZE_A3	(sizeof(NOTIFYICONDATAA)-sizeof(GUID)-(128-64)*sizeof(CHAR))

NotifyInfo& NotifyInfo::operator=(NOTIFYICONDATA* pnid)
{
	_hWnd = pnid->hWnd;
	_uID = pnid->uID;

	if (pnid->uFlags & NIF_MESSAGE)
		_uCallbackMessage = pnid->uCallbackMessage;

	if (pnid->uFlags & NIF_ICON) {
		 // Some applications destroy the icon immediatelly after completing the
		 // NIM_ADD/MODIFY message, so we have to make a copy of it.
		if (_hIcon)
			DestroyIcon(_hIcon);

		_hIcon = (HICON) CopyImage(pnid->hIcon, IMAGE_ICON, 16, 16, 0);
	}

#ifdef NIF_STATE	// as of 21.08.2003 missing in MinGW headers
	if (pnid->uFlags & NIF_STATE)
		_dwState = (_dwState&~pnid->dwStateMask) | (pnid->dwState&pnid->dwStateMask);
#endif

	 // store tool tip text
	if (pnid->uFlags & NIF_TIP)
		if (pnid->cbSize==NID_SIZE_W6 || pnid->cbSize==NID_SIZE_W5 || pnid->cbSize==NID_SIZE_W3) {
			 // UNICODE version of NOTIFYICONDATA structure
			LPCWSTR txt = (LPCWSTR)pnid->szTip;
			int max_len = pnid->cbSize==NID_SIZE_W3? 64: 128;

			 // get tooltip string length
			int l = 0;
			for(; l<max_len; ++l)
				if (!txt[l])
					break;

			_tipText.assign(txt, l);
		} else if (pnid->cbSize==NID_SIZE_A6 || pnid->cbSize==NID_SIZE_A5 || pnid->cbSize==NID_SIZE_A3) {
			LPCSTR txt = (LPCSTR)pnid->szTip;
			int max_len = pnid->cbSize==NID_SIZE_A3? 64: 128;

			int l = 0;
			for(int l=0; l<max_len; ++l)
				if (!txt[l])
					break;

			_tipText.assign(txt, l);
		}

	return *this;
}


NotifyArea::NotifyArea(HWND hwnd)
 :	super(hwnd),
	_tooltip(hwnd)
{
	_next_idx = 0;
	_clock_width = 0;
	_last_icon_count = 0;
	_show_hidden = false;
}

LRESULT NotifyArea::Init(LPCREATESTRUCT pcs)
{
	if (super::Init(pcs))
		return 1;

#ifndef __MINGW32__	// SHRestricted() missing in MinGW (as of 29.10.2003)
	if (!g_Globals._SHRestricted || !SHRestricted(REST_HIDECLOCK))
#endif
	{
		HKEY hkeyStuckRects = 0;
		DWORD buffer[10];
		DWORD len = sizeof(buffer);

		bool hide_clock = false;

		 // check if the clock should be hidden
		if (!RegOpenKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StuckRects2"), &hkeyStuckRects) &&
			!RegQueryValueEx(hkeyStuckRects, TEXT("Settings"), 0, NULL, (LPBYTE)buffer, &len) &&
			len==sizeof(buffer) && buffer[0]==sizeof(buffer))
			hide_clock = buffer[2] & 0x08? true: false;

		if (!hide_clock) {
			 // create clock window
			_hwndClock = ClockWindow::Create(_hwnd);

			if (_hwndClock) {
				ClientRect clock_size(_hwndClock);
				_clock_width = clock_size.right;
			}
		}

		if (hkeyStuckRects)
			RegCloseKey(hkeyStuckRects);
	}

	SetTimer(_hwnd, 0, 1000, NULL);

	return 0;
}

NotifyArea::~NotifyArea()
{
	KillTimer(_hwnd, 0);
}

HWND NotifyArea::Create(HWND hwndParent)
{
	static BtnWindowClass wcTrayNotify(CLASSNAME_TRAYNOTIFY, CS_DBLCLKS);

	ClientRect clnt(hwndParent);

#ifndef _ROS_
	return Window::Create(WINDOW_CREATOR(NotifyArea), WS_EX_STATICEDGE,
							wcTrayNotify, TITLE_TRAYNOTIFY, WS_CHILD|WS_VISIBLE,
							clnt.right-(NOTIFYAREA_WIDTH_DEF+1), 1, NOTIFYAREA_WIDTH_DEF, clnt.bottom-2, hwndParent);
#else
	return Window::Create(WINDOW_CREATOR(NotifyArea), 0,
							wcTrayNotify, TITLE_TRAYNOTIFY, WS_CHILD|WS_VISIBLE,
							clnt.right-(NOTIFYAREA_WIDTH_DEF+1), 1, NOTIFYAREA_WIDTH_DEF, clnt.bottom-2, hwndParent);
#endif
}

LRESULT NotifyArea::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_PAINT:
		Paint();
		break;

	  case WM_TIMER: {
		TimerTick();

		ClockWindow* clock_window = GET_WINDOW(ClockWindow, _hwndClock);

		if (clock_window)
			clock_window->TimerTick();
		break;}

	  case WM_SIZE: {
		int cx = LOWORD(lparam);
		SetWindowPos(_hwndClock, 0, cx-_clock_width, 0, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
		break;}

	  case PM_GET_WIDTH:
		return _sorted_icons.size()*NOTIFYICON_DIST + NOTIFYAREA_SPACE + _clock_width;

	  case WM_CONTEXTMENU:
		break;	// don't let WM_CONTEXTMENU go through to the desktop bar

	  case WM_COPYDATA: {	// receive NotifyHook answers
		String path;
		HWND hwnd;

		if (_hook.ModulePathCopyData(lparam, &hwnd, path)) {
			_window_modules[hwnd] = path;
			//@@ -> trigger DetermineHideState()
		}

		break;}

	  default:
		if (nmsg>=WM_MOUSEFIRST && nmsg<=WM_MOUSELAST) {
			 // close startup menu and other popup menus
			 // This functionality is missing in MS Windows.
			if (nmsg==WM_LBUTTONDOWN || nmsg==WM_MBUTTONDOWN || nmsg==WM_RBUTTONDOWN
#ifdef WM_XBUTTONDOWN
				|| nmsg==WM_XBUTTONDOWN
#endif
				)
				CancelModes();

			NotifyIconSet::iterator found = IconHitTest(Point(lparam));

			if (found != _sorted_icons.end()) {
				NotifyInfo& entry = const_cast<NotifyInfo&>(*found);	// Why does GCC 3.3 need this additional const_cast ?!

				 // Notify the message if the owner is still alive
				if (IsWindow(entry._hWnd)) {
					if (nmsg == WM_MOUSEMOVE ||		// avoid to call blocking SendMessage() for merely moving the mouse over icons
						nmsg == WM_LBUTTONDOWN ||	// Some programs need PostMessage() instead of SendMessage().
						nmsg == WM_MBUTTONDOWN ||	// So call SendMessage() only for BUTTONUP and BLCLK messages
#ifdef WM_XBUTTONDOWN
						nmsg == WM_XBUTTONDOWN ||
#endif
						nmsg == WM_RBUTTONDOWN)
						PostMessage(entry._hWnd, entry._uCallbackMessage, entry._uID, nmsg);
					else {
						 // allow SetForegroundWindow() in client process
						DWORD pid;

						if (GetWindowThreadProcessId(entry._hWnd, &pid)) {
							 // bind dynamically to AllowSetForegroundWindow() to be compatible to WIN98
							static DynamicFct<BOOL(WINAPI*)(DWORD)> AllowSetForegroundWindow(TEXT("USER32"), "AllowSetForegroundWindow");

							if (AllowSetForegroundWindow)
								(*AllowSetForegroundWindow)(pid);
						}

						SendMessage(entry._hWnd, entry._uCallbackMessage, entry._uID, nmsg);
					}
				}
				else if (_icon_map.erase(entry))	// delete icons without valid owner window
					Refresh();
			}
		}

		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}

int NotifyArea::Notify(int id, NMHDR* pnmh)
{
	if (pnmh->code == TTN_GETDISPINFO) {
		LPNMTTDISPINFO pdi = (LPNMTTDISPINFO)pnmh;

		Point pt(GetMessagePos());
		ScreenToClient(_hwnd, &pt);

		NotifyIconSet::iterator found = IconHitTest(pt);

		if (found != _sorted_icons.end()) {
			NotifyInfo& entry = const_cast<NotifyInfo&>(*found);	// Why does GCC 3.3 need this additional const_cast ?!

			pdi->lpszText = (LPTSTR)entry._tipText.c_str();
		}
	}

	return 0;
}

void NotifyArea::CancelModes()
{
	PostMessage(HWND_BROADCAST, WM_CANCELMODE, 0, 0);

	for(NotifyIconSet::const_iterator it=_sorted_icons.begin(); it!=_sorted_icons.end(); ++it)
		PostMessage(it->_hWnd, WM_CANCELMODE, 0, 0);
}

LRESULT NotifyArea::ProcessTrayNotification(int notify_code, NOTIFYICONDATA* pnid)
{
	switch(notify_code) {
	  case NIM_ADD:
	  case NIM_MODIFY:
		if ((int)pnid->uID >= 0) {	///@todo This is a fix for Windows Task Manager.
			NotifyInfo& entry = _icon_map[pnid] = pnid;

			 // a new entry?
			if (entry._idx == -1)
				entry._idx = ++_next_idx;

#if NOTIFYICON_VERSION>=3	// as of 21.08.2003 missing in MinGW headers
			if (DetermineHideState(entry))
				entry._dwState |= NIS_HIDDEN;
#endif

			Refresh();	///@todo call only if really changes occurred

			return TRUE;
		}
		break;

	  case NIM_DELETE: {
		NotifyIconMap::iterator found = _icon_map.find(pnid);

		if (found != _icon_map.end()) {
			if (found->second._hIcon)
				DestroyIcon(found->second._hIcon);
			_icon_map.erase(found);
			Refresh();
			return TRUE;
		}
		break;}

#if NOTIFYICON_VERSION>=3	// as of 21.08.2003 missing in MinGW headers
	  case NIM_SETFOCUS:
		SetForegroundWindow(_hwnd);
		return TRUE;

	  case NIM_SETVERSION:
		NotifyIconMap::iterator found = _icon_map.find(pnid);

		if (found != _icon_map.end()) {
			found->second._version = pnid->UNION_MEMBER(uVersion);
			return TRUE;
		} else
			return FALSE;
#endif
	}

	return FALSE;
}

void NotifyArea::Refresh()
{
	_sorted_icons.clear();

	 // sort icon infos by display index
	for(NotifyIconMap::const_iterator it=_icon_map.begin(); it!=_icon_map.end(); ++it) {
		const NotifyInfo& entry = it->second;

#ifdef NIF_STATE	// as of 21.08.2003 missing in MinGW headers
		if (_show_hidden || !(entry._dwState & NIS_HIDDEN))
#endif
			_sorted_icons.insert(entry);
	}

	 // sync tooltip areas to current icon number
	if (_sorted_icons.size() != _last_icon_count) {
		RECT rect = {2, 3, 2+16, 3+16};
		size_t icon_cnt = _sorted_icons.size();

		size_t tt_idx = 0;
		while(tt_idx < icon_cnt) {
			_tooltip.add(_hwnd, tt_idx++, rect);

			rect.left += NOTIFYICON_DIST;
			rect.right += NOTIFYICON_DIST;
		}

		while(tt_idx < _last_icon_count)
			_tooltip.remove(_hwnd, tt_idx++);

		_last_icon_count = _sorted_icons.size();
	}

	SendMessage(GetParent(_hwnd), PM_RESIZE_CHILDREN, 0, 0);

	InvalidateRect(_hwnd, NULL, FALSE);	// refresh icon display
	UpdateWindow(_hwnd);
}

void NotifyArea::Paint()
{
	BufferedPaintCanvas canvas(_hwnd);

	 // first fill with the background color
	FillRect(canvas, &canvas.rcPaint, GetSysColorBrush(COLOR_BTNFACE));

#ifdef _ROS_
	DrawEdge(canvas, ClientRect(_hwnd), BDR_SUNKENOUTER, BF_RECT);
#endif

	 // draw icons
	int x = 2;
	int y = 3;

	for(NotifyIconSet::const_iterator it=_sorted_icons.begin(); it!=_sorted_icons.end(); ++it) {
		DrawIconEx(canvas, x, y, it->_hIcon, 16, 16, 0, 0, DI_NORMAL);

		x += NOTIFYICON_DIST;
	}
}

void NotifyArea::TimerTick()
{
	bool do_refresh = false;

	 // Look for task icons without valid owner window.
	 // This is an advanced feature, which is missing in MS Windows.
	for(NotifyIconSet::const_iterator it=_sorted_icons.begin(); it!=_sorted_icons.end(); ++it) {
		const NotifyInfo& entry = *it;

		if (!IsWindow(entry._hWnd))
			if (_icon_map.erase(entry))	// delete icons without valid owner window
				++do_refresh;
	}

	if (do_refresh)
		Refresh();
}

 /// search for a icon at a given client coordinate position
NotifyIconSet::iterator NotifyArea::IconHitTest(const POINT& pos)
{
	if (pos.y<2 || pos.y>=2+16)
		return _sorted_icons.end();

	NotifyIconSet::iterator it = _sorted_icons.begin();

	int x = 2;

	for(; it!=_sorted_icons.end(); ++it) {
		//NotifyInfo& entry = const_cast<NotifyInfo&>(*it);	// Why does GCC 3.3 need this additional const_cast ?!

		if (pos.x>=x && pos.x<x+16)
			break;

		x += NOTIFYICON_DIST;
	}

	return it;
}

#if NOTIFYICON_VERSION>=3	// as of 21.08.2003 missing in MinGW headers
bool NotifyArea::DetermineHideState(NotifyInfo& entry)
{
	if (entry._tipText == TEXT("FRITZ!fon"))
		return true;

	if (entry._tipText == TEXT("FRITZ!fax"))
		return true;

	TCHAR title[MAX_PATH];

	if (GetWindowText(entry._hWnd, title, MAX_PATH)) {
		if (_tcsstr(title, TEXT("Task Manager")))
			return false;

		if (_tcsstr(title, TEXT("AntiVir")))
			return true;

		if (_tcsstr(title, TEXT("Apache")))
			return true;

		if (_tcsstr(title, TEXT("FRITZ!web")))
			return true;
	}

	const String& modulePath = _window_modules[entry._hWnd];

	 // request module path for new windows (We will get an asynchronous answer by a WM_COPYDATA message.)
	if (modulePath.empty()) {
		_hook.GetModulePath(entry._hWnd, _hwnd);
		return false;
	}

	if (modulePath == TEXT("xyz"))	//@@
		return true;

	return false;
}
#endif


ClockWindow::ClockWindow(HWND hwnd)
 :	super(hwnd),
	_tooltip(hwnd)
{
	*_time = TEXT('\0');
	FormatTime();

	_tooltip.add(_hwnd, _hwnd);
}

HWND ClockWindow::Create(HWND hwndParent)
{
	static BtnWindowClass wcClock(CLASSNAME_CLOCKWINDOW, CS_DBLCLKS);

	ClientRect clnt(hwndParent);

	WindowCanvas canvas(hwndParent);
	FontSelection font(canvas, GetStockFont(DEFAULT_GUI_FONT));

	RECT rect = {0, 0, 0, 0};
	TCHAR buffer[16];

	if (!GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, NULL, NULL, buffer, sizeof(buffer)/sizeof(TCHAR)))
		_tcscpy(buffer, TEXT("00:00"));

	DrawText(canvas, buffer, -1, &rect, DT_SINGLELINE|DT_NOPREFIX|DT_CALCRECT);
	int clockwindowWidth = rect.right-rect.left + 4;

	return Window::Create(WINDOW_CREATOR(ClockWindow), 0,
							wcClock, NULL, WS_CHILD|WS_VISIBLE,
							clnt.right-(clockwindowWidth), 1, clockwindowWidth, clnt.bottom-2, hwndParent);
}

LRESULT ClockWindow::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_PAINT:
		Paint();
		break;

	  case WM_LBUTTONDBLCLK:
		//launch_file(_hwnd, TEXT("timedate.cpl"), SW_SHOWNORMAL);	// This would be enough, but we want the fastest solution.
		//launch_file(_hwnd, TEXT("rundll32.exe /d shell32.dll,Control_RunDLL timedate.cpl"), SW_SHOWNORMAL);
		RunDLL(_hwnd, TEXT("shell32"), "Control_RunDLL", TEXT("timedate.cpl"), SW_SHOWNORMAL);
		break;

	  default:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}

int ClockWindow::Notify(int id, NMHDR* pnmh)
{
	if (pnmh->code == TTN_GETDISPINFO) {
		LPNMTTDISPINFO pdi = (LPNMTTDISPINFO)pnmh;

		SYSTEMTIME systime;
		TCHAR buffer[64];

		GetLocalTime(&systime);
		GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &systime, NULL, buffer, 64);

		_tcscpy(pdi->szText, buffer);
	}

	return 0;
}

void ClockWindow::TimerTick()
{
	if (FormatTime())
		InvalidateRect(_hwnd, NULL, TRUE);	// refresh displayed time
}

bool ClockWindow::FormatTime()
{
	TCHAR buffer[16];

	if (GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, NULL, NULL, buffer, sizeof(buffer)/sizeof(TCHAR)))
		if (_tcscmp(buffer, _time)) {
			_tcscpy(_time, buffer);
			return true;	// The text to display has changed.
		}

	return false;	// no change
}

void ClockWindow::Paint()
{
	PaintCanvas canvas(_hwnd);

#ifdef _ROS_
	DrawEdge(canvas, ClientRect(_hwnd), BDR_SUNKENOUTER, BF_TOP|BF_RIGHT);
#endif

	BkMode bkmode(canvas, TRANSPARENT);
	FontSelection font(canvas, GetStockFont(ANSI_VAR_FONT));

	DrawText(canvas, _time, -1, ClientRect(_hwnd), DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
}

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
 // globals.h
 //
 // Martin Fuchs, 23.07.2003
 //


 /// structure containing global variable of Explorer
extern struct ExplorerGlobals
{
	ExplorerGlobals();

	HINSTANCE	_hInstance;
	ATOM		_hframeClass;
	UINT		_cfStrFName;
	HWND		_hMainWnd;
	bool		_prescan_nodes;
	bool		_desktop_mode;

	FILE*		_log;

#ifndef __MINGW32__	// SHRestricted() missing in MinGW (as of 29.10.2003)
	DWORD(STDAPICALLTYPE* _SHRestricted)(RESTRICTIONS);
#endif
} g_Globals;

#define	LOG(x) if (g_Globals._log) _ftprintf(g_Globals._log, TEXT("%s\n"), (LPCTSTR)(x));


 /// convenient loading of string resources
struct ResString : public String
{
	ResString(UINT nid);
};

 /// convenient loading of standard (32x32) icon resources
struct ResIcon
{
	ResIcon(UINT nid);

	operator HICON() const {return _hIcon;}

protected:
	HICON	_hIcon;
};

 /// convenient loading of small (16x16) icon resources
struct SmallIcon
{
	SmallIcon(UINT nid);

	operator HICON() const {return _hIcon;}

protected:
	HICON	_hIcon;
};

 /// convenient loading of icon resources with specified sizes
struct ResIconEx
{
	ResIconEx(UINT nid, int w, int h);

	operator HICON() const {return _hIcon;}

protected:
	HICON	_hIcon;
};

 /// set big and small icons out of the resources for a window
extern void SetWindowIcon(HWND hwnd, UINT nid);

 /// convenient loading of bitmap resources
struct ResBitmap
{
	ResBitmap(UINT nid);
	~ResBitmap() {DeleteObject(_hBmp);}

	operator HBITMAP() const {return _hBmp;}

protected:
	HBITMAP	_hBmp;
};

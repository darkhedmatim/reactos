/*
 *  ReactOS winfile
 *
 *  mdiclient.h
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
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

#ifndef __MDICLIENT_H__
#define __MDICLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"



LRESULT CALLBACK ChildWndProc(HWND, UINT, WPARAM, LPARAM);

HWND create_child_window(ChildWnd* child);

void toggle_child(HWND hwnd, UINT cmd, HWND hchild);
ChildWnd* alloc_child_window(LPCTSTR path);
void set_header(Pane* pane);

void toggle_child(HWND hwnd, UINT cmd, HWND hchild);
BOOL activate_drive_window(LPCTSTR path);



#ifdef __cplusplus
};
#endif

#endif // __MDICLIENT_H__

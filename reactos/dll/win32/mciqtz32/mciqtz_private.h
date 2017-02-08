/*
 * DirectShow MCI Driver
 *
 * Copyright 2009 Christian Costa
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

#ifndef __WINE_PRIVATE_MCIQTZ_H
#define __WINE_PRIVATE_MCIQTZ_H

#define COBJMACROS

#include <dshow.h>

typedef struct {
    HANDLE         thread;
    HANDLE         notify;
    HANDLE         done;
    DWORD          msg;
    DWORD_PTR      devid;
    DWORD          flags;
    DWORD_PTR      parms;
    LRESULT        res;
} WINE_MCIQTZ_TASK;

typedef struct {
    MCIDEVICEID    wDevID;
    BOOL           opened;
    BOOL           uninit;
    IGraphBuilder* pgraph;
    IMediaControl* pmctrl;
    IMediaSeeking* seek;
    IMediaEvent*   mevent;
    IVideoWindow*  vidwin;
    IBasicVideo*   vidbasic;
    IBasicAudio*   audio;
    DWORD          time_format;
    UINT           command_table;
    HWND           parent;
    MCIDEVICEID    notify_devid;
    HANDLE         callback;
    HANDLE         thread;
    HANDLE         stop_event;
    CRITICAL_SECTION cs;
    WINE_MCIQTZ_TASK task;
} WINE_MCIQTZ;

#endif  /* __WINE_PRIVATE_MCIQTZ_H */

/* $Id: cmdline.h,v 1.1 2004/11/01 20:49:32 gvg Exp $
 *
 *  FreeLdr boot loader
 *  Copyright (C) 2002, 2003 ReactOS Team
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

#ifndef __CMDLINE_H__
#define __CMDLINE_H__

typedef struct tagCMDLINEINFO
{
  char *DefaultOperatingSystem;
  S32 TimeOut;
} CMDLINEINFO, *PCMDLINEINFO;

extern void CmdLineParse(char *CmdLine);

extern char *CmdLineGetDefaultOS(void);
extern S32 CmdLineGetTimeOut(void);

#endif /* __CMDLINE_H__ */

/* EOF */

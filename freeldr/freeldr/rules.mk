#
#  FreeLoader
#  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
	

CC		= gcc
LD		= ld
AR		= ar
RM		= cmd /C del
CP		= cmd /C copy
MAKE	= make

# For a release build uncomment this line
FLAGS = -Wall -nostdlib -nostdinc -fno-builtin -I./ -I../ -O3

# For a debug build uncomment this line
#FLAGS = -Wall -nostdlib -nostdinc -fno-builtin -I./ -I../ -DDEBUG -O3

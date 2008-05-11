# $Id: Makefile,v 1.3 2003/09/12 17:51:48 vizzini Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = wsock32

TARGET_BASE = 0x75050000

TARGET_CFLAGS = \
 -I./include \
 -Wall \
 -Werror \
 -fno-builtin \
 -DUNICODE \
 -DLE \
 -DDBG

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = kernel32.a ws2_32.a mswsock.a

TARGET_OBJECTS = stubs.o

TARGET_CLEAN = $(DEP_FILES)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# Automatic dependency tracking
DEP_OBJECTS := $(TARGET_OBJECTS)
include $(PATH_TO_TOP)/tools/depend.mk

# EOF

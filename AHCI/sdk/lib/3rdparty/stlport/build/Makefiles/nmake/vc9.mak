
CFLAGS_COMMON = /nologo /W4 /EHsc
CXXFLAGS_COMMON = /nologo /W4 /EHsc

DEFS_STLDBG = /GS
DEFS_STATIC_STLDBG = /GS

OPT_REL = $(OPT_REL) /GL
LDFLAGS_REL = $(LDFLAGS_REL) /LTCG


!include vc-common.mak


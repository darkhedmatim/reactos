.SUFFIXES:
.SUFFIXES: .cpp .c .obj .lib .sys *.pdb

DDKINC=f:\ddk\inc
DDKLIB=f:\ddk\lib\i386\free

MSVC_SPEC=-cbstring

CXX=cl -c
CXXFLAGS=-nologo -Gz -W3 -GX -Z7 -Gy -Oy- -Od -I. -I$(DDKINC)\
         -D_DEBUG -DDBG=1 -D_X86_=1 -D_WIN32_WINNT=0x0400 $(MSVC_SPEC)

LINK=link
LFLAGS=-nologo -nodefaultlib -entry:"DriverEntry" -base:0x10000\
       -libpath:$(DDKLIB) -driver -debug\
       -subsystem:native,4.0 -version:4.0\
       -osversion:4.0 -opt:ref -opt:icf -opt:nowin98

LIBS=ntoskrnl.lib int64.lib Hal.lib ScsiPort.lib

all: uniata.sys

.cpp.obj::
        $(CXX) $(CXXFLAGS) $<

uniata.sys: id_ata.obj id_dma.obj id_probe.obj
        $(LINK) $(LFLAGS) -out:$@ -pdb:$*.pdb $** $(LIBS)

clean:
        -del *.obj *.sys *.pdb

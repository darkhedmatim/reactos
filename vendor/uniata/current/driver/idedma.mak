#////////////////////////////////////////////////////////////////////
#// Alexander A. Telyatnikov (Alter) 2002-2007. Kiev, Ukraine
#// All rights reserved
#////////////////////////////////////////////////////////////////////

!IF "$(CFG)" == ""
CFG=UniATA - Win32 Debug
!MESSAGE No configuration specified. Defaulting to UniATA - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "UniATA - Win32 Release" && "$(CFG)" != "UniATA - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "UniATA.mak" CFG="UniATA - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "UniATA - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "UniATA - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

#makefile for nt4
BaseDir=$(BASEDIR)
BaseDirLib=$(BASEDIR351)
DDKINC=/I $(BaseDir)\inc

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe
LINK32=link.exe

!IF  "$(CFG)" == "UniATA - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
LibPath=$(BaseDirLib)\Lib\i386\Free\

!ELSEIF  "$(CFG)" == "UniATA - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
LibPath=$(BaseDirLib)\Lib\i386\Checked\

!ENDIF 

# Begin Custom Macros
OutDir=$(OUTDIR)
DistDir=$(OUTDIR)_Dist
SymDir=$(OUTDIR)_sym
# End Custom Macros

TargetPath=$(OUTDIR)\IdeDma.sys
InputPath=$(OUTDIR)\IdeDma.sys
SOURCE="$(InputPath)"

DEF_FILE= \
	".\IdeDma.def"
LINK32_OBJS= \
	"$(INTDIR)\id_probe.obj" \
	"$(INTDIR)\id_ata.obj" \
	"$(INTDIR)\id_dma.obj" \
	"$(INTDIR)\id_init.obj" \
	"$(INTDIR)\id_queue.obj" \
	"$(INTDIR)\id_sata.obj" \
	"$(INTDIR)\id_badblock.obj" \
	"$(INTDIR)\stdafx.obj" \
	"$(INTDIR)\idedma.res"


!IF  "$(CFG)" == "UniATA - Win32 Release"
ALL : "$(OUTDIR)\IdeDma.sys" ".\copy.msg"
!ELSEIF  "$(CFG)" == "UniATA - Win32 Debug"
ALL : "$(OUTDIR)\IdeDma.sys" ".\copy.msg"
!ENDIF 

"build_inf.exe" : "bm_devs.h"
!IF "$(NO_BUILD_INF)" == ""
	cd ..\build_inf
	nmake /A CFG="build_inf - Win32 Release"
	cd ..\driver
	copy ..\build_inf\Release\build_inf.exe .\build_inf.exe
	copy ..\build_inf\Release\build_inf.exe .\Dist\build_inf.exe
!ENDIF 


"atactl.exe" : "uata_ctl.h"
!IF "$(NO_BUILD_INF)" == ""
	cd ..\atactl
	nmake /A CFG="atactl - Win32 Release"
	cd ..\driver
	copy ..\atactl\Release\atactl.exe .\atactl.exe
	copy ..\atactl\Release\atactl.exe .\Dist\atactl.exe
!ENDIF 

#ALL :
#	nmake CFG="UniATA - Win32 Release"
#	nmake CFG="UniATA - Win32 Debug"

PKG : "BusMaster_v$(VER).rar"


"Release\idedma.sys" :
	nmake /A CFG="UniATA - Win32 Release"

"Debug\idedma.sys" :
	nmake /A CFG="UniATA - Win32 Debug"

"BusMaster_v$(VER).rar" : "$(OUTDIR)\IdeDma.sys" ".\copy.msg"
	cd ..
	driver\Dist\fix_dep.bat
	cd driver
!IF  "$(CFG)" == "UniATA - Win32 Release"
	nmake /A CFG="UniATA - Win32 Debug"
	nmake /A CFG="UniATA - Win32 Release"
!ELSEIF  "$(CFG)" == "UniATA - Win32 Debug"
	nmake /A CFG="UniATA - Win32 Release"
	nmake /A CFG="UniATA - Win32 Debug"
!ENDIF 
	rar a -s -mdD -m5 BusMaster_v$(VER).rar @..\pkg_files.dist
	rar a -s -mdD -m5 BusMaster_v$(VER)_Dbg.rar Debug_Dist
	rar a -s -mdD -m5 BusMaster_v$(VER)_src.rar @..\pkg_files.src
	rar a -s -mdD -m5 BusMaster_v$(VER)_sym.rar Release_sym

CLEAN :
	-@erase $(LINK32_OBJS)
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\IdeDma.sys"
	-@erase ".\copy.msg"
	-@erase "$(INTDIR)\idedma.pch"
!IF  "$(CFG)" == "UniATA - Win32 Debug"
	-@erase "$(OUTDIR)\IdeDma.map"
	-@erase "$(OUTDIR)\IdeDma.pdb"
!ENDIF 

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(DistDir)" :
    if not exist "$(DistDir)/$(NULL)" mkdir "$(DistDir)"

!IF  "$(CFG)" == "UniATA - Win32 Release"

CPP_PROJ_BASE=/nologo /Gz /MT /W3 /GX /O2 $(DDKINC) /DNDEBUG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /DIdeDma_EXPORTS /D_X86_ /D_WIN32_WINNT=0x0400 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_PROJ=$(CPP_PROJ_BASE) /Yu"stdafx.h"

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
	
LINK32_FLAGS=/LIBPATH:$(LibPath) ntoskrnl.lib int64.lib Hal.lib ScsiPort.lib .\Lib\Release\CrossNtK.lib /nologo /entry:"DriverEntry" /incremental:no /debug /pdb:"$(OUTDIR)\IdeDma.pdb" /machine:I386 /nodefaultlib /def:".\IdeDma.def" /out:"$(OUTDIR)\IdeDma.sys" /driver /pdbtype:sept /subsystem:native /opt:ref /opt:icf
#LINK32_FLAGS=$(BaseDir)\Lib\i386\Free\ntoskrnl.lib $(BaseDir)\Lib\i386\Free\int64.lib $(BaseDir)\Lib\i386\Checked\Hal.lib /nologo /entry:"DriverEntry" /incremental:no /pdb:"$(OUTDIR)\IdeDma.pdb" /machine:I386 /nodefaultlib /def:".\IdeDma.def" /out:"$(OUTDIR)\IdeDma.sys" /driver /subsystem:native 

!ELSEIF  "$(CFG)" == "UniATA - Win32 Debug"

CPP_PROJ_BASE=/nologo /Gz /MTd /W3 /GX /Z7 /Od $(DDKINC) /D_DEBUG /DDBG /DWIN32 /D_WINDOWS /D_MBCS /D_USRDLL /DIdeDma_EXPORTS /D_X86_ /D_WIN32_WINNT=0x0400 /Fp"$(INTDIR)\idedma.pch" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_PROJ=$(CPP_PROJ_BASE) /Yu"stdafx.h"

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 

LINK32_FLAGS=/LIBPATH:$(LibPath) ntoskrnl.lib int64.lib Hal.lib ScsiPort.lib .\Lib\Debug\PostDbgMesgK.lib .\Lib\Debug\CrossNtK.lib /nologo /entry:"DriverEntry" /incremental:no /pdb:"$(OUTDIR)\IdeDma.pdb" /map:"$(INTDIR)\IdeDma.map" /debug /machine:I386 /nodefaultlib /def:".\IdeDma.def" /out:"$(OUTDIR)\IdeDma.sys" /pdbtype:sept /driver /subsystem:native,4.00 

!ENDIF 

!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("idedma.dep")
!INCLUDE "idedma.dep"
!ELSE 
!MESSAGE Warning: cannot find "idedma.dep"
!ENDIF 
!ENDIF 

"$(OUTDIR)\IdeDma.sys" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

".\copy.msg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)" "$(DistDir)" "build_inf.exe" "atactl.exe"
	<<tempfile.bat 
	@echo off 
	copy $(TargetPath) C:\WINNT\System32\Drivers\IdeDma.sys 
	copy $(TargetPath) C:\WINNT\System32\Drivers\IdeDmb.sys 
	copy $(TargetPath) $(DistDir)\uniata.sys
!IF  "$(CFG)" == "UniATA - Win32 Release"
	copy $(OutDir)\idedma.pdb  $(SymDir)\uniata.pdb
!ELSEIF  "$(CFG)" == "UniATA - Win32 Debug"
	copy $(OutDir)\idedma.pdb  $(DistDir)\uniata.pdb
!ENDIF 
	cd Dist
	call rebuild_inf.bat $(VER)
	cd ..
	copy Dist\* $(DistDir)
	echo $(DistDir)
	echo Driver copied > copy.msg 
<< 

!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("IdeDma.dep")
!INCLUDE "IdeDma.dep"
!ELSE 
!MESSAGE Warning: cannot find "IdeDma.dep"
!ENDIF 
!ENDIF 


REL_PATH=.
SRC_EXT=cpp

SRC=id_ata
!INCLUDE build.mak

SRC=id_dma
!INCLUDE build.mak

SRC=id_init
!INCLUDE build.mak

SRC=id_probe
!INCLUDE build.mak

SRC=id_queue
!INCLUDE build.mak

SRC=id_sata
!INCLUDE build.mak

SRC=id_badblock
!INCLUDE build.mak

SRC=stdafx
CPP_PROJ=$(CPP_PROJ_BASE) /Yc"stdafx.h"
"$(INTDIR)\StdAfx.obj" "$(INTDIR)\idedma.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_PROJ) $(SOURCE)
<<


SRC=idedma

"uniata_ver.h_.lk" :
    echo "." > "uniata_ver.h_.lk"

"uniata_ver.h_" : "uniata_ver.h_.lk"
    del "uniata_ver.h_.lk"

"uniata_ver.h" : "uniata_ver.h_"
    copy "uniata_ver.h_" "uniata_ver.h"
    srchrep.exe -src "**VER**" -dest "$(VER)" "uniata_ver.h"

"$(INTDIR)\$(SRC).res" : "$(SRC).rc" "$(INTDIR)" "uniata_ver.h"
    -@erase ".\$(SRC).res"
    $(RSC) $(RSC_PROJ) "$(SRC).rc"
    -@erase "$(INTDIR)\$(SRC).res"
    -@move "$(SRC).res" "$(INTDIR)\$(SRC).res"


# Microsoft Developer Studio Generated NMAKE File, Based on build_inf.dsp
!IF "$(CFG)" == ""
CFG=build_inf - Win32 Debug
!MESSAGE No configuration specified. Defaulting to build_inf - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "build_inf - Win32 Release" && "$(CFG)" != "build_inf - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "build_inf.mak" CFG="build_inf - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "build_inf - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "build_inf - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "build_inf - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\build_inf.exe"


CLEAN :
	-@erase "$(INTDIR)\build_inf.obj"
	-@erase "$(INTDIR)\SvcManLib.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\build_inf.exe"
	-@erase "$(OUTDIR)\build_inf.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O1 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "USER_MODE" /Fp"$(INTDIR)\build_inf.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\build_inf.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib advapi32.lib /nologo /subsystem:console /pdb:none /map:"$(INTDIR)\build_inf.map" /machine:I386 /out:"$(OUTDIR)\build_inf.exe" /align:512 /merge:.bss=.data 
LINK32_OBJS= \
	"$(INTDIR)\build_inf.obj" \
	"$(INTDIR)\SvcManLib.obj"

"$(OUTDIR)\build_inf.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE="$(InputPath)"
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\build_inf.exe"
   copy Release\build_inf.exe ..\driver
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "build_inf - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\build_inf.exe"


CLEAN :
	-@erase "$(INTDIR)\build_inf.obj"
	-@erase "$(INTDIR)\SvcManLib.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\build_inf.exe"
	-@erase "$(OUTDIR)\build_inf.ilk"
	-@erase "$(OUTDIR)\build_inf.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "USER_MODE" /Fp"$(INTDIR)\build_inf.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\build_inf.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\build_inf1.pdb" /debug /machine:I386 /out:"$(OUTDIR)\build_inf.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\build_inf.obj" \
	"$(INTDIR)\SvcManLib.obj"

"$(OUTDIR)\build_inf.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("build_inf.dep")
!INCLUDE "build_inf.dep"
!ELSE 
!MESSAGE Warning: cannot find "build_inf.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "build_inf - Win32 Release" || "$(CFG)" == "build_inf - Win32 Debug"
SOURCE=.\build_inf.cpp

"$(INTDIR)\build_inf.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\SvcManLib.cpp

"$(INTDIR)\SvcManLib.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 


#////////////////////////////////////////////////////////////////////
#// Alexander A. Telyatnikov (Alter) 2002-2004. Kiev, Ukraine
#// All rights reserved
#////////////////////////////////////////////////////////////////////

SOURCE="$(REL_PATH)\$(SRC).$(SRC_EXT)"
!IF  "$(CFG)" == "IdeDma - Win32 Release"
"$(INTDIR)\$(SRC).obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\idedma.pch"
!ELSEIF  "$(CFG)" == "IdeDma - Win32 Debug"
"$(INTDIR)\$(SRC).obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\idedma.pch"
!ENDIF 

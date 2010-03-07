!define PRODUCT_NAME "ReactOS Build Environment for Windows"
!define PRODUCT_VERSION "1.5"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\RosBE.cmd"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKCU"
!define PRODUCT_STARTMENU_REGVAL "NSIS:StartMenuDir"

;;
;; Basic installer options
;;
Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "RosBE-${PRODUCT_VERSION}.exe"
InstallDirRegKey HKCU "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

;;
;; Add version/product information metadata to the installation file.
;;
VIAddVersionKey /LANG=1033 "FileVersion" "1.5.0.0"
VIAddVersionKey /LANG=1033 "ProductVersion" "${PRODUCT_VERSION}"
VIAddVersionKey /LANG=1033 "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey /LANG=1033 "Comments" "This installer was written by Peter Ward and Daniel Reimer using Nullsoft Scriptable Install System (http://nsis.sourceforge.net/)"
VIAddVersionKey /LANG=1033 "CompanyName" "ReactOS Team"
VIAddVersionKey /LANG=1033 "LegalTrademarks" "Copyright � 2010 ReactOS Team"
VIAddVersionKey /LANG=1033 "LegalCopyright" "Copyright � 2010 ReactOS Team"
VIAddVersionKey /LANG=1033 "FileDescription" "${PRODUCT_NAME} Setup"
VIProductVersion "1.5.0.0"

CRCCheck force
SetDatablockOptimize on
XPStyle on
SetCompressor /FINAL /SOLID lzma

!include "MUI2.nsh"
!include "InstallOptions.nsh"
!include "RosSourceDir.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"

;;
;; Read our custom page ini, remove previous version and make sure only
;; one instance of the installer is running.
;;
Function .onInit
    ReadRegStr $R3 HKLM \
    "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
    StrCpy $R4 $R3 3
    System::Call 'kernel32::CreateMutexA(i 0, i 0, t "RosBE-v${PRODUCT_VERSION}-Installer") i .r1 ?e'
    Pop $R0
    StrCmp $R0 0 +3
        MessageBox MB_OK|MB_ICONEXCLAMATION "The ${PRODUCT_NAME} v${PRODUCT_VERSION} installer is already running."
        Abort
    ${If} ${RunningX64}
        StrCpy $INSTDIR "$PROGRAMFILES64\RosBE"
    ${Else}
        StrCpy $INSTDIR "$PROGRAMFILES\RosBE"
    ${Endif}
    Call UninstallPrevious
    !insertmacro INSTALLOPTIONS_EXTRACT "RosSourceDir.ini"
FunctionEnd

;;
;; MUI Settings
;;
!define MUI_ABORTWARNING
!define MUI_ICON "Icons\rosbe.ico"
!define MUI_UNICON "Icons\uninstall.ico"
!define MUI_COMPONENTSPAGE_NODESC

!define MUI_WELCOMEPAGE_TITLE_3LINES
!insertmacro MUI_PAGE_WELCOME
;!insertmacro MUI_PAGE_LICENSE "Root\License.txt"
!insertmacro MUI_PAGE_DIRECTORY

;;
;; ReactOS Source Directory Pages
;;
var REACTOS_SOURCE_DIRECTORY
!insertmacro CUSTOM_PAGE_ROSDIRECTORY

;;
;; Start menu page
;;
var ICONS_GROUP
!define MUI_STARTMENUPAGE_NODISABLE
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "ReactOS Build Environment"
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "${PRODUCT_STARTMENU_REGVAL}"
!insertmacro MUI_PAGE_STARTMENU Application $ICONS_GROUP

!insertmacro MUI_PAGE_COMPONENTS

!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_TITLE_3LINES
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\README.pdf"
!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
!insertmacro MUI_PAGE_FINISH

;;
;; Uninstaller pages
;;
!insertmacro MUI_UNPAGE_INSTFILES

;;
;;  Language and reserve files
;;
ReserveFile "${NSISDIR}\Plugins\InstallOptions.dll"
!insertmacro MUI_LANGUAGE "English"

Section -BaseFiles SEC01
    SetShellVarContext current
    SetOutPath "$INSTDIR"
    SetOverwrite try
    File /r Icons\rosbe.ico
    File /r Root\README.pdf
    File /r Root\changelog.txt
    File /r Root\LICENSE.txt
    File /r Root\Basedir.cmd
    File /r Root\Build-Shared.cmd
    File /r Root\charch.cmd
    File /r Root\chdefgcc.cmd
    File /r Root\Clean.cmd
    File /r Root\Help.cmd
    File /r Root\Make.cmd
    File /r Root\Makex.cmd
    File /r Root\Remake.cmd
    File /r Root\Remakex.cmd
    File /r Root\Renv.cmd
    File /r Root\RosBE.cmd
    File /r Root\rosbe-gcc-env.cmd
    File /r Root\TimeDate.cmd
    File /r Root\version.cmd
    SetOutPath "$INSTDIR\Tools"
    SetOverwrite try
    File /r Components\Tools\flash.exe
    File /r Components\Tools\buildtime.exe
    File /r Components\Tools\chknewer.exe
    File /r Components\Tools\cpucount.exe
    File /r Components\Tools\getdate.exe
    File /r Components\Tools\getincludes.exe
    File /r Components\Tools\rquote.exe
    File /r Components\Tools\tee.exe
    File /r Components\Tools\make.exe
    File /r Components\Tools\libintl3.dll
    File /r Components\Tools\libiconv2.dll
    File /r Components\Tools\regex2.dll
    File /r Components\Tools\bison.exe
    File /r Components\Tools\flex.exe
    File /r Components\Tools\flex++.exe
    File /r Components\Tools\iconv.exe
    File /r Components\Tools\m4.exe
    File /r Components\Tools\patch.exe
SectionEnd

Section -MinGWGCCNASM SEC02
    SetOutPath "$INSTDIR\i386"
    SetOverwrite try
    File /r Components\i386\*.*
SectionEnd

Section "RosBE Configurator (options)" SEC03
    SetShellVarContext current
    SetOutPath "$INSTDIR"
    SetOverwrite try
    File /r Root\options.cmd
    SetOutPath "$INSTDIR\Tools"
    SetOverwrite try
    File /r Components\Tools\options.exe
    !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
        CreateDirectory "$SMPROGRAMS\$ICONS_GROUP"
        SetOutPath $INSTDIR
        CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\Options.lnk" \
                       "$INSTDIR\Tools\options.exe"
    !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section "Subversion Tools (svn, ssvn)" SEC04
    SetShellVarContext current
    SetOutPath "$INSTDIR"
    SetOverwrite try
    File /r Root\sSVN.cmd
    SetOutPath "$INSTDIR\Tools"
    SetOverwrite try
    File /r Components\Tools\svn.exe
    File /r Components\Tools\intl3_svn.dll
    File /r Components\Tools\libapr-1.dll
    File /r Components\Tools\libeay32.dll
    File /r Components\Tools\ssleay32.dll
    File /r Components\Tools\libaprutil-1.dll
    File /r Components\Tools\libdb41.dll
    File /r Components\Tools\libapriconv-1.dll
    File /r Components\Tools\libsasl.dll
    File /r Components\Tools\libsvn_client-1.dll
    File /r Components\Tools\libsvn_delta-1.dll
    File /r Components\Tools\libsvn_diff-1.dll
    File /r Components\Tools\libsvn_fs-1.dll
    File /r Components\Tools\libsvn_ra-1.dll
    File /r Components\Tools\libsvn_repos-1.dll
    File /r Components\Tools\libsvn_subr-1.dll
    File /r Components\Tools\libsvn_wc-1.dll
    SetOutPath "$INSTDIR\Tools\licenses"
    SetOverwrite try
    File /r Components\Tools\licenses\*.*
SectionEnd

Section "Shortcut Tool (scut)" SEC05
    SetShellVarContext current
    SetOutPath "$INSTDIR"
    SetOverwrite try
    File /r Root\scut.cmd
    SetOutPath "$INSTDIR\Tools"
    SetOverwrite try
    File /r Components\Tools\scut.exe
SectionEnd

Section "ccache - Compiler Cache" SEC06
    SetShellVarContext current
    SetOutPath "$INSTDIR\Tools"
    SetOverwrite try
    File /r Components\Tools\ccache.exe
    File /r Components\Tools\cygwin1.dll
SectionEnd

Section "GDB - The GNU Project Debugger" SEC07
    SetShellVarContext current
    SetOutPath "$INSTDIR\Tools"
    SetOverwrite try
    File /r Components\Tools\gdb.exe
    File /r Components\Tools\gdbserver.exe
SectionEnd

Section "relAddr2Line / kdbg Tools" SEC08
    SetShellVarContext current
    SetOutPath "$INSTDIR"
    SetOverwrite try
    File /r Root\raddr2line.cmd
    File /r Root\kdbg.cmd
    SetOutPath "$INSTDIR\Tools"
    SetOverwrite try
    File /r Components\Tools\echoh.exe
    File /r Components\Tools\log2lines.exe
    File /r Components\Tools\piperead.exe
    File /r Components\Tools\chkslash.exe
SectionEnd

Section "Other Tools (chdefdir and config)" SEC09
    SetShellVarContext current
    SetOutPath "$INSTDIR"
    SetOverwrite try
    File /r Root\chdefdir.cmd
    File /r Root\Config.cmd
SectionEnd

Section "Update Script" SEC10
    SetShellVarContext current
    SetOutPath "$INSTDIR\Tools"
    SetOverwrite try
    File /r Components\Tools\wget.exe
    File /r Components\Tools\7z.exe
    File /r Components\Tools\libeay32.dll
    File /r Components\Tools\libssl32.dll
    File /r Components\Tools\elevate.exe
    SetOutPath "$INSTDIR"
    SetOverwrite try
    File /r Root\update.cmd
SectionEnd

Section "PowerShell Version" SEC11
SetShellVarContext current
    SetOutPath "$INSTDIR"
    SetOverwrite try
    File /r Components\Powershell\Build.ps1
    File /r Components\Powershell\RosBE.ps1
    File /r Components\Powershell\rosbe-gcc-env.ps1
    File /r Components\Powershell\Help.ps1
    File /r Components\Powershell\chdefgcc.ps1
    File /r Components\Powershell\Clean.ps1
    File /r Components\Powershell\version.ps1
    File /r Components\Powershell\charch.ps1
    File /r Components\Powershell\chdefdir.ps1
    File /r Components\Powershell\Config.ps1
    File /r Components\Powershell\options.ps1
    File /r Components\Powershell\Remake.ps1
    File /r Components\Powershell\Remakex.ps1
    File /r Components\Powershell\kdbg.ps1
    File /r Components\Powershell\reladdr2line.ps1
    File /r Components\Powershell\scut.ps1
    File /r Components\Powershell\sSVN.ps1
    File /r Components\Powershell\update.ps1
    WriteRegStr HKLM "SOFTWARE\Microsoft\PowerShell\1\ShellIds\Microsoft.PowerShell" "ExecutionPolicy" "RemoteSigned"
    WriteRegStr HKLM "SOFTWARE\Wow6432Node\Microsoft\PowerShell\1\ShellIds\Microsoft.PowerShell" "ExecutionPolicy" "RemoteSigned"
    SetOutPath "$DESKTOP"
    SetOverwrite try
    File /r "Components\Powershell\RosBE PS - PostInstall.reg"
    MessageBox MB_ICONINFORMATION|MB_OK \
               "A REG-File was generated on your desktop. Please use it with Admin Rights to set Powershell's execution rights correctly if your RosBE Powershell Version fails to run after install. Otherwise, just delete it."
SectionEnd

Section -StartMenuShortcuts SEC12
    SetShellVarContext current

    ;;
    ;; Add our start menu shortcuts.
    ;;
    IfFileExists "$SMPROGRAMS\$ICONS_GROUP\ReactOS Build Environment.lnk" +13 0
        !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
            CreateDirectory "$SMPROGRAMS\$ICONS_GROUP"
            SetOutPath $REACTOS_SOURCE_DIRECTORY
            IfFileExists "$INSTDIR\RosBE.cmd" 0 +2
                CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\ReactOS Build Environment.lnk" "$SYSDIR\cmd.exe" '/t:0A /k "$INSTDIR\RosBE.cmd"' "$INSTDIR\rosbe.ico"
            IfFileExists "$INSTDIR\RosBE.ps1" 0 +2
                CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\ReactOS Build Environment - Powershell.lnk" "$SYSDIR\WindowsPowerShell\v1.0\powershell.exe" "-noexit &'$INSTDIR\RosBE.ps1'" "$INSTDIR\rosbe.ico"
            SetOutPath $INSTDIR
            CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\Uninstall RosBE.lnk" \
                           "$INSTDIR\Uninstall-${PRODUCT_VERSION}.exe"
            CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\Readme.lnk" \
                           "$INSTDIR\README.pdf"
    !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section /o "Desktop Shortcuts" SEC13
    SetShellVarContext current

    ;;
    ;; Add our desktop shortcuts.
    ;;
    IfFileExists "$DESKTOP\ReactOS Build Environment.lnk" +6 0
        SetOutPath $REACTOS_SOURCE_DIRECTORY
        IfFileExists "$INSTDIR\RosBE.cmd" 0 +2
            CreateShortCut "$DESKTOP\ReactOS Build Environment.lnk" "$SYSDIR\cmd.exe" '/t:0A /k "$INSTDIR\RosBE.cmd"' "$INSTDIR\rosbe.ico"
        IfFileExists "$INSTDIR\RosBE.ps1" 0 +2
            CreateShortCut "$DESKTOP\ReactOS Build Environment - Powershell.lnk" "$SYSDIR\WindowsPowerShell\v1.0\powershell.exe" "-noexit &'$INSTDIR\RosBE.ps1'" "$INSTDIR\rosbe.ico"
SectionEnd

Section /o "Quick Launch Shortcuts" SEC14
    SetShellVarContext current

    ;;
    ;; Add our quick launch shortcuts.
    ;;
    IfFileExists "$QUICKLAUNCH\ReactOS Build Environment.lnk" +6 0
        SetOutPath $REACTOS_SOURCE_DIRECTORY
        IfFileExists "$INSTDIR\RosBE.cmd" 0 +2
            CreateShortCut "$QUICKLAUNCH\ReactOS Build Environment.lnk" "$SYSDIR\cmd.exe" '/t:0A /k "$INSTDIR\RosBE.cmd"' "$INSTDIR\rosbe.ico"
        IfFileExists "$INSTDIR\RosBE.ps1" 0 +2
            CreateShortCut "$QUICKLAUNCH\ReactOS Build Environment - Powershell.lnk" "$SYSDIR\WindowsPowerShell\v1.0\powershell.exe" "-noexit &'$INSTDIR\RosBE.ps1'" "$INSTDIR\rosbe.ico"
SectionEnd

Section -Post SEC15
    WriteUninstaller "$INSTDIR\Uninstall-${PRODUCT_VERSION}.exe"
    WriteRegStr HKCU "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\RosBE.cmd"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\Uninstall-${PRODUCT_VERSION}.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
SectionEnd

Function un.onUninstSuccess
    HideWindow
    MessageBox MB_ICONINFORMATION|MB_OK \
               "ReactOS Build Environment was successfully removed from your computer."
FunctionEnd

Function un.onInit
    MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 \
        "Are you sure you want to remove ReactOS Build Environment and all of its components?" \
        IDYES +2
    Abort
    IfFileExists "$PROFILE\RosBE\." 0 +5
        MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 \
        "Do you want to remove the ReactOS Build Environment configuration file from the Profile Path?" \
        IDNO +2
        RMDir /r /REBOOTOK "$PROFILE\RosBE"
    IfFileExists "$APPDATA\RosBE\." 0 +5
        MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 \
        "Do you want to remove the ReactOS Build Environment configuration file from the Application Data Path?" \
        IDNO +2
        RMDir /r /REBOOTOK "$APPDATA\RosBE"
    MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 \
    "Do you want to remove the Shortcuts? If you just want to Update to a new Version of RosBE, keep them. This keeps your previous settings." \
    IDNO +5
        Delete /REBOOTOK "$DESKTOP\ReactOS Build Environment.lnk"
        Delete /REBOOTOK "$QUICKLAUNCH\ReactOS Build Environment.lnk"
        Delete /REBOOTOK "$DESKTOP\ReactOS Build Environment - Powershell.lnk"
        Delete /REBOOTOK "$QUICKLAUNCH\ReactOS Build Environment - Powershell.lnk"
FunctionEnd

Section Uninstall
    !insertmacro MUI_STARTMENU_GETFOLDER "Application" $ICONS_GROUP
    SetShellVarContext current

    ;;
    ;; Clean up installed files.
    ;;
    RMDir /r /REBOOTOK "$INSTDIR\i386"
    RMDir /r /REBOOTOK "$INSTDIR\Tools"
    StrCmp $ICONS_GROUP "" NO_SHORTCUTS
    RMDir /r /REBOOTOK "$SMPROGRAMS\$ICONS_GROUP"
    NO_SHORTCUTS:
    Delete /REBOOTOK "$INSTDIR\Basedir.cmd"
    Delete /REBOOTOK "$INSTDIR\Build.ps1"
    Delete /REBOOTOK "$INSTDIR\Build-Shared.cmd"
    Delete /REBOOTOK "$INSTDIR\chdefdir.cmd"
    Delete /REBOOTOK "$INSTDIR\chdefdir.ps1"
    Delete /REBOOTOK "$INSTDIR\charch.cmd"
    Delete /REBOOTOK "$INSTDIR\charch.ps1"
    Delete /REBOOTOK "$INSTDIR\chdefgcc.cmd"
    Delete /REBOOTOK "$INSTDIR\chdefgcc.ps1"
    Delete /REBOOTOK "$INSTDIR\Clean.cmd"
    Delete /REBOOTOK "$INSTDIR\Clean.ps1"
    Delete /REBOOTOK "$INSTDIR\Config.cmd"
    Delete /REBOOTOK "$INSTDIR\Config.ps1"
    Delete /REBOOTOK "$INSTDIR\Help.cmd"
    Delete /REBOOTOK "$INSTDIR\Help.ps1"
    Delete /REBOOTOK "$INSTDIR\Make.cmd"
    Delete /REBOOTOK "$INSTDIR\Makex.cmd"
    Delete /REBOOTOK "$INSTDIR\options.cmd"
    Delete /REBOOTOK "$INSTDIR\options.ps1"
    Delete /REBOOTOK "$INSTDIR\raddr2line.cmd"
    Delete /REBOOTOK "$INSTDIR\reladdr2line.ps1"
    Delete /REBOOTOK "$INSTDIR\Renv.cmd"
    Delete /REBOOTOK "$INSTDIR\Remake.cmd"
    Delete /REBOOTOK "$INSTDIR\Remakex.cmd"
    Delete /REBOOTOK "$INSTDIR\Remake.ps1"
    Delete /REBOOTOK "$INSTDIR\Remakex.ps1"
    Delete /REBOOTOK "$INSTDIR\kdbg.cmd"
    Delete /REBOOTOK "$INSTDIR\kdbg.ps1"
    Delete /REBOOTOK "$INSTDIR\RosBE.cmd"
    Delete /REBOOTOK "$INSTDIR\RosBE.ps1"
    Delete /REBOOTOK "$INSTDIR\rosbe-gcc-env.cmd"
    Delete /REBOOTOK "$INSTDIR\rosbe-gcc-env.ps1"
    Delete /REBOOTOK "$INSTDIR\scut.cmd"
    Delete /REBOOTOK "$INSTDIR\scut.ps1"
    Delete /REBOOTOK "$INSTDIR\sSVN.cmd"
    Delete /REBOOTOK "$INSTDIR\sSVN.ps1"
    Delete /REBOOTOK "$INSTDIR\TimeDate.cmd"
    Delete /REBOOTOK "$INSTDIR\update.cmd"
    Delete /REBOOTOK "$INSTDIR\update.ps1"
    Delete /REBOOTOK "$INSTDIR\README.pdf"
    Delete /REBOOTOK "$INSTDIR\rosbe.ico"
    Delete /REBOOTOK "$INSTDIR\uninstall.ico"
    Delete /REBOOTOK "$INSTDIR\ChangeLog.txt"
    Delete /REBOOTOK "$INSTDIR\LICENSE.txt"
    Delete /REBOOTOK "$INSTDIR\srclist.txt"
    Delete /REBOOTOK "$INSTDIR\config.rbuild"
    Delete /REBOOTOK "$INSTDIR\version.cmd"
    Delete /REBOOTOK "$INSTDIR\version.ps1"
    Delete /REBOOTOK "$INSTDIR\Uninstall-${PRODUCT_VERSION}.exe"
    ;; Whoever dares to change this back into: RMDir /r /REBOOTOK "$INSTDIR" will be KILLED!!!
    RMDir /REBOOTOK "$INSTDIR"

    ;;
    ;; Clean up the registry.
    ;;
    DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
    DeleteRegKey HKCU "${PRODUCT_DIR_REGKEY}"
    SetAutoClose true
SectionEnd

Function UninstallPrevious
    ReadRegStr $R0 HKCU \
               "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" \
               "UninstallString"
    ReadRegStr $R1 HKCU \
               "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" \
               "DisplayVersion"
    ${If} $R1 == "${PRODUCT_VERSION}"
        messageBox MB_OK|MB_ICONEXCLAMATION \
            "You already have the ${PRODUCT_NAME} v${PRODUCT_VERSION} installed. You should uninstall the ${PRODUCT_NAME} v${PRODUCT_VERSION} if you want to reinstall."
    ${EndIf}
    ${If} $R0 == ""
        ReadRegStr $R0 HKLM \
                   "Software\Microsoft\Windows\CurrentVersion\Uninstall\ReactOS Build Environment" \
                   "UninstallString"
        ReadRegStr $R1 HKLM \
                   "Software\Microsoft\Windows\CurrentVersion\Uninstall\ReactOS Build Environment" \
                   "DisplayVersion"
        ${If} $R0 == ""
            Return
        ${EndIf}
    ${EndIf}
    MessageBox MB_YESNO|MB_ICONQUESTION  \
               "A previous version of the ${PRODUCT_NAME} was found. You should uninstall it before installing this version.$\n$\nDo you want to do that now?" \
               IDNO UninstallPrevious_no \
               IDYES UninstallPrevious_yes
    Abort
    UninstallPrevious_yes:
        Var /global PREVIOUSINSTDIR
        Push $R0
        Call GetParent
        Pop $PREVIOUSINSTDIR
        Pop $R0
        ExecWait '$R0 _?=$PREVIOUSINSTDIR'
    UninstallPrevious_no:
FunctionEnd

Function GetParent
    Exch $R0
    Push $R1
    Push $R2
    Push $R3
    Push $R4

    StrCpy $R1 0
    StrLen $R2 $R0

    loop:
        IntOp $R1 $R1 + 1
        IntCmp $R1 $R2 get 0 get
        StrCpy $R3 $R0 1 -$R1
        StrCmp $R3 "\" get
        Goto loop

    get:
        StrCpy $R0 $R0 -$R1

        Pop $R3
        Pop $R2
        Pop $R1
        Exch $R0
FunctionEnd

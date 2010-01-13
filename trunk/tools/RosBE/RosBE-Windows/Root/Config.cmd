::
:: PROJECT:     RosBE - ReactOS Build Environment for Windows
:: LICENSE:     GNU General Public License v2. (see LICENSE.txt)
:: FILE:        Root/Config.cmd
:: PURPOSE:     A Basic Config.rbuild Creator for ReactOS.
:: COPYRIGHT:   Copyright 2009 Daniel Reimer <reimer.daniel@freenet.de>
::

@echo off
if not defined _ROSBE_DEBUG set _ROSBE_DEBUG=0
if %_ROSBE_DEBUG% == 1 (
    @echo on
)

setlocal enabledelayedexpansion
title ReactOS Build Configurator

if not exist "%APPDATA%\RosBE\RBUILDFLAGS.FLG" (
    echo -da> "%APPDATA%\RosBE\RBUILDFLAGS.FLG"
)

:: Receive the first Parameter and decide what to do.
if /i "%1" == "delete" (
    echo config.rbuild will be permanently deleted. All your settings will be gone.
    echo Continue?
    set /p YESNO="(yes), (no)"
    if /i "!YESNO!"=="yes" goto :CONT
    if /i "!YESNO!"=="y" goto :CONT
    goto :NOK

    :CONT
    if exist "config.rbuild" (
        del "config.rbuild"
        echo Main Configuration File was found and deleted.
    ) else (
        echo Main Configuration File was not found in ReactOS Source Tree.
    )

    if exist "%APPDATA%\RosBE\config.rbuild" (
        del "%APPDATA%\RosBE\config.rbuild"
        echo Working Configuration File was found and deleted.
    ) else (
        echo Working Configuration File was not found in ReactOS Source Tree.
    )

    if exist "%APPDATA%\RosBE\RBUILDFLAGS.FLG" (
        del "%APPDATA%\RosBE\RBUILDFLAGS.FLG"
        echo RBuild Flags File was found and deleted.
    ) else (
        echo RBuild Flags File was not found in ReactOS Source Tree.
    )

    goto :NOK
)

if /i "%1" == "update" (
    echo old config.rbuild will be deleted and will be updated with a recent,
    echo default one. You will need to reconfigure it to your wishes later.
    echo Continue?
    set /p YESNO="(yes), (no)"
    if /i "!YESNO!"=="yes" goto :CONT2
    if /i "!YESNO!"=="y" goto :CONT2
    goto :NOK

    :CONT2
    del "%_ROSBE_BASEDIR%\*.rbuild"
    del "config.rbuild"
    copy "config.template.rbuild" "%APPDATA%\RosBE\config.rbuild"
    echo Successfully Updated.
    goto :NOK
)

if /i "%1" == "rbuild" (
    echo Be verbose.
    echo Default is: no
    echo.
    for /f "usebackq tokens=* delims= " %%i in (`"echo %ROS_RBUILDFLAGS% | find "-v""`) do set VERBOSE_B=%%i
    if "!VERBOSE_B!" == "" (
        set VERBOSE_B=no
    ) else (
        set VERBOSE_B=yes
    )
    echo Right now: !VERBOSE_B!
    set /p VERBOSE="(yes), (no)"
    if "!VERBOSE!" == "" (
        set VERBOSE=!VERBOSE_B!
    )
    if "!VERBOSE!" == "yes" (
        set RBUILDFLAGS=-v
    )
    cls

    echo Delete generated files as soon as they are not needed anymore.
    echo Default is: no
    echo.
    for /f "usebackq tokens=* delims= " %%i in (`"echo %ROS_RBUILDFLAGS% | find "-c""`) do set CLEAN_B=%%i
    if "!CLEAN_B!" == "" (
        set CLEAN_B=no
    ) else (
        set CLEAN_B=yes
    )
    echo Right now: !CLEAN_B!
    set /p CLEAN="(yes), (no)"
    if "!CLEAN!" == "" (
        set CLEAN=!CLEAN_B!
    )
    if "!CLEAN!" == "yes" (
        set RBUILDFLAGS=!RBUILDFLAGS! -c
    )
    cls

    echo Disable/Enable automatic dependencies.
    echo Default is: yes
    echo.
    for /f "usebackq tokens=* delims= " %%i in (`"echo %ROS_RBUILDFLAGS% | find "-df""`) do set DEPENDS_B=%%i
    for /f "usebackq tokens=* delims= " %%i in (`"echo %ROS_RBUILDFLAGS% | find "-dd""`) do set DEPENDS_B2=%%i
    if not "!DEPENDS_B!" == "" (
        set DEPENDS_B=full
    ) else if not "!DEPENDS_B2!" == "" (
        set DEPENDS_B=no
    ) else (
        set DEPENDS_B=yes
    )
    echo Right now: !DEPENDS_B!
    set /p DEPENDS="(full), (yes), (no)"
    if "!DEPENDS!" == "" (
        set DEPENDS=!DEPENDS_B!
    )
    if "!DEPENDS!" == "full" (
        set RBUILDFLAGS=!RBUILDFLAGS! -df
    ) else if "!DEPENDS!" == "no" (
        set RBUILDFLAGS=!RBUILDFLAGS! -dd
    )
    cls

    echo Use precompiled headers.
    echo Default is: yes
    echo.
    for /f "usebackq tokens=* delims= " %%i in (`"echo %ROS_RBUILDFLAGS% | find "-hd""`) do set PRECHEADER_B=%%i
    if "!PRECHEADER_B!" == "" (
        set PRECHEADER_B=yes
    ) else (
        set PRECHEADER_B=no
    )
    echo Right now: !PRECHEADER_B!
    set /p PRECHEADER="(yes), (no)"
    if "!PRECHEADER!" == "" (
        set PRECHEADER=!PRECHEADER_B!
    )
    if "!PRECHEADER!" == "no" (
        set RBUILDFLAGS=!RBUILDFLAGS! -hd
    )
    cls

    echo Let make handle creation of install directories. Rbuild will not generate
    echo the directories.
    echo Default is: no
    echo.
    for /f "usebackq tokens=* delims= " %%i in (`"echo %ROS_RBUILDFLAGS% | find "-mi""`) do set MAKEDIR_B=%%i
    if "!MAKEDIR_B!" == "" (
        set MAKEDIR_B=no
    ) else (
        set MAKEDIR_B=yes
    )
    echo Right now: !MAKEDIR_B!
    set /p MAKEDIR="(yes), (no)"
    if "!MAKEDIR!" == "" (
        set MAKEDIR=!MAKEDIR_B!
    )
    if "!MAKEDIR!" == "yes" (
        set RBUILDFLAGS=!RBUILDFLAGS! -mi
    )
    cls

    echo Generate proxy makefiles in source tree instead of the output tree.
    echo Default is: no
    echo.
    for /f "usebackq tokens=* delims= " %%i in (`"echo %ROS_RBUILDFLAGS% | find "-ps""`) do set PROXYMAKE_B=%%i
    if "!PROXYMAKE_B!" == "" (
        set PROXYMAKE_B=no
    ) else (
        set PROXYMAKE_B=yes
    )
    echo Right now: !PROXYMAKE_B!
    set /p PROXYMAKE="(yes), (no)"
    if "!PROXYMAKE!" == "" (
        set PROXYMAKE=!PROXYMAKE_B!
    )
    if "!PROXYMAKE!" == "yes" (
        set RBUILDFLAGS=!RBUILDFLAGS! -ps
    )
    cls

    echo Use compilation units.
    echo Default is: yes
    echo.
    for /f "usebackq tokens=* delims= " %%i in (`"echo %ROS_RBUILDFLAGS% | find "-ud""`) do set COMPUNITS_B=%%i
    if "!COMPUNITS_B!" == "" (
        set COMPUNITS_B=yes
    ) else (
        set COMPUNITS_B=no
    )
    echo Right now: !COMPUNITS_B!
    set /p COMPUNITS="(yes), (no)"
    if "!COMPUNITS!" == "" (
        set COMPUNITS=!COMPUNITS_B!
    )
    if "!COMPUNITS!" == "no" (
        set RBUILDFLAGS=!RBUILDFLAGS! -ud
    )
    cls

    echo Input XML.
    echo Default is: no
    echo.
    for /f "usebackq tokens=* delims= " %%i in (`"echo %ROS_RBUILDFLAGS% | find "-r""`) do set XML_B=%%i
    if "!XML_B!" == "" (
        set XML_B=no
    ) else (
        set XML_B=yes
    )
    echo Right now: !XML_B!
    set /p XML="(yes), (no)"
    if "!XML!" == "" (
        set XML=!XML_B!
    )
    if "!XML!" == "yes" (
        set RBUILDFLAGS=!RBUILDFLAGS! -r
    )
    cls
    echo !RBUILDFLAGS! > "%APPDATA%\RosBE\RBUILDFLAGS.FLG"
    set ROS_RBUILDFLAGS=!RBUILDFLAGS!
    goto :NOK
)

if not "%1" == "" (
    echo Unknown parameter specified. Try 'help config'.
    goto :NOK
)

:: Check if config.rbuild already exists. If not, get a working copy.
if not exist "%APPDATA%\RosBE\config.rbuild" (
    copy "config.template.rbuild" "%APPDATA%\RosBE\config.rbuild"
)

:: Help prevent non-useful bug reports/questions.
echo.
echo *** Configurations other than release/debug are not useful for ***
echo *** posting bug reports, and generally not very useful for     ***
echo *** IRC/Forum discussion. Please refrain from doing so unless  ***
echo *** you are sure about what you are doing.                     ***
echo.

set /p YESNO="(yes), (no)"
if /i "%YESNO%"=="yes" goto :OK
if /i "%YESNO%"=="y" goto :OK
goto :NOK

:OK
:: Check if config.template.rbuild is newer than config.rbuild, if it is then
:: inform the user and offer an update.
if exist ".\config.rbuild" (
    chknewer.exe config.template.rbuild config.rbuild
    if !errorlevel! == 1 (
        echo.
        echo *** config.template.rbuild is newer than working config.rbuild ***
        echo *** The Editor cannot continue with this file. Do you wanna    ***
        echo *** update to the most recent one? You need to reset all your  ***
        echo *** previously made settings.                                  ***
        echo.
        set /p YESNO="(yes), (no)"
        if /i "!YESNO!"=="yes" goto :YES
        if /i "!YESNO!"=="y" goto :YES
        goto :NOK
        :YES
        del "%APPDATA%\RosBE\*.rbuild"
        del "config.rbuild"
        copy "config.template.rbuild" "%APPDATA%\RosBE\config.rbuild"
        goto :OK
    )
)


:: Start with reading settings from config.rbuild and let the user edit them.
echo Sub-Architecture to build for.
echo Default is: none
echo.
for /f "usebackq tokens=3" %%i in (`"type "%APPDATA%\RosBE\config.rbuild" | find "SARCH" | find "property name""`) do set SARCH=%%i
set SARCH=%SARCH:~7,-1%
echo Right now: %SARCH%
set /p SARCH_CH="(), (xbox)"
cls

echo Generate instructions for this CPU type. Specify one of:
echo.
echo Intel: i386, i486, i586, pentium, pentium-mmx, i686, pentiumpro, pentium2
echo        pentium3, pentium3m, pentium-m, pentium4, pentium4m, prescott, nocona
echo        core2
echo AMD:   k6, k6-2, k6-3, athlon, athlon-tbird, athlon-4, athlon-xp, athlon-mp, k8
echo        opteron, athlon64, athlon-fx, opteron-sse3, barcelona, geode
echo IDT:   winchip-c6, winchip2
echo VIA:   c3, c3-2
echo Default is: pentium
echo.
for /f "usebackq tokens=3" %%i in (`"type "%APPDATA%\RosBE\config.rbuild" | find "OARCH" | find "property name""`) do set OARCH=%%i
set OARCH=%OARCH:~7,-1%
echo Right now: %OARCH%
set /p OARCH_CH=
if "%OARCH_CH%" == "" (
    set OARCH_CH=%OARCH%
)
cls

echo Which CPU ReactOS should be optimized for. Specify one of the above CPUs or
echo generic. When this option is not used, GCC will optimize for the processor
echo specified by OARCH.
echo Default is: i686
echo.
for /f "usebackq tokens=3" %%i in (`"type "%APPDATA%\RosBE\config.rbuild" | find "TUNE" | find "property name""`) do set TUNE=%%i
set TUNE=%TUNE:~7,-1%
echo Right now: %TUNE%
set /p TUNE_CH=
if "%TUNE_CH%" == "" (
    set TUNE_CH=%TUNE%
)
cls

echo What level do you want ReactOS to be optimized at.
echo This setting does not work if GDB is set.
echo 0 = off
echo 1 = Normal compiling. Recommended. It is the default setting in
echo official release builds and debug builds.
echo warning : 2,3,4,5 is not tested on ReactOS. Change at own risk.
echo.
for /f "usebackq tokens=3" %%i in (`"type "%APPDATA%\RosBE\config.rbuild" | find "OPTIMIZE" | find "property name""`) do set OPTIMIZE=%%i
set OPTIMIZE=%OPTIMIZE:~7,-1%
echo Right now: %OPTIMIZE%
set /p OPTIMIZE_CH="(0), (1), (2), (3), (4), (5)"
if "%OPTIMIZE_CH%" == "" (
    set OPTIMIZE_CH=%OPTIMIZE%
)
cls

echo Whether to compile in the integrated kernel debugger.
echo Default is: 1
echo.
for /f "usebackq tokens=3" %%i in (`"type "%APPDATA%\RosBE\config.rbuild" | find "KDBG" | find "property name""`) do set KDBG=%%i
set KDBG=%KDBG:~7,-1%
echo Right now: %KDBG%
set /p KDBG_CH="(0), (1)"
if "%KDBG_CH%" == "" (
    set KDBG_CH=%KDBG%
)
cls

echo Whether to compile for debugging. No compiler optimizations will be
echo performed.
echo Default is: 1
echo.
for /f "usebackq tokens=3" %%i in (`"type "%APPDATA%\RosBE\config.rbuild" | find "DBG" | find "property name" | find /V "KDBG""`) do set DBG=%%i
set DBG=%DBG:~7,-1%
echo Right now: %DBG%
set /p DBG_CH="(0), (1)"
if "%DBG_CH%" == "" (
    set DBG_CH=%DBG%
)
cls

echo Whether to compile for debugging with GDB. If you don't use GDB,
echo don't enable this.
echo Default is: 0
echo.
for /f "usebackq tokens=3" %%i in (`"type "%APPDATA%\RosBE\config.rbuild" | find "GDB" | find "property name""`) do set GDB=%%i
set GDB=%GDB:~7,-1%
echo Right now: %GDB%
set /p GDB_CH="(0), (1)"
if "%GDB_CH%" == "" (
    set GDB_CH=%GDB%
)
cls

echo Whether to compile apps/libs with features covered software patents
echo or not. If you live in a country where software patents are
echo valid/apply, don't enable this (except they/you purchased a license
echo from the patent owner).
echo Default is: 0
echo.
for /f "usebackq tokens=3" %%i in (`"type "%APPDATA%\RosBE\config.rbuild" | find "NSWPAT" | find "property name""`) do set NSWPAT=%%i
set NSWPAT=%NSWPAT:~7,-1%
echo Right now: %NSWPAT%
set /p NSWPAT_CH="(0), (1)"
if "%NSWPAT_CH%" == "" (
    set NSWPAT_CH=%NSWPAT%
)
cls

echo Whether to compile with the KD protocol. This will disable support for
echo KDBG as well as rossym and symbol lookups, and allow WinDBG to connect
echo to ReactOS. This is currently not fully working, and requires kdcom
echo from Windows 2003 or TinyKRNL. Booting into debug mode with this flag
echo enabled will result in a failure to enter GUI mode. Do not enable
echo unless you know what you're doing.
echo Default is: 0
echo.
for /f "usebackq tokens=3" %%i in (`"type "%APPDATA%\RosBE\config.rbuild" | find "_WINKD_" | find "property name""`) do set WINKD=%%i
set WINKD=%WINKD:~7,-1%
echo Right now: %WINKD%
set /p WINKD_CH="(0), (1)"
if "%WINKD_CH%" == "" (
    set WINKD_CH=%WINKD%
)
cls

echo Whether to compile support for ELF files. Do not enable unless you know what
echo you're doing.
echo Default is: 0
echo.
for /f "usebackq tokens=3" %%i in (`"type "%APPDATA%\RosBE\config.rbuild" | find "_ELF_" | find "property name""`) do set ELF=%%i
set ELF=%ELF:~7,-1%
echo Right now: %ELF%
set /p ELF_CH="(0), (1)"
if "%ELF_CH%" == "" (
    set ELF_CH=%ELF%
)
cls

echo Whether to compile the multi processor versions for ntoskrnl and hal.
echo Default is: 1
echo.
for /f "usebackq tokens=3" %%i in (`"type "%APPDATA%\RosBE\config.rbuild" | find "BUILD_MP" | find "property name""`) do set BUILD_MP=%%i
set BUILD_MP=%BUILD_MP:~7,-1%
echo Right now: %BUILD_MP%
set /p BUILD_MP_CH="(0), (1)"
if "%BUILD_MP_CH%" == "" (
    set BUILD_MP_CH=%BUILD_MP%
)
cls

:: Generate a config.rbuild, copy it to the Source Tree and delete temp files.
echo ^<?xml version="1.0"?^>>%TEMP%\config.tmp
echo ^<!DOCTYPE group SYSTEM "tools/rbuild/project.dtd"^>>%TEMP%\config.tmp
echo ^<group^>>%TEMP%\config.tmp
echo ^<property name="SARCH" value="%SARCH_CH%" /^>>>%TEMP%\config.tmp
echo ^<property name="OARCH" value="%OARCH_CH%" /^>>>%TEMP%\config.tmp
echo ^<property name="TUNE" value="%TUNE_CH%" /^>>>%TEMP%\config.tmp
echo ^<property name="OPTIMIZE" value="%OPTIMIZE_CH%" /^>>>%TEMP%\config.tmp
echo ^<property name="KDBG" value="%KDBG_CH%" /^>>>%TEMP%\config.tmp
echo ^<property name="DBG" value="%DBG_CH%" /^>>>%TEMP%\config.tmp
echo ^<property name="GDB" value="%GDB_CH%" /^>>>%TEMP%\config.tmp
echo ^<property name="NSWPAT" value="%NSWPAT_CH%" /^>>>%TEMP%\config.tmp
echo ^<property name="_WINKD_" value="%WINKD_CH%" /^>>>%TEMP%\config.tmp
echo ^<property name="_ELF_" value="%ELF_CH%" /^>>>%TEMP%\config.tmp
echo ^<property name="BUILD_MP" value="%BUILD_MP_CH%" /^>>>%TEMP%\config.tmp
echo ^</group^>>>%TEMP%\config.tmp

copy "%TEMP%\config.tmp" "%APPDATA%\RosBE\config.rbuild" >NUL
del %TEMP%\config.tmp
copy "%APPDATA%\RosBE\config.rbuild" "config.rbuild" >NUL

:NOK
title ReactOS Build Environment %_ROSBE_VERSION%
endlocal & set ROS_RBUILDFLAGS=%RBUILDFLAGS%

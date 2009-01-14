::
:: PROJECT:     RosBE - ReactOS Build Environment for Windows
:: LICENSE:     GNU General Public License v2. (see LICENSE.txt)
:: FILE:        Root/Build.cmd
:: PURPOSE:     Perform the build of ReactOS.
:: COPYRIGHT:   Copyright 2007 Daniel Reimer <reimer.daniel@freenet.de>
::                             Colin Finck <mail@colinfinck.de>
::                             Peter Ward <dralnix@gmail.com>
::
::
@echo off
if not defined _ROSBE_DEBUG set _ROSBE_DEBUG=0
if %_ROSBE_DEBUG% == 1 (
    @echo on
)

::
:: Check if config.template.rbuild is newer than config.rbuild, if it is then
:: abort the build and inform the user.
::
setlocal enabledelayedexpansion
if exist .\config.rbuild (
    "%_ROSBE_BASEDIR%\Tools\chknewer.exe" .\config.template.rbuild .\config.rbuild
    if !errorlevel! == 1 (
        echo.
        echo *** config.template.rbuild is newer than config.rbuild ***
        echo *** aborting build. Please check for changes and       ***
        echo *** update your config.rbuild.                         ***
        echo.
        endlocal
        goto :EOC
    )
)
endlocal

::
:: Check if strip, no Debug Symbols or ccache are being used and set the appropriate options.
::
if "%_ROSBE_NOSTRIP%" == "1" (
    set ROS_BUILDNOSTRIP=yes
) else (
    set ROS_BUILDNOSTRIP=no
)

if "%_ROSBE_STRIP%" == "1" (
    set ROS_LEAN_AND_MEAN=yes
) else (
    set ROS_LEAN_AND_MEAN=no
)

:: Small Security Check to prevent useless apps.
if "%ROS_LEAN_AND_MEAN%" == "yes" (
    if "%ROS_BUILDNOSTRIP%" == "yes" (
        cls
        echo Selecting Stripping and removing Debug Symbols together will most likely cause useless apps. Please deselect one of them.
        goto :EOC
    )
)

if "%_ROSBE_USECCACHE%" == "1" (
    set CCACHE_DIR=%APPDATA%\RosBE\.ccache
    set _ROSBE_CCACHE=ccache 
)

:: Target defaults to host(i386)

set HOST_CC=%_ROSBE_CCACHE%gcc
set HOST_CPP=%_ROSBE_CCACHE%g++

set TARGET_CC=%_ROSBE_CCACHE%gcc
set TARGET_CPP=%_ROSBE_CCACHE%g++

if not "%ROS_ARCH%" == "" (
    set TARGET_CC=%_ROSBE_CCACHE%%ROS_ARCH%-pc-mingw32-gcc
    set TARGET_CPP=%_ROSBE_CCACHE%%ROS_ARCH%-pc-mingw32-g++
)

::
:: Check if the user has chosen to use a different object or output path
:: and set it accordingly.
::
if defined _ROSBE_OBJPATH (
    if not exist "%_ROSBE_OBJPATH%\." (
        echo WARNING: The Object-Path specified doesn't seem to exist. Creating...
    )
    set ROS_INTERMEDIATE=%_ROSBE_OBJPATH%
)
if defined _ROSBE_OUTPATH (
    if not exist "%_ROSBE_OUTPATH%\." (
        echo WARNING: The Output-Path specified doesn't seem to exist. Creating...
    )
    set ROS_OUTPUT=%_ROSBE_OUTPATH%
    set ROS_TEMPORARY=%_ROSBE_OUTPATH%
    )
)

::
:: Get the current date and time for use in in our build log's file name.
::
call "%_ROSBE_BASEDIR%\TimeDate.cmd"

::
:: Check if writing logs is enabled, if so check if our log directory
:: exists, if it doesn't, create it.
::
if %_ROSBE_WRITELOG% == 1 (
    if not exist "%_ROSBE_LOGDIR%\." (
        mkdir "%_ROSBE_LOGDIR%" 1> NUL 2> NUL
    )
)

::
:: Check if we are using -j or not.
::
if "%1" == "multi" (
    if not "%2" == "" (
        title 'makex %2' parallel build started: %TIMERAW%   %ROS_ARCH%
    ) else (
        title 'makex' parallel build started: %TIMERAW%   %ROS_ARCH%
    )
    call :BUILDMULTI %*
) else (
    if not "%1" == "" (
        title 'make %1' build started: %TIMERAW%   %ROS_ARCH%
    ) else (
        title 'make' build started: %TIMERAW%   %ROS_ARCH%
    )
    call :BUILD %*
)
goto :EOC

:BUILD

if %_ROSBE_SHOWTIME% == 1 (
    if %_ROSBE_WRITELOG% == 1 (
        "%_ROSBE_BASEDIR%\Tools\buildtime.exe" "%_ROSBE_MINGWMAKE%" %* 2>&1 | "%_ROSBE_BASEDIR%\Tools\tee.exe" "%_ROSBE_LOGDIR%\BuildLog-%_ROSBE_GCCVERSION%-%DATENAME%-%TIMENAME%.txt"
    ) else (
        "%_ROSBE_BASEDIR%\Tools\buildtime.exe" "%_ROSBE_MINGWMAKE%" %*
    )
) else (
    if %_ROSBE_WRITELOG% == 1 (
        "%_ROSBE_MINGWMAKE%" %* 2>&1 | "%_ROSBE_BASEDIR%\Tools\tee.exe" "%_ROSBE_LOGDIR%\BuildLog-%_ROSBE_GCCVERSION%-%DATENAME%-%TIMENAME%.txt"
    ) else (
        "%_ROSBE_MINGWMAKE%" %*
    )
)
goto :EOF

::
:: Get the number of CPUs in the system so we know how many jobs to execute.
:: To modify the number used alter the options used with cpucount:
:: No Option - Number of CPUs.
:: -x1       - Number of CPUs, plus 1.
:: -x2       - Number of CPUs, doubled.
:: -a        - Determine the cpu count based on the inherited process affinity mask.
::

:BUILDMULTI

for /f "usebackq" %%i in (`"%_ROSBE_BASEDIR%\Tools\cpucount.exe" -x1`) do set CPUCOUNT=%%i

if %_ROSBE_SHOWTIME% == 1 (
    if %_ROSBE_WRITELOG% == 1 (
        "%_ROSBE_BASEDIR%\Tools\buildtime.exe" "%_ROSBE_MINGWMAKE%" -j %CPUCOUNT% %2 %3 %4 %5 %6 %7 %8 %9 2>&1 | "%_ROSBE_BASEDIR%\Tools\tee.exe" "%_ROSBE_LOGDIR%\BuildLog-%_ROSBE_GCCVERSION%-%DATENAME%-%TIMENAME%.txt"
    ) else (
        "%_ROSBE_BASEDIR%\Tools\buildtime.exe" "%_ROSBE_MINGWMAKE%" -j %CPUCOUNT% %2 %3 %4 %5 %6 %7 %8 %9
    )
) else (
    if %_ROSBE_WRITELOG% == 1 (
        "%_ROSBE_MINGWMAKE%" -j %CPUCOUNT% %2 %3 %4 %5 %6 %7 %8 %9 2>&1 | "%_ROSBE_BASEDIR%\Tools\tee.exe" "%_ROSBE_LOGDIR%\BuildLog-%_ROSBE_GCCVERSION%-%DATENAME%-%TIMENAME%.txt"
    ) else (
        "%_ROSBE_MINGWMAKE%" -j %CPUCOUNT% %2 %3 %4 %5 %6 %7 %8 %9
    )
)
goto :EOF

::
:: Highlight the fact that building has ended.
::

:EOC

"%_ROSBE_BASEDIR%\Tools\flash.exe"

if defined _ROSBE_VERSION (
    title ReactOS Build Environment %_ROSBE_VERSION%
)

::
:: Unload all used Vars.
::
set ROS_BUILDNOSTRIP=
set ROS_LEAN_AND_MEAN=
set HOST_CC=
set HOST_CPP=
set TARGET_CC=
set TARGET_CPP=
set ROS_INTERMEDIATE=
set ROS_OUTPUT=
set ROS_TEMPORARY=
set CPUCOUNT=
set CCACHE_DIR=
set _ROSBE_CCACHE=

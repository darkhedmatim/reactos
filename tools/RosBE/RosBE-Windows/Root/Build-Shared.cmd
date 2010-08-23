::
:: PROJECT:     RosBE - ReactOS Build Environment for Windows
:: LICENSE:     GNU General Public License v2. (see LICENSE.txt)
:: FILE:        Root/Build-Shared.cmd
:: PURPOSE:     Perform the build of ReactOS - Shared commands.
:: COPYRIGHT:   Copyright 2010 Daniel Reimer <reimer.daniel@freenet.de>
::                             Colin Finck <colin@reactos.org>
::                             Peter Ward <dralnix@gmail.com>
::

@echo off
if not defined _ROSBE_DEBUG set _ROSBE_DEBUG=0
if %_ROSBE_DEBUG% == 1 (
    @echo on
)

:: Check if config.template.rbuild is newer than config.rbuild, if it is then
:: abort the build and inform the user.
if exist .\config.rbuild (
    chknewer.exe config.template.rbuild config.rbuild
    if !errorlevel! == 1 (
        echo.
        echo *** config.template.rbuild is newer than config.rbuild ***
        echo *** aborting build. Please check for changes and       ***
        echo *** update your config.rbuild.                         ***
        echo.
        goto :EOC
    )
)

if "%_ROSBE_USECCACHE%" == "1" (
    if not "%_ROSBE_CACHESIZE%" == "" (
        ccache -M %_ROSBE_CACHESIZE%G
    ) else (
        ccache -M 8G
    )
    set _ROSBE_CCACHE=ccache 
) else (
    set _ROSBE_CCACHE=
)

set HOST_CC=%_ROSBE_CCACHE%gcc
set HOST_CPP=%_ROSBE_CCACHE%g++
set TARGET_CC=%_ROSBE_CCACHE%%_ROSBE_PREFIX%gcc
set TARGET_CPP=%_ROSBE_CCACHE%%_ROSBE_PREFIX%g++

:: Get the current date and time for use in in our build log's file name.
call "%_ROSBE_BASEDIR%\TimeDate.cmd"

title '%TITLE_COMMAND%' build started: %TIMERAW%   (%ROS_ARCH%)

:: Do the actual building
if %_ROSBE_SHOWTIME% == 1 (
    set BUILDTIME_COMMAND=buildtime.exe
) else (
    set BUILDTIME_COMMAND=
)

if %_ROSBE_WRITELOG% == 1 (
    if not exist "%_ROSBE_LOGDIR%\." (
        mkdir "%_ROSBE_LOGDIR%" 1> NUL 2> NUL
    )
    %BUILDTIME_COMMAND% mingw32-make.exe -j %MAKE_JOBS% %* 2>&1 | tee.exe "%_ROSBE_LOGDIR%\BuildLog-%ROS_ARCH%-%datename%-%timename%.txt"
) else (
    %BUILDTIME_COMMAND% mingw32-make.exe -j %MAKE_JOBS% %*
)

:EOC
:: Highlight the fact that building has ended.

if !errorlevel! GEQ 1 (
    playwav.exe error.wav
) else (
    playwav.exe notification.wav
)

flash.exe

title ReactOS Build Environment %_ROSBE_VERSION%

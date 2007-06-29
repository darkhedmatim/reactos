::
:: PROJECT:     RosBE - ReactOS Build Environment for Windows
:: LICENSE:     GPL - See COPYING in the top level directory
:: FILE:        Root/Build-Shared.cmd
:: PURPOSE:     Shared actions for "Build.cmd" and "Build-Multi.cmd".
:: COPYRIGHT:   Copyright 2007 Daniel Reimer <reimer.daniel@freenet.de>
::                             Colin Finck <mail@colinfinck.de>
::                             Peter Ward <dralnix@gmail.com>
::
::
@echo off

title Building...

::
:: Take over the 2nd parameter of Build.cmd, which enables/disables stripping
::
if "%1" == "strip" (
    set ROS_LEAN_AND_MEAN = yes
    goto :Build
)
if "%1"  == "" (
    set ROS_LEAN_AND_MEAN = no
    goto :Build
)

:Build

::
:: Check if config.template.rbuild is newer than config.rbuild, if it is then
:: abort the build and inform the user.
::
if exist "%_ROSSOURCEDIR%\config.rbuild" (
    %ROSBEBASEDIR%\Tools\test.exe "%_ROSSOURCEDIR%\config.template.rbuild" -nt "%_ROSSOURCEDIR%\config.rbuild"
    if not errorlevel 1 (
        echo.
        echo *** config.template.rbuild is newer than config.rbuild ***
        echo *** aborting build. Please check for changes and       ***
        echo *** update your config.rbuild.                         ***
        echo.
        goto :EOB
    )
)

::
:: Get the current date and time for use in in our build log's file name.
::
call "%ROSBEBASEDIR%\TimeDate.cmd"

::
:: Now use mingw32-make to build ReactOS, passing along options, if any.
::
call "%ROSBEBASEDIR%\Tools\buildtime.exe" %MAKE_COMMAND% 2>&1 | "%ROSBEBASEDIR%\Tools\tee.exe" "%_ROSBELOGDIR%\BuildLog-%_MINGWVERSION%-%DATENAME%-%TIMENAME%.txt"

::
:: Highlight the fact that building has ended.
::
call "%ROSBEBASEDIR%\Tools\flash.exe"

goto :EOB

:EOB
title ReactOS Build Environment 0.3.7

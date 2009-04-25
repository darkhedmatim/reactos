::
:: PROJECT:     RosBE - ReactOS Build Environment for Windows
:: LICENSE:     GNU General Public License v2. (see LICENSE.txt)
:: FILE:        Root/charch.cmd
:: PURPOSE:     Tool to change the current Arch to build ROS for in RosBE.
:: COPYRIGHT:   Copyright 2009 Daniel Reimer <reimer.daniel@freenet.de>
::
@echo off
if not defined _ROSBE_DEBUG set _ROSBE_DEBUG=0
if %_ROSBE_DEBUG% == 1 (
    @echo on
)

title Change the Architecture to build for...

::
:: Parse the command line arguments.
:: ROSBE_ARCH: Default is i386, can be set to amd64, ppc or arm.
::

if "%1" == "" (
    call :INTERACTIVE
) else (
    set _1=%1
)
if /i "%_1%" == "i386" (
    set _ROSBE_ARCH=
) else (
    set _ROSBE_ARCH=%_1%
)
goto :EOA

::
:: Refresh all needed Params by recalling the main Path setting CMD File.
::

:EOA

call "%_ROSBE_BASEDIR%\rosbe-gcc-env.cmd"
"%_ROSBE_BASEDIR%\version.cmd"
goto :EOC

::
:: If Parameters were set, parse them, if not, ask the user to add them.
::

:INTERACTIVE

set /p _1="Please enter a Architecture you want to build ReactOS for: "
if "%_1%" == "" (
    echo ERROR: You must enter a Architecture.
    goto :EOC
)
goto :EOF

:EOC

if defined _ROSBE_VERSION (
    title ReactOS Build Environment %_ROSBE_VERSION%
)

::
:: Unload all used Vars.
::
set _1=

::
:: PROJECT:     RosBE - ReactOS Build Environment for Windows
:: LICENSE:     GNU General Public License v2. (see LICENSE.txt)
:: FILE:        Root/sSVN.cmd
:: PURPOSE:     Integrated SVN Client.
:: COPYRIGHT:   Copyright 2007 Daniel Reimer <reimer.daniel@freenet.de>
::
::
@echo off
if not defined _ROSBE_DEBUG set _ROSBE_DEBUG=0
if %_ROSBE_DEBUG% == 1 (
    @echo on
)

::
:: Receive the first parameter and decide what to do.
::
if "%1" == "" (
    echo No parameter specified. Try 'help [COMMAND]'.
    goto :EOC
)

::
:: These two are directly parsed to svn.
::
if /i "%1" == "update" (
    title SVN Updating...
    echo This might take a while, so please be patient.
    echo.
    if not "%2" == "" (
        "%_ROSBE_BASEDIR%\Tools\svn.exe" update -r %2
    ) else (
        "%_ROSBE_BASEDIR%\Tools\svn.exe" update
    )
    goto :EOC
)
if /i "%1" == "cleanup" (
    title SVN Cleaning...
    echo This might take a while, so please be patient.
    echo.
    "%_ROSBE_BASEDIR%\Tools\svn.exe" cleanup
    goto :EOC
)

::
:: Check if the folder is empty. If not, output an error.
::
if /i "%1" == "create" (
    title SVN Creating...
    if exist ".svn\." (
        echo ERROR: Folder already contains a repository.
        goto :EOC
    )
    dir /b 2>nul | findstr "." >nul
    if errorlevel 1 (
        "%_ROSBE_BASEDIR%\Tools\svn.exe" checkout svn://svn.reactos.org/reactos/trunk/reactos .
    ) else (
        echo ERROR: Folder is not empty. Continuing is dangerous and can cause errors. ABORTED
    )
    goto :EOC
)

::
:: Output the revision of the local and online trees and tell the user if
:: its up to date or not.
::
if /i "%1" == "status" (
    title SVN Status
    echo This might take a while, so please be patient.
    echo.
    for /f "usebackq tokens=2" %%i in (`""%_ROSBE_BASEDIR%\Tools\svn.exe" info | find "Revision:""`) do set OFFSVN=%%i
    for /f "usebackq tokens=2" %%j in (`""%_ROSBE_BASEDIR%\Tools\svn.exe" info svn://svn.reactos.org/reactos/trunk/reactos | find "Revision:""`) do set ONSVN=%%j
    call :UP
    goto :EOC
)

if not "%1" == "" (
    echo Unknown parameter specified. Try 'help ssvn'.
    goto :EOC
)

:UP

echo Local Revision: %OFFSVN%
echo Online HEAD Revision: %ONSVN%
echo.
if %OFFSVN% lss %ONSVN% (
    echo Your tree is not up to date. Do you want to update it?
    goto :UP2
)
if %OFFSVN% equ %ONSVN% (
    echo Your tree is up to date.
    goto :EOF
)

:UP2

set /p UP="Please enter 'yes' or 'no': "
if /i "%UP%"=="yes" "%_ROSBE_BASEDIR%\ssvn" update
if /i "%UP%"=="y" "%_ROSBE_BASEDIR%\ssvn" update
goto :EOF

:EOC

if defined _ROSBE_VERSION (
    title ReactOS Build Environment %_ROSBE_VERSION%
)

::
:: Unload all used Vars.
::
set OFFSVN=
set ONSVN=
set UP=

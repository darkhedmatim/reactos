@echo off

REM This is needed so as to avoid static expansion of environment variables
REM inside if (...) conditionals.
REM See http://stackoverflow.com/questions/305605/weird-scope-issue-in-bat-file
REM for more explanation.
REM Precisely needed for configuring Visual Studio Environment.
setlocal enabledelayedexpansion

REM Does the user need help?
if /I "%1" == "help" goto help
if /I "%1" == "/?" (
:help
    echo Help for configure script
    echo Syntax: path\to\source\configure.cmd [script-options] [Cmake-options]
    echo Available script-options: Codeblocks, Eclipse, Makefiles, clang, VSSolution, RTC
    echo Cmake-options: -DVARIABLE:TYPE=VALUE
    endlocal
    exit /b
)

REM Special case %1 = arm_hosttools %2 = vcvarsall.bat %3 = %CMAKE_GENERATOR%
if /I "%1" == "arm_hosttools" (
    echo Configuring x86 host tools for ARM cross build

    REM This launches %VSINSTALLDIR%VS\vcvarsall.bat
    call %2 x86

    REM Configure host tools for x86.
    cmake -G %3 -DARCH:STRING=i386 %~dp0
    exit
)

REM Get the source root directory
set REACTOS_SOURCE_DIR=%~dp0

REM Set default generator
set CMAKE_GENERATOR="Ninja"
set CMAKE_GENERATOR_HOST=!CMAKE_GENERATOR!

REM Detect presence of cmake
cmd /c cmake --version 2>&1 | find "cmake version" > NUL || goto cmake_notfound

REM Detect build environment (MinGW, VS, WDK, ...)
if defined ROS_ARCH (
    echo Detected RosBE for %ROS_ARCH%
    set BUILD_ENVIRONMENT=MinGW
    set ARCH=%ROS_ARCH%
    set MINGW_TOOCHAIN_FILE=toolchain-gcc.cmake

) else if defined VCINSTALLDIR (
    REM VS command prompt does not put this in environment vars
    cl 2>&1 | find "x86" > NUL && set ARCH=i386
    cl 2>&1 | find "x64" > NUL && set ARCH=amd64
    cl 2>&1 | find "ARM" > NUL && set ARCH=arm
    cl 2>&1 | find "15.00." > NUL && set VS_VERSION=9
    cl 2>&1 | find "16.00." > NUL && set VS_VERSION=10
    cl 2>&1 | find "17.00." > NUL && set VS_VERSION=11
    cl 2>&1 | find "18.00." > NUL && set VS_VERSION=12
    cl 2>&1 | find "19.00." > NUL && set VS_VERSION=14
    if not defined VS_VERSION (
        echo Error: Visual Studio version too old or version detection failed.
        endlocal
        exit /b
    )
    set BUILD_ENVIRONMENT=VS
    set VS_SOLUTION=0
    set VS_RUNTIME_CHECKS=0
    echo Detected Visual Studio Environment !BUILD_ENVIRONMENT!!VS_VERSION!-!ARCH!
) else (
    echo Error: Unable to detect build environment. Configure script failure.
    endlocal
    exit /b
)

REM Checkpoint
if not defined ARCH (
    echo Unknown build architecture
    endlocal
    exit /b
)

set NEW_STYLE_BUILD=1

REM Parse command line parameters
:repeat
    if /I "%1" == "-DNEW_STYLE_BUILD" (
        set NEW_STYLE_BUILD=%2
    ) else if "%BUILD_ENVIRONMENT%" == "MinGW" (
        if /I "%1" == "Codeblocks" (
            set CMAKE_GENERATOR="CodeBlocks - MinGW Makefiles"
        ) else if /I "%1" == "Eclipse" (
            set CMAKE_GENERATOR="Eclipse CDT4 - MinGW Makefiles"
        ) else if /I "%1" == "Makefiles" (
            set CMAKE_GENERATOR="MinGW Makefiles"
        ) else if /I "%1" == "clang" (
            set MINGW_TOOCHAIN_FILE=toolchain-clang.cmake
        ) else if /I "%1" == "VSSolution" (
            echo. && echo Error: Creation of VS Solution files is not supported in a MinGW environment.
            echo Please run this command in a [Developer] Command Prompt for Visual Studio.
            endlocal
            exit /b
        ) else if /I "%1" == "RTC" (
            echo. && echo 	Warning: RTC switch is ignored outside of a Visual Studio environment. && echo.
        ) else (
            goto continue
        )
    ) else (
        if /I "%1" == "CodeBlocks" (
            set CMAKE_GENERATOR="CodeBlocks - NMake Makefiles"
        ) else if /I "%1" == "Eclipse" (
            set CMAKE_GENERATOR="Eclipse CDT4 - NMake Makefiles"
        ) else if /I "%1" == "Makefiles" (
            set CMAKE_GENERATOR="NMake Makefiles"
        ) else if /I "%1" == "VSSolution" (
            set VS_SOLUTION=1
            if "!VS_VERSION!" == "9" (
                if "!ARCH!" == "amd64" (
                    set CMAKE_GENERATOR="Visual Studio 9 2008 Win64"
                ) else (
                    set CMAKE_GENERATOR="Visual Studio 9 2008"
                )
            ) else if "!VS_VERSION!" == "10" (
                if "!ARCH!" == "amd64" (
                    set CMAKE_GENERATOR="Visual Studio 10 Win64"
                ) else (
                    set CMAKE_GENERATOR="Visual Studio 10"
                )
            ) else if "!VS_VERSION!" == "11" (
                if "!ARCH!" == "amd64" (
                    set CMAKE_GENERATOR="Visual Studio 11 Win64"
                ) else if "!ARCH!" == "arm" (
                    set CMAKE_GENERATOR="Visual Studio 11 ARM"
                    set CMAKE_GENERATOR_HOST="Visual Studio 11"
                ) else (
                    set CMAKE_GENERATOR="Visual Studio 11"
                )
            ) else if "!VS_VERSION!" == "12" (
                if "!ARCH!" == "amd64" (
                    set CMAKE_GENERATOR="Visual Studio 12 Win64"
                ) else if "!ARCH!" == "arm" (
                    set CMAKE_GENERATOR="Visual Studio 12 ARM"
                    set CMAKE_GENERATOR_HOST="Visual Studio 12"
                ) else (
                    set CMAKE_GENERATOR="Visual Studio 12"
                )
            ) else if "!VS_VERSION!" == "14" (
                if "!ARCH!" == "amd64" (
                    set CMAKE_GENERATOR="Visual Studio 14 Win64"
                ) else if "!ARCH!" == "arm" (
                    set CMAKE_GENERATOR="Visual Studio 14 ARM"
                    set CMAKE_GENERATOR_HOST="Visual Studio 14"
                ) else (
                    set CMAKE_GENERATOR="Visual Studio 14"
                )
            )
        ) else if /I "%1" == "RTC" (
            echo Runtime checks enabled
            set VS_RUNTIME_CHECKS=1
        ) else (
            goto continue
        )
    )

    REM Go to next parameter
    SHIFT
    goto repeat
:continue

REM Inform the user about the default build
if "!CMAKE_GENERATOR!" == "Ninja" (
    echo This script defaults to Ninja. Type "configure help" for alternative options.
)

REM Create directories
set REACTOS_OUTPUT_PATH=output-%BUILD_ENVIRONMENT%-%ARCH%

if "%VS_SOLUTION%" == "1" (
    set REACTOS_OUTPUT_PATH=%REACTOS_OUTPUT_PATH%-sln
)

if "%REACTOS_SOURCE_DIR%" == "%CD%\" (
    set CD_SAME_AS_SOURCE=1
    echo Creating directories in %REACTOS_OUTPUT_PATH%

    if not exist %REACTOS_OUTPUT_PATH% (
        mkdir %REACTOS_OUTPUT_PATH%
    )
    cd %REACTOS_OUTPUT_PATH%
)

if "%VS_SOLUTION%" == "1" (

    if exist build.ninja (
        echo. && echo Error: This directory has already been configured for ninja.
        echo An output folder configured for ninja can't be reconfigured for VSSolution.
        echo Use an empty folder or delete the contents of this folder, then try again.
        endlocal
        exit /b
    )
) else if exist REACTOS.sln (
    echo. && echo Error: This directory has already been configured for Visual Studio.
    echo An output folder configured for VSSolution can't be reconfigured for ninja.
    echo Use an empty folder or delete the contents of this folder, then try again. && echo.
    endlocal
    exit /b
)

if "%NEW_STYLE_BUILD%"=="0" (

    if not exist host-tools (
        mkdir host-tools
    )

    if not exist reactos (
        mkdir reactos
    )

    echo Preparing host tools...
    cd host-tools
    if EXIST CMakeCache.txt (
        del CMakeCache.txt /q
    )

    set REACTOS_BUILD_TOOLS_DIR=!CD!

    REM Use x86 for ARM host tools
    if "%ARCH%" == "arm" (
        REM Launch new script instance for x86 host tools configuration
        start "Preparing host tools for ARM cross build..." /I /B /WAIT %~dp0configure.cmd arm_hosttools "%VSINSTALLDIR%VC\vcvarsall.bat" %CMAKE_GENERATOR_HOST%
    ) else (
        cmake -G %CMAKE_GENERATOR% -DARCH:STRING=%ARCH% "%REACTOS_SOURCE_DIR%"
    )

    cd..

)

echo Preparing reactos...

if "%NEW_STYLE_BUILD%"=="0" (
    cd reactos
)

if EXIST CMakeCache.txt (
    del CMakeCache.txt /q
    del host-tools\CMakeCache.txt /q
)

if "%NEW_STYLE_BUILD%"=="0" (
    set BUILD_TOOLS_FLAG=-DREACTOS_BUILD_TOOLS_DIR:PATH="%REACTOS_BUILD_TOOLS_DIR%"
)

if "%BUILD_ENVIRONMENT%" == "MinGW" (
    cmake -G %CMAKE_GENERATOR% -DENABLE_CCACHE:BOOL=0 -DCMAKE_TOOLCHAIN_FILE:FILEPATH=%MINGW_TOOCHAIN_FILE% -DARCH:STRING=%ARCH% %BUILD_TOOLS_FLAG% %* "%REACTOS_SOURCE_DIR%"
) else (
    cmake -G %CMAKE_GENERATOR% -DCMAKE_TOOLCHAIN_FILE:FILEPATH=toolchain-msvc.cmake -DARCH:STRING=%ARCH% %BUILD_TOOLS_FLAG% -DRUNTIME_CHECKS:BOOL=%VS_RUNTIME_CHECKS% %* "%REACTOS_SOURCE_DIR%"
)

if "%NEW_STYLE_BUILD%"=="0" (
    cd..
)

if "%CD_SAME_AS_SOURCE%" == "1" (
    echo Configure script complete^^! Execute appropriate build commands ^(ex: ninja, make, nmake, etc...^) from %REACTOS_OUTPUT_PATH%.
) else (
    echo Configure script complete^^! Execute appropriate build commands ^(ex: ninja, make, nmake, etc...^).
)
endlocal
exit /b

:cmake_notfound
echo Unable to find cmake, if it is installed, check your PATH variable.
endlocal
exit /b

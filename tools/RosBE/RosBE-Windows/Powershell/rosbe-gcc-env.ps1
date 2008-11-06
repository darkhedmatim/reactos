#
# PROJECT:     RosBE - ReactOS Build Environment for Windows
# LICENSE:     GNU General Public License v2. (see LICENSE.txt)
# FILE:        Root/rosbe-gcc-env.ps1
# PURPOSE:     Set up the GCC 4.x.x build environment.
# COPYRIGHT:   Copyright 2008 Daniel Reimer <reimer.daniel@freenet.de>
#
#

#
# Check if we are running within the RosBE, and if not initialize GCC for the
# current directory.
#
if ($_ROSBE_HOST_MINGWPATH -eq $null) {
    $_ROSBE_MODE = "MinGW"
    $_ROSBE_HOST_MINGWPATH = "$pwd"
    $_ROSBE_ORIGINALPATH = "$PATH"
}


#
# Set the Arch Variables
#

$ENV:ROS_ARCH = $null
$ENV:ROS_PREFIX = $null

if ($_ROSBE_ARCH -eq 1) {
    $ENV:ROS_ARCH = "arm"
    $ENV:ROS_PREFIX = "arm-pc-mingw32"
    $_ROSBE_TARGET_MINGWPATH = "$_ROSBE_BASEDIR\arm"
}
if ($_ROSBE_ARCH -eq 2) {
    $ENV:ROS_ARCH = "ppc"
    $ENV:ROS_PREFIX = "ppc-pc-mingw32"
    $_ROSBE_TARGET_MINGWPATH = "$_ROSBE_BASEDIR\ppc"
}
if ($_ROSBE_ARCH -eq 3) {
    $ENV:ROS_ARCH = "amd64"
    $ENV:ROS_PREFIX = "x86_64-pc-mingw32"
    $_ROSBE_TARGET_MINGWPATH = "$_ROSBE_BASEDIR\x86_64"
}

#
# Set up the GCC 4.x.x build environment.
#
$ENV:PATH = "$_ROSBE_HOST_MINGWPATH\bin;$_ROSBE_TARGET_MINGWPATH\bin;$_ROSBE_ORIGINALPATH"
$global:_ROSBE_MINGWMAKE = "$_ROSBE_HOST_MINGWPATH\bin\mingw32-make.exe"
if ($ENV:ROS_ARCH -ne $null) {
    & "$ENV:ROS_PREFIX-gcc" -v 2> v.txt
    $global:_ROSBE_GCCVERSION = (select-string -path .\v.txt "gcc version") -replace ".*version ((\d|\.)+).*",'$1'
    $global:_ROSBE_GCCTARGET = (select-string -path .\v.txt "target=") -replace ".*--target=(.+?)\b.*",'$1'
    rm v.txt
} else {
    gcc -v 2> v.txt
    $global:_ROSBE_GCCVERSION = (select-string -path .\v.txt "gcc version") -replace ".*version ((\d|\.)+).*",'$1'
    $global:_ROSBE_GCCTARGET = (select-string -path .\v.txt "target=") -replace ".*--target=(.+?)\b.*",'$1'
    rm v.txt
}
gcc -v 2> v.txt
$global:_ROSBE_HOST_GCCVERSION = (select-string -path .\v.txt "gcc version") -replace ".*version ((\d|\.)+).*",'$1'
$global:_ROSBE_HOST_GCCTARGET = (select-string -path .\v.txt "target=") -replace ".*--target=(.+?)\b.*",'$1'
rm v.txt

if ($_ROSBE_MODE -eq "MinGW") {
    $ENV:C_INCLUDE_PATH = "$_ROSBE_HOST_MINGWPATH\include;$_ROSBE_HOST_MINGWPATH\lib\gcc\$_ROSBE_GCCTARGET\$_ROSBE_GCCVERSION\include"
    $ENV:CPLUS_INCLUDE_PATH = "$_ROSBE_HOST_MINGWPATH\include;$_ROSBE_HOST_MINGWPATH\include\c++\$_ROSBE_GCCVERSION;$_ROSBE_HOST_MINGWPATH\include\c++\$_ROSBE_GCCVERSION\$_ROSBE_GCCTARGET;$_ROSBE_HOST_MINGWPATH\lib\gcc\$_ROSBE_GCCTARGET\$_ROSBE_GCCVERSION\include"
} else {
    $ENV:HOST_CFLAGS = "-I""$_ROSBE_HOST_MINGWPATH\include"" -I""$_ROSBE_HOST_MINGWPATH\lib\gcc\$_ROSBE_HOST_GCCTARGET\$_ROSBE_HOST_GCCVERSION\include"""
    $ENV:HOST_CPPFLAGS = "-I""$_ROSBE_HOST_MINGWPATH\include"" -I""$_ROSBE_HOST_MINGWPATH\include\c++\$_ROSBE_HOST_GCCVERSION"" -I""$_ROSBE_HOST_MINGWPATH\include\c++\$_ROSBE_HOST_GCCVERSION\$_ROSBE_HOST_GCCTARGET"" -I""$_ROSBE_HOST_MINGWPATH\lib\gcc\$_ROSBE_HOST_GCCTARGET\$_ROSBE_HOST_GCCVERSION\include"""
    $ENV:TARGET_CFLAGS = "-I""$_ROSBE_TARGET_MINGWPATH\include"" -I""$_ROSBE_TARGET_MINGWPATH\lib\gcc\$_ROSBE_GCCTARGET\$_ROSBE_GCCVERSION\include"""
    $ENV:TARGET_CPPFLAGS = "-I""$_ROSBE_TARGET_MINGWPATH\include"" -I""$_ROSBE_TARGET_MINGWPATH\include\c++\$_ROSBE_GCCVERSION"" -I""$_ROSBE_TARGET_MINGWPATH\include\c++\$_ROSBE_GCCVERSION\$_ROSBE_GCCTARGET"" -I""$_ROSBE_TARGET_MINGWPATH\lib\gcc\$_ROSBE_GCCTARGET\$_ROSBE_GCCVERSION\include"""
}

#
# Display the current version of GCC, NASM, ld and make.
#
if ($ENV:ROS_ARCH -ne $null) {
    & "$_ROSBE_TARGET_MINGWPATH\bin\$ENV:ROS_PREFIX-gcc" -v 2> v.txt
    (select-string -path .\v.txt "gcc version") -replace ".*: (.+?)\b",'$1'
    rm v.txt
    "gcc target - $_ROSBE_GCCTARGET"
    & "$_ROSBE_TARGET_MINGWPATH\bin\$ENV:ROS_PREFIX-ld" -v
} else {
    gcc -v 2> v.txt
    (select-string -path .\v.txt "gcc version" ) -replace ".*: (.+?)\b",'$1'
    rm v.txt
    "gcc target - $_ROSBE_GCCTARGET"
    & ld -v
}
if (Test-Path "$_ROSBE_HOST_MINGWPATH\bin\nasm.exe") {
    & nasm -v
} else {
    if (Test-Path "$_ROSBE_HOST_MINGWPATH\bin\yasm.exe") {
        & yasm --version | select-string "yasm 0"
    } else {
        if ($_ROSBE_MODE -eq "RosBE") {
            "ERROR: NASM or YASM is required to build ReactOS, none found in the current MinGW/GCC."
        }
    }
}
if (Test-Path "$_ROSBE_HOST_MINGWPATH\bin\bison.exe") {
    & bison --version | find "GNU Bison"
} else {
    if ($_ROSBE_MODE -eq "RosBE") {
        "WARNING: Bison will soon be required to build ReactOS, none found in the current MinGW/GCC."
    }
}
if (Test-Path "$_ROSBE_HOST_MINGWPATH\bin\flex.exe") {
    $fver = (& flex --version) -replace ".*version ((\d|\.)+).*",'$1'
    "flex $fver"
} else {
    if ($_ROSBE_MODE -eq "RosBE") {
        "WARNING: Flex will soon be required to build ReactOS, none found in the current MinGW/GCC."
    }
}
& mingw32-make -v | & find "GNU Make"

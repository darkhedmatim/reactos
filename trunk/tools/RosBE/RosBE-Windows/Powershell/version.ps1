#
# PROJECT:     RosBE - ReactOS Build Environment for Windows
# LICENSE:     GNU General Public License v2. (see LICENSE.txt)
# FILE:        Root/version.ps1
# PURPOSE:     Display the current version of GCC, NASM, ld and make.
# COPYRIGHT:   Copyright 2010 Daniel Reimer <reimer.daniel@freenet.de>
#

(get-WmiObject Win32_OperatingSystem).caption

# GCC
$TARGETGCC = "$global:_ROSBE_PREFIX" + "gcc"
& $TARGETGCC -v 2> gcctvers.tmp
(select-string -path .\gcctvers.tmp "gcc version") -replace ".*:(.*?)\b",'$1'
"gcc target - $ENV:ROS_ARCH"
remove-item gcctvers.tmp

# LD
$run = "$_ROSBE_TARGET_MINGWPATH\bin\$_ROSBE_PREFIX" + "ld"
& "$run" -v

# NASM or YASM
if (Test-Path "$_ROSBE_HOST_MINGWPATH\bin\nasm.exe") {
    & nasm -v
} else {
    & yasm --version | select-string "yasm 0"
}

# Bison, Flex and Make
& bison '--version' | select-string "GNU Bison"
$fver = (& flex '--version') -replace ".*version ((\d|\.)+).*",'$1'
"flex $fver"
& make.exe -v | & find "GNU Make"

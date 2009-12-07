#
# PROJECT:     RosBE - ReactOS Build Environment for Windows
# LICENSE:     GNU General Public License v2. (see LICENSE.txt)
# FILE:        Root/options.ps1
# PURPOSE:     Starts options.exe and restarts RosBE afterwards.
# COPYRIGHT:   Copyright 2009 Daniel Reimer <reimer.daniel@freenet.de>
#

$host.ui.RawUI.WindowTitle = "Options"



if ("$ENV:ROS_ARCH" -eq "amd64") {
    $options="$_ROSBE_BASEDIR\Tools\options.exe"
    $param = "amd64"
    $cfgfile="$ENV:APPDATA\RosBE\rosbe-options-amd64.ps1"
} else {
    $options="$_ROSBE_BASEDIR\Tools\options.exe"
    $param = $null
    $cfgfile="$ENV:APPDATA\RosBE\rosbe-options.ps1"
}

# Run options.exe

if (Test-Path "$options") {
    Push-Location "$_ROSBE_BASEDIR"
    &{IEX "& '$options' $param"} | out-null
    Pop-Location
    if (Test-Path "$cfgfile") {
        & "$cfgfile"
    }
} else {
    "ERROR: options executable was not found."
}

$host.ui.RawUI.WindowTitle = "ReactOS Build Environment $_ROSBE_VERSION"

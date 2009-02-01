#
# PROJECT:     RosBE - ReactOS Build Environment for Windows
# LICENSE:     GNU General Public License v2. (see LICENSE.txt)
# FILE:        Root/update.ps1
# PURPOSE:     RosBE Updater.
# COPYRIGHT:   Copyright 2009 Daniel Reimer <reimer.daniel@freenet.de>
#
#

#
# Set Title
#
$host.ui.RawUI.WindowTitle = "Updating..."

function EOC {
    set-location "$_ROSBE_OPATH"
    if ($_ROSBE_VERSION -ne $null) {
        $host.ui.RawUI.WindowTitle = "ReactOS Build Environment $_ROSBE_VERSION"
    }
    exit
}

function UPDCHECK {
    set-location "$ENV:APPDATA\RosBE\Updates"

    if (Test-Path "$_ROSBE_VERSION-$_ROSBE_STATCOUNT.txt") {
        return
    }
    if (!(Test-Path "$_ROSBE_VERSION-$_ROSBE_STATCOUNT.txt")) {
        get-webfile $_ROSBE_URL/$_ROSBE_VERSION-$_ROSBE_STATCOUNT.txt $PWD\$_ROSBE_VERSION-$_ROSBE_STATCOUNT.txt
    }
    if (Test-Path "$_ROSBE_VERSION-$_ROSBE_STATCOUNT.txt") {
        get-Content "$_ROSBE_VERSION-$_ROSBE_STATCOUNT.txt"
        ""
        "Install?"
        $YESNO = Read-Host "(yes), (no)"
        if (($YESNO -eq "yes") -or ($YESNO -eq "y")) {
            if (!(Test-Path "$_ROSBE_VERSION-$_ROSBE_STATCOUNT.7z")) {
                get-webfile $_ROSBE_URL/$_ROSBE_VERSION-$_ROSBE_STATCOUNT.txt $PWD\$_ROSBE_VERSION-$_ROSBE_STATCOUNT.txt
            }
            if (Test-Path "$_ROSBE_VERSION-$_ROSBE_STATCOUNT.7z") {
                remove-item "$_ROSBE_VERSION-$_ROSBE_STATCOUNT\*.*" -force -EA SilentlyContinue
                IEX "&'$_ROSBE_BASEDIR\Tools\7z.exe' x '$_ROSBE_VERSION-$_ROSBE_STATCOUNT.7z'"
                set-location "$_ROSBE_VERSION-$_ROSBE_STATCOUNT"
                IEX "& .\$_ROSBE_VERSION-$_ROSBE_STATCOUNT.ps1"
                return
            } else {
                "ERROR: This Update does not seem to exist or the Internet connection is not working correctly."
                return
            }
        } elseif ($YESNO -eq "no") {
            "Do you want to be asked again to install this update?"
            $YESNO = Read-Host "(yes), (no)"
            if (($YESNO -eq "yes") -or ($YESNO -eq "y")) {
                remove-item "$_ROSBE_VERSION-$_ROSBE_STATCOUNT.txt" -force -EA SilentlyContinue
            }
            return
        }
    } else {
        if ($_ROSBE_MULTIUPD -ne "1") {
            "ERROR: This Update does not seem to exist or the Internet connection is not working correctly."
            return
        }
    }
}

#
# The Update Server.
#
$_ROSBE_URL = "http://danielreimer.5x.to/rosbe"

#
# Save the recent dir to cd back there at the end.
#
$_ROSBE_OPATH = "$pwd"

if (!(Test-Path "$_ROSBE_BASEDIR\Tools\7z.exe")) {
    set-location "$_ROSBE_BASEDIR\Tools"
    get-webfile $_ROSBE_URL/7z.exe $PWD\7z.exe
    set-location $_ROSBE_OPATH
}

set-location $_ROSBE_BASEDIR

#
# First check for a new Updater.
#
rename-item update.ps1 update2.ps1
get-webfile $_ROSBE_URL/update.ps1 $PWD\update.ps1
if ((gi .\update.ps1).LastWriteTime -gt (gi .\update2.ps1).LastWriteTime) {
    clear-host
    "Updater got updated and needs to be restarted."
    remove-item update2.ps1 -force
    EOC
} else {
    remove-item update.ps1 -force
    rename-item update2.ps1 update.ps1
}
#
# Get to the Updates Subfolder.
#
if (!(Test-Path "$ENV:APPDATA\RosBE\Updates")) {New-Item -path "$ENV:APPDATA\RosBE" -name "Updates" -type directory}
set-location "$ENV:APPDATA\RosBE\Updates"

#
# Parse the args.
#
if ("$args" -eq "") {
    $_ROSBE_MULTIUPD = 1
    $_ROSBE_STATCOUNT = 1
    while ($_ROSBE_STATCOUNT -lt 10) {
        UPDCHECK
        $_ROSBE_STATCOUNT += 1
    }
    EOC
}
if ("$args" -eq "reset") {
    remove-item "$ENV:APPDATA\RosBE\Updates\*.*" -force -recurse -EA SilentlyContinue
    remove-item "$ENV:APPDATA\RosBE\Updates\tmp\*.*" -force -recurse -EA SilentlyContinue
    EOC
}
$arg1 = $args[0]
$arg2 = $args[1]
if ("$arg1" -eq "nr") {
    $_ROSBE_STATCOUNT = $arg2
    UPDCHECK
    EOC
}
if ("$arg1" -eq "delete") {
    $_ROSBE_STATCOUNT = $arg2
    remove-item "$ENV:APPDATA\RosBE\Updates\$_ROSBE_VERSION-$_ROSBE_STATCOUNT.*" -force -recurse -EA SilentlyContinue
    remove-item "$ENV:APPDATA\RosBE\Updates\tmp\$_ROSBE_VERSION-$_ROSBE_STATCOUNT.*" -force -recurse -EA SilentlyContinue
    EOC
}
if ("$arg1" -eq "info") {
    $_ROSBE_STATCOUNT = $arg2
    set-location tmp
    if (!(Test-path "$_ROSBE_VERSION-$_ROSBE_STATCOUNT.txt")) {
        get-webfile $_ROSBE_URL/$_ROSBE_VERSION-$_ROSBE_STATCOUNT.txt $PWD\$_ROSBE_VERSION-$_ROSBE_STATCOUNT.txt
        if (Test-Path "$_ROSBE_VERSION-$_ROSBE_STATCOUNT.txt") {
            get-content "$_ROSBE_VERSION-$_ROSBE_STATCOUNT.txt"
        } else {
            "ERROR: This Update does not seem to exist or the Internet connection is not working correctly."
        }
    }
    set-location ..
    remove-item "tmp\*.*" -force -EA SilentlyContinue
    EOC
}
$arg1 = $null
$arg2 = $null
if ("$args" -eq "status") {
    $_ROSBE_STATCOUNT = 1
    if (!(test-path "tmp")) {New-Item -name "tmp" -type directory}
    copy-item *.txt .\tmp\.
    set-location tmp
    while($_ROSBE_STATCOUNT -lt 10) {
        if (!(Test-path "$_ROSBE_VERSION-$_ROSBE_STATCOUNT.txt")) {
            get-webfile $_ROSBE_URL/$_ROSBE_VERSION-$_ROSBE_STATCOUNT.txt $PWD\$_ROSBE_VERSION-$_ROSBE_STATCOUNT.txt
            if (Test-Path "$_ROSBE_VERSION-$_ROSBE_STATCOUNT.txt") {
                 $_ROSBE_UPDATES += "$_ROSBE_STATCOUNT "
            }
        }
        $_ROSBE_STATCOUNT += 1
    }
    set-location ..
    remove-item "tmp\*.*" -force -EA SilentlyContinue
    if ($_ROSBE_UPDATES -ne $null) {
        "Following Updates available: $_ROSBE_UPDATES"
    } else {
        "RosBE is up to Date."
    }
    EOC
}
if ("$args" -ne "") {
    "Unknown parameter specified. Try 'help update'."
    EOC
}

$_ROSBE_UPDFINISH = $null
$_ROSBE_OPATH = $null
$_ROSBE_UPDDATE = $null
$_ROSBE_UPDATES = $null
$_ROSBE_UPDDATE2 = $null
$_ROSBE_MULTIUPD = $null
$_ROSBE_STATCOUNT = $null

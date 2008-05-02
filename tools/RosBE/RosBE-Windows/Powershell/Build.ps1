#
# PROJECT:     RosBE - ReactOS Build Environment for Windows
# LICENSE:     GNU General Public License v2. (see LICENSE.txt)
# FILE:        Root/Build.ps1
# PURPOSE:     Perform the build of ReactOS.
# COPYRIGHT:   Copyright 2008 Daniel Reimer <reimer.daniel@freenet.de>
#
#

#
# Check if config.template.rbuild is newer than config.rbuild, if it is then
# abort the build and inform the user.
#
if (Test-Path ".\config.rbuild") {
    if ((gi .\config.template.rbuild).LastWriteTime -gt (gi .\config.rbuild).LastWriteTime) {
        ""
        "*** config.template.rbuild is newer than config.rbuild ***"
        "*** aborting build. Please check for changes and       ***"
        "*** update your config.rbuild.                         ***"
        ""
        exit
    }
}

#
# Check if strip or ccache are being used and set the appropriate options.
#
if ($_ROSBE_STRIP -ne $null) {
    if ($_ROSBE_STRIP -ne 1) {
        $ENV:ROS_LEAN_AND_MEAN = "yes"
    } else {
        $ENV:ROS_LEAN_AND_MEAN = "no"
    }
}
if ($_ROSBE_USECCACHE -ne $null) {
    if ($_ROSBE_USECCACHE -eq 1) {
        $ENV:CCACHE_DIR = "$APPDATA\RosBE\.ccache"
        $ENV:HOST_CC = "ccache gcc"
        $ENV:HOST_CPP = "ccache g++"
        $ENV:TARGET_CC = "ccache gcc"
        $ENV:TARGET_CPP = "ccache g++"
    } else {
        $ENV:HOST_CC = "gcc"
        $ENV:HOST_CPP = "g++"
        $ENV:TARGET_CC = "gcc"
        $ENV:TARGET_CPP = "g++"
    }
}

#
# Check if the user has chosen to use a different object or output path and set
# it accordingly.
#
if ($_ROSBE_OBJPATH -ne $null) {
    if ( Test-Path "$_ROSBE_OBJPATH\.") {
        "ERROR: The path specified doesn't seem to exist."
        exit
    } else {
        $ENV:ROS_INTERMEDIATE = "$_ROSBE_OBJPATH"
    }
}
if ($_ROSBE_OUTPATH -ne $null) {
    if (Test-Path "$_ROSBE_OUTPATH\.") {
        "ERROR: The path specified doesn't seem to exist."
        exit
    } else {
        $ENV:ROS_OUTPUT = "$_ROSBE_OUTPATH"
        $ENV:ROS_TEMPORARY = "$_ROSBE_OUTPATH"
    }
}

#
# Get the current date and time for use in in our build log's file name.
#
$TIMERAW = get-date -f t
$DATENAME = get-date -f dyMMyyyy
$TIMENAME = get-date -f HHmm

#
# Check if writing logs is enabled, if so check if our log directory exists, if
# it doesn't, create it.
#
if ($_ROSBE_WRITELOG -eq 1) {
    if (!(Test-Path "$_ROSBE_LOGDIR")) {
        $null = (New-Item -path "$_ROSBE_ROSSOURCEDIR" -name "RosBE-Logs" -type directory)
    }
}

function BUILD {
    if ($_ROSBE_SHOWTIME -eq 1) {
        [System.Diagnostics.Stopwatch] $sw;
        $sw = New-Object System.Diagnostics.StopWatch
        if ($_ROSBE_WRITELOG -eq 1) {
            if (!(Test-Path "$_ROSBE_LOGDIR\BuildLog-$_ROSBE_GCCVERSION-$DATENAME-$TIMENAME.txt")) {
                $null = (New-Item -path "$_ROSBE_LOGDIR" -name "BuildLog-$_ROSBE_GCCVERSION-$DATENAME-$TIMENAME.txt" -type file)
            }
            $sw.Start()
            IEX "&'$_ROSBE_MINGWMAKE' $($args)" 2>&1 | tee-object -filepath "$_ROSBE_LOGDIR\BuildLog-$_ROSBE_GCCVERSION-$DATENAME-$TIMENAME.txt"
            $sw.Stop()
            write-host "Total Build Time:" $sw.Elapsed.ToString()
        } else {
            $sw.Start()
            IEX "&'$_ROSBE_MINGWMAKE' $($args)"
            $sw.Stop()
            write-host "Total Build Time:" $sw.Elapsed.ToString()
        }
    } else {
        if ($_ROSBE_WRITELOG -eq 1) {
            if (!(Test-Path "$_ROSBE_LOGDIR\BuildLog-$_ROSBE_GCCVERSION-$DATENAME-$TIMENAME.txt")) {
                $null = (New-Item -path "$_ROSBE_LOGDIR" -name "BuildLog-$_ROSBE_GCCVERSION-$DATENAME-$TIMENAME.txt" -type file)
            }
            IEX "&'$_ROSBE_MINGWMAKE' $($args)" 2>&1 | tee-object -filepath "$_ROSBE_LOGDIR\BuildLog-$_ROSBE_GCCVERSION-$DATENAME-$TIMENAME.txt"
        } else {
            IEX "&'$_ROSBE_MINGWMAKE' $($args)"
        }
    }
}

function BUILDMULTI {
    #
    # Get the number of CPUs in the system so we know how many jobs to execute.
    # To modify the number used alter the options used with cpucount:
    # No Option - Number of CPUs.
    # -x1       - Number of CPUs, plus 1.
    # -x2       - Number of CPUs, doubled.
    # -a        - Determine the cpu count based on the inherited process affinity mask.
    #
    $CPUCOUNT= (gwmi win32_processor).numberofcores + 1

    if ($_ROSBE_SHOWTIME -eq 1) {
        [System.Diagnostics.Stopwatch] $sw;
        $sw = New-Object System.Diagnostics.StopWatch
        if ($_ROSBE_WRITELOG -eq 1) {
            if (!(Test-Path "$_ROSBE_LOGDIR\BuildLog-$_ROSBE_GCCVERSION-$DATENAME-$TIMENAME.txt")) {
                $null = (New-Item -path "$_ROSBE_LOGDIR" -name "BuildLog-$_ROSBE_GCCVERSION-$DATENAME-$TIMENAME.txt" -type file)
            }
            $sw.Start()
            IEX "&'$_ROSBE_MINGWMAKE' -j $CPUCOUNT $($args)" 2>&1 | tee-object -filepath "$_ROSBE_LOGDIR\BuildLog-$_ROSBE_GCCVERSION-$DATENAME-$TIMENAME.txt"
            $sw.Stop()
            write-host "Total Build Time:" $sw.Elapsed.ToString()
        } else {
            $sw.Start()
            IEX "&'$_ROSBE_MINGWMAKE' -j $CPUCOUNT $($args)"
            $sw.Stop()
            write-host "Total Build Time:" $sw.Elapsed.ToString()
        }
    } else {
        if ($_ROSBE_WRITELOG -eq 1) {
            if (!(Test-Path "$_ROSBE_LOGDIR\BuildLog-$_ROSBE_GCCVERSION-$DATENAME-$TIMENAME.txt")) {
                $null = (New-Item -path "$_ROSBE_LOGDIR" -name "BuildLog-$_ROSBE_GCCVERSION-$DATENAME-$TIMENAME.txt" -type file)
            }
            IEX "&'$_ROSBE_MINGWMAKE' -j $CPUCOUNT $($args)" 2>&1 | tee-object -filepath "$_ROSBE_LOGDIR\BuildLog-$_ROSBE_GCCVERSION-$DATENAME-$TIMENAME.txt"
        } else {
            IEX "&'$_ROSBE_MINGWMAKE' -j $CPUCOUNT $($args)"
        }
    }
}

#
# Check if we are using -j or not.
#
if ($args.count -gt 1) {
    if ($args[0] -eq "multi") {
        $host.ui.RawUI.WindowTitle = "makex $($args) parallel build started: $TIMERAW"
    }
    BUILDMULTI $args
} else {
    if ($args.count -gt 0) {
        $host.ui.RawUI.WindowTitle = "make $($args) build started: $TIMERAW"
    }
    BUILD $args
}

#
# Highlight the fact that building has ended.
#
"$_ROSBE_BASEDIR\Tools\flash.exe"

if ($_ROSBE_VERSION -ne $null) {
    $host.ui.RawUI.WindowTitle = "ReactOS Build Environment $_ROSBE_VERSION"
}

#
# Unload all used Vars.
#
$ENV:ROS_LEAN_AND_MEAN = $null
$ENV:HOST_CC = $null
$ENV:HOST_CPP = $null
$ENV:TARGET_CC = $null
$ENV:TARGET_CPP = $null
$ENV:ROS_INTERMEDIATE = $null
$ENV:ROS_OUTPUT = $null
$ENV:ROS_TEMPORARY = $null
$ENV:CPUCOUNT = $null
$ENV:CCACHE_DIR = $null

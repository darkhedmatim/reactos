#!/bin/bash
#
# ReactOS Build Environment for Unix-based Operating Systems - Builder Tool for the Base package
# Copyright 2007-2009 Colin Finck <mail@colinfinck.de>
# partially based on the BuildMingwCross script (http://www.mingw.org/MinGWiki/index.php/BuildMingwCross)
#
# Released under GNU GPL v2 or any later version.

# Constants
ROSBE_VERSION="1.4.1"
TARGET_ARCH="i386"
KNOWN_ROSBE_VERSIONS="0.3.6 1.1 1.4 1.4.1"
DEFAULT_INSTALL_DIR="/usr/local/RosBE"
NEEDED_TOOLS="bison flex gcc g++ grep makeinfo"		# GNU Make has a special check

# Get the absolute path to the script directory
cd `dirname $0`
SCRIPTDIR="$PWD"
SOURCEDIR="$SCRIPTDIR/sources"

source "$SCRIPTDIR/scripts/rosbelibrary.sh"


echo "*******************************************************************************"
echo "*         ReactOS Build Environment for Unix-based Operating Systems          *"
echo "*                       Builder Tool for the Base package                     *"
echo "*                      by Colin Finck <mail@colinfinck.de>                    *"
echo "*                                                                             *"
echo "*                               Version $ROSBE_VERSION                                 *"
echo "*******************************************************************************"

echo
echo "This script compiles and installs a complete Build Environment for building ReactOS."
echo

setup_check_requirements

# Select the installation directory
boldmsg "Installation Directory"

echo "In which directory do you want to install it?"
echo "Enter the path to the directory here or simply press ENTER to install it into the default directory."

# RosBE-Unix 1.4 switched from /usr/RosBE to /usr/local/RosBE, show a message for the people who might want to update
if [ -f "/usr/RosBE/RosBE-Version" ]; then
	echo
	redmsg "Warning: " "-n"
	echo "The default installation path /usr/RosBE was changed to /usr/local/RosBE starting with version 1.4."
	echo "If you want to update a previous installation, enter /usr/RosBE here."
fi

createdir=false
installdir=""
reinstall=false
update=false

while [ "$installdir" = "" ]; do
	read -p "[$DEFAULT_INSTALL_DIR] " installdir
	echo

	if [ "$installdir" = "" ]; then
		installdir=$DEFAULT_INSTALL_DIR
	fi

	# Make sure we have the absolute path to the installation directory
	installdir=`eval echo $installdir`

	# Check if the installation directory already exists
	if [ -f "$installdir" ]; then
		echo "The directory \"$installdir\" is a file! Please enter another directory!"
		echo
		installdir=""

	elif [ -d "$installdir" ]; then
		# Check if the directory is empty
		if [ ! "`ls $installdir`" = "" ]; then
			if [ -f "$installdir/RosBE-Version" ]; then
				# Allow the user to update an already installed RosBE version
				choice=""
				installed_version=`cat "$installdir/RosBE-Version"`

				for known_version in $KNOWN_ROSBE_VERSIONS; do
					if [ "$known_version" = "$installed_version" ]; then
						if [ "$installed_version" = "$ROSBE_VERSION" ]; then
							echo "The current version of the Build Environment is already installed in this directory."
							echo "Please choose one of the following options:"
							echo
							echo " (R)einstall all components of the Build Environment"
							echo " (C)hoose a different installation directory"
							echo

							showchoice "R" "r R c C"
						else
							echo "An older version of the Build Environment has been found in this directory."
							echo "Please choose one of the following options:"
							echo
							echo " (U)pdate the existing Build Environment"
							echo " (R)einstall all components of the Build Environment"
							echo " (C)hoose a different installation directory"
							echo

							showchoice "U" "u U r R c C"
						fi

						echo
						break
					fi
				done

				case "$choice" in
					"U"|"u")
						update=true
						;;
					"R"|"r")
						reinstall=true
						;;
					"C"|"c")
						echo "Please enter another directory!"
						installdir=""
						;;
				esac
			else
				echo "The directory \"$installdir\" is not empty. Do you really want to continue? (yes/no)"
				read -p "[no] " answer
				echo

				if [ "$answer" != "yes" ]; then
					echo "Please enter another directory!"
					installdir=""
				fi
			fi
		fi

	else
		createdir=true
		echo "The directory \"$installdir\" does not exist. The installation script will create it for you."
		echo
	fi
done

# Ready to start
boldmsg "Ready to start"

echo "Ready to build and install the ReactOS Build Environment."
echo "Press any key to continue or Ctrl+C to exit."
read

if $update; then
	setvalues=false

	process_cpucount=false
	process_mingwruntime=false
	process_w32api=false
	process_binutils=false
	process_gcc=false
	process_make=false
	process_nasm=false
	process_buildtime=false
	process_scut=false
	process_getincludes=false

	# Logic behind this update part:
	#   - KNOWN_ROSBE_VERSIONS contains all versions from the oldest to the newest one (in this order!)
	#   - setvalues will be set to true, when the iterator reaches the installed version
	#   - Then the processing settings of the installed and the later versions will be applied.
	#     (i.e. if gcc changed from 0.3.7 to 0.3.8, but not from 0.3.6 to 0.3.7, the gcc build will correctly be reprocessed
	#      if the user has an installed RosBE 0.3.6 and wants to update to 0.3.8)
	for known_version in $KNOWN_ROSBE_VERSIONS; do
		if [ "$known_version" = "$installed_version" ]; then
			setvalues=true
		fi

		if $setvalues; then
			case "$known_version" in
				"0.3.6")
					# Updated components from 0.3.6 to 1.1
					process_cpucount=true
					process_mingwruntime=true
					process_w32api=true
					process_binutils=true
					process_gcc=true
					process_make=true
					process_nasm=true
					process_scut=true
					;;

				"1.1")
					# Updated components from 1.1 to 1.4
					process_binutils=true
					process_nasm=true

					# Reorganize the existing files
					mkdir -p "$installdir/$TARGET_ARCH/bin"
					mv "$installdir/bin/mingw32-"* "$installdir/$TARGET_ARCH/bin"
					mv "$installdir/$TARGET_ARCH/bin/mingw32-make" "$installdir/bin/make"
					mv "$installdir/bin/nasm" "$installdir/$TARGET_ARCH/bin/nasm"
					mv "$installdir/bin/ndisasm" "$installdir/$TARGET_ARCH/bin/ndisasm"
					mv "$installdir/include" "$installdir/$TARGET_ARCH/include"
					mv "$installdir/lib" "$installdir/$TARGET_ARCH/lib"
					mv "$installdir/libexec" "$installdir/$TARGET_ARCH/libexec"
					mv "$installdir/mingw32" "$installdir/$TARGET_ARCH/mingw32"
					;;

				"1.4")
					# Updated components from 1.4 to 1.4.1
					process_getincludes=true
			esac
		fi
	done
else
	process_cpucount=true
	process_mingwruntime=true
	process_w32api=true
	process_binutils=true
	process_gcc=true
	process_make=true
	process_nasm=true
	process_buildtime=true
	process_scut=true
	process_getincludes=true

	# Delete the contents of the current installation directory if we're reinstalling
	if $reinstall; then
		rm -rf "$installdir/"*
	fi

	# Create the directory if necessary
	if $createdir; then
		if ! mkdir -p "$installdir"; then
			redmsg "Could not create \"$installdir\", aborted!"
			exit 1
		fi
	fi
fi

# Test if the installation directory is writeable
if [ ! -w "$installdir" ]; then
	redmsg "Installation directory \"$installdir\" is not writeable, aborted!"
	exit 1
fi

#
# Build the tools
#
boldmsg "Building..."
mkdir "$installdir/bin" >& /dev/null
mkdir -p "$installdir/$TARGET_ARCH/mingw32" >& /dev/null

# For compiling gcc, it needs to access the already compiled mingw32 binutils
PATH="$installdir/$TARGET_ARCH/bin:$PATH"

# cpucount
if $process_cpucount; then
	echo -n "Compiling cpucount... "
	gcc -s -o "$installdir/bin/cpucount" "$SCRIPTDIR/tools/cpucount.c"
	setup_check_run
fi

CPUCOUNT=`$installdir/bin/cpucount -x1`
cd "$installdir/$TARGET_ARCH/mingw32"

# mingw-runtime
if $process_mingwruntime; then
	echo -n "Extracting mingw-runtime... "
	tar -xjf "$SOURCEDIR/mingw-runtime.tar.bz2" >& "$SCRIPTDIR/tar.log"
	setup_check_run "tar"
fi

# w32api
if $process_w32api; then
	echo -n "Extracting w32api... "
	tar -xjf "$SOURCEDIR/w32api.tar.bz2" >& "$SCRIPTDIR/tar.log"
	setup_check_run "tar"
fi

cd "$SOURCEDIR"

# binutils
if $process_binutils; then
	rm -rf "binutils"
	rm -rf "binutils-build"

	echo -n "Extracting binutils... "
	tar -xjf "binutils.tar.bz2" >& "$SCRIPTDIR/tar.log"
	setup_check_run "tar"

	echo -n "Configuring binutils... "
	mkdir "binutils-build"
	cd "binutils-build"
	../binutils/configure --prefix="$installdir/$TARGET_ARCH" --target=mingw32 --disable-nls --with-gcc \
			--with-gnu-as --with-gnu-ld --disable-shared >& "$SCRIPTDIR/configure.log"
	setup_check_run "configure"

	echo -n "Building binutils... "
	$makecmd -j $CPUCOUNT CFLAGS="-O2 -fno-exceptions" LDFLAGS="-s" >& "$SCRIPTDIR/make.log"
	setup_check_run "make"

	echo -n "Installing binutils... "
	$makecmd install >& "$SCRIPTDIR/make.log"
	setup_check_run "make"

	echo -n "Cleaning up binutils... "
	cd "$SOURCEDIR"
	rm -rf "binutils-build"
	rm -rf "binutils"
	setup_check_run
fi

# gcc
if $process_gcc; then
	rm -rf "gcc"
	rm -rf "gcc-build"

	echo -n "Extracting gcc... "
	tar -xjf "gcc.tar.bz2" >& "$SCRIPTDIR/tar.log"
	setup_check_run "tar"

	echo -n "Configuring gcc... "
	mkdir "gcc-build"
	cd "gcc-build"
	../gcc/configure --prefix="$installdir/$TARGET_ARCH" --target=mingw32 \
			--with-headers="$installdir/$TARGET_ARCH/mingw32/include"--with-gcc --with-gnu-ld \
			--with-gnu-as --enable-languages=c,c++ --enable-checking=release \
			--enable-threads=win32 --disable-win32-registry --disable-nls  \
			--disable-shared >& "$SCRIPTDIR/configure.log"
	setup_check_run "configure"

	echo -n "Building gcc... "
	$makecmd -j $CPUCOUNT CFLAGS="-O2" CXXFLAGS="-O2" LDFLAGS="-s" >& "$SCRIPTDIR/make.log"
	setup_check_run "make"

	echo -n "Installing gcc... "
	$makecmd install >& "$SCRIPTDIR/make.log"
	setup_check_run "make"

	echo -n "Cleaning up gcc... "
	cd "$SOURCEDIR"
	rm -rf "gcc-build"
	rm -rf "gcc"
	setup_check_run
fi

# make
if $process_make; then
	rm -rf "make"
	rm -rf "make-build"

	echo -n "Extracting make... "
	tar -xjf "make.tar.bz2" >& "$SCRIPTDIR/tar.log"
	setup_check_run "tar"

	echo -n "Configuring make... "
	mkdir "make-build"
	cd "make-build"
	../make/configure --prefix="$installdir" --disable-dependency-tracking \
			--disable-nls --enable-case-insensitive-file-system \
			--disable-job-server --disable-rpath >& "$SCRIPTDIR/configure.log"
	setup_check_run "configure"

	echo -n "Building make... "
	$makecmd -j $CPUCOUNT CFLAGS="-s -O2 -mms-bitfields" >& "$SCRIPTDIR/make.log"
	setup_check_run "make"

	echo -n "Installing make... "
	$makecmd install >& "$SCRIPTDIR/make.log"
	setup_check_run "make"

	echo -n "Cleaning up make... "
	cd "$SOURCEDIR"
	rm -rf "make-build"
	rm -rf "make"
	setup_check_run
fi

# nasm
if $process_nasm; then
	rm -rf "nasm"

	echo -n "Extracting nasm... "
	tar -xjf "nasm.tar.bz2" >& "$SCRIPTDIR/tar.log"
	setup_check_run "tar"

	echo -n "Configuring nasm... "
	cd "nasm"
	./configure --prefix="$installdir/$TARGET_ARCH"  >& "$SCRIPTDIR/configure.log"
	setup_check_run "configure"

	echo -n "Building nasm... "
	$makecmd -j $CPUCOUNT >& "$SCRIPTDIR/make.log"
	setup_check_run "make"

	echo -n "Installing nasm... "
	$makecmd install >& "$SCRIPTDIR/make.log"
	setup_check_run "make"

	echo -n "Cleaning up nasm... "
	cd "$SOURCEDIR"
	rm -rf "nasm"
	setup_check_run
fi

# Final actions
echo
boldmsg "Final actions"

if [ "$installdir" = "/usr/RosBE" ] && $update; then
	# Changed default installation directory starting with RosBE-Unix 1.4
	echo "Do you want to move /usr/RosBE to /usr/local/RosBE? (yes/no)"
	read -p "[yes] " answer

	if [ "$answer" = "" ] || [ "$answer" = "yes" ]; then
		mv "/usr/RosBE" "/usr/local/RosBE"
		installdir="/usr/local/RosBE"
	fi
fi

if $process_buildtime; then
	echo -n "Compiling buildtime... "
	gcc -s -o "$installdir/bin/buildtime" "$SCRIPTDIR/tools/buildtime.c"
	setup_check_run
fi

if $process_scut; then
	echo -n "Compiling scut... "
	gcc -s -o "$installdir/bin/scut" "$SCRIPTDIR/tools/scut.c"
	setup_check_run
fi

if $process_getincludes; then
	echo -n "Compiling getincludes... "
	gcc -s -o "$installdir/bin/getincludes" "$SCRIPTDIR/tools/getincludes.c"
	setup_check_run
fi

echo -n "Removing unneeded files... "
rm -rf "$installdir/share"
rm -rf "$installdir/$TARGET_ARCH/mingw32/sys-include"
rm -rf "$installdir/$TARGET_ARCH/info"
rm -rf "$installdir/$TARGET_ARCH/man"
rm -rf "$installdir/$TARGET_ARCH/share"
setup_check_run

echo -n "Removing debugging symbols... "
ls "$installdir/$TARGET_ARCH/bin/"* "$installdir/$TARGET_ARCH/mingw32/bin/"* | grep -v "gccbug" |
while read file; do
	strip "$file"
done
setup_check_run

echo -n "Copying scripts... "
cp -R "$SCRIPTDIR/scripts/"* "$installdir"
setup_check_run

echo -n "Writing version... "
echo "$ROSBE_VERSION" > "$installdir/RosBE-Version"
setup_check_run
echo

# Finish
boldmsg "Finished successfully!"
echo "To create a shortcut to the Build Environment on the Desktop, please switch back to your"
echo "normal User Account (I assume you ran this script as \"root\")."
echo "Then execute the following command:"
echo
echo "  $installdir/createshortcut.sh"
echo
echo "If you just want to start the Build Environment without using a shortcut, execute the"
echo "following command:"
echo
echo "  $installdir/RosBE.sh [source directory] [color code] [architecture]"
echo
echo "All parameters for that script are optional."

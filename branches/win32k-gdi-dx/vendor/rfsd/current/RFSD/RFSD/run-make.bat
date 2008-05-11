REM	BUILD FOR RFSD

pushd .
call "C:\Program Files\Microsoft Visual Studio .NET 2003\Common7\Tools\vsvars32.bat"
call C:\WINDDK\3790\bin\setenv.bat C:\WINDDK\3790 chk WXP
popd

cd nls
nmake

cd ..
REM set BUILD_ALLOW_ALL_WARNINGS=1

REM prefast nmake
nmake

REM prefast /filterpreset=winpft list
REM prefast list

echo Build process finished.
copy /Y chk\i386\rfsdfsd.sys Z:\Share

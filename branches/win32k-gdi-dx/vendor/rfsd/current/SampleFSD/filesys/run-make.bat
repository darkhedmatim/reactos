pushd .
call C:\WINDDK\3790\bin\setenv.bat C:\WINDDK\3790 chk WXP
popd
set BUILD_ALLOW_ALL_WARNINGS=1
cd src
nmake

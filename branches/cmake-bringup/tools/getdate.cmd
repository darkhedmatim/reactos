@echo off
goto test

:get_date_yyyymmdd
setlocal enableextensions enabledelayedexpansion
set _RV=
set _ERR=0
set _CMD=reg query "HKCU\Control Panel\International" /v sShortDate
for /f "usebackq skip=2 tokens=3,* delims= " %%i in (`%_CMD%`) do (
rem Amsterdam/Houston servers:
rem Short date format (%%i): M/d/yyyy
rem Sample %DATE%: Thu 05/29/2009
rem London servers:
rem Short date format (%%i): dd/MM/yyyy
rem Sample %DATE%: 29/05/2009
set D=!DATE!
rem Uncomment these for debug
rem echo i: %%i
rem echo D: !D!
if "%%i"=="M/d/yyyy" (
rem I'm assuming the day abbreviation will always be three chars
rem (so we account for four in total, including the space). So
rem far, I can attest that this is definitely the case on Wed &
rem Thu ;-)
set yyyy=!D:~-4!
set mm=!D:~-10,-8!
set dd=!D:~-7,-5!
) else if "%%i"=="dd/MM/yyyy" (
set yyyy=!D:~-4!
set mm=!D:~-7,-5!
set dd=!D:~-10,-8!
) else (
echo fatal: I don't understand this system's date format (%%i^)
set _ERR=1
)
)
set _RETVAL=!yyyy!!mm!!dd!
endlocal & set _YYYYMMDD=%_RETVAL% & set _ERROR=%_ERR%
exit /b %_ERROR%

:test
call :get_date_yyyymmdd
rem echo formatted date: %_YYYYMMDD% 
echo %_YYYYMMDD%
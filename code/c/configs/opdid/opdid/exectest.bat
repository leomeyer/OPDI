@echo off

:loop
if "%1" == "" goto end
echo exectest.bat parameter: %1
shift
goto loop

set /p id="Enter test value: "

:end

@echo off
REM Runs "cmake .." in the current directory (the build directory) and shows how far it got.
REM   call scripts\CMakeGenerate.bat "<log file>"
REM CMake's whole output is appended to the log file. The exit code is CMake's.
REM
REM vcpkg installs the project's packages while CMake configures, which takes many minutes when they
REM have to be built: the bar follows vcpkg's "Installing N/M <package>" lines (0 - 80%), then CMake's
REM own milestones (80 - 100%).
REM
REM Paths are only ever read as !var! while delayed expansion is on: a %var% holding a "!" (a folder
REM named "Hello!") would lose it there.

setlocal DisableDelayedExpansion
if "%~1"==":run" goto run_cmake

set "_log=%~1"
if "%_log%"=="" (
    echo CMakeGenerate.bat: no log file given.
    exit /b 2
)
REM A name of its own per run: a CMake left running by an earlier window that was closed writes its
REM own file, never this run's. Files such runs left behind are removed.
del /f /q "%CD%\cmake_generate.*.exitcode" >nul 2>&1
set "_exitfile=%CD%\cmake_generate.%RANDOM%%RANDOM%.exitcode"
for /f %%A in ('copy /Z "%~f0" nul') do set "CR=%%A"

REM CMake runs in the background (this script's :run mode, below); it writes CMake's exit code to
REM a file when it ends.
start "CMakeGenerate" /b cmd /d /c call "%~f0" :run "%_log%" "%_exitfile%"

setlocal EnableDelayedExpansion
set "_pad=                                        "
set "_percent=0"
set "_text=Starting CMake"
call :seconds _start

REM Progress milestones CMake prints after vcpkg is done (in no particular order)
set k1=The C compiler identification
set k2=The CXX compiler identification
set k3=Detecting C compiler ABI info
set k4=Detecting CXX compiler ABI info
set k5=Check for working C compiler
set k6=Check for working CXX compiler
set k7=Detecting C compile features
set k8=Detecting CXX compile features
set k9=Configuring done
set k10=Generating done
set k11=Build files have been written

:wait
call :draw
if exist "!_exitfile!" goto finished
>nul ping 127.0.0.1 -n 2
call :update
goto wait

:finished
REM The background half creates the file before it writes the code: read it again until it is there.
set "_rc="
for /L %%i in (1,1,20) do if not defined _rc (
    set /p _rc=<"!_exitfile!"
    if not defined _rc >nul ping 127.0.0.1 -n 2
)
del /f /q "!_exitfile!" >nul 2>&1
if not defined _rc set "_rc=1"
set "_rc=!_rc: =!"
if "!_rc!"=="0" (
    set "_percent=100"
    set "_text=Done"
) else (
    set "_text=CMake failed"
)
call :draw
echo.
exit /b !_rc!

:update
if not exist "!_log!" goto :eof
findstr /L /C:"-- Running vcpkg install" "!_log!" >nul 2>&1 && set "_text=vcpkg: checking packages"

REM The last "Installing N/M <package>" line: package N of M is being installed.
set "_vcpkg="
for /f "delims=" %%L in ('findstr /B /R /C:"Installing [0-9]*/[0-9]* " "!_log!" 2^>nul') do set "_vcpkg=%%L"
if defined _vcpkg (
    for /f "tokens=2,3,4 delims=/ " %%a in ("!_vcpkg!") do (
        set /a "_percent=(%%a - 1) * 80 / %%b"
        for /f "tokens=1 delims=[:" %%p in ("%%c") do set "_text=vcpkg %%a/%%b: %%p"
    )
)

findstr /L /C:"-- Running vcpkg install - done" "!_log!" >nul 2>&1 && (
    set "_percent=80"
    set "_text=Configuring the project"
)

set "_found=0"
for /L %%S in (1,1,11) do (
    findstr /L /C:"!k%%S!" "!_log!" >nul 2>&1 && (
        set /a _found+=1
        set "_text=!k%%S!"
    )
)
if !_found! GTR 0 set /a "_percent=80 + _found * 19 / 11"
REM After the 8 compiler checks CMake configures the project itself until "Configuring done"
if !_found! EQU 8 set "_text=Configuring the project"
goto :eof

:draw
set /a "_filled=_percent * 30 / 100"
set "_bar="
for /L %%i in (1,1,30) do if %%i LEQ !_filled! (set "_bar=!_bar!#") else (set "_bar=!_bar!-")
set "_p=  !_percent!"
set "_p=!_p:~-3!"
call :seconds _now
set /a "_elapsed=_now - _start"
if !_elapsed! LSS 0 set /a "_elapsed+=86400"
set /a "_min=_elapsed / 60, _sec=_elapsed %% 60"
set "_sec=0!_sec!"
set "_sec=!_sec:~-2!"
set "_t=!_text!!_pad!"
set "_t=!_t:~0,40!"
<nul set /p "=[!_bar!] !_p!%% !_min!:!_sec! !_t!!CR!"
goto :eof

:seconds
REM Seconds since midnight from %time% (hh:mm:ss.cc, any separators)
set "_tm=%time: =0%"
set /a "%~1=(1%_tm:~0,2% - 100) * 3600 + (1%_tm:~3,2% - 100) * 60 + (1%_tm:~6,2% - 100)"
goto :eof

:run_cmake
REM Background half, started above as: CMakeGenerate.bat :run "<log file>" "<exit code file>"
REM "call" so that a cmake.cmd / cmake.bat wrapper on the PATH returns here too.
call cmake .. >> "%~2" 2>&1
> "%~3" echo %errorlevel%
exit /b

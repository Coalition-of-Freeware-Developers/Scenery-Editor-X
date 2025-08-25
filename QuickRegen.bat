@echo off
REM Quick CMake project regeneration for Visual Studio
REM Does NOT clear build cache or assets, just reruns CMake in build dir

REM Enable ANSI escape sequences
for /F "tokens=1,2 delims=#" %%a in ('"prompt #$H#$E# & echo on & for %%b in (1) do rem"') do (
  set "ESC=%%b"
)
set log_file=%~dp0CMake_Gen.log
echo %ESC%[92m-----------------------------------%ESC%[0m
echo %ESC%[92mRunning CMake Project Quick Generation%ESC%[0m
echo %ESC%[92m-----------------------------------%ESC%[0m

set log_file=%~dp0CMake_Gen.log

REM Check for build directory
if not exist "build" (
    echo %ESC%[91mBuild directory does not exist!%ESC%[0m
    echo %ESC%[91mRun SetupProject.bat first to initialize the project.%ESC%[0m
    exit /b 1
)

REM Check for CMake
where /q cmake
if errorlevel 1 (
	echo CMake not found on PATH! Please install CMake and add it to your PATH.
	echo -----------------------------------
	exit /b 1
) else (
	echo CMake found on PATH.
	echo -----------------------------------
)

REM Regenerate Visual Studio project files
echo %ESC%[92mStarting CMake Project Generation%ESC%[0m
echo %ESC%[92m-----------------------------------%ESC%[0m
cd build
(
    echo %ESC%[92mCMake-----------------------------------%ESC%[0m
    echo %ESC%[92mRunning CMake Project Generation%ESC%[0m
    echo %ESC%[92m%date% %time%%ESC%[0m
    echo %ESC%[92m-----------------------------------%ESC%[0m
) > "%~dp0CMake_Gen.log"
REM --- Run CMake asynchronously and show a pseudo progress bar while it works ---
set "_cmakeFlag=%temp%\cmake_quickregen_done.flag"
if exist "%_cmakeFlag%" del /f /q "%_cmakeFlag%" >nul 2>&1

REM Launch cmake in background; when finished create a flag file
REM Launch CMake (background) and create flag file on completion (removed needless backslash escapes)
start "CMakeGenerate" /b cmd /c "( cmake .. >> "%~dp0CMake_Gen.log" 2>&1 & echo done>"%_cmakeFlag%" )"

REM Progress loop (uses scripts\Progress.bat). We can't know real progress, so we animate 0-95%.
set /a _p=0
set /a _pmax=100
set "_progressScript=%~dp0scripts\Progress.bat"
if not exist "%_progressScript%" (
    REM Fallback: wait silently if progress script missing
    echo (Progress script not found, waiting for CMake...)
    goto :wait_for_cmake_quickregen
)

echo Tracking CMake progress (log milestones)...
call "%_progressScript%" 50 0 100
set stepsTotal=11
set currentStep=0
set lastStep=0
set fallbackPercent=0
set maxFallback=90
set idleLoops=0
set maxIdleLoops=6

REM Progress milestone keywords (ordered)
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

:progress_loop_quickregen
if exist "%_cmakeFlag%" goto :finish_progress_quickregen

REM Update milestone based on log contents
for /L %%S in (1,1,11) do (
    if !currentStep! lss %%S (
        for /f "delims=" %%L in ('findstr /C:"!k%%S!" "%~dp0CMake_Gen.log" 2^>nul') do set currentStep=%%S
    )
)

REM Compute percent from milestones
set /a percent=(currentStep*100)/stepsTotal

REM Fallback incremental if idle
if !currentStep! EQU !lastStep! (
        set /a idleLoops+=1
        if !idleLoops! GEQ !maxIdleLoops! if !fallbackPercent! LSS !maxFallback! (
                set /a fallbackPercent+=1
                set idleLoops=0
        )
) else (
        set idleLoops=0
)

if !fallbackPercent! GTR !percent! set percent=!fallbackPercent!
if !percent! GTR 99 set percent=99

call "%_progressScript%" 50 !percent! 100
set lastStep=!currentStep!

>nul ping 127.0.0.1 -n 2
goto :progress_loop_quickregen

:finish_progress_quickregen
call "%_progressScript%" 50 100 100

:wait_for_cmake_quickregen
REM Ensure cmake finished before proceeding
if not exist "%_cmakeFlag%" goto :wait_for_cmake_quickregen
del /f /q "%_cmakeFlag%" >nul 2>&1
cd ..


findstr /C:"CMake Error" "%log_file%" > nul
if %errorlevel% equ 0 (
	echo %ESC%[91m-----------------------------------%ESC%[0m
    echo %ESC%[91mCMake Error Detected!%ESC%[0m
    echo %ESC%[91mPlease check "CMake_Gen.log" for more information about the errors.%ESC%[0m
    echo %ESC%[91m-----------------------------------%ESC%[0m
) else (
    echo %ESC%[92mCMake Project Generation Complete%ESC%[0m
    echo %ESC%[92m-----------------------------------%ESC%[0m
    echo %ESC%[92mA log file has been created called CMake_Gen.log%ESC%[0m
	echo %ESC%[92m-----------------------------------%ESC%[0m
)

echo %ESC%[92mPress any key to exit...%ESC%[0m
pause >nul

@echo off

REM Enable ANSI escape sequences
for /F "tokens=1,2 delims=#" %%a in ('"prompt #$H#$E# & echo on & for %%b in (1) do rem"') do (
  set "ESC=%%b"
)

set log_file=%~dp0CMake_Gen.log
echo %ESC%[92m-----------------------------------%ESC%[0m
echo %ESC%[92mRunning CMake Project Generation%ESC%[0m
echo %ESC%[92m-----------------------------------%ESC%[0m

echo -----------------------------------
echo Setting up project
echo -----------------------------------

choice /m "Do you want to update git submodules?"
if errorlevel 2 (
	echo Skipping git submodule update.
	echo -----------------------------------
	goto build_directory
) else (
	echo Updating git submodules.
	echo -----------------------------------
	where /q git
	if errorlevel 1 (
	    echo Cannot find git on PATH! Please make sure git repository is initialized.
	    echo -----------------------------------
	    echo Please initialize submodules manually and rerun.
	    exit /b 1
	) ELSE (
	    git submodule sync --recursive
	    git submodule update --init --recursive
	)
)

:build_directory
echo Checking if Build directory exists.
echo -----------------------------------
if exist "build" (
    choice /m "Do you want to generate a fresh build by deleting the existing build folder?"
    if errorlevel 2 (
        echo Keeping existing build directory.
        echo -----------------------------------
    ) else (
        echo Generating a fresh build.
        echo -----------------------------------
        echo Deleting existing Build directory.
        rmdir /s /q "build"
        echo -----------------------------------
        mkdir build
    )
) else (
    echo Creating Build directory.
    echo -----------------------------------
    mkdir build
)

if exist "build\assets" (
    echo Assets folder already exists in Build directory.
    choice /m "Do you want to overwrite the assets folder?"
    if errorlevel 2 (
        echo Skipping...
		echo -----------------------------------
        goto cmake_generation
    ) else (
        echo Overwriting assets folder.
		echo -----------------------------------
        rmdir /s /q "build\assets"
		echo -----------------------------------
    )
)

echo Copying assets folder to Build directory.
echo -----------------------------------
xcopy /E /I "assets" "build\assets"
echo -----------------------------------

:cmake_generation
echo Checking if CMake is installed.
echo -----------------------------------
where /q cmake

if errorlevel 1 (
	echo CMake not found on PATH! Please install CMake and add it to your PATH.
	echo -----------------------------------
	exit /b 1
) else (
	echo CMake found on PATH.
	echo -----------------------------------
)

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
set "_cmakeFlag=%temp%\cmake_setup_done.flag"
if exist "%_cmakeFlag%" del /f /q "%_cmakeFlag%" >nul 2>&1

REM Launch CMake (background) and create flag file on completion (removed needless backslash escapes)
start "CMakeGenerate" /b cmd /c "( cmake .. >> "%~dp0CMake_Gen.log" 2>&1 & echo done>"%_cmakeFlag%" )"

set /a _p=0
set /a _pmax=100
set "_progressScript=%~dp0scripts\Progress.bat"
if not exist "%_progressScript%" (
    echo (Progress script not found, waiting for CMake...)
    goto :wait_for_cmake_setup
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

:progress_loop_setup
if exist "%_cmakeFlag%" goto :finish_progress_setup

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
goto :progress_loop_setup

:finish_progress_setup
call "%_progressScript%" 50 100 100

:wait_for_cmake_setup
if not exist "%_cmakeFlag%" goto :wait_for_cmake_setup
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

REM --- Prompt for Doxygen documentation generation ---
echo %ESC%[92mPress any key to exit...%ESC%[0m
pause >nul

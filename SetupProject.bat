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
cmake .. >> "%~dp0CMake_Gen.log" 2>&1
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

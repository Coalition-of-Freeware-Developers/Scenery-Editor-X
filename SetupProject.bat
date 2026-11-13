@echo off

REM Enable ANSI escape sequences
for /F "tokens=1,2 delims=#" %%a in ('"prompt #$H#$E# & echo on & for %%b in (1) do rem"') do (
  set "ESC=%%b"
)

REM Work from the repository root, wherever the script was started from
cd /d "%~dp0"

set "log_file=%~dp0CMake_Gen.log"
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
    choice /m "Do you want to generate a fresh build by deleting the existing build files?"
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
	if exist "lib" (
		echo Deleting existing lib directory.
		rmdir /s /q "lib"
		echo -----------------------------------
	)
	if exist "bin" (
		echo Deleting existing bin directory.
		rmdir /s /q "bin"
		echo -----------------------------------
	)
	if exist "pdb" (
		echo Deleting existing pdb directory.
		rmdir /s /q "pdb"
		echo -----------------------------------
	)
) else (
    echo Creating Build directory.
    echo -----------------------------------
    mkdir build
)

if exist "build\resources" (
    echo Resources folder already exists in Build directory.
    choice /m "Do you want to overwrite the resources folder?"
    if errorlevel 2 (
        echo Skipping...
		echo -----------------------------------
        goto cmake_generation
    ) else (
        echo Overwriting resources folder.
		echo -----------------------------------
        rmdir /s /q "build\resources"
		echo -----------------------------------
    )
)

echo Copying resources folder to Build directory.
echo -----------------------------------
xcopy /E /I "resources" "build\resources"
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
echo vcpkg first installs the packages the project needs. Packages that are not in vcpkg's
echo binary cache are built from source, which can take 20 minutes or more on a fresh build.
echo -----------------------------------
cd build
(
    echo %ESC%[92mCMake-----------------------------------%ESC%[0m
    echo %ESC%[92mRunning CMake Project Generation%ESC%[0m
    echo %ESC%[92m%date% %time%%ESC%[0m
    echo %ESC%[92m-----------------------------------%ESC%[0m
) > "%log_file%"
call "%~dp0scripts\CMakeGenerate.bat" "%log_file%"
set "cmake_result=%errorlevel%"
cd ..

if not "%cmake_result%"=="0" (
    echo %ESC%[91m-----------------------------------%ESC%[0m
    echo %ESC%[91mCMake Project Generation Failed [exit code %cmake_result%]%ESC%[0m
    echo %ESC%[91m-----------------------------------%ESC%[0m
    echo The end of CMake_Gen.log:
    set "SEDX_CMAKE_LOG=%log_file%"
    powershell -NoProfile -Command "Get-Content -LiteralPath $env:SEDX_CMAKE_LOG -Tail 25"
    echo %ESC%[91m-----------------------------------%ESC%[0m
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
exit /b %cmake_result%

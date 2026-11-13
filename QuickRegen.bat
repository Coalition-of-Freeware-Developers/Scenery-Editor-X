@echo off
REM Quick CMake project regeneration for Visual Studio
REM Does NOT clear build cache or assets, just reruns CMake in build dir

REM Enable ANSI escape sequences
for /F "tokens=1,2 delims=#" %%a in ('"prompt #$H#$E# & echo on & for %%b in (1) do rem"') do (
  set "ESC=%%b"
)

REM Work from the repository root, wherever the script was started from
cd /d "%~dp0"

set "log_file=%~dp0CMake_Gen.log"
echo %ESC%[92m-----------------------------------%ESC%[0m
echo %ESC%[92mRunning CMake Project Quick Generation%ESC%[0m
echo %ESC%[92m-----------------------------------%ESC%[0m

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

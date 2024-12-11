@echo off

echo Updating git submodules ...

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

set log_file=%~dp0CMake_Gen.log
echo -----------------------------------
echo Running CMake Project Generation
echo -----------------------------------

echo Checking if Build directory exists.

if not exist "build" (
        echo Creating Build directory.
        mkdir build
)
cd build
(
    echo -----------------------------------
    echo Running CMake Project Generation
    echo %date% %time%
    echo -----------------------------------
) > "%~dp0CMake_Gen.log"
cmake .. >> "%~dp0CMake_Gen.log" 2>&1
cd ..
echo CMake output logged to CMake_Gen.log

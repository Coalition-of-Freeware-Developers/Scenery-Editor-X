@echo off

set log_file=%~dp0CMake_Gen.log
echo -----------------------------------
echo Running CMake Project Generation
echo -----------------------------------

echo Checking if Build directory exists.

if not exist "build" (
        echo Creating Build directory.
        mkdir build
)
else (
		echo Build directory already exists.
)

if exist "build\assets" (
    echo Assets folder already exists in Build directory.
    choice /m "Do you want to overwrite the assets folder?"
    if errorlevel 2 (
        echo Skipping...
        goto cmake_generation
    ) else (
        echo Overwriting assets folder.
        rmdir /s /q "build\assets"
    )
)

echo Copying assets folder to Build directory.
xcopy /E /I "assets" "build\assets"

:cmake_generation
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
timeout /t 5 /nobreak > nul

echo -----------------------------------
echo Project Setup Complete
echo -----------------------------------
timeout /t 2 > nul

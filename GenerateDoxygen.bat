@echo off
REM Doxygen Documentation Generation Script
REM This script checks for Doxygen and generates documentation using CMake if available.

REM Enable ANSI escape sequences
for /F "tokens=1,2 delims=#" %%a in ('"prompt #$H#$E# & echo on & for %%b in (1) do rem"') do (
  set "ESC=%%b"
)

echo %ESC%[92m-----------------------------------%ESC%[0m
echo %ESC%[92mDoxygen Documentation Generation%ESC%[0m
echo %ESC%[92m-----------------------------------%ESC%[0m

REM Check if Doxygen is installed
where /q doxygen
if errorlevel 1 (
    echo %ESC%[91mDoxygen not found on PATH! Please install Doxygen and add it to your PATH.%ESC%[0m
    echo %ESC%[91mDocumentation will not be generated.%ESC%[0m
    exit /b 1
) else (
    echo %ESC%[92mDoxygen found on PATH.%ESC%[0m
    echo %ESC%[92mGenerating documentation...%ESC%[0m
    cd scripts
    doxygen Doxyfile
    cd ..
    if errorlevel 1 (
        echo %ESC%[91mDoxygen documentation generation failed!%ESC%[0m
    ) else (
        echo %ESC%[92mDoxygen documentation generated successfully.%ESC%[0m
    )
)
echo %ESC%[92mPress any key to exit...%ESC%[0m
pause >nul

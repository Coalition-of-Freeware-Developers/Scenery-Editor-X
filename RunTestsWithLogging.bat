@echo off
REM -------------------------------------------------------
REM Scenery Editor X - Test Runner with Enhanced Logging
REM -------------------------------------------------------
REM Copyright (c) 2025 Thomas Ray
REM Copyright (c) 2025 Coalition of Freeware Developers
REM -------------------------------------------------------
REM RunTestsWithLogging.bat
REM -------------------------------------------------------
REM Batch script to run all tests with comprehensive logging
REM -------------------------------------------------------

echo === Scenery Editor X - Test Runner with Enhanced Logging ===
echo.

REM Create logs directory if it doesn't exist (for all logging)
if not exist "logs" (
    echo Creating logs directory...
    mkdir "logs"
)

REM Generate timestamp for log files
for /f "tokens=2 delims==" %%a in ('wmic OS Get localdatetime /value') do set "dt=%%a"
set "timestamp=%dt:~0,4%%dt:~4,2%%dt:~6,2%_%dt:~8,2%%dt:~10,2%%dt:~12,2%"

echo Timestamp: %timestamp%
echo All test outputs will be saved to logs\ directory
echo.

REM Function to run a test with logging
REM %1 = Test name
REM %2 = Executable path
REM %3 = Additional arguments (optional)

:run_test
setlocal
set "test_name=%1"
set "test_exe=%2"
set "test_args=%3"

echo Running %test_name%...
if exist "%test_exe%" (
    set "console_log=logs\%test_name%_console_%timestamp%.log"
    echo   Console output will be logged to: !console_log!
    "%test_exe%" %test_args% 2>&1 | tee "!console_log!"
    echo   ✓ %test_name% completed - Console log: !console_log!
) else (
    echo   ⚠ %test_exe% not found - make sure tests are built
)
echo.
endlocal
goto :eof

REM Run all tests
echo 1. Running Reference Counting Tests...
call :run_test "RefTests" "build\source\Tests\Debug\RefTests.exe" "--reporter console --verbosity high"

echo 2. Running Memory Allocator Tests...
call :run_test "MemoryTests" "build\source\Tests\Debug\MemoryAllocatorTests.exe" "--reporter console --verbosity high"

echo 3. Running Settings Tests...
call :run_test "SettingsTests" "build\source\Tests\Debug\SettingsTest.exe" "--reporter console --verbosity high"

echo 4. Running Format Tester...
call :run_test "FormatTester" "build\source\Tests\Debug\FormatTester.exe" ""

echo === Test Execution Summary ===
echo All test logs saved to logs\ directory with timestamp: %timestamp%
echo.

echo Test Log Files:
dir /b "logs\*%timestamp%*.log" 2>nul
echo.

echo To view logs:
echo   type logs\[filename]
echo.

echo Test run completed!
pause

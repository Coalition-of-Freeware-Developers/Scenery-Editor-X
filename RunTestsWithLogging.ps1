# -------------------------------------------------------
# Scenery Editor X - Test Runner with Enhanced Logging
# -------------------------------------------------------
# Copyright (c) 2025 Thomas Ray
# Copyright (c) 2025 Coalition of Freeware Developers
# -------------------------------------------------------
# RunTestsWithLogging.ps1
# -------------------------------------------------------
# PowerShell script to run all tests with comprehensive logging
# -------------------------------------------------------

param(
    [switch]$SkipBuild = $false,
    [string]$TestFilter = "*",
    [switch]$Verbose = $false
)

function Write-ColoredOutput {
    param(
        [string]$Message,
        [string]$Color = "White"
    )
    Write-Host $Message -ForegroundColor $Color
}

function Run-TestWithLogging {
    param(
        [string]$TestName,
        [string]$TestExe,
        [string]$TestArgs = "",
        [string]$Timestamp
    )

    Write-ColoredOutput "Running $TestName..." "Cyan"

    if (Test-Path $TestExe) {
        $consoleLog = "logs\${TestName}_console_${Timestamp}.log"
        Write-Host "  Console output will be logged to: $consoleLog"

        if ($TestArgs) {
            & $TestExe $TestArgs.Split(' ') 2>&1 | Tee-Object -FilePath $consoleLog
        } else {
            & $TestExe 2>&1 | Tee-Object -FilePath $consoleLog
        }        if ($LASTEXITCODE -eq 0) {
            Write-ColoredOutput "  ✓ $TestName completed successfully - Console log: $consoleLog" "Green"
        } else {
            Write-ColoredOutput "  ✗ $TestName failed with exit code $LASTEXITCODE - Console log: $consoleLog" "Red"
        }
    } else {
        Write-ColoredOutput "  ⚠ $TestExe not found - make sure tests are built" "Yellow"
    }
    Write-Host ""
}

# Main script
Write-ColoredOutput "=== Scenery Editor X - Test Runner with Enhanced Logging ===" "Magenta"
Write-Host ""

# Create directory
if (!(Test-Path "logs")) {
    Write-Host "Creating logs directory..."
    New-Item -ItemType Directory -Path "logs" | Out-Null
}

# Generate timestamp
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
Write-Host "Timestamp: $timestamp"
Write-Host "All test outputs will be saved to logs\ directory"
Write-Host ""

# Build tests if not skipped
if (!$SkipBuild) {
    Write-ColoredOutput "Building tests..." "Yellow"

    # Configure CMake
    Write-Host "Configuring CMake..."
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    if ($LASTEXITCODE -ne 0) {
        Write-ColoredOutput "CMake configuration failed!" "Red"
        exit 1
    }

    # Build all tests
    Write-Host "Building all test targets..."
    cmake --build build --target RefTests MemoryAllocatorTests SettingsTest FormatTester --config Debug --parallel
    if ($LASTEXITCODE -ne 0) {
        Write-ColoredOutput "Build failed!" "Red"
        exit 1
    }

    Write-ColoredOutput "Build completed successfully!" "Green"
    Write-Host ""
}

# Define test configurations
$tests = @(
    @{
        Name = "RefTests"
        Exe = "build\source\Tests\Debug\RefTests.exe"
        Args = "--reporter console --verbosity high"
    },
    @{
        Name = "MemoryTests"
        Exe = "build\source\Tests\Debug\MemoryAllocatorTests.exe"
        Args = "--reporter console --verbosity high"
    },
    @{
        Name = "SettingsTests"
        Exe = "build\source\Tests\Debug\SettingsTest.exe"
        Args = "--reporter console --verbosity high"
    },
    @{
        Name = "FormatTester"
        Exe = "build\source\Tests\Debug\FormatTester.exe"
        Args = ""
    }
)

# Filter tests if specified
if ($TestFilter -ne "*") {
    $tests = $tests | Where-Object { $_.Name -like $TestFilter }
}

# Run tests
$testNumber = 1
foreach ($test in $tests) {
    Write-Host "$testNumber. Running $($test.Name)..."
    Run-TestWithLogging -TestName $test.Name -TestExe $test.Exe -TestArgs $test.Args -Timestamp $timestamp
    $testNumber++
}

# Summary
Write-ColoredOutput "=== Test Execution Summary ===" "Magenta"
Write-Host "All test logs saved to logs\ directory with timestamp: $timestamp"
Write-Host ""

Write-Host "Test Log Files:"
Get-ChildItem "logs\*$timestamp*.log" -ErrorAction SilentlyContinue | ForEach-Object {
    Write-Host "  - $($_.Name)"
}
Write-Host ""

Write-Host "To view logs:"
Write-Host "  Get-Content logs\[filename]"
Write-Host ""

Write-ColoredOutput "Test run completed!" "Green"

# Show recent logs if verbose
if ($Verbose) {
    Write-Host ""
    Write-ColoredOutput "=== Recent Log Files ===" "Cyan"

    $recentConsoleLog = Get-ChildItem "logs\*console*$timestamp*.log" -ErrorAction SilentlyContinue |
                        Sort-Object LastWriteTime -Descending |
                        Select-Object -First 1

    if ($recentConsoleLog) {
        Write-Host "Most recent console log: $($recentConsoleLog.Name)"
        Write-Host "Last 20 lines:"
        Get-Content $recentConsoleLog.FullName -Tail 20
    }
}

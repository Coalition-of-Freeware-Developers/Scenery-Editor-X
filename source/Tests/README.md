# Scenery Editor X Reference Counting Tests

This directory contains unit tests for the reference counting system used in the Scenery Editor X project.

## Test Files

- **RefTest.cpp**: Tests for basic functionality of the `Ref<T>` smart pointer class
- **WeakRefTest.cpp**: Tests for the `WeakRef<T>` weak reference implementation
- **RefPerformanceTest.cpp**: Performance and stress tests for reference counting
- **RefThreadSafetyTest.cpp**: Tests for thread safety of the reference counting system

## Enhanced Logging System

The test suite now includes a comprehensive logging system that captures detailed test execution information without requiring the main application framework as a dependency.

### Logging Components

- **TestLogger.h**: Lightweight standalone logging system for tests
- **SimpleTestHelper.h**: RAII helpers and macros for easy test logging
- **Catch2TestListener.h**: Event listener for automatic Catch2 test event logging

### Log Output Locations

The logging system creates two types of logs:

1. **Console Logs** (`test-logs/` directory):
   - Capture all console output from test execution
   - Include Catch2 test results and timing information
   - Timestamped filenames for easy identification

2. **Detailed Application Logs** (`logs/` directory):
   - Created by the test executables themselves
   - Include detailed test case information, assertions, and custom logging
   - Structured format with timestamps and log levels

## Building and Running Tests

### Prerequisites

- CMake 3.16 or higher
- C++20 compatible compiler
- Catch2 v3 installed and findable by CMake

### Build Instructions

```bash
# Create a build directory
mkdir build
cd build

# Configure CMake
cmake ..

# Build the tests
cmake --build .

# Run the tests
ctest
# OR
./RefTests
```

### Running Tests with Enhanced Logging

#### Using VS Code Tasks

The project includes enhanced VS Code tasks that automatically capture test output:

- **Run All Tests with Comprehensive Logging**: Runs all tests and saves detailed logs
- **Run RefTests with Enhanced Logging**: Runs only the reference counting tests with logging
- **Run Memory Tests with Enhanced Logging**: Runs memory allocator tests with logging
- **Run Settings Tests with Enhanced Logging**: Runs settings tests with logging
- **CTest - Run All with Enhanced Logging**: Runs all tests via CTest with verbose logging

#### Using PowerShell Script

```powershell
# Run all tests with logging
.\RunTestsWithLogging.ps1

# Run all tests without rebuilding
.\RunTestsWithLogging.ps1 -SkipBuild

# Run specific tests only
.\RunTestsWithLogging.ps1 -TestFilter "RefTests"

# Run with verbose output (shows recent log contents)
.\RunTestsWithLogging.ps1 -Verbose
```

#### Using Batch Script (Windows)

```cmd
# Run all tests with logging
RunTestsWithLogging.bat
```

#### Manual Execution with Logging

```bash
# Create log directories
mkdir test-logs logs

# Run tests with console output capture
./RefTests --reporter console --verbosity high 2>&1 | tee test-logs/RefTests_$(date +%Y%m%d_%H%M%S).log
```

### Running Specific Tests

You can run specific test categories using Catch2 tags:

```bash
# Run all tests
./RefTests

# Run only Ref tests
./RefTests "[Ref]"

# Run only WeakRef tests
./RefTests "[WeakRef]"

# Run performance tests
./RefTests "[performance]"

# Run thread safety tests
./RefTests "[thread]"

# Exclude performance tests
./RefTests "~[performance]"

# Run with enhanced Catch2 output
./RefTests --reporter console --verbosity high
```

## Log File Examples

### Console Log Format
```
Starting RefTests - Output will be logged to: test-logs/RefTests_console_20250118_143022.log
Note: The test executable also creates its own detailed log in logs/ directory

===============================================================================
All tests passed (42 assertions in 12 test cases)

RefTests completed. Check both:
  - Console log: test-logs/RefTests_console_20250118_143022.log
  - Detailed application log: logs/RefTests_*.log
```

### Detailed Application Log Format
```
[2025-01-18 14:30:22.123] [INFO ] [MAIN] === Scenery Editor X - Reference Counting Tests ===
[2025-01-18 14:30:22.124] [INFO ] [MAIN] Test executable: RefTests
[2025-01-18 14:30:22.125] [INFO ] [MAIN] Log file: logs/RefTests_20250118_143022.log
[2025-01-18 14:30:22.126] [INFO ] [TEST_START] Starting test: Ref default construction
[2025-01-18 14:30:22.127] [INFO ] [TEST_START] ================================================================================
[2025-01-18 14:30:22.128] [INFO ] [SECTION] --- Default constructor creates null reference ---
[2025-01-18 14:30:22.129] [INFO ] [TEST] Testing default constructor behavior
[2025-01-18 14:30:22.130] [INFO ] [TEST] Created default Ref<TestObject>
[2025-01-18 14:30:22.131] [INFO ] [ASSERTION] PASS: ref -> false: Default ref should be false/null
```

## Test Coverage

The tests cover the following aspects of the reference counting system:

1. **Basic Functionality**
   - Creation and destruction of references
   - Reference counting
   - Pointer access and dereferencing
   - Type conversion and casting

2. **Thread Safety**
   - Concurrent reference creation and destruction
   - Concurrent reference access and modification
   - Thread contention scenarios

3. **Performance**
   - Creation and destruction performance
   - Copy and move performance
   - Memory usage
   - Comparison with std::shared_ptr

4. **Memory Safety**
   - Proper cleanup of resources
   - Handling of circular references with WeakRef
   - Prevention of memory leaks

## Troubleshooting

### Common Issues

1. **Tests run too fast to see output**:
   - Use the enhanced logging tasks or scripts
   - Check the generated log files in `test-logs/` and `logs/` directories

2. **Log files not created**:
   - Ensure the directories have write permissions
   - Check that the test executables are built and accessible

3. **Build failures**:
   - Ensure all dependencies (Catch2, CMake) are properly installed
   - Check that the C++20 compiler is available

### Log File Locations

- **Console logs**: `test-logs/[TestName]_console_[timestamp].log`
- **Detailed logs**: `logs/[TestName]_[timestamp].log`
- **CTest logs**: `test-logs/CTest_All_[timestamp].log`

### Viewing Logs

```powershell
# View most recent console log
Get-Content (Get-ChildItem test-logs\*console*.log | Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName

# View most recent detailed log
Get-Content (Get-ChildItem logs\*.log | Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName

# Search for specific test results
Select-String "FAIL" test-logs\*.log
Select-String "PASS" logs\*.log
```

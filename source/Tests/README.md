# Scenery Editor X Reference Counting Tests

This directory contains unit tests for the reference counting system used in the Scenery Editor X project.

## Test Files

- **RefTest.cpp**: Tests for basic functionality of the `Ref<T>` smart pointer class
- **WeakRefTest.cpp**: Tests for the `WeakRef<T>` weak reference implementation
- **RefPerformanceTest.cpp**: Performance and stress tests for reference counting
- **RefThreadSafetyTest.cpp**: Tests for thread safety of the reference counting system

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

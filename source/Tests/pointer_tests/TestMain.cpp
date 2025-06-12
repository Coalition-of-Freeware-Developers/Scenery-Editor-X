/**
* -------------------------------------------------------
* Scenery Editor X - Unit Tests
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* TestMain.cpp
* -------------------------------------------------------
* Main entry point for running the tests
* -------------------------------------------------------
*/
#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

// This file defines the main entry point for the test runner.
// No tests should be defined here.

// Tests are defined in the following files:
//  - RefTest.cpp           - Basic functionality tests for Ref<T>
//  - WeakRefTest.cpp       - Basic functionality tests for WeakRef<T>
//  - RefPerformanceTest.cpp   - Performance and stress tests for reference counting
//  - RefThreadSafetyTest.cpp  - Thread safety tests for reference counting

// To run specific tests, use the command line arguments:
// [test executable] [tag expression]
//
// Examples:
// Run all tests:
//   RefTests
//
// Run only Ref tests:
//   RefTests "[Ref]"
//
// Run only WeakRef tests:
//   RefTests "[WeakRef]"
//
// Run performance tests:
//   RefTests "[performance]"
//
// Run thread safety tests:
//   RefTests "[thread]"
//
// Exclude performance tests:
//   RefTests "~[performance]"

/**
* -------------------------------------------------------
* Scenery Editor X - edX Format Tests
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* EdxTestMain.cpp
* -------------------------------------------------------
* Main entry point for edX format testing
* -------------------------------------------------------
*/

#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

// This file defines the main entry point for the edX format test runner.
// No tests should be defined here.

// Tests are defined in the following files:
//  - EdxProjectFileTest.cpp    - Tests for edX project file operations
//  - EdxLibraryFileTest.cpp    - Tests for edX library file operations
//  - EdxManagerTest.cpp        - Tests for EdxManager high-level API
//  - EdxSerializationTest.cpp  - Tests for JSON serialization/deserialization
//  - EdxValidationTest.cpp     - Tests for data validation
//  - EdxIntegrationTest.cpp    - Integration tests and file generation

// To run specific tests, use the command line arguments:
// [test executable] [tag expression]
//
// Examples:
// Run all tests:
//   EdxTests
//
// Run only project file tests:
//   EdxTests "[project]"
//
// Run only library tests:
//   EdxTests "[library]"
//
// Run performance tests:
//   EdxTests "[performance]"
//
// Run integration tests:
//   EdxTests "[integration]"
//
// Exclude performance tests:
//   EdxTests "~[performance]"
//
// Run tests that create files for inspection:
//   EdxTests "[file-generation]"

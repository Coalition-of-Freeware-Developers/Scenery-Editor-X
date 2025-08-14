/**
* -------------------------------------------------------
* Scenery Editor X - Unit Tests
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* TestMain.cpp
* -------------------------------------------------------
* Main entry point for smart pointer tests
* -------------------------------------------------------
*/
#include <catch2/catch_all.hpp>
#include <iomanip>
#include <sstream>
#include "../TestLogger.h"

/// -------------------------------------------------------------------

// Initialize test logging when the program starts
struct TestLogInitializer
{
    TestLogInitializer()
    {
        // Initialize test logger with timestamp in filename
        const auto now = std::chrono::system_clock::now();
        const auto time_t = std::chrono::system_clock::to_time_t(now);

        std::stringstream logFileName;
        logFileName << "RefTests_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") << ".log";

        TestUtils::TestLogger::GetInstance().Initialize(logFileName.str(), true);
        TestUtils::TestLogger::GetInstance().Log(TestUtils::LogLevel::Info, "MAIN",
            "=== Scenery Editor X - Reference Counting Tests ===");
        TestUtils::TestLogger::GetInstance().Log(TestUtils::LogLevel::Info, "MAIN",
            "Test executable: RefTests");
        TestUtils::TestLogger::GetInstance().Log(TestUtils::LogLevel::Info, "MAIN",
            "Log file: logs/{}", logFileName.str());
    }

    ~TestLogInitializer()
    {
        TestUtils::TestLogger::GetInstance().Log(TestUtils::LogLevel::Info, "MAIN",
            "Test execution completed. Check log file for detailed results.");
        TestUtils::TestLogger::GetInstance().Shutdown();
    }
};

/// -------------------------------------------------------------------

///< Global instance to ensure initialization
static TestLogInitializer g_testLogInit;

///< This file defines the main entry point for the smart pointer test runner.
///< No tests should be defined here.

///< Tests are defined in the following files:
///<  - RefTest.cpp                - Basic functionality tests for Ref<T>
///<  - WeakRefTest.cpp           - Basic functionality tests for WeakRef<T>
///<  - RefPerformanceTest.cpp    - Performance and stress tests
///<  - RefThreadSafetyTest.cpp   - Thread safety validation tests
///<  - RefPerformanceTest.cpp   - Performance and stress tests for reference counting
///<  - RefThreadSafetyTest.cpp  - Thread safety tests for reference counting

///< To run specific tests, use the command line arguments:
///< [test executable] [tag expression]
///<
///< Examples:
///< Run all tests:
///<   RefTests
///<
///< Run only Ref tests:
///<   RefTests "[Ref]"
///<
///< Run only WeakRef tests:
///<   RefTests "[WeakRef]"
///<
///< Run performance tests:
///<   RefTests "[performance]"
///<
///< Run thread safety tests:
///<   RefTests "[thread]"
///<
///< Exclude performance tests:
///<   RefTests "~[performance]"

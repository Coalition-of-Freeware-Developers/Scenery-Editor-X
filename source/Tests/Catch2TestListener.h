/**
* -------------------------------------------------------
* Scenery Editor X - Test System
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* Catch2TestListener.h
* -------------------------------------------------------
* Event listener for Catch2 that automatically logs test events
* -------------------------------------------------------
*/
#pragma once

#include <catch2/catch_all.hpp>
#include "TestLogger.h"
#include <string>
#include <chrono>

namespace TestUtils
{
    /**
     * @brief Custom Catch2 event listener that logs test events
     *
     * This listener automatically captures all test events from Catch2
     * and logs them using our TestLogger system. It provides detailed
     * information about test execution without requiring manual logging
     * in test cases.
     */
    class Catch2TestListener : public Catch::EventListenerBase
    {
    public:
        using EventListenerBase::EventListenerBase;

        void testRunStarting(Catch::TestRunInfo const& testRunInfo) override
        {
            TestLogger::GetInstance().Log(LogLevel::Info, "TEST_RUN",
                "Test run starting - Catch2 v{}", CATCH_VERSION_STRING);
            TestLogger::GetInstance().Log(LogLevel::Info, "TEST_RUN",
                "Command line: {}", testRunInfo.name);
            TestLogger::GetInstance().Log(LogLevel::Info, "TEST_RUN", std::string(100, '='));

            m_TestRunStartTime = std::chrono::steady_clock::now();
            m_TotalTests = 0;
            m_PassedTests = 0;
            m_FailedTests = 0;
        }

        void testRunEnded(Catch::TestRunStats const& testRunStats) override
        {
            auto endTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                endTime - m_TestRunStartTime);

            TestLogger::GetInstance().Log(LogLevel::Info, "TEST_RUN", std::string(100, '='));
            TestLogger::GetInstance().Log(LogLevel::Info, "TEST_RUN",
                "Test run completed in {}ms", duration.count());
            TestLogger::GetInstance().Log(LogLevel::Info, "TEST_RUN",
                "Total tests: {}", m_TotalTests);
            TestLogger::GetInstance().Log(LogLevel::Info, "TEST_RUN",
                "Passed: {}", m_PassedTests);
            TestLogger::GetInstance().Log(LogLevel::Info, "TEST_RUN",
                "Failed: {}", m_FailedTests);

            if (testRunStats.totals.testCases.failed > 0)
            {
                TestLogger::GetInstance().Log(LogLevel::Error, "TEST_RUN",
                    "TEST RUN FAILED - {} test(s) failed", testRunStats.totals.testCases.failed);
            }
            else
            {
                TestLogger::GetInstance().Log(LogLevel::Info, "TEST_RUN",
                    "TEST RUN PASSED - All tests successful");
            }
        }

        void testCaseStarting(Catch::TestCaseInfo const& testInfo) override
        {
            m_CurrentTestName = testInfo.name;
            m_TestStartTime = std::chrono::steady_clock::now();

            TestLogger::GetInstance().Log(LogLevel::Info, "TEST_CASE",
                "Starting: {}", testInfo.name);

            if (!testInfo.tags.empty())
            {
                std::string tagsStr;
                for (const auto& tag : testInfo.tags)
                {
                    if (!tagsStr.empty()) tagsStr += ", ";
                    tagsStr += tag;
                }
                TestLogger::GetInstance().Log(LogLevel::Info, "TEST_CASE",
                    "Tags: {}", tagsStr);
            }

            if (!testInfo.description.empty())
            {
                TestLogger::GetInstance().Log(LogLevel::Info, "TEST_CASE",
                    "Description: {}", testInfo.description);
            }

            TestLogger::GetInstance().Log(LogLevel::Info, "TEST_CASE",
                "Location: {}:{}", testInfo.lineInfo.file, testInfo.lineInfo.line);
            TestLogger::GetInstance().Log(LogLevel::Info, "TEST_CASE", std::string(80, '-'));

            m_TotalTests++;
        }

        void testCaseEnded(Catch::TestCaseStats const& testCaseStats) override
        {
            auto endTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                endTime - m_TestStartTime);

            bool passed = testCaseStats.totals.testCases.passed > 0 &&
                         testCaseStats.totals.testCases.failed == 0;

            if (passed)
            {
                m_PassedTests++;
                TestLogger::GetInstance().Log(LogLevel::Info, "TEST_CASE",
                    "PASSED: {} ({}ms)", m_CurrentTestName, duration.count());
            }
            else
            {
                m_FailedTests++;
                TestLogger::GetInstance().Log(LogLevel::Error, "TEST_CASE",
                    "FAILED: {} ({}ms)", m_CurrentTestName, duration.count());
            }

            // Log assertion summary
            if (testCaseStats.totals.assertions.total > 0)
            {
                TestLogger::GetInstance().Log(LogLevel::Info, "TEST_CASE",
                    "Assertions - Total: {}, Passed: {}, Failed: {}",
                    testCaseStats.totals.assertions.total,
                    testCaseStats.totals.assertions.passed,
                    testCaseStats.totals.assertions.failed);
            }

            TestLogger::GetInstance().Log(LogLevel::Info, "TEST_CASE", std::string(80, '-'));
            TestLogger::GetInstance().Log(LogLevel::Info, "TEST_CASE", ""); // Empty line
        }

        void sectionStarting(Catch::SectionInfo const& sectionInfo) override
        {
            m_SectionDepth++;
            std::string indent(m_SectionDepth * 2, ' ');
            TestLogger::GetInstance().Log(LogLevel::Info, "SECTION",
                "{}Starting section: {}", indent, sectionInfo.name);

            if (!sectionInfo.description.empty())
            {
                TestLogger::GetInstance().Log(LogLevel::Info, "SECTION",
                    "{}Description: {}", indent, sectionInfo.description);
            }
        }

        void sectionEnded(Catch::SectionStats const& sectionStats) override
        {
            std::string indent(m_SectionDepth * 2, ' ');
            bool passed = sectionStats.sectionInfo.capturedStdOut.empty() &&
                         sectionStats.sectionInfo.capturedStdErr.empty();

            TestLogger::GetInstance().Log(LogLevel::Info, "SECTION",
                "{}Completed section: {}", indent, sectionStats.sectionInfo.name);

            m_SectionDepth--;
        }

        void assertionStarting(Catch::AssertionInfo const& assertionInfo) override
        {
            // We can log assertion details if needed
            if (assertionInfo.macroName == "REQUIRE" || assertionInfo.macroName == "CHECK")
            {
                TestLogger::GetInstance().Log(LogLevel::Debug, "ASSERTION",
                    "Evaluating: {} at {}:{}",
                    assertionInfo.capturedExpression,
                    assertionInfo.lineInfo.file,
                    assertionInfo.lineInfo.line);
            }
        }

        void assertionEnded(Catch::AssertionStats const& assertionStats) override
        {
            const auto& result = assertionStats.assertionResult;

            if (result.isOk())
            {
                TestLogger::GetInstance().Log(LogLevel::Info, "ASSERTION",
                    "PASS: {} -> {}",
                    result.getExpression(),
                    result.getExpandedExpression());
            }
            else
            {
                TestLogger::GetInstance().Log(LogLevel::Error, "ASSERTION",
                    "FAIL: {} -> {}",
                    result.getExpression(),
                    result.getExpandedExpression());

                if (!result.getMessage().empty())
                {
                    TestLogger::GetInstance().Log(LogLevel::Error, "ASSERTION",
                        "Message: {}", result.getMessage());
                }

                TestLogger::GetInstance().Log(LogLevel::Error, "ASSERTION",
                    "Location: {}:{}",
                    result.getSourceInfo().file,
                    result.getSourceInfo().line);
            }
        }

        void fatalErrorEncountered(Catch::StringRef error) override
        {
            TestLogger::GetInstance().Log(LogLevel::Fatal, "FATAL_ERROR",
                "Fatal error encountered: {}", error);
        }

    private:
        std::chrono::steady_clock::time_point m_TestRunStartTime;
        std::chrono::steady_clock::time_point m_TestStartTime;
        std::string m_CurrentTestName;
        int m_SectionDepth = 0;
        int m_TotalTests = 0;
        int m_PassedTests = 0;
        int m_FailedTests = 0;
    };

    /**
     * @brief Register the test listener with Catch2
     *
     * Call this macro at the beginning of your test main function
     * to automatically register the logging event listener.
     */
    #define REGISTER_TEST_LISTENER() \
        CATCH_REGISTER_LISTENER(TestUtils::Catch2TestListener)

} // namespace TestUtils

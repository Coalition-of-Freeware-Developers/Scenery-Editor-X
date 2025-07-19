/**
* -------------------------------------------------------
* Scenery Editor X - Catch2 Test Event Listener
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* TestListener.h
* -------------------------------------------------------
* Catch2 event listener for automatic test logging
* -------------------------------------------------------
*/
#pragma once

#include <catch2/catch_test_macros.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/catch_test_case_info.hpp>
#include <catch2/catch_section_info.hpp>
#include <catch2/catch_assertion_result.hpp>
#include "TestLogger.h"

namespace TestUtils
{
    /**
     * @brief Catch2 event listener that automatically logs test events
     *
     * This listener integrates with Catch2's event system to automatically
     * log test starts, ends, sections, and assertion results to our test logger.
     */
    class TestEventListener : public Catch::EventListenerBase
    {
    public:
        using EventListenerBase::EventListenerBase;

        /**
         * @brief Called when test run starts
         */
        void testRunStarting(Catch::TestRunInfo const& testRunInfo) override
        {
            // Initialize logger with test suite name
            std::string logFileName = "test_run_" + GetTimestamp() + ".log";
            TestLogger::GetInstance().Initialize(logFileName, true);

            TEST_LOG_INFO("TEST_RUN", "========================================");
            TEST_LOG_INFO("TEST_RUN", "Test Run Starting");
            TEST_LOG_INFO("TEST_RUN", "Test executable: {}", testRunInfo.name);
            TEST_LOG_INFO("TEST_RUN", "========================================");
        }

        /**
         * @brief Called when test run ends
         */
        void testRunEnded(Catch::TestRunStats const& testRunStats) override
        {
            TEST_LOG_INFO("TEST_RUN", "========================================");
            TEST_LOG_INFO("TEST_RUN", "Test Run Completed");
            TEST_LOG_INFO("TEST_RUN", "Total Tests: {}", testRunStats.totals.testCases.total());
            TEST_LOG_INFO("TEST_RUN", "Passed: {}", testRunStats.totals.testCases.passed);
            TEST_LOG_INFO("TEST_RUN", "Failed: {}", testRunStats.totals.testCases.failed);
            TEST_LOG_INFO("TEST_RUN", "Total Assertions: {}", testRunStats.totals.assertions.total());
            TEST_LOG_INFO("TEST_RUN", "Passed Assertions: {}", testRunStats.totals.assertions.passed);
            TEST_LOG_INFO("TEST_RUN", "Failed Assertions: {}", testRunStats.totals.assertions.failed);
            TEST_LOG_INFO("TEST_RUN", "========================================");

            TestLogger::GetInstance().Shutdown();
        }

        /**
         * @brief Called when a test case starts
         */
        void testCaseStarting(Catch::TestCaseInfo const& testInfo) override
        {
            m_CurrentTestName = testInfo.name;

            TEST_LOG_INFO("TEST_CASE", "Starting test case: {}", testInfo.name);
            TEST_LOG_DEBUG("TEST_CASE", "Description: {}", testInfo.description);
            TEST_LOG_DEBUG("TEST_CASE", "Tags: {}", testInfo.tagsAsString());
            TEST_LOG_DEBUG("TEST_CASE", "Source file: {}:{}", testInfo.lineInfo.file, testInfo.lineInfo.line);
            TEST_LOG_INFO("TEST_CASE", "----------------------------------------");
        }

        /**
         * @brief Called when a test case ends
         */
        void testCaseEnded(Catch::TestCaseStats const& testCaseStats) override
        {
            bool testPassed = testCaseStats.totals.assertions.allPassed();

            TEST_LOG_INFO("TEST_CASE", "----------------------------------------");
            TEST_LOG_INFO("TEST_CASE", "Test case ended: {}", m_CurrentTestName);
            TEST_LOG_INFO("TEST_CASE", "Result: {}", testPassed ? "PASSED" : "FAILED");
            TEST_LOG_INFO("TEST_CASE", "Assertions - Total: {}, Passed: {}, Failed: {}",
                         testCaseStats.totals.assertions.total(),
                         testCaseStats.totals.assertions.passed,
                         testCaseStats.totals.assertions.failed);

            if (testCaseStats.stdOut.size() > 0)
            {
                TEST_LOG_DEBUG("TEST_CASE", "Standard Output: {}", testCaseStats.stdOut);
            }
            if (testCaseStats.stdErr.size() > 0)
            {
                TEST_LOG_ERROR("TEST_CASE", "Standard Error: {}", testCaseStats.stdErr);
            }

            TEST_LOG_INFO("TEST_CASE", "");
        }

        /**
         * @brief Called when a section starts
         */
        void sectionStarting(Catch::SectionInfo const& sectionInfo) override
        {
            TEST_LOG_INFO("SECTION", "Section: {}", sectionInfo.name);
            if (!sectionInfo.description.empty())
            {
                TEST_LOG_DEBUG("SECTION", "Description: {}", sectionInfo.description);
            }
        }

        /**
         * @brief Called when a section ends
         */
        void sectionEnded(Catch::SectionStats const& sectionStats) override
        {
            bool sectionPassed = sectionStats.assertions.allPassed();
            TEST_LOG_DEBUG("SECTION", "Section '{}' ended - {}",
                          sectionStats.sectionInfo.name,
                          sectionPassed ? "PASSED" : "FAILED");
        }

        /**
         * @brief Called when an assertion is made
         */
        void assertionEnded(Catch::AssertionStats const& assertionStats) override
        {
            const auto& result = assertionStats.assertionResult;

            if (result.succeeded())
            {
                TEST_LOG_DEBUG("ASSERTION", "PASS: {} at {}:{}",
                              result.getExpressionInMacro(),
                              result.getSourceInfo().file,
                              result.getSourceInfo().line);
            }
            else
            {
                TEST_LOG_ERROR("ASSERTION", "FAIL: {} at {}:{}",
                              result.getExpressionInMacro(),
                              result.getSourceInfo().file,
                              result.getSourceInfo().line);

                if (result.hasMessage())
                {
                    TEST_LOG_ERROR("ASSERTION", "Message: {}", result.getMessage());
                }

                if (result.hasExpandedExpression() &&
                    result.getExpandedExpression() != result.getExpressionInMacro())
                {
                    TEST_LOG_ERROR("ASSERTION", "Expanded: {}", result.getExpandedExpression());
                }
            }
        }

        /**
         * @brief Called when a fatal error condition occurs
         */
        void fatalErrorEncountered(Catch::StringRef error) override
        {
            TEST_LOG_FATAL("FATAL_ERROR", "Fatal error encountered: {}", error);
        }

    private:
        /**
         * @brief Get current timestamp for log file naming
         */
        std::string GetTimestamp()
        {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);

            std::stringstream ss;
            ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
            return ss.str();
        }

        std::string m_CurrentTestName;
    };

} // namespace TestUtils

// Register the listener with Catch2
CATCH_REGISTER_LISTENER(TestUtils::TestEventListener)

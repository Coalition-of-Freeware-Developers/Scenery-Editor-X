#include "SimpleTestHelper.h"
#include "../pointer_tests/RefTest.cpp"

/**
 * @file LoggingIntegrationExample.cpp
 * @brief Example demonstrating how to integrate test logging with existing Catch2 tests
 *
 * This file shows how to use the TestLogger and SimpleTestHelper utilities
 * to add logging to your Catch2 tests for better output tracking and debugging.
 */

namespace LoggingExamples
{
    using namespace PointerTests;

    TEST_CASE("Logged Ref Basic Operations Example", "[logging][example][ref]")
    {
        // Initialize test logging - this sets up the log file and console output
        INIT_TEST_LOGGING("test_output.log", true);

        // Use TEST_CASE_LOG for automatic test case lifecycle logging
        TEST_CASE_LOG();

        SECTION("Creating and using Ref objects")
        {
            // Use SECTION_LOG for automatic section logging
            SECTION_LOG();

            // Log test information
            LOG_TEST_INFO("Creating test object with value 42");
            auto* rawPtr = new TestObject(42);

            LOG_TEST_INFO("Creating Ref from raw pointer");
            Ref<TestObject> ref(rawPtr);

            // Log assertions with context
            LOG_ASSERTION("Ref should be valid", ref.IsValid());
            REQUIRE(ref.IsValid());

            LOG_ASSERTION("Ref should have use count of 1", ref.UseCount() == 1);
            REQUIRE(ref.UseCount() == 1);

            LOG_ASSERTION("Ref should contain correct value", ref->GetValue() == 42);
            REQUIRE(ref->GetValue() == 42);

            LOG_TEST_INFO("Test object operations completed successfully");
        }

        SECTION("Copy construction and reference counting")
        {
            SECTION_LOG();

            LOG_TEST_INFO("Testing copy construction and reference counting");

            auto originalRef = CreateRef<TestObject>(100);
            LOG_TEST_INFO("Created original ref with use count: " + std::to_string(originalRef.UseCount()));

            {
                LOG_TEST_INFO("Creating copy of ref");
                auto copiedRef = originalRef;

                LOG_ASSERTION("Both refs should be valid", originalRef.IsValid() && copiedRef.IsValid());
                REQUIRE(originalRef.IsValid());
                REQUIRE(copiedRef.IsValid());

                LOG_ASSERTION("Use count should be 2", originalRef.UseCount() == 2);
                REQUIRE(originalRef.UseCount() == 2);
                REQUIRE(copiedRef.UseCount() == 2);

                LOG_TEST_INFO("Copy ref going out of scope");
            }

            LOG_ASSERTION("Use count should return to 1", originalRef.UseCount() == 1);
            REQUIRE(originalRef.UseCount() == 1);

            LOG_TEST_INFO("Reference counting test completed successfully");
        }

        SECTION("Error handling and edge cases")
        {
            SECTION_LOG();

            LOG_TEST_WARN("Testing edge cases and potential error conditions");

            // Test null reference behavior
            Ref<TestObject> nullRef;
            LOG_ASSERTION("Null ref should not be valid", !nullRef.IsValid());
            REQUIRE_FALSE(nullRef.IsValid());

            // Test reset behavior
            auto ref = CreateRef<TestObject>(999);
            LOG_TEST_INFO("Created ref, now testing reset");
            ref.Reset();

            LOG_ASSERTION("Reset ref should not be valid", !ref.IsValid());
            REQUIRE_FALSE(ref.IsValid());

            LOG_TEST_INFO("Edge case testing completed");
        }

        LOG_TEST_INFO("All test sections completed successfully");
    }

    TEST_CASE("Performance Timing Example", "[logging][example][performance]")
    {
        INIT_TEST_LOGGING("performance_test.log", true);
        TEST_CASE_LOG();

        SECTION("Measuring Ref operations performance")
        {
            SECTION_LOG();

            const int iterations = 10000;
            LOG_TEST_INFO("Starting performance test with " + std::to_string(iterations) + " iterations");

            auto start = std::chrono::high_resolution_clock::now();

            // Create many Ref objects
            std::vector<Ref<TestObject>> refs;
            refs.reserve(iterations);

            for (int i = 0; i < iterations; ++i)
            {
                refs.push_back(CreateRef<TestObject>(i));
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            LOG_TEST_INFO("Created " + std::to_string(iterations) + " Ref objects in " +
                         std::to_string(duration.count()) + " microseconds");

            // Verify all objects were created successfully
            bool allValid = true;
            for (const auto& ref : refs)
            {
                if (!ref.IsValid())
                {
                    allValid = false;
                    break;
                }
            }

            LOG_ASSERTION("All Ref objects should be valid", allValid);
            REQUIRE(allValid);

            LOG_TEST_INFO("Performance test completed - average time per Ref: " +
                         std::to_string(duration.count() / static_cast<double>(iterations)) + " microseconds");
        }
    }

    TEST_CASE("Error Scenario Example", "[logging][example][error]")
    {
        INIT_TEST_LOGGING("error_test.log", true);
        TEST_CASE_LOG();

        SECTION("Intentional error for logging demonstration")
        {
            SECTION_LOG();

            LOG_TEST_WARN("This section demonstrates error logging");

            try
            {
                // Simulate some operation that might fail
                auto ref = CreateRef<TestObject>(42);

                // Intentionally cause a scenario that might be problematic
                if (ref->GetValue() != 999)  // This will be true
                {
                    LOG_TEST_ERROR("Value mismatch detected - expected 999, got " +
                                  std::to_string(ref->GetValue()));

                    // Note: In a real test, you might use REQUIRE here, but for demonstration
                    // we'll just log the issue
                    LOG_TEST_INFO("Continuing test despite value mismatch for demonstration");
                }

                LOG_TEST_INFO("Error scenario demonstration completed");
            }
            catch (const std::exception& e)
            {
                LOG_TEST_ERROR("Exception caught: " + std::string(e.what()));
                throw; // Re-throw to fail the test
            }
        }
    }
}

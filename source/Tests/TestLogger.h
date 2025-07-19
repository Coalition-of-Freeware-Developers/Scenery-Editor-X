/**
* -------------------------------------------------------
* Scenery Editor X - Test Logging System
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* TestLogger.h
* -------------------------------------------------------
* Lightweight logging system for unit tests
* -------------------------------------------------------
*/
#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <memory>
#include <mutex>

// Cross-platform filesystem support
#if defined(_WIN32)
    #include <direct.h>
    #include <windows.h>
    #define MKDIR(path) _mkdir(path)
#else
    #include <sys/stat.h>
    #define MKDIR(path) mkdir(path, 0777)
#endif

namespace TestUtils
{
    /**
     * @brief Log levels for test output
     */
    enum class LogLevel
    {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Fatal
    };

    /**
     * @brief Simple test logger that outputs to both console and file
     *
     * This is a lightweight logging system specifically designed for tests.
     * It doesn't depend on the main application framework and can be used
     * standalone in test executables.
     */
    class TestLogger
    {
    public:
        /**
         * @brief Get the singleton instance of the test logger
         */
        static TestLogger& GetInstance()
        {
            static TestLogger instance;
            return instance;
        }

        /**
         * @brief Initialize the logger with a specific log file
         * @param logFileName Name of the log file (will be created in logs/ directory)
         * @param enableConsole Whether to also output to console
         */
        void Initialize(const std::string& logFileName, bool enableConsole = true)
        {
            std::lock_guard<std::mutex> lock(m_Mutex);

            m_EnableConsole = enableConsole;

            // Create logs directory if it doesn't exist
            std::string logDir = "logs";
            MKDIR(logDir.c_str());

            // Open log file
            std::string logPath = logDir + "/" + logFileName;
            m_LogFile.open(logPath, std::ios::out | std::ios::trunc);

            if (m_LogFile.is_open())
            {
                m_IsInitialized = true;
                LogInternal(LogLevel::Info, "Test Logger", "Logger initialized - Log file: " + logPath);
            }
            else
            {
                std::cerr << "Failed to open log file: " << logPath << std::endl;
            }
        }

        /**
         * @brief Log a message with specified level
         */
        template<typename... Args>
        void Log(LogLevel level, const std::string& category, const std::string& format, Args&&... args)
        {
            if (!m_IsInitialized) return;

            std::string message = FormatString(format, std::forward<Args>(args)...);
            LogInternal(level, category, message);
        }

        /**
         * @brief Log test start
         */
        void LogTestStart(const std::string& testName)
        {
            LogInternal(LogLevel::Info, "TEST_START", "Starting test: " + testName);
            LogInternal(LogLevel::Info, "TEST_START", std::string(80, '='));
        }

        /**
         * @brief Log test end with result
         */
        void LogTestEnd(const std::string& testName, bool passed)
        {
            LogInternal(LogLevel::Info, "TEST_END", std::string(80, '='));
            LogInternal(passed ? LogLevel::Info : LogLevel::Error,
                       "TEST_END",
                       "Test " + testName + " " + (passed ? "PASSED" : "FAILED"));
            LogInternal(LogLevel::Info, "TEST_END", "");
        }

        /**
         * @brief Log test section
         */
        void LogSection(const std::string& sectionName)
        {
            LogInternal(LogLevel::Info, "SECTION", "--- " + sectionName + " ---");
        }

        /**
         * @brief Shutdown the logger
         */
        void Shutdown()
        {
            std::lock_guard<std::mutex> lock(m_Mutex);

            if (m_IsInitialized)
            {
                LogInternal(LogLevel::Info, "Test Logger", "Logger shutting down");
                m_LogFile.close();
                m_IsInitialized = false;
            }
        }

    private:
        TestLogger() = default;
        ~TestLogger() { Shutdown(); }

        // Delete copy constructor and assignment
        TestLogger(const TestLogger&) = delete;
        TestLogger& operator=(const TestLogger&) = delete;

        /**
         * @brief Format string with arguments (simple implementation)
         */
        template<typename... Args>
        std::string FormatString(const std::string& format, Args&&... args)
        {
            std::ostringstream oss;
            FormatHelper(oss, format, std::forward<Args>(args)...);
            return oss.str();
        }

        // Specialization for no arguments
        std::string FormatString(const std::string& format)
        {
            return format;
        }

        /**
         * @brief Helper for string formatting
         */
        template<typename T, typename... Args>
        void FormatHelper(std::ostringstream& oss, const std::string& format, T&& value, Args&&... args)
        {
            size_t pos = format.find("{}");
            if (pos != std::string::npos)
            {
                oss << format.substr(0, pos) << value;
                FormatHelper(oss, format.substr(pos + 2), std::forward<Args>(args)...);
            }
            else
            {
                oss << format;
            }
        }

        void FormatHelper(std::ostringstream& oss, const std::string& format)
        {
            oss << format;
        }

        /**
         * @brief Internal logging implementation
         */
        void LogInternal(LogLevel level, const std::string& category, const std::string& message)
        {
            std::lock_guard<std::mutex> lock(m_Mutex);

            if (!m_IsInitialized) return;

            // Get current time
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;

            // Format timestamp
            std::stringstream timeStr;
            timeStr << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
            timeStr << "." << std::setfill('0') << std::setw(3) << ms.count();

            // Format log level
            std::string levelStr = LogLevelToString(level);

            // Create formatted message
            std::string formattedMessage = "[" + timeStr.str() + "] [" + levelStr + "] [" + category + "] " + message;

            // Output to file
            if (m_LogFile.is_open())
            {
                m_LogFile << formattedMessage << std::endl;
                m_LogFile.flush();
            }

            // Output to console if enabled
            if (m_EnableConsole)
            {
                if (level >= LogLevel::Error)
                {
                    std::cerr << formattedMessage << std::endl;
                }
                else
                {
                    std::cout << formattedMessage << std::endl;
                }
            }
        }

        /**
         * @brief Convert log level to string
         */
        std::string LogLevelToString(LogLevel level)
        {
            switch (level)
            {
                case LogLevel::Trace:   return "TRACE";
                case LogLevel::Debug:   return "DEBUG";
                case LogLevel::Info:    return "INFO ";
                case LogLevel::Warning: return "WARN ";
                case LogLevel::Error:   return "ERROR";
                case LogLevel::Fatal:   return "FATAL";
                default:                return "UNKNW";
            }
        }

        std::ofstream m_LogFile;
        bool m_EnableConsole = true;
        bool m_IsInitialized = false;
        std::mutex m_Mutex;
    };

    // Convenience macros for logging
    #define TEST_LOG_TRACE(category, message, ...) \
        TestUtils::TestLogger::GetInstance().Log(TestUtils::LogLevel::Trace, category, message, ##__VA_ARGS__)

    #define TEST_LOG_DEBUG(category, message, ...) \
        TestUtils::TestLogger::GetInstance().Log(TestUtils::LogLevel::Debug, category, message, ##__VA_ARGS__)

    #define TEST_LOG_INFO(category, message, ...) \
        TestUtils::TestLogger::GetInstance().Log(TestUtils::LogLevel::Info, category, message, ##__VA_ARGS__)

    #define TEST_LOG_WARN(category, message, ...) \
        TestUtils::TestLogger::GetInstance().Log(TestUtils::LogLevel::Warning, category, message, ##__VA_ARGS__)

    #define TEST_LOG_ERROR(category, message, ...) \
        TestUtils::TestLogger::GetInstance().Log(TestUtils::LogLevel::Error, category, message, ##__VA_ARGS__)

    #define TEST_LOG_FATAL(category, message, ...) \
        TestUtils::TestLogger::GetInstance().Log(TestUtils::LogLevel::Fatal, category, message, ##__VA_ARGS__)

    // Test lifecycle macros
    #define TEST_START(testName) \
        TestUtils::TestLogger::GetInstance().LogTestStart(testName)

    #define TEST_END(testName, passed) \
        TestUtils::TestLogger::GetInstance().LogTestEnd(testName, passed)

    #define TEST_SECTION(sectionName) \
        TestUtils::TestLogger::GetInstance().LogSection(sectionName)

    /**
     * @brief RAII wrapper for test lifecycle logging
     */
    class TestLifecycleLogger
    {
    public:
        TestLifecycleLogger(const std::string& testName)
            : m_TestName(testName), m_Success(false)
        {
            TEST_START(m_TestName);
        }

        ~TestLifecycleLogger()
        {
            TEST_END(m_TestName, m_Success);
        }

        void SetSuccess(bool success) { m_Success = success; }

    private:
        std::string m_TestName;
        bool m_Success;
    };

    #define TEST_LIFECYCLE(testName) \
        TestUtils::TestLifecycleLogger __test_lifecycle(testName); \
        bool __test_success = true

    #define TEST_LIFECYCLE_SUCCESS() \
        __test_lifecycle.SetSuccess(__test_success)

    #define TEST_LIFECYCLE_FAIL() \
        __test_success = false; \
        __test_lifecycle.SetSuccess(false)

} // namespace TestUtils

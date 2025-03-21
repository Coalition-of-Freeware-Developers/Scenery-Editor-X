#include "Logging.hpp"
#include <core/version.h>

#include <string>
#include <sysinfoapi.h>

#include <memory>
#include <minwinbase.h>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_color_sinks-inl.h>
#include <spdlog/spdlog.h>
#include <spdlog/spdlog-inl.h>
#include <timezoneapi.h>
#include <vector>

/**
 * @brief Static member to hold the logger instance.
 */
std::shared_ptr<spdlog::logger> Log::_EditorLogger;

/**
 * @brief Initializes the logging system with console and file sinks.
 * 
 * This method sets up the logging system to output logs to both the console
 * and a file named "SceneryEditorX.log". It configures the log patterns for
 * each sink and sets the logging level to trace. The logger is also set to
 * flush on every trace level log entry.
 */
void Log::Init()
{
    if (!_EditorLogger) // Only initialize the logger if it hasn't been initialized yet
    {
        _EditorLogger = spdlog::stdout_color_mt("Logger"); // Create a console logger
        spdlog::set_level(spdlog::level::trace);                                    // Set global log level to trace
    }

    std::vector<spdlog::sink_ptr> sinks;                                            // create a vector of sink pointers

    sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());    // create a console sink

    /**
    * @brief Adds a file sink to the logger.
    * 
    * This line adds a file sink to the logger, which outputs log messages to a file named "SceneryEditorX.log".
    * The second parameter 'true' indicates that the log file should be created in append mode, meaning new log
    * messages will be added to the end of the file if it already exists.
    */
    sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("SceneryEditorX.log", true));


/**
 * @brief Sets the log pattern for the console and file sinks.
 * 
 * This line sets the log pattern for the console sink to include the time, logger name, and message,
 * with color formatting. The file sink pattern includes the time, log level, logger name, and message.
 * 
 * @param pattern The pattern to set for the log output.
 */
    sinks[0]->set_pattern("%^[%T] %n: %v%$");       // color coding
    sinks[1]->set_pattern("[%T] [%l] %n: %v");      // no color coding

    _EditorLogger = std::make_shared<spdlog::logger>("SceneryEditorX",
                                                     sinks.begin(),
                                                     sinks.end());                                        // create a logger with the sinks
    spdlog::register_logger(_EditorLogger);                                                                     // register the logger with spdlog
    _EditorLogger->set_level(spdlog::level::trace);                                                    // set the logging level to trace
    _EditorLogger->flush_on(spdlog::level::trace);                                                     // flush the logger on trace level log entries
}

/**
 * @brief Logs a message with the trace level.
 * 
 * This method logs a message with the trace level. The message is output to the console and the log file.
 * 
 * @param message The message to log.
 */
std::string getOsName()
{
#ifdef _WIN32
    return "Windows 32-bit";
#elif _WIN64
    return "Windows 64-bit";
#elif __APPLE__ || __MACH__
    return "Mac OSX";
#elif __linux__
    return "Linux";
#elif __unix || __unix__
    return "Unix";
#else
    return "Other";
#endif
}

void Log::LogHeader()
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    SYSTEMTIME systemTime;
    GetSystemTime(&systemTime);

    TIME_ZONE_INFORMATION timeZoneInfo;
    GetTimeZoneInformation(&timeZoneInfo);

    std::wstring timeZoneName =
        (timeZoneInfo.StandardName[0] != L'\0') ? timeZoneInfo.StandardName : timeZoneInfo.DaylightName;

    spdlog::info("============================================");
    spdlog::info("System Information");
    spdlog::info("Operating System: {}", getOsName());
    spdlog::info("{:02}:{:02}:{:02} {:02}/{:02}/{:04}",
                 systemTime.wHour,
                 systemTime.wMinute,
                 systemTime.wSecond,
                 systemTime.wDay,
                 systemTime.wMonth,
                 systemTime.wYear);
    spdlog::info("Time Zone: {}", std::string(timeZoneName.begin(), timeZoneName.end()));
    spdlog::info("Processor Architecture: {}", sysInfo.wProcessorArchitecture);
    spdlog::info("Processor Cores: {}", sysInfo.dwNumberOfProcessors);
    spdlog::info("Page Size: {}", sysInfo.dwPageSize);
    //spdlog::info("Processor Type: {}", sysInfo.dwProcessorType);
    //spdlog::info("Minimum Application Address: {}", sysInfo.lpMinimumApplicationAddress);
    //spdlog::info("Maximum Application Address: {}", sysInfo.lpMaximumApplicationAddress);
    //spdlog::info("Active Processor Mask: {}", sysInfo.dwActiveProcessorMask);
    spdlog::info("============================================");
    spdlog::info("============================================");
    spdlog::info("Scenery Editor X");
    spdlog::info("Version: {}.{}.{}.{}", SEDX_VER_MAJOR, SEDX_VER_MINOR, SEDX_VER_PATCH, SEDX_VER_BUILD);
    spdlog::info("Build Date: {}", __DATE__);
    spdlog::info("Build Time: {}", __TIME__);
    spdlog::info("Coalition of Freeware Developers");
    spdlog::info("Copyright (C) 2024");
    spdlog::info("============================================");
    spdlog::info("============================================");
}

void Log::Shutdown()
{
    spdlog::info("Shutting down logging system...");
    spdlog::shutdown();
}

#include "Logging.hpp"
#include <version.h>

#include <iostream>
#include <string>
#include <sysinfoapi.h>
#include <VersionHelpers.h>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

/**
 * @brief Static member to hold the logger instance.
 */
std::shared_ptr<spdlog::logger> Log::_logger;

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
    if (!_logger)
    {
        _logger = spdlog::stdout_color_mt("Logger");
        spdlog::set_level(spdlog::level::trace);
    }

    std::vector<spdlog::sink_ptr> sinks;

    sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("SceneryEditorX.log", true));

    sinks[0]->set_pattern("%^[%T] %n: %v%$");
    sinks[1]->set_pattern("[%T] [%l] %n: %v");

    _logger = std::make_shared<spdlog::logger>("SceneryEditorX", sinks.begin(), sinks.end());
    spdlog::register_logger(_logger);
    _logger->set_level(spdlog::level::trace);
    _logger->flush_on(spdlog::level::trace);
}

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
#elif __FreeBSD__
    return "FreeBSD";
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
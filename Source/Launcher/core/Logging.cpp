#include <Logging.hpp>
#include <core/version.h>

#include <codecvt>
#include <locale>
#include <string>
#include <sysinfoapi.h>

#include <memory>
#include <minwinbase.h>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>
#include <spdlog/spdlog-inl.h>
#include <timezoneapi.h>
#include <vector>
#include <iostream>
#include <filesystem>

std::atomic<bool> bWasInit = false;

/**
* @brief Initializes the logging system with console and file sinks.
* 
* This method sets up the logging system to output logs to both the console
* and a file named "Launcher.log". It configures the log patterns for
* each sink and sets the logging level to trace. The logger is also set to
* flush on every trace level log entry.
*/
void Log::Init()
{
    try
    {
        if (!bWasInit) // Only initialize once
        {
            bWasInit = true;
            // Create an asynchronous logger with a queue size of 8192
            spdlog::init_thread_pool(8192, 1);

            std::vector<spdlog::sink_ptr> sinks;

            // Console sink with color output
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_pattern("%^[%T] %n: %v%$");
            sinks.emplace_back(console_sink);

            //Get the path to write the file
#ifdef _WIN32
            char exePath[MAX_PATH]{0};
            GetModuleFileNameA(NULL, exePath, MAX_PATH);
            std::filesystem::path pLogPath = std::filesystem::path(exePath).parent_path() / "Launcher.log";
#else
            std::filesystem::path pLogPath = "Launcher.log";
#endif

            // File sink for writing logs
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(pLogPath.string(), true);
            file_sink->set_pattern("[%Y-%m-%d %T] [%l] %n: %v");
            sinks.emplace_back(file_sink);

            // Combine sinks into an asynchronous logger
            auto _logger = std::make_shared<spdlog::async_logger>("Launcher",sinks.begin(),sinks.end()
                ,spdlog::thread_pool(),
                spdlog::async_overflow_policy::block);

            // Set global log level and flush policy
            _logger->set_level(spdlog::level::trace);
            _logger->flush_on(spdlog::level::trace);
            spdlog::register_logger(_logger);
            spdlog::set_default_logger(_logger);

            // Debug message to verify initialization
            spdlog::info("Logger initialized successfully.");
        }
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        std::cout << "Logger initialization failed: " << ex.what() << std::endl;
    }
}

void Log::Shutdown()
{
    spdlog::info("Shutting down logging system...");
    std::cout << "Shutting down logging system..." << std::endl;
    spdlog::shutdown();
}


static std::string getOsName()
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
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::string timeZoneNameStr = converter.to_bytes(timeZoneName);
    spdlog::info("Time Zone: {}", timeZoneNameStr);
    spdlog::info("Processor Architecture: {}", sysInfo.wProcessorArchitecture);
    spdlog::info("Processor Cores: {}", sysInfo.dwNumberOfProcessors);
    spdlog::info("Page Size: {}", sysInfo.dwPageSize);
    //spdlog::info("Processor Type: {}", sysInfo.dwProcessorType);
    //spdlog::info("Minimum Application Address: {}", sysInfo.lpMinimumApplicationAddress);
    //spdlog::info("Maximum Application Address: {}", sysInfo.lpMaximumApplicationAddress);
    //spdlog::info("Active Processor Mask: {}", sysInfo.dwActiveProcessorMask);
    spdlog::info("============================================");
    spdlog::info("============================================");
    spdlog::info("Scenery Editor X | Launcher");
    spdlog::info("Version: {}.{}.{}.{}", SEDX_VER_MAJOR, SEDX_VER_MINOR, SEDX_VER_PATCH, SEDX_VER_BUILD);
    spdlog::info("Build Date: {}", __DATE__);
    spdlog::info("Build Time: {}", __TIME__);
    spdlog::info("Coalition of Freeware Developers");
    spdlog::info("Copyright (C) 2024");
    spdlog::info("============================================");
    spdlog::info("============================================");
}

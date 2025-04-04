/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* logging.cpp
* -------------------------------------------------------
* Created: 25/1/2025
* -------------------------------------------------------
*/

#include <minwinbase.h>
#include <SceneryEditorX/core/version.h>
#include <SceneryEditorX/renderer/vk_util.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <sysinfoapi.h>
#include <timezoneapi.h>
#include <vulkan/vulkan.h>

// -------------------------------------------------------

/**
 * @brief Static member to hold the logger instance.
 */
std::shared_ptr<spdlog::logger> Log::EditorLogger_;
std::shared_ptr<spdlog::logger> Log::LauncherLogger_;
std::shared_ptr<spdlog::logger> Log::VulkanLogger_;

/**
 * @brief Static member to hold the enabled tags.
 */
std::map<std::string, Log::TagDetails> Log::EnabledTags_ =
{
    {"Animation",			TagDetails{true, Level::Warn}},
    {"Asset Pack",			TagDetails{true, Level::Warn}},
    {"AssetManager",		TagDetails{true, Level::Info}},
    {"LibraryManager",		TagDetails{true, Level::Info}},
	{"AssetLoader",			TagDetails{true, Level::Warn}},
	{"AssetLoaderGLTF",		TagDetails{true, Level::Warn}},
	{"AssetLoaderOBJ",		TagDetails{true, Level::Warn}},
	{"AssetLoaderFBX",		TagDetails{true, Level::Warn}},
    {"AssetSystem",			TagDetails{true, Level::Info}},
    {"Assimp",				TagDetails{true, Level::Error}},
    {"Core",				TagDetails{true, Level::Trace}},
    {"GLFW",				TagDetails{true, Level::Error}},
    {"Memory",				TagDetails{true, Level::Error}},
    {"Mesh",				TagDetails{true, Level::Warn}},
    {"Project",				TagDetails{true, Level::Warn}},
    {"Renderer",			TagDetails{true, Level::Info}},
    {"Scene",				TagDetails{true, Level::Info}},
    {"Scripting",			TagDetails{true, Level::Warn}},
    {"Timer",				TagDetails{false, Level::Trace}},
};


void Log::SetDefaultTagSettings()
{
	EnabledTags_ = DefaultTagDetails_;
}

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

    std::vector<spdlog::sink_ptr> editorSinks = {
        std::make_shared<spdlog::sinks::basic_file_sink_mt>("SceneryEditorX.log", true),
        std::make_shared<spdlog::sinks::stdout_color_sink_mt>()
	};

    std::vector<spdlog::sink_ptr> launcherSinks = {
        std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
        std::make_shared<spdlog::sinks::basic_file_sink_mt>("SceneryEditorX.log", true)
	};

    std::vector<spdlog::sink_ptr> vulkanSinks = {
        std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
        std::make_shared<spdlog::sinks::basic_file_sink_mt>("VulkanDebug.log", true)
	};

    //editorSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("SceneryEditorX.log", true));
    //editorSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	// 
    //vulkanSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    //vulkanSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("VulkanDebug.log", true));

    // -------------------------------------------------------

    editorSinks[0]->set_pattern("%^[%T] %n: %v%$");  // color coding for console
    editorSinks[1]->set_pattern("[%T] [%l] %n: %v"); // no color coding for file

    launcherSinks[0]->set_pattern("%^[%T] %n: %v%$");  // color coding for console
    launcherSinks[1]->set_pattern("[%T] [%l] %n: %v"); // no color coding for file

    vulkanSinks[0]->set_pattern("%^[%T] %n: %v%$");  // color coding for console
    vulkanSinks[1]->set_pattern("[%T] [%l] %n: %v"); // no color coding for file

    // -------------------------------------------------------

    EditorLogger_ = std::make_shared<spdlog::logger>("SceneryEditorX", editorSinks.begin(), editorSinks.end());
    EditorLogger_->set_level(spdlog::level::trace);

    LauncherLogger_ = std::make_shared<spdlog::logger>("SceneryEditorX", launcherSinks.begin(), launcherSinks.end());
    LauncherLogger_->set_level(spdlog::level::trace);

    VulkanLogger_ = std::make_shared<spdlog::logger>("VulkanDebug", vulkanSinks.begin(), vulkanSinks.end());
    VulkanLogger_->set_level(spdlog::level::trace);

    // -------------------------------------------------------

    //spdlog::register_logger(VulkanLogger_);

    //VulkanLogger_->flush_on(spdlog::level::trace);

	// -------------------------------------------------------

//	std::vector<spdlog::sink_ptr> sinks;                                              // create a vector of sink pointers
//	sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>()); // create a console sink

	/**
	 * @brief Adds a file sink to the logger.
	 * 
	 * This line adds a file sink to the logger, which outputs log messages to a file named "SceneryEditorX.log".
	 * The second parameter 'true' indicates that the log file should be created in append mode, meaning new log
	 * messages will be added to the end of the file if it already exists.
	 */
//	sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("SceneryEditorX.log", true));

   /**
    * @brief Sets the log pattern for the console and file sinks.
    * 
    * This line sets the log pattern for the console sink to include the time, logger name, and message,
    * with color formatting. The file sink pattern includes the time, log level, logger name, and message.
    * 
    * @param pattern The pattern to set for the log output.
    */
//	sinks[0]->set_pattern("%^[%T] %n: %v%$");       // color coding
//	sinks[1]->set_pattern("[%T] [%l] %n: %v");      // no color coding

	 // Create a logger with the sinks
//	EditorLogger_ = std::make_shared<spdlog::logger>("SceneryEditorX",
//													 sinks.begin(),
//													 sinks.end());

	// Register the logger with spdlog
//	spdlog::register_logger(EditorLogger_);                        
//	EditorLogger_->set_level(spdlog::level::trace);                                                   // set the logging level to trace
//	EditorLogger_->flush_on(spdlog::level::trace);                                                    // flush the logger on trace level log entries
}

void Log::LogVulkanDebug(const std::string &message)
{
    if (VulkanLogger_)
    {
        // Parse severity from the formatted message
        if (message.find("[ERROR]") != std::string::npos)
        {
            VulkanLogger_->error(message);
        }
        else if (message.find("[WARNING]") != std::string::npos)
        {
            VulkanLogger_->warn(message);
        }
        else if (message.find("[INFO]") != std::string::npos)
        {
            VulkanLogger_->info(message);
        }
        else if (message.find("[VERBOSE]") != std::string::npos)
        {
            VulkanLogger_->debug(message);
        }
        else if (message.find("error") != std::string::npos || message.find("ERROR") != std::string::npos)
        {
            VulkanLogger_->error(message);
        }
        else if (message.find("warning") != std::string::npos || message.find("WARNING") != std::string::npos)
        {
            VulkanLogger_->warn(message);
        }
        else if (message.find("performance") != std::string::npos || message.find("PERFORMANCE") != std::string::npos)
        {
            VulkanLogger_->warn("PERFORMANCE: {}", message);
        }
        else
        {
            VulkanLogger_->trace(message);
        }

        // Always flush to ensure messages are written immediately
        VulkanLogger_->flush();
    }
}

void Log::LogVulkanResult(VkResult result, const std::string &operation)
{
    if (VulkanLogger_)
    {
        if (result != VK_SUCCESS)
        {
            const char *resultString = VK_ERROR_STRING(result);
            std::string message = "Vulkan operation '" + operation + "' returned " + resultString;

            if (result < 0)
            { // Negative values are errors
                VulkanLogger_->error("{}", message);
            }
            else
            { // Non-zero positive values are warnings/info
                VulkanLogger_->warn("{}", message);
            }

            VulkanLogger_->flush();
        }
        else
        {
            // Optionally log successful operations at trace level
            VulkanLogger_->trace("Vulkan operation '{}' completed successfully", operation);
        }
    }
}

// -------------------------------------------------------

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
	// -------------------------------------------------------
	// TODO: Refactor this code to use enum case values for the different processor architectures. (Example: x86, x64, ARM/ AMD, Intel i9)
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);

	// -------------------------------------------------------
	// TODO:
	SYSTEMTIME systemTime;
	GetSystemTime(&systemTime);

	// -------------------------------------------------------

	// TODO: Add enum case values for the different time zones to return. (Example: EST,GMT,DST)
	TIME_ZONE_INFORMATION timeZoneInfo;
	GetTimeZoneInformation(&timeZoneInfo);
	std::wstring timeZoneName =
		(timeZoneInfo.StandardName[0] != L'\0') ? timeZoneInfo.StandardName : timeZoneInfo.DaylightName;

	// -------------------------------------------------------

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
	spdlog::info("Timer Zone: {}", std::string(timeZoneName.begin(), timeZoneName.end()));
	spdlog::info("Processor Architecture: {}", sysInfo.wProcessorArchitecture);
	spdlog::info("Processor Cores: {}", sysInfo.dwNumberOfProcessors);
	spdlog::info("Page Size: {}", sysInfo.dwPageSize);
	spdlog::info("Processor Type: {}", sysInfo.dwProcessorType);
	spdlog::info("Minimum EditorApplication Address: {}", sysInfo.lpMinimumApplicationAddress);
	spdlog::info("Maximum EditorApplication Address: {}", sysInfo.lpMaximumApplicationAddress);
	spdlog::info("Active Processor Mask: {}", sysInfo.dwActiveProcessorMask);
	spdlog::info("============================================");
	spdlog::info("============================================");
	spdlog::info("Scenery Editor X");
	spdlog::info("Version: {}.{}.{}.{}", SEDX_VER_MAJOR, SEDX_VER_MINOR, SEDX_VER_PATCH, SEDX_VER_BUILD);
	spdlog::info("Build Date: {}", __DATE__);
	spdlog::info("Build Timer: {}", __TIME__);
	spdlog::info("Coalition of Freeware Developers");
	spdlog::info("Copyright (C) 2025");
	spdlog::info("============================================");
	spdlog::info("============================================");
}

void Log::shut_down()
{
    spdlog::info("Shutting down logging system...");
    EditorLogger_.reset();
    LauncherLogger_.reset();
    VulkanLogger_.reset();
    spdlog::drop_all();
	spdlog::shutdown();
}

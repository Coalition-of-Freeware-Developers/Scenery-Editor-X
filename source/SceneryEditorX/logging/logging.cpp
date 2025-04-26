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
#include <SceneryEditorX/core/application_data.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <sysinfoapi.h>
#include <timezoneapi.h>
#include <vulkan/vulkan.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	
	/**
	 * @brief Static member to hold the logger instance.
	 */
	std::shared_ptr<spdlog::logger> Log::CoreLogger;
	std::shared_ptr<spdlog::logger> Log::EditorLogger;
	std::shared_ptr<spdlog::logger> Log::EditorConsoleLogger;
	
	/**
	 * @brief Static member to hold the enabled tags.
	 */
	std::map<std::string, Log::TagDetails> Log::DefaultTagDetails_ =
	{
	    {"Animation",			    TagDetails{true, Level::Warn}},
	    {"Asset Pack",			TagDetails{true, Level::Warn}},
	    {"AssetManager",		    TagDetails{true, Level::Info}},
	    {"LibraryManager",		TagDetails{true, Level::Info}},
		{"AssetLoader",			TagDetails{true, Level::Warn}},
		{"AssetLoaderGLTF",		TagDetails{true, Level::Warn}},
		{"AssetLoaderOBJ",		TagDetails{true, Level::Warn}},
		{"AssetLoaderFBX",		TagDetails{true, Level::Warn}},
	    {"AssetSystem",			TagDetails{true, Level::Info}},
	    {"Assimp",				TagDetails{true, Level::Error}},
	    {"Core",					TagDetails{true, Level::Trace}},
	    {"GLFW",					TagDetails{true, Level::Error}},
	    {"Memory",				TagDetails{true, Level::Error}},
	    {"Mesh",				    TagDetails{true, Level::Warn}},
	    {"Project",				TagDetails{true, Level::Warn}},
	    {"Renderer",				TagDetails{true, Level::Info}},
	    {"Scene",					TagDetails{true, Level::Info}},
	    {"Scripting",				TagDetails{true, Level::Warn}},
	    {"Timer",					TagDetails{false, Level::Trace}},
	};
	
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
	
	    std::vector<spdlog::sink_ptr> coreSinks = {
	        std::make_shared<spdlog::sinks::basic_file_sink_mt>("SceneryEditorX.log", true),
	        std::make_shared<spdlog::sinks::stdout_color_sink_mt>()};
	
	    std::vector<spdlog::sink_ptr> editorSinks = {
	        std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
	        std::make_shared<spdlog::sinks::basic_file_sink_mt>("SceneryEditorX.log", true)};
	
	    std::vector<spdlog::sink_ptr> editorConsoleSinks = {
	        std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
	        std::make_shared<spdlog::sinks::basic_file_sink_mt>("EditorCore.log", true)};
	
	    //coreSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("SceneryEditorX.log", true));
	    //coreSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	    //
	    //editorConsoleSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	    //editorConsoleSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("VulkanDebug.log", true));
	
	    // -------------------------------------------------------
	
	    coreSinks[0]->set_pattern("%^[%T] %n: %v%$");  // color coding for console
	    editorSinks[0]->set_pattern("%^[%T] %n: %v%$");  // color coding for console
	
	    coreSinks[1]->set_pattern("[%T] [%l] %n: %v"); // no color coding for file
	    editorSinks[1]->set_pattern("[%T] [%l] %n: %v"); // no color coding for file
		for (const auto sink : editorConsoleSinks) sink->set_pattern("%^%v%$");
	
	    // -------------------------------------------------------
	
	    CoreLogger = std::make_shared<spdlog::logger>("SceneryEditorX", coreSinks.begin(), coreSinks.end());
	    CoreLogger->set_level(spdlog::level::trace);
	
	    EditorLogger = std::make_shared<spdlog::logger>("SceneryEditorX", editorSinks.begin(), editorSinks.end());
	    EditorLogger->set_level(spdlog::level::trace);
	
	    EditorConsoleLogger = std::make_shared<spdlog::logger>("VulkanDebug", editorConsoleSinks.begin(), editorConsoleSinks.end());
	    EditorConsoleLogger->set_level(spdlog::level::trace);
	
		SetDefaultTagSettings();
	
	    // -------------------------------------------------------
	}

	void Log::SetDefaultTagSettings()
    {
        EnabledTags_ = DefaultTagDetails_;
    }

	void Log::LogVulkanDebug(const std::string &message)
	{
	    if (CoreLogger)
	    {
	        // Parse severity from the formatted message
	        if (message.find("[ERROR]") != std::string::npos)
	        {
	            CoreLogger->error(message);
	        }
	        else if (message.find("[WARNING]") != std::string::npos)
	        {
	            CoreLogger->warn(message);
	        }
	        else if (message.find("[INFO]") != std::string::npos)
	        {
	            CoreLogger->info(message);
	        }
	        else if (message.find("[VERBOSE]") != std::string::npos)
	        {
	            CoreLogger->debug(message);
	        }
	        else if (message.find("error") != std::string::npos || message.find("ERROR") != std::string::npos)
	        {
	            CoreLogger->error(message);
	        }
	        else if (message.find("warning") != std::string::npos || message.find("WARNING") != std::string::npos)
	        {
	            CoreLogger->warn(message);
	        }
	        else if (message.find("performance") != std::string::npos || message.find("PERFORMANCE") != std::string::npos)
	        {
	            CoreLogger->warn("PERFORMANCE: {}", message);
	        }
	        else
	        {
	            CoreLogger->trace(message);
	        }
	
	        // Always flush to ensure messages are written immediately
	        CoreLogger->flush();
	    }
	}

    /*
	void Log::LogVulkanResult(VkResult result, const std::string &operation)
	{
	    if (CoreLogger)
	    {
	        if (result != VK_SUCCESS)
	        {
	            const char *resultString = vkErrorString(result);
	            std::string message = "Vulkan operation '" + operation + "' returned " + resultString;
	
	            if (result < 0)
	            { // Negative values are errors
	                CoreLogger->error("{}", message);
	            }
	            else
	            { // Non-zero positive values are warnings/info
	                CoreLogger->warn("{}", message);
	            }
	
	            CoreLogger->flush();
	        }
	        else
	        {
	            // Optionally log successful operations at trace level
	            CoreLogger->trace("Vulkan operation '{}' completed successfully", operation);
	        }
	    }
	}
	*/


    /**
     * @brief Get the name of the operating system.
     * @return The name of the operating system as a string.
     */
    [[nodiscard]] static constexpr std::string getOsName()
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

    // -------------------------------------------------------

	void Log::LogHeader()
	{
        SoftwareStats stats;
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
		spdlog::info("Time Zone: {}", std::string(timeZoneName.begin(), timeZoneName.end()));
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
        spdlog::info("Version: {}", SEDX_VERSION_STRING);
		spdlog::info("Build Date: {}", __DATE__);
		spdlog::info("Build Time: {}", __TIME__);
		spdlog::info("Coalition of Freeware Developers");
		spdlog::info("Copyright (C) 2025");
		spdlog::info("============================================");
		spdlog::info("============================================");
	}
	
	void Log::ShutDown()
	{
	    EditorConsoleLogger.reset();
	    EditorLogger.reset();
	    CoreLogger.reset();
	    spdlog::drop_all(); // Drop all loggers
		spdlog::shutdown();
	}
	
	// -------------------------------------------------------
	// taken from Sam Lantiga: https://www.libsdl.org/tmp/SDL/test/testvulkan.c
    [[maybe_unused]] static const char* vkErrorString(VkResult result)
	{
		switch (static_cast<int>(result))
		{
		case VK_SUCCESS:
			return "VK_SUCCESS";
		case VK_NOT_READY:
			return "VK_NOT_READY";
		case VK_TIMEOUT:
			return "VK_TIMEOUT";
		case VK_EVENT_SET:
			return "VK_EVENT_SET";
		case VK_EVENT_RESET:
			return "VK_EVENT_RESET";
		case VK_INCOMPLETE:
			return "VK_INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED:
			return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST:
			return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED:
			return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT:
			return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT:
			return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT:
			return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER:
			return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS:
			return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED:
			return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_FRAGMENTED_POOL:
			return "VK_ERROR_FRAGMENTED_POOL";
		case VK_ERROR_SURFACE_LOST_KHR:
			return "VK_ERROR_SURFACE_LOST_KHR";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
			return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case VK_SUBOPTIMAL_KHR:
			return "VK_SUBOPTIMAL_KHR";
		case VK_ERROR_OUT_OF_DATE_KHR:
			return "VK_ERROR_OUT_OF_DATE_KHR";
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
			return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		case VK_ERROR_VALIDATION_FAILED_EXT:
			return "VK_ERROR_VALIDATION_FAILED_EXT";
		case VK_ERROR_OUT_OF_POOL_MEMORY_KHR:
			return "VK_ERROR_OUT_OF_POOL_MEMORY_KHR";
		case VK_ERROR_INVALID_SHADER_NV:
			return "VK_ERROR_INVALID_SHADER_NV";
		default:
			break;
		}
		if (result < 0)
			return "VK_ERROR_<Unknown>";
		return "VK_<Unknown>";
	}

	// -------------------------------------------------------

} // namespace SceneryEditorX

// -------------------------------------------------------

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
    std::shared_ptr<spdlog::logger> Log::LauncherLogger;
	
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
	    try 
	    {
            /// Check if loggers already exist and drop them
            if (spdlog::get("SceneryEditorX-Core"))
                spdlog::drop("SceneryEditorX-Core");
            if (spdlog::get("SceneryEditorX-Editor"))
                spdlog::drop("SceneryEditorX-Editor");
            if (spdlog::get("VulkanDebug"))
                spdlog::drop("VulkanDebug");
            if (spdlog::get("Launcher"))
                spdlog::drop("Launcher");

	        /// -------------------------------------------------------

	        std::vector<spdlog::sink_ptr> coreSinks = {
                std::make_shared<spdlog::sinks::basic_file_sink_mt>("SceneryEditorX.log", true),
	            std::make_shared<spdlog::sinks::stdout_color_sink_mt>()};
	
	        std::vector<spdlog::sink_ptr> editorSinks = {
	            std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
                std::make_shared<spdlog::sinks::basic_file_sink_mt>("SceneryEditorX.log", true)};
	
	        std::vector<spdlog::sink_ptr> editorConsoleSinks = {
	            std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
	            std::make_shared<spdlog::sinks::basic_file_sink_mt>("EditorCore.log", true)};

			std::vector <spdlog::sink_ptr> launcherSinks = {
				std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
                std::make_shared<spdlog::sinks::basic_file_sink_mt>("Launcher.log", true)};
	
	        /// -------------------------------------------------------
	
	        /// Pattern for console sinks
	        coreSinks[1]->set_pattern("%^[%T] %n: %v%$");
	        editorSinks[0]->set_pattern("%^[%T] %n: %v%$");
            launcherSinks[1]->set_pattern("%^[%T] %n: %v%$"); 
	
	        /// Pattern for file sinks - note the correct indices
	        coreSinks[0]->set_pattern("[%T] [%l] %n: %v");
	        editorSinks[1]->set_pattern("[%T] [%l] %n: %v"); 
            launcherSinks[1]->set_pattern("[%T] [%l] %n: %v");
	        
	        for (const auto& sink : editorConsoleSinks) 
	            sink->set_pattern("%^%v%$");
	
	        /// -------------------------------------------------------
	
	        CoreLogger = std::make_shared<spdlog::logger>("Core", coreSinks.begin(), coreSinks.end());
	        CoreLogger->set_level(spdlog::level::trace);
	        CoreLogger->flush_on(spdlog::level::info); // Flush on info level and above
	
	        EditorLogger = std::make_shared<spdlog::logger>("SceneryEditorX", editorSinks.begin(), editorSinks.end());
	        EditorLogger->set_level(spdlog::level::trace);
	        EditorLogger->flush_on(spdlog::level::info); // Flush on info level and above
	
	        EditorConsoleLogger = std::make_shared<spdlog::logger>("Vulkan", editorConsoleSinks.begin(), editorConsoleSinks.end());
	        EditorConsoleLogger->set_level(spdlog::level::trace);
	        EditorConsoleLogger->flush_on(spdlog::level::info); // Flush on info level and above

			LauncherLogger = std::make_shared<spdlog::logger>("Launcher", launcherSinks.begin(), launcherSinks.end());
            LauncherLogger->set_level(spdlog::level::trace);
            LauncherLogger->flush_on(spdlog::level::info); // Flush on info level and above
	
	        // Register loggers with spdlog
	        spdlog::register_logger(CoreLogger);
	        spdlog::register_logger(EditorLogger);
	        spdlog::register_logger(EditorConsoleLogger);
			spdlog::register_logger(LauncherLogger);
	
	        SetDefaultTagSettings();
	
	        CoreLogger->info("Log system initialized successfully");
	        CoreLogger->flush();
	    }
	    catch (const spdlog::spdlog_ex& ex)
	    {
	        std::cerr << "Log initialization failed: " << ex.what() << '\n';
	    }

	    /// -------------------------------------------------------
	}

	void Log::SetDefaultTagSettings()
    {
        EnabledTags_ = DefaultTagDetails_;
    }

	void Log::LogVulkanDebug(const std::string &message)
	{
	    if (CoreLogger)
	    {
	        /// Parse severity from the formatted message
	        if (message.find("[ERROR]") != std::string::npos)
                CoreLogger->error(message);
            else if (message.find("[WARNING]") != std::string::npos)
                CoreLogger->warn(message);
            else if (message.find("[INFO]") != std::string::npos)
                CoreLogger->info(message);
            else if (message.find("[VERBOSE]") != std::string::npos)
                CoreLogger->debug(message);
            else if (message.find("error") != std::string::npos || message.find("ERROR") != std::string::npos)
                CoreLogger->error(message);
            else if (message.find("warning") != std::string::npos || message.find("WARNING") != std::string::npos)
                CoreLogger->warn(message);
            else if (message.find("performance") != std::string::npos || message.find("PERFORMANCE") != std::string::npos)
                CoreLogger->warn("PERFORMANCE: {}", message);
            else
                CoreLogger->trace(message);

            /// Always flush to ensure messages are written immediately
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

    /// -------------------------------------------------------

	void Log::LogHeader()
	{
        AppData stats;
		/// -------------------------------------------------------
		// TODO: Refactor this code to use enum case values for the different processor architectures. (Example: x86, x64, ARM/ AMD, Intel i9)
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
	
		/// -------------------------------------------------------
		// TODO:
		SYSTEMTIME systemTime;
		GetSystemTime(&systemTime);
	
		/// -------------------------------------------------------
	
		// TODO: Add enum case values for the different time zones to return. (Example: EST,GMT,DST)
		TIME_ZONE_INFORMATION timeZoneInfo;
		GetTimeZoneInformation(&timeZoneInfo);
		std::wstring timeZoneName =
			(timeZoneInfo.StandardName[0] != L'\0') ? timeZoneInfo.StandardName : timeZoneInfo.DaylightName;
	
		/// -------------------------------------------------------
	
		SEDX_CORE_INFO("============================================");
		SEDX_CORE_INFO("System Information");
		SEDX_CORE_INFO("Operating System: {}", getOsName());
		SEDX_CORE_INFO("{:02}:{:02}:{:02} {:02}/{:02}/{:04}",
					 systemTime.wHour,
					 systemTime.wMinute,
					 systemTime.wSecond,
					 systemTime.wDay,
					 systemTime.wMonth,
					 systemTime.wYear);
		SEDX_CORE_INFO("Time Zone: {}", std::string(timeZoneName.begin(), timeZoneName.end()));
		SEDX_CORE_INFO("Processor Architecture: {}", sysInfo.wProcessorArchitecture);
		SEDX_CORE_INFO("Processor Cores: {}", sysInfo.dwNumberOfProcessors);
		SEDX_CORE_INFO("Page Size: {}", sysInfo.dwPageSize);
		SEDX_CORE_INFO("Processor Type: {}", sysInfo.dwProcessorType);
		SEDX_CORE_INFO("Minimum EditorApplication Address: {}", sysInfo.lpMinimumApplicationAddress);
		SEDX_CORE_INFO("Maximum EditorApplication Address: {}", sysInfo.lpMaximumApplicationAddress);
		SEDX_CORE_INFO("Active Processor Mask: {}", sysInfo.dwActiveProcessorMask);
		SEDX_CORE_INFO("============================================");
		SEDX_CORE_INFO("============================================");
		SEDX_CORE_INFO("Scenery Editor X");
        SEDX_CORE_INFO("Version: {}", SEDX_VERSION_STRING);
		SEDX_CORE_INFO("Build Date: {}", __DATE__);
		SEDX_CORE_INFO("Build Time: {}", __TIME__);
		SEDX_CORE_INFO("Coalition of Freeware Developers");
		SEDX_CORE_INFO("Copyright (C) 2025");
		SEDX_CORE_INFO("============================================");
		SEDX_CORE_INFO("============================================");
	}
	
	void Log::ShutDown()
	{
        if (CoreLogger)
        {
            CoreLogger->flush();
            spdlog::drop(CoreLogger->name()); // Explicitly drop by name
            CoreLogger.reset();
        }

        if (EditorLogger)
        {
            EditorLogger->flush();
            spdlog::drop(EditorLogger->name()); // Explicitly drop by name
            EditorLogger.reset();
        }

        if (EditorConsoleLogger)
        {
            EditorConsoleLogger->flush();
            spdlog::drop(EditorConsoleLogger->name()); // Explicitly drop by name
            EditorConsoleLogger.reset();
        }

	    if (LauncherLogger)
        {
            LauncherLogger->flush();
            spdlog::drop(LauncherLogger->name()); // Explicitly drop by name
            LauncherLogger.reset();
        }
	    
	    spdlog::drop_all(); // Drop all loggers
	    spdlog::shutdown();
	}
	
	/// -------------------------------------------------------

	/// taken from Sam Lantiga: https://www.libsdl.org/tmp/SDL/test/testvulkan.c
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

	/// -------------------------------------------------------

} // namespace SceneryEditorX

/// -------------------------------------------------------

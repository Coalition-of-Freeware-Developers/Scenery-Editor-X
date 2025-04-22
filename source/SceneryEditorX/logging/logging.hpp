/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* logging.hpp
* -------------------------------------------------------
* Created: 25/1/2025
* -------------------------------------------------------
*/
#pragma once

#include <cstdint>
#include <fmt/core.h>
#include <SceneryEditorX/logging/log_templates.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.h>

// -------------------------------------------------------

#define SEDX_ASSERT_MESSAGE_BOX (!SEDX_RELEASE && SEDX_PLATFORM_WINDOWS)

#if defined SEDX_ASSERT_MESSAGE_BOX
	#ifdef SEDX_PLATFORM_WINDOWS
	#include <Windows.h>
	#endif
#endif

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class Log
	 * @brief A static class for initializing and accessing the logger.
	 *
	 * This class provides a static interface for initializing the logger and
	 * accessing the logger instance. It uses the spdlog library for logging.
	 */
	
	class Log
	{
	public:

        /**
		 * @brief An enumeration of log types.
		 */
	    enum class Type : uint8_t
	    {
	        Core = 0,
	        Editor = 1
	    };
	    /**
		 * @brief An enumeration of log levels.
		 */
	    enum class Level : uint8_t
	    {
	        Trace = 0,
	        Info,
	        Warn,
	        Error,
	        Fatal
	    };
	
	    /**
		 * @brief A structure to hold the details of a tag.
		 */
	    struct TagDetails
	    {
	        bool Enabled = true;
	        Level LevelFilter = Level::Trace;
	    };
	
	    // ------------------------------------------------
	
	    /**
		 * @brief Initializes the logger instance.
		 *
		 * This function initializes the logger instance with the desired settings.
		 */
	    static void Init();
	
	    /**
		 * @brief Logs system information to the console.
		 *
		 * This function logs system information to the console. It logs the
		 * operating system version, processor architecture, number of processors,
		 * page size, and processor type.
		 */
	    static void LogHeader();
	
	    /**
		 * @brief Shuts down the logger instance.
		 *
		 * This function shuts down the logger instance and releases any resources
		 * associated with it. It should be called before the application exits to
		 * ensure that all log messages are properly flushed.
		 */
	    static void ShutDown();
	
	    /**
		* @brief Logs a message with the specified vulkan log level.
		*
		* This function logs a message with the vulkan log level to the console and the log file.
		*/
	    static void LogVulkanDebug(const std::string &message);
	
	    // Helper function for logging Vulkan API calls with result checking
	    static void LogVulkanResult(VkResult result, const std::string &operation);
	
	    /**
		 * @brief Gets the logger instance.
		 * @return A shared pointer to the logger instance.
		 *
		 * This function returns a shared pointer to the logger instance.
		 */
        static std::shared_ptr<spdlog::logger> &GetCoreLogger() { return CoreLogger; }
        static std::shared_ptr<spdlog::logger> &GetEditorLogger() { return EditorLogger; }
        static std::shared_ptr<spdlog::logger> &GetEditorConsoleLogger() { return EditorConsoleLogger; }
	
	    // -------------------------------------------------------------
	
	    static bool HasTag(const std::string &tag) { return EnabledTags_.contains(std::string(tag.data(), tag.size())); }
	    static std::map<std::string, TagDetails> &EnabledTags() { return EnabledTags_; }
	    static void SetDefaultTagSettings();
	
	    // -----------------------------------------------------------

#ifdef SEDX_PLATFORM_WINDOWS
        template <typename... Args>
        static void PrintMessage(Log::Type type, Log::Level level, std::format_string<Args...> format, Args &&...args);
#else
		template<typename... Args>
		static void PrintMessage(Log::Type type, Log::Level level, const std::string_view format, Args&&... args);
#endif
	    template <typename... Args>
	    static void PrintMessageTag(Log::Type type, Log::Level level, std::string_view tag, std::format_string<Args...> format, Args &&...args);
									
	    static void PrintMessageTag(Log::Type type, Log::Level level, std::string_view tag, std::string_view message);
	
	    template <typename... Args>
	    static void PrintAssertMessage(Log::Type type, std::string_view prefix, std::format_string<Args...> message, Args &&...args);
	
	    static void PrintAssertMessage(Log::Type type, std::string_view prefix);
	
	    // -----------------------------------------------------------
	
	    /**
		 * @brief Converts a log level to a string.
		 * @param level The log level to convert.
		 * @return The string representation of the log level.
		 */
	    static const char *LevelToString(Level level)
	    {
	        switch (level)
	        {
	        case Level::Trace: return "Trace";
	        case Level::Info: return "Info";
	        case Level::Warn: return "Warn";
	        case Level::Error: return "Error";
	        case Level::Fatal: return "Fatal";
	        }
	        return "";
	    }
	
	    /**
		* @brief Converts a string to a log level.
		* @param string The string to convert.
		* @return The log level corresponding to the string.
		*/
	    static Level LevelFromString(std::string_view string)
	    {
	        if (string == "Trace") return Level::Trace;
	        if (string == "Info") return Level::Info;
	        if (string == "Warn") return Level::Warn;
	        if (string == "Error") return Level::Error;
	        if (string == "Fatal") return Level::Fatal;
	
	        return Level::Trace;
	    }
	
	
	private:
	    /**
		 * @brief The logger instances for the Core Logger, Editor Logger, and Editor Debug Logger for the UI debug console.
		 */
	    static std::shared_ptr<spdlog::logger> CoreLogger;
	    static std::shared_ptr<spdlog::logger> EditorLogger;
	    static std::shared_ptr<spdlog::logger> EditorConsoleLogger;
	
	    inline static std::map<std::string, TagDetails> EnabledTags_;
	    static std::map<std::string, TagDetails> DefaultTagDetails_;
	};

} // namespace SceneryEditorX

    // -------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Tagged logs (prefer these!)                                                                                      //
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	// Core logging
	#define SEDX_CORE_TRACE_TAG(tag, ...) ::SceneryEditorX::Log::PrintMessageTag(::SceneryEditorX::Log::Type::Core, ::SceneryEditorX::Log::Level::Trace, tag, __VA_ARGS__)
	#define SEDX_CORE_INFO_TAG(tag, ...)  ::SceneryEditorX::Log::PrintMessageTag(::SceneryEditorX::Log::Type::Core, ::SceneryEditorX::Log::Level::Info, tag, __VA_ARGS__)
	#define SEDX_CORE_WARN_TAG(tag, ...)  ::SceneryEditorX::Log::PrintMessageTag(::SceneryEditorX::Log::Type::Core, ::SceneryEditorX::Log::Level::Warn, tag, __VA_ARGS__)
	#define SEDX_CORE_ERROR_TAG(tag, ...) ::SceneryEditorX::Log::PrintMessageTag(::SceneryEditorX::Log::Type::Core, ::SceneryEditorX::Log::Level::Error, tag, __VA_ARGS__)
	#define SEDX_CORE_FATAL_TAG(tag, ...) ::SceneryEditorX::Log::PrintMessageTag(::SceneryEditorX::Log::Type::Core, ::SceneryEditorX::Log::Level::Fatal, tag, __VA_ARGS__)
	
	// Editor logging
	#define EDITOR_TRACE_TAG(tag, ...) ::SceneryEditorX::Log::PrintMessageTag(::SceneryEditorX::Log::Type::Editor, ::SceneryEditorX::Log::Level::Trace, tag, __VA_ARGS__)
	#define EDITOR_INFO_TAG(tag, ...)  ::SceneryEditorX::Log::PrintMessageTag(::SceneryEditorX::Log::Type::Editor, ::SceneryEditorX::Log::Level::Info, tag, __VA_ARGS__)
	#define EDITOR_WARN_TAG(tag, ...)  ::SceneryEditorX::Log::PrintMessageTag(::SceneryEditorX::Log::Type::Editor, ::SceneryEditorX::Log::Level::Warn, tag, __VA_ARGS__)
	#define EDITOR_ERROR_TAG(tag, ...) ::SceneryEditorX::Log::PrintMessageTag(::SceneryEditorX::Log::Type::Editor, ::SceneryEditorX::Log::Level::Error, tag, __VA_ARGS__)
	#define EDITOR_FATAL_TAG(tag, ...) ::SceneryEditorX::Log::PrintMessageTag(::SceneryEditorX::Log::Type::Editor, ::SceneryEditorX::Log::Level::Fatal, tag, __VA_ARGS__)
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	// Core Logging
	#define SEDX_CORE_TRACE(...) ::SceneryEditorX::Log::PrintMessage(::SceneryEditorX::Log::Type::Core, ::SceneryEditorX::Log::Level::Trace, __VA_ARGS__)
	#define SEDX_CORE_INFO(...)  ::SceneryEditorX::Log::PrintMessage(::SceneryEditorX::Log::Type::Core, ::SceneryEditorX::Log::Level::Info, __VA_ARGS__)
	#define SEDX_CORE_WARN(...)  ::SceneryEditorX::Log::PrintMessage(::SceneryEditorX::Log::Type::Core, ::SceneryEditorX::Log::Level::Warn, __VA_ARGS__)
	#define SEDX_CORE_ERROR(...) ::SceneryEditorX::Log::PrintMessage(::SceneryEditorX::Log::Type::Core, ::SceneryEditorX::Log::Level::Error, __VA_ARGS__)
	#define SEDX_CORE_FATAL(...) ::SceneryEditorX::Log::PrintMessage(::SceneryEditorX::Log::Type::Core, ::SceneryEditorX::Log::Level::Fatal, __VA_ARGS__)
	
	// Client Logging
	#define EDITOR_TRACE(...) ::SceneryEditorX::Log::PrintMessage(::SceneryEditorX::Log::Type::Editor, ::SceneryEditorX::Log::Level::Trace, __VA_ARGS__)
	#define EDITOR_INFO(...)  ::SceneryEditorX::Log::PrintMessage(::SceneryEditorX::Log::Type::Editor, ::SceneryEditorX::Log::Level::Info, __VA_ARGS__)
	#define EDITOR_WARN(...)  ::SceneryEditorX::Log::PrintMessage(::SceneryEditorX::Log::Type::Editor, ::SceneryEditorX::Log::Level::Warn, __VA_ARGS__)
	#define EDITOR_ERROR(...) ::SceneryEditorX::Log::PrintMessage(::SceneryEditorX::Log::Type::Editor, ::SceneryEditorX::Log::Level::Error, __VA_ARGS__)
	#define EDITOR_FATAL(...) ::SceneryEditorX::Log::PrintMessage(::SceneryEditorX::Log::Type::Editor, ::SceneryEditorX::Log::Level::Fatal, __VA_ARGS__)
	
	// Editor Console Logging Macros
	#define EDITOR_CONSOLE_LOG_TRACE(...) ::SceneryEditorX::Log::GetEditorConsoleLogger()->trace(__VA_ARGS__)
	#define EDITOR_CONSOLE_LOG_INFO(...)  ::SceneryEditorX::Log::GetEditorConsoleLogger()->info(__VA_ARGS__)
	#define EDITOR_CONSOLE_LOG_WARN(...)  ::SceneryEditorX::Log::GetEditorConsoleLogger()->warn(__VA_ARGS__)
	#define EDITOR_CONSOLE_LOG_ERROR(...) ::SceneryEditorX::Log::GetEditorConsoleLogger()->error(__VA_ARGS__)
	#define EDITOR_CONSOLE_LOG_FATAL(...) ::SceneryEditorX::Log::GetEditorConsoleLogger()->critical(__VA_ARGS__)
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// ----------------------------------------------------

namespace SceneryEditorX
{
#ifdef SEDX_PLATFORM_WINDOWS
	template <typename... Args>
	void Log::PrintMessage(Log::Type type, Log::Level level, std::format_string<Args...> format, Args &&...args)
#else
	template <typename... Args>
	void Log::PrintMessage(Log::Type type, Log::Level level, const std::string_view format, Args &&...args)
#endif

	{
	    auto &detail = EnabledTags_[""];
	    if (detail.Enabled && detail.LevelFilter <= level)
	    {
	        auto logger = (type == Type::Core) ? GetCoreLogger() : GetEditorLogger();
	        switch (level)
	        {
	        case Level::Trace:
	            logger->trace(format, std::forward<Args>(args)...);
	            break;
	        case Level::Info:
	            logger->info(format, std::forward<Args>(args)...);
	            break;
	        case Level::Warn:
	            logger->warn(format, std::forward<Args>(args)...);
	            break;
	        case Level::Error:
	            logger->error(format, std::forward<Args>(args)...);
	            break;
	        case Level::Fatal:
	            logger->critical(format, std::forward<Args>(args)...);
	            break;
	        }
	    }
	}
	
	// ----------------------------------------------------
	
	template <typename... Args>
	void Log::PrintMessageTag(Log::Type type, Log::Level level, std::string_view tag, const std::format_string<Args...> format, Args &&...args)
	{
	    auto &detail = EnabledTags_[std::string(tag)];
	    if (detail.Enabled && detail.LevelFilter <= level)
	    {
	        auto logger = (type == Type::Core) ? GetCoreLogger() : GetEditorLogger();
	        std::string formatted = std::format(format, std::forward<Args>(args)...);
	        switch (level)
	        {
	        case Level::Trace:
	            logger->trace("[{}] {}", tag, formatted);
	            break;
	        case Level::Info:
	            logger->info("[{}] {}", tag, formatted);
	            break;
	        case Level::Warn:
	            logger->warn("[{}] {}", tag, formatted);
	            break;
	        case Level::Error:
	            logger->error("[{}] {}", tag, formatted);
	            break;
	        case Level::Fatal:
	            logger->critical("[{}] {}", tag, formatted);
	            break;
	        }
	    }
	}
	
	
	// ----------------------------------------------------
	
	inline void Log::PrintMessageTag(Log::Type type, Log::Level level, std::string_view tag, std::string_view message)
	{
	    auto &detail = EnabledTags_[std::string(tag.data(), tag.size())]; // Convert fmt::string_view to std::string properly
	    if (detail.Enabled && detail.LevelFilter <= level)
	    {
	        auto logger = (type == Type::Core) ? GetCoreLogger() : GetEditorLogger();
	        switch (level)
	        {
	        case Level::Trace:
	            logger->trace("[{}] {}", tag, message); // Use curly braces instead of {0} for fmt format strings
	            break;
	        case Level::Info:
	            logger->info("[{}] {}", tag, message);
	            break;
	        case Level::Warn:
	            logger->warn("[{}] {}", tag, message);
	            break;
	        case Level::Error:
	            logger->error("[{}] {}", tag, message);
	            break;
	        case Level::Fatal:
	            logger->critical("[{}] {}", tag, message);
	            break;
	        }
	    }
	}
	
	// ----------------------------------------------------
	
	template <typename... Args>
	void Log::PrintAssertMessage(Log::Type type, std::string_view prefix, std::format_string<Args...> message, Args &&...args)
	{
	    auto logger = (type == Type::Core) ? GetCoreLogger() : GetEditorLogger();
	    auto formatted = std::format(message, std::forward<Args>(args)...);
	    logger->error("{}: {}", prefix, formatted);
	#ifdef SEDX_ASSERT_MESSAGE_BOX
	    MessageBoxA(nullptr, formatted.c_str(), "Scenery Editor X | Assert", MB_OK | MB_ICONERROR);
	#endif
	}
	
	
	// ----------------------------------------------------
	
	inline void Log::PrintAssertMessage(Log::Type type, std::string_view prefix)
	{
	    auto logger = (type == Type::Core) ? GetCoreLogger() : GetEditorLogger();
	    logger->error("{}", prefix);
	#ifdef SEDX_ASSERT_MESSAGE_BOX
	    MessageBoxA(nullptr, "- No message -", "Scenery Editor X | Assert", MB_OK | MB_ICONERROR);
	#endif
	}

} // namespace SceneryEditorX


	// ----------------------------------------------------
	
	
	/**
	 *
	 * Launcher Log Macros
	 * 
	 * @brief A macro to log a message with the specified log level.
	 *
	 * This macro logs a message with the specified log level to the console and the log file.
	 */
	#define LAUNCHER_LOG_TRACE(...)		::SceneryEditorX::Log::GetEditorLogger()->trace(__VA_ARGS__)
	#define LAUNCHER_LOG_INFO(...)		::SceneryEditorX::Log::GetEditorLogger()->info(__VA_ARGS__)
	#define LAUNCHER_LOG_WARN(...)		::SceneryEditorX::Log::GetEditorLogger()->warn(__VA_ARGS__)
	#define LAUNCHER_LOG_ERROR(...)		::SceneryEditorX::Log::GetEditorLogger()->error(__VA_ARGS__)
	#define LAUNCHER_LOG_CRITICAL(...)  ::SceneryEditorX::Log::GetEditorLogger()->critical(__VA_ARGS__);
	
	/*
	 * 
	 * Editor Log Macros
	 *
	 */
	#define EDITOR_LOG_TRACE(...)    ::SceneryEditorX::Log::GetEditorLogger()->trace(__VA_ARGS__)
	#define EDITOR_LOG_INFO(...)     ::SceneryEditorX::Log::GetEditorLogger()->info(__VA_ARGS__)
	#define EDITOR_LOG_WARN(...)     ::SceneryEditorX::Log::GetEditorLogger()->warn(__VA_ARGS__)
	#define EDITOR_LOG_ERROR(...)    ::SceneryEditorX::Log::GetEditorLogger()->error(__VA_ARGS__)
	#define EDITOR_LOG_CRITICAL(...) ::SceneryEditorX::Log::GetEditorLogger()->critical(__VA_ARGS__);

// -------------------------------------------------------

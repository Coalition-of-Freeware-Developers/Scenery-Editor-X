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
#include <spdlog/spdlog.h>

#include <SceneryEditorX/logging/log_formatter.h>
#include <vulkan/vulkan.h>

// -------------------------------------------------------

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

	void Debug(const char *format, ...);
	void Info(const char *format, ...);
	void Warn(const char *format, ...);
	void Error(const char *format, ...);
	void Critical(const char *format, ...);

	/**
	 * @brief An enumeration of log types.
	 */
	enum class Type : uint8_t
	{
		Editor = 0,
		Launcher = 1,
        VulkanDebug = 2,
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
    static void shut_down();

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
	inline static std::shared_ptr<spdlog::logger> &GetEditorLogger() { return EditorLogger_; }
	inline static std::shared_ptr<spdlog::logger> &GetLauncherLogger() { return LauncherLogger_; }
	inline static std::shared_ptr<spdlog::logger> &GetVulkanLogger() { return VulkanLogger_; }

	// -------------------------------------------------------------

    static bool HasTag(fmt::string_view tag) { return EnabledTags_.find(std::string(tag.data(), tag.size())) != EnabledTags_.end(); }
	static std::map<std::string, TagDetails>& EnabledTags() { return EnabledTags_; }
	static void SetDefaultTagSettings();

	// -----------------------------------------------------------

	template<typename... Args>
	static void PrintMessageTag(Log::Type type, Log::Level level, fmt::string_view tag, fmt::format_string<Args...> format, Args&&... args);
	
	static void PrintMessageTag(Log::Type type, Log::Level level, fmt::string_view tag, fmt::string_view message);

	template <typename... Args>
    static void PrintMessage(Log::Type type, Log::Level level, fmt::format_string<Args...> format, Args &&...args);

	template<typename... Args>
	static void PrintAssertMessage(Log::Type type, fmt::string_view prefix, fmt::format_string<Args...> message, Args&&... args);
	
	static void PrintAssertMessage(Log::Type type, fmt::string_view prefix);

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
        case Level::Trace:
            return "Trace";
        case Level::Info:
            return "Info";
        case Level::Warn:
            return "Warn";
        case Level::Error:
            return "Error";
        case Level::Fatal:
            return "Fatal";
        }
        return "";
    }

	/**
	* @brief Converts a string to a log level.
	* @param string The string to convert.
	* @return The log level corresponding to the string.
	*/
    static Level LevelFromString(fmt::string_view string)
    {
        if (string == "Trace")
            return Level::Trace;
        if (string == "Info")
            return Level::Info;
        if (string == "Warn")
            return Level::Warn;
        if (string == "Error")
            return Level::Error;
        if (string == "Fatal")
            return Level::Fatal;

        return Level::Trace;
    }


private:

	/**
	 * @brief The logger instances for the Editor Logger, launcher Logger, and Vulkan Debug Logger.
	 */
	static std::shared_ptr<spdlog::logger> EditorLogger_; 
	static std::shared_ptr<spdlog::logger> LauncherLogger_;
    static std::shared_ptr<spdlog::logger> VulkanLogger_;


	static std::map<std::string, TagDetails> EnabledTags_;
	static std::map<std::string, TagDetails> DefaultTagDetails_;
};

// -------------------------------------------------------

/**
*
* Launcher Log Macros
* 
* @brief A macro to log a message with the specified log level.
*
* This macro logs a message with the specified log level to the console and the log file.
*/

#define LAUNCHER_ASSERT(x, ...)                                                                                             \
	if (!(x))                                                                                                               \
	{                                                                                                                       \
		LAUNCHER_LOG_CRITICAL("Assertion Failed: {0}", __VA_ARGS__);                                                        \
		__debugbreak();                                                                                                     \
	}                                                                                                                       \
	__debugbreak();                                                                                                         \
	abort();

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tagged logs (prefer these!)                                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
* ---------------------------------------------------------
* Editor Log Macros
* ---------------------------------------------------------
*/
#define EDITOR_LOG_TRACE(...)				::Log::PrintMessage(::Log::Type::Editor, ::Log::Level::Trace, __VA_ARGS__)
#define EDITOR_LOG_INFO(...)				::Log::PrintMessage(::Log::Type::Editor, ::Log::Level::Info,  __VA_ARGS__)
#define EDITOR_LOG_WARN(...)				::Log::PrintMessage(::Log::Type::Editor, ::Log::Level::Warn,  __VA_ARGS__)
#define EDITOR_LOG_ERROR(...)				::Log::PrintMessage(::Log::Type::Editor, ::Log::Level::Error, __VA_ARGS__)
#define EDITOR_LOG_CRITICAL(...)			::Log::PrintMessage(::Log::Type::Editor, ::Log::Level::Fatal, __VA_ARGS__)

#define EDITOR_LOG_TRACE_TAG(tag, ...)		::Log::PrintMessageTag(::Log::Type::Editor, ::Log::Level::Trace, tag, __VA_ARGS__)
#define EDITOR_LOG_INFO_TAG(tag, ...)		::Log::PrintMessageTag(::Log::Type::Editor, ::Log::Level::Info, tag, __VA_ARGS__)
#define EDITOR_LOG_WARN_TAG(tag, ...)		::Log::PrintMessageTag(::Log::Type::Editor, ::Log::Level::Warn, tag, __VA_ARGS__)
#define EDITOR_LOG_ERROR_TAG(tag, ...)		::Log::PrintMessageTag(::Log::Type::Editor, ::Log::Level::Error, tag, __VA_ARGS__)
#define EDITOR_LOG_CRITICAL_TAG(tag, ...)   ::Log::PrintMessageTag(::Log::Type::Editor, ::Log::Level::Fatal, tag, __VA_ARGS__)

//-------------------------------------------------------

#define LAUNCHER_LOG_TRACE(...)				::Log::PrintMessage(::Log::Type::Launcher, ::Log::Level::Trace, __VA_ARGS__)
#define LAUNCHER_LOG_INFO(...)				::Log::PrintMessage(::Log::Type::Launcher, ::Log::Level::Info,  __VA_ARGS__)
#define LAUNCHER_LOG_WARN(...)				::Log::PrintMessage(::Log::Type::Launcher, ::Log::Level::Warn,  __VA_ARGS__)
#define LAUNCHER_LOG_ERROR(...)				::Log::PrintMessage(::Log::Type::Launcher, ::Log::Level::Error, __VA_ARGS__)
#define LAUNCHER_LOG_CRITICAL(...)			::Log::PrintMessage(::Log::Type::Launcher, ::Log::Level::Fatal, __VA_ARGS__)

#define LAUNCHER_LOG_TRACE_TAG(tag, ...)	::Log::PrintMessageTag(::Log::Type::Launcher, ::Log::Level::Trace, tag, __VA_ARGS__)
#define LAUNCHER_LOG_INFO_TAG(tag, ...)		::Log::PrintMessageTag(::Log::Type::Launcher, ::Log::Level::Info, tag, __VA_ARGS__)
#define LAUNCHER_LOG_WARN_TAG(tag, ...)		::Log::PrintMessageTag(::Log::Type::Launcher, ::Log::Level::Warn, tag, __VA_ARGS__)
#define LAUNCHER_LOG_ERROR_TAG(tag, ...)	::Log::PrintMessageTag(::Log::Type::Launcher, ::Log::Level::Error, tag, __VA_ARGS__)
#define LAUNCHER_LOG_CRITICAL_TAG(tag, ...) ::Log::PrintMessageTag(::Log::Type::Launcher, ::Log::Level::Fatal, tag, __VA_ARGS__)

//-------------------------------------------------------

#define VULKAN_LOG_TRACE(...)				::Log::PrintMessage(::Log::Type::VulkanDebug, ::Log::Level::Trace, __VA_ARGS__)
#define VULKAN_LOG_INFO(...)				::Log::PrintMessage(::Log::Type::VulkanDebug, ::Log::Level::Info,  __VA_ARGS__)
#define VULKAN_LOG_WARN(...)				::Log::PrintMessage(::Log::Type::VulkanDebug, ::Log::Level::Warn,  __VA_ARGS__)
#define VULKAN_LOG_ERROR(...)				::Log::PrintMessage(::Log::Type::VulkanDebug, ::Log::Level::Error, __VA_ARGS__)
#define VULKAN_LOG_CRITICAL(...)			::Log::PrintMessage(::Log::Type::VulkanDebug, ::Log::Level::Fatal, __VA_ARGS__)

#define VULKAN_LOG_TRACE_TAG(tag, ...)	    ::Log::PrintMessageTag(::Log::Type::VulkanDebug, ::Log::Level::Trace, tag, __VA_ARGS__)
#define VULKAN_LOG_INFO_TAG(tag, ...)		::Log::PrintMessageTag(::Log::Type::VulkanDebug, ::Log::Level::Info, tag, __VA_ARGS__)
#define VULKAN_LOG_WARN_TAG(tag, ...)		::Log::PrintMessageTag(::Log::Type::VulkanDebug, ::Log::Level::Warn, tag, __VA_ARGS__)
#define VULKAN_LOG_ERROR_TAG(tag, ...)	    ::Log::PrintMessageTag(::Log::Type::VulkanDebug, ::Log::Level::Error, tag, __VA_ARGS__)
#define VULKAN_LOG_CRITICAL_TAG(tag, ...)   ::Log::PrintMessageTag(::Log::Type::VulkanDebug, ::Log::Level::Fatal, tag, __VA_ARGS__)

//-------------------------------------------------------

#define LAUNCHER_TRACE(...)					::Log::GetLauncherLogger()->trace(__VA_ARGS__)
#define LAUNCHER_INFO(...)					::Log::GetLauncherLogger()->info(__VA_ARGS__)
#define LAUNCHER_WARN(...)					::Log::GetLauncherLogger()->warn(__VA_ARGS__)
#define LAUNCHER_ERROR(...)					::Log::GetLauncherLogger()->error(__VA_ARGS__)
#define LAUNCHER_CRITICAL(...)				::Log::GetLauncherLogger()->critical(__VA_ARGS__)

#define EDITOR_TRACE(...)					::Log::GetEditorLogger()->trace(__VA_ARGS__)
#define EDITOR_INFO(...)					::Log::GetEditorLogger()->info(__VA_ARGS__)
#define EDITOR_WARN(...)					::Log::GetEditorLogger()->warn(__VA_ARGS__)
#define EDITOR_ERROR(...)					::Log::GetEditorLogger()->error(__VA_ARGS__)
#define EDITOR_CRITICAL(...)				::Log::GetEditorLogger()->critical(__VA_ARGS__)

#define VULKAN_CONSOLE_LOG_TRACE(...)		::Log::GetVulkanLogger()->trace(__VA_ARGS__)
#define VULKAN_CONSOLE_LOG_INFO(...)		::Log::GetVulkanLogger()->info(__VA_ARGS__)
#define VULKAN_CONSOLE_LOG_WARN(...)		::Log::GetVulkanLogger()->warn(__VA_ARGS__)
#define VULKAN_CONSOLE_LOG_ERROR(...)		::Log::GetVulkanLogger()->error(__VA_ARGS__)
#define VULKAN_CONSOLE_LOG_CRITICAL(...)	::Log::GetVulkanLogger()->critical(__VA_ARGS__)

//-------------------------------------------------------

#define EDITOR_ASSERT(x, ...)                                                                                               \
	if (!(x))                                                                                                               \
	{                                                                                                                       \
		EDITOR_LOG_CRITICAL("Assertion Failed: {0}", __VA_ARGS__);                                                          \
		__debugbreak();                                                                                                     \
	}                                                                                                                       \
	__debugbreak();                                                                                                         \
	abort();

#define ASSERT(condition, ...)                                                                                             \
	{                                                                                                                      \
		if (!(condition))                                                                                                  \
		{                                                                                                                  \
			EDITOR_LOG_ERROR("[ASSERTION FAILED] {0}", __VA_ARGS__);                                                       \
			abort();                                                                                                       \
		}                                                                                                                  \
	}

/*
*
* @brief A macro to log a message with the specified log level.
*
* This macro logs a message with the specified log level to the console and the log file.
*/
#ifdef SEDX_DEBUG
	#define DEBUG_ASSERT(condition, ...)                                                                                   \
		{                                                                                                                  \
			if (!(condition))                                                                                              \
			{                                                                                                              \
				EDITOR_LOG_ERROR("[ASSERTION FAILED] {0}", __VA_ARGS__);                                                   \
				SEDX_DEBUGBREAK();                                                                                         \
			}                                                                                                              \
		}
	#define DEBUG_VK(result, ...)                                                                                          \
		{                                                                                                                  \
			if ((result) != VK_SUCCESS)                                                                                    \
			{                                                                                                              \
				EDITOR_LOG_ERROR("[VULKAN ERROR = {0}] {1}", VK_ERROR_STRING((result)), __VA_ARGS__);                      \
				SEDX_DEBUGBREAK();                                                                                         \
			}                                                                                                              \
		}
	#define DEBUG_TRACE(...) Log::GetEditorLogger()->trace(__VA_ARGS__)
#else
	#define DEBUG_TRACE(...)
	#define DEBUG_ASSERT(condition, ...)
	#define DEBUG_VK(result, ...)
#endif

// ----------------------------------------------------

template <typename... Args>
void Log::PrintMessage(Log::Type type, Log::Level level, fmt::format_string<Args...> format, Args &&...args)
{
    auto &detail = EnabledTags_[""];
    if (detail.Enabled && detail.LevelFilter <= level)
    {
        auto &logger = (type == Type::Editor) ? GetEditorLogger() :
			((type == Type::VulkanDebug) ? GetVulkanLogger() : GetLauncherLogger());
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
void Log::PrintMessageTag(Log::Type type, Log::Level level, fmt::string_view tag, const fmt::format_string<Args...> format, Args &&...args)
{
    auto &detail = EnabledTags_[std::string(tag.data(), tag.size())];
    if (detail.Enabled && detail.LevelFilter <= level)
    {
        auto &logger = (type == Type::Editor) ? GetEditorLogger() : 
                     ((type == Type::VulkanDebug) ? GetVulkanLogger() : GetLauncherLogger());
        std::string formatted = fmt::format(format, std::forward<Args>(args)...);
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

inline void Log::PrintMessageTag(Log::Type type, Log::Level level, fmt::string_view tag, fmt::string_view message)
{
    auto &detail = EnabledTags_[std::string(tag.data(), tag.size())];  // Convert fmt::string_view to std::string properly
    if (detail.Enabled && detail.LevelFilter <= level)
    {
        auto &logger = (type == Type::Editor) ? GetEditorLogger() : 
                     ((type == Type::VulkanDebug) ? GetVulkanLogger() : GetLauncherLogger());
        switch (level)
        {
        case Level::Trace:
            logger->trace("[{}] {}", tag, message);  // Use curly braces instead of {0} for fmt format strings
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

inline void Log::PrintAssertMessage(Log::Type type, fmt::string_view prefix)
{
    auto &logger = (type == Type::Editor) ? GetEditorLogger() :
        ((type == Type::VulkanDebug) ? GetVulkanLogger() : GetLauncherLogger());
    logger->error("{}", prefix);
#if ASSERT_MESSAGE_BOX
    MessageBoxA(nullptr, "- No message -", "Scenery Editor X | Assert", MB_OK | MB_ICONERROR);
#endif
}

// ----------------------------------------------------

template <typename... Args>
void Log::PrintAssertMessage(Log::Type type, fmt::string_view prefix, fmt::format_string<Args...> message, Args &&...args)
{
    auto &logger = (type == Type::Editor) ? GetEditorLogger() : 
                 ((type == Type::VulkanDebug) ? GetVulkanLogger() : GetLauncherLogger());
    auto formatted = fmt::format(message, std::forward<Args>(args)...);
    logger->error("{}: {}", prefix, formatted);
#if ASSERT_MESSAGE_BOX
    MessageBoxA(nullptr, formatted.c_str(), "Scenery Editor X | Assert", MB_OK | MB_ICONERROR);
#endif
}

// ----------------------------------------------------


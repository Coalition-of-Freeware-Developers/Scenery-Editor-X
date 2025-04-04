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
#include <memory>
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
		Launcher = 1
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
	static std::shared_ptr<spdlog::logger> &GetEditorLogger() { return _EditorLogger; }
	static std::shared_ptr<spdlog::logger> &GetLauncherLogger() { return _LauncherLogger; }

private:

	/**
	 * @brief The logger instances for the Editor Logger, launcher Logger, and Vulkan Debug Logger.
	 */
	static std::shared_ptr<spdlog::logger> _EditorLogger; 
	static std::shared_ptr<spdlog::logger> _LauncherLogger;
    static std::shared_ptr<spdlog::logger> _VulkanLogger;
};



/**
*
* Launcher Log Macros
* 
* @brief A macro to log a message with the specified log level.
*
* This macro logs a message with the specified log level to the console and the log file.
*/
#define LAUNCHER_LOG_TRACE(...)         Log::GetEditorLogger()->trace(__VA_ARGS__)
#define LAUNCHER_LOG_INFO(...)          Log::GetEditorLogger()->info(__VA_ARGS__)
#define LAUNCHER_LOG_WARN(...)          Log::GetEditorLogger()->warn(__VA_ARGS__)
#define LAUNCHER_LOG_ERROR(...)         Log::GetEditorLogger()->error(__VA_ARGS__)
#define LAUNCHER_LOG_CRITICAL(...)      Log::GetEditorLogger()->critical(__VA_ARGS__);
#define LAUNCHER_ASSERT(x, ...)                                                                                             \
	if (!(x))                                                                                                               \
	{                                                                                                                       \
		LAUNCHER_LOG_CRITICAL("Assertion Failed: {0}", __VA_ARGS__);                                                        \
		__debugbreak();                                                                                                     \
	}                                                                                                                       \
	__debugbreak();                                                                                                         \
	abort();

/*
* 
* Editor Log Macros
*
*/
#define EDITOR_LOG_TRACE(...)           Log::GetEditorLogger()->trace(__VA_ARGS__)
#define EDITOR_LOG_INFO(...)            Log::GetEditorLogger()->info(__VA_ARGS__)
#define EDITOR_LOG_WARN(...)            Log::GetEditorLogger()->warn(__VA_ARGS__)
#define EDITOR_LOG_ERROR(...)           Log::GetEditorLogger()->error(__VA_ARGS__)
#define EDITOR_LOG_CRITICAL(...)        Log::GetEditorLogger()->critical(__VA_ARGS__);
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

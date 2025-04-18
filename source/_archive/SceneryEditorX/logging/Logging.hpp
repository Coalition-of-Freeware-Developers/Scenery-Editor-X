#pragma once

#include <spdlog/spdlog.h>

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <iostream>

#ifdef _WIN32
    #ifndef _WIN64
        #error "x86 not supported!"
    #endif
#define SEDX_PLATFORM_WINDOWS
#endif

#ifdef __linux__
    #define SEDX_PLATFORM_LINUX
#endif

#ifdef SEDX_DEBUG
    #ifdef SEDX_PLATFORM_WINDOWS
        #define SEDX_DEBUGBREAK() __debugbreak()
    #endif
    #ifdef SEDX_PLATFORM_LINUX
        #include <signal.h>
        #define SEDX_DEBUGBREAK() raise(SIGTRAP)
    #endif
#else
    #define SEDX_DEBUGBREAK()
#endif

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
    * @brief Shuts down the logger instance.
    *
    * This function shuts down the logger instance and releases any resources
    * associated with it. It should be called before the application exits to
    * ensure that all log messages are properly flushed.
    */
    static void Shutdown();

    /**
     * @brief Logs system information to the console.
     *
     * This function logs system information to the console. It logs the
     * operating system version, processor architecture, number of processors,
     * page size, and processor type.
     */
    static void LogHeader();

    /**
     * @brief Gets the logger instance.
     * @return A shared pointer to the logger instance.
     *
     * This function returns a shared pointer to the logger instance.
     */
    static std::shared_ptr<spdlog::logger> &GetEditorLogger() { return _EditorLogger; }
    static std::shared_ptr<spdlog::logger> &GetLauncherLogger() { return _LauncherLogger; }

    private:

    static std::shared_ptr<spdlog::logger> _EditorLogger; //< The logger instance.
    static std::shared_ptr<spdlog::logger> _LauncherLogger; //< The logger instance.
};

/**
*
* Launcher Log Macros
* 
* @brief A macro to log a message with the specified log level.
*
* This macro logs a message with the specified log level to the console and the log file.
*/
#define LAUNCHER_LOG_TRACE(...)         Log::GetLauncherLogger()->trace(__VA_ARGS__)
#define LAUNCHER_LOG_INFO(...)          Log::GetLauncherLogger()->info(__VA_ARGS__)
#define LAUNCHER_LOG_WARN(...)          Log::GetLauncherLogger()->warn(__VA_ARGS__)
#define LAUNCHER_LOG_ERROR(...)         Log::GetLauncherLogger()->error(__VA_ARGS__)
#define LAUNCHER_LOG_CRITICAL(...)      Log::GetLauncherLogger()->critical(__VA_ARGS__);
#define LAUNCHER_ASSERT(x, ...)                                                                                        \
    if (!(x))                                                                                                          \
    {                                                                                                                  \
        LAUNCHER_LOG_CRITICAL("Assertion Failed: {0}", __VA_ARGS__);                                                   \
        __debugbreak();                                                                                                \
    } __debugbreak(); \ abort();

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
#define EDITOR_ASSERT(x, ...)                                                                                          \
    if (!(x))                                                                                                          \
    {                                                                                                                  \
        EDITOR_LOG_CRITICAL("Assertion Failed: {0}", __VA_ARGS__);                                                     \
        __debugbreak();                                                                                                \
    }                                                                                                                  \
    __debugbreak(); \ abort();

/*
*
* @brief A macro to log a message with the specified log level.
*
* This macro logs a message with the specified log level to the console and the log file.
*/
#ifdef SEDX_DEBUG
    #ifdef EDITOR
        #define DEBUG_ASSERT(condition, ...)                                                                                   \
            {                                                                                                                  \
                if (!(condition))                                                                                              \
                {                                                                                                              \
                    EDITOR_LOG_ERROR("[ASSERTION FAILED] {0}", __VA_ARGS__);                                                          \
                    SEDX_DEBUGBREAK();                                                                                         \
                }                                                                                                              \
            }
        #define ASSERT(condition, ...)                                                                                         \
            {                                                                                                                  \
                if (!(condition))                                                                                              \
                {                                                                                                              \
                    EDITOR_LOG_ERROR("[ASSERTION FAILED] {0}", __VA_ARGS__);                                                          \
                    abort();                                                                                                   \
                }                                                                                                              \
            }
        #define DEBUG_VK(res, ...)                                                                                             \
            {                                                                                                                  \
                if ((res) != VK_SUCCESS)                                                                                       \
                {                                                                                                              \
                    EDITOR_LOG_ERROR("[VULKAN ERROR = {0}] {1}", VK_ERROR_STRING((res)), __VA_ARGS__);                                \
                    SEDX_DEBUGBREAK();                                                                                         \
                }                                                                                                              \
            }

        #define DEBUG_TRACE(...) Log::GetEditorLogger()->trace(__VA_ARGS__)
    #elif defined(LAUNCHER)
        #define DEBUG_TRACE(...) Log::GetLauncherLogger()->trace(__VA_ARGS__)
    #else
        #define DEBUG_TRACE(...)
    #endif
#else
    #define DEBUG_ASSERT(condition, ...)
    #define DEBUG_VK(res, ...)
#endif

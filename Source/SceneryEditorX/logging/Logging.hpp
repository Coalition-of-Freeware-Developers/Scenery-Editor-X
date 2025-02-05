#pragma once

#include <spdlog/spdlog.h>

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

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
    static std::shared_ptr<spdlog::logger>& GetEditorLogger()   { return _EditorLogger; }
    static std::shared_ptr<spdlog::logger>& GetLauncherLogger() { return _LauncherLogger; }

private:
    static std::shared_ptr<spdlog::logger> _EditorLogger; //< The logger instance.
    static std::shared_ptr<spdlog::logger> _LauncherLogger; //< The logger instance.
};

// Launcher Log Macros

#define LAUNCHER_LOG_TRACE(...)         Log::Get()->trace(__VA_ARGS__)
#define LAUNCHER_LOG_INFO(...)          Log::Get()->info(__VA_ARGS__)
#define LAUNCHER_LOG_WARN(...)          Log::Get()->warn(__VA_ARGS__)
#define LAUNCHER_LOG_ERROR(...)         Log::Get()->error(__VA_ARGS__)
#define LAUNCHER_LOG_CRITICAL(...)      Log::Get()->critical(__VA_ARGS__);
#define LAUNCHER_ASSERT(x, ...)                                                                                        \
    if (!(x))                                                                                                          \
    {                                                                                                                  \
        LAUNCHER_LOG_CRITICAL("Assertion Failed: {0}", __VA_ARGS__);                                                   \
        __debugbreak();                                                                                                \
    } __debugbreak(); \ abort();

#ifdef SEDX_DEBUG
#define DEBUG_TRACE(...) Log::Get()->trace(__VA_ARGS__)
#else
#define DEBUG_TRACE(...)
#endif

#pragma once

#include <memory>
#include <spdlog/spdlog.h>

/**
 * @class Log
 * @brief A static class for initializing and accessing the logger.
 *
 * This class provides a static interface for initializing the logger and
 * accessing the logger instance. It uses the spdlog library for logging.
 */
namespace Launcher
{
    class Log
    {
    public:
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
             * @brief Gets the logger instance.
             * @return A shared pointer to the logger instance.
             *
             * This function returns a shared pointer to the logger instance.
             */
        static std::shared_ptr<spdlog::logger> &Get()
        {
            return _logger;
        }
    
    private:
        static std::shared_ptr<spdlog::logger> _logger; ///< The logger instance.
    
    
    #define LOG_INFO(...) Log::Get()->info(__VA_ARGS__)
    #define LOG_WARN(...) Log::Get()->warn(__VA_ARGS__)
    #define LOG_ERROR(...) Log::Get()->error(__VA_ARGS__)
    #define LOG_CRITICAL(...)                                                                                              \
        Log::Get()->critical(__VA_ARGS__);                                                                                 \
        abort();
    
    #ifdef SEDX_DEBUG
    #define DEBUG_TRACE(...) Log::Get()->trace(__VA_ARGS__)
    #else
    #define DEBUG_TRACE(...)
    #endif
    };
} // namespace Launcher

/**
* -------------------------------------------------------
* Scenery Editor X | Crash Handler Service
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* crash_handler.h
* -------------------------------------------------------
* Created: 13/05/2025
* -------------------------------------------------------
*/
#pragma once
#include <CrashHandler/service_ipc.h>
#include <functional>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>

/// -------------------------------------------------------

namespace CrashHandler
{
	class CrashHandlerImpl; /// PIMPL for platform-specific implementations

	class CrashService
    {
	public:
	    struct CrashHandlerConfig
	    {
	        std::string appName = "CrashHandler";
	        std::string companyName = "Coalition-of-Freeware-Developers";
	        std::string appVersion;
	        std::string dumpDir;
	        bool enableBackgroundService = true;
	        int heartbeatIntervalMs = 5000;

	        /// Callback when a crash is detected
	        std::function<void(const std::string& dumpPath)> onCrashDetectedCallback;
	    };

	    /// Initialize crash handler in the main application
        static bool Init(const CrashHandlerConfig &config);

        ~CrashService();

	    /// Shut down the crash handler
	    static void Shutdown();

	    /// Send a heartbeat to the crash handler service
	    static void Tick();

	    /// Add custom crash data
	    static void addCrashData(const std::string& key, const std::string& value);

	    /// Write a minidump manually (for non-crash scenarios)
	    static bool WriteDump(const std::string& reason);

	private:
        CrashHandlerConfig m_config;
        std::shared_ptr<BackgroundService> m_backgroundService;
	    static std::unique_ptr<CrashHandlerImpl> s_impl;
        std::string dumpPath;
        std::thread m_heartbeatThread;

        static bool filterCallback(void *context)
        {
            return true; /// Process all crashes
        }

        /*
        static bool minidumpCallback(const wchar_t *dump_path,
                                     const wchar_t *minidump_id,
                                     void *context,
                                     EXCEPTION_POINTERS *exinfo,
                                     MDRawAssertionInfo *assertion,
                                     bool succeeded)
        {
            if (succeeded)
            {
                CrashHandlerImpl *self = static_cast<CrashHandlerImpl *>(context);
                std::string dumpPath = utf8FromWchar(dump_path) + std::string("\\") + utf8FromWchar(minidump_id) + ".dmp";

                spdlog::critical("Application crashed. Minidump written to: {}", dumpPath);

                /// Notify the background service about the crash
                self->notifyBackgroundService(dumpPath);

                /// Invoke user callback if provided
                if (self->m_config.onCrashDetectedCallback)
                {
                    self->m_config.onCrashDetectedCallback(dumpPath);
                }
            }
            else
            {
                spdlog::error("Failed to write minidump");
            }

            return succeeded;
        }
        */

        /// Platform-specific background service and IPC methods...
        /// ...


	};

} // namespace CrashHandler

/// -------------------------------------------------------

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
#include <functional>
#include <memory>
#include <string>
#include <Windows.h>
#include <chrono>
#include <iomanip>

/// -------------------------------------------------------

namespace CrashHandler
{
    /// Forward declarations
	class IPCClient;
    class CrashHandlerImpl;
	
	/// Initialize Windows native exception handler
	bool InitializeNativeCrashHandler(const std::string &dumpPath,
	                                  const std::string &applicationId,
	                                  std::function<void(const std::string &)> crashCallback = nullptr);
	
	/// Generate a crash dump manually (for non-crash errors)
	void GenerateDump(const std::string &reason);
	
	/// Windows exception filter function
	LONG WINAPI UnhandledExceptionFilter(EXCEPTION_POINTERS *exceptionPointers);

	std::string getCurrentTimestamp()
    {
        const auto now = std::chrono::system_clock::now();
        const auto timeT = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_s(&tm, &timeT);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
        return oss.str();
    }

	/// Client API for applications to use
	class ClientAPI
	{
	public:
	    ClientAPI(const std::string &applicationId);
	    ~ClientAPI();
	
	    /// Initialize crash handler
	    bool initialize(const std::string &dumpPath = "");
	
	    /// Send heartbeat to crash handler service
	    void sendHeartbeat();
	
	    /// Log an error (will be collected by crash handler service)
	    void logError(const std::string &errorMessage);
	
	    /// Update project state (so crash handler knows what project was open)
	    void updateProjectState(const std::string &projectPath) const;
	
	    /// Manually trigger a crash report for a non-fatal error
	    void reportNonFatalError(const std::string &errorMessage);
	
	private:
	    std::string m_applicationId;
	    std::string m_dumpPath;
	    std::shared_ptr<IPCClient> m_ipcClient;
	    std::function<void(const std::string &)> m_crashCallback;
	    bool m_isInitialized;
	};

    class CrashService
    {
    public:
        struct CrashHandlerConfig
        {
            std::string dumpDir;
        };

        static bool Init(const CrashHandlerConfig &config);
        static void Shutdown();
        static void Tick();
        static void addCrashData(const std::string &key, const std::string &value);
        static bool WriteDump(const std::string &reason);

        ~CrashService();

    private:
        static std::unique_ptr<CrashHandlerImpl> s_impl;
    };

} // namespace CrashHandler

/// -------------------------------------------------------

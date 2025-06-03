/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* native_crash_handler.cpp
* -------------------------------------------------------
* Created: 2/6/2025
* -------------------------------------------------------
*/
#include <CrashHandler/crash_handler.h>
#include <CrashHandler/service_ipc.h>
#include <DbgHelp.h>
#include <filesystem>
#include <functional>
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <Windows.h>

#include <nlohmann/json.hpp>

#pragma comment(lib, "Dbghelp.lib")

/// -------------------------------------------------------

namespace CrashHandler
{
	/// Global variables to store callbacks and dump path
	static std::string g_dumpPath;
	static std::string g_applicationId;
	static std::function<void(const std::string &)> g_crashCallback;
	static std::shared_ptr<IPCClient> g_ipcClient;
	static LPTOP_LEVEL_EXCEPTION_FILTER g_previousExceptionFilter = nullptr;
	
	/// Write minidump file using Windows DbgHelp
	bool WriteMiniDump(EXCEPTION_POINTERS *exceptionPointers, const std::string &dumpFilePath)
	{
	    HANDLE hFile =
	        CreateFileA(dumpFilePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
	    if (hFile == INVALID_HANDLE_VALUE)
	    {
	        spdlog::error("Failed to create dump file: {}", GetLastError());
	        return false;
	    }
	
	    MINIDUMP_EXCEPTION_INFORMATION exInfo;
	    exInfo.ThreadId = GetCurrentThreadId();
	    exInfo.ExceptionPointers = exceptionPointers;
	    exInfo.ClientPointers = FALSE;
	
	    /// Write a comprehensive minidump with full memory
	    MINIDUMP_TYPE dumpType =
	        (MINIDUMP_TYPE)(MiniDumpWithFullMemory | MiniDumpWithFullMemoryInfo | MiniDumpWithHandleData |
	                        MiniDumpWithUnloadedModules | MiniDumpWithThreadInfo);
	
	    BOOL result = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, dumpType, &exInfo, NULL, NULL);
	
	    CloseHandle(hFile);
	
	    if (!result)
	    {
	        spdlog::error("Failed to write minidump: {}", GetLastError());
	        return false;
	    }
	
	    spdlog::info("Minidump successfully written to: {}", dumpFilePath);
	    return true;
	}
	
	/// Unhandled exception filter function
	LONG WINAPI UnhandledExceptionFilter(EXCEPTION_POINTERS *exceptionPointers)
	{
	    spdlog::error("Unhandled exception detected");
	
	    /// Generate timestamp for unique filename
	    auto now = std::chrono::system_clock::now();
	    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
	
	    /// Create dump directory if it doesn't exist
	    std::filesystem::path dumpDir(g_dumpPath);
	    if (!std::filesystem::exists(dumpDir))
	    {
	        std::filesystem::create_directories(dumpDir);
	    }
	
	    /// Create dump file path
	    std::string dumpFilename = g_applicationId + "_" + std::to_string(timestamp) + ".dmp";
	    std::string dumpFilePath = (dumpDir / dumpFilename).string();
	
	    /// Write the minidump
	    bool success = WriteMiniDump(exceptionPointers, dumpFilePath);
	
	    if (success)
	    {
	        /// Notify via IPC if client is available
	        if (g_ipcClient && g_ipcClient->isConnected())
	        {
	            IPCMessage crashMsg{MessageType::CRASH, dumpFilePath, getCurrentTimestamp(), g_applicationId};
	            g_ipcClient->sendMessage(crashMsg);
	            spdlog::info("Crash notification sent via IPC");
	        }
	
	        /// Call the crash callback if registered
	        if (g_crashCallback)
	        {
	            g_crashCallback(dumpFilePath);
	        }
	    }
	
	    /// Call previous exception filter if available
	    if (g_previousExceptionFilter)
	    {
	        return g_previousExceptionFilter(exceptionPointers);
	    }
	
	    return EXCEPTION_CONTINUE_SEARCH;
	}
	
	/// Initialize Windows native exception handler
	bool InitializeNativeCrashHandler(const std::string &dumpPath,
	                                  const std::string &applicationId,
	                                  std::function<void(const std::string &)> crashCallback)
	{
	    /// Store global variables
	    g_dumpPath = dumpPath.empty() ? std::filesystem::temp_directory_path().string() : dumpPath;
	    g_applicationId = applicationId;
	    g_crashCallback = crashCallback;
	
	    /// Create dump directory if it doesn't exist
	    std::filesystem::path dumpDir(g_dumpPath);
	    if (!std::filesystem::exists(dumpDir))
	    {
	        std::filesystem::create_directories(dumpDir);
	    }
	
	    /// Initialize IPC client if needed
	    if (!g_ipcClient)
	    {
	        g_ipcClient = std::make_shared<IPCClient>(applicationId);
	        if (!g_ipcClient->connect())
	        {
	            spdlog::warn("Failed to connect to crash handler service");
	            /// Continue without IPC - local handling will still work
	        }
	    }
	
	    /// Set up Windows unhandled exception filter
	    g_previousExceptionFilter = SetUnhandledExceptionFilter(UnhandledExceptionFilter);
	
	    spdlog::info("Native crash handler initialized for application: {}", applicationId);
	    return true;
	}
	
	/// Function to manually trigger a crash dump (useful for non-crash errors)
	void GenerateDump(const std::string &reason)
	{
	    spdlog::info("Manually generating crash dump. Reason: {}", reason);
	
	    /// Create an artificial exception to generate a dump
	    __try
	    {
	        RaiseException(0xE0000001, 0, 0, NULL);
	    }
        __except (CrashHandler::UnhandledExceptionFilter(static_cast<EXCEPTION_POINTERS *>(GetExceptionInformation())),
				  EXCEPTION_EXECUTE_HANDLER)
	    {
	        /// Exception handled by our filter
	    }
	}
	
	/// Client API implementation
	ClientAPI::ClientAPI(const std::string &applicationId)
	    : m_applicationId(applicationId), m_dumpPath(std::filesystem::temp_directory_path().string()),
	      m_isInitialized(false)
	{
	}
	
	ClientAPI::~ClientAPI()
	{
	    /// Disconnect from IPC service
	    if (m_ipcClient && m_ipcClient->isConnected())
	    {
	        m_ipcClient->disconnect();
	    }
	}
	
	bool ClientAPI::initialize(const std::string &dumpPath)
	{
	    if (m_isInitialized)
	    {
	        spdlog::warn("Crash handler already initialized");
	        return true;
	    }
	
	    if (!dumpPath.empty())
	    {
	        m_dumpPath = dumpPath;
	    }
	
	    /// Initialize IPC client
	    m_ipcClient = std::make_shared<IPCClient>(m_applicationId);
	    bool ipcConnected = m_ipcClient->connect();
	
	    if (!ipcConnected)
	    {
	        spdlog::warn("Failed to connect to crash handler service, crashes will be handled locally");
	    }
	
	    /// Set crash callback
	    m_crashCallback = [this](const std::string &dumpPath) {
	        spdlog::info("Crash detected: {}", dumpPath);
	
	        /// If IPC failed, show dialog directly
	        if (!m_ipcClient || !m_ipcClient->isConnected())
	        {
	            /// This would normally be handled by the service
	            /// In this case, we need to create a dialog directly
	            /// This implementation is simplified - in reality, you'd have a separate process for this
	            MessageBoxA(nullptr,
                            "Scenery Editor X has crashed. A crash report has been generated.",
                            "Scenery Editor X - Crash Detected",
                            MB_OK | MB_ICONERROR);
	        }
	    };
	
	    /// Initialize native crash handler
	    bool result = InitializeNativeCrashHandler(m_dumpPath, m_applicationId, m_crashCallback);
	    if (result)
	    {
	        m_isInitialized = true;
	
	        /// Send startup message to the service
	        if (ipcConnected)
	        {
	            IPCMessage startupMsg{MessageType::STARTUP, "Application starting", getCurrentTimestamp(), m_applicationId};
	            m_ipcClient->sendMessage(startupMsg);
	        }
	    }
	
	    return result;
	}
	
	void ClientAPI::sendHeartbeat()
	{
	    if (!m_isInitialized || !m_ipcClient || !m_ipcClient->isConnected())
	    {
	        return;
	    }
	
	    IPCMessage heartbeatMsg{MessageType::HEARTBEAT, "Application running", getCurrentTimestamp(), m_applicationId};
	    m_ipcClient->sendMessage(heartbeatMsg);
	}
	
	void ClientAPI::logError(const std::string &errorMessage)
	{
	    if (!m_isInitialized || !m_ipcClient || !m_ipcClient->isConnected())
	    {
	        spdlog::error("Error logged but not sent to service: {}", errorMessage);
	        return;
	    }
	
	    IPCMessage errorMsg{MessageType::ERROR_LOG, errorMessage, getCurrentTimestamp(), m_applicationId};
	    m_ipcClient->sendMessage(errorMsg);
	}
	
	void ClientAPI::updateProjectState(const std::string &projectPath) const
    {
	    if (!m_isInitialized || !m_ipcClient || !m_ipcClient->isConnected())
	    {
	        return;
	    }
	
	    /// Create JSON with project details
	    nlohmann::json projectJson;
	    projectJson["projectPath"] = projectPath;
	
	    IPCMessage projectMsg{MessageType::PROJECT_STATE, projectJson.dump(), getCurrentTimestamp(), m_applicationId};
	    m_ipcClient->sendMessage(projectMsg);
	}
	
	void ClientAPI::reportNonFatalError(const std::string &errorMessage)
	{
	    /// Log the error
	    logError(errorMessage);
	
	    /// Generate a non-fatal crash report
	    GenerateDump("Non-fatal error: " + errorMessage);
	}

} // namespace CrashHandler

/// -------------------------------------------------------

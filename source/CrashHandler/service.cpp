/**
* -------------------------------------------------------
* Scenery Editor X | Crash Handler Service
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* service.cpp
* -------------------------------------------------------
* Created: 13/05/2025
* -------------------------------------------------------
*/
#include <chrono>
#include <CrashHandler/crash_handler.h>
#include <CrashHandler/dialog.h>
#include <CrashHandler/service_ipc.h>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <Psapi.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <thread>
#include <Windows.h>
#include <minidumpapiset.h>

using json = nlohmann::json;

/// -------------------------------------------------------

namespace CrashHandler
{
	static void setupLogging()
	{
	    const auto logDir = std::filesystem::temp_directory_path() / "SceneryEditorX_Logs";
	    if (!std::filesystem::exists(logDir))
	    {
	        std::filesystem::create_directories(logDir);
	    }
	
	    const auto logFile = logDir / "SceneryEditorX_CrashHandler.log";
	    const auto logger = spdlog::basic_logger_mt("crash_handler", logFile.string());
	    spdlog::set_default_logger(logger);
	    spdlog::set_level(spdlog::level::debug);
	    spdlog::info("Crash handler service started");
	}
	
	/// Implementation of BackgroundService
	BackgroundService::BackgroundService(const std::string &dumpDir)
	    : m_lastHeartbeatTime(getCurrentTimestamp()),
	      m_dumpDir(dumpDir.empty() ? std::filesystem::temp_directory_path() / "SceneryEditorX_Dumps" : dumpDir),
	      m_logDir(std::filesystem::temp_directory_path() / "SceneryEditorX_Logs"), m_isRunning(false)
	{
	    /// Create directories if they don't exist
	    if (!std::filesystem::exists(m_dumpDir))
	    {
	        std::filesystem::create_directories(m_dumpDir);
	    }
	    if (!std::filesystem::exists(m_logDir))
	    {
	        std::filesystem::create_directories(m_logDir);
	    }
	
	    /// Initialize IPC server
	    m_ipcServer = std::make_unique<IPCServer>();
	
	    /// Register message handlers
	    m_ipcServer->registerCallback(MessageType::STARTUP, [this](const IPCMessage &msg) { onStartupMessage(msg); });
	    m_ipcServer->registerCallback(MessageType::HEARTBEAT, [this](const IPCMessage &msg) { onHeartbeatMessage(msg); });
	    m_ipcServer->registerCallback(MessageType::ERROR_LOG, [this](const IPCMessage &msg) { onErrorLogMessage(msg); });
	    m_ipcServer->registerCallback(MessageType::CRASH, [this](const IPCMessage &msg) { onCrashMessage(msg); });
	    m_ipcServer->registerCallback(MessageType::SHUTDOWN, [this](const IPCMessage &msg) { onShutdownMessage(msg); });
	    m_ipcServer->registerCallback(MessageType::PROJECT_STATE,
	                                  [this](const IPCMessage &msg) { onProjectStateMessage(msg); });
	}
	
	BackgroundService::~BackgroundService()
	{
	    stop();
	}
	
	std::chrono::system_clock::time_point BackgroundService::parseTimeString(const std::string &timeStr)
	{
	    try
	    {
	        std::tm tm = {};
	        std::stringstream ss(timeStr);
	        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
	        auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
	        return tp;
	    }
	    catch (const std::exception &e)
	    {
	        spdlog::error("Failed to parse time string: {}", e.what());
	        return std::chrono::system_clock::now() - std::chrono::seconds(5);
	    }
	}
	
	void BackgroundService::start()
	{
	    if (m_isRunning)
	    {
	        spdlog::warn("Background service is already running");
	        return;
	    }
	
	    m_isRunning = true;
	    spdlog::info("Starting background service");
	
	    /// Start IPC server
	    m_ipcServer->start();
	
	    /// Start heartbeat thread
	    startHeartbeatThread();
	}
	
	void BackgroundService::stop()
	{
	    if (!m_isRunning)
	    {
	        return;
	    }
	
	    m_isRunning = false;
	    spdlog::info("Stopping background service");
	
	    /// Stop IPC server
	    m_ipcServer->stop();
	
	    /// Stop heartbeat thread
	    if (m_heartbeatThread.joinable())
	    {
	        m_heartbeatThread.join();
	    }
	}
	
	void BackgroundService::monitorApplication()
	{
	    while (m_isRunning)
	    {
	        /// Check if we've received recent heartbeats from all applications
	        auto now = std::chrono::system_clock::now();
	
	        {
	            std::lock_guard<std::mutex> lock(m_heartbeatMutex);
	            for (auto it = m_appHeartbeats.begin(); it != m_appHeartbeats.end();)
	            {
	                const auto &[appId, lastHeartbeat] = *it;
	                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastHeartbeat).count();
	
	                if (elapsed > 10)
	                {
	                    /// Application may have crashed without generating a dump
	                    spdlog::warn("No heartbeat received from {} in {} seconds, application may have crashed",
	                                 appId,
	                                 elapsed);
	
	                    /// Find most recent dump
	                    m_recentDump = findMostRecentDump();
	
	                    if (m_recentDump.empty())
	                    {
	                        /// No dump found, the app might be hanging or killed
	                        spdlog::info("No crash dump found, generating one for the hung process");
	                        generateDumpForHungProcess();
	                    }
	
	                    /// Collect logs and project data
	                    collectApplicationData(appId);
	
	                    /// Show dialog to user
	                    CrashDialog dialog;
	                    dialog.setCollectedLogs(m_collectedLogs);
	                    dialog.setProjectPath(m_currentProjectPath);
	                    dialog.showCrashDialog(m_recentDump);
	
	                    /// Remove this app from monitoring
	                    it = m_appHeartbeats.erase(it);
	                }
	                else
	                {
	                    ++it;
	                }
	            }
	        }
	
	        std::this_thread::sleep_for(std::chrono::seconds(1));
	    }
	}
	
	void BackgroundService::collectApplicationData(const std::string &applicationId)
	{
	    spdlog::info("Collecting application data for: {}", applicationId);
	
	    /// Clear previous collections
	    m_collectedLogs.clear();
	
	    /// Collect logs
	    collectLogs(applicationId, m_logDir.string());
	
	    /// Collect project data if available
	    if (!m_currentProjectPath.empty())
	    {
	        collectProjectData(m_currentProjectPath);
	    }
	}
	
	void BackgroundService::collectLogs(const std::string &applicationId, const std::string &logDir)
	{
	    spdlog::info("Collecting logs for: {} from {}", applicationId, logDir);
	
	    try
	    {
	        std::filesystem::path logDirPath(logDir);
	        if (!std::filesystem::exists(logDirPath))
	        {
	            spdlog::error("Log directory does not exist: {}", logDir);
	            return;
	        }
	
	        /// Create a directory to store collected logs
	        std::filesystem::path collectedLogsDir = m_dumpDir / "logs" / applicationId;
	        if (!std::filesystem::exists(collectedLogsDir))
	        {
	            std::filesystem::create_directories(collectedLogsDir);
	        }
	
	        /// Find and copy all log files
	        for (const auto &entry : std::filesystem::directory_iterator(logDirPath))
	        {
	            if (entry.is_regular_file() && entry.path().extension() == ".log")
	            {
	                std::string filename = entry.path().filename().string();
	                if (filename.find(applicationId) != std::string::npos)
	                {
	                    std::filesystem::path destPath = collectedLogsDir / filename;
	                    std::filesystem::copy_file(entry.path(),
	                                               destPath,
	                                               std::filesystem::copy_options::overwrite_existing);
	
	                    /// Store the log file path
	                    m_collectedLogs.push_back(destPath.string());
	                    spdlog::info("Collected log file: {}", destPath.string());
	                }
	            }
	        }
	    }
	    catch (const std::exception &e)
	    {
	        spdlog::error("Error collecting logs: {}", e.what());
	    }
	}
	
	void BackgroundService::collectProjectData(const std::string &projectPath)
	{
	    spdlog::info("Collecting project data from: {}", projectPath);
	
	    try
	    {
	        std::filesystem::path projectPathObj(projectPath);
	        if (!std::filesystem::exists(projectPathObj))
	        {
	            spdlog::error("Project path does not exist: {}", projectPath);
	            return;
	        }
	
	        /// Create a directory to store project data
	        std::filesystem::path projectDataDir = m_dumpDir / "project_data";
	        if (!std::filesystem::exists(projectDataDir))
	        {
	            std::filesystem::create_directories(projectDataDir);
	        }
	
	        /// Store project path for crash report
	        m_currentProjectPath = projectPath;
	
	        /// Copy project file (or directory tree if needed)
	        if (std::filesystem::is_regular_file(projectPathObj))
	        {
	            std::filesystem::path destPath = projectDataDir / projectPathObj.filename();
	            std::filesystem::copy_file(projectPathObj, destPath, std::filesystem::copy_options::overwrite_existing);
	            m_collectedProjectData = destPath.string();
	            spdlog::info("Collected project file: {}", destPath.string());
	        }
	        else if (std::filesystem::is_directory(projectPathObj))
	        {
	            /// Just record the path - copying the entire directory might be too large
	            m_collectedProjectData = projectPath;
	            spdlog::info("Project directory recorded: {}", projectPath);
	        }
	    }
	    catch (const std::exception &e)
	    {
	        spdlog::error("Error collecting project data: {}", e.what());
	    }
	}
	
	std::string BackgroundService::findMostRecentDump() const
	{
	    std::string recentDump;
	    std::filesystem::path dumpDir = m_dumpDir;
	
	    if (!std::filesystem::exists(dumpDir))
	    {
	        spdlog::error("Dump directory does not exist: {}", dumpDir.string());
	        return recentDump;
	    }
	
	    std::filesystem::file_time_type latestTime;
	    bool foundDump = false;
	
	    for (const auto &entry : std::filesystem::directory_iterator(dumpDir))
	    {
	        if (entry.path().extension() == ".dmp")
	        {
	            auto lastWriteTime = std::filesystem::last_write_time(entry);
	            if (!foundDump || lastWriteTime > latestTime)
	            {
	                recentDump = entry.path().string();
	                latestTime = lastWriteTime;
	                foundDump = true;
	            }
	        }
	    }
	
	    return recentDump;
	}
	
	void BackgroundService::generateDumpForHungProcess()
	{
	    spdlog::info("Generating dump for hung process");
	
	    /// Enumerate processes to find SceneryEditorX and/or Launcher
	    DWORD processes[1024], bytesNeeded;
	    if (!EnumProcesses(processes, sizeof(processes), &bytesNeeded))
	    {
	        spdlog::error("Failed to enumerate processes: {}", GetLastError());
	        return;
	    }
	
	    DWORD processCount = bytesNeeded / sizeof(DWORD);
	    for (DWORD i = 0; i < processCount; i++)
	    {
	        if (processes[i] == 0)
	            continue;
	
	        HANDLE hProcess =
	            OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_ALL_ACCESS, FALSE, processes[i]);
	        if (hProcess == NULL)
	            continue;
	
	        char processName[MAX_PATH] = "<unknown>";
	        HMODULE hMod;
	        DWORD cbNeeded;
	
	        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
	        {
	            GetModuleBaseNameA(hProcess, hMod, processName, sizeof(processName) / sizeof(char));
	        }
	
	        /// Check if this is one of our target applications
	        if (std::string(processName).find("SceneryEditorX") != std::string::npos ||
	            std::string(processName).find("Launcher") != std::string::npos)
	        {
	
	            spdlog::info("Found process: {} ({})", processName, processes[i]);
	
	            /// Generate dump file path
	            std::filesystem::path dumpPath =
	                m_dumpDir / (std::string(processName) + "_" + std::to_string(processes[i]) + ".dmp");
	
	            /// Create dump file
	            HANDLE hDumpFile = CreateFileA(dumpPath.string().c_str(),
	                                           GENERIC_WRITE,
	                                           0,
	                                           NULL,
	                                           CREATE_ALWAYS,
	                                           FILE_ATTRIBUTE_NORMAL,
	                                           NULL);
	
	            if (hDumpFile != INVALID_HANDLE_VALUE)
	            {
	                MINIDUMP_EXCEPTION_INFORMATION exInfo;
	                exInfo.ThreadId = 0;
	                exInfo.ExceptionPointers = NULL;
	                exInfo.ClientPointers = FALSE;
	
	                /// Write comprehensive minidump
	                MINIDUMP_TYPE dumpType =
	                    (MINIDUMP_TYPE)(MiniDumpWithFullMemory | MiniDumpWithFullMemoryInfo | MiniDumpWithHandleData |
	                                    MiniDumpWithUnloadedModules | MiniDumpWithThreadInfo);
	
	                /// Write minidump
	                if (MiniDumpWriteDump(hProcess, processes[i], hDumpFile, dumpType, &exInfo, NULL, NULL))
	                {
	                    spdlog::info("Successfully generated dump file: {}", dumpPath.string());
	                    m_recentDump = dumpPath.string();
	                }
	                else
	                {
	                    spdlog::error("Failed to write dump file: {}", GetLastError());
	                }
	
	                CloseHandle(hDumpFile);
	            }
	            else
	            {
	                spdlog::error("Failed to create dump file: {}", GetLastError());
	            }
	        }
	
	        CloseHandle(hProcess);
	    }
	}
	
	void BackgroundService::notifyCrash(const std::string &dumpPath)
	{
	    spdlog::info("Received crash notification: {}", dumpPath);
	    m_recentDump = dumpPath;
	
	    /// Find which application crashed
	    std::string appId = "Unknown";
	    for (const auto &id : m_appHeartbeats | std::views::keys)
	    {
	        if (dumpPath.find(id) != std::string::npos)
	        {
	            appId = id;
	            break;
	        }
	    }
	
	    /// Collect logs and project data
	    collectApplicationData(appId);
	
	    /// Show crash dialog
	    CrashDialog dialog;
	    dialog.setCollectedLogs(m_collectedLogs);
	    dialog.setProjectPath(m_currentProjectPath);
	    dialog.showCrashDialog(dumpPath);
	}
	
	void BackgroundService::handleErrorLog(const std::string &errorLog, const std::string &applicationId)
	{
	    spdlog::info("Handling error log from {}: {}", applicationId, errorLog);
	
	    /// Save error log to file
	    std::filesystem::path errorLogPath =
	        m_logDir / (applicationId + "_error_" +
	                    std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".log");
        if (std::ofstream logFile(errorLogPath); logFile.is_open())
	    {
	        logFile << errorLog;
	        logFile.close();
	        spdlog::info("Error log saved to: {}", errorLogPath.string());
	
	        /// Add to collected logs
	        m_collectedLogs.push_back(errorLogPath.string());
	    }
	}
	
	void BackgroundService::handleProjectState(const std::string &projectData)
	{
	    spdlog::info("Handling project state update");
	
	    /// Parse project data
	    try
	    {
	        json j = json::parse(projectData);
	        m_currentProjectPath = j["projectPath"].get<std::string>();
	        spdlog::info("Project path updated: {}", m_currentProjectPath);
	    }
	    catch (const std::exception &e)
	    {
	        spdlog::error("Failed to parse project data: {}", e.what());
	    }
	}
	
	void BackgroundService::sendHeartbeat()
	{
	    /// Send heartbeat to applications
	    IPCMessage heartbeatMsg{MessageType::HEARTBEAT, "Service running", getCurrentTimestamp(), "CrashHandlerService"};
	
	    std::lock_guard<std::mutex> lock(m_heartbeatMutex);
	    for (const auto &appId : m_appHeartbeats | std::views::keys)
	    {
	        m_ipcServer->sendMessage(appId, heartbeatMsg);
	    }
	}
	
	void BackgroundService::startHeartbeatThread()
	{
	    m_heartbeatThread = std::thread([this]() {
	        while (m_isRunning)
	        {
	            std::this_thread::sleep_for(std::chrono::seconds(5));
	            sendHeartbeat();
	        }
	    });
	}
	
	/// IPC message handlers
	///void BackgroundService::
	
	
} /// namespace CrashHandler

/// -------------------------------------------------------

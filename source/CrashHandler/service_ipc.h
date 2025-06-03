/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* service_ipc.h
* -------------------------------------------------------
* Created: 13/5/2025
* -------------------------------------------------------
*/
#pragma once
#include <chrono>
#include <fcntl.h>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>

/// -------------------------------------------------------

namespace CrashHandler
{
	/// IPC message types
	enum class MessageType : uint8_t
	{
	    STARTUP,
	    HEARTBEAT,
	    ERROR_LOG,
	    CRASH,
	    SHUTDOWN,
	    PROJECT_STATE
	};

    /// IPC message structure
    struct IPCMessage
    {
        MessageType type;
        std::string content;
        std::string timestamp;
        std::string applicationId; /// Either "Launcher" or "SceneryEditorX"
    };

    /// Callback function type for IPC message handlers
    using MessageCallback = std::function<void(const IPCMessage &)>;

    /// IPC Server class to handle communication with client applications
    class IPCServer
    {
    public:
        IPCServer(const std::string &pipeName = "SceneryEditorXCrashService");
        ~IPCServer();

        void start();
        void stop();
        void registerCallback(MessageType type, MessageCallback callback);
        bool sendMessage(const std::string &client, const IPCMessage &message);

    private:
        void listenThread();
        bool processMessage(const std::string &message, std::string &response);

        std::string m_pipeName;
        bool m_isRunning;
        std::thread m_listenThread;
        std::mutex m_callbackMutex;
        std::map<MessageType, std::vector<MessageCallback>> m_callbacks;
        std::map<std::string, void *> m_clientHandles; // Stores handles to connected clients
    };

    /// IPC Client class for Launcher and SceneryEditorX to use
    class IPCClient
    {
    public:
        IPCClient(const std::string &clientId, const std::string &pipeName = "SceneryEditorXCrashService");
        ~IPCClient();

        bool connect();
        void disconnect();
        bool sendMessage(const IPCMessage &message);
        [[nodiscard]] bool isConnected() const { return m_isConnected; }

    private:
        std::string m_clientId;
        std::string m_pipeName;
        void *m_pipeHandle;
        bool m_isConnected;
    };

    class BackgroundService
    {
    public:
        BackgroundService(const std::string &dumpDir);
        ~BackgroundService();

        void generateDumpForHungProcess();
        void monitorApplication();
        void collectApplicationData(const std::string &applicationId);
        void collectLogs(const std::string &applicationId, const std::string &logDir);
        void collectProjectData(const std::string &projectPath);

        void start();
        void stop();
        void notifyCrash(const std::string &dumpPath);
        void handleErrorLog(const std::string &errorLog, const std::string &applicationId);
        void handleProjectState(const std::string &projectData);
        void sendHeartbeat();
        void startHeartbeatThread();

        /// IPC message handlers
        void onStartupMessage(const IPCMessage &message);
        void onHeartbeatMessage(const IPCMessage &message);
        void onErrorLogMessage(const IPCMessage &message);
        void onCrashMessage(const IPCMessage &message);
        void onShutdownMessage(const IPCMessage &message);
        void onProjectStateMessage(const IPCMessage &message);

        static std::chrono::system_clock::time_point parseTimeString(const std::string &timeStr);
        [[nodiscard]] std::string findMostRecentDump() const;
        [[nodiscard]] bool isRunning() const { return m_isRunning; }

    private:
        std::thread m_heartbeatThread;
        std::string m_lastHeartbeatTime;
        std::string m_recentDump;
        std::filesystem::path m_dumpDir;
        std::filesystem::path m_logDir;
        std::string m_currentProjectPath;
        std::map<std::string, std::chrono::system_clock::time_point> m_appHeartbeats;
        std::mutex m_heartbeatMutex;
        std::unique_ptr<IPCServer> m_ipcServer;
        bool m_isRunning;
        std::vector<std::string> m_collectedLogs;
        std::string m_collectedProjectData;
    };

} // namespace CrashHandler

/// -------------------------------------------------------

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
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>

/// -------------------------------------------------------

namespace CrashHandler
{
    class BackgroundService
	{
    public:
        BackgroundService(const std::string &dumpDir);
        ~BackgroundService();

        void generateDumpForHungProcess();
        void monitorApplication();

        void start();
        void stop();
        void notifyCrash(const std::string &dumpPath);
        void sendHeartbeat();
        void startHeartbeatThread();

        static std::chrono::system_clock::time_point parseTimeString(const std::string &timeStr);
        std::string findMostRecentDump() const;
        [[nodiscard]] bool isRunning() const { return m_isRunning; }

    private:
        std::thread m_heartbeatThread;
        std::string m_lastHeartbeatTime;
        std::string m_recentDump;
        std::filesystem::path m_dumpDir;
        bool m_isRunning;
    };

} // namespace CrashHandler

/// -------------------------------------------------------

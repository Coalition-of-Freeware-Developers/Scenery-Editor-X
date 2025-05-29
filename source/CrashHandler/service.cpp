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
#include <filesystem>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

/// -------------------------------------------------------

namespace CrashHandler
{
    static void setupLogging()
    {
        const auto logFile = std::filesystem::temp_directory_path() / "SceneryEditorX_CrashHandler.log";
        const auto logger = spdlog::basic_logger_mt("crash_handler", logFile.string());
        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::debug);
        spdlog::info("Crash handler service started");
    }

    // Other BackgroundService implementations...

    BackgroundService::BackgroundService(const std::string& dumpDir)
        : m_lastHeartbeatTime("2025-05-13T12:00:00Z")
        , m_dumpDir(dumpDir) // Example timestamp
        , m_isRunning(false)
    {
    }

    BackgroundService::~BackgroundService()
    {
        stop();
    }

    // Helper function to parse time string to time_point
    std::chrono::system_clock::time_point BackgroundService::parseTimeString(const std::string &timeStr)
    {
        // Simple implementation - in production this should properly parse the timestamp
        // For now return a fixed time point for demonstration
        return std::chrono::system_clock::now() - std::chrono::seconds(5);
    }

    void BackgroundService::monitorApplication()
    {
        while (m_isRunning)
        {
            // Check if we've received a recent heartbeat
            auto now = std::chrono::system_clock::now();
            auto lastHeartbeat = parseTimeString(m_lastHeartbeatTime);
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastHeartbeat).count();

            if (elapsed > 10)
            {
                // Application may have crashed without generating a dump
                spdlog::warn("No heartbeat received in {} seconds, application may have crashed", elapsed);

                // Find most recent dump
                m_recentDump = findMostRecentDump();

                if (m_recentDump.empty())
                {
                    // No dump found, the app might be hanging or killed
                    spdlog::info("No crash dump found, generating one for the hung process");
                    generateDumpForHungProcess();
                }

                // Show dialog to user
                CrashDialog dialog;
                dialog.showCrashDialog(m_recentDump);
                
                m_isRunning = false;
                break;
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    // Implementation of findMostRecentDump
    std::string BackgroundService::findMostRecentDump() const
    {
        std::string recentDump;
        std::filesystem::path dumpDir = m_dumpDir.empty() ? 
            std::filesystem::temp_directory_path() : m_dumpDir;

        if (!std::filesystem::exists(dumpDir)) {
            spdlog::error("Dump directory does not exist: {}", dumpDir.string());
            return recentDump;
        }

        for (const auto &entry : std::filesystem::directory_iterator(dumpDir))
        {
            if (entry.path().extension() == ".dmp")
            {
                auto lastWriteTime = std::filesystem::last_write_time(entry);
                if (recentDump.empty() || lastWriteTime > std::filesystem::last_write_time(recentDump))
                {
                    recentDump = entry.path().string();
                }
            }
        }

        return recentDump;
    }

    void BackgroundService::generateDumpForHungProcess()
    {
        // Implementation for generating a dump for hung process
        spdlog::info("Generating dump for hung process");
        // Add implementation here
    }

    void BackgroundService::start()
    {
        if (m_isRunning) {
            spdlog::warn("Background service is already running");
            return;
        }

        m_isRunning = true;
        spdlog::info("Starting background service");
        startHeartbeatThread();
    }

    void BackgroundService::stop()
    {
        if (!m_isRunning) {
            return;
        }

        m_isRunning = false;
        spdlog::info("Stopping background service");
        
        if (m_heartbeatThread.joinable()) {
            m_heartbeatThread.join();
        }
    }

    void BackgroundService::notifyCrash(const std::string &dumpPath)
    {
        spdlog::info("Received crash notification: {}", dumpPath);
        m_recentDump = dumpPath;
        
        // Show crash dialog
        CrashDialog dialog;
        dialog.showCrashDialog(dumpPath);
    }

    void BackgroundService::sendHeartbeat()
    {
        // Implementation for sending heartbeat to main process
        // In a real implementation, this would communicate with the main app
        m_lastHeartbeatTime = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
        spdlog::debug("Heartbeat sent");
    }

    void BackgroundService::startHeartbeatThread()
    {
        m_heartbeatThread = std::thread([this]()
        {
            while (m_isRunning)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(5000)); // Default interval
                sendHeartbeat();
            }
        });
    }

    void handleIpcMessages(BackgroundService& service)
    {
        // Set up IPC channel to receive messages from the main application
        // Process messages: startup, heartbeat, shutdown, crash notifications
        spdlog::info("IPC message handler started");
        
        // Implementation would go here
        // This would handle messages from the main application
    }

} // namespace CrashHandler

/// -------------------------------------------------------

int main(int argc, char *argv[])
{
    CrashHandler::setupLogging();

    // Parse command line arguments
    std::string dumpDir = std::filesystem::temp_directory_path().string();
    // Parse additional arguments here

    // Create background service
    CrashHandler::BackgroundService service(dumpDir);
    service.start();

    // Start IPC server
    std::thread ipcThread(CrashHandler::handleIpcMessages, std::ref(service));

    // Wait for threads
    if (ipcThread.joinable())
    {
        ipcThread.join();
    }

    return 0;
}

/// -------------------------------------------------------

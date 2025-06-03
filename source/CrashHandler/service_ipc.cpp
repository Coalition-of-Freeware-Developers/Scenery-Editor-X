/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* service_ipc.cpp
* -------------------------------------------------------
* Created: 2/6/2025
* -------------------------------------------------------
*/
#include <CrashHandler/service_ipc.h>
#include <spdlog/spdlog.h>
#include <windows.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;

/// -------------------------------------------------------

namespace CrashHandler
{
	// Helper functions
	std::string getCurrentTimestamp()
	{
	    auto now = std::chrono::system_clock::now();
	    auto nowTime = std::chrono::system_clock::to_time_t(now);
	    std::stringstream ss;
	    ss << std::put_time(std::localtime(&nowTime), "%Y-%m-%dT%H:%M:%SZ");
	    return ss.str();
	}
	
	// Serialize IPCMessage to JSON string
	std::string serializeMessage(const IPCMessage &message)
	{
	    json j;
	    j["type"] = static_cast<int>(message.type);
	    j["content"] = message.content;
	    j["timestamp"] = message.timestamp.empty() ? getCurrentTimestamp() : message.timestamp;
	    j["applicationId"] = message.applicationId;
	    return j.dump();
	}
	
	// Deserialize JSON string to IPCMessage
	IPCMessage deserializeMessage(const std::string &jsonStr)
	{
	    try
	    {
	        json j = json::parse(jsonStr);
	        IPCMessage message;
	        message.type = static_cast<MessageType>(j["type"].get<int>());
	        message.content = j["content"].get<std::string>();
	        message.timestamp = j["timestamp"].get<std::string>();
	        message.applicationId = j["applicationId"].get<std::string>();
	        return message;
	    }
	    catch (const std::exception &e)
	    {
	        spdlog::error("Failed to deserialize message: {}", e.what());
	        return IPCMessage{MessageType::ERROR_LOG, "Invalid message format", getCurrentTimestamp(), "Unknown"};
	    }
	}
	
	// IPCServer implementation
	IPCServer::IPCServer(const std::string &pipeName) : m_pipeName(pipeName), m_isRunning(false)
	{
	}
	
	IPCServer::~IPCServer()
	{
	    stop();
	}
	
	void IPCServer::start()
	{
	    if (m_isRunning)
	    {
	        spdlog::warn("IPC Server is already running");
	        return;
	    }
	
	    m_isRunning = true;
	    m_listenThread = std::thread(&IPCServer::listenThread, this);
	    spdlog::info("IPC Server started with pipe name: {}", m_pipeName);
	}
	
	void IPCServer::stop()
	{
	    if (!m_isRunning)
	    {
	        return;
	    }
	
	    m_isRunning = false;
	
	    if (m_listenThread.joinable())
	    {
	        m_listenThread.join();
	    }
	
	    // Close all client handles
	    for (auto &client : m_clientHandles)
	    {
	        if (client.second != INVALID_HANDLE_VALUE)
	        {
	            CloseHandle(client.second);
	        }
	    }
	    m_clientHandles.clear();
	
	    spdlog::info("IPC Server stopped");
	}
	
	void IPCServer::registerCallback(MessageType type, MessageCallback callback)
	{
	    std::lock_guard<std::mutex> lock(m_callbackMutex);
	    m_callbacks[type].push_back(callback);
	}
	
	bool IPCServer::sendMessage(const std::string &client, const IPCMessage &message)
	{
	    auto it = m_clientHandles.find(client);
	    if (it == m_clientHandles.end() || it->second == INVALID_HANDLE_VALUE)
	    {
	        spdlog::error("Client not connected: {}", client);
	        return false;
	    }
	
	    std::string msgStr = serializeMessage(message);
	    DWORD bytesWritten;
	    bool success = WriteFile(it->second, msgStr.c_str(), static_cast<DWORD>(msgStr.size()), &bytesWritten, NULL);
	
	    if (!success || bytesWritten != msgStr.size())
	    {
	        spdlog::error("Failed to send message to client {}: {}", client, GetLastError());
	        return false;
	    }
	
	    return true;
	}
	
	void IPCServer::listenThread()
	{
	    while (m_isRunning)
	    {
	        // Create named pipe instance
	        std::string pipePath = "\\\\.\\pipe\\" + m_pipeName;
	        HANDLE hPipe = CreateNamedPipeA(pipePath.c_str(),
	                                        PIPE_ACCESS_DUPLEX,
	                                        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
	                                        PIPE_UNLIMITED_INSTANCES,
	                                        4096,
	                                        4096,
	                                        0,
	                                        NULL);
	
	        if (hPipe == INVALID_HANDLE_VALUE)
	        {
	            spdlog::error("Failed to create named pipe: {}", GetLastError());
	            std::this_thread::sleep_for(std::chrono::seconds(1));
	            continue;
	        }
	
	        spdlog::debug("Waiting for client connection...");
	        bool connected = ConnectNamedPipe(hPipe, NULL) ? true : (GetLastError() == ERROR_PIPE_CONNECTED);
	
	        if (!connected)
	        {
	            spdlog::error("Client connection failed: {}", GetLastError());
	            CloseHandle(hPipe);
	            continue;
	        }
	
	        // Client connected, read initial message to get client ID
	        char buffer[4096];
	        DWORD bytesRead;
	        bool success = ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, NULL);
	
	        if (!success || bytesRead == 0)
	        {
	            spdlog::error("Failed to read initial message from client: {}", GetLastError());
	            CloseHandle(hPipe);
	            continue;
	        }
	
	        // Process message
	        std::string message(buffer, bytesRead);
	        std::string response;
	        if (processMessage(message, response))
	        {
	            // Extract client ID from the message
	            IPCMessage msg = deserializeMessage(message);
	            std::string clientId = msg.applicationId;
	
	            // Store client handle
	            m_clientHandles[clientId] = hPipe;
	            spdlog::info("Client connected: {}", clientId);
	
	            // Start a new thread to handle this client
	            std::thread([this, hPipe, clientId]() {
	                char clientBuffer[4096];
	                DWORD clientBytesRead;
	                while (m_isRunning)
	                {
	                    bool readSuccess = ReadFile(hPipe, clientBuffer, sizeof(clientBuffer), &clientBytesRead, NULL);
	
	                    if (!readSuccess || clientBytesRead == 0)
	                    {
	                        DWORD error = GetLastError();
	                        if (error == ERROR_BROKEN_PIPE || error == ERROR_PIPE_NOT_CONNECTED)
	                        {
	                            spdlog::info("Client disconnected: {}", clientId);
	                        }
	                        else
	                        {
	                            spdlog::error("Failed to read from client {}: {}", clientId, error);
	                        }
	                        break;
	                    }
	
	                    std::string clientMessage(clientBuffer, clientBytesRead);
	                    std::string clientResponse;
	                    processMessage(clientMessage, clientResponse);
	
	                    // If we have a response, send it back
	                    if (!clientResponse.empty())
	                    {
	                        DWORD bytesWritten;
	                        WriteFile(hPipe,
	                                  clientResponse.c_str(),
	                                  static_cast<DWORD>(clientResponse.size()),
	                                  &bytesWritten,
	                                  NULL);
	                    }
	                }
	
	                // Clean up
	                std::lock_guard<std::mutex> lock(m_callbackMutex);
	                CloseHandle(hPipe);
	                m_clientHandles.erase(clientId);
	            }).detach();
	        }
	        else
	        {
	            // Failed to process initial message, close connection
	            CloseHandle(hPipe);
	        }
	    }
	}
	
	bool IPCServer::processMessage(const std::string &message, std::string &response)
	{
	    try
	    {
	        IPCMessage msg = deserializeMessage(message);
	
	        // Invoke registered callbacks
	        std::lock_guard<std::mutex> lock(m_callbackMutex);
	        auto it = m_callbacks.find(msg.type);
	        if (it != m_callbacks.end())
	        {
	            for (const auto &callback : it->second)
	            {
	                callback(msg);
	            }
	        }
	
	        // Create response (acknowledgment)
	        IPCMessage responseMsg{MessageType::HEARTBEAT, "ACK", getCurrentTimestamp(), "CrashHandlerService"};
	        response = serializeMessage(responseMsg);
	
	        return true;
	    }
	    catch (const std::exception &e)
	    {
	        spdlog::error("Failed to process message: {}", e.what());
	        return false;
	    }
	}
	
	// IPCClient implementation
	IPCClient::IPCClient(const std::string &clientId, const std::string &pipeName)
	    : m_clientId(clientId), m_pipeName(pipeName), m_pipeHandle(INVALID_HANDLE_VALUE), m_isConnected(false)
	{
	}
	
	IPCClient::~IPCClient()
	{
	    disconnect();
	}
	
	bool IPCClient::connect()
	{
	    if (m_isConnected)
	    {
	        return true;
	    }
	
	    std::string pipePath = "\\\\.\\pipe\\" + m_pipeName;
	    m_pipeHandle = CreateFileA(pipePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	
	    if (m_pipeHandle == INVALID_HANDLE_VALUE)
	    {
	        DWORD error = GetLastError();
	        if (error == ERROR_PIPE_BUSY)
	        {
	            // Wait for pipe to become available
	            if (!WaitNamedPipeA(pipePath.c_str(), 5000))
	            {
	                spdlog::error("Pipe busy and wait timed out: {}", GetLastError());
	                return false;
	            }
	            // Try again
	            m_pipeHandle = CreateFileA(pipePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	            if (m_pipeHandle == INVALID_HANDLE_VALUE)
	            {
	                spdlog::error("Failed to connect to pipe after wait: {}", GetLastError());
	                return false;
	            }
	        }
	        else
	        {
	            spdlog::error("Failed to connect to pipe: {}", error);
	            return false;
	        }
	    }
	
	    DWORD mode = PIPE_READMODE_MESSAGE;
	    if (!SetNamedPipeHandleState(m_pipeHandle, &mode, NULL, NULL))
	    {
	        spdlog::error("Failed to set pipe mode: {}", GetLastError());
	        CloseHandle(m_pipeHandle);
	        m_pipeHandle = INVALID_HANDLE_VALUE;
	        return false;
	    }
	
	    // Send initial message with client ID
	    IPCMessage initialMsg{MessageType::STARTUP, "Client connecting", getCurrentTimestamp(), m_clientId};
	
	    if (!sendMessage(initialMsg))
	    {
	        spdlog::error("Failed to send initial message");
	        CloseHandle(m_pipeHandle);
	        m_pipeHandle = INVALID_HANDLE_VALUE;
	        return false;
	    }
	
	    m_isConnected = true;
	    spdlog::info("Connected to pipe: {}", m_pipeName);
	    return true;
	}
	
	void IPCClient::disconnect()
	{
	    if (m_pipeHandle != INVALID_HANDLE_VALUE)
	    {
	        // Send shutdown message if still connected
	        if (m_isConnected)
	        {
	            IPCMessage shutdownMsg{MessageType::SHUTDOWN, "Client disconnecting", getCurrentTimestamp(), m_clientId};
	            sendMessage(shutdownMsg);
	        }
	
	        CloseHandle(m_pipeHandle);
	        m_pipeHandle = INVALID_HANDLE_VALUE;
	        m_isConnected = false;
	        spdlog::info("Disconnected from pipe");
	    }
	}
	
	bool IPCClient::sendMessage(const IPCMessage &message)
	{
	    if (!m_isConnected || m_pipeHandle == INVALID_HANDLE_VALUE)
	    {
	        spdlog::error("Not connected to pipe");
	        return false;
	    }
	
	    std::string msgStr = serializeMessage(message);
	    DWORD bytesWritten;
	    bool success = WriteFile(m_pipeHandle, msgStr.c_str(), static_cast<DWORD>(msgStr.size()), &bytesWritten, NULL);
	
	    if (!success || bytesWritten != msgStr.size())
	    {
	        spdlog::error("Failed to send message: {}", GetLastError());
	        return false;
	    }
	
	    // Read response (optional)
	    char buffer[4096];
	    DWORD bytesRead;
	    success = ReadFile(m_pipeHandle, buffer, sizeof(buffer), &bytesRead, NULL);
	
	    if (success && bytesRead > 0)
	    {
	        std::string response(buffer, bytesRead);
	        spdlog::debug("Received response: {}", response);
	    }
	
	    return true;
	}
} // namespace CrashHandler

/// -------------------------------------------------------

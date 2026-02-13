/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * crash_handler.cpp
 * -------------------------------------------------------
 * Created: 13/5/2025
 * -------------------------------------------------------
 */
#include <CrashHandler/crash_handler.h>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <string>

/// -------------------------------------------------------

namespace CrashHandler
{
    /// Define the CrashHandlerImpl class before usage
    class CrashHandlerImpl
    {
    public:
        explicit CrashHandlerImpl(CrashService::CrashHandlerConfig config)
            : m_config(std::move(config))
        {
            /// Initialize crash handler implementation
            spdlog::debug("Initializing crash handler implementation");
            
            /// Create dump directory if it doesn't exist
            if (!m_config.dumpDir.empty())
            {
                std::filesystem::create_directories(m_config.dumpDir);
            }
            
            /// Initialize platform-specific crash handler
        }
        
        ~CrashHandlerImpl()
        {
            /// Cleanup any resources
            spdlog::debug("Shutting down crash handler implementation");
        }
        
        void addCrashData(const std::string& key, const std::string& value)
        {
            /// Add custom crash data implementation
            m_crashData[key] = value;
        }
        
        bool writeDump(const std::string& reason)
        {
            /// Implement manual dump creation
            spdlog::info("Writing manual dump: {}", reason);
            return true;
        }
        
        void notifyBackgroundService(const std::string& dumpPath)
        {
            /// Notify background service about crash
            spdlog::info("Notifying background service about crash: {}", dumpPath);
        }
        
    private:
        CrashService::CrashHandlerConfig m_config;
        std::map<std::string, std::string> m_crashData;
    };

    std::unique_ptr<CrashHandlerImpl> CrashService::s_impl;

    bool CrashService::Init(const CrashHandlerConfig &config)
    {
        try
        {
            s_impl = std::make_unique<CrashHandlerImpl>(config);
            return true;
        }
        catch (const std::exception &e)
        {
            spdlog::error("Failed to initialize crash handler: {}", e.what());
            return false;
        }
    }

    CrashService::~CrashService()
    {
        Shutdown();
    }

    void CrashService::Shutdown()
    {
        if (s_impl)
        {
            s_impl.reset();
        }
    }

    void CrashService::Tick()
    {
        if (s_impl)
        {
            /// Implement heartbeat functionality
        }
    }

    void CrashService::addCrashData(const std::string &key, const std::string &value)
    {
        if (s_impl)
        {
            s_impl->addCrashData(key, value);
        }
    }

    bool CrashService::WriteDump(const std::string &reason)
    {
        if (s_impl)
        {
            return s_impl->writeDump(reason);
        }
        return false;
    }

} // namespace CrashHandler

/// -------------------------------------------------------

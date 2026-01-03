/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* application_context.h
* -------------------------------------------------------
* Created: 25/12/2025
* -------------------------------------------------------
*/
#pragma once
#include <memory>
#include <string>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX
{
/**
     * @class PlatformContext
     * @brief Application context contains an abstract platform-specific.
     *
     */
    class PlatformContext
    {
    public:
		virtual ~PlatformContext() = default;
        // TODO: Decide if there really is any need for singleton access
        //static PlatformContext &Get() { return *appCtx; } // Singleton access

		virtual const std::string &GetWorkingDirectory() const { return m_WorkingDirectory; }
		virtual const std::string &GetTempDirectory() const { return m_TempDirectory; }
        virtual const std::vector<std::string> &GetCommandLineArgs() const { return m_CommandLineArgs; }

        //static Ref<PlatformContext> Create(const std::string &name, const std::string &workingDirectory, const std::vector<std::string> &commandLineArgs);

    protected:
        std::string m_WorkingDirectory;
        std::string m_TempDirectory;
        std::vector<std::string> m_CommandLineArgs;
        //static PlatformContext *appCtx;

        PlatformContext() = default;
        
    };

}

// -------------------------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* platform.cpp
* -------------------------------------------------------
* Created: 27/12/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/core/platform/platform.h>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/logging/logging.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{
    Platform::Platform(const PlatformContext &context)
    {
        SEDX_CORE_INFO_TAG("Platform", "Platform initialized");
        SEDX_CORE_INFO("  Working Directory: {}", context.GetWorkingDirectory());
        SEDX_CORE_INFO("  Temp Directory: {}", context.GetTempDirectory());
        SEDX_CORE_INFO("  Command Line Args: {}", context.GetCommandLineArgs().size());
    }

    ExitCode Platform::MainLoop()
    {
        SEDX_CORE_INFO_TAG("Platform", "Starting main loop");

        ExitCode exitCode = ExitCode::Success;

        while (exitCode == ExitCode::Success)
        {
            exitCode = MainLoopFrame();
        }

        return exitCode;
    }

    ExitCode Platform::MainLoopFrame()
    {
        // Check if application wants to close
        if (!Application::Get().GetWindow().GetShouldClose())
        {
            Update();
            return ExitCode::Success;
        }

        return ExitCode::Close;
    }

    void Platform::Update()
    {
        // Process events and update application state
        Application::Get().ProcessEvents();
    }

    void Platform::Terminate(ExitCode code)
    {
        SEDX_CORE_INFO_TAG("Platform", "Platform terminating with code: {}", static_cast<int>(code));
    }

    void Platform::Close()
    {
        SEDX_CORE_INFO_TAG("Platform", "Platform shutdown requested");
        Application::Get().Stop();
    }

}

// -------------------------------------------------------

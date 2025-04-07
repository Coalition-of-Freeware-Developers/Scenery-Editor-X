/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* scenery_editor_x.cpp
* -------------------------------------------------------
* Created: 5/4/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/core/cmd_line.h>
#include <SceneryEditorX/core/entry_point.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/utils/file_system.h>

// -------------------------------------------------------

SceneryEditorX::EditorApplication *CreateApplication(const int argc, const char *argv[])
{
    SceneryEditorX::CommandLineParser cli(argc, argv);

    auto raw = cli.GetRawArgs();
    if (raw.size() > 1)
    {
        SEDX_CORE_WARN("More than one project path specified, using `{}'", raw[0]);
    }

    // ---------------------------------------------------------

    auto cd = cli.GetOpt("C");
    if (!cd.empty())
    {
        SceneryEditorX::FileSystem::SetWorkingDirectory(cd);
    }

    // ---------------------------------------------------------

    std::string_view projectPath;
    if (!raw.empty())
        projectPath = raw[0];
    if (argc > 1)
        projectPath = argv[1];

    // ---------------------------------------------------------

    SceneryEditorX::EditorSpecification specification;
    specification.name = "Scenery Editor X";
    specification.width = 1280;
    specification.height = 720;
    specification.projectPath = projectPath;
    specification.StartMaximized = true;
    specification.VSync = true;

    SceneryEditorX::EditorConfig config;
    specification.IconPath = config.projectPath + "/icon.png";
    specification.binPath = config.projectPath + "/bin";

    return new SceneryEditorX::EditorApplication(specification, projectPath);
}

// -------------------------------------------------------

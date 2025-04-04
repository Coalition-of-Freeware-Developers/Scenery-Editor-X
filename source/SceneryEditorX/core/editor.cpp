/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* editor.cpp
* -------------------------------------------------------
* Created: 16/3/2025
* -------------------------------------------------------
*/

#include <Launcher/core/launcher_main.h>
#include <SceneryEditorX/core/editor.h>
#include <SceneryEditorX/core/cmd_line.h>

// -------------------------------------------------------

SceneryEditorX::EditorApplication *CreateApplication(const int argc, const char *argv[])
{
    SceneryEditorX::CommandLineParser cli(argc, argv);

    auto raw = cli.GetRawArgs();
    if (raw.size() > 1)
    {
        //EDITOR_LOG_WARN("More than one project path specified, using `{}'", raw[0]);
    }

	auto cd = cli.GetOpt("C");
    if (!cd.empty())
    {
        //SceneryEditorX::FileSystem::SetWorkingDirectory(cd);
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

	return new SceneryEditorX::EditorApplication(specification, projectPath);
}

// -------------------------------------------------------

SceneryEditorX::EditorApplication::EditorApplication(const EditorSpecification &specification, std::string_view projectPath)
    : vkRenderer(), ui()
{
    // Initialize the application with the given specification and project path
    // You can add more initialization code here if needed
}
// -------------------------------------------------------

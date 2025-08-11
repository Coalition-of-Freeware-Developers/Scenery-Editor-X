/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* editor_settings.cpp
* -------------------------------------------------------
* Created: 10/8/2025
* -------------------------------------------------------
*/
#include <filesystem>
#include <Editor/settings/editor_settings.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    static std::filesystem::path s_EditorSettingsPath;

    EditorSettings &EditorSettings::Get()
    {
        static EditorSettings s_Settings;
        return s_Settings;
    }

}

/// -------------------------------------------------------

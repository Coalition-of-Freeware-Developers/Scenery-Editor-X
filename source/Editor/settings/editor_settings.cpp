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
 * editor_settings.cpp
 * -------------------------------------------------------
 * Created: 10/8/2025
 * -------------------------------------------------------
 */
#include "Editor/settings/editor_settings.h"
#include <algorithm>
#include <filesystem>
#include <Editor/core/transform_tool.h>
#include <SceneryEditorX/filesystem/vfs.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		constexpr const char *LOG_TAG = "EditorSettings";

		Ref<EditorSettingsManager> &Manager()
		{
			static Ref<EditorSettingsManager> manager;
			return manager;
		}

		/* @brief The transform tool's snapping and gizmo options from the settings. */
		void ApplyToTools(const EditorSettings &settings)
		{
			TransformTool::Settings &tool = TransformTool::GetSettings();
			tool.snap = settings.enableGridSnapping;
			tool.snapMove = settings.translationSnapValue;
			tool.snapAngle = settings.rotationSnapValue;
			tool.snapScale = settings.scaleSnapValue;
			tool.localSpace = settings.gizmoLocalSpace;
			tool.pivotActive = settings.gizmoPivotActive;
		}

		/* @brief The transform tool's current options into the settings (the viewport toolbar changes them). */
		void ReadFromTools(EditorSettings &settings)
		{
			const TransformTool::Settings &tool = TransformTool::GetSettings();
			settings.enableGridSnapping = tool.snap;
			settings.translationSnapValue = tool.snapMove;
			settings.rotationSnapValue = tool.snapAngle;
			settings.scaleSnapValue = tool.snapScale;
			settings.gizmoLocalSpace = tool.localSpace;
			settings.gizmoPivotActive = tool.pivotActive;
		}

		float ReadFloat(const ConfigFile &file, const char *path, const float defaultValue, const float low, const float high)
		{
			return std::clamp(static_cast<float>(file.GetFloatOption(path, defaultValue)), low, high);
		}
	}

	// -------------------------------------------------------

	EditorSettings &EditorSettings::Get()
	{
		static EditorSettings s_Settings;
		return s_Settings;
	}

	// -------------------------------------------------------

	void EditorSettingsSerializer::Init()
	{
		Manager() = CreateRef<EditorSettingsManager>();
		EditorSettings::Get() = Manager()->GetSettings();
		ApplyToTools(EditorSettings::Get());
		EDITOR_TRACE_TAG(LOG_TAG, "Editor settings from {}", VFS::ToUtf8(Manager()->GetConfigPath()));
	}

	void EditorSettingsSerializer::LoadSettings()
	{
		if (!Manager())
		{
			Init();
			return;
		}
		Manager()->ReadSettings();
		EditorSettings::Get() = Manager()->GetSettings();
		ApplyToTools(EditorSettings::Get());
	}

	void EditorSettingsSerializer::SaveSettings()
	{
		if (!Manager())
			return;
		ReadFromTools(EditorSettings::Get());
		Manager()->SetSettings(EditorSettings::Get());
		Manager()->WriteSettings();
	}

	void EditorSettingsSerializer::Shutdown()
	{
		SaveSettings();
		Manager() = nullptr;
	}

	// -------------------------------------------------------

	EditorSettingsManager::EditorSettingsManager() : m_File(DEFAULT_LOCATION)
	{
		ReadSettings();
	}

	EditorSettingsManager::EditorSettingsManager(const std::filesystem::path &configPath) : m_File(VFS::ToUtf8(configPath))
	{
		ReadSettings();
	}

	EditorSettingsManager::~EditorSettingsManager()
	{
		if (m_Writable)
			WriteSettings();
	}

	void EditorSettingsManager::SetSettings(const EditorSettings &settings)
	{
		m_Settings = settings;
	}

	bool EditorSettingsManager::ReadSettings()
	{
		std::string error;
		m_Writable = true;
		const ConfigFile::LoadResult result = m_File.Load(&error);
		if (result == ConfigFile::LoadResult::Invalid)
		{
			// Keep what the user had before the defaults replace it
			std::string backupError;
			if (m_File.Backup(".invalid", &backupError))
			{
				EDITOR_WARN_TAG(LOG_TAG, "{} - kept as {}.invalid, using the defaults", error, m_File.GetLocation());
			}
			else
			{
				EDITOR_WARN_TAG(LOG_TAG, "{} - using the defaults, the file is left as it is ({})", error, backupError);
				m_Writable = false;
			}
		}
		else if (result == ConfigFile::LoadResult::Failed)
		{
			EDITOR_ERROR_TAG(LOG_TAG, "Could not read the editor settings: {}", error);
			m_Writable = false;
		}

		// Every field from the file, the defaults for what's missing; numbers clamped to what the editor's controls allow
		const EditorSettings defaults;
		EditorSettings &s = m_Settings;
		s.enableGridSnapping = m_File.GetBoolOption("editor.snapping.enabled", defaults.enableGridSnapping);
		s.translationSnapValue = ReadFloat(m_File, "editor.snapping.translation", defaults.translationSnapValue, 0.01f, 1000.0f);
		s.rotationSnapValue = ReadFloat(m_File, "editor.snapping.rotation", defaults.rotationSnapValue, 0.5f, 180.0f);
		s.scaleSnapValue = ReadFloat(m_File, "editor.snapping.scale", defaults.scaleSnapValue, 0.01f, 10.0f);
		s.gizmoLocalSpace = m_File.GetBoolOption("editor.gizmo.local_space", defaults.gizmoLocalSpace);
		s.gizmoPivotActive = m_File.GetBoolOption("editor.gizmo.pivot_active", defaults.gizmoPivotActive);
		s.showColliders = m_File.GetBoolOption("editor.viewport.show_colliders", defaults.showColliders);
		s.showLights = m_File.GetBoolOption("editor.viewport.show_lights", defaults.showLights);
		s.showGrid = m_File.GetBoolOption("editor.viewport.show_grid", defaults.showGrid);
		s.enableVSync = m_File.GetBoolOption("editor.display.vsync", defaults.enableVSync);
		s.maxFps = std::clamp(m_File.GetIntOption("editor.display.max_fps", defaults.maxFps), 10, 1000);
		return result == ConfigFile::LoadResult::Loaded;
	}

	bool EditorSettingsManager::WriteSettings()
	{
		if (!m_Writable)
		{
			EDITOR_WARN_TAG(LOG_TAG, "{} not saved: it couldn't be read, and writing would lose what's in it", m_File.GetLocation());
			return false;
		}

		const EditorSettings &s = m_Settings;
		m_File.AddBoolOption("editor.snapping.enabled", s.enableGridSnapping);
		m_File.AddFloatOption("editor.snapping.translation", s.translationSnapValue);
		m_File.AddFloatOption("editor.snapping.rotation", s.rotationSnapValue);
		m_File.AddFloatOption("editor.snapping.scale", s.scaleSnapValue);
		m_File.AddBoolOption("editor.gizmo.local_space", s.gizmoLocalSpace);
		m_File.AddBoolOption("editor.gizmo.pivot_active", s.gizmoPivotActive);
		m_File.AddBoolOption("editor.viewport.show_colliders", s.showColliders);
		m_File.AddBoolOption("editor.viewport.show_lights", s.showLights);
		m_File.AddBoolOption("editor.viewport.show_grid", s.showGrid);
		m_File.AddBoolOption("editor.display.vsync", s.enableVSync);
		m_File.AddIntOption("editor.display.max_fps", s.maxFps);

		// Nothing changed and the file is there: nothing to write
		if (!m_File.IsDirty() && m_File.FileExists())
			return true;

		std::string error;
		if (!m_File.Save(&error))
		{
			EDITOR_ERROR_TAG(LOG_TAG, "Could not save the editor settings: {}", error);
			return false;
		}
		EDITOR_TRACE_TAG(LOG_TAG, "Editor settings saved to {}", VFS::ToUtf8(m_File.GetPhysicalPath()));
		return true;
	}

} // namespace SceneryEditorX

// -------------------------------------------------------

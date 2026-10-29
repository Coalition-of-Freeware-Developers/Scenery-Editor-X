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
 * settings.cpp
 * -------------------------------------------------------
 * Created: 23/4/2025
 * -------------------------------------------------------
 */
#include "settings.h"
#include "steam_parser.h"
#include <filesystem>
#include <SceneryEditorX/core/application/application_data.h>
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/xplane/xp_install_finder.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		constexpr const char *LOG_TAG = "Settings";

		// What versions up to 0.2.30 wrote as the project folder on every platform: a Windows path nothing expanded
		constexpr const char *LEGACY_PROJECT_DIR = "~\\Documents\\SceneryEditorX";

		/* @brief <Documents>/SceneryEditorX (XDG_DOCUMENTS_DIR on Linux), else a folder in the working folder. */
		std::filesystem::path GetPlatformProjectFolder()
		{
			const std::filesystem::path documents = VFS::GetFolder(VFS::Folder::Documents);
			return documents.empty() ? std::filesystem::path("SceneryEditorX") : documents / "SceneryEditorX";
		}

		bool ContainsNoCase(std::string text, const std::string_view word)
		{
			for (char &c : text)
				c = c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
			return text.find(word) != std::string::npos;
		}
	}

	// ----------------------------------------------------------

	Settings::Settings() = default;

	Settings::Settings(std::string location)
	{
		Open(std::move(location));
	}

	bool Settings::Open(std::string location)
	{
		SetLocation(std::move(location));
		const bool loaded = ReadSettings();

		// A new file, or one from an older version: everything the settings have gets a value
		ApplyDefaults();
		LoadDataFromConfig();
		bool changed = !loaded || IsDirty();

		// This runs while the application is being constructed: a failure here may cost the X-Plane path, never the editor
		try
		{
			if (!ValidateXPlanePaths())
			{
				if (!m_XPlaneStats.xPlanePath.empty())
					SEDX_CORE_WARN_TAG(LOG_TAG, "{} is no longer an X-Plane folder, searching for X-Plane", m_XPlaneStats.xPlanePath);
				if (DetectXPlanePath())
				{
					SEDX_CORE_TRACE_TAG(LOG_TAG, "X-Plane detected and paths configured");
					changed = true;
				}
			}
		}
		catch (const std::exception &e)
		{
			SEDX_CORE_ERROR_TAG(LOG_TAG, "Could not detect X-Plane: {}", e.what());
		}

		if (m_LoadResult == LoadResult::Failed)
			return false;	// The defaults are in use; the file stays as it is
		return changed ? WriteSettings() : true;
	}

	bool Settings::ReadSettings()
	{
		std::string error;
		m_LoadResult = Load(&error);
		switch (m_LoadResult)
		{
			case LoadResult::Loaded:
				SEDX_CORE_TRACE_TAG(LOG_TAG, "Reading settings from: {}", VFS::ToUtf8(GetPhysicalPath()));
				UpgradeOldValues();
				LoadDataFromConfig();
				return true;

			case LoadResult::Missing:
				SEDX_CORE_TRACE_TAG(LOG_TAG, "Config file not found: {}", GetLocation());
				return false;

			case LoadResult::Invalid:
			{
				// Keep what the user had: the defaults replace the file only once a copy of it is safe
				std::string backupError;
				if (Backup(".invalid", &backupError))
				{
					SEDX_CORE_WARN_TAG(LOG_TAG, "{} - kept as {}.invalid, using the defaults", error, GetLocation());
				}
				else
				{
					SEDX_CORE_WARN_TAG(LOG_TAG, "{} - using the defaults, the file is left as it is ({})", error, backupError);
					m_LoadResult = LoadResult::Failed;
				}
				return false;
			}

			case LoadResult::Failed:
			default:
				SEDX_CORE_ERROR_TAG(LOG_TAG, "Error trying to read application settings: {}", error);
				return false;
		}
	}

	bool Settings::WriteSettings()
	{
		if (m_LoadResult == LoadResult::Failed)
		{
			SEDX_CORE_WARN_TAG(LOG_TAG, "{} not written: it couldn't be read, and writing would lose what's in it", GetLocation());
			return false;
		}

		// Ensure all required sections exist and the document carries our data before writing
		ApplyDefaults();
		UpdateConfigFromData();

		std::string error;
		if (!Save(&error))
		{
			SEDX_CORE_ERROR_TAG(LOG_TAG, "Error writing settings to file: {}", error);
			return false;
		}
		SEDX_CORE_TRACE_TAG(LOG_TAG, "Settings successfully written to: {}", VFS::ToUtf8(GetPhysicalPath()));
		return true;
	}

	bool Settings::DetectXPlanePath()
	{
		// The search the X-Plane library uses: X-Plane's install lists, Steam, the registry and the usual folders on fixed drives
		for (const XPlane::InstallCandidate &candidate : XPlane::FindInstallCandidates())
		{
			if (!candidate.complete)
				continue;
			SEDX_CORE_INFO_TAG(LOG_TAG, "Found X-Plane {} at {} ({})", candidate.majorVersion, VFS::ToUtf8(candidate.root), XPlane::GetInstallSourceName(candidate.source));
			if (SetXPlanePath(VFS::ToUtf8(candidate.root)))
			{
				if (candidate.majorVersion > 0)
					m_XPlaneStats.xPlaneVersion = "X-Plane " + std::to_string(candidate.majorVersion);
				UpdateConfigFromData();
				return true;
			}
		}

		SEDX_CORE_WARN_TAG(LOG_TAG, "Could not automatically detect an X-Plane 11 or 12 installation");
		SEDX_CORE_INFO_TAG(LOG_TAG, "Set the X-Plane folder in the X-Plane Library panel");

		// Initialize with empty path but don't report error
		m_XPlaneStats.xPlanePath = "";
		m_XPlaneStats.xPlaneBinPath = "";
		m_XPlaneStats.xPlaneResourcesPath = "";
		m_XPlaneStats.isSteam = false;
		m_XPlaneStats.xPlaneVersion = "X-Plane 12";

		// Return false to indicate the path wasn't found automatically
		return false;
	}

	bool Settings::SetXPlanePath(const std::string &path)
	{
		if (path.empty())
		{
			SEDX_CORE_ERROR_TAG(LOG_TAG, "Cannot set X-Plane path: Empty path provided");
			return false;
		}

		if (!SteamGameFinder::ValidateXPlanePath(path))
		{
			SEDX_CORE_ERROR_TAG(LOG_TAG, "Not an X-Plane folder: {}", path);
			return false;
		}

		// The main path, the paths derived from it, and the document (WriteSettings saves it)
		m_XPlaneStats.xPlanePath = path;
		UpdateDerivedXPlanePaths();
		UpdateConfigFromData();

		SEDX_CORE_TRACE_TAG(LOG_TAG, "X-Plane path set to: {}", path);
		return true;
	}

	std::string Settings::GetXPlanePath() const
	{
		return m_XPlaneStats.xPlanePath;
	}

	bool Settings::ValidateXPlanePaths() const
	{
		if (m_XPlaneStats.xPlanePath.empty())
			return false;

		return SteamGameFinder::ValidateXPlanePath(m_XPlaneStats.xPlanePath);
	}

	void Settings::UpdateDerivedXPlanePaths()
	{
		if (m_XPlaneStats.xPlanePath.empty())
		{
			SEDX_CORE_WARN_TAG(LOG_TAG, "Cannot update derived paths: X-Plane path is empty");
			return;
		}

		// As the disk spells them: a case-sensitive file system (Linux) has "Resources", not "resources"
		const std::filesystem::path basePath = VFS::FromUtf8(m_XPlaneStats.xPlanePath);
		m_XPlaneStats.xPlaneBinPath = VFS::ToUtf8(VFS::ResolveCase(basePath / "bin"));
		m_XPlaneStats.xPlaneResourcesPath = VFS::ToUtf8(VFS::ResolveCase(basePath / "Resources"));

		// Check if this is a Steam installation by examining the path.
		// The path typically contains "steamapps/common" for Steam installations
		m_XPlaneStats.isSteam = ContainsNoCase(m_XPlaneStats.xPlanePath, "steamapps") ||
			(ContainsNoCase(m_XPlaneStats.xPlanePath, "steam") && ContainsNoCase(m_XPlaneStats.xPlanePath, "common"));

		SEDX_CORE_TRACE_TAG(LOG_TAG, "Updated derived paths - Bin: {}, Resources: {}, Steam: {}",
							m_XPlaneStats.xPlaneBinPath,
							m_XPlaneStats.xPlaneResourcesPath,
							m_XPlaneStats.isSteam ? "true" : "false");
	}

	std::filesystem::path Settings::GetDefaultProjectFolder() const
	{
		const std::string folder = GetStringOption("project.default_project_dir");
		if (folder.empty())
			return GetPlatformProjectFolder();

		// "~", "~/..." or "~\..." is the home folder, on every platform
		if (folder[0] == '~' && (folder.size() == 1 || folder[1] == '/' || folder[1] == '\\'))
		{
			const std::filesystem::path home = VFS::GetFolder(VFS::Folder::Home);
			if (!home.empty())
				return folder.size() <= 2 ? home : home / VFS::FromUtf8(VFS::NormalizeRelative(folder.substr(2)));
		}
		return VFS::FromUtf8(folder);
	}

	void Settings::ApplyDefaults()
	{
		const auto defaultBool = [this](const char *path, const bool value) { if (!HasOption(path)) AddBoolOption(path, value); };
		const auto defaultInt = [this](const char *path, const int value) { if (!HasOption(path)) AddIntOption(path, value); };
		const auto defaultString = [this](const char *path, const std::string &value) { if (!HasOption(path)) AddStringOption(path, value); };

		// Application settings
		defaultString("application.version", AppData::versionString);
		defaultBool("application.no_titlebar", false);

		// X-Plane settings
		defaultString("x_plane.version", "X-Plane 12");
		defaultString("x_plane.path", "");
		defaultString("x_plane.bin_path", "");
		defaultString("x_plane.resources_path", "");
		defaultBool("x_plane.is_steam", false);

		// User interface settings
		defaultString("ui.theme", "dark");
		defaultInt("ui.font_size", 12);
		defaultString("ui.language", "english");

		// Project settings
		defaultBool("project.auto_save", true);
		defaultInt("project.auto_save_interval", 5);	// minutes
		defaultInt("project.backup_count", 3);
		defaultString("project.default_project_dir", VFS::ToUtf8(GetPlatformProjectFolder()));
	}

	void Settings::UpgradeOldValues()
	{
		if (GetStringOption("project.default_project_dir") == LEGACY_PROJECT_DIR)
		{
			const std::string folder = VFS::ToUtf8(GetPlatformProjectFolder());
			AddStringOption("project.default_project_dir", folder);
			SEDX_CORE_INFO_TAG(LOG_TAG, "Default project folder {} -> {}", LEGACY_PROJECT_DIR, folder);
		}
	}

	void Settings::LoadDataFromConfig()
	{
		// X-Plane stats
		m_XPlaneStats.xPlaneVersion = GetStringOption("x_plane.version", m_XPlaneStats.xPlaneVersion);
		m_XPlaneStats.xPlanePath = GetStringOption("x_plane.path", m_XPlaneStats.xPlanePath);
		m_XPlaneStats.xPlaneBinPath = GetStringOption("x_plane.bin_path", m_XPlaneStats.xPlaneBinPath);
		m_XPlaneStats.xPlaneResourcesPath = GetStringOption("x_plane.resources_path", m_XPlaneStats.xPlaneResourcesPath);
		m_XPlaneStats.isSteam = GetBoolOption("x_plane.is_steam", m_XPlaneStats.isSteam);

		// Application stats
		m_AppStats.noTitlebar = GetBoolOption("application.no_titlebar", m_AppStats.noTitlebar);
	}

	void Settings::UpdateConfigFromData()
	{
		// X-Plane data
		AddStringOption("x_plane.version", m_XPlaneStats.xPlaneVersion);
		AddStringOption("x_plane.path", m_XPlaneStats.xPlanePath);
		AddStringOption("x_plane.bin_path", m_XPlaneStats.xPlaneBinPath);
		AddStringOption("x_plane.resources_path", m_XPlaneStats.xPlaneResourcesPath);
		AddBoolOption("x_plane.is_steam", m_XPlaneStats.isSteam);

		// Application data
		AddBoolOption("application.no_titlebar", m_AppStats.noTitlebar);
		AddStringOption("application.version", AppData::versionString);
	}

}

// -------------------------------------------------------

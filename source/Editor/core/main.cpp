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
 * main.cpp
 * -------------------------------------------------------
 * Created: 16/3/2025
 * -------------------------------------------------------
 */
#include "editor_layer.h"
#include <SceneryEditorX/core/platform/entrypoint.h>
#include <SceneryEditorX/filesystem/vfs.h>

// -------------------------------------------------------

static std::filesystem::path s_ProjectSolutionPath = "";
static std::vector<std::string> s_ClArguments;
static std::string s_TempDirectory;
static uint32_t s_ClArg_Flags = 0;
static bool s_CacheClearedByCli = false;
static auto operator<(const ImVec2 &lhs, const ImVec2 &rhs) { return lhs.x < rhs.x && lhs.y < rhs.y; }

/**
 * @brief Clears temporary cache files under the platform temp directory.
 */
static void ClearTempCache()
{
	if (s_CacheClearedByCli)
		return;

	if (s_TempDirectory.empty())
	{
		EDITOR_WARN_TAG("Editor", "Cannot clear cache: temp directory is empty");
		return;
	}

	// The platform context's folders are UTF-8: std::filesystem::path(std::string) would read them as ANSI on Windows
	const std::filesystem::path tempRoot = SceneryEditorX::VFS::FromUtf8(s_TempDirectory) / "SceneryEditorX";
	const std::string tempRootText = SceneryEditorX::VFS::ToUtf8(tempRoot);
	std::error_code ec;

	if (!std::filesystem::exists(tempRoot, ec))
	{
		EDITOR_INFO_TAG("Editor", "--clear-cache requested, but no temp cache folder found at '{}'", tempRootText);
		s_CacheClearedByCli = true;
		return;
	}

	const uintmax_t removed = std::filesystem::remove_all(tempRoot, ec);
	if (ec)
	{
		EDITOR_ERROR_TAG("Editor", "Failed to clear temp cache '{}' ({}): {}", tempRootText, ec.value(), ec.message());
		return;
	}

	EDITOR_INFO_TAG("Editor", "Cleared temp cache folder '{}' (removed {} entries)", tempRootText, removed);
	s_CacheClearedByCli = true;
}

/**
 * @brief Check if a specific command line argument is present.
 * @param argument The command line argument to check for.
 * @return True if the argument is present, false otherwise.
 */
static bool HasArgument(const std::string &argument)
{
	for (const auto &arg : s_ClArguments)
	{
		if (arg == argument)
			return true;
	}

	return false;
}

/**
 * @brief Process command line arguments and set internal flags accordingly.
 */
static void ProcessClArgs()
{
	EDITOR_TRACE_TAG("Editor", "Processing command line arguments");

	for (const auto &arg : s_ClArguments)
	{
		if (arg == "--clear-cache")
		{
			EDITOR_INFO_TAG("Editor", "Clearing temporary cache due to CLI argument: {}", arg);
			ClearTempCache();
		}

		if (arg == "--recompile-shaders")
		{
			EDITOR_INFO_TAG("Editor", "Shader cache bypass requested via CLI argument: {}", arg);
		}
	}
}

/**
 * @class Editor
 * @brief Main application class for the Scenery Editor X.
 *
 * The Editor class serves as the central controller for the editor,
 * managing the lifecycle of the application and coordinating between the graphics engine,
 * UI system, asset management, and viewport rendering. It handles initialization,
 * main loop execution, frame rendering, and resource management.
 */
class Editor : public SceneryEditorX::Application
{
public:
	explicit Editor(const SceneryEditorX::PlatformContext& context) : Application(context) {}

	Editor(const SceneryEditorX::PlatformContext &context, const SceneryEditorX::Ref<SceneryEditorX::UserPreferences> &userPreferences) 
	: Application(context), m_UserPreferences(userPreferences)
	{
		s_ClArguments = context.GetCommandLineArgs();
		s_TempDirectory = context.GetTempDirectory();
		ProcessClArgs(); // Process command line arguments to set internal flags before initialization
		EDITOR_INFO_TAG("Editor", "=== Initializing Editor with PlatformContext and UserPreferences ===");
		EDITOR_INFO_TAG("Editor", "Working Directory: {}", context.GetWorkingDirectory());
		EDITOR_INFO_TAG("Editor", "Temp Directory: {}", context.GetTempDirectory());
	}

	virtual void OnInit() override
	{
		const auto start = std::chrono::high_resolution_clock::now();
		std::filesystem::path appdata = SceneryEditorX::IO::FileSystem::GetPersistentStoragePath();

		// Persistent Storage
		{
			m_PersistentStoragePath = appdata;

			if (!SceneryEditorX::IO::FileSystem::Exists(m_PersistentStoragePath))
				SceneryEditorX::IO::FileSystem::CreateDir(m_PersistentStoragePath);
		}

	    if (!m_UserPreferences)
		{
			EDITOR_INFO_TAG("Main", "User Preferences not initialized. Creating default preferences and continuing initialization.");
			m_UserPreferences = SceneryEditorX::CreateRef<SceneryEditorX::UserPreferences>();
		}

		m_UserPreferences->LoadPreferences();

		if (m_UserPreferences->GetRecentProjects().empty() && !m_ProjectPath.empty())
		{
		    m_UserPreferences->SetStartupProject(m_ProjectPath);
			m_ProjectPath = m_UserPreferences->GetStartupProject();
			m_UserPreferences->LoadPreferences();
		}

		if (!SceneryEditorX::Window::IsVisible())
		{
			EDITOR_WARN_TAG("Editor", "Main window is not visible after creation.");
		}
		
		PushLayer(new SceneryEditorX::EditorLayer(m_UserPreferences));

		const auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
		EDITOR_INFO_TAG("Editor", "Editor initialization complete ({} ms)", duration);
	}

private:
	std::string m_ProjectPath;
	std::filesystem::path m_PersistentStoragePath;
	SceneryEditorX::Ref<SceneryEditorX::UserPreferences> m_UserPreferences;
};

// -------------------------------------------------------

SceneryEditorX::Application* SceneryEditorX::CreateApplication(const PlatformContext& context)
{
	s_ClArguments = context.GetCommandLineArgs();
	s_TempDirectory = context.GetTempDirectory();
	ProcessClArgs(); // Process command line arguments to set internal flags before initialization

	// Return a new instance of the editor application using PlatformContext
	return new Editor(context);
}

// -------------------------------------------------------

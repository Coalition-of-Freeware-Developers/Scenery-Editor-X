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
 * project.cpp
 * -------------------------------------------------------
 * Created: 7/5/2025
 * -------------------------------------------------------
 */
#include "project.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	Ref<Project> Project::m_ActiveProject = nullptr;
	Ref<AssetManager> Project::m_AssetManager = nullptr;

	Project::Project()
	{
		// TODO: Initialize the project
	}
	Project::~Project()
	{
		// TODO: Cleanup the project
	}

	void Project::SetActive(Ref<Project> &project)
	{
		if (m_ActiveProject)
		{
			//s_AssetManager->Shutdown();
			//s_AssetManager = nullptr;
			// TODO: Add code to handle the previous active project if needed
		}

		m_ActiveProject = project;
		if (m_ActiveProject)
		{
			//s_AssetManager = CreateRef<EditorAssetManager>();
		}
	}

	/*
	std::filesystem::path Project::GetAssetRegistryPath()
	{
		if (activeProject)
		{
			return activeProject->GetAssetDirectory() / "AssetRegistry.json";
		}
		return {};
	}


	/*
	std::filesystem::path Project::GetActiveAssetDirectory()
	{
		if (activeProject)
			return activeProject->GetAssetDirectory();

		return {};
	}
	*/

	/*
	std::filesystem::path Project::GetAssetDirectory()
	{
		// TODO: Implement proper asset directory retrieval from config
		return config.projectPath / "assets";
	}
	*/

	void Project::CreateProject(std::string name, std::filesystem::path path)
	{
		// TODO: Implement project creation
	}

	void Project::Load(const std::filesystem::path &inPath)
	{
		// TODO: Implement project loading
	}

	void Project::Save(const std::filesystem::path &inPath)
	{
		// TODO: Implement project saving
	}

	const std::string &Project::GetProjectName()
	{
		SEDX_CORE_ASSERT(m_ActiveProject, "No active project set");
		return m_ActiveProject->GetConfig().name;
	}

	std::filesystem::path Project::GetProjectDirectory()
	{
		SEDX_CORE_ASSERT(m_ActiveProject, "No active project set");
		return m_ActiveProject->GetConfig().projectPath;
	}

	std::filesystem::path Project::GetAssetRegistryPath()
	{
		SEDX_CORE_ASSERT(m_ActiveProject, "No active project set");
		return std::filesystem::path(m_ActiveProject->GetConfig().projectPath) /
			   m_ActiveProject->GetConfig().assetRegistry;
	}

	std::filesystem::path Project::GetActiveAssetDirectory()
	{
		SEDX_CORE_ASSERT(m_ActiveProject, "No active project set");
		return m_ActiveProject->GetAssetDirectory();
	}

	std::filesystem::path Project::GetAssetDirectory() const
	{
		return std::filesystem::path(GetConfig().projectPath) / GetConfig().assetDir;
	}

	std::filesystem::path Project::GetMeshPath()
	{
		SEDX_CORE_ASSERT(m_ActiveProject);
		return std::filesystem::path(m_ActiveProject->GetConfig().projectPath) / m_ActiveProject->GetConfig().meshPath;
	}

	std::filesystem::path Project::GetCacheDirectory()
	{
		SEDX_CORE_ASSERT(m_ActiveProject, "No active project set");
		return std::filesystem::path(m_ActiveProject->GetConfig().projectPath) / "../cache";
	}

	std::filesystem::path Project::GetScriptModulePath()
	{
		SEDX_CORE_ASSERT(m_ActiveProject);
		return std::filesystem::path(m_ActiveProject->GetConfig().projectPath) /
			   m_ActiveProject->GetConfig().pluginPath;
	}

} // namespace SceneryEditorX

// -------------------------------------------------------

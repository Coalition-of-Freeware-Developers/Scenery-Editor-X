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
 * project.h
 * -------------------------------------------------------
 * Created: 7/5/2025
 * -------------------------------------------------------
 */
#pragma once
#include "project_settings.h"
#include <filesystem>
#include <SceneryEditorX/asset/manager/asset_manager.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	class Project : public RefCounted
	{
	public:
		Project();
		~Project();

		// -------------------------------------------------------

		const ProjectConfig &GetConfig() const { return m_Config; }

		static Ref<Project> GetActive() { return m_ActiveProject; }

		static void SetActive(Ref<Project> &project);

		void CreateProject(std::string name, std::filesystem::path path);

		void Load(const std::filesystem::path &inPath);
		void Save(const std::filesystem::path &inPath);

		static Ref<AssetManager> GetAssetManager() { return m_AssetManager; }

		// -------------------------------------------------------

		static const std::string &GetProjectName();

		static std::filesystem::path GetProjectDirectory();

		static std::filesystem::path GetAssetRegistryPath();

		static std::filesystem::path GetActiveAssetDirectory();

		std::filesystem::path GetAssetDirectory() const;

		static std::filesystem::path GetMeshPath();

		static std::filesystem::path GetCacheDirectory();

		static std::filesystem::path GetScriptModulePath();

	private:
		ProjectConfig m_Config;
		std::string m_ProjectName;
		std::filesystem::path m_ProjectPath;
		std::filesystem::path m_BinPath;

		static Ref<AssetManager> m_AssetManager;
		static Ref<Project> m_ActiveProject;
	};

}

// -------------------------------------------------------

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
#include <filesystem>
//#include "SceneryEditorX/asset/managers/asset_manager.h"
//#include "SceneryEditorX/asset/managers/editor_asset_manager.h"
#include "project_settings.h"
#include <SceneryEditorX/logging/asserts.h>
#include <SceneryEditorX/core/platform/config/editor_config.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Project : public RefCounted
	{
	public:
	    Project();
	    ~Project();

	    // -------------------------------------------------------

		const ProjectConfig &GetConfig() const { return config; }

        static std::filesystem::path GetAssetRegistryPath()
		{
            SEDX_CORE_ASSERT(activeProject);
            return std::filesystem::path(activeProject->GetConfig().projectPath) /
                   activeProject->GetConfig().assetRegistry;
		}

        static std::filesystem::path GetActiveAssetDirectory()
		{
            SEDX_CORE_ASSERT(activeProject);
            return activeProject->GetAssetDirectory();
		}

        static Ref<Project> GetActive() { return activeProject; }
        static void SetActive(Ref<Project> &project);

        void CreateProject(std::string name, std::filesystem::path path);
	    void Load(const std::filesystem::path &InPath);
	    void Save(const std::filesystem::path &InPath);

        //static Ref<AssetManager> GetAssetManager() { return s_AssetManager; }
        //static Ref<EditorAssetManager> GetEditorAssetManager() { return s_AssetManager.As<EditorAssetManager>(); }

        std::filesystem::path GetAssetDirectory();

	    // -------------------------------------------------------

		static const std::string &GetProjectName()
        {
            SEDX_CORE_ASSERT(activeProject);
            return activeProject->GetConfig().name;
        }

		static std::filesystem::path GetProjectDirectory()
		{
            SEDX_CORE_ASSERT(activeProject);
            return activeProject->GetConfig().projectPath;
		}

	    static std::filesystem::path GetCacheDirectory()
		{
            SEDX_CORE_ASSERT(activeProject);
            return std::filesystem::path(activeProject->GetConfig().projectPath) / "cache";
		}

	private:
        ProjectConfig config;
        std::string projectName;
        std::filesystem::path projectPath;
        std::filesystem::path binPath;

	    //static Ref<AssetManager> s_AssetManager;
        inline static Ref<Project> activeProject;

	};

    // -------------------------------------------------------

}

// -------------------------------------------------------

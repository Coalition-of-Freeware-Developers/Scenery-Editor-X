/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* project.h
* -------------------------------------------------------
* Created: 16/10/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/project/project.h>

namespace Project
{
	struct CacheData
	{
	    char projectPath[1024] = "projects/default.edX";
	    char binPath[1024] = "projects/default.bin";
	};

    /// -------------------------------------------------------

	class DefaultProject : public SceneryEditorX::Project
	{
	public:
		DefaultProject();
		~DefaultProject();
        void ReadProjCache();
        void WriteProjCache();

		static std::filesystem::path GetAssetDirectory();
		static std::filesystem::path GetAssetRegistryPath();

        /// -------------------------------------------------------

	private:
	    CacheData cacheData;
	};

} // namespace Project

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* asset_pack_header.h
* -------------------------------------------------------
* Created: 13/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <map>
#include <SceneryEditorX/asset/asset.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	struct AssetPackFile
	{
		struct AssetInfo
		{
			uint64_t PackedOffset;
			uint64_t PackedSize;
			uint16_t Type;
			uint16_t Flags; // compressed type, etc.
		};
		
		struct SceneInfo
		{
			uint64_t PackedOffset = 0;
			uint64_t PackedSize = 0;
			uint16_t Flags = 0; // compressed type, etc.
			std::map<uint64_t, AssetInfo> Assets; // AssetHandle->AssetInfo
		};

		struct IndexTable
		{
			uint64_t PackedAppBinaryOffset = 0;
			uint64_t PackedAppBinarySize = 0;
			std::map<uint64_t, SceneInfo> Scenes; // AssetHandle->SceneInfo
		};

		struct FileHeader
		{
            const char HEADER[3] = {'e', 'd', 'X'};
			uint32_t Version = 3;
			uint64_t BuildVersion = 0; // Usually date/time format (eg. 202210061535)
		};

		FileHeader Header;
		IndexTable Index;
	};

}

/// -------------------------------------------------------

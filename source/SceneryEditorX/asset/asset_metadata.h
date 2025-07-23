/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* asset_metadata.h
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/asset/asset_types.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	enum class AssetStatus : uint8_t
	{
		None = 0,
		Ready = 1,
		Invalid = 2,
		Loading = 3
	};

	struct AssetMetadata
	{
        AssetHandle Handle = 0;
		AssetType Type;
		std::filesystem::path FilePath;

		AssetStatus Status = AssetStatus::None;

		uint64_t FileLastWriteTime = 0; // TODO: this is the last write time of the file WE LOADED
		bool IsDataLoaded = false;

		bool IsValid() const { return Handle != 0; }
	};

	struct EditorAssetLoadResponse
	{
		AssetMetadata Metadata;
		Ref<Asset> Asset;
	};

}

/// -------------------------------------------------------

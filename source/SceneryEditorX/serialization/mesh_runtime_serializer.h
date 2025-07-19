/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* mesh_runtime_serializer.h
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/asset/asset.h>
#include <SceneryEditorX/asset/asset_pack_header.h>
#include <SceneryEditorX/filestreaming/file_streaming.h>
#include <SceneryEditorX/serialization/asset_serializer.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	class MeshRuntimeSerializer
	{
	public:
		bool SerializeToAssetPack(uint64_t handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo);
		Ref<Asset> DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo);
	};

}

/// --------------------------------------------------------


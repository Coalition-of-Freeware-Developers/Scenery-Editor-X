/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* mesh_serializer.h
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/asset/asset.h>
#include <SceneryEditorX/asset/asset_metadata.h>
#include <SceneryEditorX/asset/asset_pack_header.h>
#include <SceneryEditorX/serialization/asset_serializer.h>
#include <SceneryEditorX/utils/filestreaming/file_streaming.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	class MeshSourceSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override {}
		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;

		virtual bool SerializeToAssetPack(uint64_t handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const override;
		virtual Ref<Asset> DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const override;
	};

	/// -------------------------------------------------------

	class MeshSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override;
		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
		virtual void RegisterDependencies(const AssetMetadata& metadata) const override;

		virtual bool SerializeToAssetPack(uint64_t handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const override;
		virtual Ref<Asset> DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const override;
	};

	/// -------------------------------------------------------

	class StaticMeshSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override;
		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
		virtual void RegisterDependencies(const AssetMetadata& metadata) const override;

		virtual bool SerializeToAssetPack(uint64_t handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const override;
		virtual Ref<Asset> DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const override;
	};

}

/// -------------------------------------------------------

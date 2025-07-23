/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* asset_importer.cpp
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/asset/asset_importer.h>
#include <SceneryEditorX/logging/profiler.hpp>
#include <SceneryEditorX/project/project.h>
#include <SceneryEditorX/serialization/mesh_serializer.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	/*
	void AssetImporter::Init()
	{
		s_Serializers.clear();
		s_Serializers[AssetType::Prefab] = CreateScope<PrefabSerializer>();
		s_Serializers[AssetType::Texture] = CreateScope<TextureSerializer>();
		s_Serializers[AssetType::Mesh] = CreateScope<MeshSerializer>();
		s_Serializers[AssetType::MeshSource] = CreateScope<MeshSourceSerializer>();
		s_Serializers[AssetType::Material] = CreateScope<MaterialAssetSerializer>();
		s_Serializers[AssetType::EnvMap] = CreateScope<EnvironmentSerializer>();
		s_Serializers[AssetType::Scene] = CreateScope<SceneAssetSerializer>();
		s_Serializers[AssetType::Font] = CreateScope<FontSerializer>();
		//s_Serializers[AssetType::Animation] = CreateScope<AnimationAssetSerializer>();
		//s_Serializers[AssetType::AnimationGraph] = CreateScope<AnimationGraphAssetSerializer>();
		s_Serializers[AssetType::ScriptFile] = CreateScope<ScriptFileSerializer>();
	}
	*/

	/*
	void AssetImporter::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset)
	{
		if (!s_Serializers.contains(metadata.Type))
		{
			SEDX_CORE_WARN("There's currently no importer for assets of type {0}", metadata.FilePath.stem().string());
			return;
		}

		s_Serializers[asset->GetAssetType()]->Serialize(metadata, asset);
	}
	*/

	/*
	void AssetImporter::Serialize(const Ref<Asset>& asset)
	{
		const AssetMetadata& metadata = Project::GetEditorAssetManager()->GetMetadata(asset->Handle);
		Serialize(metadata, asset);
	}
	*/

	/*
	bool AssetImporter::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset)
	{
		SEDX_PROFILE_FUNC("AssetImporter::TryLoadData");

		if (!s_Serializers.contains(metadata.Type))
		{
			SEDX_CORE_WARN("There's currently no importer for assets of type {0}", metadata.FilePath.stem().string());
			return false;
		}

		// SEDX_CORE_TRACE("AssetImporter::TryLoadData - {}", metadata.FilePath);
		return s_Serializers[metadata.Type]->TryLoadData(metadata, asset);
	}
	*/

	/*
	void AssetImporter::RegisterDependencies(const AssetMetadata& metadata)
	{
		if (!s_Serializers.contains(metadata.Type))
		{
			SEDX_CORE_WARN("There's currently no importer for assets of type {0}", metadata.FilePath.stem().string());
			return;
		}

		s_Serializers[metadata.Type]->RegisterDependencies(metadata);
	}
	*/

	/*
	bool AssetImporter::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo)
	{
		outInfo.Size = 0;

		if (!AssetManager::IsAssetHandleValid(handle))
			return false;

		if (auto asset = AssetManager::Get<Asset>(handle); !asset)
		{
			SEDX_CORE_WARN("Failed to get asset with handle {0}", handle);
			return false;
		}

		const AssetType type = AssetManager::Get(handle);
		if (!s_Serializers.contains(type))
		{
			const auto& metadata = Project::GetEditorAssetManager()->GetMetadata(handle);
			SEDX_CORE_WARN("There's currently no serializer for assets of type {0}", metadata.FilePath.stem().string());
			return false;
		}

		return s_Serializers[type]->SerializeToAssetPack(handle, stream, outInfo);
	}
	*/

	/*
	Ref<Asset> AssetImporter::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo)
	{
		AssetType assetType = (AssetType)assetInfo.Type;
		if (!s_Serializers.contains(assetType))
			return nullptr;

		return s_Serializers[assetType]->DeserializeFromAssetPack(stream, assetInfo);
	}
	*/

	/*
	Ref<Scene> AssetImporter::DeserializeSceneFromAssetPack(FileStreamReader& stream, const AssetPackFile::SceneInfo& sceneInfo)
	{
        constexpr AssetType assetType = AssetType::Scene;
		if (!s_Serializers.contains(assetType))
			return nullptr;

		const SceneAssetSerializer *sceneAssetSerializer = static_cast<SceneAssetSerializer *>(s_Serializers[assetType].get());
		return sceneAssetSerializer->DeserializeSceneFromAssetPack(stream, sceneInfo);
	}

	std::unordered_map<AssetType, Scope<AssetSerializer>> AssetImporter::s_Serializers;
	*/

}

/// -------------------------------------------------------

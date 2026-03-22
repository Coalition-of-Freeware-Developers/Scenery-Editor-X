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
 * asset_importer.cpp
 * -------------------------------------------------------
 * Created: 20/03/2026
 * -------------------------------------------------------
 */
#include "asset_importer.h"
#include <SceneryEditorX/project/project.h>
#include <SceneryEditorX/scene/scene.h>
#include <SceneryEditorX/utils/filestreaming/file_streaming.h>

// -------------------------------------------------------

namespace SceneryEditorX
{	

	/*
	std::unordered_map<AssetType, Scope<AssetSerializer>> AssetImporter::s_Serializers;
	
	void AssetImporter::Init()
	{
		s_Serializers.clear();
		s_Serializers[AssetType::Prefab]		= CreateScope<PrefabSerializer>();
		s_Serializers[AssetType::Texture]		= CreateScope<TextureSerializer>();
		s_Serializers[AssetType::Mesh]			= CreateScope<MeshSerializer>();
		s_Serializers[AssetType::Material]		= CreateScope<MaterialAssetSerializer>();
		s_Serializers[AssetType::EnvMap]		= CreateScope<EnvironmentSerializer>();
		s_Serializers[AssetType::Scene]			= CreateScope<SceneAssetSerializer>();
		s_Serializers[AssetType::Font]			= CreateScope<FontSerializer>();
		s_Serializers[AssetType::MeshCollider]	= CreateScope<MeshColliderSerializer>();
		s_Serializers[AssetType::Skeleton]		= CreateScope<SkeletonAssetSerializer>();
		s_Serializers[AssetType::Animation]		= CreateScope<AnimationAssetSerializer>();
		s_Serializers[AssetType::ScriptFile]	= CreateScope<ScriptFileSerializer>();
	}

	void AssetImporter::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset)
	{
		if (!s_Serializers.contains(metadata.type))
		{
			SEDX_CORE_WARN("There's currently no importer for assets of type {0}", metadata.filePath.stem().string());
			return;
		}

		s_Serializers[asset->GetAssetType()]->Serialize(metadata, asset);
	}

	void AssetImporter::Serialize(const Ref<Asset>& asset)
	{
		const AssetMetadata& metadata = Project::GetAssetManager()->GetMetadata(asset->pHandle);
		Serialize(metadata, asset);
	}

	bool AssetImporter::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset)
	{
		//SEDX_PROFILE_FUNC("AssetImporter::TryLoadData");

		if (s_Serializers.find(metadata.type) == s_Serializers.end())
		{
			SEDX_CORE_WARN("There's currently no importer for assets of type {0}", metadata.filePath.stem().string());
			return false;
		}

		// HZ_CORE_TRACE("AssetImporter::TryLoadData - {}", metadata.FilePath);
		return s_Serializers[metadata.type]->TryLoadData(metadata, asset);
	}

	void AssetImporter::RegisterDependencies(const AssetMetadata& metadata)
	{
		if (s_Serializers.find(metadata.type) == s_Serializers.end())
		{
			SEDX_CORE_WARN("There's currently no importer for assets of type {0}", metadata.filePath.stem().string());
			return;
		}

		s_Serializers[metadata.type]->RegisterDependencies(metadata);
	}

	bool AssetImporter::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo)
	{
		outInfo.Size = 0;

		if (!AssetManager::IsAssetHandleValid(handle))
			return false;

		AssetType type = AssetManager::GetAssetType(handle);
		if (s_Serializers.find(type) == s_Serializers.end())
		{
			const auto& metadata = Project::GetAssetManager()->GetMetadata(handle);
			SEDX_CORE_WARN("There's currently no serializer for assets of type {0}", metadata.filePath.stem().string());
			return false;
		}

		return s_Serializers[type]->SerializeToAssetPack(handle, stream, outInfo);
	}

	Ref<Asset> AssetImporter::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo)
	{
		AssetType assetType = (AssetType)assetInfo.Type;
		if (s_Serializers.find(assetType) == s_Serializers.end())
			return nullptr;

		return s_Serializers[assetType]->DeserializeFromAssetPack(stream, assetInfo);
	}

	Ref<Scene> AssetImporter::DeserializeSceneFromAssetPack(FileStreamReader& stream, const AssetPackFile::SceneInfo& sceneInfo)
	{
		AssetType assetType = AssetType::Scene;
		if (s_Serializers.find(assetType) == s_Serializers.end())
			return nullptr;

		SceneAssetSerializer* sceneAssetSerializer = (SceneAssetSerializer*)s_Serializers[assetType].get();
		return sceneAssetSerializer->DeserializeSceneFromAssetPack(stream, sceneInfo);
	}
	*/

}

// -------------------------------------------------------

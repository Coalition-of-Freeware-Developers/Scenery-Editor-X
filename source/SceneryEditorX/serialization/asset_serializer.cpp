/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* asset_serializer.cpp
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#include <cstring>
#include <filesystem>
#include <SceneryEditorX/asset/asset_pack_header.h>
#include <SceneryEditorX/platform/file_manager.hpp>
#include <SceneryEditorX/project/project.h>
#include <SceneryEditorX/renderer/fonts/font.h>
#include <SceneryEditorX/renderer/shaders/shader.h>
#include <SceneryEditorX/renderer/texture.h>
#include <SceneryEditorX/serialization/asset_serializer.h>
#include <SceneryEditorX/serialization/json_serializer.h>
#include <SceneryEditorX/serialization/serialization_macros.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    namespace Utils
    {
        struct TransformComponent;
    }

    //////////////////////////////////////////////////////////////////////////////////
	/// TextureSerializer
	//////////////////////////////////////////////////////////////////////////////////

	bool TextureSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
        asset = CreateRef<Texture2D>(TextureSpecification(), Project::GetEditorAssetManager()->GetFileSystemPathString(metadata));
		asset->Handle = metadata.Handle;

		const bool result = asset.As<Texture2D>()->Loaded();
		if (!result)
			asset->SetFlag(AssetFlag::Invalid, true);

		return result;
	}

	//////////////////////////////////////////////////////////////////////////////////
	/// FontSerializer
	//////////////////////////////////////////////////////////////////////////////////

	bool FontSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		asset = CreateRef<Font>(Project::GetEditorAssetManager()->GetFileSystemPathString(metadata));
		asset->Handle = metadata.Handle;

#if 0
		// TODO: we should probably handle fonts not loading correctly
		bool result = asset.As<Font>()->Loaded();
		if (!result)
			asset->SetFlag(AssetFlag::Invalid, true);
#endif

		return true;
	}

	bool FontSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		outInfo.Offset = stream.GetStreamPosition();

		const Ref<Font> font = AssetManager::Get<Font>(handle);
		const auto path = Project::GetEditorAssetManager()->GetFileSystemPath(handle);
		stream.WriteString(font->GetName());
        const Memory::Buffer fontData = IO::FileSystem::ReadBytes(path);
		stream.WriteBuffer(fontData);

		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
		return true;
	}

	Ref<Asset> FontSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		stream.SetStreamPosition(assetInfo.PackedOffset);

		std::string name;
		stream.ReadString(name);
        Memory::Buffer &fontData = nullptr;
		stream.ReadBuffer(fontData);

		return CreateRef<Font>(name, fontData);
	}

	//////////////////////////////////////////////////////////////////////////////////
	/// MaterialAssetSerializer
	//////////////////////////////////////////////////////////////////////////////////

	void MaterialAssetSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		const Ref<MaterialAsset> materialAsset = asset.As<MaterialAsset>();

		const std::string jsonString = SerializeToJSON(materialAsset);

		std::ofstream fout(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		fout << jsonString;
	}

	bool MaterialAssetSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		Ref<MaterialAsset> materialAsset;
		if (!DeserializeFromJSON(GetJSON(metadata), materialAsset, metadata.Handle))
            return false;

        asset = materialAsset;
		return true;
	}

	void MaterialAssetSerializer::RegisterDependencies(const AssetMetadata& metadata) const
	{
		RegisterDependenciesFromJSON(GetJSON(metadata), metadata.Handle);
	}

	bool MaterialAssetSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		const Ref<MaterialAsset> materialAsset = AssetManager::Get<MaterialAsset>(handle);

		const std::string jsonString = SerializeToJSON(materialAsset);
		outInfo.Offset = stream.GetStreamPosition();
		stream.WriteString(jsonString);
		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
		return true;
	}

	Ref<Asset> MaterialAssetSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		stream.SetStreamPosition(assetInfo.PackedOffset);
		std::string jsonString;
		stream.ReadString(jsonString);

		Ref<MaterialAsset> materialAsset;
        if (const bool result = DeserializeFromJSON(jsonString, materialAsset, 0); !result)
			return nullptr;

		return materialAsset;
	}

	std::string MaterialAssetSerializer::SerializeToJSON(const Ref<MaterialAsset> &materialAsset) const
	{
		nlohmann::json materialJson;
		nlohmann::json materialObject;

		/**
		 * TODO: this should have shader UUID when that's a thing right now only supports PBR or Transparent shaders
		 */
		Ref<Shader> transparentShader = Renderer::GetShaderLibrary()->Get("PBR_Transparent");
		bool transparent = materialAsset->GetMaterial()->GetShader() == transparentShader;
		SEDX_SERIALIZE_PROPERTY(Transparent, transparent, materialObject);

		SEDX_SERIALIZE_PROPERTY(AlbedoColor, materialAsset->GetAlbedoColor(), materialObject);
		SEDX_SERIALIZE_PROPERTY(Emission, materialAsset->GetEmission(), materialObject);
		if (!transparent)
		{
			SEDX_SERIALIZE_PROPERTY(UseNormalMap, materialAsset->IsUsingNormalMap(), materialObject);
			SEDX_SERIALIZE_PROPERTY(Metalness, materialAsset->GetMetalness(), materialObject);
			SEDX_SERIALIZE_PROPERTY(Roughness, materialAsset->GetRoughness(), materialObject);
		}
		else
		{
			SEDX_SERIALIZE_PROPERTY(Transparency, materialAsset->GetTransparency(), materialObject);
		}

		{
			Ref<Texture2D> albedoMap = materialAsset->GetAlbedoMap();
			bool hasAlbedoMap = albedoMap ? !albedoMap.EqualsObject(Renderer::GetWhiteTexture()) : false;
			AssetHandle albedoMapHandle = hasAlbedoMap ? albedoMap->Handle : UUID(0);
			SEDX_SERIALIZE_PROPERTY(AlbedoMap, albedoMapHandle, materialObject);
		}
		if (!transparent)
		{
            Ref<Texture2D> normalMap = materialAsset->GetNormalMap();
            bool hasNormalMap = normalMap ? !normalMap.EqualsObject(Renderer::GetWhiteTexture()) : false;
            AssetHandle normalMapHandle = hasNormalMap ? normalMap->Handle : UUID(0);
            SEDX_SERIALIZE_PROPERTY(NormalMap, normalMapHandle, materialObject);

            Ref<Texture2D> metalnessMap = materialAsset->GetMetalnessMap();
            bool hasMetalnessMap = metalnessMap ? !metalnessMap.EqualsObject(Renderer::GetWhiteTexture()) : false;
            AssetHandle metalnessMapHandle = hasMetalnessMap ? metalnessMap->Handle : UUID(0);
            SEDX_SERIALIZE_PROPERTY(MetalnessMap, metalnessMapHandle, materialObject);

            Ref<Texture2D> roughnessMap = materialAsset->GetRoughnessMap();
            bool hasRoughnessMap = roughnessMap ? !roughnessMap.EqualsObject(Renderer::GetWhiteTexture()) : false;
            AssetHandle roughnessMapHandle = hasRoughnessMap ? roughnessMap->Handle : UUID(0);
            SEDX_SERIALIZE_PROPERTY(RoughnessMap, roughnessMapHandle, materialObject);
        }

		SEDX_SERIALIZE_PROPERTY(MaterialFlags, materialAsset->GetMaterial()->GetFlags(), materialObject);

		materialJson["Material"] = materialObject;
		return materialJson.dump(2);
	}

	std::string MaterialAssetSerializer::GetJSON(const AssetMetadata& metadata) const
	{
		const std::ifstream stream(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		if (!stream.is_open())
			return {};

		std::stringstream strStream;
		strStream << stream.rdbuf();
		return strStream.str();
	}

	void MaterialAssetSerializer::RegisterDependenciesFromJSON(const std::string& jsonString, AssetHandle handle) const
	{
		AssetManager::DeregisterDependencies(handle);

		nlohmann::json root = nlohmann::json::parse(jsonString);
		nlohmann::json materialNode = root["Material"];

		AssetHandle albedoMap, normalMap, metalnessMap, roughnessMap;
		SEDX_DESERIALIZE_PROPERTY(AlbedoMap, albedoMap, materialNode, (AssetHandle)0)
		SEDX_DESERIALIZE_PROPERTY(NormalMap, normalMap, materialNode, (AssetHandle)0)
		SEDX_DESERIALIZE_PROPERTY(MetalnessMap, metalnessMap, materialNode, (AssetHandle)0)
		SEDX_DESERIALIZE_PROPERTY(RoughnessMap, roughnessMap, materialNode, (AssetHandle)0)

		/// note: we should always register something, even 0.
		AssetManager::RegisterDependency(albedoMap, handle);
		AssetManager::RegisterDependency(normalMap, handle);
		AssetManager::RegisterDependency(metalnessMap, handle);
		AssetManager::RegisterDependency(roughnessMap, handle);
	}

	bool MaterialAssetSerializer::DeserializeFromJSON(const std::string& jsonString, Ref<MaterialAsset>& targetMaterialAsset, AssetHandle handle) const
	{
		RegisterDependenciesFromJSON(jsonString, handle);

		nlohmann::json root = nlohmann::json::parse(jsonString);
		nlohmann::json materialNode = root["Material"];

		bool transparent = false;
		SEDX_DESERIALIZE_PROPERTY(Transparent, transparent, materialNode, false)

		targetMaterialAsset = CreateRef<MaterialAsset>(transparent);
		targetMaterialAsset->Handle = handle;

		SEDX_DESERIALIZE_PROPERTY(AlbedoColor, targetMaterialAsset->GetAlbedoColor(), materialNode, Vec3(0.8f))
		SEDX_DESERIALIZE_PROPERTY(Emission, targetMaterialAsset->GetEmission(), materialNode, 0.0f)

		if (!transparent)
		{
			targetMaterialAsset->SetUseNormalMap(materialNode.contains("UseNormalMap") ? materialNode["UseNormalMap"].get<bool>() : false);
			SEDX_DESERIALIZE_PROPERTY(Metalness, targetMaterialAsset->GetMetalness(), materialNode, 0.0f)
			SEDX_DESERIALIZE_PROPERTY(Roughness, targetMaterialAsset->GetRoughness(), materialNode, 0.5f)
		}
		else
		{
			SEDX_DESERIALIZE_PROPERTY(Transparency, targetMaterialAsset->GetTransparency(), materialNode, 1.0f)
		}

		AssetHandle albedoMap, normalMap, metalnessMap, roughnessMap;
		SEDX_DESERIALIZE_PROPERTY(AlbedoMap, albedoMap, materialNode, (AssetHandle)0)
		if (!transparent)
		{
			SEDX_DESERIALIZE_PROPERTY(NormalMap, normalMap, materialNode, (AssetHandle)0)
			SEDX_DESERIALIZE_PROPERTY(MetalnessMap, metalnessMap, materialNode, (AssetHandle)0)
			SEDX_DESERIALIZE_PROPERTY(RoughnessMap, roughnessMap, materialNode, (AssetHandle)0)
		}
		if (albedoMap && AssetManager::IsAssetHandleValid(albedoMap))
            targetMaterialAsset->SetAlbedoMap(albedoMap);

        if (normalMap && AssetManager::IsAssetHandleValid(normalMap))
            targetMaterialAsset->SetNormalMap(normalMap);

        if (metalnessMap && AssetManager::IsAssetHandleValid(metalnessMap))
            targetMaterialAsset->SetMetalnessMap(metalnessMap);

        if (roughnessMap && AssetManager::IsAssetHandleValid(roughnessMap))
            targetMaterialAsset->SetRoughnessMap(roughnessMap);

        SEDX_DESERIALIZE_PROPERTY(MaterialFlags, roughnessMap, materialNode, (AssetHandle)0)
		if (materialNode.contains("MaterialFlags"))
			targetMaterialAsset->GetMaterial()->SetFlags(materialNode["MaterialFlags"].get<uint32_t>());

		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////
	/// EnvironmentSerializer
	//////////////////////////////////////////////////////////////////////////////////

	bool EnvironmentSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		auto [radiance, irradiance] = Renderer::CreateEnvironmentMap(Project::GetEditorAssetManager()->GetFileSystemPathString(metadata));

		if (!radiance || !irradiance)
			return false;

		asset = CreateRef<Environment>(radiance, irradiance);
		asset->Handle = metadata.Handle;
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////
	/// PrefabSerializer
	//////////////////////////////////////////////////////////////////////////////////

	void PrefabSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		const Ref<Prefab> prefab = asset.As<Prefab>();
		const std::string jsonString = SerializeToJSON(prefab);
		std::ofstream fout(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		fout << jsonString;
	}

	bool PrefabSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		const std::ifstream stream(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		if (!stream.is_open())
			return false;

		std::stringstream strStream;
		strStream << stream.rdbuf();

		Ref<Prefab> prefab = CreateRef<Prefab>();
        if (const bool success = DeserializeFromJSON(strStream.str(), prefab); !success)
			return false;

		asset = prefab;
		asset->Handle = metadata.Handle;
		return true;
	}

	std::string PrefabSerializer::SerializeToJSON(const Ref<Prefab> &prefab) const
	{
		nlohmann::json root;
		nlohmann::json prefabArray = nlohmann::json::array();

		prefab->m_Scene->m_Registry.each([&](auto entityID)
		{
			Entity entity = { entityID, prefab->m_Scene.Raw() };
			if (!entity || !entity.HasComponent<IDComponent>())
				return;

			///< Use the existing binary serialization system for entities
			nlohmann::json entityJson;
			nlohmann::json serializedEntity;
			BinaryStorage storage;
			Serializer serializer(serializedEntity, storage, Serializer::SAVE, *AssetManager::Instance());

			/**
			 * TODO: Replace with proper entity serialization
			 * For now, just serialize basic entity data
             */
			auto& idComponent = entity.GetComponent<IDComponent>();
			entityJson["ID"] = static_cast<uint64_t>(idComponent.ID);

			if (entity.HasComponent<TagComponent>())
			{
				auto& tagComponent = entity.GetComponent<TagComponent>();
				entityJson["Tag"] = tagComponent.Tag;
			}

			if (entity.HasComponent<Utils::TransformComponent>())
			{
                auto &transformComponent = entity.GetComponent<Utils::TransformComponent>();
				entityJson["Transform"]["Translation"] = nlohmann::json::array(
            {
					transformComponent.Translation.x, transformComponent.Translation.y, transformComponent.Translation.z
                });
				entityJson["Transform"]["Rotation"] = nlohmann::json::array(
            {
					transformComponent.Rotation.x, transformComponent.Rotation.y, transformComponent.Rotation.z
				});
				entityJson["Transform"]["Scale"] = nlohmann::json::array(
            {
					transformComponent.Scale.x, transformComponent.Scale.y, transformComponent.Scale.z
				});
			}

			prefabArray.push_back(entityJson);
		});

		root["Prefab"] = prefabArray;
		return root.dump(2);
	}

	bool PrefabSerializer::DeserializeFromJSON(const std::string& jsonString, const Ref<Prefab> &prefab) const
	{
		nlohmann::json data = nlohmann::json::parse(jsonString);
		if (!data.contains("Prefab"))
			return false;

        /**
         * TODO: Implement proper entity deserialization using binary serialization system
		 * For now, create basic entities
         */
		for (nlohmann::json prefabArray = data["Prefab"]; const auto& entityJson : prefabArray)
		{
			Entity entity = prefab->m_Scene->CreateEntity();

			if (entityJson.contains("ID"))
			{
				auto &idComponent = entity.GetComponent<IDComponent>();
				idComponent.ID = static_cast<UUID>(entityJson["ID"].get<uint64_t>());
			}

			if (entityJson.contains("Tag"))
			{
				auto &tagComponent = entity.GetComponent<TagComponent>();
				tagComponent.Tag = entityJson["Tag"].get<std::string>();
			}

			if (entityJson.contains("Transform"))
			{
				auto &transformComponent = entity.GetComponent<Utils::TransformComponent>();
				if (entityJson["Transform"].contains("Translation"))
				{
                    if (auto translation = entityJson["Transform"]["Translation"]; translation.is_array() && translation.size() == 3) {
						transformComponent.Translation = {
							translation[0].get<float>(),
							translation[1].get<float>(),
							translation[2].get<float>()
						};
					}
				}

				if (entityJson["Transform"].contains("Rotation"))
				{
                    if (auto rotation = entityJson["Transform"]["Rotation"]; rotation.is_array() && rotation.size() == 3) {
						transformComponent.Rotation = {
							rotation[0].get<float>(),
							rotation[1].get<float>(),
							rotation[2].get<float>()
						};
					}
				}

				if (entityJson["Transform"].contains("Scale"))
				{
                    if (auto scale = entityJson["Transform"]["Scale"]; scale.is_array() && scale.size() == 3) {
						transformComponent.Scale = {
							scale[0].get<float>(),
							scale[1].get<float>(),
							scale[2].get<float>()
						};
					}
				}
			}
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////
	/// SceneAssetSerializer
	//////////////////////////////////////////////////////////////////////////////////

	void SceneAssetSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		/**
		 * SceneAsset uses the existing binary serialization system through AssetManager
		 * The actual serialization happens in AssetManager::SaveProject() which uses
		 * the binary serialization system with SerializeWriter and the Serializer struct
		 * For individual scene file export, we would use the binary system:
		 */

		const Ref<Scene> sceneAsset = asset.As<Scene>();

		FileStreamWriter writer(Project::GetEditorAssetManager()->GetFileSystemPath(metadata).string());
		if (!writer.IsStreamGood())
		{
			SEDX_CORE_ERROR_TAG("SceneAssetSerializer", "Failed to create scene file");
			return;
		}

		///< Write file header for standalone scene files
		constexpr uint32_t SCENE_FILE_VERSION = 1;
		constexpr char SCENE_MAGIC[4] = {'S', 'C', 'N', 'E'};

		writer.WriteRaw(SCENE_MAGIC);
		writer.WriteRaw(SCENE_FILE_VERSION);
		writer.WriteString(sceneAsset->name);
		writer.WriteRaw(sceneAsset->uuid);

		///< Use the existing binary serialization system
		using namespace SceneryEditorX::Serialization;

		///< Serialize scene data - this would need Scene to implement the binary serialization interface
		///< The Scene should have static Serialize/Deserialize methods or use SERIALIZABLE macro
		Serialize(&writer, *sceneAsset);


		// For now, scenes are only serialized as part of the project through AssetManager
		SEDX_CORE_WARN_TAG("SceneAssetSerializer", "Individual scene serialization not implemented - scenes are saved as part of project files");
	}

	bool SceneAssetSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		/**
		 * SceneAsset loading is handled by AssetManager::LoadProject() which uses the binary serialization
		 * system with SerializeReader and the Serializer struct
		 */

		/// For individual scene file loading, we would use the binary system:

		FileStreamReader reader(Project::GetEditorAssetManager()->GetFileSystemPath(metadata).string());
		if (!reader.IsStreamGood())
		{
			SEDX_CORE_ERROR_TAG("SceneAssetSerializer", "Failed to open scene file");
			return false;
		}

		///< Read and validate file header
		char magic[4];
		reader.ReadRaw(magic);
		if (std::memcmp(magic, "SCENE", 4) != 0)
		{
			SEDX_CORE_ERROR_TAG("SceneAssetSerializer", "Invalid scene file magic");
			return false;
		}

		uint32_t version;
		reader.ReadRaw(version);
		if (version > 1)
		{
			SEDX_CORE_ERROR_TAG("SceneAssetSerializer", "Unsupported scene file version: {}", version);
			return false;
		}

		///< Read basic scene info
		std::string sceneName;
		uint32_t sceneUuid;
		reader.ReadString(sceneName);
		reader.ReadRaw(sceneUuid);

		///< Create scene asset
		const Ref<Scene> sceneAsset = CreateRef<Scene>();
		sceneAsset->name = sceneName;
		sceneAsset->uuid = sceneUuid;
		sceneAsset->Handle = metadata.Handle;

		///< Use the existing binary serialization system
		using namespace SceneryEditorX::Serialization;

		///< Deserialize scene data - this would need Scene to implement the binary serialization interface
		if (!Deserialize(&reader, *sceneAsset))
		{
			SEDX_CORE_ERROR_TAG("SceneAssetSerializer", "Failed to deserialize scene data");
			return false;
		}

		asset = sceneAsset;
		return true;

		///< For now, scenes are only loaded as part of the project through AssetManager
		SEDX_CORE_WARN_TAG("SceneAssetSerializer", "Individual scene loading not implemented - scenes are loaded as part of project files");
		return false;
	}

	bool SceneAssetSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		outInfo.Offset = stream.GetStreamPosition();

		///< SceneAsset serialization for asset packs uses the existing binary serialization system.
		///< This leverages the same system used in AssetManager::SaveProject()

		///< The scene data would be serialized using the binary system:

		const Ref<Scene> sceneAsset = AssetManager::Get<Scene>(handle);

		///< Use the existing binary serialization system
		using namespace SceneryEditorX::Serialization;

		///< This would serialize the scene using the same system as the main project files
		if (!Serialize(&stream, *sceneAsset))
		{
			SEDX_CORE_ERROR_TAG("SceneAssetSerializer", "Failed to serialize scene to asset pack");
			return false;
		}

		///< For now, write a placeholder
		stream.WriteString("SCENE_PLACEHOLDER");

		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
		return true;
	}

	Ref<Asset> SceneAssetSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		stream.SetStreamPosition(assetInfo.PackedOffset);

		///< SceneAsset deserialization from asset packs uses the existing binary serialization system

		using namespace SceneryEditorX::Serialization;

		Ref<Scene> sceneAsset = CreateRef<Scene>();

		///< This would deserialize the scene using the same system as the main project files
		if (!Deserialize(&stream, *sceneAsset))
		{
			SEDX_CORE_ERROR_TAG("SceneAssetSerializer", "Failed to deserialize scene from asset pack");
			return nullptr;
		}

		return sceneAsset;

		/// For now, read the placeholder
		std::string placeholder;
		stream.ReadString(placeholder);

		/// Return a basic scene asset
		Ref<Scene> sceneAsset = CreateRef<Scene>();
		sceneAsset->name = "Scene";
		return sceneAsset;
	}

	//////////////////////////////////////////////////////////////////////////////////
	/// MeshColliderSerializer
	//////////////////////////////////////////////////////////////////////////////////

	void MeshColliderSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		const Ref<MeshColliderAsset> meshCollider = asset.As<MeshColliderAsset>();
		const std::string jsonString = SerializeToJSON(meshCollider);

		std::ofstream fout(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		fout << jsonString;
	}

	bool MeshColliderSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		const std::ifstream stream(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		if (!stream.is_open())
			return false;

		std::stringstream strStream;
		strStream << stream.rdbuf();

		if (strStream.rdbuf()->in_avail() == 0)
			return false;

		Ref<MeshColliderAsset> meshCollider = CreateRef<MeshColliderAsset>();
        if (const bool result = DeserializeFromJSON(strStream.str(), meshCollider); !result)
			return false;

		asset = meshCollider;
		asset->Handle = metadata.Handle;
		return true;
	}

	bool MeshColliderSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		const Ref<MeshColliderAsset> meshCollider = AssetManager::Get<MeshColliderAsset>(handle);

		const std::string jsonString = SerializeToJSON(meshCollider);
		outInfo.Offset = stream.GetStreamPosition();
		stream.WriteString(jsonString);
		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
		return true;
	}

	Ref<Asset> MeshColliderSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		stream.SetStreamPosition(assetInfo.PackedOffset);
		std::string jsonString;
		stream.ReadString(jsonString);

		Ref<MeshColliderAsset> meshCollider = CreateRef<MeshColliderAsset>();
        if (const bool result = DeserializeFromJSON(jsonString, meshCollider); !result)
			return nullptr;

		return meshCollider;
	}

	std::string MeshColliderSerializer::SerializeToJSON(const Ref<MeshColliderAsset> &meshCollider)
    {
		nlohmann::json root;

		root["ColliderMesh"] = meshCollider->ColliderMesh;
		root["EnableVertexWelding"] = meshCollider->EnableVertexWelding;
		root["VertexWeldTolerance"] = meshCollider->VertexWeldTolerance;
		root["FlipNormals"] = meshCollider->FlipNormals;
		root["CheckZeroAreaTriangles"] = meshCollider->CheckZeroAreaTriangles;
		root["AreaTestEpsilon"] = meshCollider->AreaTestEpsilon;
		root["ShiftVerticesToOrigin"] = meshCollider->ShiftVerticesToOrigin;
		root["AlwaysShareShape"] = meshCollider->AlwaysShareShape;
		root["CollisionComplexity"] = static_cast<uint8_t>(meshCollider->CollisionComplexity);

		root["ColliderScale"] = nlohmann::json::array({
			meshCollider->ColliderScale.x,
			meshCollider->ColliderScale.y,
			meshCollider->ColliderScale.z
		});

		root["PreviewScale"] = nlohmann::json::array({
			meshCollider->PreviewScale.x,
			meshCollider->PreviewScale.y,
			meshCollider->PreviewScale.z
		});

		nlohmann::json materialJson;
		materialJson["Friction"] = meshCollider->Material.Friction;
		materialJson["Restitution"] = meshCollider->Material.Restitution;
		root["ColliderMaterial"] = materialJson;

		return root.dump(2);
	}

	bool MeshColliderSerializer::DeserializeFromJSON(const std::string& jsonString, const Ref<MeshColliderAsset> &targetMeshCollider)
    {
		nlohmann::json data = nlohmann::json::parse(jsonString);

		targetMeshCollider->ColliderMesh = data.value("ColliderMesh", AssetHandle(0));

		if (data.contains("ColliderMaterial"))
		{
			targetMeshCollider->Material.Friction = data["ColliderMaterial"].value("Friction", 0.1f);
			targetMeshCollider->Material.Restitution = data["ColliderMaterial"].value("Restitution", 0.05f);
		}

		targetMeshCollider->EnableVertexWelding = data.value("EnableVertexWelding", true);
		targetMeshCollider->VertexWeldTolerance = glm::clamp<float>(data.value("VertexWeldTolerance", 0.1f), 0.05f, 1.0f);
		targetMeshCollider->FlipNormals = data.value("FlipNormals", false);
		targetMeshCollider->CheckZeroAreaTriangles = data.value("CheckZeroAreaTriangles", false);
		targetMeshCollider->AreaTestEpsilon = glm::max(0.06f, data.value("AreaTestEpsilon", 0.06f));
		targetMeshCollider->ShiftVerticesToOrigin = data.value("ShiftVerticesToOrigin", false);
		targetMeshCollider->AlwaysShareShape = data.value("AlwaysShareShape", false);
		targetMeshCollider->CollisionComplexity = static_cast<ECollisionComplexity>(data.value("CollisionComplexity", static_cast<uint8_t>(0)));

		///< Handle ColliderScale
		if (data.contains("ColliderScale") && data["ColliderScale"].is_array() && data["ColliderScale"].size() == 3) {
			targetMeshCollider->ColliderScale = Vec3(
				data["ColliderScale"][0].get<float>(),
				data["ColliderScale"][1].get<float>(),
				data["ColliderScale"][2].get<float>());
		}
        else
		{
			targetMeshCollider->ColliderScale = Vec3(1.0f);
		}

		///< Handle PreviewScale
		if (data.contains("PreviewScale") && data["PreviewScale"].is_array() && data["PreviewScale"].size() == 3)
		{
			targetMeshCollider->PreviewScale = Vec3(
				data["PreviewScale"][0].get<float>(),
				data["PreviewScale"][1].get<float>(),
				data["PreviewScale"][2].get<float>());
		}
        else
		{
			targetMeshCollider->PreviewScale = Vec3(1.0f);
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////
	/// ScriptFileSerializer
	//////////////////////////////////////////////////////////////////////////////////

	void ScriptFileSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		std::ofstream stream(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		SEDX_CORE_VERIFY(stream.is_open());

		std::ifstream templateStream("plugins/ExamplePlugin/PluginExample.py");
		SEDX_CORE_VERIFY(templateStream.is_open());

		std::stringstream templateStrStream;
		templateStrStream << templateStream.rdbuf();
		std::string templateString = templateStrStream.str();

		templateStream.close();

		auto replaceTemplateToken = [&templateString](const char* token, const std::string& value)
		{
			size_t pos = 0;
			while ((pos = templateString.find(token, pos)) != std::string::npos)
			{
				templateString.replace(pos, strlen(token), value);
				pos += strlen(token);
			}
		};

		auto scriptFileAsset = asset.As<ScriptFileAsset>();
		replaceTemplateToken("$NAMESPACE_NAME$", scriptFileAsset->GetClassNamespace());
		replaceTemplateToken("$CLASS_NAME$", scriptFileAsset->GetClassName());

		stream << templateString;
		stream.close();
	}

	bool ScriptFileSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		asset = Ref<ScriptFileAsset>::Create();
		asset->Handle = metadata.Handle;
		return true;
	}

	bool ScriptFileSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		return true;
	}

	Ref<Asset> ScriptFileSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		SEDX_CORE_VERIFY(false); /// Not implemented
		return nullptr;
	}

	void AssetSerializer::RegisterDependencies(const AssetMetadata& metadata) const
	{
		AssetManager::RegisterDependency(0, metadata.Handle);
	}
}

/// -------------------------------------------------------

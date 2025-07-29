/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* asset_manager.cpp
* -------------------------------------------------------
* Created: 19/3/2025
* -------------------------------------------------------
*/
//#include <nlohmann/json.hpp>
//#include <random>
//#include <SceneryEditorX/asset/asset.h>
#include <SceneryEditorX/asset/asset_manager.h>
//#include <SceneryEditorX/core/identifiers/uuid.h>
//#include <SceneryEditorX/core/time/time.h>
//#include <SceneryEditorX/platform/file_manager.hpp>
//#include <SceneryEditorX/scene/model_asset.h>
//#include <SceneryEditorX/serialization/asset_serializer.h>
//#include <SceneryEditorX/serialization/mesh_serializer.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

    /*
    using Json = nlohmann::json;

    /// Asset Manager
    struct AssetManagerImpl
    {
        UUID32 lastAssetsHash;
        Json lastJson;
        std::filesystem::path currentProjectPath;
        std::filesystem::path currentBinPath;
        std::filesystem::path requestedProjectPath;
        std::filesystem::path requestedBinPath;
        std::filesystem::path requestedAssetPath;
    };

	void AssetManager::LoadProject(const std::filesystem::path& path, const std::filesystem::path& binPath)
    {
	    Time::TimeLog t("AssetManager::LoadProject", true);
	    if (!std::ifstream(path))
		{
	        SEDX_CORE_ERROR("Project file not found: {} {}", path.string(), binPath.string());
	        return;
	    }
        //BinaryStorage storage;
	    //int dir = Serializer::LOAD;
        Json j = Json::parse(IO::FileManager::ReadFile(path.string()));
        //storage.data = IO::FileManager::ReadRawBytes(binPath);
	    std::vector<uint32_t> uuids;

	    /*
	    for (auto& assetJson : j["assets"])
		{
	        Ref<Asset> asset;
            AssetSerializer ser = AssetSerializer(assetJson, storage, dir, *this);
	        ser.Serialize(asset);
	        uuids.push_back(asset->uuid);
	    }
	    #1#

	    /*
	    for (auto& assetJson : j["scenes"])
		{
	        Ref<Asset> asset;
            SceneSerializer ser = SceneSerializer(assetJson, storage, dir, *this);
	        ser.Serialize(asset);
	        uuids.push_back(asset->uuid);
	    }
	    #1#

	    initialScene = j["initialScene"];
        for (auto &scene : GetAll<Scene>(ObjectType::Scene))
		{
	        // scene->UpdateParents(); // TODO: Implement UpdateParents method if needed
	    }

	    impl->lastJson = std::move(j);
        impl->lastAssetsHash = UUID32(Utils::UUID::HashUUID(uuids));
	    impl->currentProjectPath = path;
	    impl->currentBinPath = binPath;
	}

	void AssetManager::SaveProject(const std::filesystem::path& path, const std::filesystem::path& binPath)
    {
	    Time::TimeLog t("AssetManager::SaveProject");
	    //BinaryStorage storage;
	    //int dir = Serializer::SAVE;
	    std::vector<Ref<Asset>> assetsOrdered;
	    std::vector<uint32_t> assetsUUIDs;
	    assetsOrdered.reserve(assets.size());

        for (auto &[uuid, asset] : assets)
        {
            assetsOrdered.push_back(asset);
            assetsUUIDs.push_back(uuid);
        }

        UUID32 assetsHash = UUID32(UUID32(Utils::UUID::HashUUID(assetsUUIDs)));
	    if (assetsHash != impl->lastAssetsHash)
		{
	        SEDX_CORE_INFO("Serializing assets..");
	        Json j;
	        j["scenes"] = Json::object();
            j["assets"] = Json::array();

	        /// Serialize Assets
	        std::ranges::sort(assetsOrdered, [&](const Ref<Asset>& a, const Ref<Asset>& b) {return a->type < b->type; });

	        for (const Ref<Asset> &asset : assetsOrdered)
			{
	            if (asset->type == ObjectType::Scene)
                    continue;

                Json assetJson;
                //SceneAssetSerializer s = SceneAssetSerializer::Serialize(assetJson, storage, dir, *this);
	            //s.Serialize(asset);
	            j["assets"].push_back(assetJson);
	        }

	        // TODO: Implement binary storage serialization
	        // IO::FileDialogs::WriteFileBytes(binPath, storage.data);
	        impl->lastJson = std::move(j);
	    }

	    /*
	    /// always serialize scenes
	    for (auto& scene : GetAll<Scene>(ObjectType::Scene))
		{
            const std::string sceneUuid = std::to_string(static_cast<unsigned long long>(scene->uuid));
	        Json& assetJson = impl->lastJson["scenes"][sceneUuid];
            SceneAssetSerializer s = SceneAssetSerializer::Serialize(assetJson, storage, dir, *this);
	        s.Serialize(scene);
	    }
	    #1#

	    impl->lastJson["initialScene"] = initialScene;
        const std::string jsonDump = impl->lastJson.dump();
        IO::FileDialogs::WriteFile(path, jsonDump);
	    impl->lastAssetsHash = assetsHash;
	}

	Mat4 Node::ComposeTransform(const Vec3 &pos, const Vec3 &rot, const Vec3 &scl, const Mat4 &parent)
	{
		const Mat4 rotationMat = glm::toMat4(Quat(glm::radians(rot)));
		const Mat4 translationMat = glm::translate(Mat4(1.0f), pos);
		const Mat4 scaleMat = glm::scale(scl);
		return parent * (translationMat * rotationMat * scaleMat);
	}

	Mat4 Node::GetLocalTransform() const { return ComposeTransform(position, rotation, scale); }
	Mat4 Node::GetWorldTransform() const { return GetParentTransform() * GetLocalTransform(); }
	Mat4 Node::GetParentTransform() const { return parent ? parent->GetWorldTransform() : Mat4(1); }
	Vec3 Node::GetWorldPosition() const { return GetParentTransform() * Vec4(position, 1); }
	Vec3 Node::GetWorldFront() const { return GetWorldTransform() * Vec4(0, -1, 0, 0); }

    MeshNode::MeshNode() : localTransform() { auto type = ObjectType::Mesh; }
    LightNode::LightNode() { type = ObjectType::Light; }

	/*
	void TextureAsset::Serialize(TextureSerializer &ser)
	{
        ser.vector("data", data);
		ser("width", width);
		ser("height", height);
		ser("channels", channels);
	}
	#1#

	/*
	void ModelAsset::Serialize(MeshSerializer &ser)
	{
		ser.vector("vertices", vertices);
		ser.Vector("indices", indices);
	}
	#1#

	/*
	void MaterialAsset::Serialize(MaterialAssetSerializer &ser)
	{
		ser("color", color);
		ser("emission", emission);
		ser("metallic", metallic);
		ser("roughness", roughness);
		ser.Asset("colorMap", colorMap);
		ser.Asset("aoMap", aoMap);
		ser.Asset("emissionMap", emissionMap);
		ser.Asset("normalMap", normalMap);
		ser.Asset("metallicRoughnessMap", metallicRoughnessMap);
	}
	#1#

	/*
	void Scene::Serialize(Serializer &ser)
	{
		ser.VectorRef("nodes", nodes);
		ser("ambientLight", ambientLight);
		ser("ambientLightColor", ambientLightColor);
		ser("lightSamples", lightSamples);
		ser("aoSamples", aoSamples);
		ser("aoMin", aoMin);
		ser("aoMax", aoMax);
		ser("exposure", exposure);
        //ser("shadowType", rendershadowType);
		//ser("taaEnabled", taaEnabled);
		//ser("taaReconstruct", taaReconstruct);
		ser.Node("mainCamera", mainCamera, this);
	}
	#1#

	/*
	void Node::Serialize(Serializer &ser)
	{
		ser.VectorRef("children", children);
		ser("position", position);
		ser("rotation", rotation);
		ser("scale", scale);
	}
	#1#

	/*
	void MeshNode::Serialize(Serializer &ser)
	{
		Node::Serialize(ser);
		ser.Asset("mesh", mesh);
		ser.Asset("material", material);
	}
	#1#

	/*
	void LightNode::Serialize(Serializer &ser)
	{
		Node::Serialize(ser);
		ser("color", color);
		ser("intensity", intensity);
		ser("lightType", lightType);
		ser("innerAngle", innerAngle);
		ser("outerAngle", outerAngle);
		ser("radius", radius);
		ser("shadowMapRange", shadowMapRange);
		ser("shadowMapFar", shadowMapFar);
		ser("volumetricType", volumetricType);

		ser("volumetricScreenAbsorption", volumetricScreenSpaceParams.absorption);
		ser("volumetricScreenSamples", volumetricScreenSpaceParams.samples);

		ser("volumetricShadowWeight", volumetricShadowMapParams.weight);
		ser("volumetricShadowAbsorption", volumetricShadowMapParams.absorption);
		ser("volumetricShadowDensity", volumetricShadowMapParams.density);
		ser("volumetricShadowSamples", volumetricShadowMapParams.samples);
	}
	#1#

	AssetManager::AssetManager()
	{
		impl = new AssetManagerImpl;
	}

	AssetManager::~AssetManager()
	{
		delete impl;
	}

	Ref<Scene> AssetManager::GetInitialScene()
	{
		if (!initialScene)
            CreateAsset<Scene>("DefaultScene");

        return GetAsset<Scene>(initialScene);
	}

    Ref<CameraNode> AssetManager::GetMainCamera(const Ref<Scene> &scene)
    {
        if (!scene->mainCamera)
        {
            auto cam = CreateObject<CameraNode>("Default Camera");
            scene->Add(cam);
            scene->mainCamera = cam;
            return cam;
        }
        return scene->mainCamera;
    }

	uint32_t AssetManager::NewUUID()
    {
        ///< TODO: replace with something actually UUID
        static std::random_device rd;
        static std::mt19937_64 eng(rd());
        static std::uniform_int_distribution<uint64_t> dist(std::llround(std::pow(2, 61)), std::llround(std::pow(2, 62)));
        return dist(eng);
    }

    std::string AssetManager::GetProjectName() const
    {
        return impl->currentProjectPath.stem().string();
    }
    */

}

/// -------------------------------------------------------

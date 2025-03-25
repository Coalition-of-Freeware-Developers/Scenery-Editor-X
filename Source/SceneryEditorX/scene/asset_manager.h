/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* asset_manager.h
* -------------------------------------------------------
* Created: 19/3/2025
* -------------------------------------------------------
*/

#pragma once

#include <SceneryEditorX/core/base.hpp>
#include <vector>

// -------------------------------------------------------

namespace AssetManager
{

    struct Serializer;
    struct AssetManager;

    enum class ObjectType
    {
        Invalid,
        TextureAsset,
        MeshAsset,
        MaterialAsset,
        SceneAsset,
        Node,
        MeshNode,
        LightNode,
        CameraNode,
        Count,
    };

    inline std::string ObjectTypeName[] = {
        "Invalid",
        "Texture",
        "Mesh",
        "Material",
        "Scene",
        "Node",
        "MeshNode",
        "LightNode",
        "CameraNode",
        "Count",
    };

    inline std::string ShadowTypeNames[] = {"Disabled", "RayTraced", "Map"};

    struct Object
    {
        std::string name = "Unintialized";
        //UUID uuid = 0;
        ObjectType type = ObjectType::Invalid;
        // todo: rethink this gpu dirty flag...
        bool gpuDirty = true;

        Object &operator=(Object &rhs)
        {
            name = rhs.name;
            type = rhs.type;
            gpuDirty = true;
            return *this;
        }

        virtual ~Object();
        virtual void Serialize(Serializer &s) = 0;
    };

    struct Asset : Object
    {
        virtual ~Asset();
        virtual void Serialize(Serializer &s) = 0;
    };

    struct TextureAsset : Asset
    {
        std::vector<uint8_t> data;
        int channels = 0;
        int width = 0;
        int height = 0;

        TextureAsset();
        virtual void Serialize(Serializer &s);
    };

    struct MeshAsset : Asset
    {
        struct MeshVertex
        {
            Vec3 position;
            Vec3 normal;
            Vec4 tangent;
            Vec2 texCoord;
            bool operator==(const MeshVertex &o) const
            {
                return position == o.position && normal == o.normal && texCoord == o.texCoord;
            }
        };
        std::vector<MeshVertex> vertices;
        std::vector<uint32_t> indices;

        MeshAsset();
        virtual void Serialize(Serializer &s);
    };

    struct MaterialAsset : Asset
    {
        Vec4 color = Vec4(1.0f);
        Vec3 emission = Vec3(0.0f);
        float metallic = 0;
        float roughness = 0.5;
        //Ref<TextureAsset> aoMap;
        //Ref<TextureAsset> colorMap;
        //Ref<TextureAsset> normalMap;
        //Ref<TextureAsset> emissionMap;
        //Ref<TextureAsset> metallicRoughnessMap;

        MaterialAsset();
        virtual void Serialize(Serializer &s);
    };

    struct AssetManager
    {
        AssetManager();
        ~AssetManager();
        //std::vector<Ref<Node>> AddAssetsToScene(Ref<SceneAsset> &scene, const std::vector<std::string> &paths);
        //void LoadProject(const std::filesystem::path &path, const std::filesystem::path &binPath);
        //void SaveProject(const std::filesystem::path &path, const std::filesystem::path &binPath);
        //Ref<SceneAsset> GetInitialScene();
        //Ref<CameraNode> GetMainCamera(Ref<SceneAsset> &scene);

        /*
	template <typename T>
	Ref<T> Get(UUID uuid)
	{
		return std::dynamic_pointer_cast<T>(assets[uuid]);
	}

	Ref<Asset> Get(UUID uuid)
	{
		return assets[uuid];
	}

	template <typename T>
	std::vector<Ref<T>> GetAll(ObjectType type) const
	{
		std::vector<Ref<T>> all;
		for (auto &pair : assets)
		{
			if (pair.second->type == type)
			{
				all.emplace_back(std::dynamic_pointer_cast<T>(pair.second));
			}
		}
		return all;
	}

	std::vector<Ref<Asset>> GetAll() const
	{
		std::vector<Ref<Asset>> all;
		for (auto &pair : assets)
		{
			all.emplace_back(std::dynamic_pointer_cast<Asset>(pair.second));
		}
		return all;
	}

	template <typename T>
	static Ref<T> CreateObject(const std::string &name, UUID uuid = 0)
	{
		if (uuid == 0)
		{
			uuid = NewUUID();
		}
		Ref<T> a = std::make_shared<T>();
		a->name = name;
		a->uuid = uuid;
		return a;
	}

	template <typename T>
	Ref<T> CreateAsset(const std::string &name, UUID uuid = 0)
	{
		if (uuid == 0)
		{
			uuid = NewUUID();
		}
		Ref<T> a = std::make_shared<T>();
		a->name = name;
		a->uuid = uuid;
		assets[a->uuid] = a;
		if (a->type == ObjectType::SceneAsset && !initialScene)
		{
			initialScene = a->uuid;
		}
		return a;
	}

	Ref<Object> CreateObject(ObjectType type, const std::string &name, UUID uuid = 0)
	{
		switch (type)
		{
		case ObjectType::TextureAsset:
			return CreateAsset<TextureAsset>(name, uuid);
		case ObjectType::MaterialAsset:
			return CreateAsset<MaterialAsset>(name, uuid);
		case ObjectType::MeshAsset:
			return CreateAsset<MeshAsset>(name, uuid);
		case ObjectType::SceneAsset:
			return CreateAsset<SceneAsset>(name, uuid);
		case ObjectType::Node:
			return CreateObject<Node>(name, uuid);
		case ObjectType::MeshNode:
			return CreateObject<MeshNode>(name, uuid);
		case ObjectType::LightNode:
			return CreateObject<LightNode>(name, uuid);
		case ObjectType::CameraNode:
			return CreateObject<CameraNode>(name, uuid);
		default:
			DEBUG_ASSERT(false, "Invalid object type {}.", type) return nullptr;
		}
	}

	template <typename T>
	Ref<T> CloneAsset(const Ref<Object> &rhs)
	{
		Ref<T> asset = CreateAsset<T>(rhs->name, 0);
		*asset = *std::dynamic_pointer_cast<T>(rhs);
		return asset;
	}

	template <typename T>
	static Ref<T> CloneObject(const Ref<Object> &rhs)
	{
		Ref<T> object = CreateObject<T>(rhs->name, 0);
		*object = *std::dynamic_pointer_cast<T>(rhs);
		return object;
	}

	Ref<Object> CloneAsset(ObjectType type, const Ref<Object> &rhs)
	{
		switch (type)
		{
		case ObjectType::SceneAsset:
			return CloneAsset<SceneAsset>(rhs);
		default:
			DEBUG_ASSERT(false, "Invalid asset type {}.", type) return nullptr;
		}
	}

	static Ref<Object> CloneObject(ObjectType type, const Ref<Object> &rhs)
	{
		switch (type)
		{
		case ObjectType::Node:
			return CloneObject<Node>(rhs);
		case ObjectType::MeshNode:
			return CloneObject<MeshNode>(rhs);
		case ObjectType::LightNode:
			return CloneObject<LightNode>(rhs);
		case ObjectType::CameraNode:
			return CloneObject<CameraNode>(rhs);
		default:
			DEBUG_ASSERT(false, "Invalid object type {}.", type) return nullptr;
		}
	}

	bool HasLoadRequest() const;
	void LoadRequestedProject();
	void RequestLoadProject(const std::filesystem::path &path, const std::filesystem::path &binPath);
*/
        std::string GetProjectName();
        std::filesystem::path GetCurrentProjectPath();
        std::filesystem::path GetCurrentBinPath();

    private:
        struct AssetManagerImpl *impl;
        //std::unordered_map<UUID, Ref<Asset>> assets;
        static UUID NewUUID();
        //UUID initialScene = 0;
    };

    struct SceneAsset : Asset
    {
        //std::vector<Ref<Node>> nodes;
        Vec3 ambientLightColor = Vec3(1);
        float ambientLight = 0.01f;
        int aoSamples = 4;
        int lightSamples = 2;
        float aoMin = 0.0001f;
        float aoMax = 1.0000f;
        float exposure = 2.0f;
        //ShadowType shadowType = ShadowType::ShadowRayTraced;
        uint32_t shadowResolution = 1024;

        float camSpeed = 0.01f;
        float zoomSpeed = 1.0f;
        float rotationSpeed = 0.3f;
        bool autoOrbit = false;
        //Ref<CameraNode> mainCamera;

        bool taaEnabled = true;
        bool taaReconstruct = true;

        /*
	template <typename T>
	Ref<T> Add()
	{
		Ref<T> node = std::make_shared<T>();
		nodes.push_back(node);
		return node;
	}

	void Add(const Ref<Node> &node)
	{
		nodes.push_back(node);
	}

	void DeleteRecursive(const Ref<Node> &node);
	*/
    };
} // namespace AssetManager

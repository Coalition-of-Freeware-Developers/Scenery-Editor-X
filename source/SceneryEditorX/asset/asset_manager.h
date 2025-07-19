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
#include <SceneryEditorX/asset/asset.h>
#include <SceneryEditorX/renderer/vulkan/vk_data.h>
#include <SceneryEditorX/scene/camera.h>
#include <SceneryEditorX/scene/lights.h>
#include <SceneryEditorX/scene/material.h>
#include <SceneryEditorX/scene/model_asset.h>
#include <SceneryEditorX/scene/node.h>
#include <SceneryEditorX/scene/scene.h>
#include <SceneryEditorX/scene/texture.h>
#include <vector>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	struct Serializer;

	/// -------------------------------------------------------
		
	inline std::string ShadowTypeNames[] = {"Disabled", "RayTraced", "Map"};

	/// -------------------------------------------------------

	class AssetManager
    {
	public:
        AssetManager();
        ~AssetManager();

		/// -------------------------------------------------------

		std::vector<Ref<Node>> AddAssetsToScene(Ref<Scene> &scene, const std::vector<std::string> &paths);
        void LoadProject(const std::filesystem::path& path, const std::filesystem::path& binPath);
        void SaveProject(const std::filesystem::path& path, const std::filesystem::path &binPath);

		/// -------------------------------------------------------

		Ref<Scene> GetInitialScene();
        static Ref<CameraNode> GetMainCamera(const Ref<Scene> &scene);

		/// -------------------------------------------------------

	    bool HasLoadRequest() const;
        void LoadRequestedProject();
        void RequestLoadProject(const std::filesystem::path &path, const std::filesystem::path &binPath);
        std::string GetProjectName() const;
        std::filesystem::path GetCurrentProjectPath();
        std::filesystem::path GetCurrentBinPath();

		/// -------------------------------------------------------

		template <typename T>
        Ref<T> Get(uint32_t uuid)
        {
            return assets[uuid].DynamicCast<T>();
        }

		/// -------------------------------------------------------

        Ref<Asset> Get(uint32_t uuid)
        {
            return assets[uuid];
        }

		/// -------------------------------------------------------

        template <typename T>
        std::vector<Ref<T>> GetAll(ObjectType type) const
        {
            std::vector<Ref<T>> all;
            for (const auto &asset : assets | std::views::values)
            {
                if (asset->type == type)
                {
                    all.emplace_back(asset.DynamicCast<T>());
                }
            }
            return all;
        }

		/// -------------------------------------------------------

		template <typename T>
        static Ref<T> CreateObject(const std::string &name, uint32_t uuid = 0)
        {
            if (uuid == 0)
            {
                uuid = NewUUID();
            }
            Ref<T> a = CreateRef<T>();
            a->name = name;
            a->uuid = uuid;
            return a;
        }

		/// -------------------------------------------------------

        template <typename T>
        Ref<T> CreateAsset(const std::string &name, uint32_t uuid = 0)
        {
            if (uuid == 0)
            {
                uuid = NewUUID();
            }
            Ref<T> a = CreateRef<T>();
            a->name = name;
            a->uuid = uuid;
            assets[a->uuid] = a;
            if (a->type == ObjectType::Scene && !initialScene)
            {
                initialScene = a->uuid;
            }
            return a;
        }

		/// -------------------------------------------------------

        Ref<Object> CreateObject(ObjectType type, const std::string &name, const uint32_t uuid = 0)
        {
            switch (type)
            {
                case ObjectType::Texture: return CreateAsset<TextureAsset>(name, uuid);
                case ObjectType::Material: return CreateAsset<MaterialAsset>(name, uuid);
                case ObjectType::Mesh: return CreateAsset<ModelAsset>(name, uuid);
                case ObjectType::Scene: return CreateAsset<Scene>(name, uuid);
                case ObjectType::Node: return CreateObject<Node>(name, uuid);
                case ObjectType::Light: return CreateObject<LightNode>(name, uuid);
                default:
                    return nullptr;
            }
        }

		/// -------------------------------------------------------

        template <typename T>
        static Ref<Object> CloneObject(const Ref<Object> &rhs)
        {
            Ref<T> object = CreateObject<T>(rhs->name, 0);
            *object = *rhs.DynamicCast<T>();
            return object;
        }

		/// -------------------------------------------------------

        template <typename T>
        Ref<T> CloneAsset(const Ref<Object> &rhs)
        {
            Ref<T> asset = CreateAsset<T>(rhs->name, 0);
            *asset = *rhs.DynamicCast<T>();
            return asset;
        }

		/// -------------------------------------------------------

        Ref<Object> CloneAsset(ObjectType type, const Ref<Object> &rhs)
        {
            switch (type)
            {
            case ObjectType::Scene:
                return CloneAsset<Scene>(rhs);
            default:
                return nullptr;
            }
        }

		/// -------------------------------------------------------

        static Ref<Object> CloneObject(ObjectType type, const Ref<Object> &rhs)
        {
            switch (type)
            {
                case ObjectType::Node: return CloneObject<Node>(rhs);
                case ObjectType::Mesh: return CloneObject<MeshNode>(rhs);
                case ObjectType::Light: return CloneObject<LightNode>(rhs);
                case ObjectType::Camera: return CloneObject<CameraNode>(rhs);
                default:
                    return nullptr;
            }
        }

	    /// -------------------------------------------------------

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

        /// -------------------------------------------------------

	    struct AssetManagerImpl *impl;
        std::unordered_map<uint32_t, Ref<Asset>> assets;

    private:
        RenderData renderData;
        static uint32_t NewUUID();
        uint32_t initialScene = 0;
        std::vector<Ref<Node>> nodes;


        friend class GraphicsEngine;
    };

}

/// -------------------------------------------------------

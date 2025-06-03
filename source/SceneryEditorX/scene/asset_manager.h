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
#include <GraphicsEngine/vulkan/render_data.h>
#include <SceneryEditorX/scene/asset.h>
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

		std::vector<Ref<Node>> AddAssetsToScene(Ref<SceneAsset> &scene, const std::vector<std::string> &paths);
        void LoadProject(const std::filesystem::path& path, const std::filesystem::path& binPath);
        void SaveProject(const std::filesystem::path& path, const std::filesystem::path &binPath);

		/// -------------------------------------------------------

		Ref<SceneAsset> GetInitialScene();
        Ref<CameraNode> GetMainCamera(Ref<SceneAsset> &scene);

		/// -------------------------------------------------------

	    bool HasLoadRequest() const;
        void LoadRequestedProject();
        void RequestLoadProject(const std::filesystem::path &path, const std::filesystem::path &binPath);
        std::string GetProjectName();
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
            if (a->type == ObjectType::SceneAsset && !initialScene)
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
                case ObjectType::TextureAsset: return CreateAsset<TextureAsset>(name, uuid);
                case ObjectType::MaterialAsset: return CreateAsset<MaterialAsset>(name, uuid);
                case ObjectType::MeshAsset: return CreateAsset<ModelAsset>(name, uuid);
                case ObjectType::SceneAsset: return CreateAsset<SceneAsset>(name, uuid);
                case ObjectType::Node: return CreateObject<Node>(name, uuid);
                case ObjectType::MeshNode: return CreateObject<MeshNode>(name, uuid);
                case ObjectType::LightNode: return CreateObject<LightNode>(name, uuid);
                case ObjectType::CameraNode: return CreateObject<CameraNode>(name, uuid);
                default:
#ifndef NDEBUG
                    if (static_cast<int>(type) < 0 || static_cast<int>(type) >= static_cast<int>(ObjectType::Count))
                        assert(false);
#endif
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
            case ObjectType::SceneAsset:
                return CloneAsset<SceneAsset>(rhs);
            default:
#ifndef NDEBUG
                if (static_cast<int>(type) < 0 || static_cast<int>(type) >= static_cast<int>(ObjectType::Count))
                    assert(false);
#endif
                return nullptr;
            }
        }

		/// -------------------------------------------------------

        static Ref<Object> CloneObject(ObjectType type, const Ref<Object> &rhs)
        {
            switch (type)
            {
                case ObjectType::Node: return CloneObject<Node>(rhs);
                case ObjectType::MeshNode: return CloneObject<MeshNode>(rhs);
                case ObjectType::LightNode: return CloneObject<LightNode>(rhs);
                case ObjectType::CameraNode: return CloneObject<CameraNode>(rhs);
                default:
#ifndef NDEBUG
                    if (static_cast<int>(type) < 0 || static_cast<int>(type) >= static_cast<int>(ObjectType::Count))
                        assert(false);
#endif
                    return nullptr;
            }
        }
    

    private:
        RenderData renderData;

        struct AssetManagerImpl *impl;
        std::unordered_map<uint32_t, Ref<Asset>> assets;
        static uint32_t NewUUID();
        uint32_t initialScene = 0;

        friend class GraphicsEngine;
    };

}

/// -------------------------------------------------------

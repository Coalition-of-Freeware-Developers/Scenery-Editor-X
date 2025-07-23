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

#include <SceneryEditorX/project/project.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

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
        GLOBAL Ref<CameraNode> GetMainCamera(const Ref<Scene> &scene);

        /**
         * @brief Checks if the asset referred to by assetHandle could potentially be valid.
         *
         * This function checks if the asset handle is valid, but does not guarantee that the asset is loaded or exists on disk.
         * An asset handle is considered potentially valid if:
         * - The asset handle is not empty
         * - The asset type is known and registered in the asset manager
         * - The asset metadata exists in the asset registry
         * This function does not check if the asset file exists on disk or if the asset can be loaded successfully.
         *
         * @param assetHandle The asset handle to check.
         * @returns True if assetHandle could potentially be valid.
         */
        GLOBAL bool IsAssetHandleValid(const AssetHandle &assetHandle)
        {
            return Project::GetAssetManager()->IsAssetHandleValid(assetHandle);
        }

        /**
         * @brief Checks if the asset referred to by assetHandle is valid.
         *
         * This function checks if the asset handle is valid and if the asset can be loaded successfully.
         * An asset is invalid if any of the following are true:
         * - The asset handle is invalid
         * - The file referred to by asset metadata is missing
         * - The asset could not be loaded from file
         *
         * @note This will attempt to load the asset if it is not already loaded.
         *
         * @param assetHandle The asset handle to check.
         * @returns True if the asset referred to by assetHandle is valid.
         */
        GLOBAL bool IsAssetValid(const AssetHandle &assetHandle) { return Project::GetAssetManager()->IsAssetValid(assetHandle); }

	    GLOBAL bool IsMemoryAsset(AssetHandle handle) { return Project::GetAssetManager()->IsMemoryAsset(handle); }
        GLOBAL bool IsPhysicalAsset(AssetHandle handle) { return Project::GetAssetManager()->IsPhysicalAsset(handle); }
        GLOBAL bool ReloadData(AssetHandle assetHandle) { return Project::GetAssetManager()->ReloadData(assetHandle); }
        GLOBAL bool EnsureCurrent(AssetHandle assetHandle) { return Project::GetAssetManager()->EnsureCurrent(assetHandle); }
        GLOBAL bool EnsureAllLoadedCurrent() { return Project::GetAssetManager()->EnsureAllLoadedCurrent(); }
        GLOBAL AssetType GetAssetType(AssetHandle assetHandle) { return Project::GetAssetManager()->GetAssetType(assetHandle); }


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

	    template<typename T>
        GLOBAL std::unordered_set<AssetHandle> GetAllAssetsWithType()
		{
			return Project::GetAssetManager()->GetAllAssetsWithType(T::GetStaticType());
		}

		static const std::unordered_map<AssetHandle, Ref<Asset>>& GetLoadedAssets() { return Project::GetAssetManager()->GetLoadedAssets(); }

		/**
		 * @note The memory-only asset must be fully initialised before you AddMemoryOnlyAsset()
		 * Assets are not themselves thread-safe, but can potentially be accessed from multiple threads.
		 * Thread safety therefore depends on the assets being immutable once they've been added to the asset manager.
         */
		template<typename TAsset>
        GLOBAL AssetHandle AddMemoryOnlyAsset(Ref<TAsset> asset)
		{
			static_assert(std::is_base_of_v<Asset, TAsset>, "AddMemoryOnlyAsset only works for types derived from Asset");
			if (!asset->Handle)
                asset->Handle = AssetHandle(); ///< @note: should handle generation happen here?

		    Project::GetAssetManager()->AddMemoryOnlyAsset(asset);
			return asset->Handle;
		}


        Ref<Asset> Get(uint32_t uuid) { return assets[uuid]; }
		
		GLOBAL Ref<Asset> GetMemoryAsset(const AssetHandle &handle)
		{
		    return Project::GetAssetManager()->GetMemoryAsset(handle);
		}

		///< Handle is dependent on dependency.  e.g. handle could be a material, and dependency could be a texture that the material uses.
		GLOBAL void RegisterDependency(const AssetHandle &dependency, const AssetHandle &handle)
		{
		    return Project::GetAssetManager()->RegisterDependency(dependency, handle);
		}

		///< Remove dependency of handle on dependency
		GLOBAL void DeregisterDependency(const AssetHandle &dependency, const AssetHandle &handle)
		{
		    return Project::GetAssetManager()->DeregisterDependency(dependency, handle);
		}

		/** Remove all dependencies of handle */
		GLOBAL void DeregisterDependencies(const AssetHandle &handle)
		{
		    return Project::GetAssetManager()->DeregisterDependencies(handle);
		}

		GLOBAL void RemoveAsset(const AssetHandle &handle)
		{
		    Project::GetAssetManager()->RemoveAsset(handle);
		}

		/// -------------------------------------------------------

        template <typename T>
        std::vector<Ref<T>> GetAll(ObjectType type) const
        {
            std::vector<Ref<T>> all;
            for (const auto &asset : assets | std::views::values)
            {
                if (asset->type == type)
                    all.emplace_back(asset.DynamicCast<T>());
            }
            return all;
        }

		/// -------------------------------------------------------

		template <typename T>
        GLOBAL Ref<T> CreateObject(const std::string &name, uint32_t uuid = 0)
        {
            if (uuid == 0)
                uuid = NewUUID();

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
                uuid = NewUUID();

            Ref<T> a = CreateRef<T>();
            a->name = name;
            a->uuid = uuid;
            assets[a->uuid] = a;
            if (a->type == ObjectType::Scene && !initialScene)
                initialScene = a->uuid;

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
        GLOBAL Ref<Object> CloneObject(const Ref<Object> &rhs)
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

        /*
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
        */

		/// -------------------------------------------------------

        GLOBAL Ref<Object> CloneObject(ObjectType type, const Ref<Object> &rhs)
        {
            switch (type)
            {
                case ObjectType::Node: return CloneObject<Node>(rhs);
                case ObjectType::Mesh: return CloneObject<MeshNode>(rhs);
                case ObjectType::Light: return CloneObject<LightNode>(rhs);
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

		
        /// -------------------------------------------------------

    private:
        RenderData renderData;
        LOCAL uint32_t NewUUID();
        uint32_t initialScene = 0;
        std::vector<Ref<Node>> nodes;

        friend class GraphicsEngine;
    };

}

/// -------------------------------------------------------

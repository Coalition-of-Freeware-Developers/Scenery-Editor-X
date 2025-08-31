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
#include "SceneryEditorX/asset/asset.h"
#include "SceneryEditorX/project/project.h"
#include "SceneryEditorX/renderer/vulkan/vk_data.h"
#include "SceneryEditorX/scene/camera.h"
#include "SceneryEditorX/scene/lights.h"
#include "SceneryEditorX/scene/material.h"
#include "SceneryEditorX/scene/model_asset.h"
#include "SceneryEditorX/scene/node.h"
#include "SceneryEditorX/scene/scene.h"
#include "SceneryEditorX/scene/texture.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{

	/*
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
        //static Ref<CameraNode> GetMainCamera(const Ref<Scene> &scene);

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
         #1#
        static bool IsAssetHandleValid(const AssetHandle &assetHandle)
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
         * @note - This will attempt to load the asset if it is not already loaded.
         *
         * @param assetHandle The asset handle to check.
         * @returns True if the asset referred to by assetHandle is valid.
         #1#
        static bool IsAssetValid(const AssetHandle &assetHandle)
        {
            return Project::GetAssetManager()->IsAssetValid(assetHandle);
        }

        /**
         * @brief Checks if the asset referred to by handle is a memory asset or a physical asset.
         *
         * These assets are loaded into memory and do not have a physical file on disk.
         *
         * @param handle The asset handle to check.
         * @return True if the asset is a memory asset, false if it is a physical asset.
         #1#
        static bool IsMemoryAsset(const AssetHandle &handle)
        {
            return Project::GetAssetManager()->IsMemoryAsset(handle);
        }

		/**
		 * @brief Checks if the asset referred to by handle is a physical asset.
		 *
		 * Physical assets are those that have a corresponding file on disk.
		 *
		 * @param handle The asset handle to check.
		 * @return True if the asset is a physical asset, false if it is a memory asset.
		 #1#
        static bool IsPhysicalAsset(const AssetHandle &handle)
        {
            return Project::GetAssetManager()->IsPhysicalAsset(handle);
        }

        /**
         * @brief Reloads the asset data for the asset referred to by assetHandle.
         *
         * This function attempts to reload the asset data from disk or memory.
         * If the asset is a memory asset, it will reload the data from the memory representation.
         *
         * @param assetHandle The asset handle to reload.
         * @return True if the asset data was successfully reloaded, false otherwise.
         #1#
        static bool ReloadData(const AssetHandle &assetHandle)
        {
            return Project::GetAssetManager()->ReloadData(assetHandle);
        }

        /**
         * @brief Ensures that the asset referred to by assetHandle is current.
         *
         * This function checks if the asset is loaded and up-to-date.
         * If the asset is not loaded, it will attempt to load it.
         *
         * @param assetHandle The asset handle to ensure current.
         * @return True if the asset is current, false otherwise.
         #1#
        static bool EnsureCurrent(const AssetHandle &assetHandle)
        {
            return Project::GetAssetManager()->EnsureCurrent(assetHandle);
        }

        /**
         * @brief Ensures that all assets in the asset manager are loaded and current.
         *
         * This function checks all assets in the asset manager and attempts to load them if they are not already loaded.
         * It will also ensure that all assets are up-to-date with their metadata.
         *
         * @return True if all assets are loaded and current, false if any asset failed to load or is not current.
         #1#
        static bool EnsureAllLoadedCurrent()
        {
            return Project::GetAssetManager()->EnsureAllLoadedCurrent();
        }

        /**
         * @brief Gets the type of the asset referred to by assetHandle.
         *
         * This function retrieves the type of the asset based on its handle.
         * It checks the asset registry and returns the type of the asset.
         *
         * @param assetHandle The asset handle to check.
         * @return The type of the asset referred to by assetHandle.
         #1#
        static AssetType GetAssetType(AssetHandle &assetHandle)
        {
            return Project::GetAssetManager()->GetAssetType(assetHandle);
        }

		/// -------------------------------------------------------

	    bool HasLoadRequest() const;
        void LoadRequestedProject();
        void RequestLoadProject(const std::filesystem::path &path, const std::filesystem::path &binPath);
        std::string GetProjectName() const;
        std::filesystem::path GetCurrentProjectPath();
        std::filesystem::path GetCurrentBinPath();

		/// -------------------------------------------------------

		/**
		 * @brief Gets the asset with the specified UUID.
		 *
		 * This function retrieves the asset from the asset manager using its UUID.
		 * It returns a reference to the asset if it exists, or nullptr if it does not.
		 *
		 * @param uuid The UUID of the asset to retrieve.
		 * @return A reference to the asset with the specified UUID, or nullptr if it does not exist.
		 #1#
		template <typename T>
        Ref<T> GetAsset(uint32_t uuid)
        {
            return assets[uuid].DynamicCast<T>();
        }

		/// -------------------------------------------------------

		/**
		 * @brief Gets all assets of a specific type.
		 *
		 * This function retrieves all assets of the specified type from the asset manager.
		 * It uses the static type of the asset to filter the assets.
		 *
		 * @param T The type of the asset to retrieve.
		 * @return A set of asset handles for all assets of the specified type.
		 #1#
	    template<typename T>
        static std::unordered_set<AssetHandle> GetAllAssetsWithType()
		{
            return Project::GetAssetManager()->GetAllAssetsWithType(T::GetStaticType());
		}

        /**
         * @brief Gets all loaded assets in the asset manager.
         *
         * This function returns a map of all loaded assets, where the key is the asset handle
         * and the value is a reference to the asset object.
         *
         * @return A constant reference to the map of loaded assets.
         #1#
        static const std::unordered_map<AssetHandle, Ref<Asset>>& GetLoadedAssets()
		{
		    return Project::GetAssetManager()->GetLoadedAssets();
		}

		/**
		 * @note - The memory-only asset must be fully initialised before you AddMemoryOnlyAsset()
		 * Assets are not themselves thread-safe, but can potentially be accessed from multiple threads.
		 * Thread safety therefore depends on the assets being immutable once they've been added to the asset manager.
         #1#
		template<typename TAsset>
        static AssetHandle AddMemoryOnlyAsset(Ref<TAsset> asset)
		{
			static_assert(std::is_base_of_v<Asset, TAsset>, "AddMemoryOnlyAsset only works for types derived from Asset");
			if (!asset->Handle)
                asset->Handle = AssetHandle(); ///< @note: should handle generation happen here?

		    Project::GetAssetManager()->AddMemoryOnlyAsset(asset);
			return asset->Handle;
		}


        Ref<Asset> GetAsset(uint32_t uuid) { return assets[uuid]; }

		/// Static template method for getting typed assets by handle
		template<typename T>
		static Ref<T> GetAsset(const AssetHandle &handle)
		{
		    auto asset = Project::GetAssetManager()->GetAsset(handle);
		    return asset ? asset.DynamicCast<T>() : nullptr;
		}

		static Ref<Asset> GetMemoryAsset(const AssetHandle &handle)
		{
		    return Project::GetAssetManager()->GetMemoryAsset(handle);
		}

		///< Handle is dependent on dependency.  e.g. handle could be a material, and dependency could be a texture that the material uses.
		static void RegisterDependency(const AssetHandle &dependency, const AssetHandle &handle)
		{
		    return Project::GetAssetManager()->RegisterDependency(dependency, handle);
		}

		///< Remove dependency of handle on dependency
		static void DeregisterDependency(const AssetHandle &dependency, const AssetHandle &handle)
		{
		    return Project::GetAssetManager()->DeregisterDependency(dependency, handle);
		}

		/** Remove all dependencies of handle #1#
		static void DeregisterDependencies(const AssetHandle &handle)
		{
		    return Project::GetAssetManager()->DeregisterDependencies(handle);
		}

		static void RemoveAsset(const AssetHandle &handle)
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
        static Ref<T> CreateObject(const std::string &name, uint32_t uuid = 0)
        {
            if (uuid == 0)
                uuid = NewUUID();

            Ref<T> a = CreateRef<T>();
            a->name = name;
            a->uuid = UUID(uuid);
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
            a->uuid = UUID(uuid);
            assets[a->uuid] = a;
            if (a->type == ObjectType::Scene && !initialScene)
                initialScene = static_cast<uint32_t>(a->uuid);

            return a;
        }

		/// -------------------------------------------------------

        Ref<Object> CreateObject(ObjectType type, const std::string &name, const uint32_t uuid = 0)
        {
            switch (type)
            {
                case ObjectType::Texture:	return CreateAsset<TextureAsset>(name, uuid);
                case ObjectType::Material:	return CreateAsset<MaterialAsset>(name, uuid);
                case ObjectType::Mesh:		return CreateAsset<ModelAsset>(name, uuid);
                case ObjectType::Scene:		return CreateAsset<Scene>(name, uuid);
                case ObjectType::Node:		return CreateObject<Node>(name, uuid);
                case ObjectType::Light:		return CreateObject<LightNode>(name, uuid);
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

        /// -------------------------------------------------------

    private:
        RenderData renderData;
        static uint32_t NewUUID();
        uint32_t initialScene = 0;
        std::vector<Ref<Node>> nodes;
        std::unordered_map<uint32_t, Ref<Asset>> assets;

        friend class GraphicsEngine;
    };
    */


}

/// -------------------------------------------------------

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
 * asset_manager.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "asset.h"
#include "asset_metadata.h"
#include "asset_registry.h"
#include "model.h"
#include <memory>
#include <shared_mutex>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Project;

	class AssetManager : public RefCounted
	{
	public:
		AssetManager() = default;
		virtual ~AssetManager() = default;

#pragma region Asset Add & Remove

		/**
		 * @brief Imports an asset from the specified file path.
		 * @param filepath The path to the asset file.
		 * @return The handle of the imported asset.
		 */
		AssetHandle ImportAsset(const std::filesystem::path& filepath);
		
	    /**
		 * @brief Adds a memory-only asset.
		 * @param asset The memory-only asset to add.
		 */
		void AddMemoryOnlyAsset(Ref<Asset> asset);

		/**
		 * @brief Adds an asset by loading a model and its textures.
		 * @param allocator The Vulkan memory allocator.
		 * @param cmdPool The Vulkan command pool.
		 * @param queue The Vulkan queue.
		 * @param modelFile The path to the model file.
		 * @param textureFiles The paths to the texture files.
		 * @param modelAllocInfo The allocation info for the model.
		 * @return True if the asset was successfully added, false otherwise.
		 */
		static bool AddAsset(VmaAllocator allocator, VkCommandPool cmdPool, VkQueue queue, const std::string& modelFile, const std::vector<std::string>& textureFiles, const VmaAllocationCreateInfo& modelAllocInfo);
	    
	    /**
		 * @brief Checks if the given asset handle refers to a loaded asset.
		 * @param handle The handle of the asset to check.
		 * @return True if the asset referred to by handle is loaded, false otherwise.
		 */
		bool IsAssetLoaded(AssetHandle handle);

		/**
		 * @brief Removes the asset associated with the given handle.
		 * @param handle The handle of the asset to remove.
		 */
		void RemoveAsset(AssetHandle handle);
		
		/**
		 * @brief Destroys all assets.
		 */
		void DestroyAll();

#pragma endregion
#pragma region Asset Getters
		/**
		 * @brief Gets the type of the asset associated with the given handle.
		 * @param assetHandle The handle of the asset.
		 * @return The type of the asset.
		 */
		AssetType GetAssetType(AssetHandle assetHandle);

		/**
		 * @brief Gets the asset registry.
		 * @return A reference to the asset registry.
		 */
		const AssetRegistry& GetAssetRegistry() const { return m_AssetRegistry; }

		/**
		 * @brief Gets the handle of the asset associated with the given file path.
		 * @param filepath The path to the asset file.
		 * @return The handle of the asset.
		 */
		AssetHandle GetAssetHandleFromFilePath(const std::filesystem::path& filepath);

		/**
		 * @brief Gets the metadata of the asset associated with the given handle.
		 * @param handle The handle of the asset.
		 * @return The metadata of the asset.
		 */
		AssetMetadata GetMetadata(AssetHandle handle);

	    /**
		 * @brief Gets the type of the asset associated with the given file extension.
		 * @param extension The file extension of the asset.
		 * @return The type of the asset.
		 */
		static AssetType GetAssetTypeFromExtension(const std::string& extension);
		
		/**
		 * @brief Gets the default file extension for the given asset type.
		 * @param type The type of the asset.
		 * @return The default file extension for the asset type.
		 */
		static std::string GetDefaultExtensionForAssetType(AssetType type);

		/**
		 * @brief Gets the type of the asset associated with the given file path.
		 * @param path The path to the asset file.
		 * @return The type of the asset.
		 */
		static AssetType GetAssetTypeFromPath(const std::filesystem::path& path);
	    
	    /**
		 * @brief Gets the asset associated with the given handle.
		 * @param assetHandle The handle of the asset.
		 * @return A reference to the asset.
		 */
		Ref<Asset> GetAsset(AssetHandle assetHandle);
				
	    /**
		 * @brief Gets the memory-only asset associated with the given handle.
		 * @param handle The handle of the asset.
		 * @return A reference to the memory-only asset.
		 */
		static Ref<Asset> GetMemoryAsset(AssetHandle handle);

		/**
		 * @brief Gets the relative path for the given file path.
		 * @param filepath The path to the file.
		 * @return The relative path of the file.
		 */
		static std::filesystem::path GetRelativePath(const std::filesystem::path &filepath);
		
		/**
		 * @brief Gets the asset at the specified index.
		 * @param index The index of the asset to retrieve.
		 * @return A reference to the asset at the specified index.
		 */
		[[nodiscard]] const Asset& GetAsset(size_t index) const { return *m_AssetSets.at(index); }

		/**
		 * @brief Gets the model asset at the specified index.
		 * @param index The index of the model asset to retrieve.
		 * @return A reference to the model asset at the specified index.
		 */
		const Asset &GetAsset(size_t index) { return *m_AssetSets.at(index); }
		
		/**
		 * @brief Retrieves a placeholder asset for the specified asset type.
		 * @param type The type of asset for which to retrieve a placeholder.
		 * @return A reference to the placeholder asset.
		 */
		static Ref<Asset> GetPlaceholderAsset(AssetType type);

		/**
		 * @brief Retrieves all loaded assets.
		 * @return A constant reference to the unordered map of loaded assets.
		 */
		static const std::unordered_map<AssetHandle, Ref<Asset>>& GetLoadedAssets();
		
		/**
		 * @brief Retrieves all assets of the specified type.
		 * @param type The type of assets to retrieve.
		 * @return A set of asset handles representing the assets of the specified type.
		 */
		std::unordered_set<AssetHandle> GetAllAssetsWithType(AssetType type);

#pragma endregion
#pragma region Asset Setters

		/**
		 * @brief Sets the metadata of the asset associated with the given handle.
		 * @param handle The handle of the asset.
		 * @param metadata The metadata to set for the asset.
		 */
		void SetMetadata(AssetHandle handle, const AssetMetadata& metadata);

#pragma endregion

		/**
		 * @brief Gets the file system path for the given asset metadata.
		 * @param metadata The metadata of the asset.
		 * @return The file system path of the asset.
		 */
		static std::filesystem::path GetFileSystemPath(const AssetMetadata &metadata);

		/**
		 * @brief Gets the file system path for the asset associated with the given handle.
		 * @param handle The handle of the asset.
		 * @return The file system path of the asset.
		 */
		std::filesystem::path GetFileSystemPath(AssetHandle handle);

		/**
		 * @brief Gets the file system path as a string for the given asset metadata.
		 * @param metadata The metadata of the asset.
		 * @return The file system path as a string.
		 */
		static std::string GetFileSystemPathString(const AssetMetadata &metadata);

		/**
		 * @brief Reloads the data for the asset associated with the given handle.
		 * @param assetHandle The handle of the asset to reload.
		 * @return True if the data was successfully reloaded, false otherwise.
		 */
		bool ReloadData(AssetHandle assetHandle);

		/**
		 * @brief Reloads the data for the asset associated with the given handle asynchronously.
		 * @param assetHandle The handle of the asset to reload.
		 */
		void ReloadDataAsync(AssetHandle assetHandle);

		/**
		 * @brief Ensures that the asset associated with the given handle is current.
		 * @param assetHandle The handle of the asset to check.
		 * @return True if the asset is current, false otherwise.
		 */
		bool EnsureCurrent(AssetHandle assetHandle);

		/**
		 * @brief Ensures that all loaded assets are current.
		 * @return True if all loaded assets are current, false otherwise.
		 */
		bool EnsureAllLoadedCurrent();

		/**
		 * @brief Gets the number of assets.
		 * @return The number of assets.
		 */
		[[nodiscard]] size_t Count() const { return m_AssetSets.size(); }

		/**
		 * @brief Checks if the given asset handle is valid.
		 * @param assetHandle The handle of the asset to check.
		 * @return True if the asset handle is valid, false otherwise.
		 */
		static bool IsAssetHandleValid(AssetHandle assetHandle);

		/**
		 * @brief Checks if the given asset handle refers to a valid asset.
		 * @param assetHandle The handle of the asset to check.
		 * @return True if the asset referred to by assetHandle is valid.
		 * @note An asset is invalid if any of the following are true:
		 * - The asset handle is invalid
		 * - The file referred to by asset metadata is missing
		 * - The asset could not be loaded from file
		 */
		static bool IsAssetValid(AssetHandle assetHandle);

		/**
		 * @brief Checks if the given asset handle is missing asset.
		 * @param assetHandle The handle of the asset to check.
		 * @return True if the asset referred to by assetHandle is missing.
		 * @note A memory-only asset cannot be missing.
		 */
		static bool IsAssetMissing(AssetHandle assetHandle);

		/**
		 * @brief Checks if the file associated with the given asset metadata exists.
		 * @param metadata The metadata of the asset to check.
		 * @return True if the file exists, false otherwise.
		 */
		static bool FileExists(AssetMetadata& metadata);

		/**
		 * @brief Checks if the given asset handle refers to a memory-only asset.
		 * @param handle The handle of the asset to check.
		 * @return True if the asset referred to by handle is a memory-only asset.
		 */
		static bool IsMemoryAsset(AssetHandle handle);

		/**
		 * @brief Checks if the given asset handle refers to a physical asset.
		 * @param handle The handle of the asset to check.
		 * @return True if the asset referred to by handle is a physical asset.
		 */
		static bool IsPhysicalAsset(AssetHandle handle);

		/*
		template<typename T>
		static Ref<T> GetAsset(AssetHandle assetHandle)
		{
			//static std::mutex mutex;
			//std::scoped_lock<std::mutex> lock(mutex);

			Ref<Asset> asset = Project::GetAssetManager()->GetAsset(assetHandle);
			return asset.As<T>();
		}
		*/

		/*
		template<typename T>
		static AsyncAssetResult<T> GetAssetAsync(AssetHandle assetHandle)
		{
		#if ASYNC_ASSETS
			AsyncAssetResult<Asset> result = Project::GetAssetManager()->GetAssetAsync(assetHandle);
			return AsyncAssetResult<T>(result);
		#else
			return { GetAsset<T>(assetHandle), true };
		#endif
		}
		*/

		/**
		 * @brief Registers a dependency between two assets.
		 * @param dependency The handle of the asset that is depended upon.
		 * @param handle The handle of the asset that depends on the dependency.
		 */
		static void RegisterDependency(AssetHandle dependency, AssetHandle handle);

		/**
		 * @brief Deregisters a dependency between two assets.
		 * @param dependency The handle of the asset that is depended upon.
		 * @param handle The handle of the asset that depends on the dependency.
		 */
		static void DeregisterDependency(AssetHandle dependency, AssetHandle handle);

		/**
		 * @brief Deregisters all dependencies of the specified asset.
		 * @param handle The handle of the asset whose dependencies are to be deregistered.
		 */
		static void DeregisterDependencies(AssetHandle handle);

		/**
		 * @brief Retrieves the dependencies of the specified asset.
		 * @param handle The handle of the asset whose dependencies are to be retrieved.
		 * @return A set of asset handles representing the dependencies of the specified asset.
		 */
		std::unordered_set<AssetHandle> GetDependencies(AssetHandle handle);

		/**
		 * @brief Replaces a loaded asset with a new asset.
		 * @param handle The handle of the asset to be replaced.
		 * @param newAsset The new asset to replace the old asset with.
		 */
		void ReplaceLoadedAsset(AssetHandle handle, Ref<Asset> newAsset) { m_LoadedAssets[handle] = newAsset; }

	private:
		/**
		 * @brief Retrieves an asset, including invalid ones.
		 * @param assetHandle The handle of the asset to retrieve.
		 * @return A reference to the asset, or a placeholder if the asset is invalid.
		 */
		Ref<Asset> GetAssetIncludingInvalid(AssetHandle assetHandle);

		/**
		 * @brief Loads the asset registry.
		 */
		static void LoadAssetRegistry();

		/**
		 * @brief Processes the assets in the specified directory.
		 * @param directoryPath The path of the directory to process.
		 */
		void ProcessDirectory(const std::filesystem::path& directoryPath);

		/**
		 * @brief Reloads all assets.
		 */
		void ReloadAssets();

		/**
		 * @brief Writes the asset registry to a file.
		 */
		void WriteRegistryToFile();

		/**
		 * @brief Handles the event when an asset is renamed.
		 * @param assetHandle The handle of the asset that was renamed.
		 * @param newFilePath The new file path of the renamed asset.
		 */
		void OnAssetRenamed(AssetHandle assetHandle, const std::filesystem::path& newFilePath);

		/**
		 * @brief Handles the event when an asset is deleted.
		 * @param assetHandle The handle of the asset that was deleted.
		 */
		void OnAssetDeleted(AssetHandle assetHandle);

		/**
		 * @brief Updates the dependents of the specified asset.
		 * @param handle The handle of the asset whose dependents are to be updated.
		 */
		void UpdateDependents(AssetHandle handle);

		std::vector<Scope<Asset>> m_AssetSets;

		std::unordered_map<AssetHandle, std::unordered_set<AssetHandle>> m_AssetDependents;   // asset handle -> assets that depend on it.
		std::unordered_map<AssetHandle, std::unordered_set<AssetHandle>> m_AssetDependencies; // asset handle -> assets that it depends on.
		std::shared_mutex m_AssetDependenciesMutex;

		// Asset registry is accessed from multiple threads.
		// Access requires synchronization through m_AssetRegistryMutex
		// It is _written to_ only by main thread, so reading in main thread can be done without mutex
		AssetRegistry m_AssetRegistry;
		std::shared_mutex m_AssetRegistryMutex;

		std::unordered_map<AssetHandle, Ref<Asset>> m_LoadedAssets;
		std::unordered_map<AssetHandle, Ref<Asset>> m_MemoryAssets;
		std::shared_mutex m_MemoryAssetsMutex;
	};

}

// -------------------------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* editor_asset_manager.h
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/asset/asset.h>
#include <SceneryEditorX/asset/asset_metadata.h>
#include <SceneryEditorX/asset/asset_registry.h>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/events/editor_events.h>
#include <SceneryEditorX/platform/cfg_loader.h>
#include <SceneryEditorX/platform/file_manager.hpp>
#include <shared_mutex>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    class EditorAssetSystem;

    class EditorAssetManager : public RefCounted
	{
	public:
		EditorAssetManager();

		virtual void Shutdown();

		virtual AssetType GetAssetType(AssetHandle assetHandle);
        virtual Ref<Asset> GetAsset(AssetHandle assetHandle);
        virtual AsyncAssetResult<Asset> GetAssetAsync(AssetHandle assetHandle);

		virtual void AddMemoryOnlyAsset(Ref<Asset> asset);
		virtual bool ReloadData(AssetHandle assetHandle);
        virtual void ReloadDataAsync(AssetHandle assetHandle);
        virtual bool EnsureCurrent(AssetHandle assetHandle);
		virtual bool EnsureAllLoadedCurrent();
		virtual bool IsAssetHandleValid(AssetHandle assetHandle) { return GetMemoryAsset(assetHandle) || GetMetadata(assetHandle).IsValid(); }
        virtual Ref<Asset> GetMemoryAsset(AssetHandle handle);
        virtual bool IsAssetLoaded(AssetHandle handle);
        virtual bool IsAssetValid(AssetHandle handle);
        virtual bool IsAssetMissing(AssetHandle handle);
        virtual bool IsMemoryAsset(AssetHandle handle);
        virtual bool IsPhysicalAsset(AssetHandle handle);
        virtual void RemoveAsset(AssetHandle handle);

		virtual void RegisterDependency(AssetHandle dependency, AssetHandle handle);
        virtual void DeregisterDependency(AssetHandle dependency, AssetHandle handle);
        virtual void DeregisterDependencies(AssetHandle handle);
        virtual std::unordered_set<AssetHandle> GetDependencies(AssetHandle handle);

		virtual void SyncWithAssetThread();

		virtual std::unordered_set<AssetHandle> GetAllAssetsWithType(AssetType type);
		virtual const std::unordered_map<AssetHandle, Ref<Asset>>& GetLoadedAssets() { return m_LoadedAssets; }

		// ------------- Editor-only ----------------

		const AssetRegistry& GetAssetRegistry() const { return m_AssetRegistry; }

		// Get all memory-only assets.
		// Returned by value so that caller need not hold a lock on m_MemoryAssetsMutex
        std::unordered_map<AssetHandle, Ref<Asset>> GetMemoryAssets();

		// note: GetMetadata(AssetHandle) is the ONLY EditorAssetManager function that it is safe to call
		//       from any thread.
		//       All other methods on EditorAssetManager are thread-unsafe and should only be called from the main thread.
		//       SetMetadata() must only be called from main-thread, otherwise it will break safety of all the other
		//       un-synchronized EditorAssetManager functions.
		//
		// thread-safe access to metadata
		// This function returns an AssetMetadata (specifically not a reference) as with references there is no guarantee
		// that the referred to data doesn't get modified (or even destroyed) by another thread
        AssetMetadata GetMetadata(AssetHandle handle);
		// note: do NOT add non-const version of GetMetadata().  For thread-safety you must modify through SetMetaData()

		// thread-safe modification of metadata
		// TODO: don't really need the handle parameter since handle is in metadata anyway
        void SetMetadata(AssetHandle handle, const AssetMetadata &metadata);

		AssetHandle ImportAsset(const std::filesystem::path &filepath);
		AssetHandle GetAssetHandleFromFilePath(const std::filesystem::path &filepath);

		AssetType GetAssetTypeFromExtension(const std::string& extension);
		std::string GetDefaultExtensionForAssetType(AssetType type);
		AssetType GetAssetTypeFromPath(const std::filesystem::path& path);

		std::filesystem::path GetFileSystemPath(AssetHandle handle);
		std::filesystem::path GetFileSystemPath(const AssetMetadata& metadata);
		std::string GetFileSystemPathString(const AssetMetadata& metadata);
		std::filesystem::path GetRelativePath(const std::filesystem::path& filepath);

		bool FileExists(const AssetMetadata& metadata) const;

		template<typename T, typename... Args>
		Ref<T> CreateOrReplaceAsset(const std::filesystem::path& path, Args&&... args)
		{
			static_assert(std::is_base_of_v<Asset, T>, "CreateOrReplaceAsset only works for types derived from Asset");

			/// Check if asset for this file already exists.
			/// If it does, and it's the same type we just replace existing asset
			/// Otherwise we create a whole new asset.
			auto relativePath = GetRelativePath(path);
			auto handle = GetAssetHandleFromFilePath(relativePath);
			AssetMetadata metadata = handle ? GetMetadata(handle) : AssetMetadata{};
			if (metadata.Type != T::GetStaticType())
                metadata = {};

            bool replaceAsset = false;
			if (metadata.Handle == 0)
			{
				metadata.Handle = {};
				metadata.FilePath = relativePath;
				metadata.Type = T::GetStaticType();
				metadata.IsDataLoaded = true;
				SetMetadata(metadata.Handle, metadata);
				WriteRegistryToFile();
			}
			else
			{
				replaceAsset = true;
			}

			Ref<T> asset = Ref<T>::Create(std::forward<Args>(args)...);
			asset->Handle = metadata.Handle;
			m_LoadedAssets[asset->Handle] = asset;
			AssetImporter::Serialize(metadata, asset);

			///< Read serialized timestamp
			const auto absolutePath = GetFileSystemPath(metadata);
			metadata.FileLastWriteTime = IO::FileSystem::GetLastWriteTime(absolutePath);
			SetMetadata(metadata.Handle, metadata);

			if (replaceAsset)
			{
				SEDX_CORE_INFO_TAG("AssetManager", "Replaced asset {}", metadata.FilePath.string());
				UpdateDependents(metadata.Handle);
				Application::Get().DispatchEvent<AssetReloadedEvent, /*DispatchImmediately=*/true>(metadata.Handle);
			}

			return asset;
		}

		void ReplaceLoadedAsset(const AssetHandle handle, const Ref<Asset> &newAsset) { m_LoadedAssets[handle] = newAsset; }


	private:
        Ref<Asset> GetAssetIncludingInvalid(AssetHandle assetHandle);

		void LoadAssetRegistry();
		void ProcessDirectory(const std::filesystem::path& directoryPath);
		void ReloadAssets();
		void WriteRegistryToFile();

		void OnAssetRenamed(AssetHandle assetHandle, const std::filesystem::path &newFilePath);
        void OnAssetDeleted(AssetHandle assetHandle);

		void UpdateDependents(AssetHandle handle);

		///< TODO: move to AssetSystem
		///< NOTE: this collection is accessed only from the main thread, and so does not need any synchronization
        std::unordered_map<AssetHandle, Ref<Asset>> m_LoadedAssets;

		// NOTE: this collection is accessed and modified from both the main thread and the asset thread, and so requires synchronization
        std::unordered_map<AssetHandle, Ref<Asset>> m_MemoryAssets;
		std::shared_mutex m_MemoryAssetsMutex;

		std::unordered_map<AssetHandle, std::unordered_set<AssetHandle>> m_AssetDependents;   // asset handle -> assets that depend on it.
		std::unordered_map<AssetHandle, std::unordered_set<AssetHandle>> m_AssetDependencies; // asset handle -> assets that it depends on.
		std::shared_mutex m_AssetDependenciesMutex;

		Ref<EditorAssetSystem> m_AssetThread;

		// Asset registry is accessed from multiple threads.
		// Access requires synchronization through m_AssetRegistryMutex
		// It is _written to_ only by main thread, so reading in main thread can be done without mutex
		AssetRegistry m_AssetRegistry;
		std::shared_mutex m_AssetRegistryMutex;

		friend class ContentBrowserPanel;
		friend class ContentBrowserAsset;
		friend class ContentBrowserDirectory;
		friend class EditorAssetSystem;
	};

}

/// -------------------------------------------------------

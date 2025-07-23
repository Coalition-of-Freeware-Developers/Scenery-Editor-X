/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* editor_asset_manager.cpp
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#include <nlohmann/json.hpp>
#include <SceneryEditorX/asset/asset_importer.h>
#include <SceneryEditorX/asset/editor_asset_manager.h>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/events/editor_events.h>
#include <SceneryEditorX/core/time/timer.h>
#include <SceneryEditorX/logging/profiler.hpp>
#include <SceneryEditorX/platform/file_manager.hpp>
#include <SceneryEditorX/project/project.h>
#include <SceneryEditorX/utils/string_utils.h>
#include "asset_extentions.h"
#include "asset_types.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{

	static AssetMetadata s_NullMetadata;

	EditorAssetManager::EditorAssetManager()
	{
#if ASYNC_ASSETS
		m_AssetThread = Ref<EditorAssetSystem>::Create();
#endif

		AssetImporter::Init();

		LoadAssetRegistry();
		ReloadAssets();
	}

	void EditorAssetManager::Shutdown()
	{
#if ASYNC_ASSETS
		m_AssetThread->StopAndWait();
#endif
		WriteRegistryToFile();
	}

	AssetType EditorAssetManager::GetAssetType(AssetHandle assetHandle)
	{
		if (!IsAssetHandleValid(assetHandle))
			return AssetType::None;

		if (IsMemoryAsset(assetHandle))
			return GetAsset(assetHandle)->GetAssetType();

		const auto& metadata = GetMetadata(assetHandle);
		return metadata.Type;
	}

	Ref<Asset> EditorAssetManager::GetAsset(AssetHandle assetHandle)
	{
		SEDX_PROFILE_FUNC();
		//SEDX_SCOPE_PERF("AssetManager::GetAsset")

		Ref<Asset> asset = GetAssetIncludingInvalid(assetHandle);
		return asset && asset->IsValid() ? asset : nullptr;
	}

	AsyncAssetResult<Asset> EditorAssetManager::GetAssetAsync(AssetHandle assetHandle)
	{
#if ASYNC_ASSETS
		SEDX_PROFILE_FUNC();
		SEDX_SCOPE_PERF("AssetManager::GetAssetAsync");

		if (auto asset = GetMemoryAsset(assetHandle); asset)
			return { asset, true };

		auto metadata = GetMetadata(assetHandle);
		if (!metadata.IsValid())
			return { nullptr }; // TODO: return special error asset

		Ref<Asset> asset = nullptr;
		if (metadata.IsDataLoaded)
		{
			SEDX_CORE_VERIFY(m_LoadedAssets.contains(assetHandle));
			return { m_LoadedAssets.at(assetHandle), true };
		}

		// Queue load (if not already) and return placeholder
		if (metadata.Status != AssetStatus::Loading)
		{
			auto metadataLoad = metadata;
			metadataLoad.Status = AssetStatus::Loading;
			SetMetadata(assetHandle, metadataLoad);
			m_AssetThread->QueueAssetLoad(metadata);
		}

		return AssetManager::GetPlaceholderAsset(metadata.Type);
#else
		return { GetAsset(assetHandle), true };
#endif
	}

	void EditorAssetManager::AddMemoryOnlyAsset(const Ref<Asset> asset)
	{
		///< Memory-only assets are not added to m_AssetRegistry (because that would require full thread synchronization for access to registry, we would like to avoid that)
		std::scoped_lock lock(m_MemoryAssetsMutex);
		m_MemoryAssets[asset->Handle] = asset;
	}

	std::unordered_set<AssetHandle> EditorAssetManager::GetAllAssetsWithType(AssetType type)
	{
        std::unordered_set<AssetHandle> result;

		/**
		 * @brief Loop over memory only assets.
		 *
		 * @note: This needs a lock because asset thread can create memory only assets
		 */
		{
			std::shared_lock lock(m_MemoryAssetsMutex);
			for (const auto& [handle, asset] : m_MemoryAssets)
			{
				if (asset->GetAssetType() == type)
					result.insert(handle);
			}
		}

		{
			std::shared_lock lock(m_AssetRegistryMutex);
			for (const auto& [handle, metadata] : m_AssetRegistry)
			{
				if (metadata.Type == type)
					result.insert(handle);
			}
		}
		return result;
	}

	std::unordered_map<AssetHandle, Ref<Asset>> EditorAssetManager::GetMemoryAssets()
	{
		std::shared_lock lock(m_MemoryAssetsMutex);
		return m_MemoryAssets;
	}

	AssetMetadata EditorAssetManager::GetMetadata(AssetHandle handle)
	{
		std::shared_lock lock(m_AssetRegistryMutex);

		if (m_AssetRegistry.Contains(handle))
			return m_AssetRegistry.Get(handle);

		return s_NullMetadata;
	}

	void EditorAssetManager::SetMetadata(AssetHandle handle, const AssetMetadata &metadata)
	{
		std::unique_lock lock(m_AssetRegistryMutex);
		m_AssetRegistry.Set(handle, metadata);
	}


	AssetHandle EditorAssetManager::GetAssetHandleFromFilePath(const std::filesystem::path &filepath)
	{
		const auto relativePath = GetRelativePath(filepath);
		std::shared_lock lock(m_AssetRegistryMutex);
		for (auto &metadata : m_AssetRegistry | std::views::values)
		{
			if (metadata.FilePath == relativePath)
			{
				return metadata.Handle;
			}
		}
		return 0;
	}


	AssetType EditorAssetManager::GetAssetTypeFromExtension(const std::string& extension)
	{
		const std::string ext = Utils::String::ToLowerCopy(extension);
		if (!s_AssetExtensionMap.contains(ext))
			return AssetType::None;

		return s_AssetExtensionMap.at(ext);
	}

	std::string EditorAssetManager::GetDefaultExtensionForAssetType(AssetType type)
	{
		for (const auto& [ext, assetType] : s_AssetExtensionMap)
		{
			if (assetType == type)
				return ext;
		}
		return "";
	}

	AssetType EditorAssetManager::GetAssetTypeFromPath(const std::filesystem::path& path)
	{
		return GetAssetTypeFromExtension(path.extension().string());
	}

	std::filesystem::path EditorAssetManager::GetFileSystemPath(const AssetMetadata& metadata)
	{
		return Project::GetActiveAssetDirectory() / metadata.FilePath;
	}

	std::filesystem::path EditorAssetManager::GetFileSystemPath(AssetHandle handle)
	{
		return GetFileSystemPathString(GetMetadata(handle));
	}

	std::string EditorAssetManager::GetFileSystemPathString(const AssetMetadata& metadata)
	{
		return GetFileSystemPath(metadata).string();
	}

	std::filesystem::path EditorAssetManager::GetRelativePath(const std::filesystem::path& filepath)
	{
		std::filesystem::path relativePath = filepath.lexically_normal();
		std::string temp = filepath.string();
		if (temp.find(Project::GetActiveAssetDirectory().string()) != std::string::npos)
		{
			relativePath = std::filesystem::relative(filepath, Project::GetActiveAssetDirectory());
			if (relativePath.empty())
			{
				relativePath = filepath.lexically_normal();
			}
		}
		return relativePath;
	}

	bool EditorAssetManager::FileExists(const AssetMetadata& metadata) const
	{
		return IO::FileSystem::Exists(Project::GetActive()->GetAssetDirectory() / metadata.FilePath);
	}

	bool EditorAssetManager::ReloadData(AssetHandle assetHandle)
	{
		auto metadata = GetMetadata(assetHandle);
		if (!metadata.IsValid())
		{
			SEDX_CORE_ERROR("Trying to reload invalid asset");
			return false;
		}

		Ref<Asset> asset = GetAsset(assetHandle);

		// If the asset is a Mesh, StaticMesh, Rig, or Animation, then instead of reloading the mesh we reload
		// the underlying mesh source.
		// (the assumption being that its the mesh source that's likely changed (e.g. via DCC authoring tool) and
		// its that content that the user wishes to reload)
		// The Mesh/StaticMesh/Rig/Animation ends up getting reloaded anyway due asset dependencies)
		if (metadata.Type == AssetType::StaticMesh && asset)
		{
			auto mesh = asset.As<StaticMesh>();
			return ReloadData(mesh->GetMeshSource());
		}
		if (metadata.Type == AssetType::Mesh && asset)
		{
			auto mesh = asset.As<Mesh>();
			return ReloadData(mesh->GetMeshSource());
		}
        if (metadata.Type == AssetType::Animation && asset)
        {
            auto animation = asset.As<AnimationAsset>();
            auto mesh = AssetManager::GetAsset<Mesh>(animation->GetMeshHandle());
            bool reloaded = ReloadData(animation->GetAnimationSource());
            if (mesh && mesh->GetMeshSource() != animation->GetAnimationSource())
            {
                reloaded |= ReloadData(mesh->GetMeshSource());
            }
            return reloaded;
        }
        else
        {
            SEDX_CORE_INFO_TAG("AssetManager", "RELOADING ASSET - {}", metadata.FilePath.string());
            metadata.IsDataLoaded = AssetImporter::TryLoadData(metadata, asset);
            if (metadata.IsDataLoaded)
            {
                const auto absolutePath = GetFileSystemPath(metadata);
                metadata.FileLastWriteTime = IO::FileSystem::GetLastWriteTime(absolutePath);
                m_LoadedAssets[assetHandle] = asset;
                SetMetadata(assetHandle, metadata);
                SEDX_CORE_INFO_TAG("AssetManager", "Finished reloading asset {}", metadata.FilePath.string());
                UpdateDependents(assetHandle);
                Application::Get().DispatchEvent<AssetReloadedEvent, /*DispatchImmediately=*/true>(assetHandle);
            }
            else
            {
                SEDX_CORE_ERROR_TAG("AssetManager", "Failed to reload asset {}", metadata.FilePath.string());
            }
        }

        return metadata.IsDataLoaded;
	}

	void EditorAssetManager::ReloadDataAsync(AssetHandle assetHandle)
	{
#if ASYNC_ASSETS
		// Queue load (if not already)
		auto metadata = GetMetadata(assetHandle);
		if (!metadata.IsValid())
		{
			SEDX_CORE_ERROR("Trying to reload invalid asset");
			return;
		}

		if (metadata.Status != AssetStatus::Loading)
		{
			m_AssetThread->QueueAssetLoad(metadata);
			metadata.Status = AssetStatus::Loading;
			SetMetadata(assetHandle, metadata);
		}
#else
		ReloadData(assetHandle);
#endif
	}

	// Returns true if asset was reloaded
    bool EditorAssetManager::EnsureCurrent(AssetHandle assetHandle)
	{
		const auto& metadata = GetMetadata(assetHandle);
		auto absolutePath = GetFileSystemPath(metadata);

		if (!IO::FileSystem::Exists(absolutePath))
			return false;

		AssetHandle actualLastWriteTime = IO::FileSystem::GetLastWriteTime(absolutePath);
        AssetHandle recordedLastWriteTime = metadata.FileLastWriteTime;

		if (actualLastWriteTime == recordedLastWriteTime)
			return false;

		return ReloadData(assetHandle);
	}

	bool EditorAssetManager::EnsureAllLoadedCurrent()
	{
		SEDX_PROFILE_FUNC();

		bool loaded = false;
		for (const auto &handle : m_LoadedAssets | std::views::keys)
		{
			loaded |= EnsureCurrent(handle);
		}
		return loaded;
	}

	Ref<Asset> EditorAssetManager::GetMemoryAsset(AssetHandle handle)
	{
		std::shared_lock lock(m_MemoryAssetsMutex);
		if (const auto it = m_MemoryAssets.find(handle); it != m_MemoryAssets.end())
			return it->second;

		return nullptr;
	}

	bool EditorAssetManager::IsAssetLoaded(AssetHandle handle)
	{
		return m_LoadedAssets.contains(handle);
	}

	bool EditorAssetManager::IsAssetValid(AssetHandle handle)
	{
		SEDX_PROFILE_FUNC();
		SEDX_SCOPE_PERF("AssetManager::IsAssetValid")

		auto asset = GetAssetIncludingInvalid(handle);
		return asset && asset->IsValid();
	}

	bool EditorAssetManager::IsAssetMissing(AssetHandle handle)
	{
		SEDX_PROFILE_FUNC();
		SEDX_SCOPE_PERF("AssetManager::IsAssetMissing")

		if(GetMemoryAsset(handle))
			return false;

		const auto metadata = GetMetadata(handle);
		return !IO::FileSystem::Exists(Project::GetActive()->GetAssetDirectory() / metadata.FilePath);
	}

	bool EditorAssetManager::IsMemoryAsset(AssetHandle handle)
	{
		std::scoped_lock lock(m_MemoryAssetsMutex);
		return m_MemoryAssets.contains(handle);
	}

	bool EditorAssetManager::IsPhysicalAsset(AssetHandle handle)
	{
		return !IsMemoryAsset(handle);
	}

	void EditorAssetManager::RemoveAsset(AssetHandle handle)
	{
		{
			std::scoped_lock lock(m_MemoryAssetsMutex);
			if (m_MemoryAssets.contains(handle))
				m_MemoryAssets.erase(handle);
		}

		if (m_LoadedAssets.contains(handle))
			m_LoadedAssets.erase(handle);

		{
			std::scoped_lock lock(m_AssetRegistryMutex);
			if (m_AssetRegistry.Contains(handle))
				m_AssetRegistry.Remove(handle);
		}
	}

	// handle is dependent on dependency
    void EditorAssetManager::RegisterDependency(AssetHandle dependency, AssetHandle handle)
	{
		std::scoped_lock lock(m_AssetDependenciesMutex);

		if (dependency != 0)
		{
			SEDX_CORE_ASSERT(handle != 0);
			m_AssetDependents[dependency].insert(handle);
			m_AssetDependencies[handle].insert(dependency);
			return;
		}

		// otherwise just make sure there is an entry in m_AssetDependencies for handle
		if (!m_AssetDependencies.contains(handle))
		{
			m_AssetDependencies[handle] = {};
		}
	}

	// handle is no longer dependent on dependency
    void EditorAssetManager::DeregisterDependency(AssetHandle dependency, AssetHandle handle)
	{
		std::scoped_lock lock(m_AssetDependenciesMutex);
		if (dependency != 0)
		{
			m_AssetDependents[dependency].erase(handle);
			m_AssetDependencies[handle].erase(dependency);
		}
	}

	void EditorAssetManager::DeregisterDependencies(AssetHandle handle)
	{
		std::scoped_lock lock(m_AssetDependenciesMutex);
		if (auto it = m_AssetDependencies.find(handle); it != m_AssetDependencies.end())
		{
            for (AssetHandle dependency : it->second)
			{
				m_AssetDependents[dependency].erase(handle);
			}
			m_AssetDependencies.erase(it);
		}
	}

	std::unordered_set<AssetHandle> EditorAssetManager::GetDependencies(AssetHandle handle)
	{
		bool registered = false;
        std::unordered_set<AssetHandle> result;
		{
			std::shared_lock lock(m_AssetDependenciesMutex);
			if (const auto it = m_AssetDependencies.find(handle); it != m_AssetDependencies.end())
			{
				registered = true;
				result = it->second;
			}
		}

		if (!registered)
		{
			if (const auto metadata = GetMetadata(handle); metadata.IsValid())
			{
				AssetImporter::RegisterDependencies(metadata);
				{
					std::shared_lock lock(m_AssetDependenciesMutex);
					if (const auto it = m_AssetDependencies.find(handle); it != m_AssetDependencies.end())
					{
						result = it->second;
					}
				}
			}
			else
			{
				m_AssetDependencies[handle] = {};
			}
			registered = true;

		}
		SEDX_CORE_ASSERT(registered || (GetMetadata(handle).Handle == 0), "asset dependencies are not registered!");

		return result;
	}

	void EditorAssetManager::UpdateDependents(AssetHandle handle)
	{
        std::unordered_set<AssetHandle> dependents;
		{
			std::shared_lock lock(m_AssetDependenciesMutex);
			if (auto it = m_AssetDependents.find(handle); it != m_AssetDependents.end())
				dependents = it->second;
		}
        for (AssetHandle dependent : dependents)
		{
			if(IsAssetLoaded(dependent)) {
                if (Ref<Asset> asset = GetAsset(dependent))
				{
					asset->OnDependencyUpdated(handle);
				}
			}
		}
	}

	void EditorAssetManager::SyncWithAssetThread()
	{
#if ASYNC_ASSETS
		std::vector<EditorAssetLoadResponse> freshAssets;

		m_AssetThread->RetrieveReadyAssets(freshAssets);
		for (auto& alr : freshAssets)
		{
			SEDX_CORE_ASSERT(alr.Asset->Handle == alr.Metadata.Handle, "uint64_t mismatch in AssetLoadResponse");
			m_LoadedAssets[alr.Metadata.Handle] = alr.Asset;
			alr.Metadata.Status = AssetStatus::Ready;
			alr.Metadata.IsDataLoaded = true;
			SetMetadata(alr.Metadata.Handle, alr.Metadata);
		}

		m_AssetThread->UpdateLoadedAssetList(m_LoadedAssets);

		// Update dependencies after syncing everything
		for (const auto& alr : freshAssets)
		{
			UpdateDependents(alr.Metadata.Handle);
		}
#else
		Application::Get().SyncEvents();
#endif
	}

	AssetHandle EditorAssetManager::ImportAsset(const std::filesystem::path &filepath)
	{
		const std::filesystem::path path = GetRelativePath(filepath);

		if (auto handle = GetAssetHandleFromFilePath(path); handle)
		{
			return handle;
		}

		const AssetType type = GetAssetTypeFromPath(path);
		if (type == AssetType::None)
		{
			return 0;
		}

		AssetMetadata metadata;
        metadata.Handle = AssetHandle();
		metadata.FilePath = path;
		metadata.Type = type;

		const auto absolutePath = GetFileSystemPath(metadata);
		metadata.FileLastWriteTime = IO::FileSystem::GetLastWriteTime(absolutePath);
		SetMetadata(metadata.Handle, metadata);

		return metadata.Handle;
	}

	Ref<Asset> EditorAssetManager::GetAssetIncludingInvalid(AssetHandle assetHandle)
	{
		if (auto asset = GetMemoryAsset(assetHandle); asset)
			return asset;

		Ref<Asset> asset = nullptr;
        if (auto metadata = GetMetadata(assetHandle); metadata.IsValid())
		{
			if (metadata.IsDataLoaded)
			{
				asset = m_LoadedAssets[assetHandle];
			}
			else
			{
				if (Application::IsMainThread())
				{
					// If we're main thread, we can just try loading the asset as normal
					SEDX_CORE_INFO_TAG("AssetManager", "LOADING ASSET - {}", metadata.FilePath.string());
					if (AssetImporter::TryLoadData(metadata, asset))
					{
						auto metadataLoaded = metadata;
						metadataLoaded.IsDataLoaded = true;
						const auto absolutePath = GetFileSystemPath(metadata);
						metadataLoaded.FileLastWriteTime = IO::FileSystem::GetLastWriteTime(absolutePath);
						m_LoadedAssets[assetHandle] = asset;
						SetMetadata(assetHandle, metadataLoaded);
						SEDX_CORE_INFO_TAG("AssetManager", "Finished loading asset {}", metadata.FilePath.string());
					}
					else
					{
						SEDX_CORE_ERROR_TAG("AssetManager", "Failed to load asset {}", metadata.FilePath.string());
					}
				}
				else
				{
					// Not main thread -> ask AssetThread for the asset
					// If the asset needs to be loaded, this will load the asset.
					// The load will happen on this thread (which is probably asset thread, but occasionally might be audio thread).
					// The asset will get synced into main thread at next asset sync point.
					asset = m_AssetThread->GetAsset(metadata);
				}
			}
		}
		return asset;
	}

	void EditorAssetManager::LoadAssetRegistry()
	{
		SEDX_CORE_INFO("[AssetManager] Loading Asset Registry");

		const auto& assetRegistryPath = Project::GetAssetRegistryPath();
		if (!IO::FileSystem::Exists(assetRegistryPath))
			return;

		std::ifstream stream(assetRegistryPath);
		SEDX_CORE_ASSERT(stream);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		try
		{
			nlohmann::json data = nlohmann::json::parse(strStream.str());

			if (!data.contains("Assets"))
			{
				SEDX_CORE_ERROR("[AssetManager] Asset Registry appears to be corrupted!");
				SEDX_CORE_VERIFY(false);
				return;
			}

			auto handles = data["Assets"];
			if (!handles.is_array())
			{
				SEDX_CORE_ERROR("[AssetManager] Assets field is not an array!");
				SEDX_CORE_VERIFY(false);
				return;
			}

			for (const auto& entry : handles)
			{
				if (!entry.contains("FilePath") || !entry.contains("Handle") || !entry.contains("Type"))
				{
					SEDX_CORE_WARN("[AssetManager] Skipping malformed asset entry");
					continue;
				}

				std::string filepath = entry["FilePath"].get<std::string>();

				AssetMetadata metadata;
				metadata.Handle = entry["Handle"].get<uint64_t>();
				metadata.FilePath = filepath;
				metadata.Type = (AssetType)Utils::AssetTypeFromString(entry["Type"].get<std::string>());

			if (metadata.Type == AssetType::None)
				continue;

			if (metadata.Type != GetAssetTypeFromPath(filepath))
			{
				SEDX_CORE_WARN_TAG("AssetManager", "Mismatch between stored AssetType and extension type when reading asset registry!");
				metadata.Type = GetAssetTypeFromPath(filepath);
			}

			if (!IO::FileSystem::Exists(GetFileSystemPath(metadata)))
			{
				SEDX_CORE_WARN("[AssetManager] Missing asset '{0}' detected in registry file, trying to locate...", metadata.FilePath);

				std::string mostLikelyCandidate;
				uint32_t bestScore = 0;

				for (auto& pathEntry : std::filesystem::recursive_directory_iterator(Project::GetActiveAssetDirectory()))
				{
					const std::filesystem::path& path = pathEntry.path();

					if (path.filename() != metadata.FilePath.filename())
						continue;

					if (bestScore > 0)
						SEDX_CORE_WARN("[AssetManager] Multiple candidates found...");

					std::vector<std::string> candiateParts = Utils::SplitString(path.string(), "/\\");

					uint32_t score = 0;
					for (const auto& part : candiateParts)
					{
						if (filepath.find(part) != std::string::npos)
							score++;
					}

					SEDX_CORE_WARN("'{0}' has a score of {1}, best score is {2}", path.string(), score, bestScore);

					if (bestScore > 0 && score == bestScore)
					{
						// TODO: How do we handle this?
						// Probably prompt the user at this point?
					}

					if (score <= bestScore)
						continue;

					bestScore = score;
					mostLikelyCandidate = path.string();
				}

				if (mostLikelyCandidate.empty() && bestScore == 0)
				{
					SEDX_CORE_ERROR("[AssetManager] Failed to locate a potential match for '{0}'", metadata.FilePath);
					continue;
				}

				std::ranges::replace(mostLikelyCandidate, '\\', '/');
				metadata.FilePath = std::filesystem::relative(mostLikelyCandidate, Project::GetActive()->GetAssetDirectory());
				SEDX_CORE_WARN("[AssetManager] Found most likely match '{0}'", metadata.FilePath);
			}

			if (metadata.Handle == 0)
			{
				SEDX_CORE_WARN("[AssetManager] uint64_t for {0} is 0, this shouldn't happen.", metadata.FilePath);
				continue;
			}

			SetMetadata(metadata.Handle, metadata);
		}

		SEDX_CORE_INFO("[AssetManager] Loaded {0} asset entries", m_AssetRegistry.Count());
		}
		catch (const nlohmann::json::parse_error& e)
		{
			SEDX_CORE_ERROR("[AssetManager] Failed to parse asset registry JSON: {}", e.what());
			SEDX_CORE_VERIFY(false);
		}
		catch (const nlohmann::json::type_error& e)
		{
			SEDX_CORE_ERROR("[AssetManager] JSON type error in asset registry: {}", e.what());
			SEDX_CORE_VERIFY(false);
		}
		catch (const std::exception& e)
		{
			SEDX_CORE_ERROR("[AssetManager] Unexpected error loading asset registry: {}", e.what());
			SEDX_CORE_VERIFY(false);
		}
	}

	void EditorAssetManager::ProcessDirectory(const std::filesystem::path& directoryPath)
	{
		for (const auto &entry : std::filesystem::directory_iterator(directoryPath))
		{
			if (entry.is_directory())
				ProcessDirectory(entry.path());
			else
				ImportAsset(entry.path());
		}
	}

	void EditorAssetManager::ReloadAssets()
	{
		ProcessDirectory(Project::GetActiveAssetDirectory().string());
		WriteRegistryToFile();
	}

	void EditorAssetManager::WriteRegistryToFile()
	{
		// Sort assets by UUID to make project management easier
		struct AssetRegistryEntry
		{
			std::string FilePath;
			AssetType Type;
		};
		std::map<UUID, AssetRegistryEntry> sortedMap;
		for (auto &metadata : m_AssetRegistry | std::views::values)
		{
			if (!IO::FileSystem::Exists(GetFileSystemPath(metadata)))
				continue;

			std::string pathToSerialize = metadata.FilePath.string();
			// NOTE: if Windows
			std::ranges::replace(pathToSerialize, '\\', '/');
			sortedMap[metadata.Handle] = { pathToSerialize, metadata.Type };
		}

		SEDX_CORE_INFO("[AssetManager] serializing asset registry with {0} entries", sortedMap.size());

		nlohmann::json registryJson;
		nlohmann::json assetsArray = nlohmann::json::array();

		for (auto& [handle, entry] : sortedMap)
		{
			nlohmann::json assetEntry;
			assetEntry["Handle"] = handle;
			assetEntry["FilePath"] = entry.FilePath;
			assetEntry["Type"] = Utils::AssetTypeToString(entry.Type);
			assetsArray.push_back(assetEntry);
		}

		registryJson["Assets"] = assetsArray;

		const std::string& assetRegistryPath = Project::GetAssetRegistryPath().string();
		std::ofstream fout(assetRegistryPath);
		fout << registryJson.dump(2); // Pretty print with 2-space indentation
	}

	void EditorAssetManager::OnAssetRenamed(AssetHandle assetHandle, const std::filesystem::path &newFilePath)
	{
		AssetMetadata metadata = GetMetadata(assetHandle);
		if (!metadata.IsValid())
			return;

		metadata.FilePath = GetRelativePath(newFilePath);
		SetMetadata(assetHandle, metadata);
		WriteRegistryToFile();
	}

	void EditorAssetManager::OnAssetDeleted(AssetHandle assetHandle)
	{
		RemoveAsset(assetHandle);
		WriteRegistryToFile();
	}

}

/// -------------------------------------------------------

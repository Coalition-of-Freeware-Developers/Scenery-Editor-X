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
 * asset_manager.cpp
 * -------------------------------------------------------
 * Created: 12/02/2026
 * -------------------------------------------------------
 */
#include "asset_manager.h"

#include "SceneryEditorX/asset/asset_extensions.h"
#include "SceneryEditorX/asset/asset_metadata.h"
#include "SceneryEditorX/core/application/application.h"
#include "SceneryEditorX/utils/string_utils.h"
#include <SceneryEditorX/core/events/editor_events.h>
#include <SceneryEditorX/project/project.h>
#include <SceneryEditorX/renderer/renderer.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	static AssetMetadata s_NullMetadata;

	static std::unordered_map<AssetType, std::function<Ref<Asset>()>> s_AssetPlaceholderTable =
	{
		/*
		{ AssetType::Texture, []() { return Renderer::GetWhiteTexture(); }},
		{ AssetType::EnvMap, []() { return Renderer::GetEmptyEnvironment(); }},
		{ AssetType::Font, []() { return Font::GetDefaultFont(); }},
		*/
	};

	AssetHandle AssetManager::ImportAsset(const std::filesystem::path &filepath)
	{
		std::filesystem::path path = GetRelativePath(filepath);

		if (auto handle = GetAssetHandleFromFilePath(path); handle != 0)
		{
			return handle;
		}

		AssetType type = GetAssetTypeFromPath(path);
		if (type == AssetType::None)
		{
			return static_cast<AssetHandle>(0);
		}

		AssetMetadata metadata;
		metadata.handle = AssetHandle();
		metadata.filePath = path;
		metadata.type = type;

		auto absolutePath = GetFileSystemPath(metadata);
		metadata.fileLastWriteTime = IO::FileSystem::GetLastWriteTime(absolutePath);
		SetMetadata(metadata.handle, metadata);

		return metadata.handle;
	}

	void AssetManager::AddMemoryOnlyAsset(Ref<Asset> asset)
	{
		// Memory-only assets are not added to m_AssetRegistry (because that would require full thread synchronization for access to registry, we would like to avoid that)
		std::scoped_lock lock(m_MemoryAssetsMutex);
		m_MemoryAssets[asset->pHandle] = asset;
	}

	bool AssetManager::AddAsset(VmaAllocator allocator,
								VkCommandPool cmdPool,
								VkQueue queue,
								const std::string &modelFile,
								const std::vector<std::string> &textureFiles,
								const VmaAllocationCreateInfo &modelAllocInfo)
	{
		/*
		m_AssetSets.emplace_back(CreateScope<Asset>());
		if (!m_AssetSets.back()->Load(allocator, cmdPool, queue, modelFile, textureFiles, modelAllocInfo))
		{
			m_AssetSets.pop_back();
			return false;
		}
		*/

		SEDX_CORE_TRACE_TAG("AssetManager", "Loaded asset: {} ({} textures)", modelFile, textureFiles.size());
		return true;
	}

	bool AssetManager::IsAssetLoaded(AssetHandle handle)
	{
		return m_LoadedAssets.contains(handle);
	}

	void AssetManager::RemoveAsset(AssetHandle handle)
	{
		{
			std::scoped_lock lock(m_MemoryAssetsMutex);
			if (m_MemoryAssets.contains(handle))
			{
				m_MemoryAssets.erase(handle);
			}
		}

		if (m_LoadedAssets.contains(handle))
		{
			m_LoadedAssets.erase(handle);
		}

		{
			std::scoped_lock lock(m_AssetRegistryMutex);
			if (m_AssetRegistry.Contains(handle))
			{
				m_AssetRegistry.Remove(handle);
			}
		}
	}

	/**
	 * @brief Destroy all assets managed by this AssetManager instance.
	 * 
	 * This method iterates through all loaded assets and properly destroys their
	 * Vulkan resources using the device's memory allocator, then clears the asset list.
	 */
	void AssetManager::DestroyAll()
	{
		SEDX_CORE_TRACE_TAG("AssetManager", "Destroying all assets ({} total)", m_AssetSets.size());

		/*
		Ref<Device> device = RenderContext::Get()->GetDevice();
		SEDX_CORE_ASSERT(device.IsValid(), "Device must be valid to destroy assets");

		VmaAllocator allocator = device->GetMemoryAllocator().GetAllocator();

		for (auto &asset : m_AssetSets)
		{
			if (asset)
			{
				asset->Destroy(allocator);
			}
		}

		m_AssetSets.clear();
		*/

		SEDX_CORE_TRACE_TAG("AssetManager", "All assets destroyed");
	}

	AssetType AssetManager::GetAssetType(AssetHandle assetHandle)
	{
		if (!IsAssetHandleValid(assetHandle))
		{
			return AssetType::None;
		}

		if (IsMemoryAsset(assetHandle))
		{
			auto assetManager = Project::GetAssetManager();
			if (!assetManager)
				return AssetType::None;

			if (Ref<Asset> asset = assetManager->GetAsset(assetHandle))
				return asset->GetAssetType();

			return AssetType::None;
		}

		const auto &metadata = GetMetadata(assetHandle);
		return metadata.type;
	}

	AssetHandle AssetManager::GetAssetHandleFromFilePath(const std::filesystem::path &filepath)
	{
		const auto relativePath = GetRelativePath(filepath);
		std::shared_lock lock(m_AssetRegistryMutex);
		for (auto &metadata : m_AssetRegistry | std::views::values)
		{
			if (metadata.filePath == relativePath)
			{
				return metadata.handle;
			}
		}
		return static_cast<AssetHandle>(0);
	}

	AssetMetadata AssetManager::GetMetadata(const AssetHandle &handle)
	{
		auto assetManager = Project::GetAssetManager();
		if (!assetManager)
			return s_NullMetadata;

		std::shared_lock lock(assetManager->m_AssetRegistryMutex);

		if (assetManager->m_AssetRegistry.Contains(handle))
		{
			return assetManager->m_AssetRegistry.Get(handle);
		}

		return s_NullMetadata;
	}

	AssetType AssetManager::GetAssetTypeFromExtension(const std::string &extension)
	{
		std::string ext = Utils::String::ToLowerCopy(extension);
		if (!s_AssetExtensionMap.contains(ext))
		{
			return AssetType::None;
		}

		return s_AssetExtensionMap.at(ext);
	}

	std::string AssetManager::GetDefaultExtensionForAssetType(AssetType type)
	{
		for (const auto &[ext, assetType] : s_AssetExtensionMap)
		{
			if (assetType == type)
				return ext;
		}
		return "";
	}

	AssetType AssetManager::GetAssetTypeFromPath(const std::filesystem::path &path)
	{
		return GetAssetTypeFromExtension(path.extension().string());
	}

	Ref<Asset> AssetManager::GetAsset(AssetHandle assetHandle)
	{
		Ref<Asset> asset = GetAssetIncludingInvalid(assetHandle);
		return asset && asset->IsValid() ? asset : nullptr;
	}

	Ref<Asset> AssetManager::GetMemoryAsset(AssetHandle handle)
	{
	  auto assetManager = Project::GetAssetManager();
		if (!assetManager)
			return nullptr;

		std::shared_lock lock(assetManager->m_MemoryAssetsMutex);
		if (auto it = assetManager->m_MemoryAssets.find(handle); it != assetManager->m_MemoryAssets.end())
			return it->second;

		return nullptr;
	}

	std::filesystem::path AssetManager::GetRelativePath(const std::filesystem::path &filepath)
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

	Ref<Asset> AssetManager::GetPlaceholderAsset(AssetType type)
	{
		if (s_AssetPlaceholderTable.contains(type))
		{
			return s_AssetPlaceholderTable.at(type)();
		}

		return nullptr;
	}

	const std::unordered_map<AssetHandle, Ref<Asset>> &AssetManager::GetLoadedAssets()
	{
	   static const std::unordered_map<AssetHandle, Ref<Asset>> s_EmptyLoadedAssets;
		auto assetManager = Project::GetAssetManager();
		return assetManager ? assetManager->m_LoadedAssets : s_EmptyLoadedAssets;
	}


	std::unordered_set<AssetHandle> AssetManager::GetAllAssetsWithType(AssetType type)
	{
		std::unordered_set<AssetHandle> result;

		// loop over memory only assets
		// This needs a lock because asset thread can create memory only assets
		{
			std::shared_lock lock(m_MemoryAssetsMutex);
			for (const auto &[handle, asset] : m_MemoryAssets)
			{
				if (asset->GetAssetType() == type)
					result.insert(handle);
			}
		}

		{
			std::shared_lock lock(m_AssetRegistryMutex);
			for (const auto &[handle, metadata] : m_AssetRegistry)
			{
				if (metadata.type == type)
					result.insert(handle);
			}
		}
		return result;
	}

	void AssetManager::SetMetadata(AssetHandle handle, const AssetMetadata &metadata)
	{
		std::unique_lock lock(m_AssetRegistryMutex);
		m_AssetRegistry.Set(std::move(handle), metadata);
	}

	std::filesystem::path AssetManager::GetFileSystemPath(const AssetMetadata &metadata)
	{
		return Project::GetActiveAssetDirectory() / metadata.filePath;
	}

	std::filesystem::path AssetManager::GetFileSystemPath(AssetHandle handle)
	{
		return GetFileSystemPathString(GetMetadata(handle));
	}

	std::string AssetManager::GetFileSystemPathString(const AssetMetadata &metadata)
	{
		return GetFileSystemPath(metadata).string();
	}

	bool AssetManager::ReloadData(AssetHandle assetHandle)
	{
		auto metadata = GetMetadata(assetHandle);
		if (!metadata.IsValid())
		{
			SEDX_CORE_ERROR("Trying to reload invalid asset");
			return false;
		}

		Ref<Asset> asset = GetAsset(assetHandle);

		// TODO: When Mesh::GetMeshSource() is implemented, re-enable the mesh source reload pattern below.
		// If the asset is a Mesh, reload the underlying mesh source instead (mesh reloads via dependency update).
		/*
		if (metadata.type == AssetType::Mesh && asset)
		{
			auto mesh = asset.As<Mesh>();
			return ReloadData(mesh->GetMeshSource());
		}
		*/

		SEDX_CORE_INFO_TAG("AssetManager", "RELOADING ASSET - {}", metadata.filePath.string());

		// TODO: Implement AssetImporter::TryLoadData when AssetImporter is available.
		metadata.isDataLoaded = false; // AssetImporter::TryLoadData(metadata, asset);
		if (metadata.isDataLoaded)
		{
			auto absolutePath = GetFileSystemPath(metadata);
			metadata.fileLastWriteTime = IO::FileSystem::GetLastWriteTime(absolutePath);
			m_LoadedAssets[assetHandle] = asset;
			SetMetadata(assetHandle, metadata);
			SEDX_CORE_INFO_TAG("AssetManager", "Finished reloading asset {}", metadata.filePath.string());
			UpdateDependents(assetHandle);
			Application::Get().DispatchEvent<AssetReloadedEvent, true>(assetHandle);
		}
		else
		{
			SEDX_CORE_ERROR_TAG("AssetManager", "Failed to reload asset {}", metadata.filePath.string());
		}

		return metadata.isDataLoaded;
	}

	void AssetManager::ReloadDataAsync(AssetHandle assetHandle)
	{
	}

	bool AssetManager::EnsureCurrent(AssetHandle assetHandle)
	{
		const auto &metadata = GetMetadata(assetHandle);
		auto absolutePath = GetFileSystemPath(metadata);

		if (!IO::FileSystem::Exists(absolutePath))
			return false;

		uint64_t actualLastWriteTime = IO::FileSystem::GetLastWriteTime(absolutePath);
		uint64_t recordedLastWriteTime = metadata.fileLastWriteTime;

		if (actualLastWriteTime == recordedLastWriteTime)
			return false;

		return ReloadData(assetHandle);
	}

	bool AssetManager::EnsureAllLoadedCurrent()
	{
		bool loaded = false;
		for (const auto &handle : m_LoadedAssets | std::views::keys)
		{
			loaded |= EnsureCurrent(handle);
		}
		return loaded;
	}

	bool AssetManager::IsAssetHandleValid(AssetHandle assetHandle)
	{
	 if (assetHandle == 0)
			return false;

		auto assetManager = Project::GetAssetManager();
		if (!assetManager)
			return false;

		if (IsMemoryAsset(assetHandle))
			return true;

		std::shared_lock lock(assetManager->m_AssetRegistryMutex);
		return assetManager->m_AssetRegistry.Contains(assetHandle);
	}

	bool AssetManager::IsAssetValid(AssetHandle assetHandle)
	{
	   if (!IsAssetHandleValid(assetHandle))
			return false;

		if (IsMemoryAsset(assetHandle))
			return true;

		auto metadata = GetMetadata(assetHandle);
		if (!metadata.IsValid())
			return false;

		return FileExists(metadata);
	}

	bool AssetManager::IsAssetMissing(AssetHandle assetHandle)
	{
	 if (!IsAssetHandleValid(assetHandle) || IsMemoryAsset(assetHandle))
			return false;

		auto metadata = GetMetadata(assetHandle);
		if (!metadata.IsValid())
			return true;

		return !FileExists(metadata);
	}

	bool AssetManager::FileExists(AssetMetadata &metadata)
	{
		return IO::FileSystem::Exists(Project::GetActive()->GetAssetDirectory() / metadata.filePath);
	}

	bool AssetManager::IsMemoryAsset(AssetHandle handle)
	{
	 auto assetManager = Project::GetAssetManager();
		if (!assetManager)
			return false;

		std::shared_lock lock(assetManager->m_MemoryAssetsMutex);
		return assetManager->m_MemoryAssets.contains(handle);
	}

	bool AssetManager::IsPhysicalAsset(AssetHandle handle)
	{
	 return IsAssetHandleValid(handle) && !IsMemoryAsset(handle);
	}

	void AssetManager::RegisterDependency(AssetHandle dependency, AssetHandle handle)
	{
	  auto assetManager = Project::GetAssetManager();
		if (!assetManager || dependency == 0 || handle == 0)
			return;

		std::unique_lock lock(assetManager->m_AssetDependenciesMutex);
		assetManager->m_AssetDependencies[handle].insert(dependency);
		assetManager->m_AssetDependents[dependency].insert(handle);
	}

	void AssetManager::DeregisterDependency(AssetHandle dependency, AssetHandle handle)
	{
		auto assetManager = Project::GetAssetManager();
		if (!assetManager)
			return;

		std::unique_lock lock(assetManager->m_AssetDependenciesMutex);

		if (auto it = assetManager->m_AssetDependencies.find(handle); it != assetManager->m_AssetDependencies.end())
		{
			it->second.erase(dependency);
			if (it->second.empty())
				assetManager->m_AssetDependencies.erase(it);
		}

		if (auto it = assetManager->m_AssetDependents.find(dependency); it != assetManager->m_AssetDependents.end())
		{
			it->second.erase(handle);
			if (it->second.empty())
				assetManager->m_AssetDependents.erase(it);
		}
	}

	void AssetManager::DeregisterDependencies(AssetHandle handle)
	{
	  auto assetManager = Project::GetAssetManager();
		if (!assetManager)
			return;

		std::unique_lock lock(assetManager->m_AssetDependenciesMutex);

		if (auto depsIt = assetManager->m_AssetDependencies.find(handle); depsIt != assetManager->m_AssetDependencies.end())
		{
			for (AssetHandle dependency : depsIt->second)
			{
				if (auto dependentsIt = assetManager->m_AssetDependents.find(dependency); dependentsIt != assetManager->m_AssetDependents.end())
				{
					dependentsIt->second.erase(handle);
					if (dependentsIt->second.empty())
						assetManager->m_AssetDependents.erase(dependentsIt);
				}
			}
			assetManager->m_AssetDependencies.erase(depsIt);
		}

		if (auto dependentsIt = assetManager->m_AssetDependents.find(handle); dependentsIt != assetManager->m_AssetDependents.end())
		{
			for (AssetHandle dependent : dependentsIt->second)
			{
				if (auto depsIt = assetManager->m_AssetDependencies.find(dependent); depsIt != assetManager->m_AssetDependencies.end())
				{
					depsIt->second.erase(handle);
					if (depsIt->second.empty())
						assetManager->m_AssetDependencies.erase(depsIt);
				}
			}
			assetManager->m_AssetDependents.erase(dependentsIt);
		}
	}

	std::unordered_set<AssetHandle> AssetManager::GetDependencies(AssetHandle handle)
	{
		bool registered = false;
		std::unordered_set<AssetHandle> result;
		{
			std::shared_lock lock(m_AssetDependenciesMutex);
			if (auto it = m_AssetDependencies.find(handle); it != m_AssetDependencies.end())
			{
				registered = true;
				result = it->second;
			}
		}

		if (!registered)
		{
			if (auto metadata = GetMetadata(handle); metadata.IsValid())
			{
				SEDX_CORE_INFO_TAG("AssetManager", "This operation is still under development");
				//SEDX_CORE_TRACE_TAG("AssetManager", "Registering dependencies for asset {}", metadata.filePath.string());
				/*
				AssetImporter::RegisterDependencies(metadata);
				{
					std::shared_lock lock(m_AssetDependenciesMutex);
					if (auto it = m_AssetDependencies.find(handle); it != m_AssetDependencies.end())
					{
						result = it->second;
					}
				}
				*/
			}
			else
			{
				m_AssetDependencies[handle] = {};
			}
			registered = true;
		}
		SEDX_CORE_ASSERT(registered || (GetMetadata(handle).handle == 0), "asset dependencies are not registered!");

		return result;
	}

	Ref<Asset> AssetManager::GetAssetIncludingInvalid(AssetHandle assetHandle)
	{
		if (auto asset = GetMemoryAsset(assetHandle); asset)
			return asset;

		Ref<Asset> asset = nullptr;
		auto metadata = GetMetadata(assetHandle);
		if (metadata.IsValid())
		{
			if (metadata.isDataLoaded)
			{
				asset = m_LoadedAssets[assetHandle];
			}
			/*else
			{
				if (Application::IsMainThread())
				{
					// If we're main thread, we can just try loading the asset as normal
					SEDX_CORE_INFO_TAG("AssetManager", "LOADING ASSET - {}", metadata.filePath.string());
					if (AssetImporter::TryLoadData(metadata, asset))
					{
						auto metadataLoaded = metadata;
						metadataLoaded.isDataLoaded = true;
						auto absolutePath = GetFileSystemPath(metadata);
						metadataLoaded.fileLastWriteTime = IO::FileSystem::GetLastWriteTime(absolutePath);
						m_LoadedAssets[assetHandle] = asset;
						SetMetadata(assetHandle, metadataLoaded);
						SEDX_CORE_INFO_TAG("AssetManager", "Finished loading asset {}", metadata.filePath.string());
					}
					else
					{
						SEDX_CORE_ERROR_TAG("AssetManager", "Failed to load asset {}", metadata.filePath.string());
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
			}*/
		}
		return asset;
	}

	void AssetManager::LoadAssetRegistry()
	{
		SEDX_CORE_TRACE_TAG("AssetManager", "Asset Registry loading has not yet been implemented");
		// SEDX_CORE_INFO("[AssetManager] Loading Asset Registry");

		/*
		const auto& assetRegistryPath = Project::GetAssetRegistryPath();
		if (!IO::FileSystem::Exists(assetRegistryPath))
			return;

		std::ifstream stream(assetRegistryPath);
		SEDX_CORE_ASSERT(stream);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		auto handles = data["Assets"];
		if (!handles)
		{
			SEDX_CORE_ERROR("[AssetManager] Asset Registry appears to be corrupted!");
			SEDX_CORE_VERIFY(false);
			return;
		}

		for (auto entry : handles)
		{
			std::string filepath = entry["FilePath"].as<std::string>();

			AssetMetadata metadata;
			metadata.handle = entry["Handle"].as<uint64_t>();
			metadata.filePath = filepath;
			metadata.type = (AssetType)Utils::AssetTypeFromString(entry["Type"].as<std::string>());

			if (metadata.type == AssetType::None)
				continue;

			if (metadata.type != GetAssetTypeFromPath(filepath))
			{
				SEDX_CORE_WARN_TAG("AssetManager", "Mismatch between stored AssetType and extension type when reading asset registry!");
				metadata.type = GetAssetTypeFromPath(filepath);
			}

			if (!IO::FileSystem::Exists(GetFileSystemPath(metadata)))
			{
				SEDX_CORE_WARN("[AssetManager] Missing asset '{0}' detected in registry file, trying to locate...", metadata.FilePath);

				std::string mostLikelyCandidate;
				uint32_t bestScore = 0;

				for (auto& pathEntry : std::filesystem::recursive_directory_iterator(Project::GetActiveAssetDirectory()))
				{
					const std::filesystem::path& path = pathEntry.path();

					if (path.filename() != metadata.filePath.filename())
						continue;

					if (bestScore > 0)
					{
						SEDX_CORE_WARN("[AssetManager] Multiple candidates found...");
					}

					std::vector<std::string> candidateParts = Utils::SplitString(path.string(), "/\\");

					uint32_t score = 0;
					for (const auto& part : candidateParts)
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
					SEDX_CORE_ERROR("[AssetManager] Failed to locate a potential match for '{0}'", metadata.filePath);
					continue;
				}

				std::ranges::replace(mostLikelyCandidate, '\\', '/');
				metadata.filePath = std::filesystem::relative(mostLikelyCandidate, Project::GetActive()->GetAssetDirectory());
				SEDX_CORE_WARN("[AssetManager] Found most likely match '{0}'", metadata.filePath);
			}

			if (metadata.handle == 0)
			{
				SEDX_CORE_WARN("[AssetManager] AssetHandle for {0} is 0, this shouldn't happen.", metadata.filePath);
				continue;
			}

			SetMetadata(metadata.handle, metadata);
		}
		
		SEDX_CORE_INFO("[AssetManager] Loaded {0} asset entries", m_AssetRegistry.Count());
		*/
	}

	void AssetManager::ProcessDirectory(const std::filesystem::path &directoryPath)
	{
		for (const auto &entry : std::filesystem::directory_iterator(directoryPath))
		{
			if (entry.is_directory())
			{
				ProcessDirectory(entry.path());
			}
			else
			{
				ImportAsset(entry.path());
			}
		}
	}

	void AssetManager::ReloadAssets()
	{
		ProcessDirectory(Project::GetActiveAssetDirectory().string());
		WriteRegistryToFile();
	}
	
	void AssetManager::WriteRegistryToFile()
	{
		// Sort assets by UUID to make project management easier
		struct AssetRegistryEntry
		{
			std::string filePath;
			AssetType type;
		};
		std::map<UUID, AssetRegistryEntry> sortedMap;
		for (auto &metadata : m_AssetRegistry | std::views::values)
		{
			if (!IO::FileSystem::Exists(GetFileSystemPath(metadata)))
				continue;

			std::string pathToSerialize = metadata.filePath.string();
			std::ranges::replace(pathToSerialize, '\\', '/');
			sortedMap[metadata.handle] = {.filePath = pathToSerialize, .type = metadata.type};
		}

		SEDX_CORE_INFO("[AssetManager] serializing asset registry with {0} entries", sortedMap.size());

		/**
		 * TODO: Integrate the EDX format for projects and working with xml and json files. 
		 * For now, we'll just write a simple text file with the asset registry entries.  This is not ideal, but it will work for now.  
		 * We can replace this with a proper serialization format later. 
		 */

		/*
		YAML::Emitter out;
		out << YAML::BeginMap;

		out << YAML::Key << "Assets" << YAML::BeginSeq;
		for (auto& [handle, entry] : sortedMap)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Handle" << YAML::Value << handle;
			out << YAML::Key << "FilePath" << YAML::Value << entry.filePath;
			out << YAML::Key << "Type" << YAML::Value << Utils::AssetTypeToString(entry.type);
			out << YAML::EndMap;
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;
		*/

		const std::string &assetRegistryPath = Project::GetAssetRegistryPath().string();
		std::ofstream fout(assetRegistryPath);
	}

	void AssetManager::OnAssetRenamed(AssetHandle assetHandle, const std::filesystem::path &newFilePath)
	{
		AssetMetadata metadata = GetMetadata(assetHandle);
		if (!metadata.IsValid())
			return;

		metadata.filePath = GetRelativePath(newFilePath);
		SetMetadata(assetHandle, metadata);
		WriteRegistryToFile();
	}
	
	void AssetManager::OnAssetDeleted(AssetHandle assetHandle)
	{
		RemoveAsset(assetHandle);
		WriteRegistryToFile();
	}

	void AssetManager::UpdateDependents(AssetHandle handle)
	{
		std::unordered_set<AssetHandle> dependents;
		{
			std::shared_lock lock(m_AssetDependenciesMutex);
			if (auto it = m_AssetDependents.find(handle); it != m_AssetDependents.end())
			{
				dependents = it->second;
			}
		}

		for (const AssetHandle &dependent : dependents)
		{
			if (IsAssetLoaded(dependent))
			{
				if (Ref<Asset> asset = GetAsset(dependent))
				{
					asset->OnDependencyUpdated(handle);
				}
			}
		}
	}

} // namespace SceneryEditorX

// -------------------------------------------------------

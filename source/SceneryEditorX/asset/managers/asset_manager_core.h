/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* asset_manager_core.h
* -------------------------------------------------------
* Created: 9/8/2025
* -------------------------------------------------------
*/
#pragma once
#include <unordered_map>
#include <unordered_set>
#include "SceneryEditorX/asset/asset.h"
#include "SceneryEditorX/asset/asset_types.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
    ///////////////////////////////////////////////////////////////////
	/// AssetManagerBase //////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////
	/// Implementation in RuntimeAssetManager and EditorAssetManager //
	/// Static wrapper in AssetManager ////////////////////////////////
	///////////////////////////////////////////////////////////////////
	class AssetManagerBase : public RefCounted
	{
	public:
		AssetManagerBase() = default;
		virtual ~AssetManagerBase() = default;

		virtual void Shutdown() = 0;

		virtual AssetType GetAssetType(AssetHandle assetHandle) = 0;
		virtual Ref<Asset> GetAsset(AssetHandle assetHandle) = 0;
		virtual AsyncAssetResult<Asset> GetAssetAsync(AssetHandle assetHandle) = 0;

		virtual void AddMemoryOnlyAsset(Ref<Asset> asset) = 0;
		virtual bool ReloadData(AssetHandle assetHandle) = 0;
		virtual void ReloadDataAsync(AssetHandle assetHandle) = 0;
		virtual bool EnsureCurrent(AssetHandle assetHandle) = 0;
		virtual bool EnsureAllLoadedCurrent() = 0;
		virtual bool IsAssetHandleValid(AssetHandle assetHandle) = 0; /// The asset handle is valid (this says nothing about the asset itself)
		virtual Ref<Asset> GetMemoryAsset(AssetHandle handle) = 0;    /// If exists in memory only (i.e. there is no backing file) return it otherwise return nullptr (this is more efficient than IsMemoryAsset() followed by GetAsset())
		virtual bool IsAssetLoaded(AssetHandle handle) = 0;           /// Asset has been loaded from file (it could still be invalid)
		virtual bool IsAssetValid(AssetHandle handle) = 0;            /// Asset file was loaded, but is invalid for some reason (e.g. corrupt file)
		virtual bool IsAssetMissing(AssetHandle handle) = 0;          /// Asset file is missing
		virtual bool IsMemoryAsset(AssetHandle handle) = 0;
		virtual bool IsPhysicalAsset(AssetHandle handle) = 0;
		virtual void RemoveAsset(AssetHandle handle) = 0;

		/// Handle is dependent on dependency.  e.g. handle could be a material, and dependency could be a texture that the material uses.
		virtual void RegisterDependency(AssetHandle dependency, AssetHandle handle) = 0;

		/// Remove dependency of handle on dependency
		virtual void DeregisterDependency(AssetHandle dependency, AssetHandle handle) = 0;

		/// Remove all dependencies of handle
		virtual void DeregisterDependencies(AssetHandle handle) = 0;

		/// Get the dependencies of handle. e.g. handle could be a material, GetDependencies(handle) returns all the textures that the material uses.
		virtual std::unordered_set<AssetHandle> GetDependencies(AssetHandle handle) = 0;

		virtual void SyncWithAssetThread() = 0;

		virtual std::unordered_set<AssetHandle> GetAllAssetsWithType(AssetType type) = 0;
		virtual const std::unordered_map<AssetHandle, Ref<Asset>>& GetLoadedAssets() = 0;
	};
}

/// -------------------------------------------------------

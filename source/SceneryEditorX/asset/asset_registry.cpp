/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* asset_registry.cpp
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/asset/asset_registry.h>
#include <SceneryEditorX/core/application/application.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

#define SEDX_ASSETREGISTRY_LOG 0
#if SEDX_ASSETREGISTRY_LOG
#define ASSET_LOG(...) SEDX_CORE_TRACE_TAG("ASSET", __VA_ARGS__)
#else
#define ASSET_LOG(...)
#endif

	const AssetMetadata &AssetRegistry::Get(const AssetHandle handle) const
	{
        SEDX_CORE_ASSERT(m_AssetRegistry.contains(handle));
		ASSET_LOG("Retrieving const handle {}", handle);
		return m_AssetRegistry.at(handle);
	}

	void AssetRegistry::Set(const AssetHandle handle, const AssetMetadata &metadata)
	{
		SEDX_CORE_ASSERT(metadata.Handle == handle);
		SEDX_CORE_ASSERT(handle != 0);
		SEDX_CORE_ASSERT(Application::IsMainThread(), "AssetRegistry::Set() has been called from other than the main thread!"); // Refer comments in EditorAssetManager
		m_AssetRegistry[handle] = metadata;
	}

	bool AssetRegistry::Contains(const AssetHandle handle) const
	{
		ASSET_LOG("Contains handle {}", handle);
		return m_AssetRegistry.contains(handle);
	}

	size_t AssetRegistry::Remove(const AssetHandle handle)
	{
		ASSET_LOG("Removing handle", handle);
		return m_AssetRegistry.erase(handle);
	}

	void AssetRegistry::Clear()
	{
		ASSET_LOG("Clearing registry");
		m_AssetRegistry.clear();
	}

}

/// -------------------------------------------------------

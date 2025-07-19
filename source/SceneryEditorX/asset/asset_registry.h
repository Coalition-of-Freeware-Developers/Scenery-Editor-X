/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* asset_registry.h
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/asset/asset.h>
#include <SceneryEditorX/asset/asset_metadata.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	// WARNING: The AssetRegistry is not itself thread-safe, so if accessing AssetRegistry
	// via multiple threads, you must take care to provide your own synchronization.
	class AssetRegistry
	{
	public:

		// note: no non-const Get() function.  If you need to modify the metadata, use Set().
		// This aids correct usage in a multithreaded environment.
        const AssetMetadata &Get(const AssetHandle handle) const;
        void Set(const AssetHandle handle, const AssetMetadata &metadata);

		size_t Count() const { return m_AssetRegistry.size(); }
        bool Contains(const AssetHandle handle) const;
        size_t Remove(const AssetHandle handle);
		void Clear();

		auto begin() { return m_AssetRegistry.begin(); }
		auto end() { return m_AssetRegistry.end(); }
		auto begin() const { return m_AssetRegistry.cbegin(); }
		auto end() const { return m_AssetRegistry.cend(); }
	private:
        std::unordered_map<AssetHandle, AssetMetadata> m_AssetRegistry;
	};
}

/// -------------------------------------------------------

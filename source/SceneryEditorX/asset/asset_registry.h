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
 * asset_registry.h
 * -------------------------------------------------------
 * Created: 10/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include "asset_metadata.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class AssetRegistry
	 * @brief Manages the metadata of all assets in the Scenery Editor X, providing thread-safe access and modification of asset metadata.
	 */
	class AssetRegistry
	{
	public:
		/**
		 * @brief Gets the metadata for an asset.  
		 *
		 * @param handle The handle of the asset.
		 * @return The metadata of the asset.
		 * @note This is thread-safe, but the returned reference should not be modified directly.
		 * This aids correct usage in a multi-threaded environment.
		 * @note No non-const Get() function.  If you need to modify the metadata, use Set().
		 */
		[[nodiscard]] const AssetMetadata& Get(const AssetHandle handle) const;

		/**
		 * @brief Sets the metadata for an asset.
		 * @param handle The handle of the asset.
		 * @param metadata The metadata to set for the asset.
		 */
		void Set(const AssetHandle handle, const AssetMetadata& metadata);

		/**
		 * @brief Gets the number of assets in the registry.
		 * @return The number of assets.
		 */
		[[nodiscard]] size_t Count() const { return m_AssetRegistry.size(); }

		/**
		 * @brief Checks if the registry contains an asset.
		 * @param handle The handle of the asset.
		 * @return True if the asset exists, false otherwise.
		 */
		[[nodiscard]] bool Contains(const AssetHandle handle) const;

		/**
		 * @brief Removes an asset from the registry.
		 * @param handle The handle of the asset to remove.
		 * @return The number of assets removed (0 or 1).
		 */
		size_t Remove(const AssetHandle handle);

		/**
		 * @brief Clears all assets from the registry.
		 */
		void Clear();

		/**
		 * @brief Gets an iterator to the beginning of the asset registry.
		 * @return An iterator to the beginning of the asset registry.
		 */
		auto begin() { return m_AssetRegistry.begin(); }

		/**
		 * @brief Gets an iterator to the end of the asset registry.
		 * @return An iterator to the end of the asset registry.
		 */
		auto end() { return m_AssetRegistry.end(); }

		/**
		 * @brief Gets a const iterator to the beginning of the asset registry.
		 * @return A const iterator to the beginning of the asset registry.
		 */
		[[nodiscard]] auto begin() const { return m_AssetRegistry.cbegin(); }

		/**
		 * @brief Gets a const iterator to the end of the asset registry.
		 * @return A const iterator to the end of the asset registry.
		 */
		[[nodiscard]] auto end() const { return m_AssetRegistry.cend(); }
	private:
		std::unordered_map<AssetHandle, AssetMetadata> m_AssetRegistry;
	};

}

// -------------------------------------------------------

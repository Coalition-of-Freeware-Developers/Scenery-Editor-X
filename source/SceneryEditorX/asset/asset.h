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
 * asset.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "asset_types.h"
#include <SceneryEditorX/core/identifiers/uuid.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	typedef UUID AssetHandle;

	/**
	 * @class Asset
	 * @brief Represents a generic asset in the Scenery Editor X.
	 */
	class Asset : public RefCounted
	{
	public:
		AssetHandle pHandle;
		uint16_t pFlags = (uint16_t)AssetFlag::None;

		virtual ~Asset() {}

		static AssetType GetStaticType() { return AssetType::None; }
		virtual AssetType GetAssetType() const { return AssetType::None; }

		/**
		 * @brief Called when a dependency of the asset is updated.
		 * @param handle The handle of the updated dependency.
		 */
		virtual void OnDependencyUpdated(AssetHandle handle) {
		}

		/**
		 * @brief Compares this asset with another asset for equality.
		 * @param other The other asset to compare with.
		 * @return True if the assets are equal, false otherwise.
		 */
		virtual bool operator==(const Asset& other) const { return pHandle == other.pHandle; }

		/**
		 * @brief Compares this asset with another asset for inequality.
		 * @param other The other asset to compare with.
		 * @return True if the assets are not equal, false otherwise.
		 */
		virtual bool operator!=(const Asset& other) const { return !(*this == other); }

	private:
		friend class AssetManager;

		/**
		 * @brief Checks if the asset is valid.
		 * @return True if the asset is valid, false otherwise.
		 */
		bool IsValid() const
		{
			return ((pFlags & (uint16_t)AssetFlag::Missing) | (pFlags & (uint16_t)AssetFlag::Invalid)) == 0;
		}

		/**
		 * @brief Checks if a specific flag is set for the asset.
		 * @param flag The flag to check.
		 * @return True if the flag is set, false otherwise.
		 */
		bool IsFlagSet(AssetFlag flag) const
		{
			return (uint16_t)flag & pFlags;
		}

		/**
		 * @brief Sets or clears a specific flag for the asset.
		 * @param flag The flag to set or clear.
		 * @param value True to set the flag, false to clear it.
		 */
		void SetFlag(AssetFlag flag, bool value = true)
		{
			if (value)
			{
				pFlags |= (uint16_t)flag;
			}
			else
			{
				pFlags &= ~(uint16_t)flag;
			}
		}
	};
	
	// -------------------------------------------------------

	/**
	 * @brief Represents the result of an asynchronous asset loading operation, containing a reference to the asset and its readiness state.
	 * @tparam T The type of the asset being loaded.
	 */
	template<typename T>
	struct AsyncAssetResult
	{
		Ref<T> p_Asset;
		bool p_IsReady = false;

		AsyncAssetResult() = default;

		/**
		 * @brief Copy constructor for AsyncAssetResult.
		 * @param other The AsyncAssetResult to copy from.
		 */
		AsyncAssetResult(const AsyncAssetResult<T>& other) = default;

		/**
		 * @brief Constructs an AsyncAssetResult with the given asset and readiness state.
		 * @param asset The asset being loaded.
		 * @param isReady Indicates whether the asset is ready.
		 */
		AsyncAssetResult(Ref<T> asset, bool isReady = false) : p_Asset(asset), p_IsReady(isReady) {}

		/**
		 * @brief Constructs an AsyncAssetResult from another AsyncAssetResult of a different type, performing a type conversion if possible.
		 * @tparam T2 The type of the asset in the other AsyncAssetResult.
		 * @param other The other AsyncAssetResult to convert from.
		 */
		template<typename T2>
		AsyncAssetResult(const AsyncAssetResult<T2>& other) : p_Asset(other.p_Asset.template As<T>()), p_IsReady(other.p_IsReady) {}

		/* @brief Converts the AsyncAssetResult to a Ref<T>, allowing access to the underlying asset. */
		operator Ref<T>() const { return p_Asset; }

		/* @brief Converts the AsyncAssetResult to a boolean, indicating whether the asset is ready. */
		operator bool() const { return p_IsReady; }
	};
}

// -------------------------------------------------------

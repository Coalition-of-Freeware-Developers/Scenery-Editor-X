/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* asset.h
* -------------------------------------------------------
* Created: 15/4/2025
* -------------------------------------------------------
*/
#pragma once
#include "object.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{

	// ReSharper disable once CppRedundantQualifier
	using AssetHandle = UUID;

    /// -------------------------------------------------------

	/**
	 * @brief Bitfield flags for tracking asset state and validation status
	 * 
	 * AssetFlag provides a bitfield enumeration for tracking various states
	 * and conditions of assets within the Scenery Editor X engine. These flags
	 * are used by the asset management system to track loading status, validation
	 * results, and runtime conditions that affect asset usability.
	 * 
	 * The flags are designed to be combined using bitwise operations to represent
	 * multiple simultaneous states. This allows the asset system to efficiently
	 * track complex asset conditions and make informed decisions about asset
	 * loading, caching, and usage.
	 * 
	 * Key use cases:
	 * - Asset validation during loading to detect corrupted or incomplete data
	 * - Hot-reloading system to track assets that need to be reprocessed
	 * - Editor UI to display appropriate warnings and status indicators
	 * - Dependency resolution to handle missing asset references gracefully
	 * - Asset streaming to prioritize loading of critical assets
	 * 
	 * @note - Flags use BIT() macro to ensure proper bit positioning
	 * @note - uint16_t provides 16 possible flag combinations while maintaining memory efficiency
	 * @note - Additional flags can be added using BIT(2), BIT(3), etc. for future extensions
	 * 
	 * @code
	 * // Usage example for asset validation
	 * AssetFlag flags = AssetFlag::None;
	 * 
	 * if (!ValidateAssetData(asset))
	 * {
	 *     flags |= AssetFlag::Invalid;
	 * }
	 * 
	 * if (!FileExists(asset->GetPath()))
	 * {
	 *     flags |= AssetFlag::Missing;
	 * }
	 * 
	 * // Check for specific conditions
	 * if (flags & AssetFlag::Missing)
	 * {
	 *     SEDX_CORE_WARN("Asset file missing: {}", asset->GetPath());
	 *     // Attempt to locate backup or placeholder
	 * }
	 * 
	 * if (flags & AssetFlag::Invalid)
	 * {
	 *     SEDX_CORE_ERROR("Asset data corrupted: {}", asset->GetPath());
	 *     // Mark for reloading or use fallback asset
	 * }
	 * @endcode
	 */
    enum class AssetFlag : uint16_t  // NOLINT(performance-enum-size)
    {
        None = 0,
        Missing = BIT(0),
        Invalid = BIT(1)
    };

    class Asset : public Object
    {
    public:
        AssetHandle Handle = AssetHandle(0);
        uint16_t Flags = (uint16_t)AssetFlag::None;
        virtual ~Asset() override;

        GLOBAL ObjectType GetStaticType() { return ObjectType::None; }
        virtual ObjectType GetAssetType() const { return ObjectType::None; }

        virtual bool operator!=(const Asset &other) const { return !(*this == other); }
        virtual bool operator==(const Asset &other) const { return Handle == other.Handle; }

        /// -------------------------------------------------------

        virtual void OnDependencyUpdated(uint64_t handle)
        {
        }
        //virtual void Serialize(SerializeWriter &ser) override = 0;
        virtual void Load(const std::string &path) = 0;
        virtual void Unload() = 0;
        virtual void SetName(const std::string &name) = 0;

        /// -------------------------------------------------------

    private:
        //friend class EditorAssetManager;
        //friend class TextureSerializer;
        friend class TextureAsset;

        bool IsValid() const
        {
            // ReSharper disable once CppRedundantParentheses
            return ((Flags & (uint16_t)(AssetFlag::Missing)) | (Flags & (uint16_t)(AssetFlag::Invalid))) == 0;
        }

        bool IsFlagSet(AssetFlag flag) const
        {
            return static_cast<uint16_t>(flag) & Flags;
        }

        void SetFlag(AssetFlag flag, const bool value = true)
        {
            if (value)
                Flags |= static_cast<uint16_t>(flag);
            else
                Flags &= ~static_cast<uint16_t>(flag);
        }
    };

    /// -------------------------------------------------------

    template <typename T>
    struct AsyncAssetResult
    {
        Ref<T> Asset;
        bool IsReady = false;

        AsyncAssetResult() = default;
        AsyncAssetResult(const AsyncAssetResult<T> &other) = default;

        explicit AsyncAssetResult(Ref<T> asset, const bool isReady = false) : Asset(asset), IsReady(isReady)
        {
        }

        template <typename T2>
        explicit AsyncAssetResult(const AsyncAssetResult<T2> &other)
            : Asset(other.Asset.template As<T>()), IsReady(other.IsReady)
        {
        }

        explicit operator Ref<T>() const
        {
            return Asset;
        }
        explicit operator bool() const
        {
            return IsReady;
        }
    };


}

/// -------------------------------------------------------

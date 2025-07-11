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
#include <SceneryEditorX/core/pointers.h>
#include <SceneryEditorX/serialization/serializer_writer.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

    /// -------------------------------------------------------

	enum class ObjectType : uint8_t
	{
		None = 0,
	    Invalid,
		EnvMap,
	    TextureAsset,
        PrefabAsset,
	    MeshAsset,
	    MaterialAsset,
	    SceneAsset,
	    Node,
	    MeshNode,
	    LightNode,
	    CameraNode,
	    Count,
	};
	
	inline std::string ObjectTypeName[] = {
	    "Invalid",
	    "Texture",
	    "Mesh",
	    "Material",
	    "Scene",
	    "Node",
	    "MeshNode",
	    "LightNode",
	    "CameraNode",
	    "Count",
	};

	enum class AssetFlag : uint16_t
    {
        None = 0,
        Missing = BIT(0),
        Invalid = BIT(1)
    };

	/// -------------------------------------------------------

	struct Object : RefCounted
	{
	    std::string name = "Uninitialized";
	    uint32_t uuid = 0;
	    ObjectType type = ObjectType::Invalid;
	    //TODO: rethink this gpu dirty flag...
	    bool gpuDirty = true;

        Object &operator=(const Object &rhs)
	    {
	        name = rhs.name;
	        type = rhs.type;
	        gpuDirty = true;
	        return *this;
	    }
	
	    virtual ~Object() override;
	    virtual void Serialize(SerializeWriter &ser) = 0;
	};

    /// -------------------------------------------------------

	class Asset : public Object
	{
	public:
        uint64_t Handle = 0;
        uint16_t Flags = (uint16_t)AssetFlag::None;
        virtual ~Asset() override;

		GLOBAL ObjectType GetStaticType() { return ObjectType::None; }
        [[nodiscard]] virtual ObjectType GetAssetType() const { return ObjectType::None; }

        [[nodiscard]] virtual bool operator!=(const Asset& other) const { return !(*this == other); }
	    [[nodiscard]] virtual bool operator==(const Asset &other) const { return Handle == other.Handle;}

	    virtual void OnDependencyUpdated(uint64_t handle) {}
        virtual void Serialize(SerializeWriter &ser) override = 0;
        virtual void Load(const std::string &path) = 0;
        virtual void Unload() = 0;
        virtual void SetName(const std::string &name) = 0;

	private:
        friend class EditorAssetManager;
        friend class TextureAsset;

	    bool IsValid() const { return (Flags & (uint16_t)AssetFlag::Missing | Flags & (uint16_t)AssetFlag::Invalid) == 0; }
		bool IsFlagSet(AssetFlag flag) const { return (uint16_t)flag & Flags; }
		void SetFlag(AssetFlag flag, bool value = true)
		{
			if (value)
				Flags |= (uint16_t)flag;
			else
				Flags &= ~(uint16_t)flag;
		}
    };

	template<typename T>
	struct AsyncAssetResult
	{
		Ref<T> Asset;
		bool IsReady = false;

		AsyncAssetResult() = default;
		AsyncAssetResult(const AsyncAssetResult<T>& other) = default;

		AsyncAssetResult(Ref<T> asset, bool isReady = false) : Asset(asset), IsReady(isReady) {}

		template<typename T2>
		AsyncAssetResult(const AsyncAssetResult<T2>& other) : Asset(other.Asset.template As<T>()), IsReady(other.IsReady) {}

		operator Ref<T>() const { return Asset; }
		operator bool() const { return IsReady; }
	};

}

/// -------------------------------------------------------

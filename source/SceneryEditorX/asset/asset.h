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
#include <SceneryEditorX/serialization/serializer_writer.h>
#include <SceneryEditorX/utils/pointers.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

    using AssetHandle = UUID;

	enum class ObjectType : uint16_t
	{
		None = 0,
		Scene,
        Node,
		Prefab,
		Mesh,
		StaticMesh,
		Camera,
		Light,
		MeshSource,
		Material,
		Texture,
		EnvMap,
		Audio,
		SoundConfig,
		SpatializationConfig,
		Font,
		Script,
		ScriptFile,
		MeshCollider,
		SoundGraphSound,
		Animation,
		AnimationGraph
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
        UUID uuid = UUID();
	    ObjectType type = ObjectType::None;
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

	using AssetHandle = UUID;

	class Asset : public Object
	{
	public:
        AssetHandle Handle = AssetHandle(0);
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
		void SetFlag(AssetFlag flag, const bool value = true)
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

		AsyncAssetResult(Ref<T> asset, const bool isReady = false) : Asset(asset), IsReady(isReady) {}

		template<typename T2>
		AsyncAssetResult(const AsyncAssetResult<T2>& other) : Asset(other.Asset.template As<T>()), IsReady(other.IsReady) {}

		operator Ref<T>() const { return Asset; }
		operator bool() const { return IsReady; }
	};

}

/// -------------------------------------------------------

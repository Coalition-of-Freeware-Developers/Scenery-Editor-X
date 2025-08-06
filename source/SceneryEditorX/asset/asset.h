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
#include <SceneryEditorX/core/identifiers/uuid.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	// ReSharper disable once CppRedundantQualifier
	using AssetHandle = UUID;

    /// -------------------------------------------------------

	enum class ObjectType : uint16_t  // NOLINT(performance-enum-size)
	{
		None = 0,
		Scene,
        Node,
		Prefab,
		Mesh,
		StaticMesh,
		Light,
		MeshSource,
		Material,
		Texture,
		EnvMap,
		Font,
		Script,
		ScriptFile,
		MeshCollider,
		Animation,
		AnimationGraph
	};

	inline ObjectType ObjectTypeName(std::string_view objectType)
    {
	        if (objectType == "None")                return ObjectType::None;
			if (objectType == "Scene")               return ObjectType::Scene;
			if (objectType == "Node")                return ObjectType::Node;
			if (objectType == "Prefab")              return ObjectType::Prefab;
			if (objectType == "Mesh")                return ObjectType::Mesh;
			if (objectType == "StaticMesh")          return ObjectType::StaticMesh;
			if (objectType == "MeshSource")          return ObjectType::MeshSource;
			if (objectType == "Material")            return ObjectType::Material;
			if (objectType == "Texture")             return ObjectType::Texture;
			if (objectType == "EnvMap")              return ObjectType::EnvMap;
			if (objectType == "Font")                return ObjectType::Font;
			if (objectType == "Script")              return ObjectType::Script;
			if (objectType == "ScriptFile")          return ObjectType::ScriptFile;
			if (objectType == "MeshCollider")        return ObjectType::MeshCollider;
			if (objectType == "Animation")           return ObjectType::Animation;
			if (objectType == "AnimationGraph")      return ObjectType::AnimationGraph;

			return ObjectType::None;
	}

    inline const char* ObjectTypeToString(ObjectType type)
	{
		switch (type)
		{
			case ObjectType::None:           return "None";
			case ObjectType::Scene:          return "Scene";
			case ObjectType::Node:           return "Node";
			case ObjectType::Prefab:         return "Prefab";
			case ObjectType::Mesh:           return "Mesh";
			case ObjectType::StaticMesh:     return "StaticMesh";
			case ObjectType::Light:          return "Light";
			case ObjectType::MeshSource:     return "MeshSource";
			case ObjectType::Material:       return "Material";
			case ObjectType::Texture:        return "Texture";
			case ObjectType::EnvMap:         return "EnvMap";
			case ObjectType::Font:           return "Font";
			case ObjectType::Script:         return "Script";
			case ObjectType::ScriptFile:     return "ScriptFile";
			case ObjectType::MeshCollider:   return "MeshCollider";
			case ObjectType::Animation:      return "Animation";
			case ObjectType::AnimationGraph: return "AnimationGraph";
		}

        SEDX_CORE_ASSERT(false, "Unknown Asset Type");
        return "None";
    }

    enum class AssetFlag : uint16_t  // NOLINT(performance-enum-size)
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

	    /// TODO: rethink this gpu dirty flag...
	    bool gpuDirty = true;

        Object &operator=(const Object &rhs)
	    {
	        name = rhs.name;
	        type = rhs.type;
	        gpuDirty = true;
	        return *this;
	    }

	    virtual ~Object() override;
	    //virtual void Serialize(SerializeWriter &ser) = 0;
	};

    /// -------------------------------------------------------

	class Asset : public Object
	{
	public:
        AssetHandle Handle = AssetHandle(0);
        uint16_t Flags = (uint16_t)AssetFlag::None;
        virtual ~Asset() override;

		GLOBAL ObjectType GetStaticType() { return ObjectType::None; }
        virtual ObjectType GetAssetType() const { return ObjectType::None; }

        virtual bool operator!=(const Asset& other) const { return !(*this == other); }
        virtual bool operator==(const Asset &other) const { return Handle == other.Handle; }

        /// -------------------------------------------------------

	    virtual void OnDependencyUpdated(uint64_t handle) {}
        //virtual void Serialize(SerializeWriter &ser) override = 0;
        virtual void Load(const std::string &path) = 0;
        virtual void Unload() = 0;
        virtual void SetName(const std::string &name) = 0;

        /// -------------------------------------------------------

	private:
        friend class EditorAssetManager;
        //friend class TextureSerializer;
        friend class TextureAsset;

	    bool IsValid() const
	    {
            // ReSharper disable once CppRedundantParentheses
            return ((Flags & (uint16_t)(AssetFlag::Missing)) | (Flags & (uint16_t)(AssetFlag::Invalid))) == 0;
	    }

		bool IsFlagSet(AssetFlag flag) const { return static_cast<uint16_t>(flag) & Flags; }

		void SetFlag(AssetFlag flag, const bool value = true)
		{
			if (value)
				Flags |= static_cast<uint16_t>(flag);
			else
				Flags &= ~static_cast<uint16_t>(flag);
		}

    };

    /// -------------------------------------------------------

	template<typename T>
	struct AsyncAssetResult
	{
		Ref<T> Asset;
		bool IsReady = false;

		AsyncAssetResult() = default;
		AsyncAssetResult(const AsyncAssetResult<T>& other) = default;

        explicit AsyncAssetResult(Ref<T> asset, const bool isReady = false) : Asset(asset), IsReady(isReady) {}

		template<typename T2>
        explicit AsyncAssetResult(const AsyncAssetResult<T2>& other) : Asset(other.Asset.template As<T>()), IsReady(other.IsReady) {}

        explicit operator Ref<T>() const { return Asset; }
        explicit operator bool() const { return IsReady; }
	};

}

/// -------------------------------------------------------

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

/// -------------------------------------------------------

namespace SceneryEditorX
{
    struct Serializer;

    /// -------------------------------------------------------

	enum class ObjectType : uint8_t
	{
		None = 0,
	    Invalid,
	    TextureAsset,
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
	
	    virtual ~Object();
	    virtual void Serialize(Serializer &ser) = 0;
	};

    /// -------------------------------------------------------

	class Asset : public Object
	{
	public:
        uint64_t Handle = 0;
        virtual ~Asset() override;

		GLOBAL ObjectType GetStaticType() { return ObjectType::None; }
        [[nodiscard]] virtual ObjectType GetAssetType() const { return ObjectType::None; }
        [[nodiscard]] virtual bool operator!=(const Asset& other) const { return !(*this == other); }

		virtual bool operator==(const Asset &other) const { return Handle == other.Handle;}
        virtual void Serialize(Serializer &ser) override = 0;
        virtual void Load(const std::string &path) = 0;
        virtual void Unload() = 0;
        virtual void SetName(const std::string &name) = 0;
    };

} // namespace SceneryEditorX

/// -------------------------------------------------------

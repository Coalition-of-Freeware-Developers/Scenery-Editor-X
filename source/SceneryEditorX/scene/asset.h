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

// -------------------------------------------------------

namespace SceneryEditorX
{
    struct Serializer;

    // -------------------------------------------------------

	enum class ObjectType
	{
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

	// -------------------------------------------------------

	struct Object
	{
	    std::string name = "Uninitialized";
	    uint32_t uuid = 0;
	    ObjectType type = ObjectType::Invalid;
	    // todo: rethink this gpu dirty flag...
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

	class Asset : public Object
	{
	public:
        virtual ~Asset() override;
        virtual void Serialize(Serializer &ser) = 0;

        //virtual void Load(const std::string &path) = 0;
        //virtual void Unload() = 0;
        //virtual void SetName(const std::string &name) = 0;
    };

} // namespace SceneryEditorX

// -------------------------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* model.h
* -------------------------------------------------------
* Created: 15/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <glm/glm.hpp>
#include <SceneryEditorX/scene/asset.h>
#include <SceneryEditorX/scene/material.h>
#include <SceneryEditorX/scene/node.h>
#include <vulkan/vulkan.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct Serializer;
	
	// -------------------------------------------------------

    struct MeshVertex
    {
        Vec3 position;
        Vec3 color;
        Vec3 normal;
        Vec4 tangent;
        Vec2 texCoord;

        bool operator==(const MeshVertex &other) const
        {
            return position == other.position && normal == other.normal && texCoord == other.texCoord;
        }
    };

    // -------------------------------------------------------

    class Model : public Asset
	{
	public:
        Model();
        virtual ~Model() override;

        virtual void Serialize(Serializer &ser) override;

        std::vector<MeshVertex> vertices;
        std::vector<uint32_t> indices;

        // -------------------------------------------------------

        virtual void Load(const std::string &path);
        //virtual void Unload() override;
        //virtual void SetName(const std::string &name) override;
        //[[nodiscard]] virtual bool IsLoaded() const override;
        //[[nodiscard]] virtual const std::string &GetPath() const override;
        //[[nodiscard]] virtual const std::string &GetName() const override;

    private:
        friend class AssetManager;
	};

    struct MeshNode : Node
    {
        Ref<Model> mesh;
        //Ref<MaterialAsset> material;

        MeshNode();
        //virtual void Serialize(Serializer &ser) override;
    };

	// -------------------------------------------------------

} // namespace SceneryEditorX

// -------------------------------------------------------

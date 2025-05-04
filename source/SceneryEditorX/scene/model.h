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
#include <SceneryEditorX/scene/asset.h>
#include <SceneryEditorX/scene/material.h>
#include <SceneryEditorX/scene/node.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct Serializer;
	
	// -------------------------------------------------------

    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 texCoord;

        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription;
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

            return attributeDescriptions;
        }

        bool operator==(const Vertex &other) const
        {
            return pos == other.pos && color == other.color && texCoord == other.texCoord;
        }
    };

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
        virtual void Serialize(Serializer &ser) override;
    };

	// -------------------------------------------------------

} // namespace SceneryEditorX

// -------------------------------------------------------

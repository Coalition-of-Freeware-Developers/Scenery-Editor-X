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
#include <SceneryEditorX/asset/asset.h>
#include <SceneryEditorX/scene/material.h>
//#include <SceneryEditorX/scene/node.h>
#include <vulkan/vulkan.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	//struct Serializer;
	
	/// -------------------------------------------------------

    struct MeshVertex
    {
        Vec3 position;
        Vec3 color;
        Vec3 normal;
        Vec4 tangent;
        Vec2 texCoord;

        /// -------------------------------------------------------

        bool operator==(const MeshVertex &other) const
        {
            return position == other.position && normal == other.normal && texCoord == other.texCoord;
        }

        /// -------------------------------------------------------
       
        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription;
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(MeshVertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            
            return bindingDescription;
        }

        /// -------------------------------------------------------
        
        static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};
            
            /// Position
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(MeshVertex, position);
            
            /// Color
            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(MeshVertex, color);
            
            /// Normal
            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(MeshVertex, normal);
            
            /// Tangent
            attributeDescriptions[3].binding = 0;
            attributeDescriptions[3].location = 3;
            attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attributeDescriptions[3].offset = offsetof(MeshVertex, tangent);
            
            /// Texture coordinates
            attributeDescriptions[4].binding = 0;
            attributeDescriptions[4].location = 4;
            attributeDescriptions[4].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[4].offset = offsetof(MeshVertex, texCoord);
            
            return attributeDescriptions;
        }
    };

    /// -------------------------------------------------------

    class ModelAsset : public Asset
	{
	public:
        ModelAsset();
        virtual ~ModelAsset() override;

		struct Index
        {
            uint32_t V1, V2, V3;
        };

        static_assert(sizeof(Index) == 3 * sizeof(uint32_t));

        struct Triangle
        {
            MeshVertex V0, V1, V2;

            Triangle(const MeshVertex &v0, const MeshVertex &v1, const MeshVertex &v2) : V0(v0), V1(v1), V2(v2) {}
        };

        //virtual void Serialize(Serializer &ser);

        std::vector<MeshVertex> vertices;
        std::vector<uint32_t> indices;

        /// -------------------------------------------------------

        virtual void Load(const std::string &path) override;
        virtual void Unload() override;
        virtual void SetName(const std::string &name) override;
        [[nodiscard]] virtual bool IsLoaded() const;
        [[nodiscard]] virtual const std::string &GetPath() const;
        [[nodiscard]] virtual const std::string &GetName() const;

    private:
        friend class AssetManager;
        std::string modelPath;
        std::string modelName;
        bool loaded = false;
	};

    struct MeshNode /*: GraphNode*/
    {
        uint32_t parent = 0xffffffff;
        Ref<ModelAsset> mesh;
        //Ref<MaterialAsset> material;
        std::vector<uint32_t> children;
        std::vector<uint32_t> submeshes;
        std::string name;
        Mat4 localTransform;

        bool IsRoot() const { return parent == 0xffffffff; }

        MeshNode();
        //virtual void Serialize(Serializer &ser);
    };

	/// ------------------------------------------------------- 

}

/// -------------------------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vertex_buffer.h
* -------------------------------------------------------
* Created: 18/5/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/vulkan/render_data.h>
#include <SceneryEditorX/vulkan/vk_core.h>

/// ----------------------------------------------------------

namespace SceneryEditorX
{
    enum class VertexBufferType : uint8_t
    {
        None = 0,
		Static = 1,
		Dynamic = 2, /// Dynamic vertex buffer
    };

    /**
     * @class VertexBuffer
     * @brief Manages vertex data storage and configuration in Vulkan
     *
     * This class handles the creation and management of vertex buffers in Vulkan,
     * including memory allocation and vertex attribute descriptions. It provides
     * functionality for storing and transferring vertex data to the GPU.
     */
    class VertexBuffer
    {
    public:
        /**
         * @brief Constructor for VertexBuffer
         *
         * Initializes the VertexBuffer instance by getting references to required
         * graphics engine components.
         */
        VertexBuffer();

        /**
         * @brief Destructor for VertexBuffer
         *
         * Cleans up vertex buffer resources and associated memory.
         */
        virtual ~VertexBuffer();

        /**
         * @struct Vertex
         * @brief Defines the structure and layout of vertex data
         *
         * Contains position, color, and texture coordinate data for each vertex,
         * along with methods to describe the vertex layout to Vulkan.
         */
        struct Vertex
        {
            glm::vec3 pos;      ///< 3D position of the vertex
            glm::vec3 color;    ///< RGB color of the vertex
            glm::vec2 texCoord; ///< Texture coordinates of the vertex

            /**
             * @brief Provides the vertex binding description for Vulkan
             * @return VkVertexInputBindingDescription describing how to bind vertex data
             *
             * Specifies the stride and input rate for the vertex data.
             */
            static VkVertexInputBindingDescription GetBindingDescription()
            {
                VkVertexInputBindingDescription bindingDescription;
                bindingDescription.binding = 0;
                bindingDescription.stride = sizeof(Vertex);
                bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                return bindingDescription;
            }

            /**
             * @brief Provides attribute descriptions for vertex data components
             * @return Array of attribute descriptions for position, color, and texture coordinates
             *
             * Describes how to interpret each attribute (position, color, texture coordinates)
             * within the vertex data, including their locations, formats, and offsets.
             */
            static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
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

            /**
             * @brief Equality comparison operator for vertices
             * @param other The vertex to compare against
             * @return true if vertices are equal, false otherwise
             */
            bool operator==(const Vertex &other) const
            {
                return pos == other.pos && color == other.color && texCoord == other.texCoord;
            }
        };

        /**
         * @brief Creates and initializes the vertex buffer on the GPU
         *
         * Allocates memory for the vertex buffer and transfers vertex data to it.
         * The buffer is created with appropriate usage flags for vertex data.
         */
        [[nodiscard]] Buffer CreateVertexBuffer() const;

    private:
        Ref<GraphicsEngine> *gfxEngine;      ///< Pointer to the graphics engine reference
        Ref<MemoryAllocator> allocator;      ///< Reference to the memory allocator
        std::vector<Vertex> vertices;        ///< Storage for vertex data
        RenderData renderData;
        VkBuffer vertexBuffer;               ///< Handle to the Vulkan vertex buffer
        VkDeviceMemory vertexBufferMemory;   ///< Handle to the allocated memory for the vertex buffer
    };

}

/// ----------------------------------------------------------

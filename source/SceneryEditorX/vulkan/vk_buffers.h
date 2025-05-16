/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_buffers.h
* -------------------------------------------------------
* Created: 5/5/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/vulkan/buffer_data.h>
#include <SceneryEditorX/vulkan/vk_device.h>
#include "vk_swapchain.h"

// --------------------------------------------

namespace SceneryEditorX
{
    /**
     * @brief Creates a Vulkan buffer with specified parameters
     * 
     * @param size Size of the buffer in bytes
     * @param usage Flags specifying how the buffer will be used (vertex, index, uniform, etc.)
     * @param memory Memory type flags indicating where the buffer should be allocated (GPU, CPU, etc.)
     * @param name Optional debug name for the buffer resource
     * @return Buffer A buffer object containing the Vulkan buffer handle and allocation information
     * 
     * This function creates a Vulkan buffer with the specified size and usage flags,
     * allocating it in the appropriate memory type based on the memory parameter.
     * It abstracts away the details of buffer creation and memory allocation in Vulkan.
     */
    Buffer CreateBuffer(uint32_t size, BufferUsageFlags usage, MemoryFlags memory = MemoryType::GPU, const std::string& name = "");
    
    /**
     * @brief Copies data between two Vulkan buffers
     * 
     * @param srcBuffer Source buffer to copy from
     * @param dstBuffer Destination buffer to copy to
     * @param size Number of bytes to copy
     * 
     * This function performs a buffer-to-buffer copy operation through a command buffer,
     * typically used to transfer data from a staging buffer to a device-local buffer.
     * The operation is completed synchronously by creating, executing and destroying
     * a single-use command buffer.
     */
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    
    /**
     * @brief Copies buffer data to an image
     * 
     * @param buffer Source buffer containing image data
     * @param image Destination image to copy data to
     * @param width Width of the image in pixels
     * @param height Height of the image in pixels
     * 
     * This function transfers data from a buffer to an image, typically used when
     * loading texture data. It handles the necessary image layout transitions and
     * submits a copy command to the graphics queue. The image should be in the
     * appropriate layout for a transfer destination operation before calling this function.
     */
    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    /// ----------------------------------------------------------

    /**
     * @class UniformBuffer
     * @brief Manages uniform buffer objects for shader uniforms in Vulkan
     *
     * This class handles the creation, management, and updating of uniform buffers
     * used to pass transformation matrices and other uniform data to shaders.
     * It maintains a separate buffer for each frame in flight to prevent race conditions.
     */
    class UniformBuffer
	{
	public:
        /**
         * @struct UBO
         * @brief Uniform Buffer Object structure for shader uniforms
         *
         * Contains transformation matrices aligned for GPU memory access.
         * The alignas specifier ensures proper memory alignment for GPU access.
         */
		struct UBO
		{
			alignas(16) glm::mat4 model;  ///< Model transformation matrix
			alignas(16) glm::mat4 view;   ///< View transformation matrix
			alignas(16) glm::mat4 proj;   ///< Projection transformation matrix
		};

        /**
         * @brief Constructor for UniformBuffer
         * 
         * Initializes the UniformBuffer instance by getting references to the 
         * required graphics engine components.
         */
        UniformBuffer();

        /**
         * @brief Destructor for UniformBuffer
         *
         * Cleans up resources associated with the uniform buffers.
         */
        ~UniformBuffer();

        /**
         * @brief Creates uniform buffers for each frame in the swap chain
         *
         * Allocates and initializes the uniform buffers used to pass transformation
         * matrices to shaders. Creates one buffer per swap chain image.
         */
        void CreateUniformBuffers();

        /**
         * @brief Creates a Vulkan buffer with specified parameters
         *
         * @param size Size of the buffer in bytes
         * @param usage Buffer usage flags indicating how the buffer will be used
         * @param properties Memory property flags for the buffer allocation
         * @param buffer Reference to store the created buffer handle
         * @param bufferMemory Reference to store the allocated memory handle
         */
        void CreateBuffer(VkDeviceSize size,
                          VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties,
                          VkBuffer &buffer, VkDeviceMemory &bufferMemory) const;

        /**
         * @brief Updates the contents of a uniform buffer for the current frame
         *
         * @param currentImage Index of the current swap chain image/frame
         * 
         * Updates transformation matrices in the uniform buffer for the specified
         * frame with the current camera and model state.
         */
        void UpdateUniformBuffer(uint32_t currentImage) const;

    private:
        Ref<GraphicsEngine> *gfxEngine;						///< Pointer to the graphics engine reference
        Ref<VulkanDevice> vkDevice;							///< Reference to the Vulkan logical device
        Ref<MemoryAllocator> allocator;						///< Reference to the memory allocator
        RenderData renderData;                              ///< Reference to the render data structure
        std::vector<VkBuffer> uniformBuffers;				///< Array of uniform buffer handles (one per frame)
        std::vector<VkDeviceMemory> uniformBuffersMemory;	///< Array of memory handles for uniform buffers

    };

    /// ----------------------------------------------------------

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
        ~VertexBuffer();

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
            static VkVertexInputBindingDescription getBindingDescription()
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
        void CreateVertexBuffer() const;

    private:
        //Ref<GraphicsEngine> *gfxEngine;      ///< Pointer to the graphics engine reference
        Ref<VulkanDevice> vkDevice;          ///< Reference to the Vulkan logical device
        Ref<MemoryAllocator> allocator;      ///< Reference to the memory allocator
        std::vector<Vertex> vertices;        ///< Storage for vertex data
        RenderData renderData;
        VkBuffer vertexBuffer;               ///< Handle to the Vulkan vertex buffer
        VkDeviceMemory vertexBufferMemory;   ///< Handle to the allocated memory for the vertex buffer
    };


    /// ----------------------------------------------------------

    /**
     * @class IndexBuffer
     * @brief Manages index data storage and configuration in Vulkan
     *
     * This class handles the creation and management of index buffers in Vulkan,
     * including memory allocation and transfer of index data to the GPU.
     * Index buffers store the indices that define how vertices are connected
     * to form geometry primitives (triangles).
     */
    class IndexBuffer
    {
    public:
        /**
         * @brief Constructor for IndexBuffer
         *
         * Initializes the IndexBuffer instance by getting references to required
         * graphics engine components.
         */
        IndexBuffer();

        /**
         * @brief Destructor for IndexBuffer
         *
         * Cleans up index buffer resources and associated memory.
         */
        ~IndexBuffer();

        /**
         * @brief Creates and initializes the index buffer on the GPU
         *
         * Allocates memory for the index buffer and transfers index data to it.
         * The buffer is created with appropriate usage flags for index data access
         * during rendering.
         */
        void CreateIndexBuffer() const;

    private:
        Ref<GraphicsEngine> *gfxEngine;		///< Pointer to the graphics engine reference
        Ref<VulkanDevice> vkDevice;			///< Reference to the Vulkan logical device
        Ref<MemoryAllocator> allocator;		///< Reference to the memory allocator
        RenderData renderData;
        std::vector<uint32_t> indices;		///< Storage for index data
        VkBuffer indexBuffer;				///< Handle to the Vulkan index buffer
        VkDeviceMemory indexBufferMemory;	///< Handle to the allocated memory for the index buffer
    };


} // namespace SceneryEditorX

// ----------------------------------------------------------

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
#include <SceneryEditorX/renderer/buffer_data.h>
#include <SceneryEditorX/renderer/image_data.h>
#include <SceneryEditorX/vulkan/vk_device.h>

// --------------------------------------------

namespace SceneryEditorX
{

    Buffer CreateBuffer(uint32_t size, BufferUsageFlags usage, MemoryFlags memory = MemoryType::GPU, const std::string& name = "");
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    /// ----------------------------------------------------------

    class UniformBuffer
	{
	public:

		struct UBO
		{
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view;
			alignas(16) glm::mat4 proj;
		};

        UniformBuffer();
        ~UniformBuffer();

        void CreateUniformBuffers();
        void CreateBuffer(VkDeviceSize size,
                          VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties,
                          VkBuffer &buffer,
                          VkDeviceMemory &bufferMemory) const;
        void UpdateUniformBuffer(uint32_t currentImage) const;

    private:
        Ref<GraphicsEngine> *renderer;
        Ref<VulkanDevice> vkDevice;
        Ref<MemoryAllocator> allocator;
        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;

    };

    /// ----------------------------------------------------------

    class VertexBuffer
    {
    public:
        VertexBuffer();
        ~VertexBuffer();

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
0-
		    bool operator==(const Vertex &other) const
		    {
		        return pos == other.pos && color == other.color && texCoord == other.texCoord;
		    }
		};

        void CreateVertexBuffer();

    private:
        Ref<GraphicsEngine> *renderer;
        Ref<VulkanDevice> vkDevice;
        Ref<MemoryAllocator> allocator;
        std::vector<Vertex> vertices;

        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
    };

    /// ----------------------------------------------------------

    class IndexBuffer
    {
    public:
        IndexBuffer();
        ~IndexBuffer();

        void CreateIndexBuffer();

    private:
        Ref<GraphicsEngine> *renderer;
        Ref<VulkanDevice> vkDevice;
        Ref<MemoryAllocator> allocator;
        std::vector<uint32_t> indices;

        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;

    };

} // namespace SceneryEditorX

// ----------------------------------------------------------

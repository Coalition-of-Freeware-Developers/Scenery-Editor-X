/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vertex_buffer.cpp
* -------------------------------------------------------
* Created: 18/5/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/buffers/vertex_buffer.h>
#include <SceneryEditorX/renderer/vulkan/vk_buffers.h>

/// ---------------------------------------------------------

namespace SceneryEditorX
{

	/*
	VertexBuffer::VertexBuffer(VertexBufferType type, VertexFormat vertexFormat, uint32_t initialCapacity)
	{
	}
	*/

    /*
	VertexBuffer::VertexBuffer(const std::vector<Vertex> &initialVertices, VertexBufferType type)
	{
	}
	*/

    /**
     * @brief Constructor for VertexBuffer with raw data.
     *
     * @param data Pointer to vertex data.
     * @param size Size of the data in bytes.
     * @param usage Buffer usage type (default: Static).
     */
    VertexBuffer::VertexBuffer(const void *data, uint64_t size, VertexBufferType usage) : m_Size(size)
    {
        m_LocalData.Allocate(size);
        if (data)
            m_LocalData.Write(data, size, 0);

        /// Create a shared reference to this instance for the render thread
        Ref<VertexBuffer> instance(this);
        Renderer::Submit([instance]() mutable {
            auto device = RenderContext::GetCurrentDevice();
            MemoryAllocator allocator("VertexBuffer");

            VkBufferCreateInfo vertexBufferCreateInfo = {};
            vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            vertexBufferCreateInfo.size = instance->m_Size;
            vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

            instance->m_MemoryAllocation = allocator.AllocateBuffer(vertexBufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, instance->m_VulkanBuffer);

        });
    }

    /*
    Ref<VertexBuffer> VertexBuffer::CreateBuffer(VertexBufferType type, VertexFormat vertexFormat, uint32_t initialCapacity)
    {
        return CreateRef<VertexBuffer>(CreateBuffer(type, vertexFormat, initialCapacity));
    }
    */

    /**
     * @brief Destructor for VertexBuffer
     *
     * Cleans up the Vulkan vertex buffer and its associated memory allocation.
     * This destructor is responsible for releasing resources allocated for the vertex buffer
     * across all frames in flight. It ensures that the Vulkan buffer is properly destroyed
     * and the memory allocation is freed to prevent memory leaks.
     *
     */
    VertexBuffer::~VertexBuffer()
    {
        for (size_t i = 0; i < renderData.framesInFlight; i++)
            allocator->DestroyBuffer(vertexBuffer, vertexBuffersAllocation);
    }

    void VertexBuffer::SetData(const std::vector<Vertex> &newVertices, bool recreateBuffer)
    {
    }

    void VertexBuffer::AppendData(const std::vector<Vertex> &additionalVertices, bool recreateBuffer)
    {
    }

    void VertexBuffer::UpdateData(uint32_t startIndex, const std::vector<Vertex> &updatedVertices)
    {
    }

    void VertexBuffer::ClearData(bool releaseBuffer)
    {
    }

    void VertexBuffer::Reserve(uint32_t newCapacity, bool preserveData)
    {
    }

    VkVertexInputBindingDescription VertexBuffer::GetBindingDescription(uint32_t binding, VkVertexInputRate inputRate) const
    {
        return {};
    }

    std::vector<VkVertexInputAttributeDescription> VertexBuffer::GetAttributeDescriptions(uint32_t binding) const
    {
        return {};
    }

    //TODO: Move to primitive creation code. -> renderer/primitives.cpp
    /*
    Ref<VertexBuffer> VertexBuffer::CreatePrimitive(PrimitiveType type, const Vec3 &size, const Vec3 &color)
    {
        return {};
    }
	*/

    std::vector<VkVertexInputAttributeDescription> VertexBuffer::CreateAttributeDescriptions(uint32_t binding) const
    {
        return {};
    }

}

/// ---------------------------------------------------------

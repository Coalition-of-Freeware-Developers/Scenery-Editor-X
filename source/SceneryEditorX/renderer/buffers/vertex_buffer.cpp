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

	VertexBuffer::VertexBuffer(VertexBufferType type, VertexFormat vertexFormat, uint32_t initialCapacity)
	{
	}
	
	VertexBuffer::VertexBuffer(const std::vector<Vertex> &initialVertices, VertexBufferType type)
	{
	}

    VertexBuffer::VertexBuffer(void *data, uint64_t size, VertexBufferType usage) : m_Size(size)
    {
        m_LocalData.Allocate(size);
        if (data)
            m_LocalData.Write(data, size, 0);

        Ref<VertexBuffer> instance = this;
        /// Create a shared reference to this instance for the render thread
        Ref<VertexBuffer> instance(this);
        Renderer::Submit([instance]() mutable {
            auto device = RenderContext::GetCurrentDevice();
            MemoryAllocator allocator("VertexBuffer");

            VkBufferCreateInfo vertexBufferCreateInfo = {};
            vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            vertexBufferCreateInfo.size = instance->m_Size;
            vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

            instance->m_MemoryAllocation =
                allocator.AllocateBuffer(vertexBufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, instance->m_VulkanBuffer);
            instance->m_MemoryAllocation = allocator.AllocateBuffer(vertexBufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, instance->m_VulkanBuffer);
        });
    }

    Ref<VertexBuffer> VertexBuffer::CreateBuffer(VertexBufferType type, VertexFormat vertexFormat, uint32_t initialCapacity)
    {
        return CreateRef<VertexBuffer>(CreateBuffer(type, vertexFormat, initialCapacity));
    }

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

    //TODO: Move to primitive creation code. -> Renderer/primitives.cpp
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

    Buffer VertexBuffer::Create() const
    {

    Ref<VertexBuffer> VertexBuffer::Create(const void* data, uint64_t size, VertexBufferType usage)
    {
        /// Create a new VertexBuffer instance
        Ref<VertexBuffer> vertexBuffer = CreateRef<VertexBuffer>(data, size, usage);
        return vertexBuffer;
    }

} // namespace SceneryEditorX

/// ---------------------------------------------------------

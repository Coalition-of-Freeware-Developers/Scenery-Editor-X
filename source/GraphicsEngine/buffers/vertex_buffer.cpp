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
#include <GraphicsEngine/buffers/buffer_data.h>
#include <GraphicsEngine/buffers/vertex_buffer.h>
#include <GraphicsEngine/vulkan/vk_buffers.h>

/// ---------------------------------------------------------

namespace SceneryEditorX
{

	VertexBuffer::VertexBuffer(VertexBufferType type, VertexFormat vertexFormat, uint32_t initialCapacity)
	{
	}
	
	VertexBuffer::VertexBuffer(const std::vector<Vertex> &initialVertices, VertexBufferType type)
	{
	}
	
	VertexBuffer::~VertexBuffer()
    {
        for (size_t i = 0; i < RenderData::framesInFlight; i++)
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

    void * VertexBuffer::MapMemory()
    {
        return nullptr;
    }

    void VertexBuffer::UnmapMemory()
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

    Ref<VertexBuffer> VertexBuffer::CreatePrimitive(PrimitiveType type, const Vec3 &size, const Vec3 &color)
    {
        return {};
    }

    void VertexBuffer::Initialize()
    {
    }

    void VertexBuffer::Release()
    {
    }

    std::vector<VkVertexInputAttributeDescription> VertexBuffer::CreateAttributeDescriptions(uint32_t binding) const
    {
        return {};
    }

    Buffer VertexBuffer::CreateVertexBuffer() const
    {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        VkBuffer stagingBuffer = nullptr;
        VmaAllocation stagingBufferAllocation = nullptr;

	    /// --------------------------------------

        CreateBuffer(bufferSize, BufferUsage::TransferSrc, MemoryType::CPU, "VertexStaging#");

        void* data = allocator->MapMemory<void>(stagingBufferAllocation);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        allocator->UnmapMemory(stagingBufferAllocation);

		/// --------------------------------------

        CreateBuffer(bufferSize, BufferUsage::Vertex | BufferUsage::TransferDst, MemoryType::GPU, "VertexBuffer#");

        CopyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		/// --------------------------------------

        allocator->DestroyBuffer(stagingBuffer, stagingBufferAllocation);
        return {};
    }

} // namespace SceneryEditorX

/// ---------------------------------------------------------

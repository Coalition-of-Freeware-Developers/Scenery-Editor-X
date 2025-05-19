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
#include <SceneryEditorX/vulkan/buffers/buffer_data.h>
#include <SceneryEditorX/vulkan/buffers/vertex_buffer.h>
#include <SceneryEditorX/vulkan/vk_buffers.h>
#include <SceneryEditorX/vulkan/vk_device.h>

/// ---------------------------------------------------------

namespace SceneryEditorX
{
	
    VertexBuffer::VertexBuffer()
    {
		//CreateVertexBuffer();
    }

    VertexBuffer::~VertexBuffer()
    {
        const VkDevice vkDevice = gfxEngine->get()->GetLogicDevice()->GetDevice();

        for (size_t i = 0; i < RenderData::framesInFlight; i++)
		{
            vkDestroyBuffer(vkDevice, vertexBuffer, nullptr);
            vkFreeMemory(vkDevice, vertexBufferMemory, nullptr);
		}
    }

    Buffer VertexBuffer::CreateVertexBuffer() const
    {
        const VkDevice vkDevice = gfxEngine->get()->GetLogicDevice()->GetDevice();

        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        VkBuffer stagingBuffer = nullptr;
        VkDeviceMemory stagingBufferMemory = nullptr;

	    /// --------------------------------------

        CreateBuffer(bufferSize, BufferUsage::TransferSrc, MemoryType::CPU, "VertexStaging#");

        void *data;
        vkMapMemory(vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(vkDevice, stagingBufferMemory);

		/// --------------------------------------

        CreateBuffer(bufferSize, BufferUsage::Vertex | BufferUsage::TransferDst, MemoryType::GPU, "VertexBuffer#");

        CopyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		/// --------------------------------------

        vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
        vkFreeMemory(vkDevice, stagingBufferMemory, nullptr);
        return {};
    }

} // namespace SceneryEditorX

/// ---------------------------------------------------------

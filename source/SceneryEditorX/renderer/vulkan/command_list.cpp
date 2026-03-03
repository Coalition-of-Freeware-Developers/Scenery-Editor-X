/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * command_list.cpp
 * -------------------------------------------------------
 * Created: 12/02/2026
 * -------------------------------------------------------
 */
#include "command_list.h"
#include "buffer.h"
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	CommandList::CommandList(Queue *queue, CommandPool cmdPool, const char *name)
	{
        m_Queue = queue;

	}
	
	CommandList::~CommandList()
	{
	}
	
	void CommandList::Begin()
	{
	}
	
	void CommandList::Submit(FrameSync *semaphoreWait, const bool isImmediate, FrameSync *semaphoreSignal)
	{
	}
	
	void CommandList::WaitForExecution(const bool logWaitTime)
	{
	}

    void CommandList::SetVertexBuffer(const Buffer *vertexBuffer,  Buffer* instance)
    {
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command List must be in Recording state to set vertex buffer.");
        SEDX_CORE_ASSERT(m_CmdBuffer != VK_NULL_HANDLE, "Command List has no active VkCommandBuffer");
        SEDX_CORE_ASSERT(vertexBuffer != nullptr, "Vertex buffer must be valid");
        SEDX_CORE_ASSERT(instance != nullptr, "Instance buffer must be valid");

        VkBuffer vertex_buffers[2] = {
            vertexBuffer->Get(),
            instance->Get(),
        };
        SEDX_CORE_ASSERT(vertex_buffers[0] != VK_NULL_HANDLE && vertex_buffers[1] != VK_NULL_HANDLE,
            "Vertex and instance buffers must be valid Vulkan handles");

        VkDeviceSize offsets[2] = {0, 0};

        vkCmdBindVertexBuffers(m_CmdBuffer, 0, 2, vertex_buffers, offsets);
    }

    void CommandList::Draw(const uint32_t vertexCount, const uint32_t vertexOffset)
    {
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command List must be in Recording state to issue draw calls.");

        vkCmdDraw(m_CmdBuffer, vertexCount, 1, vertexOffset, 0);
    }

    void CommandList::DrawIndexed(const uint32_t indexCount, const uint32_t instCount, const uint32_t indexOffset, const uint32_t vertexOffset, const uint32_t instIndex)
    {
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command List must be in Recording state to issue draw calls.");

        vkCmdDrawIndexed(m_CmdBuffer, indexCount, instCount, indexOffset, static_cast<int32_t>(vertexOffset), instIndex);
    }

    void CommandList::SetViewport(const Viewport &viewport) const
    {
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to set viewport");

        VkViewport vkViewport{};
        vkViewport.x = viewport.x;
        vkViewport.y = viewport.y;
        vkViewport.width = viewport.width;
        vkViewport.height = viewport.height;
        vkViewport.minDepth = viewport.depth_min;
        vkViewport.maxDepth = viewport.depth_max;
        vkCmdSetViewport(m_CmdBuffer, 0, 1, &vkViewport);
    }

    void CommandList::SetScissor(const xMath::Rectangle &scissorRect) const
    {
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to set scissor");

        VkRect2D scissor{};
        scissor.offset.x = static_cast<int32_t>(scissorRect.x);
        scissor.offset.y = static_cast<int32_t>(scissorRect.y);
        scissor.extent.width = static_cast<uint32_t>(scissorRect.width);
        scissor.extent.height = static_cast<uint32_t>(scissorRect.height);
        vkCmdSetScissor(m_CmdBuffer, 0, 1, &scissor);
    }

    void CommandList::SetCullMode(const VkCullModeFlags cullMode)
    {
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to set cull mode");

        m_CullMode = cullMode;
        vkCmdSetCullMode(m_CmdBuffer, cullMode);
    }

    void CommandList::CopyImageToBuffer(Image *src, Buffer *dst)
    {
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to copy image to buffer");
        SEDX_CORE_ASSERT(src != nullptr && dst != nullptr, "Source image and destination buffer must be valid");

        src->SetLayout(Layout::ImageLayout::TransferSrc, this);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = GetAspectMask(src->GetFormat());
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { src->GetWidth(), src->GetHeight(), 1 };

        vkCmdCopyImageToBuffer(m_CmdBuffer, src->GetResource(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->Get(), 1, &region);
    }

    void CommandList::CopyBufferToBuffer(void *src, Buffer *dst, uint64_t size)
    {
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to copy buffer data");
        SEDX_CORE_ASSERT(src != nullptr && dst != nullptr && size > 0, "Source, destination, and size must be valid");

        VkBufferCopy region{};
        region.size = size;
        const VkBuffer srcBuffer = *reinterpret_cast<VkBuffer *>(src);
        vkCmdCopyBuffer(m_CmdBuffer, srcBuffer, dst->Get(), 1, &region);
    }

    void CommandList::CopyBufferToBuffer(Buffer *src, Buffer *dst, uint64_t size)
    {
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to copy buffers");
        SEDX_CORE_ASSERT(src != nullptr && dst != nullptr && size > 0, "Source, destination, and size must be valid");

        VkBufferCopy region{};
        region.size = size;
        vkCmdCopyBuffer(m_CmdBuffer, src->Get(), dst->Get(), 1, &region);
    }

} // namespace SceneryEditorX

// -------------------------------------------------------

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
#include "graphics_debug.h"
#include "render_context.h"
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

    #pragma region Static Command Actions

    static VkImageMemoryBarrier2 CreateImageMemoryBarrier(void* img, const VkAccessFlags& srcMask, const VkAccessFlags& dstMask,
		const VkImageLayout& oldLayout, const VkImageLayout& newLayout, const std::optional<VkImageSubresourceRange>& subresourceRange)
	{
		VkImageMemoryBarrier2 barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barrier.pNext = nullptr;
		barrier.srcAccessMask = srcMask;
		barrier.dstAccessMask = dstMask;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = *static_cast<VkImage*>(img);
		
		if (subresourceRange.has_value())
		{
		    const VkImageSubresourceRange& currentSubresourceRange = subresourceRange.value();
			barrier.subresourceRange.aspectMask = currentSubresourceRange.aspectMask;
			barrier.subresourceRange.baseMipLevel = currentSubresourceRange.baseMipLevel;
			barrier.subresourceRange.levelCount = currentSubresourceRange.levelCount;
			barrier.subresourceRange.baseArrayLayer = currentSubresourceRange.baseArrayLayer;
			barrier.subresourceRange.layerCount = currentSubresourceRange.layerCount;
		}
        else
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
        }

		return barrier;
    }

    static Layout::ImageLayout GetImageLayoutType(const VkImageLayout& layout)
	{
		switch (layout)
		{
			case VK_IMAGE_LAYOUT_UNDEFINED: return Layout::ImageLayout::Undefined;
			case VK_IMAGE_LAYOUT_GENERAL: return Layout::ImageLayout::General;
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: return Layout::ImageLayout::ColorAttachment;
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: return Layout::ImageLayout::DepthAttachmentStencilRead;
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL: return Layout::ImageLayout::DepthStencilRead;
			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: return Layout::ImageLayout::ShaderRead;
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: return Layout::ImageLayout::TransferSrc;
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: return Layout::ImageLayout::TransferDst;
			default:
				SEDX_CORE_ASSERT(false, "Unsupported image layout!");
				return Layout::ImageLayout::Undefined; // Fallback
		}
    }

    static VkImageLayout GetVkImageLayout(const Layout::ImageLayout &layout)
    {
        switch (layout)
        {
            case Layout::ImageLayout::Undefined:					return VK_IMAGE_LAYOUT_UNDEFINED;
            case Layout::ImageLayout::General:						return VK_IMAGE_LAYOUT_GENERAL;
            case Layout::ImageLayout::ColorAttachment:				return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            case Layout::ImageLayout::DepthStencilAttachment:		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            case Layout::ImageLayout::DepthStencilRead:				return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            case Layout::ImageLayout::ShaderRead:					return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            case Layout::ImageLayout::TransferSrc:					return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            case Layout::ImageLayout::TransferDst:					return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            case Layout::ImageLayout::DepthReadStencilAttachment:	return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
            case Layout::ImageLayout::DepthAttachmentStencilRead:	return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
            case Layout::ImageLayout::FragmentShadingRate:			return VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
            case Layout::ImageLayout::DepthAttachment:				return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            case Layout::ImageLayout::DepthRead:					return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
            case Layout::ImageLayout::StencilAttachment:			return VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
            case Layout::ImageLayout::StencilRead:					return VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
            case Layout::ImageLayout::Read:							return VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
            case Layout::ImageLayout::Attachment:					return VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            case Layout::ImageLayout::Present:						return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            case Layout::ImageLayout::MaxEnum:						return VK_IMAGE_LAYOUT_MAX_ENUM;
        }

        return VK_IMAGE_LAYOUT_MAX_ENUM;
    }

    #pragma endregion

    // -------------------------------------------------------

	CommandList::CommandList(Queue *queue, const CommandPool &cmdPool, const char *name) : InheritanceBundle<RefCounted, IResource>(ResourceType::CommandList)
    {
        m_Queue = queue;

        // Command Buffer
        {
            Ref<Device> device = RenderContext::Get()->GetDevice();
            // define
            VkCommandBufferAllocateInfo allocateInfo = {};
            allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocateInfo.commandPool = cmdPool.GetPool();
            allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocateInfo.commandBufferCount = 1;

            // allocate
            SEDX_VK_RESULT_ASSERT(vkAllocateCommandBuffers(device->GetLogicalDevice(), &allocateInfo, &m_CmdBuffer), "Failed to allocate command buffers");

            // name
            Debugging::SetResourceName(m_CmdBuffer, ResourceType::CommandList, name);
            m_ObjectName = name;
        }
    }

    CommandList::~CommandList()
	{
	}
	
	void CommandList::Begin()
	{
        SEDX_CORE_ASSERT(m_State == CommandState::Idle, "Command list must be in idle state to begin recording.");
     
        // Begin Command buffer
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        SEDX_CORE_ASSERT(vkBeginCommandBuffer(m_CmdBuffer, &beginInfo) == VK_SUCCESS, "Failed to begin command buffer");

        // Set states
        m_State     = CommandState::Recording;
        //m_Pso  = PipelineState();
        m_CullMode  = VK_CULL_MODE_BACK_BIT;
    
        // set dynamic states
        if (m_Queue->GetType() == QueueType::Graphics)
        {
            // Cull mode
            SetCullMode(VK_CULL_MODE_BACK_BIT);
    
			// Scissor Rectangle
            /*
            xMath::Rectangle scissorRect;
            scissorRect.x      = 0.0f;
            scissorRect.y      = 0.0f;
            scissorRect.width  = static_cast<float>(m_pso.GetWidth());
            scissorRect.height = static_cast<float>(m_pso.GetHeight());
            SetScissor(scissorRect);*/
        }
    
	}
	
	void CommandList::Submit(FrameSync *semaphoreWait, const bool isImmediate, FrameSync *semaphoreSignal)
	{
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to submit.");

        // end recording: flush any pending layout transitions, then close the command buffer
        EndRenderPass();
        //FlushBarriers();
        SEDX_VK_RESULT_ASSERT(vkEndCommandBuffer(m_CmdBuffer), "Failed to end command buffer");

        // only signal a binary semaphore when explicitly provided (e.g. swapchain present).
        // the cmd list's own binary semaphore is not auto-signaled because with multi-submit
        // per frame (async compute overlap), intermediate submits would signal it repeatedly
        // without a corresponding wait, violating the vulkan spec for binary semaphores.
		FrameSync *signal = semaphoreSignal;

		/*
		m_Queue->Submit(m_CmdBuffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,	// Wait flags
			semaphoreWait,						// Wait Semaphore (binary)
			signal,								// Signal Semaphore (binary)
			semaphoreSignal,					// Signal Semaphore (Timeline)
            semaphoreTimelineWait,              // Wait Semaphore (timeline, for cross-queue synchronization)
            timelineWait);                      // Value to wait on for timeline semaphore (if used)
        */

        if (semaphoreWait)
        {
            semaphoreWait->SetUserCmdList(this);
        }

        m_State = CommandState::Submitted;
	}
	
	void CommandList::WaitForExecution(const bool logWaitTime)
	{
	}

    void CommandList::ClearDepth(void *img, float clearDepth)
    {
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command List must be in Recording state to clear texture.");
		SEDX_CORE_ASSERT(img != nullptr, "Must have a valid image.");

        // Transition image to transfer dst layout
		InsertBarrier(img, Layout::ImageLayout::TransferDst);

        // Define the range of the image to clear (full image)
		VkImageSubresourceRange range{};
        range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		range.baseMipLevel = 0;
		range.levelCount = VK_REMAINING_MIP_LEVELS;
		range.baseArrayLayer = 0;
		range.layerCount = VK_REMAINING_ARRAY_LAYERS;

        VkClearDepthStencilValue clearDepthStencil;
		clearDepthStencil.depth = clearDepth;
		clearDepthStencil.stencil = 0;

        VkImage vkImage = *reinterpret_cast<VkImage *>(img);

		vkCmdClearDepthStencilImage(m_CmdBuffer, vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearDepthStencil, 1, &range);
    }

    void CommandList::ClearStencil(void *img, uint32_t clearStencil)
    {
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command List must be in Recording state to clear texture.");
		SEDX_CORE_ASSERT(img != nullptr, "Must have a valid image.");

        // Transition image to transfer dst layout
		InsertBarrier(img, Layout::ImageLayout::TransferDst);

		VkImageSubresourceRange range{};
		range.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
		range.baseMipLevel = 0;
		range.levelCount = VK_REMAINING_MIP_LEVELS;
		range.baseArrayLayer = 0;
		range.layerCount = VK_REMAINING_ARRAY_LAYERS;

        VkClearDepthStencilValue clearDepthStencil;
		clearDepthStencil.depth = 0;
		clearDepthStencil.stencil = clearStencil;

        VkImage vkImage = *reinterpret_cast<VkImage *>(img);

		vkCmdClearDepthStencilImage(m_CmdBuffer, vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearDepthStencil, 1, &range);
    }

    void CommandList::ClearTexture(void *img, const Color &color)
    {
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command List must be in Recording state to clear texture.");
		SEDX_CORE_ASSERT(img != nullptr, "Must have a valid image.");

		// Transition image to transfer dst layout
		InsertBarrier(img, Layout::ImageLayout::TransferDst);

        // Define the range of the image to clear (full image)
		VkImageSubresourceRange range;
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Assuming it's a color image
		range.baseMipLevel = 0;
		range.levelCount = VK_REMAINING_MIP_LEVELS;
		range.baseArrayLayer = 0;
		range.layerCount = VK_REMAINING_ARRAY_LAYERS;

		VkClearColorValue clearColor = {};
		clearColor.float32[0] = color.r;
		clearColor.float32[1] = color.g;
		clearColor.float32[2] = color.b;
		clearColor.float32[3] = color.a;

		VkImage vkImage = *reinterpret_cast<VkImage *>(img);

		// Clear the image
        vkCmdClearColorImage(m_CmdBuffer, vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);
    }

    void CommandList::SetVertexBuffer(const Buffer *vertexBuffer, const Buffer *instance)
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

    void CommandList::EndRenderPass()
    {
        if (!m_RenderPassActive)
            return;

        vkCmdEndRendering(m_CmdBuffer);
        m_RenderPassActive = false;
    }

    void CommandList::InsertBarrier(void *img, Layout::ImageLayout layout, uint32_t mip, uint32_t mipRange)
    {
        //CreateImageMemoryBarrier(img, 0, 0, GetVkImageLayout(layout), layout, std::nullopt);

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

    void CommandList::Dispatch(uint32_t x, uint32_t y, uint32_t z /*= 1*/)
    {
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to dispatch compute work");

        //PreDraw();

        vkCmdDispatch(m_CmdBuffer, x, y, z);
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

        VkRect2D scissor;
        scissor.offset.x = static_cast<int32_t>(scissorRect.x);
        scissor.offset.y = static_cast<int32_t>(scissorRect.y);
        scissor.extent.width = static_cast<uint32_t>(scissorRect.width);
        scissor.extent.height = static_cast<uint32_t>(scissorRect.height);
        vkCmdSetScissor(m_CmdBuffer, 0, 1, &scissor);
    }

    void CommandList::SetCullMode(const VkCullModeFlags cullMode)
    {
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to set cull mode");
        SEDX_CORE_ASSERT(cullMode != 0, "Vulkan cull mode must be valid");

        m_CullMode = cullMode;
        vkCmdSetCullMode(m_CmdBuffer, cullMode);
    }

    /*
    void CommandList::CopyImageToBuffer(void *src, Buffer *dst)
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
    */

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

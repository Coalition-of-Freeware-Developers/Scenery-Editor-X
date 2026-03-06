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
#include "image_resource.h"
#include "render_context.h"
#include "swapchain.h"
#include "asset/texture_image.h"
#include "debug/graphics_debug.h"
#include "SceneryEditorX/renderer/renderer.h"
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
    /// Per-image layout tracking: key = &VkImage (stable address), value = current layout.
    /// Protected by s_ImageLayoutsMutex for safe concurrent reads from multiple threads.
    static std::unordered_map<void*, Layout::ImageLayout> s_ImageLayouts;
    static std::mutex s_ImageLayoutsMutex;

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

	static VkCullModeFlags GetCullingType(const CullMode cullMode)
	{
		switch (cullMode)
		{
			case CullMode::None:	return VK_CULL_MODE_NONE;
			case CullMode::Front:	return VK_CULL_MODE_FRONT_BIT;
			case CullMode::Back:	return VK_CULL_MODE_BACK_BIT;
			case CullMode::All:		return VK_CULL_MODE_FRONT_AND_BACK;
			case CullMode::MaxEnum:
				SEDX_CORE_ERROR_TAG("Command List","Invalid cull mode!");
				break;
		}

		SEDX_CORE_WARN_TAG("Command List","Invalid cull mode! Defaulting to backface cull mode.");
		return VK_CULL_MODE_BACK_BIT; // Fallback
	}

	/**
	 * @brief Derive the VkImageAspectFlags for a given Vulkan image format.
	 * Depth-only formats return DEPTH_BIT, combined depth/stencil return both, stencil-only returns STENCIL_BIT,
	 * and all other (colour) formats return COLOR_BIT.
	 */
	static VkImageAspectFlags GetAspectMaskFromFormat(const VkFormat format)
	{
		switch (format)
		{
			case VK_FORMAT_D16_UNORM:
			case VK_FORMAT_D32_SFLOAT:
			case VK_FORMAT_X8_D24_UNORM_PACK32:
				return VK_IMAGE_ASPECT_DEPTH_BIT;
			case VK_FORMAT_D16_UNORM_S8_UINT:
			case VK_FORMAT_D24_UNORM_S8_UINT:
			case VK_FORMAT_D32_SFLOAT_S8_UINT:
				return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			case VK_FORMAT_S8_UINT:
				return VK_IMAGE_ASPECT_STENCIL_BIT;
			default:
				return VK_IMAGE_ASPECT_COLOR_BIT;
		}
	}

	/**
	 * @brief Maps a Layout::ImageLayout to the corresponding Vulkan pipeline stage and access masks.
	 * Used internally by InsertBarrier to build correct VkImageMemoryBarrier2 entries.
	 */
	struct BarrierAccessInfo
	{
		VkAccessFlags2       accessMask;
		VkPipelineStageFlags2 stageFlags;
	};

	static BarrierAccessInfo GetLayoutAccessInfo(const Layout::ImageLayout layout)
	{
		switch (layout)
		{
			case Layout::ImageLayout::Undefined:
				return { VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT };
			case Layout::ImageLayout::General:
				return { VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
						 VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT };
			case Layout::ImageLayout::ColorAttachment:
			case Layout::ImageLayout::Attachment:
				return { VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
						 VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT };
			case Layout::ImageLayout::DepthStencilAttachment:
			case Layout::ImageLayout::DepthAttachment:
			case Layout::ImageLayout::StencilAttachment:
				return { VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
						 VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT };
			case Layout::ImageLayout::DepthStencilRead:
			case Layout::ImageLayout::DepthRead:
			case Layout::ImageLayout::StencilRead:
			case Layout::ImageLayout::DepthReadStencilAttachment:
			case Layout::ImageLayout::DepthAttachmentStencilRead:
				return { VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
						 VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT };
			case Layout::ImageLayout::ShaderRead:
			case Layout::ImageLayout::Read:
				return { VK_ACCESS_2_SHADER_READ_BIT,
						 VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
						 VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT };
			case Layout::ImageLayout::TransferSrc:
				return { VK_ACCESS_2_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT };
			case Layout::ImageLayout::TransferDst:
				return { VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT };
			case Layout::ImageLayout::Present:
				return { VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT };
			case Layout::ImageLayout::FragmentShadingRate:
				return { VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR,
						 VK_PIPELINE_STAGE_2_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR };
			default:
				return { VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
						 VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT };
		}
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

		m_RenderingCompleteSemaphore = CreateRef<FrameSync>(SyncType::Semaphore);
		m_RenderingCompleteTimeline  = CreateRef<FrameSync>(SyncType::SemaphoreTimeline);
		m_SubmitSync                 = CreateRef<FrameSync>(SyncType::Fence);

		// TODO: Initialize Vulkan query pools for GPU timestamping and occlusion queries, setting up the necessary resources and filling them with initial data.

    }

	CommandList::~CommandList()
	{
		/* TODO: Responsible for cleaning up and deallocating resources associated with query pools by adding them to the deletion queue. */
	}

	Layout::ImageLayout CommandList::GetImageLayout(void *image, uint32_t mipIndex)
	{
		std::lock_guard<std::mutex> lock(s_ImageLayoutsMutex);
		const auto it = s_ImageLayouts.find(image);
		if (it != s_ImageLayouts.end())
			return it->second;
		return Layout::ImageLayout::Undefined;
	}

	void CommandList::RemoveLayout(void *image)
	{
		std::lock_guard<std::mutex> lock(s_ImageLayoutsMutex);
		s_ImageLayouts.erase(image);
	}
	
	void CommandList::Begin()
	{
		SEDX_CORE_ASSERT(m_State == CommandState::Idle, "Command list must be in idle state to begin recording.");

		// ONE_TIME_SUBMIT hints to the driver that this recording will be submitted exactly once
		// before being reset, allowing internal optimizations on tiling/mobile GPU architectures.
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		SEDX_CORE_ASSERT(vkBeginCommandBuffer(m_CmdBuffer, &beginInfo) == VK_SUCCESS, "Failed to begin command buffer");

		m_State           = CommandState::Recording;
		m_RenderPassActive = false;

		// Set common dynamic state for graphics queues so every pass starts from a known baseline.
		if (m_Queue->GetType() == QueueType::Graphics)
		{
			// Backface culling is the safe default; individual passes override as needed.
			SetCullMode(CullMode::Back);

			// Default viewport and scissor cover the full swapchain extent.
			// Passes that render to off-screen targets must override these before drawing.
			if (const Swapchain *swapchain = Renderer::GetSwapChain())
			{
				const VkExtent2D extent = swapchain->GetExtent();
				if (extent.width > 0 && extent.height > 0)
				{
					Viewport vp{};
					vp.x         = 0.0f;
					vp.y         = 0.0f;
					vp.width     = static_cast<float>(extent.width);
					vp.height    = static_cast<float>(extent.height);
					vp.depth_min = 0.0f;
					vp.depth_max = 1.0f;
					SetViewport(vp);

					xMath::Rectangle scissorRect{};
					scissorRect.x      = 0.0f;
					scissorRect.y      = 0.0f;
					scissorRect.width  = static_cast<float>(extent.width);
					scissorRect.height = static_cast<float>(extent.height);
					SetScissor(scissorRect);
				}
			}
		}
	}
	
	void CommandList::Submit(FrameSync *semaphoreWait, const bool isImmediate, FrameSync *semaphoreSignal, FrameSync *semaphoreTimeline, uint64_t timelineValue)
	{
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to submit.");

		// End recording: close any open render pass, then seal the command buffer.
		EndRenderPass();
		SEDX_VK_RESULT_ASSERT(vkEndCommandBuffer(m_CmdBuffer), "Failed to end command buffer");

		// -------------------------------------------------------
		// Build wait semaphore list
		// -------------------------------------------------------
		std::vector<VkSemaphoreSubmitInfo> waitInfos;

		// Binary wait — e.g. swapchain image-acquired semaphore
		if (semaphoreWait && semaphoreWait->GetVkSemaphore() != VK_NULL_HANDLE)
		{
			VkSemaphoreSubmitInfo info{};
			info.sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
			info.semaphore = semaphoreWait->GetVkSemaphore();
			info.value     = 0; // 0 = binary semaphore
			info.stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
			waitInfos.push_back(info);
		}

		// Timeline wait — cross-queue dependency (e.g. g-buffer ready before async compute)
		if (semaphoreTimeline && semaphoreTimeline->GetVkSemaphore() != VK_NULL_HANDLE && timelineValue > 0)
		{
			VkSemaphoreSubmitInfo info{};
			info.sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
			info.semaphore = semaphoreTimeline->GetVkSemaphore();
			info.value     = timelineValue;
			info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			waitInfos.push_back(info);
		}

		// -------------------------------------------------------
		// Build signal semaphore list
		// -------------------------------------------------------
		std::vector<VkSemaphoreSubmitInfo> signalInfos;

		// Always advance and signal the command list's own timeline semaphore so that
		// dependent queues can wait on this submission's completion.
		const uint64_t nextTimelineValue = m_RenderingCompleteTimeline->GetNextSignalValue();
		{
			VkSemaphoreSubmitInfo info{};
			info.sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
			info.semaphore = m_RenderingCompleteTimeline->GetVkSemaphore();
			info.value     = nextTimelineValue;
			info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			signalInfos.push_back(info);
		}

		// Optional binary signal — only when explicitly requested (e.g. swapchain present chain).
		// Binary semaphores must not be signaled redundantly across multi-submit frames.
		if (semaphoreSignal && semaphoreSignal->GetVkSemaphore() != VK_NULL_HANDLE)
		{
			VkSemaphoreSubmitInfo info{};
			info.sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
			info.semaphore = semaphoreSignal->GetVkSemaphore();
			info.value     = 0; // 0 = binary semaphore
			info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			signalInfos.push_back(info);
		}

		// -------------------------------------------------------
		// Command buffer info
		// -------------------------------------------------------
		VkCommandBufferSubmitInfo cmdInfo{};
		cmdInfo.sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		cmdInfo.commandBuffer = m_CmdBuffer;
		cmdInfo.deviceMask    = 0;

		// -------------------------------------------------------
		// Assemble and submit
		// -------------------------------------------------------
		VkSubmitInfo2 submitInfo{};
		submitInfo.sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submitInfo.waitSemaphoreInfoCount   = static_cast<uint32_t>(waitInfos.size());
		submitInfo.pWaitSemaphoreInfos      = waitInfos.data();
		submitInfo.commandBufferInfoCount   = 1;
		submitInfo.pCommandBufferInfos      = &cmdInfo;
		submitInfo.signalSemaphoreInfoCount = static_cast<uint32_t>(signalInfos.size());
		submitInfo.pSignalSemaphoreInfos    = signalInfos.data();

		// For immediate (synchronous) submissions, attach the per-cmdlist fence so we can
		// CPU-block in this call.  Reset the fence first since it starts signaled.
		const VkFence fence = isImmediate ? m_SubmitSync->GetVkFence() : VK_NULL_HANDLE;
		if (fence != VK_NULL_HANDLE)
			Fence::Reset(fence);

		m_Queue->Submit(submitInfo, fence);

		// Record which command list produced the binary wait semaphore's signal
		if (semaphoreWait)
			semaphoreWait->SetUserCmdList(this);

		m_State = CommandState::Submitted;

		// Block the CPU until the GPU finishes if immediate mode was requested.
		if (isImmediate && fence != VK_NULL_HANDLE)
		{
			Fence::Wait(UINT64_MAX, fence);
			Fence::Reset(fence);
			m_State = CommandState::Idle;
		}
	}
	
	void CommandList::WaitForExecution(const bool logWaitTime)
	{
		if (m_State != CommandState::Submitted)
			return;

		SEDX_CORE_ASSERT(m_RenderingCompleteTimeline != nullptr, "Timeline semaphore must be initialized");

		const VkSemaphore semaphore = m_RenderingCompleteTimeline->GetVkSemaphore();
		const uint64_t    waitValue = m_RenderingCompleteTimeline->GetValue();

		if (semaphore == VK_NULL_HANDLE || waitValue == 0)
			return;

		const auto startTime = logWaitTime ? std::chrono::high_resolution_clock::now()
										   : std::chrono::high_resolution_clock::time_point{};

		const Ref<Device> device = RenderContext::Get()->GetDevice();

		VkSemaphoreWaitInfo waitInfo{};
		waitInfo.sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
		waitInfo.semaphoreCount = 1;
		waitInfo.pSemaphores    = &semaphore;
		waitInfo.pValues        = &waitValue;

		SEDX_VK_RESULT_ASSERT(vkWaitSemaphores(device->GetLogicalDevice(), &waitInfo, UINT64_MAX),
							   "Failed to wait for command list execution via timeline semaphore");

		if (logWaitTime)
		{
			const auto endTime = std::chrono::high_resolution_clock::now();
			const float ms = std::chrono::duration<float, std::milli>(endTime - startTime).count();
			SEDX_CORE_INFO_TAG("CmdList", "'{}' GPU wait: {:.3f} ms", m_ObjectName, ms);
		}

		m_State = CommandState::Idle;
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

    void CommandList::SetIndexBuffer(const Buffer *indexBuffer)
    {
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command List must be in Recording state to set index buffer.");
        SEDX_CORE_ASSERT(indexBuffer != nullptr, "Index buffer must be valid");
        SEDX_CORE_ASSERT(indexBuffer->Get() != nullptr, "Index buffer must have a valid buffer");
        if (m_BufferID_Index == indexBuffer->GetObjectId())
            return;

        //bool is16Bit = indexBuffer->GetStride() == sizeof(uint16_t);

        vkCmdBindIndexBuffer(
            m_CmdBuffer,          // commandBuffer
            indexBuffer->Get(),   // buffer
            0,                    // offset
            VK_INDEX_TYPE_UINT16  // indexType
        );

        m_BufferID_Index = indexBuffer->GetObjectId();
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
        SEDX_CORE_ASSERT(img != nullptr, "Image handle must be valid for barrier insertion");
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to insert barriers");

        const uint32_t baseMip = (mip == ALL_MIPS) ? 0 : mip;
        const Layout::ImageLayout currentLayout = GetImageLayout(img, baseMip);

        if (currentLayout == layout)
            return;

        const BarrierAccessInfo srcInfo = GetLayoutAccessInfo(currentLayout);
        const BarrierAccessInfo dstInfo = GetLayoutAccessInfo(layout);

        VkImageSubresourceRange range{};
        range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel   = baseMip;
        range.levelCount     = (mip == ALL_MIPS || mipRange == 0) ? VK_REMAINING_MIP_LEVELS : mipRange;
        range.baseArrayLayer = 0;
        range.layerCount     = VK_REMAINING_ARRAY_LAYERS;

        VkImageMemoryBarrier2 barrier{};
        barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcStageMask        = srcInfo.stageFlags;
        barrier.srcAccessMask       = srcInfo.accessMask;
        barrier.dstStageMask        = dstInfo.stageFlags;
        barrier.dstAccessMask       = dstInfo.accessMask;
        barrier.oldLayout           = GetVkImageLayout(currentLayout);
        barrier.newLayout           = GetVkImageLayout(layout);
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image               = *static_cast<VkImage *>(img);
        barrier.subresourceRange    = range;

        VkDependencyInfo depInfo{};
        depInfo.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        depInfo.imageMemoryBarrierCount = 1;
        depInfo.pImageMemoryBarriers    = &barrier;

        vkCmdPipelineBarrier2(m_CmdBuffer, &depInfo);

        {
            std::lock_guard<std::mutex> lock(s_ImageLayoutsMutex);
            s_ImageLayouts[img] = layout;
        }
    }

    void CommandList::InsertBarrier(Buffer *buffer)
    {
        SEDX_CORE_ASSERT(buffer != nullptr && buffer->Get() != VK_NULL_HANDLE, "Buffer must be valid for barrier insertion");
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to insert barriers");

        VkBufferMemoryBarrier2 barrier{};
        barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        barrier.srcStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.srcAccessMask       = VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        barrier.dstAccessMask       = VK_ACCESS_2_SHADER_READ_BIT  | VK_ACCESS_2_TRANSFER_READ_BIT |
                                      VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_2_INDEX_READ_BIT |
                                      VK_ACCESS_2_UNIFORM_READ_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer              = buffer->Get();
        barrier.offset              = 0;
        barrier.size                = VK_WHOLE_SIZE;

        VkDependencyInfo depInfo{};
        depInfo.sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        depInfo.bufferMemoryBarrierCount = 1;
        depInfo.pBufferMemoryBarriers    = &barrier;

        vkCmdPipelineBarrier2(m_CmdBuffer, &depInfo);
    }

    void CommandList::InsertBarrier(void *image, VkFormat format, uint32_t mipIndex, uint32_t mipRange, uint32_t arrayLength, Layout::ImageLayout layout)
    {
        SEDX_CORE_ASSERT(image != nullptr, "Image handle must be valid for barrier insertion");
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to insert barriers");

        const Layout::ImageLayout currentLayout = GetImageLayout(image, mipIndex);

        if (currentLayout == layout)
            return;

        const BarrierAccessInfo srcInfo = GetLayoutAccessInfo(currentLayout);
        const BarrierAccessInfo dstInfo = GetLayoutAccessInfo(layout);

        VkImageSubresourceRange range{};
        range.aspectMask     = GetAspectMaskFromFormat(format);
        range.baseMipLevel   = mipIndex;
        range.levelCount     = (mipRange == 0)      ? VK_REMAINING_MIP_LEVELS   : mipRange;
        range.baseArrayLayer = 0;
        range.layerCount     = (arrayLength == 0)   ? VK_REMAINING_ARRAY_LAYERS : arrayLength;

        VkImageMemoryBarrier2 barrier{};
        barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcStageMask        = srcInfo.stageFlags;
        barrier.srcAccessMask       = srcInfo.accessMask;
        barrier.dstStageMask        = dstInfo.stageFlags;
        barrier.dstAccessMask       = dstInfo.accessMask;
        barrier.oldLayout           = GetVkImageLayout(currentLayout);
        barrier.newLayout           = GetVkImageLayout(layout);
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image               = *static_cast<VkImage *>(image);
        barrier.subresourceRange    = range;

        VkDependencyInfo depInfo{};
        depInfo.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        depInfo.imageMemoryBarrierCount = 1;
        depInfo.pImageMemoryBarriers    = &barrier;

        vkCmdPipelineBarrier2(m_CmdBuffer, &depInfo);

        {
            std::lock_guard<std::mutex> lock(s_ImageLayoutsMutex);
            s_ImageLayouts[image] = layout;
        }
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

    void CommandList::Dispatch(ImageResource *img, float resolutionScale)
    {
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to dispatch compute work");
        SEDX_CORE_ASSERT(img != nullptr, "ImageResource must be valid for compute dispatch");

        // Clamp scale to [0.5, 1.0]: below 0.5 produces too few workgroups to cover the texture safely
        resolutionScale = xMath::Clamp(resolutionScale, 0.5f, 1.0f);

        // Transition to General layout so the compute shader can read/write the image as a UAV
        InsertBarrier(img->Get(), img->GetImageSpec().format, 0, 0, 0, Layout::ImageLayout::General);

        const uint32_t threadGroupSize = 8;

        // Scaled dimensions — round up to guarantee full coverage at sub-1.0 scales
        const uint32_t scaledWidth  = static_cast<uint32_t>(ceil(img->GetWidth()  * resolutionScale));
        const uint32_t scaledHeight = static_cast<uint32_t>(ceil(img->GetHeight() * resolutionScale));
        const uint32_t scaledDepth  = (img->GetImageSpec().type == ImageType::Type3D)
            ? static_cast<uint32_t>(ceil(img->GetImageSpec().depth * resolutionScale))
            : 1;

        // Conservative dispatch counts (ceil division ensures all texels are covered)
        const uint32_t dispatchX = (scaledWidth  + threadGroupSize - 1) / threadGroupSize;
        const uint32_t dispatchY = (scaledHeight + threadGroupSize - 1) / threadGroupSize;
        const uint32_t dispatchZ = (scaledDepth  + threadGroupSize - 1) / threadGroupSize;

        Dispatch(dispatchX, dispatchY, dispatchZ);

        // Transition to ShaderRead so subsequent graphics/compute passes can sample the result
        InsertBarrier(img->Get(), img->GetImageSpec().format, 0, 0, 0, Layout::ImageLayout::ShaderRead);
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

    void CommandList::SetCullMode(const CullMode cullMode)
    {
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to set cull mode");
        SEDX_CORE_ASSERT(cullMode != CullMode::MaxEnum, "Invalid cull mode. You must specify a valid cull mode.");

        m_CullMode = GetCullingType(cullMode);
        vkCmdSetCullMode(m_CmdBuffer, m_CullMode);
    }

    void CommandList::SetTexture(const uint32_t slot, ImageResource* img, const uint32_t mipIndex /*= all_mips*/, uint32_t mipRange /*= 0*/, const bool uav /*= false*/)
    {
        SEDX_CORE_ASSERT(m_State == CommandState::Recording);

        if (mipIndex != ALL_MIPS)
        {
            SEDX_CORE_ASSERT(mipRange != 0, "If a mip was specified, then mip_range can't be 0");
        }

        if (!m_DescriptorLayout_Current)
        {
            SEDX_CORE_WARN_TAG("CommandList","Descriptor layout not set, try setting texture \"%s\" within a render pass", img->GetObjectName().c_str());
            return;
        }

        // if the texture is null, or it's still loading, ignore it
        if (!img || img->GetResourceState() != ResourceState::PreparedForGpu)
            return;

        // get some texture info
        const uint32_t mip_count     = img->GetImageSpec().mipCount;
        const bool mip_specified     = mipIndex != ALL_MIPS;
        const uint32_t mip_start     = mip_specified ? mipIndex : 0;
        Layout::ImageLayout current_layout = CommandList::GetImageLayout(img->Get(), mip_start);

        SEDX_CORE_ASSERT(current_layout != Layout::ImageLayout::MaxEnum, "Invalid layout");

        // transition to appropriate layout (if needed)
        {
            Layout::ImageLayout targetLayout = Layout::ImageLayout::MaxEnum;
            if (uav)
            {
                SEDX_CORE_ASSERT((img->GetImageSpec().flags & ImageResourceFlags::UnorderedAccessView) != 0);

                // according to section 13.1 of the Vulkan spec, storage textures have to be in a general layout.
                // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#descriptorsets-storageimage
                targetLayout = Layout::ImageLayout::General;
            }
            else
            {
                SEDX_CORE_ASSERT((img->GetImageSpec().flags & ImageResourceFlags::ShaderViews) != 0);
                targetLayout = Layout::ImageLayout::ShaderRead;
            }

            // verify that an appropriate layout has been deduced
            SEDX_CORE_ASSERT(targetLayout != Layout::ImageLayout::MaxEnum);

            // determine if a layout transition is needed
            bool transition_required = current_layout != targetLayout;
            {
                bool rest_mips_have_same_layout = true;
                for (uint32_t i = mip_start; i < mip_count; i++)
                {
                    if (targetLayout != CommandList::GetImageLayout(img->Get(), i))
                    {
                        rest_mips_have_same_layout = false;
                        break;
                    }
                }

                transition_required = !rest_mips_have_same_layout ? true : transition_required;
            }

            // transition
            if (transition_required)
            {
                img->SetLayout(targetLayout, this, mipIndex, mipRange);
            }
        }

        // TODO: Bind img to descriptor slot when DescriptorSet exposes a SetTexture API.
        // m_DescriptorLayout_Current->SetTexture(slot, img, mipIndex, mipRange);
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

    void CommandList::SetPipelineState(const PipelineState& /*pso*/)
    {
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to set pipeline state");
        // TODO: Resolve or create a cached VkPipeline from the PipelineState descriptor,
        //       begin the dynamic render pass, and bind the pipeline.
        // This stub exists so that all render-pass code that calls SetPipelineState compiles.
        SEDX_CORE_WARN_TAG("CommandList", "SetPipelineState: stub — VkPipeline creation not yet wired");
    }

    void CommandList::PushConstants(const PushConstantBuffer& data)
    {
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to push constants");
        // TODO: Bind data to the active pipeline layout via vkCmdPushConstants.
        // The layout and stage flags must be sourced from the currently bound pipeline.
        (void)data;
        SEDX_CORE_WARN_TAG("CommandList", "PushConstants: stub — pipeline layout not yet wired");
    }

    void CommandList::SetBuffer(Renderer_BindingsUav /*slot*/, Buffer* /*buffer*/)
    {
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to bind a buffer");
        // TODO: Bind structured/storage buffer to the UAV slot in the active descriptor set.
        SEDX_CORE_WARN_TAG("CommandList", "SetBuffer: stub — descriptor update not yet wired");
    }

    void CommandList::SetBufferVertex(Buffer* vertexBuffer)
    {
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to set vertex buffer");
        SEDX_CORE_ASSERT(vertexBuffer != nullptr, "Vertex buffer must be valid");

        const VkBuffer buf    = vertexBuffer->Get();
        const VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(m_CmdBuffer, 0, 1, &buf, &offset);
    }

    void CommandList::SetBufferIndex(Buffer* indexBuffer)
    {
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to set index buffer");
        SEDX_CORE_ASSERT(indexBuffer != nullptr, "Index buffer must be valid");

        vkCmdBindIndexBuffer(m_CmdBuffer, indexBuffer->Get(), 0, VK_INDEX_TYPE_UINT32);
    }

    void CommandList::DrawIndexedIndirectCount(Buffer* drawArgs, uint64_t argsOffset, Buffer* countBuffer, uint64_t countOffset, uint32_t maxDrawCount)
    {
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state for indirect draw");
        SEDX_CORE_ASSERT(drawArgs != nullptr && countBuffer != nullptr, "Indirect draw buffers must be valid");

        vkCmdDrawIndexedIndirectCount(
            m_CmdBuffer,
            drawArgs->Get(), argsOffset,
            countBuffer->Get(), countOffset,
            maxDrawCount,
            sizeof(VkDrawIndexedIndirectCommand)
        );
    }

    void CommandList::InsertBarrier(ImageResource* img, BarrierType type)
    {
        SEDX_CORE_ASSERT(img != nullptr, "ImageResource must be valid for barrier insertion");
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to insert barriers");

        Layout::ImageLayout targetLayout = Layout::ImageLayout::General;

        switch (type)
        {
            case BarrierType::EnsureWriteThenRead:
                targetLayout = Layout::ImageLayout::ShaderRead;
                break;
            case BarrierType::EnsureReadThenWrite:
            case BarrierType::EnsureWriteThenWrite:
                targetLayout = Layout::ImageLayout::General;
                break;
            default:
                break;
        }

        InsertBarrier(img->Get(), img->GetImageSpec().format, 0, 0, 0, targetLayout);
    }

    void CommandList::Blit(ImageResource* src, ImageResource* dst, bool /*keepAspect*/, float /*resolutionScale*/)
    {
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to blit");
        SEDX_CORE_ASSERT(src != nullptr && dst != nullptr, "Source and destination images must be valid for blit");

        // Transition source to TransferSrc and destination to TransferDst
        InsertBarrier(src->Get(), src->GetImageSpec().format, 0, 1, 0, Layout::ImageLayout::TransferSrc);
        InsertBarrier(dst->Get(), dst->GetImageSpec().format, 0, 1, 0, Layout::ImageLayout::TransferDst);

        VkImageBlit region{};
        region.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.mipLevel       = 0;
        region.srcSubresource.baseArrayLayer = 0;
        region.srcSubresource.layerCount     = 1;
        region.srcOffsets[0]                 = { 0, 0, 0 };
        region.srcOffsets[1]                 = { static_cast<int32_t>(src->GetWidth()), static_cast<int32_t>(src->GetHeight()), 1 };

        region.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.dstSubresource.mipLevel       = 0;
        region.dstSubresource.baseArrayLayer = 0;
        region.dstSubresource.layerCount     = 1;
        region.dstOffsets[0]                 = { 0, 0, 0 };
        region.dstOffsets[1]                 = { static_cast<int32_t>(dst->GetWidth()), static_cast<int32_t>(dst->GetHeight()), 1 };

        const Ref<Device> device = RenderContext::Get()->GetDevice();
        vkCmdBlitImage(
            m_CmdBuffer,
            *src->Get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            *dst->Get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &region,
            VK_FILTER_LINEAR
        );
    }

} // namespace SceneryEditorX

// -------------------------------------------------------

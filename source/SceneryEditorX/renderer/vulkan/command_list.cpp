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

#include "bindless_manager.h"
#include "buffer.h"
#include "depth_stencil.h"
#include "image_resource.h"
#include "rasterizer.h"
#include "render_context.h"
#include "swapchain.h"
#include "debug/graphics_debug.h"
#include "pipeline/barrier_info.h"
#include "pipeline/pipeline_state.h"
#include "pipeline/pipeline.h"
#include "shader/shader_stage.h"

#include <SceneryEditorX/renderer/vulkan/push_constant_buffer.h>
#include <array>
#include <chrono>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <SceneryEditorX/renderer/renderer.h>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		/**
		* @struct ImmediateExecutionState
		* @brief Represents the state for immediate execution of command lists.
		*/
		struct ImmediateExecutionState
		{
			Scope<CommandPool> pool;
			Ref<CommandList> cmdList;
			std::mutex mutex;
		};
	}

	// Per-image layout tracking: key = &VkImage (stable address), value = per-mip current layouts.
	// Protected by s_ImageLayoutsMutex for safe concurrent reads from multiple threads.
	static std::unordered_map<VkImage, std::array<Layout::ImageLayout, MAX_MIP_COUNT>> s_ImageLayouts;	static std::mutex s_ImageLayoutsMutex;
	static std::array<ImmediateExecutionState, static_cast<size_t>(QueueType::MaxEnum)> s_ImmediateStates;
	//std::unordered_map<void*, std::array<Layout::ImageLayout, MAX_MIP_COUNT>> image_Layouts;

	#pragma region Static Command Actions

	/**
	 * @brief Get the current layout of a specific mip level of a Vulkan image.
	 * @param image vulkan image handle
	 * @param mipIndex mip level index
	 * @return current layout of the specified mip level
	 */
	static Layout::ImageLayout GetLayout(VkImage image, uint32_t mipIndex)
	{
		SEDX_CORE_ASSERT(image != nullptr, "Image must not be null");
		std::scoped_lock lock(s_ImageLayoutsMutex);

		auto it = s_ImageLayouts.find(image);
		if (it == s_ImageLayouts.end())
		{
			return Layout::ImageLayout::MaxEnum;
		}

		SEDX_CORE_ASSERT(mipIndex < MAX_MIP_COUNT, "Mip index out of range");
		return it->second[mipIndex];
	}

	/**
	 * @brief Set the layout of a specific range of mip levels for a Vulkan image.
	 * @param image vulkan image handle
	 * @param mip_index starting mip level index
	 * @param mip_range number of mip levels to set
	 * @param layout new layout to set
	 */
	static void SetLayout(VkImage image, uint32_t mip_index, uint32_t mip_range, Layout::ImageLayout layout)
	{
		SEDX_CORE_ASSERT(image != nullptr, "Image must not be null");
		SEDX_CORE_ASSERT(mip_index < MAX_MIP_COUNT, "Mip index out of range");
		SEDX_CORE_ASSERT(mip_index + mip_range <= MAX_MIP_COUNT, "Mip range out of range");
		std::scoped_lock lock(s_ImageLayoutsMutex);

		auto it = s_ImageLayouts.find(image);
		if (it == s_ImageLayouts.end())
		{
			std::array<Layout::ImageLayout, MAX_MIP_COUNT> layouts;
			layouts.fill(Layout::ImageLayout::MaxEnum);
			s_ImageLayouts[image] = layouts;
			it = s_ImageLayouts.find(image);
		}

		uint32_t mip_end = xMath::Min(mip_index + mip_range, MAX_MIP_COUNT);
		for (uint32_t i = mip_index; i < mip_end; ++i)
		{
			it->second[i] = layout;
		}
	}

	/**
	 * @brief Remove the layout tracking for a Vulkan image.
	 * @param image vulkan image handle
	 */
	static void RemoveLayout(VkImage image)
	{
		std::scoped_lock lock(s_ImageLayoutsMutex);
		s_ImageLayouts.erase(image);
	}

	/**
	 * @brief Convert a barrier scope to Vulkan pipeline stages.
	 * @param scope the barrier scope
	 * @param isDepth whether the barrier is for a depth attachment
	 * @return the corresponding Vulkan pipeline stage flags
	 */
	static VkPipelineStageFlags2 ScopeToStages(BarrierScope scope, bool isDepth = false)
	{
		switch (scope)
		{
		case BarrierScope::Graphics:
			return VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
				   VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT |
				   VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
				   (isDepth ? (VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT)
						: VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
		case BarrierScope::Compute:
			return VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		case BarrierScope::Transfer:
			return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		case BarrierScope::Fragment:
			return VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		case BarrierScope::All:
			return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		case BarrierScope::Auto:
		default:
			return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // auto handled by layout-based deduction
		}
	}

	/**
	 * @brief Create a Vulkan image memory barrier.
	 * @param img a pointer to the Vulkan image handle
	 * @param srcMask the source access mask
	 * @param dstMask the destination access mask
	 * @param oldLayout the old image layout
	 * @param newLayout the new image layout
	 * @param subresourceRange the optional subresource range
	 * @return the created VkImageMemoryBarrier2 structure
	 */
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
		// img may be a pointer to a VkImage handle (VkImage*). Safely dereference if non-null.
		if (img)
		{
			barrier.image = *static_cast<VkImage*>(img);
		}
		else
		{
			barrier.image = VK_NULL_HANDLE;
		}
		
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

	/**
	 * @brief Convert a Vulkan image layout to its corresponding abstract image layout.
	 * @param layout the Vulkan image layout to convert
	 * @return the corresponding abstract image layout
	 */
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

	/**
	 * @brief Convert an abstract image layout to its corresponding Vulkan image layout.
	 * @param layout the abstract image layout to convert
	 * @return the corresponding VkImageLayout value
	 */
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

	/**
	 * @brief Convert a cull mode to its corresponding Vulkan cull mode flags.
	 * @param cullMode the cull mode to convert
	 * @return the corresponding Vulkan cull mode flags
	 */
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
	 * @brief Convert a color to its corresponding Vulkan attachment load operation.
	 * @param color the color to convert
	 * @return the corresponding Vulkan attachment load operation
	 */
	static VkAttachmentLoadOp GetColorLoadOp(const Color& color)
	{
		if (color == COLOR_DONT_CARE)
			return VK_ATTACHMENT_LOAD_OP_DONT_CARE;

		if (color == COLOR_LOAD)
			return VK_ATTACHMENT_LOAD_OP_LOAD;

		return VK_ATTACHMENT_LOAD_OP_CLEAR;
	};

	/**
	 * @brief Convert a depth value to its corresponding Vulkan attachment load operation.
	 * @param depth the depth value to convert
	 * @return the corresponding Vulkan attachment load operation
	 */
	static VkAttachmentLoadOp GetDepthLoadOp(const float depth)
	{
		if (depth == DEPTH_DONT_CARE)
			return VK_ATTACHMENT_LOAD_OP_DONT_CARE;

		if (depth == DEPTH_LOAD)
			return VK_ATTACHMENT_LOAD_OP_LOAD;

		return VK_ATTACHMENT_LOAD_OP_CLEAR;
	};

	/**
	 * @brief Get the aspect mask for a given Vulkan format.
	 * @param format the Vulkan format
	 * @return the corresponding aspect mask
	 */
	static uint32_t GetAspectMask(const VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_D32_SFLOAT:
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		default:
			return VK_IMAGE_ASPECT_COLOR_BIT;
		}
	}

	/**
	 * @struct BarrierAccessInfo
	 * @brief Maps a Layout::ImageLayout to the corresponding Vulkan pipeline stage and access masks.
	 * Used internally by InsertBarrier to build correct VkImageMemoryBarrier2 entries.
	 */
	struct BarrierAccessInfo
	{
		VkAccessFlags2        accessMask;
		VkPipelineStageFlags2 stageFlags;
	};

	/**
	 * @brief Given a Layout::ImageLayout, return the corresponding Vulkan access mask and pipeline stage flags for barriers.
	 * @param layout The image layout to query.
	 * @return A BarrierAccessInfo struct containing the access mask and stage flags.
	 */
	static BarrierAccessInfo GetLayoutAccessInfo(const Layout::ImageLayout layout)
	{
		switch (layout)
		{
			case Layout::ImageLayout::Undefined:
				return {
					.accessMask = VK_ACCESS_2_NONE, 
					.stageFlags = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT 
				};
			case Layout::ImageLayout::General:
				return {
					.accessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
					.stageFlags = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT 
				};
			case Layout::ImageLayout::ColorAttachment:
			case Layout::ImageLayout::Attachment:
				return {
					.accessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
					.stageFlags = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT 
				};
			case Layout::ImageLayout::DepthStencilAttachment:
			case Layout::ImageLayout::DepthAttachment:
			case Layout::ImageLayout::StencilAttachment:
				return {
					.accessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					.stageFlags = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT 
				};
			case Layout::ImageLayout::DepthStencilRead:
			case Layout::ImageLayout::DepthRead:
			case Layout::ImageLayout::StencilRead:
			case Layout::ImageLayout::DepthReadStencilAttachment:
			case Layout::ImageLayout::DepthAttachmentStencilRead:
				return {
					.accessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
					.stageFlags = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT 
				};
			case Layout::ImageLayout::ShaderRead:
			case Layout::ImageLayout::Read:
				return {
					.accessMask = VK_ACCESS_2_SHADER_READ_BIT,
					.stageFlags = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT 
				};
			case Layout::ImageLayout::TransferSrc:
				return {
					.accessMask = VK_ACCESS_2_TRANSFER_READ_BIT, 
					.stageFlags = VK_PIPELINE_STAGE_2_TRANSFER_BIT 
				};
			case Layout::ImageLayout::TransferDst:
				return {
					.accessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT, 
					.stageFlags = VK_PIPELINE_STAGE_2_TRANSFER_BIT
				};
			case Layout::ImageLayout::Present:
				return {
					.accessMask = VK_ACCESS_2_NONE, 
					.stageFlags = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT 
				};
			case Layout::ImageLayout::FragmentShadingRate:
				return {
					.accessMask = VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR,
					.stageFlags = VK_PIPELINE_STAGE_2_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR 
				};
			default:
				return {
					.accessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
					.stageFlags = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT
				};
		}
	}

#pragma endregion

	CommandList::CommandList(Queue *queue, const CommandPool &cmdPool, const char *name) : InheritanceBundle<RefCounted, IResource>(ResourceType::CommandList)
	{
		m_Queue = queue;

		// Use the device from the supplied CommandPool — safe during Device::Device()
		// construction because the pool holds an explicit Ref<Device>. Using
		// RenderContext::Get()->GetDevice() here crashes: RenderContext::m_Device is
		// not yet assigned when this ctor is called from inside Device::Device().
		Ref<Device> device = cmdPool.GetDevice();

		// Command Buffer
		{
			VkCommandBufferAllocateInfo allocateInfo = {};
			allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocateInfo.commandPool = cmdPool.GetPool();
			allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocateInfo.commandBufferCount = 1;

			// allocate
			SEDX_VK_RESULT_ASSERT(vkAllocateCommandBuffers(device->GetLogicalDevice(), &allocateInfo, &m_CmdBuffer), "Failed to allocate command buffers");

			// name — pass the logical device explicitly so naming works even during Device construction
			// before RenderContext::m_Device has been assigned.
			Debugging::SetResourceName(device->GetLogicalDevice(), m_CmdBuffer, ResourceType::CommandList, name);
			m_ObjectName = name;
		}

		// Thread the logical device down to FrameSync/Fence/Semaphore so that their
		// CreateSyncObject calls do not touch RenderContext::Get()->GetDevice() — which is
		// null when this constructor is reached from inside Device::Device().
		const VkDevice vkDevice = device ? device->GetLogicalDevice() : VK_NULL_HANDLE;

		m_RenderingCompleteSemaphore = CreateRef<FrameSync>(SyncType::Semaphore, vkDevice);
		m_RenderingCompleteTimeline  = CreateRef<FrameSync>(SyncType::SemaphoreTimeline, vkDevice);
		m_SubmitSync                 = CreateRef<FrameSync>(SyncType::Fence, vkDevice);

		// TODO: Initialize Vulkan query pools for GPU timestamping and occlusion queries, setting up the necessary resources and filling them with initial data.

	}

	CommandList::~CommandList()
	{
		/* TODO: Responsible for cleaning up and deallocating resources associated with query pools by adding them to the deletion queue. */
	}

	Layout::ImageLayout CommandList::GetImageLayout(VkImage image, uint32_t mipIndex)
	{
		std::scoped_lock lock(s_ImageLayoutsMutex);
		const auto it = s_ImageLayouts.find(image);
		if (it != s_ImageLayouts.end())
		{
			SEDX_CORE_ASSERT(mipIndex < MAX_MIP_COUNT, "Mip index out of range");
			return it->second[mipIndex];
		}

		return Layout::ImageLayout::Undefined;
	}

	Layout::ImageLayout CommandList::GetImageLayout(ImageResource* image, uint32_t mipIndex)
	{
		SEDX_CORE_ASSERT(image != nullptr, "ImageResource must be valid");
		VkImage* imgPtr = image->Get();
		return GetImageLayout(imgPtr, mipIndex);
	}

	Layout::ImageLayout CommandList::GetImageLayout(VkImage* imagePtr, uint32_t mipIndex)
	{
		if (imagePtr == nullptr || *imagePtr == VK_NULL_HANDLE)
			return Layout::ImageLayout::Undefined;

		return GetImageLayout(*imagePtr, mipIndex);
	}

	void CommandList::RemoveLayout(void *image)
	{
		std::scoped_lock lock(s_ImageLayoutsMutex);
		// callers may pass either a VkImage value, a VkImage* (pointer to handle), or an ImageResource*
		if (image == nullptr)
			return;

		// If the caller passed a VkImage* (pointer to handle), dereference to get the VkImage value
		if (VkImage* imgPtr = reinterpret_cast<VkImage*>(image))
		{
			// reinterpret_cast is used because some call sites pass &vector[index] or ImageResource::Get()
			VkImage img = *imgPtr;
			s_ImageLayouts.erase(img);
			return;
		}

		// Fallback: try to treat the pointer itself as a VkImage (covers cases where callers passed the handle directly)
		s_ImageLayouts.erase(static_cast<VkImage>(image));
	}

	CommandList* CommandList::BeginImmediateExecution(const QueueType type)
	{
		Ref<RenderContext> context = RenderContext::Get();
		SEDX_CORE_ASSERT(context.IsValid(), "RenderContext must be valid for immediate execution");

		Ref<Device> device = context->GetDevice();
		SEDX_CORE_ASSERT(device.IsValid(), "Device must be valid for immediate execution");

		Ref<QueueManager> queueManager = device->GetQueueManager();
		SEDX_CORE_ASSERT(queueManager.IsValid(), "QueueManager must be valid for immediate execution");

		struct ImmediateState
		{
			std::unique_ptr<CommandPool> pool;
			Ref<CommandList> cmdList;
			std::mutex mutex;
		};

		static std::array<ImmediateState, static_cast<size_t>(QueueType::MaxEnum)> immediateStates;

		if (Ref<Queue>* queueRef = queueManager->GetQueue(type); queueRef && *queueRef)
		{
			ImmediateState& state = immediateStates[static_cast<size_t>(type)];
			std::scoped_lock lock(state.mutex);

			if (!state.pool)
			{
				const uint32_t family = queueManager->GetFamilyIndexByType(type);
				state.pool = std::make_unique<CommandPool>(device, family, CommandPoolType::Resettable);
			}

			if (!state.cmdList)
			{
				state.cmdList = CreateRef<CommandList>(queueRef->Get(), *state.pool, "ImmediateCommandList");
			}

			if (state.cmdList->GetState() != CommandState::Idle)
			{
				state.cmdList->WaitForExecution();
			}

			state.cmdList->Begin();
			return state.cmdList.Get();
		}

		SEDX_CORE_ERROR_TAG("CommandList", "Immediate execution queue not available for type {}", static_cast<uint32_t>(type));
		return nullptr;
	}

	void CommandList::EndImmediateExecution(CommandList* cmdList)
	{
		SEDX_CORE_ASSERT(cmdList != nullptr, "Immediate command list cannot be null");
		cmdList->Submit(nullptr, true);
		cmdList->WaitForExecution();
	}

	void CommandList::ShutdownImmediateExecution()
	{
		for (auto& state : s_ImmediateStates)
		{
			std::scoped_lock lock(state.mutex);
			state.cmdList.Reset();
			state.pool.reset();
		}
	}
	
	void CommandList::Begin()
	{
		// Dedicated command list lifecycle: if the list is still pending from a previous
		// submit, wait for completion before re-recording.
		if (m_State == CommandState::Submitted)
		{
			WaitForExecution();
		}

		SEDX_CORE_ASSERT(m_State == CommandState::Idle, "Command list must be in idle state to begin recording.");

		// ONE_TIME_SUBMIT hints to the driver that this recording will be submitted exactly once
		// before being reset, allowing internal optimizations on tiling/mobile GPU architectures.
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		SEDX_CORE_ASSERT(vkBeginCommandBuffer(m_CmdBuffer, &beginInfo) == VK_SUCCESS, "Failed to begin command buffer");

		m_State            = CommandState::Recording;
		m_RenderPassActive = false;

		// Debugging aid: set a name so we can identify command buffers in tools
		// and log when Begin is called for non-idle lists.
		if (m_ObjectName.empty())
			m_ObjectName = "CommandList";

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
	
	void CommandList::Seal()
	{
		// End recording without submitting.  Used when the caller (e.g. SubmitAndPresent)
		// will include the raw VkCommandBuffer in its own VkSubmitInfo so that semaphore
		// ownership stays with the renderer's swapchain submit path.
		if (m_State != CommandState::Recording)
			return;

		EndRenderPass();
		FlushBarriers();
		SEDX_VK_RESULT_ASSERT(vkEndCommandBuffer(m_CmdBuffer), "Failed to end command buffer in Seal()");

		// Mark Idle so Begin() does not stall on WaitForExecution next frame.
		// The fence guaranteeing GPU completion is the renderer's per-frame fence in
		// s_FenceHandles[m_CurrentFrameIndex], which is waited in BeginFrame().
		m_State = CommandState::Idle;
	}

	void CommandList::SetExternalRecordingBuffer(VkCommandBuffer externalCb, bool renderPassActive)
	{
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "CommandList must be Recording before SetExternalRecordingBuffer");
		SEDX_CORE_ASSERT(m_SavedCmdBuffer == VK_NULL_HANDLE, "SetExternalRecordingBuffer called without matching RestoreCommandBuffer");
		SEDX_CORE_ASSERT(externalCb != VK_NULL_HANDLE, "External command buffer must not be null");

		m_SavedCmdBuffer        = m_CmdBuffer;
		m_SavedRenderPassActive = m_RenderPassActive;
		m_CmdBuffer             = externalCb;
		m_RenderPassActive      = renderPassActive;
	}

	void CommandList::RestoreCommandBuffer()
	{
		SEDX_CORE_ASSERT(m_SavedCmdBuffer != VK_NULL_HANDLE, "RestoreCommandBuffer called without a prior SetExternalRecordingBuffer");

		// Do NOT end the external render pass — the caller owns it.
		// Flush any pending barriers that were accumulated during the UI pass
		// onto the external CB before we switch back.
		FlushBarriers();

		m_CmdBuffer             = m_SavedCmdBuffer;
		m_RenderPassActive      = m_SavedRenderPassActive;
		m_SavedCmdBuffer        = VK_NULL_HANDLE;
		m_SavedRenderPassActive = false;
	}

	void CommandList::Submit(FrameSync *semaphoreWait, const bool isImmediate, FrameSync *semaphoreSignal, FrameSync *semaphoreTimeline, uint64_t timelineValue)
	{
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to submit.");

		// End recording: close any open render pass, then seal the command buffer.
		EndRenderPass();
		FlushBarriers();
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

		// Always signal this command list's timeline semaphore with an explicitly tracked
		// strictly increasing value.
		const uint64_t nextTimelineValue = m_NextTimelineSignalValue++;
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
		{
			Fence::Reset(fence);
		}

		m_Queue->Submit(submitInfo, fence);
		m_LastTimelineSignalValue = nextTimelineValue;

		// Record which command list produced the binary wait semaphore's signal
		if (semaphoreWait)
		{
			semaphoreWait->SetUserCmdList(this);
		}

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
		const uint64_t    waitValue = m_LastTimelineSignalValue;

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
			SEDX_CORE_INFO_TAG("CommandList", "'{}' GPU wait: {:.3f} ms", m_ObjectName, ms);
		}

		m_State = CommandState::Idle;
	}

	void CommandList::ClearDepth(VkImage img, float clearDepth)
	{
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command List must be in Recording state to clear texture.");
		SEDX_CORE_ASSERT(img != VK_NULL_HANDLE, "Must have a valid image.");

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

		VkImage vkImage = img;

		vkCmdClearDepthStencilImage(m_CmdBuffer, vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearDepthStencil, 1, &range);
	}

	void CommandList::ClearStencil(VkImage img, uint32_t clearStencil)
	{
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command List must be in Recording state to clear texture.");
		SEDX_CORE_ASSERT(img != VK_NULL_HANDLE, "Must have a valid image.");

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

		VkImage vkImage = img;

		vkCmdClearDepthStencilImage(m_CmdBuffer, vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearDepthStencil, 1, &range);
	}

	void CommandList::ClearTexture(VkImage img, const Color &color)
	{
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command List must be in Recording state to clear texture.");
		SEDX_CORE_ASSERT(img != VK_NULL_HANDLE, "Must have a valid image.");

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

		VkImage vkImage = img;

		// Clear the image
		vkCmdClearColorImage(m_CmdBuffer, vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);
	}

	void CommandList::ClearTexture(ImageResource* img, const Color &color)
	{
		SEDX_CORE_ASSERT(img != nullptr, "ImageResource must be valid");
		SEDX_CORE_ASSERT(img->Get() != nullptr && *img->Get() != VK_NULL_HANDLE, "ImageResource must contain a valid VkImage");
		ClearTexture(*img->Get(), color);
	}

	void CommandList::UpdateBuffer(Buffer* buffer, const uint64_t offset, const uint64_t size, const void* data)
	{
		SEDX_CORE_ASSERT(buffer != nullptr, "Buffer must be valid");
		SEDX_CORE_ASSERT(data != nullptr, "Source data must be valid");

		const uint64_t totalSize = static_cast<uint64_t>(buffer->GetStride()) * static_cast<uint64_t>(buffer->GetElementCount());
		SEDX_CORE_ASSERT(size > 0, "Update size must be greater than zero");
		SEDX_CORE_ASSERT(offset + size <= totalSize, "Update range exceeds destination buffer bounds");

		void* destination = buffer->GetMappedData();
		const bool needsUnmap = destination == nullptr;

		if (!destination)
		{
			destination = buffer->Map();
		}

		SEDX_CORE_ASSERT(destination != nullptr, "Failed to map destination buffer");
		memcpy(static_cast<uint8_t*>(destination) + offset, data, static_cast<size_t>(size));

		if (needsUnmap)
		{
			buffer->Unmap();
		}
	}

	void CommandList::EndRenderPass()
	{
		if (!m_RenderPassActive)
			return;

		vkCmdEndRendering(m_CmdBuffer);
		m_RenderPassActive = false;
	}

	void CommandList::InsertBarrier(VkImage image, VkFormat format, uint32_t mipIndex, uint32_t mipRange, uint32_t arrayLength, Layout::ImageLayout layout)
	{
		SEDX_CORE_ASSERT(image != VK_NULL_HANDLE, "Image handle must be valid for barrier insertion");
		// Keep assert for debug, but defensively handle invalid state at runtime to avoid
		// crashing inside the GPU driver when running release builds or when asserts
		// are disabled.
		if (m_CmdBuffer == VK_NULL_HANDLE || m_State != CommandState::Recording)
		{
			SEDX_CORE_WARN_TAG("CommandList", "InsertBarrier called on invalid CommandList '{}' (state={}, cmdBuf=0x{:p}) - using immediate fallback",
				m_ObjectName.c_str(), static_cast<int>(m_State.load()), (void*)m_CmdBuffer);

			// Defensive behavior: in some startup/race conditions a command list may
			// still be in Submitted state when callers attempt to insert image
			// barriers. Previously a debug-only assert aborted the process; instead
			// log a warning and use the immediate-command-list fallback so the
			// application can continue running while we investigate the root cause.
			SEDX_CORE_WARN_TAG("CommandList", "Debug: InsertBarrier called on invalid CommandList - immediate fallback will be used (no assert)");

			// Fallback: perform transition using an immediate command list so we do
			// not call into the driver with an invalid command buffer.
			CommandList* temp = CommandList::BeginImmediateExecution(QueueType::Graphics);
			if (temp)
			{
				if (temp == this)
				{
					SEDX_CORE_WARN_TAG("CommandList", "Immediate fallback resolved to the same command list '{}', skipping barrier to avoid recursion", m_ObjectName.c_str());
					return;
				}
				temp->InsertBarrier(image, format, mipIndex, mipRange, arrayLength, layout);
				CommandList::EndImmediateExecution(temp);
			}
			else
			{
				SEDX_CORE_WARN_TAG("CommandList", "Failed to acquire immediate command list fallback for InsertBarrier on '{}'", m_ObjectName.c_str());
			}
			return;
		}

		// Debug-time assert to still catch misuse when assertions are enabled.
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to insert barriers");

		// VK_KHR_dynamic_rendering forbids image barriers (vkCmdPipelineBarrier2 with
		// imageMemoryBarrierCount > 0) inside an active render pass instance unless
		// VK_KHR_dynamic_rendering_local_read is enabled (it is not).  End the pass
		// first so the transition is recorded in the correct command stream position.
		if (m_RenderPassActive)
			EndRenderPass();

		const uint32_t baseMip = (mipIndex == ALL_MIPS) ? 0 : mipIndex;
		const uint32_t levelCount = (mipIndex == ALL_MIPS || mipRange == 0) ? VK_REMAINING_MIP_LEVELS : mipRange;
		const uint32_t layerCount = (arrayLength == 0) ? VK_REMAINING_ARRAY_LAYERS : arrayLength;
		const Layout::ImageLayout currentLayout = GetImageLayout(image, baseMip);

		if (currentLayout == layout)
			return;

		const BarrierAccessInfo srcInfo = GetLayoutAccessInfo(currentLayout);
		const BarrierAccessInfo dstInfo = GetLayoutAccessInfo(layout);

		VkImageSubresourceRange range{};
		range.aspectMask     = (format == VK_FORMAT_UNDEFINED) ? VK_IMAGE_ASPECT_COLOR_BIT : GetAspectMaskFromFormat(format);
		range.baseMipLevel   = baseMip;
		range.levelCount     = levelCount;
		range.baseArrayLayer = 0;
		range.layerCount     = layerCount;

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
		barrier.image               = image;
		barrier.subresourceRange    = range;

		VkDependencyInfo depInfo{};
		depInfo.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		depInfo.imageMemoryBarrierCount = 1;
		depInfo.pImageMemoryBarriers    = &barrier;

		vkCmdPipelineBarrier2(m_CmdBuffer, &depInfo);

	   {
			std::scoped_lock lock(s_ImageLayoutsMutex);
			auto it = s_ImageLayouts.find(image);
			if (it == s_ImageLayouts.end())
			{
				std::array<Layout::ImageLayout, MAX_MIP_COUNT> layouts;
				layouts.fill(Layout::ImageLayout::MaxEnum);
				s_ImageLayouts[image] = layouts;
				it = s_ImageLayouts.find(image);
			}

			const uint32_t trackedLevels = (levelCount == VK_REMAINING_MIP_LEVELS) ? (MAX_MIP_COUNT - baseMip) : xMath::Min(levelCount, MAX_MIP_COUNT - baseMip);
			for (uint32_t i = 0; i < trackedLevels; ++i)
			{
				it->second[baseMip + i] = layout;
			}
	   }
	}

	void CommandList::InsertBarrier(Buffer *buffer)
	{
		SEDX_CORE_ASSERT(buffer != nullptr && buffer->Get() != VK_NULL_HANDLE, "Buffer must be valid for barrier insertion");
		if (m_State != CommandState::Recording)
		{
			SEDX_CORE_WARN_TAG("CommandList", "InsertBarrier(Buffer) called while '{}' is not recording (state={}), skipping", m_ObjectName.c_str(), static_cast<int>(m_State.load()));
			return;
		}

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

	void CommandList::FlushBarriers()
	{
		if (m_PendingBarriers.empty())
			return;	
		
		// determine the dst scope hint from the current pso (narrows overly broad auto scopes)
		BarrierScope psoScopeHint = BarrierScope::All;
		if (m_pso.IsCompute())
		{
			psoScopeHint = BarrierScope::Compute;
		}
		else if (m_pso.IsGraphics())
		{
			psoScopeHint = BarrierScope::Graphics;
		}

		// helper: set image sync access masks based on layout and sync type
		auto set_sync_access_masks = [](VkImageMemoryBarrier2& b, Layout::ImageLayout layout, BarrierType sync_type)
		{
			bool isReadOnlyLayout = (layout == Layout::ImageLayout::ShaderRead);

			switch (sync_type)
			{
				case BarrierType::EnsureWriteThenRead:
					if (isReadOnlyLayout)
					{
						b.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
						b.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
					}
					else
					{
						b.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
						b.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
					}
					break;
				case BarrierType::EnsureReadThenWrite:
					if (isReadOnlyLayout)
					{
						b.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
						b.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
					}
					else
					{
						b.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
						b.dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
					}
					break;
				case BarrierType::EnsureWriteThenWrite:
					if (isReadOnlyLayout)
					{
						b.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
						b.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
					}
					else
					{
						b.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
						b.dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
					}
					break;
			}
		};

		static thread_local std::vector<VkImageMemoryBarrier2> image_barriers;
		static thread_local std::vector<VkBufferMemoryBarrier2> buffer_barriers;
		image_barriers.clear();
		buffer_barriers.clear();

		for (const auto& pending : m_PendingBarriers)
		{
			switch (pending.barrier.type)
			{
				case Barrier::Type::ImageLayout:
				{
					// use pso-aware scope narrowing for the dst when auto and the target layout is general
					BarrierScope effective_dst = pending.barrier.scope_dst;
					if (effective_dst == BarrierScope::Auto && pending.layoutNew == Layout::ImageLayout::General)
						effective_dst = psoScopeHint;

					{
						// Build barrier based on layout info
						const BarrierAccessInfo srcInfo = GetLayoutAccessInfo(pending.layoutOld);
						const BarrierAccessInfo dstInfo = GetLayoutAccessInfo(pending.layoutNew);

						VkImageMemoryBarrier2 vk_barrier = {};
						vk_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
						vk_barrier.srcStageMask = srcInfo.stageFlags;
						vk_barrier.srcAccessMask = srcInfo.accessMask;
						vk_barrier.dstStageMask = dstInfo.stageFlags;
						vk_barrier.dstAccessMask = dstInfo.accessMask;
						vk_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
						vk_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
						// pending.image is stored as a VkImage handle. Assign directly.
						vk_barrier.image = pending.image;
						vk_barrier.oldLayout = GetVkImageLayout(pending.layoutOld);
						vk_barrier.newLayout = GetVkImageLayout(pending.layoutNew);
						vk_barrier.subresourceRange.aspectMask = pending.aspect_Mask;
						vk_barrier.subresourceRange.baseMipLevel = pending.mip_Index;
						vk_barrier.subresourceRange.levelCount = (pending.mip_Range == 0) ? VK_REMAINING_MIP_LEVELS : pending.mip_Range;
						vk_barrier.subresourceRange.baseArrayLayer = 0;
						vk_barrier.subresourceRange.layerCount = (pending.array_Length == 0) ? VK_REMAINING_ARRAY_LAYERS : pending.array_Length;

						image_barriers.push_back(vk_barrier);
					}
					break;
				}

				case Barrier::Type::ImageSync:
				{
					// resolve stage masks with pso-aware narrowing
					VkPipelineStageFlags2 src_stages = (pending.barrier.scope_src != BarrierScope::Auto)
						? ScopeToStages(pending.barrier.scope_src) : (VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

					VkPipelineStageFlags2 dst_stages = (pending.barrier.scope_dst != BarrierScope::Auto)
						? ScopeToStages(pending.barrier.scope_dst) : ScopeToStages(psoScopeHint, pending.isDepth);

					if (pending.has_PerMipViews)
					{
						for (uint32_t mip = 0; mip < pending.per_MipCount; ++mip)
						{
							Layout::ImageLayout layout = pending.per_MipLayouts[mip];

							VkImageMemoryBarrier2 vk_barrier           = {};
							vk_barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
							vk_barrier.srcStageMask                    = src_stages;
							vk_barrier.dstStageMask                    = dst_stages;
							vk_barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
							vk_barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
							// pending.image is stored as a VkImage handle. Assign directly.
							vk_barrier.image = pending.image;
							vk_barrier.oldLayout                       = GetVkImageLayout(layout);
							vk_barrier.newLayout                       = GetVkImageLayout(layout); // no transition
							vk_barrier.subresourceRange.aspectMask     = pending.aspect_Mask;
							vk_barrier.subresourceRange.baseMipLevel   = mip;
							vk_barrier.subresourceRange.levelCount     = 1;
							vk_barrier.subresourceRange.baseArrayLayer = 0;
							vk_barrier.subresourceRange.layerCount     = pending.array_Length;

							set_sync_access_masks(vk_barrier, layout, pending.barrier.sync_type);
							image_barriers.push_back(vk_barrier);
						}
					}
					else
					{
						Layout::ImageLayout layout = pending.per_MipLayouts[0];

						VkImageMemoryBarrier2 vk_barrier           = {};
						vk_barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
						vk_barrier.srcStageMask                    = src_stages;
						vk_barrier.dstStageMask                    = dst_stages;
						vk_barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
						vk_barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
						// pending.image is stored as a VkImage handle. Assign directly.
						vk_barrier.image = pending.image;
						vk_barrier.oldLayout                       = GetVkImageLayout(layout);
						vk_barrier.newLayout                       = GetVkImageLayout(layout); // no transition
						vk_barrier.subresourceRange.aspectMask     = pending.aspect_Mask;
						vk_barrier.subresourceRange.baseMipLevel   = 0;
						vk_barrier.subresourceRange.levelCount     = pending.per_MipCount;
						vk_barrier.subresourceRange.baseArrayLayer = 0;
						vk_barrier.subresourceRange.layerCount     = pending.array_Length;

						set_sync_access_masks(vk_barrier, layout, pending.barrier.sync_type);
						image_barriers.push_back(vk_barrier);
					}
					break;
				}

				case Barrier::Type::BufferSync:
				{
					VkBufferMemoryBarrier2 vk_barrier = {};
					vk_barrier.sType                  = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
					vk_barrier.srcStageMask           = (pending.barrier.scope_src != BarrierScope::Auto)
						? ScopeToStages(pending.barrier.scope_src) : VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
					vk_barrier.srcAccessMask          = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
					vk_barrier.dstStageMask           = (pending.barrier.scope_dst != BarrierScope::Auto)
						? ScopeToStages(pending.barrier.scope_dst) : ScopeToStages(psoScopeHint);
					vk_barrier.dstAccessMask          = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
					vk_barrier.srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED;
					vk_barrier.dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED;

					// Buffer wrapper exposes Get() for the underlying VkBuffer
					vk_barrier.buffer                 = pending.barrier.buffer->Get();
					vk_barrier.offset                 = pending.barrier.offset;
					vk_barrier.size                   = (pending.barrier.size == 0) ? VK_WHOLE_SIZE : pending.barrier.size;

					buffer_barriers.push_back(vk_barrier);
					break;
				}
			}
		}
		
		// Update tracked layouts for ImageLayout transitions so the tracker reflects the state after Flush
		{
			std::scoped_lock lock(s_ImageLayoutsMutex);
			for (const auto& pending : m_PendingBarriers)
			{
				if (pending.barrier.type == Barrier::Type::ImageLayout)
				{
					// Track the top-level layout_new across all mips for simplicity
					auto it = s_ImageLayouts.find(pending.image);
					if (it == s_ImageLayouts.end())
					{
						std::array<Layout::ImageLayout, MAX_MIP_COUNT> layouts;
						layouts.fill(Layout::ImageLayout::MaxEnum);
						s_ImageLayouts[pending.image] = layouts;
						it = s_ImageLayouts.find(pending.image);
					}
					for (uint32_t i = 0; i < MAX_MIP_COUNT; ++i)
					{
						it->second[i] = pending.layoutNew;
					}
				}
			}
		}

		// compute queues only support a subset of pipeline stages; filter out graphics-only stages
		if (m_Queue->GetType() == QueueType::Compute)
		{
			auto sanitize = [](VkPipelineStageFlags2 stages) -> VkPipelineStageFlags2
			{
				const VkPipelineStageFlags2 computeValid =
					VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT        |
					VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT     |
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT     |
					VK_PIPELINE_STAGE_2_TRANSFER_BIT           |
					VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT       |
					VK_PIPELINE_STAGE_2_HOST_BIT               |
					VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

				VkPipelineStageFlags2 filtered = stages & computeValid;
				return filtered ? filtered : VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			};

			for (auto& b : image_barriers)
			{
				b.srcStageMask = sanitize(b.srcStageMask);
				b.dstStageMask = sanitize(b.dstStageMask);
			}
			for (auto& b : buffer_barriers)
			{
				b.srcStageMask = sanitize(b.srcStageMask);
				b.dstStageMask = sanitize(b.dstStageMask);
			}
		}

		VkDependencyInfo dependency_info         = {};
		dependency_info.sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
		dependency_info.imageMemoryBarrierCount  = static_cast<uint32_t>(image_barriers.size());
		dependency_info.pImageMemoryBarriers     = image_barriers.data();
		dependency_info.bufferMemoryBarrierCount = static_cast<uint32_t>(buffer_barriers.size());
		dependency_info.pBufferMemoryBarriers    = buffer_barriers.data();

		EndRenderPass();
		vkCmdPipelineBarrier2(m_CmdBuffer, &dependency_info);
		m_PendingBarriers.clear();
	}

	void CommandList::Draw(const uint32_t vertexCount, const uint32_t vertexOffset)
	{
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command List must be in Recording state to issue draw calls.");
		PreDraw();
		vkCmdDraw(m_CmdBuffer, vertexCount, 1, vertexOffset, 0);
	}
	
	void CommandList::DrawIndexed(const uint32_t indexCount, const uint32_t instCount, const uint32_t indexOffset, const uint32_t vertexOffset, const uint32_t instIndex)
	{
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command List must be in Recording state to issue draw calls.");
		PreDraw();
		vkCmdDrawIndexed(m_CmdBuffer, indexCount, instCount, indexOffset, static_cast<int32_t>(vertexOffset), instIndex);
	}

	void CommandList::Dispatch(uint32_t x, uint32_t y, uint32_t z /*= 1*/)
	{
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to dispatch compute work");
		PreDraw();
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

		constexpr uint32_t threadGroupSize = 8;

		// Scaled dimensions — round up to guarantee full coverage at sub-1.0 scales
		const uint32_t scaledWidth  = static_cast<uint32_t>(ceil(img->GetWidth()  * resolutionScale));
		const uint32_t scaledHeight = static_cast<uint32_t>(ceil(img->GetHeight() * resolutionScale));
		const uint32_t scaledDepth  = (img->GetImageSpec().type == ImageType::Type3D) ? static_cast<uint32_t>(ceil(img->GetImageSpec().depth * resolutionScale)) : 1;

		// Conservative dispatch counts (ceil division ensures all texels are covered)
		const uint32_t dispatchX = (scaledWidth  + threadGroupSize - 1) / threadGroupSize;
		const uint32_t dispatchY = (scaledHeight + threadGroupSize - 1) / threadGroupSize;
		const uint32_t dispatchZ = (scaledDepth  + threadGroupSize - 1) / threadGroupSize;

		Dispatch(dispatchX, dispatchY, dispatchZ);

		// synchronize writes to the texture
		if (GetImageLayout(img->Get(), 0) == Layout::ImageLayout::General)
		{
			// Transition to ShaderRead so subsequent graphics/compute passes can sample the result
			InsertBarrier(img->Get(), img->GetImageSpec().format, 0, 0, 0, Layout::ImageLayout::ShaderRead);
		}
	}

	void CommandList::SetViewport(const Viewport &viewport) const
	{
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to set viewport");

		VkViewport newViewport;
		newViewport.x = viewport.x;
		newViewport.y = viewport.y;
		newViewport.width = viewport.width;
		newViewport.height = viewport.height;
		newViewport.minDepth = viewport.depth_min;
		newViewport.maxDepth = viewport.depth_max;
		vkCmdSetViewport(m_CmdBuffer, 0, 1, &newViewport);
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
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to set texture");

		if (mipIndex != ALL_MIPS)
			SEDX_CORE_ASSERT(mipRange != 0, "If a mip was specified, then mip_range can't be 0");

		if (!m_DescriptorLayout_Current)
		{
			SEDX_CORE_WARN_TAG("CommandList","Descriptor layout not set, try setting texture \"{}\" within a render pass", img->GetObjectName().c_str());
			return;
		}

		// if the texture is null, or it's still loading, ignore it
		if (!img || img->GetResourceState() != ResourceState::PreparedForGpu)
			return;

		// get some texture info
		const uint32_t mipCount     = img->GetImageSpec().mipCount;
		const bool mipSpecified     = mipIndex != ALL_MIPS;
		const uint32_t mipStart     = mipSpecified ? mipIndex : 0;
		Layout::ImageLayout currentLayout = GetImageLayout(img->Get(), mipStart);

		SEDX_CORE_ASSERT(currentLayout != Layout::ImageLayout::MaxEnum, "Invalid layout");

		// transition to appropriate layout (if needed)
		{
			Layout::ImageLayout targetLayout = Layout::ImageLayout::MaxEnum;
			if (uav)
			{
				SEDX_CORE_ASSERT((img->GetImageSpec().flags & ImageResourceFlags::UnorderedAccessView) != 0);

				/**
				 * According to section 13.1 of the Vulkan spec, storage textures have to be in a general layout.
				 * https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#descriptorsets-storageimage
				 */
				targetLayout = Layout::ImageLayout::General;
			}
			else
			{
				SEDX_CORE_ASSERT((img->GetImageSpec().flags & ImageResourceFlags::ShaderViews) != 0, "Image must have ShaderViews flag");
				targetLayout = Layout::ImageLayout::ShaderRead;
			}

			// verify that an appropriate layout has been deduced
			SEDX_CORE_ASSERT(targetLayout != Layout::ImageLayout::MaxEnum, "Invalid target layout");

			// determine if a layout transition is needed
			bool transitionRequired = currentLayout != targetLayout;
			{
				bool rest_mips_have_same_layout = true;
				std::array<Layout::ImageLayout, MAX_MIP_COUNT> layouts = img->GetLayouts();
				for (uint32_t i = mipStart; i < mipCount; i++)
				{
					if (targetLayout != GetImageLayout(img->Get(), i))
					{
						rest_mips_have_same_layout = false;
						break;
					}
				}

				transitionRequired = !rest_mips_have_same_layout ? true : transitionRequired;
			}

			// transition
			if (transitionRequired)
			{
				img->SetLayout(targetLayout, this, mipIndex, mipRange);
			}
		}

		/* TODO: Bind img to descriptor slot when DescriptorSet exposes a SetTexture API. */
		// set (will only happen if it's not already set)
		m_DescriptorLayout_Current->SetTexture(slot, img, mipIndex, mipRange);

		/* TODO: detect if there are changes, otherwise don't bother binding */
		m_NeedsDynamicBind = true;
	}

	void CommandList::Copy(ImageResource *src, Swapchain *dst)
	{
		SEDX_CORE_ASSERT((src->GetFlags() & ImageResourceFlags::BlitClear) != 0, "The image resource needs the BlitClear flag");
		SEDX_CORE_ASSERT(src->GetWidth() == dst->GetWidth(), "Source and destination textures must have the same width");
		SEDX_CORE_ASSERT(src->GetHeight() == dst->GetHeight(), "Source and destination textures must have the same height");
		SEDX_CORE_ASSERT(src->GetFormat() == dst->GetImageFormat(), "Source and destination textures must have the same format");

		VkImageCopy copyRegion               = {};
		copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.srcSubresource.mipLevel   = 0;
		copyRegion.srcSubresource.layerCount = 1;
		copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.dstSubresource.mipLevel   = 0;
		copyRegion.dstSubresource.layerCount = 1;
		copyRegion.extent.width              = src->GetWidth();
		copyRegion.extent.height             = src->GetHeight();
		copyRegion.extent.depth              = 1;

		// transition to blit appropriate layouts
		// src is ImageResource*, so forward to the wrapper that accepts ImageResource*
		Layout::ImageLayout initialSrcLayout = GetImageLayout(src, 0);

		// determine which swapchain image we will target
		const uint32_t imgIndex = dst->GetImageIndex();

		src->SetLayout(Layout::ImageLayout::TransferSrc, this);

		// Avoid taking address of a possibly-temporary vector element. Copy the handle
		// to a local VkImage and use the value overload of InsertBarrier.
		VkImage dstImage = dst->GetImages()[imgIndex];
		InsertBarrier(dstImage, dst->GetImageFormat(), 0, 1, 1, Layout::ImageLayout::TransferDst);

		// blit
		vkCmdCopyImage(m_CmdBuffer,
			static_cast<VkImage>(*src->Get()), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &copyRegion);

		// transition to the initial layout
		src->SetLayout(initialSrcLayout, this);
		InsertBarrier(dstImage, dst->GetImageFormat(), 0, 1, 1, Layout::ImageLayout::Present);
	}

	void CommandList::Copy(ImageResource *src, ImageResource *dst, const bool blitMips)
	{
		SEDX_CORE_ASSERT((src->GetFlags() & ImageResourceFlags::BlitClear) != 0, "The texture needs the BlitClear flag");
		SEDX_CORE_ASSERT((dst->GetFlags() & ImageResourceFlags::BlitClear) != 0, "The texture needs the BlitClear flag");
		SEDX_CORE_ASSERT(src->GetWidth() == dst->GetWidth(), "Source and destination textures must have the same width");
		SEDX_CORE_ASSERT(src->GetHeight() == dst->GetHeight(), "Source and destination textures must have the same height");
		SEDX_CORE_ASSERT(src->GetFormat() == dst->GetFormat(), "Source and destination textures must have the same format");

		if (blitMips)
		{
			SEDX_CORE_ASSERT(src->GetMipCount() == dst->GetMipCount(),
				"If the mips are blitted, then the mip count between the source and the destination textures must match");
		}

		std::array<VkImageCopy, MAX_MIP_COUNT> copyRegions = {};
		uint32_t copyRegionCount = blitMips ? src->GetMipCount() : 1;
		for (uint32_t mipIndex = 0; mipIndex < copyRegionCount; mipIndex++)
		{
			VkImageCopy& copyRegion              = copyRegions[mipIndex];
			copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.srcSubresource.mipLevel   = mipIndex;
			copyRegion.srcSubresource.layerCount = 1;
			copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.dstSubresource.mipLevel   = mipIndex;
			copyRegion.dstSubresource.layerCount = 1;
			copyRegion.extent.width              = src->GetWidth()  >> mipIndex;
			copyRegion.extent.height             = src->GetHeight() >> mipIndex;
			copyRegion.extent.depth              = 1;
		}

		// save the initial layouts
		std::array<Layout::ImageLayout, MAX_MIP_COUNT> initialSrcLayouts;
		std::array<Layout::ImageLayout, MAX_MIP_COUNT> initialDstLayouts;

		for (uint32_t i = 0; i < MAX_MIP_COUNT; ++i)
		{
			if (i < copyRegionCount) // only need to preserve layouts for the mips we will touch
			{
				initialSrcLayouts[i] = GetImageLayout(src->Get(), i);
				initialDstLayouts[i] = GetImageLayout(dst->Get(), i);
			}
			else
			{
				initialSrcLayouts[i] = Layout::ImageLayout::Undefined;
				initialDstLayouts[i] = Layout::ImageLayout::Undefined;
			}
		}

		// transition to blit appropriate layouts
		src->SetLayout(Layout::ImageLayout::TransferSrc, this);
		dst->SetLayout(Layout::ImageLayout::TransferDst, this);

		vkCmdCopyImage(m_CmdBuffer,
			static_cast<VkImage>(*src->Get()), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			static_cast<VkImage>(*dst->Get()), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			copyRegionCount, copyRegions.data());

		// transition to the initial layouts
		if (blitMips)
		{
			for (uint32_t i = 0; i < src->GetMipCount(); i++)
			{
				src->SetLayout(initialSrcLayouts[i], this, i, 1);
				dst->SetLayout(initialDstLayouts[i], this, i, 1);
			}
		}
		else
		{
			src->SetLayout(initialSrcLayouts[0], this);
			dst->SetLayout(initialDstLayouts[0], this);
		}

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

	void CommandList::PreDraw()
	{
		FlushBarriers();

		if (!m_RenderPassActive && m_pso.IsGraphics())
		{
			BeginRenderPass();
		}

		if (m_NeedsDynamicBind)
		{
			DescriptorSet::SetDynamicDescriptor(m_pso, m_CmdBuffer, m_Pipeline.GetLayout(), m_DescriptorLayout_Current);
			m_NeedsDynamicBind = false;
		}
	}

	void CommandList::BeginRenderPass()
	{
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to begin a render pass");
		EndRenderPass();

		if (!m_pso.IsGraphics())
			return;

		VkRenderingInfo renderInfo		= {};
		renderInfo.sType				= VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderInfo.renderArea.offset = {.x = 0, .y = 0 };
		renderInfo.renderArea.extent = {.width = m_pso.GetWidth(), .height = m_pso.GetHeight() };
		renderInfo.layerCount           = 1;
		renderInfo.colorAttachmentCount = 0;
		renderInfo.pColorAttachments    = nullptr;
		renderInfo.pDepthAttachment     = nullptr;
		renderInfo.pStencilAttachment   = nullptr;
		
		// multiview: viewMask selects which array layers to render into simultaneously
		if (m_pso.isMultiview)
		{
			renderInfo.viewMask = 0b11;
		}

		// color attachments
		std::array<VkRenderingAttachmentInfo, MAX_RENDER_TARGET_COUNT> attachments_color{};
		uint32_t attachment_index = 0;

		{
			// swapchain buffer as a render target
			Swapchain* swapchain = m_pso.renderTarget_Swapchain;
			if (swapchain)
			{
				// determine the current swapchain image index and use the corresponding view
				const uint32_t imgIndex = swapchain->GetImageIndex();
				VkImage dstImage = swapchain->GetImages()[imgIndex];

				// transition the targeted swapchain image to attachment layout
				InsertBarrier(dstImage, swapchain->GetImageFormat(), 0, 1, 1, Layout::ImageLayout::Attachment);

				VkRenderingAttachmentInfo color_attachment{};
				color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
				// pick the image view corresponding to the acquired swapchain image
				color_attachment.imageView = swapchain->GetImageViews()[imgIndex];
				color_attachment.imageLayout = GetVkImageLayout(Layout::ImageLayout::Attachment);
				color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

				SEDX_CORE_ASSERT(color_attachment.imageView != VK_NULL_HANDLE, "Invalid image view");

				attachments_color[attachment_index++] = color_attachment;
			}
			else // regular render target(s)
			{

				for (uint32_t i = 0; i < MAX_RENDER_TARGET_COUNT; i++)
				{
					ImageResource* rt = m_pso.renderTarget_ColorTextures[i];
					if (rt == nullptr)
						break;

					SEDX_CORE_ASSERT(rt->IsColorFormat(), "The texture wasn't created with the Texture_RenderTarget flag and/or isn't a color format");

					// transition to the appropriate layout
					rt->SetLayout(Layout::ImageLayout::Attachment, this);

					VkRenderingAttachmentInfo color_attachment{};
					color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
					color_attachment.imageView = m_pso.isMultiview && rt->GetRenderTargetView_MultiView()
						? static_cast<VkImageView>(rt->GetRenderTargetView_MultiView())
						: static_cast<VkImageView>(rt->GetRenderTargetView(m_pso.renderTarget_ArrayIndex));
					color_attachment.imageLayout = GetVkImageLayout(GetImageLayout(rt, 0));
					// convert PipelineStateColor to Color for GetColorLoadOp
					Color clearCol{ m_pso.clearColor[i].r, m_pso.clearColor[i].g, m_pso.clearColor[i].b, m_pso.clearColor[i].a };
					color_attachment.loadOp = m_Load_Color_RenderTargets[i] ? VK_ATTACHMENT_LOAD_OP_LOAD : GetColorLoadOp(clearCol);
					color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
					color_attachment.clearValue.color = { m_pso.clearColor[i].r, m_pso.clearColor[i].g, m_pso.clearColor[i].b, m_pso.clearColor[i].a };

					SEDX_CORE_ASSERT(color_attachment.imageView != VK_NULL_HANDLE, "Invalid image view");

					attachments_color[attachment_index++] = color_attachment;
				}
			}
			renderInfo.colorAttachmentCount = attachment_index;
			renderInfo.pColorAttachments    = attachments_color.data();
		}

		// depth-stencil attachment
		VkRenderingAttachmentInfo attachment_depth_stencil{};
		if (m_pso.renderTarget_DepthTexture != nullptr)
		{
			ImageResource* rt = m_pso.renderTarget_DepthTexture;
			if (!m_pso.resolutionScale)
			{ 
				SEDX_CORE_ASSERT(rt->GetWidth() == renderInfo.renderArea.extent.width, "The depth buffer doesn't match the output resolution");
			}
			SEDX_CORE_ASSERT(rt->IsDepthStencilFormat(), "Invalid depth-stencil format");

			// transition to the appropriate layout
			Layout::ImageLayout layout = Layout::ImageLayout::Attachment;
			rt->SetLayout(layout, this);

			attachment_depth_stencil.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			attachment_depth_stencil.imageView = m_pso.isMultiview && rt->GetDepthStencilView_MultiView()
				? static_cast<VkImageView>(rt->GetDepthStencilView_MultiView())
				: static_cast<VkImageView>(rt->GetDepthStencilView(m_pso.renderTarget_ArrayIndex));
			attachment_depth_stencil.imageLayout = GetVkImageLayout(GetImageLayout(rt, 0));
			// depth load op derived from the pso clearDepth sentinel
			attachment_depth_stencil.loadOp = GetDepthLoadOp(m_pso.clearDepth);
			attachment_depth_stencil.storeOp = m_pso.depthStencil_State->IsDepthWriteEnabled() ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_NONE;
			attachment_depth_stencil.clearValue.depthStencil.depth   = m_pso.clearDepth;
			attachment_depth_stencil.clearValue.depthStencil.stencil = m_pso.clearStencil;

			renderInfo.pDepthAttachment = &attachment_depth_stencil;

			// we are using the combined depth-stencil approach
			// this means we can assign the depth attachment as the stencil attachment
			if (m_pso.renderTarget_DepthTexture->IsStencilFormat())
			{
				renderInfo.pStencilAttachment = renderInfo.pDepthAttachment;
			}
		}

		// variable rate shading
		VkRenderingFragmentShadingRateAttachmentInfoKHR attachment_shading_rate = {};
		if (m_pso.vrsInputTexture)
		{
			m_pso.vrsInputTexture->SetLayout(Layout::ImageLayout::ShadingRateAttachment, this);
	
			const Ref<Device> device = RenderContext::Get()->GetDevice();
			const DeviceStatics stats = device->GetDeviceStatics();

			attachment_shading_rate.sType                          = VK_STRUCTURE_TYPE_RENDERING_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR;
			attachment_shading_rate.imageView                      = static_cast<VkImageView>(m_pso.vrsInputTexture->GetRenderTargetView());
			attachment_shading_rate.imageLayout                    = GetVkImageLayout(GetImageLayout(m_pso.vrsInputTexture, 0));
			// Use DeviceStatics populated during device initialization for max texel sizes
			attachment_shading_rate.shadingRateAttachmentTexelSize = {
				.width  = stats.maxShadingRateTexelSizeX,
				.height = stats.maxShadingRateTexelSizeY 
			};
	
			renderInfo.pNext = &attachment_shading_rate;
		}
	
		// begin dynamic render pass
		FlushBarriers();
		vkCmdBeginRendering(static_cast<VkCommandBuffer>(m_CmdBuffer), &renderInfo);

		// set dynamic states
		{
			// variable rate shading
			Device::SetVariableRateShading(this, m_pso.vrsInputTexture != nullptr);
	
			// Set viewport
			Viewport viewport;
			viewport.width  = static_cast<float>(m_pso.GetWidth());
			viewport.height = static_cast<float>(m_pso.GetHeight());
			SetViewport(viewport);
		}

		// reset
		m_Load_Depth_RenderTarget = false;
		for (uint32_t i = 0; i < MAX_RENDER_TARGET_COUNT; i++)
		{
			m_Load_Color_RenderTargets[i] = false;
		}

		m_RenderPassActive = true;
	}

	void CommandList::SetPipelineState(PipelineState& pso)
	{
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to set pipeline state");

		// Compute path is still TODO.
		if (pso.shaders.contains(static_cast<uint32_t>(Stage::Compute)))
		{
			static bool warnedCompute = false;
			if (!warnedCompute)
			{
				SEDX_CORE_WARN_TAG("CommandList", "SetPipelineState: compute pipeline binding not yet wired");
				warnedCompute = true;
			}
			return;
		}

		ImageResource* colorTarget = pso.renderTarget_ColorTextures[0];

		// If there is no explicit color target but a render pass is already active on
		// the current command buffer (e.g. injected via SetExternalRecordingBuffer for
		// UI rendering inside RecordRenderCommands), skip the EndRenderPass /
		// vkCmdBeginRendering block and only build + bind the pipeline.
		if (!colorTarget && m_RenderPassActive)
		{
			// Store the incoming PSO so PushConstants() and PreDraw() can reference it.
			m_pso = pso;

			// Build and bind the pipeline using the external render pass's format context.
			{
				PipelineState mutablePso = pso;
				m_Pipeline = Pipeline(mutablePso, m_DescriptorLayout_Current);
			}

			if (m_Pipeline.Get() != VK_NULL_HANDLE)
			{
				vkCmdBindPipeline(m_CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline.Get());
			}
			else if (Ref<RenderContext> context = RenderContext::Get(); context && context->pipeline != VK_NULL_HANDLE)
			{
				vkCmdBindPipeline(m_CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->pipeline);
			}
			return;
		}

		if (!colorTarget)
			return;

		// Layout transitions are emitted via vkCmdPipelineBarrier2, which must be
		// outside an active dynamic rendering instance unless special features are enabled.
		// Close the current render pass before transitioning attachments.
		EndRenderPass();

		// Transition the color attachment to VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL.
		colorTarget->SetLayout(Layout::ImageLayout::Attachment, this, ALL_MIPS, 0);
		VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		colorAttachment.imageView   = colorTarget->GetImageView();
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
		colorAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;

		VkRenderingAttachmentInfo depthAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		if (ImageResource* depthTarget = pso.renderTarget_DepthTexture)
		{
			// Depth attachments must use DepthStencilAttachment layout, not the generic
			// Attachment (color) layout.  The barrier for DepthStencilAttachment carries
			// EARLY/LATE_FRAGMENT_TESTS stages and DEPTH_STENCIL_ATTACHMENT_READ|WRITE
			// access masks, which eliminates the READ_AFTER_WRITE hazard on the depth
			// attachment reported by validation (VE-5).
			depthTarget->SetLayout(Layout::ImageLayout::DepthStencilAttachment, this, ALL_MIPS, 0);
			depthAttachment.imageView   = depthTarget->GetImageView();
			depthAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
			depthAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_LOAD;
			depthAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.clearValue.depthStencil.depth   = pso.clearDepth;
			depthAttachment.clearValue.depthStencil.stencil = pso.clearStencil;
		}

		// Store the incoming PSO so PushConstants() and PreDraw() can reference it.
		m_pso = pso;

		// Build a format-correct VkPipeline from the PSO.
		// Pipeline() handles both the full bindless path (when m_DescriptorLayout_Current is set)
		// and the minimal bootstrap path (layout = nullptr), centralising all Vulkan pipeline
		// creation in the Pipeline class.
		{
			PipelineState mutablePso = pso;
			m_Pipeline = Pipeline(mutablePso, m_DescriptorLayout_Current);
		}

		VkRenderingInfo renderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
		renderingInfo.renderArea.offset    = { 0, 0 };
		renderingInfo.renderArea.extent    = { colorTarget->GetWidth(), colorTarget->GetHeight() };
		renderingInfo.layerCount           = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments    = &colorAttachment;
		renderingInfo.pDepthAttachment     = (depthAttachment.imageView != VK_NULL_HANDLE) ? &depthAttachment : nullptr;
		vkCmdBeginRendering(m_CmdBuffer, &renderingInfo);
		m_RenderPassActive = true;

		// After vkCmdBeginRendering the driver has implicitly transitioned the attachments.
		// Stamp the tracked layouts to match the physical GPU state so that post-render
		// barriers (e.g. the TransferSrc barrier in Blit) use correct srcStageMask /
		// srcAccessMask and do not produce WRITE_AFTER_WRITE validation errors.
		SetLayout(*colorTarget->Get(), 0, MAX_MIP_COUNT, Layout::ImageLayout::Attachment);
		if (pso.renderTarget_DepthTexture)
		{
			SetLayout(*pso.renderTarget_DepthTexture->Get(), 0, MAX_MIP_COUNT, Layout::ImageLayout::DepthStencilAttachment);
		}

		if (m_Pipeline.Get() != VK_NULL_HANDLE)
		{
			vkCmdBindPipeline(m_CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline.Get());
		}
		else
		{
			// Fallback to the bootstrap pipeline from the render context while the
			// descriptor layout / format-correct pipeline is being wired up.
			if (Ref<RenderContext> context = RenderContext::Get(); context && context->pipeline != VK_NULL_HANDLE)
				vkCmdBindPipeline(m_CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->pipeline);
		}
	}

	void CommandList::PushConstants(const PushConstantBuffer_Pass& data)
	{
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to push constants");

		// Prefer the layout from the PSO-driven pipeline built in SetPipelineState();
		// fall back to the bootstrap layout stored in the render context.
		VkPipelineLayout layout = m_Pipeline.GetLayout();
		if (layout == VK_NULL_HANDLE)
		{
			if (Ref<RenderContext> context = RenderContext::Get())
				layout = context->pipelineLayout;
		}

		if (layout == VK_NULL_HANDLE)
		{
			static bool warnedOnce = false;
			if (!warnedOnce)
			{
				SEDX_CORE_WARN_TAG("CommandList", "PushConstants: no pipeline layout available — skipping push constant upload");
				warnedOnce = true;
			}
			return;
		}

		// Derive the stage flags from the compiled pipeline's push-constant reflection;
		// default to VS|FS if reflection data is absent (covers the grid shader case).
		const uint32_t stages = m_Pipeline.GetPushConstantStages() != 0
			? m_Pipeline.GetPushConstantStages()
			: static_cast<uint32_t>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

		vkCmdPushConstants(
			m_CmdBuffer,
			layout,
			stages,
			0,
			static_cast<uint32_t>(sizeof(PushConstantBuffer_Pass)),
			&data);
	}

	void CommandList::SetBuffer(const uint32_t slot, Buffer *buffer) const 
	{
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to bind a buffer");

		if (!m_DescriptorLayout_Current)
		{
			SEDX_CORE_WARN_TAG("CommandList","Descriptor layout not set, try setting buffer \"{}\" within a render pass", buffer->GetObjectName().c_str());
			return;
		}

		m_DescriptorLayout_Current->SetBuffer(slot, buffer);

		m_NeedsDynamicBind = true;

		/* TODO: Bind structured/storage buffer to the UAV slot in the active descriptor set. */
		//SEDX_CORE_WARN_TAG("CommandList", "SetBuffer: stub — descriptor update not yet wired");
	}

	void CommandList::SetVertexBuffer(Buffer *vertexBuffer)
	{
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to set vertex buffer");
		SEDX_CORE_ASSERT(vertexBuffer != nullptr, "Vertex buffer must be valid");

		const VkBuffer buf = vertexBuffer->Get();
		const VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(m_CmdBuffer, 0, 1, &buf, &offset);
	}
	
	void CommandList::SetVertexBuffer(const Buffer *vertexBuffer, const Buffer *instance)
	{
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command List must be in Recording state to set vertex buffer.");
		SEDX_CORE_ASSERT(m_CmdBuffer != VK_NULL_HANDLE, "Command List has no active VkCommandBuffer");
		SEDX_CORE_ASSERT(vertexBuffer != nullptr, "Vertex buffer must be valid");

		// the instance buffer is optional but always part of the pipeline therefore it can't be null
		if (!instance)
		{
			instance = Renderer::GetBuffer(Renderer_Buffer::DummyInstance);
		}

		// prepare buffers and offsets arrays
		VkBuffer vertexBuffers[2] = {
	
			(vertexBuffer->Get()),  // slot 0: vertex buffer
			(instance->Get())		// slot 1: instance buffer
		};
		SEDX_CORE_ASSERT(vertexBuffers[0] != nullptr && vertexBuffers[1] != nullptr, "Vertex and instance buffers must be valid");

		VkDeviceSize offsets[2] = { 0, 0 };

		// check if vertex buffer id has changed to trigger binding
		if (m_BufferID_Vertex != vertexBuffer->GetObjectId() || m_BufferID_Instance != instance->GetObjectId())
		{
			vkCmdBindVertexBuffers(
				static_cast<VkCommandBuffer>(m_CmdBuffer),	// commandBuffer
				0,                                          // firstBinding
				2,                                          // bindingCount
				vertexBuffers,                              // pBuffers
				offsets                                     // pOffsets
			);
	
			// track currently bound buffers
			m_BufferID_Vertex   = vertexBuffer->GetObjectId();
			m_BufferID_Instance = instance->GetObjectId();
		}

		/*
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(m_CmdBuffer, 0, 1, &vertexBufferHandle, &offset);
		*/
	}

	void CommandList::SetIndexBuffer(Buffer *indexBuffer)
	{
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to set index buffer");
		SEDX_CORE_ASSERT(indexBuffer != nullptr, "Index buffer must be valid");

		vkCmdBindIndexBuffer(m_CmdBuffer, indexBuffer->Get(), 0, VK_INDEX_TYPE_UINT32);
	}
	
	void CommandList::SetIndexBuffer(const Buffer *indexBuffer)
	{
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command List must be in Recording state to set index buffer.");
		SEDX_CORE_ASSERT(indexBuffer != nullptr, "Index buffer must be valid");
		SEDX_CORE_ASSERT(indexBuffer->Get() != nullptr, "Index buffer must have a valid buffer");
		if (m_BufferID_Index == indexBuffer->GetObjectId())
			return;

		// TODO: Support 32-bit index buffers if needed. For now, we assume all index buffers are 16-bit, which is common for most meshes and saves memory bandwidth.
		bool is16Bit = indexBuffer->GetStride() == sizeof(uint16_t);

		vkCmdBindIndexBuffer(
			m_CmdBuffer,          // commandBuffer
			indexBuffer->Get(),   // buffer
			0,                    // offset
			is16Bit ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32 // indexType
		);

		m_BufferID_Index = indexBuffer->GetObjectId();
	}

	void CommandList::DrawIndexedIndirectCount(Buffer* drawArgs, uint64_t argsOffset, Buffer* countBuffer, uint64_t countOffset, uint32_t maxDrawCount)
	{
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state for indirect draw");
		SEDX_CORE_ASSERT(drawArgs != nullptr && countBuffer != nullptr, "Indirect draw buffers must be valid");

		PreDraw();

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
	 if (m_State != CommandState::Recording)
		{
			SEDX_CORE_WARN_TAG("CommandList", "InsertBarrier(ImageResource) called while '{}' is not recording (state={}), skipping", m_ObjectName.c_str(), static_cast<int>(m_State.load()));
			return;
		}

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

	void CommandList::InsertBarrier(VkImage* imagePtr, VkFormat format, uint32_t mipIndex, uint32_t mipRange, uint32_t arrayLength, Layout::ImageLayout layout)
	{
		if (imagePtr == nullptr || *imagePtr == VK_NULL_HANDLE)
			return;
	
		InsertBarrier(*imagePtr, format, mipIndex, mipRange, arrayLength, layout);
	}
	
	void CommandList::InsertBarrier(VkImage* imgPtr, Layout::ImageLayout layout, uint32_t mip, uint32_t mipRange)
	{
		if (imgPtr == nullptr || *imgPtr == VK_NULL_HANDLE)
			return;
	
		InsertBarrier(*imgPtr, layout, mip, mipRange);
	}

	void CommandList::InsertBarrier(VkImage img, Layout::ImageLayout layout, uint32_t mip, uint32_t mipRange)
	{
		// Thin wrapper: call the detailed InsertBarrier variant with undefined format/arrayLength
		if (img == VK_NULL_HANDLE)
			return;
	
		// Use VK_FORMAT_UNDEFINED; the detailed implementation will default to color aspect if needed
		InsertBarrier(img, VK_FORMAT_UNDEFINED, mip, mipRange, 0, layout);
	}

	void CommandList::Blit(ImageResource *src, Swapchain *dst)
	{
		SEDX_CORE_ASSERT((src->GetFlags() & ImageResourceFlags::BlitClear) != 0, "The image resource needs the BlitClear flag");
		SEDX_CORE_ASSERT(src->GetWidth() <= dst->GetWidth() && src->GetHeight() <= dst->GetHeight(),
			"The source image dimension(s) are larger than the those of the destination image");

		VkOffset3D srcBlitSize = {};
		srcBlitSize.x          = src->GetWidth();
		srcBlitSize.y          = src->GetHeight();
		srcBlitSize.z          = 1;

		VkOffset3D destBlitSize = {};
		destBlitSize.x          = dst->GetWidth();
		destBlitSize.y          = dst->GetHeight();
		destBlitSize.z          = 1;

		VkImageBlit blit_region                   = {};
		blit_region.srcSubresource.mipLevel       = 0;
		blit_region.srcSubresource.baseArrayLayer = 0;
		blit_region.srcSubresource.layerCount     = 1;
		blit_region.srcSubresource.aspectMask     = GetAspectMaskFromFormat(src->GetFormat());
		blit_region.srcOffsets[0]                 = {0, 0, 0 };
		blit_region.srcOffsets[1]                 = srcBlitSize;
		blit_region.dstSubresource.mipLevel       = 0;
		blit_region.dstSubresource.baseArrayLayer = 0;
		blit_region.dstSubresource.layerCount     = 1;
		blit_region.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		blit_region.dstOffsets[0]                 = {0, 0, 0 };
		blit_region.dstOffsets[1]                 = destBlitSize;

		// save the initial layout
		Layout::ImageLayout srcInitLayout = GetImageLayout(src, 0);

		// transition to blit appropriate layouts
		src->SetLayout(Layout::ImageLayout::TransferSrc, this);
		const uint32_t imgIndex = dst->GetImageIndex();
		// Avoid taking address of a possibly-temporary vector element (which can
		// produce a dangling pointer). Copy the VkImage handle into a local and
		// use the overload that accepts a VkImage value.
		VkImage dstImage = dst->GetImages()[imgIndex];
		InsertBarrier(dstImage, dst->GetImageFormat(), 0, 1, 1, Layout::ImageLayout::TransferDst);

		// deduce filter
		bool widthEqual  = src->GetWidth() == dst->GetWidth();
		bool heightEqual = src->GetHeight() == dst->GetHeight();
		VkFilter filter = widthEqual && heightEqual ? VkFilter::VK_FILTER_NEAREST : VkFilter::VK_FILTER_LINEAR;

		// blit
		vkCmdBlitImage(static_cast<VkCommandBuffer>(m_CmdBuffer),
			*src->Get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			dstImage,  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit_region, filter);

		// transition to the initial layouts
		src->SetLayout(srcInitLayout, this);
		// Reuse the previously-captured handle rather than taking the address of
		// a potentially temporary vector element.
		InsertBarrier(dstImage, dst->GetImageFormat(), 0, 1, 1, Layout::ImageLayout::Present);
	}

	void CommandList::Blit(ImageResource *src, ImageResource *dst, const bool blitMips, const float sourceScaling)
	{
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command list must be in recording state to blit");

		SEDX_CORE_ASSERT(src && dst, "Source and destination images cannot be null");
		SEDX_CORE_ASSERT((src->GetFlags() & ImageResourceFlags::BlitClear) != 0, "Blit requires the texture to be created with the RHI_Texture_ClearOrBlit flag");
		SEDX_CORE_ASSERT((dst->GetFlags() & ImageResourceFlags::BlitClear) != 0, "Blit requires the texture to be created with the RHI_Texture_ClearOrBlit flag");
		SEDX_CORE_ASSERT(src->GetChannelCount() == dst->GetChannelCount(), "Source and destination images must have the same number of channels");
		SEDX_CORE_ASSERT(src->GetBitsPerChannel() == dst->GetBitsPerChannel() || (src->IsColorFormat() && dst->IsColorFormat()), "Source and destination bit depths must match or be convertible color formats");
		SEDX_CORE_ASSERT(!src->IsDepthFormat() || !dst->IsDepthFormat() || src->GetFormat() == dst->GetFormat(), "Depth formats must be identical for blit");
		if (blitMips)
		{
			SEDX_CORE_ASSERT(src->GetMipCount() == dst->GetMipCount(), "If the mips are blitted, then the mip count between the source and the destination textures must match");
		}

		/*
		// vkCmdBlitImage and image layout transitions must run outside an active
		// dynamic rendering instance.
		EndRenderPass();

		// Transition full mip ranges to keep layout tracking consistent with Vulkan state.
		// The layout tracker is currently image-wide (not per-mip), so transitioning only mip 0
		// can lead to validation mismatches on higher mips.
		InsertBarrier(src->Get(), src->GetImageSpec().format, 0, 0, 0, Layout::ImageLayout::TransferSrc);
		InsertBarrier(dst->Get(), dst->GetImageSpec().format, 0, 0, 0, Layout::ImageLayout::TransferDst);
		*/
		
		// compute a blit region for each mip
		std::array<VkOffset3D,  MAX_MIP_COUNT> blitOffsetsSrc = {};
		std::array<VkOffset3D,  MAX_MIP_COUNT> blitOffsetsDst = {};
		std::array<VkImageBlit, MAX_MIP_COUNT> blitRegions	  = {};
		uint32_t blitRegionCount = blitMips ? src->GetMipCount() : 1;
		for (uint32_t mipIndex = 0; mipIndex < blitRegionCount; mipIndex++)
		{
			VkOffset3D& srcBlitSize						= blitOffsetsSrc[mipIndex];
			srcBlitSize.x								= static_cast<int32_t>(src->GetWidth()  * sourceScaling) >> mipIndex;
			srcBlitSize.y								= static_cast<int32_t>(src->GetHeight() * sourceScaling) >> mipIndex;
			srcBlitSize.z								= 1;

			VkOffset3D& dstBlitSize						= blitOffsetsDst[mipIndex];
			dstBlitSize.x								= dst->GetWidth()  >> mipIndex;
			dstBlitSize.y								= dst->GetHeight() >> mipIndex;
			dstBlitSize.z								= 1;

			VkImageBlit& blitRegion						= blitRegions[mipIndex];
			blitRegion.srcSubresource.mipLevel			= mipIndex;
			blitRegion.srcSubresource.baseArrayLayer	= 0;
			blitRegion.srcSubresource.layerCount		= 1;
			blitRegion.srcSubresource.aspectMask		= GetAspectMask(src->GetFormat());
			blitRegion.srcOffsets[0]					= {.x = 0, .y = 0, .z = 0 };
			blitRegion.srcOffsets[1]					= srcBlitSize;
			blitRegion.dstSubresource.mipLevel			= mipIndex;
			blitRegion.dstSubresource.baseArrayLayer	= 0;
			blitRegion.dstSubresource.layerCount		= 1;
			blitRegion.dstSubresource.aspectMask		= GetAspectMask(dst->GetFormat());
			blitRegion.dstOffsets[0]					= {.x = 0, .y = 0, .z = 0 };
			blitRegion.dstOffsets[1]					= dstBlitSize;
		}

		// save the initial layouts
		std::array<Layout::ImageLayout, MAX_MIP_COUNT> initSrcLayout = src->GetLayouts();
		std::array<Layout::ImageLayout, MAX_MIP_COUNT> initDstLayout = dst->GetLayouts();

		// transition to blit appropriate layouts
		src->SetLayout(Layout::ImageLayout::TransferSrc, this);
		dst->SetLayout(Layout::ImageLayout::TransferDst, this);

		VkFilter filter = (src->IsDepthFormat() || dst->IsDepthFormat() ||
			(src->GetWidth() == dst->GetWidth() && src->GetHeight() == dst->GetHeight())) ? VK_FILTER_NEAREST  : VK_FILTER_LINEAR;

		const Ref<Device> device = RenderContext::Get()->GetDevice();
		vkCmdBlitImage(m_CmdBuffer,
			*src->Get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			*dst->Get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			blitRegionCount, &blitRegions[0], filter);

		// transition to the initial layouts
		if (blitMips)
		{
			for (uint32_t i = 0; i < src->GetMipCount(); i++)
			{
				src->SetLayout(initSrcLayout[i], this, i, 1);
				dst->SetLayout(initDstLayout[i], this, i, 1);
			}
		}
		else
		{
			src->SetLayout(initSrcLayout[0], this);
			dst->SetLayout(initDstLayout[0], this);
		}
	}
	
	void CommandList::BlitToArrayLayer(ImageResource* src, ImageResource* dst, uint32_t dstLayer)
	{
		SEDX_CORE_ASSERT(src && dst, "Source and destination images must be valid for blit");
		SEDX_CORE_ASSERT((src->GetFlags() & ImageResourceFlags::BlitClear) != 0, "Source image must have BlitClear flag");
		SEDX_CORE_ASSERT((dst->GetFlags() & ImageResourceFlags::BlitClear) != 0, "Destination image must have BlitClear flag");

		Layout::ImageLayout initialSrcLayout = GetImageLayout(src, 0);
		Layout::ImageLayout initialDstLayout = GetImageLayout(dst, 0);

		src->SetLayout(Layout::ImageLayout::TransferSrc, this);
		dst->SetLayout(Layout::ImageLayout::TransferDst, this);
		FlushBarriers();

		VkImageBlit blitRegion = {};
		blitRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion.srcSubresource.mipLevel       = 0;
		blitRegion.srcSubresource.baseArrayLayer = 0;
		blitRegion.srcSubresource.layerCount     = 1;
		blitRegion.srcOffsets[0]                 = {.x = 0, .y = 0, .z = 0 };
		blitRegion.srcOffsets[1]                 = {
			.x = static_cast<int32_t>(src->GetWidth()),
			.y = static_cast<int32_t>(src->GetHeight()),
			.z = 1 
		};
		blitRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		blitRegion.dstSubresource.mipLevel       = 0;
		blitRegion.dstSubresource.baseArrayLayer = dstLayer;
		blitRegion.dstSubresource.layerCount     = 1;
		blitRegion.dstOffsets[0]                 = {.x = 0, .y = 0, .z = 0 };
		blitRegion.dstOffsets[1]                 = {
			.x = static_cast<int32_t>(dst->GetWidth()),
			.y = static_cast<int32_t>(dst->GetHeight()),
			.z = 1 
		};

		vkCmdBlitImage(m_CmdBuffer,
			*src->Get(),VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			*dst->Get(),VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blitRegion, VK_FILTER_LINEAR);

		src->SetLayout(initialSrcLayout, this);
		dst->SetLayout(initialDstLayout, this);
	}

} // namespace SceneryEditorX

// -------------------------------------------------------

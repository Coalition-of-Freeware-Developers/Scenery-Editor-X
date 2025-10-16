/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* command_manager.cpp
* -------------------------------------------------------
* Created: 25/8/2025
* -------------------------------------------------------
*/
#include "command_manager.h"
#include "render_context.h"
#include "vulkan/vk_util.h"

/// -------------------------------------------------------

/// -------------------------------------------------------

// Active implementations (outside of legacy commented block)
namespace SceneryEditorX
{
	CommandPool::CommandPool(const Ref<VulkanDevice>& vulkanDevice, Queue type)
	{
		queueType = type;

		const VkDevice device = vulkanDevice->GetDevice();
		const auto& qIndices = vulkanDevice->GetPhysicalDevice()->GetQueueFamilyIndices();

		// Graphics pool
		VkCommandPoolCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		ci.queueFamilyIndex = qIndices.GetGraphicsFamily();
		if (VkResult res = vkCreateCommandPool(device, &ci, nullptr, &GraphicsCmdPool); res != VK_SUCCESS)
		{
			SEDX_CORE_ERROR_TAG("VULKAN", "Failed to create graphics command pool (err {0})", res);
			GraphicsCmdPool = VK_NULL_HANDLE;
		}

		// Compute pool (fallback to graphics if same family or create fails)
		ci.queueFamilyIndex = qIndices.GetComputeFamily();
		if (ci.queueFamilyIndex != qIndices.GetGraphicsFamily())
		{
			if (VkResult res = vkCreateCommandPool(device, &ci, nullptr, &ComputeCmdPool); res != VK_SUCCESS)
			{
				SEDX_CORE_WARN_TAG("VULKAN", "Failed to create compute command pool (err {0}), using graphics pool", res);
				ComputeCmdPool = GraphicsCmdPool;
			}
		}
		else
		{
			ComputeCmdPool = GraphicsCmdPool;
		}

		// Transfer pool (optional, fallback to graphics)
		ci.queueFamilyIndex = qIndices.GetTransferFamily();
		if (ci.queueFamilyIndex != qIndices.GetGraphicsFamily() && ci.queueFamilyIndex != qIndices.GetComputeFamily())
		{
			if (VkResult res = vkCreateCommandPool(device, &ci, nullptr, &TransferCmdPool); res != VK_SUCCESS)
			{
				SEDX_CORE_WARN_TAG("VULKAN", "Failed to create transfer command pool (err {0}), using graphics pool", res);
				TransferCmdPool = GraphicsCmdPool;
			}
		}
		else
		{
			TransferCmdPool = GraphicsCmdPool;
		}
	}

	CommandPool::~CommandPool()
	{
		const auto deviceRef = RenderContext::GetCurrentDevice();
		if (!deviceRef)
			return;

		const VkDevice device = deviceRef->GetDevice();

		if (TransferCmdPool != VK_NULL_HANDLE && TransferCmdPool != GraphicsCmdPool && TransferCmdPool != ComputeCmdPool)
			vkDestroyCommandPool(device, TransferCmdPool, nullptr);

		if (ComputeCmdPool != VK_NULL_HANDLE && ComputeCmdPool != GraphicsCmdPool)
			vkDestroyCommandPool(device, ComputeCmdPool, nullptr);

		if (GraphicsCmdPool != VK_NULL_HANDLE)
			vkDestroyCommandPool(device, GraphicsCmdPool, nullptr);

		GraphicsCmdPool = VK_NULL_HANDLE;
		ComputeCmdPool = VK_NULL_HANDLE;
		TransferCmdPool = VK_NULL_HANDLE;
	}

	VkCommandBuffer CommandPool::AllocateCommandBuffer(bool begin, bool compute) const
	{
		const auto deviceRef = RenderContext::GetCurrentDevice();
		SEDX_CORE_ASSERT(deviceRef, "VulkanDevice must be valid to allocate command buffers");
		const VkDevice device = deviceRef->GetDevice();
		const VkCommandPool pool = compute ? ComputeCmdPool : GraphicsCmdPool;

		VkCommandBufferAllocateInfo ai{};
		ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		ai.commandPool = pool;
		ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		ai.commandBufferCount = 1;

		VkCommandBuffer cmd = VK_NULL_HANDLE;
		if (VkResult res = vkAllocateCommandBuffers(device, &ai, &cmd); res != VK_SUCCESS)
		{
			SEDX_CORE_ERROR_TAG("VULKAN", "vkAllocateCommandBuffers failed (err {0})", res);
			return VK_NULL_HANDLE;
		}

		if (begin)
		{
			VkCommandBufferBeginInfo bi{};
			bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			if (VkResult res = vkBeginCommandBuffer(cmd, &bi); res != VK_SUCCESS)
			{
				SEDX_CORE_ERROR_TAG("VULKAN", "vkBeginCommandBuffer failed (err {0})", res);
				vkFreeCommandBuffers(device, pool, 1, &cmd);
				return VK_NULL_HANDLE;
			}
		}

		return cmd;
	}

	void CommandPool::FlushCmdBuffer(VkCommandBuffer cmdBuffer) const
	{
		const auto deviceRef = RenderContext::GetCurrentDevice();
		SEDX_CORE_ASSERT(deviceRef, "No VulkanDevice available");
		VkDevice device = deviceRef->GetDevice();
		VkQueue queue = deviceRef->GetGraphicsQueue();

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));

		VkSubmitInfo si{};
		si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		si.commandBufferCount = 1;
		si.pCommandBuffers = &cmdBuffer;

		VkFenceCreateInfo fi{};
		fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		VkFence fence = VK_NULL_HANDLE;
		VK_CHECK_RESULT(vkCreateFence(device, &fi, nullptr, &fence));

		if (VkResult res = vkQueueSubmit(queue, 1, &si, fence); res != VK_SUCCESS)
			SEDX_CORE_ERROR_TAG("VULKAN", "vkQueueSubmit failed (err {0})", res);

		vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
		vkDestroyFence(device, fence, nullptr);

		vkFreeCommandBuffers(device, GraphicsCmdPool, 1, &cmdBuffer);
	}

	void CommandPool::FlushCmdBuffer(VkCommandBuffer cmdBuffer, VkQueue queue) const
	{
		const auto deviceRef = RenderContext::GetCurrentDevice();
		SEDX_CORE_ASSERT(deviceRef, "No VulkanDevice available");
		VkDevice device = deviceRef->GetDevice();

		if (cmdBuffer == VK_NULL_HANDLE)
			return;

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));

		VkSubmitInfo si{};
		si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		si.commandBufferCount = 1;
		si.pCommandBuffers = &cmdBuffer;

		VkFenceCreateInfo fi{};
		fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		VkFence fence = VK_NULL_HANDLE;
		VK_CHECK_RESULT(vkCreateFence(device, &fi, nullptr, &fence));

		if (VkResult res = vkQueueSubmit(queue, 1, &si, fence); res != VK_SUCCESS)
			SEDX_CORE_ERROR_TAG("VULKAN", "vkQueueSubmit (explicit queue) failed (err {0})", res);

		vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
		vkDestroyFence(device, fence, nullptr);

		vkFreeCommandBuffers(device, GraphicsCmdPool, 1, &cmdBuffer);
	}
}

/// -------------------------------------------------------

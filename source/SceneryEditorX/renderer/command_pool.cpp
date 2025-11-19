/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* command_pool.cpp
* -------------------------------------------------------
* Created: 16/11/2025
* -------------------------------------------------------
*/
#include "command_pool.h"
#include "render_context.h"
#include "SceneryEditorX/logging/logging.hpp"

// -------------------------------------------------------

namespace SceneryEditorX
{
	static uint32_t GetPoolType(Queue queueType)
	{
	    const auto &indices = RenderContext::Get()->GetPhysicalDevice()->GetQueueFamilyIndices();
	    switch (queueType)
	    {
			case Queue::Graphics: return indices.GetGraphicsFamily();
			case Queue::Compute: return indices.GetComputeFamily();
			case Queue::Transfer: return indices.GetTransferFamily();
			case Queue::Present: return indices.GetPresentFamily();
			default: return indices.GetGraphicsFamily();
	    }
	}
	
	static std::string ToString(Queue queueType)
	{
	    switch (queueType)
	    {
			case Queue::Graphics: return "Graphics";
			case Queue::Compute: return "Compute";
			case Queue::Transfer: return "Transfer";
			case Queue::Present: return "Present";
			default: return "Unknown";
	    }
	}

    /**
	 * @brief Static accessor method to get the singleton instance of CommandPool
	 * @return Reference to the singleton CommandPool instance
	 */
    Ref<CommandPool> CommandPool::Get()
    {
        return CreateRef<CommandPool>(RenderContext::Get()->GetLogicDevice(), Queue::Graphics);
    }

	CommandPool::CommandPool(const Ref<VulkanDevice> &vulkanDevice, Queue type)
	{
	    m_QueueType = type;
	    const uint32_t familyIndex = GetPoolType(type);
	    const VkDevice device = vulkanDevice->GetDevice();
	
	    VkCommandPoolCreateInfo ci{};
	    ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	    ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	    ci.queueFamilyIndex = familyIndex;
	    if (VkResult res = vkCreateCommandPool(device, &ci, nullptr, &m_CmdPool); res != VK_SUCCESS)
	    {
	        SEDX_CORE_ERROR_TAG("VULKAN", "Failed to create {0} command pool (err {1})", ToString(m_QueueType), res);
	        m_CmdPool = VK_NULL_HANDLE;
	    }
	
		#ifdef SEDX_DEBUG
		    SEDX_CORE_INFO_TAG("VULKAN", "{0} command pool created successfully", ToString(m_QueueType));
		#endif
	}
	
	CommandPool::~CommandPool()
	{
	    const auto deviceRef = RenderContext::GetCurrentDevice();
	    if (!deviceRef)
	        return;
	
	    const VkDevice device = deviceRef->GetDevice();
	
	    if (m_CmdPool != VK_NULL_HANDLE)
	    {
	        vkDestroyCommandPool(device, m_CmdPool, nullptr);
	    }
	
	    m_CmdPool = VK_NULL_HANDLE;
		#ifdef SEDX_DEBUG
		    SEDX_CORE_INFO_TAG("VULKAN", "{0} command pool destroyed successfully", ToString(m_QueueType));
		#endif
	}
	
}

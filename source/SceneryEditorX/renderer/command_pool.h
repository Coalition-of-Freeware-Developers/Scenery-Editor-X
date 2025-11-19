/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* command_pool.h
* -------------------------------------------------------
* Created: 16/11/2025
* -------------------------------------------------------
*/
#pragma once
#include "SceneryEditorX/utils/pointers.h"
#include "vulkan/vk_device.h"
#include "vulkan/vk_enums.h"
#include <vulkan/vulkan.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class CommandPool : public RefCounted
	{
	public:
	    /**
		 * @brief Create command pools for a device
		 * @param vulkanDevice The device to create command pools for
		 * @param type The type of queue this command pool will be used with (graphics, compute, transfer, etc.)
		 */
	    CommandPool(const Ref<VulkanDevice> &vulkanDevice, Queue type);
	    virtual ~CommandPool() override;

        static Ref<CommandPool> Get(); // Static accessor method to get the singleton instance
		[[nodiscard]] VkCommandPool GetCmdPool() const { return m_CmdPool; } 
	
	private:
	    Queue m_QueueType;
        VkCommandPool m_CmdPool = VK_NULL_HANDLE;
	};
	
}

// -------------------------------------------------------

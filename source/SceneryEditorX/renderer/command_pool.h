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
#include "device.h"
#include "enums.h"
#include "SceneryEditorX/utils/pointers.h"
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
		 * @param name The name of the command pool (for debugging purposes)
		 */
	    CommandPool(const Ref<VulkanDevice> &vulkanDevice, Queue type, const std::string& name);
	    virtual ~CommandPool() override;

		CommandPool &operator=(const CommandPool &)		= delete; // Delete copy assignment operator
        CommandPool(const CommandPool &)				= delete; // Delete copy constructor
        CommandPool &operator=(CommandPool &&) noexcept = delete; // Delete move assignment operator
        CommandPool(CommandPool &&) noexcept			= delete; // Delete move constructor

        static Ref<CommandPool> Get(); // Static accessor method to get the singleton instance
		[[nodiscard]] VkCommandPool GetCmdPool() const { return m_CmdPool; } 
		Queue GetQueueType() const { return m_QueueType; }
	
	private:
        std::string m_Name;
	    Queue m_QueueType = MaxEnum;
        VkCommandPool m_CmdPool = VK_NULL_HANDLE;
	};
	
}

// -------------------------------------------------------

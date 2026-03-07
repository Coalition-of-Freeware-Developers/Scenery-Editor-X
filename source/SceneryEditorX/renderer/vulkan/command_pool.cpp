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
 * command_pool.cpp
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#include "command_pool.h"
#include "render_context.h"
#include <utility>
#include <vector>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

    static VkCommandPoolCreateFlags GetPoolFlags(CommandPoolType pool)
	{
		switch (pool)
		{
			case CommandPoolType::Transient:  return VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
			case CommandPoolType::Resettable: return VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			case CommandPoolType::Protected:  return VK_COMMAND_POOL_CREATE_PROTECTED_BIT;
			default:
				return 0;
		}
    }

    CommandPool::CommandPool(const Ref<Device>& device, uint32_t queueFamilyIndex, CommandPoolType pool)
    {
        m_Device = device;
        m_PoolType = pool;

        // Validate that device is properly initialized
        SEDX_CORE_ASSERT(m_Device.IsValid(), "Device reference is null");
        SEDX_CORE_ASSERT(m_Device->GetLogicalDevice() != VK_NULL_HANDLE, "Logical device is null - Device::Create() was not called or failed");

        // Validate that volk has loaded device-level functions
        SEDX_CORE_ASSERT(vkCreateCommandPool != nullptr, "vkCreateCommandPool is null - volkLoadDevice() was not called after device creation");

        VkCommandPoolCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        ci.queueFamilyIndex = queueFamilyIndex;
        ci.flags = GetPoolFlags(m_PoolType);

        if (VkResult r = vkCreateCommandPool(m_Device->GetLogicalDevice(), &ci, nullptr, &m_CmdPool); r != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("CommandPool", "Failed to create command pool: VkResult = {}", static_cast<int>(r));
            m_CmdPool = VK_NULL_HANDLE;
        }
        else
        {
            SEDX_CORE_TRACE_TAG("CommandPool", "Command pool created successfully (queue family: {}, type: {})", queueFamilyIndex, static_cast<int>(pool));
        }
    }

    CommandPool::CommandPool(uint32_t queueFamilyIndex, CommandPoolType pool)
    {
        m_Device = RenderContext::Get()->GetDevice();
        m_PoolType = pool;

        // Validate that device is properly initialized
        SEDX_CORE_ASSERT(m_Device.IsValid(), "Device reference is null - RenderContext not initialized");
        SEDX_CORE_ASSERT(m_Device->GetLogicalDevice() != VK_NULL_HANDLE, "Logical device is null - Device::Create() was not called or failed");

        // Validate that volk has loaded device-level functions
        SEDX_CORE_ASSERT(vkCreateCommandPool != nullptr, "vkCreateCommandPool is null - volkLoadDevice() was not called after device creation");

        VkCommandPoolCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        ci.queueFamilyIndex = queueFamilyIndex;
        ci.flags = GetPoolFlags(m_PoolType);

        if (VkResult r = vkCreateCommandPool(m_Device->GetLogicalDevice(), &ci, nullptr, &m_CmdPool); r != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("CommandPool", "Failed to create command pool: VkResult = {}", static_cast<int>(r));
            m_CmdPool = VK_NULL_HANDLE;
        }
        else
        {
            SEDX_CORE_TRACE_TAG("CommandPool", "Command pool created successfully (queue family: {}, type: {})", queueFamilyIndex, static_cast<int>(pool));
        }
    }

    CommandPool::~CommandPool()
    {
        Destroy();
        m_Device.Reset(); // Release reference to device
    }

    CommandPool::CommandPool(CommandPool &&other) noexcept : m_Device(std::exchange(other.m_Device, VK_NULL_HANDLE)), m_CmdPool(std::exchange(other.m_CmdPool, VK_NULL_HANDLE)) {}
	
	CommandPool &CommandPool::operator=(CommandPool &&other) noexcept
    {
        if (this != &other)
        {
            Destroy();
            m_CmdPool = std::exchange(other.m_CmdPool, VK_NULL_HANDLE);
            m_Device = std::exchange(other.m_Device, VK_NULL_HANDLE);
        }

        return *this;
    }
	
	std::vector<VkCommandBuffer> CommandPool::Allocate(uint32_t count) const
    {
        if (m_CmdPool == VK_NULL_HANDLE)
            return {};

        std::vector<VkCommandBuffer> buffers(count);
        VkCommandBufferAllocateInfo allocateInfo = {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = m_CmdPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = count;

        if (VkResult r = vkAllocateCommandBuffers(m_Device->GetLogicalDevice(), &allocateInfo, buffers.data()); r != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("CommandPool", "Failed to allocate command buffers: {}", static_cast<int>(r));
            return {};
        }

        return buffers;
    }
	
	void CommandPool::Destroy()
    {
        if (m_Device.IsValid() && m_CmdPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(m_Device->GetLogicalDevice(), m_CmdPool, nullptr);
            m_CmdPool = VK_NULL_HANDLE;
            SEDX_CORE_TRACE_TAG("CommandPool", "Command pool destroyed");
        }
    }

}

// -------------------------------------------------------

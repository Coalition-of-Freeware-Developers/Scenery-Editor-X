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
 * uniform_bufferset.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "shader_module.h"
#include <array>
#include <vma/vk_mem_alloc.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	// Manages a set of per-frame uniform buffers (ShaderDataBuffer). Uses VMA to
	// allocate buffers that are host-visible and retrieves m_Device addresses for
	// shader m_Device address push-constant usage.
	class UniformBufferSet
	{
	public:
	    UniformBufferSet() = default;
	    UniformBufferSet(VmaAllocator allocator) : m_Device(VK_NULL_HANDLE), m_Allocator(allocator)
	    {
	        Create();
	    }
	
	    ~UniformBufferSet()
	    {
	        // best-effort cleanup; call destroy(m_Device, m_Allocator) explicitly before m_Allocator/m_Device teardown
	        if (!m_Destroyed)
	        {
	            // nothing here
	        }
	    }
	
	    void Create()
	    {
	        if (!m_Allocator || m_Device == VK_NULL_HANDLE)
	            return;

	        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	        {
	            VkBufferCreateInfo uBufferCI
	            {
	                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	                .size = sizeof(ShaderData),
	                .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
	            };

	            VmaAllocationCreateInfo uBufferAllocCI
	            {
	                .flags =  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
					          VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
	                          VMA_ALLOCATION_CREATE_MAPPED_BIT,
	                .usage = VMA_MEMORY_USAGE_AUTO
	            };

                if (VkResult r = vmaCreateBuffer(m_Allocator, &uBufferCI, &uBufferAllocCI, &m_Buffers[i].buffer, &m_Buffers[i].allocation, nullptr); r != VK_SUCCESS)
	            {
                    SEDX_CORE_WARN_TAG("UniformBufferSet","vmaCreateBuffer failed: {}", r);
	                continue;
	            }

	            vmaMapMemory(m_Allocator, m_Buffers[i].allocation, &m_Buffers[i].mapped);
	            VkBufferDeviceAddressInfo uBufferBdaInfo
	            {
	                .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
	                .buffer = m_Buffers[i].buffer
	            };

	            m_Buffers[i].deviceAddress = vkGetBufferDeviceAddress(m_Device->GetLogicalDevice(), &uBufferBdaInfo);
	        }
	    }
	
	    void Destroy()
	    {
	        if (m_Destroyed)
	        {
	            return;
	        }

	        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	        {
	            if (m_Buffers[i].allocation != VK_NULL_HANDLE)
	            {
	                vmaUnmapMemory(m_Allocator, m_Buffers[i].allocation);
	                vmaDestroyBuffer(m_Allocator, m_Buffers[i].buffer, m_Buffers[i].allocation);

	                m_Buffers[i].buffer = VK_NULL_HANDLE;
	                m_Buffers[i].allocation = VK_NULL_HANDLE;
	                m_Buffers[i].mapped = nullptr;
	                m_Buffers[i].deviceAddress = 0;
	            }
	        }

	        m_Destroyed = true;
	    }
	
	    std::array<ShaderDataBuffer, MAX_FRAMES_IN_FLIGHT> &Buffers() { return m_Buffers; }
	
	private:
        Ref<Device> m_Device;
	    VmaAllocator m_Allocator;
	    std::array<ShaderDataBuffer, MAX_FRAMES_IN_FLIGHT> m_Buffers{};
	    bool m_Destroyed = false;
	};
	
} // namespace SceneryEditorX

// -------------------------------------------------------

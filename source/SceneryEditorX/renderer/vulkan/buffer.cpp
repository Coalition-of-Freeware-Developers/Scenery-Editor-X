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
 * buffer.cpp
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
// ReSharper disable CppInconsistentNaming
#include "buffer.h"
#include "render_context.h"
#include <volk/volk.h>

// --------------------------------

namespace SceneryEditorX
{
	Buffer::Buffer(const VmaAllocator allocator, const VkDeviceSize size, const VkBufferUsageFlags usage, const VmaAllocationCreateInfo& allocInfo) :  m_Allocator(allocator)
	{
        VkBufferCreateInfo bufferCI{}; 
	    bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCI.size = size;
	    bufferCI.usage = usage;

	    vmaCreateBuffer(m_Allocator, &bufferCI, &allocInfo, &m_Buffer, &m_Allocation, nullptr);
	}
	
	Buffer::~Buffer()
	{
        SEDX_CORE_ASSERT(m_Buffer == VK_NULL_HANDLE && m_Allocation == VK_NULL_HANDLE,
                         "Buffer destructor called without freeing resources. This may indicate a memory leak. Call "
                         "FreeBuffer() or Destroy() before destruction.");
	    Destroy();
	}
	
	Buffer::Buffer(Buffer&& other) noexcept : m_Buffer(other.m_Buffer), m_Allocation(other.m_Allocation), m_Allocator(other.m_Allocator), 
                                              m_MappedData(other.m_MappedData), m_DeviceAddress(other.m_DeviceAddress)
	{
	    other.m_Buffer = VK_NULL_HANDLE;
	    other.m_Allocation = VK_NULL_HANDLE;
	    other.m_MappedData = nullptr;
	    other.m_DeviceAddress = 0;
	}
	
	Buffer& Buffer::operator=(Buffer&& other) noexcept
	{
	    if (this != &other)
		{
	        Destroy();
	        m_Allocator = other.m_Allocator;
	        m_Buffer = other.m_Buffer;
	        m_Allocation = other.m_Allocation;
	        m_MappedData = other.m_MappedData;
	        m_DeviceAddress = other.m_DeviceAddress;
	
	        other.m_Buffer = VK_NULL_HANDLE;
	        other.m_Allocation = VK_NULL_HANDLE;
	        other.m_MappedData = nullptr;
	        other.m_DeviceAddress = 0;
	    }
	    return *this;
	}
	
	void* Buffer::Map()
	{
	    if (!m_MappedData && m_Allocation != VK_NULL_HANDLE)
		{
	        vmaMapMemory(m_Allocator, m_Allocation, &m_MappedData);
	    }
	    return m_MappedData;
	}
	
	void Buffer::Unmap()
	{
	    if (m_MappedData && m_Allocation != VK_NULL_HANDLE)
		{
	        vmaUnmapMemory(m_Allocator, m_Allocation);
	        m_MappedData = nullptr;
	    }
	}
	
	VkDeviceAddress Buffer::DeviceAddress()
	{        
	    const Ref<Device> device = RenderContext::Get()->GetDevice();
	    if (m_DeviceAddress == 0 && m_Buffer != VK_NULL_HANDLE && device->GetLogicalDevice() != VK_NULL_HANDLE)
		{
            VkBufferDeviceAddressInfo buffDeviceAddInfo{};
	        buffDeviceAddInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	        buffDeviceAddInfo.buffer = m_Buffer;

	        m_DeviceAddress = vkGetBufferDeviceAddress(device->GetLogicalDevice(), &buffDeviceAddInfo);
	    }

	    return m_DeviceAddress;
	}

    void Buffer::FreeImageBuffer(void* &buffer)
    {
	    if (buffer == nullptr || buffer == VK_NULL_HANDLE)
	    {
	        SEDX_CORE_WARN_TAG("Buffer", "FreeBuffer called with null buffer");
	        return;
	    }
	
	    // Get global allocator and the allocation associated with this resource
	    VmaAllocator allocator = MemoryAllocator::GetAllocator();
	    VmaAllocation allocation = MemoryAllocator::GetAllocation(buffer);
	
	    if (allocation == VK_NULL_HANDLE)
	    {
	        SEDX_CORE_ERROR_TAG("Buffer", "No VMA allocation found for buffer {} - cannot free", ToString(buffer));
	        return;
	    }
	    // Destroy the image using the allocator retrieved from MemoryAllocator
	    VkImage buf = static_cast<VkImage>(buffer);
	    vmaDestroyImage(allocator, buf, allocation);

	    // Remove allocation tracking and clear the caller's reference
	    MemoryAllocator::FreeAllocation(buffer);
	    buffer = nullptr;
		SEDX_CORE_TRACE_TAG("Buffer", "Freed buffer {}", ToString(buf));
    }

    void Buffer::FreeBuffer(void *&buffer)
	{
	    if (buffer == nullptr || buffer == VK_NULL_HANDLE)
	    {
	        SEDX_CORE_WARN_TAG("Buffer", "FreeBuffer called with null buffer");
	        return;
	    }
	
	    // Get global allocator and the allocation associated with this resource
	    VmaAllocator allocator = MemoryAllocator::GetAllocator();
	    VmaAllocation allocation = MemoryAllocator::GetAllocation(buffer);
	
	    if (allocation == VK_NULL_HANDLE)
	    {
	        SEDX_CORE_ERROR_TAG("Buffer", "No VMA allocation found for buffer {} - cannot free", ToString(buffer));
	        return;
	    }
	
	    // Destroy the buffer using the allocator retrieved from MemoryAllocator
	    VkBuffer buf = reinterpret_cast<VkBuffer>(buffer);
	    vmaDestroyBuffer(allocator, buf, allocation);
	
	    // Remove allocation tracking and clear the caller's reference
	    MemoryAllocator::FreeAllocation(buffer);
	    buffer = nullptr;
	
	    SEDX_CORE_TRACE_TAG("Buffer", "Freed buffer {}", ToString(buf));
	}

    void Buffer::Destroy()
	{
	    if (m_Buffer != VK_NULL_HANDLE)
		{
	        if (m_MappedData && m_Allocation != VK_NULL_HANDLE)
			{
	            vmaUnmapMemory(m_Allocator, m_Allocation);
	            m_MappedData = nullptr;
	        }

	        if (m_Allocation != VK_NULL_HANDLE)
			{
	            vmaDestroyBuffer(m_Allocator, m_Buffer, m_Allocation);
	        } 
	        else 
	        {
	            vmaDestroyBuffer(m_Allocator, m_Buffer, m_Allocation);
	        }

	        m_Buffer = VK_NULL_HANDLE;
	        m_Allocation = VK_NULL_HANDLE;
	        m_DeviceAddress = 0;
	    }
	}
	
}

// -----------------------------------

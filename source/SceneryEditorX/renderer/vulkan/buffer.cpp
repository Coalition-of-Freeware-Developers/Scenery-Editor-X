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
#include "buffer.h"
#include <volk/volk.h>

// --------------------------------

namespace SceneryEditorX
{
	Buffer::Buffer(VmaAllocator allocator, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, const VmaAllocationCreateInfo& allocInfo) : m_Device(device), m_Allocator(allocator)
	{
	    VkBufferCreateInfo bufferCI{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = size, .usage = usage };
	    vmaCreateBuffer(m_Allocator, &bufferCI, &allocInfo, &m_Buffer, &m_Allocation, nullptr);
	}
	
	Buffer::~Buffer()
	{
	    Destroy();
	}
	
	Buffer::Buffer(Buffer&& other) noexcept : m_Device(other.m_Device), m_Buffer(other.m_Buffer), m_Allocation(other.m_Allocation), m_Allocator(other.m_Allocator), m_MappedData(other.m_MappedData), m_DeviceAddress(other.m_DeviceAddress)
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
	        m_Device = other.m_Device;
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
	    if (m_DeviceAddress == 0 && m_Buffer != VK_NULL_HANDLE && m_Device != VK_NULL_HANDLE)
		{
	        VkBufferDeviceAddressInfo bdai{ 
	            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, 
	            .buffer = m_Buffer 
	        };
	        m_DeviceAddress = vkGetBufferDeviceAddress(m_Device, &bdai);
	    }
	    return m_DeviceAddress;
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

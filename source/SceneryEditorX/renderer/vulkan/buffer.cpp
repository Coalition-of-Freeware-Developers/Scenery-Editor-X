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

	Buffer::Buffer(const VmaAllocator allocator, const VkDeviceSize size, const VkBufferUsageFlags usage, const VmaAllocationCreateInfo& allocInfo)
	    : m_Allocator(allocator ? allocator : MemoryAllocator::GetAllocator())
	{
	    VkBufferCreateInfo bufferCI{};
	    bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	    bufferCI.size = size;
	    bufferCI.usage = usage;
	    bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	
	    const VkResult result = MemoryAllocator::CreateBuffer(bufferCI, allocInfo, m_Buffer, m_Allocation);
	    if (result != VK_SUCCESS)
	    {
	        m_Buffer = VK_NULL_HANDLE;
	        m_Allocation = VK_NULL_HANDLE;
	    }
	}

	Buffer::~Buffer()
	{
	    if (m_Buffer != VK_NULL_HANDLE || m_Allocation != VK_NULL_HANDLE)
	    {
	        SEDX_CORE_WARN_TAG("Buffer", "Destroying leaked buffer in destructor");
	        Destroy();
	    }
	}

	Buffer::Buffer(Buffer &&other) noexcept : m_Buffer(other.m_Buffer), m_Allocation(other.m_Allocation), 
    m_Allocator(other.m_Allocator), m_MappedData(other.m_MappedData), m_DeviceAddress(other.m_DeviceAddress)
	{
	    other.m_Buffer = VK_NULL_HANDLE;
	    other.m_Allocation = VK_NULL_HANDLE;
	    other.m_MappedData = nullptr;
	    other.m_DeviceAddress = 0;
	}
		
    Buffer &Buffer::operator=(Buffer &&other) noexcept
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
	    if (m_Allocation == VK_NULL_HANDLE || m_Allocator == VK_NULL_HANDLE)
	        return nullptr;
	
      if (!m_MappedData)
		{
			VmaAllocationInfo allocationInfo{};
			vmaGetAllocationInfo(m_Allocator, m_Allocation, &allocationInfo);

			if (allocationInfo.pMappedData)
			{
				m_MappedData = allocationInfo.pMappedData;
				return m_MappedData;
			}

			void* mappedData = nullptr;
			SEDX_VK_RESULT_ASSERT(vmaMapMemory(m_Allocator, m_Allocation, &mappedData), "Failed to map buffer memory");
			m_MappedData = mappedData;
		}
	
	    return m_MappedData;
	}
		
	void Buffer::Unmap()
	{
	    if (m_Allocation == VK_NULL_HANDLE || m_Allocator == VK_NULL_HANDLE)
	    {
	        SEDX_CORE_WARN_TAG("Buffer", "Unmap called on invalid allocation/allocator");
	        m_MappedData = nullptr;
	        return;
	    }
	
      VmaAllocationInfo allocationInfo{};
		vmaGetAllocationInfo(m_Allocator, m_Allocation, &allocationInfo);

		if (!m_MappedData && !allocationInfo.pMappedData)
		{
			// Already unmapped (or never mapped) — no-op
			return;
		}
	
	    vmaUnmapMemory(m_Allocator, m_Allocation);
	    m_MappedData = nullptr;
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
	
	void Buffer::FreeBuffer(VkBuffer buffer, VmaAllocation allocation)
	{
        SEDX_CORE_ASSERT(buffer != VK_NULL_HANDLE, "FreeBuffer called with null buffer");
        SEDX_CORE_ASSERT(allocation != VK_NULL_HANDLE, "FreeBuffer called with null allocation");
	
	    MemoryAllocator::DestroyBuffer(buffer, allocation);
	
	    SEDX_CORE_TRACE_TAG("Buffer", "Freed buffer {}", ToString(buffer));
	}
	
    void Buffer::FreeImageBuffer(VkImage image, VmaAllocation allocation)
	{
        SEDX_CORE_ASSERT(image != VK_NULL_HANDLE, "FreeImageBuffer called with null image");
        SEDX_CORE_ASSERT(allocation != VK_NULL_HANDLE, "FreeImageBuffer called with null allocation");
	
	    MemoryAllocator::DestroyImage(image, allocation);
	    MemoryAllocator::FreeAllocation(allocation);
	    SEDX_CORE_TRACE_TAG("Buffer", "Freed image {}", ToString(image));
	}
	
	void Buffer::Destroy()
	{
     if (m_Buffer == VK_NULL_HANDLE && m_Allocation == VK_NULL_HANDLE)
	        return;
	
     if (m_Allocation != VK_NULL_HANDLE && m_Allocator != VK_NULL_HANDLE)
	    {
          VmaAllocationInfo allocationInfo{};
			vmaGetAllocationInfo(m_Allocator, m_Allocation, &allocationInfo);

			if (m_MappedData || allocationInfo.pMappedData)
			{
				vmaUnmapMemory(m_Allocator, m_Allocation);
			}
	    }

		m_MappedData = nullptr;
	
     if (m_Buffer != VK_NULL_HANDLE || m_Allocation != VK_NULL_HANDLE)
	    {
	        MemoryAllocator::DestroyBuffer(m_Buffer, m_Allocation);
	    }
	
	    m_Buffer = VK_NULL_HANDLE;
	    m_Allocation = VK_NULL_HANDLE;
	    m_DeviceAddress = 0;
	}
	
}

// -----------------------------------

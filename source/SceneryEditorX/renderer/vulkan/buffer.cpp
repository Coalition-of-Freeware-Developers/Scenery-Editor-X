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
#include <cstring>
#include <volk/volk.h>

// --------------------------------

namespace SceneryEditorX
{

	Buffer::Buffer(const size_t stride, const uint32_t elementCount, const void *data, const bool mappable, const char *name) : 
		m_Allocator(MemoryAllocator::GetAllocator()), m_StrideUnaligned(static_cast<uint32_t>(stride)), m_Stride(static_cast<uint32_t>(stride)), 
		m_ElementCount(elementCount), m_IsMappable(mappable)
	{
		if (m_Buffer)
		{
			QueueManager::AddDeletionQueue(ResourceType::Buffer, m_Buffer);
			m_Buffer = VK_NULL_HANDLE;
		}
	
		SetObjectName(name ? name : "");
	
		const VkDeviceSize totalSize = static_cast<VkDeviceSize>(stride) * elementCount;
		if (totalSize == 0 || m_Allocator == VK_NULL_HANDLE)
			return;
	
		VkBufferCreateInfo bufferCI{};
		bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCI.size = totalSize;
		bufferCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	
		VmaAllocationCreateInfo allocCI{};
		allocCI.usage = VMA_MEMORY_USAGE_AUTO;
	
		if (mappable)
			allocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	
		VmaAllocationInfo allocationInfo{};
		const VkResult result = MemoryAllocator::CreateBuffer(bufferCI, allocCI, m_Buffer, m_Allocation, &allocationInfo);
		if (result != VK_SUCCESS)
		{
			SEDX_CORE_ERROR_TAG("Buffer", "Failed to create buffer '{}'", name ? name : "");
			m_Buffer = VK_NULL_HANDLE;
			m_Allocation = VK_NULL_HANDLE;
			return;
		}
	
		if (allocationInfo.pMappedData)
		{
			m_MappedData = allocationInfo.pMappedData;
			m_ExplicitlyMapped = false;
		}
	
		if (data && totalSize > 0)
		{
			if (mappable)
			{
				void *mapped = Map();
				if (mapped)
				{
					std::memcpy(mapped, data, static_cast<size_t>(totalSize));
					Unmap();
				}
			}
			else
			{
				UploadSubRegion(data, 0, static_cast<uint64_t>(totalSize));
			}
		}
	}

	Buffer::Buffer(const VmaAllocator allocator, const VkDeviceSize size, const VkBufferUsageFlags usage, const VmaAllocationCreateInfo &allocInfo)
		: m_Allocator(allocator ? allocator : MemoryAllocator::GetAllocator())
	{
		m_IsMappable = (allocInfo.flags & (VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
								VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT)) != 0;
	
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

	Buffer::Buffer(Buffer &&other) noexcept : m_Buffer(other.m_Buffer), m_Allocation(other.m_Allocation), m_Allocator(other.m_Allocator),
	  m_MappedData(other.m_MappedData), m_DeviceAddress(other.m_DeviceAddress), m_ExplicitlyMapped(other.m_ExplicitlyMapped)
	{
		other.m_Buffer = VK_NULL_HANDLE;
		other.m_Allocation = VK_NULL_HANDLE;
		other.m_MappedData = nullptr;
		other.m_ExplicitlyMapped = false;
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
			m_ExplicitlyMapped = other.m_ExplicitlyMapped;
			m_DeviceAddress = other.m_DeviceAddress;
	
			other.m_Buffer = VK_NULL_HANDLE;
			other.m_Allocation = VK_NULL_HANDLE;
			other.m_MappedData = nullptr;
			other.m_ExplicitlyMapped = false;
			other.m_DeviceAddress = 0;
		}
	
		return *this;
	}
		
	void *Buffer::Map()
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
				m_ExplicitlyMapped = false;
				return m_MappedData;
			}
	
			void *mappedData = nullptr;
			SEDX_VK_RESULT_ASSERT(vmaMapMemory(m_Allocator, m_Allocation, &mappedData), "Failed to map buffer memory");
			m_MappedData = mappedData;
			m_ExplicitlyMapped = true;
		}
	
		return m_MappedData;
	}

	void Buffer::Map(void *resource, void *&data)
	{
		data = nullptr;
		auto *buffer = static_cast<Buffer *>(resource);
		if (!buffer)
		{
			SEDX_CORE_WARN_TAG("Buffer", "Map(resource): resource is null");
			return;
		}

		if (!buffer->Valid() || buffer->Allocation() == VK_NULL_HANDLE)
		{
			SEDX_CORE_WARN_TAG("Buffer", "Attempt to map resource allocation is invalid");
			return;
		}

		if (MemoryAllocator::GetAllocator() == VK_NULL_HANDLE)
		{
			SEDX_CORE_WARN_TAG("Buffer", "VMA allocator is not initialized");
			return;
		}
	
		data = buffer->Map();
		if (!data)
			SEDX_CORE_WARN_TAG("Buffer", "Failed to map resource memory");
	}

	void Buffer::Unmap()
	{
		if (m_Allocation == VK_NULL_HANDLE || m_Allocator == VK_NULL_HANDLE)
		{
			SEDX_CORE_WARN_TAG("Buffer", "Unmap called on invalid allocation/allocator");
			m_MappedData = nullptr;
			m_ExplicitlyMapped = false;
			return;
		}
	
		VmaAllocationInfo allocationInfo{};
		vmaGetAllocationInfo(m_Allocator, m_Allocation, &allocationInfo);
	
		if (!m_MappedData && !allocationInfo.pMappedData)
		{
			// Already unmapped (or never mapped) — no-op
			return;
		}
	
		if (m_ExplicitlyMapped)
		{
			vmaUnmapMemory(m_Allocator, m_Allocation);
		}
		m_MappedData = nullptr;
		m_ExplicitlyMapped = false;
	}

	void Buffer::Unmap(void *resource)
	{
		auto *buffer = static_cast<Buffer *>(resource);
		if (!buffer)
			return;
	
		buffer->Unmap();
	}

	void *Buffer::AcquireStagingBuffer(uint64_t size)
	{
		if (size == 0)
			return nullptr;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	
		auto *stagingBuffer = new Buffer(MemoryAllocator::GetAllocator(), size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, allocInfo);
		if (!stagingBuffer->Valid())
		{
			delete stagingBuffer;
			return nullptr;
		}
	
		return stagingBuffer;
	}

	void Buffer::ReleaseStagingBuffer(void *buffer)
	{
		if (!buffer)
			return;
	
		delete static_cast<Buffer *>(buffer);
	}

	void Buffer::UploadSubRegion(const void *data, uint64_t offsetBytes, uint64_t sizeBytes)
	{
		SEDX_CORE_ASSERT(data != nullptr);
		SEDX_CORE_ASSERT(offsetBytes + sizeBytes <= GetObjectSize());
	
		if (m_IsMappable)
		{
			if (!m_MappedData)
			{
				Map();
			}
	
			SEDX_CORE_ASSERT(m_MappedData != nullptr, "Mapped buffer data is null");
			// for mapped buffers, direct memcpy at the offset
			std::memcpy(static_cast<uint8_t *>(m_MappedData) + offsetBytes, data, sizeBytes);
		}
		else
		{
			// for device-local buffers, stage and copy at the specified offset
			auto *stagingBuffer = static_cast<Buffer *>(Buffer::AcquireStagingBuffer(sizeBytes));
			SEDX_CORE_ASSERT(stagingBuffer != nullptr, "Failed to acquire staging buffer");
	
			void *mapped = stagingBuffer->Map();
			SEDX_CORE_ASSERT(mapped != nullptr, "Failed to map staging buffer");
			std::memcpy(mapped, data, sizeBytes);
			stagingBuffer->Unmap();
	
			VkBuffer bufferVk = m_Buffer;
			VkBuffer stagingVk = stagingBuffer->Get();
			VkBufferCopy copy_region = {};
			copy_region.srcOffset = 0;
			copy_region.dstOffset = offsetBytes;
			copy_region.size = sizeBytes;
	
			// use graphics queue to avoid a cross-queue sync hazard with draw commands that read the same buffer
			CommandList *cmd_list = CommandList::BeginImmediateExecution(QueueType::Graphics);
			vkCmdCopyBuffer(static_cast<VkCommandBuffer>(cmd_list->GetCommandBuffer()),
							stagingVk,
							bufferVk,
							1,
							&copy_region);
			CommandList::EndImmediateExecution(cmd_list);
	
			Buffer::ReleaseStagingBuffer(stagingBuffer);
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
		SEDX_CORE_TRACE_TAG("Buffer", "Freed image {}", ToString(image));
	}
	
	void Buffer::Destroy()
	{
		if (m_Buffer == VK_NULL_HANDLE && m_Allocation == VK_NULL_HANDLE)
			return;
	
		if (m_Allocation != VK_NULL_HANDLE && m_Allocator != VK_NULL_HANDLE && m_ExplicitlyMapped)
		{
			MemoryAllocator::UnmapMemory(m_Allocation);
		}
	
		m_MappedData = nullptr;
		m_ExplicitlyMapped = false;
	
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

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
 * buffer.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "memory_allocator.h"
#include <SceneryEditorX/utils/inheritance.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
    class Buffer : public SharedObject
	{
	public:
	    Buffer() = default;
	    Buffer(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage, const VmaAllocationCreateInfo& allocInfo);
        virtual ~Buffer() override;

        Buffer(const Buffer&) = delete;
	    Buffer& operator=(const Buffer&) = delete;
	    Buffer(Buffer&& other) noexcept;
	    Buffer& operator=(Buffer&& other) noexcept;

        [[nodiscard]] VkBuffer Get() const { return m_Buffer; }
        [[nodiscard]] VmaAllocation Allocation() const { return m_Allocation; }
	    void* Map();
	    void Unmap();
	    VkDeviceAddress DeviceAddress();
	
        [[nodiscard]] bool Valid() const { return m_Buffer != VK_NULL_HANDLE; }

		static void FreeBuffer(VkBuffer buffer, VmaAllocation allocation);
		static void FreeImageBuffer(VkImage image, VmaAllocation allocation);
	    void Destroy(); // Explicitly free underlying VMA resources before m_Allocator destruction
        uint32_t GetStride() const          { return m_stride; }

	private:
	    VkBuffer m_Buffer{ VK_NULL_HANDLE };
	    VmaAllocation m_Allocation{ VK_NULL_HANDLE };
        VmaAllocator m_Allocator{VK_NULL_HANDLE};
	    void* m_MappedData{ nullptr };
        bool m_ExplicitlyMapped{ false };
	    VkDeviceAddress m_DeviceAddress{ 0 };
        uint32_t m_stride_unaligned    = 0;
        uint32_t m_stride              = 0;
	};

}

// -------------------------------------------------------

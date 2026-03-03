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
 * memory_allocator.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "SceneryEditorX/core/resource/iobject.h"
#include "SceneryEditorX/renderer/gpu_stats.h"
#include <vma/vk_mem_alloc.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct AllocInfo;
	class Device;

	class MemoryAllocator : public IObject
	{
	public:
        MemoryAllocator() = default;
	    MemoryAllocator(const char* name);
        ~MemoryAllocator();

		static void Init(Ref<Device> device);
        static void Tick(uint64_t frameCount);
        static void SaveAllocation(VmaAllocation allocation, AllocInfo allocInfo);
        static void FreeAllocation(VmaAllocation allocation);
        static void Shutdown();

	    VmaAllocation AllocateBuffer(VkBufferCreateInfo bufferCI, VmaMemoryUsage usage, VkBuffer& buffOut);
		VmaAllocation AllocateImage(VkImageCreateInfo imgCI, VmaMemoryUsage usage, VkImage& imgOut, VkDeviceSize* allocSize = nullptr);
	    static VkResult CreateBuffer(const VkBufferCreateInfo& bufferCI, const VmaAllocationCreateInfo& allocInfo, VkBuffer& outBuffer,
                                 VmaAllocation& outAllocation, VmaAllocationInfo* outAllocationInfo = nullptr);

        static void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation);
        static void DestroyImage(VkImage image, VmaAllocation allocation);

        static VmaAllocation GetAllocation(VmaAllocation allocation);
        static VmaAllocator GetAllocator();
        static uint64_t GetAllocatedMemory();
        static uint64_t GetAvailableMemory();

        /**
	     * @brief Maps the memory of a given allocation to a pointer of type T.
	     * @tparam T The type to map the memory to.
	     * @param allocation The VMA allocation to map.
	     * @return A pointer to the mapped memory of type T.
	     */
	    template<typename T>
		T* MapMemory(VmaAllocation allocation)
		{
			T* mappedMemory;
			vmaMapMemory(GetAllocator(), allocation, (void**)&mappedMemory);
			return mappedMemory;
		}

        static void UnmapMemory(VmaAllocation allocation);

		static void DumpStats();
        static GPUMemoryStats GetMemoryStats();
    private:
        Ref<Device> m_Device;
	};
	
}

// -------------------------------------------------------

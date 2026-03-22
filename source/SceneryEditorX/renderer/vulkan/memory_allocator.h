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
#include <SceneryEditorX/core/resource/iobject.h>
#include <SceneryEditorX/renderer/gpu_stats.h>
#include <vma/vk_mem_alloc.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct AllocInfo;
	class Device;

	/**
	 * @class MemoryAllocator
	 * @brief Manages Vulkan memory allocations using the Vulkan Memory Allocator (VMA) library.
	 */
	class MemoryAllocator : public IObject
	{
	public:
		MemoryAllocator() = default;
		MemoryAllocator(const char* name);
		~MemoryAllocator();

		/**
		 * @brief Initializes the memory allocator with the given device.
		 * @param device The Vulkan device to use for memory allocation.
		 */
		static void Init(Ref<Device> device);

		/**
		 * @brief Updates the memory allocator for the given frame count.
		 * @param frameCount The current frame count.
		 */
		static void Tick(uint64_t frameCount);

		/**
		 * @brief Saves the allocation information for a given VMA allocation.
		 * @param allocation The VMA allocation handle.
		 * @param allocInfo The allocation information to save.
		 */
		static void SaveAllocation(VmaAllocation allocation, AllocInfo allocInfo);

		/**
		 * @brief Frees the memory allocation for a given VMA allocation.
		 * @param allocation The VMA allocation handle to free.
		 */
		static void FreeAllocation(VmaAllocation allocation);

		/**
		 * @brief Shuts down the memory allocator and releases all resources.
		 */
		static void Shutdown();

		/**
		 * @brief Allocates a buffer using the Vulkan Memory Allocator (VMA).
		 * @param bufferCI The Vulkan buffer creation info.
		 * @param usage The VMA memory usage type.
		 * @param buffOut The output Vulkan buffer handle.
		 * @return The VMA allocation handle.
		 */
		VmaAllocation AllocateBuffer(VkBufferCreateInfo bufferCI, VmaMemoryUsage usage, VkBuffer& buffOut);

		/**
		 * @brief Allocates an image using the Vulkan Memory Allocator (VMA).
		 * @param imgCI The Vulkan image creation info.
		 * @param usage The VMA memory usage type.
		 * @param imgOut The output Vulkan image handle.
		 * @param allocSize The size of the allocated memory (optional).
		 * @return The VMA allocation handle.
		 */
		VmaAllocation AllocateImage(VkImageCreateInfo imgCI, VmaMemoryUsage usage, VkImage& imgOut, VkDeviceSize* allocSize = nullptr);

		/**
		 * @brief Creates a buffer using the Vulkan Memory Allocator (VMA).
		 * @param bufferCI The Vulkan buffer creation info.
		 * @param allocInfo The VMA allocation creation info.
		 * @param outBuffer The output Vulkan buffer handle.
		 * @param outAllocation The output VMA allocation handle.
		 * @param outAllocationInfo The output VMA allocation info (optional).
		 * @return The result of the buffer creation.
		 */
		static VkResult CreateBuffer(const VkBufferCreateInfo& bufferCI, const VmaAllocationCreateInfo& allocInfo, VkBuffer& outBuffer,
								 VmaAllocation& outAllocation, VmaAllocationInfo* outAllocationInfo = nullptr);

		/**
		 * @brief Destroys a buffer and frees its associated memory.
		 * @param buffer The Vulkan buffer to destroy.
		 * @param allocation The VMA allocation associated with the buffer.
		 */
		static void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation);

		/**
		 * @brief Destroys an image and frees its associated memory.
		 * @param image The Vulkan image to destroy.
		 * @param allocation The VMA allocation associated with the image.
		 */
		static void DestroyImage(VkImage image, VmaAllocation allocation);

		/**
		 * @brief Retrieves the VMA allocation associated with a given allocation handle.
		 * @param allocation The VMA allocation handle.
		 * @return The VMA allocation.
		 */
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

		/**
		 * @brief Unmaps the memory of a given allocation.
		 * @param allocation The VMA allocation to unmap.
		 */
		static void UnmapMemory(VmaAllocation allocation);

		static void DumpStats();
		static GPUMemoryStats GetMemoryStats();
	private:
		Ref<Device> m_Device;
	};
	
}

// -------------------------------------------------------

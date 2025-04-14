/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_allocator.h
* -------------------------------------------------------
* Created: 8/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/renderer/vk_device.h>
#include <string>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct SoftwareStats;

    class MemoryAllocator
    {
    public:
        MemoryAllocator() = default;
        explicit MemoryAllocator(const std::string &tag);
        ~MemoryAllocator();

		// ---------------------------------------------------------

        static VmaAllocation AllocateBuffer(const VkBufferCreateInfo &bufferCreateInfo, VmaMemoryUsage usage, VkBuffer &outBuffer);
        static VmaAllocation AllocateImage(const VkImageCreateInfo &imageCreateInfo, VmaMemoryUsage usage, VkImage &outImage, VkDeviceSize* allocatedSize = nullptr);

        static void Free(VmaAllocation allocation);
        static void DestroyImage(VkImage image, VmaAllocation allocation);
        static void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation);

		// ---------------------------------------------------------

		template<typename T>
		T* MapMemory(VmaAllocation allocation)
		{
			T* mappedMemory;
            vmaMapMemory(GetMemAllocator(), allocation, static_cast<void **>(&mappedMemory));
			return mappedMemory;
		}

		// ---------------------------------------------------------

        static void UnmapMemory(VmaAllocation allocation);
        static VmaAllocator& GetMemAllocator();

		static void Init(Ref<VulkanDevice> &device);
		static void Shutdown();

		// ---------------------------------------------------------

    private:
        std::string Tag_;
    };

} // namespace SceneryEditorX

// -------------------------------------------------------

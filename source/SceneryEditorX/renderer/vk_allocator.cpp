/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_allocator.cpp
* -------------------------------------------------------
* Created: 8/4/2025
* -------------------------------------------------------
*/

#include <map>
#include <SceneryEditorX/core/memory.h>
#include <SceneryEditorX/renderer/vk_allocator.h>
#include <SceneryEditorX/renderer/vk_core.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct VulkanAllocatorData
	{
		VmaAllocator Allocator;
		uint64_t TotalAllocatedBytes = 0;
		uint64_t MemoryUsage = 0; // all heaps
	};

	// ---------------------------------------------------------

	enum class AllocationType : uint8_t
    {
        None = 0,
        Buffer = 1,
        Image = 2
    };

    // ---------------------------------------------------------

	static VulkanAllocatorData *memAllocatorData = nullptr;

	struct AllocInfo
	{
		uint64_t AllocatedSize = 0;
		AllocationType Type = AllocationType::None;
	};

    static std::map<VmaAllocation, AllocInfo> s_AllocationMap;

	// ---------------------------------------------------------

    MemoryAllocator::MemoryAllocator(const std::string &tag) : Tag_(tag) {}

    MemoryAllocator::~MemoryAllocator() {}

	// ---------------------------------------------------------

	VmaAllocation MemoryAllocator::AllocateBuffer(const VkBufferCreateInfo &bufferCreateInfo, const VmaMemoryUsage usage, VkBuffer &outBuffer)
	{
        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = usage;

		VmaAllocation allocation;
        vmaCreateBuffer(memAllocatorData->Allocator, &bufferCreateInfo, &allocCreateInfo, &outBuffer, &allocation, nullptr);
        if (allocation == nullptr)
		{
            ErrMsg("Failed to allocate buffer memory");
		}

		VmaAllocationInfo allocInfo{};
		vmaGetAllocationInfo(memAllocatorData->Allocator, allocation, &allocInfo);

		memAllocatorData->TotalAllocatedBytes += allocInfo.size;

        return allocation;

	}

	VmaAllocation MemoryAllocator::AllocateImage(const VkImageCreateInfo &imageCreateInfo, const VmaMemoryUsage usage, VkImage& outImage, VkDeviceSize* allocatedSize)
	{
        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = usage;

		VmaAllocation allocation;
		vmaCreateImage(memAllocatorData->Allocator, &imageCreateInfo, &allocCreateInfo, &outImage, &allocation, nullptr);
		if (allocation == nullptr)
        {
            ErrMsg("Failed to allocate GPU image");
        }

		VmaAllocationInfo allocInfo;
		vmaGetAllocationInfo(memAllocatorData->Allocator, allocation, &allocInfo);
        if (allocatedSize)
        {
			*allocatedSize = allocInfo.size;
        }

        memAllocatorData->TotalAllocatedBytes += allocInfo.size;

        return allocation;
	}

	void MemoryAllocator::Free(const VmaAllocation allocation)
    {
        vmaFreeMemory(memAllocatorData->Allocator, allocation);

    }

	void MemoryAllocator::DestroyImage(const VkImage image, const VmaAllocation allocation)
    {
        vmaDestroyImage(memAllocatorData->Allocator, image, allocation);

	}

	void MemoryAllocator::DestroyBuffer(const VkBuffer buffer, const VmaAllocation allocation)
    {
        vmaDestroyBuffer(memAllocatorData->Allocator, buffer, allocation);
    }

	void MemoryAllocator::UnmapMemory(const VmaAllocation allocation)
	{
		vmaUnmapMemory(memAllocatorData->Allocator, allocation);
	}

	// ---------------------------------------------------------

	void MemoryAllocator::Init(Ref<VulkanDevice> &device)
    {
        const SoftwareStats version;
        memAllocatorData = hnew VulkanAllocatorData();

		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.vulkanApiVersion = version.minVulkanVersion;
        allocatorInfo.physicalDevice = device->GetPhysicalDevice()->GetGPUDevice();
        allocatorInfo.device = device->GetDevice();
        allocatorInfo.instance = GraphicsEngine::GetInstance();

		vmaCreateAllocator(&allocatorInfo, &memAllocatorData->Allocator);

	}

	void MemoryAllocator::Shutdown()
    {
        vmaDestroyAllocator(memAllocatorData->Allocator);
        hdelete memAllocatorData;
        memAllocatorData = nullptr;
    }

	VmaAllocator& MemoryAllocator::GetMemAllocator()
	{
		return memAllocatorData->Allocator;
	}



} // namespace SceneryEditorX

// -------------------------------------------------------

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
 * memory_allocator.cpp
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#include "memory_allocator.h"
#include "render_context.h"
#include <mutex>

/**
 * VMA implementation — must be defined in exactly ONE translation unit.
 * VK_NO_PROTOTYPES is active (volk is used), so we disable VMA's static
 * Vulkan function resolution and enable dynamic resolution via the
 * VmaVulkanFunctions struct populated in MemoryAllocator::Init().
 */
#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS  0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#ifndef VK_USE_64_BIT_NON_DISPATCHABLE_HANDLES
#define VK_USE_64_BIT_NON_DISPATCHABLE_HANDLES 1
#endif
#include <SceneryEditorX/utils/string_utils.h>
#include <vma/vk_mem_alloc.h>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @struct SEDX_AllocatorData
	 * @brief Holds data related to the Vulkan memory allocator.
	 */
	struct SEDX_AllocatorData
	{
		VmaAllocator allocator;
		uint64_t totalAllocatedBytes = 0;
		uint64_t memoryUsage = 0; // all heaps
	};

	/**
	 * @enum AllocationType
	 * @brief Enumeration of allocation types for tracking purposes.
	 */
	enum class AllocationType : uint8_t
	{
		None	= 0, 
		Buffer	= 1, 
		Image	= 2
	};

	/**
	 * @struct AllocInfo
	 * @brief Struct to store information about a memory allocation, including its size and type.
	 */
	struct AllocInfo
	{
		uint64_t allocatedSize = 0;
		AllocationType type = AllocationType::None;
	};

	static SEDX_AllocatorData *s_AllocatorData; // Pointer to the allocator data, initialized in MemoryAllocator::Init()
	static std::map<VmaAllocation, AllocInfo> s_AllocationMap; // Map to track active allocations and their information
	static std::mutex s_MutexAllocator; // Mutex to protect access to the allocation map for thread safety

	// -------------------------------------------------------

	MemoryAllocator::MemoryAllocator(const char* name)
	{
		m_ObjectName = name;
		SEDX_CORE_TRACE_TAG("MemoryAllocator","Allocation {0}: created", m_ObjectName);
	}

	MemoryAllocator::~MemoryAllocator()
	{
		SEDX_CORE_ASSERT(s_AllocatorData != nullptr, "Allocator data is null");
		SEDX_CORE_ASSERT(s_AllocationMap.empty(), "There are still allocations");
		vmaDestroyAllocator(s_AllocatorData->allocator);
		s_AllocatorData = nullptr;
		m_Device.Reset();
		SEDX_CORE_TRACE_TAG("MemoryAllocator","Allocation {0}: destroyed", m_ObjectName);
	}

	void MemoryAllocator::Init(Ref<Device> device)
	{
		SEDX_CORE_ASSERT(device != nullptr, "Device cannot be null");
		SEDX_CORE_ASSERT(device->GetLogicalDevice() != VK_NULL_HANDLE, "Logical device cannot be VK_NULL_HANDLE when initializing VMA allocator");
		SEDX_CORE_ASSERT(device->GetPhysicalDevice() != VK_NULL_HANDLE, "Physical device cannot be VK_NULL_HANDLE when initializing VMA allocator");
		s_AllocatorData = new SEDX_AllocatorData();

		// Zero-initialize and provide the two root function pointers.
		// With VMA_DYNAMIC_VULKAN_FUNCTIONS=1, VMA will use these to
		// resolve all other Vulkan function pointers at runtime.
		VmaVulkanFunctions vkFunctions = {};
		vkFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
		vkFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

		// Initialize VulkanMemoryAllocator
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
		allocatorInfo.physicalDevice = device->GetPhysicalDevice();
		allocatorInfo.device = device->GetLogicalDevice();
		allocatorInfo.instance = RenderContext::GetInstance();
		allocatorInfo.pVulkanFunctions = &vkFunctions;
		
		SEDX_VK_RESULT_ASSERT(vmaCreateAllocator(&allocatorInfo, &s_AllocatorData->allocator));
		SEDX_CORE_TRACE_TAG("MemoryAllocator", "VMA allocator created");
	}

	/**
	 * @brief Per-frame update for the device, used to manage memory allocation frames.
	 * @param frameCount Current frame count
	 */
	void MemoryAllocator::Tick(const uint64_t frameCount)
	{
		const Ref<Device> device = RenderContext::Get()->GetDevice();
		/**
		 * https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/staying_within_budget.html
		 * make sure to call vmaSetCurrentFrameIndex() every frame
		 * budget is queried from Vulkan inside of it to avoid overhead of querying it with every allocation
		 */
		if (!device.IsValid())
		{
			SEDX_CORE_WARN_TAG("MemoryAllocator", "Device singleton is not initialized before Tick().");
			return;
		}

	#ifdef SEDX_DEBUG

		// Add null check for s_AllocatorData
		if (s_AllocatorData == nullptr || s_AllocatorData->allocator == nullptr)
		{
			SEDX_CORE_WARN_TAG("MemoryAllocator", "VMA allocator not initialized");
			return;
		}
	#endif

		vmaSetCurrentFrameIndex(GetAllocator(), static_cast<uint32_t>(frameCount));
	}

	void MemoryAllocator::SaveAllocation(VmaAllocation allocation, AllocInfo allocInfo)
	{
		SEDX_CORE_ASSERT(allocation != nullptr, "Allocation is null");
		std::scoped_lock lock(s_MutexAllocator);
		s_AllocationMap.emplace(allocation, allocInfo);
		SEDX_CORE_TRACE_TAG("MemoryAllocator", "Allocation saved; total allocations: {0}", s_AllocationMap.size());
	}

	void MemoryAllocator::FreeAllocation(VmaAllocation allocation)
	{
		std::scoped_lock lock(s_MutexAllocator);
		s_AllocationMap.erase(allocation);
		SEDX_CORE_TRACE_TAG("MemoryAllocator", "Allocation freed; remaining allocations: {0}", s_AllocationMap.size());
	}

	void MemoryAllocator::Shutdown()
	{
		vmaDestroyAllocator(s_AllocatorData->allocator);

		delete s_AllocatorData;
		s_AllocatorData = nullptr;
		SEDX_CORE_TRACE_TAG("MemoryAllocator", "VMA allocator destroyed");
	}

	VmaAllocation MemoryAllocator::AllocateBuffer(VkBufferCreateInfo bufferCI, VmaMemoryUsage usage, VkBuffer &buffOut)
	{
		SEDX_CORE_VERIFY(bufferCI.size > 0);

		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = usage;

		VmaAllocation allocation;
		vmaCreateBuffer(s_AllocatorData->allocator, &bufferCI, &allocCreateInfo, &buffOut, &allocation, nullptr);
		if (allocation == nullptr)
		{
			SEDX_CORE_ERROR_TAG("MemoryAllocator", "Failed to allocate GPU buffer!");
			SEDX_CORE_ERROR("Requested size: {}", Utils::BytesToString(bufferCI.size));
			auto stats = GetMemoryStats();
			SEDX_CORE_ERROR("GPU mem usage: {}/{}", Utils::BytesToString(stats.used), Utils::BytesToString(stats.totalAvailable));
		}

		// TODO: Tracking
		VmaAllocationInfo allocInfo{};
		vmaGetAllocationInfo(s_AllocatorData->allocator, allocation, &allocInfo);
		SEDX_CORE_TRACE_TAG("MemoryAllocator", "Allocating buffer; size = {0}", m_ObjectName, Utils::BytesToString(allocInfo.size));

		{
			s_AllocatorData->totalAllocatedBytes += allocInfo.size;
			SEDX_CORE_TRACE_TAG("MemoryAllocator", "{0}: total allocated since start is {1}", m_ObjectName, Utils::BytesToString(s_AllocatorData->totalAllocatedBytes));
		}

	#if SEDX_GPU_TRACK_MEMORY_ALLOCATION
		auto& allocTrack = s_AllocationMap[allocation];
		allocTrack.allocatedSize = allocInfo.size;
		allocTrack.type = AllocationType::Buffer;
		s_AllocatorData->memoryUsage += allocInfo.size;
	#endif

		return allocation;
	}

	VmaAllocation MemoryAllocator::AllocateImage(VkImageCreateInfo imgCI, VmaMemoryUsage usage, VkImage &imgOut, VkDeviceSize *allocSize)
	{
		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = usage;

		VmaAllocation allocation;
		vmaCreateImage(s_AllocatorData->allocator, &imgCI, &allocCreateInfo, &imgOut, &allocation, nullptr);
		if (allocation == nullptr)
		{
			SEDX_CORE_ERROR_TAG("MemoryAllocator", "Failed to allocate GPU image!");
			SEDX_CORE_ERROR("  Requested size: {}x{}x{}", imgCI.extent.width, imgCI.extent.height, imgCI.extent.depth);
			SEDX_CORE_ERROR("  Mips: {}", imgCI.mipLevels);
			SEDX_CORE_ERROR("  Layers: {}", imgCI.arrayLayers);
			auto stats = GetMemoryStats();
			SEDX_CORE_ERROR("  GPU mem usage: {}/{}", Utils::BytesToString(stats.used), Utils::BytesToString(stats.totalAvailable));
		}

		// TODO: Improve memory allocation tracking
		VmaAllocationInfo allocInfo;
		vmaGetAllocationInfo(s_AllocatorData->allocator, allocation, &allocInfo);
		if (allocSize)
		{
			*allocSize = allocInfo.size;
		}
		SEDX_CORE_TRACE_TAG("MemoryAllocator", "Allocating image; size = {0}", m_ObjectName, Utils::BytesToString(allocInfo.size));

		{
			s_AllocatorData->totalAllocatedBytes += allocInfo.size;
			SEDX_CORE_TRACE_TAG("MemoryAllocator", "{0}: total allocated since start is {1}", m_ObjectName, Utils::BytesToString(s_AllocatorData->totalAllocatedBytes));
		}

	//#if SEDX_GPU_TRACK_MEMORY_ALLOCATION
		auto& allocTrack = s_AllocationMap[allocation];
		allocTrack.allocatedSize = allocInfo.size;
		allocTrack.type = AllocationType::Image;
		s_AllocatorData->memoryUsage += allocInfo.size;
	//#endif

		SEDX_CORE_TRACE_TAG("MemoryAllocator", "vmaCreateImage returning handle of image allocation {}", (void*)allocation);

		return allocation;
	}

	VkResult MemoryAllocator::CreateBuffer(const VkBufferCreateInfo& bufferCI, const VmaAllocationCreateInfo& allocInfo,
										   VkBuffer& outBuffer, VmaAllocation& outAllocation, VmaAllocationInfo* outAllocationInfo)
	{
		SEDX_CORE_ASSERT(s_AllocatorData && s_AllocatorData->allocator, "VMA allocator is not initialized");
		SEDX_CORE_ASSERT(bufferCI.size > 0, "Buffer size must be > 0");
	
		outBuffer = VK_NULL_HANDLE;
		outAllocation = VK_NULL_HANDLE;
	
		const VkResult result = vmaCreateBuffer(s_AllocatorData->allocator, &bufferCI, &allocInfo, &outBuffer, &outAllocation, outAllocationInfo);
		if (result != VK_SUCCESS)
		{
			SEDX_CORE_ERROR_TAG("MemoryAllocator", "vmaCreateBuffer failed (size={}, usage={}, result={})",
								bufferCI.size, static_cast<uint32_t>(bufferCI.usage), static_cast<int32_t>(result));
			return result;
		}
		
		SEDX_CORE_TRACE_TAG("MemoryAllocator", "Allocating buffer size = {0}", Utils::BytesToString(bufferCI.size));
		return VK_SUCCESS;
	}

	void MemoryAllocator::DestroyBuffer(VkBuffer buffer, VmaAllocation allocation)
	{
		SEDX_CORE_ASSERT(buffer, "Buffer is null");
		SEDX_CORE_ASSERT(allocation, "Allocation is null");
		vmaDestroyBuffer(s_AllocatorData->allocator, buffer, allocation);

	//#if SEDX_GPU_TRACK_MEMORY_ALLOCATION
		auto it = s_AllocationMap.find(allocation);
		if (it != s_AllocationMap.end())
		{
			s_AllocatorData->memoryUsage -= it->second.allocatedSize;
			s_AllocationMap.erase(it);
		}
		else
		{
			SEDX_CORE_ERROR_TAG("MemoryAllocator", "Could not find GPU memory allocation: {}", (void*)allocation);
		}
	//#endif
		SEDX_CORE_TRACE_TAG("MemoryAllocator", "Buffer destroyed");
	}

	void MemoryAllocator::DestroyImage(VkImage image, VmaAllocation allocation)
	{
		SEDX_CORE_ASSERT(image);
		SEDX_CORE_ASSERT(allocation);
		vmaDestroyImage(s_AllocatorData->allocator, image, allocation);

//#if SEDX_GPU_TRACK_MEMORY_ALLOCATION
		auto it = s_AllocationMap.find(allocation);
		if (it != s_AllocationMap.end())
		{
			s_AllocatorData->memoryUsage -= it->second.allocatedSize;
			s_AllocationMap.erase(it);
		}
		else
		{
			SEDX_CORE_ERROR_TAG("MemoryAllocator", "Could not find GPU memory allocation: {}", (void*)allocation);
		}
//#endif
		SEDX_CORE_TRACE_TAG("MemoryAllocator", "Image destroyed");
	}

	VmaAllocation MemoryAllocator::GetAllocation(const VmaAllocation allocation)
	{
		std::scoped_lock lock(s_MutexAllocator);
		auto it = s_AllocationMap.find(allocation);
		SEDX_CORE_ASSERT(it != s_AllocationMap.end(), "Allocation not found in map");
		SEDX_CORE_TRACE_TAG("MemoryAllocator", "GetAllocation called for allocation {0}; found: {1}", (void*)allocation, it != s_AllocationMap.end());
		return it != s_AllocationMap.end() ? it->first : nullptr;
	}

	VmaAllocator MemoryAllocator::GetAllocator()
	{
		return s_AllocatorData->allocator;
	}

	uint64_t MemoryAllocator::GetAllocatedMemory()
	{
		uint64_t bytes = 0;
	
		const Ref<Device> device = RenderContext::Get()->GetDevice();
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(static_cast<VkPhysicalDevice>(device->GetPhysicalDevice()), &memoryProperties);
	
		VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
		vmaGetHeapBudgets(s_AllocatorData->allocator, budgets);
	
		for (uint32_t i = 0; i < VK_MAX_MEMORY_HEAPS; i++)
		{
			if (memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
			{
				if (budgets[i].budget < (1ull << 60))
					bytes += budgets[i].usage;
			}
		}
	
		SEDX_CORE_TRACE_TAG("MemoryAllocator", "GetAllocatedMemory: {0} bytes", bytes);
		return bytes / (1024ull * 1024ull);
	}

	uint64_t MemoryAllocator::GetAvailableMemory()
	{
		uint64_t bytes = 0;
		Ref<Device> device = RenderContext::Get()->GetDevice();

		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(static_cast<VkPhysicalDevice>(device->GetPhysicalDevice()), &memoryProperties);
	
		VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
		vmaGetHeapBudgets(s_AllocatorData->allocator, budgets);
	
		for (uint32_t i = 0; i < VK_MAX_MEMORY_HEAPS; i++)
		{
			if (memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
			{
				if (budgets[i].budget < (1ull << 60))
					bytes += budgets[i].budget;
			}
		}
	
		SEDX_CORE_TRACE_TAG("MemoryAllocator", "GetAvailableMemory: {0} bytes", bytes);
		return bytes / (1024ull * 1024ull);
	}

	void MemoryAllocator::UnmapMemory(VmaAllocation allocation)
	{
		vmaUnmapMemory(s_AllocatorData->allocator, allocation);
		SEDX_CORE_TRACE_TAG("MemoryAllocator", "Memory unmapped for allocation {0}", (void *)allocation);
	}

	void MemoryAllocator::DumpStats()
	{
		const auto& memoryProps = RenderContext::Get()->GetDevice()->GetDeviceMemoryProperties();
		std::vector<VmaBudget> budgets(memoryProps.memoryProperties.memoryHeapCount);
		vmaGetHeapBudgets(s_AllocatorData->allocator, budgets.data());

		SEDX_CORE_WARN("=== VMA Heap Budgets ===");
		SEDX_CORE_WARN("===================================");
		for (VmaBudget& b : budgets)
		{
			SEDX_CORE_WARN("VmaBudget.allocationBytes = {0}", Utils::BytesToString(b.statistics.allocationBytes));
			SEDX_CORE_WARN("VmaBudget.blockBytes = {0}", Utils::BytesToString(b.statistics.blockBytes));
			SEDX_CORE_WARN("VmaBudget.usage = {0}", Utils::BytesToString(b.usage));
			SEDX_CORE_WARN("VmaBudget.budget = {0}", Utils::BytesToString(b.budget));
		}
		SEDX_CORE_WARN("===================================");
	}

	GPUMemoryStats MemoryAllocator::GetMemoryStats()
	{
		const auto& memoryProps = RenderContext::Get()->GetDevice()->GetDeviceMemoryProperties();
		std::vector<VmaBudget> budgets(memoryProps.memoryProperties.memoryHeapCount);
		vmaGetHeapBudgets(s_AllocatorData->allocator, budgets.data());
		SEDX_CORE_TRACE_TAG("MemoryAllocator", "GetMemoryStats: total budget across all heaps is {0} bytes", Utils::BytesToString(budgets[0].budget));

		uint64_t budget = 0;
		for (VmaBudget& b : budgets)
		{
			budget += b.budget;
		}

		GPUMemoryStats result;
		for (const auto& [k, v] : s_AllocationMap)
		{
			if (v.type == AllocationType::Buffer)
			{
				result.bufferAllocationCount++;
				result.bufferAllocationSize += v.allocatedSize;
			}
			else if (v.type == AllocationType::Image)
			{
				result.imageAllocationCount++;
				result.imageAllocationSize += v.allocatedSize;
			}
		}

		result.allocationCount = s_AllocationMap.size();
		result.used = s_AllocatorData->memoryUsage;
		result.totalAvailable = budget;
		SEDX_CORE_TRACE_TAG("MemoryAllocator", "GetMemoryStats: total used memory is {0} bytes", Utils::BytesToString(result.used));
		SEDX_CORE_TRACE_TAG("MemoryAllocator", "GetMemoryStats: total available memory is {0} bytes", Utils::BytesToString(result.totalAvailable));
		return result;
#if 0
		VmaTotalStatistics stats;
		vmaCalculateStatistics(s_AllocatorData->allocator, &stats);

		uint64_t usedMemory = stats.total.usedBytes;
		uint64_t freeMemory = stats.total.unusedBytes;

		return { usedMemory, freeMemory };
#endif
	}

} // namespace SceneryEditorX

// -------------------------------------------------------

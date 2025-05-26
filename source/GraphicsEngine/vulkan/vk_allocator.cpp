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
#define VMA_IMPLEMENTATION
#include <SceneryEditorX/core/application_data.h>
#include <SceneryEditorX/core/memory.h>
#include <GraphicsEngine/vulkan/vk_allocator.h>
#include <GraphicsEngine/vulkan/vk_core.h>
#include <vma/vk_mem_alloc.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

    VkDeviceSize MemoryAllocator::customBufferSize = DEFAULT_CUSTOM_BUFFER_SIZE;

    /// ---------------------------------------------------------

	/**
	 * @struct VulkanAllocatorData
	 * @brief Stores data about Vulkan memory allocations and usage statistics.
	 * 
	 * This structure maintains the main VMA allocator object and tracks memory usage metrics
	 * including allocated bytes, freed bytes, allocation counts, and peak memory usage.
	 * It serves as the primary interface to the Vulkan Memory Allocator (VMA) library.
	 */
	struct VulkanAllocatorData
	{
	    /** @brief VMA allocator instance used for all memory operations */
	    VmaAllocator Allocator = nullptr;
	
	    /** @brief Total bytes allocated across all memory heaps */
	    uint64_t BytesAllocated = 0;
	
	    /** @brief Total bytes freed since allocator creation */
	    uint64_t BytesFreed = 0;
	
	    /** @brief Number of currently active allocations */
	    uint64_t CurrentAllocations = 0;
	
	    /** @brief Highest recorded memory usage in bytes */
	    uint64_t PeakMemoryUsage = 0;
	};

    /**
     * @brief Per memory type statistics tracking array.
     * 
     * This array stores memory allocation statistics for each Vulkan memory type.
     * Each element corresponds to a specific memory type index (up to VK_MAX_MEMORY_TYPES)
     * and tracks metrics like bytes allocated, bytes freed, and allocation counts
     * for that specific memory type.
     * 
     * This data is used for detailed reporting, memory usage optimization, 
     * and identifying which memory types are under the most pressure.
     */
    GLOBAL std::array<VulkanAllocatorData, VK_MAX_MEMORY_TYPES> memoryTypeStats;

	/**
	 * @brief Static global instance of the Vulkan memory allocator data.
	 * 
	 * This pointer holds the singleton instance of VulkanAllocatorData that contains
	 * the VMA allocator and related memory statistics. It's initialized in the 
	 * MemoryAllocator::Init() method and destroyed in MemoryAllocator::Shutdown().
	 * 
	 * The singleton pattern allows multiple components to access the same allocator
	 * instance throughout the application without having to pass it explicitly.
	 * This is particularly important for utility functions like those in the
	 * VulkanMemoryUtils namespace.
	 */
	GLOBAL VulkanAllocatorData *memAllocatorData = nullptr;


	/**
	 * @enum AllocationType
	 * @brief Categorizes the type of Vulkan memory allocation.
	 * 
	 * This enumeration distinguishes between different types of GPU allocations,
	 * which affects how they're tracked and managed by the memory allocator.
	 * Different allocation types may have different memory requirements,
	 * defragmentation characteristics, and usage patterns.
	 */
    enum class AllocationType : uint8_t
    {
        None = 0,   /** @brief No allocation type specified */
        Buffer = 1, /** @brief Buffer allocation (uniform buffers, vertex buffers, etc.) */
        Image = 2   /** @brief Image allocation (textures, render targets, etc.) */
    };
	
	/**
	 * @struct AllocInfo
	 * @brief Tracks information about a Vulkan memory allocation.
	 * 
	 * This structure is used internally by the memory allocator to keep track of
	 * metadata about each allocation, including its size and type (buffer or image).
	 * This information is necessary for memory statistics, defragmentation decisions,
	 * and proper cleanup when allocations are freed.
	 */
	struct AllocInfo
	{
        uint64_t AllocatedSize = 0;                 /** @brief Size of the allocation in bytes */
	    AllocationType Type = AllocationType::None; /** @brief Type of the allocation (buffer, image, etc.) */
	};

	/**
	 * @brief Global map tracking information about all active allocations.
	 * 
	 * This map maintains a record of all current allocations managed by the VulkanMemoryAllocator.
	 * Each entry maps a VmaAllocation handle to its corresponding AllocInfo structure,
	 * which contains metadata about the allocation such as its size and type.
	 * 
	 * The map is used for:
	 * - Tracking memory usage statistics
	 * - Properly cleaning up resources during deallocation
	 * - Identifying candidate allocations for defragmentation
	 * - Supporting debug and profiling operations
	 * 
	 * This is a global static instance shared across all memory allocation operations.
	 */
	GLOBAL std::map<VmaAllocation, AllocInfo> AllocationMap;

	/// ---------------------------------------------------------

    /**
     * @brief Constructs a memory allocator with the given tag.
     * 
     * Creates a new memory allocator instance with a specified tag name
     * for identification and logging purposes. The tag helps track
     * allocations from different systems within the application.
     * 
     * @param tag A string identifier for this allocator instance
     */
    MemoryAllocator::MemoryAllocator(std::string tag) : Tag_(std::move(tag)), currentStrategy() { }

    /**
     * @brief Destroys the memory allocator instance.
     * 
     * Virtual destructor that ensures proper cleanup of derived allocator types.
     * The actual Vulkan memory cleanup happens in the Shutdown() method, which
     * must be called explicitly before destruction.
     */
    MemoryAllocator::~MemoryAllocator() = default;

    /// ---------------------------------------------------------

	/*
	VkDeviceSize MemoryAllocator::GetCustomBufferSize()
	{
	    auto& settings = GetApplicationSettings();
	    if (settings.HasOption("vulkan.custom_buffer_size"))
	    {
	        return static_cast<VkDeviceSize>(settings.GetIntOption("vulkan.custom_buffer_size", 
	            static_cast<int>(DEFAULT_CUSTOM_BUFFER_SIZE)));
	    }
	    return customBufferSize;
	}
	
	bool MemoryAllocator::SetCustomBufferSize(VkDeviceSize size, const VulkanDevice& device)
	{
	    // Validate against device limits
	    const auto& deviceLimits = device.vkPhysDevice->GetProperties().limits;
	    
	    if (!ApplicationSettings::ValidateBufferSize(size, deviceLimits))
	    {
	        return false;
	    }
	    
	    // Store locally and in settings
	    customBufferSize = size;
	    auto& settings = GetApplicationSettings();
	    settings.SetCustomBufferSize(size);
	    return true;
	}
	*/
	
    /**
     * @brief Initializes the Vulkan memory allocator for efficient GPU memory management.
     * 
     * This function creates and configures the Vulkan Memory Allocator (VMA) instance
     * that will handle all memory allocations for buffers, images, and other GPU resources.
     * VMA provides efficient memory management, minimizes fragmentation, and optimizes 
     * allocation strategies based on usage patterns.
     * 
     * The function:
     * 1. Sets up the core allocator configuration with the physical device, logical device, and instance
     * 2. Configures optional features like buffer device address support when available
     * 3. Provides function pointers for dynamic Vulkan function loading
     * 4. Creates the memory allocator instance
     * 
     * Once initialized, all Vulkan memory allocations should be handled through this allocator
     * rather than directly through vkAllocateMemory for optimal performance and resource management.
     */
    void VulkanDevice::InitializeMemoryAllocator()
    {
        SEDX_CORE_TRACE_TAG("Vulkan Device", "Initializing Vulkan Memory Allocator");

        /// Create VMA (Vulkan Memory Allocator) instance
        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.physicalDevice = vkPhysDevice->GetGPUDevices();
        allocatorCreateInfo.device = device;
        allocatorCreateInfo.instance = GraphicsEngine::GetInstance();

        /// Set up flags
        allocatorCreateInfo.flags = 0;

        /// Enable buffer device address if available
        if (vkGetBufferDeviceAddressKHR != nullptr)
            allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

        /// Use a descriptor pool for memory allocation if needed
        /* allocatorCreateInfo.pAllocationCallbacks = allocator; */

        /// Create VMA allocator
        VmaVulkanFunctions vulkanFunctions = {};
        vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;
        allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

        /// Create the memory allocator
        memoryAllocator = CreateRef<MemoryAllocator>();

        SEDX_CORE_TRACE_TAG("Vulkan Device", "Vulkan Memory Allocator initialized successfully");
    }


	/**
	 * @brief Begins a defragmentation process for GPU memory.
	 * 
	 * This function initiates the memory defragmentation process.
	 * It creates a defragmentation context that can be used to optimize memory layout
	 * and reduce fragmentation in GPU memory.
	 * 
	 * @param flags Defragmentation flags to control the algorithm used
	 */
    void MemoryAllocator::BeginDefragmentation(VmaDefragmentationFlags flags)
    {
        std::lock_guard<std::mutex> lock(this->allocationMutex);

        if (!memAllocatorData || !memAllocatorData->Allocator)
        {
            SEDX_CORE_ERROR("Memory allocator not initialized when starting defragmentation");
            return;
        }

        /// If already in progress, end the current defragmentation first
        if (defragmentationContext != nullptr)
        {
            SEDX_CORE_WARN("Defragmentation already in progress, ending previous session first");
            EndDefragmentation();
        }

        /// Clear any previous candidates list
        defragmentationCandidates.clear();

        /// Create VMA defragmentation info
        VmaDefragmentationInfo defragInfo = {};
        defragInfo.flags = flags;
        defragInfo.pool = nullptr;         /// Will be filled in EndDefragmentation

        /// Set algorithm based on flags or use default
        if (flags == 0)
        {
            /// If no specific algorithm requested, use balanced by default for general purpose
            defragInfo.flags |= VMA_DEFRAGMENTATION_FLAG_ALGORITHM_BALANCED_BIT;
        }

        /// Log defragmentation start with selected algorithm
        const char *algorithmName;
        if (defragInfo.flags & VMA_DEFRAGMENTATION_FLAG_ALGORITHM_FAST_BIT)
            algorithmName = "Fast";
        else if (defragInfo.flags & VMA_DEFRAGMENTATION_FLAG_ALGORITHM_BALANCED_BIT)
            algorithmName = "Balanced";
        else if (defragInfo.flags & VMA_DEFRAGMENTATION_FLAG_ALGORITHM_FULL_BIT)
            algorithmName = "Full";
        else
            algorithmName = "Unknown";

        SEDX_CORE_INFO("Beginning memory defragmentation with {} algorithm", algorithmName);

        /// Don't create the actual context yet - do that in 'EndDefragmentation' after we've collected all the allocations to de-fragment
    }

	/**
	 * @fn EndDefragmentation
	 * @brief Ends the defragmentation process and applies the optimizations.
	 * 
	 * This function finalizes the defragmentation process by processing
	 * all marked allocations and rearranging them to reduce fragmentation.
	 */
    void MemoryAllocator::EndDefragmentation()
    {
        std::lock_guard<std::mutex> lock(allocationMutex);

        if (!memAllocatorData || !memAllocatorData->Allocator)
        {
            SEDX_CORE_ERROR("Memory allocator not initialized when ending defragmentation");
            return;
        }

        /// Check if we have candidates to de-fragment
        if (defragmentationCandidates.empty())
        {
            SEDX_CORE_WARN("No allocations marked for defragmentation");
            return;
        }

        /// Prepare defragmentation parameters
        VmaDefragmentationInfo defragInfo = {};
        defragInfo.flags = VMA_DEFRAGMENTATION_FLAG_ALGORITHM_BALANCED_BIT;
        defragInfo.pool = nullptr;            /// Use default pool
        defragInfo.maxBytesPerPass = 0;       /// No limit on bytes per pass
        defragInfo.maxAllocationsPerPass = 0; /// No limit on allocations per pass

        /// Stats to track results
        VmaDefragmentationStats defragStats = {};

        /// Create and execute defragmentation
        if (VkResult result = vmaBeginDefragmentation(memAllocatorData->Allocator, &defragInfo, &defragmentationContext); result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to begin memory defragmentation, error: {}", static_cast<int>(result));
            defragmentationContext = nullptr;
            defragmentationCandidates.clear();
            return;
        }

        /// End the defragmentation operation
        vmaEndDefragmentation(memAllocatorData->Allocator, defragmentationContext, &defragStats);

        /// Log results
        SEDX_CORE_INFO("Memory defragmentation completed:");
        SEDX_CORE_INFO("  - Bytes moved: {} MB", static_cast<double>(defragStats.bytesMoved) / (1024.0 * 1024.0));
        SEDX_CORE_INFO("  - Bytes freed: {} MB", static_cast<double>(defragStats.bytesFreed) / (1024.0 * 1024.0));
        SEDX_CORE_INFO("  - Allocations moved: {}", defragStats.allocationsMoved);
        SEDX_CORE_INFO("  - Device memory blocks freed: {}", defragStats.deviceMemoryBlocksFreed);

        /// Reset state
        defragmentationContext = nullptr;
        defragmentationCandidates.clear();

        /// Update peak memory usage stats after defragmentation
        const AllocationStats currentStats = GetStats();
        memAllocatorData->PeakMemoryUsage = currentStats.usedBytes;
    }

	/**
	 * @fn MarkForDefragmentation
	 * @brief Marks an allocation to be included in the defragmentation process
	 * 
	 * This function adds a specific allocation to the list of allocations
	 * that will be considered for defragmentation when EndDefragmentation is called.
	 *
	 * @param allocation The VMA allocation to be marked for defragmentation
	 */
    void MemoryAllocator::MarkForDefragmentation(VmaAllocation allocation)
    {
        std::lock_guard<std::mutex> lock(allocationMutex);

        if (!memAllocatorData || !memAllocatorData->Allocator)
        {
            SEDX_CORE_ERROR("Memory allocator not initialized when marking for defragmentation");
            return;
        }

        if (allocation == nullptr)
        {
            SEDX_CORE_WARN("Attempted to mark null allocation for defragmentation");
            return;
        }

        /// Check if allocation is valid and in our tracking map
        if (!AllocationMap.contains(allocation))
        {
            SEDX_CORE_WARN("Attempted to mark unknown allocation for defragmentation");
            return;
        }

        /// Check if this allocation is already marked
        if (std::ranges::find(defragmentationCandidates, allocation) != defragmentationCandidates.end())
        {
            return; /// Already marked, no need to add again
        }

        /// Get allocation info to determine if it's suitable for defragmentation
        VmaAllocationInfo allocInfo;
        vmaGetAllocationInfo(memAllocatorData->Allocator, allocation, &allocInfo);

        /// Usually only device-local allocations benefit from defragmentation
        /// However, we'll allow any allocation to be marked and let VMA handle compatibility

        /// Add to candidates list
        defragmentationCandidates.push_back(allocation);

        /// Log only occasionally to prevent spam
        if (defragmentationCandidates.size() % 100 == 1 || defragmentationCandidates.size() < 5)
        {
            const auto &[allocatedSize, allocType] = AllocationMap[allocation];
            const char *typeStr = allocType == AllocationType::Buffer ? "buffer" : "image";

            SEDX_CORE_INFO("Marked {} allocation of size {} KB for defragmentation ({} total marked)",
                           typeStr,
                           allocatedSize / 1024,
                           defragmentationCandidates.size());
        }
    }

    /// ---------------------------------------------------------

	VmaAllocation MemoryAllocator::AllocateBuffer(const VkBufferCreateInfo &bufferCreateInfo, const VmaMemoryUsage usage, VkBuffer &outBuffer)
	{
        std::lock_guard<std::mutex> lock(allocationMutex);

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = usage;

	    ApplyAllocationStrategy(allocCreateInfo);

        /// Choose appropriate memory pool based on size
        const VkDeviceSize size = bufferCreateInfo.size;
        VmaPool pool;

        if (size <= SMALL_BUFFER_SIZE)
        {
            pool = GetOrCreateBufferPool(SMALL_BUFFER_SIZE, usage);
            allocCreateInfo.pool = pool;
        }
        else if (size <= MEDIUM_BUFFER_SIZE)
        {
            pool = GetOrCreateBufferPool(MEDIUM_BUFFER_SIZE, usage);
            allocCreateInfo.pool = pool;
        }
        else if (size <= LARGE_BUFFER_SIZE)
        {
            pool = GetOrCreateBufferPool(LARGE_BUFFER_SIZE, usage);
            allocCreateInfo.pool = pool;
        }

        /// ---------------------------------------------------------

        /// Create the allocation
        VmaAllocation allocation = {};
        VmaAllocationInfo allocInfo{};

        if (const VkResult result = vmaCreateBuffer(memAllocatorData->Allocator, &bufferCreateInfo, &allocCreateInfo, &outBuffer, &allocation, &allocInfo); result != VK_SUCCESS || allocation == nullptr)
        {
            SEDX_CORE_ERROR( "Failed to allocate buffer memory: {}", static_cast<int>(result));
            return nullptr;
        }

        /// Update statistics
        memAllocatorData->BytesAllocated += allocInfo.size;
        memAllocatorData->BytesAllocated++;
        memAllocatorData->CurrentAllocations++;

        uint32_t memoryTypeIndex = allocInfo.memoryType;
        memoryTypeStats[memoryTypeIndex].BytesAllocated += allocInfo.size;
        memoryTypeStats[memoryTypeIndex].CurrentAllocations++;
        memoryTypeStats[memoryTypeIndex].BytesAllocated++;

        /// Update peak memory usage
        memAllocatorData->PeakMemoryUsage = std::max(memAllocatorData->PeakMemoryUsage, memAllocatorData->BytesAllocated);

        /// Store allocation tracking information
        AllocInfo info;
        info.AllocatedSize = allocInfo.size;
        info.Type = AllocationType::Buffer;
        AllocationMap[allocation] = info;

        return allocation;
	}

	/**
	 * @fn AllocateImage
	 * @brief Allocates a GPU image with the specified properties.
	 * 
	 * This function creates a Vulkan image and allocates memory for it using the VMA allocator.
	 * It sets up the image creation info and allocation info, and returns the created image
	 * and its associated allocation handle.
	 *
	 * @param imageCreateInfo The Vulkan image creation info structure
	 * @param usage The memory usage type for the allocation (e.g., GPU only, CPU visible)
	 * @param outImage A reference to store the created Vulkan image handle
	 * @param allocatedSize Optional pointer to store the size of the allocated memory
	 *
	 * @return The VMA allocation handle for the allocated image memory
	 */
	VmaAllocation MemoryAllocator::AllocateImage(const VkImageCreateInfo &imageCreateInfo, const VmaMemoryUsage usage, VkImage& outImage, VkDeviceSize* allocatedSize)
	{
        std::lock_guard<std::mutex> lock(allocationMutex);

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = usage;

        VmaAllocation allocation;
        vmaCreateImage(memAllocatorData->Allocator, &imageCreateInfo, &allocCreateInfo, &outImage, &allocation, nullptr);
        if (allocation == nullptr)
        {
            ErrMsg("Failed to allocate GPU image");
            return nullptr;
        }

		VmaAllocationInfo allocInfo;
		vmaGetAllocationInfo(memAllocatorData->Allocator, allocation, &allocInfo);
        if (allocatedSize)
        {
			*allocatedSize = allocInfo.size;
        }

	    memAllocatorData->BytesAllocated += allocInfo.size;

        /// Store allocation tracking information
        AllocInfo info;
        info.AllocatedSize = allocInfo.size;
        info.Type = AllocationType::Image;
        AllocationMap[allocation] = info;

        return allocation;
	}

    /// -------------------------------------------------

	/**
	 * @fn Free
	 * @brief Frees the memory associated with a Vulkan allocation.
	 * 
	 * This function releases the memory allocated for a specific Vulkan resource
	 * (buffer or image) and updates the internal tracking structures to reflect
	 * the deallocation. It ensures that all resources are properly cleaned up
	 * to prevent memory leaks.
	 *
	 * @param allocation The VMA allocation handle to be freed
	 */
    void MemoryAllocator::Free(const VmaAllocation allocation)
    {
        std::lock_guard<std::mutex> lock(allocationMutex);

        if (AllocationMap.contains(allocation))
        {
            auto &[AllocatedSize, Type] = AllocationMap[allocation];
            memAllocatorData->BytesAllocated -= AllocatedSize;
            AllocationMap.erase(allocation);
        }

        vmaFreeMemory(memAllocatorData->Allocator, allocation);
    }

    /**
	 * @fn DestroyImage
	 * @brief Destroys a Vulkan image and its associated memory allocation.
	 * 
	 * This function releases the Vulkan image and its memory allocation,
	 * ensuring that all resources are properly cleaned up to prevent memory leaks.
	 *
	 * @param image The Vulkan image handle to be destroyed
	 * @param allocation The VMA allocation handle associated with the image
	 */
    void MemoryAllocator::DestroyImage(const VkImage image, const VmaAllocation allocation)
    {
        std::lock_guard<std::mutex> lock(allocationMutex);

        if (AllocationMap.contains(allocation))
        {
            const auto &[AllocatedSize, Type] = AllocationMap[allocation];
            memAllocatorData->BytesAllocated -= AllocatedSize;
            AllocationMap.erase(allocation);
        }

        vmaDestroyImage(memAllocatorData->Allocator, image, allocation);
    }

	/**
	 * @namespace VulkanMemoryUtils
	 * @brief Utility functions for Vulkan memory management.
	 *
	 * This namespace contains helper functions for managing Vulkan memory allocations,
	 * de-allocations, and other memory-related operations.
	 */
    namespace VulkanMemoryUtils
    {
		/**
		 * @fn DestroyBuffer
		 * @brief Destroys a Vulkan buffer and its associated memory allocation.
		 *
		 * This function releases the Vulkan buffer and its memory allocation,
		 * ensuring that all resources are properly cleaned up to prevent memory leaks.
		 *
		 * @param buffer The Vulkan buffer handle to be destroyed
		 * @param allocation The VMA allocation handle associated with the buffer
		 */
		void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation)
		{
		    if (buffer != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE)
		    {
		        /// Destroy buffer with VMA
                vmaDestroyBuffer(memAllocatorData->Allocator, buffer, allocation);
		        SEDX_CORE_TRACE("Buffer destroyed successfully");
		    }
		}
    } /// namespace VulkanMemoryUtils

    /**
     * @fn DestroyBuffer
     * @brief Destroys a Vulkan buffer and its associated memory allocation.
     *
     * @param buffer The Vulkan buffer handle to be destroyed
     * @param allocation The VMA allocation handle associated with the buffer
     */
    void MemoryAllocator::DestroyBuffer(const VkBuffer buffer, const VmaAllocation allocation)
    {
        std::lock_guard<std::mutex> lock(allocationMutex);

        if (buffer != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE)
        {
            if (AllocationMap.contains(allocation))
            {
                const auto &[AllocatedSize, Type] = AllocationMap[allocation];
                memAllocatorData->BytesAllocated -= AllocatedSize;
                AllocationMap.erase(allocation);
            }

            vmaDestroyBuffer(memAllocatorData->Allocator, buffer, allocation);
        }
    }

    /**
     * @brief Gets the current custom buffer size.
     * 
     * This function returns the custom buffer size that has been set for the memory allocator,
     * or the default value if no custom size has been configured. The custom buffer size affects
     * how memory is allocated for larger buffers and can be optimized for specific workloads.
     * 
     * @return The current custom buffer size in bytes.
     * 
     * @note If customBufferSize is zero, this function will return DEFAULT_CUSTOM_BUFFER_SIZE.
     */
    VkDeviceSize MemoryAllocator::GetCustomBufferSize()
    {
        /// Return the custom buffer size if set, otherwise return the default value
        return customBufferSize ? customBufferSize : DEFAULT_CUSTOM_BUFFER_SIZE;
    }

    /**
     * @brief Sets the custom buffer size for allocations if supported by the device
     * 
     * This function configures the custom buffer size used by the memory allocator,
     * but only if the specified size meets the device's requirements. It validates
     * that:
     * 1. The buffer size is not zero
     * 2. The buffer size is properly aligned with the device's non-coherent atom size
     * 3. The device has at least one memory type with DEVICE_LOCAL_BIT property
     * 
     * Setting a custom buffer size can optimize memory allocation patterns for specific
     * workloads and memory access patterns. The value affects how larger buffers are
     * allocated and managed by the allocator.
     * 
     * @param size The desired buffer size in bytes.
     * @param device Reference to the Vulkan device for checking compatibility.
     * @return true if the custom buffer size was set successfully.
     * @return false if unsupported or invalid.
     */
    bool MemoryAllocator::SetCustomBufferSize(const VkDeviceSize size, const VulkanDevice &device)
    {
        /// Ensure the size is valid and aligned
        if (size == 0 || size % device.GetPhysicalDevice()->GetLimits().nonCoherentAtomSize != 0)
            return false;

        /// Check if the device supports the requested buffer size
        const auto& memoryProperties = device.GetPhysicalDevice()->GetMemoryProperties();
        bool isSupported = false;

        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
            if ((memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0)
            {
                isSupported = true;
                break;
            }

        if (!isSupported)
            return false;

        /// Set the custom buffer size
        customBufferSize = size;
        return true;
    }

    /**
	 * @fn UnmapMemory
	 * @brief Unmaps a previously mapped memory allocation.
	 *
	 * This function unmaps the memory associated with a specific VMA allocation,
	 * allowing it to be accessed by the GPU again. It is important to call this
	 * function after mapping memory to ensure that the GPU can access the data.
	 *
	 * @param allocation The VMA allocation handle to be unmapped
	 */
	void MemoryAllocator::UnmapMemory(const VmaAllocation allocation)
    {
		vmaUnmapMemory(memAllocatorData->Allocator, allocation);
	}

    /// ---------------------------------------------------------

    /**
     * @fn GetOrCreateBufferPool
     * @brief Retrieves or creates a buffer memory pool for the specified size and usage.
     *
     * This function checks if a memory pool for the specified size already exists.
     * If it does, it returns the existing pool. If not, it creates a new pool
     *
     * @param size The size of the buffer for which to create or retrieve the pool
     * @param usage The memory usage type for the allocation (e.g., GPU only, CPU visible)
     */
    VmaPool MemoryAllocator::GetOrCreateBufferPool(VkDeviceSize size, const VmaMemoryUsage usage)
    {
        std::lock_guard<std::mutex> lock(poolMutex);

        /// Check if pool for this size already exists
        if (const auto it = bufferPools.find(size); it != bufferPools.end())
        {
            if (const auto &pool = it->second; !pool.pools.empty())
            {
                return pool.pools[0]; /// Return the first pool (we could implement more sophisticated selection)
            }
        }
        else
        {
            /// Create new pool entry for this size
            bufferPools[size] = MemoryPool(size, usage);
        }

        /// Create new VMA pool
        MemoryPool &pool = bufferPools[size];

        VmaPoolCreateInfo poolInfo = pool.createInfo;
        poolInfo.memoryTypeIndex = 0; /// You might want to determine this based on usage
        poolInfo.blockSize = size;

        VmaPool newPool = {};

        if (const VkResult result = vmaCreatePool(memAllocatorData->Allocator, &poolInfo, &newPool); result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to create memory pool of size {}: {}", size, static_cast<int>(result));
            return nullptr;
        }

        pool.pools.push_back(newPool);
        return newPool;
    }

    /**
	 * @fn GetOrCreateImagePool
	 * @brief Retrieves or creates an image memory pool for the specified size and usage.
	 *
	 * This function checks if a memory pool for the specified size already exists.
	 * If it does, it returns the existing pool. If not, it creates a new pool
	 *
	 * @param size The size of the image for which to create or retrieve the pool
	 * @param usage The memory usage type for the allocation (e.g., GPU only, CPU visible)
	 */
    VmaPool MemoryAllocator::GetOrCreateImagePool(VkDeviceSize size, VmaMemoryUsage usage)
    {
        std::lock_guard<std::mutex> lock(poolMutex);

        /// Check if pool for this size already exists
        if (const auto it = imagePools.find(size); it != imagePools.end())
        {
            if (const auto &pool = it->second; !pool.pools.empty())
            {
                return pool.pools[0]; /// Return the first pool (we could implement more sophisticated selection)
            }
        }
        else
        {
            /// Create new pool entry for this size
            imagePools[size] = MemoryPool(size, usage);
        }

        /// Create new VMA pool
        MemoryPool &pool = imagePools[size];

        VmaPoolCreateInfo poolInfo = pool.createInfo;
        poolInfo.memoryTypeIndex = 0; /// You might want to determine this based on usage
        poolInfo.blockSize = size;

        VmaPool newPool = {};

        if (const VkResult result = vmaCreatePool(memAllocatorData->Allocator, &poolInfo, &newPool); result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to create memory pool of size {}: {}", size, static_cast<int>(result));
            return nullptr;
        }

        pool.pools.push_back(newPool);
        return newPool;
    }

    /**
	 * @brief Checks if memory usage is exceeding the warning threshold
	 * 
	 * This function queries the VMA budget and determines if memory usage
	 * exceeds the configured warning threshold.
	 * 
	 * @return true if the memory is over budget (exceeding threshold)
	 * @return false if memory usage is within acceptable limits
	 */
    bool MemoryAllocator::CheckMemoryBudget() const
    {
        if (!memAllocatorData || !memAllocatorData->Allocator)
        {
            SEDX_CORE_ERROR("Memory allocator not initialized when checking budget");
            return false;
        }

        /// Get memory budget from VMA
        VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
        vmaGetHeapBudgets(memAllocatorData->Allocator, budgets);

        /// Get physical device properties to determine number of heaps
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(GraphicsEngine::GetCurrentDevice()->GetPhysicalDevice()->GetGPUDevices(), &memProps);

        uint64_t totalAllocation = 0;
        uint64_t totalBudget = 0;

        /// Calculate totals across all heaps
        for (uint32_t i = 0; i < memProps.memoryHeapCount; i++)
        {
            totalAllocation += budgets[i].usage;
            totalBudget += budgets[i].budget;

            /// Log individual heap usage if it's close to budget
            if (const float usagePercent = static_cast<float>(budgets[i].usage) / static_cast<float>(budgets[i].budget); usagePercent > memoryWarningThreshold)
            {
                SEDX_CORE_WARN("Memory heap {} is at {:.1f}% usage ({} MB / {} MB)",
                               i,
                               usagePercent * 100.0f,
                               budgets[i].usage / (1024 * 1024),
                               budgets[i].budget / (1024 * 1024));
            }
        }

        const float totalUsagePercent = static_cast<float>(totalAllocation) / static_cast<float>(totalBudget);
        const bool isOverBudget = totalUsagePercent > memoryWarningThreshold;

        if (isOverBudget)
        {
            SEDX_CORE_WARN("Total GPU memory usage exceeds threshold: {:.1f}% ({} MB / {} MB)",
                           totalUsagePercent * 100.0f,
                           totalAllocation / (1024 * 1024),
                           totalBudget / (1024 * 1024));
        }

        return isOverBudget;
    }

    /**
     * @fn AlignBufferSize
     * @brief Aligns the buffer size to the nearest power of two or custom alignment.
     *
     * This function ensures that the buffer size is aligned to a specified
     * alignment value or to a default alignment based on the size of the buffer.
     *
     * @param size The size of the buffer to be aligned
     * @return The aligned size of the buffer
     */
    VkDeviceSize MemoryAllocator::AlignBufferSize(VkDeviceSize size) const
    {
        if (size == 0)
            return 0;

        /// Apply custom alignment if specified
        if (customBufferAlignment > 0)
        {
            /// Round up to the next multiple of customBufferAlignment
            return ((size + customBufferAlignment - 1) / customBufferAlignment) * customBufferAlignment;
        }

        /// If no custom alignment is set but size is small, round up to improve cache efficiency
        if (size < SMALL_BUFFER_SIZE)
        {
            /// For small buffers, align to 256 bytes to improve cache efficiency
            constexpr VkDeviceSize defaultSmallBufferAlignment = 256;
            return ((size + defaultSmallBufferAlignment - 1) / defaultSmallBufferAlignment) * defaultSmallBufferAlignment;
        }

        /// For medium to large buffers, align to 4KB (typical page size)
        if (size < LARGE_BUFFER_SIZE)
        {
            constexpr VkDeviceSize pageSize = 4 * 1024; /// 4KB
            return ((size + pageSize - 1) / pageSize) * pageSize;
        }

        /// For very large buffers, use default alignment
        return size;
    }

    // ---------------------------------------------------------

    /**
     * @fn Init
     * @brief Initializes the memory allocator with the specified Vulkan device.
     *
     * @param device The Vulkan device to be used for memory allocation
     */
    void MemoryAllocator::Init(const Ref<VulkanDevice> &device)
    {
        memAllocatorData = hnew VulkanAllocatorData();

		RenderData renderData;
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.vulkanApiVersion = renderData.minVulkanVersion;
        allocatorInfo.physicalDevice = device->GetPhysicalDevice()->GetGPUDevices();
        allocatorInfo.device = device->Selected();
        allocatorInfo.instance = GraphicsEngine::GetInstance();

        vmaCreateAllocator(&allocatorInfo, &memAllocatorData->Allocator);
	}

    /**
     * @fn Shutdown
     * @brief Cleans up the memory allocator and releases all resources.
     *
     * This function destroys the VMA allocator instance and cleans up
     * any remaining resources associated with the memory allocator.
     */
    void MemoryAllocator::Shutdown()
    {
        vmaDestroyAllocator(memAllocatorData->Allocator);
        hdelete memAllocatorData;
        memAllocatorData = nullptr;
    }

    /**
     * @fn ContainsAllocation
     * @brief Checks if the allocator contains a specific allocation.
     *
     * This function checks if the specified VMA allocation is present
     * in the internal allocation tracking map.
     *
     * @param allocation The VMA allocation handle to check
     * @return true if the allocation is found
     * @return false if the allocation is not found
     */
    bool MemoryAllocator::ContainsAllocation(VmaAllocation allocation)
    {
        return AllocationMap.contains(allocation);
    }

    /**
	 * @fn GetMemAllocator
	 * @brief Retrieves the Vulkan memory allocator instance.
	 *
	 * This function returns the VMA allocator instance used for memory management.
	 *
	 * @return The VMA allocator instance
	 */
    VmaAllocator MemoryAllocator::GetAllocator()
    {
        SEDX_ASSERT(memAllocatorData != nullptr, "Memory allocator data is null");
        return memAllocatorData->Allocator;
    }

	/**
	 * @brief Gets the current memory allocation statistics
	 * 
	 * This function queries the VMA for current allocation statistics
	 * and returns them in a structured format.
	 * 
	 * @return AllocationStats containing memory usage information
	 */
    MemoryAllocator::AllocationStats MemoryAllocator::GetStats()
    {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex &>(allocationMutex));

        AllocationStats stats{};

        if (!memAllocatorData || !memAllocatorData->Allocator)
        {
            SEDX_CORE_ERROR("Memory allocator not initialized when getting stats");
            return stats;
        }

        /// Get VMA statistics
        VmaTotalStatistics vmaStats;
        vmaCalculateStatistics(memAllocatorData->Allocator, &vmaStats);

        /// Fill our custom stats structure
        stats.totalBytes = vmaStats.total.statistics.blockBytes;
        stats.usedBytes = vmaStats.total.statistics.allocationBytes;
        stats.allocationCount = vmaStats.total.statistics.allocationCount;

        /// Calculate fragmentation ratio (if no blocks, fragmentation is 0)
        if (vmaStats.total.statistics.blockCount > 0)
        {
            /// Calculate fragmentation as 1 - (used / allocated)
            /// This represents the proportion of allocated memory that's not being used
            stats.fragmentationRatio = 1.0f - ((float)vmaStats.total.statistics.allocationBytes / (float)vmaStats.total.statistics.blockBytes);
        }
        else
        {
            stats.fragmentationRatio = 0.0f;
        }

        return stats;
    }

	/**
	 * @fn PrintDetailedStats
	 * @brief Prints detailed memory allocation statistics to the log
	 * 
	 * This function outputs comprehensive memory usage information to help
	 * with debugging and monitoring memory usage patterns.
	 *
	 * @note This function is thread-safe and locks the allocation mutex
	 * to ensure consistent access to memory statistics.
	 */
    void MemoryAllocator::PrintDetailedStats() const
    {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex &>(allocationMutex));

        if (!memAllocatorData || !memAllocatorData->Allocator)
        {
            SEDX_CORE_ERROR("Memory allocator not initialized when printing stats");
            return;
        }

        /// Get statistics from VMA
        VmaTotalStatistics vmaStats;
        vmaCalculateStatistics(memAllocatorData->Allocator, &vmaStats);

        /// Get memory budget from VMA
        VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
        vmaGetHeapBudgets(memAllocatorData->Allocator, budgets);

        /// Get physical device properties to determine number of heaps
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(GraphicsEngine::GetCurrentDevice()->GetPhysicalDevice()->GetGPUDevices(), &memProps);

        SEDX_CORE_INFO("----------- VULKAN MEMORY ALLOCATION STATS -----------");
        SEDX_CORE_INFO("Tag: {}", Tag_);
        SEDX_CORE_INFO("Total memory allocated: {} MB", vmaStats.total.statistics.blockBytes / (1024 * 1024));
        SEDX_CORE_INFO("Memory used by allocations: {} MB", vmaStats.total.statistics.allocationBytes / (1024 * 1024));
        SEDX_CORE_INFO("Memory wasted (fragmentation): {} MB", (vmaStats.total.statistics.blockBytes - vmaStats.total.statistics.allocationBytes) / (1024 * 1024));
        SEDX_CORE_INFO( "Fragmentation ratio: {:.2f}%", (1.0f - static_cast<float>(vmaStats.total.statistics.allocationBytes) / static_cast<float>(vmaStats.total.statistics.blockBytes)) * 100.0f);
        SEDX_CORE_INFO("Total allocation count: {}", vmaStats.total.statistics.allocationCount);
        SEDX_CORE_INFO("Total block count: {}", vmaStats.total.statistics.blockCount);

        /// Print per-heap statistics
        SEDX_CORE_INFO("-------- MEMORY HEAP DETAILS (BUDGETs) --------");
        for (uint32_t i = 0; i < memProps.memoryHeapCount; i++)
        {
            const char *heapType = (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) ? "DEVICE" : "HOST";
            float usagePercent = static_cast<float>(budgets[i].usage) / static_cast<float>(budgets[i].budget) * 100.0f;

            SEDX_CORE_INFO("Heap {}: {} - Size: {} MB, Used: {} MB ({:.1f}%)", i, heapType, budgets[i].budget / (1024 * 1024), budgets[i].usage / (1024 * 1024), usagePercent);
        }

        SEDX_CORE_INFO("-------- MEMORY TYPE DETAILS --------");
        for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
        {
            /// Skip if no memory is allocated from this type
            if (memoryTypeStats[i].BytesAllocated == 0) continue;

            /// Determine memory type properties string
            std::string propertyStr;
            const VkMemoryPropertyFlags flags = memProps.memoryTypes[i].propertyFlags;

            if (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) propertyStr += "DEVICE_LOCAL ";
            if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) propertyStr += "HOST_VISIBLE ";
            if (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) propertyStr += "HOST_COHERENT ";
            if (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) propertyStr += "HOST_CACHED ";
            if (flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) propertyStr += "LAZILY_ALLOCATED ";

            SEDX_CORE_INFO("Type {}: Heap {}, Properties: {}", i, memProps.memoryTypes[i].heapIndex, propertyStr);
            SEDX_CORE_INFO("  Allocated: {} MB, Active allocations: {}",
                           (memoryTypeStats[i].BytesAllocated - memoryTypeStats[i].BytesFreed) / (1024 * 1024),
                           memoryTypeStats[i].CurrentAllocations);
        }

        SEDX_CORE_INFO("Peak memory usage: {} MB", memAllocatorData->PeakMemoryUsage / (1024 * 1024));
        SEDX_CORE_INFO("--------------------------------------------------------");
    }

	/**
	 * @fn ResetStats
	 * @brief Resets memory allocation statistics tracking
	 * 
	 * This function clears the statistics counters without affecting
	 * actual memory allocations.
	 *
	 * @note This function is thread-safe and locks the allocation mutex
	 * to ensure consistent access to memory statistics.
	 */
    void MemoryAllocator::ResetStats()
    {
        std::lock_guard<std::mutex> lock(allocationMutex);

        if (!memAllocatorData)
        {
            SEDX_CORE_ERROR("Memory allocator not initialized when resetting stats");
            return;
        }

        /// Reset our allocation tracking statistics
        for (auto &[Allocator, BytesAllocated, BytesFreed, CurrentAllocations, PeakMemoryUsage] :
             memoryTypeStats)
        {
            /// Keep track of current allocations, but reset historical tracking
            const uint64_t currentAllocCount = CurrentAllocations;
            const uint64_t currentBytes = CurrentAllocations - BytesFreed;

            BytesAllocated = currentBytes;
            BytesFreed = 0;
            BytesAllocated = currentAllocCount;
        }

        /// Reset peak memory usage to current usage
        memAllocatorData->PeakMemoryUsage = memAllocatorData->BytesAllocated;

        SEDX_CORE_INFO("Memory allocation statistics have been reset");
    }

    /**
     * @brief Sets the memory allocation strategy
     *
     * This function allows the user to specify the memory allocation strategy
     * to be used by the allocator. The strategy can be set to optimize for
     * speed, memory usage, or use the default strategy.
     *
     * @param strategy The desired allocation strategy
     *
     * @note This function is thread-safe and locks the allocation mutex
     * to ensure consistent access to the allocation strategy.
     */
    void MemoryAllocator::SetAllocationStrategy(const AllocationStrategy strategy)
    {
        std::lock_guard<std::mutex> lock(allocationMutex);

        if (strategy != currentStrategy)
        {
            SEDX_CORE_INFO_TAG("VulkanAllocator", "Changing memory allocation strategy from {} to {}",
                               currentStrategy == AllocationStrategy::Default          ? "Default"
                               : currentStrategy == AllocationStrategy::SpeedOptimized ? "SpeedOptimized" : "MemoryOptimized",
                               strategy == AllocationStrategy::Default          ? "Default"
                               : strategy == AllocationStrategy::SpeedOptimized ? "SpeedOptimized" : "MemoryOptimized");

            currentStrategy = strategy;
        }
    }


    /**
     * @fn ApplyAllocationStrategy
     * @brief Applies the current allocation strategy to the VMA allocation create info.
     *
     * This function modifies the VMA allocation create info structure
     * to apply the current allocation strategy.
     *
     *
     * @param createInfo The VMA allocation create info structure to modify
     */
    void MemoryAllocator::ApplyAllocationStrategy(VmaAllocationCreateInfo &createInfo) const
    {
        switch (currentStrategy)
        {
        case AllocationStrategy::SpeedOptimized:
            /// Optimize for speed - prefer pre-allocated memory
            createInfo.flags |= VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT;
            /// For speed-optimized allocations, we don't need to be as strict about finding the perfect fit
            createInfo.flags &= ~VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;
            createInfo.flags &= ~VMA_DEFRAGMENTATION_FLAG_ALGORITHM_FAST_BIT;
            break;

        case AllocationStrategy::MemoryOptimized:
            /// Optimize for memory efficiency - try to find the smallest fitting block
            createInfo.flags |= VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;
            /// For memory-optimized allocations, also try to minimize fragmentation
            createInfo.flags |= VMA_DEFRAGMENTATION_FLAG_ALGORITHM_BALANCED_BIT;
            /// But we don't care about allocation time as much
            createInfo.flags &= ~VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT;
            break;

        case AllocationStrategy::Default:
        default:
            /// Let VMA decide the best strategy - don't set any specific strategy flags
            createInfo.flags &= ~(VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT |
								  VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT |
								  VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT |
								  VMA_DEFRAGMENTATION_FLAG_ALGORITHM_MASK);
            break;
        }
    }

	/**
	 * @fn GetMemoryBudget
	 * @brief Gets the current memory budget information
	 * 
	 * This function retrieves memory budget details from the VMA
	 * to provide information about total memory and current usage.
	 * 
	 * @return MemoryBudget containing memory usage and budget information
	 */
    MemoryAllocator::MemoryBudget MemoryAllocator::GetMemoryBudget() const
    {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex &>(allocationMutex));

        MemoryBudget budget{};

        if (!memAllocatorData || !memAllocatorData->Allocator)
        {
            SEDX_CORE_ERROR("Memory allocator not initialized when getting budget");
            return budget;
        }

        /// Get memory budget from VMA
        VmaBudget vmabudgets[VK_MAX_MEMORY_HEAPS];
        vmaGetHeapBudgets(memAllocatorData->Allocator, vmabudgets);

        /// Get physical device properties to determine number of heaps
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(GraphicsEngine::GetCurrentDevice()->GetPhysicalDevice()->GetGPUDevices(), &memProps);

        uint64_t totalBudget = 0;
        uint64_t totalUsage = 0;

        /// Calculate totals across all heaps
        for (uint32_t i = 0; i < memProps.memoryHeapCount; i++)
        {
            totalBudget += vmabudgets[i].budget;
            totalUsage += vmabudgets[i].usage;
        }

        budget.totalBytes = totalBudget;
        budget.usedBytes = totalUsage;
        budget.usagePercentage = totalBudget > 0 ? ((float)totalUsage / (float)totalBudget) : 0.0f;
        budget.isOverBudget = budget.usagePercentage > memoryWarningThreshold;

        return budget;
    }

    /**
	 * @fn SetMemoryUsageWarningThreshold
	 * @brief Sets the memory usage warning threshold.
	 *
	 * This function allows the user to specify a threshold for memory usage
	 * warnings. If the memory usage exceeds this threshold, a warning will be logged.
	 *
	 * @param percentage The percentage of memory usage (0.0 to 1.0)
	 */
    void MemoryAllocator::SetMemoryUsageWarningThreshold(float percentage)
    {
        std::lock_guard<std::mutex> lock(allocationMutex);

        if (percentage <= 0.0f || percentage > 1.0f)
        {
            SEDX_CORE_WARN_TAG("Memory Allocator",
                "Invalid memory warning threshold value: {}, must be between 0.0 and 1.0. Using default value (0.9)", percentage);
            percentage = 0.9f;
        }

        memoryWarningThreshold = percentage;
        SEDX_CORE_INFO_TAG("VulkanAllocator", "Memory usage warning threshold set to {:.1f}%", percentage * 100.0f);

        /// Check current memory status against new threshold
        if (memAllocatorData && memAllocatorData->Allocator)
        {
            if (!CheckMemoryBudget())
                SEDX_CORE_WARN_TAG("Memory Allocator", "Memory budget exceeded!");
        }
    }


	/**
	 * @fn SetBufferAlignment
	 * @brief Sets the custom buffer alignment for memory allocations.
	 *
	 * This function allows the user to specify a custom alignment value
	 * for buffer memory allocations. The alignment must be a power of 2.
	 * If the alignment is not a power of 2, it will be rounded up to the next power of 2.
	 *
	 * @param alignment The custom buffer alignment value (must be a power of 2)
	 */
    void MemoryAllocator::SetBufferAlignment(VkDeviceSize alignment)
    {
        std::lock_guard<std::mutex> lock(allocationMutex);

        /// Ensure alignment is a power of 2
        if (alignment & (alignment - 1))
        {
            SEDX_CORE_WARN_TAG("VulkanAllocator", "Buffer alignment must be a power of 2, got {}", alignment);

            /// Round up to the next power of 2
            VkDeviceSize powerOf2 = 1;
            while (powerOf2 < alignment)
                powerOf2 *= 2;

            alignment = powerOf2;
            SEDX_CORE_INFO_TAG("VulkanAllocator", "Rounded buffer alignment to {} (next power of 2)", alignment);
        }

        customBufferAlignment = alignment;
        SEDX_CORE_INFO_TAG("VulkanAllocator", "Custom buffer alignment set to {} bytes", alignment);
    }

    /**
     * @fn AllocateBufferBatch
     * @brief Allocates a batch of Vulkan buffers with the specified sizes and usage.
     *
     * This function allocates multiple Vulkan buffers in a single batch operation,
     * allowing for more efficient memory allocation. It takes a vector of sizes,
     * a usage flag, and a memory usage type. The function returns a vector of
     * BatchBufferAllocation structures containing the allocated buffers and their sizes.
     * 
     * @param sizes A vector of sizes for the buffers to be allocated
     * @param usage The usage flag for the buffers (e.g., VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
     * @param memoryUsage The memory usage type (e.g., VMA_MEMORY_USAGE_GPU_ONLY)
     * @return A vector of BatchBufferAllocation structures containing the allocated buffers and their sizes
     */
    std::vector<MemoryAllocator::BatchBufferAllocation> MemoryAllocator::AllocateBufferBatch(const std::vector<VkDeviceSize> &sizes, const BufferUsageFlags usage, const VmaMemoryUsage memoryUsage) const
    {
        std::vector<BatchBufferAllocation> allocations;

        if (sizes.empty())
        {
            SEDX_CORE_WARN_TAG("VulkanAllocator", "Attempted to allocate empty buffer batch");
            return allocations;
        }

        if (!memAllocatorData || !memAllocatorData->Allocator)
        {
            SEDX_CORE_ERROR_TAG("VulkanAllocator", "Memory allocator not initialized when allocating buffer batch");
            return allocations;
        }

        allocations.reserve(sizes.size());
        uint64_t totalAllocation = 0;

        /// Apply allocation strategy
        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = memoryUsage;
        ApplyAllocationStrategy(allocCreateInfo);

        /// Allocate each buffer
        for (const auto &size : sizes)
        {
            if (size == 0)
                continue;

            const VkDeviceSize alignedSize = AlignBufferSize(size);

            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = alignedSize;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            BatchBufferAllocation allocation = {};
            VmaAllocationInfo allocInfo = {};

            const VkResult vkResult = vmaCreateBuffer(memAllocatorData->Allocator,
                                                &bufferInfo,
                                                &allocCreateInfo,
                                                &allocation.buffer,
                                                &allocation.allocation,
                                                &allocInfo);

            if (vkResult != VK_SUCCESS)
            {
                SEDX_CORE_ERROR_TAG("VulkanAllocator",
                                    "Failed to allocate buffer in batch, error: {}",
                                    static_cast<int>(vkResult));
                continue;
            }

            allocation.size = alignedSize;
			
            /// Update allocation tracking
            totalAllocation += allocInfo.size;
            memAllocatorData->BytesAllocated += allocInfo.size;
            memAllocatorData->BytesAllocated++;
            memAllocatorData->CurrentAllocations++;
			
            /// Update per-memory-type statistics
            const uint32_t memoryTypeIndex = allocInfo.memoryType;
            memoryTypeStats[memoryTypeIndex].BytesAllocated += allocInfo.size;
            memoryTypeStats[memoryTypeIndex].CurrentAllocations++;
            memoryTypeStats[memoryTypeIndex].BytesAllocated++;

            /// Store allocation info
            AllocInfo info;
            info.AllocatedSize = allocInfo.size;
            info.Type = AllocationType::Buffer;
            AllocationMap[allocation.allocation] = info;
            allocations.push_back(allocation);
        }

        /// Update peak memory usage
        memAllocatorData->PeakMemoryUsage =
            std::max(memAllocatorData->PeakMemoryUsage, memAllocatorData->BytesAllocated);

        if (!allocations.empty())
        {
            SEDX_CORE_INFO_TAG("VulkanAllocator",
                               "Allocated batch of {} buffers totaling {} MB",
                               allocations.size(), static_cast<double>(totalAllocation) / (1024.0 * 1024.0));
        }

        return allocations;
    }

    /**
     * @fn FreeBufferBatch
     * @brief Frees a batch of Vulkan buffers and their associated memory allocations.
     *
     * This function releases the Vulkan buffers and their memory allocations
     * in a single batch operation. It takes a vector of BatchBufferAllocation
     * structures containing the buffers and their allocations.
     *
     * @param allocations A vector of BatchBufferAllocation structures containing the buffers and their allocations
     */
    void MemoryAllocator::FreeBufferBatch(const std::vector<BatchBufferAllocation> &allocations)
    {
        if (allocations.empty())
            return;

        std::lock_guard<std::mutex> lock(allocationMutex);

        if (!memAllocatorData || !memAllocatorData->Allocator)
        {
            SEDX_CORE_ERROR_TAG("VulkanAllocator", "Memory allocator not initialized when freeing buffer batch");
            return;
        }

        uint64_t totalFreed = 0;
        size_t count = 0;

        for (const auto &allocation : allocations)
        {
            if (allocation.buffer == VK_NULL_HANDLE || allocation.allocation == nullptr)
                continue;

            /// Track total memory being freed
            if (AllocationMap.contains(allocation.allocation))
            {
                const auto &[AllocatedSize, Type] = AllocationMap[allocation.allocation];
                totalFreed += AllocatedSize;
                memAllocatorData->BytesAllocated -= AllocatedSize;
                AllocationMap.erase(allocation.allocation);
            }

            /// Destroy the buffer
            vmaDestroyBuffer(memAllocatorData->Allocator, allocation.buffer, allocation.allocation);
            count++;
        }

        if (count > 0)
        {
            SEDX_CORE_INFO_TAG("VulkanAllocator", "Freed batch of {} buffers totaling {} MB", count, static_cast<double>(totalFreed) / (1024.0 * 1024.0));
        }
    }

} // namespace SceneryEditorX

// -------------------------------------------------------

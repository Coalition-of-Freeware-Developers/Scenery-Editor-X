/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* memory.cpp
* -------------------------------------------------------
* Created: 11/4/2025
* -------------------------------------------------------
*/
#include "memory.h"
#include <mutex>

/// -------------------------------------------------------

namespace SceneryEditorX
{

	static AllocationStats GlobalStats;
	static bool InInit_ = false;

    /// -------------------------------------------------------

    /**
     * @brief Initializes the memory allocation tracking system
     * 
     * This function creates and initializes the core AllocatorData structure that
     * tracks memory allocations throughout the application. The initialization
     * uses AllocateRaw to avoid recursive allocation issues during setup.
     * 
     * It is designed to be safe when called multiple times - subsequent calls
     * will return immediately if the allocator has already been initialized.
     * 
     * The function sets the InInit_ flag during initialization to prevent
     * recursive initialization attempts.
     */
	void Allocator::Init()
	{
	    if (Data_)
            return;

        InInit_ = true;
	    AllocatorData *data = static_cast<AllocatorData *>(AllocateRaw(sizeof(AllocatorData)));
	    new (data) AllocatorData();
	    Data_ = data;
	    InInit_ = false;
	}

    /**
     * @brief Allocates raw memory without tracking or statistics
     * 
     * This function performs a raw memory allocation using malloc without any tracking
     * in the allocation system. It's primarily used during initialization of the allocator
     * itself to avoid recursive allocation problems, and in other cases where memory
     * tracking would cause issues.
     * 
     * @param size The number of bytes to allocate
     * @return Pointer to the allocated memory block, or nullptr if allocation fails
     */
    void* Allocator::AllocateRaw(const size_t size) { return malloc(size); }

    /**
     * @brief Allocates memory from the system and tracks it in the allocation system
     * 
     * This function allocates memory through the standard malloc call and registers
     * the allocation in the tracking system. It updates global statistics to maintain
     * a record of memory usage.
     * 
     * Special handling is provided for when:
     * - The allocator is initializing (to avoid recursive initialization)
     * - The tracking data hasn't been initialized yet
     * 
     * When profiling is enabled, the allocation is also reported to the Tracy profiler.
     * 
     * @param size The number of bytes to allocate
     * @return Pointer to the allocated memory block
     */
	void* Allocator::Allocate(size_t size)
	{
	    if (InInit_)
            return AllocateRaw(size);

        if (!Data_)
            Init();

        void *memory = malloc(size);
	
	    {
	        std::scoped_lock<std::mutex> lock(Data_->Mutex_);
            Allocation &alloc = Data_->AllocationMap[memory];
	        alloc.Memory = memory;
	        alloc.Size = size;
	
	        GlobalStats.TotalAllocated += size;
	    }
	
	#if SEDX_ENABLE_PROFILING
	    TracyAlloc(memory, size);
	#endif
	
	    return memory;
	}

    /**
     * @brief Allocates memory from the system with a specified category descriptor
     * 
     * This function allocates memory through the standard malloc call and registers
     * the allocation in the tracking system with a specific category identifier.
     * The category allows for more granular memory usage tracking, as statistics
     * are maintained per category.
     * 
     * If the allocator is not yet initialized, it calls Init() to set up the
     * tracking system before proceeding with the allocation.
     * 
     * The function updates both the global statistics and category-specific
     * statistics to maintain a complete record of memory usage.
     * 
     * When profiling is enabled, the allocation is also reported to the Tracy profiler.
     * 
     * @param size The number of bytes to allocate
     * @param desc Category descriptor string to identify this allocation type
     * @return Pointer to the allocated memory block
     */
	void *Allocator::Allocate(size_t size, const char *desc)
	{
	    if (!Data_)
            Init();

        void *memory = malloc(size);
	    {
	        std::scoped_lock lock(Data_->Mutex_);
            auto &[Memory, Size, Category] = Data_->AllocationMap[memory];
	        Memory = memory;
	        Size = size;
	        Category = desc;
	
	        GlobalStats.TotalAllocated += size;
	        if (desc)
                Data_->AllocStatsMap[desc].TotalAllocated += size;
        }
	
	#if SEDX_ENABLE_PROFILING
	    TracyAlloc(memory, size);
	#endif
	
	    return memory;
	}

    /**
     * @brief Allocates memory from the system with file and line information for tracking
     * 
     * This function allocates memory through the standard malloc call and registers
     * the allocation in the tracking system with file path and line number information.
     * This variant is particularly useful for debugging memory leaks, as it allows
     * allocations to be traced back to specific source code locations.
     * 
     * If the allocator is not yet initialized, it calls Init() to set up the
     * tracking system before proceeding with the allocation.
     * 
     * The function uses the file path as a category identifier and updates both global
     * and file-specific statistics to maintain a comprehensive record of memory usage.
     * This helps identify which source files are responsible for memory allocations.
     * 
     * When profiling is enabled, the allocation is also reported to the Tracy profiler.
     * 
     * @param size The number of bytes to allocate
     * @param file Source file path where the allocation is requested from
     * @param line Line number in the source file where the allocation is requested
     * @return Pointer to the allocated memory block
     */
	void* Allocator::Allocate(size_t size, const char *file, int line)
	{
	    if (!Data_)
            Init();

        void *memory = malloc(size);
	
	    {
	        std::scoped_lock lock(Data_->Mutex_);
            auto &[Memory, Size, Category] = Data_->AllocationMap[memory];
	        Memory = memory;
	        Size = size;
	        Category = file;
	
	        GlobalStats.TotalAllocated += size;
	        Data_->AllocStatsMap[file].TotalAllocated += size;
	    }
	
	#if SEDX_ENABLE_PROFILING
	    TracyAlloc(memory, size);
	#endif
	
	    return memory;
	}

    /**
     * @brief Deallocates memory and removes tracking information for the allocation
     * 
     * This function deallocates a previously allocated memory block and updates
     * the tracking statistics accordingly. It is designed to be thread-safe with
     * proper mutex locking around critical sections.
     * 
     * The function performs the following operations:
     * 1. Returns immediately if the memory pointer is null
     * 2. Locates the allocation entry in the tracking map
     * 3. Updates global statistics and category-specific statistics if the allocation is found
     * 4. Removes the allocation from the tracking map
     * 5. Reports the deallocation to the Tracy profiler if profiling is enabled
     * 6. Issues a fatal error in debug builds if the memory block was not found in tracking
     * 7. Calls free() to release the memory back to the system
     * 
     * @param memory Pointer to the memory block to be deallocated
     */
	void Allocator::Free(void *memory)
	{
	    if (memory == nullptr)
            return;

        {
	        bool found;
	        {
	            std::scoped_lock lock(Data_->Mutex_);
                const auto allocMapIt = Data_->AllocationMap.find(memory);
                found = allocMapIt != Data_->AllocationMap.end();
	            if (found)
	            {
	                const Allocation &alloc = allocMapIt->second;
	                GlobalStats.TotalFreed += alloc.Size;
	                if (alloc.Category)
                    {
                        Data_->AllocStatsMap[alloc.Category].TotalFreed += alloc.Size;
                    }
	
	                Data_->AllocationMap.erase(memory);
	            }
	        }
	
	#if SEDX_ENABLE_PROFILING
	        TracyFree(memory);
	#endif
	
	#ifndef SEDX_DIST
	        if (!found)
                SEDX_CORE_FATAL_TAG("Memory", "Memory block {0} not present in alloc map", memory);
#endif
	    }
	
	    free(memory);
	}

	namespace Memory
	{
		/**
		 * @brief Returns statistics about memory allocation for monitoring purposes
		 * 
		 * This function provides access to the global memory allocation statistics maintained
		 * by the memory system. These statistics include the total number of bytes allocated
		 * and freed since the start of the application.
		 * 
		 * The statistics can be used to:
		 * - Monitor overall memory usage
		 * - Detect memory leaks (by comparing TotalAllocated to TotalFreed)
		 * - Generate memory usage reports
		 * 
		 * @return A constant reference to the AllocationStats structure containing the global 
		 *         allocation statistics
		 */
		const AllocationStats &GetAllocationStats() { return GlobalStats; }
	}

}

#if defined(SEDX_TRACK_MEMORY) && defined(SEDX_PLATFORM_WINDOWS)

/**
 * @brief Global override of the standard operator new
 * 
 * This function redirects all dynamic memory allocations to the SceneryEditorX tracking system.
 * It allows the memory system to record statistics, detect leaks, and provide allocation
 * information for debugging and profiling.
 * 
 * @param size The number of bytes to allocate
 * @return Pointer to the allocated memory
 * @note - This override is only active when SEDX_TRACK_MEMORY and SEDX_PLATFORM_WINDOWS are defined
 */
_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size)
_VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t size) { return Allocator::Allocate(size); }

/**
 * @brief Global override of the standard operator new[]
 * 
 * This function redirects all dynamic array allocations to the SceneryEditorX tracking system.
 * The memory system will record the allocation and update global statistics.
 * 
 * @param size The number of bytes to allocate for the array
 * @return Pointer to the allocated memory block
 * @note - This override is only active when SEDX_TRACK_MEMORY and SEDX_PLATFORM_WINDOWS are defined
 */
_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size)
_VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t size) {  return Allocator::Allocate(size); }

/**
 * @brief Placement new operator with category descriptor
 * 
 * This override allows allocations to be tagged with a category descriptor string,
 * enabling more granular memory tracking and statistics collection based on
 * allocation purpose or component type.
 * 
 * @param size The number of bytes to allocate
 * @param desc Category descriptor string for this allocation
 * @return Pointer to the allocated memory block
 * @note - This override is only active when SEDX_TRACK_MEMORY and SEDX_PLATFORM_WINDOWS are defined
 */
_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size)
_VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t size, const char *desc) { return Allocator::Allocate(size, desc); }

/**
 * @brief Placement new[] operator with category descriptor
 * 
 * This override allows array allocations to be tagged with a category descriptor string,
 * enabling more granular memory tracking and statistics collection based on
 * allocation purpose or component type.
 * 
 * @param size The number of bytes to allocate for the array
 * @param desc Category descriptor string for this allocation
 * @return Pointer to the allocated memory block
 * @note - This override is only active when SEDX_TRACK_MEMORY and SEDX_PLATFORM_WINDOWS are defined
 */
_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size)
_VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t size, const char *desc) { return Allocator::Allocate(size, desc); }

/**
 * @brief Placement new operator with source location information
 * 
 * This override captures the source file and line number where the allocation occurs,
 * which is valuable for memory leak detection and debugging. It allows the memory
 * tracking system to report exactly where problematic allocations originated.
 * 
 * @param size The number of bytes to allocate
 * @param file Source file path where the allocation is requested
 * @param line Line number in the source file where the allocation is requested
 * @return Pointer to the allocated memory block
 * @note - This override is only active when SEDX_TRACK_MEMORY and SEDX_PLATFORM_WINDOWS are defined
 */
_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size)
_VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t size, const char *file, int line) { return Allocator::Allocate(size, file, line); }

/**
 * @brief Placement new[] operator with source location information
 * 
 * This override captures the source file and line number where the array allocation occurs,
 * which is valuable for memory leak detection and debugging. It allows the memory
 * tracking system to report exactly where problematic array allocations originated.
 * 
 * @param size The number of bytes to allocate for the array
 * @param file Source file path where the allocation is requested
 * @param line Line number in the source file where the allocation is requested
 * @return Pointer to the allocated memory block
 * @note - This override is only active when SEDX_TRACK_MEMORY and SEDX_PLATFORM_WINDOWS are defined
 */
_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size)
_VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t size, const char *file, int line) { return Allocator::Allocate(size, file, line); }

/**
 * @brief Global override of the standard operator delete
 * 
 * This function redirects all dynamic memory de-allocations to the SceneryEditorX tracking system.
 * It allows the memory system to update statistics and remove allocation records from tracking.
 * 
 * @param memory Pointer to the memory block to be deallocated
 * @note - This override is only active when SEDX_TRACK_MEMORY and SEDX_PLATFORM_WINDOWS are defined
 */
void __CRTDECL operator delete(void *memory) { return Allocator::Free(memory); }

/**
 * @brief Placement delete operator matching new with category descriptor
 * 
 * This operator is called when an exception is thrown during initialization of an object
 * allocated with the matching placement new operator that accepts a category descriptor.
 * 
 * @param memory Pointer to the memory block to be deallocated
 * @param desc Category descriptor string (unused for deallocation)
 * @note - This override is only active when SEDX_TRACK_MEMORY and SEDX_PLATFORM_WINDOWS are defined
 */
void __CRTDECL operator delete(void *memory, const char *desc) { return Allocator::Free(memory); }

/**
 * @brief Placement delete operator matching new with source location information
 * 
 * This operator is called when an exception is thrown during initialization of an object
 * allocated with the matching placement new operator that accepts file and line information.
 * 
 * @param memory Pointer to the memory block to be deallocated
 * @param file Source file path (unused for deallocation)
 * @param line Line number (unused for deallocation)
 * @note - This override is only active when SEDX_TRACK_MEMORY and SEDX_PLATFORM_WINDOWS are defined
 */
void __CRTDECL operator delete(void *memory, const char *file, int line) { return Allocator::Free(memory); }

/**
 * @brief Global override of the standard operator delete[]
 * 
 * This function redirects all dynamic array de-allocations to the SceneryEditorX tracking system.
 * It allows the memory system to update statistics and remove array allocation records from tracking.
 * 
 * @param memory Pointer to the memory block to be deallocated
 * @note - This override is only active when SEDX_TRACK_MEMORY and SEDX_PLATFORM_WINDOWS are defined
 */
void __CRTDECL operator delete[](void *memory) { return Allocator::Free(memory); }

/**
 * @brief Placement delete[] operator matching new[] with category descriptor
 * 
 * This operator is called when an exception is thrown during initialization of an array
 * allocated with the matching placement new[] operator that accepts a category descriptor.
 * 
 * @param memory Pointer to the memory block to be deallocated
 * @param desc Category descriptor string (unused for deallocation)
 * @note - This override is only active when SEDX_TRACK_MEMORY and SEDX_PLATFORM_WINDOWS are defined
 */
void __CRTDECL operator delete[](void *memory, const char *desc) { return Allocator::Free(memory); }

/**
 * @brief Placement delete[] operator matching new[] with source location information
 * 
 * This operator is called when an exception is thrown during initialization of an array
 * allocated with the matching placement new[] operator that accepts file and line information.
 * 
 * @param memory Pointer to the memory block to be deallocated
 * @param file Source file path (unused for deallocation)
 * @param line Line number (unused for deallocation)
 * @note - This override is only active when SEDX_TRACK_MEMORY and SEDX_PLATFORM_WINDOWS are defined
 */
void __CRTDECL operator delete[](void *memory, const char *file, int line) { return Allocator::Free(memory); }

#endif


// -------------------------------------------------------

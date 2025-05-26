/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* memory.h
* -------------------------------------------------------
* Created: 11/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <map>
#include <mutex>
#include <utility>

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @struct AllocationStats
	 * @brief Statistics about memory allocations.
	 * 
	 * Tracks the total number of bytes allocated and freed by the memory system.
	 * This information can be used for monitoring memory usage and detecting leaks.
	 */
	struct AllocationStats
	{
		/** @brief Total number of bytes allocated since program start */
		size_t TotalAllocated = 0;
		
		/** @brief Total number of bytes freed since program start */
		size_t TotalFreed = 0;
	};


	/**
	 * @struct Allocation
	 * @brief Represents a tracked memory allocation in the system.
	 * 
	 * This structure stores information about a memory allocation made through
	 * the memory tracking system. It tracks the allocated memory address, size,
	 * and a category identifier that can be used for diagnostics and memory profiling.
	 * The Allocation objects are stored in the AllocatorData's AllocationMap to
	 * maintain a record of all active memory allocations.
	 */
    struct Allocation
    {
        /** @brief Pointer to the allocated memory block */
        void *Memory = nullptr;

        /** @brief Size of the allocated memory block in bytes */
        size_t Size = 0;

        /** @brief Category/label for the allocation (useful for debugging and memory profiling) */
        const char *Category = nullptr;
    };

    /// -------------------------------------------------------

	namespace Memory
	{
		/**
		 * @brief Retrieves the current memory allocation statistics.
		 *
		 * Returns a reference to the global allocation statistics that tracks the
		 * total amount of memory allocated and freed by the memory system since
		 * program start. This can be used for monitoring memory usage and detecting
		 * memory leaks in the application.
		 * 
		 * @return A constant reference to the AllocationStats structure containing current allocation data
		 * @see AllocationStats
		 */
		const AllocationStats& GetAllocationStats();

	}

    /// -------------------------------------------------------
	/**
	 * @struct Mallocator
	 * @brief A minimal STL-compatible allocator that uses malloc/free.
	 * 
	 * This allocator provides a minimal implementation that meets C++ STL allocator
	 * requirements while using the standard malloc/free functions for memory management.
	 * It's primarily used internally by the memory tracking system for its own data
	 * structures like AllocationMap to avoid recursion issues that would occur if those
	 * containers used the tracked allocator themselves.
	 * 
	 * @tparam T The type of objects to allocate
	 */
	template <class T>
	struct Mallocator
	{
		/** @brief Type required by STL allocator concept */
		using value_type = T;

		/** @brief Default constructor */
		Mallocator() = default;
		
		/**
		 * @brief Copy constructor from another Mallocator of different type.
		 * @tparam U The type of the other allocator
		 * @param The other allocator to copy from
		 */
		template <class U> constexpr Mallocator(const Mallocator <U>&) noexcept {}

		/**
		 * @brief Allocate memory for n objects of type T.
		 * @param n Number of objects to allocate space for
		 * @return Pointer to the allocated memory
		 * @throws std::bad_array_new_length If allocation size would overflow
		 * @throws std::bad_alloc If allocation fails
		 */
		T* allocate(std::size_t n)
		{
        #undef max
			if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
				throw std::bad_array_new_length();

			if (auto p = static_cast<T*>(std::malloc(n * sizeof(T)))) {
				return p;
			}

			throw std::bad_alloc();
		}

		/**
		 * @brief Deallocate previously allocated memory.
		 * @param p Pointer to the memory to deallocate
		 * @param n Number of objects that were allocated (unused)
		 */
        static void deallocate(T* p, std::size_t n) noexcept {
			std::free(p);
		}
	};


	/**
	 * @struct AllocatorData
	 * @brief Manages memory allocation tracking and statistics.
	 * 
	 * This structure contains the core data structures used for tracking memory allocations
	 * in the memory system. It maintains maps of all active allocations along with their
	 * metadata and provides statistics about memory usage categorized by allocation type.
	 * Thread safety is ensured through mutex locks for both general allocations and statistics.
	 */
	struct AllocatorData
	{
		/** @brief Custom allocator for the allocation map to avoid recursive allocation issues */
		using MapAlloc = Mallocator<std::pair<const void* const, Allocation>>;
		
		/** @brief Custom allocator for the statistics map to avoid recursive allocation issues */
		using StatsMapAlloc = Mallocator<std::pair<const char* const, AllocationStats>>;

		/** @brief Type definition for the map storing memory usage statistics by category */
		using AllocationStatsMap = std::map<const char*, AllocationStats, std::less<const char*>, StatsMapAlloc>;

		/** @brief Map of all currently active memory allocations, indexed by memory address */
		std::map<const void*, Allocation, std::less<const void*>, MapAlloc> AllocationMap;
		
		/** @brief Map of memory usage statistics, categorized by allocation description/type */
		AllocationStatsMap AllocStatsMap;

		/** @brief Mutex for thread-safe access to the allocation map */
		std::mutex Mutex_;
		
		/** @brief Mutex for thread-safe access to the allocation statistics map */
		std::mutex MutexStats_;
	};

	/// -------------------------------------------------------

	/**
	 * @class Allocator
	 * @brief Central memory management system that handles allocation tracking and statistics.
	 * 
	 * The Allocator class provides a static interface for memory allocation operations within 
	 * the SceneryEditorX system. It allows for allocations to be tracked with optional descriptive
	 * information for debugging and profiling purposes. The class maintains internal statistics
	 * about memory usage which can be accessed for memory monitoring and leak detection.
	 * 
	 * All allocations made through this class are recorded in the internal AllocatorData structure,
	 * which keeps track of allocation sizes, categories, and provides thread-safe access to this information.
	 * 
	 * When memory tracking is enabled (SEDX_TRACK_MEMORY), the global new/delete operators are overridden
	 * to use this allocator, allowing comprehensive tracking of all dynamic memory allocations.
	 */
	class Allocator
	{
	public:
		/**
		 * @brief Initializes the memory allocation system.
		 * 
		 * Must be called before any other Allocator function is used.
		 * Creates and initializes the internal AllocatorData structure that tracks allocations.
		 */
		static void Init();

		/**
		 * @brief Performs a raw memory allocation without tracking.
		 * 
		 * Use this function when you need memory but don't want the allocation to be tracked
		 * in the memory statistics system. This is primarily used internally to avoid
		 * recursive tracking issues.
		 * 
		 * @param size The number of bytes to allocate
		 * @return Pointer to the allocated memory block, or nullptr if allocation fails
		 */
		static void* AllocateRaw(size_t size);

		/**
		 * @brief Allocates memory and tracks it in the memory system.
		 * 
		 * @param size The number of bytes to allocate
		 * @return Pointer to the allocated memory block, or nullptr if allocation fails
		 */
		static void* Allocate(size_t size);
		
		/**
		 * @brief Allocates memory with a category descriptor for tracking.
		 * 
		 * The descriptor string is used to categorize the allocation in memory statistics,
		 * which can be useful for debugging memory usage by different subsystems.
		 * 
		 * @param size The number of bytes to allocate
		 * @param desc String descriptor/category for the allocation
		 * @return Pointer to the allocated memory block, or nullptr if allocation fails
		 */
		static void* Allocate(size_t size, const char* desc);
		
		/**
		 * @brief Allocates memory with source file and line information for tracking.
		 * 
		 * This variant is typically used by the overridden new operator to automatically
		 * capture source location information for memory allocations.
		 * 
		 * @param size The number of bytes to allocate
		 * @param file Source file name where the allocation was requested
		 * @param line Line number in source file where the allocation was requested
		 * @return Pointer to the allocated memory block, or nullptr if allocation fails
		 */
		static void* Allocate(size_t size, const char* file, int line);
		
		/**
		 * @brief Deallocates previously allocated memory.
		 * 
		 * Updates memory statistics and removes the allocation from tracking.
		 * 
		 * @param memory Pointer to memory previously allocated with Allocate()
		 */
		static void Free(void* memory);

		/**
		 * @brief Retrieves the allocation statistics categorized by allocation type.
		 * 
		 * @return Constant reference to the map containing allocation statistics for each category
		 */
		static const AllocatorData::AllocationStatsMap& GetAllocationStats() { return Data_->AllocStatsMap; }
	private:
		/**
		 * @brief Pointer to the internal allocation tracking data structure.
		 * 
		 * This is initialized in Init() and stores all allocation information
		 * and statistics throughout the program's execution.
		 */
		inline static AllocatorData* Data_ = nullptr;
	};


} // namespace SceneryEditorX

/// -------------------------------------------------------

#if SEDX_TRACK_MEMORY

#ifdef SEDX_PLATFORM_WINDOWS

/**
 * @brief Global override for operator new that enables memory tracking.
 * 
 * This function intercepts all memory allocations done via the new operator and routes them
 * through the SceneryEditorX memory tracking system. The allocation is recorded with detailed
 * information for later analysis and leak detection.
 * 
 * @param size Size of the memory block to be allocated in bytes
 * @return Pointer to the allocated memory block
 * @throws std::bad_alloc If the memory allocation fails
 * @see Allocator::Allocate
 */
_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t size);

/**
 * @brief Global override for array operator new that enables memory tracking.
 * 
 * Similar to the scalar version, this function intercepts all array allocations and routes them
 * through the SceneryEditorX memory tracking system. The allocation is recorded with information
 * about the array allocation for later analysis.
 * 
 * @param size Size of the memory block to be allocated in bytes
 * @return Pointer to the allocated memory block
 * @throws std::bad_alloc If the memory allocation fails
 * @see Allocator::Allocate
 */
_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t size);

/**
 * @brief Placement operator new with category descriptor for memory tracking.
 * 
 * This version allows the caller to specify a category descriptor for the allocation,
 * which is recorded in the memory tracking system and can be used for categorized
 * memory analysis and reporting.
 * 
 * @param size Size of the memory block to be allocated in bytes
 * @param desc Descriptor/category for the allocation (stored for debugging)
 * @return Pointer to the allocated memory block
 * @throws std::bad_alloc If the memory allocation fails
 * @see Allocator::Allocate
 */
_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t size, const char* desc);

/**
 * @brief Placement array operator new with category descriptor for memory tracking.
 * 
 * Similar to the scalar version with descriptor, this function allows specifying a
 * category for array allocations, which is used for memory tracking and reporting.
 * 
 * @param size Size of the memory block to be allocated in bytes
 * @param desc Descriptor/category for the allocation (stored for debugging)
 * @return Pointer to the allocated memory block
 * @throws std::bad_alloc If the memory allocation fails
 * @see Allocator::Allocate
 */
_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t size, const char* desc);

/**
 * @brief Placement operator new with source location information for memory tracking.
 * 
 * This version captures the source file and line number where the allocation occurs,
 * providing detailed location information for memory tracking and leak identification.
 * This is primarily used by the hnew macro.
 * 
 * @param size Size of the memory block to be allocated in bytes
 * @param file Source file name where the allocation was made
 * @param line Line number in the source file where the allocation was made
 * @return Pointer to the allocated memory block
 * @throws std::bad_alloc If the memory allocation fails
 * @see Allocator::Allocate
 */
_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t size, const char* file, int line);

/**
 * @brief Placement array operator new with source location information.
 * 
 * Similar to the scalar version with source location, this function captures file and line
 * information for array allocations to enhance memory tracking capabilities.
 * 
 * @param size Size of the memory block to be allocated in bytes
 * @param file Source file name where the allocation was made
 * @param line Line number in the source file where the allocation was made
 * @return Pointer to the allocated memory block
 * @throws std::bad_alloc If the memory allocation fails
 * @see Allocator::Allocate
 */
_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t size, const char* file, int line);

/**
 * @brief Global override for operator delete that enables memory tracking.
 * 
 * This function intercepts all deallocations done via the delete operator and routes them
 * through the SceneryEditorX memory tracking system to maintain accurate allocation statistics.
 * 
 * @param memory Pointer to the memory block to be deallocated
 * @see Allocator::Free
 */
void __CRTDECL operator delete(void* memory);

/**
 * @brief Placement operator delete matching the descriptor-based new operator.
 * 
 * This "delete" operator matches the descriptor-based new operator and is called in case
 * of exceptions during the construction phase after memory allocation.
 * 
 * @param memory Pointer to the memory block to be deallocated
 * @param desc Descriptor that was used during allocation (unused for deallocation)
 * @see Allocator::Free
 */
void __CRTDECL operator delete(void* memory, const char* desc);

/**
 * @brief Placement operator delete matching the source location-based new operator.
 * 
 * This "delete" operator matches the file/line-based new operator and is called in case
 * of exceptions during the construction phase after memory allocation.
 * 
 * @param memory Pointer to the memory block to be deallocated
 * @param file Source file name (unused for deallocation)
 * @param line Line number (unused for deallocation)
 * @see Allocator::Free
 */
void __CRTDECL operator delete(void* memory, const char* file, int line);

/**
 * @brief Global override for array operator delete that enables memory tracking.
 * 
 * This function intercepts all array deallocations and routes them through the
 * SceneryEditorX memory tracking system to maintain accurate allocation statistics.
 * 
 * @param memory Pointer to the memory block to be deallocated
 * @see Allocator::Free
 */
void __CRTDECL operator delete[](void* memory);

/**
 * @brief Placement array operator delete matching the descriptor-based new[] operator.
 * 
 * This "delete" operator matches the descriptor-based new[] operator and is called in case
 * of exceptions during the array construction phase after memory allocation.
 * 
 * @param memory Pointer to the memory block to be deallocated
 * @param desc Descriptor that was used during allocation (unused for deallocation)
 * @see Allocator::Free
 */
void __CRTDECL operator delete[](void* memory, const char* desc);

/**
 * @brief Placement array operator delete matching the source location-based new[] operator.
 * 
 * This "delete" operator matches the file/line-based new[] operator and is called in case
 * of exceptions during the array construction phase after memory allocation.
 * 
 * @param memory Pointer to the memory block to be deallocated
 * @param file Source file name (unused for deallocation)
 * @param line Line number (unused for deallocation)
 * @see Allocator::Free
 */
void __CRTDECL operator delete[](void* memory, const char* file, int line);

/**
 * @brief Macro that expands to a new operator with source file and line information.
 * 
 * This macro is used to replace standard 'new' with a version that automatically captures
 * the current source file and line number for enhanced memory tracking.
 */
#define hnew new(__FILE__, __LINE__)

/**
 * @brief Macro that expands to the standard delete operator.
 * 
 * This macro is provided for symmetry with hnew and to allow for potential future
 * enhancements to the delete operation.
 */
#define hdelete delete

#else
/**
 * @brief Warning message for non-Windows platforms where memory tracking is not implemented.
 * 
 * This preprocessor warning is shown when building on platforms where the memory tracking
 * system is not available, falling back to standard memory allocation.
 */
#warning "Memory tracking not available on non-Windows platform"

/**
 * @brief On non-Windows platforms, hnew falls back to standard new.
 */
#define hnew new

/**
 * @brief On non-Windows platforms, hdelete falls back to standard delete.
 */
#define hdelete delete

#endif

#else

/**
 * @brief When memory tracking is disabled, hnew falls back to standard new.
 */
#define hnew new

/**
 * @brief When memory tracking is disabled, hdelete falls back to standard delete.
 */
#define hdelete delete

#endif

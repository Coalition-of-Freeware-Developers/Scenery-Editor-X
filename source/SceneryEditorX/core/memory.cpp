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
#include <mutex>
#include <SceneryEditorX/core/base.hpp>
#include <SceneryEditorX/core/memory.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	LOCAL AllocationStats GlobalStats;
	LOCAL bool InInit_ = false;

    // -------------------------------------------------------

	void Allocator::Init()
	{
	    if (Data_)
        {
            return;
        }
	
	    InInit_ = true;
	    AllocatorData *data = static_cast<AllocatorData *>(AllocateRaw(sizeof(AllocatorData)));
	    new (data) AllocatorData();
	    Data_ = data;
	    InInit_ = false;
	}
	
	void* Allocator::AllocateRaw(const size_t size)
	{
	    return malloc(size);
	}
	
	void* Allocator::Allocate(size_t size)
	{
	    if (InInit_)
        {
            return AllocateRaw(size);
        }
	
	    if (!Data_)
        {
            Init();
        }
	
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
	
	void *Allocator::Allocate(size_t size, const char *desc)
	{
	    if (!Data_)
        {
            Init();
        }
	
	    void *memory = malloc(size);
	    {
	        std::scoped_lock lock(Data_->Mutex_);
            Allocation &alloc = Data_->AllocationMap[memory];
	        alloc.Memory = memory;
	        alloc.Size = size;
	        alloc.Category = desc;
	
	        GlobalStats.TotalAllocated += size;
	        if (desc)
            {
                Data_->AllocStatsMap[desc].TotalAllocated += size;
            }
	    }
	
	#if SEDX_ENABLE_PROFILING
	    TracyAlloc(memory, size);
	#endif
	
	    return memory;
	}
	
	void* Allocator::Allocate(size_t size, const char *file, int line)
	{
	    if (!Data_)
        {
            Init();
        }
	
	    void *memory = malloc(size);
	
	    {
	        std::scoped_lock lock(Data_->Mutex_);
            Allocation &alloc = Data_->AllocationMap[memory];
	        alloc.Memory = memory;
	        alloc.Size = size;
	        alloc.Category = file;
	
	        GlobalStats.TotalAllocated += size;
	        Data_->AllocStatsMap[file].TotalAllocated += size;
	    }
	
	#if SEDX_ENABLE_PROFILING
	    TracyAlloc(memory, size);
	#endif
	
	    return memory;
	}
	
	void Allocator::Free(void *memory)
	{
	    if (memory == nullptr)
        {
            return;
        }
	
	    {
	        bool found = false;
	        {
	            std::scoped_lock lock(Data_->Mutex_);
                auto allocMapIt = Data_->AllocationMap.find(memory);
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
            {
                SEDX_CORE_FATAL_TAG("Memory", "Memory block {0} not present in alloc map", memory);

            }
	#endif
	    }
	
	    free(memory);
	}
	
	namespace Memory
	{
		
		const AllocationStats &GetAllocationStats()
		{
		    return GlobalStats;
		}

	} // namespace Memory

} // namespace SceneryEditorX

#if defined(SEDX_TRACK_MEMORY) && defined(SEDX_PLATFORM_WINDOWS)

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size)
_VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t size) { return Allocator::Allocate(size); }

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size)
_VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t size) {  return Allocator::Allocate(size); }

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size)
_VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t size, const char *desc) { return Allocator::Allocate(size, desc); }

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size)
_VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t size, const char *desc) { return Allocator::Allocate(size, desc); }

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size)
_VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t size, const char *file, int line) { return Allocator::Allocate(size, file, line); }

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size)
_VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t size, const char *file, int line) { return Allocator::Allocate(size, file, line); }

void __CRTDECL operator delete(void *memory) { return Allocator::Free(memory); }
void __CRTDECL operator delete(void *memory, const char *desc) { return Allocator::Free(memory); }
void __CRTDECL operator delete(void *memory, const char *file, int line) { eturn Allocator::Free(memory); }
void __CRTDECL operator delete[](void *memory) { return Allocator::Free(memory); }
void __CRTDECL operator delete[](void *memory, const char *desc) { return Allocator::Free(memory); }
void __CRTDECL operator delete[](void *memory, const char *file, int line) { return Allocator::Free(memory); }

#endif


// -------------------------------------------------------

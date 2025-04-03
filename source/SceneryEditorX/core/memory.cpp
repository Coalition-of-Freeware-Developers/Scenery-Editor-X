/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* memory.cpp
* -------------------------------------------------------
* Created: 2/4/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/core/memory.h>
#include <mutex>

// -------------------------------------------------------

namespace SceneryEditorX
{
	static SceneryEditorX::AllocationStats GlobalStats_;
	static bool InitiProgress_ = false;

	// -------------------------------------------------------

	void Allocator::Init()
	{
		if (AllocData_)
			return;

		InitiProgress_ = true;
		AllocatorData* data = (AllocatorData*)Allocator::AllocateRaw(sizeof(AllocatorData));
		new(data) AllocatorData();
		AllocData_ = data;
		InitiProgress_ = false;
	}

	void* Allocator::AllocateRaw(size_t size)
	{
		return malloc(size);
	}

	void* Allocator::Allocate(size_t size)
	{
		if (InitiProgress_)
			return AllocateRaw(size);

		if (!AllocData_)
			Init();

		void* memory = malloc(size);

		{
			std::scoped_lock<std::mutex> lock(AllocData_->Mutex_);
			Allocation& alloc = AllocData_->AllocMap_[memory];
			alloc.Memory = memory;
			alloc.Size = size;
		
			GlobalStats_.TotalAllocated += size;
		}

#if SEDX_ENABLE_PROFILING
		TracyAlloc(memory, size);
#endif

		return memory;
	}

	void* Allocator::Allocate(size_t size, const char* desc)
	{
		if (!AllocData_)
			Init();

		void* memory = malloc(size);

		{
			std::scoped_lock<std::mutex> lock(AllocData_->Mutex_);
			Allocation& alloc = AllocData_->AllocMap_[memory];
			alloc.Memory = memory;
			alloc.Size = size;
			alloc.Category = desc;

			GlobalStats_.TotalAllocated += size;
			if (desc)
				AllocData_->AllocStatsMap_[desc].TotalAllocated += size;
		}

#if SEDX_ENABLE_PROFILING
		TracyAlloc(memory, size);
#endif

		return memory;
	}

	void* Allocator::Allocate(size_t size, const char* file, int line)
	{
		if (!AllocData_)
			Init();

		void* memory = malloc(size);

		{
			std::scoped_lock<std::mutex> lock(AllocData_->Mutex_);
			Allocation& alloc = AllocData_->AllocMap_[memory];
			alloc.Memory = memory;
			alloc.Size = size;
			alloc.Category = file;

			GlobalStats_.TotalAllocated += size;
			AllocData_->AllocStatsMap_[file].TotalAllocated += size;
		}

#if SEDX_ENABLE_PROFILING
		TracyAlloc(memory, size);
#endif

		return memory;
	}

	void Allocator::Free(void* memory)
	{
		if (memory == nullptr)
			return;

		{
			bool found = false;
			{
				std::scoped_lock<std::mutex> lock(AllocData_->Mutex_);
				auto allocMapIt = AllocData_->AllocMap_.find(memory);
				found = allocMapIt != AllocData_->AllocMap_.end();
				if (found)
				{
					const Allocation& alloc = allocMapIt->second;
					GlobalStats_.TotalFreed += alloc.Size;
					if (alloc.Category)
						AllocData_->AllocStatsMap_[alloc.Category].TotalFreed += alloc.Size;

					AllocData_->AllocMap_.erase(memory);
				}
			}

#if SEDX_ENABLE_PROFILING
			TracyFree(memory);
#endif

#ifndef SEDX_RELEASE
			if (!found)
				EDITOR_LOG_CRITICAL("Memory", "Memory block {0} not present in alloc map", memory);
#endif
		}
		
		free(memory);
	}
	
	namespace Memory {

		const AllocationStats& GetAllocationStats() { return GlobalStats_; }
	}

} // namespace SceneryEditorX

// -------------------------------------------------------

#ifdef SEDX_TRACK_MEMORY

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t size)
{
	return SceneryEditorX::Allocator::Allocate(size);
}

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t size)
{
	return SceneryEditorX::Allocator::Allocate(size);
}

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t size, const char* desc)
{
	return SceneryEditorX::Allocator::Allocate(size, desc);
}

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t size, const char* desc)
{
	return SceneryEditorX::Allocator::Allocate(size, desc);
}

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t size, const char* file, int line)
{
	return SceneryEditorX::Allocator::Allocate(size, file, line);
}

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(size) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t size, const char* file, int line)
{
	return SceneryEditorX::Allocator::Allocate(size, file, line);
}

void __CRTDECL operator delete(void* memory)
{
	return SceneryEditorX::Allocator::Free(memory);
}

void __CRTDECL operator delete(void* memory, const char* desc)
{
	return SceneryEditorX::Allocator::Free(memory);
}

void __CRTDECL operator delete(void* memory, const char* file, int line)
{
	return SceneryEditorX::Allocator::Free(memory);
}

void __CRTDECL operator delete[](void* memory)
{
	return SceneryEditorX::Allocator::Free(memory);
}

void __CRTDECL operator delete[](void* memory, const char* desc)
{
	return SceneryEditorX::Allocator::Free(memory);
}

void __CRTDECL operator delete[](void* memory, const char* file, int line)
{
	return SceneryEditorX::Allocator::Free(memory);
}

#endif

// -------------------------------------------------------

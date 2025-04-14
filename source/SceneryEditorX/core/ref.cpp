/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* ref.cpp
* -------------------------------------------------------
* Created: 10/4/2025
* -------------------------------------------------------
*/

#include <unordered_set>

// -------------------------------------------------------
#define  SEDX_CORE_ASSERT(x)                                                                                           \
    if (!(x))                                                                                                          \
    {                                                                                                                  \
        throw std::runtime_error("Assertion failed: " #x);                                                             \
    }


// -------------------------------------------------------

namespace SceneryEditorX
{
	static std::unordered_set<void*> liveReferences_;
	static std::mutex liveReferenceMutex_;

	// -------------------------------------------------------

	namespace RefUtils {

		void AddToLiveReferences(void* instance)
		{
			std::scoped_lock lock(liveReferenceMutex_);
            SEDX_CORE_ASSERT(instance);
            liveReferences_.insert(instance);
		}

		void RemoveFromLiveReferences(void* instance)
		{
			std::scoped_lock lock(liveReferenceMutex_);
            SEDX_CORE_ASSERT(instance);
            SEDX_CORE_ASSERT(liveReferences_.contains(instance));
			liveReferences_.erase(instance);
		}

		bool IsLive(void* instance)
		{
            SEDX_CORE_ASSERT(instance);
			return liveReferences_.contains(instance);
		}
	}

	// -------------------------------------------------------

} // namespace SceneryEditorX

// -------------------------------------------------------

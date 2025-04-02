/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* scene_ref.cpp
* -------------------------------------------------------
* Created: 2/4/2025
* -------------------------------------------------------
*/

#include <mutex>
#include <unordered_set>

// -------------------------------------------------------

namespace SceneryEditorX
{

	static std::unordered_set<void*> LiveRef_;
	static std::mutex LiveRefMutex_;

	namespace RefUtils
	{
		void AddToLiveReferences(void* instance)
		{
			std::scoped_lock<std::mutex> lock(LiveRefMutex_);
			LiveRef_.insert(instance);
		}

		void RemoveFromLiveReferences(void* instance)
		{
			std::scoped_lock<std::mutex> lock(LiveRefMutex_);
			LiveRef_.erase(instance);
		}

		bool IsLive(void* instance)
		{
			return LiveRef_.find(instance) != LiveRef_.end();
		}
	} // namespace RefUtils


} // namespace SceneryEditorX

// -------------------------------------------------------

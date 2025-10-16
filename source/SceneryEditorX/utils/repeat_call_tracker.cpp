/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* repeat_call_tracker.cpp
* -------------------------------------------------------
* Created: 9/27/2025
* -------------------------------------------------------
*/
#include "repeat_call_tracker.h"
#include "SceneryEditorX/logging/logging.hpp"
#include <mutex>
#include <unordered_set>

namespace SceneryEditorX { namespace Utils {

    static std::mutex s_Mutex;
    static std::unordered_set<std::string> s_Called;

    void RepeatCallTracker::Track(const char* token, const char* file, int line)
    {
        if (!token) return;
        const std::string key = std::string(token);
        bool duplicate = false;
        {
            std::scoped_lock lock(s_Mutex);
            auto [it, inserted] = s_Called.insert(key);
            duplicate = !inserted;
        }

        if (duplicate)
        {
            SEDX_CORE_WARN_TAG("INIT", "Repeat call detected for '{}' at {}:{} — skipping duplicate work if not already guarded.", key, file, line);
        }
    }

}} // namespace

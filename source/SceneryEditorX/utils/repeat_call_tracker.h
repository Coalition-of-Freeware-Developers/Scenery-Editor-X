/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* repeat_call_tracker.h
* -------------------------------------------------------
* Created: 9/27/2025
* -------------------------------------------------------
*/
#pragma once
#include <string>

// -------------------------------------------------------

namespace SceneryEditorX::Utils 
{
    /**
     * @brief Simple thread-safe tracker to detect and log repeated calls to sensitive functions.
     *
     * Use the SEDX_TRACK_CALL(token) macro at the start of init/setup functions. The first call
     * records the token and does nothing further. On subsequent calls during the same process run,
     * a warning with the callsite is emitted to help locate duplicate initialization paths.
     */
    class RepeatCallTracker
    {
    public:
        static void Track(const char* token, const char* file, int line);


    };

}

#define SEDX_TRACK_CALL(token) ::SceneryEditorX::Utils::RepeatCallTracker::Track((token), __FILE__, __LINE__)

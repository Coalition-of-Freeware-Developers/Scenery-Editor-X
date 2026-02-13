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
 * repeat_call_tracker.cpp
 * -------------------------------------------------------
 * Created: 9/27/2025
 * -------------------------------------------------------
 */
#include "repeat_call_tracker.h"
#include "SceneryEditorX/logging/logging.hpp"
#include <mutex>
#include <unordered_set>

//---------------------------------------------------------

namespace SceneryEditorX::Utils 
{

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

}

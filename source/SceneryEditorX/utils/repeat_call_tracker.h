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

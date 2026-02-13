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
 * thread_pool.h
 * -------------------------------------------------------
 * Created: 26/9/2025
 * -------------------------------------------------------
 */
#pragma once
#include <future>
#include <functional>

// -------------------------------------------------------

namespace SceneryEditorX
{
    using Task = std::function<void()>;

    class ThreadPool : public RefCounted
    {
    public:
        ThreadPool() = default;

		static void Init();
		static void Shutdown();

		// Adds a task to the thread pool and returns a future to track its completion
		static std::future<void> Submit(Task&& task);

        // Spread execution of a given function across all available threads
        static void ParallelLoop(std::function<void(uint32_t workIndexStart, uint32_t workIndexEnd)> &&function, const uint32_t workTotal);

        // Wait for all threads to finish work
        static void Flush(bool removeQueued = false);

        // Stats
        static uint32_t GetThreadCount();
        static uint32_t GetWorkingThreadCount();
        static uint32_t GetIdleThreadCount();
        static bool AreTasksRunning();

    };


}

// -------------------------------------------------------

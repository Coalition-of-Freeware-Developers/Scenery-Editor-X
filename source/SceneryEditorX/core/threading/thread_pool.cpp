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
 * thread_pool.cpp
 * -------------------------------------------------------
 * Created: 26/9/2025
 * -------------------------------------------------------
 */
#include "thread_pool.h"
#include <deque>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		// Stats
		static uint32_t threadCount = 0;
		static std::atomic<uint32_t> workingThreadCount = 0;
		
		// Sync objects
		static std::mutex mutex_tasks;
		static std::condition_variable conditionVar;
		
		// Threads
		static std::vector<std::thread> threads;
		
		// Tasks
		static std::deque<Task> tasks;
		
		// Misc
		static bool isStopping = false;
	}

	void ThreadPool::Init()
	{
		while (true)
		{
            Task task;
		    {
				std::unique_lock<std::mutex> lock(mutex_tasks);
				conditionVar.wait(lock, [] { return isStopping || !tasks.empty(); });

				if (isStopping && tasks.empty()) return;

				Task currentTask = std::move(tasks.front());
				tasks.pop_front();
		    }

		    workingThreadCount.fetch_add(1, std::memory_order::memory_order_relaxed);
            try
            {
                task();
            }
            catch (...)
            {
                // Mute exceptions from tasks to avoid crashing the thread pool
            }
            workingThreadCount.fetch_sub(1, std::memory_order::memory_order_relaxed);

		}
	}

    void ThreadPool::Shutdown()
    {
        // ensure queued tasks are flushed and optionally removed by caller
        Flush(true);
        
	    {
			std::unique_lock<std::mutex> lock(mutex_tasks);
            isStopping = true;
        }

        // wake up all threads so they can exit
        conditionVar.notify_all();

        for (auto &t : threads)
        {
            if (t.joinable())
                t.join();
        }

        threads.clear();

        // reset counters
        workingThreadCount.store(0, std::memory_order::memory_order_relaxed);
        threadCount = 0;
    }

    std::future<void> ThreadPool::Submit(Task &&task)
    {
        // Use std::shared_ptr for standard library types that don't inherit from RefCounted
        auto packagedTask = std::make_shared<std::packaged_task<void()>>(std::forward<Task>(task));
        std::future<void> fut = packagedTask->get_future();

        std::unique_lock<std::mutex> lock(mutex_tasks);

        // Wrap the packaged task execution in a simple lambda that will be stored in the deque
        tasks.emplace_back([packagedTask]()
        {
            try
            {
                (*packagedTask)();
            }
            catch (...)
            {
                // rethrow inside packaged_task will be captured by future
                throw;
            }
        });

        // notify one thread that there is work
        conditionVar.notify_one();

        return fut;
    }

    void ThreadPool::ParallelLoop(std::function<void(uint32_t workIndexStart, uint32_t workIndexEnd)> &&function, const uint32_t workTotal)
    {
        SEDX_ASSERT(workTotal > 0, "a parallel loop must have a work_total of at least 1");

        /**
         * If all worker threads are busy or there are no threads, 
         * run the work serially on the calling thread to avoid deadlock
         */
        if (GetWorkingThreadCount() == threadCount || threads.empty())
        {
            function(0, workTotal);
            return;
        }

        // Decide how many workers will be used (at least 1)
        uint32_t workers = std::max(1u, threadCount);

        // Divide the work as evenly as possible among workers
        uint32_t baseWork = workTotal / workers;	// minimum amount of work per worker
        uint32_t remainder = workTotal % workers;	// leftover work distributed one per worker

        // Store futures so we can wait for all tasks to complete
        std::vector<std::future<void>> futures;
        futures.reserve(workers);

        uint32_t workIndex = 0;
        for (uint32_t i = 0; i < workers && workIndex < workTotal; ++i)
        {
            // Each worker gets base_work, and if remainder > 0, give one extra unit of work
            uint32_t workToDo = baseWork + (remainder > 0 ? 1u : 0u);
            if (remainder > 0) --remainder;

            // Define the start and end of this worker's range
            uint32_t start = workIndex;
            uint32_t end = workIndex + workToDo;

            // Enqueue the task into the thread pool
            futures.emplace_back(Submit([fn = function, start, end]() mutable { fn(start, end); }));

            // Move to the next block of work
            workIndex = end;
        }

        // Wait for all worker tasks to finish
        for (auto &f : futures)
        {
            f.get();
        }

    }
   
    void ThreadPool::Flush(bool removeQueued)
    {
        if (removeQueued)
        {
            std::unique_lock<std::mutex> lock(mutex_tasks);
            tasks.clear();
        }

        // Wait until there are no working threads
        while (AreTasksRunning())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

    uint32_t ThreadPool::GetThreadCount() { return threadCount; }
    uint32_t ThreadPool::GetWorkingThreadCount() { return workingThreadCount.load(std::memory_order_relaxed); }
    uint32_t ThreadPool::GetIdleThreadCount() {  return (threadCount > GetWorkingThreadCount()) ? (threadCount - GetWorkingThreadCount()) : 0; }

    bool ThreadPool::AreTasksRunning() { return GetWorkingThreadCount() != 0; }

} // namespace SceneryEditorX

// -------------------------------------------------------

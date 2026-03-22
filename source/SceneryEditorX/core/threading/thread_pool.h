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
#include <functional>
#include <future>

// -------------------------------------------------------

namespace SceneryEditorX
{
	typedef std::function<void()> Task;

	/**
	 * @class ThreadPool
	 * @brief A thread pool for managing and executing tasks concurrently
	 */
	class ThreadPool : public RefCounted
	{
	public:
		ThreadPool() = default;

		/* @brief Initializes the thread pool */
		static void Init();

		/* @brief Shuts down the thread pool */
		static void Shutdown();

		/**
		 * @brief Adds a task to the thread pool and returns a future to track its completion
		 * @param task The task to be executed
		 * @return A future that can be used to wait for the task's completion
		 */
		static std::future<void> Submit(Task&& task);

		/**
		 * @brief Executes a function in parallel across all available threads
		 * @param function The function to be executed in parallel
		 * @param workTotal The total amount of work to be divided among threads
		 */
		static void ParallelLoop(std::function<void(uint32_t workIndexStart, uint32_t workIndexEnd)> &&function, const uint32_t workTotal);

		/**
		 * @brief Waits for all threads to finish their work
		 * @param removeQueued If true, removes all queued tasks before waiting
		 */
		static void Flush(bool removeQueued = false);

		/**
		 * @brief Gets the total number of threads in the thread pool
		 * @return The total number of threads
		 */
		static uint32_t GetThreadCount();

		/**
		 * @brief Gets the number of threads currently working on tasks
		 * @return The number of working threads
		 */
		static uint32_t GetWorkingThreadCount();

		/**
		 * @brief Gets the number of idle threads in the thread pool
		 * @return The number of idle threads
		 */
		static uint32_t GetIdleThreadCount();

		/**
		 * @brief Checks if there are any tasks currently running in the thread pool
		 * @return True if there are tasks running, false otherwise
		 */
		static bool AreTasksRunning();

	};


}

// -------------------------------------------------------

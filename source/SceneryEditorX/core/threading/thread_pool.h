/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
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

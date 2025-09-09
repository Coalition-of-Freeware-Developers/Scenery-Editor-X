/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* render_dispatcher.h
* -------------------------------------------------------
* Created: 27/8/2025
* -------------------------------------------------------
*/
#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include "vulkan/vk_data.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @brief Asynchronous render job dispatcher and deferred GPU resource destruction manager.
	 *
	 * The RenderDispatcher hosts a single background worker thread that executes CPU‑side
	 * render preparation tasks enqueued through Enqueue(). It additionally provides a
	 * frame-delayed resource free ring used to safely destroy GPU resources only after
	 * the GPU has finished using them (multi-frame in-flight protection).
	 *
	 * Design goals:
	 *  - Tiny API surface
	 *  - Deterministic shutdown & leak prevention
	 *  - Inline fallback when uninitialized (simplifies early bootstrap & tests)
	 *  - Minimal locking (separate mutex for active queue vs. deferred free ring)
	 *
	 * Usage pattern:
	 *  1. Call Init() during renderer initialization (before scheduling async work)
	 *  2. Enqueue background jobs with Enqueue()
	 *  3. Schedule destruction of GPU objects with EnqueueResourceFree()
	 *  4. Call NextFrame() once per rendered frame to advance & execute the safe bucket
	 *  5. Optionally Flush() prior to major state transitions (e.g., swapchain rebuild)
	 *  6. Call Shutdown() during renderer teardown (executes any remaining deferred frees)
	 *
	 * Thread safety:
	 *  - Enqueue() & EnqueueResourceFree() are thread-safe
	 *  - Flush() and Shutdown() must not be called concurrently with Shutdown()
	 *  - NextFrame() should be called from the render/main thread only, once per frame
	 *
	 * @note If Init() has not been called, submitted jobs execute immediately on the
	 *       calling thread to avoid ordering hazards during startup/testing.
	 */
	class RenderDispatcher : public RefCounted
	{
	public:
    	/// @brief Job function signature executed by the dispatcher worker.
	    using Job = std::function<void()>;

	    /**
	     * @brief Initialize the dispatcher singleton and spawn the worker thread.
	     *
	     * Determines the frames-in-flight value from renderer data (fallback = 3),
	     * allocates the resource free ring, and starts the background loop. Idempotent.
	     */
	    static void Init();

	    /**
	     * @brief Gracefully stop the worker thread and execute all pending deferred frees.
	     *
	     * Signals the queue to quit, joins the worker, then synchronously runs remaining
	     * resource free buckets to guarantee deterministic teardown.
	     */
	    static void Shutdown();

    	/**
	     * @brief Submit a generic background job.
	     * @param job Callable executed FIFO on the internal worker thread. If the
	     *            dispatcher is not initialized the job executes inline immediately.
	     * @warning Long blocking jobs will starve subsequent jobs; keep tasks short.
	     */
	    static void Enqueue(Job job);                 // generic render-thread work

	    /**
	     * @brief Schedule a job to run only after a safe GPU frame boundary.
	     *
	     * The job is inserted into the bucket corresponding to the frame that will
	     * become safe (currentIndex + framesInFlight - 1). Executed when enough frames
	     * have advanced through NextFrame().
	     * @param job Callable that performs deferred destruction / recycling.
	     * @note Use this for Vulkan object destroys, descriptor pool recycling, etc.
	     */
	    static void EnqueueResourceFree(Job job);     // executes after GPU frame complete (simple ring)

	    /**
	     * @brief Block the calling thread until the active background job queue is empty.
	     * @note Does NOT force execution of deferred resource free buckets.
	     */
	    static void Flush();	// block until queues empty

	    /**
	     * @brief Advance the frames-in-flight ring and execute now-safe resource free jobs.
	     * @param frameIndex (Reserved) Current frame index supplied by renderer; currently
	     *                   unused but kept for future validation / diagnostics.
	     */
	    static void NextFrame(uint32_t frameIndex);   // move resource free ring

	    /**
	     * @brief Query initialization state.
	     * @return True if Init() has been successfully called and not yet shut down.
	     */
	    static bool IsInitialized();

	private:

	    /**
	     * @brief Internal active job queue & synchronization primitives.
	     */
	    struct Queues
	    {
	        std::mutex mtx;                 // Protects jobs / quitting flag
	        std::condition_variable cv;     // Signals worker of new jobs or shutdown
	        std::queue<Job> jobs;           // FIFO of pending background jobs
	        bool quitting = false;          // Set true to terminate worker loop
	    };

	    /**
	     * @brief Per-frame bucket of deferred resource free jobs.
	     */
	    struct RFQueue { std::vector<Job> jobs; };

	    /// @brief Worker thread main loop (blocks on cv until work or shutdown).
	    static void WorkerLoop();

	    static Ref<RenderDispatcher>	s_Instance;             // Lifetime anchor (Ref-counted singleton)
	    static std::thread				s_Worker;               // Background worker thread
	    static Queues					s_Queue;                // Active job queue + sync
	    static std::mutex				s_RFMutex;              // Protects resource free ring operations
        static RenderData				renderData;             // Number of concurrent frames (ring size)
	    static std::vector<RFQueue>		s_ResourceFreeRing;     // Frame-delayed destruction buckets
	    static uint32_t					s_CurrentRFIndex;       // Index of frame bucket most recently completed
	};

}

/// -------------------------------------------------------

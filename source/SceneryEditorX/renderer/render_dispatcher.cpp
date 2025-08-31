/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* render_dispatcher.cpp
* -------------------------------------------------------
* Created: 27/8/2025
* -------------------------------------------------------
*/

/**
 * @file render_dispatcher.cpp
 * @brief Implementation of the RenderDispatcher asynchronous job & deferred resource free system.
 *
 * See `render_dispatcher.h` and `docs/render-dispatcher-documentation.md` for detailed design notes.
 */

#include "render_dispatcher.h"
#include "renderer.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{

	/// Singleton lifetime anchor instance (created in Init, released in Shutdown)
	Ref<RenderDispatcher> RenderDispatcher::s_Instance = nullptr;
	/// Background worker thread executing FIFO jobs
	std::thread RenderDispatcher::s_Worker;
	/// Active job queue + synchronization primitives
	RenderDispatcher::Queues RenderDispatcher::s_Queue;
	/// Mutex protecting the resource free ring structure
	std::mutex RenderDispatcher::s_RFMutex;
	/// Ring of per-frame deferred destruction job buckets
	std::vector<RenderDispatcher::RFQueue> RenderDispatcher::s_ResourceFreeRing;
	/// Index of the frame bucket that just became safe for destruction
	uint32_t RenderDispatcher::s_CurrentRFIndex = 0;

    /// -------------------------------------------------------

	/**
	 * @brief Initialize dispatcher: create singleton, size ring, start worker thread.
	 *
	 * Determines frames-in-flight from renderer data (falls back to 3 when zero / unset).
	 * Idempotent: repeated calls after first successful initialization are ignored.
	 *
	 * Thread-safety: Safe to call from a single-threaded init sequence. Not intended
	 * to race with Shutdown().
	 */
	void RenderDispatcher::Init()
	{
	    if (s_Instance) return; // Already initialized
	    s_Instance = CreateRef<RenderDispatcher>();
	    renderData.framesInFlight = Renderer::GetRenderData().framesInFlight ? Renderer::GetRenderData().framesInFlight : 3;
        s_ResourceFreeRing.resize(renderData.framesInFlight);
	    s_Worker = std::thread(&RenderDispatcher::WorkerLoop);
	}

    /// -------------------------------------------------------

	/**
	 * @brief Gracefully terminate dispatcher and execute all deferred destruction jobs.
	 *
	 * Signals the worker thread, joins it, then executes every remaining resource
	 * free bucket synchronously to guarantee deterministic teardown.
	 * Safe to call if never initialized (no-op).
	 */
	void RenderDispatcher::Shutdown()
	{
	    if (!s_Instance) return;
	    {
	        std::unique_lock<std::mutex> lock(s_Queue.mtx);
	        s_Queue.quitting = true;
	        s_Queue.cv.notify_all();
	    }
	    if (s_Worker.joinable()) s_Worker.join();

	    // Execute any remaining resource free jobs (all buckets)
	    for (auto &bucket : s_ResourceFreeRing)
		{
	        for (auto &job : bucket.jobs) job();
	        bucket.jobs.clear();
	    }
	    s_Instance.Reset();
	}

    /// -------------------------------------------------------

	/**
	 * @return True if the dispatcher singleton exists (Init has been called and not yet shut down).
	 */
	bool RenderDispatcher::IsInitialized() { return s_Instance != nullptr; }

    /// -------------------------------------------------------

	/**
	 * @brief Submit a generic background job.
	 *
	 * If uninitialized, executes the job immediately inline (simplifies early
	 * startup & unit tests). Otherwise, enqueues into the FIFO processed by the
	 * worker thread.
	 * @param job Callable to execute.
	 */
	void RenderDispatcher::Enqueue(Job job)
	{
	    if (!s_Instance) { job(); return; }
	    {
	        std::lock_guard<std::mutex> lock(s_Queue.mtx);
	        s_Queue.jobs.emplace(std::move(job));
	    }
	    s_Queue.cv.notify_one();
	}

    /// -------------------------------------------------------

	/**
	 * @brief Schedule a deferred destruction job for execution after a safe GPU frame boundary.
	 *
	 * Inserts the job into the bucket that will become safe after cycling through
	 * all frames in flight. If uninitialized, executes immediately.
	 * @param job Callable performing resource release (e.g., Vulkan object destroy).
	 */
	void RenderDispatcher::EnqueueResourceFree(Job job)
	{
	    if (!s_Instance) { job(); return; }
	    std::lock_guard<std::mutex> lock(s_RFMutex);
	    s_ResourceFreeRing[(s_CurrentRFIndex + renderData.framesInFlight - 1) % renderData.framesInFlight].jobs.emplace_back(std::move(job));
	}

    /// -------------------------------------------------------

	/**
	 * @brief Advance frame ring and execute resource free jobs for the now-safe bucket.
	 *
	 * Should be called exactly once per rendered frame (after GPU submission of the
	 * previous frame), typically at the end of a frame just before recording the next.
	 * @param frameIndex (Reserved for future validation) - currently unused.
	 */
	void RenderDispatcher::NextFrame(uint32_t frameIndex)
	{
	    (void)frameIndex; // not used yet
	    std::vector<Job> toExecute;
	    {
	        std::lock_guard<std::mutex> lock(s_RFMutex);
            s_CurrentRFIndex = (s_CurrentRFIndex + 1) % renderData.framesInFlight;
	        toExecute.swap(s_ResourceFreeRing[s_CurrentRFIndex].jobs);
	    }
	    for (auto &job : toExecute) job();
	}

    /// -------------------------------------------------------

	/**
	 * @brief Block until the active background job queue is empty.
	 *
	 * Does not execute or wait on deferred resource free buckets; those are processed
	 * via NextFrame() or during Shutdown().
	 */
	void RenderDispatcher::Flush()
	{
	    if (!s_Instance) return;
	    std::unique_lock<std::mutex> lock(s_Queue.mtx);
	    s_Queue.cv.wait(lock, [](){ return s_Queue.jobs.empty(); });
    }

    /// -------------------------------------------------------

	/**
	 * @brief Internal worker thread main loop.
	 *
	 * Waits on condition variable until a job arrives or a quit signal is issued,
	 * executes jobs FIFO, catching and logging any exceptions so a single failing
	 * job does not terminate the thread.
	 */
	void RenderDispatcher::WorkerLoop()
	{
	    while (true)
		{
	        Job job;
	        {
	            std::unique_lock<std::mutex> lock(s_Queue.mtx);
	            s_Queue.cv.wait(lock, [](){ return s_Queue.quitting || !s_Queue.jobs.empty(); });
	            if (s_Queue.quitting && s_Queue.jobs.empty()) break;
	            job = std::move(s_Queue.jobs.front());
	            s_Queue.jobs.pop();
	        }
	        try
	        {
	            job();
	        }
	        catch (const std::exception &e)
	        {
	            SEDX_CORE_ERROR_TAG("FRAME DISPATCHER","Render job exception: {}", e.what());
	        }
	    }
	}

}

/// -------------------------------------------------------

# Scenery Editor X - RenderDispatcher System Documentation

---

## Overview

The `RenderDispatcher` is a lightweight asynchronous job dispatch and deferred resource destruction system dedicated to the rendering subsystem. It provides:

- A single background worker thread for CPU‑side render preparation tasks
- A frame‑delayed (frames-in-flight) resource free ring to safely destroy GPU resources only after the GPU has finished using them
- Simple, exception‑safe job submission API (`Enqueue`, `EnqueueResourceFree`)
- Deterministic shutdown semantics and inline fallback behavior when not initialized

This system helps decouple transient or auxiliary rendering work (state updates, buffer uploads preparation, material baking steps, CPU side culling, etc.) from the main thread while ensuring that resource destruction never races with in‑flight GPU command buffers.

## Class Documentation

### Class Structure

```cpp
class RenderDispatcher : public RefCounted
{
public:
    using Job = std::function<void()>;

    // Lifecycle
    static void Init();
    static void Shutdown();

    // Job Submission
    static void Enqueue(Job job);              // Generic background (worker thread) job
    static void EnqueueResourceFree(Job job);  // Deferred execution after a safe GPU frame boundary

    // Synchronization
    static void Flush();                       // Blocks until job queue is empty (does NOT flush resource free ring)
    static void NextFrame(uint32_t frameIndex);// Advance frame + execute resource free bucket that became safe

    static bool IsInitialized();
private:
    struct Queues {
        std::mutex mtx;
        std::condition_variable cv;
        std::queue<Job> jobs;   // FIFO job queue processed by worker thread
        bool quitting = false;  // Signal for graceful termination
    };

    struct RFQueue { std::vector<Job> jobs; };  // Per-frame resource free bucket

    static void WorkerLoop();                  // Background processing loop

    static Ref<RenderDispatcher> s_Instance;   // Lifetime anchor (Ref-counted)
    static std::thread          s_Worker;      // Background worker thread
    static Queues               s_Queue;       // Active job queue & sync primitives
    static std::mutex           s_RFMutex;     // Protects resource free ring
    static uint32_t             s_FramesInFlight;      // Size of resource free ring
    static std::vector<RFQueue> s_ResourceFreeRing;    // Ring of deferred free job buckets
    static uint32_t             s_CurrentRFIndex;      // Index of frame bucket just completed (ready to execute)
};
```

## Key Features

### 1. Single Background Worker

Offloads non‑time‑critical render preparation tasks while keeping the API intentionally simple (no work stealing, no priority tiers). This minimizes synchronization overhead and simplifies debugging.

### 2. Frame-Delayed Resource Free Ring

Ensures GPU resources (buffers, images, descriptor pools, etc.) are destroyed only after the GPU has finished referencing them. Jobs enqueued via `EnqueueResourceFree()` are placed into the bucket associated with the frame that will become safe after a full frames‑in‑flight cycle.

### 3. Inline Fallback Before Init / After Shutdown

If the dispatcher is not initialized, submitted jobs execute immediately on the calling thread. This removes ordering fragility during early engine bootstrap or unit testing scenarios.

### 4. Exception Safety

Each job is wrapped in a `try/catch` block inside `WorkerLoop()`. Exceptions are logged with a tagged error (`DISPATCH`) and do not terminate the worker thread.

### 5. Deterministic Shutdown

During `Shutdown()` the job queue is drained, the worker joined, and all remaining deferred resource free buckets are executed synchronously to prevent leaks.

### 6. Minimal Lock Contention

Distinct mutexes are used for the active job FIFO and the resource free ring. Most frames only acquire the RF mutex briefly in `NextFrame()`.

## Architecture Diagram

```text
                  +-------------------------+
                  |  Main / Render Thread  |
                  +-----------+-------------+
                              | Enqueue(Job)
                              v
                    +-------------------+        Worker Thread
   (mtx / cv)  ---> |   s_Queue.jobs    |  ---->  while (!quit) { wait + pop + run }
                    +-------------------+
                              |
                  EnqueueResourceFree(Job)
                              |
                              v  (ring indexed by frames-in-flight)
        +-----------+-----------+-----------+-----------+
        |  Bucket0  |  Bucket1  |  Bucket2  |  BucketN  |  ...
        +-----------+-----------+-----------+-----------+
                 ^
                 | (NextFrame rotates & executes newly safe bucket)
```

## Detailed Method Documentation

### `static void Init()`

Initializes singleton instance, derives `framesInFlight` from renderer data (falls back to 3), sizes the resource free ring, and launches the worker thread. Safe to call multiple times (idempotent). Must be called before multithreaded rendering tasks are scheduled.

### `static void Shutdown()`

Signals the worker to quit, joins the thread, executes all remaining deferred resource free jobs synchronously, and resets the singleton. Safe to call even if never initialized.

### `static void Enqueue(Job job)`

Adds a generic job to the background queue. If uninitialized, runs inline. Jobs are FIFO and executed serially on the worker thread. Use for CPU‑only preparation tasks (material metadata baking, staging buffer fills, light clustering precomputation, etc.). Avoid long blocking operations (disk I/O should usually be async elsewhere).

### `static void EnqueueResourceFree(Job job)`

Schedules a job to run after a safe GPU boundary—typically used for destroying Vulkan objects, releasing staging allocations, or recycling descriptor sets. The job is inserted into the previous frame's bucket `(currentIndex + framesInFlight - 1) % framesInFlight`, meaning it will execute after enough `NextFrame()` calls have advanced the ring to make that frame safe.

### `static void NextFrame(uint32_t frameIndex)`

Advances the resource free ring once per rendered frame (frame index currently unused but reserved for future validation / debugging). The bucket at the *new* current index is swapped out and its jobs executed immediately on the calling thread (usually the render thread) before continuing with the new frame’s logic.

### `static void Flush()`

Blocks until the active job queue is empty. It does not force execution of pending resource free jobs; those only run through `NextFrame()` or at shutdown. Use sparingly (e.g., before resizing swapchain or tearing down subsystems that depend on background-prepared artifacts).

### `static bool IsInitialized()`

Utility query to allow conditional scheduling paths in initialization sequences or test harnesses.

## Usage Examples

### Example 1: Engine Initialization and Shutdown

```cpp
void Engine::Initialize()
{
    RenderDispatcher::Init();
    // Continue with renderer, swapchain, and module initialization...
}

void Engine::Shutdown()
{
    // Ensure all frames processed
    RenderDispatcher::Flush();
    RenderDispatcher::Shutdown();
}
```

### Example 2: Scheduling Background Preparation Work

```cpp
void MaterialSystem::BuildMaterialAsync(const Ref<MaterialAsset>& asset)
{
    RenderDispatcher::Enqueue([asset](){
        asset->BakeCPUData();        // CPU heavy transformation
        asset->GenerateLookupTables();
    });
}
```

### Example 3: Safe GPU Resource Destruction

```cpp
void BufferCache::DestroyLater(VkBuffer buffer, VmaAllocation alloc)
{
    RenderDispatcher::EnqueueResourceFree([device = m_Device, buffer, alloc](){
        vmaDestroyBuffer(device->GetAllocator(), buffer, alloc);
    });
}
```

### Example 4: Per-Frame Advancement

```cpp
void Renderer::EndFrame(uint32_t frameIndex)
{
    // Submit command buffers, present, etc.
    // After GPU submission of frame N and before recording frame N+1:
    RenderDispatcher::NextFrame(frameIndex);
}
```

### Example 5: Inline Fallback (Unit Test Scenario)

```cpp
TEST_CASE("RenderDispatcher inline fallback")
{
    // Not calling Init() on purpose
    std::atomic<int> counter = 0;
    RenderDispatcher::Enqueue([&](){ ++counter; }); // Executes immediately
    REQUIRE(counter == 1);
}
```

## Best Practices

1. Keep jobs short & non‑blocking—avoid long sleeps or large synchronous I/O.
2. Capture engine objects with `Ref<T>`; avoid raw owning pointers in lambdas.
3. Use `EnqueueResourceFree` for any destruction touching GPU resources (buffers, images, pipelines) unless you have explicit GPU idle synchronization.
4. Call `NextFrame()` exactly once per rendered frame (after ensuring GPU submission order) to prevent deferred destruction buildup.
5. Use tagged logging (`SEDX_CORE_TRACE_TAG("DISPATCH", ...)`) for debugging heavy dispatch loads.

## Performance Considerations

- Single-threaded execution model intentionally limits contention; if profiling shows the queue becoming a bottleneck, consider splitting categories (e.g., I/O vs. CPU transform) in a future extension.
- Avoid large captures—prefer passing lightweight identifiers or handles.
- Deferred destruction ring prevents stalls compared to forcing `vkDeviceWaitIdle()` before each free.

## Thread Safety

- Job queue protected by `s_Queue.mtx` + condition variable.
- Resource free ring protected by `s_RFMutex`—held only briefly during bucket swap or insertion.
- Jobs themselves run without additional locking—internal code must ensure its own synchronization when touching shared state.

## Error Handling & Logging

- Exceptions inside jobs are caught and logged via `SEDX_CORE_ERROR_TAG("DISPATCH", ...)`.
- Initialization is idempotent; misuse (double init / shutdown) is harmless but should be avoided in high-level architecture.
- `Flush()` waits only for the active job queue (not resource free ring). If you must guarantee all destruction executed, call `NextFrame()` enough times (equal to `framesInFlight`) or let `Shutdown()` finalize.

## Debugging Tips

- Add temporary verbose traces around large batch submissions to detect frame hitches.
- Instrument job durations by wrapping lambdas:

```cpp
RenderDispatcher::Enqueue([start = Time::Now()](){
    SEDX_PROFILE_SCOPE("LightClusterBuild");
    BuildClusters();
    SEDX_CORE_TRACE_TAG("DISPATCH", "Light clustering took {} ms", (Time::Now() - start).Milliseconds());
});
```

- If resources appear to leak, verify `NextFrame()` is being called consistently.

## Integration Pattern: Renderer Lifecycle

```cpp
class Renderer
{
public:
    void Initialize()
    {
        // Init dispatcher before creating GPU resources that may be destroyed asynchronously
        RenderDispatcher::Init();
        CreateSwapchain();
        CreateFrameResources();
    }

    void RenderLoop()
    {
        while (!m_ShouldExit)
        {
            const uint32_t frame = AcquireFrameIndex();
            RecordFrame(frame);
            SubmitFrame(frame);
            // Advance dispatch ring (frees resources from frame that became safe)
            RenderDispatcher::NextFrame(frame);
        }
    }

    void Shutdown()
    {
        // Flush CPU jobs before tearing down renderer state
        RenderDispatcher::Flush();
        RenderDispatcher::Shutdown();
    }
};
```

## Common Pitfalls

| Pitfall | Consequence | Resolution |
|---------|-------------|-----------|
| Forgetting to call `NextFrame()` | Deferred free ring never executes → memory growth | Call once per frame after GPU submit/present |
| Using `Flush()` expecting resource destruction | Resource free jobs remain pending | Advance frames or call `Shutdown()` |
| Long blocking job (e.g., disk I/O) in queue | Starves subsequent jobs | Offload I/O elsewhere or pipeline smaller steps |
| Capturing raw pointer to short‑lived object | Dangling access in worker thread | Capture `Ref<T>` or validate weak refs |
| Destroying Vulkan objects immediately instead of `EnqueueResourceFree` | Possible use-after-free on GPU | Use deferred destruction ring |

## Extension Ideas (Future Work)

- Priority queues (e.g., high vs. low latency tasks)
- Instrumented profiling per job type / category
- Multiple worker threads with work stealing if CPU load demands it
- Debug UI panel listing queued and deferred jobs

## Summary

The `RenderDispatcher` provides a focused, low-overhead solution for asynchronous render preparation and safe deferred GPU resource destruction. Its deliberate simplicity reduces synchronization complexity while still addressing the core lifetime hazards of multi-frame GPU pipelines. Proper integration (initialization, per-frame advancement, and shutdown) ensures predictable behavior and resource hygiene across the renderer lifecycle.

---

> For related systems see: `module-stage-documentation.md` (execution ordering) and `memory-system-documentation.md` (smart pointer ownership patterns).

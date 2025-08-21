# Renderer Command Submission & Resource Freeing System

> Scenery Editor X – Vulkan Rendering Subsystem Internal Documentation
> Focus: `Renderer::Submit` and `Renderer::SubmitResourceFree` templates + supporting architecture

---

## Table of Contents

1. Purpose & Scope
2. Architectural Context
3. Command Queue Model
4. Core Templates
   - 4.1 `Renderer::Submit`
   - 4.2 `Renderer::SubmitResourceFree`
5. Memory & Lifetime Semantics
6. Threading & Ordering Guarantees
7. Choosing the Right API (Decision Matrix)
8. Usage Patterns & Code Examples
9. Capture Guidelines & Anti‑Patterns
10. GPU Resource Destruction Strategy
11. Frame Timeline & Queues (ASCII Diagrams)
12. Error Handling, Safety, and Assertions
13. Performance Considerations & Micro‑Optimizations
14. Testing & Validation Strategies
15. Extension Ideas / Future Enhancements
16. FAQ / Troubleshooting
17. Doxygen Blocks (Copy‑Ready)
18. Quick Reference Cheat Sheet

---

## 1. Purpose & Scope

This document explains how deferred work and deferred **GPU-safe resource destruction** are scheduled inside the Scenery Editor X Vulkan renderer through two key template functions:

- `Renderer::Submit(FuncT&& func)` – enqueue general render-thread work.
- `Renderer::SubmitResourceFree(FuncT&& func)` – enqueue destruction logic that must run only when the GPU has finished referencing the resource.

It also clarifies memory layout, ordering guarantees, best practices for lambda captures, and integration with multi-frame-in-flight synchronization.

---

## 2. Architectural Context

The renderer decouples **producer threads** (game logic, asset streaming, editor UI) from the **render thread** via lightweight linear command queues. Each queue entry embeds:

1. A small header pointing to a trampoline function (`RenderCommandFn`).
2. Placement‑new constructed callable (lambda/functor) blob of size `sizeof(FuncT)`.

Execution drains the queue on the render thread, invoking each trampoline which casts, calls, and explicitly destroys the callable.

Additionally, the renderer maintains per-frame **resource release queues** to safely destroy Vulkan objects after fence completion.

---

## 3. Command Queue Model

| Aspect               | Detail                                                                                           |
| -------------------- | ------------------------------------------------------------------------------------------------ |
| Allocation           | Bump/linear within a frame-local buffer (no per-item heap alloc).                                |
| Submission           | Lock-free / minimal synchronization (atomic index or external fencing; implementation-specific). |
| Drain Point          | At render thread stage: frame build, end-of-frame, or explicit flush.                            |
| Destruction          | Callable destructor executed immediately after invocation (explicit).                            |
| Resource Free Queues | One per frame-in-flight (e.g., triple-buffered) – drained when safe.                            |

The **explicit destructor** enables non-trivial captures (e.g., `std::string`, `Ref<T>`, STL containers) without imposing a trivially-destructible contract.

---

## 4. Core Templates

### 4.1 `Renderer::Submit`

General-purpose deferred execution on the render thread. Use for:

- State changes
- Recording command buffers
- Pipeline/material invalidation
- Upload staging finalization

### 4.2 `Renderer::SubmitResourceFree`

Specialized for GPU resource destruction. Ensures code runs in a *safe frame context* where prior submissions using the resource have completed. If called off the render thread, it re-routes through `Submit` to maintain consistency.

---

## 5. Memory & Lifetime Semantics

| Element          | Behavior                                                         |
| ---------------- | ---------------------------------------------------------------- |
| Callable Storage | Raw bytes from queue's linear buffer.                            |
| Construction     | Placement-new with perfect forwarding of `func`.               |
| Invocation       | Trampoline casts to original type and executes `operator()`.   |
| Destruction      | Manual destructor call immediately after execution.              |
| Ownership        | The queue exclusively owns the lifecycle of the stored callable. |

**Implication:** Do *not* capture references to stack objects unless guaranteed to outlive execution. Prefer capturing by value or using owning smart handles (`Ref<T>`).

---

## 6. Threading & Ordering Guarantees

| Guarantee                          | Explanation                                                                 |
| ---------------------------------- | --------------------------------------------------------------------------- |
| FIFO within a single queue         | Submissions drain in order of allocation.                                   |
| Cross-thread safety                | Multiple producers may call `Submit` concurrently.                        |
| Intra-frame resource free ordering | `SubmitResourceFree` commands for a frame preserve their relative order.  |
| Inter-frame release ordering       | Older frames’ release queues are drained before newer ones (after fences). |
| Non-preemption                     | No command runs before its predecessors in the same queue.                  |

---

## 7. Choosing the Right API (Decision Matrix)

| Scenario                                               | Use                    | Rationale                                    |
| ------------------------------------------------------ | ---------------------- | -------------------------------------------- |
| Record render work                                     | `Submit`             | Normal deferred logic.                       |
| CPU-side bookkeeping (stats, counters)                 | `Submit`             | Must execute on render thread for coherence. |
| Destroy VkImage / VkBuffer / VkSampler                 | `SubmitResourceFree` | Must wait for GPU usage to finish.           |
| Free staging buffer after copy completes*this* frame | `SubmitResourceFree` | Ensures fence safety.                        |
| Rebuild pipeline after shader hot reload               | `Submit`             | Not a destruction hazard.                    |
| Batched destruction of many small handles              | `SubmitResourceFree` | Minimizes queue overhead.                    |

---

## 8. Usage Patterns & Code Examples

### 8.1 Basic Work Submission

```cpp
Renderer::Submit([mesh = meshRef, material = matRef]() {
    mesh->RecordDraw(material);
});
```

### 8.2 Upload Then Free Staging

```cpp
auto staging = CreateStagingBuffer(bytes);
Renderer::Submit([staging, gpuBuf = gpuBuffer]() {
    CopyToGPU(staging, gpuBuf);
});
Renderer::SubmitResourceFree([staging]() {
    DestroyBuffer(staging);
});
```

### 8.3 Vulkan Handle Destruction

```cpp
Renderer::SubmitResourceFree([
    dev = device,
    img = image,
    view = imageView,
    alloc = allocation
]() {
    if (view) vkDestroyImageView(dev, view, nullptr);
    if (img)  vkDestroyImage(dev, img, nullptr);
    vmaFreeMemory(VmaAllocatorFromDevice(dev), alloc);
});
```

### 8.4 Batched Descriptor Pool Release

```cpp
Renderer::SubmitResourceFree([
    dev = device,
    pools = std::move(descriptorPools)
]() mutable {
    for (VkDescriptorPool p : pools)
        vkDestroyDescriptorPool(dev, p, nullptr);
});
```

### 8.5 Shader Pipeline Invalidation

```cpp
Renderer::Submit([pl = pipeline]() {
    pl->Invalidate();
    pl->Recreate();
});
```

---

## 9. Capture Guidelines & Anti‑Patterns

---

## 6. Threading & Ordering Guarantees

| Guarantee                          | Explanation                                                                 |
| ---------------------------------- | --------------------------------------------------------------------------- |
| FIFO within a single queue         | Submissions drain in order of allocation.                                   |
| Cross-thread safety                | Multiple producers may call `Submit` concurrently.                        |
| Intra-frame resource free ordering | `SubmitResourceFree` commands for a frame preserve their relative order.  |
| Inter-frame release ordering       | Older frames’ release queues are drained before newer ones (after fences). |
| Non-preemption                     | No command runs before its predecessors in the same queue.                  |

---

## 7. Choosing the Right API (Decision Matrix)

| Scenario                                               | Use                    | Rationale                                    |
| ------------------------------------------------------ | ---------------------- | -------------------------------------------- |
| Record render work                                     | `Submit`             | Normal deferred logic.                       |
| CPU-side bookkeeping (stats, counters)                 | `Submit`             | Must execute on render thread for coherence. |
| Destroy VkImage / VkBuffer / VkSampler                 | `SubmitResourceFree` | Must wait for GPU usage to finish.           |
| Free staging buffer after copy completes*this* frame | `SubmitResourceFree` | Ensures fence safety.                        |
| Rebuild pipeline after shader hot reload               | `Submit`             | Not a destruction hazard.                    |
| Batched destruction of many small handles              | `SubmitResourceFree` | Minimizes queue overhead.                    |

---

## 8. Usage Patterns & Code Examples

### 8.1 Basic Work Submission

```cpp
Renderer::Submit([mesh = meshRef, material = matRef]() {
    mesh->RecordDraw(material);
});
```

### 8.2 Upload Then Free Staging

```cpp
auto staging = CreateStagingBuffer(bytes);
Renderer::Submit([staging, gpuBuf = gpuBuffer]() {
    CopyToGPU(staging, gpuBuf);
});
Renderer::SubmitResourceFree([staging]() {
    DestroyBuffer(staging);
});
```

### 8.3 Vulkan Handle Destruction

```cpp
Renderer::SubmitResourceFree([
    dev = device,
    img = image,
    view = imageView,
    alloc = allocation
]() {
    if (view) vkDestroyImageView(dev, view, nullptr);
    if (img)  vkDestroyImage(dev, img, nullptr);
    vmaFreeMemory(VmaAllocatorFromDevice(dev), alloc);
});
```

### 8.4 Batched Descriptor Pool Release

```cpp
Renderer::SubmitResourceFree([
    dev = device,
    pools = std::move(descriptorPools)
]() mutable {
    for (VkDescriptorPool p : pools)
        vkDestroyDescriptorPool(dev, p, nullptr);
});
```

### 8.5 Shader Pipeline Invalidation

```cpp
Renderer::Submit([pl = pipeline]() {
    pl->Invalidate();
    pl->Recreate();
});
```

---

## 9. Capture Guidelines & Anti‑Patterns

| Practice                  | Recommendation                                             |
| ------------------------- | ---------------------------------------------------------- |
| Large containers          | Move into lambda `[data = std::move(vec)]` then consume. |
| Raw pointers              | Capture strong `Ref<T>` or ensure lifetime externally.   |
| References                | Avoid unless static or globally owned.                     |
| Exceptions                | Lambdas must be effectively `noexcept`.                  |
| Heavy per-item submission | Batch where possible.                                      |

**Anti‑Pattern (dangling ref):**

```cpp
void Foo() {
    VkBuffer local = CreateTemp();
    Renderer::Submit([&]() { Use(local); }); // UB: local may be gone
}
```

**Correct:**

```cpp
void Foo() {
    VkBuffer local = CreateTemp();
    Renderer::Submit([buf = local]() { Use(buf); });
}
```

---

## 10. GPU Resource Destruction Strategy

1. **Acquire** resource (image, buffer, sampler, pipeline).
2. **Use** across N frames.
3. **Schedule destruction** via `SubmitResourceFree` when higher-level object finalizes.
4. **Drain logic** executes only after frame fence ensures GPU completion.
5. **Manual destructor of lambda** releases captured owning wrappers (e.g. `Ref<T>`).

This prevents premature device calls that could trigger validation errors or undefined behavior.

---

## 11. Frame Timeline & Queues

### 11.1 General Command Flow

```
(Game/UI Threads)                   (Render Thread)
   Submit(funcA)  ──────┐        Drain Frame N Queue
   Submit(funcB)  ───┐  │        ├─ invoke funcA; destroy
                      │  │        ├─ invoke funcB; destroy
SubmitResourceFree(F)  │  │        └─ Build + Present
                        ▼  │
                [Frame N Command Queue]
```

### 11.2 Resource Free Flow (Triple Buffer Example)

```
Frame Indices:   0        1        2        0        1 ...
Submission:      +RF0     +RF1     +RF2     +RF0     +RF1
Drain (safe):    drainQ0  drainQ1  drainQ2  drainQ0  drainQ1
Fence Order:     F0       F1       F2       F0'      F1'
```

`+RFx` = enqueue into release queue for frame x. Draining occurs only when fence for that frame signals.

---

## 12. Error Handling, Safety, and Assertions

| Concern                | Mitigation                                       |
| ---------------------- | ------------------------------------------------ |
| Destruction order risk | Use `SubmitResourceFree` only for GPU handles. |
| Exception in callable  | Forbid throwing; log and early return instead.   |
| Oversized capture      | Refactor to ID + lookup on render thread.        |
| Double scheduling free | Null out / mark handles after scheduling.        |

Potential (optional) debug assertions:

```cpp
static_assert(!std::is_reference_v<FuncT>, "FuncT cannot be a reference");
```

---

## 13. Performance Considerations & Micro‑Optimizations

| Technique                                          | Benefit                                    |
| -------------------------------------------------- | ------------------------------------------ |
| Batch frees                                        | Fewer queue entries & cache misses.        |
| Avoid copying big captures                         | Reduces memcpy cost inside Allocate.       |
| Trivial destruction fast path (future)             | Skip destructor if trivially destructible. |
| Pre-reserve vector resources in higher-level code  | Less reallocation before capture move.     |
| Reuse RAII wrappers that auto-schedule destruction | Less manual lambda code.                   |

**Measurement Hooks:** Add counters for: total bytes allocated, #commands submitted this frame, #resource frees pending.

---

## 14. Testing & Validation Strategies

| Test Type                  | Focus                                                       |
| -------------------------- | ----------------------------------------------------------- |
| Unit: ordering             | Enqueue N commands, record side effect order.               |
| Unit: capture lifetimes    | Ensure non-trivial destructor logs run.                     |
| Integration: resource free | Create + schedule destruction; verify no validation errors. |
| Stress: high frequency     | Thousands of small lambdas, ensure no leaks / asserts.      |
| Fences: frame rollover     | Cycle frames > frames-in-flight, confirm delayed frees.     |

Pseudo test (Catch2 style):

```cpp
TEST_CASE("Submit ordering") {
    std::vector<int> order;
    Renderer::Submit([&]{ order.push_back(1); });
    Renderer::Submit([&]{ order.push_back(2); });
    DrainRenderThreadOnce();
    REQUIRE(order == std::vector<int>{1,2});
}
```

---

## 15. Extension Ideas / Future Enhancements

| Idea                         | Description                                                       |
| ---------------------------- | ----------------------------------------------------------------- |
| `SubmitTrivial` variant    | Skips explicit destructor when `is_trivially_destructible_v`.   |
| Debug tagging                | Allow labeling commands for profiling (store a const char* tag).  |
| Stats overlay                | Real-time ImGui display of queue depth & bytes.                   |
| RAII Vulkan handle wrapper   | Calls `SubmitResourceFree` automatically in destructor.         |
| Validation layer integration | Assert if resource free invoked on wrong thread without deferral. |
| Command coalescing           | Merge adjacent destruction lambdas for same object type.          |

---

## 16. FAQ / Troubleshooting

**Q:** I see validation errors about destroying an in-use image.
**A:** Ensure destruction uses `SubmitResourceFree` *and* the frame index logic is correctly synchronized with GPU fences.

**Q:** My lambda destructor never runs.
**A:** Confirm queues are actually drained during the frame loop / shutdown sequence.

**Q:** Performance regression after adding many lambdas.
**A:** Batch small tasks or encode them into a single function that loops.

**Q:** Can I store a future/promise in the lambda?
**A:** Yes, but ensure thread-safe signaling and avoid blocking the render thread.

---

## 17. Doxygen Blocks (Copy‑Ready)

### 17.1 `Renderer::Submit`

```cpp
/**
 * @brief Enqueue an arbitrary callable for execution on the render thread.
 *
 * The callable is placement-new constructed into a transient linear buffer owned by the
 * active CommandQueue. During drain a trampoline invokes operator() and then explicitly
 * calls the destructor, permitting non-trivially-destructible captures.
 *
 * Thread-safe: yes. Ordering: FIFO per queue.
 *
 * Lifetime Rules:
 *  - Capture by value or use Ref<T> for shared ownership.
 *  - Avoid capturing references to stack data that may expire before execution.
 *
 * Performance Notes:
 *  - Keep captures small; prefer move-capturing pre-sized containers or handles.
 *  - Consider batching many tiny operations into one lambda.
 *
 * @tparam FuncT Invocable type; must support `void operator()()` and be destructible.
 * @param func Callable instance (forwarded, move/copy constructed into queue storage).
 *
 * @warning Lambda must not throw; exceptions crossing the trampoline boundary are UB.
 * @see SubmitResourceFree For GPU-safe resource destruction scheduling.
 */
```

### 17.2 `Renderer::SubmitResourceFree`

```cpp
/**
 * @brief Schedule a callable that frees GPU/Vulkan resources after the GPU is done using them.
 *
 * If called on the render thread, the callable is directly placed into the current frame's
 * release queue. Otherwise it is marshalled through Submit() to ensure render-thread context.
 * Destroy callbacks run only when the frame's fence indicates GPU completion.
 *
 * Use Cases: vkDestroy* calls, vmaFreeMemory, descriptor pool resets, pipeline cache purges.
 *
 * @tparam FuncT Invocable type with `void operator()()`.
 * @param func Callable performing destruction; captured resources must be sufficient for cleanup.
 *
 * @note Same explicit destructor pattern as Submit(); non-trivial captures are allowed.
 * @warning Do NOT use for regular per-frame logic; reserve for destruction/finalization.
 * @see Submit General deferred work submission.
 */
```

---

## 18. Quick Reference Cheat Sheet

| Action                  | API                                 | Notes                |
| ----------------------- | ----------------------------------- | -------------------- |
| Defer render-side logic | `Submit`                          | General work.        |
| Destroy VkBuffer later  | `SubmitResourceFree`              | GPU safe.            |
| Batch frees             | `SubmitResourceFree` (one lambda) | Fewer queue entries. |
| Rebuild pipeline        | `Submit`                          | Invalidation logic.  |
| Upload & free staging   | `Submit` + `SubmitResourceFree` | Two-phase lifecycle. |

---

## Appendices

### A. Potential Debug Instrumentation Hook

```cpp
struct CommandStats { std::atomic<size_t> submitted = 0; std::atomic<size_t> bytes = 0; };
```

Integrate increments inside `Allocate` and expose via ImGui panel.

### B. RAII Wrapper Concept (Pseudo)

```cpp
class DeferredVkImage {
public:
    DeferredVkImage(VkDevice d, VkImage i) : dev(d), img(i) {}
    ~DeferredVkImage() {
        if (img)
            Renderer::SubmitResourceFree([d = dev, i = img]() { vkDestroyImage(d, i, nullptr); });
    }
private: VkDevice dev; VkImage img; };
```

---

**End of Document**

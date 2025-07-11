# Thread_Manager.h - High-Level Thread Coordination Documentation

---

# Overview

The `thread_manager.h` header defines the high-level thread coordination system for Scenery Editor X. It provides sophisticated frame synchronization, render thread management, and application-render thread coordination specifically designed for Vulkan-based 3D rendering applications.

## File Structure

```cpp
/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware-Developers
* -------------------------------------------------------
* thread_manager.h
* -------------------------------------------------------
* Created: 22/6/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/core/threading/thread.h>

namespace SceneryEditorX
{
    struct RenderThreadData;
  
    enum class ThreadingPolicy : uint8_t { /* ... */ };
  
    class ThreadManager { /* ... */ };
}
```

## Core Components

### RenderThreadData Structure (Forward Declaration)

```cpp
struct RenderThreadData;
```

**Purpose**: Forward declaration of internal synchronization data structure used by ThreadManager.

**Implementation Details**:

- Contains Windows-specific synchronization primitives
- Manages thread state coordination
- Implemented in thread_manager.cpp for encapsulation

**Thread Safety**: Designed for safe access from multiple threads with proper synchronization.

### ThreadingPolicy Enumeration

```cpp
enum class ThreadingPolicy : uint8_t
{
    None = 0,           /// None will run on the main thread
    SingleThreaded,     /// SingleThreaded will run on the main thread  
    MultiThreaded       /// MultiThreaded will run on a separate thread
};
```

**Purpose**: Defines the threading strategy for the application, enabling flexible deployment across different hardware configurations and debugging scenarios.

#### Policy Descriptions

##### None (0)

- **Behavior**: All rendering operations execute on the main thread
- **Use Cases**: Debugging, single-core systems, minimal overhead scenarios
- **Performance**: No thread coordination overhead, but blocks main thread
- **Debugging**: Simplifies debugging by eliminating thread synchronization

##### SingleThreaded (1)

- **Behavior**: All operations run on main thread with simplified coordination
- **Use Cases**: Development debugging, profiling, compatibility testing
- **Performance**: No thread switching overhead, sequential execution
- **Resource Usage**: Minimal memory footprint, single call stack

##### MultiThreaded (2)

- **Behavior**: Separate dedicated render thread with full coordination
- **Use Cases**: Production builds, multi-core systems, optimal performance
- **Performance**: Parallel execution, optimal CPU utilization
- **Scalability**: Enables concurrent application logic and rendering

#### Usage Examples

```cpp
// Development configuration
ThreadManager debugManager(ThreadingPolicy::SingleThreaded);

// Production configuration  
ThreadManager prodManager(ThreadingPolicy::MultiThreaded);

// Conditional policy based on hardware
ThreadingPolicy SelectOptimalPolicy() {
    uint32_t coreCount = std::thread::hardware_concurrency();
  
    if (coreCount <= 2) {
        SEDX_CORE_INFO("Using single-threaded rendering for low core count: {}", coreCount);
        return ThreadingPolicy::SingleThreaded;
    } else {
        SEDX_CORE_INFO("Using multi-threaded rendering for core count: {}", coreCount);
        return ThreadingPolicy::MultiThreaded;
    }
}

// Configuration-driven policy selection
ThreadingPolicy LoadPolicyFromConfig() {
    Config config;
    std::string policyStr = config.Get<std::string>("threading.policy", "MultiThreaded");
  
    if (policyStr == "None") return ThreadingPolicy::None;
    if (policyStr == "SingleThreaded") return ThreadingPolicy::SingleThreaded;
    if (policyStr == "MultiThreaded") return ThreadingPolicy::MultiThreaded;
  
    SEDX_CORE_WARN("Unknown threading policy '{}', defaulting to MultiThreaded", policyStr);
    return ThreadingPolicy::MultiThreaded;
}
```

### ThreadManager Class

```cpp
class ThreadManager
{
public:
    enum class State : uint8_t
    {
        Idle = 0,
        Busy,
        Kick
    };
  
    ThreadManager(ThreadingPolicy policy);
    ~ThreadManager();

    static bool checkRenderThread();
  
    void Run();
    bool isRunning() const { return m_isRunning; }
    void Terminate();

    void Wait(State waitForState);
    void WaitAndSet(State waitForState, State setToState);
    void Set(State setToState);

    void NextFrame();
    void BlockUntilRenderComplete();
    void Kick();
    void Pump();

private:
    RenderThreadData *m_Data;
    bool m_isRunning = false;
    ThreadingPolicy m_policy;
    Thread renderThread;
    std::atomic<uint32_t> appThreadFrame = 0;
};
```

#### Inner Enumeration: State

```cpp
enum class State : uint8_t
{
    Idle = 0,
    Busy,
    Kick
};
```

**Purpose**: Represents the current state of the render thread for precise coordination between application and render threads.

##### State Descriptions

###### Idle (0)

- **Meaning**: Render thread is waiting for work
- **Usage**: Application can safely prepare next frame
- **Transitions**: From Busy after render completion, to Kick when work available

###### Busy (1)

- **Meaning**: Render thread is actively processing
- **Usage**: Application should not submit new render commands
- **Transitions**: From Kick when work starts, to Idle when work completes

###### Kick (2)

- **Meaning**: Signal to start render thread processing
- **Usage**: Trigger render thread to begin frame processing
- **Transitions**: From Idle when new work is ready, to Busy when processing starts

#### State Transition Diagram

```
    [Idle] ───Kick()───> [Kick] ───Start Work───> [Busy] ───Complete───> [Idle]
      ↑                                                                      │
      └──────────────────── BlockUntilRenderComplete() ────────────────────┘
```

#### Constructor and Destructor

##### Constructor

```cpp
ThreadManager(ThreadingPolicy policy);
```

**Purpose**: Initializes the thread management system with the specified threading policy.

**Parameters**:

- `policy`: Threading strategy to use (None, SingleThreaded, or MultiThreaded)

**Initialization Process**:

1. Allocates RenderThreadData structure
2. Initializes synchronization primitives (for MultiThreaded policy)
3. Sets up internal state tracking
4. Prepares render thread (doesn't start until Run() is called)

**Usage Examples**:

```cpp
// Basic initialization
ThreadManager manager(ThreadingPolicy::MultiThreaded);

// Configuration-driven initialization
class RenderEngine {
private:
    ThreadManager m_threadManager;
  
public:
    RenderEngine(const EngineConfig& config) 
        : m_threadManager(DetermineThreadingPolicy(config)) {
      
        SEDX_CORE_INFO("Render engine initialized with threading policy: {}", 
                       ThreadingPolicyToString(config.threadingPolicy));
    }
  
private:
    ThreadingPolicy DetermineThreadingPolicy(const EngineConfig& config) {
        if (config.debugMode) {
            return ThreadingPolicy::SingleThreaded;  // Easier debugging
        }
      
        if (config.targetPlatform == Platform::Mobile) {
            return ThreadingPolicy::SingleThreaded;  // Power efficiency
        }
      
        return ThreadingPolicy::MultiThreaded;  // Optimal performance
    }
};
```

##### Destructor

```cpp
~ThreadManager();
```

**Purpose**: Ensures proper cleanup of threading resources and thread termination.

**Cleanup Process**:

1. Terminates render thread if running
2. Releases synchronization primitives
3. Cleans up RenderThreadData structure
4. Resets static thread tracking

**RAII Compliance**: Automatically handles all resource cleanup without requiring explicit shutdown calls.

#### Public Interface Methods

##### checkRenderThread (Static Method)

```cpp
static bool checkRenderThread();
```

**Purpose**: Determines if the calling thread is the render thread.

**Returns**: `true` if currently executing on the render thread, `false` otherwise

**Thread Safety**: Thread-safe, can be called from any thread

**Usage Patterns**:

```cpp
// Conditional execution based on thread context
void PerformRenderOperation() {
    if (ThreadManager::checkRenderThread()) {
        // Execute directly - we're already on render thread
        ExecuteRenderCommands();
    } else {
        // Submit to render thread
        Renderer::Submit([]() {
            ExecuteRenderCommands();
        });
    }
}

// Debug assertions for thread affinity
void UpdateVulkanDescriptorSets() {
    SEDX_ASSERT(ThreadManager::checkRenderThread(), 
                "Vulkan descriptor updates must be performed on render thread");
  
    // Safe to update descriptors
    vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

// Thread-safe resource management
class ThreadAwareRenderer {
public:
    void UpdateBuffer(const Ref<Buffer>& buffer, const void* data, size_t size) {
        if (ThreadManager::checkRenderThread()) {
            UpdateBufferDirect(buffer, data, size);
        } else {
            // Copy data and submit to render thread
            auto dataCopy = std::make_shared<std::vector<uint8_t>>(
                static_cast<const uint8_t*>(data), 
                static_cast<const uint8_t*>(data) + size
            );
          
            Renderer::Submit([buffer, dataCopy]() {
                UpdateBufferDirect(buffer, dataCopy->data(), dataCopy->size());
            });
        }
    }
};
```

##### Run

```cpp
void Run();
```

**Purpose**: Starts the thread management system and begins render thread execution.

**Behavior by Policy**:

- **SingleThreaded/None**: Sets up state tracking only
- **MultiThreaded**: Creates and starts dedicated render thread

**Implementation Details**:

- Sets `m_isRunning = true`
- For MultiThreaded: Dispatches `Renderer::RenderThreadFunc` to render thread
- Registers render thread ID for `checkRenderThread()` functionality

**Usage Examples**:

```cpp
// Basic startup sequence
ThreadManager manager(ThreadingPolicy::MultiThreaded);
manager.Run();

// Application lifecycle management
class Application {
private:
    ThreadManager m_threadManager;
  
public:
    void Initialize() {
        SEDX_CORE_INFO("Starting thread manager...");
        m_threadManager.Run();
        SEDX_CORE_INFO("Thread manager started successfully");
    }
  
    void Shutdown() {
        SEDX_CORE_INFO("Shutting down thread manager...");
        m_threadManager.Terminate();
        SEDX_CORE_INFO("Thread manager shutdown complete");
    }
};

// Error handling during startup
void SafeStartup(ThreadManager& manager) {
    try {
        manager.Run();
        SEDX_CORE_INFO("Threading system started successfully");
    } catch (const std::exception& e) {
        SEDX_CORE_ERROR("Failed to start threading system: {}", e.what());
        throw;
    }
}
```

##### isRunning

```cpp
bool isRunning() const { return m_isRunning; }
```

**Purpose**: Checks if the thread management system is currently active.

**Returns**: `true` if system is running, `false` if not started or terminated

**Thread Safety**: Atomic read operation, safe from any thread

**Usage Examples**:

```cpp
// Main application loop
while (manager.isRunning() && !shouldExit) {
    UpdateApplication();
    manager.Pump();
}

// Conditional operations
void ConditionalRender(ThreadManager& manager) {
    if (manager.isRunning()) {
        manager.Kick();
        manager.BlockUntilRenderComplete();
    } else {
        SEDX_CORE_WARN("Attempted to render with inactive thread manager");
    }
}

// Status monitoring
class ThreadManagerMonitor {
private:
    ThreadManager& m_manager;
  
public:
    void LogStatus() {
        SEDX_CORE_INFO("Thread manager status: {}", 
                       m_manager.isRunning() ? "Running" : "Stopped");
    }
};
```

##### Terminate

```cpp
void Terminate();
```

**Purpose**: Gracefully shuts down the thread management system and waits for render thread completion.

**Shutdown Process**:

1. Sets `m_isRunning = false`
2. Signals render thread to exit via `Pump()`
3. Waits for render thread completion via `Join()`
4. Resets thread tracking state

**Thread Safety**: Safe to call from main thread, blocks until completion

**Usage Examples**:

```cpp
// Graceful application shutdown
class Application {
public:
    ~Application() {
        if (m_threadManager.isRunning()) {
            m_threadManager.Terminate();
        }
    }
};

// Exception-safe shutdown
void SafeShutdown(ThreadManager& manager) {
    try {
        manager.Terminate();
        SEDX_CORE_INFO("Thread manager terminated successfully");
    } catch (const std::exception& e) {
        SEDX_CORE_ERROR("Error during thread manager shutdown: {}", e.what());
    }
}

// Timeout-based shutdown (conceptual)
bool ShutdownWithTimeout(ThreadManager& manager, std::chrono::milliseconds timeout) {
    auto start = std::chrono::high_resolution_clock::now();
  
    // Note: Actual implementation would need async termination
    manager.Terminate();
  
    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    bool completedInTime = elapsed < timeout;
  
    if (!completedInTime) {
        SEDX_CORE_WARN("Thread manager shutdown exceeded timeout of {}ms", timeout.count());
    }
  
    return completedInTime;
}
```

#### Thread State Management Methods

##### Wait

```cpp
void Wait(State waitForState);
```

**Purpose**: Blocks the calling thread until the render thread reaches the specified state.

**Parameters**:

- `waitForState`: The target state to wait for (Idle, Busy, or Kick)

**Behavior by Policy**:

- **SingleThreaded/None**: Returns immediately (no-op)
- **MultiThreaded**: Blocks using condition variable until state matches

**Usage Examples**:

```cpp
// Wait for render thread to become idle
manager.Wait(ThreadManager::State::Idle);
SEDX_CORE_INFO("Render thread is now idle, safe to update resources");

// Synchronization in complex scenarios
void SynchronizedResourceUpdate(ThreadManager& manager) {
    // Ensure render thread is idle before modifying shared resources
    manager.Wait(ThreadManager::State::Idle);
  
    // Safe to update shared resources
    UpdateSharedRenderResources();
  
    // Kick render thread to process updates
    manager.Set(ThreadManager::State::Kick);
}

// Pipeline synchronization
class PipelineStage {
public:
    void WaitForPreviousStage(ThreadManager& manager) {
        manager.Wait(ThreadManager::State::Idle);  // Wait for previous stage completion
        SEDX_CORE_INFO("Previous pipeline stage completed, starting next stage");
    }
};
```

##### WaitAndSet

```cpp
void WaitAndSet(State waitForState, State setToState);
```

**Purpose**: Atomically waits for a specific state and then transitions to a new state.

**Parameters**:

- `waitForState`: State to wait for before proceeding
- `setToState`: State to transition to after wait completes

**Atomicity**: The wait and set operations are performed atomically under mutex protection

**Usage Examples**:

```cpp
// Atomic state transition for frame processing
manager.WaitAndSet(ThreadManager::State::Idle, ThreadManager::State::Kick);

// Render loop coordination
void RenderFrameCoordinated(ThreadManager& manager) {
    // Prepare frame data
    PrepareFrameData();
  
    // Atomically wait for idle and start processing
    manager.WaitAndSet(ThreadManager::State::Idle, ThreadManager::State::Kick);
  
    // Continue with other application work while render thread processes
    UpdateApplicationLogic();
}

// Multi-stage pipeline coordination
class RenderPipeline {
public:
    void ExecuteStage(ThreadManager& manager, PipelineStage stage) {
        switch (stage) {
            case PipelineStage::Geometry:
                manager.WaitAndSet(ThreadManager::State::Idle, ThreadManager::State::Kick);
                break;
            case PipelineStage::Lighting:
                manager.WaitAndSet(ThreadManager::State::Idle, ThreadManager::State::Kick);
                break;
            case PipelineStage::PostProcess:
                manager.WaitAndSet(ThreadManager::State::Idle, ThreadManager::State::Kick);
                break;
        }
    }
};
```

##### Set

```cpp
void Set(State setToState);
```

**Purpose**: Immediately sets the render thread state without waiting.

**Parameters**:

- `setToState`: Target state to transition to

**Non-blocking**: Returns immediately after setting state

**Usage Examples**:

```cpp
// Immediate state change
manager.Set(ThreadManager::State::Kick);  // Start render processing

// Conditional state management
void ConditionalKick(ThreadManager& manager, bool shouldRender) {
    if (shouldRender) {
        manager.Set(ThreadManager::State::Kick);
        SEDX_CORE_INFO("Render processing initiated");
    } else {
        SEDX_CORE_INFO("Skipping render for this frame");
    }
}

// Emergency stop scenario
void EmergencyStop(ThreadManager& manager) {
    manager.Set(ThreadManager::State::Idle);
    SEDX_CORE_WARN("Emergency stop initiated - render thread set to idle");
}
```

#### High-Level Coordination Methods

##### NextFrame

```cpp
void NextFrame();
```

**Purpose**: Advances the frame counter and swaps command queues for double-buffered rendering.

**Implementation Details**:

- Increments atomic frame counter
- Calls `Renderer::SwapQueues()` for command buffer management
- Prepares system for next frame's render commands

**Usage Examples**:

```cpp
// Standard frame advancement
void GameLoop(ThreadManager& manager) {
    while (running) {
        UpdateGame();
        manager.NextFrame();  // Advance to next frame
        manager.Kick();       // Start rendering
        manager.BlockUntilRenderComplete();  // Wait for completion
        PresentFrame();
    }
}

// Frame rate limiting
class FrameRateLimiter {
private:
    std::chrono::milliseconds m_targetFrameTime{16};  // 60 FPS
  
public:
    void LimitedNextFrame(ThreadManager& manager) {
        auto frameStart = std::chrono::high_resolution_clock::now();
      
        manager.NextFrame();
      
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto frameTime = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
      
        if (frameTime < m_targetFrameTime) {
            std::this_thread::sleep_for(m_targetFrameTime - frameTime);
        }
    }
};
```

##### BlockUntilRenderComplete

```cpp
void BlockUntilRenderComplete();
```

**Purpose**: Blocks the calling thread until the render thread completes its current work.

**Behavior by Policy**:

- **SingleThreaded/None**: Returns immediately
- **MultiThreaded**: Blocks until render thread state becomes Idle

**Usage Examples**:

```cpp
// Frame synchronization
void SynchronizedFrame(ThreadManager& manager) {
    manager.Kick();                          // Start render
    DoApplicationWork();                     // Parallel application work
    manager.BlockUntilRenderComplete();      // Wait for render completion
    PresentFrame();                          // Present completed frame
}

// Resource update synchronization  
void SafeResourceUpdate(ThreadManager& manager) {
    manager.BlockUntilRenderComplete();      // Ensure render is complete
    UpdateRenderResources();                 // Safe to modify resources
    manager.Kick();                          // Restart rendering
}

// Performance measurement
class RenderPerformanceProfiler {
public:
    void ProfileFrame(ThreadManager& manager) {
        auto start = std::chrono::high_resolution_clock::now();
      
        manager.Kick();
        manager.BlockUntilRenderComplete();
      
        auto end = std::chrono::high_resolution_clock::now();
        auto renderTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
      
        SEDX_CORE_INFO("Render time: {}μs", renderTime.count());
    }
};
```

##### Kick

```cpp
void Kick();
```

**Purpose**: Initiates render thread processing for the current frame.

**Behavior by Policy**:

- **SingleThreaded/None**: Directly calls `Renderer::WaitAndRender()`
- **MultiThreaded**: Sets state to Kick, waking render thread

**Usage Examples**:

```cpp
// Basic frame rendering
manager.Kick();  // Start render processing

// Conditional rendering
void ConditionalRender(ThreadManager& manager, bool needsRender) {
    if (needsRender) {
        manager.Kick();
        SEDX_CORE_INFO("Frame render initiated");
    } else {
        SEDX_CORE_INFO("Skipping frame render");
    }
}

// Immediate mode rendering
class ImmediateRenderer {
public:
    void RenderNow(ThreadManager& manager) {
        manager.Kick();                          // Start immediately
        manager.BlockUntilRenderComplete();      // Wait for completion
        // Frame is now complete
    }
};
```

##### Pump

```cpp
void Pump();
```

**Purpose**: High-level convenience method that performs a complete frame cycle: advance frame, start rendering, and wait for completion.

**Equivalent to**:

```cpp
void Pump() {
    NextFrame();
    Kick();
    BlockUntilRenderComplete();
}
```

**Usage Examples**:

```cpp
// Simplified main loop
void SimpleGameLoop(ThreadManager& manager) {
    while (running) {
        UpdateGame();
        manager.Pump();  // Complete frame cycle
        PresentFrame();
    }
}

// Frame-by-frame debugging
void DebugFrameStep(ThreadManager& manager) {
    SEDX_CORE_INFO("Executing single frame step");
    manager.Pump();
    SEDX_CORE_INFO("Frame step completed");
}

// Benchmarking
class FrameBenchmark {
public:
    void BenchmarkFrames(ThreadManager& manager, uint32_t frameCount) {
        auto start = std::chrono::high_resolution_clock::now();
      
        for (uint32_t i = 0; i < frameCount; ++i) {
            manager.Pump();
        }
      
        auto end = std::chrono::high_resolution_clock::now();
        auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
      
        double avgFrameTime = static_cast<double>(totalTime.count()) / frameCount;
        SEDX_CORE_INFO("Average frame time: {:.2f}ms over {} frames", avgFrameTime, frameCount);
    }
};
```

## Integration Patterns

### Vulkan Renderer Integration

```cpp
class VulkanApplication {
private:
    ThreadManager m_threadManager;
  
public:
    VulkanApplication() : m_threadManager(ThreadingPolicy::MultiThreaded) {}
  
    void Initialize() {
        InitializeVulkan();
        m_threadManager.Run();
    }
  
    void MainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
          
            UpdateApplicationLogic();
            RecordCommandBuffers();
          
            m_threadManager.Pump();  // Complete frame
        }
    }
  
    void Cleanup() {
        m_threadManager.Terminate();
        CleanupVulkan();
    }
};
```

### Multi-Threading Patterns

```cpp
// Producer-Consumer with ThreadManager
class RenderCommandProducer {
public:
    void ProduceCommands(ThreadManager& manager) {
        // Ensure render thread is idle
        manager.Wait(ThreadManager::State::Idle);
      
        // Produce render commands
        RecordRenderCommands();
      
        // Signal render thread to process
        manager.Set(ThreadManager::State::Kick);
    }
};
```

## Best Practices for GitHub Copilot

### 1. Always Use Appropriate Threading Policy

```cpp
// PREFERRED - Policy based on context
ThreadManager debugManager(ThreadingPolicy::SingleThreaded);  // For debugging
ThreadManager prodManager(ThreadingPolicy::MultiThreaded);    // For production

// AVOID - Hardcoded policy
ThreadManager manager(ThreadingPolicy::MultiThreaded);  // Might not be optimal for all contexts
```

### 2. Proper State Coordination

```cpp
// PREFERRED - Atomic state transitions
manager.WaitAndSet(ThreadManager::State::Idle, ThreadManager::State::Kick);

// AVOID - Separate operations that could race
manager.Wait(ThreadManager::State::Idle);
manager.Set(ThreadManager::State::Kick);  // Race condition possible
```

### 3. Use checkRenderThread() for Context Validation

```cpp
// PREFERRED - Context-aware execution
if (ThreadManager::checkRenderThread()) {
    UpdateVulkanResources();
} else {
    Renderer::Submit([]() { UpdateVulkanResources(); });
}

// AVOID - Assuming thread context
UpdateVulkanResources();  // Might not be on correct thread
```

This documentation provides comprehensive understanding of the ThreadManager class and its role in coordinating multi-threaded rendering in Scenery Editor X.

# Scenery Editor X - Threading System Documentation

---

## Overview

The Scenery Editor X threading system provides a comprehensive multi-platform threading framework designed specifically for high-performance Vulkan-based 3D rendering applications. The system consists of four core components that work together to manage thread creation, synchronization, and render thread coordination.

## Architecture

The threading system is built around four main components:

#### Core Components

1. **Thread** (`thread.h/cpp`) - Cross-platform thread wrapper with Windows-specific optimizations
2. **ThreadSignal** (`thread.h/cpp`) - Event-based thread synchronization primitives
3. **ThreadManager** (`thread_manager.h/cpp`) - High-level thread coordination and render pipeline management
4. **RenderThreadData** (internal) - Thread state management for render synchronization

## Key Design Principles

- **Platform Abstraction**: Provides cross-platform threading with Windows-specific optimizations
- **Render Thread Coordination**: Specialized for managing separate render threads in graphics applications
- **Efficient Synchronization**: Uses OS-native synchronization primitives for minimal overhead
- **Thread Safety**: All components are designed for thread-safe operation
- **Performance Focus**: Optimized for high-frequency render loop operations

---

## File Documentation

### thread.h

The `thread.h` header defines the core threading primitives used throughout Scenery Editor X.

#### Classes

##### Thread Class

```cpp
class Thread
{
public:
    Thread(const std::string &name);
  
    template<typename Func, typename... Args>
    void Dispatch(Func &&func, Args &&...args);
  
    void SetName(const std::string &name);
    void Join();
    std::thread::id GetThreadID() const;
  
private:
    std::string name;
    std::thread mem_thread;
};
```

**Purpose**: Provides a named thread wrapper around `std::thread` with Windows-specific optimizations for debugging and performance profiling.

**Key Features**:

- Named threads for easier debugging and profiling
- Template-based function dispatch with perfect forwarding
- Thread affinity setting for performance optimization
- Windows thread description support for debugging tools

##### ThreadSignal Class

```cpp
class ThreadSignal
{
public:
    ThreadSignal(const std::string &name, bool manualReset = false);
  
    void Wait();
    void Signal();
    void Reset();
  
private:
    void *signalHandle = nullptr;
};
```

**Purpose**: Provides event-based synchronization between threads using Windows Event objects.

**Key Features**:

- Named events for debugging purposes
- Auto-reset or manual-reset behavior
- Efficient OS-level synchronization
- Thread-safe signaling mechanism

---

### thread.cpp

The implementation file provides Windows-specific optimizations and cross-platform compatibility.

#### Key Implementation Details

##### Thread Management

```cpp
void Thread::SetName(const std::string &name)
{
    HANDLE threadHandle = mem_thread.native_handle();
    std::wstring str(name.begin(), name.end());
    SetThreadDescription(threadHandle, str.c_str());
    SetThreadAffinityMask(threadHandle, 8);
    this->name = name;
}
```

**Thread Affinity**: Sets thread affinity to CPU core 8 for consistent performance characteristics in rendering workloads.

**Thread Naming**: Uses Windows `SetThreadDescription` API for debugger integration and profiling tools.

##### Event Synchronization

```cpp
ThreadSignal::ThreadSignal(const std::string &name, const bool manualReset)
{
    std::wstring str(name.begin(), name.end());
    signalHandle = CreateEventW(nullptr, manualReset ? TRUE : FALSE, FALSE, str.c_str());
}
```

**Event Creation**: Creates named Windows Event objects with configurable reset behavior.

**Synchronization Operations**: Provides `Wait()`, `Signal()`, and `Reset()` operations using Windows synchronization APIs.

---

### thread_manager.h

Defines the high-level thread management system for coordinating application and render threads.

#### Enums

##### ThreadingPolicy

```cpp
enum class ThreadingPolicy : uint8_t
{
    None = 0,           /// None will run on the main thread
    SingleThreaded,     /// SingleThreaded will run on the main thread
    MultiThreaded       /// MultiThreaded will run on a separate thread
};
```

**Purpose**: Defines the threading strategy for the application, allowing flexible deployment across different hardware configurations.

##### ThreadManager::State

```cpp
enum class State : uint8_t
{
    Idle = 0,
    Busy,
    Kick
};
```

**Purpose**: Represents the current state of the render thread for coordination between application and render threads.

#### ThreadManager Class

```cpp
class ThreadManager
{
public:
    ThreadManager(ThreadingPolicy policy);
    ~ThreadManager();
  
    static bool checkRenderThread();
  
    void Run();
    bool isRunning() const;
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

**Purpose**: Manages the coordination between the main application thread and the dedicated render thread, providing frame synchronization and state management.

---

### thread_manager.cpp

Contains the implementation of thread coordination logic and render pipeline management.

#### Key Implementation Details

##### RenderThreadData Structure

```cpp
struct RenderThreadData
{
    CRITICAL_SECTION m_CriticalSection;
    CONDITION_VARIABLE m_ConditionVariable;
    ThreadManager::State m_State = ThreadManager::State::Idle;
};
```

**Purpose**: Internal data structure for managing thread synchronization state between application and render threads.

##### Thread Coordination

The ThreadManager uses Windows Critical Sections and Condition Variables for efficient thread coordination:

```cpp
void ThreadManager::WaitAndSet(State waitForState, State setToState)
{
    if (m_policy == ThreadingPolicy::SingleThreaded)
        return;

    EnterCriticalSection(&m_Data->m_CriticalSection);
    while (m_Data->m_State != waitForState)
        SleepConditionVariableCS(&m_Data->m_ConditionVariable, &m_Data->m_CriticalSection, INFINITE);

    m_Data->m_State = setToState;
    WakeAllConditionVariable(&m_Data->m_ConditionVariable);
    LeaveCriticalSection(&m_Data->m_CriticalSection);
}
```

---

## Usage Examples and Patterns

### Basic Thread Creation

```cpp
// Create a named thread for background processing
Thread backgroundThread("Background Processor");

// Dispatch work to the thread
backgroundThread.Dispatch([](int value) {
    // Perform background work
    ProcessData(value);
}, 42);

// Wait for completion
backgroundThread.Join();
```

### Thread Synchronization

```cpp
// Create synchronization primitives
ThreadSignal renderCompleteSignal("RenderComplete", false);
ThreadSignal frameStartSignal("FrameStart", true);

// Worker thread waits for signal
void WorkerThreadFunc() {
    while (running) {
        frameStartSignal.Wait();
    
        // Perform rendering work
        RenderFrame();
    
        // Signal completion
        renderCompleteSignal.Signal();
    }
}

// Main thread coordination
void MainThreadFunc() {
    frameStartSignal.Signal();           // Start rendering
    renderCompleteSignal.Wait();         // Wait for completion
    renderCompleteSignal.Reset();        // Reset for next frame
}
```

### Render Thread Management

```cpp
// Initialize threading system
ThreadManager threadManager(ThreadingPolicy::MultiThreaded);

// Start the render thread
threadManager.Run();

// Game loop coordination
while (applicationRunning) {
    // Update application logic
    UpdateApplicationLogic();
  
    // Prepare render data
    PrepareRenderData();
  
    // Coordinate with render thread
    threadManager.Pump();  // Advance frame and sync
}

// Shutdown
threadManager.Terminate();
```

### Advanced Synchronization Patterns

```cpp
// Multi-stage pipeline coordination
class RenderPipeline {
private:
    ThreadManager m_threadManager;
    ThreadSignal m_geometryReady;
    ThreadSignal m_lightingReady;
    ThreadSignal m_postProcessReady;
  
public:
    void ExecuteFrame() {
        // Geometry pass
        m_threadManager.Kick();
        m_geometryReady.Wait();
    
        // Lighting pass
        PrepareLightingData();
        m_threadManager.Set(ThreadManager::State::Kick);
        m_lightingReady.Wait();
    
        // Post-processing
        PreparePostProcessData();
        m_threadManager.Set(ThreadManager::State::Kick);
        m_postProcessReady.Wait();
    
        // Present
        m_threadManager.BlockUntilRenderComplete();
    }
};
```

## Thread Safety Considerations

### Reference Counting Integration

The threading system is designed to work seamlessly with the Scenery Editor X smart pointer system:

```cpp
// Thread-safe resource sharing
class ResourceManager {
private:
    Thread m_workerThread;
    std::mutex m_resourceMutex;
    std::vector<Ref<Texture>> m_pendingTextures;
  
public:
    void LoadTextureAsync(const std::string& path) {
        m_workerThread.Dispatch([this, path]() {
            auto texture = CreateRef<Texture>(path);
        
            std::lock_guard<std::mutex> lock(m_resourceMutex);
            m_pendingTextures.push_back(texture);
        });
    }
};
```

### Memory Management

The threading system respects the project's memory management guidelines:

```cpp
// Custom allocator integration
class ThreadSafeAllocator {
private:
    Thread m_allocationThread;
  
public:
    template<typename T, typename... Args>
    Ref<T> AllocateAsync(Args&&... args) {
        auto promise = std::make_shared<std::promise<Ref<T>>>();
        auto future = promise->get_future();
    
        m_allocationThread.Dispatch([promise, args...]() {
            auto object = CreateRef<T>(std::forward<Args>(args)...);
            promise->set_value(object);
        });
    
        return future.get();
    }
};
```

## Performance Optimization Guidelines

### Thread Affinity

```cpp
// Set specific thread affinity for different workloads
void OptimizeThreadAffinity() {
    Thread renderThread("Render");
    Thread audioThread("Audio");
    Thread networkThread("Network");
  
    // Render thread gets dedicated cores
    renderThread.SetName("Render");  // Automatically sets affinity to core 8
  
    // For custom affinity:
    HANDLE handle = renderThread.GetNativeHandle();
    SetThreadAffinityMask(handle, 0x0F);  // Cores 0-3
}
```

### Minimizing Context Switches

```cpp
// Batch operations to reduce thread context switches
class BatchProcessor {
private:
    ThreadSignal m_batchReady;
    std::queue<std::function<void()>> m_workQueue;
    std::mutex m_queueMutex;
  
public:
    void ProcessBatch() {
        std::vector<std::function<void()>> batch;
    
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            while (!m_workQueue.empty()) {
                batch.push_back(m_workQueue.front());
                m_workQueue.pop();
            }
        }
    
        // Process entire batch without interruption
        for (auto& work : batch) {
            work();
        }
    }
};
```

## Integration with Renderer

### Render Thread Coordination

The threading system is specifically designed to work with the Vulkan renderer:

```cpp
// Typical render loop integration
void RenderLoop() {
    ThreadManager threadManager(ThreadingPolicy::MultiThreaded);
    threadManager.Run();
  
    while (applicationRunning) {
        // Application thread: Update logic
        UpdateScene();
    
        // Prepare render commands
        CommandBuffer cmd = AcquireCommandBuffer();
        RecordRenderCommands(cmd);
    
        // Submit to render thread
        threadManager.NextFrame();
        threadManager.Kick();
    
        // Wait for GPU completion
        threadManager.BlockUntilRenderComplete();
    
        // Present frame
        SwapBuffers();
    }
  
    threadManager.Terminate();
}
```

### Resource Upload Coordination

```cpp
// Coordinate resource uploads with render thread
class ResourceUploader {
private:
    ThreadManager* m_threadManager;
    ThreadSignal m_uploadComplete;
  
public:
    void UploadTexture(const Ref<Texture>& texture) {
        // Ensure we're not on render thread
        if (!ThreadManager::checkRenderThread()) {
            // Submit upload command to render thread
            Renderer::Submit([this, texture]() {
                PerformTextureUpload(texture);
                m_uploadComplete.Signal();
            });
        
            // Wait for completion
            m_uploadComplete.Wait();
            m_uploadComplete.Reset();
        } else {
            // Already on render thread, upload directly
            PerformTextureUpload(texture);
        }
    }
};
```

## Debugging and Profiling

### Thread Naming for Debugging

```cpp
// Consistent thread naming for debugging tools
void SetupThreadNames() {
    Thread mainThread("Main");
    Thread renderThread("Render");
    Thread audioThread("Audio");
    Thread networkThread("Network");
    Thread fileIOThread("FileIO");
  
    // Names appear in:
    // - Visual Studio debugger
    // - Intel VTune Profiler
    // - Process Explorer
    // - RenderDoc captures
}
```

### Performance Profiling Integration

```cpp
// Profile thread performance
class ThreadProfiler {
public:
    static void ProfileRenderThread() {
        Thread profileThread("Profiler");
    
        profileThread.Dispatch([]() {
            SEDX_PROFILE_THREAD("Render Thread Profiler");
        
            while (profilingActive) {
                auto start = std::chrono::high_resolution_clock::now();
            
                // Wait for render thread state change
                ThreadManager::WaitForState(ThreadManager::State::Busy);
            
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
                SEDX_PROFILE_SCOPE_DURATION("Frame Time", duration.count());
            }
        });
    }
};
```

## Best Practices for GitHub Copilot

When generating code for Scenery Editor X, GitHub Copilot should follow these threading guidelines:

### 1. Always Use Project Threading Classes

```cpp
// PREFERRED - Use project threading classes
Thread workerThread("Worker");
ThreadSignal syncSignal("Sync");
ThreadManager renderManager(ThreadingPolicy::MultiThreaded);

// AVOID - Direct std::thread usage
std::thread workerThread;  // Missing naming and optimization
```

### 2. Proper Thread Naming

```cpp
// PREFERRED - Descriptive thread names
Thread geometryThread("Geometry Processor");
Thread textureThread("Texture Loader");
Thread audioThread("Audio Engine");

// AVOID - Generic or missing names
Thread thread1("Thread");  // Non-descriptive
Thread thread2("");       // Empty name
```

### 3. Integrate with ThreadManager

```cpp
// PREFERRED - Use ThreadManager for render coordination
if (ThreadManager::checkRenderThread()) {
    // Execute directly on render thread
    PerformRenderOperation();
} else {
    // Submit to render thread
    Renderer::Submit([]() {
        PerformRenderOperation();
    });
}

// AVOID - Manual thread coordination
if (currentThread == renderThread) {  // Manual checking
    PerformRenderOperation();
}
```

### 4. Proper Synchronization Patterns

```cpp
// PREFERRED - Use ThreadSignal for synchronization
ThreadSignal loadComplete("LoadComplete");

backgroundThread.Dispatch([&loadComplete]() {
    LoadResources();
    loadComplete.Signal();
});

loadComplete.Wait();

// AVOID - Manual synchronization
std::condition_variable cv;  // Bypasses project synchronization
std::mutex mtx;
```

### 5. Memory-Safe Thread Operations

```cpp
// PREFERRED - Use Ref<T> for thread-safe resource sharing
void ProcessTexturesAsync(const std::vector<Ref<Texture>>& textures) {
    Thread processingThread("Texture Processor");
  
    processingThread.Dispatch([textures]() {
        for (const auto& texture : textures) {
            if (texture) {  // Safe to check
                ProcessTexture(texture);
            }
        }
    });
}

// AVOID - Raw pointers in threaded contexts
void ProcessTextures(std::vector<Texture*> textures) {  // Unsafe
    // Risk of dangling pointers
}
```

### 6. Error Handling in Threading Context

```cpp
// PREFERRED - Proper error handling with logging
Thread workerThread("Worker");

workerThread.Dispatch([]() {
    try {
        PerformWork();
    } catch (const std::exception& e) {
        SEDX_CORE_ERROR("Worker thread error: {}", e.what());
        // Handle error appropriately
    }
});

// AVOID - Unhandled exceptions in threads
workerThread.Dispatch([]() {
    PerformWork();  // Could throw unhandled exception
});
```

This documentation provides comprehensive guidelines for using the Scenery Editor X threading system and should help GitHub Copilot generate code that adheres to the project's threading patterns and best practices.

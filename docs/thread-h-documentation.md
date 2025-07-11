# Thread.h - Core Threading Primitives Documentation

## Overview

The `thread.h` header file defines the foundational threading classes for Scenery Editor X. It provides cross-platform thread management with Windows-specific optimizations, focusing on named threads for debugging and event-based synchronization primitives.

## File Structure

```cpp
/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* thread.h
* -------------------------------------------------------
* Created: 22/6/2025
* -------------------------------------------------------
*/
#pragma once
#include <string>
#include <thread>

namespace SceneryEditorX
{
    class Thread { /* ... */ };
    class ThreadSignal { /* ... */ };
}
```

## Classes

### Thread Class

#### Purpose

The `Thread` class is a wrapper around `std::thread` that provides enhanced functionality for 3D rendering applications, including thread naming for debugging, automatic affinity setting, and template-based function dispatch with perfect forwarding.

#### Class Definition

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

#### Member Functions

##### Constructor

```cpp
Thread(const std::string &name);
```

**Purpose**: Creates a new Thread object with the specified name. The thread is not started until `Dispatch()` is called.

**Parameters**:
- `name`: A descriptive name for the thread used in debugging tools and profilers

**Usage Example**:
```cpp
Thread renderThread("Render Thread");
Thread audioThread("Audio Processing");
Thread networkThread("Network IO");
```

**Best Practices**:
- Use descriptive names that indicate the thread's purpose
- Follow naming convention: "Purpose Description" (e.g., "Texture Loader", "Command Buffer Recorder")
- Avoid generic names like "Thread1" or "Worker"

##### Dispatch (Template Function)

```cpp
template<typename Func, typename... Args>
void Dispatch(Func &&func, Args &&...args);
```

**Purpose**: Starts the thread execution with the provided function and arguments using perfect forwarding.

**Template Parameters**:
- `Func`: Function object type (lambda, function pointer, member function, etc.)
- `Args`: Variadic template for function arguments

**Parameters**:
- `func`: The function or callable object to execute on the thread
- `args`: Arguments to forward to the function

**Implementation Details**:
- Uses `std::forward` for perfect forwarding of arguments
- Automatically calls `SetName()` after thread creation
- Creates the underlying `std::thread` with forwarded parameters

**Usage Examples**:

```cpp
// Lambda function with no parameters
Thread workerThread("Worker");
workerThread.Dispatch([]() {
    SEDX_CORE_INFO("Worker thread started");
    PerformBackgroundWork();
});

// Lambda with captured variables
int processId = 42;
Thread processingThread("Data Processor");
processingThread.Dispatch([processId](const std::string& data) {
    ProcessData(data, processId);
}, "sample_data");

// Function pointer
void RenderFunction(const RenderData& data, float deltaTime);
Thread renderThread("Render");
renderThread.Dispatch(RenderFunction, renderData, 0.016f);

// Member function
class Renderer {
public:
    void RenderFrame(const Scene& scene);
};

Renderer renderer;
Scene currentScene;
Thread renderThread("Render");
renderThread.Dispatch(&Renderer::RenderFrame, &renderer, currentScene);

// Complex lambda with multiple parameters
Thread complexThread("Complex Processor");
complexThread.Dispatch([](const Ref<Mesh>& mesh, const Matrix4& transform, int lodLevel) {
    mesh->UpdateTransform(transform);
    mesh->SetLOD(lodLevel);
    mesh->Process();
}, meshRef, transformMatrix, 2);
```

**Thread-Safe Usage Patterns**:

```cpp
// Safe resource sharing with Ref<T>
Thread resourceThread("Resource Loader");
resourceThread.Dispatch([](Ref<TextureManager> manager, const std::string& path) {
    auto texture = manager->LoadTexture(path);
    // Ref<T> ensures thread-safe reference counting
}, textureManager, "assets/textures/diffuse.png");

// Proper error handling in threads
Thread safeThread("Safe Worker");
safeThread.Dispatch([]() {
    try {
        PerformRiskyOperation();
    } catch (const std::exception& e) {
        SEDX_CORE_ERROR("Thread error: {}", e.what());
    }
});
```

##### SetName

```cpp
void SetName(const std::string &name);
```

**Purpose**: Sets or updates the thread name for debugging and profiling tools.

**Parameters**:
- `name`: New name for the thread

**Implementation Details**:
- Converts string to wide string for Windows API
- Calls Windows `SetThreadDescription()` for debugger integration
- Sets thread affinity to CPU core 8 for consistent performance
- Updates internal name storage

**Usage Example**:
```cpp
Thread dynamicThread("Initial Name");
dynamicThread.SetName("Updated Purpose - Geometry Processing");
```

**Note**: This function is automatically called by `Dispatch()`, so manual calling is typically not necessary unless you need to change the thread name after creation.

##### Join

```cpp
void Join();
```

**Purpose**: Waits for the thread to complete execution.

**Implementation Details**:
- Checks if the thread is joinable before calling `std::thread::join()`
- Thread-safe and can be called multiple times

**Usage Example**:
```cpp
Thread backgroundThread("Background Work");
backgroundThread.Dispatch([]() {
    PerformLongRunningTask();
});

// Do other work on main thread
DoMainThreadWork();

// Wait for background thread to complete
backgroundThread.Join();
```

**Best Practices**:
- Always join threads before destroying Thread objects
- Consider using RAII patterns for automatic joining
- Be aware of potential deadlocks if threads wait on each other

##### GetThreadID

```cpp
std::thread::id GetThreadID() const;
```

**Purpose**: Returns the unique identifier of the underlying thread.

**Returns**: `std::thread::id` object representing the thread's unique identifier

**Usage Examples**:
```cpp
Thread workerThread("Worker");
workerThread.Dispatch([]() { /* work */ });

std::thread::id workerId = workerThread.GetThreadID();

// Thread identification in debugging
SEDX_CORE_INFO("Worker thread ID: {}", workerId);

// Thread comparison
if (std::this_thread::get_id() == workerThread.GetThreadID()) {
    SEDX_CORE_INFO("Currently executing on worker thread");
}
```

#### Advanced Usage Patterns

##### RAII Thread Management

```cpp
class RAIIThread {
private:
    Thread m_thread;
    
public:
    RAIIThread(const std::string& name) : m_thread(name) {}
    
    ~RAIIThread() {
        m_thread.Join();  // Automatic cleanup
    }
    
    template<typename Func, typename... Args>
    void Start(Func&& func, Args&&... args) {
        m_thread.Dispatch(std::forward<Func>(func), std::forward<Args>(args)...);
    }
};
```

##### Thread Pool Integration

```cpp
class ThreadPool {
private:
    std::vector<Thread> m_threads;
    
public:
    ThreadPool(size_t threadCount) {
        m_threads.reserve(threadCount);
        for (size_t i = 0; i < threadCount; ++i) {
            m_threads.emplace_back("Pool Worker " + std::to_string(i));
        }
    }
    
    void StartWorkers() {
        for (auto& thread : m_threads) {
            thread.Dispatch([this]() {
                WorkerThreadFunction();
            });
        }
    }
};
```

### ThreadSignal Class

#### Purpose

The `ThreadSignal` class provides event-based synchronization between threads using Windows Event objects. It offers efficient OS-level synchronization with both auto-reset and manual-reset capabilities.

#### Class Definition

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

#### Member Functions

##### Constructor

```cpp
ThreadSignal(const std::string &name, bool manualReset = false);
```

**Purpose**: Creates a new named event object for thread synchronization.

**Parameters**:
- `name`: Descriptive name for the event (used in debugging and system monitoring)
- `manualReset`: If `true`, creates a manual-reset event; if `false`, creates an auto-reset event

**Auto-Reset vs Manual-Reset**:
- **Auto-Reset (default)**: Event automatically resets to non-signaled state after a single waiting thread is released
- **Manual-Reset**: Event remains signaled until explicitly reset, allowing multiple threads to be released

**Usage Examples**:
```cpp
// Auto-reset event for single-thread notification
ThreadSignal taskComplete("Task Completed");

// Manual-reset event for multiple-thread notification
ThreadSignal allThreadsStart("All Threads Start", true);

// Frame synchronization
ThreadSignal frameReady("Frame Ready");
ThreadSignal renderComplete("Render Complete");
```

##### Wait

```cpp
void Wait();
```

**Purpose**: Blocks the calling thread until the event is signaled.

**Implementation Details**:
- Uses Windows `WaitForSingleObject()` with `INFINITE` timeout
- Thread-safe and efficient OS-level blocking
- For auto-reset events, automatically resets the event when returning

**Usage Examples**:
```cpp
// Basic synchronization
ThreadSignal dataReady("Data Ready");

// Producer thread
Thread producer("Producer");
producer.Dispatch([&dataReady]() {
    PrepareData();
    dataReady.Signal();  // Notify data is ready
});

// Consumer thread
Thread consumer("Consumer");
consumer.Dispatch([&dataReady]() {
    dataReady.Wait();    // Wait for data
    ProcessData();
});
```

##### Signal

```cpp
void Signal();
```

**Purpose**: Sets the event to signaled state, releasing waiting threads.

**Implementation Details**:
- Uses Windows `SetEvent()` function
- For auto-reset events, releases one waiting thread
- For manual-reset events, releases all waiting threads

**Usage Examples**:
```cpp
// Single thread notification
ThreadSignal resourceLoaded("Resource Loaded");
resourceLoaded.Signal();  // Wake up one waiting thread

// Multiple thread notification
ThreadSignal startProcessing("Start Processing", true);
startProcessing.Signal();  // Wake up all waiting threads
```

##### Reset

```cpp
void Reset();
```

**Purpose**: Manually resets the event to non-signaled state.

**Implementation Details**:
- Uses Windows `ResetEvent()` function
- Only necessary for manual-reset events
- Auto-reset events reset automatically

**Usage Examples**:
```cpp
// Manual-reset event cycle
ThreadSignal batchStart("Batch Start", true);

// Start all worker threads
batchStart.Signal();

// Wait for all workers to start
std::this_thread::sleep_for(std::chrono::milliseconds(10));

// Reset for next batch
batchStart.Reset();
```

#### Advanced Usage Patterns

##### Producer-Consumer Pattern

```cpp
class ProducerConsumer {
private:
    ThreadSignal m_dataReady;
    ThreadSignal m_spaceAvailable;
    std::queue<WorkItem> m_workQueue;
    std::mutex m_queueMutex;
    
public:
    ProducerConsumer() 
        : m_dataReady("Data Ready")
        , m_spaceAvailable("Space Available") {}
    
    void ProducerThread() {
        Thread producer("Producer");
        producer.Dispatch([this]() {
            while (running) {
                m_spaceAvailable.Wait();  // Wait for space
                
                {
                    std::lock_guard<std::mutex> lock(m_queueMutex);
                    m_workQueue.push(CreateWorkItem());
                }
                
                m_dataReady.Signal();  // Notify data available
            }
        });
    }
    
    void ConsumerThread() {
        Thread consumer("Consumer");
        consumer.Dispatch([this]() {
            while (running) {
                m_dataReady.Wait();  // Wait for data
                
                WorkItem item;
                {
                    std::lock_guard<std::mutex> lock(m_queueMutex);
                    item = m_workQueue.front();
                    m_workQueue.pop();
                }
                
                ProcessWorkItem(item);
                m_spaceAvailable.Signal();  // Notify space available
            }
        });
    }
};
```

##### Frame Synchronization Pattern

```cpp
class FrameSynchronizer {
private:
    ThreadSignal m_frameStart;
    ThreadSignal m_geometryComplete;
    ThreadSignal m_lightingComplete;
    ThreadSignal m_renderComplete;
    
public:
    FrameSynchronizer()
        : m_frameStart("Frame Start", true)  // Manual reset for all threads
        , m_geometryComplete("Geometry Complete")
        , m_lightingComplete("Lighting Complete")
        , m_renderComplete("Render Complete") {}
    
    void ExecuteFrame() {
        // Start all render stages
        m_frameStart.Signal();
        
        // Wait for geometry pass
        m_geometryComplete.Wait();
        
        // Wait for lighting pass
        m_lightingComplete.Wait();
        
        // Wait for final render
        m_renderComplete.Wait();
        
        // Reset for next frame
        m_frameStart.Reset();
    }
    
    void GeometryWorker() {
        Thread geometryThread("Geometry");
        geometryThread.Dispatch([this]() {
            while (running) {
                m_frameStart.Wait();
                ProcessGeometry();
                m_geometryComplete.Signal();
            }
        });
    }
};
```

##### Timeout and Cancellation Pattern

```cpp
class CancellableOperation {
private:
    ThreadSignal m_operationComplete;
    ThreadSignal m_cancelRequested;
    std::atomic<bool> m_cancelled{false};
    
public:
    CancellableOperation()
        : m_operationComplete("Operation Complete")
        , m_cancelRequested("Cancel Requested") {}
    
    bool ExecuteWithTimeout(std::chrono::milliseconds timeout) {
        Thread worker("Cancellable Worker");
        worker.Dispatch([this]() {
            while (!m_cancelled) {
                // Check for cancellation periodically
                if (CheckCancellation()) {
                    break;
                }
                
                PerformWork();
                
                if (WorkComplete()) {
                    m_operationComplete.Signal();
                    break;
                }
            }
        });
        
        // Use external timeout mechanism or implement custom wait
        auto start = std::chrono::high_resolution_clock::now();
        while (!HasCompleted() && !m_cancelled) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            
            auto elapsed = std::chrono::high_resolution_clock::now() - start;
            if (elapsed > timeout) {
                Cancel();
                break;
            }
        }
        
        worker.Join();
        return HasCompleted() && !m_cancelled;
    }
    
    void Cancel() {
        m_cancelled = true;
        m_cancelRequested.Signal();
    }
};
```

## Integration with Scenery Editor X Systems

### Memory Management Integration

```cpp
// Thread-safe resource management
class ThreadSafeResourceManager {
private:
    Thread m_loaderThread;
    ThreadSignal m_resourceReady;
    
public:
    Ref<Texture> LoadTextureAsync(const std::string& path) {
        auto promise = CreateRef<std::promise<Ref<Texture>>>();
        auto future = promise->get_future();
        
        m_loaderThread.Dispatch([promise, path]() {
            try {
                auto texture = CreateRef<Texture>(path);
                promise->set_value(texture);
            } catch (const std::exception& e) {
                promise->set_exception(std::current_exception());
            }
        });
        
        return future.get();
    }
};
```

### Logging Integration

```cpp
// Thread-safe logging in threaded contexts
void ThreadedOperation() {
    Thread workerThread("Logged Worker");
    
    workerThread.Dispatch([]() {
        SEDX_PROFILE_THREAD("Worker Thread");
        
        SEDX_CORE_INFO("Worker thread started");
        
        try {
            PerformWork();
            SEDX_CORE_INFO("Work completed successfully");
        } catch (const std::exception& e) {
            SEDX_CORE_ERROR("Worker thread error: {}", e.what());
        }
        
        SEDX_CORE_INFO("Worker thread ending");
    });
    
    workerThread.Join();
}
```

## Best Practices for GitHub Copilot

When generating code using the Thread and ThreadSignal classes:

### 1. Always Use Named Threads

```cpp
// PREFERRED
Thread renderThread("Vulkan Render");
Thread audioThread("FMOD Audio Engine");
Thread networkThread("TCP Client");

// AVOID
Thread thread1("Thread");
Thread t("t");
```

### 2. Proper Resource Management

```cpp
// PREFERRED - RAII pattern
{
    Thread workerThread("Resource Processor");
    workerThread.Dispatch([]() { ProcessResources(); });
    workerThread.Join();  // Automatic cleanup
}

// AVOID - Manual cleanup prone to errors
Thread* worker = new Thread("Worker");
worker->Dispatch([]() { ProcessResources(); });
// Missing join() and delete
```

### 3. Use ThreadSignal for Synchronization

```cpp
// PREFERRED - Use project synchronization
ThreadSignal taskComplete("Task Complete");
ThreadSignal dataReady("Data Ready");

// AVOID - Standard library synchronization
std::condition_variable cv;  // Bypasses project patterns
std::binary_semaphore sem;   // Platform-specific
```

### 4. Integrate with Project Systems

```cpp
// PREFERRED - Use project logging and error handling
Thread workerThread("Asset Loader");
workerThread.Dispatch([]() {
    try {
        LoadAssets();
    } catch (const std::exception& e) {
        SEDX_CORE_ERROR("Asset loading failed: {}", e.what());
    }
});

// AVOID - Direct standard output
workerThread.Dispatch([]() {
    std::cout << "Loading assets\n";  // Not integrated with logging system
});
```

This documentation provides comprehensive guidance for using the Thread and ThreadSignal classes effectively within the Scenery Editor X framework.

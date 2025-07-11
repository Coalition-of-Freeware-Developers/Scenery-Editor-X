# Scenery Editor X - Threading Documentation Index

---



## Overview

This documentation provides comprehensive coverage of the Scenery Editor X threading system, designed specifically for high-performance Vulkan-based 3D rendering applications. The threading system consists of four core files that work together to provide sophisticated multi-threaded coordination.

## Documentation Structure

### Core Documentation Files

1. **[Threading System Overview](threading-system-documentation.md)**

   - Complete system architecture and design principles
   - Integration patterns and best practices
   - GitHub Copilot usage guidelines
2. **[thread.h Documentation](thread-h-documentation.md)**

   - Thread and ThreadSignal class interfaces
   - Cross-platform thread management
   - Event-based synchronization primitives
3. **[thread.cpp Documentation](thread-cpp-documentation.md)**

   - Windows-specific implementation details
   - Performance optimizations and debugging integration
   - Platform-specific features and error handling
4. **[thread_manager.h Documentation](thread-manager-h-documentation.md)**

   - High-level thread coordination interface
   - ThreadingPolicy enumeration and ThreadManager class
   - State management and frame synchronization
5. **[thread_manager.cpp Documentation](thread-manager-cpp-documentation.md)**

   - Windows synchronization primitive implementations
   - Critical section and condition variable usage
   - Performance optimization opportunities

## Quick Reference Guide

### Core Classes

| Class                | File                 | Purpose                                         |
| -------------------- | -------------------- | ----------------------------------------------- |
| `Thread`           | thread.h/cpp         | Named thread wrapper with Windows optimizations |
| `ThreadSignal`     | thread.h/cpp         | Event-based thread synchronization              |
| `ThreadManager`    | thread_manager.h/cpp | High-level render thread coordination           |
| `RenderThreadData` | thread_manager.cpp   | Internal synchronization data structure         |

### Threading Policies

| Policy             | Use Case                        | Performance                   |
| ------------------ | ------------------------------- | ----------------------------- |
| `None`           | Debugging, minimal systems      | Lowest overhead, sequential   |
| `SingleThreaded` | Development, compatibility      | Low overhead, simplified      |
| `MultiThreaded`  | Production, optimal performance | Highest performance, parallel |

### Thread States

| State    | Meaning                    | Usage                            |
| -------- | -------------------------- | -------------------------------- |
| `Idle` | Render thread waiting      | Safe for resource updates        |
| `Busy` | Render thread processing   | Application should not interfere |
| `Kick` | Signal to start processing | Triggers render thread work      |

## Common Usage Patterns

### Basic Thread Creation

```cpp
// Create and start a named thread
Thread workerThread("Asset Loader");
workerThread.Dispatch([](const std::string& path) {
    LoadAsset(path);
}, "assets/model.obj");

// Wait for completion
workerThread.Join();
```

### Thread Synchronization

```cpp
// Event-based synchronization
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

### Render Thread Coordination

```cpp
// Initialize threading system
ThreadManager threadManager(ThreadingPolicy::MultiThreaded);
threadManager.Run();

// Main application loop
while (running) {
    UpdateApplication();        // Application logic
    PrepareRenderData();       // Prepare frame data
    threadManager.Pump();      // Complete frame cycle
    PresentFrame();            // Present to screen
}

// Shutdown
threadManager.Terminate();
```

### Context-Aware Operations

```cpp
// Automatic context detection
void PerformRenderOperation() {
    if (ThreadManager::checkRenderThread()) {
        // Already on render thread - execute directly
        ExecuteRenderCommands();
    } else {
        // Submit to render thread
        Renderer::Submit([]() {
            ExecuteRenderCommands();
        });
    }
}
```

## Architecture Diagrams

### Thread Communication Flow

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│ Application     │    │   ThreadManager  │    │ Render Thread   │
│ Thread          │    │                  │    │                 │
├─────────────────┤    ├──────────────────┤    ├─────────────────┤
│ UpdateLogic()   │    │                  │    │                 │
│ PrepareData()   │───▶│ NextFrame()      │    │                 │
│                 │    │ Kick()           │───▶│ WaitAndRender() │
│                 │    │                  │    │ ProcessFrame()  │
│ DoOtherWork()   │    │                  │    │                 │
│                 │◀───│ BlockUntilRender │◀───│ FrameComplete() │
│ PresentFrame()  │    │ Complete()       │    │                 │
└─────────────────┘    └──────────────────┘    └─────────────────┘
```

### State Transition Diagram

```
     ┌─────────┐
     │  Idle   │◀──────────────────────┐
     └─────────┘                       │
          │                            │
     Kick()│                           │ Complete
          ▼                            │
     ┌─────────┐     Start Work        │
     │  Kick   │────────────────▶┌─────────┐
     └─────────┘                 │  Busy   │
                                 └─────────┘
```

## Integration Guidelines

### Memory Management Integration

```cpp
// Thread-safe resource sharing with Ref<T>
class ThreadSafeResourceManager {
private:
    Thread m_loaderThread;
  
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
// Project logging system integration
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
    });
  
    workerThread.Join();
}
```

### Vulkan Renderer Integration

```cpp
// Render thread coordination for Vulkan
class VulkanRenderCoordinator {
private:
    ThreadManager m_threadManager;
  
public:
    void RenderFrame() {
        // Record command buffers on application thread
        RecordCommandBuffers();
    
        // Coordinate with render thread
        m_threadManager.NextFrame();
        m_threadManager.Kick();
    
        // Parallel work while render thread processes
        UpdateApplicationState();
    
        // Wait for render completion
        m_threadManager.BlockUntilRenderComplete();
    
        // Present frame
        PresentToSwapchain();
    }
};
```

## Performance Considerations

### Thread Affinity

- **Render Thread**: Automatically set to CPU core 8 for consistent performance
- **Application Thread**: Typically runs on main core (0)
- **Worker Threads**: Can be assigned to remaining cores for optimal load distribution

### Synchronization Overhead

| Operation               | Cost    | Use Case                |
| ----------------------- | ------- | ----------------------- |
| `checkRenderThread()` | Minimal | Frequent context checks |
| `Set()`               | Low     | State changes           |
| `Wait()`              | Medium  | Frame synchronization   |
| `WaitAndSet()`        | Medium  | Atomic transitions      |

### Memory Usage

- **Thread**: ~8 bytes + OS thread overhead
- **ThreadSignal**: ~8 bytes + Windows Event handle
- **ThreadManager**: ~64 bytes + synchronization primitives
- **RenderThreadData**: ~32 bytes for Windows objects

## Debugging and Profiling

### Visual Studio Integration

- Named threads appear in debugger threads window
- Thread descriptions visible in process monitoring tools
- RenderDoc captures show thread information

### Performance Profiling

- Intel VTune Profiler shows thread activity
- Custom profiling macros integrate with threading system
- Frame timing analysis through thread coordination

### Common Debugging Scenarios

1. **Deadlocks**: Use thread state logging and synchronization tracing
2. **Race Conditions**: Enable thread sanitizer and add debug assertions
3. **Performance Issues**: Profile thread coordination overhead

## Best Practices Summary

### For Application Developers

1. **Always use named threads** for easier debugging
2. **Check thread context** before performing render operations
3. **Use ThreadSignal** for event-based synchronization
4. **Integrate with project logging** for consistency
5. **Handle exceptions** properly in threaded contexts

### For GitHub Copilot

1. **Use project threading classes** instead of standard library equivalents
2. **Follow naming conventions** for threads and signals
3. **Check ThreadingPolicy** before synchronization operations
4. **Integrate with memory management** system (Ref `<T>`, Scope `<T>`)
5. **Use project logging macros** in threaded contexts

## File Dependencies

```
thread.h
├── <string>
├── <thread>
└── Used by: thread_manager.h, application code

thread.cpp
├── thread.h
├── Windows API headers
└── Implements: Thread, ThreadSignal

thread_manager.h
├── thread.h
└── Used by: renderer, application

thread_manager.cpp
├── thread_manager.h
├── renderer/renderer.h
└── Implements: ThreadManager, RenderThreadData
```

## Migration and Compatibility

### From Standard Threading

```cpp
// OLD - Standard library threading
std::thread worker([]() { DoWork(); });
std::mutex mtx;
std::condition_variable cv;

// NEW - Scenery Editor X threading
Thread worker("Worker");
worker.Dispatch([]() { DoWork(); });
ThreadSignal workComplete("Work Complete");
```

### Platform Abstraction

The current implementation is Windows-specific but designed for easy cross-platform extension:

- **Windows**: Uses CRITICAL_SECTION and CONDITION_VARIABLE
- **Linux**: Would use pthread_mutex_t and pthread_cond_t
- **macOS**: Would use pthread primitives with macOS-specific optimizations

## Troubleshooting

### Common Issues

1. **Thread not starting**: Check ThreadingPolicy configuration
2. **Synchronization timeouts**: Verify state transition logic
3. **Memory leaks**: Ensure proper Thread::Join() calls
4. **Performance degradation**: Profile synchronization overhead

### Debug Output

Enable threading debug output by defining:

```cpp
#define SEDX_THREADING_DEBUG 1
```

This documentation index provides a comprehensive guide to the Scenery Editor X threading system, enabling developers and AI assistants to effectively utilize the framework for high-performance multi-threaded applications.

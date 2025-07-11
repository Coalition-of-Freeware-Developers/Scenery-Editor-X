# GitHub Copilot Instructions: Scenery Editor X Threading System

## Overview

This document provides comprehensive instructions for GitHub Copilot when working with the Scenery Editor X threading system. The threading framework is designed specifically for high-performance Vulkan-based 3D rendering applications with sophisticated multi-threaded coordination.

## Core Architecture

The threading system consists of four main components:

### 1. Thread Class (`thread.h/cpp`)
- **Purpose**: Cross-platform thread wrapper with Windows optimizations
- **Key Features**: Named threads, template dispatch, thread affinity, debugging support
- **Usage**: Basic thread creation and management

### 2. ThreadSignal Class (`thread.h/cpp`)  
- **Purpose**: Event-based thread synchronization using Windows Event objects
- **Key Features**: Named events, auto/manual reset, efficient OS-level sync
- **Usage**: Thread coordination and signaling

### 3. ThreadManager Class (`thread_manager.h/cpp`)
- **Purpose**: High-level render thread coordination and pipeline management
- **Key Features**: Threading policies, state management, frame synchronization
- **Usage**: Main/render thread coordination in graphics applications

### 4. RenderThreadData (internal)
- **Purpose**: Internal synchronization data structure
- **Key Features**: Critical sections, condition variables, state tracking
- **Usage**: Internal ThreadManager synchronization

## Threading Policies

```cpp
enum class ThreadingPolicy : uint8_t
{
    None = 0,           // None will run on the main thread
    SingleThreaded,     // SingleThreaded will run on the main thread  
    MultiThreaded       // MultiThreaded will run on a separate thread
};
```

### Policy Usage Guidelines:
- **None/SingleThreaded**: Use for simple applications or debugging
- **MultiThreaded**: Use for production applications with separate render threads

## Class Usage Patterns

### Thread Class Usage

#### Basic Thread Creation
```cpp
// Create a named thread
Thread workerThread("Worker Thread");

// Dispatch work with perfect forwarding
workerThread.Dispatch([](int value) {
    // Work function
    ProcessData(value);
}, 42);

// Wait for completion
workerThread.Join();
```

#### Thread Management Best Practices
```cpp
// Always set meaningful names for debugging
Thread renderThread("Render Thread");

// Use template dispatch for type safety
auto lambda = [](const RenderData& data) {
    ProcessRenderCommands(data);
};
renderThread.Dispatch(lambda, renderData);

// Check thread ID for render thread validation
if (ThreadManager::checkRenderThread()) {
    // This code runs on the render thread
}
```

### ThreadSignal Usage

#### Basic Synchronization
```cpp
// Create named signal for debugging
ThreadSignal frameComplete("Frame Complete", false); // auto-reset

// Signal from one thread
frameComplete.Signal();

// Wait on another thread
frameComplete.Wait(); // Blocks until signaled
```

#### Manual Reset Signals
```cpp
// Use manual reset for multiple waiters
ThreadSignal shutdownSignal("Shutdown Signal", true);

// Signal shutdown to all threads
shutdownSignal.Signal();

// Multiple threads can wait
workerThread1.Wait(); // All threads wake up
workerThread2.Wait();

// Must manually reset for reuse
shutdownSignal.Reset();
```

### ThreadManager Usage

#### Initialization and Lifecycle
```cpp
// Create thread manager with policy
ThreadManager threadManager(ThreadingPolicy::MultiThreaded);

// Start the render thread
threadManager.Run();

// Main application loop
while (running) {
    // Update application logic
    UpdateApplication();
    
    // Pump render thread (frame coordination)
    threadManager.Pump();
}

// Clean shutdown
threadManager.Terminate();
```

#### State Management
```cpp
// Wait for specific render thread state
threadManager.Wait(ThreadManager::State::Idle);

// Atomic state transitions
threadManager.WaitAndSet(ThreadManager::State::Idle, ThreadManager::State::Busy);

// Simple state setting
threadManager.Set(ThreadManager::State::Kick);
```

#### Frame Synchronization
```cpp
// Advance to next frame
threadManager.NextFrame();

// Kick render thread processing
threadManager.Kick();

// Block until render completion
threadManager.BlockUntilRenderComplete();

// Complete frame pump (NextFrame + Kick + BlockUntilRenderComplete)
threadManager.Pump();
```

## Implementation Guidelines for Copilot

### 1. Thread Creation Standards
```cpp
// ALWAYS use meaningful names
Thread myThread("Descriptive Thread Name");

// PREFER template dispatch over std::thread directly
myThread.Dispatch(functionPtr, args...);

// ALWAYS join threads before destruction
myThread.Join();
```

### 2. Synchronization Patterns
```cpp
// USE ThreadSignal for event-based coordination
ThreadSignal dataReady("Data Ready");

// Producer thread
processData();
dataReady.Signal();

// Consumer thread  
dataReady.Wait();
consumeData();
```

### 3. Render Thread Coordination
```cpp
// ALWAYS check if running on render thread when needed
if (ThreadManager::checkRenderThread()) {
    // Safe to call Vulkan/OpenGL functions
    ExecuteRenderCommands();
} else {
    // Dispatch to render thread
    SubmitRenderCommands();
}
```

### 4. Error Handling Patterns
```cpp
// ALWAYS handle thread creation failures gracefully
try {
    Thread workerThread("Worker");
    workerThread.Dispatch(workFunction);
} catch (const std::exception& e) {
    SEDX_LOG_ERROR("Failed to create thread: {}", e.what());
    // Fallback to single-threaded execution
}
```

### 5. Memory Management
```cpp
// USE RAII for automatic cleanup
class ThreadedProcessor {
private:
    Thread m_workerThread{"Processor Thread"};
    ThreadSignal m_shutdownSignal{"Shutdown"};
    
public:
    ~ThreadedProcessor() {
        m_shutdownSignal.Signal();
        m_workerThread.Join();
    }
};
```

## Common Anti-Patterns to Avoid

### ❌ DON'T: Create unnamed threads
```cpp
Thread thread(""); // Bad - no meaningful name
```

### ❌ DON'T: Forget to join threads
```cpp
Thread worker("Worker");
worker.Dispatch(func);
// Missing worker.Join() - potential resource leak
```

### ❌ DON'T: Use polling instead of signals
```cpp
// Bad - polling
while (!dataReady) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

// Good - event-based
dataReady.Wait();
```

### ❌ DON'T: Call render functions on wrong thread
```cpp
// Bad - calling Vulkan on main thread
vkQueueSubmit(...); // Should be on render thread only
```

## Performance Considerations

### 1. Thread Affinity
- Thread class automatically sets affinity to core 8 for optimization
- Consider this when creating multiple threads

### 2. Synchronization Overhead
- ThreadSignal uses OS events - minimal overhead
- Critical sections in ThreadManager are optimized for high frequency

### 3. Frame Synchronization
- `Pump()` is designed for 60+ FPS applications
- State transitions are atomic and lock-free where possible

## Integration with Renderer

### Render Thread Function Signature
```cpp
// Renderer must provide this function
void Renderer::RenderThreadFunc(ThreadManager* manager);

// Example implementation pattern
void Renderer::RenderThreadFunc(ThreadManager* manager) {
    while (manager->isRunning()) {
        manager->WaitAndSet(State::Kick, State::Busy);
        
        // Execute render commands
        ExecuteRenderCommands();
        
        manager->Set(State::Idle);
    }
}
```

### Queue Management
```cpp
// Renderer provides queue swapping
void Renderer::SwapQueues(); // Called from NextFrame()
void Renderer::WaitAndRender(ThreadManager* manager); // Called from single-threaded mode
```

## Debugging and Profiling

### Thread Names in Tools
- All threads are named for easy identification in debuggers
- Use RenderDoc, Visual Studio debugger, or Intel VTune
- Thread names appear in call stacks and thread lists

### Logging Integration
```cpp
// Use project logging system for thread-related messages
SEDX_LOG_INFO("Thread '{}' started", threadName);
SEDX_LOG_ERROR("Thread synchronization timeout");
```

### State Monitoring
```cpp
// ThreadManager states can be logged for debugging
SEDX_LOG_DEBUG("ThreadManager state: {}", static_cast<int>(currentState));
```

## Testing Patterns

### Unit Testing Threads
```cpp
TEST_CASE("Thread Creation and Execution") {
    bool executed = false;
    Thread testThread("Test Thread");
    
    testThread.Dispatch([&executed]() {
        executed = true;
    });
    
    testThread.Join();
    REQUIRE(executed == true);
}
```

### Testing Synchronization
```cpp
TEST_CASE("ThreadSignal Coordination") {
    ThreadSignal signal("Test Signal");
    bool signaled = false;
    
    Thread producer("Producer");
    producer.Dispatch([&signal, &signaled]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        signaled = true;
        signal.Signal();
    });
    
    signal.Wait();
    REQUIRE(signaled == true);
    producer.Join();
}
```

## Platform Considerations

### Windows-Specific Features
- Uses SetThreadDescription for debugging
- SetThreadAffinityMask for performance
- Critical sections and condition variables for synchronization
- Windows Event objects for ThreadSignal

### Cross-Platform Notes
- Core Thread class wraps std::thread for portability
- Platform-specific optimizations in .cpp files
- Consider platform differences when extending

## Migration and Upgrade Patterns

### From std::thread
```cpp
// Old pattern
std::thread worker([]() { doWork(); });
worker.join();

// New pattern - more debuggable and optimized
Thread worker("Work Thread");
worker.Dispatch([]() { doWork(); });
worker.Join();
```

### Adding Render Thread Support
```cpp
// Add to existing application
class Application {
    ThreadManager m_threadManager{ThreadingPolicy::MultiThreaded};
    
    void Run() {
        m_threadManager.Run();
        
        while (running) {
            UpdateLogic();
            m_threadManager.Pump(); // Coordinate with render thread
        }
        
        m_threadManager.Terminate();
    }
};
```

This threading system is optimized for high-performance graphics applications and should be used as the standard for all thread-related operations in Scenery Editor X. Always prefer these classes over raw std::thread or OS-specific primitives.

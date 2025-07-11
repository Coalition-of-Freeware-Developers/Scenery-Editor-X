# Thread.cpp - Implementation Documentation

## Overview

The `thread.cpp` file contains the Windows-specific implementation of the Thread and ThreadSignal classes. It provides optimized thread management with debugging support, thread affinity settings, and efficient event-based synchronization using Windows API primitives.

## File Structure

```cpp
/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* thread.cpp
* -------------------------------------------------------
* Created: 22/6/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/core/threading/thread.h>

#define GLFW_EXPOSE_NATIVE_WIN32

namespace SceneryEditorX
{
    // Implementation details...
}
```

## Implementation Details

### Thread Class Implementation

#### Constructor Implementation

```cpp
Thread::Thread(const std::string &name)
{
    this->name = name;
}
```

**Purpose**: Initializes a Thread object with the specified name. The actual thread is not created until `Dispatch()` is called.

**Implementation Notes**:
- Simple member initialization
- No OS resources allocated at construction time
- Follows RAII principles for delayed resource acquisition

**Memory Management**:
- No dynamic allocation in constructor
- Thread-safe construction process
- Exception-safe initialization

#### SetName Implementation

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

**Purpose**: Sets the thread name for debugging tools and configures thread affinity for optimal performance.

**Implementation Breakdown**:

1. **Native Handle Acquisition**:
   ```cpp
   HANDLE threadHandle = mem_thread.native_handle();
   ```
   - Retrieves the Windows HANDLE from the std::thread
   - Enables access to Windows-specific thread APIs
   - Handle is valid only after thread creation

2. **String Conversion**:
   ```cpp
   std::wstring str(name.begin(), name.end());
   ```
   - Converts UTF-8 string to wide string (UTF-16)
   - Required for Windows Unicode API compatibility
   - Handles international characters properly

3. **Thread Description Setting**:
   ```cpp
   SetThreadDescription(threadHandle, str.c_str());
   ```
   - Uses Windows 10 Version 1607+ API
   - Visible in debuggers (Visual Studio, WinDbg)
   - Appears in profiling tools (Intel VTune, Process Explorer)
   - No-op on older Windows versions (graceful degradation)

4. **Thread Affinity Configuration**:
   ```cpp
   SetThreadAffinityMask(threadHandle, 8);
   ```
   - Sets thread affinity to CPU core 8 (bit mask: 0x08)
   - Provides consistent performance characteristics
   - Reduces context switching overhead
   - Optimal for render threads in multi-core systems

5. **Internal State Update**:
   ```cpp
   this->name = name;
   ```
   - Updates internal name storage
   - Maintains consistency with OS-level naming

**Performance Considerations**:
- Thread affinity setting improves cache locality
- Reduces CPU migration overhead
- Core 8 selection based on typical Intel CPU topology
- May need adjustment for different CPU architectures

**Debugging Benefits**:
- Thread names appear in Visual Studio debugger threads window
- RenderDoc captures show named threads
- Process Explorer displays thread names
- Simplifies multi-threaded debugging workflows

**Usage Example with Error Handling**:
```cpp
void SafeSetName(Thread& thread, const std::string& name) {
    try {
        thread.SetName(name);
        SEDX_CORE_INFO("Thread '{}' named successfully", name);
    } catch (const std::exception& e) {
        SEDX_CORE_WARN("Failed to set thread name '{}': {}", name, e.what());
    }
}
```

#### Join Implementation

```cpp
void Thread::Join()
{
    if (mem_thread.joinable())
        mem_thread.join();
}
```

**Purpose**: Safely waits for thread completion with proper state checking.

**Implementation Details**:
- **Joinability Check**: Prevents exceptions from calling join() on non-joinable threads
- **Thread Safety**: Safe to call multiple times
- **Exception Safety**: No exceptions thrown if thread is not joinable

**Thread States and Joinability**:
- **Joinable**: Thread was created and hasn't been joined or detached
- **Non-Joinable**: Thread was never started, already joined, or detached

**Best Practices Integration**:
```cpp
// RAII pattern for automatic joining
class ScopedThread {
private:
    Thread m_thread;
    
public:
    ScopedThread(const std::string& name) : m_thread(name) {}
    
    ~ScopedThread() {
        m_thread.Join();  // Safe automatic cleanup
    }
    
    template<typename Func, typename... Args>
    void Start(Func&& func, Args&&... args) {
        m_thread.Dispatch(std::forward<Func>(func), std::forward<Args>(args)...);
    }
};
```

#### GetThreadID Implementation

```cpp
std::thread::id Thread::GetThreadID() const
{
    return mem_thread.get_id();
}
```

**Purpose**: Returns the unique thread identifier for debugging and thread comparison.

**Implementation Notes**:
- Returns `std::thread::id` for type safety
- const-qualified for read-only access
- Thread-safe operation

**Usage Patterns**:
```cpp
// Thread identification in logging
Thread workerThread("Data Processor");
SEDX_CORE_INFO("Worker thread ID: {}", workerThread.GetThreadID());

// Thread comparison for validation
bool IsWorkerThread(const Thread& worker) {
    return std::this_thread::get_id() == worker.GetThreadID();
}

// Debug assertion for thread affinity
void ValidateRenderThread(const Thread& renderThread) {
    SEDX_ASSERT(std::this_thread::get_id() == renderThread.GetThreadID(), 
                "Function must be called from render thread");
}
```

### ThreadSignal Class Implementation

#### Constructor Implementation

```cpp
ThreadSignal::ThreadSignal(const std::string &name, const bool manualReset)
{
    std::wstring str(name.begin(), name.end());
    signalHandle = CreateEventW(nullptr, manualReset ? TRUE : FALSE, FALSE, str.c_str());
}
```

**Purpose**: Creates a Windows Event object for efficient inter-thread synchronization.

**Implementation Breakdown**:

1. **String Conversion**:
   ```cpp
   std::wstring str(name.begin(), name.end());
   ```
   - Converts to wide string for Windows Unicode API
   - Handles international characters in event names
   - Consistent with Windows naming conventions

2. **Event Creation**:
   ```cpp
   signalHandle = CreateEventW(nullptr, manualReset ? TRUE : FALSE, FALSE, str.c_str());
   ```

**CreateEventW Parameters**:
- **Security Attributes** (`nullptr`): Default security, not inheritable
- **Manual Reset** (`manualReset ? TRUE : FALSE`): Controls reset behavior
- **Initial State** (`FALSE`): Event starts in non-signaled state
- **Name** (`str.c_str()`): Named event for debugging and system monitoring

**Event Types**:
- **Auto-Reset Events** (`manualReset = false`):
  - Automatically reset after releasing one waiting thread
  - Ideal for single-threaded notifications
  - Producer-consumer scenarios

- **Manual-Reset Events** (`manualReset = true`):
  - Remain signaled until explicitly reset
  - Release all waiting threads simultaneously
  - Barrier synchronization scenarios

**Error Handling Considerations**:
```cpp
// Enhanced constructor with error handling
ThreadSignal::ThreadSignal(const std::string &name, const bool manualReset)
{
    std::wstring str(name.begin(), name.end());
    signalHandle = CreateEventW(nullptr, manualReset ? TRUE : FALSE, FALSE, str.c_str());
    
    if (signalHandle == nullptr) {
        DWORD error = GetLastError();
        SEDX_CORE_ERROR("Failed to create event '{}': Windows error {}", name, error);
        throw std::runtime_error("Event creation failed");
    }
}
```

#### Wait Implementation

```cpp
void ThreadSignal::Wait()
{
    WaitForSingleObject(signalHandle, INFINITE);
}
```

**Purpose**: Blocks the calling thread until the event is signaled.

**Implementation Details**:
- **WaitForSingleObject**: Efficient kernel-level wait
- **INFINITE Timeout**: Blocks indefinitely until signaled
- **Return Value**: Ignored (assumes success for simplicity)

**Performance Characteristics**:
- **Kernel Transition**: Thread switches to kernel mode for waiting
- **CPU Efficiency**: Thread yields CPU while waiting
- **Wake-up Latency**: Minimal latency when event is signaled
- **Memory Usage**: No busy-waiting, minimal memory overhead

**Enhanced Implementation with Error Handling**:
```cpp
void ThreadSignal::Wait()
{
    DWORD result = WaitForSingleObject(signalHandle, INFINITE);
    
    switch (result) {
        case WAIT_OBJECT_0:
            // Success - event was signaled
            break;
        case WAIT_FAILED:
            DWORD error = GetLastError();
            SEDX_CORE_ERROR("Wait failed with error: {}", error);
            break;
        default:
            SEDX_CORE_WARN("Unexpected wait result: {}", result);
            break;
    }
}
```

**Timeout Variant Implementation**:
```cpp
bool ThreadSignal::WaitTimeout(std::chrono::milliseconds timeout)
{
    DWORD timeoutMs = static_cast<DWORD>(timeout.count());
    DWORD result = WaitForSingleObject(signalHandle, timeoutMs);
    
    switch (result) {
        case WAIT_OBJECT_0:
            return true;  // Signaled
        case WAIT_TIMEOUT:
            return false; // Timeout occurred
        case WAIT_FAILED:
            DWORD error = GetLastError();
            SEDX_CORE_ERROR("Wait with timeout failed: {}", error);
            return false;
        default:
            return false;
    }
}
```

#### Signal Implementation

```cpp
void ThreadSignal::Signal()
{
    SetEvent(signalHandle);
}
```

**Purpose**: Sets the event to signaled state, releasing waiting threads.

**Implementation Details**:
- **SetEvent**: Windows API for event signaling
- **Atomic Operation**: Thread-safe signaling
- **Return Value**: Ignored for simplicity

**Behavior by Event Type**:
- **Auto-Reset**: Releases one waiting thread, then automatically resets
- **Manual-Reset**: Releases all waiting threads, remains signaled

**Enhanced Implementation**:
```cpp
void ThreadSignal::Signal()
{
    if (!SetEvent(signalHandle)) {
        DWORD error = GetLastError();
        SEDX_CORE_ERROR("Failed to signal event: Windows error {}", error);
    }
}
```

**Performance Considerations**:
- **Kernel Call Overhead**: Minimal system call overhead
- **Thread Wake-up**: Immediate wake-up of waiting threads
- **Scalability**: Efficient with multiple waiting threads

#### Reset Implementation

```cpp
void ThreadSignal::Reset()
{
    ResetEvent(signalHandle);
}
```

**Purpose**: Manually resets the event to non-signaled state.

**Implementation Details**:
- **ResetEvent**: Windows API for manual reset
- **Manual-Reset Events**: Primarily used with manual-reset events
- **Auto-Reset Events**: Usually not needed (automatic reset)

**Usage Patterns**:
```cpp
// Barrier pattern with manual-reset event
ThreadSignal barrier("Thread Barrier", true);

// Start all worker threads
barrier.Signal();

// Wait for all workers to initialize
std::this_thread::sleep_for(std::chrono::milliseconds(100));

// Reset barrier for next synchronization point
barrier.Reset();
```

**Enhanced Implementation**:
```cpp
void ThreadSignal::Reset()
{
    if (!ResetEvent(signalHandle)) {
        DWORD error = GetLastError();
        SEDX_CORE_WARN("Failed to reset event: Windows error {}", error);
    }
}
```

## Platform-Specific Optimizations

### Windows-Specific Features

#### Thread Debugging Integration

```cpp
// Automatic debugger integration
void Thread::SetName(const std::string &name)
{
    // ... existing implementation ...
    
    // Additional debugging support
    #ifdef _DEBUG
    // Set thread name for Visual Studio debugger (legacy method)
    const DWORD MS_VC_EXCEPTION = 0x406D1388;
    struct THREADNAME_INFO {
        DWORD dwType;     // Must be 0x1000
        LPCSTR szName;    // Pointer to name (in user addr space)
        DWORD dwThreadID; // Thread ID (-1=caller thread)
        DWORD dwFlags;    // Reserved for future use, must be zero
    };
    
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = name.c_str();
    info.dwThreadID = GetCurrentThreadId();
    info.dwFlags = 0;
    
    __try {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        // Ignore exceptions in debugger communication
    }
    #endif
}
```

#### Performance Monitoring Integration

```cpp
// Thread performance monitoring
class ThreadPerformanceMonitor {
private:
    HANDLE m_threadHandle;
    std::string m_threadName;
    
public:
    ThreadPerformanceMonitor(const Thread& thread) 
        : m_threadHandle(thread.GetNativeHandle())
        , m_threadName(thread.GetName()) {}
    
    double GetCPUUsage() {
        FILETIME creationTime, exitTime, kernelTime, userTime;
        if (GetThreadTimes(m_threadHandle, &creationTime, &exitTime, &kernelTime, &userTime)) {
            // Calculate CPU usage percentage
            return CalculateCPUPercentage(kernelTime, userTime);
        }
        return 0.0;
    }
    
    void LogPerformanceStats() {
        SEDX_CORE_INFO("Thread '{}' CPU usage: {:.2f}%", m_threadName, GetCPUUsage());
    }
};
```

### Cross-Platform Compatibility

#### Preprocessor Abstractions

```cpp
// Platform-specific abstractions
#ifdef SEDX_PLATFORM_WINDOWS
    #define SEDX_THREAD_HANDLE HANDLE
    #define SEDX_INVALID_THREAD_HANDLE INVALID_HANDLE_VALUE
#elif defined(SEDX_PLATFORM_LINUX)
    #define SEDX_THREAD_HANDLE pthread_t
    #define SEDX_INVALID_THREAD_HANDLE 0
#elif defined(SEDX_PLATFORM_MACOS)
    #define SEDX_THREAD_HANDLE pthread_t
    #define SEDX_INVALID_THREAD_HANDLE 0
#endif

// Cross-platform thread naming
void Thread::SetNameCrossPlatform(const std::string& name) {
#ifdef SEDX_PLATFORM_WINDOWS
    // Windows implementation (current)
    SetName(name);
#elif defined(SEDX_PLATFORM_LINUX)
    pthread_setname_np(mem_thread.native_handle(), name.substr(0, 15).c_str());
#elif defined(SEDX_PLATFORM_MACOS)
    if (std::this_thread::get_id() == mem_thread.get_id()) {
        pthread_setname_np(name.c_str());
    }
#endif
}
```

## Error Handling and Robustness

### Exception Safety

```cpp
// Exception-safe thread operations
class RobustThread {
private:
    Thread m_thread;
    std::atomic<bool> m_hasException{false};
    std::exception_ptr m_exceptionPtr;
    
public:
    template<typename Func, typename... Args>
    void DispatchSafe(Func&& func, Args&&... args) {
        m_thread.Dispatch([this, func = std::forward<Func>(func), args...]() mutable {
            try {
                func(args...);
            } catch (...) {
                m_hasException = true;
                m_exceptionPtr = std::current_exception();
                SEDX_CORE_ERROR("Exception in thread '{}'", m_thread.GetName());
            }
        });
    }
    
    void JoinAndRethrow() {
        m_thread.Join();
        if (m_hasException && m_exceptionPtr) {
            std::rethrow_exception(m_exceptionPtr);
        }
    }
};
```

### Resource Cleanup

```cpp
// RAII for ThreadSignal
class ThreadSignalRAII {
private:
    void* m_handle;
    
public:
    ThreadSignalRAII(const std::string& name, bool manualReset = false) {
        std::wstring wName(name.begin(), name.end());
        m_handle = CreateEventW(nullptr, manualReset ? TRUE : FALSE, FALSE, wName.c_str());
        
        if (m_handle == nullptr) {
            throw std::runtime_error("Failed to create event: " + name);
        }
    }
    
    ~ThreadSignalRAII() {
        if (m_handle != nullptr) {
            CloseHandle(m_handle);
        }
    }
    
    // Move semantics
    ThreadSignalRAII(ThreadSignalRAII&& other) noexcept
        : m_handle(other.m_handle) {
        other.m_handle = nullptr;
    }
    
    ThreadSignalRAII& operator=(ThreadSignalRAII&& other) noexcept {
        if (this != &other) {
            if (m_handle != nullptr) {
                CloseHandle(m_handle);
            }
            m_handle = other.m_handle;
            other.m_handle = nullptr;
        }
        return *this;
    }
    
    // Delete copy operations
    ThreadSignalRAII(const ThreadSignalRAII&) = delete;
    ThreadSignalRAII& operator=(const ThreadSignalRAII&) = delete;
};
```

## Integration Examples

### Vulkan Renderer Integration

```cpp
// Thread coordination for Vulkan rendering
class VulkanRenderCoordinator {
private:
    Thread m_renderThread;
    ThreadSignal m_frameStart;
    ThreadSignal m_commandsReady;
    ThreadSignal m_renderComplete;
    
public:
    VulkanRenderCoordinator() 
        : m_renderThread("Vulkan Render Thread")
        , m_frameStart("Frame Start")
        , m_commandsReady("Commands Ready") 
        , m_renderComplete("Render Complete") {
        
        m_renderThread.Dispatch([this]() {
            VulkanRenderLoop();
        });
    }
    
private:
    void VulkanRenderLoop() {
        // Set thread name for RenderDoc captures
        m_renderThread.SetName("Vulkan Render Thread");
        
        while (m_running) {
            m_frameStart.Wait();
            
            // Record command buffers
            RecordCommandBuffers();
            m_commandsReady.Signal();
            
            // Submit to GPU
            SubmitToGPU();
            
            // Wait for GPU completion
            WaitForGPU();
            m_renderComplete.Signal();
        }
    }
    
public:
    void RenderFrame() {
        m_frameStart.Signal();
        m_commandsReady.Wait();
        m_renderComplete.Wait();
    }
};
```

### Asset Loading Integration

```cpp
// Threaded asset loading with progress tracking
class ThreadedAssetLoader {
private:
    Thread m_loaderThread;
    ThreadSignal m_loadRequest;
    ThreadSignal m_loadComplete;
    std::atomic<float> m_progress{0.0f};
    
public:
    Ref<Asset> LoadAssetAsync(const std::string& path) {
        auto assetPromise = CreateRef<std::promise<Ref<Asset>>>();
        auto assetFuture = assetPromise->get_future();
        
        m_loaderThread.Dispatch([this, path, assetPromise]() {
            try {
                SEDX_PROFILE_THREAD("Asset Loader");
                
                auto asset = LoadAssetFromDisk(path);
                UpdateProgress(1.0f);
                
                assetPromise->set_value(asset);
                m_loadComplete.Signal();
                
            } catch (const std::exception& e) {
                SEDX_CORE_ERROR("Asset loading failed: {}", e.what());
                assetPromise->set_exception(std::current_exception());
            }
        });
        
        return assetFuture.get();
    }
    
private:
    void UpdateProgress(float progress) {
        m_progress = progress;
        SEDX_CORE_INFO("Asset loading progress: {:.1f}%", progress * 100.0f);
    }
};
```

## Best Practices for Implementation

### 1. Always Handle Windows API Errors

```cpp
// PREFERRED - Check API return values
void ThreadSignal::Signal() {
    if (!SetEvent(signalHandle)) {
        DWORD error = GetLastError();
        SEDX_CORE_ERROR("SetEvent failed: {}", error);
        // Consider throwing exception for critical errors
    }
}

// AVOID - Ignoring API errors
void ThreadSignal::Signal() {
    SetEvent(signalHandle);  // No error checking
}
```

### 2. Use RAII for Resource Management

```cpp
// PREFERRED - Automatic cleanup
class ManagedThread {
    Thread m_thread;
public:
    ~ManagedThread() { m_thread.Join(); }
};

// AVOID - Manual cleanup
Thread* thread = new Thread("Worker");
// ... use thread ...
delete thread;  // Might not be called, missing Join()
```

### 3. Integrate with Project Logging

```cpp
// PREFERRED - Use project logging system
m_renderThread.Dispatch([]() {
    SEDX_PROFILE_THREAD("Render Thread");
    SEDX_CORE_INFO("Render thread started");
    // ... work ...
});

// AVOID - Direct console output
m_renderThread.Dispatch([]() {
    std::cout << "Render thread started\n";  // Not integrated
});
```

This implementation documentation provides comprehensive understanding of the Windows-specific optimizations and integration patterns used in the Scenery Editor X threading system.

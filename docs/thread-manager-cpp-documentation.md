# Thread_Manager.cpp - Implementation Documentation

---

## Overview

The `thread_manager.cpp` file contains the Windows-specific implementation of the ThreadManager class and RenderThreadData structure. It provides sophisticated thread coordination using Windows synchronization primitives optimized for high-performance Vulkan rendering applications.

## File Structure

```cpp
/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* thread_manager.cpp
* -------------------------------------------------------
* Created: 22/6/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/core/threading/thread.h>
#include <SceneryEditorX/core/threading/thread_manager.h>
#include <SceneryEditorX/renderer/renderer.h>

namespace SceneryEditorX
{
    // Implementation details...
}
```

## Core Data Structures

### RenderThreadData Structure

```cpp
struct RenderThreadData
{
    CRITICAL_SECTION m_CriticalSection;
    CONDITION_VARIABLE m_ConditionVariable;
    ThreadManager::State m_State = ThreadManager::State::Idle;
};
```

**Purpose**: Internal data structure managing synchronization between application and render threads.

#### Member Analysis

##### CRITICAL_SECTION m_CriticalSection

**Purpose**: Windows lightweight mutual exclusion object for protecting shared state.

**Characteristics**:

- **User-mode optimization**: Attempts user-mode acquisition before kernel transition
- **Recursive locking**: Same thread can acquire multiple times
- **Performance**: Faster than mutex for Windows applications
- **Scope**: Process-local, cannot be shared between processes

**Usage Pattern**:

```cpp
// Atomic state check and modification
EnterCriticalSection(&m_Data->m_CriticalSection);
if (m_Data->m_State == ThreadManager::State::Idle) {
    m_Data->m_State = ThreadManager::State::Kick;
    // Signal waiting threads
}
LeaveCriticalSection(&m_Data->m_CriticalSection);
```

**Performance Characteristics**:

- **Contention**: Minimal overhead when uncontended
- **Kernel transition**: Only occurs under contention
- **Spin count**: Windows automatically optimizes spin behavior
- **Memory ordering**: Provides full memory barrier semantics

##### CONDITION_VARIABLE m_ConditionVariable

**Purpose**: Windows condition variable for efficient thread blocking and notification.

**Characteristics**:

- **Efficient waiting**: Threads block without consuming CPU cycles
- **Spurious wakeups**: Handles spurious wakeups correctly with while loops
- **Multiple waiters**: Supports multiple threads waiting on same condition
- **Atomic operations**: Integrated with critical section for atomic wait/signal

**Usage Pattern**:

```cpp
// Wait for specific state
EnterCriticalSection(&m_Data->m_CriticalSection);
while (m_Data->m_State != desiredState) {
    // Atomically releases critical section and waits
    SleepConditionVariableCS(&m_Data->m_ConditionVariable, &m_Data->m_CriticalSection, INFINITE);
}
// Critical section reacquired here
LeaveCriticalSection(&m_Data->m_CriticalSection);
```

**Thread Safety Guarantees**:

- **Atomic wait**: Release of critical section and wait are atomic
- **Reacquisition**: Critical section automatically reacquired on wake
- **Memory ordering**: Full memory barrier on wake-up
- **Signal safety**: Safe to signal from any thread

##### ThreadManager::State m_State

**Purpose**: Current state of the render thread coordination system.

**State Semantics**:

- **Idle**: Render thread waiting for work, application thread can safely prepare data
- **Busy**: Render thread actively processing, application should not interfere
- **Kick**: Signal for render thread to start processing

**Thread Visibility**:

- Protected by critical section for all access
- Atomic read/write semantics guaranteed by synchronization
- Consistent view across all threads

### Static Thread Tracking

```cpp
static std::thread::id s_threadID;
```

**Purpose**: Global tracking of the render thread ID for `checkRenderThread()` functionality.

**Implementation Details**:

- **Initialization**: Set during `ThreadManager::Run()`
- **Reset**: Cleared during `ThreadManager::Terminate()`
- **Thread Safety**: Read-only after initialization, safe from all threads
- **Lifetime**: Managed by ThreadManager constructor/destructor

## Implementation Analysis

### Constructor Implementation

```cpp
ThreadManager::ThreadManager(ThreadingPolicy policy) : renderThread("Render Thread"), m_policy(policy)
{
    m_Data = new RenderThreadData();

    if (m_policy == ThreadingPolicy::MultiThreaded)
    {
        InitializeCriticalSection(&m_Data->m_CriticalSection);
        InitializeConditionVariable(&m_Data->m_ConditionVariable);
    }
}
```

**Implementation Breakdown**:

1. **Member Initialization**:

   ```cpp
   : renderThread("Render Thread"), m_policy(policy)
   ```

   - Initializes render thread with descriptive name
   - Stores threading policy for behavior decisions
2. **Data Structure Allocation**:

   ```cpp
   m_Data = new RenderThreadData();
   ```

   - Dynamic allocation for internal synchronization data
   - Allows forward declaration in header
   - Enables pimpl-like encapsulation
3. **Conditional Synchronization Setup**:

   ```cpp
   if (m_policy == ThreadingPolicy::MultiThreaded)
   {
       InitializeCriticalSection(&m_Data->m_CriticalSection);
       InitializeConditionVariable(&m_Data->m_ConditionVariable);
   }
   ```

   - Only initializes synchronization primitives when needed
   - Reduces overhead for single-threaded policies
   - Windows-specific initialization

**Memory Management**:

- RenderThreadData allocated on heap for lifetime management
- Critical section and condition variable are POD types
- No additional allocations beyond the structure itself

**Exception Safety**:

```cpp
// Enhanced constructor with exception safety
ThreadManager::ThreadManager(ThreadingPolicy policy) try 
    : renderThread("Render Thread"), m_policy(policy), m_Data(nullptr)
{
    m_Data = new RenderThreadData();
  
    if (m_policy == ThreadingPolicy::MultiThreaded) {
        InitializeCriticalSection(&m_Data->m_CriticalSection);
        InitializeConditionVariable(&m_Data->m_ConditionVariable);
    }
} catch (...) {
    delete m_Data;
    throw;
}
```

### Destructor Implementation

```cpp
ThreadManager::~ThreadManager()
{
    if (m_policy == ThreadingPolicy::MultiThreaded)
        DeleteCriticalSection(&m_Data->m_CriticalSection);

    s_threadID = std::thread::id();
}
```

**Implementation Analysis**:

1. **Synchronization Cleanup**:

   ```cpp
   if (m_policy == ThreadingPolicy::MultiThreaded)
       DeleteCriticalSection(&m_Data->m_CriticalSection);
   ```

   - Conditionally cleans up Windows synchronization objects
   - Matches initialization pattern in constructor
   - No cleanup needed for condition variables (automatic)
2. **Thread ID Reset**:

   ```cpp
   s_threadID = std::thread::id();
   ```

   - Resets global thread tracking
   - Ensures `checkRenderThread()` returns false after destruction
   - Thread-safe assignment of default-constructed id

**Resource Management Issues**:

```cpp
// Missing in current implementation - should include:
delete m_Data;  // Memory leak fix
```

**Enhanced Destructor**:

```cpp
ThreadManager::~ThreadManager()
{
    // Ensure thread is terminated before cleanup
    if (m_isRunning) {
        Terminate();
    }
  
    // Clean up synchronization objects
    if (m_policy == ThreadingPolicy::MultiThreaded) {
        DeleteCriticalSection(&m_Data->m_CriticalSection);
    }
  
    // Clean up data structure
    delete m_Data;
    m_Data = nullptr;
  
    // Reset thread tracking
    s_threadID = std::thread::id();
}
```

### checkRenderThread Implementation

```cpp
bool ThreadManager::checkRenderThread()
{
    return s_threadID == std::this_thread::get_id();
}
```

**Purpose**: Thread-safe determination of render thread context.

**Implementation Details**:

- **Thread ID Comparison**: Uses `std::thread::id` equality operator
- **Thread Safety**: Read-only access to static variable
- **Performance**: Extremely fast, single comparison operation
- **Reliability**: Works correctly even if ThreadManager is destroyed

**Usage Validation**:

```cpp
// Debug validation usage
void ValidateRenderContext() {
    if (ThreadManager::checkRenderThread()) {
        SEDX_CORE_INFO("Executing on render thread - context valid");
    } else {
        SEDX_CORE_WARN("Not on render thread - submit to renderer instead");
    }
}

// Performance-critical path
void OptimizedRenderOperation() {
    // Fast path: already on render thread
    if (ThreadManager::checkRenderThread()) {
        ExecuteRenderCommand();
        return;
    }
  
    // Slow path: submit to render thread
    Renderer::Submit([]() {
        ExecuteRenderCommand();
    });
}
```

### Run Implementation

```cpp
void ThreadManager::Run()
{
    m_isRunning = true;
    if (m_policy == ThreadingPolicy::MultiThreaded)
        renderThread.Dispatch(Renderer::RenderThreadFunc, this);

    s_threadID = renderThread.GetThreadID();
}
```

**Implementation Breakdown**:

1. **State Activation**:

   ```cpp
   m_isRunning = true;
   ```

   - Sets running flag for system activation
   - Enables other methods to function correctly
   - Atomic bool ensures thread-safe access
2. **Conditional Thread Creation**:

   ```cpp
   if (m_policy == ThreadingPolicy::MultiThreaded)
       renderThread.Dispatch(Renderer::RenderThreadFunc, this);
   ```

   - Only creates actual thread for MultiThreaded policy
   - Passes `this` pointer to render function for coordination
   - Uses Thread::Dispatch for proper thread initialization
3. **Thread ID Registration**:

   ```cpp
   s_threadID = renderThread.GetThreadID();
   ```

   - Registers render thread for `checkRenderThread()` functionality
   - Thread-safe assignment operation
   - Works correctly for all threading policies

**Threading Policy Behavior**:

- **MultiThreaded**: Creates dedicated render thread
- **SingleThreaded/None**: No actual thread creation, ID still registered for consistency

**Error Handling Enhancement**:

```cpp
void ThreadManager::Run()
{
    if (m_isRunning) {
        SEDX_CORE_WARN("ThreadManager::Run() called on already running system");
        return;
    }
  
    try {
        m_isRunning = true;
    
        if (m_policy == ThreadingPolicy::MultiThreaded) {
            renderThread.Dispatch(Renderer::RenderThreadFunc, this);
        }
    
        s_threadID = renderThread.GetThreadID();
        SEDX_CORE_INFO("ThreadManager started successfully with policy: {}", ThreadingPolicyToString(m_policy));
    
    } catch (const std::exception& e) {
        m_isRunning = false;
        SEDX_CORE_ERROR("Failed to start ThreadManager: {}", e.what());
        throw;
    }
}
```

### Terminate Implementation

```cpp
void ThreadManager::Terminate()
{
    m_isRunning = false;
    Pump();

    if (m_policy == ThreadingPolicy::MultiThreaded)
        renderThread.Join();

    s_threadID = std::thread::id();
}
```

**Implementation Analysis**:

1. **State Deactivation**:

   ```cpp
   m_isRunning = false;
   ```

   - Signals render thread to exit its main loop
   - Atomic operation ensures immediate visibility to render thread
2. **Final Frame Processing**:

   ```cpp
   Pump();
   ```

   - Ensures render thread processes the termination signal
   - Advances frame counter and triggers render thread wake-up
   - Provides clean shutdown opportunity
3. **Thread Synchronization**:

   ```cpp
   if (m_policy == ThreadingPolicy::MultiThreaded)
       renderThread.Join();
   ```

   - Waits for render thread to complete current work and exit
   - Only joins actual threads (MultiThreaded policy)
   - Ensures clean resource cleanup
4. **Thread ID Cleanup**:

   ```cpp
   s_threadID = std::thread::id();
   ```

   - Resets global thread tracking
   - Ensures `checkRenderThread()` returns false after termination

**Shutdown Timing Considerations**:

```cpp
// Enhanced termination with timeout
bool ThreadManager::TerminateWithTimeout(std::chrono::milliseconds timeout)
{
    m_isRunning = false;
    Pump();  // Signal termination
  
    if (m_policy == ThreadingPolicy::MultiThreaded) {
        // Implementation would need async join with timeout
        auto future = std::async(std::launch::async, [this]() {
            renderThread.Join();
        });
    
        if (future.wait_for(timeout) == std::future_status::timeout) {
            SEDX_CORE_ERROR("Render thread failed to terminate within timeout");
            return false;
        }
    }
  
    s_threadID = std::thread::id();
    return true;
}
```

### State Management Implementation

#### Wait Implementation

```cpp
void ThreadManager::Wait(State waitForState)
{
    if (m_policy == ThreadingPolicy::SingleThreaded)
        return;

    EnterCriticalSection(&m_Data->m_CriticalSection);
    while (m_Data->m_State != waitForState)
    {
        // This releases the CS so that another thread can wake it
        SleepConditionVariableCS(&m_Data->m_ConditionVariable, &m_Data->m_CriticalSection, INFINITE);
    }
    LeaveCriticalSection(&m_Data->m_CriticalSection);
}
```

**Implementation Breakdown**:

1. **Policy Check**:

   ```cpp
   if (m_policy == ThreadingPolicy::SingleThreaded)
       return;
   ```

   - Early return for single-threaded mode
   - Avoids unnecessary synchronization overhead
   - Maintains API consistency across policies
2. **Critical Section Entry**:

   ```cpp
   EnterCriticalSection(&m_Data->m_CriticalSection);
   ```

   - Acquires exclusive access to state
   - Prevents race conditions during state checking
   - Provides memory barrier for state visibility
3. **Condition Wait Loop**:

   ```cpp
   while (m_Data->m_State != waitForState)
   {
       SleepConditionVariableCS(&m_Data->m_ConditionVariable, &m_Data->m_CriticalSection, INFINITE);
   }
   ```

   - **While Loop**: Handles spurious wakeups correctly
   - **Atomic Wait**: `SleepConditionVariableCS` atomically releases critical section and waits
   - **INFINITE Timeout**: Waits indefinitely until condition is met
   - **Automatic Reacquisition**: Critical section automatically reacquired on wake
4. **Critical Section Exit**:

   ```cpp
   LeaveCriticalSection(&m_Data->m_CriticalSection);
   ```

   - Releases exclusive access to state
   - Allows other threads to modify state
   - Provides memory barrier for subsequent operations

**Performance Characteristics**:

- **Uncontended**: Extremely fast when state already matches
- **Contended**: Efficient blocking with minimal CPU usage
- **Spurious Wakeups**: Handled correctly without user intervention
- **Memory Ordering**: Full barrier semantics

#### WaitAndSet Implementation

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

**Atomic Operation Analysis**:

1. **Wait Phase** (identical to `Wait()`):

   - Same implementation as standalone Wait method
   - Ensures target state is reached before proceeding
2. **Set Phase**:

   ```cpp
   m_Data->m_State = setToState;
   WakeAllConditionVariable(&m_Data->m_ConditionVariable);
   ```

   - **State Assignment**: Atomically updates state under critical section
   - **Notification**: Wakes all waiting threads about state change
   - **Atomicity**: Both operations protected by same critical section

**Atomicity Guarantee**:

- The wait and set operations are guaranteed to be atomic
- No other thread can observe intermediate states
- State change and notification are indivisible

**Usage Scenarios**:

```cpp
// Frame boundary coordination
manager.WaitAndSet(ThreadManager::State::Idle, ThreadManager::State::Kick);

// Pipeline stage transitions
void TransitionPipelineStage(ThreadManager& manager, PipelineStage from, PipelineStage to) {
    State fromState = PipelineStageToState(from);
    State toState = PipelineStageToState(to);
    manager.WaitAndSet(fromState, toState);
}
```

#### Set Implementation

```cpp
void ThreadManager::Set(State setToState)
{
    if (m_policy == ThreadingPolicy::SingleThreaded)
        return;

    EnterCriticalSection(&m_Data->m_CriticalSection);
    m_Data->m_State = setToState;
    WakeAllConditionVariable(&m_Data->m_ConditionVariable);
    LeaveCriticalSection(&m_Data->m_CriticalSection);
}
```

**Implementation Details**:

1. **Immediate State Change**:

   - No waiting phase, directly modifies state
   - Fastest synchronization operation
   - Non-blocking for calling thread
2. **Thread Notification**:

   ```cpp
   WakeAllConditionVariable(&m_Data->m_ConditionVariable);
   ```

   - Wakes all threads waiting on the condition variable
   - Ensures immediate response to state changes
   - Handles multiple waiters correctly

**Performance Profile**:

- **Minimal Latency**: Direct state change without waiting
- **Broad Notification**: All waiting threads notified
- **Low Overhead**: Single critical section acquisition

### High-Level Coordination Implementation

#### NextFrame Implementation

```cpp
void ThreadManager::NextFrame()
{
    ++appThreadFrame;
    Renderer::SwapQueues();
}
```

**Purpose**: Advances frame synchronization and swaps render command queues.

**Implementation Details**:

1. **Frame Counter Increment**:

   ```cpp
   ++appThreadFrame;
   ```

   - Atomic increment of frame counter
   - Tracks application thread frame progression
   - Used for synchronization validation
2. **Queue Management**:

   ```cpp
   Renderer::SwapQueues();
   ```

   - Swaps double-buffered command queues
   - Allows application to record new commands while render thread processes previous
   - Critical for avoiding render thread stalls

**Thread Safety**:

- Frame counter is atomic, safe from all threads
- SwapQueues() must be thread-safe or called from correct thread context

#### BlockUntilRenderComplete Implementation

```cpp
void ThreadManager::BlockUntilRenderComplete()
{
    if (m_policy == ThreadingPolicy::SingleThreaded)
        return;

    Wait(State::Idle);
}
```

**Simplicity**: Direct delegation to Wait() with Idle state.

**Policy Handling**: Early return for single-threaded mode maintains performance.

#### Kick Implementation

```cpp
void ThreadManager::Kick()
{
    if (m_policy == ThreadingPolicy::MultiThreaded)
        Set(State::Kick);
    else
        Renderer::WaitAndRender(this);
}
```

**Policy-Specific Behavior**:

1. **MultiThreaded**:

   ```cpp
   Set(State::Kick);
   ```

   - Signals render thread to start processing
   - Non-blocking operation
   - Render thread processes asynchronously
2. **SingleThreaded/None**:

   ```cpp
   Renderer::WaitAndRender(this);
   ```

   - Direct synchronous execution
   - Blocks until rendering completes
   - No thread coordination needed

#### Pump Implementation

```cpp
void ThreadManager::Pump()
{
    NextFrame();
    Kick();
    BlockUntilRenderComplete();
}
```

**Complete Frame Cycle**: Combines all frame operations into single call.

**Sequence Analysis**:

1. `NextFrame()`: Advances frame counter and swaps queues
2. `Kick()`: Initiates render processing
3. `BlockUntilRenderComplete()`: Waits for completion

**Usage Pattern**: Ideal for simple main loops requiring complete frame synchronization.

## Duplicate Implementation Issue

### GetThreadID Duplicate

```cpp
std::thread::id Thread::GetThreadID() const
{
    return mem_thread.get_id();
}
```

**Issue**: This method is implemented in both `thread.cpp` and `thread_manager.cpp`.

**Solution**: Remove from `thread_manager.cpp` as it belongs in `thread.cpp`.

## Performance Optimization Opportunities

### Enhanced State Management

```cpp
// Optimized state management with minimal locking
class OptimizedThreadManager {
private:
    std::atomic<ThreadManager::State> m_atomicState{ThreadManager::State::Idle};
    CONDITION_VARIABLE m_conditionVariable;
    CRITICAL_SECTION m_criticalSection;
  
public:
    void SetAtomic(State newState) {
        m_atomicState.store(newState);
        WakeAllConditionVariable(&m_conditionVariable);
    }
  
    void WaitAtomic(State waitForState) {
        while (m_atomicState.load() != waitForState) {
            EnterCriticalSection(&m_criticalSection);
            if (m_atomicState.load() != waitForState) {
                SleepConditionVariableCS(&m_conditionVariable, &m_criticalSection, INFINITE);
            }
            LeaveCriticalSection(&m_criticalSection);
        }
    }
};
```

### Lock-Free Frame Counter

```cpp
// Lock-free frame coordination
class LockFreeFrameCounter {
private:
    std::atomic<uint64_t> m_appFrame{0};
    std::atomic<uint64_t> m_renderFrame{0};
  
public:
    void AdvanceAppFrame() {
        m_appFrame.fetch_add(1, std::memory_order_release);
    }
  
    bool HasRenderWork() const {
        return m_renderFrame.load(std::memory_order_acquire) < m_appFrame.load(std::memory_order_acquire);
    }
  
    void CompleteRenderFrame() {
        m_renderFrame.fetch_add(1, std::memory_order_release);
    }
};
```

## Error Handling Improvements

### Resource Cleanup

```cpp
// RAII wrapper for critical section
class CriticalSectionRAII {
private:
    CRITICAL_SECTION* m_cs;
  
public:
    explicit CriticalSectionRAII(CRITICAL_SECTION* cs) : m_cs(cs) {
        EnterCriticalSection(m_cs);
    }
  
    ~CriticalSectionRAII() {
        LeaveCriticalSection(m_cs);
    }
  
    // Non-copyable, non-movable
    CriticalSectionRAII(const CriticalSectionRAII&) = delete;
    CriticalSectionRAII& operator=(const CriticalSectionRAII&) = delete;
};

// Usage in thread manager methods
void ThreadManager::SafeWait(State waitForState) {
    if (m_policy == ThreadingPolicy::SingleThreaded)
        return;
    
    CriticalSectionRAII lock(&m_Data->m_CriticalSection);
    while (m_Data->m_State != waitForState) {
        SleepConditionVariableCS(&m_Data->m_ConditionVariable, &m_Data->m_CriticalSection, INFINITE);
    }
    // Automatic unlock via RAII
}
```

### Exception Safety

```cpp
// Exception-safe thread management
class ExceptionSafeThreadManager {
public:
    void SafeRun() {
        try {
            Run();
        } catch (const std::exception& e) {
            SEDX_CORE_ERROR("ThreadManager::Run() failed: {}", e.what());
            Cleanup();
            throw;
        }
    }
  
private:
    void Cleanup() {
        if (m_isRunning) {
            m_isRunning = false;
            // Emergency cleanup without blocking
        }
    }
};
```

## Integration Examples

### Vulkan Command Buffer Coordination

```cpp
class VulkanCommandCoordinator {
private:
    ThreadManager& m_threadManager;
    VkCommandBuffer m_appCommandBuffer;
    VkCommandBuffer m_renderCommandBuffer;
  
public:
    void RecordAndSubmit() {
        // Application thread records commands
        RecordApplicationCommands(m_appCommandBuffer);
    
        // Coordinate with render thread
        m_threadManager.NextFrame();  // Swap command buffers
        m_threadManager.Kick();       // Start render thread processing
    
        // Render thread processes while app prepares next frame
        PrepareNextFrameData();
    
        // Wait for render completion before present
        m_threadManager.BlockUntilRenderComplete();
    
        PresentFrame();
    }
};
```

### Asset Loading Coordination

```cpp
class AssetLoadingCoordinator {
private:
    ThreadManager& m_threadManager;
  
public:
    void LoadAssetsForFrame() {
        // Ensure render thread is idle before loading
        m_threadManager.Wait(ThreadManager::State::Idle);
    
        // Load assets safely
        LoadPendingAssets();
    
        // Signal render thread to continue
        m_threadManager.Set(ThreadManager::State::Kick);
    }
};
```

## Best Practices for GitHub Copilot

### 1. Proper Policy Checking

```cpp
// PREFERRED - Always check policy before synchronization operations
void SafeStateOperation(ThreadManager& manager) {
    if (manager.GetPolicy() == ThreadingPolicy::MultiThreaded) {
        manager.Wait(ThreadManager::State::Idle);
    }
    // Perform operation
}

// AVOID - Synchronization without policy check
manager.Wait(ThreadManager::State::Idle);  // May be no-op for single-threaded
```

### 2. Exception-Safe Resource Management

```cpp
// PREFERRED - RAII for automatic cleanup
{
    CriticalSectionRAII lock(&criticalSection);
    PerformCriticalOperation();
}  // Automatic unlock

// AVOID - Manual lock management
EnterCriticalSection(&criticalSection);
PerformCriticalOperation();
LeaveCriticalSection(&criticalSection);  // Could be skipped on exception
```

### 3. Atomic Operations for Performance

```cpp
// PREFERRED - Use atomic operations where appropriate
std::atomic<bool> frameReady{false};
if (frameReady.load(std::memory_order_acquire)) {
    ProcessFrame();
}

// AVOID - Unnecessary synchronization for simple flags
EnterCriticalSection(&cs);
bool ready = frameReady;
LeaveCriticalSection(&cs);
```

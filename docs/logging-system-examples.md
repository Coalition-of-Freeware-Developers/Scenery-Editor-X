# Scenery Editor X Logging System - Examples and Use Cases

## Table of Contents
1. [Basic Usage Examples](#basic-usage-examples)
2. [Advanced Patterns](#advanced-patterns)
3. [Subsystem-Specific Examples](#subsystem-specific-examples)
4. [Performance and Debugging](#performance-and-debugging)
5. [Integration Patterns](#integration-patterns)
6. [Error Handling Examples](#error-handling-examples)
7. [Configuration Examples](#configuration-examples)
8. [Real-World Scenarios](#real-world-scenarios)

## Basic Usage Examples

### Simple Information Logging
```cpp
void InitializeRenderer() {
    SEDX_CORE_INFO_TAG("Renderer", "Initializing Vulkan renderer");
    
    // Initialize Vulkan components...
    
    SEDX_CORE_INFO_TAG("Renderer", "Vulkan renderer initialized successfully");
    SEDX_CORE_INFO_TAG("Renderer", "GPU: {}", gpuName);
    SEDX_CORE_INFO_TAG("Renderer", "Driver version: {}", driverVersion);
}
```

### Warning and Error Reporting
```cpp
bool LoadTexture(const std::string& path) {
    SEDX_CORE_TRACE_TAG("AssetManager", "Loading texture: {}", path);
    
    if (!std::filesystem::exists(path)) {
        SEDX_CORE_WARN_TAG("AssetManager", "Texture file not found: {}", path);
        SEDX_CORE_INFO_TAG("AssetManager", "Using default texture instead");
        return LoadDefaultTexture();
    }
    
    auto result = LoadTextureFromFile(path);
    if (!result.success) {
        SEDX_CORE_ERROR_TAG("AssetManager", "Failed to load texture: {} - {}", 
                           path, result.errorMessage);
        return false;
    }
    
    SEDX_CORE_INFO_TAG("AssetManager", "Successfully loaded texture: {} ({}x{}, {} bytes)",
                       path, result.width, result.height, result.dataSize);
    return true;
}
```

### Editor UI Logging
```cpp
void OnMenuItemClicked(const std::string& menuItem) {
    EDITOR_TRACE_TAG("UI", "Menu item clicked: {}", menuItem);
    
    if (menuItem == "New Project") {
        EDITOR_INFO_TAG("Project", "Creating new project");
        CreateNewProject();
    } else if (menuItem == "Save") {
        if (HasUnsavedChanges()) {
            EDITOR_INFO_TAG("Project", "Saving project changes");
            SaveProject();
        } else {
            EDITOR_TRACE_TAG("Project", "No changes to save");
        }
    }
}
```

## Advanced Patterns

### Conditional Expensive Logging
```cpp
void ProcessMeshData(const MeshData& mesh) {
    SEDX_CORE_TRACE_TAG("Mesh", "Processing mesh with {} vertices", mesh.vertices.size());
    
    // Only generate expensive debug info if detailed mesh logging is enabled
    if (Log::HasTag("MeshDetails") && 
        Log::EnabledTags()["MeshDetails"].LevelFilter <= Log::Level::Trace) {
        
        auto boundingBox = CalculateBoundingBox(mesh);
        auto surfaceArea = CalculateSurfaceArea(mesh);
        std::string materialInfo = GenerateMaterialReport(mesh.materials);
        
        SEDX_CORE_TRACE_TAG("MeshDetails", "Mesh analysis: BB({},{},{}) to ({},{},{}), Area: {}, Materials: {}",
                           boundingBox.min.x, boundingBox.min.y, boundingBox.min.z,
                           boundingBox.max.x, boundingBox.max.y, boundingBox.max.z,
                           surfaceArea, materialInfo);
    }
    
    // Regular processing continues...
}
```

### Performance Monitoring
```cpp
class PerformanceTimer {
private:
    std::chrono::high_resolution_clock::time_point startTime;
    std::string operation;
    std::string tag;

public:
    PerformanceTimer(const std::string& op, const std::string& logTag = "Performance") 
        : operation(op), tag(logTag) {
        if (Log::HasTag(tag)) {
            startTime = std::chrono::high_resolution_clock::now();
            SEDX_CORE_TRACE_TAG(tag, "Starting: {}", operation);
        }
    }
    
    ~PerformanceTimer() {
        if (Log::HasTag(tag)) {
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
            SEDX_CORE_TRACE_TAG(tag, "Completed: {} in {} microseconds", operation, duration.count());
        }
    }
};

// Usage:
void RenderFrame() {
    PerformanceTimer timer("Frame Render");
    
    // Render operations...
    {
        PerformanceTimer drawTimer("Draw Calls", "Renderer");
        SubmitDrawCalls();
    }
    
    {
        PerformanceTimer presentTimer("Present", "Renderer");
        PresentFrame();
    }
}
```

### State Change Logging
```cpp
class ApplicationState {
public:
    enum class State {
        Initializing,
        Running,
        Paused,
        Shutting_Down
    };

private:
    State currentState = State::Initializing;

public:
    void ChangeState(State newState) {
        if (currentState == newState) {
            SEDX_CORE_TRACE_TAG("Application", "State change request ignored: already in {}",
                               StateToString(newState));
            return;
        }
        
        SEDX_CORE_INFO_TAG("Application", "State transition: {} -> {}",
                          StateToString(currentState), StateToString(newState));
        
        // Validate state transition
        if (!IsValidTransition(currentState, newState)) {
            SEDX_CORE_ERROR_TAG("Application", "Invalid state transition: {} -> {}",
                               StateToString(currentState), StateToString(newState));
            return;
        }
        
        State oldState = currentState;
        currentState = newState;
        
        // Log any important side effects
        OnStateChanged(oldState, newState);
    }
    
private:
    void OnStateChanged(State from, State to) {
        if (to == State::Paused) {
            SEDX_CORE_INFO_TAG("Application", "Application paused - saving auto-recovery data");
        } else if (from == State::Paused && to == State::Running) {
            SEDX_CORE_INFO_TAG("Application", "Application resumed from pause");
        } else if (to == State::Shutting_Down) {
            SEDX_CORE_INFO_TAG("Application", "Beginning shutdown sequence");
        }
    }
};
```

## Subsystem-Specific Examples

### Vulkan Graphics Logging
```cpp
class VulkanDevice {
public:
    bool CreateLogicalDevice() {
        SEDX_CORE_INFO_TAG("Vulkan", "Creating logical device");
        
        // Query device features
        VkPhysicalDeviceFeatures deviceFeatures{};
        vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
        
        if (!deviceFeatures.samplerAnisotropy) {
            SEDX_CORE_WARN_TAG("Vulkan", "Anisotropic filtering not supported");
        }
        
        // Create device
        VkDeviceCreateInfo createInfo{};
        // ... setup createInfo ...
        
        VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
        if (result != VK_SUCCESS) {
            SEDX_CORE_ERROR_TAG("Vulkan", "Failed to create logical device: {}", VkResultToString(result));
            return false;
        }
        
        SEDX_CORE_INFO_TAG("Vulkan", "Logical device created successfully");
        
        // Log device capabilities
        LogDeviceCapabilities();
        
        return true;
    }
    
private:
    void LogDeviceCapabilities() {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        
        SEDX_CORE_INFO_TAG("Vulkan", "Device: {}", properties.deviceName);
        SEDX_CORE_INFO_TAG("Vulkan", "API Version: {}.{}.{}",
                          VK_VERSION_MAJOR(properties.apiVersion),
                          VK_VERSION_MINOR(properties.apiVersion),
                          VK_VERSION_PATCH(properties.apiVersion));
        SEDX_CORE_INFO_TAG("Vulkan", "Driver Version: {}", properties.driverVersion);
        SEDX_CORE_INFO_TAG("Vulkan", "Max Texture Size: {}", properties.limits.maxImageDimension2D);
        
        // Log memory heaps
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        
        for (uint32_t i = 0; i < memProperties.memoryHeapCount; i++) {
            const auto& heap = memProperties.memoryHeaps[i];
            SEDX_CORE_INFO_TAG("Vulkan", "Memory Heap {}: {} MB {}",
                              i, heap.size / (1024 * 1024),
                              (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) ? "(Device Local)" : "(Host)");
        }
    }
};
```

### Asset Management Logging
```cpp
class AssetManager {
public:
    template<typename AssetType>
    std::shared_ptr<AssetType> LoadAsset(const std::string& path) {
        SEDX_CORE_TRACE_TAG("AssetManager", "LoadAsset<{}> requested: {}", 
                           typeid(AssetType).name(), path);
        
        // Check cache first
        auto cached = GetFromCache<AssetType>(path);
        if (cached) {
            SEDX_CORE_TRACE_TAG("AssetManager", "Asset found in cache: {}", path);
            return cached;
        }
        
        SEDX_CORE_INFO_TAG("AssetManager", "Loading asset from disk: {}", path);
        
        auto startTime = std::chrono::high_resolution_clock::now();
        auto asset = LoadAssetFromDisk<AssetType>(path);
        auto endTime = std::chrono::high_resolution_clock::now();
        auto loadTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        if (!asset) {
            SEDX_CORE_ERROR_TAG("AssetManager", "Failed to load asset: {}", path);
            return nullptr;
        }
        
        // Log loading statistics
        size_t memoryUsed = asset->GetMemoryUsage();
        SEDX_CORE_INFO_TAG("AssetManager", "Asset loaded: {} ({} KB in {} ms)",
                          path, memoryUsed / 1024, loadTime.count());
        
        // Add to cache
        AddToCache(path, asset);
        LogCacheStatistics();
        
        return asset;
    }
    
private:
    void LogCacheStatistics() {
        if (Log::HasTag("AssetCache")) {
            size_t totalMemory = 0;
            size_t assetCount = 0;
            
            for (const auto& [path, asset] : assetCache) {
                totalMemory += asset->GetMemoryUsage();
                assetCount++;
            }
            
            SEDX_CORE_TRACE_TAG("AssetCache", "Cache stats: {} assets, {} MB total",
                               assetCount, totalMemory / (1024 * 1024));
        }
    }
};
```

### Memory Management Logging
```cpp
class MemoryAllocator {
public:
    void* Allocate(size_t size, const std::string& category = "General") {
        SEDX_CORE_TRACE_TAG("Memory", "Allocating {} bytes for category: {}", size, category);
        
        void* ptr = malloc(size);
        if (!ptr) {
            SEDX_CORE_ERROR_TAG("Memory", "Failed to allocate {} bytes for category: {}", 
                               size, category);
            LogMemoryStatistics();
            return nullptr;
        }
        
        // Track allocation
        std::lock_guard<std::mutex> lock(allocationMutex);
        allocations[ptr] = {size, category, std::chrono::steady_clock::now()};
        totalAllocated += size;
        categoryTotals[category] += size;
        
        SEDX_CORE_TRACE_TAG("Memory", "Allocated {} bytes at {} for category: {} (total: {} MB)",
                           size, ptr, category, totalAllocated / (1024 * 1024));
        
        return ptr;
    }
    
    void Deallocate(void* ptr) {
        if (!ptr) return;
        
        std::lock_guard<std::mutex> lock(allocationMutex);
        auto it = allocations.find(ptr);
        if (it == allocations.end()) {
            SEDX_CORE_ERROR_TAG("Memory", "Attempting to deallocate untracked pointer: {}", ptr);
            return;
        }
        
        const auto& info = it->second;
        auto lifetime = std::chrono::steady_clock::now() - info.allocTime;
        auto lifetimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(lifetime);
        
        SEDX_CORE_TRACE_TAG("Memory", "Deallocating {} bytes at {} from category: {} (lifetime: {} ms)",
                           info.size, ptr, info.category, lifetimeMs.count());
        
        totalAllocated -= info.size;
        categoryTotals[info.category] -= info.size;
        allocations.erase(it);
        
        free(ptr);
    }
    
    void LogMemoryStatistics() {
        SEDX_CORE_INFO_TAG("Memory", "=== Memory Statistics ===");
        SEDX_CORE_INFO_TAG("Memory", "Total allocated: {} MB", totalAllocated / (1024 * 1024));
        SEDX_CORE_INFO_TAG("Memory", "Active allocations: {}", allocations.size());
        
        for (const auto& [category, total] : categoryTotals) {
            if (total > 0) {
                SEDX_CORE_INFO_TAG("Memory", "Category '{}': {} MB", 
                                  category, total / (1024 * 1024));
            }
        }
        
        // Check for memory leaks in debug builds
        #ifdef SEDX_DEBUG
        if (!allocations.empty()) {
            SEDX_CORE_WARN_TAG("Memory", "Potential memory leaks detected:");
            for (const auto& [ptr, info] : allocations) {
                auto lifetime = std::chrono::steady_clock::now() - info.allocTime;
                auto lifetimeMin = std::chrono::duration_cast<std::chrono::minutes>(lifetime);
                
                if (lifetimeMin.count() > 5) { // Allocations older than 5 minutes
                    SEDX_CORE_WARN_TAG("Memory", "Long-lived allocation: {} bytes at {} ({} minutes old)",
                                      info.size, ptr, lifetimeMin.count());
                }
            }
        }
        #endif
    }
};
```

## Performance and Debugging

### Frame Rate Monitoring
```cpp
class FrameRateMonitor {
private:
    std::chrono::steady_clock::time_point lastFrameTime;
    std::chrono::steady_clock::time_point lastLogTime;
    std::vector<float> frameTimes;
    size_t frameCount = 0;
    
public:
    void BeginFrame() {
        auto currentTime = std::chrono::steady_clock::now();
        
        if (frameCount > 0) {
            auto frameDuration = currentTime - lastFrameTime;
            float frameTimeMs = std::chrono::duration<float, std::milli>(frameDuration).count();
            frameTimes.push_back(frameTimeMs);
            
            // Keep only last 60 frames for rolling average
            if (frameTimes.size() > 60) {
                frameTimes.erase(frameTimes.begin());
            }
        }
        
        lastFrameTime = currentTime;
        frameCount++;
        
        // Log statistics every second
        auto timeSinceLastLog = currentTime - lastLogTime;
        if (std::chrono::duration_cast<std::chrono::seconds>(timeSinceLastLog).count() >= 1) {
            LogFrameStatistics();
            lastLogTime = currentTime;
        }
    }
    
private:
    void LogFrameStatistics() {
        if (frameTimes.empty()) return;
        
        float avgFrameTime = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0f) / frameTimes.size();
        float avgFPS = 1000.0f / avgFrameTime;
        
        float minFrameTime = *std::min_element(frameTimes.begin(), frameTimes.end());
        float maxFrameTime = *std::max_element(frameTimes.begin(), frameTimes.end());
        
        SEDX_CORE_TRACE_TAG("Performance", "FPS: {:.1f} avg, Frame Time: {:.2f}ms avg ({:.2f}-{:.2f}ms range)",
                           avgFPS, avgFrameTime, minFrameTime, maxFrameTime);
        
        // Warn about performance issues
        if (avgFPS < 30.0f) {
            SEDX_CORE_WARN_TAG("Performance", "Low frame rate detected: {:.1f} FPS", avgFPS);
        }
        
        if (maxFrameTime > 50.0f) {
            SEDX_CORE_WARN_TAG("Performance", "Frame spike detected: {:.2f}ms", maxFrameTime);
        }
    }
};
```

### Resource Usage Monitoring
```cpp
class ResourceMonitor {
public:
    void LogResourceUsage() {
        // CPU usage
        float cpuUsage = GetCPUUsage();
        SEDX_CORE_TRACE_TAG("Resources", "CPU Usage: {:.1f}%", cpuUsage);
        
        if (cpuUsage > 80.0f) {
            SEDX_CORE_WARN_TAG("Resources", "High CPU usage detected: {:.1f}%", cpuUsage);
        }
        
        // Memory usage
        auto memInfo = GetMemoryInfo();
        float memUsagePercent = (float)memInfo.usedMemory / memInfo.totalMemory * 100.0f;
        
        SEDX_CORE_TRACE_TAG("Resources", "Memory Usage: {} MB / {} MB ({:.1f}%)",
                           memInfo.usedMemory / (1024 * 1024),
                           memInfo.totalMemory / (1024 * 1024),
                           memUsagePercent);
        
        if (memUsagePercent > 85.0f) {
            SEDX_CORE_WARN_TAG("Resources", "High memory usage: {:.1f}%", memUsagePercent);
        }
        
        // GPU memory (Vulkan)
        LogGPUMemoryUsage();
    }
    
private:
    void LogGPUMemoryUsage() {
        // Query Vulkan memory usage
        VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
        vmaGetBudget(vmaAllocator, budgets);
        
        for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; i++) {
            const auto& heap = memoryProperties.memoryHeaps[i];
            const auto& budget = budgets[i];
            
            float usagePercent = (float)budget.usage / heap.size * 100.0f;
            
            SEDX_CORE_TRACE_TAG("GPU", "GPU Heap {}: {} MB / {} MB ({:.1f}%)",
                               i, budget.usage / (1024 * 1024),
                               heap.size / (1024 * 1024), usagePercent);
            
            if (usagePercent > 90.0f) {
                SEDX_CORE_WARN_TAG("GPU", "GPU memory heap {} nearly full: {:.1f}%", i, usagePercent);
            }
        }
    }
};
```

## Integration Patterns

### Error Recovery Logging
```cpp
class NetworkManager {
public:
    bool ConnectToServer(const std::string& address, int port) {
        int attemptCount = 0;
        const int maxAttempts = 3;
        
        while (attemptCount < maxAttempts) {
            attemptCount++;
            
            SEDX_CORE_INFO_TAG("Network", "Connection attempt {} to {}:{}", 
                              attemptCount, address, port);
            
            if (AttemptConnection(address, port)) {
                SEDX_CORE_INFO_TAG("Network", "Successfully connected to {}:{} on attempt {}",
                                  address, port, attemptCount);
                return true;
            }
            
            SEDX_CORE_WARN_TAG("Network", "Connection attempt {} failed to {}:{}",
                              attemptCount, address, port);
            
            if (attemptCount < maxAttempts) {
                int delaySeconds = attemptCount * 2; // Exponential backoff
                SEDX_CORE_INFO_TAG("Network", "Retrying connection in {} seconds", delaySeconds);
                std::this_thread::sleep_for(std::chrono::seconds(delaySeconds));
            }
        }
        
        SEDX_CORE_ERROR_TAG("Network", "Failed to connect to {}:{} after {} attempts",
                           address, port, maxAttempts);
        return false;
    }
};
```

### Transaction Logging
```cpp
class SceneSerializer {
public:
    bool SaveScene(const Scene& scene, const std::string& filepath) {
        std::string transactionId = GenerateTransactionId();
        
        SEDX_CORE_INFO_TAG("Serialization", "Starting scene save transaction: {} -> {}",
                          transactionId, filepath);
        
        try {
            // Create backup
            std::string backupPath = filepath + ".backup";
            if (std::filesystem::exists(filepath)) {
                SEDX_CORE_TRACE_TAG("Serialization", "Creating backup: {}", backupPath);
                std::filesystem::copy_file(filepath, backupPath);
            }
            
            // Serialize scene
            SEDX_CORE_TRACE_TAG("Serialization", "Serializing {} objects", scene.GetObjectCount());
            
            auto data = SerializeScene(scene);
            
            SEDX_CORE_TRACE_TAG("Serialization", "Writing {} bytes to file", data.size());
            
            // Write to temporary file first
            std::string tempPath = filepath + ".tmp";
            WriteToFile(tempPath, data);
            
            // Atomic rename
            std::filesystem::rename(tempPath, filepath);
            
            // Remove backup on success
            if (std::filesystem::exists(backupPath)) {
                std::filesystem::remove(backupPath);
            }
            
            SEDX_CORE_INFO_TAG("Serialization", "Scene save transaction completed: {}", transactionId);
            return true;
            
        } catch (const std::exception& e) {
            SEDX_CORE_ERROR_TAG("Serialization", "Scene save transaction failed: {} - {}",
                               transactionId, e.what());
            
            // Cleanup temporary files
            std::string tempPath = filepath + ".tmp";
            if (std::filesystem::exists(tempPath)) {
                std::filesystem::remove(tempPath);
                SEDX_CORE_TRACE_TAG("Serialization", "Cleaned up temporary file: {}", tempPath);
            }
            
            return false;
        }
    }
};
```

## Configuration Examples

### Runtime Tag Configuration
```cpp
class LoggingConfig {
public:
    void LoadConfiguration(const std::string& configFile) {
        SEDX_CORE_INFO_TAG("Config", "Loading logging configuration from: {}", configFile);
        
        try {
            auto config = ParseConfigFile(configFile);
            
            // Clear existing tags and load from config
            auto& tags = Log::EnabledTags();
            tags.clear();
            
            for (const auto& [tagName, tagConfig] : config.tags) {
                Log::TagDetails details;
                details.Enabled = tagConfig.enabled;
                details.LevelFilter = Log::LevelFromString(tagConfig.level);
                
                tags[tagName] = details;
                
                SEDX_CORE_TRACE_TAG("Config", "Configured tag '{}': enabled={}, level={}",
                                   tagName, details.Enabled, Log::LevelToString(details.LevelFilter));
            }
            
            SEDX_CORE_INFO_TAG("Config", "Loaded configuration for {} tags", tags.size());
            
        } catch (const std::exception& e) {
            SEDX_CORE_ERROR_TAG("Config", "Failed to load logging configuration: {}", e.what());
            SEDX_CORE_INFO_TAG("Config", "Using default logging configuration");
            Log::SetDefaultTagSettings();
        }
    }
    
    void SaveConfiguration(const std::string& configFile) {
        SEDX_CORE_INFO_TAG("Config", "Saving logging configuration to: {}", configFile);
        
        try {
            ConfigData config;
            
            for (const auto& [tagName, details] : Log::EnabledTags()) {
                TagConfig tagConfig;
                tagConfig.enabled = details.Enabled;
                tagConfig.level = Log::LevelToString(details.LevelFilter);
                config.tags[tagName] = tagConfig;
            }
            
            WriteConfigFile(configFile, config);
            SEDX_CORE_INFO_TAG("Config", "Successfully saved logging configuration");
            
        } catch (const std::exception& e) {
            SEDX_CORE_ERROR_TAG("Config", "Failed to save logging configuration: {}", e.what());
        }
    }
};
```

### Development vs Release Configuration
```cpp
void ConfigureLoggingForBuild() {
    auto& tags = Log::EnabledTags();
    
    #ifdef SEDX_DEBUG
        // Debug build - verbose logging
        SEDX_CORE_INFO("Configuring debug build logging");
        
        tags["Renderer"] = {true, Log::Level::Trace};
        tags["Memory"] = {true, Log::Level::Trace};
        tags["AssetManager"] = {true, Log::Level::Trace};
        tags["Performance"] = {true, Log::Level::Trace};
        tags["Animation"] = {true, Log::Level::Trace};
        tags["Physics"] = {true, Log::Level::Trace};
        
        // Enable detailed subsystem logging
        tags["Vulkan"] = {true, Log::Level::Trace};
        tags["FileSystem"] = {true, Log::Level::Trace};
        tags["Serialization"] = {true, Log::Level::Trace};
        
    #elif defined(SEDX_DEVELOPMENT)
        // Development build - moderate logging
        SEDX_CORE_INFO("Configuring development build logging");
        
        tags["Renderer"] = {true, Log::Level::Info};
        tags["Memory"] = {true, Log::Level::Warn};
        tags["AssetManager"] = {true, Log::Level::Info};
        tags["Performance"] = {true, Log::Level::Info};
        tags["Animation"] = {true, Log::Level::Warn};
        tags["Physics"] = {true, Log::Level::Warn};
        
        // Reduce verbosity for stable systems
        tags["Vulkan"] = {true, Log::Level::Warn};
        tags["FileSystem"] = {true, Log::Level::Warn};
        
    #else
        // Release build - minimal logging
        SEDX_CORE_INFO("Configuring release build logging");
        
        tags["Renderer"] = {true, Log::Level::Error};
        tags["Memory"] = {true, Log::Level::Error};
        tags["AssetManager"] = {true, Log::Level::Warn};
        tags["Performance"] = {false, Log::Level::Trace};
        tags["Animation"] = {false, Log::Level::Trace};
        tags["Physics"] = {true, Log::Level::Error};
        
        // Disable debug-only tags
        tags["Vulkan"] = {true, Log::Level::Error};
        tags["FileSystem"] = {true, Log::Level::Error};
        tags["Serialization"] = {false, Log::Level::Trace};
        
    #endif
    
    // Always enable critical systems
    tags["Core"] = {true, Log::Level::Info};
    tags["Application"] = {true, Log::Level::Info};
    tags["Security"] = {true, Log::Level::Warn};
}
```

## Real-World Scenarios

### Application Startup Sequence
```cpp
int main() {
    // Initialize logging first
    SceneryEditorX::Log::Init();
    SceneryEditorX::Log::LogHeader();
    
    SEDX_CORE_INFO("=== Scenery Editor X Startup ===");
    
    // Configure logging for build type
    ConfigureLoggingForBuild();
    
    try {
        SEDX_CORE_INFO_TAG("Application", "Initializing core systems");
        
        // Initialize critical systems
        if (!InitializeMemoryManager()) {
            SEDX_CORE_FATAL_TAG("Application", "Failed to initialize memory manager");
            return -1;
        }
        
        if (!InitializeFileSystem()) {
            SEDX_CORE_FATAL_TAG("Application", "Failed to initialize file system");
            return -1;
        }
        
        SEDX_CORE_INFO_TAG("Application", "Initializing graphics subsystem");
        if (!InitializeRenderer()) {
            SEDX_CORE_ERROR_TAG("Application", "Failed to initialize renderer");
            SEDX_CORE_INFO_TAG("Application", "Attempting fallback renderer");
            if (!InitializeFallbackRenderer()) {
                SEDX_CORE_FATAL_TAG("Application", "All renderer initialization attempts failed");
                return -1;
            }
        }
        
        SEDX_CORE_INFO_TAG("Application", "Loading user preferences");
        LoadUserPreferences();
        
        SEDX_CORE_INFO_TAG("Application", "Creating main window");
        auto window = CreateMainWindow();
        if (!window) {
            SEDX_CORE_FATAL_TAG("Application", "Failed to create main window");
            return -1;
        }
        
        SEDX_CORE_INFO("=== Startup Complete ===");
        SEDX_CORE_INFO_TAG("Application", "Entering main loop");
        
        // Main application loop
        RunMainLoop(window);
        
        SEDX_CORE_INFO_TAG("Application", "=== Beginning Shutdown ===");
        
        // Cleanup in reverse order
        window.reset();
        SEDX_CORE_INFO_TAG("Application", "Main window destroyed");
        
        ShutdownRenderer();
        SEDX_CORE_INFO_TAG("Application", "Renderer shut down");
        
        ShutdownFileSystem();
        SEDX_CORE_INFO_TAG("Application", "File system shut down");
        
        ShutdownMemoryManager();
        SEDX_CORE_INFO_TAG("Application", "Memory manager shut down");
        
        SEDX_CORE_INFO("=== Shutdown Complete ===");
        
    } catch (const std::exception& e) {
        SEDX_CORE_FATAL_TAG("Application", "Unhandled exception during execution: {}", e.what());
        SceneryEditorX::Log::FlushAll();
        return -1;
    }
    
    // Final cleanup
    SceneryEditorX::Log::ShutDown();
    return 0;
}
```

### Complex Error Handling Scenario
```cpp
class SceneLoader {
public:
    std::shared_ptr<Scene> LoadScene(const std::string& scenePath) {
        EDITOR_INFO_TAG("SceneLoader", "Loading scene: {}", scenePath);
        
        if (!ValidateSceneFile(scenePath)) {
            return nullptr;
        }
        
        auto scene = std::make_shared<Scene>();
        
        try {
            // Load scene metadata
            auto metadata = LoadSceneMetadata(scenePath);
            EDITOR_TRACE_TAG("SceneLoader", "Scene metadata: version={}, object_count={}, size={} bytes",
                            metadata.version, metadata.objectCount, metadata.fileSize);
            
            // Version compatibility check
            if (metadata.version > CURRENT_SCENE_VERSION) {
                EDITOR_ERROR_TAG("SceneLoader", "Scene version {} is newer than supported version {}",
                                metadata.version, CURRENT_SCENE_VERSION);
                return nullptr;
            } else if (metadata.version < MINIMUM_SCENE_VERSION) {
                EDITOR_WARN_TAG("SceneLoader", "Scene version {} is deprecated, attempting conversion",
                               metadata.version);
                if (!ConvertLegacyScene(scenePath, metadata.version)) {
                    EDITOR_ERROR_TAG("SceneLoader", "Failed to convert legacy scene version {}",
                                    metadata.version);
                    return nullptr;
                }
            }
            
            // Load scene objects
            size_t successCount = 0;
            size_t failureCount = 0;
            
            for (const auto& objectRef : metadata.objects) {
                try {
                    auto object = LoadSceneObject(objectRef);
                    if (object) {
                        scene->AddObject(object);
                        successCount++;
                        EDITOR_TRACE_TAG("SceneLoader", "Loaded object: {} (type: {})",
                                        object->GetName(), object->GetTypeName());
                    } else {
                        failureCount++;
                        EDITOR_WARN_TAG("SceneLoader", "Failed to load object: {}", objectRef.name);
                    }
                } catch (const std::exception& e) {
                    failureCount++;
                    EDITOR_ERROR_TAG("SceneLoader", "Exception loading object {}: {}", 
                                    objectRef.name, e.what());
                }
            }
            
            // Report loading results
            EDITOR_INFO_TAG("SceneLoader", "Scene loading completed: {} successful, {} failed",
                           successCount, failureCount);
            
            if (failureCount > 0) {
                float failureRate = (float)failureCount / (successCount + failureCount) * 100.0f;
                if (failureRate > 50.0f) {
                    EDITOR_ERROR_TAG("SceneLoader", "High failure rate: {:.1f}% - scene may be corrupted",
                                    failureRate);
                } else {
                    EDITOR_WARN_TAG("SceneLoader", "Some objects failed to load: {:.1f}% failure rate",
                                   failureRate);
                }
            }
            
            // Post-processing
            if (successCount > 0) {
                EDITOR_TRACE_TAG("SceneLoader", "Running post-load processing");
                scene->PostLoadProcessing();
                
                // Validate scene integrity
                auto validationResults = ValidateSceneIntegrity(scene);
                if (!validationResults.empty()) {
                    EDITOR_WARN_TAG("SceneLoader", "Scene validation issues found:");
                    for (const auto& issue : validationResults) {
                        EDITOR_WARN_TAG("SceneLoader", "  - {}", issue);
                    }
                }
                
                EDITOR_INFO_TAG("SceneLoader", "Successfully loaded scene: {} ({} objects)",
                               scenePath, successCount);
                return scene;
            } else {
                EDITOR_ERROR_TAG("SceneLoader", "No objects successfully loaded from scene");
                return nullptr;
            }
            
        } catch (const std::exception& e) {
            EDITOR_ERROR_TAG("SceneLoader", "Critical error loading scene {}: {}", 
                           scenePath, e.what());
            return nullptr;
        }
    }
    
private:
    bool ValidateSceneFile(const std::string& path) {
        if (!std::filesystem::exists(path)) {
            EDITOR_ERROR_TAG("SceneLoader", "Scene file not found: {}", path);
            return false;
        }
        
        auto fileSize = std::filesystem::file_size(path);
        if (fileSize == 0) {
            EDITOR_ERROR_TAG("SceneLoader", "Scene file is empty: {}", path);
            return false;
        }
        
        if (fileSize > MAX_SCENE_FILE_SIZE) {
            EDITOR_ERROR_TAG("SceneLoader", "Scene file too large: {} bytes (max: {} bytes)",
                           fileSize, MAX_SCENE_FILE_SIZE);
            return false;
        }
        
        // Check file permissions
        std::error_code ec;
        auto perms = std::filesystem::status(path, ec).permissions();
        if (ec) {
            EDITOR_ERROR_TAG("SceneLoader", "Cannot check file permissions: {} - {}", 
                           path, ec.message());
            return false;
        }
        
        using std::filesystem::perms;
        if ((perms & perms::owner_read) == perms::none) {
            EDITOR_ERROR_TAG("SceneLoader", "No read permission for scene file: {}", path);
            return false;
        }
        
        EDITOR_TRACE_TAG("SceneLoader", "Scene file validation passed: {} ({} bytes)",
                        path, fileSize);
        return true;
    }
};
```

This comprehensive examples document demonstrates real-world usage patterns that GitHub Copilot and other AI agents can reference to understand how to properly implement logging in the Scenery Editor X codebase, following established patterns and best practices.

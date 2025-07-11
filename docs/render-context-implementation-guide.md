# Scenery Editor X - RenderContext Implementation Guide

---

## Introduction

This document provides a comprehensive implementation guide for working with the RenderContext system in Scenery Editor X. It includes practical examples, best practices, and detailed use cases for different scenarios.

---

## Core Implementation Patterns

### 1. Basic Module Integration

Here's how to properly integrate RenderContext into a custom module:

```cpp
class MyRenderingModule : public Module
{
public:
    explicit MyRenderingModule(const std::string& name = "MyRenderingModule")
        : Module(name)
    {
    }
  
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing {} ===", GetName());
      
        // Get RenderContext singleton
        m_RenderContext = RenderContext::Get();
        SEDX_CORE_ASSERT(m_RenderContext, "RenderContext must be initialized before modules");
      
        // Store device reference for frequent access
        m_Device = m_RenderContext->GetLogicDevice();
        SEDX_CORE_ASSERT(m_Device && m_Device->GetDevice() != VK_NULL_HANDLE, "Invalid Vulkan device");
      
        // Initialize module-specific resources
        try
        {
            InitializeVulkanResources();
            SEDX_CORE_INFO_TAG("INIT", "Resources loaded for {}", GetName());
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("INIT", "Failed to load resources: {}", e.what());
            throw;
        }
    }
  
    void OnDetach() override
    {
        SEDX_CORE_INFO("Cleaning up {}", GetName());
      
        // Resources cleaned up automatically by smart pointers
        m_Buffer.Reset();
        m_RenderContext.Reset();
        m_Device.Reset();
    }
  
    void OnUpdate() override
    {
        if (!m_IsEnabled || !m_Device) return;
        SEDX_PROFILE_SCOPE("MyRenderingModule::OnUpdate");
      
        // Perform rendering operations
        UpdateResources();
    }
  
private:
    void InitializeVulkanResources()
    {
        // Example: Create a vertex buffer
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = 1024;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      
        VkBuffer buffer;
        VkResult result = vkCreateBuffer(
            m_Device->GetDevice(), 
            &bufferInfo, 
            m_RenderContext->allocatorCallback, 
            &buffer
        );
      
        SEDX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create vertex buffer");
      
        // Store buffer in a managed resource (implement BufferResource as needed)
        m_Buffer = CreateRef<BufferResource>(buffer, bufferInfo.size);
    }
  
    void UpdateResources()
    {
        // Example: Update buffer data or perform rendering operations
        if (m_Buffer && m_Buffer->IsValid())
        {
            // Perform operations with the buffer
        }
    }
  
private:
    Ref<RenderContext> m_RenderContext;
    Ref<VulkanDevice> m_Device;
    Ref<BufferResource> m_Buffer;
    bool m_IsEnabled = true;
};
```

### 2. Resource Creation Patterns

#### Buffer Creation

```cpp
class BufferManager
{
public:
    static Ref<Buffer> CreateVertexBuffer(size_t size, const void* data = nullptr)
    {
        SEDX_PROFILE_SCOPE("BufferManager::CreateVertexBuffer");
      
        const auto device = RenderContext::GetCurrentDevice();
        SEDX_CORE_ASSERT(device, "No valid device available");
      
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      
        // Use the allocator callback from RenderContext
        VkBuffer vkBuffer;
        VkResult result = vkCreateBuffer(
            device->GetDevice(),
            &bufferInfo,
            RenderContext::Get()->allocatorCallback,
            &vkBuffer
        );
      
        if (result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("BUFFER", "Failed to create vertex buffer: {}", static_cast<int>(result));
            return nullptr;
        }
      
        // Allocate memory (simplified - in practice, use VMA or custom allocator)
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device->GetDevice(), vkBuffer, &memRequirements);
      
        auto buffer = CreateRef<Buffer>();
        buffer->buffer = vkBuffer;
        buffer->size = size;
      
        // Copy data if provided
        if (data)
        {
            CopyDataToBuffer(buffer, data, size);
        }
      
        SEDX_CORE_INFO_TAG("BUFFER", "Created vertex buffer: {} bytes", size);
        return buffer;
    }
  
    static Ref<Buffer> CreateUniformBuffer(size_t size)
    {
        const auto device = RenderContext::GetCurrentDevice();
      
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      
        VkBuffer vkBuffer;
        VkResult result = vkCreateBuffer(
            device->GetDevice(),
            &bufferInfo,
            RenderContext::Get()->allocatorCallback,
            &vkBuffer
        );
      
        SEDX_CORE_VERIFY(result == VK_SUCCESS, "Failed to create uniform buffer");
      
        auto buffer = CreateRef<Buffer>();
        buffer->buffer = vkBuffer;
        buffer->size = size;
      
        return buffer;
    }
  
private:
    static void CopyDataToBuffer(Ref<Buffer> buffer, const void* data, size_t size)
    {
        // Implementation for copying data to buffer
        // This would typically involve staging buffers and command submission
    }
};
```

#### Image/Texture Creation

```cpp
class TextureManager
{
public:
    static Ref<Texture2D> CreateTexture2D(uint32_t width, uint32_t height, VkFormat format)
    {
        SEDX_PROFILE_SCOPE("TextureManager::CreateTexture2D");
      
        const auto device = RenderContext::GetCurrentDevice();
        const auto physDevice = device->GetPhysicalDevice();
      
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TEXTURE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      
        VkImage image;
        VkResult result = vkCreateImage(
            device->GetDevice(),
            &imageInfo,
            RenderContext::Get()->allocatorCallback,
            &image
        );
      
        if (result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("TEXTURE", "Failed to create texture: {}x{}, format: {}", 
                                width, height, static_cast<int>(format));
            return nullptr;
        }
      
        auto texture = CreateRef<Texture2D>();
        texture->image = image;
        texture->width = width;
        texture->height = height;
        texture->format = format;
      
        SEDX_CORE_INFO_TAG("TEXTURE", "Created texture: {}x{}", width, height);
        return texture;
    }
};
```

### 3. Device Access Patterns

#### Safe Device Access

```cpp
class VulkanOperationHelper
{
public:
    template<typename Operation>
    static bool SafeDeviceOperation(Operation&& op, const std::string& operationName)
    {
        try
        {
            auto device = RenderContext::GetCurrentDevice();
            if (!device || device->GetDevice() == VK_NULL_HANDLE)
            {
                SEDX_CORE_ERROR_TAG("VULKAN", "No valid device for operation: {}", operationName);
                return false;
            }
          
            VkResult result = op(device);
            if (result != VK_SUCCESS)
            {
                SEDX_CORE_ERROR_TAG("VULKAN", "Operation '{}' failed with result: {}", 
                                   operationName, static_cast<int>(result));
                return false;
            }
          
            return true;
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("VULKAN", "Exception in operation '{}': {}", operationName, e.what());
            return false;
        }
    }
};

// Usage example:
bool CreateSomething()
{
    return VulkanOperationHelper::SafeDeviceOperation(
        [](Ref<VulkanDevice> device) -> VkResult {
            // Perform Vulkan operations here
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = device->GetGraphicsQueueFamily();
          
            VkCommandPool commandPool;
            return vkCreateCommandPool(
                device->GetDevice(),
                &poolInfo,
                RenderContext::Get()->allocatorCallback,
                &commandPool
            );
        },
        "CreateCommandPool"
    );
}
```

### 4. Initialization Patterns

#### Application Startup

```cpp
class Application
{
public:
    bool Initialize()
    {
        SEDX_CORE_INFO("=== Application Initialization ===");
      
        try
        {
            // Initialize RenderContext first
            SEDX_CORE_INFO("Initializing render context...");
            auto renderContext = RenderContext::Get();
            renderContext->Init();
          
            // Verify initialization
            if (!renderContext->GetLogicDevice())
            {
                SEDX_CORE_ERROR("Failed to initialize graphics system");
                return false;
            }
          
            SEDX_CORE_INFO("Graphics system initialized successfully");
          
            // Initialize modules that depend on RenderContext
            InitializeModules();
          
            return true;
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR("Application initialization failed: {}", e.what());
            return false;
        }
    }
  
private:
    void InitializeModules()
    {
        // Core rendering modules
        auto rendererModule = CreateRef<RendererModule>();
        m_ModuleStage.PushModule(rendererModule.get());
      
        // UI overlay (should be last)
        auto uiModule = CreateRef<UIModule>();
        m_ModuleStage.PushOverlay(uiModule.get());
      
        // Store references for lifetime management
        m_Modules.push_back(rendererModule);
        m_Modules.push_back(uiModule);
      
        // Attach all modules
        for (auto* module : m_ModuleStage)
        {
            module->OnAttach();
        }
    }
  
private:
    ModuleStage m_ModuleStage;
    std::vector<Ref<Module>> m_Modules;
};
```

### 5. Debug and Validation Patterns

#### Custom Debug Callback

```cpp
class DebugManager
{
public:
    static void SetupEnhancedDebugging()
    {
        auto renderContext = RenderContext::Get();
        if (!renderContext)
        {
            SEDX_CORE_WARN("RenderContext not available for debug setup");
            return;
        }
      
#ifdef SEDX_DEBUG
        // Additional debug setup if needed
        VkInstance instance = RenderContext::GetInstance();
        if (instance != VK_NULL_HANDLE)
        {
            // Setup additional debug features
            SetupObjectNaming();
            SetupMemoryTracking();
        }
#endif
    }
  
private:
#ifdef SEDX_DEBUG
    static void SetupObjectNaming()
    {
        // Enable object naming for better debugging
        auto device = RenderContext::GetCurrentDevice();
        if (device && device->GetDevice() != VK_NULL_HANDLE)
        {
            // Set up object naming utilities
            SEDX_CORE_INFO_TAG("DEBUG", "Object naming enabled");
        }
    }
  
    static void SetupMemoryTracking()
    {
        // Setup memory allocation tracking
        SEDX_CORE_INFO_TAG("DEBUG", "Memory tracking enabled");
    }
#endif
};
```

### 6. Error Recovery Patterns

#### Graceful Degradation

```cpp
class RenderingSystem
{
public:
    bool InitializeWithFallback()
    {
        try
        {
            // Try full initialization
            auto renderContext = RenderContext::Get();
            renderContext->Init();
          
            if (ValidateFullCapabilities())
            {
                m_RenderingMode = RenderingMode::Full;
                SEDX_CORE_INFO("Full rendering capabilities available");
                return true;
            }
            else
            {
                // Fall back to reduced capabilities
                m_RenderingMode = RenderingMode::Reduced;
                SEDX_CORE_WARN("Using reduced rendering capabilities");
                return InitializeReducedMode();
            }
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR("Graphics initialization failed: {}", e.what());
          
            // Try software fallback
            return InitializeSoftwareFallback();
        }
    }
  
private:
    bool ValidateFullCapabilities()
    {
        auto device = RenderContext::GetCurrentDevice();
        if (!device) return false;
      
        // Check for required features
        const auto& features = device->GetPhysicalDevice()->GetDeviceFeatures();
        const auto& properties = device->GetPhysicalDevice()->GetDeviceProperties();
      
        bool hasRequiredFeatures = 
            features.geometryShader &&
            features.tessellationShader &&
            properties.limits.maxDescriptorSetSamplers >= 1000;
          
        return hasRequiredFeatures;
    }
  
    bool InitializeReducedMode()
    {
        SEDX_CORE_INFO("Initializing reduced rendering mode");
        // Initialize with basic capabilities only
        return true;
    }
  
    bool InitializeSoftwareFallback()
    {
        SEDX_CORE_WARN("Falling back to software rendering");
        m_RenderingMode = RenderingMode::Software;
        return true;
    }
  
private:
    enum class RenderingMode { Full, Reduced, Software };
    RenderingMode m_RenderingMode = RenderingMode::Full;
};
```

### 7. Testing Integration

#### Unit Test Support

```cpp
namespace Tests
{
    class RenderContextTestFixture
    {
    public:
        void SetUp()
        {
            // Initialize headless rendering context for testing
            SEDX_CORE_INFO("Setting up test render context");
          
            try
            {
                m_TestContext = RenderContext::Get();
                m_TestContext->Init();
              
                SEDX_CORE_ASSERT(m_TestContext->GetLogicDevice(), "Test context requires valid device");
                SEDX_CORE_INFO("Test render context initialized successfully");
            }
            catch (const std::exception& e)
            {
                SEDX_CORE_ERROR("Failed to initialize test context: {}", e.what());
                throw;
            }
        }
      
        void TearDown()
        {
            // Cleanup is automatic with smart pointers
            m_TestContext.Reset();
            SEDX_CORE_INFO("Test render context cleaned up");
        }
      
        Ref<VulkanDevice> GetTestDevice() const
        {
            return m_TestContext->GetLogicDevice();
        }
      
    private:
        Ref<RenderContext> m_TestContext;
    };
  
    // Example test
    TEST_F(RenderContextTestFixture, BufferCreationTest)
    {
        auto device = GetTestDevice();
        ASSERT_TRUE(device);
      
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = 1024;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
      
        VkBuffer buffer;
        VkResult result = vkCreateBuffer(
            device->GetDevice(),
            &bufferInfo,
            RenderContext::Get()->allocatorCallback,
            &buffer
        );
      
        EXPECT_EQ(result, VK_SUCCESS);
      
        if (result == VK_SUCCESS)
        {
            vkDestroyBuffer(device->GetDevice(), buffer, RenderContext::Get()->allocatorCallback);
        }
    }
}
```

### 8. Performance Monitoring

#### Resource Usage Tracking

```cpp
class RenderContextMonitor
{
public:
    static void StartMonitoring()
    {
        auto device = RenderContext::GetCurrentDevice();
        if (!device) return;
      
        // Start performance monitoring
        m_StartTime = std::chrono::high_resolution_clock::now();
        LogSystemInfo();
      
        SEDX_CORE_INFO_TAG("MONITOR", "Performance monitoring started");
    }
  
    static void LogFrameStats()
    {
        static uint32_t frameCount = 0;
        static auto lastReportTime = std::chrono::high_resolution_clock::now();
      
        frameCount++;
      
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastReportTime).count();
      
        if (elapsed >= 5) // Report every 5 seconds
        {
            float fps = frameCount / float(elapsed);
            LogMemoryUsage();
            SEDX_CORE_INFO_TAG("MONITOR", "Average FPS: {:.2f}", fps);
          
            frameCount = 0;
            lastReportTime = currentTime;
        }
    }
  
private:
    static void LogSystemInfo()
    {
        auto device = RenderContext::GetCurrentDevice();
        if (!device) return;
      
        const auto& properties = device->GetPhysicalDevice()->GetDeviceProperties();
        SEDX_CORE_INFO_TAG("MONITOR", "GPU: {}", properties.deviceName);
        SEDX_CORE_INFO_TAG("MONITOR", "Driver Version: {}", properties.driverVersion);
        SEDX_CORE_INFO_TAG("MONITOR", "Vulkan API Version: {}.{}.{}", 
                          VK_VERSION_MAJOR(properties.apiVersion),
                          VK_VERSION_MINOR(properties.apiVersion),
                          VK_VERSION_PATCH(properties.apiVersion));
    }
  
    static void LogMemoryUsage()
    {
        auto device = RenderContext::GetCurrentDevice();
        if (!device) return;
      
        // Log memory usage statistics
        const auto& memProps = device->GetPhysicalDevice()->GetMemoryProperties();
        for (uint32_t i = 0; i < memProps.memoryHeapCount; ++i)
        {
            const auto& heap = memProps.memoryHeaps[i];
            if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            {
                SEDX_CORE_INFO_TAG("MONITOR", "Device Memory Heap {}: {} MB", 
                                  i, heap.size / (1024 * 1024));
            }
        }
    }
  
private:
    static std::chrono::high_resolution_clock::time_point m_StartTime;
};
```

## Best Practices Summary

1. **Always validate RenderContext availability** before using Vulkan resources
2. **Store device references** in modules to avoid repeated singleton calls
3. **Use the allocator callback** for all Vulkan resource creation
4. **Implement proper error handling** with tagged logging
5. **Follow RAII principles** with smart pointers for automatic cleanup
6. **Enable debug features** in development builds
7. **Test with validation layers** enabled
8. **Monitor resource usage** in performance-critical applications
9. **Implement graceful fallbacks** for hardware capability differences
10. **Use profiling scopes** for performance analysis

## Common Pitfalls to Avoid

1. **Don't access RenderContext before initialization**
2. **Don't store raw Vulkan handles without proper lifetime management**
3. **Don't ignore validation layer messages in debug builds**
4. **Don't create resources without checking device capabilities**
5. **Don't forget to use the allocator callback for consistency**
6. **Don't perform Vulkan operations from arbitrary threads**
7. **Don't assume all devices support the same features**
8. **Don't mix different memory allocation strategies**

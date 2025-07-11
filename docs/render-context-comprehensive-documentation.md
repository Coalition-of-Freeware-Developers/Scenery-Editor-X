# Scenery Editor X - RenderContext Comprehensive Documentation

## Overview

The RenderContext system is the central graphics management component in Scenery Editor X, responsible for initializing and managing the complete Vulkan rendering environment. This system serves as a singleton that provides global access to Vulkan instances, physical devices, logical devices, and related rendering resources throughout the application.

The RenderContext acts as the foundational layer upon which all graphics operations are built, ensuring proper initialization sequence, resource management, and providing a consistent interface for Vulkan operations across the entire application.

---

## Table of Contents

1. [Architecture &amp; Design](#architecture--design)
2. [Class Structure](#class-structure)
3. [Initialization Process](#initialization-process)
4. [Usage Patterns](#usage-patterns)
5. [Integration Guidelines](#integration-guidelines)
6. [Error Handling](#error-handling)
7. [Debug Features](#debug-features)
8. [Performance Considerations](#performance-considerations)
9. [Examples](#examples)
10. [Best Practices](#best-practices)
11. [Troubleshooting](#troubleshooting)

---

## Architecture & Design

### Design Principles

The RenderContext follows several key design principles:

1. **Singleton Pattern**: Ensures single instance throughout application lifecycle
2. **RAII (Resource Acquisition Is Initialization)**: Automatic resource management through smart pointers
3. **Vulkan Best Practices**: Follows official Vulkan guidelines for instance and device management
4. **Platform Abstraction**: Handles platform-specific extensions and configurations
5. **Debug Safety**: Comprehensive validation and debugging support in development builds

### Core Components

```cpp
// Core Vulkan components managed by RenderContext
VkInstance instance;                        // Main Vulkan instance
Ref<VulkanPhysicalDevice> vkPhysicalDevice; // Selected GPU device
Ref<VulkanDevice> vkDevice;                 // Logical device wrapper
VkAllocationCallbacks* allocatorCallback;   // Custom allocation callbacks

// Debug components (debug builds only)
VkDebugUtilsMessengerEXT debugMessenger;    // Validation layer messenger
VkDebugReportCallbackEXT debugCallback;     // Legacy debug callback
```

### Dependency Graph

```
Application
    ↓
RenderContext::Get()->Init()
    ↓
┌─────────────┬─────────────┬─────────────┐
│   Instance  │   Physical  │   Logical   │
│  Creation   │   Device    │   Device    │
│             │  Selection  │  Creation   │
└─────────────┴─────────────┴─────────────┘
    ↓
Pipeline Cache & Resource Managers
    ↓
Rendering Modules & Systems
```

---

## Class Structure

### RenderContext Class Definition

```cpp
/**
 * @file render_context.h
 * @brief Central Vulkan context management for Scenery Editor X
 * 
 * The RenderContext provides singleton access to Vulkan resources and handles
 * the complete initialization sequence for graphics operations.
 */
class RenderContext : public RefCounted
{
public:
    /// Construction & Destruction
    RenderContext();
    virtual ~RenderContext() override;

    /// Core Initialization
    virtual void Init();

    /// Singleton Access
    GLOBAL Ref<RenderContext> Get();
    GLOBAL VkInstance GetInstance();

    /// Device Access
    Ref<VulkanDevice> GetLogicDevice() { return vkDevice; }
    LOCAL Ref<VulkanDevice> GetCurrentDevice() { return Get()->GetLogicDevice(); }

    /// Resource Management
    std::vector<uint8_t> GetPipelineCacheData() const;
    VkAllocationCallbacks* allocatorCallback = nullptr;

private:
    /// Core Vulkan Objects
    Ref<VulkanPhysicalDevice> vkPhysicalDevice;
    Ref<VulkanDevice> vkDevice;
    inline LOCAL VkInstance instance;

    /// Debug Support (Debug builds only)
#ifdef SEDX_DEBUG
    VkDebugReportCallbackEXT debugCallback = VK_NULL_HANDLE;
#endif
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
};
```

### Key Methods

#### Singleton Access

```cpp
// Get the singleton instance (creates if needed)
Ref<RenderContext> Get();

// Access Vulkan instance directly
VkInstance GetInstance();

// Quick device access for Vulkan operations
Ref<VulkanDevice> GetCurrentDevice();
```

#### Resource Access

```cpp
// Get the logical device for resource creation
Ref<VulkanDevice> GetLogicDevice();

// Access pipeline cache data for serialization
std::vector<uint8_t> GetPipelineCacheData() const;
```

---

## Initialization Process

The RenderContext initialization follows a strict sequence to ensure proper Vulkan setup:

### 1. Application Information Setup

```cpp
// Application metadata for Vulkan
VkApplicationInfo appInfo{};
appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
appInfo.pApplicationName = appData.appName.c_str();
appInfo.applicationVersion = AppData::version;
appInfo.pEngineName = AppData::renderName.c_str();
appInfo.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
appInfo.apiVersion = apiVersion;
```

### 2. Extension and Layer Configuration

The system automatically detects and configures:

- **Platform Extensions**: Win32, X11, Wayland, MacOS surface extensions
- **Debug Extensions**: `VK_EXT_debug_utils` for development builds
- **Validation Layers**: Khronos validation layer in debug mode
- **Optional Extensions**: Surface maintenance, capabilities extensions

```cpp
std::vector<const char*> instanceExtensions = {
    VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(SEDX_PLATFORM_WINDOWS)
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(SEDX_PLATFORM_LINUX)
    #if defined(VK_USE_PLATFORM_WAYLAND_KHR)
        VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
    #else
        VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
    #endif
#elif defined(SEDX_PLATFORM_APPLE)
    VK_EXT_LAYER_SETTINGS_EXTENSION_NAME,
    VK_MVK_MACOS_SURFACE_EXTENSION_NAME,
#endif
};
```

### 3. Validation Layer Setup

```cpp
// Debug builds enable comprehensive validation
#ifdef SEDX_DEBUG
LOCAL constexpr bool enableValidationLayers = true;
#else
LOCAL constexpr bool enableValidationLayers = false;
#endif

LOCAL const char* validationLayer[] = {"VK_LAYER_KHRONOS_validation"};
```

### 4. Instance Creation

```cpp
VkInstanceCreateInfo createInfo = {};
createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
createInfo.pApplicationInfo = &appInfo;
createInfo.enabledLayerCount = enableValidationLayers ? 1 : 0;
createInfo.ppEnabledLayerNames = enableValidationLayers ? validationLayer : nullptr;
createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
createInfo.ppEnabledExtensionNames = instanceExtensions.data();

VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
```

### 5. Debug Messenger Setup

```cpp
if (enableValidationLayers)
{
    VkDebugUtilsMessengerCreateInfoEXT messengerInfo;
    PopulateDebugMsgCreateInfo(messengerInfo);
    CreateDebugUtilsMessengerEXT(instance, &messengerInfo, nullptr, &debugMessenger);
}
```

### 6. Physical Device Selection

```cpp
// Select the best available GPU
vkPhysicalDevice = VulkanPhysicalDevice::Select(instance);
if (!vkPhysicalDevice)
{
    SEDX_CORE_ERROR_TAG("Graphics Engine", "No suitable Vulkan physical device found!");
    return;
}
```

### 7. Logical Device Creation

```cpp
// Create logical device with required features
vkDevice = CreateRef<VulkanDevice>(vkPhysicalDevice);

// Verify successful creation
if (!vkDevice || vkDevice->GetDevice() == VK_NULL_HANDLE)
{
    SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create valid Vulkan device!");
    return;
}
```

### 8. Pipeline Cache Initialization

```cpp
// Initialize pipeline cache for performance
PipelineCache pipelineCache;
pipelineCache.CreateCache();
```

---

## Usage Patterns

### Basic Access Pattern

```cpp
// Standard access pattern for getting RenderContext
auto renderContext = RenderContext::Get();

// Access logical device for resource operations
auto device = renderContext->GetLogicDevice();

// Direct Vulkan instance access
VkInstance instance = RenderContext::GetInstance();
```

### Common Vulkan Operations

```cpp
void CreateVulkanResource()
{
    // Get current device for Vulkan calls
    const auto device = RenderContext::GetCurrentDevice();
    const VkDevice vkDevice = device->GetDevice();
  
    // Use allocator callback for consistency
    VkBuffer buffer;
    VkResult result = vkCreateBuffer(
        vkDevice, 
        &bufferInfo, 
        RenderContext::Get()->allocatorCallback, 
        &buffer
    );
  
    SEDX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create buffer");
}
```

### Module Integration Pattern

```cpp
class RenderingModule : public Module
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO("Initializing rendering module");
      
        // Ensure RenderContext is available
        m_RenderContext = RenderContext::Get();
        SEDX_CORE_ASSERT(m_RenderContext, "RenderContext must be initialized before modules");
      
        // Store device reference for frequent access
        m_Device = m_RenderContext->GetLogicDevice();
        SEDX_CORE_ASSERT(m_Device && m_Device->GetDevice() != VK_NULL_HANDLE, "Invalid device");
      
        InitializeResources();
    }
  
    void OnDetach() override
    {
        // Cleanup is automatic with smart pointers
        m_Device.Reset();
        m_RenderContext.Reset();
    }

private:
    Ref<RenderContext> m_RenderContext;
    Ref<VulkanDevice> m_Device;
  
    void InitializeResources()
    {
        // Create module-specific Vulkan resources
        CreateBuffers();
        CreatePipelines();
        CreateDescriptorSets();
    }
};
```

---

## Integration Guidelines

### Application Startup Sequence

```cpp
class Application
{
public:
    bool Initialize()
    {
        SEDX_CORE_INFO("=== Application Initialization ===");
      
        try
        {
            // 1. Initialize RenderContext first
            SEDX_CORE_INFO("Initializing render context...");
            auto renderContext = RenderContext::Get();
            renderContext->Init();
          
            // 2. Verify initialization
            if (!renderContext->GetLogicDevice())
            {
                SEDX_CORE_ERROR("Failed to initialize graphics system");
                return false;
            }
          
            SEDX_CORE_INFO("Graphics system initialized successfully");
          
            // 3. Initialize dependent systems
            InitializeRenderingSystems();
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
    void InitializeRenderingSystems()
    {
        // Initialize systems that depend on RenderContext
        m_Renderer = CreateRef<Renderer>();
        m_SwapChain = CreateRef<SwapChain>();
        m_CommandPool = CreateRef<CommandPool>();
    }
};
```

### Module Development Best Practices

1. **Always verify RenderContext availability**:

```cpp
SEDX_CORE_ASSERT(RenderContext::Get(), "RenderContext required for graphics operations");
```

2. **Store device references for performance**:

```cpp
// Cache device reference to avoid repeated singleton access
m_Device = RenderContext::GetCurrentDevice();
```

3. **Use consistent allocation patterns**:

```cpp
// Always use the RenderContext allocator callback
vkCreateBuffer(device, &info, RenderContext::Get()->allocatorCallback, &buffer);
```

4. **Follow proper resource lifecycle**:

```cpp
// Resources are automatically cleaned up via smart pointers
// Manual cleanup only needed for raw Vulkan handles
```

### Resource Creation Patterns

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
            SEDX_CORE_ERROR("Failed to create vertex buffer: {}", static_cast<int>(result));
            return nullptr;
        }
      
        return CreateRef<Buffer>(vkBuffer, size);
    }
};
```

---

## Error Handling

### Validation Layer Support

```cpp
// Automatic validation in debug builds
#ifdef SEDX_DEBUG
LOCAL constexpr bool enableValidationLayers = true;
#else
LOCAL constexpr bool enableValidationLayers = false;
#endif
```

### API Version Checking

```cpp
if (!VulkanChecks::CheckAPIVersion(RenderData::minVulkanVersion))
{
    SEDX_CORE_ERROR_TAG("Graphics Engine", "Incompatible Vulkan driver version!");
    return;
}
```

### Device Validation

```cpp
if (!vkDevice || vkDevice->GetDevice() == VK_NULL_HANDLE)
{
    SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create valid Vulkan device!");
    return;
}
```

### Exception Safety

```cpp
try
{
    // Initialization code
    renderContext->Init();
    SEDX_CORE_INFO("RenderContext initialization complete");
}
catch (const std::exception& e)
{
    SEDX_CORE_ERROR("Exception during RenderContext initialization: {}", e.what());
    // Cleanup is automatic via RAII
}
catch (...)
{
    SEDX_CORE_ERROR("Unknown exception during RenderContext initialization");
}
```

---

## Debug Features

### Debug Callback Implementation

```cpp
LOCAL VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugMsgCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    // Filter messages by severity
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        SEDX_CORE_WARN("[Validation Layer] {}", pCallbackData->pMessage);
  
    return VK_FALSE; // Continue execution
}
```

### Enhanced Debug Configuration

```cpp
// Platform-specific validation layer settings
#if defined(VK_EXT_layer_settings) && VK_EXT_layer_settings
const VkBool32 gpuav_descriptor_checks = VK_FALSE;
const VkBool32 gpuav_indirect_draws_buffers = VK_FALSE;
const VkBool32 gpuav_post_process_descriptor_indexing = VK_FALSE;

const VkLayerSettingEXT settings[] = {
    {"VK_LAYER_KHRONOS_validation", "gpuav_descriptor_checks", 
     VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &gpuav_descriptor_checks},
    {"VK_LAYER_KHRONOS_validation", "gpuav_indirect_draws_buffers", 
     VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &gpuav_indirect_draws_buffers},
    {"VK_LAYER_KHRONOS_validation", "gpuav_post_process_descriptor_indexing", 
     VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &gpuav_post_process_descriptor_indexing},
};
#endif
```

---

## Performance Considerations

### Pipeline Cache Management

```cpp
// Pipeline cache improves shader compilation performance
PipelineCache pipelineCache;
pipelineCache.CreateCache();

// Cache can be serialized for persistence across sessions
std::vector<uint8_t> cacheData = renderContext->GetPipelineCacheData();
```

### Device Selection Optimization

The physical device selection prioritizes:

1. Discrete GPUs over integrated GPUs
2. Devices with required feature support
3. Devices with optimal queue family configurations
4. Devices with maximum memory heaps

### Memory Allocation Strategy

```cpp
// Custom allocation callbacks can be set for memory tracking
VkAllocationCallbacks* allocatorCallback = nullptr;

// Use consistent allocation patterns throughout the application
vkCreateBuffer(device, &info, allocatorCallback, &buffer);
```

---

## Examples

### Basic RenderContext Usage

```cpp
#include <SceneryEditorX/renderer/render_context.h>

int main()
{
    try
    {
        // Initialize the render context
        auto renderContext = SceneryEditorX::RenderContext::Get();
        renderContext->Init();
      
        // Verify initialization
        if (auto device = renderContext->GetLogicDevice())
        {
            std::cout << "Graphics system initialized successfully" << std::endl;
          
            // Use the graphics system
            // ... application logic ...
        }
        else
        {
            std::cerr << "Failed to initialize graphics system" << std::endl;
            return -1;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
  
    return 0;
}
```

### Custom Rendering Module

```cpp
class CustomRenderingModule : public Module
{
public:
    CustomRenderingModule() : Module("CustomRenderingModule") {}
  
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing {} ===", GetName());
      
        // Get RenderContext singleton
        m_RenderContext = RenderContext::Get();
        SEDX_CORE_ASSERT(m_RenderContext, "RenderContext must be initialized");
      
        // Store device reference
        m_Device = m_RenderContext->GetLogicDevice();
        SEDX_CORE_ASSERT(m_Device && m_Device->GetDevice() != VK_NULL_HANDLE, "Invalid device");
      
        // Initialize resources
        InitializeVulkanResources();
      
        SEDX_CORE_INFO_TAG("INIT", "Resources loaded for {}", GetName());
    }
  
    void OnDetach() override
    {
        SEDX_CORE_INFO("=== Shutting down {} ===", GetName());
      
        // Cleanup Vulkan resources
        CleanupVulkanResources();
      
        // Reset references
        m_Device.Reset();
        m_RenderContext.Reset();
    }
  
    void OnUpdate(float deltaTime) override
    {
        if (m_IsEnabled)
        {
            UpdateRenderingData(deltaTime);
        }
    }
  
    void OnRender() override
    {
        if (m_IsEnabled && m_Device)
        {
            PerformRenderingOperations();
        }
    }

private:
    Ref<RenderContext> m_RenderContext;
    Ref<VulkanDevice> m_Device;
    bool m_IsEnabled = true;
  
    // Module-specific resources
    VkBuffer m_VertexBuffer = VK_NULL_HANDLE;
    VkBuffer m_IndexBuffer = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
  
    void InitializeVulkanResources()
    {
        CreateVertexBuffer();
        CreateIndexBuffer();
        CreatePipeline();
    }
  
    void CreateVertexBuffer()
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(vertices);
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      
        VkResult result = vkCreateBuffer(
            m_Device->GetDevice(),
            &bufferInfo,
            m_RenderContext->allocatorCallback,
            &m_VertexBuffer
        );
      
        SEDX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create vertex buffer");
    }
  
    void CleanupVulkanResources()
    {
        if (m_Device && m_Device->GetDevice() != VK_NULL_HANDLE)
        {
            if (m_VertexBuffer != VK_NULL_HANDLE)
            {
                vkDestroyBuffer(m_Device->GetDevice(), m_VertexBuffer, 
                               m_RenderContext->allocatorCallback);
                m_VertexBuffer = VK_NULL_HANDLE;
            }
          
            if (m_IndexBuffer != VK_NULL_HANDLE)
            {
                vkDestroyBuffer(m_Device->GetDevice(), m_IndexBuffer, 
                               m_RenderContext->allocatorCallback);
                m_IndexBuffer = VK_NULL_HANDLE;
            }
          
            if (m_Pipeline != VK_NULL_HANDLE)
            {
                vkDestroyPipeline(m_Device->GetDevice(), m_Pipeline, 
                                 m_RenderContext->allocatorCallback);
                m_Pipeline = VK_NULL_HANDLE;
            }
        }
    }
};
```

### Unit Testing Support

```cpp
namespace Tests
{
    class RenderContextTestFixture
    {
    public:
        void SetUp()
        {
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
            vkDestroyBuffer(device->GetDevice(), buffer, 
                           RenderContext::Get()->allocatorCallback);
        }
    }
}
```

---

## Best Practices

### 1. Initialization Order

```cpp
// CORRECT: Initialize RenderContext before any graphics operations
auto renderContext = RenderContext::Get();
renderContext->Init();

// Then initialize dependent systems
auto renderer = CreateRef<Renderer>();
auto swapChain = CreateRef<SwapChain>();
```

### 2. Resource Management

```cpp
// CORRECT: Use smart pointers for automatic cleanup
Ref<RenderContext> m_RenderContext = RenderContext::Get();

// CORRECT: Cache device references for performance
Ref<VulkanDevice> m_Device = m_RenderContext->GetLogicDevice();
```

### 3. Error Checking

```cpp
// CORRECT: Always verify RenderContext initialization
if (!renderContext->GetLogicDevice())
{
    SEDX_CORE_ERROR("Graphics system initialization failed");
    return false;
}

// CORRECT: Check Vulkan operation results
VkResult result = vkCreateBuffer(device, &info, allocator, &buffer);
SEDX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create buffer");
```

### 4. Thread Safety

```cpp
// CORRECT: Access RenderContext from main thread during initialization
// Vulkan operations can be performed on worker threads following Vulkan's rules
```

### 5. Memory Allocation

```cpp
// CORRECT: Use consistent allocator callbacks
vkCreateBuffer(device, &info, RenderContext::Get()->allocatorCallback, &buffer);

// CORRECT: Follow RAII principles
class BufferResource
{
public:
    BufferResource(VkBuffer buffer) : m_Buffer(buffer) {}
  
    ~BufferResource()
    {
        if (m_Buffer != VK_NULL_HANDLE)
        {
            auto device = RenderContext::GetCurrentDevice();
            vkDestroyBuffer(device->GetDevice(), m_Buffer, 
                           RenderContext::Get()->allocatorCallback);
        }
    }
  
private:
    VkBuffer m_Buffer = VK_NULL_HANDLE;
};
```

---

## Troubleshooting

### Common Issues

#### 1. Validation Layer Not Found

**Symptoms**: Error messages about missing validation layers
**Solutions**:

- Ensure Vulkan SDK is properly installed
- Check `VK_LAYER_PATH` environment variable
- Verify validation layers are available: `vulkaninfo | grep -i validation`

#### 2. Device Creation Failure

**Symptoms**: `vkDevice` is null or device creation fails
**Solutions**:

- Verify GPU supports minimum Vulkan version
- Check available queue families
- Update graphics drivers
- Verify required device features are available

#### 3. Extension Not Available

**Symptoms**: Instance creation fails due to missing extensions
**Solutions**:

- Update graphics drivers
- Check platform-specific extension availability
- Use `vulkaninfo` to list available extensions

#### 4. Pipeline Cache Issues

**Symptoms**: Pipeline creation performance problems
**Solutions**:

- Verify pipeline cache directory permissions
- Check disk space availability
- Clear pipeline cache for corrupted data

### Debug Output Analysis

Enable comprehensive logging to diagnose issues:

```cpp
// Core initialization logging
SEDX_CORE_INFO_TAG("Graphics Engine", "Initializing RenderContext");
SEDX_CORE_TRACE_TAG("Graphics Engine", "Creating Vulkan Instance");
SEDX_CORE_INFO_TAG("Graphics Engine", "Validation layers available: {}", khronosAvailable);

// Device selection logging
SEDX_CORE_INFO_TAG("Graphics Engine", "Selected device: {}", deviceName);
SEDX_CORE_TRACE_TAG("Graphics Engine", "Device features: ...");
```

### Memory Debugging

```cpp
#ifdef SEDX_DEBUG
// Enable additional memory debugging
class MemoryTracker
{
public:
    static void EnableTracking()
    {
        auto renderContext = RenderContext::Get();
        // Setup custom allocator callbacks for tracking
        renderContext->allocatorCallback = &s_TrackedAllocator;
    }
  
private:
    static VkAllocationCallbacks s_TrackedAllocator;
};
#endif
```

---

## Future Enhancements

### Planned Features

1. **Multi-GPU Support**: Support for multiple physical devices
2. **Dynamic Device Switching**: Runtime GPU switching capability
3. **Enhanced Pipeline Cache**: Persistent cache with versioning
4. **Vulkan 1.3+ Features**: Ray tracing, enhanced synchronization
5. **Memory Pool Management**: Advanced memory allocation strategies

### Extension Points

The RenderContext is designed to be extensible:

```cpp
// Future extension example
class RenderContextExtensions
{
public:
    static void EnableRayTracing(RenderContext* context);
    static void EnableMeshShaders(RenderContext* context);
    static void ConfigureMultiGPU(RenderContext* context);
};
```

---

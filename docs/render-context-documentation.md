# Scenery Editor X - RenderContext System Documentation

---

## Overview

The RenderContext system is the central component responsible for initializing and managing the Vulkan rendering environment in Scenery Editor X. It serves as a singleton that provides global access to the Vulkan instance, physical device, logical device, and related rendering resources throughout the application.

---

## Architecture

The RenderContext follows the singleton pattern and acts as the primary interface between the application and Vulkan API. It manages the complete lifecycle of Vulkan resources from initialization to cleanup.

### Key Components

1. **VkInstance**: The main Vulkan instance that connects the application to the Vulkan runtime
2. **VulkanPhysicalDevice**: Wrapper for the selected physical GPU device
3. **VulkanDevice**: Wrapper for the logical device and its associated queues
4. **Debug Utilities**: Validation layers and debug messengers for development builds
5. **Pipeline Cache**: Optimized storage for compiled pipeline objects

---

## Class Structure

### RenderContext Class

```cpp
class RenderContext : public RefCounted
{
public:
    RenderContext();
    virtual ~RenderContext() override;

    /// Core Methods
    virtual void Init();
    GLOBAL Ref<RenderContext> Get();
    GLOBAL VkInstance GetInstance();
  
    /// Device Access
    Ref<VulkanDevice> GetLogicDevice() { return vkDevice; }
    LOCAL Ref<VulkanDevice> GetCurrentDevice() { return Get()->GetLogicDevice(); }
  
    /// Utility Methods
    std::vector<uint8_t> GetPipelineCacheData() const;
  
    /// Allocation Support
    VkAllocationCallbacks *allocatorCallback = nullptr;

private:
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



---

## Initialization Process

The RenderContext initialization follows a specific sequence:

### 1. Application Info Setup

```cpp
VkApplicationInfo appInfo{};
appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
appInfo.pApplicationName = appData.appName.c_str();
appInfo.applicationVersion = AppData::version;
appInfo.pEngineName = AppData::renderName.c_str();
appInfo.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
appInfo.apiVersion = apiVersion;
```

### 2. Extension and Layer Configuration

The system automatically detects and enables:

- Platform-specific surface extensions (Win32, X11, Wayland, MacOS)
- Debug utilities for development builds
- Validation layers when `SEDX_DEBUG` is defined
- Surface maintenance and capabilities extensions

### 3. Instance Creation

```cpp
VkInstanceCreateInfo createInfo = {};
createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
createInfo.pApplicationInfo = &appInfo;
createInfo.enabledLayerCount = enableValidationLayers ? 1 : 0;
createInfo.ppEnabledLayerNames = enableValidationLayers ? validationLayer : nullptr;
createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
createInfo.ppEnabledExtensionNames = instanceExtensions.data();

vkCreateInstance(&createInfo, nullptr, &instance);
```

### 4. Debug Setup (Debug Builds)

```cpp
if (enableValidationLayers)
{
    VkDebugUtilsMessengerCreateInfoEXT messengerInfo;
    PopulateDebugMsgCreateInfo(messengerInfo);
    CreateDebugUtilsMessengerEXT(instance, &messengerInfo, nullptr, &debugMessenger);
}
```

### 5. Device Selection and Creation

```cpp
vkPhysicalDevice = VulkanPhysicalDevice::Select(instance);
vkDevice = CreateRef<VulkanDevice>(vkPhysicalDevice);
```

### 6. Pipeline Cache Creation

```cpp
PipelineCache pipelineCache;
pipelineCache.CreateCache();
```



---

## Usage Patterns

### Basic Access Pattern

```cpp
// Get the singleton instance
auto renderContext = RenderContext::Get();

// Access the logical device
auto device = renderContext->GetLogicDevice();

// Access the Vulkan instance directly
VkInstance instance = RenderContext::GetInstance();
```

### Common Usage in Vulkan Operations

```cpp
void SomeVulkanOperation()
{
    // Get current device for Vulkan calls
    const auto device = RenderContext::GetCurrentDevice();
    const VkDevice vkDevice = device->GetDevice();
  
    // Perform Vulkan operations
    vkCreateBuffer(vkDevice, &bufferInfo, RenderContext::Get()->allocatorCallback, &buffer);
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
      
        // Access device for resource creation
        m_Device = m_RenderContext->GetLogicDevice();
        InitializeResources();
    }
  
private:
    Ref<RenderContext> m_RenderContext;
    Ref<VulkanDevice> m_Device;
};
```



---

## Error Handling

The RenderContext implements comprehensive error handling:

### Validation Layer Support

```cpp
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
    SEDX_CORE_INFO("RenderContext initialization complete");
}
catch (const std::exception& e)
{
    SEDX_CORE_ERROR("Exception during RenderContext initialization: {}", e.what());
}
catch (...)
{
    SEDX_CORE_ERROR("Unknown exception during RenderContext initialization");
}
```



---

## Resource Management

### Memory Management

The RenderContext uses the custom smart pointer system for automatic resource management:

```cpp
// Resources are automatically managed via Ref<T>
Ref<VulkanPhysicalDevice> vkPhysicalDevice;
Ref<VulkanDevice> vkDevice;

// Singleton instance management
LOCAL Ref<RenderContext> s_Instance = nullptr;
```

### Cleanup Process

The destructor ensures proper cleanup order:

```cpp
RenderContext::~RenderContext()
{
    // Clean up debug utilities first
#ifdef SEDX_DEBUG
    if (debugMessenger != VK_NULL_HANDLE && instance != VK_NULL_HANDLE)
    {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        debugMessenger = VK_NULL_HANDLE;
    }
#endif

    // Destroy Vulkan instance last
    if (instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance, nullptr);
        instance = VK_NULL_HANDLE;
    }
}
```

---

## Platform Support

The RenderContext supports multiple platforms through conditional compilation:

### Windows

```cpp
#if defined(SEDX_PLATFORM_WINDOWS)
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
```

### Linux

```cpp
#elif defined(SEDX_PLATFORM_LINUX)
    #if defined(VK_USE_PLATFORM_WAYLAND_KHR)
        VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
    #else
        VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
    #endif
#endif
```

### macOS

```cpp
#elif defined(SEDX_PLATFORM_APPLE)
    VK_EXT_LAYER_SETTINGS_EXTENSION_NAME, 
    VK_MVK_MACOS_SURFACE_EXTENSION_NAME,
#endif
```



---

## Debug Features

### Validation Layers

The system automatically enables Khronos validation layers in debug builds:

```cpp
LOCAL const char* validationLayer[] = {"VK_LAYER_KHRONOS_validation"};
```

### Debug Callback

```cpp
LOCAL VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugMsgCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        SEDX_CORE_WARN("[Validation Layer] {0}", pCallbackData->pMessage);
    return VK_FALSE;
}
```



---

## Performance Considerations

### Pipeline Cache

The RenderContext initializes a pipeline cache to improve shader compilation performance:

```cpp
PipelineCache pipelineCache;
pipelineCache.CreateCache();
```

### Device Selection

Physical device selection prioritizes devices with the best feature support and performance characteristics.

---

## Integration Guidelines

### Module Development

When creating modules that use Vulkan:

1. **Always check RenderContext availability**:

   ```cpp
   SEDX_CORE_ASSERT(RenderContext::Get(), "RenderContext required");
   ```
2. **Store device references for frequent access**:

   ```cpp
   m_Device = RenderContext::GetCurrentDevice();
   ```
3. **Use allocator callbacks for consistency**:

   ```cpp
   vkCreateBuffer(device, &info, RenderContext::Get()->allocatorCallback, &buffer);
   ```

### Error Handling Best Practices

1. **Always validate device access**:

   ```cpp
   auto device = RenderContext::GetCurrentDevice();
   SEDX_CORE_ASSERT(device && device->GetDevice() != VK_NULL_HANDLE, "Invalid device");
   ```
2. **Use tagged logging for categorization**:

   ```cpp
   SEDX_CORE_ERROR_TAG("RENDER", "Failed to create resource");
   ```
3. **Handle initialization failures gracefully**:

   ```cpp
   if (!RenderContext::Get()->GetLogicDevice())
   {
       SEDX_CORE_ERROR("Graphics system not available");
       return false;
   }
   ```

---

## Thread Safety

The RenderContext singleton is designed for single-threaded initialization but provides thread-safe access to read-only resources. Vulkan command recording and submission should follow Vulkan's thread safety guidelines.

## Testing Support

The RenderContext supports headless initialization for unit testing:

```cpp
namespace Tests
{
    void VulkanTestEnvironment::initializeVulkan()
    {
        // Simulates RenderContext::Init() for testing
        auto context = RenderContext::Get();
        context->Init();
    }
}
```



---

## Troubleshooting

### Common Issues

1. **Validation Layer Not Found**

   - Ensure Vulkan SDK is properly installed
   - Check `VK_LAYER_PATH` environment variable
2. **Device Creation Failure**

   - Verify GPU supports required Vulkan version
   - Check available queue families
3. **Extension Not Available**

   - Update graphics drivers
   - Verify platform-specific extensions

### Debug Output

Enable debug logging to diagnose issues:

```cpp
SEDX_CORE_INFO_TAG("Graphics Engine", "Validation layers available: {}", khronosAvailable);
SEDX_CORE_TRACE_TAG("Graphics Engine", "Creating Vulkan Instance");
```

## Future Enhancements

- Multi-GPU support
- Dynamic device switching
- Enhanced pipeline cache management
- Vulkan 1.3+ feature detection
- Ray tracing extension support

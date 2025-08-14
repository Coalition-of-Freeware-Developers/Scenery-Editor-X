# Vulkan Utility Header Documentation (`vk_util.h`)

## Overview

The `vk_util.h` header file provides a comprehensive collection of Vulkan utility functions, macros, and inline helpers that streamline Vulkan development within the Scenery Editor X application. This file serves as the central hub for Vulkan error handling, debugging, result checking, and utility functions that simplify common Vulkan operations.

## Core Design Philosophy

The utility system is built around several key principles:

1. **Robust Error Handling**: Every Vulkan operation should be validated and logged appropriately
2. **Development-Friendly Debugging**: Comprehensive debug information during development builds
3. **Performance-Conscious**: Zero-overhead abstractions in release builds
4. **Consistent API**: Uniform interface patterns across all utility functions
5. **Integration with Module System**: Seamless integration with the application's module architecture

## Debug Extensions System

### Function Pointers for Debug Utils

```cpp
inline PFN_vkSetDebugUtilsObjectNameEXT fpSetDebugUtilsObjectNameEXT;
inline PFN_vkCmdBeginDebugUtilsLabelEXT fpCmdBeginDebugUtilsLabelEXT;
inline PFN_vkCmdEndDebugUtilsLabelEXT fpCmdEndDebugUtilsLabelEXT;
inline PFN_vkCmdInsertDebugUtilsLabelEXT fpCmdInsertDebugUtilsLabelEXT;
```

**Purpose**: These inline function pointers provide access to Vulkan's debug utilities extension functions.

**Usage Example**:

```cpp
// Initialize debug extensions in your module
class RenderModule : public Module
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("RENDER", "Initializing Vulkan debug extensions");
        VulkanLoadDebugUtilsExtensions(m_VulkanInstance);
      
        // Name your objects for debugging
        SetDebugUtilsObjectName(
            m_Device, 
            VK_OBJECT_TYPE_BUFFER, 
            "VertexBuffer_Terrain", 
            m_VertexBuffer
        );
    }
};
```

### Debug Utilities Functions

```cpp
void VulkanLoadDebugUtilsExtensions(VkInstance instance);
void RetrieveDiagnosticCheckpoints();
```

**Best Practices**:

- Always call `VulkanLoadDebugUtilsExtensions` during Vulkan instance initialization
- Use debug object naming consistently throughout your codebase
- Retrieve diagnostic checkpoints when investigating device lost errors

## Error Checking System

### Basic Result Checking

```cpp
inline void VulkanCheckResult(const VkResult result)
inline void VulkanCheckResult(const VkResult result, const char *file, int line)
```

**Purpose**: Core functions for validating Vulkan operation results with appropriate error handling.

**Usage Example**:

```cpp
void CreateBuffer()
{
    VkResult result = vkCreateBuffer(m_Device, &bufferInfo, nullptr, &m_Buffer);
    VulkanCheckResult(result, __FILE__, __LINE__);
  
    // Alternative: Use the macro for automatic file/line tracking
    VK_CHECK_RESULT(vkCreateBuffer(m_Device, &bufferInfo, nullptr, &m_Buffer));
}
```

### Advanced Error Checking Macros

```cpp
#define VK_CHECK_RESULT(f) \
{ \
    VkResult result = (f); \
    VulkanCheckResult(result, __FILE__, __LINE__); \
}

#define VK_CHECK(result) \
if ((result) != VK_SUCCESS) \
{ \
    SEDX_CORE_ERROR("Vulkan Error: {}", (result)); \
    __debugbreak(); \
    return false; \
}
```

**Usage Patterns**:

```cpp
// For functions that should abort on failure
void CriticalOperation()
{
    VK_CHECK_RESULT(vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device));
}

// For functions that return success/failure status
bool OptionalOperation()
{
    VkResult result = vkCreateSomeOptionalResource();
    VK_CHECK(result);
    return true;
}
```

### Logging-Integrated Error Handling

```cpp
#define VK_LOG_RESULT(result, operation) \
{ \
    ::SceneryEditorX::Log::LogVulkanResult(result, operation); \
    if ((result) != VK_SUCCESS) \
    { \
        return false; \
    } \
}

#define VK_LOG_RESULT_OR_THROW(result, operation) \
{ \
    ::SceneryEditorX::Log::LogVulkanResult(result, operation); \
    if ((result) != VK_SUCCESS) \
    { \
        throw std::runtime_error((operation) + " failed with " + VK_ERROR_STRING(result)); \
    } \
}
```

**Usage Example**:

```cpp
class BufferManager : public Module
{
public:
    bool CreateVertexBuffer(size_t size)
    {
        VkBufferCreateInfo bufferInfo = GetBufferCreateInfo(size);
        VkResult result = vkCreateBuffer(m_Device, &bufferInfo, nullptr, &m_VertexBuffer);
      
        VK_LOG_RESULT(result, "Creating vertex buffer");
      
        SEDX_CORE_INFO_TAG("BUFFER", "Created vertex buffer of size {}", size);
        return true;
    }
  
    void CreateCriticalBuffer()
    {
        VkBufferCreateInfo bufferInfo = GetCriticalBufferInfo();
        VkResult result = vkCreateBuffer(m_Device, &bufferInfo, nullptr, &m_CriticalBuffer);
      
        VK_LOG_RESULT_OR_THROW(result, "Creating critical system buffer");
    }
};
```

## Feature Validation System

### Feature Checking Macro

```cpp
#define CHECK_FEATURE(feature)
if (requiredFeatures.feature && !deviceFeatures.feature)
{
    missingFeatures = true;
    if (!missingFeaturesLog.empty()) missingFeaturesLog += ", ";
    missingFeaturesLog += #feature;
    SEDX_CORE_ERROR("  Missing feature: {}", #feature);
}
```

**Usage Example**:

```cpp
bool ValidateDeviceFeatures(VkPhysicalDeviceFeatures requiredFeatures, 
                          VkPhysicalDeviceFeatures deviceFeatures)
{
    bool missingFeatures = false;
    std::string missingFeaturesLog;
  
    // Check individual features
    CHECK_FEATURE(samplerAnisotropy);
    CHECK_FEATURE(fillModeNonSolid);
    CHECK_FEATURE(geometryShader);
    CHECK_FEATURE(tessellationShader);
  
    if (missingFeatures)
    {
        SEDX_CORE_ERROR_TAG("DEVICE", "Missing required features: {}", missingFeaturesLog);
        return false;
    }
  
    return true;
}
```

## Vulkan Logging System

### Specialized Vulkan Logging Macros

```cpp
#define VK_LOG_INFO(message)    // Logs informational Vulkan messages
#define VK_LOG_WARN(message)    // Logs Vulkan warnings  
#define VK_LOG_ERROR(message)   // Logs Vulkan errors
#define VK_LOG_DEBUG(message)   // Logs debug information
```

**Purpose**: Provides Vulkan-specific logging that integrates with the application's logging system.

**Usage Example**:

```cpp
class VulkanRenderer : public Module
{
public:
    void OnUpdate() override
    {
        SEDX_PROFILE_SCOPE("VulkanRenderer::OnUpdate");
      
        VK_LOG_DEBUG("Beginning frame rendering");
      
        VkResult result = BeginFrame();
        if (result != VK_SUCCESS)
        {
            VK_LOG_ERROR(fmt::format("Failed to begin frame: {}", vkErrorString(result)));
            return;
        }
      
        RenderScene();
      
        result = EndFrame();
        if (result != VK_SUCCESS)
        {
            VK_LOG_ERROR(fmt::format("Failed to end frame: {}", vkErrorString(result)));
        }
      
        VK_LOG_DEBUG("Frame rendering completed");
    }
};
```

## String Conversion Utilities

### Vulkan Enum to String Functions

```cpp
const char* vkDebugSeverityString(VkDebugUtilsMessageSeverityFlagBitsEXT severity);
const char* vkDebugType(VkDebugUtilsMessageTypeFlagsEXT type);
const char* vkErrorString(VkResult errorCode);
const char* vkFormatString(VkFormat format);
const char* vkDeviceTypeString(VkPhysicalDeviceType type);
const char* vkColorSpaceString(VkColorSpaceKHR colorSpace);
const char* vkQueueFlagsString(VkQueueFlags flags);
const char* vkMemoryPropertyFlagsString(VkMemoryPropertyFlags flags);
const char* VkObjectTypeToString(VkObjectType objectType);
```

**Usage Example**:

```cpp
void LogDeviceInfo(VkPhysicalDevice physicalDevice)
{
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
  
    SEDX_CORE_INFO_TAG("DEVICE", "Selected device: {} ({})", 
        properties.deviceName, 
        vkDeviceTypeString(properties.deviceType));
  
    SEDX_CORE_INFO_TAG("DEVICE", "Vendor: {}", 
        VendorIDToString(properties.vendorID));
}

void LogFormatSupport(VkFormat format, VkFormatProperties properties)
{
    SEDX_CORE_INFO_TAG("FORMAT", "Format {} support:", vkFormatString(format));
  
    if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
    {
        VK_LOG_INFO("  - Supports sampled images with optimal tiling");
    }
  
    if (properties.bufferFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT)
    {
        VK_LOG_INFO("  - Supports vertex buffer usage");
    }
}
```

## Utility Functions

### Format Information

```cpp
extern int getBPP(VkFormat format);
```

**Purpose**: Returns the bits per pixel for a given Vulkan format.

**Usage Example**:

```cpp
size_t CalculateImageSize(VkFormat format, uint32_t width, uint32_t height)
{
    int bitsPerPixel = getBPP(format);
    if (bitsPerPixel == -1)
    {
        SEDX_CORE_ERROR_TAG("FORMAT", "Unsupported format: {}", vkFormatString(format));
        return 0;
    }
  
    return (width * height * bitsPerPixel) / 8;
}
```

### Debug Object Naming

```cpp
static void SetDebugUtilsObjectName(const VkDevice device, 
                                   const VkObjectType objectType, 
                                   const std::string& name, 
                                   const void* handle);
```

**Usage Example**:

```cpp
class TextureManager : public Module
{
public:
    Ref<Texture2D> CreateTexture(const std::string& name, const TextureCreateInfo& info)
    {
        auto texture = CreateRef<Texture2D>();
      
        // Create the Vulkan objects
        CreateVulkanTexture(texture, info);
      
        // Name objects for debugging
        SetDebugUtilsObjectName(m_Device, VK_OBJECT_TYPE_IMAGE, 
            fmt::format("Texture_{}", name), texture->GetImage());
        SetDebugUtilsObjectName(m_Device, VK_OBJECT_TYPE_IMAGE_VIEW, 
            fmt::format("TextureView_{}", name), texture->GetImageView());
      
        SEDX_CORE_INFO_TAG("TEXTURE", "Created texture: {}", name);
        return texture;
    }
};
```

### Mathematical Utilities

```cpp
inline float HaltonSeq(uint32_t i, const uint32_t b);
```

**Purpose**: Generates Halton sequence values for temporal anti-aliasing and sampling.

**Usage Example**:

```cpp
class TemporalAntiAliasing : public Module
{
private:
    void GenerateJitterPattern()
    {
        for (uint32_t i = 0; i < m_SampleCount; ++i)
        {
            float x = HaltonSeq(i, 2);  // Base 2 for X
            float y = HaltonSeq(i, 3);  // Base 3 for Y
          
            m_JitterPattern[i] = Vec2(x - 0.5f, y - 0.5f);
        }
    }
  
    Vec2 GetCurrentJitter()
    {
        return m_JitterPattern[m_FrameIndex % m_SampleCount];
    }
};
```

## Resource Allocation Tracking

```cpp
namespace SceneryEditorX::Util
{
    struct ResourceAllocationCounts
    {
        uint32_t Samplers = 0;
    };
  
    ResourceAllocationCounts &GetResourceAllocationCounts();
}
```

**Usage Example**:

```cpp
class SamplerManager : public Module
{
public:
    Ref<Sampler> CreateSampler(const SamplerCreateInfo& info)
    {
        auto& counts = SceneryEditorX::Util::GetResourceAllocationCounts();
      
        if (counts.Samplers >= MAX_SAMPLERS)
        {
            SEDX_CORE_ERROR_TAG("SAMPLER", "Maximum sampler count exceeded: {}", counts.Samplers);
            return nullptr;
        }
      
        auto sampler = CreateRef<Sampler>();
        CreateVulkanSampler(sampler, info);
      
        counts.Samplers++;
        SEDX_CORE_INFO_TAG("SAMPLER", "Created sampler #{}", counts.Samplers);
      
        return sampler;
    }
  
    void DestroySampler(Ref<Sampler> sampler)
    {
        if (sampler)
        {
            DestroyVulkanSampler(sampler);
          
            auto& counts = SceneryEditorX::Util::GetResourceAllocationCounts();
            counts.Samplers--;
          
            SEDX_CORE_INFO_TAG("SAMPLER", "Destroyed sampler, remaining: {}", counts.Samplers);
        }
    }
};
```

## Integration with Module System

### Module Lifecycle Integration

```cpp
class VulkanModule : public Module
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("VULKAN", "=== Initializing Vulkan Module ===");
      
        try
        {
            // Initialize Vulkan instance
            VK_CHECK_RESULT(CreateVulkanInstance());
          
            // Load debug extensions
            VulkanLoadDebugUtilsExtensions(m_Instance);
          
            // Select physical device
            SelectPhysicalDevice();
          
            // Create logical device
            VK_CHECK_RESULT(CreateLogicalDevice());
          
            SEDX_CORE_INFO_TAG("VULKAN", "Vulkan module initialized successfully");
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("VULKAN", "Failed to initialize Vulkan: {}", e.what());
            throw;
        }
    }
  
    void OnDetach() override
    {
        SEDX_CORE_INFO_TAG("VULKAN", "=== Cleaning up Vulkan Module ===");
      
        // Cleanup is automatic with smart pointers and RAII
        // But we should wait for device idle
        if (m_Device)
        {
            VK_CHECK_RESULT(vkDeviceWaitIdle(m_Device));
        }
      
        SEDX_CORE_INFO_TAG("VULKAN", "Vulkan module cleanup completed");
    }
  
    void OnUpdate() override
    {
        SEDX_PROFILE_SCOPE("VulkanModule::OnUpdate");
      
        // Frame-based operations with error checking
        VkResult result = AcquireNextImage();
        VK_LOG_RESULT(result, "Acquiring next swapchain image");
    }
};
```

## Best Practices for Development

### Error Handling Strategy

1. **Use appropriate macros** for different scenarios:

   - `VK_CHECK_RESULT()` for critical operations
   - `VK_LOG_RESULT()` for operations that can fail gracefully
   - `VK_LOG_RESULT_OR_THROW()` for operations in exception-safe contexts
2. **Always validate device features** before using them:

```cpp
if (!ValidateRequiredFeatures())
{
    SEDX_CORE_ERROR_TAG("DEVICE", "Device does not support required features");
    return false;
}
```

3. **Use debug naming consistently**:

```cpp
// Name all your Vulkan objects for easier debugging
SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_BUFFER, "MainVertexBuffer", buffer);
```

### Performance Considerations

1. **Debug builds vs Release builds**: Debug utilities are designed to have minimal impact in release builds
2. **String formatting**: Use tagged logging to avoid string formatting in hot paths
3. **Error checking**: Critical path operations should use minimal error checking in release builds

### Thread Safety

- All utility functions are designed to be thread-safe when used appropriately
- Debug object naming should be done from the thread that owns the Vulkan context
- Logging functions are thread-safe through the underlying logging system

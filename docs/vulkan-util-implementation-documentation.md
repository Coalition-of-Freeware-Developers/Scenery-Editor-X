# Vulkan Utility Implementation Documentation (`vk_util.cpp`)

## Overview

The `vk_util.cpp` file provides the implementation of essential Vulkan utility functions that support debugging, error handling, format management, and device information retrieval throughout the Scenery Editor X application. This implementation file works in conjunction with `vk_util.h` to provide a comprehensive utility layer for Vulkan operations.

## Core Implementation Philosophy

The implementation follows these key principles:

1. **Comprehensive Error Reporting**: Every function provides detailed error information with appropriate logging
2. **Format Support**: Extensive support for all standard Vulkan formats with proper validation
3. **Debug Integration**: Seamless integration with the application's debug and logging systems
4. **Performance Awareness**: Efficient implementations that minimize overhead
5. **Maintainability**: Clear, well-documented code that's easy to extend and modify

## String Conversion Functions

### Device Information Functions

```cpp
const char* vkDeviceTypeString(const VkPhysicalDeviceType type)
{
    switch (type)
    {
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:  return "Integrated GPU";
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:    return "Discrete GPU";
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:     return "Virtual GPU";
    case VK_PHYSICAL_DEVICE_TYPE_CPU:             return "CPU";
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:           return "Other";
    default:                                      return "Unknown";
    }
}
```

**Purpose**: Converts Vulkan device types to human-readable strings for logging and debugging.

**Usage Example**:

```cpp
class DeviceSelector : public Module
{
public:
    void LogAvailableDevices()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
      
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());
      
        SEDX_CORE_INFO_TAG("DEVICE", "Available devices:");
        for (const auto& device : devices)
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);
          
            SEDX_CORE_INFO_TAG("DEVICE", "  - {} ({})", 
                properties.deviceName, 
                vkDeviceTypeString(properties.deviceType));
        }
    }
  
    VkPhysicalDevice SelectBestDevice()
    {
        // Prefer discrete GPU, fallback to integrated
        auto devices = EnumeratePhysicalDevices();
      
        for (const auto& device : devices)
        {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);
          
            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                SEDX_CORE_INFO_TAG("DEVICE", "Selected discrete GPU: {}", props.deviceName);
                return device;
            }
        }
      
        SEDX_CORE_WARN_TAG("DEVICE", "No discrete GPU found, using fallback");
        return devices[0];
    }
};
```

### Color Space and Queue Information

```cpp
const char* vkColorSpaceString(const VkColorSpaceKHR colorSpace)
const char* vkQueueFlagsString(const VkQueueFlags flags)  
const char* vkMemoryPropertyFlagsString(VkMemoryPropertyFlags flags)
```

**Usage Example**:

```cpp
class SwapchainManager : public Module
{
public:
    void LogSurfaceFormats(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
      
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, formats.data());
      
        SEDX_CORE_INFO_TAG("SWAPCHAIN", "Available surface formats:");
        for (const auto& format : formats)
        {
            SEDX_CORE_INFO_TAG("SWAPCHAIN", "  - Format: {}, Color Space: {}", 
                vkFormatString(format.format),
                vkColorSpaceString(format.colorSpace));
        }
    }
  
    void LogQueueFamilies(VkPhysicalDevice device)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
      
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
      
        SEDX_CORE_INFO_TAG("QUEUE", "Available queue families:");
        for (size_t i = 0; i < queueFamilies.size(); i++)
        {
            const auto& family = queueFamilies[i];
            SEDX_CORE_INFO_TAG("QUEUE", "  Family {}: {} queues, Flags: {}", 
                i, family.queueCount, vkQueueFlagsString(family.queueFlags));
        }
    }
};
```

### Debug Message Handling

```cpp
const char* vkDebugSeverityString(const VkDebugUtilsMessageSeverityFlagBitsEXT severity)
const char* vkDebugType(const VkDebugUtilsMessageTypeFlagsEXT type)
```

**Usage Example**:

```cpp
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    std::string severityStr = vkDebugSeverityString(messageSeverity);
    std::string typeStr = vkDebugType(messageType);
  
    switch (messageSeverity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            SEDX_CORE_ERROR_TAG("VULKAN", "[{}] [{}] {}", severityStr, typeStr, pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            SEDX_CORE_WARN_TAG("VULKAN", "[{}] [{}] {}", severityStr, typeStr, pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            SEDX_CORE_INFO_TAG("VULKAN", "[{}] [{}] {}", severityStr, typeStr, pCallbackData->pMessage);
            break;
        default:
            SEDX_CORE_DEBUG_TAG("VULKAN", "[{}] [{}] {}", severityStr, typeStr, pCallbackData->pMessage);
            break;
    }
  
    return VK_FALSE;
}
```

## Debug Extensions Implementation

### Loading Debug Utilities

```cpp
void VulkanLoadDebugUtilsExtensions(const VkInstance instance)
{
    fpSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
    if (fpSetDebugUtilsObjectNameEXT == nullptr)
    {
        fpSetDebugUtilsObjectNameEXT = [](VkDevice device, const VkDebugUtilsObjectNameInfoEXT *pNameInfo)
        {
            return VK_SUCCESS;
        };
    }
    // ... similar for other function pointers
}
```

**Purpose**: Safely loads debug extension function pointers with fallback implementations.

**Usage Example**:

```cpp
class VulkanInstance : public Module
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("VULKAN", "Creating Vulkan instance");
      
        // Create instance with debug extensions
        std::vector<const char*> extensions = {
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            // ... other extensions
        };
      
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
      
        VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &m_Instance));
      
        // Load debug utilities - essential for development
        VulkanLoadDebugUtilsExtensions(m_Instance);
      
        SEDX_CORE_INFO_TAG("VULKAN", "Debug extensions loaded successfully");
    }
};
```

## Format Information Functions

### Bits Per Pixel Calculation

```cpp
int32_t getBPP(const VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_R4G4_UNORM_PACK8:
        return 8;
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_UNORM:
        return 32;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return 64;
    // ... extensive format coverage
    case VK_FORMAT_UNDEFINED: 
    default:
        return -1;
    }
}
```

**Purpose**: Provides bit depth information for Vulkan formats, essential for memory calculations.

**Usage Example**:

```cpp
class TextureLoader : public Module
{
public:
    Ref<Texture2D> LoadTexture(const std::string& path)
    {
        // Load image data
        int width, height, channels;
        stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
      
        if (!pixels)
        {
            SEDX_CORE_ERROR_TAG("TEXTURE", "Failed to load texture: {}", path);
            return nullptr;
        }
      
        // Determine format based on channels
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        int bpp = getBPP(format);
      
        if (bpp == -1)
        {
            SEDX_CORE_ERROR_TAG("TEXTURE", "Unsupported format for texture: {}", path);
            stbi_image_free(pixels);
            return nullptr;
        }
      
        VkDeviceSize imageSize = width * height * (bpp / 8);
      
        SEDX_CORE_INFO_TAG("TEXTURE", "Loading texture {} ({}x{}, {} bytes, {} bpp)", 
            path, width, height, imageSize, bpp);
      
        auto texture = CreateTextureFromData(pixels, width, height, format);
        stbi_image_free(pixels);
      
        return texture;
    }
  
private:
    size_t CalculateMipLevelSize(uint32_t width, uint32_t height, VkFormat format, uint32_t mipLevel)
    {
        uint32_t mipWidth = std::max(1u, width >> mipLevel);
        uint32_t mipHeight = std::max(1u, height >> mipLevel);
        int bpp = getBPP(format);
      
        return mipWidth * mipHeight * (bpp / 8);
    }
};
```

## Access and Pipeline Stage Utilities

### Image Layout Access Patterns

```cpp
VkAccessFlags VK_ACCESS_FLAGS(const VkImageLayout layout)
{
    switch (layout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        return 0;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    // ... other layouts
    }
}

VkPipelineStageFlags VK_PIPELINE_STAGE_FLAGS(const VkImageLayout layout)
{
    switch (layout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    // ... other layouts
    }
}
```

**Purpose**: Automatically determines appropriate access masks and pipeline stages for image layout transitions.

**Usage Example**:

```cpp
class ImageTransitioner : public Module
{
public:
    void TransitionImageLayout(VkCommandBuffer cmd, VkImage image, 
                             VkImageLayout oldLayout, VkImageLayout newLayout,
                             uint32_t mipLevels = 1)
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
      
        // Automatically determine access patterns
        barrier.srcAccessMask = VK_ACCESS_FLAGS(oldLayout);
        barrier.dstAccessMask = VK_ACCESS_FLAGS(newLayout);
      
        // Set up subresource range
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
      
        // Automatically determine pipeline stages
        VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_FLAGS(oldLayout);
        VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_FLAGS(newLayout);
      
        vkCmdPipelineBarrier(
            cmd,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
      
        SEDX_CORE_DEBUG_TAG("IMAGE", "Transitioned image layout from {} to {}",
            static_cast<int>(oldLayout), static_cast<int>(newLayout));
    }
  
    void PrepareTextureForShaderAccess(VkCommandBuffer cmd, VkImage image)
    {
        TransitionImageLayout(cmd, image, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
};
```

## Format String Conversion

### Comprehensive Format Support

```cpp
const char *vkFormatString(const VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_UNDEFINED:                     return "VK_FORMAT_UNDEFINED";
    case VK_FORMAT_R4G4_UNORM_PACK8:             return "VK_FORMAT_R4G4_UNORM_PACK8";
    case VK_FORMAT_R8G8B8A8_UNORM:               return "VK_FORMAT_R8G8B8A8_UNORM";
    case VK_FORMAT_B8G8R8A8_UNORM:               return "VK_FORMAT_B8G8R8A8_UNORM";
    // ... comprehensive format coverage including extensions
    case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK:      return "VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK";
    default:                                      return "UNKNOWN";
    }
}
```

**Usage Example**:

```cpp
class FormatValidator : public Module
{
public:
    bool ValidateFormatSupport(VkPhysicalDevice physicalDevice, VkFormat format, 
                              VkImageTiling tiling, VkFormatFeatureFlags features)
    {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
      
        VkFormatFeatureFlags availableFeatures = (tiling == VK_IMAGE_TILING_LINEAR) ?
            formatProps.linearTilingFeatures : formatProps.optimalTilingFeatures;
      
        if ((availableFeatures & features) != features)
        {
            SEDX_CORE_ERROR_TAG("FORMAT", "Format {} does not support required features for {} tiling",
                vkFormatString(format), 
                (tiling == VK_IMAGE_TILING_LINEAR) ? "linear" : "optimal");
            return false;
        }
      
        SEDX_CORE_INFO_TAG("FORMAT", "Format {} supports all required features", 
            vkFormatString(format));
        return true;
    }
  
    VkFormat FindSupportedDepthFormat(VkPhysicalDevice physicalDevice)
    {
        std::vector<VkFormat> candidates = {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        };
      
        for (VkFormat format : candidates)
        {
            if (ValidateFormatSupport(physicalDevice, format, VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
            {
                SEDX_CORE_INFO_TAG("FORMAT", "Selected depth format: {}", vkFormatString(format));
                return format;
            }
        }
      
        SEDX_CORE_ERROR_TAG("FORMAT", "Failed to find supported depth format");
        return VK_FORMAT_UNDEFINED;
    }
};
```

## Error String Conversion

### Comprehensive Error Handling

```cpp
const char *vkErrorString(const VkResult errorCode)
{
    switch (errorCode)
    {
    case VK_SUCCESS:                            return "VK_SUCCESS";
    case VK_ERROR_OUT_OF_HOST_MEMORY:           return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:         return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_DEVICE_LOST:                  return "VK_ERROR_DEVICE_LOST";
    // ... comprehensive error code coverage
    case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:    return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
    default:                                    return "Unknown VkResult";
    }
}
```

**Usage Example**:

```cpp
class ErrorHandler : public Module
{
public:
    void HandleVulkanError(VkResult result, const std::string& operation)
    {
        if (result == VK_SUCCESS)
            return;
      
        std::string errorStr = vkErrorString(result);
      
        // Different handling based on error type
        switch (result)
        {
            case VK_ERROR_DEVICE_LOST:
                SEDX_CORE_CRITICAL_TAG("VULKAN", "Device lost during {}: {}", operation, errorStr);
                // Attempt device recovery
                AttemptDeviceRecovery();
                break;
              
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                SEDX_CORE_ERROR_TAG("VULKAN", "Out of device memory during {}: {}", operation, errorStr);
                // Attempt memory cleanup
                CleanupDeviceMemory();
                break;
              
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                SEDX_CORE_ERROR_TAG("VULKAN", "Out of host memory during {}: {}", operation, errorStr);
                // Attempt host memory cleanup
                CleanupHostMemory();
                break;
              
            default:
                SEDX_CORE_ERROR_TAG("VULKAN", "Error during {}: {}", operation, errorStr);
                break;
        }
    }
  
    bool TryOperation(const std::function<VkResult()>& operation, const std::string& description)
    {
        VkResult result = operation();
        if (result != VK_SUCCESS)
        {
            HandleVulkanError(result, description);
            return false;
        }
        return true;
    }
};
```

## Vendor and Device Information

### Vendor Identification

```cpp
const char* VendorIDToString(const uint32_t vendorID)
{
    switch (vendorID)
    {
    case 0x10DE: return "NVIDIA";
    case 0x1002: return "AMD";
    case 0x8086: return "INTEL";
    case 0x13B5: return "ARM";
    }
    return "Unknown";
}
```

**Usage Example**:

```cpp
class DeviceProfiler : public Module
{
public:
    void ProfileDevice(VkPhysicalDevice device)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
      
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(device, &memProperties);
      
        std::string vendor = VendorIDToString(properties.vendorID);
        std::string deviceType = vkDeviceTypeString(properties.deviceType);
      
        SEDX_CORE_INFO_TAG("DEVICE", "=== Device Profile ===");
        SEDX_CORE_INFO_TAG("DEVICE", "Name: {}", properties.deviceName);
        SEDX_CORE_INFO_TAG("DEVICE", "Vendor: {} (0x{:X})", vendor, properties.vendorID);
        SEDX_CORE_INFO_TAG("DEVICE", "Type: {}", deviceType);
        SEDX_CORE_INFO_TAG("DEVICE", "API Version: {}.{}.{}", 
            VK_VERSION_MAJOR(properties.apiVersion),
            VK_VERSION_MINOR(properties.apiVersion),
            VK_VERSION_PATCH(properties.apiVersion));
      
        // Log memory heaps
        SEDX_CORE_INFO_TAG("DEVICE", "Memory Heaps:");
        for (uint32_t i = 0; i < memProperties.memoryHeapCount; i++)
        {
            const auto& heap = memProperties.memoryHeaps[i];
            SEDX_CORE_INFO_TAG("DEVICE", "  Heap {}: {} MB, Flags: {}", 
                i, heap.size / (1024 * 1024), heap.flags);
        }
      
        // Apply vendor-specific optimizations
        ApplyVendorOptimizations(vendor);
    }
  
private:
    void ApplyVendorOptimizations(const std::string& vendor)
    {
        if (vendor == "NVIDIA")
        {
            SEDX_CORE_INFO_TAG("DEVICE", "Applying NVIDIA-specific optimizations");
            // Enable NVIDIA-specific features
        }
        else if (vendor == "AMD")
        {
            SEDX_CORE_INFO_TAG("DEVICE", "Applying AMD-specific optimizations");
            // Enable AMD-specific features
        }
        // ... other vendors
    }
};
```

## Resource Allocation Tracking

### Allocation Counter Implementation

```cpp
namespace SceneryEditorX::Util
{
    LOCAL ResourceAllocationCounts s_ResourceAllocationCounts;
  
    ResourceAllocationCounts &GetResourceAllocationCounts()
    {
        return s_ResourceAllocationCounts;
    }
}
```

**Usage Example**:

```cpp
class ResourceMonitor : public Module
{
public:
    void OnUIRender() override
    {
        if (ImGui::Begin("Resource Monitor"))
        {
            auto& counts = SceneryEditorX::Util::GetResourceAllocationCounts();
          
            ImGui::Text("Resource Allocation Status:");
            ImGui::Separator();
          
            ImGui::Text("Samplers: %u", counts.Samplers);
          
            // Progress bars for resource usage
            float samplerUsage = static_cast<float>(counts.Samplers) / MAX_SAMPLERS;
            ImGui::ProgressBar(samplerUsage, ImVec2(-1, 0), 
                fmt::format("{}/{}", counts.Samplers, MAX_SAMPLERS).c_str());
          
            if (samplerUsage > 0.8f)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Warning: High sampler usage!");
            }
        }
        ImGui::End();
    }
  
    void LogResourceUsage()
    {
        auto& counts = SceneryEditorX::Util::GetResourceAllocationCounts();
      
        SEDX_CORE_INFO_TAG("RESOURCES", "Current resource allocation:");
        SEDX_CORE_INFO_TAG("RESOURCES", "  Samplers: {}/{}", counts.Samplers, MAX_SAMPLERS);
      
        if (counts.Samplers > MAX_SAMPLERS * 0.8)
        {
            SEDX_CORE_WARN_TAG("RESOURCES", "Approaching sampler limit");
        }
    }
};
```

## Integration Best Practices

### Module Lifecycle Integration

```cpp
class VulkanUtilityModule : public Module
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("UTIL", "=== Initializing Vulkan Utilities ===");
      
        // Validate that all required functions are available
        ValidateUtilityFunctions();
      
        // Initialize resource tracking
        ResetResourceCounts();
      
        SEDX_CORE_INFO_TAG("UTIL", "Vulkan utilities initialized");
    }
  
    void OnDetach() override
    {
        SEDX_CORE_INFO_TAG("UTIL", "=== Vulkan Utilities Cleanup ===");
      
        // Log final resource counts
        LogFinalResourceUsage();
    }
  
private:
    void ValidateUtilityFunctions()
    {
        // Test format string functions
        const char* testFormat = vkFormatString(VK_FORMAT_R8G8B8A8_UNORM);
        SEDX_CORE_ASSERT(testFormat != nullptr);
      
        // Test error string functions
        const char* testError = vkErrorString(VK_SUCCESS);
        SEDX_CORE_ASSERT(testError != nullptr);
      
        SEDX_CORE_INFO_TAG("UTIL", "All utility functions validated");
    }
  
    void ResetResourceCounts()
    {
        auto& counts = SceneryEditorX::Util::GetResourceAllocationCounts();
        counts.Samplers = 0;
      
        SEDX_CORE_INFO_TAG("UTIL", "Resource counters reset");
    }
  
    void LogFinalResourceUsage()
    {
        auto& counts = SceneryEditorX::Util::GetResourceAllocationCounts();
      
        SEDX_CORE_INFO_TAG("UTIL", "Final resource usage:");
        SEDX_CORE_INFO_TAG("UTIL", "  Samplers: {}", counts.Samplers);
      
        if (counts.Samplers > 0)
        {
            SEDX_CORE_WARN_TAG("UTIL", "Warning: {} samplers not cleaned up", counts.Samplers);
        }
    }
};
```

## Performance Considerations

1. **String Functions**: All string conversion functions are designed for debugging and logging, not performance-critical paths
2. **Format Detection**: `getBPP()` function is optimized for common formats first
3. **Memory Overhead**: Resource tracking has minimal memory footprint
4. **Debug vs Release**: Debug utilities have zero impact in release builds where appropriate

## Thread Safety

- All string conversion functions are thread-safe (read-only operations)
- Resource allocation counters should be accessed from the main thread only
- Debug utilities are thread-safe when used with the appropriate Vulkan context

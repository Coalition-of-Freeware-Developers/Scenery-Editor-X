# Vulkan Utility System Instructions for GitHub Copilot

## Overview

This document provides comprehensive instructions for GitHub Copilot on how to properly utilize, extend, and maintain the Vulkan utility system within Scenery Editor X. The system consists of three core files: `vk_enums.h`, `vk_util.h`, and `vk_util.cpp`, which provide the foundation for all Vulkan operations in the application.

## Core Principles for Vulkan Utility Development

### 1. Always Use the Enumeration System

**DO**:
```cpp
// Use type-safe enums from vk_enums.h
ResourceAccessFlags access = ResourceAccessFlags::ShaderRead | ResourceAccessFlags::ShaderWrite;
PipelineStage stage = PipelineStage::FragmentShader;
```

**DON'T**:
```cpp
// Avoid raw Vulkan enums
VkAccessFlags access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
VkPipelineStageFlags stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
```

### 2. Mandatory Error Checking

**ALWAYS** use the provided error checking macros for Vulkan operations:

```cpp
// For critical operations that must succeed
VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));

// For operations that can fail gracefully
VkResult result = vkSomeOperation();
VK_LOG_RESULT(result, "Description of operation");

// For exception-safe contexts
VK_LOG_RESULT_OR_THROW(result, "Critical operation description");
```

### 3. Comprehensive Logging Integration

**ALWAYS** use tagged logging for Vulkan operations:

```cpp
class VulkanModule : public Module
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("VULKAN", "=== Initializing Vulkan Module ===");
        
        // Use specific tags for different subsystems
        SEDX_CORE_INFO_TAG("DEVICE", "Selecting physical device");
        SEDX_CORE_INFO_TAG("MEMORY", "Allocating device memory");
        SEDX_CORE_INFO_TAG("PIPELINE", "Creating graphics pipeline");
    }
};
```

### 4. Debug Object Naming

**ALWAYS** name Vulkan objects for debugging:

```cpp
void CreateBuffer(const std::string& name, VkBuffer& buffer)
{
    VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));
    
    // Essential for debugging - name all objects
    SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_BUFFER, 
        fmt::format("Buffer_{}", name), buffer);
}
```

## Module Integration Patterns

### Standard Vulkan Module Structure

When creating Vulkan-related modules, follow this pattern:

```cpp
class VulkanResourceModule : public Module
{
public:
    explicit VulkanResourceModule(const std::string& name = "VulkanResourceModule")
        : Module(name)
    {
    }
    
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("VULKAN", "=== Initializing {} ===", GetName());
        
        try
        {
            // Use error checking macros
            InitializeVulkanResources();
            SEDX_CORE_INFO_TAG("VULKAN", "{} initialized successfully", GetName());
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("VULKAN", "Failed to initialize {}: {}", GetName(), e.what());
            throw;
        }
    }
    
    void OnDetach() override
    {
        SEDX_CORE_INFO_TAG("VULKAN", "=== Cleaning up {} ===", GetName());
        
        // Wait for device idle before cleanup
        if (m_Device)
        {
            VK_CHECK_RESULT(vkDeviceWaitIdle(m_Device));
        }
        
        // Smart pointers handle cleanup automatically
        SEDX_CORE_INFO_TAG("VULKAN", "{} cleanup completed", GetName());
    }
    
    void OnUpdate() override
    {
        if (!m_IsEnabled) return;
        SEDX_PROFILE_SCOPE("VulkanResourceModule::OnUpdate");
        
        // Frame-based operations with error checking
        PerformFrameOperations();
    }
    
private:
    bool m_IsEnabled = true;
    VkDevice m_Device = VK_NULL_HANDLE;
    
    void InitializeVulkanResources()
    {
        // Always validate prerequisites
        SEDX_CORE_ASSERT(m_Device != VK_NULL_HANDLE, "Device must be valid");
        
        // Use utility functions for format validation
        ValidateFormats();
        
        // Create resources with proper error handling
        CreateBuffers();
        CreateImages();
        CreateDescriptorSets();
    }
};
```

## Format and Validation Patterns

### Format Selection and Validation

```cpp
class FormatManager : public Module
{
public:
    VkFormat SelectOptimalFormat(const std::vector<VkFormat>& candidates,
                                VkImageTiling tiling,
                                VkFormatFeatureFlags features)
    {
        for (VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);
            
            VkFormatFeatureFlags availableFeatures = (tiling == VK_IMAGE_TILING_LINEAR) ?
                props.linearTilingFeatures : props.optimalTilingFeatures;
            
            if ((availableFeatures & features) == features)
            {
                SEDX_CORE_INFO_TAG("FORMAT", "Selected format: {}", vkFormatString(format));
                return format;
            }
        }
        
        SEDX_CORE_ERROR_TAG("FORMAT", "Failed to find suitable format");
        return VK_FORMAT_UNDEFINED;
    }
    
    size_t CalculateImageSize(VkFormat format, uint32_t width, uint32_t height, uint32_t mipLevels = 1)
    {
        int bpp = getBPP(format);
        if (bpp == -1)
        {
            SEDX_CORE_ERROR_TAG("FORMAT", "Unsupported format: {}", vkFormatString(format));
            return 0;
        }
        
        size_t totalSize = 0;
        for (uint32_t mip = 0; mip < mipLevels; ++mip)
        {
            uint32_t mipWidth = std::max(1u, width >> mip);
            uint32_t mipHeight = std::max(1u, height >> mip);
            totalSize += mipWidth * mipHeight * (bpp / 8);
        }
        
        return totalSize;
    }
};
```

## Resource Management Best Practices

### Buffer Creation Pattern

```cpp
class BufferManager : public Module
{
public:
    Ref<Buffer> CreateBuffer(const std::string& name, VkDeviceSize size, 
                           VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    {
        auto buffer = CreateRef<Buffer>();
        
        // Create buffer with error checking
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        VK_CHECK_RESULT(vkCreateBuffer(m_Device, &bufferInfo, nullptr, &buffer->buffer));
        
        // Name the buffer for debugging
        SetDebugUtilsObjectName(m_Device, VK_OBJECT_TYPE_BUFFER, 
            fmt::format("Buffer_{}", name), buffer->buffer);
        
        // Allocate memory
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_Device, buffer->buffer, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
        
        VK_CHECK_RESULT(vkAllocateMemory(m_Device, &allocInfo, nullptr, &buffer->memory));
        VK_CHECK_RESULT(vkBindBufferMemory(m_Device, buffer->buffer, buffer->memory, 0));
        
        SEDX_CORE_INFO_TAG("BUFFER", "Created buffer '{}' (size: {} bytes)", name, size);
        
        return buffer;
    }
    
private:
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);
        
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && 
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        
        SEDX_CORE_ERROR_TAG("MEMORY", "Failed to find suitable memory type");
        SEDX_CORE_ASSERT(false, "Failed to find suitable memory type");
        return 0;
    }
};
```

### Image Layout Transition Pattern

```cpp
class ImageTransitioner : public Module
{
public:
    void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image,
                             VkImageLayout oldLayout, VkImageLayout newLayout,
                             uint32_t mipLevels = 1, uint32_t layerCount = 1)
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        
        // Use utility functions for automatic access mask determination
        barrier.srcAccessMask = VK_ACCESS_FLAGS(oldLayout);
        barrier.dstAccessMask = VK_ACCESS_FLAGS(newLayout);
        
        // Set up subresource range
        barrier.subresourceRange.aspectMask = DetermineAspectMask(newLayout);
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = layerCount;
        
        // Use utility functions for automatic pipeline stage determination
        VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_FLAGS(oldLayout);
        VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_FLAGS(newLayout);
        
        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
        
        SEDX_CORE_DEBUG_TAG("IMAGE", "Transitioned image layout: {} -> {}",
            static_cast<int>(oldLayout), static_cast<int>(newLayout));
    }
    
private:
    VkImageAspectFlags DetermineAspectMask(VkImageLayout layout)
    {
        if (layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
            layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
        {
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }
};
```

## Error Handling Strategies

### Device Lost Recovery Pattern

```cpp
class VulkanErrorHandler : public Module
{
public:
    void HandleDeviceLost()
    {
        SEDX_CORE_CRITICAL_TAG("VULKAN", "Device lost detected - attempting recovery");
        
        // Wait before attempting recovery
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(3s);
        
        // Retrieve diagnostic information
        RetrieveDiagnosticCheckpoints();
        
        // Attempt device recreation
        RecreateDevice();
    }
    
    bool TryVulkanOperation(const std::function<VkResult()>& operation, 
                           const std::string& description, 
                           int maxRetries = 3)
    {
        for (int attempt = 0; attempt < maxRetries; ++attempt)
        {
            VkResult result = operation();
            
            if (result == VK_SUCCESS)
            {
                return true;
            }
            
            if (result == VK_ERROR_DEVICE_LOST)
            {
                SEDX_CORE_ERROR_TAG("VULKAN", "Device lost during: {}", description);
                HandleDeviceLost();
                continue;
            }
            
            SEDX_CORE_ERROR_TAG("VULKAN", "Failed {}: {} (attempt {}/{})", 
                description, vkErrorString(result), attempt + 1, maxRetries);
            
            if (attempt < maxRetries - 1)
            {
                std::this_thread::sleep_for(100ms);
            }
        }
        
        return false;
    }
};
```

## Resource Tracking and Monitoring

### Resource Allocation Monitoring

```cpp
class ResourceMonitor : public Module
{
public:
    void OnUIRender() override
    {
        if (ImGui::Begin("Vulkan Resource Monitor"))
        {
            auto& counts = SceneryEditorX::Util::GetResourceAllocationCounts();
            
            // Resource usage display
            ImGui::Text("Resource Allocation Status");
            ImGui::Separator();
            
            // Samplers
            float samplerUsage = static_cast<float>(counts.Samplers) / MAX_SAMPLERS;
            ImGui::Text("Samplers: %u/%u", counts.Samplers, MAX_SAMPLERS);
            ImGui::ProgressBar(samplerUsage, ImVec2(-1, 0));
            
            if (samplerUsage > 0.8f)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), 
                    "Warning: High sampler usage!");
            }
            
            // Add warning for approaching limits
            if (samplerUsage > 0.9f)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), 
                    "Critical: Sampler limit almost reached!");
            }
        }
        ImGui::End();
    }
    
    void ValidateResourceLimits()
    {
        auto& counts = SceneryEditorX::Util::GetResourceAllocationCounts();
        
        if (counts.Samplers > MAX_SAMPLERS * 0.8)
        {
            SEDX_CORE_WARN_TAG("RESOURCES", "Approaching sampler limit: {}/{}", 
                counts.Samplers, MAX_SAMPLERS);
        }
    }
};
```

## Device Feature Validation

### Comprehensive Feature Checking

```cpp
class DeviceValidator : public Module
{
public:
    bool ValidateDeviceRequirements(VkPhysicalDevice device)
    {
        VkPhysicalDeviceFeatures availableFeatures;
        vkGetPhysicalDeviceFeatures(device, &availableFeatures);
        
        VkPhysicalDeviceFeatures requiredFeatures{};
        requiredFeatures.samplerAnisotropy = VK_TRUE;
        requiredFeatures.fillModeNonSolid = VK_TRUE;
        requiredFeatures.geometryShader = VK_TRUE;
        
        bool missingFeatures = false;
        std::string missingFeaturesLog;
        
        // Use the CHECK_FEATURE macro from vk_util.h
        CHECK_FEATURE(samplerAnisotropy);
        CHECK_FEATURE(fillModeNonSolid);
        CHECK_FEATURE(geometryShader);
        CHECK_FEATURE(tessellationShader);
        
        if (missingFeatures)
        {
            SEDX_CORE_ERROR_TAG("DEVICE", "Device validation failed. Missing features: {}", 
                missingFeaturesLog);
            return false;
        }
        
        SEDX_CORE_INFO_TAG("DEVICE", "Device validation passed");
        return true;
    }
};
```

## Key Requirements for Copilot Development

### 1. Mandatory Patterns

- **ALWAYS** use `VK_CHECK_RESULT()` for Vulkan function calls
- **ALWAYS** use tagged logging with appropriate subsystem tags
- **ALWAYS** name Vulkan objects with `SetDebugUtilsObjectName()`
- **ALWAYS** inherit from `Module` for Vulkan-related components
- **ALWAYS** use the custom smart pointer system (`CreateRef<T>()`, `Ref<T>`)

### 2. Error Handling Requirements

- **NEVER** ignore Vulkan result codes
- **ALWAYS** provide meaningful error messages with context
- **ALWAYS** use the string conversion utilities (`vkErrorString()`, `vkFormatString()`, etc.)
- **ALWAYS** handle device lost scenarios appropriately

### 3. Resource Management Requirements

- **ALWAYS** use RAII principles with smart pointers
- **ALWAYS** track resource allocations
- **ALWAYS** validate resource limits before allocation
- **ALWAYS** clean up resources in reverse order of creation

### 4. Development Workflow

- **ALWAYS** test with Vulkan validation layers enabled
- **ALWAYS** verify debug object naming in RenderDoc/similar tools
- **ALWAYS** check resource limits and allocation counts
- **ALWAYS** profile memory usage and performance impact

### 5. Documentation Requirements

- **ALWAYS** document public functions with Doxygen format
- **ALWAYS** provide usage examples for complex operations
- **ALWAYS** explain the purpose and context of Vulkan operations
- **ALWAYS** include error handling examples

This instruction set ensures that all Vulkan-related code in Scenery Editor X follows consistent patterns, provides robust error handling, and integrates seamlessly with the application's module architecture and logging systems.

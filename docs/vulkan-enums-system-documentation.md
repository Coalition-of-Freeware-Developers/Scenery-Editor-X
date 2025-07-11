# Vulkan Enums System Documentation

## Overview

The `vk_enums.h` file provides a comprehensive collection of enumeration classes and constants that abstract Vulkan-specific enumerations into a more user-friendly format for the Scenery Editor X application. These enumerations serve as type-safe wrappers around Vulkan's native enum values while maintaining compatibility with the underlying Vulkan API.

## Purpose and Design Philosophy

The enumeration system serves several critical purposes:

1. **Type Safety**: Provides strongly-typed enums that prevent implicit conversions and catch errors at compile time
2. **Abstraction**: Abstracts away Vulkan's verbose naming conventions with more readable identifiers
3. **Consistency**: Maintains consistent naming patterns across the entire rendering system
4. **Documentation**: Each enum is thoroughly documented with its purpose and usage context
5. **Future-Proofing**: Allows for easy extension and modification without breaking existing code

## Core Enumerations

### ResourceAccessFlags

```cpp
enum class ResourceAccessFlags
{
    None                        = 0,
    IndirectCommandRead         = 0x00000001,
    IndexRead                   = 0x00000002,
    VertexAttributeRead         = 0x00000004,
    UniformRead                 = 0x00000008,
    InputAttachmentRead         = 0x00000010,
    ShaderRead                  = 0x00000020,
    ShaderWrite                 = 0x00000040,
    ColorAttachmentRead         = 0x00000080,
    ColorAttachmentWrite        = 0x00000100,
    DepthStencilAttachmentRead  = 0x00000200,
    DepthStencilAttachmentWrite = 0x00000400,
    TransferRead                = 0x00000800,
    TransferWrite               = 0x00001000,
    HostRead                    = 0x00002000,
    HostWrite                   = 0x00004000,
    MemoryRead                  = 0x00008000,
    MemoryWrite                 = 0x00010000,
};
```

**Purpose**: Defines access patterns for Vulkan resources during synchronization operations.

**Usage Example**:
```cpp
// Setting up memory barriers for texture access
ResourceAccessFlags sourceAccess = ResourceAccessFlags::TransferWrite;
ResourceAccessFlags destinationAccess = ResourceAccessFlags::ShaderRead;

// Use in pipeline barriers
VkMemoryBarrier barrier{};
barrier.srcAccessMask = static_cast<VkAccessFlags>(sourceAccess);
barrier.dstAccessMask = static_cast<VkAccessFlags>(destinationAccess);
```

**Best Practices**:
- Use bitwise OR operations to combine multiple access flags
- Always specify appropriate source and destination access masks for barriers
- Consider the actual usage patterns of your resources

### PipelineStage

```cpp
enum class PipelineStage
{
    None                        = 0,
    TopOfPipe                   = 0x00000001,
    DrawIndirect                = 0x00000002,
    VertexInput                 = 0x00000004,
    VertexShader                = 0x00000008,
    TesselationControlShader    = 0x00000010,
    TesselationEvaluationShader = 0x00000020,
    GeometryShader              = 0x00000040,
    FragmentShader              = 0x00000080,
    EarlyFragmentTests          = 0x00000100,
    LateFragmentTests           = 0x00000200,
    ColorAttachmentOutput       = 0x00000400,
    ComputeShader               = 0x00000800,
    Transfer                    = 0x00001000,
    BottomOfPipe                = 0x00002000,
    Host                        = 0x00004000,
    AllGraphics                 = 0x00008000,
    AllCommands                 = 0x00010000
};
```

**Purpose**: Represents different stages of the Vulkan graphics and compute pipeline for synchronization.

**Usage Example**:
```cpp
// Synchronizing between vertex and fragment shader stages
PipelineStage sourceStage = PipelineStage::VertexShader;
PipelineStage destinationStage = PipelineStage::FragmentShader;

// Create pipeline barrier
VkMemoryBarrier barrier{};
barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
// Use in vkCmdPipelineBarrier with appropriate stage masks
```

### ShaderStage

```cpp
namespace ShaderStage
{
    enum class Stage
    {
        Vertex      = 0x00000001,
        Geometry    = 0x00000008,
        Fragment    = 0x00000010,
        Compute     = 0x00000020,
        AllGraphics = 0x0000001F,
        All         = 0x7FFFFFFF,
    };
}
```

**Purpose**: Identifies specific shader stages for resource binding and pipeline creation.

**Usage Example**:
```cpp
// Creating descriptor set layout bindings for different shader stages
VkDescriptorSetLayoutBinding binding{};
binding.stageFlags = static_cast<VkShaderStageFlags>(ShaderStage::Stage::Vertex | ShaderStage::Stage::Fragment);
binding.binding = 0;
binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
```

### Resource and Texture Enumerations

```cpp
enum class ResourceType : uint16_t
{
    None = 0,
    UniformBuffer,
    UniformBufferSet,
    StorageBuffer,
    StorageBufferSet,
    Texture2D,
    TextureCube,
    Image2D
};

enum class ResourceInputType : uint16_t
{
    None = 0,
    UniformBuffer,
    StorageBuffer,
    ImageSampler1D,
    ImageSampler2D,
    ImageSampler3D,
    StorageImage1D,
    StorageImage2D,
    StorageImage3D	    
};
```

**Usage Example**:
```cpp
// Resource binding in shader
class ShaderResource
{
public:
    void BindResource(ResourceType type, uint32_t binding)
    {
        switch(type)
        {
            case ResourceType::UniformBuffer:
                BindUniformBuffer(binding);
                break;
            case ResourceType::Texture2D:
                BindTexture2D(binding);
                break;
            // ... other cases
        }
    }
};
```

### Rendering Configuration Enums

```cpp
enum SamplerFilter : uint8_t
{
    SamplerFilter_Nearest = 0,
    SamplerFilter_Linear
};

enum SamplerWrap : uint8_t
{
    SamplerWrap_Repeat = 0,
    SamplerWrap_Clamp,
    SamplerWrap_MirrorRepeat
};

enum PolygonMode : uint8_t
{
    PolygonMode_Fill = 0,
    PolygonMode_Line = 1,
};
```

**Usage Example**:
```cpp
// Creating sampler configuration
VkSamplerCreateInfo samplerInfo{};
samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
samplerInfo.magFilter = static_cast<VkFilter>(SamplerFilter_Linear);
samplerInfo.minFilter = static_cast<VkFilter>(SamplerFilter_Linear);
samplerInfo.addressModeU = static_cast<VkSamplerAddressMode>(SamplerWrap_Repeat);
samplerInfo.addressModeV = static_cast<VkSamplerAddressMode>(SamplerWrap_Repeat);
```

### Framebuffer and Attachment Enums

```cpp
enum class FramebufferBlendMode : uint8_t
{
    None = 0,
    OneZero,
    SrcAlphaOneMinusSrcAlpha,
    Additive,
    Zero_SrcColor
};

enum class AttachmentLoadOp : uint8_t
{
    Inherit = 0,
    Clear   = 1,
    Load    = 2
};
```

**Usage Example**:
```cpp
// Configuring render pass attachments
VkAttachmentDescription colorAttachment{};
colorAttachment.loadOp = static_cast<VkAttachmentLoadOp>(AttachmentLoadOp::Clear);
colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

// Setting up blend state
VkPipelineColorBlendAttachmentState blendAttachment{};
switch(blendMode)
{
    case FramebufferBlendMode::SrcAlphaOneMinusSrcAlpha:
        blendAttachment.blendEnable = VK_TRUE;
        blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        break;
    // ... other blend modes
}
```

## Integration Patterns

### Module Integration

When using these enums in modules, follow the established patterns:

```cpp
class RenderModule : public Module
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("RENDER", "Initializing render module");
        
        // Create resources using the enum system
        CreateRenderTargets();
        SetupShaderResources();
    }

private:
    void CreateRenderTargets()
    {
        // Use enums for type-safe resource creation
        for(auto& target : m_RenderTargets)
        {
            target.SetLoadOp(AttachmentLoadOp::Clear);
            target.SetBlendMode(FramebufferBlendMode::SrcAlphaOneMinusSrcAlpha);
        }
    }
    
    void SetupShaderResources()
    {
        // Type-safe shader stage specification
        m_ShaderBinding.stages = ShaderStage::Stage::Vertex | ShaderStage::Stage::Fragment;
        m_ShaderBinding.resourceType = ResourceType::UniformBuffer;
    }
};
```

### Error Handling with Enums

```cpp
bool ValidateResourceAccess(ResourceAccessFlags access, ResourceType type)
{
    // Validate that access patterns match resource types
    if (type == ResourceType::UniformBuffer)
    {
        if (access & ResourceAccessFlags::ShaderWrite)
        {
            SEDX_CORE_ERROR_TAG("VALIDATION", "Uniform buffers should not have write access");
            return false;
        }
    }
    
    return true;
}
```

## Performance Considerations

1. **Compile-Time Optimization**: All enums are designed for compile-time resolution
2. **Cache-Friendly**: Enum values are designed to minimize branching in hot paths
3. **Memory Efficient**: Most enums use 8 or 16-bit storage to reduce memory footprint

## Best Practices for Copilot Development

1. **Always use the enum classes** instead of raw Vulkan enums for type safety
2. **Document enum usage** with clear examples in your code
3. **Use bitwise operations** appropriately for flag-based enums
4. **Validate enum values** before casting to Vulkan types
5. **Follow the established naming patterns** when extending enums
6. **Use tagged logging** when working with enum-related errors

## Extension Guidelines

When adding new enums:

1. Follow the existing naming conventions
2. Provide comprehensive documentation
3. Include usage examples
4. Consider forward compatibility
5. Add appropriate validation functions
6. Update related utility functions

## Thread Safety

All enums in this system are inherently thread-safe as they are compile-time constants. However, when using them in multi-threaded contexts:

- Ensure proper synchronization when modifying enum-dependent state
- Use appropriate memory barriers when enum values affect GPU state
- Consider using atomic operations for enum-based flags in concurrent scenarios

This enumeration system forms the foundation for type-safe, maintainable Vulkan code throughout the Scenery Editor X application.

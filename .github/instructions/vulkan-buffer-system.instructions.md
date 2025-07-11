# Vulkan Buffer System Instructions for GitHub Copilot

## Overview

These instructions guide GitHub Copilot in generating code that follows the Vulkan Buffer System architecture and best practices in Scenery Editor X. All buffer-related code should adhere to these patterns and conventions.

## Core Buffer System Principles

### 1. Always Use the Module System

All buffer management components must inherit from the Module base class:

```cpp
class BufferModule : public Module
{
public:
    explicit BufferModule(const std::string& name = "BufferModule")
        : Module(name)
    {
    }
    
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing {} ===", GetName());
        // Initialize buffers using CreateBuffer()
        CreateRenderingBuffers();
    }
    
    void OnDetach() override
    {
        SEDX_CORE_INFO("Cleaning up {}", GetName());
        // Cleanup is automatic with smart pointers
    }
    
    void OnUpdate() override
    {
        if (!m_IsEnabled) return;
        SEDX_PROFILE_SCOPE("BufferModule::OnUpdate");
        // Update dynamic buffers
    }
    
private:
    bool m_IsEnabled = true;
};
```

### 2. Smart Pointer Usage

Always use the custom smart pointer system for buffer resources:

```cpp
// ✅ Correct: Use CreateRef<T>() for buffer resources
Buffer vertexBuffer = CreateBuffer(
    dataSize,
    BufferUsage::Vertex,
    MemoryType::GPU,
    "Vertex Buffer"
);

// ✅ Store buffers as members using proper cleanup
class RenderModule : public Module
{
private:
    Buffer m_VertexBuffer;    // Automatic cleanup through RAII
    Buffer m_IndexBuffer;
    Buffer m_UniformBuffer;
};

// ✅ Manual cleanup if needed
buffer.resource.Reset();
```

### 3. Buffer Creation Patterns

Use the `CreateBuffer()` function with appropriate parameters:

```cpp
// Vertex buffers (device-local for performance)
Buffer vertexBuffer = CreateBuffer(
    vertexDataSize,
    BufferUsage::Vertex,        // Usage flags
    MemoryType::GPU,           // Device-local memory
    "Mesh Vertex Buffer"       // Debug name
);

// Uniform buffers (CPU-accessible for updates)
Buffer uniformBuffer = CreateBuffer(
    sizeof(UniformData),
    BufferUsage::Uniform,
    MemoryType::CPU,           // Host-visible for updates
    "Scene Uniforms"
);

// Storage buffers (automatic bindless setup)
Buffer storageBuffer = CreateBuffer(
    dataSize,
    BufferUsage::Storage,
    MemoryType::GPU,
    "Compute Storage"
);
// Note: Storage buffers automatically get Address flag and bindless registration
```

### 4. Memory Management Patterns

#### Staging Buffer Pattern for Large Data

```cpp
void UploadLargeDataSet(const std::vector<Vertex>& vertices)
{
    const size_t dataSize = vertices.size() * sizeof(Vertex);
    
    // Create staging buffer
    Buffer stagingBuffer = CreateBuffer(
        dataSize,
        BufferUsage::TransferSrc,
        MemoryType::CPU,
        "Vertex Upload Staging"
    );
    
    // Upload data to staging buffer
    void* mapped = MapBuffer(stagingBuffer);
    memcpy(mapped, vertices.data(), dataSize);
    UnmapBuffer(stagingBuffer);
    
    // Create device-local buffer
    Buffer deviceBuffer = CreateBuffer(
        dataSize,
        BufferUsage::Vertex,
        MemoryType::GPU,
        "Vertex Buffer"
    );
    
    // Transfer data
    CopyBuffer(stagingBuffer.resource->buffer, 
              deviceBuffer.resource->buffer, 
              dataSize);
    
    // Staging buffer automatically cleaned up when out of scope
}
```

#### Dynamic Data Updates

```cpp
void UpdatePerFrameData()
{
    // For CPU-accessible buffers, use direct mapping
    if (m_UniformBuffer.memory & MemoryType::CPU)
    {
        void* mapped = MapBuffer(m_UniformBuffer);
        memcpy(mapped, &m_UniformData, sizeof(m_UniformData));
        UnmapBuffer(m_UniformBuffer);
    }
}
```

### 5. Error Handling and Validation

Always include proper error handling and validation:

```cpp
void CreateBuffers()
{
    try
    {
        // Validate parameters
        SEDX_CORE_ASSERT(dataSize > 0, "Invalid buffer size");
        
        m_VertexBuffer = CreateBuffer(
            dataSize,
            BufferUsage::Vertex,
            MemoryType::GPU,
            GetName() + " Vertex Buffer"
        );
        
        // Verify creation success
        SEDX_CORE_VERIFY(m_VertexBuffer.resource, "Failed to create vertex buffer");
        SEDX_CORE_VERIFY(m_VertexBuffer.resource->buffer != VK_NULL_HANDLE, 
                        "Invalid Vulkan buffer handle");
    }
    catch (const std::exception& e)
    {
        SEDX_CORE_ERROR_TAG("BUFFER", "Buffer creation failed: {}", e.what());
        throw;
    }
}

// Always validate before mapping
void* mapped = MapBuffer(buffer);
if (!mapped)
{
    SEDX_CORE_ERROR_TAG("BUFFER", "Failed to map buffer");
    return;
}
```

### 6. Logging and Profiling

Use tagged logging and profiling scopes:

```cpp
void OnAttach() override
{
    SEDX_CORE_INFO("=== Initializing {} ===", GetName());
    
    try
    {
        CreateBuffers();
        SEDX_CORE_INFO_TAG("BUFFER", "Buffers created successfully");
    }
    catch (const std::exception& e)
    {
        SEDX_CORE_ERROR_TAG("BUFFER", "Failed to create buffers: {}", e.what());
        throw;
    }
}

void OnUpdate() override
{
    if (!m_IsEnabled) return;
    SEDX_PROFILE_SCOPE("BufferModule::OnUpdate");
    
    // Update logic with profiling
    UpdateDynamicBuffers();
}
```

## Buffer Usage Flag Guidelines

### Automatic Flag Application

The system automatically applies required flags based on usage:

```cpp
// ✅ Vertex buffers automatically get TransferDst flag
Buffer vertexBuffer = CreateBuffer(size, BufferUsage::Vertex, MemoryType::GPU);
// Actual flags: Vertex | TransferDst

// ✅ Index buffers automatically get TransferDst flag
Buffer indexBuffer = CreateBuffer(size, BufferUsage::Index, MemoryType::GPU);
// Actual flags: Index | TransferDst

// ✅ Storage buffers automatically get Address flag and alignment
Buffer storageBuffer = CreateBuffer(size, BufferUsage::Storage, MemoryType::GPU);
// Actual flags: Storage | Address
// Size is automatically aligned to minStorageBufferOffsetAlignment
```

### Common Usage Combinations

```cpp
// Vertex buffer with staging upload
BufferUsage::Vertex  // Automatically adds TransferDst

// Index buffer with staging upload
BufferUsage::Index   // Automatically adds TransferDst

// Uniform buffer (CPU updates)
BufferUsage::Uniform

// Storage buffer (compute shader access)
BufferUsage::Storage // Automatically adds Address and bindless setup

// Indirect drawing buffer
BufferUsage::Indirect | BufferUsage::Storage // Allow compute shader writes

// Ray tracing acceleration structure input
BufferUsage::AccelerationStructureInput // Automatically adds Address | TransferDst

// Staging buffer for uploads
BufferUsage::TransferSrc
```

## Memory Type Selection

### Choose Memory Type Based on Access Pattern

```cpp
// ✅ Device-local (GPU) for frequently accessed, rarely updated data
MemoryType::GPU
// Use for: vertex buffers, index buffers, read-only textures, storage buffers

// ✅ Host-visible (CPU) for frequently updated data
MemoryType::CPU
// Use for: per-frame uniforms, dynamic vertex buffers, staging buffers
```

### Performance Considerations

```cpp
// ✅ High-performance static geometry
Buffer staticVertexBuffer = CreateBuffer(
    vertexDataSize,
    BufferUsage::Vertex,
    MemoryType::GPU,  // Device-local for best GPU performance
    "Static Mesh Vertices"
);

// ✅ Dynamic per-frame data
Buffer dynamicUniformBuffer = CreateBuffer(
    sizeof(FrameData),
    BufferUsage::Uniform,
    MemoryType::CPU,  // Host-visible for frequent CPU updates
    "Per-Frame Uniforms"
);
```

## Common Buffer Patterns

### 1. Static Mesh Buffer Creation

```cpp
class StaticMeshModule : public Module
{
public:
    void OnAttach() override
    {
        CreateMeshBuffers();
    }
    
private:
    void CreateMeshBuffers()
    {
        // Upload vertex data
        m_VertexBuffer = BufferFactory::CreateVertexBuffer(
            m_VertexData.data(),
            m_VertexData.size() * sizeof(Vertex),
            GetName() + " Vertices"
        );
        
        // Upload index data
        m_IndexBuffer = BufferFactory::CreateIndexBuffer(
            m_IndexData.data(),
            m_IndexData.size() * sizeof(uint32_t),
            GetName() + " Indices"
        );
    }
    
    Buffer m_VertexBuffer;
    Buffer m_IndexBuffer;
    std::vector<Vertex> m_VertexData;
    std::vector<uint32_t> m_IndexData;
};
```

### 2. Dynamic Uniform Buffer Management

```cpp
class UniformModule : public Module
{
public:
    void OnAttach() override
    {
        m_UniformBuffer = CreateBuffer(
            sizeof(SceneUniforms),
            BufferUsage::Uniform,
            MemoryType::CPU,
            "Scene Uniform Buffer"
        );
    }
    
    void OnUpdate() override
    {
        if (!m_IsEnabled) return;
        SEDX_PROFILE_SCOPE("UniformModule::OnUpdate");
        
        UpdateUniforms();
    }
    
private:
    void UpdateUniforms()
    {
        SceneUniforms uniforms = {
            .viewMatrix = m_Camera->GetViewMatrix(),
            .projMatrix = m_Camera->GetProjectionMatrix(),
            .lightPos = m_LightPosition,
            .time = GetTime()
        };
        
        void* mapped = MapBuffer(m_UniformBuffer);
        memcpy(mapped, &uniforms, sizeof(SceneUniforms));
        UnmapBuffer(m_UniformBuffer);
    }
    
    Buffer m_UniformBuffer;
    Ref<Camera> m_Camera;
    glm::vec3 m_LightPosition{0.0f, 10.0f, 0.0f};
};
```

### 3. Compute Storage Buffer Setup

```cpp
class ComputeModule : public Module
{
public:
    void OnAttach() override
    {
        CreateComputeBuffers();
    }
    
private:
    void CreateComputeBuffers()
    {
        // Input storage buffer (automatically gets bindless registration)
        m_InputBuffer = CreateBuffer(
            m_ParticleCount * sizeof(Particle),
            BufferUsage::Storage,
            MemoryType::GPU,
            "Particle Input Buffer"
        );
        
        // Output storage buffer
        m_OutputBuffer = CreateBuffer(
            m_ParticleCount * sizeof(Particle),
            BufferUsage::Storage,
            MemoryType::GPU,
            "Particle Output Buffer"
        );
        
        SEDX_CORE_INFO_TAG("COMPUTE", "Storage buffers created with IDs: {}, {}", 
                          m_InputBuffer.ID(), m_OutputBuffer.ID());
    }
    
    Buffer m_InputBuffer;
    Buffer m_OutputBuffer;
    static constexpr size_t m_ParticleCount = 10000;
};
```

### 4. Debug UI Integration

```cpp
void OnUIRender() override
{
    if (ImGui::Begin("Buffer Debug"))
    {
        ImGui::Text("Vertex Buffer ID: %u", m_VertexBuffer.ID());
        ImGui::Text("Index Buffer ID: %u", m_IndexBuffer.ID());
        ImGui::Text("Uniform Buffer ID: %u", m_UniformBuffer.ID());
        
        ImGui::Text("Vertex Buffer Size: %llu bytes", m_VertexBuffer.GetSize());
        ImGui::Text("Index Buffer Size: %llu bytes", m_IndexBuffer.GetSize());
        ImGui::Text("Uniform Buffer Size: %llu bytes", m_UniformBuffer.GetSize());
        
        ImGui::Checkbox("Enable Module", &m_IsEnabled);
        
        if (ImGui::Button("Recreate Buffers"))
        {
            RecreateBuffers();
        }
    }
    ImGui::End();
}
```

## Specialized Buffer Types

### 1. Staging Buffer Pool Usage

```cpp
// Use staging buffer pool for efficient uploads
Buffer stagingBuffer = m_BufferPoolManager->AcquireStagingBuffer(dataSize);

// Upload data
void* mapped = MapBuffer(stagingBuffer);
memcpy(mapped, sourceData, dataSize);
UnmapBuffer(stagingBuffer);

// Transfer to device buffer
CopyBuffer(stagingBuffer.resource->buffer, deviceBuffer.resource->buffer, dataSize);

// Return to pool
m_BufferPoolManager->ReleaseStagingBuffer(stagingBuffer);
```

### 2. Bindless Storage Buffer Access

```cpp
// Storage buffers automatically register for bindless access
Buffer storageBuffer = CreateBuffer(
    dataSize,
    BufferUsage::Storage,
    MemoryType::GPU,
    "Bindless Storage"
);

// Resource ID automatically assigned and descriptor updated
uint32_t resourceID = storageBuffer.ID();

// In shader: use buffer with resource ID
// layout(set = 0, binding = 1) buffer StorageBuffers {
//     BufferData data[];
// } storageBuffers[];
// 
// BufferData myData = storageBuffers[resourceID].data[index];
```

## Anti-Patterns to Avoid

### ❌ Incorrect Patterns

```cpp
// ❌ Don't use raw new/delete
BufferResource* resource = new BufferResource(); // Wrong!

// ❌ Don't forget RAII principles
VkBuffer rawBuffer; // Wrong - no automatic cleanup

// ❌ Don't ignore memory type for mapping
Buffer gpuBuffer = CreateBuffer(size, usage, MemoryType::GPU);
void* mapped = MapBuffer(gpuBuffer); // Will assert!

// ❌ Don't skip validation
Buffer buffer = CreateBuffer(0, usage, memory); // Invalid size!

// ❌ Don't map without checking memory type
if (buffer.memory & MemoryType::CPU)
{
    void* mapped = MapBuffer(buffer); // Check first
}
```

### ✅ Correct Patterns

```cpp
// ✅ Use smart pointers and RAII
Buffer buffer = CreateBuffer(size, usage, memory, name);

// ✅ Proper memory type selection
Buffer cpuBuffer = CreateBuffer(size, usage, MemoryType::CPU, name);
void* mapped = MapBuffer(cpuBuffer); // OK

// ✅ Always validate parameters
SEDX_CORE_ASSERT(size > 0, "Invalid buffer size");

// ✅ Check memory type before mapping
if (buffer.memory & MemoryType::CPU)
{
    void* mapped = MapBuffer(buffer);
    // ... use mapped data
    UnmapBuffer(buffer);
}
```

## Configuration and Module Integration

### JSON Configuration Support

```cpp
struct BufferConfig
{
    size_t defaultVertexBufferSize = 64 * 1024 * 1024;  // 64MB
    size_t defaultIndexBufferSize = 16 * 1024 * 1024;   // 16MB
    size_t defaultUniformBufferSize = 1024;             // 1KB
    bool enableDebugNames = true;
    
    void ToJSON(nlohmann::json& j) const
    {
        j["defaultVertexBufferSize"] = defaultVertexBufferSize;
        j["defaultIndexBufferSize"] = defaultIndexBufferSize;
        j["defaultUniformBufferSize"] = defaultUniformBufferSize;
        j["enableDebugNames"] = enableDebugNames;
    }
    
    void FromJSON(const nlohmann::json& j)
    {
        defaultVertexBufferSize = j.value("defaultVertexBufferSize", 64 * 1024 * 1024);
        defaultIndexBufferSize = j.value("defaultIndexBufferSize", 16 * 1024 * 1024);
        defaultUniformBufferSize = j.value("defaultUniformBufferSize", 1024);
        enableDebugNames = j.value("enableDebugNames", true);
    }
};
```

## Key Reminders for Copilot

1. **Always inherit from Module** for buffer management components
2. **Use CreateBuffer()** instead of manual Vulkan buffer creation
3. **Apply proper memory types** based on access patterns (GPU/CPU)
4. **Include error handling** with assertions and try-catch blocks
5. **Use tagged logging** for better categorization
6. **Add profiling scopes** for performance monitoring
7. **Validate buffer parameters** before creation
8. **Check memory type** before mapping operations
9. **Use staging buffers** for large data uploads
10. **Follow RAII principles** for automatic resource cleanup
11. **Include debug UI** for runtime inspection when appropriate
12. **Handle configuration** through JSON when needed

## File Naming and Organization

When creating buffer-related files, follow these patterns:

```cpp
// Module files
class TerrainBufferModule : public Module {};      // terrain_buffer_module.h/.cpp
class UIBufferManager : public Module {};          // ui_buffer_manager.h/.cpp
class ComputeBufferSystem : public Module {};      // compute_buffer_system.h/.cpp

// Utility files
namespace BufferUtils {};                          // buffer_utils.h/.cpp
class BufferFactory {};                            // buffer_factory.h/.cpp
class BufferPool {};                               // buffer_pool.h/.cpp
```

This instruction set ensures consistent, efficient, and maintainable buffer management code that integrates properly with the Scenery Editor X architecture and follows established patterns throughout the codebase.

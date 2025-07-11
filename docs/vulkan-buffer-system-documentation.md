# Scenery Editor X - Vulkan Buffer System Documentation

---

## Overview

The Vulkan Buffer System in Scenery Editor X provides a comprehensive abstraction layer over Vulkan's buffer management functionality. It handles creation, allocation, memory mapping, and destruction of GPU buffers using the Vulkan Memory Allocator (VMA) library for efficient memory management.

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Core Components](#core-components)
3. [Buffer Types and Usage](#buffer-types-and-usage)
4. [Memory Management](#memory-management)
5. [API Reference](#api-reference)
6. [Usage Examples](#usage-examples)
7. [Best Practices](#best-practices)
8. [Performance Considerations](#performance-considerations)
9. [Error Handling](#error-handling)
10. [Integration with Module System](#integration-with-module-system)

---

## Architecture Overview

The Vulkan Buffer System follows the engine's module-based architecture and uses modern C++ features along with the custom smart pointer system. The system is designed with the following principles:

- **RAII Management**: Automatic resource cleanup through smart pointers and destructors
- **Memory Safety**: Proper allocation tracking and validation
- **Performance**: Efficient memory allocation strategies with VMA
- **Modularity**: Clean separation of concerns between allocation, mapping, and usage
- **Debug Support**: Comprehensive logging and validation in debug builds

### System Components

```
┌─────────────────────────────────────────────────────────────┐
│                    Vulkan Buffer System                     │
├─────────────────────────────────────────────────────────────┤
│  Buffer Management Layer                                    │
│  ┌─────────────────┐  ┌──────────────────┐               │
│  │     Buffer      │  │  BufferResource  │               │
│  │   (High Level)  │  │  (Low Level)     │               │
│  └─────────────────┘  └──────────────────┘               │
├─────────────────────────────────────────────────────────────┤
│  Memory Allocation Layer                                    │
│  ┌─────────────────┐  ┌──────────────────┐               │
│  │ MemoryAllocator │  │ VulkanMemoryUtils│               │
│  │   (Engine)      │  │   (Utilities)    │               │
│  └─────────────────┘  └──────────────────┘               │
├─────────────────────────────────────────────────────────────┤
│  Vulkan Memory Allocator (VMA)                             │
├─────────────────────────────────────────────────────────────┤
│  Vulkan API                                                 │
└─────────────────────────────────────────────────────────────┘
```



---

## Core Components

### 1. Buffer Structure

The `Buffer` struct is the primary interface for working with GPU buffers:

```cpp
struct Buffer
{
    void *data = nullptr;               ///< Pointer to mapped memory region (CPU access)
    Ref<BufferResource> resource;       ///< The Vulkan buffer resource
    uint64_t size = 0;                  ///< Size of the buffer in bytes
    BufferUsageFlags usage;             ///< Usage flags for the buffer
    MemoryFlags memory;                 ///< Memory type flags (GPU/CPU)
    uint32_t ID() const;                ///< Unique identifier for tracking
};
```

**Key Features:**

- **Smart Resource Management**: Uses `Ref<BufferResource>` for automatic cleanup
- **Memory Mapping Support**: Direct CPU access through `data` pointer when applicable
- **Usage Tracking**: Comprehensive flags for buffer usage patterns
- **Unique Identification**: Resource ID for debugging and bindless descriptors

### 2. BufferResource Structure

The `BufferResource` struct encapsulates the low-level Vulkan buffer:

```cpp
struct BufferResource : Resource
{
    VkBuffer buffer = VK_NULL_HANDLE;           ///< Vulkan buffer handle
    VmaAllocation allocation;                    ///< VMA memory allocation
    VkDeviceMemory memory = VK_NULL_HANDLE;     ///< Native Vulkan device memory
  
    virtual ~BufferResource() override;         ///< Automatic cleanup
    virtual ResourceDescriptorInfo GetDescriptorInfo() const override;
};
```

**Key Features:**

- **RAII Cleanup**: Virtual destructor ensures proper resource deallocation
- **VMA Integration**: Direct integration with Vulkan Memory Allocator
- **Descriptor Support**: Provides descriptor information for pipeline binding

### 3. Buffer Usage Flags

The system provides comprehensive buffer usage flags for different use cases:

```cpp
namespace BufferUsage
{
    enum BufferUsageFlags
    {
        TransferSrc                 = 0x00000001,  ///< Source for transfer operations
        TransferDst                 = 0x00000002,  ///< Destination for transfer operations
        UniformTexel                = 0x00000004,  ///< Uniform texel buffer
        StorageTexel                = 0x00000008,  ///< Storage texel buffer
        Uniform                     = 0x00000010,  ///< Uniform buffer
        Storage                     = 0x00000020,  ///< Storage buffer (SSBO)
        Index                       = 0x00000040,  ///< Index buffer
        Vertex                      = 0x00000080,  ///< Vertex buffer
        Indirect                    = 0x00000100,  ///< Indirect drawing buffer
        Address                     = 0x00020000,  ///< Buffer device address
        AccelerationStructureInput  = 0x00080000,  ///< Ray tracing input
        AccelerationStructure       = 0x00100000,  ///< Ray tracing structure
        ShaderBindingTable          = 0x00000400,  ///< Ray tracing SBT
        // Additional flags for specialized use cases...
    };
}
```

### 4. Memory Types

Memory type flags determine where the buffer is allocated:

```cpp
enum MemoryType : uint8_t
{
    GPU = 0x00000001,                           ///< Device-local (GPU only)
    CPU = 0x00000002 | 0x00000004,             ///< Host-visible (CPU accessible)
};
```



---

## Buffer Types and Usage

### Vertex Buffers

Vertex buffers store geometry data for rendering:

```cpp
// Create a vertex buffer
Buffer vertexBuffer = CreateBuffer(
    vertexData.size(),                          // Size in bytes
    BufferUsage::Vertex,                        // Vertex buffer usage
    MemoryType::GPU,                           // Device-local memory
    "Mesh Vertex Buffer"                       // Debug name
);

// Usage flags automatically applied:
// - BufferUsage::Vertex
// - BufferUsage::TransferDst (automatically added)
```

### Index Buffers

Index buffers store indices for indexed rendering:

```cpp
// Create an index buffer
Buffer indexBuffer = CreateBuffer(
    indexData.size(),
    BufferUsage::Index,
    MemoryType::GPU,
    "Mesh Index Buffer"
);

// Usage flags automatically applied:
// - BufferUsage::Index  
// - BufferUsage::TransferDst (automatically added)
```

### Uniform Buffers

Uniform buffers provide constant data to shaders:

```cpp
// Create a uniform buffer
Buffer uniformBuffer = CreateBuffer(
    sizeof(UniformData),
    BufferUsage::Uniform,
    MemoryType::CPU,                           // CPU accessible for updates
    "Scene Uniform Buffer"
);

// Can be mapped for direct CPU access
void* mappedData = MapBuffer(uniformBuffer);
memcpy(mappedData, &uniformData, sizeof(UniformData));
UnmapBuffer(uniformBuffer);
```

### Storage Buffers (SSBO)

Storage buffers provide read/write access from shaders:

```cpp
// Create a storage buffer
Buffer storageBuffer = CreateBuffer(
    dataSize,
    BufferUsage::Storage,
    MemoryType::GPU,
    "Compute Storage Buffer"
);

// Usage flags automatically applied:
// - BufferUsage::Storage
// - BufferUsage::Address (automatically added)
// - Bindless descriptor setup (automatic)
```

### Staging Buffers

Staging buffers facilitate CPU-to-GPU data transfer:

```cpp
// Create a staging buffer using helper function
Buffer stagingBuffer = device->CreateStagingBuffer(
    dataSize,
    "Data Transfer Staging"
);

// Usage flags applied:
// - BufferUsage::TransferSrc
// - MemoryType::CPU (host-visible)
```



---

## Memory Management

### VMA Integration

The system uses Vulkan Memory Allocator (VMA) for efficient memory management:

```cpp
// VMA handles memory allocation automatically
VmaAllocationCreateInfo allocInfo = {};
allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

// CPU accessible buffers get additional mapping flags
if (memory & CPU)
    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | 
                     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
```

### Memory Alignment

The system handles memory alignment requirements automatically:

```cpp
// Storage buffers are aligned to minimum required alignment
if (usage & BufferUsage::Storage)
{
    size += size % device->GetPhysicalDevice()
                        ->GetDeviceProperties()
                        .limits.minStorageBufferOffsetAlignment;
}
```

### Memory Mapping

For CPU-accessible buffers, the system provides mapping utilities:

```cpp
// Map buffer for CPU access
void* MapBuffer(BufferResource &buffer);

// Unmap buffer when finished
void UnmapBuffer(BufferResource &buffer);

// Example usage in a module
class DataUploadModule : public Module
{
    void UploadData(const Buffer& buffer, const void* data, size_t size)
    {
        SEDX_CORE_ASSERT(buffer.memory & MemoryType::CPU, "Buffer not CPU accessible");
      
        void* mappedData = MapBuffer(buffer);
        memcpy(mappedData, data, size);
        UnmapBuffer(buffer);
      
        SEDX_CORE_INFO_TAG("UPLOAD", "Uploaded {} bytes to buffer {}", size, buffer.ID());
    }
};
```



---

## API Reference

### Core Functions

#### CreateBuffer

```cpp
Buffer CreateBuffer(
    uint64_t size,                              // Size in bytes
    BufferUsageFlags usage,                     // Usage flags
    MemoryFlags memory = MemoryType::GPU,       // Memory type (default: GPU)
    const std::string& name = ""               // Debug name (optional)
);
```

**Purpose**: Creates a new Vulkan buffer with specified parameters.

**Parameters**:

- `size`: Buffer size in bytes
- `usage`: Buffer usage flags (combinations allowed)
- `memory`: Memory type flags (GPU/CPU)
- `name`: Optional debug name for identification

**Returns**: Buffer object with allocated resources

**Usage Flag Modifications**: The function automatically adds required flags:

- Vertex/Index buffers get `TransferDst` flag
- Storage buffers get `Address` flag and alignment
- Acceleration structure buffers get appropriate flags

#### CopyBuffer

```cpp
void CopyBuffer(
    VkBuffer srcBuffer,                         // Source buffer
    VkBuffer dstBuffer,                         // Destination buffer  
    VkDeviceSize size                          // Number of bytes to copy
);
```

**Purpose**: Copies data between two Vulkan buffers using a command buffer.

**Parameters**:

- `srcBuffer`: Source buffer (usually staging buffer)
- `dstBuffer`: Destination buffer (usually device-local)
- `size`: Number of bytes to copy

**Implementation**: Creates a single-use command buffer, executes copy, and waits for completion.

#### CopyBufferToImage

```cpp
void CopyBufferToImage(
    VkBuffer buffer,                           // Source buffer
    VkImage image,                             // Destination image
    uint32_t width,                            // Image width
    uint32_t height                            // Image height
);
```

**Purpose**: Copies buffer data to an image (texture upload).

### Memory Mapping Functions

#### MapBuffer

```cpp
void* MapBuffer(BufferResource &buffer);
```

**Purpose**: Maps buffer memory for CPU access.

**Returns**: Pointer to mapped memory region

**Requirements**: Buffer must be created with `MemoryType::CPU`

#### UnmapBuffer

```cpp
void UnmapBuffer(BufferResource &buffer);
```

**Purpose**: Unmaps previously mapped buffer memory.

**Requirements**: Buffer must have been mapped with `MapBuffer`

### Buffer Class Methods

The Buffer struct provides several utility methods:

```cpp
class Buffer
{
public:
    // Memory operations
    void Allocate(uint64_t size);               // Allocate CPU memory
    void Release();                             // Release CPU memory
    void ZeroInitialize() const;                // Zero-initialize memory
  
    // Data access
    template<typename T>
    T& Read(uint64_t offset = 0);              // Read typed data
  
    template<typename T>
    const T& Read(uint64_t offset = 0) const;  // Read typed data (const)
  
    byte* ReadBytes(uint64_t size, uint64_t offset) const; // Read raw bytes
  
    void Write(const void* data, uint64_t size, uint64_t offset = 0) const; // Write data
  
    // Utility
    uint64_t GetSize() const;                   // Get buffer size
    uint32_t ID() const;                        // Get unique resource ID
  
    // Static factory methods
    static Buffer Copy(const Buffer& other);    // Copy constructor
    static Buffer Copy(const void* data, uint64_t size); // Copy from data
  
    // Operators
    explicit operator bool() const;             // Check if valid
    byte& operator[](int index);                // Array access
    byte operator[](int index) const;           // Array access (const)
  
    template<typename T>
    T* As() const;                             // Cast data pointer
};
```



---

## Usage Examples

### Example 1: Basic Vertex Buffer Creation

```cpp
class MeshModule : public Module
{
public:
    explicit MeshModule(const std::string& name = "MeshModule")
        : Module(name)
    {
    }
  
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("MESH", "=== Initializing {} ===", GetName());
      
        // Define vertex data
        std::vector<Vertex> vertices = {
            {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
            {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
            {{ 0.0f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}
        };
      
        // Create staging buffer
        const size_t bufferSize = vertices.size() * sizeof(Vertex);
        Buffer stagingBuffer = CreateBuffer(
            bufferSize,
            BufferUsage::TransferSrc,
            MemoryType::CPU,
            "Triangle Staging Buffer"
        );
      
        // Map and copy data
        void* mappedData = MapBuffer(stagingBuffer);
        memcpy(mappedData, vertices.data(), bufferSize);
        UnmapBuffer(stagingBuffer);
      
        // Create device-local vertex buffer
        m_VertexBuffer = CreateBuffer(
            bufferSize,
            BufferUsage::Vertex,
            MemoryType::GPU,
            "Triangle Vertex Buffer"
        );
      
        // Copy from staging to device buffer
        CopyBuffer(stagingBuffer.resource->buffer, 
                  m_VertexBuffer.resource->buffer, 
                  bufferSize);
      
        SEDX_CORE_INFO_TAG("MESH", "Created vertex buffer with {} vertices", vertices.size());
    }
  
    void OnDetach() override
    {
        SEDX_CORE_INFO_TAG("MESH", "Cleaning up {}", GetName());
        // Resources cleaned up automatically by smart pointers
        m_VertexBuffer.resource.Reset();
    }
  
private:
    Buffer m_VertexBuffer;
};
```

### Example 2: Dynamic Uniform Buffer Management

```cpp
class UniformBufferModule : public Module
{
public:
    struct SceneUniforms
    {
        glm::mat4 viewMatrix;
        glm::mat4 projMatrix;
        glm::vec4 lightPosition;
        float time;
    };
  
    void OnAttach() override
    {
        SEDX_PROFILE_SCOPE("UniformBufferModule::OnAttach");
      
        // Create uniform buffer
        m_UniformBuffer = CreateBuffer(
            sizeof(SceneUniforms),
            BufferUsage::Uniform,
            MemoryType::CPU,  // CPU accessible for frequent updates
            "Scene Uniform Buffer"
        );
      
        SEDX_CORE_INFO_TAG("UNIFORM", "Created uniform buffer (size: {} bytes)", 
                          sizeof(SceneUniforms));
    }
  
    void OnUpdate() override
    {
        if (!m_IsEnabled) return;
        SEDX_PROFILE_SCOPE("UniformBufferModule::OnUpdate");
      
        // Update uniform data
        SceneUniforms uniforms;
        uniforms.viewMatrix = m_Camera->GetViewMatrix();
        uniforms.projMatrix = m_Camera->GetProjectionMatrix();
        uniforms.lightPosition = glm::vec4(m_LightPosition, 1.0f);
        uniforms.time = GetTime();
      
        // Map and update buffer
        void* mappedData = MapBuffer(m_UniformBuffer);
        memcpy(mappedData, &uniforms, sizeof(SceneUniforms));
        UnmapBuffer(m_UniformBuffer);
    }
  
private:
    Buffer m_UniformBuffer;
    Ref<Camera> m_Camera;
    glm::vec3 m_LightPosition{0.0f, 10.0f, 0.0f};
    bool m_IsEnabled = true;
};
```

### Example 3: Storage Buffer for Compute

```cpp
class ComputeModule : public Module
{
public:
    void OnAttach() override
    {
        SEDX_PROFILE_SCOPE("ComputeModule::OnAttach");
      
        // Create input storage buffer
        m_InputBuffer = CreateBuffer(
            m_ParticleCount * sizeof(Particle),
            BufferUsage::Storage,
            MemoryType::GPU,
            "Particle Input Buffer"
        );
      
        // Create output storage buffer
        m_OutputBuffer = CreateBuffer(
            m_ParticleCount * sizeof(Particle),
            BufferUsage::Storage,
            MemoryType::GPU,
            "Particle Output Buffer"
        );
      
        // Initialize particle data
        InitializeParticles();
      
        SEDX_CORE_INFO_TAG("COMPUTE", "Created storage buffers for {} particles", 
                          m_ParticleCount);
        SEDX_CORE_INFO_TAG("COMPUTE", "Input buffer ID: {}, Output buffer ID: {}", 
                          m_InputBuffer.ID(), m_OutputBuffer.ID());
    }
  
private:
    void InitializeParticles()
    {
        // Create staging buffer
        const size_t bufferSize = m_ParticleCount * sizeof(Particle);
        Buffer stagingBuffer = CreateBuffer(
            bufferSize,
            BufferUsage::TransferSrc,
            MemoryType::CPU,
            "Particle Init Staging"
        );
      
        // Generate initial particle data
        std::vector<Particle> particles(m_ParticleCount);
        for (size_t i = 0; i < m_ParticleCount; ++i)
        {
            particles[i].position = GetRandomPosition();
            particles[i].velocity = GetRandomVelocity();
            particles[i].lifetime = GetRandomLifetime();
        }
      
        // Upload to staging buffer
        void* mappedData = MapBuffer(stagingBuffer);
        memcpy(mappedData, particles.data(), bufferSize);
        UnmapBuffer(stagingBuffer);
      
        // Copy to device buffer
        CopyBuffer(stagingBuffer.resource->buffer, 
                  m_InputBuffer.resource->buffer, 
                  bufferSize);
    }
  
    Buffer m_InputBuffer;
    Buffer m_OutputBuffer;
    static constexpr size_t m_ParticleCount = 10000;
};
```

### Example 4: Bindless Resource Management

```cpp
class BindlessBufferModule : public Module
{
public:
    void OnAttach() override
    {
        // Create multiple storage buffers for bindless access
        for (int i = 0; i < m_BufferCount; ++i)
        {
            Buffer storageBuffer = CreateBuffer(
                m_BufferSize,
                BufferUsage::Storage,
                MemoryType::GPU,
                fmt::format("Storage Buffer {}", i)
            );
          
            // Buffer automatically gets registered for bindless access
            // Resource ID is assigned from available pool
            m_StorageBuffers.push_back(storageBuffer);
          
            SEDX_CORE_INFO_TAG("BINDLESS", "Created storage buffer {} with ID {}", 
                              i, storageBuffer.ID());
        }
    }
  
    uint32_t GetBufferResourceID(int index) const
    {
        SEDX_CORE_ASSERT(index < m_StorageBuffers.size(), "Buffer index out of range");
        return m_StorageBuffers[index].ID();
    }
  
private:
    std::vector<Buffer> m_StorageBuffers;
    static constexpr int m_BufferCount = 16;
    static constexpr size_t m_BufferSize = 1024 * 1024; // 1MB each
};
```



---

## Best Practices

### 1. Memory Management Guidelines

```cpp
class BufferManagerModule : public Module
{
public:
    void OnAttach() override
    {
        // ✅ Good: Use device-local memory for frequently accessed GPU data
        m_StaticVertexBuffer = CreateBuffer(
            vertexDataSize,
            BufferUsage::Vertex,
            MemoryType::GPU,  // Device-local for best performance
            "Static Mesh Vertices"
        );
      
        // ✅ Good: Use CPU-accessible memory for frequently updated data
        m_DynamicUniformBuffer = CreateBuffer(
            sizeof(FrameUniforms),
            BufferUsage::Uniform,
            MemoryType::CPU,  // Host-visible for updates
            "Dynamic Frame Uniforms"
        );
      
        // ✅ Good: Use staging buffers for large one-time uploads
        UploadLargeDataSet();
    }
  
private:
    void UploadLargeDataSet()
    {
        // Create staging buffer for upload
        Buffer stagingBuffer = CreateBuffer(
            largeDataSize,
            BufferUsage::TransferSrc,
            MemoryType::CPU,
            "Large Dataset Staging"
        );
      
        // Upload data
        void* mappedData = MapBuffer(stagingBuffer);
        memcpy(mappedData, largeData.data(), largeDataSize);
        UnmapBuffer(stagingBuffer);
      
        // Create device buffer
        Buffer deviceBuffer = CreateBuffer(
            largeDataSize,
            BufferUsage::Storage,
            MemoryType::GPU,
            "Large Dataset Device"
        );
      
        // Transfer data
        CopyBuffer(stagingBuffer.resource->buffer, 
                  deviceBuffer.resource->buffer, 
                  largeDataSize);
      
        // Staging buffer automatically cleaned up when out of scope
    }
};
```

### 2. Error Handling Patterns

```cpp
class SafeBufferModule : public Module
{
public:
    void OnAttach() override
    {
        try
        {
            CreateBuffers();
            SEDX_CORE_INFO_TAG("BUFFER", "All buffers created successfully");
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("BUFFER", "Failed to create buffers: {}", e.what());
            m_InitializationFailed = true;
        }
    }
  
    void OnUpdate() override
    {
        if (m_InitializationFailed) return;
      
        // Always validate before use
        SEDX_CORE_ASSERT(m_VertexBuffer.resource, "Vertex buffer not initialized");
        SEDX_CORE_ASSERT(m_VertexBuffer.memory & MemoryType::GPU, "Invalid memory type");
      
        // Safe memory mapping
        if (m_UniformBuffer.memory & MemoryType::CPU)
        {
            UpdateUniforms();
        }
    }
  
private:
    void CreateBuffers()
    {
        // Validate parameters before creation
        SEDX_CORE_ASSERT(m_VertexDataSize > 0, "Invalid vertex data size");
      
        m_VertexBuffer = CreateBuffer(
            m_VertexDataSize,
            BufferUsage::Vertex,
            MemoryType::GPU,
            GetName() + " Vertex Buffer"
        );
      
        // Verify creation success
        SEDX_CORE_VERIFY(m_VertexBuffer.resource, "Failed to create vertex buffer");
        SEDX_CORE_VERIFY(m_VertexBuffer.resource->buffer != VK_NULL_HANDLE, 
                        "Invalid Vulkan buffer handle");
    }
  
    void UpdateUniforms()
    {
        try
        {
            void* mappedData = MapBuffer(m_UniformBuffer);
          
            // Always check mapped pointer
            if (!mappedData)
            {
                SEDX_CORE_ERROR_TAG("BUFFER", "Failed to map uniform buffer");
                return;
            }
          
            // Update data
            memcpy(mappedData, &m_UniformData, sizeof(m_UniformData));
          
            // Always unmap
            UnmapBuffer(m_UniformBuffer);
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("BUFFER", "Error updating uniforms: {}", e.what());
        }
    }
  
    Buffer m_VertexBuffer;
    Buffer m_UniformBuffer;
    UniformData m_UniformData;
    size_t m_VertexDataSize = 0;
    bool m_InitializationFailed = false;
};
```

### 3. Performance Optimization

```cpp
class OptimizedBufferModule : public Module
{
public:
    void OnAttach() override
    {
        // ✅ Batch buffer creation to reduce overhead
        CreateAllBuffers();
      
        // ✅ Pre-allocate frequently used staging buffers
        m_StagingPool.reserve(m_MaxStagingBuffers);
        for (int i = 0; i < m_MaxStagingBuffers; ++i)
        {
            m_StagingPool.push_back(CreateBuffer(
                m_StagingBufferSize,
                BufferUsage::TransferSrc,
                MemoryType::CPU,
                fmt::format("Staging Pool {}", i)
            ));
        }
    }
  
    void OnUpdate() override
    {
        SEDX_PROFILE_SCOPE("OptimizedBufferModule::OnUpdate");
      
        // ✅ Minimize mapping operations
        if (m_UniformsNeedUpdate)
        {
            UpdateUniformsEfficiently();
            m_UniformsNeedUpdate = false;
        }
      
        // ✅ Batch data uploads
        if (!m_PendingUploads.empty())
        {
            ProcessPendingUploads();
        }
    }
  
private:
    void UpdateUniformsEfficiently()
    {
        // ✅ Keep buffers mapped for frequent updates
        if (!m_UniformsMapped)
        {
            m_MappedUniformData = MapBuffer(m_UniformBuffer);
            m_UniformsMapped = true;
        }
      
        // Direct update without remapping
        memcpy(m_MappedUniformData, &m_UniformData, sizeof(m_UniformData));
    }
  
    void ProcessPendingUploads()
    {
        SEDX_PROFILE_SCOPE("ProcessPendingUploads");
      
        // ✅ Use staging buffer pool to avoid allocations
        if (m_CurrentStagingIndex >= m_StagingPool.size())
            m_CurrentStagingIndex = 0;
          
        Buffer& stagingBuffer = m_StagingPool[m_CurrentStagingIndex++];
      
        // Batch multiple uploads in single staging buffer
        size_t offset = 0;
        void* mappedStaging = MapBuffer(stagingBuffer);
      
        for (const auto& upload : m_PendingUploads)
        {
            if (offset + upload.size > m_StagingBufferSize)
                break; // Staging buffer full
              
            memcpy(static_cast<byte*>(mappedStaging) + offset, 
                   upload.data, upload.size);
            offset += upload.size;
        }
      
        UnmapBuffer(stagingBuffer);
      
        // Batch copy operations
        // ... copy staging to device buffers ...
      
        m_PendingUploads.clear();
    }
  
    struct PendingUpload
    {
        const void* data;
        size_t size;
        Buffer targetBuffer;
    };
  
    std::vector<Buffer> m_StagingPool;
    std::vector<PendingUpload> m_PendingUploads;
    void* m_MappedUniformData = nullptr;
    bool m_UniformsMapped = false;
    bool m_UniformsNeedUpdate = false;
    size_t m_CurrentStagingIndex = 0;
  
    static constexpr int m_MaxStagingBuffers = 4;
    static constexpr size_t m_StagingBufferSize = 64 * 1024; // 64KB
};
```



---

## Performance Considerations

### Memory Allocation Strategy

1. **Device-Local Memory (GPU)**: Use for frequently accessed data that doesn't need CPU updates

   - Vertex buffers, index buffers, textures
   - Read-only uniform buffers
   - Storage buffers for compute shaders
2. **Host-Visible Memory (CPU)**: Use for data that needs frequent CPU updates

   - Dynamic uniform buffers (per-frame data)
   - Staging buffers for data upload
   - Buffers for CPU-GPU synchronization
3. **Memory Alignment**: The system automatically handles alignment requirements

   - Storage buffers are aligned to `minStorageBufferOffsetAlignment`
   - Uniform buffers respect `minUniformBufferOffsetAlignment`

### Buffer Usage Patterns

1. **Static Data**: Create once, use many times

   ```cpp
   // Static mesh data - device-local for best performance
   Buffer staticVertexBuffer = CreateBuffer(
       vertexDataSize,
       BufferUsage::Vertex,
       MemoryType::GPU
   );
   ```
2. **Dynamic Data**: Updated frequently

   ```cpp
   // Per-frame uniforms - host-visible for easy updates
   Buffer dynamicUniformBuffer = CreateBuffer(
       sizeof(FrameData),
       BufferUsage::Uniform,
       MemoryType::CPU
   );
   ```
3. **Streaming Data**: Large data sets transferred occasionally

   ```cpp
   // Use staging buffer pattern for large uploads
   Buffer CreateLargeDataBuffer(const void* data, size_t size)
   {
       // Create staging buffer
       Buffer stagingBuffer = CreateBuffer(size, BufferUsage::TransferSrc, MemoryType::CPU);

       // Upload data
       void* mapped = MapBuffer(stagingBuffer);
       memcpy(mapped, data, size);
       UnmapBuffer(stagingBuffer);

       // Create device buffer
       Buffer deviceBuffer = CreateBuffer(size, targetUsage, MemoryType::GPU);

       // Transfer
       CopyBuffer(stagingBuffer.resource->buffer, deviceBuffer.resource->buffer, size);

       return deviceBuffer;
   }
   ```

---

## Error Handling

### Validation and Assertions

The buffer system includes comprehensive validation:

```cpp
// Resource ID validation
uint32_t Buffer::ID() const
{
    SEDX_ASSERT(resource->resourceID != -1, "Invalid Buffer Resource ID!");
    return static_cast<uint32_t>(resource->resourceID);
}

// Memory type validation for mapping
void* MapBuffer(BufferResource &buffer)
{
    SEDX_ASSERT(buffer.memory & MemoryType::CPU, "Buffer not accessible to the CPU.");
    // ... mapping logic
}
```

### Common Error Scenarios

1. **Invalid Memory Type for Mapping**

   ```cpp
   // ❌ Error: Trying to map GPU-only buffer
   Buffer gpuBuffer = CreateBuffer(size, BufferUsage::Vertex, MemoryType::GPU);
   void* mapped = MapBuffer(gpuBuffer); // Will assert in debug build

   // ✅ Correct: Use CPU-accessible memory for mapping
   Buffer cpuBuffer = CreateBuffer(size, BufferUsage::Uniform, MemoryType::CPU);
   void* mapped = MapBuffer(cpuBuffer); // OK
   ```
2. **Buffer Overflow Protection**

   ```cpp
   // The Buffer class includes overflow protection
   void Buffer::Write(const void* data, uint64_t size, uint64_t offset) const
   {
       SEDX_CORE_ASSERT(offset + size <= this->size, "Buffer overflow!");
       memcpy(static_cast<byte *>(this->data) + offset, data, size);
   }
   ```
3. **Resource Cleanup**

   ```cpp
   // ✅ Automatic cleanup through RAII
   {
       Buffer buffer = CreateBuffer(size, usage, memory);
       // ... use buffer
   } // Buffer automatically cleaned up when out of scope

   // ✅ Manual cleanup if needed
   buffer.resource.Reset(); // Explicitly release resources

   ```

---

## Integration with Module System

### Module Lifecycle Integration

The buffer system integrates seamlessly with the module lifecycle:

```cpp
class RenderingModule : public Module
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing {} ===", GetName());
      
        // Create buffers during module initialization
        try
        {
            CreateRenderingBuffers();
            SEDX_CORE_INFO_TAG("RENDER", "Buffers created successfully");
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("RENDER", "Failed to create buffers: {}", e.what());
            throw;
        }
    }
  
    void OnDetach() override
    {
        SEDX_CORE_INFO("Cleaning up {}", GetName());
        // Buffers automatically cleaned up by smart pointers
        m_VertexBuffer.resource.Reset();
        m_IndexBuffer.resource.Reset();
        m_UniformBuffer.resource.Reset();
    }
  
    void OnUpdate() override
    {
        if (!m_IsEnabled) return;
        SEDX_PROFILE_SCOPE("RenderingModule::OnUpdate");
      
        // Update dynamic buffers
        UpdatePerFrameData();
    }
  
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
          
            ImGui::Checkbox("Enable Updates", &m_IsEnabled);
        }
        ImGui::End();
    }
  
private:
    void CreateRenderingBuffers()
    {
        // Create vertex buffer
        m_VertexBuffer = CreateBuffer(
            m_VertexDataSize,
            BufferUsage::Vertex,
            MemoryType::GPU,
            fmt::format("{} Vertex Buffer", GetName())
        );
      
        // Create index buffer
        m_IndexBuffer = CreateBuffer(
            m_IndexDataSize,
            BufferUsage::Index,
            MemoryType::GPU,
            fmt::format("{} Index Buffer", GetName())
        );
      
        // Create uniform buffer
        m_UniformBuffer = CreateBuffer(
            sizeof(UniformData),
            BufferUsage::Uniform,
            MemoryType::CPU,
            fmt::format("{} Uniform Buffer", GetName())
        );
    }
  
    void UpdatePerFrameData()
    {
        // Update uniform data
        UniformData uniforms;
        uniforms.modelMatrix = GetModelMatrix();
        uniforms.viewMatrix = GetViewMatrix();
        uniforms.projMatrix = GetProjectionMatrix();
      
        // Map and update
        void* mapped = MapBuffer(m_UniformBuffer);
        memcpy(mapped, &uniforms, sizeof(UniformData));
        UnmapBuffer(m_UniformBuffer);
    }
  
    Buffer m_VertexBuffer;
    Buffer m_IndexBuffer;
    Buffer m_UniformBuffer;
    bool m_IsEnabled = true;
  
    size_t m_VertexDataSize = 0;
    size_t m_IndexDataSize = 0;
};
```

### Configuration Management

Buffer modules can be configured through JSON:

```cpp
class ConfigurableBufferModule : public Module
{
public:
    struct BufferConfig
    {
        size_t maxVertexBufferSize = 64 * 1024 * 1024;    // 64MB
        size_t maxIndexBufferSize = 16 * 1024 * 1024;     // 16MB
        size_t uniformBufferSize = 1024;                   // 1KB
        bool enableDebugNames = true;
      
        void ToJSON(nlohmann::json& j) const
        {
            j["maxVertexBufferSize"] = maxVertexBufferSize;
            j["maxIndexBufferSize"] = maxIndexBufferSize;
            j["uniformBufferSize"] = uniformBufferSize;
            j["enableDebugNames"] = enableDebugNames;
        }
      
        void FromJSON(const nlohmann::json& j)
        {
            maxVertexBufferSize = j.value("maxVertexBufferSize", 64 * 1024 * 1024);
            maxIndexBufferSize = j.value("maxIndexBufferSize", 16 * 1024 * 1024);
            uniformBufferSize = j.value("uniformBufferSize", 1024);
            enableDebugNames = j.value("enableDebugNames", true);
        }
    };
  
    void OnAttach() override
    {
        LoadConfiguration();
        CreateBuffersFromConfig();
    }
  
    void OnDetach() override
    {
        SaveConfiguration();
    }
  
private:
    void LoadConfiguration()
    {
        const auto configPath = fmt::format("config/{}.json", GetName());
        try
        {
            if (std::filesystem::exists(configPath))
            {
                std::ifstream file(configPath);
                nlohmann::json j;
                file >> j;
                m_Config.FromJSON(j);
                SEDX_CORE_INFO_TAG("CONFIG", "Loaded buffer config for {}", GetName());
            }
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_WARN_TAG("CONFIG", "Failed to load buffer config: {}", e.what());
        }
    }
  
    void CreateBuffersFromConfig()
    {
        const std::string namePrefix = m_Config.enableDebugNames ? GetName() + " " : "";
      
        m_VertexBuffer = CreateBuffer(
            m_Config.maxVertexBufferSize,
            BufferUsage::Vertex,
            MemoryType::GPU,
            namePrefix + "Vertex Buffer"
        );
      
        m_IndexBuffer = CreateBuffer(
            m_Config.maxIndexBufferSize,
            BufferUsage::Index,
            MemoryType::GPU,
            namePrefix + "Index Buffer"
        );
      
        m_UniformBuffer = CreateBuffer(
            m_Config.uniformBufferSize,
            BufferUsage::Uniform,
            MemoryType::CPU,
            namePrefix + "Uniform Buffer"
        );
    }
  
    BufferConfig m_Config;
    Buffer m_VertexBuffer;
    Buffer m_IndexBuffer;
    Buffer m_UniformBuffer;
};
```

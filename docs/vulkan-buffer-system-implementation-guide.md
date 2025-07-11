# Scenery Editor X - Vulkan Buffer System Implementation Guide

---

## Overview

This guide provides detailed implementation patterns, advanced usage scenarios, and technical specifications for the Vulkan Buffer System in Scenery Editor X. It serves as a reference for developers implementing buffer management functionality and extending the existing system.

## Table of Contents

1. [Implementation Architecture](#implementation-architecture)
2. [Advanced Buffer Management](#advanced-buffer-management)
3. [Memory Pool Integration](#memory-pool-integration)
4. [Bindless Resource System](#bindless-resource-system)
5. [Performance Profiling](#performance-profiling)
6. [Custom Buffer Types](#custom-buffer-types)
7. [Synchronization Patterns](#synchronization-patterns)
8. [Testing and Validation](#testing-and-validation)
9. [Platform-Specific Considerations](#platform-specific-considerations)
10. [Debugging and Diagnostics](#debugging-and-diagnostics)

---

## Implementation Architecture

### Core Implementation Patterns

The Vulkan Buffer System follows several key implementation patterns that ensure consistency and maintainability across the codebase.

#### 1. Resource Factory Pattern

```cpp
class BufferFactory
{
public:
    /**
     * @brief Creates specialized buffers based on use case
     */
    static Buffer CreateVertexBuffer(const void* vertexData, size_t dataSize, const std::string& name = "")
    {
        SEDX_PROFILE_SCOPE("BufferFactory::CreateVertexBuffer");
      
        // Create staging buffer for upload
        Buffer stagingBuffer = CreateBuffer(
            dataSize,
            BufferUsage::TransferSrc,
            MemoryType::CPU,
            name + " Staging"
        );
      
        // Upload vertex data
        void* mapped = MapBuffer(stagingBuffer);
        memcpy(mapped, vertexData, dataSize);
        UnmapBuffer(stagingBuffer);
      
        // Create device-local vertex buffer
        Buffer vertexBuffer = CreateBuffer(
            dataSize,
            BufferUsage::Vertex,
            MemoryType::GPU,
            name
        );
      
        // Transfer data
        CopyBuffer(stagingBuffer.resource->buffer, vertexBuffer.resource->buffer, dataSize);
      
        return vertexBuffer;
    }
  
    static Buffer CreateDynamicUniformBuffer(size_t dataSize, const std::string& name = "")
    {
        SEDX_PROFILE_SCOPE("BufferFactory::CreateDynamicUniformBuffer");
      
        return CreateBuffer(
            dataSize,
            BufferUsage::Uniform,
            MemoryType::CPU,  // CPU accessible for frequent updates
            name
        );
    }
  
    static Buffer CreateStorageBuffer(size_t dataSize, bool cpuAccess = false, const std::string& name = "")
    {
        SEDX_PROFILE_SCOPE("BufferFactory::CreateStorageBuffer");
      
        return CreateBuffer(
            dataSize,
            BufferUsage::Storage,
            cpuAccess ? MemoryType::CPU : MemoryType::GPU,
            name
        );
    }
  
    static Buffer CreateIndirectBuffer(size_t commandCount, const std::string& name = "")
    {
        SEDX_PROFILE_SCOPE("BufferFactory::CreateIndirectBuffer");
      
        const size_t bufferSize = commandCount * sizeof(VkDrawIndirectCommand);
        return CreateBuffer(
            bufferSize,
            BufferUsage::Indirect | BufferUsage::Storage,  // Allow compute shader writes
            MemoryType::GPU,
            name
        );
    }
};
```

#### 2. Buffer Pool Manager

```cpp
class BufferPoolManager : public Module
{
public:
    explicit BufferPoolManager(const std::string& name = "BufferPoolManager")
        : Module(name)
    {
    }
  
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing {} ===", GetName());
      
        // Initialize different buffer pools
        InitializeStagingPool();
        InitializeUniformPool();
        InitializeStoragePool();
      
        SEDX_CORE_INFO_TAG("POOL", "Buffer pools initialized successfully");
    }
  
    void OnDetach() override
    {
        SEDX_CORE_INFO("Cleaning up {}", GetName());
      
        // Pools automatically cleaned up by smart pointers
        m_StagingPool.clear();
        m_UniformPool.clear();
        m_StoragePool.clear();
    }
  
    /**
     * @brief Acquires a staging buffer from the pool
     */
    Buffer AcquireStagingBuffer(size_t requiredSize)
    {
        std::lock_guard<std::mutex> lock(m_PoolMutex);
      
        // Find available buffer with sufficient size
        for (auto& poolBuffer : m_StagingPool)
        {
            if (!poolBuffer.inUse && poolBuffer.buffer.GetSize() >= requiredSize)
            {
                poolBuffer.inUse = true;
                poolBuffer.frameLastUsed = GetCurrentFrame();
                return poolBuffer.buffer;
            }
        }
      
        // Create new buffer if none available
        Buffer newBuffer = CreateBuffer(
            std::max(requiredSize, m_MinStagingSize),
            BufferUsage::TransferSrc,
            MemoryType::CPU,
            fmt::format("Staging Pool Buffer {}", m_StagingPool.size())
        );
      
        PooledBuffer pooledBuffer;
        pooledBuffer.buffer = newBuffer;
        pooledBuffer.inUse = true;
        pooledBuffer.frameLastUsed = GetCurrentFrame();
      
        m_StagingPool.push_back(pooledBuffer);
      
        SEDX_CORE_INFO_TAG("POOL", "Created new staging buffer (size: {} bytes)", 
                          newBuffer.GetSize());
      
        return newBuffer;
    }
  
    /**
     * @brief Returns a staging buffer to the pool
     */
    void ReleaseStagingBuffer(const Buffer& buffer)
    {
        std::lock_guard<std::mutex> lock(m_PoolMutex);
      
        for (auto& poolBuffer : m_StagingPool)
        {
            if (poolBuffer.buffer.ID() == buffer.ID())
            {
                poolBuffer.inUse = false;
                return;
            }
        }
      
        SEDX_CORE_WARN_TAG("POOL", "Attempted to release unknown staging buffer");
    }
  
    void OnUpdate() override
    {
        if (!m_IsEnabled) return;
        SEDX_PROFILE_SCOPE("BufferPoolManager::OnUpdate");
      
        // Periodically clean up unused buffers
        if (ShouldCleanupPools())
        {
            CleanupUnusedBuffers();
        }
    }
  
private:
    struct PooledBuffer
    {
        Buffer buffer;
        bool inUse = false;
        uint64_t frameLastUsed = 0;
    };
  
    void InitializeStagingPool()
    {
        // Pre-allocate common staging buffer sizes
        std::vector<size_t> commonSizes = {
            64 * 1024,      // 64KB
            256 * 1024,     // 256KB
            1024 * 1024,    // 1MB
            4 * 1024 * 1024 // 4MB
        };
      
        for (size_t size : commonSizes)
        {
            for (int i = 0; i < m_InitialPoolSize; ++i)
            {
                Buffer buffer = CreateBuffer(
                    size,
                    BufferUsage::TransferSrc,
                    MemoryType::CPU,
                    fmt::format("Staging Pool {}KB #{}", size / 1024, i)
                );
              
                PooledBuffer pooledBuffer;
                pooledBuffer.buffer = buffer;
                pooledBuffer.inUse = false;
                pooledBuffer.frameLastUsed = 0;
              
                m_StagingPool.push_back(pooledBuffer);
            }
        }
    }
  
    void InitializeUniformPool()
    {
        // Pre-allocate uniform buffers for common sizes
        for (int i = 0; i < m_InitialPoolSize * 2; ++i)
        {
            Buffer buffer = CreateBuffer(
                m_StandardUniformSize,
                BufferUsage::Uniform,
                MemoryType::CPU,
                fmt::format("Uniform Pool #{}", i)
            );
          
            PooledBuffer pooledBuffer;
            pooledBuffer.buffer = buffer;
            pooledBuffer.inUse = false;
            pooledBuffer.frameLastUsed = 0;
          
            m_UniformPool.push_back(pooledBuffer);
        }
    }
  
    void InitializeStoragePool()
    {
        // Pre-allocate storage buffers for common compute workloads
        std::vector<size_t> storageSizes = {
            1024 * 1024,    // 1MB
            4 * 1024 * 1024, // 4MB
            16 * 1024 * 1024 // 16MB
        };
      
        for (size_t size : storageSizes)
        {
            for (int i = 0; i < m_InitialPoolSize; ++i)
            {
                Buffer buffer = CreateBuffer(
                    size,
                    BufferUsage::Storage,
                    MemoryType::GPU,
                    fmt::format("Storage Pool {}MB #{}", size / (1024 * 1024), i)
                );
              
                PooledBuffer pooledBuffer;
                pooledBuffer.buffer = buffer;
                pooledBuffer.inUse = false;
                pooledBuffer.frameLastUsed = 0;
              
                m_StoragePool.push_back(pooledBuffer);
            }
        }
    }
  
    bool ShouldCleanupPools()
    {
        const uint64_t currentFrame = GetCurrentFrame();
        return (currentFrame - m_LastCleanupFrame) > m_CleanupInterval;
    }
  
    void CleanupUnusedBuffers()
    {
        std::lock_guard<std::mutex> lock(m_PoolMutex);
        const uint64_t currentFrame = GetCurrentFrame();
      
        auto cleanupPool = [&](std::vector<PooledBuffer>& pool, const std::string& poolName)
        {
            size_t removedCount = 0;
            pool.erase(std::remove_if(pool.begin(), pool.end(),
                [&](const PooledBuffer& poolBuffer)
                {
                    if (!poolBuffer.inUse && 
                        (currentFrame - poolBuffer.frameLastUsed) > m_MaxUnusedFrames)
                    {
                        removedCount++;
                        return true;
                    }
                    return false;
                }), pool.end());
              
            if (removedCount > 0)
            {
                SEDX_CORE_INFO_TAG("POOL", "Cleaned up {} unused buffers from {} pool", 
                                  removedCount, poolName);
            }
        };
      
        cleanupPool(m_StagingPool, "staging");
        cleanupPool(m_UniformPool, "uniform");
        cleanupPool(m_StoragePool, "storage");
      
        m_LastCleanupFrame = currentFrame;
    }
  
    std::vector<PooledBuffer> m_StagingPool;
    std::vector<PooledBuffer> m_UniformPool;
    std::vector<PooledBuffer> m_StoragePool;
  
    std::mutex m_PoolMutex;
    bool m_IsEnabled = true;
  
    static constexpr int m_InitialPoolSize = 4;
    static constexpr size_t m_MinStagingSize = 64 * 1024;     // 64KB
    static constexpr size_t m_StandardUniformSize = 1024;     // 1KB
    static constexpr uint64_t m_CleanupInterval = 120;        // frames
    static constexpr uint64_t m_MaxUnusedFrames = 300;        // frames
  
    uint64_t m_LastCleanupFrame = 0;
};
```



---

## Advanced Buffer Management

### 1. Hierarchical Buffer System

```cpp
class HierarchicalBufferManager : public Module
{
public:
    /**
     * @brief Multi-level buffer hierarchy for different access patterns
     */
    enum class BufferTier
    {
        Hot,        // Frequently accessed (CPU cache-friendly)
        Warm,       // Occasionally accessed (GPU local)
        Cold        // Rarely accessed (system memory)
    };
  
    struct BufferDescriptor
    {
        size_t size;
        BufferUsageFlags usage;
        BufferTier tier;
        std::string name;
        uint32_t accessFrequency = 0;  // Access counter for tier promotion/demotion
    };
  
    Buffer CreateTieredBuffer(const BufferDescriptor& desc)
    {
        SEDX_PROFILE_SCOPE("HierarchicalBufferManager::CreateTieredBuffer");
      
        MemoryFlags memoryType = DetermineMemoryType(desc.tier);
      
        Buffer buffer = CreateBuffer(
            desc.size,
            desc.usage,
            memoryType,
            desc.name
        );
      
        // Track buffer for tier management
        m_BufferMetadata[buffer.ID()] = {
            .descriptor = desc,
            .lastAccessFrame = GetCurrentFrame(),
            .accessCount = 0,
            .tier = desc.tier
        };
      
        return buffer;
    }
  
    void OnUpdate() override
    {
        if (!m_IsEnabled) return;
        SEDX_PROFILE_SCOPE("HierarchicalBufferManager::OnUpdate");
      
        // Analyze buffer access patterns and adjust tiers
        if (ShouldAnalyzeTiers())
        {
            AnalyzeAndAdjustTiers();
        }
    }
  
private:
    struct BufferMetadata
    {
        BufferDescriptor descriptor;
        uint64_t lastAccessFrame;
        uint32_t accessCount;
        BufferTier tier;
    };
  
    MemoryFlags DetermineMemoryType(BufferTier tier)
    {
        switch (tier)
        {
            case BufferTier::Hot:
                return MemoryType::CPU;  // Host-visible for fast access
            case BufferTier::Warm:
                return MemoryType::GPU;  // Device-local for GPU operations
            case BufferTier::Cold:
                return MemoryType::GPU;  // Device-local, less frequently accessed
            default:
                return MemoryType::GPU;
        }
    }
  
    void AnalyzeAndAdjustTiers()
    {
        SEDX_PROFILE_SCOPE("AnalyzeAndAdjustTiers");
      
        const uint64_t currentFrame = GetCurrentFrame();
      
        for (auto& [bufferID, metadata] : m_BufferMetadata)
        {
            const uint64_t framesSinceAccess = currentFrame - metadata.lastAccessFrame;
            const float accessRate = static_cast<float>(metadata.accessCount) / framesSinceAccess;
          
            BufferTier newTier = DetermineTierFromAccessPattern(accessRate, framesSinceAccess);
          
            if (newTier != metadata.tier)
            {
                // Tier change needed - implement migration strategy
                SEDX_CORE_INFO_TAG("TIER", "Buffer {} tier change: {} -> {}", 
                                  bufferID, 
                                  TierToString(metadata.tier), 
                                  TierToString(newTier));
              
                // In a full implementation, this would trigger buffer migration
                metadata.tier = newTier;
            }
        }
    }
  
    BufferTier DetermineTierFromAccessPattern(float accessRate, uint64_t framesSinceAccess)
    {
        if (accessRate > m_HotTierThreshold)
            return BufferTier::Hot;
        else if (accessRate > m_WarmTierThreshold && framesSinceAccess < m_ColdFrameThreshold)
            return BufferTier::Warm;
        else
            return BufferTier::Cold;
    }
  
    std::unordered_map<uint32_t, BufferMetadata> m_BufferMetadata;
  
    static constexpr float m_HotTierThreshold = 0.1f;      // 10% access rate
    static constexpr float m_WarmTierThreshold = 0.01f;    // 1% access rate
    static constexpr uint64_t m_ColdFrameThreshold = 1000; // frames
};
```

### 2. Adaptive Buffer Sizing

```cpp
class AdaptiveBufferSystem : public Module
{
public:
    /**
     * @brief Buffer that grows/shrinks based on usage patterns
     */
    class AdaptiveBuffer
    {
    public:
        AdaptiveBuffer(BufferUsageFlags usage, MemoryFlags memory, const std::string& name)
            : m_Usage(usage), m_Memory(memory), m_Name(name)
        {
            // Start with initial size
            Resize(m_InitialSize);
        }
      
        void WriteData(const void* data, size_t dataSize, size_t offset = 0)
        {
            SEDX_PROFILE_SCOPE("AdaptiveBuffer::WriteData");
          
            // Check if buffer needs to grow
            if (offset + dataSize > m_CurrentSize)
            {
                size_t newSize = CalculateNewSize(offset + dataSize);
                Resize(newSize);
            }
          
            // Write data to buffer
            if (m_Memory & MemoryType::CPU)
            {
                void* mapped = MapBuffer(m_Buffer);
                memcpy(static_cast<byte*>(mapped) + offset, data, dataSize);
                UnmapBuffer(m_Buffer);
            }
            else
            {
                // Use staging buffer for GPU-only buffers
                UploadDataViaStaging(data, dataSize, offset);
            }
          
            // Update usage statistics
            m_MaxUsedSize = std::max(m_MaxUsedSize, offset + dataSize);
            m_WriteCount++;
        }
      
        void OnFrameEnd()
        {
            SEDX_PROFILE_SCOPE("AdaptiveBuffer::OnFrameEnd");
          
            // Check if buffer should shrink
            if (ShouldShrink())
            {
                size_t newSize = CalculateShrinkSize();
                if (newSize < m_CurrentSize)
                {
                    Resize(newSize);
                    SEDX_CORE_INFO_TAG("ADAPTIVE", "Shrunk buffer '{}' from {} to {} bytes", 
                                      m_Name, m_CurrentSize, newSize);
                }
            }
          
            // Reset frame statistics
            m_MaxUsedSize = 0;
            m_WriteCount = 0;
        }
      
        const Buffer& GetBuffer() const { return m_Buffer; }
        size_t GetCurrentSize() const { return m_CurrentSize; }
      
    private:
        void Resize(size_t newSize)
        {
            SEDX_PROFILE_SCOPE("AdaptiveBuffer::Resize");
          
            if (newSize == m_CurrentSize) return;
          
            // Create new buffer
            Buffer newBuffer = CreateBuffer(
                newSize,
                m_Usage,
                m_Memory,
                m_Name
            );
          
            // Copy existing data if buffer exists
            if (m_Buffer.resource && m_CurrentSize > 0)
            {
                size_t copySize = std::min(m_CurrentSize, newSize);
                CopyBuffer(m_Buffer.resource->buffer, newBuffer.resource->buffer, copySize);
            }
          
            // Replace old buffer
            m_Buffer = newBuffer;
            m_CurrentSize = newSize;
          
            SEDX_CORE_INFO_TAG("ADAPTIVE", "Resized buffer '{}' to {} bytes", m_Name, newSize);
        }
      
        size_t CalculateNewSize(size_t requiredSize)
        {
            // Grow by growth factor, but at least to required size
            size_t growthSize = static_cast<size_t>(m_CurrentSize * m_GrowthFactor);
            return std::max(requiredSize, growthSize);
        }
      
        size_t CalculateShrinkSize()
        {
            // Shrink to usage plus overhead
            return static_cast<size_t>(m_MaxUsedSize * m_ShrinkFactor);
        }
      
        bool ShouldShrink()
        {
            // Shrink if utilization is low for several frames
            float utilization = static_cast<float>(m_MaxUsedSize) / m_CurrentSize;
            return utilization < m_ShrinkThreshold && m_WriteCount > 0;
        }
      
        void UploadDataViaStaging(const void* data, size_t dataSize, size_t offset)
        {
            // Create temporary staging buffer
            Buffer stagingBuffer = CreateBuffer(
                dataSize,
                BufferUsage::TransferSrc,
                MemoryType::CPU,
                m_Name + " Temp Staging"
            );
          
            // Upload to staging
            void* mapped = MapBuffer(stagingBuffer);
            memcpy(mapped, data, dataSize);
            UnmapBuffer(stagingBuffer);
          
            // Copy to main buffer with offset
            // Note: This is simplified - real implementation would need command buffer with offset
            CopyBuffer(stagingBuffer.resource->buffer, m_Buffer.resource->buffer, dataSize);
        }
      
        Buffer m_Buffer;
        BufferUsageFlags m_Usage;
        MemoryFlags m_Memory;
        std::string m_Name;
      
        size_t m_CurrentSize = 0;
        size_t m_MaxUsedSize = 0;
        uint32_t m_WriteCount = 0;
      
        static constexpr size_t m_InitialSize = 1024;           // 1KB
        static constexpr float m_GrowthFactor = 1.5f;           // 50% growth
        static constexpr float m_ShrinkFactor = 1.2f;           // 20% overhead
        static constexpr float m_ShrinkThreshold = 0.5f;        // 50% utilization
    };
};
```



---

## Memory Pool Integration

### 1. Custom Memory Pool Creation

```cpp
class CustomMemoryPoolManager : public Module
{
public:
    /**
     * @brief Creates specialized memory pools for different buffer types
     */
    struct PoolConfiguration
    {
        VkDeviceSize blockSize;
        uint32_t minBlockCount;
        uint32_t maxBlockCount;
        VkMemoryPropertyFlags memoryType;
        std::string name;
    };
  
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing {} ===", GetName());
      
        // Create specialized pools
        CreateVertexBufferPool();
        CreateUniformBufferPool();
        CreateStorageBufferPool();
        CreateRayTracingPool();
      
        SEDX_CORE_INFO_TAG("POOL", "All memory pools created successfully");
    }
  
    Buffer AllocateFromPool(const std::string& poolName, size_t size, BufferUsageFlags usage)
    {
        SEDX_PROFILE_SCOPE("CustomMemoryPoolManager::AllocateFromPool");
      
        auto poolIt = m_Pools.find(poolName);
        if (poolIt == m_Pools.end())
        {
            SEDX_CORE_ERROR_TAG("POOL", "Pool '{}' not found", poolName);
            // Fallback to standard allocation
            return CreateBuffer(size, usage, MemoryType::GPU);
        }
      
        // Use VMA pool for allocation
        VmaPool pool = poolIt->second.pool;
      
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = static_cast<VkBufferUsageFlagBits>(usage);
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.pool = pool;  // Use our custom pool
      
        const Ref<BufferResource> resource = CreateRef<BufferResource>();
        const auto device = RenderContext::Get()->GetLogicDevice();
      
        VkResult result = vmaCreateBuffer(
            device->GetMemoryAllocator(),
            &bufferInfo,
            &allocInfo,
            &resource->buffer,
            &resource->allocation,
            nullptr
        );
      
        if (result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("POOL", "Failed to allocate from pool '{}'", poolName);
            return {}; // Return empty buffer
        }
      
        // Create buffer wrapper
        Buffer buffer;
        buffer.resource = resource;
        buffer.size = size;
        buffer.usage = usage;
        buffer.memory = MemoryType::GPU; // Pools are GPU-local by default
      
        // Update pool statistics
        m_Pools[poolName].allocatedBytes += size;
        m_Pools[poolName].allocationCount++;
      
        return buffer;
    }
  
private:
    struct PoolInfo
    {
        VmaPool pool;
        PoolConfiguration config;
        uint64_t allocatedBytes = 0;
        uint32_t allocationCount = 0;
    };
  
    void CreateVertexBufferPool()
    {
        PoolConfiguration config = {
            .blockSize = 64 * 1024 * 1024,  // 64MB blocks
            .minBlockCount = 2,
            .maxBlockCount = 8,
            .memoryType = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .name = "VertexBufferPool"
        };
      
        CreatePool("vertex", config);
    }
  
    void CreateUniformBufferPool()
    {
        PoolConfiguration config = {
            .blockSize = 16 * 1024 * 1024,  // 16MB blocks
            .minBlockCount = 1,
            .maxBlockCount = 4,
            .memoryType = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            .name = "UniformBufferPool"
        };
      
        CreatePool("uniform", config);
    }
  
    void CreateStorageBufferPool()
    {
        PoolConfiguration config = {
            .blockSize = 128 * 1024 * 1024, // 128MB blocks
            .minBlockCount = 1,
            .maxBlockCount = 6,
            .memoryType = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .name = "StorageBufferPool"
        };
      
        CreatePool("storage", config);
    }
  
    void CreateRayTracingPool()
    {
        PoolConfiguration config = {
            .blockSize = 256 * 1024 * 1024, // 256MB blocks
            .minBlockCount = 1,
            .maxBlockCount = 4,
            .memoryType = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .name = "RayTracingPool"
        };
      
        CreatePool("raytracing", config);
    }
  
    void CreatePool(const std::string& name, const PoolConfiguration& config)
    {
        SEDX_PROFILE_SCOPE("CreatePool");
      
        const auto device = RenderContext::Get()->GetLogicDevice();
      
        // Find memory type index
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(device->GetPhysicalDevice()->GetDevice(), &memProps);
      
        uint32_t memoryTypeIndex = UINT32_MAX;
        for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
        {
            if ((memProps.memoryTypes[i].propertyFlags & config.memoryType) == config.memoryType)
            {
                memoryTypeIndex = i;
                break;
            }
        }
      
        if (memoryTypeIndex == UINT32_MAX)
        {
            SEDX_CORE_ERROR_TAG("POOL", "Could not find suitable memory type for pool '{}'", name);
            return;
        }
      
        // Create VMA pool
        VmaPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.memoryTypeIndex = memoryTypeIndex;
        poolCreateInfo.blockSize = config.blockSize;
        poolCreateInfo.minBlockCount = config.minBlockCount;
        poolCreateInfo.maxBlockCount = config.maxBlockCount;
        poolCreateInfo.flags = VMA_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT;
      
        VmaPool pool;
        VkResult result = vmaCreatePool(device->GetMemoryAllocator(), &poolCreateInfo, &pool);
      
        if (result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("POOL", "Failed to create memory pool '{}'", name);
            return;
        }
      
        // Store pool info
        PoolInfo poolInfo = {
            .pool = pool,
            .config = config
        };
      
        m_Pools[name] = poolInfo;
      
        SEDX_CORE_INFO_TAG("POOL", "Created memory pool '{}' (block size: {} MB)", 
                          name, config.blockSize / (1024 * 1024));
    }
  
    std::unordered_map<std::string, PoolInfo> m_Pools;
};
```



---

## Bindless Resource System

### 1. Bindless Buffer Management

```cpp
class BindlessBufferSystem : public Module
{
public:
    /**
     * @brief Maximum number of bindless buffer resources
     */
    static constexpr uint32_t MAX_BINDLESS_BUFFERS = 16384;
  
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing {} ===", GetName());
      
        InitializeDescriptorSet();
        InitializeResourcePool();
      
        SEDX_CORE_INFO_TAG("BINDLESS", "Bindless buffer system initialized");
    }
  
    /**
     * @brief Registers a buffer for bindless access
     */
    uint32_t RegisterBuffer(const Buffer& buffer)
    {
        SEDX_PROFILE_SCOPE("BindlessBufferSystem::RegisterBuffer");
      
        std::lock_guard<std::mutex> lock(m_ResourceMutex);
      
        // Get next available resource ID
        if (m_AvailableResourceIDs.empty())
        {
            SEDX_CORE_ERROR_TAG("BINDLESS", "No available bindless resource IDs");
            return INVALID_RESOURCE_ID;
        }
      
        uint32_t resourceID = m_AvailableResourceIDs.back();
        m_AvailableResourceIDs.pop_back();
      
        // Update descriptor set
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = buffer.resource->buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = buffer.size;
      
        VkWriteDescriptorSet writeDescriptor = {};
        writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptor.dstSet = m_BindlessDescriptorSet;
        writeDescriptor.dstBinding = STORAGE_BUFFER_BINDING;
        writeDescriptor.dstArrayElement = resourceID;
        writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeDescriptor.descriptorCount = 1;
        writeDescriptor.pBufferInfo = &bufferInfo;
      
        const auto device = RenderContext::Get()->GetLogicDevice();
        vkUpdateDescriptorSets(device->GetDevice(), 1, &writeDescriptor, 0, nullptr);
      
        // Store resource info
        ResourceInfo info = {
            .buffer = buffer,
            .resourceID = resourceID,
            .registrationFrame = GetCurrentFrame()
        };
      
        m_RegisteredResources[resourceID] = info;
      
        SEDX_CORE_INFO_TAG("BINDLESS", "Registered buffer '{}' with resource ID {}", 
                          buffer.resource->name, resourceID);
      
        return resourceID;
    }
  
    /**
     * @brief Unregisters a buffer from bindless access
     */
    void UnregisterBuffer(uint32_t resourceID)
    {
        SEDX_PROFILE_SCOPE("BindlessBufferSystem::UnregisterBuffer");
      
        std::lock_guard<std::mutex> lock(m_ResourceMutex);
      
        auto it = m_RegisteredResources.find(resourceID);
        if (it == m_RegisteredResources.end())
        {
            SEDX_CORE_WARN_TAG("BINDLESS", "Attempted to unregister unknown resource ID {}", resourceID);
            return;
        }
      
        // Clear descriptor entry
        VkDescriptorBufferInfo nullBufferInfo = {};
      
        VkWriteDescriptorSet writeDescriptor = {};
        writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptor.dstSet = m_BindlessDescriptorSet;
        writeDescriptor.dstBinding = STORAGE_BUFFER_BINDING;
        writeDescriptor.dstArrayElement = resourceID;
        writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeDescriptor.descriptorCount = 1;
        writeDescriptor.pBufferInfo = &nullBufferInfo;
      
        const auto device = RenderContext::Get()->GetLogicDevice();
        vkUpdateDescriptorSets(device->GetDevice(), 1, &writeDescriptor, 0, nullptr);
      
        // Return resource ID to pool
        m_AvailableResourceIDs.push_back(resourceID);
        m_RegisteredResources.erase(it);
      
        SEDX_CORE_INFO_TAG("BINDLESS", "Unregistered resource ID {}", resourceID);
    }
  
    /**
     * @brief Gets the bindless descriptor set for pipeline binding
     */
    VkDescriptorSet GetDescriptorSet() const { return m_BindlessDescriptorSet; }
  
    void OnUIRender() override
    {
        if (ImGui::Begin("Bindless Buffer Debug"))
        {
            ImGui::Text("Registered Resources: %zu / %u", 
                       m_RegisteredResources.size(), MAX_BINDLESS_BUFFERS);
            ImGui::Text("Available Resource IDs: %zu", m_AvailableResourceIDs.size());
          
            if (ImGui::CollapsingHeader("Registered Buffers"))
            {
                for (const auto& [resourceID, info] : m_RegisteredResources)
                {
                    ImGui::Text("ID %u: %s (Frame %llu)", 
                               resourceID, 
                               info.buffer.resource->name.c_str(),
                               info.registrationFrame);
                }
            }
        }
        ImGui::End();
    }
  
private:
    struct ResourceInfo
    {
        Buffer buffer;
        uint32_t resourceID;
        uint64_t registrationFrame;
    };
  
    void InitializeDescriptorSet()
    {
        const auto device = RenderContext::Get()->GetLogicDevice();
      
        // Create descriptor pool
        VkDescriptorPoolSize poolSize = {};
        poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSize.descriptorCount = MAX_BINDLESS_BUFFERS;
      
        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        poolInfo.maxSets = 1;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
      
        VK_CHECK_RESULT(vkCreateDescriptorPool(device->GetDevice(), &poolInfo, nullptr, &m_DescriptorPool));
      
        // Create descriptor set layout
        VkDescriptorSetLayoutBinding binding = {};
        binding.binding = STORAGE_BUFFER_BINDING;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        binding.descriptorCount = MAX_BINDLESS_BUFFERS;
        binding.stageFlags = VK_SHADER_STAGE_ALL;
      
        VkDescriptorBindingFlags bindingFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
                                               VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
      
        VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo = {};
        bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        bindingFlagsInfo.bindingCount = 1;
        bindingFlagsInfo.pBindingFlags = &bindingFlags;
      
        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &binding;
        layoutInfo.pNext = &bindingFlagsInfo;
      
        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->GetDevice(), &layoutInfo, nullptr, &m_DescriptorSetLayout));
      
        // Allocate descriptor set
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_DescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_DescriptorSetLayout;
      
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device->GetDevice(), &allocInfo, &m_BindlessDescriptorSet));
    }
  
    void InitializeResourcePool()
    {
        // Initialize available resource ID pool
        m_AvailableResourceIDs.reserve(MAX_BINDLESS_BUFFERS);
        for (uint32_t i = 0; i < MAX_BINDLESS_BUFFERS; ++i)
        {
            m_AvailableResourceIDs.push_back(i);
        }
      
        // Reverse so we allocate from 0 upwards
        std::reverse(m_AvailableResourceIDs.begin(), m_AvailableResourceIDs.end());
    }
  
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_BindlessDescriptorSet = VK_NULL_HANDLE;
  
    std::unordered_map<uint32_t, ResourceInfo> m_RegisteredResources;
    std::vector<uint32_t> m_AvailableResourceIDs;
    std::mutex m_ResourceMutex;
  
    static constexpr uint32_t STORAGE_BUFFER_BINDING = 1;
    static constexpr uint32_t INVALID_RESOURCE_ID = UINT32_MAX;
};
```



---

## Performance Profiling

### 1. Buffer Performance Monitor

```cpp
class BufferPerformanceMonitor : public Module
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing {} ===", GetName());
      
        // Initialize performance counters
        ResetCounters();
      
        SEDX_CORE_INFO_TAG("PERF", "Buffer performance monitoring enabled");
    }
  
    void OnUpdate() override
    {
        if (!m_IsEnabled) return;
        SEDX_PROFILE_SCOPE("BufferPerformanceMonitor::OnUpdate");
      
        // Update performance metrics
        UpdateMetrics();
      
        // Check for performance warnings
        if (ShouldReportMetrics())
        {
            ReportPerformanceMetrics();
        }
    }
  
    /**
     * @brief Records a buffer creation event
     */
    void RecordBufferCreation(const Buffer& buffer, uint64_t creationTimeNs)
    {
        std::lock_guard<std::mutex> lock(m_MetricsMutex);
      
        m_CreationStats.totalCreations++;
        m_CreationStats.totalCreationTime += creationTimeNs;
        m_CreationStats.averageCreationTime = m_CreationStats.totalCreationTime / m_CreationStats.totalCreations;
      
        // Track size distribution
        size_t sizeCategory = GetSizeCategory(buffer.GetSize());
        m_SizeDistribution[sizeCategory]++;
      
        // Track memory type usage
        if (buffer.memory & MemoryType::CPU)
            m_MemoryTypeStats.cpuBuffers++;
        else
            m_MemoryTypeStats.gpuBuffers++;
          
        m_MemoryTypeStats.totalAllocatedBytes += buffer.GetSize();
    }
  
    /**
     * @brief Records a buffer mapping event
     */
    void RecordBufferMapping(uint32_t bufferID, uint64_t mappingTimeNs)
    {
        std::lock_guard<std::mutex> lock(m_MetricsMutex);
      
        m_MappingStats.totalMappings++;
        m_MappingStats.totalMappingTime += mappingTimeNs;
        m_MappingStats.averageMappingTime = m_MappingStats.totalMappingTime / m_MappingStats.totalMappings;
      
        // Track per-buffer mapping frequency
        m_BufferMappingFrequency[bufferID]++;
    }
  
    /**
     * @brief Records a buffer copy operation
     */
    void RecordBufferCopy(size_t dataSize, uint64_t copyTimeNs)
    {
        std::lock_guard<std::mutex> lock(m_MetricsMutex);
      
        m_CopyStats.totalCopies++;
        m_CopyStats.totalDataTransferred += dataSize;
        m_CopyStats.totalCopyTime += copyTimeNs;
        m_CopyStats.averageThroughput = 
            static_cast<double>(m_CopyStats.totalDataTransferred) / (m_CopyStats.totalCopyTime / 1e9);
    }
  
    void OnUIRender() override
    {
        if (ImGui::Begin("Buffer Performance Monitor"))
        {
            std::lock_guard<std::mutex> lock(m_MetricsMutex);
          
            if (ImGui::CollapsingHeader("Creation Statistics"))
            {
                ImGui::Text("Total Creations: %llu", m_CreationStats.totalCreations);
                ImGui::Text("Average Creation Time: %.2f ms", 
                           m_CreationStats.averageCreationTime / 1e6);
            }
          
            if (ImGui::CollapsingHeader("Memory Usage"))
            {
                ImGui::Text("CPU Buffers: %u", m_MemoryTypeStats.cpuBuffers);
                ImGui::Text("GPU Buffers: %u", m_MemoryTypeStats.gpuBuffers);
                ImGui::Text("Total Allocated: %.2f MB", 
                           m_MemoryTypeStats.totalAllocatedBytes / (1024.0 * 1024.0));
            }
          
            if (ImGui::CollapsingHeader("Mapping Statistics"))
            {
                ImGui::Text("Total Mappings: %llu", m_MappingStats.totalMappings);
                ImGui::Text("Average Mapping Time: %.2f µs", 
                           m_MappingStats.averageMappingTime / 1e3);
              
                if (ImGui::TreeNode("Frequent Mappers"))
                {
                    for (const auto& [bufferID, count] : m_BufferMappingFrequency)
                    {
                        if (count > 100) // Only show frequently mapped buffers
                        {
                            ImGui::Text("Buffer %u: %u mappings", bufferID, count);
                        }
                    }
                    ImGui::TreePop();
                }
            }
          
            if (ImGui::CollapsingHeader("Copy Statistics"))
            {
                ImGui::Text("Total Copies: %llu", m_CopyStats.totalCopies);
                ImGui::Text("Data Transferred: %.2f MB", 
                           m_CopyStats.totalDataTransferred / (1024.0 * 1024.0));
                ImGui::Text("Average Throughput: %.2f MB/s", 
                           m_CopyStats.averageThroughput / (1024.0 * 1024.0));
            }
          
            if (ImGui::Button("Reset Counters"))
            {
                ResetCounters();
            }
        }
        ImGui::End();
    }
  
private:
    struct CreationStats
    {
        uint64_t totalCreations = 0;
        uint64_t totalCreationTime = 0;
        uint64_t averageCreationTime = 0;
    };
  
    struct MemoryTypeStats
    {
        uint32_t cpuBuffers = 0;
        uint32_t gpuBuffers = 0;
        uint64_t totalAllocatedBytes = 0;
    };
  
    struct MappingStats
    {
        uint64_t totalMappings = 0;
        uint64_t totalMappingTime = 0;
        uint64_t averageMappingTime = 0;
    };
  
    struct CopyStats
    {
        uint64_t totalCopies = 0;
        uint64_t totalDataTransferred = 0;
        uint64_t totalCopyTime = 0;
        double averageThroughput = 0.0; // bytes/second
    };
  
    size_t GetSizeCategory(size_t bufferSize)
    {
        if (bufferSize <= 1024) return 0;          // ≤ 1KB
        else if (bufferSize <= 64 * 1024) return 1;     // ≤ 64KB
        else if (bufferSize <= 1024 * 1024) return 2;   // ≤ 1MB
        else if (bufferSize <= 16 * 1024 * 1024) return 3; // ≤ 16MB
        else return 4;                             // > 16MB
    }
  
    void UpdateMetrics()
    {
        // Update frame-based metrics
        const uint64_t currentFrame = GetCurrentFrame();
      
        if (currentFrame - m_LastMetricsFrame > m_MetricsUpdateInterval)
        {
            // Calculate frame-based statistics
            m_LastMetricsFrame = currentFrame;
        }
    }
  
    bool ShouldReportMetrics()
    {
        const uint64_t currentFrame = GetCurrentFrame();
        return (currentFrame - m_LastReportFrame) > m_ReportInterval;
    }
  
    void ReportPerformanceMetrics()
    {
        std::lock_guard<std::mutex> lock(m_MetricsMutex);
      
        SEDX_CORE_INFO_TAG("PERF", "=== Buffer Performance Report ===");
        SEDX_CORE_INFO_TAG("PERF", "Buffers Created: {}", m_CreationStats.totalCreations);
        SEDX_CORE_INFO_TAG("PERF", "Average Creation Time: {:.2f} ms", 
                          m_CreationStats.averageCreationTime / 1e6);
        SEDX_CORE_INFO_TAG("PERF", "Total Memory Allocated: {:.2f} MB", 
                          m_MemoryTypeStats.totalAllocatedBytes / (1024.0 * 1024.0));
        SEDX_CORE_INFO_TAG("PERF", "Average Copy Throughput: {:.2f} MB/s", 
                          m_CopyStats.averageThroughput / (1024.0 * 1024.0));
      
        m_LastReportFrame = GetCurrentFrame();
    }
  
    void ResetCounters()
    {
        std::lock_guard<std::mutex> lock(m_MetricsMutex);
      
        m_CreationStats = {};
        m_MemoryTypeStats = {};
        m_MappingStats = {};
        m_CopyStats = {};
        m_SizeDistribution.clear();
        m_BufferMappingFrequency.clear();
    }
  
    CreationStats m_CreationStats;
    MemoryTypeStats m_MemoryTypeStats;
    MappingStats m_MappingStats;
    CopyStats m_CopyStats;
  
    std::unordered_map<size_t, uint32_t> m_SizeDistribution;
    std::unordered_map<uint32_t, uint32_t> m_BufferMappingFrequency;
  
    std::mutex m_MetricsMutex;
    bool m_IsEnabled = true;
  
    uint64_t m_LastMetricsFrame = 0;
    uint64_t m_LastReportFrame = 0;
  
    static constexpr uint64_t m_MetricsUpdateInterval = 60;  // frames
    static constexpr uint64_t m_ReportInterval = 3600;      // frames (1 minute at 60fps)
};
```

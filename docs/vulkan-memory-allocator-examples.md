# Scenery Editor X - Vulkan Memory Allocator Usage Examples

---

## Overview

This document provides comprehensive examples of using the Vulkan Memory Allocator system in Scenery Editor X. The examples range from basic allocation patterns to advanced optimization techniques, demonstrating real-world usage scenarios for 3D graphics applications.

---

## Basic Usage Examples

### 1. Simple Buffer Allocation

```cpp
#include <SceneryEditorX/renderer/vulkan/vk_allocator.h>
#include <SceneryEditorX/logging/logging.hpp>

class MeshLoader
{
public:
    void LoadMeshData(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    {
        // Create allocator for mesh loading
        auto allocator = CreateRef<MemoryAllocator>("MeshLoader");
    
        // Calculate buffer sizes
        VkDeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();
        VkDeviceSize indexBufferSize = sizeof(uint32_t) * indices.size();
    
        // Create vertex buffer
        VkBufferCreateInfo vertexBufferInfo{};
        vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertexBufferInfo.size = vertexBufferSize;
        vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
        VkBuffer vertexBuffer;
        VmaAllocation vertexAllocation = allocator->AllocateBuffer(
            vertexBufferInfo, 
            VMA_MEMORY_USAGE_GPU_ONLY, 
            vertexBuffer
        );
    
        SEDX_CORE_ASSERT(vertexAllocation != nullptr, "Failed to allocate vertex buffer");
        SEDX_CORE_INFO("Allocated vertex buffer: {} KB", vertexBufferSize / 1024);
    
        // Create index buffer
        VkBufferCreateInfo indexBufferInfo{};
        indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        indexBufferInfo.size = indexBufferSize;
        indexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        indexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
        VkBuffer indexBuffer;
        VmaAllocation indexAllocation = allocator->AllocateBuffer(
            indexBufferInfo, 
            VMA_MEMORY_USAGE_GPU_ONLY, 
            indexBuffer
        );
    
        SEDX_CORE_ASSERT(indexAllocation != nullptr, "Failed to allocate index buffer");
        SEDX_CORE_INFO("Allocated index buffer: {} KB", indexBufferSize / 1024);
    
        // Upload data using staging buffers
        UploadVertexData(vertices, vertexBuffer, vertexAllocation, allocator);
        UploadIndexData(indices, indexBuffer, indexAllocation, allocator);
    
        // Store for later use
        m_VertexBuffer = vertexBuffer;
        m_VertexAllocation = vertexAllocation;
        m_IndexBuffer = indexBuffer;
        m_IndexAllocation = indexAllocation;
        m_Allocator = allocator;
    }
  
    ~MeshLoader()
    {
        // Cleanup resources
        if (m_VertexBuffer != VK_NULL_HANDLE) {
            m_Allocator->DestroyBuffer(m_VertexBuffer, m_VertexAllocation);
        }
        if (m_IndexBuffer != VK_NULL_HANDLE) {
            m_Allocator->DestroyBuffer(m_IndexBuffer, m_IndexAllocation);
        }
    }
  
private:
    void UploadVertexData(const std::vector<Vertex>& vertices, 
                         VkBuffer destBuffer, 
                         VmaAllocation destAllocation,
                         Ref<MemoryAllocator> allocator)
    {
        VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();
    
        // Create staging buffer
        VkBufferCreateInfo stagingInfo{};
        stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingInfo.size = bufferSize;
        stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
        VkBuffer stagingBuffer;
        VmaAllocation stagingAllocation = allocator->AllocateBuffer(
            stagingInfo, 
            VMA_MEMORY_USAGE_CPU_TO_GPU, 
            stagingBuffer
        );
    
        // Map and copy data
        void* mappedData = allocator->MapMemory<void>(stagingAllocation);
        memcpy(mappedData, vertices.data(), bufferSize);
        MemoryAllocator::UnmapMemory(stagingAllocation);
    
        // Copy from staging to GPU buffer
        CopyBuffer(stagingBuffer, destBuffer, bufferSize);
    
        // Cleanup staging buffer
        allocator->DestroyBuffer(stagingBuffer, stagingAllocation);
    }
  
    VkBuffer m_VertexBuffer = VK_NULL_HANDLE;
    VmaAllocation m_VertexAllocation = nullptr;
    VkBuffer m_IndexBuffer = VK_NULL_HANDLE;
    VmaAllocation m_IndexAllocation = nullptr;
    Ref<MemoryAllocator> m_Allocator;
};
```

### 2. Texture Allocation

```cpp
class TextureLoader
{
public:
    Ref<Texture2D> LoadTexture(const std::string& filepath)
    {
        // Load image data from file
        int width, height, channels;
        stbi_uc* pixels = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    
        SEDX_CORE_ASSERT(pixels != nullptr, "Failed to load texture: {}", filepath);
    
        VkDeviceSize imageSize = width * height * 4; // RGBA
    
        // Create allocator for texture operations
        auto allocator = CreateRef<MemoryAllocator>("TextureLoader");
    
        // Create image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(width);
        imageInfo.extent.height = static_cast<uint32_t>(height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | 
                         VK_IMAGE_USAGE_TRANSFER_SRC_BIT | 
                         VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
        VkImage image;
        VkDeviceSize allocatedSize;
        VmaAllocation imageAllocation = allocator->AllocateImage(
            imageInfo, 
            VMA_MEMORY_USAGE_GPU_ONLY, 
            image, 
            &allocatedSize
        );
    
        SEDX_CORE_ASSERT(imageAllocation != nullptr, "Failed to allocate texture memory");
        SEDX_CORE_INFO("Allocated texture ({}x{}): {} MB", 
                       width, height, allocatedSize / (1024 * 1024));
    
        // Upload texture data
        UploadTextureData(pixels, imageSize, image, width, height, allocator);
    
        // Generate mipmaps
        GenerateMipmaps(image, width, height, imageInfo.mipLevels);
    
        // Create texture object
        auto texture = CreateRef<Texture2D>();
        texture->SetImage(image, imageAllocation);
        texture->SetDimensions(width, height);
        texture->SetFormat(VK_FORMAT_R8G8B8A8_SRGB);
    
        // Cleanup
        stbi_image_free(pixels);
    
        return texture;
    }
  
private:
    void UploadTextureData(stbi_uc* pixels, 
                          VkDeviceSize imageSize, 
                          VkImage destImage,
                          int width, 
                          int height,
                          Ref<MemoryAllocator> allocator)
    {
        // Create staging buffer
        VkBufferCreateInfo stagingInfo{};
        stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingInfo.size = imageSize;
        stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
        VkBuffer stagingBuffer;
        VmaAllocation stagingAllocation = allocator->AllocateBuffer(
            stagingInfo, 
            VMA_MEMORY_USAGE_CPU_TO_GPU, 
            stagingBuffer
        );
    
        // Copy pixel data to staging buffer
        void* data = allocator->MapMemory<void>(stagingAllocation);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        MemoryAllocator::UnmapMemory(stagingAllocation);
    
        // Transition image layout and copy data
        TransitionImageLayout(destImage, VK_FORMAT_R8G8B8A8_SRGB, 
                             VK_IMAGE_LAYOUT_UNDEFINED, 
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    
        CopyBufferToImage(stagingBuffer, destImage, 
                         static_cast<uint32_t>(width), 
                         static_cast<uint32_t>(height));
    
        TransitionImageLayout(destImage, VK_FORMAT_R8G8B8A8_SRGB, 
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
        // Cleanup staging buffer
        allocator->DestroyBuffer(stagingBuffer, stagingAllocation);
    }
};
```

---

## Advanced Usage Examples

### 3. Batch Allocation for Scene Loading

```cpp
class SceneLoader : public Module
{
public:
    void LoadScene(const SceneData& sceneData)
    {
        SEDX_PROFILE_SCOPE("SceneLoader::LoadScene");
    
        // Create specialized allocator for scene loading
        m_Allocator = CreateRef<MemoryAllocator>("SceneLoader");
        m_Allocator->SetAllocationStrategy(MemoryAllocator::AllocationStrategy::SpeedOptimized);
    
        // Prepare buffer sizes for batch allocation
        std::vector<VkDeviceSize> bufferSizes;
        std::vector<MeshInfo> meshInfos;
    
        for (const auto& meshData : sceneData.meshes) {
            MeshInfo info;
            info.vertexBufferSize = sizeof(Vertex) * meshData.vertices.size();
            info.indexBufferSize = sizeof(uint32_t) * meshData.indices.size();
            info.materialUBOSize = sizeof(MaterialUBO);
        
            bufferSizes.push_back(info.vertexBufferSize);
            bufferSizes.push_back(info.indexBufferSize);
            bufferSizes.push_back(info.materialUBOSize);
        
            meshInfos.push_back(info);
        }
    
        SEDX_CORE_INFO("Loading scene with {} meshes, total buffers: {}", 
                       sceneData.meshes.size(), bufferSizes.size());
    
        // Allocate all buffers in batch for better performance
        auto allocations = m_Allocator->AllocateBufferBatch(
            bufferSizes,
            BufferUsage::Vertex | BufferUsage::Index | BufferUsage::Uniform,
            VMA_MEMORY_USAGE_GPU_ONLY
        );
    
        SEDX_CORE_ASSERT(allocations.size() == bufferSizes.size(), 
                         "Batch allocation failed");
    
        // Process allocated buffers
        size_t allocationIndex = 0;
        for (size_t meshIndex = 0; meshIndex < sceneData.meshes.size(); ++meshIndex) {
            const auto& meshData = sceneData.meshes[meshIndex];
        
            // Vertex buffer
            auto& vertexAlloc = allocations[allocationIndex++];
            UploadMeshVertices(meshData.vertices, vertexAlloc);
        
            // Index buffer
            auto& indexAlloc = allocations[allocationIndex++];
            UploadMeshIndices(meshData.indices, indexAlloc);
        
            // Material UBO
            auto& materialAlloc = allocations[allocationIndex++];
            UploadMaterialData(meshData.material, materialAlloc);
        
            // Store mesh info
            SceneMesh sceneMesh;
            sceneMesh.vertexBuffer = vertexAlloc.buffer;
            sceneMesh.vertexAllocation = vertexAlloc.allocation;
            sceneMesh.indexBuffer = indexAlloc.buffer;
            sceneMesh.indexAllocation = indexAlloc.allocation;
            sceneMesh.materialUBO = materialAlloc.buffer;
            sceneMesh.materialAllocation = materialAlloc.allocation;
            sceneMesh.indexCount = static_cast<uint32_t>(meshData.indices.size());
        
            m_SceneMeshes.push_back(sceneMesh);
        }
    
        // Print allocation statistics
        auto stats = m_Allocator->GetStats();
        SEDX_CORE_INFO("Scene loaded successfully:");
        SEDX_CORE_INFO("  Total memory used: {} MB", stats.usedBytes / (1024 * 1024));
        SEDX_CORE_INFO("  Active allocations: {}", stats.allocationCount);
        SEDX_CORE_INFO("  Fragmentation: {:.2f}%", stats.fragmentationRatio * 100.0f);
    }
  
    void OnDetach() override
    {
        SEDX_CORE_INFO("=== Cleaning up SceneLoader ===");
    
        // Print final statistics before cleanup
        if (m_Allocator) {
            m_Allocator->PrintDetailedStats();
        }
    
        // Clear scene meshes (RAII cleanup)
        m_SceneMeshes.clear();
        m_Allocator.Reset();
    }
  
private:
    struct MeshInfo
    {
        VkDeviceSize vertexBufferSize;
        VkDeviceSize indexBufferSize;
        VkDeviceSize materialUBOSize;
    };
  
    struct SceneMesh
    {
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VmaAllocation vertexAllocation = nullptr;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VmaAllocation indexAllocation = nullptr;
        VkBuffer materialUBO = VK_NULL_HANDLE;
        VmaAllocation materialAllocation = nullptr;
        uint32_t indexCount = 0;
    
        ~SceneMesh()
        {
            // Automatic cleanup through allocator
            if (vertexBuffer != VK_NULL_HANDLE) {
                // Note: Actual cleanup handled by allocator's destructor
            }
        }
    };
  
    void UploadMeshVertices(const std::vector<Vertex>& vertices, 
                           const MemoryAllocator::BatchBufferAllocation& allocation)
    {
        VkDeviceSize dataSize = sizeof(Vertex) * vertices.size();
        UploadDataToBuffer(vertices.data(), dataSize, allocation.buffer);
    }
  
    void UploadMeshIndices(const std::vector<uint32_t>& indices, 
                          const MemoryAllocator::BatchBufferAllocation& allocation)
    {
        VkDeviceSize dataSize = sizeof(uint32_t) * indices.size();
        UploadDataToBuffer(indices.data(), dataSize, allocation.buffer);
    }
  
    void UploadMaterialData(const MaterialData& material, 
                           const MemoryAllocator::BatchBufferAllocation& allocation)
    {
        MaterialUBO ubo = CreateMaterialUBO(material);
        UploadDataToBuffer(&ubo, sizeof(MaterialUBO), allocation.buffer);
    }
  
    std::vector<SceneMesh> m_SceneMeshes;
    Ref<MemoryAllocator> m_Allocator;
};
```

### 4. Dynamic Memory Management with Defragmentation

```cpp
class DynamicRenderer : public Module
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing DynamicRenderer ===");
    
        // Create memory allocator optimized for dynamic allocation
        m_Allocator = CreateRef<MemoryAllocator>("DynamicRenderer");
        m_Allocator->SetAllocationStrategy(MemoryAllocator::AllocationStrategy::MemoryOptimized);
        m_Allocator->SetMemoryUsageWarningThreshold(0.8f);
    
        // Set up periodic defragmentation
        m_DefragmentationTimer = 0.0f;
        m_DefragmentationInterval = 10.0f; // 10 seconds
    
        InitializeResourcePools();
    }
  
    void OnUpdate() override
    {
        SEDX_PROFILE_SCOPE("DynamicRenderer::OnUpdate");
    
        // Update defragmentation timer
        m_DefragmentationTimer += GetDeltaTime();
    
        // Perform periodic defragmentation
        if (m_DefragmentationTimer >= m_DefragmentationInterval) {
            PerformDefragmentation();
            m_DefragmentationTimer = 0.0f;
        }
    
        // Check memory budget
        auto budget = m_Allocator->GetMemoryBudget();
        if (budget.isOverBudget) {
            HandleMemoryPressure(budget);
        }
    
        // Process dynamic allocations/deallocations
        ProcessDynamicResources();
    }
  
    VkBuffer AllocateDynamicBuffer(VkDeviceSize size, VkBufferUsageFlags usage)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
        VkBuffer buffer;
        VmaAllocation allocation = m_Allocator->AllocateBuffer(
            bufferInfo, 
            VMA_MEMORY_USAGE_GPU_ONLY, 
            buffer
        );
    
        if (allocation == nullptr) {
            SEDX_CORE_ERROR("Failed to allocate dynamic buffer of size {} KB", size / 1024);
            return VK_NULL_HANDLE;
        }
    
        // Track allocation for defragmentation
        DynamicAllocation dynAlloc;
        dynAlloc.buffer = buffer;
        dynAlloc.allocation = allocation;
        dynAlloc.size = size;
        dynAlloc.lastUsedFrame = GetCurrentFrame();
        dynAlloc.usageFrequency = 1;
    
        m_DynamicAllocations[buffer] = dynAlloc;
    
        SEDX_CORE_TRACE("Allocated dynamic buffer: {} KB", size / 1024);
        return buffer;
    }
  
    void FreeDynamicBuffer(VkBuffer buffer)
    {
        auto it = m_DynamicAllocations.find(buffer);
        if (it == m_DynamicAllocations.end()) {
            SEDX_CORE_WARN("Attempted to free unknown dynamic buffer");
            return;
        }
    
        m_Allocator->DestroyBuffer(buffer, it->second.allocation);
        m_DynamicAllocations.erase(it);
    
        SEDX_CORE_TRACE("Freed dynamic buffer");
    }
  
private:
    struct DynamicAllocation
    {
        VkBuffer buffer;
        VmaAllocation allocation;
        VkDeviceSize size;
        uint64_t lastUsedFrame;
        uint32_t usageFrequency;
    };
  
    void PerformDefragmentation()
    {
        SEDX_PROFILE_SCOPE("DynamicRenderer::PerformDefragmentation");
    
        SEDX_CORE_INFO("Starting memory defragmentation");
    
        // Begin defragmentation process
        m_Allocator->BeginDefragmentation(VMA_DEFRAGMENTATION_FLAG_ALGORITHM_BALANCED_BIT);
    
        // Mark candidates for defragmentation based on usage patterns
        uint64_t currentFrame = GetCurrentFrame();
        uint32_t candidateCount = 0;
    
        for (const auto& [buffer, dynAlloc] : m_DynamicAllocations) {
            bool shouldDefragment = false;
        
            // Mark unused allocations for defragmentation
            if (currentFrame - dynAlloc.lastUsedFrame > 300) { // Unused for 5 seconds at 60fps
                shouldDefragment = true;
            }
        
            // Mark frequently used allocations for optimization
            if (dynAlloc.usageFrequency > 100) {
                shouldDefragment = true;
            }
        
            // Mark large allocations for defragmentation
            if (dynAlloc.size > 1024 * 1024) { // > 1MB
                shouldDefragment = true;
            }
        
            if (shouldDefragment) {
                m_Allocator->MarkForDefragmentation(dynAlloc.allocation);
                candidateCount++;
            }
        }
    
        SEDX_CORE_INFO("Marked {} allocations for defragmentation", candidateCount);
    
        // Execute defragmentation
        m_Allocator->EndDefragmentation();
    
        // Update statistics
        auto stats = m_Allocator->GetStats();
        SEDX_CORE_INFO("Post-defragmentation stats:");
        SEDX_CORE_INFO("  Fragmentation: {:.2f}%", stats.fragmentationRatio * 100.0f);
        SEDX_CORE_INFO("  Used memory: {} MB", stats.usedBytes / (1024 * 1024));
    }
  
    void HandleMemoryPressure(const MemoryAllocator::MemoryBudget& budget)
    {
        SEDX_CORE_WARN("Memory pressure detected: {:.1f}% usage", 
                       budget.usagePercentage * 100.0f);
    
        // Find and free unused allocations
        uint64_t currentFrame = GetCurrentFrame();
        std::vector<VkBuffer> buffersToFree;
    
        for (const auto& [buffer, dynAlloc] : m_DynamicAllocations) {
            // Free allocations unused for more than 10 seconds
            if (currentFrame - dynAlloc.lastUsedFrame > 600) {
                buffersToFree.push_back(buffer);
            }
        }
    
        for (VkBuffer buffer : buffersToFree) {
            FreeDynamicBuffer(buffer);
        }
    
        if (!buffersToFree.empty()) {
            SEDX_CORE_INFO("Freed {} unused dynamic buffers due to memory pressure", 
                           buffersToFree.size());
        }
    
        // Trigger immediate defragmentation
        PerformDefragmentation();
    }
  
    void ProcessDynamicResources()
    {
        // Update usage tracking for dynamic allocations
        // This would be called when resources are actually used in rendering
        uint64_t currentFrame = GetCurrentFrame();
    
        for (auto& [buffer, dynAlloc] : m_DynamicAllocations) {
            // Example: Update usage based on rendering commands
            if (IsBufferUsedThisFrame(buffer)) {
                dynAlloc.lastUsedFrame = currentFrame;
                dynAlloc.usageFrequency++;
            }
        }
    }
  
    std::unordered_map<VkBuffer, DynamicAllocation> m_DynamicAllocations;
    Ref<MemoryAllocator> m_Allocator;
    float m_DefragmentationTimer;
    float m_DefragmentationInterval;
};
```

### 5. Memory Pool Optimization

```cpp
class OptimizedBufferManager
{
public:
    OptimizedBufferManager()
    {
        // Create allocator with custom configuration
        m_Allocator = CreateRef<MemoryAllocator>("BufferManager");
        m_Allocator->SetAllocationStrategy(MemoryAllocator::AllocationStrategy::SpeedOptimized);
    
        // Set custom buffer alignment for optimal cache performance
        m_Allocator->SetBufferAlignment(256); // 256-byte alignment
    
        // Pre-warm memory pools
        PrewarmMemoryPools();
    }
  
    // Optimized allocation for common buffer types
    VkBuffer AllocateVertexBuffer(VkDeviceSize size)
    {
        return AllocateFromPool(size, BufferType::Vertex);
    }
  
    VkBuffer AllocateIndexBuffer(VkDeviceSize size)
    {
        return AllocateFromPool(size, BufferType::Index);
    }
  
    VkBuffer AllocateUniformBuffer(VkDeviceSize size)
    {
        return AllocateFromPool(size, BufferType::Uniform);
    }
  
    VkBuffer AllocateStorageBuffer(VkDeviceSize size)
    {
        return AllocateFromPool(size, BufferType::Storage);
    }
  
    void PrintPoolStatistics()
    {
        SEDX_CORE_INFO("=== Buffer Pool Statistics ===");
    
        for (const auto& [type, pool] : m_BufferPools) {
            const char* typeName = GetBufferTypeName(type);
            SEDX_CORE_INFO("{} Pool:", typeName);
            SEDX_CORE_INFO("  Total allocations: {}", pool.totalAllocations);
            SEDX_CORE_INFO("  Active buffers: {}", pool.activeBuffers.size());
            SEDX_CORE_INFO("  Free buffers: {}", pool.freeBuffers.size());
            SEDX_CORE_INFO("  Total size: {} MB", pool.totalSize / (1024 * 1024));
        }
    
        m_Allocator->PrintDetailedStats();
    }
  
private:
    enum class BufferType
    {
        Vertex,
        Index,
        Uniform,
        Storage
    };
  
    struct BufferPool
    {
        struct PooledBuffer
        {
            VkBuffer buffer;
            VmaAllocation allocation;
            VkDeviceSize size;
            bool inUse;
        };
    
        std::vector<PooledBuffer> activeBuffers;
        std::vector<PooledBuffer> freeBuffers;
        uint32_t totalAllocations = 0;
        VkDeviceSize totalSize = 0;
    };
  
    VkBuffer AllocateFromPool(VkDeviceSize size, BufferType type)
    {
        auto& pool = m_BufferPools[type];
    
        // Try to find a suitable free buffer
        for (auto it = pool.freeBuffers.begin(); it != pool.freeBuffers.end(); ++it) {
            if (it->size >= size && it->size <= size * 2) { // Allow up to 2x size for reuse
                BufferPool::PooledBuffer buffer = *it;
                pool.freeBuffers.erase(it);
                pool.activeBuffers.push_back(buffer);
            
                SEDX_CORE_TRACE("Reused {} buffer: {} KB", 
                               GetBufferTypeName(type), size / 1024);
                return buffer.buffer;
            }
        }
    
        // Allocate new buffer
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = GetBufferUsageFlags(type);
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
        VkBuffer buffer;
        VmaAllocation allocation = m_Allocator->AllocateBuffer(
            bufferInfo, 
            VMA_MEMORY_USAGE_GPU_ONLY, 
            buffer
        );
    
        if (allocation == nullptr) {
            SEDX_CORE_ERROR("Failed to allocate {} buffer of size {} KB", 
                            GetBufferTypeName(type), size / 1024);
            return VK_NULL_HANDLE;
        }
    
        // Add to active buffers
        BufferPool::PooledBuffer pooledBuffer;
        pooledBuffer.buffer = buffer;
        pooledBuffer.allocation = allocation;
        pooledBuffer.size = size;
        pooledBuffer.inUse = true;
    
        pool.activeBuffers.push_back(pooledBuffer);
        pool.totalAllocations++;
        pool.totalSize += size;
    
        SEDX_CORE_TRACE("Allocated new {} buffer: {} KB", 
                       GetBufferTypeName(type), size / 1024);
    
        return buffer;
    }
  
    void PrewarmMemoryPools()
    {
        SEDX_CORE_INFO("Pre-warming memory pools...");
    
        // Pre-allocate common buffer sizes
        struct PrewarmInfo
        {
            BufferType type;
            VkDeviceSize size;
            uint32_t count;
        };
    
        std::vector<PrewarmInfo> prewarmSizes = {
            {BufferType::Vertex, 64 * 1024, 5},      // 64KB vertex buffers
            {BufferType::Vertex, 256 * 1024, 3},     // 256KB vertex buffers
            {BufferType::Index, 32 * 1024, 5},       // 32KB index buffers
            {BufferType::Index, 128 * 1024, 3},      // 128KB index buffers
            {BufferType::Uniform, 256, 10},          // 256B uniform buffers
            {BufferType::Uniform, 1024, 5},          // 1KB uniform buffers
            {BufferType::Storage, 4 * 1024, 3},      // 4KB storage buffers
        };
    
        for (const auto& info : prewarmSizes) {
            for (uint32_t i = 0; i < info.count; ++i) {
                VkBuffer buffer = AllocateFromPool(info.size, info.type);
                if (buffer != VK_NULL_HANDLE) {
                    // Immediately mark as free for reuse
                    ReturnBufferToPool(buffer, info.type);
                }
            }
        }
    
        SEDX_CORE_INFO("Memory pool pre-warming complete");
    }
  
    void ReturnBufferToPool(VkBuffer buffer, BufferType type)
    {
        auto& pool = m_BufferPools[type];
    
        // Find buffer in active list
        for (auto it = pool.activeBuffers.begin(); it != pool.activeBuffers.end(); ++it) {
            if (it->buffer == buffer) {
                it->inUse = false;
                pool.freeBuffers.push_back(*it);
                pool.activeBuffers.erase(it);
                return;
            }
        }
    
        SEDX_CORE_WARN("Attempted to return unknown buffer to pool");
    }
  
    VkBufferUsageFlags GetBufferUsageFlags(BufferType type)
    {
        switch (type) {
        case BufferType::Vertex:
            return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        case BufferType::Index:
            return VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        case BufferType::Uniform:
            return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        case BufferType::Storage:
            return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        default:
            return 0;
        }
    }
  
    const char* GetBufferTypeName(BufferType type)
    {
        switch (type) {
        case BufferType::Vertex: return "Vertex";
        case BufferType::Index: return "Index";
        case BufferType::Uniform: return "Uniform";
        case BufferType::Storage: return "Storage";
        default: return "Unknown";
        }
    }
  
    std::unordered_map<BufferType, BufferPool> m_BufferPools;
    Ref<MemoryAllocator> m_Allocator;
};
```

---

## Integration Examples

### 6. Terrain System with LOD

```cpp
class TerrainRenderer : public Module
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing TerrainRenderer ===");
    
        // Create memory allocator for terrain system
        m_Allocator = CreateRef<MemoryAllocator>("TerrainRenderer");
        m_Allocator->SetAllocationStrategy(MemoryAllocator::AllocationStrategy::MemoryOptimized);
    
        // Set up large buffer support for terrain data
        auto device = RenderContext::GetCurrentDevice();
        if (MemoryAllocator::SetCustomBufferSize(128 * 1024 * 1024, *device)) {
            SEDX_CORE_INFO("Enabled large buffer support (128MB) for terrain");
        }
    
        GenerateTerrainData();
    }
  
    void GenerateTerrainData()
    {
        SEDX_PROFILE_SCOPE("TerrainRenderer::GenerateTerrainData");
    
        // Generate multiple LOD levels
        for (int lod = 0; lod < MAX_LOD_LEVELS; ++lod) {
            int resolution = BASE_RESOLUTION >> lod;
        
            // Generate heightmap data
            std::vector<float> heightData = GenerateHeightmap(resolution);
        
            // Create terrain mesh
            auto meshData = GenerateTerrainMesh(heightData, resolution, lod);
        
            // Allocate GPU buffers
            AllocateTerrainLOD(meshData, lod);
        
            SEDX_CORE_INFO("Generated terrain LOD {}: {}x{} vertices", 
                           lod, resolution, resolution);
        }
    
        // Print memory usage
        auto stats = m_Allocator->GetStats();
        SEDX_CORE_INFO("Terrain memory usage: {} MB", stats.usedBytes / (1024 * 1024));
    }
  
    void UpdateLOD(const glm::vec3& cameraPosition)
    {
        SEDX_PROFILE_SCOPE("TerrainRenderer::UpdateLOD");
    
        // Determine which LOD levels are needed based on camera distance
        std::vector<int> requiredLODs;
        for (int lod = 0; lod < MAX_LOD_LEVELS; ++lod) {
            float distance = CalculateDistanceToLOD(cameraPosition, lod);
            if (distance < LOD_DISTANCES[lod]) {
                requiredLODs.push_back(lod);
            }
        }
    
        // Mark unused LODs for defragmentation to free memory
        m_Allocator->BeginDefragmentation();
        for (int lod = 0; lod < MAX_LOD_LEVELS; ++lod) {
            if (std::find(requiredLODs.begin(), requiredLODs.end(), lod) == requiredLODs.end()) {
                if (m_TerrainLODs[lod].vertexAllocation) {
                    m_Allocator->MarkForDefragmentation(m_TerrainLODs[lod].vertexAllocation);
                }
                if (m_TerrainLODs[lod].indexAllocation) {
                    m_Allocator->MarkForDefragmentation(m_TerrainLODs[lod].indexAllocation);
                }
            }
        }
        m_Allocator->EndDefragmentation();
    }
  
private:
    struct TerrainLOD
    {
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VmaAllocation vertexAllocation = nullptr;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VmaAllocation indexAllocation = nullptr;
        uint32_t indexCount = 0;
        int resolution = 0;
    
        ~TerrainLOD()
        {
            // Cleanup handled by allocator
        }
    };
  
    void AllocateTerrainLOD(const TerrainMeshData& meshData, int lod)
    {
        // Calculate buffer sizes
        VkDeviceSize vertexSize = sizeof(TerrainVertex) * meshData.vertices.size();
        VkDeviceSize indexSize = sizeof(uint32_t) * meshData.indices.size();
    
        // Allocate vertex buffer
        VkBufferCreateInfo vertexInfo{};
        vertexInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertexInfo.size = vertexSize;
        vertexInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        vertexInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
        VkBuffer vertexBuffer;
        VmaAllocation vertexAllocation = m_Allocator->AllocateBuffer(
            vertexInfo, VMA_MEMORY_USAGE_GPU_ONLY, vertexBuffer);
    
        // Allocate index buffer
        VkBufferCreateInfo indexInfo{};
        indexInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        indexInfo.size = indexSize;
        indexInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        indexInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
        VkBuffer indexBuffer;
        VmaAllocation indexAllocation = m_Allocator->AllocateBuffer(
            indexInfo, VMA_MEMORY_USAGE_GPU_ONLY, indexBuffer);
    
        // Upload data
        UploadTerrainData(meshData, vertexBuffer, indexBuffer);
    
        // Store LOD data
        m_TerrainLODs[lod].vertexBuffer = vertexBuffer;
        m_TerrainLODs[lod].vertexAllocation = vertexAllocation;
        m_TerrainLODs[lod].indexBuffer = indexBuffer;
        m_TerrainLODs[lod].indexAllocation = indexAllocation;
        m_TerrainLODs[lod].indexCount = static_cast<uint32_t>(meshData.indices.size());
        m_TerrainLODs[lod].resolution = meshData.resolution;
    }
  
    static constexpr int MAX_LOD_LEVELS = 5;
    static constexpr int BASE_RESOLUTION = 1024;
    static constexpr float LOD_DISTANCES[MAX_LOD_LEVELS] = {100.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f};
  
    std::array<TerrainLOD, MAX_LOD_LEVELS> m_TerrainLODs;
    Ref<MemoryAllocator> m_Allocator;
};
```

### 7. Memory Monitoring and Debug Tools

```cpp
class MemoryProfiler : public Module
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing MemoryProfiler ===");
    
        // Create allocator for profiling tools
        m_Allocator = CreateRef<MemoryAllocator>("MemoryProfiler");
    
        // Set up profiling callbacks
        SetupProfilingHooks();
    
        // Initialize monitoring
        m_MonitoringEnabled = true;
        m_StatisticsUpdateInterval = 1.0f; // Update every second
        m_StatisticsTimer = 0.0f;
    }
  
    void OnUpdate() override
    {
        if (!m_MonitoringEnabled) return;
    
        SEDX_PROFILE_SCOPE("MemoryProfiler::OnUpdate");
    
        m_StatisticsTimer += GetDeltaTime();
    
        if (m_StatisticsTimer >= m_StatisticsUpdateInterval) {
            CollectStatistics();
            m_StatisticsTimer = 0.0f;
        }
    
        // Check for memory warnings
        CheckMemoryHealth();
    }
  
    void OnUIRender() override
    {
        if (m_ShowMemoryWindow) {
            RenderMemoryWindow();
        }
    }
  
    void OnEvent() override
    {
        // Handle keyboard shortcuts for memory profiling
        if (Input::IsKeyPressed(KeyCode::F10)) {
            m_ShowMemoryWindow = !m_ShowMemoryWindow;
        }
    
        if (Input::IsKeyPressed(KeyCode::F11)) {
            TriggerMemorySnapshot();
        }
    
        if (Input::IsKeyPressed(KeyCode::F12)) {
            TriggerDefragmentation();
        }
    }
  
private:
    struct MemorySnapshot
    {
        std::chrono::steady_clock::time_point timestamp;
        MemoryAllocator::AllocationStats stats;
        MemoryAllocator::MemoryBudget budget;
        std::string description;
    };
  
    void CollectStatistics()
    {
        // Collect stats from all known allocators
        auto globalStats = MemoryAllocator::GetStats();
        auto globalBudget = MemoryAllocator::GetMemoryBudget();
    
        // Store historical data
        m_StatisticsHistory.push_back({
            std::chrono::steady_clock::now(),
            globalStats,
            globalBudget,
            "Periodic Update"
        });
    
        // Keep only recent history (last 10 minutes)
        const auto cutoffTime = std::chrono::steady_clock::now() - std::chrono::minutes(10);
        m_StatisticsHistory.erase(
            std::remove_if(m_StatisticsHistory.begin(), m_StatisticsHistory.end(),
                [cutoffTime](const MemorySnapshot& snapshot) {
                    return snapshot.timestamp < cutoffTime;
                }),
            m_StatisticsHistory.end()
        );
    }
  
    void CheckMemoryHealth()
    {
        auto budget = MemoryAllocator::GetMemoryBudget();
    
        // Check for memory pressure
        if (budget.usagePercentage > 0.9f) {
            if (!m_HighMemoryUsageWarned) {
                SEDX_CORE_WARN_TAG("MemoryProfiler", 
                                   "High memory usage detected: {:.1f}%", 
                                   budget.usagePercentage * 100.0f);
                m_HighMemoryUsageWarned = true;
            }
        } else {
            m_HighMemoryUsageWarned = false;
        }
    
        // Check fragmentation
        auto stats = MemoryAllocator::GetStats();
        if (stats.fragmentationRatio > 0.4f) {
            if (!m_HighFragmentationWarned) {
                SEDX_CORE_WARN_TAG("MemoryProfiler", 
                                   "High memory fragmentation: {:.1f}%", 
                                   stats.fragmentationRatio * 100.0f);
                m_HighFragmentationWarned = true;
            }
        } else {
            m_HighFragmentationWarned = false;
        }
    }
  
    void RenderMemoryWindow()
    {
        ImGui::Begin("Memory Profiler", &m_ShowMemoryWindow);
    
        // Current statistics
        auto currentStats = MemoryAllocator::GetStats();
        auto currentBudget = MemoryAllocator::GetMemoryBudget();
    
        ImGui::Text("Current Memory Usage");
        ImGui::Separator();
    
        ImGui::Text("Total Allocated: %.2f MB", 
                   static_cast<float>(currentStats.totalBytes) / (1024.0f * 1024.0f));
        ImGui::Text("Used Memory: %.2f MB", 
                   static_cast<float>(currentStats.usedBytes) / (1024.0f * 1024.0f));
        ImGui::Text("Active Allocations: %llu", currentStats.allocationCount);
        ImGui::Text("Fragmentation: %.2f%%", currentStats.fragmentationRatio * 100.0f);
    
        ImGui::Spacing();
        ImGui::Text("Memory Budget");
        ImGui::Separator();
    
        ImGui::Text("Budget: %.2f MB", 
                   static_cast<float>(currentBudget.totalBytes) / (1024.0f * 1024.0f));
        ImGui::Text("Used: %.2f MB (%.1f%%)", 
                   static_cast<float>(currentBudget.usedBytes) / (1024.0f * 1024.0f),
                   currentBudget.usagePercentage * 100.0f);
    
        // Progress bar for memory usage
        ImGui::ProgressBar(currentBudget.usagePercentage, 
                          ImVec2(-1, 0), 
                          fmt::format("{:.1f}%", currentBudget.usagePercentage * 100.0f).c_str());
    
        ImGui::Spacing();
    
        // Control buttons
        if (ImGui::Button("Take Snapshot")) {
            TriggerMemorySnapshot();
        }
        ImGui::SameLine();
        if (ImGui::Button("Defragment")) {
            TriggerDefragmentation();
        }
        ImGui::SameLine();
        if (ImGui::Button("Print Detailed Stats")) {
            m_Allocator->PrintDetailedStats();
        }
    
        // Historical graph
        if (ImGui::CollapsingHeader("Memory Usage History")) {
            RenderMemoryGraph();
        }
    
        // Snapshot list
        if (ImGui::CollapsingHeader("Memory Snapshots")) {
            RenderSnapshotList();
        }
    
        ImGui::End();
    }
  
    void RenderMemoryGraph()
    {
        if (m_StatisticsHistory.empty()) return;
    
        // Prepare data for plotting
        std::vector<float> usageData;
        std::vector<float> fragmentationData;
    
        for (const auto& snapshot : m_StatisticsHistory) {
            usageData.push_back(snapshot.budget.usagePercentage);
            fragmentationData.push_back(snapshot.stats.fragmentationRatio);
        }
    
        // Plot memory usage over time
        ImGui::PlotLines("Memory Usage %", usageData.data(), 
                        static_cast<int>(usageData.size()), 
                        0, nullptr, 0.0f, 1.0f, ImVec2(0, 80));
    
        // Plot fragmentation over time
        ImGui::PlotLines("Fragmentation %", fragmentationData.data(), 
                        static_cast<int>(fragmentationData.size()), 
                        0, nullptr, 0.0f, 1.0f, ImVec2(0, 80));
    }
  
    void RenderSnapshotList()
    {
        for (size_t i = 0; i < m_Snapshots.size(); ++i) {
            const auto& snapshot = m_Snapshots[i];
        
            if (ImGui::TreeNode(fmt::format("Snapshot {} - {}", i, snapshot.description).c_str())) {
                ImGui::Text("Timestamp: %s", FormatTimestamp(snapshot.timestamp).c_str());
                ImGui::Text("Used Memory: %.2f MB", 
                           static_cast<float>(snapshot.stats.usedBytes) / (1024.0f * 1024.0f));
                ImGui::Text("Allocations: %llu", snapshot.stats.allocationCount);
                ImGui::Text("Fragmentation: %.2f%%", snapshot.stats.fragmentationRatio * 100.0f);
                ImGui::Text("Budget Usage: %.1f%%", snapshot.budget.usagePercentage * 100.0f);
            
                ImGui::TreePop();
            }
        }
    }
  
    void TriggerMemorySnapshot()
    {
        MemorySnapshot snapshot;
        snapshot.timestamp = std::chrono::steady_clock::now();
        snapshot.stats = MemoryAllocator::GetStats();
        snapshot.budget = MemoryAllocator::GetMemoryBudget();
        snapshot.description = fmt::format("Manual Snapshot {}", m_Snapshots.size() + 1);
    
        m_Snapshots.push_back(snapshot);
    
        SEDX_CORE_INFO_TAG("MemoryProfiler", "Memory snapshot taken: {} MB used, {:.1f}% fragmentation",
                           snapshot.stats.usedBytes / (1024 * 1024),
                           snapshot.stats.fragmentationRatio * 100.0f);
    }
  
    void TriggerDefragmentation()
    {
        SEDX_CORE_INFO_TAG("MemoryProfiler", "Manual defragmentation triggered");
    
        // Take snapshot before defragmentation
        auto beforeSnapshot = CreateSnapshot("Before Defragmentation");
    
        // Trigger defragmentation on all allocators
        m_Allocator->BeginDefragmentation(VMA_DEFRAGMENTATION_FLAG_ALGORITHM_BALANCED_BIT);
        m_Allocator->EndDefragmentation();
    
        // Take snapshot after defragmentation
        auto afterSnapshot = CreateSnapshot("After Defragmentation");
    
        // Log results
        float fragmentationImprovement = 
            (beforeSnapshot.stats.fragmentationRatio - afterSnapshot.stats.fragmentationRatio) * 100.0f;
    
        SEDX_CORE_INFO_TAG("MemoryProfiler", 
                           "Defragmentation complete: {:.2f}% fragmentation improvement",
                           fragmentationImprovement);
    }
  
    MemorySnapshot CreateSnapshot(const std::string& description)
    {
        MemorySnapshot snapshot;
        snapshot.timestamp = std::chrono::steady_clock::now();
        snapshot.stats = MemoryAllocator::GetStats();
        snapshot.budget = MemoryAllocator::GetMemoryBudget();
        snapshot.description = description;
        return snapshot;
    }
  
    std::vector<MemorySnapshot> m_StatisticsHistory;
    std::vector<MemorySnapshot> m_Snapshots;
    Ref<MemoryAllocator> m_Allocator;
    bool m_MonitoringEnabled = false;
    bool m_ShowMemoryWindow = false;
    float m_StatisticsUpdateInterval;
    float m_StatisticsTimer;
    bool m_HighMemoryUsageWarned = false;
    bool m_HighFragmentationWarned = false;
};
```

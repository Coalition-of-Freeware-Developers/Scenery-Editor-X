# Scenery Editor X - Smart Pointer System Documentation

## Table of Contents
1. [Overview](#overview)
2. [Core Components](#core-components)
3. [Usage Guidelines for GitHub Copilot](#usage-guidelines-for-github-copilot)
4. [Design Patterns and Best Practices](#design-patterns-and-best-practices)
5. [Common Use Cases](#common-use-cases)
6. [Performance Considerations](#performance-considerations)
7. [Thread Safety](#thread-safety)
8. [Error Handling](#error-handling)
9. [Interoperability](#interoperability)
10. [Migration Guidelines](#migration-guidelines)

## Overview

The Scenery Editor X smart pointer system (`pointers.h`) provides a comprehensive reference counting and weak reference implementation optimized for 3D rendering applications. This system is designed to replace standard library smart pointers in scenarios where fine-grained control over memory management is required.

### Key Features
- **Thread-safe reference counting** using atomic operations
- **Weak references** for breaking circular dependencies
- **Type-safe conversions** with compile-time checks
- **Interoperability** with `std::shared_ptr` and `std::unique_ptr`
- **Zero-overhead** unique ownership via `Scope<T>`
- **Vulkan-optimized** for high-performance rendering workloads

### When to Use This System
- ✅ Vulkan resource management (textures, buffers, pipelines)
- ✅ Scene graph hierarchies with parent-child relationships
- ✅ Asset caching systems
- ✅ Render command dependencies
- ✅ Multi-threaded rendering scenarios
- ❌ Simple RAII for stack objects (use regular stack allocation)
- ❌ External library interfaces (prefer standard smart pointers)

## Core Components

### 1. RefCounted Base Class

All objects managed by `Ref<T>` must inherit from `RefCounted`. This class provides atomic reference counting functionality.

```cpp
class MyRenderObject : public RefCounted {
public:
    MyRenderObject(int id) : m_Id(id) {
        // Constructor logic
    }
    
    int GetId() const { return m_Id; }
    
private:
    int m_Id;
};
```

**Key Points for GitHub Copilot:**
- Always inherit publicly from `RefCounted` for shared objects
- The destructor is automatically virtual
- Reference count starts at 0 and is managed automatically
- Copy/move operations don't affect the reference count

### 2. Ref<T> - Strong References

Primary smart pointer for shared ownership with automatic memory management.

#### Creation Patterns
```cpp
// PREFERRED: Use CreateRef for new objects
Ref<Texture> texture = CreateRef<Texture>("diffuse.png", 512, 512);

// Constructor with parameters
Ref<Material> material = CreateRef<Material>(MaterialType::PBR);

// From raw pointer (use sparingly)
auto* rawPtr = new MyObject();
Ref<MyObject> obj(rawPtr);

// Copy construction (shares ownership)
Ref<Texture> sharedTexture = texture;

// Move construction (transfers ownership)
Ref<Texture> movedTexture = std::move(texture);
```

#### Access Patterns
```cpp
// Check validity before use
if (texture) {
    // Safe to access
    auto width = texture->GetWidth();
    auto& texData = *texture;
}

// Alternative validity check
if (texture.IsValid()) {
    texture->Bind();
}

// Get raw pointer (use carefully)
Texture* rawPtr = texture.Get();
```

#### Type Conversions
```cpp
// Static cast (compile-time checked)
Ref<BaseTexture> base = texture.As<BaseTexture>();

// Dynamic cast (runtime checked, can return null)
Ref<SpecialTexture> special = base.DynamicCast<SpecialTexture>();
if (special) {
    special->DoSpecialOperation();
}
```

### 3. WeakRef<T> - Weak References

Non-owning observation of `Ref<T>` managed objects. Essential for breaking circular dependencies.

#### Creation and Usage
```cpp
// Create from strong reference
Ref<SceneNode> node = CreateRef<SceneNode>();
WeakRef<SceneNode> weakNode = node;

// Safe access pattern
if (!weakNode.Expired()) {
    if (auto strongRef = weakNode.Lock()) {
        strongRef->Update(); // Safe to use
    }
}

// Alternative access pattern
auto locked = weakNode.Lock();
if (locked) {
    locked->Process();
}
```

#### Observer Pattern Implementation
```cpp
class TextureManager {
private:
    std::unordered_map<std::string, WeakRef<Texture>> m_TextureCache;
    
public:
    void RegisterTexture(const std::string& name, Ref<Texture> texture) {
        m_TextureCache[name] = texture; // Store weak reference
    }
    
    Ref<Texture> GetTexture(const std::string& name) {
        auto it = m_TextureCache.find(name);
        if (it != m_TextureCache.end()) {
            if (auto texture = it->second.Lock()) {
                return texture; // Still alive
            }
            // Clean up expired entry
            m_TextureCache.erase(it);
        }
        return nullptr;
    }
    
    void CleanExpired() {
        for (auto it = m_TextureCache.begin(); it != m_TextureCache.end();) {
            if (it->second.Expired()) {
                it = m_TextureCache.erase(it);
            } else {
                ++it;
            }
        }
    }
};
```

### 4. Scope<T> - Unique Ownership

Alias for `std::unique_ptr` providing exclusive ownership semantics.

```cpp
// Create unique objects
Scope<CommandBuffer> cmdBuffer = CreateScope<CommandBuffer>(device);

// Transfer ownership
Scope<CommandBuffer> transferred = std::move(cmdBuffer);
// cmdBuffer is now null

// Perfect for temporary/local resources
void RenderFrame() {
    auto frameBuffer = CreateScope<FrameBuffer>(width, height);
    frameBuffer->Clear();
    // Automatically destroyed at end of scope
}
```

## Usage Guidelines for GitHub Copilot

### 1. Smart Pointer Selection

**When generating code, GitHub Copilot should:**

```cpp
// ✅ PREFERRED - Use project smart pointers
Ref<Texture> texture = CreateRef<Texture>("path.png");
WeakRef<Material> material = strongMaterial;
Scope<Buffer> buffer = CreateScope<Buffer>(1024);

// ❌ AVOID - Standard library (unless interfacing with external APIs)
auto texture = std::make_shared<Texture>("path.png");
std::weak_ptr<Material> material = strongMaterial;
```

### 2. Class Design Patterns

**For shared objects:**
```cpp
class RenderTarget : public RefCounted {
public:
    static Ref<RenderTarget> Create(uint32_t width, uint32_t height) {
        return CreateRef<RenderTarget>(width, height);
    }
    
private:
    RenderTarget(uint32_t width, uint32_t height) 
        : m_Width(width), m_Height(height) {}
    
    uint32_t m_Width, m_Height;
};
```

**For unique objects:**
```cpp
class CommandList {
public:
    static Scope<CommandList> Create() {
        return CreateScope<CommandList>();
    }
    
private:
    CommandList() = default;
};
```

### 3. Relationship Management

**Parent-Child Relationships (avoiding cycles):**
```cpp
class SceneNode : public RefCounted {
public:
    void AddChild(Ref<SceneNode> child) {
        m_Children.push_back(child);           // Strong reference
        child->m_Parent = WeakRef<SceneNode>(*this); // Weak reference breaks cycle
    }
    
    void RemoveChild(Ref<SceneNode> child) {
        auto it = std::find(m_Children.begin(), m_Children.end(), child);
        if (it != m_Children.end()) {
            (*it)->m_Parent = WeakRef<SceneNode>(); // Clear parent
            m_Children.erase(it);
        }
    }
    
    Ref<SceneNode> GetParent() const {
        return m_Parent.Lock(); // Safe access to parent
    }
    
private:
    std::vector<Ref<SceneNode>> m_Children;  // Own children
    WeakRef<SceneNode> m_Parent;             // Observe parent
};
```

### 4. Resource Ownership

**Clear ownership semantics:**
```cpp
class Mesh : public RefCounted {
public:
    // Mesh owns its material
    void SetMaterial(Ref<Material> material) {
        m_Material = std::move(material);
    }
    
    // Mesh observes its parent (parent owns mesh)
    void SetParent(Ref<SceneNode> parent) {
        m_Parent = parent;
    }
    
    // Mesh shares vertex buffer with other meshes
    void SetVertexBuffer(Ref<VertexBuffer> buffer) {
        m_VertexBuffer = buffer;
    }
    
private:
    Ref<Material> m_Material;        // Owned resource
    WeakRef<SceneNode> m_Parent;     // Observer relationship
    Ref<VertexBuffer> m_VertexBuffer; // Shared resource
};
```

### 5. Function Signatures

**Parameter passing guidelines:**
```cpp
// For input parameters that won't be stored
void ProcessTexture(const Ref<Texture>& texture);

// For parameters that will be stored/moved
void SetTexture(Ref<Texture> texture) {
    m_Texture = std::move(texture);
}

// For optional parameters
void SetOptionalMaterial(Ref<Material> material = nullptr);

// For output parameters
Ref<Texture> CreateTexture(const std::string& path);

// For weak observation
void RegisterObserver(WeakRef<Observer> observer);
```

## Design Patterns and Best Practices

### 1. Factory Pattern with Smart Pointers

```cpp
class TextureFactory {
public:
    static Ref<Texture> CreateTexture2D(const std::string& path) {
        auto texture = CreateRef<Texture2D>();
        if (!texture->LoadFromFile(path)) {
            return nullptr; // Failed to create
        }
        return texture;
    }
    
    static Ref<RenderTexture> CreateRenderTexture(uint32_t width, uint32_t height) {
        return CreateRef<RenderTexture>(width, height);
    }
};

// Usage
auto diffuse = TextureFactory::CreateTexture2D("textures/diffuse.png");
auto renderTarget = TextureFactory::CreateRenderTexture(1920, 1080);
```

### 2. Resource Manager Pattern

```cpp
class ResourceManager {
private:
    std::unordered_map<std::string, WeakRef<Texture>> m_TextureCache;
    std::unordered_map<std::string, WeakRef<Mesh>> m_MeshCache;
    mutable std::mutex m_CacheMutex;
    
public:
    Ref<Texture> LoadTexture(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_CacheMutex);
        
        // Check cache first
        auto it = m_TextureCache.find(path);
        if (it != m_TextureCache.end()) {
            if (auto cached = it->second.Lock()) {
                return cached; // Return cached texture
            }
            // Remove expired entry
            m_TextureCache.erase(it);
        }
        
        // Load new texture
        auto texture = CreateRef<Texture>();
        if (texture->LoadFromFile(path)) {
            m_TextureCache[path] = texture; // Cache weak reference
            return texture;
        }
        
        return nullptr; // Failed to load
    }
    
    void CleanCache() {
        std::lock_guard<std::mutex> lock(m_CacheMutex);
        
        // Clean texture cache
        for (auto it = m_TextureCache.begin(); it != m_TextureCache.end();) {
            if (it->second.Expired()) {
                it = m_TextureCache.erase(it);
            } else {
                ++it;
            }
        }
        
        // Clean mesh cache
        for (auto it = m_MeshCache.begin(); it != m_MeshCache.end();) {
            if (it->second.Expired()) {
                it = m_MeshCache.erase(it);
            } else {
                ++it;
            }
        }
    }
};
```

### 3. Command Pattern with Smart Pointers

```cpp
class RenderCommand : public RefCounted {
public:
    virtual ~RenderCommand() = default;
    virtual void Execute() = 0;
};

class DrawMeshCommand : public RenderCommand {
public:
    DrawMeshCommand(Ref<Mesh> mesh, Ref<Material> material)
        : m_Mesh(std::move(mesh)), m_Material(std::move(material)) {}
    
    void Execute() override {
        if (m_Mesh && m_Material) {
            // Execute rendering
            m_Material->Bind();
            m_Mesh->Draw();
        }
    }
    
private:
    Ref<Mesh> m_Mesh;
    Ref<Material> m_Material;
};

class CommandQueue {
private:
    std::vector<Ref<RenderCommand>> m_Commands;
    
public:
    void AddCommand(Ref<RenderCommand> command) {
        m_Commands.push_back(std::move(command));
    }
    
    void ExecuteAll() {
        for (auto& command : m_Commands) {
            command->Execute();
        }
        m_Commands.clear();
    }
};
```

### 4. Observer Pattern with Weak References

```cpp
class RenderEventObserver : public RefCounted {
public:
    virtual ~RenderEventObserver() = default;
    virtual void OnTextureLoaded(Ref<Texture> texture) = 0;
    virtual void OnMeshUpdated(Ref<Mesh> mesh) = 0;
};

class RenderEventDispatcher {
private:
    std::vector<WeakRef<RenderEventObserver>> m_Observers;
    
public:
    void AddObserver(Ref<RenderEventObserver> observer) {
        m_Observers.push_back(observer);
    }
    
    void NotifyTextureLoaded(Ref<Texture> texture) {
        // Clean expired observers and notify active ones
        for (auto it = m_Observers.begin(); it != m_Observers.end();) {
            if (auto observer = it->Lock()) {
                observer->OnTextureLoaded(texture);
                ++it;
            } else {
                it = m_Observers.erase(it); // Remove expired observer
            }
        }
    }
    
    void NotifyMeshUpdated(Ref<Mesh> mesh) {
        for (auto it = m_Observers.begin(); it != m_Observers.end();) {
            if (auto observer = it->Lock()) {
                observer->OnMeshUpdated(mesh);
                ++it;
            } else {
                it = m_Observers.erase(it);
            }
        }
    }
};
```

## Common Use Cases

### 1. Vulkan Resource Management

```cpp
class VulkanTexture : public RefCounted {
public:
    static Ref<VulkanTexture> Create(VkDevice device, uint32_t width, uint32_t height) {
        auto texture = CreateRef<VulkanTexture>(device, width, height);
        if (!texture->Initialize()) {
            return nullptr;
        }
        return texture;
    }
    
    ~VulkanTexture() {
        if (m_Image != VK_NULL_HANDLE) {
            vkDestroyImage(m_Device, m_Image, nullptr);
        }
        if (m_Memory != VK_NULL_HANDLE) {
            vkFreeMemory(m_Device, m_Memory, nullptr);
        }
    }
    
private:
    VulkanTexture(VkDevice device, uint32_t width, uint32_t height)
        : m_Device(device), m_Width(width), m_Height(height) {}
    
    bool Initialize() {
        // Vulkan texture creation logic
        return true;
    }
    
    VkDevice m_Device;
    VkImage m_Image = VK_NULL_HANDLE;
    VkDeviceMemory m_Memory = VK_NULL_HANDLE;
    uint32_t m_Width, m_Height;
};

class VulkanBuffer : public RefCounted {
public:
    static Ref<VulkanBuffer> Create(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage) {
        auto buffer = CreateRef<VulkanBuffer>(device, size, usage);
        if (!buffer->Initialize()) {
            return nullptr;
        }
        return buffer;
    }
    
    ~VulkanBuffer() {
        if (m_Buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_Device, m_Buffer, nullptr);
        }
        if (m_Memory != VK_NULL_HANDLE) {
            vkFreeMemory(m_Device, m_Memory, nullptr);
        }
    }
    
private:
    VulkanBuffer(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage)
        : m_Device(device), m_Size(size), m_Usage(usage) {}
    
    bool Initialize() {
        // Vulkan buffer creation logic
        return true;
    }
    
    VkDevice m_Device;
    VkBuffer m_Buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_Memory = VK_NULL_HANDLE;
    VkDeviceSize m_Size;
    VkBufferUsageFlags m_Usage;
};
```

### 2. Scene Graph Implementation

```cpp
class Transform : public RefCounted {
public:
    void SetPosition(const Vec3& position) { m_Position = position; }
    void SetRotation(const Quat& rotation) { m_Rotation = rotation; }
    void SetScale(const Vec3& scale) { m_Scale = scale; }

    Mat4 GetMatrix() const {
        return Mat4::Translate(m_Position) *
               m_Rotation.ToMatrix() *
               Mat4::Scale(m_Scale);
    }
    
private:
    Vec3 m_Position = Vec3(0.0f);
    Quat m_Rotation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
    Vec3 m_Scale = Vec3(1.0f);
};

class SceneNode : public RefCounted {
public:
    SceneNode() : m_Transform(CreateRef<Transform>()) {}
    
    void AddChild(Ref<SceneNode> child) {
        child->m_Parent = WeakRef<SceneNode>(*this);
        m_Children.push_back(std::move(child));
    }
    
    void RemoveChild(Ref<SceneNode> child) {
        auto it = std::find(m_Children.begin(), m_Children.end(), child);
        if (it != m_Children.end()) {
            (*it)->m_Parent = WeakRef<SceneNode>();
            m_Children.erase(it);
        }
    }
    
    void SetMesh(Ref<Mesh> mesh) {
        m_Mesh = std::move(mesh);
    }
    
    Mat4 GetWorldMatrix() const {
        Mat4 worldMatrix = m_Transform->GetMatrix();
        if (auto parent = m_Parent.Lock()) {
            worldMatrix = parent->GetWorldMatrix() * worldMatrix;
        }
        return worldMatrix;
    }
    
    void Update(float deltaTime) {
        // Update this node
        OnUpdate(deltaTime);
        
        // Update children
        for (auto& child : m_Children) {
            child->Update(deltaTime);
        }
    }
    
    void Render(const RenderContext& context) {
        if (m_Mesh) {
            RenderParams params;
            params.worldMatrix = GetWorldMatrix();
            m_Mesh->Render(context, params);
        }
        
        // Render children
        for (auto& child : m_Children) {
            child->Render(context);
        }
    }
    
protected:
    virtual void OnUpdate(float deltaTime) {}
    
private:
    Ref<Transform> m_Transform;
    Ref<Mesh> m_Mesh;
    std::vector<Ref<SceneNode>> m_Children;
    WeakRef<SceneNode> m_Parent;
};
```

### 3. Asset Loading and Caching

```cpp
class Asset : public RefCounted {
public:
    enum class State {
        Unloaded,
        Loading,
        Loaded,
        Failed
    };
    
    virtual ~Asset() = default;
    virtual bool Load() = 0;
    virtual void Unload() = 0;
    
    State GetState() const { return m_State; }
    const std::string& GetPath() const { return m_Path; }
    
protected:
    Asset(const std::string& path) : m_Path(path), m_State(State::Unloaded) {}
    
    std::string m_Path;
    State m_State;
};

class AssetManager {
private:
    std::unordered_map<std::string, WeakRef<Asset>> m_AssetCache;
    mutable std::mutex m_CacheMutex;
    
public:
    template<typename T, typename... Args>
    Ref<T> LoadAsset(const std::string& path, Args&&... args) {
        static_assert(std::is_base_of_v<Asset, T>, "T must inherit from Asset");
        
        std::lock_guard<std::mutex> lock(m_CacheMutex);
        
        // Check cache
        auto it = m_AssetCache.find(path);
        if (it != m_AssetCache.end()) {
            if (auto cached = it->second.Lock()) {
                // Try to cast to requested type
                if (auto typedAsset = cached.DynamicCast<T>()) {
                    return typedAsset;
                }
            }
            // Remove expired or wrong type
            m_AssetCache.erase(it);
        }
        
        // Create new asset
        auto asset = CreateRef<T>(path, std::forward<Args>(args)...);
        if (asset->Load()) {
            m_AssetCache[path] = asset;
            return asset;
        }
        
        return nullptr;
    }
    
    void UnloadUnused() {
        std::lock_guard<std::mutex> lock(m_CacheMutex);
        
        for (auto it = m_AssetCache.begin(); it != m_AssetCache.end();) {
            if (auto asset = it->second.Lock()) {
                // Check if only the cache holds a reference
                if (asset.UseCount() == 1) {
                    asset->Unload();
                    it = m_AssetCache.erase(it);
                } else {
                    ++it;
                }
            } else {
                it = m_AssetCache.erase(it);
            }
        }
    }
};
```

## Performance Considerations

### 1. Reference Counting Overhead

```cpp
// ✅ EFFICIENT - Minimize reference count operations
void ProcessTextures(const std::vector<Ref<Texture>>& textures) {
    for (const auto& texture : textures) { // No copy, no ref count change
        texture->Process();
    }
}

// ❌ INEFFICIENT - Unnecessary copies
void ProcessTextures(const std::vector<Ref<Texture>>& textures) {
    for (auto texture : textures) { // Copy increases/decreases ref count
        texture->Process();
    }
}
```

### 2. Move Semantics

```cpp
// ✅ EFFICIENT - Use move semantics
class Material : public RefCounted {
public:
    void SetDiffuseTexture(Ref<Texture> texture) {
        m_DiffuseTexture = std::move(texture); // Move, no ref count change
    }
    
private:
    Ref<Texture> m_DiffuseTexture;
};

// ❌ INEFFICIENT - Unnecessary copies
class Material : public RefCounted {
public:
    void SetDiffuseTexture(const Ref<Texture>& texture) {
        m_DiffuseTexture = texture; // Copy increases ref count
    }
    
private:
    Ref<Texture> m_DiffuseTexture;
};
```

### 3. Weak Reference Usage

```cpp
// ✅ EFFICIENT - Check expiration before locking
void ProcessNode(const WeakRef<SceneNode>& weakNode) {
    if (!weakNode.Expired()) { // Quick atomic check
        if (auto node = weakNode.Lock()) { // Acquire strong reference
            node->Update();
        }
    }
}

// ❌ LESS EFFICIENT - Always try to lock
void ProcessNode(const WeakRef<SceneNode>& weakNode) {
    if (auto node = weakNode.Lock()) { // Always tries to acquire lock
        node->Update();
    }
}
```

## Thread Safety

### 1. Reference Counting

The reference counting operations are fully thread-safe:

```cpp
// Safe to do from multiple threads
void ThreadSafeFunction(Ref<Texture> texture) {
    // Copying Ref is thread-safe
    Ref<Texture> localCopy = texture;
    
    // Moving Ref is thread-safe
    auto moved = std::move(texture);
    
    // Accessing object requires external synchronization
    // texture->GetWidth(); // Requires synchronization if object is mutable
}
```

### 2. Weak Reference Safety

```cpp
class ThreadSafeCache {
private:
    std::unordered_map<std::string, WeakRef<Asset>> m_Cache;
    mutable std::shared_mutex m_CacheMutex;
    
public:
    Ref<Asset> GetAsset(const std::string& id) {
        // Read lock for cache access
        std::shared_lock<std::shared_mutex> readLock(m_CacheMutex);
        
        auto it = m_Cache.find(id);
        if (it != m_Cache.end()) {
            if (auto asset = it->second.Lock()) { // Thread-safe
                return asset;
            }
        }
        
        readLock.unlock();
        
        // Write lock for cache modification
        std::unique_lock<std::shared_mutex> writeLock(m_CacheMutex);
        
        // Double-check pattern
        it = m_Cache.find(id);
        if (it != m_Cache.end()) {
            if (auto asset = it->second.Lock()) {
                return asset;
            }
            m_Cache.erase(it); // Clean expired entry
        }
        
        // Load new asset...
        auto newAsset = LoadAssetFromDisk(id);
        if (newAsset) {
            m_Cache[id] = newAsset;
        }
        
        return newAsset;
    }
};
```

## Error Handling

### 1. Null Pointer Handling

```cpp
// ✅ DEFENSIVE - Always check validity
void SafeFunction(Ref<Texture> texture) {
    if (!texture) {
        LogError("Null texture passed to SafeFunction");
        return;
    }
    
    if (!texture.IsValid()) {
        LogError("Invalid texture passed to SafeFunction");
        return;
    }
    
    // Safe to use
    texture->Bind();
}

// ✅ ASSERTION - For internal consistency
void InternalFunction(Ref<Texture> texture) {
    assert(texture && "Texture must not be null");
    assert(texture.IsValid() && "Texture must be valid");
    
    // Safe to use in debug builds
    texture->Bind();
}
```

### 2. Weak Reference Expiration

```cpp
class RenderSystem {
private:
    std::vector<WeakRef<RenderObject>> m_RenderObjects;
    
public:
    void RenderFrame() {
        // Process valid objects and clean expired ones
        for (auto it = m_RenderObjects.begin(); it != m_RenderObjects.end();) {
            if (auto obj = it->Lock()) {
                try {
                    obj->Render();
                    ++it;
                } catch (const std::exception& e) {
                    LogError("Render error: {}", e.what());
                    ++it; // Keep the object, error might be transient
                }
            } else {
                // Object expired, remove from list
                it = m_RenderObjects.erase(it);
            }
        }
    }
};
```

## Interoperability

### 1. With Standard Library

```cpp
// Converting to std::shared_ptr
Ref<Texture> texture = CreateRef<Texture>();
std::shared_ptr<Texture> stdPtr = texture.ToSharedPtr();

// Converting from std::shared_ptr
std::shared_ptr<Texture> stdTexture = std::make_shared<Texture>();
Ref<Texture> refTexture(stdTexture);

// Using with standard algorithms
std::vector<Ref<Mesh>> meshes;
std::sort(meshes.begin(), meshes.end(), 
    [](const Ref<Mesh>& a, const Ref<Mesh>& b) {
        return a->GetVertexCount() < b->GetVertexCount();
    });
```

### 2. With External Libraries

```cpp
// When interfacing with C APIs
void CallCFunction(Ref<Buffer> buffer) {
    // Extract raw pointer for C API
    c_function(buffer.Get());
}

// When working with libraries expecting std::shared_ptr
void LibraryFunction(const std::shared_ptr<Texture>& texture);

void CallLibrary(Ref<Texture> texture) {
    LibraryFunction(texture.ToSharedPtr());
}
```

## Migration Guidelines

### 1. From std::shared_ptr

```cpp
// OLD CODE
class OldRenderer {
private:
    std::shared_ptr<Texture> m_Texture;
    std::vector<std::shared_ptr<Mesh>> m_Meshes;
    
public:
    void SetTexture(std::shared_ptr<Texture> texture) {
        m_Texture = texture;
    }
};

// NEW CODE
class NewRenderer {
private:
    Ref<Texture> m_Texture;
    std::vector<Ref<Mesh>> m_Meshes;
    
public:
    void SetTexture(Ref<Texture> texture) {
        m_Texture = std::move(texture);
    }
};
```

### 2. From Raw Pointers

```cpp
// OLD CODE (DANGEROUS)
class OldMaterial {
private:
    Texture* m_DiffuseTexture = nullptr;
    
public:
    ~OldMaterial() {
        delete m_DiffuseTexture; // Manual memory management
    }
    
    void SetTexture(Texture* texture) {
        delete m_DiffuseTexture;
        m_DiffuseTexture = texture;
    }
};

// NEW CODE (SAFE)
class NewMaterial : public RefCounted {
private:
    Ref<Texture> m_DiffuseTexture;
    
public:
    // Automatic cleanup, no manual delete needed
    
    void SetTexture(Ref<Texture> texture) {
        m_DiffuseTexture = std::move(texture);
    }
};
```

## Summary for GitHub Copilot

When generating code for Scenery Editor X:

1. **Always use project smart pointers**: `Ref<T>`, `WeakRef<T>`, `Scope<T>`
2. **Inherit from RefCounted**: All shared objects must inherit from `RefCounted`
3. **Use appropriate ownership**: Strong refs for ownership, weak refs for observation
4. **Prefer move semantics**: Use `std::move()` when transferring ownership
5. **Check validity**: Always validate pointers before use
6. **Break cycles**: Use weak references to prevent circular dependencies
7. **Follow factory patterns**: Use `CreateRef<T>()` and `CreateScope<T>()`
8. **Handle thread safety**: Use appropriate synchronization for shared data
9. **Clean expired references**: Regularly clean up expired weak references
10. **Use const references**: For parameters that won't be stored

This smart pointer system is the foundation of memory management in Scenery Editor X. Following these guidelines ensures safe, efficient, and maintainable code.
```cpp
class RefCounted
{
public:
    RefCounted() = default;
    virtual ~RefCounted() = default;
    
    uint32_t IncRefCount() const noexcept;
    uint32_t DecRefCount() const noexcept;
    uint32_t GetRefCount() const noexcept;
    
private:
    mutable std::atomic<uint32_t> m_RefCount{0};
};
```

### Usage Example
```cpp
class Texture : public RefCounted
{
public:
    Texture(const std::string& path) : m_Path(path) {}
    
    const std::string& GetPath() const { return m_Path; }
    
private:
    std::string m_Path;
};

// Usage
Ref<Texture> texture = CreateRef<Texture>("assets/diffuse.png");
```

## Ref<T> - Strong References

### Purpose
`Ref<T>` is the primary smart pointer for shared ownership. It automatically manages the lifetime of objects through reference counting.

### Key Features
- Automatic memory management
- Thread-safe reference counting
- Type conversion support
- Interoperability with std::shared_ptr
- Move semantics for performance

### Construction Examples
```cpp
// Create new object
Ref<Texture> texture = CreateRef<Texture>("texture.png");

// From raw pointer (takes ownership)
Texture* rawTexture = new Texture("texture.png");
Ref<Texture> texture(rawTexture);

// Copy construction (shares ownership)
Ref<Texture> texture2 = texture;

// Move construction (transfers ownership)
Ref<Texture> texture3 = std::move(texture);

// From std::shared_ptr
std::shared_ptr<Texture> sharedTexture = std::make_shared<Texture>("texture.png");
Ref<Texture> texture4(sharedTexture);
```

### Common Operations
```cpp
// Check if valid
if (texture) {
    // Safe to use
}

// Get raw pointer
Texture* rawPtr = texture.Get();

// Access members
std::string path = texture->GetPath();
Texture& ref = *texture;

// Check reference count
uint32_t count = texture.UseCount();

// Check if unique owner
bool unique = texture.IsUnique();

// Reset to null or new object
texture.Reset();
texture.Reset(new Texture("new_texture.png"));
```

### Type Conversions
```cpp
// Static cast (compile-time)
Ref<BaseTexture> base = texture.As<BaseTexture>();

// Dynamic cast (runtime)
Ref<DerivedTexture> derived = base.DynamicCast<DerivedTexture>();
if (derived) {
    // Cast succeeded
}
```

## WeakRef<T> - Weak References

### Purpose
`WeakRef<T>` provides non-owning observation of objects managed by `Ref<T>`. It's essential for breaking circular dependencies and safe observation patterns.

### Key Features
- Non-owning observation
- Automatic expiration detection
- Thread-safe access
- Conversion to strong references

### Usage Examples
```cpp
// Create from strong reference
Ref<Texture> texture = CreateRef<Texture>("texture.png");
WeakRef<Texture> weakTexture = texture;

// Check if still valid
if (!weakTexture.Expired()) {
    // Try to get strong reference
    Ref<Texture> strongRef = weakTexture.Lock();
    if (strongRef) {
        // Object still exists, safe to use
        std::string path = strongRef->GetPath();
    }
}

// Observer pattern example
class TextureManager
{
private:
    std::vector<WeakRef<Texture>> m_ObservedTextures;
    
public:
    void RegisterTexture(const Ref<Texture>& texture) {
        m_ObservedTextures.push_back(texture);
    }
    
    void CleanupExpiredTextures() {
        m_ObservedTextures.erase(
            std::remove_if(m_ObservedTextures.begin(), m_ObservedTextures.end(),
                [](const WeakRef<Texture>& weak) { return weak.Expired(); }),
            m_ObservedTextures.end()
        );
    }
};
```

### Circular Dependency Prevention
```cpp
class SceneNode : public RefCounted
{
public:
    void AddChild(const Ref<SceneNode>& child) {
        m_Children.push_back(child);
        child->m_Parent = WeakRef<SceneNode>(*this); // Weak reference to parent
    }
    
    Ref<SceneNode> GetParent() const {
        return m_Parent.Lock(); // Convert to strong reference when needed
    }
    
private:
    std::vector<Ref<SceneNode>> m_Children;     // Strong references to children
    WeakRef<SceneNode> m_Parent;                // Weak reference to parent
};
```

## Scope<T> - Unique Ownership

### Purpose
`Scope<T>` is an alias for `std::unique_ptr<T>` providing exclusive ownership semantics.

### Usage
```cpp
// Create unique object
Scope<Buffer> buffer = CreateScope<Buffer>(1024);

// Move semantics (ownership transfer)
Scope<Buffer> buffer2 = std::move(buffer);

// Access
buffer2->Write(data, size);
```

## Control Block System

### Internal Architecture
The weak reference system uses a control block registry to manage the lifetime of weak references independently from the objects they observe.

```cpp
namespace Internal {
    template<typename T>
    class ControlBlock {
        T* m_Ptr;                          // Pointer to object (nullable)
        std::atomic<uint32_t> m_WeakCount; // Number of weak references
    };
    
    template<typename T>
    class ControlBlockRegistry {
        std::unordered_map<T*, ControlBlock<T>*> m_Blocks;
        std::mutex m_Mutex;
    };
}
```

### Lifecycle Management
1. When first `WeakRef<T>` is created, a control block is allocated
2. Control block tracks weak reference count
3. When object is destroyed, control block pointer is set to nullptr
4. Control block is destroyed when last weak reference expires

## Usage Guidelines

### When to Use Each Type

#### Use `Ref<T>` when:
- You need shared ownership
- Object lifetime depends on multiple owners
- Working with polymorphic objects
- Interfacing with existing Ref-based APIs

#### Use `WeakRef<T>` when:
- You need to observe an object without owning it
- Breaking circular dependencies
- Implementing observer patterns
- Caching references that might become invalid

#### Use `Scope<T>` when:
- You need exclusive ownership
- Object has clear single owner
- RAII semantics are sufficient
- Performance is critical (no reference counting overhead)

### Object Design Guidelines

```cpp
// Good: Clear ownership model
class Mesh : public RefCounted
{
public:
    void SetMaterial(const Ref<Material>& material) {
        m_Material = material; // Strong reference - mesh owns material
    }
    
    void SetParentNode(const Ref<SceneNode>& parent) {
        m_ParentNode = parent; // Weak reference - node owns mesh
    }
    
private:
    Ref<Material> m_Material;      // Owned resource
    WeakRef<SceneNode> m_ParentNode; // Observer relationship
};
```

## Performance Considerations

### Reference Counting Overhead
- Atomic operations have small but measurable cost
- Prefer move semantics when transferring ownership
- Consider `Scope<T>` for performance-critical single-owner scenarios

### Memory Overhead
- `Ref<T>`: One pointer (8 bytes on x64)
- `WeakRef<T>`: One pointer to control block (8 bytes on x64)
- Control blocks: Allocated on-demand, shared among weak references

### Optimization Tips
```cpp
// Prefer move construction/assignment
Ref<Texture> texture = std::move(sourceTexture);

// Avoid unnecessary copies
void ProcessTexture(const Ref<Texture>& texture); // Good: pass by const reference
void ProcessTexture(Ref<Texture> texture);        // Less efficient: copy

// Use CreateRef for new objects
Ref<Texture> texture = CreateRef<Texture>(args...); // Preferred
Ref<Texture> texture(new Texture(args...));         // Works but less clear
```

## Thread Safety

### Guarantees
- Reference counting operations are atomic and thread-safe
- Multiple threads can safely copy/destroy `Ref<T>` instances
- Control block operations are protected by mutex
- Object destruction is thread-safe

### Safe Patterns
```cpp
// Safe: Multiple threads can copy references
std::vector<std::thread> threads;
Ref<Texture> sharedTexture = CreateRef<Texture>("texture.png");

for (int i = 0; i < 4; ++i) {
    threads.emplace_back([sharedTexture]() {
        // Each thread has its own copy of the Ref
        ProcessTexture(sharedTexture);
    });
}

// Safe: Weak reference checking
void BackgroundThread(WeakRef<Texture> weakTexture) {
    while (!shouldExit) {
        if (auto texture = weakTexture.Lock()) {
            // Safe to use texture in this scope
            UpdateTexture(texture);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}
```

### Unsafe Patterns to Avoid
```cpp
// UNSAFE: Racing on raw pointer access
Ref<Texture> texture = GetTexture();
if (texture) {
    // Another thread might destroy texture here!
    texture->DoSomething(); // Potential crash
}

// SAFER: Use local strong reference
if (auto localTexture = texture) {
    localTexture->DoSomething(); // Safe: we own a reference
}
```

## Examples and Best Practices

### Resource Management Example
```cpp
class RenderContext : public RefCounted
{
public:
    Ref<Buffer> CreateBuffer(size_t size, BufferUsage usage) {
        auto buffer = CreateRef<Buffer>(size, usage);
        m_Buffers.push_back(buffer); // Strong reference for tracking
        return buffer;
    }
    
    void CleanupUnusedBuffers() {
        m_Buffers.erase(
            std::remove_if(m_Buffers.begin(), m_Buffers.end(),
                [](const WeakRef<Buffer>& weak) { return weak.Expired(); }),
            m_Buffers.end()
        );
    }
    
private:
    std::vector<WeakRef<Buffer>> m_Buffers; // Weak references for cleanup
};
```

### Asset Management Example
```cpp
class AssetManager : public RefCounted
{
public:
    Ref<Texture> LoadTexture(const std::string& path) {
        // Check cache first
        auto it = m_TextureCache.find(path);
        if (it != m_TextureCache.end()) {
            if (auto texture = it->second.Lock()) {
                return texture; // Return cached texture
            }
            m_TextureCache.erase(it); // Clean up expired entry
        }
        
        // Load new texture
        auto texture = CreateRef<Texture>(path);
        m_TextureCache[path] = texture; // Cache as weak reference
        return texture;
    }
    
private:
    std::unordered_map<std::string, WeakRef<Texture>> m_TextureCache;
};
```

### Scene Graph Example
```cpp
class SceneNode : public RefCounted
{
public:
    void AddChild(const Ref<SceneNode>& child) {
        m_Children.push_back(child);
        child->m_Parent = WeakRef<SceneNode>(*this);
    }
    
    void RemoveChild(const Ref<SceneNode>& child) {
        auto it = std::find(m_Children.begin(), m_Children.end(), child);
        if (it != m_Children.end()) {
            (*it)->m_Parent.Reset();
            m_Children.erase(it);
        }
    }
    
    Ref<SceneNode> GetParent() const {
        return m_Parent.Lock();
    }
    
    const std::vector<Ref<SceneNode>>& GetChildren() const {
        return m_Children;
    }
    
private:
    std::vector<Ref<SceneNode>> m_Children;
    WeakRef<SceneNode> m_Parent;
};
```

## GitHub Copilot Guidelines

### Code Generation Standards

When GitHub Copilot generates code for the Scenery Editor X project, follow these guidelines:

#### 1. **Always Use Project Smart Pointers**
```cpp
// CORRECT: Use project smart pointers
Ref<Texture> texture = CreateRef<Texture>("path.png");
WeakRef<Material> materialObserver = material;
Scope<Buffer> tempBuffer = CreateScope<Buffer>(size);

// AVOID: Standard library smart pointers (unless interfacing with external APIs)
std::shared_ptr<Texture> texture = std::make_shared<Texture>("path.png");
std::unique_ptr<Buffer> buffer = std::make_unique<Buffer>(size);
```

#### 2. **Inheritance from RefCounted**
```cpp
// CORRECT: All shared objects inherit from RefCounted
class RenderPass : public RefCounted
{
    // Implementation
};

// CORRECT: Use appropriate base class for polymorphic hierarchies
class Texture : public RefCounted
{
public:
    virtual ~Texture() = default;
    virtual void Bind() = 0;
};

class Texture2D : public Texture
{
public:
    void Bind() override { /* implementation */ }
};
```

#### 3. **Factory Pattern Usage**
```cpp
// CORRECT: Use CreateRef for object creation
class TextureFactory
{
public:
    static Ref<Texture> CreateTexture2D(uint32_t width, uint32_t height) {
        return CreateRef<Texture2D>(width, height);
    }
    
    static Ref<Texture> LoadFromFile(const std::string& path) {
        return CreateRef<FileTexture>(path);
    }
};
```

#### 4. **Ownership Semantics**
```cpp
// CORRECT: Clear ownership relationships
class Mesh : public RefCounted
{
public:
    void SetMaterial(const Ref<Material>& material) { m_Material = material; }
    void SetParentNode(const WeakRef<SceneNode>& parent) { m_Parent = parent; }
    
private:
    Ref<Material> m_Material;        // Mesh owns its material
    WeakRef<SceneNode> m_Parent;     // Mesh observes its parent
};
```

#### 5. **Error Handling Patterns**
```cpp
// CORRECT: Check validity before use
bool ProcessTexture(const WeakRef<Texture>& weakTexture) {
    auto texture = weakTexture.Lock();
    if (!texture) {
        SEDX_LOG_WARNING("Texture reference expired");
        return false;
    }
    
    // Safe to use texture
    texture->Bind();
    return true;
}
```

#### 6. **Collection Management**
```cpp
// CORRECT: Use appropriate reference type for collections
class RenderQueue
{
public:
    void AddMesh(const Ref<Mesh>& mesh) {
        m_Meshes.push_back(mesh); // Strong reference - queue owns meshes
    }
    
    void AddObserver(const Ref<RenderObserver>& observer) {
        m_Observers.push_back(observer); // Weak reference - don't own observers
    }
    
private:
    std::vector<Ref<Mesh>> m_Meshes;
    std::vector<WeakRef<RenderObserver>> m_Observers;
};
```

#### 7. **Thread Safety Considerations**
```cpp
// CORRECT: Thread-safe access patterns
class ThreadSafeResourceCache
{
public:
    Ref<Resource> GetResource(const std::string& id) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        
        auto it = m_Cache.find(id);
        if (it != m_Cache.end()) {
            if (auto resource = it->second.Lock()) {
                return resource;
            }
            m_Cache.erase(it); // Clean up expired entry
        }
        
        return nullptr;
    }
    
private:
    std::mutex m_Mutex;
    std::unordered_map<std::string, WeakRef<Resource>> m_Cache;
};
```

#### 8. **Documentation Standards**
When generating new classes, always include proper Doxygen documentation:

```cpp
/**
 * @brief A high-performance texture implementation for Vulkan rendering
 * 
 * This class provides a reference-counted texture object that can be safely
 * shared across multiple rendering contexts. It supports various texture formats
 * and provides efficient GPU memory management through VMA integration.
 * 
 * Usage example:
 * @code
 * auto texture = CreateRef<VulkanTexture>(1024, 1024, TextureFormat::RGBA8);
 * texture->LoadFromFile("assets/diffuse.png");
 * material->SetDiffuseTexture(texture);
 * @endcode
 * 
 * @note This class is thread-safe for reference counting operations but texture
 *       data modifications should be synchronized externally.
 */
class VulkanTexture : public RefCounted
{
    // Implementation
};
```

### Code Review Checklist for Copilot

When reviewing or generating code, ensure:

- [ ] All shared objects inherit from `RefCounted`
- [ ] Use `CreateRef<T>()` for object creation
- [ ] Prefer `WeakRef<T>` for observer relationships
- [ ] Use `Scope<T>` for single-owner scenarios
- [ ] Check weak references before use with `.Lock()`
- [ ] Proper const-correctness in reference parameters
- [ ] Thread-safe patterns for multi-threaded code
- [ ] Appropriate documentation with usage examples
- [ ] Clear ownership semantics in class design
- [ ] Error handling for expired references

This comprehensive system provides a robust foundation for memory management in the Scenery Editor X project, ensuring both performance and safety in complex 3D rendering scenarios.

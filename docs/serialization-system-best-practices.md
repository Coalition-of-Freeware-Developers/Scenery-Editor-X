# Scenery Editor X - Serialization System Best Practices

---

## Overview

This document outlines best practices, design patterns, and guidelines for effectively using the Scenery Editor X serialization system. Following these practices ensures optimal performance, maintainability, and reliability in your serialization implementations.

## Core Principles

### 1. Choose the Right Approach

**Use SERIALIZABLE for Simple Data Structures**

```cpp
// ✅ Good: Simple data structure with straightforward members
struct MaterialProperties
{
    std::string name;
    Vec3 albedo{1.0f, 1.0f, 1.0f};
    float roughness = 0.5f;
    float metallic = 0.0f;
    bool isTransparent = false;
    uint32_t textureFlags = 0;
};

SERIALIZABLE(MaterialProperties,
    &MaterialProperties::name,
    &MaterialProperties::albedo,
    &MaterialProperties::roughness,
    &MaterialProperties::metallic,
    &MaterialProperties::isTransparent,
    &MaterialProperties::textureFlags
);
```

**Use Intrusive Interface for Complex Logic**

```cpp
// ✅ Good: Complex object requiring custom serialization logic
class TextureAtlas
{
private:
    std::vector<uint8_t> m_PixelData;
    std::vector<TextureRegion> m_Regions;
    uint32_t m_Width, m_Height;
    TextureFormat m_Format;
  
public:
    static void Serialize(SerializeWriter* writer, const TextureAtlas& atlas)
    {
        // Write metadata only, not the actual pixel data
        writer->WriteRaw(atlas.m_Width);
        writer->WriteRaw(atlas.m_Height);
        writer->WriteRaw(atlas.m_Format);
      
        // Serialize region definitions
        using namespace SceneryEditorX::Serialization;
        Serialize(writer, atlas.m_Regions);
      
        // Serialize compressed pixel data
        auto compressedData = atlas.CompressPixelData();
        writer->WriteArray(compressedData);
    }
  
    static void Deserialize(SerializeReader* reader, TextureAtlas& atlas)
    {
        reader->ReadRaw(atlas.m_Width);
        reader->ReadRaw(atlas.m_Height);
        reader->ReadRaw(atlas.m_Format);
      
        using namespace SceneryEditorX::Serialization;
        Deserialize(reader, atlas.m_Regions);
      
        std::vector<uint8_t> compressedData;
        reader->ReadArray(compressedData);
      
        // Decompress and validate pixel data
        atlas.DecompressPixelData(compressedData);
        atlas.ValidateRegions();
    }
};
```

### 2. Version Your Data Structures

**Always Include Version Information**

```cpp
// ✅ Good: Versioned data structure
class SaveFile
{
private:
    static constexpr uint32_t CURRENT_VERSION = 3;
  
    uint32_t m_Version = CURRENT_VERSION;
    std::string m_PlayerName;
    Vec3 m_PlayerPosition;
    std::vector<InventoryItem> m_Inventory;
    std::map<std::string, bool> m_Achievements; // Added in v2
    PlayerStatistics m_Statistics;              // Added in v3
  
public:
    static void Serialize(SerializeWriter* writer, const SaveFile& save)
    {
        writer->WriteRaw(save.m_Version);
        writer->WriteString(save.m_PlayerName);
      
        using namespace SceneryEditorX::Serialization;
        Serialize(writer, save.m_PlayerPosition, save.m_Inventory);
      
        if (save.m_Version >= 2) {
            Serialize(writer, save.m_Achievements);
        }
      
        if (save.m_Version >= 3) {
            Serialize(writer, save.m_Statistics);
        }
    }
  
    static void Deserialize(SerializeReader* reader, SaveFile& save)
    {
        reader->ReadRaw(save.m_Version);
      
        if (save.m_Version > CURRENT_VERSION) {
            SEDX_CORE_ERROR_TAG("SAVE", "Unsupported save version: {}", save.m_Version);
            throw std::runtime_error("Unsupported save file version");
        }
      
        reader->ReadString(save.m_PlayerName);
      
        using namespace SceneryEditorX::Serialization;
        Deserialize(reader, save.m_PlayerPosition, save.m_Inventory);
      
        if (save.m_Version >= 2) {
            Deserialize(reader, save.m_Achievements);
        } else {
            save.m_Achievements.clear(); // Default for older versions
        }
      
        if (save.m_Version >= 3) {
            Deserialize(reader, save.m_Statistics);
        } else {
            save.m_Statistics = PlayerStatistics{}; // Default for older versions
        }
      
        // Update to current version after successful load
        save.m_Version = CURRENT_VERSION;
    }
};
```

### 3. Handle Pointers and References Properly

**Serialize by ID/UUID, Not by Pointer**

```cpp
// ❌ Bad: Serializing raw pointers
struct BadNode
{
    Node* parent;           // Raw pointer - undefined behavior
    std::vector<Node*> children; // Raw pointers - undefined behavior
};

// ✅ Good: Serialize by identifier
struct GoodNode
{
    UUID nodeId;
    UUID parentId;          // Use ID instead of pointer
    std::vector<UUID> childIds; // Use IDs instead of pointers
  
    // Reconstruct relationships after deserialization
    void ResolveReferences(NodeManager& manager)
    {
        if (parentId != UUID::Invalid()) {
            m_Parent = manager.FindNode(parentId);
        }
      
        for (const auto& childId : childIds) {
            auto child = manager.FindNode(childId);
            if (child) {
                m_Children.push_back(child);
            }
        }
    }
  
private:
    Node* m_Parent = nullptr;
    std::vector<Node*> m_Children;
};

SERIALIZABLE(GoodNode,
    &GoodNode::nodeId,
    &GoodNode::parentId,
    &GoodNode::childIds
);
```

### 4. Validate Data During Deserialization

**Always Validate Critical Data**

```cpp
// ✅ Good: Comprehensive validation
class Mesh
{
public:
    static void Deserialize(SerializeReader* reader, Mesh& mesh)
    {
        // Read basic data
        reader->ReadString(mesh.m_Name);
        reader->ReadArray(mesh.m_Vertices);
        reader->ReadArray(mesh.m_Indices);
      
        // Validate data integrity
        if (mesh.m_Name.empty()) {
            SEDX_CORE_WARN_TAG("MESH", "Mesh has no name, assigning default");
            mesh.m_Name = "UnnamedMesh";
        }
      
        if (mesh.m_Vertices.empty()) {
            SEDX_CORE_ERROR_TAG("MESH", "Mesh has no vertices");
            throw std::runtime_error("Invalid mesh: no vertices");
        }
      
        if (mesh.m_Indices.empty()) {
            SEDX_CORE_WARN_TAG("MESH", "Mesh has no indices, creating default");
            mesh.GenerateDefaultIndices();
        }
      
        // Validate index bounds
        for (uint32_t index : mesh.m_Indices) {
            if (index >= mesh.m_Vertices.size()) {
                SEDX_CORE_ERROR_TAG("MESH", "Index {} out of bounds (max: {})", 
                    index, mesh.m_Vertices.size() - 1);
                throw std::runtime_error("Invalid mesh indices");
            }
        }
      
        // Recalculate derived data
        mesh.RecalculateBounds();
        mesh.ValidateNormals();
    }
};
```

## Performance Best Practices

### 1. Use Bulk Operations for Large Data

**Optimize Container Serialization**

```cpp
// ✅ Good: Bulk operations for trivial types
namespace SceneryEditorX::Serialization::Impl
{
    template<>
    static inline bool SerializeImpl(SerializeWriter* writer, const std::vector<float>& vec)
    {
        writer->WriteRaw<uint32_t>(static_cast<uint32_t>(vec.size()));
      
        // Single memory operation for trivial types
        if (!vec.empty()) {
            writer->WriteData(
                reinterpret_cast<const char*>(vec.data()),
                vec.size() * sizeof(float)
            );
        }
      
        return true;
    }
  
    template<>
    static inline bool DeserializeImpl(SerializeReader* reader, std::vector<float>& vec)
    {
        uint32_t size;
        reader->ReadRaw(size);
      
        vec.resize(size);
      
        if (size > 0) {
            reader->ReadData(
                reinterpret_cast<char*>(vec.data()),
                size * sizeof(float)
            );
        }
      
        return true;
    }
}
```

### 2. Profile Critical Paths

**Use Profiling for Serialization Hotspots**

```cpp
// ✅ Good: Profile serialization operations
class LargeDataset
{
public:
    static void Serialize(SerializeWriter* writer, const LargeDataset& dataset)
    {
        SEDX_PROFILE_SCOPE("LargeDataset::Serialize");
      
        {
            SEDX_PROFILE_SCOPE("SerializeMetadata");
            writer->WriteString(dataset.m_Name);
            writer->WriteRaw(dataset.m_CreationTime);
        }
      
        {
            SEDX_PROFILE_SCOPE("SerializeVertexData");
            writer->WriteArray(dataset.m_Vertices);
        }
      
        {
            SEDX_PROFILE_SCOPE("SerializeTextureData");
            dataset.SerializeCompressedTextures(writer);
        }
    }
};
```

### 3. Pre-allocate Containers

**Reserve Capacity for Known Sizes**

```cpp
// ✅ Good: Pre-allocate containers
template<typename T>
void ReadArrayOptimized(SerializeReader* reader, std::vector<T>& array)
{
    uint32_t size;
    reader->ReadRaw(size);
  
    // Pre-allocate to avoid reallocations
    array.clear();
    array.reserve(size);
  
    for (uint32_t i = 0; i < size; ++i) {
        array.emplace_back();
      
        using namespace SceneryEditorX::Serialization;
        Deserialize(reader, array.back());
    }
}
```

## Error Handling Best Practices

### 1. Use Exception-Safe Patterns

**Implement RAII and Exception Safety**

```cpp
// ✅ Good: Exception-safe serialization
class SafeFileWriter
{
private:
    std::unique_ptr<SerializeWriter> m_Writer;
    std::string m_TempFilename;
    std::string m_FinalFilename;
  
public:
    SafeFileWriter(const std::string& filename)
        : m_TempFilename(filename + ".tmp")
        , m_FinalFilename(filename)
    {
        m_Writer = std::make_unique<FileWriter>(m_TempFilename);
    }
  
    ~SafeFileWriter()
    {
        if (std::filesystem::exists(m_TempFilename)) {
            std::filesystem::remove(m_TempFilename); // Cleanup on failure
        }
    }
  
    template<typename... Args>
    bool Write(const Args&... args)
    {
        if (!m_Writer || !*m_Writer) {
            return false;
        }
      
        try {
            using namespace SceneryEditorX::Serialization;
            bool success = Serialize(m_Writer.get(), args...);
          
            if (success) {
                m_Writer.reset(); // Close file
              
                // Atomic rename on success
                std::filesystem::rename(m_TempFilename, m_FinalFilename);
                return true;
            }
        } catch (const std::exception& e) {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Exception during write: {}", e.what());
        }
      
        return false;
    }
};
```

### 2. Validate Stream State

**Check Stream Health at Critical Points**

```cpp
// ✅ Good: Stream validation
template<typename T>
bool ValidatedSerialize(SerializeWriter* writer, const T& data)
{
    if (!writer) {
        SEDX_CORE_ERROR_TAG("SERIALIZATION", "Null writer");
        return false;
    }
  
    if (!writer->IsStreamGood()) {
        SEDX_CORE_ERROR_TAG("SERIALIZATION", "Writer stream not ready");
        return false;
    }
  
    uint64_t startPosition = writer->GetStreamPosition();
  
    try {
        using namespace SceneryEditorX::Serialization;
        bool success = Serialize(writer, data);
      
        if (!success || !writer->IsStreamGood()) {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Serialization failed");
          
            // Attempt rollback
            try {
                writer->SetStreamPosition(startPosition);
            } catch (...) {
                SEDX_CORE_ERROR_TAG("SERIALIZATION", "Failed to rollback stream position");
            }
          
            return false;
        }
      
        return true;
    } catch (const std::exception& e) {
        SEDX_CORE_ERROR_TAG("SERIALIZATION", "Exception: {}", e.what());
        return false;
    }
}
```

## Memory Management Best Practices

### 1. Use Smart Pointers Appropriately

**Follow the Custom Pointer System**

```cpp
// ✅ Good: Use custom smart pointer system
class ResourceManager
{
private:
    std::unordered_map<UUID, Ref<Texture>> m_Textures;
    std::unordered_map<UUID, Ref<Mesh>> m_Meshes;
  
public:
    template<typename T>
    void SaveResource(const UUID& id, const Ref<T>& resource, const std::string& filename)
    {
        if (!resource) {
            SEDX_CORE_ERROR_TAG("RESOURCE", "Cannot save null resource");
            return;
        }
      
        FileWriter writer(filename);
        if (writer) {
            writer.WriteRaw(static_cast<uint64_t>(id));
          
            using namespace SceneryEditorX::Serialization;
            Serialize(&writer, *resource);
        }
    }
  
    template<typename T>
    Ref<T> LoadResource(const UUID& id, const std::string& filename)
    {
        FileReader reader(filename);
        if (!reader) return nullptr;
      
        uint64_t fileId;
        reader.ReadRaw(fileId);
      
        if (UUID(fileId) != id) {
            SEDX_CORE_ERROR_TAG("RESOURCE", "Resource ID mismatch");
            return nullptr;
        }
      
        auto resource = CreateRef<T>();
      
        using namespace SceneryEditorX::Serialization;
        if (Deserialize(&reader, *resource)) {
            return resource;
        }
      
        return nullptr;
    }
};
```

### 2. Manage Large Memory Allocations

**Handle Large Data Sets Efficiently**

```cpp
// ✅ Good: Streaming large data
class LargeTextureArray
{
private:
    struct TextureHeader
    {
        uint32_t width, height;
        TextureFormat format;
        uint32_t mipLevels;
        uint64_t dataOffset;
        uint64_t dataSize;
    };
  
    std::vector<TextureHeader> m_Headers;
    std::unique_ptr<SerializeWriter> m_DataWriter;
  
public:
    void BeginSerialization(const std::string& filename)
    {
        m_DataWriter = std::make_unique<FileWriter>(filename);
      
        // Reserve space for header count
        uint32_t headerCount = 0;
        m_DataWriter->WriteRaw(headerCount);
    }
  
    void AddTexture(const Texture& texture)
    {
        if (!m_DataWriter) return;
      
        TextureHeader header;
        header.width = texture.GetWidth();
        header.height = texture.GetHeight();
        header.format = texture.GetFormat();
        header.mipLevels = texture.GetMipLevels();
        header.dataOffset = m_DataWriter->GetStreamPosition();
      
        // Write texture data
        auto pixelData = texture.GetPixelData();
        header.dataSize = pixelData.size();
      
        m_DataWriter->WriteArray(pixelData);
      
        m_Headers.push_back(header);
    }
  
    void EndSerialization()
    {
        if (!m_DataWriter) return;
      
        // Write headers at the end
        uint64_t headersOffset = m_DataWriter->GetStreamPosition();
      
        for (const auto& header : m_Headers) {
            m_DataWriter->WriteRaw(header);
        }
      
        // Update header count at beginning
        m_DataWriter->SetStreamPosition(0);
        m_DataWriter->WriteRaw(static_cast<uint32_t>(m_Headers.size()));
      
        m_DataWriter.reset();
        m_Headers.clear();
    }
};
```

## Integration Patterns

### 1. Module System Integration

**Create Serialization-Aware Modules**

```cpp
// ✅ Good: Module with serialization support
class DataPersistenceModule : public Module
{
private:
    std::string m_DataDirectory;
    std::unordered_map<std::string, std::vector<uint8_t>> m_Cache;
  
public:
    explicit DataPersistenceModule(std::string dataDir = "data/") 
        : Module("DataPersistenceModule")
        , m_DataDirectory(std::move(dataDir))
    {
    }
  
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("DATA_PERSISTENCE", "Initializing data persistence");
      
        // Create data directory if it doesn't exist
        if (!std::filesystem::exists(m_DataDirectory)) {
            std::filesystem::create_directories(m_DataDirectory);
        }
      
        LoadCache();
    }
  
    void OnDetach() override
    {
        SEDX_CORE_INFO_TAG("DATA_PERSISTENCE", "Saving data cache");
        SaveCache();
    }
  
    template<typename T>
    bool SaveData(const std::string& key, const T& data, bool useCache = true)
    {
        SEDX_PROFILE_SCOPE("DataPersistenceModule::SaveData");
      
        if (useCache) {
            MemoryWriter writer;
          
            using namespace SceneryEditorX::Serialization;
            if (Serialize(&writer, data)) {
                m_Cache[key] = writer.GetBuffer();
                return true;
            }
        } else {
            auto filename = m_DataDirectory + key + ".dat";
            FileWriter writer(filename);
          
            using namespace SceneryEditorX::Serialization;
            return Serialize(&writer, data);
        }
      
        return false;
    }
  
    template<typename T>
    bool LoadData(const std::string& key, T& data, bool useCache = true)
    {
        SEDX_PROFILE_SCOPE("DataPersistenceModule::LoadData");
      
        if (useCache) {
            auto it = m_Cache.find(key);
            if (it != m_Cache.end()) {
                MemoryReader reader(it->second);
              
                using namespace SceneryEditorX::Serialization;
                return Deserialize(&reader, data);
            }
        }
      
        auto filename = m_DataDirectory + key + ".dat";
        FileReader reader(filename);
      
        using namespace SceneryEditorX::Serialization;
        return Deserialize(&reader, data);
    }
  
private:
    void SaveCache()
    {
        auto cacheFile = m_DataDirectory + "cache.dat";
        FileWriter writer(cacheFile);
      
        using namespace SceneryEditorX::Serialization;
        Serialize(&writer, m_Cache);
    }
  
    void LoadCache()
    {
        auto cacheFile = m_DataDirectory + "cache.dat";
        if (!std::filesystem::exists(cacheFile)) return;
      
        FileReader reader(cacheFile);
      
        using namespace SceneryEditorX::Serialization;
        Deserialize(&reader, m_Cache);
    }
};
```

### 2. Asset System Integration

**Implement Asset-Aware Serialization**

```cpp
// ✅ Good: Asset reference serialization
class AssetReference
{
private:
    UUID m_AssetId;
    std::string m_AssetPath;
    mutable WeakRef<Asset> m_CachedAsset;
  
public:
    AssetReference() = default;
    explicit AssetReference(const UUID& id) : m_AssetId(id) {}
    AssetReference(const UUID& id, std::string path) 
        : m_AssetId(id), m_AssetPath(std::move(path)) {}
  
    static void Serialize(SerializeWriter* writer, const AssetReference& ref)
    {
        writer->WriteRaw(static_cast<uint64_t>(ref.m_AssetId));
        writer->WriteString(ref.m_AssetPath);
    }
  
    static void Deserialize(SerializeReader* reader, AssetReference& ref)
    {
        uint64_t id;
        reader->ReadRaw(id);
        reader->ReadString(ref.m_AssetPath);
      
        ref.m_AssetId = UUID(id);
        ref.m_CachedAsset.Reset(); // Clear cache
    }
  
    template<typename T>
    Ref<T> Resolve() const
    {
        if (auto cached = m_CachedAsset.Lock()) {
            return cached.As<T>();
        }
      
        auto asset = AssetManager::Get().LoadAsset<T>(m_AssetId, m_AssetPath);
        m_CachedAsset = asset;
        return asset;
    }
  
    UUID GetId() const { return m_AssetId; }
    const std::string& GetPath() const { return m_AssetPath; }
};
```

## Common Pitfalls to Avoid

### 1. Don't Serialize Computed Values

```cpp
// ❌ Bad: Serializing computed/derived data
struct BadTransform
{
    Vec3 position;
    Vec3 rotation;
    Vec3 scale;
    Mat4 worldMatrix; // Computed from position/rotation/scale - DON'T serialize
    Mat4 inverseMatrix; // Computed from worldMatrix - DON'T serialize
};

// ✅ Good: Only serialize source data
struct GoodTransform
{
    Vec3 position;
    Vec3 rotation;
    Vec3 scale;
  
    // Computed values - recalculated after deserialization
    Mat4 GetWorldMatrix() const { return CalculateWorldMatrix(); }
    Mat4 GetInverseMatrix() const { return GetWorldMatrix().Inverse(); }
  
private:
    Mat4 CalculateWorldMatrix() const
    {
        return Mat4::Translation(position) * 
               Mat4::Rotation(rotation) * 
               Mat4::Scale(scale);
    }
};

SERIALIZABLE(GoodTransform,
    &GoodTransform::position,
    &GoodTransform::rotation,
    &GoodTransform::scale
);
```

### 2. Don't Ignore Platform Differences

```cpp
// ❌ Bad: Platform-dependent serialization
struct BadHeader
{
    size_t itemCount; // size_t varies by platform!
    long timestamp;   // long varies by platform!
};

// ✅ Good: Use fixed-size types
struct GoodHeader
{
    uint32_t itemCount; // Always 32-bit
    uint64_t timestamp; // Always 64-bit
};

SERIALIZABLE(GoodHeader,
    &GoodHeader::itemCount,
    &GoodHeader::timestamp
);
```

### 3. Don't Forget Endianness for Cross-Platform

```cpp
// ✅ Good: Handle endianness for cross-platform compatibility
namespace SceneryEditorX::Serialization::Impl
{
    template<>
    static inline bool SerializeImpl(SerializeWriter* writer, const uint32_t& value)
    {
        // Convert to little-endian for consistent cross-platform format
        uint32_t littleEndianValue = ToLittleEndian(value);
        writer->WriteRaw(littleEndianValue);
        return true;
    }
  
    template<>
    static inline bool DeserializeImpl(SerializeReader* reader, uint32_t& value)
    {
        uint32_t littleEndianValue;
        reader->ReadRaw(littleEndianValue);
        value = FromLittleEndian(littleEndianValue);
        return true;
    }
}
```

# Scenery Editor X - Serialization System Documentation

---

## Overview

The Scenery Editor X serialization system provides a comprehensive, high-performance binary serialization framework designed for efficient storage and retrieval of complex application data. The system supports both intrusive and non-intrusive serialization approaches, integrates seamlessly with the reflection system, and offers specialized handling for various data types including primitives, containers, custom objects, and Vulkan buffers.

## Architecture

### Core Components

The serialization system consists of several key components:

1. **SerializeWriter** - Abstract base class for binary data writing
2. **SerializeReader** - Abstract base class for binary data reading
3. **Serialization::Impl** - Template-based serialization implementation
4. **SERIALIZABLE** - Macro for automatic member-wise serialization
5. **Type-specific specializations** - Custom handling for complex types

### Design Principles

- **Performance-first**: Binary format for minimal overhead
- **Type-safe**: Template-based approach with compile-time type checking
- **Extensible**: Support for custom serialization through specializations
- **Integration**: Deep integration with the reflection system via `Types::Description`
- **Memory-efficient**: Optimized for large data sets and complex scenes

## Core Classes

### SerializeWriter

Abstract base class providing binary data writing capabilities:

```cpp
class SerializeWriter
{
public:
    virtual ~SerializeWriter() = default;

    // Core interface - must be implemented by derived classes
    [[nodiscard]] virtual bool IsStreamGood() const = 0;
    virtual uint64_t GetStreamPosition() = 0;
    virtual void SetStreamPosition(uint64_t position) = 0;
    virtual bool WriteData(const char* data, size_t size) = 0;

    // Convenience methods
    explicit operator bool() const { return IsStreamGood(); }
  
    // Specialized write methods
    void WriteBuffer(Buffer buffer, bool writeSize = true);
    void WriteZero(uint64_t size);
    void WriteString(const std::string& string);
  
    // Template methods for type-safe serialization
    template<typename T> void WriteRaw(const T& type);
    template<typename T> void WriteObject(const T& obj);
    template<typename Key, typename Value> void WriteMap(const std::map<Key, Value>& map, bool writeSize = true);
    template<typename T> void WriteArray(const std::vector<T>& array, bool writeSize = true);
};
```

**Key Features:**

- **Stream abstraction**: Allows different output targets (files, memory, network)
- **Type safety**: Template methods prevent type mismatches
- **Container support**: Built-in handling for STL containers
- **Buffer integration**: Direct Vulkan buffer serialization

### SerializeReader

Abstract base class providing binary data reading capabilities:

```cpp
class SerializeReader
{
public:
    virtual ~SerializeReader() = default;

    // Core interface - must be implemented by derived classes
    [[nodiscard]] virtual bool IsStreamGood() const = 0;
    virtual uint64_t GetStreamPosition() = 0;
    virtual void SetStreamPosition(uint64_t position) = 0;
    virtual bool ReadData(char* destination, size_t size) = 0;

    // Convenience methods
    explicit operator bool() const { return IsStreamGood(); }
  
    // Specialized read methods
    void ReadBuffer(Buffer& buffer, uint32_t size = 0);
    void ReadString(std::string& string);
  
    // Template methods for type-safe deserialization
    template<typename T> void ReadRaw(T& type);
    template<typename T> void ReadObject(T& obj);
    template<typename Key, typename Value> void ReadMap(std::map<Key, Value>& map, uint32_t size = 0);
    template<typename T> void ReadArray(std::vector<T>& array, uint32_t size = 0);
};
```

**Key Features:**

- **Symmetric API**: Mirrors writer interface for consistency
- **Size flexibility**: Optional size parameters for known-size data
- **Error handling**: Built-in stream validation
- **Container reconstruction**: Automatic resizing and population

## Serialization Implementation

### Template-Based Architecture

The serialization system uses a sophisticated template-based approach with multiple layers:

```cpp
namespace SceneryEditorX::Serialization
{
    // Top-level functions
    template <typename... Ts>
    static bool Serialize(SerializeWriter *writer, const Ts &...vs);
  
    template <typename... Ts>
    static bool Deserialize(SerializeReader *reader, Ts &...vs);
  
    namespace Impl
    {
        // Type-specific implementation routing
        template <typename T>
        static bool SerializeByType(SerializeWriter *writer, const T &v)
        {
            if constexpr (Types::Described<T>::value)
                return Types::Description<T>::Apply([&writer](const auto &...members) { 
                    return Serialize(writer, members...); 
                }, v);
            else if constexpr (Types::is_array_v<T>)
                return SerializeVec(writer, v);
            else
                return SerializeImpl(writer, v);
        }
    }
}
```

### Type Detection and Routing

The system automatically determines the appropriate serialization method:

1. **Described types**: Uses reflection system for member-wise serialization
2. **Array types**: Special handling for vectors and containers
3. **Trivial types**: Direct binary copy
4. **Custom types**: Falls back to intrusive interface or specializations

## SERIALIZABLE Macro System

### Basic Usage

The `SERIALIZABLE` macro provides automatic member-wise serialization:

```cpp
struct Transform
{
    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 rotation{0.0f, 0.0f, 0.0f};
    Vec3 scale{1.0f, 1.0f, 1.0f};
    bool visible = true;
    float opacity = 1.0f;
  
    void Reset() { *this = Transform{}; }
    bool IsIdentity() const { return position == Vec3{} && rotation == Vec3{} && scale == Vec3{1,1,1}; }
};

// Make the type serializable
SERIALIZABLE(Transform,
    &Transform::position,
    &Transform::rotation,
    &Transform::scale,
    &Transform::visible,
    &Transform::opacity
);
```

### Macro Expansion

The `SERIALIZABLE` macro expands to create a type description:

```cpp
#define SERIALIZABLE(Class, ...)                                                    \
    template <>                                                                     \
    struct SceneryEditorX::Types::Description<Class> : SceneryEditorX::Types::MemberList<__VA_ARGS__> \
    {};
```

This integration with the reflection system enables:

- **Automatic serialization**: No manual implementation required
- **Type safety**: Compile-time member validation
- **Reflection integration**: Seamless use with other reflection features

## Specializations and Custom Types

### Built-in Specializations

The system includes specializations for common types:

```cpp
namespace SceneryEditorX::Serialization::Impl
{
    // UUID specialization
    template <>
    static inline bool SerializeImpl(SerializeWriter *writer, const UUID &v)
    {
        writer->WriteRaw((uint64_t)v);
        return true;
    }
  
    // String specialization
    template <>
    static inline bool SerializeImpl(SerializeWriter *writer, const std::string &v)
    {
        writer->WriteString(v);
        return true;
    }
  
    // Identifier specialization
    template <>
    static inline bool SerializeImpl(SerializeWriter *writer, const Identifier &v)
    {
        writer->WriteRaw((uint32_t)v);
        return true;
    }
}
```

### Creating Custom Specializations

For types requiring custom serialization logic:

```cpp
namespace SceneryEditorX::Serialization::Impl
{
    template<>
    static inline bool SerializeImpl(SerializeWriter* writer, const CustomMatrix& matrix)
    {
        // Custom serialization for matrix data
        writer->WriteRaw(matrix.determinant);
        writer->WriteArray(matrix.elements);
        writer->WriteRaw(matrix.isIdentity);
        return true;
    }
  
    template<>
    static inline bool DeserializeImpl(SerializeReader* reader, CustomMatrix& matrix)
    {
        // Custom deserialization for matrix data
        reader->ReadRaw(matrix.determinant);
        reader->ReadArray(matrix.elements);
        reader->ReadRaw(matrix.isIdentity);
        matrix.RecalculateProperties(); // Custom post-load logic
        return true;
    }
}
```

### Intrusive Interface

Alternatively, types can provide their own serialization methods:

```cpp
class CustomObject
{
private:
    std::string m_Name;
    std::vector<float> m_Data;
    int m_Version;

public:
    static void Serialize(SerializeWriter* writer, const CustomObject& obj)
    {
        writer->WriteRaw(obj.m_Version);
        writer->WriteString(obj.m_Name);
        writer->WriteArray(obj.m_Data);
    }
  
    static void Deserialize(SerializeReader* reader, CustomObject& obj)
    {
        reader->ReadRaw(obj.m_Version);
        reader->ReadString(obj.m_Name);
        reader->ReadArray(obj.m_Data);
    }
};
```

## Container Support

### Automatic Container Serialization

The system provides built-in support for STL containers:

```cpp
// Vectors are automatically handled
std::vector<int> numbers = {1, 2, 3, 4, 5};
std::vector<std::string> names = {"Alice", "Bob", "Charlie"};
std::vector<Transform> transforms(10);

// Serialization automatically includes size information
Serialize(writer, numbers, names, transforms);

// Maps with automatic key-value serialization
std::map<std::string, float> properties;
properties["roughness"] = 0.5f;
properties["metallic"] = 0.0f;

// Unordered maps are also supported
std::unordered_map<uint32_t, Mesh> meshCache;

Serialize(writer, properties, meshCache);
```

### Custom Container Handling

For specialized containers, you can provide custom serialization:

```cpp
template<typename T>
class CircularBuffer
{
private:
    std::vector<T> m_Data;
    size_t m_Head = 0;
    size_t m_Size = 0;
  
public:
    static void Serialize(SerializeWriter* writer, const CircularBuffer<T>& buffer)
    {
        writer->WriteRaw(buffer.m_Head);
        writer->WriteRaw(buffer.m_Size);
        writer->WriteArray(buffer.m_Data);
    }
  
    static void Deserialize(SerializeReader* reader, CircularBuffer<T>& buffer)
    {
        reader->ReadRaw(buffer.m_Head);
        reader->ReadRaw(buffer.m_Size);
        reader->ReadArray(buffer.m_Data);
    }
};
```

## Usage Examples

### Basic Serialization

```cpp
#include <SceneryEditorX/serialization/serializer_impl.h>

// File-based writer implementation
class FileWriter : public SerializeWriter
{
private:
    std::ofstream m_Stream;
  
public:
    FileWriter(const std::string& filename) : m_Stream(filename, std::ios::binary) {}
  
    bool IsStreamGood() const override { return m_Stream.good(); }
    uint64_t GetStreamPosition() override { return m_Stream.tellp(); }
    void SetStreamPosition(uint64_t position) override { m_Stream.seekp(position); }
    bool WriteData(const char* data, size_t size) override 
    {
        m_Stream.write(data, size);
        return m_Stream.good();
    }
};

// Usage
void SaveSceneData()
{
    // Prepare data
    Transform cameraTransform;
    cameraTransform.position = {0.0f, 5.0f, 10.0f};
    cameraTransform.rotation = {-15.0f, 0.0f, 0.0f};
  
    std::vector<Transform> objectTransforms(100);
    std::map<std::string, float> settings;
    settings["fov"] = 60.0f;
    settings["nearPlane"] = 0.1f;
    settings["farPlane"] = 1000.0f;
  
    // Serialize to file
    FileWriter writer("scene_data.bin");
    if (writer)
    {
        using namespace SceneryEditorX::Serialization;
      
        bool success = Serialize(&writer, 
            cameraTransform,
            objectTransforms,
            settings
        );
      
        if (success) {
            SEDX_CORE_INFO_TAG("SERIALIZATION", "Scene data saved successfully");
        } else {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Failed to save scene data");
        }
    }
}
```

### Complex Object Serialization

```cpp
// Complex scene object with multiple data types
struct SceneObject
{
    UUID id;
    std::string name;
    Transform transform;
    std::vector<Ref<Material>> materials;
    std::map<std::string, std::string> metadata;
    bool isStatic = true;
    float lodDistance = 100.0f;
};

// Make it serializable
SERIALIZABLE(SceneObject,
    &SceneObject::id,
    &SceneObject::name,
    &SceneObject::transform,
    &SceneObject::materials,
    &SceneObject::metadata,
    &SceneObject::isStatic,
    &SceneObject::lodDistance
);

// Usage in scene serialization
void SaveCompleteScene()
{
    std::vector<SceneObject> sceneObjects;
    // ... populate scene objects ...
  
    FileWriter writer("complete_scene.bin");
    using namespace SceneryEditorX::Serialization;
  
    // Single call serializes entire scene
    Serialize(&writer, sceneObjects);
}
```

### Memory-Based Serialization

```cpp
// Memory buffer writer for in-memory serialization
class MemoryWriter : public SerializeWriter
{
private:
    std::vector<uint8_t> m_Buffer;
    size_t m_Position = 0;
  
public:
    bool IsStreamGood() const override { return true; }
    uint64_t GetStreamPosition() override { return m_Position; }
    void SetStreamPosition(uint64_t position) override { m_Position = position; }
  
    bool WriteData(const char* data, size_t size) override 
    {
        if (m_Position + size > m_Buffer.size()) {
            m_Buffer.resize(m_Position + size);
        }
        std::memcpy(m_Buffer.data() + m_Position, data, size);
        m_Position += size;
        return true;
    }
  
    const std::vector<uint8_t>& GetBuffer() const { return m_Buffer; }
    void Clear() { m_Buffer.clear(); m_Position = 0; }
};

// Usage for caching or network transmission
MemoryWriter memWriter;
Serialize(&memWriter, largeDataSet);

// Get serialized data
const auto& buffer = memWriter.GetBuffer();
// Send over network or cache...
```

### Deserialization Example

```cpp
// File-based reader implementation
class FileReader : public SerializeReader
{
private:
    std::ifstream m_Stream;
  
public:
    FileReader(const std::string& filename) : m_Stream(filename, std::ios::binary) {}
  
    bool IsStreamGood() const override { return m_Stream.good(); }
    uint64_t GetStreamPosition() override { return m_Stream.tellg(); }
    void SetStreamPosition(uint64_t position) override { m_Stream.seekg(position); }
    bool ReadData(char* destination, size_t size) override 
    {
        m_Stream.read(destination, size);
        return m_Stream.good();
    }
};

// Loading scene data
void LoadSceneData()
{
    FileReader reader("scene_data.bin");
    if (reader)
    {
        Transform cameraTransform;
        std::vector<Transform> objectTransforms;
        std::map<std::string, float> settings;
      
        using namespace SceneryEditorX::Serialization;
      
        bool success = Deserialize(&reader,
            cameraTransform,
            objectTransforms,
            settings
        );
      
        if (success) {
            SEDX_CORE_INFO_TAG("SERIALIZATION", "Scene data loaded successfully");
            SEDX_CORE_INFO_TAG("SERIALIZATION", "Loaded {} objects", objectTransforms.size());
        } else {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Failed to load scene data");
        }
    }
}
```

## Vulkan Buffer Integration

### Buffer Serialization

The system provides specialized support for Vulkan buffers:

```cpp
void SerializeWriter::WriteBuffer(Buffer buffer, bool writeSize)
{
    if (writeSize)
        WriteData((char*)&buffer.size, sizeof(uint64_t));
  
    WriteData((char*)buffer.data, buffer.size);
}

void SerializeReader::ReadBuffer(Buffer& buffer, uint32_t size)
{
    buffer.size = size;
    if (size == 0)
        ReadData((char*)&buffer.size, sizeof(uint64_t));
  
    buffer.Allocate(buffer.size);
    ReadData((char*)buffer.data, buffer.size);
}
```

### Usage with Graphics Resources

```cpp
struct MeshData
{
    Buffer vertexBuffer;
    Buffer indexBuffer;
    uint32_t vertexCount;
    uint32_t indexCount;
};

// Serialize mesh data including GPU buffers
void SaveMeshToFile(const MeshData& mesh, const std::string& filename)
{
    FileWriter writer(filename);
  
    writer.WriteRaw(mesh.vertexCount);
    writer.WriteRaw(mesh.indexCount);
    writer.WriteBuffer(mesh.vertexBuffer);
    writer.WriteBuffer(mesh.indexBuffer);
}

// Load mesh data and recreate GPU resources
MeshData LoadMeshFromFile(const std::string& filename)
{
    FileReader reader(filename);
    MeshData mesh;
  
    reader.ReadRaw(mesh.vertexCount);
    reader.ReadRaw(mesh.indexCount);
    reader.ReadBuffer(mesh.vertexBuffer);
    reader.ReadBuffer(mesh.indexBuffer);
  
    return mesh;
}
```

## Performance Considerations

### Optimization Strategies

1. **Binary Format**: All data is stored in efficient binary format
2. **Batch Operations**: Multiple values can be serialized in single calls
3. **Memory Mapping**: Large buffers can be memory-mapped for zero-copy operations
4. **Compile-time Optimization**: Template system enables aggressive optimization

### Memory Management

```cpp
// Efficient large data serialization
class PooledMemoryWriter : public SerializeWriter
{
private:
    MemoryPool& m_Pool;
    uint8_t* m_Buffer;
    size_t m_Capacity;
    size_t m_Position = 0;
  
public:
    PooledMemoryWriter(MemoryPool& pool, size_t initialCapacity) 
        : m_Pool(pool), m_Capacity(initialCapacity)
    {
        m_Buffer = m_Pool.Allocate(m_Capacity);
    }
  
    bool WriteData(const char* data, size_t size) override 
    {
        if (m_Position + size > m_Capacity) {
            // Grow buffer using pool
            size_t newCapacity = std::max(m_Capacity * 2, m_Position + size);
            uint8_t* newBuffer = m_Pool.Allocate(newCapacity);
            std::memcpy(newBuffer, m_Buffer, m_Position);
            m_Pool.Deallocate(m_Buffer);
            m_Buffer = newBuffer;
            m_Capacity = newCapacity;
        }
      
        std::memcpy(m_Buffer + m_Position, data, size);
        m_Position += size;
        return true;
    }
  
    // ... other methods
};
```

### Bulk Operations

```cpp
// Efficient bulk serialization
template<typename T>
void SerializeBulkData(SerializeWriter* writer, const std::vector<T>& data)
{
    if constexpr (std::is_trivial_v<T>) {
        // Direct memory copy for trivial types
        writer->WriteRaw<uint32_t>(data.size());
        writer->WriteData(reinterpret_cast<const char*>(data.data()), 
                         data.size() * sizeof(T));
    } else {
        // Use standard serialization for complex types
        using namespace SceneryEditorX::Serialization;
        Serialize(writer, data);
    }
}
```

## Error Handling and Validation

### Stream Validation

```cpp
// Robust serialization with error checking
template<typename... Args>
bool SafeSerialize(SerializeWriter* writer, const Args&... args)
{
    if (!writer || !writer->IsStreamGood()) {
        SEDX_CORE_ERROR_TAG("SERIALIZATION", "Invalid writer stream");
        return false;
    }
  
    uint64_t startPosition = writer->GetStreamPosition();
  
    try {
        using namespace SceneryEditorX::Serialization;
        bool success = Serialize(writer, args...);
      
        if (!success || !writer->IsStreamGood()) {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Serialization failed");
            writer->SetStreamPosition(startPosition); // Rollback
            return false;
        }
      
        return true;
    } catch (const std::exception& e) {
        SEDX_CORE_ERROR_TAG("SERIALIZATION", "Exception during serialization: {}", e.what());
        writer->SetStreamPosition(startPosition); // Rollback
        return false;
    }
}
```

### Version Handling

```cpp
// Versioned serialization for backward compatibility
struct VersionedData
{
    static constexpr uint32_t CURRENT_VERSION = 2;
  
    uint32_t version = CURRENT_VERSION;
    std::string name;
    std::vector<float> values;
    bool newFeature = false; // Added in version 2
  
    static void Serialize(SerializeWriter* writer, const VersionedData& data)
    {
        writer->WriteRaw(data.version);
        writer->WriteString(data.name);
        writer->WriteArray(data.values);
      
        if (data.version >= 2) {
            writer->WriteRaw(data.newFeature);
        }
    }
  
    static void Deserialize(SerializeReader* reader, VersionedData& data)
    {
        reader->ReadRaw(data.version);
        reader->ReadString(data.name);
        reader->ReadArray(data.values);
      
        if (data.version >= 2) {
            reader->ReadRaw(data.newFeature);
        } else {
            data.newFeature = false; // Default for older versions
        }
    }
};
```

## Integration with Module System

### Module-Based Serialization

```cpp
class SerializationModule : public Module
{
private:
    std::unique_ptr<SerializeWriter> m_Writer;
    std::unique_ptr<SerializeReader> m_Reader;
  
public:
    SerializationModule() : Module("SerializationModule") {}
  
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("SERIALIZATION", "Initializing serialization module");
      
        // Initialize serialization systems
        RegisterCustomSerializers();
        SetupFileHandlers();
    }
  
    template<typename... Args>
    bool SaveToFile(const std::string& filename, const Args&... data)
    {
        SEDX_PROFILE_SCOPE("SerializationModule::SaveToFile");
      
        auto writer = CreateRef<FileWriter>(filename);
        if (!writer || !*writer) {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Failed to open file for writing: {}", filename);
            return false;
        }
      
        using namespace SceneryEditorX::Serialization;
        return SafeSerialize(writer.get(), data...);
    }
  
    template<typename... Args>
    bool LoadFromFile(const std::string& filename, Args&... data)
    {
        SEDX_PROFILE_SCOPE("SerializationModule::LoadFromFile");
      
        auto reader = CreateRef<FileReader>(filename);
        if (!reader || !*reader) {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Failed to open file for reading: {}", filename);
            return false;
        }
      
        using namespace SceneryEditorX::Serialization;
        return SafeDeserialize(reader.get(), data...);
    }
  
private:
    void RegisterCustomSerializers()
    {
        // Register application-specific serializers
    }
  
    void SetupFileHandlers()
    {
        // Setup file format handlers
    }
};
```

## Best Practices

### 1. Use SERIALIZABLE for Simple Types

```cpp
// ✅ Good: Use SERIALIZABLE for straightforward data structures
struct MaterialProperties
{
    float roughness = 0.5f;
    float metallic = 0.0f;
    Vec3 albedo{1.0f, 1.0f, 1.0f};
    bool isTransparent = false;
};

SERIALIZABLE(MaterialProperties,
    &MaterialProperties::roughness,
    &MaterialProperties::metallic,
    &MaterialProperties::albedo,
    &MaterialProperties::isTransparent
);
```

### 2. Implement Custom Logic for Complex Types

```cpp
// ✅ Good: Custom serialization for types with complex state
class TextureManager
{
private:
    std::unordered_map<UUID, Ref<Texture>> m_LoadedTextures;
    std::string m_TextureDirectory;
  
public:
    static void Serialize(SerializeWriter* writer, const TextureManager& mgr)
    {
        writer->WriteString(mgr.m_TextureDirectory);
      
        // Only serialize texture paths, not loaded data
        std::vector<std::pair<UUID, std::string>> texturePaths;
        for (const auto& [id, texture] : mgr.m_LoadedTextures) {
            texturePaths.emplace_back(id, texture->GetFilePath());
        }
      
        writer->WriteArray(texturePaths);
    }
  
    static void Deserialize(SerializeReader* reader, TextureManager& mgr)
    {
        reader->ReadString(mgr.m_TextureDirectory);
      
        std::vector<std::pair<UUID, std::string>> texturePaths;
        reader->ReadArray(texturePaths);
      
        // Recreate texture references
        for (const auto& [id, path] : texturePaths) {
            mgr.LoadTexture(id, path);
        }
    }
};
```

### 3. Handle Pointers and References Carefully

```cpp
// ✅ Good: Serialize by ID/UUID for object references
struct SceneNode
{
    UUID id;
    std::string name;
    Transform transform;
    UUID parentId; // Instead of SceneNode* parent
    std::vector<UUID> childIds; // Instead of std::vector<SceneNode*> children
};

SERIALIZABLE(SceneNode,
    &SceneNode::id,
    &SceneNode::name,
    &SceneNode::transform,
    &SceneNode::parentId,
    &SceneNode::childIds
);
```

### 4. Version Your Data Structures

```cpp
// ✅ Good: Include version information for future compatibility
struct FileHeader
{
    char magic[4] = {'S', 'E', 'D', 'X'};
    uint32_t version = 1;
    uint64_t dataSize = 0;
    uint32_t checksum = 0;
};

SERIALIZABLE(FileHeader,
    &FileHeader::magic,
    &FileHeader::version,
    &FileHeader::dataSize,
    &FileHeader::checksum
);
```

### 5. Profile Critical Paths

```cpp
// ✅ Good: Profile serialization of large data sets
void SerializeLargeScene()
{
    SEDX_PROFILE_SCOPE("SerializeLargeScene");
  
    {
        SEDX_PROFILE_SCOPE("PrepareData");
        // Data preparation
    }
  
    {
        SEDX_PROFILE_SCOPE("WriteToFile");
        FileWriter writer("large_scene.bin");
        using namespace SceneryEditorX::Serialization;
        Serialize(&writer, sceneData);
    }
}
```

## Common Patterns

### Scene Serialization Pattern

```cpp
class Scene
{
private:
    std::vector<SceneObject> m_Objects;
    std::unordered_map<std::string, std::string> m_Metadata;
    Transform m_WorldTransform;
  
public:
    bool SaveToFile(const std::string& filename) const
    {
        FileWriter writer(filename);
        if (!writer) return false;
      
        // Write file header
        FileHeader header;
        header.dataSize = CalculateDataSize();
        writer.WriteRaw(header);
      
        // Write scene data
        using namespace SceneryEditorX::Serialization;
        return Serialize(&writer, m_WorldTransform, m_Objects, m_Metadata);
    }
  
    bool LoadFromFile(const std::string& filename)
    {
        FileReader reader(filename);
        if (!reader) return false;
      
        // Read and validate header
        FileHeader header;
        reader.ReadRaw(header);
        if (strncmp(header.magic, "SEDX", 4) != 0) {
            SEDX_CORE_ERROR_TAG("SCENE", "Invalid file format");
            return false;
        }
      
        // Read scene data
        using namespace SceneryEditorX::Serialization;
        return Deserialize(&reader, m_WorldTransform, m_Objects, m_Metadata);
    }
};
```

### Asset Cache Pattern

```cpp
template<typename T>
class SerializableCache
{
private:
    std::unordered_map<UUID, T> m_Cache;
    std::string m_CacheFile;
  
public:
    void SaveCache()
    {
        MemoryWriter writer;
      
        // Serialize all cached items
        using namespace SceneryEditorX::Serialization;
        Serialize(&writer, m_Cache);
      
        // Write to compressed file
        CompressAndWriteToFile(m_CacheFile, writer.GetBuffer());
    }
  
    void LoadCache()
    {
        auto buffer = ReadAndDecompressFromFile(m_CacheFile);
        if (buffer.empty()) return;
      
        MemoryReader reader(buffer);
      
        using namespace SceneryEditorX::Serialization;
        Deserialize(&reader, m_Cache);
    }
};
```

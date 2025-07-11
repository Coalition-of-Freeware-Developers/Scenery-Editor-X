# Scenery Editor X - Serialization Implementation Guide

---

## Overview

The `serializer_impl.h` file contains the core template-based serialization implementation for Scenery Editor X. This system provides automatic, type-safe binary serialization for both intrusive and non-intrusive approaches, with deep integration into the reflection system and support for complex data structures.

## Architecture Overview

### Design Principles

The serialization implementation is built on several key principles:

1. **Template-based dispatch**: Automatic type detection and routing
2. **Reflection integration**: Seamless use with `SERIALIZABLE` macro and `Types::Description`
3. **Extensibility**: Custom specializations for complex types
4. **Performance**: Zero-overhead abstractions and compile-time optimization
5. **Type safety**: Compile-time type checking and validation

### Core Components

```cpp
namespace SceneryEditorX::Serialization
{
    // Main serialization functions
    template <typename... Ts>
    static bool Serialize(SerializeWriter *writer, const Ts &...vs);
  
    template <typename... Ts>
    static bool Deserialize(SerializeReader *reader, Ts &...vs);
  
    namespace Impl
    {
        // Implementation details and type routing
        template <typename T>
        static bool SerializeByType(SerializeWriter *writer, const T &v);
      
        template <typename T>
        static bool DeserializeByType(SerializeReader *reader, T &v);
    }
}
```

## SERIALIZABLE Macro

### Macro Definition

```cpp
#define SERIALIZABLE(Class, ...)                                                    \
    template <>                                                                     \
    struct SceneryEditorX::Types::Description<Class> : SceneryEditorX::Types::MemberList<__VA_ARGS__> \
    {};
```

### Usage and Benefits

The `SERIALIZABLE` macro creates a type description that integrates with the reflection system:

```cpp
struct Transform
{
    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 rotation{0.0f, 0.0f, 0.0f};
    Vec3 scale{1.0f, 1.0f, 1.0f};
    bool visible = true;
    float opacity = 1.0f;
  
    // Non-serialized methods
    void Reset() { *this = Transform{}; }
    bool IsIdentity() const { return position == Vec3{} && rotation == Vec3{} && scale == Vec3{1,1,1}; }
};

// Enable automatic serialization for all listed members
SERIALIZABLE(Transform,
    &Transform::position,
    &Transform::rotation,
    &Transform::scale,
    &Transform::visible,
    &Transform::opacity
);

// Usage - automatic serialization without manual implementation
void SaveTransform(const Transform& transform)
{
    FileWriter writer("transform.bin");
    using namespace SceneryEditorX::Serialization;
  
    // Single call serializes all members automatically
    bool success = Serialize(&writer, transform);
}
```

### What the Macro Creates

The macro expands to create a specialization of `Types::Description`:

```cpp
template <>
struct SceneryEditorX::Types::Description<Transform> : SceneryEditorX::Types::MemberList<
    &Transform::position,
    &Transform::rotation,
    &Transform::scale,
    &Transform::visible,
    &Transform::opacity
>
{
    // Inherited from MemberList:
    // - TTuple type definition
    // - Apply() method for member iteration
    // - Type detection utilities
};
```

## Type Routing System

### SerializeByType Implementation

```cpp
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
```

### Type Detection Logic

The system uses compile-time type traits to determine the appropriate serialization method:

1. **Described Types** (`Types::Described<T>::value`):

   - Types with `SERIALIZABLE` macro applied
   - Uses reflection to serialize all listed members
   - Automatic member-wise serialization
2. **Array Types** (`Types::is_array_v<T>`):

   - `std::vector<T>` and similar containers
   - Special handling for size prefixes and element iteration
3. **Default Types**:

   - Falls back to `SerializeImpl` specializations
   - Handles trivial types and custom implementations

### Example Type Routing

```cpp
// Described type - uses reflection
struct Material { float roughness; Vec3 color; };
SERIALIZABLE(Material, &Material::roughness, &Material::color);

// Array type - uses vector serialization
std::vector<Material> materials;

// Trivial type - uses raw binary copy
int primitiveValue = 42;

// Custom type with specialization
UUID uniqueId;

// All handled automatically by type routing
Serialize(&writer, materials, primitiveValue, uniqueId);
```

## Built-in Specializations

### UUID Specialization

```cpp
namespace SceneryEditorX::Serialization::Impl
{
    template <>
    static inline bool SerializeImpl(SerializeWriter *writer, const UUID &v)
    {
        writer->WriteRaw(static_cast<uint64_t>(v));
        return true;
    }
  
    template <>
    static inline bool DeserializeImpl(SerializeReader *reader, UUID &v)
    {
        uint64_t data;
        reader->ReadRaw(data);
        v = UUID(data);
        return true;
    }
}
```

### String Specialization

```cpp
template <>
static inline bool SerializeImpl(SerializeWriter *writer, const std::string &v)
{
    writer->WriteString(v);
    return true;
}

template <>
static inline bool DeserializeImpl(SerializeReader *reader, std::string &v)
{
    reader->ReadString(v);
    return true;
}
```

### Identifier Specialization

```cpp
template <>
static inline bool SerializeImpl(SerializeWriter *writer, const Identifier &v)
{
    writer->WriteRaw(static_cast<uint32_t>(v));
    return true;
}

template <>
static inline bool DeserializeImpl(SerializeReader *reader, Identifier &v)
{
    uint32_t data;
    reader->ReadRaw(data);
    v = Identifier(data);
    return true;
}
```

## Vector Serialization

### SerializeVec Implementation

```cpp
template <typename T>
static bool SerializeVec(SerializeWriter *writer, const std::vector<T> &vec)
{
    // Write vector size first
    writer->WriteRaw<uint32_t>(static_cast<uint32_t>(vec.size()));

    // Serialize each element using recursive Serialize call
    for (const auto &element : vec)
        Serialize(writer, element);

    return true;
}
```

### DeserializeVec Implementation

```cpp
template <typename T>
static bool DeserializeVec(SerializeReader *reader, std::vector<T> &vec)
{
    uint32_t size = 0;
    reader->ReadRaw<uint32_t>(size);

    vec.resize(size);

    for (uint32_t i = 0; i < size; i++)
        Deserialize(reader, vec[i]);

    return true;
}
```

### Vector Usage Examples

```cpp
// Simple vector of primitives
std::vector<float> weights = {0.1f, 0.5f, 0.3f, 0.1f};
Serialize(&writer, weights);

// Vector of complex objects
std::vector<Transform> transforms(100);
for (auto& transform : transforms) {
    transform.position = RandomVec3();
    transform.rotation = RandomVec3();
}
Serialize(&writer, transforms);

// Nested vectors
std::vector<std::vector<int>> matrix = {{1,2,3}, {4,5,6}, {7,8,9}};
Serialize(&writer, matrix);
```

## Custom Specializations

### Creating Type Specializations

For types requiring custom serialization logic, create specializations in the `Impl` namespace:

```cpp
namespace SceneryEditorX::Serialization::Impl
{
    template<>
    static inline bool SerializeImpl(SerializeWriter* writer, const Matrix4& matrix)
    {
        // Custom serialization for 4x4 matrix
        for (int i = 0; i < 16; ++i) {
            writer->WriteRaw(matrix.data[i]);
        }
      
        // Optional: include computed properties
        writer->WriteRaw(matrix.IsIdentity());
        writer->WriteRaw(matrix.GetDeterminant());
      
        return true;
    }
  
    template<>
    static inline bool DeserializeImpl(SerializeReader* reader, Matrix4& matrix)
    {
        // Read matrix data
        for (int i = 0; i < 16; ++i) {
            reader->ReadRaw(matrix.data[i]);
        }
      
        // Read computed properties (or recalculate)
        bool isIdentity;
        float determinant;
        reader->ReadRaw(isIdentity);
        reader->ReadRaw(determinant);
      
        // Validate or recalculate
        matrix.ValidateProperties();
      
        return true;
    }
}
```

### Complex Object Specialization

```cpp
namespace SceneryEditorX::Serialization::Impl
{
    template<>
    static inline bool SerializeImpl(SerializeWriter* writer, const Texture& texture)
    {
        // Serialize metadata, not actual pixel data
        writer->WriteString(texture.GetFilePath());
        writer->WriteRaw(texture.GetWidth());
        writer->WriteRaw(texture.GetHeight());
        writer->WriteRaw(texture.GetFormat());
        writer->WriteRaw(texture.GetMipLevels());
      
        // Serialize settings
        const auto& settings = texture.GetSettings();
        writer->WriteRaw(settings.wrapS);
        writer->WriteRaw(settings.wrapT);
        writer->WriteRaw(settings.minFilter);
        writer->WriteRaw(settings.magFilter);
      
        return true;
    }
  
    template<>
    static inline bool DeserializeImpl(SerializeReader* reader, Texture& texture)
    {
        std::string filePath;
        uint32_t width, height, format, mipLevels;
      
        reader->ReadString(filePath);
        reader->ReadRaw(width);
        reader->ReadRaw(height);
        reader->ReadRaw(format);
        reader->ReadRaw(mipLevels);
      
        TextureSettings settings;
        reader->ReadRaw(settings.wrapS);
        reader->ReadRaw(settings.wrapT);
        reader->ReadRaw(settings.minFilter);
        reader->ReadRaw(settings.magFilter);
      
        // Recreate texture from file
        texture = Texture::LoadFromFile(filePath, settings);
      
        // Validate properties match
        SEDX_CORE_ASSERT(texture.GetWidth() == width, "Width mismatch");
        SEDX_CORE_ASSERT(texture.GetHeight() == height, "Height mismatch");
      
        return true;
    }
}
```

## Intrusive Interface

### Method Signature

Types can provide their own serialization by implementing static methods:

```cpp
class CustomObject
{
public:
    static void Serialize(SerializeWriter* writer, const CustomObject& obj);
    static void Deserialize(SerializeReader* reader, CustomObject& obj);
};
```

### Implementation Example

```cpp
class Mesh
{
private:
    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;
    std::string m_Name;
    AABB m_BoundingBox;
  
public:
    static void Serialize(SerializeWriter* writer, const Mesh& mesh)
    {
        SEDX_PROFILE_SCOPE("Mesh::Serialize");
      
        // Write metadata
        writer->WriteString(mesh.m_Name);
        writer->WriteRaw(mesh.m_BoundingBox);
      
        // Write geometry data
        writer->WriteArray(mesh.m_Vertices);
        writer->WriteArray(mesh.m_Indices);
    }
  
    static void Deserialize(SerializeReader* reader, Mesh& mesh)
    {
        SEDX_PROFILE_SCOPE("Mesh::Deserialize");
      
        // Read metadata
        reader->ReadString(mesh.m_Name);
        reader->ReadRaw(mesh.m_BoundingBox);
      
        // Read geometry data
        reader->ReadArray(mesh.m_Vertices);
        reader->ReadArray(mesh.m_Indices);
      
        // Post-load processing
        mesh.RecalculateBounds();
        mesh.GenerateNormals();
    }
};
```

## Advanced Usage Patterns

### Versioned Serialization

```cpp
class VersionedData
{
private:
    static constexpr uint32_t CURRENT_VERSION = 3;
  
    uint32_t m_Version = CURRENT_VERSION;
    std::string m_Name;
    std::vector<float> m_Values;
    bool m_NewFeature = false;       // Added in v2
    Transform m_Transform;           // Added in v3
  
public:
    static void Serialize(SerializeWriter* writer, const VersionedData& data)
    {
        writer->WriteRaw(data.m_Version);
        writer->WriteString(data.m_Name);
        writer->WriteArray(data.m_Values);
      
        if (data.m_Version >= 2) {
            writer->WriteRaw(data.m_NewFeature);
        }
      
        if (data.m_Version >= 3) {
            using namespace SceneryEditorX::Serialization;
            Serialize(writer, data.m_Transform);
        }
    }
  
    static void Deserialize(SerializeReader* reader, VersionedData& data)
    {
        reader->ReadRaw(data.m_Version);
        reader->ReadString(data.m_Name);
        reader->ReadArray(data.m_Values);
      
        if (data.m_Version >= 2) {
            reader->ReadRaw(data.m_NewFeature);
        } else {
            data.m_NewFeature = false; // Default for v1
        }
      
        if (data.m_Version >= 3) {
            using namespace SceneryEditorX::Serialization;
            Deserialize(reader, data.m_Transform);
        } else {
            data.m_Transform = Transform{}; // Default for v1-v2
        }
      
        // Update version to current
        data.m_Version = CURRENT_VERSION;
    }
};
```

### Conditional Serialization

```cpp
class ConditionalData
{
private:
    bool m_HasOptionalData = false;
    std::string m_RequiredField;
    std::vector<float> m_OptionalArray;
  
public:
    static void Serialize(SerializeWriter* writer, const ConditionalData& data)
    {
        writer->WriteString(data.m_RequiredField);
        writer->WriteRaw(data.m_HasOptionalData);
      
        if (data.m_HasOptionalData) {
            writer->WriteArray(data.m_OptionalArray);
        }
    }
  
    static void Deserialize(SerializeReader* reader, ConditionalData& data)
    {
        reader->ReadString(data.m_RequiredField);
        reader->ReadRaw(data.m_HasOptionalData);
      
        if (data.m_HasOptionalData) {
            reader->ReadArray(data.m_OptionalArray);
        } else {
            data.m_OptionalArray.clear();
        }
    }
};
```

### Polymorphic Serialization

```cpp
enum class ShapeType : uint32_t
{
    Circle = 1,
    Rectangle = 2,
    Triangle = 3
};

class Shape
{
protected:
    ShapeType m_Type;
  
public:
    virtual ~Shape() = default;
  
    static void Serialize(SerializeWriter* writer, const Shape& shape)
    {
        writer->WriteRaw(shape.m_Type);
      
        switch (shape.m_Type) {
            case ShapeType::Circle:
                SerializeCircle(writer, static_cast<const Circle&>(shape));
                break;
            case ShapeType::Rectangle:
                SerializeRectangle(writer, static_cast<const Rectangle&>(shape));
                break;
            case ShapeType::Triangle:
                SerializeTriangle(writer, static_cast<const Triangle&>(shape));
                break;
        }
    }
  
    static std::unique_ptr<Shape> Deserialize(SerializeReader* reader)
    {
        ShapeType type;
        reader->ReadRaw(type);
      
        switch (type) {
            case ShapeType::Circle:
                return DeserializeCircle(reader);
            case ShapeType::Rectangle:
                return DeserializeRectangle(reader);
            case ShapeType::Triangle:
                return DeserializeTriangle(reader);
            default:
                SEDX_CORE_ERROR_TAG("SERIALIZATION", "Unknown shape type: {}", static_cast<uint32_t>(type));
                return nullptr;
        }
    }
  
private:
    static void SerializeCircle(SerializeWriter* writer, const Circle& circle);
    static void SerializeRectangle(SerializeWriter* writer, const Rectangle& rect);
    static void SerializeTriangle(SerializeWriter* writer, const Triangle& tri);
  
    static std::unique_ptr<Circle> DeserializeCircle(SerializeReader* reader);
    static std::unique_ptr<Rectangle> DeserializeRectangle(SerializeReader* reader);
    static std::unique_ptr<Triangle> DeserializeTriangle(SerializeReader* reader);
};
```

## Performance Optimization

### Compile-time Optimizations

The template system enables aggressive compile-time optimization:

```cpp
// For trivial types, this compiles to direct memory copy
struct TrivialData
{
    float x, y, z;
    int id;
};

SERIALIZABLE(TrivialData, &TrivialData::x, &TrivialData::y, &TrivialData::z, &TrivialData::id);

// Compiler can optimize this to a single memory operation
std::vector<TrivialData> data(1000);
Serialize(&writer, data);
```

### Bulk Operations

For performance-critical code, consider bulk operations:

```cpp
namespace SceneryEditorX::Serialization::Impl
{
    template<>
    static inline bool SerializeImpl(SerializeWriter* writer, const std::vector<float>& vec)
    {
        writer->WriteRaw<uint32_t>(static_cast<uint32_t>(vec.size()));
      
        // Direct memory write for float arrays
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

## Error Handling

### Return Value Handling

All serialization functions return `bool` to indicate success:

```cpp
bool SaveComplexData(const ComplexData& data, const std::string& filename)
{
    FileWriter writer(filename);
    if (!writer) {
        SEDX_CORE_ERROR_TAG("SERIALIZATION", "Failed to open file: {}", filename);
        return false;
    }
  
    using namespace SceneryEditorX::Serialization;
  
    bool success = Serialize(&writer, data);
    if (!success) {
        SEDX_CORE_ERROR_TAG("SERIALIZATION", "Failed to serialize data");
        return false;
    }
  
    return true;
}
```

### Exception Safety

The serialization system is designed to be exception-safe:

```cpp
template<typename T>
bool SafeSerialize(SerializeWriter* writer, const T& data)
{
    uint64_t startPosition = writer->GetStreamPosition();
  
    try {
        using namespace SceneryEditorX::Serialization;
        return Serialize(writer, data);
    } catch (const std::exception& e) {
        SEDX_CORE_ERROR_TAG("SERIALIZATION", "Exception during serialization: {}", e.what());
      
        // Attempt to rollback
        try {
            writer->SetStreamPosition(startPosition);
        } catch (...) {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Failed to rollback stream position");
        }
      
        return false;
    }
}
```

## Integration Examples

### Module Integration

```cpp
class DataPersistenceModule : public Module
{
private:
    std::unordered_map<std::string, std::vector<uint8_t>> m_Cache;
  
public:
    template<typename T>
    bool SaveData(const std::string& key, const T& data)
    {
        SEDX_PROFILE_SCOPE("DataPersistenceModule::SaveData");
      
        MemoryWriter writer;
      
        using namespace SceneryEditorX::Serialization;
        bool success = Serialize(&writer, data);
      
        if (success) {
            m_Cache[key] = writer.GetBuffer();
            SEDX_CORE_INFO_TAG("PERSISTENCE", "Cached data for key: {}", key);
        }
      
        return success;
    }
  
    template<typename T>
    bool LoadData(const std::string& key, T& data)
    {
        SEDX_PROFILE_SCOPE("DataPersistenceModule::LoadData");
      
        auto it = m_Cache.find(key);
        if (it == m_Cache.end()) {
            SEDX_CORE_WARN_TAG("PERSISTENCE", "No cached data for key: {}", key);
            return false;
        }
      
        MemoryReader reader(it->second);
      
        using namespace SceneryEditorX::Serialization;
        return Deserialize(&reader, data);
    }
};
```

This implementation guide provides comprehensive coverage of the serialization system's capabilities and usage patterns, enabling developers to effectively utilize the binary serialization framework in Scenery Editor X.

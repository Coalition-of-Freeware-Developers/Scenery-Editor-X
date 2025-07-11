# GitHub Copilot Instructions - Serialization System

## Overview

This file provides GitHub Copilot with specific instructions for working with the Scenery Editor X serialization system. The serialization framework is a high-performance, binary data persistence system with both intrusive and non-intrusive capabilities, deep reflection integration, and extensive type safety features.

## Core System Components

### Primary Headers and Files

- `serializer.hpp` - Legacy JSON-based serialization (largely superseded)
- `serializer_writer.h/.cpp` - Abstract base class for binary data writing
- `serializer_reader.h/.cpp` - Abstract base class for binary data reading  
- `serializer_impl.h` - Template-based serialization implementation with type routing

### Integration Headers

The serialization system integrates with:
- `reflection/type_descriptors.h` - Reflection system integration via `SERIALIZABLE` macro
- `utils/types.h` - Type utilities and custom UUID/Identifier types
- `renderer/vulkan/vk_buffers.h` - Vulkan buffer serialization support

## Code Generation Patterns

### 1. Use SERIALIZABLE Macro for Simple Structures

When generating code for data structures that need serialization, prefer the `SERIALIZABLE` macro:

```cpp
struct YourDataStructure
{
    std::string name;
    Vec3 position;
    float value;
    bool isActive;
};

// Always add SERIALIZABLE macro after struct definition
SERIALIZABLE(YourDataStructure,
    &YourDataStructure::name,
    &YourDataStructure::position,
    &YourDataStructure::value,
    &YourDataStructure::isActive
);
```

**Rules:**
- List only data members that should be persisted
- Exclude computed/derived values
- Exclude raw pointers - use UUID references instead
- Order members logically (metadata first, then data)

### 2. Implement Intrusive Interface for Complex Types

For types requiring custom logic, implement the intrusive serialization interface:

```cpp
class ComplexType
{
public:
    static void Serialize(SerializeWriter* writer, const ComplexType& obj)
    {
        SEDX_PROFILE_SCOPE("ComplexType::Serialize");
        
        // Write version for future compatibility
        constexpr uint32_t version = 1;
        writer->WriteRaw(version);
        
        // Custom serialization logic
        writer->WriteString(obj.m_Name);
        writer->WriteArray(obj.m_Data);
        
        // Use reflection system for sub-objects when possible
        using namespace SceneryEditorX::Serialization;
        Serialize(writer, obj.m_SubObject);
    }
    
    static void Deserialize(SerializeReader* reader, ComplexType& obj)
    {
        SEDX_PROFILE_SCOPE("ComplexType::Deserialize");
        
        uint32_t version;
        reader->ReadRaw(version);
        
        if (version > 1) {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Unsupported version: {}", version);
            throw std::runtime_error("Unsupported version");
        }
        
        reader->ReadString(obj.m_Name);
        reader->ReadArray(obj.m_Data);
        
        using namespace SceneryEditorX::Serialization;
        Deserialize(reader, obj.m_SubObject);
        
        // Post-load validation and processing
        obj.ValidateData();
    }
};
```

### 3. Create Custom Writer/Reader Implementations

When implementing new writer/reader classes, inherit from the abstract base classes:

```cpp
class YourWriter : public SerializeWriter
{
public:
    explicit YourWriter(/* parameters */) { /* initialization */ }
    
    // Implement required virtual methods
    bool IsStreamGood() const override { /* implementation */ }
    uint64_t GetStreamPosition() override { /* implementation */ }
    void SetStreamPosition(uint64_t position) override { /* implementation */ }
    bool WriteData(const char* data, size_t size) override { /* implementation */ }
    
    // Add any custom methods specific to your writer
};
```

### 4. Handle Asset References Properly

Never serialize raw pointers or direct object references. Instead, use UUID-based references:

```cpp
// ❌ Don't do this
struct BadObject
{
    SomeAsset* assetPtr;  // Raw pointer - will cause crashes
    std::vector<OtherObject*> objects; // Raw pointers
};

// ✅ Do this instead
struct GoodObject
{
    UUID assetId;  // Reference by ID
    std::vector<UUID> objectIds; // Reference by IDs
    
    // Provide resolution methods
    Ref<SomeAsset> ResolveAsset() const 
    {
        return AssetManager::Get().LoadAsset<SomeAsset>(assetId);
    }
};

SERIALIZABLE(GoodObject,
    &GoodObject::assetId,
    &GoodObject::objectIds
);
```

## Naming Conventions

### Class and Method Patterns

- Writer classes: `*Writer` (e.g., `FileWriter`, `MemoryWriter`, `NetworkWriter`)
- Reader classes: `*Reader` (e.g., `FileReader`, `MemoryReader`, `NetworkReader`)
- Serialization methods: `Serialize(SerializeWriter*, const T&)`
- Deserialization methods: `Deserialize(SerializeReader*, T&)`

### File Naming

- Header files: `*_serialization.h` for serialization-specific headers
- Implementation files: `*_serialization.cpp`
- Data files: Use appropriate extensions (`.dat`, `.bin`, `.cache`, etc.)

## Error Handling Patterns

### Always Validate Stream State

```cpp
template<typename T>
bool SafeSerialize(SerializeWriter* writer, const T& data)
{
    if (!writer || !writer->IsStreamGood()) {
        SEDX_CORE_ERROR_TAG("SERIALIZATION", "Invalid writer stream");
        return false;
    }
    
    uint64_t startPosition = writer->GetStreamPosition();
    
    try {
        using namespace SceneryEditorX::Serialization;
        bool success = Serialize(writer, data);
        
        if (!success || !writer->IsStreamGood()) {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Serialization failed");
            writer->SetStreamPosition(startPosition); // Rollback
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        SEDX_CORE_ERROR_TAG("SERIALIZATION", "Exception: {}", e.what());
        writer->SetStreamPosition(startPosition); // Rollback
        return false;
    }
}
```

### Include Version Handling

Always include version information in custom serialization:

```cpp
// In Serialize method
constexpr uint32_t CURRENT_VERSION = 2;
writer->WriteRaw(CURRENT_VERSION);

// In Deserialize method  
uint32_t version;
reader->ReadRaw(version);

if (version > CURRENT_VERSION) {
    SEDX_CORE_ERROR_TAG("CLASS_NAME", "Unsupported version: {}", version);
    throw std::runtime_error("Unsupported file version");
}

// Handle different versions
if (version >= 2) {
    // Read new fields added in version 2
}
```

## Performance Guidelines

### Use Profiling Scopes

Always add profiling scopes to serialization methods:

```cpp
static void Serialize(SerializeWriter* writer, const LargeObject& obj)
{
    SEDX_PROFILE_SCOPE("LargeObject::Serialize");
    
    {
        SEDX_PROFILE_SCOPE("SerializeMetadata");
        // Metadata serialization
    }
    
    {
        SEDX_PROFILE_SCOPE("SerializeBulkData");
        // Large data serialization
    }
}
```

### Optimize for Bulk Data

For large arrays of trivial types, use bulk operations:

```cpp
namespace SceneryEditorX::Serialization::Impl
{
    template<>
    static inline bool SerializeImpl(SerializeWriter* writer, const std::vector<float>& vec)
    {
        writer->WriteRaw<uint32_t>(static_cast<uint32_t>(vec.size()));
        
        if (!vec.empty()) {
            // Single memory operation for trivial types
            writer->WriteData(
                reinterpret_cast<const char*>(vec.data()),
                vec.size() * sizeof(float)
            );
        }
        
        return true;
    }
}
```

## Integration with Module System

### Module-Based Serialization

When creating modules that handle serialization:

```cpp
class YourSerializationModule : public Module
{
public:
    explicit YourSerializationModule() : Module("YourSerializationModule") {}
    
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("YOUR_MODULE", "Initializing serialization module");
        // Setup serialization systems
    }
    
    void OnDetach() override
    {
        SEDX_CORE_INFO_TAG("YOUR_MODULE", "Saving cached data");
        // Save any pending data
    }
    
    template<typename T>
    bool SaveData(const std::string& key, const T& data)
    {
        SEDX_PROFILE_SCOPE("YourSerializationModule::SaveData");
        
        // Implementation using serialization system
        FileWriter writer(GetDataPath(key));
        using namespace SceneryEditorX::Serialization;
        return Serialize(&writer, data);
    }
};
```

## Logging Integration

### Use Tagged Logging

Always use tagged logging in serialization code:

```cpp
// For general serialization operations
SEDX_CORE_INFO_TAG("SERIALIZATION", "Saved {} objects", objectCount);
SEDX_CORE_ERROR_TAG("SERIALIZATION", "Failed to open file: {}", filename);

// For specific subsystems
SEDX_CORE_INFO_TAG("ASSET_CACHE", "Cached asset {} ({} bytes)", id, size);
SEDX_CORE_WARN_TAG("MESH_LOADER", "Mesh has invalid indices, fixing automatically");
```

## Memory Management

### Use Custom Smart Pointers

Always use the custom smart pointer system:

```cpp
// Creation
auto writer = CreateRef<FileWriter>("output.dat");
auto reader = CreateRef<FileReader>("input.dat");

// Storage
Ref<SerializeWriter> m_Writer;
Ref<SerializeReader> m_Reader;

// Weak references for non-owning relationships
WeakRef<SerializeWriter> m_CachedWriter;
```

## Common Use Cases

### File Format Headers

```cpp
struct FileHeader
{
    char magic[4];      // File type identifier
    uint32_t version;   // Format version
    uint64_t dataSize;  // Size of data section
    uint32_t checksum;  // Data integrity check
};

SERIALIZABLE(FileHeader,
    &FileHeader::magic,
    &FileHeader::version,
    &FileHeader::dataSize,
    &FileHeader::checksum
);
```

### Asset Caching

```cpp
template<typename T>
class AssetCache
{
    std::unordered_map<UUID, std::vector<uint8_t>> m_Cache;
    
public:
    void CacheAsset(const UUID& id, const T& asset)
    {
        MemoryWriter writer;
        using namespace SceneryEditorX::Serialization;
        
        if (Serialize(&writer, asset)) {
            m_Cache[id] = writer.GetBuffer();
        }
    }
    
    std::optional<T> LoadAsset(const UUID& id)
    {
        auto it = m_Cache.find(id);
        if (it == m_Cache.end()) return std::nullopt;
        
        MemoryReader reader(it->second);
        T asset;
        
        using namespace SceneryEditorX::Serialization;
        if (Deserialize(&reader, asset)) {
            return asset;
        }
        
        return std::nullopt;
    }
};
```

## Integration Rules

1. **Always use the namespace**: `using namespace SceneryEditorX::Serialization;`
2. **Include proper headers**: Include `serializer_impl.h` for template functions
3. **Follow module patterns**: Integrate with the Module system for lifecycle management
4. **Use smart pointers**: Use the custom smart pointer system, not raw pointers
5. **Add profiling**: Include profiling scopes in performance-critical serialization code
6. **Tag logging**: Use appropriate tags for logging messages
7. **Validate data**: Always validate data during deserialization
8. **Handle versions**: Include version handling for forward compatibility

## Anti-Patterns to Avoid

❌ **Don't serialize computed values**
❌ **Don't use raw pointers in serialized data**
❌ **Don't ignore stream validation**
❌ **Don't forget version information**
❌ **Don't use platform-dependent types**
❌ **Don't serialize temporary/cache data**
❌ **Don't implement manual serialization when SERIALIZABLE suffices**

This instruction set ensures that GitHub Copilot generates code that follows Scenery Editor X serialization system conventions and best practices.

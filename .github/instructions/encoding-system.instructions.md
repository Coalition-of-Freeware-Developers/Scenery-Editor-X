# GitHub Copilot Instructions for Encoding System

## Overview

The SceneryEditorX encoding system provides essential utilities for data transformation, hashing, and unique identification throughout the application. This system is located in `SceneryEditorX/core/encoding.hpp` and `SceneryEditorX/core/encoding.cpp` and follows the project's module-based architecture and coding standards.

## Core Components and Usage

### Base64 Encoding Operations

**Always use the SceneryEditorX encoding system for Base64 operations:**

```cpp
// Encoding binary data
const unsigned char* binaryData = /* your binary data */;
size_t dataSize = /* data size */;
std::string encoded = SceneryEditorX::Encoding::EncodeBase64(binaryData, dataSize);

// Decoding Base64 strings
std::string base64String = "SGVsbG8gV29ybGQ=";
std::vector<uint8_t> decoded = SceneryEditorX::Encoding::DecodeBase64(base64String);
```

**Use Base64 encoding for:**
- Asset serialization in configuration files
- Network protocol payloads
- Binary data storage in text-based formats
- Scene file metadata
- Texture atlas metadata

### Hash Operations and UUID Generation

**Use HashCombine for building composite hashes:**

```cpp
uint32_t hash = 0;
SceneryEditorX::Encoding::HashCombine(hash, objectName);
SceneryEditorX::Encoding::HashCombine(hash, position.x);
SceneryEditorX::Encoding::HashCombine(hash, position.y);
SceneryEditorX::Encoding::HashCombine(hash, position.z);

// For memory blocks
SceneryEditorX::Encoding::HashCombine(hash, &structure, sizeof(structure));
```

**Use HashUUID for deterministic ID generation:**

```cpp
std::vector<uint32_t> sceneData = {objectType, materialHash, meshHash};
uint32_t sceneHash = SceneryEditorX::Encoding::HashUUID(sceneData);
SceneryEditorX::UUID sceneId = {static_cast<uint64_t>(sceneHash)};
```

### UUID Structure Usage

**Always use the UUID structure for unique identifiers:**

```cpp
struct SceneObject
{
    SceneryEditorX::UUID m_ID;
    
    void GenerateID()
    {
        uint32_t hash = 0;
        SceneryEditorX::Encoding::HashCombine(hash, GetName());
        SceneryEditorX::Encoding::HashCombine(hash, GetTransform());
        m_ID.ID = static_cast<uint64_t>(hash);
    }
};
```

## Implementation Patterns

### Module Integration Pattern

**When creating modules that use encoding operations:**

```cpp
class EncodingAwareModule : public Module
{
public:
    explicit EncodingAwareModule() : Module("EncodingAwareModule") {}
    
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("ENCODING", "Initializing encoding-aware module");
        
        // Validate encoding system availability
        TestEncodingOperations();
        
        SEDX_CORE_INFO_TAG("ENCODING", "Module ready with encoding support");
    }
    
private:
    void TestEncodingOperations()
    {
        // Test Base64 round-trip
        const std::string testData = "TestData";
        auto encoded = SceneryEditorX::Encoding::EncodeBase64(
            reinterpret_cast<const unsigned char*>(testData.c_str()), 
            testData.length()
        );
        auto decoded = SceneryEditorX::Encoding::DecodeBase64(encoded);
        
        SEDX_CORE_ASSERT(!encoded.empty(), "Base64 encoding failed");
        SEDX_CORE_ASSERT(!decoded.empty(), "Base64 decoding failed");
    }
};
```

### Asset Serialization Pattern

**For asset management with encoding:**

```cpp
class AssetManager
{
public:
    std::string SerializeAsset(const Ref<Asset>& asset)
    {
        SEDX_PROFILE_SCOPE("AssetManager::SerializeAsset");
        
        try
        {
            std::vector<uint8_t> binaryData = asset->ToBinary();
            
            if (binaryData.empty())
            {
                SEDX_CORE_WARN_TAG("ASSET", "Asset {} produced empty binary data", 
                                   asset->GetName());
                return "";
            }
            
            return SceneryEditorX::Encoding::EncodeBase64(
                binaryData.data(), binaryData.size()
            );
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("ASSET", "Failed to serialize asset: {}", e.what());
            return "";
        }
    }
    
    Ref<Asset> DeserializeAsset(const std::string& encodedData, AssetType type)
    {
        SEDX_PROFILE_SCOPE("AssetManager::DeserializeAsset");
        
        if (encodedData.empty())
        {
            SEDX_CORE_WARN_TAG("ASSET", "Cannot deserialize empty data");
            return nullptr;
        }
        
        try
        {
            auto binaryData = SceneryEditorX::Encoding::DecodeBase64(encodedData);
            return Asset::FromBinary(binaryData, type);
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("ASSET", "Failed to deserialize asset: {}", e.what());
            return nullptr;
        }
    }
};
```

### Caching with Hash-based Keys

**For resource caching systems:**

```cpp
class ResourceCache
{
private:
    std::unordered_map<uint32_t, Ref<Resource>> m_Cache;
    
public:
    uint32_t CalculateResourceHash(const std::string& path, 
                                  const LoadParameters& params)
    {
        uint32_t hash = 0;
        
        SceneryEditorX::Encoding::HashCombine(hash, path);
        SceneryEditorX::Encoding::HashCombine(hash, &params, sizeof(params));
        
        // Include file modification time if available
        try
        {
            auto fileTime = std::filesystem::last_write_time(path);
            auto timeValue = fileTime.time_since_epoch().count();
            SceneryEditorX::Encoding::HashCombine(hash, timeValue);
        }
        catch (const std::filesystem::filesystem_error&)
        {
            SEDX_CORE_WARN_TAG("CACHE", "Could not get file time for: {}", path);
        }
        
        return hash;
    }
    
    Ref<Resource> GetOrLoad(const std::string& path, const LoadParameters& params)
    {
        uint32_t hash = CalculateResourceHash(path, params);
        
        auto it = m_Cache.find(hash);
        if (it != m_Cache.end())
        {
            SEDX_CORE_INFO_TAG("CACHE", "Cache hit for resource: {}", path);
            return it->second;
        }
        
        auto resource = LoadResource(path, params);
        if (resource)
        {
            m_Cache[hash] = resource;
            SEDX_CORE_INFO_TAG("CACHE", "Cached resource: {} (hash: {})", path, hash);
        }
        
        return resource;
    }
};
```

## Error Handling and Validation

### Input Validation Pattern

**Always validate inputs before encoding operations:**

```cpp
bool ValidateBase64Input(const std::string& input)
{
    if (input.empty()) return false;
    
    size_t paddingCount = 0;
    for (char c : input)
    {
        if (c == '=') 
        {
            paddingCount++;
        }
        else if (paddingCount > 0) 
        {
            // Padding should only be at end
            return false;
        }
    }
    
    return paddingCount <= 2; // Maximum 2 padding characters
}

std::vector<uint8_t> SafeDecodeBase64(const std::string& input)
{
    if (!ValidateBase64Input(input))
    {
        SEDX_CORE_WARN_TAG("ENCODING", "Invalid Base64 input detected");
        return {};
    }
    
    return SceneryEditorX::Encoding::DecodeBase64(input);
}
```

### Memory Safety Pattern

**For safe memory operations with hashing:**

```cpp
template<typename T>
uint32_t SafeHashStruct(const T& structure)
{
    static_assert(std::is_trivially_copyable_v<T>, 
                 "Type must be trivially copyable for safe hashing");
    
    uint32_t hash = 0;
    SceneryEditorX::Encoding::HashCombine(hash, 
        const_cast<T*>(&structure), sizeof(T));
    return hash;
}
```

## Performance Guidelines

### Efficient Base64 Operations

**Pre-allocate output buffers when size is known:**

```cpp
std::string EncodeWithReserve(const std::vector<uint8_t>& data)
{
    std::string result;
    result.reserve((data.size() + 2) / 3 * 4); // Base64 expansion factor
    
    return SceneryEditorX::Encoding::EncodeBase64(data.data(), data.size());
}
```

### Batch Hash Operations

**Minimize function call overhead:**

```cpp
uint32_t HashMultipleValues(const std::vector<std::string>& values)
{
    uint32_t hash = 0;
    for (const auto& value : values)
    {
        SceneryEditorX::Encoding::HashCombine(hash, value);
    }
    return hash;
}
```

### Streaming Hash for Large Data

**For large data structures:**

```cpp
class StreamingHasher
{
private:
    uint32_t m_Hash = 0;
    
public:
    void Update(const void* data, size_t size)
    {
        SceneryEditorX::Encoding::HashCombine(m_Hash, 
            const_cast<void*>(data), static_cast<uint32_t>(size));
    }
    
    uint32_t Finalize() { return m_Hash; }
    void Reset() { m_Hash = 0; }
};
```

## Integration with Other Systems

### Serialization System Integration

**Use encoding with the serialization system:**

```cpp
class SerializationHelper
{
public:
    template<typename T>
    std::string SerializeToBase64(const T& object)
    {
        SerializerWriter writer;
        writer.Write(object);
        
        const auto& data = writer.GetData();
        return SceneryEditorX::Encoding::EncodeBase64(data.data(), data.size());
    }
    
    template<typename T>
    T DeserializeFromBase64(const std::string& encodedData)
    {
        auto binaryData = SceneryEditorX::Encoding::DecodeBase64(encodedData);
        
        SerializerReader reader(binaryData);
        T object;
        reader.Read(object);
        return object;
    }
};
```

### Settings System Integration

**For binary settings with encoding:**

```cpp
class SettingsManager
{
public:
    template<typename SettingsType>
    bool SaveBinarySettings(const std::string& filename, const SettingsType& settings)
    {
        try
        {
            const uint8_t* settingsBytes = reinterpret_cast<const uint8_t*>(&settings);
            std::string encoded = SceneryEditorX::Encoding::EncodeBase64(
                settingsBytes, sizeof(SettingsType)
            );
            
            std::ofstream file(filename);
            file << "settings_data=" << encoded << std::endl;
            
            SEDX_CORE_INFO_TAG("SETTINGS", "Saved binary settings to {}", filename);
            return true;
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("SETTINGS", "Failed to save settings: {}", e.what());
            return false;
        }
    }
};
```

## Testing Patterns

### Unit Test Structure

**Always include round-trip tests:**

```cpp
TEST_CASE("Encoding System Tests", "[encoding]")
{
    SECTION("Base64 round-trip")
    {
        const std::string original = "SceneryEditorX Test Data";
        const auto* input = reinterpret_cast<const unsigned char*>(original.c_str());
        
        std::string encoded = SceneryEditorX::Encoding::EncodeBase64(input, original.length());
        REQUIRE(!encoded.empty());
        
        std::vector<uint8_t> decoded = SceneryEditorX::Encoding::DecodeBase64(encoded);
        std::string result(decoded.begin(), decoded.end());
        
        REQUIRE(result == original);
    }
    
    SECTION("Hash consistency")
    {
        uint32_t hash1 = 0, hash2 = 0;
        
        SceneryEditorX::Encoding::HashCombine(hash1, std::string("test"));
        SceneryEditorX::Encoding::HashCombine(hash1, 42);
        
        SceneryEditorX::Encoding::HashCombine(hash2, std::string("test"));
        SceneryEditorX::Encoding::HashCombine(hash2, 42);
        
        REQUIRE(hash1 == hash2);
    }
}
```

## Key Reminders

1. **Always use SceneryEditorX::Encoding** for all encoding operations
2. **Use HashCombine for incremental hash building** instead of manual hash calculations
3. **Include error handling and logging** with appropriate tags (ENCODING, ASSET, CACHE, etc.)
4. **Validate inputs** before performing encoding/decoding operations
5. **Use UUID structure** for all unique identifiers
6. **Follow RAII principles** with smart pointers for memory management
7. **Include profiling scopes** for performance-critical encoding operations
8. **Use tagged logging** for better debugging and monitoring
9. **Test round-trip operations** to ensure data integrity
10. **Consider memory efficiency** for large data operations

The encoding system is thread-safe and designed for high-performance operations throughout the SceneryEditorX application. Always prefer the provided encoding utilities over custom implementations to maintain consistency and reliability.

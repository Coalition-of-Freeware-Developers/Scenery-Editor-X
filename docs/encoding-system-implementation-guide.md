# Scenery Editor X - Encoding System Implementation Guide

---

## Implementation Overview

This guide provides detailed implementation examples and best practices for using the SceneryEditorX encoding system. The encoding system is implemented in `encoding.hpp` and `encoding.cpp` within the `SceneryEditorX::core` namespace.

---

## File Structure and Dependencies

### Header File (`encoding.hpp`)

```cpp
#pragma once
#include <string>
#include <vector>

namespace SceneryEditorX
{
    struct UUID { uint64_t ID; };
  
    class Encoding
    {
    public:
        GLOBAL uint64_t ID;
      
        // Base64 operations
        GLOBAL std::string EncodeBase64(const unsigned char *input, size_t len);
        GLOBAL std::vector<uint8_t> DecodeBase64(const std::string &input);
      
        // Hashing operations
        GLOBAL uint32_t HashUUID(const std::vector<uint32_t> &vec);
      
        template <typename T>
        void HashCombine(uint32_t &h, const T &v);
      
        GLOBAL void HashCombine(uint32_t &h, void *ptr, const uint32_t size);
    };
}
```

### Implementation File (`encoding.cpp`)

The implementation uses standard library algorithms and follows the project's coding standards with proper error handling and performance optimization.

---

## Detailed Implementation Examples

### 1. Asset Serialization Framework

#### Binary Asset Serialization

```cpp
#include <SceneryEditorX/core/encoding.hpp>
#include <SceneryEditorX/core/pointers.h>
#include <SceneryEditorX/logging/logging.hpp>

class AssetSerializer : public Module
{
public:
    explicit AssetSerializer() : Module("AssetSerializer") {}
  
    /**
     * @brief Serializes a 3D model asset to Base64 encoded string
     * @param asset The asset to serialize
     * @return Base64 encoded asset data
     */
    std::string SerializeAsset(const Ref<Asset>& asset)
    {
        SEDX_PROFILE_SCOPE("AssetSerializer::SerializeAsset");
      
        try
        {
            // Convert asset to binary format
            std::vector<uint8_t> binaryData = asset->ToBinary();
          
            if (binaryData.empty())
            {
                SEDX_CORE_WARN_TAG("SERIALIZATION", "Asset {} produced empty binary data", 
                                   asset->GetName());
                return "";
            }
          
            // Encode to Base64
            std::string encoded = SceneryEditorX::Encoding::EncodeBase64(
                binaryData.data(), binaryData.size()
            );
          
            SEDX_CORE_INFO_TAG("SERIALIZATION", "Serialized asset {} ({} bytes -> {} chars)",
                              asset->GetName(), binaryData.size(), encoded.size());
          
            return encoded;
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Failed to serialize asset {}: {}", 
                               asset->GetName(), e.what());
            return "";
        }
    }
  
    /**
     * @brief Deserializes a Base64 encoded asset string
     * @param encodedData Base64 encoded asset data
     * @param assetType Type of asset to create
     * @return Deserialized asset or nullptr on failure
     */
    Ref<Asset> DeserializeAsset(const std::string& encodedData, AssetType assetType)
    {
        SEDX_PROFILE_SCOPE("AssetSerializer::DeserializeAsset");
      
        if (encodedData.empty())
        {
            SEDX_CORE_WARN_TAG("SERIALIZATION", "Cannot deserialize empty encoded data");
            return nullptr;
        }
      
        try
        {
            // Decode from Base64
            std::vector<uint8_t> binaryData = 
                SceneryEditorX::Encoding::DecodeBase64(encodedData);
          
            if (binaryData.empty())
            {
                SEDX_CORE_ERROR_TAG("SERIALIZATION", "Base64 decoding produced empty result");
                return nullptr;
            }
          
            // Create asset from binary data
            auto asset = Asset::FromBinary(binaryData, assetType);
          
            if (asset)
            {
                SEDX_CORE_INFO_TAG("SERIALIZATION", "Deserialized asset ({} chars -> {} bytes)",
                                  encodedData.size(), binaryData.size());
            }
          
            return asset;
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Failed to deserialize asset: {}", e.what());
            return nullptr;
        }
    }
};
```

#### Scene File Format Integration

```cpp
class SceneFileManager
{
private:
    static constexpr const char* SCENE_FILE_HEADER = "SEDX_SCENE_V1";
  
public:
    struct SceneHeader
    {
        char signature[16];
        uint32_t version;
        uint32_t assetCount;
        uint64_t timestamp;
        uint32_t headerHash;
    };
  
    bool SaveScene(const std::string& filename, const Ref<Scene>& scene)
    {
        try
        {
            std::ofstream file(filename, std::ios::binary);
            if (!file.is_open())
            {
                SEDX_CORE_ERROR_TAG("SCENE", "Failed to open file for writing: {}", filename);
                return false;
            }
          
            // Create and write header
            SceneHeader header = CreateSceneHeader(scene);
            file.write(reinterpret_cast<const char*>(&header), sizeof(SceneHeader));
          
            // Serialize each asset
            const auto& assets = scene->GetAssets();
            for (const auto& asset : assets)
            {
                std::string encodedAsset = SerializeAssetForFile(asset);
              
                // Write asset size and data
                uint32_t assetSize = static_cast<uint32_t>(encodedAsset.size());
                file.write(reinterpret_cast<const char*>(&assetSize), sizeof(uint32_t));
                file.write(encodedAsset.c_str(), encodedAsset.size());
            }
          
            SEDX_CORE_INFO_TAG("SCENE", "Successfully saved scene to {}", filename);
            return true;
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("SCENE", "Exception while saving scene: {}", e.what());
            return false;
        }
    }
  
private:
    SceneHeader CreateSceneHeader(const Ref<Scene>& scene)
    {
        SceneHeader header = {};
      
        // Copy signature
        std::strncpy(header.signature, SCENE_FILE_HEADER, sizeof(header.signature));
      
        header.version = 1;
        header.assetCount = static_cast<uint32_t>(scene->GetAssets().size());
        header.timestamp = static_cast<uint64_t>(std::time(nullptr));
      
        // Calculate header hash (excluding the hash field itself)
        uint32_t hash = 0;
        SceneryEditorX::Encoding::HashCombine(hash, &header, 
            sizeof(SceneHeader) - sizeof(uint32_t));
        header.headerHash = hash;
      
        return header;
    }
  
    std::string SerializeAssetForFile(const Ref<Asset>& asset)
    {
        // Create asset metadata
        AssetMetadata metadata;
        metadata.type = asset->GetType();
        metadata.nameLength = static_cast<uint32_t>(asset->GetName().size());
        metadata.dataSize = asset->GetDataSize();
      
        // Calculate metadata hash
        uint32_t metadataHash = 0;
        SceneryEditorX::Encoding::HashCombine(metadataHash, &metadata, sizeof(AssetMetadata));
        SceneryEditorX::Encoding::HashCombine(metadataHash, asset->GetName());
      
        // Encode asset data
        std::vector<uint8_t> assetData = asset->ToBinary();
        std::string encodedData = SceneryEditorX::Encoding::EncodeBase64(
            assetData.data(), assetData.size()
        );
      
        // Combine metadata and encoded data
        std::stringstream result;
        result.write(reinterpret_cast<const char*>(&metadata), sizeof(AssetMetadata));
        result.write(reinterpret_cast<const char*>(&metadataHash), sizeof(uint32_t));
        result << asset->GetName();
        result << encodedData;
      
        return result.str();
    }
};
```

### 2. Advanced Hashing Implementations

#### Hierarchical Scene Object Identification

```cpp
class SceneObjectIDManager
{
private:
    std::unordered_map<uint64_t, WeakRef<SceneObject>> m_ObjectRegistry;
    uint64_t m_NextID = 1;
  
public:
    /**
     * @brief Generates a deterministic ID based on object properties
     * @param object The scene object to generate ID for
     * @return Unique 64-bit identifier
     */
    SceneryEditorX::UUID GenerateDeterministicID(const Ref<SceneObject>& object)
    {
        uint32_t hash = 0;
      
        // Hash object hierarchy path
        std::string hierarchyPath = BuildHierarchyPath(object);
        SceneryEditorX::Encoding::HashCombine(hash, hierarchyPath);
      
        // Hash transformation matrix
        const auto& transform = object->GetTransform();
    SceneryEditorX::Encoding::HashCombine(hash, &transform, sizeof(Mat4));
      
        // Hash object type and properties
        SceneryEditorX::Encoding::HashCombine(hash, static_cast<uint32_t>(object->GetType()));
      
        // Hash material properties if available
        if (auto material = object->GetMaterial())
        {
            uint32_t materialHash = HashMaterial(material);
            SceneryEditorX::Encoding::HashCombine(hash, materialHash);
        }
      
        // Hash mesh data if available
        if (auto mesh = object->GetMesh())
        {
            uint32_t meshHash = HashMesh(mesh);
            SceneryEditorX::Encoding::HashCombine(hash, meshHash);
        }
      
        // Convert to 64-bit UUID
        SceneryEditorX::UUID id;
        id.ID = static_cast<uint64_t>(hash) | (static_cast<uint64_t>(GetTimestamp()) << 32);
      
        return id;
    }
  
    /**
     * @brief Generates a simple sequential ID for runtime objects
     * @return Sequential 64-bit identifier
     */
    SceneryEditorX::UUID GenerateSequentialID()
    {
        SceneryEditorX::UUID id;
        id.ID = m_NextID++;
        return id;
    }
  
private:
    std::string BuildHierarchyPath(const Ref<SceneObject>& object)
    {
        std::vector<std::string> pathComponents;
        auto current = object;
      
        while (current)
        {
            pathComponents.push_back(current->GetName());
            current = current->GetParent();
        }
      
        // Reverse to get root-to-leaf path
        std::reverse(pathComponents.begin(), pathComponents.end());
      
        std::string path;
        for (size_t i = 0; i < pathComponents.size(); ++i)
        {
            if (i > 0) path += "/";
            path += pathComponents[i];
        }
      
        return path;
    }
  
    uint32_t HashMaterial(const Ref<Material>& material)
    {
        uint32_t hash = 0;
      
        const auto& props = material->GetProperties();
        SceneryEditorX::Encoding::HashCombine(hash, props.albedo.r);
        SceneryEditorX::Encoding::HashCombine(hash, props.albedo.g);
        SceneryEditorX::Encoding::HashCombine(hash, props.albedo.b);
        SceneryEditorX::Encoding::HashCombine(hash, props.albedo.a);
        SceneryEditorX::Encoding::HashCombine(hash, props.metallic);
        SceneryEditorX::Encoding::HashCombine(hash, props.roughness);
        SceneryEditorX::Encoding::HashCombine(hash, props.emission);
      
        // Hash texture paths
        for (const auto& texture : material->GetTextures())
        {
            SceneryEditorX::Encoding::HashCombine(hash, texture->GetPath());
        }
      
        return hash;
    }
  
    uint32_t HashMesh(const Ref<Mesh>& mesh)
    {
        uint32_t hash = 0;
      
        // Hash vertex count and index count
        SceneryEditorX::Encoding::HashCombine(hash, mesh->GetVertexCount());
        SceneryEditorX::Encoding::HashCombine(hash, mesh->GetIndexCount());
      
        // Hash bounding box for geometric representation
        const auto& bounds = mesh->GetBoundingBox();
        SceneryEditorX::Encoding::HashCombine(hash, &bounds, sizeof(BoundingBox));
      
        // For performance, only hash a subset of vertices for large meshes
        const auto& vertices = mesh->GetVertices();
        size_t sampleCount = std::min(vertices.size(), size_t(100));
        size_t step = std::max(vertices.size() / sampleCount, size_t(1));
      
        for (size_t i = 0; i < vertices.size(); i += step)
        {
            SceneryEditorX::Encoding::HashCombine(hash, &vertices[i], sizeof(Vertex));
        }
      
        return hash;
    }
  
    uint32_t GetTimestamp()
    {
        return static_cast<uint32_t>(std::time(nullptr));
    }
};
```

#### Resource Caching System

```cpp
class ResourceCache : public Module
{
private:
    struct CacheEntry
    {
        Ref<Resource> resource;
        uint32_t hash;
        std::chrono::steady_clock::time_point lastAccessed;
        size_t accessCount;
    };
  
    std::unordered_map<uint32_t, CacheEntry> m_Cache;
    std::mutex m_CacheMutex;
    size_t m_MaxCacheSize = 1024;
  
public:
    explicit ResourceCache() : Module("ResourceCache") {}
  
    /**
     * @brief Gets or loads a resource with caching based on content hash
     * @param resourcePath Path to the resource
     * @param parameters Loading parameters
     * @return Cached or newly loaded resource
     */
    template<typename ResourceType, typename ParameterType>
    Ref<ResourceType> GetOrLoadResource(const std::string& resourcePath,
                                      const ParameterType& parameters)
    {
        SEDX_PROFILE_SCOPE("ResourceCache::GetOrLoadResource");
      
        // Calculate hash of resource path and parameters
        uint32_t resourceHash = CalculateResourceHash(resourcePath, parameters);
      
        std::lock_guard<std::mutex> lock(m_CacheMutex);
      
        // Check if resource is already cached
        auto it = m_Cache.find(resourceHash);
        if (it != m_Cache.end())
        {
            // Update access statistics
            it->second.lastAccessed = std::chrono::steady_clock::now();
            it->second.accessCount++;
          
            SEDX_CORE_INFO_TAG("CACHE", "Cache hit for resource: {} (hash: {})", 
                              resourcePath, resourceHash);
          
            return std::static_pointer_cast<ResourceType>(it->second.resource);
        }
      
        // Resource not cached, load it
        SEDX_CORE_INFO_TAG("CACHE", "Cache miss for resource: {} (hash: {})", 
                          resourcePath, resourceHash);
      
        auto resource = LoadResource<ResourceType>(resourcePath, parameters);
        if (!resource)
        {
            SEDX_CORE_ERROR_TAG("CACHE", "Failed to load resource: {}", resourcePath);
            return nullptr;
        }
      
        // Add to cache
        CacheEntry entry;
        entry.resource = resource;
        entry.hash = resourceHash;
        entry.lastAccessed = std::chrono::steady_clock::now();
        entry.accessCount = 1;
      
        m_Cache[resourceHash] = entry;
      
        // Clean cache if it's getting too large
        if (m_Cache.size() > m_MaxCacheSize)
        {
            CleanOldEntries();
        }
      
        return resource;
    }
  
private:
    template<typename ParameterType>
    uint32_t CalculateResourceHash(const std::string& resourcePath,
                                  const ParameterType& parameters)
    {
        uint32_t hash = 0;
      
        // Hash the resource path
        SceneryEditorX::Encoding::HashCombine(hash, resourcePath);
      
        // Hash the parameters structure
        SceneryEditorX::Encoding::HashCombine(hash, &parameters, sizeof(ParameterType));
      
        // Include file modification time if available
        try
        {
            auto fileTime = std::filesystem::last_write_time(resourcePath);
            auto timeValue = fileTime.time_since_epoch().count();
            SceneryEditorX::Encoding::HashCombine(hash, timeValue);
        }
        catch (const std::filesystem::filesystem_error&)
        {
            // File doesn't exist or can't be accessed, use path hash only
            SEDX_CORE_WARN_TAG("CACHE", "Could not get modification time for: {}", resourcePath);
        }
      
        return hash;
    }
  
    template<typename ResourceType>
    Ref<ResourceType> LoadResource(const std::string& resourcePath,
                                  const auto& parameters)
    {
        // Implementation would depend on resource type
        // This is a placeholder for the actual loading logic
        return ResourceType::Load(resourcePath, parameters);
    }
  
    void CleanOldEntries()
    {
        // Remove least recently used entries
        std::vector<std::pair<uint32_t, std::chrono::steady_clock::time_point>> entries;
      
        for (const auto& [hash, entry] : m_Cache)
        {
            entries.emplace_back(hash, entry.lastAccessed);
        }
      
        // Sort by access time (oldest first)
        std::sort(entries.begin(), entries.end(),
                 [](const auto& a, const auto& b) { return a.second < b.second; });
      
        // Remove oldest 25% of entries
        size_t entriesToRemove = m_Cache.size() / 4;
        for (size_t i = 0; i < entriesToRemove && i < entries.size(); ++i)
        {
            uint32_t hashToRemove = entries[i].first;
            m_Cache.erase(hashToRemove);
            SEDX_CORE_INFO_TAG("CACHE", "Removed cached resource with hash: {}", hashToRemove);
        }
    }
};
```

### 3. Configuration and Settings Integration

#### Settings Serialization with Base64

```cpp
class AdvancedSettingsManager : public Module
{
private:
    struct BinarySettingsHeader
    {
        uint32_t version;
        uint32_t settingsCount;
        uint32_t totalSize;
        uint32_t checksum;
    };
  
public:
    explicit AdvancedSettingsManager() : Module("AdvancedSettingsManager") {}
  
    /**
     * @brief Saves complex settings to a configuration file using Base64 encoding
     * @param filename Configuration file path
     * @param settings Settings data structure
     * @return true if save was successful
     */
    template<typename SettingsType>
    bool SaveBinarySettings(const std::string& filename, const SettingsType& settings)
    {
        SEDX_PROFILE_SCOPE("AdvancedSettingsManager::SaveBinarySettings");
      
        try
        {
            // Create binary buffer
            std::vector<uint8_t> binaryData;
          
            // Add header
            BinarySettingsHeader header = CreateSettingsHeader(settings);
            const uint8_t* headerBytes = reinterpret_cast<const uint8_t*>(&header);
            binaryData.insert(binaryData.end(), headerBytes, headerBytes + sizeof(header));
          
            // Add settings data
            const uint8_t* settingsBytes = reinterpret_cast<const uint8_t*>(&settings);
            binaryData.insert(binaryData.end(), settingsBytes, settingsBytes + sizeof(settings));
          
            // Encode to Base64
            std::string encoded = SceneryEditorX::Encoding::EncodeBase64(
                binaryData.data(), binaryData.size()
            );
          
            // Write to file with metadata
            std::ofstream file(filename);
            if (!file.is_open())
            {
                SEDX_CORE_ERROR_TAG("SETTINGS", "Failed to open settings file: {}", filename);
                return false;
            }
          
            file << "# SceneryEditorX Settings File\n";
            file << "# Version: " << header.version << "\n";
            file << "# Generated: " << GetCurrentTimestamp() << "\n";
            file << "settings_data=" << encoded << "\n";
          
            SEDX_CORE_INFO_TAG("SETTINGS", "Saved binary settings to {} ({} bytes)", 
                              filename, binaryData.size());
            return true;
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("SETTINGS", "Failed to save settings: {}", e.what());
            return false;
        }
    }
  
    /**
     * @brief Loads complex settings from a Base64 encoded configuration file
     * @param filename Configuration file path
     * @param settings Output settings data structure
     * @return true if load was successful
     */
    template<typename SettingsType>
    bool LoadBinarySettings(const std::string& filename, SettingsType& settings)
    {
        SEDX_PROFILE_SCOPE("AdvancedSettingsManager::LoadBinarySettings");
      
        try
        {
            std::ifstream file(filename);
            if (!file.is_open())
            {
                SEDX_CORE_WARN_TAG("SETTINGS", "Settings file not found: {}", filename);
                return false;
            }
          
            std::string line;
            std::string encodedData;
          
            // Find the settings data line
            while (std::getline(file, line))
            {
                if (line.starts_with("settings_data="))
                {
                    encodedData = line.substr(14); // Skip "settings_data="
                    break;
                }
            }
          
            if (encodedData.empty())
            {
                SEDX_CORE_ERROR_TAG("SETTINGS", "No settings data found in file: {}", filename);
                return false;
            }
          
            // Decode from Base64
            std::vector<uint8_t> binaryData = 
                SceneryEditorX::Encoding::DecodeBase64(encodedData);
          
            if (binaryData.size() < sizeof(BinarySettingsHeader))
            {
                SEDX_CORE_ERROR_TAG("SETTINGS", "Invalid settings data size: {}", binaryData.size());
                return false;
            }
          
            // Extract header
            BinarySettingsHeader header;
            std::memcpy(&header, binaryData.data(), sizeof(header));
          
            // Validate header
            if (!ValidateSettingsHeader(header, binaryData))
            {
                SEDX_CORE_ERROR_TAG("SETTINGS", "Settings header validation failed");
                return false;
            }
          
            // Extract settings data
            if (binaryData.size() < sizeof(BinarySettingsHeader) + sizeof(SettingsType))
            {
                SEDX_CORE_ERROR_TAG("SETTINGS", "Insufficient data for settings structure");
                return false;
            }
          
            std::memcpy(&settings, binaryData.data() + sizeof(BinarySettingsHeader), 
                       sizeof(SettingsType));
          
            SEDX_CORE_INFO_TAG("SETTINGS", "Loaded binary settings from {} ({} bytes)", 
                              filename, binaryData.size());
            return true;
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("SETTINGS", "Failed to load settings: {}", e.what());
            return false;
        }
    }
  
private:
    template<typename SettingsType>
    BinarySettingsHeader CreateSettingsHeader(const SettingsType& settings)
    {
        BinarySettingsHeader header = {};
        header.version = 1;
        header.settingsCount = 1;
        header.totalSize = sizeof(SettingsType);
      
        // Calculate checksum of settings data
        uint32_t checksum = 0;
        SceneryEditorX::Encoding::HashCombine(checksum, 
            const_cast<SettingsType*>(&settings), sizeof(SettingsType));
        header.checksum = checksum;
      
        return header;
    }
  
    bool ValidateSettingsHeader(const BinarySettingsHeader& header,
                               const std::vector<uint8_t>& data)
    {
        // Check version compatibility
        if (header.version > 1)
        {
            SEDX_CORE_WARN_TAG("SETTINGS", "Settings version {} not supported", header.version);
            return false;
        }
      
        // Check data size
        if (data.size() != sizeof(BinarySettingsHeader) + header.totalSize)
        {
            SEDX_CORE_ERROR_TAG("SETTINGS", "Settings size mismatch: expected {}, got {}", 
                               sizeof(BinarySettingsHeader) + header.totalSize, data.size());
            return false;
        }
      
        // Verify checksum
        uint32_t calculatedChecksum = 0;
        const uint8_t* settingsData = data.data() + sizeof(BinarySettingsHeader);
        SceneryEditorX::Encoding::HashCombine(calculatedChecksum, 
            const_cast<uint8_t*>(settingsData), header.totalSize);
      
        if (calculatedChecksum != header.checksum)
        {
            SEDX_CORE_ERROR_TAG("SETTINGS", "Settings checksum mismatch: expected {}, got {}", 
                               header.checksum, calculatedChecksum);
            return false;
        }
      
        return true;
    }
  
    std::string GetCurrentTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        return std::ctime(&time_t);
    }
};
```

---

## Performance Optimization Techniques

### 1. Memory-Efficient Base64 Operations

```cpp
class OptimizedBase64
{
private:
    // Pre-computed lookup table for faster decoding
    static constexpr uint8_t DecodeTable[256] = {
        // ... initialize with Base64 decode values
    };
  
public:
    /**
     * @brief Memory-efficient Base64 encoding with pre-allocated output
     * @param input Input data
     * @param inputSize Size of input data
     * @param output Pre-allocated output buffer
     * @param outputSize Size of output buffer
     * @return Number of bytes written to output
     */
    static size_t EncodeBase64Optimized(const uint8_t* input, size_t inputSize,
                                       char* output, size_t outputSize)
    {
        // Calculate required output size
        size_t requiredSize = ((inputSize + 2) / 3) * 4;
        if (outputSize < requiredSize + 1) // +1 for null terminator
            return 0;
      
        size_t outputPos = 0;
        size_t i = 0;
      
        // Process 3-byte groups
        for (; i + 2 < inputSize; i += 3)
        {
            uint32_t triple = (input[i] << 16) | (input[i + 1] << 8) | input[i + 2];
          
            output[outputPos++] = base64Chars[(triple >> 18) & 0x3F];
            output[outputPos++] = base64Chars[(triple >> 12) & 0x3F];
            output[outputPos++] = base64Chars[(triple >> 6) & 0x3F];
            output[outputPos++] = base64Chars[triple & 0x3F];
        }
      
        // Handle remaining bytes
        if (i < inputSize)
        {
            uint32_t triple = input[i] << 16;
            if (i + 1 < inputSize)
                triple |= input[i + 1] << 8;
          
            output[outputPos++] = base64Chars[(triple >> 18) & 0x3F];
            output[outputPos++] = base64Chars[(triple >> 12) & 0x3F];
          
            if (i + 1 < inputSize)
                output[outputPos++] = base64Chars[(triple >> 6) & 0x3F];
            else
                output[outputPos++] = '=';
          
            output[outputPos++] = '=';
        }
      
        output[outputPos] = '\0';
        return outputPos;
    }
};
```

### 2. Streaming Hash Operations for Large Data

```cpp
class StreamingHasher
{
private:
    uint32_t m_Hash = 0;
    size_t m_TotalBytes = 0;
  
public:
    /**
     * @brief Adds data to the running hash calculation
     * @param data Pointer to data
     * @param size Size of data in bytes
     */
    void Update(const void* data, size_t size)
    {
        // Process data in chunks for better cache performance
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        constexpr size_t ChunkSize = 4096;
      
        for (size_t offset = 0; offset < size; offset += ChunkSize)
        {
            size_t chunkSize = std::min(ChunkSize, size - offset);
          
            // Use the encoding system's hash combine for chunks
            SceneryEditorX::Encoding::HashCombine(m_Hash, 
                const_cast<uint8_t*>(bytes + offset), 
                static_cast<uint32_t>(chunkSize));
        }
      
        m_TotalBytes += size;
    }
  
    /**
     * @brief Finalizes the hash calculation
     * @return Final hash value
     */
    uint32_t Finalize()
    {
        // Include total byte count in final hash
        SceneryEditorX::Encoding::HashCombine(m_Hash, m_TotalBytes);
        return m_Hash;
    }
  
    /**
     * @brief Resets the hasher for reuse
     */
    void Reset()
    {
        m_Hash = 0;
        m_TotalBytes = 0;
    }
};
```

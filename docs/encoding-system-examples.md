# Scenery Editor X - Encoding System Examples and Use Cases

---

## Implementation Examples

This document provides practical examples of how the SceneryEditorX encoding system is used throughout the application in real-world scenarios.

## 1. X-Plane Asset Processing

### OBJ File Checksum Validation

```cpp
class XPlaneObjProcessor
{
public:
    struct ObjFileInfo
    {
        std::string filename;
        uint32_t contentHash;
        std::chrono::file_time_type lastModified;
        size_t fileSize;
    };
  
    /**
     * @brief Validates X-Plane OBJ file integrity using content hashing
     * @param objPath Path to the OBJ file
     * @return File information with content hash
     */
    ObjFileInfo ProcessObjFile(const std::string& objPath)
    {
        SEDX_PROFILE_SCOPE("XPlaneObjProcessor::ProcessObjFile");
      
        ObjFileInfo info;
        info.filename = std::filesystem::path(objPath).filename().string();
      
        try
        {
            // Get file metadata
            info.lastModified = std::filesystem::last_write_time(objPath);
            info.fileSize = std::filesystem::file_size(objPath);
          
            // Read file content
            std::ifstream file(objPath, std::ios::binary);
            if (!file.is_open())
            {
                SEDX_CORE_ERROR_TAG("XPLANE", "Failed to open OBJ file: {}", objPath);
                return info;
            }
          
            // Calculate content hash for integrity verification
            uint32_t contentHash = 0;
            std::string line;
          
            while (std::getline(file, line))
            {
                // Skip comments and empty lines for consistent hashing
                if (line.empty() || line[0] == '#')
                    continue;
              
                // Normalize whitespace for consistent hashing
                std::string normalizedLine = NormalizeWhitespace(line);
                SceneryEditorX::Encoding::HashCombine(contentHash, normalizedLine);
            }
          
            // Include file size in hash to detect truncation
            SceneryEditorX::Encoding::HashCombine(contentHash, info.fileSize);
          
            info.contentHash = contentHash;
          
            SEDX_CORE_INFO_TAG("XPLANE", "Processed OBJ file {} (hash: {}, size: {} bytes)",
                              info.filename, info.contentHash, info.fileSize);
          
            return info;
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("XPLANE", "Exception processing OBJ file {}: {}", objPath, e.what());
            return info;
        }
    }
  
private:
    std::string NormalizeWhitespace(const std::string& line)
    {
        std::string result;
        result.reserve(line.size());
      
        bool inWhitespace = false;
        for (char c : line)
        {
            if (std::isspace(c))
            {
                if (!inWhitespace && !result.empty())
                {
                    result += ' ';
                    inWhitespace = true;
                }
            }
            else
            {
                result += c;
                inWhitespace = false;
            }
        }
      
        return result;
    }
};
```

### Texture Atlas Generation

```cpp
class TextureAtlasManager
{
private:
    struct AtlasEntry
    {
        std::string texturePath;
        uint32_t textureHash;
        glm::vec4 uvBounds; // x, y, width, height in atlas coordinates
        uint32_t atlasIndex;
    };
  
    std::vector<AtlasEntry> m_AtlasEntries;
  
public:
    /**
     * @brief Creates a texture atlas with encoded metadata
     * @param textures List of texture paths to include in atlas
     * @param atlasSize Size of the output atlas texture
     * @return Atlas metadata encoded as Base64
     */
    std::string CreateTextureAtlas(const std::vector<std::string>& textures, 
                                  uint32_t atlasSize)
    {
        SEDX_PROFILE_SCOPE("TextureAtlasManager::CreateTextureAtlas");
      
        // Clear existing entries
        m_AtlasEntries.clear();
      
        // Pack textures into atlas
        TexturePacker packer(atlasSize, atlasSize);
        uint32_t atlasIndex = 0;
      
        for (const auto& texturePath : textures)
        {
            // Load texture to get dimensions
            auto texture = LoadTexture(texturePath);
            if (!texture)
            {
                SEDX_CORE_WARN_TAG("ATLAS", "Failed to load texture: {}", texturePath);
                continue;
            }
          
            // Calculate texture content hash
            uint32_t textureHash = CalculateTextureHash(texture);
          
            // Pack texture into atlas
            glm::vec4 uvBounds = packer.PackTexture(texture->GetWidth(), texture->GetHeight());
            if (uvBounds.z == 0.0f || uvBounds.w == 0.0f)
            {
                SEDX_CORE_WARN_TAG("ATLAS", "Failed to pack texture: {}", texturePath);
                continue;
            }
          
            // Create atlas entry
            AtlasEntry entry;
            entry.texturePath = texturePath;
            entry.textureHash = textureHash;
            entry.uvBounds = uvBounds;
            entry.atlasIndex = atlasIndex;
          
            m_AtlasEntries.push_back(entry);
          
            SEDX_CORE_INFO_TAG("ATLAS", "Packed texture {} at UV ({:.3f}, {:.3f}, {:.3f}, {:.3f})",
                              texturePath, uvBounds.x, uvBounds.y, uvBounds.z, uvBounds.w);
        }
      
        // Serialize atlas metadata
        return SerializeAtlasMetadata();
    }
  
private:
    uint32_t CalculateTextureHash(const Ref<Texture>& texture)
    {
        uint32_t hash = 0;
      
        // Hash texture properties
        SceneryEditorX::Encoding::HashCombine(hash, texture->GetWidth());
        SceneryEditorX::Encoding::HashCombine(hash, texture->GetHeight());
        SceneryEditorX::Encoding::HashCombine(hash, static_cast<uint32_t>(texture->GetFormat()));
        SceneryEditorX::Encoding::HashCombine(hash, texture->GetPath());
      
        // Hash texture data (sample for performance)
        const auto& data = texture->GetData();
        if (!data.empty())
        {
            // Sample texture data at regular intervals
            size_t sampleCount = std::min(data.size(), size_t(1024));
            size_t step = std::max(data.size() / sampleCount, size_t(1));
          
            for (size_t i = 0; i < data.size(); i += step)
            {
                SceneryEditorX::Encoding::HashCombine(hash, data[i]);
            }
        }
      
        return hash;
    }
  
    std::string SerializeAtlasMetadata()
    {
        // Create binary metadata structure
        struct AtlasMetadata
        {
            uint32_t version = 1;
            uint32_t entryCount = 0;
            uint32_t atlasSize = 0;
            uint32_t metadataHash = 0;
        };
      
        std::vector<uint8_t> binaryData;
      
        // Add header
        AtlasMetadata metadata;
        metadata.entryCount = static_cast<uint32_t>(m_AtlasEntries.size());
        metadata.atlasSize = 1024; // Example atlas size
      
        // Calculate metadata hash
        uint32_t hash = 0;
        for (const auto& entry : m_AtlasEntries)
        {
            SceneryEditorX::Encoding::HashCombine(hash, entry.texturePath);
            SceneryEditorX::Encoding::HashCombine(hash, entry.textureHash);
            SceneryEditorX::Encoding::HashCombine(hash, &entry.uvBounds, sizeof(glm::vec4));
        }
        metadata.metadataHash = hash;
      
        // Add metadata to binary data
        const uint8_t* metadataBytes = reinterpret_cast<const uint8_t*>(&metadata);
        binaryData.insert(binaryData.end(), metadataBytes, metadataBytes + sizeof(metadata));
      
        // Add atlas entries
        for (const auto& entry : m_AtlasEntries)
        {
            // Add path length and path
            uint32_t pathLength = static_cast<uint32_t>(entry.texturePath.size());
            const uint8_t* pathLengthBytes = reinterpret_cast<const uint8_t*>(&pathLength);
            binaryData.insert(binaryData.end(), pathLengthBytes, pathLengthBytes + sizeof(pathLength));
          
            binaryData.insert(binaryData.end(), entry.texturePath.begin(), entry.texturePath.end());
          
            // Add entry data
            const uint8_t* entryBytes = reinterpret_cast<const uint8_t*>(&entry.textureHash);
            binaryData.insert(binaryData.end(), entryBytes, entryBytes + sizeof(uint32_t) + sizeof(glm::vec4) + sizeof(uint32_t));
        }
      
        // Encode to Base64
        return SceneryEditorX::Encoding::EncodeBase64(binaryData.data(), binaryData.size());
    }
};
```

## 2. Scene Serialization and Version Control

### Scene Delta Encoding

```cpp
class SceneDeltaManager
{
private:
    struct SceneState
    {
        uint32_t version;
        std::string sceneName;
        std::vector<SceneObjectState> objects;
        uint32_t stateHash;
    };
  
    struct SceneObjectState
    {
        SceneryEditorX::UUID objectId;
        glm::mat4 transform;
        std::string materialPath;
        std::string meshPath;
        uint32_t objectHash;
    };
  
    std::vector<SceneState> m_SceneHistory;
  
public:
    /**
     * @brief Creates a delta encoding between two scene states
     * @param previousState Previous scene state
     * @param currentState Current scene state
     * @return Base64 encoded delta data
     */
    std::string CreateSceneDelta(const SceneState& previousState, const SceneState& currentState)
    {
        SEDX_PROFILE_SCOPE("SceneDeltaManager::CreateSceneDelta");
      
        std::vector<uint8_t> deltaData;
      
        // Delta header
        struct DeltaHeader
        {
            uint32_t deltaVersion = 1;
            uint32_t previousVersion;
            uint32_t currentVersion;
            uint32_t addedObjects = 0;
            uint32_t modifiedObjects = 0;
            uint32_t removedObjects = 0;
        };
      
        DeltaHeader header;
        header.previousVersion = previousState.version;
        header.currentVersion = currentState.version;
      
        // Find object changes
        std::vector<SceneObjectState> addedObjects;
        std::vector<SceneObjectState> modifiedObjects;
        std::vector<SceneryEditorX::UUID> removedObjects;
      
        CalculateObjectDeltas(previousState.objects, currentState.objects,
                            addedObjects, modifiedObjects, removedObjects);
      
        header.addedObjects = static_cast<uint32_t>(addedObjects.size());
        header.modifiedObjects = static_cast<uint32_t>(modifiedObjects.size());
        header.removedObjects = static_cast<uint32_t>(removedObjects.size());
      
        // Add header to delta data
        const uint8_t* headerBytes = reinterpret_cast<const uint8_t*>(&header);
        deltaData.insert(deltaData.end(), headerBytes, headerBytes + sizeof(header));
      
        // Add added objects
        for (const auto& obj : addedObjects)
        {
            SerializeObjectToDelta(obj, deltaData);
        }
      
        // Add modified objects
        for (const auto& obj : modifiedObjects)
        {
            SerializeObjectToDelta(obj, deltaData);
        }
      
        // Add removed object IDs
        for (const auto& id : removedObjects)
        {
            const uint8_t* idBytes = reinterpret_cast<const uint8_t*>(&id);
            deltaData.insert(deltaData.end(), idBytes, idBytes + sizeof(id));
        }
      
        // Calculate delta hash for integrity
        uint32_t deltaHash = 0;
        SceneryEditorX::Encoding::HashCombine(deltaHash, deltaData.data(), 
                                            static_cast<uint32_t>(deltaData.size()));
      
        // Append hash to delta data
        const uint8_t* hashBytes = reinterpret_cast<const uint8_t*>(&deltaHash);
        deltaData.insert(deltaData.end(), hashBytes, hashBytes + sizeof(deltaHash));
      
        // Encode to Base64
        std::string encodedDelta = SceneryEditorX::Encoding::EncodeBase64(
            deltaData.data(), deltaData.size()
        );
      
        SEDX_CORE_INFO_TAG("SCENE", "Created scene delta: {} added, {} modified, {} removed objects",
                          addedObjects.size(), modifiedObjects.size(), removedObjects.size());
      
        return encodedDelta;
    }
  
private:
    void CalculateObjectDeltas(const std::vector<SceneObjectState>& previousObjects,
                              const std::vector<SceneObjectState>& currentObjects,
                              std::vector<SceneObjectState>& addedObjects,
                              std::vector<SceneObjectState>& modifiedObjects,
                              std::vector<SceneryEditorX::UUID>& removedObjects)
    {
        // Create lookup maps for efficient comparison
        std::unordered_map<uint64_t, const SceneObjectState*> previousMap;
        std::unordered_map<uint64_t, const SceneObjectState*> currentMap;
      
        for (const auto& obj : previousObjects)
        {
            previousMap[obj.objectId.ID] = &obj;
        }
      
        for (const auto& obj : currentObjects)
        {
            currentMap[obj.objectId.ID] = &obj;
        }
      
        // Find added and modified objects
        for (const auto& [id, currentObj] : currentMap)
        {
            auto previousIt = previousMap.find(id);
            if (previousIt == previousMap.end())
            {
                // Object was added
                addedObjects.push_back(*currentObj);
            }
            else
            {
                // Check if object was modified
                if (currentObj->objectHash != previousIt->second->objectHash)
                {
                    modifiedObjects.push_back(*currentObj);
                }
            }
        }
      
        // Find removed objects
        for (const auto& [id, previousObj] : previousMap)
        {
            if (currentMap.find(id) == currentMap.end())
            {
                removedObjects.push_back(previousObj->objectId);
            }
        }
    }
  
    void SerializeObjectToDelta(const SceneObjectState& obj, std::vector<uint8_t>& deltaData)
    {
        // Add object ID
        const uint8_t* idBytes = reinterpret_cast<const uint8_t*>(&obj.objectId);
        deltaData.insert(deltaData.end(), idBytes, idBytes + sizeof(obj.objectId));
      
        // Add transform matrix
        const uint8_t* transformBytes = reinterpret_cast<const uint8_t*>(&obj.transform);
        deltaData.insert(deltaData.end(), transformBytes, transformBytes + sizeof(obj.transform));
      
        // Add material path
        uint32_t materialPathLength = static_cast<uint32_t>(obj.materialPath.size());
        const uint8_t* pathLengthBytes = reinterpret_cast<const uint8_t*>(&materialPathLength);
        deltaData.insert(deltaData.end(), pathLengthBytes, pathLengthBytes + sizeof(materialPathLength));
        deltaData.insert(deltaData.end(), obj.materialPath.begin(), obj.materialPath.end());
      
        // Add mesh path
        uint32_t meshPathLength = static_cast<uint32_t>(obj.meshPath.size());
        const uint8_t* meshLengthBytes = reinterpret_cast<const uint8_t*>(&meshPathLength);
        deltaData.insert(deltaData.end(), meshLengthBytes, meshLengthBytes + sizeof(meshPathLength));
        deltaData.insert(deltaData.end(), obj.meshPath.begin(), obj.meshPath.end());
      
        // Add object hash
        const uint8_t* hashBytes = reinterpret_cast<const uint8_t*>(&obj.objectHash);
        deltaData.insert(deltaData.end(), hashBytes, hashBytes + sizeof(obj.objectHash));
    }
};
```

## 3. Asset Bundling and Distribution

### Asset Bundle Creation

```cpp
class AssetBundleManager : public Module
{
private:
    struct BundleHeader
    {
        char signature[16]; // "SEDX_BUNDLE_V1"
        uint32_t version;
        uint32_t assetCount;
        uint64_t bundleSize;
        uint32_t compressionType;
        uint32_t bundleHash;
    };
  
    struct AssetEntry
    {
        SceneryEditorX::UUID assetId;
        uint32_t assetType;
        uint64_t offset;
        uint64_t compressedSize;
        uint64_t uncompressedSize;
        uint32_t assetHash;
        char name[256];
    };
  
public:
    explicit AssetBundleManager() : Module("AssetBundleManager") {}
  
    /**
     * @brief Creates an asset bundle with Base64 encoded metadata
     * @param assets List of assets to bundle
     * @param bundlePath Output bundle file path
     * @return Bundle metadata as Base64 string
     */
    std::string CreateAssetBundle(const std::vector<Ref<Asset>>& assets, 
                                 const std::string& bundlePath)
    {
        SEDX_PROFILE_SCOPE("AssetBundleManager::CreateAssetBundle");
      
        try
        {
            std::ofstream bundleFile(bundlePath, std::ios::binary);
            if (!bundleFile.is_open())
            {
                SEDX_CORE_ERROR_TAG("BUNDLE", "Failed to create bundle file: {}", bundlePath);
                return "";
            }
          
            // Create bundle header
            BundleHeader header = {};
            std::strncpy(header.signature, "SEDX_BUNDLE_V1", sizeof(header.signature));
            header.version = 1;
            header.assetCount = static_cast<uint32_t>(assets.size());
            header.compressionType = 0; // No compression for now
          
            // Reserve space for header
            uint64_t headerPos = bundleFile.tellp();
            bundleFile.seekp(sizeof(BundleHeader), std::ios::cur);
          
            // Reserve space for asset entries
            uint64_t entriesPos = bundleFile.tellp();
            bundleFile.seekp(sizeof(AssetEntry) * assets.size(), std::ios::cur);
          
            // Write asset data and create entries
            std::vector<AssetEntry> entries;
            uint32_t bundleContentHash = 0;
          
            for (size_t i = 0; i < assets.size(); ++i)
            {
                const auto& asset = assets[i];
              
                // Get asset data
                std::vector<uint8_t> assetData = asset->ToBinary();
              
                // Calculate asset hash
                uint32_t assetHash = 0;
                SceneryEditorX::Encoding::HashCombine(assetHash, assetData.data(), 
                                                    static_cast<uint32_t>(assetData.size()));
              
                // Create asset entry
                AssetEntry entry = {};
                entry.assetId = asset->GetId();
                entry.assetType = static_cast<uint32_t>(asset->GetType());
                entry.offset = bundleFile.tellp();
                entry.compressedSize = assetData.size();
                entry.uncompressedSize = assetData.size();
                entry.assetHash = assetHash;
                std::strncpy(entry.name, asset->GetName().c_str(), sizeof(entry.name) - 1);
              
                entries.push_back(entry);
              
                // Write asset data
                bundleFile.write(reinterpret_cast<const char*>(assetData.data()), assetData.size());
              
                // Update bundle content hash
                SceneryEditorX::Encoding::HashCombine(bundleContentHash, assetHash);
              
                SEDX_CORE_INFO_TAG("BUNDLE", "Added asset {} to bundle ({} bytes, hash: {})",
                                  asset->GetName(), assetData.size(), assetHash);
            }
          
            // Calculate final bundle size
            header.bundleSize = static_cast<uint64_t>(bundleFile.tellp());
            header.bundleHash = bundleContentHash;
          
            // Write header
            bundleFile.seekp(headerPos);
            bundleFile.write(reinterpret_cast<const char*>(&header), sizeof(header));
          
            // Write asset entries
            bundleFile.seekp(entriesPos);
            bundleFile.write(reinterpret_cast<const char*>(entries.data()), 
                           sizeof(AssetEntry) * entries.size());
          
            bundleFile.close();
          
            SEDX_CORE_INFO_TAG("BUNDLE", "Created asset bundle {} with {} assets ({} bytes)",
                              bundlePath, assets.size(), header.bundleSize);
          
            // Create bundle metadata
            return CreateBundleMetadata(header, entries);
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("BUNDLE", "Exception creating asset bundle: {}", e.what());
            return "";
        }
    }
  
private:
    std::string CreateBundleMetadata(const BundleHeader& header, 
                                   const std::vector<AssetEntry>& entries)
    {
        // Create metadata structure
        std::vector<uint8_t> metadataData;
      
        // Add header
        const uint8_t* headerBytes = reinterpret_cast<const uint8_t*>(&header);
        metadataData.insert(metadataData.end(), headerBytes, headerBytes + sizeof(header));
      
        // Add entry count
        uint32_t entryCount = static_cast<uint32_t>(entries.size());
        const uint8_t* entryCountBytes = reinterpret_cast<const uint8_t*>(&entryCount);
        metadataData.insert(metadataData.end(), entryCountBytes, entryCountBytes + sizeof(entryCount));
      
        // Add entries
        for (const auto& entry : entries)
        {
            const uint8_t* entryBytes = reinterpret_cast<const uint8_t*>(&entry);
            metadataData.insert(metadataData.end(), entryBytes, entryBytes + sizeof(entry));
        }
      
        // Calculate metadata hash
        uint32_t metadataHash = 0;
        SceneryEditorX::Encoding::HashCombine(metadataHash, metadataData.data(), 
                                            static_cast<uint32_t>(metadataData.size()));
      
        // Append metadata hash
        const uint8_t* hashBytes = reinterpret_cast<const uint8_t*>(&metadataHash);
        metadataData.insert(metadataData.end(), hashBytes, hashBytes + sizeof(metadataHash));
      
        // Encode to Base64
        return SceneryEditorX::Encoding::EncodeBase64(metadataData.data(), metadataData.size());
    }
};
```

## 4. Network Protocol Implementation

### Scene Synchronization Protocol

```cpp
class SceneSyncProtocol
{
private:
    enum class MessageType : uint32_t
    {
        Heartbeat = 1,
        SceneUpdate = 2,
        ObjectDelta = 3,
        AssetRequest = 4,
        AssetResponse = 5
    };
  
    struct MessageHeader
    {
        uint32_t messageType;
        uint32_t messageSize;
        uint32_t sequenceNumber;
        uint32_t sessionId;
        uint32_t messageHash;
        uint64_t timestamp;
    };
  
public:
    /**
     * @brief Creates a scene update message with Base64 encoded payload
     * @param sceneData Scene data to synchronize
     * @param sessionId Current session ID
     * @param sequenceNumber Message sequence number
     * @return Base64 encoded message
     */
    std::string CreateSceneUpdateMessage(const SceneData& sceneData, 
                                       uint32_t sessionId, 
                                       uint32_t sequenceNumber)
    {
        SEDX_PROFILE_SCOPE("SceneSyncProtocol::CreateSceneUpdateMessage");
      
        try
        {
            // Serialize scene data
            std::vector<uint8_t> sceneBytes = SerializeSceneData(sceneData);
          
            // Create message header
            MessageHeader header = {};
            header.messageType = static_cast<uint32_t>(MessageType::SceneUpdate);
            header.messageSize = static_cast<uint32_t>(sizeof(MessageHeader) + sceneBytes.size());
            header.sequenceNumber = sequenceNumber;
            header.sessionId = sessionId;
            header.timestamp = GetCurrentTimestamp();
          
            // Calculate message hash
            uint32_t contentHash = 0;
            SceneryEditorX::Encoding::HashCombine(contentHash, sceneBytes.data(), 
                                                static_cast<uint32_t>(sceneBytes.size()));
            SceneryEditorX::Encoding::HashCombine(contentHash, &header, 
                                                sizeof(MessageHeader) - sizeof(uint32_t));
            header.messageHash = contentHash;
          
            // Combine header and payload
            std::vector<uint8_t> messageData;
            const uint8_t* headerBytes = reinterpret_cast<const uint8_t*>(&header);
            messageData.insert(messageData.end(), headerBytes, headerBytes + sizeof(header));
            messageData.insert(messageData.end(), sceneBytes.begin(), sceneBytes.end());
          
            // Encode to Base64 for network transmission
            std::string encodedMessage = SceneryEditorX::Encoding::EncodeBase64(
                messageData.data(), messageData.size()
            );
          
            SEDX_CORE_INFO_TAG("NETWORK", "Created scene update message (seq: {}, size: {} bytes)",
                              sequenceNumber, messageData.size());
          
            return encodedMessage;
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("NETWORK", "Failed to create scene update message: {}", e.what());
            return "";
        }
    }
  
    /**
     * @brief Validates and parses a received scene update message
     * @param encodedMessage Base64 encoded message
     * @return Parsed scene data or empty optional on failure
     */
    std::optional<SceneData> ParseSceneUpdateMessage(const std::string& encodedMessage)
    {
        SEDX_PROFILE_SCOPE("SceneSyncProtocol::ParseSceneUpdateMessage");
      
        try
        {
            // Decode from Base64
            std::vector<uint8_t> messageData = 
                SceneryEditorX::Encoding::DecodeBase64(encodedMessage);
          
            if (messageData.size() < sizeof(MessageHeader))
            {
                SEDX_CORE_ERROR_TAG("NETWORK", "Message too small for header: {} bytes", 
                                   messageData.size());
                return std::nullopt;
            }
          
            // Extract header
            MessageHeader header;
            std::memcpy(&header, messageData.data(), sizeof(header));
          
            // Validate message size
            if (messageData.size() != header.messageSize)
            {
                SEDX_CORE_ERROR_TAG("NETWORK", "Message size mismatch: expected {}, got {}",
                                   header.messageSize, messageData.size());
                return std::nullopt;
            }
          
            // Validate message type
            if (header.messageType != static_cast<uint32_t>(MessageType::SceneUpdate))
            {
                SEDX_CORE_WARN_TAG("NETWORK", "Unexpected message type: {}", header.messageType);
                return std::nullopt;
            }
          
            // Extract payload
            std::vector<uint8_t> sceneBytes(
                messageData.begin() + sizeof(MessageHeader),
                messageData.end()
            );
          
            // Validate message hash
            uint32_t calculatedHash = 0;
            SceneryEditorX::Encoding::HashCombine(calculatedHash, sceneBytes.data(), 
                                                static_cast<uint32_t>(sceneBytes.size()));
            MessageHeader headerForHash = header;
            headerForHash.messageHash = 0; // Exclude hash field from hash calculation
            SceneryEditorX::Encoding::HashCombine(calculatedHash, &headerForHash, 
                                                sizeof(MessageHeader) - sizeof(uint32_t));
          
            if (calculatedHash != header.messageHash)
            {
                SEDX_CORE_ERROR_TAG("NETWORK", "Message hash validation failed: expected {}, got {}",
                                   header.messageHash, calculatedHash);
                return std::nullopt;
            }
          
            // Deserialize scene data
            SceneData sceneData = DeserializeSceneData(sceneBytes);
          
            SEDX_CORE_INFO_TAG("NETWORK", "Parsed scene update message (seq: {}, timestamp: {})",
                              header.sequenceNumber, header.timestamp);
          
            return sceneData;
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("NETWORK", "Exception parsing scene update message: {}", e.what());
            return std::nullopt;
        }
    }
  
private:
    std::vector<uint8_t> SerializeSceneData(const SceneData& sceneData)
    {
        // Implementation would serialize scene data to binary format
        // This is a placeholder for the actual serialization logic
        std::vector<uint8_t> result;
      
        // Add scene metadata
        uint32_t objectCount = static_cast<uint32_t>(sceneData.objects.size());
        const uint8_t* countBytes = reinterpret_cast<const uint8_t*>(&objectCount);
        result.insert(result.end(), countBytes, countBytes + sizeof(objectCount));
      
        // Add scene objects
        for (const auto& object : sceneData.objects)
        {
            std::vector<uint8_t> objectBytes = SerializeSceneObject(object);
            result.insert(result.end(), objectBytes.begin(), objectBytes.end());
        }
      
        return result;
    }
  
    SceneData DeserializeSceneData(const std::vector<uint8_t>& data)
    {
        // Implementation would deserialize scene data from binary format
        // This is a placeholder for the actual deserialization logic
        SceneData sceneData;
      
        if (data.size() < sizeof(uint32_t))
            return sceneData;
      
        // Extract object count
        uint32_t objectCount;
        std::memcpy(&objectCount, data.data(), sizeof(objectCount));
      
        // Deserialize objects (simplified)
        size_t offset = sizeof(uint32_t);
        for (uint32_t i = 0; i < objectCount && offset < data.size(); ++i)
        {
            SceneObject object = DeserializeSceneObject(data, offset);
            sceneData.objects.push_back(object);
        }
      
        return sceneData;
    }
  
    uint64_t GetCurrentTimestamp()
    {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );
    }
};
```

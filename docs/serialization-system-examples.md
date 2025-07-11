# Scenery Editor X - Serialization System Usage Examples

---

## Overview

This document provides comprehensive examples demonstrating the practical use of the Scenery Editor X serialization system. These examples cover common use cases, best practices, and integration patterns for real-world application development.

## Basic Serialization Examples

### Simple Data Structures

```cpp
#include <SceneryEditorX/serialization/serializer_impl.h>

// Define a simple material structure
struct Material
{
    std::string name;
    Vec3 albedo{1.0f, 1.0f, 1.0f};
    float roughness = 0.5f;
    float metallic = 0.0f;
    bool isTransparent = false;
};

// Make it serializable
SERIALIZABLE(Material,
    &Material::name,
    &Material::albedo,
    &Material::roughness,
    &Material::metallic,
    &Material::isTransparent
);

// Usage example
void SaveMaterial()
{
    // Create material
    Material woodMaterial;
    woodMaterial.name = "Oak Wood";
    woodMaterial.albedo = {0.6f, 0.4f, 0.2f};
    woodMaterial.roughness = 0.8f;
    woodMaterial.metallic = 0.0f;
  
    // Save to file
    FileWriter writer("materials/wood.mat");
    if (writer) {
        using namespace SceneryEditorX::Serialization;
        bool success = Serialize(&writer, woodMaterial);
      
        if (success) {
            SEDX_CORE_INFO_TAG("MATERIAL", "Saved material: {}", woodMaterial.name);
        }
    }
}

void LoadMaterial()
{
    Material loadedMaterial;
  
    FileReader reader("materials/wood.mat");
    if (reader) {
        using namespace SceneryEditorX::Serialization;
        bool success = Deserialize(&reader, loadedMaterial);
      
        if (success) {
            SEDX_CORE_INFO_TAG("MATERIAL", "Loaded material: {}", loadedMaterial.name);
            SEDX_CORE_INFO_TAG("MATERIAL", "Albedo: ({}, {}, {})", 
                loadedMaterial.albedo.x, loadedMaterial.albedo.y, loadedMaterial.albedo.z);
        }
    }
}
```

### Container Serialization

```cpp
// Scene object with multiple container types
struct Scene
{
    std::string name;
    std::vector<Transform> objectTransforms;
    std::map<std::string, Material> materials;
    std::unordered_map<uint32_t, std::string> objectNames;
    std::vector<std::vector<int>> lodLevels; // Nested containers
};

SERIALIZABLE(Scene,
    &Scene::name,
    &Scene::objectTransforms,
    &Scene::materials,
    &Scene::objectNames,
    &Scene::lodLevels
);

void CreateAndSaveScene()
{
    Scene scene;
    scene.name = "Test Scene";
  
    // Add some transforms
    for (int i = 0; i < 10; ++i) {
        Transform transform;
        transform.position = {i * 2.0f, 0.0f, 0.0f};
        transform.rotation = {0.0f, i * 45.0f, 0.0f};
        scene.objectTransforms.push_back(transform);
    }
  
    // Add materials
    Material metalMaterial;
    metalMaterial.name = "Steel";
    metalMaterial.metallic = 1.0f;
    metalMaterial.roughness = 0.2f;
    scene.materials["steel"] = metalMaterial;
  
    Material plasticMaterial;
    plasticMaterial.name = "Plastic";
    plasticMaterial.metallic = 0.0f;
    plasticMaterial.roughness = 0.7f;
    scene.materials["plastic"] = plasticMaterial;
  
    // Add object names
    for (uint32_t i = 0; i < 10; ++i) {
        scene.objectNames[i] = fmt::format("Object_{}", i);
    }
  
    // Add LOD levels
    scene.lodLevels = {{100, 50, 25}, {80, 40, 20}, {60, 30, 15}};
  
    // Save scene
    FileWriter writer("scenes/test_scene.scene");
    using namespace SceneryEditorX::Serialization;
  
    if (Serialize(&writer, scene)) {
        SEDX_CORE_INFO_TAG("SCENE", "Scene saved successfully: {}", scene.name);
    }
}
```

## Advanced Usage Examples

### Custom Serialization with Intrusive Interface

```cpp
class Mesh
{
private:
    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;
    std::string m_Name;
    AABB m_BoundingBox;
    bool m_IsOptimized = false;
  
public:
    // Custom serialization with optimization and validation
    static void Serialize(SerializeWriter* writer, const Mesh& mesh)
    {
        SEDX_PROFILE_SCOPE("Mesh::Serialize");
      
        // Write header with version info
        uint32_t version = 2;
        writer->WriteRaw(version);
      
        // Write metadata
        writer->WriteString(mesh.m_Name);
        writer->WriteRaw(mesh.m_BoundingBox);
        writer->WriteRaw(mesh.m_IsOptimized);
      
        // Write vertex data
        writer->WriteRaw<uint32_t>(static_cast<uint32_t>(mesh.m_Vertices.size()));
        for (const auto& vertex : mesh.m_Vertices) {
            writer->WriteRaw(vertex.position);
            writer->WriteRaw(vertex.normal);
            writer->WriteRaw(vertex.uv);
        }
      
        // Write index data
        writer->WriteArray(mesh.m_Indices);
      
        SEDX_CORE_INFO_TAG("MESH", "Serialized mesh '{}' with {} vertices, {} indices", 
            mesh.m_Name, mesh.m_Vertices.size(), mesh.m_Indices.size());
    }
  
    static void Deserialize(SerializeReader* reader, Mesh& mesh)
    {
        SEDX_PROFILE_SCOPE("Mesh::Deserialize");
      
        // Read and validate version
        uint32_t version;
        reader->ReadRaw(version);
      
        if (version < 1 || version > 2) {
            SEDX_CORE_ERROR_TAG("MESH", "Unsupported mesh version: {}", version);
            throw std::runtime_error("Unsupported mesh version");
        }
      
        // Read metadata
        reader->ReadString(mesh.m_Name);
        reader->ReadRaw(mesh.m_BoundingBox);
      
        if (version >= 2) {
            reader->ReadRaw(mesh.m_IsOptimized);
        } else {
            mesh.m_IsOptimized = false;
        }
      
        // Read vertex data
        uint32_t vertexCount;
        reader->ReadRaw(vertexCount);
      
        mesh.m_Vertices.clear();
        mesh.m_Vertices.reserve(vertexCount);
      
        for (uint32_t i = 0; i < vertexCount; ++i) {
            Vertex vertex;
            reader->ReadRaw(vertex.position);
            reader->ReadRaw(vertex.normal);
            reader->ReadRaw(vertex.uv);
            mesh.m_Vertices.push_back(vertex);
        }
      
        // Read index data
        reader->ReadArray(mesh.m_Indices);
      
        // Post-load validation
        mesh.ValidateGeometry();
      
        SEDX_CORE_INFO_TAG("MESH", "Loaded mesh '{}' with {} vertices, {} indices", 
            mesh.m_Name, mesh.m_Vertices.size(), mesh.m_Indices.size());
    }
  
private:
    void ValidateGeometry()
    {
        if (m_Vertices.empty()) {
            SEDX_CORE_WARN_TAG("MESH", "Mesh '{}' has no vertices", m_Name);
        }
      
        if (m_Indices.empty()) {
            SEDX_CORE_WARN_TAG("MESH", "Mesh '{}' has no indices", m_Name);
        }
      
        // Validate index bounds
        for (uint32_t index : m_Indices) {
            if (index >= m_Vertices.size()) {
                SEDX_CORE_ERROR_TAG("MESH", "Invalid index {} in mesh '{}'", index, m_Name);
                throw std::runtime_error("Invalid mesh indices");
            }
        }
    }
};
```

### Asset Management Integration

```cpp
class AssetCache
{
private:
    std::unordered_map<UUID, std::vector<uint8_t>> m_SerializedAssets;
    std::unordered_map<UUID, std::chrono::system_clock::time_point> m_LastAccess;
  
public:
    template<typename T>
    void CacheAsset(const UUID& id, const T& asset)
    {
        SEDX_PROFILE_SCOPE("AssetCache::CacheAsset");
      
        MemoryWriter writer;
      
        using namespace SceneryEditorX::Serialization;
        if (Serialize(&writer, asset)) {
            m_SerializedAssets[id] = writer.GetBuffer();
            m_LastAccess[id] = std::chrono::system_clock::now();
          
            SEDX_CORE_INFO_TAG("ASSET_CACHE", "Cached asset {} ({} bytes)", 
                id.ToString(), writer.GetBuffer().size());
        }
    }
  
    template<typename T>
    std::optional<T> LoadAsset(const UUID& id)
    {
        SEDX_PROFILE_SCOPE("AssetCache::LoadAsset");
      
        auto it = m_SerializedAssets.find(id);
        if (it == m_SerializedAssets.end()) {
            return std::nullopt;
        }
      
        MemoryReader reader(it->second);
        T asset;
      
        using namespace SceneryEditorX::Serialization;
        if (Deserialize(&reader, asset)) {
            m_LastAccess[id] = std::chrono::system_clock::now();
          
            SEDX_CORE_INFO_TAG("ASSET_CACHE", "Loaded asset {} from cache", id.ToString());
            return asset;
        }
      
        return std::nullopt;
    }
  
    void SaveCacheToFile(const std::string& filename)
    {
        FileWriter writer(filename);
        if (!writer) return;
      
        // Write cache header
        struct CacheHeader {
            char magic[4] = {'A', 'C', 'H', 'E'};
            uint32_t version = 1;
            uint32_t assetCount;
        };
      
        CacheHeader header;
        header.assetCount = static_cast<uint32_t>(m_SerializedAssets.size());
        writer.WriteRaw(header);
      
        // Write each cached asset
        for (const auto& [id, data] : m_SerializedAssets) {
            writer.WriteRaw(static_cast<uint64_t>(id));
            writer.WriteRaw<uint32_t>(static_cast<uint32_t>(data.size()));
            writer.WriteData(reinterpret_cast<const char*>(data.data()), data.size());
        }
      
        SEDX_CORE_INFO_TAG("ASSET_CACHE", "Saved {} assets to cache file", header.assetCount);
    }
  
    bool LoadCacheFromFile(const std::string& filename)
    {
        FileReader reader(filename);
        if (!reader) return false;
      
        // Read and validate header
        struct CacheHeader {
            char magic[4];
            uint32_t version;
            uint32_t assetCount;
        };
      
        CacheHeader header;
        reader.ReadRaw(header);
      
        if (strncmp(header.magic, "ACHE", 4) != 0) {
            SEDX_CORE_ERROR_TAG("ASSET_CACHE", "Invalid cache file format");
            return false;
        }
      
        if (header.version != 1) {
            SEDX_CORE_ERROR_TAG("ASSET_CACHE", "Unsupported cache version: {}", header.version);
            return false;
        }
      
        // Clear existing cache
        m_SerializedAssets.clear();
        m_LastAccess.clear();
      
        // Load each asset
        for (uint32_t i = 0; i < header.assetCount; ++i) {
            uint64_t rawId;
            uint32_t dataSize;
          
            reader.ReadRaw(rawId);
            reader.ReadRaw(dataSize);
          
            UUID id(rawId);
            std::vector<uint8_t> data(dataSize);
            reader.ReadData(reinterpret_cast<char*>(data.data()), dataSize);
          
            m_SerializedAssets[id] = std::move(data);
            m_LastAccess[id] = std::chrono::system_clock::now();
        }
      
        SEDX_CORE_INFO_TAG("ASSET_CACHE", "Loaded {} assets from cache file", header.assetCount);
        return true;
    }
};
```

### Network Serialization

```cpp
class NetworkPacket
{
public:
    enum class Type : uint8_t
    {
        PlayerUpdate = 1,
        ObjectSpawn = 2,
        ObjectDestroy = 3,
        ChatMessage = 4
    };
  
private:
    Type m_Type;
    uint32_t m_Timestamp;
    std::vector<uint8_t> m_Data;
  
public:
    template<typename T>
    static NetworkPacket Create(Type type, const T& data)
    {
        NetworkPacket packet;
        packet.m_Type = type;
        packet.m_Timestamp = GetCurrentTimestamp();
      
        // Serialize data to internal buffer
        MemoryWriter writer;
        using namespace SceneryEditorX::Serialization;
      
        if (Serialize(&writer, data)) {
            packet.m_Data = writer.GetBuffer();
        }
      
        return packet;
    }
  
    template<typename T>
    bool ExtractData(T& data) const
    {
        if (m_Data.empty()) return false;
      
        MemoryReader reader(m_Data);
        using namespace SceneryEditorX::Serialization;
      
        return Deserialize(&reader, data);
    }
  
    std::vector<uint8_t> Serialize() const
    {
        MemoryWriter writer;
      
        // Packet header
        writer.WriteRaw(m_Type);
        writer.WriteRaw(m_Timestamp);
        writer.WriteRaw<uint32_t>(static_cast<uint32_t>(m_Data.size()));
      
        // Packet data
        if (!m_Data.empty()) {
            writer.WriteData(reinterpret_cast<const char*>(m_Data.data()), m_Data.size());
        }
      
        return writer.GetBuffer();
    }
  
    static NetworkPacket Deserialize(const std::vector<uint8_t>& buffer)
    {
        MemoryReader reader(buffer);
      
        NetworkPacket packet;
        reader.ReadRaw(packet.m_Type);
        reader.ReadRaw(packet.m_Timestamp);
      
        uint32_t dataSize;
        reader.ReadRaw(dataSize);
      
        if (dataSize > 0) {
            packet.m_Data.resize(dataSize);
            reader.ReadData(reinterpret_cast<char*>(packet.m_Data.data()), dataSize);
        }
      
        return packet;
    }
  
    Type GetType() const { return m_Type; }
    uint32_t GetTimestamp() const { return m_Timestamp; }
};

// Usage example
struct PlayerUpdate
{
    UUID playerId;
    Vec3 position;
    Vec3 velocity;
    float health;
};

SERIALIZABLE(PlayerUpdate,
    &PlayerUpdate::playerId,
    &PlayerUpdate::position,
    &PlayerUpdate::velocity,
    &PlayerUpdate::health
);

void SendPlayerUpdate(NetworkConnection& connection, const PlayerUpdate& update)
{
    auto packet = NetworkPacket::Create(NetworkPacket::Type::PlayerUpdate, update);
    auto serializedData = packet.Serialize();
  
    connection.Send(serializedData.data(), serializedData.size());
  
    SEDX_CORE_INFO_TAG("NETWORK", "Sent player update for {}", update.playerId.ToString());
}

void HandleReceivedPacket(const std::vector<uint8_t>& data)
{
    auto packet = NetworkPacket::Deserialize(data);
  
    switch (packet.GetType()) {
        case NetworkPacket::Type::PlayerUpdate: {
            PlayerUpdate update;
            if (packet.ExtractData(update)) {
                ProcessPlayerUpdate(update);
            }
            break;
        }
        // Handle other packet types...
    }
}
```

## File Format Examples

### Custom File Format with Header

```cpp
struct ModelFile
{
    struct Header
    {
        char magic[4] = {'S', 'M', 'D', 'L'};  // Scenery Model
        uint16_t versionMajor = 1;
        uint16_t versionMinor = 0;
        uint32_t flags = 0;
        uint64_t creationTime;
        uint64_t dataOffset;
        uint32_t checksum;
    };
  
    Header header;
    std::string name;
    std::string author;
    std::vector<Mesh> meshes;
    std::vector<Material> materials;
    std::map<std::string, std::string> metadata;
  
    bool SaveToFile(const std::string& filename) const
    {
        FileWriter writer(filename);
        if (!writer) return false;
      
        // Prepare header
        Header fileHeader = header;
        fileHeader.creationTime = GetCurrentTimestamp();
        fileHeader.dataOffset = sizeof(Header);
      
        // Write header placeholder
        writer.WriteRaw(fileHeader);
      
        // Calculate data start position
        uint64_t dataStart = writer.GetStreamPosition();
      
        // Write data
        using namespace SceneryEditorX::Serialization;
        bool success = Serialize(&writer,
            name,
            author,
            meshes,
            materials,
            metadata
        );
      
        if (!success) return false;
      
        // Calculate checksum
        uint64_t dataEnd = writer.GetStreamPosition();
        uint64_t dataSize = dataEnd - dataStart;
      
        // Update header with final information
        fileHeader.dataOffset = dataStart;
        fileHeader.checksum = CalculateChecksum(writer, dataStart, dataSize);
      
        // Write final header
        writer.SetStreamPosition(0);
        writer.WriteRaw(fileHeader);
      
        SEDX_CORE_INFO_TAG("MODEL", "Saved model '{}' with {} meshes, {} materials", 
            name, meshes.size(), materials.size());
      
        return true;
    }
  
    bool LoadFromFile(const std::string& filename)
    {
        FileReader reader(filename);
        if (!reader) return false;
      
        // Read and validate header
        reader.ReadRaw(header);
      
        if (strncmp(header.magic, "SMDL", 4) != 0) {
            SEDX_CORE_ERROR_TAG("MODEL", "Invalid model file format");
            return false;
        }
      
        if (header.versionMajor > 1) {
            SEDX_CORE_ERROR_TAG("MODEL", "Unsupported model version: {}.{}", 
                header.versionMajor, header.versionMinor);
            return false;
        }
      
        // Seek to data section
        reader.SetStreamPosition(header.dataOffset);
      
        // Read data
        using namespace SceneryEditorX::Serialization;
        bool success = Deserialize(&reader,
            name,
            author,
            meshes,
            materials,
            metadata
        );
      
        if (!success) return false;
      
        // Validate checksum
        uint64_t currentPos = reader.GetStreamPosition();
        uint64_t dataSize = currentPos - header.dataOffset;
        uint32_t calculatedChecksum = CalculateChecksum(reader, header.dataOffset, dataSize);
      
        if (calculatedChecksum != header.checksum) {
            SEDX_CORE_ERROR_TAG("MODEL", "Model file checksum mismatch");
            return false;
        }
      
        SEDX_CORE_INFO_TAG("MODEL", "Loaded model '{}' with {} meshes, {} materials", 
            name, meshes.size(), materials.size());
      
        return true;
    }
  
private:
    uint32_t CalculateChecksum(SerializeReader& reader, uint64_t offset, uint64_t size) const
    {
        // Simple CRC32 checksum implementation
        // In practice, use a proper checksum algorithm
        uint32_t checksum = 0;
      
        uint64_t savedPos = reader.GetStreamPosition();
        reader.SetStreamPosition(offset);
      
        const size_t bufferSize = 4096;
        std::vector<uint8_t> buffer(bufferSize);
      
        uint64_t remaining = size;
        while (remaining > 0) {
            size_t toRead = std::min(remaining, static_cast<uint64_t>(bufferSize));
          
            if (!reader.ReadData(reinterpret_cast<char*>(buffer.data()), toRead)) {
                break;
            }
          
            for (size_t i = 0; i < toRead; ++i) {
                checksum = (checksum << 1) ^ buffer[i];
            }
          
            remaining -= toRead;
        }
      
        reader.SetStreamPosition(savedPos);
        return checksum;
    }
  
    uint32_t CalculateChecksum(SerializeWriter& writer, uint64_t offset, uint64_t size) const
    {
        // For writer, we'd need to read back the data or use a different approach
        // This is a simplified example
        return 0xDEADBEEF; // Placeholder
    }
};
```

### Shader Pack File Format

```cpp
class ShaderPackFile
{
public:
    struct Header
    {
        char magic[4] = {'X', 'S', 'P', 'K'};
        uint32_t version = 1;
        uint32_t shaderCount;
        uint32_t totalSize;
    };
  
    struct ShaderEntry
    {
        std::string name;
        uint32_t stage; // Vertex, Fragment, etc.
        std::vector<uint8_t> spirvCode;
        std::map<std::string, uint32_t> uniformLocations;
    };
  
private:
    std::vector<ShaderEntry> m_Shaders;
  
public:
    void AddShader(const ShaderEntry& shader)
    {
        m_Shaders.push_back(shader);
    }
  
    bool SaveToFile(const std::string& filename) const
    {
        FileWriter writer(filename);
        if (!writer) return false;
      
        // Calculate total size
        uint32_t totalSize = 0;
        for (const auto& shader : m_Shaders) {
            totalSize += shader.name.size() + sizeof(uint32_t); // name length + name
            totalSize += sizeof(uint32_t); // stage
            totalSize += sizeof(uint32_t) + shader.spirvCode.size(); // code size + code
            totalSize += sizeof(uint32_t); // uniform count
            for (const auto& [name, location] : shader.uniformLocations) {
                totalSize += sizeof(uint32_t) + name.size(); // name length + name
                totalSize += sizeof(uint32_t); // location
            }
        }
      
        // Write header
        Header header;
        header.shaderCount = static_cast<uint32_t>(m_Shaders.size());
        header.totalSize = totalSize;
        writer.WriteRaw(header);
      
        // Write shaders
        for (const auto& shader : m_Shaders) {
            writer.WriteString(shader.name);
            writer.WriteRaw(shader.stage);
            writer.WriteArray(shader.spirvCode);
            writer.WriteMap(shader.uniformLocations);
        }
      
        SEDX_CORE_INFO_TAG("SHADER_PACK", "Saved shader pack with {} shaders", m_Shaders.size());
        return true;
    }
  
    bool LoadFromFile(const std::string& filename)
    {
        FileReader reader(filename);
        if (!reader) return false;
      
        // Read header
        Header header;
        reader.ReadRaw(header);
      
        if (strncmp(header.magic, "XSPK", 4) != 0) {
            SEDX_CORE_ERROR_TAG("SHADER_PACK", "Invalid shader pack format");
            return false;
        }
      
        // Read shaders
        m_Shaders.clear();
        m_Shaders.reserve(header.shaderCount);
      
        for (uint32_t i = 0; i < header.shaderCount; ++i) {
            ShaderEntry shader;
          
            reader.ReadString(shader.name);
            reader.ReadRaw(shader.stage);
            reader.ReadArray(shader.spirvCode);
            reader.ReadMap(shader.uniformLocations);
          
            m_Shaders.push_back(std::move(shader));
        }
      
        SEDX_CORE_INFO_TAG("SHADER_PACK", "Loaded shader pack with {} shaders", m_Shaders.size());
        return true;
    }
  
    const std::vector<ShaderEntry>& GetShaders() const { return m_Shaders; }
};
```

## Performance Examples

### Bulk Data Processing

```cpp
class TerrainHeightmap
{
private:
    uint32_t m_Width, m_Height;
    std::vector<float> m_Heights;
  
public:
    TerrainHeightmap(uint32_t width, uint32_t height)
        : m_Width(width), m_Height(height)
    {
        m_Heights.resize(width * height);
    }
  
    // Optimized serialization for large datasets
    static void Serialize(SerializeWriter* writer, const TerrainHeightmap& terrain)
    {
        SEDX_PROFILE_SCOPE("TerrainHeightmap::Serialize");
      
        // Write dimensions
        writer->WriteRaw(terrain.m_Width);
        writer->WriteRaw(terrain.m_Height);
      
        // Write height data as a single block for better performance
        uint32_t dataSize = static_cast<uint32_t>(terrain.m_Heights.size());
        writer->WriteRaw(dataSize);
      
        if (dataSize > 0) {
            writer->WriteData(
                reinterpret_cast<const char*>(terrain.m_Heights.data()),
                dataSize * sizeof(float)
            );
        }
      
        SEDX_CORE_INFO_TAG("TERRAIN", "Serialized heightmap {}x{} ({} KB)", 
            terrain.m_Width, terrain.m_Height, (dataSize * sizeof(float)) / 1024);
    }
  
    static void Deserialize(SerializeReader* reader, TerrainHeightmap& terrain)
    {
        SEDX_PROFILE_SCOPE("TerrainHeightmap::Deserialize");
      
        // Read dimensions
        reader->ReadRaw(terrain.m_Width);
        reader->ReadRaw(terrain.m_Height);
      
        // Read height data
        uint32_t dataSize;
        reader->ReadRaw(dataSize);
      
        terrain.m_Heights.resize(dataSize);
      
        if (dataSize > 0) {
            reader->ReadData(
                reinterpret_cast<char*>(terrain.m_Heights.data()),
                dataSize * sizeof(float)
            );
        }
      
        SEDX_CORE_INFO_TAG("TERRAIN", "Loaded heightmap {}x{} ({} KB)", 
            terrain.m_Width, terrain.m_Height, (dataSize * sizeof(float)) / 1024);
    }
};
```

### Streaming Serialization

```cpp
class StreamingLogger
{
private:
    std::unique_ptr<SerializeWriter> m_Writer;
    std::mutex m_WriteMutex;
  
public:
    bool Initialize(const std::string& filename)
    {
        m_Writer = std::make_unique<FileWriter>(filename);
      
        if (!m_Writer || !*m_Writer) {
            SEDX_CORE_ERROR_TAG("STREAMING_LOG", "Failed to open log file: {}", filename);
            return false;
        }
      
        // Write log header
        struct LogHeader {
            char magic[4] = {'S', 'L', 'O', 'G'};
            uint32_t version = 1;
            uint64_t startTime;
        };
      
        LogHeader header;
        header.startTime = GetCurrentTimestamp();
        m_Writer->WriteRaw(header);
      
        return true;
    }
  
    template<typename T>
    void LogEntry(const T& entry)
    {
        std::lock_guard<std::mutex> lock(m_WriteMutex);
      
        if (!m_Writer || !*m_Writer) return;
      
        // Write timestamp
        uint64_t timestamp = GetCurrentTimestamp();
        m_Writer->WriteRaw(timestamp);
      
        // Write entry
        using namespace SceneryEditorX::Serialization;
        Serialize(m_Writer.get(), entry);
    }
  
    void Flush()
    {
        std::lock_guard<std::mutex> lock(m_WriteMutex);
        // For file streams, this would call flush on the underlying stream
    }
};

// Usage
struct LogEntry
{
    std::string level;
    std::string message;
    std::string file;
    uint32_t line;
};

SERIALIZABLE(LogEntry,
    &LogEntry::level,
    &LogEntry::message,
    &LogEntry::file,
    &LogEntry::line
);

StreamingLogger logger;
logger.Initialize("application.log");

// Log entries throughout application runtime
logger.LogEntry(LogEntry{"INFO", "Application started", __FILE__, __LINE__});
logger.LogEntry(LogEntry{"WARN", "Low memory warning", __FILE__, __LINE__});
```

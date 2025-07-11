# Scenery Editor X - Encoding System Documentation

---

## Overview

The SceneryEditorX encoding system provides a comprehensive set of utilities for data encoding, decoding, and hashing operations. This system is designed to handle binary data transformation, unique identifier generation, and cryptographic hash operations essential for the 3D scenery editor's asset management and data serialization needs.

---

## Architecture

The encoding system is implemented as a static utility class within the `SceneryEditorX` namespace, providing thread-safe, stateless operations for various encoding and hashing requirements.

### Core Components

#### 1. UUID Structure

```cpp
struct UUID
{
    uint64_t ID;  ///< 64-bit unique identifier value
};
```

The UUID structure provides a standardized way to represent unique identifiers throughout the application. It uses a 64-bit integer for efficient storage and comparison operations.

#### 2. Encoding Class

The main `Encoding` class contains all static methods for encoding, decoding, and hashing operations. All methods are marked with the `GLOBAL` macro (defined as `static`) to ensure they don't require instantiation.

---

## Core Functionality

### Base64 Encoding/Decoding

#### Base64 Encoding

```cpp
GLOBAL std::string EncodeBase64(const unsigned char *input, size_t len);
```

**Purpose**: Converts binary data to Base64 string representation for safe text-based transmission and storage.

**Algorithm Details**:

- Processes input data in 3-byte blocks
- Each 3-byte block is converted to 4 Base64 characters
- Uses standard Base64 alphabet: `A-Z`, `a-z`, `0-9`, `+`, `/`
- Applies padding with `=` characters when input length is not divisible by 3
- Handles incomplete final blocks with proper padding

**Example Usage**:

```cpp
const char* binaryData = "Hello World";
size_t dataLength = strlen(binaryData);
std::string encoded = SceneryEditorX::Encoding::EncodeBase64(
    reinterpret_cast<const unsigned char*>(binaryData), 
    dataLength
);
// Result: "SGVsbG8gV29ybGQ="
```

#### Base64 Decoding

```cpp
GLOBAL std::vector<uint8_t> DecodeBase64(const std::string &input);
```

**Purpose**: Converts Base64 encoded strings back to their original binary representation.

**Algorithm Details**:

- Processes input in 4-character blocks
- Each 4-character block is converted back to 3 bytes of binary data
- Handles padding characters ('=') correctly
- Ignores invalid Base64 characters
- Returns a vector of bytes for memory-safe handling

**Example Usage**:

```cpp
std::string base64Input = "SGVsbG8gV29ybGQ=";
std::vector<uint8_t> decoded = SceneryEditorX::Encoding::DecodeBase64(base64Input);
// Convert back to string for verification
std::string result(decoded.begin(), decoded.end());
// Result: "Hello World"
```

### Hashing Operations

#### UUID Hashing

```cpp
GLOBAL uint32_t HashUUID(const std::vector<uint32_t> &vec);
```

**Purpose**: Generates deterministic hash values from collections of 32-bit integers, useful for creating unique identifiers from multiple data points.

**Algorithm Details**:

- Sorts input vector to ensure deterministic results regardless of input order
- Uses FNV-1a-inspired algorithm with magic number `0x9e3779b9` (golden ratio constant)
- Combines seed value with vector size and individual elements
- Produces 32-bit hash suitable for UUID generation

**Example Usage**:

```cpp
std::vector<uint32_t> sceneData = {123, 456, 789, 101112};
uint32_t sceneHash = SceneryEditorX::Encoding::HashUUID(sceneData);
// Use hash as part of scene identifier
SceneryEditorX::UUID sceneId = {static_cast<uint64_t>(sceneHash)};
```

#### Template Hash Combination

```cpp
template <typename T>
void HashCombine(uint32_t &h, const T &v);
```

**Purpose**: Combines existing hash values with new data using the FNV-1a algorithm, enabling incremental hash building.

**Algorithm Details**:

- Uses `std::hash<T>` to generate hash for the input value
- Applies FNV-1a mixing with golden ratio constant
- Performs bit shifting operations for better distribution
- Updates hash value in-place for efficiency

**Example Usage**:

```cpp
uint32_t combinedHash = 0;
SceneryEditorX::Encoding::HashCombine(combinedHash, std::string("texture_name"));
SceneryEditorX::Encoding::HashCombine(combinedHash, 1024); // texture size
SceneryEditorX::Encoding::HashCombine(combinedHash, 768);  // texture height
// Result: Hash representing the combination of all inputs
```

#### Memory Block Hash Combination

```cpp
GLOBAL void HashCombine(uint32_t &h, void *ptr, const uint32_t size);
```

**Purpose**: Creates hash values from arbitrary memory blocks, useful for hashing complex data structures or binary data.

**Algorithm Details**:

- Uses `std::string_view` to create a view over the memory block
- Applies `std::hash<std::string_view>` for efficient hashing
- Overwrites the existing hash value (doesn't combine)
- Handles raw memory safely through string_view abstraction

**Example Usage**:

```cpp
struct VertexData {
    float x, y, z;
    float nx, ny, nz;
    float u, v;
};

VertexData vertex = {1.0f, 2.0f, 3.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f};
uint32_t vertexHash = 0;
SceneryEditorX::Encoding::HashCombine(vertexHash, &vertex, sizeof(VertexData));
```



---

## Use Cases and Applications

### 1. Asset Serialization

The encoding system is essential for serializing complex 3D assets and scene data:

```cpp
// Serialize mesh data to Base64 for storage
class MeshSerializer
{
public:
    std::string SerializeMesh(const Mesh& mesh)
    {
        // Convert mesh data to binary format
        std::vector<uint8_t> binaryData = mesh.ToBinary();
      
        // Encode to Base64 for safe storage
        return SceneryEditorX::Encoding::EncodeBase64(
            binaryData.data(), 
            binaryData.size()
        );
    }
  
    Mesh DeserializeMesh(const std::string& base64Data)
    {
        // Decode from Base64
        std::vector<uint8_t> binaryData = 
            SceneryEditorX::Encoding::DecodeBase64(base64Data);
      
        // Reconstruct mesh from binary data
        return Mesh::FromBinary(binaryData);
    }
};
```

### 2. Scene Object Identification

Generate unique identifiers for scene objects based on their properties:

```cpp
class SceneObject
{
private:
    SceneryEditorX::UUID m_ID;
  
public:
    void GenerateID(const std::string& name, const glm::vec3& position, 
                   uint32_t typeID)
    {
        uint32_t hash = 0;
      
        // Combine various properties to create unique ID
        SceneryEditorX::Encoding::HashCombine(hash, name);
        SceneryEditorX::Encoding::HashCombine(hash, position.x);
        SceneryEditorX::Encoding::HashCombine(hash, position.y);
        SceneryEditorX::Encoding::HashCombine(hash, position.z);
        SceneryEditorX::Encoding::HashCombine(hash, typeID);
      
        m_ID.ID = static_cast<uint64_t>(hash);
    }
};
```

### 3. Material and Texture Hashing

Create efficient lookups for materials and textures:

```cpp
class MaterialManager
{
private:
    std::unordered_map<uint32_t, Ref<Material>> m_MaterialCache;
  
public:
    uint32_t CreateMaterialHash(const MaterialProperties& props)
    {
        uint32_t hash = 0;
      
        // Hash material properties
        SceneryEditorX::Encoding::HashCombine(hash, props.diffuseColor.r);
        SceneryEditorX::Encoding::HashCombine(hash, props.diffuseColor.g);
        SceneryEditorX::Encoding::HashCombine(hash, props.diffuseColor.b);
        SceneryEditorX::Encoding::HashCombine(hash, props.roughness);
        SceneryEditorX::Encoding::HashCombine(hash, props.metallic);
        SceneryEditorX::Encoding::HashCombine(hash, props.diffuseTexturePath);
      
        return hash;
    }
  
    Ref<Material> GetOrCreateMaterial(const MaterialProperties& props)
    {
        uint32_t hash = CreateMaterialHash(props);
      
        auto it = m_MaterialCache.find(hash);
        if (it != m_MaterialCache.end())
            return it->second;
      
        // Create new material if not found
        auto material = CreateRef<Material>(props);
        m_MaterialCache[hash] = material;
        return material;
    }
};
```

### 4. Configuration Data Encoding

Store configuration data safely in text-based formats:

```cpp
class ConfigurationManager
{
public:
    void SaveBinaryConfig(const std::string& filename, 
                         const void* data, size_t size)
    {
        // Encode binary configuration to Base64
        std::string encoded = SceneryEditorX::Encoding::EncodeBase64(
            static_cast<const unsigned char*>(data), size
        );
      
        // Save to configuration file
        std::ofstream file(filename);
        file << "config_data=" << encoded << std::endl;
    }
  
    std::vector<uint8_t> LoadBinaryConfig(const std::string& filename)
    {
        std::ifstream file(filename);
        std::string line;
      
        if (std::getline(file, line) && line.starts_with("config_data="))
        {
            std::string encoded = line.substr(12); // Skip "config_data="
            return SceneryEditorX::Encoding::DecodeBase64(encoded);
        }
      
        return {};
    }
};
```

### 5. Shader Cache Management

Use hashing for efficient shader cache management:

```cpp
class ShaderManager
{
private:
    std::unordered_map<uint32_t, Ref<Shader>> m_ShaderCache;
  
public:
    uint32_t CreateShaderHash(const std::string& vertexSource,
                             const std::string& fragmentSource,
                             const std::vector<std::string>& defines)
    {
        uint32_t hash = 0;
      
        // Hash shader sources and compilation defines
        SceneryEditorX::Encoding::HashCombine(hash, vertexSource);
        SceneryEditorX::Encoding::HashCombine(hash, fragmentSource);
      
        for (const auto& define : defines)
            SceneryEditorX::Encoding::HashCombine(hash, define);
      
        return hash;
    }
  
    Ref<Shader> GetOrCompileShader(const std::string& vertexSource,
                                  const std::string& fragmentSource,
                                  const std::vector<std::string>& defines)
    {
        uint32_t hash = CreateShaderHash(vertexSource, fragmentSource, defines);
      
        auto it = m_ShaderCache.find(hash);
        if (it != m_ShaderCache.end())
        {
            SEDX_CORE_INFO_TAG("SHADER", "Using cached shader with hash: {}", hash);
            return it->second;
        }
      
        // Compile new shader
        SEDX_CORE_INFO_TAG("SHADER", "Compiling new shader with hash: {}", hash);
        auto shader = CreateRef<Shader>(vertexSource, fragmentSource, defines);
        m_ShaderCache[hash] = shader;
        return shader;
    }
};
```



---

## Performance Considerations

### Memory Efficiency

- **Base64 Operations**: Use string reserves for known output sizes
- **Hash Operations**: Prefer in-place updates with `HashCombine`
- **UUID Generation**: Use move semantics for vector operations

### Thread Safety

All encoding operations are thread-safe as they:

- Use only static methods with no shared state
- Operate on local variables and parameters
- Don't modify global state

### Optimization Tips

1. **Pre-allocate strings** for Base64 operations when output size is known:

   ```cpp
   std::string result;
   result.reserve((inputSize + 2) / 3 * 4); // Base64 expansion factor
   ```
2. **Batch hash operations** to minimize function call overhead:

   ```cpp
   uint32_t hash = 0;
   for (const auto& item : collection)
       SceneryEditorX::Encoding::HashCombine(hash, item);
   ```
3. **Use memory hashing** for complex structures instead of individual field hashing:

   ```cpp
   // More efficient for large structures
   SceneryEditorX::Encoding::HashCombine(hash, &structure, sizeof(structure));
   ```

## Error Handling and Best Practices

### Input Validation

```cpp
bool ValidateBase64Input(const std::string& input)
{
    // Check for valid Base64 characters and proper padding
    if (input.empty()) return false;
  
    size_t paddingCount = 0;
    for (char c : input)
    {
        if (c == '=') paddingCount++;
        else if (paddingCount > 0) return false; // Padding should only be at end
        else if (!IsBase64(static_cast<unsigned char>(c))) return false;
    }
  
    return paddingCount <= 2; // Maximum 2 padding characters
}
```

### Safe Memory Operations

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

### Integration with Module System

```cpp
class EncodingModule : public Module
{
public:
    explicit EncodingModule() : Module("EncodingModule") {}
  
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("ENCODING", "Initializing encoding system");
      
        // Initialize any required encoding tables or caches
        InitializeEncodingTables();
      
        SEDX_CORE_INFO_TAG("ENCODING", "Encoding system ready");
    }
  
    void OnDetach() override
    {
        SEDX_CORE_INFO_TAG("ENCODING", "Shutting down encoding system");
        // Cleanup any allocated resources
    }
  
private:
    void InitializeEncodingTables()
    {
        // Validate Base64 character table
        SEDX_CORE_ASSERT(base64Chars.size() == 64, 
                        "Base64 character table must contain exactly 64 characters");
      
        // Perform any other initialization
    }
};
```

## Testing and Validation

### Unit Test Examples

```cpp
TEST_CASE("Base64 Encoding/Decoding", "[encoding]")
{
    SECTION("Round-trip encoding")
    {
        const std::string original = "Hello, SceneryEditorX!";
        const auto* input = reinterpret_cast<const unsigned char*>(original.c_str());
      
        std::string encoded = SceneryEditorX::Encoding::EncodeBase64(input, original.length());
        std::vector<uint8_t> decoded = SceneryEditorX::Encoding::DecodeBase64(encoded);
      
        std::string result(decoded.begin(), decoded.end());
        REQUIRE(result == original);
    }
  
    SECTION("Empty input handling")
    {
        std::string encoded = SceneryEditorX::Encoding::EncodeBase64(nullptr, 0);
        REQUIRE(encoded.empty());
      
        std::vector<uint8_t> decoded = SceneryEditorX::Encoding::DecodeBase64("");
        REQUIRE(decoded.empty());
    }
}

TEST_CASE("Hash Operations", "[encoding]")
{
    SECTION("Hash combination is deterministic")
    {
        uint32_t hash1 = 0, hash2 = 0;
      
        SceneryEditorX::Encoding::HashCombine(hash1, std::string("test"));
        SceneryEditorX::Encoding::HashCombine(hash1, 42);
      
        SceneryEditorX::Encoding::HashCombine(hash2, std::string("test"));
        SceneryEditorX::Encoding::HashCombine(hash2, 42);
      
        REQUIRE(hash1 == hash2);
    }
  
    SECTION("UUID hash handles empty vector")
    {
        std::vector<uint32_t> empty;
        uint32_t hash = SceneryEditorX::Encoding::HashUUID(empty);
        REQUIRE(hash == 0); // Empty vector should produce hash of 0
    }
}
```

# Scenery Editor X - SerializeWriter API Reference

---

## Overview

`SerializeWriter` is the abstract base class providing binary data writing capabilities for the Scenery Editor X serialization system. It defines a standardized interface for writing various data types to different output targets while maintaining type safety and performance.

## Class Definition

```cpp
namespace SceneryEditorX
{
    class SerializeWriter
    {
    public:
        virtual ~SerializeWriter() = default;

        // Core virtual interface
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
      
        // Template methods
        template<typename T> void WriteRaw(const T& type);
        template<typename T> void WriteObject(const T& obj);
        template<typename Key, typename Value> void WriteMap(const std::map<Key, Value>& map, bool writeSize = true);
        template<typename Key, typename Value> void WriteMap(const std::unordered_map<Key, Value>& map, bool writeSize = true);
        template<typename Value> void WriteMap(const std::unordered_map<std::string, Value>& map, bool writeSize = true);
        template<typename T> void WriteArray(const std::vector<T>& array, bool writeSize = true);
    };
}
```

## Virtual Interface

### IsStreamGood()

```cpp
[[nodiscard]] virtual bool IsStreamGood() const = 0;
```

**Purpose**: Check if the output stream is in a valid state for writing.

**Returns**: `true` if the stream is ready for write operations, `false` otherwise.

**Implementation Notes**:

- Must be implemented by derived classes
- Should check the underlying stream/buffer state
- Used internally by other methods to validate operations

**Example Implementation**:

```cpp
// File-based implementation
bool FileWriter::IsStreamGood() const 
{
    return m_FileStream.good() && m_FileStream.is_open();
}

// Memory-based implementation  
bool MemoryWriter::IsStreamGood() const 
{
    return m_Position <= m_Buffer.capacity();
}
```

### GetStreamPosition()

```cpp
virtual uint64_t GetStreamPosition() = 0;
```

**Purpose**: Get the current write position in the stream.

**Returns**: Current position as byte offset from beginning of stream.

**Implementation Notes**:

- Must be implemented by derived classes
- Used for seeking operations and size calculations
- Should return position in bytes, not elements

**Example Implementation**:

```cpp
uint64_t FileWriter::GetStreamPosition() 
{
    return static_cast<uint64_t>(m_FileStream.tellp());
}

uint64_t MemoryWriter::GetStreamPosition() 
{
    return m_Position;
}
```

### SetStreamPosition()

```cpp
virtual void SetStreamPosition(uint64_t position) = 0;
```

**Purpose**: Set the current write position in the stream.

**Parameters**:

- `position`: Byte offset from beginning of stream

**Implementation Notes**:

- Must be implemented by derived classes
- Used for seeking to specific locations
- Should handle bounds checking appropriately

**Example Implementation**:

```cpp
void FileWriter::SetStreamPosition(uint64_t position) 
{
    m_FileStream.seekp(position);
    SEDX_CORE_ASSERT(m_FileStream.good(), "Failed to seek to position {}", position);
}

void MemoryWriter::SetStreamPosition(uint64_t position) 
{
    SEDX_CORE_ASSERT(position <= m_Buffer.size(), "Position {} exceeds buffer size", position);
    m_Position = position;
}
```

### WriteData()

```cpp
virtual bool WriteData(const char* data, size_t size) = 0;
```

**Purpose**: Write raw binary data to the stream.

**Parameters**:

- `data`: Pointer to source data
- `size`: Number of bytes to write

**Returns**: `true` if write was successful, `false` otherwise.

**Implementation Notes**:

- Must be implemented by derived classes
- Core method used by all other write operations
- Should handle error conditions gracefully

**Example Implementation**:

```cpp
bool FileWriter::WriteData(const char* data, size_t size) 
{
    m_FileStream.write(data, size);
    return m_FileStream.good();
}

bool MemoryWriter::WriteData(const char* data, size_t size) 
{
    if (m_Position + size > m_Buffer.size()) {
        m_Buffer.resize(m_Position + size);
    }
    std::memcpy(m_Buffer.data() + m_Position, data, size);
    m_Position += size;
    return true;
}
```

## Convenience Methods

### Operator bool()

```cpp
explicit operator bool() const { return IsStreamGood(); }
```

**Purpose**: Allow writer to be used in boolean contexts.

**Returns**: Result of `IsStreamGood()`

**Usage**:

```cpp
FileWriter writer("output.bin");
if (writer) {
    // Safe to write
    writer.WriteRaw(42);
}
```

## Specialized Write Methods

### WriteBuffer()

```cpp
void WriteBuffer(Buffer buffer, bool writeSize = true);
```

**Purpose**: Write a Vulkan buffer to the stream.

**Parameters**:

- `buffer`: Vulkan buffer containing data to write
- `writeSize`: Whether to write buffer size before data (default: true)

**Behavior**:

- If `writeSize` is true, writes buffer size as `uint64_t` first
- Writes buffer data directly to stream
- Handles buffer memory mapping internally

**Usage**:

```cpp
Buffer vertexBuffer = CreateVertexBuffer(vertices);
writer.WriteBuffer(vertexBuffer); // Writes size + data
writer.WriteBuffer(vertexBuffer, false); // Writes only data
```

### WriteZero()

```cpp
void WriteZero(uint64_t size);
```

**Purpose**: Write zeros to the stream for padding or initialization.

**Parameters**:

- `size`: Number of zero bytes to write

**Use Cases**:

- Padding to alignment boundaries
- Initializing reserved space
- Creating placeholder data

**Usage**:

```cpp
// Align to 16-byte boundary
uint64_t currentPos = writer.GetStreamPosition();
uint64_t alignment = 16;
uint64_t padding = (alignment - (currentPos % alignment)) % alignment;
if (padding > 0) {
    writer.WriteZero(padding);
}
```

### WriteString()

```cpp
void WriteString(const std::string& string);
```

**Purpose**: Write a string with size prefix.

**Parameters**:

- `string`: String to write

**Format**:

- Writes string length as `size_t`
- Writes string characters (without null terminator)

**Usage**:

```cpp
std::string filename = "mesh_data.obj";
writer.WriteString(filename);

// Can be read back with SerializeReader::ReadString()
```

## Template Methods

### WriteRaw()

```cpp
template<typename T>
void WriteRaw(const T& type);
```

**Purpose**: Write a trivial type directly to stream.

**Parameters**:

- `type`: Object to write (must be trivially copyable)

**Constraints**:

- `T` should be trivially copyable
- No pointer types (undefined behavior)
- Platform-dependent for complex types

**Usage**:

```cpp
int32_t count = 42;
float position = 3.14f;
UUID objectId = GenerateUUID();

writer.WriteRaw(count);
writer.WriteRaw(position);  
writer.WriteRaw(objectId);

// Custom trivial struct
struct Header {
    uint32_t version;
    uint32_t flags;
    uint64_t timestamp;
};

Header header{1, 0x1000, GetCurrentTime()};
writer.WriteRaw(header);
```

### WriteObject()

```cpp
template<typename T>
void WriteObject(const T& obj);
```

**Purpose**: Write an object using its serialization interface.

**Parameters**:

- `obj`: Object to serialize

**Requirements**:

- Object must have static `Serialize(SerializeWriter*, const T&)` method
- Or use SERIALIZABLE macro
- Or have specialization in `Serialization::Impl` namespace

**Usage**:

```cpp
struct ComplexObject {
    std::string name;
    std::vector<float> data;
  
    static void Serialize(SerializeWriter* writer, const ComplexObject& obj) {
        writer->WriteString(obj.name);
        writer->WriteArray(obj.data);
    }
};

ComplexObject obj{"example", {1.0f, 2.0f, 3.0f}};
writer.WriteObject(obj);
```

### WriteMap() - std::map

```cpp
template<typename Key, typename Value>
void WriteMap(const std::map<Key, Value>& map, bool writeSize = true);
```

**Purpose**: Write a std::map to the stream.

**Parameters**:

- `map`: Map to serialize
- `writeSize`: Whether to write map size first (default: true)

**Format**:

- Writes map size as `uint32_t` (if writeSize is true)
- For each key-value pair:
  - Writes key (using WriteRaw for trivial types, WriteObject otherwise)
  - Writes value (using WriteRaw for trivial types, WriteObject otherwise)

**Usage**:

```cpp
std::map<std::string, float> settings;
settings["roughness"] = 0.5f;
settings["metallic"] = 0.2f;

writer.WriteMap(settings);

// For known-size maps
writer.WriteMap(settings, false); // Skip size writing
```

### WriteMap() - std::unordered_map

```cpp
template<typename Key, typename Value>
void WriteMap(const std::unordered_map<Key, Value>& map, bool writeSize = true);
```

**Purpose**: Write an unordered_map to the stream.

**Parameters**:

- `map`: Unordered map to serialize
- `writeSize`: Whether to write map size first (default: true)

**Behavior**: Identical to std::map version but for unordered_map.

**Usage**:

```cpp
std::unordered_map<uint32_t, Transform> objects;
objects[1] = Transform{};
objects[2] = Transform{};

writer.WriteMap(objects);
```

### WriteMap() - String Keys

```cpp
template<typename Value>
void WriteMap(const std::unordered_map<std::string, Value>& map, bool writeSize = true);
```

**Purpose**: Optimized version for string-keyed maps.

**Parameters**:

- `map`: String-keyed unordered map
- `writeSize`: Whether to write map size first (default: true)

**Optimization**: Uses `WriteString()` for keys instead of generic object serialization.

**Usage**:

```cpp
std::unordered_map<std::string, MaterialProperties> materials;
materials["wood"] = MaterialProperties{0.8f, 0.0f};
materials["metal"] = MaterialProperties{0.2f, 1.0f};

writer.WriteMap(materials);
```

### WriteArray()

```cpp
template<typename T>
void WriteArray(const std::vector<T>& array, bool writeSize = true);
```

**Purpose**: Write a vector to the stream.

**Parameters**:

- `array`: Vector to serialize
- `writeSize`: Whether to write array size first (default: true)

**Format**:

- Writes array size as `uint32_t` (if writeSize is true)
- For each element:
  - Uses WriteRaw for trivial types
  - Uses WriteObject for complex types

**Usage**:

```cpp
std::vector<int> numbers = {1, 2, 3, 4, 5};
std::vector<Transform> transforms(10);

writer.WriteArray(numbers);
writer.WriteArray(transforms);

// For known-size arrays
writer.WriteArray(numbers, false);
```

### WriteArray() - String Specialization

```cpp
template<>
void WriteArray(const std::vector<std::string>& array, bool writeSize = true);
```

**Purpose**: Optimized specialization for string vectors.

**Parameters**:

- `array`: Vector of strings
- `writeSize`: Whether to write array size first (default: true)

**Optimization**: Uses `WriteString()` for each element.

**Usage**:

```cpp
std::vector<std::string> filenames = {
    "texture1.png",
    "texture2.png", 
    "normalmap.png"
};

writer.WriteArray(filenames);
```

## Implementation Examples

### File Writer Implementation

```cpp
class FileWriter : public SerializeWriter
{
private:
    std::ofstream m_Stream;
    std::string m_Filename;
  
public:
    explicit FileWriter(const std::string& filename) 
        : m_Filename(filename)
        , m_Stream(filename, std::ios::binary)
    {
        if (!m_Stream.is_open()) {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Failed to open file for writing: {}", filename);
        }
    }
  
    ~FileWriter() override
    {
        if (m_Stream.is_open()) {
            m_Stream.close();
        }
    }
  
    bool IsStreamGood() const override 
    {
        return m_Stream.good() && m_Stream.is_open();
    }
  
    uint64_t GetStreamPosition() override 
    {
        return static_cast<uint64_t>(m_Stream.tellp());
    }
  
    void SetStreamPosition(uint64_t position) override 
    {
        m_Stream.seekp(position);
        if (!m_Stream.good()) {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Failed to seek to position {}", position);
        }
    }
  
    bool WriteData(const char* data, size_t size) override 
    {
        m_Stream.write(data, size);
        return m_Stream.good();
    }
};
```

### Memory Writer Implementation

```cpp
class MemoryWriter : public SerializeWriter  
{
private:
    std::vector<uint8_t> m_Buffer;
    size_t m_Position = 0;
  
public:
    MemoryWriter(size_t initialCapacity = 1024)
    {
        m_Buffer.reserve(initialCapacity);
    }
  
    bool IsStreamGood() const override 
    {
        return m_Position <= m_Buffer.capacity();
    }
  
    uint64_t GetStreamPosition() override 
    {
        return m_Position;
    }
  
    void SetStreamPosition(uint64_t position) override 
    {
        SEDX_CORE_ASSERT(position <= m_Buffer.size(), "Position {} exceeds buffer size", position);
        m_Position = static_cast<size_t>(position);
    }
  
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
```

### Network Writer Implementation

```cpp
class NetworkWriter : public SerializeWriter
{
private:
    std::vector<uint8_t> m_Buffer;
    size_t m_Position = 0;
    NetworkSocket& m_Socket;
    size_t m_FlushThreshold;
  
public:
    NetworkWriter(NetworkSocket& socket, size_t flushThreshold = 64 * 1024)
        : m_Socket(socket), m_FlushThreshold(flushThreshold)
    {
        m_Buffer.reserve(m_FlushThreshold * 2);
    }
  
    bool IsStreamGood() const override 
    {
        return m_Socket.IsConnected();
    }
  
    uint64_t GetStreamPosition() override 
    {
        return m_Position;
    }
  
    void SetStreamPosition(uint64_t position) override 
    {
        // Network streams typically don't support seeking
        SEDX_CORE_ERROR_TAG("SERIALIZATION", "Seeking not supported on network streams");
    }
  
    bool WriteData(const char* data, size_t size) override 
    {
        // Buffer data until flush threshold
        if (m_Buffer.size() + size > m_FlushThreshold) {
            if (!Flush()) return false;
        }
      
        m_Buffer.insert(m_Buffer.end(), data, data + size);
        m_Position += size;
        return true;
    }
  
    bool Flush()
    {
        if (m_Buffer.empty()) return true;
      
        bool success = m_Socket.Send(m_Buffer.data(), m_Buffer.size());
        if (success) {
            m_Buffer.clear();
        }
        return success;
    }
  
    ~NetworkWriter() override
    {
        Flush();
    }
};
```

## Error Handling

### Stream Validation

Always check stream state before performing operations:

```cpp
void SafeWriteOperation(SerializeWriter* writer, const Data& data)
{
    if (!writer || !writer->IsStreamGood()) {
        SEDX_CORE_ERROR_TAG("SERIALIZATION", "Invalid writer stream");
        return;
    }
  
    uint64_t startPosition = writer->GetStreamPosition();
  
    try {
        writer->WriteObject(data);
      
        if (!writer->IsStreamGood()) {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Stream error during write");
            writer->SetStreamPosition(startPosition); // Rollback
        }
    } catch (const std::exception& e) {
        SEDX_CORE_ERROR_TAG("SERIALIZATION", "Exception during write: {}", e.what());
        writer->SetStreamPosition(startPosition); // Rollback
    }
}
```

### Assertions and Validation

Use assertions for development-time validation:

```cpp
template<typename T>
void SerializeWriter::WriteRaw(const T& type)
{
    static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
    SEDX_CORE_ASSERT(IsStreamGood(), "Stream not in good state");
  
    const bool success = WriteData(reinterpret_cast<const char*>(&type), sizeof(T));
    SEDX_CORE_ASSERT(success, "Failed to write raw data");
}
```

## Performance Considerations

### Batch Writes

For better performance, batch related writes:

```cpp
// ❌ Multiple small writes
for (const auto& vertex : vertices) {
    writer.WriteRaw(vertex.position);
    writer.WriteRaw(vertex.normal);
    writer.WriteRaw(vertex.uv);
}

// ✅ Single large write
writer.WriteArray(vertices);
```

### Buffer Management

Pre-allocate buffers for known data sizes:

```cpp
class OptimizedMemoryWriter : public SerializeWriter
{
public:
    OptimizedMemoryWriter(size_t expectedSize)
    {
        m_Buffer.reserve(expectedSize * 1.2); // 20% overhead
    }
  
    // ... implementation
};
```

### Alignment Considerations

Consider data alignment for performance:

```cpp
void WriteAlignedData(SerializeWriter* writer, const AlignedData& data)
{
    // Ensure proper alignment
    uint64_t currentPos = writer->GetStreamPosition();
    constexpr uint64_t alignment = alignof(AlignedData);
  
    uint64_t padding = (alignment - (currentPos % alignment)) % alignment;
    if (padding > 0) {
        writer->WriteZero(padding);
    }
  
    writer->WriteRaw(data);
}
```

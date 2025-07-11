# Scenery Editor X - SerializeReader API Reference

---

## Overview

`SerializeReader` is the abstract base class providing binary data reading capabilities for the Scenery Editor X serialization system. It defines a standardized interface for reading various data types from different input sources while maintaining type safety and data integrity.

## Class Definition

```cpp
namespace SceneryEditorX
{
    class SerializeReader
    {
    public:
        virtual ~SerializeReader() = default;

        // Core virtual interface
        [[nodiscard]] virtual bool IsStreamGood() const = 0;
        virtual uint64_t GetStreamPosition() = 0;
        virtual void SetStreamPosition(uint64_t position) = 0;
        virtual bool ReadData(char* destination, size_t size) = 0;

        // Convenience methods
        explicit operator bool() const { return IsStreamGood(); }
      
        // Specialized read methods
        void ReadBuffer(Buffer& buffer, uint32_t size = 0);
        void ReadString(std::string& string);
      
        // Template methods
        template<typename T> void ReadRaw(T& type);
        template<typename T> void ReadObject(T& obj);
        template<typename Key, typename Value> void ReadMap(std::map<Key, Value>& map, uint32_t size = 0);
        template<typename Key, typename Value> void ReadMap(std::unordered_map<Key, Value>& map, uint32_t size = 0);
        template<typename Value> void ReadMap(std::unordered_map<std::string, Value>& map, uint32_t size = 0);
        template<typename T> void ReadArray(std::vector<T>& array, uint32_t size = 0);
    };
}
```

## Virtual Interface

### IsStreamGood()

```cpp
[[nodiscard]] virtual bool IsStreamGood() const = 0;
```

**Purpose**: Check if the input stream is in a valid state for reading.

**Returns**: `true` if the stream is ready for read operations, `false` otherwise.

**Implementation Notes**:

- Must be implemented by derived classes
- Should check underlying stream/buffer state and EOF conditions
- Used internally to validate operations before proceeding

**Example Implementation**:

```cpp
// File-based implementation
bool FileReader::IsStreamGood() const 
{
    return m_FileStream.good() && !m_FileStream.eof();
}

// Memory-based implementation  
bool MemoryReader::IsStreamGood() const 
{
    return m_Position < m_Buffer.size();
}
```

### GetStreamPosition()

```cpp
virtual uint64_t GetStreamPosition() = 0;
```

**Purpose**: Get the current read position in the stream.

**Returns**: Current position as byte offset from beginning of stream.

**Implementation Notes**:

- Must be implemented by derived classes
- Used for seeking operations and progress tracking
- Should return position in bytes, not elements

**Example Implementation**:

```cpp
uint64_t FileReader::GetStreamPosition() 
{
    return static_cast<uint64_t>(m_FileStream.tellg());
}

uint64_t MemoryReader::GetStreamPosition() 
{
    return m_Position;
}
```

### SetStreamPosition()

```cpp
virtual void SetStreamPosition(uint64_t position) = 0;
```

**Purpose**: Set the current read position in the stream.

**Parameters**:

- `position`: Byte offset from beginning of stream

**Implementation Notes**:

- Must be implemented by derived classes
- Used for seeking to specific locations
- Should handle bounds checking and EOF conditions

**Example Implementation**:

```cpp
void FileReader::SetStreamPosition(uint64_t position) 
{
    m_FileStream.seekg(position);
    SEDX_CORE_ASSERT(m_FileStream.good(), "Failed to seek to position {}", position);
}

void MemoryReader::SetStreamPosition(uint64_t position) 
{
    SEDX_CORE_ASSERT(position <= m_Buffer.size(), "Position {} exceeds buffer size", position);
    m_Position = position;
}
```

### ReadData()

```cpp
virtual bool ReadData(char* destination, size_t size) = 0;
```

**Purpose**: Read raw binary data from the stream.

**Parameters**:

- `destination`: Pointer to destination buffer
- `size`: Number of bytes to read

**Returns**: `true` if read was successful, `false` otherwise.

**Implementation Notes**:

- Must be implemented by derived classes
- Core method used by all other read operations
- Should handle EOF and error conditions gracefully

**Example Implementation**:

```cpp
bool FileReader::ReadData(char* destination, size_t size) 
{
    m_FileStream.read(destination, size);
    return m_FileStream.good() && m_FileStream.gcount() == static_cast<std::streamsize>(size);
}

bool MemoryReader::ReadData(char* destination, size_t size) 
{
    if (m_Position + size > m_Buffer.size()) {
        return false; // Not enough data
    }
    std::memcpy(destination, m_Buffer.data() + m_Position, size);
    m_Position += size;
    return true;
}
```

## Convenience Methods

### Operator bool()

```cpp
explicit operator bool() const { return IsStreamGood(); }
```

**Purpose**: Allow reader to be used in boolean contexts.

**Returns**: Result of `IsStreamGood()`

**Usage**:

```cpp
FileReader reader("input.bin");
if (reader) {
    // Safe to read
    int value;
    reader.ReadRaw(value);
}
```

## Specialized Read Methods

### ReadBuffer()

```cpp
void ReadBuffer(Buffer& buffer, uint32_t size = 0);
```

**Purpose**: Read a Vulkan buffer from the stream.

**Parameters**:

- `buffer`: Buffer to populate with read data
- `size`: Known buffer size (0 = read size from stream)

**Behavior**:

- If `size` is 0, reads buffer size as `uint64_t` first
- Allocates buffer memory using `buffer.Allocate()`
- Reads buffer data directly from stream

**Usage**:

```cpp
Buffer vertexBuffer;
reader.ReadBuffer(vertexBuffer); // Reads size + data

Buffer indexBuffer;  
reader.ReadBuffer(indexBuffer, knownSize); // Reads only data
```

### ReadString()

```cpp
void ReadString(std::string& string);
```

**Purpose**: Read a string with size prefix.

**Parameters**:

- `string`: String to populate

**Format**:

- Reads string length as `size_t`
- Reads string characters and resizes string accordingly

**Usage**:

```cpp
std::string filename;
reader.ReadString(filename);

// Matches data written by SerializeWriter::WriteString()
```

## Template Methods

### ReadRaw()

```cpp
template<typename T>
void ReadRaw(T& type);
```

**Purpose**: Read a trivial type directly from stream.

**Parameters**:

- `type`: Reference to object to populate

**Constraints**:

- `T` should be trivially copyable
- No pointer types (undefined behavior)
- Platform-dependent for complex types

**Usage**:

```cpp
int32_t count;
float position;
UUID objectId;

reader.ReadRaw(count);
reader.ReadRaw(position);  
reader.ReadRaw(objectId);

// Custom trivial struct
struct Header {
    uint32_t version;
    uint32_t flags;
    uint64_t timestamp;
};

Header header;
reader.ReadRaw(header);
```

### ReadObject()

```cpp
template<typename T>
void ReadObject(T& obj);
```

**Purpose**: Read an object using its deserialization interface.

**Parameters**:

- `obj`: Object to populate

**Requirements**:

- Object must have static `Deserialize(SerializeReader*, T&)` method
- Or use SERIALIZABLE macro
- Or have specialization in `Serialization::Impl` namespace

**Usage**:

```cpp
struct ComplexObject {
    std::string name;
    std::vector<float> data;
  
    static void Deserialize(SerializeReader* reader, ComplexObject& obj) {
        reader->ReadString(obj.name);
        reader->ReadArray(obj.data);
    }
};

ComplexObject obj;
reader.ReadObject(obj);
```

### ReadMap() - std::map

```cpp
template<typename Key, typename Value>
void ReadMap(std::map<Key, Value>& map, uint32_t size = 0);
```

**Purpose**: Read a std::map from the stream.

**Parameters**:

- `map`: Map to populate
- `size`: Known map size (0 = read size from stream)

**Format**:

- Reads map size as `uint32_t` (if size is 0)
- For each key-value pair:
  - Reads key (using ReadRaw for trivial types, ReadObject otherwise)
  - Reads value (using ReadRaw for trivial types, ReadObject otherwise)

**Usage**:

```cpp
std::map<std::string, float> settings;
reader.ReadMap(settings);

// For known-size maps
reader.ReadMap(settings, knownSize);
```

### ReadMap() - std::unordered_map

```cpp
template<typename Key, typename Value>
void ReadMap(std::unordered_map<Key, Value>& map, uint32_t size = 0);
```

**Purpose**: Read an unordered_map from the stream.

**Parameters**:

- `map`: Unordered map to populate
- `size`: Known map size (0 = read size from stream)

**Behavior**: Identical to std::map version but for unordered_map.

**Usage**:

```cpp
std::unordered_map<uint32_t, Transform> objects;
reader.ReadMap(objects);
```

### ReadMap() - String Keys

```cpp
template<typename Value>
void ReadMap(std::unordered_map<std::string, Value>& map, uint32_t size = 0);
```

**Purpose**: Optimized version for string-keyed maps.

**Parameters**:

- `map`: String-keyed unordered map to populate
- `size`: Known map size (0 = read size from stream)

**Optimization**: Uses `ReadString()` for keys instead of generic object deserialization.

**Usage**:

```cpp
std::unordered_map<std::string, MaterialProperties> materials;
reader.ReadMap(materials);
```

### ReadArray()

```cpp
template<typename T>
void ReadArray(std::vector<T>& array, uint32_t size = 0);
```

**Purpose**: Read a vector from the stream.

**Parameters**:

- `array`: Vector to populate
- `size`: Known array size (0 = read size from stream)

**Format**:

- Reads array size as `uint32_t` (if size is 0)
- Resizes vector to accommodate elements
- For each element:
  - Uses ReadRaw for trivial types
  - Uses ReadObject for complex types

**Usage**:

```cpp
std::vector<int> numbers;
std::vector<Transform> transforms;

reader.ReadArray(numbers);
reader.ReadArray(transforms);

// For known-size arrays
reader.ReadArray(numbers, knownSize);
```

### ReadArray() - String Specialization

```cpp
template<>
void ReadArray(std::vector<std::string>& array, uint32_t size = 0);
```

**Purpose**: Optimized specialization for string vectors.

**Parameters**:

- `array`: Vector of strings to populate
- `size`: Known array size (0 = read size from stream)

**Optimization**: Uses `ReadString()` for each element.

**Usage**:

```cpp
std::vector<std::string> filenames;
reader.ReadArray(filenames);
```

## Implementation Examples

### File Reader Implementation

```cpp
class FileReader : public SerializeReader
{
private:
    std::ifstream m_Stream;
    std::string m_Filename;
  
public:
    explicit FileReader(const std::string& filename) 
        : m_Filename(filename)
        , m_Stream(filename, std::ios::binary)
    {
        if (!m_Stream.is_open()) {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Failed to open file for reading: {}", filename);
        }
    }
  
    ~FileReader() override
    {
        if (m_Stream.is_open()) {
            m_Stream.close();
        }
    }
  
    bool IsStreamGood() const override 
    {
        return m_Stream.good() && !m_Stream.eof();
    }
  
    uint64_t GetStreamPosition() override 
    {
        return static_cast<uint64_t>(m_Stream.tellg());
    }
  
    void SetStreamPosition(uint64_t position) override 
    {
        m_Stream.seekg(position);
        if (!m_Stream.good()) {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Failed to seek to position {}", position);
        }
    }
  
    bool ReadData(char* destination, size_t size) override 
    {
        m_Stream.read(destination, size);
        return m_Stream.good() && m_Stream.gcount() == static_cast<std::streamsize>(size);
    }
  
    // Additional methods
    size_t GetFileSize() const
    {
        auto currentPos = m_Stream.tellg();
        m_Stream.seekg(0, std::ios::end);
        auto size = m_Stream.tellg();
        m_Stream.seekg(currentPos);
        return static_cast<size_t>(size);
    }
};
```

### Memory Reader Implementation

```cpp
class MemoryReader : public SerializeReader  
{
private:
    const std::vector<uint8_t>& m_Buffer;
    size_t m_Position = 0;
  
public:
    explicit MemoryReader(const std::vector<uint8_t>& buffer)
        : m_Buffer(buffer)
    {
    }
  
    bool IsStreamGood() const override 
    {
        return m_Position < m_Buffer.size();
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
  
    bool ReadData(char* destination, size_t size) override 
    {
        if (m_Position + size > m_Buffer.size()) {
            return false; // Not enough data remaining
        }
        std::memcpy(destination, m_Buffer.data() + m_Position, size);
        m_Position += size;
        return true;
    }
  
    // Additional methods
    size_t GetRemainingBytes() const 
    {
        return m_Buffer.size() - m_Position;
    }
  
    bool HasMoreData() const 
    {
        return m_Position < m_Buffer.size();
    }
};
```

### Network Reader Implementation

```cpp
class NetworkReader : public SerializeReader
{
private:
    NetworkSocket& m_Socket;
    std::vector<uint8_t> m_Buffer;
    size_t m_Position = 0;
    size_t m_ValidData = 0;
  
public:
    explicit NetworkReader(NetworkSocket& socket, size_t bufferSize = 64 * 1024)
        : m_Socket(socket)
    {
        m_Buffer.resize(bufferSize);
    }
  
    bool IsStreamGood() const override 
    {
        return m_Socket.IsConnected() && (m_Position < m_ValidData || CanReceiveMore());
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
  
    bool ReadData(char* destination, size_t size) override 
    {
        while (m_Position + size > m_ValidData) {
            if (!FillBuffer()) {
                return false; // No more data available
            }
        }
      
        std::memcpy(destination, m_Buffer.data() + m_Position, size);
        m_Position += size;
        return true;
    }
  
private:
    bool FillBuffer()
    {
        // Compact buffer if needed
        if (m_Position > m_Buffer.size() / 2) {
            std::memmove(m_Buffer.data(), m_Buffer.data() + m_Position, m_ValidData - m_Position);
            m_ValidData -= m_Position;
            m_Position = 0;
        }
      
        // Receive more data
        size_t received = m_Socket.Receive(
            m_Buffer.data() + m_ValidData, 
            m_Buffer.size() - m_ValidData
        );
      
        if (received > 0) {
            m_ValidData += received;
            return true;
        }
      
        return false;
    }
  
    bool CanReceiveMore() const
    {
        return m_Socket.HasDataAvailable();
    }
};
```

### Compressed Reader Implementation

```cpp
class CompressedReader : public SerializeReader
{
private:
    std::unique_ptr<SerializeReader> m_Source;
    std::vector<uint8_t> m_DecompressedBuffer;
    size_t m_Position = 0;
    bool m_IsDecompressed = false;
  
public:
    explicit CompressedReader(std::unique_ptr<SerializeReader> source)
        : m_Source(std::move(source))
    {
    }
  
    bool IsStreamGood() const override 
    {
        return m_IsDecompressed ? (m_Position < m_DecompressedBuffer.size()) : m_Source->IsStreamGood();
    }
  
    uint64_t GetStreamPosition() override 
    {
        EnsureDecompressed();
        return m_Position;
    }
  
    void SetStreamPosition(uint64_t position) override 
    {
        EnsureDecompressed();
        SEDX_CORE_ASSERT(position <= m_DecompressedBuffer.size(), "Position exceeds buffer size");
        m_Position = static_cast<size_t>(position);
    }
  
    bool ReadData(char* destination, size_t size) override 
    {
        EnsureDecompressed();
      
        if (m_Position + size > m_DecompressedBuffer.size()) {
            return false;
        }
      
        std::memcpy(destination, m_DecompressedBuffer.data() + m_Position, size);
        m_Position += size;
        return true;
    }
  
private:
    void EnsureDecompressed()
    {
        if (m_IsDecompressed) return;
      
        // Read compressed data header
        uint64_t compressedSize, uncompressedSize;
        m_Source->ReadRaw(compressedSize);
        m_Source->ReadRaw(uncompressedSize);
      
        // Read compressed data
        std::vector<uint8_t> compressedData(compressedSize);
        m_Source->ReadData(reinterpret_cast<char*>(compressedData.data()), compressedSize);
      
        // Decompress
        m_DecompressedBuffer.resize(uncompressedSize);
        bool success = DecompressData(
            compressedData.data(), compressedSize,
            m_DecompressedBuffer.data(), uncompressedSize
        );
      
        SEDX_CORE_ASSERT(success, "Failed to decompress data");
        m_IsDecompressed = true;
    }
};
```

## Error Handling

### Stream Validation

Always validate stream state before reading:

```cpp
template<typename T>
bool SafeRead(SerializeReader* reader, T& value)
{
    if (!reader || !reader->IsStreamGood()) {
        SEDX_CORE_ERROR_TAG("SERIALIZATION", "Invalid reader stream");
        return false;
    }
  
    try {
        reader->ReadRaw(value);
        return reader->IsStreamGood();
    } catch (const std::exception& e) {
        SEDX_CORE_ERROR_TAG("SERIALIZATION", "Exception during read: {}", e.what());
        return false;
    }
}
```

### EOF Handling

Check for end-of-file conditions:

```cpp
bool ReadUntilEOF(SerializeReader* reader, std::vector<Record>& records)
{
    while (reader->IsStreamGood()) {
        Record record;
      
        // Try to read record header first
        uint32_t recordType;
        if (!SafeRead(reader, recordType)) {
            break; // EOF reached
        }
      
        // Read rest of record based on type
        record.type = recordType;
        reader->ReadObject(record);
      
        records.push_back(std::move(record));
    }
  
    return true;
}
```

### Data Validation

Validate read data for consistency:

```cpp
struct ValidatedHeader {
    char magic[4];
    uint32_t version;
    uint64_t dataSize;
  
    static bool Deserialize(SerializeReader* reader, ValidatedHeader& header)
    {
        reader->ReadRaw(header.magic);
        reader->ReadRaw(header.version);
        reader->ReadRaw(header.dataSize);
      
        // Validate magic number
        if (strncmp(header.magic, "SEDX", 4) != 0) {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Invalid file magic number");
            return false;
        }
      
        // Validate version
        if (header.version > CURRENT_VERSION) {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Unsupported file version: {}", header.version);
            return false;
        }
      
        return true;
    }
};
```

## Performance Considerations

### Buffer Management

Pre-allocate containers for known sizes:

```cpp
template<typename T>
void ReadArrayOptimized(SerializeReader* reader, std::vector<T>& array)
{
    uint32_t size;
    reader->ReadRaw(size);
  
    // Reserve capacity to avoid reallocations
    array.clear();
    array.reserve(size);
  
    for (uint32_t i = 0; i < size; ++i) {
        T element;
        reader->ReadObject(element);
        array.push_back(std::move(element));
    }
}
```

### Batch Reads

Read large amounts of data efficiently:

```cpp
template<typename T>
void ReadTrivialArrayFast(SerializeReader* reader, std::vector<T>& array)
{
    static_assert(std::is_trivially_copyable_v<T>);
  
    uint32_t size;
    reader->ReadRaw(size);
  
    array.resize(size);
  
    // Direct memory read for trivial types
    const size_t totalBytes = size * sizeof(T);
    bool success = reader->ReadData(
        reinterpret_cast<char*>(array.data()), 
        totalBytes
    );
  
    SEDX_CORE_ASSERT(success, "Failed to read array data");
}
```

### Memory Mapping for Large Files

```cpp
class MemoryMappedReader : public SerializeReader
{
private:
    void* m_MappedMemory = nullptr;
    size_t m_FileSize = 0;
    size_t m_Position = 0;
  
public:
    explicit MemoryMappedReader(const std::string& filename)
    {
        // Platform-specific memory mapping
        #ifdef _WIN32
            // Windows implementation
        #else
            // POSIX implementation
        #endif
    }
  
    bool ReadData(char* destination, size_t size) override 
    {
        if (m_Position + size > m_FileSize) {
            return false;
        }
      
        // Direct memory copy from mapped region
        std::memcpy(destination, 
                   static_cast<char*>(m_MappedMemory) + m_Position, 
                   size);
        m_Position += size;
        return true;
    }
  
    // ... other methods
};
```

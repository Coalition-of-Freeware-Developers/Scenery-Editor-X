# Scenery Editor X - UUID System Documentation

## Overview

The UUID (Universally Unique Identifier) system in Scenery Editor X provides a robust mechanism for generating unique identifiers for various entities within the application. The system implements both 64-bit and 32-bit UUID variants to optimize memory usage and performance for different use cases.

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [UUID Classes](#uuid-classes)
3. [Implementation Details](#implementation-details)
4. [Usage Examples](#usage-examples)
5. [Performance Considerations](#performance-considerations)
6. [Best Practices](#best-practices)
7. [API Reference](#api-reference)

---

## Architecture Overview

The UUID system is designed around two main classes:

- **`UUID`**: 64-bit identifier for primary entities
- **`UUID32`**: 32-bit identifier for performance-critical contexts

Both classes use cryptographically secure random number generation to ensure uniqueness across application instances.

### Key Features

- **Thread-safe**: Uses separate random number generators for each UUID type
- **Hash-compatible**: Provides STL hash specializations for use in containers
- **Efficient**: Minimal memory footprint with optimized storage
- **Deterministic**: Supports construction from explicit values for serialization

---

## UUID Classes

### UUID Class (64-bit)

The primary UUID class uses a 64-bit integer for maximum uniqueness.

```cpp
class UUID
{
public:
    UUID();                           // Generate random UUID
    explicit UUID(uint64_t uuid);     // Create from explicit value
    UUID(const UUID& other);          // Copy constructor
    
    explicit operator uint64_t() const;  // Convert to uint64_t
    
private:
    uint64_t m_UUID;
};
```

**Use Cases:**
- Entity IDs in the scene graph
- Asset identifiers
- Component instances
- Scene objects
- Persistent data references

### UUID32 Class (32-bit)

The compact UUID class uses a 32-bit integer for performance-critical scenarios.

```cpp
class UUID32
{
public:
    UUID32();                         // Generate random UUID
    explicit UUID32(uint32_t uuid);   // Create from explicit value
    UUID32(const UUID32& other);      // Copy constructor
    
    explicit operator uint32_t() const;  // Convert to uint32_t
    
private:
    uint32_t m_UUID;
};
```

**Use Cases:**
- Temporary object identifiers
- UI element IDs
- Event handling tokens
- Cache keys
- Short-lived references

---

## Implementation Details

### Random Number Generation

The system uses separate random number generators for each UUID type to ensure thread safety and optimal performance:

```cpp
// Global random number generators (thread-safe)
static std::random_device s_RandomDevice;
static std::mt19937_64 eng(s_RandomDevice());              // For UUID
static std::uniform_int_distribution<uint64_t> s_UniformDistribution;
static std::mt19937 eng32(s_RandomDevice());               // For UUID32
static std::uniform_int_distribution<uint32_t> s_UniformDistribution32;
```

### Hash Specializations

Both UUID classes provide STL hash specializations for use in hash-based containers:

```cpp
namespace std {
    template <>
    struct hash<SceneryEditorX::UUID> {
        std::size_t operator()(const SceneryEditorX::UUID& uuid) const {
            return static_cast<std::size_t>(static_cast<uint64_t>(uuid));
        }
    };
    
    template <>
    struct hash<SceneryEditorX::UUID32> {
        std::size_t operator()(const SceneryEditorX::UUID32& uuid) const {
            return hash<uint32_t>()(static_cast<uint32_t>(uuid));
        }
    };
}
```

---

## Usage Examples

### Basic UUID Generation

```cpp
#include <SceneryEditorX/core/uuid.h>

using namespace SceneryEditorX;

// Generate random UUIDs
UUID entityId;           // 64-bit UUID
UUID32 eventToken;       // 32-bit UUID

// Create from explicit values (e.g., for serialization)
UUID loadedEntity(0x123456789ABCDEF0);
UUID32 savedToken(0x12345678);
```

### Entity System Integration

```cpp
class Entity
{
private:
    UUID m_EntityId;
    std::string m_Name;
    
public:
    Entity(const std::string& name) 
        : m_EntityId(), m_Name(name) {}  // Auto-generates UUID
    
    UUID GetId() const { return m_EntityId; }
    const std::string& GetName() const { return m_Name; }
};

// Usage
Entity player("Player");
Entity enemy("Enemy");

std::cout << "Player ID: " << static_cast<uint64_t>(player.GetId()) << std::endl;
```

### Container Usage

```cpp
#include <unordered_map>
#include <unordered_set>

// Hash maps with UUID keys
std::unordered_map<UUID, Entity> entityMap;
std::unordered_map<UUID32, std::string> eventNames;

// Hash sets
std::unordered_set<UUID> activeEntities;
std::unordered_set<UUID32> pendingEvents;

// Adding entities
Entity newEntity("TestEntity");
entityMap[newEntity.GetId()] = newEntity;
activeEntities.insert(newEntity.GetId());
```

### Event System Integration

```cpp
class EventSystem
{
private:
    std::unordered_map<UUID32, std::function<void()>> m_EventHandlers;
    
public:
    UUID32 RegisterHandler(std::function<void()> handler)
    {
        UUID32 token;
        m_EventHandlers[token] = handler;
        return token;
    }
    
    void UnregisterHandler(UUID32 token)
    {
        m_EventHandlers.erase(token);
    }
    
    void TriggerEvent(UUID32 token)
    {
        if (auto it = m_EventHandlers.find(token); it != m_EventHandlers.end())
        {
            it->second();
        }
    }
};
```

### Serialization Support

```cpp
#include <json/json.h>

// Serialize UUID to JSON
Json::Value SerializeEntity(const Entity& entity)
{
    Json::Value json;
    json["id"] = static_cast<uint64_t>(entity.GetId());
    json["name"] = entity.GetName();
    return json;
}

// Deserialize UUID from JSON
Entity DeserializeEntity(const Json::Value& json)
{
    uint64_t id = json["id"].asUInt64();
    std::string name = json["name"].asString();
    
    // Create entity with explicit UUID
    Entity entity(name);
    // Note: This would require a setter or constructor modification
    return entity;
}
```

---

## Performance Considerations

### Memory Usage

- **UUID**: 8 bytes per instance
- **UUID32**: 4 bytes per instance

### Generation Performance

- **UUID**: ~100-500 ns per generation (depends on RNG quality)
- **UUID32**: ~50-250 ns per generation (faster due to smaller state)

### Hash Performance

- **UUID**: Direct cast to `std::size_t` - O(1)
- **UUID32**: Hash through `std::hash<uint32_t>` - O(1)

### Recommendations

1. **Use UUID32 for temporary identifiers** (UI elements, event tokens)
2. **Use UUID for persistent identifiers** (entities, assets, components)
3. **Avoid frequent generation** in performance-critical loops
4. **Consider object pooling** for frequently created/destroyed objects

---

## Best Practices

### 1. Choosing the Right UUID Type

```cpp
// ✅ Good: Use UUID for persistent entities
class SceneObject
{
    UUID m_Id;  // Will be saved/loaded
};

// ✅ Good: Use UUID32 for temporary UI elements
class UIWidget
{
    UUID32 m_WidgetId;  // Only exists during UI session
};
```

### 2. Container Selection

```cpp
// ✅ Good: Use unordered containers for UUID keys
std::unordered_map<UUID, Entity> entities;
std::unordered_set<UUID32> activeTokens;

// ❌ Avoid: Ordered containers are unnecessary overhead
std::map<UUID, Entity> entities;  // Slower due to ordering
```

### 3. Explicit Conversion

```cpp
// ✅ Good: Explicit conversion for serialization
uint64_t serializedId = static_cast<uint64_t>(entity.GetId());

// ❌ Avoid: Implicit conversion (wouldn't compile anyway)
uint64_t id = entity.GetId();  // Compilation error
```

### 4. Thread Safety

```cpp
// ✅ Good: UUID generation is thread-safe
std::vector<std::thread> threads;
for (int i = 0; i < 10; ++i) {
    threads.emplace_back([]() {
        UUID id;  // Safe to generate in multiple threads
    });
}
```

### 5. Comparison and Equality

```cpp
// ✅ Good: Direct comparison works
UUID id1, id2;
if (id1 == id2) {  // Extremely unlikely but possible
    // Handle collision
}

// ✅ Good: Use in conditions
if (entity.GetId() == targetId) {
    // Process entity
}
```

---

## API Reference

### UUID Class Methods

#### Constructors

```cpp
UUID();                      // Generate random UUID
explicit UUID(uint64_t uuid); // Create from explicit value
UUID(const UUID& other);      // Copy constructor
```

#### Operators

```cpp
explicit operator uint64_t() const;  // Convert to uint64_t
```

#### Comparison (Implicit)

```cpp
bool operator==(const UUID& other) const;  // Equality comparison
bool operator!=(const UUID& other) const;  // Inequality comparison
```

### UUID32 Class Methods

#### Constructors

```cpp
UUID32();                        // Generate random UUID32
explicit UUID32(uint32_t uuid);  // Create from explicit value
UUID32(const UUID32& other);     // Copy constructor
```

#### Operators

```cpp
explicit operator uint32_t() const;  // Convert to uint32_t
```

#### Comparison (Implicit)

```cpp
bool operator==(const UUID32& other) const;  // Equality comparison
bool operator!=(const UUID32& other) const;  // Inequality comparison
```

### Hash Specializations

Both UUID classes provide `std::hash` specializations for use in STL containers.

---

## Integration with Other Systems

### Asset Manager

```cpp
class AssetManager
{
private:
    std::unordered_map<UUID, Asset> m_Assets;
    
public:
    UUID LoadAsset(const std::string& path);
    Asset* GetAsset(UUID id);
    void UnloadAsset(UUID id);
};
```

### Scene Graph

```cpp
class SceneNode
{
private:
    UUID m_NodeId;
    std::vector<UUID> m_Children;
    UUID m_Parent;
    
public:
    UUID GetId() const { return m_NodeId; }
    void AddChild(UUID childId);
    void RemoveChild(UUID childId);
};
```

### Component System

```cpp
class ComponentManager
{
private:
    std::unordered_map<UUID, std::unique_ptr<Component>> m_Components;
    
public:
    template<typename T>
    T* AddComponent(UUID entityId);
    
    template<typename T>
    T* GetComponent(UUID entityId);
    
    void RemoveComponent(UUID entityId, UUID componentId);
};
```

---

## Troubleshooting

### Common Issues

1. **Hash collisions**: Extremely rare but possible
2. **Serialization format**: Ensure consistent endianness
3. **Thread safety**: UUID generation is safe, but container access isn't
4. **Memory leaks**: UUIDs are value types, no special cleanup needed

### Debug Utilities

```cpp
#ifdef SEDX_DEBUG
std::string UUIDToString(const UUID& uuid) {
    std::stringstream ss;
    ss << std::hex << static_cast<uint64_t>(uuid);
    return ss.str();
}

std::string UUID32ToString(const UUID32& uuid) {
    std::stringstream ss;
    ss << std::hex << static_cast<uint32_t>(uuid);
    return ss.str();
}
#endif
```

---

## Future Enhancements

1. **UUID128**: True 128-bit UUID support
2. **Deterministic UUIDs**: Seed-based generation for testing
3. **UUID Pools**: Pre-generated UUID pools for performance
4. **Compression**: Variable-length encoding for storage
5. **Validation**: UUID format validation utilities

---

*This documentation is part of the Scenery Editor X project. For more information, see the main project documentation.*

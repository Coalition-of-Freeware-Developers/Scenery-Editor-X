# Scenery Editor X - Type Descriptors System Documentation

---

# Overview

The Type Descriptors system (`type_descriptors.h`) provides a powerful compile-time reflection mechanism for C++ classes in Scenery Editor X. This system enables introspection, serialization, debug printing, and runtime manipulation of class members through a clean, macro-based interface.

The system is the cornerstone of the reflection framework, providing the foundational `DESCRIBED` macro that makes C++ types introspectable at both compile-time and runtime. It integrates seamlessly with the type utilities, names, and structs systems to provide a complete reflection solution.

## Core Architecture

### Description Template

The `Description<T, TTag>` template is the foundation of the reflection system:

```cpp
template<typename T, typename TTag = TDummyTag>
struct Description;
```

**Key Properties:**

- **T**: The type being described
- **TTag**: Optional tag for multiple descriptions of the same type
- **Compile-time**: All operations are performed at compile-time when possible
- **Zero-overhead**: No runtime cost when not used

### MemberList Template

`MemberList<auto... MemberPointers>` is a utility wrapper that operates on variadic lists of member pointers:

```cpp
template<auto... MemberPointers>
struct MemberList
{
    using TTuple = decltype(std::tuple(MemberPointers...));
    using TVariant = std::variant<filter_void_t<TMemberPtrType<decltype(MemberPointers)>>...>;
  
    static constexpr size_t Count();
  
    // Function application methods
    template<typename TObj, typename TFunc>
    static void ApplyToMembers(TObj& obj, TFunc&& func);
  
    template<typename TObj, typename TFunc>  
    static void ForEachMember(TObj& obj, TFunc&& func);
  
    template<typename TObj, typename TFunc>
    static void ForEachDataMember(TObj& obj, TFunc&& func);
};
```

### Description Interface

`DescriptionInterface` provides convenient methods for accessing members by name and implements common reflection operations:

```cpp
template<class TDescription, class TObjType, class TTag, class TList>
struct DescriptionInterface
{
    static constexpr size_t NumberOfMembers = TList::Count();
    static constexpr size_t INVALID_INDEX = static_cast<size_t>(-1);
  
    // Member access by name
    static std::optional<size_t> GetMemberIndexByName(std::string_view name);
    static std::optional<std::string_view> GetTypeNameByName(std::string_view name);
    static std::optional<size_t> GetMemberSizeByName(std::string_view name);
    static std::optional<bool> IsFunctionByName(std::string_view name);
};
```

## The DESCRIBED Macro System

### Basic Usage

The `DESCRIBED` macro is the primary interface for making types reflectable:

```cpp
struct Transform
{
    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 rotation{0.0f, 0.0f, 0.0f}; 
    Vec3 scale{1.0f, 1.0f, 1.0f};
    bool visible = true;
  
    void Reset() { *this = Transform{}; }
    bool IsIdentity() const { return position == Vec3{} && rotation == Vec3{} && scale == Vec3{1,1,1}; }
};

// Make the type reflectable
DESCRIBED(Transform,
    &Transform::position,
    &Transform::rotation,
    &Transform::scale,
    &Transform::visible,
    &Transform::Reset,
    &Transform::IsIdentity
);
```

### Tagged Descriptions

For multiple descriptions of the same type:

```cpp
struct GameEntity {};

// Default description for serialization
DESCRIBED(GameEntity,
    &GameEntity::id,
    &GameEntity::name,
    &GameEntity::transform
);

// Network-specific description (only networked properties)
struct NetworkTag {};
DESCRIBED_TAGGED(GameEntity, NetworkTag,
    &GameEntity::id,
    &GameEntity::transform
);
```

### Macro Expansion Details

The `DESCRIBED` macro generates:

```cpp
template<>
struct SceneryEditorX::Types::Description<Transform, SceneryEditorX::Types::TDummyTag> 
    : SceneryEditorX::Types::MemberList<&Transform::position, &Transform::rotation, /*...*/>,
      SceneryEditorX::Types::DescriptionInterface</*...*/> 
{
    static constexpr std::string_view Namespace = ""; // Or detected namespace
    static constexpr std::string_view ClassName = "Transform";
    static constexpr std::array<std::string_view, 6> MemberNames = {"position", "rotation", /*...*/};
};
```

## Advanced Features

### Member Access and Manipulation

#### Getting Member Values by Index

```cpp
// Compile-time access (zero overhead)
template<size_t Index>
static auto GetMemberValue(const Object& obj)
{
    constexpr auto memberPtr = std::get<Index>(TTuple{});
    if constexpr (std::is_member_object_pointer_v<decltype(memberPtr)>) {
        return obj.*memberPtr;
    } else {
        // Function pointer - return callable
        return [&obj](auto&&... args) { return (obj.*memberPtr)(std::forward<decltype(args)>(args)...); };
    }
}

// Runtime access with type conversion
template<typename T>
static std::optional<T> GetMemberValueOfType(size_t index, const Object& obj)
{
    std::optional<T> result;
    ApplyToMemberAtIndex(index, obj, [&result](const auto& value) {
        if constexpr (std::is_convertible_v<std::decay_t<decltype(value)>, T>) {
            result = static_cast<T>(value);
        }
    });
    return result;
}
```

#### Setting Member Values

```cpp
// By index with type checking
template<typename T>
static bool SetMemberValue(size_t index, Object& obj, const T& value)
{
    bool success = false;
    ApplyToMemberAtIndex(index, obj, [&](auto& member) {
        using MemberType = std::decay_t<decltype(member)>;
        if constexpr (std::is_assignable_v<MemberType&, T>) {
            member = value;
            success = true;
        }
    });
    return success;
}

// By name with automatic type conversion
template<typename T>
static bool SetMemberValueByName(std::string_view name, Object& obj, const T& value)
{
    auto index = GetMemberIndexByName(name);
    return index ? SetMemberValue(*index, obj, value) : false;
}
```

#### Variant-Based Access

```cpp
using MemberVariant = std::variant<int, float, Vec3, bool, std::string>;

static std::optional<MemberVariant> GetMemberValueVariantByName(std::string_view name, const Object& obj)
{
    auto index = GetMemberIndexByName(name);
    if (!index) return std::nullopt;
  
    std::optional<MemberVariant> result;
    ApplyToMemberAtIndex(*index, obj, [&result](const auto& value) {
        using ValueType = std::decay_t<decltype(value)>;
        if constexpr (std::variant_alternative_v<ValueType, MemberVariant>) {
            result = MemberVariant{value};
        }
    });
    return result;
}
```

### Function Member Handling

The system handles both data members and member functions:

```cpp
struct AudioSource 
{
    float volume = 1.0f;
    bool playing = false;
  
    void Play() { playing = true; }
    void Stop() { playing = false; }
    void SetVolume(float v) { volume = std::clamp(v, 0.0f, 1.0f); }
    float GetVolume() const { return volume; }
};

DESCRIBED(AudioSource,
    &AudioSource::volume,
    &AudioSource::playing,
    &AudioSource::Play,
    &AudioSource::Stop,
    &AudioSource::SetVolume,
    &AudioSource::GetVolume
);

// Usage
AudioSource source;

// Call member functions through reflection
Description<AudioSource>::CallMemberFunction("Play", source);
Description<AudioSource>::CallMemberFunction("SetVolume", source, 0.8f);

// Check if member is a function
bool isFunction = *Description<AudioSource>::IsFunctionByName("Play"); // true
bool isData = *Description<AudioSource>::IsFunctionByName("volume");   // false
```

### Debug and Serialization Support

#### Streaming Support

```cpp
template<typename T, typename... Ts>
std::ostream& operator<<(std::ostream& os, const std::variant<T, Ts...>& v)
{
    std::visit([&os](const auto& value) {
        if constexpr (is_streamable_v<std::decay_t<decltype(value)>>) {
            os << value;
        } else {
            os << "[non-streamable " << type::type_name<std::decay_t<decltype(value)>>() << "]";
        }
    }, v);
    return os;
}
```

#### Debug Printing

```cpp
template<typename T>
void PrintTypeInfo()
{
    using Desc = Description<T>;
    std::cout << "Type: " << Desc::ClassName << "\n";
    std::cout << "Size: " << sizeof(T) << " bytes\n";
    std::cout << "Members (" << Desc::NumberOfMembers << "):\n";
  
    for (size_t i = 0; i < Desc::NumberOfMembers; ++i) {
        auto typeName = Desc::GetTypeNameByIndex(i);
        auto memberName = Desc::MemberNames[i];
        auto size = Desc::GetMemberSizeByIndex(i);
        auto isFunc = Desc::IsFunctionByIndex(i);
  
        std::cout << "  " << (isFunc ? "function" : "data") << " " 
                  << typeName << " " << memberName;
        if (!isFunc) std::cout << " [" << size << " bytes]";
        std::cout << "\n";
    }
}
```

## Integration with Other Systems

### Type Names Integration

```cpp
// Automatic type name extraction for members
template<size_t Index>
static constexpr std::string_view GetMemberTypeName()
{
    using MemberType = typename member_pointer::return_type<
        std::remove_cvref_t<decltype(std::get<Index>(TTuple()))>>::type;
    return type::type_name<MemberType>();
}
```

### Type Structs Integration

```cpp
// Generate runtime ClassInfo from compile-time Description
template<class T>
static ClassInfo GetClassInfo()
{
    static_assert(Described<T>::value, "Type must be described");
    return ClassInfo::Of<T>();
}
```

## Best Practices

### 1. Member Ordering

```cpp
// ✅ Good: Group related members, data before functions
DESCRIBED(GameObject,
    // Core data
    &GameObject::id,
    &GameObject::name,
    &GameObject::transform,
  
    // State data
    &GameObject::active,
    &GameObject::visible,
  
    // Behavior functions
    &GameObject::Update,
    &GameObject::Render,
    &GameObject::GetBounds
);
```

### 2. Namespace Handling

```cpp
namespace SceneryEditorX::Renderer {
    struct Shader {
        std::string name;
        ShaderType type;
        void Compile();
    };
}

// The macro will automatically detect the namespace
DESCRIBED(SceneryEditorX::Renderer::Shader,
    &SceneryEditorX::Renderer::Shader::name,
    &SceneryEditorX::Renderer::Shader::type,
    &SceneryEditorX::Renderer::Shader::Compile
);
```

### 3. Performance Considerations

```cpp
// ✅ Prefer compile-time access when index is known
template<typename T>
void ProcessTransform(T& transform)
{
    // Fast compile-time access
    auto& position = Description<T>::template GetMemberValue<0>(transform);
    auto& rotation = Description<T>::template GetMemberValue<1>(transform);
  
    // Process position and rotation...
}

// ⚠️ Runtime access when needed for dynamic scenarios
void ProcessDynamicMember(Object& obj, std::string_view memberName)
{
    auto value = Description<Object>::GetMemberValueByName<float>(memberName, obj);
    if (value) {
        // Process value...
    }
}
```

### 4. Error Handling

```cpp
// Always check optional returns for runtime access
auto memberIndex = Description<MyClass>::GetMemberIndexByName("nonexistent");
if (memberIndex) {
    // Safe to use *memberIndex
} else {
    // Handle missing member
    LOG_WARNING("Member 'nonexistent' not found in MyClass");
}
```

## Common Use Cases

### 1. Property Editors

```cpp
template<typename T>
void DrawPropertyEditor(T& object)
{
    using Desc = Description<T>;
  
    for (size_t i = 0; i < Desc::NumberOfMembers; ++i) {
        if (*Desc::IsFunctionByIndex(i)) continue; // Skip functions
  
        std::string memberName{Desc::MemberNames[i]};
        auto typeName = *Desc::GetTypeNameByIndex(i);
  
        if (typeName == "float") {
            auto value = Desc::template GetMemberValueOfType<float>(i, object);
            if (value && ImGui::SliderFloat(memberName.c_str(), &*value, 0.0f, 1.0f)) {
                Desc::SetMemberValue(i, object, *value);
            }
        }
        // Handle other types...
    }
}
```

### 2. Serialization Framework

```cpp
template<typename T>
nlohmann::json SerializeToJson(const T& object)
{
    static_assert(Described<T>::value, "Type must be described");
  
    nlohmann::json result;
    using Desc = Description<T>;
  
    Desc::ForEachDataMember(object, [&result](const auto& member, std::string_view name) {
        if constexpr (is_streamable_v<std::decay_t<decltype(member)>>) {
            result[std::string{name}] = member;
        }
    });
  
    return result;
}
```

### 3. Debug Inspection

```cpp
template<typename T>
void InspectObject(const T& object, const std::string& objectName)
{
    using Desc = Description<T>;
  
    std::cout << "=== " << objectName << " (" << Desc::ClassName << ") ===\n";
  
    for (size_t i = 0; i < Desc::NumberOfMembers; ++i) {
        if (*Desc::IsFunctionByIndex(i)) continue;
  
        auto name = Desc::MemberNames[i];
        auto variant = Desc::GetMemberValueVariantByIndex(i, object);
  
        std::cout << name << ": ";
        if (variant) {
            std::cout << *variant;
        } else {
            std::cout << "[cannot display]";
        }
        std::cout << "\n";
    }
}
```

## Limitations and Considerations

### 1. Compilation Requirements

- Requires C++17 minimum for `std::string_view` and `if constexpr`
- Works with C++20 for enhanced features
- Member pointers must be compile-time constants

### 2. Performance Implications

- Compile-time access: Zero overhead
- Runtime access by name: Hash map lookup overhead
- Member function calls through reflection: Slight indirection cost

### 3. Type Limitations

```cpp
// ✅ Supported member types
struct Supported {
    int value;
    float* pointer;
    std::string text;
    std::vector<int> container;
    void Function();
    bool Method() const;
};

// ❌ Not directly supported (requires wrapper)
struct NotSupported {
    static int staticMember;        // Static members not supported
    int bitfield : 4;              // Bitfields not supported
    volatile int volatileMember;    // Volatile qualification issues
};
```

### 4. Template Instantiation

The `DESCRIBED` macro must be placed at global scope and the type must be complete at the point of description.

## Troubleshooting

### Common Errors

1. **"Type must be 'Described'"**

   ```cpp
   // Error: Missing DESCRIBED macro
   ClassInfo info = ClassInfo::Of<MyClass>(); // ❌

   // Fix: Add description
   DESCRIBED(MyClass, &MyClass::member1, &MyClass::member2);
   ClassInfo info = ClassInfo::Of<MyClass>(); // ✅
   ```
2. **"No member named..."**

   ```cpp
   // Error: Member name mismatch
   auto value = Description<MyClass>::GetMemberValueByName<int>("membe1", obj); // ❌ typo

   // Fix: Use correct name or check member names
   auto value = Description<MyClass>::GetMemberValueByName<int>("member1", obj); // ✅
   ```
3. **Template compilation errors**

   ```cpp
   // Error: Incomplete type in DESCRIBED
   class Forward; // Forward declaration
   DESCRIBED(Forward, &Forward::member); // ❌ Incomplete type

   // Fix: Define type first
   class Forward { int member; };
   DESCRIBED(Forward, &Forward::member); // ✅
   ```

This comprehensive system provides the foundation for all reflection operations in Scenery Editor X, enabling powerful debugging, serialization, and runtime introspection capabilities while maintaining high performance through compile-time optimization.

```c++
// By compile-time index
bool success = Description::SetMemberValue<0>(newValue, myObject);

// By runtime index
bool success = Description::SetMemberValue(0, newValue, myObject);

// By member name
bool success = Description::SetMemberValueByName("memberName", newValue, myObject);
```

### 2. Type Information Queries

```cpp
// Check if member is a function
bool isFunc = Description<MyClass>::IsFunction<0>();
auto optionalIsFunc = Description<MyClass>::IsFunctionByName("memberName");

// Get member size
size_t size = Description<MyClass>::GetMemberSize<0>();
auto optionalSize = Description<MyClass>::GetMemberSizeByName("memberName");

// Get type name
auto typeName = Description<MyClass>::GetTypeName<0>();
auto optionalTypeName = Description<MyClass>::GetTypeNameByName("memberName");
```

### 3. Functional Programming Support

#### Apply Functions to All Members

```cpp
// Apply function to variadic pack of all members
Description<MyClass>::Apply([](auto&... members) {
    // Process all members as parameters
    ((std::cout << members << " "), ...);
}, myObject);

// Apply function to each member individually
Description<MyClass>::ApplyForEach([](auto& member) {
    // Process each member separately
    std::cout << member << std::endl;
}, myObject);
```

#### Apply to Static Types (No Object Instance Required)

```cpp
Description<MyClass>::ApplyToStaticType([](auto... memberPointers) {
    // Work with member pointer types at compile time
    ((std::cout << typeid(decltype(memberPointers)).name() << " "), ...);
});
```

### 4. Debug and Introspection

```cpp
// Print class structure
Description<MyClass>::Print(std::cout);

// Print class structure with values
Description<MyClass>::Print(std::cout, myObject);
```

## Usage Patterns

### Basic Class Description

```cpp
class Transform
{
public:
    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 rotation{0.0f, 0.0f, 0.0f};
    Vec3 scale{1.0f, 1.0f, 1.0f};
  
    void Reset() { position = rotation = {0.0f, 0.0f, 0.0f}; scale = {1.0f, 1.0f, 1.0f}; }
    bool IsIdentity() const { return position == Vec3{} && rotation == Vec3{} && scale == Vec3{1.0f}; }
  
private:
    bool m_IsDirty = false;
};

// Enable reflection
DESCRIBED(Transform,
    &Transform::position,
    &Transform::rotation, 
    &Transform::scale,
    &Transform::Reset,
    &Transform::IsIdentity,
    &Transform::m_IsDirty
);
```

### Tagged Descriptions

Use tagged descriptions for different contexts or serialization formats:

```cpp
struct EditorTag {};
struct SerializationTag {};

// Different member sets for different contexts
DESCRIBED_TAGGED(Transform, EditorTag,
    &Transform::position,
    &Transform::rotation,
    &Transform::scale
);

DESCRIBED_TAGGED(Transform, SerializationTag,
    &Transform::position,
    &Transform::rotation,
    &Transform::scale,
    &Transform::m_IsDirty
);
```

### Generic Processing Functions

```cpp
template<typename T>
void SerializeObject(const T& obj, StreamWriter* writer)
{
    static_assert(Described<T>::value, "Type must be described for serialization");
  
    using Desc = Description<T>;
  
    // Write number of members
    writer->Write(Desc::Count());
  
    // Serialize each data member (skip functions)
    Desc::ApplyForEach([&](const auto& member) {
        if constexpr (!std::is_member_function_pointer_v<decltype(&member - &obj)>) {
            writer->Write(member);
        }
    }, obj);
}

template<typename T>
void PrintObjectDebugInfo(const T& obj)
{
    static_assert(Described<T>::value, "Type must be described for debug printing");
  
    using Desc = Description<T>;
    std::cout << "=== " << Desc::ClassName << " Debug Info ===" << std::endl;
  
    for (size_t i = 0; i < Desc::Count(); ++i) {
        auto memberName = Desc::GetMemberName(i);
        auto typeName = Desc::GetTypeName(i);
        auto isFunction = Desc::IsFunction(i);
  
        std::cout << *typeName << " " << *memberName;
  
        if (*isFunction) {
            std::cout << " (function)";
        } else {
            std::cout << " = ";
            // Print value using variant system
            auto variant = Desc::GetMemberValue<typename Desc::TVariant>(i, obj);
            std::cout << variant;
        }
        std::cout << std::endl;
    }
}
```

### Editor Integration

```cpp
class PropertyEditor
{
public:
    template<typename T>
    void EditObject(T& obj)
    {
        static_assert(Described<T>::value, "Type must be described for editing");
  
        using Desc = Description<T>;
  
        ImGui::Text("Editing: %s", std::string(Desc::ClassName).c_str());
  
        for (size_t i = 0; i < Desc::Count(); ++i) {
            auto memberName = *Desc::GetMemberName(i);
            auto isFunction = *Desc::IsFunction(i);
      
            if (!isFunction) {
                ImGui::PushID(static_cast<int>(i));
                EditMemberByIndex<T>(i, obj, memberName);
                ImGui::PopID();
            }
        }
    }
  
private:
    template<typename T>
    void EditMemberByIndex(size_t index, T& obj, std::string_view memberName)
    {
        using Desc = Description<T>;
  
        // Try common types
        if (auto value = Desc::template GetMemberValueOfType<float>(index, obj)) {
            float f = *value;
            if (ImGui::DragFloat(std::string(memberName).c_str(), &f)) {
                Desc::SetMemberValue(index, f, obj);
            }
        }
        else if (auto value = Desc::template GetMemberValueOfType<int>(index, obj)) {
            int i = *value;
            if (ImGui::DragInt(std::string(memberName).c_str(), &i)) {
                Desc::SetMemberValue(index, i, obj);
            }
        }
        else if (auto value = Desc::template GetMemberValueOfType<bool>(index, obj)) {
            bool b = *value;
            if (ImGui::Checkbox(std::string(memberName).c_str(), &b)) {
                Desc::SetMemberValue(index, b, obj);
            }
        }
        // Add more types as needed...
    }
};
```

## Advanced Features

### Member Filtering and Conditional Access

```cpp
template<typename T, typename Predicate>
void ProcessMembersIf(T& obj, Predicate pred)
{
    using Desc = Description<T>;
  
    for (size_t i = 0; i < Desc::Count(); ++i) {
        if (pred(i, *Desc::GetMemberName(i), *Desc::GetTypeName(i))) {
            // Process member at index i
            auto variant = Desc::GetMemberValue<typename Desc::TVariant>(i, obj);
            // Do something with variant...
        }
    }
}

// Usage: Process only float members
ProcessMembersIf(transform, [](size_t index, std::string_view name, std::string_view type) {
    return type == "float";
});
```

### Compile-Time Member Validation

```cpp
template<typename T>
constexpr bool ValidateRequiredMembers()
{
    using Desc = Description<T>;
  
    // Check that required members exist
    constexpr bool hasPosition = Desc::HasMember("position");
    constexpr bool hasRotation = Desc::HasMember("rotation");
    constexpr bool hasScale = Desc::HasMember("scale");
  
    return hasPosition && hasRotation && hasScale;
}

// Compile-time assertion
static_assert(ValidateRequiredMembers<Transform>(), "Transform must have position, rotation, and scale members");
```

## Best Practices

### 1. Naming Conventions

- Use descriptive member names that will be useful in debug output
- Consider how member names will appear in serialization and editor UIs
- Use consistent naming patterns across your codebase

### 2. Member Selection

- Include all data members that need to be introspected
- Include relevant accessor/mutator functions
- Consider creating different tagged descriptions for different use cases

### 3. Performance Considerations

- Reflection operations are compile-time when possible
- Runtime member access by name has O(n) complexity
- Use compile-time indices when performance is critical
- Cache member indices for frequently accessed members

### 4. Error Handling

- Always check return values from optional-returning functions
- Use `static_assert` with `Described<T>::value` to ensure types are properly described
- Validate member existence before attempting access by name

### 5. Memory and Type Safety

- The system uses perfect forwarding to preserve value categories
- Member pointers are validated at compile-time
- Type conversions are explicit and checked via SFINAE

## Integration with Other Systems

### Serialization Integration

```cpp
template<typename T>
void SerializeDescribed(StreamWriter* writer, const T& obj)
{
    static_assert(Described<T>::value);
  
    using Desc = Description<T>;
  
    // Write class name for versioning/debugging
    writer->Write(std::string(Desc::ClassName));
  
    // Write member count
    writer->Write(static_cast<uint32_t>(Desc::Count()));
  
    // Write each non-function member
    for (size_t i = 0; i < Desc::Count(); ++i) {
        if (!*Desc::IsFunction(i)) {
            writer->Write(std::string(*Desc::GetMemberName(i)));
      
            // Use variant system for type-erased serialization
            auto variant = Desc::GetMemberValue<typename Desc::TVariant>(i, obj);
            SerializeVariant(writer, variant);
        }
    }
}
```

### Module System Integration

```cpp
class ReflectionModule : public Module
{
public:
    template<typename T>
    void RegisterType()
    {
        static_assert(Described<T>::value);
  
        using Desc = Description<T>;
  
        TypeInfo info;
        info.name = Desc::ClassName;
        info.size = sizeof(T);
        info.memberCount = Desc::Count();
  
        // Store reflection information for runtime access
        m_TypeRegistry[std::string(Desc::ClassName)] = std::move(info);
  
        SEDX_CORE_INFO_TAG("REFLECTION", "Registered type: {}", Desc::ClassName);
    }
  
private:
    std::unordered_map<std::string, TypeInfo> m_TypeRegistry;
};
```

## Common Patterns and Examples

### Configuration System

```cpp
struct AppConfig
{
    bool enableVSync = true;
    int maxFPS = 60;
    float volume = 1.0f;
    std::string graphicsAPI = "Vulkan";
  
    void Reset() {
        enableVSync = true;
        maxFPS = 60;
        volume = 1.0f;
        graphicsAPI = "Vulkan";
    }
};

DESCRIBED(AppConfig,
    &AppConfig::enableVSync,
    &AppConfig::maxFPS,
    &AppConfig::volume,
    &AppConfig::graphicsAPI,
    &AppConfig::Reset
);

// Automatic configuration loading/saving
template<typename ConfigType>
void LoadConfigFromFile(const std::string& filename, ConfigType& config)
{
    using Desc = Description<ConfigType>;
  
    // Use libconfig or JSON to load values by member name
    // This allows configuration files to be robust against member reordering
}
```

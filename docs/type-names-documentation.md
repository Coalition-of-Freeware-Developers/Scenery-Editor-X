# Type Names System Documentation

## Overview

The Type Names system (`type_names.h`) provides compile-time type name extraction for C++ types in Scenery Editor X. This system enables retrieving human-readable type names at compile time without RTTI, supporting multiple compilers and offering options for namespace inclusion/exclusion.

This system is fundamental to the reflection framework, providing the type name information used by the Description system, ClassInfo generation, and debugging utilities throughout the codebase.

## Core Architecture

### Compile-Time Type Name Extraction

The system leverages compiler-specific function signature introspection to extract type names at compile time using template metaprogramming and compile-time string manipulation:

- **Clang**: Uses `__PRETTY_FUNCTION__` with pattern `"T = TypeName"`
- **GCC**: Uses `__PRETTY_FUNCTION__` with pattern `"; T = TypeName"`  
- **MSVC**: Uses `__FUNCSIG__` with pattern `"e,TypeName>(void)"`

### Template Metaprogramming Foundation

The implementation uses advanced template metaprogramming techniques:
- `std::string_view` for zero-overhead compile-time string operations
- `std::index_sequence` for compile-time array generation
- Template specialization for compiler-specific handling
- Constexpr functions for compile-time evaluation

## API Reference

### Primary Functions

#### `type_name<T>()`
```cpp
template <typename T>
constexpr std::string_view type_name()
```

Returns the type name without namespace qualifiers. This is the most commonly used function for type identification.

**Examples:**
```cpp
// Basic types
static_assert(type_name<int>() == "int");
static_assert(type_name<float>() == "float");
static_assert(type_name<bool>() == "bool");
static_assert(type_name<char>() == "char");
static_assert(type_name<double>() == "double");

// Pointer and reference types
static_assert(type_name<int*>() == "int*");
static_assert(type_name<const float*>() == "const float*");
static_assert(type_name<const float&>() == "const float&");
static_assert(type_name<int**>() == "int**");

// Standard library types (namespace stripped)
static_assert(type_name<std::vector<int>>() == "vector<int>");
static_assert(type_name<std::string>() == "basic_string<char>");
static_assert(type_name<std::map<int, float>>() == "map<int,float>");
static_assert(type_name<std::shared_ptr<int>>() == "shared_ptr<int>");

// Custom types (namespace stripped)
namespace SceneryEditorX {
    struct Transform {};
    class Renderer {};
}
static_assert(type_name<SceneryEditorX::Transform>() == "Transform");
static_assert(type_name<SceneryEditorX::Renderer>() == "Renderer");

// Template types
template<typename T>
struct MyTemplate {};
static_assert(type_name<MyTemplate<int>>() == "MyTemplate<int>");
```

#### `type_name_keep_namespace<T>()`
```cpp
template <typename T>
constexpr std::string_view type_name_keep_namespace()
```

Returns the fully qualified type name including all namespace information. Essential for debugging and serialization where full type identification is needed.

**Examples:**
```cpp
// Custom types with full namespace preservation
static_assert(type_name_keep_namespace<SceneryEditorX::Transform>() == "SceneryEditorX::Transform");
static_assert(type_name_keep_namespace<SceneryEditorX::Renderer::VulkanDevice>() == "SceneryEditorX::Renderer::VulkanDevice");

// Standard library types with std namespace
static_assert(type_name_keep_namespace<std::vector<int>>() == "std::vector<int>");
static_assert(type_name_keep_namespace<std::unique_ptr<float>>() == "std::unique_ptr<float>");

// Nested template types
static_assert(type_name_keep_namespace<std::map<std::string, SceneryEditorX::GameObject>>() == 
              "std::map<std::string,SceneryEditorX::GameObject>");
```

## Implementation Details

### Compiler-Specific Patterns

The system adapts to different compilers by detecting specific patterns in function signatures:

```cpp
#if defined(__clang__)
    constexpr auto prefix = std::string_view{ "T = " };
    constexpr auto suffix = std::string_view{ "]" };
    constexpr auto function = std::string_view{ __PRETTY_FUNCTION__ };
#elif defined(__GNUC__)
    constexpr auto prefix = std::string_view{ "; T = " };
    constexpr auto suffix = std::string_view{ "]" };
    constexpr auto function = std::string_view{ __PRETTY_FUNCTION__ };
#elif defined(_MSC_VER)
    constexpr auto prefix = std::string_view{ "e," };
    constexpr auto suffix = std::string_view{ ">(void)" };
    constexpr auto function = std::string_view{ __FUNCSIG__ };
#endif
```

### String Processing Algorithm

The compile-time string extraction follows this pattern:

1. **Pattern Matching**: Find the prefix and suffix markers in the function signature
2. **Extraction**: Extract the substring between markers containing the type name
3. **Namespace Processing**: For non-namespace versions, find the last `::` and extract the final component
4. **Array Conversion**: Convert `string_view` to `std::array<char>` for storage
5. **Caching**: Use template specialization to cache results per type

```cpp
template <std::size_t...Idxs>
constexpr auto substring_as_array(std::string_view str, std::index_sequence<Idxs...>)
{
    return std::array{ str[Idxs]..., '\0' };
}

template <bool keep_namespace, typename T>
constexpr auto type_name_array()
{
    // Compiler-specific function signature extraction
    constexpr auto start = function.find(prefix) + prefix.size();
    constexpr auto end = function.rfind(suffix);
    constexpr auto name = function.substr(start, (end - start));
    
    // Namespace handling
    if constexpr (keep_namespace) {
        return substring_as_array(name, std::make_index_sequence<name.size()>{});
    } else {
        // Find last :: and extract final component
        constexpr auto new_start = name.find_last_of("::");
        if constexpr (new_start != std::string_view::npos) {
            constexpr auto stripped_name = name.substr(new_start + 1);
            return substring_as_array(stripped_name, std::make_index_sequence<stripped_name.size()>{});
        } else {
            return substring_as_array(name, std::make_index_sequence<name.size()>{});
        }
    }
}
```

### Template Specialization Cache

Results are cached using template specialization to avoid recomputation:

```cpp
template <typename T, bool keep_namespace>
struct type_name_holder
{
    static constexpr auto value = type_name_array<keep_namespace, T>();
};

template <typename T>
constexpr std::string_view type_name()
{
    constexpr auto& value = impl::type_name_holder<T, false>::value;
    return std::string_view{ value.data(), value.size() - 1 };
}
```

## Advanced Usage Patterns

### Template Metaprogramming Integration

The type names system integrates seamlessly with template metaprogramming:

```cpp
// Type-based dispatch using type names
template<typename T>
void ProcessType(const T& value)
{
    constexpr auto typeName = type_name<T>();
    
    if constexpr (typeName == "float" || typeName == "double") {
        // Floating-point processing
        ProcessFloat(value);
    } else if constexpr (typeName == "int" || typeName == "long") {
        // Integer processing
        ProcessInteger(value);
    } else {
        // Generic processing
        ProcessGeneric(value);
    }
}

// Compile-time type categorization
template<typename T>
constexpr bool IsNumericType()
{
    constexpr auto name = type_name<T>();
    return name == "int" || name == "float" || name == "double" || 
           name == "long" || name == "short" || name == "char";
}
```

### Runtime Type Information

Generate runtime type strings from compile-time information:

```cpp
// Convert compile-time type names to runtime strings
template<typename T>
std::string GetRuntimeTypeName()
{
    constexpr auto compile_time_name = type_name<T>();
    return std::string{compile_time_name.data(), compile_time_name.size()};
}

// Type registry using type names as keys
class TypeRegistry 
{
    std::unordered_map<std::string, std::unique_ptr<TypeInfo>> m_types;
    
public:
    template<typename T>
    void RegisterType()
    {
        auto typeName = GetRuntimeTypeName<T>();
        m_types[typeName] = std::make_unique<TypeInfoImpl<T>>();
    }
    
    TypeInfo* GetType(std::string_view typeName) const
    {
        auto it = m_types.find(std::string{typeName});
        return it != m_types.end() ? it->second.get() : nullptr;
    }
};
```

### Debugging and Logging Integration

Enhanced debugging with type information:

```cpp
// Logging with automatic type information
template<typename T>
void LogValue(const T& value, std::string_view variableName)
{
    constexpr auto typeName = type_name<T>();
    constexpr auto fullTypeName = type_name_keep_namespace<T>();
    
    if constexpr (is_streamable_v<T>) {
        LOG_DEBUG("Variable '{}' of type '{}' ({}): {}", 
                  variableName, typeName, fullTypeName, value);
    } else {
        LOG_DEBUG("Variable '{}' of type '{}' ({}): [not streamable]", 
                  variableName, typeName, fullTypeName);
    }
}

#define LOG_VAR(var) LogValue(var, #var)

// Usage
Vec3 position{1.0f, 2.0f, 3.0f};
LOG_VAR(position); // Logs: Variable 'position' of type 'Vec3' (SceneryEditorX::Vec3): {1, 2, 3}
```

### Serialization Support

Type-name-based serialization system:

```cpp
// JSON serialization with type information
template<typename T>
nlohmann::json SerializeWithTypeInfo(const T& value)
{
    nlohmann::json result;
    result["__type"] = type_name_keep_namespace<T>();
    
    if constexpr (Described<T>::value) {
        // Use reflection for described types
        result["data"] = SerializeReflected(value);
    } else if constexpr (is_streamable_v<T>) {
        // Direct serialization for streamable types
        std::stringstream ss;
        ss << value;
        result["data"] = ss.str();
    } else {
        result["data"] = nullptr;
        result["__error"] = "Type not serializable";
    }
    
    return result;
}

// Deserialization factory
template<typename T>
std::optional<T> DeserializeFromTypeInfo(const nlohmann::json& json)
{
    if (!json.contains("__type") || !json.contains("data")) {
        return std::nullopt;
    }
    
    std::string expectedType = type_name_keep_namespace<T>();
    if (json["__type"] != expectedType) {
        LOG_WARNING("Type mismatch: expected '{}', got '{}'", 
                   expectedType, json["__type"].get<std::string>());
        return std::nullopt;
    }
    
    return DeserializeData<T>(json["data"]);
}
```

## Integration with Reflection System

### Type Descriptor Integration

The type names system provides essential type information to the reflection framework:

```cpp
// Type names are automatically used in DESCRIBED macros
template<size_t Index>
static constexpr std::string_view GetMemberTypeName()
{
    using MemberType = typename member_pointer::return_type<
        std::remove_cvref_t<decltype(std::get<Index>(TTuple()))>>::type;
    return type::type_name<MemberType>(); // ← Integration point
}

// ClassInfo generation uses type names
template<class T>
static ClassInfo Of()
{
    ClassInfo info;
    info.Name = type_name<T>(); // ← Short name for class name
    
    for (const auto& memberName : Description<T>::MemberNames) {
        auto typeName = GetTypeNameByName(memberName); // ← Uses type_name<> internally
        info.Members.push_back(Member{
            .Name = memberName,
            .TypeName = std::string{typeName.data(), typeName.size()}
        });
    }
    
    return info;
}
```

### Member Type Resolution

Automatic type name extraction for class members:

```cpp
struct Shader
{
    std::string name;
    ShaderType type;
    std::vector<Uniform> uniforms;
    
    void Compile();
    bool IsCompiled() const;
};

DESCRIBED(Shader,
    &Shader::name,        // Type extracted as "basic_string<char>"
    &Shader::type,        // Type extracted as "ShaderType"  
    &Shader::uniforms,    // Type extracted as "vector<Uniform>"
    &Shader::Compile,     // Return type extracted as "void"
    &Shader::IsCompiled   // Return type extracted as "bool"
);

// The type names are automatically available through reflection
auto memberTypeName = Description<Shader>::GetTypeNameByName("name");
assert(*memberTypeName == "basic_string<char>");
```

## Performance Characteristics

### Compile-Time Evaluation

All type name extraction happens at compile time:

```cpp
// Zero runtime cost - computed at compile time
constexpr auto intTypeName = type_name<int>();
constexpr auto vectorTypeName = type_name<std::vector<float>>();

// Results are stored as static constexpr arrays
template<typename T>
struct TypeNameStorage 
{
    static constexpr auto name = type_name<T>();
    static constexpr auto full_name = type_name_keep_namespace<T>();
};
```

### Memory Usage

Type names are stored efficiently:
- **Compile-time**: No memory usage, values computed during compilation
- **Runtime storage**: Only when explicitly requested, stored as `std::string_view` pointing to static data
- **Caching**: Template specialization ensures each type's name is computed only once

### Benchmark Results

```cpp
// Compile-time access (zero overhead)
auto CompileTimeAccess() 
{
    constexpr auto name = type_name<MyComplexType>(); // No runtime cost
    return name;
}

// Runtime string conversion (minimal cost)
std::string RuntimeConversion()
{
    return std::string{type_name<MyComplexType>()}; // Only string allocation cost
}
```

## Best Practices

### 1. Prefer Compile-Time Access
```cpp
// ✅ Good: Compile-time type checking
template<typename T>
void ProcessSpecificType(const T& value)
{
    if constexpr (type_name<T>() == "Transform") {
        // Compile-time dispatch - zero overhead
        ProcessTransform(value);
    }
}

// ⚠️ Less efficient: Runtime string comparison
template<typename T>
void ProcessSpecificTypeRuntime(const T& value)
{
    std::string typeName{type_name<T>()};
    if (typeName == "Transform") { // Runtime string comparison
        ProcessTransform(value);
    }
}
```

### 2. Use Appropriate Namespace Version
```cpp
// ✅ Good: Use short names for user-facing display
void ShowPropertyEditor(std::string_view typeName)
{
    ImGui::Text("Type: %s", std::string{type_name<PropertyType>()}.c_str());
}

// ✅ Good: Use full names for serialization/unique identification
void SerializeType(const auto& value)
{
    json["__type"] = type_name_keep_namespace<decltype(value)>();
}
```

### 3. Leverage Template Specialization
```cpp
// ✅ Good: Specialize behavior based on type names
template<typename T>
struct Serializer
{
    static void Serialize(const T& value, Archive& ar)
    {
        // Generic implementation
    }
};

// Specialization based on type name pattern
template<typename T>
requires (type_name<T>().starts_with("std::"))
struct Serializer<T>
{
    static void Serialize(const T& value, Archive& ar)
    {
        // Optimized implementation for standard library types
    }
};
```

### 4. Error Handling for Unknown Types
```cpp
template<typename T>
std::optional<std::string> GetDisplayName()
{
    constexpr auto name = type_name<T>();
    
    // Handle known type mappings
    if constexpr (name == "basic_string<char>") {
        return "String";
    } else if constexpr (name.starts_with("vector<")) {
        return "Array";
    } else if constexpr (name.starts_with("map<")) {
        return "Dictionary";
    } else {
        // Return the raw type name for unknown types
        return std::string{name};
    }
}
```

## Common Use Cases

### 1. Dynamic Type Registry
```cpp
class ComponentRegistry
{
    std::unordered_map<std::string, std::function<std::unique_ptr<Component>()>> m_factories;
    
public:
    template<typename T>
    void RegisterComponent()
    {
        static_assert(std::is_base_of_v<Component, T>);
        
        std::string typeName{type_name<T>()};
        m_factories[typeName] = []() { return std::make_unique<T>(); };
    }
    
    std::unique_ptr<Component> CreateComponent(std::string_view typeName)
    {
        auto it = m_factories.find(std::string{typeName});
        return it != m_factories.end() ? it->second() : nullptr;
    }
};
```

### 2. Configuration System
```cpp
template<typename T>
void SaveToConfig(const T& value, std::string_view key)
{
    Config config;
    config.Set(std::string{key}, value);
    config.SetType(std::string{key}, type_name_keep_namespace<T>());
    config.Save();
}

template<typename T>
std::optional<T> LoadFromConfig(std::string_view key)
{
    Config config;
    config.Load();
    
    auto storedType = config.GetType(std::string{key});
    auto expectedType = type_name_keep_namespace<T>();
    
    if (storedType != expectedType) {
        LOG_WARNING("Config type mismatch for key '{}': expected '{}', got '{}'",
                   key, expectedType, storedType);
        return std::nullopt;
    }
    
    return config.Get<T>(std::string{key});
}
```

### 3. ImGui Property Display
```cpp
template<typename T>
void DisplayProperty(const T& value, std::string_view name)
{
    constexpr auto typeName = type_name<T>();
    
    if constexpr (typeName == "bool") {
        bool mutableValue = value;
        ImGui::Checkbox(std::string{name}.c_str(), &mutableValue);
    } else if constexpr (typeName == "float") {
        float mutableValue = value;
        ImGui::SliderFloat(std::string{name}.c_str(), &mutableValue, 0.0f, 1.0f);
    } else if constexpr (typeName == "int") {
        int mutableValue = value;
        ImGui::InputInt(std::string{name}.c_str(), &mutableValue);
    } else {
        // Display as read-only text for unknown types
        ImGui::Text("%s: %s", std::string{name}.c_str(), std::string{typeName}.c_str());
    }
}
```

## Troubleshooting

### Common Compiler Issues

1. **Template instantiation depth**
   ```cpp
   // Issue: Deep template recursion with complex types
   using ComplexType = std::map<std::string, std::vector<std::shared_ptr<MyClass>>>;
   auto name = type_name<ComplexType>(); // May hit instantiation limits
   
   // Solution: Increase template depth or simplify types
   #pragma GCC optimize("-ftemplate-depth=1024")
   ```

2. **Compiler-specific differences**
   ```cpp
   // MSVC may produce: "class std::basic_string<char>"
   // GCC/Clang may produce: "std::basic_string<char>"
   
   // Solution: Use consistent string matching
   constexpr bool IsString(std::string_view typeName) {
       return typeName.ends_with("basic_string<char>") || 
              typeName.ends_with("string");
   }
   ```

3. **Template parameter formatting**
   ```cpp
   // Different compilers may format template parameters differently
   // "vector<int>" vs "vector<int >" vs "std::vector<int, std::allocator<int>>"
   
   // Solution: Use prefix/suffix matching for template types
   constexpr bool IsVector(std::string_view typeName) {
       return typeName.starts_with("vector<") || 
              typeName.contains("::vector<");
   }
   ```

### Debugging Type Name Issues

```cpp
// Debug macro to print type names at compile time
#define DEBUG_TYPE_NAME(T) \
    static_assert(false, "Type: " #T " -> " type_name<T>().data())

// Usage during development
// DEBUG_TYPE_NAME(MyComplexType); // Will show resolved name in compiler error

// Runtime debugging helper
template<typename T>
void PrintTypeInfo()
{
    std::cout << "Type: " << type_name<T>() << "\n";
    std::cout << "Full: " << type_name_keep_namespace<T>() << "\n";
    std::cout << "Size: " << sizeof(T) << " bytes\n";
}
```

This comprehensive type name system provides the foundation for all type identification needs in Scenery Editor X, enabling powerful reflection, debugging, and dynamic type handling capabilities while maintaining excellent compile-time performance.
2. **Extraction**: Extract the substring between markers containing the type name
3. **Namespace Filtering**: Optionally remove namespace qualifiers by finding the last `::`
4. **Array Conversion**: Convert the `std::string_view` to a compile-time character array

### Template Specialization Holder

```cpp
template <typename T, bool keep_namespace>
struct type_name_holder
{
    static constexpr auto value = type_name_array<keep_namespace, T>();
};
```

This holder pattern ensures that the expensive compile-time string processing is performed only once per type and caches the result.

## Usage Patterns

### Debug and Logging Integration

```cpp
template<typename T>
void LogObjectCreation(const T& obj)
{
    SEDX_CORE_INFO_TAG("OBJECT", "Created object of type: {}", type_name<T>());
}

// Usage
Transform transform;
LogObjectCreation(transform); // Logs: "Created object of type: Transform"
```

### Reflection System Integration

```cpp
template<typename T>
struct TypeInfo
{
    static constexpr std::string_view Name = type_name<T>();
    static constexpr std::string_view FullName = type_name_keep_namespace<T>();
    static constexpr size_t Size = sizeof(T);
};

// Usage
using TransformInfo = TypeInfo<Transform>;
static_assert(TransformInfo::Name == "Transform");
static_assert(TransformInfo::FullName == "SceneryEditorX::Transform");
```

### Serialization Type Headers

```cpp
template<typename T>
void WriteTypeHeader(StreamWriter* writer, const T& obj)
{
    // Write type name for versioning and debugging
    const auto typeName = type_name<T>();
    writer->WriteString(std::string(typeName));
    
    SEDX_CORE_DEBUG_TAG("SERIALIZATION", "Serializing type: {}", typeName);
}

template<typename T>
bool ReadTypeHeader(StreamReader* reader)
{
    const auto expectedType = type_name<T>();
    const auto actualType = reader->ReadString();
    
    if (actualType != expectedType) {
        SEDX_CORE_ERROR_TAG("SERIALIZATION", 
            "Type mismatch: expected {}, got {}", expectedType, actualType);
        return false;
    }
    
    return true;
}
```

### Template Metaprogramming

```cpp
template<typename T>
constexpr bool IsStandardLibraryType()
{
    constexpr auto fullName = type_name_keep_namespace<T>();
    return fullName.starts_with("std::");
}

template<typename T>
constexpr bool IsSceneryEditorXType()
{
    constexpr auto fullName = type_name_keep_namespace<T>();
    return fullName.starts_with("SceneryEditorX::");
}

// Usage in template constraints
template<typename T>
void ProcessCustomType(const T& obj)
{
    static_assert(IsSceneryEditorXType<T>(), "Function only accepts SceneryEditorX types");
    
    SEDX_CORE_INFO("Processing custom type: {}", type_name<T>());
    // ...
}
```

### Error Messages and Diagnostics

```cpp
template<typename Expected, typename Actual>
void ThrowTypeMismatchError()
{
    const auto expectedName = type_name<Expected>();
    const auto actualName = type_name<Actual>();
    
    throw std::runtime_error(fmt::format(
        "Type mismatch: expected '{}', but got '{}'", 
        expectedName, actualName
    ));
}

// Enhanced assertions with type information
#define SEDX_ASSERT_TYPE_MATCH(expected, actual) \
    if constexpr (!std::is_same_v<expected, actual>) { \
        SEDX_CORE_ERROR("Type mismatch: expected '{}', got '{}'", \
            type_name<expected>(), type_name<actual>()); \
        SEDX_ASSERT(false, "Type assertion failed"); \
    }
```

### Dynamic Type Registration

```cpp
class TypeRegistry
{
public:
    template<typename T>
    void RegisterType()
    {
        const auto typeName = type_name<T>();
        const auto fullTypeName = type_name_keep_namespace<T>();
        
        TypeData data{
            .name = std::string(typeName),
            .fullName = std::string(fullTypeName),
            .size = sizeof(T),
            .alignment = alignof(T)
        };
        
        m_Types[std::string(typeName)] = std::move(data);
        
        SEDX_CORE_INFO_TAG("REGISTRY", "Registered type: {} ({})", 
            typeName, fullTypeName);
    }
    
    std::optional<TypeData> GetTypeInfo(std::string_view name) const
    {
        auto it = m_Types.find(std::string(name));
        return (it != m_Types.end()) ? std::optional(it->second) : std::nullopt;
    }

private:
    struct TypeData
    {
        std::string name;
        std::string fullName;
        size_t size;
        size_t alignment;
    };
    
    std::unordered_map<std::string, TypeData> m_Types;
};
```

## Advanced Usage Examples

### Compile-Time Type Name Validation

```cpp
template<typename T>
constexpr bool HasValidTypeName()
{
    constexpr auto name = type_name<T>();
    return !name.empty() && !name.starts_with("(anonymous") && !name.contains("lambda");
}

// Ensure types have reasonable names for serialization
template<typename T>
void SerializeNamedType(StreamWriter* writer, const T& obj)
{
    static_assert(HasValidTypeName<T>(), 
        "Type must have a valid name for serialization");
    
    WriteTypeHeader(writer, obj);
    // ... serialize object
}
```

### Type Name-Based Factory Pattern

```cpp
class ComponentFactory
{
public:
    template<typename T>
    void RegisterComponent()
    {
        static_assert(std::is_base_of_v<Component, T>);
        
        const auto typeName = type_name<T>();
        m_Creators[std::string(typeName)] = []() -> std::unique_ptr<Component> {
            return std::make_unique<T>();
        };
        
        SEDX_CORE_INFO_TAG("FACTORY", "Registered component type: {}", typeName);
    }
    
    std::unique_ptr<Component> CreateComponent(std::string_view typeName) const
    {
        auto it = m_Creators.find(std::string(typeName));
        if (it != m_Creators.end()) {
            return it->second();
        }
        
        SEDX_CORE_WARN_TAG("FACTORY", "Unknown component type: {}", typeName);
        return nullptr;
    }

private:
    std::unordered_map<std::string, std::function<std::unique_ptr<Component>()>> m_Creators;
};

// Registration
factory.RegisterComponent<TransformComponent>();
factory.RegisterComponent<MeshRendererComponent>();

// Usage with automatic type name resolution
auto component = factory.CreateComponent(type_name<TransformComponent>());
```

### Debugging and Profiling Integration

```cpp
template<typename T>
class ScopedProfiler
{
public:
    ScopedProfiler()
    {
        constexpr auto typeName = type_name<T>();
        m_ProfileName = fmt::format("{}::{}", typeName, __FUNCTION__);
        
        SEDX_PROFILE_BEGIN(m_ProfileName.c_str());
        SEDX_CORE_TRACE_TAG("PROFILER", "Begin profiling: {}", m_ProfileName);
    }
    
    ~ScopedProfiler()
    {
        SEDX_PROFILE_END();
        SEDX_CORE_TRACE_TAG("PROFILER", "End profiling: {}", m_ProfileName);
    }

private:
    std::string m_ProfileName;
};

// Usage in template functions
template<typename T>
void ProcessComplexOperation(const T& data)
{
    ScopedProfiler<T> profiler; // Automatically names profile scope with type
    
    // ... complex processing
}
```

## Performance Considerations

### Compile-Time Efficiency
- All operations are performed at compile time
- Results are cached in template specializations
- No runtime overhead for type name retrieval

### Memory Usage
- Type names are stored as `constexpr` arrays
- Minimal memory footprint per unique type
- String views provide zero-copy access

### Compilation Impact
- May increase compilation time for large numbers of types
- Template instantiation cost is proportional to type complexity
- Consider explicit instantiation for frequently used types

## Integration Guidelines

### With Logging System
```cpp
#define SEDX_LOG_TYPE_INFO(type, message, ...) \
    SEDX_CORE_INFO_TAG(type_name<type>(), message, ##__VA_ARGS__)

#define SEDX_LOG_TYPE_ERROR(type, message, ...) \
    SEDX_CORE_ERROR_TAG(type_name<type>(), message, ##__VA_ARGS__)

// Usage
SEDX_LOG_TYPE_INFO(Transform, "Transform component initialized");
SEDX_LOG_TYPE_ERROR(Renderer, "Failed to create render target");
```

### With Module System
```cpp
template<typename ModuleType>
class TypedModule : public Module
{
public:
    TypedModule() : Module(std::string(type_name<ModuleType>())) {}
    
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("MODULE", "Attaching module: {}", type_name<ModuleType>());
        static_cast<ModuleType*>(this)->OnModuleAttach();
    }
    
protected:
    virtual void OnModuleAttach() = 0;
};

// Usage
class RenderModule : public TypedModule<RenderModule>
{
protected:
    void OnModuleAttach() override
    {
        // Module-specific initialization
        SEDX_CORE_INFO("Render module attached with automatic naming");
    }
};
```

## Best Practices

1. **Prefer `type_name<T>()` for most use cases** - It provides clean, readable names
2. **Use `type_name_keep_namespace<T>()` for serialization** - Full qualification prevents naming conflicts
3. **Cache type names in hot paths** - Even though compile-time, avoid redundant template instantiation
4. **Combine with static_assert for validation** - Ensure type names meet requirements at compile time
5. **Integrate with logging tags** - Use type names as structured logging categories

The Type Names system provides a foundation for runtime type identification and debugging while maintaining zero runtime overhead and full compile-time safety.

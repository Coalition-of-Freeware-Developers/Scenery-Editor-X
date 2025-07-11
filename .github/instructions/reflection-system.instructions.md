# Scenery Editor X - Reflection System - Development Instructions

## Overview for AI Development Assistance

This document provides specific instructions for GitHub Copilot instances and agent modes to understand and correctly utilize the SceneryEditorX reflection system. The system provides comprehensive compile-time type introspection, runtime reflection, and dynamic type manipulation capabilities built around the `DESCRIBED` macro and supporting utilities.

## Core Architecture Understanding

### Reflection System Hierarchy

```
Application Code (uses DESCRIBED macro)
    ↓
Type Descriptors (compile-time reflection via Description<T>)
    ↓
Type Names (compile-time type name extraction)
    ↓
Type Utils (template metaprogramming utilities)
    ↓
Type Structs (runtime ClassInfo and Member structures)
```

### Key Files and Their Roles

- `source/SceneryEditorX/utils/reflection/type_descriptors.h` - Core DESCRIBED macro and Description<T> template
- `source/SceneryEditorX/utils/reflection/type_names.h` - Compile-time type name extraction
- `source/SceneryEditorX/utils/reflection/type_utils.h` - Template metaprogramming utilities
- `source/SceneryEditorX/utils/reflection/type_structs.h` - Runtime reflection structures

## Implementation Patterns for AI Assistance

### 1. Making Types Reflectable (DESCRIBED Macro)

**ALWAYS use these patterns when creating new types that need reflection:**

```cpp
// ✅ CORRECT - Basic class description
struct Transform
{
    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 rotation{0.0f, 0.0f, 0.0f};
    Vec3 scale{1.0f, 1.0f, 1.0f};
    bool visible = true;
    
    void Reset() { *this = Transform{}; }
    bool IsIdentity() const { return position == Vec3{} && rotation == Vec3{} && scale == Vec3{1,1,1}; }
};

// Place DESCRIBED at global scope, after class definition
DESCRIBED(Transform,
    &Transform::position,
    &Transform::rotation,
    &Transform::scale,
    &Transform::visible,
    &Transform::Reset,
    &Transform::IsIdentity
);

// ✅ CORRECT - Namespaced types
namespace SceneryEditorX::Renderer {
    struct Material {
        std::string name;
        Vec3 albedo;
        float metallic = 0.0f;
        float roughness = 0.5f;
        
        void LoadFromFile(const std::string& path);
        bool IsValid() const;
    };
}

DESCRIBED(SceneryEditorX::Renderer::Material,
    &SceneryEditorX::Renderer::Material::name,
    &SceneryEditorX::Renderer::Material::albedo,
    &SceneryEditorX::Renderer::Material::metallic,
    &SceneryEditorX::Renderer::Material::roughness,
    &SceneryEditorX::Renderer::Material::LoadFromFile,
    &SceneryEditorX::Renderer::Material::IsValid
);

// ✅ CORRECT - Tagged descriptions for different use cases
struct GameEntity {
    uint32_t id;
    std::string name;
    Transform transform;
    bool networkSync = false;
    float health = 100.0f;
};

// Default description (all members)
DESCRIBED(GameEntity,
    &GameEntity::id,
    &GameEntity::name,
    &GameEntity::transform,
    &GameEntity::networkSync,
    &GameEntity::health
);

// Network-only description (only networked data)
struct NetworkTag {};
DESCRIBED_TAGGED(GameEntity, NetworkTag,
    &GameEntity::id,
    &GameEntity::transform,
    &GameEntity::networkSync
);
```

**AVOID these patterns:**

```cpp
// ❌ WRONG - Missing DESCRIBED macro
struct BadTransform {
    Vec3 position;
    // No DESCRIBED macro - not reflectable
};

// ❌ WRONG - DESCRIBED inside namespace or class
namespace SceneryEditorX {
    struct MyClass {};
    DESCRIBED(MyClass, /*...*/); // Wrong - must be at global scope
}

// ❌ WRONG - Incomplete member list
struct PartialReflection {
    int a, b, c;
};
DESCRIBED(PartialReflection, &PartialReflection::a); // Missing b and c

// ❌ WRONG - Static members (not supported)
struct UnsupportedMembers {
    static int staticValue; // Cannot be reflected
    int normalValue;
};
DESCRIBED(UnsupportedMembers, &UnsupportedMembers::staticValue); // Error
```

### 2. Accessing Type Information

**Use these patterns for type introspection:**

```cpp
// ✅ CORRECT - Compile-time type name access
template<typename T>
void LogTypeInfo()
{
    constexpr auto typeName = type_name<T>();
    constexpr auto fullTypeName = type_name_keep_namespace<T>();
    
    LOG_INFO("Type: {} (full: {})", typeName, fullTypeName);
    LOG_INFO("Size: {} bytes", sizeof(T));
    
    if constexpr (Described<T>::value) {
        LOG_INFO("Reflectable: Yes ({} members)", Description<T>::NumberOfMembers);
    } else {
        LOG_INFO("Reflectable: No");
    }
}

// ✅ CORRECT - Runtime class information
template<typename T>
void InspectClass()
{
    static_assert(Described<T>::value, "Type must be described");
    
    ClassInfo info = ClassInfo::Of<T>();
    
    std::cout << "Class: " << info.Name << "\n";
    std::cout << "Size: " << info.Size << " bytes\n";
    std::cout << "Members:\n";
    
    for (const auto& member : info.Members) {
        std::cout << "  " << (member.Type == Member::Data ? "Data" : "Func")
                  << " | " << member.TypeName << " " << member.Name;
        if (member.Type == Member::Data) {
            std::cout << " (" << member.Size << " bytes)";
        }
        std::cout << "\n";
    }
}

// ✅ CORRECT - Member access by name
template<typename T>
void ProcessMemberByName(T& object, std::string_view memberName)
{
    using Desc = Description<T>;
    
    // Check if member exists
    auto memberIndex = Desc::GetMemberIndexByName(memberName);
    if (!memberIndex) {
        LOG_WARNING("Member '{}' not found in type '{}'", memberName, type_name<T>());
        return;
    }
    
    // Get member type information
    auto typeName = Desc::GetTypeNameByName(memberName);
    auto isFunction = Desc::IsFunctionByName(memberName);
    
    if (*isFunction) {
        LOG_INFO("Calling function member: {}", memberName);
        // Call function through reflection if needed
    } else {
        LOG_INFO("Accessing data member: {} (type: {})", memberName, *typeName);
        
        // Type-specific processing
        if (*typeName == "float") {
            auto value = Desc::template GetMemberValueOfType<float>(memberName, object);
            if (value) ProcessFloatValue(*value);
        } else if (*typeName == "Vec3") {
            auto value = Desc::template GetMemberValueOfType<Vec3>(memberName, object);
            if (value) ProcessVec3Value(*value);
        }
    }
}
```

### 3. Property Editors and UI Generation

**Use these patterns for dynamic UI generation:**

```cpp
// ✅ CORRECT - Generic property editor
template<typename T>
class ReflectionPropertyEditor
{
    static_assert(Described<T>::value, "Type must be described for property editing");
    
public:
    void RenderEditor(T& object)
    {
        using Desc = Description<T>;
        
        ImGui::Begin(fmt::format("Editor: {}", Desc::ClassName).c_str());
        
        // Render each data member
        for (size_t i = 0; i < Desc::NumberOfMembers; ++i) {
            auto memberName = Desc::MemberNames[i];
            auto isFunction = *Desc::IsFunctionByIndex(i);
            
            if (isFunction) continue; // Skip functions for data editor
            
            RenderMemberEditor(object, memberName, i);
        }
        
        ImGui::End();
    }

private:
    void RenderMemberEditor(T& object, std::string_view memberName, size_t memberIndex)
    {
        using Desc = Description<T>;
        auto typeName = *Desc::GetTypeNameByIndex(memberIndex);
        
        // Type-specific editor widgets
        if (typeName == "float") {
            auto value = Desc::template GetMemberValueOfType<float>(memberName, object);
            if (value) {
                float editValue = *value;
                if (ImGui::SliderFloat(std::string{memberName}.c_str(), &editValue, 0.0f, 100.0f)) {
                    Desc::SetMemberValueByName(memberName, object, editValue);
                }
            }
        } else if (typeName == "bool") {
            auto value = Desc::template GetMemberValueOfType<bool>(memberName, object);
            if (value) {
                bool editValue = *value;
                if (ImGui::Checkbox(std::string{memberName}.c_str(), &editValue)) {
                    Desc::SetMemberValueByName(memberName, object, editValue);
                }
            }
        } else if (typeName == "Vec3") {
            auto value = Desc::template GetMemberValueOfType<Vec3>(memberName, object);
            if (value) {
                Vec3 editValue = *value;
                if (ImGui::SliderFloat3(std::string{memberName}.c_str(), &editValue.x, -10.0f, 10.0f)) {
                    Desc::SetMemberValueByName(memberName, object, editValue);
                }
            }
        } else {
            // Fallback for unknown types
            ImGui::Text("%s: %s", std::string{memberName}.c_str(), std::string{typeName}.c_str());
        }
    }
};
```

### 4. Serialization Patterns

**Use these patterns for reflection-based serialization:**

```cpp
// ✅ CORRECT - JSON serialization
template<typename T>
nlohmann::json SerializeReflected(const T& object)
{
    static_assert(Described<T>::value, "Type must be described for serialization");
    
    nlohmann::json result;
    using Desc = Description<T>;
    
    result["__type"] = type_name_keep_namespace<T>();
    result["__version"] = 1; // Version for compatibility
    
    // Serialize only data members
    for (size_t i = 0; i < Desc::NumberOfMembers; ++i) {
        auto memberName = Desc::MemberNames[i];
        auto isFunction = *Desc::IsFunctionByIndex(i);
        
        if (isFunction) continue; // Skip functions
        
        auto typeName = *Desc::GetTypeNameByIndex(i);
        
        // Type-specific serialization
        if (typeName == "float") {
            auto value = Desc::template GetMemberValueOfType<float>(memberName, object);
            if (value) result["data"][std::string{memberName}] = *value;
        } else if (typeName == "bool") {
            auto value = Desc::template GetMemberValueOfType<bool>(memberName, object);
            if (value) result["data"][std::string{memberName}] = *value;
        } else if (typeName == "Vec3") {
            auto value = Desc::template GetMemberValueOfType<Vec3>(memberName, object);
            if (value) {
                result["data"][std::string{memberName}] = {
                    {"x", value->x}, {"y", value->y}, {"z", value->z}
                };
            }
        } else if (typeName == "basic_string<char>") {
            auto value = Desc::template GetMemberValueOfType<std::string>(memberName, object);
            if (value) result["data"][std::string{memberName}] = *value;
        }
    }
    
    return result;
}

// ✅ CORRECT - JSON deserialization
template<typename T>
std::optional<T> DeserializeReflected(const nlohmann::json& json)
{
    static_assert(Described<T>::value, "Type must be described for deserialization");
    
    // Validate type
    if (!json.contains("__type") || json["__type"] != type_name_keep_namespace<T>()) {
        LOG_ERROR("Type mismatch in deserialization");
        return std::nullopt;
    }
    
    T object{};
    using Desc = Description<T>;
    
    if (!json.contains("data")) {
        LOG_WARNING("No data section in JSON");
        return object; // Return default-constructed object
    }
    
    const auto& data = json["data"];
    
    // Deserialize each data member
    for (size_t i = 0; i < Desc::NumberOfMembers; ++i) {
        auto memberName = Desc::MemberNames[i];
        auto isFunction = *Desc::IsFunctionByIndex(i);
        
        if (isFunction) continue; // Skip functions
        
        std::string memberKey{memberName};
        if (!data.contains(memberKey)) continue;
        
        auto typeName = *Desc::GetTypeNameByIndex(i);
        
        // Type-specific deserialization
        if (typeName == "float" && data[memberKey].is_number()) {
            Desc::SetMemberValueByName(memberName, object, data[memberKey].get<float>());
        } else if (typeName == "bool" && data[memberKey].is_boolean()) {
            Desc::SetMemberValueByName(memberName, object, data[memberKey].get<bool>());
        } else if (typeName == "Vec3" && data[memberKey].is_object()) {
            const auto& vecData = data[memberKey];
            if (vecData.contains("x") && vecData.contains("y") && vecData.contains("z")) {
                Vec3 vec{vecData["x"], vecData["y"], vecData["z"]};
                Desc::SetMemberValueByName(memberName, object, vec);
            }
        } else if (typeName == "basic_string<char>" && data[memberKey].is_string()) {
            Desc::SetMemberValueByName(memberName, object, data[memberKey].get<std::string>());
        }
    }
    
    return object;
}
```

### 5. Component System Integration

**Use these patterns for reflection-based component systems:**

```cpp
// ✅ CORRECT - Component registry with reflection
class ComponentRegistry
{
    std::unordered_map<std::string, ClassInfo> m_componentInfo;
    std::unordered_map<std::string, std::function<std::unique_ptr<Component>()>> m_factories;
    
public:
    template<typename T>
    void RegisterComponent()
    {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
        static_assert(Described<T>::value, "Component type must be described");
        
        ClassInfo info = ClassInfo::Of<T>();
        m_componentInfo[info.Name] = std::move(info);
        m_factories[info.Name] = []() { return std::make_unique<T>(); };
        
        LOG_INFO("Registered component: {} ({} bytes, {} members)", 
                 info.Name, info.Size, info.Members.size());
    }
    
    std::unique_ptr<Component> CreateComponent(const std::string& typeName)
    {
        auto it = m_factories.find(typeName);
        return it != m_factories.end() ? it->second() : nullptr;
    }
    
    const ClassInfo* GetComponentInfo(const std::string& typeName) const
    {
        auto it = m_componentInfo.find(typeName);
        return it != m_componentInfo.end() ? &it->second : nullptr;
    }
    
    std::vector<std::string> GetRegisteredComponents() const
    {
        std::vector<std::string> names;
        for (const auto& [name, info] : m_componentInfo) {
            names.push_back(name);
        }
        return names;
    }
};

// Usage
ComponentRegistry registry;
registry.RegisterComponent<TransformComponent>();
registry.RegisterComponent<MeshRendererComponent>();
registry.RegisterComponent<ColliderComponent>();
```

## Performance Guidelines

### 1. Compile-Time vs Runtime Access

```cpp
// ✅ PREFER - Compile-time access when member index is known
template<typename T>
void OptimizedAccess(T& object)
{
    using Desc = Description<T>;
    
    // Zero-overhead compile-time access
    auto& firstMember = Desc::template GetMemberValue<0>(object);
    auto& secondMember = Desc::template GetMemberValue<1>(object);
    
    // Process members with full optimization
    ProcessMember(firstMember);
    ProcessMember(secondMember);
}

// ⚠️ USE SPARINGLY - Runtime access when dynamic behavior is needed
template<typename T>
void DynamicAccess(T& object, const std::vector<std::string>& memberNames)
{
    using Desc = Description<T>;
    
    for (const auto& name : memberNames) {
        // Runtime hash map lookup - has overhead
        auto value = Desc::GetMemberValueVariantByName(name, object);
        if (value) {
            ProcessVariantValue(*value);
        }
    }
}
```

### 2. Template Instantiation Optimization

```cpp
// ✅ CORRECT - Use type traits to avoid unnecessary instantiations
template<typename T>
void ConditionalProcessing(T& object)
{
    if constexpr (Described<T>::value) {
        ProcessReflectedType(object);
    } else if constexpr (is_streamable_v<T>) {
        ProcessStreamableType(object);
    } else {
        ProcessGenericType(object);
    }
}

// ❌ AVOID - Unconditional reflection usage
template<typename T>
void UnconditionalProcessing(T& object)
{
    Description<T> desc; // Error if T is not described
    // Process...
}
```

### 3. Memory-Efficient ClassInfo Usage

```cpp
// ✅ CORRECT - Cache ClassInfo for repeated use
template<typename T>
class ComponentProcessor
{
    static const ClassInfo& GetCachedInfo()
    {
        static ClassInfo info = ClassInfo::Of<T>();
        return info;
    }
    
public:
    void ProcessComponent(T& component)
    {
        const auto& info = GetCachedInfo(); // Cached, no regeneration
        ProcessWithInfo(component, info);
    }
};

// ❌ AVOID - Regenerating ClassInfo repeatedly
template<typename T>
void InefficientProcessing(T& component)
{
    ClassInfo info = ClassInfo::Of<T>(); // Regenerated every call
    ProcessWithInfo(component, info);
}
```

## Error Handling Patterns

### 1. Type Validation

```cpp
// ✅ CORRECT - Comprehensive type checking
template<typename T>
bool ValidateReflectedType()
{
    // Compile-time check
    if constexpr (!Described<T>::value) {
        LOG_ERROR("Type '{}' is not described", type_name<T>());
        return false;
    }
    
    // Runtime validation
    try {
        ClassInfo info = ClassInfo::Of<T>();
        
        if (info.Name.empty()) {
            LOG_ERROR("Type '{}' has empty name", type_name<T>());
            return false;
        }
        
        if (info.Size != sizeof(T)) {
            LOG_ERROR("Type '{}' size mismatch: expected {}, got {}", 
                     type_name<T>(), sizeof(T), info.Size);
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception validating type '{}': {}", type_name<T>(), e.what());
        return false;
    }
}
```

### 2. Safe Member Access

```cpp
// ✅ CORRECT - Safe member access with error handling
template<typename T, typename ValueType>
std::optional<ValueType> SafeGetMemberValue(const T& object, std::string_view memberName)
{
    static_assert(Described<T>::value, "Type must be described");
    
    using Desc = Description<T>;
    
    // Check member existence
    auto memberIndex = Desc::GetMemberIndexByName(memberName);
    if (!memberIndex) {
        LOG_WARNING("Member '{}' not found in type '{}'", memberName, type_name<T>());
        return std::nullopt;
    }
    
    // Check if it's a function
    auto isFunction = Desc::IsFunctionByName(memberName);
    if (!isFunction || *isFunction) {
        LOG_WARNING("Member '{}' is a function, not data", memberName);
        return std::nullopt;
    }
    
    // Get expected type
    auto expectedType = Desc::GetTypeNameByName(memberName);
    auto actualType = type_name<ValueType>();
    
    if (!expectedType || *expectedType != actualType) {
        LOG_WARNING("Type mismatch for member '{}': expected '{}', requested '{}'", 
                   memberName, expectedType ? *expectedType : "unknown", actualType);
        return std::nullopt;
    }
    
    // Safe access
    return Desc::template GetMemberValueOfType<ValueType>(memberName, object);
}
```

## Common Anti-Patterns to Avoid

### 1. Reflection System Misuse

```cpp
// ❌ WRONG - Using reflection for simple known-type access
struct SimpleStruct {
    float x, y;
};
DESCRIBED(SimpleStruct, &SimpleStruct::x, &SimpleStruct::y);

void BadUsage(SimpleStruct& s) {
    // Unnecessary overhead - use direct access instead
    auto x = Description<SimpleStruct>::GetMemberValueByName<float>("x", s);
}

void GoodUsage(SimpleStruct& s) {
    // Direct access - zero overhead
    float x = s.x;
}

// ❌ WRONG - Reflection in performance-critical loops
void BadLoop(std::vector<Transform>& transforms) {
    for (auto& transform : transforms) {
        // Reflection lookup in inner loop - very slow
        auto pos = Description<Transform>::GetMemberValueByName<Vec3>("position", transform);
    }
}

void GoodLoop(std::vector<Transform>& transforms) {
    for (auto& transform : transforms) {
        // Direct access - optimized
        Vec3& pos = transform.position;
    }
}
```

### 2. Incomplete Type Descriptions

```cpp
// ❌ WRONG - Incomplete member listing
struct PartialReflection {
    int important;
    float alsoImportant;
    bool forgotten;  // Missing from DESCRIBED!
};

DESCRIBED(PartialReflection, 
    &PartialReflection::important,
    &PartialReflection::alsoImportant
    // Missing &PartialReflection::forgotten
);

// ✅ CORRECT - Complete member listing
DESCRIBED(PartialReflection,
    &PartialReflection::important,
    &PartialReflection::alsoImportant,
    &PartialReflection::forgotten
);
```

### 3. Incorrect Error Handling

```cpp
// ❌ WRONG - Assuming reflection always works
template<typename T>
void AssumeReflection(T& object) {
    ClassInfo info = ClassInfo::Of<T>(); // May throw if T not described
    // Process without checking...
}

// ✅ CORRECT - Defensive programming
template<typename T>
void SafeReflection(T& object) {
    if constexpr (Described<T>::value) {
        try {
            ClassInfo info = ClassInfo::Of<T>();
            ProcessReflectedObject(object, info);
        } catch (const std::exception& e) {
            LOG_ERROR("Reflection failed for {}: {}", type_name<T>(), e.what());
            ProcessFallback(object);
        }
    } else {
        ProcessFallback(object);
    }
}
```

## Integration Guidelines

### 1. Always prefer compile-time when possible
- Use `type_name<T>()` for type identification
- Use `Description<T>::GetMemberValue<N>()` for known indices
- Use `if constexpr (Described<T>::value)` for conditional reflection

### 2. Cache expensive operations
- Store `ClassInfo` results in static variables
- Reuse reflection data across multiple objects of the same type

### 3. Validate types and members
- Check `Described<T>::value` before using reflection
- Validate member names and types before access
- Handle reflection failures gracefully

### 4. Use appropriate access patterns
- Compile-time access for performance-critical code
- Runtime access for dynamic/data-driven scenarios
- Variant access for generic processing

### 5. Follow naming conventions
- Use PascalCase for described class names
- Use camelCase for member names in DESCRIBED macros
- Include all relevant members in descriptions

This reflection system provides powerful introspection capabilities while maintaining excellent performance when used correctly. Always prefer direct access for known types and reserve reflection for truly dynamic scenarios.

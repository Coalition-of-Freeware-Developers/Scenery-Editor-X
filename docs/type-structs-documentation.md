# Type Structs System Documentation

## Overview

The Type Structs system (`type_structs.h`) provides runtime reflection and introspection structures for C++ classes in Scenery Editor X. This system bridges compile-time type information with runtime data structures, enabling dynamic inspection, serialization metadata, debugging capabilities, and runtime type manipulation.

This system works in conjunction with the Type Descriptors system to convert compile-time reflection data into runtime-accessible structures, making it possible to inspect types dynamically, build property editors, implement serialization systems, and create debugging tools.

## Core Architecture

### Member Structure

The `Member` struct represents comprehensive metadata about individual class members, supporting both data members and member functions:

```cpp
struct Member
{
    std::string Name;        // Member name (e.g., "position", "GetTransform")
    size_t Size;            // Size in bytes (0 for functions)
    std::string TypeName;   // Type name (e.g., "Vec3", "float", "bool")
    
    enum EType : uint8_t
    {
        Function,           // Member function
        Data               // Data member (variable)
    } Type;
    
    bool operator==(const Member& other) const;
};
```

**Member Properties:**
- **Name**: The exact identifier as it appears in code
- **Size**: Memory footprint for data members, 0 for functions
- **TypeName**: Human-readable type identifier (uses type_name<> system)
- **Type**: Classification as either data or function member

### ClassInfo Structure

The `ClassInfo` struct contains comprehensive metadata about a described class, serving as the primary runtime reflection interface:

```cpp
struct ClassInfo
{
    std::string Name;               // Class name (without namespace)
    size_t Size;                   // sizeof(T) - total class size in bytes
    std::vector<Member> Members;   // All described members (data and functions)
    
    template<class T>
    static ClassInfo Of();         // Generate ClassInfo from described type
    
    bool operator==(const ClassInfo& other) const;
};
```

**ClassInfo Properties:**
- **Name**: Short class name for display purposes
- **Size**: Total memory footprint of the class
- **Members**: Ordered list of all members defined in DESCRIBED macro

## Key Features

### 1. Runtime Type Information Generation

The `ClassInfo::Of<T>()` static method generates comprehensive runtime reflection data from compile-time descriptions:

```cpp
// Define a complex class with mixed member types
struct Transform
{
    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 rotation{0.0f, 0.0f, 0.0f};
    Vec3 scale{1.0f, 1.0f, 1.0f};
    bool visible = true;
    float opacity = 1.0f;
    
    void Reset() { *this = Transform{}; }
    bool IsIdentity() const { return position == Vec3{} && rotation == Vec3{} && scale == Vec3{1,1,1}; }
    Vec3 GetWorldPosition() const { return position; }
    void SetPosition(const Vec3& pos) { position = pos; }
};

// Make the type reflectable
DESCRIBED(Transform,
    &Transform::position,
    &Transform::rotation,
    &Transform::scale,
    &Transform::visible,
    &Transform::opacity,
    &Transform::Reset,
    &Transform::IsIdentity,
    &Transform::GetWorldPosition,
    &Transform::SetPosition
);

// Generate runtime reflection information
ClassInfo info = ClassInfo::Of<Transform>();

// Runtime accessible metadata:
// info.Name = "Transform"
// info.Size = sizeof(Transform) (e.g., 52 bytes)
// info.Members contains 9 entries:
//   - 5 data members (position, rotation, scale, visible, opacity)
//   - 4 function members (Reset, IsIdentity, GetWorldPosition, SetPosition)
```

### 2. Automatic Member Type Classification

The system automatically categorizes and analyzes members during generation:

```cpp
// Data members are analyzed for size and type
Member positionMember{
    .Name = "position",
    .Size = sizeof(Vec3),        // e.g., 12 bytes
    .TypeName = "Vec3",          // Extracted via type_name<>
    .Type = Member::Data
};

Member visibleMember{
    .Name = "visible",
    .Size = sizeof(bool),        // e.g., 1 byte
    .TypeName = "bool",
    .Type = Member::Data
};

// Function members analyze return types
Member resetMember{
    .Name = "Reset", 
    .Size = 0,                   // Functions have no storage size
    .TypeName = "void",          // Return type extracted
    .Type = Member::Function
};

Member getPositionMember{
    .Name = "GetWorldPosition",
    .Size = 0,
    .TypeName = "Vec3",          // Return type is Vec3
    .Type = Member::Function
};
```

### 3. Integration with Type Names System

The ClassInfo generation automatically leverages the type names system for accurate type identification:

```cpp
template<class T>
static ClassInfo Of()
{
    static_assert(Types::Described<T>::value, "Type must be 'Described'.");
    
    ClassInfo info;
    using Descr = Types::Description<T>;
    
    // Use type_name<> for class name extraction
    info.Name = Descr::ClassName;  // Automatically extracted from namespace
    info.Size = sizeof(T);
    
    // Process each member defined in DESCRIBED macro
    for (const auto& memberName : Descr::MemberNames)
    {
        const bool isFunction = *Descr::IsFunctionByName(memberName);
        const auto typeName = *Descr::GetTypeNameByName(memberName);
        const auto memberSize = *Descr::GetMemberSizeByName(memberName);
        
        info.Members.push_back(Member{
            .Name = std::string{memberName},
            .Size = memberSize * !isFunction,  // 0 for functions
            .TypeName = std::string{typeName.data(), typeName.size()},
            .Type = isFunction ? Member::Function : Member::Data
        });
    }
    
    return info;
}
```

## Advanced Features

### 1. Runtime Member Inspection

```cpp
void InspectClass(const ClassInfo& info)
{
    std::cout << "=== Class: " << info.Name << " ===\n";
    std::cout << "Total size: " << info.Size << " bytes\n";
    std::cout << "Member count: " << info.Members.size() << "\n\n";
    
    size_t dataMembers = 0;
    size_t functionMembers = 0;
    size_t totalDataSize = 0;
    
    for (const auto& member : info.Members)
    {
        std::cout << (member.Type == Member::Data ? "Data" : "Func") 
                  << " | " << member.TypeName 
                  << " | " << member.Name;
        
        if (member.Type == Member::Data) {
            std::cout << " (" << member.Size << " bytes)";
            dataMembers++;
            totalDataSize += member.Size;
        } else {
            functionMembers++;
        }
        std::cout << "\n";
    }
    
    std::cout << "\nSummary:\n";
    std::cout << "  Data members: " << dataMembers << " (" << totalDataSize << " bytes)\n";
    std::cout << "  Function members: " << functionMembers << "\n";
}
```

### 2. Dynamic Property Editor Generation

```cpp
template<typename T>
class RuntimePropertyEditor
{
    ClassInfo m_classInfo;
    
public:
    RuntimePropertyEditor() : m_classInfo(ClassInfo::Of<T>()) {}
    
    void RenderEditor(T& object)
    {
        ImGui::Begin(fmt::format("Editor: {}", m_classInfo.Name).c_str());
        
        // Display class metadata
        ImGui::Text("Type: %s", m_classInfo.Name.c_str());
        ImGui::Text("Size: %zu bytes", m_classInfo.Size);
        ImGui::Separator();
        
        // Render editors for each data member
        for (const auto& member : m_classInfo.Members)
        {
            if (member.Type == Member::Function) continue; // Skip functions
            
            RenderMemberEditor(object, member);
        }
        
        // Display function information
        ImGui::Separator();
        ImGui::Text("Available Functions:");
        for (const auto& member : m_classInfo.Members)
        {
            if (member.Type == Member::Data) continue; // Skip data
            
            if (ImGui::Button(fmt::format("{}() -> {}", member.Name, member.TypeName).c_str()))
            {
                CallMemberFunction(object, member.Name);
            }
        }
        
        ImGui::End();
    }

private:
    void RenderMemberEditor(T& object, const Member& member)
    {
        using Desc = Description<T>;
        
        if (member.TypeName == "float") {
            auto value = Desc::template GetMemberValueOfType<float>(member.Name, object);
            if (value) {
                float editValue = *value;
                if (ImGui::SliderFloat(member.Name.c_str(), &editValue, 0.0f, 100.0f)) {
                    Desc::SetMemberValueByName(member.Name, object, editValue);
                }
            }
        } else if (member.TypeName == "bool") {
            auto value = Desc::template GetMemberValueOfType<bool>(member.Name, object);
            if (value) {
                bool editValue = *value;
                if (ImGui::Checkbox(member.Name.c_str(), &editValue)) {
                    Desc::SetMemberValueByName(member.Name, object, editValue);
                }
            }
        } else if (member.TypeName == "Vec3") {
            auto value = Desc::template GetMemberValueOfType<Vec3>(member.Name, object);
            if (value) {
                Vec3 editValue = *value;
                if (ImGui::SliderFloat3(member.Name.c_str(), &editValue.x, -10.0f, 10.0f)) {
                    Desc::SetMemberValueByName(member.Name, object, editValue);
                }
            }
        } else {
            // Generic display for unknown types
            ImGui::Text("%s: %s [%zu bytes]", member.Name.c_str(), member.TypeName.c_str(), member.Size);
        }
    }
    
    void CallMemberFunction(T& object, const std::string& functionName)
    {
        // Implementation would use Description<T> to call the function
        LOG_INFO("Calling function: {}", functionName);
    }
};
```

### 3. Serialization Metadata

```cpp
class SerializationRegistry
{
    std::unordered_map<std::string, ClassInfo> m_classInfo;
    
public:
    template<typename T>
    void RegisterClass()
    {
        static_assert(Described<T>::value, "Type must be described");
        
        ClassInfo info = ClassInfo::Of<T>();
        m_classInfo[info.Name] = std::move(info);
    }
    
    std::optional<ClassInfo> GetClassInfo(const std::string& className) const
    {
        auto it = m_classInfo.find(className);
        return it != m_classInfo.end() ? std::make_optional(it->second) : std::nullopt;
    }
    
    nlohmann::json SerializeSchema() const
    {
        nlohmann::json schema;
        
        for (const auto& [className, info] : m_classInfo)
        {
            nlohmann::json classSchema;
            classSchema["name"] = info.Name;
            classSchema["size"] = info.Size;
            
            for (const auto& member : info.Members)
            {
                nlohmann::json memberSchema;
                memberSchema["name"] = member.Name;
                memberSchema["type"] = member.TypeName;
                memberSchema["size"] = member.Size;
                memberSchema["category"] = (member.Type == Member::Data) ? "data" : "function";
                
                classSchema["members"].push_back(memberSchema);
            }
            
            schema["classes"][className] = classSchema;
        }
        
        return schema;
    }
};

// Usage
SerializationRegistry registry;
registry.RegisterClass<Transform>();
registry.RegisterClass<GameObject>();
registry.RegisterClass<Renderer>();

// Export schema for external tools
auto schema = registry.SerializeSchema();
std::ofstream schemaFile("reflection_schema.json");
schemaFile << schema.dump(2);
```

### 4. Debugging and Validation

```cpp
namespace Reflection
{
    // Built-in test validation
    bool ClassInfoTest()
    {
        static const ClassInfo cl = ClassInfo::Of<TestStruct>();
        
        // Expected structure for validation
        static const ClassInfo ExpectedInfo
        {
            .Name = "TestStruct",
            .Size = 24,  // Platform-dependent
            .Members = std::vector<Member>
            {
                {
                    .Name = "i",
                    .Size = 4,
                    .TypeName = "int",
                    .Type = Member::Data
                },
                {
                    .Name = "f",
                    .Size = 4,
                    .TypeName = "float",
                    .Type = Member::Data
                },
                {
                    .Name = "ch",
                    .Size = 1,
                    .TypeName = "char",
                    .Type = Member::Data
                },
                {
                    .Name = "pi",
                    .Size = 8,  // 64-bit pointer
                    .TypeName = "int*",
                    .Type = Member::Data
                },
                {
                    .Name = "vfunc",
                    .Size = 0,
                    .TypeName = "void",
                    .Type = Member::Function
                },
                {
                    .Name = "bfunc",
                    .Size = 0,
                    .TypeName = "bool",
                    .Type = Member::Function
                }
            }
        };
        
        // Validate generated reflection data matches expected
        return cl == ExpectedInfo;
    }
    
    // Runtime validation helper
    template<typename T>
    bool ValidateClassInfo()
    {
        try {
            ClassInfo info = ClassInfo::Of<T>();
            
            // Basic validation checks
            if (info.Name.empty()) return false;
            if (info.Size != sizeof(T)) return false;
            if (info.Members.empty()) return false;
            
            // Validate member consistency
            size_t dataSize = 0;
            for (const auto& member : info.Members) {
                if (member.Name.empty()) return false;
                if (member.TypeName.empty()) return false;
                
                if (member.Type == Member::Data) {
                    if (member.Size == 0) return false; // Data members must have size
                    dataSize += member.Size;
                } else {
                    if (member.Size != 0) return false; // Functions must have zero size
                }
            }
            
            // Note: dataSize may be less than sizeof(T) due to padding/alignment
            
            return true;
        } catch (...) {
            return false;
        }
    }
}
```

## Integration Patterns

### 1. Component System Integration

```cpp
template<typename ComponentType>
class ComponentWrapper
{
    ClassInfo m_info;
    
public:
    ComponentWrapper() : m_info(ClassInfo::Of<ComponentType>()) {}
    
    const ClassInfo& GetInfo() const { return m_info; }
    
    nlohmann::json Serialize(const ComponentType& component) const
    {
        nlohmann::json result;
        result["__type"] = m_info.Name;
        
        for (const auto& member : m_info.Members) {
            if (member.Type == Member::Data) {
                auto value = GetMemberValueAsJson(component, member);
                if (value) {
                    result["data"][member.Name] = *value;
                }
            }
        }
        
        return result;
    }
    
    std::optional<ComponentType> Deserialize(const nlohmann::json& json) const
    {
        if (json["__type"] != m_info.Name) return std::nullopt;
        
        ComponentType component{};
        
        for (const auto& member : m_info.Members) {
            if (member.Type == Member::Data && json["data"].contains(member.Name)) {
                SetMemberValueFromJson(component, member, json["data"][member.Name]);
            }
        }
        
        return component;
    }
};
```

### 2. Asset System Integration

```cpp
class AssetMetadata
{
    std::unordered_map<std::string, ClassInfo> m_assetTypes;
    
public:
    template<typename AssetType>
    void RegisterAssetType()
    {
        ClassInfo info = ClassInfo::Of<AssetType>();
        m_assetTypes[info.Name] = std::move(info);
    }
    
    std::vector<std::string> GetRegisteredAssetTypes() const
    {
        std::vector<std::string> types;
        for (const auto& [typeName, info] : m_assetTypes) {
            types.push_back(typeName);
        }
        return types;
    }
    
    std::optional<size_t> GetAssetSize(const std::string& typeName) const
    {
        auto it = m_assetTypes.find(typeName);
        return it != m_assetTypes.end() ? std::make_optional(it->second.Size) : std::nullopt;
    }
    
    std::vector<Member> GetAssetProperties(const std::string& typeName) const
    {
        auto it = m_assetTypes.find(typeName);
        return it != m_assetTypes.end() ? it->second.Members : std::vector<Member>{};
    }
};
```

### 3. Scripting Bridge

```cpp
class ScriptingBridge
{
public:
    template<typename T>
    void ExposeTypeToScript(lua_State* L)
    {
        ClassInfo info = ClassInfo::Of<T>();
        
        // Create Lua table for the type
        lua_newtable(L);
        
        // Expose data members as properties
        for (const auto& member : info.Members) {
            if (member.Type == Member::Data) {
                ExposeDataMember<T>(L, member);
            } else {
                ExposeFunctionMember<T>(L, member);
            }
        }
        
        // Register the table globally
        lua_setglobal(L, info.Name.c_str());
    }

private:
    template<typename T>
    void ExposeDataMember(lua_State* L, const Member& member)
    {
        // Create getter/setter functions for the member
        std::string getterName = "get_" + member.Name;
        std::string setterName = "set_" + member.Name;
        
        // Register getter
        lua_pushcfunction(L, [](lua_State* L) -> int {
            // Implementation would use Description<T> to get member value
            return 1; // Return count
        });
        lua_setfield(L, -2, getterName.c_str());
        
        // Register setter  
        lua_pushcfunction(L, [](lua_State* L) -> int {
            // Implementation would use Description<T> to set member value
            return 0; // Return count
        });
        lua_setfield(L, -2, setterName.c_str());
    }
};
```

## Best Practices

### 1. Efficient ClassInfo Usage
```cpp
// ✅ Good: Cache ClassInfo for repeated use
template<typename T>
class TypedPropertyEditor
{
    static const ClassInfo& GetCachedInfo()
    {
        static ClassInfo info = ClassInfo::Of<T>();
        return info;
    }
    
public:
    void RenderEditor(T& object)
    {
        const auto& info = GetCachedInfo(); // No repeated generation
        // Use info...
    }
};

// ❌ Avoid: Regenerating ClassInfo repeatedly
template<typename T>
void ProcessType(T& object)
{
    ClassInfo info = ClassInfo::Of<T>(); // Regenerated every call
    // Process...
}
```

### 2. Member Type Handling
```cpp
// ✅ Good: Type-safe member processing
void ProcessMember(const Member& member, auto& object)
{
    if (member.Type == Member::Data) {
        if (member.TypeName == "float") {
            ProcessFloatMember(object, member.Name);
        } else if (member.TypeName == "Vec3") {
            ProcessVec3Member(object, member.Name);
        } else {
            ProcessGenericMember(object, member);
        }
    }
}

// ❌ Avoid: String-based type checking without fallbacks
void ProcessMemberUnsafe(const Member& member, auto& object)
{
    // No type validation or fallback handling
    ProcessFloatMember(object, member.Name); // May fail silently
}
```

### 3. Error Handling
```cpp
// ✅ Good: Robust error handling
template<typename T>
std::optional<ClassInfo> SafeGetClassInfo()
{
    if constexpr (Described<T>::value) {
        try {
            return ClassInfo::Of<T>();
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to generate ClassInfo for {}: {}", type_name<T>(), e.what());
            return std::nullopt;
        }
    } else {
        LOG_WARNING("Type {} is not described", type_name<T>());
        return std::nullopt;
    }
}
```

### 4. Performance Considerations
```cpp
// ✅ Good: Static initialization for frequently used types
template<typename T>
const ClassInfo& GetStaticClassInfo()
{
    static const ClassInfo info = ClassInfo::Of<T>();
    return info;
}

// ✅ Good: Lazy initialization for conditional use
template<typename T>
const ClassInfo& GetLazyClassInfo()
{
    static std::optional<ClassInfo> cached;
    if (!cached) {
        cached = ClassInfo::Of<T>();
    }
    return *cached;
}
```

## Testing and Validation

### Unit Test Framework
```cpp
class ReflectionTests
{
public:
    static bool RunAllTests()
    {
        return TestBasicClassInfo() &&
               TestMemberClassification() &&
               TestTypeNameExtraction() &&
               TestEquality() &&
               TestComplexTypes();
    }

private:
    static bool TestBasicClassInfo()
    {
        struct SimpleStruct {
            int value;
            void Method() {}
        };
        
        DESCRIBED(SimpleStruct, &SimpleStruct::value, &SimpleStruct::Method);
        
        ClassInfo info = ClassInfo::Of<SimpleStruct>();
        
        return info.Name == "SimpleStruct" &&
               info.Size == sizeof(SimpleStruct) &&
               info.Members.size() == 2 &&
               info.Members[0].Type == Member::Data &&
               info.Members[1].Type == Member::Function;
    }
    
    // Additional test methods...
};
```

This comprehensive Type Structs system provides the runtime foundation for dynamic type inspection, property editing, serialization, and debugging capabilities in Scenery Editor X, seamlessly bridging compile-time reflection with runtime accessibility.

### 3. Validation and Testing

The system includes comprehensive testing infrastructure:

```cpp
namespace Reflection
{
    bool ClassInfoTest(); // Validates the reflection system
}
```

## Usage Patterns

### Basic Runtime Inspection

```cpp
template<typename T>
void InspectType()
{
    static_assert(Described<T>::value, "Type must be described");
    
    const ClassInfo info = ClassInfo::Of<T>();
    
    std::cout << "Class: " << info.Name << std::endl;
    std::cout << "Size: " << info.Size << " bytes" << std::endl;
    std::cout << "Members (" << info.Members.size() << "):" << std::endl;
    
    for (const auto& member : info.Members) {
        std::cout << "  ";
        
        if (member.Type == Member::Function) {
            std::cout << member.TypeName << " " << member.Name << "() [FUNCTION]";
        } else {
            std::cout << member.TypeName << " " << member.Name 
                     << " [" << member.Size << " bytes]";
        }
        
        std::cout << std::endl;
    }
}

// Usage
InspectType<Transform>();
/*
Output:
Class: Transform
Size: 36 bytes
Members (5):
  Vec3 position [12 bytes]
  Vec3 rotation [12 bytes]  
  Vec3 scale [12 bytes]
  void Reset() [FUNCTION]
  bool IsIdentity() [FUNCTION]
*/
```

### Serialization Metadata

```cpp
class SerializationMetadata
{
public:
    template<typename T>
    void RegisterType()
    {
        static_assert(Described<T>::value);
        
        const ClassInfo info = ClassInfo::Of<T>();
        
        // Store only data members for serialization
        std::vector<Member> dataMembers;
        std::copy_if(info.Members.begin(), info.Members.end(),
                    std::back_inserter(dataMembers),
                    [](const Member& m) { return m.Type == Member::Data; });
        
        SerializationInfo serInfo{
            .className = info.Name,
            .totalSize = info.Size,
            .dataMembers = std::move(dataMembers),
            .version = 1 // For versioning support
        };
        
        m_SerializationData[info.Name] = std::move(serInfo);
        
        SEDX_CORE_INFO_TAG("SERIALIZATION", 
            "Registered type '{}' with {} data members", 
            info.Name, dataMembers.size());
    }
    
    std::optional<SerializationInfo> GetSerializationInfo(const std::string& typeName) const
    {
        auto it = m_SerializationData.find(typeName);
        return (it != m_SerializationData.end()) ? 
            std::optional(it->second) : std::nullopt;
    }

private:
    struct SerializationInfo
    {
        std::string className;
        size_t totalSize;
        std::vector<Member> dataMembers;
        uint32_t version;
    };
    
    std::unordered_map<std::string, SerializationInfo> m_SerializationData;
};
```

### Dynamic Property System

```cpp
class PropertySystem
{
public:
    template<typename T>
    void RegisterProperties()
    {
        const ClassInfo info = ClassInfo::Of<T>();
        
        PropertyGroup group;
        group.typeName = info.Name;
        
        for (const auto& member : info.Members) {
            if (member.Type == Member::Data) {
                PropertyDescriptor prop{
                    .name = member.Name,
                    .typeName = member.TypeName,
                    .size = member.Size,
                    .isReadOnly = false // Could be enhanced with const detection
                };
                
                group.properties.push_back(std::move(prop));
            }
        }
        
        m_PropertyGroups[info.Name] = std::move(group);
    }
    
    const PropertyGroup* GetProperties(const std::string& typeName) const
    {
        auto it = m_PropertyGroups.find(typeName);
        return (it != m_PropertyGroups.end()) ? &it->second : nullptr;
    }

private:
    struct PropertyDescriptor
    {
        std::string name;
        std::string typeName;
        size_t size;
        bool isReadOnly;
    };
    
    struct PropertyGroup
    {
        std::string typeName;
        std::vector<PropertyDescriptor> properties;
    };
    
    std::unordered_map<std::string, PropertyGroup> m_PropertyGroups;
};
```

### Debug Information Generation

```cpp
class DebugInfoGenerator
{
public:
    template<typename T>
    std::string GenerateDebugInfo()
    {
        const ClassInfo info = ClassInfo::Of<T>();
        
        std::ostringstream oss;
        oss << "=== DEBUG INFO: " << info.Name << " ===" << std::endl;
        oss << "Total Size: " << info.Size << " bytes" << std::endl;
        oss << std::endl;
        
        // Separate data and function members
        std::vector<Member> dataMembers, functionMembers;
        
        for (const auto& member : info.Members) {
            if (member.Type == Member::Data) {
                dataMembers.push_back(member);
            } else {
                functionMembers.push_back(member);
            }
        }
        
        // Data members section
        if (!dataMembers.empty()) {
            oss << "Data Members (" << dataMembers.size() << "):" << std::endl;
            size_t totalDataSize = 0;
            
            for (const auto& member : dataMembers) {
                oss << "  " << member.TypeName << " " << member.Name 
                    << " [" << member.Size << " bytes]" << std::endl;
                totalDataSize += member.Size;
            }
            
            oss << "  Total Data Size: " << totalDataSize << " bytes" << std::endl;
            
            if (totalDataSize < info.Size) {
                oss << "  Padding/Alignment: " << (info.Size - totalDataSize) 
                    << " bytes" << std::endl;
            }
            oss << std::endl;
        }
        
        // Function members section  
        if (!functionMembers.empty()) {
            oss << "Function Members (" << functionMembers.size() << "):" << std::endl;
            
            for (const auto& member : functionMembers) {
                oss << "  " << member.TypeName << " " << member.Name << "()" << std::endl;
            }
            oss << std::endl;
        }
        
        oss << "=== END DEBUG INFO ===" << std::endl;
        
        return oss.str();
    }
};

// Usage
DebugInfoGenerator generator;
std::string debugInfo = generator.GenerateDebugInfo<Transform>();
SEDX_CORE_DEBUG_TAG("REFLECTION", "Debug info:\n{}", debugInfo);
```

### Memory Layout Analysis

```cpp
class MemoryLayoutAnalyzer
{
public:
    template<typename T>
    LayoutAnalysis AnalyzeLayout()
    {
        const ClassInfo info = ClassInfo::Of<T>();
        
        LayoutAnalysis analysis;
        analysis.className = info.Name;
        analysis.totalSize = info.Size;
        
        size_t dataSize = 0;
        size_t largestMember = 0;
        
        for (const auto& member : info.Members) {
            if (member.Type == Member::Data) {
                dataSize += member.Size;
                largestMember = std::max(largestMember, member.Size);
                
                analysis.dataMembers.push_back({
                    .name = member.Name,
                    .type = member.TypeName,
                    .size = member.Size
                });
            }
        }
        
        analysis.totalDataSize = dataSize;
        analysis.paddingBytes = info.Size - dataSize;
        analysis.wastedSpace = static_cast<float>(analysis.paddingBytes) / info.Size * 100.0f;
        analysis.largestMemberSize = largestMember;
        
        // Basic alignment analysis
        if (analysis.paddingBytes > 0) {
            analysis.hasAlignment = true;
            analysis.suggestedAlignment = largestMember; // Simplified heuristic
        }
        
        return analysis;
    }

private:
    struct MemberInfo
    {
        std::string name;
        std::string type;
        size_t size;
    };
    
    struct LayoutAnalysis
    {
        std::string className;
        size_t totalSize = 0;
        size_t totalDataSize = 0;
        size_t paddingBytes = 0;
        size_t largestMemberSize = 0;
        float wastedSpace = 0.0f;
        bool hasAlignment = false;
        size_t suggestedAlignment = 0;
        std::vector<MemberInfo> dataMembers;
    };
};
```

## Advanced Integration Examples

### Editor Integration

```cpp
class TypeBrowser
{
public:
    void RenderUI()
    {
        ImGui::Begin("Type Browser");
        
        // Type selection combo
        if (ImGui::BeginCombo("Select Type", m_SelectedType.c_str())) {
            for (const auto& [typeName, info] : m_RegisteredTypes) {
                if (ImGui::Selectable(typeName.c_str(), typeName == m_SelectedType)) {
                    m_SelectedType = typeName;
                }
            }
            ImGui::EndCombo();
        }
        
        // Display selected type info
        if (!m_SelectedType.empty()) {
            const auto& info = m_RegisteredTypes[m_SelectedType];
            RenderTypeInfo(info);
        }
        
        ImGui::End();
    }
    
    template<typename T>
    void RegisterType()
    {
        const ClassInfo info = ClassInfo::Of<T>();
        m_RegisteredTypes[info.Name] = info;
    }

private:
    void RenderTypeInfo(const ClassInfo& info)
    {
        ImGui::Text("Class: %s", info.Name.c_str());
        ImGui::Text("Size: %zu bytes", info.Size);
        
        ImGui::Separator();
        
        if (ImGui::TreeNode("Members")) {
            for (const auto& member : info.Members) {
                ImGui::PushID(member.Name.c_str());
                
                if (member.Type == Member::Function) {
                    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), 
                        "%s %s() [FUNCTION]", 
                        member.TypeName.c_str(), member.Name.c_str());
                } else {
                    ImGui::Text("%s %s [%zu bytes]", 
                        member.TypeName.c_str(), member.Name.c_str(), member.Size);
                }
                
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
    }
    
    std::unordered_map<std::string, ClassInfo> m_RegisteredTypes;
    std::string m_SelectedType;
};
```

### Configuration System Integration

```cpp
class ConfigurationValidator
{
public:
    template<typename ConfigType>
    ValidationResult ValidateConfig(const nlohmann::json& config)
    {
        const ClassInfo info = ClassInfo::Of<ConfigType>();
        
        ValidationResult result;
        result.typeName = info.Name;
        result.isValid = true;
        
        // Check that all data members have corresponding JSON fields
        for (const auto& member : info.Members) {
            if (member.Type == Member::Data) {
                if (!config.contains(member.Name)) {
                    result.isValid = false;
                    result.errors.push_back(fmt::format(
                        "Missing required field: '{}' of type '{}'", 
                        member.Name, member.TypeName));
                } else {
                    // Type-specific validation could be added here
                    result.validatedFields.push_back(member.Name);
                }
            }
        }
        
        // Check for unknown fields in JSON
        for (auto& [key, value] : config.items()) {
            bool found = false;
            for (const auto& member : info.Members) {
                if (member.Type == Member::Data && member.Name == key) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                result.warnings.push_back(fmt::format(
                    "Unknown field in config: '{}'", key));
            }
        }
        
        return result;
    }

private:
    struct ValidationResult
    {
        std::string typeName;
        bool isValid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        std::vector<std::string> validatedFields;
    };
};
```

### Module System Integration

```cpp
class ReflectionModule : public Module
{
public:
    explicit ReflectionModule() : Module("ReflectionModule") {}
    
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing Reflection Module ===");
        
        // Register core types
        RegisterCoreTypes();
        
        // Validate reflection system
        if (!RunValidationTests()) {
            SEDX_CORE_ERROR("Reflection system validation failed!");
            return;
        }
        
        SEDX_CORE_INFO("Reflection module initialized with {} types", 
            m_TypeRegistry.size());
    }
    
    template<typename T>
    void RegisterType()
    {
        static_assert(Described<T>::value, "Type must be described");
        
        const ClassInfo info = ClassInfo::Of<T>();
        m_TypeRegistry[info.Name] = info;
        
        SEDX_CORE_INFO_TAG("REFLECTION", "Registered type: {}", info.Name);
    }
    
    const ClassInfo* GetTypeInfo(const std::string& typeName) const
    {
        auto it = m_TypeRegistry.find(typeName);
        return (it != m_TypeRegistry.end()) ? &it->second : nullptr;
    }
    
    void OnUIRender() override
    {
        if (m_ShowDebugWindow) {
            RenderDebugUI();
        }
    }

private:
    void RegisterCoreTypes()
    {
        RegisterType<TestStruct>(); // From the example in type_structs.h
        // Register other core types...
    }
    
    bool RunValidationTests()
    {
        return Types::Reflection::ClassInfoTest();
    }
    
    void RenderDebugUI()
    {
        ImGui::Begin("Reflection Debug", &m_ShowDebugWindow);
        
        ImGui::Text("Registered Types: %zu", m_TypeRegistry.size());
        
        if (ImGui::TreeNode("Type Registry")) {
            for (const auto& [name, info] : m_TypeRegistry) {
                if (ImGui::TreeNode(name.c_str())) {
                    ImGui::Text("Size: %zu bytes", info.Size);
                    ImGui::Text("Members: %zu", info.Members.size());
                    
                    if (ImGui::TreeNode("Member Details")) {
                        for (const auto& member : info.Members) {
                            const char* typeStr = (member.Type == Member::Function) ? "FUNC" : "DATA";
                            ImGui::Text("[%s] %s %s (%zu bytes)", 
                                typeStr, member.TypeName.c_str(), 
                                member.Name.c_str(), member.Size);
                        }
                        ImGui::TreePop();
                    }
                    
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }
        
        ImGui::End();
    }
    
    std::unordered_map<std::string, ClassInfo> m_TypeRegistry;
    bool m_ShowDebugWindow = false;
};
```

## Best Practices

### 1. Type Registration Strategy
```cpp
// Register types in module initialization
void MyModule::OnAttach() override
{
    // Register all types used by this module
    m_ReflectionSystem.RegisterType<Transform>();
    m_ReflectionSystem.RegisterType<Mesh>();
    m_ReflectionSystem.RegisterType<Material>();
}
```

### 2. Validation and Error Handling
```cpp
template<typename T>
ClassInfo SafeGetClassInfo()
{
    static_assert(Described<T>::value, "Type must be described for reflection");
    
    try {
        return ClassInfo::Of<T>();
    } catch (const std::exception& e) {
        SEDX_CORE_ERROR_TAG("REFLECTION", 
            "Failed to generate ClassInfo for {}: {}", 
            type_name<T>(), e.what());
        
        // Return minimal info for error recovery
        return ClassInfo{
            .Name = std::string(type_name<T>()),
            .Size = sizeof(T),
            .Members = {}
        };
    }
}
```

### 3. Performance Considerations
```cpp
// Cache ClassInfo for frequently used types
template<typename T>
const ClassInfo& GetCachedClassInfo()
{
    static const ClassInfo info = ClassInfo::Of<T>();
    return info;
}
```

The Type Structs system provides the runtime foundation for reflection, enabling dynamic type inspection, serialization, debugging, and editor integration while maintaining seamless integration with the compile-time type descriptor system.

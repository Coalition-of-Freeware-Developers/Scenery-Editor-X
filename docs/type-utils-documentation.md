# Type Utils System Documentation

## Overview

The Type Utils system (`type_utils.h`) provides essential template metaprogramming utilities and type traits for the Scenery Editor X reflection system. This system includes member pointer introspection, type classification utilities, template specialization detection, and advanced C++20/C++17 compatibility helpers.

This utility library serves as the foundational layer for the entire reflection framework, providing the low-level template metaprogramming tools that enable the higher-level Description, ClassInfo, and type name systems to function effectively.

## Core Architecture

### Member Pointer Utilities

The `member_pointer` namespace provides comprehensive utilities for extracting return types from member pointers, essential for the reflection system's ability to work with both data members and member functions:

```cpp
namespace member_pointer
{
    namespace impl
    {
        // Function member pointer specialization
        template <typename Object, typename Return, typename... Args>
        struct return_type_function<Return(Object::*)(Args...)> { using type = Return; };

        template <typename Object, typename Return, typename... Args>
        struct return_type_function<Return(Object::*)(Args...) const> { using type = Return; };

        // Data member pointer specialization  
        template <typename Object, typename Return>
        struct return_type_object<Return Object::*> { using type = Return; };
    }

    template<typename T>
    struct return_type; // Unified interface for both function and data member pointers
}
```

**Supported Member Pointer Types:**
- **Member Functions**: `Return(Object::*)(Args...)` → `Return`
- **Const Member Functions**: `Return(Object::*)(Args...) const` → `Return`  
- **Member Objects**: `Return Object::*` → `Return`

### Template Specialization Detection

Critical for determining if types have been made reflectable via the `DESCRIBED` macro:

```cpp
template<typename T, typename = void>
struct is_specialized : std::false_type {};

template<typename T>
struct is_specialized<T, std::void_t<decltype(T{})>> : std::true_type {};

// Usage in reflection system
template<typename T>
constexpr bool IsDescribed = is_specialized<Description<T>>::value;
```

### Container Type Detection

Provides unified detection for array-like containers:

```cpp
namespace is_array_impl
{
    template <typename T>                    struct is_array_impl : std::false_type {};
    template <typename T, std::size_t N>     struct is_array_impl<std::array<T, N>> : std::true_type {};
    template <typename... Args>              struct is_array_impl<std::vector<Args...>> : std::true_type {};
}

template<typename T>
struct is_array
{
    static constexpr bool value = is_array_impl::is_array_impl<std::decay_t<T>>::value;
};

template<typename T>
inline constexpr bool is_array_v = is_array<T>::value;
```

### Stream Output Detection

Enables conditional streaming support in the reflection system:

```cpp
template<class T>
class is_streamable
{
    // SFINAE-based detection using decltype and comma operator
    template<class TT>
    static constexpr decltype(std::declval<std::ostream &>() << std::declval<TT>(), std::true_type()) test(int);

    template<class>
    static constexpr std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

template<class T>
inline constexpr bool is_streamable_v = is_streamable<T>::value;
```

### Void Type Filtering

Replaces void types with alternatives, essential for variant types in reflection:

```cpp
struct filter_void_alt {}; // Default alternative for void

template<class T, class Alternative = filter_void_alt>
struct filter_void
{
    using type = std::conditional_t<std::is_void_v<T>, Alternative, T>;
};

template<class T>
using filter_void_t = filter_void<T>::type;

// Usage in reflection variant generation
using MemberVariant = std::variant<filter_void_t<ReturnType1>, filter_void_t<ReturnType2>, /*...*/>;
```

## Advanced Template Utilities

### Cross-Platform Tuple Iteration

Provides C++20/C++17 compatible tuple iteration with lambda forwarding:

```cpp
#if __cplusplus >= 202002L
// C++20 version with template lambda parameters
template <typename TupleT, typename Fn>
constexpr void for_each_tuple(TupleT&& tp, Fn&& fn)
{
    std::apply(
        [&fn]<typename ...T>(T&& ...args)
        {
            (fn(std::forward<T>(args)), ...); // C++17 fold expression
        },
        std::forward<TupleT>(tp)
    );
}
#else
// C++17 compatible version with auto parameters
template <typename TupleT, typename Fn>
constexpr void for_each_tuple(TupleT&& tp, Fn&& fn)
{
    std::apply(
        [&fn](auto&& ...args)
        {
            (fn(std::forward<decltype(args)>(args)), ...);
        },
        std::forward<TupleT>(tp)
    );
}
#endif

// Usage example
auto tuple = std::make_tuple(1, 2.5f, "hello", Vec3{});
for_each_tuple(tuple, [](const auto& element) {
    if constexpr (is_streamable_v<std::decay_t<decltype(element)>>) {
        std::cout << element << " ";
    } else {
        std::cout << "[non-streamable] ";
    }
});
```

### Variadic Parameter Access

Safe access to specific elements in variadic parameter packs:

```cpp
#if __cplusplus >= 202002L
// C++20 version with template lambdas
template<auto N>
static constexpr auto unroll = [](auto expr)
{
    [expr] <auto...Is>(std::index_sequence<Is...>)
    {
        ((expr.template operator()<Is>()), ...);
    }(std::make_index_sequence<N>());
};

template<auto N, typename...Args>
constexpr auto nth_element(Args... args)
{
    return[&]<size_t...Ns>(std::index_sequence<Ns...>)
    {
        return ((Ns == N) ? args : ...);
    }(std::make_index_sequence<sizeof...(Args)>());
}

template<auto N>
constexpr auto nth_element_l = [](auto... args)
{
    return[&]<std::size_t... Ns>(std::index_sequence<Ns...>)
    {
        return ((Ns == N) ? args : ...);
    }(std::make_index_sequence<sizeof...(args)>{});
};

#else
// C++17 compatible version
namespace impl
{
    template<size_t...Ns, typename ...Args>
    constexpr auto nth_element_unroll(std::index_sequence<Ns...>, Args...args)
    {
        // Implementation uses recursive template instantiation
        return std::get<sizeof...(Ns)>(std::forward_as_tuple(args...));
    }
}

template<auto N, typename...Args>
constexpr auto nth_element(Args... args)
{
    return impl::nth_element_unroll(std::make_index_sequence<N>(), args...);
}
#endif

// Usage examples
constexpr auto first = nth_element<0>(10, 20.5f, "hello"); // returns 10
constexpr auto second = nth_element<1>(10, 20.5f, "hello"); // returns 20.5f
constexpr auto third = nth_element<2>(10, 20.5f, "hello"); // returns "hello"
```

## Integration with Reflection System

### Member Pointer Type Extraction

The reflection system relies heavily on these utilities:

```cpp
template<auto... MemberPointers>
struct MemberList
{
    // Use return_type to extract member types
    template<size_t Index>
    using TMemberType = typename member_pointer::return_type<
        std::remove_cvref_t<decltype(std::get<Index>(std::tuple(MemberPointers...)))>
    >::type;
    
    // Create variant type excluding void (for function return types)
    using TVariant = std::variant<filter_void_t<TMemberType<Indices>>...>;
};
```

### Type Classification in Descriptions

```cpp
template<typename T>
void ProcessMemberValue(const T& value, std::string_view memberName)
{
    if constexpr (is_array_v<T>) {
        ProcessArrayMember(value, memberName);
    } else if constexpr (is_streamable_v<T>) {
        ProcessStreamableMember(value, memberName);
    } else {
        ProcessGenericMember(value, memberName);
    }
}
```

### Template Specialization Checking

```cpp
template<typename T>
void EnsureDescribed()
{
    static_assert(is_specialized<Description<T>>::value, 
                  "Type must be described using DESCRIBED macro");
}
```

## Practical Usage Patterns

### Type-Safe Member Processing

```cpp
template<typename Object, auto MemberPtr>
void ProcessMember(Object& obj)
{
    using MemberType = typename member_pointer::return_type<decltype(MemberPtr)>::type;
    
    if constexpr (std::is_member_object_pointer_v<decltype(MemberPtr)>) {
        // Data member processing
        auto& memberValue = obj.*MemberPtr;
        
        if constexpr (is_array_v<MemberType>) {
            ProcessArrayData(memberValue);
        } else if constexpr (is_streamable_v<MemberType>) {
            ProcessStreamableData(memberValue);
        }
    } else {
        // Function member processing
        if constexpr (std::is_void_v<MemberType>) {
            (obj.*MemberPtr)(); // Void function call
        } else {
            auto result = (obj.*MemberPtr)(); // Function with return value
            ProcessResult(result);
        }
    }
}
```

### Conditional Template Instantiation

```cpp
template<typename T>
class PropertyEditor
{
public:
    void Render(T& object)
    {
        if constexpr (is_specialized<Description<T>>::value) {
            RenderReflectedProperties(object);
        } else {
            RenderFallbackEditor(object);
        }
    }

private:
    void RenderReflectedProperties(T& object)
    {
        using Desc = Description<T>;
        for_each_tuple(Desc::MemberNames, [&](auto memberName) {
            RenderProperty(object, memberName);
        });
    }
    
    void RenderFallbackEditor(T& object)
    {
        ImGui::Text("Type '%s' is not reflectable", type_name<T>().data());
        if constexpr (is_streamable_v<T>) {
            std::stringstream ss;
            ss << object;
            ImGui::Text("Value: %s", ss.str().c_str());
        }
    }
};
```

### Advanced Type Filtering

```cpp
template<typename... Types>
struct TypeFilter
{
    // Filter to only streamable types
    using StreamableTypes = std::tuple<
        std::conditional_t<is_streamable_v<Types>, Types, void>...
    >;
    
    // Filter to only array types
    using ArrayTypes = std::tuple<
        std::conditional_t<is_array_v<Types>, Types, void>...
    >;
    
    // Create variant from valid types (excluding void)
    using ValidVariant = std::variant<filter_void_t<Types>...>;
};

// Usage with reflection
template<typename T>
void ProcessAllMembers(T& object)
{
    using Desc = Description<T>;
    using FilteredTypes = TypeFilter<
        typename member_pointer::return_type<decltype(Desc::MemberPointers)>::type...
    >;
    
    typename FilteredTypes::ValidVariant memberValue;
    // Process each member...
}
```

### Performance Optimization Utilities

```cpp
// Compile-time loop unrolling
template<size_t N, typename Func>
constexpr void unroll_loop(Func&& func)
{
    if constexpr (__cplusplus >= 202002L) {
        unroll<N>([&func]<auto I>() { func.template operator()<I>(); });
    } else {
        // C++17 fallback with recursive template instantiation
        if constexpr (N > 0) {
            func.template operator()<N-1>();
            unroll_loop<N-1>(func);
        }
    }
}

// Usage for optimized member iteration
template<typename T>
void ProcessAllMembersOptimized(T& object)
{
    using Desc = Description<T>;
    unroll_loop<Desc::NumberOfMembers>([&object]<auto Index>() {
        auto& member = Desc::template GetMemberValue<Index>(object);
        ProcessSingleMember(member);
    });
}
```

## Best Practices

### 1. Prefer Compile-Time Evaluation
```cpp
// ✅ Good: Compile-time type checking
template<typename T>
void ProcessValue(const T& value)
{
    if constexpr (is_streamable_v<T>) {
        // Compile-time branch - dead code elimination
        LogStreamableValue(value);
    } else {
        LogNonStreamableValue<T>();
    }
}

// ❌ Avoid: Runtime type checking when compile-time is available
template<typename T>
void ProcessValueRuntime(const T& value)
{
    bool isStreamable = is_streamable_v<T>; // Runtime check unnecessary
    if (isStreamable) {
        LogStreamableValue(value);
    }
}
```

### 2. Use Type Traits for Template Constraints
```cpp
// ✅ Good: Enable template only for valid types
template<typename T>
requires is_specialized<Description<T>>::value
void ProcessDescribedType(T& object)
{
    // Implementation guaranteed to work with described types
}

// ✅ Good: SFINAE-based enabling for older standards
template<typename T, typename = std::enable_if_t<is_array_v<T>>>
void ProcessArrayType(const T& container)
{
    // Implementation for array-like containers
}
```

### 3. Combine Utilities for Complex Operations
```cpp
// ✅ Good: Combine multiple utilities for robust type handling
template<typename T>
auto GetDisplayValue(const T& value) -> std::string
{
    if constexpr (is_streamable_v<T>) {
        std::stringstream ss;
        ss << value;
        return ss.str();
    } else if constexpr (is_array_v<T>) {
        return fmt::format("Array[{}]", value.size());
    } else if constexpr (is_specialized<Description<T>>::value) {
        return fmt::format("{}{{...}}", type_name<T>());
    } else {
        return fmt::format("[{}]", type_name<T>());
    }
}
```

### 4. Handle Edge Cases Gracefully
```cpp
// ✅ Good: Robust handling of void and other special types
template<typename T>
using safe_return_type = std::conditional_t<
    std::is_void_v<T>,
    void_placeholder,  // Custom placeholder type
    T
>;

template<auto MemberPtr>
using member_return_type = safe_return_type<
    typename member_pointer::return_type<decltype(MemberPtr)>::type
>;
```

## Common Use Cases

### 1. Generic Serialization Framework
```cpp
template<typename T>
class GenericSerializer
{
public:
    nlohmann::json Serialize(const T& object)
    {
        nlohmann::json result;
        
        if constexpr (is_specialized<Description<T>>::value) {
            SerializeReflected(object, result);
        } else if constexpr (is_streamable_v<T>) {
            SerializeStreamable(object, result);
        } else if constexpr (is_array_v<T>) {
            SerializeArray(object, result);
        } else {
            result["__error"] = "Type not serializable";
        }
        
        return result;
    }

private:
    void SerializeReflected(const T& object, nlohmann::json& json)
    {
        using Desc = Description<T>;
        for_each_tuple(Desc::MemberNames, [&](auto memberName) {
            auto value = Desc::GetMemberValueByName(memberName, object);
            if (value) {
                json[std::string{memberName}] = SerializeValue(*value);
            }
        });
    }
    
    // Additional serialization methods...
};
```

### 2. Property Inspector System
```cpp
template<typename T>
class PropertyInspector
{
public:
    void Inspect(T& object, std::string_view objectName)
    {
        ImGui::Begin(fmt::format("Inspector: {}", objectName).c_str());
        
        DisplayTypeInfo<T>();
        ImGui::Separator();
        
        if constexpr (is_specialized<Description<T>>::value) {
            InspectReflectedMembers(object);
        } else {
            InspectFallback(object);
        }
        
        ImGui::End();
    }

private:
    template<typename U>
    void DisplayTypeInfo()
    {
        ImGui::Text("Type: %s", type_name<U>().data());
        ImGui::Text("Size: %zu bytes", sizeof(U));
        ImGui::Text("Streamable: %s", is_streamable_v<U> ? "Yes" : "No");
        ImGui::Text("Array-like: %s", is_array_v<U> ? "Yes" : "No");
        ImGui::Text("Described: %s", is_specialized<Description<U>>::value ? "Yes" : "No");
    }
    
    void InspectReflectedMembers(T& object)
    {
        using Desc = Description<T>;
        for (size_t i = 0; i < Desc::NumberOfMembers; ++i) {
            auto memberName = Desc::MemberNames[i];
            InspectMember(object, memberName, i);
        }
    }
    
    // Additional inspection methods...
};
```

### 3. Type-Safe Configuration System
```cpp
template<typename T>
class TypedConfiguration
{
    static_assert(is_specialized<Description<T>>::value, 
                  "Configuration types must be described");

public:
    void Save(const T& config, std::string_view filename)
    {
        nlohmann::json json;
        json["__type"] = type_name_keep_namespace<T>();
        json["__version"] = GetTypeVersion<T>();
        
        using Desc = Description<T>;
        for_each_tuple(Desc::MemberNames, [&](auto memberName) {
            auto value = Desc::GetMemberValueByName(memberName, config);
            if (value && IsSerializableMember<decltype(*value)>()) {
                json["data"][std::string{memberName}] = *value;
            }
        });
        
        std::ofstream file{std::string{filename}};
        file << json.dump(2);
    }
    
    std::optional<T> Load(std::string_view filename)
    {
        std::ifstream file{std::string{filename}};
        if (!file.is_open()) return std::nullopt;
        
        nlohmann::json json;
        file >> json;
        
        if (!ValidateConfigVersion(json)) return std::nullopt;
        
        T config{};
        LoadConfigData(config, json["data"]);
        return config;
    }

private:
    template<typename U>
    constexpr bool IsSerializableMember()
    {
        return is_streamable_v<U> || is_array_v<U> || is_specialized<Description<U>>::value;
    }
    
    // Additional configuration methods...
};
```

## Debugging and Troubleshooting

### Template Instantiation Issues
```cpp
// Debug macro to examine template instantiations
#define DEBUG_TYPE_TRAITS(T) \
    static_assert(false, \
        "Type: " #T "\n" \
        "Streamable: " + std::to_string(is_streamable_v<T>) + "\n" \
        "Array: " + std::to_string(is_array_v<T>) + "\n" \
        "Described: " + std::to_string(is_specialized<Description<T>>::value))

// Diagnostic function for runtime debugging
template<typename T>
void DiagnoseType()
{
    std::cout << "=== Type Diagnostics for " << type_name<T>() << " ===\n";
    std::cout << "Size: " << sizeof(T) << " bytes\n";
    std::cout << "Alignment: " << alignof(T) << " bytes\n";
    std::cout << "Trivial: " << std::is_trivial_v<T> << "\n";
    std::cout << "Streamable: " << is_streamable_v<T> << "\n";
    std::cout << "Array-like: " << is_array_v<T> << "\n";
    std::cout << "Described: " << is_specialized<Description<T>>::value << "\n";
    
    if constexpr (is_specialized<Description<T>>::value) {
        std::cout << "Member count: " << Description<T>::NumberOfMembers << "\n";
    }
}
```

### Common Compilation Errors
```cpp
// Issue: Member pointer type deduction failure
// template<auto MemberPtr>
// void Process() {
//     using Type = typename member_pointer::return_type<MemberPtr>::type; // Error
// }

// Solution: Proper type handling
template<auto MemberPtr>
void Process() {
    using Type = typename member_pointer::return_type<std::decay_t<decltype(MemberPtr)>>::type;
    // Now works correctly
}

// Issue: SFINAE failure with complex types
// template<typename T, typename = std::enable_if_t<is_streamable_v<T>>>
// void ProcessComplex(const T& value); // May fail with incomplete types

// Solution: Deferred evaluation
template<typename T>
void ProcessComplex(const T& value) {
    if constexpr (is_streamable_v<T>) {
        // Safe evaluation within if constexpr
        ProcessStreamable(value);
    }
}
```

This comprehensive type utilities system provides the essential building blocks for the entire Scenery Editor X reflection framework, enabling sophisticated compile-time type manipulation while maintaining excellent performance and broad compiler compatibility.
auto third = nth_element<2>(10, 20, 30, 40); // Returns 30
```

### Compile-Time Loop Unrolling

```cpp
#if __cplusplus >= 202002L
template<auto N>
static constexpr auto unroll = [](auto expr) {
    // Efficiently unroll expression N times
};
#endif
```

## Detailed API Reference

### Member Pointer Return Type Extraction

```cpp
class MyClass
{
public:
    int value = 42;
    float GetValue() const { return 42.0f; }
    void SetValue(int v) { value = v; }
};

// Extract return types
using IntType = member_pointer::return_type<decltype(&MyClass::value)>::type;          // int
using FloatType = member_pointer::return_type<decltype(&MyClass::GetValue)>::type;    // float  
using VoidType = member_pointer::return_type<decltype(&MyClass::SetValue)>::type;     // void

static_assert(std::is_same_v<IntType, int>);
static_assert(std::is_same_v<FloatType, float>);
static_assert(std::is_same_v<VoidType, void>);
```

### Template Specialization Detection

```cpp
template<typename T>
struct MyTemplate; // Forward declaration only

template<>
struct MyTemplate<int> { int value = 0; }; // Specialization

// Detection
static_assert(is_specialized<MyTemplate<int>>::value);     // true
static_assert(!is_specialized<MyTemplate<float>>::value);  // false
```

### Container Type Classification

```cpp
// Array-like types
static_assert(is_array_v<std::vector<int>>);      // true
static_assert(is_array_v<std::array<float, 4>>);  // true
static_assert(!is_array_v<int>);                  // false
static_assert(!is_array_v<std::string>);          // false

// Usage in template constraints
template<typename T>
void ProcessContainer(const T& container)
{
    static_assert(is_array_v<T>, "Function requires array-like container");
    
    // Process container elements...
}
```

### Stream Output Detection

```cpp
struct Streamable
{
    int value;
    friend std::ostream& operator<<(std::ostream& os, const Streamable& s) {
        return os << s.value;
    }
};

struct NonStreamable
{
    int value;
    // No operator<< defined
};

// Detection
static_assert(is_streamable_v<Streamable>);      // true
static_assert(is_streamable_v<int>);             // true (built-in)
static_assert(is_streamable_v<std::string>);     // true (standard library)
static_assert(!is_streamable_v<NonStreamable>);  // false

// Conditional streaming
template<typename T>
void SafePrint(const T& value)
{
    if constexpr (is_streamable_v<T>) {
        std::cout << value << std::endl;
    } else {
        std::cout << "[Non-streamable type: " << type_name<T>() << "]" << std::endl;
    }
}
```

### Void Type Handling

```cpp
// Filter void types for variant storage
using SafeType1 = filter_void_t<int>;           // int
using SafeType2 = filter_void_t<void>;          // filter_void_alt (placeholder)
using SafeType3 = filter_void_t<void, double>;  // double (custom alternative)

// Usage in template metaprogramming
template<typename... Types>
using VariantType = std::variant<filter_void_t<Types>...>;

// Handles void types gracefully in variant storage
using MyVariant = VariantType<int, void, float>; // std::variant<int, filter_void_alt, float>
```

## Usage Patterns

### Reflection System Integration

```cpp
template<typename T>
class TypeIntrospector
{
public:
    static constexpr bool IsDescribed = is_specialized<Description<T>>::value;
    static constexpr bool IsStreamable = is_streamable_v<T>;
    static constexpr bool IsContainer = is_array_v<T>;
    
    template<auto MemberPtr>
    using MemberReturnType = typename member_pointer::return_type<decltype(MemberPtr)>::type;
    
    static void PrintTypeInfo()
    {
        std::cout << "Type: " << type_name<T>() << std::endl;
        std::cout << "  Described: " << (IsDescribed ? "Yes" : "No") << std::endl;
        std::cout << "  Streamable: " << (IsStreamable ? "Yes" : "No") << std::endl;
        std::cout << "  Container: " << (IsContainer ? "Yes" : "No") << std::endl;
    }
};

// Usage
TypeIntrospector<Transform>::PrintTypeInfo();
```

### Safe Member Access Utilities

```cpp
template<typename T, auto MemberPtr>
class SafeMemberAccessor
{
private:
    using MemberType = typename member_pointer::return_type<decltype(MemberPtr)>::type;
    static constexpr bool IsFunction = std::is_member_function_pointer_v<decltype(MemberPtr)>;
    
public:
    template<typename ObjectType>
    static auto GetValue(const ObjectType& obj) -> std::enable_if_t<!IsFunction, MemberType>
    {
        return obj.*MemberPtr;
    }
    
    template<typename ObjectType>
    static auto CallFunction(ObjectType& obj) -> std::enable_if_t<IsFunction, MemberType>
    {
        return (obj.*MemberPtr)();
    }
    
    static constexpr bool CanGetValue() { return !IsFunction; }
    static constexpr bool CanCall() { return IsFunction; }
    static constexpr size_t GetSize() { return IsFunction ? 0 : sizeof(MemberType); }
};

// Usage
class TestClass
{
public:
    int value = 42;
    int GetValue() const { return value; }
};

using ValueAccessor = SafeMemberAccessor<TestClass, &TestClass::value>;
using FunctionAccessor = SafeMemberAccessor<TestClass, &TestClass::GetValue>;

TestClass obj;
int val1 = ValueAccessor::GetValue(obj);        // Direct member access
int val2 = FunctionAccessor::CallFunction(obj); // Function call
```

### Generic Container Processing

```cpp
template<typename T>
void ProcessIfContainer(const T& item)
{
    if constexpr (is_array_v<T>) {
        std::cout << "Processing container with " << item.size() << " elements:" << std::endl;
        
        for (const auto& element : item) {
            if constexpr (is_streamable_v<std::decay_t<decltype(element)>>) {
                std::cout << "  " << element << std::endl;
            } else {
                std::cout << "  [Non-streamable element]" << std::endl;
            }
        }
    } else {
        std::cout << "Processing single item: ";
        if constexpr (is_streamable_v<T>) {
            std::cout << item << std::endl;
        } else {
            std::cout << "[Non-streamable]" << std::endl;
        }
    }
}

// Usage
std::vector<int> numbers = {1, 2, 3, 4, 5};
ProcessIfContainer(numbers);    // Container processing

int singleValue = 42;
ProcessIfContainer(singleValue); // Single item processing
```

### Template Metaprogramming Helpers

```cpp
template<typename... Types>
class TypeList
{
public:
    static constexpr size_t Count = sizeof...(Types);
    
    template<size_t Index>
    using TypeAt = std::decay_t<decltype(nth_element<Index>(std::declval<Types>()...))>;
    
    template<template<typename> class Predicate>
    static constexpr size_t CountIf()
    {
        return (static_cast<size_t>(Predicate<Types>::value) + ...);
    }
    
    template<typename Visitor>
    static void VisitTypes(Visitor&& visitor)
    {
        (visitor.template operator()<Types>(), ...);
    }
    
    // Get all streamable types
    static constexpr size_t StreamableCount = CountIf<is_streamable>();
    
    // Get all container types
    static constexpr size_t ContainerCount = CountIf<is_array>();
};

// Usage
using MyTypes = TypeList<int, std::vector<float>, std::string, Transform>;

static_assert(MyTypes::Count == 4);
static_assert(MyTypes::StreamableCount >= 3); // int, string, and potentially others
static_assert(MyTypes::ContainerCount >= 2);  // vector and string

using SecondType = MyTypes::TypeAt<1>; // std::vector<float>
```

### C++20/C++17 Compatibility Patterns

```cpp
// Cross-platform tuple processing
template<typename TupleType, typename Processor>
void ProcessTupleElements(const TupleType& tuple, Processor&& processor)
{
    for_each_tuple(tuple, [&processor](const auto& element) {
        using ElementType = std::decay_t<decltype(element)>;
        
        if constexpr (is_streamable_v<ElementType>) {
            processor.ProcessStreamable(element);
        } else {
            processor.ProcessNonStreamable(element);
        }
    });
}

// Usage with lambda processor
auto tuple = std::make_tuple(42, 3.14f, std::string("hello"), Transform{});

struct TupleProcessor
{
    template<typename T>
    void ProcessStreamable(const T& value) {
        std::cout << "Streamable: " << value << std::endl;
    }
    
    template<typename T>
    void ProcessNonStreamable(const T& value) {
        std::cout << "Non-streamable: " << type_name<T>() << std::endl;
    }
};

TupleProcessor processor;
ProcessTupleElements(tuple, processor);
```

## Integration with Core Systems

### Logging System Integration

```cpp
template<typename T>
void LogTypeInformation()
{
    SEDX_CORE_INFO_TAG("TYPE_UTILS", "Type Analysis: {}", type_name<T>());
    SEDX_CORE_INFO_TAG("TYPE_UTILS", "  Size: {} bytes", sizeof(T));
    SEDX_CORE_INFO_TAG("TYPE_UTILS", "  Described: {}", is_specialized<Description<T>>::value);
    SEDX_CORE_INFO_TAG("TYPE_UTILS", "  Streamable: {}", is_streamable_v<T>);
    SEDX_CORE_INFO_TAG("TYPE_UTILS", "  Container: {}", is_array_v<T>);
}
```

### Memory System Integration

```cpp
template<typename T>
Ref<T> CreateSafeObject()
{
    static_assert(!std::is_void_v<T>, "Cannot create void objects");
    
    auto obj = CreateRef<T>();
    
    if constexpr (is_specialized<Description<T>>::value) {
        SEDX_CORE_INFO_TAG("MEMORY", "Created described object: {}", type_name<T>());
        // Could add automatic registration to reflection system
    }
    
    return obj;
}
```

### Module System Integration

```cpp
class TypeUtilsModule : public Module
{
public:
    explicit TypeUtilsModule() : Module("TypeUtilsModule") {}
    
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing Type Utils Module ===");
        
        // Validate utilities with common types
        ValidateUtilities();
        
        SEDX_CORE_INFO("Type Utils module initialized successfully");
    }

private:
    void ValidateUtilities()
    {
        // Test member pointer utilities
        static_assert(std::is_same_v<
            member_pointer::return_type<decltype(&std::string::size)>::type,
            size_t
        >);
        
        // Test type detection
        static_assert(is_streamable_v<int>);
        static_assert(is_array_v<std::vector<int>>);
        static_assert(!is_array_v<int>);
        
        SEDX_CORE_INFO_TAG("TYPE_UTILS", "All utility validations passed");
    }
};
```

## Performance Considerations

### Compile-Time Optimization
- All type traits are evaluated at compile time
- Template specializations are cached by the compiler
- No runtime overhead for type classification

### Template Instantiation
- Use `static_assert` to catch issues early
- Prefer `constexpr` over runtime checks where possible
- Cache expensive template computations in type aliases

### Memory Usage
- Type traits have zero runtime memory footprint
- Template specializations may increase binary size
- Use explicit instantiation for frequently used patterns

## Best Practices

### 1. Type Safety
```cpp
template<typename T>
void ProcessMember(T& obj, auto memberPtr)
{
    using MemberType = typename member_pointer::return_type<decltype(memberPtr)>::type;
    
    static_assert(!std::is_void_v<MemberType> || 
                  std::is_member_function_pointer_v<decltype(memberPtr)>,
                  "Non-function members cannot have void type");
    
    // Safe processing...
}
```

### 2. Error Messages
```cpp
template<typename T>
void RequireStreamable()
{
    static_assert(is_streamable_v<T>, 
        "Type must support operator<< for this operation. "
        "Consider implementing operator<< or using a different approach.");
}
```

### 3. Conditional Compilation
```cpp
template<typename T>
void OptionallyPrint(const T& value)
{
    if constexpr (is_streamable_v<T>) {
        std::cout << value;
    } else {
        std::cout << "[" << type_name<T>() << " object]";
    }
}
```

The Type Utils system provides the foundational metaprogramming infrastructure that enables the sophisticated reflection and type manipulation capabilities throughout Scenery Editor X, ensuring type safety, performance, and cross-platform compatibility.

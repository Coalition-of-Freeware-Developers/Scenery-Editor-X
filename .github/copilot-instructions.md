---
name: 'Scenery Editor X Development Guidelines'
description: 'Guidelines and best practices for contributing to the Scenery Editor X codebase, including architectural patterns, C++ features to leverage, and coding standards.'
applyTo: "**/*.cpp, **/*.h, **/*.hpp, **/*.cc, **/*.cxx, **/*.c"
---

# GitHub Copilot Instructions for Scenery Editor X Application

This repository contains the C++ source code for Scenery Editor X, a sophisticated 3D scenery editor with a modular architecture for exporting and importing 3D models and related data for X-Plane 12 airports.

## Project Overview & Architecture

Scenery Editor X is a modern C++20/C++23 application built with:

- **Vulkan Graphics API:** High-performance 3D rendering with PBR materials and multiple render passes
- **Module System:** Component-based architecture with lifecycle management and proper dependency injection
- **Custom Memory Management:** Smart pointer system with reference counting and automatic cleanup
- **Comprehensive Logging:** Tagged logging with spdlog backend and runtime debug capabilities
- **Reflection System:** Compile-time introspection using DESCRIBED macro for serialization and runtime inspection
- **Configuration Management:** libconfig-based settings with JSON serialization support
- **Custom Math Library:** Has a custom math shared library called "xMath" with `XMATH_API` macro to define `.dll` symbol import and export operations
- **Testing Framework:** Catch2 for unit testing with CTest integration

## Essential Development Guidelines

### Language & Standards

- **Language:** C++20/C++23 with modern features and template metaprogramming
- **Platform Support:** Windows (primary), macOS, and Linux with Vulkan 1.4, and KosmicKrisp.
- **Build System:** CMake 3.8+ with vcpkg for some dependency management
- **Code Quality:** Readable, maintainable, and well-structured following DRY principles

### Core Architectural Patterns

- **Always use the Layer System** for all major application components
- **Custom Smart Pointers:** Use `Ref<T>`, `WeakRef<T>`, and `CreateRef<T>()` instead of std smart pointers
- **Tagged Logging:** Use `SEDX_CORE_*_TAG()` macros for categorized logging
- **Assertion-Driven Development:** Use `SEDX_CORE_ASSERT()` and `SEDX_VERIFY()` for validation
- **Reflection-Based Serialization:** Use `DESCRIBED()` macro for automatic serialization support

## Project Guidelines

- For slang shaders there are specific defined entry point names for each type.
	- Vertex shaders must use `main_vs` as the entry point.
	- Fragment shaders must use `main_frag` as the entry point.
	- Compute shaders must use `main_comp` as the entry point.
	- Tessellation control shaders must use `main_tesc` as the entry point.
	- Tessellation evaluation shaders must use `main_tese` as the entry point.
- Scenery Editor X does not require or need any Raytracing capabilities or support.
- For `grid.slang` pipeline wiring, use shader entry points `main_vs` (vertex) and `main_frag` (fragment) as declared in the shader source.
- Prefer avoiding macro-based implementations for component type mapping; use explicit C++ constructs (traits/specializations/functions) instead.
- For this repository, UI/editor changes should use the existing SceneryEditorX::Renderer framework and application framework (Editor, Event, UILayer, Viewport) rather than introducing a separate rendering stack.
- xMath library `Matrix` uses row-major order by default.  This means that the memory layout of the matrix is such that the elements of each row are stored contiguously in memory.
- When using xMath `Matrix` with Vulkan, which typically uses column-major order, you need to transpose the matrix before passing it to Vulkan shaders to ensure correct interpretation of the data.
- If the macro/preprocessor definition of `XMATH_COORD_DEFINED` is defined, then the xMath library will use a left-handed coordinate system.  If it is not defined, then the xMath library will use a right-handed coordinate system.  This allows for flexibility in choosing the coordinate system that best fits the needs of the application or engine being developed.
- When using xMath with Vulkan, you should ensure that the coordinate system used by xMath matches the coordinate system expected by Vulkan.  Vulkan uses a right-handed coordinate system by default, so if you want to use a left-handed coordinate system with xMath, you should define `XMATH_COORD_DEFINED` to ensure compatibility with Vulkan's expectations.
- When using xMath with Vulkan, you must declare `MATRIX_ROW_MAJOR` or `MATRIX_COLUMN_MAJOR` before including the xMath headers to specify the memory layout of matrices.  By default, xMath uses row-major order, but Vulkan typically expects column-major order.  Declaring the appropriate macro will ensure that the matrix data is correctly interpreted when passed to Vulkan shaders.
- Ensure that xMath `MATRIX_COLUMN_MAJOR` is declared.

## C++ Best Practices & Guidelines
- Use `constexpr` for compile-time constants and functions where possible
- Prefer `inline` variables for header-only constants
- Use `[[nodiscard]]` to indicate functions whose return values should not be ignored
- Utilize `std::optional`, `std::variant`, and `std::expected` for safer error handling and return types

### C++ Features to Leverage
- Use `std::ranges` for more expressive and efficient range-based operations
- Use `std::optional` for functions that may fail or return no value, instead of raw pointers or special return values
- Use `std::variant` for functions that can return multiple types of results, instead of using inheritance or unions
- Use `std::expected` for functions that can return either a value or an error, providing a more expressive way to handle errors without exceptions
- Use `std::optional` for function parameters that are not required, instead of using default values or overloading
- Use RVO (Return Value Optimization) and NRVO (Named Return Value Optimization) to avoid unnecessary copies when returning objects from functions
	- RVO (Return Value Optimization): Usually applies to unnamed, temporary objects, such as return MyObject();. Since C++17, RVO is guaranteed by the standard, making it mandatory.
	- NRVO (Named Return Value Optimization): Applies to named local variables, such as MyObject obj; return obj;. Unlike RVO, NRVO is not guaranteed by the standard, but it is implemented by most modern compilers (GCC, Clang, MSVC) when optimizations are enabled.
	- Functionality: Both optimizations prevent unnecessary calls to copy or move constructors, reducing memory usage and CPU overhead.
	- Limitations: NRVO may not apply if the function has complex control flows (e.g., returning different named objects in if-else branches).
	- Verification: Tools like cargo-show-asm (for Rust, similar to C++), or looking at disassembly with objdump, can confirm if these optimizations are applied.
- Use Structured Bindings for unpacking tuples and pairs, improving readability when working with multiple return values or structured data
- Use Designated Initializers for aggregate types to improve readability and maintainability when initializing complex structures
- Use `std::string_view` for read-only string parameters to avoid unnecessary string copying and improve performance
- Use Ref qualified function parameters (e.g., `const Ref<T>&`) to avoid unnecessary reference counting overhead when passing smart pointers
- Use `Scope<T>` for exclusive ownership semantics when only one owner is needed, instead of `Ref<T>`
- Use RAII (Resource Acquisition Is Initialization) principles for resource management, ensuring that resources are properly released when they go out of scope
- Use concepts and SFINAE for template metaprogramming to create more expressive and type-safe templates
- Use Class Template Argument Deduction (CTAD) for cleaner syntax when creating instances of template classes without needing to specify template arguments explicitly
- Use Argument-Dependent Lookup (ADL) to allow for more flexible function overloading and better integration with user-defined types
- Use move semantics and forwarding to optimize performance when working with temporary objects and function arguments
- Use lambda expressions for concise and flexible function objects, especially for callbacks and event handlers
- Use Inherited Base Class Constructors to simplify constructor definitions in derived classes, allowing them to inherit constructors from their base class without needing to redefine them

#### Fold Expressions
Use Fold Expressions to provide a concise syntax to reduce a variadic template parameter pack over a binary operator There are four primary forms of fold expressions, determined by where the ellipsis `(...)` is placed and whether an initial value is provided:

	1. Unary Right Fold: `(pack op ...)`, Expands to `(p<sub>1</sub> op (p<sub>2</sub> op (p<sub>3</sub> op ... p<sub>n</sub>)))`
	2. Unary Left Fold: `(... op pack)`, Expands to `(((p<sub>1</sub> op p<sub>2</sub>) op p<sub>3</sub>) op ... p<sub>n</sub>)`
	3. Binary Right Fold: `(pack op ... op init)`, Expands to `(p<sub>1</sub> op (p<sub>2</sub> op (... (p<sub>n</sub> op init))))`
	4. Binary Left Fold: `(init op ... op pack)`, Expands to `((((init op p<sub>1</sub>) op p<sub>2</sub>) op ... p<sub>n</sub>))`
		- `pack`: An expression that contains an unexpanded pack and does not contain an operator with precedence lower than cast at the top level (formally, a cast-expression).
		- `op`: Any of the 32 valid C++ binary operators. **Note**: In a binary fold, both ops must be the same.
		- `init`: An expression that does not contain an unexpanded pack and does not contain an operator with precedence lower than cast at the top level (formally, a cast-expression)
	- (where N is the number of elements in the pack expansion)

**Example**
```
template<typename... Args>
bool all(Args... args) { return (... && args); }

bool b = all(true, true, true, false);
// within all(), the unary left fold expands as
//  return ((true && true) && true) && false;
// b is false
```

When a unary fold is used with a pack expansion of length zero, only the following operators are allowed:

1) Logical AND (`&&`). The value for the empty pack is `true`.
2) Logical OR (`||`). The value for the empty pack is `false`.
3) The comma operator (`,`). The value for the empty pack is `void()`.

**NOTE**
If the expression used as init or as pack has an operator with precedence below cast at the top level, it must be parenthesized:

```
template<typename... Args>
int sum(Args&&... args)
{
//  return (args + ... + 1 * 2);   // Error: operator with precedence below cast
    return (args + ... + (1 * 2)); // OK
}
```

### Core Layer Implementation

**Always inherit from the Layer base class for application components:**

```cpp
class CustomLayer : public Layer
{
public:
    explicit CustomLayer(const std::string& name = "CustomLayer")
        : Layer(name)
    {
    }

    // Implement lifecycle methods - called by ModuleStage in specific order
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("INIT", "=== Initializing {} ===", GetName());

        // Initialize resources using CreateRef<T>()
        m_Texture = CreateRef<Texture2D>("assets/default.png");
        m_Buffer = CreateRef<VertexBuffer>(1024);

        // Validate initialization
        SEDX_CORE_ASSERT(m_Texture->IsValid(), "Failed to load texture");
        SEDX_CORE_ASSERT(m_Buffer->IsValid(), "Failed to create buffer");

        // Register for events/callbacks
        RegisterEventHandlers();

        m_IsInitialized = true;
        SEDX_CORE_INFO_TAG("INIT", "{} initialization complete", GetName());
    }

    void OnDetach() override
    {
        SEDX_CORE_INFO_TAG("CLEANUP", "Cleaning up {}", GetName());

        // Cleanup is automatic with smart pointers
        m_Texture.Reset();
        m_Buffer.Reset();

        // Unregister from events/callbacks
        UnregisterEventHandlers();

        m_IsInitialized = false;
    }

    void OnUpdate() override
    {
        if (!m_IsEnabled || !m_IsInitialized) return;

        SEDX_PROFILE_SCOPE("CustomModule::OnUpdate");
        // Per-frame logic here
        ProcessFrameLogic();
    }

    void OnUIRender() override
    {
        if (!m_IsEnabled) return;

        // ImGui rendering for debug/UI panels
        if (m_ShowDebugPanel)
        {
            ImGui::Begin("Custom Module Debug");
            ImGui::Text("Module: %s", GetName().c_str());
            ImGui::Checkbox("Enabled", &m_IsEnabled);
            ImGui::End();
        }
    }

    void OnEvent() override
    {
        if (!m_IsEnabled) return;

        // Handle application events
        ProcessEvents();
    }

private:
    bool m_IsEnabled = true;
    bool m_IsInitialized = false;
    bool m_ShowDebugPanel = false;

    Ref<Texture2D> m_Texture;
    Ref<VertexBuffer> m_Buffer;
};
```

### LayerStage Management & Lifecycle

**Use LayerStage for proper layer ordering and lifecycle management:**

```cpp
class Application
{
private:
    LayerStage m_LayerStage;
    std::vector<Ref<Layer>> m_Layers; // Store references for lifetime management

public:
    void InitializeLayers()
    {
        SEDX_CORE_INFO("=== Initializing Application Layers ===");

        // Core systems first (use PushLayer for main application logic)
        auto terrainLayer = CreateRef<TerrainLayer>();
        auto renderLayer = CreateRef<RenderLayer>();
        auto assetLayer = CreateRef<AssetManagerLayer>();

        m_LayerStage.PushLayer(terrainLayer.get());
        m_LayerStage.PushLayer(renderLayer.get());
        m_LayerStage.PushLayer(assetLayer.get());

        // UI overlays last (use PushOverlay for UI/debug panels)
        auto debugOverlay = CreateRef<DebugOverlay>();
        auto settingsOverlay = CreateRef<SettingsOverlay>();

        m_LayerStage.PushOverlay(debugOverlay.get());
        m_LayerStage.PushOverlay(settingsOverlay.get());

        // Store references for lifetime management
        m_Layers = {
            terrainLayer, renderLayer, assetLayer,
            debugOverlay, settingsOverlay
        };

        // Initialize all layers in proper order (layers first, then overlays)
        for (auto* layer : m_LayerStage)
        {
            try
            {
                layer->OnAttach();
            }
            catch (const std::exception& e)
            {
                SEDX_CORE_ERROR_TAG("INIT", "Failed to initialize {}: {}",
                                   layer->GetName(), e.what());
                throw; // Re-throw to prevent partial initialization
            }
        }

        SEDX_CORE_INFO("=== Layer Initialization Complete ===");
    }

    void UpdateLayers()
    {
        SEDX_PROFILE_SCOPE("Application::UpdateLayers");

        for (auto* layer : m_LayerStage)
        {
            layer->OnUpdate();
        }
    }

    void RenderLayers()
    {
        for (auto* layer : m_LayerStage)
        {
            layer->OnUIRender();
        }
    }

    void ShutdownLayers()
    {
        // Detach in reverse order for proper cleanup
        for (auto it = m_LayerStage.rbegin(); it != m_LayerStage.rend(); ++it)
        {
            (*it)->OnDetach();
        }

        m_Layers.clear(); // Release all layer references
    }
};
```

## Custom Smart Pointer System

Scenery Editor X uses a custom reference-counted smart pointer system instead of standard library smart pointers. This system provides automatic memory management with deterministic destruction and better debugging capabilities.
---

- `RefCounted`: Base class that provides reference counting functionality for any class that inherits from it.

- `CreateRef<T>()`: Factory function for creating new objects managed by `Ref<T>`.
- `Ref<T>`: Strong reference that keeps the object alive as long as at least one `Ref<T>` exists.

- `WeakRef<T>`: Non-owning reference that can be used to check if the object is still alive without affecting its lifetime.

- `CreateScope<T>()`: Factory function for creating objects managed by `Scope<T>`.
- `Scope<T>`: Unique ownership pointer for exclusive ownership semantics (not reference-counted).

- `As<T>()`: Static cast for `Ref<T>` types, with debug assertions in debug builds.
- `DynamicCast<T>()`: Safe dynamic cast for `Ref<T>` types, returns `nullptr` if the cast fails.
- `Reset()`: Method to release the reference and potentially destroy the object if no more references exist.
- `Get()`: Method to access the raw pointer managed by `Ref<T>` or `Scope<T>`.

### Smart Pointer Usage Patterns

**Always use the custom smart pointer system:**

```cpp
// Object Creation - use CreateRef<T>() factory function
auto texture = CreateRef<Texture2D>("path/to/texture.png");
auto buffer = CreateRef<VertexBuffer>(bufferSize);
auto shader = CreateRef<Shader>("vertex.glsl", "fragment.glsl");

// Ownership Storage - use Ref<T> for owning references
class RenderSystem
{
private:
    Ref<Mesh> m_Mesh;           // Strong reference - keeps object alive
    Ref<Shader> m_Shader;       // Multiple Ref<T> can point to same object
    Ref<Camera> m_Camera;

public:
    void Initialize()
    {
        m_Mesh = CreateRef<Mesh>("assets/model.obj");
        m_Shader = CreateRef<Shader>("shaders/pbr");
        m_Camera = CreateRef<Camera>();

        // Validate creation
        SEDX_CORE_ASSERT(m_Mesh->IsValid(), "Failed to load mesh");
        SEDX_CORE_ASSERT(m_Shader->IsValid(), "Failed to compile shader");
    }

    // Reference passing - pass by const Ref<T>& for performance
    void SetShader(const Ref<Shader>& shader)
    {
        SEDX_CORE_ASSERT(shader != nullptr, "Shader cannot be null");
        m_Shader = shader; // Increment reference count
    }

    Ref<Shader> GetShader() const
    {
        return m_Shader; // Return copy increments reference count
    }
};

// Weak References - use WeakRef<T> for non-owning references
class ParticleSystem
{
private:
    WeakRef<Camera> m_CameraRef;    // Doesn't affect object lifetime
    WeakRef<Scene> m_SceneRef;      // Can become null if object destroyed

public:
    void SetCamera(const Ref<Camera>& camera)
    {
        m_CameraRef = camera; // Store weak reference
    }

    void Update()
    {
        if (auto camera = m_CameraRef.Lock()) // Convert to Ref<T> if still valid
        {
            // Use camera safely - guaranteed to be alive in this scope
            auto viewMatrix = camera->GetViewMatrix();
            ProcessParticles(viewMatrix);
        }
        else
        {
            SEDX_CORE_WARN("Camera reference is no longer valid");
        }
    }
};

// Resource Cleanup - use .Reset() to release references
class ResourceManager
{
private:
    std::unordered_map<std::string, Ref<Texture2D>> m_Textures;

public:
    void ClearTextures()
    {
        for (auto& [name, texture] : m_Textures)
        {
            texture.Reset(); // Release reference - object destroyed when ref count = 0
        }
        m_Textures.clear();
    }

    void RemoveTexture(const std::string& name)
    {
        if (auto it = m_Textures.find(name); it != m_Textures.end())
        {
            it->second.Reset(); // Explicit release
            m_Textures.erase(it);
        }
    }
};

// Type Casting - use .As<T>() for static cast, .DynamicCast<T>() for safe cast
class ComponentSystem
{
public:
    void ProcessComponent(const Ref<Component>& component)
    {
        // Static cast - use when you know the type (debug builds will assert)
        if (component->GetType() == ComponentType::Transform)
        {
            auto transform = component.As<TransformComponent>();
            transform->UpdateMatrix();
        }

        // Dynamic cast - safer but slower, returns nullptr if cast fails
        if (auto renderable = component.DynamicCast<RenderableComponent>())
        {
            renderable->Render();
        }
        else
        {
            SEDX_CORE_WARN("Component is not renderable: {}", component->GetName());
        }
    }
};

// Reference Counting Debug Information
void CheckReferenceCount(const Ref<Texture2D>& texture)
{
    SEDX_CORE_INFO("Texture '{}' has {} references",
                   texture->GetName(), texture.GetRefCount());

    // Warning: Don't store the count - it can change between calls!
}
```

### RefCounted Base Class Integration

**When creating new classes that need reference counting:**

```cpp
class CustomResource : public RefCounted
{
public:
    CustomResource(const std::string& name) : m_Name(name)
    {
        SEDX_CORE_INFO_TAG("RESOURCE", "Creating resource: {}", m_Name);
    }

    virtual ~CustomResource()
    {
        SEDX_CORE_INFO_TAG("RESOURCE", "Destroying resource: {}", m_Name);
    }

    const std::string& GetName() const { return m_Name; }

private:
    std::string m_Name;
};

// Usage
auto resource = CreateRef<CustomResource>("MyResource");
// Reference count = 1

auto backup = resource;
// Reference count = 2

resource.Reset();
// Reference count = 1, object still alive

backup.Reset();
// Reference count = 0, object destroyed, destructor called
```
---

# Comprehensive Logging and Assertion System

Scenery Editor X provides a sophisticated logging framework with tagged categorization, runtime configuration, and comprehensive assertion system for development-time validation.

## Tagged Logging System

**Use tagged logging for better categorization and debugging:**

- Core system logs should use `SEDX_CORE_*` macros without tags for general information.
- For more organized logs, use `SEDX_CORE_*_TAG()` macros to categorize messages by subsystem (e.g., "RENDER", "ASSET", "VULKAN").

#### Core System Logging (Engine/Framework code)

- `SEDX_CORE_INFO("General application information");` - Use for general information about application state and operations
- `SEDX_CORE_WARN("Non-critical warning occurred");` - Use for general information about application state and operations
- `SEDX_CORE_ERROR("Critical error that needs attention");` - Use for general information about application state and operations

#### Tagged Logging (Preferred for organized log output)

- `SEDX_CORE_INFO_TAG("Layer", "Layer '{}' initialized successfully", layerName);` - Use for logging layer initialization
- `SEDX_CORE_ERROR_TAG("VULKAN", "Vulkan API error: {} (code: {})", errorMsg, errorCode);` - Use for logging Vulkan API errors
- `SEDX_CORE_WARN_TAG("MEMORY", "High memory usage detected: {:.2f} MB", memUsageMB);` - Use for logging memory warnings
- `SEDX_CORE_INFO_TAG("ASSET", "Loaded texture: {} ({}x{})", texName, width, height);` - Use for logging asset loading
- `SEDX_CORE_DEBUG_TAG("RENDER", "Rendering frame {} with {} draw calls", frameNum, drawCalls);` - Use for logging rendering debug information

#### Application/Editor Logging (Application-specific code)

- `EDITOR_TRACE("Trace message for editor operations");` - Use for detailed tracing of editor operations
- `EDITOR_INFO("Editor-specific information");` - Use for general information related to editor operations
- `EDITOR_WARN("Panel '{}' failed to initialize", panelName);` - Use for logging warnings related to editor operations
- `EDITOR_ERROR("Failed to save project file: {}", filename);` - Use for logging errors related to editor operations
- `EDITOR_FATAL("Fatal error in editor: {}", errorMsg);` - Use for logging critical errors that may lead to crashes in editor operations

#### Range and Bounds Checking Asserts

- `SEDX_ASSERT_IN_RANGE(value, min, max, "Value {} is out of range ({} - {})", value, min, max);` - Use for validating that a value falls within a specified range, with formatted error messages in case of failure
- `SEDX_CORE_ASSERT_IN_RANGE(value, min, max, "Value {} is out of range ({} - {})", value, min, max);` - Use for validating that a value falls within a specified range, with formatted error messages in case of failure
- `SEDX_ASSERT_INDEX_IN_BOUNDS(index, size, "Index {} out of bounds (size: {})", index, size);` - Use for validating that a value falls within a specified range, with formatted error messages in case of failure
- `SEDX_CORE_ASSERT_INDEX_IN_BOUNDS(index, size, "Index {} out of bounds (size: {})", index, size);` - Use for validating that a value falls within a specified range, with formatted error messages in case of failure

#### Comparison Asserts

- `SEDX_ASSERT_EQUAL(expected, actual, "Expected {} but got {}", expected, actual);` - Use for validating that two values are equal, with formatted error messages in case of failure
- `SEDX_CORE_ASSERT_EQUAL(expected, actual, "Expected {} but got {}", expected, actual);` - Use for validating that two values are equal in core/engine code, with formatted error messages in case of failure
- `SEDX_ASSERT_NOT_EQUAL(unexpected, actual, "Did not expect {} but got {}", unexpected, actual);` - Use for validating that two values are not equal, with formatted error messages in case of failure
- `SEDX_CORE_ASSERT_NOT_EQUAL(unexpected, actual, "Did not expect {} but got {}", unexpected, actual);` - Use for validating that two values are not equal in core/engine code, with formatted error messages in case of failure

#### Static Assertions - Type Traits

- `SEDX_TRIVIAL_STATIC_ASSERT(T, "Type must be trivially copyable");` - Use for validating that a type is trivially copyable, which can be important for performance and memory management in graphics programming
- `SEDX_TRIVIAL_DESTRUCTIBLE_ASSERT(T, "Type must be trivially destructible");` - Use for validating that a type has a trivial destructor, which can allow for optimizations in memory management and object lifetime handling
- `SEDX_STD_LAYOUT_ASSERT(T, "Type must have standard layout");` - Use for validating that a type has a standard layout, which is important for interoperability with C APIs and certain memory layouts
- `SEDX_POD_ASSERT(T, "Type must be a POD type");` - Use for validating that a type is a Plain Old Data (POD) type, which can be important for serialization, memory copying, and performance optimizations
- `SEDX_DEFAULT_CONSTRUCTIBLE_ASSERT(T, "Type must be default constructible");` - Use for validating that a type can be default constructed, which is important for certain container types and algorithms that require default initialization
- `SEDX_COPY_CONSTRUCTIBLE_ASSERT(T, "Type must be copy constructible");` - Use for validating that a type can be copy constructed, which is important for value semantics and certain container types
- `SEDX_MOVE_CONSTRUCTIBLE_ASSERT(T, "Type must be move constructible");` - Use for validating that a type can be move constructed, which is important for performance optimizations and certain container types
- `SEDX_NOTHROW_MOVE_CONSTRUCTIBLE_ASSERT(T, "Type must be nothrow move constructible");` - Use for validating that a type can be move constructed without throwing exceptions, which can allow for certain optimizations in container implementations
- `SEDX_ENUM_ASSERT(T, "Type must be an enum");` - Use for validating that a type is an enumeration type, which can be important for certain template specializations and type traits
- `SEDX_SCOPED_ENUM_ASSERT(T, "Type must be a scoped enum");` - Use for validating that a type is a scoped enumeration (enum class), which can provide better type safety and scoping for enumerations
- `SEDX_POLYMORPHIC_ASSERT(T, "Type must be polymorphic");` - Use for validating that a type is polymorphic (has virtual functions), which can be important for certain dynamic behaviors and type traits
- `SEDX_ABSTRACT_ASSERT(T, "Type must be abstract");` - Use for validating that a type is abstract (has at least one pure virtual function), which can be important for defining interfaces and base classes
- `SEDX_FINAL_ASSERT(T, "Type must be final");` - Use for validating that a type is final (cannot be inherited from), which can be important for certain design patterns and optimizations

#### Vulkan Validation Assertions

- `SEDX_VK_RESULT_ASSERT(result, "Vulkan operation failed: {}", VulkanResultToString(result));` - Use for validating Vulkan API calls and translating error codes to readable messages

#### Static Assertions - Size and Alignment

- `SEDX_SIZE_ASSERT(T, expectedSize, "Type size must be {} bytes", expectedSize);` - Use for validating that a type has an exact size in bytes
- `SEDX_MAX_SIZE_ASSERT(T, maxSize, "Type size must be less than or equal to {} bytes", maxSize);` - Use for validating that a type does not exceed a maximum size in bytes
- `SEDX_MIN_SIZE_ASSERT(T, minSize, "Type size must be greater than or equal to {} bytes", minSize);` - Use for validating that a type meets a minimum size requirement in bytes
- `SEDX_ALIGNMENT_ASSERT(T, expectedAlignment, "Type alignment must be {} bytes", expectedAlignment);` - Use for validating that a type has an exact alignment requirement in bytes
- `SEDX_MIN_ALIGNMENT_ASSERT(T, minAlignment, "Type alignment must be at least {} bytes", minAlignment);` - Use for validating that a type meets a minimum alignment requirement in bytes
- `SEDX_SAME_SIZE_ASSERT(T1, T2, "Types must be the same size");` - Use for validating that two types have the same size in bytes
- `SEDX_POWER_OF_2_SIZE_ASSERT(T, "Type size must be a power of 2");` - Use for validating that a type's size is a power of 2, which can be important for GPU buffer layouts and alignment requirements

#### Static Assertions - Type Relationships

- `SEDX_DERIVED_FROM_ASSERT(Derived, Base, "Type must be derived from {}", typeid(Base).name());` - Use for validating inheritance relationships between types
- `SEDX_SAME_TYPE_ASSERT(T1, T2, "Types must be the same");` - Use for validating that two types are exactly the same
- `SEDX_CONVERTIBLE_TO_ASSERT(From, To, "Type {} must be convertible to {}", typeid(From).name(), typeid(To).name());` - Use for validating that one type can be implicitly converted to another

#### Static Assertions - Numeric Properties


- `SEDX_INTEGRAL_ASSERT(T, "Type must be an integral type");` - Use for validating that a type is an integral type
- `SEDX_FLOATING_POINT_ASSERT(T, "Type must be a floating-point type");` - Use for validating that a type is a floating-point type
- `SEDX_ARITHMETIC_ASSERT(T, "Type must be an arithmetic type");` - Use for validating that a type is either an integral or floating-point type
- `SEDX_SIGNED_ASSERT(T, "Type must be a signed type");` - Use for validating that a type is a signed type
- `SEDX_UNSIGNED_ASSERT(T, "Type must be an unsigned type");` - Use for validating that a type is an unsigned type

#### Static Assertions - Container and Pointer Properties

- `SEDX_POINTER_ASSERT(T, "Type must be a pointer");` - Use for validating that a type is a pointer type
- `SEDX_NOT_POINTER_ASSERT(T, "Type must not be a pointer");` - Use for validating that a type is not a pointer type
- `SEDX_REFERENCE_ASSERT(T, "Type must be a reference");` - Use for validating that a type is a reference type
- `SEDX_ARRAY_ASSERT(T, "Type must be an array");` - Use for validating that a type is an array type
- `SEDX_CLASS_ASSERT(T, "Type must be a class or struct");` - Use for validating that a type is a class or struct type
- `SEDX_EMPTY_ASSERT(T, "Type must be an empty class or struct");` - Use for validating that a type is an empty class or struct type (no non-static data members)

#### Combined Validation Macros

- `SEDX_VERTEX_STRUCT_ASSERT(T, "Type must be a valid vertex struct with position, normal, and UV attributes");` - Use these combined macros for common validation patterns in graphics programming, such as vertex structures, GPU buffer layouts, and flags enums.
- `SEDX_GPU_BUFFER_STRUCT_ASSERT(T, alignment, "Type must be a valid GPU buffer struct with standard layout and appropriate size");` - Use these combined macros for common validation patterns in graphics programming, such as vertex structures, GPU buffer layouts, and flags enums.
- `SEDX_FLAGS_ENUM_ASSERT(T, "Type must be a valid flags enum with bitwise operators defined");` - Use these combined macros for common validation patterns in graphics programming, such as vertex structures, GPU buffer layouts, and flags enums.

#### Compile-Time Constant Validation

- `SEDX_CONSTEXPR_ASSERT(expr, "Compile-time assertion failed: {}", #expr);` - Use for validating constant expressions at compile time
- `SEDX_POSITIVE_CONSTANT_ASSERT(expr, "Compile-time assertion failed: {} must be positive", #expr);` - Use for validating that a compile-time constant is positive
- `SEDX_NON_NEGATIVE_CONSTANT_ASSERT(expr, "Compile-time assertion failed: {} must be non-negative", #expr);` - Use for validating that a compile-time constant is zero or positive
- `SEDX_POWER_OF_2_CONSTANT_ASSERT(expr, "Compile-time assertion failed: {} must be a power of 2", #expr);`	- Use for validating that a compile-time constant is a power of 2

```cpp
// Layer Lifecycle Logging Pattern
class CustomLayer : public Layer
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("INIT", "=== Initializing {} ===", GetName());

        try
        {
            InitializeResources();
            SEDX_CORE_INFO_TAG("INIT", "{} resources loaded successfully", GetName());

            RegisterEventHandlers();
            SEDX_CORE_INFO_TAG("INIT", "{} event handlers registered", GetName());

            m_IsInitialized = true;
            SEDX_CORE_INFO_TAG("INIT", "{} initialization complete", GetName());
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("INIT", "{} initialization failed: {}", GetName(), e.what());
            m_InitializationFailed = true;
            throw;
        }
    }

    void OnDetach() override
    {
        SEDX_CORE_INFO_TAG("CLEANUP", "Cleaning up {}", GetName());

        UnregisterEventHandlers();
        SEDX_CORE_DEBUG_TAG("CLEANUP", "Event handlers unregistered for {}", GetName());

        // Resources cleaned up automatically by smart pointers
        m_Texture.Reset();
        m_Buffer.Reset();
        SEDX_CORE_DEBUG_TAG("CLEANUP", "Resources released for {}", GetName());

        m_IsInitialized = false;
        SEDX_CORE_INFO_TAG("CLEANUP", "{} cleanup complete", GetName());
    }

private:
    bool m_IsInitialized = false;
    bool m_InitializationFailed = false;
    Ref<Texture2D> m_Texture;
    Ref<VertexBuffer> m_Buffer;
};
```

### Assertion System for Development

**Use assertions for development-time validation and debugging:**

```cpp
// Core Assertions - use in engine/system code
SEDX_CORE_ASSERT(pointer != nullptr, "Pointer cannot be null");
SEDX_CORE_ASSERT(index < size, "Index {} out of bounds (size: {})", index, size);
SEDX_CORE_ASSERT(result == VK_SUCCESS, "Vulkan operation failed: {}", result);

// Application Assertions - use in editor code
SEDX_ASSERT(texture->IsValid(), "Texture must be valid before use");
SEDX_ASSERT(component != nullptr, "Component cannot be null");

// Assert that a pointer is not null
SEDX_CORE_ASSERT_NOT_NULL(resource, "Resource should not be null at this point");
SEDX_ASSERT_NOT_NULL(resource, "Resource should not be null at this point");

SEDX_ASSERT_NULL(resource, "Resource should be null at this point");
SEDX_CORE_ASSERT_NULL(resource, "Resource should be null at this point");

// Verification - continues execution in release builds, logs in debug
SEDX_CORE_VERIFY(resource->Initialize(), "Resource initialization failed");
SEDX_VERIFY(file.is_open(), "File should be open at this point");

// Resource Management Assertions
void LoadTexture(const std::string& path)
{
    SEDX_CORE_ASSERT(!path.empty(), "Texture path cannot be empty");

    auto texture = CreateRef<Texture2D>(path);
    SEDX_CORE_ASSERT(texture != nullptr, "Failed to create texture object");
    SEDX_CORE_ASSERT(texture->IsValid(), "Texture failed to load: {}", path);

    m_Textures[path] = texture;
    SEDX_CORE_INFO_TAG("ASSET", "Texture loaded: {}", path);
}

// Vulkan API Error Checking Pattern
VkResult CreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, VkBuffer* pBuffer)
{
    VkResult result = vkCreateBuffer(device, pCreateInfo, nullptr, pBuffer);
    SEDX_CORE_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan buffer: {}",
                     VulkanResultToString(result));

    if (result == VK_SUCCESS)
    {
        SEDX_CORE_DEBUG_TAG("VULKAN", "Buffer created successfully (size: {} bytes)",
                           pCreateInfo->size);
    }

    return result;
}

// State Validation in Update Loops
void OnUpdate() override
{
    SEDX_CORE_VERIFY(m_IsInitialized, "Module must be initialized before update");

    if (!m_IsEnabled || m_InitializationFailed)
    {
        return; // Skip update silently
    }

    SEDX_PROFILE_SCOPE("CustomModule::OnUpdate");

    // Validate critical state before processing
    SEDX_CORE_ASSERT(m_Renderer != nullptr, "Renderer is required for update");
    SEDX_CORE_ASSERT(m_Camera.IsValid(), "Valid camera is required");

    try
    {
        ProcessUpdate();
    }
    catch (const std::exception& e)
    {
        SEDX_CORE_ERROR_TAG("UPDATE", "Update failed in {}: {}", GetName(), e.what());
        // Don't re-throw in update loops to prevent crashes
    }
}
```
---

## Performance Profiling Integration

**Include profiling scopes in performance-critical code:**

#### Tracy Profiling macro definitions for function and scope profiling

- `SEDX_PROFILE_FUNC()` - Use at the beginning of a function to profile its execution time with the function name as the label. Is Tracy's `ZoneScoped` macro with automatic function name labeling.
- `SEDX_PROFILE_FUNC_COLOR(color)` - Use at the beginning of a function to profile its execution time with a custom color for the profiling zone. Is Tracy's `ZoneScopedC(color)` macro with automatic function name labeling.
- `SEDX_PROFILE_FRAME(name)` - Use at the beginning of the main loop or frame processing function to mark the start of a new frame in the profiler with a custom name. Is Tracy's `FrameMark` macro with a custom name.
- `SEDX_PROFILE_THREAD(name)` - Use at the beginning of a thread's entry point to name the thread in the profiler. Is Tracy's `SetThreadName` macro.
- `SEDX_PROFILE_ZONE(name, color)` - Use to create a custom profiling zone with a specific name and color. Is Tracy's `ZoneName(name, strlen(name))` `ZoneScopedC(color)` macro with a custom name. Also terminates with `SEDX_PROFILE_ZONE_END()`.

#### Predefined color constants for profiling zones

- `SEDX_PROFILE_COLOR_WAIT`			- Red
- `SEDX_PROFILE_COLOR_SUBMIT`		- Blue
- `SEDX_PROFILE_COLOR_PRESENT`		- Green
- `SEDX_PROFILE_COLOR_CREATE`		- Orange
- `SEDX_PROFILE_COLOR_DESTROY`		- Light orange
- `SEDX_PROFILE_COLOR_BARRIER`		- White
- `SEDX_PROFILE_COLOR_CMD_DRAW`		- Dark red
- `SEDX_PROFILE_COLOR_CMD_COPY`		- Dark pink
- `SEDX_PROFILE_COLOR_CMD_RTX`		- Dark red
- `SEDX_PROFILE_COLOR_CMD_DISPATCH` -	 Dark red

```cpp
void ExpensiveRenderFunction()
{
    SEDX_PROFILE_FUNC("ExpensiveRenderFunction");

    {
        SEDX_PROFILE_FUNC("Geometry Processing");
        ProcessGeometry();
    }

    {
        SEDX_PROFILE_FUNC("Material Binding");
        BindMaterials();
    }

    {
        SEDX_PROFILE_FUNC("Draw Calls");
        ExecuteDrawCalls();
    }
}

// Layer updates should always be profiled
void OnUpdate() override
{
    if (!m_IsEnabled) return;

    SEDX_PROFILE_FUNC("TerrainLayer::OnUpdate");

    // Profile individual systems
    {
        SEDX_PROFILE_FUNC("Terrain LOD Update");
        UpdateLevelOfDetail();
    }

    {
        SEDX_PROFILE_FUNC("Terrain Culling");
        PerformFrustumCulling();
    }
}
```

### Error Handling Best Practices

**Combine logging with proper error handling:**

```cpp
class RobustResourceLoader
{
public:
    Ref<Texture2D> LoadTexture(const std::string& path)
    {
        SEDX_CORE_INFO_TAG("LOADER", "Loading texture: {}", path);

        try
        {
            // Validate input
            SEDX_CORE_ASSERT(!path.empty(), "Texture path cannot be empty");

            if (!std::filesystem::exists(path))
            {
                SEDX_CORE_ERROR_TAG("LOADER", "Texture file not found: {}", path);
                return nullptr;
            }

            auto texture = CreateRef<Texture2D>(path);
            SEDX_CORE_VERIFY(texture != nullptr, "Texture creation failed");

            if (!texture->IsValid())
            {
                SEDX_CORE_ERROR_TAG("LOADER", "Invalid texture data: {}", path);
                return nullptr;
            }

            SEDX_CORE_INFO_TAG("LOADER", "Texture loaded: {} ({}x{})",
                              path, texture->GetWidth(), texture->GetHeight());
            return texture;
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("LOADER", "Exception loading texture {}: {}", path, e.what());
            return nullptr;
        }
    }
};
```

### Performance and Profiling

**Include profiling scopes in performance-critical code:**

```cpp
void ExpensiveFunction()
{
    SEDX_PROFILE_SCOPE("ExpensiveFunction");

    {
        SEDX_PROFILE_SCOPE("InitializationSection");
        InitializeData();
    }

    ProcessData();
}

// Module updates should be profiled
void OnUpdate() override
{
    if (!m_IsEnabled) return;
    SEDX_PROFILE_SCOPE("CustomModule::OnUpdate");

    // Update logic here
}
```

---

## Documentation Guidelines:

- All documentation uses the Doxygen style format.
- Please document all functions, enums, templates, structs, and classes with short but descriptive and helpful documentation and use.

### Documentation Examples:

```cpp

/**
* @brief Creates a Vulkan buffer with specified parameters
*
* This function handles the creation of a Vulkan buffer with appropriate memory allocation using VMA.
* It automatically applies usage flags based on the buffer's intended purpose and handles memory
* alignment requirements for different buffer types.
*
* The function performs the following:
* 1. Adjusts usage flags based on the provided buffer usage type
* 2. Adds transfer destination flags for vertex and index buffers
* 3. Handles special requirements for storage buffers and acceleration structures
* 4. Creates the buffer with VMA memory allocation
* 5. Sets up descriptor updates for storage buffers
*
* @param size   Size of the buffer in bytes
* @param usage  Flags specifying how the buffer will be used
* @param memory Memory property flags for the buffer allocation
* @param name   Optional name for the buffer (for debugging purposes)
*
* @return       A Buffer structure containing the created buffer and its metadata
*/
Buffer CreateBuffer(uint64_t size, BufferUsageFlags usage, MemoryFlags memory, const std::string &name)
{
    const auto device = RenderContext::Get()->GetLogicDevice();
        // Get the allocator from the current device
        const VmaAllocator vmaAllocator = device->GetMemoryAllocator();

    // Add transfer destination flag for vertex buffers
    if (usage & BufferUsage::Vertex)
        usage |= BufferUsage::TransferDst;

    // Add transfer destination flag for index buffers
    if (usage & BufferUsage::Index)
        usage |= BufferUsage::TransferDst;

    // Handle storage buffers - add address flag and align size
    if (usage & BufferUsage::Storage)
    {
        // Align storage buffer size to minimum required alignment
        usage |= BufferUsage::Address;
            size += size % device->GetPhysicalDevice()->GetDeviceProperties().limits.minStorageBufferOffsetAlignment;
    }

    // Handle acceleration structure input buffers
    if (usage & BufferUsage::AccelerationStructureInput)
    {
        usage |= BufferUsage::Address;
        usage |= BufferUsage::TransferDst;
    }

    // Handle acceleration structure buffers
    if (usage & BufferUsage::AccelerationStructure)
        usage |= BufferUsage::Address;

    // Create buffer resource
    const Ref<BufferResource> resource = CreateRef<BufferResource>();

    // Configure buffer creation info
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = static_cast<VkBufferUsageFlagBits>(usage);
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Configure memory allocation info
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    // Enable memory mapping for CPU-accessible buffers
    if (memory & CPU)
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    // Create the buffer with VMA
    SEDX_ASSERT(vmaCreateBuffer(vmaAllocator, &bufferInfo, &allocInfo, &resource->buffer, &resource->allocation, nullptr));

    // Create and populate the buffer wrapper
    Buffer buffer = {
        .resource = resource,
        .size = size,
        .usage = usage,
        .memory = memory,
    };

    // Handle storage buffer descriptors for bindless access
    if (usage & BufferUsage::Storage)
    {
        // BindlessResources bindlessData;
        // Get a resource ID from the available pool
        resource->resourceID = ImageID::availBufferRID.back();
        ImageID::availBufferRID.pop_back();

        // Set up descriptor info for the storage buffer
        VkDescriptorBufferInfo descriptorInfo;
        VkWriteDescriptorSet write = {};
        descriptorInfo.buffer = resource->buffer;
        descriptorInfo.offset = 0;
        descriptorInfo.range = size;

        // Configure descriptor write operation
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        //write.dstSet = bindlessData.bindlessDescriptorSet;
        write.dstBinding = 1;
        write.dstArrayElement = buffer.resource->resourceID;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &descriptorInfo;

        // Update descriptor set with buffer info
            vkUpdateDescriptorSets(device->GetDevice(), 1, &write, 0, nullptr);
    }

    return buffer;
}
```
---

## Dependencies:

- **Vulkan 1.4 SDK - (v1.4.341.1)** \
The rendering backend for Scenery Editor X, providing low-level access to GPU features and high performance for rendering complex scenes.
- **SDL3** \
This Scenery Editor X utilizes SDL3 for its multi-platform windowing and interaction library.
- **ImGui** \
The library for creating and managing the GUI elements and interactions in the application. Used for the multiple viewports and ImGui panel docking. This is the ImGui Docking branch that is being used allowing for multiple viewports and docking of panels.
- **ImGuizmo** \
ImGizmo is a small (.h and .cpp) library built ontop of Dear ImGui that allow you to manipulate(Rotate & translate at the moment) 4x4 float matrices. No other dependancies. Coded with Immediate Mode (IM) philosophy in mind.
- **libconfig** \
C/C++ library for processing structured configuration files. Scenery Editor X uses this library for creating, reading, and structuring the user settings, system settings, and core application settings. This library is to be use with any operation requring reading and writing configuration data to and from `.cfg` files.
- **Catch2** \
Used for writing and running unit tests.
- **X-PlaneSceneryLibrary** \
This library contains the data formating and layout of the multiple custom file formats that X-Plane uses.
- **Tracy** \
A real-time, nanosecond resolution, remote telemetry, hybrid frame and sampling profiler for games and other applications. Used for performance profiling in the application.
- **VMA (Vulkan Memory Allocator)** \
A library that provides a higher-level interface for managing memory in Vulkan applications. It simplifies the process of allocating and deallocating memory for buffers and images, and can help optimize memory usage and performance.
- **xMath** \
A custom math library that provides vector, matrix, and other mathematical operations commonly used in graphics programming. This library is used for all mathematical calculations in the application, including transformations, camera calculations, and physics simulations.
- **spdlog** \
A fast, header-only C++ logging library. Used for the logging system in the application, providing various log levels, formatting options, and performance optimizations.
- **fmt** \
A modern formatting library for C++. Used for string formatting in logging and other parts of the application where formatted output is needed.
- **RenderDoc** \
A graphics debugger that allows developers to capture and analyze frames from their applications. Used for debugging and optimizing rendering code in the application.
- **KTX** \
A file format for storing textures and texture arrays. Used for loading and managing texture assets in the application, especially those that require efficient GPU memory usage and fast loading times.
- **IconFontCppHeaders** \
A library that provides C++ header files for popular icon fonts, allowing developers to easily include and use icons in their applications. Used for adding icons to the ImGui interface in the application.
- **entt**  \
A fast and efficient entity-component-system (ECS) library for C++. Used for managing entities and their components in the application, allowing for flexible and efficient game object management.
- **TaskFlow** \
A C++ library for task-based parallelism. Used for managing and executing tasks in parallel, improving performance and responsiveness in the application.

---

# Code Style Guidelines

### Naming Conventions

```cpp
// Classes: PascalCase
class TerrainRenderer : public Layer { };

// Member variables: m_ prefix with camelCase
private:
    bool m_IsInitialized = false;
    Ref<Texture2D> m_DiffuseTexture;
    std::vector<Vertex> m_Vertices;

// Methods: PascalCase
void InitializeResources();
bool ValidateConfiguration();

// Constants: UPPER_SNAKE_CASE
static constexpr uint32_t MAX_TEXTURE_SIZE = 4096;
static constexpr float DEFAULT_FOV = 45.0f;

// Enums: PascalCase with scoped enum
enum class RenderPass
{
    Shadow,
    Depth,
    Forward,
    Transparent
};
```
---

## File Organization

```cpp
// Header file structure
#pragma once

// System includes
#include <vector>
#include <memory>
#include <string>

// Third-party includes
#include <vulkan/vulkan.h>
#include <imgui.h>

// Project includes
#include <SceneryEditorX/core/layers/layer.h>
#include <SceneryEditorX/core/pointers.h>
#include <SceneryEditorX/logging/logging.hpp>

namespace SceneryEditorX
{
    class CustomLayer : public Layer
    {
    public:
        // Constructor/Destructor
        // Public methods

    protected:
        // Protected methods

    private:
        // Private members
        // Private methods
    };
}
```

## UI Development with ImGui

**Create debug UI for layers:**

```cpp
class UILayer : public Layer
{
public:
    void OnUIRender() override
    {
        if (m_ShowDebugWindow)
        {
            ImGui::Begin("Layer Debug");

            ImGui::Text("Layer: %s", GetName().c_str());
            ImGui::Checkbox("Enabled", &m_IsEnabled);

            if (ImGui::CollapsingHeader("Settings"))
            {
                ImGui::SliderFloat("Update Rate", &m_UpdateRate, 1.0f, 60.0f);
                ImGui::ColorEdit3("Color", m_Color);
            }

            if (ImGui::Button("Reset Layer"))
            {
                ResetToDefaults();
            }

            ImGui::End();
        }
    }

private:
    bool m_ShowDebugWindow = false;
    bool m_IsEnabled = true;
    float m_UpdateRate = 60.0f;
    float m_Color[3] = {1.0f, 1.0f, 1.0f};
};
```
---

## Advanced Reflection and Serialization System

Reflection and serialization system that enables automatic serialization, runtime type inspection, and member access by name. The system is built around a **SERIALIZABLE** macro that integrates with the reflection framework.

#### SERIALIZABLE Macro for Type Introspection

**Use `SERIALIZABLE` macro to enable reflection and automatic serialization on classes and structs:**

```cpp
// Basic structure with reflection and serialization
struct Transform
{
    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 rotation{0.0f, 0.0f, 0.0f};
    Vec3 scale{1.0f, 1.0f, 1.0f};
    bool visible = true;
    float opacity = 1.0f;

    // Non-serialized methods
    void Reset() { *this = Transform{}; }
    bool IsIdentity() const { return position == Vec3{} && rotation == Vec3{} && scale == Vec3{1,1,1}; }
};

// Enable automatic serialization for all listed members
SERIALIZABLE(Transform,
    &Transform::position,
    &Transform::rotation,
    &Transform::scale,
    &Transform::visible,
    &Transform::opacity
);

// Asset class with reflection for serialization
class MaterialAsset : public RefCounted
{
public:
    MaterialAsset() = default;

    // Material properties
    Vec3 color{1.0f, 1.0f, 1.0f};
    Vec3 emission{0.0f, 0.0f, 0.0f};
    float metallic = 0.0f;
    float roughness = 0.5f;

    // Asset references
    Ref<Texture2D> colorMap;
    Ref<Texture2D> aoMap;
    Ref<Texture2D> emissionMap;
    Ref<Texture2D> normalMap;
    Ref<Texture2D> metallicRoughnessMap;

    // Custom serialization method for complex asset relationships
    void Serialize(Serializer& ser)
    {
        ser("color", color);
        ser("emission", emission);
        ser("metallic", metallic);
        ser("roughness", roughness);
        ser.Asset("colorMap", colorMap);
        ser.Asset("aoMap", aoMap);
        ser.Asset("emissionMap", emissionMap);
        ser.Asset("normalMap", normalMap);
        ser.Asset("metallicRoughnessMap", metallicRoughnessMap);
    }
};
```
---

# Advanced Serialization System Architecture

A multi-tiered serialization system supporting both binary and JSON formats, with automatic type registration and asset relationship management.

### Core Serialization Components

**1. SerializeWriter and SerializeReader Base Classes**

```cpp
// Binary Writer Interface - for performance-critical serialization
class SerializeWriter
{
public:
    virtual ~SerializeWriter() = default;

    [[nodiscard]] virtual bool IsStreamGood() const = 0;
    virtual uint64_t GetStreamPosition() = 0;
    virtual void SetStreamPosition(uint64_t position) = 0;
    virtual bool WriteData(const char* data, size_t size) = 0;

    explicit operator bool() const { return IsStreamGood(); }

    void WriteBuffer(Memory::Buffer buffer, bool writeSize = true);
    void WriteZero(uint64_t size);
    void WriteString(const std::string& string);

    template<typename T>
    void WriteRaw(const T& type)
    {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
        WriteData(reinterpret_cast<const char*>(&type), sizeof(T));
    }

    template<typename T>
    void WriteObject(const T& obj)
    {
        using namespace SceneryEditorX::Serialization;
        Serialize(this, obj);
    }
};

// Binary Reader Interface - for high-performance deserialization
class SerializeReader
{
public:
    virtual ~SerializeReader() = default;

    [[nodiscard]] virtual bool IsStreamGood() const = 0;
    virtual uint64_t GetStreamPosition() = 0;
    virtual void SetStreamPosition(uint64_t position) = 0;
    virtual bool ReadData(char* destination, size_t size) = 0;

    explicit operator bool() const { return IsStreamGood(); }

    void ReadBuffer(Memory::Buffer& buffer, uint32_t size = 0);
    void ReadString(std::string& string);

    template<typename T>
    void ReadRaw(T& type)
    {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
        ReadData(reinterpret_cast<char*>(&type), sizeof(T));
    }

    template<typename T>
    void ReadObject(T& obj)
    {
        using namespace SceneryEditorX::Serialization;
        Deserialize(this, obj);
    }
};
```

**2. File Streaming System for Asset Persistence**

```cpp
// File-based binary writer for asset packs and scene files
class FileStreamWriter : public StreamWriter
{
public:
    explicit FileStreamWriter(const std::filesystem::path& path);
    virtual ~FileStreamWriter() override;

    [[nodiscard]] virtual bool IsStreamGood() const override final { return m_Stream.good(); }
    virtual uint64_t GetStreamPosition() override final { return m_Stream.tellp(); }
    virtual void SetStreamPosition(uint64_t position) override final { m_Stream.seekp(position); }
    virtual bool WriteData(const char* data, size_t size) override final;

private:
    std::filesystem::path m_Path;
    std::ofstream m_Stream;
};

// File-based binary reader for asset loading
class FileStreamReader : public StreamReader
{
public:
    explicit FileStreamReader(const std::filesystem::path& path);
    virtual ~FileStreamReader() override;

    [[nodiscard]] const std::filesystem::path& GetFilePath() const { return m_Path; }
    [[nodiscard]] virtual bool IsStreamGood() const override final { return m_Stream.good(); }
    virtual uint64_t GetStreamPosition() override { return m_Stream.tellg(); }
    virtual void SetStreamPosition(uint64_t position) override { m_Stream.seekg(position); }
    virtual bool ReadData(char* destination, size_t size) override;

private:
    std::filesystem::path m_Path;
    std::ifstream m_Stream;
};
```

**3. Asset Serializer Framework**

```cpp
// Base serializer interface for all asset types
class AssetSerializer
{
public:
    virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const = 0;
    virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const = 0;
    virtual void RegisterDependencies(const AssetMetadata& metadata) const;

    // Asset pack serialization for efficient distribution
    virtual bool SerializeToAssetPack(uint64_t handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const = 0;
    virtual Ref<Asset> DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const = 0;
};

// Material asset serializer with JSON and binary support
class MaterialAssetSerializer : public AssetSerializer
{
public:
    virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override
    {
        const Ref<MaterialAsset> materialAsset = asset.As<MaterialAsset>();
        const std::string jsonString = SerializeToJSON(materialAsset);

        std::ofstream fout(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
        fout << jsonString;
    }

    virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override
    {
        std::ifstream fin(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
        if (!fin.is_open())
        {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Failed to open material file: {}", metadata.FilePath.string());
            return false;
        }

        std::string jsonString((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());

        Ref<MaterialAsset> materialAsset = CreateRef<MaterialAsset>();
        materialAsset->Handle = metadata.Handle;

        if (!DeserializeFromJSON(jsonString, materialAsset, metadata.Handle))
        {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Failed to deserialize material: {}", metadata.FilePath.string());
            return false;
        }

        asset = materialAsset;
        return true;
    }

    virtual bool SerializeToAssetPack(uint64_t handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const override
    {
        const Ref<MaterialAsset> materialAsset = AssetManager::Get<MaterialAsset>(handle);
        const std::string jsonString = SerializeToJSON(materialAsset);

        outInfo.Offset = stream.GetStreamPosition();
        stream.WriteString(jsonString);
        outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;

        return true;
    }

private:
    std::string SerializeToJSON(const Ref<MaterialAsset>& materialAsset) const
    {
        nlohmann::json json;

        // Serialize material properties
        json["color"] = {materialAsset->color.x, materialAsset->color.y, materialAsset->color.z};
        json["emission"] = {materialAsset->emission.x, materialAsset->emission.y, materialAsset->emission.z};
        json["metallic"] = materialAsset->metallic;
        json["roughness"] = materialAsset->roughness;

        // Serialize asset references by handle
        json["colorMap"] = materialAsset->colorMap ? materialAsset->colorMap->Handle : 0;
        json["aoMap"] = materialAsset->aoMap ? materialAsset->aoMap->Handle : 0;
        json["emissionMap"] = materialAsset->emissionMap ? materialAsset->emissionMap->Handle : 0;
        json["normalMap"] = materialAsset->normalMap ? materialAsset->normalMap->Handle : 0;
        json["metallicRoughnessMap"] = materialAsset->metallicRoughnessMap ? materialAsset->metallicRoughnessMap->Handle : 0;

        return json.dump(4);
    }

    bool DeserializeFromJSON(const std::string& jsonString, Ref<MaterialAsset>& targetMaterialAsset, uint64_t handle) const
    {
        try
        {
            nlohmann::json json = nlohmann::json::parse(jsonString);

            // Deserialize properties with macros for safety
            SEDX_DESERIALIZE_PROPERTY(color, targetMaterialAsset->color, json, Vec3{1.0f, 1.0f, 1.0f});
            SEDX_DESERIALIZE_PROPERTY(emission, targetMaterialAsset->emission, json, Vec3{0.0f, 0.0f, 0.0f});
            SEDX_DESERIALIZE_PROPERTY(metallic, targetMaterialAsset->metallic, json, 0.0f);
            SEDX_DESERIALIZE_PROPERTY(roughness, targetMaterialAsset->roughness, json, 0.5f);

            // Deserialize asset references
            SEDX_DESERIALIZE_PROPERTY_ASSET(colorMap, targetMaterialAsset->colorMap, json, Texture2D);
            SEDX_DESERIALIZE_PROPERTY_ASSET(aoMap, targetMaterialAsset->aoMap, json, Texture2D);
            SEDX_DESERIALIZE_PROPERTY_ASSET(emissionMap, targetMaterialAsset->emissionMap, json, Texture2D);
            SEDX_DESERIALIZE_PROPERTY_ASSET(normalMap, targetMaterialAsset->normalMap, json, Texture2D);
            SEDX_DESERIALIZE_PROPERTY_ASSET(metallicRoughnessMap, targetMaterialAsset->metallicRoughnessMap, json, Texture2D);

            return true;
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "JSON deserialization failed: {}", e.what());
            return false;
        }
    }
};
```
---

# Serialization Utility Macros

**Use these macros for safe property deserialization:**

```cpp
// Safe property deserialization with default fallback
#define SEDX_DESERIALIZE_PROPERTY(propName, destination, inputData, defaultValue) \
if ((inputData).contains(#propName))                                              \
{                                                                                 \
    try                                                                           \
    {                                                                             \
        (inputData)[#propName].get_to((destination));                           \
    }                                                                             \
    catch (const std::exception& e)                                              \
    {                                                                             \
        SEDX_CORE_ERROR_TAG("SERIALIZATION", "Failed to deserialize {}: {}", #propName, e.what()); \
        (destination) = defaultValue;                                            \
    }                                                                             \
}                                                                                 \
else                                                                              \
{                                                                                 \
    (destination) = defaultValue;                                                \
}

// Safe asset reference deserialization
#define SEDX_DESERIALIZE_PROPERTY_ASSET(propName, destination, inputData, assetClass) \
{                                                                                      \
    AssetHandle assetHandle = (inputData).contains(#propName) ? (inputData)[#propName].get<uint64_t>() : 0; \
    if (AssetManager::IsAssetHandleValid(assetHandle))                                 \
    {                                                                                  \
        (destination) = AssetManager::GetAsset<assetClass>(assetHandle);              \
    }                                                                                  \
    else                                                                               \
    {                                                                                  \
        (destination) = nullptr;                                                       \
    }                                                                                  \
}
```

## JSON Integration with nlohmann::json

**Built-in JSON serializers for common types:**

```cpp
namespace nlohmann
{
    // GLM vector type JSON conversions
    template<>
    struct adl_serializer<Vec2>
    {
        static void to_json(json& j, const Vec2& v)
        {
            j = json{v.x, v.y};
        }

        static void from_json(const json& j, Vec2& v)
        {
            if (j.is_array() && j.size() == 2)
            {
                v.x = j[0];
                v.y = j[1];
            }
        }
    };

    template<>
    struct adl_serializer<Vec3>
    {
        static void to_json(json& j, const Vec3& v)
        {
            j = json{v.x, v.y, v.z};
        }

        static void from_json(const json& j, Vec3& v)
        {
            if (j.is_array() && j.size() == 3)
            {
                v.x = j[0];
                v.y = j[1];
                v.z = j[2];
            }
        }
    };

    template<>
    struct adl_serializer<Vec4>
    {
        static void to_json(json& j, const Vec4& v)
        {
            j = json{v.x, v.y, v.z, v.w};
        }

        static void from_json(const json& j, Vec4& v)
        {
            if (j.is_array() && j.size() == 4)
            {
                v.x = j[0];
                v.y = j[1];
                v.z = j[2];
                v.w = j[3];
            }
        }
    };
}
```

## Asset Cache and Management System

**High-performance asset caching with serialization:**

```cpp
template<typename T>
class SerializableAssetCache : public Module
{
public:
    explicit SerializableAssetCache() : Module("AssetCache") {}

    void CacheAsset(const UUID& id, const T& asset)
    {
        SEDX_PROFILE_SCOPE("AssetCache::CacheAsset");

        MemoryWriter writer;

        using namespace SceneryEditorX::Serialization;
        if (Serialize(&writer, asset))
        {
            m_SerializedAssets[id] = writer.GetBuffer();
            m_LastAccess[id] = std::chrono::system_clock::now();

            SEDX_CORE_INFO_TAG("ASSET_CACHE", "Cached asset {} ({} bytes)",
                              id.ToString(), writer.GetBuffer().size());
        }
    }

    std::optional<T> LoadAsset(const UUID& id)
    {
        SEDX_PROFILE_SCOPE("AssetCache::LoadAsset");

        auto it = m_SerializedAssets.find(id);
        if (it == m_SerializedAssets.end())
        {
            return std::nullopt;
        }

        MemoryReader reader(it->second);
        T asset;

        using namespace SceneryEditorX::Serialization;
        if (Deserialize(&reader, asset))
        {
            m_LastAccess[id] = std::chrono::system_clock::now();
            SEDX_CORE_INFO_TAG("ASSET_CACHE", "Loaded asset {} from cache", id.ToString());
            return asset;
        }

        return std::nullopt;
    }

    void SaveCacheToFile(const std::string& filename)
    {
        FileStreamWriter writer(filename);
        if (!writer) return;

        // Write cache header with magic bytes
        struct CacheHeader
        {
            char magic[4] = {'A', 'C', 'H', 'E'};
            uint32_t version = 1;
            uint32_t assetCount;
        };

        CacheHeader header;
        header.assetCount = static_cast<uint32_t>(m_SerializedAssets.size());
        writer.WriteRaw(header);

        // Write each cached asset
        for (const auto& [id, data] : m_SerializedAssets)
        {
            writer.WriteRaw(static_cast<uint64_t>(id));
            writer.WriteRaw<uint32_t>(static_cast<uint32_t>(data.size()));
            writer.WriteData(reinterpret_cast<const char*>(data.data()), data.size());
        }

        SEDX_CORE_INFO_TAG("ASSET_CACHE", "Saved {} assets to cache file", header.assetCount);
    }

    bool LoadCacheFromFile(const std::string& filename)
    {
        FileStreamReader reader(filename);
        if (!reader) return false;

        // Read and validate header
        struct CacheHeader
        {
            char magic[4];
            uint32_t version;
            uint32_t assetCount;
        };

        CacheHeader header;
        reader.ReadRaw(header);

        if (strncmp(header.magic, "ACHE", 4) != 0)
        {
            SEDX_CORE_ERROR_TAG("ASSET_CACHE", "Invalid cache file format");
            return false;
        }

        if (header.version != 1)
        {
            SEDX_CORE_ERROR_TAG("ASSET_CACHE", "Unsupported cache version: {}", header.version);
            return false;
        }

        // Clear existing cache
        m_SerializedAssets.clear();
        m_LastAccess.clear();

        // Load each asset
        for (uint32_t i = 0; i < header.assetCount; ++i)
        {
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

private:
    std::unordered_map<UUID, std::vector<uint8_t>> m_SerializedAssets;
    std::unordered_map<UUID, std::chrono::system_clock::time_point> m_LastAccess;
};
```

## Tiered Settings Serialization with libconfig

**For configuration files, use the TieringSerializer for .cfg format:**

```cpp
class TieringSerializer
{
public:
    static void Serialize(const Tiering::TieringSettings& tieringSettings, const std::filesystem::path& filepath)
    {
        SEDX_PROFILE_SCOPE("TieringSerializer::Serialize");

        try
        {
            libconfig::Config config;
            libconfig::Setting& root = config.getRoot();

            // Create tiering structure
            libconfig::Setting& tiering = root.add("tiering", libconfig::Setting::TypeGroup);

            // Shadow settings
            libconfig::Setting& shadows = tiering.add("shadows", libconfig::Setting::TypeGroup);
            shadows.add("quality", libconfig::Setting::TypeInt) = static_cast<int>(tieringSettings.shadowQuality);
            shadows.add("cascadeCount", libconfig::Setting::TypeInt) = tieringSettings.shadowCascadeCount;
            shadows.add("distance", libconfig::Setting::TypeFloat) = tieringSettings.shadowDistance;

            // Ambient occlusion settings
            libconfig::Setting& ao = tiering.add("ambientOcclusion", libconfig::Setting::TypeGroup);
            ao.add("enabled", libconfig::Setting::TypeBoolean) = tieringSettings.aoEnabled;
            ao.add("quality", libconfig::Setting::TypeInt) = static_cast<int>(tieringSettings.aoQuality);
            ao.add("radius", libconfig::Setting::TypeFloat) = tieringSettings.aoRadius;

            // Screen-space reflections
            libconfig::Setting& ssr = tiering.add("screenSpaceReflections", libconfig::Setting::TypeGroup);
            ssr.add("enabled", libconfig::Setting::TypeBoolean) = tieringSettings.ssrEnabled;
            ssr.add("quality", libconfig::Setting::TypeInt) = static_cast<int>(tieringSettings.ssrQuality);
            ssr.add("maxDistance", libconfig::Setting::TypeFloat) = tieringSettings.ssrMaxDistance;

            // Write to file
            config.writeFile(filepath.string().c_str());

            SEDX_CORE_INFO_TAG("SERIALIZATION", "Tiering settings saved to: {}", filepath.string());
        }
        catch (const libconfig::ConfigException& e)
        {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Failed to save tiering settings: {}", e.what());
        }
    }

    static bool Deserialize(Tiering::TieringSettings& outTieringSettings, const std::filesystem::path& filepath)
    {
        SEDX_PROFILE_SCOPE("TieringSerializer::Deserialize");

        try
        {
            libconfig::Config config;
            config.readFile(filepath.string().c_str());

            const libconfig::Setting& tiering = config.lookup("tiering");

            // Load shadow settings
            if (tiering.exists("shadows"))
            {
                const libconfig::Setting& shadows = tiering.lookup("shadows");
                shadows.lookupValue("quality", reinterpret_cast<int&>(outTieringSettings.shadowQuality));
                shadows.lookupValue("cascadeCount", outTieringSettings.shadowCascadeCount);
                shadows.lookupValue("distance", outTieringSettings.shadowDistance);
            }

            // Load ambient occlusion settings
            if (tiering.exists("ambientOcclusion"))
            {
                const libconfig::Setting& ao = tiering.lookup("ambientOcclusion");
                ao.lookupValue("enabled", outTieringSettings.aoEnabled);
                ao.lookupValue("quality", reinterpret_cast<int&>(outTieringSettings.aoQuality));
                ao.lookupValue("radius", outTieringSettings.aoRadius);
            }

            // Load screen-space reflection settings
            if (tiering.exists("screenSpaceReflections"))
            {
                const libconfig::Setting& ssr = tiering.lookup("screenSpaceReflections");
                ssr.lookupValue("enabled", outTieringSettings.ssrEnabled);
                ssr.lookupValue("quality", reinterpret_cast<int&>(outTieringSettings.ssrQuality));
                ssr.lookupValue("maxDistance", outTieringSettings.ssrMaxDistance);
            }

            SEDX_CORE_INFO_TAG("SERIALIZATION", "Tiering settings loaded from: {}", filepath.string());
            return true;
        }
        catch (const libconfig::ConfigException& e)
        {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Failed to load tiering settings: {}", e.what());
            return false;
        }
    }
};
```

# Build System and Development Workflow

### Project Structure and CMake Configuration

**Key project structure patterns:**

```
SceneryEditorX/
├── .github/                         # GitHub workflows and copilot instructions
├── bin/                             # Compiled binaries and executables (Generated during build)
├── assets/                          # Runtime assets (textures, models, shaders)
├── build/                           # CMake build output (Generated during build)
├── cache/                           # Runtime cache directory
├── cmake/                           # Custom CMake modules
├── dependency/                      # Third-party dependencies (vcpkg managed)
│   ├── Catch2/                      # Testing framework
│   ├── entt/                        # Entity-component-system library
│   ├── freeimage/                   # Image loading library
│   ├── freetype/                    # Font rendering library
│   ├── IconFontCppHeaders/          # Icon font header
│   ├── json/                        # JSON library
│   ├── json-cpp-gen/                # JSON library generator
│   ├── libconfig/                   # Configuration library
│   ├── msdf-atlas-gen/              # MSDF atlas generator
│   ├── renderdoc/                   # RenderDoc API
│   ├── SDL/                         # Window management
│   ├── taskflow/                    # Taskflow library
│   ├── vcpkg/                       # Package manager
│   ├── tiny_gltf.h                  # TinyGLTF include header
│   ├── tiny_obj_loader.cc           # TinyOBJLoader source file
│   ├── tiny_obj_loader.h            # TinyOBJLoader header file
│   └── tinyddsloader.h              # TinyDDSLoader header file
├── docs/                            # Technical documentation
├── lib/                             # Compiled library output (Generated during build)
├── natvis/                          # Visual Studio debugger visualizations
├── pdb/                             # Program database files for debugging (Generated during build)
├── plugins/                         # Plugin modules
├── projects/                        # Project modules
├── resources/                       # Runtime resources
├── scripts/                         # Build and utility scripts
└── source/                          # Main source code
	├── CrashHandler/              	 # Crash handling and minidump generation
	├── Editor/              	 	 # The actual runtime editor application
	│   ├── core/ 			 		 # Core editor systems (UI, input, etc.)
	│   ├── modules/ 			 	 # Editor modules
	│   ├── projects/ 			 	 # Code for project management and scene handling
	│   ├── settings/ 			 	 # Editor settings management
	│   └── ui/ 			 	 	 # Editor user interface
	├── Launcher/              	 	 # Scenery Editor X Launcher application
	│   ├── core/ 			 		 # Core systems
	│   ├── registry/ 			 	 # Registry key management and utilities
	│   └── ui/ 			 	 	 # User interface
	├── SceneryEditorX/              # Core engine/application code
	│   ├── asset/                   # Asset management code
	│   ├── core/                    # Core systems (modules, memory, etc.)
	│   ├── filesystem/              # File system management
	│   ├── logging/                 # Logging system
	│   ├── plugin/                	 # Plugin system
	│   ├── project/                 # Project management system
	│   ├── renderer/                # Vulkan rendering system
	│   ├── scene/                   # Scene management
	│   ├── serialization/           # Serialization system
	│   ├── settings/                # Settings management
	│   └── utils/             		 # Utilities
	└── Tests/                       # Unit tests and test utilities
```
---

### Testing Framework Integration

**Catch2 test configuration:**

```cmake
# Test executable setup
ADD_EXECUTABLE(RefTests
    ref_counting_tests/RefTest.cpp
    ref_counting_tests/WeakRefTest.cpp
)

TARGET_LINK_LIBRARIES(RefTests PRIVATE
    Catch2::Catch2WithMain
    # Link against your modules/libraries
)

# Enable CTest integration
INCLUDE(CTest)
INCLUDE(Catch)
catch_discover_tests(RefTests)

# Test organization by folders
SET_PROPERTY(TARGET RefTests MemoryAllocatorTests SettingsTest
             PROPERTY FOLDER "Tests")
```

**Writing Catch2 tests:**

```cpp
#include <catch2/catch_test_macros.hpp>
#include <SceneryEditorX/utils/pointers.h>

TEST_CASE("Ref counting functionality", "[memory][ref]")
{
    SECTION("Basic reference counting")
    {
        auto obj = CreateRef<TestObject>();
        REQUIRE(obj.GetRefCount() == 1);

        {
            auto copy = obj;
            REQUIRE(obj.GetRefCount() == 2);
        }

        REQUIRE(obj.GetRefCount() == 1);
    }

    SECTION("WeakRef behavior")
    {
        WeakRef<TestObject> weak;

        {
            auto strong = CreateRef<TestObject>();
            weak = strong;
            REQUIRE(weak.Lock() != nullptr);
        }

        REQUIRE(weak.Lock() == nullptr);
    }
}

TEST_CASE("Layer lifecycle", "[layer][core]")
{
    auto layer = CreateRef<TestLayer>("TestLayer");

    SECTION("Initialization")
    {
        REQUIRE_NOTHROW(layer->OnAttach());
        REQUIRE(layer->IsInitialized());
    }

    SECTION("Cleanup")
    {
        layer->OnAttach();
        REQUIRE_NOTHROW(layer->OnDetach());
        REQUIRE_FALSE(layer->IsInitialized());
    }
}
```

### Development and Debug Setup

**VS Code tasks configuration patterns:**

```json
{
    "label": "CMake Configure",
    "type": "shell",
    "command": "cmake",
    "args": ["-S", ".", "-B", "build", "-DCMAKE_BUILD_TYPE=Debug"],
    "group": "build"
}

{
    "label": "Build All Tests",
    "type": "shell",
    "command": "cmake",
    "args": [
        "--build", "build",
        "--target", "RefTests", "MemoryAllocatorTests", "SettingsTest",
        "--config", "Debug", "--parallel"
    ],
    "group": {"kind": "test", "isDefault": true},
    "dependsOn": "CMake Configure"
}

{
    "label": "Run RefTests with Enhanced Logging",
    "type": "shell",
    "command": "powershell",
    "args": [
        "-Command", "& {",
        "if (!(Test-Path 'test-logs')) { New-Item -ItemType Directory -Path 'test-logs' };",
        "& '${workspaceFolder}/build/source/Tests/Debug/RefTests.exe' --reporter console --verbosity high 2>&1 | Tee-Object -FilePath test-logs/RefTests_console.log;",
        "}"
    ],
    "dependsOn": "Build RefTests"
}
```

**Debugging and logging workflow:**

```cpp
// Debug module initialization
void Application::InitializeDebugModules()
{
    SEDX_CORE_INFO_TAG("DEBUG", "=== Initializing Debug Modules ===");

    // Create debug UI modules
    auto debugOverlay = CreateRef<DebugOverlay>();
    auto performanceOverlay = CreateRef<PerformanceOverlay>();
    auto memoryTracker = CreateRef<MemoryTrackerModule>();

    // Configure debug settings
    debugOverlay->SetEnabled(true);
    performanceOverlay->SetUpdateFrequency(60.0f);
    memoryTracker->EnableDetailedTracking(true);

    // Register debug modules
    m_ModuleStage.PushOverlay(debugOverlay.get());
    m_ModuleStage.PushOverlay(performanceOverlay.get());
    m_ModuleStage.PushModule(memoryTracker.get());

    // Store references
    m_DebugModules = {debugOverlay, performanceOverlay, memoryTracker};

    SEDX_CORE_INFO_TAG("DEBUG", "✓ Debug modules ready");
}

// Performance profiling integration
class PerformanceProfiler
{
public:
    void BeginFrame()
    {
        SEDX_PROFILE_SCOPE("Frame");
        m_FrameStart = std::chrono::high_resolution_clock::now();
    }

    void EndFrame()
    {
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>
                           (frameEnd - m_FrameStart);

        m_FrameTimes.push_back(frameDuration.count());

        // Keep only last 60 frames
        if (m_FrameTimes.size() > 60)
        {
            m_FrameTimes.erase(m_FrameTimes.begin());
        }

        SEDX_CORE_DEBUG_TAG("PERF", "Frame time: {:.2f}ms",
                           frameDuration.count() / 1000.0f);
    }

private:
    std::chrono::high_resolution_clock::time_point m_FrameStart;
    std::vector<int64_t> m_FrameTimes;
};
```

## Key Reminders for Copilot

1. **Always use the Module base class** for application components
2. **Prefer PushModule() for core systems**, PushOverlay() for UI
3. **Use CreateRef `<T>`() for object creation**, never raw new/delete
4. **Include profiling scopes** in performance-critical code
5. **Use tagged logging** for better categorization
6. **Implement OnAttach/OnDetach pairs** for proper lifecycle management
7. **Follow RAII principles** for resource management
8. **Use assertions** for development-time validation
9. **Include debug UI** for runtime inspection when appropriate
10. **Handle errors gracefully** with proper logging and recovery
11. **Use SERIALIZABLE macro** for automatic type serialization
12. **Implement AssetSerializer** for each asset type with both JSON and binary support
13. **Use FileStreamWriter/Reader** for efficient binary asset packs
14. **Apply SEDX_DESERIALIZE_PROPERTY macros** for safe JSON deserialization
15. **Use TieringSerializer** for configuration files (.cfg format)
16. **Always validate stream state** with IsStreamGood() before operations
17. **Profile serialization operations** with appropriate scopes
18. **Use asset reference serialization** for maintaining relationships between assets

---

### Error Handling Patterns

```cpp
class RobustLayer : public Layer
{
public:
    void OnAttach() override
    {
        try
        {
            SEDX_CORE_INFO("Initializing {}", GetName());

            if (!ValidateDependencies())
            {
                SEDX_CORE_ERROR("Dependencies not met for {}", GetName());
                m_InitializationFailed = true;
                return;
            }

            InitializeResources();
            m_IsInitialized = true;
            SEDX_CORE_INFO("{} initialized successfully", GetName());
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR("Exception in {} initialization: {}", GetName(), e.what());
            m_InitializationFailed = true;
        }
    }

    void OnUpdate() override
    {
        if (m_InitializationFailed || !m_IsInitialized)
            return;

        try
        {
            PerformUpdate();
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR("Exception in {} update: {}", GetName(), e.what());
        }
    }

private:
    bool m_IsInitialized = false;
    bool m_InitializationFailed = false;
};
```
---

# Serialization Development Patterns

### Asset Type Implementation Pattern

**Create new asset types with complete serialization support:**

```cpp
// Asset class definition
class TerrainAsset : public RefCounted
{
public:
    TerrainAsset() = default;

    // Terrain-specific properties
    std::string m_HeightmapPath;
    Vec2 m_TerrainSize{1000.0f, 1000.0f};
    float m_HeightScale = 100.0f;
    uint32_t m_ResolutionX = 512;
    uint32_t m_ResolutionY = 512;

    // Material assignments
    std::vector<Ref<MaterialAsset>> m_TerrainLayers;
    std::vector<float> m_LayerBlendWeights;

    // Custom serialization for complex types
    void Serialize(Serializer& ser)
    {
        ser("heightmapPath", m_HeightmapPath);
        ser("terrainSize", m_TerrainSize);
        ser("heightScale", m_HeightScale);
        ser("resolutionX", m_ResolutionX);
        ser("resolutionY", m_ResolutionY);
        ser.VectorRef("terrainLayers", m_TerrainLayers);
        ser.Vector("layerBlendWeights", m_LayerBlendWeights);
    }
};

// Asset serializer implementation
class TerrainAssetSerializer : public AssetSerializer
{
public:
    virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override
    {
        const Ref<TerrainAsset> terrainAsset = asset.As<TerrainAsset>();
        const std::string jsonString = SerializeToJSON(terrainAsset);

        std::ofstream fout(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
        fout << jsonString;

        SEDX_CORE_INFO_TAG("SERIALIZATION", "Terrain asset serialized: {}", metadata.FilePath.string());
    }

    virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override
    {
        std::ifstream fin(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
        if (!fin.is_open())
        {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Failed to open terrain file: {}", metadata.FilePath.string());
            return false;
        }

        std::string jsonString((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());

        Ref<TerrainAsset> terrainAsset = CreateRef<TerrainAsset>();
        terrainAsset->Handle = metadata.Handle;

        if (!DeserializeFromJSON(jsonString, terrainAsset))
        {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Failed to deserialize terrain: {}", metadata.FilePath.string());
            return false;
        }

        asset = terrainAsset;
        return true;
    }

    virtual bool SerializeToAssetPack(uint64_t handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const override
    {
        const Ref<TerrainAsset> terrainAsset = AssetManager::Get<TerrainAsset>(handle);
        const std::string jsonString = SerializeToJSON(terrainAsset);

        outInfo.Offset = stream.GetStreamPosition();
        stream.WriteString(jsonString);
        outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;

        SEDX_CORE_DEBUG_TAG("ASSET_PACK", "Terrain serialized to pack: {} bytes", outInfo.Size);
        return true;
    }

    virtual Ref<Asset> DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const override
    {
        stream.SetStreamPosition(assetInfo.PackedOffset);
        std::string jsonString;
        stream.ReadString(jsonString);

        Ref<TerrainAsset> terrainAsset = CreateRef<TerrainAsset>();
        if (!DeserializeFromJSON(jsonString, terrainAsset))
        {
            SEDX_CORE_ERROR_TAG("ASSET_PACK", "Failed to deserialize terrain from pack");
            return nullptr;
        }

        return terrainAsset;
    }

private:
    std::string SerializeToJSON(const Ref<TerrainAsset>& terrain) const
    {
        nlohmann::json json;

        json["heightmapPath"] = terrain->m_HeightmapPath;
        json["terrainSize"] = {terrain->m_TerrainSize.x, terrain->m_TerrainSize.y};
        json["heightScale"] = terrain->m_HeightScale;
        json["resolutionX"] = terrain->m_ResolutionX;
        json["resolutionY"] = terrain->m_ResolutionY;

        // Serialize material layer handles
        json["terrainLayers"] = nlohmann::json::array();
        for (const auto& material : terrain->m_TerrainLayers)
        {
            json["terrainLayers"].push_back(material ? material->Handle : 0);
        }

        json["layerBlendWeights"] = terrain->m_LayerBlendWeights;

        return json.dump(4);
    }

    bool DeserializeFromJSON(const std::string& jsonString, Ref<TerrainAsset>& terrain) const
    {
        try
        {
            nlohmann::json json = nlohmann::json::parse(jsonString);

            SEDX_DESERIALIZE_PROPERTY(heightmapPath, terrain->m_HeightmapPath, json, "");
            SEDX_DESERIALIZE_PROPERTY(terrainSize, terrain->m_TerrainSize, json, Vec2{1000.0f, 1000.0f});
            SEDX_DESERIALIZE_PROPERTY(heightScale, terrain->m_HeightScale, json, 100.0f);
            SEDX_DESERIALIZE_PROPERTY(resolutionX, terrain->m_ResolutionX, json, 512u);
            SEDX_DESERIALIZE_PROPERTY(resolutionY, terrain->m_ResolutionY, json, 512u);
            SEDX_DESERIALIZE_PROPERTY(layerBlendWeights, terrain->m_LayerBlendWeights, json, std::vector<float>());

            // Deserialize material layer references
            if (json.contains("terrainLayers") && json["terrainLayers"].is_array())
            {
                terrain->m_TerrainLayers.clear();
                for (const auto& layerHandle : json["terrainLayers"])
                {
                    AssetHandle handle = layerHandle.get<uint64_t>();
                    if (AssetManager::IsAssetHandleValid(handle))
                    {
                        terrain->m_TerrainLayers.push_back(AssetManager::GetAsset<MaterialAsset>(handle));
                    }
                    else
                    {
                        terrain->m_TerrainLayers.push_back(nullptr);
                    }
                }
            }

            return true;
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("SERIALIZATION", "Terrain JSON deserialization failed: {}", e.what());
            return false;
        }
    }
};
```

## Binary Scene Serialization Pattern

**Efficient scene serialization using the binary system:**

```cpp
class SceneManager : public Module
{
public:
    bool SaveSceneToBinary(const Ref<Scene>& scene, const std::filesystem::path& filepath)
    {
        SEDX_PROFILE_SCOPE("SceneManager::SaveSceneToBinary");

        FileStreamWriter writer(filepath);
        if (!writer.IsStreamGood())
        {
            SEDX_CORE_ERROR_TAG("SCENE", "Failed to create scene file: {}", filepath.string());
            return false;
        }

        // Write scene header
        constexpr char SCENE_MAGIC[4] = {'S', 'C', 'N', 'E'};
        constexpr uint32_t SCENE_VERSION = 1;

        writer.WriteRaw(SCENE_MAGIC);
        writer.WriteRaw(SCENE_VERSION);
        writer.WriteString(scene->name);
        writer.WriteRaw(scene->uuid);

        // Use reflection-based binary serialization
        using namespace SceneryEditorX::Serialization;
        if (!Serialize(&writer, *scene))
        {
            SEDX_CORE_ERROR_TAG("SCENE", "Failed to serialize scene data: {}", scene->name);
            return false;
        }

        SEDX_CORE_INFO_TAG("SCENE", "Scene saved: {} ({} bytes)",
                          scene->name, writer.GetStreamPosition());
        return true;
    }

    Ref<Scene> LoadSceneFromBinary(const std::filesystem::path& filepath)
    {
        SEDX_PROFILE_SCOPE("SceneManager::LoadSceneFromBinary");

        FileStreamReader reader(filepath);
        if (!reader.IsStreamGood())
        {
            SEDX_CORE_ERROR_TAG("SCENE", "Failed to open scene file: {}", filepath.string());
            return nullptr;
        }

        // Read and validate scene header
        char magic[4];
        uint32_t version;
        reader.ReadRaw(magic);
        reader.ReadRaw(version);

        if (strncmp(magic, "SCNE", 4) != 0)
        {
            SEDX_CORE_ERROR_TAG("SCENE", "Invalid scene file format: {}", filepath.string());
            return nullptr;
        }

        if (version != 1)
        {
            SEDX_CORE_ERROR_TAG("SCENE", "Unsupported scene version {}: {}", version, filepath.string());
            return nullptr;
        }

        // Read scene metadata
        std::string sceneName;
        uint32_t sceneUuid;
        reader.ReadString(sceneName);
        reader.ReadRaw(sceneUuid);

        // Create scene and deserialize
        auto scene = CreateRef<Scene>();
        scene->name = sceneName;
        scene->uuid = sceneUuid;

        using namespace SceneryEditorX::Serialization;
        if (!Deserialize(&reader, *scene))
        {
            SEDX_CORE_ERROR_TAG("SCENE", "Failed to deserialize scene data: {}", sceneName);
            return nullptr;
        }

        SEDX_CORE_INFO_TAG("SCENE", "Scene loaded: {} ({} bytes)",
                          sceneName, reader.GetStreamPosition());
        return scene;
    }
};
```

## Configuration Management Pattern

**Structured settings serialization with validation:**

```cpp
struct ApplicationSettings
{
    // Rendering settings
    uint32_t renderWidth = 1920;
    uint32_t renderHeight = 1080;
    bool fullscreen = false;
    uint32_t msaaSamples = 4;
    bool vsync = true;

    // Editor settings
    std::string defaultProjectPath = "projects/";
    bool autoSave = true;
    float autoSaveInterval = 300.0f; // 5 minutes

    // Performance settings
    uint32_t maxThreads = 0; // 0 = auto-detect
    size_t maxMemoryMB = 4096;
    bool enableProfiling = false;
};

// Enable reflection-based serialization
SERIALIZABLE(ApplicationSettings,
    &ApplicationSettings::renderWidth,
    &ApplicationSettings::renderHeight,
    &ApplicationSettings::fullscreen,
    &ApplicationSettings::msaaSamples,
    &ApplicationSettings::vsync,
    &ApplicationSettings::defaultProjectPath,
    &ApplicationSettings::autoSave,
    &ApplicationSettings::autoSaveInterval,
    &ApplicationSettings::maxThreads,
    &ApplicationSettings::maxMemoryMB,
    &ApplicationSettings::enableProfiling
);

class SettingsManager : public Module
{
public:
    explicit SettingsManager() : Module("SettingsManager") {}

    bool SaveSettings(const ApplicationSettings& settings, const std::filesystem::path& filepath)
    {
        SEDX_PROFILE_SCOPE("SettingsManager::SaveSettings");

        try
        {
            libconfig::Config config;
            libconfig::Setting& root = config.getRoot();

            // Application group
            auto& app = root.add("application", libconfig::Setting::TypeGroup);
            app.add("renderWidth", libconfig::Setting::TypeInt) = static_cast<int>(settings.renderWidth);
            app.add("renderHeight", libconfig::Setting::TypeInt) = static_cast<int>(settings.renderHeight);
            app.add("fullscreen", libconfig::Setting::TypeBoolean) = settings.fullscreen;
            app.add("msaaSamples", libconfig::Setting::TypeInt) = static_cast<int>(settings.msaaSamples);
            app.add("vsync", libconfig::Setting::TypeBoolean) = settings.vsync;

            // Editor group
            auto& editor = root.add("editor", libconfig::Setting::TypeGroup);
            editor.add("defaultProjectPath", libconfig::Setting::TypeString) = settings.defaultProjectPath.c_str();
            editor.add("autoSave", libconfig::Setting::TypeBoolean) = settings.autoSave;
            editor.add("autoSaveInterval", libconfig::Setting::TypeFloat) = settings.autoSaveInterval;

            // Performance group
            auto& performance = root.add("performance", libconfig::Setting::TypeGroup);
            performance.add("maxThreads", libconfig::Setting::TypeInt) = static_cast<int>(settings.maxThreads);
            performance.add("maxMemoryMB", libconfig::Setting::TypeInt) = static_cast<int>(settings.maxMemoryMB);
            performance.add("enableProfiling", libconfig::Setting::TypeBoolean) = settings.enableProfiling;

            config.writeFile(filepath.string().c_str());

            SEDX_CORE_INFO_TAG("SETTINGS", "Application settings saved: {}", filepath.string());
            return true;
        }
        catch (const libconfig::ConfigException& e)
        {
            SEDX_CORE_ERROR_TAG("SETTINGS", "Failed to save settings: {}", e.what());
            return false;
        }
    }

    bool LoadSettings(ApplicationSettings& settings, const std::filesystem::path& filepath)
    {
        SEDX_PROFILE_SCOPE("SettingsManager::LoadSettings");

        try
        {
            libconfig::Config config;
            config.readFile(filepath.string().c_str());

            // Load application settings with validation
            if (config.getRoot().exists("application"))
            {
                const auto& app = config.getRoot().lookup("application");

                int width, height, samples;
                if (app.lookupValue("renderWidth", width) && width > 0)
                    settings.renderWidth = static_cast<uint32_t>(width);
                if (app.lookupValue("renderHeight", height) && height > 0)
                    settings.renderHeight = static_cast<uint32_t>(height);
                if (app.lookupValue("msaaSamples", samples) && samples >= 0)
                    settings.msaaSamples = static_cast<uint32_t>(samples);

                app.lookupValue("fullscreen", settings.fullscreen);
                app.lookupValue("vsync", settings.vsync);
            }

            // Load editor settings
            if (config.getRoot().exists("editor"))
            {
                const auto& editor = config.getRoot().lookup("editor");

                std::string projectPath;
                if (editor.lookupValue("defaultProjectPath", projectPath))
                    settings.defaultProjectPath = projectPath;

                editor.lookupValue("autoSave", settings.autoSave);
                editor.lookupValue("autoSaveInterval", settings.autoSaveInterval);
            }

            // Load performance settings
            if (config.getRoot().exists("performance"))
            {
                const auto& perf = config.getRoot().lookup("performance");

                int maxThreads, maxMemory;
                if (perf.lookupValue("maxThreads", maxThreads) && maxThreads >= 0)
                    settings.maxThreads = static_cast<uint32_t>(maxThreads);
                if (perf.lookupValue("maxMemoryMB", maxMemory) && maxMemory > 0)
                    settings.maxMemoryMB = static_cast<size_t>(maxMemory);

                perf.lookupValue("enableProfiling", settings.enableProfiling);
            }

            SEDX_CORE_INFO_TAG("SETTINGS", "Application settings loaded: {}", filepath.string());
            return true;
        }
        catch (const libconfig::ConfigException& e)
        {
            SEDX_CORE_WARN_TAG("SETTINGS", "Failed to load settings (using defaults): {}", e.what());
            return false; // Use default values
        }
    }

private:
    ApplicationSettings m_CurrentSettings;
};
```

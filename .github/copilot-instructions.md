# GitHub Copilot Instructions for Scenery Editor X Application

This repository contains the C++ source code for Scenery Editor X, a sophisticated 3D scenery editor with a modular architecture for exporting and importing 3D models and related data.

## General Principles

- Code must be written to be **readable, maintainable, and well-structured**.
- Follow the **DRY (Don't Repeat Yourself) principle** to avoid redundancy.
- Keep code **simple and concise** while ensuring clarity-**avoid over-engineering**
- Write code that is multi-platform to operator on **multi-platform devices including windows, mac, and linux**.
- **Use the Module System** for all major application components to ensure proper lifecycle management and architectural consistency.

## Development Guidelines

* **Language:** C++20/C++23 with modern features
* **Architecture:** Modular design using the Module base class and ModuleStage management system
* **Coding Style:** Adhere to the project's coding style guidelines (refer to `coding-style.instructions.md`).
* **Error Handling:** Implement proper error handling, including checking for null pointers and potential exceptions.
* **Memory Management:** Use appropriate memory management techniques to prevent memory leaks, especially when working with the graphics renderer. Managing the memory can be done through the standard library templates (std) and the custom memory code provided for use in this project (refer to `memory-system.instructions.md`).
* **Efficiency:** Prioritize performance and optimize code for large models and complex scenes involving multiple light sources with multiple render passes, texture sets, and PBR materials.
* **Robustness:** Handle edge cases and invalid input gracefully.
* **Unit Tests:** Implement comprehensive unit tests for all core functionalities using the Catch2 framework.
* **Reusability:** For smart pointers, unique pointers, and weak pointers use the custom pointer template located in the `pointers.h` file which was developed for these uses and includes reference counting (refer to `smartpointers.instructions.md`).
* **Logging:** Please utilize the custom logging implementation for runtime log information as it utilizes more detailed asserts, logging macros, and saves the log entries out to a file to better review (refer to `logging-system.instructions.md`)

## Module System Architecture

### Core Module Principles

**Always inherit from the Module base class for application components:**

```cpp
class CustomModule : public Module
{
public:
    explicit CustomModule(const std::string& name = "CustomModule")
        : Module(name)
    {
    }
    
    // Implement lifecycle methods
    void OnAttach() override
    {
        SEDX_CORE_INFO("Initializing {}", GetName());
        // Initialize resources using CreateRef<T>()
        // Register for events/callbacks
    }
    
    void OnDetach() override
    {
        SEDX_CORE_INFO("Cleaning up {}", GetName());
        // Cleanup is automatic with smart pointers
        // Unregister from events/callbacks
    }
    
    void OnUpdate() override
    {
        if (!m_IsEnabled) return;
        SEDX_PROFILE_SCOPE("CustomModule::OnUpdate");
        // Per-frame logic here
    }
    
    void OnUIRender() override
    {
        // ImGui rendering for debug/UI panels
    }
    
    void OnEvent() override
    {
        // Handle application events
    }
    
private:
    bool m_IsEnabled = true;
};
```

### ModuleStage Management

**Use ModuleStage for proper module ordering and lifecycle:**

```cpp
void InitializeModules()
{
    // Core modules first (use PushModule)
    auto terrainModule = CreateRef<TerrainModule>();
    m_ModuleStage.PushModule(terrainModule.get());
    
    // UI overlays last (use PushOverlay)  
    auto debugOverlay = CreateRef<DebugOverlay>();
    m_ModuleStage.PushOverlay(debugOverlay.get());
    
    // Store references for lifetime management
    m_Modules.push_back(terrainModule);
    m_Modules.push_back(debugOverlay);
    
    // Attach all modules
    for (auto* module : m_ModuleStage)
    {
        module->OnAttach();
    }
}
```

### Memory Management with Smart Pointers

**Always use the custom smart pointer system:**

```cpp
// Creation - use CreateRef<T>() for new objects
auto texture = CreateRef<Texture2D>("path/to/texture.png");
auto buffer = CreateRef<VertexBuffer>(bufferSize);

// Storage - use Ref<T> for ownership
Ref<Mesh> m_Mesh;
Ref<Shader> m_Shader;

// Weak references - use WeakRef<T> for non-owning references
WeakRef<Camera> m_CameraRef;

// Reset - use .Reset() to release references
texture.Reset();

// Casting - use .As<T>() for static cast, .DynamicCast<T>() for dynamic cast
auto derivedPtr = basePtr.As<DerivedClass>();
auto safeCast = basePtr.DynamicCast<DerivedClass>(); // Returns nullptr if cast fails
```

### Logging System Usage

**Use tagged logging for better categorization:**

```cpp
// Core system logging
SEDX_CORE_INFO("General information");
SEDX_CORE_WARN("Warning message");  
SEDX_CORE_ERROR("Error occurred");

// Tagged logging (preferred)
SEDX_CORE_INFO_TAG("MODULE", "Module '{}' initialized", moduleName);
SEDX_CORE_ERROR_TAG("VULKAN", "Vulkan error: {}", errorCode);
SEDX_CORE_WARN_TAG("MEMORY", "High memory usage: {} MB", memUsage);

// Module-specific logging in lifecycle methods
void OnAttach() override
{
    SEDX_CORE_INFO("=== Initializing {} ===", GetName());
    try
    {
        InitializeResources();
        SEDX_CORE_INFO_TAG("INIT", "Resources loaded for {}", GetName());
    }
    catch (const std::exception& e)
    {
        SEDX_CORE_ERROR_TAG("INIT", "Failed to load resources: {}", e.what());
        throw;
    }
}
```

### Error Handling and Assertions

**Use the assertion system for validation:**

```cpp
// Core assertions - use in system/engine code
SEDX_CORE_ASSERT(condition, "Error message with {}", formatArgs);
SEDX_CORE_ASSERT(pointer != nullptr, "Null pointer detected");

// Application assertions - use in application/editor code  
SEDX_ASSERT(condition, "Application assertion failed");

// Verification - continues in release builds
SEDX_CORE_VERIFY(resource->IsValid(), "Resource validation failed");
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

## Documentation Guidelines:

* All documentation uses the Doxygen style format.
* Please document all functions, enums, templates, structs, and classes with short but descriptive and helpful documentation and use.

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
        /// Get the allocator from the current device
        const VmaAllocator vmaAllocator = device->GetMemoryAllocator();

    /// Add transfer destination flag for vertex buffers
    if (usage & BufferUsage::Vertex)
        usage |= BufferUsage::TransferDst;

    /// Add transfer destination flag for index buffers
    if (usage & BufferUsage::Index)
        usage |= BufferUsage::TransferDst;

    /// Handle storage buffers - add address flag and align size
    if (usage & BufferUsage::Storage)
    {
        /// Align storage buffer size to minimum required alignment
        usage |= BufferUsage::Address;
            size += size % device->GetPhysicalDevice()->GetDeviceProperties().limits.minStorageBufferOffsetAlignment;
    }

    /// Handle acceleration structure input buffers
    if (usage & BufferUsage::AccelerationStructureInput)
    {
        usage |= BufferUsage::Address;
        usage |= BufferUsage::TransferDst;
    }

    /// Handle acceleration structure buffers
    if (usage & BufferUsage::AccelerationStructure)
        usage |= BufferUsage::Address;

    /// Create buffer resource
    const Ref<BufferResource> resource = CreateRef<BufferResource>();

    /// Configure buffer creation info
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = static_cast<VkBufferUsageFlagBits>(usage);
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    /// Configure memory allocation info
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    /// Enable memory mapping for CPU-accessible buffers
    if (memory & CPU)
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    /// Create the buffer with VMA
    SEDX_ASSERT(vmaCreateBuffer(vmaAllocator, &bufferInfo, &allocInfo, &resource->buffer, &resource->allocation, nullptr));

    /// Create and populate the buffer wrapper
    Buffer buffer = {
        .resource = resource,
        .size = size,
        .usage = usage,
        .memory = memory,
    };

    /// Handle storage buffer descriptors for bindless access
    if (usage & BufferUsage::Storage)
    {
        //BindlessResources bindlessData;
        /// Get a resource ID from the available pool
        resource->resourceID = ImageID::availBufferRID.back();
        ImageID::availBufferRID.pop_back();

        /// Set up descriptor info for the storage buffer
        VkDescriptorBufferInfo descriptorInfo;
        VkWriteDescriptorSet write = {};
        descriptorInfo.buffer = resource->buffer;
        descriptorInfo.offset = 0;
        descriptorInfo.range = size;

        /// Configure descriptor write operation
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        //write.dstSet = bindlessData.bindlessDescriptorSet;
        write.dstBinding = 1;
        write.dstArrayElement = buffer.resource->resourceID;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &descriptorInfo;

        /// Update descriptor set with buffer info
            vkUpdateDescriptorSets(device->GetDevice(), 1, &write, 0, nullptr);
        }

    return buffer;
}

```

## Dependencies:

* **Vulkan Graphics API:** The core graphics library for rendering the various scenes, UI, and assets in the application.
* **GLFW3:** This Scenery Editor X utilizes GLFW3/GLFW for its multi-platform windowing and interaction library.
* **ImGui:** The library for creating and managing the GUI elements and interactions in the application. Used for the multiple viewports and ImGui panel docking.
* **ImGuizmo:** ImGizmo is a small (.h and .cpp) library built ontop of Dear ImGui that allow you to manipulate(Rotate & translate at the moment) 4x4 float matrices. No other dependancies. Coded with Immediate Mode (IM) philosophy in mind.
* **libconfig:** C/C++ library for processing structured configuration files. Scenery Editor X uses this library for creating, reading, and structuring the user settings, system settings, and core application settings. This library is to be use with any operation requring reading and writing configuration data to and from `.cfg` files.
* **Catch2:**  Used for writing and running unit tests.
* **X-PlaneSceneryLibrary:** This library contains the data formating and layout of the multiple custom file formats that X-Plane uses.

## UI Development with ImGui

**Create debug UI for modules:**

```cpp
class UIModule : public Module
{
public:
    void OnUIRender() override
    {
        if (m_ShowDebugWindow)
        {
            ImGui::Begin("Module Debug");
            
            ImGui::Text("Module: %s", GetName().c_str());
            ImGui::Checkbox("Enabled", &m_IsEnabled);
            
            if (ImGui::CollapsingHeader("Settings"))
            {
                ImGui::SliderFloat("Update Rate", &m_UpdateRate, 1.0f, 60.0f);
                ImGui::ColorEdit3("Color", m_Color);
            }
            
            if (ImGui::Button("Reset Module"))
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

### Configuration Management

**Handle module configuration with JSON:**

```cpp
class ConfigurableModule : public Module
{
public:
    struct Config
    {
        bool enabled = true;
        float updateRate = 60.0f;
        std::string resourcePath = "assets/";
        
        void ToJSON(nlohmann::json& j) const
        {
            j["enabled"] = enabled;
            j["updateRate"] = updateRate;
            j["resourcePath"] = resourcePath;
        }
        
        void FromJSON(const nlohmann::json& j)
        {
            enabled = j.value("enabled", true);
            updateRate = j.value("updateRate", 60.0f);
            resourcePath = j.value("resourcePath", std::string("assets/"));
        }
    };
    
    void OnAttach() override
    {
        LoadConfiguration();
        ApplyConfiguration();
    }
    
    void OnDetach() override
    {
        SaveConfiguration();
    }
    
private:
    Config m_Config;
    
    void LoadConfiguration()
    {
        const auto configPath = fmt::format("config/{}.json", GetName());
        try
        {
            if (std::filesystem::exists(configPath))
            {
                std::ifstream file(configPath);
                nlohmann::json j;
                file >> j;
                m_Config.FromJSON(j);
                SEDX_CORE_INFO_TAG("CONFIG", "Loaded config for {}", GetName());
            }
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_WARN_TAG("CONFIG", "Failed to load config: {}", e.what());
        }
    }
};
```

## Code Style Guidelines

### Naming Conventions

```cpp
// Classes: PascalCase
class TerrainRenderer : public Module { };

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

### File Organization

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
#include <SceneryEditorX/core/modules/module.h>
#include <SceneryEditorX/core/pointers.h>
#include <SceneryEditorX/logging/logging.hpp>

namespace SceneryEditorX
{
    class CustomModule : public Module
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

## Key Reminders for Copilot

1. **Always use the Module base class** for application components
2. **Prefer PushModule() for core systems**, PushOverlay() for UI
3. **Use CreateRef<T>() for object creation**, never raw new/delete
4. **Include profiling scopes** in performance-critical code
5. **Use tagged logging** for better categorization
6. **Implement OnAttach/OnDetach pairs** for proper lifecycle management
7. **Follow RAII principles** for resource management
8. **Use assertions** for development-time validation
9. **Include debug UI** for runtime inspection when appropriate
10. **Handle errors gracefully** with proper logging and recovery

## Module Development Patterns

### Resource Management in Modules

```cpp
class ResourceModule : public Module
{
public:
    void OnAttach() override
    {
        // Initialize resources - automatic cleanup with smart pointers
        m_Texture = CreateRef<Texture2D>("assets/texture.png");
        m_Mesh = CreateRef<Mesh>("assets/model.obj");
        
        // Validate resource loading
        SEDX_CORE_ASSERT(m_Texture->IsValid(), "Failed to load texture");
        SEDX_CORE_ASSERT(m_Mesh->IsValid(), "Failed to load mesh");
    }
    
    void OnDetach() override
    {
        // Resources cleaned up automatically by smart pointers
        m_Texture.Reset();
        m_Mesh.Reset();
    }
    
private:
    Ref<Texture2D> m_Texture;
    Ref<Mesh> m_Mesh;
};
```

### Error Handling Patterns

```cpp
class RobustModule : public Module
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

### Event Handling in Modules

```cpp
class EventHandlerModule : public Module
{
public:
    void OnEvent() override
    {
        // Handle different event types through application's event system
        if (m_ShouldProcessEvents)
        {
            ProcessModuleSpecificEvents();
        }
    }
    
    void OnKeyPressed(const KeyPressedEvent& event)
    {
        if (event.GetKeyCode() == KeyCode::F1)
        {
            m_ShowDebugWindow = !m_ShowDebugWindow;
            event.SetConsumed(true); // Prevent other modules from processing
        }
    }
    
private:
    bool m_ShouldProcessEvents = true;
    bool m_ShowDebugWindow = false;
};
```

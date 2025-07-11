# Scenery Editor X - Module System Documentation

## Overview

The Module System in Scenery Editor X provides a flexible, component-based architecture for organizing different functional components of the application. This system enables modular development where each component can be attached, detached, and updated independently, promoting code reusability, maintainability, and clear separation of concerns.

The system consists of two primary components:

1. **Module Base Class** - The foundational interface for all modular components
2. **ModuleStage Manager** - The container and lifecycle manager for Module instances

This documentation is designed for GitHub Copilot instances and agent modes to understand and effectively utilize the existing module framework while maintaining adherence to project standards.

---

## Table of Contents

1. [Module Base Class Documentation](#module-base-class-documentation)
2. [ModuleStage Manager Documentation](#modulestage-manager-documentation)
3. [Implementation Examples](#implementation-examples)
4. [Best Practices &amp; Patterns](#best-practices--patterns)
5. [Integration Guidelines](#integration-guidelines)
6. [Error Handling &amp; Debugging](#error-handling--debugging)

---

## Module Base Class Documentation

### Files: `module.h` & `module.cpp`

**Location**: `source/SceneryEditorX/core/modules/`

The `Module` class serves as the foundational base class for all modular components in Scenery Editor X. It provides a standardized interface for lifecycle management, frame-based updates, UI rendering, and event handling.

### Class Definition

```cpp
/**
 * @class Module
 * @brief Base Module class representing a logical section of the application.
 * 
 * Modules are used to organize different functional components of the application
 * that can be attached, detached and updated independently. Each module can handle
 * its own rendering, events, and update logic.
 */
class Module : public RefCounted
{
public:
    /**
     * @brief Construct a modular component that can be plugged into the application.
     * @param name The name of the module for identification and debugging.
     */
    explicit Module(const std::string &name = "Module");
  
    /**
     * @brief Virtual destructor ensures proper cleanup of derived classes.
     */
    virtual ~Module() override;
  
    // Lifecycle Management
    virtual void OnAttach() {}
    virtual void OnDetach() {}
  
    // Frame-based Operations
    virtual void OnUpdate() {}
    virtual void OnUIRender() {}
  
    // Event Processing
    virtual void OnEvent() {}

private:
    std::string m_Name; ///< The name of the module.
};
```

### Key Features

#### 1. **Reference Counting Integration**

- **Base Class**: Inherits from `RefCounted` for automatic memory management
- **Memory Safety**: Enables safe sharing and lifetime management across the application
- **Smart Pointers**: Works seamlessly with `Ref<T>` and `WeakRef<T>` system
- **Leak Prevention**: Prevents memory leaks and dangling pointers through reference counting

```cpp
// Example: Creating and managing Module instances
auto terrainModule = CreateRef<TerrainModule>("TerrainRenderer");
auto lightingModule = CreateRef<LightingModule>("SceneLighting");

// Modules are automatically cleaned up when references are released
```

#### 2. **Lifecycle Management**

- **OnAttach()**: Called when module is added to the application

  - Initialize module-specific resources
  - Set up dependencies and connections
  - Register for events or callbacks
- **OnDetach()**: Called when module is removed from the application

  - Clean up module-specific resources
  - Unregister from events or callbacks
  - Save persistent state if needed

```cpp
class ExampleModule : public Module
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO("Initializing ExampleModule");
      
        // Initialize resources
        m_Buffer = CreateRef<VertexBuffer>(bufferSize);
        m_Texture = CreateRef<Texture2D>("assets/textures/example.png");
      
        // Register for events
        EventSystem::RegisterCallback(EventType::KeyPressed, 
                                    BIND_EVENT_FN(OnKeyPressed));
    }
  
    void OnDetach() override
    {
        SEDX_CORE_INFO("Cleaning up ExampleModule");
      
        // Cleanup resources (automatic with smart pointers)
        m_Buffer.Reset();
        m_Texture.Reset();
      
        // Unregister from events
        EventSystem::UnregisterCallback(EventType::KeyPressed, this);
    }
};
```

#### 3. **Frame-based Operations**

- **OnUpdate()**: Called every frame for module logic

  - Update game state
  - Process input that affects the module
  - Perform per-frame calculations
  - Handle animations and transitions
- **OnUIRender()**: Called every frame for ImGui rendering

  - Render debug UI
  - Display module-specific controls
  - Show performance metrics
  - Provide user interaction interfaces

```cpp
class PhysicsModule : public Module
{
public:
    void OnUpdate() override
    {
        SEDX_PROFILE_SCOPE("PhysicsModule::OnUpdate");
      
        // Update physics simulation
        float deltaTime = Application::GetDeltaTime();
        m_PhysicsWorld->Step(deltaTime);
      
        // Process collision events
        ProcessCollisions();
      
        // Update dynamic objects
        UpdateDynamicBodies();
    }
  
    void OnUIRender() override
    {
        if (m_ShowDebugUI)
        {
            ImGui::Begin("Physics Debug");
          
            ImGui::Text("Active Bodies: %d", m_PhysicsWorld->GetBodyCount());
            ImGui::Text("Contacts: %d", m_PhysicsWorld->GetContactCount());
          
            if (ImGui::SliderFloat("Gravity", &m_Gravity, -20.0f, 20.0f))
            {
                m_PhysicsWorld->SetGravity({0.0f, m_Gravity});
            }
          
            ImGui::End();
        }
    }
};
```

#### 4. **Event Processing**

- **OnEvent()**: Called for event processing
  - Handle input events
  - Process application events
  - Manage module-specific event responses
  - Enable event consumption to prevent propagation

```cpp
class InputModule : public Module
{
public:
    void OnEvent() override
    {
        // Handle events through the event dispatcher
        // Events are typically passed through the application's event system
      
        // Example of event handling pattern:
        if (auto* keyEvent = Event::Cast<KeyPressedEvent>(currentEvent))
        {
            OnKeyPressed(*keyEvent);
        }
        else if (auto* mouseEvent = Event::Cast<MouseButtonPressedEvent>(currentEvent))
        {
            OnMousePressed(*mouseEvent);
        }
    }
  
private:
    void OnKeyPressed(const KeyPressedEvent& event)
    {
        if (event.GetKeyCode() == KeyCode::Escape)
        {
            // Handle escape key
            event.SetConsumed(true); // Prevent other modules from processing
        }
    }
};
```

#### 5. **Module Identification**

- **Naming System**: Each module has a unique name for debugging and identification
- **Logging Support**: Name is used in logging and profiling systems
- **Runtime Queries**: Enables finding specific modules at runtime

```cpp
// Module creation with descriptive names
auto module = CreateRef<CustomModule>("SceneryRenderer");

// Logging with module identification
SEDX_CORE_INFO("Module '{}' initialized successfully", module->GetName());
```

### Virtual Method Implementation Guidelines

When implementing custom modules, follow these patterns:

#### **OnAttach() Implementation**

```cpp
void CustomModule::OnAttach() override
{
    SEDX_CORE_INFO("Attaching module: {}", GetName());
  
    // 1. Initialize module-specific resources
    InitializeResources();
  
    // 2. Set up dependencies
    ValidateDependencies();
  
    // 3. Register for events/callbacks
    RegisterCallbacks();
  
    // 4. Load configuration/settings
    LoadSettings();
  
    SEDX_CORE_INFO("Module '{}' attached successfully", GetName());
}
```

#### **OnDetach() Implementation**

```cpp
void CustomModule::OnDetach() override
{
    SEDX_CORE_INFO("Detaching module: {}", GetName());
  
    // 1. Save state/configuration
    SaveSettings();
  
    // 2. Unregister from events/callbacks
    UnregisterCallbacks();
  
    // 3. Clean up resources (automatic with smart pointers)
    CleanupResources();
  
    SEDX_CORE_INFO("Module '{}' detached successfully", GetName());
}
```

#### **OnUpdate() Implementation**

```cpp
void CustomModule::OnUpdate() override
{
    // Early exit if module is disabled
    if (!m_IsEnabled) return;
  
    SEDX_PROFILE_SCOPE("CustomModule::OnUpdate");
  
    // 1. Update module state
    UpdateModuleState();
  
    // 2. Process frame-specific logic
    ProcessFrameLogic();
  
    // 3. Handle time-based operations
    ProcessTimers();
  
    // 4. Update dependent systems
    UpdateDependentSystems();
}
```

---

## ModuleStage Manager Documentation

### Files: `module_stage.h` & `module_stage.cpp`

**Location**: `source/SceneryEditorX/core/modules/`

The `ModuleStage` class is a sophisticated container and manager for Module instances. It provides an ordered collection system that distinguishes between regular modules and overlay modules, enabling proper layering and execution order management.

### Class Definition

```cpp
/**
 * @class ModuleStage
 * @brief Container and manager for Module instances with ordered execution.
 * 
 * Manages module lifecycle and execution order, distinguishing between
 * regular modules and overlay modules for proper layering.
 */
class ModuleStage
{
public:
    ModuleStage() = default;
    ~ModuleStage() = default;

    // Module Management
    void PushModule(Module *module);
    void PushOverlay(Module *overlay);
    void PopModule(Module *module);
    void PopOverlay(Module *overlay);

    // Access Operations
    Module *operator[](size_t index);
    const Module *operator[](size_t index) const;
    [[nodiscard]] size_t Size() const { return moduleStage.size(); }

    // Iterator Support
    std::vector<Module*>::iterator begin() { return moduleStage.begin(); }
    std::vector<Module*>::iterator end() { return moduleStage.end(); }

private:
    std::vector<Module*> moduleStage;
    unsigned int moduleInsertIndex = 0;
};
```

### Architecture & Execution Order

The ModuleStage uses a two-tier architecture:

```
┌─────────────────────────────────────────────────────────────┐
│                    ModuleStage Layout                       │
├─────────────────────────────────────────────────────────────┤
│ Regular Modules (Index 0 to moduleInsertIndex-1)           │
│ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐            │
│ │   Module 0  │ │   Module 1  │ │   Module 2  │   ...      │
│ │  (Terrain)  │ │ (Physics)   │ │ (Audio)     │            │
│ └─────────────┘ └─────────────┘ └─────────────┘            │
├─────────────────────────────────────────────────────────────┤
│ Overlay Modules (Index moduleInsertIndex to end)           │
│ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐            │
│ │  Overlay 0  │ │  Overlay 1  │ │  Overlay 2  │   ...      │
│ │ (Debug UI)  │ │ (Scene Panel│ │ (Inspector) │            │
│ └─────────────┘ └─────────────┘ └─────────────┘            │
└─────────────────────────────────────────────────────────────┘
```

**Execution Order**:

- **Updates**: Regular modules → Overlay modules (forward order)
- **Events**: Overlay modules → Regular modules (reverse order)
- **Rendering**: Regular modules → Overlay modules (forward order)

### Method Documentation

#### **void PushModule(Module *module)**

**Purpose**: Adds a regular module to the stage at the current insertion index.

**Behavior**:

- Inserts module at `moduleInsertIndex` position
- Increments `moduleInsertIndex` to maintain separation
- Regular modules execute before overlays

**Use Cases**: Core application modules that need to execute before UI overlays.

```cpp
// Example: Adding core modules
auto terrainModule = CreateRef<TerrainModule>();
auto physicsModule = CreateRef<PhysicsModule>();

moduleStage.PushModule(terrainModule.get());  // Index 0
moduleStage.PushModule(physicsModule.get());  // Index 1
// moduleInsertIndex is now 2
```

**Implementation Details**:

```cpp
void ModuleStage::PushModule(Module *module)
{
    // Insert at current insertion index and increment
    moduleStage.emplace(moduleStage.begin() + moduleInsertIndex, module);
    moduleInsertIndex++;
}
```

#### **void PushOverlay(Module *overlay)**

**Purpose**: Adds an overlay module to the end of the stage.

**Behavior**:

- Always appends to the end of the vector
- Overlays render on top of regular modules
- Does not affect `moduleInsertIndex`

**Use Cases**: UI panels, debug overlays, HUD elements.

```cpp
// Example: Adding UI overlays
auto debugOverlay = CreateRef<DebugOverlay>();
auto sceneHierarchy = CreateRef<SceneHierarchyPanel>();

moduleStage.PushOverlay(debugOverlay.get());     // Appended to end
moduleStage.PushOverlay(sceneHierarchy.get());   // Appended to end
```

**Implementation Details**:

```cpp
void ModuleStage::PushOverlay(Module *overlay)
{
    // Simply append to the end
    moduleStage.emplace_back(overlay);
}
```

#### **void PopModule(Module *module) / void PopOverlay(Module *overlay)**

**Purpose**: Removes specified modules from the stage.

**Behavior**:

- Searches for the module using `std::ranges::find`
- Removes if found, logs error if not found
- `PopModule` decrements `moduleInsertIndex`
- `PopOverlay` does not affect `moduleInsertIndex`

```cpp
// Example: Removing modules
moduleStage.PopModule(terrainModule.get());    // Removes and decrements index
moduleStage.PopOverlay(debugOverlay.get());    // Removes from end
```

**Implementation Details**:

```cpp
void ModuleStage::PopModule(Module *module)
{
    if (const auto it = std::ranges::find(moduleStage, module); it != moduleStage.end())
    {
        moduleStage.erase(it);
        moduleInsertIndex--;
    }
    else
    {
        SEDX_CORE_ERROR("Module not found in stage");
    }
}

void ModuleStage::PopOverlay(Module *overlay)
{
    if (const auto it = std::ranges::find(moduleStage, overlay); it != moduleStage.end())
    {
        moduleStage.erase(it);
    }
    else
    {
        SEDX_CORE_ERROR("Overlay not found in stage");
    }
}
```

#### **Access and Iteration Methods**

**Array Access**:

```cpp
Module *operator[](size_t index)
{
    SEDX_CORE_ASSERT(index >= 0 && index < moduleStage.size());
    return moduleStage[index];
}
```

**Iterator Support**:

```cpp
// Range-based for loop support
for (auto* module : moduleStage)
{
    module->OnUpdate();
}

// Traditional iterator usage
for (auto it = moduleStage.begin(); it != moduleStage.end(); ++it)
{
    (*it)->OnUIRender();
}
```

---

## Implementation Examples

### Example 1: Complete Application Module Setup

```cpp
class SceneryEditorXApplication
{
public:
    void Initialize()
    {
        SetupCoreModules();
        SetupRenderingModules();
        SetupUIOverlays();
        AttachAllModules();
      
        SEDX_CORE_INFO("Application initialized with {} modules", 
                       m_ModuleStage.Size());
    }
  
    void Shutdown()
    {
        DetachAllModules();
        ClearModuleStage();
    }
  
    void Update()
    {
        SEDX_PROFILE_SCOPE("Application::Update");
      
        // Update all modules in order (modules first, then overlays)
        for (auto* module : m_ModuleStage)
        {
            module->OnUpdate();
        }
    }
  
    void Render()
    {
        SEDX_PROFILE_SCOPE("Application::Render");
      
        // Render all modules in order
        for (auto* module : m_ModuleStage)
        {
            module->OnUIRender();
        }
    }
  
    void ProcessEvents(Event& event)
    {
        SEDX_PROFILE_SCOPE("Application::ProcessEvents");
      
        // Process events in reverse order (overlays first)
        for (auto it = m_ModuleStage.end() - 1; it >= m_ModuleStage.begin(); --it)
        {
            auto* module = *it;
            module->OnEvent();
          
            if (event.IsConsumed())
            {
                break; // Stop processing if event was consumed
            }
        }
    }

private:
    ModuleStage m_ModuleStage;
  
    // Module storage for lifetime management
    Ref<TerrainModule> m_TerrainModule;
    Ref<PhysicsModule> m_PhysicsModule;
    Ref<LightingModule> m_LightingModule;
    Ref<AudioModule> m_AudioModule;
    Ref<DebugOverlay> m_DebugOverlay;
    Ref<SceneHierarchy> m_SceneHierarchy;
  
    void SetupCoreModules()
    {
        // Create core application modules
        m_TerrainModule = CreateRef<TerrainModule>("TerrainSystem");
        m_PhysicsModule = CreateRef<PhysicsModule>("PhysicsWorld");
        m_LightingModule = CreateRef<LightingModule>("LightingRenderer");
        m_AudioModule = CreateRef<AudioModule>("AudioEngine");
      
        // Add to stage in dependency order
        m_ModuleStage.PushModule(m_TerrainModule.get());   // Index 0
        m_ModuleStage.PushModule(m_PhysicsModule.get());   // Index 1
        m_ModuleStage.PushModule(m_LightingModule.get());  // Index 2
        m_ModuleStage.PushModule(m_AudioModule.get());     // Index 3
      
        SEDX_CORE_INFO("Core modules added to stage");
    }
  
    void SetupUIOverlays()
    {
        // Create UI overlay modules
        m_DebugOverlay = CreateRef<DebugOverlay>("DebugUI");
        m_SceneHierarchy = CreateRef<SceneHierarchy>("ScenePanel");
      
        // Add as overlays (appended to end)
        m_ModuleStage.PushOverlay(m_DebugOverlay.get());
        m_ModuleStage.PushOverlay(m_SceneHierarchy.get());
      
        SEDX_CORE_INFO("UI overlays added to stage");
    }
  
    void AttachAllModules()
    {
        for (auto* module : m_ModuleStage)
        {
            module->OnAttach();
        }
    }
  
    void DetachAllModules()
    {
        // Detach in reverse order for proper cleanup
        for (auto it = m_ModuleStage.end() - 1; it >= m_ModuleStage.begin(); --it)
        {
            (*it)->OnDetach();
        }
    }
};
```

### Example 2: Dynamic Module Management

```cpp
class ModuleManager
{
public:
    ModuleManager(ModuleStage& stage) : m_ModuleStage(stage) {}
  
    template<typename ModuleType, typename... Args>
    ModuleType* CreateAndAddModule(Args&&... args)
    {
        auto module = CreateRef<ModuleType>(std::forward<Args>(args)...);
        ModuleType* modulePtr = module.get();
      
        // Store for lifetime management
        m_ManagedModules.push_back(module);
      
        // Add to stage and attach
        m_ModuleStage.PushModule(modulePtr);
        modulePtr->OnAttach();
      
        SEDX_CORE_INFO("Created and added module: {}", typeid(ModuleType).name());
        return modulePtr;
    }
  
    template<typename OverlayType, typename... Args>
    OverlayType* CreateAndAddOverlay(Args&&... args)
    {
        auto overlay = CreateRef<OverlayType>(std::forward<Args>(args)...);
        OverlayType* overlayPtr = overlay.get();
      
        // Store for lifetime management
        m_ManagedModules.push_back(overlay);
      
        // Add to stage and attach
        m_ModuleStage.PushOverlay(overlayPtr);
        overlayPtr->OnAttach();
      
        SEDX_CORE_INFO("Created and added overlay: {}", typeid(OverlayType).name());
        return overlayPtr;
    }
  
    template<typename ModuleType>
    ModuleType* FindModule()
    {
        for (auto* module : m_ModuleStage)
        {
            if (auto* typedModule = dynamic_cast<ModuleType*>(module))
                return typedModule;
        }
        return nullptr;
    }
  
    void RemoveModule(Module* module)
    {
        // Detach first
        module->OnDetach();
      
        // Remove from stage
        m_ModuleStage.PopModule(module);
      
        // Remove from managed storage
        RemoveFromManagedStorage(module);
      
        SEDX_CORE_INFO("Removed module from stage");
    }

private:
    ModuleStage& m_ModuleStage;
    std::vector<Ref<Module>> m_ManagedModules;
  
    void RemoveFromManagedStorage(Module* module)
    {
        m_ManagedModules.erase(
            std::remove_if(m_ManagedModules.begin(), m_ManagedModules.end(),
                [module](const Ref<Module>& managedModule) {
                    return managedModule.get() == module;
                }),
            m_ManagedModules.end()
        );
    }
};
```

### Example 3: Custom Module Implementation

```cpp
class SceneryRenderModule : public Module
{
public:
    explicit SceneryRenderModule(const std::string& name = "SceneryRenderer")
        : Module(name)
        , m_IsEnabled(true)
        , m_RenderDistance(1000.0f)
    {
    }
  
    void OnAttach() override
    {
        SEDX_CORE_INFO("Initializing Scenery Render Module");
      
        // Initialize rendering resources
        m_Shader = CreateRef<Shader>("assets/shaders/scenery.glsl");
        m_Camera = CreateRef<Camera>();
        m_SceneryManager = CreateRef<SceneryManager>();
      
        // Set up render targets
        m_RenderTarget = CreateRef<Framebuffer>(1920, 1080);
      
        // Register for events
        EventSystem::Subscribe(EventType::WindowResize, 
                              BIND_EVENT_FN(OnWindowResize));
      
        SEDX_CORE_INFO("Scenery Render Module initialized successfully");
    }
  
    void OnDetach() override
    {
        SEDX_CORE_INFO("Cleaning up Scenery Render Module");
      
        // Unregister from events
        EventSystem::Unsubscribe(EventType::WindowResize, this);
      
        // Resources automatically cleaned up by smart pointers
        m_Shader.Reset();
        m_Camera.Reset();
        m_SceneryManager.Reset();
        m_RenderTarget.Reset();
      
        SEDX_CORE_INFO("Scenery Render Module cleaned up");
    }
  
    void OnUpdate() override
    {
        if (!m_IsEnabled) return;
      
        SEDX_PROFILE_SCOPE("SceneryRenderModule::OnUpdate");
      
        // Update camera
        m_Camera->OnUpdate();
      
        // Cull scenery objects
        auto visibleObjects = m_SceneryManager->CullObjects(
            m_Camera->GetViewFrustum(), m_RenderDistance);
      
        // Update LOD levels
        m_SceneryManager->UpdateLOD(m_Camera->GetPosition(), visibleObjects);
      
        // Prepare render commands
        PrepareRenderCommands(visibleObjects);
    }
  
    void OnUIRender() override
    {
        if (m_ShowDebugUI)
        {
            ImGui::Begin("Scenery Renderer");
          
            ImGui::Checkbox("Enabled", &m_IsEnabled);
            ImGui::SliderFloat("Render Distance", &m_RenderDistance, 100.0f, 5000.0f);
          
            if (ImGui::CollapsingHeader("Statistics"))
            {
                ImGui::Text("Visible Objects: %d", m_LastVisibleCount);
                ImGui::Text("Draw Calls: %d", m_LastDrawCalls);
                ImGui::Text("Triangles: %d", m_LastTriangleCount);
            }
          
            if (ImGui::CollapsingHeader("Camera"))
            {
                auto pos = m_Camera->GetPosition();
                ImGui::Text("Position: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
              
                auto dir = m_Camera->GetForward();
                ImGui::Text("Forward: %.2f, %.2f, %.2f", dir.x, dir.y, dir.z);
            }
          
            ImGui::End();
        }
    }
  
    void OnEvent() override
    {
        // Handle scenery-specific events
        // Events are typically dispatched through the application's event system
    }
  
    // Module-specific methods
    void EnableDebugUI(bool enable) { m_ShowDebugUI = enable; }
    void SetRenderDistance(float distance) { m_RenderDistance = distance; }
  
    const Ref<Camera>& GetCamera() const { return m_Camera; }
    const SceneryManager& GetSceneryManager() const { return *m_SceneryManager; }

private:
    // Core components
    Ref<Shader> m_Shader;
    Ref<Camera> m_Camera;
    Ref<SceneryManager> m_SceneryManager;
    Ref<Framebuffer> m_RenderTarget;
  
    // Module state
    bool m_IsEnabled;
    bool m_ShowDebugUI = false;
    float m_RenderDistance;
  
    // Statistics
    uint32_t m_LastVisibleCount = 0;
    uint32_t m_LastDrawCalls = 0;
    uint32_t m_LastTriangleCount = 0;
  
    void PrepareRenderCommands(const std::vector<SceneryObject*>& visibleObjects)
    {
        SEDX_PROFILE_SCOPE("SceneryRenderModule::PrepareRenderCommands");
      
        m_LastVisibleCount = static_cast<uint32_t>(visibleObjects.size());
        m_LastDrawCalls = 0;
        m_LastTriangleCount = 0;
      
        for (auto* object : visibleObjects)
        {
            // Submit render commands
            RenderCommand::DrawMesh(object->GetMesh(), object->GetTransform(), m_Shader);
          
            m_LastDrawCalls++;
            m_LastTriangleCount += object->GetMesh()->GetTriangleCount();
        }
    }
  
    void OnWindowResize(const WindowResizeEvent& event)
    {
        // Update render target size
        m_RenderTarget->Resize(event.GetWidth(), event.GetHeight());
      
        // Update camera aspect ratio
        m_Camera->SetAspectRatio(static_cast<float>(event.GetWidth()) / event.GetHeight());
    }
};
```

---

## Best Practices & Patterns

### 1. **Module Lifecycle Management**

#### **Initialization Order**

```cpp
// Correct dependency order
void SetupModules()
{
    // 1. Foundation modules first
    m_ModuleStage.PushModule(CreateRef<ResourceModule>().get());
    m_ModuleStage.PushModule(CreateRef<InputModule>().get());
  
    // 2. Core systems
    m_ModuleStage.PushModule(CreateRef<PhysicsModule>().get());
    m_ModuleStage.PushModule(CreateRef<AudioModule>().get());
  
    // 3. Rendering modules
    m_ModuleStage.PushModule(CreateRef<RenderModule>().get());
    m_ModuleStage.PushModule(CreateRef<SceneryModule>().get());
  
    // 4. UI overlays last
    m_ModuleStage.PushOverlay(CreateRef<DebugUI>().get());
    m_ModuleStage.PushOverlay(CreateRef<EditorUI>().get());
}
```

#### **Cleanup Order**

```cpp
// Proper cleanup sequence
void Shutdown()
{
    // Detach in reverse order
    for (auto it = m_ModuleStage.end() - 1; it >= m_ModuleStage.begin(); --it)
    {
        (*it)->OnDetach();
    }
  
    // Clear the stage
    while (m_ModuleStage.Size() > 0)
    {
        // Remove last module (maintains index integrity)
        auto* lastModule = m_ModuleStage[m_ModuleStage.Size() - 1];
      
        // Determine removal method based on position
        if (IsOverlay(lastModule))
            m_ModuleStage.PopOverlay(lastModule);
        else
            m_ModuleStage.PopModule(lastModule);
    }
}
```

### 2. **Memory Management Patterns**

#### **Smart Pointer Usage**

```cpp
class ModuleContainer
{
public:
    void AddModule(const std::string& name)
    {
        // Create with smart pointer for automatic lifetime management
        auto module = CreateRef<CustomModule>(name);
      
        // Store reference for lifetime
        m_Modules.push_back(module);
      
        // Add raw pointer to stage
        m_ModuleStage.PushModule(module.get());
      
        // Attach after adding to stage
        module->OnAttach();
    }
  
private:
    ModuleStage m_ModuleStage;
    std::vector<Ref<Module>> m_Modules; // Lifetime management
};
```

#### **Resource Management in Modules**

```cpp
class ResourceAwareModule : public Module
{
public:
    void OnAttach() override
    {
        // Use RAII for resource management
        m_Texture = CreateRef<Texture2D>("assets/texture.png");
        m_Buffer = CreateRef<VertexBuffer>(bufferSize);
      
        // Register cleanup callbacks if needed
        ResourceManager::RegisterCleanupCallback(
            [this]() { CleanupResources(); });
    }
  
    void OnDetach() override
    {
        // Resources automatically cleaned up by smart pointers
        // Manual cleanup only if needed for specific cases
        m_Texture.Reset();
        m_Buffer.Reset();
    }
  
private:
    Ref<Texture2D> m_Texture;
    Ref<VertexBuffer> m_Buffer;
};
```

### 3. **Performance Optimization**

#### **Efficient Update Patterns**

```cpp
class OptimizedModule : public Module
{
public:
    void OnUpdate() override
    {
        // Early exit for disabled modules
        if (!m_IsEnabled) return;
      
        // Use profiling scopes for performance monitoring
        SEDX_PROFILE_SCOPE("OptimizedModule::OnUpdate");
      
        // Cache expensive calculations
        if (m_NeedsUpdate)
        {
            UpdateExpensiveCalculations();
            m_NeedsUpdate = false;
        }
      
        // Update only when necessary
        if (ShouldUpdateThisFrame())
        {
            PerformFrameUpdate();
        }
    }
  
private:
    bool m_IsEnabled = true;
    bool m_NeedsUpdate = true;
    uint32_t m_FrameCounter = 0;
  
    bool ShouldUpdateThisFrame()
    {
        // Update every N frames for less critical modules
        return (m_FrameCounter++ % m_UpdateFrequency) == 0;
    }
  
    uint32_t m_UpdateFrequency = 1; // Update every frame by default
};
```

#### **Event Processing Optimization**

```cpp
class EventOptimizedModule : public Module
{
public:
    void OnEvent() override
    {
        // Only process events this module cares about
        if (!ShouldProcessEvent(currentEvent))
            return;
          
        // Fast event type dispatch
        switch (currentEvent.GetType())
        {
            case EventType::KeyPressed:
                OnKeyPressed(static_cast<const KeyEvent&>(currentEvent));
                break;
            case EventType::MouseMoved:
                OnMouseMoved(static_cast<const MouseEvent&>(currentEvent));
                break;
            // ... other event types
        }
    }
  
private:
    // Bitmask for events this module cares about
    EventTypeMask m_InterestedEvents = 
        EventType::KeyPressed | EventType::MouseMoved;
  
    bool ShouldProcessEvent(const Event& event)
    {
        return (m_InterestedEvents & event.GetType()) != 0;
    }
};
```

### 4. **Error Handling & Debugging**

#### **Robust Error Handling**

```cpp
class RobustModule : public Module
{
public:
    void OnAttach() override
    {
        try
        {
            SEDX_CORE_INFO("Initializing {}", GetName());
          
            // Validate dependencies
            if (!ValidateDependencies())
            {
                SEDX_CORE_ERROR("Module '{}' dependencies not met", GetName());
                m_InitializationFailed = true;
                return;
            }
          
            // Initialize resources with error checking
            if (!InitializeResources())
            {
                SEDX_CORE_ERROR("Failed to initialize resources for '{}'", GetName());
                m_InitializationFailed = true;
                return;
            }
          
            m_IsInitialized = true;
            SEDX_CORE_INFO("Module '{}' initialized successfully", GetName());
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR("Exception during '{}' initialization: {}", GetName(), e.what());
            m_InitializationFailed = true;
        }
    }
  
    void OnUpdate() override
    {
        // Skip updates if initialization failed
        if (m_InitializationFailed || !m_IsInitialized)
            return;
          
        try
        {
            // Perform update operations
            PerformUpdate();
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR("Exception in '{}' update: {}", GetName(), e.what());
            // Optionally disable module on repeated errors
            m_ErrorCount++;
            if (m_ErrorCount > MAX_ERRORS)
            {
                SEDX_CORE_WARN("Disabling module '{}' due to repeated errors", GetName());
                m_IsEnabled = false;
            }
        }
    }
  
private:
    bool m_IsInitialized = false;
    bool m_InitializationFailed = false;
    bool m_IsEnabled = true;
    uint32_t m_ErrorCount = 0;
    static constexpr uint32_t MAX_ERRORS = 10;
  
    bool ValidateDependencies()
    {
        // Check for required systems, resources, etc.
        return true; // Implementation specific
    }
  
    bool InitializeResources()
    {
        // Initialize module-specific resources
        return true; // Implementation specific
    }
};
```

#### **Debug Information**

```cpp
class DebuggableModule : public Module
{
public:
    void OnUIRender() override
    {
        if (m_ShowDebugInfo)
        {
            RenderDebugUI();
        }
    }
  
    // Debug interface
    void EnableDebugMode(bool enable) { m_ShowDebugInfo = enable; }
    void DumpState() const
    {
        SEDX_CORE_INFO("=== Module State: {} ===", GetName());
        SEDX_CORE_INFO("Initialized: {}", m_IsInitialized);
        SEDX_CORE_INFO("Enabled: {}", m_IsEnabled);
        SEDX_CORE_INFO("Update Count: {}", m_UpdateCount);
        SEDX_CORE_INFO("Last Update Time: {:.3f}ms", m_LastUpdateTime);
        // ... other state information
    }
  
private:
    bool m_ShowDebugInfo = false;
    uint64_t m_UpdateCount = 0;
    float m_LastUpdateTime = 0.0f;
  
    void RenderDebugUI()
    {
        ImGui::Begin((GetName() + " Debug").c_str());
      
        ImGui::Text("Module: %s", GetName().c_str());
        ImGui::Text("Status: %s", m_IsInitialized ? "Initialized" : "Not Initialized");
        ImGui::Checkbox("Enabled", &m_IsEnabled);
      
        ImGui::Separator();
        ImGui::Text("Performance");
        ImGui::Text("Updates: %llu", m_UpdateCount);
        ImGui::Text("Last Update: %.3f ms", m_LastUpdateTime);
      
        if (ImGui::Button("Dump State to Log"))
        {
            DumpState();
        }
      
        ImGui::End();
    }
};
```

---

## Integration Guidelines

### 1. **Module Communication Patterns**

#### **Direct Module Access**

```cpp
class ModuleCommunicationExample
{
public:
    void Initialize()
    {
        // Store references to frequently accessed modules
        m_TerrainModule = CreateRef<TerrainModule>();
        m_PhysicsModule = CreateRef<PhysicsModule>();
      
        m_ModuleStage.PushModule(m_TerrainModule.get());
        m_ModuleStage.PushModule(m_PhysicsModule.get());
      
        // Set up cross-module dependencies
        m_PhysicsModule->SetTerrainProvider(m_TerrainModule.get());
    }
  
    void UpdateModuleCommunication()
    {
        // Direct access through stored references
        auto terrainHeight = m_TerrainModule->GetHeightAt({100, 100});
        m_PhysicsModule->UpdateGroundHeight(terrainHeight);
    }
  
private:
    ModuleStage m_ModuleStage;
    Ref<TerrainModule> m_TerrainModule;
    Ref<PhysicsModule> m_PhysicsModule;
};
```

#### **Event-Based Communication**

```cpp
class EventBasedModule : public Module
{
public:
    void OnEvent() override
    {
        // Handle custom application events
        if (auto* terrainEvent = Event::Cast<TerrainChangedEvent>(currentEvent))
        {
            OnTerrainChanged(*terrainEvent);
        }
    }
  
private:
    void OnTerrainChanged(const TerrainChangedEvent& event)
    {
        // Respond to terrain changes from other modules
        UpdatePhysicsColliders(event.GetAffectedRegion());
    }
  
    void NotifyTerrainChange(const Region& region)
    {
        // Emit event for other modules
        auto event = TerrainChangedEvent(region);
        EventSystem::Dispatch(event);
    }
};
```

#### **Service Locator Pattern**

```cpp
class ServiceModule : public Module
{
public:
    void OnAttach() override
    {
        // Register services this module provides
        ServiceLocator::RegisterService<ITerrainService>(this);
        ServiceLocator::RegisterService<IHeightProvider>(this);
    }
  
    void OnDetach() override
    {
        // Unregister services
        ServiceLocator::UnregisterService<ITerrainService>();
        ServiceLocator::UnregisterService<IHeightProvider>();
    }
};

// Usage in other modules
class ConsumerModule : public Module
{
public:
    void OnUpdate() override
    {
        // Access services provided by other modules
        if (auto* terrainService = ServiceLocator::GetService<ITerrainService>())
        {
            auto height = terrainService->GetHeightAt({x, z});
            // Use height data
        }
    }
};
```

### 2. **Configuration and Settings**

#### **Module Configuration**

```cpp
class ConfigurableModule : public Module
{
public:
    struct Config
    {
        bool enabled = true;
        float updateFrequency = 60.0f;
        int maxObjects = 1000;
        std::string resourcePath = "assets/";
      
        // Serialization support
        void Serialize(JSON& json) const
        {
            json["enabled"] = enabled;
            json["updateFrequency"] = updateFrequency;
            json["maxObjects"] = maxObjects;
            json["resourcePath"] = resourcePath;
        }
      
        void Deserialize(const JSON& json)
        {
            enabled = json.value("enabled", true);
            updateFrequency = json.value("updateFrequency", 60.0f);
            maxObjects = json.value("maxObjects", 1000);
            resourcePath = json.value("resourcePath", std::string("assets/"));
        }
    };
  
    explicit ConfigurableModule(const Config& config = {})
        : Module("ConfigurableModule")
        , m_Config(config)
    {
    }
  
    void OnAttach() override
    {
        // Load configuration from file
        LoadConfiguration();
      
        // Apply configuration
        ApplyConfiguration();
    }
  
    void OnDetach() override
    {
        // Save configuration
        SaveConfiguration();
    }
  
    // Runtime configuration changes
    void SetConfig(const Config& config)
    {
        m_Config = config;
        ApplyConfiguration();
    }
  
    const Config& GetConfig() const { return m_Config; }

private:
    Config m_Config;
  
    void LoadConfiguration()
    {
        try
        {
            auto configPath = std::format("config/{}.json", GetName());
            if (std::filesystem::exists(configPath))
            {
                std::ifstream file(configPath);
                JSON json;
                file >> json;
                m_Config.Deserialize(json);
              
                SEDX_CORE_INFO("Loaded configuration for module '{}'", GetName());
            }
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_WARN("Failed to load config for '{}': {}", GetName(), e.what());
        }
    }
  
    void SaveConfiguration()
    {
        try
        {
            auto configPath = std::format("config/{}.json", GetName());
            std::filesystem::create_directories(std::filesystem::path(configPath).parent_path());
          
            std::ofstream file(configPath);
            JSON json;
            m_Config.Serialize(json);
            file << json.dump(4);
          
            SEDX_CORE_INFO("Saved configuration for module '{}'", GetName());
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR("Failed to save config for '{}': {}", GetName(), e.what());
        }
    }
  
    void ApplyConfiguration()
    {
        // Apply configuration settings
        // Implementation specific to module
    }
};
```

### 3. **Module Factory Pattern**

#### **Automatic Module Registration**

```cpp
// Module factory for dynamic creation
class ModuleFactory
{
public:
    template<typename ModuleType>
    static void RegisterModule(const std::string& name)
    {
        GetRegistry()[name] = []() -> std::unique_ptr<Module> {
            return std::make_unique<ModuleType>();
        };
    }
  
    static std::unique_ptr<Module> CreateModule(const std::string& name)
    {
        auto& registry = GetRegistry();
        auto it = registry.find(name);
        if (it != registry.end())
        {
            return it->second();
        }
      
        SEDX_CORE_ERROR("Unknown module type: {}", name);
        return nullptr;
    }
  
    static std::vector<std::string> GetRegisteredModules()
    {
        std::vector<std::string> names;
        for (const auto& pair : GetRegistry())
        {
            names.push_back(pair.first);
        }
        return names;
    }
  
private:
    using ModuleCreator = std::function<std::unique_ptr<Module>()>;
    using ModuleRegistry = std::unordered_map<std::string, ModuleCreator>;
  
    static ModuleRegistry& GetRegistry()
    {
        static ModuleRegistry registry;
        return registry;
    }
};

// Auto-registration helper
template<typename ModuleType>
struct ModuleRegistrar
{
    ModuleRegistrar(const std::string& name)
    {
        ModuleFactory::RegisterModule<ModuleType>(name);
    }
};

#define REGISTER_MODULE(ModuleType, Name) \
    static ModuleRegistrar<ModuleType> g_##ModuleType##Registrar(Name)

// Usage in module files:
// REGISTER_MODULE(TerrainModule, "Terrain");
// REGISTER_MODULE(PhysicsModule, "Physics");
```

---

## Error Handling & Debugging

### 1. **Logging Integration**

The module system integrates with the Scenery Editor X logging system using standardized macros:

#### **Core Logging Macros**

```cpp
// Core module logging
SEDX_CORE_TRACE("Module '{}' performing operation", GetName());
SEDX_CORE_INFO("Module '{}' initialized successfully", GetName());
SEDX_CORE_WARN("Module '{}' dependency not found", GetName());
SEDX_CORE_ERROR("Module '{}' initialization failed", GetName());
SEDX_CORE_FATAL("Module '{}' critical error", GetName());

// Tagged logging for better filtering
SEDX_CORE_INFO_TAG("MODULE", "Loading module: {}", GetName());
SEDX_CORE_ERROR_TAG("MODULE", "Failed to load: {}", errorMessage);
```

#### **Module-Specific Logging**

```cpp
class LoggingModule : public Module
{
public:
    void OnAttach() override
    {
        // Log with module context
        SEDX_CORE_INFO("=== Initializing {} ===", GetName());
      
        // Use tagged logging for categorization
        SEDX_CORE_INFO_TAG("INIT", "Starting resource loading for {}", GetName());
      
        try
        {
            InitializeResources();
            SEDX_CORE_INFO_TAG("INIT", "Resources loaded successfully for {}", GetName());
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("INIT", "Resource loading failed for {}: {}", GetName(), e.what());
            throw; // Re-throw to allow higher-level handling
        }
      
        SEDX_CORE_INFO("=== {} Initialization Complete ===", GetName());
    }
  
private:
    void LogPerformanceMetrics()
    {
        // Use performance-specific tags
        SEDX_CORE_TRACE_TAG("PERF", "Module {} - Update time: {:.3f}ms", 
                            GetName(), m_LastUpdateTime);
        SEDX_CORE_TRACE_TAG("PERF", "Module {} - Memory usage: {} MB", 
                            GetName(), GetMemoryUsage() / 1024 / 1024);
    }
};
```

### 2. **Assertion System**

#### **Module State Validation**

```cpp
class ValidatedModule : public Module
{
public:
    void OnUpdate() override
    {
        // Validate module state before operations
        SEDX_CORE_ASSERT(m_IsInitialized, "Module '{}' not initialized", GetName());
        SEDX_CORE_ASSERT(m_Resources != nullptr, "Module '{}' resources not loaded", GetName());
      
        // Verify preconditions
        SEDX_VERIFY(ValidateInternalState(), "Module '{}' internal state invalid", GetName());
      
        // Perform operations with assertions
        PerformUpdate();
      
        // Validate postconditions
        SEDX_VERIFY(ValidateUpdateResults(), "Module '{}' update produced invalid results", GetName());
    }
  
private:
    bool ValidateInternalState() const
    {
        // Check internal consistency
        return m_Resources != nullptr && 
               m_IsInitialized && 
               m_ComponentCount >= 0;
    }
  
    bool ValidateUpdateResults() const
    {
        // Verify update results are valid
        return true; // Implementation specific
    }
};
```

### 3. **Debug UI Integration**

#### **Comprehensive Debug Interface**

```cpp
class DebugUIModule : public Module
{
public:
    void OnUIRender() override
    {
        RenderModuleDebugWindow();
        RenderPerformanceWindow();
        RenderStateInspector();
    }
  
private:
    void RenderModuleDebugWindow()
    {
        if (!m_ShowDebugWindow) return;
      
        ImGui::Begin("Module Debug");
      
        // Module information
        ImGui::Text("Module: %s", GetName().c_str());
        ImGui::Text("Status: %s", m_IsInitialized ? "Initialized" : "Not Initialized");
        ImGui::Checkbox("Enabled", &m_IsEnabled);
      
        ImGui::Separator();
      
        // Performance metrics
        if (ImGui::CollapsingHeader("Performance"))
        {
            ImGui::Text("Updates/sec: %.1f", 1.0f / m_AverageUpdateTime);
            ImGui::Text("Avg Update Time: %.3f ms", m_AverageUpdateTime * 1000.0f);
            ImGui::Text("Max Update Time: %.3f ms", m_MaxUpdateTime * 1000.0f);
          
            // Update time graph
            ImGui::PlotLines("Update Times", m_UpdateTimes.data(), 
                           static_cast<int>(m_UpdateTimes.size()), 0, nullptr, 
                           0.0f, m_MaxUpdateTime);
        }
      
        // Memory usage
        if (ImGui::CollapsingHeader("Memory"))
        {
            auto memUsage = GetMemoryUsage();
            ImGui::Text("Memory Usage: %.2f MB", memUsage / 1024.0f / 1024.0f);
          
            if (ImGui::Button("Force Garbage Collection"))
            {
                TriggerGarbageCollection();
            }
        }
      
        // Module-specific debug controls
        RenderModuleSpecificDebugUI();
      
        ImGui::End();
    }
  
    void RenderPerformanceWindow()
    {
        if (!m_ShowPerformanceWindow) return;
      
        ImGui::Begin("Module Performance");
      
        // Real-time performance graph
        static std::vector<float> frameTimes;
        frameTimes.push_back(m_LastUpdateTime);
        if (frameTimes.size() > 100) frameTimes.erase(frameTimes.begin());
      
        ImGui::PlotHistogram("Frame Times", frameTimes.data(), 
                           static_cast<int>(frameTimes.size()));
      
        // Performance statistics
        ImGui::Text("Current FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
      
        ImGui::End();
    }
  
    void RenderStateInspector()
    {
        if (!m_ShowStateInspector) return;
      
        ImGui::Begin("Module State Inspector");
      
        // Reflection-based state display
        ImGui::Text("Internal State:");
        ImGui::Indent();
      
        ImGui::Text("Component Count: %d", m_ComponentCount);
        ImGui::Text("Resource Count: %d", m_ResourceCount);
        ImGui::Text("Active Operations: %d", m_ActiveOperations);
      
        ImGui::Unindent();
      
        // State modification controls
        if (ImGui::Button("Reset Module"))
        {
            ResetModule();
        }
      
        ImGui::SameLine();
        if (ImGui::Button("Dump State to Log"))
        {
            DumpStateToLog();
        }
      
        ImGui::End();
    }
  
    virtual void RenderModuleSpecificDebugUI() {}
  
    // Debug control flags
    bool m_ShowDebugWindow = false;
    bool m_ShowPerformanceWindow = false;
    bool m_ShowStateInspector = false;
  
    // Performance tracking
    float m_LastUpdateTime = 0.0f;
    float m_AverageUpdateTime = 0.0f;
    float m_MaxUpdateTime = 0.0f;
    std::vector<float> m_UpdateTimes;
  
    // State tracking
    int m_ComponentCount = 0;
    int m_ResourceCount = 0;
    int m_ActiveOperations = 0;
};
```

### 4. **Testing Support**

#### **Module Testing Framework**

```cpp
class TestableModule : public Module
{
public:
    // Test interface
    struct TestInterface
    {
        virtual ~TestInterface() = default;
        virtual bool RunSelfTest() = 0;
        virtual void SimulateError(const std::string& errorType) = 0;
        virtual std::map<std::string, float> GetMetrics() = 0;
    };
  
    // Expose test interface in debug builds
    #ifdef SEDX_DEBUG
    TestInterface* GetTestInterface() { return this; }
    #endif
  
protected:
    virtual bool RunSelfTest() override
    {
        SEDX_CORE_INFO("Running self-test for module '{}'", GetName());
      
        // Test initialization
        if (!TestInitialization())
        {
            SEDX_CORE_ERROR("Initialization test failed for '{}'", GetName());
            return false;
        }
      
        // Test core functionality
        if (!TestCoreFunctionality())
        {
            SEDX_CORE_ERROR("Core functionality test failed for '{}'", GetName());
            return false;
        }
      
        // Test error handling
        if (!TestErrorHandling())
        {
            SEDX_CORE_ERROR("Error handling test failed for '{}'", GetName());
            return false;
        }
      
        SEDX_CORE_INFO("All tests passed for module '{}'", GetName());
        return true;
    }
  
    virtual void SimulateError(const std::string& errorType) override
    {
        if (errorType == "initialization_failure")
        {
            m_SimulateInitFailure = true;
        }
        else if (errorType == "resource_exhaustion")
        {
            m_SimulateResourceExhaustion = true;
        }
        // ... other error types
    }
  
    virtual std::map<std::string, float> GetMetrics() override
    {
        return {
            {"update_time_ms", m_LastUpdateTime * 1000.0f},
            {"memory_usage_mb", GetMemoryUsage() / 1024.0f / 1024.0f},
            {"error_count", static_cast<float>(m_ErrorCount)},
            {"success_rate", CalculateSuccessRate()}
        };
    }
  
private:
    bool TestInitialization() { return true; }
    bool TestCoreFunctionality() { return true; }
    bool TestErrorHandling() { return true; }
    float CalculateSuccessRate() { return 1.0f; }
  
    // Test simulation flags
    bool m_SimulateInitFailure = false;
    bool m_SimulateResourceExhaustion = false;
    uint32_t m_ErrorCount = 0;
};
```

---

## Conclusion

This comprehensive documentation provides GitHub Copilot instances and agent modes with detailed understanding of the Scenery Editor X Module System. The system enables:

1. **Modular Architecture**: Clean separation of concerns through the Module base class
2. **Lifecycle Management**: Proper initialization and cleanup through standardized methods
3. **Ordered Execution**: ModuleStage ensures correct update and rendering order
4. **Memory Safety**: Integration with the reference counting system prevents leaks
5. **Performance Monitoring**: Built-in profiling and debugging support
6. **Error Handling**: Comprehensive error handling and logging integration

### Key Takeaways for Implementation

1. **Always inherit from Module** for any application component that needs lifecycle management
2. **Use ModuleStage.PushModule()** for core application modules
3. **Use ModuleStage.PushOverlay()** for UI and debug overlays
4. **Implement proper OnAttach()/OnDetach()** pairs for resource management
5. **Use the logging system** with appropriate tags for debugging
6. **Follow the reference counting patterns** for memory management
7. **Implement debug UI** for runtime inspection and debugging

This framework ensures consistent, maintainable, and efficient modular development while adhering to the project's architectural standards and coding practices.

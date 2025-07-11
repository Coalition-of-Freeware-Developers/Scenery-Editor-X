# Scenery Editor X - ModuleStage System Documentation

---

## Overview

The `ModuleStage` class is a sophisticated container and manager for Module instances in Scenery Editor X. It provides an ordered collection system that distinguishes between regular modules and overlay modules, enabling proper layering and execution order management. This system is crucial for maintaining the correct initialization, update, and rendering order of application components.

## ModuleStage Class Documentation

The `ModuleStage` class manages the lifecycle and execution order of modules within the application. It maintains two distinct categories of modules: regular modules and overlay modules, ensuring proper layering for rendering and event handling.

### Class Structure

```cpp
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
    [[nodiscard]] size_t Size() const;

    // Iterator Support
    std::vector<Module*>::iterator begin();
    std::vector<Module*>::iterator end();

private:
    std::vector<Module*> moduleStage;
    unsigned int moduleInsertIndex = 0;
};
```

### Key Features

#### 1. **Dual-Layer Architecture**

- **Regular Modules**: Core application logic modules (terrain, lighting, physics, etc.)
- **Overlay Modules**: UI and overlay components that render on top

#### 2. **Ordered Insertion**

- Regular modules are inserted at the beginning of the stage
- Overlay modules are always appended to the end
- Maintains consistent rendering and update order

#### 3. **STL-Compatible Interface**

- Iterator support for range-based loops
- Index-based access with bounds checking
- Size queries for container management

#### 4. **Memory Safety**

- Bounds checking with assertions
- Safe removal operations with existence validation
- Error logging for failed operations

### Architecture Diagram

```
ModuleStage Layout:
[Regular Modules]          [Overlay Modules]
[0] [1] [2] ... [n-1]     [n] [n+1] ... [size-1]
 ^                         ^
 moduleInsertIndex         Overlays start here
```

### Detailed Method Documentation

#### Module Management Methods

##### `void PushModule(Module *module)`

**Purpose**: Adds a regular module to the stage at the current insertion index.

**Behavior**:

- Inserts the module at `moduleInsertIndex` position
- Increments `moduleInsertIndex` to maintain insertion order
- Regular modules are processed before overlays

**Use Case**: Core application modules that need to execute before UI overlays.

```cpp
// Example: Adding core modules
auto terrainModule = CreateRef<TerrainModule>();
auto physicsModule = CreateRef<PhysicsModule>();

moduleStage.PushModule(terrainModule.get());  // Index 0
moduleStage.PushModule(physicsModule.get());  // Index 1
// moduleInsertIndex is now 2
```

##### `void PushOverlay(Module *overlay)`

**Purpose**: Adds an overlay module to the end of the stage.

**Behavior**:

- Always appends to the end of the vector
- Overlays render on top of regular modules
- Does not affect `moduleInsertIndex`

**Use Case**: UI panels, debug overlays, HUD elements.

```cpp
// Example: Adding UI overlays
auto debugOverlay = CreateRef<DebugOverlay>();
auto sceneHierarchy = CreateRef<SceneHierarchyPanel>();

moduleStage.PushOverlay(debugOverlay.get());     // Appended to end
moduleStage.PushOverlay(sceneHierarchy.get());   // Appended to end
```

##### `void PopModule(Module *module)` / `void PopOverlay(Module *overlay)`

**Purpose**: Removes specified modules from the stage.

**Behavior**:

- Searches for the module in the container
- Removes if found, logs error if not found
- `PopModule` decrements `moduleInsertIndex`
- `PopOverlay` does not affect `moduleInsertIndex`

```cpp
// Example: Removing modules
moduleStage.PopModule(terrainModule.get());    // Removes and decrements index
moduleStage.PopOverlay(debugOverlay.get());    // Removes from end
```

### Usage Examples

#### Example 1: Complete Application Module Setup

```cpp
class Application
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
        // Update all modules in order
        for (auto* module : m_ModuleStage)
        {
            module->OnUpdate();
        }
    }
  
    void Render()
    {
        // Render all modules in order (regular modules first, then overlays)
        for (auto* module : m_ModuleStage)
        {
            module->OnUIRender();
        }
    }

private:
    ModuleStage m_ModuleStage;
  
    // Core modules (stored as Ref for lifetime management)
    Ref<TerrainModule> m_TerrainModule;
    Ref<PhysicsModule> m_PhysicsModule;
    Ref<LightingModule> m_LightingModule;
    Ref<AudioModule> m_AudioModule;
  
    // UI overlays
    Ref<DebugOverlay> m_DebugOverlay;
    Ref<SceneHierarchy> m_SceneHierarchy;
    Ref<PropertyPanel> m_PropertyPanel;
    Ref<AssetBrowser> m_AssetBrowser;
  
    void SetupCoreModules()
    {
        // Create core application modules
        m_TerrainModule = CreateRef<TerrainModule>();
        m_PhysicsModule = CreateRef<PhysicsModule>();
        m_LightingModule = CreateRef<LightingModule>();
        m_AudioModule = CreateRef<AudioModule>();
  
        // Add to stage in dependency order
        m_ModuleStage.PushModule(m_TerrainModule.get());   // Index 0
        m_ModuleStage.PushModule(m_PhysicsModule.get());   // Index 1
        m_ModuleStage.PushModule(m_LightingModule.get());  // Index 2
        m_ModuleStage.PushModule(m_AudioModule.get());     // Index 3
  
        SEDX_CORE_INFO("Core modules added to stage");
    }
  
    void SetupRenderingModules()
    {
        // Add rendering modules that depend on core modules
        auto renderModule = CreateRef<RenderModule>();
        auto postProcessModule = CreateRef<PostProcessModule>();
  
        m_ModuleStage.PushModule(renderModule.get());      // Index 4
        m_ModuleStage.PushModule(postProcessModule.get()); // Index 5
  
        SEDX_CORE_INFO("Rendering modules added to stage");
    }
  
    void SetupUIOverlays()
    {
        // Create UI overlay modules
        m_DebugOverlay = CreateRef<DebugOverlay>();
        m_SceneHierarchy = CreateRef<SceneHierarchy>();
        m_PropertyPanel = CreateRef<PropertyPanel>();
        m_AssetBrowser = CreateRef<AssetBrowser>();
  
        // Add as overlays (will be appended to end)
        m_ModuleStage.PushOverlay(m_DebugOverlay.get());
        m_ModuleStage.PushOverlay(m_SceneHierarchy.get());
        m_ModuleStage.PushOverlay(m_PropertyPanel.get());
        m_ModuleStage.PushOverlay(m_AssetBrowser.get());
  
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
  
    void ClearModuleStage()
    {
        // Clear all module references
        while (m_ModuleStage.Size() > 0)
        {
            auto* lastModule = m_ModuleStage[m_ModuleStage.Size() - 1];
      
            // Determine if it's an overlay or regular module
            bool isOverlay = /* logic to determine overlay status */;
      
            if (isOverlay)
                m_ModuleStage.PopOverlay(lastModule);
            else
                m_ModuleStage.PopModule(lastModule);
        }
    }
};
```

#### Example 2: Dynamic Module Management

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
  
        // Add to stage
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
  
        // Add to stage
        m_ModuleStage.PushOverlay(overlayPtr);
        overlayPtr->OnAttach();
  
        SEDX_CORE_INFO("Created and added overlay: {}", typeid(OverlayType).name());
        return overlayPtr;
    }
  
    void RemoveModule(Module* module)
    {
        // Detach first
        module->OnDetach();
  
        // Remove from stage
        m_ModuleStage.PopModule(module);
  
        // Remove from managed modules
        m_ManagedModules.erase(
            std::remove_if(m_ManagedModules.begin(), m_ManagedModules.end(),
                [module](const Ref<Module>& managedModule) {
                    return managedModule.get() == module;
                }),
            m_ManagedModules.end()
        );
  
        SEDX_CORE_INFO("Removed module from stage");
    }
  
    void RemoveOverlay(Module* overlay)
    {
        // Detach first
        overlay->OnDetach();
  
        // Remove from stage
        m_ModuleStage.PopOverlay(overlay);
  
        // Remove from managed modules
        m_ManagedModules.erase(
            std::remove_if(m_ManagedModules.begin(), m_ManagedModules.end(),
                [overlay](const Ref<Module>& managedModule) {
                    return managedModule.get() == overlay;
                }),
            m_ManagedModules.end()
        );
  
        SEDX_CORE_INFO("Removed overlay from stage");
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
  
    void PrintModuleStageInfo()
    {
        SEDX_CORE_INFO("ModuleStage contains {} modules:", m_ModuleStage.Size());
  
        for (size_t i = 0; i < m_ModuleStage.Size(); ++i)
        {
            auto* module = m_ModuleStage[i];
            SEDX_CORE_INFO("  [{}]: {} ({})", 
                           i, 
                           typeid(*module).name(),
                           (i < m_ModuleInsertIndex) ? "Module" : "Overlay");
        }
    }

private:
    ModuleStage& m_ModuleStage;
    std::vector<Ref<Module>> m_ManagedModules;
    unsigned int m_ModuleInsertIndex = 0; // Track for debugging
};

// Usage example
void ExampleDynamicModuleManagement()
{
    ModuleStage stage;
    ModuleManager manager(stage);
  
    // Create core modules
    auto* terrain = manager.CreateAndAddModule<TerrainModule>();
    auto* physics = manager.CreateAndAddModule<PhysicsModule>();
  
    // Create UI overlays
    auto* debugUI = manager.CreateAndAddOverlay<DebugOverlay>();
    auto* scenePanel = manager.CreateAndAddOverlay<ScenePanelOverlay>();
  
    // Print current stage state
    manager.PrintModuleStageInfo();
  
    // Find and use a specific module
    if (auto* terrainModule = manager.FindModule<TerrainModule>())
    {
        // Use terrain module
        terrainModule->RegenerateTerrain();
    }
  
    // Conditionally remove modules
    if (/* some condition */)
    {
        manager.RemoveOverlay(debugUI);
    }
}
```

#### Example 3: Event Processing with ModuleStage

```cpp
class EventDispatcher
{
public:
    EventDispatcher(ModuleStage& stage) : m_ModuleStage(stage) {}
  
    void DispatchEvent(const Event& event)
    {
        // Process events in reverse order (overlays first)
        // This allows UI overlays to consume events before core modules
  
        for (auto it = m_ModuleStage.end() - 1; it >= m_ModuleStage.begin(); --it)
        {
            auto* module = *it;
      
            // Allow module to process event
            module->OnEvent(event);
      
            // Check if event was consumed
            if (event.IsConsumed())
            {
                SEDX_CORE_TRACE("Event consumed by module: {}", 
                                typeid(*module).name());
                break;
            }
        }
    }
  
    void DispatchUpdateEvents()
    {
        // Update events process in forward order (modules first, then overlays)
        for (auto* module : m_ModuleStage)
        {
            module->OnUpdate();
        }
    }
  
    void DispatchRenderEvents()
    {
        // Render in forward order (modules first, overlays on top)
        for (auto* module : m_ModuleStage)
        {
            module->OnUIRender();
        }
    }

private:
    ModuleStage& m_ModuleStage;
};
```

### Best Practices

#### 1. **Module Ordering**

- Add core modules first (terrain, physics, audio)
- Add dependent modules after their dependencies
- Add UI overlays last to ensure proper rendering order

#### 2. **Memory Management**

- Store module instances as `Ref<Module>` for lifetime management
- Only store raw pointers in the ModuleStage
- Use RAII principles for automatic cleanup

#### 3. **Error Handling**

- Check module validity before operations
- Use proper logging for module operations
- Handle failed module operations gracefully

#### 4. **Performance Considerations**

- Minimize module creation/destruction during runtime
- Use module enable/disable flags instead of removal when possible
- Consider module pooling for frequently created/destroyed modules

#### 5. **Debugging Support**

- Use descriptive module names for identification
- Implement module state inspection methods
- Log module lifecycle events

### Integration Patterns

#### Pattern 1: Modular Game Engine Structure

```cpp
class SceneryEditorXEngine
{
public:
    void Initialize()
    {
        // Core systems
        m_ModuleStage.PushModule(CreateRef<ResourceModule>().get());
        m_ModuleStage.PushModule(CreateRef<RenderModule>().get());
        m_ModuleStage.PushModule(CreateRef<InputModule>().get());
        m_ModuleStage.PushModule(CreateRef<AudioModule>().get());
  
        // Game-specific modules
        m_ModuleStage.PushModule(CreateRef<TerrainModule>().get());
        m_ModuleStage.PushModule(CreateRef<SceneryModule>().get());
        m_ModuleStage.PushModule(CreateRef<WeatherModule>().get());
  
        // UI overlays
        m_ModuleStage.PushOverlay(CreateRef<EditorUI>().get());
        m_ModuleStage.PushOverlay(CreateRef<DebugOverlay>().get());
  
        AttachAllModules();
    }
  
private:
    ModuleStage m_ModuleStage;
    std::vector<Ref<Module>> m_ModuleStorage; // Lifetime management
};
```

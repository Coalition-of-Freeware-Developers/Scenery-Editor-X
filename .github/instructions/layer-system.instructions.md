# Scenery Editor X - Layer System Documentation

## Overview

The Layer System in Scenery Editor X provides a flexible, component-based architecture for organizing different functional components of the application. This system enables modular development where each component can be attached, detached, and updated independently, promoting code reusability, maintainability, and clear separation of concerns.

The system consists of two primary components:

1. **Layer Base Class** - The foundational interface for all modular components
2. **LayerStage Manager** - The container and lifecycle manager for Layer instances

This documentation is designed for GitHub Copilot instances and agent modes to understand and effectively utilize the existing layer framework while maintaining adherence to project standards.

---

## Table of Contents

1. [Layer Base Class Documentation](#layer-base-class-documentation)
2. [LayerStage Manager Documentation](#layerstage-manager-documentation)
3. [Implementation Examples](#implementation-examples)
4. [Best Practices &amp; Patterns](#best-practices--patterns)
5. [Integration Guidelines](#integration-guidelines)
6. [Error Handling &amp; Debugging](#error-handling--debugging)

---

## Layer Base Class Documentation

### Files: `layer.h` & `layer.cpp`

**Location**: `source/SceneryEditorX/core/layers/`

The `Layer` class serves as the foundational base class for all modular components in Scenery Editor X. It provides a standardized interface for lifecycle management, frame-based updates, UI rendering, and event handling.

### Class Definition

```cpp
/**
 * @class Layer
 * @brief Base Layer class representing a logical section of the application.
 *
 * Layers are used to organize different functional components of the application
 * that can be attached, detached and updated independently. Each layer can handle
 * its own rendering, events, and update logic.
 */
class Layer : public RefCounted
{
public:
    /**
     * @brief Construct a modular component that can be plugged into the application.
     * @param name The name of the layer for identification and debugging.
     */
    explicit Layer(const std::string &name = "Layer");

    /**
     * @brief Virtual destructor ensures proper cleanup of derived classes.
     */
    virtual ~Layer() override;

    // Lifecycle Management
    virtual void OnAttach() {}
    virtual void OnDetach() {}

    // Frame-based Operations
    virtual void OnUpdate() {}
    virtual void OnUIRender() {}

    // Event Processing
    virtual void OnEvent() {}

private:
    std::string m_Name; //  The name of the layer.
};
```

### Key Features

#### 1. **Reference Counting Integration**

- **Base Class**: Inherits from `RefCounted` for automatic memory management
- **Memory Safety**: Enables safe sharing and lifetime management across the application
- **Smart Pointers**: Works seamlessly with `Ref<T>` and `WeakRef<T>` system
- **Leak Prevention**: Prevents memory leaks and dangling pointers through reference counting

```cpp
// Example: Creating and managing Layer instances
auto terrainLayer = CreateRef<TerrainLayer>("TerrainRenderer");
auto lightingLayer = CreateRef<LightingLayer>("SceneLighting");

// Layers are automatically cleaned up when references are released
```

#### 2. **Lifecycle Management**

- **OnAttach()**: Called when layer is added to the application
    - Initialize layer-specific resources
    - Set up dependencies and connections
    - Register for events or callbacks

- **OnDetach()**: Called when layer is removed from the application
    - Clean up layer-specific resources
    - Unregister from events or callbacks
    - Save persistent state if needed

```cpp
class ExampleLayer : public Layer
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO("Initializing ExampleLayer");

        // Initialize resources
        m_Buffer = CreateRef<VertexBuffer>(bufferSize);
        m_Texture = CreateRef<Texture2D>("assets/textures/example.png");

        // Register for events
        EventSystem::RegisterCallback(EventType::KeyPressed,
                                    BIND_EVENT_FN(OnKeyPressed));
    }

    void OnDetach() override
    {
        SEDX_CORE_INFO("Cleaning up ExampleLayer");

        // Cleanup resources (automatic with smart pointers)
        m_Buffer.Reset();
        m_Texture.Reset();

        // Unregister from events
        EventSystem::UnregisterCallback(EventType::KeyPressed, this);
    }
};
```

#### 3. **Frame-based Operations**

- **OnUpdate()**: Called every frame for layer logic
    - Update game state
    - Process input that affects the layer
    - Perform per-frame calculations
    - Handle animations and transitions

- **OnUIRender()**: Called every frame for ImGui rendering
    - Render debug UI
    - Display layer-specific controls
    - Show performance metrics
    - Provide user interaction interfaces

```cpp
class PhysicsLayer : public Layer
{
public:
    void OnUpdate() override
    {
        SEDX_PROFILE_SCOPE("PhysicsLayer::OnUpdate");

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
    - Manage layer-specific event responses
    - Enable event consumption to prevent propagation

```cpp
class InputLayer : public Layer
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
            event.SetConsumed(true); // Prevent other layers from processing
        }
    }
};
```

#### 5. **Layer Identification**

- **Naming System**: Each layer has a unique name for debugging and identification
- **Logging Support**: Name is used in logging and profiling systems
- **Runtime Queries**: Enables finding specific layers at runtime

```cpp
// Layer creation with descriptive names
auto layer = CreateRef<CustomLayer>("SceneryRenderer");

// Logging with layer identification
SEDX_CORE_INFO("Layer '{}' initialized successfully", layer->GetName());
```

### Virtual Method Implementation Guidelines

When implementing custom layers, follow these patterns:

#### **OnAttach() Implementation**

```cpp
void CustomLayer::OnAttach() override
{
    SEDX_CORE_INFO("Attaching layer: {}", GetName());

    // 1. Initialize layer-specific resources
    InitializeResources();

    // 2. Set up dependencies
    ValidateDependencies();

    // 3. Register for events/callbacks
    RegisterCallbacks();

    // 4. Load configuration/settings
    LoadSettings();

    SEDX_CORE_INFO("Layer '{}' attached successfully", GetName());
}
```

#### **OnDetach() Implementation**

```cpp
void CustomLayer::OnDetach() override
{
    SEDX_CORE_INFO("Detaching layer: {}", GetName());

    // 1. Save state/configuration
    SaveSettings();

    // 2. Unregister from events/callbacks
    UnregisterCallbacks();

    // 3. Clean up resources (automatic with smart pointers)
    CleanupResources();

    SEDX_CORE_INFO("Layer '{}' detached successfully", GetName());
}
```

#### **OnUpdate() Implementation**

```cpp
void CustomLayer::OnUpdate() override
{
    // Early exit if layer is disabled
    if (!m_IsEnabled) return;

    SEDX_PROFILE_SCOPE("CustomLayer::OnUpdate");

    // 1. Update layer state
    UpdateLayerState();

    // 2. Process frame-specific logic
    ProcessFrameLogic();

    // 3. Handle time-based operations
    ProcessTimers();

    // 4. Update dependent systems
    UpdateDependentSystems();
}
```

---

## LayerStage Manager Documentation

### Files: `layer_stage.h` & `layer_stage.cpp`

**Location**: `source/SceneryEditorX/core/layers/`

The `LayerStage` class is a sophisticated container and manager for Layer instances. It provides an ordered collection system that distinguishes between regular layers and overlay layers, enabling proper layering and execution order management.

### Class Definition

```cpp
/**
 * @class LayerStage
 * @brief Container and manager for Layer instances with ordered execution.
 *
 * Manages layer lifecycle and execution order, distinguishing between
 * regular layers and overlay layers for proper layering.
 */
class LayerStage
{
public:
    LayerStage() = default;
    ~LayerStage() = default;

    // Layer Management
    void PushLayer(Layer *layer);
    void PushOverlay(Layer *overlay);
    void PopLayer(Layer *layer);
    void PopOverlay(Layer *overlay);

    // Access Operations
    Layer *operator[](size_t index);
    const Layer *operator[](size_t index) const;
    [[nodiscard]] size_t Size() const { return layerStage.size(); }

    // Iterator Support
    std::vector<Layer*>::iterator begin() { return layerStage.begin(); }
    std::vector<Layer*>::iterator end() { return layerStage.end(); }

private:
    std::vector<Layer*> layerStage;
    unsigned int layerInsertIndex = 0;
};
```

### Architecture & Execution Order

The LayerStage uses a two-tier architecture:

```
┌─────────────────────────────────────────────────────────────┐
│                    LayerStage Layout                       │
├─────────────────────────────────────────────────────────────┤
│ Regular Layers (Index 0 to layerInsertIndex-1)           │
│ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐            │
│ │   Layer 0  │ │   Layer 1  │ │   Layer 2  │   ...      │
│ │  (Terrain)  │ │ (Physics)   │ │ (Audio)     │            │
│ └─────────────┘ └─────────────┘ └─────────────┘            │
├─────────────────────────────────────────────────────────────┤
│ Overlay Layers (Index layerInsertIndex to end)           │
│ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐            │
│ │  Overlay 0  │ │  Overlay 1  │ │  Overlay 2  │   ...      │
│ │ (Debug UI)  │ │ (Scene Panel│ │ (Inspector) │            │
│ └─────────────┘ └─────────────┘ └─────────────┘            │
└─────────────────────────────────────────────────────────────┘
```

**Execution Order**:

- **Updates**: Regular layers → Overlay layers (forward order)
- **Events**: Overlay layers → Regular layers (reverse order)
- **Rendering**: Regular layers → Overlay layers (forward order)

### Method Documentation

#### **void PushLayer(Layer \*layer)**

**Purpose**: Adds a regular layer to the stage at the current insertion index.

**Behavior**:

- Inserts layer at `layerInsertIndex` position
- Increments `layerInsertIndex` to maintain separation
- Regular layers execute before overlays

**Use Cases**: Core application layers that need to execute before UI overlays.

```cpp
// Example: Adding core layers
auto terrainLayer = CreateRef<TerrainLayer>();
auto physicsLayer = CreateRef<PhysicsLayer>();

layerStage.PushLayer(terrainLayer.get());  // Index 0
layerStage.PushLayer(physicsLayer.get());  // Index 1
// layerInsertIndex is now 2
```

**Implementation Details**:

```cpp
void LayerStage::PushLayer(Layer *layer)
{
    // Insert at current insertion index and increment
    layerStage.emplace(layerStage.begin() + layerInsertIndex, layer);
    layerInsertIndex++;
}
```

#### **void PushOverlay(Layer \*overlay)**

**Purpose**: Adds an overlay layer to the end of the stage.

**Behavior**:

- Always appends to the end of the vector
- Overlays render on top of regular layers
- Does not affect `layerInsertIndex`

**Use Cases**: UI panels, debug overlays, HUD elements.

```cpp
// Example: Adding UI overlays
auto debugOverlay = CreateRef<DebugOverlay>();
auto sceneHierarchy = CreateRef<SceneHierarchyPanel>();

layerStage.PushOverlay(debugOverlay.get());     // Appended to end
layerStage.PushOverlay(sceneHierarchy.get());   // Appended to end
```

**Implementation Details**:

```cpp
void LayerStage::PushOverlay(Layer *overlay)
{
    // Simply append to the end
    layerStage.emplace_back(overlay);
}
```

#### **void PopLayer(Layer *layer) / void PopOverlay(Layer *overlay)**

**Purpose**: Removes specified layers from the stage.

**Behavior**:

- Searches for the layer using `std::ranges::find`
- Removes if found, logs error if not found
- `PopLayer` decrements `layerInsertIndex`
- `PopOverlay` does not affect `layerInsertIndex`

```cpp
// Example: Removing layers
layerStage.PopLayer(terrainLayer.get());    // Removes and decrements index
layerStage.PopOverlay(debugOverlay.get());    // Removes from end
```

**Implementation Details**:

```cpp
void LayerStage::PopLayer(Layer *layer)
{
    if (const auto it = std::ranges::find(layerStage, layer); it != layerStage.end())
    {
        layerStage.erase(it);
        layerInsertIndex--;
    }
    else
    {
        SEDX_CORE_ERROR("Layer not found in stage");
    }
}

void LayerStage::PopOverlay(Layer *overlay)
{
    if (const auto it = std::ranges::find(layerStage, overlay); it != layerStage.end())
    {
        layerStage.erase(it);
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
Layer *operator[](size_t index)
{
    SEDX_CORE_ASSERT(index >= 0 && index < layerStage.size());
    return layerStage[index];
}
```

**Iterator Support**:

```cpp
// Range-based for loop support
for (auto* layer : layerStage)
{
    layer->OnUpdate();
}

// Traditional iterator usage
for (auto it = layerStage.begin(); it != layerStage.end(); ++it)
{
    (*it)->OnUIRender();
}
```

---

## Implementation Examples

### Example 1: Complete Application Layer Setup

```cpp
class SceneryEditorXApplication
{
public:
    void Initialize()
    {
        SetupCoreLayers();
        SetupRenderingLayers();
        SetupUIOverlays();
        AttachAllLayers();

        SEDX_CORE_INFO("Application initialized with {} layers",
                       m_LayerStage.Size());
    }

    void Shutdown()
    {
        DetachAllLayers();
        ClearLayerStage();
    }

    void Update()
    {
        SEDX_PROFILE_SCOPE("Application::Update");

        // Update all layers in order (layers first, then overlays)
        for (auto* layer : m_LayerStage)
        {
            layer->OnUpdate();
        }
    }

    void Render()
    {
        SEDX_PROFILE_SCOPE("Application::Render");

        // Render all layers in order
        for (auto* layer : m_LayerStage)
        {
            layer->OnUIRender();
        }
    }

    void ProcessEvents(Event& event)
    {
        SEDX_PROFILE_SCOPE("Application::ProcessEvents");

        // Process events in reverse order (overlays first)
        for (auto it = m_LayerStage.end() - 1; it >= m_LayerStage.begin(); --it)
        {
            auto* layer = *it;
            layer->OnEvent();

            if (event.IsConsumed())
            {
                break; // Stop processing if event was consumed
            }
        }
    }

private:
    LayerStage m_LayerStage;

    // Layer storage for lifetime management
    Ref<TerrainLayer> m_TerrainLayer;
    Ref<PhysicsLayer> m_PhysicsLayer;
    Ref<LightingLayer> m_LightingLayer;
    Ref<AudioLayer> m_AudioLayer;
    Ref<DebugOverlay> m_DebugOverlay;
    Ref<SceneHierarchy> m_SceneHierarchy;

    void SetupCoreLayers()
    {
        // Create core application layers
        m_TerrainLayer = CreateRef<TerrainLayer>("TerrainSystem");
        m_PhysicsLayer = CreateRef<PhysicsLayer>("PhysicsWorld");
        m_LightingLayer = CreateRef<LightingLayer>("LightingRenderer");
        m_AudioLayer = CreateRef<AudioLayer>("AudioEngine");

        // Add to stage in dependency order
        m_LayerStage.PushLayer(m_TerrainLayer.get());   // Index 0
        m_LayerStage.PushLayer(m_PhysicsLayer.get());   // Index 1
        m_LayerStage.PushLayer(m_LightingLayer.get());  // Index 2
        m_LayerStage.PushLayer(m_AudioLayer.get());     // Index 3

        SEDX_CORE_INFO("Core layers added to stage");
    }

    void SetupUIOverlays()
    {
        // Create UI overlay layers
        m_DebugOverlay = CreateRef<DebugOverlay>("DebugUI");
        m_SceneHierarchy = CreateRef<SceneHierarchy>("ScenePanel");

        // Add as overlays (appended to end)
        m_LayerStage.PushOverlay(m_DebugOverlay.get());
        m_LayerStage.PushOverlay(m_SceneHierarchy.get());

        SEDX_CORE_INFO("UI overlays added to stage");
    }

    void AttachAllLayers()
    {
        for (auto* layer : m_LayerStage)
        {
            layer->OnAttach();
        }
    }

    void DetachAllLayers()
    {
        // Detach in reverse order for proper cleanup
        for (auto it = m_LayerStage.end() - 1; it >= m_LayerStage.begin(); --it)
        {
            (*it)->OnDetach();
        }
    }
};
```

### Example 2: Dynamic Layer Management

```cpp
class LayerManager
{
public:
    LayerManager(LayerStage& stage) : m_LayerStage(stage) {}

    template<typename LayerType, typename... Args>
    LayerType* CreateAndAddLayer(Args&&... args)
    {
        auto layer = CreateRef<LayerType>(std::forward<Args>(args)...);
        LayerType* layerPtr = layer.get();

        // Store for lifetime management
        m_ManagedLayers.push_back(layer);

        // Add to stage and attach
        m_LayerStage.PushLayer(layerPtr);
        layerPtr->OnAttach();

        SEDX_CORE_INFO("Created and added layer: {}", typeid(LayerType).name());
        return layerPtr;
    }

    template<typename OverlayType, typename... Args>
    OverlayType* CreateAndAddOverlay(Args&&... args)
    {
        auto overlay = CreateRef<OverlayType>(std::forward<Args>(args)...);
        OverlayType* overlayPtr = overlay.get();

        // Store for lifetime management
        m_ManagedLayers.push_back(overlay);

        // Add to stage and attach
        m_LayerStage.PushOverlay(overlayPtr);
        overlayPtr->OnAttach();

        SEDX_CORE_INFO("Created and added overlay: {}", typeid(OverlayType).name());
        return overlayPtr;
    }

    template<typename LayerType>
    LayerType* FindLayer()
    {
        for (auto* layer : m_LayerStage)
        {
            if (auto* typedLayer = dynamic_cast<LayerType*>(layer))
                return typedLayer;
        }
        return nullptr;
    }

    void RemoveLayer(Layer* layer)
    {
        // Detach first
        layer->OnDetach();

        // Remove from stage
        m_LayerStage.PopLayer(layer);

        // Remove from managed storage
        RemoveFromManagedStorage(layer);

        SEDX_CORE_INFO("Removed layer from stage");
    }

private:
    LayerStage& m_LayerStage;
    std::vector<Ref<Layer>> m_ManagedLayers;

    void RemoveFromManagedStorage(Layer* layer)
    {
        m_ManagedLayers.erase(
            std::remove_if(m_ManagedLayers.begin(), m_ManagedLayers.end(),
                [layer](const Ref<Layer>& managedLayer) {
                    return managedLayer.get() == layer;
                }),
            m_ManagedLayers.end()
        );
    }
};
```

### Example 3: Custom Layer Implementation

```cpp
class SceneryRenderLayer : public Layer
{
public:
    explicit SceneryRenderLayer(const std::string& name = "SceneryRenderer")
        : Layer(name)
        , m_IsEnabled(true)
        , m_RenderDistance(1000.0f)
    {
    }

    void OnAttach() override
    {
        SEDX_CORE_INFO("Initializing Scenery Render Layer");

        // Initialize rendering resources
        m_Shader = CreateRef<Shader>("assets/shaders/scenery.glsl");
        m_Camera = CreateRef<Camera>();
        m_SceneryManager = CreateRef<SceneryManager>();

        // Set up render targets
        m_RenderTarget = CreateRef<Framebuffer>(1920, 1080);

        // Register for events
        EventSystem::Subscribe(EventType::WindowResize,
                              BIND_EVENT_FN(OnWindowResize));

        SEDX_CORE_INFO("Scenery Render Layer initialized successfully");
    }

    void OnDetach() override
    {
        SEDX_CORE_INFO("Cleaning up Scenery Render Layer");

        // Unregister from events
        EventSystem::Unsubscribe(EventType::WindowResize, this);

        // Resources automatically cleaned up by smart pointers
        m_Shader.Reset();
        m_Camera.Reset();
        m_SceneryManager.Reset();
        m_RenderTarget.Reset();

        SEDX_CORE_INFO("Scenery Render Layer cleaned up");
    }

    void OnUpdate() override
    {
        if (!m_IsEnabled) return;

        SEDX_PROFILE_SCOPE("SceneryRenderLayer::OnUpdate");

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

    // Layer-specific methods
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

    // Layer state
    bool m_IsEnabled;
    bool m_ShowDebugUI = false;
    float m_RenderDistance;

    // Statistics
    uint32_t m_LastVisibleCount = 0;
    uint32_t m_LastDrawCalls = 0;
    uint32_t m_LastTriangleCount = 0;

    void PrepareRenderCommands(const std::vector<SceneryObject*>& visibleObjects)
    {
        SEDX_PROFILE_SCOPE("SceneryRenderLayer::PrepareRenderCommands");

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

### 1. **Layer Lifecycle Management**

#### **Initialization Order**

```cpp
// Correct dependency order
void SetupLayers()
{
    // 1. Foundation layers first
    m_LayerStage.PushLayer(CreateRef<ResourceLayer>().get());
    m_LayerStage.PushLayer(CreateRef<InputLayer>().get());

    // 2. Core systems
    m_LayerStage.PushLayer(CreateRef<PhysicsLayer>().get());
    m_LayerStage.PushLayer(CreateRef<AudioLayer>().get());

    // 3. Rendering layers
    m_LayerStage.PushLayer(CreateRef<RenderLayer>().get());
    m_LayerStage.PushLayer(CreateRef<SceneryLayer>().get());

    // 4. UI overlays last
    m_LayerStage.PushOverlay(CreateRef<DebugUI>().get());
    m_LayerStage.PushOverlay(CreateRef<EditorUI>().get());
}
```

#### **Cleanup Order**

```cpp
// Proper cleanup sequence
void Shutdown()
{
    // Detach in reverse order
    for (auto it = m_LayerStage.end() - 1; it >= m_LayerStage.begin(); --it)
    {
        (*it)->OnDetach();
    }

    // Clear the stage
    while (m_LayerStage.Size() > 0)
    {
        // Remove last layer (maintains index integrity)
        auto* lastLayer = m_LayerStage[m_LayerStage.Size() - 1];

        // Determine removal method based on position
        if (IsOverlay(lastLayer))
            m_LayerStage.PopOverlay(lastLayer);
        else
            m_LayerStage.PopLayer(lastLayer);
    }
}
```

### 2. **Memory Management Patterns**

#### **Smart Pointer Usage**

```cpp
class LayerContainer
{
public:
    void AddLayer(const std::string& name)
    {
        // Create with smart pointer for automatic lifetime management
        auto layer = CreateRef<CustomLayer>(name);

        // Store reference for lifetime
        m_Layers.push_back(layer);

        // Add raw pointer to stage
        m_LayerStage.PushLayer(layer.get());

        // Attach after adding to stage
        layer->OnAttach();
    }

private:
    LayerStage m_LayerStage;
    std::vector<Ref<Layer>> m_Layers; // Lifetime management
};
```

#### **Resource Management in Layers**

```cpp
class ResourceAwareLayer : public Layer
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
class OptimizedLayer : public Layer
{
public:
    void OnUpdate() override
    {
        // Early exit for disabled layers
        if (!m_IsEnabled) return;

        // Use profiling scopes for performance monitoring
        SEDX_PROFILE_SCOPE("OptimizedLayer::OnUpdate");

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
        // Update every N frames for less critical layers
        return (m_FrameCounter++ % m_UpdateFrequency) == 0;
    }

    uint32_t m_UpdateFrequency = 1; // Update every frame by default
};
```

#### **Event Processing Optimization**

```cpp
class EventOptimizedLayer : public Layer
{
public:
    void OnEvent() override
    {
        // Only process events this layer cares about
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
    // Bitmask for events this layer cares about
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
class RobustLayer : public Layer
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
                SEDX_CORE_ERROR("Layer '{}' dependencies not met", GetName());
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
            SEDX_CORE_INFO("Layer '{}' initialized successfully", GetName());
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
            // Optionally disable layer on repeated errors
            m_ErrorCount++;
            if (m_ErrorCount > MAX_ERRORS)
            {
                SEDX_CORE_WARN("Disabling layer '{}' due to repeated errors", GetName());
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
        // Initialize layer-specific resources
        return true; // Implementation specific
    }
};
```

#### **Debug Information**

```cpp
class DebuggableLayer : public Layer
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
        SEDX_CORE_INFO("=== Layer State: {} ===", GetName());
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

        ImGui::Text("Layer: %s", GetName().c_str());
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

### 1. **Layer Communication Patterns**

#### **Direct Layer Access**

```cpp
class LayerCommunicationExample
{
public:
    void Initialize()
    {
        // Store references to frequently accessed layers
        m_TerrainLayer = CreateRef<TerrainLayer>();
        m_PhysicsLayer = CreateRef<PhysicsLayer>();

        m_LayerStage.PushLayer(m_TerrainLayer.get());
        m_LayerStage.PushLayer(m_PhysicsLayer.get());

        // Set up cross-layer dependencies
        m_PhysicsLayer->SetTerrainProvider(m_TerrainLayer.get());
    }

    void UpdateLayerCommunication()
    {
        // Direct access through stored references
        auto terrainHeight = m_TerrainLayer->GetHeightAt({100, 100});
        m_PhysicsLayer->UpdateGroundHeight(terrainHeight);
    }

private:
    LayerStage m_LayerStage;
    Ref<TerrainLayer> m_TerrainLayer;
    Ref<PhysicsLayer> m_PhysicsLayer;
};
```

#### **Event-Based Communication**

```cpp
class EventBasedLayer : public Layer
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
        // Respond to terrain changes from other layers
        UpdatePhysicsColliders(event.GetAffectedRegion());
    }

    void NotifyTerrainChange(const Region& region)
    {
        // Emit event for other layers
        auto event = TerrainChangedEvent(region);
        EventSystem::Dispatch(event);
    }
};
```

#### **Service Locator Pattern**

```cpp
class ServiceLayer : public Layer
{
public:
    void OnAttach() override
    {
        // Register services this layer provides
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

// Usage in other layers
class ConsumerLayer : public Layer
{
public:
    void OnUpdate() override
    {
        // Access services provided by other layers
        if (auto* terrainService = ServiceLocator::GetService<ITerrainService>())
        {
            auto height = terrainService->GetHeightAt({x, z});
            // Use height data
        }
    }
};
```

### 2. **Configuration and Settings**

#### **Layer Configuration**

```cpp
class ConfigurableLayer : public Layer
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

    explicit ConfigurableLayer(const Config& config = {})
        : Layer("ConfigurableLayer")
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

                SEDX_CORE_INFO("Loaded configuration for layer '{}'", GetName());
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

            SEDX_CORE_INFO("Saved configuration for layer '{}'", GetName());
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR("Failed to save config for '{}': {}", GetName(), e.what());
        }
    }

    void ApplyConfiguration()
    {
        // Apply configuration settings
        // Implementation specific to layer
    }
};
```

### 3. **Layer Factory Pattern**

#### **Automatic Layer Registration**

```cpp
// Layer factory for dynamic creation
class LayerFactory
{
public:
    template<typename LayerType>
    static void RegisterLayer(const std::string& name)
    {
        GetRegistry()[name] = []() -> std::unique_ptr<Layer> {
            return std::make_unique<LayerType>();
        };
    }

    static std::unique_ptr<Layer> CreateLayer(const std::string& name)
    {
        auto& registry = GetRegistry();
        auto it = registry.find(name);
        if (it != registry.end())
        {
            return it->second();
        }

        SEDX_CORE_ERROR("Unknown layer type: {}", name);
        return nullptr;
    }

    static std::vector<std::string> GetRegisteredLayers()
    {
        std::vector<std::string> names;
        for (const auto& pair : GetRegistry())
        {
            names.push_back(pair.first);
        }
        return names;
    }

private:
    using LayerCreator = std::function<std::unique_ptr<Layer>()>;
    using LayerRegistry = std::unordered_map<std::string, LayerCreator>;

    static LayerRegistry& GetRegistry()
    {
        static LayerRegistry registry;
        return registry;
    }
};

// Auto-registration helper
template<typename LayerType>
struct LayerRegistrar
{
    LayerRegistrar(const std::string& name)
    {
        LayerFactory::RegisterLayer<LayerType>(name);
    }
};

#define REGISTER_LAYER(LayerType, Name) \
    static LayerRegistrar<LayerType> g_##LayerType##Registrar(Name)

// Usage in layer files:
// REGISTER_LAYER(TerrainLayer, "Terrain");
// REGISTER_LAYER(PhysicsLayer, "Physics");
```

---

## Error Handling & Debugging

### 1. **Logging Integration**

The layer system integrates with the Scenery Editor X logging system using standardized macros:

#### **Core Logging Macros**

```cpp
// Core layer logging
SEDX_CORE_TRACE("Layer '{}' performing operation", GetName());
SEDX_CORE_INFO("Layer '{}' initialized successfully", GetName());
SEDX_CORE_WARN("Layer '{}' dependency not found", GetName());
SEDX_CORE_ERROR("Layer '{}' initialization failed", GetName());
SEDX_CORE_FATAL("Layer '{}' critical error", GetName());

// Tagged logging for better filtering
SEDX_CORE_INFO_TAG("LAYER", "Loading layer: {}", GetName());
SEDX_CORE_ERROR_TAG("LAYER", "Failed to load: {}", errorMessage);
```

#### **Layer-Specific Logging**

```cpp
class LoggingLayer : public Layer
{
public:
    void OnAttach() override
    {
        // Log with layer context
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
        SEDX_CORE_TRACE_TAG("PERF", "Layer {} - Update time: {:.3f}ms",
                            GetName(), m_LastUpdateTime);
        SEDX_CORE_TRACE_TAG("PERF", "Layer {} - Memory usage: {} MB",
                            GetName(), GetMemoryUsage() / 1024 / 1024);
    }
};
```

### 2. **Assertion System**

#### **Layer State Validation**

```cpp
class ValidatedLayer : public Layer
{
public:
    void OnUpdate() override
    {
        // Validate layer state before operations
        SEDX_CORE_ASSERT(m_IsInitialized, "Layer '{}' not initialized", GetName());
        SEDX_CORE_ASSERT(m_Resources != nullptr, "Layer '{}' resources not loaded", GetName());

        // Verify preconditions
        SEDX_VERIFY(ValidateInternalState(), "Layer '{}' internal state invalid", GetName());

        // Perform operations with assertions
        PerformUpdate();

        // Validate postconditions
        SEDX_VERIFY(ValidateUpdateResults(), "Layer '{}' update produced invalid results", GetName());
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
class DebugUILayer : public Layer
{
public:
    void OnUIRender() override
    {
        RenderLayerDebugWindow();
        RenderPerformanceWindow();
        RenderStateInspector();
    }

private:
    void RenderLayerDebugWindow()
    {
        if (!m_ShowDebugWindow) return;

        ImGui::Begin("Layer Debug");

        // Layer information
        ImGui::Text("Layer: %s", GetName().c_str());
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

        // Layer-specific debug controls
        RenderLayerSpecificDebugUI();

        ImGui::End();
    }

    void RenderPerformanceWindow()
    {
        if (!m_ShowPerformanceWindow) return;

        ImGui::Begin("Layer Performance");

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

        ImGui::Begin("Layer State Inspector");

        // Reflection-based state display
        ImGui::Text("Internal State:");
        ImGui::Indent();

        ImGui::Text("Component Count: %d", m_ComponentCount);
        ImGui::Text("Resource Count: %d", m_ResourceCount);
        ImGui::Text("Active Operations: %d", m_ActiveOperations);

        ImGui::Unindent();

        // State modification controls
        if (ImGui::Button("Reset Layer"))
        {
            ResetLayer();
        }

        ImGui::SameLine();
        if (ImGui::Button("Dump State to Log"))
        {
            DumpStateToLog();
        }

        ImGui::End();
    }

    virtual void RenderLayerSpecificDebugUI() {}

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

#### **Layer Testing Framework**

```cpp
class TestableLayer : public Layer
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
        SEDX_CORE_INFO("Running self-test for layer '{}'", GetName());

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

        SEDX_CORE_INFO("All tests passed for layer '{}'", GetName());
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

This comprehensive documentation provides GitHub Copilot instances and agent modes with detailed understanding of the Scenery Editor X Layer System. The system enables:

1. **Modular Architecture**: Clean separation of concerns through the Layer base class
2. **Lifecycle Management**: Proper initialization and cleanup through standardized methods
3. **Ordered Execution**: LayerStage ensures correct update and rendering order
4. **Memory Safety**: Integration with the reference counting system prevents leaks
5. **Performance Monitoring**: Built-in profiling and debugging support
6. **Error Handling**: Comprehensive error handling and logging integration

### Key Takeaways for Implementation

1. **Always inherit from Layer** for any application component that needs lifecycle management
2. **Use LayerStage.PushLayer()** for core application layers
3. **Use LayerStage.PushOverlay()** for UI and debug overlays
4. **Implement proper OnAttach()/OnDetach()** pairs for resource management
5. **Use the logging system** with appropriate tags for debugging
6. **Follow the reference counting patterns** for memory management
7. **Implement debug UI** for runtime inspection and debugging

This framework ensures consistent, maintainable, and efficient modular development while adhering to the project's architectural standards and coding practices.

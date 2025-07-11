# Scenery Editor X - Module System Documentation

## Overview

The Module System in Scenery Editor X provides a flexible, component-based architecture for organizing different functional components of the application. This system allows for modular development where each component can be attached, detached, and updated independently, promoting code reusability, maintainability, and clear separation of concerns.

## Core Components

### 1. Module Base Class (`module.h` / `module.cpp`)

### 2. ModuleStage Manager (`module_stage.h` / `module_stage.cpp`)

---

## Module Base Class Documentation

The `Module` class serves as the foundational base class for all modular components in Scenery Editor X. It provides a standardized interface for lifecycle management, updates, rendering, and event handling.

### Class Structure

```cpp
class Module : public RefCounted
{
public:
    explicit Module(const std::string &name = "Module");
    virtual ~Module() override;
  
    // Lifecycle Methods
    virtual void OnAttach() {}
    virtual void OnDetach() {}
  
    // Update and Rendering
    virtual void OnUpdate() {}
    virtual void OnUIRender() {}
  
    // Event Handling
    virtual void OnEvent() {}

private:
    std::string m_Name; ///< The name of the module.
};
```

### Key Features

#### 1. **Reference Counting**

- Inherits from `RefCounted` for automatic memory management
- Enables safe sharing and lifetime management across the application
- Prevents memory leaks and dangling pointers

#### 2. **Lifecycle Management**

- `OnAttach()`: Called when module is added to the application
- `OnDetach()`: Called when module is removed from the application
- Virtual destructor ensures proper cleanup of derived classes

#### 3. **Frame-based Operations**

- `OnUpdate()`: Called every frame for module logic
- `OnUIRender()`: Called every frame for ImGui rendering
- `OnEvent()`: Called for event processing

#### 4. **Identification**

- Each module has a unique name for debugging and identification
- Useful for logging, profiling, and module management

### Usage Examples

#### Example 1: Basic Module Implementation

```cpp
#include <SceneryEditorX/core/modules/module.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/logging/asserts.h>

class TerrainModule : public Module
{
public:
    TerrainModule() : Module("TerrainModule") {}
  
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing {} ===", GetName());
        
        try
        {
            // Critical initialization - use ASSERT for required components
            SEDX_CORE_ASSERT(LoadTerrainShaders(), "Failed to load terrain shaders");
            SEDX_CORE_ASSERT(CreateTerrainBuffers(), "Failed to create terrain buffers");
            
            // Non-critical features - use VERIFY for graceful degradation
            SEDX_CORE_VERIFY(LoadDefaultTerrain(), "Failed to load default terrain, using fallback");
            
            m_IsInitialized = true;
            SEDX_CORE_INFO_TAG("INIT", "Terrain module initialized successfully");
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("INIT", "Failed to initialize terrain module: {}", e.what());
            throw;
        }
    }
  
    void OnDetach() override
    {
        SEDX_CORE_INFO("=== Cleaning up {} ===", GetName());
      
        // Cleanup terrain resources
        CleanupTerrainBuffers();
        UnloadTerrainShaders();
        
        // Verify cleanup completed successfully
        SEDX_CORE_VERIFY(m_ActiveTerrainChunks.empty(), 
                         "{} terrain chunks leaked during shutdown", m_ActiveTerrainChunks.size());
      
        m_IsInitialized = false;
    }
  
    void OnUpdate() override
    {
        if (!m_IsEnabled) return;
        
        SEDX_CORE_ASSERT(m_IsInitialized, "Terrain module update called before initialization");
        SEDX_PROFILE_SCOPE("TerrainModule::OnUpdate");
      
        // Update terrain LOD based on camera position
        UpdateTerrainLOD();
      
        // Update terrain streaming
        UpdateTerrainStreaming();
      
        // Update terrain physics
        UpdateTerrainPhysics();
    }
  
    void OnUIRender() override
    {
        if (!m_IsInitialized) return;
      
        if (ImGui::Begin("Terrain Settings"))
        {
            ImGui::SliderFloat("LOD Distance", &m_LodDistance, 100.0f, 10000.0f);
            ImGui::SliderInt("Max Terrain Chunks", &m_MaxChunks, 16, 256);
          
            if (ImGui::Button("Regenerate Terrain"))
            {
                // Validate terrain parameters before regeneration
                SEDX_ASSERT(m_LodDistance > 0.0f, "LOD distance must be positive");
                SEDX_ASSERT(m_MaxChunks > 0, "Max chunks must be greater than zero");
                
                RegenerateTerrain();
            }
        }
        ImGui::End();
    }
  
    void OnEvent() override
    {
        // Handle terrain-specific events
        // Could process camera movement, user input, etc.
    }

private:
    bool m_IsInitialized = false;
    bool m_IsEnabled = true;
    float m_LodDistance = 1000.0f;
    int m_MaxChunks = 64;
    std::vector<TerrainChunk> m_ActiveTerrainChunks;
  
    bool LoadTerrainShaders() { /* Implementation */ return true; }
    bool CreateTerrainBuffers() { /* Implementation */ return true; }
    bool LoadDefaultTerrain() { /* Implementation */ return true; }
    void CleanupTerrainBuffers() { /* Implementation */ }
    void UnloadTerrainShaders() { /* Implementation */ }
    void UpdateTerrainLOD() { /* Implementation */ }
    void UpdateTerrainStreaming() { /* Implementation */ }
    void UpdateTerrainPhysics() { /* Implementation */ }
    void RegenerateTerrain() { /* Implementation */ }
};
```

#### Example 2: UI Module Implementation

```cpp
class SceneryBrowserModule : public Module
{
public:
    SceneryBrowserModule() : Module("SceneryBrowserModule") {}
  
    void OnAttach() override
    {
        SEDX_CORE_INFO("=== Initializing {} ===", GetName());
        
        try
        {
            // Critical initialization
            SEDX_ASSERT(LoadSceneryDatabase(), "Failed to load scenery database");
            
            // Non-critical features with graceful degradation
            SEDX_VERIFY(InitializeFileWatchers(), "File watching disabled - manual refresh required");
            
            m_IsVisible = true;
            SEDX_CORE_INFO_TAG("INIT", "Scenery browser module initialized successfully");
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("INIT", "Failed to initialize scenery browser: {}", e.what());
            throw;
        }
    }
  
    void OnDetach() override
    {
        SEDX_CORE_INFO("=== Cleaning up {} ===", GetName());
        
        // Cleanup file watchers
        CleanupFileWatchers();
        
        // Save user preferences with validation
        SEDX_VERIFY(SaveBrowserPreferences(), "Failed to save browser preferences");
    }
  
    void OnUpdate() override
    {
        // Update file watcher events
        ProcessFileWatcherEvents();
      
        // Update scenery loading status
        UpdateSceneryLoadingStatus();
    }
  
    void OnUIRender() override
    {
        if (!m_IsVisible) return;
      
        if (ImGui::Begin("Scenery Browser", &m_IsVisible))
        {
            // Search bar
            ImGui::InputText("Search", m_SearchBuffer, sizeof(m_SearchBuffer));
          
            // Category filter
            ImGui::Combo("Category", &m_SelectedCategory, m_Categories, m_CategoryCount);
          
            // Scenery list
            if (ImGui::BeginChild("SceneryList"))
            {
                for (const auto& scenery : m_FilteredSceneries)
                {
                    if (ImGui::Selectable(scenery.name.c_str(), scenery.isSelected))
                    {
                        OnScenerySelected(scenery);
                    }
                  
                    // Right-click context menu
                    if (ImGui::BeginPopupContextItem())
                    {
                        if (ImGui::MenuItem("Load Scenery"))
                            LoadScenery(scenery);
                        if (ImGui::MenuItem("Properties"))
                            ShowSceneryProperties(scenery);
                        ImGui::EndPopup();
                    }
                }
            }
            ImGui::EndChild();
        }
        ImGui::End();
    }
  
    void OnEvent() override
    {
        // Handle keyboard shortcuts for scenery browser
        // Handle drag-and-drop events
    }

private:
    bool m_IsVisible = true;
    char m_SearchBuffer[256] = "";
    int m_SelectedCategory = 0;
    const char* m_Categories[5] = {"All", "Airports", "Objects", "Terrain", "Custom"};
    int m_CategoryCount = 5;
  
    struct SceneryItem
    {
        std::string name;
        std::string path;
        bool isSelected = false;
        bool isLoaded = false;
    };
  
    std::vector<SceneryItem> m_FilteredSceneries;
  
    void LoadSceneryDatabase() { /* Implementation */ }
    void InitializeFileWatchers() { /* Implementation */ }
    void CleanupFileWatchers() { /* Implementation */ }
    void SaveBrowserPreferences() { /* Implementation */ }
    void ProcessFileWatcherEvents() { /* Implementation */ }
    void UpdateSceneryLoadingStatus() { /* Implementation */ }
    void OnScenerySelected(const SceneryItem& scenery) { /* Implementation */ }
    void LoadScenery(const SceneryItem& scenery) { /* Implementation */ }
    void ShowSceneryProperties(const SceneryItem& scenery) { /* Implementation */ }
};
```

#### Example 3: Rendering Module Implementation

```cpp
class LightingModule : public Module
{
public:
    LightingModule() : Module("LightingModule") {}
  
    void OnAttach() override
    {
        SEDX_CORE_INFO("Lighting module attached - initializing lighting systems");
      
        // Initialize lighting shaders
        LoadLightingShaders();
      
        // Create lighting uniform buffers
        CreateLightingBuffers();
      
        // Set up default lighting
        SetupDefaultLighting();
      
        m_IsEnabled = true;
    }
  
    void OnDetach() override
    {
        SEDX_CORE_INFO("Lighting module detached - cleaning up lighting resources");
      
        // Cleanup lighting resources
        CleanupLightingBuffers();
        UnloadLightingShaders();
      
        m_IsEnabled = false;
    }
  
    void OnUpdate() override
    {
        if (!m_IsEnabled) return;
      
        // Update dynamic lighting
        UpdateDynamicLights();
      
        // Update shadow maps
        UpdateShadowMaps();
      
        // Update lighting uniforms
        UpdateLightingUniforms();
      
        // Update time-of-day lighting
        UpdateTimeOfDayLighting();
    }
  
    void OnUIRender() override
    {
        if (!m_IsEnabled) return;
      
        if (ImGui::Begin("Lighting Controls"))
        {
            // Sun settings
            if (ImGui::CollapsingHeader("Sun & Sky"))
            {
                ImGui::SliderFloat("Sun Elevation", &m_SunElevation, -90.0f, 90.0f);
                ImGui::SliderFloat("Sun Azimuth", &m_SunAzimuth, 0.0f, 360.0f);
                ImGui::ColorEdit3("Sun Color", &m_SunColor.x);
                ImGui::SliderFloat("Sun Intensity", &m_SunIntensity, 0.0f, 10.0f);
            }
          
            // Environment settings
            if (ImGui::CollapsingHeader("Environment"))
            {
                ImGui::SliderFloat("Ambient Light", &m_AmbientLight, 0.0f, 1.0f);
                ImGui::ColorEdit3("Sky Color", &m_SkyColor.x);
                ImGui::SliderFloat("Fog Density", &m_FogDensity, 0.0f, 1.0f);
            }
          
            // Shadow settings
            if (ImGui::CollapsingHeader("Shadows"))
            {
                ImGui::Checkbox("Enable Shadows", &m_ShadowsEnabled);
                ImGui::SliderInt("Shadow Map Size", &m_ShadowMapSize, 512, 4096);
                ImGui::SliderFloat("Shadow Distance", &m_ShadowDistance, 100.0f, 5000.0f);
                ImGui::SliderFloat("Shadow Bias", &m_ShadowBias, 0.0001f, 0.01f);
            }
        }
        ImGui::End();
    }
  
    void OnEvent() override
    {
        // Handle lighting-related events
        // Could process time changes, weather events, etc.
    }

private:
    bool m_IsEnabled = false;
  
    // Sun & Sky settings
    float m_SunElevation = 45.0f;
    float m_SunAzimuth = 180.0f;
    glm::vec3 m_SunColor = glm::vec3(1.0f, 0.9f, 0.8f);
    float m_SunIntensity = 3.0f;
  
    // Environment settings
    float m_AmbientLight = 0.1f;
    glm::vec3 m_SkyColor = glm::vec3(0.5f, 0.7f, 1.0f);
    float m_FogDensity = 0.01f;
  
    // Shadow settings
    bool m_ShadowsEnabled = true;
    int m_ShadowMapSize = 2048;
    float m_ShadowDistance = 1000.0f;
    float m_ShadowBias = 0.005f;
  
    void LoadLightingShaders() { /* Implementation */ }
    void CreateLightingBuffers() { /* Implementation */ }
    void SetupDefaultLighting() { /* Implementation */ }
    void CleanupLightingBuffers() { /* Implementation */ }
    void UnloadLightingShaders() { /* Implementation */ }
    void UpdateDynamicLights() { /* Implementation */ }
    void UpdateShadowMaps() { /* Implementation */ }
    void UpdateLightingUniforms() { /* Implementation */ }
    void UpdateTimeOfDayLighting() { /* Implementation */ }
};
```

### Best Practices

#### 1. **Initialization and Cleanup**

- Always pair resource allocation in `OnAttach()` with cleanup in `OnDetach()`
- Use RAII principles with smart pointers and SEDX memory management
- Log important lifecycle events for debugging

#### 2. **Performance Considerations**

- Keep `OnUpdate()` methods efficient as they're called every frame
- Use early returns when the module is disabled or not initialized
- Cache expensive calculations and update only when necessary

#### 3. **Error Handling**

- Use SEDX logging macros (`SEDX_CORE_INFO`, `SEDX_CORE_ERROR`, etc.)
- Handle edge cases gracefully in all virtual methods
- Validate module state before performing operations

#### 4. **Memory Management**

- Use the custom memory tracking system when available
- Follow the project's smart pointer conventions (`Ref<T>`, `Unique<T>`)
- Use tracked allocation macros in debug builds

#### 5. **Naming Conventions**

- Use descriptive module names that reflect their purpose
- Follow the project's naming conventions for member variables
- Use consistent naming for similar operations across modules

### Integration Examples

#### Example 1: Module Registration

```cpp
// In application initialization
void Application::InitializeModules()
{
    // Create and register core modules
    auto terrainModule = CreateRef<TerrainModule>();
    auto lightingModule = CreateRef<LightingModule>();
    auto sceneryBrowserModule = CreateRef<SceneryBrowserModule>();
  
    // Add modules to the stage
    m_ModuleStage.PushModule(terrainModule.get());
    m_ModuleStage.PushModule(lightingModule.get());
    m_ModuleStage.PushOverlay(sceneryBrowserModule.get()); // UI overlay
  
    SEDX_CORE_INFO("All modules initialized successfully");
}
```

#### Example 2: Module Communication

```cpp
class ModuleManager
{
public:
    template<typename T>
    T* GetModule(const std::string& name)
    {
        for (auto* module : m_ModuleStage)
        {
            if (auto* typedModule = dynamic_cast<T*>(module))
            {
                return typedModule;
            }
        }
        return nullptr;
    }
  
    void EnableModule(const std::string& name, bool enable)
    {
        // Implementation to enable/disable modules
    }
  
private:
    ModuleStage m_ModuleStage;
};

// Usage
auto* lightingModule = moduleManager.GetModule<LightingModule>("LightingModule");
if (lightingModule)
{
    lightingModule->SetSunElevation(45.0f);
}
```

This documentation provides comprehensive guidance for implementing and using the Module base class in Scenery Editor X, following the project's coding standards and architectural patterns.

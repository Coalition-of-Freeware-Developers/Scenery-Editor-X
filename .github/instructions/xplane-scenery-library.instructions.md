# X-Plane Scenery Library System Instructions for GitHub Copilot

## Overview

These instructions guide GitHub Copilot in properly utilizing the X-Plane Scenery Library system within Scenery Editor X. This system provides comprehensive support for X-Plane's complex asset management, virtual file systems, layer groups, and parsing capabilities.

## System Architecture

The X-Plane Scenery Library system consists of several interconnected components:

### Core Components

1. **XPAsset System** - Base asset abstraction and management
2. **XPObj System** - OBJ8 file parsing and representation
3. **XPLayerGroups** - Rendering layer management and organization
4. **XPLibraryPath** - Virtual path resolution and regional variants
5. **XPLibrarySystem** - Central virtual file system coordinator
6. **Parser Headers** - Convenient aggregation headers for easy inclusion

### Include Strategy

```cpp
// For complete X-Plane functionality
#include "X-PlaneSceneryLibraryParsers.h"
#include "X-PlaneSceneryLibraryUtils.h"

// For specific components only
#include "XPAsset.h"         // Base asset classes
#include "XPObj.h"           // OBJ8 file parsing
#include "XPLayerGroups.h"   // Layer group management
#include "XPLibraryPath.h"   // Path resolution system
#include "XPLibrarySystem.h" // Virtual file system
#include "TextUtils.h"       // Text processing utilities
```

## Development Guidelines

### 1. Module-Based Architecture

Always implement X-Plane functionality as modules inheriting from the base Module class:

```cpp
class XPlaneAssetModule : public Module
{
public:
    explicit XPlaneAssetModule() : Module("XPlaneAssetModule") {}
    
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("XPLANE", "Initializing X-Plane asset system");
        InitializeAssetSystem();
    }
    
    void OnDetach() override
    {
        SEDX_CORE_INFO_TAG("XPLANE", "Cleaning up X-Plane assets");
        // Smart pointers handle cleanup automatically
    }
    
    void OnUpdate() override
    {
        if (!m_IsEnabled) return;
        SEDX_PROFILE_SCOPE("XPlaneAssetModule::OnUpdate");
        
        UpdateAssetSystem();
    }
    
private:
    bool m_IsEnabled = true;
    std::vector<Ref<XPAsset::Asset>> m_Assets;
};
```

### 2. Smart Pointer Usage

Always use the custom smart pointer system for X-Plane assets:

```cpp
// Asset creation
auto obj = CreateRef<XPAsset::Obj>();
auto definition = CreateRef<XPLibrary::Definition>();

// Asset storage
std::vector<Ref<XPAsset::Asset>> m_Assets;
std::map<std::string, Ref<XPAsset::Obj>> m_ObjCache;

// Weak references for non-owning pointers
WeakRef<XPAsset::Asset> m_CurrentAsset;
```

### 3. Logging Standards

Use tagged logging for X-Plane operations:

```cpp
// System-level logging
SEDX_CORE_INFO_TAG("XPLANE", "Loading X-Plane asset: {}", assetPath);
SEDX_CORE_WARN_TAG("XPLANE", "Asset validation warning: {}", warning);
SEDX_CORE_ERROR_TAG("XPLANE", "Failed to load asset: {}", error);

// Component-specific logging
SEDX_CORE_INFO_TAG("XP_OBJ", "Parsed {} vertices, {} indices", vertexCount, indexCount);
SEDX_CORE_INFO_TAG("XP_LAYER", "Object assigned to layer group: {}", layerGroup);
SEDX_CORE_INFO_TAG("XP_LIB", "Resolved virtual path: {} -> {}", virtualPath, realPath);
```

### 4. Error Handling

Implement comprehensive error handling with proper validation:

```cpp
bool LoadXPlaneAsset(const std::filesystem::path& path)
{
    try
    {
        // Validate input
        SEDX_CORE_ASSERT(!path.empty(), "Asset path cannot be empty");
        if (!std::filesystem::exists(path))
        {
            SEDX_CORE_ERROR_TAG("XPLANE", "Asset file not found: {}", path.string());
            return false;
        }
        
        // Load asset
        auto asset = CreateRef<XPAsset::Obj>();
        if (!asset->Load(path))
        {
            SEDX_CORE_ERROR_TAG("XPLANE", "Failed to parse asset: {}", path.string());
            return false;
        }
        
        // Validate loaded data
        if (!ValidateAsset(asset))
        {
            SEDX_CORE_ERROR_TAG("XPLANE", "Asset validation failed: {}", path.string());
            return false;
        }
        
        return true;
    }
    catch (const std::exception& e)
    {
        SEDX_CORE_ERROR_TAG("XPLANE", "Exception loading asset {}: {}", path.string(), e.what());
        return false;
    }
}
```

## Component-Specific Guidelines

### XPAsset System

- **Base Class**: Always inherit from `XPAsset::Asset` for custom asset types
- **Properties**: Use the standard texture and material property system
- **Layer Groups**: Always set appropriate layer groups using `XPLayerGroups::Resolve()`

```cpp
class CustomAsset : public XPAsset::Asset
{
private:
    void MakeMeVirtual() override {} // Required for abstract base
    
public:
    CustomAsset(const std::filesystem::path& assetPath)
    {
        pReal = assetPath;
        intLayerGroup = XPLayerGroups::Resolve("objects", 0);
        
        // Configure textures
        pBaseTex = assetPath.parent_path() / "textures" / "diffuse.png";
        bHasBaseTex = std::filesystem::exists(pBaseTex);
    }
};
```

### XPObj System

- **Loading**: Always validate OBJ8 files before and after loading
- **Geometry**: Check vertex and index data integrity
- **Draw Calls**: Properly handle draped vs. non-draped geometry
- **Layer Groups**: Respect layer group assignments for proper rendering

```cpp
void ProcessObjFile(Ref<XPAsset::Obj>& obj)
{
    // Validate geometry
    SEDX_CORE_ASSERT(!obj->Vertices.empty(), "OBJ has no vertices");
    SEDX_CORE_ASSERT(!obj->Indices.empty(), "OBJ has no indices");
    
    // Process draw calls
    for (const auto& drawCall : obj->DrawCalls)
    {
        if (drawCall.bDraped)
        {
            ProcessDrapedGeometry(obj, drawCall);
        }
        else
        {
            ProcessRegularGeometry(obj, drawCall);
        }
    }
}
```

### XPLayerGroups System

- **Constants**: Use predefined constants instead of magic numbers
- **Resolution**: Always use `Resolve()` functions for layer calculations
- **Validation**: Verify layer assignments make sense for object types

```cpp
void ConfigureObjectLayering(Ref<XPAsset::Asset>& asset, const std::string& objectType)
{
    if (objectType == "terrain")
    {
        asset->intLayerGroup = XPLayerGroups::Resolve("terrain", 0);
    }
    else if (objectType == "runway")
    {
        asset->intLayerGroup = XPLayerGroups::Resolve("runways", 0);
    }
    else if (objectType == "building")
    {
        asset->intLayerGroup = XPLayerGroups::Resolve("objects", 2);
    }
    
    SEDX_CORE_INFO_TAG("XP_LAYER", "Assigned {} to layer: {}", objectType, asset->intLayerGroup);
}
```

### XPLibraryPath System

- **Virtual Paths**: Always validate virtual path format
- **Regional Variants**: Handle geographic and conditional constraints
- **Seasonal Support**: Implement proper season-based asset selection
- **Weighted Selection**: Use the weighted random selection system

```cpp
XPLibrary::DefinitionPath ResolveAssetPath(const std::string& virtualPath,
                                          double latitude, double longitude,
                                          char season = 'd')
{
    // Find definition
    auto definition = FindDefinition(virtualPath);
    if (definition.pVirtual.empty())
    {
        SEDX_CORE_ERROR_TAG("XP_LIB", "Virtual path not found: {}", virtualPath);
        return {};
    }
    
    // Find regional match
    auto regional = FindBestRegionalMatch(definition, latitude, longitude);
    if (!regional.has_value())
    {
        SEDX_CORE_ERROR_TAG("XP_LIB", "No regional match for: {}, {}", latitude, longitude);
        return {};
    }
    
    // Get seasonal variant
    return GetSeasonalVariant(regional.value(), season);
}
```

### XPLibrarySystem

- **Initialization**: Properly set up X-Plane root and scenery pack paths
- **Priority Order**: Respect scenery pack loading order from scenery_packs.ini
- **File Validation**: Validate all referenced files exist
- **Performance**: Use caching for frequently accessed definitions

```cpp
class LibrarySystemModule : public Module
{
public:
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("XP_LIB", "Initializing X-Plane library system");
        
        try
        {
            // Detect X-Plane installation
            DetectXPlaneInstallation();
            
            // Load scenery pack order
            LoadSceneryPackOrder();
            
            // Initialize virtual file system
            m_VFS.LoadFileSystem(m_XPlaneRoot, m_CurrentPackage, m_SceneryPacks);
            
            SEDX_CORE_INFO_TAG("XP_LIB", "Library system initialized successfully");
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("XP_LIB", "Failed to initialize: {}", e.what());
            throw;
        }
    }
    
private:
    XPLibrary::VirtualFileSystem m_VFS;
    std::filesystem::path m_XPlaneRoot;
    std::filesystem::path m_CurrentPackage;
    std::vector<std::filesystem::path> m_SceneryPacks;
};
```

## Performance Guidelines

### 1. Asset Caching

Implement caching for frequently accessed assets:

```cpp
class AssetCache
{
private:
    std::unordered_map<std::string, WeakRef<XPAsset::Asset>> m_Cache;
    
public:
    Ref<XPAsset::Asset> GetOrLoad(const std::string& path)
    {
        // Check cache first
        if (auto cached = m_Cache[path].Lock())
        {
            return cached;
        }
        
        // Load new asset
        auto asset = LoadAsset(path);
        if (asset)
        {
            m_Cache[path] = asset;
        }
        
        return asset;
    }
};
```

### 2. Layer Group Optimization

Cache layer group calculations:

```cpp
class LayerGroupCache
{
private:
    static std::map<std::pair<std::string, int>, int> s_Cache;
    
public:
    static int GetCachedLayerGroup(const std::string& group, int offset)
    {
        auto key = std::make_pair(group, offset);
        auto it = s_Cache.find(key);
        
        if (it != s_Cache.end())
        {
            return it->second;
        }
        
        int result = XPLayerGroups::Resolve(group, offset);
        s_Cache[key] = result;
        return result;
    }
};
```

### 3. Batch Processing

Process multiple assets in batches for efficiency:

```cpp
void ProcessAssetBatch(const std::vector<std::filesystem::path>& assetPaths)
{
    SEDX_PROFILE_SCOPE("ProcessAssetBatch");
    
    // Group by type for optimized processing
    std::map<std::string, std::vector<std::filesystem::path>> groupedPaths;
    
    for (const auto& path : assetPaths)
    {
        groupedPaths[path.extension().string()].push_back(path);
    }
    
    // Process each type in batch
    for (const auto& [extension, paths] : groupedPaths)
    {
        ProcessAssetTypeGroup(extension, paths);
    }
}
```

## Debug and Diagnostics

### 1. Debug UI Implementation

Provide debug interfaces for X-Plane systems:

```cpp
void OnUIRender() override
{
    if (m_ShowXPlaneDebug)
    {
        ImGui::Begin("X-Plane System Debug", &m_ShowXPlaneDebug);
        
        // Asset statistics
        if (ImGui::CollapsingHeader("Asset Statistics"))
        {
            ImGui::Text("Loaded Assets: %zu", m_Assets.size());
            ImGui::Text("Cache Hits: %zu", m_CacheHits);
            ImGui::Text("Cache Misses: %zu", m_CacheMisses);
        }
        
        // Layer group calculator
        if (ImGui::CollapsingHeader("Layer Group Calculator"))
        {
            static char groupName[64] = "objects";
            static int offset = 0;
            
            ImGui::InputText("Group", groupName, sizeof(groupName));
            ImGui::InputInt("Offset", &offset);
            
            if (ImGui::Button("Calculate"))
            {
                int layer = XPLayerGroups::Resolve(groupName, offset);
                ImGui::Text("Result: %d", layer);
            }
        }
        
        ImGui::End();
    }
}
```

### 2. System Validation

Implement comprehensive system validation:

```cpp
bool ValidateXPlaneSystem()
{
    bool isValid = true;
    
    // Validate X-Plane installation
    if (!ValidateXPlaneInstallation())
    {
        SEDX_CORE_ERROR_TAG("XP_VALIDATE", "Invalid X-Plane installation");
        isValid = false;
    }
    
    // Validate scenery packs
    if (!ValidateSceneryPacks())
    {
        SEDX_CORE_ERROR_TAG("XP_VALIDATE", "Invalid scenery pack configuration");
        isValid = false;
    }
    
    // Validate library definitions
    if (!ValidateLibraryDefinitions())
    {
        SEDX_CORE_ERROR_TAG("XP_VALIDATE", "Invalid library definitions");
        isValid = false;
    }
    
    return isValid;
}
```

## Integration Patterns

### 1. With Rendering System

```cpp
class XPlaneRenderer : public Module
{
public:
    void RenderXPlaneAssets()
    {
        // Sort by layer group for proper depth ordering
        SortAssetsByLayerGroup();
        
        // Render each layer group
        for (const auto& [layerGroup, assets] : m_LayeredAssets)
        {
            SetupLayerGroupRendering(layerGroup);
            
            for (const auto& asset : assets)
            {
                RenderAsset(asset);
            }
        }
    }
    
private:
    void SetupLayerGroupRendering(int layerGroup)
    {
        float depthOffset = layerGroup * 0.001f;
        SetDepthOffset(depthOffset);
        
        // Configure blend modes based on layer group
        auto layerInfo = XPLayerGroups::Resolve(layerGroup);
        ConfigureBlendModeForLayer(layerInfo.first);
    }
};
```

### 2. With Configuration System

```cpp
class XPlaneConfiguration
{
public:
    struct Config
    {
        std::filesystem::path xplaneRoot;
        std::vector<std::filesystem::path> sceneryPacks;
        bool enableSeasonalVariants = true;
        bool enableRegionalVariants = true;
        
        void ToJSON(nlohmann::json& j) const
        {
            j["xplaneRoot"] = xplaneRoot.string();
            j["enableSeasonalVariants"] = enableSeasonalVariants;
            j["enableRegionalVariants"] = enableRegionalVariants;
        }
        
        void FromJSON(const nlohmann::json& j)
        {
            xplaneRoot = j.value("xplaneRoot", "");
            enableSeasonalVariants = j.value("enableSeasonalVariants", true);
            enableRegionalVariants = j.value("enableRegionalVariants", true);
        }
    };
};
```

## Common Patterns and Anti-Patterns

### ✅ Correct Patterns

```cpp
// Use smart pointers
auto asset = CreateRef<XPAsset::Obj>();

// Use tagged logging
SEDX_CORE_INFO_TAG("XPLANE", "Operation completed");

// Use module architecture
class XPlaneModule : public Module {};

// Use proper error handling
if (!asset->Load(path))
{
    SEDX_CORE_ERROR_TAG("XPLANE", "Load failed: {}", path.string());
    return false;
}
```

### ❌ Anti-Patterns to Avoid

```cpp
// Don't use raw pointers
XPAsset::Obj* asset = new XPAsset::Obj(); // ❌

// Don't use magic numbers for layer groups
asset->intLayerGroup = 93; // ❌ Use XPLayerGroups::OBJECTS instead

// Don't ignore error conditions
asset->Load(path); // ❌ Always check return values

// Don't use untagged logging
SEDX_CORE_INFO("Something happened"); // ❌ Use tagged logging
```

## File Organization

When working with X-Plane systems, organize files logically:

```
source/XPlane/
├── Assets/          # Asset management modules
├── LayerGroups/     # Layer group utilities  
├── Library/         # Library system modules
├── Parsing/         # File parsing modules
└── Utils/           # Utility functions
```

Follow these guidelines to ensure consistent, maintainable, and efficient integration of X-Plane scenery library functionality within Scenery Editor X.

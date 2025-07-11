# X-Plane Scenery Library - Parser Headers Documentation

## Overview

The X-Plane Scenery Library Parser Headers provide convenient single-include access to all parsing functionality within the library system. These headers are designed to simplify integration and provide organized access to the various parsing components.

## Header Organization

### X-PlaneSceneryLibraryParsers.h

The main parser aggregation header that includes all asset parsing functionality.

```cpp
//Module:  X-PlaneSceneryLibraryParsers
//Author:  Connor Russell
//Date:    10/8/2024 8:36:19 PM
//Purpose: Provides a single header that includes all the parser functions from the library

#pragma once

//Generic utilities
#include "XPLayerGroups.h"

//Asset parsers/data structures
#include "XPAsset.h"
#include "XPObj.h"
```

### X-PlaneSceneryLibraryUtils.h

The utility aggregation header that includes all utility functions from the library.

```cpp
//Module:  X-PlaneSceneryLibraryUtils
//Author:  Connor Russell
//Date:    10/8/2024 8:36:09 PM
//Purpose: Provides a single header that includes all the utility functions from the library

#pragma once

//Include necessary headers
#include "TextUtils.h"
```

## Usage Examples

### Basic Include Strategy

```cpp
// In your module that needs X-Plane parsing functionality
#include "X-PlaneSceneryLibraryParsers.h"
#include "X-PlaneSceneryLibraryUtils.h"

class XPlaneSceneryModule : public Module
{
public:
    explicit XPlaneSceneryModule() : Module("XPlaneSceneryModule") {}
    
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("XPLANE", "Initializing X-Plane scenery parsing");
        
        // Now have access to all parsing functionality
        LoadSceneryAssets();
        ProcessLayerGroups();
        PerformTextProcessing();
    }
    
private:
    void LoadSceneryAssets()
    {
        // XPAsset functionality available
        auto asset = CreateRef<XPAsset::Obj>();
        
        // XPObj parsing available
        if (asset->Load("path/to/scenery.obj"))
        {
            SEDX_CORE_INFO_TAG("XPLANE", "Loaded scenery object successfully");
        }
    }
    
    void ProcessLayerGroups()
    {
        // XPLayerGroups functionality available
        int terrainLayer = XPLayerGroups::Resolve("terrain", 0);
        int objectLayer = XPLayerGroups::Resolve("objects", 3);
        
        SEDX_CORE_INFO_TAG("XPLANE", "Terrain layer: {}, Object layer: {}", 
            terrainLayer, objectLayer);
    }
    
    void PerformTextProcessing()
    {
        // TextUtils functionality available
        std::string testString = "  test string with whitespace  ";
        std::string trimmed = TextUtils::TrimWhitespace(testString);
        
        SEDX_CORE_INFO_TAG("XPLANE", "Trimmed string: '{}'", trimmed);
    }
};
```

### Selective Include Strategy

For modules that only need specific functionality, you can include individual headers:

```cpp
// Module that only needs layer group functionality
#include "XPLayerGroups.h"

class LayerManagementModule : public Module
{
public:
    void ConfigureObjectLayers()
    {
        // Only layer group functionality available
        int layer = XPLayerGroups::Resolve("objects", 2);
        ConfigureRenderLayer(layer);
    }
};
```

```cpp
// Module that only needs OBJ parsing
#include "XPObj.h"
#include "XPAsset.h"  // Required dependency

class ObjParsingModule : public Module
{
public:
    void LoadObjFile(const std::filesystem::path& path)
    {
        auto obj = CreateRef<XPAsset::Obj>();
        if (obj->Load(path))
        {
            ProcessObjData(obj);
        }
    }
};
```

### Advanced Integration Example

```cpp
#include "X-PlaneSceneryLibraryParsers.h"
#include "X-PlaneSceneryLibraryUtils.h"

class ComprehensiveXPlaneModule : public Module
{
public:
    explicit ComprehensiveXPlaneModule() : Module("ComprehensiveXPlaneModule") {}
    
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("XPLANE", "Initializing comprehensive X-Plane integration");
        
        // Initialize all subsystems
        InitializeAssetParsing();
        InitializeLayerManagement();
        InitializeTextProcessing();
    }
    
    void OnUpdate() override
    {
        if (!m_IsEnabled) return;
        SEDX_PROFILE_SCOPE("ComprehensiveXPlaneModule::OnUpdate");
        
        // Update all subsystems
        UpdateAssetLoading();
        UpdateLayerManagement();
    }
    
    void OnUIRender() override
    {
        if (m_ShowDebugWindow)
        {
            RenderXPlaneDebugInterface();
        }
    }
    
private:
    bool m_IsEnabled = true;
    bool m_ShowDebugWindow = false;
    
    // Asset management
    std::vector<Ref<XPAsset::Obj>> m_LoadedObjects;
    std::map<int, std::vector<Ref<XPAsset::Asset>>> m_LayeredAssets;
    
    void InitializeAssetParsing()
    {
        SEDX_CORE_INFO_TAG("XPLANE", "Initializing asset parsing subsystem");
        
        // Set up asset loading pipeline
        SetupAssetLoadingPipeline();
    }
    
    void InitializeLayerManagement()
    {
        SEDX_CORE_INFO_TAG("XPLANE", "Initializing layer management subsystem");
        
        // Configure all layer groups
        ConfigureAllLayerGroups();
    }
    
    void InitializeTextProcessing()
    {
        SEDX_CORE_INFO_TAG("XPLANE", "Initializing text processing subsystem");
        
        // Set up text utilities
        SetupTextProcessing();
    }
    
    void SetupAssetLoadingPipeline()
    {
        // Asset loading configuration
        m_LoadedObjects.reserve(1000); // Reserve space for expected objects
        
        SEDX_CORE_INFO_TAG("XPLANE", "Asset loading pipeline configured");
    }
    
    void ConfigureAllLayerGroups()
    {
        // Configure standard layer groups
        struct LayerConfig
        {
            std::string name;
            int baseLayer;
            std::string description;
        };
        
        std::vector<LayerConfig> configs = {
            {"terrain", XPLayerGroups::TERRAIN, "Base terrain surfaces"},
            {"beaches", XPLayerGroups::BEACHES, "Shoreline features"},
            {"shoulders", XPLayerGroups::SHOULDERS, "Road/runway shoulders"},
            {"taxiways", XPLayerGroups::TAXIWAYS, "Airport taxiway surfaces"},
            {"runways", XPLayerGroups::RUNWAYS, "Airport runway surfaces"},
            {"markings", XPLayerGroups::MARKINGS, "Surface markings"},
            {"airports", XPLayerGroups::AIRPORTS, "Airport buildings"},
            {"roads", XPLayerGroups::ROADS, "Road infrastructure"},
            {"objects", XPLayerGroups::OBJECTS, "General 3D objects"},
            {"light_objects", XPLayerGroups::LIGHT_OBJECTS, "Lighting elements"},
            {"cars", XPLayerGroups::CARS, "Animated vehicles"}
        };
        
        for (const auto& config : configs)
        {
            m_LayeredAssets[config.baseLayer] = {};
            SEDX_CORE_INFO_TAG("XPLANE", "Configured layer '{}' at {}: {}", 
                config.name, config.baseLayer, config.description);
        }
    }
    
    void SetupTextProcessing()
    {
        // Text processing utilities are header-only, no setup needed
        SEDX_CORE_INFO_TAG("XPLANE", "Text processing utilities ready");
    }
    
    void UpdateAssetLoading()
    {
        // Process any pending asset loading operations
        ProcessPendingAssetLoads();
    }
    
    void UpdateLayerManagement()
    {
        // Update layer-based rendering organization
        OrganizeAssetsByLayer();
    }
    
    void ProcessPendingAssetLoads()
    {
        // Asset loading logic using XPObj and XPAsset functionality
        // This would process a queue of assets to load
    }
    
    void OrganizeAssetsByLayer()
    {
        // Layer organization using XPLayerGroups functionality
        for (auto& obj : m_LoadedObjects)
        {
            int layerGroup = obj->intLayerGroup;
            
            // Ensure layer group exists in our map
            if (m_LayeredAssets.find(layerGroup) == m_LayeredAssets.end())
            {
                m_LayeredAssets[layerGroup] = {};
            }
            
            // Add to appropriate layer (if not already there)
            auto& layerAssets = m_LayeredAssets[layerGroup];
            if (std::find(layerAssets.begin(), layerAssets.end(), obj) == layerAssets.end())
            {
                layerAssets.push_back(obj);
            }
        }
    }
    
    void RenderXPlaneDebugInterface()
    {
        ImGui::Begin("X-Plane Integration Debug", &m_ShowDebugWindow);
        
        ImGui::Text("X-Plane Scenery System Status");
        ImGui::Separator();
        
        // Asset statistics
        if (ImGui::CollapsingHeader("Asset Statistics"))
        {
            ImGui::Text("Total Loaded Objects: %zu", m_LoadedObjects.size());
            
            for (const auto& [layerGroup, assets] : m_LayeredAssets)
            {
                auto layerInfo = XPLayerGroups::Resolve(layerGroup);
                ImGui::Text("Layer %s (%d): %zu assets", 
                    layerInfo.first.c_str(), layerGroup, assets.size());
            }
        }
        
        // Layer group utilities
        if (ImGui::CollapsingHeader("Layer Group Calculator"))
        {
            RenderLayerGroupCalculator();
        }
        
        // Text processing utilities
        if (ImGui::CollapsingHeader("Text Processing Test"))
        {
            RenderTextProcessingTest();
        }
        
        // Asset loading controls
        if (ImGui::CollapsingHeader("Asset Loading"))
        {
            RenderAssetLoadingControls();
        }
        
        ImGui::End();
    }
    
    void RenderLayerGroupCalculator()
    {
        static char layerGroupName[64] = "objects";
        static int layerOffset = 0;
        static int calculatedLayer = 0;
        
        ImGui::InputText("Layer Group", layerGroupName, sizeof(layerGroupName));
        ImGui::InputInt("Offset", &layerOffset);
        
        if (ImGui::Button("Calculate Layer"))
        {
            calculatedLayer = XPLayerGroups::Resolve(std::string(layerGroupName), layerOffset);
        }
        
        ImGui::Text("Calculated Layer: %d", calculatedLayer);
        
        // Reverse calculation
        static int reverseLayer = 93;
        
        ImGui::InputInt("Reverse Layer", &reverseLayer);
        
        if (ImGui::Button("Resolve Layer"))
        {
            auto resolved = XPLayerGroups::Resolve(reverseLayer);
            ImGui::Text("Resolved: %s + %d", resolved.first.c_str(), resolved.second);
        }
    }
    
    void RenderTextProcessingTest()
    {
        static char testString[256] = "  test string with whitespace  ";
        
        ImGui::InputText("Test String", testString, sizeof(testString));
        
        if (ImGui::Button("Trim Whitespace"))
        {
            std::string input(testString);
            std::string trimmed = TextUtils::TrimWhitespace(input);
            strncpy_s(testString, trimmed.c_str(), sizeof(testString) - 1);
        }
        
        // Tokenization test
        static char delimiterString[64] = " ,;";
        
        ImGui::InputText("Delimiters", delimiterString, sizeof(delimiterString));
        
        if (ImGui::Button("Tokenize"))
        {
            std::string input(testString);
            std::vector<char> delims;
            for (char c : std::string(delimiterString))
            {
                if (c != '\0') delims.push_back(c);
            }
            
            auto tokens = TextUtils::TokenizeString(input, delims);
            
            SEDX_CORE_INFO_TAG("XPLANE", "Tokenized into {} tokens:", tokens.size());
            for (size_t i = 0; i < tokens.size(); ++i)
            {
                SEDX_CORE_INFO_TAG("XPLANE", "  Token {}: '{}'", i, tokens[i]);
            }
        }
    }
    
    void RenderAssetLoadingControls()
    {
        static char assetPath[512] = "";
        
        ImGui::InputText("Asset Path", assetPath, sizeof(assetPath));
        
        if (ImGui::Button("Load OBJ Asset"))
        {
            LoadObjAsset(assetPath);
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Clear All Assets"))
        {
            ClearAllAssets();
        }
        
        ImGui::Text("Memory Usage: ~%zu KB", 
            (m_LoadedObjects.size() * sizeof(XPAsset::Obj)) / 1024);
    }
    
    void LoadObjAsset(const std::string& path)
    {
        if (path.empty()) return;
        
        try
        {
            auto obj = CreateRef<XPAsset::Obj>();
            
            if (obj->Load(std::filesystem::path(path)))
            {
                m_LoadedObjects.push_back(obj);
                SEDX_CORE_INFO_TAG("XPLANE", "Successfully loaded asset: {}", path);
                SEDX_CORE_INFO_TAG("XPLANE", "  Vertices: {}, Indices: {}, Draw Calls: {}", 
                    obj->Vertices.size(), obj->Indices.size(), obj->DrawCalls.size());
            }
            else
            {
                SEDX_CORE_ERROR_TAG("XPLANE", "Failed to load asset: {}", path);
            }
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("XPLANE", "Exception loading asset {}: {}", path, e.what());
        }
    }
    
    void ClearAllAssets()
    {
        m_LoadedObjects.clear();
        m_LayeredAssets.clear();
        ConfigureAllLayerGroups(); // Reinitialize layer groups
        
        SEDX_CORE_INFO_TAG("XPLANE", "Cleared all loaded assets");
    }
};
```

## Integration Guidelines

### 1. Header Inclusion Strategy

```cpp
// For full X-Plane functionality
#include "X-PlaneSceneryLibraryParsers.h"
#include "X-PlaneSceneryLibraryUtils.h"

// For specific functionality only
#include "XPObj.h"           // OBJ parsing only
#include "XPLayerGroups.h"   // Layer groups only
#include "TextUtils.h"       // Text utilities only
```

### 2. Namespace Usage

```cpp
// All X-Plane functionality is in appropriate namespaces
using namespace XPAsset;        // For asset classes
using namespace XPLayerGroups;  // For layer group functions
using namespace TextUtils;      // For text utility functions
```

### 3. Error Handling

```cpp
class SafeXPlaneIntegration
{
public:
    static bool LoadXPlaneAssetSafely(const std::filesystem::path& path)
    {
        try
        {
            // Validate inputs
            if (path.empty() || !std::filesystem::exists(path))
            {
                SEDX_CORE_ERROR_TAG("XPLANE", "Invalid path: {}", path.string());
                return false;
            }
            
            // Check file extension
            if (path.extension() != ".obj")
            {
                SEDX_CORE_ERROR_TAG("XPLANE", "Unsupported file type: {}", 
                    path.extension().string());
                return false;
            }
            
            // Attempt to load
            auto obj = CreateRef<XPAsset::Obj>();
            if (!obj->Load(path))
            {
                SEDX_CORE_ERROR_TAG("XPLANE", "Failed to parse OBJ file: {}", path.string());
                return false;
            }
            
            // Validate loaded data
            if (!ValidateObjData(obj))
            {
                SEDX_CORE_ERROR_TAG("XPLANE", "Invalid OBJ data in: {}", path.string());
                return false;
            }
            
            SEDX_CORE_INFO_TAG("XPLANE", "Successfully loaded X-Plane asset: {}", path.string());
            return true;
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("XPLANE", "Exception loading X-Plane asset {}: {}", 
                path.string(), e.what());
            return false;
        }
    }
    
private:
    static bool ValidateObjData(const Ref<XPAsset::Obj>& obj)
    {
        // Validate geometry
        if (obj->Vertices.empty())
        {
            SEDX_CORE_ERROR_TAG("XPLANE", "OBJ has no vertices");
            return false;
        }
        
        if (obj->Indices.empty())
        {
            SEDX_CORE_ERROR_TAG("XPLANE", "OBJ has no indices");
            return false;
        }
        
        // Validate indices are within bounds
        size_t maxIndex = obj->Vertices.size() - 1;
        for (size_t idx : obj->Indices)
        {
            if (idx > maxIndex)
            {
                SEDX_CORE_ERROR_TAG("XPLANE", "Index {} out of bounds (max: {})", idx, maxIndex);
                return false;
            }
        }
        
        return true;
    }
};
```

## Best Practices

### 1. Include Only What You Need

```cpp
// Good: Specific includes for specific functionality
#include "XPLayerGroups.h"    // Only need layer groups

class LayerOnlyModule : public Module
{
    // Use only layer group functionality
};

// Avoid: Including everything when you only need specific parts
// #include "X-PlaneSceneryLibraryParsers.h"  // Overkill for layer-only module
```

### 2. Proper Module Organization

```cpp
// Organize X-Plane functionality into focused modules
class XPlaneAssetModule : public Module
{
    // Handles asset loading and management
};

class XPlaneLayerModule : public Module
{
    // Handles layer group management
};

class XPlaneTextModule : public Module
{
    // Handles text processing for X-Plane files
};
```

### 3. Performance Considerations

```cpp
class OptimizedXPlaneUsage
{
public:
    // Cache frequently used layer calculations
    static int GetCachedLayerGroup(const std::string& group, int offset)
    {
        static std::map<std::pair<std::string, int>, int> cache;
        
        auto key = std::make_pair(group, offset);
        auto it = cache.find(key);
        
        if (it != cache.end())
        {
            return it->second;
        }
        
        int result = XPLayerGroups::Resolve(group, offset);
        cache[key] = result;
        return result;
    }
    
    // Batch text processing operations
    static std::vector<std::string> ProcessTextBatch(const std::vector<std::string>& inputs)
    {
        std::vector<std::string> results;
        results.reserve(inputs.size());
        
        for (const auto& input : inputs)
        {
            results.push_back(TextUtils::TrimWhitespace(input));
        }
        
        return results;
    }
};
```

These parser headers provide a clean, organized interface to all X-Plane scenery library functionality while maintaining the modular architecture principles of Scenery Editor X.

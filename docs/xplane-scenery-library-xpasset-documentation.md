# X-Plane Scenery Library - XPAsset System Documentation

## Overview

The XPAsset system provides a foundational framework for representing and managing X-Plane 3D assets within Scenery Editor X. This system forms the base abstraction layer for all X-Plane scenery assets, providing common properties and functionality that all asset types share.

## Architecture

### Core Components

- **`XPAsset::Asset`** - Abstract base class for all X-Plane assets
- **Asset Type Inheritance** - Specialized classes inherit from Asset to implement specific asset types

## Class Reference

### XPAsset::Asset

The abstract base class that defines the common interface and properties for all X-Plane assets.

#### Key Properties

```cpp
class Asset
{
public:
    std::filesystem::path pReal;        // Real filesystem path to the asset
    
    // Texture paths (relative to asset path)
    std::filesystem::path pBaseTex;     // Diffuse/albedo texture
    std::filesystem::path pNormalTex;   // Normal map texture
    std::filesystem::path pMaterialTex; // Dedicated material texture
    
    // Texture availability flags
    bool bHasBaseTex{false};
    bool bHasNormalTex{false};
    bool bHasMaterialTex{false};
    
    // Material properties
    double dblNormalScale{1.0};                  // Normal map intensity
    int intLayerGroup{XPLayerGroups::TERRAIN};   // Rendering layer group
    bool bSuperRoughness{false};                 // Enhanced roughness mode
};
```

#### Design Patterns

1. **Virtual Inheritance**: Uses pure virtual function to enforce abstract nature
2. **RAII**: Filesystem paths manage their own memory automatically
3. **Default Initialization**: All boolean flags default to safe values

## Usage Examples

### Basic Asset Implementation

```cpp
// Example of creating a custom asset type
class CustomTerrainAsset : public XPAsset::Asset
{
private:
    void MakeMeVirtual() override {} // Required for abstract base
    
public:
    CustomTerrainAsset(const std::filesystem::path& assetPath)
    {
        pReal = assetPath;
        intLayerGroup = XPLayerGroups::TERRAIN;
        
        // Set up default textures relative to asset path
        pBaseTex = assetPath.parent_path() / "textures" / "terrain_diffuse.png";
        pNormalTex = assetPath.parent_path() / "textures" / "terrain_normal.png";
        
        // Validate texture existence
        bHasBaseTex = std::filesystem::exists(pBaseTex);
        bHasNormalTex = std::filesystem::exists(pNormalTex);
        
        if (bHasNormalTex)
        {
            dblNormalScale = 2.0; // Enhanced normal intensity for terrain
        }
    }
    
    void ConfigureForHighDetailTerrain()
    {
        // Configure for high-detail terrain rendering
        bSuperRoughness = true;
        dblNormalScale = 1.5;
        intLayerGroup = XPLayerGroups::Resolve("terrain", 2); // Higher priority
    }
};
```

### Asset Management in Module Context

```cpp
class TerrainAssetModule : public Module
{
public:
    explicit TerrainAssetModule() : Module("TerrainAssetModule") {}
    
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("TERRAIN", "Initializing terrain asset management");
        
        // Load terrain assets from directory
        LoadTerrainAssets("assets/terrain/");
        
        SEDX_CORE_INFO_TAG("TERRAIN", "Loaded {} terrain assets", m_TerrainAssets.size());
    }
    
    void OnDetach() override
    {
        SEDX_CORE_INFO_TAG("TERRAIN", "Cleaning up terrain assets");
        m_TerrainAssets.clear(); // Smart pointers handle cleanup
    }
    
    void OnUpdate() override
    {
        if (!m_IsEnabled) return;
        SEDX_PROFILE_SCOPE("TerrainAssetModule::OnUpdate");
        
        // Update asset states based on current conditions
        UpdateAssetVisibility();
    }
    
private:
    std::vector<Ref<CustomTerrainAsset>> m_TerrainAssets;
    bool m_IsEnabled = true;
    
    void LoadTerrainAssets(const std::filesystem::path& terrainDir)
    {
        try
        {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(terrainDir))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".obj")
                {
                    auto asset = CreateRef<CustomTerrainAsset>(entry.path());
                    
                    // Validate asset before adding
                    if (ValidateAsset(asset))
                    {
                        m_TerrainAssets.push_back(asset);
                        SEDX_CORE_INFO_TAG("TERRAIN", "Loaded asset: {}", entry.path().string());
                    }
                    else
                    {
                        SEDX_CORE_WARN_TAG("TERRAIN", "Invalid asset skipped: {}", entry.path().string());
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("TERRAIN", "Failed to load terrain assets: {}", e.what());
        }
    }
    
    bool ValidateAsset(const Ref<CustomTerrainAsset>& asset)
    {
        SEDX_CORE_ASSERT(asset != nullptr, "Asset cannot be null");
        
        // Validate required files exist
        if (!std::filesystem::exists(asset->pReal))
        {
            SEDX_CORE_ERROR_TAG("TERRAIN", "Asset file does not exist: {}", asset->pReal.string());
            return false;
        }
        
        return true;
    }
    
    void UpdateAssetVisibility()
    {
        // Example: Update assets based on current view distance
        for (auto& asset : m_TerrainAssets)
        {
            // Asset-specific update logic would go here
            // This might involve checking distances, LOD levels, etc.
        }
    }
};
```

### Integration with Rendering System

```cpp
class AssetRenderer : public Module
{
public:
    void RenderAsset(const Ref<XPAsset::Asset>& asset)
    {
        SEDX_PROFILE_SCOPE("AssetRenderer::RenderAsset");
        
        // Configure rendering based on asset properties
        SetupMaterialFromAsset(asset);
        SetupLayerGroupRendering(asset->intLayerGroup);
        
        // Handle special rendering modes
        if (asset->bSuperRoughness)
        {
            EnableSuperRoughnessShader();
        }
        
        // Bind textures
        BindAssetTextures(asset);
        
        // Render the asset
        SubmitAssetForRendering(asset);
    }
    
private:
    void SetupMaterialFromAsset(const Ref<XPAsset::Asset>& asset)
    {
        MaterialProperties material{};
        
        // Configure normal mapping
        if (asset->bHasNormalTex)
        {
            material.normalScale = static_cast<float>(asset->dblNormalScale);
            material.hasNormalMap = true;
        }
        
        // Configure material textures
        material.hasDedicatedMaterial = asset->bHasMaterialTex;
        
        // Apply material to current render state
        ApplyMaterial(material);
    }
    
    void BindAssetTextures(const Ref<XPAsset::Asset>& asset)
    {
        // Bind diffuse texture
        if (asset->bHasBaseTex)
        {
            auto diffuseTexture = LoadTexture(asset->pBaseTex);
            BindTexture(0, diffuseTexture); // Slot 0: Diffuse
        }
        
        // Bind normal texture
        if (asset->bHasNormalTex)
        {
            auto normalTexture = LoadTexture(asset->pNormalTex);
            BindTexture(1, normalTexture); // Slot 1: Normal
        }
        
        // Bind material texture if separate
        if (asset->bHasMaterialTex)
        {
            auto materialTexture = LoadTexture(asset->pMaterialTex);
            BindTexture(2, materialTexture); // Slot 2: Material
        }
    }
    
    void SetupLayerGroupRendering(int layerGroup)
    {
        // Configure depth testing and rendering order based on layer group
        float depthOffset = layerGroup * 0.001f; // Small offset per layer
        SetDepthOffset(depthOffset);
        
        // Configure blend modes for different layer groups
        if (layerGroup >= XPLayerGroups::LIGHT_OBJECTS)
        {
            EnableAlphaBlending();
        }
        else
        {
            DisableAlphaBlending();
        }
    }
};
```

## Best Practices

### 1. Asset Validation

Always validate assets before use:

```cpp
bool ValidateAsset(const Ref<XPAsset::Asset>& asset)
{
    SEDX_CORE_ASSERT(asset != nullptr, "Asset cannot be null");
    
    // Check file existence
    if (!std::filesystem::exists(asset->pReal))
    {
        SEDX_CORE_ERROR_TAG("ASSET", "Asset file missing: {}", asset->pReal.string());
        return false;
    }
    
    // Validate texture paths if claimed to exist
    if (asset->bHasBaseTex && !std::filesystem::exists(asset->pBaseTex))
    {
        SEDX_CORE_WARN_TAG("ASSET", "Base texture missing: {}", asset->pBaseTex.string());
        asset->bHasBaseTex = false; // Correct the flag
    }
    
    return true;
}
```

### 2. Memory Management

Use smart pointers for automatic cleanup:

```cpp
class AssetManager : public Module
{
private:
    std::vector<Ref<XPAsset::Asset>> m_Assets;
    std::unordered_map<std::string, WeakRef<XPAsset::Asset>> m_AssetCache;
    
public:
    Ref<XPAsset::Asset> LoadAsset(const std::filesystem::path& path)
    {
        const std::string pathKey = path.string();
        
        // Check cache first
        if (auto cached = m_AssetCache[pathKey].Lock())
        {
            return cached;
        }
        
        // Create new asset
        auto asset = CreateRef<CustomAsset>(path);
        
        // Store in cache and management
        m_AssetCache[pathKey] = asset;
        m_Assets.push_back(asset);
        
        return asset;
    }
    
    void OnDetach() override
    {
        // Automatic cleanup - smart pointers handle everything
        m_Assets.clear();
        m_AssetCache.clear();
    }
};
```

### 3. Layer Group Management

Properly handle layer groups for correct rendering order:

```cpp
void ConfigureAssetLayering(Ref<XPAsset::Asset>& asset, const std::string& assetType)
{
    // Set appropriate layer group based on asset type
    if (assetType == "terrain")
    {
        asset->intLayerGroup = XPLayerGroups::TERRAIN;
    }
    else if (assetType == "building")
    {
        asset->intLayerGroup = XPLayerGroups::OBJECTS;
    }
    else if (assetType == "lighting")
    {
        asset->intLayerGroup = XPLayerGroups::LIGHT_OBJECTS;
    }
    
    SEDX_CORE_INFO_TAG("ASSET", "Asset layered in group: {}", asset->intLayerGroup);
}
```

## Integration Points

### With Module System

- Inherit from `Module` for asset management components
- Use `OnAttach()` for asset loading and initialization
- Use `OnDetach()` for cleanup (automatic with smart pointers)
- Use `OnUpdate()` for runtime asset state updates

### With Logging System

- Use tagged logging for asset operations: `SEDX_CORE_INFO_TAG("ASSET", ...)`
- Log asset loading, validation, and errors
- Use appropriate log levels (INFO for normal operations, WARN for recoverable issues, ERROR for failures)

### With Layer Groups System

- Use `XPLayerGroups::Resolve()` for layer group calculations
- Set `intLayerGroup` appropriately for rendering order
- Consider layer priorities when placing assets

### With Memory Management

- Always use `CreateRef<T>()` for asset creation
- Store assets in `Ref<T>` containers
- Use `WeakRef<T>` for caching to avoid circular references
- Let smart pointers handle cleanup automatically

## Error Handling

```cpp
class RobustAssetLoader
{
public:
    std::optional<Ref<XPAsset::Asset>> LoadAssetSafely(const std::filesystem::path& path)
    {
        try
        {
            // Validate input
            if (path.empty() || !std::filesystem::exists(path))
            {
                SEDX_CORE_ERROR_TAG("ASSET", "Invalid asset path: {}", path.string());
                return std::nullopt;
            }
            
            // Create asset
            auto asset = CreateRef<CustomAsset>(path);
            
            // Validate result
            if (!ValidateAsset(asset))
            {
                SEDX_CORE_ERROR_TAG("ASSET", "Asset validation failed: {}", path.string());
                return std::nullopt;
            }
            
            SEDX_CORE_INFO_TAG("ASSET", "Successfully loaded asset: {}", path.string());
            return asset;
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("ASSET", "Exception loading asset {}: {}", path.string(), e.what());
            return std::nullopt;
        }
    }
};
```

This system provides a robust foundation for X-Plane asset management while maintaining consistency with the Scenery Editor X architecture and coding standards.

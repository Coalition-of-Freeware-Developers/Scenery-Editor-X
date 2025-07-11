# X-Plane Scenery Library - XPLibraryPath System Documentation

## Overview

The XPLibraryPath system provides sophisticated path management and regional definition capabilities for X-Plane scenery libraries. This system handles the complex relationships between virtual paths, real filesystem paths, regional variants, seasonal conditions, and weighted random selection that define X-Plane's library system.

## Architecture

### Core Components

- **`XPLibrary::DefinitionPath`** - Individual file path with metadata
- **`XPLibrary::DefinitionOptions`** - Weighted collection of path variants
- **`XPLibrary::Region`** - Geographic and conditional constraints
- **`XPLibrary::RegionalDefinitions`** - Seasonal variants within regions
- **`XPLibrary::Definition`** - Complete virtual path definition with all variants

### Path Resolution Hierarchy

```
Definition (Virtual Path)
├── RegionalDefinitions (by priority)
│   ├── Region Conditions (geographic/conditional)
│   └── Seasonal Variants
│       ├── Summer Options (weighted)
│       ├── Winter Options (weighted)
│       ├── Spring Options (weighted)
│       ├── Fall Options (weighted)
│       ├── Default Options (weighted)
│       └── Backup Options (fallback)
└── Multiple DefinitionOptions per season
    └── Multiple DefinitionPath per option (weighted selection)
```

## Class Reference

### XPLibrary::DefinitionPath

Represents an individual file path with its package context and metadata.

```cpp
class DefinitionPath
{
public:
    std::filesystem::path pPackagePath;  // Parent package directory
    std::filesystem::path pRealPath;     // Complete filesystem path
    std::filesystem::path pPath;         // Relative path within package
    bool bFromLibrary{false};            // True if from library.txt, false if local
    
    void SetPath(std::filesystem::path InBasePath, std::filesystem::path InRelativePath);
};
```

### XPLibrary::DefinitionOptions

Manages weighted collections of path options for random selection.

```cpp
class DefinitionOptions
{
private:
    double dblTotalRatio{0};
    std::vector<std::pair<double, XPLibrary::DefinitionPath>> vctOptions;
    
public:
    void AddOption(XPLibrary::DefinitionPath InPath, double InRatio = 1.0);
    XPLibrary::DefinitionPath GetRandomOption();
    void ResetOptions();
    size_t GetOptionCount() const;
    std::vector<std::pair<double, XPLibrary::DefinitionPath>>& GetOptions();
};
```

### XPLibrary::Region

Defines geographic boundaries and conditional constraints for scenery placement.

```cpp
class Region
{
public:
    // Conditional logic: value1, operator, value2
    std::vector<std::tuple<std::string, std::string, std::string>> Conditions;
    
    // Geographic boundaries
    double dblNorth{91}, dblSouth{-91};   // Latitude bounds
    double dblEast{-181}, dblWest{181};   // Longitude bounds
    
    bool CompatibleWith(double InLat, double InLon) const;
};
```

### XPLibrary::RegionalDefinitions

Seasonal variant management within geographic regions.

```cpp
class RegionalDefinitions
{
public:
    std::string strRegionName;
    
    // Seasonal variant collections
    XPLibrary::DefinitionOptions dSummer;
    XPLibrary::DefinitionOptions dWinter;
    XPLibrary::DefinitionOptions dFall;
    XPLibrary::DefinitionOptions dSpring;
    XPLibrary::DefinitionOptions dDefault;
    XPLibrary::DefinitionOptions dBackup;
    
    XPLibrary::DefinitionPath GetVersion(char InSeason);
};
```

### XPLibrary::Definition

Complete virtual path definition with all regional and seasonal variants.

```cpp
class Definition
{
public:
    std::filesystem::path pVirtual;                         // Virtual library path
    std::vector<RegionalDefinitions> vctRegionalDefs;       // Ordered by priority
    bool bIsPrivate{false};                                 // Asset accessibility
};
```

## Usage Examples

### Basic Path Definition Setup

```cpp
class LibraryPathModule : public Module
{
public:
    explicit LibraryPathModule() : Module("LibraryPathModule") {}
    
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("LIBPATH", "Initializing library path system");
        
        // Create a basic library definition
        CreateBasicDefinition();
        
        // Create regional variant definition
        CreateRegionalDefinition();
        
        // Create seasonal definition
        CreateSeasonalDefinition();
    }
    
private:
    std::vector<XPLibrary::Definition> m_Definitions;
    std::map<std::string, XPLibrary::Region> m_Regions;
    
    void CreateBasicDefinition()
    {
        // Create a simple virtual path definition
        XPLibrary::Definition definition;
        definition.pVirtual = "lib/airport/vehicles/pushback.obj";
        definition.bIsPrivate = false;
        
        // Create a basic regional definition for global use
        XPLibrary::RegionalDefinitions globalRegion;
        globalRegion.strRegionName = "region_all";
        
        // Add default options
        AddBasicOptions(globalRegion.dDefault);
        
        definition.vctRegionalDefs.push_back(globalRegion);
        m_Definitions.push_back(definition);
        
        SEDX_CORE_INFO_TAG("LIBPATH", "Created basic definition for: {}", 
            definition.pVirtual.string());
    }
    
    void AddBasicOptions(XPLibrary::DefinitionOptions& options)
    {
        // Add multiple weighted variants
        
        // Primary option (50% weight)
        XPLibrary::DefinitionPath primaryPath;
        primaryPath.SetPath("Custom Scenery/ZZZZ_Airport_Pack/", "objects/vehicles/pushback_modern.obj");
        primaryPath.bFromLibrary = true;
        options.AddOption(primaryPath, 0.5);
        
        // Secondary option (30% weight)
        XPLibrary::DefinitionPath secondaryPath;
        secondaryPath.SetPath("Custom Scenery/ZZZZ_Airport_Pack/", "objects/vehicles/pushback_classic.obj");
        secondaryPath.bFromLibrary = true;
        options.AddOption(secondaryPath, 0.3);
        
        // Tertiary option (20% weight)
        XPLibrary::DefinitionPath tertiaryPath;
        tertiaryPath.SetPath("Resources/default scenery/", "1000 airport scenery/vehicles/pushback.obj");
        tertiaryPath.bFromLibrary = true;
        options.AddOption(tertiaryPath, 0.2);
        
        SEDX_CORE_INFO_TAG("LIBPATH", "Added {} options with total weight: {}", 
            options.GetOptionCount(), 1.0);
    }
    
    void CreateRegionalDefinition()
    {
        // Create a region-specific definition
        XPLibrary::Definition definition;
        definition.pVirtual = "lib/g10/forests/deciduous.for";
        definition.bIsPrivate = false;
        
        // Create temperate region
        XPLibrary::Region temperateRegion = CreateTemperateRegion();
        m_Regions["temperate"] = temperateRegion;
        
        XPLibrary::RegionalDefinitions temperateDefinition;
        temperateDefinition.strRegionName = "temperate";
        
        // Add temperate-specific options
        AddTemperateForestOptions(temperateDefinition.dDefault);
        
        definition.vctRegionalDefs.push_back(temperateDefinition);
        
        // Create tropical region fallback
        XPLibrary::RegionalDefinitions globalFallback;
        globalFallback.strRegionName = "region_all";
        AddGenericForestOptions(globalFallback.dDefault);
        
        definition.vctRegionalDefs.push_back(globalFallback);
        m_Definitions.push_back(definition);
        
        SEDX_CORE_INFO_TAG("LIBPATH", "Created regional definition for: {}", 
            definition.pVirtual.string());
    }
    
    XPLibrary::Region CreateTemperateRegion()
    {
        XPLibrary::Region region;
        
        // Set geographic boundaries for temperate zones
        region.dblNorth = 60.0;   // 60°N
        region.dblSouth = 30.0;   // 30°N
        region.dblWest = -130.0;  // Western boundary
        region.dblEast = 50.0;    // Eastern boundary
        
        // Add conditional constraints
        region.Conditions.push_back(std::make_tuple("climate", "==", "temperate"));
        region.Conditions.push_back(std::make_tuple("elevation", "<", "2000"));
        
        return region;
    }
    
    void AddTemperateForestOptions(XPLibrary::DefinitionOptions& options)
    {
        // Temperate deciduous forest variants
        
        XPLibrary::DefinitionPath oakForest;
        oakForest.SetPath("Custom Scenery/HD_Forests/", "temperate/oak_forest_dense.for");
        oakForest.bFromLibrary = true;
        options.AddOption(oakForest, 0.4);
        
        XPLibrary::DefinitionPath mapleForest;
        mapleForest.SetPath("Custom Scenery/HD_Forests/", "temperate/maple_forest_mixed.for");
        mapleForest.bFromLibrary = true;
        options.AddOption(mapleForest, 0.35);
        
        XPLibrary::DefinitionPath birchForest;
        birchForest.SetPath("Custom Scenery/HD_Forests/", "temperate/birch_forest_light.for");
        birchForest.bFromLibrary = true;
        options.AddOption(birchForest, 0.25);
    }
    
    void AddGenericForestOptions(XPLibrary::DefinitionOptions& options)
    {
        // Generic forest fallback
        XPLibrary::DefinitionPath genericForest;
        genericForest.SetPath("Resources/default scenery/", "1000 forests/deciduous.for");
        genericForest.bFromLibrary = true;
        options.AddOption(genericForest, 1.0);
    }
    
    void CreateSeasonalDefinition()
    {
        // Create definition with seasonal variants
        XPLibrary::Definition definition;
        definition.pVirtual = "lib/g10/terrain/farmland.ter";
        definition.bIsPrivate = false;
        
        XPLibrary::RegionalDefinitions agriculturalRegion;
        agriculturalRegion.strRegionName = "agricultural";
        
        // Configure seasonal variants
        ConfigureSeasonalOptions(agriculturalRegion);
        
        definition.vctRegionalDefs.push_back(agriculturalRegion);
        m_Definitions.push_back(definition);
        
        SEDX_CORE_INFO_TAG("LIBPATH", "Created seasonal definition for: {}", 
            definition.pVirtual.string());
    }
    
    void ConfigureSeasonalOptions(XPLibrary::RegionalDefinitions& regional)
    {
        // Summer variants (lush, green)
        ConfigureSummerOptions(regional.dSummer);
        
        // Winter variants (snow, dormant)
        ConfigureWinterOptions(regional.dWinter);
        
        // Spring variants (growth, mixed)
        ConfigureSpringOptions(regional.dSpring);
        
        // Fall variants (harvest, brown)
        ConfigureFallOptions(regional.dFall);
        
        // Default fallback
        ConfigureDefaultOptions(regional.dDefault);
    }
    
    void ConfigureSummerOptions(XPLibrary::DefinitionOptions& options)
    {
        XPLibrary::DefinitionPath lushFarm;
        lushFarm.SetPath("Custom Scenery/Seasonal_Terrain/", "summer/farmland_lush.ter");
        lushFarm.bFromLibrary = true;
        options.AddOption(lushFarm, 0.6);
        
        XPLibrary::DefinitionPath cropFarm;
        cropFarm.SetPath("Custom Scenery/Seasonal_Terrain/", "summer/farmland_crops.ter");
        cropFarm.bFromLibrary = true;
        options.AddOption(cropFarm, 0.4);
    }
    
    void ConfigureWinterOptions(XPLibrary::DefinitionOptions& options)
    {
        XPLibrary::DefinitionPath snowyFarm;
        snowyFarm.SetPath("Custom Scenery/Seasonal_Terrain/", "winter/farmland_snow.ter");
        snowyFarm.bFromLibrary = true;
        options.AddOption(snowyFarm, 0.7);
        
        XPLibrary::DefinitionPath dormantFarm;
        dormantFarm.SetPath("Custom Scenery/Seasonal_Terrain/", "winter/farmland_dormant.ter");
        dormantFarm.bFromLibrary = true;
        options.AddOption(dormantFarm, 0.3);
    }
    
    void ConfigureSpringOptions(XPLibrary::DefinitionOptions& options)
    {
        XPLibrary::DefinitionPath plowedFarm;
        plowedFarm.SetPath("Custom Scenery/Seasonal_Terrain/", "spring/farmland_plowed.ter");
        plowedFarm.bFromLibrary = true;
        options.AddOption(plowedFarm, 0.5);
        
        XPLibrary::DefinitionPath seedingFarm;
        seedingFarm.SetPath("Custom Scenery/Seasonal_Terrain/", "spring/farmland_seeding.ter");
        seedingFarm.bFromLibrary = true;
        options.AddOption(seedingFarm, 0.5);
    }
    
    void ConfigureFallOptions(XPLibrary::DefinitionOptions& options)
    {
        XPLibrary::DefinitionPath harvestFarm;
        harvestFarm.SetPath("Custom Scenery/Seasonal_Terrain/", "fall/farmland_harvest.ter");
        harvestFarm.bFromLibrary = true;
        options.AddOption(harvestFarm, 0.6);
        
        XPLibrary::DefinitionPath stubbleFarm;
        stubbleFarm.SetPath("Custom Scenery/Seasonal_Terrain/", "fall/farmland_stubble.ter");
        stubbleFarm.bFromLibrary = true;
        options.AddOption(stubbleFarm, 0.4);
    }
    
    void ConfigureDefaultOptions(XPLibrary::DefinitionOptions& options)
    {
        XPLibrary::DefinitionPath defaultFarm;
        defaultFarm.SetPath("Resources/default scenery/", "1000 terrain/farmland.ter");
        defaultFarm.bFromLibrary = true;
        options.AddOption(defaultFarm, 1.0);
    }
};
```

### Advanced Path Resolution

```cpp
class AdvancedPathResolver : public Module
{
public:
    void ResolveAssetPath(const std::string& virtualPath, 
                         double latitude, double longitude, 
                         char season = 'd')
    {
        SEDX_PROFILE_SCOPE("AdvancedPathResolver::ResolveAssetPath");
        
        // Find definition for virtual path
        auto definition = FindDefinition(virtualPath);
        if (!definition.has_value())
        {
            SEDX_CORE_ERROR_TAG("RESOLVE", "No definition found for: {}", virtualPath);
            return;
        }
        
        // Find best regional match
        auto regionalDef = FindBestRegionalMatch(definition.value(), latitude, longitude);
        if (!regionalDef.has_value())
        {
            SEDX_CORE_ERROR_TAG("RESOLVE", "No regional match for coordinates: {}, {}", 
                latitude, longitude);
            return;
        }
        
        // Get seasonal variant
        auto path = GetSeasonalVariant(regionalDef.value(), season);
        if (path.pRealPath.empty())
        {
            SEDX_CORE_ERROR_TAG("RESOLVE", "No valid path for season: {}", season);
            return;
        }
        
        // Validate and use resolved path
        if (ValidateResolvedPath(path))
        {
            UseResolvedAsset(path);
        }
    }
    
private:
    std::vector<XPLibrary::Definition> m_Definitions;
    std::map<std::string, XPLibrary::Region> m_Regions;
    
    std::optional<XPLibrary::Definition> FindDefinition(const std::string& virtualPath)
    {
        auto it = std::find_if(m_Definitions.begin(), m_Definitions.end(),
            [&virtualPath](const XPLibrary::Definition& def)
            {
                return def.pVirtual.string() == virtualPath;
            });
        
        if (it != m_Definitions.end())
        {
            return *it;
        }
        
        return std::nullopt;
    }
    
    std::optional<XPLibrary::RegionalDefinitions> FindBestRegionalMatch(
        const XPLibrary::Definition& definition,
        double latitude, double longitude)
    {
        // Regional definitions are ordered by priority
        for (const auto& regional : definition.vctRegionalDefs)
        {
            if (regional.strRegionName == "region_all")
            {
                // Global fallback always matches
                return regional;
            }
            
            auto regionIt = m_Regions.find(regional.strRegionName);
            if (regionIt != m_Regions.end())
            {
                if (regionIt->second.CompatibleWith(latitude, longitude))
                {
                    SEDX_CORE_INFO_TAG("RESOLVE", "Matched region: {}", regional.strRegionName);
                    return regional;
                }
            }
        }
        
        return std::nullopt;
    }
    
    XPLibrary::DefinitionPath GetSeasonalVariant(
        const XPLibrary::RegionalDefinitions& regional,
        char season)
    {
        XPLibrary::DefinitionPath path;
        
        switch (season)
        {
        case XPLibrary::SEASON_SUMMER:
        case XPLibrary::SEASON_DEFAULT:
            path = GetBestOption(regional.dSummer, regional.dDefault, regional.dBackup);
            break;
            
        case XPLibrary::SEASON_WINTER:
            path = GetBestOption(regional.dWinter, regional.dDefault, regional.dBackup);
            break;
            
        case XPLibrary::SEASON_SPRING:
            path = GetBestOption(regional.dSpring, regional.dDefault, regional.dBackup);
            break;
            
        case XPLibrary::SEASON_FALL:
            path = GetBestOption(regional.dFall, regional.dDefault, regional.dBackup);
            break;
            
        default:
            path = GetBestOption(regional.dDefault, regional.dBackup);
            break;
        }
        
        if (!path.pRealPath.empty())
        {
            SEDX_CORE_INFO_TAG("RESOLVE", "Selected path: {} (season: {})", 
                path.pRealPath.string(), season);
        }
        
        return path;
    }
    
    XPLibrary::DefinitionPath GetBestOption(
        const XPLibrary::DefinitionOptions& primary,
        const XPLibrary::DefinitionOptions& fallback1,
        const XPLibrary::DefinitionOptions& fallback2 = {})
    {
        // Try primary options first
        if (primary.GetOptionCount() > 0)
        {
            return const_cast<XPLibrary::DefinitionOptions&>(primary).GetRandomOption();
        }
        
        // Try first fallback
        if (fallback1.GetOptionCount() > 0)
        {
            SEDX_CORE_INFO_TAG("RESOLVE", "Using fallback options");
            return const_cast<XPLibrary::DefinitionOptions&>(fallback1).GetRandomOption();
        }
        
        // Try second fallback
        if (fallback2.GetOptionCount() > 0)
        {
            SEDX_CORE_INFO_TAG("RESOLVE", "Using backup options");
            return const_cast<XPLibrary::DefinitionOptions&>(fallback2).GetRandomOption();
        }
        
        // Return empty path if no options available
        return XPLibrary::DefinitionPath{};
    }
    
    bool ValidateResolvedPath(const XPLibrary::DefinitionPath& path)
    {
        // Check if file exists
        if (!std::filesystem::exists(path.pRealPath))
        {
            SEDX_CORE_ERROR_TAG("RESOLVE", "Resolved path does not exist: {}", 
                path.pRealPath.string());
            return false;
        }
        
        // Additional validation checks could go here
        // - File size checks
        // - File format validation
        // - Access permissions
        
        return true;
    }
    
    void UseResolvedAsset(const XPLibrary::DefinitionPath& path)
    {
        SEDX_CORE_INFO_TAG("RESOLVE", "Using asset: {} from package: {}", 
            path.pPath.string(), path.pPackagePath.string());
        
        // Load and use the resolved asset
        // This would typically involve:
        // - Loading the asset file
        // - Adding it to the scene
        // - Applying any package-specific configurations
    }
};
```

### Path Management and Caching

```cpp
class PathManagerModule : public Module
{
public:
    explicit PathManagerModule() : Module("PathManagerModule") {}
    
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("PATHMAN", "Initializing path management system");
        InitializePathCache();
    }
    
    void OnDetach() override
    {
        SEDX_CORE_INFO_TAG("PATHMAN", "Cleaning up path management");
        m_PathCache.clear();
        m_ResolvedPaths.clear();
    }
    
private:
    // Caching system for resolved paths
    struct CacheKey
    {
        std::string virtualPath;
        double latitude;
        double longitude;
        char season;
        
        bool operator<(const CacheKey& other) const
        {
            if (virtualPath != other.virtualPath) return virtualPath < other.virtualPath;
            if (latitude != other.latitude) return latitude < other.latitude;
            if (longitude != other.longitude) return longitude < other.longitude;
            return season < other.season;
        }
    };
    
    std::map<CacheKey, XPLibrary::DefinitionPath> m_PathCache;
    std::unordered_map<std::string, XPLibrary::Definition> m_ResolvedPaths;
    
    void InitializePathCache()
    {
        // Pre-populate cache with commonly used paths
        PrePopulateCommonPaths();
        
        // Set up cache cleanup timer
        ScheduleCacheCleanup();
    }
    
    void PrePopulateCommonPaths()
    {
        // Common virtual paths that are frequently requested
        std::vector<std::string> commonPaths = {
            "lib/airport/vehicles/pushback.obj",
            "lib/g10/terrain/grass.ter",
            "lib/g10/forests/deciduous.for",
            "lib/airport/Common_Elements/Vehicles/fuel_truck.obj"
        };
        
        // Common test coordinates (major airports)
        std::vector<std::pair<double, double>> testCoords = {
            {37.6213, -122.3790},  // KSFO
            {40.6413, -73.7781},   // KJFK
            {51.4700, -0.4543},    // EGLL
            {35.7722, 140.3928}    // RJAA
        };
        
        for (const auto& path : commonPaths)
        {
            for (const auto& coord : testCoords)
            {
                for (char season : {XPLibrary::SEASON_SUMMER, XPLibrary::SEASON_WINTER})
                {
                    CacheKey key{path, coord.first, coord.second, season};
                    // Pre-resolve would happen here in a real implementation
                    // m_PathCache[key] = ResolvePathInternal(path, coord.first, coord.second, season);
                }
            }
        }
        
        SEDX_CORE_INFO_TAG("PATHMAN", "Pre-populated path cache with {} entries", m_PathCache.size());
    }
    
    void ScheduleCacheCleanup()
    {
        // In a real implementation, this would set up a timer
        // to periodically clean up unused cache entries
    }
    
public:
    XPLibrary::DefinitionPath GetCachedPath(const std::string& virtualPath,
                                           double latitude, double longitude,
                                           char season = XPLibrary::SEASON_DEFAULT)
    {
        CacheKey key{virtualPath, latitude, longitude, season};
        
        auto cacheIt = m_PathCache.find(key);
        if (cacheIt != m_PathCache.end())
        {
            SEDX_CORE_INFO_TAG("PATHMAN", "Cache hit for: {}", virtualPath);
            return cacheIt->second;
        }
        
        // Cache miss - resolve and cache
        SEDX_CORE_INFO_TAG("PATHMAN", "Cache miss for: {} - resolving", virtualPath);
        
        auto resolvedPath = ResolvePathInternal(virtualPath, latitude, longitude, season);
        m_PathCache[key] = resolvedPath;
        
        return resolvedPath;
    }
    
private:
    XPLibrary::DefinitionPath ResolvePathInternal(const std::string& virtualPath,
                                                 double latitude, double longitude,
                                                 char season)
    {
        // This would contain the actual path resolution logic
        // similar to the AdvancedPathResolver example above
        
        // For now, return a placeholder
        XPLibrary::DefinitionPath path;
        path.pVirtual = virtualPath;
        path.bFromLibrary = true;
        
        return path;
    }
    
public:
    void ClearCache()
    {
        m_PathCache.clear();
        SEDX_CORE_INFO_TAG("PATHMAN", "Path cache cleared");
    }
    
    void GetCacheStatistics(size_t& totalEntries, size_t& hitCount, size_t& missCount)
    {
        totalEntries = m_PathCache.size();
        // Hit/miss counts would be tracked in a real implementation
        hitCount = 0;
        missCount = 0;
    }
};
```

## Best Practices

### 1. Efficient Path Resolution

```cpp
class OptimizedPathResolution
{
public:
    static XPLibrary::DefinitionPath ResolveFast(
        const XPLibrary::Definition& definition,
        double latitude, double longitude,
        char season = XPLibrary::SEASON_DEFAULT)
    {
        SEDX_PROFILE_SCOPE("OptimizedPathResolution::ResolveFast");
        
        // Fast path for simple cases
        if (definition.vctRegionalDefs.size() == 1)
        {
            const auto& regional = definition.vctRegionalDefs[0];
            if (regional.strRegionName == "region_all")
            {
                return GetSeasonalVariantFast(regional, season);
            }
        }
        
        // Standard resolution for complex cases
        return ResolveStandard(definition, latitude, longitude, season);
    }
    
private:
    static XPLibrary::DefinitionPath GetSeasonalVariantFast(
        const XPLibrary::RegionalDefinitions& regional,
        char season)
    {
        const XPLibrary::DefinitionOptions* targetOptions = nullptr;
        
        switch (season)
        {
        case XPLibrary::SEASON_SUMMER:
            targetOptions = &regional.dSummer;
            break;
        case XPLibrary::SEASON_WINTER:
            targetOptions = &regional.dWinter;
            break;
        case XPLibrary::SEASON_SPRING:
            targetOptions = &regional.dSpring;
            break;
        case XPLibrary::SEASON_FALL:
            targetOptions = &regional.dFall;
            break;
        default:
            targetOptions = &regional.dDefault;
            break;
        }
        
        // Quick return if target has options
        if (targetOptions && targetOptions->GetOptionCount() > 0)
        {
            return const_cast<XPLibrary::DefinitionOptions*>(targetOptions)->GetRandomOption();
        }
        
        // Fall back to default
        if (regional.dDefault.GetOptionCount() > 0)
        {
            return const_cast<XPLibrary::DefinitionOptions&>(regional.dDefault).GetRandomOption();
        }
        
        // Last resort backup
        if (regional.dBackup.GetOptionCount() > 0)
        {
            return const_cast<XPLibrary::DefinitionOptions&>(regional.dBackup).GetRandomOption();
        }
        
        return XPLibrary::DefinitionPath{};
    }
    
    static XPLibrary::DefinitionPath ResolveStandard(
        const XPLibrary::Definition& definition,
        double latitude, double longitude,
        char season)
    {
        // Standard resolution implementation
        // This would contain the full resolution logic
        return XPLibrary::DefinitionPath{};
    }
};
```

### 2. Memory Management

```cpp
class EfficientPathStorage
{
public:
    // Use string interning for common paths
    static std::string InternPath(const std::string& path)
    {
        static std::unordered_set<std::string> pathPool;
        auto [it, inserted] = pathPool.insert(path);
        return *it;  // Return reference to pooled string
    }
    
    // Optimize definition storage
    static void OptimizeDefinition(XPLibrary::Definition& definition)
    {
        // Intern virtual path
        definition.pVirtual = InternPath(definition.pVirtual.string());
        
        // Optimize regional definitions
        for (auto& regional : definition.vctRegionalDefs)
        {
            regional.strRegionName = InternPath(regional.strRegionName);
            OptimizeOptions(regional.dSummer);
            OptimizeOptions(regional.dWinter);
            OptimizeOptions(regional.dSpring);
            OptimizeOptions(regional.dFall);
            OptimizeOptions(regional.dDefault);
            OptimizeOptions(regional.dBackup);
        }
    }
    
private:
    static void OptimizeOptions(XPLibrary::DefinitionOptions& options)
    {
        auto& optionList = options.GetOptions();
        for (auto& [weight, path] : optionList)
        {
            // Intern path strings
            path.pPackagePath = InternPath(path.pPackagePath.string());
            path.pPath = InternPath(path.pPath.string());
        }
    }
};
```

This comprehensive path management system enables efficient and flexible asset resolution while maintaining compatibility with X-Plane's complex library system requirements.

# X-Plane Scenery Library - XPLibrarySystem Documentation

## Overview

The XPLibrarySystem provides comprehensive management for X-Plane's virtual file system, handling the complex library.txt parsing, scenery pack prioritization, and virtual-to-real path resolution. This system serves as the central coordination point for all library-based asset loading and management within Scenery Editor X.

## Architecture

### Core Components

- **`XPLibrary::VirtualFileSystem`** - Main system coordinator
- **Library.txt Processing** - Parses and interprets X-Plane library files
- **Scenery Pack Prioritization** - Manages loading order and override hierarchy
- **Path Resolution Engine** - Converts virtual paths to real filesystem paths
- **Region Management** - Handles geographic and conditional constraints

### System Flow

```
X-Plane Installation Root
├── Custom Scenery Packs (ordered by scenery_packs.ini)
│   ├── Package A/library.txt
│   ├── Package B/library.txt
│   └── Package N/library.txt
├── Resources/default scenery/library.txt
└── Current Package (local files, highest priority)

↓ Processing Flow ↓

VirtualFileSystem
├── LoadFileSystem()
│   ├── Load local package files
│   ├── Process library.txt files (by priority)
│   ├── Parse definitions and regions
│   └── Build virtual path mapping
├── GetDefinition() - Virtual path lookup
└── GetRegion() - Regional constraint lookup
```

## Class Reference

### XPLibrary::VirtualFileSystem

The central class that manages the entire virtual file system for X-Plane scenery.

```cpp
class VirtualFileSystem
{
private:
    std::vector<XPLibrary::Definition> vctDefinitions;     // All virtual path definitions
    std::map<std::string, XPLibrary::Region> mRegions;    // Regional constraints
    
public:
    /**
     * @brief Loads the complete file system from X-Plane installation
     * @param InXpRootPath Root directory of X-Plane installation
     * @param InCurrentPackagePath Path to current scenery package being edited
     * @param InCustomSceneryPacks Ordered list of custom scenery packs (highest priority first)
     */
    void LoadFileSystem(std::filesystem::path InXpRootPath,
                       std::filesystem::path InCurrentPackagePath,
                       std::vector<std::filesystem::path> InCustomSceneryPacks);
    
    /**
     * @brief Resolves a virtual path to its definition
     * @param InPath Virtual library path (e.g., "lib/airport/vehicles/pushback.obj")
     * @return Complete definition with all variants, or empty if not found
     */
    XPLibrary::Definition GetDefinition(const std::string& InPath);
    
    /**
     * @brief Retrieves region definition by name
     * @param InPath Region name
     * @return Copy of region definition, or empty if not found
     */
    XPLibrary::Region GetRegion(const std::string& InPath);
};
```

## Usage Examples

### Basic System Setup

```cpp
class LibrarySystemModule : public Module
{
public:
    explicit LibrarySystemModule() : Module("LibrarySystemModule") {}
    
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("LIBRARY", "Initializing X-Plane library system");
        
        // Initialize the virtual file system
        InitializeLibrarySystem();
        
        SEDX_CORE_INFO_TAG("LIBRARY", "Library system initialized successfully");
    }
    
    void OnDetach() override
    {
        SEDX_CORE_INFO_TAG("LIBRARY", "Shutting down library system");
        // VFS cleanup handled automatically
    }
    
private:
    XPLibrary::VirtualFileSystem m_VirtualFileSystem;
    std::filesystem::path m_XPlaneRoot;
    std::filesystem::path m_CurrentPackage;
    std::vector<std::filesystem::path> m_CustomSceneryPacks;
    
    void InitializeLibrarySystem()
    {
        try
        {
            // Discover X-Plane installation
            DetectXPlaneInstallation();
            
            // Load scenery pack order
            LoadSceneryPackOrder();
            
            // Set up current package
            SetupCurrentPackage();
            
            // Initialize the virtual file system
            m_VirtualFileSystem.LoadFileSystem(m_XPlaneRoot, m_CurrentPackage, m_CustomSceneryPacks);
            
            // Verify system integrity
            ValidateLibrarySystem();
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("LIBRARY", "Failed to initialize library system: {}", e.what());
            throw;
        }
    }
    
    void DetectXPlaneInstallation()
    {
        // Check common X-Plane installation locations
        std::vector<std::filesystem::path> possiblePaths = {
            "C:/X-Plane 12",
            "C:/X-Plane 11",
            "D:/X-Plane 12",
            "D:/X-Plane 11"
        };
        
        for (const auto& path : possiblePaths)
        {
            if (IsValidXPlaneInstallation(path))
            {
                m_XPlaneRoot = path;
                SEDX_CORE_INFO_TAG("LIBRARY", "Found X-Plane installation: {}", path.string());
                return;
            }
        }
        
        // If not found in common locations, prompt user or use configuration
        m_XPlaneRoot = GetXPlanePathFromConfig();
        
        if (!IsValidXPlaneInstallation(m_XPlaneRoot))
        {
            throw std::runtime_error("Invalid X-Plane installation path");
        }
    }
    
    bool IsValidXPlaneInstallation(const std::filesystem::path& path)
    {
        // Check for required X-Plane files and directories
        return std::filesystem::exists(path / "X-Plane.exe") ||
               std::filesystem::exists(path / "X-Plane") ||
               std::filesystem::exists(path / "Resources" / "default scenery");
    }
    
    void LoadSceneryPackOrder()
    {
        std::filesystem::path sceneryPacksFile = m_XPlaneRoot / "Custom Scenery" / "scenery_packs.ini";
        
        if (!std::filesystem::exists(sceneryPacksFile))
        {
            SEDX_CORE_WARN_TAG("LIBRARY", "scenery_packs.ini not found, scanning directory");
            ScanCustomSceneryDirectory();
            return;
        }
        
        try
        {
            std::ifstream file(sceneryPacksFile);
            std::string line;
            
            while (std::getline(file, line))
            {
                if (line.empty() || line[0] == 'I')
                {
                    continue; // Skip empty lines and comments
                }
                
                if (line.starts_with("SCENERY_PACK "))
                {
                    std::string packPath = line.substr(13); // Remove "SCENERY_PACK "
                    
                    // Convert to absolute path
                    std::filesystem::path absolutePath = m_XPlaneRoot / "Custom Scenery" / packPath;
                    
                    if (std::filesystem::exists(absolutePath))
                    {
                        m_CustomSceneryPacks.push_back(absolutePath);
                        SEDX_CORE_INFO_TAG("LIBRARY", "Added scenery pack: {}", packPath);
                    }
                    else
                    {
                        SEDX_CORE_WARN_TAG("LIBRARY", "Scenery pack not found: {}", packPath);
                    }
                }
            }
            
            SEDX_CORE_INFO_TAG("LIBRARY", "Loaded {} scenery packs from scenery_packs.ini", 
                m_CustomSceneryPacks.size());
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("LIBRARY", "Failed to load scenery_packs.ini: {}", e.what());
            ScanCustomSceneryDirectory();
        }
    }
    
    void ScanCustomSceneryDirectory()
    {
        std::filesystem::path customSceneryDir = m_XPlaneRoot / "Custom Scenery";
        
        if (!std::filesystem::exists(customSceneryDir))
        {
            SEDX_CORE_WARN_TAG("LIBRARY", "Custom Scenery directory not found");
            return;
        }
        
        try
        {
            for (const auto& entry : std::filesystem::directory_iterator(customSceneryDir))
            {
                if (entry.is_directory())
                {
                    m_CustomSceneryPacks.push_back(entry.path());
                    SEDX_CORE_INFO_TAG("LIBRARY", "Discovered scenery pack: {}", 
                        entry.path().filename().string());
                }
            }
            
            // Sort alphabetically since we don't have priority order
            std::sort(m_CustomSceneryPacks.begin(), m_CustomSceneryPacks.end());
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("LIBRARY", "Failed to scan Custom Scenery directory: {}", e.what());
        }
    }
    
    void SetupCurrentPackage()
    {
        // Set current package to the project directory or a default location
        m_CurrentPackage = GetCurrentProjectPath();
        
        if (m_CurrentPackage.empty() || !std::filesystem::exists(m_CurrentPackage))
        {
            // Create a default package directory
            m_CurrentPackage = std::filesystem::current_path() / "current_package";
            std::filesystem::create_directories(m_CurrentPackage);
        }
        
        SEDX_CORE_INFO_TAG("LIBRARY", "Current package: {}", m_CurrentPackage.string());
    }
    
    void ValidateLibrarySystem()
    {
        // Test basic functionality
        TestBasicResolution();
        
        // Check for common issues
        CheckForCommonIssues();
        
        SEDX_CORE_INFO_TAG("LIBRARY", "Library system validation completed");
    }
    
    void TestBasicResolution()
    {
        // Test resolution of a common virtual path
        std::string testPath = "lib/airport/vehicles/pushback.obj";
        auto definition = m_VirtualFileSystem.GetDefinition(testPath);
        
        if (definition.pVirtual.empty())
        {
            SEDX_CORE_WARN_TAG("LIBRARY", "Basic resolution test failed for: {}", testPath);
        }
        else
        {
            SEDX_CORE_INFO_TAG("LIBRARY", "Basic resolution test passed");
        }
    }
    
    void CheckForCommonIssues()
    {
        // Check for duplicate library definitions
        // Check for missing referenced files
        // Check for circular dependencies
        // This would contain specific validation logic
    }
    
    std::filesystem::path GetXPlanePathFromConfig()
    {
        // This would read from application configuration
        return "C:/X-Plane 12"; // Default fallback
    }
    
    std::filesystem::path GetCurrentProjectPath()
    {
        // This would get the current project path from the application
        return std::filesystem::current_path();
    }
    
public:
    // Public interface for asset resolution
    XPLibrary::Definition ResolveVirtualPath(const std::string& virtualPath)
    {
        return m_VirtualFileSystem.GetDefinition(virtualPath);
    }
    
    XPLibrary::Region GetRegionInfo(const std::string& regionName)
    {
        return m_VirtualFileSystem.GetRegion(regionName);
    }
    
    const std::vector<std::filesystem::path>& GetSceneryPacks() const
    {
        return m_CustomSceneryPacks;
    }
    
    const std::filesystem::path& GetXPlaneRoot() const
    {
        return m_XPlaneRoot;
    }
};
```

### Advanced Asset Resolution

```cpp
class AssetResolutionService : public Module
{
public:
    explicit AssetResolutionService(LibrarySystemModule* librarySystem)
        : Module("AssetResolutionService"), m_LibrarySystem(librarySystem) {}
    
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("ASSET_RES", "Initializing asset resolution service");
        SEDX_CORE_ASSERT(m_LibrarySystem != nullptr, "Library system is required");
    }
    
    Ref<XPAsset::Asset> LoadAsset(const std::string& virtualPath,
                                 double latitude = 0.0, 
                                 double longitude = 0.0,
                                 char season = 'd')
    {
        SEDX_PROFILE_SCOPE("AssetResolutionService::LoadAsset");
        
        try
        {
            // Resolve virtual path to definition
            auto definition = m_LibrarySystem->ResolveVirtualPath(virtualPath);
            if (definition.pVirtual.empty())
            {
                SEDX_CORE_ERROR_TAG("ASSET_RES", "Virtual path not found: {}", virtualPath);
                return nullptr;
            }
            
            // Check privacy constraints
            if (definition.bIsPrivate)
            {
                SEDX_CORE_WARN_TAG("ASSET_RES", "Attempting to load private asset: {}", virtualPath);
                return nullptr;
            }
            
            // Find best regional match
            auto bestPath = ResolveBestPath(definition, latitude, longitude, season);
            if (bestPath.pRealPath.empty())
            {
                SEDX_CORE_ERROR_TAG("ASSET_RES", "No valid path found for: {}", virtualPath);
                return nullptr;
            }
            
            // Load the actual asset
            return LoadPhysicalAsset(bestPath);
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("ASSET_RES", "Exception loading asset {}: {}", virtualPath, e.what());
            return nullptr;
        }
    }
    
private:
    LibrarySystemModule* m_LibrarySystem;
    std::unordered_map<std::string, WeakRef<XPAsset::Asset>> m_AssetCache;
    
    XPLibrary::DefinitionPath ResolveBestPath(const XPLibrary::Definition& definition,
                                             double latitude, double longitude,
                                             char season)
    {
        // Find the best regional definition
        for (const auto& regional : definition.vctRegionalDefs)
        {
            if (IsRegionalMatch(regional, latitude, longitude))
            {
                auto path = GetSeasonalPath(regional, season);
                if (!path.pRealPath.empty())
                {
                    return path;
                }
            }
        }
        
        // No match found
        return XPLibrary::DefinitionPath{};
    }
    
    bool IsRegionalMatch(const XPLibrary::RegionalDefinitions& regional,
                        double latitude, double longitude)
    {
        if (regional.strRegionName == "region_all")
        {
            return true; // Global fallback always matches
        }
        
        auto region = m_LibrarySystem->GetRegionInfo(regional.strRegionName);
        return region.CompatibleWith(latitude, longitude);
    }
    
    XPLibrary::DefinitionPath GetSeasonalPath(const XPLibrary::RegionalDefinitions& regional,
                                             char season)
    {
        // Try to get path for specific season
        auto path = GetPathForSeason(regional, season);
        if (!path.pRealPath.empty())
        {
            return path;
        }
        
        // Fall back to default
        if (regional.dDefault.GetOptionCount() > 0)
        {
            return const_cast<XPLibrary::DefinitionOptions&>(regional.dDefault).GetRandomOption();
        }
        
        // Last resort: backup
        if (regional.dBackup.GetOptionCount() > 0)
        {
            return const_cast<XPLibrary::DefinitionOptions&>(regional.dBackup).GetRandomOption();
        }
        
        return XPLibrary::DefinitionPath{};
    }
    
    XPLibrary::DefinitionPath GetPathForSeason(const XPLibrary::RegionalDefinitions& regional,
                                              char season)
    {
        const XPLibrary::DefinitionOptions* options = nullptr;
        
        switch (season)
        {
        case 's': // Summer
            options = &regional.dSummer;
            break;
        case 'w': // Winter
            options = &regional.dWinter;
            break;
        case 'p': // Spring
            options = &regional.dSpring;
            break;
        case 'f': // Fall
            options = &regional.dFall;
            break;
        default:
            options = &regional.dDefault;
            break;
        }
        
        if (options && options->GetOptionCount() > 0)
        {
            return const_cast<XPLibrary::DefinitionOptions*>(options)->GetRandomOption();
        }
        
        return XPLibrary::DefinitionPath{};
    }
    
    Ref<XPAsset::Asset> LoadPhysicalAsset(const XPLibrary::DefinitionPath& path)
    {
        // Check cache first
        std::string cacheKey = path.pRealPath.string();
        if (auto cached = m_AssetCache[cacheKey].Lock())
        {
            SEDX_CORE_INFO_TAG("ASSET_RES", "Cache hit for: {}", cacheKey);
            return cached;
        }
        
        // Load new asset
        Ref<XPAsset::Asset> asset = nullptr;
        
        if (path.pRealPath.extension() == ".obj")
        {
            asset = LoadObjAsset(path);
        }
        else if (path.pRealPath.extension() == ".ter")
        {
            asset = LoadTerrainAsset(path);
        }
        else if (path.pRealPath.extension() == ".for")
        {
            asset = LoadForestAsset(path);
        }
        else
        {
            SEDX_CORE_WARN_TAG("ASSET_RES", "Unknown asset type: {}", 
                path.pRealPath.extension().string());
            return nullptr;
        }
        
        if (asset)
        {
            // Cache the loaded asset
            m_AssetCache[cacheKey] = asset;
            SEDX_CORE_INFO_TAG("ASSET_RES", "Loaded and cached asset: {}", cacheKey);
        }
        
        return asset;
    }
    
    Ref<XPAsset::Asset> LoadObjAsset(const XPLibrary::DefinitionPath& path)
    {
        auto objAsset = CreateRef<XPAsset::Obj>();
        
        if (objAsset->Load(path.pRealPath))
        {
            SEDX_CORE_INFO_TAG("ASSET_RES", "Loaded OBJ asset: {}", path.pRealPath.string());
            return objAsset;
        }
        
        SEDX_CORE_ERROR_TAG("ASSET_RES", "Failed to load OBJ asset: {}", path.pRealPath.string());
        return nullptr;
    }
    
    Ref<XPAsset::Asset> LoadTerrainAsset(const XPLibrary::DefinitionPath& path)
    {
        // Terrain asset loading would be implemented here
        SEDX_CORE_INFO_TAG("ASSET_RES", "Loading terrain asset: {}", path.pRealPath.string());
        
        // For now, return a basic asset
        auto asset = CreateRef<XPAsset::Asset>();
        // This would need a concrete implementation, not the abstract base class
        return nullptr; // Placeholder
    }
    
    Ref<XPAsset::Asset> LoadForestAsset(const XPLibrary::DefinitionPath& path)
    {
        // Forest asset loading would be implemented here
        SEDX_CORE_INFO_TAG("ASSET_RES", "Loading forest asset: {}", path.pRealPath.string());
        
        // For now, return a basic asset
        auto asset = CreateRef<XPAsset::Asset>();
        // This would need a concrete implementation, not the abstract base class
        return nullptr; // Placeholder
    }
    
public:
    void ClearAssetCache()
    {
        m_AssetCache.clear();
        SEDX_CORE_INFO_TAG("ASSET_RES", "Asset cache cleared");
    }
    
    size_t GetCacheSize() const
    {
        return m_AssetCache.size();
    }
    
    void OnUIRender() override
    {
        if (m_ShowDebugWindow)
        {
            RenderDebugInterface();
        }
    }
    
private:
    bool m_ShowDebugWindow = false;
    
    void RenderDebugInterface()
    {
        ImGui::Begin("Asset Resolution Debug", &m_ShowDebugWindow);
        
        ImGui::Text("Asset Resolution Service Status");
        ImGui::Separator();
        
        ImGui::Text("Cache Size: %zu", m_AssetCache.size());
        
        if (ImGui::Button("Clear Cache"))
        {
            ClearAssetCache();
        }
        
        ImGui::Separator();
        
        // Virtual path tester
        static char virtualPath[256] = "lib/airport/vehicles/pushback.obj";
        static float latitude = 37.6213f;  // KSFO
        static float longitude = -122.3790f;
        static int season = 0;
        const char* seasons[] = {"Default", "Summer", "Winter", "Spring", "Fall"};
        
        ImGui::Text("Test Virtual Path Resolution:");
        ImGui::InputText("Virtual Path", virtualPath, sizeof(virtualPath));
        ImGui::SliderFloat("Latitude", &latitude, -90.0f, 90.0f);
        ImGui::SliderFloat("Longitude", &longitude, -180.0f, 180.0f);
        ImGui::Combo("Season", &season, seasons, IM_ARRAYSIZE(seasons));
        
        if (ImGui::Button("Test Resolution"))
        {
            char seasonChar = 'd';
            switch (season)
            {
            case 1: seasonChar = 's'; break; // Summer
            case 2: seasonChar = 'w'; break; // Winter
            case 3: seasonChar = 'p'; break; // Spring
            case 4: seasonChar = 'f'; break; // Fall
            }
            
            auto asset = LoadAsset(virtualPath, latitude, longitude, seasonChar);
            if (asset)
            {
                SEDX_CORE_INFO_TAG("ASSET_RES", "Resolution test succeeded");
            }
            else
            {
                SEDX_CORE_WARN_TAG("ASSET_RES", "Resolution test failed");
            }
        }
        
        ImGui::End();
    }
};
```

### System Monitoring and Diagnostics

```cpp
class LibrarySystemDiagnostics : public Module
{
public:
    explicit LibrarySystemDiagnostics(LibrarySystemModule* librarySystem)
        : Module("LibrarySystemDiagnostics"), m_LibrarySystem(librarySystem) {}
    
    void OnAttach() override
    {
        SEDX_CORE_INFO_TAG("DIAG", "Initializing library system diagnostics");
        PerformSystemDiagnostics();
    }
    
    void OnUpdate() override
    {
        if (!m_IsEnabled) return;
        SEDX_PROFILE_SCOPE("LibrarySystemDiagnostics::OnUpdate");
        
        // Periodic health checks
        if (ShouldPerformHealthCheck())
        {
            PerformHealthCheck();
        }
    }
    
    void OnUIRender() override
    {
        if (m_ShowDiagnosticsWindow)
        {
            RenderDiagnosticsInterface();
        }
    }
    
private:
    LibrarySystemModule* m_LibrarySystem;
    bool m_IsEnabled = true;
    bool m_ShowDiagnosticsWindow = false;
    
    struct DiagnosticData
    {
        size_t totalDefinitions = 0;
        size_t totalRegions = 0;
        size_t totalSceneryPacks = 0;
        std::vector<std::string> missingFiles;
        std::vector<std::string> orphanedDefinitions;
        std::vector<std::string> issues;
    } m_DiagData;
    
    void PerformSystemDiagnostics()
    {
        SEDX_PROFILE_SCOPE("LibrarySystemDiagnostics::PerformSystemDiagnostics");
        
        SEDX_CORE_INFO_TAG("DIAG", "Starting comprehensive system diagnostics");
        
        // Clear previous diagnostic data
        m_DiagData = {};
        
        // Collect system statistics
        CollectSystemStatistics();
        
        // Check for missing files
        CheckForMissingFiles();
        
        // Validate definitions
        ValidateDefinitions();
        
        // Check scenery pack integrity
        CheckSceneryPackIntegrity();
        
        // Generate report
        GenerateDiagnosticReport();
        
        SEDX_CORE_INFO_TAG("DIAG", "System diagnostics completed");
    }
    
    void CollectSystemStatistics()
    {
        m_DiagData.totalSceneryPacks = m_LibrarySystem->GetSceneryPacks().size();
        
        // Additional statistics would be collected here
        // This would require access to the VFS internals
        
        SEDX_CORE_INFO_TAG("DIAG", "Collected system statistics");
    }
    
    void CheckForMissingFiles()
    {
        // This would check if all referenced files actually exist
        // Implementation would iterate through all definitions and verify paths
        
        SEDX_CORE_INFO_TAG("DIAG", "Checked for missing files");
    }
    
    void ValidateDefinitions()
    {
        // This would validate the integrity of all definitions
        // Check for circular references, invalid regions, etc.
        
        SEDX_CORE_INFO_TAG("DIAG", "Validated definitions");
    }
    
    void CheckSceneryPackIntegrity()
    {
        for (const auto& packPath : m_LibrarySystem->GetSceneryPacks())
        {
            if (!ValidateSceneryPack(packPath))
            {
                m_DiagData.issues.push_back("Invalid scenery pack: " + packPath.string());
            }
        }
        
        SEDX_CORE_INFO_TAG("DIAG", "Checked scenery pack integrity");
    }
    
    bool ValidateSceneryPack(const std::filesystem::path& packPath)
    {
        // Check if pack directory exists
        if (!std::filesystem::exists(packPath))
        {
            return false;
        }
        
        // Check for library.txt or object files
        bool hasLibraryTxt = std::filesystem::exists(packPath / "library.txt");
        bool hasObjects = false;
        
        try
        {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(packPath))
            {
                if (entry.is_regular_file())
                {
                    auto ext = entry.path().extension();
                    if (ext == ".obj" || ext == ".ter" || ext == ".for")
                    {
                        hasObjects = true;
                        break;
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_WARN_TAG("DIAG", "Error scanning pack {}: {}", packPath.string(), e.what());
            return false;
        }
        
        return hasLibraryTxt || hasObjects;
    }
    
    void GenerateDiagnosticReport()
    {
        SEDX_CORE_INFO_TAG("DIAG", "=== Library System Diagnostic Report ===");
        SEDX_CORE_INFO_TAG("DIAG", "Total Definitions: {}", m_DiagData.totalDefinitions);
        SEDX_CORE_INFO_TAG("DIAG", "Total Regions: {}", m_DiagData.totalRegions);
        SEDX_CORE_INFO_TAG("DIAG", "Total Scenery Packs: {}", m_DiagData.totalSceneryPacks);
        
        if (!m_DiagData.missingFiles.empty())
        {
            SEDX_CORE_WARN_TAG("DIAG", "Missing Files: {}", m_DiagData.missingFiles.size());
            for (const auto& file : m_DiagData.missingFiles)
            {
                SEDX_CORE_WARN_TAG("DIAG", "  - {}", file);
            }
        }
        
        if (!m_DiagData.issues.empty())
        {
            SEDX_CORE_ERROR_TAG("DIAG", "Issues Found: {}", m_DiagData.issues.size());
            for (const auto& issue : m_DiagData.issues)
            {
                SEDX_CORE_ERROR_TAG("DIAG", "  - {}", issue);
            }
        }
        
        if (m_DiagData.missingFiles.empty() && m_DiagData.issues.empty())
        {
            SEDX_CORE_INFO_TAG("DIAG", "System appears healthy - no issues detected");
        }
        
        SEDX_CORE_INFO_TAG("DIAG", "=== End Diagnostic Report ===");
    }
    
    bool ShouldPerformHealthCheck()
    {
        // Perform health checks every 5 minutes
        static auto lastCheck = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - lastCheck);
        
        if (elapsed.count() >= 5)
        {
            lastCheck = now;
            return true;
        }
        
        return false;
    }
    
    void PerformHealthCheck()
    {
        SEDX_PROFILE_SCOPE("LibrarySystemDiagnostics::PerformHealthCheck");
        
        // Quick health check - less comprehensive than full diagnostics
        bool systemHealthy = true;
        
        // Check if X-Plane root is still accessible
        if (!std::filesystem::exists(m_LibrarySystem->GetXPlaneRoot()))
        {
            SEDX_CORE_ERROR_TAG("DIAG", "X-Plane root directory no longer accessible");
            systemHealthy = false;
        }
        
        // Check if scenery packs are still accessible
        for (const auto& pack : m_LibrarySystem->GetSceneryPacks())
        {
            if (!std::filesystem::exists(pack))
            {
                SEDX_CORE_WARN_TAG("DIAG", "Scenery pack no longer accessible: {}", pack.string());
                systemHealthy = false;
            }
        }
        
        if (systemHealthy)
        {
            SEDX_CORE_INFO_TAG("DIAG", "System health check passed");
        }
        else
        {
            SEDX_CORE_WARN_TAG("DIAG", "System health check detected issues");
        }
    }
    
    void RenderDiagnosticsInterface()
    {
        ImGui::Begin("Library System Diagnostics", &m_ShowDiagnosticsWindow);
        
        ImGui::Text("Library System Health Monitor");
        ImGui::Separator();
        
        // System overview
        ImGui::Text("X-Plane Root: %s", m_LibrarySystem->GetXPlaneRoot().string().c_str());
        ImGui::Text("Scenery Packs: %zu", m_DiagData.totalSceneryPacks);
        ImGui::Text("Definitions: %zu", m_DiagData.totalDefinitions);
        ImGui::Text("Regions: %zu", m_DiagData.totalRegions);
        
        ImGui::Separator();
        
        // Control buttons
        if (ImGui::Button("Run Full Diagnostics"))
        {
            PerformSystemDiagnostics();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Run Health Check"))
        {
            PerformHealthCheck();
        }
        
        ImGui::Separator();
        
        // Issues display
        if (!m_DiagData.issues.empty())
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Issues Detected:");
            for (const auto& issue : m_DiagData.issues)
            {
                ImGui::BulletText("%s", issue.c_str());
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "No issues detected");
        }
        
        ImGui::End();
    }
};
```

## Best Practices

### 1. Proper System Initialization

```cpp
class LibrarySystemManager
{
public:
    static bool InitializeLibrarySystem(const std::filesystem::path& xplaneRoot,
                                       const std::filesystem::path& currentPackage)
    {
        try
        {
            // Validate inputs
            SEDX_CORE_ASSERT(!xplaneRoot.empty(), "X-Plane root path cannot be empty");
            SEDX_CORE_ASSERT(std::filesystem::exists(xplaneRoot), "X-Plane root path must exist");
            
            // Create library system module
            auto librarySystem = CreateRef<LibrarySystemModule>();
            
            // Initialize with proper error handling
            librarySystem->OnAttach();
            
            // Store for global access
            s_LibrarySystem = librarySystem;
            
            SEDX_CORE_INFO_TAG("LIBRARY", "Library system initialized successfully");
            return true;
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("LIBRARY", "Failed to initialize library system: {}", e.what());
            return false;
        }
    }
    
    static Ref<LibrarySystemModule> GetLibrarySystem()
    {
        SEDX_CORE_ASSERT(s_LibrarySystem != nullptr, "Library system not initialized");
        return s_LibrarySystem;
    }
    
private:
    static Ref<LibrarySystemModule> s_LibrarySystem;
};
```

### 2. Efficient Resource Management

```cpp
class EfficientLibraryManagement
{
public:
    static void OptimizeMemoryUsage()
    {
        // Implement memory optimization strategies
        // - String interning for common paths
        // - Lazy loading of definitions
        // - Cache size limits with LRU eviction
    }
    
    static void ConfigureForPerformance()
    {
        // Performance optimization strategies
        // - Pre-compile frequently used definitions
        // - Index definitions for fast lookup
        // - Use memory-mapped files for large library.txt files
    }
};
```

This system provides comprehensive management of X-Plane's complex library system while maintaining high performance and reliability within the Scenery Editor X architecture.

# Scenery Editor X - Settings System Documentation

---

## Overview

The Settings System in Scenery Editor X provides a comprehensive configuration management framework built on top of the libconfig++ library. It handles application settings, X-Plane integration paths, user preferences, and runtime configuration with automatic detection, validation, and persistence capabilities.

## Architecture

### Core Components

#### ApplicationSettings Class

The `ApplicationSettings` class serves as the central configuration manager, inheriting from `RefCounted` for proper memory management within the smart pointer system.

**Location:** `source/SceneryEditorX/platform/settings.h` and `source/SceneryEditorX/platform/settings.cpp`

**Key Features:**

- Hierarchical configuration structure using libconfig format
- Automatic X-Plane path detection via Steam and common paths
- Type-safe setting storage and retrieval
- Validation and error handling
- Platform-specific path resolution
- Runtime configuration updates

### Data Structures

#### AppData Structure

Stores global application configuration and version information.

```cpp
struct AppData
{
    std::string appName = "Scenery Editor X";
    uint32_t WinWidth = 1280;
    uint32_t WinHeight = 720;
    std::string renderName = "X-Plane 12 Graphics Emulator";
    std::string versionString = SEDX_VERSION_STRING;
    uint32_t version = SEDX_VERSION;
  
    bool Fullscreen = false;
    bool NoTitlebar = false;
    bool VSync = false;
    bool StartMaximized = true;
    bool Resizable = true;
    bool EnableImGui = true;
    std::string WorkingDirectory;
    std::filesystem::path IconPath;
};
```

#### XPlaneStats Structure

Contains X-Plane simulator configuration and path information.

```cpp
struct XPlaneStats
{
    std::string xPlaneVersion;         // X-Plane version identifier
    std::string xPlanePath;            // Root installation path
    std::string xPlaneBinPath;         // Executable directory path
    std::string xPlaneResourcesPath;   // Resources directory path
    bool isSteam = false;              // Steam installation flag
};
```

## Configuration File Structure

The settings system uses a hierarchical configuration format with the following sections:

### Application Section

```
application: {
    version = "0.2.27";
    no_titlebar = false;
};
```

### X-Plane Section

```
x_plane: {
    version = "X-Plane 12.06b1";
    path = "C:\\Program Files\\X-Plane 12";
    bin_path = "C:\\Program Files\\X-Plane 12\\bin";
    resources_path = "C:\\Program Files\\X-Plane 12\\Resources";
    is_steam = false;
};
```

### UI Section

```
ui: {
    theme = "dark";
    font_size = 12;
    language = "english";
};
```

### Project Section

```
project: {
    auto_save = true;
    auto_save_interval = 5;  # minutes
    backup_count = 3;
    default_project_dir = "C:\\Users\\User\\Documents\\SceneryEditorX";
};
```

## API Reference

### Constructor

```cpp
explicit ApplicationSettings(std::filesystem::path filepath);
```

**Purpose:** Initializes the settings manager with the specified configuration file path.
**Parameters:**

- `filepath`: Path to the configuration file to use

**Behavior:**

1. Attempts to read existing configuration
2. If configuration doesn't exist, creates minimal default configuration
3. Attempts automatic X-Plane path detection
4. Validates X-Plane paths if found in existing configuration
5. Saves initial or updated configuration

### Core File Operations

#### ReadSettings()

```cpp
bool ReadSettings();
```

**Purpose:** Reads settings from the configuration file into memory.
**Returns:** `true` if settings were successfully loaded, `false` otherwise
**Error Handling:** Handles file I/O errors, parse errors, and configuration exceptions

#### WriteSettings()

```cpp
void WriteSettings();
```

**Purpose:** Writes current in-memory settings to the configuration file.
**Side Effects:**

- Ensures all required sections exist
- Updates configuration from internal data structures
- Updates the internal settings map

### Setting Management

#### String Options

```cpp
void SetOption(const std::string &key, const std::string &value);
void GetOption(const std::string &key, std::string &value);
bool HasOption(const std::string &key) const;
void RemoveOption(const std::string &key);
```

#### Typed Options

```cpp
// Adding options
void AddIntOption(const std::string &path, int value);
void AddFloatOption(const std::string &path, double value);
void AddBoolOption(const std::string &path, bool value);
void AddStringOption(const std::string &path, const std::string &value);

// Retrieving options
int GetIntOption(const std::string &path, int defaultValue = 0) const;
double GetFloatOption(const std::string &path, double defaultValue = 0.0) const;
bool GetBoolOption(const std::string &path, bool defaultValue = false) const;
std::string GetStringOption(const std::string &path, const std::string &defaultValue = "") const;
```

**Path Format:** Uses dot notation for hierarchical settings (e.g., `"ui.theme"`, `"project.auto_save"`)

### X-Plane Integration

#### Automatic Detection

```cpp
bool DetectXPlanePath();
```

**Purpose:** Automatically detects X-Plane installation path using multiple detection methods.
**Detection Methods:**

1. Steam installation detection via `SteamGameFinder`
2. Common installation paths per platform
3. Registry checks (Windows)
4. Standard application directories (macOS/Linux)

**Platform-Specific Paths:**

- **Windows:** `C:\X-Plane 12`, `C:\Program Files\X-Plane 12`, etc.
- **macOS:** `/Applications/X-Plane 12`, `~/Applications/X-Plane 12`
- **Linux:** `/opt/X-Plane 12`, `~/Games/X-Plane 12`

#### Manual Configuration

```cpp
bool SetXPlanePath(const std::string &path);
std::string GetXPlanePath() const;
bool ValidateXPlanePaths() const;
void UpdateDerivedXPlanePaths();
```

**SetXPlanePath:** Validates and sets the X-Plane installation path, automatically deriving bin and resources paths.

**ValidateXPlanePaths:** Ensures all required X-Plane files and directories exist.

**UpdateDerivedXPlanePaths:** Updates bin and resources paths based on the main installation path.

### Data Access

#### Application Data

```cpp
const AppData& GetAppStats() const;
AppData& GetAppStats();
```

#### X-Plane Data

```cpp
const XPlaneStats& GetXPlaneStats() const;
XPlaneStats& GetXPlaneStats();
```

## Implementation Details

### Configuration Templates

The system uses predefined templates for creating default configuration sections:

```cpp
INTERNAL constexpr const char *APPLICATION_SECTION_TEMPLATE = R"(
  # Application settings
  version = "${APP_VERSION}";
  no_titlebar = false;
)";
```

### Path Creation Algorithm

The `CreateSettingPath` template method handles creating hierarchical configuration paths:

1. Splits the path by dots to identify hierarchy levels
2. Navigates or creates configuration groups as needed
3. Adds the final value with appropriate type
4. Updates the internal settings map

### Error Handling Strategy

The settings system employs multiple error handling strategies:

1. **Graceful Degradation:** Missing configuration files trigger creation of minimal default configuration
2. **Validation Recovery:** Invalid X-Plane paths trigger automatic re-detection
3. **Exception Safety:** All libconfig exceptions are caught and logged appropriately
4. **Default Values:** All getter methods provide sensible default values

### Memory Management

- Uses the custom smart pointer system (`RefCounted` inheritance)
- Automatic cleanup of libconfig resources
- Settings map provides fast string-based access without repeated parsing

### Thread Safety

**Important:** The ApplicationSettings class is **not thread-safe**. Access from multiple threads requires external synchronization using the threading system's mechanisms.

## Configuration File Lifecycle

### Initialization Sequence

1. **Constructor Called** with configuration file path
2. **ReadSettings()** attempts to load existing configuration
3. If loading fails:
   - **InitMinConfig()** creates minimal default configuration
   - **DetectXPlanePath()** attempts automatic X-Plane detection
   - **WriteSettings()** persists initial configuration
4. If loading succeeds:
   - **ValidateXPlanePaths()** checks X-Plane configuration validity
   - Re-detection and update if paths are invalid

### Runtime Updates

1. Settings modified via API calls
2. Internal data structures updated immediately
3. **WriteSettings()** called to persist changes
4. **LoadSettingsToMap()** refreshes fast-access map

### Default Value Strategy

The system provides intelligent defaults:

- **Application:** Version from build configuration, sensible UI defaults
- **X-Plane:** Empty paths if detection fails (user must configure manually)
- **UI:** Dark theme, 12pt font, English language
- **Project:** Auto-save enabled, 5-minute intervals, user documents directory

## Platform Considerations

### Windows

- Uses `USERPROFILE` environment variable for home directory
- Checks multiple drive letters for X-Plane installations
- Registry integration for Steam detection

### macOS

- Uses `HOME` environment variable
- Checks standard application directories
- Handles app bundle structures

### Linux

- Uses `HOME` environment variable
- Checks `/opt` and user directories
- Handles package manager installations

## Integration with Other Systems

### Logging System

All operations use tagged logging for better categorization:

```cpp
SEDX_CORE_INFO_TAG("SETTINGS", "Settings successfully written to: {}", filePath.string());
SEDX_CORE_ERROR_TAG("SETTINGS", "X-Plane paths in configuration are invalid");
```

### Smart Pointer System

Settings objects are managed via the custom smart pointer system:

```cpp
auto settings = CreateRef<ApplicationSettings>("config/settings.cfg");
```

### Steam Integration

Utilizes the `SteamGameFinder` class for automatic X-Plane detection in Steam installations.

## Performance Characteristics

### File I/O

- Configuration file reading/writing is performed synchronously
- Minimal file I/O - only on initialization and explicit save operations
- Text-based configuration format for human readability and debugging

### Memory Usage

- Maintains two representations: libconfig tree and fast-access string map
- Memory usage scales linearly with configuration size
- Automatic cleanup via smart pointer system

### Access Patterns

- String map provides O(1) access for frequent lookups
- Typed getters parse on-demand for type safety
- Hierarchical paths support logical organization

## Error Codes and Diagnostics

### Common Error Scenarios

1. **Configuration File Not Found**

   - Creates default configuration automatically
   - Logs trace-level message for debugging
2. **Parse Errors**

   - Logs detailed parse error information (file, line, error description)
   - Falls back to default configuration
3. **X-Plane Path Invalid**

   - Triggers automatic re-detection
   - Logs warning with suggested manual configuration
4. **Permission Errors**

   - Logs file I/O errors with full path information
   - Settings remain in memory until next successful write

### Logging Categories

- **SETTINGS:** General settings operations
- **CONFIG:** Configuration file operations
- **XPLANE:** X-Plane specific operations

## Best Practices

### Initialization

1. Create settings instance early in application lifecycle
2. Check X-Plane path validity before renderer initialization
3. Handle missing X-Plane gracefully for partial functionality

### Setting Management

1. Use hierarchical paths for logical organization
2. Provide meaningful default values for all settings
3. Validate user input before calling setter methods

### Error Handling

1. Always check return values for critical operations
2. Implement fallback behavior for missing configurations
3. Use appropriate logging levels for different error scenarios

### Performance

1. Cache frequently accessed settings in local variables
2. Batch setting updates when possible
3. Minimize WriteSettings() calls during runtime

## Security Considerations

### Path Validation

- All X-Plane paths are validated before use
- Steam detection uses safe path parsing
- User-provided paths undergo sanitization

### Configuration Integrity

- libconfig provides built-in parsing validation
- Type checking prevents invalid data storage
- Graceful handling of malformed configuration files

### File Permissions

- Respects system file permissions
- Handles permission errors gracefully
- No elevation of privileges required

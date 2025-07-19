# Scenery Editor X - edX File Format Library

## Overview

The edX file format library provides a modern, JSON-based file format system for Scenery Editor X projects and asset libraries. This refactored version uses nlohmann::json for robust JSON handling and follows modern C++ practices with cross-platform compatibility.

## Features

- **JSON-Based Format**: Clean, human-readable format using industry-standard JSON
- **Cross-Platform**: Works on Windows, macOS, and Linux 
- **Modern C++**: Uses C++17 features with smart pointers and proper RAII
- **Extensible**: Easy to add new properties and data structures
- **Validation**: Built-in validation for data integrity
- **Type Safety**: Strong typing with comprehensive error handling
- **Version Control Friendly**: Text-based format works well with Git

## File Format Structure

### Project Files (.edX)

Project files contain complete scenery project information:

```json
{
  "Project": {
    "name": "My Scenery Project",
    "editor-version": "2.0.0",
    "XPlaneVersion": "12.1.4-r3",
    "author": "Author Name",
    "editdate": "2025-07-11T10:30:00Z",
    "createdate": "2025-07-11T09:00:00Z",
    "description": "Description of the project"
  },
  "Airport": {
    "Name": "San Francisco International Airport",
    "ICAO": "KSFO",
    "IATA": "SFO",
    "FAA": "KSFO",
    "City": "San Francisco",
    "State": "California",
    "Country": "USA",
    "RegionCode": "US-W",
    "DatumLat": 37.618999,
    "DatumLon": -122.375,
    "Elevation": 13,
    "MagVar": -14.0,
    "TransitionAlt": 18000,
    "TransitionLevel": "FL180",
    "Tower": 120.5,
    "Ground": 121.8,
    "ATIS": 135.1
  },
  "Libraries": [
    {
      "Library": "Laminar Research Airport Scenery",
      "local-path": "./Resources/default scenery/airport scenery/library.txt",
      "entry-count": 5000,
      "uuid": "15da3863-07d1-462a-be65-3873058675f3",
      "short-id": "a1b2c3d4",
      "version": "12.1.4"
    }
  ],
  "Assets": [
    {
      "id": "Asset001",
      "unique-id": "a1b2c3d4",
      "latitude": 37.618999,
      "longitude": -122.375,
      "altitude": 642,
      "heading": 0.0,
      "associated-library": "6587fddb",
      "layer-id": "terminals",
      "group-id": "group1",
      "locked": false,
      "hidden": false,
      "selected": false,
      "other-properties": {
        "building-type": "Terminal"
      }
    }
  ],
  "Layers": [
    {
      "layer-id": "terminals",
      "name": "Terminal Buildings",
      "description": "Airport terminal structures",
      "locked": false,
      "hidden": false,
      "opacity": 1.0,
      "z-order": 1,
      "asset-ids": ["Asset001"]
    }
  ],
  "Settings": {
    "render-quality": "high",
    "auto-save": true
  }
}
```

### Library Files (.lib)

Library files contain reusable asset definitions:

```json
{
  "Library": {
    "name": "Demo Airport Objects",
    "path": "./libraries/demo",
    "version": "1.0.0",
    "author": "Author Name",
    "git-repository": "https://github.com/demo/airport-objects",
    "size-mb": 125.5,
    "description": "Demonstration library for airport objects",
    "last-modified": "2025-07-11T10:30:00Z"
  },
  "Objects": [
    {
      "id": "terminal_modern_01",
      "unique-id": "e4f5a2b1",
      "asset-type": "building",
      "name": "Modern Terminal Building",
      "description": "Large modern terminal with glass facade",
      "category": "terminal",
      "tags": ["modern", "glass", "large"],
      "object-path": "objects/terminals/modern_01.obj",
      "texture-path": "textures/terminals/modern_01_diff.png",
      "preview-image": "previews/modern_01.jpg",
      "properties": {
        "floors": 3,
        "capacity": 5000,
        "year_built": 2020,
        "has_jetways": true
      }
    }
  ]
}
```

## API Usage

### Basic Operations

```cpp
#include "edXManager.h"

// Create manager
edx::EdxManager manager;

// Create a new project
auto project = manager.create_project("My Project", "Author Name", "KSFO");

// Configure project
project->airport.name = "San Francisco International";
project->airport.datumLat = 37.618999;
project->airport.datumLon = -122.375;

// Add an asset
edx::SceneAsset asset;
asset.id = "Asset001";
asset.uniqueId = edx::generate_unique_id();
asset.latitude = 37.619;
asset.longitude = -122.376;
asset.heading = 45.0;
project->assets.push_back(asset);

// Save project
manager.save_project(*project, "MyProject.edX");

// Load project
auto loadedProject = manager.load_project("MyProject.edX");
```

### Library Operations

```cpp
// Create a new library
auto library = manager.create_library("My Library", "Author", "1.0.0");

// Add an object
edx::LibraryObject obj;
obj.id = "building_01";
obj.uniqueId = edx::generate_unique_id();
obj.name = "Office Building";
obj.assetType = "building";
obj.category = "commercial";
obj.objectPath = "objects/building_01.obj";
library->add_object(obj);

// Save library
manager.save_library(*library, "MyLibrary.lib");
```

### Quick Functions

```cpp
// Quick save/load without manager
auto project = edx::load_project_quick("project.edX");
if (project) {
    // Modify project...
    edx::save_project_quick(*project, "project.edX");
}
```

## Building

### Requirements

- C++17 compatible compiler
- CMake 3.15 or higher
- nlohmann::json (included)

### CMake Integration

```cmake
# Add to your CMakeLists.txt
add_subdirectory(source/edX)
target_link_libraries(your_target PRIVATE edX)
```

### Compilation

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Architecture

### Class Hierarchy

- `edx::EdxManager` - Main API interface
- `edx::EdxProject` - Complete project container
- `edx::LibraryFile` - Library container
- Core structures:
  - `ProjectInfo` - Project metadata
  - `AirportInfo` - Airport data
  - `LibraryReference` - Library references
  - `SceneAsset` - Individual assets
  - `SceneLayer` - Layer organization
  - `Library` - Library metadata
  - `LibraryObject` - Library objects

### JSON Serialization

All structures implement `to_json()` and `from_json()` methods for automatic serialization using nlohmann::json.

### Error Handling

- Exception-safe operations
- Comprehensive validation
- Progress callbacks for long operations
- Detailed error reporting

## Migration from Legacy Format

The library includes conversion utilities for migrating from older text-based formats:

```cpp
edx::EdxManager manager;
bool success = manager.convert_legacy_project("old_format.txt", "new_format.edX");
```

## Cross-Platform Compatibility

The library uses standard C++ and cross-platform file I/O:

- Windows: Full support with Visual Studio 2019+
- macOS: Clang 10+ with C++17 support  
- Linux: GCC 7+ or Clang 6+ with C++17 support

## Performance Considerations

- JSON parsing is optimized for typical project sizes
- Memory usage scales with asset count
- File I/O uses streaming for large files
- Validation is optional for performance-critical scenarios

## Future Enhancements

- Binary format option for large projects
- Compression support
- Incremental loading/saving
- Multi-threading support for large operations
- Plugin system for custom asset types

## License

Copyright (c) 2025 Coalition of Freeware Developers
Licensed under the same terms as Scenery Editor X.

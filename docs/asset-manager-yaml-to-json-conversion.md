# Asset Manager YAML to JSON Conversion

## Summary of Changes

This document summarizes the conversion of the Asset Manager from YAML serialization to JSON serialization using the project's standard nlohmann::json library.

## Files Modified

### 1. editor_asset_manager.cpp

**Includes Updated:**
- Added: `#include <nlohmann/json.hpp>`
- The project already has nlohmann-json as a dependency in vcpkg.json

**LoadAssetRegistry() Function:**
- Replaced `YAML::Node data = YAML::Load(strStream.str());` with `nlohmann::json data = nlohmann::json::parse(strStream.str());`
- Replaced `data["Assets"]` validation with proper JSON type checking
- Replaced `entry["FilePath"].as<std::string>()` with `entry["FilePath"].get<std::string>()`
- Replaced `entry["Handle"].as<uint64_t>()` with `entry["Handle"].get<uint64_t>()`
- Replaced `entry["Type"].as<std::string>()` with `entry["Type"].get<std::string>()`
- Added comprehensive error handling with try-catch blocks for:
  - `nlohmann::json::parse_error` - Invalid JSON syntax
  - `nlohmann::json::type_error` - Type conversion errors
  - `std::exception` - General exceptions
- Added validation to ensure "Assets" field is an array
- Added validation to ensure required fields exist in each entry

**WriteRegistryToFile() Function:**
- Replaced YAML::Emitter with nlohmann::json construction
- Replaced YAML key-value writing with JSON object creation
- Used `nlohmann::json::array()` for the Assets collection
- Used `registryJson.dump(2)` for pretty-printed output with 2-space indentation
- Maintained the same data structure and sorting logic

## JSON Format

The new JSON format maintains the same logical structure as the previous YAML:

```json
{
  "Assets": [
    {
      "Handle": 1001,
      "FilePath": "models/building1.obj",
      "Type": "StaticMesh"
    },
    {
      "Handle": 1002,
      "FilePath": "textures/building1_diffuse.png", 
      "Type": "Texture2D"
    }
  ]
}
```

## Benefits of the Conversion

1. **Consistency**: Aligns with the project's standard JSON serialization approach used throughout the codebase
2. **Performance**: nlohmann::json is generally faster than yaml-cpp for parsing and generation
3. **Error Handling**: Better error reporting with specific exception types
4. **Validation**: More robust validation of data structure and types
5. **Maintainability**: Reduces dependency on yaml-cpp library
6. **Integration**: Better integration with existing JSON-based systems in the project

## Compatibility

- The conversion maintains full backward compatibility with existing asset registry data structure
- The loading function includes robust error handling for malformed JSON
- Field validation ensures data integrity during loading
- File paths are still normalized (backslash to forward slash conversion on Windows)

## Testing

The conversion includes:
- Parse error handling for invalid JSON syntax
- Type error handling for incorrect data types
- Field existence validation 
- Array type validation for the Assets collection
- Comprehensive logging for debugging

Example asset registry file has been created at `docs/asset-registry-example.json` for reference.

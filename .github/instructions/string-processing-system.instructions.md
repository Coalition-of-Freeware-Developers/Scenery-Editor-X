# String Processing System Instructions for GitHub Copilot

## Overview

This document provides GitHub Copilot agents and coding assistants with comprehensive instructions for working with the Scenery Editor X string processing system. These instructions ensure consistent usage patterns, performance optimization, and adherence to the established architecture when implementing string-related functionality.

## System Architecture Understanding

### Three-Layer String Processing System

When working with strings in Scenery Editor X, understand the three-layer architecture:

1. **StringUtils Namespace** (`string_utils.h`) - Low-level, template-based utilities
2. **String Namespace** (`string.h`/`string.cpp`) - High-level string operations
3. **Global Functions** - File path processing and specialized utilities

**Rule**: Always use the most appropriate layer for your use case:
- Use `StringUtils` for performance-critical template operations
- Use `String` namespace for general string processing
- Use global functions for file path and data formatting operations

### Performance-First Design Principles

**Memory Efficiency**:
```cpp
// ✅ ALWAYS prefer string_view for read-only operations
void ProcessFilename(std::string_view filename) {
    auto extension = GetExtension(std::string(filename)); // Convert only when needed
}

// ❌ NEVER create unnecessary copies
void ProcessFilename(const std::string& filename) {
    auto copy = filename; // Unnecessary allocation
}
```

**Move Semantics**:
```cpp
// ✅ ALWAYS use move semantics for temporary strings
std::string ProcessUserInput(std::string input) {
    input = String::TrimWhitespace(std::move(input));
    String::ToLower(input);  // In-place modification
    return input;  // Automatic move
}

// ✅ ALWAYS use move in utility calls
auto cleaned = StringUtils::trim(std::move(dirtyString));
```

## Function Selection Guidelines

### Case Conversion Operations

**Use `String` namespace for general case operations**:
```cpp
// ✅ Correct usage patterns
bool isImageFile = String::EqualsIgnoreCase(extension, "png");
String::ToLower(identifier);  // In-place modification
auto lowercase = String::ToLowerCopy(originalText);  // When original needed
```

**Platform-specific comparisons**:
```cpp
// ✅ Use CompareCase for sorting and ordering
std::sort(filenames.begin(), filenames.end(), [](const auto& a, const auto& b) {
    return String::CompareCase(a, b) < 0;
});
```

### String Trimming and Cleaning

**Choose appropriate trimming function**:
```cpp
// ✅ Use StringUtils for performance-critical code
std::string_view ProcessConfigLine(std::string_view line) {
    return StringUtils::trim(line);  // No allocation
}

// ✅ Use String namespace for general text processing
std::string CleanUserInput(const std::string& input) {
    auto cleaned = String::TrimWhitespace(input);
    String::Erase(cleaned, "\r\n\t");
    return cleaned;
}
```

**Character removal patterns**:
```cpp
// ✅ Remove specific characters efficiently
String::Erase(text, "!@#$%^&*()");  // Remove multiple characters
String::Erase(text, "\r\n");        // Remove line endings

// ✅ Remove outer quotes/brackets
auto unquoted = StringUtils::removeDoubleQuotes(std::move(quotedText));
auto content = StringUtils::removeOuterCharacter(std::move(bracketedText), '[');
```

### String Splitting and Joining

**Use appropriate splitting method**:
```cpp
// ✅ Simple single-character splitting
auto tokens = SplitString(csvLine, ',');

// ✅ Multiple delimiter characters  
auto components = SplitString(path, "/\\");

// ✅ Complex delimiter logic with StringUtils
auto customSplit = StringUtils::splitString(text, [](char c) {
    return std::isspace(c) || c == ',' || c == ';';
}, false);

// ✅ Line-based processing
auto lines = StringUtils::splitIntoLines(fileContent, false);
```

**Efficient string joining**:
```cpp
// ✅ Pre-calculate size for large joins
std::string BuildPath(const std::vector<std::string>& components) {
    #ifdef SEDX_PLATFORM_WINDOWS
        return StringUtils::joinStrings(components, "\\");
    #else
        return StringUtils::joinStrings(components, "/");
    #endif
}

// ✅ SQL query construction
std::string query = "SELECT " + StringUtils::joinStrings(columns, ", ") + 
                   " FROM " + tableName;
```

### Multi-String Replace Operations

**Use StringUtils::replace for multiple replacements**:
```cpp
// ✅ Efficient multiple replacements in single pass
std::string SanitizeFilename(const std::string& filename) {
    return StringUtils::replace(filename,
        "<", "",
        ">", "",
        ":", "",
        "\"", "",
        "|", "",
        "?", "",
        "*", ""
    );
}

// ✅ HTML entity encoding
std::string EncodeHtml(const std::string& text) {
    return StringUtils::replace(text,
        "&", "&amp;",    // Must be first!
        "<", "&lt;",
        ">", "&gt;",
        "\"", "&quot;"
    );
}
```

## File Path Processing Standards

### Always Use Global Functions for File Operations

```cpp
// ✅ Correct file path processing
void ProcessAssetFile(const std::string& filePath) {
    auto filename = GetFilename(filePath);           // Extract filename
    auto extension = GetExtension(std::string(filename));    // Get extension
    auto baseName = RemoveExtension(std::string(filename));  // Remove extension
    
    String::ToLower(extension);  // Normalize extension for comparison
    
    if (extension == "png" || extension == "jpg") {
        LoadImageAsset(filePath);
    }
}

// ✅ Asset organization by type
void OrganizeAssets(const std::vector<std::string>& assetPaths) {
    std::map<std::string, std::vector<std::string>> assetsByType;
    
    for (const auto& path : assetPaths) {
        auto extension = String::ToLowerCopy(GetExtension(path));
        assetsByType[extension].push_back(path);
    }
}
```

### File Content Processing

**Always handle BOM correctly**:
```cpp
// ✅ Use ReadFileAndSkipBOM for text files
std::string LoadConfigFile(const std::filesystem::path& configPath) {
    auto content = ReadFileAndSkipBOM(configPath);
    
    if (content.empty()) {
        SEDX_CORE_ERROR_TAG("Config", "Failed to read config file: {}", configPath.string());
        return "";
    }
    
    // Note: First character is always '\t' (dummy character)
    auto lines = StringUtils::splitIntoLines(content.substr(1), false);
    return ProcessConfigLines(lines);
}

// ✅ Manual BOM handling when needed
std::ifstream file("data.txt");
int skippedBytes = SkipBOM(file);
// File stream is now positioned correctly
```

## Data Formatting Standards

### Size and Duration Formatting

**Use appropriate formatting functions**:
```cpp
// ✅ Memory usage reporting
void ReportMemoryUsage() {
    auto stats = Allocator::GetAllocationStats();
    
    for (const auto& [category, allocation] : stats) {
        size_t currentUsage = allocation.TotalAllocated - allocation.TotalFreed;
        auto usageStr = BytesToString(currentUsage);
        
        SEDX_CORE_INFO_TAG("Memory", "Category '{}': {}", category, usageStr);
    }
}

// ✅ Performance timing
class PerformanceTimer {
public:
    ~PerformanceTimer() {
        auto duration = std::chrono::high_resolution_clock::now() - startTime;
        auto durationStr = StringUtils::getDurationDescription(
            std::chrono::duration_cast<std::chrono::microseconds>(duration));
        
        SEDX_CORE_INFO_TAG("Performance", "{} completed in {}", 
                          operationName, durationStr);
    }
};
```

### Type Name Processing

**Make type names user-friendly**:
```cpp
// ✅ Debug UI type display
template<typename T>
void DisplayTypeInfo() {
    auto typeName = typeid(T).name();
    auto friendlyName = CreateUserFriendlyTypeName(typeName);
    auto readableName = TemplateToParenthesis(friendlyName);
    
    ImGui::Text("Type: %s", readableName.c_str());
}

// ✅ Error message formatting
template<typename T>
void ReportTypeError(const std::string& operation) {
    auto typeName = CreateUserFriendlyTypeName(typeid(T).name());
    SEDX_CORE_ERROR_TAG("TypeError", "Cannot {} on type '{}'", 
                        operation, typeName);
}
```

## Integration with Scenery Editor X Architecture

### Module System Integration

**String processing in modules**:
```cpp
class FileManagerModule : public Module {
private:
    std::unordered_map<std::string, std::string> extensionMap;
    
public:
    void OnAttach() override {
        SEDX_CORE_INFO_TAG("FileManager", "Initializing file manager");
        BuildExtensionMap();
    }
    
    bool IsValidAssetFile(const std::string& filename) const {
        auto extension = String::ToLowerCopy(GetExtension(filename));
        return extensionMap.contains(extension);
    }
    
private:
    void BuildExtensionMap() {
        // Use string processing to categorize file types
        std::vector<std::string> imageExts = {"png", "jpg", "jpeg", "bmp"};
        for (const auto& ext : imageExts) {
            extensionMap[ext] = "Image";
        }
    }
};
```

### Configuration System Integration

**Configuration file processing**:
```cpp
class ConfigurationModule : public Module {
public:
    void LoadConfiguration() {
        auto content = ReadFileAndSkipBOM(configPath);
        auto lines = StringUtils::splitIntoLines(content.substr(1), false);
        
        for (const auto& line : lines) {
            auto trimmed = StringUtils::trim(line);
            
            if (trimmed.empty() || trimmed[0] == '#') {
                continue;  // Skip empty lines and comments
            }
            
            ParseConfigLine(trimmed);
        }
    }
    
private:
    void ParseConfigLine(std::string_view line) {
        auto equalPos = line.find('=');
        if (equalPos == std::string_view::npos) return;
        
        auto key = StringUtils::trim(line.substr(0, equalPos));
        auto value = StringUtils::trim(line.substr(equalPos + 1));
        
        // Remove quotes if present
        auto cleanValue = StringUtils::removeDoubleQuotes(std::string(value));
        
        settings[String::ToLowerCopy(key)] = cleanValue;
    }
};
```

### Logging System Integration

**Consistent logging with string utilities**:
```cpp
// ✅ Use string formatting in log messages
void ProcessAssets(const std::vector<std::string>& assetPaths) {
    SEDX_CORE_INFO_TAG("AssetProcessor", "Processing {} assets", assetPaths.size());
    
    for (const auto& path : assetPaths) {
        auto filename = GetFilename(path);
        auto extension = String::ToLowerCopy(GetExtension(std::string(filename)));
        
        SEDX_CORE_TRACE_TAG("AssetProcessor", "Processing {}.{}", 
                           RemoveExtension(std::string(filename)), extension);
        
        // Process asset...
    }
}
```

## Error Handling Patterns

### Safe String Operations

**Always validate inputs**:
```cpp
// ✅ Validate before processing
std::string ProcessUserInput(std::string_view input) {
    if (input.empty()) {
        throw std::invalid_argument("Input cannot be empty");
    }
    
    auto cleaned = String::TrimWhitespace(std::string(input));
    if (cleaned.empty()) {
        throw std::invalid_argument("Input contains only whitespace");
    }
    
    return cleaned;
}

// ✅ Safe file operations
std::optional<std::string> SafeReadFile(const std::filesystem::path& path) {
    try {
        if (!std::filesystem::exists(path)) {
            SEDX_CORE_WARN_TAG("FileIO", "File does not exist: {}", path.string());
            return std::nullopt;
        }
        
        auto content = ReadFileAndSkipBOM(path);
        if (content.empty()) {
            SEDX_CORE_WARN_TAG("FileIO", "File is empty: {}", path.string());
            return std::nullopt;
        }
        
        return content;
        
    } catch (const std::exception& e) {
        SEDX_CORE_ERROR_TAG("FileIO", "Error reading {}: {}", path.string(), e.what());
        return std::nullopt;
    }
}
```

### Graceful Degradation

**Handle edge cases gracefully**:
```cpp
// ✅ Graceful handling of malformed input
std::vector<std::string> ParseCsvLine(std::string_view line) {
    auto fields = SplitString(line, ',');
    
    // Clean up each field
    for (auto& field : fields) {
        field = String::TrimWhitespace(std::move(field));
        
        // Handle quoted fields
        if (field.size() >= 2 && field.front() == '"' && field.back() == '"') {
            field = StringUtils::removeDoubleQuotes(std::move(field));
            // Unescape internal quotes
            field = StringUtils::replace(field, "\"\"", "\"");
        }
    }
    
    return fields;
}
```

## Performance Optimization Guidelines

### Memory Allocation Minimization

**Use string_view for read-only operations**:
```cpp
// ✅ Minimize allocations
void ProcessLogLines(std::string_view logContent) {
    auto lines = StringUtils::splitIntoLines(logContent, false);
    
    for (const auto& line : lines) {
        auto trimmed = StringUtils::trim(line);  // Returns string_view
        
        if (StringUtils::startsWith(trimmed, "[ERROR]")) {
            ProcessErrorLine(trimmed);
        }
    }
}
```

**Reserve capacity for large operations**:
```cpp
// ✅ Reserve space for known size operations
std::string BuildLargeString(const std::vector<std::string>& parts) {
    size_t totalSize = 0;
    for (const auto& part : parts) {
        totalSize += part.size();
    }
    
    std::string result;
    result.reserve(totalSize + parts.size());  // Include separators
    
    return StringUtils::joinStrings(parts, " ");
}
```

### Algorithm Selection

**Choose appropriate algorithms**:
```cpp
// ✅ Use efficient search for known patterns
bool IsConfigurationFile(const std::string& filename) {
    auto extension = String::ToLowerCopy(GetExtension(filename));
    
    // Use efficient set lookup for multiple checks
    static const std::unordered_set<std::string> configExtensions = {
        "cfg", "ini", "conf", "json", "xml", "yaml", "toml"
    };
    
    return configExtensions.contains(extension);
}

// ✅ Use appropriate string matching
std::string FindBestMatch(std::string_view input, 
                         const std::vector<std::string>& candidates) {
    // Use exact match first (O(1) with hash map)
    std::unordered_set<std::string> candidateSet(candidates.begin(), candidates.end());
    if (candidateSet.contains(std::string(input))) {
        return std::string(input);
    }
    
    // Fall back to fuzzy matching (expensive)
    std::string bestMatch;
    size_t bestDistance = SIZE_MAX;
    
    for (const auto& candidate : candidates) {
        auto distance = StringUtils::getLevenshteinDistance(input, candidate);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestMatch = candidate;
        }
    }
    
    return bestMatch;
}
```

## Common Anti-Patterns to Avoid

### Memory Inefficiency

```cpp
// ❌ DON'T create unnecessary string copies
void ProcessFilesBad(const std::vector<std::string>& filePaths) {
    for (auto path : filePaths) {  // Copy!
        auto filename = GetFilename(path);  // Another copy!
        auto extension = GetExtension(std::string(filename));  // Yet another copy!
    }
}

// ✅ DO use references and views
void ProcessFilesGood(const std::vector<std::string>& filePaths) {
    for (const auto& path : filePaths) {  // Reference
        auto filename = GetFilename(path);  // string_view
        auto extension = GetExtension(std::string(filename));  // Convert only when needed
    }
}
```

### Inefficient String Building

```cpp
// ❌ DON'T use string concatenation in loops
std::string BuildPathBad(const std::vector<std::string>& components) {
    std::string result;
    for (const auto& component : components) {
        result += component + "/";  // Reallocates every iteration
    }
    return result;
}

// ✅ DO use joinStrings or reserve capacity
std::string BuildPathGood(const std::vector<std::string>& components) {
    return StringUtils::joinStrings(components, "/");
}
```

### Case Sensitivity Issues

```cpp
// ❌ DON'T ignore case sensitivity in comparisons
bool IsImageFileBad(const std::string& filename) {
    auto ext = GetExtension(filename);
    return ext == "png" || ext == "jpg";  // Misses "PNG", "JPG"
}

// ✅ DO normalize case for comparisons
bool IsImageFileGood(const std::string& filename) {
    auto ext = String::ToLowerCopy(GetExtension(filename));
    return ext == "png" || ext == "jpg" || ext == "jpeg";
}
```

## Testing and Validation Guidelines

### Unit Test Patterns

**Test edge cases and performance**:
```cpp
TEST_CASE("String processing handles edge cases", "[string]") {
    // Empty strings
    REQUIRE(StringUtils::trim("") == "");
    REQUIRE(String::TrimWhitespace("   ") == "");
    
    // Unicode and special characters
    auto result = StringUtils::replace("héllo wörld", "ö", "o");
    REQUIRE(result == "héllo world");
    
    // Large inputs (performance test)
    std::string large(10000, 'a');
    auto start = std::chrono::high_resolution_clock::now();
    String::ToLower(large);
    auto duration = std::chrono::high_resolution_clock::now() - start;
    REQUIRE(duration < std::chrono::milliseconds(10));
}
```

### Memory Leak Detection

**Validate memory usage**:
```cpp
TEST_CASE("String operations don't leak memory", "[string][memory]") {
    auto initialStats = Allocator::GetAllocationStats();
    
    {
        // Perform many string operations
        for (int i = 0; i < 1000; ++i) {
            auto result = String::ToLowerCopy("TEST STRING " + std::to_string(i));
            auto trimmed = StringUtils::trim(result);
            auto parts = SplitString(result, ' ');
        }
    }  // All temporary objects destroyed
    
    auto finalStats = Allocator::GetAllocationStats();
    // Memory usage should return to baseline
    REQUIRE(finalStats.TotalAllocated - finalStats.TotalFreed <= 
            initialStats.TotalAllocated - initialStats.TotalFreed + TOLERANCE);
}
```

## Summary of Key Rules

1. **Function Selection**: Use the appropriate layer (StringUtils/String/Global) for your use case
2. **Performance**: Prefer `string_view` for read-only operations, use move semantics for temporary strings
3. **Memory**: Reserve capacity for large operations, avoid unnecessary string copies
4. **File Processing**: Always use `ReadFileAndSkipBOM` for text files, handle the dummy tab character
5. **Case Sensitivity**: Always normalize case for comparisons, use `String::EqualsIgnoreCase` when appropriate
6. **Error Handling**: Validate inputs, use logging system for errors, provide graceful degradation
7. **Module Integration**: Follow Module system patterns, use appropriate logging tags
8. **Testing**: Test edge cases, validate memory usage, include performance tests

These instructions ensure that all string processing code in Scenery Editor X follows consistent patterns, maintains high performance, and integrates properly with the existing architecture.

# Scenery Editor X - String Utilities System Documentation

---



## Overview

The Scenery Editor X String Utilities System provides a comprehensive collection of string manipulation, processing, and utility functions designed for high-performance C++ applications. This system consists of three main components that work together to provide robust string handling capabilities:

1. **StringUtils Namespace** (`string_utils.h`) - Low-level string processing utilities
2. **String Namespace** (`string.h`/`string.cpp`) - High-level string operations and formatting
3. **Global String Functions** - File path manipulation and specialized utilities

This documentation serves as a comprehensive guide for developers and AI coding assistants to effectively utilize and maintain consistency with the existing string processing framework.

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [StringUtils Namespace Reference](#stringutils-namespace-reference)
3. [String Namespace Reference](#string-namespace-reference)
4. [Global String Functions](#global-string-functions)
5. [Usage Patterns and Examples](#usage-patterns-and-examples)
6. [Best Practices](#best-practices)
7. [Performance Considerations](#performance-considerations)
8. [Integration Guidelines](#integration-guidelines)

## Architecture Overview

### Design Philosophy

The string utilities system follows several key design principles:

- **Efficiency**: Template-based implementations with compile-time optimizations
- **Flexibility**: Support for multiple string types (`std::string`, `std::string_view`, `const char*`)
- **Safety**: Null-safe operations and bounds checking
- **Consistency**: Unified interface across different string operations
- **Modern C++**: Leverages C++17/C++20 features for improved performance

### Component Hierarchy

```
┌─────────────────────────────────────────────────────────────────┐
│                    Application Layer                             │
├─────────────────────────────────────────────────────────────────┤
│  File Operations  │  String Formatting  │  Text Processing      │
├─────────────────────────────────────────────────────────────────┤
│                    String Namespace                              │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │ Case Operations │  │ Whitespace Mgmt │  │ Time/Size Utils │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│                    StringUtils Namespace                         │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │ Trim Operations │  │ Search/Replace  │  │ Type Conversion │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│                    Standard Library Foundation                   │
│              <string>, <string_view>, <algorithm>               │
└─────────────────────────────────────────────────────────────────┘
```

## StringUtils Namespace Reference

### Core Character Classification

```cpp
namespace SceneryEditorX::StringUtils
{
    inline bool isWhitespace(char c);
    inline bool isDigit(char c);
}
```

**Purpose**: Efficient character classification functions optimized for common use cases.

**Implementation Details**:

- `isWhitespace()`: Checks for space, tab, newline, carriage return, form feed, and vertical tab
- `isDigit()`: Fast digit checking using arithmetic operations instead of character comparisons

**Usage Examples**:

```cpp
// Character classification in parsing
bool ParseToken(std::string_view input, size_t& pos) {
    // Skip whitespace
    while (pos < input.size() && StringUtils::isWhitespace(input[pos])) {
        ++pos;
    }
  
    // Parse number
    if (pos < input.size() && StringUtils::isDigit(input[pos])) {
        // Process digit...
        return true;
    }
  
    return false;
}

// Custom string validation
bool IsValidIdentifier(std::string_view name) {
    if (name.empty() || StringUtils::isDigit(name[0])) {
        return false; // Cannot start with digit
    }
  
    for (char c : name) {
        if (!std::isalnum(c) && c != '_') {
            return false;
        }
    }
  
    return true;
}
```

### Text Trimming Operations

#### Basic Trimming Functions

```cpp
std::string trim(std::string textToTrim);
std::string_view trim(std::string_view textToTrim);
std::string_view trim(const char* textToTrim);

std::string trimStart(std::string textToTrim);
std::string_view trimStart(std::string_view textToTrim);
std::string_view trimStart(const char* textToTrim);

std::string trimEnd(std::string textToTrim);
std::string_view trimEnd(std::string_view textToTrim);
std::string_view trimEnd(const char* textToTrim);
```

**Design Features**:

- **Type-aware**: Returns appropriate type based on input (value vs. view)
- **Non-allocating**: `string_view` overloads avoid memory allocation
- **Safe**: Handles empty strings and null pointers gracefully

**Usage Examples**:

```cpp
// Configuration file parsing
void ParseConfigLine(std::string_view line) {
    line = StringUtils::trim(line);
  
    if (line.empty() || line[0] == '#') {
        return; // Skip empty lines and comments
    }
  
    auto equalPos = line.find('=');
    if (equalPos != std::string_view::npos) {
        auto key = StringUtils::trim(line.substr(0, equalPos));
        auto value = StringUtils::trim(line.substr(equalPos + 1));
      
        // Process key-value pair...
    }
}

// User input processing
std::string ProcessUserInput(const std::string& input) {
    auto trimmed = StringUtils::trim(input);
  
    if (trimmed.empty()) {
        SEDX_CORE_WARN_TAG("Input", "Empty input received");
        return "";
    }
  
    return std::string(trimmed);
}

// Log file processing
void ProcessLogEntry(std::string_view entry) {
    // Remove leading/trailing whitespace but preserve internal formatting
    auto cleanEntry = StringUtils::trim(entry);
  
    // Extract timestamp (assumes format: "[TIMESTAMP] MESSAGE")
    if (cleanEntry.starts_with('[')) {
        auto closeBracket = cleanEntry.find(']');
        if (closeBracket != std::string_view::npos) {
            auto timestamp = cleanEntry.substr(1, closeBracket - 1);
            auto message = StringUtils::trimStart(cleanEntry.substr(closeBracket + 1));
          
            // Process timestamp and message...
        }
    }
}
```

#### Advanced Character Removal

```cpp
std::string removeOuterCharacter(std::string text, char outerChar);
std::string removeDoubleQuotes(std::string text);
std::string removeSingleQuotes(std::string text);
std::string addDoubleQuotes(std::string text);
std::string addSingleQuotes(std::string text);
```

**Usage Examples**:

```cpp
// JSON string processing
std::string ProcessJsonString(std::string_view jsonValue) {
    std::string processed(jsonValue);
  
    // Remove surrounding quotes if present
    processed = StringUtils::removeDoubleQuotes(std::move(processed));
  
    // Unescape common sequences
    processed = StringUtils::replace(processed, "\\\"", "\"", "\\n", "\n", "\\t", "\t");
  
    return processed;
}

// SQL query building
std::string BuildSqlQuery(const std::string& tableName, const std::vector<std::string>& values) {
    std::string query = "INSERT INTO " + tableName + " VALUES (";
  
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) query += ", ";
      
        // Add quotes around string values
        query += StringUtils::addSingleQuotes(values[i]);
    }
  
    query += ");";
    return query;
}

// Configuration value processing
std::string ProcessConfigValue(std::string_view rawValue) {
    std::string value(rawValue);
  
    // Handle quoted values
    if ((value.front() == '"' && value.back() == '"') ||
        (value.front() == '\'' && value.back() == '\'')) {
      
        char quote = value.front();
        value = StringUtils::removeOuterCharacter(std::move(value), quote);
    }
  
    return StringUtils::trim(std::move(value));
}
```

### Case Conversion Operations

```cpp
std::string toLowerCase(std::string text);
std::string toUpperCase(std::string text);
```

**Implementation Features**:

- **Locale-independent**: Uses standard ASCII case conversion
- **In-place modification**: Efficiently modifies the input string
- **UTF-8 safe**: Properly handles multi-byte sequences

**Usage Examples**:

```cpp
// File extension checking
bool HasValidImageExtension(const std::string& filename) {
    auto extension = GetExtension(filename);
    extension = StringUtils::toLowerCase(std::move(extension));
  
    static const std::unordered_set<std::string> validExtensions = {
        "png", "jpg", "jpeg", "bmp", "tga", "dds", "ktx"
    };
  
    return validExtensions.contains(extension);
}

// Command processing
void ProcessCommand(std::string_view input) {
    auto command = StringUtils::toLowerCase(std::string(input));
    command = StringUtils::trim(std::move(command));
  
    if (command == "help") {
        ShowHelp();
    } else if (command == "exit" || command == "quit") {
        RequestExit();
    } else if (command.starts_with("load ")) {
        auto filename = command.substr(5);
        LoadFile(StringUtils::trim(filename));
    }
}

// Asset name normalization
std::string NormalizeAssetName(const std::string& assetName) {
    auto normalized = StringUtils::toLowerCase(assetName);
  
    // Replace spaces and special characters with underscores
    normalized = StringUtils::replace(normalized, " ", "_", "-", "_", ".", "_");
  
    // Remove multiple consecutive underscores
    while (normalized.find("__") != std::string::npos) {
        normalized = StringUtils::replace(normalized, "__", "_");
    }
  
    return StringUtils::trim(std::move(normalized), "_");
}
```

### String Splitting Operations

#### Generic Splitting Functions

```cpp
template <typename IsDelimiterChar>
std::vector<std::string> splitString(std::string_view textToSplit, 
                                     IsDelimiterChar&& isDelimiterChar, 
                                     bool includeDelimitersInResult);

std::vector<std::string> splitString(std::string_view textToSplit, 
                                     char delimiterCharacter, 
                                     bool includeDelimitersInResult);

std::vector<std::string> splitAtWhitespace(std::string_view text, 
                                           bool keepDelimiters = false);

std::vector<std::string> splitIntoLines(std::string_view text, 
                                        bool includeNewLinesInResult);
```

**Usage Examples**:

```cpp
// CSV parsing
std::vector<std::string> ParseCsvLine(std::string_view line) {
    auto fields = StringUtils::splitString(line, ',', false);
  
    // Trim and unquote each field
    for (auto& field : fields) {
        field = StringUtils::trim(std::move(field));
        field = StringUtils::removeDoubleQuotes(std::move(field));
    }
  
    return fields;
}

// Path parsing with custom delimiter logic
std::vector<std::string> ParsePath(std::string_view path) {
    // Split on both forward and back slashes
    auto isPathDelimiter = [](char c) { return c == '/' || c == '\\'; };
  
    auto components = StringUtils::splitString(path, isPathDelimiter, false);
  
    // Remove empty components and normalize
    components.erase(
        std::remove_if(components.begin(), components.end(),
                      [](const std::string& s) { return s.empty(); }),
        components.end()
    );
  
    return components;
}

// Shader source processing
void ProcessShaderSource(std::string_view source) {
    auto lines = StringUtils::splitIntoLines(source, false);
  
    for (size_t i = 0; i < lines.size(); ++i) {
        auto line = StringUtils::trim(lines[i]);
      
        if (line.starts_with("#include")) {
            // Process include directive
            auto includeFile = ExtractIncludeFile(line);
            auto includedSource = LoadShaderFile(includeFile);
          
            // Replace include line with actual content
            lines[i] = includedSource;
        }
    }
  
    // Reconstruct shader source
    auto processedSource = StringUtils::joinStrings(lines, "\n");
}

// Command-line argument parsing
std::vector<std::string> ParseCommandLine(std::string_view cmdLine) {
    auto args = StringUtils::splitAtWhitespace(cmdLine, false);
  
    // Handle quoted arguments
    std::vector<std::string> processedArgs;
    std::string currentArg;
    bool inQuotes = false;
  
    for (const auto& arg : args) {
        if (!inQuotes && (arg.front() == '"' || arg.front() == '\'')) {
            inQuotes = true;
            currentArg = arg.substr(1);
          
            if (arg.back() == arg.front()) {
                // Single-word quoted argument
                currentArg.pop_back();
                processedArgs.push_back(std::move(currentArg));
                currentArg.clear();
                inQuotes = false;
            }
        } else if (inQuotes) {
            if (!currentArg.empty()) currentArg += " ";
            currentArg += arg;
          
            if (arg.back() == '"' || arg.back() == '\'') {
                currentArg.pop_back();
                processedArgs.push_back(std::move(currentArg));
                currentArg.clear();
                inQuotes = false;
            }
        } else {
            processedArgs.push_back(arg);
        }
    }
  
    return processedArgs;
}
```

#### String Joining Operations

```cpp
template <typename ArrayOfStrings>
std::string joinStrings(const ArrayOfStrings& strings, std::string_view separator);
```

**Usage Examples**:

```cpp
// Building file paths
std::string BuildPath(const std::vector<std::string>& components) {
    #ifdef SEDX_PLATFORM_WINDOWS
        return StringUtils::joinStrings(components, "\\");
    #else
        return StringUtils::joinStrings(components, "/");
    #endif
}

// SQL query construction
std::string BuildSelectQuery(const std::string& table, 
                           const std::vector<std::string>& columns,
                           const std::vector<std::string>& conditions) {
    std::string query = "SELECT ";
  
    if (columns.empty()) {
        query += "*";
    } else {
        query += StringUtils::joinStrings(columns, ", ");
    }
  
    query += " FROM " + table;
  
    if (!conditions.empty()) {
        query += " WHERE " + StringUtils::joinStrings(conditions, " AND ");
    }
  
    return query;
}

// Log message formatting
std::string FormatLogMessage(const std::vector<std::string>& tags,
                           const std::string& message) {
    std::string formatted = "[";
    formatted += StringUtils::joinStrings(tags, "][");
    formatted += "] " + message;
  
    return formatted;
}
```

### String Search and Comparison

```cpp
bool contains(std::string_view text, std::string_view possibleSubstring);
bool startsWith(std::string_view text, char possibleStart);
bool startsWith(std::string_view text, std::string_view possibleStart);
bool endsWith(std::string_view text, char possibleEnd);
bool endsWith(std::string_view text, std::string_view possibleEnd);
```

**Usage Examples**:

```cpp
// File type detection
enum class FileType { Text, Image, Audio, Video, Unknown };

FileType DetectFileType(const std::string& filename) {
    auto extension = StringUtils::toLowerCase(GetExtension(filename));
  
    if (StringUtils::contains("txt,log,ini,cfg,json,xml", extension)) {
        return FileType::Text;
    } else if (StringUtils::contains("png,jpg,jpeg,bmp,tga", extension)) {
        return FileType::Image;
    } else if (StringUtils::contains("wav,mp3,ogg,flac", extension)) {
        return FileType::Audio;
    } else if (StringUtils::contains("mp4,avi,mkv,mov", extension)) {
        return FileType::Video;
    }
  
    return FileType::Unknown;
}

// URL validation
bool IsValidHttpUrl(std::string_view url) {
    return StringUtils::startsWith(url, "http://") || 
           StringUtils::startsWith(url, "https://");
}

// File extension checking
bool IsShaderFile(const std::string& filename) {
    return StringUtils::endsWith(filename, ".vert") ||
           StringUtils::endsWith(filename, ".frag") ||
           StringUtils::endsWith(filename, ".geom") ||
           StringUtils::endsWith(filename, ".comp") ||
           StringUtils::endsWith(filename, ".glsl");
}

// Content analysis
void AnalyzeTextContent(std::string_view content) {
    if (StringUtils::contains(content, "TODO") || 
        StringUtils::contains(content, "FIXME")) {
        SEDX_CORE_WARN_TAG("CodeAnalysis", "Found TODO/FIXME comments");
    }
  
    if (StringUtils::contains(content, "assert(") ||
        StringUtils::contains(content, "SEDX_ASSERT")) {
        SEDX_CORE_INFO_TAG("CodeAnalysis", "Contains assertion statements");
    }
}
```

### Advanced String Processing

#### Multi-Replace Operations

```cpp
template <typename StringType, typename... OtherReplacements>
std::string replace(StringType textToSearch, 
                   std::string_view firstSubstringToReplace, 
                   std::string_view firstReplacement, 
                   OtherReplacements&&... otherPairsOfStringsToReplace);
```

**Usage Examples**:

```cpp
// HTML entity encoding
std::string EncodeHtmlEntities(const std::string& text) {
    return StringUtils::replace(text,
        "&", "&",
        "<", "<",
        ">", ">",
        "\"", """,
        "'", "'"
    );
}

// Path normalization
std::string NormalizePath(const std::string& path) {
    auto normalized = StringUtils::replace(path,
        "\\", "/",      // Convert backslashes to forward slashes
        "//", "/",      // Remove double slashes
        "/./", "/",     // Remove current directory references
        "/..", "/"      // Simplify parent directory references
    );
  
    return StringUtils::trim(std::move(normalized), "/");
}

// Shader preprocessing
std::string PreprocessShader(const std::string& source) {
    return StringUtils::replace(source,
        "{{VERTEX_BINDING}}", "0",
        "{{MAX_LIGHTS}}", "16",
        "{{SHADOW_MAP_SIZE}}", "2048",
        "{{PI}}", "3.14159265359"
    );
}

// String sanitization for filenames
std::string SanitizeFilename(const std::string& filename) {
    return StringUtils::replace(filename,
        "<", "",
        ">", "",
        ":", "",
        "\"", "",
        "|", "",
        "?", "",
        "*", "",
        "/", "_",
        "\\", "_"
    );
}
```

#### Hex String Operations

```cpp
int hexToInt(uint32_t unicodeChar);

template <typename IntegerType>
std::string createHexString(IntegerType value, int minNumDigits = 0);
```

**Usage Examples**:

```cpp
// Color parsing
struct Color {
    uint8_t r, g, b, a;
  
    static Color FromHexString(std::string_view hexStr) {
        if (hexStr.starts_with("#")) {
            hexStr = hexStr.substr(1);
        }
      
        if (hexStr.length() == 6) {
            // RGB format
            uint32_t rgb = 0;
            for (char c : hexStr) {
                rgb = (rgb << 4) + StringUtils::hexToInt(c);
            }
          
            return Color{
                static_cast<uint8_t>((rgb >> 16) & 0xFF),
                static_cast<uint8_t>((rgb >> 8) & 0xFF),
                static_cast<uint8_t>(rgb & 0xFF),
                255
            };
        }
      
        return Color{0, 0, 0, 255}; // Default to black
    }
  
    std::string ToHexString() const {
        uint32_t rgb = (r << 16) | (g << 8) | b;
        return "#" + StringUtils::createHexString(rgb, 6);
    }
};

// Memory address formatting
std::string FormatPointer(const void* ptr) {
    auto address = reinterpret_cast<uintptr_t>(ptr);
    return "0x" + StringUtils::createHexString(address, sizeof(void*) * 2);
}

// Debug output formatting
void DumpBufferAsHex(const void* data, size_t size) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
  
    for (size_t i = 0; i < size; i += 16) {
        // Address
        std::string line = StringUtils::createHexString(i, 8) + ": ";
      
        // Hex bytes
        for (size_t j = 0; j < 16 && i + j < size; ++j) {
            line += StringUtils::createHexString(bytes[i + j], 2) + " ";
        }
      
        SEDX_CORE_TRACE_TAG("Memory", line);
    }
}
```

### Utility Functions

#### Distance Calculations

```cpp
template <typename StringType>
size_t getLevenshteinDistance(const StringType& string1, const StringType& string2);
```

**Usage Examples**:

```cpp
// Fuzzy string matching for commands
std::string FindBestMatch(const std::string& input, 
                         const std::vector<std::string>& commands) {
    if (commands.empty()) return "";
  
    std::string bestMatch = commands[0];
    size_t bestDistance = StringUtils::getLevenshteinDistance(input, bestMatch);
  
    for (size_t i = 1; i < commands.size(); ++i) {
        size_t distance = StringUtils::getLevenshteinDistance(input, commands[i]);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestMatch = commands[i];
        }
    }
  
    // Only suggest if reasonably close
    if (bestDistance <= input.length() / 2) {
        return bestMatch;
    }
  
    return "";
}

// Asset name suggestions
void SuggestAssetNames(const std::string& partialName,
                      const std::vector<std::string>& availableAssets) {
    std::vector<std::pair<std::string, size_t>> candidates;
  
    for (const auto& asset : availableAssets) {
        if (StringUtils::contains(asset, partialName)) {
            size_t distance = StringUtils::getLevenshteinDistance(partialName, asset);
            candidates.emplace_back(asset, distance);
        }
    }
  
    // Sort by distance (closest first)
    std::sort(candidates.begin(), candidates.end(),
             [](const auto& a, const auto& b) { return a.second < b.second; });
  
    SEDX_CORE_INFO_TAG("AssetManager", "Suggestions for '{}':", partialName);
    for (size_t i = 0; i < std::min(size_t(5), candidates.size()); ++i) {
        SEDX_CORE_INFO_TAG("AssetManager", "  {}", candidates[i].first);
    }
}
```

#### Time and Size Formatting

```cpp
std::string getDurationDescription(std::chrono::duration<double, std::micro> duration);
std::string getByteSizeDescription(uint64_t sizeInBytes);
```

**Usage Examples**:

```cpp
// Performance monitoring
class PerformanceTimer {
private:
    std::chrono::high_resolution_clock::time_point startTime;
    std::string operationName;
  
public:
    PerformanceTimer(std::string_view name) 
        : startTime(std::chrono::high_resolution_clock::now())
        , operationName(name) {}
  
    ~PerformanceTimer() {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            endTime - startTime);
      
        std::string durationStr = StringUtils::getDurationDescription(duration);
        SEDX_CORE_INFO_TAG("Performance", "{} completed in {}", 
                          operationName, durationStr);
    }
};

// Memory usage reporting
void ReportMemoryUsage() {
    auto stats = Allocator::GetAllocationStats();
  
    for (const auto& [category, allocation] : stats) {
        size_t currentUsage = allocation.TotalAllocated - allocation.TotalFreed;
      
        std::string usageStr = StringUtils::getByteSizeDescription(currentUsage);
        std::string totalStr = StringUtils::getByteSizeDescription(allocation.TotalAllocated);
      
        SEDX_CORE_INFO_TAG("Memory", "Category '{}': {} current, {} total allocated",
                          category, usageStr, totalStr);
    }
}

// File size display
void DisplayFileInfo(const std::filesystem::path& filePath) {
    if (std::filesystem::exists(filePath)) {
        auto fileSize = std::filesystem::file_size(filePath);
        auto sizeStr = StringUtils::getByteSizeDescription(fileSize);
      
        auto lastModified = std::filesystem::last_write_time(filePath);
        // Convert to displayable time format...
      
        SEDX_CORE_INFO_TAG("FileSystem", "File: {} ({})", 
                          filePath.filename().string(), sizeStr);
    }
}
```

## String Namespace Reference

### Case Operations

```cpp
namespace SceneryEditorX::String
{
    bool EqualsIgnoreCase(std::string_view a, std::string_view b);
    std::string& ToLower(std::string& string);
    std::string ToLowerCopy(std::string_view string);
    std::string& ToUpper(std::string& string);
    std::string ToUpperCopy(std::string_view string);
    int32_t CompareCase(std::string_view a, std::string_view b);
}
```

**Usage Examples**:

```cpp
// Configuration key comparison
class ConfigManager {
private:
    std::map<std::string, std::string, std::less<>> config;
  
public:
    void SetValue(std::string_view key, std::string_view value) {
        auto normalizedKey = String::ToLowerCopy(key);
        config[normalizedKey] = value;
    }
  
    std::string GetValue(std::string_view key) const {
        auto normalizedKey = String::ToLowerCopy(key);
        auto it = config.find(normalizedKey);
        return it != config.end() ? it->second : "";
    }
  
    bool HasKey(std::string_view key) const {
        auto normalizedKey = String::ToLowerCopy(key);
        return config.contains(normalizedKey);
    }
};

// File extension checking
bool IsImageFile(const std::string& filename) {
    auto extension = GetExtension(filename);
  
    return String::EqualsIgnoreCase(extension, "png") ||
           String::EqualsIgnoreCase(extension, "jpg") ||
           String::EqualsIgnoreCase(extension, "jpeg") ||
           String::EqualsIgnoreCase(extension, "bmp") ||
           String::EqualsIgnoreCase(extension, "tga");
}

// Natural string sorting
bool NaturalStringCompare(std::string_view a, std::string_view b) {
    return String::CompareCase(a, b) < 0;
}
```

### Whitespace Management

```cpp
std::string TrimWhitespace(const std::string& str);
std::string RemoveWhitespace(const std::string& str);
void Erase(std::string& str, const char* chars);
void Erase(std::string& str, const std::string& chars);
```

**Usage Examples**:

```cpp
// Input validation
std::string ValidateUserInput(const std::string& input) {
    auto cleaned = String::TrimWhitespace(input);
  
    if (cleaned.empty()) {
        throw std::invalid_argument("Input cannot be empty");
    }
  
    // Remove any control characters
    String::Erase(cleaned, "\r\n\t\f\v");
  
    return cleaned;
}

// Identifier generation
std::string GenerateIdentifier(const std::string& displayName) {
    auto identifier = displayName;
  
    // Convert to lowercase
    String::ToLower(identifier);
  
    // Remove all whitespace
    identifier = String::RemoveWhitespace(identifier);
  
    // Remove special characters
    String::Erase(identifier, "!@#$%^&*()+={}[]|\\:;\"'<>,.?/");
  
    return identifier;
}

// Code formatting
std::string FormatCodeLine(const std::string& line) {
    auto formatted = String::TrimWhitespace(line);
  
    // Ensure proper spacing around operators
    formatted = StringUtils::replace(formatted,
        "=", " = ",
        "+", " + ",
        "-", " - ",
        "*", " * ",
        "/", " / "
    );
  
    // Clean up multiple spaces
    while (formatted.find("  ") != std::string::npos) {
        formatted = StringUtils::replace(formatted, "  ", " ");
    }
  
    return formatted;
}
```

### String Manipulation

```cpp
std::string SubStr(const std::string& string, size_t offset, size_t count = std::string::npos);
```

**Usage Examples**:

```cpp
// Safe substring extraction
std::string ExtractToken(const std::string& text, size_t start, size_t end) {
    if (start >= text.length()) {
        return "";
    }
  
    size_t length = (end == std::string::npos) ? std::string::npos : end - start;
    return String::SubStr(text, start, length);
}

// File name processing
std::string GetFileNameWithoutPath(const std::string& fullPath) {
    auto lastSlash = fullPath.find_last_of("/\\");
    if (lastSlash == std::string::npos) {
        return fullPath;
    }
  
    return String::SubStr(fullPath, lastSlash + 1);
}
```

### Time Formatting

```cpp
std::string GetCurrentTimeString(bool includeDate = false, bool useDashes = false);
```

**Usage Examples**:

```cpp
// Log file naming
std::string CreateLogFileName() {
    auto timestamp = String::GetCurrentTimeString(true, true);
    return "SceneryEditorX_" + timestamp + ".log";
}

// Timestamped backups
void CreateBackup(const std::string& originalFile) {
    auto extension = GetExtension(originalFile);
    auto baseName = RemoveExtension(originalFile);
    auto timestamp = String::GetCurrentTimeString(true, true);
  
    auto backupName = baseName + "_backup_" + timestamp + "." + extension;
  
    std::filesystem::copy_file(originalFile, backupName);
    SEDX_CORE_INFO_TAG("Backup", "Created backup: {}", backupName);
}

// Performance logging
void LogPerformanceMetric(const std::string& operation, double duration) {
    auto timestamp = String::GetCurrentTimeString(false, false);
    auto message = fmt::format("[{}] {}: {:.3f}ms", timestamp, operation, duration);
  
    // Log to performance file...
}
```

## Global String Functions

### File Path Operations

```cpp
std::string_view GetFilename(std::string_view filepath);
std::string GetExtension(const std::string& filename);
std::string RemoveExtension(const std::string& filename);
```

**Usage Examples**:

```cpp
// Asset loading system
class AssetLoader {
public:
    template<typename T>
    Ref<T> LoadAsset(const std::string& filePath) {
        auto filename = GetFilename(filePath);
        auto extension = GetExtension(std::string(filename));
      
        String::ToLower(extension);
      
        if (extension == "png" || extension == "jpg" || extension == "jpeg") {
            return LoadTexture<T>(filePath);
        } else if (extension == "obj" || extension == "fbx" || extension == "gltf") {
            return LoadModel<T>(filePath);
        } else if (extension == "wav" || extension == "mp3" || extension == "ogg") {
            return LoadAudio<T>(filePath);
        }
      
        SEDX_CORE_ERROR_TAG("AssetLoader", "Unsupported file type: {}", extension);
        return nullptr;
    }
  
private:
    template<typename T>
    Ref<T> LoadTexture(const std::string& filePath) {
        auto baseName = RemoveExtension(std::string(GetFilename(filePath)));
        auto texture = CreateRef<T>();
      
        // Load texture implementation...
        SEDX_CORE_INFO_TAG("AssetLoader", "Loaded texture: {}", baseName);
      
        return texture;
    }
};

// File organization
void OrganizeAssetsByType(const std::vector<std::string>& assetPaths) {
    std::map<std::string, std::vector<std::string>> assetsByType;
  
    for (const auto& path : assetPaths) {
        auto extension = GetExtension(path);
        String::ToLower(extension);
      
        assetsByType[extension].push_back(path);
    }
  
    for (const auto& [type, assets] : assetsByType) {
        SEDX_CORE_INFO_TAG("AssetOrganizer", "Found {} {} files", assets.size(), type);
      
        // Create type-specific directories if needed
        std::filesystem::create_directories("assets/" + type);
      
        for (const auto& asset : assets) {
            auto filename = GetFilename(asset);
            auto destPath = "assets/" + type + "/" + std::string(filename);
          
            if (!std::filesystem::exists(destPath)) {
                std::filesystem::copy_file(asset, destPath);
            }
        }
    }
}
```

### String Splitting and Processing

```cpp
std::vector<std::string> SplitString(std::string_view string, const std::string_view& delimiters);
std::vector<std::string> SplitString(std::string_view string, char delimiter);
std::string SplitAtUpperCase(std::string_view string, std::string_view delimiter = " ", bool ifLowerCaseOnTheRight = true);
```

**Usage Examples**:

```cpp
// Command parsing
struct Command {
    std::string name;
    std::vector<std::string> arguments;
  
    static Command Parse(std::string_view commandLine) {
        auto parts = SplitString(commandLine, ' ');
      
        if (parts.empty()) {
            return Command{};
        }
      
        Command cmd;
        cmd.name = String::ToLowerCopy(parts[0]);
        cmd.arguments.assign(parts.begin() + 1, parts.end());
      
        return cmd;
    }
};

// Class name formatting
std::string FormatClassName(std::string_view className) {
    // Convert "VulkanRenderer" to "Vulkan Renderer"
    return SplitAtUpperCase(className, " ");
}

// URL parsing
struct UrlComponents {
    std::string protocol;
    std::string host;
    std::string path;
    std::map<std::string, std::string> queryParams;
  
    static UrlComponents Parse(std::string_view url) {
        UrlComponents components;
      
        // Split protocol
        auto protocolEnd = url.find("://");
        if (protocolEnd != std::string_view::npos) {
            components.protocol = url.substr(0, protocolEnd);
            url = url.substr(protocolEnd + 3);
        }
      
        // Split host and path
        auto pathStart = url.find('/');
        if (pathStart != std::string_view::npos) {
            components.host = url.substr(0, pathStart);
            components.path = url.substr(pathStart);
        } else {
            components.host = url;
        }
      
        // Parse query parameters
        auto queryStart = components.path.find('?');
        if (queryStart != std::string::npos) {
            auto queryString = components.path.substr(queryStart + 1);
            components.path = components.path.substr(0, queryStart);
          
            auto params = SplitString(queryString, '&');
            for (const auto& param : params) {
                auto keyValue = SplitString(param, '=');
                if (keyValue.size() == 2) {
                    components.queryParams[keyValue[0]] = keyValue[1];
                }
            }
        }
      
        return components;
    }
};
```

### Data Formatting

```cpp
std::string BytesToString(uint64_t bytes);
std::string DurationToString(std::chrono::duration<double> duration);
```

**Usage Examples**:

```cpp
// System monitoring
class SystemMonitor {
public:
    void ReportSystemStatus() {
        // Memory usage
        auto memoryUsage = GetSystemMemoryUsage();
        auto memoryStr = BytesToString(memoryUsage);
        SEDX_CORE_INFO_TAG("System", "Memory usage: {}", memoryStr);
      
        // CPU usage over time
        auto cpuTime = GetCpuTime();
        auto cpuStr = DurationToString(cpuTime);
        SEDX_CORE_INFO_TAG("System", "CPU time: {}", cpuStr);
      
        // Disk usage
        auto diskUsage = GetDiskUsage();
        auto diskStr = BytesToString(diskUsage);
        SEDX_CORE_INFO_TAG("System", "Disk usage: {}", diskStr);
    }
  
private:
    uint64_t GetSystemMemoryUsage() {
        // Implementation to get system memory usage
        return 0;
    }
  
    std::chrono::duration<double> GetCpuTime() {
        // Implementation to get CPU time
        return std::chrono::seconds(0);
    }
  
    uint64_t GetDiskUsage() {
        // Implementation to get disk usage
        return 0;
    }
};

// Progress reporting
class ProgressReporter {
private:
    std::chrono::high_resolution_clock::time_point startTime;
    uint64_t totalBytes;
    uint64_t processedBytes;
  
public:
    ProgressReporter(uint64_t total) 
        : startTime(std::chrono::high_resolution_clock::now())
        , totalBytes(total)
        , processedBytes(0) {}
  
    void Update(uint64_t processed) {
        processedBytes = processed;
      
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsed = currentTime - startTime;
      
        double progress = static_cast<double>(processedBytes) / totalBytes;
        auto estimatedTotal = elapsed / progress;
        auto remaining = estimatedTotal - elapsed;
      
        auto processedStr = BytesToString(processedBytes);
        auto totalStr = BytesToString(totalBytes);
        auto remainingStr = DurationToString(remaining);
      
        SEDX_CORE_INFO_TAG("Progress", "Processed {}/{} ({:.1f}%) - ETA: {}",
                          processedStr, totalStr, progress * 100.0, remainingStr);
    }
};
```

### Template and Type Processing

```cpp
std::string TemplateToParenthesis(std::string_view name);
std::string CreateUserFriendlyTypeName(std::string_view name);
```

**Usage Examples**:

```cpp
// Debug UI for reflection system
template<typename T>
void DisplayTypeInfo() {
    auto typeName = typeid(T).name();
    auto friendlyName = CreateUserFriendlyTypeName(typeName);
  
    ImGui::Text("Type: %s", friendlyName.c_str());
    ImGui::Text("Size: %zu bytes", sizeof(T));
  
    if constexpr (std::is_class_v<T>) {
        ImGui::Text("Class type");
    } else if constexpr (std::is_fundamental_v<T>) {
        ImGui::Text("Fundamental type");
    }
}

// Error message formatting
template<typename T>
void ReportTypeError(const std::string& operation) {
    auto typeName = typeid(T).name();
    auto readableName = CreateUserFriendlyTypeName(typeName);
    auto templateFormatted = TemplateToParenthesis(readableName);
  
    SEDX_CORE_ERROR_TAG("TypeError", "Cannot perform '{}' on type '{}'", 
                        operation, templateFormatted);
}

// Documentation generation
void GenerateTypeDocumentation() {
    std::vector<std::type_info> registeredTypes = GetRegisteredTypes();
  
    for (const auto& typeInfo : registeredTypes) {
        auto typeName = typeInfo.name();
        auto friendlyName = CreateUserFriendlyTypeName(typeName);
        auto formattedName = TemplateToParenthesis(friendlyName);
      
        // Generate documentation entry...
        std::cout << "## " << formattedName << "\n\n";
        std::cout << "Type information and usage examples...\n\n";
    }
}
```

### File I/O Utilities

```cpp
int SkipBOM(std::istream& in);
std::string ReadFileAndSkipBOM(const std::filesystem::path& filepath);
```

**Usage Examples**:

```cpp
// Configuration file loading
class ConfigLoader {
public:
    static std::map<std::string, std::string> LoadConfig(const std::filesystem::path& configPath) {
        std::map<std::string, std::string> config;
      
        auto content = ReadFileAndSkipBOM(configPath);
        if (content.empty()) {
            SEDX_CORE_WARN_TAG("Config", "Failed to read config file: {}", configPath.string());
            return config;
        }
      
        auto lines = StringUtils::splitIntoLines(content, false);
      
        for (const auto& line : lines) {
            auto trimmed = StringUtils::trim(line);
          
            if (trimmed.empty() || trimmed[0] == '#') {
                continue; // Skip empty lines and comments
            }
          
            auto equalPos = trimmed.find('=');
            if (equalPos != std::string::npos) {
                auto key = StringUtils::trim(trimmed.substr(0, equalPos));
                auto value = StringUtils::trim(trimmed.substr(equalPos + 1));
              
                // Remove quotes from value if present
                value = StringUtils::removeDoubleQuotes(std::move(value));
              
                config[std::string(key)] = std::string(value);
            }
        }
      
        SEDX_CORE_INFO_TAG("Config", "Loaded {} configuration entries from {}", 
                          config.size(), configPath.filename().string());
      
        return config;
    }
};

// Shader source loading
std::string LoadShaderSource(const std::filesystem::path& shaderPath) {
    auto source = ReadFileAndSkipBOM(shaderPath);
  
    if (source.empty()) {
        SEDX_CORE_ERROR_TAG("Shader", "Failed to load shader: {}", shaderPath.string());
        return "";
    }
  
    // Process #include directives
    auto lines = StringUtils::splitIntoLines(source, false);
    std::string processedSource;
  
    for (const auto& line : lines) {
        auto trimmed = StringUtils::trim(line);
      
        if (trimmed.starts_with("#include")) {
            auto includeFile = ExtractIncludeFileName(trimmed);
            auto includePath = shaderPath.parent_path() / includeFile;
          
            if (std::filesystem::exists(includePath)) {
                auto includeSource = LoadShaderSource(includePath); // Recursive
                processedSource += includeSource + "\n";
            } else {
                SEDX_CORE_WARN_TAG("Shader", "Include file not found: {}", includePath.string());
                processedSource += line + "\n"; // Keep original line
            }
        } else {
            processedSource += line + "\n";
        }
    }
  
    return processedSource;
}

// Text file analysis
void AnalyzeTextFile(const std::filesystem::path& filePath) {
    auto content = ReadFileAndSkipBOM(filePath);
  
    if (content.empty()) {
        SEDX_CORE_WARN_TAG("FileAnalysis", "Could not read file: {}", filePath.string());
        return;
    }
  
    auto lines = StringUtils::splitIntoLines(content, false);
  
    size_t totalLines = lines.size();
    size_t emptyLines = 0;
    size_t commentLines = 0;
    size_t codeLines = 0;
  
    for (const auto& line : lines) {
        auto trimmed = StringUtils::trim(line);
      
        if (trimmed.empty()) {
            emptyLines++;
        } else if (trimmed.starts_with("//") || trimmed.starts_with("#")) {
            commentLines++;
        } else {
            codeLines++;
        }
    }
  
    SEDX_CORE_INFO_TAG("FileAnalysis", "File: {}", filePath.filename().string());
    SEDX_CORE_INFO_TAG("FileAnalysis", "  Total lines: {}", totalLines);
    SEDX_CORE_INFO_TAG("FileAnalysis", "  Code lines: {}", codeLines);
    SEDX_CORE_INFO_TAG("FileAnalysis", "  Comment lines: {}", commentLines);
    SEDX_CORE_INFO_TAG("FileAnalysis", "  Empty lines: {}", emptyLines);
}
```

## Best Practices

### 1. Choose the Right Function for the Job

```cpp
// ✅ Good: Use string_view for read-only operations
void ProcessFilename(std::string_view filename) {
    auto extension = GetExtension(std::string(filename)); // Convert only when needed
    // Process extension...
}

// ❌ Avoid: Unnecessary string copying
void ProcessFilename(const std::string& filename) {
    auto extension = GetExtension(filename); // Creates unnecessary copy
    // Process extension...
}

// ✅ Good: Use appropriate trim function
std::string_view ProcessLine(std::string_view line) {
    return StringUtils::trim(line); // Returns string_view - no allocation
}

std::string ProcessUserInput(std::string input) {
    return StringUtils::trim(std::move(input)); // Modifies in-place
}
```

### 2. Leverage Template Functions for Flexibility

```cpp
// ✅ Good: Use multi-replace for complex transformations
std::string SanitizeInput(const std::string& input) {
    return StringUtils::replace(input,
        "&", "&",
        "<", "<",
        ">", ">",
        "\"", """
    );
}

// ❌ Avoid: Multiple separate replace calls
std::string SanitizeInputSlow(std::string input) {
    input = StringUtils::replace(input, "&", "&");
    input = StringUtils::replace(input, "<", "<");
    input = StringUtils::replace(input, ">", ">");
    input = StringUtils::replace(input, "\"", """);
    return input;
}
```

### 3. Use Consistent Error Handling

```cpp
// ✅ Good: Consistent error handling with logging
std::string LoadConfigValue(const std::string& key) {
    auto configPath = GetConfigPath();
  
    if (!std::filesystem::exists(configPath)) {
        SEDX_CORE_WARN_TAG("Config", "Config file not found: {}", configPath.string());
        return "";
    }
  
    auto content = ReadFileAndSkipBOM(configPath);
    if (content.empty()) {
        SEDX_CORE_ERROR_TAG("Config", "Failed to read config file");
        return "";
    }
  
    // Parse configuration...
    return "";
}
```

### 4. Optimize for Common Use Cases

```cpp
// ✅ Good: Cache expensive operations
class PathProcessor {
private:
    mutable std::unordered_map<std::string, std::string> extensionCache;
  
public:
    std::string GetCachedExtension(const std::string& filename) const {
        auto it = extensionCache.find(filename);
        if (it != extensionCache.end()) {
            return it->second;
        }
      
        auto extension = String::ToLowerCopy(GetExtension(filename));
        extensionCache[filename] = extension;
        return extension;
    }
};

// ✅ Good: Use appropriate data structures
class CommandRegistry {
private:
    std::unordered_map<std::string, std::function<void()>> commands;
  
public:
    void RegisterCommand(std::string_view name, std::function<void()> handler) {
        auto normalizedName = String::ToLowerCopy(name);
        commands[normalizedName] = std::move(handler);
    }
  
    bool ExecuteCommand(std::string_view name) {
        auto normalizedName = String::ToLowerCopy(name);
        auto it = commands.find(normalizedName);
      
        if (it != commands.end()) {
            it->second();
            return true;
        }
      
        return false;
    }
};
```

## Performance Considerations

### 1. Memory Allocation Minimization

```cpp
// ✅ Good: Minimize allocations with string_view
void ProcessLogLines(std::string_view logContent) {
    auto lines = StringUtils::splitIntoLines(logContent, false);
  
    for (const auto& line : lines) {
        auto trimmed = StringUtils::trim(line); // string_view - no allocation
      
        if (!trimmed.empty() && trimmed[0] != '#') {
            ProcessLogLine(trimmed);
        }
    }
}

// ❌ Avoid: Excessive string copying
void ProcessLogLinesSlow(const std::string& logContent) {
    auto lines = StringUtils::splitIntoLines(logContent, false);
  
    for (auto line : lines) { // Copy!
        line = StringUtils::trim(std::move(line)); // Unnecessary move
      
        if (!line.empty() && line[0] != '#') {
            ProcessLogLine(line);
        }
    }
}
```

### 2. Efficient String Operations

```cpp
// ✅ Good: Reserve space for large string operations
std::string BuildLargeString(const std::vector<std::string>& parts) {
    size_t totalSize = 0;
    for (const auto& part : parts) {
        totalSize += part.size();
    }
  
    std::string result;
    result.reserve(totalSize + parts.size()); // Include separators
  
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) result += " ";
        result += parts[i];
    }
  
    return result;
}

// ✅ Good: Use move semantics appropriately
std::string ProcessString(std::string input) {
    input = String::TrimWhitespace(std::move(input));
    input = StringUtils::toLowerCase(std::move(input));
    String::Erase(input, "!@#$%^&*()");
  
    return input; // Automatic move
}
```

### 3. Algorithm Selection

```cpp
// ✅ Good: Choose efficient algorithms for the use case
class FastStringMatcher {
private:
    std::unordered_set<std::string> exactMatches;
    std::vector<std::string> prefixMatches;
  
public:
    void AddExactMatch(const std::string& str) {
        exactMatches.insert(String::ToLowerCopy(str));
    }
  
    void AddPrefixMatch(const std::string& str) {
        prefixMatches.push_back(String::ToLowerCopy(str));
    }
  
    bool IsMatch(std::string_view input) const {
        auto normalizedInput = String::ToLowerCopy(input);
      
        // O(1) exact match check
        if (exactMatches.contains(normalizedInput)) {
            return true;
        }
      
        // O(n) prefix match check (only if exact fails)
        for (const auto& prefix : prefixMatches) {
            if (StringUtils::startsWith(normalizedInput, prefix)) {
                return true;
            }
        }
      
        return false;
    }
};
```

## Integration Guidelines

### 1. Module Integration Patterns

```cpp
// ✅ Good: Integrate string utilities into modules
class FileManagerModule : public Module {
private:
    std::unordered_map<std::string, std::string> fileExtensionMap;
  
public:
    void OnAttach() override {
        SEDX_CORE_INFO_TAG("FileManager", "Initializing file manager");
      
        // Build extension mapping
        BuildExtensionMap();
    }
  
    bool IsValidAssetFile(const std::string& filename) const {
        auto extension = String::ToLowerCopy(GetExtension(filename));
        return fileExtensionMap.contains(extension);
    }
  
private:
    void BuildExtensionMap() {
        std::vector<std::string> validExtensions = {
            "png", "jpg", "jpeg", "bmp", "tga",     // Images
            "obj", "fbx", "gltf", "dae",            // Models
            "wav", "mp3", "ogg", "flac",            // Audio
            "vert", "frag", "geom", "comp"          // Shaders
        };
      
        for (const auto& ext : validExtensions) {
            fileExtensionMap[ext] = DetermineFileType(ext);
        }
      
        SEDX_CORE_INFO_TAG("FileManager", "Registered {} file extensions", 
                          fileExtensionMap.size());
    }
  
    std::string DetermineFileType(const std::string& extension) const {
        if (StringUtils::contains("png,jpg,jpeg,bmp,tga", extension)) {
            return "Image";
        } else if (StringUtils::contains("obj,fbx,gltf,dae", extension)) {
            return "Model";
        } else if (StringUtils::contains("wav,mp3,ogg,flac", extension)) {
            return "Audio";
        } else if (StringUtils::contains("vert,frag,geom,comp", extension)) {
            return "Shader";
        }
      
        return "Unknown";
    }
};
```

### 2. Configuration System Integration

```cpp
// ✅ Good: Use string utilities in configuration management
class ConfigurationModule : public Module {
private:
    std::map<std::string, std::string> settings;
    std::filesystem::path configPath;
  
public:
    void OnAttach() override {
        configPath = GetConfigDirectory() / "settings.cfg";
        LoadConfiguration();
    }
  
    void OnDetach() override {
        SaveConfiguration();
    }
  
    template<typename T>
    T GetSetting(std::string_view key, const T& defaultValue) const {
        auto normalizedKey = String::ToLowerCopy(key);
        auto it = settings.find(normalizedKey);
      
        if (it == settings.end()) {
            return defaultValue;
        }
      
        return ParseValue<T>(it->second);
    }
  
    template<typename T>
    void SetSetting(std::string_view key, const T& value) {
        auto normalizedKey = String::ToLowerCopy(key);
        settings[normalizedKey] = FormatValue(value);
    }
  
private:
    void LoadConfiguration() {
        if (!std::filesystem::exists(configPath)) {
            SEDX_CORE_WARN_TAG("Config", "Configuration file not found, using defaults");
            return;
        }
      
        auto content = ReadFileAndSkipBOM(configPath);
        auto lines = StringUtils::splitIntoLines(content, false);
      
        for (const auto& line : lines) {
            ParseConfigLine(line);
        }
      
        SEDX_CORE_INFO_TAG("Config", "Loaded {} settings from configuration", 
                          settings.size());
    }
  
    void ParseConfigLine(std::string_view line) {
        line = StringUtils::trim(line);
      
        if (line.empty() || line[0] == '#') {
            return; // Skip comments and empty lines
        }
      
        auto equalPos = line.find('=');
        if (equalPos == std::string_view::npos) {
            return; // Invalid format
        }
      
        auto key = StringUtils::trim(line.substr(0, equalPos));
        auto value = StringUtils::trim(line.substr(equalPos + 1));
      
        // Remove quotes if present
        if ((value.front() == '"' && value.back() == '"') ||
            (value.front() == '\'' && value.back() == '\'')) {
            value = value.substr(1, value.length() - 2);
        }
      
        auto normalizedKey = String::ToLowerCopy(key);
        settings[normalizedKey] = std::string(value);
    }
  
    template<typename T>
    T ParseValue(const std::string& str) const {
        if constexpr (std::is_same_v<T, bool>) {
            auto lower = String::ToLowerCopy(str);
            return lower == "true" || lower == "1" || lower == "yes";
        } else if constexpr (std::is_integral_v<T>) {
            return static_cast<T>(std::stoll(str));
        } else if constexpr (std::is_floating_point_v<T>) {
            return static_cast<T>(std::stod(str));
        } else {
            return T(str);
        }
    }
  
    template<typename T>
    std::string FormatValue(const T& value) const {
        if constexpr (std::is_same_v<T, bool>) {
            return value ? "true" : "false";
        } else if constexpr (std::is_arithmetic_v<T>) {
            return std::to_string(value);
        } else {
            return std::string(value);
        }
    }
};
```

This comprehensive documentation provides detailed coverage of the string utilities system, including practical examples and integration patterns that follow the Scenery Editor X architecture principles. The documentation is designed to serve as a reference for both human developers and AI coding assistants to maintain consistency and best practices when working with string operations in the codebase.

# String.cpp Implementation Guide

---

## Overview

The `string.cpp` file provides the implementation of high-level string processing functions defined in `string.h`. This implementation guide covers the internal workings, algorithms, design decisions, and implementation patterns used throughout the string processing system.

This documentation serves as a comprehensive reference for developers and AI coding assistants to understand how the string utilities are implemented and how to extend or modify them while maintaining consistency with the existing architecture.

## Table of Contents

1. [Implementation Architecture](#implementation-architecture)
2. [Algorithm Implementations](#algorithm-implementations)
3. [Memory Management Patterns](#memory-management-patterns)
4. [Performance Optimizations](#performance-optimizations)
5. [Platform-Specific Code](#platform-specific-code)
6. [Error Handling Strategies](#error-handling-strategies)
7. [Extension Guidelines](#extension-guidelines)
8. [Testing Considerations](#testing-considerations)

## Implementation Architecture

### Design Principles

The `string.cpp` implementation follows several key design principles that align with the Scenery Editor X architecture:

1. **Performance First**: Optimized algorithms with minimal memory allocation
2. **Safety**: Null-safe operations and bounds checking
3. **Modern C++**: Leverages C++17/C++20 features (ranges, string_view)
4. **Cross-Platform**: Handles platform differences transparently
5. **Memory Efficiency**: Uses move semantics and in-place operations where possible

### Header Dependencies

```cpp
#include <algorithm>        // For std::ranges algorithms
#include <fstream>          // For file I/O operations  
#include <regex>            // For complex string parsing
#include <SceneryEditorX/utils/string.h>       // Public interface
#include <SceneryEditorX/utils/string_utils.h> // Low-level utilities
#include <utility>          // For std::move, std::forward
```

### Namespace Organization

```cpp
namespace SceneryEditorX
{
    namespace String
    {
        // High-level string operations
        // Case conversion, whitespace management, etc.
    }

    // Global utility functions
    // File path processing, splitting, formatting
}
```

## Algorithm Implementations

### Case Conversion Algorithms

#### `EqualsIgnoreCase` Implementation

```cpp
bool EqualsIgnoreCase(const std::string_view a, const std::string_view b)
{
    if (a.size() != b.size())
        return false;

    return std::ranges::equal(a, b, [](const char a, const char b)
    {
        return std::tolower(a) == std::tolower(b);
    });
}
```

**Design Decisions**:

- **Early size check**: Eliminates unnecessary character comparisons
- **std::ranges::equal**: Modern C++20 algorithm with custom comparator
- **Lambda comparator**: Inline case-insensitive comparison for performance
- **std::tolower**: Standard library function (locale-independent for ASCII)

**Performance Characteristics**:

- **Time Complexity**: O(n) where n is string length
- **Space Complexity**: O(1) - no allocations
- **Early termination**: Returns false immediately on size mismatch

**Alternative Implementations Considered**:

```cpp
// ❌ Slower: Creates temporary strings
bool EqualsSlow(std::string_view a, std::string_view b) {
    return String::ToLowerCopy(a) == String::ToLowerCopy(b);
}

// ❌ Locale-dependent behavior
bool EqualsLocale(std::string_view a, std::string_view b) {
    return std::equal(a.begin(), a.end(), b.begin(), b.end(),
                     [](char a, char b) { return std::tolower(a, std::locale()) == std::tolower(b, std::locale()); });
}
```

#### `ToLower` and `ToUpper` Implementations

```cpp
std::string& ToLower(std::string& string)
{
    std::ranges::transform(string, string.begin(), [](const unsigned char c) { 
        return std::tolower(c); 
    });
    return string;
}

std::string& ToUpper(std::string& string)
{
    std::ranges::transform(string, string.begin(), [](const unsigned char c) { 
        return std::toupper(c); 
    });
    return string;
}
```

**Implementation Details**:

- **In-place transformation**: Modifies original string (memory efficient)
- **std::ranges::transform**: Modern C++20 algorithm
- **unsigned char cast**: Prevents undefined behavior with signed char
- **Return reference**: Enables method chaining

**Copy Variants**:

```cpp
std::string ToLowerCopy(const std::string_view string)
{
    std::string result(string);
    ToLower(result);
    return result;
}
```

**Design Pattern**: Copy functions delegate to in-place versions for code reuse

### Character Removal Algorithms

#### `Erase` Implementation

```cpp
void Erase(std::string& str, const char* chars)
{
    for (size_t i = 0; i < strlen(chars); i++)
        str.erase(std::ranges::remove(str, chars[i]).begin(), str.end());
}
```

**Algorithm Analysis**:

- **Multiple passes**: One pass per character to remove
- **std::ranges::remove**: Moves unwanted characters to end
- **str.erase**: Removes moved characters from container

**Performance Considerations**:

- **Time Complexity**: O(n*m) where n=string length, m=characters to remove
- **Space Complexity**: O(1) - in-place operation
- **Cache efficiency**: Modern ranges algorithms are well-optimized

**Alternative Single-Pass Implementation**:

```cpp
void EraseOptimized(std::string& str, const char* chars) {
    auto isCharToRemove = [chars](char c) {
        return std::strchr(chars, c) != nullptr;
    };
  
    str.erase(std::remove_if(str.begin(), str.end(), isCharToRemove), str.end());
}
```

### Whitespace Management

#### `TrimWhitespace` Implementation

```cpp
const std::string WHITESPACE = " \n\r\t\f\v";

std::string TrimWhitespace(const std::string& str)
{
    const size_t start = str.find_first_not_of(WHITESPACE);
    const std::string trimmed = (start == std::string::npos) ? "" : str.substr(start);

    const size_t end = trimmed.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : trimmed.substr(0, end + 1);
}
```

**Algorithm Steps**:

1. **Find first non-whitespace**: Using `find_first_not_of`
2. **Create initial trim**: Substring from first valid character
3. **Find last non-whitespace**: Using `find_last_not_of`
4. **Final trim**: Substring to last valid character

**Edge Cases Handled**:

- **Empty string**: Returns empty string
- **All whitespace**: Returns empty string
- **No whitespace**: Returns original string
- **Only leading**: Trims leading only
- **Only trailing**: Trims trailing only

**Performance Optimization**:

```cpp
// Optimized version using string_view (internal)
std::string_view TrimWhitespaceView(std::string_view str) {
    str.remove_prefix(std::min(str.find_first_not_of(WHITESPACE), str.size()));
    str.remove_suffix(std::min(str.size() - str.find_last_not_of(WHITESPACE) - 1, str.size()));
    return str;
}
```

### Platform-Specific Implementations

#### Case-Insensitive Comparison

```cpp
int32_t CompareCase(std::string_view a, std::string_view b)
{
#ifdef SEDX_PLATFORM_WINDOWS
    return _stricmp(a.data(), b.data());
#else
    return strcasecmp(a.data(), b.data());
#endif
}
```

**Platform Differences**:

- **Windows**: Uses `_stricmp` (Microsoft extension)
- **POSIX**: Uses `strcasecmp` (POSIX standard)
- **Both**: Return same comparison semantics

**Safety Note**: Requires null-terminated strings (guaranteed by string_view.data())

### File Processing Implementations

#### `SplitStringAndKeepDelims` Implementation

```cpp
std::vector<std::string> SplitStringAndKeepDelims(std::string str)
{
    const static std::regex re(R"((^\W|^\w+)|(\w+)|[:()])", std::regex_constants::optimize);

    std::regex_iterator rit(str.begin(), str.end(), re);
    std::regex_iterator<std::string::iterator> rend;
    std::vector<std::string> result;

    while (rit != rend)
    {
        result.emplace_back(rit->str());
        ++rit;
    }
    return result;
}
```

**Regex Pattern Analysis**:

- `(^\W|^\w+)`: Captures initial non-word chars or word sequence
- `(\w+)`: Captures word sequences
- `[:()]`: Captures specific delimiter characters
- `std::regex_constants::optimize`: Compile-time optimization hint

**Use Case**: Shader source processing where delimiters need preservation

#### Generic String Splitting

```cpp
std::vector<std::string> SplitString(const std::string_view string, const std::string_view& delimiters)
{
    size_t first = 0;
    std::vector<std::string> result;
  
    while (first <= string.size())
    {
        const auto second = string.find_first_of(delimiters, first);

        if (first != second)
            result.emplace_back(string.substr(first, second - first));

        if (second == std::string_view::npos)
            break;

        first = second + 1;
    }

    return result;
}
```

**Algorithm Walkthrough**:

1. **Initialize position**: Start at beginning of string
2. **Find next delimiter**: Use `find_first_of` for any delimiter
3. **Extract token**: If non-empty segment found
4. **Advance position**: Move past current delimiter
5. **Repeat**: Until end of string reached

**Edge Case Handling**:

- **Empty tokens**: Consecutive delimiters create empty strings
- **Trailing delimiters**: Don't create trailing empty tokens
- **No delimiters**: Returns single-element vector

### Advanced String Processing

#### `SplitAtUpperCase` Implementation

```cpp
std::string SplitAtUpperCase(std::string_view string, std::string_view delimiter, bool ifLowerCaseOnTheRight /*= true*/)
{
    std::string str(string);
    for (int i = static_cast<int>(string.size()) - 1; i > 0; --i)
    {
        if (const auto rightIsLower = [&] { 
            return std::cmp_less(i, string.size()) && std::islower(str[i + 1]); 
        }; std::isupper(str[i]) && (!ifLowerCaseOnTheRight || rightIsLower()))
        {
            str.insert(i, delimiter);
        }
    }

    return str;
}
```

**Algorithm Details**:

- **Reverse iteration**: Prevents position shifts during insertion
- **Lambda for readability**: `rightIsLower` encapsulates condition
- **Conditional insertion**: Based on `ifLowerCaseOnTheRight` parameter
- **Safe comparison**: Uses `std::cmp_less` for signed/unsigned safety

**Examples of Behavior**:

```cpp
SplitAtUpperCase("XMLHttpRequest", " ", true);  // "XMLHttp Request"
SplitAtUpperCase("XMLHttpRequest", " ", false); // "X M L Http Request"
```

### Data Formatting Implementations

#### `BytesToString` Implementation

```cpp
std::string BytesToString(uint64_t bytes)
{
    constexpr uint64_t GB = 1024 * 1024 * 1024;
    constexpr uint64_t MB = 1024 * 1024;
    constexpr uint64_t KB = 1024;

    char buffer[32 + 1] {};

    if (bytes >= GB)
        snprintf(buffer, sizeof(buffer), "%.2f GB", (float)bytes / (float)GB);
    else if (bytes >= MB)
        snprintf(buffer, sizeof(buffer), "%.2f MB", (float)bytes / (float)MB);
    else if (bytes >= KB)
        snprintf(buffer, sizeof(buffer), "%.2f KB", (float)bytes / (float)KB);
    else
        snprintf(buffer, sizeof(buffer), "%.2f bytes", (float)bytes);

    return std::string(buffer);
}
```

**Implementation Choices**:

- **Constexpr constants**: Compile-time calculation of thresholds
- **Stack buffer**: Fixed-size buffer avoids heap allocation
- **snprintf**: Safe formatted printing with bounds checking
- **Float conversion**: Ensures proper decimal formatting
- **Cascading if-else**: Largest unit first for proper classification

#### Duration Formatting

```cpp
std::string DurationToString(std::chrono::duration<double> duration)
{
    const auto durations = BreakDownDuration<std::chrono::minutes, std::chrono::seconds, std::chrono::milliseconds>(duration);

    std::stringstream durSs;
    durSs << std::setfill('0') << std::setw(1) << std::get<0>(durations).count() << ':'
        << std::setfill('0') << std::setw(2) << std::get<1>(durations).count() << '.'
        << std::setfill('0') << std::setw(3) << std::get<2>(durations).count();
    return durSs.str();
}
```

**Formatting Details**:

- **Template delegation**: Uses `BreakDownDuration` template
- **Stream formatting**: Precise control over output format
- **Zero padding**: Ensures consistent MM:SS.mmm format
- **Component extraction**: Uses `std::get<N>` for tuple access

### Type Name Processing

#### `TemplateToParenthesis` Implementation

```cpp
std::string TemplateToParenthesis(std::string_view name)
{
    std::string str(name);

    if (!StringUtils::contains(name, "<") || !StringUtils::contains(name, ">"))
        return str;

    const auto i = str.find('<');
    if (i > 1 && str[i - 1] != ' ')
        str.insert(i, " ");

    str[i + 2] = std::toupper(str[i + 2]);

    return StringUtils::replace(str, "<", "(", ">", ")");
}
```

**Processing Steps**:

1. **Early exit**: Return unchanged if no templates
2. **Space insertion**: Add space before `<` if needed
3. **Capitalization**: Uppercase first character after opening
4. **Bracket replacement**: Convert `<>` to `()`

**Before/After Examples**:

- `"vector<int>"` → `"vector (Int)"`
- `"map<string,float>"` → `"map (String,float)"`

### File I/O Implementations

#### BOM Detection and Skipping

```cpp
int SkipBOM(std::istream& in)
{
    char test[4] = {};
    in.seekg(0, std::ios::beg);
    in.read(test, 3);
  
    if (strcmp(test, "\xEF\xBB\xBF") == 0)
    {
        in.seekg(3, std::ios::beg);
        return 3;
    }
  
    in.seekg(0, std::ios::beg);
    return 0;
}
```

**Algorithm Steps**:

1. **Read first 3 bytes**: Into null-terminated buffer
2. **Compare with BOM**: UTF-8 BOM is 0xEF 0xBB 0xBF
3. **Position stream**: Skip BOM if found, reset if not
4. **Return count**: Number of bytes skipped

#### Complete File Reading

```cpp
std::string ReadFileAndSkipBOM(const std::filesystem::path& filepath)
{
    std::string result;
    std::ifstream in(filepath, std::ios::in | std::ios::binary);
  
    if (in)
    {
        in.seekg(0, std::ios::end);
        auto fileSize = in.tellg();
        const int skippedChars = SkipBOM(in);

        fileSize -= skippedChars - 1;
        result.resize(fileSize);
        in.read(result.data() + 1, fileSize);
    
        /// Add a dummy tab to beginning of file.
        result[0] = '\t';
    }
  
    in.close();
    return result;
}
```

**Implementation Details**:

- **Binary mode**: Prevents text mode transformations
- **Size calculation**: Gets file size then adjusts for BOM
- **Memory pre-allocation**: Resizes string to exact size needed
- **Dummy character**: Adds tab at beginning (implementation-specific)
- **Automatic cleanup**: RAII ensures file closure

## Memory Management Patterns

### Move Semantics Usage

```cpp
// ✅ Good: Efficient move-based operations
std::string ProcessString(std::string input) {
    input = String::TrimWhitespace(std::move(input));
    String::ToLower(input);  // In-place modification
    return input;  // Automatic move
}

// ✅ Good: Move in utility functions
std::string removeOuterCharacter(std::string text, char outerChar) {
    // Process text...
    return std::move(text);  // Explicit move
}
```

### String View Optimization

```cpp
// ✅ Good: Avoid unnecessary allocations
void ProcessTokens(std::string_view input) {
    auto tokens = SplitString(input, ',');  // Only creates vector of strings
  
    for (const auto& token : tokens) {
        auto trimmed = StringUtils::trim(token);  // Returns string_view
        // Process without allocation...
    }
}
```

### Container Reserve Patterns

```cpp
std::vector<std::string> SplitStringOptimized(std::string_view text, char delimiter) {
    // Estimate token count to reduce reallocations
    size_t estimatedTokens = std::count(text.begin(), text.end(), delimiter) + 1;
  
    std::vector<std::string> result;
    result.reserve(estimatedTokens);
  
    // Splitting algorithm...
    return result;
}
```

## Performance Optimizations

### Algorithm Selection

1. **String Search**: Uses Boyer-Moore when available (`std::string::find`)
2. **Character Classification**: Fast arithmetic checks over function calls
3. **Memory Access**: Prefers sequential access patterns
4. **Branch Prediction**: Structures conditionals for common cases first

### Compile-Time Optimizations

```cpp
// Compile-time string constants
const std::string WHITESPACE = " \n\r\t\f\v";

// Constexpr functions where possible
constexpr bool IsValidChar(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}
```

### Cache-Friendly Patterns

```cpp
// ✅ Good: Sequential access pattern
void ProcessStringData(std::string& str) {
    for (size_t i = 0; i < str.size(); ++i) {
        // Process character at position i
    }
}

// ❌ Avoid: Random access patterns
void ProcessStringDataSlow(std::string& str) {
    for (size_t i = str.size(); i > 0; --i) {
        // Non-sequential access can hurt cache performance
    }
}
```

## Error Handling Strategies

### Defensive Programming

```cpp
std::string SafeSubstring(const std::string& str, size_t pos, size_t len) {
    if (pos >= str.length()) {
        return "";  // Return empty instead of throwing
    }
  
    return str.substr(pos, std::min(len, str.length() - pos));
}
```

### Exception Safety

```cpp
// Strong exception safety guarantee
std::string ProcessStringWithRollback(const std::string& input) {
    std::string result = input;  // Copy for rollback
  
    try {
        // Perform operations on result
        String::ToLower(result);
        result = String::TrimWhitespace(std::move(result));
        // More operations...
    
        return result;  // Commit changes
    }
    catch (...) {
        // result destructor provides automatic cleanup
        throw;  // Re-throw exception
    }
}
```

### Logging Integration

```cpp
std::string LoadConfigFile(const std::filesystem::path& path) {
    SEDX_CORE_TRACE_TAG("StringUtils", "Loading config file: {}", path.string());
  
    try {
        auto content = ReadFileAndSkipBOM(path);
    
        if (content.empty()) {
            SEDX_CORE_WARN_TAG("StringUtils", "Config file is empty: {}", path.string());
            return "";
        }
    
        SEDX_CORE_INFO_TAG("StringUtils", "Loaded {} bytes from config", content.size());
        return content;
    
    } catch (const std::exception& e) {
        SEDX_CORE_ERROR_TAG("StringUtils", "Failed to load config {}: {}", 
                           path.string(), e.what());
        return "";
    }
}
```

## Extension Guidelines

### Adding New String Functions

When adding new string processing functions, follow these patterns:

1. **Header Declaration**: Add to appropriate namespace in `string.h`
2. **Implementation**: Follow existing patterns in `string.cpp`
3. **Documentation**: Include Doxygen comments
4. **Testing**: Add comprehensive test cases
5. **Performance**: Consider both time and space complexity

#### Example Extension

```cpp
// In string.h
namespace SceneryEditorX::String
{
    /// Removes all occurrences of a substring from a string
    std::string RemoveSubstring(const std::string& text, std::string_view toRemove);
  
    /// In-place version for performance
    std::string& RemoveSubstringInPlace(std::string& text, std::string_view toRemove);
}

// In string.cpp
std::string RemoveSubstring(const std::string& text, std::string_view toRemove) 
{
    std::string result = text;
    RemoveSubstringInPlace(result, toRemove);
    return result;
}

std::string& RemoveSubstringInPlace(std::string& text, std::string_view toRemove) 
{
    if (toRemove.empty()) {
        return text;
    }
  
    size_t pos = 0;
    while ((pos = text.find(toRemove, pos)) != std::string::npos) {
        text.erase(pos, toRemove.length());
    }
  
    return text;
}
```

### Unicode Considerations

When extending for Unicode support:

```cpp
// Future Unicode support pattern
namespace SceneryEditorX::String::Unicode
{
    std::u8string ToLowerUtf8(const std::u8string& text);
    bool EqualsIgnoreCaseUtf8(std::u8string_view a, std::u8string_view b);
    size_t LengthUtf8(std::u8string_view text);  // Character count, not byte count
}
```

## Testing Considerations

### Unit Test Patterns

```cpp
// Test all edge cases
TEST_CASE("String::TrimWhitespace handles edge cases", "[string]") {
    REQUIRE(String::TrimWhitespace("") == "");
    REQUIRE(String::TrimWhitespace("   ") == "");
    REQUIRE(String::TrimWhitespace("text") == "text");
    REQUIRE(String::TrimWhitespace("  text  ") == "text");
    REQUIRE(String::TrimWhitespace("\t\ntext\r\n") == "text");
}

// Test performance characteristics
TEST_CASE("String operations performance", "[string][performance]") {
    std::string large_string(1000000, 'a');
  
    auto start = std::chrono::high_resolution_clock::now();
    String::ToLower(large_string);
    auto end = std::chrono::high_resolution_clock::now();
  
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    REQUIRE(duration.count() < 100);  // Should complete in < 100ms
}
```

### Memory Leak Detection

```cpp
TEST_CASE("String operations don't leak memory", "[string][memory]") {
    size_t initial_memory = GetCurrentMemoryUsage();
  
    {
        for (int i = 0; i < 10000; ++i) {
            auto result = String::ToLowerCopy("TEST STRING");
            // result goes out of scope
        }
    }
  
    size_t final_memory = GetCurrentMemoryUsage();
    REQUIRE(final_memory <= initial_memory + ACCEPTABLE_OVERHEAD);
}
```

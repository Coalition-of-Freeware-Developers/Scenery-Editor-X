# StringUtils.h Low-Level Utilities Documentation

---

## Overview

The `string_utils.h` header provides the foundational layer of string processing utilities for the Scenery Editor X application. This file contains template-based, high-performance string manipulation functions that serve as the building blocks for higher-level string operations.

This documentation covers the low-level string utilities, their template implementations, performance characteristics, and usage patterns within the Scenery Editor X architecture.

## Table of Contents

1. [Design Philosophy](#design-philosophy)
2. [Character Classification Functions](#character-classification-functions)
3. [Template-Based Text Processing](#template-based-text-processing)
4. [String Trimming Operations](#string-trimming-operations)
5. [Search and Replace Algorithms](#search-and-replace-algorithms)
6. [String Splitting Templates](#string-splitting-templates)
7. [Type Conversion Utilities](#type-conversion-utilities)
8. [Performance Analysis](#performance-analysis)
9. [Implementation Patterns](#implementation-patterns)
10. [Extension Guidelines](#extension-guidelines)

## Design Philosophy

### Core Principles

The StringUtils namespace embodies several key design principles:

1. **Zero-Cost Abstractions**: Template implementations that compile to optimal code
2. **Type Safety**: Strong typing prevents common string processing errors
3. **Flexibility**: Works with multiple string types (`std::string`, `std::string_view`, `const char*`)
4. **Performance**: Optimized algorithms with minimal memory allocation
5. **Composability**: Functions designed to work together seamlessly

### Template Design Strategy

```cpp
namespace SceneryEditorX::StringUtils
{
    // Generic templates that work with multiple string types
    template <typename StringType, typename... OtherReplacements>
    std::string replace(StringType textToSearch, /*...*/);
  
    // Overloaded functions for specific types
    std::string trim(std::string textToTrim);           // Move semantics
    std::string_view trim(std::string_view textToTrim); // Zero-copy
    std::string_view trim(const char* textToTrim);      // C-string support
}
```

## Character Classification Functions

### Basic Character Tests

#### `inline bool isWhitespace(char c)`

**Purpose**: High-performance whitespace detection optimized for common use cases.

**Implementation**:

```cpp
inline bool isWhitespace(char c) { 
    return c == ' ' || (c <= 13 && c >= 9); 
}
```

**Character Range Coverage**:

- `c == ' '`: Space (ASCII 32)
- `c >= 9 && c <= 13`: Tab(9), LF(10), VT(11), FF(12), CR(13)

**Performance Characteristics**:

- **Branch-free for space**: Most common case handled first
- **Range check optimization**: Single comparison for control characters
- **Inline expansion**: Zero function call overhead

**Usage Examples**:

```cpp
// Parsing whitespace-delimited data
void SkipWhitespace(std::string_view text, size_t& pos) {
    while (pos < text.size() && StringUtils::isWhitespace(text[pos])) {
        ++pos;
    }
}

// Custom string cleaning
std::string RemoveAllWhitespace(const std::string& text) {
    std::string result;
    result.reserve(text.size());
  
    for (char c : text) {
        if (!StringUtils::isWhitespace(c)) {
            result += c;
        }
    }
  
    return result;
}
```

#### `inline bool isDigit(char c)`

**Purpose**: Fast digit detection using arithmetic operations instead of character comparisons.

**Implementation**:

```cpp
inline bool isDigit(char c) { 
    return static_cast<uint32_t>(c - '0') < 10; 
}
```

**Algorithm Explanation**:

- **Arithmetic trick**: Subtracts '0' (ASCII 48) from character
- **Range check**: Valid digits result in 0-9, others produce values ≥ 10
- **Unsigned comparison**: Handles negative results correctly (wrap to large positive)

**Performance Advantages**:

- **Single arithmetic operation**: Faster than `c >= '0' && c <= '9'`
- **No branches**: Compiles to efficient assembly code
- **Cache-friendly**: No memory access required

**Usage Examples**:

```cpp
// Number parsing
bool ParseInteger(std::string_view text, size_t& pos, int& value) {
    if (pos >= text.size() || !StringUtils::isDigit(text[pos])) {
        return false;
    }
  
    value = 0;
    while (pos < text.size() && StringUtils::isDigit(text[pos])) {
        value = value * 10 + (text[pos] - '0');
        ++pos;
    }
  
    return true;
}

// Validation
bool IsNumericString(std::string_view text) {
    return !text.empty() && 
           std::all_of(text.begin(), text.end(), StringUtils::isDigit);
}
```

## Template-Based Text Processing

### Multi-String Replace Template

#### `template <typename StringType, typename... OtherReplacements> std::string replace(...)`

**Purpose**: Efficiently replace multiple substrings in a single pass through template recursion.

**Template Signature**:

```cpp
template <typename StringType, typename... OtherReplacements>
std::string replace(StringType textToSearch, 
                   std::string_view firstSubstringToReplace, 
                   std::string_view firstReplacement, 
                   OtherReplacements&&... otherPairsOfStringsToReplace);
```

**Implementation Strategy**:

```cpp
// Base case: single replacement
if constexpr (sizeof...(otherPairsOfStringsToReplace) == 0)
{
    size_t pos = 0;
    for (;;)
    {
        pos = textToSearch.find(firstToReplace, pos);
        if (pos == std::string::npos)
            return textToSearch;

        textToSearch.replace(pos, firstToReplace.length(), firstReplacement);
        pos += firstReplacement.length();
    }
}
// Recursive case: multiple replacements
else
{
    return replace(replace(std::move(textToSearch), firstToReplace, firstReplacement), 
                   std::forward<OtherReplacements>(otherPairsOfStringsToReplace)...);
}
```

**Template Metaprogramming Features**:

- **Static assertion**: Ensures even number of replacement arguments
- **Perfect forwarding**: Preserves value categories of arguments
- **Conditional compilation**: Different code paths for base/recursive cases
- **Type deduction**: Works with various string types

**Usage Examples**:

```cpp
// HTML entity encoding
std::string SafeHtmlText(const std::string& text) {
    return StringUtils::replace(text,
        "&", "&",    // Must be first to avoid double-encoding
        "<", "<",
        ">", ">",
        "\"", """,
        "'", "'"
    );
}

// Path normalization
std::string NormalizePath(const std::string& path) {
    return StringUtils::replace(path,
        "\\", "/",       // Convert backslashes
        "//", "/",       // Remove double slashes
        "/./", "/"       // Remove current directory references
    );
}

// String cleaning for identifiers
std::string CreateIdentifier(const std::string& displayName) {
    return StringUtils::replace(displayName,
        " ", "_",
        "-", "_",
        ".", "_",
        "(", "",
        ")", ""
    );
}
```

**Performance Characteristics**:

- **Time Complexity**: O(n*r) where n=text length, r=number of replacements
- **Space Complexity**: O(n) for result string
- **Optimization**: Modern compilers can optimize recursive template calls

### Character Processing Templates

#### `template <typename IsDelimiterChar> std::vector<std::string> splitString(...)`

**Purpose**: Generic string splitting using customizable delimiter detection logic.

**Template Signature**:

```cpp
template <typename IsDelimiterChar>
std::vector<std::string> splitString(std::string_view textToSplit, 
                                     IsDelimiterChar&& isDelimiterChar, 
                                     bool includeDelimitersInResult);
```

**Customizable Delimiter Logic**:

```cpp
// Lambda-based delimiter detection
auto isCustomDelimiter = [](char c) {
    return c == ',' || c == ';' || c == '|';
};

auto tokens = StringUtils::splitString(text, isCustomDelimiter, false);

// Function object for complex logic
struct PathDelimiter {
    bool operator()(char c) const {
        return c == '/' || c == '\\';
    }
};

auto pathComponents = StringUtils::splitString(path, PathDelimiter{}, false);

// Predicate for specific contexts
auto isShaderDelimiter = [](char c) {
    return std::isspace(c) || c == '(' || c == ')' || c == '{' || c == '}';
};
```

#### Advanced Delimiter Templates

**Two-Phase Delimiter Detection**:

```cpp
template <typename CharStartsDelimiter, typename CharIsInDelimiterBody>
std::vector<std::string> splitString(std::string_view textToSplit, 
                                     CharStartsDelimiter&& isDelimiterStart, 
                                     CharIsInDelimiterBody&& isDelimiterBody, 
                                     bool includeDelimitersInResult);
```

**Use Cases**:

```cpp
// String literal parsing (handles quoted strings)
auto isQuoteStart = [](char c) { return c == '"' || c == '\''; };
auto isInQuote = [quote = '\0'](char c) mutable {
    if (quote == '\0' && (c == '"' || c == '\'')) {
        quote = c;
        return true;
    }
    if (c == quote) {
        quote = '\0';
        return true;
    }
    return quote != '\0';
};

// Comment parsing (handles /* */ blocks)
auto isCommentStart = [](char c) { return c == '/'; };
auto isInComment = [inComment = false](char c) mutable {
    // Complex comment detection logic...
    return inComment;
};
```

## String Trimming Operations

### Type-Aware Trimming Functions

#### Return Type Optimization

```cpp
// Returns std::string (moves input)
std::string trim(std::string textToTrim);

// Returns std::string_view (zero-copy)
std::string_view trim(std::string_view textToTrim);

// Returns std::string_view (converts and trims)
std::string_view trim(const char* textToTrim);
```

**Design Pattern**: Return type matches input lifetime

- **String input**: Can safely move and modify
- **String view input**: Can return view of original data
- **C-string input**: Convert to view, then trim

#### Implementation Details

```cpp
// Efficient composition pattern
inline std::string trim(std::string text) { 
    return trimStart(trimEnd(std::move(text))); 
}

inline std::string_view trim(std::string_view text) { 
    return trimStart(trimEnd(text)); 
}

// Delegation to avoid code duplication
inline std::string_view trim(const char* text) { 
    return trim(std::string_view(text)); 
}
```

**Performance Benefits**:

- **No redundant allocations**: String views avoid memory allocation
- **Move semantics**: String inputs use efficient move operations
- **Function composition**: Reuses existing trimStart/trimEnd logic

### Character Removal Functions

#### `removeOuterCharacter` Implementation

```cpp
std::string removeOuterCharacter(std::string text, char outerChar);
```

**Usage Examples**:

```cpp
// JSON string processing
std::string ProcessJsonValue(std::string_view jsonValue) {
    std::string value(jsonValue);
  
    // Remove surrounding quotes
    value = StringUtils::removeDoubleQuotes(std::move(value));
  
    // Unescape JSON sequences
    value = StringUtils::replace(std::move(value),
        "\\\"", "\"",
        "\\n", "\n",
        "\\t", "\t",
        "\\\\", "\\"
    );
  
    return value;
}

// Configuration value parsing
std::string ParseConfigValue(std::string_view rawValue) {
    std::string value(rawValue);
  
    // Handle various quote styles
    if (value.size() >= 2) {
        if ((value.front() == '"' && value.back() == '"') ||
            (value.front() == '\'' && value.back() == '\'')) {
            value = StringUtils::removeOuterCharacter(std::move(value), value.front());
        }
    }
  
    return StringUtils::trim(std::move(value));
}
```

#### Convenience Functions

```cpp
inline std::string removeDoubleQuotes(std::string text) { 
    return removeOuterCharacter(std::move(text), '"'); 
}

inline std::string removeSingleQuotes(std::string text) { 
    return removeOuterCharacter(std::move(text), '\''); 
}

inline std::string addDoubleQuotes(std::string text) { 
    return "\"" + std::move(text) + "\""; 
}

inline std::string addSingleQuotes(std::string text) { 
    return "'" + std::move(text) + "'"; 
}
```

## Search and Replace Algorithms

### String Search Functions

#### Efficient String Matching

```cpp
bool contains(std::string_view text, std::string_view possibleSubstring);
bool startsWith(std::string_view text, char possibleStart);
bool startsWith(std::string_view text, std::string_view possibleStart);
bool endsWith(std::string_view text, char possibleEnd);
bool endsWith(std::string_view text, std::string_view possibleEnd);
```

**Implementation Strategy**:

- **Character overloads**: Optimized single-character checks
- **String view parameters**: Avoid unnecessary string copies
- **Short-circuit evaluation**: Early exit for impossible matches

**Performance Optimizations**:

```cpp
// Character version (most efficient)
inline bool startsWith(std::string_view text, char c) {
    return !text.empty() && text[0] == c;
}

// String version with length optimization
inline bool startsWith(std::string_view text, std::string_view prefix) {
    return text.length() >= prefix.length() && 
           text.substr(0, prefix.length()) == prefix;
}
```

### Distance Calculations

#### `getLevenshteinDistance` Template

```cpp
template <typename StringType>
size_t getLevenshteinDistance(const StringType& string1, const StringType& string2);
```

**Algorithm Implementation**:

```cpp
template <typename StringType>
size_t getLevenshteinDistance(const StringType& string1, const StringType& string2) {
    const size_t len1 = string1.size();
    const size_t len2 = string2.size();
  
    // Optimization: handle empty strings
    if (len1 == 0) return len2;
    if (len2 == 0) return len1;
  
    // Dynamic programming matrix
    std::vector<std::vector<size_t>> matrix(len1 + 1, std::vector<size_t>(len2 + 1));
  
    // Initialize base cases
    for (size_t i = 0; i <= len1; ++i) matrix[i][0] = i;
    for (size_t j = 0; j <= len2; ++j) matrix[0][j] = j;
  
    // Fill matrix using recurrence relation
    for (size_t i = 1; i <= len1; ++i) {
        for (size_t j = 1; j <= len2; ++j) {
            size_t cost = (string1[i-1] == string2[j-1]) ? 0 : 1;
        
            matrix[i][j] = std::min({
                matrix[i-1][j] + 1,      // Deletion
                matrix[i][j-1] + 1,      // Insertion  
                matrix[i-1][j-1] + cost  // Substitution
            });
        }
    }
  
    return matrix[len1][len2];
}
```

**Usage Examples**:

```cpp
// Fuzzy command matching
std::string FindBestCommandMatch(std::string_view userInput, 
                                const std::vector<std::string>& availableCommands) {
    if (availableCommands.empty()) return "";
  
    std::string bestMatch = availableCommands[0];
    size_t bestDistance = StringUtils::getLevenshteinDistance(userInput, bestMatch);
  
    for (size_t i = 1; i < availableCommands.size(); ++i) {
        size_t distance = StringUtils::getLevenshteinDistance(userInput, availableCommands[i]);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestMatch = availableCommands[i];
        }
    }
  
    // Only suggest if reasonably similar
    if (bestDistance <= userInput.length() / 2) {
        return bestMatch;
    }
  
    return "";
}

// Spell checking
bool IsSpellingMistake(std::string_view word, const std::set<std::string>& dictionary) {
    // Check exact match first
    if (dictionary.contains(std::string(word))) {
        return false;
    }
  
    // Check for close matches (edit distance ≤ 2)
    for (const auto& dictWord : dictionary) {
        if (StringUtils::getLevenshteinDistance(word, dictWord) <= 2) {
            return false; // Close enough to be considered correct
        }
    }
  
    return true; // Likely misspelled
}
```

## String Splitting Templates

### Advanced Splitting Functions

#### `joinStrings` Template

```cpp
template <typename ArrayOfStrings>
std::string joinStrings(const ArrayOfStrings& strings, std::string_view separator);
```

**Template Requirements**:

- **Container concept**: Must support range-based for loops
- **Element type**: Elements must be convertible to string
- **Iterator support**: Begin/end iterators required

**Implementation Pattern**:

```cpp
template <typename ArrayOfStrings>
std::string joinStrings(const ArrayOfStrings& strings, std::string_view separator) {
    if (strings.empty()) {
        return "";
    }
  
    // Calculate total size for efficient allocation
    size_t totalSize = 0;
    size_t count = 0;
  
    for (const auto& str : strings) {
        totalSize += str.size();
        ++count;
    }
  
    if (count > 1) {
        totalSize += separator.size() * (count - 1);
    }
  
    // Pre-allocate result string
    std::string result;
    result.reserve(totalSize);
  
    // Join with separators
    bool first = true;
    for (const auto& str : strings) {
        if (!first) {
            result += separator;
        }
        result += str;
        first = false;
    }
  
    return result;
}
```

**Usage Examples**:

```cpp
// Build file paths
std::string BuildPath(const std::vector<std::string>& components) {
    #ifdef SEDX_PLATFORM_WINDOWS
        return StringUtils::joinStrings(components, "\\");
    #else
        return StringUtils::joinStrings(components, "/");
    #endif
}

// SQL query construction
std::string BuildSelectQuery(const std::vector<std::string>& columns,
                           const std::string& tableName) {
    std::string query = "SELECT ";
  
    if (columns.empty()) {
        query += "*";
    } else {
        query += StringUtils::joinStrings(columns, ", ");
    }
  
    query += " FROM " + tableName;
    return query;
}

// CSV generation
std::string CreateCsvLine(const std::vector<std::string>& values) {
    // Escape and quote values as needed
    std::vector<std::string> quotedValues;
    quotedValues.reserve(values.size());
  
    for (const auto& value : values) {
        if (value.find(',') != std::string::npos || 
            value.find('"') != std::string::npos ||
            value.find('\n') != std::string::npos) {
            // Escape quotes and wrap in quotes
            auto escaped = StringUtils::replace(value, "\"", "\"\"");
            quotedValues.push_back(StringUtils::addDoubleQuotes(std::move(escaped)));
        } else {
            quotedValues.push_back(value);
        }
    }
  
    return StringUtils::joinStrings(quotedValues, ",");
}
```

### Specialized Splitting Functions

#### `splitIntoLines` Implementation

```cpp
std::vector<std::string> splitIntoLines(std::string_view text, bool includeNewLinesInResult);
```

**Cross-Platform Line Ending Handling**:

```cpp
std::vector<std::string> splitIntoLines(std::string_view text, bool includeNewLines) {
    std::vector<std::string> lines;
    size_t start = 0;
  
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\n') {
            // Handle \r\n and \n line endings
            size_t end = i;
            if (i > 0 && text[i-1] == '\r') {
                --end; // Don't include \r in line content
            }
        
            if (includeNewLines) {
                lines.emplace_back(text.substr(start, i - start + 1));
            } else {
                lines.emplace_back(text.substr(start, end - start));
            }
        
            start = i + 1;
        }
    }
  
    // Handle last line (may not end with newline)
    if (start < text.size()) {
        lines.emplace_back(text.substr(start));
    }
  
    return lines;
}
```

## Type Conversion Utilities

### Hexadecimal Processing

#### `hexToInt` Function

```cpp
inline int hexToInt(uint32_t unicodeChar)
{
    auto d1 = unicodeChar - static_cast<uint32_t>('0'); 
    if (d1 < 10u) return static_cast<int>(d1);
  
    auto d2 = d1 + static_cast<uint32_t>('0' - 'a'); 
    if (d2 < 6u) return static_cast<int>(d2 + 10);
  
    auto d3 = d2 + static_cast<uint32_t>('a' - 'A'); 
    if (d3 < 6u) return static_cast<int>(d3 + 10);
  
    return -1;
}
```

**Algorithm Explanation**:

- **Digit check**: '0'-'9' maps to 0-9
- **Lowercase hex**: 'a'-'f' maps to 10-15
- **Uppercase hex**: 'A'-'F' maps to 10-15
- **Invalid input**: Returns -1 for non-hex characters

#### `createHexString` Template

```cpp
template <typename IntegerType>
std::string createHexString(IntegerType value, int minNumDigits = 0)
{
    static_assert(std::is_integral_v<IntegerType>, "Need to pass integers into this method");
  
    auto unsignedValue = static_cast<std::make_unsigned_t<IntegerType>>(value);
    assert(minNumDigits <= 32);

    char hex[40];
    const auto end = hex + sizeof(hex) - 1;
    auto d = end;
    *d = 0;

    for (;;)
    {
        *--d = "0123456789abcdef"[static_cast<uint32_t>(unsignedValue) & 15u];
        unsignedValue = static_cast<decltype(unsignedValue)>(unsignedValue >> 4);
        --minNumDigits;

        if (unsignedValue == 0 && minNumDigits <= 0)
            return std::string(d, end);
    }
}
```

**Implementation Features**:

- **Template specialization**: Works with any integer type
- **Unsigned conversion**: Handles signed integers correctly
- **Reverse generation**: Builds hex string from right to left
- **Minimum digits**: Zero-pads to specified width
- **Lookup table**: Uses character array for fast digit conversion

### Duration Formatting

#### `getDurationDescription` Implementation

```cpp
std::string getDurationDescription(std::chrono::duration<double, std::micro> duration);
```

**Adaptive Time Unit Selection**:

```cpp
std::string getDurationDescription(std::chrono::duration<double, std::micro> duration) {
    auto micros = duration.count();
  
    if (micros >= 3600000000.0) {  // >= 1 hour
        auto hours = micros / 3600000000.0;
        return fmt::format("{:.2f} hours", hours);
    } else if (micros >= 60000000.0) {  // >= 1 minute
        auto minutes = micros / 60000000.0;
        return fmt::format("{:.2f} minutes", minutes);
    } else if (micros >= 1000000.0) {  // >= 1 second
        auto seconds = micros / 1000000.0;
        return fmt::format("{:.3f} seconds", seconds);
    } else if (micros >= 1000.0) {  // >= 1 millisecond
        auto millis = micros / 1000.0;
        return fmt::format("{:.3f} ms", millis);
    } else {
        return fmt::format("{:.3f} μs", micros);
    }
}
```

#### `getByteSizeDescription` Implementation

```cpp
std::string getByteSizeDescription(uint64_t sizeInBytes);
```

**Binary Size Units**:

```cpp
std::string getByteSizeDescription(uint64_t size) {
    constexpr uint64_t TB = 1024ULL * 1024ULL * 1024ULL * 1024ULL;
    constexpr uint64_t GB = 1024ULL * 1024ULL * 1024ULL;
    constexpr uint64_t MB = 1024ULL * 1024ULL;
    constexpr uint64_t KB = 1024ULL;
  
    if (size >= TB) {
        return fmt::format("{:.2f} TB", static_cast<double>(size) / TB);
    } else if (size >= GB) {
        return fmt::format("{:.2f} GB", static_cast<double>(size) / GB);
    } else if (size >= MB) {
        return fmt::format("{:.2f} MB", static_cast<double>(size) / MB);
    } else if (size >= KB) {
        return fmt::format("{:.2f} KB", static_cast<double>(size) / KB);
    } else {
        return fmt::format("{} bytes", size);
    }
}
```

## Performance Analysis

### Computational Complexity

| Function                   | Time Complexity | Space Complexity | Notes                         |
| -------------------------- | --------------- | ---------------- | ----------------------------- |
| `isWhitespace`           | O(1)            | O(1)             | Single arithmetic check       |
| `isDigit`                | O(1)            | O(1)             | Single arithmetic check       |
| `replace`                | O(n*r)          | O(n)             | n=text length, r=replacements |
| `trim`                   | O(n)            | O(1) or O(n)     | Depends on return type        |
| `splitString`            | O(n)            | O(k)             | k=number of tokens            |
| `joinStrings`            | O(n)            | O(n)             | n=total character count       |
| `getLevenshteinDistance` | O(m*n)          | O(m*n)           | m,n=string lengths            |

### Memory Usage Patterns

#### Zero-Copy Operations

```cpp
// ✅ Good: No memory allocation
std::string_view ProcessLine(std::string_view line) {
    line = StringUtils::trim(line);  // Returns trimmed view
  
    if (StringUtils::startsWith(line, "#")) {
        return std::string_view{};  // Empty view for comments
    }
  
    return line;
}
```

#### Efficient Memory Allocation

```cpp
// ✅ Good: Pre-calculate and reserve
std::string BuildLargeString(const std::vector<std::string>& parts) {
    size_t totalSize = 0;
    for (const auto& part : parts) {
        totalSize += part.size();
    }
  
    std::string result;
    result.reserve(totalSize + parts.size() - 1);  // Include separators
  
    return StringUtils::joinStrings(parts, " ");
}
```

### Compiler Optimizations

#### Template Instantiation Optimization

```cpp
// Explicit instantiation for common types reduces compile time
extern template std::string StringUtils::replace<std::string>(
    std::string, std::string_view, std::string_view);

extern template std::vector<std::string> StringUtils::splitString<char>(
    std::string_view, char, bool);
```

#### Inline Function Benefits

```cpp
// Small functions benefit from inlining
inline bool isWhitespace(char c) { /* ... */ }
inline bool isDigit(char c) { /* ... */ }

// Complex templates should not be forced inline
template <typename StringType, typename... OtherReplacements>
/* not inline */ std::string replace(/* ... */);
```

## Implementation Patterns

### SFINAE and Template Constraints

```cpp
// Enable template only for string-like types
template <typename StringType>
std::enable_if_t<std::is_convertible_v<StringType, std::string_view>, bool>
contains(const StringType& text, std::string_view substring) {
    return std::string_view(text).find(substring) != std::string_view::npos;
}

// C++20 concepts version
template <typename StringType>
requires std::convertible_to<StringType, std::string_view>
bool contains(const StringType& text, std::string_view substring) {
    return std::string_view(text).find(substring) != std::string_view::npos;
}
```

### Perfect Forwarding Patterns

```cpp
template <typename StringType, typename... Args>
auto processString(StringType&& str, Args&&... args) {
    // Forward string type correctly
    if constexpr (std::is_rvalue_reference_v<StringType&&>) {
        return doProcessing(std::move(str), std::forward<Args>(args)...);
    } else {
        return doProcessing(str, std::forward<Args>(args)...);
    }
}
```

### Error Handling in Templates

```cpp
template <typename InputIt, typename OutputIt>
OutputIt copyValidCharacters(InputIt first, InputIt last, OutputIt result) {
    static_assert(std::is_same_v<typename std::iterator_traits<InputIt>::value_type, char>,
                  "Input iterator must yield char");
  
    return std::copy_if(first, last, result, [](char c) {
        return c >= 32 && c <= 126;  // Printable ASCII only
    });
}
```

## Extension Guidelines

### Adding New Template Functions

When extending the StringUtils namespace:

1. **Use templates appropriately**: For functions that work with multiple string types
2. **Provide overloads**: For specific optimizations
3. **Document complexity**: Include time/space complexity in comments
4. **Test thoroughly**: Cover edge cases and performance characteristics

#### Example Extension

```cpp
namespace SceneryEditorX::StringUtils
{
    /// Removes consecutive duplicate characters from string
    /// Time complexity: O(n), Space complexity: O(1) for in-place version
    template <typename StringType>
    std::string removeDuplicateChars(StringType&& text) {
        std::string result(std::forward<StringType>(text));
    
        if (result.size() <= 1) {
            return result;
        }
    
        auto writePos = result.begin();
        auto readPos = result.begin();
        char lastChar = *readPos++;
        *writePos++ = lastChar;
    
        while (readPos != result.end()) {
            if (*readPos != lastChar) {
                lastChar = *readPos;
                *writePos++ = lastChar;
            }
            ++readPos;
        }
    
        result.erase(writePos, result.end());
        return result;
    }
  
    /// In-place version for better performance
    std::string& removeDuplicateCharsInPlace(std::string& text);
}
```

### Performance Testing Framework

```cpp
// Template for performance testing string functions
template <typename Func, typename... Args>
auto benchmarkStringFunction(const std::string& testName, 
                            size_t iterations,
                            Func&& func, 
                            Args&&... args) {
    auto start = std::chrono::high_resolution_clock::now();
  
    for (size_t i = 0; i < iterations; ++i) {
        auto result = func(args...);
        // Prevent optimization from eliminating the call
        DoNotOptimize(result);
    }
  
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  
    SEDX_CORE_INFO_TAG("Performance", "{}: {} iterations in {}", 
                      testName, iterations, 
                      StringUtils::getDurationDescription(duration));
  
    return duration;
}
```

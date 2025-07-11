# String.h API Reference Documentation

---

## Overview

The `string.h` header file provides the high-level string processing interface for the Scenery Editor X application. This header defines the `String` namespace containing essential string manipulation functions, as well as global utility functions for file path processing, string splitting, and data formatting.

This API reference serves as a comprehensive guide to all functions, their parameters, return values, and usage patterns within the Scenery Editor X architecture.

## Table of Contents

1. [String Namespace Functions](#string-namespace-functions)
2. [Global String Functions](#global-string-functions)
3. [Template Functions](#template-functions)
4. [Constexpr Utilities](#constexpr-utilities)
5. [File I/O Functions](#file-io-functions)
6. [Function Reference](#function-reference)
7. [Usage Examples](#usage-examples)
8. [Error Handling](#error-handling)

## String Namespace Functions

### Case Conversion and Comparison

#### `bool EqualsIgnoreCase(std::string_view a, std::string_view b)`

**Purpose**: Performs case-insensitive comparison of two string views.

**Parameters**:

- `a`: First string to compare
- `b`: Second string to compare

**Returns**: `true` if strings are equal (ignoring case), `false` otherwise

**Complexity**: O(n) where n is the length of the shorter string

**Thread Safety**: Thread-safe (read-only operation)

**Example**:

```cpp
bool result1 = String::EqualsIgnoreCase("Hello", "HELLO");     // true
bool result2 = String::EqualsIgnoreCase("test", "Test");       // true
bool result3 = String::EqualsIgnoreCase("abc", "def");         // false
```

---

#### `std::string& ToLower(std::string& string)`

**Purpose**: Converts a string to lowercase in-place.

**Parameters**:

- `string`: Reference to the string to convert (modified in-place)

**Returns**: Reference to the modified string (for chaining)

**Complexity**: O(n) where n is the string length

**Thread Safety**: Not thread-safe (modifies input)

**Example**:

```cpp
std::string text = "Hello World";
String::ToLower(text);  // text is now "hello world"

// Chaining example
std::string result = String::ToLower(String::TrimWhitespace(userInput));
```

---

#### `std::string ToLowerCopy(std::string_view string)`

**Purpose**: Creates a lowercase copy of the input string without modifying the original.

**Parameters**:

- `string`: String view to convert

**Returns**: New string containing lowercase version

**Complexity**: O(n) where n is the string length

**Thread Safety**: Thread-safe

**Example**:

```cpp
std::string_view original = "MIXED Case";
std::string lower = String::ToLowerCopy(original);  // "mixed case"
// original unchanged
```

---

#### `std::string& ToUpper(std::string& string)`

**Purpose**: Converts a string to uppercase in-place.

**Parameters**:

- `string`: Reference to the string to convert (modified in-place)

**Returns**: Reference to the modified string (for chaining)

**Example**:

```cpp
std::string text = "hello world";
String::ToUpper(text);  // text is now "HELLO WORLD"
```

---

#### `std::string ToUpperCopy(std::string_view string)`

**Purpose**: Creates an uppercase copy of the input string.

**Parameters**:

- `string`: String view to convert

**Returns**: New string containing uppercase version

**Example**:

```cpp
std::string_view original = "mixed Case";
std::string upper = String::ToUpperCopy(original);  // "MIXED CASE"
```

---

#### `int32_t CompareCase(std::string_view a, std::string_view b)`

**Purpose**: Performs case-insensitive string comparison returning ordering information.

**Parameters**:

- `a`: First string to compare
- `b`: Second string to compare

**Returns**:

- Negative value if `a` < `b`
- Zero if `a` == `b`
- Positive value if `a` > `b`

**Platform Notes**: Uses `_stricmp` on Windows, `strcasecmp` on other platforms

**Example**:

```cpp
int result1 = String::CompareCase("apple", "BANANA");  // < 0
int result2 = String::CompareCase("HELLO", "hello");   // == 0
int result3 = String::CompareCase("zebra", "APPLE");   // > 0
```

### Character and Whitespace Management

#### `void Erase(std::string& str, const char* chars)`

**Purpose**: Removes all occurrences of specified characters from a string.

**Parameters**:

- `str`: String to modify (modified in-place)
- `chars`: Null-terminated string containing characters to remove

**Returns**: None (void)

**Complexity**: O(n*m) where n is string length, m is number of characters to remove

**Example**:

```cpp
std::string text = "Hello, World!";
String::Erase(text, ",!");  // text becomes "Hello World"

// Remove multiple character types
String::Erase(text, " \t\n\r");  // Remove all whitespace
```

---

#### `void Erase(std::string& str, const std::string& chars)`

**Purpose**: Removes all occurrences of specified characters from a string (string overload).

**Parameters**:

- `str`: String to modify (modified in-place)
- `chars`: String containing characters to remove

**Returns**: None (void)

**Example**:

```cpp
std::string text = "a1b2c3d4";
std::string toRemove = "1234";
String::Erase(text, toRemove);  // text becomes "abcd"
```

---

#### `std::string TrimWhitespace(const std::string& str)`

**Purpose**: Returns a copy of the string with leading and trailing whitespace removed.

**Parameters**:

- `str`: String to trim

**Returns**: New string with whitespace trimmed

**Whitespace Definition**: Space, newline, carriage return, tab, form feed, vertical tab

**Example**:

```cpp
std::string text = "  \t  Hello World  \n  ";
std::string trimmed = String::TrimWhitespace(text);  // "Hello World"
```

---

#### `std::string RemoveWhitespace(const std::string& str)`

**Purpose**: Returns a copy of the string with all whitespace characters removed.

**Parameters**:

- `str`: String to process

**Returns**: New string with all whitespace removed

**Example**:

```cpp
std::string text = "Hello\t World\n Test";
std::string cleaned = String::RemoveWhitespace(text);  // "HelloWorldTest"
```

### String Manipulation

#### `std::string SubStr(const std::string& string, size_t offset, size_t count = std::string::npos)`

**Purpose**: Safe substring extraction that handles out-of-bounds conditions gracefully.

**Parameters**:

- `string`: Source string
- `offset`: Starting position (0-based)
- `count`: Number of characters to extract (default: to end of string)

**Returns**:

- Substring if valid range
- Original string if `offset` is `std::string::npos`
- Original string if `offset` is out of bounds

**Safety**: Prevents out-of-bounds access, unlike standard `substr()`

**Example**:

```cpp
std::string text = "Hello World";

std::string sub1 = String::SubStr(text, 6);      // "World"
std::string sub2 = String::SubStr(text, 0, 5);   // "Hello"
std::string sub3 = String::SubStr(text, 100);    // "Hello World" (safe)
std::string sub4 = String::SubStr(text, std::string::npos);  // "Hello World"
```

### Time Utilities

#### `std::string GetCurrentTimeString(bool includeDate = false, bool useDashes = false)`

**Purpose**: Generates formatted timestamp strings for logging and file naming.

**Parameters**:

- `includeDate`: If true, includes date in format (default: false)
- `useDashes`: If true, replaces colons with dashes for filename safety (default: false)

**Returns**: Formatted time string

**Formats**:

- Time only: "14:30:25"
- With date: "2025:01:15:14:30:25"
- With dashes: "14-30-25" or "2025-01-15-14-30-25"

**Example**:

```cpp
// For logging
std::string logTime = String::GetCurrentTimeString();  // "14:30:25"

// For filenames (safe characters)
std::string fileTime = String::GetCurrentTimeString(true, true);  // "2025-01-15-14-30-25"

// For backup files
std::string backupName = "backup_" + String::GetCurrentTimeString(true, true) + ".cfg";
```

## Global String Functions

### File Path Processing

#### `std::string_view GetFilename(std::string_view filepath)`

**Purpose**: Extracts the filename from a complete file path.

**Parameters**:

- `filepath`: Complete file path (forward or backward slashes)

**Returns**:

- String view pointing to filename portion
- Empty string view if no filename found

**Note**: Returns a view into original string - ensure original remains valid

**Example**:

```cpp
std::string_view filename1 = GetFilename("/path/to/file.txt");      // "file.txt"
std::string_view filename2 = GetFilename("C:\\Windows\\file.exe");  // "file.exe"
std::string_view filename3 = GetFilename("file.txt");              // "file.txt"
std::string_view filename4 = GetFilename("/path/to/dir/");          // ""
```

---

#### `std::string GetExtension(const std::string& filename)`

**Purpose**: Extracts the file extension from a filename.

**Parameters**:

- `filename`: Filename or full path

**Returns**:

- File extension without the dot
- Empty string if no extension found

**Example**:

```cpp
std::string ext1 = GetExtension("document.pdf");        // "pdf"
std::string ext2 = GetExtension("archive.tar.gz");      // "gz"
std::string ext3 = GetExtension("README");              // ""
std::string ext4 = GetExtension(".hidden");             // ""
std::string ext5 = GetExtension("/path/file.TXT");      // "TXT"
```

---

#### `std::string RemoveExtension(const std::string& filename)`

**Purpose**: Returns filename with extension removed.

**Parameters**:

- `filename`: Filename to process

**Returns**: Filename without extension

**Example**:

```cpp
std::string name1 = RemoveExtension("document.pdf");     // "document"
std::string name2 = RemoveExtension("archive.tar.gz");   // "archive.tar"
std::string name3 = RemoveExtension("README");           // "README"
```

### String Splitting Functions

#### `std::vector<std::string> SplitString(std::string_view string, const std::string_view& delimiters)`

**Purpose**: Splits a string using any character from a set of delimiters.

**Parameters**:

- `string`: String to split
- `delimiters`: String containing delimiter characters

**Returns**: Vector of string tokens (delimiters excluded)

**Behavior**: Consecutive delimiters create empty tokens

**Example**:

```cpp
// Multiple delimiters
auto tokens1 = SplitString("a,b;c:d", ",;:");           // ["a", "b", "c", "d"]

// Path components
auto tokens2 = SplitString("/usr/local/bin", "/");       // ["", "usr", "local", "bin"]

// CSV with semicolon
auto tokens3 = SplitString("name;age;city", ";");        // ["name", "age", "city"]
```

---

#### `std::vector<std::string> SplitString(std::string_view string, char delimiter)`

**Purpose**: Splits a string using a single character delimiter.

**Parameters**:

- `string`: String to split
- `delimiter`: Single character delimiter

**Returns**: Vector of string tokens

**Example**:

```cpp
auto tokens1 = SplitString("apple,banana,cherry", ','); // ["apple", "banana", "cherry"]
auto tokens2 = SplitString("one|two|three", '|');       // ["one", "two", "three"]
```

### Advanced String Processing

#### `std::string SplitAtUpperCase(std::string_view string, std::string_view delimiter = " ", bool ifLowerCaseOnTheRight = true)`

**Purpose**: Inserts delimiters before uppercase characters to create readable text.

**Parameters**:

- `string`: String to process
- `delimiter`: String to insert (default: space)
- `ifLowerCaseOnTheRight`: Only split if followed by lowercase (default: true)

**Returns**: String with delimiters inserted

**Use Cases**: Converting camelCase/PascalCase to readable text

**Example**:

```cpp
// Class names to readable text
std::string readable1 = SplitAtUpperCase("VulkanRenderer");      // "Vulkan Renderer"
std::string readable2 = SplitAtUpperCase("XMLHttpRequest");      // "XMLHttp Request"

// Custom delimiter
std::string underscored = SplitAtUpperCase("MyClassName", "_");  // "My_Class_Name"

// Control splitting behavior
std::string acronym = SplitAtUpperCase("XMLParser", " ", false); // "X M L Parser"
```

### Data Formatting Functions

#### `std::string BytesToString(uint64_t bytes)`

**Purpose**: Converts byte count to human-readable string with appropriate units.

**Parameters**:

- `bytes`: Number of bytes to format

**Returns**: Formatted string with units (bytes, KB, MB, GB)

**Precision**: 2 decimal places for fractional values

**Example**:

```cpp
std::string size1 = BytesToString(512);           // "512.00 bytes"
std::string size2 = BytesToString(1536);          // "1.50 KB"
std::string size3 = BytesToString(2097152);       // "2.00 MB"
std::string size4 = BytesToString(3221225472ULL); // "3.00 GB"
```

---

#### `std::string DurationToString(std::chrono::duration<double> duration)`

**Purpose**: Formats duration as MM:SS.mmm string for display.

**Parameters**:

- `duration`: Duration to format

**Returns**: Formatted string in "M:SS.mmm" format

**Format**: Minutes:Seconds.Milliseconds with proper zero-padding

**Example**:

```cpp
using namespace std::chrono;

auto dur1 = duration<double>(65.123);    // 65.123 seconds
std::string str1 = DurationToString(dur1);  // "1:05.123"

auto dur2 = duration<double>(3.456);     // 3.456 seconds  
std::string str2 = DurationToString(dur2);  // "0:03.456"
```

### Type Name Processing

#### `std::string TemplateToParenthesis(std::string_view name)`

**Purpose**: Converts template angle brackets to parentheses for readable display.

**Parameters**:

- `name`: Type name containing templates

**Returns**: String with `<>` replaced by `()` and formatting improvements

**Processing**:

- Adds space before opening bracket if needed
- Capitalizes first letter after opening bracket
- Converts `<Type>` to ` (Type)`

**Example**:

```cpp
std::string readable1 = TemplateToParenthesis("std::vector<int>");           // "std::vector (Int)"
std::string readable2 = TemplateToParenthesis("std::map<string,int>");       // "std::map (String,int)"
std::string readable3 = TemplateToParenthesis("MyClass<T,U>");               // "MyClass (T,U)"
```

---

#### `std::string CreateUserFriendlyTypeName(std::string_view name)`

**Purpose**: Creates human-readable type names from C++ type strings.

**Parameters**:

- `name`: Raw C++ type name (typically from `typeid().name()`)

**Returns**: User-friendly formatted type name

**Processing**:

1. Removes namespace prefixes
2. Splits camelCase words
3. Converts templates to parentheses format

**Example**:

```cpp
// Typical usage with typeid
std::string friendly1 = CreateUserFriendlyTypeName(typeid(std::vector<int>).name());
// Result: "Vector (Int)"

std::string friendly2 = CreateUserFriendlyTypeName("MyProject::TextureManager");
// Result: "Texture Manager"
```

## Template Functions

### Duration Breakdown

#### `template <class... Durations, class DurationIn> std::tuple<Durations...> BreakDownDuration(DurationIn d)`

**Purpose**: Breaks down a duration into component units (hours, minutes, seconds, etc.).

**Template Parameters**:

- `Durations...`: Duration types to break down into
- `DurationIn`: Input duration type

**Parameters**:

- `d`: Duration to break down

**Returns**: Tuple containing the duration broken into specified units

**Example**:

```cpp
using namespace std::chrono;

auto total = duration<double>(3725.123);  // 3725.123 seconds

// Break down into hours, minutes, seconds, milliseconds
auto breakdown = BreakDownDuration<hours, minutes, seconds, milliseconds>(total);

auto hrs = std::get<0>(breakdown);   // 1 hour
auto min = std::get<1>(breakdown);   // 2 minutes  
auto sec = std::get<2>(breakdown);   // 5 seconds
auto ms = std::get<3>(breakdown);    // 123 milliseconds
```

### Unique Name Generation

#### `template <typename IsAlreadyUsedFn> std::string AddSuffixToMakeUnique(const std::string& name, IsAlreadyUsedFn&& isUsed)`

**Purpose**: Generates unique names by appending numeric suffixes.

**Template Parameters**:

- `IsAlreadyUsedFn`: Callable that returns true if name is already used

**Parameters**:

- `name`: Base name to make unique
- `isUsed`: Function/lambda that checks if a name is already in use

**Returns**: Unique name (possibly with suffix)

**Behavior**: Tries name, then name_2, name_3, etc. until finding unused name

**Example**:

```cpp
std::set<std::string> existingNames = {"file", "file_2", "document"};

auto checker = [&](const std::string& name) {
    return existingNames.contains(name);
};

std::string unique1 = AddSuffixToMakeUnique("file", checker);      // "file_3"
std::string unique2 = AddSuffixToMakeUnique("newfile", checker);   // "newfile"
std::string unique3 = AddSuffixToMakeUnique("document", checker);  // "document_2"
```

## Constexpr Utilities

### String Prefix/Suffix Checking

#### `constexpr bool StartsWith(std::string_view t, std::string_view s)`

**Purpose**: Compile-time check if string starts with given prefix.

**Parameters**:

- `t`: String to check
- `s`: Prefix to look for

**Returns**: True if `t` starts with `s`

**Advantages**: Compile-time evaluation when possible, no allocation

**Example**:

```cpp
constexpr bool result1 = StartsWith("hello world", "hello");  // true
constexpr bool result2 = StartsWith("test", "testing");       // false

// Runtime usage
bool dynamic = StartsWith(userInput, "cmd:");
```

---

#### `constexpr bool EndsWith(std::string_view t, std::string_view s)`

**Purpose**: Compile-time check if string ends with given suffix.

**Parameters**:

- `t`: String to check
- `s`: Suffix to look for

**Returns**: True if `t` ends with `s`

**Example**:

```cpp
constexpr bool result1 = EndsWith("document.pdf", ".pdf");    // true
constexpr bool result2 = EndsWith("image.png", ".jpg");       // false

// File extension checking
bool isShader = EndsWith(filename, ".vert") || EndsWith(filename, ".frag");
```

### Token Counting

#### `constexpr size_t GetNumberOfTokens(std::string_view source, std::string_view delimiter)`

**Purpose**: Counts number of tokens that would result from splitting by delimiter.

**Parameters**:

- `source`: String to analyze
- `delimiter`: Delimiter string

**Returns**: Number of tokens (always at least 1 if source is non-empty)

**Use Case**: Pre-allocating arrays for compile-time string splitting

**Example**:

```cpp
constexpr size_t count1 = GetNumberOfTokens("a,b,c", ",");      // 3
constexpr size_t count2 = GetNumberOfTokens("single", ",");     // 1
constexpr size_t count3 = GetNumberOfTokens("a,,b", ",");       // 3 (includes empty)

// Use for array sizing
constexpr auto tokenCount = GetNumberOfTokens(CSV_DATA, ",");
std::array<std::string_view, tokenCount> tokens;
```

### Compile-time String Splitting

#### `template <size_t N> constexpr std::array<std::string_view, N> SplitString(std::string_view source, std::string_view delimiter)`

**Purpose**: Compile-time string splitting into fixed-size array.

**Template Parameters**:

- `N`: Number of expected tokens

**Parameters**:

- `source`: String to split
- `delimiter`: Delimiter string

**Returns**: Array of string views pointing into original string

**Requirements**: Template parameter `N` must match actual token count

**Example**:

```cpp
constexpr std::string_view data = "red,green,blue";
constexpr auto colors = SplitString<3>(data, ",");

// colors[0] = "red"
// colors[1] = "green"  
// colors[2] = "blue"

// Use with GetNumberOfTokens for safety
constexpr auto count = GetNumberOfTokens(data, ",");
constexpr auto tokens = SplitString<count>(data, ",");
```

### Namespace Processing

#### `constexpr std::string_view RemoveNamespace(std::string_view name)`

**Purpose**: Removes C++ namespace qualification from type/function names.

**Parameters**:

- `name`: Fully qualified name

**Returns**: Name with namespace removed

**Behavior**: Removes everything up to and including last `::`

**Example**:

```cpp
constexpr auto simple1 = RemoveNamespace("std::vector");           // "vector"
constexpr auto simple2 = RemoveNamespace("MyProject::Math::Vec3"); // "Vec3"
constexpr auto simple3 = RemoveNamespace("GlobalFunction");        // "GlobalFunction"
```

---

#### `constexpr std::string_view RemoveOuterNamespace(std::string_view name)`

**Purpose**: Removes only the outermost namespace level.

**Parameters**:

- `name`: Fully qualified name

**Returns**: Name with only outer namespace removed

**Example**:

```cpp
constexpr auto partial1 = RemoveOuterNamespace("std::vector");           // "vector"
constexpr auto partial2 = RemoveOuterNamespace("MyProject::Math::Vec3"); // "Math::Vec3"
constexpr auto partial3 = RemoveOuterNamespace("GlobalFunction");        // "GlobalFunction"
```

#### `template <size_t N> constexpr std::array<std::string_view, N> RemoveNamespace(std::array<std::string_view, N> memberList)`

**Purpose**: Removes namespaces from all elements in an array.

**Template Parameters**:

- `N`: Array size

**Parameters**:

- `memberList`: Array of fully qualified names

**Returns**: Array with namespaces removed from all elements

**Example**:

```cpp
constexpr std::array<std::string_view, 3> qualified = {
    "std::string", "MyProject::Vector3", "GlobalType"
};

constexpr auto simple = RemoveNamespace(qualified);
// Result: {"string", "Vector3", "GlobalType"}
```

### Variable Name Processing

#### `constexpr std::string_view RemovePrefixAndSuffix(std::string_view name)`

**Purpose**: Removes common variable prefixes and suffixes used in code generation.

**Parameters**:

- `name`: Variable name to clean

**Returns**: Name with common prefixes/suffixes removed

**Removed Patterns**:

- Prefixes: `in_`, `out_`
- Suffixes: `_Raw`

**Use Case**: Cleaning up generated variable names for display

**Example**:

```cpp
constexpr auto clean1 = RemovePrefixAndSuffix("in_Position");    // "Position"
constexpr auto clean2 = RemovePrefixAndSuffix("out_Color");      // "Color"
constexpr auto clean3 = RemovePrefixAndSuffix("data_Raw");       // "data"
constexpr auto clean4 = RemovePrefixAndSuffix("normalVar");      // "normalVar"
```

## File I/O Functions

### BOM Handling

#### `int SkipBOM(std::istream& in)`

**Purpose**: Detects and skips UTF-8 Byte Order Mark in input stream.

**Parameters**:

- `in`: Input stream to process

**Returns**: Number of bytes skipped (0 or 3)

**Behavior**:

- Checks for UTF-8 BOM (0xEF 0xBB 0xBF)
- Advances stream position past BOM if found
- Resets to beginning if no BOM found

**Side Effects**: Modifies stream position

**Example**:

```cpp
std::ifstream file("config.txt");
int skipped = SkipBOM(file);

if (skipped > 0) {
    SEDX_CORE_INFO_TAG("FileIO", "Skipped {} byte BOM", skipped);
}

// Stream is now positioned after BOM (if present) or at beginning
std::string content;
std::getline(file, content);
```

---

#### `std::string ReadFileAndSkipBOM(const std::filesystem::path& filepath)`

**Purpose**: Reads entire file content, automatically handling UTF-8 BOM.

**Parameters**:

- `filepath`: Path to file to read

**Returns**:

- File content as string (BOM removed if present)
- Empty string if file cannot be read

**Special Behavior**: Adds dummy tab character at beginning of string

**Error Handling**: Returns empty string on failure (check with `.empty()`)

**Example**:

```cpp
auto configPath = std::filesystem::path("settings.cfg");
std::string content = ReadFileAndSkipBOM(configPath);

if (content.empty()) {
    SEDX_CORE_ERROR_TAG("Config", "Failed to read config file");
    return false;
}

// Process content (note: first character is always '\t')
auto lines = StringUtils::splitIntoLines(content.substr(1), false);
```

## Error Handling Patterns

### Safe String Operations

```cpp
// ✅ Good: Check for empty results
std::string extension = GetExtension(filename);
if (extension.empty()) {
    SEDX_CORE_WARN_TAG("FileProcessor", "File has no extension: {}", filename);
    return false;
}

// ✅ Good: Validate input before processing
std::string ProcessUserInput(std::string_view input) {
    if (input.empty()) {
        throw std::invalid_argument("Input cannot be empty");
    }
  
    auto trimmed = String::TrimWhitespace(std::string(input));
    if (trimmed.empty()) {
        throw std::invalid_argument("Input contains only whitespace");
    }
  
    return trimmed;
}
```

### File Operation Error Handling

```cpp
// ✅ Good: Comprehensive file reading with error handling
std::optional<std::string> SafeReadFile(const std::filesystem::path& path) {
    try {
        if (!std::filesystem::exists(path)) {
            SEDX_CORE_WARN_TAG("FileIO", "File does not exist: {}", path.string());
            return std::nullopt;
        }
    
        if (!std::filesystem::is_regular_file(path)) {
            SEDX_CORE_ERROR_TAG("FileIO", "Path is not a regular file: {}", path.string());
            return std::nullopt;
        }
    
        auto content = ReadFileAndSkipBOM(path);
        if (content.empty()) {
            SEDX_CORE_WARN_TAG("FileIO", "File is empty or unreadable: {}", path.string());
            return std::nullopt;
        }
    
        return content;
    
    } catch (const std::filesystem::filesystem_error& e) {
        SEDX_CORE_ERROR_TAG("FileIO", "Filesystem error reading {}: {}", 
                           path.string(), e.what());
        return std::nullopt;
    }
}
```

## Performance Notes

### Memory Efficiency

1. **Use `string_view` when possible**: Avoids unnecessary copying
2. **Reserve string capacity**: For large concatenations
3. **Move semantics**: Use `std::move()` for temporary strings
4. **In-place operations**: Prefer `ToLower()` over `ToLowerCopy()` when original isn't needed

### Algorithm Complexity

| Function                | Time Complexity | Space Complexity      |
| ----------------------- | --------------- | --------------------- |
| `EqualsIgnoreCase`    | O(n)            | O(1)                  |
| `ToLower`/`ToUpper` | O(n)            | O(1)                  |
| `TrimWhitespace`      | O(n)            | O(n)                  |
| `SplitString`         | O(n)            | O(k) where k = tokens |
| `GetExtension`        | O(n)            | O(1)                  |
| `BytesToString`       | O(1)            | O(1)                  |

### Threading Considerations

- **Thread-safe functions**: All `const` and `string_view` functions
- **Non-thread-safe**: In-place modification functions (`ToLower`, `Erase`)
- **File I/O**: Not thread-safe due to shared file handles

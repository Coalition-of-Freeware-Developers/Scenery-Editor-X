# Scenery Editor X - Coding Style Guidelines

This document outlines the coding style and formatting requirements for the Scenery Editor X project. All code contributions must adhere to these guidelines to maintain consistency and readability throughout the codebase.

## Overview

The project uses automated formatting tools to ensure consistent code style:

- **clang-format** for C++ code formatting
- **EditorConfig** for general file formatting rules

## General Formatting Rules

### Indentation and Spacing

- **Indentation Style**: Tabs only (no spaces for indentation)
- **Tab Width**: 4 characters
- **Indent Size**: 4 characters
- **Trailing Whitespace**: Must be trimmed from all lines
- **Final Newline**: All files must end with a newline
- **Line Endings**: CRLF (Windows style) for all files

### Line Length

- **Maximum Line Length**: 120 characters
- Long lines should be broken appropriately following the formatting rules below

## C++ Specific Formatting Rules

### Braces and Block Structure

- **Brace Style**: Custom Allman style with braces on new lines
- **Control Statements**: Always use braces, even for single statements
- **Empty Blocks**: Split empty functions, records, and namespaces

```cpp
// Correct
if (condition)
{
    doSomething();
}

// Incorrect
if (condition) doSomething();
```

### Function and Method Formatting

- **Short Functions**: Not allowed on single line
- **Function Parameters**: Up to three parameters per line before when breaking
- **Return Type**: Never on its own line
- **Constructor Initializers**: Break before colon

```cpp
// Function declaration
ReturnType FunctionName(Type1 parameter1, Type2 parameter2, Type3 parameter3,
    Type4 parameter4
);

// Constructor with initializer list
MyClass::MyClass(Type1 param1, Type2 param2)
    : member1(param1), member2(param2)
{
    // Constructor body
}
```

### Access Modifiers

- **Offset**: -4 characters from class indentation level

```cpp
class MyClass
{
public:
    void PublicMethod();
  
private:
    int privateMember;
};
```

### Pointer and Reference Alignment

- **Pointer Alignment**: Right-aligned (attach to variable name)

```cpp
// Correct
int *pointer;
const std::string &reference;

// Incorrect
int* pointer;
const std::string& reference;
```

### Template Declarations

- **Template Breaking**: Never break before template declarations when multiline

```cpp
template<typename T, typename U>
class MyTemplate
{
    // Class body
};
```

### Spacing Rules

- **Control Statements**: Space before parentheses
- **Function Calls**: No space before parentheses
- **Operators**: Spaces around assignment and binary operators
- **Comments**: Single space before trailing comments

```cpp
// Control statements
if (condition)
while (condition)
for (int i = 0; i < size; ++i)

// Function calls
functionCall(parameter);

// Operators
int result = a + b;
variable = value;

// Comments
int value = 42; // Trailing comment
```

### Case Labels and Switch Statements

- **Case Labels**: Not indented relative to switch
- **Case Blocks**: Use of braces is optional

```cpp
switch (value)
{
case 1:
{
    doSomething();
    break;
}
case 2:
{
    doSomethingElse();
    break;
}
default:
{
    handleDefault();
    break;
}
}
```

### Include Organization

- **Include Blocks**: Preserve existing organization
- **Include Sorting**: Disabled (maintain manual organization)
- **Include Categories** (in order of priority):
  1. Project headers
  2. LLVM/Clang headers
  3. Third-party library headers (gtest, gmock, json, etc.)

### Namespace Formatting

- **Namespace Indentation**: None
- **Namespace Comments**: Automatically fixed
- **Compact Namespaces**: Disabled

```cpp
namespace MyNamespace
{
    class MyClass
          {
        // Class content not indented relative to namespace
    };
} // namespace MyNamespace
```

## Documentation Standards

All code must follow Doxygen documentation standards:

### Function Documentation

```cpp
/**
 * @brief Brief description of the function
 * 
 * Detailed description of what the function does, including:
 * 1. Step-by-step process explanation
 * 2. Important implementation details
 * 3. Special requirements or considerations
 * 
 * @param paramName Description of the parameter
 * @param anotherParam Description of another parameter
 * @return Description of return value
 * 
 * @note Any important notes about usage
 * @warning Any warnings about potential issues
 */
ReturnType FunctionName(Type1 paramName, Type2 anotherParam);
```

### Class Documentation

```cpp
/**
 * @brief Brief description of the class
 * 
 * Detailed description of the class purpose, responsibilities,
 * and usage patterns.
 * 
 * @note Usage notes
 * @warning Important warnings
 */
class MyClass
{
public:
    /**
     * @brief Brief description of member function
     * @param param Parameter description
     * @return Return value description
     */
    int MemberFunction(int param);
  
private:
    int m_memberVariable; ///< Brief description of member variable
};
```

## Memory Management Guidelines

- Use custom smart pointers from `pointers.h` and `pointers.cpp`
- Utilize memory utilities from `memory.h` and `memory.cpp`
- Follow RAII principles
- Avoid raw pointers except when interfacing with C APIs

## Error Handling

- Implement comprehensive error checking
- Check for null pointers before dereferencing
- Handle exceptions appropriately
- Use assertions for debug builds (`SEDX_ASSERT` macro)

## Performance Considerations

- Optimize for large models and complex scenes
- Consider multiple render passes and texture sets
- Efficient handling of PBR materials
- Minimize memory allocations in hot paths

## Automated Formatting

The project uses clang-format for automatic code formatting. Before submitting code:

1. Run clang-format on all modified C++ files
2. Ensure EditorConfig settings are applied
3. Verify no trailing whitespace or missing newlines

### Running clang-format

```bash
clang-format -i source/**/*.cpp source/**/*.h
```

## Enforcement

All pull requests must pass automated style checks. Code that doesn't conform to these guidelines will be automatically rejected by CI/CD pipelines.

## Tools Configuration

The style is enforced through:

- `.clang-format` configuration file in the repository root
- `.editorconfig` configuration file in the repository root
- Pre-commit hooks (if configured)
- CI/CD pipeline checks

For any questions about style guidelines not covered in this document, refer to the project's `.clang-format` and `.editorconfig` files for the definitive formatting rules.

# Scenery Editor X Logging System Documentation Index

This directory contains comprehensive documentation for the Scenery Editor X logging system. The documentation is organized to serve different audiences and use cases.

## Documentation Overview

### For GitHub Copilot and AI Agents
- **[logging-copilot-instructions.md](logging-copilot-instructions.md)** - Quick reference and guidelines specifically for AI coding assistants. Start here for immediate implementation guidance.

### For Developers
- **[logging-system-documentation.md](logging-system-documentation.md)** - Complete system overview, architecture, and usage patterns
- **[logging-system-api-reference.md](logging-system-api-reference.md)** - Detailed API documentation for all classes, methods, and macros
- **[logging-system-examples.md](logging-system-examples.md)** - Comprehensive examples and real-world use cases

## Quick Start Guide

### 1. For AI Assistants (GitHub Copilot, etc.)
Start with [logging-copilot-instructions.md](logging-copilot-instructions.md) which provides:
- Quick reference for proper macro usage
- Standard tag names and conventions
- Code patterns and templates
- Anti-patterns to avoid

### 2. For New Developers
Begin with [logging-system-documentation.md](logging-system-documentation.md) to understand:
- System architecture and design
- Logger types and their purposes
- Tag-based filtering system
- Integration with assertion system

### 3. For API Reference
Use [logging-system-api-reference.md](logging-system-api-reference.md) for:
- Complete class and method documentation
- Parameter details and return values
- Usage examples for each API
- Template method specifications

### 4. For Implementation Examples
Refer to [logging-system-examples.md](logging-system-examples.md) for:
- Real-world usage patterns
- Subsystem-specific implementations
- Performance monitoring techniques
- Error handling strategies

## Key Concepts Summary

### Logger Types
- **Core Logger**: Engine internals, system operations, Vulkan/graphics
- **Editor Logger**: UI operations, user interactions, editor tools
- **Editor Console Logger**: In-editor debug console display
- **Launcher Logger**: Application startup and launcher operations

### Log Levels (by severity)
1. **Trace**: Detailed debugging information
2. **Info**: General informational messages
3. **Warn**: Warning messages for potential issues
4. **Error**: Error conditions that don't halt execution
5. **Fatal**: Critical errors that may cause termination

### Preferred Logging Pattern
```cpp
// Always use tagged logging with descriptive tags
SEDX_CORE_INFO_TAG("Renderer::Vulkan", "Device created: {}", deviceName);
EDITOR_WARN_TAG("AssetManager", "Asset not found: {}", assetPath);
```

### Standard Tag Categories
- **Core Tags**: `"Renderer"`, `"Memory"`, `"AssetManager"`, `"FileSystem"`, `"Vulkan"`
- **Editor Tags**: `"UI"`, `"Project"`, `"Scene"`, `"Tools"`, `"Serialization"`
- **System Tags**: `"Application"`, `"Config"`, `"Performance"`, `"Network"`

## File Organization

```
docs/
├── logging-copilot-instructions.md    # Quick reference for AI assistants
├── logging-system-documentation.md    # Complete system documentation  
├── logging-system-api-reference.md    # Detailed API documentation
├── logging-system-examples.md         # Comprehensive examples
└── logging-documentation-index.md     # This index file
```

## Source Files

The logging system is implemented in:
- `source/SceneryEditorX/logging/logging.hpp` - Main header with class definitions and macros
- `source/SceneryEditorX/logging/logging.cpp` - Implementation of logging functionality
- `source/SceneryEditorX/logging/asserts.h` - Assertion macros that integrate with logging

## Integration Points

### With Assertion System
The logging system provides the backend for assertion macros:
```cpp
SEDX_CORE_ASSERT(condition, "Error message: {}", details);
SEDX_VERIFY(condition, "Verification failed: {}", context);
```

### With Vulkan Debugging
Special integration for Vulkan validation layers:
```cpp
Log::LogVulkanDebug(validationMessage);  // Automatic severity parsing
Log::LogVulkanResult(result, operation);  // VkResult handling
```

### With Performance Monitoring
Built-in support for performance tracking:
```cpp
if (Log::HasTag("Performance")) {
    // Expensive performance logging only when enabled
}
```

## Best Practices Quick Reference

1. **Use tagged logging** - Always prefer `*_TAG` macros over untagged versions
2. **Choose appropriate levels** - Match severity to the actual impact
3. **Provide context** - Include relevant values, file paths, error codes
4. **Use consistent formatting** - Follow established patterns for similar operations
5. **Conditional expensive logging** - Check tag enablement before costly operations
6. **Meaningful tag names** - Use descriptive, hierarchical tags like `"Renderer::Vulkan"`

## Getting Help

- For quick implementation guidance: [logging-copilot-instructions.md](logging-copilot-instructions.md)
- For understanding the system: [logging-system-documentation.md](logging-system-documentation.md)  
- For API details: [logging-system-api-reference.md](logging-system-api-reference.md)
- For implementation patterns: [logging-system-examples.md](logging-system-examples.md)

## Contributing

When modifying the logging system:
1. Update relevant documentation files
2. Add examples for new features
3. Update the API reference for interface changes
4. Ensure AI instruction consistency

This documentation is designed to be self-contained and comprehensive, providing everything needed to understand and properly use the Scenery Editor X logging system.

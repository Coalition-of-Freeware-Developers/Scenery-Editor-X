# -------------------------------------------------------
# Scenery Editor X - Unit Tests
# -------------------------------------------------------
# Copyright (c) 2025 Thomas Ray
# Copyright (c) 2025 Coalition of Freeware Developers
# -------------------------------------------------------
# CMakeLists.txt for Ref tests
# -------------------------------------------------------

CMAKE_MINIMUM_REQUIRED(VERSION 3.16)
PROJECT(RefTests VERSION 1.0.0 LANGUAGES CXX)

# Set C++20 standard
SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

# Define the test executable
ADD_EXECUTABLE(RefTests
    pointer_tests/TestMain.cpp
    pointer_tests/RefTest.cpp
    pointer_tests/WeakRefTest.cpp
    pointer_tests/RefPerformanceTest.cpp
    pointer_tests/RefThreadSafetyTest.cpp
)

# Include directories
TARGET_INCLUDE_DIRECTORIES(RefTests PRIVATE
    ${CMAKE_SOURCE_DIR}/source
)

# Link with Catch2
TARGET_LINK_LIBRARIES(RefTests PRIVATE
    Catch2::Catch2WithMain
    glm::glm-header-only
)

# Enable parallel compilation for MSVC
IF(MSVC)
    TARGET_COMPILE_OPTIONS(RefTests PRIVATE /MP)
ENDIF()

# Enable warning level
IF(MSVC)
    TARGET_COMPILE_OPTIONS(RefTests PRIVATE /W4)
ELSE()
    TARGET_COMPILE_OPTIONS(RefTests PRIVATE -Wall -Wextra -Wpedantic)
ENDIF()

# Register with CTest
INCLUDE(CTest)
INCLUDE(Catch)
catch_discover_tests(RefTests)

# --------------------------------
# Memory Allocator Tests
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating Memory Allocator Tests")

# Define the test executable for memory allocator tests
ADD_EXECUTABLE(MemoryAllocatorTests
    memory_tests/MemoryTestMain.cpp
    memory_tests/MemoryAllocatorTestUtils.cpp
    memory_tests/MemoryAllocatorTest.cpp
    memory_tests/MemoryAllocatorStressTest.cpp
    memory_tests/MemoryPoolTest.cpp
)

# Include directories
TARGET_INCLUDE_DIRECTORIES(MemoryAllocatorTests PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/dependency/vma
    $ENV{VULKAN_SDK}/Include/
)

# Link with Catch2 and Vulkan libraries
TARGET_LINK_LIBRARIES(MemoryAllocatorTests PRIVATE
    Catch2::Catch2
    vulkan-1
)

# Enable parallel compilation for MSVC
IF(MSVC)
    TARGET_COMPILE_OPTIONS(MemoryAllocatorTests PRIVATE /MP)
ENDIF()

# Enable warning level
IF(MSVC)
    TARGET_COMPILE_OPTIONS(MemoryAllocatorTests PRIVATE /W4)
ELSE()
    TARGET_COMPILE_OPTIONS(MemoryAllocatorTests PRIVATE -Wall -Wextra -Wpedantic)
ENDIF()

# Register with CTest
catch_discover_tests(MemoryAllocatorTests)

# --------------------------------
# File Format Tester
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating edX Format Tester")

ADD_EXECUTABLE(FormatTester
    file_formats/main.cpp
    file_formats/FileBrowser.cpp
    file_formats/FileBrowser.hpp
    file_formats/FormatTester.rc
    file_formats/resource.h
    ${CMAKE_SOURCE_DIR}/source/edX/edXProjectFile.h
)

TARGET_LINK_LIBRARIES(FormatTester PRIVATE
    glm::glm-header-only
    spdlog::spdlog_header_only
    glfw
    dxgi
    d3d12
    Shell32.lib
)

TARGET_INCLUDE_DIRECTORIES(FormatTester PRIVATE
    ${CMAKE_SOURCE_DIR}/dependency/imgui
    ${CMAKE_SOURCE_DIR}/dependency/imgui/backends
    ${CMAKE_SOURCE_DIR}/dependency/imgui/misc
    ${CMAKE_SOURCE_DIR}/dependency/ImGuizmo
    ${CMAKE_SOURCE_DIR}/source
    ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
    $ENV{VULKAN_SDK}/Include/
    ${Stb_INCLUDE_DIR}
)

# --------------------------------
# Defines
# --------------------------------
TARGET_COMPILE_DEFINITIONS(FormatTester PRIVATE WIN32_LEAN_AND_MEAN)

# --------------------------------
# Setting Config Tests
# --------------------------------

# Define the test executable
ADD_EXECUTABLE(SettingsTest
    settings_tests/SettingsTest.cpp
)

# Include directories
TARGET_INCLUDE_DIRECTORIES(SettingsTest PRIVATE
    ${CMAKE_SOURCE_DIR}/source
)

# Link with Catch2 and libconfig libraries
TARGET_LINK_LIBRARIES(SettingsTest PRIVATE
    Catch2::Catch2WithMain
    config++
    dxgi
    d3d12
    Shell32.lib
)

# Enable parallel compilation for MSVC
IF(MSVC)
    TARGET_COMPILE_OPTIONS(SettingsTest PRIVATE /MP)
ENDIF()

# Enable warning level
IF(MSVC)
    TARGET_COMPILE_OPTIONS(SettingsTest PRIVATE /W4)
ELSE()
    TARGET_COMPILE_OPTIONS(SettingsTest PRIVATE -Wall -Wextra -Wpedantic)
ENDIF()

# Register with CTest
INCLUDE(CTest)
INCLUDE(Catch)
catch_discover_tests(SettingsTest)

# --------------------------------
# edX Format Tests
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating edX Format Tests")

ADD_EXECUTABLE(EdxTests
    edx_tests/EdxTestMain.cpp
    edx_tests/EdxProjectFileTest.cpp
    edx_tests/EdxLibraryFileTest.cpp
    edx_tests/EdxManagerTest.cpp
    edx_tests/EdxProjectFileComprehensiveTest.cpp
)

# Include directories
TARGET_INCLUDE_DIRECTORIES(EdxTests PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/dependency
)

# Link with Catch2 and other dependencies
TARGET_LINK_LIBRARIES(EdxTests PRIVATE
    Catch2::Catch2WithMain
    edX
)

# Enable parallel compilation for MSVC
IF(MSVC)
    TARGET_COMPILE_OPTIONS(EdxTests PRIVATE /MP)
ENDIF()

# Enable warning level
IF(MSVC)
    TARGET_COMPILE_OPTIONS(EdxTests PRIVATE /W4)
ELSE()
    TARGET_COMPILE_OPTIONS(EdxTests PRIVATE -Wall -Wextra -Wpedantic)
ENDIF()

# Register with CTest
catch_discover_tests(EdxTests)

# --------------------------------
# edX Demo Generator (separate executable)
# --------------------------------

ADD_EXECUTABLE(EdxDemoGenerator
    edx_tests/EdxDemoGenerator.cpp
)

# Include directories
TARGET_INCLUDE_DIRECTORIES(EdxDemoGenerator PRIVATE
    ${CMAKE_SOURCE_DIR}/source
    ${CMAKE_SOURCE_DIR}/dependency
)

# Link with edX library
TARGET_LINK_LIBRARIES(EdxDemoGenerator PRIVATE
    edX
)

# Enable parallel compilation for MSVC
IF(MSVC)
    TARGET_COMPILE_OPTIONS(EdxDemoGenerator PRIVATE /MP)
ENDIF()

# Enable warning level
IF(MSVC)
    TARGET_COMPILE_OPTIONS(EdxDemoGenerator PRIVATE /W4)
ELSE()
    TARGET_COMPILE_OPTIONS(EdxDemoGenerator PRIVATE -Wall -Wextra -Wpedantic)
ENDIF()

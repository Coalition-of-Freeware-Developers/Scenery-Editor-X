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
)
# Disable engine logging in this target; profiling (Tracy) simply not defined here
TARGET_COMPILE_DEFINITIONS(RefTests PRIVATE SEDX_NO_LOGGING ZoneScoped=)

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

# -------------------------------------------------------
# Optional Vulkan smoke tests (disabled by default)
# -------------------------------------------------------
OPTION(SEDX_ENABLE_VULKAN_SMOKE_TESTS "Build Vulkan smoke tests (requires engine includes)" OFF)
IF(SEDX_ENABLE_VULKAN_SMOKE_TESTS)
    ADD_EXECUTABLE(VulkanSmokeTests
        vulkan_tests/CommandPoolSmoke.cpp
        vulkan_tests/BufferSmoke.cpp
        vulkan_tests/ImageSmoke.cpp
        vulkan_tests/AppStub.cpp
    )

    TARGET_INCLUDE_DIRECTORIES(VulkanSmokeTests PRIVATE
        ${CMAKE_SOURCE_DIR}/source
        ${CMAKE_SOURCE_DIR}/dependency
        ${CMAKE_SOURCE_DIR}/dependency/entt/src
        ${CMAKE_SOURCE_DIR}/dependency/imgui
        ${CMAKE_SOURCE_DIR}/dependency/imgui/backends
        ${CMAKE_SOURCE_DIR}/dependency/imgui/misc
        ${CMAKE_SOURCE_DIR}/dependency/xMath
        $ENV{VULKAN_SDK}/Include/
		$ENV{VULKAN_SDK}/Include/vma
        ${Stb_INCLUDE_DIR}
    )

    TARGET_LINK_LIBRARIES(VulkanSmokeTests PRIVATE
        Catch2::Catch2WithMain
        Vulkan::Vulkan
        spdlog::spdlog_header_only
        imgui
        AppCore
        xMath
    )

    IF(MSVC)
        TARGET_COMPILE_OPTIONS(VulkanSmokeTests PRIVATE /MP /W4)
    ELSE()
        TARGET_COMPILE_OPTIONS(VulkanSmokeTests PRIVATE -Wall -Wextra -Wpedantic)
    ENDIF()

    INCLUDE(Catch)
    # On Windows, copy dependent DLLs (like xMath) next to the test exe so discovery can run
    if(WIN32)
        add_custom_command(TARGET VulkanSmokeTests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "[VulkanSmokeTests] Copying runtime DLLs to test output dir"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:xMath::xMath> $<TARGET_FILE_DIR:VulkanSmokeTests>
        )
    endif()

    # Prefix test names so filtering with -R VulkanSmokeTests works in CTest
    catch_discover_tests(
        VulkanSmokeTests
        TEST_PREFIX "VulkanSmokeTests::"
        TEST_PROPERTIES LABELS "vulkan;smoke"
    )

    # Ensure the Vulkan smoke tests run in a controlled, headless mode under CTest
    # and only when explicitly opted in. We set environment variables for the tests
    # so they don't try to create WSI surfaces and so they actually execute instead of skipping.
    if(VulkanSmokeTests_TESTS)
        foreach(test_name IN LISTS VulkanSmokeTests_TESTS)
            set_tests_properties("${test_name}" PROPERTIES
                ENVIRONMENT "SEDX_RUN_VULKAN_SMOKE_TESTS=1;SEDX_HEADLESS=1"
            )
        endforeach()
    endif()
ENDIF()

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
# No Tracy undef needed; not defined for this target

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

TARGET_COMPILE_DEFINITIONS(MemoryAllocatorTests PRIVATE VK_USE_64_BIT_NON_DISPATCHABLE_HANDLES)

# Register with CTest
catch_discover_tests(MemoryAllocatorTests)

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
# Disable engine logging; omit Tracy
TARGET_COMPILE_DEFINITIONS(SettingsTest PRIVATE SEDX_NO_LOGGING ZoneScoped=)

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
# Conversion Tests
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating Conversion Unit Tests")

FILE(GLOB CONVERSION_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/conversion_tests/*.cpp
)

ADD_EXECUTABLE(ConversionTests
    ${CONVERSION_TEST_SOURCES}
)

TARGET_INCLUDE_DIRECTORIES(ConversionTests PRIVATE
    ${CMAKE_SOURCE_DIR}/source
)

TARGET_LINK_LIBRARIES(ConversionTests PRIVATE
    Catch2::Catch2WithMain
)

IF(MSVC)
    TARGET_COMPILE_OPTIONS(ConversionTests PRIVATE /MP /W4)
ELSE()
    TARGET_COMPILE_OPTIONS(ConversionTests PRIVATE -Wall -Wextra -Wpedantic)
ENDIF()

INCLUDE(Catch)
catch_discover_tests(ConversionTests)


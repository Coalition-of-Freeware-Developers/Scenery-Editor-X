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

# Determine MSVC-like behavior: prefer SEDX_PREFERRED_COMPILER when available,
# otherwise fall back to the built-in CMake `MSVC` variable.
if(DEFINED SEDX_PREFERRED_COMPILER)
    if(SEDX_PREFERRED_COMPILER STREQUAL "MSVC" OR SEDX_PREFERRED_COMPILER STREQUAL "CLANG_CL")
        set(_SEDX_MSVCLIKE TRUE)
    else()
        set(_SEDX_MSVCLIKE FALSE)
    endif()
else()
    if(MSVC)
        set(_SEDX_MSVCLIKE TRUE)
    else()
        set(_SEDX_MSVCLIKE FALSE)
    endif()
endif()

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

# Enable parallel compilation for MSVC-like compilers
IF(_SEDX_MSVCLIKE)
    TARGET_COMPILE_OPTIONS(RefTests PRIVATE /MP)
ENDIF()

# Enable warning level
IF(_SEDX_MSVCLIKE)
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

    IF(_SEDX_MSVCLIKE)
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
    IF(_SEDX_MSVCLIKE)
        TARGET_COMPILE_OPTIONS(MemoryAllocatorTests PRIVATE /MP)
    ENDIF()

# Enable warning level
IF(_SEDX_MSVCLIKE)
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

# Define the test executable: ConfigFile (the settings files) over the VFS, standard library + libconfig only
ADD_EXECUTABLE(SettingsTest
    settings_tests/SettingsTest.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/settings/config_file.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/filesystem/vfs.cpp
)

# Include directories
TARGET_INCLUDE_DIRECTORIES(SettingsTest PRIVATE
    ${CMAKE_SOURCE_DIR}/source
)

# Link with Catch2 and libconfig++ (named config++ outside MSVC)
IF(TARGET libconfig++)
    SET(_SEDX_LIBCONFIGXX libconfig++)
ELSE()
    SET(_SEDX_LIBCONFIGXX config++)
ENDIF()
TARGET_LINK_LIBRARIES(SettingsTest PRIVATE
    Catch2::Catch2WithMain
    ${_SEDX_LIBCONFIGXX}
)

# Enable parallel compilation for MSVC-like compilers
IF(_SEDX_MSVCLIKE)
    TARGET_COMPILE_OPTIONS(SettingsTest PRIVATE /MP)
ENDIF()

# Enable warning level
IF(_SEDX_MSVCLIKE)
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

IF(_SEDX_MSVCLIKE)
    TARGET_COMPILE_OPTIONS(ConversionTests PRIVATE /MP /W4)
ELSE()
    TARGET_COMPILE_OPTIONS(ConversionTests PRIVATE -Wall -Wextra -Wpedantic)
ENDIF()

INCLUDE(Catch)
catch_discover_tests(ConversionTests)

# --------------------------------
# X-Plane Format Tests (OBJ8, library.txt, scenery_packs.ini, apt.dat, airport index; standard library only)
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating X-Plane Format Unit Tests")

ADD_EXECUTABLE(XPlaneFormatTests
    ${CMAKE_CURRENT_SOURCE_DIR}/xplane_tests/XPlaneFormatTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xplane_tests/AirportTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xplane_tests/OverlayTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xplane_tests/DsfWriterTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xplane_tests/FacadeTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xplane_tests/FacadeWriterTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xplane_tests/InstallFinderTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xplane_tests/BrdfLutTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xplane_tests/ThumbnailTests.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/environment/brdf_lut.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/filesystem/vfs.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_obj8.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_library.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_lights.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_apt.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_apt_geometry.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_airport_index.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_gateway_types.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_dsf.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_assets.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_facade.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_facade_writer.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_facade_authoring.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_overlay.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_install_finder.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_thumbnail.cpp
)

TARGET_INCLUDE_DIRECTORIES(XPlaneFormatTests PRIVATE
    ${CMAKE_SOURCE_DIR}/source
)

TARGET_LINK_LIBRARIES(XPlaneFormatTests PRIVATE
    Catch2::Catch2WithMain
    nlohmann_json::nlohmann_json
)

IF(WIN32)
    # xp_install_finder reads the registry and the desktop folder
    TARGET_LINK_LIBRARIES(XPlaneFormatTests PRIVATE advapi32 shell32)
ENDIF()

IF(_SEDX_MSVCLIKE)
    TARGET_COMPILE_OPTIONS(XPlaneFormatTests PRIVATE /MP /W4)
ELSE()
    TARGET_COMPILE_OPTIONS(XPlaneFormatTests PRIVATE -Wall -Wextra -Wpedantic)
ENDIF()

INCLUDE(Catch)
catch_discover_tests(XPlaneFormatTests)

# --------------------------------
# Project File Tests (.edX JSON, .sex binary, WorldEditor earth.wed.xml, cache, document, DSF conversion, the VFS and
# scenery pack creation / export; standard library only)
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating Project File Unit Tests")

ADD_EXECUTABLE(ProjectFormatTests
    ${CMAKE_CURRENT_SOURCE_DIR}/project_tests/ProjectFormatTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/project_tests/SplineEditTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/project_tests/VfsTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/project_tests/SceneryPackTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/project_tests/FacadeEditTests.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/filesystem/vfs.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/project/spline_edit.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/project/facade_edit.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/project/scenery_project.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/project/project_edx.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/project/project_sex.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/project/project_wed.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/project/xml_document.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/project/project_io.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/project/project_cache.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/project/project_document.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/project/project_dsf.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/project/scenery_pack.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_airport_document.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_airport_index.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_apt.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_library.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_apt_geometry.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_dsf.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_assets.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_facade.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_overlay.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/xplane/xp_install_finder.cpp
)

TARGET_INCLUDE_DIRECTORIES(ProjectFormatTests PRIVATE
    ${CMAKE_SOURCE_DIR}/source
)

TARGET_LINK_LIBRARIES(ProjectFormatTests PRIVATE
    Catch2::Catch2WithMain
    nlohmann_json::nlohmann_json
)

IF(_SEDX_MSVCLIKE)
    TARGET_COMPILE_OPTIONS(ProjectFormatTests PRIVATE /MP /W4 /bigobj)
ELSE()
    TARGET_COMPILE_OPTIONS(ProjectFormatTests PRIVATE -Wall -Wextra -Wpedantic)
ENDIF()

catch_discover_tests(ProjectFormatTests)


# --------------------------------
# Environment Tests (the XP12 environment model: parity scenarios, solar position, weather, seasons, quality tables;
# standard library + nlohmann_json only)
# --------------------------------

MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating Environment Unit Tests")

ADD_EXECUTABLE(EnvironmentTests
    ${CMAKE_CURRENT_SOURCE_DIR}/environment_tests/ParityScenarioTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/environment_tests/EnvironmentTests.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/environment/parity_scenario.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/environment/solar_position.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/environment/surface_weather.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/environment/season_model.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/environment/sky_conditions.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/environment/environment_state.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/environment/environment.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/environment/sky_light.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/environment/atmosphere_params.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/renderer/renderer_quality.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/renderer/renderer_features.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/renderer/exposure.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/renderer/env_probes.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/renderer/csm.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/renderer/cacao.cpp
    ${CMAKE_SOURCE_DIR}/source/SceneryEditorX/renderer/ssr.cpp
)

TARGET_INCLUDE_DIRECTORIES(EnvironmentTests PRIVATE
    ${CMAKE_SOURCE_DIR}/source
)

TARGET_LINK_LIBRARIES(EnvironmentTests PRIVATE
    Catch2::Catch2WithMain
    nlohmann_json::nlohmann_json
)

IF(_SEDX_MSVCLIKE)
    TARGET_COMPILE_OPTIONS(EnvironmentTests PRIVATE /MP /W4)
ELSE()
    TARGET_COMPILE_OPTIONS(EnvironmentTests PRIVATE -Wall -Wextra -Wpedantic)
ENDIF()

catch_discover_tests(EnvironmentTests)

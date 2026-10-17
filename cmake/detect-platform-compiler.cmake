## Early platform and compiler detection
# This file runs before toolchain selection to set conservative defaults
# and assist vcpkg in choosing appropriate triplets/flags when possible.

# Respect user override via -DSEDX_PREFERRED_COMPILER=MSVC|CLANG|GCC
if(NOT DEFINED SEDX_PREFERRED_COMPILER)
    if(DEFINED ENV{SEDX_PREFERRED_COMPILER})
        set(SEDX_PREFERRED_COMPILER $ENV{SEDX_PREFERRED_COMPILER} CACHE STRING "Preferred compiler (MSVC, CLANG, GCC)" FORCE)
    endif()
endif()

if(NOT DEFINED SEDX_PREFERRED_COMPILER)
    # Try to infer from explicit environment hints first
    if(DEFINED ENV{CXX})
        string(TOLOWER "$ENV{CXX}" _cxx_lower)
        if(_cxx_lower MATCHES "clang-cl" OR _cxx_lower MATCHES "clang-cl.exe")
            set(SEDX_PREFERRED_COMPILER "CLANG_CL" CACHE STRING "Preferred compiler (inferred from CXX)" FORCE)
        elseif(_cxx_lower MATCHES "clang" OR _cxx_lower MATCHES "clang.exe")
            set(SEDX_PREFERRED_COMPILER "CLANG" CACHE STRING "Preferred compiler (inferred from CXX)" FORCE)
        elseif(_cxx_lower MATCHES "g\+\+" OR _cxx_lower MATCHES "g++.exe" OR _cxx_lower MATCHES "gcc")
            set(SEDX_PREFERRED_COMPILER "GCC" CACHE STRING "Preferred compiler (inferred from CXX)" FORCE)
        endif()
    endif()

    # If still unknown, fall back to generator / platform heuristics
    if(NOT DEFINED SEDX_PREFERRED_COMPILER)
        if(DEFINED CMAKE_GENERATOR)
            string(TOUPPER "${CMAKE_GENERATOR}" _gen_upper)
            if(_gen_upper MATCHES "VISUAL STUDIO|MSBUILD|NMAKE")
                set(SEDX_PREFERRED_COMPILER "MSVC" CACHE STRING "Preferred compiler (inferred)" FORCE)
            elseif(_gen_upper MATCHES "NINJA")
                if(APPLE)
                    set(SEDX_PREFERRED_COMPILER "CLANG" CACHE STRING "Preferred compiler (inferred)" FORCE)
                elseif(UNIX)
                    set(SEDX_PREFERRED_COMPILER "GCC" CACHE STRING "Preferred compiler (inferred)" FORCE)
                else()
                    # Windows + Ninja: prefer clang-cl if user provided CXX hint above, otherwise prefer MSVC by default
                    set(SEDX_PREFERRED_COMPILER "MSVC" CACHE STRING "Preferred compiler (inferred)" FORCE)
                endif()
            else()
                if(WIN32)
                    set(SEDX_PREFERRED_COMPILER "MSVC" CACHE STRING "Preferred compiler (fallback)" FORCE)
                elseif(APPLE)
                    set(SEDX_PREFERRED_COMPILER "CLANG" CACHE STRING "Preferred compiler (fallback)" FORCE)
                else()
                    set(SEDX_PREFERRED_COMPILER "GCC" CACHE STRING "Preferred compiler (fallback)" FORCE)
                endif()
            endif()
        else()
            if(WIN32)
                set(SEDX_PREFERRED_COMPILER "MSVC" CACHE STRING "Preferred compiler (platform fallback)" FORCE)
            elseif(APPLE)
                set(SEDX_PREFERRED_COMPILER "CLANG" CACHE STRING "Preferred compiler (platform fallback)" FORCE)
            else()
                set(SEDX_PREFERRED_COMPILER "GCC" CACHE STRING "Preferred compiler (platform fallback)" FORCE)
            endif()
        endif()
    endif()
endif()

# Set a conservative vcpkg triplet if not already defined by the user or presets
if(NOT DEFINED VCPKG_TARGET_TRIPLET)
    if(SEDX_PREFERRED_COMPILER STREQUAL "MSVC" OR SEDX_PREFERRED_COMPILER STREQUAL "CLANG_CL")
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(DEFAULT_VCPKG_TRIPLET "x64-windows")
        else()
            set(DEFAULT_VCPKG_TRIPLET "x86-windows")
        endif()
    elseif(SEDX_PREFERRED_COMPILER STREQUAL "CLANG")
        if(APPLE)
            if(CMAKE_SIZEOF_VOID_P EQUAL 8)
                set(DEFAULT_VCPKG_TRIPLET "x64-osx")
            else()
                set(DEFAULT_VCPKG_TRIPLET "arm64-osx")
            endif()
        else()
            # On Windows clang that is not clang-cl is likely MinGW-style (use mingw triplet)
            if(WIN32)
                set(DEFAULT_VCPKG_TRIPLET "x64-mingw")
            else()
                set(DEFAULT_VCPKG_TRIPLET "x64-linux")
            endif()
        endif()
    else()
        # GCC default
        set(DEFAULT_VCPKG_TRIPLET "x64-linux")
    endif()
    set(VCPKG_TARGET_TRIPLET "${DEFAULT_VCPKG_TRIPLET}" CACHE STRING "Default vcpkg target triplet inferred from platform/compiler" FORCE)
endif()

# Expose a human-readable summary early in configuration
message(STATUS "SEDX: Preferred compiler -> ${SEDX_PREFERRED_COMPILER}")
message(STATUS "SEDX: vcpkg target triplet -> ${VCPKG_TARGET_TRIPLET}")

# Provide minimal per-compiler default flags if user hasn't set them. These are conservative
# and intended to give consistent behavior when vcpkg or vendor scripts examine cached flags.
if(NOT DEFINED SEDX_DEFAULT_CXX_FLAGS)
    if(SEDX_PREFERRED_COMPILER STREQUAL "MSVC")
        set(SEDX_DEFAULT_CXX_FLAGS "/EHsc /GR" CACHE STRING "Default CXX flags for MSVC" FORCE)
    elseif(SEDX_PREFERRED_COMPILER STREQUAL "CLANG_CL")
        # clang-cl accepts MSVC-style flags; prefer MSVC-compatible defaults
        set(SEDX_DEFAULT_CXX_FLAGS "/EHsc /GR" CACHE STRING "Default CXX flags for clang-cl" FORCE)
    elseif(SEDX_PREFERRED_COMPILER STREQUAL "CLANG")
        set(SEDX_DEFAULT_CXX_FLAGS "-std=gnu++20" CACHE STRING "Default CXX flags for Clang" FORCE)
    else()
        set(SEDX_DEFAULT_CXX_FLAGS "-std=gnu++20" CACHE STRING "Default CXX flags for GCC" FORCE)
    endif()
endif()

# Optionally set cache variables that some vendor scripts check early
if(NOT DEFINED CMAKE_CXX_FLAGS_INIT)
    set(CMAKE_CXX_FLAGS_INIT "${SEDX_DEFAULT_CXX_FLAGS}" CACHE STRING "Initial CXX flags (inferred)" FORCE)
endif()

# Allow downstream code to detect we ran early detection
set(SEDX_PLATFORM_DETECTION_RAN TRUE CACHE INTERNAL "Set when detect-platform-compiler.cmake ran")

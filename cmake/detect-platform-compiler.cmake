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
                if(CMAKE_HOST_APPLE)
                    set(SEDX_PREFERRED_COMPILER "CLANG" CACHE STRING "Preferred compiler (inferred)" FORCE)
                elseif(CMAKE_HOST_UNIX)
                    set(SEDX_PREFERRED_COMPILER "GCC" CACHE STRING "Preferred compiler (inferred)" FORCE)
                else()
                    # Windows + Ninja: prefer clang-cl if user provided CXX hint above, otherwise prefer MSVC by default
                    set(SEDX_PREFERRED_COMPILER "MSVC" CACHE STRING "Preferred compiler (inferred)" FORCE)
                endif()
            else()
                if(CMAKE_HOST_WIN32)
                    set(SEDX_PREFERRED_COMPILER "MSVC" CACHE STRING "Preferred compiler (fallback)" FORCE)
                elseif(CMAKE_HOST_APPLE)
                    set(SEDX_PREFERRED_COMPILER "CLANG" CACHE STRING "Preferred compiler (fallback)" FORCE)
                else()
                    set(SEDX_PREFERRED_COMPILER "GCC" CACHE STRING "Preferred compiler (fallback)" FORCE)
                endif()
            endif()
        else()
            if(CMAKE_HOST_WIN32)
                set(SEDX_PREFERRED_COMPILER "MSVC" CACHE STRING "Preferred compiler (platform fallback)" FORCE)
            elseif(CMAKE_HOST_APPLE)
                set(SEDX_PREFERRED_COMPILER "CLANG" CACHE STRING "Preferred compiler (platform fallback)" FORCE)
            else()
                set(SEDX_PREFERRED_COMPILER "GCC" CACHE STRING "Preferred compiler (platform fallback)" FORCE)
            endif()
        endif()
    endif()
endif()

# Set a vcpkg triplet if the user or a preset didn't.
# This file runs before project(), so CMAKE_SIZEOF_VOID_P is not known yet: testing it here picked
# the 32-bit x86-windows triplet for every 64-bit Windows build (vcpkg then built all its ports for
# 32-bit and find_package(Freetype) rejected them). The target architecture comes from what the
# generator and the environment say instead, as in vcpkg's own toolchain. Platform tests here use the
# CMAKE_HOST_* variables: WIN32 / APPLE / UNIX only describe the target once project() has run, so this
# assumes the build targets the host OS (cross builds pass -DVCPKG_TARGET_TRIPLET, as vcpkg also expects).
if(NOT DEFINED VCPKG_TARGET_TRIPLET)
    if(CMAKE_GENERATOR_PLATFORM)
        # Visual Studio generators: -A Win32 | x64 | ARM64
        set(_sedx_arch "${CMAKE_GENERATOR_PLATFORM}")
    elseif(CMAKE_HOST_APPLE AND CMAKE_OSX_ARCHITECTURES)
        set(_sedx_arch "${CMAKE_OSX_ARCHITECTURES}")
        if(CMAKE_OSX_ARCHITECTURES MATCHES ";")
            # A vcpkg triplet holds one architecture: the libraries come out for the first one only
            list(GET CMAKE_OSX_ARCHITECTURES 0 _sedx_arch)
            message(WARNING "SEDX: CMAKE_OSX_ARCHITECTURES is \"${CMAKE_OSX_ARCHITECTURES}\" (a universal build), "
                "but a vcpkg triplet holds one architecture: vcpkg's libraries are built for ${_sedx_arch} only. "
                "Pass -DVCPKG_TARGET_TRIPLET (and one architecture) to choose.")
        endif()
    elseif(CMAKE_HOST_WIN32 AND DEFINED ENV{VSCMD_ARG_TGT_ARCH})
        # Ninja / NMake from a Visual Studio developer prompt
        set(_sedx_arch "$ENV{VSCMD_ARG_TGT_ARCH}")
    else()
        # Otherwise the build targets the machine it runs on (AMD64, x86_64, arm64, aarch64...)
        cmake_host_system_information(RESULT _sedx_arch QUERY OS_PLATFORM)
    endif()
    string(TOLOWER "${_sedx_arch}" _sedx_arch)
    if(_sedx_arch MATCHES "^(arm64|aarch64)")
        set(_sedx_arch "arm64")
    elseif(_sedx_arch MATCHES "^arm")
        # 32-bit ARM: armv7l, armv6l, ARM
        set(_sedx_arch "arm")
    elseif(_sedx_arch MATCHES "^(win32|x86|i[3-6]86)$")
        set(_sedx_arch "x86")
    else()
        set(_sedx_arch "x64")
    endif()

    if(SEDX_PREFERRED_COMPILER STREQUAL "MSVC" OR SEDX_PREFERRED_COMPILER STREQUAL "CLANG_CL")
        set(_sedx_triplet_os "windows")
    elseif(CMAKE_HOST_APPLE)
        set(_sedx_triplet_os "osx")
    elseif(CMAKE_HOST_WIN32)
        # clang or gcc on Windows that is not clang-cl: MinGW
        set(_sedx_triplet_os "mingw-dynamic")
    else()
        set(_sedx_triplet_os "linux")
    endif()
    set(VCPKG_TARGET_TRIPLET "${_sedx_arch}-${_sedx_triplet_os}" CACHE STRING "Default vcpkg target triplet inferred from platform/compiler" FORCE)
    unset(_sedx_arch)
    unset(_sedx_triplet_os)
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

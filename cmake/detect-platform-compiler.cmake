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
        # "g\\+\\+" reaches the regex as g\+\+ (a literal "g++"); "g\+\+" reached it as g++, which
        # failed to compile, so CXX=g++ stopped the configure
        elseif(_cxx_lower MATCHES "g\\+\\+|gcc")
            set(SEDX_PREFERRED_COMPILER "GCC" CACHE STRING "Preferred compiler (inferred from CXX)" FORCE)
        endif()
    endif()

    # An MSYS2 MinGW-family shell (MSYSTEM UCRT64, MINGW64, MINGW32, CLANG64, CLANGARM64) puts that
    # environment's gcc or clang on the PATH, and a Ninja or Makefiles configure finds it there. Not in a
    # Visual Studio developer prompt, which brings cl.exe.
    set(_sedx_msys2_compiler "")
    if(CMAKE_HOST_WIN32 AND DEFINED ENV{MSYSTEM} AND NOT DEFINED ENV{VSCMD_ARG_TGT_ARCH})
        if("$ENV{MSYSTEM}" MATCHES "^CLANG")
            set(_sedx_msys2_compiler "CLANG")
        elseif("$ENV{MSYSTEM}" MATCHES "^(MINGW|UCRT)")
            set(_sedx_msys2_compiler "GCC")
        endif()
    endif()

    # If still unknown, fall back to generator / platform heuristics
    if(NOT DEFINED SEDX_PREFERRED_COMPILER)
        if(DEFINED CMAKE_GENERATOR)
            string(TOUPPER "${CMAKE_GENERATOR}" _gen_upper)
            if(_gen_upper MATCHES "VISUAL STUDIO|MSBUILD|NMAKE")
                set(SEDX_PREFERRED_COMPILER "MSVC" CACHE STRING "Preferred compiler (inferred)" FORCE)
            elseif(_gen_upper MATCHES "MINGW MAKEFILES|MSYS MAKEFILES")
                if(_sedx_msys2_compiler STREQUAL "CLANG")
                    set(SEDX_PREFERRED_COMPILER "CLANG" CACHE STRING "Preferred compiler (inferred)" FORCE)
                else()
                    set(SEDX_PREFERRED_COMPILER "GCC" CACHE STRING "Preferred compiler (inferred)" FORCE)
                endif()
            elseif(_gen_upper MATCHES "NINJA")
                if(CMAKE_HOST_APPLE)
                    set(SEDX_PREFERRED_COMPILER "CLANG" CACHE STRING "Preferred compiler (inferred)" FORCE)
                elseif(CMAKE_HOST_UNIX)
                    set(SEDX_PREFERRED_COMPILER "GCC" CACHE STRING "Preferred compiler (inferred)" FORCE)
                elseif(_sedx_msys2_compiler)
                    set(SEDX_PREFERRED_COMPILER "${_sedx_msys2_compiler}" CACHE STRING "Preferred compiler (MSYS2 shell)" FORCE)
                else()
                    # Windows + Ninja: prefer clang-cl if user provided CXX hint above, otherwise prefer MSVC by default
                    set(SEDX_PREFERRED_COMPILER "MSVC" CACHE STRING "Preferred compiler (inferred)" FORCE)
                endif()
            else()
                if(CMAKE_HOST_WIN32 AND _sedx_msys2_compiler)
                    set(SEDX_PREFERRED_COMPILER "${_sedx_msys2_compiler}" CACHE STRING "Preferred compiler (MSYS2 shell)" FORCE)
                elseif(CMAKE_HOST_WIN32)
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
    unset(_sedx_msys2_compiler)
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
        set(_sedx_raw_arch "${CMAKE_GENERATOR_PLATFORM}")
    elseif(CMAKE_HOST_APPLE AND CMAKE_OSX_ARCHITECTURES)
        set(_sedx_raw_arch "${CMAKE_OSX_ARCHITECTURES}")
        if(CMAKE_OSX_ARCHITECTURES MATCHES ";")
            # A vcpkg triplet holds one architecture: the libraries come out for the first one only
            list(GET CMAKE_OSX_ARCHITECTURES 0 _sedx_raw_arch)
            message(WARNING "SEDX: CMAKE_OSX_ARCHITECTURES is \"${CMAKE_OSX_ARCHITECTURES}\" (a universal build), "
                "but a vcpkg triplet holds one architecture: vcpkg's libraries are built for ${_sedx_raw_arch} only. "
                "Pass -DVCPKG_TARGET_TRIPLET (and one architecture) to choose.")
        endif()
    elseif(CMAKE_HOST_WIN32 AND DEFINED ENV{VSCMD_ARG_TGT_ARCH} AND NOT CMAKE_GENERATOR MATCHES "^Visual Studio")
        # Ninja / NMake / Makefiles from a Visual Studio developer prompt. A Visual Studio generator ignores the
        # prompt and builds for the host without -A, so it takes the host's architecture below.
        set(_sedx_raw_arch "$ENV{VSCMD_ARG_TGT_ARCH}")
    elseif(CMAKE_HOST_WIN32 AND DEFINED ENV{MSYSTEM_CARCH} AND SEDX_PREFERRED_COMPILER MATCHES "^(GCC|CLANG)$")
        # An MSYS2 MinGW-family shell: the architecture of its compiler (x86_64, i686, aarch64)
        set(_sedx_raw_arch "$ENV{MSYSTEM_CARCH}")
    else()
        # Otherwise the build targets the machine it runs on (AMD64, x86_64, arm64, aarch64...)
        cmake_host_system_information(RESULT _sedx_raw_arch QUERY OS_PLATFORM)
    endif()
    # vcpkg's names for the architectures. Others (ppc64le, s390x, riscv64, loongarch64...) are already
    # vcpkg's names; the triplet check below stops on any that vcpkg doesn't have.
    string(TOLOWER "${_sedx_raw_arch}" _sedx_arch)
    if(_sedx_arch MATCHES "^(amd64|x86_64|x64)$")
        set(_sedx_arch "x64")
    elseif(_sedx_arch STREQUAL "arm64ec")
        set(_sedx_arch "arm64ec")
    elseif(_sedx_arch MATCHES "^(arm64|aarch64)")
        set(_sedx_arch "arm64")
    elseif(_sedx_arch MATCHES "^arm")
        # 32-bit ARM: armv7l, armv6l, ARM
        set(_sedx_arch "arm")
    elseif(_sedx_arch MATCHES "^(win32|x86|i[3-6]86)$")
        set(_sedx_arch "x86")
    endif()

    # MSYS2's own cmake (not its UCRT64 / MinGW one) and Cygwin's report a UNIX host, but the build is
    # still for Windows
    if(CMAKE_HOST_WIN32 OR CMAKE_HOST_SYSTEM_NAME MATCHES "^(MSYS|CYGWIN)")
        set(_sedx_host_windows TRUE)
    else()
        set(_sedx_host_windows FALSE)
    endif()
    if(SEDX_PREFERRED_COMPILER STREQUAL "MSVC" OR SEDX_PREFERRED_COMPILER STREQUAL "CLANG_CL")
        set(_sedx_triplet_os "windows")
    elseif(CMAKE_HOST_APPLE)
        set(_sedx_triplet_os "osx")
    elseif(_sedx_host_windows AND SEDX_PREFERRED_COMPILER STREQUAL "CLANG" AND DEFINED ENV{VSCMD_ARG_TGT_ARCH})
        # clang (not clang-cl) from a Visual Studio developer prompt: LLVM's clang, MSVC ABI
        set(_sedx_triplet_os "windows")
    elseif(_sedx_host_windows)
        # gcc, or clang outside a Visual Studio developer prompt, on Windows: MinGW
        set(_sedx_triplet_os "mingw-dynamic")
    else()
        set(_sedx_triplet_os "linux")
    endif()
    set(_sedx_triplet "${_sedx_arch}-${_sedx_triplet_os}")

    # Stop here, not in vcpkg, on a combination vcpkg has no triplet for (x86-osx, an architecture vcpkg
    # doesn't know...). Skipped without the vcpkg submodule or with overlay triplets, which can add names.
    if(DEFINED VCPKG_ROOT)
        set(_sedx_vcpkg_root "${VCPKG_ROOT}")
    else()
        set(_sedx_vcpkg_root "${CMAKE_CURRENT_LIST_DIR}/../dependency/vcpkg")
    endif()
    if(EXISTS "${_sedx_vcpkg_root}/triplets" AND NOT VCPKG_OVERLAY_TRIPLETS
            AND NOT EXISTS "${_sedx_vcpkg_root}/triplets/${_sedx_triplet}.cmake"
            AND NOT EXISTS "${_sedx_vcpkg_root}/triplets/community/${_sedx_triplet}.cmake")
        message(FATAL_ERROR "SEDX: no vcpkg triplet fits this build: ${_sedx_triplet} (from the architecture "
            "\"${_sedx_raw_arch}\" and the ${SEDX_PREFERRED_COMPILER} compiler) is not one of vcpkg's triplets. "
            "Pass one with -DVCPKG_TARGET_TRIPLET=<triplet> (see ${_sedx_vcpkg_root}/triplets).")
    endif()

    set(VCPKG_TARGET_TRIPLET "${_sedx_triplet}" CACHE STRING "Default vcpkg target triplet inferred from platform/compiler" FORCE)
    set(_sedx_triplet_note " (guessed from the platform and compiler; -DVCPKG_TARGET_TRIPLET=<triplet> chooses another)")
    unset(_sedx_raw_arch)
    unset(_sedx_arch)
    unset(_sedx_triplet_os)
    unset(_sedx_host_windows)
    unset(_sedx_triplet)
    unset(_sedx_vcpkg_root)
endif()

# Expose a human-readable summary early in configuration
message(STATUS "SEDX: Preferred compiler -> ${SEDX_PREFERRED_COMPILER}")
message(STATUS "SEDX: vcpkg target triplet -> ${VCPKG_TARGET_TRIPLET}${_sedx_triplet_note}")
unset(_sedx_triplet_note)

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

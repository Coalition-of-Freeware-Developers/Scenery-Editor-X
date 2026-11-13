# Checks the vcpkg triplet cmake/detect-platform-compiler.cmake picks when neither the user nor a preset
# set one: the code that chose the 32-bit x86-windows triplet for every 64-bit Windows build until PR 91.
# Needs only CMake, no compiler or vcpkg:
#
#   cmake -P cmake/tests/triplet_detection.cmake
#
# Each case runs the file in its own `cmake -P` process, pretending to be the host, generator and
# environment the case names, against a fake vcpkg root (in build/) that holds the triplet names below.
# The exit status is non-zero when any case fails.

cmake_minimum_required(VERSION 3.20)

if(SEDX_TEST_CASE)
    # One case. The host variables are what detect-platform-compiler.cmake reads before project();
    # "native" keeps the real ones.
    if(NOT SEDX_TEST_HOST STREQUAL "native")
        set(CMAKE_HOST_WIN32 "")
        set(CMAKE_HOST_APPLE "")
        set(CMAKE_HOST_UNIX "")
        if(SEDX_TEST_HOST STREQUAL "windows")
            set(CMAKE_HOST_WIN32 1)
        elseif(SEDX_TEST_HOST STREQUAL "macos")
            set(CMAKE_HOST_APPLE 1)
            set(CMAKE_HOST_UNIX 1)
        else()
            set(CMAKE_HOST_UNIX 1)
        endif()
    endif()
    # A list can't pass through execute_process as one argument: SEDX_TEST_OSX_ARCHS holds it with commas
    if(DEFINED ENV{SEDX_TEST_OSX_ARCHS})
        string(REPLACE "," ";" CMAKE_OSX_ARCHITECTURES "$ENV{SEDX_TEST_OSX_ARCHS}")
    endif()
    include("${CMAKE_CURRENT_LIST_DIR}/../detect-platform-compiler.cmake")
    message("SEDX_TEST_TRIPLET=${VCPKG_TARGET_TRIPLET}")
    return()
endif()

set(_self "${CMAKE_CURRENT_LIST_FILE}")
get_filename_component(_repo "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(_fake_vcpkg "${_repo}/build/triplet-detection-test/vcpkg")
file(REMOVE_RECURSE "${_fake_vcpkg}")
foreach(_t x64-windows x86-windows arm64-windows arm-windows arm64ec-windows x64-osx arm64-osx
        x64-linux x86-linux arm64-linux arm-linux ppc64le-linux
        x64-mingw-dynamic x86-mingw-dynamic arm64-mingw-dynamic)
    file(WRITE "${_fake_vcpkg}/triplets/${_t}.cmake" "")
endforeach()

# sedx_case(<name> <expected> <host> [DEFINES -D...] [ENV NAME=VALUE...] [WARNS <regex>])
# <expected> is a triplet, "MATCHES:<regex>" for cases that depend on the machine running the test, or
# "ERROR" when the configure must stop. <host> is windows, macos, linux or native. WARNS: a warning the case must print.
function(sedx_case name expected host)
    cmake_parse_arguments(C "" "WARNS" "DEFINES;ENV" ${ARGN})
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env --unset=CXX --unset=SEDX_PREFERRED_COMPILER --unset=VSCMD_ARG_TGT_ARCH --unset=SEDX_TEST_OSX_ARCHS
                ${C_ENV}
                "${CMAKE_COMMAND}" -DSEDX_TEST_CASE=1 "-DSEDX_TEST_HOST=${host}" "-DVCPKG_ROOT=${_fake_vcpkg}"
                ${C_DEFINES} -P "${_self}"
        RESULT_VARIABLE _rc OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
    set(_got "")
    if(_err MATCHES "SEDX_TEST_TRIPLET=([^\r\n]*)")
        set(_got "${CMAKE_MATCH_1}")
    endif()

    if(expected STREQUAL "ERROR")
        if(NOT _rc EQUAL 0 AND _err MATCHES "no vcpkg triplet fits this build")
            set(_ok TRUE)
            set(_got "stopped, as expected")
        else()
            set(_ok FALSE)
        endif()
    elseif(NOT _rc EQUAL 0)
        set(_ok FALSE)
        string(STRIP "${_err}" _got)
    elseif(expected MATCHES "^MATCHES:(.*)$")
        if(_got MATCHES "${CMAKE_MATCH_1}")
            set(_ok TRUE)
        else()
            set(_ok FALSE)
        endif()
    else()
        if(_got STREQUAL expected)
            set(_ok TRUE)
        else()
            set(_ok FALSE)
        endif()
    endif()

    if(_ok AND C_WARNS AND NOT _err MATCHES "${C_WARNS}")
        set(_ok FALSE)
        set(_got "${_got} without the warning ${C_WARNS}")
    endif()

    if(_ok)
        message(STATUS "pass  ${name}: ${_got}")
    else()
        message(STATUS "FAIL  ${name}: expected ${expected}, got ${_got}")
        set_property(GLOBAL APPEND PROPERTY SEDX_TRIPLET_FAILURES "${name}")
    endif()
endfunction()

set(_vs "-DCMAKE_GENERATOR=Visual Studio 18 2026")

# Windows, Visual Studio. No -A: the machine running the test decides, but never the 32-bit x86-windows.
sedx_case("VS, no -A" "MATCHES:^(x64|arm64)-windows$" windows DEFINES "${_vs}")
sedx_case("VS -A x64" x64-windows windows DEFINES "${_vs}" -DCMAKE_GENERATOR_PLATFORM=x64)
sedx_case("VS -A Win32" x86-windows windows DEFINES "${_vs}" -DCMAKE_GENERATOR_PLATFORM=Win32)
sedx_case("VS -A ARM64" arm64-windows windows DEFINES "${_vs}" -DCMAKE_GENERATOR_PLATFORM=ARM64)
sedx_case("VS -A ARM64EC" arm64ec-windows windows DEFINES "${_vs}" -DCMAKE_GENERATOR_PLATFORM=ARM64EC)
sedx_case("VS -A ARM" arm-windows windows DEFINES "${_vs}" -DCMAKE_GENERATOR_PLATFORM=ARM)
sedx_case("VS, no -A, from a VS x86 prompt (VS ignores the prompt)" "MATCHES:^(x64|arm64)-windows$" windows
          DEFINES "${_vs}" ENV VSCMD_ARG_TGT_ARCH=x86)
sedx_case("VS -A x64, from a VS x86 prompt" x64-windows windows
          DEFINES "${_vs}" -DCMAKE_GENERATOR_PLATFORM=x64 ENV VSCMD_ARG_TGT_ARCH=x86)
sedx_case("VS -A with no vcpkg triplet" ERROR windows DEFINES "${_vs}" -DCMAKE_GENERATOR_PLATFORM=Itanium)

# Windows, Ninja: a Visual Studio developer prompt, MinGW, clang-cl
sedx_case("Ninja, VS x64 prompt" x64-windows windows DEFINES -DCMAKE_GENERATOR=Ninja ENV VSCMD_ARG_TGT_ARCH=x64)
sedx_case("Ninja, VS x86 prompt" x86-windows windows DEFINES -DCMAKE_GENERATOR=Ninja ENV VSCMD_ARG_TGT_ARCH=x86)
sedx_case("Ninja, CXX=g++ (MinGW)" "MATCHES:^(x64|arm64)-mingw-dynamic$" windows
          DEFINES -DCMAKE_GENERATOR=Ninja ENV CXX=g++)
sedx_case("Ninja, CXX=clang (MinGW)" "MATCHES:^(x64|arm64)-mingw-dynamic$" windows DEFINES -DCMAKE_GENERATOR=Ninja ENV CXX=clang)
sedx_case("Ninja, CXX=clang, VS x64 prompt (MSVC ABI)" x64-windows windows
          DEFINES -DCMAKE_GENERATOR=Ninja ENV CXX=clang VSCMD_ARG_TGT_ARCH=x64)
sedx_case("Ninja, CXX=clang-cl" "MATCHES:^(x64|arm64)-windows$" windows DEFINES -DCMAKE_GENERATOR=Ninja ENV CXX=clang-cl)

# macOS
sedx_case("macOS arm64" arm64-osx macos DEFINES -DCMAKE_GENERATOR=Ninja -DCMAKE_OSX_ARCHITECTURES=arm64)
sedx_case("macOS x86_64" x64-osx macos DEFINES -DCMAKE_GENERATOR=Ninja -DCMAKE_OSX_ARCHITECTURES=x86_64)
sedx_case("macOS universal (first architecture)" arm64-osx macos
          DEFINES -DCMAKE_GENERATOR=Ninja ENV SEDX_TEST_OSX_ARCHS=arm64,x86_64 WARNS "a universal build")
sedx_case("macOS universal, x86_64 first" x64-osx macos
          DEFINES -DCMAKE_GENERATOR=Ninja ENV SEDX_TEST_OSX_ARCHS=x86_64,arm64 WARNS "a universal build")
sedx_case("macOS i386 (vcpkg has no x86-osx)" ERROR macos DEFINES -DCMAKE_GENERATOR=Ninja -DCMAKE_OSX_ARCHITECTURES=i386)

# Linux, and the machine running the test as it is
sedx_case("Linux, Makefiles" "MATCHES:^(x64|arm64)-linux$" linux DEFINES "-DCMAKE_GENERATOR=Unix Makefiles")
sedx_case("Linux, CXX=g++" "MATCHES:^(x64|arm64)-linux$" linux DEFINES -DCMAKE_GENERATOR=Ninja ENV CXX=g++)
sedx_case("This machine, no hints" "MATCHES:^(x64|arm64)-(windows|osx|linux)$" native)

file(REMOVE_RECURSE "${_repo}/build/triplet-detection-test")
get_property(_failures GLOBAL PROPERTY SEDX_TRIPLET_FAILURES)
if(_failures)
    list(LENGTH _failures _count)
    message(FATAL_ERROR "${_count} triplet detection case(s) failed: ${_failures}")
endif()
message(STATUS "All triplet detection cases passed")

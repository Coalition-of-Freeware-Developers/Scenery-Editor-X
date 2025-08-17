# --------------------------------
# Math Library
# --------------------------------
MESSAGE(STATUS "=================================================")
MESSAGE(STATUS "Generating Math Library (xMath)")

# Optionally enable Tracy profiling instrumentation for this library.
# Default OFF to keep the math DLL free of external profiling dependencies unless explicitly requested.
option(XMATH_WITH_TRACY "Enable Tracy profiling in xMath" OFF)

SET (PROJECT_CONFIG_FILES
	${CMAKE_SOURCE_DIR}/.clang-format
	${CMAKE_SOURCE_DIR}/.editorconfig
	${CMAKE_SOURCE_DIR}/SceneryEditorX.licenseheader
	resource.h
	math_config.h
)
SET(MATH_DIRECTORY ${CMAKE_SOURCE_DIR}/source/Math)
FILE(GLOB MATH_HEADER_DIR
    ${MATH_DIRECTORY}/includes
)
FILE(GLOB MATH_HEADER_FILES
    ${MATH_DIRECTORY}/includes/*
)
FILE(GLOB MATH_SOURCE_DIR
    ${MATH_DIRECTORY}/src
)
FILE(GLOB MATH_SOURCE_FILES
    ${MATH_DIRECTORY}/src/*
)
FILE(GLOB MATH_DOCUMENTATION_FILES
	${MATH_DIRECTORY}/docs/*
)
ADD_LIBRARY(xMath
	SHARED
    ${MATH_SOURCE_FILES}
    ${MATH_HEADER_FILES}
	${PROJECT_CONFIG_FILES}
	${MATH_DOCUMENTATION_FILES}
    xMath.rc
)

# Use project-wide C++20 standard (required for <format> etc.)
set_target_properties(xMath PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES)

# Source groups for IDE organization
SOURCE_GROUP("Core"
	FILES
	${MATH_HEADER_DIR}/aabb.h
	${MATH_HEADER_DIR}/constants.h
	${MATH_HEADER_DIR}/dot.h
	${MATH_HEADER_DIR}/epsilon.h
	${MATH_HEADER_DIR}/xmath.hpp
	${MATH_HEADER_DIR}/quat.h
	${MATH_SOURCE_DIR}/quat.cpp
)
SOURCE_GROUP("Transforms"
	FILES
	${MATH_HEADER_DIR}/scale.h
	${MATH_HEADER_DIR}/translate.h
	${MATH_HEADER_DIR}/rotation.h
	${MATH_HEADER_DIR}/transforms.h
	${MATH_SOURCE_DIR}/transforms.cpp
	${MATH_HEADER_DIR}/projection.h
)
SOURCE_GROUP("Colors"
	FILES
	${MATH_HEADER_DIR}/colors.h
	${MATH_SOURCE_DIR}/colors.cpp
	${MATH_HEADER_DIR}/gradients.h
	${MATH_SOURCE_DIR}/gradients.cpp
)
SOURCE_GROUP("Matrix"
	FILES
	${MATH_HEADER_DIR}/matrix.h
	${MATH_SOURCE_DIR}/matrix.cpp
	${MATH_HEADER_DIR}/mat2.h
	${MATH_HEADER_DIR}/mat3.h
	${MATH_HEADER_DIR}/mat4.h
)
SOURCE_GROUP("Vectors"
	FILES
	${MATH_HEADER_DIR}/vector.h
	${MATH_HEADER_DIR}/vec2.h
	${MATH_HEADER_DIR}/vec3.h
	${MATH_HEADER_DIR}/vec4.h
)
SOURCE_GROUP("Utilities"
	FILES
	${MATH_SOURCE_DIR}/math_utils.cpp
	${MATH_HEADER_DIR}/math_utils.h
)
SOURCE_GROUP("Documentation"
	FILES
	    ${MATH_DOCUMENTATION_FILES}
)
SOURCE_GROUP("Resource"
	FILES
	    resource.h
	    xMath.rc
		math_config.h
)
# C++20 already enforced above; remove older standard feature setting
TARGET_COMPILE_DEFINITIONS(xMath
	PRIVATE
		UNICODE
		_UNICODE
		_CRT_SECURE_NO_WARNINGS
		$<$<CONFIG:Debug>:_CONSOLE>
		$<$<CONFIG:Debug>:_DEBUG>
		$<$<CONFIG:Debug>:SEDX_DEBUG>
		$<$<CONFIG:Release>:SEDX_RELEASE>
		XMATH_BUILD  # Signal we are building the xMath DLL (controls XMATH_API)
		$<$<BOOL:${XMATH_WITH_TRACY}>:XMATH_ALLOW_TRACY>
)

TARGET_INCLUDE_DIRECTORIES(xMath
	PUBLIC
		# Public headers for consumers (only expose math includes + root for config if needed)
		${CMAKE_SOURCE_DIR}/source
		${CMAKE_SOURCE_DIR}/source/Math
		${CMAKE_SOURCE_DIR}/source/Math/includes
	PRIVATE
		${Stb_INCLUDE_DIR}
)

# Ensure consistent UTF-8 source decoding on MSVC (prevents fmt / Unicode warnings)
if (MSVC)
	target_compile_options(xMath PRIVATE /utf-8)
endif()

# Pure math library should not pull graphics / platform GUI libs by default.
# Removed dxgi/d3d12/Shell32 linkage (was unnecessary here). If future SIMD or platform
# specific functionality needs system libs they can be reintroduced behind an option.

# Set output directory
SET_TARGET_PROPERTIES(xMath PROPERTIES
    OUTPUT_NAME "xMath"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

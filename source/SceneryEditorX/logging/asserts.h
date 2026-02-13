/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * asserts.h
 * -------------------------------------------------------
 * Created: 11/4/2025
 * -------------------------------------------------------
 */
#pragma once
#include "SceneryEditorX/core/base.h"
#include <limits>
#include <type_traits>
#if !defined(SEDX_NO_LOGGING)
#include "logging.hpp"
#endif

// -------------------------------------------------------
// Platform-specific Debug Break
// -------------------------------------------------------

#ifdef SEDX_PLATFORM_WINDOWS
    #define SEDX_DEBUG_BREAK __debugbreak()
#elif defined(SEDX_COMPILER_CLANG)
    #define SEDX_DEBUG_BREAK __builtin_debugtrap()
#else
    #define SEDX_DEBUG_BREAK
#endif

// -------------------------------------------------------
// Runtime Assertion Configuration
// -------------------------------------------------------

#ifdef SEDX_DEBUG
    #define SEDX_ENABLE_ASSERTS
#endif

#define SEDX_ENABLE_VERIFY

// -------------------------------------------------------
// Runtime Assertion Macros
// -------------------------------------------------------

#ifdef SEDX_ENABLE_ASSERTS
	#ifdef SEDX_COMPILER_CLANG
		#if !defined(SEDX_NO_LOGGING)
			#define SEDX_CORE_ASSERT_MESSAGE_INTERNAL(...) ::SceneryEditorX::Log::PrintAssertMessage(::SceneryEditorX::Log::Type::Core, "Assertion Failed (" __FILE__ ":" SEDX_STRINGIFY(__LINE__) ") ", ##__VA_ARGS__)
			#define SEDX_ASSERT_MESSAGE_INTERNAL(...) ::SceneryEditorX::Log::PrintAssertMessage(::SceneryEditorX::Log::Type::Editor, "Assertion Failed (" __FILE__ ":" SEDX_STRINGIFY(__LINE__) ") ", ##__VA_ARGS__)
		#else
			#define SEDX_CORE_ASSERT_MESSAGE_INTERNAL(...) ((void)0)
			#define SEDX_ASSERT_MESSAGE_INTERNAL(...) ((void)0)
		#endif
	#else
		#if !defined(SEDX_NO_LOGGING)
			#define SEDX_CORE_ASSERT_MESSAGE_INTERNAL(...)  ::SceneryEditorX::Log::PrintAssertMessage(::SceneryEditorX::Log::Type::Core, "Assertion Failed (" __FILE__ ":" SEDX_STRINGIFY(__LINE__) ") " __VA_OPT__(, ) __VA_ARGS__)
			#define SEDX_ASSERT_MESSAGE_INTERNAL(...) ::SceneryEditorX::Log::PrintAssertMessage(::SceneryEditorX::Log::Type::Editor, "Assertion Failed (" __FILE__ ":" SEDX_STRINGIFY(__LINE__) ") " __VA_OPT__(, ) __VA_ARGS__)
		#else
			#define SEDX_CORE_ASSERT_MESSAGE_INTERNAL(...) ((void)0)
			#define SEDX_ASSERT_MESSAGE_INTERNAL(...) ((void)0)
		#endif
	#endif

#define SEDX_CORE_ASSERT(condition, ...) do { if (!(condition)) { SEDX_CORE_ASSERT_MESSAGE_INTERNAL(__VA_ARGS__); SEDX_DEBUG_BREAK; } } while (0)
	#define SEDX_ASSERT(condition, ...) do { if (!(condition)) { SEDX_ASSERT_MESSAGE_INTERNAL(__VA_ARGS__); SEDX_DEBUG_BREAK; } } while (0)
#else
	#define SEDX_CORE_ASSERT(condition, ...) ((void)(condition))
	#define SEDX_ASSERT(condition, ...) ((void)(condition))
#endif

// -------------------------------------------------------
// Verify Macros (Always Evaluate Condition)
// -------------------------------------------------------

#ifdef SEDX_ENABLE_VERIFY
	#if !defined(SEDX_NO_LOGGING)
		#ifdef SEDX_COMPILER_CLANG
			#define SEDX_CORE_VERIFY_MESSAGE_INTERNAL(...) ::SceneryEditorX::Log::PrintAssertMessage(::SceneryEditorX::Log::Type::Core, "Verify Failed (" __FILE__ ":" SEDX_STRINGIFY(__LINE__) ") ", ##__VA_ARGS__)
			#define SEDX_VERIFY_MESSAGE_INTERNAL(...) ::SceneryEditorX::Log::PrintAssertMessage(::SceneryEditorX::Log::Type::Editor, "Verify Failed (" __FILE__ ":" SEDX_STRINGIFY(__LINE__) ") ", ##__VA_ARGS__)
		#else
			#define SEDX_CORE_VERIFY_MESSAGE_INTERNAL(...) ::SceneryEditorX::Log::PrintAssertMessage(::SceneryEditorX::Log::Type::Core, "Verify Failed (" __FILE__ ":" SEDX_STRINGIFY(__LINE__) ") " __VA_OPT__(, ) __VA_ARGS__)
			#define SEDX_VERIFY_MESSAGE_INTERNAL(...)  ::SceneryEditorX::Log::PrintAssertMessage(::SceneryEditorX::Log::Type::Editor, "Verify Failed (" __FILE__ ":" SEDX_STRINGIFY(__LINE__) ") " __VA_OPT__(, ) __VA_ARGS__)
		#endif
	#else
		#define SEDX_CORE_VERIFY_MESSAGE_INTERNAL(...) ((void)0)
		#define SEDX_VERIFY_MESSAGE_INTERNAL(...) ((void)0)
	#endif

	#define SEDX_CORE_VERIFY(condition, ...) do { if (!(condition)) { SEDX_CORE_VERIFY_MESSAGE_INTERNAL(__VA_ARGS__); SEDX_DEBUG_BREAK; } } while (0)
	#define SEDX_VERIFY(condition, ...) do { if (!(condition)) { SEDX_VERIFY_MESSAGE_INTERNAL(__VA_ARGS__); SEDX_DEBUG_BREAK; } } while (0)
#else
	#define SEDX_CORE_VERIFY(condition, ...) ((void)(condition))
	#define SEDX_VERIFY(condition, ...) ((void)(condition))
#endif

// -------------------------------------------------------
// Pointer Validation Asserts
// -------------------------------------------------------

/**
 * @brief Assert that a pointer is not null
 * 
 * Validates that a pointer is non-null before dereferencing.
 * Useful for preventing null pointer dereferences in debug builds.
 * 
 * @param ptr The pointer to validate
 * @param ... Optional message format string and arguments
 */
#define SEDX_ASSERT_NOT_NULL(ptr, ...) SEDX_ASSERT((ptr) != nullptr, "Pointer '" #ptr "' is null" __VA_OPT__(": ") __VA_ARGS__)
#define SEDX_CORE_ASSERT_NOT_NULL(ptr, ...) SEDX_CORE_ASSERT((ptr) != nullptr, "Pointer '" #ptr "' is null" __VA_OPT__(": ") __VA_ARGS__)

/**
 * @brief Assert that a pointer is null
 * 
 * Validates that a pointer is expected to be null.
 * Useful for ensuring cleanup operations have completed.
 * 
 * @param ptr The pointer to validate
 * @param ... Optional message format string and arguments
 */
#define SEDX_ASSERT_NULL(ptr, ...) SEDX_ASSERT((ptr) == nullptr, "Pointer '" #ptr "' is not null" __VA_OPT__(": ") __VA_ARGS__)
#define SEDX_CORE_ASSERT_NULL(ptr, ...) SEDX_CORE_ASSERT((ptr) == nullptr, "Pointer '" #ptr "' is not null" __VA_OPT__(": ") __VA_ARGS__)

// -------------------------------------------------------
// Range and Bounds Checking Asserts
// -------------------------------------------------------

/**
 * @brief Assert that a value is within a specified range [min, max]
 * 
 * Validates that a value falls within an inclusive range.
 * 
 * @param value The value to check
 * @param min Minimum allowed value (inclusive)
 * @param max Maximum allowed value (inclusive)
 * @param ... Optional message format string and arguments
 */
#define SEDX_ASSERT_IN_RANGE(value, min, max, ...) \
    SEDX_ASSERT((value) >= (min) && (value) <= (max), "Value '" #value "' ({}) is out of range [{}, {}]" __VA_OPT__(": ") __VA_ARGS__, (value), (min), (max))

#define SEDX_CORE_ASSERT_IN_RANGE(value, min, max, ...) \
    SEDX_CORE_ASSERT((value) >= (min) && (value) <= (max), "Value '" #value "' ({}) is out of range [{}, {}]" __VA_OPT__(": ") __VA_ARGS__, (value), (min), (max))

/**
 * @brief Assert that an index is within valid bounds for a container
 * 
 * Validates that an index can safely access a container element.
 * 
 * @param index The index to validate
 * @param size The size of the container
 * @param ... Optional message format string and arguments
 */
#define SEDX_ASSERT_INDEX_IN_BOUNDS(index, size, ...) \
    SEDX_ASSERT((index) < (size), "Index {} is out of bounds (size: {})" __VA_OPT__(": ") __VA_ARGS__, (index), (size))

#define SEDX_CORE_ASSERT_INDEX_IN_BOUNDS(index, size, ...) \
    SEDX_CORE_ASSERT((index) < (size), "Index {} is out of bounds (size: {})" __VA_OPT__(": ") __VA_ARGS__, (index), (size))

// -------------------------------------------------------
// Comparison Asserts
// -------------------------------------------------------

/**
 * @brief Assert that two values are equal
 */
#define SEDX_ASSERT_EQUAL(a, b, ...) SEDX_ASSERT((a) == (b), "Expected '" #a "' == '" #b "', got {} != {}" __VA_OPT__(": ") __VA_ARGS__, (a), (b))
#define SEDX_CORE_ASSERT_EQUAL(a, b, ...) SEDX_CORE_ASSERT((a) == (b), "Expected '" #a "' == '" #b "', got {} != {}" __VA_OPT__(": ") __VA_ARGS__, (a), (b))

/**
 * @brief Assert that two values are not equal
 */
#define SEDX_ASSERT_NOT_EQUAL(a, b, ...) SEDX_ASSERT((a) != (b), "Expected '" #a "' != '" #b "', but both equal {}" __VA_OPT__(": ") __VA_ARGS__, (a))
#define SEDX_CORE_ASSERT_NOT_EQUAL(a, b, ...) SEDX_CORE_ASSERT((a) != (b), "Expected '" #a "' != '" #b "', but both equal {}" __VA_OPT__(": ") __VA_ARGS__, (a))

// -------------------------------------------------------
// Unreachable Code Markers
// -------------------------------------------------------

/**
 * @brief Mark code paths that should never be reached
 * 
 * Use this macro to mark code paths that should be unreachable.
 * In debug builds, it will assert if reached. In release builds,
 * it provides a hint to the compiler for optimization.
 * 
 * @param ... Optional message format string and arguments
 */
#ifdef SEDX_ENABLE_ASSERTS
    #define SEDX_UNREACHABLE(...) \
        do { \
            SEDX_ASSERT_MESSAGE_INTERNAL("Unreachable code reached" __VA_OPT__(": ") __VA_ARGS__); \
            SEDX_DEBUG_BREAK; \
        } while (0)
    
    #define SEDX_CORE_UNREACHABLE(...) \
        do { \
            SEDX_CORE_ASSERT_MESSAGE_INTERNAL("Unreachable code reached" __VA_OPT__(": ") __VA_ARGS__); \
            SEDX_DEBUG_BREAK; \
        } while (0)
#else
    #ifdef SEDX_COMPILER_MSVC
        #define SEDX_UNREACHABLE(...) __assume(0)
        #define SEDX_CORE_UNREACHABLE(...) __assume(0)
    #elif defined(SEDX_COMPILER_CLANG) || defined(SEDX_COMPILER_GCC)
        #define SEDX_UNREACHABLE(...) __builtin_unreachable()
        #define SEDX_CORE_UNREACHABLE(...) __builtin_unreachable()
    #else
        #define SEDX_UNREACHABLE(...) ((void)0)
        #define SEDX_CORE_UNREACHABLE(...) ((void)0)
    #endif
#endif

// -------------------------------------------------------
// Static Assertions - Type Traits
// -------------------------------------------------------

/**
 * @brief Assert that a type is trivially copyable
 * 
 * Ensures a type can be safely copied using memcpy.
 * Essential for GPU buffer uploads and binary serialization.
 */
#define SEDX_TRIVIAL_STATIC_ASSERT(type) \
    static_assert(std::is_trivially_copyable<type>::value, "Type " SEDX_STRINGIFY(type) " must be trivially copyable!")

/**
 * @brief Assert that a type is trivially destructible
 * 
 * Ensures a type doesn't require explicit destructor calls.
 * Important for placement new scenarios and memory pools.
 */
#define SEDX_TRIVIAL_DESTRUCTIBLE_ASSERT(type) \
    static_assert(std::is_trivially_destructible<type>::value, "Type " SEDX_STRINGIFY(type) " must be trivially destructible!")

/**
 * @brief Assert that a type is standard layout
 * 
 * Ensures a type has a compatible layout with C.
 * Required for interfacing with C APIs and certain serialization scenarios.
 */
#define SEDX_STANDARD_LAYOUT_ASSERT(type) \
    static_assert(std::is_standard_layout<type>::value, "Type " SEDX_STRINGIFY(type) " must have standard layout!")

/**
 * @brief Assert that a type is a POD (Plain Old Data) type
 * 
 * Combines trivial and standard layout requirements.
 * POD types are safe for binary serialization and C interop.
 */
#define SEDX_POD_ASSERT(type) \
    static_assert(std::is_trivially_copyable<type>::value && std::is_standard_layout<type>::value, \
                  "Type " SEDX_STRINGIFY(type) " must be a POD type (trivially copyable + standard layout)!")

/**
 * @brief Assert that a type is default constructible
 */
#define SEDX_DEFAULT_CONSTRUCTIBLE_ASSERT(type) \
    static_assert(std::is_default_constructible<type>::value, "Type " SEDX_STRINGIFY(type) " must be default constructible!")

/**
 * @brief Assert that a type is copy constructible
 */
#define SEDX_COPY_CONSTRUCTIBLE_ASSERT(type) \
    static_assert(std::is_copy_constructible<type>::value, "Type " SEDX_STRINGIFY(type) " must be copy constructible!")

/**
 * @brief Assert that a type is move constructible
 */
#define SEDX_MOVE_CONSTRUCTIBLE_ASSERT(type) \
    static_assert(std::is_move_constructible<type>::value, "Type " SEDX_STRINGIFY(type) " must be move constructible!")

/**
 * @brief Assert that a type is nothrow move constructible
 * 
 * Ensures move constructor doesn't throw exceptions.
 * Important for containers and exception safety guarantees.
 */
#define SEDX_NOTHROW_MOVE_CONSTRUCTIBLE_ASSERT(type) \
    static_assert(std::is_nothrow_move_constructible<type>::value, "Type " SEDX_STRINGIFY(type) " must be nothrow move constructible!")

/**
 * @brief Assert that a type is an enum
 */
#define SEDX_ENUM_ASSERT(type) \
    static_assert(std::is_enum<type>::value, "Type " SEDX_STRINGIFY(type) " must be an enum!")

/**
 * @brief Assert that a type is a scoped enum (enum class)
 */
#define SEDX_SCOPED_ENUM_ASSERT(type) \
    static_assert(std::is_enum<type>::value && !std::is_convertible<type, int>::value, \
                  "Type " SEDX_STRINGIFY(type) " must be a scoped enum (enum class)!")

/**
 * @brief Assert that a type is polymorphic (has virtual functions)
 */
#define SEDX_POLYMORPHIC_ASSERT(type) \
    static_assert(std::is_polymorphic<type>::value, "Type " SEDX_STRINGIFY(type) " must be polymorphic (have virtual functions)!")

/**
 * @brief Assert that a type is abstract
 */
#define SEDX_ABSTRACT_ASSERT(type) \
    static_assert(std::is_abstract<type>::value, "Type " SEDX_STRINGIFY(type) " must be abstract!")

/**
 * @brief Assert that a type is final
 */
#define SEDX_FINAL_ASSERT(type) \
    static_assert(std::is_final<type>::value, "Type " SEDX_STRINGIFY(type) " must be final!")

// -------------------------------------------------------
// Static Assertions - Size and Alignment
// -------------------------------------------------------

/**
 * @brief Assert that a type has a specific size
 * 
 * Ensures struct/class layout matches expected size.
 * Critical for binary formats and GPU buffer layouts.
 */
#define SEDX_SIZE_ASSERT(type, expectedSize) \
    static_assert(sizeof(type) == (expectedSize), \
                  "Type " SEDX_STRINGIFY(type) " size is " SEDX_STRINGIFY(sizeof(type)) " bytes, expected " SEDX_STRINGIFY(expectedSize) " bytes!")

/**
 * @brief Assert that a type's size is less than or equal to a maximum
 */
#define SEDX_MAX_SIZE_ASSERT(type, maxSize) \
    static_assert(sizeof(type) <= (maxSize), \
                  "Type " SEDX_STRINGIFY(type) " size (" SEDX_STRINGIFY(sizeof(type)) " bytes) exceeds maximum " SEDX_STRINGIFY(maxSize) " bytes!")

/**
 * @brief Assert that a type's size is at least a minimum
 */
#define SEDX_MIN_SIZE_ASSERT(type, minSize) \
    static_assert(sizeof(type) >= (minSize), \
                  "Type " SEDX_STRINGIFY(type) " size (" SEDX_STRINGIFY(sizeof(type)) " bytes) is less than minimum " SEDX_STRINGIFY(minSize) " bytes!")

/**
 * @brief Assert that a type has a specific alignment
 * 
 * Ensures proper memory alignment for SIMD operations and GPU requirements.
 */
#define SEDX_ALIGNMENT_ASSERT(type, expectedAlignment) \
    static_assert(alignof(type) == (expectedAlignment), \
                  "Type " SEDX_STRINGIFY(type) " alignment is " SEDX_STRINGIFY(alignof(type)) ", expected " SEDX_STRINGIFY(expectedAlignment) "!")

/**
 * @brief Assert that a type's alignment is at least a minimum
 */
#define SEDX_MIN_ALIGNMENT_ASSERT(type, minAlignment) \
    static_assert(alignof(type) >= (minAlignment), \
                  "Type " SEDX_STRINGIFY(type) " alignment (" SEDX_STRINGIFY(alignof(type)) ") is less than required " SEDX_STRINGIFY(minAlignment) "!")

/**
 * @brief Assert that two types have the same size
 */
#define SEDX_SAME_SIZE_ASSERT(type1, type2) \
    static_assert(sizeof(type1) == sizeof(type2), \
                  "Types " SEDX_STRINGIFY(type1) " and " SEDX_STRINGIFY(type2) " must have the same size!")

/**
 * @brief Assert that a type's size is a power of 2
 */
#define SEDX_POWER_OF_2_SIZE_ASSERT(type) \
    static_assert((sizeof(type) & (sizeof(type) - 1)) == 0, \
                  "Type " SEDX_STRINGIFY(type) " size must be a power of 2!")

// -------------------------------------------------------
// Static Assertions - Type Relationships
// -------------------------------------------------------

/**
 * @brief Assert that one type is derived from another
 */
#define SEDX_DERIVED_FROM_ASSERT(derived, base) \
    static_assert(std::is_base_of<base, derived>::value, \
                  "Type " SEDX_STRINGIFY(derived) " must be derived from " SEDX_STRINGIFY(base) "!")

/**
 * @brief Assert that two types are the same
 */
#define SEDX_SAME_TYPE_ASSERT(type1, type2) \
    static_assert(std::is_same<type1, type2>::value, \
                  "Types " SEDX_STRINGIFY(type1) " and " SEDX_STRINGIFY(type2) " must be the same!")

/**
 * @brief Assert that a type is convertible to another
 */
#define SEDX_CONVERTIBLE_TO_ASSERT(from, to) \
    static_assert(std::is_convertible<from, to>::value, \
                  "Type " SEDX_STRINGIFY(from) " must be convertible to " SEDX_STRINGIFY(to) "!")

// -------------------------------------------------------
// Static Assertions - Numeric Properties
// -------------------------------------------------------

/**
 * @brief Assert that a type is an integral type
 */
#define SEDX_INTEGRAL_ASSERT(type) \
    static_assert(std::is_integral<type>::value, "Type " SEDX_STRINGIFY(type) " must be an integral type!")

/**
 * @brief Assert that a type is a floating point type
 */
#define SEDX_FLOATING_POINT_ASSERT(type) \
    static_assert(std::is_floating_point<type>::value, "Type " SEDX_STRINGIFY(type) " must be a floating point type!")

/**
 * @brief Assert that a type is an arithmetic type (integral or floating point)
 */
#define SEDX_ARITHMETIC_ASSERT(type) \
    static_assert(std::is_arithmetic<type>::value, "Type " SEDX_STRINGIFY(type) " must be an arithmetic type!")

/**
 * @brief Assert that a type is signed
 */
#define SEDX_SIGNED_ASSERT(type) \
    static_assert(std::is_signed<type>::value, "Type " SEDX_STRINGIFY(type) " must be signed!")

/**
 * @brief Assert that a type is unsigned
 */
#define SEDX_UNSIGNED_ASSERT(type) \
    static_assert(std::is_unsigned<type>::value, "Type " SEDX_STRINGIFY(type) " must be unsigned!")

// -------------------------------------------------------
// Static Assertions - Container and Pointer Properties
// -------------------------------------------------------

/**
 * @brief Assert that a type is a pointer
 */
#define SEDX_POINTER_ASSERT(type) \
    static_assert(std::is_pointer<type>::value, "Type " SEDX_STRINGIFY(type) " must be a pointer!")

/**
 * @brief Assert that a type is not a pointer
 */
#define SEDX_NOT_POINTER_ASSERT(type) \
    static_assert(!std::is_pointer<type>::value, "Type " SEDX_STRINGIFY(type) " must not be a pointer!")

/**
 * @brief Assert that a type is a reference
 */
#define SEDX_REFERENCE_ASSERT(type) \
    static_assert(std::is_reference<type>::value, "Type " SEDX_STRINGIFY(type) " must be a reference!")

/**
 * @brief Assert that a type is an array
 */
#define SEDX_ARRAY_ASSERT(type) \
    static_assert(std::is_array<type>::value, "Type " SEDX_STRINGIFY(type) " must be an array!")

/**
 * @brief Assert that a type is a class or struct
 */
#define SEDX_CLASS_ASSERT(type) \
    static_assert(std::is_class<type>::value, "Type " SEDX_STRINGIFY(type) " must be a class or struct!")

/**
 * @brief Assert that a type is empty (no non-static data members)
 */
#define SEDX_EMPTY_ASSERT(type) \
    static_assert(std::is_empty<type>::value, "Type " SEDX_STRINGIFY(type) " must be empty!")

// -------------------------------------------------------
// Combined Validation Macros
// -------------------------------------------------------

/**
 * @brief Comprehensive vertex structure validation
 * 
 * Validates that a vertex structure meets all requirements:
 * - Trivially copyable for GPU upload
 * - Standard layout for shader compatibility
 * - Size matches expected GPU alignment
 */
#define SEDX_VERTEX_STRUCT_ASSERT(type) \
    SEDX_TRIVIAL_STATIC_ASSERT(type); \
    SEDX_STANDARD_LAYOUT_ASSERT(type); \
    static_assert(sizeof(type) % 4 == 0, "Vertex type " SEDX_STRINGIFY(type) " size must be 4-byte aligned!")

/**
 * @brief Comprehensive GPU buffer structure validation
 * 
 * Validates buffer structures for GPU upload:
 * - Trivially copyable
 * - Proper alignment (typically 16 bytes for UBOs)
 */
#define SEDX_GPU_BUFFER_STRUCT_ASSERT(type, alignment) \
    SEDX_TRIVIAL_STATIC_ASSERT(type); \
    SEDX_STANDARD_LAYOUT_ASSERT(type); \
    SEDX_MIN_ALIGNMENT_ASSERT(type, alignment)

/**
 * @brief Validate enum can be used as flags
 * 
 * Ensures an enum is suitable for bitwise operations.
 */
#define SEDX_FLAGS_ENUM_ASSERT(type) \
    SEDX_ENUM_ASSERT(type); \
    SEDX_UNSIGNED_ASSERT(std::underlying_type_t<type>)

// -------------------------------------------------------
// Compile-Time Constant Validation
// -------------------------------------------------------

/**
 * @brief Assert that a value is a compile-time constant
 */
#define SEDX_CONSTEXPR_ASSERT(expr, ...) \
    static_assert(expr, "Expression must be true: " #expr __VA_OPT__(" - ") __VA_ARGS__)

/**
 * @brief Assert that a value is positive at compile time
 */
#define SEDX_POSITIVE_CONSTANT_ASSERT(value) \
    static_assert((value) > 0, "Value " SEDX_STRINGIFY(value) " must be positive!")

/**
 * @brief Assert that a value is non-negative at compile time
 */
#define SEDX_NON_NEGATIVE_CONSTANT_ASSERT(value) \
    static_assert((value) >= 0, "Value " SEDX_STRINGIFY(value) " must be non-negative!")

/**
 * @brief Assert that a value is a power of 2 at compile time
 */
#define SEDX_POWER_OF_2_CONSTANT_ASSERT(value) \
    static_assert(((value) > 0) && (((value) & ((value) - 1)) == 0), \
                  "Value " SEDX_STRINGIFY(value) " must be a power of 2!")

// -------------------------------------------------------

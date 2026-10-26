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
 * base.hpp
 * -------------------------------------------------------
 * Created: 16/3/2025
 * -------------------------------------------------------
 */
#pragma once
#include <atomic>
#include <cstdint>
#include <iosfwd>
#include <sstream>
#include <string>
#include <vector>
#include <SceneryEditorX/utils/system_detection.h>

// -------------------------------------------------------

/**
* Type aliases for fixed-width integer types
*/
typedef uint8_t u8;			// Unsigned 8-bit integer
typedef uint16_t u16;		// Unsigned 16-bit integer
typedef uint32_t u32;		// Unsigned 32-bit integer
typedef uint64_t u64;		// Unsigned 64-bit integer
typedef int8_t i8;			// Signed 8-bit integer
typedef int16_t i16;		// Signed 16-bit integer
typedef int32_t i32;		// Signed 32-bit integer
typedef int64_t i64;		// Signed 64-bit integer
typedef float f32;			// 32-bit floating point
typedef double f64;			// 64-bit floating point
typedef u32 ResourceID;     // Resource Identifier, alias for unsigned 32-bit integer

// -------------------------------------------------------

// Forward declarations to avoid heavy includes and cycles
namespace xMath
{
	class Mat2;
	class Mat3;
	class Mat4;
	class Quat;

	template<typename T>
	struct TVector2;

	template<typename T>
	struct TVector3;

	template<typename T>
	struct TVector4;
	
}

// -------------------------------------------------------

typedef xMath::TVector2<float> Vec2;   		// 2D vector
typedef xMath::TVector3<float> Vec3;   		// 3D vector
typedef xMath::TVector4<float> Vec4;   		// 4D vector
typedef xMath::TVector2<int32_t> iVec2; 	// 2D integer vector
typedef xMath::TVector3<int32_t> iVec3; 	// 3D integer vector
typedef xMath::TVector4<int32_t> iVec4; 	// 4D integer vector
typedef xMath::TVector2<uint32_t> UVec2; 	// 2D unsigned integer vector
typedef xMath::TVector3<uint32_t> UVec3; 	// 3D unsigned integer vector
typedef xMath::TVector4<uint32_t> UVec4; 	// 4D unsigned integer vector
typedef xMath::TVector2<bool> Bool2;     	// 2D boolean vector
typedef xMath::TVector3<bool> Bool3;     	// 3D boolean vector
typedef xMath::TVector4<bool> Bool4;     	// 4D boolean vector
typedef xMath::Mat2 Mat2;					// 2x2 matrix
typedef xMath::Mat3 Mat3;					// 3x3 matrix
typedef xMath::Mat4 Mat4;					// 4x4 matrix
typedef xMath::Quat Quat;					// Quaternion for rotation

// -------------------------------------------------------

using namespace xMath;

// -------------------------------------------------------

/**
 * @brief Aligns a size value to a specified alignment boundary.
 *
 * This macro calculates the smallest value greater than or equal to the
 * provided size that is aligned to the specified alignment boundary.
 *
 * @param size The original size to align.
 * @param alignment The alignment boundary (must be a power of 2).
 * @return The aligned size value.
 *
 * @note - If size is already aligned to alignment, it will be returned unchanged.
 *
 * @example
 * // Align 17 to 8-byte boundary (result: 24)
 * size_t alignedSize = ALIGN_AS(17, 8);
 */
#define ALIGN_AS(size, alignment) ((size) % (alignment) > 0 ? (size) + (alignment) - (size) % (alignment) : (size))

/**
 * @brief Calculates the number of elements in a statically allocated array.
 *
 * This macro uses the size of the entire array divided by the size of a single element
 * to determine the number of elements in the array. Only valid for statically allocated
 * arrays, not for pointers to arrays or dynamically allocated arrays.
 *
 * @param arr The array whose element count is to be determined.
 * @return The number of elements in the array.
 *
 * @warning This macro will not work correctly with pointers to arrays or dynamically
 *          allocated arrays (i.e., those allocated with new[] or malloc()).
 *
 * @example
 * // For an array: int numbers[10];
 * size_t count = COUNT_OF(numbers); // result: 10
 */
#define COUNT_OF(arr) (sizeof((arr)) / sizeof((arr)[0]))

/**
 * @brief Expands a macro's value.
 *
 * This utility macro ensures that a macro parameter is fully expanded before it's used.
 * This is particularly useful in complex macro definitions where multiple levels
 * of macro expansion are needed.
 *
 * @param x The macro to expand.
 * @return The expanded value of the macro.
 */
#define SEDX_EXPAND_MACRO(x) x

/**
 * @brief Implementation helper for SEDX_STRINGIFY.
 *
 * This internal macro converts its argument to a string literal.
 * Not intended to be used directly.
 *
 * @param x The token to convert to a string.
 * @return The string representation of the token.
 */
#define SEDX_STRINGIFY_IMPL(x) #x

/**
 * @brief Converts a token to a string literal after macro expansion.
 *
 * This macro ensures that its argument is fully expanded before being
 * converted to a string literal, which is useful when the argument is itself a macro.
 *
 * @param x The token or macro to convert to a string.
 * @return The string representation of the expanded token.
 *
 * @example
 * #define VERSION 1.2
 * const char* version = SEDX_STRINGIFY(VERSION); // results in "1.2"
 */
#define SEDX_STRINGIFY(x) SEDX_STRINGIFY_IMPL(x)

/**
 * @brief Creates a bit mask with the bit at position x set to 1.
 *
 * This macro shifts 1 to the left by x positions, creating a bit mask
 * with only the bit at position x set.
 *
 * @param x The position of the bit to set (0-based).
 * @return An integer with only the bit at position x set.
 *
 * @example
 * // Create a mask with bit 3 set (binary: 00001000, decimal: 8)
 * unsigned int mask = BIT(3);
 */
#define BIT(x) (1 << x)

/**
 * @brief Creates a lambda that binds a member function to the current instance.
 *
 * This macro simplifies the creation of lambda functions that call member functions
 * of the current class instance. It's particularly useful for callbacks and event handlers.
 *
 * @param fn The member function to bind.
 * @return A lambda that forwards its arguments to the specified member function.
 *
 * @example
 * // Register a callback
 * eventSystem.registerHandler(EventType::Resize, SEDX_BIND_EVENT_FN(MyClass::OnResize));
 */
#define SEDX_BIND_EVENT_FN(fn) [this](auto &&...args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @brief Alias for unsigned 8-bit integer, providing a type named 'byte'.
	 *
	 * This type represents a byte of memory (8 bits) and is consistent with
	 * the standard uint8_t type. It can be used for operations that specifically
	 * deal with raw bytes of memory rather than character data or other numeric types.
	 *
	 * @note - Equivalent to uint8_t from <cstdint>
	 */
	typedef uint8_t byte;

} // namespace SceneryEditorX

/**
 * @brief Template function to convert an input to char.
 * @tparam T The type of the input.
 * @param input The input to convert.
 * @return The converted char.
 */
template <typename T>
char ToChar(const T &input)
{
	std::stringstream ss;
	ss << input;
	std::string str = ss.str();
	return !str.empty() ? str[0] : '\0';
}

/**
 * @brief Template function to convert an input to const char.
 * @tparam T The type of the input.
 * @param input The input to convert.
 * @return The converted const char.
 */
template <typename T>
const char *ToConstChar(const T &input)
{
	static std::string str;
	std::stringstream ss;
	ss << input;
	str = ss.str();
	return str.c_str();
}

/**
* @brief Convert a value to a string.
* @tparam T The type of the value to convert.
* @param value The value to convert.
* @return The string representation of the value.
*/
template <typename T>
std::string ToString(const T &value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

/**
 * @brief Convert a string to a value.
 * @tparam N The type of the value to convert.
 * @param arr
 * @return The value representation of the string.
 */
template <size_t N>
std::vector<std::string> ArrayToVector(const std::array<const char *, N> &arr)
{
	std::vector<std::string> vec;
	for (const char *str : arr)
	{
		vec.emplace_back(str);
	}
	return vec;
}

/**
 * @brief Convert a string to a value.
 * @tparam T The type of the value to convert.
 * @param str The string to convert.
 * @return The value representation of the string.
 */
/*
template <typename T>
void CmdTimeStamp(const std::string &m_Name, T callback)
{
	int id = CmdBeginTimeStamp(m_Name);
	callback();
	CmdEndTimeStamp(id);
}
*/

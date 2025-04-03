/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* base.hpp
* -------------------------------------------------------
* Created: 16/3/2025
* -------------------------------------------------------
*/

#pragma once

#include <cstdint>
#include <glm/glm.hpp>

// -------------------------------------------------------

/*
* Type aliases for fixed-width integer types
*/
using u8  = uint8_t;  // Unsigned 8-bit integer
using u16 = uint16_t; // Unsigned 16-bit integer
using u32 = uint32_t; // Unsigned 32-bit integer
using u64 = uint64_t; // Unsigned 64-bit integer
using i8  = int8_t;   // Signed 8-bit integer
using i16 = int16_t;  // Signed 16-bit integer
using i32 = int32_t;  // Signed 32-bit integer
using i64 = int64_t;  // Signed 64-bit integer
using f32 = float;    // 32-bit floating point
using f64 = double;   // 64-bit floating point
using RID = u32;      // Resource Identifier, alias for unsigned 32-bit integer

// -------------------------------------------------------

using Vec2 = glm::vec2; // 2D vector
using Vec3 = glm::vec3; // 3D vector
using Vec4 = glm::vec4; // 4D vector

using Mat2 = glm::mat2; // 2x2 matrix
using Mat3 = glm::mat3; // 3x3 matrix
using Mat4 = glm::mat4; // 4x4 matrix

// -------------------------------------------------------

// Macro to align a given size to the specified alignment
#define ALIGN_AS(size, alignment) ((size) % (alignment) > 0 ? (size) + (alignment) - (size) % (alignment) : (size))

// Macro to count the number of elements in an array
#define COUNT_OF(arr) (sizeof((arr)) / sizeof((arr)[0]))

#define SEDX_EXPAND_MACRO(x) x
#define SEDX_STRINGIFY_MACRO(x) #x
#define BIT(x) (1u << x)
#define SEDX_BIND_EVENT_FN(fn) [this](auto &&...args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

// -------------------------------------------------------

/**
 * @brief Alias template for a unique pointer to type T.
 * @tparam T The type to manage.
 */
template <typename T>
using Scope = std::unique_ptr<T>;

/**
 * @brief Creates a unique pointer to an object of type T.
 * @tparam T The type to manage.
 * @tparam Args The types of the arguments to pass to the constructor of T.
 * @param args The arguments to pass to the constructor of T.
 * @return A unique pointer to an object of type T.
 */
template <typename T, typename... Args>
constexpr Scope<T> CreateScope(Args &&...args)
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}

/**
 * @brief Alias template for a shared pointer to type T.
 * @tparam T The type to manage.
 */
template <typename T>
using Ref = std::shared_ptr<T>;

/**
 * @brief Creates a shared pointer to an object of type T.
 * @tparam T The type to manage.
 * @tparam Args The types of the arguments to pass to the constructor of T.
 * @param args The arguments to pass to the constructor of T.
 * @return A shared pointer to an object of type T.
 */
template <typename T, typename... Args>
constexpr Ref<T> CreateRef(Args &&...args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}

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
 * @tparam T The type of the value to convert.
 * @param str The string to convert.
 * @return The value representation of the string.
 */
template <size_t N>
std::vector<std::string> arrayToVector(const std::array<const char *, N> &arr)
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
void CmdTimeStamp(const std::string &name, T callback)
{
	int id = CmdBeginTimeStamp(name);
	callback();
	CmdEndTimeStamp(id);
}
*/

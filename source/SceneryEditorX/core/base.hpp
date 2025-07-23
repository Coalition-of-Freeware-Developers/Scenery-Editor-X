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
#include <atomic>
#include <cstdint>
#include <glm/glm.hpp>
#include <iosfwd>
#include <SceneryEditorX/platform/system_detection.h>
#include <sstream>
#include <string>
#include <vector>

/// -------------------------------------------------------

#ifdef SEDX_DEBUG
constexpr bool enableValidationLayers = true;
#else
constexpr bool enableValidationLayers = false;
#endif

/// -------------------------------------------------------

/**
* Type aliases for fixed-width integer types
*/
using u8  = uint8_t;  ///< Unsigned 8-bit integer
using u16 = uint16_t; ///< Unsigned 16-bit integer
using u32 = uint32_t; ///< Unsigned 32-bit integer
using u64 = uint64_t; ///< Unsigned 64-bit integer
using i8  = int8_t;   ///< Signed 8-bit integer
using i16 = int16_t;  ///< Signed 16-bit integer
using i32 = int32_t;  ///< Signed 32-bit integer
using i64 = int64_t;  ///< Signed 64-bit integer
using f32 = float;    ///< 32-bit floating point
using f64 = double;   ///< 64-bit floating point
using ResourceID = u32;      ///< Resource Identifier, alias for unsigned 32-bit integer

/// -------------------------------------------------------

using Vec2	=	glm::vec2;	///< 2D vector
using Vec3	=	glm::vec3;	///< 3D vector
using Vec4	=	glm::vec4;	///< 4D vector
using iVec2 =	glm::ivec2; ///< 2D integer vector
using iVec3 =	glm::ivec3; ///< 3D integer vector
using iVec4 =	glm::ivec4; ///< 4D integer vector
using Mat2	=	glm::mat2;	///< 2x2 matrix
using Mat3	=	glm::mat3;	///< 3x3 matrix
using Mat4	=	glm::mat4;	///< 4x4 matrix
using Quat	=	glm::quat;	///< Quaternion for rotation

/// -------------------------------------------------------

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
 * @note If size is already aligned to alignment, it will be returned unchanged.
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

/// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @brief Rounds a value down to the nearest multiple of the specified factor.
	 * 
	 * This function rounds down the input value to the nearest multiple of the given factor
	 * using integer division followed by multiplication. For example, RoundDown(17, 5) = 15.
	 * 
	 * @tparam T The numeric type of both the value and factor (int, float, etc.)
	 * @param x The value to round down
	 * @param fac The factor to which x should be rounded (must be non-zero)
	 * @return T The value of x rounded down to the nearest multiple of fac
	 */
	template<typename T>
	T RoundDown(T x, T fac) { return x / fac * fac; }

	/**
	 * @brief Rounds a value up to the nearest multiple of the specified factor.
	 * 
	 * This function rounds up the input value to the nearest multiple of the given factor.
	 * It adds (fac - 1) to x before rounding down to ensure rounding up.
	 * For example, RoundUp(17, 5) = 20.
	 * 
	 * @tparam T The numeric type of both the value and factor (int, float, etc.)
	 * @param x The value to round up
	 * @param fac The factor to which x should be rounded (must be non-zero)
	 * @return T The value of x rounded up to the nearest multiple of fac
	 */
	template<typename T>
	T RoundUp(T x, T fac) { return RoundDown(x + fac - 1, fac); }

    /**
     * @brief Alias for unsigned 8-bit integer, providing a type named 'byte'.
     * 
     * This type represents a byte of memory (8 bits) and is consistent with
     * the standard uint8_t type. It can be used for operations that specifically
     * deal with raw bytes of memory rather than character data or other numeric types.
     * 
     * @note Equivalent to uint8_t from <cstdint>
     */
    using byte = uint8_t;

	/**
	 * @struct AtomicFlag
	 * @brief A lightweight wrapper around std::atomic_flag providing intuitive dirty state management.
	 * 
	 * The AtomicFlag class encapsulates a std::atomic_flag to provide a thread-safe
	 * mechanism for tracking and resetting a "dirty" state. This is useful for signaling
	 * that data has changed and needs processing, in a way that's safe for concurrent access.
	 * 
	 * The implementation uses standard atomic operations to ensure thread safety without
	 * explicit locking. When copied, the new instance starts in a "clean" state regardless
	 * of the source object's state.
	 * 
	 * @note The semantics of atomic_flag in C++ guarantee that operations on the flag
	 *       are atomic and provide synchronization between threads.
	 */
	struct AtomicFlag
	{
	    /**
	     * @brief Sets the flag to dirty state.
	     * 
	     * Marks the flag as dirty by clearing the atomic flag. This operation is atomic
	     * and can be safely called from multiple threads.
	     */
	    SEDX_FORCE_INLINE void SetDirty() { flag.clear(); }
	    
	    /**
	     * @brief Checks if the flag is dirty and atomically resets it if it is.
	     * 
	     * Atomically tests if the flag is in the dirty state (cleared) and sets it
	     * (marking it as clean) in a single operation.
	     * 
	     * @return true if the flag was dirty before this call (indicating data needs processing).
	     * @return false if the flag was already clean.
	     */
	    SEDX_FORCE_INLINE bool CheckAndResetIfDirty() { return !flag.test_and_set(); }
	
	    /**
	     * @brief Constructs an AtomicFlag in clean state.
	     * 
	     * The flag is initialized to the "clean" state (set).
	     */
	    explicit AtomicFlag() noexcept { flag.test_and_set(); }
	    
	    /**
	     * @brief Copy constructor creates a clean flag regardless of source state.
	     * 
	     * When copying an AtomicFlag, the new instance is always initialized to 
	     * the clean state, regardless of whether the source was dirty or clean.
	     * 
	     * @param other The source AtomicFlag (its state is not copied).
	     */
	    AtomicFlag(const AtomicFlag&) noexcept {}
	    
	    /**
	     * @brief Copy assignment operator preserves the current instance's state.
	     * 
	     * The copy assignment operator doesn't modify the current instance's state.
	     * 
	     * @param other The source AtomicFlag (its state is ignored).
	     * @return Reference to this instance.
	     */
	    AtomicFlag& operator=(const AtomicFlag&) noexcept { return *this; }
	    
	    /**
	     * @brief Move constructor creates a clean flag regardless of source state.
	     * 
	     * When moving an AtomicFlag, the new instance is always initialized to 
	     * the clean state, regardless of whether the source was dirty or clean.
	     * 
	     * @param other The source AtomicFlag being moved (its state is not transferred).
	     */
	    AtomicFlag(AtomicFlag&&) noexcept {};
	    
	    /**
	     * @brief Move assignment operator preserves the current instance's state.
	     * 
	     * The move assignment operator doesn't modify the current instance's state.
	     * 
	     * @param other The source AtomicFlag being moved (its state is ignored).
	     * @return Reference to this instance.
	     */
	    AtomicFlag& operator=(AtomicFlag&&) noexcept { return *this; }
	
	private:
	    std::atomic_flag flag; ///< The underlying atomic flag that stores the state
	};


    // -------------------------------------------------------

	/**
	 * @struct Flag
	 * @brief A lightweight boolean flag class that tracks and resets dirty state.
	 * 
	 * The Flag class provides a simple mechanism to track whether something has been 
	 * marked as "dirty" (needing attention) and to atomically check and reset this state.
	 * Unlike AtomicFlag, this implementation uses a regular bool and is not thread-safe.
	 */
	struct Flag
	{
		/**
		 * @brief Sets the flag to dirty state.
		 * 
		 * Marks the flag as dirty, indicating that some action or update is needed.
		 */
		SEDX_FORCE_INLINE void SetDirty() noexcept { flag = true; }

		/**
		 * @brief Checks if the flag is dirty and atomically resets it if it is.
		 * 
		 * @return true if the flag was dirty before the reset operation.
		 * @return false if the flag was not dirty.
		 */
		SEDX_FORCE_INLINE bool CheckAndResetIfDirty() noexcept
		{
			if (flag)
				return !((flag = !flag));

		    return false;
        }

		/**
		 * @brief Checks if the flag is currently in a dirty state.
		 * 
		 * Unlike CheckAndResetIfDirty(), this method does not modify the flag's state.
		 * 
		 * @return true if the flag is dirty.
		 * @return false if the flag is not dirty.
		 */
		SEDX_FORCE_INLINE bool IsDirty() const noexcept { return flag; }

	private:
		bool flag = false; ///< Internal boolean that stores the dirty state, initially not dirty
	};


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
 * @tparam T The type of the value to convert.
 * @param arr
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

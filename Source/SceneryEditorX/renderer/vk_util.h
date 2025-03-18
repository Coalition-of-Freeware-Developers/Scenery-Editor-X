/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_util.h
* -------------------------------------------------------
* Created: 18/3/2025
* -------------------------------------------------------
*/

#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#define VK_FLAGS_NONE 0
#define DEFAULT_FENCE_TIMEOUT 100000000000

/**
* @brief Macro to check the result of a Vulkan function.
 *
 * This macro checks the result of a Vulkan function and prints an error message if the result is not VK_SUCCESS.
 *
 * @param result The result of the Vulkan function.
 * @param msg The error message to print.
 */
#define VK_CHECK_RESULT(result, message)                                                                               \
    if (result != VK_SUCCESS)                                                                                          \
    {                                                                                                                  \
        fprintf(stderr, "Error in %s:%d - %s, code %x\n", __FILE__, __LINE__, message, result);                        \
        exit(1);                                                                                                 \
    }

/**
 * @brief Macro to get the size of an array.
 *
 * This macro calculates the number of elements in a static array.
 *
 * @param arr The array whose size is to be determined.
 * @return The number of elements in the array.
 */
#define VK_CHECK(result)                                                                                               \
	    if (result != VK_SUCCESS)                                                                                      \
	    {                                                                                                              \
	        std::cout << "Vulkan Error: " << result << std::endl;                                                      \
	        __debugbreak();                                                                                            \
	        return false;                                                                                              \
	    }

// -------------------------------------------------------

/**
 * @brief Helper function to get the bits per pixel of a Vulkan format.
 * @param format Vulkan format to check.
 * @param BPP = Bits-Per-Pixel.
 * @return The bits per pixel of the given format, -1 for invalid formats.
 */
extern int getBPP(VkFormat format);

enum class ShaderSourceLanguage
{
    GLSL,
    HLSL,
    SPV,
};

enum class ShadingLanguage
{
    GLSL,
    HLSL,
};

// -------------------------------------------------------
/**
* @brief Convert a value to a string.
* @tparam T The type of the value to convert.
* @param value The value to convert.
* @return The string representation of the value.
*/
template <typename T>
std::string ToString(const T& value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

// -------------------------------------------------------

/**
 * @brief Get the string representation of a Vulkan debug message severity.
 * @param severity The Vulkan debug message severity.
 * @return The string representation of the severity.
 */
const char* getDebugSeverityStr(VkDebugUtilsMessageSeverityFlagBitsEXT severity);

/**
 * @brief Get the string representation of a Vulkan debug message type.
 * @param type The Vulkan debug message type.
 * @return The string representation of the type.
 */
const char* getDebugType(VkDebugUtilsMessageTypeFlagsEXT type);

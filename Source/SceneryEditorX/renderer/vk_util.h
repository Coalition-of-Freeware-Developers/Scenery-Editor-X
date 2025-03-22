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

// -------------------------------------------------------

#define VK_FLAGS_NONE 0
#define DEFAULT_FENCE_TIMEOUT 100000000000

// -------------------------------------------------------

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

// -------------------------------------------------------

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
 * @brief Get the string representation of a Vulkan debug message severity.
 * @param severity The Vulkan debug message severity.
 * @return The string representation of the severity.
 */
const char* VK_DEBUG_SEVERITY_STRING(VkDebugUtilsMessageSeverityFlagBitsEXT severity);

/**
 * @brief Get the string representation of a Vulkan debug message type.
 * @param type The Vulkan debug message type.
 * @return The string representation of the type.
 */
const char* VK_DEBUG_TYPE(VkDebugUtilsMessageTypeFlagsEXT type);

/**
 * @brief Get the string representation of a Vulkan error code.
 * @param errorCode The Vulkan error code.
 * @return The string representation of the error code.
 */
const char* VK_ERROR_STRING(VkResult errorCode);

/**
 * @brief Get the string representation of a Vulkan format.
 * @param format The Vulkan format.
 * @return The string representation of the format.
 */
const char* VK_FORMAT_STRING(VkFormat format);

/**
 * @brief Get the string representation of a Vulkan device type.
 * @param type The Vulkan device type.
 * @return The string representation of the device type.
 */
const char* VK_DEVICE_TYPE_STRING(VkPhysicalDeviceType type);

/**
 * @brief Get the string representation of a Vulkan color space.
 * @param colorSpace The Vulkan color space.
 * @return The string representation of the color space.
 */
const char* VK_COLOR_SPACE_STRING(VkColorSpaceKHR colorSpace);

/**
 * @brief Get the string representation of Vulkan queue flags.
 * @param flags The Vulkan queue flags.
 * @return The string representation of the queue flags.
 */
const char* VK_QUEUE_FLAGS_STRING(VkQueueFlags flags);

/**
 * @brief Get the string representation of Vulkan memory property flags.
 * @param flags The Vulkan memory property flags.
 * @return The string representation of the memory property flags.
 */
const char* VK_MEMORY_PROPERTY_FLAGS_STRING(VkMemoryPropertyFlags flags);

// -------------------------------------------------------

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
#include <SceneryEditorX/renderer/vk_device.h>
#include <vulkan/vulkan.h>

// -------------------------------------------------------

#define VK_FLAGS_NONE 0
#define DEFAULT_FENCE_TIMEOUT 100000000000

// -------------------------------------------------------

namespace SceneryEditorX
{
	class VulkanPhysicalDevice;
}

// -------------------------------------------------------

/**
 * @brief Dump GPU information to the log.
 * 
 * retrieves and logs the GPU information such as vendor, device, and version.
 */
void GPUInfoDump();

// -------------------------------------------------------

/**
 * @brief Get the string representation of a Vulkan debug message severity.
 * @param severity The Vulkan debug message severity.
 * @return The string representation of the severity.
 */
const char *VK_DEBUG_SEVERITY_STRING(VkDebugUtilsMessageSeverityFlagBitsEXT severity);

/**
 * @brief Get the string representation of a Vulkan debug message type.
 * @param type The Vulkan debug message type.
 * @return The string representation of the type.
 */
const char *VK_DEBUG_TYPE(VkDebugUtilsMessageTypeFlagsEXT type);

/**
 * @brief Get the string representation of a Vulkan error code.
 * @param errorCode The Vulkan error code.
 * @return The string representation of the error code.
 */
const char *VK_ERROR_STRING(VkResult errorCode);

/**
 * @brief Get the string representation of a Vulkan format.
 * @param format The Vulkan format.
 * @return The string representation of the format.
 */
const char *VK_FORMAT_STRING(VkFormat format);

/**
 * @brief Get the string representation of a Vulkan device type.
 * @param type The Vulkan device type.
 * @return The string representation of the device type.
 */
const char *VK_DEVICE_TYPE_STRING(VkPhysicalDeviceType type);

/**
 * @brief Get the string representation of a Vulkan color space.
 * @param colorSpace The Vulkan color space.
 * @return The string representation of the color space.
 */
const char *VK_COLOR_SPACE_STRING(VkColorSpaceKHR colorSpace);

/**
 * @brief Get the string representation of Vulkan queue flags.
 * @param flags The Vulkan queue flags.
 * @return The string representation of the queue flags.
 */
const char *VK_QUEUE_FLAGS_STRING(VkQueueFlags flags);

/**
 * @brief Get the string representation of Vulkan memory property flags.
 * @param flags The Vulkan memory property flags.
 * @return The string representation of the memory property flags.
 */
const char *VK_MEMORY_PROPERTY_FLAGS_STRING(VkMemoryPropertyFlags flags);

/**
 * @brief Get the string representation of Vulkan memory heap flags.
 * @param flags The Vulkan memory heap flags.
 * @return The string representation of the memory heap flags.
 */
const char *VK_ERROR_STRING(VkResult result);

/**
 * @brief Get the string representation of a Vulkan vendor ID.
 * @param vendorID The Vulkan vendor ID.
 * @return The string representation of the vendor ID.
 */
const char *VK_VENDER_ID_STRING(uint32_t vendorID);


std::vector<char> CompileShader(const std::filesystem::path &path);

// -------------------------------------------------------

/**
 * @brief Check the result of a Vulkan operation and log it.
 * 
 * This function checks the result of a Vulkan operation and logs an error message if the result is not VK_SUCCESS.
 * It also dumps GPU information if the result indicates a device lost error.
 *
 * @param result The result of the Vulkan operation.
 */
inline void VulkanCheckResult(VkResult result, const char *file, int line)
{
    if (result != VK_SUCCESS)                                                                                          
    {
        VULKAN_LOG_ERROR("VKResult: '{0}' in {1}:{2}", VK_ERROR_STRING(result), file, line);
        if (result == VK_ERROR_DEVICE_LOST)
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(3s);
            GPUInfoDump();
        }

    }
}

/**
* @brief Macro to check the result of a Vulkan function.
 *
 * This macro checks the result of a Vulkan function and prints an error message if the result is not VK_SUCCESS.
 *
 * @param result The result of the Vulkan function.
 * @param msg The error message to print.
 */
//#define VK_CHECK_RESULT(result, message)                                                                               \
//	if (result != VK_SUCCESS)                                                                                          \
//	{                                                                                                                  \
//		fprintf(stderr, "Error in %s:%d - %s, code %x\n", __FILE__, __LINE__, message, result);                        \
//		exit(1);                                                                                                       \
//	}

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

// Add these to vk_util.h after your existing macros

/**
 * @brief Log Vulkan result and return false if operation fails
 * 
 * This macro logs the result of a Vulkan operation and returns false if it fails.
 * Useful for functions that return a boolean success indicator.
 */
#define VK_LOG_RESULT(result, operation)                                                                               \
    {                                                                                                                  \
        Log::LogVulkanResult(result, operation);                                                                       \
        if (result != VK_SUCCESS)                                                                                      \
        {                                                                                                              \
            return false;                                                                                              \
        }                                                                                                              \
    }

/**
 * @brief Log Vulkan result and throw exception if operation fails
 * 
 * This macro logs the result of a Vulkan operation and throws a runtime_error if it fails.
 * Useful when you want to abort execution on failure.
 */
#define VK_LOG_RESULT_OR_THROW(result, operation)                                                                      \
    {                                                                                                                  \
        Log::LogVulkanResult(result, operation);                                                                       \
        if (result != VK_SUCCESS)                                                                                      \
        {                                                                                                              \
            throw std::runtime_error(operation + " failed with " + VK_ERROR_STRING(result));                           \
        }                                                                                                              \
    }


/**
* @brief Check Vulkan result and assert if it fails
 * 
 * This macro checks the result of a Vulkan operation and asserts if it fails.
 * Useful for debugging and ensuring that Vulkan operations succeed.
 */
#define VK_CHECK_RESULT(f)                                                                                             \
    {                                                                                                                  \
        VkResult res = (f);                                                                                            \
        ::VulkanCheckResult(res, __FILE__, __LINE__);                                                                  \
    }

/**
 * @brief Log Vulkan result without any control flow change
 * 
 * This macro logs the result of a Vulkan operation without affecting control flow.
 * Useful for operations where you handle the result separately.
 */
#define VK_LOG_INFO(message)                                                                                           \
    {                                                                                                                  \
        if (Log::VulkanLogger_)                                                                                        \
        {                                                                                                              \
            Log::VulkanLogger_->info(message);                                                                         \
            Log::VulkanLogger_->flush();                                                                               \
        }                                                                                                              \
    }

#define VK_LOG_WARN(message)                                                                                           \
    {                                                                                                                  \
        if (Log::VulkanLogger_)                                                                                        \
        {                                                                                                              \
            Log::VulkanLogger_->warn(message);                                                                         \
            Log::VulkanLogger_->flush();                                                                               \
        }                                                                                                              \
    }

#define VK_LOG_ERROR(message)                                                                                          \
    {                                                                                                                  \
        if (Log::VulkanLogger_)                                                                                        \
        {                                                                                                              \
            Log::VulkanLogger_->error(message);                                                                        \
            Log::VulkanLogger_->flush();                                                                               \
        }                                                                                                              \
    }

#define VK_LOG_DEBUG(message)                                                                                          \
    {                                                                                                                  \
        if (Log::VulkanLogger_)                                                                                        \
        {                                                                                                              \
            Log::VulkanLogger_->debug(message);                                                                        \
            Log::VulkanLogger_->flush();                                                                               \
        }                                                                                                              \
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



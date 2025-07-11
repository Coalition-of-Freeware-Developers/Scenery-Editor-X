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

/// -------------------------------------------------------

inline PFN_vkSetDebugUtilsObjectNameEXT fpSetDebugUtilsObjectNameEXT; ///Making it static randomly sets it to nullptr for some reason.
inline PFN_vkCmdBeginDebugUtilsLabelEXT fpCmdBeginDebugUtilsLabelEXT;
inline PFN_vkCmdEndDebugUtilsLabelEXT fpCmdEndDebugUtilsLabelEXT;
inline PFN_vkCmdInsertDebugUtilsLabelEXT fpCmdInsertDebugUtilsLabelEXT;

/// -------------------------------------------------------

void VulkanLoadDebugUtilsExtensions(VkInstance instance);
void RetrieveDiagnosticCheckpoints();

/// -------------------------------------------------------

inline void VulkanCheckResult(const VkResult result)
{
    if (result != VK_SUCCESS)
    {
        SEDX_CORE_ERROR("VkResult is '{0}'", ToString(result));
        if (result == VK_ERROR_DEVICE_LOST)
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(3s);
            //RetrieveDiagnosticCheckpoints();
            //DumpGPUInfo();
        }
        SEDX_CORE_ASSERT(result == VK_SUCCESS);
    }
}

/// -------------------------------------------------------

inline void VulkanCheckResult(const VkResult result, const char *file, int line)
{
    if (result != VK_SUCCESS)
    {
        SEDX_CORE_ERROR("VkResult is '{0}' in {1}:{2}", ToString(result), file, line);
        if (result == VK_ERROR_DEVICE_LOST)
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(3s);
            //RetrieveDiagnosticCheckpoints();
            //DumpGPUInfo();
        }
        SEDX_CORE_ASSERT(result == VK_SUCCESS);
    }
}

/// -------------------------------------------------------

/**
* @brief Macro to check the result of a Vulkan function.
 *
 * This macro checks the result of a Vulkan function and prints an error message if the result is not VK_SUCCESS.
 *
 * @param result The result of the Vulkan function.
 * @param msg The error message to print.
 */

#define VK_CHECK_RESULT(f)                                                                                             \
    {                                                                                                                  \
        VkResult result = (f);                                                                                         \
        VulkanCheckResult(result, __FILE__, __LINE__);                                                                 \
    }

/// -------------------------------------------------------

/**
 * @brief Macro to check Vulkan features.
 *
 * This macro checks if the required Vulkan features are supported by the device.
 * If any required feature is not supported, it logs an error message.
 *
 * @param requiredFeatures The required Vulkan features.
 * @param deviceFeatures The supported Vulkan features of the device.
 */
/// Helper macro to check each feature
#define CHECK_FEATURE(feature)                                                                                         \
    if (requiredFeatures.feature && !deviceFeatures.feature)                                                           \
    {                                                                                                                  \
        missingFeatures = true;                                                                                        \
		if (!missingFeaturesLog.empty()) missingFeaturesLog += ", ";                                                   \
        missingFeaturesLog += #feature;                                                                                \
        SEDX_CORE_ERROR("  Missing feature: {}", #feature);                                                            \
    }

/// ----------------------------------------------------------

/**
 * @brief Macro to get the size of an array.
 *
 * This macro calculates the number of elements in a static array.
 *
 * @param arr The array whose size is to be determined.
 * @return The number of elements in the array.
 */
#define VK_CHECK(result)                                                                                               \
		if ((result) != VK_SUCCESS)                                                                                    \
		{                                                                                                              \
			SEDX_CORE_ERROR("Vulkan Error: {}", (result));                                                             \
			__debugbreak();                                                                                            \
			return false;                                                                                              \
		}

/// ----------------------------------------------------------

/**
 * @brief Log Vulkan result and return false if operation fails
 * 
 * This macro logs the result of a Vulkan operation and returns false if it fails.
 * Useful for functions that return a boolean success indicator.
 */
#define VK_LOG_RESULT(result, operation)                                                                               \
    {                                                                                                                  \
        ::SceneryEditorX::Log::LogVulkanResult(result, operation);                                                     \
        if ((result) != VK_SUCCESS)                                                                                    \
        {                                                                                                              \
            return false;                                                                                              \
        }                                                                                                              \
    }

/// ----------------------------------------------------------

/**
 * @brief Log Vulkan result and throw exception if operation fails
 * 
 * This macro logs the result of a Vulkan operation and throws a runtime_error if it fails.
 * Useful when you want to abort execution on failure.
 */
#define VK_LOG_RESULT_OR_THROW(result, operation)                                                                      \
    {                                                                                                                  \
        ::SceneryEditorX::Log::LogVulkanResult(result, operation);                                                     \
        if ((result) != VK_SUCCESS)                                                                                    \
        {                                                                                                              \
            throw std::runtime_error((operation) + " failed with " + VK_ERROR_STRING(result));                         \
        }                                                                                                              \
    }

/// ----------------------------------------------------------

/**
 * @brief Log Vulkan result without any control flow change
 * 
 * This macro logs the result of a Vulkan operation without affecting control flow.
 * Useful for operations where you handle the result separately.
 */
#define VK_LOG_INFO(message)                                                                                           \
    {                                                                                                                  \
        if (::SceneryEditorX::Log::_VulkanLogger)                                                                      \
        {                                                                                                              \
            ::SceneryEditorX::Log::_VulkanLogger->info(message);                                                       \
            ::SceneryEditorX::Log::_VulkanLogger->flush();                                                             \
        }                                                                                                              \
    }

#define VK_LOG_WARN(message)                                                                                           \
    {                                                                                                                  \
        if (::SceneryEditorX::Log::_VulkanLogger)                                                                      \
        {                                                                                                              \
            ::SceneryEditorX::Log::_VulkanLogger->warn(message);													   \
            ::SceneryEditorX::Log::_VulkanLogger->flush();															   \
        }                                                                                                              \
    }

#define VK_LOG_ERROR(message)                                                                                          \
    {                                                                                                                  \
        if (::SceneryEditorX::Log::_VulkanLogger)                                                                      \
        {                                                                                                              \
            ::SceneryEditorX::Log::_VulkanLogger->error(message);                                                      \
            ::SceneryEditorX::Log::_VulkanLogger->flush();                                                             \
        }                                                                                                              \
    }

#define VK_LOG_DEBUG(message)                                                                                          \
    {                                                                                                                  \
        if (::SceneryEditorX::Log::_VulkanLogger)                                                                      \
        {                                                                                                              \
            ::SceneryEditorX::Log::_VulkanLogger->debug(message);                                                      \
            ::SceneryEditorX::Log::_VulkanLogger->flush();                                                             \
        }                                                                                                              \
    }

/// -------------------------------------------------------

/**
 * @brief Helper function to get the bits per pixel of a Vulkan format.
 * @param format Vulkan format to check.
 * @param BPP = Bits-Per-Pixel.
 * @return The bits per pixel of the given format, -1 for invalid formats.
 */
extern int getBPP(VkFormat format);

/// -------------------------------------------------------

/**
 * @brief Get the string representation of a Vulkan debug message severity.
 * @param severity The Vulkan debug message severity.
 * @return The string representation of the severity.
 */
const char* vkDebugSeverityString(VkDebugUtilsMessageSeverityFlagBitsEXT severity);

/**
 * @brief Get the string representation of a Vulkan debug message type.
 * @param type The Vulkan debug message type.
 * @return The string representation of the type.
 */
const char* vkDebugType(VkDebugUtilsMessageTypeFlagsEXT type);

/**
 * @brief Get the string representation of a Vulkan error code.
 * @param errorCode The Vulkan error code.
 * @return The string representation of the error code.
 */
const char* vkErrorString(VkResult errorCode);

/**
 * @brief Get the string representation of a Vulkan format.
 * @param format The Vulkan format.
 * @return The string representation of the format.
 */
const char* vkFormatString(VkFormat format);

/**
 * @brief Get the string representation of a Vulkan device type.
 * @param type The Vulkan device type.
 * @return The string representation of the device type.
 */
const char* vkDeviceTypeString(VkPhysicalDeviceType type);

/**
 * @brief Get the string representation of a Vulkan color space.
 * @param colorSpace The Vulkan color space.
 * @return The string representation of the color space.
 */
const char* vkColorSpaceString(VkColorSpaceKHR colorSpace);

/**
 * @brief Get the string representation of Vulkan queue flags.
 * @param flags The Vulkan queue flags.
 * @return The string representation of the queue flags.
 */
const char* vkQueueFlagsString(VkQueueFlags flags);

/**
 * @brief Get the string representation of Vulkan memory property flags.
 * @param flags The Vulkan memory property flags.
 * @return The string representation of the memory property flags.
 */
const char* vkMemoryPropertyFlagsString(VkMemoryPropertyFlags flags);

/// -------------------------------------------------------

inline float HaltonSeq(uint32_t i, const uint32_t b)
{
    float f = 1.0f;
    float r = 0.0f;

    while (i > 0)
    {
        f /= static_cast<float>(b);
        r = r + f * static_cast<float>(i % b);
        i = static_cast<uint32_t>(floorf(static_cast<float>(i) / static_cast<float>(b)));
    }

    return r;
}

/// -------------------------------------------------------

static void SetDebugUtilsObjectName(const VkDevice device, const VkObjectType objectType, const std::string& name, const void* handle)
{
	VkDebugUtilsObjectNameInfoEXT nameInfo;
	nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	nameInfo.objectType = objectType;
	nameInfo.pObjectName = name.c_str();
	nameInfo.objectHandle = (uint64_t)handle;
	nameInfo.pNext = VK_NULL_HANDLE;

	VK_CHECK_RESULT(fpSetDebugUtilsObjectNameEXT(device, &nameInfo))
}

/**
 * @brief Get the string representation of a Vulkan object type.
 * @param objectType The Vulkan object type.
 * @return The string representation of the object type.
 */
const char *VkObjectTypeToString(VkObjectType objectType);

void SetDeviceVendorName(const uint32_t vendorID);

void GetDeviceName(const std::string &deviceName);

namespace SceneryEditorX::Util
{

	struct ResourceAllocationCounts
	{
	    uint32_t Samplers = 0;
	};
	
	ResourceAllocationCounts &GetResourceAllocationCounts();

}

/// -------------------------------------------------------


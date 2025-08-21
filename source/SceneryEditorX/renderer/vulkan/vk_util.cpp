/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_util.cpp
* -------------------------------------------------------
* Created: 18/3/2025
* -------------------------------------------------------
*/
// ReSharper disable StringLiteralTypo
#include "vk_util.h"
#include "vk_data.h"

/// -------------------------------------------------------
namespace SceneryEditorX::Utils
{
	/**
	 * @brief Get the string representation of a Vulkan physical device type.
	 *
	 * This function returns a string representation of the given Vulkan physical device type.
	 * It is used for debugging purposes to identify the type of physical device being used.
	 *
	 * @param type The Vulkan physical device type.
	 * @return const char* A string representation of the physical device type.
	 */
	// ReSharper disable once CppNotAllPathsReturnValue
	const char* vkDeviceTypeString(const VkPhysicalDeviceType type)
	{
	    switch (type)
	    {
	    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU :	return "Integrated GPU";
	    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU :		return "Discrete GPU";
	    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU :		return "Virtual GPU";
	    case VK_PHYSICAL_DEVICE_TYPE_CPU :				return "CPU";
	    case VK_PHYSICAL_DEVICE_TYPE_OTHER:             return "Other";
	    case VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM:
	        break;
	    default :										return "Unknown";
	    }
	}
	
	/**
	 * @brief Get the string representation of a Vulkan color space.
	 *
	 * This function returns a string representation of the given Vulkan color space.
	 * It is used for debugging purposes to identify the color space used in Vulkan operations.
	 *
	 * @param colorSpace The Vulkan color space.
	 * @return const char* A string representation of the color space.
	 */
	const char* vkColorSpaceString(const VkColorSpaceKHR colorSpace)
	{
	    switch (colorSpace)
	    {
	    case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR :			return "SRGB_NONLINEAR";
	    case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT :		return "DISPLAY_P3_NONLINEAR";
	    case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT :		return "EXTENDED_SRGB_LINEAR";
	    case VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT :			return "DISPLAY_P3_LINEAR";
	    case VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT :			return "DCI_P3_NONLINEAR";
	    case VK_COLOR_SPACE_BT709_LINEAR_EXT :				return "BT709_LINEAR";
	    case VK_COLOR_SPACE_BT709_NONLINEAR_EXT :			return "BT709_NONLINEAR";
	    case VK_COLOR_SPACE_BT2020_LINEAR_EXT :				return "BT2020_LINEAR";
	    case VK_COLOR_SPACE_HDR10_ST2084_EXT :				return "HDR10_ST2084";
	    case VK_COLOR_SPACE_DOLBYVISION_EXT :				return "DOLBYVISION";
	    case VK_COLOR_SPACE_HDR10_HLG_EXT :					return "HDR10_HLG";
	    case VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT :			return "ADOBERGB_LINEAR";
	    case VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT :		return "ADOBERGB_NONLINEAR";
	    case VK_COLOR_SPACE_PASS_THROUGH_EXT :				return "PASS_THROUGH";
	    case VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT :	return "EXTENDED_SRGB_NONLINEAR";
	    case VK_COLOR_SPACE_DISPLAY_NATIVE_AMD :			return "DISPLAY_NATIVE_AMD";
	    case VK_COLOR_SPACE_MAX_ENUM_KHR :					return "MAX_ENUM_KHR";
	    default :											return "Unknown";
	    }
	}
	
	/**
	 * @brief Get the string representation of Vulkan queue flags.
	 *
	 * This function returns a string representation of the given Vulkan queue flags.
	 * It is used for debugging purposes to identify the capabilities of a Vulkan queue.
	 *
	 * @param flags The Vulkan queue flags.
	 * @return const char* A string representation of the queue flags.
	 */
	const char* vkQueueFlagsString(const VkQueueFlags flags)
	{
	    switch (flags)
	    {
	    case VK_QUEUE_GRAPHICS_BIT :		return "Graphics";
	    case VK_QUEUE_COMPUTE_BIT :			return "Compute";
	    case VK_QUEUE_TRANSFER_BIT :		return "Transfer";
	    case VK_QUEUE_SPARSE_BINDING_BIT :	return "Sparse Binding";
	    case VK_QUEUE_PROTECTED_BIT :		return "Protected";
	    default :							return "Unknown";
	    }
	}
	
	/**
	 * @brief Get the string representation of Vulkan memory property flags.
	 *
	 * This function returns a string representation of the given Vulkan memory property flags.
	 * It is used for debugging purposes to identify the properties of memory allocations.
	 *
	 * @param flags The Vulkan memory property flags.
	 * @return const char* A string representation of the memory property flags.
	 */
	const char* vkMemoryPropertyFlagsString(VkMemoryPropertyFlags flags)
	{
	    switch (flags)
	    {
	    case VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT :			return "Device Local";
	    case VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT :			return "Host Visible";
	    case VK_MEMORY_PROPERTY_HOST_COHERENT_BIT :			return "Host Coherent";
	    case VK_MEMORY_PROPERTY_HOST_CACHED_BIT :			return "Host Cached";
	    case VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT :		return "Lazily Allocated";
	    case VK_MEMORY_PROPERTY_PROTECTED_BIT :				return "Protected";
	    case VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD :	return "Device Coherent";
	    case VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD :	return "Device Uncached";
	    case VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV :		return "RDMA Capable";
	    case VK_MEMORY_PROPERTY_FLAG_BITS_MAX_ENUM :		return "Flag Bits Max Enum";
	    default :											return "Unknown";
	    }
	}
	
	/**
	 * @brief Get the string representation of a Vulkan debug message severity.
	 *
	 * This function returns a string representation of the given Vulkan debug message severity.
	 * It is used for debugging purposes to identify the severity level of messages being logged.
	 *
	 * @param severity The Vulkan debug message severity flags.
	 * @return const char* A string representation of the debug message severity.
	 */
	const char* vkDebugSeverityString(const VkDebugUtilsMessageSeverityFlagBitsEXT severity)
	{
	    switch (severity)
	    {
	    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT :			return "VERBOSE";
	    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT :				return "INFO";
	    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT :			return "WARNING";
	    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT :			return "ERROR";
	    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:	break;
	    default :														exit(1);
	    }
	
	    return "No Known Severity";
	}
	
	/**
	 * @brief Get the string representation of a Vulkan debug message type.
	 *
	 * This function returns a string representation of the given Vulkan debug message type.
	 * It is used for debugging purposes to identify the type of message being logged.
	 *
	 * @param type The Vulkan debug message type flags.
	 * @return const char* A string representation of the debug message type.
	 */
	const char* vkDebugType(const VkDebugUtilsMessageTypeFlagsEXT type)
	{
	    switch (type)
	    {
	    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT :					return "GENERAL";
	    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT :				return "VALIDATION";
	    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT :				return "PERFORMANCE";
	#ifdef _WIN64
	    case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT :	return "DEVICE ADDRESS BINDING";
	#endif
	    default :
	        exit(1);
	    }
	
	    return "No Known Type";
	}
	
	/// -------------------------------------------------------
	
	/**
	 * @brief Load Vulkan debug utils extensions.
	 *
	 * This function loads the Vulkan debug utils extensions by retrieving
	 * function pointers for various debug utilities functions.
	 * If the function pointers cannot be retrieved, it assigns
	 * default no-op implementations to ensure the application can still run without debug utilities.
	 *
	 * @param instance The Vulkan instance from which to load the debug utils extensions.
	 */
	void VulkanLoadDebugUtilsExtensions(const VkInstance instance)
	{
	    fpSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
	    if (fpSetDebugUtilsObjectNameEXT == nullptr)
	        fpSetDebugUtilsObjectNameEXT = [](VkDevice device, const VkDebugUtilsObjectNameInfoEXT *pNameInfo) { return VK_SUCCESS; };
	
	    fpCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT");
	    if (fpCmdBeginDebugUtilsLabelEXT == nullptr)
	        fpCmdBeginDebugUtilsLabelEXT = [](VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT *pLabelInfo) {};
	
	    fpCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT");
	    if (fpCmdEndDebugUtilsLabelEXT == nullptr)
	        fpCmdEndDebugUtilsLabelEXT = [](VkCommandBuffer commandBuffer) {};
	
	    fpCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT");
	    if (fpCmdInsertDebugUtilsLabelEXT == nullptr)
	        fpCmdInsertDebugUtilsLabelEXT = [](VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT *pLabelInfo) {};
	}
	
	/// -------------------------------------------------------
	
	/**
	* @brief Get the number of bytes per pixel for a given format.
	 *
	 * This function returns the number of bytes per pixel for a given Vulkan format.
	 *
	 * @param format The Vulkan format.
	 * @return uint32_t The number of bytes per pixel.
	 */
	int32_t getBPP(const VkFormat format)
	{
	    switch (format)
	    {
	    case VK_FORMAT_R4G4_UNORM_PACK8 :
	        return 8;
	    case VK_FORMAT_R4G4B4A4_UNORM_PACK16 :
	    case VK_FORMAT_B4G4R4A4_UNORM_PACK16 :
	    case VK_FORMAT_R5G6B5_UNORM_PACK16 :
	    case VK_FORMAT_B5G6R5_UNORM_PACK16 :
	    case VK_FORMAT_R5G5B5A1_UNORM_PACK16 :
	    case VK_FORMAT_B5G5R5A1_UNORM_PACK16 :
	    case VK_FORMAT_A1R5G5B5_UNORM_PACK16 :
	        return 16;
	    case VK_FORMAT_R8_UNORM :
	    case VK_FORMAT_R8_SNORM :
	    case VK_FORMAT_R8_USCALED :
	    case VK_FORMAT_R8_SSCALED :
	    case VK_FORMAT_R8_UINT :
	    case VK_FORMAT_R8_SINT :
	    case VK_FORMAT_R8_SRGB :
	        return 8;
	    case VK_FORMAT_R8G8_UNORM :
	    case VK_FORMAT_R8G8_SNORM :
	    case VK_FORMAT_R8G8_USCALED :
	    case VK_FORMAT_R8G8_SSCALED :
	    case VK_FORMAT_R8G8_UINT :
	    case VK_FORMAT_R8G8_SINT :
	    case VK_FORMAT_R8G8_SRGB :
	        return 16;
	    case VK_FORMAT_R8G8B8_UNORM :
	    case VK_FORMAT_R8G8B8_SNORM :
	    case VK_FORMAT_R8G8B8_USCALED :
	    case VK_FORMAT_R8G8B8_SSCALED :
	    case VK_FORMAT_R8G8B8_UINT :
	    case VK_FORMAT_R8G8B8_SINT :
	    case VK_FORMAT_R8G8B8_SRGB :
	    case VK_FORMAT_B8G8R8_UNORM :
	    case VK_FORMAT_B8G8R8_SNORM :
	    case VK_FORMAT_B8G8R8_USCALED :
	    case VK_FORMAT_B8G8R8_SSCALED :
	    case VK_FORMAT_B8G8R8_UINT :
	    case VK_FORMAT_B8G8R8_SINT :
	    case VK_FORMAT_B8G8R8_SRGB :
	        return 24;
	    case VK_FORMAT_R8G8B8A8_UNORM :
	    case VK_FORMAT_R8G8B8A8_SNORM :
	    case VK_FORMAT_R8G8B8A8_USCALED :
	    case VK_FORMAT_R8G8B8A8_SSCALED :
	    case VK_FORMAT_R8G8B8A8_UINT :
	    case VK_FORMAT_R8G8B8A8_SINT :
	    case VK_FORMAT_R8G8B8A8_SRGB :
	    case VK_FORMAT_B8G8R8A8_UNORM :
	    case VK_FORMAT_B8G8R8A8_SNORM :
	    case VK_FORMAT_B8G8R8A8_USCALED :
	    case VK_FORMAT_B8G8R8A8_SSCALED :
	    case VK_FORMAT_B8G8R8A8_UINT :
	    case VK_FORMAT_B8G8R8A8_SINT :
	    case VK_FORMAT_B8G8R8A8_SRGB :
	    case VK_FORMAT_A8B8G8R8_UNORM_PACK32 :
	    case VK_FORMAT_A8B8G8R8_SNORM_PACK32 :
	    case VK_FORMAT_A8B8G8R8_USCALED_PACK32 :
	    case VK_FORMAT_A8B8G8R8_SSCALED_PACK32 :
	    case VK_FORMAT_A8B8G8R8_UINT_PACK32 :
	    case VK_FORMAT_A8B8G8R8_SINT_PACK32 :
	    case VK_FORMAT_A8B8G8R8_SRGB_PACK32 :
	        return 32;
	    case VK_FORMAT_A2R10G10B10_UNORM_PACK32 :
	    case VK_FORMAT_A2R10G10B10_SNORM_PACK32 :
	    case VK_FORMAT_A2R10G10B10_USCALED_PACK32 :
	    case VK_FORMAT_A2R10G10B10_SSCALED_PACK32 :
	    case VK_FORMAT_A2R10G10B10_UINT_PACK32 :
	    case VK_FORMAT_A2R10G10B10_SINT_PACK32 :
	    case VK_FORMAT_A2B10G10R10_UNORM_PACK32 :
	    case VK_FORMAT_A2B10G10R10_SNORM_PACK32 :
	    case VK_FORMAT_A2B10G10R10_USCALED_PACK32 :
	    case VK_FORMAT_A2B10G10R10_SSCALED_PACK32 :
	    case VK_FORMAT_A2B10G10R10_UINT_PACK32 :
	    case VK_FORMAT_A2B10G10R10_SINT_PACK32 :
	        return 32;
	    case VK_FORMAT_R16_UNORM :
	    case VK_FORMAT_R16_SNORM :
	    case VK_FORMAT_R16_USCALED :
	    case VK_FORMAT_R16_SSCALED :
	    case VK_FORMAT_R16_UINT :
	    case VK_FORMAT_R16_SINT :
	    case VK_FORMAT_R16_SFLOAT :
	        return 16;
	    case VK_FORMAT_R16G16_UNORM :
	    case VK_FORMAT_R16G16_SNORM :
	    case VK_FORMAT_R16G16_USCALED :
	    case VK_FORMAT_R16G16_SSCALED :
	    case VK_FORMAT_R16G16_UINT :
	    case VK_FORMAT_R16G16_SINT :
	    case VK_FORMAT_R16G16_SFLOAT :
	        return 32;
	    case VK_FORMAT_R16G16B16_UNORM :
	    case VK_FORMAT_R16G16B16_SNORM :
	    case VK_FORMAT_R16G16B16_USCALED :
	    case VK_FORMAT_R16G16B16_SSCALED :
	    case VK_FORMAT_R16G16B16_UINT :
	    case VK_FORMAT_R16G16B16_SINT :
	    case VK_FORMAT_R16G16B16_SFLOAT :
	        return 48;
	    case VK_FORMAT_R16G16B16A16_UNORM :
	    case VK_FORMAT_R16G16B16A16_SNORM :
	    case VK_FORMAT_R16G16B16A16_USCALED :
	    case VK_FORMAT_R16G16B16A16_SSCALED :
	    case VK_FORMAT_R16G16B16A16_UINT :
	    case VK_FORMAT_R16G16B16A16_SINT :
	    case VK_FORMAT_R16G16B16A16_SFLOAT :
	        return 64;
	    case VK_FORMAT_R32_UINT :
	    case VK_FORMAT_R32_SINT :
	    case VK_FORMAT_R32_SFLOAT :
	        return 32;
	    case VK_FORMAT_R32G32_UINT :
	    case VK_FORMAT_R32G32_SINT :
	    case VK_FORMAT_R32G32_SFLOAT :
	        return 64;
	    case VK_FORMAT_R32G32B32_UINT :
	    case VK_FORMAT_R32G32B32_SINT :
	    case VK_FORMAT_R32G32B32_SFLOAT :
	        return 96;
	    case VK_FORMAT_R32G32B32A32_UINT :
	    case VK_FORMAT_R32G32B32A32_SINT :
	    case VK_FORMAT_R32G32B32A32_SFLOAT :
	        return 128;
	    case VK_FORMAT_R64_UINT :
	    case VK_FORMAT_R64_SINT :
	    case VK_FORMAT_R64_SFLOAT :
	        return 64;
	    case VK_FORMAT_R64G64_UINT :
	    case VK_FORMAT_R64G64_SINT :
	    case VK_FORMAT_R64G64_SFLOAT :
	        return 128;
	    case VK_FORMAT_R64G64B64_UINT :
	    case VK_FORMAT_R64G64B64_SINT :
	    case VK_FORMAT_R64G64B64_SFLOAT :
	        return 192;
	    case VK_FORMAT_R64G64B64A64_UINT :
	    case VK_FORMAT_R64G64B64A64_SINT :
	    case VK_FORMAT_R64G64B64A64_SFLOAT :
	        return 256;
	    case VK_FORMAT_B10G11R11_UFLOAT_PACK32 :	return 32;
	    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32 :		return 32;
	    case VK_FORMAT_D16_UNORM :					return 16;
	    case VK_FORMAT_X8_D24_UNORM_PACK32 :		return 32;
	    case VK_FORMAT_D32_SFLOAT :					return 32;
	    case VK_FORMAT_S8_UINT :					return 8;
	    case VK_FORMAT_D16_UNORM_S8_UINT :			return 24;
	    case VK_FORMAT_D24_UNORM_S8_UINT :			return 32;
	    case VK_FORMAT_D32_SFLOAT_S8_UINT :			return 40;
	    case VK_FORMAT_UNDEFINED : default :
	        return -1;
	    }
	}
	
	/**
	 * @brief Get the number of bytes per pixel for a given format.
	 *
	 * This function returns the number of bytes per pixel for a given Vulkan format.
	 *
	 * @param layout The Vulkan image layout.
	 * @param format The Vulkan format.
	 * @return uint32_t The number of bytes per pixel.
	 */
	VkAccessFlags VK_ACCESS_FLAGS(const VkImageLayout layout)
	{
	    switch (layout)
	    {
	    case VK_IMAGE_LAYOUT_UNDEFINED :
	    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR :								return 0;
	    case VK_IMAGE_LAYOUT_PREINITIALIZED :								return VK_ACCESS_HOST_WRITE_BIT;
	    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL :						return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL :						return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	    case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR : return VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
	    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL :						return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
	    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL :							return VK_ACCESS_TRANSFER_READ_BIT;
	    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL :							return VK_ACCESS_TRANSFER_WRITE_BIT;
	    case VK_IMAGE_LAYOUT_GENERAL : assert(false && "Don't know how to get a meaningful VkAccessFlags for VK_IMAGE_LAYOUT_GENERAL! Don't use it!");
	        return 0;
	    default : assert(false);
	        return 0;
	    }
	}
	
	/**
	 * @brief Get the number of bytes per pixel for a given format.
	 *
	 * This function returns the number of bytes per pixel for a given Vulkan format.
	 *
	 * @param layout
	 * @param format The Vulkan format.
	 * @return uint32_t The number of bytes per pixel.
	 */
	VkPipelineStageFlags VK_PIPELINE_STAGE_FLAGS(const VkImageLayout layout)
	{
	    switch (layout)
	    {
	    case VK_IMAGE_LAYOUT_UNDEFINED :									return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	    case VK_IMAGE_LAYOUT_PREINITIALIZED :								return VK_PIPELINE_STAGE_HOST_BIT;
	    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL :							return VK_PIPELINE_STAGE_TRANSFER_BIT;
	    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL :						return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL :						return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	    case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR : return VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
	    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL :						return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR :								return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	    case VK_IMAGE_LAYOUT_GENERAL : assert(false && "Don't know how to get a meaningful VkPipelineStageFlags for VK_IMAGE_LAYOUT_GENERAL! Don't use it!");
	        return 0;
	    default : assert(false);
	        return 0;
	    }
	}
	
	/**
	 * @brief Get the string representation of a Vulkan format.
	 *
	 * This function returns a string representation of a given Vulkan format.
	 *
	 * @param format The Vulkan format.
	 * @return const char* The string representation of the format.
	 */
	const char *vkFormatString(const VkFormat format)
	{
	    switch (format)
	    {
	    case VK_FORMAT_UNDEFINED :										return "VK_FORMAT_UNDEFINED";
	    case VK_FORMAT_R4G4_UNORM_PACK8 :								return "VK_FORMAT_R4G4_UNORM_PACK8";
	    case VK_FORMAT_R4G4B4A4_UNORM_PACK16 :							return "VK_FORMAT_R4G4B4A4_UNORM_PACK16";
	    case VK_FORMAT_B4G4R4A4_UNORM_PACK16 :							return "VK_FORMAT_B4G4R4A4_UNORM_PACK16";
	    case VK_FORMAT_R5G6B5_UNORM_PACK16 :							return "VK_FORMAT_R5G6B5_UNORM_PACK16";
	    case VK_FORMAT_B5G6R5_UNORM_PACK16 :							return "VK_FORMAT_B5G6R5_UNORM_PACK16";
	    case VK_FORMAT_R5G5B5A1_UNORM_PACK16 :							return "VK_FORMAT_R5G5B5A1_UNORM_PACK16";
	    case VK_FORMAT_B5G5R5A1_UNORM_PACK16 :							return "VK_FORMAT_B5G5R5A1_UNORM_PACK16";
	    case VK_FORMAT_A1R5G5B5_UNORM_PACK16 :							return "VK_FORMAT_A1R5G5B5_UNORM_PACK16";
	    case VK_FORMAT_R8_UNORM :										return "VK_FORMAT_R8_UNORM";
	    case VK_FORMAT_R8_SNORM :										return "VK_FORMAT_R8_SNORM";
	    case VK_FORMAT_R8_USCALED :										return "VK_FORMAT_R8_USCALED";
	    case VK_FORMAT_R8_SSCALED :										return "VK_FORMAT_R8_SSCALED";
	    case VK_FORMAT_R8_UINT :										return "VK_FORMAT_R8_UINT";
	    case VK_FORMAT_R8_SINT :										return "VK_FORMAT_R8_SINT";
	    case VK_FORMAT_R8_SRGB :										return "VK_FORMAT_R8_SRGB";
	    case VK_FORMAT_R8G8_UNORM :										return "VK_FORMAT_R8G8_UNORM";
	    case VK_FORMAT_R8G8_SNORM :										return "VK_FORMAT_R8G8_SNORM";
	    case VK_FORMAT_R8G8_USCALED :									return "VK_FORMAT_R8G8_USCALED";
	    case VK_FORMAT_R8G8_SSCALED :									return "VK_FORMAT_R8G8_SSCALED";
	    case VK_FORMAT_R8G8_UINT :										return "VK_FORMAT_R8G8_UINT";
	    case VK_FORMAT_R8G8_SINT :										return "VK_FORMAT_R8G8_SINT";
	    case VK_FORMAT_R8G8_SRGB :										return "VK_FORMAT_R8G8_SRGB";
	    case VK_FORMAT_R8G8B8_UNORM :									return "VK_FORMAT_R8G8B8_UNORM";
	    case VK_FORMAT_R8G8B8_SNORM :									return "VK_FORMAT_R8G8B8_SNORM";
	    case VK_FORMAT_R8G8B8_USCALED :									return "VK_FORMAT_R8G8B8_USCALED";
	    case VK_FORMAT_R8G8B8_SSCALED :									return "VK_FORMAT_R8G8B8_SSCALED";
	    case VK_FORMAT_R8G8B8_UINT :									return "VK_FORMAT_R8G8B8_UINT";
	    case VK_FORMAT_R8G8B8_SINT :									return "VK_FORMAT_R8G8B8_SINT";
	    case VK_FORMAT_R8G8B8_SRGB :									return "VK_FORMAT_R8G8B8_SRGB";
	    case VK_FORMAT_B8G8R8_UNORM :									return "VK_FORMAT_B8G8R8_UNORM";
	    case VK_FORMAT_B8G8R8_SNORM :									return "VK_FORMAT_B8G8R8_SNORM";
	    case VK_FORMAT_B8G8R8_USCALED :									return "VK_FORMAT_B8G8R8_USCALED";
	    case VK_FORMAT_B8G8R8_SSCALED :									return "VK_FORMAT_B8G8R8_SSCALED";
	    case VK_FORMAT_B8G8R8_UINT :									return "VK_FORMAT_B8G8R8_UINT";
	    case VK_FORMAT_B8G8R8_SINT :									return "VK_FORMAT_B8G8R8_SINT";
	    case VK_FORMAT_B8G8R8_SRGB :									return "VK_FORMAT_B8G8R8_SRGB";
	    case VK_FORMAT_R8G8B8A8_UNORM :									return "VK_FORMAT_R8G8B8A8_UNORM";
	    case VK_FORMAT_R8G8B8A8_SNORM :									return "VK_FORMAT_R8G8B8A8_SNORM";
	    case VK_FORMAT_R8G8B8A8_USCALED :								return "VK_FORMAT_R8G8B8A8_USCALED";
	    case VK_FORMAT_R8G8B8A8_SSCALED :								return "VK_FORMAT_R8G8B8A8_SSCALED";
	    case VK_FORMAT_R8G8B8A8_UINT :									return "VK_FORMAT_R8G8B8A8_UINT";
	    case VK_FORMAT_R8G8B8A8_SINT :									return "VK_FORMAT_R8G8B8A8_SINT";
	    case VK_FORMAT_R8G8B8A8_SRGB :									return "VK_FORMAT_R8G8B8A8_SRGB";
	    case VK_FORMAT_B8G8R8A8_UNORM :									return "VK_FORMAT_B8G8R8A8_UNORM";
	    case VK_FORMAT_B8G8R8A8_SNORM :									return "VK_FORMAT_B8G8R8A8_SNORM";
	    case VK_FORMAT_B8G8R8A8_USCALED :								return "VK_FORMAT_B8G8R8A8_USCALED";
	    case VK_FORMAT_B8G8R8A8_SSCALED :								return "VK_FORMAT_B8G8R8A8_SSCALED";
	    case VK_FORMAT_B8G8R8A8_UINT :									return "VK_FORMAT_B8G8R8A8_UINT";
	    case VK_FORMAT_B8G8R8A8_SINT :									return "VK_FORMAT_B8G8R8A8_SINT";
	    case VK_FORMAT_B8G8R8A8_SRGB :									return "VK_FORMAT_B8G8R8A8_SRGB";
	    case VK_FORMAT_A8B8G8R8_UNORM_PACK32 :							return "VK_FORMAT_A8B8G8R8_UNORM_PACK32";
	    case VK_FORMAT_A8B8G8R8_SNORM_PACK32 :							return "VK_FORMAT_A8B8G8R8_SNORM_PACK32";
	    case VK_FORMAT_A8B8G8R8_USCALED_PACK32 :						return "VK_FORMAT_A8B8G8R8_USCALED_PACK32";
	    case VK_FORMAT_A8B8G8R8_SSCALED_PACK32 :						return "VK_FORMAT_A8B8G8R8_SSCALED_PACK32";
	    case VK_FORMAT_A8B8G8R8_UINT_PACK32 :							return "VK_FORMAT_A8B8G8R8_UINT_PACK32";
	    case VK_FORMAT_A8B8G8R8_SINT_PACK32 :							return "VK_FORMAT_A8B8G8R8_SINT_PACK32";
	    case VK_FORMAT_A8B8G8R8_SRGB_PACK32 :							return "VK_FORMAT_A8B8G8R8_SRGB_PACK32";
	    case VK_FORMAT_A2R10G10B10_UNORM_PACK32 :						return "VK_FORMAT_A2R10G10B10_UNORM_PACK32";
	    case VK_FORMAT_A2R10G10B10_SNORM_PACK32 :						return "VK_FORMAT_A2R10G10B10_SNORM_PACK32";
	    case VK_FORMAT_A2R10G10B10_USCALED_PACK32 :						return "VK_FORMAT_A2R10G10B10_USCALED_PACK32";
	    case VK_FORMAT_A2R10G10B10_SSCALED_PACK32 :						return "VK_FORMAT_A2R10G10B10_SSCALED_PACK32";
	    case VK_FORMAT_A2R10G10B10_UINT_PACK32 :						return "VK_FORMAT_A2R10G10B10_UINT_PACK32";
	    case VK_FORMAT_A2R10G10B10_SINT_PACK32 :						return "VK_FORMAT_A2R10G10B10_SINT_PACK32";
	    case VK_FORMAT_A2B10G10R10_UNORM_PACK32 :						return "VK_FORMAT_A2B10G10R10_UNORM_PACK32";
	    case VK_FORMAT_A2B10G10R10_SNORM_PACK32 :						return "VK_FORMAT_A2B10G10R10_SNORM_PACK32";
	    case VK_FORMAT_A2B10G10R10_USCALED_PACK32 :						return "VK_FORMAT_A2B10G10R10_USCALED_PACK32";
	    case VK_FORMAT_A2B10G10R10_SSCALED_PACK32 :						return "VK_FORMAT_A2B10G10R10_SSCALED_PACK32";
	    case VK_FORMAT_A2B10G10R10_UINT_PACK32 :						return "VK_FORMAT_A2B10G10R10_UINT_PACK32";
	    case VK_FORMAT_A2B10G10R10_SINT_PACK32 :						return "VK_FORMAT_A2B10G10R10_SINT_PACK32";
	    case VK_FORMAT_R16_UNORM :										return "VK_FORMAT_R16_UNORM";
	    case VK_FORMAT_R16_SNORM :										return "VK_FORMAT_R16_SNORM";
	    case VK_FORMAT_R16_USCALED :									return "VK_FORMAT_R16_USCALED";
	    case VK_FORMAT_R16_SSCALED :									return "VK_FORMAT_R16_SSCALED";
	    case VK_FORMAT_R16_UINT :										return "VK_FORMAT_R16_UINT";
	    case VK_FORMAT_R16_SINT :										return "VK_FORMAT_R16_SINT";
	    case VK_FORMAT_R16_SFLOAT :										return "VK_FORMAT_R16_SFLOAT";
	    case VK_FORMAT_R16G16_UNORM :									return "VK_FORMAT_R16G16_UNORM";
	    case VK_FORMAT_R16G16_SNORM :									return "VK_FORMAT_R16G16_SNORM";
	    case VK_FORMAT_R16G16_USCALED :									return "VK_FORMAT_R16G16_USCALED";
	    case VK_FORMAT_R16G16_SSCALED :									return "VK_FORMAT_R16G16_SSCALED";
	    case VK_FORMAT_R16G16_UINT :									return "VK_FORMAT_R16G16_UINT";
	    case VK_FORMAT_R16G16_SINT :									return "VK_FORMAT_R16G16_SINT";
	    case VK_FORMAT_R16G16_SFLOAT :									return "VK_FORMAT_R16G16_SFLOAT";
	    case VK_FORMAT_R16G16B16_UNORM :								return "VK_FORMAT_R16G16B16_UNORM";
	    case VK_FORMAT_R16G16B16_SNORM :								return "VK_FORMAT_R16G16B16_SNORM";
	    case VK_FORMAT_R16G16B16_USCALED :								return "VK_FORMAT_R16G16B16_USCALED";
	    case VK_FORMAT_R16G16B16_SSCALED :								return "VK_FORMAT_R16G16B16_SSCALED";
	    case VK_FORMAT_R16G16B16_UINT :									return "VK_FORMAT_R16G16B16_UINT";
	    case VK_FORMAT_R16G16B16_SINT :									return "VK_FORMAT_R16G16B16_SINT";
	    case VK_FORMAT_R16G16B16_SFLOAT :								return "VK_FORMAT_R16G16B16_SFLOAT";
	    case VK_FORMAT_R16G16B16A16_UNORM :								return "VK_FORMAT_R16G16B16A16_UNORM";
	    case VK_FORMAT_R16G16B16A16_SNORM :								return "VK_FORMAT_R16G16B16A16_SNORM";
	    case VK_FORMAT_R16G16B16A16_USCALED :							return "VK_FORMAT_R16G16B16A16_USCALED";
	    case VK_FORMAT_R16G16B16A16_SSCALED :							return "VK_FORMAT_R16G16B16A16_SSCALED";
	    case VK_FORMAT_R16G16B16A16_UINT :								return "VK_FORMAT_R16G16B16A16_UINT";
	    case VK_FORMAT_R16G16B16A16_SINT :								return "VK_FORMAT_R16G16B16A16_SINT";
	    case VK_FORMAT_R16G16B16A16_SFLOAT :							return "VK_FORMAT_R16G16B16A16_SFLOAT";
	    case VK_FORMAT_R32_UINT :										return "VK_FORMAT_R32_UINT";
	    case VK_FORMAT_R32_SINT :										return "VK_FORMAT_R32_SINT";
	    case VK_FORMAT_R32_SFLOAT :										return "VK_FORMAT_R32_SFLOAT";
	    case VK_FORMAT_R32G32_UINT :									return "VK_FORMAT_R32G32_UINT";
	    case VK_FORMAT_R32G32_SINT :									return "VK_FORMAT_R32G32_SINT";
	    case VK_FORMAT_R32G32_SFLOAT :									return "VK_FORMAT_R32G32_SFLOAT";
	    case VK_FORMAT_R32G32B32_UINT :									return "VK_FORMAT_R32G32B32_UINT";
	    case VK_FORMAT_R32G32B32_SINT :									return "VK_FORMAT_R32G32B32_SINT";
	    case VK_FORMAT_R32G32B32_SFLOAT :								return "VK_FORMAT_R32G32B32_SFLOAT";
	    case VK_FORMAT_R32G32B32A32_UINT :								return "VK_FORMAT_R32G32B32A32_UINT";
	    case VK_FORMAT_R32G32B32A32_SINT :								return "VK_FORMAT_R32G32B32A32_SINT";
	    case VK_FORMAT_R32G32B32A32_SFLOAT :							return "VK_FORMAT_R32G32B32A32_SFLOAT";
	    case VK_FORMAT_R64_UINT :										return "VK_FORMAT_R64_UINT";
	    case VK_FORMAT_R64_SINT :										return "VK_FORMAT_R64_SINT";
	    case VK_FORMAT_R64_SFLOAT :										return "VK_FORMAT_R64_SFLOAT";
	    case VK_FORMAT_R64G64_UINT :									return "VK_FORMAT_R64G64_UINT";
	    case VK_FORMAT_R64G64_SINT :									return "VK_FORMAT_R64G64_SINT";
	    case VK_FORMAT_R64G64_SFLOAT :									return "VK_FORMAT_R64G64_SFLOAT";
	    case VK_FORMAT_R64G64B64_UINT :									return "VK_FORMAT_R64G64B64_UINT";
	    case VK_FORMAT_R64G64B64_SINT :									return "VK_FORMAT_R64G64B64_SINT";
	    case VK_FORMAT_R64G64B64_SFLOAT :								return "VK_FORMAT_R64G64B64_SFLOAT";
	    case VK_FORMAT_R64G64B64A64_UINT :								return "VK_FORMAT_R64G64B64A64_UINT";
	    case VK_FORMAT_R64G64B64A64_SINT :								return "VK_FORMAT_R64G64B64A64_SINT";
	    case VK_FORMAT_R64G64B64A64_SFLOAT :							return "VK_FORMAT_R64G64B64A64_SFLOAT";
	    case VK_FORMAT_B10G11R11_UFLOAT_PACK32 :						return "VK_FORMAT_B10G11R11_UFLOAT_PACK32";
	    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32 :							return "VK_FORMAT_E5B9G9R9_UFLOAT_PACK32";
	    case VK_FORMAT_D16_UNORM :										return "VK_FORMAT_D16_UNORM";
	    case VK_FORMAT_X8_D24_UNORM_PACK32 :							return "VK_FORMAT_X8_D24_UNORM_PACK32";
	    case VK_FORMAT_D32_SFLOAT :										return "VK_FORMAT_D32_SFLOAT";
	    case VK_FORMAT_S8_UINT :										return "VK_FORMAT_S8_UINT";
	    case VK_FORMAT_D16_UNORM_S8_UINT :								return "VK_FORMAT_D16_UNORM_S8_UINT";
	    case VK_FORMAT_D24_UNORM_S8_UINT :								return "VK_FORMAT_D24_UNORM_S8_UINT";
	    case VK_FORMAT_D32_SFLOAT_S8_UINT :								return "VK_FORMAT_D32_SFLOAT_S8_UINT";
	    case VK_FORMAT_BC1_RGB_UNORM_BLOCK :							return "VK_FORMAT_BC1_RGB_UNORM_BLOCK";
	    case VK_FORMAT_BC1_RGB_SRGB_BLOCK :								return "VK_FORMAT_BC1_RGB_SRGB_BLOCK";
	    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK :							return "VK_FORMAT_BC1_RGBA_UNORM_BLOCK";
	    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK :							return "VK_FORMAT_BC1_RGBA_SRGB_BLOCK";
	    case VK_FORMAT_BC2_UNORM_BLOCK :								return "VK_FORMAT_BC2_UNORM_BLOCK";
	    case VK_FORMAT_BC2_SRGB_BLOCK :									return "VK_FORMAT_BC2_SRGB_BLOCK";
	    case VK_FORMAT_BC3_UNORM_BLOCK :								return "VK_FORMAT_BC3_UNORM_BLOCK";
	    case VK_FORMAT_BC3_SRGB_BLOCK :									return "VK_FORMAT_BC3_SRGB_BLOCK";
	    case VK_FORMAT_BC4_UNORM_BLOCK :								return "VK_FORMAT_BC4_UNORM_BLOCK";
	    case VK_FORMAT_BC4_SNORM_BLOCK :								return "VK_FORMAT_BC4_SNORM_BLOCK";
	    case VK_FORMAT_BC5_UNORM_BLOCK :								return "VK_FORMAT_BC5_UNORM_BLOCK";
	    case VK_FORMAT_BC5_SNORM_BLOCK :								return "VK_FORMAT_BC5_SNORM_BLOCK";
	    case VK_FORMAT_BC6H_UFLOAT_BLOCK :								return "VK_FORMAT_BC6H_UFLOAT_BLOCK";
	    case VK_FORMAT_BC6H_SFLOAT_BLOCK :								return "VK_FORMAT_BC6H_SFLOAT_BLOCK";
	    case VK_FORMAT_BC7_UNORM_BLOCK :								return "VK_FORMAT_BC7_UNORM_BLOCK";
	    case VK_FORMAT_BC7_SRGB_BLOCK :									return "VK_FORMAT_BC7_SRGB_BLOCK";
	    case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK :						return "VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK";
	    case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK :							return "VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK";
	    case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK :						return "VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK";
	    case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK :						return "VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK";
	    case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK :						return "VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK";
	    case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK :						return "VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK";
	    case VK_FORMAT_EAC_R11_UNORM_BLOCK :							return "VK_FORMAT_EAC_R11_UNORM_BLOCK";
	    case VK_FORMAT_EAC_R11_SNORM_BLOCK :							return "VK_FORMAT_EAC_R11_SNORM_BLOCK";
	    case VK_FORMAT_EAC_R11G11_UNORM_BLOCK :							return "VK_FORMAT_EAC_R11G11_UNORM_BLOCK";
	    case VK_FORMAT_EAC_R11G11_SNORM_BLOCK :							return "VK_FORMAT_EAC_R11G11_SNORM_BLOCK";
	    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK :							return "VK_FORMAT_ASTC_4x4_UNORM_BLOCK";
	    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK :							return "VK_FORMAT_ASTC_4x4_SRGB_BLOCK";
	    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK :							return "VK_FORMAT_ASTC_5x4_UNORM_BLOCK";
	    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK :							return "VK_FORMAT_ASTC_5x4_SRGB_BLOCK";
	    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK :							return "VK_FORMAT_ASTC_5x5_UNORM_BLOCK";
	    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK :							return "VK_FORMAT_ASTC_5x5_SRGB_BLOCK";
	    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK :							return "VK_FORMAT_ASTC_6x5_UNORM_BLOCK";
	    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK :							return "VK_FORMAT_ASTC_6x5_SRGB_BLOCK";
	    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK :							return "VK_FORMAT_ASTC_6x6_UNORM_BLOCK";
	    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK :							return "VK_FORMAT_ASTC_6x6_SRGB_BLOCK";
	    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK :							return "VK_FORMAT_ASTC_8x5_UNORM_BLOCK";
	    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK :							return "VK_FORMAT_ASTC_8x5_SRGB_BLOCK";
	    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK :							return "VK_FORMAT_ASTC_8x6_UNORM_BLOCK";
	    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK :							return "VK_FORMAT_ASTC_8x6_SRGB_BLOCK";
	    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK :							return "VK_FORMAT_ASTC_8x8_UNORM_BLOCK";
	    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK :							return "VK_FORMAT_ASTC_8x8_SRGB_BLOCK";
	    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK :							return "VK_FORMAT_ASTC_10x5_UNORM_BLOCK";
	    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK :							return "VK_FORMAT_ASTC_10x5_SRGB_BLOCK";
	    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK :							return "VK_FORMAT_ASTC_10x6_UNORM_BLOCK";
	    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK :							return "VK_FORMAT_ASTC_10x6_SRGB_BLOCK";
	    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK :							return "VK_FORMAT_ASTC_10x8_UNORM_BLOCK";
	    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK :							return "VK_FORMAT_ASTC_10x8_SRGB_BLOCK";
	    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK :							return "VK_FORMAT_ASTC_10x10_UNORM_BLOCK";
	    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK :							return "VK_FORMAT_ASTC_10x10_SRGB_BLOCK";
	    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK :							return "VK_FORMAT_ASTC_12x10_UNORM_BLOCK";
	    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK :							return "VK_FORMAT_ASTC_12x10_SRGB_BLOCK";
	    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK :							return "VK_FORMAT_ASTC_12x12_UNORM_BLOCK";
	    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK :							return "VK_FORMAT_ASTC_12x12_SRGB_BLOCK";
	    case VK_FORMAT_G8B8G8R8_422_UNORM :								return "VK_FORMAT_G8B8G8R8_422_UNORM";
	    case VK_FORMAT_B8G8R8G8_422_UNORM :								return "VK_FORMAT_B8G8R8G8_422_UNORM";
	    case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM :						return "VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM";
	    case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM :						return "VK_FORMAT_G8_B8R8_2PLANE_420_UNORM";
	    case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM :						return "VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM";
	    case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM :						return "VK_FORMAT_G8_B8R8_2PLANE_422_UNORM";
	    case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM :						return "VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM";
	    case VK_FORMAT_R10X6_UNORM_PACK16 :								return "VK_FORMAT_R10X6_UNORM_PACK16";
	    case VK_FORMAT_R10X6G10X6_UNORM_2PACK16 :						return "VK_FORMAT_R10X6G10X6_UNORM_2PACK16";
	    case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16 :				return "VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16";
	    case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16 :			return "VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16";
	    case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16 :			return "VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16";
	    case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16 :		return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16";
	    case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16 :		return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16";
	    case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16 :		return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16";
	    case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16 :		return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16";
	    case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16 :		return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16";
	    case VK_FORMAT_R12X4_UNORM_PACK16 :								return "VK_FORMAT_R12X4_UNORM_PACK16";
	    case VK_FORMAT_R12X4G12X4_UNORM_2PACK16 :						return "VK_FORMAT_R12X4G12X4_UNORM_2PACK16";
	    case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16 :				return "VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16";
	    case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16 :			return "VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16";
	    case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16 :			return "VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16";
	    case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16 :		return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16";
	    case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16 :		return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16";
	    case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16 :		return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16";
	    case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16 :		return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16";
	    case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16 :		return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16";
	    case VK_FORMAT_G16B16G16R16_422_UNORM :							return "VK_FORMAT_G16B16G16R16_422_UNORM";
	    case VK_FORMAT_B16G16R16G16_422_UNORM :							return "VK_FORMAT_B16G16R16G16_422_UNORM";
	    case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM :					return "VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM";
	    case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM :					return "VK_FORMAT_G16_B16R16_2PLANE_420_UNORM";
	    case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM :					return "VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM";
	    case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM :					return "VK_FORMAT_G16_B16R16_2PLANE_422_UNORM";
	    case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM :					return "VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM";
	    case VK_FORMAT_G8_B8R8_2PLANE_444_UNORM :						return "VK_FORMAT_G8_B8R8_2PLANE_444_UNORM";
	    case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16 :		return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16";
	    case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16 :		return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16";
	    case VK_FORMAT_G16_B16R16_2PLANE_444_UNORM :					return "VK_FORMAT_G16_B16R16_2PLANE_444_UNORM";
	    case VK_FORMAT_A4R4G4B4_UNORM_PACK16 :							return "VK_FORMAT_A4R4G4B4_UNORM_PACK16";
	    case VK_FORMAT_A4B4G4R4_UNORM_PACK16 :							return "VK_FORMAT_A4B4G4R4_UNORM_PACK16";
	    case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK :							return "VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK";
	    case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK :							return "VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK";
	    case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK :							return "VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK";
	    case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK :							return "VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK";
	    case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK :							return "VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK";
	    case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK :							return "VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK";
	    case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK :							return "VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK";
	    case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK :							return "VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK";
	    case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK :							return "VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK";
	    case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK :							return "VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK";
	    case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK :							return "VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK";
	    case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK :						return "VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK";
	    case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK :						return "VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK";
	    case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK :						return "VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK";
	    case VK_FORMAT_A1B5G5R5_UNORM_PACK16 :							return "VK_FORMAT_A1B5G5R5_UNORM_PACK16";		/// ONLY IN VULKAN 1.4+
	    case VK_FORMAT_A8_UNORM :										return "VK_FORMAT_A8_UNORM";					/// ONLY IN VULKAN 1.4+
	    case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG :					return "VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG";
	    case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG :					return "VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG";
	    case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG :					return "VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG";
	    case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG :					return "VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG";
	    case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG :						return "VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG";
	    case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG :						return "VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG";
	    case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG :						return "VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG";
	    case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG :						return "VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG";
	    case VK_FORMAT_R16G16_SFIXED5_NV :								return "VK_FORMAT_R16G16_SFIXED5_NV";
	    case VK_FORMAT_MAX_ENUM :										return "VK_FORMAT_MAX_ENUM";
	    default :														return "UNKNOWN";
	    }
	}
	
	/**
	 * @brief Get the string representation of a Vulkan result code.
	 *
	 * @param errorCode The Vulkan result code.
	 * @return The string representation of the result code.
	 */
	const char *vkErrorString(const VkResult errorCode)
	{
	    switch (errorCode)
	    {
	    case VK_SUCCESS :												return "VK_SUCCESS";
	    case VK_NOT_READY :												return "VK_NOT_READY";
	    case VK_TIMEOUT :												return "VK_TIMEOUT";
	    case VK_EVENT_SET :												return "VK_EVENT_SET";
	    case VK_EVENT_RESET :											return "VK_EVENT_RESET";
	    case VK_INCOMPLETE :											return "VK_INCOMPLETE";
	    case VK_ERROR_OUT_OF_HOST_MEMORY :								return "VK_ERROR_OUT_OF_HOST_MEMORY";
	    case VK_ERROR_OUT_OF_DEVICE_MEMORY :							return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
	    case VK_ERROR_INITIALIZATION_FAILED :							return "VK_ERROR_INITIALIZATION_FAILED";
	    case VK_ERROR_DEVICE_LOST :										return "VK_ERROR_DEVICE_LOST";
	    case VK_ERROR_MEMORY_MAP_FAILED :								return "VK_ERROR_MEMORY_MAP_FAILED";
	    case VK_ERROR_LAYER_NOT_PRESENT :								return "VK_ERROR_LAYER_NOT_PRESENT";
	    case VK_ERROR_EXTENSION_NOT_PRESENT :							return "VK_ERROR_EXTENSION_NOT_PRESENT";
	    case VK_ERROR_FEATURE_NOT_PRESENT :								return "VK_ERROR_FEATURE_NOT_PRESENT";
	    case VK_ERROR_INCOMPATIBLE_DRIVER :								return "VK_ERROR_INCOMPATIBLE_DRIVER";
	    case VK_ERROR_TOO_MANY_OBJECTS :								return "VK_ERROR_TOO_MANY_OBJECTS";
	    case VK_ERROR_FORMAT_NOT_SUPPORTED :							return "VK_ERROR_FORMAT_NOT_SUPPORTED";
	    case VK_ERROR_FRAGMENTED_POOL:									return "VK_ERROR_FRAGMENTED_POOL";
	    case VK_ERROR_UNKNOWN:											return "VK_ERROR_UNKNOWN";
	    case VK_ERROR_OUT_OF_POOL_MEMORY:								return "VK_ERROR_OUT_OF_POOL_MEMORY";
	    case VK_ERROR_INVALID_EXTERNAL_HANDLE:							return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
	    case VK_ERROR_FRAGMENTATION:									return "VK_ERROR_FRAGMENTATION";
	    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:                   return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
	    case VK_PIPELINE_COMPILE_REQUIRED:                              return "VK_PIPELINE_COMPILE_REQUIRED";
	    case VK_ERROR_NOT_PERMITTED:                                    return "VK_ERROR_NOT_PERMITTED";
	    case VK_ERROR_SURFACE_LOST_KHR:                                 return "VK_ERROR_SURFACE_LOST_KHR";
	    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:                         return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
	    case VK_SUBOPTIMAL_KHR:											return "VK_SUBOPTIMAL_KHR";
	    case VK_ERROR_OUT_OF_DATE_KHR:                                  return "VK_ERROR_OUT_OF_DATE_KHR";
	    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:                         return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
	    case VK_ERROR_VALIDATION_FAILED_EXT:                            return "VK_ERROR_VALIDATION_FAILED_EXT";
	    case VK_ERROR_INVALID_SHADER_NV:                                return "VK_ERROR_INVALID_SHADER_NV";
	    case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:                    return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
	    case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:           return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
	    case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:        return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
	    case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:			return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
	    case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:			return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
	    case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:				return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
	    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:     return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
	    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:              return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
	    case VK_THREAD_IDLE_KHR:                                        return "VK_THREAD_IDLE_KHR";
	    case VK_THREAD_DONE_KHR:                                        return "VK_THREAD_DONE_KHR";
	    case VK_OPERATION_DEFERRED_KHR:                                 return "VK_OPERATION_DEFERRED_KHR";
	    case VK_OPERATION_NOT_DEFERRED_KHR:                             return "VK_OPERATION_NOT_DEFERRED_KHR";
	    case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:                 return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
	    case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:                        return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
	    case VK_INCOMPATIBLE_SHADER_BINARY_EXT:                         return "VK_INCOMPATIBLE_SHADER_BINARY_EXT";
	    case VK_PIPELINE_BINARY_MISSING_KHR:                            return "VK_PIPELINE_BINARY_MISSING_KHR";
	    case VK_ERROR_NOT_ENOUGH_SPACE_KHR:                             return "VK_ERROR_NOT_ENOUGH_SPACE_KHR";
	    case VK_RESULT_MAX_ENUM:										return "VK_RESULT_MAX_ENUM";
	    default :														return "Unknown VkResult";
	    }
	}
	
	/**
	 * @brief Get the string representation of a Vulkan object type.
	 *
	 * @param objectType The Vulkan object type.
	 * @return The string representation of the object type.
	 */
	const char *VkObjectTypeToString(const VkObjectType objectType)
	{
	    switch (objectType)
	    {
	    case VK_OBJECT_TYPE_COMMAND_BUFFER:								return "VK_OBJECT_TYPE_COMMAND_BUFFER";
	    case VK_OBJECT_TYPE_PIPELINE:									return "VK_OBJECT_TYPE_PIPELINE";
	    case VK_OBJECT_TYPE_FRAMEBUFFER:								return "VK_OBJECT_TYPE_FRAMEBUFFER";
	    case VK_OBJECT_TYPE_IMAGE:										return "VK_OBJECT_TYPE_IMAGE";
	    case VK_OBJECT_TYPE_QUERY_POOL:									return "VK_OBJECT_TYPE_QUERY_POOL";
	    case VK_OBJECT_TYPE_RENDER_PASS:								return "VK_OBJECT_TYPE_RENDER_PASS";
	    case VK_OBJECT_TYPE_COMMAND_POOL:								return "VK_OBJECT_TYPE_COMMAND_POOL";
	    case VK_OBJECT_TYPE_PIPELINE_CACHE:								return "VK_OBJECT_TYPE_PIPELINE_CACHE";
	    case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR:					return "VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR";
	    case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV:					return "VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV";
	    case VK_OBJECT_TYPE_BUFFER:										return "VK_OBJECT_TYPE_BUFFER";
	    case VK_OBJECT_TYPE_BUFFER_VIEW:								return "VK_OBJECT_TYPE_BUFFER_VIEW";
	    case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT:					return "VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT";
	    case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT:					return "VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT";
	    case VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR:						return "VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR";
	    case VK_OBJECT_TYPE_DESCRIPTOR_POOL:							return "VK_OBJECT_TYPE_DESCRIPTOR_POOL";
	    case VK_OBJECT_TYPE_DESCRIPTOR_SET:								return "VK_OBJECT_TYPE_DESCRIPTOR_SET";
	    case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:						return "VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT";
	    case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE:					return "VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE";
	    case VK_OBJECT_TYPE_PRIVATE_DATA_SLOT_EXT:						return "VK_OBJECT_TYPE_PRIVATE_DATA_SLOT_EXT";
	    case VK_OBJECT_TYPE_DEVICE:										return "VK_OBJECT_TYPE_DEVICE";
	    case VK_OBJECT_TYPE_DEVICE_MEMORY:								return "VK_OBJECT_TYPE_DEVICE_MEMORY";
	    case VK_OBJECT_TYPE_PIPELINE_LAYOUT:							return "VK_OBJECT_TYPE_PIPELINE_LAYOUT";
	    case VK_OBJECT_TYPE_DISPLAY_KHR:								return "VK_OBJECT_TYPE_DISPLAY_KHR";
	    case VK_OBJECT_TYPE_DISPLAY_MODE_KHR:							return "VK_OBJECT_TYPE_DISPLAY_MODE_KHR";
	    case VK_OBJECT_TYPE_PHYSICAL_DEVICE:							return "VK_OBJECT_TYPE_PHYSICAL_DEVICE";
	    case VK_OBJECT_TYPE_EVENT:										return "VK_OBJECT_TYPE_EVENT";
	    case VK_OBJECT_TYPE_FENCE:										return "VK_OBJECT_TYPE_FENCE";
	    case VK_OBJECT_TYPE_IMAGE_VIEW:									return "VK_OBJECT_TYPE_IMAGE_VIEW";
	    case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV:				return "VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV";
	    case VK_OBJECT_TYPE_INSTANCE:									return "VK_OBJECT_TYPE_INSTANCE";
	    case VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL:			return "VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL";
	    case VK_OBJECT_TYPE_QUEUE:										return "VK_OBJECT_TYPE_QUEUE";
	    case VK_OBJECT_TYPE_SAMPLER:									return "VK_OBJECT_TYPE_SAMPLER";
	    case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION:					return "VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION";
	    case VK_OBJECT_TYPE_SEMAPHORE:									return "VK_OBJECT_TYPE_SEMAPHORE";
	    case VK_OBJECT_TYPE_SHADER_MODULE:								return "VK_OBJECT_TYPE_SHADER_MODULE";
	    case VK_OBJECT_TYPE_SURFACE_KHR:								return "VK_OBJECT_TYPE_SURFACE_KHR";
	    case VK_OBJECT_TYPE_SWAPCHAIN_KHR:								return "VK_OBJECT_TYPE_SWAPCHAIN_KHR";
	    case VK_OBJECT_TYPE_VALIDATION_CACHE_EXT:						return "VK_OBJECT_TYPE_VALIDATION_CACHE_EXT";
	    case VK_OBJECT_TYPE_UNKNOWN:									return "VK_OBJECT_TYPE_UNKNOWN";
	    case VK_OBJECT_TYPE_MAX_ENUM:									return "VK_OBJECT_TYPE_MAX_ENUM";
	    default:
	        break;
	    }
	
	    SEDX_CORE_ASSERT(false);
	    return "";
	}

    /**
     * @brief Converts a Vulkan vendor ID to a human-readable string.
     * @param vendorID The vendor ID of the graphics device (e.g., 0x10DE for NVIDIA, 0x1002 for AMD).
     * @return A string representation of the vendor name.
     */
    GLOBAL const char* VendorIDToString(const uint32_t vendorID)
	{
	    // ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
	    switch (vendorID)
	    {
	    case 0x10DE: return "NVIDIA";
	    case 0x1002: return "AMD";
	    case 0x8086: return "INTEL";
	    case 0x13B5: return "ARM";
	    }
	    return "Unknown";
	}

    /**
     * @brief Sets the graphics device vendor name for the current rendering context.
     * @param vendorID The vendor ID of the graphics device (e.g., 0x10DE for NVIDIA, 0x1002 for AMD).
     */
    void SetDeviceVendorName(const uint32_t vendorID)
	{
	    VendorIDToString(vendorID);
	}
	
	/**
	 * @brief Sets the graphics device name for the current rendering context.
	 *
	 * This function validates and stores the graphics device name in the global render data structure.
	 * It performs input validation to ensure the device name is not empty and logs the operation
	 * for debugging purposes. The device name is typically obtained from Vulkan physical device
	 * properties and used for display in UI, logging, and debugging information.
	 *
	 * The function performs the following operations:
	 * 1. Validates the input device name is not empty
	 * 2. Creates or updates the global RenderData structure
	 * 3. Stores the device name in the render data
	 * 4. Logs the operation with appropriate tags for debugging
	 *
	 * @param deviceName The name of the graphics device (e.g., "NVIDIA GeForce RTX 4080", "AMD Radeon RX 7900 XTX")
	 *                   Must not be empty or the function will log an error and return early
	 *
	 * @note - This function is typically called during Vulkan device initialization to store
	 *       the selected physical device name for later reference in the application.
	 *
	 * @warning If the device name is empty, the function will log an error using SEDX_CORE_ERROR_TAG
	 *          and return without setting any device information.
	 *
	 * @see VendorIDToString() for converting vendor IDs to readable vendor names
	 * @see SetDeviceVendorName() for setting vendor-specific information
	 *
	 * @example
	 * @code
	 * // During Vulkan device enumeration
	 * VkPhysicalDeviceProperties deviceProperties;
	 * vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	 * GetDeviceName(std::string(deviceProperties.deviceName));
	 * @endcode 
	 */
	void GetDeviceName(const std::string &deviceName)
	{
		///< Validate input parameters
		if (deviceName.empty())
	        return;
	
	    SEDX_CORE_INFO_TAG("VULKAN", "Setting graphics device name: '{}'", deviceName);
	
		///< Create or update the global render data structure
		SceneryEditorX::RenderData deviceData;
		deviceData.Device = deviceName;
	
		//SEDX_CORE_DEBUG_TAG("VULKAN", "Device name successfully stored in render data structure");
		
		///< TODO: Consider storing this in a global render context or device manager
		///< for persistent access throughout the application lifecycle
	}
	
	VkPrimitiveTopology GetVKTopology(PrimitiveTopology topology)
	{
	    switch (topology)
	    {
			case PrimitiveTopology::Points:			return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
			case PrimitiveTopology::Lines:			return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
			case PrimitiveTopology::Triangles:		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			case PrimitiveTopology::LineStrip:		return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
			case PrimitiveTopology::TriangleStrip:	return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			case PrimitiveTopology::TriangleFan:	return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
	        case PrimitiveTopology::None:
	            break;
	    }
	
	    SEDX_CORE_ASSERT(false, "Unknown toplogy");
	    return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
	}
	
	VkCompareOp GetVulkanCompareOperator(const DepthCompareOperator compareOp)
	{
	    switch (compareOp)
	    {
			case DepthCompareOperator::Never:			return VK_COMPARE_OP_NEVER;
			case DepthCompareOperator::NotEqual:		return VK_COMPARE_OP_NOT_EQUAL;
			case DepthCompareOperator::Less:			return VK_COMPARE_OP_LESS;
			case DepthCompareOperator::LessOrEqual:		return VK_COMPARE_OP_LESS_OR_EQUAL;
			case DepthCompareOperator::Greater:			return VK_COMPARE_OP_GREATER;
			case DepthCompareOperator::GreaterOrEqual:	return VK_COMPARE_OP_GREATER_OR_EQUAL;
			case DepthCompareOperator::Equal:			return VK_COMPARE_OP_EQUAL;
			case DepthCompareOperator::Always:			return VK_COMPARE_OP_ALWAYS;
			case DepthCompareOperator::None:
	            break;
	    }
	    SEDX_CORE_ASSERT(false, "Unknown Operator");
	    return VK_COMPARE_OP_MAX_ENUM;
	}


	uint32_t ShaderDataTypeSize(ShaderDataType type)
	{
		switch (type)
		{
			case ShaderDataType::Float:    return 4;
			case ShaderDataType::Float2:   return 4 * 2;
			case ShaderDataType::Float3:   return 4 * 3;
			case ShaderDataType::Float4:   return 4 * 4;
			case ShaderDataType::Mat3:     return 4 * 3 * 3;
			case ShaderDataType::Mat4:     return 4 * 4 * 4;
			case ShaderDataType::Int:      return 4;
			case ShaderDataType::Int2:     return 4 * 2;
			case ShaderDataType::Int3:     return 4 * 3;
			case ShaderDataType::Int4:     return 4 * 4;
			case ShaderDataType::Bool:     return 1;
		}

		SEDX_CORE_ASSERT(false, "Unknown ShaderDataType!");
		return 0;
	}

	VkFormat ShaderDataTypeToVulkanFormat(ShaderDataType type)
	{
		switch (type)
		{
			case ShaderDataType::Float:     return VK_FORMAT_R32_SFLOAT;
			case ShaderDataType::Float2:    return VK_FORMAT_R32G32_SFLOAT;
			case ShaderDataType::Float3:    return VK_FORMAT_R32G32B32_SFLOAT;
			case ShaderDataType::Float4:    return VK_FORMAT_R32G32B32A32_SFLOAT;
			case ShaderDataType::Int:       return VK_FORMAT_R32_SINT;
			case ShaderDataType::Int2:      return VK_FORMAT_R32G32_SINT;
			case ShaderDataType::Int3:      return VK_FORMAT_R32G32B32_SINT;
			case ShaderDataType::Int4:      return VK_FORMAT_R32G32B32A32_SINT;
		}
		SEDX_CORE_ASSERT(false);
		return VK_FORMAT_UNDEFINED;
	}

	/// -------------------------------------------------------
	
    LOCAL ResourceAllocationCounts s_ResourceAllocationCounts;
	
    ResourceAllocationCounts &GetResourceAllocationCounts()
    {
        return s_ResourceAllocationCounts;
    }
	
}

/// -------------------------------------------------------

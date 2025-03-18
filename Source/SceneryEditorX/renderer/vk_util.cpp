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


#include <SceneryEditorX/renderer/vk_core.h>
#include <vulkan/vulkan_core.h>
#include <vk_mem_alloc.h>
#include <cstdlib>

// -------------------------------------------------------

// Define operator<< for VkPhysicalDeviceType
std::ostream& operator<<(std::ostream& os,VkPhysicalDeviceType type)
{
	switch (type)
	{
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		os << "Integrated GPU";
		break;
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		os << "Discrete GPU";
		break;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
		os << "Virtual GPU";
		break;
		case VK_PHYSICAL_DEVICE_TYPE_CPU:
		os << "CPU";
		break;
		default:
		os << "Unknown";
		break;
	}
	return os;
}

// Define operator<< for VkQueueFlags
std::ostream& operator<<(std::ostream& os,VkQueueFlags flags)
{
	if (flags & VK_QUEUE_GRAPHICS_BIT)
		os << "Graphics ";
	if (flags & VK_QUEUE_COMPUTE_BIT)
		os << "Compute ";
	if (flags & VK_QUEUE_TRANSFER_BIT)
		os << "Transfer ";
	if (flags & VK_QUEUE_SPARSE_BINDING_BIT)
		os << "Sparse Binding ";
	if (flags & VK_QUEUE_PROTECTED_BIT)
		os << "Protected ";
	return os;
}

// Define operator<< for VkFormat
std::ostream& operator<<(std::ostream& os,VkFormat format)
{
	switch (format)
	{
		case VK_FORMAT_R8G8B8A8_UNORM:
		os << "R8G8B8A8_UNORM";
		break;
	// Add cases for other formats as needed
		default:
		os << "Unknown";
		break;
	}
	return os;
}

// Define operator<< for VkColorSpaceKHR
std::ostream& operator<<(std::ostream& os,VkColorSpaceKHR colorSpace)
{
	switch (colorSpace)
	{
		case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
		os << "SRGB_NONLINEAR";
		break;
	// Add cases for other color spaces as needed
		default:
		os << "Unknown";
		break;
	}
	return os;
}

// Define operator<< for VkPresentModeKHR
const char* getDebugSeverityStr(VkDebugUtilsMessageSeverityFlagBitsEXT severity)
{
	switch (severity)
	{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		return "VERBOSE";
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		return "INFO";
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		return "WARNING";
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		return "ERROR";
		default:
		exit(1);
	}
	return "No Known Severity";
}

// Define operator<< for VkDebugUtilsMessageTypeFlagsEXT
const char* getDebugType(VkDebugUtilsMessageTypeFlagsEXT type)
{
	switch (type)
	{
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
		return "GENERAL";
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
		return "VALIDATION";
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
		return "PERFORMANCE";
	#ifdef _WIN64
		case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
		return "DEVICE ADDRESS BINDING";
	#endif
		default:
		exit(1);
	}
	return "No Known Type";
}


/**
* @brief Get the number of bytes per pixel for a given format.
 *
 * This function returns the number of bytes per pixel for a given Vulkan format.
 *
 * @param format The Vulkan format.
 * @return uint32_t The number of bytes per pixel.
 */
int32_t getBPP(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_R4G4_UNORM_PACK8:
        return 8;
        case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
        case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
        case VK_FORMAT_R5G6B5_UNORM_PACK16:
        case VK_FORMAT_B5G6R5_UNORM_PACK16:
        case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
        case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
        case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
        return 16;
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_SNORM:
        case VK_FORMAT_R8_USCALED:
        case VK_FORMAT_R8_SSCALED:
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8_SINT:
        case VK_FORMAT_R8_SRGB:
        return 8;
        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SNORM:
        case VK_FORMAT_R8G8_USCALED:
        case VK_FORMAT_R8G8_SSCALED:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8_SINT:
        case VK_FORMAT_R8G8_SRGB:
        return 16;
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SNORM:
        case VK_FORMAT_R8G8B8_USCALED:
        case VK_FORMAT_R8G8B8_SSCALED:
        case VK_FORMAT_R8G8B8_UINT:
        case VK_FORMAT_R8G8B8_SINT:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_B8G8R8_UNORM:
        case VK_FORMAT_B8G8R8_SNORM:
        case VK_FORMAT_B8G8R8_USCALED:
        case VK_FORMAT_B8G8R8_SSCALED:
        case VK_FORMAT_B8G8R8_UINT:
        case VK_FORMAT_B8G8R8_SINT:
        case VK_FORMAT_B8G8R8_SRGB:
        return 24;
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_USCALED:
        case VK_FORMAT_R8G8B8A8_SSCALED:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SNORM:
        case VK_FORMAT_B8G8R8A8_USCALED:
        case VK_FORMAT_B8G8R8A8_SSCALED:
        case VK_FORMAT_B8G8R8A8_UINT:
        case VK_FORMAT_B8G8R8A8_SINT:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
        case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
        case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
        case VK_FORMAT_A8B8G8R8_UINT_PACK32:
        case VK_FORMAT_A8B8G8R8_SINT_PACK32:
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
        return 32;
        case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
        case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
        case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
        case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
        case VK_FORMAT_A2R10G10B10_UINT_PACK32:
        case VK_FORMAT_A2R10G10B10_SINT_PACK32:
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
        case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
        case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
        case VK_FORMAT_A2B10G10R10_UINT_PACK32:
        case VK_FORMAT_A2B10G10R10_SINT_PACK32:
        return 32;
        case VK_FORMAT_R16_UNORM:
        case VK_FORMAT_R16_SNORM:
        case VK_FORMAT_R16_USCALED:
        case VK_FORMAT_R16_SSCALED:
        case VK_FORMAT_R16_UINT:
        case VK_FORMAT_R16_SINT:
        case VK_FORMAT_R16_SFLOAT:
        return 16;
        case VK_FORMAT_R16G16_UNORM:
        case VK_FORMAT_R16G16_SNORM:
        case VK_FORMAT_R16G16_USCALED:
        case VK_FORMAT_R16G16_SSCALED:
        case VK_FORMAT_R16G16_UINT:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_R16G16_SFLOAT:
        return 32;
        case VK_FORMAT_R16G16B16_UNORM:
        case VK_FORMAT_R16G16B16_SNORM:
        case VK_FORMAT_R16G16B16_USCALED:
        case VK_FORMAT_R16G16B16_SSCALED:
        case VK_FORMAT_R16G16B16_UINT:
        case VK_FORMAT_R16G16B16_SINT:
        case VK_FORMAT_R16G16B16_SFLOAT:
        return 48;
        case VK_FORMAT_R16G16B16A16_UNORM:
        case VK_FORMAT_R16G16B16A16_SNORM:
        case VK_FORMAT_R16G16B16A16_USCALED:
        case VK_FORMAT_R16G16B16A16_SSCALED:
        case VK_FORMAT_R16G16B16A16_UINT:
        case VK_FORMAT_R16G16B16A16_SINT:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
        return 64;
        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32_SFLOAT:
        return 32;
        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32_SFLOAT:
        return 64;
        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32_SFLOAT:
        return 96;
        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
        return 128;
        case VK_FORMAT_R64_UINT:
        case VK_FORMAT_R64_SINT:
        case VK_FORMAT_R64_SFLOAT:
        return 64;
        case VK_FORMAT_R64G64_UINT:
        case VK_FORMAT_R64G64_SINT:
        case VK_FORMAT_R64G64_SFLOAT:
        return 128;
        case VK_FORMAT_R64G64B64_UINT:
        case VK_FORMAT_R64G64B64_SINT:
        case VK_FORMAT_R64G64B64_SFLOAT:
        return 192;
        case VK_FORMAT_R64G64B64A64_UINT:
        case VK_FORMAT_R64G64B64A64_SINT:
        case VK_FORMAT_R64G64B64A64_SFLOAT:
        return 256;
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        return 32;
        case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
        return 32;
        case VK_FORMAT_D16_UNORM:
        return 16;
        case VK_FORMAT_X8_D24_UNORM_PACK32:
        return 32;
        case VK_FORMAT_D32_SFLOAT:
        return 32;
        case VK_FORMAT_S8_UINT:
        return 8;
        case VK_FORMAT_D16_UNORM_S8_UINT:
        return 24;
        case VK_FORMAT_D24_UNORM_S8_UINT:
        return 32;
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return 40;
        case VK_FORMAT_UNDEFINED:
        default:
        return -1;
    }
}

/**
* @brief Get the number of bytes per pixel for a given format.
 *
 * This function returns the number of bytes per pixel for a given Vulkan format.
 *
 * @param format The Vulkan format.
 * @return uint32_t The number of bytes per pixel.
 */
VkAccessFlags getAccessFlags(VkImageLayout layout)
{
    switch (layout)
    {
        case VK_IMAGE_LAYOUT_UNDEFINED:
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        return 0;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
        return VK_ACCESS_HOST_WRITE_BIT;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
        return VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return VK_ACCESS_TRANSFER_READ_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return VK_ACCESS_TRANSFER_WRITE_BIT;
        case VK_IMAGE_LAYOUT_GENERAL:
        assert(false && "Don't know how to get a meaningful VkAccessFlags for VK_IMAGE_LAYOUT_GENERAL! Don't use it!");
        return 0;
        default:
        assert(false);
        return 0;
    }
}

/**
* @brief Get the number of bytes per pixel for a given format.
 *
 * This function returns the number of bytes per pixel for a given Vulkan format.
 *
 * @param format The Vulkan format.
 * @return uint32_t The number of bytes per pixel.
 */
VkPipelineStageFlags getPipelineStageFlags(VkImageLayout layout)
{
    switch (layout)
    {
        case VK_IMAGE_LAYOUT_UNDEFINED:
        return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
        return VK_PIPELINE_STAGE_HOST_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
        return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
        return VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        case VK_IMAGE_LAYOUT_GENERAL:
        assert(false &&
               "Don't know how to get a meaningful VkPipelineStageFlags for VK_IMAGE_LAYOUT_GENERAL! Don't use it!");
        return 0;
        default:
        assert(false);
        return 0;
    }
}


// taken from Sam Lantiga: https://www.libsdl.org/tmp/SDL/test/testvulkan.c
static const char* VK_ERROR_STRING(VkResult result)
{
    switch ((int)result)
    {
        case VK_SUCCESS:
        return "VK_SUCCESS";
        case VK_NOT_READY:
        return "VK_NOT_READY";
        case VK_TIMEOUT:
        return "VK_TIMEOUT";
        case VK_EVENT_SET:
        return "VK_EVENT_SET";
        case VK_EVENT_RESET:
        return "VK_EVENT_RESET";
        case VK_INCOMPLETE:
        return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
        return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:
        return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:
        return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED:
        return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:
        return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
        return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
        return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
        return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:
        return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
        return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL:
        return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_SURFACE_LOST_KHR:
        return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR:
        return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:
        return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT:
        return "VK_ERROR_VALIDATION_FAILED_EXT";
        case VK_ERROR_OUT_OF_POOL_MEMORY_KHR:
        return "VK_ERROR_OUT_OF_POOL_MEMORY_KHR";
        case VK_ERROR_INVALID_SHADER_NV:
        return "VK_ERROR_INVALID_SHADER_NV";
        default:
        break;
    }
    if (result < 0)
        return "VK_ERROR_<Unknown>";
    return "VK_<Unknown>";
}

/**
 * @brief Get the number of bytes per pixel for a given format.
 *
 * This function returns the number of bytes per pixel for a given Vulkan format.
 *
 * @param format The Vulkan format.
 * @return uint32_t The number of bytes per pixel.
 */

/*
VkSurfaceFormatKHR setSurfaceFormat(VkPhysicalDevice gpu,
                                    VkSurfaceKHR surface,
                                    std::vector<VkFormat> const& preferredFormats)
{
    uint32_t surfaceFormatNum;
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu,surface,&surfaceFormatNum,nullptr);
    assert(0 < surfaceFormatNum);
    std::vector<VkSurfaceFormatKHR> supportedSurfaceFormats(surfaceFormatNum);
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu,surface,&surfaceFormatNum,supportedSurfaceFormats.data());

    const auto it =
        std::ranges::find_if(supportedSurfaceFormats,[&preferredFormats](VkSurfaceFormatKHR surfaceFormat)
    {
        return std::ranges::any_of(preferredFormats,
                                   [&surfaceFormat](VkFormat format)
        {
            return format == surfaceFormat.format;
        });
    });

// We use the first supported format as a fallback in case none of the preferred formats is available
    return it != supportedSurfaceFormats.end() ? *it : supportedSurfaceFormats[0];
}
*/
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


#include <cstdlib>
#include <SceneryEditorX/renderer/vk_core.h>
#include <SceneryEditorX/renderer/vk_util.h>
#include <vk_mem_alloc.h>

// -------------------------------------------------------

const char* VK_DEVICE_TYPE_STRING(VkPhysicalDeviceType type)
{
	switch (type)
	{
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:                            return "Integrated GPU";
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:                              return "Discrete GPU";
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:                               return "Virtual GPU";
		case VK_PHYSICAL_DEVICE_TYPE_CPU:                                       return "CPU";
		default:                                                                return "Unknown";
	}
}

const char *VK_COLOR_SPACE_STRING(VkColorSpaceKHR colorSpace)
{
	switch (colorSpace)
	{
		case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:                                 return "SRGB_NONLINEAR";
		case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT:                           return "DISPLAY_P3_NONLINEAR";
		case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:                           return "EXTENDED_SRGB_LINEAR";
		case VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT:                              return "DISPLAY_P3_LINEAR";
		case VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT:                               return "DCI_P3_NONLINEAR";
		case VK_COLOR_SPACE_BT709_LINEAR_EXT:                                   return "BT709_LINEAR";
		case VK_COLOR_SPACE_BT709_NONLINEAR_EXT:                                return "BT709_NONLINEAR";
		case VK_COLOR_SPACE_BT2020_LINEAR_EXT:                                  return "BT2020_LINEAR";
		case VK_COLOR_SPACE_HDR10_ST2084_EXT:                                   return "HDR10_ST2084";
		case VK_COLOR_SPACE_DOLBYVISION_EXT:                                    return "DOLBYVISION";
		case VK_COLOR_SPACE_HDR10_HLG_EXT:                                      return "HDR10_HLG";
		case VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT:                                return "ADOBERGB_LINEAR";
		case VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT:                             return "ADOBERGB_NONLINEAR";
		case VK_COLOR_SPACE_PASS_THROUGH_EXT:                                   return "PASS_THROUGH";
		case VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT:                        return "EXTENDED_SRGB_NONLINEAR";
		case VK_COLOR_SPACE_DISPLAY_NATIVE_AMD:                                 return "DISPLAY_NATIVE_AMD";
		case VK_COLOR_SPACE_MAX_ENUM_KHR:                                       return "MAX_ENUM_KHR";
        default:
            return "Unknown";
	}
}

const char* VK_QUEUE_FLAGS_STRING(VkQueueFlags flags)
{
	switch (flags)
	{
		case VK_QUEUE_GRAPHICS_BIT:                                         return "Graphics";
		case VK_QUEUE_COMPUTE_BIT:                                          return "Compute";
		case VK_QUEUE_TRANSFER_BIT:                                         return "Transfer";
		case VK_QUEUE_SPARSE_BINDING_BIT:                                   return "Sparse Binding";
		case VK_QUEUE_PROTECTED_BIT:                                        return "Protected";
        default:
            return "Unknown";
	}
}

const char* VK_MEMORY_PROPERTY_FLAGS_STRING(VkMemoryPropertyFlags flags)
{
	switch (flags)
	{
		case VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT:                           return "Device Local";
		case VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT:                           return "Host Visible";
		case VK_MEMORY_PROPERTY_HOST_COHERENT_BIT:                          return "Host Coherent";
		case VK_MEMORY_PROPERTY_HOST_CACHED_BIT:                            return "Host Cached";
		case VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT:                       return "Lazily Allocated";
		case VK_MEMORY_PROPERTY_PROTECTED_BIT:                              return "Protected";
		case VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD:                    return "Device Coherent";
		case VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD:                    return "Device Uncached";
		case VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV:                        return "RDMA Capable";
		case VK_MEMORY_PROPERTY_FLAG_BITS_MAX_ENUM:                         return "Flag Bits Max Enum";
        default:
            return "Unknown";
	}
}

const char* VK_DEBUG_SEVERITY_STRING(VkDebugUtilsMessageSeverityFlagBitsEXT severity)
{
	switch (severity)
	{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:               return "VERBOSE";
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:                  return "INFO";
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:               return "WARNING";
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:                 return "ERROR";
		default:
			exit(1);
	}

	return "No Known Severity";
}

const char* VK_DEBUG_TYPE(VkDebugUtilsMessageTypeFlagsEXT type)
{
	switch (type)
	{
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:                   return "GENERAL";
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:                return "VALIDATION";
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:               return "PERFORMANCE";
	#ifdef _WIN64
		case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:    return "DEVICE ADDRESS BINDING";
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
VkAccessFlags VK_ACCESS_FLAGS(VkImageLayout layout)
{
	switch (layout)
	{
		case VK_IMAGE_LAYOUT_UNDEFINED:
		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:                               return 0;
		case VK_IMAGE_LAYOUT_PREINITIALIZED:                                return VK_ACCESS_HOST_WRITE_BIT;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:                      return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:                      return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:  return VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:                      return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:                          return VK_ACCESS_TRANSFER_READ_BIT;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:                          return VK_ACCESS_TRANSFER_WRITE_BIT;
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
VkPipelineStageFlags VK_PIPELINE_STAGE_FLAGS(VkImageLayout layout)
{
	switch (layout)
	{
		case VK_IMAGE_LAYOUT_UNDEFINED:                                     return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		case VK_IMAGE_LAYOUT_PREINITIALIZED:                                return VK_PIPELINE_STAGE_HOST_BIT;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:                          return VK_PIPELINE_STAGE_TRANSFER_BIT;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:                      return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:                      return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:  return VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:                      return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:                               return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		case VK_IMAGE_LAYOUT_GENERAL:
			assert(false && "Don't know how to get a meaningful VkPipelineStageFlags for VK_IMAGE_LAYOUT_GENERAL! Don't use it!");
			return 0;
		default:
			assert(false);
			return 0;
	}
}

const char* VK_FORMAT_STRING(VkFormat format)
{
	switch (format)
	{
	case VK_FORMAT_UNDEFINED:                                              return "VK_FORMAT_UNDEFINED";
	case VK_FORMAT_R4G4_UNORM_PACK8:                                       return "VK_FORMAT_R4G4_UNORM_PACK8";
	case VK_FORMAT_R4G4B4A4_UNORM_PACK16:                                  return "VK_FORMAT_R4G4B4A4_UNORM_PACK16";
	case VK_FORMAT_B4G4R4A4_UNORM_PACK16:                                  return "VK_FORMAT_B4G4R4A4_UNORM_PACK16";
	case VK_FORMAT_R5G6B5_UNORM_PACK16:                                    return "VK_FORMAT_R5G6B5_UNORM_PACK16";
	case VK_FORMAT_B5G6R5_UNORM_PACK16:                                    return "VK_FORMAT_B5G6R5_UNORM_PACK16";
	case VK_FORMAT_R5G5B5A1_UNORM_PACK16:                                  return "VK_FORMAT_R5G5B5A1_UNORM_PACK16";
	case VK_FORMAT_B5G5R5A1_UNORM_PACK16:                                  return "VK_FORMAT_B5G5R5A1_UNORM_PACK16";
	case VK_FORMAT_A1R5G5B5_UNORM_PACK16:                                  return "VK_FORMAT_A1R5G5B5_UNORM_PACK16";
	case VK_FORMAT_R8_UNORM:                                               return "VK_FORMAT_R8_UNORM";
	case VK_FORMAT_R8_SNORM:                                               return "VK_FORMAT_R8_SNORM";
	case VK_FORMAT_R8_USCALED:                                             return "VK_FORMAT_R8_USCALED";
	case VK_FORMAT_R8_SSCALED:                                             return "VK_FORMAT_R8_SSCALED";
	case VK_FORMAT_R8_UINT:                                                return "VK_FORMAT_R8_UINT";
	case VK_FORMAT_R8_SINT:                                                return "VK_FORMAT_R8_SINT";
	case VK_FORMAT_R8_SRGB:                                                return "VK_FORMAT_R8_SRGB";
	case VK_FORMAT_R8G8_UNORM:                                             return "VK_FORMAT_R8G8_UNORM";
	case VK_FORMAT_R8G8_SNORM:                                             return "VK_FORMAT_R8G8_SNORM";
	case VK_FORMAT_R8G8_USCALED:                                           return "VK_FORMAT_R8G8_USCALED";
	case VK_FORMAT_R8G8_SSCALED:                                           return "VK_FORMAT_R8G8_SSCALED";
	case VK_FORMAT_R8G8_UINT:                                              return "VK_FORMAT_R8G8_UINT";
	case VK_FORMAT_R8G8_SINT:                                              return "VK_FORMAT_R8G8_SINT";
	case VK_FORMAT_R8G8_SRGB:                                              return "VK_FORMAT_R8G8_SRGB";
	case VK_FORMAT_R8G8B8_UNORM:                                           return "VK_FORMAT_R8G8B8_UNORM";
	case VK_FORMAT_R8G8B8_SNORM:                                           return "VK_FORMAT_R8G8B8_SNORM";
	case VK_FORMAT_R8G8B8_USCALED:                                         return "VK_FORMAT_R8G8B8_USCALED";
	case VK_FORMAT_R8G8B8_SSCALED:                                         return "VK_FORMAT_R8G8B8_SSCALED";
	case VK_FORMAT_R8G8B8_UINT:                                            return "VK_FORMAT_R8G8B8_UINT";
	case VK_FORMAT_R8G8B8_SINT:                                            return "VK_FORMAT_R8G8B8_SINT";
	case VK_FORMAT_R8G8B8_SRGB:                                            return "VK_FORMAT_R8G8B8_SRGB";
	case VK_FORMAT_B8G8R8_UNORM:                                           return "VK_FORMAT_B8G8R8_UNORM";
	case VK_FORMAT_B8G8R8_SNORM:                                           return "VK_FORMAT_B8G8R8_SNORM";
	case VK_FORMAT_B8G8R8_USCALED:                                         return "VK_FORMAT_B8G8R8_USCALED";
	case VK_FORMAT_B8G8R8_SSCALED:                                         return "VK_FORMAT_B8G8R8_SSCALED";
	case VK_FORMAT_B8G8R8_UINT:                                            return "VK_FORMAT_B8G8R8_UINT";
	case VK_FORMAT_B8G8R8_SINT:                                            return "VK_FORMAT_B8G8R8_SINT";
	case VK_FORMAT_B8G8R8_SRGB:                                            return "VK_FORMAT_B8G8R8_SRGB";
	case VK_FORMAT_R8G8B8A8_UNORM:                                         return "VK_FORMAT_R8G8B8A8_UNORM";
	case VK_FORMAT_R8G8B8A8_SNORM:                                         return "VK_FORMAT_R8G8B8A8_SNORM";
	case VK_FORMAT_R8G8B8A8_USCALED:                                       return "VK_FORMAT_R8G8B8A8_USCALED";
	case VK_FORMAT_R8G8B8A8_SSCALED:                                       return "VK_FORMAT_R8G8B8A8_SSCALED";
	case VK_FORMAT_R8G8B8A8_UINT:                                          return "VK_FORMAT_R8G8B8A8_UINT";
	case VK_FORMAT_R8G8B8A8_SINT:                                          return "VK_FORMAT_R8G8B8A8_SINT";
	case VK_FORMAT_R8G8B8A8_SRGB:                                          return "VK_FORMAT_R8G8B8A8_SRGB";
	case VK_FORMAT_B8G8R8A8_UNORM:                                         return "VK_FORMAT_B8G8R8A8_UNORM";
	case VK_FORMAT_B8G8R8A8_SNORM:                                         return "VK_FORMAT_B8G8R8A8_SNORM";
	case VK_FORMAT_B8G8R8A8_USCALED:                                       return "VK_FORMAT_B8G8R8A8_USCALED";
	case VK_FORMAT_B8G8R8A8_SSCALED:                                       return "VK_FORMAT_B8G8R8A8_SSCALED";
	case VK_FORMAT_B8G8R8A8_UINT:                                          return "VK_FORMAT_B8G8R8A8_UINT";
	case VK_FORMAT_B8G8R8A8_SINT:                                          return "VK_FORMAT_B8G8R8A8_SINT";
	case VK_FORMAT_B8G8R8A8_SRGB:                                          return "VK_FORMAT_B8G8R8A8_SRGB";
	case VK_FORMAT_A8B8G8R8_UNORM_PACK32:                                  return "VK_FORMAT_A8B8G8R8_UNORM_PACK32";
	case VK_FORMAT_A8B8G8R8_SNORM_PACK32:                                  return "VK_FORMAT_A8B8G8R8_SNORM_PACK32";
	case VK_FORMAT_A8B8G8R8_USCALED_PACK32:                                return "VK_FORMAT_A8B8G8R8_USCALED_PACK32";
	case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:                                return "VK_FORMAT_A8B8G8R8_SSCALED_PACK32";
	case VK_FORMAT_A8B8G8R8_UINT_PACK32:                                   return "VK_FORMAT_A8B8G8R8_UINT_PACK32";
	case VK_FORMAT_A8B8G8R8_SINT_PACK32:                                   return "VK_FORMAT_A8B8G8R8_SINT_PACK32";
	case VK_FORMAT_A8B8G8R8_SRGB_PACK32:                                   return "VK_FORMAT_A8B8G8R8_SRGB_PACK32";
	case VK_FORMAT_A2R10G10B10_UNORM_PACK32:                               return "VK_FORMAT_A2R10G10B10_UNORM_PACK32";
	case VK_FORMAT_A2R10G10B10_SNORM_PACK32:                               return "VK_FORMAT_A2R10G10B10_SNORM_PACK32";
	case VK_FORMAT_A2R10G10B10_USCALED_PACK32:                             return "VK_FORMAT_A2R10G10B10_USCALED_PACK32";
	case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:                             return "VK_FORMAT_A2R10G10B10_SSCALED_PACK32";
	case VK_FORMAT_A2R10G10B10_UINT_PACK32:                                return "VK_FORMAT_A2R10G10B10_UINT_PACK32";
	case VK_FORMAT_A2R10G10B10_SINT_PACK32:                                return "VK_FORMAT_A2R10G10B10_SINT_PACK32";
	case VK_FORMAT_A2B10G10R10_UNORM_PACK32:                               return "VK_FORMAT_A2B10G10R10_UNORM_PACK32";
	case VK_FORMAT_A2B10G10R10_SNORM_PACK32:                               return "VK_FORMAT_A2B10G10R10_SNORM_PACK32";
	case VK_FORMAT_A2B10G10R10_USCALED_PACK32:                             return "VK_FORMAT_A2B10G10R10_USCALED_PACK32";
	case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:                             return "VK_FORMAT_A2B10G10R10_SSCALED_PACK32";
	case VK_FORMAT_A2B10G10R10_UINT_PACK32:                                return "VK_FORMAT_A2B10G10R10_UINT_PACK32";
	case VK_FORMAT_A2B10G10R10_SINT_PACK32:                                return "VK_FORMAT_A2B10G10R10_SINT_PACK32";
	case VK_FORMAT_R16_UNORM:                                              return "VK_FORMAT_R16_UNORM";
	case VK_FORMAT_R16_SNORM:                                              return "VK_FORMAT_R16_SNORM";
	case VK_FORMAT_R16_USCALED:                                            return "VK_FORMAT_R16_USCALED";
	case VK_FORMAT_R16_SSCALED:                                            return "VK_FORMAT_R16_SSCALED";
	case VK_FORMAT_R16_UINT:                                               return "VK_FORMAT_R16_UINT";
	case VK_FORMAT_R16_SINT:                                               return "VK_FORMAT_R16_SINT";
	case VK_FORMAT_R16_SFLOAT:                                             return "VK_FORMAT_R16_SFLOAT";
	case VK_FORMAT_R16G16_UNORM:                                           return "VK_FORMAT_R16G16_UNORM";
	case VK_FORMAT_R16G16_SNORM:                                           return "VK_FORMAT_R16G16_SNORM";
	case VK_FORMAT_R16G16_USCALED:                                         return "VK_FORMAT_R16G16_USCALED";
	case VK_FORMAT_R16G16_SSCALED:                                         return "VK_FORMAT_R16G16_SSCALED";
	case VK_FORMAT_R16G16_UINT:                                            return "VK_FORMAT_R16G16_UINT";
	case VK_FORMAT_R16G16_SINT:                                            return "VK_FORMAT_R16G16_SINT";
	case VK_FORMAT_R16G16_SFLOAT:                                          return "VK_FORMAT_R16G16_SFLOAT";
	case VK_FORMAT_R16G16B16_UNORM:                                        return "VK_FORMAT_R16G16B16_UNORM";
	case VK_FORMAT_R16G16B16_SNORM:                                        return "VK_FORMAT_R16G16B16_SNORM";
	case VK_FORMAT_R16G16B16_USCALED:                                      return "VK_FORMAT_R16G16B16_USCALED";
	case VK_FORMAT_R16G16B16_SSCALED:                                      return "VK_FORMAT_R16G16B16_SSCALED";
	case VK_FORMAT_R16G16B16_UINT:                                         return "VK_FORMAT_R16G16B16_UINT";
	case VK_FORMAT_R16G16B16_SINT:                                         return "VK_FORMAT_R16G16B16_SINT";
	case VK_FORMAT_R16G16B16_SFLOAT:                                       return "VK_FORMAT_R16G16B16_SFLOAT";
	case VK_FORMAT_R16G16B16A16_UNORM:                                     return "VK_FORMAT_R16G16B16A16_UNORM";
	case VK_FORMAT_R16G16B16A16_SNORM:                                     return "VK_FORMAT_R16G16B16A16_SNORM";
	case VK_FORMAT_R16G16B16A16_USCALED:                                   return "VK_FORMAT_R16G16B16A16_USCALED";
	case VK_FORMAT_R16G16B16A16_SSCALED:                                   return "VK_FORMAT_R16G16B16A16_SSCALED";
	case VK_FORMAT_R16G16B16A16_UINT:                                      return "VK_FORMAT_R16G16B16A16_UINT";
	case VK_FORMAT_R16G16B16A16_SINT:                                      return "VK_FORMAT_R16G16B16A16_SINT";
	case VK_FORMAT_R16G16B16A16_SFLOAT:                                    return "VK_FORMAT_R16G16B16A16_SFLOAT";
	case VK_FORMAT_R32_UINT:                                               return "VK_FORMAT_R32_UINT";
	case VK_FORMAT_R32_SINT:                                               return "VK_FORMAT_R32_SINT";
	case VK_FORMAT_R32_SFLOAT:                                             return "VK_FORMAT_R32_SFLOAT";
	case VK_FORMAT_R32G32_UINT:                                            return "VK_FORMAT_R32G32_UINT";
	case VK_FORMAT_R32G32_SINT:                                            return "VK_FORMAT_R32G32_SINT";
	case VK_FORMAT_R32G32_SFLOAT:                                          return "VK_FORMAT_R32G32_SFLOAT";
	case VK_FORMAT_R32G32B32_UINT:                                         return "VK_FORMAT_R32G32B32_UINT";
	case VK_FORMAT_R32G32B32_SINT:                                         return "VK_FORMAT_R32G32B32_SINT";
	case VK_FORMAT_R32G32B32_SFLOAT:                                       return "VK_FORMAT_R32G32B32_SFLOAT";
	case VK_FORMAT_R32G32B32A32_UINT:                                      return "VK_FORMAT_R32G32B32A32_UINT";
	case VK_FORMAT_R32G32B32A32_SINT:                                      return "VK_FORMAT_R32G32B32A32_SINT";
	case VK_FORMAT_R32G32B32A32_SFLOAT:                                    return "VK_FORMAT_R32G32B32A32_SFLOAT";
	case VK_FORMAT_R64_UINT:                                               return "VK_FORMAT_R64_UINT";
	case VK_FORMAT_R64_SINT:                                               return "VK_FORMAT_R64_SINT";
	case VK_FORMAT_R64_SFLOAT:                                             return "VK_FORMAT_R64_SFLOAT";
	case VK_FORMAT_R64G64_UINT:                                            return "VK_FORMAT_R64G64_UINT";
	case VK_FORMAT_R64G64_SINT:                                            return "VK_FORMAT_R64G64_SINT";
	case VK_FORMAT_R64G64_SFLOAT:                                          return "VK_FORMAT_R64G64_SFLOAT";
	case VK_FORMAT_R64G64B64_UINT:                                         return "VK_FORMAT_R64G64B64_UINT";
	case VK_FORMAT_R64G64B64_SINT:                                         return "VK_FORMAT_R64G64B64_SINT";
	case VK_FORMAT_R64G64B64_SFLOAT:                                       return "VK_FORMAT_R64G64B64_SFLOAT";
	case VK_FORMAT_R64G64B64A64_UINT:                                      return "VK_FORMAT_R64G64B64A64_UINT";
	case VK_FORMAT_R64G64B64A64_SINT:                                      return "VK_FORMAT_R64G64B64A64_SINT";
	case VK_FORMAT_R64G64B64A64_SFLOAT:                                    return "VK_FORMAT_R64G64B64A64_SFLOAT";
	case VK_FORMAT_B10G11R11_UFLOAT_PACK32:                                return "VK_FORMAT_B10G11R11_UFLOAT_PACK32";
	case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:                                 return "VK_FORMAT_E5B9G9R9_UFLOAT_PACK32";
	case VK_FORMAT_D16_UNORM:                                              return "VK_FORMAT_D16_UNORM";
	case VK_FORMAT_X8_D24_UNORM_PACK32:                                    return "VK_FORMAT_X8_D24_UNORM_PACK32";
	case VK_FORMAT_D32_SFLOAT:                                             return "VK_FORMAT_D32_SFLOAT";
	case VK_FORMAT_S8_UINT:                                                return "VK_FORMAT_S8_UINT";
	case VK_FORMAT_D16_UNORM_S8_UINT:                                      return "VK_FORMAT_D16_UNORM_S8_UINT";
	case VK_FORMAT_D24_UNORM_S8_UINT:                                      return "VK_FORMAT_D24_UNORM_S8_UINT";
	case VK_FORMAT_D32_SFLOAT_S8_UINT:                                     return "VK_FORMAT_D32_SFLOAT_S8_UINT";
	case VK_FORMAT_BC1_RGB_UNORM_BLOCK:                                    return "VK_FORMAT_BC1_RGB_UNORM_BLOCK";
	case VK_FORMAT_BC1_RGB_SRGB_BLOCK:                                     return "VK_FORMAT_BC1_RGB_SRGB_BLOCK";
	case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:                                   return "VK_FORMAT_BC1_RGBA_UNORM_BLOCK";
	case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:                                    return "VK_FORMAT_BC1_RGBA_SRGB_BLOCK";
	case VK_FORMAT_BC2_UNORM_BLOCK:                                        return "VK_FORMAT_BC2_UNORM_BLOCK";
	case VK_FORMAT_BC2_SRGB_BLOCK:                                         return "VK_FORMAT_BC2_SRGB_BLOCK";
	case VK_FORMAT_BC3_UNORM_BLOCK:                                        return "VK_FORMAT_BC3_UNORM_BLOCK";
	case VK_FORMAT_BC3_SRGB_BLOCK:                                         return "VK_FORMAT_BC3_SRGB_BLOCK";
	case VK_FORMAT_BC4_UNORM_BLOCK:                                        return "VK_FORMAT_BC4_UNORM_BLOCK";
	case VK_FORMAT_BC4_SNORM_BLOCK:                                        return "VK_FORMAT_BC4_SNORM_BLOCK";
	case VK_FORMAT_BC5_UNORM_BLOCK:                                        return "VK_FORMAT_BC5_UNORM_BLOCK";
	case VK_FORMAT_BC5_SNORM_BLOCK:                                        return "VK_FORMAT_BC5_SNORM_BLOCK";
	case VK_FORMAT_BC6H_UFLOAT_BLOCK:                                      return "VK_FORMAT_BC6H_UFLOAT_BLOCK";
	case VK_FORMAT_BC6H_SFLOAT_BLOCK:                                      return "VK_FORMAT_BC6H_SFLOAT_BLOCK";
	case VK_FORMAT_BC7_UNORM_BLOCK:                                        return "VK_FORMAT_BC7_UNORM_BLOCK";
	case VK_FORMAT_BC7_SRGB_BLOCK:                                         return "VK_FORMAT_BC7_SRGB_BLOCK";
	case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:                                return "VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK";
	case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:                                 return "VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK";
	case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:                              return "VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK";
	case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:                               return "VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK";
	case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:                              return "VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK";
	case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:                               return "VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK";
	case VK_FORMAT_EAC_R11_UNORM_BLOCK:                                    return "VK_FORMAT_EAC_R11_UNORM_BLOCK";
	case VK_FORMAT_EAC_R11_SNORM_BLOCK:                                    return "VK_FORMAT_EAC_R11_SNORM_BLOCK";
	case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:                                 return "VK_FORMAT_EAC_R11G11_UNORM_BLOCK";
	case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:                                 return "VK_FORMAT_EAC_R11G11_SNORM_BLOCK";
	case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:                                   return "VK_FORMAT_ASTC_4x4_UNORM_BLOCK";
	case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:                                    return "VK_FORMAT_ASTC_4x4_SRGB_BLOCK";
	case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:                                   return "VK_FORMAT_ASTC_5x4_UNORM_BLOCK";
	case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:                                    return "VK_FORMAT_ASTC_5x4_SRGB_BLOCK";
	case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:                                   return "VK_FORMAT_ASTC_5x5_UNORM_BLOCK";
	case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:                                    return "VK_FORMAT_ASTC_5x5_SRGB_BLOCK";
	case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:                                   return "VK_FORMAT_ASTC_6x5_UNORM_BLOCK";
	case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:                                    return "VK_FORMAT_ASTC_6x5_SRGB_BLOCK";
	case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:                                   return "VK_FORMAT_ASTC_6x6_UNORM_BLOCK";
	case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:                                    return "VK_FORMAT_ASTC_6x6_SRGB_BLOCK";
	case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:                                   return "VK_FORMAT_ASTC_8x5_UNORM_BLOCK";
	case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:                                    return "VK_FORMAT_ASTC_8x5_SRGB_BLOCK";
	case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:                                   return "VK_FORMAT_ASTC_8x6_UNORM_BLOCK";
	case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:                                    return "VK_FORMAT_ASTC_8x6_SRGB_BLOCK";
	case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:                                   return "VK_FORMAT_ASTC_8x8_UNORM_BLOCK";
	case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:                                    return "VK_FORMAT_ASTC_8x8_SRGB_BLOCK";
	case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:                                  return "VK_FORMAT_ASTC_10x5_UNORM_BLOCK";
	case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:                                   return "VK_FORMAT_ASTC_10x5_SRGB_BLOCK";
	case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:                                  return "VK_FORMAT_ASTC_10x6_UNORM_BLOCK";
	case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:                                   return "VK_FORMAT_ASTC_10x6_SRGB_BLOCK";
	case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:                                  return "VK_FORMAT_ASTC_10x8_UNORM_BLOCK";
	case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:                                   return "VK_FORMAT_ASTC_10x8_SRGB_BLOCK";
	case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:                                 return "VK_FORMAT_ASTC_10x10_UNORM_BLOCK";
	case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:                                  return "VK_FORMAT_ASTC_10x10_SRGB_BLOCK";
	case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:                                 return "VK_FORMAT_ASTC_12x10_UNORM_BLOCK";
	case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:                                  return "VK_FORMAT_ASTC_12x10_SRGB_BLOCK";
	case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:                                 return "VK_FORMAT_ASTC_12x12_UNORM_BLOCK";
	case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:                                  return "VK_FORMAT_ASTC_12x12_SRGB_BLOCK";
	case VK_FORMAT_G8B8G8R8_422_UNORM:                                     return "VK_FORMAT_G8B8G8R8_422_UNORM";
	case VK_FORMAT_B8G8R8G8_422_UNORM:                                     return "VK_FORMAT_B8G8R8G8_422_UNORM";
	case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:                              return "VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM";
	case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:                               return "VK_FORMAT_G8_B8R8_2PLANE_420_UNORM";
	case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:                              return "VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM";
	case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:                               return "VK_FORMAT_G8_B8R8_2PLANE_422_UNORM";
	case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:                              return "VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM";
	case VK_FORMAT_R10X6_UNORM_PACK16:                                     return "VK_FORMAT_R10X6_UNORM_PACK16";
	case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:                               return "VK_FORMAT_R10X6G10X6_UNORM_2PACK16";
	case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:                     return "VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16";
	case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:                 return "VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16";
	case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:                 return "VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16";
	case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:             return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16";
	case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:              return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16";
	case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:             return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16";
	case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:              return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16";
	case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:             return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16";
	case VK_FORMAT_R12X4_UNORM_PACK16:                                     return "VK_FORMAT_R12X4_UNORM_PACK16";
	case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:                               return "VK_FORMAT_R12X4G12X4_UNORM_2PACK16";
	case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:                     return "VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16";
	case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:                 return "VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16";
	case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:                 return "VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16";
	case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:             return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16";
	case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:              return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16";
	case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:             return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16";
	case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:              return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16";
	case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:             return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16";
	case VK_FORMAT_G16B16G16R16_422_UNORM:                                 return "VK_FORMAT_G16B16G16R16_422_UNORM";
	case VK_FORMAT_B16G16R16G16_422_UNORM:                                 return "VK_FORMAT_B16G16R16G16_422_UNORM";
	case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:                           return "VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM";
	case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:                            return "VK_FORMAT_G16_B16R16_2PLANE_420_UNORM";
	case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:                           return "VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM";
	case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:                            return "VK_FORMAT_G16_B16R16_2PLANE_422_UNORM";
	case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:                           return "VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM";
	case VK_FORMAT_G8_B8R8_2PLANE_444_UNORM:                               return "VK_FORMAT_G8_B8R8_2PLANE_444_UNORM";
	case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16:              return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16";
	case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16:              return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16";
	case VK_FORMAT_G16_B16R16_2PLANE_444_UNORM:                            return "VK_FORMAT_G16_B16R16_2PLANE_444_UNORM";
	case VK_FORMAT_A4R4G4B4_UNORM_PACK16:                                  return "VK_FORMAT_A4R4G4B4_UNORM_PACK16";
	case VK_FORMAT_A4B4G4R4_UNORM_PACK16:                                  return "VK_FORMAT_A4B4G4R4_UNORM_PACK16";
	case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK:                                  return "VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK";
	case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK:                                  return "VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK";
	case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK:                                  return "VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK";
	case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK:                                  return "VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK";
	case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK:                                  return "VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK";
	case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK:                                  return "VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK";
	case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK:                                  return "VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK";
	case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK:                                  return "VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK";
	case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK:                                 return "VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK";
	case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK:                                 return "VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK";
	case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK:                                 return "VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK";
	case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK:                                return "VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK";
	case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK:                                return "VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK";
	case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK:                                return "VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK";
	case VK_FORMAT_A1B5G5R5_UNORM_PACK16:                                  return "VK_FORMAT_A1B5G5R5_UNORM_PACK16";		//ONLY IN VULKAN 1.4+
	case VK_FORMAT_A8_UNORM:                                               return "VK_FORMAT_A8_UNORM";						//ONLY IN VULKAN 1.4+
	case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:                            return "VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG";
	case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:                            return "VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG";
	case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:                            return "VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG";
	case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:                            return "VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG";
	case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:                             return "VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG";
	case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:                             return "VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG";
	case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:                             return "VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG";
	case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:                             return "VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG";
	case VK_FORMAT_R16G16_SFIXED5_NV:                                      return "VK_FORMAT_R16G16_SFIXED5_NV";
	case VK_FORMAT_MAX_ENUM:                                               return "VK_FORMAT_MAX_ENUM";
    default:
        return "UNKNOWN";
	}
}

static std::vector<char> CompileShader(const std::filesystem::path &path)
{
    char compile_string[1024];
    char inpath[256];
    char outpath[256];
    std::string cwd = std::filesystem::current_path().string();
    sprintf(inpath, "%s/source/Shaders/%s", cwd.c_str(), path.string().c_str());
    sprintf(outpath, "%s/bin/%s.spv", cwd.c_str(), path.filename().string().c_str());
    sprintf(compile_string, "%s -V %s -o %s --target-env spirv1.4", GLSL_VALIDATOR, inpath, outpath);
    DEBUG_TRACE("[ShaderCompiler] Command: {}", compile_string);
    DEBUG_TRACE("[ShaderCompiler] Output:");
    while (system(compile_string))
    {
        EDITOR_LOG_WARN("[ShaderCompiler] Error! Press something to Compile Again");
        std::cin.get();
    }

    // 'ate' specify to start reading at the end of the file
    // then we can use the read position to determine the size of the file
    std::ifstream file(outpath, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        EDITOR_LOG_CRITICAL("Failed to open file: '{}'", outpath);
    }
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
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

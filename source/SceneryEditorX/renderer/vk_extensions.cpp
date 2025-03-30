/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_extensions.cpp
* -------------------------------------------------------
* Created: 30/3/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/renderer/vk_extensions.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	// Define the static extension collections
	
	// Instance extensions required by the application
	const std::vector<const char *> VulkanExtensions::requiredExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME
	};
	
	// Device extensions required by the application
	const std::vector<const char *> VulkanExtensions::deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	
	// Validation layers (for debug builds)
	const std::vector<const char *> VulkanExtensions::validationLayers = {"VK_LAYER_KHRONOS_validation"};
	
	// Accessor methods
	const std::vector<const char *> &VulkanExtensions::GetRequiredExtensions()
	{
	    return requiredExtensions;
	}
	
	const std::vector<const char *> &VulkanExtensions::GetDeviceExtensions()
	{
	    return deviceExtensions;
	}
	
	const std::vector<const char *> &VulkanExtensions::GetValidationLayers()
	{
	    return validationLayers;
	}
	
} // namespace SceneryEditorX


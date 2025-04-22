/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_checks.h
* -------------------------------------------------------
* Created: 21/3/2025
* -------------------------------------------------------
*/

#pragma once
#include <SceneryEditorX/renderer/render_data.h>
#include <vulkan/vulkan.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	// -------------------------------------------------------

    class VulkanChecks
    {
    public:
        void InitChecks(const std::vector<const char *> &extensions, const std::vector<const char *> &layers, const VkPhysicalDevice &device);

        GLOBAL	 bool IsExtensionSupported(const std::vector<VkExtensionProperties> &availExtensions, const char * extension); // Function to check if an extension is supported
        GLOBAL	 bool CheckAPIVersion(uint32_t minVulkanVersion);						// Function to check if Vulkan API version is compatible
        INTERNAL bool CheckValidationLayerSupport();									// Function to check if validation layers are supported
        INTERNAL bool CheckDeviceExtensionSupport(VkPhysicalDevice device);				// Function to check if required device extensions are available
		GLOBAL   bool CheckDeviceFeatures(const VkPhysicalDevice &device);				// Function to check the supported device features and if they are compatible.

    private:
        INTERNAL void CheckExtensions(const std::vector<const char *> &extensions);      // Function to check if required extensions are available
        void CheckLayers(const std::vector<const char *> &layers);						 // Function to check if required layers are available
        INTERNAL bool IsDeviceCompatible(const VkPhysicalDevice &device);				 // Function to check if a device is compatible

		Layers vkLayerData;
        VkPhysicalDeviceFeatures vkEnabledFeatures;

		friend class VulkanDevice;
    };


} // namespace SceneryEditorX

// -------------------------------------------------------

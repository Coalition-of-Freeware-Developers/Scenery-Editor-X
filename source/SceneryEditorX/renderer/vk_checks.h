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
#include <vector>
#include <vulkan/vulkan.h>
#include <SceneryEditorX/renderer/render_data.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	extern SoftwareStats programStats;

	// -------------------------------------------------------

    class VulkanChecks
    {
    public:
        void InitChecks(const std::vector<const char *> &extensions, const std::vector<const char *> &layers, const VkPhysicalDevice &device);

        static bool IsExtensionSupported(const std::vector<VkExtensionProperties> &availExtensions, const char * extension);
        [[nodiscard]] bool CheckValidationLayerSupport() const;
        bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;

        // Function to check if Vulkan API version is compatible
        static bool CheckAPIVersion(uint32_t minVulkanVersion);

        uint32_t ExtensionCount = 0;
        uint32_t LayerCount = 0;

    private:
        // Function to check if required extensions are available
        void CheckExtensions(const std::vector<const char *> &extensions);

        // Function to check if required layers are available
        void CheckLayers(const std::vector<const char *> &layers);

		// Function to check if a device is compatible
        static bool IsDeviceCompatible(const VkPhysicalDevice& device);



		std::vector<const char *> ValidationLayers = {"VK_LAYER_KHRONOS_validation"};

        /**
         * @brief List of required device extensions.
         */
        const std::vector<const char *> DeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        std::vector<const char *> RequiredExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
			VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME
		};

		friend class VulkanDevice;
    };


} // namespace SceneryEditorX

// -------------------------------------------------------

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
#include <SceneryEditorX/renderer/vulkan/vk_data.h>
#include <vulkan/vulkan.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

    class VulkanChecks : public RefCounted
    {
    public:
        void InitChecks(const char* ext, const std::vector<VkExtensionProperties>& props, const std::vector<const char *> &layers, const VkPhysicalDevice &device);
        INTERNAL bool IsRequiredExtensionSupported(const std::vector<VkExtensionProperties> &availExtensions, const char *extension); /// Function to check if a required extension is supported
        INTERNAL bool IsExtensionSupported(const char *extension);                                                                    /// Function to check if an extension is supported
        INTERNAL bool CheckAPIVersion(uint32_t minVulkanVersion);                                                                     /// Function to check if Vulkan API version is compatible
        [[nodiscard]] bool CheckValidationLayerSupport() const;                                                                       /// Function to check if validation layers are supported
        void CheckDeviceExtensionSupport(VkPhysicalDevice device, std::vector<VkExtensionProperties>& props, const char* validationLayer); /// Function to check if required device extensions are available
        INTERNAL bool CheckDeviceFeatures(const VkPhysicalDevice &device);                                                            /// Function to check the supported device features and if they are compatible.
        INTERNAL bool IsDeviceCompatible(const VkPhysicalDevice &device);                                                             /// Function to check if a device is compatible
        bool CheckExtension(const char* extension, const std::vector<VkExtensionProperties>& props);								  /// Function to check if required extensions are available
        void CheckLayers(const std::vector<const char *> &layers);																	  /// Function to check if required layers are available

    private:
        Extensions vkExtensions;
        VkPhysicalDeviceFeatures vkEnabledFeatures;


        friend class VulkanDevice;
    };

} // namespace SceneryEditorX

/// -------------------------------------------------------

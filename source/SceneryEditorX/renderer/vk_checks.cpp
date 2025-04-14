/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_checks.cpp
* -------------------------------------------------------
* Created: 21/3/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/renderer/vk_checks.h>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @brief Initializes the Vulkan checks.
	 * @param extensions 
	 * @param layers
	 * @param device 
	 */
	void VulkanChecks::InitChecks(const std::vector<const char *> &extensions, const std::vector<const char *> &layers, const VkPhysicalDevice &device)
	{
        CheckAPIVersion(programStats.minVulkanVersion);
	    CheckExtensions(extensions);
	    CheckLayers(layers);
        IsDeviceCompatible(device);
	}

	/**
	 * @brief Checks to see if the Vulkan API version is compatible.
	 * @param minVulkanVersion
	 * @param minimumVulkanVersion 
	 * @return True if the Vulkan API version is compatible.
	 * @return False if the Vulkan API version is not compatible.
	 */
    bool VulkanChecks::CheckAPIVersion(uint32_t minVulkanVersion)
    {
        uint32_t instanceVersion;
		vkEnumerateInstanceVersion(&instanceVersion);

		if (instanceVersion < minVulkanVersion)
		{
			SEDX_CORE_ERROR("Incompatible Vulkan driver version!");
            SEDX_CORE_ERROR("  You have {}.{}.{}.{}",
							 VK_API_VERSION_VARIANT(instanceVersion),
							 VK_API_VERSION_MAJOR(instanceVersion),
							 VK_API_VERSION_MINOR(instanceVersion),
							 VK_API_VERSION_PATCH(instanceVersion));

            SEDX_CORE_ERROR("  You need at least {}.{}.{}.{}",
							 VK_API_VERSION_VARIANT(minVulkanVersion),
							 VK_API_VERSION_MAJOR(minVulkanVersion),
							 VK_API_VERSION_MINOR(minVulkanVersion),
							 VK_API_VERSION_PATCH(minVulkanVersion));

			return false;
		}

		return true;
    }

    /**
	 * @brief Checks to see if the device has support for the required extensions.
	 * @param availExtensions 
	 * @param extension 
	 * @return True if the device has support for the required extensions.
	 * @return False if the device cannot support the required extensions.
	 */
	bool VulkanChecks::IsExtensionSupported(const std::vector<VkExtensionProperties> &availExtensions, const char *const extension)
	{
	    for (const auto &[extensionName, specVersion] : availExtensions)
	    {
	        if (strstr(extensionName, extension))
	        {
	            return true;
	        }
	    }
	
	    return false;
    }

	/**
	 * @brief Checks to see if the device has support for the required layers.
	 * @return True if the device has support for the required layers.
	 * @return False if the device cannot support the required layers.
	 */
    bool VulkanChecks::CheckValidationLayerSupport() const
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char *layerName : ValidationLayers)
        {
            auto layerFound = false;

            for (const auto &layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
            {
                return false;
            }
        }

        return true;
    }

	/**
	 * @brief Checks to see if the device has support for the required extensions.
	 * @param device
	 * @return True if the device has support for the required extensions.
	 * @return False if the device cannot support the required extensions.
	 */
	bool VulkanChecks::CheckDeviceExtensionSupport(const VkPhysicalDevice device) const
    {

        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(DeviceExtensions.begin(), DeviceExtensions.end());

		SEDX_CORE_INFO("Checking for required device extensions:");
        for (const auto &extension : DeviceExtensions)
        {
            SEDX_CORE_INFO("Required: {}", ToString(extension));
        }

        SEDX_CORE_INFO("Available device extensions:");
        for (const auto &extension : availableExtensions)
        {
            SEDX_CORE_INFO("  Available: {}", ToString(extension.extensionName));
            requiredExtensions.erase(extension.extensionName);
        }

        if (!requiredExtensions.empty())
        {
            SEDX_CORE_ERROR("Missing extensions:");
            for (const auto &ext : requiredExtensions)
            {
                SEDX_CORE_ERROR("  Missing: {}", ToString(ext));
            }
            return false;
        }

        return true;
    }

	/**
	 * @brief Checks to see if the device has support for the required extensions.
	 * @param extensions
	 * @return True if the device has support for the required extensions.
	 * @return False if the device cannot support the required extensions.
	 */
	void VulkanChecks::CheckExtensions(const std::vector<const char *> &extensions)
	{
	    vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr);
	
	    std::vector<VkExtensionProperties> availableExtensions(ExtensionCount);
	    vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, availableExtensions.data());
	#ifdef SEDX_DEBUG
	    SEDX_CORE_INFO("============================================");
	    SEDX_CORE_INFO("Available extensions");
	    SEDX_CORE_INFO("____________________________________________");
	#endif
	    for (const auto &[extensionName, specVersion] : availableExtensions)
	    {
	#ifdef SEDX_DEBUG
	        SEDX_CORE_INFO("{}", ToString(extensionName));
	#endif
	    }
	#ifdef SEDX_DEBUG
	    SEDX_CORE_INFO("============================================");
	#endif
	    for (const char *required : extensions)
	    {
	        bool found = false;
	        for (const auto &[extensionName, specVersion] : availableExtensions)
	        {
	            if (strcmp(required, extensionName) == 0)
	            {
	                found = true;
	                SEDX_CORE_INFO("Found extension: {}", ToString(required));
	                break;
	            }
	        }
	        if (!found)
	        {
	            SEDX_CORE_ERROR("Required extension not found: {}", ToString(required));
	            break;
	        }
	    }
	
	    return;
	}

	/**
	 * @brief Checks to see if the device has support for the required layers.
	 * @param layers
	 * @return True if the device has support for the required layers.
	 * @return False if the device cannot support the required layers.
	 */
	void VulkanChecks::CheckLayers(const std::vector<const char *> &layers)
	{
	    vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);
	
	    std::vector<VkLayerProperties> availableLayers(LayerCount);
	    vkEnumerateInstanceLayerProperties(&LayerCount, availableLayers.data());
	#ifdef SEDX_DEBUG
	    SEDX_CORE_INFO("Available layer/s");
	    SEDX_CORE_INFO("____________________________________________");
	#endif
	    for (const auto &layer : availableLayers)
	    {
	#ifdef SEDX_DEBUG
	        SEDX_CORE_INFO("{}", ToString(layer.layerName));
	#endif
	    }
	#ifdef SEDX_DEBUG
	    SEDX_CORE_INFO("============================================");
	#endif
	    for (const char *required : layers)
	    {
	        bool found = false;
	        for (const auto &layer : availableLayers)
	        {
	            if (strcmp(required, layer.layerName) == 0)
	            {
	                found = true;
	                SEDX_CORE_INFO("Found layer/s: {}", ToString(required));
	                break;
	            }
	        }
	        if (!found)
	        {
	            SEDX_CORE_ERROR("Required layer not found: {}", ToString(required));
	            break;
	        }
	    }
	
	    return;
	}

	/**
	 * @brief - Check if a device is suitable for rendering.
	 * 
	 * @param device - The Vulkan physical device.
	 * @return - bool True if the device is suitable, false otherwise.
	 */
    bool VulkanChecks::IsDeviceCompatible(const VkPhysicalDevice &device)
    {
        VkPhysicalDeviceFeatures deviceFeatures;
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        // Check for required features and properties
        const bool isSuitable = (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) &&
                          (deviceFeatures.geometryShader) && (deviceFeatures.tessellationShader);

        if (!isSuitable)
        {
            SEDX_CORE_ERROR("Vulkan: Device does not meet required features or is not discrete GPU");
            ErrMsg("Vulkan: Device does not meet required features or is not discrete GPU");
            return false;

			//TODO: Add error handling in case the device is an integrated GPU.
			//TODO: Set up a fallback destructor to not continue device creation.
        }

        return true;
    }


} // namespace SceneryEditorX

// -------------------------------------------------------

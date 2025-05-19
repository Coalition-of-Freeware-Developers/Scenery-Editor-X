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
#include <SceneryEditorX/core/application_data.h>
#include <SceneryEditorX/vulkan/vk_checks.h>
#include <SceneryEditorX/vulkan/vk_core.h>
#include <SceneryEditorX/vulkan/vk_util.h>

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
        CheckAPIVersion(SoftwareStats::minVulkanVersion);
	    CheckExtensions(extensions);
	    CheckLayers(layers);
	}

	/**
	 * @brief Checks to see if the Vulkan API version is compatible.
	 * @param minVulkanVersion
	 * @param minimumVulkanVersion 
	 * @return True if the Vulkan API version is compatible.
	 * @return False if the Vulkan API version is not compatible.
	 */
    bool VulkanChecks::CheckAPIVersion(const uint32_t minVulkanVersion)
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
	bool VulkanChecks::IsRequiredExtensionSupported(const std::vector<VkExtensionProperties> &availExtensions, const char *const extension)
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
     * @brief Checks to see if the device has support for the required extensions.
     * @param extension The extension name to check for.
     * @return True if the device has support for the required extensions.
     * @return False if the device cannot support the required extensions.
     */
    bool VulkanChecks::IsExtensionSupported(const char *extension)
    {
        uint32_t extensionCount = 0;

        std::unordered_set<std::string> supportedExtension;
        supportedExtension.clear();

        vkEnumerateDeviceExtensionProperties(GraphicsEngine::GetCurrentDevice()->GetPhysicalDevice()->physicalDevice, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
        for (const auto &ext: availableExtensions)
        {
            supportedExtension.emplace(ext.extensionName);
        }

		if (supportedExtension.contains(extension))
        {
            SEDX_CORE_INFO("Extension supported: {}", ToString(extension));
            return true;
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
        if (!vkLayers.validationLayer.empty())
        {
            // Add validation
            uint32_t layerCount = 0;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

            std::vector<VkLayerProperties> availableLayers(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

            for (const char *layerName : vkLayers.validationLayer)
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
        }

        return true;
    }

	/**
	 * @brief Checks to see if the device has support for the required extensions.
	 * @param device
	 * @return True if the device has support for the required extensions.
	 * @return False if the device cannot support the required extensions.
	 */
	bool VulkanChecks::CheckDeviceExtensionSupport(const VkPhysicalDevice device)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(vkExtensions.deviceExtensions.begin(),
                                                 vkExtensions.deviceExtensions.end());

		SEDX_CORE_INFO("Checking for required device extensions:");
        for (const auto &extension : vkExtensions.deviceExtensions)
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
        uint32_t extensionCount;
	    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
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
	    vkEnumerateInstanceLayerProperties(&vkLayers.layerCount, nullptr);
	
	    std::vector<VkLayerProperties> availableLayers(vkLayers.layerCount);
	    vkEnumerateInstanceLayerProperties(&vkLayers.layerCount, availableLayers.data());
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
	 * @brief Checks to see if the device has support for the required features.
	 * @param device - The Vulkan physical device.
	 * @return True if the device has support for the required features.
	 * @return False if the device cannot support the required features.
	 */
    bool VulkanChecks::CheckDeviceFeatures(const VkPhysicalDevice &device)
    {
	    VkPhysicalDeviceFeatures deviceFeatures;
	    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        constexpr VulkanDeviceFeatures requiredFeatures; // Create a temporary instance with default values
	    
	    // Check for all true features in our configuration if they're supported by the device
	    bool missingFeatures = false;
	    std::string missingFeaturesLog;
	
	    // Check all features that might be required by our application
        CHECK_FEATURE(robustBufferAccess)
        CHECK_FEATURE(fullDrawIndexUint32)
        CHECK_FEATURE(imageCubeArray)
        CHECK_FEATURE(independentBlend)
        CHECK_FEATURE(geometryShader)
        CHECK_FEATURE(tessellationShader)
        CHECK_FEATURE(sampleRateShading)
        CHECK_FEATURE(dualSrcBlend)
        CHECK_FEATURE(logicOp)
        CHECK_FEATURE(multiDrawIndirect)
        CHECK_FEATURE(drawIndirectFirstInstance)
        CHECK_FEATURE(depthClamp)
        CHECK_FEATURE(depthBiasClamp)
        CHECK_FEATURE(fillModeNonSolid)
        CHECK_FEATURE(depthBounds)
        CHECK_FEATURE(wideLines)
        CHECK_FEATURE(largePoints)
        CHECK_FEATURE(alphaToOne)
        CHECK_FEATURE(multiViewport)
        CHECK_FEATURE(samplerAnisotropy)
        CHECK_FEATURE(textureCompressionETC2)
        CHECK_FEATURE(textureCompressionASTC_LDR)
        CHECK_FEATURE(textureCompressionBC)
        CHECK_FEATURE(occlusionQueryPrecise)
        CHECK_FEATURE(pipelineStatisticsQuery)
        CHECK_FEATURE(vertexPipelineStoresAndAtomics)
        CHECK_FEATURE(fragmentStoresAndAtomics)
        CHECK_FEATURE(shaderTessellationAndGeometryPointSize)
        CHECK_FEATURE(shaderImageGatherExtended)
        CHECK_FEATURE(shaderStorageImageExtendedFormats)
        CHECK_FEATURE(shaderStorageImageMultisample)
        CHECK_FEATURE(shaderStorageImageReadWithoutFormat)
        CHECK_FEATURE(shaderStorageImageWriteWithoutFormat)
        CHECK_FEATURE(shaderUniformBufferArrayDynamicIndexing)
        CHECK_FEATURE(shaderSampledImageArrayDynamicIndexing)
        CHECK_FEATURE(shaderStorageBufferArrayDynamicIndexing)
        CHECK_FEATURE(shaderStorageImageArrayDynamicIndexing)
        CHECK_FEATURE(shaderClipDistance)
	    CHECK_FEATURE(shaderCullDistance)
	    CHECK_FEATURE(shaderFloat64)
	    CHECK_FEATURE(shaderInt64)
	    CHECK_FEATURE(shaderInt16)
	    CHECK_FEATURE(shaderResourceResidency)
	    CHECK_FEATURE(shaderResourceMinLod)
	    CHECK_FEATURE(sparseBinding)
        CHECK_FEATURE(sparseResidencyBuffer)
        CHECK_FEATURE(sparseResidencyImage2D)
        CHECK_FEATURE(sparseResidencyImage3D)
        CHECK_FEATURE(sparseResidency2Samples)
        CHECK_FEATURE(sparseResidency4Samples)
        CHECK_FEATURE(sparseResidency8Samples)
        CHECK_FEATURE(sparseResidency16Samples)
        CHECK_FEATURE(sparseResidencyAliased)
        CHECK_FEATURE(variableMultisampleRate)
        CHECK_FEATURE(inheritedQueries)

        if (missingFeatures)
	    {
	        SEDX_CORE_ERROR("Vulkan: Your device does not support all required features:");
            //SEDX_CORE_ERROR("Missing features: {}", missingFeaturesLog);
	        ErrMsg("Vulkan: Device does not support all required features");
	        return false;
	    }
	
	    SEDX_CORE_INFO("Vulkan: All required device features are supported");
	    return true;
    }

	/**
	 * @brief - Check if a device is suitable for rendering.
	 * @param device - The Vulkan physical device.
	 * @return True if the device is suitable.
	 * @return False if the device is not suitable.
	 */
    bool VulkanChecks::IsDeviceCompatible(const VkPhysicalDevice &device)
    {
        VkPhysicalDeviceFeatures deviceFeatures;
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        /// Check for required device type (discrete GPU preferred)
        if (bool isDiscreteGPU = (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU); !isDiscreteGPU)
        {
            SEDX_CORE_WARN("Vulkan: Device is not a discrete GPU. Performance might be affected.");
        }

        if (bool isSuitable = (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) && (CheckDeviceFeatures(device) == true); !isSuitable)
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

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
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/vulkan/vk_checks.h>
#include <SceneryEditorX/renderer/vulkan/vk_util.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    /**
	 * @brief Initializes the Vulkan checks.
	 * @param ext
	 * @param props
	 * @param layers
	 * @param device 
	 */
    void VulkanChecks::InitChecks(const char *ext,
                              const std::vector<VkExtensionProperties> &props,
                              const std::vector<const char *> &layers,
                              const VkPhysicalDevice &device)
	{
	    CheckAPIVersion(RenderData::minVulkanVersion);
	    CheckExtension(ext, props);
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
	    if (VkResult result = vkEnumerateInstanceVersion(&instanceVersion); result != VK_SUCCESS)
	    {
	        SEDX_CORE_ERROR("Failed to enumerate instance version: {}", ToString(result));
	        return false;
	    }
	
	    RenderData::apiVersion vulkanVersion = {vulkanVersion.Variant = VK_API_VERSION_VARIANT(instanceVersion),
	                                            vulkanVersion.Major = VK_API_VERSION_MAJOR(instanceVersion),
	                                            vulkanVersion.Minor = VK_API_VERSION_MINOR(instanceVersion),
	                                            vulkanVersion.Patch = VK_API_VERSION_PATCH(instanceVersion)};
	
	    if (instanceVersion < minVulkanVersion)
	    {
	        SEDX_CORE_ERROR_TAG("Graphics Engine", "Installed Vulkan API version is incompatible with the program!");
	        SEDX_CORE_ERROR("You have {}.{}.{}.{}",
	                        VK_API_VERSION_VARIANT(instanceVersion),
	                        VK_API_VERSION_MAJOR(instanceVersion),
	                        VK_API_VERSION_MINOR(instanceVersion),
	                        VK_API_VERSION_PATCH(instanceVersion));
	
	        SEDX_CORE_ERROR("You need at least {}.{}.{}.{}",
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
	 *
	 * @param availExtensions - The available extensions.
	 * @param extension - The extension name to check for.
	 * @return True if the device has support for the required extensions.
	 * @return False if the device cannot support the required extensions.
	 */
	bool VulkanChecks::IsRequiredExtensionSupported(const std::vector<VkExtensionProperties> &availExtensions,
	                                                const char *const extension)
	{
	
	    for (const auto &[extensionName, specVersion] : availExtensions)
	    {
	        if (strstr(extensionName, extension))
	            return true;
	    }
	
	
	    return false;
	}
	
	/**
     * @brief Checks to see if the device has support for the required extensions.
     *
     * @param extension The extension name to check for.
     * @return True if the device has support for the required extensions.
     * @return False if the device cannot support the required extensions.
     */
    bool VulkanChecks::IsExtensionSupported(const char *extension)
    {
        uint32_t extensionCount = 0;
        if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to get instance extension count");
            return false;
        }

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data()) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to enumerate instance extensions");
            return false;
        }

        for (const auto &[extensionName, specVersion] : availableExtensions)
        {
            if (strcmp(extensionName, extension) == 0)
            {
                SEDX_CORE_INFO("Extension supported: {}", extension);
                return true;
            }
        }

        return false;
    }
	
	
	/**
	 * @brief Checks to see if the device has support for the required extensions.
	 *
	 * @param device - The Vulkan physical device.
	 * @param props
	 * @param validationLayer
	 * @return True if the device has support for the required extensions.
	 * @return False if the device cannot support the required extensions.
	 */
	void VulkanChecks::CheckDeviceExtensionSupport(VkPhysicalDevice device, std::vector<VkExtensionProperties> &props, const char *validationLayer = nullptr)
	{
	    uint32_t extensionCount = 0;
	    vkEnumerateDeviceExtensionProperties(device, validationLayer, &extensionCount, nullptr);
	
	    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	    vkEnumerateDeviceExtensionProperties(device, validationLayer, &extensionCount, availableExtensions.data());
	    props.insert(props.end(), availableExtensions.begin(), availableExtensions.end());
	}
	
	/**
	 * @brief Checks to see if the device has support for the required extensions.
	 *
	 * @param extension
	 * @param props
	 * @param extensions - The extensions to check for.
	 * @return True if the device has support for the required extensions.
	 * @return False if the device cannot support the required extensions.
	 */
	bool VulkanChecks::CheckExtension(const char *extension, const std::vector<VkExtensionProperties> &props)
	{
	    for (const VkExtensionProperties &extensionProps : props)
	    {
	        if (strcmp(extension, extensionProps.extensionName) == 0)
	            return true;
	    }

	    return false;
	}

    /**
	 * @brief Checks to see if the device has support for the required features.
	 *
	 * @param device - The Vulkan physical device.
	 * @return True if the device has support for the required features.
	 * @return False if the device cannot support the required features.
	 */
	bool VulkanChecks::CheckDeviceFeatures(const VkPhysicalDevice &device)
	{
	    VkPhysicalDeviceFeatures deviceFeatures;
	    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	
	    constexpr VulkanDeviceFeatures requiredFeatures; /// Create a temporary instance with default values
	
	    /// Check for all true features in our configuration if they're supported by the device
	    bool missingFeatures = false;
	    std::string missingFeaturesLog;
	
	    /// Check all features that might be required by our application
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
	        SEDX_CORE_ERROR_TAG("Graphics Core", "Your device does not support all required Vulkan device features!");
	
	        return false;
	    }
	
	    SEDX_CORE_TRACE_TAG("Graphics Core", "All required Vulkan device features are supported");
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
	        SEDX_CORE_WARN_TAG("Graphics Core", "Graphics device is not a discrete GPU. Performance might be affected.");
	
	    if (const bool isSuitable = (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) &&
	                                (CheckDeviceFeatures(device) == true);
	        !isSuitable)
	    {
	        SEDX_CORE_ERROR_TAG(
	            "Graphics Core",
	            "Your device does not contain the features required for this Vulkan device or is not discrete GPU");
	        return false;
	
	        //TODO: Add error handling in case the device is an integrated GPU.
	        //TODO: Set up a fallback destructor to not continue device creation.
	    }
	
	    return true;
	}

    /**
     * @brief Checks if all required validation layers are available
     * 
     * @param layers The validation layers to check for
     */
    void VulkanChecks::CheckLayers(const std::vector<const char*>& layers)
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        
        if (layerCount == 0) 
        {
            SEDX_CORE_ERROR_TAG("Vulkan", "No validation layers are available on this system");
            return;
        }

        SEDX_CORE_INFO_TAG("Vulkan", "Found {} available validation layers", layerCount);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        // List all available layers for debugging
        SEDX_CORE_TRACE_TAG("Vulkan", "Available validation layers:");
        for (const auto& layer : availableLayers)
        {
            SEDX_CORE_TRACE_TAG("Vulkan", "  {} (version: {}, spec: {})", 
                layer.layerName, 
                layer.implementationVersion,
                layer.specVersion);
        }

        // Check if all requested layers are available
        bool allLayersFound = true;
        for (const char* layerName : layers)
        {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    SEDX_CORE_INFO_TAG("Vulkan", "Validation layer supported: {}", layerName);
                    break;
                }
            }

            if (!layerFound)
            {
                SEDX_CORE_WARN_TAG("Vulkan", "Validation layer not supported: {}", layerName);
                allLayersFound = false;
            }
        }
        
        if (!allLayersFound)
        {
            SEDX_CORE_WARN_TAG("Vulkan", "Not all requested validation layers are available");
        }
    }

    /**
     * @brief Checks if validation layer support is available
     * 
     * @param layers The specific validation layers to check for
     * @return true if all requested validation layers are supported
     * @return false if any requested validation layer is not supported
     */
    bool VulkanChecks::CheckValidationLayerSupport() const
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        if (layerCount == 0)
        {
            SEDX_CORE_ERROR_TAG("Vulkan", "No validation layers available on this system");
            return false;
        }

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        
        // Check for VK_LAYER_KHRONOS_validation specifically
        for (const auto& layerProperties : availableLayers) 
        {
            if (strcmp("VK_LAYER_KHRONOS_validation", layerProperties.layerName) == 0) 
            {
                SEDX_CORE_INFO_TAG("Vulkan", "Khronos validation layer is available (version: {})", 
                    layerProperties.implementationVersion);
                return true;
            }
        }
        
        SEDX_CORE_WARN_TAG("Vulkan", "Khronos validation layer is not available");
        return false;
    }

} // namespace SceneryEditorX

/// -------------------------------------------------------

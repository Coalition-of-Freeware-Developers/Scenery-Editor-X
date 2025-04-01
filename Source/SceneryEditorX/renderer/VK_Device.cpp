/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_device.cpp
* -------------------------------------------------------
* Created: 21/3/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/renderer/vk_device.h>
#include <SceneryEditorX/renderer/vk_util.h>
#include <SceneryEditorX/renderer/vk_core.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	vkPhysDevice::vkPhysDevice()
	{
	}

	vkPhysDevice::~vkPhysDevice()
	{
        vkDestroyCommandPool(device, commandPool, nullptr);
        vkDestroyDevice(device, nullptr);
	}

	/**
	 * @brief Initialize the Vulkan physical device.
	 * 
	 * @param Instance_ - The Vulkan instance.
	 * @param surface - The Vulkan surface.
	 * 
	 */
    void vkPhysDevice::Init(const VkInstance &instance, const VkSurfaceKHR &surface)
    {
        uint32_t GFXDevices = 0;      // Number of physical devices
        VkResult result = VK_SUCCESS; // Vulkan result
        std::vector<VkPhysicalDevice> Device;

        // -------------------------------------------------------

        result = vkEnumeratePhysicalDevices(instance, &GFXDevices, NULL);
        VK_CHECK_RESULT(result, "Vulkan Physical Devices");
        if (GFXDevices > 0)
        {
            Device.resize(GFXDevices);
            result = vkEnumeratePhysicalDevices(instance, &GFXDevices, Device.data());
            if ((result != VK_SUCCESS) || (GFXDevices == 0))
            {
                EDITOR_LOG_ERROR("Could not enumerate physical devices.");
                VK_CHECK_RESULT(result, "Vulkan Physical Devices");
                ErrMsg("Could not enumerate physical devices.");
                return;
            }

            // Resize the devices vector to accommodate the number of physical devices
            devices.resize(GFXDevices);

            for (uint32_t index = 0; index < GFXDevices; index++)
            {
                VkPhysicalDevice physDevice = Device[index];
                devices[index].physicalDevice = physDevice;
            }
        }
        else
        {
            EDITOR_LOG_ERROR("No physical devices found.");
            VK_CHECK_RESULT(result, "Vulkan Physical Devices");
            ErrMsg("No physical devices found.");
        }

        // -------------------------------------------------------

        for (uint32_t index = 0; index < GFXDevices; index++)
        {
            VkPhysicalDevice physDevice = Device[index];
            devices[index].physicalDevice = physDevice;

            // -------------------------------------------------------

            vkGetPhysicalDeviceProperties(physDevice, &devices[index].deviceInfo);

            DEBUG_TRACE("============================================");
            DEBUG_TRACE("Device Name: {}", ToString(devices[index].deviceInfo.deviceName));
            DEBUG_TRACE("Device Type: {}", ToString(devices[index].deviceInfo.deviceType));
            DEBUG_TRACE("Device ID: {}", ToString(devices[index].deviceInfo.deviceID));
            DEBUG_TRACE("Driver Version: {}", ToString(devices[index].deviceInfo.driverVersion));
            DEBUG_TRACE("API Version: {}", ToString(devices[index].deviceInfo.apiVersion));
            DEBUG_TRACE("Vendor ID: {}", ToString(devices[index].deviceInfo.vendorID));
            DEBUG_TRACE("============================================");

            // -------------------------------------------------------

            uint32_t numQueueFamilies = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &numQueueFamilies, NULL);

            // -------------------------------------------------------

            EDITOR_LOG_INFO("Number of GPU device family queues: {}", ToString(numQueueFamilies));

            // -------------------------------------------------------

            devices[index].queueFamilyInfo.resize(numQueueFamilies);
            devices[index].queueSupportPresent.resize(numQueueFamilies);

            vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &numQueueFamilies, devices[index].queueFamilyInfo.data());
            for (uint32_t queue = 0; queue < numQueueFamilies; queue++)
            {
                const VkQueueFamilyProperties &queueFamilyInfo = devices[index].queueFamilyInfo[queue];

                EDITOR_LOG_INFO("============================================");
                EDITOR_LOG_INFO("Queue Family Index: {}", ToString(queue));
                EDITOR_LOG_INFO("Queue Count: {}", ToString(queueFamilyInfo.queueCount));
                EDITOR_LOG_INFO("Queue Flags: {}", ToString(queueFamilyInfo.queueFlags));
                EDITOR_LOG_INFO("============================================");

                result = vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, queue, surface, &devices[index].queueSupportPresent[queue]);
                VK_CHECK_RESULT(result, "Vulkan Physical Device Surface Support");
            }

            // -------------------------------------------------------

            uint32_t numSurfaceFormats = 0;

            result = vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &numSurfaceFormats, NULL);
            VK_CHECK_RESULT(result, "Vulkan Physical Device Surface Formats");

            EDITOR_LOG_INFO("Number of surface formats: {}", ToString(numSurfaceFormats));

            devices[index].surfaceFormats.resize(numSurfaceFormats);

            result = vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &numSurfaceFormats, devices[index].surfaceFormats.data());
            VK_CHECK_RESULT(result, "Vulkan Physical Device Surface Formats");

            for (uint32_t format = 0; format < numSurfaceFormats; format++)
            {
                const VkSurfaceFormatKHR &surfaceFormat = devices[index].surfaceFormats[format];
                EDITOR_LOG_INFO("============================================");
                EDITOR_LOG_INFO("Surface Format: {}", ToString(surfaceFormat.format));
                EDITOR_LOG_INFO("Color Space: {}", ToString(surfaceFormat.colorSpace));
                EDITOR_LOG_INFO("============================================");
            }

            result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &(devices[index].surfaceCapabilities));
            //if (result == VK_ERROR_surfaceLOST_KHR)
            //{
            //    // Handle surface lost error
            //    EDITOR_LOG_ERROR("Surface lost. Recreating surface...");
            //    // Code to recreate the surface
            //    // surface = RecreateSurface();
            //    // Retry the call
            //    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice,
            //                                                       surface,
            //                                                       &(devices[index].surfaceCapabilities));
            //}
            //VK_CHECK_RESULT(result, "Vulkan Physical Device Surface Capabilities");

            EDITOR_LOG_INFO("============================================");
            EDITOR_LOG_INFO("Surface Capabilities");
            EDITOR_LOG_INFO("____________________________________________");
            EDITOR_LOG_INFO("Min Image Count: {}", ToString(devices[index].surfaceCapabilities.minImageCount));
            EDITOR_LOG_INFO("Max Image Count: {}", ToString(devices[index].surfaceCapabilities.maxImageCount));
            EDITOR_LOG_INFO("Current Extent: {}", ToString(devices[index].surfaceCapabilities.currentExtent.width));
            EDITOR_LOG_INFO("Min Image Extent: {}", ToString(devices[index].surfaceCapabilities.minImageExtent.width));
            EDITOR_LOG_INFO("Max Image Extent: {}", ToString(devices[index].surfaceCapabilities.maxImageExtent.width));
            EDITOR_LOG_INFO("Max Image Array Layers: {}", ToString(devices[index].surfaceCapabilities.maxImageArrayLayers));
            EDITOR_LOG_INFO("Supported Transforms: {}", ToString(devices[index].surfaceCapabilities.supportedTransforms));
            EDITOR_LOG_INFO("Current Transform: {}", ToString(devices[index].surfaceCapabilities.currentTransform));
            EDITOR_LOG_INFO("Supported Composite Alpha: {}", ToString(devices[index].surfaceCapabilities.supportedCompositeAlpha));
            EDITOR_LOG_INFO("Supported Usage Flags: {}", ToString(devices[index].surfaceCapabilities.supportedUsageFlags));
            EDITOR_LOG_INFO("============================================");

            // -------------------------------------------------------

            uint32_t numPresentModes = 0;
            result = vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &numPresentModes, NULL);
            VK_CHECK_RESULT(result, "Vulkan Physical Device Surface Present Modes");

            devices[index].presentModes.resize(numPresentModes);

            result = vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &numPresentModes, devices[index].presentModes.data());
            VK_CHECK_RESULT(result, "Vulkan Physical Device Surface Present Modes");
            EDITOR_LOG_INFO("Number of present modes: {}", ToString(numPresentModes));

            // -------------------------------------------------------

            vkGetPhysicalDeviceMemoryProperties(physDevice, &(devices[index].memoryInfo));

            EDITOR_LOG_INFO("Number of memory types: {}", ToString(devices[index].memoryInfo.memoryTypeCount));

            for (uint32_t mem = 0; mem < devices[index].memoryInfo.memoryTypeCount; mem++)
            {
                const VkMemoryType &memoryType = devices[index].memoryInfo.memoryTypes[mem];
                EDITOR_LOG_INFO("============================================");
                EDITOR_LOG_INFO("Memory Type Index: {}", ToString(mem));
                EDITOR_LOG_INFO("Memory Heap Index: {}", ToString(memoryType.heapIndex));
                EDITOR_LOG_INFO("Memory Property Flags: {}", ToString(memoryType.propertyFlags));
                EDITOR_LOG_INFO("============================================");
            }

            EDITOR_LOG_INFO("Number of memory heaps: {}", ToString(devices[index].memoryInfo.memoryHeapCount));

            vkGetPhysicalDeviceFeatures(physDevice, &devices[index].GFXFeatures);
        }

        // After gathering all device information, pick the most suitable device
        pickPhysicalDevice(instance, surface);

        // Create logical device with the selected physical device
        createLogicalDevice();
    }

    /**
	 * @brief Select a device based on the queue type and support present.
	 * @param queueType - The type of queue to select.
	 * @param supportPresent - Whether the queue supports present.
	 * @return - The queue family index.
	 */
    uint32_t vkPhysDevice::SelectDevice(VkQueueFlags queueType, bool supportPresent)
    {
        for (uint32_t index = 0; index < devices.size(); index++)
        {
            for (uint32_t queue = 0; queue < devices[index].queueFamilyInfo.size(); queue++)
            {
                const VkQueueFamilyProperties &QueueFamily_ = devices[index].queueFamilyInfo[queue];
                if ((QueueFamily_.queueFlags & queueType) && ((bool)devices[index].queueSupportPresent[queue] == supportPresent))
                {
                    deviceIndex = index;
                    int QueueFamily_ = queue;
                    EDITOR_LOG_INFO("Using graphics device: {}", ToString(deviceIndex));
                    EDITOR_LOG_INFO("Using queue family: {}", ToString(QueueFamily_));
                    return QueueFamily_;
                }
            }
        }

        EDITOR_LOG_ERROR("No suitable device found!");
        EDITOR_LOG_ERROR("Requires graphics Queue Type: {}", ToString(queueType));
        EDITOR_LOG_ERROR("Requires graphics support present: {}", ToString(supportPresent));

        ErrMsg("No suitable device found!");

        return 0;
    }

    /**
	* @brief Get the selected device.
	* 
	* @param const GPUDevice& - The selected device.
	* @return const GPUDevice&
	*/
    const GPUDevice &vkPhysDevice::Selected() const
    {
        if (deviceIndex < 0)
        {
            EDITOR_LOG_ERROR("No device selected!");
            ErrMsg("No device selected!");
        }

        return devices[deviceIndex];
    }

	bool vkPhysDevice::isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
	    // Find the device in our device collection
	    auto deviceIt = std::find_if(devices.begin(), devices.end(), [device](const GPUDevice &d)
		{
	        return d.physicalDevice == device;
	    });
	
	    if (deviceIt == devices.end())
	    {
	        EDITOR_LOG_ERROR("Failed to find a physical device graphics in device installed");
	        return false;
	    }
	
	    // Get queue families for this device
	    QueueFamilyIndices indices = findQueueFamilies(device, surface);
	    if (!indices.isComplete())
	    {
	        EDITOR_LOG_WARN("Your Graphics Device doesn't have required queue families");
	        return false;
	    }
	
	    // Check for device extension support
	    uint32_t extensionCount = 0;
	    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	    
	    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
	    
	    // Create a set of required extension names for easier searching
	    std::set<std::string> requiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	    
	    for (const auto& extension : availableExtensions)
		{
	        requiredExtensions.erase(extension.extensionName);
	    }
	    
	    bool extensionsSupported = requiredExtensions.empty();
	    if (!extensionsSupported)
		{
	        EDITOR_LOG_WARN("Your Graphics Device doesn't support required extensions");
	        return false;
	    }
	
	    // Check for swap chain support
	    bool swapChainAdequate = false;
	    if (extensionsSupported)
	    {
	        // Get the device data from our collection
	        const GPUDevice &deviceData = *deviceIt;
	        
	        // Check if the device has sufficient swap chain support
	        bool formatsAvailable = !deviceData.surfaceFormats.empty();
	        bool presentModesAvailable = !deviceData.presentModes.empty();
	        
	        swapChainAdequate = formatsAvailable && presentModesAvailable;
	        
	        if (!swapChainAdequate)
			{
	            EDITOR_LOG_WARN("Your Graphics Device has insufficient swap chain support");
	        }
	    }
	
	    // Check for required features
	    VkPhysicalDeviceFeatures supportedFeatures;
	    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
	    
	    bool hasRequiredFeatures = supportedFeatures.samplerAnisotropy;
	    if (!hasRequiredFeatures)
		{
	        EDITOR_LOG_WARN("Your Graphics Device doesn't support required features (anisotropic sampling)");
	    }
	
	    // Device is suitable if it has complete queue families, extension support,
	    // adequate swap chain support, and required features
	    return indices.isComplete() && extensionsSupported && swapChainAdequate && hasRequiredFeatures;
	}

	bool vkPhysDevice::checkDeviceExtensionSupport(VkPhysicalDevice device)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        const auto &deviceExtensions = VulkanExtensions::GetDeviceExtensions();
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());


        EDITOR_LOG_INFO("Checking for required device extensions:");
        for (const auto &extension : deviceExtensions)
        {
            EDITOR_LOG_INFO("  Required: {}", ToString(extension));
        }
        EDITOR_LOG_INFO("Available device extensions:");
        for (const auto &extension : availableExtensions)
        {
            EDITOR_LOG_INFO("  Available: {}", ToString(extension.extensionName));
            requiredExtensions.erase(extension.extensionName);
        }
        if (!requiredExtensions.empty())
        {
            EDITOR_LOG_ERROR("Missing extensions:");
            for (const auto &ext : requiredExtensions)
            {
                EDITOR_LOG_ERROR("  Missing: {}", ToString(ext));
            }
            return false;
        }

        return true;
    }

	bool vkPhysDevice::checkValidationLayerSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        const auto &validationLayers = VulkanExtensions::GetValidationLayers();

        for (const char *layerName : validationLayers)
        {
            bool layerFound = false;

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

	bool vkPhysDevice::isDeviceCompatible(VkPhysicalDevice device)
    {
        VkPhysicalDeviceFeatures deviceFeatures;
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        // Check for required features and properties
        bool isSuitable = (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) &&
                          (deviceFeatures.geometryShader) && (deviceFeatures.tessellationShader);

        if (!isSuitable)
        {
            EDITOR_LOG_ERROR("Vulkan: Device does not meet required features or is not discrete GPU");
            ErrMsg("Vulkan: Device does not meet required features or is not discrete GPU");
            return false;
        }

        return true;
    }

	QueueFamilyIndices vkPhysDevice::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) const
    {
        QueueFamilyIndices indices;

        auto deviceIt = std::find_if(devices.begin(), devices.end(), [device](const GPUDevice &d) {
            return d.physicalDevice == device;
        });

        if (deviceIt == devices.end())
        {
            EDITOR_LOG_ERROR("Failed to find physical graphics device in device collection");
            return indices; // Return incomplete indices
        }

        const GPUDevice &deviceData = *deviceIt;

        for (uint32_t i = 0; i < deviceData.queueFamilyInfo.size(); i++)
        {
            const VkQueueFamilyProperties &queueFamily = deviceData.queueFamilyInfo[i];

            // Check for graphics support
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphicsFamily = i;
            }

            // Check presentation support - use cached data if available
            VkBool32 presentSupport =
                (i < deviceData.queueSupportPresent.size()) ? deviceData.queueSupportPresent[i] : VK_FALSE;

            if (presentSupport)
            {
                indices.presentFamily = i;
            }

            // Break early if we have all required queue families
            if (indices.isComplete())
            {
                EDITOR_LOG_INFO("Found complete queue families - Graphics: {}, Present: {}",
                                indices.graphicsFamily.value(),
                                indices.presentFamily.value());
                break;
            }
        }

        if (!indices.isComplete())
        {
            EDITOR_LOG_WARN("Could not find complete queue families for your graphics device: {}",
                            ToString(deviceData.deviceInfo.deviceName));
        }

        return indices;
    }

    VkFormat vkPhysDevice::findDepthFormat()
    {
        return findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                   VK_IMAGE_TILING_OPTIMAL,
                                   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

	VkFormat vkPhysDevice::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
    {
        for (VkFormat format : candidates)
        {
            VkFormatProperties props;
            const GPUDevice &selectedDevice = Selected();

            vkGetPhysicalDeviceFormatProperties(selectedDevice.physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            {
                return format;
            }
        }

        throw std::runtime_error("Failed to find supported format!");
    }

	/**
	 * @brief - Check if a device is suitable for rendering.
	 * 
	 * @param device - The Vulkan physical device.
	 * @return - bool True if the device is suitable, false otherwise.
	 */
    void vkPhysDevice::pickPhysicalDevice(const VkInstance &instance, const VkSurfaceKHR &surface)
    {
        this->surface = surface; // Store surface for future use
    
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0)
        {
            EDITOR_LOG_ERROR("Failed to find GPUs with Vulkan support!");
            ErrMsg("Failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

        // Find a suitable device
        for (const auto &device : physicalDevices)
        {
            if (isDeviceSuitable(device, surface))
            {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE)
        {
            EDITOR_LOG_ERROR("Failed to find a suitable GPU!");
            ErrMsg("Failed to find a suitable GPU!");
        }

        // Get the device's physical features and properties
        vkGetPhysicalDeviceFeatures(physicalDevice, &devices[deviceIndex].GFXFeatures);
        vkGetPhysicalDeviceProperties(physicalDevice, &devices[deviceIndex].deviceInfo);
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &devices[deviceIndex].memoryInfo);

        // Store queue family indices for later use
        queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

        EDITOR_LOG_INFO("Selected physical device: {}", ToString(devices[deviceIndex].deviceInfo.deviceName));
    }

    void vkPhysDevice::createLogicalDevice()
    {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

        // Handle potentially separate queues for graphics and presentation
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pEnabledFeatures = &deviceFeatures;

        if (!checkDeviceExtensionSupport(physicalDevice))
        {
            EDITOR_LOG_ERROR("Required device extensions not supported!");
            ErrMsg("Required device extensions not supported!");
        }

        // Enable swap chain extension
        const auto &deviceExtensions = VulkanExtensions::GetDeviceExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers)
        {
            const auto &validationLayers = VulkanExtensions::GetValidationLayers();
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        VkAllocationCallbacks *allocator = nullptr; // Use nullptr instead of VK_NULL_HANDLE for allocator

        if (vkCreateDevice(physicalDevice, &createInfo, allocator, &device) != VK_SUCCESS)
        {
            EDITOR_LOG_ERROR("Failed to create logical device!");
            ErrMsg("Failed to create logical device!");
        }

        // Get queue handles
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

        EDITOR_LOG_INFO("Logical device created successfully");
    }

} // namespace SceneryEditorX

// -------------------------------------------------------

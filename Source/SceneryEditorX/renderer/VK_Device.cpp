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

// -------------------------------------------------------

namespace SceneryEditorX
{
	VulkanPhysicalDevice::VulkanPhysicalDevice()
	{
	}

	VulkanPhysicalDevice::~VulkanPhysicalDevice()
	{
	}

	/**
	 * @brief Initialize the Vulkan physical device.
	 * 
	 * @param Instance_ - The Vulkan instance.
	 * @param surface - The Vulkan surface.
	 * 
	 */
    void VulkanPhysicalDevice::Init(const VkInstance &instance, const VkSurfaceKHR &surface)
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
    }

    /**
	 * @brief Select a device based on the queue type and support present.
	 * @param queueType - The type of queue to select.
	 * @param supportPresent - Whether the queue supports present.
	 * @return - The queue family index.
	 */
    uint32_t VulkanPhysicalDevice::SelectDevice(VkQueueFlags queueType, bool supportPresent)
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
    const GPUDevice &VulkanPhysicalDevice::Selected() const
    {
        if (deviceIndex < 0)
        {
            EDITOR_LOG_ERROR("No device selected!");
            ErrMsg("No device selected!");
        }

        return devices[deviceIndex];
    }

} // namespace SceneryEditorX

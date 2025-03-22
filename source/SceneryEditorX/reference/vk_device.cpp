/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_device.cpp
* -------------------------------------------------------
* Created: 17/3/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/renderer/vk_device.h>
#include <SceneryEditorX/renderer/vk_util.h>

// -------------------------------------------------------

/**
 * @brief Initialize Vulkan layers.
 */
#ifdef SEDX_DEBUG
std::vector<const char *> layers = {"VK_LAYER_KHRONOS_validation"};
#endif

/**
 * @brief Constructor of the VulkanPhysicalDevice class.
 */
VulkanDevice::VulkanDevice()
{
}

/**
 * @brief Destructor of the VulkanPhysicalDevice class.
 */
VulkanDevice::~VulkanDevice()
{
}

void VulkanDevice::initPhysicalDevice(const VkInstance &m_Instance, const VkSurfaceKHR &m_Surface)
{
    uint32_t GFXDevices_ = 0;     // Number of physical devices
    VkResult result = VK_SUCCESS; // Vulkan result
    std::vector<VkPhysicalDevice> Device;

    // -------------------------------------------------------

    result = vkEnumeratePhysicalDevices(m_Instance, &GFXDevices_, NULL);
    VK_CHECK_RESULT(result, "Vulkan Physical Devices");
    if (GFXDevices_ > 0)
    {
        Device.resize(GFXDevices_);
        result = vkEnumeratePhysicalDevices(m_Instance, &GFXDevices_, Device.data());
        if ((result != VK_SUCCESS) || (GFXDevices_ == 0))
        {
            EDITOR_LOG_ERROR("Could not enumerate physical devices.");
            VK_CHECK_RESULT(result, "Vulkan Physical Devices");
            ErrMsg("Could not enumerate physical devices.");
            return;
        }

        // Resize the Devices_ vector to accommodate the number of physical devices
        Devices_.resize(GFXDevices_);

        for (uint32_t index = 0; index < GFXDevices_; index++)
        {
            VkPhysicalDevice PhysDevice_ = Device[index];
            Devices_[index].m_PhysicalDevice = PhysDevice_;
        }
    }
    else
    {
        EDITOR_LOG_ERROR("No physical devices found.");
        VK_CHECK_RESULT(result, "Vulkan Physical Devices");
        ErrMsg("No physical devices found.");
        return;
    }

    // -------------------------------------------------------

    for (uint32_t index = 0; index < GFXDevices_; index++)
    {
        VkPhysicalDevice PhysDevice_ = Device[index];
        Devices_[index].m_PhysicalDevice = PhysDevice_;

        // -------------------------------------------------------

        vkGetPhysicalDeviceProperties(PhysDevice_, &Devices_[index].DeviceInfo_);

        DEBUG_TRACE("============================================");
        DEBUG_TRACE("Device Name: {}", ToString(Devices_[index].DeviceInfo_.deviceName));
        DEBUG_TRACE("Device Type: {}", ToString(Devices_[index].DeviceInfo_.deviceType));
        DEBUG_TRACE("Device ID: {}", ToString(Devices_[index].DeviceInfo_.deviceID));
        DEBUG_TRACE("Driver Version: {}", ToString(Devices_[index].DeviceInfo_.driverVersion));
        DEBUG_TRACE("API Version: {}", ToString(Devices_[index].DeviceInfo_.apiVersion));
        DEBUG_TRACE("Vendor ID: {}", ToString(Devices_[index].DeviceInfo_.vendorID));
        DEBUG_TRACE("============================================");

        // -------------------------------------------------------

        uint32_t NumQueueFamilies_ = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(PhysDevice_, &NumQueueFamilies_, NULL);

        // -------------------------------------------------------

        EDITOR_LOG_INFO("Number of GPU device family queues: {}", ToString(NumQueueFamilies_));

        // -------------------------------------------------------

        Devices_[index].QueueFamilyInfo_.resize(NumQueueFamilies_);
        Devices_[index].QueueSupportPresent_.resize(NumQueueFamilies_);

        vkGetPhysicalDeviceQueueFamilyProperties(PhysDevice_,
                                                 &NumQueueFamilies_,
                                                 Devices_[index].QueueFamilyInfo_.data());
        for (uint32_t queue = 0; queue < NumQueueFamilies_; queue++)
        {
            const VkQueueFamilyProperties &QueueFamilyInfo_ = Devices_[index].QueueFamilyInfo_[queue];

            EDITOR_LOG_INFO("============================================");
            EDITOR_LOG_INFO("Queue Family Index: {}", ToString(queue));
            EDITOR_LOG_INFO("Queue Count: {}", ToString(QueueFamilyInfo_.queueCount));
            EDITOR_LOG_INFO("Queue Flags: {}", ToString(QueueFamilyInfo_.queueFlags));
            EDITOR_LOG_INFO("============================================");

            result = vkGetPhysicalDeviceSurfaceSupportKHR(PhysDevice_,
                                                          queue,
                                                          m_Surface,
                                                          &Devices_[index].QueueSupportPresent_[queue]);
            VK_CHECK_RESULT(result, "Vulkan Physical Device Surface Support");
        }

        // -------------------------------------------------------

        uint32_t NumSurfaceFormats_ = 0;
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(PhysDevice_, m_Surface, &NumSurfaceFormats_, NULL);
        VK_CHECK_RESULT(result, "Vulkan Physical Device Surface Formats");

        EDITOR_LOG_INFO("Number of surface formats: {}", ToString(NumSurfaceFormats_));

        Devices_[index].SurfaceFormats_.resize(NumSurfaceFormats_);

        result = vkGetPhysicalDeviceSurfaceFormatsKHR(PhysDevice_,
                                                      m_Surface,
                                                      &NumSurfaceFormats_,
                                                      Devices_[index].SurfaceFormats_.data());
        VK_CHECK_RESULT(result, "Vulkan Physical Device Surface Formats");

        for (uint32_t format = 0; format < NumSurfaceFormats_; format++)
        {
            const VkSurfaceFormatKHR &SurfaceFormat_ = Devices_[index].SurfaceFormats_[format];
            EDITOR_LOG_INFO("============================================");
            EDITOR_LOG_INFO("Surface Format: {}", ToString(SurfaceFormat_.format));
            EDITOR_LOG_INFO("Color Space: {}", ToString(SurfaceFormat_.colorSpace));
            EDITOR_LOG_INFO("============================================");
        }

        result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysDevice_, m_Surface, &(Devices_[index].SurfaceCapabilities_));
        if (result == VK_ERROR_SURFACE_LOST_KHR)
        {
            // Handle surface lost error
            EDITOR_LOG_ERROR("Surface lost. Recreating surface...");
            // Code to recreate the surface
            // m_Surface = RecreateSurface();
            // Retry the call
            result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysDevice_, m_Surface, &(Devices_[index].SurfaceCapabilities_));
        }
        VK_CHECK_RESULT(result, "Vulkan Physical Device Surface Capabilities");

        EDITOR_LOG_INFO("============================================");
        EDITOR_LOG_INFO("Surface Capabilities");
        EDITOR_LOG_INFO("____________________________________________");
        EDITOR_LOG_INFO("Min Image Count: {}",
                            ToString(Devices_[index].SurfaceCapabilities_.minImageCount));
        EDITOR_LOG_INFO("Max Image Count: {}",
                            ToString(Devices_[index].SurfaceCapabilities_.maxImageCount));
        EDITOR_LOG_INFO("Current Extent: {}",
                            ToString(Devices_[index].SurfaceCapabilities_.currentExtent.width));
        EDITOR_LOG_INFO("Min Image Extent: {}",
                            ToString(Devices_[index].SurfaceCapabilities_.minImageExtent.width));
        EDITOR_LOG_INFO("Max Image Extent: {}",
                            ToString(Devices_[index].SurfaceCapabilities_.maxImageExtent.width));
        EDITOR_LOG_INFO("Max Image Array Layers: {}",
                            ToString(Devices_[index].SurfaceCapabilities_.maxImageArrayLayers));
        EDITOR_LOG_INFO("Supported Transforms: {}",
                            ToString(Devices_[index].SurfaceCapabilities_.supportedTransforms));
        EDITOR_LOG_INFO("Current Transform: {}",
                            ToString(Devices_[index].SurfaceCapabilities_.currentTransform));
        EDITOR_LOG_INFO("Supported Composite Alpha: {}",
                            ToString(Devices_[index].SurfaceCapabilities_.supportedCompositeAlpha));
        EDITOR_LOG_INFO("Supported Usage Flags: {}",
                            ToString(Devices_[index].SurfaceCapabilities_.supportedUsageFlags));
        EDITOR_LOG_INFO("============================================");

        // -------------------------------------------------------

        uint32_t NumPresentModes_ = 0;
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysDevice_, m_Surface, &NumPresentModes_, NULL);
        VK_CHECK_RESULT(result, "Vulkan Physical Device Surface Present Modes");

        Devices_[index].PresentModes_.resize(NumPresentModes_);

        result = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysDevice_,
                                                           m_Surface,
                                                           &NumPresentModes_,
                                                           Devices_[index].PresentModes_.data());
        VK_CHECK_RESULT(result, "Vulkan Physical Device Surface Present Modes");
        EDITOR_LOG_INFO("Number of present modes: {}", ToString(NumPresentModes_));

        // -------------------------------------------------------

        vkGetPhysicalDeviceMemoryProperties(PhysDevice_, &(Devices_[index].MemoryInfo_));

        EDITOR_LOG_INFO("Number of memory types: {}", ToString(Devices_[index].MemoryInfo_.memoryTypeCount));

        for (uint32_t mem = 0; mem < Devices_[index].MemoryInfo_.memoryTypeCount; mem++)
        {
            const VkMemoryType &MemoryType_ = Devices_[index].MemoryInfo_.memoryTypes[mem];
            EDITOR_LOG_INFO("============================================");
            EDITOR_LOG_INFO("Memory Type Index: {}", ToString(mem));
            EDITOR_LOG_INFO("Memory Heap Index: {}", ToString(MemoryType_.heapIndex));
            EDITOR_LOG_INFO("Memory Property Flags: {}", ToString(MemoryType_.propertyFlags));
            EDITOR_LOG_INFO("============================================");
        }

        EDITOR_LOG_INFO("Number of memory heaps: {}",ToString(Devices_[index].MemoryInfo_.memoryHeapCount));

        vkGetPhysicalDeviceFeatures(PhysDevice_, &Devices_[index].GFXFeatures_);
        //Devices_[index].DepthFormat_ = FindDepthFormat(PhysDevice_);
    }
}

/**
 * @brief Select a device based on the queue type and support present.
 * @param QueueType_ - The type of queue to select.
 * @param SupportPresent_ - Whether the queue supports present.
 * @return - The queue family index.
 */
uint32_t VulkanDevice::SelectDevice(VkQueueFlags QueueType, bool SupportPresent)
{
    for (uint32_t index = 0; index < Devices_.size(); index++)
    {
        for (uint32_t queue = 0; queue < Devices_[index].QueueFamilyInfo_.size(); queue++)
        {
            const VkQueueFamilyProperties &QueueFamily_ = Devices_[index].QueueFamilyInfo_[queue];
            if ((QueueFamily_.queueFlags & QueueType) &&
                ((bool)Devices_[index].QueueSupportPresent_[queue] == SupportPresent))
            {
                DeviceIndex_ = index;
                int QueueFamily_ = queue;
                EDITOR_LOG_INFO("Using graphics device: {}", ToString(DeviceIndex_));
                EDITOR_LOG_INFO("Using queue family: {}", ToString(QueueFamily_));
                return QueueFamily_;
            }
        }
    }

    EDITOR_LOG_ERROR("No suitable device found!");
    EDITOR_LOG_ERROR("Requires graphics Queue Type: {}", ToString(QueueType));
    EDITOR_LOG_ERROR("Requires graphics support present: {}", ToString(SupportPresent));

    ErrMsg("No suitable device found!");

    return 0;
}

/**
	* @brief Get the selected device.
	* 
	* @param const GPUDevice& - The selected device.
	* @return const GPUDevice&
	*/
const GPUDevice &VulkanDevice::Selected() const
{
    if (DeviceIndex_ < 0)
    {
        EDITOR_LOG_ERROR("No device selected!");
        ErrMsg("No device selected!");
    }

    return Devices_[DeviceIndex_];
}

/**
 * @brief Get the GPU properties.
 * 
 * @param VkPhysicalDeviceProperties& - The GPU properties.
 */
void VulkanDevice::GetGPUProps(VkPhysicalDeviceProperties &GPUprops)
{
    if (DeviceIndex_ < 0)
    {
        EDITOR_LOG_ERROR("No device selected!");
        ErrMsg("No device selected!");
        return;
    }

    // Copy the device properties from the selected device
    GPUprops = Devices_[DeviceIndex_].DeviceInfo_;
}

/**
 * @brief Check if validation layers are enabled.
 * 
 * @return bool - True if validation layers are enabled, false otherwise.
 */
bool VulkanDevice::isValidationLayersEnabled()
{
#ifdef SEDX_DEBUG
    const bool enableValidationLayers = true;
#else
    const bool enableValidationLayers = false;
#endif // SEDX_DEBUG

    return enableValidationLayers;
}


/**
 * @brief - Check if a device is suitable for rendering.
 * 
 * @param device - The Vulkan physical device.
 * @return - bool True if the device is suitable, false otherwise.
 */
bool VulkanDevice::IsDeviceSuitable(VkPhysicalDevice device)
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

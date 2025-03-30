/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_device.h
* -------------------------------------------------------
* Created: 21/3/2025
* -------------------------------------------------------
*/

#pragma once
#include <vulkan/vulkan.h>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX
{
    struct QueueFamilyIndices;

	struct GPUDevice
	{
        VkFormat depthFormat;
        VkPhysicalDevice physicalDevice;
        VkPhysicalDeviceFeatures GFXFeatures;
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
	    VkPhysicalDeviceProperties deviceInfo;
        VkPhysicalDeviceMemoryProperties memoryInfo;

        std::vector<VkBool32> queueSupportPresent;
        std::vector<VkPresentModeKHR> presentModes;
        std::vector<VkSurfaceFormatKHR> surfaceFormats;
	    std::vector<VkQueueFamilyProperties> queueFamilyInfo;

	    GPUDevice() : physicalDevice(VK_NULL_HANDLE), deviceInfo({}), surfaceCapabilities({}), memoryInfo({}), GFXFeatures({}), depthFormat(VK_FORMAT_UNDEFINED)
	    {
	    }
	};

	class VulkanPhysicalDevice
    {
    public:

        VulkanPhysicalDevice();
        ~VulkanPhysicalDevice();

        void Init(const VkInstance &instance, const VkSurfaceKHR &surface);

        uint32_t SelectDevice(VkQueueFlags queueType, bool supportPresent);

        const GPUDevice &Selected() const;

        bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) const;

    private:
        std::vector<GPUDevice> devices;
        int deviceIndex = -1;

    };

} // namespace SceneryEditorX

// -------------------------------------------------------

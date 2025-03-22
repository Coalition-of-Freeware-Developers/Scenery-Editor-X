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
	struct GPUDevice
	{
        VkPhysicalDevice physicalDevice;
	    VkPhysicalDeviceProperties deviceInfo;
	    std::vector<VkQueueFamilyProperties> queueFamilyInfo;
	    std::vector<VkBool32> queueSupportPresent;
	    std::vector<VkSurfaceFormatKHR> surfaceFormats;
	    VkSurfaceCapabilitiesKHR surfaceCapabilities;
	    VkPhysicalDeviceMemoryProperties memoryInfo;
	    std::vector<VkPresentModeKHR> presentModes;
	    VkPhysicalDeviceFeatures GFXFeatures;
	    VkFormat depthFormat;
	
	    GPUDevice()
	        : physicalDevice(VK_NULL_HANDLE), deviceInfo({}), surfaceCapabilities({}), memoryInfo({}), GFXFeatures({}),
	          depthFormat(VK_FORMAT_UNDEFINED)
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

    private:
        std::vector<GPUDevice> devices;

        int deviceIndex = -1;
    };

} // namespace SceneryEditorX

// -------------------------------------------------------

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
#include <SceneryEditorX/renderer/render_data.h>
#include <vector>
#include <vulkan/vulkan.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
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

        static RendererCapabilities &GetCapabilities();

    private:
        std::vector<GPUDevice> devices;
        int deviceIndex = -1;
    };

} // namespace SceneryEditorX

// -------------------------------------------------------

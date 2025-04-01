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
#include <SceneryEditorX/renderer/vk_pipeline.h>
#include <vector>
#include <vulkan/vulkan.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct QueueFamilyIndices
	{
	    std::optional<uint32_t> graphicsFamily;
	    std::optional<uint32_t> presentFamily;
	
	    bool isComplete()
	    {
	        return graphicsFamily.has_value() && presentFamily.has_value();
	    }
	};

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

	struct GPUDevice
	{
        VkFormat depthFormat;
        VkExtent2D swapChainExtent;
        VkPhysicalDevice physicalDevice;
        VkPhysicalDeviceFeatures GFXFeatures;
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
	    VkPhysicalDeviceProperties deviceInfo;
        VkPhysicalDeviceMemoryProperties memoryInfo;

        std::vector<VkBool32> queueSupportPresent;
        std::vector<VkPresentModeKHR> presentModes;
        std::vector<VkSurfaceFormatKHR> surfaceFormats;
	    std::vector<VkQueueFamilyProperties> queueFamilyInfo;

        GPUDevice() :
			physicalDevice(VK_NULL_HANDLE),
			deviceInfo({}),
			surfaceCapabilities({}),
			memoryInfo({}),
			GFXFeatures({}),
			depthFormat(VK_FORMAT_UNDEFINED),
			swapChainExtent({0, 0})
        {
        }
	};

	class vkPhysDevice
    {
    public:

	    // -------------------------------------------------------

		#ifdef SEDX_DEBUG
		        const bool enableValidationLayers = true;
		#else
		        const bool enableValidationLayers = false;
		#endif

		// -------------------------------------------------------

        vkPhysDevice();
        ~vkPhysDevice();

		// Not copyable or movable
        vkPhysDevice(vkPhysDevice &&) = delete;
        vkPhysDevice(const vkPhysDevice &) = delete;
        vkPhysDevice &operator=(vkPhysDevice &&) = delete;
        vkPhysDevice &operator=(const vkPhysDevice &) = delete;

		// -------------------------------------------------------

        void Init(const VkInstance &instance, const VkSurfaceKHR &surface);

        uint32_t SelectDevice(VkQueueFlags queueType, bool supportPresent);
        const GPUDevice &Selected() const;

		// -------------------------------------------------------

        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        bool checkValidationLayerSupport();
        bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
        bool isDeviceCompatible(VkPhysicalDevice device);
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) const;


		// -------------------------------------------------------
        // GETTER FUNCTIONS
        // -------------------------------------------------------
		VkDevice GetDevice() const { return device; }
		VkQueue GetPresentQueue() const { return presentQueue; }
		VkQueue GetGraphicsQueue() const { return graphicsQueue; }
		VkSurfaceKHR GetSurface() const { return surface; }
		VkCommandPool GetCommandPool() const { return commandPool; }
		VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
        VkFormat findDepthFormat();
		VkExtent2D GetSwapChainExtent() const
		{
			if (deviceIndex >= 0 && deviceIndex < devices.size())
			{
				return devices[deviceIndex].swapChainExtent;
			}

            return {0, 0}; // Return a default extent if no device is selected
        }

        // -------------------------------------------------------

		std::vector<bool> activeLayers;
        std::vector<bool> activeExtensions;
        std::vector<const char *> activeLayersNames;
        std::vector<const char *> activeExtensionsNames;

        std::vector<VkExtensionProperties> availableExtensions;
        std::vector<VkExtensionProperties> instanceExtensions;

    private:
        std::vector<GPUDevice> devices;
        int deviceIndex = -1;

        VkQueue graphicsQueue;
        VkQueue presentQueue;
        VkDevice device = VK_NULL_HANDLE;
        VkSurfaceKHR surface;
        VkCommandPool commandPool;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        QueueFamilyIndices queueFamilyIndices;

		// -------------------------------------------------------

        void pickPhysicalDevice(const VkInstance& instance, const VkSurfaceKHR& surface);
        void createLogicalDevice();

        VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		// -------------------------------------------------------

    };

} // namespace SceneryEditorX

// -------------------------------------------------------

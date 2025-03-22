/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_core.h
* -------------------------------------------------------
* Created: 21/3/2025
* -------------------------------------------------------
*/

#pragma once

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <optional>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX
{
	#ifdef SEDX_DEBUG
	const bool enableValidationLayers = true;
	#else
	const bool enableValidationLayers = false;
	#endif

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
	
// -------------------------------------------------------
	
	class GraphicsEngine
	{
	public:
	
		void initEngine();
	    void cleanup();
	
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
	                                                        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	                                                        void *pUserData)
	    {
	        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	
	        return VK_FALSE;
	    }
	
	private:
	
		GLFWwindow *window;
	
		VkInstance instance = VK_NULL_HANDLE;
	    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	    VkDevice device = VK_NULL_HANDLE;
	    VkAllocationCallbacks *allocator = VK_NULL_HANDLE;
	    uint32_t apiVersion;
	
		VkSampleCountFlagBits maxSamples = VK_SAMPLE_COUNT_1_BIT;
	    VkSampleCountFlags sampleCounts;
	
	    VkSurfaceKHR surface;
	    VkQueue graphicsQueue = VK_NULL_HANDLE;
	    VkQueue presentQueue = VK_NULL_HANDLE;
	
		VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	    VkFormat swapChainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
	    VkExtent2D swapChainExtent;
	
		std::vector<VkImage> swapChainImages;
	    std::vector<VkImageView> swapChainImageViews;
	
	    std::vector<bool> activeLayers;
	    std::vector<const char *> activeLayersNames;
	    std::vector<VkLayerProperties> layers;
	    std::vector<bool> activeExtensions;
	    std::vector<const char *> activeExtensionsNames;
	    std::vector<VkExtensionProperties> instanceExtensions;
	
		// -------------------------------------------------------

		std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
        std::vector<const char *> requiredExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME,
        };
        const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

		// -------------------------------------------------------

	    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger);
	    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator);
		void createDebugMessenger();
		void createSurface();
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
	    void createInstance();
	    bool isDeviceCompatible(VkPhysicalDevice device);
	    void pickPhysicalDevice();
		void createLogicalDevice();
		void createSwapChain();
		void createImageViews();
	    void createGraphicsPipeline();
	    VkShaderModule createShaderModule(const std::vector<char> &code);
	    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
	    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
	    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
	    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	    bool isDeviceSuitable(VkPhysicalDevice device);
	    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	    bool checkValidationLayerSupport();
	};

} // namespace SceneryEditorX

// -------------------------------------------------------





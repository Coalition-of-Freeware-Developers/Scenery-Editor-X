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

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <optional>
#include <SceneryEditorX/renderer/vk_device.h>
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
	
		void initEngine(GLFWwindow *window, uint32_t width, uint32_t height);
        void cleanup();
        void DestroySwapChain();
        void recreateSurfaceFormats();

		void renderFrame();

	    VkDevice GetDevice() const { return device; }

		// -------------------------------------------------------

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
	                                                        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	                                                        void *pUserData)
	    {
	        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	
	        return VK_FALSE;
	    }

		// -------------------------------------------------------

	private:
	
		GLFWwindow *window;
	
		VkInstance instance = VK_NULL_HANDLE;
	    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
	    VkAllocationCallbacks *allocator = VK_NULL_HANDLE;
	    uint32_t apiVersion;

		// -------------------------------------------------------

		VulkanPhysicalDevice physDeviceManager;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;

		VkPhysicalDeviceFeatures physicalFeatures{};
        VkPhysicalDeviceProperties physicalProperties{};
        VkPhysicalDeviceMemoryProperties memoryProperties{};

		// -------------------------------------------------------

		VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT;
	    VkSampleCountFlags sampleCounts;

		bool framebufferResized = false;
	    VkSurfaceKHR surface;
	    VkQueue graphicsQueue = VK_NULL_HANDLE;
	    VkQueue presentQueue = VK_NULL_HANDLE;
	
		VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	    VkFormat swapChainImageFormat;
	    VkExtent2D swapChainExtent;

		// -------------------------------------------------------

	    std::vector<bool> activeLayers;
        std::vector<bool> activeExtensions;
	    std::vector<const char *> activeLayersNames;
        std::vector<const char *> activeExtensionsNames;
		std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
        std::vector<const char *> requiredExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME,
        };
        const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

        // -------------------------------------------------------

        std::vector<VkImage> swapChainImages;
        std::vector<VkFence> inFlightFences;
        std::vector<VkImageView> swapChainImageViews;
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFramebuffer> swapChainFramebuffers;
		std::vector<VkCommandBuffer> commandBuffers;
        std::vector<VkPresentModeKHR> availablePresentModes;
        std::vector<VkLayerProperties> layers;
        std::vector<VkSurfaceFormatKHR> availableSurfaceFormats;
        std::vector<VkExtensionProperties> availableExtensions;
        std::vector<VkExtensionProperties> instanceExtensions;
        std::vector<VkQueueFamilyProperties> availableFamilies;

        VkSurfaceCapabilitiesKHR surfaceCapabilities{};

		// -------------------------------------------------------

		uint32_t currentFrame = 0;
		uint32_t additionalImages = 0;
        uint32_t framesInFlight = 3;

		VkPipeline graphicsPipeline;
		VkRenderPass renderPass;
		VkCommandPool commandPool;
        VkPipelineLayout pipelineLayout;

		// -------------------------------------------------------

	    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator);
        void createInstance();
		void createDebugMessenger();
        void createSurface(GLFWwindow *glfwWindow);
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
	    bool isDeviceCompatible(VkPhysicalDevice device);
        void createSwapChain();
	    void pickPhysicalDevice();
		void createLogicalDevice();
        void createImageViews();
        void createRenderPass();
        void createGraphicsPipeline();
        void createFramebuffers();
        void createCommandPool();
        void createCommandBuffers();
        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        void createSyncObjects();
        void recreateSwapChain();


		// -------------------------------------------------------

		VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger);
        VkShaderModule createShaderModule(const std::vector<char> &code);
	    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
	    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
	    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

		// -------------------------------------------------------

	    bool isDeviceSuitable(VkPhysicalDevice device);
	    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        bool checkValidationLayerSupport();

	};

} // namespace SceneryEditorX

// -------------------------------------------------------





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
#include <functional>
#include <GLFW/glfw3.h>
#include <iostream>
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
	
	struct Vertex
	{
	    glm::vec3 pos;
	    glm::vec3 color;
	    glm::vec2 texCoord;
	
	    static VkVertexInputBindingDescription getBindingDescription()
	    {
	        VkVertexInputBindingDescription bindingDescription{};
	        bindingDescription.binding = 0;
	        bindingDescription.stride = sizeof(Vertex);
	        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	
	        return bindingDescription;
	    }
	
	    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
	    {
	        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
	
	        attributeDescriptions[0].binding = 0;
	        attributeDescriptions[0].location = 0;
	        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	        attributeDescriptions[0].offset = offsetof(Vertex, pos);
	
	        attributeDescriptions[1].binding = 0;
	        attributeDescriptions[1].location = 1;
	        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	        attributeDescriptions[1].offset = offsetof(Vertex, color);
	
	        attributeDescriptions[2].binding = 0;
	        attributeDescriptions[2].location = 2;
	        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
	
	        return attributeDescriptions;
	    }
	
	    bool operator==(const Vertex &other) const
	    {
	        return pos == other.pos && color == other.color && texCoord == other.texCoord;
	    }
	};

} // namespace SceneryEditorX

// -------------------------------------------------------

namespace std
{
	template <>
	struct std::hash<SceneryEditorX::Vertex>
	{
	    size_t operator()(SceneryEditorX::Vertex const &vertex) const
	    {
	        return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
	    }
	};

} // namespace std

// -------------------------------------------------------

namespace SceneryEditorX
{
    struct UniformBufferObject
    {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };
	
// -------------------------------------------------------
	
	class GraphicsEngine
	{
	public:
	
		void initEngine(GLFWwindow *window, uint32_t width, uint32_t height);
        void createSwapChain();
        void cleanUp();
        void cleanupSwapChain();
        void recreateSwapChain();
        void renderFrame();
        void updateUniformBuffer(uint32_t currentImage);
        void createTextureImage();
        void createTextureImageView();
        void createTextureSampler();
        void loadModel();
        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);
        void recreateSurfaceFormats();
        void generateMipmaps(VkImage image,
                             VkFormat imageFormat,
                             int32_t texWidth,
                             int32_t texHeight,
                             uint32_t mipLevels);

        LOCAL void framebufferResizeCallback(GLFWwindow *window, int width, int height);

        VkCommandBuffer beginSingleTimeCommands();
	    VkDevice GetDevice() const { return device; }
        VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        void createImage(uint32_t width,
                         uint32_t height,
                         uint32_t mipLevels,
                         VkSampleCountFlagBits numSamples,
                         VkFormat format,
                         VkImageTiling tiling,
                         VkImageUsageFlags usage,
                         VkMemoryPropertyFlags properties,
                         VkImage &image,
                         VkDeviceMemory &imageMemory);

		// -------------------------------------------------------
        // GETTER FUNCTIONS
        // -------------------------------------------------------

		VkInstance GetInstance() const { return instance; }
		VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
		VkQueue GetGraphicsQueue() const { return graphicsQueue; }
		VkQueue GetPresentQueue() const { return presentQueue; }
		VkRenderPass GetRenderPass() const { return renderPass; }
		VkDescriptorPool GetDescriptorPool() const { return descriptorPool; }
		VkExtent2D GetSwapChainExtent() const { return swapChainExtent; }
		VkSampleCountFlagBits GetMsaaSamples() const { return msaaSamples; }
		VkFormat GetSwapChainImageFormat() const { return swapChainImageFormat; }
		VkDeviceMemory GetTextureImageMemory() const { return textureImageMemory; }
		VkImage GetTextureImage() const { return textureImage; }
		VkImageView GetTextureImageView() const { return textureImageView; }
		VkSampler GetTextureSampler() const { return textureSampler; }
		VkDeviceMemory GetDepthImageMemory() const { return depthImageMemory; }
		VkImageView GetDepthImageView() const { return depthImageView; }
		VkImage GetDepthImage() const { return depthImage; }
		VkDescriptorSetLayout GetDescriptorSetLayout() const { return descriptorSetLayout; }
		VkPipelineLayout GetPipelineLayout() const { return pipelineLayout; }
		VkPipeline GetGraphicsPipeline() const { return graphicsPipeline; }
		VkCommandPool GetCommandPool() const { return commandPool; }
		VkDeviceMemory GetIndexBufferMemory() const { return indexBufferMemory; }
		VkBuffer GetIndexBuffer() const { return indexBuffer; }
		VkDeviceMemory GetVertexBufferMemory() const { return vertexBufferMemory; }
		VkBuffer GetVertexBuffer() const { return vertexBuffer; }
		VkDeviceMemory GetUniformBufferMemory(size_t index) const { return uniformBuffersMemory[index]; }
		VkBuffer GetUniformBuffer(size_t index) const { return uniformBuffers[index]; }
		VkDescriptorSet GetDescriptorSet(size_t index) const { return descriptorSets[index]; }
		VkFramebuffer GetSwapChainFramebuffer(size_t index) const { return swapChainFramebuffers[index]; }
		VkImageView GetSwapChainImageView(size_t index) const { return swapChainImageViews[index]; }
		VkSemaphore GetRenderFinishedSemaphore(size_t index) const { return renderFinishedSemaphores[index]; }
		VkSemaphore GetImageAvailableSemaphore(size_t index) const { return imageAvailableSemaphores[index]; }
        QueueFamilyIndices GetQueueFamilyIndices() const { return queueFamilyIndices; }
		const std::vector<VkImage>& GetSwapChainImages() const { return swapChainImages; }

		// -------------------------------------------------------

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
	    {
	        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	
	        return VK_FALSE;
	    }

		// -------------------------------------------------------

	private:
		GLFWwindow *window;
		VkInstance instance = VK_NULL_HANDLE;
	    VkAllocationCallbacks *allocator = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
        //VmaAllocator vmaAllocator;

		// -------------------------------------------------------

        VkDevice device = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VulkanPhysicalDevice physDeviceManager;

		// -------------------------------------------------------

		VkPhysicalDeviceFeatures physicalFeatures{};
        VkPhysicalDeviceProperties physicalProperties{};
        VkPhysicalDeviceMemoryProperties memoryProperties{};

		// -------------------------------------------------------

		VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue = VK_NULL_HANDLE;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
	    VkSurfaceKHR surface;
		VkSwapchainKHR swapChain = VK_NULL_HANDLE;
        QueueFamilyIndices queueFamilyIndices;
        VkSampleCountFlags sampleCounts;
        VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

		// -------------------------------------------------------

	    std::vector<bool> activeLayers;
        std::vector<bool> activeExtensions;
	    std::vector<const char *> activeLayersNames;
        std::vector<const char *> activeExtensionsNames;
		std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
        std::vector<const char *> requiredExtensions =
		{
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
			VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME
        };
        const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

        // -------------------------------------------------------

		std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<VkImage> swapChainImages;
        std::vector<VkFence> inFlightFences;
        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkImageView> swapChainImageViews;
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFramebuffer> swapChainFramebuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
        std::vector<VkDescriptorSet> descriptorSets;
		std::vector<VkCommandBuffer> commandBuffers;
        std::vector<VkPresentModeKHR> availablePresentModes;
        std::vector<VkLayerProperties> layers;
        std::vector<VkSurfaceFormatKHR> availableSurfaceFormats;
        std::vector<VkExtensionProperties> availableExtensions;
        std::vector<VkExtensionProperties> instanceExtensions;
        std::vector<VkQueueFamilyProperties> availableFamilies;

        VkSurfaceCapabilitiesKHR surfaceCapabilities{};

		// -------------------------------------------------------

		uint32_t mipLevels;
        uint32_t apiVersion;
		uint32_t currentFrame = 0;
		uint32_t additionalImages = 0;
        uint32_t framesInFlight = 3;
        uint32_t swapChainCurrentFrame = 0;

		VkPipeline graphicsPipeline;
		VkRenderPass renderPass;
		VkCommandPool commandPool;
        VkPipelineLayout pipelineLayout;

		VkBuffer indexBuffer;
		VkBuffer vertexBuffer;
        VkDeviceMemory indexBufferMemory;
        VkDeviceMemory vertexBufferMemory;
        VkDescriptorPool descriptorPool;
        VkDescriptorSetLayout descriptorSetLayout;

		// -------------------------------------------------------

		VkImage textureImage;
        VkSampler textureSampler;
        VkImageView textureImageView;
        VkDeviceMemory textureImageMemory;

		// -------------------------------------------------------

		VkImage depthImage;
        VkImageView depthImageView;
        VkDeviceMemory depthImageMemory;

        // -------------------------------------------------------

		VkImage colorImage;
        VkDeviceMemory colorImageMemory;
        VkImageView colorImageView;

		// -------------------------------------------------------

        bool framebufferResized = false;
        bool isDeviceSuitable(VkPhysicalDevice device);
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        bool checkValidationLayerSupport();
        bool hasStencilComponent(VkFormat format);

		// -------------------------------------------------------

	    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator);
        void createInstance();
		void createDebugMessenger();
        void createSurface(GLFWwindow *glfwWindow);
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
	    bool isDeviceCompatible(VkPhysicalDevice device);
	    void pickPhysicalDevice();
		void createLogicalDevice();
        void createImageViews();
        //void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format,  VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory);
        void createRenderPass();
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        void createVertexBuffer();
        void createColorResources();

        void createDepthResources();
        void createIndexBuffer();
        void createUniformBuffers();
        void createDescriptorPool();
        void createDescriptorSetLayout();
        void createDescriptorSets();
        void createGraphicsPipeline();
        void createFramebuffers();
        void createCommandPool();
        void createCommandBuffers();
        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        void createSyncObjects();

		// -------------------------------------------------------

		VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger);
        VkShaderModule createShaderModule(const std::vector<char> &code);
	    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
	    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
	    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
        VkSampleCountFlagBits getMaxUsableSampleCount();

		// -------------------------------------------------------

        VkFormat findDepthFormat();
	    VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		// -------------------------------------------------------

	};

} // namespace SceneryEditorX

// -------------------------------------------------------





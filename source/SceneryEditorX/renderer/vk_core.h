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
#include <SceneryEditorX/core/window.h>
#include <SceneryEditorX/renderer/render_data.h>
#include <SceneryEditorX/renderer/vk_allocator.h>
#include <SceneryEditorX/renderer/vk_checks.h>
#include <SceneryEditorX/renderer/vk_device.h>
#include <SceneryEditorX/renderer/vk_swapchain.h>
#include <SceneryEditorX/scene/model.h>

// -------------------------------------------------------

namespace std
{
	template <>
	struct std::hash<SceneryEditorX::Vertex>
	{
	    size_t operator()(const SceneryEditorX::Vertex &vertex) const noexcept
        {
	        return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
	    }
	};

} // namespace std

// -------------------------------------------------------

struct GLFWwindow;

namespace SceneryEditorX
{
	/// Forward declarations
	class SwapChain;
	struct SwapChainDetails;

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
        GraphicsEngine();
        virtual ~GraphicsEngine();

        virtual void Init(const Ref<Window> &window);
        virtual void CreateInstance(const Ref<Window> &window);
        VkRenderPass GetRenderPass() const { return renderPass;}
        Ref<Window> GetWindow() { return editorWindow; }
		Ref<SwapChain> GetSwapChain() { return vkSwapChain; }
        LOCAL Ref<VulkanDevice> GetDevice() { return VulkanDevice::GetInstance(); }
		LOCAL VkInstance GetInstance() { return vkInstance; }
        VkSampler CreateSampler(float maxLod);
        VkSampler GetSampler() { return vkDevice->GetSampler(); }
        void WaitIdle(const Ref<VulkanDevice> &device);

        //INTERNAL Ref<ShaderLibrary> GetShaderLibrary();
		//INTERNAL void WaitAndRender(RenderThread *renderThread);
        //INTERNAL void SwapQueues();

		// -------------------------------------------------------

		uint32_t GetRenderQueueIndex();
        uint32_t GetRenderQueueSubmissionIndex();
        uint32_t GetCurrentFrameIndex();

        VkCommandBuffer BeginSingleTimeCommands() const;
        void EndSingleTimeCommands(VkCommandBuffer commandBuffer) const;
        void Submit();

		// -------------------------------------------------------

		//static void BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<RenderPass> renderPass, bool explicitClear = false);
		//static void EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer);

		// -------------------------------------------------------

	    void BeginFrame();
		void EndFrame();

		// -------------------------------------------------------

		/*
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
	    {
	        std::cerr << "validation layer: " << pCallbackData->pMessage << '\n';
	
	        return VK_FALSE;
	    }
		*/

		// -------------------------------------------------------

	private:
        Ref<Window> editorWindow;
        Ref<SwapChain> vkSwapChain;
        Ref<VulkanDevice> vkDevice;
        LOCAL inline VkInstance vkInstance;

	    VkAllocationCallbacks *allocator = nullptr;
        VkDebugUtilsMessengerEXT debugMessenger = nullptr;
        VkPipelineCache pipelineCache = nullptr;

		Layers vkLayers;
		Viewport viewportData;
        RenderData renderData;
        Extensions vkExtensions;
        VulkanDeviceFeatures vkEnabledFeatures;

        Ref<VulkanChecks> checks;
        Ref<MemoryAllocator> allocatorManager;
        Ref<VulkanPhysicalDevice> vkPhysicalDevice;

        void glfwSetWindowUserPointer(const Ref<Window> &window, GLFWwindow *pointer);

		// -------------------------------------------------------

		VkDevice device = nullptr;
        VkPhysicalDevice vkPhysDevice = nullptr;
        
		// -------------------------------------------------------

		VkQueue graphicsQueue = nullptr;
        VkQueue presentQueue = nullptr;

	    VkSurfaceKHR surface;
        QueueFamilyIndices queueFamilyIndices;

        // -------------------------------------------------------

		std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
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
        std::vector<VkSurfaceFormatKHR> availableSurfaceFormats;
        std::vector<VkQueueFamilyProperties> availableFamilies;

		// -------------------------------------------------------

		VkPipeline graphicsPipeline;
		VkRenderPass renderPass;
		VkCommandPool cmdPool;
        VkPipelineLayout pipelineLayout;

		// -------------------------------------------------------

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

        INTERNAL void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator);
        void CreateSurface(GLFWwindow *glfwWindow);
        void CreateLogicalDevice();
        void CreateSwapChain();
        void CreateImageViews();
        VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
        void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples,
                         VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                         VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory);
        void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
        GLOBAL void FramebufferResizeCallback(GLFWwindow *window, int width, int height);
        void CreateRenderPass();
        void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory) const;
        void CreateVertexBuffer();
        void CreateDepthResources();
        void CreateIndexBuffer();
        void CreateUniformBuffers();
        void CreateDescriptorPool();
        void CreateDescriptorSetLayout();
        void CreateDescriptorSets();
        void CreateGraphicsPipeline();
        void CreateFramebuffers();
        void CreateCommandPool();
        void CreateCommandBuffers();
        void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        void CreateSyncObjects();
        void RecreateSwapChain();
        void RenderFrame();
        void UpdateUniformBuffer(uint32_t currentImage) const;
        void RecreateSurfaceFormats();
        void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
        void CreateColorResources();
        VkSampleCountFlagBits GetMaxUsableSampleCount() const;
        void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        void LoadModel();
        void CreateTextureImage();
        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
        void CreateTextureImageView();
        void CreateTextureSampler();
        VkShaderModule CreateShaderModule(const std::vector<char> &code);
        SwapChainDetails QuerySwapChainSupport(VkPhysicalDevice device);
        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
        VkFormat FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
        VkFormat FindDepthFormat() const;
        void CleanUp();
        void CleanupSwapChain();
        INTERNAL void PopulateDebugMsgCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

		// -------------------------------------------------------

		VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger);
        VkShaderModule createShaderModule(const std::vector<char> &code);
        VkSampleCountFlagBits GetMaxUsableSampleCount();

		// -------------------------------------------------------

	};

} // namespace SceneryEditorX

// -------------------------------------------------------





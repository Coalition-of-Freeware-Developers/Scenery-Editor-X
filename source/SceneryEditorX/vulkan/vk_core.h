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
#include <SceneryEditorX/core/window.h>
#include <SceneryEditorX/vulkan/render_data.h>
#include <SceneryEditorX/vulkan/vk_allocator.h>
#include <SceneryEditorX/vulkan/vk_buffers.h>
#include <SceneryEditorX/vulkan/vk_checks.h>
#include <SceneryEditorX/vulkan/vk_device.h>
#include <SceneryEditorX/vulkan/vk_swapchain.h>

/// -------------------------------------------------------

/*
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
*/

/// -------------------------------------------------------

struct GLFWwindow;

namespace SceneryEditorX
{
	/// Forward declarations
	class SwapChain;
	struct SwapChainDetails;
	
    /// -------------------------------------------------------
	
	class GraphicsEngine
    {
	public:
        GraphicsEngine();
        virtual ~GraphicsEngine();
        virtual void Init(const Ref<Window> &window);
        virtual void CreateInstance(const Ref<Window> &window);

        Ref<Window> GetWindow() { return editorWindow; }
		Ref<SwapChain> GetSwapChain() { return vkSwapChain; }
		Ref<VulkanDevice> GetLogicDevice() { return vkDevice; }
		LOCAL Ref<GraphicsEngine> Get() { return {}; }
		LOCAL Ref<VulkanDevice> GetCurrentDevice() { return Get()->GetLogicDevice();}
		LOCAL VkInstance GetInstance() { return vkInstance; }
	    [[nodiscard]] VkRenderPass GetRenderPass() const { return renderPass;}
        [[nodiscard]] VkSampler CreateSampler(float maxLod);
        [[nodiscard]] VkSampler GetSampler() const { return vkDevice->GetSampler(); }
        [[nodiscard]] const VkAllocationCallbacks *GetAllocatorCallback() const { return allocator; }

	    void WaitIdle(const Ref<VulkanDevice> &device);

	    //INTERNAL Ref<ShaderLibrary> GetShaderLibrary();
		//INTERNAL void WaitAndRender(RenderThread *renderThread);
        //INTERNAL void SwapQueues();

		/// -------------------------------------------------------

		uint32_t GetRenderQueueIndex();
        uint32_t GetRenderQueueSubmissionIndex();
        uint32_t GetCurrentFrameIndex();

        [[nodiscard]] VkCommandBuffer BeginSingleTimeCommands() const;
        void EndSingleTimeCommands(VkCommandBuffer commandBuffer) const;
        void Submit();

		/// -------------------------------------------------------

		//static void BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<RenderPass> renderPass, bool explicitClear = false);
		//static void EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer);

		/// -------------------------------------------------------

	    void BeginFrame();
		void EndFrame();

		/// -------------------------------------------------------

	private:
        Ref<Window> editorWindow;
        Ref<SwapChain> vkSwapChain;
        Ref<VulkanDevice> vkDevice;
        Ref<VulkanPhysicalDevice> vkPhysicalDevice;
        Ref<UniformBuffer> uniformBuffer;
        Ref<VulkanChecks> checks;
        Ref<MemoryAllocator> allocatorManager;

        LOCAL inline VkInstance vkInstance;

	    VkAllocationCallbacks *allocator = nullptr;
        VkDebugUtilsMessengerEXT debugMessenger = nullptr;
        VkPipelineCache pipelineCache = nullptr;

		Layers vkLayers;
		Viewport viewportData;
        RenderData renderData;
        Extensions vkExtensions;
        VulkanDeviceFeatures vkEnabledFeatures;

        void glfwSetWindowUserPointer(const Ref<Window> &window, GLFWwindow *pointer);

	    friend struct ImageResource;
		/// -------------------------------------------------------

		VkDevice device = nullptr;
        VkPhysicalDevice vkPhysDevice;

        /// -------------------------------------------------------

        std::vector<VkFence> inFlightFences;
        std::vector<VkImageView> swapChainImageViews;
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFramebuffer> swapChainFramebuffers;
        std::vector<VkDescriptorSet> descriptorSets;
		std::vector<VkCommandBuffer> commandBuffers;
        std::vector<VkPresentModeKHR> availablePresentModes;
        std::vector<VkSurfaceFormatKHR> availableSurfaceFormats;
        std::vector<VkQueueFamilyProperties> availableFamilies;

		/// -------------------------------------------------------

	    VkSurfaceKHR surface;
		VkPipeline graphicsPipeline;
		VkRenderPass renderPass;
        VkPipelineLayout pipelineLayout;

		/// -------------------------------------------------------

        VkDescriptorPool descriptorPool;
        VkDescriptorSetLayout descriptorSetLayout;

		/// -------------------------------------------------------

		VkImage textureImage;
        VkSampler textureSampler;
        VkImageView textureImageView;
        VkDeviceMemory textureImageMemory;

		/// -------------------------------------------------------

		VkImage depthImage;
        VkImageView depthImageView;
        VkDeviceMemory depthImageMemory;

        /// -------------------------------------------------------

		VkImage colorImage;
        VkDeviceMemory colorImageMemory;
        VkImageView colorImageView;

		/// -------------------------------------------------------

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
        void CreateVertexBuffer();
        void CreateDepthResources();
        void CreateIndexBuffer();
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
        void RecreateSurfaceFormats();
        void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) const;
        void CreateColorResources();
        VkSampleCountFlagBits GetMaxUsableSampleCount() const;
        void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
        void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
        void CreateTextureImage();
        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
        void CreateTextureImageView();
        void CreateTextureSampler();
        SwapChainDetails QuerySwapChainSupport(VkPhysicalDevice device) const;
	    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
        VkFormat FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
        VkFormat FindDepthFormat() const;
        void CleanUp();
        void CleanupSwapChain();
        VkSampleCountFlagBits GetMaxUsableSampleCount();

		/// -------------------------------------------------------
	};

} // namespace SceneryEditorX

/// -------------------------------------------------------

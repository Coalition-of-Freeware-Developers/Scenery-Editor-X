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
#include <GraphicsEngine/vulkan/render_data.h>
#include <GraphicsEngine/vulkan/vk_allocator.h>
#include <GraphicsEngine/vulkan/vk_checks.h>
#include <GraphicsEngine/vulkan/vk_cmd_buffers.h>
#include <GraphicsEngine/vulkan/vk_device.h>
#include <SceneryEditorX/core/window.h>

/// -------------------------------------------------------

struct GLFWwindow;

namespace SceneryEditorX
{
	class GraphicsEngine : public RefCounted
    {
	public:
        GraphicsEngine();
        virtual ~GraphicsEngine() override;

        virtual void Init(const Ref<Window> &window);
        //virtual void CreateInstance(const Ref<Window> &window);

        Ref<Window> GetWindow() { return editorWindow; }

        MemoryAllocator &GetMemAllocator() { return allocatorManager; }
		CommandBuffer GetCommandBuffer() { return cmdBufferManger; }

		LOCAL Ref<GraphicsEngine> Get(); ///< Static accessor method to get the singleton instance
        GLOBAL void WaitIdle(const Ref<VulkanDevice> &device);

		/// -------------------------------------------------------

	private:
        INTERNAL Ref<GraphicsEngine> gfxContext;
        
        Ref<Window> editorWindow;
        Ref<VulkanChecks> checks;
        MemoryAllocator allocatorManager;
        CommandBuffer cmdBufferManger;

	    VkAllocationCallbacks *allocator = nullptr;
		Viewport viewportData;
        RenderData renderData;

        LOCAL void glfwSetWindowUserPointer(const Ref<Window> &window, GLFWwindow *pointer);

	    friend struct ImageResource;

		/// -------------------------------------------------------

        uint32_t apiVersion;

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
        void CleanUp();
        GLOBAL void FramebufferResizeCallback(GLFWwindow *window, int width, int height);

        //void CreateSurface(GLFWwindow *glfwWindow);
        //void CreateSwapChain();
        //void CreateImageViews();
        //VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const;
        //void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) const;
        //void CreateDepthResources();
        //void CreateDescriptorPool();
        //void CreateDescriptorSetLayout();
        //void CreateFramebuffers();
        //void CreateSyncObjects();
        //void RecreateSwapChain();
        //void RenderFrame();
        //VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
        //VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
        //VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
        //void CreateTextureImageView();
        //void CreateTextureSampler();
        //INTERNAL SwapChainDetails QuerySwapChainSupport(VkPhysicalDevice device);
        //VkFormat FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
        //VkFormat FindDepthFormat() const;

        //void CleanupSwapChain();
        //VkSampleCountFlagBits GetMaxUsableSampleCount() const;

		/// -------------------------------------------------------
	};

} // namespace SceneryEditorX

/// -------------------------------------------------------

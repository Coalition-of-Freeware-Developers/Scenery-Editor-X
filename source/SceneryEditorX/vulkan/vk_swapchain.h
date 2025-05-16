/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_swapchain.h
* -------------------------------------------------------
* Created: 9/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/vulkan/render_data.h>
#include <SceneryEditorX/vulkan/vk_pipelines.h>

/// -------------------------------------------------------

struct GLFWwindow;

/// -------------------------------------------------------

namespace SceneryEditorX
{

    class Pipeline;

    /// -------------------------------------------------------

	struct SwapChainDetails
	{
	    VkSurfaceCapabilitiesKHR capabilities;
	    std::vector<VkSurfaceFormatKHR> formats;
	    std::vector<VkPresentModeKHR> presentModes;
	};

	/// -------------------------------------------------------

	struct SwapchainCommandBuffer
    {
        VkCommandPool CommandPool = nullptr;
        VkCommandBuffer CommandBuffer = nullptr;
    };

    /// -------------------------------------------------------

	class SwapChain
	{
    public:
        SwapChain() = default;
        ~SwapChain() = default;

		/// Initialization methods
		void Init(VkInstance instance, const Ref<VulkanDevice> &device);
        void InitSurface(const Ref<Window> &window);
        void Create(uint32_t width, uint32_t height, bool vsync);
		void OnResize(uint32_t width, uint32_t height);
        void Destroy();

		/// Getter methods
        [[nodiscard]] uint32_t GetWidth() const { return renderData.width; }
        [[nodiscard]] uint32_t GetHeight() const { return renderData.height; }
        [[nodiscard]] uint32_t GetImageIndex() const { return SceneryEditorX::RenderData::imageIndex; }
		[[nodiscard]] VkFormat GetColorFormat() const { return colorFormat; }
        [[nodiscard]] VkFormat GetDepthFormat() const { return depthFormat; }
		[[nodiscard]] VkRenderPass GetRenderPass() const { return renderPass; }
        [[nodiscard]] VkExtent2D GetSwapExtent() const { return swapChainExtent; }
        [[nodiscard]] Viewport GetViewport() const { return viewportData.GetViewportSize(); }
		[[nodiscard]] VkAttachmentDescription GetColorAttachment() const { return colorAttachment; }
        [[nodiscard]] VkAttachmentDescription GetDepthAttachment() const { return depthAttachment; }
        [[nodiscard]] VkSwapchainKHR GetSwapchain() const { return swapChain; }

		/// Setter methods
		void SetVSync(const bool enabled) { renderData.VSync = enabled; }

		/// Image/view utility methods
		VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const;
		void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
						 VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory) const;

	private:
        RenderData renderData;
        Viewport viewportData;
        VkInstance instance = nullptr;
        Ref<VulkanDevice> device;
        Ref<Pipeline> pipeline;
        //Ref<SwapChainDetails> swapChainDetails;

		/// Helper methods
        uint32_t AcquireNextImage();
        void CreateImageViews();
        void FindImageFormatAndColorSpace();
        void CreateDepthResources();
        [[nodiscard]] uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
        [[nodiscard]] VkFormat FindDepthFormat() const;
        [[nodiscard]] VkFormat FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

		/// Selection methods
        LOCAL VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
	    LOCAL VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, uint32_t width, uint32_t height);
        LOCAL SwapChainDetails QuerySwapChainSupport(const VulkanDevice &device);
        VkPresentModeKHR ChooseSwapPresentMode(uint32_t presentModeCount) const;

		/// Vulkan resources - derived after device is initialized
	    //VkDevice vkDevice = device->GetDevice();
        //VkPhysicalDevice vkPhysDevice = device->GetPhysicalDevice()->GetGPUDevice();
        VkDevice vkDevice = nullptr;
        VkPhysicalDevice vkPhysDevice = nullptr;
        uint32_t queueIndex = UINT32_MAX;
        uint32_t swapChainImageCount = 0;

		/// -------------------------------------------------------

		// Swapped to the 'image_data.h' file Image Struct.

		/*
		struct SwapchainImage
		{
			VkImage Image = nullptr;
			VkImageView ImageView = nullptr;
		};
		*/

		/// -------------------------------------------------------

        /// Format and attachment data
        VkFormat colorFormat;
        VkFormat depthFormat;
        VkExtent2D swapChainExtent;
        VkColorSpaceKHR colorSpace;
        VkSampleCountFlags sampleCounts;
        VkAttachmentDescription colorAttachment{};
        VkAttachmentDescription depthAttachment{};

		/// -------------------------------------------------------

		/// Core swapchain objects
        VkSurfaceKHR surface = nullptr;
        VkSwapchainKHR swapChain = nullptr;
        VkRenderPass renderPass = nullptr;
        VkAllocationCallbacks *allocator = nullptr;

		/// Image resources
        std::vector<Image> swapChainImages;
        std::vector<VkImage> swapChainImageResources;
        std::vector<VkImageView> swapChainViews;
        std::vector<VkFramebuffer> swapChainFramebuffers;
        std::vector<SwapchainCommandBuffer> cmdBuffers;

	    /// Semaphores to signal that images are available for rendering and that rendering has finished (one pair for each frame in flight)
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;

        /// Fences to signal that command buffers are ready to be reused (one for each frame in flight)
		std::vector<VkFence> waitFences;

		/// -------------------------------------------------------

        VkImage textureImage = nullptr;
        VkSampler textureSampler = nullptr;
        VkImageView textureImageView = nullptr;
        VkDeviceMemory textureImageMemory = nullptr;

        /// -------------------------------------------------------

        VkImage depthImage = nullptr;
        VkImageView depthImageView = nullptr;
        VkDeviceMemory depthImageMemory = nullptr;

        /// -------------------------------------------------------

        VkImage colorImage = nullptr;
        VkDeviceMemory colorImageMemory = nullptr;
        VkImageView colorImageView = nullptr;

        /// -------------------------------------------------------

		friend class GraphicsEngine;
    };



} // namespace SceneryEditorX

/// -------------------------------------------------------


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
#include <SceneryEditorX/renderer/vulkan/image_data.h>
#include <SceneryEditorX/renderer/vulkan/vk_pipelines.h>

/// -------------------------------------------------------

struct GLFWwindow;

/// -------------------------------------------------------

namespace SceneryEditorX
{
    class Pipeline;
    class GraphicsEngine;

    /// -------------------------------------------------------

	struct SwapChainDetails
	{
        VkSurfaceCapabilitiesKHR capabilities{};
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

	class SwapChain : public RefCounted
	{
    public:
        SwapChain() = default;

		/// Initialization methods
		void Init(VkInstance instance, const Ref<VulkanDevice> &device);
        void InitSurface();
        void Create(uint32_t width, uint32_t height, bool vsync);
        void OnResize(uint32_t width, uint32_t height);
        void Destroy();

		/// Getter methods
		[[nodiscard]] VkFormat GetColorFormat() const { return colorFormat; }
        [[nodiscard]] VkFormat GetDepthFormat() const { return depthFormat; }
        [[nodiscard]] VkExtent2D GetSwapExtent() const { return swapChainExtent; }
	    [[nodiscard]] VkSwapchainKHR GetSwapchain() const { return swapChain; }
	    [[nodiscard]] VkRenderPass GetRenderPass() const { return renderPass; }
		[[nodiscard]] VkAttachmentDescription GetColorAttachment() const { return colorAttachment; }
        [[nodiscard]] VkAttachmentDescription GetDepthAttachment() const { return depthAttachment; }
        
        /**
         * @brief Gets the texture image view for use in descriptor sets
         * @return VkImageView The texture image view handle or VK_NULL_HANDLE if not available
         */
        [[nodiscard]] VkImageView GetTextureImageView() const { return textureImageView; }
        
        /**
         * @brief Gets the texture sampler for use in descriptor sets
         * @return VkSampler The texture sampler handle or VK_NULL_HANDLE if not available
         */
        [[nodiscard]] VkSampler GetTextureSampler() const { return textureSampler; }
        
        /**
         * @brief Gets the depth image view for use in descriptor sets or framebuffers
         * @return VkImageView The depth image view handle or VK_NULL_HANDLE if not available
         */
        [[nodiscard]] VkImageView GetDepthImageView() const { return depthImageView; }

		/// Image/view utility methods
		VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const;
		void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
						 VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory) const;

	private:
        VkInstance instance = nullptr;
        Ref<VulkanDevice> vkDevice;
        Ref<GraphicsEngine> *gfxEngine;
        Ref<Pipeline> *pipeline;

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
        [[nodiscard]] VkPresentModeKHR ChooseSwapPresentMode(uint32_t presentModeCount) const;

		/// Vulkan resources - derived after device is initialized
        uint32_t queueIndex = UINT32_MAX;
        uint32_t swapChainImageCount = 0;

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


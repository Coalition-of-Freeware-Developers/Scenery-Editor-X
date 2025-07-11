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
#include <SceneryEditorX/renderer/image_data.h>
//#include <SceneryEditorX/renderer/vulkan/vk_pipeline.h>

/// -------------------------------------------------------

struct GLFWwindow;

/// -------------------------------------------------------

namespace SceneryEditorX
{
	struct SwapChainDetails
	{
        VkSurfaceCapabilitiesKHR capabilities{};
	    std::vector<VkSurfaceFormatKHR> formats;
	    std::vector<VkPresentModeKHR> presentModes;
	};

	/// -------------------------------------------------------

	class SwapChain
	{
    public:
        SwapChain() = default;

		/// Initialization methods
		void Init(VkInstance instance, const Ref<VulkanDevice> &device);
        void InitSurface(GLFWwindow *windowPtr);
        void Create(uint32_t *width, uint32_t *height, bool vsync);
        void OnResize(uint32_t width, uint32_t height);
        void Present();
        void Destroy();

		void BeginFrame();

		/// Getter methods
		[[nodiscard]] VkFormat GetColorFormat() const { return colorFormat; }
        [[nodiscard]] VkFormat GetDepthFormat() const { return depthFormat; }
        [[nodiscard]] VkExtent2D GetSwapExtent() const { return swapChainExtent; }
	    [[nodiscard]] VkRenderPass GetRenderPass() const { return renderPass; }
	    [[nodiscard]] VkSwapchainKHR GetSwapchain() const { return swapChain; }

	    [[nodiscard]] VkFramebuffer GetActiveFramebuffer() const { return GetFramebuffer(currentImageIdx); }
        [[nodiscard]] VkCommandBuffer GetActiveDrawCommandBuffer() const { return GetDrawCommandBuffer(currentFrameIdx); }
		[[nodiscard]] VkAttachmentDescription GetColorAttachment() const { return colorAttachment; }
        [[nodiscard]] VkAttachmentDescription GetDepthAttachment() const { return depthAttachment; }

        [[nodiscard]] uint32_t GetWidth() const { return swapWidth; }
        [[nodiscard]] uint32_t GetHeight() const { return swapHeight; }

		VkFramebuffer GetFramebuffer(uint32_t index) const
        {
            SEDX_CORE_ASSERT(index < swapChainFramebuffers.size());
            return swapChainFramebuffers[index];
		}

		VkCommandBuffer GetDrawCommandBuffer(uint32_t index) const
		{
			SEDX_CORE_ASSERT(index < cmdBuffers.size());
			return cmdBuffers[index].CommandBuffer;
        }

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

		uint32_t GetSwapChainImageCount() const { return swapChainImageCount; }
		uint32_t GetBufferIndex() const { return currentFrameIdx; }

		/// Image/view utility methods
		VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const;
		void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
						 VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory) const;
        uint32_t GetCurrentBufferIndex() const { return currentFrameIdx; };

    private:
        VkInstance instance = nullptr;
        Ref<VulkanDevice> vkDevice;

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
        uint32_t swapChainImageCount	= 0; /// Number of images in the swapchain
        uint32_t currentFrameIdx		= 0; /// Current frame index for swapchain operations
        uint32_t currentImageIdx		= 0; /// Current image index for swapchain operations
        uint32_t swapWidth				= 0; /// Width of the swapchain
        uint32_t swapHeight				= 0; /// Height of the swapchain
        bool VSync = false;

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
	    struct SwapchainCommandBuffer
        {
            VkCommandPool CommandPool = nullptr;
            VkCommandBuffer CommandBuffer = nullptr;
        };
        std::vector<SwapchainCommandBuffer> cmdBuffers;

	    struct SwapchainImage
        {
            VkImage Image = nullptr;
            VkImageView ImageView = nullptr;
        };
        std::vector<SwapchainImage> swapChainImage;

		/// Image resources
        //std::vector<Image> swapChainImages;
        std::vector<VkImage> swapChainImageCounts;
        std::vector<VkImageView> swapChainViews;
        std::vector<VkFramebuffer> swapChainFramebuffers;

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

		friend class RenderContext;
    };

} // namespace SceneryEditorX

/// -------------------------------------------------------


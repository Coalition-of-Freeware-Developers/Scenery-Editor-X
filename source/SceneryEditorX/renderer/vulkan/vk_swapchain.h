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
#include <SceneryEditorX/renderer/command_manager.h>
#include <xMath/includes/vec2.h>
#include "SceneryEditorX/renderer/image_data.h"

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
	    ~SwapChain();

		// Initialization methods
		void Init(VkInstance instance, const Ref<VulkanDevice> &device);
        void InitSurface(GLFWwindow *windowPtr);
        void Recreate(uint32_t *width, uint32_t *height, bool vsync);

        void Create(uint32_t* width, uint32_t* height, bool vsync);
        void OnResize(uint32_t width, uint32_t height);
        void AcquireNextImage();
        void Present();
        void Destroy();
		void BeginFrame();

        /// -------------------------------------------------------

		// Getter methods
		[[nodiscard]] VkFormat GetColorFormat() const { return colorFormat; }
        [[nodiscard]] VkFormat GetDepthFormat() const { return depthFormat; }
        [[nodiscard]] VkExtent2D GetSwapExtent() const { return swapChainExtent; }
	    [[nodiscard]] VkRenderPass GetRenderPass() const { return renderPass; }
	    [[nodiscard]] VkSwapchainKHR GetSwapchain() const { return swapChain; }
	    [[nodiscard]] VkFramebuffer GetActiveFramebuffer() const { return GetFramebuffer(currentImageIdx); }
        [[nodiscard]] VkCommandBuffer GetActiveDrawCommandBuffer() const { return GetDrawCommandBuffer(currentFrameIdx); }
		[[nodiscard]] VkAttachmentDescription GetColorAttachment() const { return colorAttachment; }
        [[nodiscard]] VkAttachmentDescription GetDepthAttachment() const { return depthAttachment; }

        /// -------------------------------------------------------

        [[nodiscard]] uint32_t GetWidth() const { return swapWidth; }
        [[nodiscard]] uint32_t GetHeight() const { return swapHeight; }
	    [[nodiscard]] Vec2 GetDimensions() const { return {swapWidth, swapHeight}; }

        /// -------------------------------------------------------

	    [[nodiscard]] VkImage GetActiveImage() const;
        [[nodiscard]] VkImage GetSwapchainImage(uint32_t index) const;
        [[nodiscard]] VkFramebuffer GetFramebuffer(uint32_t index) const;
        [[nodiscard]] VkCommandBuffer GetDrawCommandBuffer(uint32_t index) const;

        [[nodiscard]] VkImageView GetTextureImageView() const { return textureImageView; }
        [[nodiscard]] VkSampler GetTextureSampler() const { return textureSampler; }
        [[nodiscard]] VkImageView GetDepthImageView() const { return depthImageView; }

        [[nodiscard]] uint32_t GetSwapChainImageCount() const { return swapChainImageCount; }
        [[nodiscard]] uint32_t GetBufferIndex() const { return currentFrameIdx; }

		// Image/view utility methods
		VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const;
		void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
						 VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory) const;
        [[nodiscard]] uint32_t GetCurrentBufferIndex() const { return currentFrameIdx; }

    private:
        Ref<VulkanDevice> vkDevice;

		// Helper methods
        void CreateImageViews();
        void FindImageFormatAndColorSpace();
        void CreateDepthResources();
        [[nodiscard]] uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
        [[nodiscard]] VkFormat FindDepthFormat() const;
        [[nodiscard]] VkFormat FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

		// Selection methods
        static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
	    static VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, uint32_t width, uint32_t height);
        static SwapChainDetails QuerySwapChainSupport(const VulkanDevice *device);
        [[nodiscard]] VkPresentModeKHR ChooseSwapPresentMode() const;

		// Vulkan resources - derived after device is initialized
        uint32_t queueIndex = UINT32_MAX;
        uint32_t swapChainImageCount	= 0; // Number of images in the swapchain
        uint32_t currentFrameIdx		= 0; // Current frame index for swapchain operations
        uint32_t currentImageIdx		= 0; // Current image index for swapchain operations
        uint32_t swapWidth				= 0; // Width of the swapchain
        uint32_t swapHeight				= 0; // Height of the swapchain
        bool VSync = false;

		/// -------------------------------------------------------
        /// Format and attachment data
        /// -------------------------------------------------------

        VkFormat colorFormat;
        VkFormat depthFormat;
        VkExtent2D swapChainExtent;
        VkColorSpaceKHR colorSpace;
        VkSampleCountFlags sampleCounts;
        VkAttachmentDescription colorAttachment{};
        VkAttachmentDescription depthAttachment{};

		/// -------------------------------------------------------
		/// Core swapchain objects
        /// -------------------------------------------------------

        VkSurfaceKHR surface = nullptr;		// Window surface
        VkSwapchainKHR swapChain = nullptr; // Swapchain object
        VkRenderPass renderPass = nullptr;  // Render pass object

        /// -------------------------------------------------------

	    struct SwapchainCommandBuffer
        {
            VkCommandPool CommandPool = nullptr;
            VkCommandBuffer CommandBuffer = nullptr;
        };
        std::vector<SwapchainCommandBuffer> cmdBuffers;

        /// -------------------------------------------------------

	    struct SwapchainImage
        {
            VkImage Image = nullptr;
            VkImageView ImageView = nullptr;
        };
        std::vector<SwapchainImage> swapChainImage;

        /// -------------------------------------------------------

		// Image resources
        //std::vector<Image> swapChainImages;
        std::vector<VkImage> swapChainImageCounts;			// Raw VkImage handles
        std::vector<VkImageView> swapChainViews;			// Image views for the swapchain images
        std::vector<VkFramebuffer> swapChainFramebuffers;	// Framebuffers for each swapchain image

	    // Semaphores to signal that images are available for rendering and that rendering has finished (one pair for each frame in flight)
        // TODO: Replace with the Semaphore class
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;

        // Fences to signal that command buffers are ready to be reused (one for each frame in flight)
		// TODO: Replace with the Fence class
		std::vector<VkFence> waitFences;

		/// -------------------------------------------------------

        VkImage textureImage = nullptr;
        VkSampler textureSampler = nullptr;
        VkImageView textureImageView = nullptr;
        VkDeviceMemory textureImageMemory = nullptr;

        /// -------------------------------------------------------

	    // For depth buffering
        VkImage depthImage = nullptr;
        VkImageView depthImageView = nullptr;
        VkDeviceMemory depthImageMemory = nullptr;

        /// -------------------------------------------------------

	    // For multisampling
        VkImage colorImage = nullptr;
        VkDeviceMemory colorImageMemory = nullptr;
        VkImageView colorImageView = nullptr;

        /// -------------------------------------------------------

		friend class RenderContext;
    };

}

/// -------------------------------------------------------


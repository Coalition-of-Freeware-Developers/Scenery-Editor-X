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

// -------------------------------------------------------

struct GLFWwindow;

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct SwapChainDetails
	{
        VkSurfaceCapabilitiesKHR capabilities{};
	    std::vector<VkSurfaceFormatKHR> formats;
	    std::vector<VkPresentModeKHR> presentModes;
	};

	// -------------------------------------------------------

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

        // -------------------------------------------------------

		// Getter methods
		[[nodiscard]] VkFormat GetColorFormat() const { return m_colorFormat; }
        [[nodiscard]] VkFormat GetDepthFormat() const { return m_depthFormat; }
        [[nodiscard]] VkExtent2D GetSwapExtent() const { return m_swapChainExtent; }
	    [[nodiscard]] VkRenderPass GetRenderPass() const { return m_renderPass; }
	    [[nodiscard]] VkSwapchainKHR GetSwapchain() const { return m_swapChain; }
	    [[nodiscard]] VkFramebuffer GetActiveFramebuffer() const { return GetFramebuffer(m_currentImageIdx); }
        [[nodiscard]] VkCommandBuffer GetActiveDrawCommandBuffer() const { return GetDrawCommandBuffer(m_currentFrameIdx); }
		[[nodiscard]] VkAttachmentDescription GetColorAttachment() const { return m_colorAttachment; }
        [[nodiscard]] VkAttachmentDescription GetDepthAttachment() const { return m_depthAttachment; }

        // -------------------------------------------------------

        [[nodiscard]] uint32_t GetWidth() const { return m_swapWidth; }
        [[nodiscard]] uint32_t GetHeight() const { return m_swapHeight; }
	    [[nodiscard]] Vec2 GetDimensions() const { return {m_swapWidth, m_swapHeight}; }

        // -------------------------------------------------------

	    [[nodiscard]] VkImage GetActiveImage() const;
        [[nodiscard]] VkImage GetSwapchainImage(uint32_t index) const;
        [[nodiscard]] VkFramebuffer GetFramebuffer(uint32_t index) const;
        [[nodiscard]] VkCommandBuffer GetDrawCommandBuffer(uint32_t index) const;

        [[nodiscard]] VkImageView GetTextureImageView() const { return m_textureImageView; }
        [[nodiscard]] VkSampler GetTextureSampler() const { return m_textureSampler; }
        [[nodiscard]] VkImageView GetDepthImageView() const { return m_depthImageView; }

        [[nodiscard]] uint32_t GetSwapChainImageCount() const { return m_swapChainImageCount; }
        [[nodiscard]] uint32_t GetBufferIndex() const { return m_currentFrameIdx; }

		// Image/view utility methods
		VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const;
		void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
						 VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory) const;
        [[nodiscard]] uint32_t GetCurrentBufferIndex() const { return m_currentFrameIdx; }

    private:
        Ref<VulkanDevice> m_vkDevice;

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
        uint32_t m_queueIndex = UINT32_MAX;
        uint32_t m_swapChainImageCount		= 0; // Number of images in the swapchain
        uint32_t m_currentFrameIdx			= 0; // Current frame index for swapchain operations
        uint32_t m_currentImageIdx			= 0; // Current image index for swapchain operations
        uint32_t m_swapWidth				= 0; // Width of the swapchain
        uint32_t m_swapHeight				= 0; // Height of the swapchain
        bool m_vSync = false;

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Format and attachment data
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        VkFormat m_colorFormat;
        VkFormat m_depthFormat;
        VkExtent2D m_swapChainExtent;
        VkColorSpaceKHR m_colorSpace;
        VkSampleCountFlags m_sampleCounts;
        VkAttachmentDescription m_colorAttachment{};
        VkAttachmentDescription m_depthAttachment{};

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Core swapchain objects
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        VkSurfaceKHR m_surface = nullptr;		// Window surface
        VkSwapchainKHR m_swapChain = nullptr;	// Swapchain object
        VkRenderPass m_renderPass = nullptr;	// Render pass object

        // -------------------------------------------------------

	    struct SwapchainCommandBuffer
        {
            VkCommandPool m_CommandPool = nullptr;
            VkCommandBuffer m_CommandBuffer = nullptr;
        };
        std::vector<SwapchainCommandBuffer> m_cmdBuffers;

        // -------------------------------------------------------

	    struct SwapchainImage
        {
            VkImage m_Image = nullptr;
            VkImageView m_ImageView = nullptr;
        };
        std::vector<SwapchainImage> m_swapChainImage;

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Image resources
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        //std::vector<Image> swapChainImages;
        std::vector<VkImage> m_swapChainImageCounts;			// Raw VkImage handles
        std::vector<VkImageView> m_swapChainViews;			// Image views for the swapchain images
        std::vector<VkFramebuffer> m_swapChainFramebuffers;	// Framebuffers for each swapchain image

	    // Semaphores to signal that images are available for rendering and that rendering has finished (one pair for each frame in flight)
        // TODO: Replace with the Semaphore class
        std::vector<VkSemaphore> m_imageAvailableSemaphores;
        std::vector<VkSemaphore> m_renderFinishedSemaphores;

        // Fences to signal that command buffers are ready to be reused (one for each frame in flight)
		// TODO: Replace with the Fence class
		std::vector<VkFence> m_waitFences;

		// -------------------------------------------------------

        VkImage m_textureImage = nullptr;
        VkSampler m_textureSampler = nullptr;
        VkImageView m_textureImageView = nullptr;
        VkDeviceMemory m_textureImageMemory = nullptr;

        // -------------------------------------------------------

	    // For depth buffering
        VkImage m_depthImage = nullptr;
        VkImageView m_depthImageView = nullptr;
        VkDeviceMemory m_depthImageMemory = nullptr;

        // -------------------------------------------------------

	    // For multisampling
        VkImage m_colorImage = nullptr;
        VkDeviceMemory m_colorImageMemory = nullptr;
        VkImageView m_colorImageView = nullptr;

        // -------------------------------------------------------

		friend class RenderContext;
    };

}

// -------------------------------------------------------


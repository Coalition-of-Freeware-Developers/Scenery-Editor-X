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
#include "SceneryEditorX/core/window/window.h"
#include "SceneryEditorX/renderer/render_config.h"
#include <SceneryEditorX/renderer/command_manager.h>
#include <xMath/includes/vec2.h>

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
		void Init(const Ref<VulkanDevice> &device);
        void InitSurface(GLFWwindow *windowPtr);
        void Recreate(uint32_t *width, uint32_t *height, bool vsync);
        void Create();
        void OnResize(uint32_t width, uint32_t height);
        void AcquireNextImage();
        void Present();
        void Destroy();
		void BeginFrame();

        // -------------------------------------------------------

		// Getter methods
		[[nodiscard]] VkFormat GetColorFormat() const { return m_Format; }
        [[nodiscard]] VkFormat GetDepthFormat() const { return m_DepthFormat; }
        [[nodiscard]] VkExtent2D GetSwapExtent() const { return m_SwapChainExtent; }
	    [[nodiscard]] VkRenderPass GetRenderPass() const { return m_RenderPass; }
	    [[nodiscard]] VkSwapchainKHR GetSwapchain() const { return m_SwapChain; }
	    [[nodiscard]] VkFramebuffer GetActiveFramebuffer() const { return GetFramebuffer(m_ImageIdx); }
        [[nodiscard]] VkCommandBuffer GetActiveDrawCommandBuffer() const { return GetDrawCommandBuffer(m_CurrentFrameIdx); }
		[[nodiscard]] VkAttachmentDescription GetColorAttachment() const { return m_ColorAttachment; }
        [[nodiscard]] VkAttachmentDescription GetDepthAttachment() const { return m_DepthAttachment; }
		[[nodiscard]] VkSurfaceKHR GetSurface() const { return m_Surface; }

        // -------------------------------------------------------

        [[nodiscard]] uint32_t GetWidth() const { return m_Width; }
        [[nodiscard]] uint32_t GetHeight() const { return m_Height; }
	    [[nodiscard]] Vec2 GetDimensions() const { return {m_Width, m_Height}; }

        // -------------------------------------------------------

	    [[nodiscard]] VkImage GetActiveImage() const;
        [[nodiscard]] VkImage GetSwapchainImage(uint32_t index) const;
        [[nodiscard]] VkFramebuffer GetFramebuffer(uint32_t index) const;
        [[nodiscard]] VkCommandBuffer GetDrawCommandBuffer(uint32_t index) const;

        [[nodiscard]] VkImageView GetTextureImageView() const { return m_TextureImageView; }
        [[nodiscard]] VkSampler GetTextureSampler() const { return m_TextureSampler; }
        [[nodiscard]] VkImageView GetDepthImageView() const { return m_DepthImageView; }

        [[nodiscard]] uint32_t GetSwapChainImageCount() const { return m_SwapChainImageCount; }
        [[nodiscard]] uint32_t GetBufferIndex() const { return m_CurrentFrameIdx; }

		// Image/view utility methods
		VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const;
		void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
						 VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory) const;
        [[nodiscard]] uint32_t GetCurrentBufferIndex() const { return m_CurrentFrameIdx; }

		static constexpr VkFormat FORMAT_SDR = VK_FORMAT_B8G8R8A8_SRGB;
        static constexpr VkFormat FORMAT_HDR = VK_FORMAT_A2R10G10B10_UNORM_PACK32;

    private:
        Ref<VulkanDevice> m_Device;
        Window m_Window;

		// Helper methods
        void CreateImageViews();
        void FindImageFormatAndColorSpace();
        void CreateDepthResources();
        [[nodiscard]] static uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        [[nodiscard]] VkFormat FindDepthFormat() const;
        [[nodiscard]] VkFormat FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

		// Selection methods
        static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
	    static VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, uint32_t width, uint32_t height);
        static SwapChainDetails QuerySwapChainSupport(const VulkanDevice *device);
        [[nodiscard]] VkPresentModeKHR ChooseSwapPresentMode() const;

		// Vulkan resources - derived after device is initialized
        uint32_t m_QueueIndex = UINT32_MAX;
        uint32_t m_SwapChainImageCount		= 0; // Number of images in the swapchain
        uint32_t m_CurrentFrameIdx			= 0; // Current frame index for swapchain operations
        uint32_t m_ImageIdx					= 0; // Current image index for swapchain operations
        uint32_t m_SemaphoreIdx				= 0; // Current semaphore index for frame synchronization
        uint32_t m_Width					= 0; // Width of the swapchain
        uint32_t m_Height					= 0; // Height of the swapchain
        bool m_VSync = false;

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Format and attachment data																					  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        VkFormat m_Format;
        VkFormat m_DepthFormat;
        VkExtent2D m_SwapChainExtent;
        VkColorSpaceKHR m_ColorSpace;
        VkSampleCountFlags m_SampleCounts;
        VkAttachmentDescription m_ColorAttachment{};
        VkAttachmentDescription m_DepthAttachment{};
        VkPresentModeKHR m_PresentMode = VK_PRESENT_MODE_FIFO_KHR;

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Core swapchain objects
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        VkSurfaceKHR m_Surface = nullptr;		// Window surface
        VkSwapchainKHR m_SwapChain = nullptr;	// Swapchain object
        VkRenderPass m_RenderPass = nullptr;	// Render pass object

        // -------------------------------------------------------

	    struct SwapchainCommandBuffer
        {
            VkCommandPool m_CommandPool = nullptr;
            VkCommandBuffer m_CommandBuffer = nullptr;
        };
        std::vector<SwapchainCommandBuffer> m_CmdBuffers;

        // -------------------------------------------------------

	    struct SwapchainImage
        {
            VkImage m_Image = nullptr;
            VkImageView m_ImageView = nullptr;
        };
        std::vector<SwapchainImage> m_SwapChainImage;

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Image resources
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        //std::vector<Image> swapChainImages;
        std::vector<VkImage> m_SwapChainImageCounts;		// Raw VkImage handles
        std::vector<VkImageView> m_SwapChainViews;			// Image views for the swapchain images
        std::vector<VkFramebuffer> m_SwapChainFramebuffers;	// Framebuffers for each swapchain image

	    // Semaphores to signal that images are available for rendering and that rendering has finished (one pair for each frame in flight)
        // TODO: Replace with the Semaphore class
        //std::array<Ref<FrameSync>, FRAMES_IN_FLIGHT> m_ImageAcquiredSemaphores;
        //std::array<Ref<FrameSync>, FRAMES_IN_FLIGHT> m_RenderFinishedSemaphores;

        // Fences to signal that command buffers are ready to be reused (one for each frame in flight)
		// TODO: Replace with the Fence class
		std::vector<VkFence> m_WaitFences;

		// -------------------------------------------------------

        VkImage m_TextureImage = nullptr;
        VkSampler m_TextureSampler = nullptr;
        VkImageView m_TextureImageView = nullptr;
        VkDeviceMemory m_TextureImageMemory = nullptr;

        // -------------------------------------------------------

	    // For depth buffering
        VkImage m_DepthImage = nullptr;
        VkImageView m_DepthImageView = nullptr;
        VkDeviceMemory m_DepthImageMemory = nullptr;

        // -------------------------------------------------------

	    // For multisampling
        VkImage m_ColorImage = nullptr;
        VkDeviceMemory m_ColorImageMemory = nullptr;
        VkImageView m_ColorImageView = nullptr;

        // -------------------------------------------------------

		friend class RenderContext;
    };

}

// -------------------------------------------------------


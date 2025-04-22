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

#include <SceneryEditorX/renderer/render_data.h>
#include <SceneryEditorX/renderer/vk_core.h>
#include <SceneryEditorX/renderer/vk_device.h>
#include <SceneryEditorX/renderer/vk_pipelines.h>

// -------------------------------------------------------

struct GLFWwindow;

// -------------------------------------------------------

namespace SceneryEditorX
{

    class Pipeline;

    // -------------------------------------------------------

	struct SwapChainDetails
	{
	    VkSurfaceCapabilitiesKHR capabilities;
	    std::vector<VkSurfaceFormatKHR> formats;
	    std::vector<VkPresentModeKHR> presentModes;
	};

	// -------------------------------------------------------

    struct Command
    {
        void *stagingCpu;
        VkFence fence;
        VkBuffer staging;
        VkQueryPool queryPool;
        VkCommandPool pool;
        std::vector<Command> commands;
    };

	class SwapChain
	{
    public:
        SwapChain() = default;

		void Init(VkInstance instance, const Ref<VulkanDevice> &device);
        void InitSurface(const Ref<Window> &window);
        void Create(uint32_t width, uint32_t height, bool vsync);
        void Destroy();

		void OnResize(uint32_t width, uint32_t height);
		//GLOBAL bool GetSwapChainDirty() { return swapChainDirty; }
		uint32_t GetWidth() const { return renderData.width; }
		uint32_t GetHeight() const { return renderData.height; }
        GLOBAL uint32_t GetImageIndex() { return RenderData::imageIndex; }
		VkFormat GetColorFormat() const { return colorFormat; }
        VkExtent2D GetSwapExtent() const { return swapChainExtent; }
        std::vector<VkImage> GetSwapChainImages() const { return swapChainImages; }
		void SetVSync(const bool enabled) { renderData.VSync = enabled; }
		VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const;
		void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
						 VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory) const;

	private:
        RenderData renderData;
        Viewport viewportData;
        Ref<Pipeline> pipeline;
        Ref<SwapChainDetails> swapChainDetails;
        uint32_t AcquireNextImage();
        void CreateImageViews();
        void FindImageFormatAndColorSpace();
        LOCAL VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
        void CreateDepthResources();
        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) const;
        VkFormat FindDepthFormat();
        VkFormat FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        LOCAL VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, uint32_t width, uint32_t height);
        LOCAL SwapChainDetails QuerySwapChainSupport(const VulkanDevice &device);
        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

		VkDevice vkDevice = device->GetDevice();
        VkPhysicalDevice vkPhysDevice = device->GetPhysicalDevice()->GetGPUDevice();

		// -------------------------------------------------------

		struct SwapchainImage
		{
			VkImage Image = nullptr;
			VkImageView ImageView = nullptr;
		};

		// -------------------------------------------------------

        VkFormat colorFormat;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        VkRenderPass renderPass = nullptr;
        VkSurfaceKHR surface;
        VkSwapchainKHR swapChain = nullptr;
        VkColorSpaceKHR colorSpace;
        VkSampleCountFlags sampleCounts;
        VkAllocationCallbacks *allocator = VK_NULL_HANDLE;

		// -------------------------------------------------------
        //GraphicsEngine;
        Ref<VulkanDevice> device;
        std::vector<VkImage> images;
        std::vector<VkImage> swapChainImages;
        std::vector<VkImage> swapImgResources;
        std::vector<VkFence> waitFences;
        std::vector<VkImageView> swapChainViews;
        std::vector<SwapchainImage> swapImages;
        std::vector<VkFramebuffer> swapChainFramebuffers;
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;

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

		friend class GraphicsEngine;
    };



} // namespace SceneryEditorX

// -------------------------------------------------------


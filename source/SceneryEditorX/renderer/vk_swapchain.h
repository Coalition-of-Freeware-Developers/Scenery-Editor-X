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

#include <SceneryEditorX/renderer/vk_device.h>
#include <SceneryEditorX/renderer/render_data.h>

// -------------------------------------------------------

struct GLFWwindow;

// -------------------------------------------------------

namespace SceneryEditorX
{

	struct SwapChainSupportDetails
	{
	    VkSurfaceCapabilitiesKHR capabilities;
	    std::vector<VkSurfaceFormatKHR> formats;
	    std::vector<VkPresentModeKHR> presentModes;
	};

	// -------------------------------------------------------

    /*struct Queue
    {
        QueueFamilyIndices;

        struct Command
        {
            VkCommandPool pool;
            VkFence fence;
            VkQueryPool queryPool;
            VkBuffer staging;
            void* stagingCpu;
        };

        std::vector<Command> commands;
    };*/

	class SwapChain
	{
    public:
        SwapChain() = default;

		void Init(VkInstance instance, const Ref<VulkanDevice> &device);
        void InitSurface(GLFWwindow* window);
        void Create(uint32_t* width, uint32_t* height, bool vsync);
        void Destroy();

		void OnResize(uint32_t width, uint32_t height);

		uint32_t GetWidth() const { return gfxData.width; }
		uint32_t GetHeight() const { return gfxData.height; }
		uint32_t GetImageIndex() const { return gfxData.imageIndex; }

		VkFormat GetColorFormat() const { return colorFormat; }
		void SetVSync(const bool enabled) { gfxData.VSync = enabled; }

	private:
        RenderData gfxData;
        uint32_t AcquireNextImage();
        void FindImageFormatAndColorSpace();
        static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
        static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
        static VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
        static SwapChainSupportDetails QuerySwapChainSupport(const VulkanDevice &device);

		struct SwapchainImage
		{
			VkImage Image = nullptr;
			VkImageView ImageView = nullptr;
		};

		// -------------------------------------------------------

        VkFormat colorFormat;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        VkInstance instance = nullptr;
        VkRenderPass renderPass = nullptr;
        VkSurfaceKHR surface;
        VkSwapchainKHR swapChain = nullptr;
        VkColorSpaceKHR colorSpace;
        VkSampleCountFlags sampleCounts;
        VkAllocationCallbacks *allocator = VK_NULL_HANDLE;


		// -------------------------------------------------------

        Ref<VulkanDevice> device;
        std::vector<VkImage> images;
        std::vector<VkImage> swapChainImages;
        std::vector<VkImage> swapImgResources;
        std::vector<VkImageView> swapChainViews;
        std::vector<SwapchainImage> swapImages;
        std::vector<VkFramebuffer> swapChainFramebuffers;
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;

		// -------------------------------------------------------

		friend class GraphicsEngine;
    };



} // namespace SceneryEditorX

// -------------------------------------------------------


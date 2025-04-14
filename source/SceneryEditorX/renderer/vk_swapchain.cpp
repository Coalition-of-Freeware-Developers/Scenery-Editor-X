/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_swapchain.cpp
* -------------------------------------------------------
* Created: 9/4/2025
* -------------------------------------------------------
*/
#include <GLFW/glfw3.h>
#include <SceneryEditorX/core/base.hpp>
#include <SceneryEditorX/core/window.h>
#include <SceneryEditorX/renderer/render_data.h> 
#include <SceneryEditorX/renderer/vk_device.h>
#include <SceneryEditorX/renderer/vk_queue.h>
#include <SceneryEditorX/renderer/vk_swapchain.h>
#include <utility>

// -------------------------------------------------------

// Macro to get a procedure address based on a vulkan instance
#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                                                                       \
    {                                                                                                                  \
        fp##entrypoint = reinterpret_cast<PFN_vk##entrypoint>(vkGetInstanceProcAddr(inst, "vk" #entrypoint));          \
        SEDX_CORE_ASSERT(fp##entrypoint);                                                                              \
    }

// Macro to get a procedure address based on a vulkan device
#define GET_DEVICE_PROC_ADDR(dev, entrypoint)                                                                          \
    {                                                                                                                  \
        fp##entrypoint = reinterpret_cast<PFN_vk##entrypoint>(vkGetDeviceProcAddr(dev, "vk" #entrypoint));             \
        SEDX_CORE_ASSERT(fp##entrypoint);                                                                              \
    }

// -------------------------------------------------------

static PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
static PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
static PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
static PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR;
static PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
static PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
static PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
static PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
static PFN_vkQueuePresentKHR fpQueuePresentKHR;

// Nvidia extensions
PFN_vkCmdSetCheckpointNV fpCmdSetCheckpointNV;
PFN_vkGetQueueCheckpointDataNV fpGetQueueCheckpointDataNV;

// AMD extensions
/* Add AMD Specific extensions later when added */

// -------------------------------------------------------

VKAPI_ATTR void VKAPI_CALL vkCmdSetCheckpointNV(VkCommandBuffer commandBuffer, const void *pCheckpointMarker)
{
    fpCmdSetCheckpointNV(commandBuffer, pCheckpointMarker);
}

VKAPI_ATTR void VKAPI_CALL vkGetQueueCheckpointDataNV(VkQueue queue, uint32_t *pCheckpointDataCount, VkCheckpointDataNV *pCheckpointData)
{
    fpGetQueueCheckpointDataNV(queue, pCheckpointDataCount, pCheckpointData);
}

// -------------------------------------------------------

namespace SceneryEditorX
{
    void SwapChain::Init(VkInstance instance, const Ref<VulkanDevice> &device)
    {
        this->instance = instance;
        this->device = device;

        VkDevice vulkanDevice = device->GetDevice();
        GET_DEVICE_PROC_ADDR(vulkanDevice, CreateSwapchainKHR);
        GET_DEVICE_PROC_ADDR(vulkanDevice, DestroySwapchainKHR);
        GET_DEVICE_PROC_ADDR(vulkanDevice, GetSwapchainImagesKHR);
        GET_DEVICE_PROC_ADDR(vulkanDevice, AcquireNextImageKHR);
        GET_DEVICE_PROC_ADDR(vulkanDevice, QueuePresentKHR);

        GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfaceSupportKHR);
        GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfaceCapabilitiesKHR);
        GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfaceFormatsKHR);
        GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfacePresentModesKHR);

        GET_INSTANCE_PROC_ADDR(instance, CmdSetCheckpointNV);
        GET_INSTANCE_PROC_ADDR(instance, GetQueueCheckpointDataNV);
    }

    // -------------------------------------------------------

    void SwapChain::InitSurface(GLFWwindow *window)
    {
        VkPhysicalDevice physicalDevice = device->GetPhysicalDevice()->GetGPUDevice();
        GPUDevice gpuData;
        // Create the surface
        if (glfwCreateWindowSurface(instance, window, allocator, &surface) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to create window surface!");
            ErrMsg("Failed to create window surface!");
        }

        // Create our swap chain using the capabilities from the selected device
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;

        // Set minimum image count (usually min+1 for triple buffering)
        uint32_t imageCount = gpuData.surfaceCapabilities.minImageCount + 1;
        if (gpuData.surfaceCapabilities.maxImageCount > 0 &&
            imageCount > gpuData.surfaceCapabilities.maxImageCount)
        {
            imageCount = gpuData.surfaceCapabilities.maxImageCount;
        }
        createInfo.minImageCount = imageCount;

        // Select format
        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(GPUDevice().surfaceFormats);
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;

        // Select extent
        VkExtent2D extent = ChooseSwapExtent(GPUDevice().surfaceCapabilities);
        createInfo.imageExtent = extent;

        // Additional settings
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        // Handle queue families
        QueueFamilyIndices indices = device->GetPhysicalDevice()->GetQueueFamilyIndices();
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = GPUDevice().surfaceCapabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = ChooseSwapPresentMode(GPUDevice().presentModes);
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device->Selected(), &createInfo, allocator, &swapChain) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to create swap chain!");
            ErrMsg("Failed to create swap chain!");
        }

        // Get swap chain images
        vkGetSwapchainImagesKHR(device->Selected(), swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device->Selected(), swapChain, &imageCount, swapChainImages.data());

        // Store format and extent for later use
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;

        SEDX_CORE_INFO("Swap chain created successfully with {} images", imageCount);
	}

    void SwapChain::Create(uint32_t *width, uint32_t *height, bool vsync)
    {
        gfxData.VSync = vsync;
        VkDevice vulkanDevice = device->GetDevice();
		VkPhysicalDevice physicalDevice = device->GetPhysicalDevice()->GetGPUDevice();
        VkSwapchainKHR oldSwapChain = swapChain;

        VkSurfaceCapabilitiesKHR surfaceInfo;
        VK_CHECK_RESULT(fpGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceInfo));



    }


    /**
	 * @brief Destroy the swap chain.
	 */
	void SwapChain::Destroy()
	{

        for (size_t i = 0; i < swapImages.size(); i++)
        {
            vkDestroyImageView(device->GetDevice(), swapChainViews[i], nullptr);
        }

		for (size_t i = 0; i < gfxData.FramesInFlight; i++)
        {
            vkDestroySemaphore(device->GetDevice(), imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(device->GetDevice(), renderFinishedSemaphores[i], nullptr);
        }

		vkDestroySwapchainKHR(device->GetDevice(), swapChain, nullptr);

		for (auto &queue : queues)
        {
            for (auto i = 0; std::cmp_less(i, gfxData.FramesInFlight); i++)
			{
                vkDestroyCommandPool(device->GetDevice(), queue.commands[i].pool, nullptr);
                queue.commands[i].staging = {};
                queue.commands[i].stagingCpu = nullptr;
                vkDestroyFence(device->GetDevice(), queue.commands[i].fence, nullptr);
                vkDestroyQueryPool(device->GetDevice(), queue.commands[i].queryPool, nullptr);
			}
		}

		imageAvailableSemaphores.clear();
        renderFinishedSemaphores.clear();
        swapChainViews.clear();
        swapImages.clear();
        swapChain = VK_NULL_HANDLE;

		vkDeviceWaitIdle(device->GetDevice());
    }

    void SwapChain::OnResize(uint32_t width, uint32_t height)
    {
        auto vulkanDevice = device->GetDevice();
        gfxData.width = width;
        gfxData.height = height;
        if (gfxData.width == 0 || gfxData.height == 0)
        {
            SEDX_CORE_INFO("Window minimized, waiting for restore");
            return;
        }
        vkDeviceWaitIdle(vulkanDevice);
        SEDX_CORE_INFO("Window resized to {}x{}", gfxData.width, gfxData.height);
        vkDestroySwapchainKHR(vulkanDevice, swapChain, nullptr);
        Create(&gfxData.width, &gfxData.height, gfxData.VSync);
    }

    uint32_t SwapChain::AcquireNextImage()
    {
        gfxData.swapChainCurrentFrame = (gfxData.swapChainCurrentFrame + 1) % swapChainImages.size();
    }

    VkSurfaceFormatKHR SwapChain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
    {
        for (const auto &availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR SwapChain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
    {
        // Check if VSync is disabled and prefer mailbox if available
        if (!gfxData.VSync)
        {
            for (const auto &availablePresentMode : availablePresentModes)
            {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    return availablePresentMode;
                }
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D SwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
    {
        // If the current extent width is the max value, it means the window manager
        // allows us to set dimensions other than the current window size
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            SEDX_CORE_INFO("Using surface extent: {}x{}", capabilities.currentExtent.width, capabilities.currentExtent.height);
            return capabilities.currentExtent;
        }
        else
        {
            // Get the actual framebuffer size from GLFW directly for consistency
            int width, height;
            glfwGetFramebufferSize(Window::GetWindow(), &width, &height);

			gfxData.width = width;
            gfxData.height = height;

            SEDX_CORE_INFO("Window framebuffer size: {}x{}", width, height);

            // Create the extent using the retrieved dimensions
            VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

            // Clamp to the allowed min/max extents from the surface capabilities
            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            SEDX_CORE_INFO("Using calculated extent: {}x{}", actualExtent.width, actualExtent.height);
            return actualExtent;
        }
    }

    /**
     * @brief Query the swap chain support details for the device.
     * @param device - The Vulkan device to query.
     * @return - The swap chain support details.
     */
    SwapChainSupportDetails SwapChain::QuerySwapChainSupport(const VulkanDevice &device)
    {
        SwapChainSupportDetails details;

        // Copy the data that's already available
        details.capabilities = device.GetPhysicalDevice()->Selected().surfaceCapabilities;
        details.formats = device.GetPhysicalDevice()->Selected().surfaceFormats;
        details.presentModes = device.GetPhysicalDevice()->Selected().presentModes;

        return details;
    }

	/**
	 * @brief Find the image format and color space for the swap chain.
	 */
    void SwapChain::FindImageFormatAndColorSpace()
    {
        const VkPhysicalDevice physicalDevice = device->GetPhysicalDevice()->GetGPUDevice();

		uint32_t formatCount;
        fpGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

		std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
        fpGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data());

		if ((formatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED))
		{
			colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
			colorSpace = surfaceFormats[0].colorSpace;
		}
		else
		{
			// iterate over the list of available surface format and
			// check for the presence of VK_FORMAT_B8G8R8A8_UNORM
			auto found_B8G8R8A8_UNORM = false;
			for (auto&[format, colorSpace] : surfaceFormats)
			{
				if (format == VK_FORMAT_B8G8R8A8_UNORM)
				{
					colorFormat = format;
					colorSpace = colorSpace;
					found_B8G8R8A8_UNORM = true;
					break;
				}
			}

			// in case VK_FORMAT_B8G8R8A8_UNORM is not available
			// select the first available color format
			if (!found_B8G8R8A8_UNORM)
			{
				colorFormat = surfaceFormats[0].format;
				colorSpace = surfaceFormats[0].colorSpace;
			}
		}
    }



} // namespace SceneryEditorX

// -------------------------------------------------------


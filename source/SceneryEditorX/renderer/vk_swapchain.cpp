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
#include <SceneryEditorX/renderer/vk_cmd_buffers.h>
#include <SceneryEditorX/renderer/vk_device.h>
#include <SceneryEditorX/renderer/vk_swapchain.h>
#include <SceneryEditorX/renderer/vk_util.h>
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
        instance = GraphicsEngine::GetInstance();
        this->device = device;

        GET_DEVICE_PROC_ADDR(vkDevice, CreateSwapchainKHR);
        GET_DEVICE_PROC_ADDR(vkDevice, DestroySwapchainKHR);
        GET_DEVICE_PROC_ADDR(vkDevice, GetSwapchainImagesKHR);
        GET_DEVICE_PROC_ADDR(vkDevice, AcquireNextImageKHR);
        GET_DEVICE_PROC_ADDR(vkDevice, QueuePresentKHR);

        GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfaceSupportKHR);
        GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfaceCapabilitiesKHR);
        GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfaceFormatsKHR);
        GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfacePresentModesKHR);

        GET_INSTANCE_PROC_ADDR(instance, CmdSetCheckpointNV);
        GET_INSTANCE_PROC_ADDR(instance, GetQueueCheckpointDataNV);
    }

    // -------------------------------------------------------

    void SwapChain::InitSurface(const Ref<Window> &window)
    {
        VkPhysicalDevice physicalDevice = device->GetPhysicalDevice()->GetGPUDevice();
        GPUDevice gpuData;
        // Create the surface
        if (glfwCreateWindowSurface(GraphicsEngine::GetInstance(), window->GetWindow(), allocator, &surface) != VK_SUCCESS)
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
        swapChainExtent = ChooseSwapExtent(GPUDevice().surfaceCapabilities, GetWidth(), GetHeight());

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

        if (vkCreateSwapchainKHR(vkDevice, &createInfo, allocator, &swapChain) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to create swap chain!");
            ErrMsg("Failed to create swap chain!");
        }

        // Get swap chain images
        vkGetSwapchainImagesKHR(vkDevice, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(vkDevice, swapChain, &imageCount, swapChainImages.data());

        // Store format and extent for later use
        swapChainImageFormat = surfaceFormat.format;

        SEDX_CORE_INFO("Swap chain created successfully with {} images", imageCount);
	}

    void SwapChain::Create(uint32_t width, uint32_t height, bool vsync)
    {
        viewportData.width = width;
        viewportData.height = height;
        renderData.VSync = vsync;
		VkPhysicalDevice physicalDevice = device->GetPhysicalDevice()->GetGPUDevice();
        VkSwapchainKHR oldSwapChain = swapChain;

        VkSurfaceCapabilitiesKHR surfaceInfo;
        VK_CHECK_RESULT(fpGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceInfo));
    }

	void SwapChain::Destroy()
	{
        vkDestroyImageView(vkDevice, depthImageView, allocator);
        vkDestroyImage(vkDevice, depthImage, allocator);
        vkFreeMemory(vkDevice, depthImageMemory, allocator);

        for (auto framebuffer : swapChainFramebuffers)
        {
            vkDestroyFramebuffer(vkDevice, framebuffer, allocator);
        }

        for (size_t i = 0; i < swapChainImages.size(); i++)
        {
            vkDestroyImageView(vkDevice, swapChainViews[i], allocator);
        }

        vkDestroyPipeline(vkDevice, pipeline->GetPipeline(), allocator);
        vkDestroyPipelineLayout(vkDevice, pipeline->GetVulkanPipelineLayout(), allocator);
        vkDestroyRenderPass(vkDevice, renderPass, allocator);

        for (size_t i = 0; i < swapImages.size(); i++)
        {
            vkDestroyImageView(vkDevice, swapChainViews[i], nullptr);
        }

		for (size_t i = 0; i < RenderData::framesInFlight; i++)
        {
            vkDestroySemaphore(vkDevice, imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(vkDevice, renderFinishedSemaphores[i], nullptr);
        }

		vkDestroySwapchainKHR(vkDevice, swapChain, nullptr);

		for (auto &queue : queues)
        {
            for (auto i = 0; std::cmp_less(i, RenderData::framesInFlight); i++)
			{
                vkDestroyCommandPool(vkDevice, queue.commands[i].pool, nullptr);
                queue.commands[i].stagingOffset = {};
                queue.commands[i].stagingCpu = nullptr;
                vkDestroyFence(vkDevice, queue.commands[i].fence, nullptr);
                vkDestroyQueryPool(vkDevice, queue.commands[i].queryPool, nullptr);
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
        renderData.width = width;
        renderData.height = height;
        if (renderData.width == 0 || renderData.height == 0)
        {
            SEDX_CORE_INFO("Window minimized, waiting for restore");
            return;
        }
        vkDeviceWaitIdle(vkDevice);
        SEDX_CORE_INFO("Window resized to {}x{}", renderData.width, renderData.height);
        vkDestroySwapchainKHR(vkDevice, swapChain, nullptr);
        Create(viewportData.width, viewportData.height, renderData.VSync);
    }

    uint32_t SwapChain::AcquireNextImage()
    {
        renderData.swapChainCurrentFrame = (renderData.swapChainCurrentFrame + 1) % swapChainImages.size();

        VK_CHECK_RESULT(vkWaitForFences(vkDevice, 1, &waitFences[GetImageIndex()], VK_TRUE, UINT64_MAX));

		uint32_t imageIndex;

        if (VkResult result = fpAcquireNextImageKHR(vkDevice, swapChain, UINT64_MAX, imageAvailableSemaphores[GetImageIndex()], (VkFence)nullptr, &imageIndex); result != VK_SUCCESS)
		{
			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			{
                OnResize(viewportData.width, viewportData.height);
				VK_CHECK_RESULT(fpAcquireNextImageKHR(vkDevice, swapChain, UINT64_MAX, imageAvailableSemaphores[GetImageIndex()], (VkFence)nullptr, &imageIndex));
			}
		}


		return imageIndex;
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

	void SwapChain::CreateImageViews()
    {
        swapChainViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++)
        {
            swapChainViews[i] = CreateImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }
    }

    void SwapChain::CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels,
        VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
        VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage&image, VkDeviceMemory&imageMemory) const
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = numSamples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(vkDevice, &imageInfo, allocator, &image) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to create image!");
            ErrMsg("Failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(vkDevice, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(vkDevice, &allocInfo, allocator, &imageMemory) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to allocate image memory!");
            ErrMsg("Failed to allocate image memory!");
        }

        vkBindImageMemory(vkDevice, image, imageMemory, 0);

    }

    uint32_t SwapChain::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
    {
        VkPhysicalDeviceMemoryProperties memProperties = device->GetPhysicalDevice()->GetMemoryProperties();

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        SEDX_CORE_ERROR("Failed to find suitable memory type!");
        throw std::runtime_error("Failed to find suitable memory type!");
    }

    VkImageView SwapChain::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(vkDevice, &viewInfo, allocator, &imageView) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to create texture image view!");
            ErrMsg("Failed to create texture image view!");
        }

        return imageView;
    }

    VkFormat SwapChain::FindDepthFormat()
    {
        return FindSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                   VK_IMAGE_TILING_OPTIMAL,
                                   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    void SwapChain::CreateDepthResources()
    {
        VkFormat depthFormat = FindDepthFormat();

        CreateImage(swapChainExtent.width, swapChainExtent.height, 1, renderData.msaaSamples, depthFormat,
                    VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    depthImage, depthImageMemory);
        depthImageView = CreateImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    }

    VkPresentModeKHR SwapChain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) const
    {
        // Check if VSync is disabled and prefer mailbox if available
        if (!renderData.VSync)
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

	VkFormat  SwapChain::FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
    {
        for (VkFormat format : candidates)
        {
            VkFormatProperties props;
            //const GPUDevice &selectedDevice = vkDevice->GetPhysicalDevice();
            const VkPhysicalDevice physicalDevice = vkPhysDevice;


            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            {
                return format;
            }
        }

        throw std::runtime_error("Failed to find supported format!");
    }

    VkExtent2D SwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, uint32_t width, uint32_t height)
    {
        // If the current extent width is the max value, it means the window manager
        // allows us to set dimensions other than the current window size
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            SEDX_CORE_INFO("Using surface extent: {}x{}", capabilities.currentExtent.width, capabilities.currentExtent.height);
            return capabilities.currentExtent;
        }
        else
        {
            // Get the actual framebuffer size from GLFW directly for consistency
            VkExtent2D actualExtent = {width, height};

            actualExtent.width = std::max(capabilities.minImageExtent.width,
                                          std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height,
                                           std::min(capabilities.maxImageExtent.height, actualExtent.height));

			SEDX_CORE_INFO("Using calculated extent: {}x{}", actualExtent.width, actualExtent.height);

            return actualExtent;
        }
    }

    /**
     * @brief Query the swap chain support details for the device.
     * @param device - The Vulkan device to query.
     * @return - The swap chain support details.
     */
    SwapChainDetails SwapChain::QuerySwapChainSupport(const VulkanDevice &device)
    {
        SwapChainDetails details;

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
		uint32_t formatCount;
        fpGetPhysicalDeviceSurfaceFormatsKHR(vkPhysDevice, surface, &formatCount, nullptr);

		std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
        fpGetPhysicalDeviceSurfaceFormatsKHR(vkPhysDevice, surface, &formatCount, surfaceFormats.data());

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


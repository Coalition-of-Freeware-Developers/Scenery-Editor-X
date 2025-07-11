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
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/image_data.h>
#include <SceneryEditorX/renderer/vulkan/vk_data.h>
#include <SceneryEditorX/renderer/vulkan/vk_device.h>
#include <SceneryEditorX/renderer/vulkan/vk_swapchain.h>
#include <SceneryEditorX/renderer/vulkan/vk_util.h>

/// -------------------------------------------------------

/// Macro to get a procedure address based on a vulkan instance
#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                                                                   \
{                                                                                                                  \
    fp##entrypoint = reinterpret_cast<PFN_vk##entrypoint>(vkGetInstanceProcAddr(inst, "vk" #entrypoint));          \
    SEDX_CORE_ASSERT(fp##entrypoint);                                                                              \
}

/// Macro to get a procedure address based on a vulkan device
#define GET_DEVICE_PROC_ADDR(dev, entrypoint)                                                                      \
{                                                                                                                  \
    fp##entrypoint = reinterpret_cast<PFN_vk##entrypoint>(vkGetDeviceProcAddr(dev, "vk" #entrypoint));             \
    SEDX_CORE_ASSERT(fp##entrypoint);                                                                              \
}

/// -------------------------------------------------------

static PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
static PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
static PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
static PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR;
static PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
static PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
static PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
static PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
static PFN_vkQueuePresentKHR fpQueuePresentKHR;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Nvidia extensions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PFN_vkCmdSetCheckpointNV fpCmdSetCheckpointNV;
PFN_vkGetQueueCheckpointDataNV fpGetQueueCheckpointDataNV;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AMD extensions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* Add AMD Specific extensions later when added */

/// -------------------------------------------------------

static uint32_t GetImageMemorySize(VkFormat format, uint32_t width, uint32_t height)
{
    return width * height * getBPP(format);
}

/// -------------------------------------------------------

VKAPI_ATTR void VKAPI_CALL vkCmdSetCheckpointNV(VkCommandBuffer commandBuffer, const void *pCheckpointMarker)
{
    fpCmdSetCheckpointNV(commandBuffer, pCheckpointMarker);
}

VKAPI_ATTR void VKAPI_CALL vkGetQueueCheckpointDataNV(VkQueue queue, uint32_t *pCheckpointDataCount, VkCheckpointDataNV *pCheckpointData)
{
    fpGetQueueCheckpointDataNV(queue, pCheckpointDataCount, pCheckpointData);
}

/// -------------------------------------------------------

namespace SceneryEditorX
{

    void SwapChain::Init(const VkInstance instance, const Ref<VulkanDevice> &device)
    {
        /// Don't reassign the parameter "instance"
        this->instance = instance;
        vkDevice = device;

		const VkDevice vkDeviceHandle = vkDevice->GetDevice();
        GET_DEVICE_PROC_ADDR(vkDeviceHandle, CreateSwapchainKHR)
        GET_DEVICE_PROC_ADDR(vkDeviceHandle, DestroySwapchainKHR)
        GET_DEVICE_PROC_ADDR(vkDeviceHandle, GetSwapchainImagesKHR)
        GET_DEVICE_PROC_ADDR(vkDeviceHandle, AcquireNextImageKHR)
        GET_DEVICE_PROC_ADDR(vkDeviceHandle, QueuePresentKHR)

        GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfaceSupportKHR)
        GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfaceCapabilitiesKHR)
        GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfaceFormatsKHR)
        GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfacePresentModesKHR)

        GET_INSTANCE_PROC_ADDR(instance, CmdSetCheckpointNV)
        GET_INSTANCE_PROC_ADDR(instance, GetQueueCheckpointDataNV)
    }

    /// -------------------------------------------------------

    std::vector<int32_t> ImageID::availImageRID;
    std::vector<int32_t> ImageID::availBufferRID;
    std::vector<int32_t> ImageID::availTLASRID;

    void SwapChain::InitSurface(GLFWwindow *windowPtr)
    {
        const VkPhysicalDevice physicalDevice = vkDevice->GetPhysicalDevice()->GetGPUDevices();
        GPUDevice gpuData;

        /// Create the surface
        glfwCreateWindowSurface(instance, windowPtr, nullptr, &surface);

        uint32_t queueCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);
        SEDX_ASSERT(queueCount >= 1);

        std::vector<VkQueueFamilyProperties> queueProps(queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueProps.data());

        /// Find a queue with present support for presenting swapchain images
        std::vector<VkBool32> supportsPresent(queueCount);
        for (uint32_t i = 0; i < queueCount; i++)
            fpGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresent[i]);

        /// Search for a graphics and a present queue in the array of queue families, try to find one that supports both
        auto graphicsQueueNodeIndex = UINT32_MAX;
        auto presentQueueNodeIndex = UINT32_MAX;
        for (uint32_t i = 0; i < queueCount; i++)
        {
            if ((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
            {
                if (graphicsQueueNodeIndex == UINT32_MAX)
                    graphicsQueueNodeIndex = i;

                if (supportsPresent[i] == VK_TRUE)
                {
                    graphicsQueueNodeIndex = i;
                    presentQueueNodeIndex = i;
                    break;
                }
            }
        }
        if (presentQueueNodeIndex == UINT32_MAX)
        {
            /// If there's no queue that supports both present and graphics try to find a separate present queue
            for (uint32_t i = 0; i < queueCount; ++i)
            {
                if (supportsPresent[i] == VK_TRUE)
                {
                    presentQueueNodeIndex = i;
                    break;
                }
            }
        }

        SEDX_CORE_ASSERT(graphicsQueueNodeIndex != UINT32_MAX, "Failed to find a graphics queue!");
        SEDX_CORE_ASSERT(presentQueueNodeIndex != UINT32_MAX, "Failed to find a graphics queue!");

        queueIndex = graphicsQueueNodeIndex;

        FindImageFormatAndColorSpace();
    }

    /*
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

        if (vkCreateSwapchainKHR(device, &createInfo, allocator, &swapChain) != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to create swap chain!");
            ErrMsg("Failed to create swap chain!");
        }

        // Get swap chain images
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        // Store format and extent for later use
        swapChainImageFormat = surfaceFormat.format;

        SEDX_CORE_INFO("Swap chain created successfully with {} images", imageCount);
	}
	*/

    void SwapChain::Create(uint32_t *width, uint32_t *height, bool vsync)
    {
        VSync = vsync;

		VkDevice device = vkDevice->GetDevice();
        RenderData data;
        data.width = *width;
        data.height = *height;
        data.VSync = vsync;

		VkPhysicalDevice physicalDevice = vkDevice->GetPhysicalDevice()->GetGPUDevices();
        VkSwapchainKHR oldSwapChain = swapChain;

		/// Get physical device surface properties and formats
        VkSurfaceCapabilitiesKHR surfaceInfo = vkDevice->GetPhysicalDevice()->Selected().surfaceCapabilities;
        VK_CHECK_RESULT(fpGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceInfo))

        /// Get available present modes
        uint32_t presentModeCount;
        VK_CHECK_RESULT(fpGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr))
        SEDX_CORE_ASSERT(presentModeCount > 0, "No present modes available!");
        std::vector<VkPresentModeKHR> presentModes(vkDevice->GetPhysicalDevice()->Selected().presentModes);
        presentModes.resize(presentModeCount);
        VK_CHECK_RESULT(fpGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()))

		VkExtent2D swapExtent = {};
        /// If width (and height) equals the special value 0xFFFFFFFF, the size of the surface will be set by the swapchain
        if (std::cmp_equal(surfaceInfo.currentExtent.width, -1) && std::cmp_equal(surfaceInfo.currentExtent.height, -1))
        {
			/// If the surface size is undefined, the size is set to the size of the images requested.
            swapExtent.width = *width;
            swapExtent.height = *height;
        }
        else
        {
            swapExtent = surfaceInfo.currentExtent;
            *width = surfaceInfo.currentExtent.width;
            *height = surfaceInfo.currentExtent.height;
        }

		swapWidth = *width;
		swapHeight = *height;

        if (*width == 0 || *height == 0)
        {
            SEDX_CORE_TRACE_TAG("Graphics Engine", "Window minimized, waiting for restore");
            return;
        }

		/// -------------------------------------------------------

        /// Get available surface formats
        /*
        uint32_t formatCount;
        VK_CHECK_RESULT(fpGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr))
        SEDX_CORE_ASSERT(formatCount > 0, "No surface formats available!");
        */

        /// The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
        /// This mode waits for the vertical blank ("v-sync")
        VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

        /// If v-sync is not requested, try to find a mailbox mode
        /// It's the lowest latency non-tearing present mode available
        if (!vsync)
		{
			for (size_t i = 0; i < presentModeCount; i++)
			{
				if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
					break;
				}
				if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) && (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR))
				{
					swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
				}
			}
		}

		/// Determine the number of images
        uint32_t desiredNumberOfSwapchainImages = surfaceInfo.minImageCount + 1;
        if (surfaceInfo.maxImageCount > 0 && desiredNumberOfSwapchainImages > surfaceInfo.maxImageCount)
            desiredNumberOfSwapchainImages = surfaceInfo.maxImageCount;

        /// Find the transformation of the surface
        VkSurfaceTransformFlagsKHR preTransform;
        if (surfaceInfo.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
            /// We prefer a non-rotated transform
            preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        else
            preTransform = surfaceInfo.currentTransform;

        /// Find a supported composite alpha format (not all devices support alpha opaque)
        VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        /// Simply select the first composite alpha format available
        std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
        };
        for (auto &compositeAlphaFlag : compositeAlphaFlags)
        {
            if (surfaceInfo.supportedCompositeAlpha & compositeAlphaFlag)
            {
                compositeAlpha = compositeAlphaFlag;
                break;
            }
        }

        ///////////////////////////////////////////////////////////////////////////////////
		/// SwapChain Creation
        ///////////////////////////////////////////////////////////////////////////////////

        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.pNext = nullptr;
        createInfo.surface = surface;
        createInfo.minImageCount = desiredNumberOfSwapchainImages;
        createInfo.imageFormat = colorFormat;
        createInfo.imageColorSpace = colorSpace;
        createInfo.imageExtent = {swapExtent.width,swapExtent.height};
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
        createInfo.presentMode = ChooseSwapPresentMode(presentModeCount);
        createInfo.oldSwapchain = oldSwapChain;
        /// Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area.
        createInfo.clipped = VK_TRUE;
        createInfo.compositeAlpha = compositeAlpha;

		/// Enable transfer source on swap chain images if supported
        if (surfaceInfo.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
            createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        /// Enable transfer destination on swap chain images if supported
        if (surfaceInfo.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		
        VK_CHECK_RESULT(fpCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain))

        if (oldSwapChain)
            fpDestroySwapchainKHR(device, oldSwapChain, nullptr);

        for (auto &[Image, ImageView] : swapChainImage)
            vkDestroyImageView(device, ImageView, nullptr);

        VK_CHECK_RESULT(fpGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, NULL))

        /// Swapchain Images
        swapChainImage.resize(swapChainImageCount);
        swapChainImageCounts.resize(swapChainImageCount);
        VK_CHECK_RESULT(fpGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, swapChainImageCounts.data()))

		/// Acquire Swapchain buffers holding swapChainImage and imageview
        swapChainImage.resize(swapChainImageCount);
		for (uint32_t i = 0; i < swapChainImageCount; i++)
        {
			VkImageViewCreateInfo colorAttachmentView = {};
			colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			colorAttachmentView.pNext = nullptr;
			colorAttachmentView.format = colorFormat;
            colorAttachmentView.image = swapChainImageCounts[i];
			colorAttachmentView.components = {
				VK_COMPONENT_SWIZZLE_R,
				VK_COMPONENT_SWIZZLE_G,
				VK_COMPONENT_SWIZZLE_B,
				VK_COMPONENT_SWIZZLE_A
			};
			colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			colorAttachmentView.subresourceRange.baseMipLevel = 0;
			colorAttachmentView.subresourceRange.levelCount = 1;
			colorAttachmentView.subresourceRange.baseArrayLayer = 0;
			colorAttachmentView.subresourceRange.layerCount = 1;
			colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
			colorAttachmentView.flags = 0;

			swapChainImage[i].Image = swapChainImageCounts[i];
            VK_CHECK_RESULT(vkCreateImageView(device, &colorAttachmentView, nullptr, &swapChainImage[i].ImageView))
			SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_IMAGE_VIEW, std::format("Swapchain ImageView {0}", i), swapChainImage[i].ImageView);
        }

        ///////////////////////////////////////////////////////////////////////////////////
        /// SwapChain Command Buffers
        ///////////////////////////////////////////////////////////////////////////////////
        for (auto &[CommandPool, CommandBuffer] : cmdBuffers)
            vkDestroyCommandPool(device, CommandPool, nullptr);
		
        VkCommandPoolCreateInfo cmdPoolInfo = {};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.queueFamilyIndex = queueIndex;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = 1;

		cmdBuffers.resize(swapChainImageCount);
        for (auto &commandBuffer : cmdBuffers)
		{
			VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &commandBuffer.CommandPool))
			commandBufferAllocateInfo.commandPool = commandBuffer.CommandPool;
			VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer.CommandBuffer))
        }

        ///////////////////////////////////////////////////////////////////////////////////
        /// Synchronization Objects
        ///////////////////////////////////////////////////////////////////////////////////
        RenderData renderData;
        auto framesInFlight = renderData.framesInFlight;
        if (imageAvailableSemaphores.size() != framesInFlight)
		{
            imageAvailableSemaphores.resize(framesInFlight);
            renderFinishedSemaphores.resize(framesInFlight);
			VkSemaphoreCreateInfo semaphoreCreateInfo{};
			semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			for (size_t i = 0; i < framesInFlight; i++)
			{
                VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphores[i]))
				SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_SEMAPHORE, std::format("Swapchain Semaphore ImageAvailable {0}", i), imageAvailableSemaphores[i]);
                VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i]))
				SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_SEMAPHORE, std::format("Swapchain Semaphore RenderFinished {0}", i), renderFinishedSemaphores[i]);
			}
		}

		if (waitFences.size() != renderData.framesInFlight)
        {
            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            waitFences.resize(renderData.framesInFlight);
            for (auto &fence : waitFences)
            {
                VK_CHECK_RESULT(vkCreateFence(vkDevice->GetDevice(), &fenceInfo, nullptr, &fence))
                SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_FENCE, "Swapchain Fence {0}", fence);
            }
        }

		VkPipelineStageFlags pipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkFormat depthFormat = FindDepthFormat();

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Render Pass
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = colorFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef = {};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        //subpass.pDepthStencilAttachment = &depthAttachmentRef; //TODO: See if this is needed
        subpass.inputAttachmentCount = 0;
        subpass.pInputAttachments = nullptr;
        subpass.pResolveAttachments = nullptr;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = nullptr;

		VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcAccessMask = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

		VK_CHECK_RESULT(vkCreateRenderPass(vkDevice->GetDevice(), &renderPassInfo, nullptr, &renderPass))
        SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_RENDER_PASS, "Swapchain Render Pass", renderPass);

		//////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Framebuffers
        //////////////////////////////////////////////////////////////////////////////////////////////////////////

		for (auto &framebuffer : swapChainFramebuffers)
            vkDestroyFramebuffer(device, framebuffer, nullptr);

		VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.width = swapWidth;
        framebufferInfo.height = swapHeight;
        framebufferInfo.layers = 1;

		swapChainFramebuffers.resize(swapChainImageCount);
        for (size_t i = 0; i < swapChainFramebuffers.size(); i++)
        {
            framebufferInfo.pAttachments = &swapChainImage[i].ImageView;
            VK_CHECK_RESULT(vkCreateFramebuffer(vkDevice->GetDevice(), &framebufferInfo, nullptr, &swapChainFramebuffers[i]))
            SetDebugUtilsObjectName(vkDevice->GetDevice(), VK_OBJECT_TYPE_FRAMEBUFFER, std::format("Swapchain Framebuffer {0}", i), swapChainFramebuffers[i]);
        }
    }

	void SwapChain::Destroy()
    {
        const auto device = vkDevice->GetDevice();
        vkDeviceWaitIdle(device);

		if (swapChain)
            fpDestroySwapchainKHR(device, swapChain, nullptr);

		for (auto &[Image, ImageView] : swapChainImage)
            vkDestroyImageView(device, ImageView, nullptr);
        swapChainImage.clear();

        for (const auto &[CommandPool, CommandBuffer] : cmdBuffers)
            vkDestroyCommandPool(device, CommandPool, nullptr);
        cmdBuffers.clear();

        if (renderPass)
            vkDestroyRenderPass(device, renderPass, nullptr);

        for (const auto framebuffer : swapChainFramebuffers)
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        swapChainFramebuffers.clear();

        for (const auto &semaphore : imageAvailableSemaphores)
            vkDestroySemaphore(device, semaphore, nullptr);
        imageAvailableSemaphores.clear();

        for (const auto &semaphore : renderFinishedSemaphores)
            vkDestroySemaphore(device, semaphore, nullptr);
        renderFinishedSemaphores.clear();

        for (const auto &fence : waitFences)
            vkDestroyFence(device, fence, nullptr);
        waitFences.clear();

        vkDeviceWaitIdle(device);

    }

    void SwapChain::BeginFrame()
    {
        /// Resource release queue
        auto &queue = Renderer::GetRenderResourceReleaseQueue(currentFrameIdx);
        queue.Execute();

        currentFrameIdx = AcquireNextImage();
        VK_CHECK_RESULT(vkResetCommandPool(vkDevice->GetDevice(), cmdBuffers[currentFrameIdx].CommandPool, 0))
    }

    void SwapChain::OnResize(uint32_t width, uint32_t height)
    {
        const auto device = vkDevice->GetDevice();
        vkDeviceWaitIdle(device);
        Create(&width, &height, VSync);
        vkDeviceWaitIdle(device);
    }

    void SwapChain::Present()
    {
        VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

				
		VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pWaitDstStageMask = &waitStageMask;
        submitInfo.pWaitSemaphores = &imageAvailableSemaphores[currentFrameIdx];
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &renderFinishedSemaphores[currentFrameIdx];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffers[currentFrameIdx].CommandBuffer;
        submitInfo.commandBufferCount = 1;

        VK_CHECK_RESULT(vkResetFences(vkDevice->GetDevice(), 1, &waitFences[currentFrameIdx]))

        vkDevice->LockQueue();

        VK_CHECK_RESULT(vkQueueSubmit(vkDevice->GetGraphicsQueue(), 1, &submitInfo, waitFences[currentFrameIdx]));

        VkResult result;
        {
            VkPresentInfoKHR presentInfo = {};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.pNext = nullptr;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &swapChain;
            presentInfo.pImageIndices = &currentImageIdx;

            presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrameIdx];
            presentInfo.waitSemaphoreCount = 1;
            result = fpQueuePresentKHR(vkDevice->GetGraphicsQueue(), &presentInfo);
        }

        vkDevice->UnlockQueue();
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			OnResize(swapWidth, swapHeight);
        }
		else if (result != VK_SUCCESS)
		{
			SEDX_CORE_WARN_TAG("VULKAN CORE","Failed to present swap chain image!");
            VK_CHECK_RESULT(result)
        }
    }

    uint32_t SwapChain::AcquireNextImage()
    {
        auto renderData = RenderData();
        currentFrameIdx = (currentFrameIdx + 1) % renderData.framesInFlight;

        const auto device = vkDevice->GetDevice();
        renderData.swapChainCurrentFrame = (renderData.swapChainCurrentFrame + 1) % swapChainImage.size();

        VK_CHECK_RESULT(vkWaitForFences(device, 1, &waitFences[renderData.imageIndex], VK_TRUE, UINT64_MAX));

		uint32_t imageIndex;
        if (const VkResult result = fpAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[renderData.imageIndex], (VkFence)nullptr, &imageIndex); result != VK_SUCCESS)
        {
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
            {
                OnResize(swapWidth, swapHeight);
                VK_CHECK_RESULT(fpAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[renderData.imageIndex], (VkFence)nullptr, &imageIndex))
            }
        }

        return imageIndex;
	}

    VkSurfaceFormatKHR SwapChain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
    {
        for (const auto &availableFormat : availableFormats)
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return availableFormat;

        return availableFormats[0];
    }

    /*
	void SwapChain::CreateImageViews()
    {
        swapChainViews.resize(swapChainImage.size());

        for (size_t i = 0; i < swapChainImage.size(); i++)
            swapChainViews[i] = CreateImageView(swapChainImage[i].Image, static_cast<VkFormat>(swapChainImage[i]),  VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
    */

    void SwapChain::CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
        VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory &imageMemory) const
    {
        const auto device = vkDevice->GetDevice();
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

        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
            SEDX_CORE_ERROR("Failed to create image!");

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
            SEDX_CORE_ERROR("Failed to allocate image memory!");

        vkBindImageMemory(device, image, imageMemory, 0);
    }

    uint32_t SwapChain::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
    {
        const auto physDevice = vkDevice->GetPhysicalDevice()->GetGPUDevices();
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physDevice, &memProperties);
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;
        }

        SEDX_CORE_ERROR("Failed to find suitable memory type!");
        return UINT32_MAX; /// Return an invalid memory type index
    }

    VkImageView SwapChain::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const
    {
        const auto device = vkDevice->GetDevice();
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
        if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
            SEDX_CORE_ERROR("Failed to create texture image view!");

        return imageView;
    }

    VkFormat SwapChain::FindDepthFormat() const
    {
        return FindSupportedFormat({
			VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    void SwapChain::CreateDepthResources()
    {
        const RenderData renderData = {};
        const VkFormat depthFormat = FindDepthFormat();
        CreateImage(swapChainExtent.width, swapChainExtent.height, 1, renderData.msaaSamples, depthFormat,
                    VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    depthImage, depthImageMemory);
        depthImageView = CreateImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    }

    VkPresentModeKHR SwapChain::ChooseSwapPresentMode(uint32_t presentModeCount) const
    {
        VkPresentModeKHR swapchainPresentMode = vkDevice->GetPhysicalDevice()->Selected().presentModes[0];
        /// Check if VSync is disabled and prefer mailbox if available
        if (RenderData renderData; !VSync)
        {
            for (size_t i = 0; i < presentModeCount; i++)
            {
                if (swapchainPresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                    return swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;

                if (swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR && presentModeCount == VK_PRESENT_MODE_IMMEDIATE_KHR)
                    return swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }

        /// If VSync is enabled, use FIFO
        return VK_PRESENT_MODE_FIFO_KHR;
    }

	VkFormat SwapChain::FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
    {
        for (const VkFormat format : candidates)
        {
            const auto physDevice = vkDevice->GetPhysicalDevice()->GetGPUDevices();
            VkFormatProperties props;

            vkGetPhysicalDeviceFormatProperties(physDevice, format, &props);
            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
                return format;

            if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
                return format;
        }

        SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to find supported format!");
        return VK_FORMAT_UNDEFINED; /// Return a default or invalid format
    }

    VkExtent2D SwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, uint32_t width, uint32_t height)
    {
        /// If the current extent width is the max value, it means the window manager allows us to set dimensions other than the current window size
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            SEDX_CORE_INFO("Using surface extent: {}x{}", capabilities.currentExtent.width, capabilities.currentExtent.height);
            return capabilities.currentExtent;
        }
        /// Get the actual framebuffer size from GLFW directly for consistency
        VkExtent2D actualExtent = {width, height};

        actualExtent.width = std::max(capabilities.minImageExtent.width,
                                      std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height,
                                       std::min(capabilities.maxImageExtent.height, actualExtent.height));

        SEDX_CORE_INFO("Using calculated extent: {}x{}", actualExtent.width, actualExtent.height);

        return actualExtent;
    }

    /**
     * @brief Query the swap chain support details for the device.
     *
     * @param device - The Vulkan device to query.
     * @return - The swap chain support details.
     */
    SwapChainDetails SwapChain::QuerySwapChainSupport(const VulkanDevice &device)
    {
        SwapChainDetails details;
        details.formats		 = device.GetPhysicalDevice()->Selected().surfaceFormats;
        details.presentModes = device.GetPhysicalDevice()->Selected().presentModes;
        details.capabilities = device.GetPhysicalDevice()->Selected().surfaceCapabilities;

        return details;
    }

	/**
	 * @brief Find the image format and color space for the swap chain.
	 */
    void SwapChain::FindImageFormatAndColorSpace()
    {
        const VkPhysicalDevice physDevice = vkDevice->GetPhysicalDevice()->GetGPUDevices();

        /// Get list of supported surface formats
        uint32_t formatCount;
        VK_CHECK_RESULT(fpGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, nullptr))
        SEDX_CORE_ASSERT(formatCount > 0);

		std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
        VK_CHECK_RESULT(fpGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, surfaceFormats.data()))
		if (formatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
		{
			colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
			colorSpace = surfaceFormats[0].colorSpace;
		}
		else
		{
			/// Iterate over the list of available surface format and check for the presence of VK_FORMAT_B8G8R8A8_UNORM.
			bool foundRequestedFormat = false;
            for (auto &&surfaceFormat : surfaceFormats)
			{
                if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
				{
                    colorFormat = surfaceFormat.format;
                    colorSpace = surfaceFormat.colorSpace;
					foundRequestedFormat = true;
					break;
				}
			}

			/// In case VK_FORMAT_B8G8R8A8_UNORM is not available select the first available color format.
			if (!foundRequestedFormat)
			{
				colorFormat = surfaceFormats[0].format;
				colorSpace = surfaceFormats[0].colorSpace;
			}
		}
    }

} // namespace SceneryEditorX

/// -------------------------------------------------------


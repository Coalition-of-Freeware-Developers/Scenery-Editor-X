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
#include "swapchain.h"
#include "vulkan_data.h"
#include "device.h"
#include "vulkan_utils.h"
#include "SceneryEditorX/renderer/image_data.h"
#include "SceneryEditorX/renderer/render_dispatcher.h"
#include "SceneryEditorX/renderer/renderer.h"
#include <GLFW/glfw3.h>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Nvidia extensions																							  ///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PFN_vkCmdSetCheckpointNV fpCmdSetCheckpointNV;
PFN_vkGetQueueCheckpointDataNV fpGetQueueCheckpointDataNV;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AMD extensions																								  ///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* Add AMD Specific extensions later when added */

/* ------------------------------------------------------- */

static uint32_t GetImageMemorySize(VkFormat format, uint32_t width, uint32_t height)
{
    return width * height * SceneryEditorX::Utils::GetBPP(format);
}

/* ------------------------------------------------------- */

VKAPI_ATTR void VKAPI_CALL vkCmdSetCheckpointNV(VkCommandBuffer commandBuffer, const void *pCheckpointMarker)
{
    fpCmdSetCheckpointNV(commandBuffer, pCheckpointMarker);
}

VKAPI_ATTR void VKAPI_CALL vkGetQueueCheckpointDataNV(VkQueue queue, uint32_t *pCheckpointDataCount, VkCheckpointDataNV *pCheckpointData)
{
    fpGetQueueCheckpointDataNV(queue, pCheckpointDataCount, pCheckpointData);
}

/* ------------------------------------------------------- */

namespace SceneryEditorX
{

    enum class PresentMode : uint8_t
    {
		Immediate, // Doesn't wait.                  Frames are not dropped. Tearing.    Full on.
		Mailbox,   // Waits for v-blank.             Frames are dropped.     No tearing. Minimizes latency.
		Fifo,      // Waits for v-blank, every time. Frames are not dropped. No tearing. Minimizes stuttering.
    };

    /**
     * @brief Destroy the swap chain and its associated resources.
     */
    SwapChain::~SwapChain()
    {
		if (m_SwapChain != nullptr) Destroy();
    }

    /**
     * @brief Determines the appropriate color space based on the surface format
     * 
     * This function checks if the provided format supports HDR (High Dynamic Range) or 
     * should use standard SDR (Standard Dynamic Range) color space.
     * 
     * @param format The VkFormat to check for HDR support
     * @return VkColorSpaceKHR The appropriate color space (HDR10_ST2084 for HDR, SRGB_NONLINEAR for SDR)
     */
    static VkColorSpaceKHR GetColorSpace(const VkFormat format)
    {
        // Default to SDR (Standard Dynamic Range) color space
        VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        
        // Check if the format is a 10-bit per channel format (typical for HDR)
        // VK_FORMAT_A2B10G10R10_UNORM_PACK32 is the correct enum for 10-bit RGB format
        if (format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 || 
            format == VK_FORMAT_A2R10G10B10_UNORM_PACK32)
        {
            // Use HDR10 color space with ST2084 (PQ) transfer function
            colorSpace = VK_COLOR_SPACE_HDR10_ST2084_EXT;
        }

        return colorSpace;
    }

    static void SetHdrMetadata(const VkSwapchainKHR &swapchain)
    {
        auto ctx = RenderContext::Get();

        VkHdrMetadataEXT hdrMetadata = {};
        hdrMetadata.sType = VK_STRUCTURE_TYPE_HDR_METADATA_EXT;
        hdrMetadata.displayPrimaryRed.x = 0.708f;
        hdrMetadata.displayPrimaryRed.y = 0.292f;
        hdrMetadata.displayPrimaryGreen.x = 0.170f;
        hdrMetadata.displayPrimaryGreen.y = 0.797f;
        hdrMetadata.displayPrimaryBlue.x = 0.131f;
        hdrMetadata.displayPrimaryBlue.y = 0.046f;
        hdrMetadata.whitePoint.x = 0.3127f;
        hdrMetadata.whitePoint.y = 0.3290f;
        constexpr float nitsToLumin = 10000.0f;
        /* hdrMetadata.maxLuminance = Display::GetLuminanceMax() * nitsToLumin; */
        hdrMetadata.minLuminance = 0.001f * nitsToLumin;
        hdrMetadata.maxContentLightLevel = 2000.0f;
        hdrMetadata.maxFrameAverageLightLevel = 500.0f;

        PFN_vkSetHdrMetadataEXT pfnVkSetHdrMetadataEXT = reinterpret_cast<PFN_vkSetHdrMetadataEXT>(vkGetDeviceProcAddr(ctx->Get()->GetCurrentDevice()->GetDevice(), "vkSetHdrMetadataEXT"));
        SEDX_ASSERT(pfnVkSetHdrMetadataEXT != nullptr);
        pfnVkSetHdrMetadataEXT(ctx->Get()->GetCurrentDevice()->GetDevice(), 1, &swapchain, &hdrMetadata);
    }

    static VkSurfaceCapabilitiesKHR GetSurfaceCapabilities(const VkSurfaceKHR surface)
    {
        auto ctx = RenderContext::Get();
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->GetPhysicalDevice()->GetDevice(), surface, &surfaceCapabilities);
        return surfaceCapabilities;
    }

    static std::vector<VkPresentModeKHR> GetSupportedPresentModes(const VkSurfaceKHR surface)
    {
        auto ctx = RenderContext::Get();
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->GetPhysicalDevice()->GetDevice(), surface, &presentModeCount, nullptr);

        std::vector<VkPresentModeKHR> surfacePresentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->GetPhysicalDevice()->GetDevice(), surface, &presentModeCount, surfacePresentModes.data());
        return surfacePresentModes;
    }

    static VkPresentModeKHR GetPresentMode(const VkSurfaceKHR surface, const VkPresentModeKHR presentMode)
    {
        VkSurfaceCapabilitiesKHR surfaceInfo = GetSurfaceCapabilities(surface);
        std::vector<VkPresentModeKHR> presentModes = GetSupportedPresentModes(surface);

        // Prefer MAILBOX when available (low-latency triple buffering), otherwise fall back
        VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR; // guaranteed available
        
        if (!presentMode)
        {
            for (const auto& mode : presentModes)
            {
                if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                    break;
                }
            }

            if (swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR)
            {
                // If no mailbox, try immediate, then FIFO_RELAXED, then FIFO
                for (const auto& mode : presentModes)
                {
                    if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                    {
                        swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                        break;
                    }

                    if (mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
                        swapchainPresentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
                }
            }
        }

        // Determine the number of images: prefer 3 when using MAILBOX (triple buffering)
        uint32_t desiredNumberOfSwapchainImages =
            (swapchainPresentMode == VK_PRESENT_MODE_MAILBOX_KHR) ? 3 : (surfaceInfo.minImageCount + 1);
        if (surfaceInfo.maxImageCount > 0 && desiredNumberOfSwapchainImages > surfaceInfo.maxImageCount)
        {
            desiredNumberOfSwapchainImages = surfaceInfo.maxImageCount;
        }

        return swapchainPresentMode;
    }

    static std::vector<VkSurfaceFormatKHR> GetSupportedSurfaceFormats(const VkSurfaceKHR surface)
    {
        auto ctx = RenderContext::Get();
        uint32_t formatCount;
        SEDX_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->GetPhysicalDevice()->GetDevice(), surface, &formatCount, nullptr));
        std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
        SEDX_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->GetPhysicalDevice()->GetDevice(), surface, &formatCount, surfaceFormats.data()));
        return surfaceFormats;
    }

    static bool IsFormatAndColorSpaceSupported(const VkSurfaceKHR surface, VkFormat *format, VkColorSpaceKHR colorSpace)
    {
        std::vector<VkSurfaceFormatKHR> supportedFormats = GetSupportedSurfaceFormats(surface);

        // NV supports VK_FORMAT_B8G8R8A8_UNORM instead of VkFormat VK_FORMAT_B8G8R8A8_UNORM R8G8B8A8_Unorm
        /* if ((*format) == VULKAN_FORMAT[VK_FORMAT_R8G8B8A8_UNORM] &&
                 VulkanDevice::GetPrimaryPhysicalDevice()->IsNvidia())
            (*format) = VULKAN_FORMAT[VK_FORMAT_B8G8R8A8_UNORM];*/

        for (const VkSurfaceFormatKHR &supportedFormat : supportedFormats)
        {
            // Compare the format directly - the format pointer points to the VkFormat value we want to check
            bool supportFormat = supportedFormat.format == *format;
            if (bool supportColorSpace = supportedFormat.colorSpace == colorSpace; supportFormat && supportColorSpace)
                return true;
        }

        return false;
    }

    static VkCompositeAlphaFlagBitsKHR GetSupportedCompositeAlphaFormat(const VkSurfaceKHR surface)
    {
        std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
        };

        // get physical device surface capabilities
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        SEDX_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(RenderContext::GetCurrentDevice()->GetPhysicalDevice()->GetDevice(), surface, &surfaceCapabilities));

        // simply select the first composite alpha format available
        for (VkCompositeAlphaFlagBitsKHR &compositeAlpha : compositeAlphaFlags)
        {
            if (surfaceCapabilities.supportedCompositeAlpha & compositeAlpha) return compositeAlpha;
        }

        return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    }

    // -------------------------------------------------------

    /**
     * @brief Initialize the Vulkan surface for the swap chain using GLFW.
     * @param windowPtr Pointer to the GLFW window
     */
    void SwapChain::InitSurface(GLFWwindow *windowPtr)
    {
        auto ctx = RenderContext::Get();
        glfwCreateWindowSurface(ctx->GetInstance(), windowPtr, nullptr, &m_Surface); // Create the surface
    }

    /**
	 * @brief Create the swapchain.
	 */
    void SwapChain::Create()
    {
        auto ctx = RenderContext::Get(); // Get render context reference
        VkDevice device = RenderContext::GetCurrentDevice()->GetDevice();
        RenderData data;
        data.width = m_Width;
        data.height = m_Height;
        data.vSync = m_VSync;

        VkPhysicalDevice physicalDevice = ctx->GetLogicDevice()->GetPhysicalDevice()->GetDevice();
        SEDX_CORE_TRACE_TAG("Swapchain", "Got Physical Device: {}", ToString(physicalDevice));
        VkSwapchainKHR oldSwapChain = m_SwapChain;
        SEDX_CORE_TRACE_TAG("Swapchain", "Got Old Swapchain Handle: {}", ToString(oldSwapChain));

        // Get a valid graphics queue family index from the device
        if (auto queueIndices = RenderContext::GetCurrentDevice()->GetPhysicalDevice()->GetQueueFamilyIndices();
            queueIndices.GetGraphicsFamily())
        {
            m_QueueIndex = queueIndices.GetGraphicsFamily();
        }
        else
        {
            SEDX_CORE_ERROR_TAG("Swapchain", "No valid graphics queue family found!");
            return; // Or handle error appropriately
        }

        // Verify the queue index is valid before proceeding
        SEDX_CORE_ASSERT(m_QueueIndex != UINT32_MAX, "Queue family index not initialized!");

        SEDX_CORE_INFO_TAG("Swapchain", "Using queue family index: {}", m_QueueIndex);

        // Get physical device surface properties and formats
        VkSurfaceCapabilitiesKHR surfaceInfo = GetSurfaceCapabilities(static_cast<VkSurfaceKHR>(m_Surface));

        VkExtent2D swapExtent = {};
        // If width (and height) equals the special value 0xFFFFFFFF, the size of the surface will be set by the swapchain
        if (std::cmp_equal(surfaceInfo.currentExtent.width, -1) && std::cmp_equal(surfaceInfo.currentExtent.height, -1))
        {
            // If the surface size is undefined, the size is set to the size of the images requested.
            swapExtent.width = m_Width;
            swapExtent.height = m_Height;
        }
        else
        {
            swapExtent = surfaceInfo.currentExtent;
            m_Width = surfaceInfo.currentExtent.width;
            m_Height = surfaceInfo.currentExtent.height;
        }

        if (m_Width == 0 || m_Height == 0)
        {
            SEDX_CORE_TRACE_TAG("Graphics Engine", "Window minimized, waiting for restore");
            return;
        }
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_Surface, &surfaceInfo))

        // Get available present modes
        uint32_t presentModeCount;
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &presentModeCount, nullptr))
        SEDX_CORE_ASSERT(presentModeCount > 0, "No present modes available!");

        // Get available surface formats
        uint32_t formatCount;
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, nullptr))
        SEDX_CORE_ASSERT(formatCount > 0, "No surface formats available!");

        // Get the actual formats
        std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, surfaceFormats.data()))

        // Select a suitable format and color space
        bool foundSRGB = false;
        for (const auto &format : surfaceFormats)
        {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                m_Format = format.format;
                m_ColorSpace = format.colorSpace;
                foundSRGB = true;
                break;
            }
        }

        // If SRGB format not found, just use the first available format
        if (!foundSRGB)
        {
            m_Format = surfaceFormats[0].format;
            m_ColorSpace = surfaceFormats[0].colorSpace;
        }

        SEDX_CORE_INFO_TAG("Swapchain", "Selected format: {} and color space: {}", ToString(m_Format), ToString(m_ColorSpace));

        // Find the transformation of the surface
        VkSurfaceTransformFlagsKHR preTransform;
        if (surfaceInfo.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        {
            preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; // We prefer a non-rotated transform
        }
        else
        {
            preTransform = surfaceInfo.currentTransform;
        }

        // Find a supported composite alpha format (not all devices support alpha opaque)
        VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        // Simply select the first composite alpha format available
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

        // Determine the number of images based on present mode
        VkPresentModeKHR selectedPresentMode = GetPresentMode(m_Surface, m_PresentMode);
        uint32_t desiredNumberOfSwapchainImages =
            (selectedPresentMode == VK_PRESENT_MODE_MAILBOX_KHR) ? 3 : (surfaceInfo.minImageCount + 1);
        if (surfaceInfo.maxImageCount > 0 && desiredNumberOfSwapchainImages > surfaceInfo.maxImageCount)
        {
            desiredNumberOfSwapchainImages = surfaceInfo.maxImageCount;
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// SwapChain Creation																							///
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.pNext = nullptr;
        createInfo.surface = m_Surface;
        createInfo.minImageCount = desiredNumberOfSwapchainImages;
        createInfo.imageFormat = m_Format;
        createInfo.imageColorSpace = m_ColorSpace;
        createInfo.imageExtent = {.width = swapExtent.width, .height = swapExtent.height};
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.preTransform = static_cast<VkSurfaceTransformFlagBitsKHR>(preTransform);
        createInfo.compositeAlpha = compositeAlpha;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
        createInfo.clipped =
            VK_TRUE; // Setting clipped to VK_TRUE allows the implementation to discard rendering outside the surface area.

        // Workaround for enum class PresentMode naming conflict - use C-style cast through void*
        *reinterpret_cast<VkPresentModeKHR *>(&createInfo.presentMode) = selectedPresentMode;

        // Enable transfer source on swap chain images if supported
        if (surfaceInfo.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
            createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        // Enable transfer destination on swap chain images if supported
        if (surfaceInfo.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &createInfo, nullptr, &m_SwapChain))

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Destroy old swapchain if it existed																			///
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        if (createInfo.oldSwapchain != VK_NULL_HANDLE)
            vkDestroySwapchainKHR(device, createInfo.oldSwapchain, nullptr);

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Image Views																									///
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        uint32_t imageCount = 0;
        VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, m_SwapChain, &m_SwapChainImageCount, nullptr))
        VK_CHECK_RESULT(
            vkGetSwapchainImagesKHR(device, m_SwapChain, &m_SwapChainImageCount, m_SwapChainImageCounts.data()))

        // Resize the image wrapper vector to match the actual image count
        m_SwapChainImage.resize(m_SwapChainImageCount);

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Destroy old image views																						///
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        for (auto &[Image, ImageView] : m_SwapChainImage)
        {
            if (ImageView != VK_NULL_HANDLE)
                vkDestroyImageView(device, ImageView, nullptr);
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Create New Image Views																						///
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        for (uint32_t i = 0; i < m_SwapChainImageCount; i++)
        {
            VkImageViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = m_SwapChainImage[i].m_Image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = m_Format;
            viewInfo.components = {.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                                   .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                                   .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                                   .a = VK_COMPONENT_SWIZZLE_IDENTITY};
            viewInfo.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                         .baseMipLevel = 0,
                                         .levelCount = 1,
                                         .baseArrayLayer = 0,
                                         .layerCount = 1};
            VK_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &m_SwapChainImage[i].m_ImageView));
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Sync Primitives																								 ///
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        for (uint32_t i = 0; i < static_cast<uint32_t>(m_ImageAcquiredSemaphores.size()); i++)
        {
            m_ImageAcquiredSemaphores[i] = CreateRef<FrameSync>(FrameSyncType::Semaphore,
                                                                ("swapchain_" + ToString(i)).c_str(),
                                                                true,
                                                                false,
                                                                false);
        }

        // set HDR metadata only if HDR is enabled
        if (m_Format == FORMAT_HDR)
        {
            SetHdrMetadata(static_cast<VkSwapchainKHR>(m_SwapChain));
        }

		m_ImageIdx = 0;
    }

    void SwapChain::OnResize(const uint32_t width, const uint32_t height)
    {
        SEDX_CORE_INFO_TAG("Swapchain", "OnResize called: {}x{}", width, height);
        
        // Skip resize if dimensions are invalid (window minimized)
        if (m_Width == width && m_Height == height)
            return;
        
        m_Width = width;
        m_Height = height;

        // Validate device state before proceeding
        if (m_Device == nullptr || m_Device->GetDevice() == VK_NULL_HANDLE)
        {
            SEDX_CORE_ERROR_TAG("Swapchain", "Cannot resize - device is invalid");
            return;
        }

        // Recreate the swapchain with the new dimensions
        Create();
        
        SEDX_CORE_INFO_TAG("Swapchain", "✓ Swapchain resize complete: {}x{}", width, height);
    }

    /**
    * @brief Acquires the next image from the swap chain.
    *
    * This function waits for the next available image in the swap chain to be ready for rendering.
    * @note This function should be called when the application is ready to render a new frame.
    */

    // TODO: Refactor to set up integration with the FrameSync class properly and acquire next image properly.

    void SwapChain::AcquireNextImage()
    {
        // When the window is minimized acquisition will fail and it's not necessary either
        if (m_Window.IsMinimized())
            return;

        auto renderData = RenderData();

        {
            // Check 1: Validate swapchain image count (most critical - indicates swapchain initialization state)
            if (m_SwapChainImage.empty())
            {
                SEDX_CORE_ERROR_TAG("Swapchain", "Cannot acquire image - swapchain not initialized (image count is 0)");
                return;
            }

            // Check 2: Validate device pointer (prevents use-after-free)
            if (m_Device == nullptr || m_Device->GetDevice() == VK_NULL_HANDLE)
            {
                SEDX_CORE_ERROR_TAG("Swapchain", "Cannot acquire image - device is invalid (use-after-free or not initialized)");
                SEDX_CORE_ERROR_TAG("Swapchain", "Device handle: {}", ToString(m_Device ? m_Device->GetDevice() : VK_NULL_HANDLE));
                return;
            }

            // Check 3: Validate swapchain handle
            if (m_SwapChain == VK_NULL_HANDLE)
            {
                SEDX_CORE_ERROR_TAG("Swapchain", "Cannot acquire image - swapchain handle is null");
                return;
            }
        
            // Check 4: Validate synchronization primitives exist
            if (m_ImageAcquiredSemaphores.empty() || m_WaitFences.empty())
            {
                SEDX_CORE_ERROR_TAG("Swapchain", "Cannot acquire image - synchronization primitives not initialized");
                return;
            }
        }

        // All validation passed - proceed with image acquisition
        auto device = RenderContext::GetCurrentDevice();

        //m_CurrentFrameIdx = (m_CurrentFrameIdx + 1) % renderData.framesInFlight;
        //renderData.swapChainCurrentFrame = (renderData.swapChainCurrentFrame + 1) % m_SwapChainImage.size();

        // Get the current frame's synchronization object
        Ref<FrameSync> frameSync = m_ImageAcquiredSemaphores[m_ImageIdx];
        if (frameSync)
        {
            frameSync->WaitForFence(16000000);
            // VK_CHECK_RESULT(vkWaitForFences(device->GetDevice(), 1, &m_WaitFences[m_ImageIdx], VK_TRUE, UINT64_MAX))

            if (Ref<CommandManager> cmdList = frameSync->GetCmdList())
            {
                if (cmdList->GetState() == CommandState::SUBMITTED)
                {
                    SEDX_CORE_TRACE_TAG("Swapchain", "Waiting for command list fence before acquiring next image");
                    cmdList->ExecutionWait(true);
                }
            }
        }

        // Extract VkSemaphore handle from FrameSync wrapper
        VkSemaphore imageAvailableSemaphore = m_ImageAcquiredSemaphores[m_ImageIdx]->GetImageAvailableSemaphore();
        uint32_t retryCount = 0;
        constexpr uint32_t retryMax = 10;

        while (retryCount < retryMax)
        {
            int timeout = 16000000;
            VkResult result = vkAcquireNextImageKHR(device->GetDevice(), m_SwapChain, timeout, imageAvailableSemaphore, nullptr, &m_ImageIdx);
            
			if (result == VK_SUCCESS)
			{
				m_ImageAcquiredSemaphores[m_ImageIdx] = m_ImageAcquiredSemaphores[m_SemaphoreIdx];
                m_SemaphoreIdx = (m_SemaphoreIdx + 1) % m_ImageAcquiredSemaphores.size();
				SEDX_CORE_TRACE_TAG("Swapchain", "Acquired swapchain image index: {}", m_ImageIdx);
				return;
            }

            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
            {
                SEDX_CORE_WARN_TAG("Swapchain", "Swapchain out of date or suboptimal - triggering resize");
                OnResize(m_Width, m_Height);
            
                // Try acquiring again after resize - extract semaphore handle again
                VkSemaphore retryImageAvailableSemaphore = m_ImageAcquiredSemaphores[m_ImageIdx]->GetImageAvailableSemaphore();
                VK_CHECK_RESULT(vkAcquireNextImageKHR(device->GetDevice(), m_SwapChain, timeout, retryImageAvailableSemaphore, nullptr, &m_ImageIdx))
            }
            else if (result == VK_NOT_READY || result == VK_SUBOPTIMAL_KHR)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                retryCount++;
            }
            else if (result == VK_ERROR_OUT_OF_DATE_KHR)
            {
                Create();
            }
            else
            {
                SEDX_ASSERT(result);
                SEDX_CORE_ERROR_TAG("Swapchain", "Failed to acquire swapchain image: {}", ToString(result));
            }
        }
    }

    void SwapChain::Present()
    {
        // When the window is minimized acquisition will fail and it's not necessary either
        if (m_Window.IsMinimized())
            return;

        auto device = RenderContext::GetCurrentDevice();
        constexpr VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        // Extract VkSemaphore handles from FrameSync wrappers
        const Ref<FrameSync> &currentFrameSync = m_ImageAcquiredSemaphores[m_CurrentFrameIdx];
        const Ref<FrameSync> &renderFrameSync = m_RenderFinishedSemaphores[m_CurrentFrameIdx];

        VkSemaphore waitSemaphore = currentFrameSync->GetImageAvailableSemaphore();
        VkSemaphore signalSemaphore = renderFrameSync->GetRenderFinishedSemaphore();
        VkFence inFlightFence = m_WaitFences[m_CurrentFrameIdx];

        // Validate synchronization primitives
        SEDX_CORE_ASSERT(waitSemaphore != VK_NULL_HANDLE, "Wait semaphore is invalid for frame {}", m_CurrentFrameIdx);
        SEDX_CORE_ASSERT(signalSemaphore != VK_NULL_HANDLE, "Signal semaphore is invalid for frame {}", m_CurrentFrameIdx);
        SEDX_CORE_ASSERT(inFlightFence != VK_NULL_HANDLE, "In-flight fence is invalid for frame {}", m_CurrentFrameIdx);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pWaitDstStageMask = &waitStageMask;
        submitInfo.pWaitSemaphores = &waitSemaphore;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &signalSemaphore;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pCommandBuffers = &m_CmdBuffers[m_CurrentFrameIdx].m_CommandBuffer;
        submitInfo.commandBufferCount = 1;
        
        // Submit work to GPU
        VK_CHECK_RESULT(vkResetFences(device->GetDevice(), 1, &inFlightFence))
        device->LockQueue();
        VK_CHECK_RESULT(vkQueueSubmit(device->GetGraphicsQueue(), 1, &submitInfo, inFlightFence))

        // Present to swapchain - reuse the same signalSemaphore variable
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext = nullptr;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_SwapChain;
        presentInfo.pImageIndices = &m_ImageIdx;
        presentInfo.pWaitSemaphores = &signalSemaphore; // Reuse variable instead of re-extracting
        presentInfo.waitSemaphoreCount = 1;

        VkResult result = vkQueuePresentKHR(device->GetGraphicsQueue(), &presentInfo);
        device->UnlockQueue();

        // Handle swapchain recreation scenarios
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            SEDX_CORE_WARN_TAG("Swapchain", "Swapchain out of date or suboptimal - triggering resize");
            OnResize(m_Width, m_Height);
        }
        else if (result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("VULKAN CORE", "Failed to present swap chain image: {}", ToString(result));
            VK_CHECK_RESULT(result)
        }
    }

    void SwapChain::Destroy()
    {
        SEDX_CORE_INFO_TAG("Swapchain", "=== Beginning Swapchain Destruction ===");
        
        auto devRef = RenderContext::GetCurrentDevice();
        VkDevice device = devRef ? devRef->GetDevice() : VK_NULL_HANDLE;

        // Destroy presentation swapchain and related images/views
        if (m_SwapChain)
        {
            vkDestroySwapchainKHR(device, static_cast<VkSwapchainKHR>(m_SwapChain), nullptr);
            m_SwapChain = nullptr;
        }

        // Synchronization primitives
        SEDX_CORE_TRACE_TAG("Swapchain", "Destroying synchronization primitives");
        
        // FrameSync objects are managed by Ref<> smart pointers and will be automatically destroyed
        // when the std::array goes out of scope or is reset. We don't need to manually destroy
        // the underlying VkSemaphore handles as FrameSync destructor handles that.
        // Note: std::array doesn't have a clear() method, elements are destroyed automatically
        
        // However, we can explicitly reset each Ref to trigger cleanup
        for (auto &frameSync : m_ImageAcquiredSemaphores)
        {
            frameSync.Reset();
        }
        
        for (auto &frameSync : m_RenderFinishedSemaphores)
        {
            frameSync.Reset();
        }

        for (const auto &fence : m_WaitFences)
        {
            if (fence != VK_NULL_HANDLE)
                vkDestroyFence(device, fence, nullptr);
        }
        m_WaitFences.clear();

        // Destroy any auxiliary images if they were created
        if (m_DepthImageView != VK_NULL_HANDLE)
        {
            SEDX_CORE_TRACE_TAG("Swapchain", "Destroying depth resources");
            vkDestroyImageView(device, m_DepthImageView, nullptr);
            m_DepthImageView = VK_NULL_HANDLE;
        }
        if (m_DepthImage != VK_NULL_HANDLE)
        {
            vkDestroyImage(device, m_DepthImage, nullptr);
            m_DepthImage = VK_NULL_HANDLE;
        }
        if (m_DepthImageMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, m_DepthImageMemory, nullptr);
            m_DepthImageMemory = VK_NULL_HANDLE;
        }
        if (m_ColorImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device, m_ColorImageView, nullptr);
            m_ColorImageView = VK_NULL_HANDLE;
        }
        if (m_ColorImage != VK_NULL_HANDLE)
        {
            vkDestroyImage(device, m_ColorImage, nullptr);
            m_ColorImage = VK_NULL_HANDLE;
        }
        if (m_ColorImageMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, m_ColorImageMemory, nullptr);
            m_ColorImageMemory = VK_NULL_HANDLE;
        }
        if (m_TextureImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device, m_TextureImageView, nullptr);
            m_TextureImageView = VK_NULL_HANDLE;
        }
        if (m_TextureSampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(device, m_TextureSampler, nullptr);
            m_TextureSampler = VK_NULL_HANDLE;
        }
        if (m_TextureImage != VK_NULL_HANDLE)
        {
            vkDestroyImage(device, m_TextureImage, nullptr);
            m_TextureImage = VK_NULL_HANDLE;
        }
        if (m_TextureImageMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, m_TextureImageMemory, nullptr);
            m_TextureImageMemory = VK_NULL_HANDLE;
        }

        vkDeviceWaitIdle(device);

        // Finally, destroy the VkSurfaceKHR if it exists (must happen before instance destruction)
        if (m_Surface != VK_NULL_HANDLE)
        {
            if (VkInstance inst = RenderContext::GetInstance(); inst != VK_NULL_HANDLE)
				vkDestroySurfaceKHR(inst, m_Surface, nullptr);
            m_Surface = VK_NULL_HANDLE;
        }

    }

    void SwapChain::BeginFrame()
    {
        // **FIX**: Add defensive checks before accessing m_vkDevice to prevent use-after-free crashes
        if (m_Device == nullptr || m_Device->GetDevice() == VK_NULL_HANDLE)
        {
            SEDX_CORE_ERROR_TAG("Swapchain", "Cannot begin frame - device is invalid (use-after-free or not initialized)");
            return;
        }

        if (m_CmdBuffers.empty())
        {
            SEDX_CORE_ERROR_TAG("Swapchain", "Cannot begin frame - command buffers not created");
            return;
        }

        // Advance and execute deferred resource free jobs now that GPU has finished previous frame
        RenderDispatcher::NextFrame(m_CurrentFrameIdx);

        AcquireNextImage();
        VK_CHECK_RESULT(vkResetCommandPool(m_Device->GetDevice(), m_CmdBuffers[m_CurrentFrameIdx].m_CommandPool, 0))
    }

    VkSurfaceFormatKHR SwapChain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
    {
        for (const auto &availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return availableFormat;
        }

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
        auto ctx = RenderContext::Get(); // Get render context reference
        auto device = ctx->GetCurrentDevice();

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

        if (vkCreateImage(device->GetDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS)
            SEDX_CORE_ERROR("Failed to create image!");

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device->GetDevice(), image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device->GetDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
            SEDX_CORE_ERROR("Failed to allocate image memory!");

        vkBindImageMemory(device->GetDevice(), image, imageMemory, 0);
    }

    VkFramebuffer SwapChain::GetFramebuffer(uint32_t index) const
    {
        SEDX_CORE_ASSERT(index < m_SwapChainFramebuffers.size());
        return m_SwapChainFramebuffers[index];
    }

    VkCommandBuffer SwapChain::GetDrawCommandBuffer(uint32_t index) const
    {
        SEDX_CORE_ASSERT(index < m_CmdBuffers.size());
        return m_CmdBuffers[index].m_CommandBuffer;
    }

    VkImage SwapChain::GetSwapchainImage(uint32_t index) const
    {
        SEDX_CORE_ASSERT(index < m_SwapChainImage.size()); return m_SwapChainImage[index].m_Image;
    }

    VkImage SwapChain::GetActiveImage() const
    {
        SEDX_CORE_ASSERT(m_CurrentFrameIdx < m_SwapChainImage.size()); return m_SwapChainImage[m_CurrentFrameIdx].m_Image;
    }

    VkImageView SwapChain::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const
    {
        auto device = RenderContext::GetCurrentDevice();

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
        if (vkCreateImageView(device->GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
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
        CreateImage(m_SwapChainExtent.width, m_SwapChainExtent.height, 1, renderData.msaaSamples, depthFormat,
                    VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    m_DepthImage, m_DepthImageMemory);
        m_DepthImageView = CreateImageView(m_DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    }

    /*
    VkPresentModeKHR SwapChain::ChooseSwapPresentMode() const
    {
        //auto ctx = RenderContext::Get();
        const auto physDevice = RenderContext::GetCurrentDevice()->GetPhysicalDevice();

		// Make sure we have valid present modes before accessing them
		if (presentModes.empty())
            SEDX_CORE_WARN_TAG("Swapchain", "No present modes available, defaulting to VK_PRESENT_MODE_FIFO_KHR");

        for (const auto& presentMode : presentModes)
		{
		    if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		        return presentMode;
		}

		if (!m_VSync)
		{
		    for (const auto& presentMode : presentModes)
		    {
		        if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		            return presentMode;
		    }
		}

        return VK_PRESENT_MODE_FIFO_KHR; // If VSync is enabled, use FIFO
    }
    */

	VkFormat SwapChain::FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
    {
        for (const VkFormat format : candidates)
        {
            //auto ctx = RenderContext::Get(); // Get render context reference
            auto physDevice = RenderContext::GetCurrentDevice()->GetPhysicalDevice()->GetDevice();
            VkFormatProperties props;

            vkGetPhysicalDeviceFormatProperties(physDevice, format, &props);
            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
                return format;

            if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
                return format;
        }

        SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to find supported format!");
        return VK_FORMAT_UNDEFINED; // Return a default or invalid format
    }

    VkExtent2D SwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, uint32_t width, uint32_t height)
    {
        // If the current extent width is the max value, it means the window manager allows us to set dimensions other than the current window size
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            SEDX_CORE_INFO("Using surface extent: {}x{}", capabilities.currentExtent.width, capabilities.currentExtent.height);
            return capabilities.currentExtent;
        }

        // Get the actual framebuffer size from GLFW directly for consistency
        VkExtent2D actualExtent = {width, height};

        actualExtent.width = xMath::Max(capabilities.minImageExtent.width,
                                        xMath::Max(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = xMath::Max(capabilities.minImageExtent.height,
                                         xMath::Max(capabilities.maxImageExtent.height, actualExtent.height));

        SEDX_CORE_INFO("Using calculated extent: {}x{}", actualExtent.width, actualExtent.height);

        return actualExtent;
    }

    /**
     * @brief Query the swap chain support details for the device.
     *
     * @param device - The Vulkan device to query.
     * @return - The swap chain support details.
     */
    SwapChainDetails SwapChain::QuerySwapChainSupport(const VulkanDevice *device)
    {
        SwapChainDetails details;
        details.formats		 = device->GetPhysicalDevice()->GetSurfaceFormats();
        details.presentModes = device->GetPhysicalDevice()->GetPresentModes();
        details.capabilities = device->GetPhysicalDevice()->GetSurfaceCapabilities();

        return details;
    }

	/**
	 * @brief Find the image format and color space for the swap chain.
	 */
    void SwapChain::FindImageFormatAndColorSpace()
    {
        auto ctx = RenderContext::Get(); // Get render context reference
        VkPhysicalDevice physDevice = ctx->GetLogicDevice()->GetPhysicalDevice()->GetDevice();

        // Get list of supported surface formats
        uint32_t formatCount;
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, m_Surface, &formatCount, nullptr))
        SEDX_CORE_ASSERT(formatCount > 0);

		std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, m_Surface, &formatCount, surfaceFormats.data()))
		if (formatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
		{
			m_Format = VK_FORMAT_B8G8R8A8_UNORM;
			m_ColorSpace = surfaceFormats[0].colorSpace;
		}
		else
		{
			// Iterate over the list of available surface format and check for the presence of VK_FORMAT_B8G8R8A8_UNORM.
			bool foundRequestedFormat = false;
            for (auto &&surfaceFormat : surfaceFormats)
			{
                if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
				{
                    m_Format = surfaceFormat.format;
                    m_ColorSpace = surfaceFormat.colorSpace;
					foundRequestedFormat = true;
					break;
				}
			}

			// In case VK_FORMAT_B8G8R8A8_UNORM is not available select the first available color format.
			if (!foundRequestedFormat)
			{
				m_Format = surfaceFormats[0].format;
				m_ColorSpace = surfaceFormats[0].colorSpace;
			}
			}
    }

    /**
     * @brief Initialize the SwapChain with Vulkan instance and device references.
     * @param instance The Vulkan instance
     * @param device The Vulkan logical device
     * 
     * This method must be called before Create() to ensure m_vkDevice is properly initialized.
     */
    void SwapChain::Init(const Ref<VulkanDevice> &device)
    {
        SEDX_CORE_INFO_TAG("Swapchain", "Initializing SwapChain with device: {}", ToString(device->GetDevice()));
        
        // Store the device reference - this is critical to prevent use-after-free
        m_Device = device;
        
        // Validate device is properly initialized
        SEDX_CORE_ASSERT(m_Device != nullptr, "VulkanDevice cannot be null");
        SEDX_CORE_ASSERT(m_Device->GetDevice() != VK_NULL_HANDLE, "VulkanDevice handle must be valid");
        
        SEDX_CORE_INFO_TAG("Swapchain", "SwapChain initialization complete");
    }

}

// -------------------------------------------------------


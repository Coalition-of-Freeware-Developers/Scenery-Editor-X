/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * swapchain.cpp
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#include "swapchain.h"
#include "render_context.h"
#include "SceneryEditorX/core/window/window.h"
#include <iostream>
#include <utility>
#include <vector>
#include <SDL3/SDL_vulkan.h>
#include <tracy/Tracy.hpp>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

    //static VkSurfaceKHR s_Surface = VK_NULL_HANDLE;

    // -------------------------------------------------------

    Swapchain::Swapchain()
    {
       m_Device = RenderContext::Get()->GetDevice();


        VkSurfaceKHR surface = VK_NULL_HANDLE;

       SDL_Window *sdlWindow = Window::GetWindow();
       if (!sdlWindow)
       {
           SEDX_CORE_ERROR_TAG("Swapchain", "SDL window is null — cannot create Vulkan surface");
           return;
       }

       VkInstance instance = RenderContext::Get()->GetInstance();
       if (instance == VK_NULL_HANDLE)
       {
           SEDX_CORE_ERROR_TAG("Swapchain", "Vulkan instance is null — cannot create Vulkan surface");
           return;
       }

       // SDL3's SDL_Vulkan_CreateSurface returns bool, not VkResult
       if (!SDL_Vulkan_CreateSurface(sdlWindow, instance, nullptr, &surface))
       {
           SEDX_CORE_ERROR_TAG("Swapchain", "Failed to create Vulkan surface: {}", SDL_GetError());
           return;
       }

       m_Surface = surface;

       SEDX_CORE_INFO_TAG("Swapchain", "Vulkan surface created successfully");
    }

    VkSwapchainKHR Swapchain::Create(VkSurfaceKHR surface, uint32_t queueFamilyIndex, VmaAllocator allocator)
	{
		// Query surface formats and pick a reasonable default.
		uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_Device->GetPhysicalDevice(), surface, &formatCount, nullptr);
		if (formatCount == 0)
		{
			SEDX_CORE_ERROR_TAG("Swapchain", "No surface formats available");
			return VK_NULL_HANDLE;
		}

		std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_Device->GetPhysicalDevice(), surface, &formatCount, formats.data());
		VkSurfaceFormatKHR surfaceFormat = formats[0];

		for (auto &f : formats)
		{
			if (f.format == VK_FORMAT_B8G8R8A8_SRGB)
			{
			    surfaceFormat = f; break;
			}
		}
	
		VkSurfaceCapabilitiesKHR caps{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Device->GetPhysicalDevice(), surface, &caps);
	
		// Verify the selected queue family supports presentation to this surface.
		VkBool32 presentSupported = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_Device->GetPhysicalDevice(), queueFamilyIndex, surface, &presentSupported);
		if (!presentSupported)
		{
			SEDX_CORE_ERROR_TAG("Swapchain", "Selected queue family does not support presentation");
			return VK_NULL_HANDLE;
		}
	
		// (no debug prints)
		VkExtent2D extent = caps.currentExtent;
		if (std::cmp_equal(extent.width, -1))
		{
		    extent = {.width = 640, .height = 480};
		}
	
		uint32_t imageCount = caps.minImageCount + 1;
		if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
		{
		    imageCount = caps.maxImageCount;
		}
	
		VkSwapchainCreateInfoKHR ci{};
		ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		ci.surface = surface;
		ci.minImageCount = imageCount;
		ci.imageFormat = surfaceFormat.format;
		ci.imageColorSpace = surfaceFormat.colorSpace;
		ci.imageExtent = extent;
		ci.imageArrayLayers = 1;
		ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ci.preTransform = caps.currentTransform;
		ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		ci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
		ci.clipped = VK_TRUE;

		/** 
		 * If we already have a swapchain, pass it as 'oldSwapchain' to the
		 * create info so the implementation can recycle resources safely.
		 */
		VkSwapchainKHR oldSwap = m_Swapchain;
		if (oldSwap != VK_NULL_HANDLE)
		{
			ci.oldSwapchain = oldSwap;
		}
	
		VkSwapchainKHR newSwap = VK_NULL_HANDLE;
        if (VkResult r = vkCreateSwapchainKHR(m_Device->GetLogicalDevice(), &ci, nullptr, &newSwap); r != VK_SUCCESS)
		{
            SEDX_CORE_ERROR_TAG("Swapchain","Failed to create swapchain: {}", r);
			return VK_NULL_HANDLE;
		}
	
		// Fetch images for the new swapchain first
		uint32_t imgCount = 0;
		vkGetSwapchainImagesKHR(m_Device->GetLogicalDevice(), newSwap, &imgCount, nullptr);
		std::vector<VkImage> newImages(imgCount);
		vkGetSwapchainImagesKHR(m_Device->GetLogicalDevice(), newSwap, &imgCount, newImages.data());
	
		// Create image views for the new images
		std::vector<VkImageView> newImageViews(imgCount, VK_NULL_HANDLE);
		for (uint32_t i = 0; i < imgCount; ++i)
		{
			VkImageViewCreateInfo viewCI{};
			viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewCI.image = newImages[i];
			viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewCI.format = surfaceFormat.format;
			viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewCI.subresourceRange.levelCount = 1;
			viewCI.subresourceRange.layerCount = 1;
			SEDX_VK_RESULT_ASSERT(vkCreateImageView(m_Device->GetLogicalDevice(), &viewCI, nullptr, &newImageViews[i]));
		}
	
		// Create a new depth image for the new extent
		VkImage newDepthImage = VK_NULL_HANDLE;
		VmaAllocation newDepthAlloc = VK_NULL_HANDLE;
		VkImageView newDepthView = VK_NULL_HANDLE;

		VkImageCreateInfo depthImageCI{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = m_DepthFormat,
			.extent{.width = extent.width, .height = extent.height, .depth = 1 },
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};

		VmaAllocationCreateInfo allocCI{
		    .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		    .usage = VMA_MEMORY_USAGE_AUTO };
        SEDX_VK_RESULT_ASSERT(vmaCreateImage(allocator, &depthImageCI, &allocCI, &newDepthImage, &newDepthAlloc, nullptr));

		VkImageViewCreateInfo depthViewCI{};
		depthViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		depthViewCI.image = newDepthImage;
		depthViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthViewCI.format = m_DepthFormat;
		depthViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		depthViewCI.subresourceRange.levelCount = 1;
		depthViewCI.subresourceRange.layerCount = 1;
		SEDX_VK_RESULT_ASSERT(vkCreateImageView(m_Device->GetLogicalDevice(), &depthViewCI, nullptr, &newDepthView));
	
		// At this point the new swapchain and its images/views/depth exist. Now
		// we can safely destroy old resources (if any) and update our members.
		if (oldSwap != VK_NULL_HANDLE)
		{
			// Destroy old image views
			for (auto &iv : m_ImageViews)
			{
			    if (iv != VK_NULL_HANDLE) vkDestroyImageView(m_Device->GetLogicalDevice(), iv, nullptr);
			}

			// Destroy old depth resources
			if (m_DepthView != VK_NULL_HANDLE)
			{
			    vkDestroyImageView(m_Device->GetLogicalDevice(), m_DepthView, nullptr); m_DepthView = VK_NULL_HANDLE;
			}

			if (m_DepthImage != VK_NULL_HANDLE)
			{
			    vmaDestroyImage(MemoryAllocator::GetAllocator(), m_DepthImage, m_DepthAlloc); m_DepthImage = VK_NULL_HANDLE; m_DepthAlloc = VK_NULL_HANDLE;
			}

			// Destroy old swapchain handle
			if (m_Swapchain != VK_NULL_HANDLE)
			{
			    vkDestroySwapchainKHR(m_Device->GetLogicalDevice(), m_Swapchain, nullptr);
			}
		}
	
		// Tick internal state to the newly created resources
		m_Swapchain = newSwap;
		m_Images = std::move(newImages);
		m_ImageViews = std::move(newImageViews);
		m_ImageFormat = surfaceFormat.format;
		m_Extent = extent;
		m_DepthImage = newDepthImage;
		m_DepthAlloc = newDepthAlloc;
		m_DepthView = newDepthView;
	
		// Creation Succeeded
        m_ImageIndex = 0;
        m_ImageAcquired = false;
		return m_Swapchain;
	}

    VkSwapchainKHR Swapchain::Recreate(VkSurfaceKHR surface, uint32_t queueFamilyIndex, VmaAllocator allocator)
	{
        Ref<Device> device = RenderContext::Get()->GetDevice();
		// Centralized recreation flow:
		//  1. Wait for device idle to ensure no commands reference swapchain resources.
		//  2. Query surface capabilities (create() will also query internally but
		//     keeping the wait and capability refresh here keeps caller simpler).
		//  3. Call create() which will create the new swapchain and swap resources.
		// The create() implementation already handles passing the old swapchain
		// through the create-info, and replaces resources after the new ones are
		// successfully created.
		vkDeviceWaitIdle(device->GetLogicalDevice());
		VkSurfaceCapabilitiesKHR caps{};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->GetPhysicalDevice(), surface, &caps);
		(void)caps; // currently unused here but helpful for future policies
		return Create(surface, queueFamilyIndex, allocator);
	}

    bool Swapchain::AcquireNextImage(VkSemaphore imageAvailableSemaphore)
    {
        // Reset acquisition state
        m_ImageAcquired = false;

        // When the window is minimized acquisition will fail and it's not necessary either
        if (Window::IsMinimized())
            return false;

        // Ensure swapchain is valid
        if (!m_Swapchain)
            return false;

        // Try to acquire, with retry after swapchain recreation
        for (uint32_t attempt = 0; attempt < 2; ++attempt)
        {
            VkResult r = vkAcquireNextImageKHR(m_Device->GetLogicalDevice(),
                                               m_Swapchain,
                                               UINT64_MAX,
                                               imageAvailableSemaphore,
                                               VK_NULL_HANDLE,
                                               &m_ImageIndex);

            if (r == VK_SUCCESS || r == VK_SUBOPTIMAL_KHR)
            {
                m_ImageAcquired = true;
                return true;
            }

            if (r == VK_ERROR_OUT_OF_DATE_KHR)
            {
                // Swapchain is out of date (e.g. window resized).
                // Recreate the swapchain and try acquiring again.
                Recreate(m_Surface,
                         m_Device->GetQueueManager()->GetFamilyIndexByType(Graphics),
                         MemoryAllocator::GetAllocator());
            }
            else
            {
                SEDX_CORE_ERROR_TAG("Swapchain", "Failed to acquire swapchain image: {}", r);
                return false;
            }
        }

        return false;
    }

    VkResult Swapchain::Present(VkQueue presentQueue, uint32_t imageIndex, VkSemaphore waitSemaphore)
    {
        VkSwapchainKHR swapchain = m_Swapchain;

        VkPresentInfoKHR presentInfo{.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                                     .waitSemaphoreCount = (waitSemaphore != VK_NULL_HANDLE) ? 1u : 0u,
                                     .pWaitSemaphores = (waitSemaphore != VK_NULL_HANDLE) ? &waitSemaphore : nullptr,
                                     .swapchainCount = 1,
                                     .pSwapchains = &swapchain,
                                     .pImageIndices = &imageIndex};

        VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            SEDX_CORE_INFO_TAG("Swapchain", "Swapchain out of date or suboptimal during present — recreation needed");
        }
        else if (result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("Swapchain", "vkQueuePresentKHR failed: {}", static_cast<int>(result));
        }

        return result;
    }

    void Swapchain::Destroy()
	{
        Ref<Device> device = RenderContext::Get()->GetDevice(); // Avoid passing VkDevice and just fetch it from the RenderContext singleton
		if (m_DepthView != VK_NULL_HANDLE)
		{
		    vkDestroyImageView(device->GetLogicalDevice(), m_DepthView, nullptr); m_DepthView = VK_NULL_HANDLE;
		}

		if (m_DepthImage != VK_NULL_HANDLE)
		{
		    vmaDestroyImage(MemoryAllocator::GetAllocator(), m_DepthImage, m_DepthAlloc); m_DepthImage = VK_NULL_HANDLE; m_DepthAlloc = VK_NULL_HANDLE;
		}

		for (auto &iv : m_ImageViews)
		{
		    if (iv != VK_NULL_HANDLE)
		    {
		        vkDestroyImageView(device->GetLogicalDevice(), iv, nullptr);
		    }
		}

		m_ImageViews.clear();
		m_Images.clear();

		if (m_Swapchain != VK_NULL_HANDLE)
		{
		    vkDestroySwapchainKHR(device->GetLogicalDevice(), m_Swapchain, nullptr);
		    m_Swapchain = VK_NULL_HANDLE;
		}

		if (m_Surface != VK_NULL_HANDLE)
		{
			vkDestroySurfaceKHR(device->GetInstance(), m_Surface, nullptr);
			m_Surface = VK_NULL_HANDLE;
        }
	}

}

// -------------------------------------------------------

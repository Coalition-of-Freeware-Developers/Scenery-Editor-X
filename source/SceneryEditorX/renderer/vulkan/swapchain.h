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
 * swapchain.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "device.h"
#include <vector>
#include <vma/vk_mem_alloc.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
    class Window;

    class Swapchain : public RefCounted
	{
	public:
	    Swapchain();
        ~Swapchain() = default;
	
	    // Create the swapchain and associated image views and depth buffer.
	    // Returns the created VkSwapchainKHR or VK_NULL_HANDLE on failure.
	    void Create(VkSurfaceKHR surface, uint32_t queueFamilyIndex, VmaAllocator allocator);

	    // Recreate the swapchain (destroys previous images/views/depth and creates new ones).
	    // Recreate the swapchain: waits for m_Device idle, refreshes surface caps,
	    // creates a new swapchain and replaces internal resources safely.
        void Recreate(VkSurfaceKHR surface, uint32_t queueFamilyIndex, VmaAllocator allocator);
        bool AcquireNextImage(VkSemaphore imageAvailableSemaphore);
        VkResult Present(VkQueue presentQueue, uint32_t imageIndex, VkSemaphore waitSemaphore);
	    void Destroy();
	
	    // Accessors
        [[nodiscard]] VkSwapchainKHR Get() const { return m_Swapchain; }
        [[nodiscard]] VkImage GetDepthImage() const { return m_DepthImage; }
	    [[nodiscard]] VmaAllocation GetDepthAllocation() const { return m_DepthAlloc; }
	    [[nodiscard]] VkImageView GetDepthView() const { return m_DepthView; }
	    [[nodiscard]] VkFormat GetImageFormat() const { return m_ImageFormat; }
	    [[nodiscard]] VkFormat GetDepthFormat() const { return m_DepthFormat; }
	    [[nodiscard]] VkExtent2D GetExtent() const { return m_Extent; }
	    [[nodiscard]] uint32_t GetImageIndex() const { return m_ImageIndex; }
        [[nodiscard]] VkSurfaceKHR GetSurface() const { return m_Surface; }

		std::vector<VkImage> &Images() { return m_Images; }
        std::vector<VkImageView> &ImageViews() { return m_ImageViews; }

	private:
        Ref<Device> m_Device = nullptr;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
	    VkSwapchainKHR m_Swapchain{ VK_NULL_HANDLE };
	    std::vector<VkImage> m_Images;
	    std::vector<VkImageView> m_ImageViews;
	    VkImage m_DepthImage{ VK_NULL_HANDLE };
	    VmaAllocation m_DepthAlloc{ VK_NULL_HANDLE };
	    VkImageView m_DepthView{ VK_NULL_HANDLE };
	    VkFormat m_ImageFormat{ VK_FORMAT_B8G8R8A8_SRGB };
	    VkFormat m_DepthFormat{ VK_FORMAT_D24_UNORM_S8_UINT };
	    VkExtent2D m_Extent{ 0, 0 };
        uint32_t m_ImageIndex = 0;
		bool m_ImageAcquired = false;
	};

}

// -------------------------------------------------------

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
		virtual ~Swapchain() override;

		void CreateSwapchain();

		// Recreate the swapchain (destroys previous images/views/depth and creates new ones).
		// Recreate the swapchain: waits for m_Device idle, refreshes surface caps,
		// creates a new swapchain and replaces internal resources safely.
		void Recreate();
		void Resize(const uint32_t width, const uint32_t height);
		void AcquireNextImage();
		VkResult Present(VkQueue presentQueue, uint32_t imageIndex, VkSemaphore waitSemaphore);
		void SetVsync(bool enabled);
		bool GetVsync() const;

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
		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }

		std::vector<VkImage> &GetImages() { return m_Images; }
		std::vector<VkImageView> &GetImageViews() { return m_ImageViews; }

	private:
		Ref<Device> m_Device = nullptr;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
		bool m_IsDirty = false;
		uint32_t m_ImageIndex = 0;
		uint32_t m_Width = 0;
		uint32_t m_Height = 0;
		uint32_t m_SemaphoreIndex = 0;
		VkPresentModeKHR m_PresentMode = VK_PRESENT_MODE_FIFO_KHR;

		static Scope<FrameSync> s_FrameSync;
		std::vector<Ref<FrameSync>> m_AcquiredSemaphore;
		std::vector<Ref<FrameSync>> m_CompleteSemaphore;
		std::vector<VkImage> m_Images;
		std::vector<VkImageView> m_ImageViews;

		VkImage m_DepthImage{ VK_NULL_HANDLE };
		VmaAllocation m_DepthAlloc{ VK_NULL_HANDLE };
		VkImageView m_DepthView{ VK_NULL_HANDLE };
		VkFormat m_ImageFormat{ VK_FORMAT_B8G8R8A8_SRGB };
		VkFormat m_DepthFormat{ VK_FORMAT_D24_UNORM_S8_UINT };
		VkExtent2D m_Extent{ 0, 0 };
		
		bool m_ImageAcquired = false;
	};

}

// -------------------------------------------------------

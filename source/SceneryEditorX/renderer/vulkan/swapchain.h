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
#include <SDL3/SDL_video.h>
#include <SceneryEditorX/core/identifiers/flag.h>
#include <SceneryEditorX/renderer/ui/ui_impl.h>
#include <vma/vk_mem_alloc.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Window;
	class CommandList;
	class FrameSync;

	struct SwapchainSpec
	{
		SDL_Window *sdlWindow;
		uint32_t width;
		uint32_t height;
		VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
		uint32_t bufferCount;
		const char *name;
	};

	/**
	 * @class Swapchain
	 * @brief Manages the Vulkan swapchain, including images, image views, depth resources, and synchronization.
	 */
	class Swapchain : public RefCounted
	{
	public:
		/**
		 * @brief Default constructor for the Swapchain class.
		 */
		Swapchain();

		/**
		 * @brief Constructs a Swapchain with the specified configuration.
		 * @param spec The specifications for the swapchain.
		 */
		Swapchain(const SwapchainSpec &spec);

		/**
		 * @brief Destroys the Vulkan swapchain and associated resources.
		 */
		virtual ~Swapchain() override;

		/**
		 * @brief Creates the Vulkan swapchain and associated resources based on the current surface capabilities and specifications.
		 */
		void CreateSwapchain();

		/**
		 * @brief Recreates the swapchain, including all associated resources. 
		 * This should be called when the swapchain becomes invalid (e.g. window resize) or when presentation fails 
		 * due to an out-of-date swapchain. 
		 * The method will wait for the device to be idle before proceeding with recreation to ensure no commands are 
		 * referencing old swapchain resources. 
		 * It will also query the surface capabilities to ensure the new swapchain is created with valid parameters.
		 */
		void Recreate();

		/**
		 * @brief Resizes the swapchain to the specified dimensions.
		 * @param width The new width of the swapchain.
		 * @param height The new height of the swapchain.
		 */
		void Resize(const uint32_t width, const uint32_t height);

		/**
		 * @brief Acquires the next available image from the swapchain for rendering.
		 */
		void AcquireNextImage();

		/**
		 * @brief Presents the rendered image to the swapchain.
		 * @param cmdList The command list used for rendering.
		 */
		void Present(CommandList *cmdList);

		/**
		 * @brief Presents the rendered image to the swapchain using the specified Vulkan queue and synchronization semaphore.
		 * @param presentQueue The Vulkan queue used for presentation.
		 * @param imageIndex The index of the image to present.
		 * @param waitSemaphore The semaphore to wait on before presenting.
		 * @return The result of the presentation operation.
		 */
		VkResult Present(VkQueue presentQueue, uint32_t imageIndex, VkSemaphore waitSemaphore);

		/**
		 * @brief Enables or disables vertical synchronization (VSync) for the swapchain.
		 * @param enabled True to enable VSync, false to disable.
		 */
		void SetVsync(bool enabled);

		/**
		 * @brief Retrieves the current vertical synchronization (VSync) state of the swapchain.
		 * @return True if VSync is enabled, false otherwise.
		 */
		bool GetVsync() const;

		/**
		 * @brief Checks if an image has been acquired from the swapchain.
		 * @return True if an image has been acquired, false otherwise.
		 */
		bool IsImageAcquired() const { return m_ImageAcquired; }

		/**
		 * @brief Retrieves the Vulkan swapchain handle.
		 * @return the Vulkan swapchain handle.
		 */
		[[nodiscard]] VkSwapchainKHR Get() const { return m_Swapchain; }

		/**
		 * @brief Retrieves the Vulkan depth image handle.
		 * @return the Vulkan depth image handle.
		 */
		[[nodiscard]] VkImage GetDepthImage() const { return m_DepthImage; }

		/**
		 * @brief Retrieves the Vulkan depth image allocation.
		 * @return the Vulkan depth image allocation.
		 */
		[[nodiscard]] VmaAllocation GetDepthAllocation() const { return m_DepthAlloc; }

		/**
		 * @brief Retrieves the Vulkan depth image view.
		 * @return the Vulkan depth image view.
		 */
		[[nodiscard]] VkImageView GetDepthView() const { return m_DepthView; }

		/**
		 * @brief Retrieves the Vulkan image format.
		 * @return the Vulkan image format.
		 */
		[[nodiscard]] VkFormat GetImageFormat() const { return m_ImageFormat; }

		/**
		 * @brief Retrieves the Vulkan depth format.
		 * @return the Vulkan depth format.
		 */
		[[nodiscard]] VkFormat GetDepthFormat() const { return m_DepthFormat; }

		/**
		 * @brief Retrieves the Vulkan swapchain extent.
		 * @return the Vulkan swapchain extent.
		 */
		[[nodiscard]] VkExtent2D GetExtent() const { return m_Extent; }

		/**
		 * @brief Retrieves the index of the current image in the swapchain.
		 * @return the index of the current image.
		 */
		[[nodiscard]] uint32_t GetImageIndex() const { return m_ImageIndex; }

		/**
		 * @brief Retrieves the Vulkan semaphore that was signaled by the latest successful image acquisition.
		 * @return The Vulkan semaphore handle used for acquire synchronization, or VK_NULL_HANDLE if unavailable.
		 */
		[[nodiscard]] VkSemaphore GetAcquiredVkSemaphore() const;

		/**
		 * @brief Retrieves the Vulkan surface handle.
		 * @return the Vulkan surface handle.
		 */
		[[nodiscard]] VkSurfaceKHR GetSurface() const { return m_Surface; }

		/**
		 * @brief Retrieves the Vulkan swapchain present mode.
		 * @return the Vulkan swapchain present mode.
		 */
		[[nodiscard]] VkPresentModeKHR GetSwapPresentMode() const { return m_PresentMode; }

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }

		/**
		 * @brief Retrieves the Vulkan images of the swapchain.
		 * @return A reference to the vector of Vulkan images.
		 */
		std::vector<VkImage> &GetImages() { return m_Images; }

		/**
		 * @brief Retrieves the Vulkan image views of the swapchain.
		 * @return A reference to the vector of Vulkan image views.
		 */
		std::vector<VkImageView> &GetImageViews() { return m_ImageViews; }

	private:
		Ref<Device> m_Device = nullptr;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
		Flag m_IsDirty;

		uint32_t m_ImageIndex = 0;
		uint32_t m_Width = 0;
		uint32_t m_Height = 0;
		uint32_t m_SemaphoreIndex = 0;
		uint32_t m_LastAcquiredSemaphoreIndex = 0;
		VkPresentModeKHR m_PresentMode = VK_PRESENT_MODE_FIFO_KHR;

		static Scope<FrameSync> s_FrameSync;
		std::vector<Ref<FrameSync>> m_AcquiredSemaphore;
		std::vector<Ref<FrameSync>> m_CompleteSemaphore;
		std::vector<VkImage> m_Images;
		std::vector<VkImageView> m_ImageViews;

		std::array<VkImageView, 2> m_RenderTargetViews = { nullptr };

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

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
 * render_context.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "device.h"
#include "render_data.h"

// --------------------------------------------------------------

namespace SceneryEditorX
{
	class Window;
	class Swapchain;

	class RenderContext : public RefCounted
	{
	public:
		RenderContext() = default;
		virtual ~RenderContext() override;

		RenderContext(const RenderContext &) = delete;
		RenderContext &operator=(const RenderContext &) = delete;
		RenderContext(RenderContext &&) noexcept;
		RenderContext &operator=(RenderContext &&) noexcept;

		static void Init();
		static Ref<RenderContext> Get();
		static bool IsInitialized();
		static VkInstance GetInstance();

		Ref<Device> GetDevice() { return m_Device; }
		const Ref<Device> &GetLogicalDevice() { return m_Device; }
		static std::vector<uint8_t> GetPipelineCacheData() { return {}; }

		Swapchain *swapchain = nullptr;

		struct Renderable
		{
			VkDescriptorSet descriptorSet{VK_NULL_HANDLE};
			VkBuffer buffer{VK_NULL_HANDLE};
			VkDeviceSize vertexByteSize{0};
			VkDeviceSize indexByteSize{0};
			uint32_t indexCount{0};
		};
		
		VkPhysicalDevice physical = VK_NULL_HANDLE;
		uint32_t queueFamily = 0;
		VkDevice device = VK_NULL_HANDLE;
		VkQueue queue = VK_NULL_HANDLE;
		VmaAllocator allocator = VK_NULL_HANDLE;
		
		VkPipeline pipeline = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkDescriptorSet descriptorSetTex = VK_NULL_HANDLE;
		VkBuffer vBuffer = VK_NULL_HANDLE;
		VkDeviceSize vBufSize = 0;
		VkDeviceSize indexCount = 0;

		std::vector<Renderable>* renderables = nullptr; // optional: if provided, renderer will draw these
		std::array<ShaderDataBuffer, MAX_FRAMES_IN_FLIGHT>* s_ShaderDataBuffers = nullptr;
		std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT>* s_CommandBuffers = nullptr;

		// Sync objects are managed by FrameSync (vectors sized at runtime)
		std::vector<VkFence>* s_Fences = nullptr;
		std::vector<VkSemaphore>* s_PresentSemaphores = nullptr;
		std::vector<VkSemaphore>* s_RenderSemaphores = nullptr;
		//VkSurfaceCapabilitiesKHR* s_SurfaceCaps = nullptr;

	private:
		friend class Swapchain;

		VkInstance m_Instance = VK_NULL_HANDLE;
		Ref<Device> m_Device;
		VkPipelineCache m_PipelineCache = nullptr;
	};

}

// --------------------------------------------------------------

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
#include "VulkanApp.h" // TODO: remove dependency
#include "device.h"
#include <array>
#include <vector>
#include <vma/vk_mem_alloc.h>

// --------------------------------------------------------------

namespace SceneryEditorX
{
    class Window;
    class Swapchain;

    class RenderContext : public RefCounted
	{
    public:
        RenderContext();
        virtual ~RenderContext() override;

        RenderContext(const RenderContext &) = delete;
        RenderContext &operator=(const RenderContext &) = delete;
        RenderContext(RenderContext &&) noexcept;
        RenderContext &operator=(RenderContext &&) noexcept;

        void Init();
        static Ref<RenderContext> Get();
        [[nodiscard]] bool IsInitialized() const { return m_IsInitialized; }

        static VkInstance GetInstance();
        static Ref<Device> GetDevice() { return m_Device; }
        static const Ref<Device> &GetLogicalDevice() { return m_Device; }

        //Scope<MemoryAllocator> *GetMemoryAllocator() { return &m_MemAllocator;  }
        static std::vector<uint8_t> GetPipelineCacheData() { return {}; }

        struct Renderable
        {
            VkDescriptorSet descriptorSet{VK_NULL_HANDLE};
            VkBuffer buffer{VK_NULL_HANDLE};
            VkDeviceSize vertexByteSize{0};
            VkDeviceSize indexByteSize{0};
            uint32_t indexCount{0};
        };
	    
	    VkPhysicalDevice physical = VK_NULL_HANDLE;
	    VkSurfaceKHR surface = VK_NULL_HANDLE;
	    uint32_t queueFamily = 0;
	    VkDevice device = VK_NULL_HANDLE;
	    VkQueue queue = VK_NULL_HANDLE;
	    VmaAllocator allocator = VK_NULL_HANDLE;
	    Swapchain* swapchain = nullptr;
	    VkPipeline pipeline = VK_NULL_HANDLE;
	    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	    VkDescriptorSet descriptorSetTex = VK_NULL_HANDLE;
	    VkBuffer vBuffer = VK_NULL_HANDLE;
	    VkDeviceSize vBufSize = 0;
	    VkDeviceSize indexCount = 0;

	    std::vector<Renderable>* renderables = nullptr; // optional: if provided, renderer will draw these
	    std::array<ShaderDataBuffer, VulkanApp::MAX_FRAMES_IN_FLIGHT>* shaderDataBuffers = nullptr;
	    std::array<VkCommandBuffer, VulkanApp::MAX_FRAMES_IN_FLIGHT>* commandBuffers = nullptr;
	    // Sync objects are managed by FrameSync (vectors sized at runtime)
	    std::vector<VkFence>* fences = nullptr;
	    std::vector<VkSemaphore>* presentSemaphores = nullptr;
	    std::vector<VkSemaphore>* renderSemaphores = nullptr;
	    VkSurfaceCapabilitiesKHR* surfaceCaps = nullptr;

	private:
        static VkInstance m_Instance;
        static Ref<Device> m_Device;
        //Scope<MemoryAllocator>		m_MemAllocator;

        Window *window = nullptr;
        bool m_IsInitialized = false;
        VkPipelineCache m_PipelineCache = nullptr;

	};

}

// --------------------------------------------------------------

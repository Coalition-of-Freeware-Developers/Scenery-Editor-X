/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* render_context.h
* -------------------------------------------------------
* Created: 7/6/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/core/pointers.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/vulkan/vk_device.h>
#include <SceneryEditorX/renderer/vulkan/vk_swapchain.h>

/// -------------------------------------------------------

struct GLFWwindow;

namespace SceneryEditorX
{
    class RenderContext : public RefCounted
	{
	public:
		RenderContext();
        virtual ~RenderContext() override;

		/// Initialize the render context
		virtual void Init();

        Ref<VulkanDevice> GetLogicDevice() { return vkDevice; }
		GLOBAL VkInstance GetInstance() { return instance; }
        GLOBAL Ref<RenderContext> Get() { return Ref<RenderContext>(Renderer::GetContext()); }
		LOCAL Ref<VulkanDevice> GetCurrentDevice() { return Get()->GetLogicDevice(); } ///< Get the current VulkanDevice from the singleton instance

    private:
        Ref<VulkanPhysicalDevice> vkPhysicalDevice;
        Ref<VulkanDevice> vkDevice;
        inline LOCAL VkInstance instance;

        SwapChain swapChain;
        VkPipelineCache pipelineCache = nullptr;

#ifdef SEDX_DEBUG
        VkDebugReportCallbackEXT debugCallback = VK_NULL_HANDLE;
#endif

		VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    };

} // namespace SceneryEditorX

/// -------------------------------------------------------

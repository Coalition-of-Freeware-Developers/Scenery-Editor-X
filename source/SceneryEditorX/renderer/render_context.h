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
#include <vector>
#include <SceneryEditorX/core/pointers.h>
#include <SceneryEditorX/renderer/vulkan/vk_checks.h>
#include <SceneryEditorX/renderer/vulkan/vk_device.h>

/// -------------------------------------------------------

struct GLFWwindow;

namespace SceneryEditorX
{
    /// Forward declaration
    class Renderer;

    class RenderContext : public RefCounted
	{
	public:
		RenderContext();
        virtual ~RenderContext() override;

		/// Initialize the render context
		virtual void Init();

        Ref<VulkanDevice> GetLogicDevice() { return vkDevice; }
        GLOBAL VkInstance GetInstance();
        GLOBAL Ref<RenderContext> Get();
		LOCAL Ref<VulkanDevice> GetCurrentDevice() { return Get()->GetLogicDevice(); } ///< Get the current VulkanDevice from the singleton instance
        std::vector<uint8_t> GetPipelineCacheData() const;
        VkAllocationCallbacks *allocatorCallback = nullptr;

    private:
        Ref<VulkanPhysicalDevice> vkPhysicalDevice;
        Ref<VulkanDevice> vkDevice;
        inline LOCAL VkInstance instance;

		/// -------------------------------------------------------

        VkPipelineCache pipelineCache = nullptr;
        size_t pipelineCacheDataSize = 0;
        const void *pipelineCacheData = nullptr;

        /// -------------------------------------------------------

#ifdef SEDX_DEBUG
        VkDebugReportCallbackEXT debugCallback = VK_NULL_HANDLE;
#endif

		VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    };

} // namespace SceneryEditorX

/// -------------------------------------------------------

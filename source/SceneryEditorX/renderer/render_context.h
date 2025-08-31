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
#include "vulkan/vk_device.h"

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
		void Init();

        Ref<VulkanDevice> GetLogicDevice() { return vkDevice; }
        static VkInstance GetInstance();
        static Ref<RenderContext> Get();
		static Ref<VulkanDevice> GetCurrentDevice() { return Get()->GetLogicDevice(); } ///< Get the current VulkanDevice from the singleton instance
        std::vector<uint8_t> GetPipelineCacheData() const;
        VkAllocationCallbacks *allocatorCallback = nullptr;

    private:
        Ref<VulkanPhysicalDevice> vkPhysicalDevice;
        Ref<VulkanDevice> vkDevice;
        inline static VkInstance instance;

		/// -------------------------------------------------------

#ifdef SEDX_DEBUG
        VkDebugReportCallbackEXT debugCallback = VK_NULL_HANDLE;
#endif

		VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    };

}

/// -------------------------------------------------------

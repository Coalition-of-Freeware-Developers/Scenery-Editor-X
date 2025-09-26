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
    class VulkanPhysicalDevice;
    class VulkanDevice;

    class RenderContext : public RefCounted
	{
	public:
		RenderContext();
        virtual ~RenderContext() override;

		void Init();

        Ref<VulkanDevice> GetLogicDevice() { return vkDevice; }
        const Ref<VulkanDevice>& GetLogicalDevice() const { return vkDevice; }
        const Ref<VulkanPhysicalDevice>& GetPhysicalDevice() const { return vkPhysicalDevice; }

        static Ref<RenderContext> Get();
        static Ref<VulkanDevice> GetCurrentDevice() { return Get()->GetLogicDevice(); } // Get VulkanDevice from singleton instance
        static VkInstance GetInstance();

        std::vector<uint8_t> GetPipelineCacheData() const { return {}; }
        VkAllocationCallbacks* allocatorCallback = nullptr;

    private:
        Ref<VulkanPhysicalDevice>	vkPhysicalDevice;
        Ref<VulkanDevice>			vkDevice;
        VkInstance m_Instance		= VK_NULL_HANDLE;
        bool m_IsInitialized		= false;

		/// -------------------------------------------------------

#ifdef SEDX_DEBUG
        VkDebugReportCallbackEXT debugCallback = VK_NULL_HANDLE;
#endif
		VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    };

}

/// -------------------------------------------------------

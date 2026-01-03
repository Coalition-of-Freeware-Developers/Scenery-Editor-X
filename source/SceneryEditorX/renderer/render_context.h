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
#include "device.h"

struct GLFWwindow;

// -------------------------------------------------------

namespace SceneryEditorX
{
    class VulkanPhysicalDevice;
    class VulkanDevice;

    // -------------------------------------------------------

    class RenderContext : public RefCounted
	{
	public:
		RenderContext();
        virtual ~RenderContext() override;

        // Delete copy constructor and assignment operator.
		RenderContext(const RenderContext &) = delete;
        RenderContext &operator=(const RenderContext &) = delete;

        // Allow move operations if needed.
        RenderContext(RenderContext &&) noexcept;
        RenderContext &operator=(RenderContext &&) noexcept;

		void Init();
        Ref<VulkanDevice> GetLogicDevice() { return m_Device; }
        const Ref<VulkanDevice>& GetLogicalDevice() const { return m_Device; }
        const Ref<VulkanPhysicalDevice>& GetPhysicalDevice() const { return m_PhysicalDevice; }
        [[nodiscard]] bool IsInitialized() const { return m_IsInitialized; }

        static Ref<RenderContext> Get();
        static Ref<VulkanDevice> GetCurrentDevice() { return Get()->GetLogicDevice(); } // Get VulkanDevice from singleton instance
        static VkInstance GetInstance();

        std::vector<uint8_t> GetPipelineCacheData() const { return {}; }
        VkAllocationCallbacks* allocatorCallback = nullptr;

    private:
        Ref<VulkanPhysicalDevice>	m_PhysicalDevice;
        Ref<VulkanDevice>			m_Device;
        inline static VkInstance	m_Instance = VK_NULL_HANDLE;
        bool m_IsInitialized		= false;
        VkPipelineCache m_PipelineCache = nullptr;

		// -------------------------------------------------------

#ifdef SEDX_DEBUG
        VkDebugReportCallbackEXT m_DebugCallback = VK_NULL_HANDLE;
#endif
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
    };

}

// -------------------------------------------------------

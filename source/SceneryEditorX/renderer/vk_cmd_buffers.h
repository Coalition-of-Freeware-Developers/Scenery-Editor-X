/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_cmd_buffers.h
* -------------------------------------------------------
* Created: 7/4/2025
* -------------------------------------------------------
*/

#pragma once

#include <SceneryEditorX/renderer/render_data.h>
#include <SceneryEditorX/renderer/vk_device.h>
#include <SceneryEditorX/renderer/vk_util.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	// -------------------------------------------------------

	class CommandBuffer
	{
	public:
        static CommandResources &GetCurrentCommandResources();
        CommandBuffer(uint32_t count = 0);
        CommandBuffer(bool swapchain);
        ~CommandBuffer();

        virtual void Begin();
        virtual void End();
        virtual void Submit();

        void EndCmdBuffer(VkSubmitInfo submitInfo);

        static void EndCommandBuffer();

        VkCommandBuffer GetActiveCommandBuffer() const { return activeCmdBuffer; }

        VkCommandBuffer GetCommandBuffer(uint32_t frameIndex) const
        {
            frameIndex = RenderData::frameIndex;
            SEDX_CORE_ASSERT(frameIndex < cmdBuffers.size());
            return cmdBuffers[frameIndex];
        }

	private:
        VkCommandPool cmdPool = nullptr;
        VkCommandBuffer activeCmdBuffer = nullptr;
        Ref<VulkanDevice> device;
        RenderData renderData;
        std::vector<VkFence> waitFences;
        std::vector<VkCommandBuffer> cmdBuffers;
	};

    inline InternalQueue queues[Count];
    inline Queue currentQueue = Count;

	// -------------------------------------------------------

} // namespace SceneryEditorX

// -------------------------------------------------------

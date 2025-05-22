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
#include <SceneryEditorX/vulkan/render_data.h>
#include <SceneryEditorX/vulkan/vk_core.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	// -------------------------------------------------------

	class CommandBuffer
	{
	public:

        CommandBuffer(uint32_t count = 0);
        CommandBuffer(bool swapchain);
        ~CommandBuffer();

        virtual void Begin(Queue queue);
        virtual void End(VkSubmitInfo submitInfo);
        virtual void Submit();

        [[nodiscard]] CommandResources &GetCurrentCommandResources() const;
        [[nodiscard]] VkCommandBuffer GetActiveCommandBuffer() const { return activeCmdBuffer; }
        [[nodiscard]] VkCommandBuffer GetCommandBuffer(uint32_t frameIndex) const;
		[[nodiscard]] VkCommandPool GetCommandPool() const { return cmdPool; }

	private:

        VkCommandPool cmdPool = nullptr;
        VkCommandBuffer activeCmdBuffer = nullptr;

        RenderData renderData;
        Ref<GraphicsEngine> gfxEngine;

        std::vector<VkFence> waitFences;
        std::vector<VkCommandBuffer> cmdBuffers;
        std::map<std::string, float> timeStampTable;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;

		std::vector<VkQueryPool> TimestampQueryPools;
		std::vector<VkQueryPool> PipelineQueryPools;

		uint32_t availQuery = 2;
        uint32_t queryCount = 0;
		uint32_t pipelineQueryCount = 0;
        const uint32_t timeStampPerPool = 64;

		friend class VulkanDevice;
        friend class GraphicsEngine;
	};

    inline InternalQueue queues[Present];
    inline Queue currentQueue = Present;


} // namespace SceneryEditorX

// -------------------------------------------------------

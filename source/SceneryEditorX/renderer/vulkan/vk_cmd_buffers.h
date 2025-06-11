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
#include <SceneryEditorX/renderer/vulkan/render_data.h>
#include <SceneryEditorX/renderer/vulkan/vk_swapchain.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    /// Forward declarations
	class CommandPool;

    /// -------------------------------------------------------

    inline InternalQueue queues[Present];
    inline Queue currentQueue = Present;

    enum GPUQueueType : uint8_t
    {
        GPU_QUEUE_GRAPHICS = 0,
        GPU_QUEUE_COMPUTE = 1,
        GPU_QUEUE_TRANSFER = 2,
        GPU_QUEUES_COUNT = 3,
        GPU_QUEUE_UNKNOWN = static_cast<uint8_t>(-1)
    };

    /// -------------------------------------------------------

	class CommandBuffer : public RefCounted
	{
	public:
        CommandBuffer(uint32_t count = 0, std::string debugName = "");
        CommandBuffer(bool swapchain);
        virtual ~CommandBuffer() override;

		GLOBAL Ref<CommandBuffer> Get(); ///< Static accessor method to get the singleton instance

        virtual void Begin(Queue queue);
        virtual void End(VkSubmitInfo submitInfo);
        virtual void Submit();

        [[nodiscard]] CommandResources &GetCurrentCommandResources() const;
        [[nodiscard]] VkCommandBuffer GetActiveCmdBuffer() const { return activeCmdBuffer; }
        [[nodiscard]] VkCommandBuffer GetCommandBuffer(const RenderData &frameIndex) const;

	private:
        Ref<CommandBuffer> cmdBuffers;
        Ref<VulkanDevice> vkDevice;
        Ref<SwapChain> swapChain;
        VkCommandBuffer activeCmdBuffer = nullptr;
        VkCommandPool cmdPool = nullptr;

        std::vector<VkFence> waitFences;
        std::vector<VkCommandBuffer> commandBuffer;

        std::vector<VkQueryPool> TimestampQueryPools;
        std::vector<VkQueryPool> PipelineQueryPools;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;

        std::map<std::string, float> timeStampTable;

        RenderData data;
        uint32_t availQuery = 2;
        uint32_t queryCount = 0;
        uint32_t pipelineQueryCount = 0;
        uint32_t timeStampPerPool = 64;
        std::string debugName;

        friend class RenderContext;
	};

	/// ---------------------------------------------------------

} // namespace SceneryEditorX

/// -------------------------------------------------------

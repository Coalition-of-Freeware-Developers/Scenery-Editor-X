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
#include "vk_data.h"
#include "vk_swapchain.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
    // Forward declarations
	class CommandPool;

    // -------------------------------------------------------

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

    // -------------------------------------------------------

    struct PipelineStats
    {
        uint64_t InputAssemblyVertices = 0;
        uint64_t InputAssemblyPrimitives = 0;
        uint64_t VertexShaderInvocations = 0;
        uint64_t ClippingInvocations = 0;
        uint64_t ClippingPrimitives = 0;
        uint64_t FragmentShaderInvocations = 0;
        uint64_t ComputeShaderInvocations = 0;
    };

    // -------------------------------------------------------

	class CommandBuffer : public RefCounted
	{
	public:
        CommandBuffer() = default;
        explicit CommandBuffer(uint32_t count = 0, std::string debugName = "");
        CommandBuffer(std::string debugName, bool swapchain);
        virtual ~CommandBuffer() override;

		static Ref<CommandBuffer> Get(); // Static accessor method to get the singleton instance

        void Begin();
        void Begin(Queue queue);
        void Submit();
        void Submit(void *cmdBuffer, uint32_t waitFlags);
        void Wait(bool flush = false);

        void Execute(void *swapchain, uint32_t imageIdx);
        Queue GetQueueType() const { return qType;}
        CommandResources &GetCurrentCommandResources();
		const PipelineStats& GetPipelineStatistics(uint32_t frameIndex) const { return pipelineStatsQueryResults[frameIndex]; }

		uint32_t BeginTimestampQuery();
		void EndTimestampQuery(uint32_t queryID);

        [[nodiscard]] VkCommandBuffer GetActiveCmdBuffer() const { return activeCmdBuffer; }
        [[nodiscard]] VkCommandBuffer GetCommandBuffer(const RenderData &frameIndex) const;

    private:
        SwapChain swapChain;
        Ref<VulkanDevice> vkDevice;

        VkCommandPool cmdPool = nullptr;
        VkCommandBuffer activeCmdBuffer = nullptr;

        std::vector<VkFence> waitFences;
        std::vector<VkCommandBuffer> cmdBuffers;
        std::vector<PipelineStats> pipelineStatsQueryResults;

        std::vector<VkQueryPool> timestampQueryPools;
        std::vector<VkQueryPool> pipelineQueryPools;

	    std::map<std::string, float> timeStampTable;
        std::vector<std::vector<float>> executionGPUTimes;
        std::vector<std::vector<uint64_t>> timestampQueryResults;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;

	    Queue qType;
        RenderData data;

        bool ownedBySwapChain = false;
        uint8_t *cmdBuffer;
        uint8_t *cmdBufferPtr;
        std::atomic<uint32_t> cmdCount = 0;

        uint32_t availTimeQuery = 2;
        uint32_t timeQueryCount = 0;
        uint32_t pipelineQueryCount = 0;
        uint32_t timeStampPerPool = 64;
        std::string debugName;

        friend class RenderContext;
	};

}

// -------------------------------------------------------

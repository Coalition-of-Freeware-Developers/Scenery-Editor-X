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
        uint64_t inputAssemblyVertices = 0;
        uint64_t inputAssemblyPrimitives = 0;
        uint64_t vertexShaderInvocations = 0;
        uint64_t clippingInvocations = 0;
        uint64_t clippingPrimitives = 0;
        uint64_t fragmentShaderInvocations = 0;
        uint64_t computeShaderInvocations = 0;
    };

    // -------------------------------------------------------

	class CommandBuffer : public RefCounted
	{
	public:
        CommandBuffer() = default;
        CommandBuffer(std::string debugName, bool swapchain);
        //explicit CommandBuffer(Queue queue, std::string debugName = "");
        //explicit CommandBuffer(uint32_t count = 0, std::string debugName = "");
        explicit CommandBuffer(Queue queue, Ref<CommandPool> *cmdPool, std::string debugName = "");
        virtual ~CommandBuffer() override;

		static Ref<CommandBuffer> Get(); // Static accessor method to get the singleton instance

        void Begin();
        void Begin(Queue queue);
        void End();
        void End(VkSubmitInfo submitInfo);
        void Submit();
        void FlushCmdBuffer(VkCommandBuffer cmdBuffer);
        void Submit(void *cmdBuffer, uint32_t waitFlags);

        void Wait(bool flush = false);
        void Execute(void *swapchain, uint32_t imageIdx);

        Queue GetQueueType() const { return m_QType;}
        CommandResources& GetCurrentCommandResources();
		const PipelineStats& GetPipelineStatistics(uint32_t frameIndex) const { return m_PipelineStatsQueryResults[frameIndex]; }

        void FlushCmdBuffer(VkCommandBuffer cmdBuffer, VkQueue queue);
        uint32_t BeginTimestampQuery();
		void EndTimestampQuery(uint32_t queryID);
        void FlushCmdBuffer();

        [[nodiscard]] VkCommandBuffer GetActiveCmdBuffer() const { return m_ActiveCmdBuffer; }
        [[nodiscard]] VkCommandBuffer GetCommandBuffer(const RenderData &frameIndex) const;

        /**
         * Immediate submit helper (one-shot command buffer already ended)
         */
        void ImmediateSubmit(const Ref<CommandBuffer> &cmd);

        /**
		 * @brief Get the thread-local command pool.
		 * @return Reference to the thread-local command pool.
		 */
        Ref<CommandPool> GetThreadLocalCommandPool();

        /**
		 * @brief Get or create the thread-local command pool.
		 * @return Reference to the thread-local command pool.
		 */
        Ref<CommandPool> GetOrCreateThreadLocalCommandPool();

    private:
        SwapChain m_SwapChain;
        Ref<VulkanDevice> m_Device;

        Ref<CommandPool> LocalCommandPool();
        Ref<CommandPool> CreateLocalCommandPool();

        Ref<CommandPool> m_CmdPool = nullptr;
        VkCommandBuffer m_ActiveCmdBuffer = nullptr;

        std::vector<VkFence> m_WaitFences;
        std::vector<VkCommandBuffer> m_CmdBuffers;
        std::vector<PipelineStats> m_PipelineStatsQueryResults;

        std::vector<VkQueryPool> m_TimestampQueryPools;
        std::vector<VkQueryPool> m_PipelineQueryPools;

	    std::map<std::string, float> m_TimeStampTable;
        std::vector<std::vector<float>> m_ExecutionGpuTimes;
        std::vector<std::vector<uint64_t>> m_TimestampQueryResults;

        std::vector<VkSemaphore> m_ImageAvailableSemaphores;
        std::vector<VkSemaphore> m_RenderFinishedSemaphores;

	    Queue m_QType;
        RenderData m_Data;
        void *m_Resource = nullptr;
        bool m_OwnedBySwapChain = false;
        uint8_t *m_CmdBuffer;
        uint8_t *m_CmdBufferPtr;
        std::atomic<uint32_t> m_CmdCount = 0;

        uint32_t m_AvailTimeQuery = 2;
        uint32_t m_TimeQueryCount = 0;
        uint32_t m_PipelineQueryCount = 0;
        uint32_t m_TimeStampPerPool = 64;
        std::string m_DebugName;

        friend class RenderContext;
	};

}

// -------------------------------------------------------

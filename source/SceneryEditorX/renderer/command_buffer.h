/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* command_buffer.h
* -------------------------------------------------------
* Created: 22/12/2025
* -------------------------------------------------------
*/
#pragma once
#include "command_pool.h"
#include "pipeline.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

    class CommandBuffer : public RefCounted
	{
    public:
        CommandBuffer(uint32_t count = 0, std::string debugName = "");
        CommandBuffer(uint32_t count = 0, Ref<CommandPool> *cmdPool, std::string debugName = "");
        ~CommandBuffer() override;

        void Begin();
        void End();
        void Submit();

        uint32_t BeginTimestampQuery();
        void EndTimestampQuery(uint32_t queryID);

        static Ref<CommandBuffer> Get(); // Static accessor method to get the singleton instance

        VkCommandBuffer GetCommandBuffer(uint32_t frameIndex) const
        {
            SEDX_CORE_ASSERT(frameIndex < m_CommandBuffers.size());
            return m_CommandBuffers[frameIndex];
        }

    private:
        std::string m_DebugName;
        VkCommandPool m_CommandPool = nullptr;
        std::vector<VkFence> m_WaitFences;
        std::vector<VkCommandBuffer> m_CommandBuffers;
        VkCommandBuffer m_ActiveCommandBuffer = nullptr;

        uint32_t m_TimestampQueryCount = 0;
        uint32_t m_TimestampNextAvailableQuery = 2;
        std::vector<VkQueryPool> m_TimestampQueryPools;
        std::vector<VkQueryPool> m_PipelineStatisticsQueryPools;
        std::vector<std::vector<uint64_t>> m_TimestampQueryResults;
        std::vector<std::vector<float>> m_ExecutionGPUTimes;

        uint32_t m_PipelineQueryCount = 0;
        std::vector<PipelineStatistics> m_PipelineStatisticsQueryResults;
	};


}

// -------------------------------------------------------

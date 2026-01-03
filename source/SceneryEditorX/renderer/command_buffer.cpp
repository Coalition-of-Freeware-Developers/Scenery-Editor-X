/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* command_buffer.cpp
* -------------------------------------------------------
* Created: 22/12/2025
* -------------------------------------------------------
*/
#include "command_buffer.h"
#include "render_config.h"
#include "render_context.h"
#include "renderer.h"
#include "vulkan_utils.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

	CommandBuffer::CommandBuffer(uint32_t count, std::string debugName) : m_DebugName(debugName)
	{
        VkDevice device = RenderContext::Get()->GetLogicDevice()->GetDevice();
        if (count == 0)
        {
            RendererConfig cfg;
            // 0 means one per frame in flight
            count = cfg.framesInFlight;
        }

		SEDX_CORE_VERIFY(count > 0);

        VkCommandBufferAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.commandPool = m_CommandPool;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = count;
        m_CommandBuffers.resize(count);
        VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocate_info, m_CommandBuffers.data()));

        for (uint32_t i = 0; i < count; ++i)
            SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_COMMAND_BUFFER, std::format("{} (frame in flight: {})", m_DebugName, i), m_CommandBuffers[i]);

	    VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        m_WaitFences.resize(count);
        for (size_t i = 0; i < m_WaitFences.size(); ++i)
        {
            VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &m_WaitFences[i]));
            SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_FENCE, std::format("{} (frame in flight: {}) fence", m_DebugName, i), m_WaitFences[i]);
        }
	}

    CommandBuffer::CommandBuffer(uint32_t count, Ref<CommandPool> *cmdPool, std::string debugName) : m_DebugName(std::move(debugName))
    {
        VkDevice device = RenderContext::Get()->GetLogicDevice()->GetDevice();

        VkCommandBufferAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.commandPool = m_CommandPool;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = 1;

	    SEDX_CORE_VERIFY(vkAllocateCommandBuffers(device, &allocate_info, reinterpret_cast<VkCommandBuffer *>(&m_CommandBuffers)));
        for (uint32_t i = 0; i < count; ++i)
            SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_COMMAND_BUFFER, std::format("{} (frame in flight: {})", m_DebugName, i), m_CommandBuffers[i]);

        VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        m_WaitFences.resize(count);
        for (size_t i = 0; i < m_WaitFences.size(); ++i)
        {
            VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &m_WaitFences[i]));
            SetDebugUtilsObjectName(device, VK_OBJECT_TYPE_FENCE, std::format("{} (frame in flight: {}) fence", m_DebugName, i), m_WaitFences[i]);
        }

	    VkQueryPoolCreateInfo queryPoolCreateInfo = {};
        queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolCreateInfo.pNext = nullptr;

	    // Pipeline statistics queries
        m_PipelineQueryCount = 7;
        queryPoolCreateInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
        queryPoolCreateInfo.queryCount = m_PipelineQueryCount;
        queryPoolCreateInfo.pipelineStatistics = VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT | VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
                                                 VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT | VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
                                                 VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT | VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
                                                 VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;

        m_PipelineStatisticsQueryPools.resize(count);
        for (auto &pipelineStatisticsQueryPools : m_PipelineStatisticsQueryPools)
            VK_CHECK_RESULT(vkCreateQueryPool(device,
                                              &queryPoolCreateInfo,
                                              nullptr,
                                              &pipelineStatisticsQueryPools));

        m_PipelineStatisticsQueryResults.resize(count);
    }

    CommandBuffer::~CommandBuffer()
    {
        VkCommandPool commandPool = m_CommandPool;
        Renderer::SubmitResourceFree([commandPool]()
        {
            auto device = RenderContext::GetCurrentDevice();
            vkDestroyCommandPool(device->GetDevice(), commandPool, nullptr);
        });
    }

    void CommandBuffer::Begin()
    {
        //SEDX_ASSERT();
        Ref<CommandBuffer> instance = this;
        Renderer::Submit([instance]() mutable
		{
            uint32_t commandBufferIndex = Renderer::GetCurrentRenderThreadFrameIndex(); 
            
            VkCommandBufferBeginInfo cmdBufInfo = {};
            cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            cmdBufInfo.pNext = nullptr;

            commandBufferIndex %= instance->m_CommandBuffers.size();
            VkCommandBuffer commandBuffer = instance->m_CommandBuffers[commandBufferIndex];
            instance->m_ActiveCommandBuffer = commandBuffer;
            VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));

            // Timestamp query
            vkCmdResetQueryPool(commandBuffer, instance->m_TimestampQueryPools[commandBufferIndex],0, instance->m_TimestampQueryCount);
            vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, instance->m_TimestampQueryPools[commandBufferIndex], 0);

            // Pipeline stats query
            vkCmdResetQueryPool(commandBuffer, instance->m_PipelineStatisticsQueryPools[commandBufferIndex],0, instance->m_PipelineQueryCount);
            vkCmdBeginQuery(commandBuffer, instance->m_PipelineStatisticsQueryPools[commandBufferIndex], 0, 0);
		});

    }

    void CommandBuffer::End()
    {
        Ref<CommandBuffer> instance = this;
        Renderer::Submit([instance]() mutable
        {
            uint32_t commandBufferIndex = Renderer::GetCurrentRenderThreadFrameIndex();
            VkCommandBuffer commandBuffer = instance->m_ActiveCommandBuffer;
            vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, instance->m_TimestampQueryPools[commandBufferIndex], 1);
            vkCmdEndQuery(commandBuffer, instance->m_PipelineStatisticsQueryPools[commandBufferIndex], 0);
            VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

            instance->m_ActiveCommandBuffer = nullptr;
        });
    }

    void CommandBuffer::Submit()
    {
        Ref<CommandBuffer> instance = this;
        Renderer::Submit([instance]() mutable
        {
            auto device = RenderContext::GetCurrentDevice();

            uint32_t commandBufferIndex = Renderer::GetCurrentRenderThreadFrameIndex() % instance->m_CommandBuffers.size();

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            VkCommandBuffer commandBuffer = instance->m_CommandBuffers[commandBufferIndex];
            submitInfo.pCommandBuffers = &commandBuffer;

            VK_CHECK_RESULT(vkWaitForFences(device->GetDevice(), 1, &instance->m_WaitFences[commandBufferIndex], VK_TRUE, UINT64_MAX));
            VK_CHECK_RESULT(vkResetFences(device->GetDevice(), 1, &instance->m_WaitFences[commandBufferIndex]));

            SEDX_CORE_TRACE_TAG("Renderer", "Submitting Render Command Buffer {}", instance->m_DebugName);

            device->LockQueue();
            VK_CHECK_RESULT(vkQueueSubmit(device->GetGraphicsQueue(), 1, &submitInfo, instance->m_WaitFences[commandBufferIndex]));
            device->UnlockQueue();

            // Retrieve timestamp query results
            vkGetQueryPoolResults(device->GetDevice(), instance->m_TimestampQueryPools[commandBufferIndex], 0, instance->m_TimestampNextAvailableQuery,
                                  instance->m_TimestampNextAvailableQuery * sizeof(uint64_t), instance->m_TimestampQueryResults[commandBufferIndex].data(),
				sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);

            for (uint32_t i = 0; i < instance->m_TimestampNextAvailableQuery; i += 2)
            {
                uint64_t startTime = instance->m_TimestampQueryResults[commandBufferIndex][i];
                uint64_t endTime = instance->m_TimestampQueryResults[commandBufferIndex][i + 1];
                float nsTime = endTime > startTime ? (endTime - startTime) * RenderContext::Get()->GetPhysicalDevice()->GetLimits().timestampPeriod : 0.0f;
                instance->m_ExecutionGPUTimes[commandBufferIndex][i / 2] = nsTime * 0.000001f; // Time in ms
            }

            // Retrieve pipeline stats results
            vkGetQueryPoolResults(device->GetDevice(), instance->m_PipelineStatisticsQueryPools[commandBufferIndex], 0,1, sizeof(PipelineStatistics),
                                  &instance->m_PipelineStatisticsQueryResults[commandBufferIndex], sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
        });
    }

	uint32_t CommandBuffer::BeginTimestampQuery()
    {
        uint32_t queryIndex = m_TimestampNextAvailableQuery;
        m_TimestampNextAvailableQuery += 2;
        Ref<CommandBuffer> instance = this;
        Renderer::Submit([instance, queryIndex]()
        {
            uint32_t commandBufferIndex = Renderer::GetCurrentRenderThreadFrameIndex() % instance->m_CommandBuffers.size();
            VkCommandBuffer commandBuffer = instance->m_CommandBuffers[commandBufferIndex];
            vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, instance->m_TimestampQueryPools[commandBufferIndex], queryIndex);
        });
        return queryIndex;
    }

    void CommandBuffer::EndTimestampQuery(uint32_t queryID)
    {
        Ref<CommandBuffer> instance = this;
        Renderer::Submit([instance, queryID]()
        {
            uint32_t commandBufferIndex = Renderer::GetCurrentRenderThreadFrameIndex() % instance->m_CommandBuffers.size();
            VkCommandBuffer commandBuffer = instance->m_CommandBuffers[commandBufferIndex];
            vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, instance->m_TimestampQueryPools[commandBufferIndex], queryID + 1);
        });
    }

    Ref<CommandBuffer> CommandBuffer::Get()
    {
        return CreateRef<CommandBuffer>();
    }


} // namespace SceneryEditorX

// -------------------------------------------------------

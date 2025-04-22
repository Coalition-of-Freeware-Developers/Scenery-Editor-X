/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_cmd_buffers.cpp
* -------------------------------------------------------
* Created: 7/4/2025
* -------------------------------------------------------
*/

#include <SceneryEditorX/core/editor/editor.h>
#include <SceneryEditorX/renderer/vk_cmd_buffers.h>
#include <vulkan/vulkan.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

    //class SwapChain;

    // -------------------------------------------------------

	/*
    CommandResources& CommandBuffer::GetCurrentCommandResources()
	{
        return queues[currentQueue].commands[renderData.swapChainCurrentFrame];
    }
    */

    CommandBuffer::CommandBuffer(uint32_t count)
    {
    }

    /*
    CommandBuffer::CommandBuffer(bool swapchain)
    {
		VkQueryPoolCreateInfo queryPoolCreateInfo = {};
		queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		queryPoolCreateInfo.pNext = nullptr;

		// Timestamp queries
		const uint32_t maxUserQueries = 16;
		m_TimestampQueryCount = 2 + 2 * maxUserQueries;

		queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		queryPoolCreateInfo.queryCount = m_TimestampQueryCount;
		m_TimestampQueryPools.resize(RenderData::framesInFlight);
		for (auto& timestampQueryPool : m_TimestampQueryPools)
            VK_CHECK_RESULT(vkCreateQueryPool(device, &queryPoolCreateInfo, nullptr, &timestampQueryPool));

		m_TimestampQueryResults.resize(RenderData::framesInFlight);
		for (auto& timestampQueryResults : m_TimestampQueryResults)
			timestampQueryResults.resize(m_TimestampQueryCount);

		m_ExecutionGPUTimes.resize(RenderData::framesInFlight);
		for (auto& executionGPUTimes : m_ExecutionGPUTimes)
			executionGPUTimes.resize(m_TimestampQueryCount / 2);

		// Pipeline statistics queries
		pipelineQueryCount = 7;
		queryPoolCreateInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
		queryPoolCreateInfo.queryCount = pipelineQueryCount;
		queryPoolCreateInfo.pipelineStatistics =
			VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;

		m_PipelineStatisticsQueryPools.resize(RenderData::framesInFlight);
		for (auto& pipelineStatisticsQueryPools : m_PipelineStatisticsQueryPools)
            VK_CHECK_RESULT(vkCreateQueryPool(device, &queryPoolCreateInfo, nullptr, &pipelineStatisticsQueryPools));

		m_PipelineStatisticsQueryResults.resize(RenderData::framesInFlight);
    }
    */

    /*
    CommandBuffer::~CommandBuffer()
    {
        VkCommandPool commandPool = cmdPool;
        this->device;
        GraphicsEngine::SubmitResourceFree([commandPool, this]()
		{
            vkDestroyCommandPool(device->GetDevice(), commandPool, nullptr);
        });
    }

    void CommandBuffer::Begin()
    {
		Ref instance = this;
		GraphicsEngine::Submit([instance]() mutable
		{
			VkCommandBufferBeginInfo cmdBufInfo = {};
			cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			cmdBufInfo.pNext = nullptr;

			VkCommandBuffer commandBuffer = nullptr;
			if (instance->m_OwnedBySwapChain)
			{
                Ref<SwapChain> swapChain = GraphicsEngine::GetSwapChain();
				commandBuffer = swapChain.GetDrawCommandBuffer(cmdBufferIndex);
			}
			else
			{
				cmdBufferIndex %= instance->cmdBuffers.size();
				commandBuffer = instance->cmdBuffers[cmdBufferIndex];
			}
			instance->activeCmdBuffer = commandBuffer;
			VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));

			// Timestamp query
			vkCmdResetQueryPool(commandBuffer, instance->m_TimestampQueryPools[commandBufferIndex], 0, instance->m_TimestampQueryCount);
			vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, instance->m_TimestampQueryPools[commandBufferIndex], 0);

			// Pipeline stats query
			vkCmdResetQueryPool(commandBuffer, instance->m_PipelineStatisticsQueryPools[cmdBufferIndex], 0, instance->pipelineQueryCount);
			vkCmdBeginQuery(commandBuffer, instance->m_PipelineStatisticsQueryPools[cmdBufferIndex], 0, 0);
		});
    }
    */

    void CommandBuffer::End()
    {
    }

    void CommandBuffer::Submit()
    {
    }

    // -------------------------------------------------------

    /*
    void CommandBuffer::EndCmdBuffer(VkSubmitInfo submitInfo)
	{
        auto &cmd = GetCurrentCommandResources();

		vkEndCommandBuffer(cmd.cmdBuffer);

		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd.buffer;

        if (auto result = vkQueueSubmit(submitInfo.queue, 1, &submitInfo, VK_NULL_HANDLE, cmd.fence); result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to submit command buffer: {}", ToString(result));
            ErrMsg("Failed to submit command buffer!");
        }
	}
	*/


} // namespace SceneryEditorX

// -------------------------------------------------------

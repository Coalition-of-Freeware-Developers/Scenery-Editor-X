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
#include <SceneryEditorX/editor/editor.h>
#include <SceneryEditorX/vulkan/vk_cmd_buffers.h>
#include <vulkan/vulkan.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

    /// -------------------------------------------------------

    CommandResources& CommandBuffer::GetCurrentCommandResources() const
    {
        return queues[currentQueue].commands[renderData.swapChainCurrentFrame];
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

    CommandBuffer::~CommandBuffer() = default;

    void CommandBuffer::Begin(const Queue queue)
    {
        SEDX_ASSERT(currentQueue == Queue::Count, "Already recording a command buffer");
        currentQueue = queue;
        auto &cmd = GetCurrentCommandResources();
        vkWaitForFences(gfxEngine->GetLogicDevice()->GetDevice(), 1, &cmd.fence, VK_TRUE, UINT64_MAX);
        vkResetFences(gfxEngine->GetLogicDevice()->GetDevice(), 1, &cmd.fence);

		if (!cmd.timeStamps.empty())
        {
            vkGetQueryPoolResults(gfxEngine->GetLogicDevice()->GetDevice(), cmd.queryPool,0,cmd.timeStamps.size(),
								  cmd.timeStamps.size() * sizeof(uint64_t),cmd.timeStamps.data(),sizeof(uint64_t),VK_QUERY_RESULT_64_BIT);
            for (int i = 0; i < cmd.timeStampNames.size(); i++)
            {
                const uint64_t begin = cmd.timeStamps[2 * i];
                const uint64_t end = cmd.timeStamps[2 * i + 1];
                timeStampTable[cmd.timeStampNames[i]] = static_cast<float>(end - begin) * gfxEngine->GetCurrentDevice()->GetPhysicalDevice()->GetDeviceProperties().limits.timestampPeriod / 1000000.0f;
            }
            cmd.timeStamps.clear();
            cmd.timeStampNames.clear();
        }

		InternalQueue &internalQueue = queues[queue];
        vkResetCommandPool(gfxEngine->GetLogicDevice()->GetDevice(), cmdPool, 0);
        cmd.stagingOffset = 0;
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd.buffer, &beginInfo);

		if (queue != Queue::Transfer)
            vkCmdResetQueryPool(cmd.buffer, cmd.queryPool, 0, timeStampPerPool);
    }

    void CommandBuffer::End(VkSubmitInfo submitInfo)
    {
        auto &cmd = GetCurrentCommandResources();

		vkEndCommandBuffer(cmd.buffer);

		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd.buffer;

		auto result = vkQueueSubmit(queues[currentQueue].queue, 1, &submitInfo, cmd.fence);
        SEDX_ASSERT(result != VK_SUCCESS, "Failed to submit command buffer to queue");
    }

    VkCommandBuffer CommandBuffer::GetCommandBuffer(uint32_t frameIndex) const
    {
        frameIndex = RenderData::frameIndex;
        SEDX_CORE_ASSERT(frameIndex < cmdBuffers.size());
        return cmdBuffers[frameIndex];
    }

    void CommandBuffer::Submit()
    {
        const auto &cmd = GetCurrentCommandResources();

        VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        const VkSwapchainKHR swapchain = gfxEngine->GetSwapChain()->GetSwapchain();

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &imageAvailableSemaphores[renderData.swapChainCurrentFrame];
		submitInfo.pWaitDstStageMask = &waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &(cmd.buffer);
		submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &renderFinishedSemaphores[renderData.swapChainCurrentFrame];

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = submitInfo.pSignalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &RenderData::imageIndex;
        presentInfo.pResults = nullptr;


    }

    // -------------------------------------------------------

} // namespace SceneryEditorX

// -------------------------------------------------------

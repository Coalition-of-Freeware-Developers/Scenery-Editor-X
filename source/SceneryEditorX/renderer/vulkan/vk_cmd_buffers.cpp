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
#include <SceneryEditorX/renderer/vulkan/vk_cmd_buffers.h>
#include <SceneryEditorX/renderer/render_context.h>
#include <SceneryEditorX/renderer/vulkan/vk_util.h>
#include <vulkan/vulkan.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    CommandResources& CommandBuffer::GetCurrentCommandResources() const
    {
        RenderData renderData;
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

    CommandBuffer::CommandBuffer(uint32_t count, std::string debugName) : debugName(std::move(debugName))
    {
        /// Get the device from graphics engine
        const auto device = vkDevice;

        if (count == 0)
            count = data.framesInFlight;	/// 0 = one per frame in flight

        SEDX_CORE_VERIFY(count > 0, "CommandBuffer count must be greater than 0");

        /// Allocate command buffers if count > 0
        if (count > 0)
        {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = cmdPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = count;
            commandBuffer.resize(count);

            VK_CHECK_RESULT(vkAllocateCommandBuffers(device->GetDevice(), &allocInfo, commandBuffer.data()))

            for (uint32_t i = 0; i < count; ++i)
            {
                /// Set debug name for each command buffer
                if (device->vkSetDebugUtilsObjectNameEXT && !debugName.empty())
                {
                    VkDebugUtilsObjectNameInfoEXT nameInfo{};
                    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
                    nameInfo.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
                    nameInfo.objectHandle = reinterpret_cast<uint64_t>(activeCmdBuffer);
                    nameInfo.pObjectName = debugName.c_str();
                    device->vkSetDebugUtilsObjectNameEXT(device->Selected(), &nameInfo);
                }
            }
        }

    }

    CommandBuffer::~CommandBuffer()
    {
        VkCommandPool commandPool = cmdPool;
        if (!commandBuffer.empty())
        {
            auto device = vkDevice;
            vkDestroyCommandPool(device->GetDevice(), commandPool, nullptr);
            commandBuffer.clear();
        }
    }

    /// -------------------------------------------------------
    
    Ref<CommandBuffer> CommandBuffer::Get()  
    {  
        static Ref<CommandBuffer> cmdBuffersInstance; /// Static instance to ensure a single shared instance  
        if (!cmdBuffersInstance)  
        {
            SEDX_CORE_WARN_TAG("CommandBuffer", "Creating command buffers for the first time");  
            cmdBuffersInstance = CreateRef<CommandBuffer>();  
        }
        return cmdBuffersInstance;  
    }

    /// -------------------------------------------------------

    void CommandBuffer::Begin(const Queue queue)
    {
        SEDX_ASSERT(currentQueue == Queue::Count, "Already recording a command buffer");
        currentQueue = queue;

        Ref<CommandBuffer> cmdBufferInst;

        auto &cmd = GetCurrentCommandResources();
        auto device = vkDevice->GetDevice();
        auto vkPhysDevice = vkDevice->GetPhysicalDevice();

        vkWaitForFences(device, 1, &cmd.fence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &cmd.fence);

		if (!cmd.timeStamps.empty())
        {
            vkGetQueryPoolResults(device, cmd.queryPool,0,cmd.timeStamps.size(),
								  cmd.timeStamps.size() * sizeof(uint64_t),cmd.timeStamps.data(),sizeof(uint64_t),VK_QUERY_RESULT_64_BIT);
            for (int i = 0; i < cmd.timeStampNames.size(); i++)
            {
                const uint64_t begin = cmd.timeStamps[2 * i];
                const uint64_t end = cmd.timeStamps[2 * i + 1];
                timeStampTable[cmd.timeStampNames[i]] =
                    static_cast<float>(end - begin) *
                    vkPhysDevice->GetDeviceProperties().limits.timestampPeriod / 1000000.0f;
            }
            cmd.timeStamps.clear();
            cmd.timeStampNames.clear();
        }

		InternalQueue &internalQueue = queues[queue];
        vkResetCommandPool(device, cmdPool, 0);
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

    /**
     * @fn GetCommandBuffer
     * @brief Retrieves a command buffer for the specified frame index
     * 
     * @details This method provides access to a specific command buffer from the pool of
     * command buffers managed by this CommandBuffer object. Each frame in the swap chain
     * typically has its own dedicated command buffer to allow for parallel command recording
     * while previous frames are still being processed by the GPU.
     * 
     * The method performs bounds checking through an assertion to ensure the requested
     * frame index is valid and within the range of available command buffers.
     * 
     * @param frameIndex The RenderData structure containing the frame index of the desired command buffer
     * 
     * @return VkCommandBuffer The Vulkan command buffer handle for the specified frame
     * 
     * @note This method is const, indicating it doesn't modify the internal state of the CommandBuffer object
     * @note Command buffers should not be used across multiple frames without proper synchronization
     * 
     * @see Begin, End, Submit
     */
    /*
    VkCommandBuffer CommandBuffer::GetCommandBuffer(const RenderData &frameIndex) const
    {
        SEDX_CORE_ASSERT(frameIndex.frameIndex < cmdBuffers.size());
        return cmdBuffers[frameIndex.frameIndex];
    }
	*/

    void CommandBuffer::Submit()
    {
        RenderData renderData;
        const auto &cmd = GetCurrentCommandResources();

        constexpr VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        const VkSwapchainKHR swapchain = swapChain.GetSwapchain();

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
        presentInfo.pImageIndices = &renderData.imageIndex;
        presentInfo.pResults = nullptr;
    }

} // namespace SceneryEditorX

/// -------------------------------------------------------

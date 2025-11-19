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
#include "vk_cmd_buffers.h"
#include "vk_util.h"
#include "SceneryEditorX/renderer/command_pool.h"
#include "SceneryEditorX/renderer/render_context.h"
#include "SceneryEditorX/renderer/renderer.h"
#include <vulkan/vulkan.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
    /**
     * @brief Get the current command resources for the active queue and frame.
     * @return Reference to the CommandResources for the current frame in the active queue.
     */
    CommandResources & CommandBuffer::GetCurrentCommandResources()
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

    /**
     * @brief Constructs a CommandBuffer object, allocating the specified number of Vulkan command buffers.
     * @param count Number of command buffers to allocate. If 0, allocates one per frame in flight.
     * @param debugName Debug name for the command buffers for easier identification in debugging tools.
     */
    /*
    CommandBuffer::CommandBuffer(uint32_t count, std::string debugName) : debugName(std::move(debugName))
    {
        // Get the device from graphics engine
        auto device = RenderContext::GetCurrentDevice();
        m_Device = device;

        if (count == 0)
            count = data.framesInFlight; // 0 = one per frame in flight

        SEDX_CORE_VERIFY(count > 0, "CommandBuffer count must be greater than 0");

        if (cmdPool == nullptr)
        {
            VkCommandPoolCreateInfo cmdPoolInfo{};
            cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            cmdPoolInfo.queueFamilyIndex = device->GetPhysicalDevice()->GetQueueFamilyIndices().GetGraphicsFamily();
            VK_CHECK_RESULT(vkCreateCommandPool(device->GetDevice(), &cmdPoolInfo, nullptr, &cmdPool))
            SetDebugUtilsObjectName(device->GetDevice(), VK_OBJECT_TYPE_COMMAND_POOL, this->debugName, cmdPool);
        }

        // Allocate command buffers
        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = cmdPool;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = count;

        cmdBuffers.resize(count);
        VK_CHECK_RESULT(vkAllocateCommandBuffers(device->GetDevice(), &commandBufferAllocateInfo, cmdBuffers.data()))

        for (uint32_t i = 0; i < count; ++i)
        {
            SetDebugUtilsObjectName(device->GetDevice(),
                                    VK_OBJECT_TYPE_COMMAND_BUFFER,
                                    std::format("{} (frame in flight: {})", this->debugName, i),
                                    cmdBuffers[i]);
        }

        // TODO: Use Fence Class here
        VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        waitFences.resize(count);
        for (size_t i = 0; i < waitFences.size(); ++i)
        {
            VK_CHECK_RESULT(vkCreateFence(device->GetDevice(), &fenceCreateInfo, nullptr, &waitFences[i]))
            SetDebugUtilsObjectName(device->GetDevice(),
                                    VK_OBJECT_TYPE_FENCE,
                                    std::format("{} (frame in flight: {}) fence", this->debugName, i),
                                    waitFences[i]);
        }

        // Query pools
        VkQueryPoolCreateInfo queryPoolCreateInfo{};
        queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolCreateInfo.pNext = nullptr;

        // Timestamp queries
        const uint32_t maxUserQueries = 16;
        timeQueryCount = 2 + 2 * maxUserQueries;

        queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queryPoolCreateInfo.queryCount = timeQueryCount;
        timestampQueryPools.resize(count);

        for (auto &timestampQueryPool : timestampQueryPools)
            VK_CHECK_RESULT(vkCreateQueryPool(device->GetDevice(), &queryPoolCreateInfo, nullptr, &timestampQueryPool))

        timestampQueryResults.resize(count);
        for (auto &timestampResults : timestampQueryResults)
            timestampResults.resize(timeQueryCount);

        executionGPUTimes.resize(count);
        for (auto &executionTimes : executionGPUTimes)
            executionTimes.resize(timeQueryCount / 2);

        // Pipeline statistics queries
        pipelineQueryCount = 7;
        queryPoolCreateInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
        queryPoolCreateInfo.queryCount = pipelineQueryCount;
        queryPoolCreateInfo.pipelineStatistics = VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
                                                 VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
                                                 VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
                                                 VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
                                                 VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
                                                 VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
                                                 VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;

        pipelineQueryPools.resize(count);
        for (auto &pipelineStatisticsQueryPool : pipelineQueryPools)
            VK_CHECK_RESULT(
                vkCreateQueryPool(device->GetDevice(), &queryPoolCreateInfo, nullptr, &pipelineStatisticsQueryPool))

        // Store per-frame pipeline stats results
        pipelineStatsQueryResults.resize(count);
    }
    */

	/**
	 * @brief Constructs a CommandBuffer for swapchain ownership if requested.
	 * Allocates query pools sized for frames-in-flight and marks ownership state.
	 * @param debugName Debug name for the command buffers for easier identification in debugging tools.
     * @param swapchain
	 */
    CommandBuffer::CommandBuffer(std::string debugName, bool swapchain) : ownedBySwapChain(swapchain), debugName(std::move(debugName))
    {
        auto device = RenderContext::GetCurrentDevice();
        vkDevice = device;

        const uint32_t framesInFlight = data.framesInFlight;

        // Timestamp queries
        VkQueryPoolCreateInfo queryPoolCreateInfo{};
        queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolCreateInfo.pNext = nullptr;

        constexpr uint32_t maxUserQueries = 16;
        timeQueryCount = 2 + 2 * maxUserQueries;

        queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queryPoolCreateInfo.queryCount = timeQueryCount;

        timestampQueryPools.resize(framesInFlight);
        for (auto &timestampQueryPool : timestampQueryPools)
            VK_CHECK_RESULT(vkCreateQueryPool(device->GetDevice(), &queryPoolCreateInfo, nullptr, &timestampQueryPool))

        timestampQueryResults.resize(framesInFlight);
        for (auto &timestampResults : timestampQueryResults)
            timestampResults.resize(timeQueryCount);

        executionGPUTimes.resize(framesInFlight);
        for (auto &executionTimes : executionGPUTimes)
            executionTimes.resize(timeQueryCount / 2);

        // Pipeline statistics queries
        pipelineQueryCount = 7;
        queryPoolCreateInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
        queryPoolCreateInfo.queryCount = pipelineQueryCount;
        queryPoolCreateInfo.pipelineStatistics = VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
                                                 VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
                                                 VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
                                                 VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
                                                 VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
                                                 VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
                                                 VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;

        pipelineQueryPools.resize(framesInFlight);
        for (auto &pipelineStatisticsQueryPool : pipelineQueryPools)
            VK_CHECK_RESULT(
                vkCreateQueryPool(device->GetDevice(), &queryPoolCreateInfo, nullptr, &pipelineStatisticsQueryPool))

        pipelineStatsQueryResults.resize(framesInFlight);
    }

    /*
    CommandBuffer::CommandBuffer(Queue queue, std::string debugName)
    {
        SEDX_ASSERT(currentQueue == Queue::Count, "Already recording a command buffer");
		this->qType = queue;
        Ref<CommandPool> cmd_pool = GetOrCreateThreadLocalCommandPool();

        VkCommandBufferAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.commandPool = static_cast<VkCommandPool>(cmd_pool);
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = 1;

    }
    */

    /*
    CommandBuffer::CommandBuffer(std::string debugName, bool swapchain) : ownedBySwapChain(true), debugName(std::move(debugName))
    {
        auto device = RenderContext::GetCurrentDevice();
        uint32_t framesInFlight = data.framesInFlight;

        VkQueryPoolCreateInfo queryPoolCreateInfo = {};
        queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolCreateInfo.pNext = nullptr;

        // Timestamp queries
        constexpr uint32_t maxUserQueries = 16;
        timeQueryCount = 2 + 2 * maxUserQueries;

        queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queryPoolCreateInfo.queryCount = timeQueryCount;

        timestampQueryPools.resize(framesInFlight);
        for (auto &timestampQueryPool : timestampQueryPools) VK_CHECK_RESULT(
            vkCreateQueryPool(device->GetDevice(), &queryPoolCreateInfo, nullptr, &timestampQueryPool))

        timestampQueryResults.resize(framesInFlight);
        for (auto &timestampQueryResults : timestampQueryResults)
            timestampQueryResults.resize(timeQueryCount);

        executionGPUTimes.resize(framesInFlight);
        for (auto &executionGPUTimes : executionGPUTimes)
            executionGPUTimes.resize(timeQueryCount / 2);

        // Pipeline statistics queries
        pipelineQueryCount = 7;
        queryPoolCreateInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
        queryPoolCreateInfo.queryCount = pipelineQueryCount;
        queryPoolCreateInfo.pipelineStatistics =
            VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
            VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
            VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
            VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT | VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
            VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
            VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;

        pipelineQueryPools.resize(framesInFlight);
        for (auto &pipelineStatisticsQueryPools : pipelineQueryPools) VK_CHECK_RESULT(
            vkCreateQueryPool(device->GetDevice(), &queryPoolCreateInfo, nullptr, &pipelineStatisticsQueryPools))

        pipelineStatsQueryResults.resize(framesInFlight);
    }
    */

    /**
	 * @brief Destructor for CommandBuffer class. Cleans up allocated Vulkan command buffers.
	 */
    CommandBuffer::~CommandBuffer()
    {
        if (ownedBySwapChain)
            return;

        auto deviceRef = vkDevice ? vkDevice : RenderContext::GetCurrentDevice();
        auto dev = deviceRef->GetDevice();

        /*
        // Free command buffers in a single call
        if (!cmdBuffers.empty())
        {
            vkFreeCommandBuffers(dev, cmdPool, static_cast<uint32_t>(cmdBuffers.size()), cmdBuffers.data());
            cmdBuffers.clear();
        }
        */

        // Destroy fences
        for (auto &f : waitFences)
        {
            if (f)
                vkDestroyFence(dev, f, nullptr);
        }
        waitFences.clear();

        // Destroy query pools
        for (auto &qp : timestampQueryPools)
        {
            if (qp)
                vkDestroyQueryPool(dev, qp, nullptr);
        }
        timestampQueryPools.clear();

        for (auto &qp : pipelineQueryPools)
        {
            if (qp)
                vkDestroyQueryPool(dev, qp, nullptr);
        }
        pipelineQueryPools.clear();

        /*
        // Destroy command pool
        if (cmdPool)
        {
            vkDestroyCommandPool(dev, cmdPool, nullptr);
            cmdPool = nullptr;
        }*/
    }

    // -------------------------------------------------------

    /**
	 * @brief Static accessor method to get the singleton instance of CommandBuffer
	 * @return Reference to the singleton CommandBuffer instance
	 */
    Ref<CommandBuffer> CommandBuffer::Get()
    {
        static Ref<CommandBuffer> cmdBuffersInstance; // Static instance to ensure a single shared instance
        if (!cmdBuffersInstance)
        {
            SEDX_CORE_WARN_TAG("CommandBuffer", "Creating command buffers for the first time");
            // Pass parameters to disambiguate constructor overloads
            cmdBuffersInstance = CreateRef<CommandBuffer>(0u, "Global Command Buffer");
        }
        return cmdBuffersInstance;
    }

    // -------------------------------------------------------

    /**
	 * @fn Begin
	 * @brief Begins recording commands into the command buffer.
	 * 
	 * @details This method prepares the command buffer for recording by setting up
	 * the necessary Vulkan structures and states. It retrieves the appropriate command
	 * buffer for the current frame index and begins the command buffer recording process.
	 * Additionally, it resets and initializes timestamp and pipeline statistics query pools
	 * to enable performance measurements during command execution.
	 */
	void CommandBuffer::Begin()
	{
        availTimeQuery = 2;

		Ref<CommandBuffer> instance(this);
		Renderer::Submit([instance]() mutable
		{
			uint32_t commandBufferIndex = Renderer::GetCurrentRenderThreadFrameIndex();

			VkCommandBufferBeginInfo cmdBufInfo = {};
			cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			cmdBufInfo.pNext = nullptr;

			VkCommandBuffer commandBuffer = nullptr;
			if (instance->ownedBySwapChain)
			{
				SwapChain& swapChain = Application::Get().GetWindow().GetSwapChain();
				commandBuffer = swapChain.GetDrawCommandBuffer(commandBufferIndex);
			}
			else
			{
				commandBufferIndex %= instance->cmdBuffers.size();
                commandBuffer = instance->cmdBuffers[commandBufferIndex];
			}
			instance->activeCmdBuffer = commandBuffer;
			VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo))

			// Timestamp query
			vkCmdResetQueryPool(commandBuffer, instance->timestampQueryPools[commandBufferIndex], 0, instance->timeQueryCount);
			vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, instance->timestampQueryPools[commandBufferIndex], 0);

			// Pipeline stats query
			vkCmdResetQueryPool(commandBuffer, instance->pipelineQueryPools[commandBufferIndex], 0, instance->pipelineQueryCount);
			vkCmdBeginQuery(commandBuffer, instance->pipelineQueryPools[commandBufferIndex], 0, 0);
		});
	}

    /**
	 * @fn Begin
	 * @brief Begins recording commands into the command buffer for the specified queue.
	 * 
	 * @param queue The queue type (Graphics, Compute, Transfer) for which to begin recording.
	 * 
	 * @details This method prepares the command buffer for recording by setting up
	 * the necessary Vulkan structures and states for the specified queue type. It retrieves
	 * the appropriate command buffer for the current frame index and begins the command buffer
	 * recording process. Additionally, it resets and initializes timestamp query pools
	 * to enable performance measurements during command execution.
	 */
    /*
    void CommandBuffer::Begin(const Queue queue)
    {
        SEDX_ASSERT(currentQueue == Queue::Count, "Already recording a command buffer");
        currentQueue = queue;

        Ref<CommandBuffer> cmdBufferInst;

        auto &cmd = GetCurrentCommandResources();
        const auto device = m_Device->GetDevice();
        const auto vkPhysDevice = m_Device->GetPhysicalDevice();

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
                    static_cast<float>(end - begin) * vkPhysDevice->GetDeviceProperties().properties.limits.timestampPeriod / 1000000.0f;
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
    */

    /**
     * @fn End
     * @brief Ends the recording of commands into the command buffer.
     *
     * @details This method finalizes the command buffer recording process by ending
     * the command buffer and writing a timestamp to the query pool. It ensures that
     * the command buffer is properly closed and ready for submission to the GPU.
     */
    void CommandBuffer::End()
    {
        Ref<CommandBuffer> instance(this);
        Renderer::Submit([instance]() mutable
        {
            uint32_t commandBufferIndex = Renderer::GetCurrentRenderThreadFrameIndex();
            if (!instance->ownedBySwapChain)
                commandBufferIndex %= instance->cmdBuffers.size();

            VkCommandBuffer commandBuffer = instance->activeCmdBuffer;
            vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,instance->timestampQueryPools[commandBufferIndex],1);
            vkCmdEndQuery(commandBuffer, instance->pipelineQueryPools[commandBufferIndex], 0);
            VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer))

            instance->activeCmdBuffer = nullptr;
        });
    }

    /**
	 * @fn End
	 * @brief Ends the recording of commands into the command buffer and submits it to the queue.
	 * 
	 * @param submitInfo Reference to a VkSubmitInfo structure that will be populated for submission.
	 * 
	 * @details This method finalizes the command buffer recording process by ending
	 * the command buffer and preparing it for submission to the specified queue.
	 * It fills out the provided VkSubmitInfo structure with the necessary information
	 * to submit the command buffer for execution on the GPU.
	 */
    void CommandBuffer::End(VkSubmitInfo submitInfo)
    {
        const auto &cmd = GetCurrentCommandResources();

        vkEndCommandBuffer(cmd.buffer);
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd.buffer;

        const auto result = vkQueueSubmit(queues[currentQueue].queue, 1, &submitInfo, cmd.fence);
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
     * @note - This method is const, indicating it doesn't modify the internal state of the CommandBuffer object
     * @note - Command buffers should not be used across multiple frames without proper synchronization
     * 
     * @see Begin, End, Submit
     */
    VkCommandBuffer CommandBuffer::GetCommandBuffer(const RenderData &frameIndex) const
    {
        SEDX_CORE_ASSERT(frameIndex.frameIndex < cmdBuffers.size());
        return cmdBuffers[frameIndex.frameIndex];
    }

    /**
	 * @brief Submits a command buffer for immediate execution.
	 * @param cmd Reference to the command buffer to be submitted.
	 */
    void CommandBuffer::ImmediateSubmit(const Ref<CommandBuffer> &cmd)
    {
        if (!cmd) return;
        FlushCmdBuffer();
    }

    /**
     * @brief Submits the current command buffer to the graphics queue for execution
     * @details This method handles the submission of the currently recorded command buffer
     * to the graphics queue. It sets up the necessary synchronization primitives, 
     * including semaphores for image availability and render completion.
     * It constructs a VkSubmitInfo structure to specify the command buffer to be submitted,
     * the semaphores to wait on before execution, and the semaphores to signal upon completion.
     * After submission, it also prepares a VkPresentInfoKHR structure to present the rendered image
     * to the swap chain.
     *
     * @note This method assumes that the command buffer has already been recorded and is ready
     * for submission.
     */
    void CommandBuffer::Submit()
    {
        RenderData renderData;
        const auto &cmd = GetCurrentCommandResources();

        constexpr VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        const VkSwapchainKHR swapchain = swapChain.GetSwapchain();

        VkSubmitInfo submitInfo;
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &imageAvailableSemaphores[renderData.swapChainCurrentFrame];
        submitInfo.pWaitDstStageMask = &waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &(cmd.buffer);
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &renderFinishedSemaphores[renderData.swapChainCurrentFrame];

        VkPresentInfoKHR presentInfo;
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = submitInfo.pSignalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &renderData.imageIndex;
        presentInfo.pResults = nullptr;

        // NOTE: Actual queue submit/present handled elsewhere in renderer pipeline
    }

    /**
	 * @brief Submits the command buffer for execution on the GPU.
	 *
	 * @details This method handles the submission of the command buffer to the graphics queue.
	 * It sets up the necessary synchronization using fences to ensure that the command buffer
	 * is executed in the correct order. After submission, it retrieves the results
	 * of timestamp and pipeline statistics queries to provide performance metrics.
	 */
	/*
	void CommandBuffer::Submit()
	{
		if (ownedBySwapChain) return;

		Ref<CommandBuffer> instance(this);
		Renderer::Submit([instance]() mutable
		{
			auto device = RenderContext::GetCurrentDevice();

			uint32_t commandBufferIndex = Renderer::GetCurrentRenderThreadFrameIndex() % instance->cmdBuffers.size();

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
            VkCommandBuffer commandBuffer = instance->cmdBuffers[commandBufferIndex];
			submitInfo.pCommandBuffers = &commandBuffer;

			VK_CHECK_RESULT(vkWaitForFences(device->GetDevice(), 1, &instance->waitFences[commandBufferIndex], VK_TRUE, UINT64_MAX))
            VK_CHECK_RESULT(vkResetFences(device->GetDevice(), 1, &instance->waitFences[commandBufferIndex]))

			SEDX_CORE_TRACE_TAG("Renderer", "Submitting Render Command Buffer {}", instance->debugName);

			device->LockQueue();
			VK_CHECK_RESULT(vkQueueSubmit(device->GetGraphicsQueue(), 1, &submitInfo, instance->waitFences[commandBufferIndex]))
			device->UnlockQueue();

			// Retrieve timestamp query results
			vkGetQueryPoolResults(device->GetDevice(),
				instance->timestampQueryPools[commandBufferIndex], 0, instance->availTimeQuery,
				instance->availTimeQuery * sizeof(uint64_t),
				instance->timestampQueryResults[commandBufferIndex].data(),
				sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);

			for (uint32_t i = 0; i < instance->availTimeQuery; i += 2)
			{
                uint64_t startTime = instance->timestampQueryResults[commandBufferIndex][i];
                uint64_t endTime = instance->timestampQueryResults[commandBufferIndex][i + 1];
				float nsTime = endTime > startTime ? (endTime - startTime) * device->GetPhysicalDevice()->GetLimits().timestampPeriod : 0.0f;
                instance->executionGPUTimes[commandBufferIndex][i / 2] = nsTime * 0.000001f; // Time in ms
			}

			// Retrieve pipeline stats results
			vkGetQueryPoolResults(device->GetDevice(), instance->pipelineQueryPools[commandBufferIndex], 0, 1,
				sizeof(PipelineStats), &instance->pipelineStatsQueryResults[commandBufferIndex], sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
		});
	}
	*/

    // -------------------------------------------------------

    /**
     * @brief Begins a timestamp query and returns the query index
     * @return The index of the started timestamp query
     */
    uint32_t CommandBuffer::BeginTimestampQuery()
	{
        uint32_t queryIndex = availTimeQuery;
        availTimeQuery += 2;
        Ref<CommandBuffer> instance(this);
		Renderer::Submit([instance, queryIndex]()
		{
			uint32_t commandBufferIndex = Renderer::GetCurrentRenderThreadFrameIndex() % instance->cmdBuffers.size();
			VkCommandBuffer commandBuffer = instance->cmdBuffers[commandBufferIndex];
			vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, instance->timestampQueryPools[commandBufferIndex], queryIndex);
		});
		return queryIndex;
	}

    /**
	 * @brief Ends a timestamp query for the given query ID
	 * @param queryID The index of the timestamp query to end
	 */
	void CommandBuffer::EndTimestampQuery(uint32_t queryID)
	{
        Ref<CommandBuffer> instance(this);
		Renderer::Submit([instance, queryID]()
		{
            uint32_t commandBufferIndex = Renderer::GetCurrentRenderThreadFrameIndex() % instance->cmdBuffers.size();
            VkCommandBuffer commandBuffer = instance->cmdBuffers[commandBufferIndex];
			vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, instance->timestampQueryPools[commandBufferIndex], queryID + 1);
		});
	}

    // -------------------------------------------------------

    void CommandBuffer::FlushCmdBuffer()
	{
        Ref<CommandBuffer> instance(this);
		Renderer::Submit([instance]()
		{
			uint32_t commandBufferIndex = Renderer::GetCurrentRenderThreadFrameIndex() % instance->cmdBuffers.size();
			VkCommandBuffer commandBuffer = instance->cmdBuffers[commandBufferIndex];
			vkCmdEndRenderPass(commandBuffer);
		});
	}

    // -------------------------------------------------------

    /*
    void CommandBuffer::FlushCmdBuffer(VkCommandBuffer cmdBuffer)
    {
        GetThreadLocalCommandPool()->FlushCmdBuffer(cmdBuffer);
    }
    */

    /*
    void CommandBuffer::FlushCmdBuffer(VkCommandBuffer cmdBuffer, VkQueue queue)
    {
        GetThreadLocalCommandPool()->FlushCmdBuffer(cmdBuffer, queue);
    }
    */

	/*
	Ref<CommandPool> CommandBuffer::GetThreadLocalCommandPool()
    {
        const auto threadID = std::this_thread::get_id();
        SEDX_CORE_VERIFY(CmdPools.contains(threadID));

        return m_Device->CmdPools.at(threadID);
    }
    */

    /*
    Ref<CommandPool> CommandBuffer::GetOrCreateThreadLocalCommandPool()
    {
        const auto threadID = std::this_thread::get_id();
        if (const auto commandPoolIt = cmdPool.find(threadID); commandPoolIt != commandPool.end())
            return commandPoolIt->second;

        auto device = RenderContext::GetCurrentDevice();
        Ref<CommandPool> commandPool = CreateRef<CommandPool>(Ref<VulkanDevice>(device), Queue::Graphics);
        cmdPool[threadID] = commandPool;
        return commandPool;
    }
    */

}

// -------------------------------------------------------

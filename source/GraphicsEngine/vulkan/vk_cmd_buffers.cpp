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
#include <GraphicsEngine/vulkan/vk_cmd_buffers.h>
#include <vulkan/vulkan.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

    /// -------------------------------------------------------

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

    CommandBuffer::CommandBuffer(uint32_t count)
    {
        // Get the device from graphics engine
        GraphicsEngine::Get();
        auto device = GraphicsEngine::Get()->GetLogicDevice();

        // Create command pool with the VulkanDevice
        //cmdPool = CreateRef<CommandPool>(vulkanDevice);

        // Resize command buffers array
        cmdBuffers.resize(count);

        // Allocate command buffers if count > 0
        if (count > 0)
        {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = cmdPool->GetComputeCmdPool();
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = count;

            vkAllocateCommandBuffers(device->GetDevice(), &allocInfo, cmdBuffers.data());
        }
    }
    CommandBuffer::~CommandBuffer() = default;

    void CommandBuffer::Begin(const Queue queue)
    {
        SEDX_ASSERT(currentQueue == Queue::Count, "Already recording a command buffer");
        currentQueue = queue;

        auto &cmd = GetCurrentCommandResources();

        vkWaitForFences(GraphicsEngine::Get()->GetLogicDevice()->GetDevice(), 1, &cmd.fence, VK_TRUE, UINT64_MAX);
        vkResetFences(GraphicsEngine::Get()->GetLogicDevice()->GetDevice(), 1, &cmd.fence);

		if (!cmd.timeStamps.empty())
        {
            vkGetQueryPoolResults(GraphicsEngine::Get()->GetLogicDevice()->GetDevice(), cmd.queryPool,0,cmd.timeStamps.size(),
								  cmd.timeStamps.size() * sizeof(uint64_t),cmd.timeStamps.data(),sizeof(uint64_t),VK_QUERY_RESULT_64_BIT);
            for (int i = 0; i < cmd.timeStampNames.size(); i++)
            {
                const uint64_t begin = cmd.timeStamps[2 * i];
                const uint64_t end = cmd.timeStamps[2 * i + 1];
                timeStampTable[cmd.timeStampNames[i]] = static_cast<float>(end - begin) * GraphicsEngine::GetCurrentDevice()->GetPhysicalDevice()->GetDeviceProperties().limits.timestampPeriod / 1000000.0f;
            }
            cmd.timeStamps.clear();
            cmd.timeStampNames.clear();
        }

		InternalQueue &internalQueue = queues[queue];
        vkResetCommandPool(GraphicsEngine::Get()->GetLogicDevice()->GetDevice(), GetCommandPool()->GetComputeCmdPool(), 0);
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
    VkCommandBuffer CommandBuffer::GetCommandBuffer(const RenderData &frameIndex) const
    {
        SEDX_CORE_ASSERT(frameIndex.frameIndex < cmdBuffers.size());
        return cmdBuffers[frameIndex.frameIndex];
    }

    void CommandBuffer::Submit()
    {
        RenderData renderData;
        const auto &cmd = GetCurrentCommandResources();

        constexpr VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        const VkSwapchainKHR swapchain = GraphicsEngine::Get()->GetSwapChain()->GetSwapchain();

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

	/// -------------------------------------------------------
	/// CommandPool Implementation
	/// -------------------------------------------------------

	/**
	 * @fn CommandPool 
	 * @brief Creates command pools for graphics and compute operations
	 * 
	 * @details This constructor initializes separate command pools for graphics and compute queues.
	 * Command pools are memory managers for command buffers and should typically be created for
	 * each thread that will record commands. This implementation:
	 * 1. Creates a graphics command pool using the graphics queue family
	 * 2. Creates a separate compute command pool if a dedicated compute queue is available
	 * 3. Falls back to using the graphics command pool for compute operations if necessary
	 * 
	 * Command pools are created with the @enum VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT flag,
	 * allowing individual command buffers to be reset for reuse without resetting the entire pool.
	 * 
	 * @param vulkanDevice Reference to the Vulkan device that will own these command pools
	 * 
	 * @note Command pools are specific to queue families and buffers allocated from a pool
	 *       can only be submitted to queues of the matching family.
	 * 
	 * @see vkCreateCommandPool, VkCommandPoolCreateInfo
	 */
    CommandPool::CommandPool(const Ref<VulkanDevice> &vulkanDevice)
	{
		const auto vulkanDeviceHandle = vulkanDevice->GetDevice();
		const auto &queueIndices = vulkanDevice->GetPhysicalDevice()->GetQueueFamilyIndices();

		/// Create graphics command pool
		VkCommandPoolCreateInfo cmdPoolInfo{};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = queueIndices.GetGraphicsFamily();
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		VkResult result = vkCreateCommandPool(vulkanDeviceHandle, &cmdPoolInfo, nullptr, &GraphicsCmdPool);
		if (result != VK_SUCCESS)
            SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create graphics command pool! Error: {}", static_cast<int>(result));

        /// Create compute command pool if compute queue is available
		if (queueIndices.GetComputeFamily() >= 0)
		{
			cmdPoolInfo.queueFamilyIndex = queueIndices.GetComputeFamily();

			result = vkCreateCommandPool(vulkanDeviceHandle, &cmdPoolInfo, nullptr, &ComputeCmdPool);
			if (result != VK_SUCCESS)
			{
				SEDX_CORE_ERROR_TAG("Graphics Engine", "Failed to create compute command pool! Error: {}", static_cast<int>(result));
				ComputeCmdPool = GraphicsCmdPool; 	/// Fall back to using graphics pool for compute operations
			}
		}
	}

	/**
	 * @fn ~CommandPool
	 * @brief Destroys the command pools created by this object
	 * 
	 * @details This destructor properly cleans up command pool resources by:
	 * 1. Checking if the device is still valid before proceeding
	 * 2. Destroying the compute command pool if it's distinct from the graphics pool
	 * 3. Destroying the graphics command pool
	 * 4. Setting all handles to VK_NULL_HANDLE to prevent use-after-free issues
	 * 
	 * The destructor handles the case where the compute and graphics command pools
	 * share the same handle to avoid double-deletion, which would cause a Vulkan
	 * validation error.
	 * 
	 * @note Command pools must be destroyed before their parent device is destroyed.
	 *       When a command pool is destroyed, all command buffers allocated from it
	 *       are implicitly freed and should not be used afterward.
	 * 
	 * @see @fn vkDestroyCommandPool
	 */
	CommandPool::~CommandPool()
	{
        VkDevice vulkanDevice = GraphicsEngine::Get()->GetLogicDevice()->GetDevice(); 
		if (!vulkanDevice)
            return;

		/// Only destroy compute pool if it's different from graphics pool
		if (ComputeCmdPool != VK_NULL_HANDLE && ComputeCmdPool != GraphicsCmdPool)
            vkDestroyCommandPool(vulkanDevice, ComputeCmdPool, nullptr);

        if (GraphicsCmdPool != VK_NULL_HANDLE)
            vkDestroyCommandPool(vulkanDevice, GraphicsCmdPool, nullptr);

        GraphicsCmdPool = VK_NULL_HANDLE;
		ComputeCmdPool = VK_NULL_HANDLE;
	}

	/**
	 * @fn AllocateCommandBuffer
	 * @brief Allocates a command buffer from the appropriate command pool
	 * 
	 * @details This method allocates a new command buffer from either the graphics or compute command pool
	 * based on the provided parameters. It handles proper allocation, initialization, and error checking.
	 * If requested, it will also automatically begin the command buffer with one-time-submit usage flag,
	 * making it ready to record commands immediately.
	 * 
	 * The command buffer is allocated as a primary command buffer, which can be submitted directly to a queue
	 * and can call secondary command buffers. Primary command buffers cannot be called by other command buffers.
	 * 
	 * @param begin If true, the command buffer will be automatically started with @enum VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	 * @param compute If true, the command buffer will be allocated from the compute command pool;
	 *                otherwise, it will be allocated from the graphics command pool
	 * 
	 * @return VkCommandBuffer The newly allocated command buffer, or VK_NULL_HANDLE if allocation failed
	 * 
	 * @note Command buffers allocated with this method should either be freed manually or flushed using 
	 *       the FlushCmdBuffer method, which will handle submission and automatic cleanup.
	 * 
	 * @see FlushCmdBuffer
	 */
	VkCommandBuffer CommandPool::AllocateCommandBuffer(const bool begin, const bool compute) const
	{
        VkDevice vulkanDevice = GraphicsEngine::Get()->GetLogicDevice()->GetDevice(); 
		const VkCommandPool cmdPool = compute ? ComputeCmdPool : GraphicsCmdPool;

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = cmdPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer cmdBuffer;
		VkResult result = vkAllocateCommandBuffers(vulkanDevice, &allocInfo, &cmdBuffer);

		if (result != VK_SUCCESS)
		{
			SEDX_CORE_ERROR("Failed to allocate command buffer! Error: {}", static_cast<int>(result));
			return VK_NULL_HANDLE;
		}

		if (begin)
		{
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			result = vkBeginCommandBuffer(cmdBuffer, &beginInfo);
			if (result != VK_SUCCESS)
			{
				SEDX_CORE_ERROR("Failed to begin command buffer! Error: {}", static_cast<int>(result));
				vkFreeCommandBuffers(vulkanDevice, cmdPool, 1, &cmdBuffer);
				return VK_NULL_HANDLE;
			}
		}

		return cmdBuffer;
	}

	/**
	 * @fn FlushCmdBuffer
	 * @brief Submits a command buffer to the graphics queue and waits for its completion
	 * 
	 * @details This is a convenience method that forwards to the overloaded FlushCmdBuffer function,
	 * using the default graphics queue from the associated device. This method handles the
	 * full submission lifecycle of a command buffer:
	 * 1. Ending the command buffer recording
	 * 2. Creating a fence to synchronize completion
	 * 3. Submitting the command buffer to the appropriate queue
	 * 4. Waiting for execution to complete
	 * 5. Cleaning up resources
	 * 
	 * @param cmdBuffer The Vulkan command buffer to be submitted and executed
	 * 
	 * @note This method is a convenience wrapper that simplifies the common case of submitting
	 *       to the graphics queue. For submissions to other queue types (compute, transfer),
	 *       use the overloaded version with the appropriate queue parameter.
	 * 
	 * @see FlushCmdBuffer(VkCommandBuffer, VkQueue)
	 */
	void CommandPool::FlushCmdBuffer(const VkCommandBuffer cmdBuffer) const
	{
		FlushCmdBuffer(cmdBuffer, device->GetGraphicsQueue());
	}

	/**
	 * @fn FlushCmdBuffer
	 * @brief Submits a command buffer to a specified queue and waits for its completion
	 * 
	 * @details This method handles the complete submission lifecycle of a command buffer:
	 * 1. Ends the command buffer recording with vkEndCommandBuffer
	 * 2. Creates a fence to synchronize execution completion
	 * 3. Submits the command buffer to the specified queue
	 * 4. Waits for execution to complete using the fence
	 * 5. Cleans up resources (fence and command buffer)
	 * 
	 * This implementation uses a fence-based synchronization approach to ensure the GPU has
	 * completely processed the submitted commands before returning. This is suitable for
	 * operations that need to be completed before the CPU continues execution, such as
	 * resource initialization or one-time uploads to the GPU.
	 * 
	 * @param cmdBuffer The command buffer to submit and execute
	 * @param queue The queue to which the command buffer should be submitted
	 * 
	 * @note After this function returns, the command buffer has been freed and should not be used
	 * @note This function will block the calling thread until the GPU completes execution
	 * @note For regular rendering operations that don't need CPU synchronization,
	 *       consider using semaphores instead for better performance
	 * 
	 * @see vkEndCommandBuffer, vkCreateFence, vkQueueSubmit, vkWaitForFences
	 */
	void CommandPool::FlushCmdBuffer(const VkCommandBuffer cmdBuffer, const VkQueue queue) const
	{
        VkDevice vulkanDevice = GraphicsEngine::Get()->GetLogicDevice()->GetDevice();

		if (cmdBuffer == VK_NULL_HANDLE)
		{
			SEDX_CORE_WARN_TAG("Graphics Engine", "Attempted to flush a null command buffer");
			return;
		}

		/// End the command buffer
		VkResult result = vkEndCommandBuffer(cmdBuffer);
		if (result != VK_SUCCESS)
		{
			SEDX_CORE_ERROR("Failed to end command buffer! Error: {}", static_cast<int>(result));
			return;
		}

		/// Create a fence to wait for the command buffer to complete
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

		VkFence fence;
		result = vkCreateFence(vulkanDevice, &fenceInfo, nullptr, &fence);
		if (result != VK_SUCCESS)
		{
			SEDX_CORE_ERROR("Failed to create fence! Error: {}", static_cast<int>(result));
			return;
		}

		/// Submit the command buffer
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;

		result = vkQueueSubmit(queue, 1, &submitInfo, fence);
		if (result != VK_SUCCESS)
		{
			SEDX_CORE_ERROR("Failed to submit command buffer! Error: {}", static_cast<int>(result));
			vkDestroyFence(vulkanDevice, fence, nullptr);
			return;
		}

		/// Wait for the fence
		result = vkWaitForFences(vulkanDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
		if (result != VK_SUCCESS)
		{
			SEDX_CORE_ERROR("Failed to wait for fence! Error: {}", static_cast<int>(result));
		}

		/// Clean up
		vkDestroyFence(vulkanDevice, fence, nullptr);
		vkFreeCommandBuffers(vulkanDevice, GraphicsCmdPool, 1, &cmdBuffer);
	}

} // namespace SceneryEditorX

/// -------------------------------------------------------

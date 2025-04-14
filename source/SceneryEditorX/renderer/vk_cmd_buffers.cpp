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

#include <SceneryEditorX/renderer/vk_cmd_buffers.h>
#include <vulkan/vulkan.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
/*
	inline CommandResources& GetCurrentCommandResources()
	{
        return queues[currentQueue].commands[swapChainCurrentFrame];
    }

	void BeginCmdBuffer(Queue queue)
	{
		ASSERT(_ctx.currentQueue == Queue::Count, "Already recording a command buffer");
		_ctx.currentQueue = queue;
		auto& cmd = _ctx.GetCurrentCommandResources();
		vkWaitForFences(_ctx.device, 1, &cmd.fence, VK_TRUE, UINT64_MAX);
		vkResetFences(_ctx.device, 1, &cmd.fence);

		if (cmd.timeStamps.size() > 0)
		{
		    vkGetQueryPoolResults(_ctx.device, cmd.queryPool, 0, cmd.timeStamps.size(), cmd.timeStamps.size() * sizeof(uint64_t), cmd.timeStamps.data(), sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
		    for (int i = 0; i < cmd.timeStampNames.size(); i++)
			{
		        const uint64_t begin = cmd.timeStamps[2 * i];
		        const uint64_t end = cmd.timeStamps[2 * i + 1];
		        _ctx.timeStampTable[cmd.timeStampNames[i]] = float(end - begin) * _ctx.physicalProperties.limits.timestampPeriod / 1000000.0f;
		    }

		    cmd.timeStamps.clear();
		    cmd.timeStampNames.clear();
		}

		InternalQueue& iqueue = _ctx.queues[queue];
		vkResetCommandPool(_ctx.device, cmd.pool, 0);
		cmd.stagingOffset = 0;
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(cmd.buffer, &beginInfo);

		if (queue != Queue::Transfer)
		{
		    vkCmdResetQueryPool(cmd.buffer, cmd.queryPool, 0, _ctx.timeStampPerPool);
		}
	}
	
	// -------------------------------------------------------
	void EndCmdBuffer(VkSubmitInfo submitInfo)
	{
        auto &cmd = GetCurrentCommandResources();

		vkEndCommandBuffer(cmd.commandBuffer);

		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd.buffer;

		auto result = vkQueueSubmit(submitInfo.queue, 1, &submitInfo, VK_NULL_HANDLE, cmd.fence);
        if (result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR("Failed to submit command buffer: {}", ToString(result));
            ErrMsg("Failed to submit command buffer!");
        }
	}

	void EndCommandBuffer()
    {
        _ctx.EndCommandBuffer({});
        _ctx.currentQueue = SceneryEditorX::Queue::Count;
        _ctx.currentPipeline = {};
    }
	*/
} // namespace SceneryEditorX

// -------------------------------------------------------

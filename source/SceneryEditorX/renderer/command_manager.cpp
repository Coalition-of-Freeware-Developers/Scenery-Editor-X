/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* command_manager.cpp
* -------------------------------------------------------
* Created: 25/8/2025
* -------------------------------------------------------
*/
#include "command_manager.h"
#include "vulkan/vk_cmd_buffers.h"
#include "vulkan/vk_util.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

    /**
	 * @brief Retrieves or creates a thread-local command pool for a specific queue type
	 * 
	 * This method implements a two-level caching strategy for command pools:
	 * 1. First level: Maps std::thread::id to a collection of command pools
	 * 2. Second level: Maps Queue type to the actual CommandPool instance
	 * 
	 * The function ensures thread-safe, per-thread command pool management by using
	 * thread IDs as keys. If a command pool doesn't exist for the current thread and
	 * queue type combination, a new one is created and cached for future use.
	 * 
	 * Thread-local command pools prevent race conditions and eliminate the need for
	 * explicit synchronization when recording commands from multiple threads.
	 * 
	 * @param device The Vulkan device used to create the command pool if needed
	 * @param queueType The type of queue (Graphics, Compute, Transfer, etc.) for the command pool
	 * 
	 * @return A reference-counted pointer to the command pool for this thread and queue type
	 * 
	 * @note This function is thread-safe due to thread-local storage of command pools
	 * @note Command pools are cached and reused for the lifetime of the thread
	 * @note Call ThreadCommandPools::Shutdown() to clean up all cached pools
	 */
	Ref<CommandPool> ThreadCommandPools::Get(const Ref<VulkanDevice> &device, Queue queueType)
	{
	    const auto tid = std::this_thread::get_id();
	    auto &byQueue = Pools()[tid];
	    const auto key = static_cast<int>(queueType);
	    if (auto it = byQueue.find(key); it != byQueue.end() && it->second)
	        return it->second;
	
	    auto pool = CreateRef<CommandPool>(device, queueType);
	    byQueue[key] = pool;
	    return pool;
	}

	/**
	 * @brief Shuts down and releases all thread-local command pools
	 * 
	 * This method performs cleanup of all cached command pools across all threads.
	 * It iterates through the two-level pool map structure and:
	 * 1. Resets all CommandPool references (triggers RefCounted cleanup)
	 * 2. Clears the internal storage maps
	 * 
	 * This function should be called during application shutdown to ensure proper
	 * resource cleanup and prevent Vulkan validation layer warnings about leaked resources.
	 * 
	 * @note This should be called from the main thread after all rendering threads have finished
	 * @note After calling Shutdown(), any subsequent Get() calls will create new pools
	 */
    void ThreadCommandPools::Shutdown()
    {
        auto &all = Pools();
        for (auto &val : all | std::views::values)
        {
            for (auto &byQueue = val; auto &cmdPool : byQueue | std::views::values)
            {
                auto &pool = cmdPool;
                pool.Reset();
            }
        }
        all.clear();
    }

    /**
	 * @brief Provides access to the static thread-to-pool mapping storage
	 * 
	 * Returns a reference to the static storage that maintains the mapping between
	 * thread IDs and their associated command pools. The storage structure is:
	 * - Outer map: thread::id -> PoolMap
	 * - Inner map (PoolMap): Queue type (as int) -> Ref<CommandPool>
	 * 
	 * This static variable ensures that command pools persist for the lifetime of
	 * the application and are accessible from any thread.
	 * 
	 * @return Reference to the static thread-local command pool storage
	 * 
	 * @note The static variable is function-local to ensure thread-safe initialization
	 * @note Access to individual thread entries is inherently thread-safe since each
	 *       thread accesses only its own entry via std::this_thread::get_id()
	 */
    std::unordered_map<std::thread::id, ThreadCommandPools::PoolMap> &ThreadCommandPools::Pools()
    {
        static std::unordered_map<std::thread::id, PoolMap> s_pools;
        return s_pools;
    }

    // -------------------------------------------------------

    CommandManager::CommandManager(Queue *queue, void *cmdPool, const std::string &debugName)
    {
        m_Queue = queue;
        //Ref<CommandBuffer> cmdBuffer = CreateRef<CommandBuffer>(cmdPool, debugName);
    }

    CommandManager::~CommandManager()
    {

    }

    void CommandManager::Begin()
    {

    }

    void CommandManager::Submit(FrameSync *semaphoreWait, bool immediate)
    {

    }

    void CommandManager::ExecutionWait(bool waitTime)
    {

    }

    void CommandManager::PipelineState()
    {

    }

    void CommandManager::Draw(const uint32_t count, const uint32_t vertexStartIdx)
    {

    }

    void CommandManager::DrawIndexed(const uint32_t count, const uint32_t indexOffset, const uint32_t vertexOffset, const uint32_t instanceIdx, const uint32_t instanceCount)
    {

    }

    void CommandManager::ClearPipelineStateRenderTargets(Pipeline &pipelineState)
    {

    }

    void CommandManager::Dispatch(Texture *texture)
    {

    }

    void CommandManager::Dispatch(uint32_t x, uint32_t y, uint32_t z)
    {

    }

    void CommandManager::Blit(Texture *src, Texture *dst, const bool blitMips, const float srcScaling)
    {

    }

    /*
    void CommandManager::Blit(Texture *source, SwapChain *destination)
    {
    }
    */

    /*
    void CommandManager::Copy(Texture *source, Texture *destination, const bool blit_mips)
    {
    }
    */

    /*
    void CommandManager::Copy(Texture *source, SwapChain *destination)
    {
    }
    */

    /*
    void CommandManager::SetViewport(const Viewport &viewport) const
    {
    }
    */

    /*
    void CommandManager::SetScissorRectangle(const xMath::Rectangle &scissor_rectangle) const
    {
    }
    */

    /*
    void CommandManager::SetCullMode(const CullMode cull_mode)
    {
    }
    */

    /*
    void CommandManager::SetVertexBuffer(const Buffer *vertex, Buffer *instance)
    {
    }
    */

    /*
    void CommandManager::SetIndexBuffer(const Buffer *buffer)
    {
    }
    */

    /*
    void CommandManager::SetBuffer(const uint32_t slot, Buffer *buffer) const
    {
    }
    */

    /*
    void CommandManager::UpdateBuffer(Buffer *buffer, const uint64_t offset, const uint64_t size, const void *data)
    {
    }
    */

    /*
    void CommandManager::SetConstantBuffer(const uint32_t slot, Buffer *constant_buffer) const
    {
    }
    */

    /*
    void CommandManager::PushConstants(const uint32_t offset, const uint32_t size, const void *data)
    {
    }
    */

    /*
    void CommandManager::BeginMarker(std::string debugName)
    {
    }
    */

    /*
    void CommandManager::EndMarker()
    {
    }
    */

    /*
    uint32_t CommandManager::BeginTimestamp()
    {
    }*/

    /*
    void CommandManager::EndTimestamp()
    {
    }
    */

    /*
    float CommandManager::GetTimestampResult(const uint32_t index_timestamp)
    {
    }
    */

    /*
    void CommandManager::BeginOcclusionQuery(const uint64_t entity_id)
    {
    }
    */
 
    /*
    void CommandManager::EndOcclusionQuery()
    {
    }
    */

    /*
    bool CommandManager::GetOcclusionQueryResult(const uint64_t entity_id)
    {
    }*/

    /*
    void CommandManager::UpdateOcclusionQueries()
    {
    }
    */

    /*
    void CommandManager::BeginTimeBlock(std::string debugName, const bool gpu_marker, const bool gpu_timing)
    {
    }
    */

    /*
    void CommandManager::EndTimeBlock()
    {
    }
    */

    /*
    void CommandManager::InsertBarrier(void *image,
                                       const VkFormat format,
                                       const uint32_t mipIdx,
                                       const uint32_t mip_range,
                                       const uint32_t arrayLength,
                                       const Layout::ImageLayout layoutNew)
    {
    }
    */

    /*
    void CommandManager::InsertBarrierReadWrite(Buffer *buffer)
    {
    }
    */

    /*
    void CommandManager::InsertPendingBarrierGroup()
    {
    }
    */

    /*
    void CommandManager::RenderPassEnd()
    {
    }
    */

    /*
    void CommandManager::RemoveLayout(void *image)
    {
    }
    */

    /*
    Layout::ImageLayout CommandManager::GetImageLayout(void *image, uint32_t mipIdx)
    {
    }
    */

    /*
    void CommandManager::PreDraw()
    {
    }
    */

    /*
    void CommandManager::RenderPassBegin()
    {
    }
    */

    /*
	VkCommandBuffer CommandPool::AllocateCommandBuffer(bool begin, bool compute) const
	{
		const auto deviceRef = RenderContext::GetCurrentDevice();
		SEDX_CORE_ASSERT(deviceRef, "VulkanDevice must be valid to allocate command buffers");
		const VkDevice device = deviceRef->GetDevice();
		const VkCommandPool pool = compute ? ComputeCmdPool : GraphicsCmdPool;

		VkCommandBufferAllocateInfo ai{};
		ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		ai.commandPool = pool;
		ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		ai.commandBufferCount = 1;

		VkCommandBuffer cmd = VK_NULL_HANDLE;
		if (VkResult res = vkAllocateCommandBuffers(device, &ai, &cmd); res != VK_SUCCESS)
		{
			SEDX_CORE_ERROR_TAG("VULKAN", "vkAllocateCommandBuffers failed (err {0})", res);
			return VK_NULL_HANDLE;
		}

		if (begin)
		{
			VkCommandBufferBeginInfo bi{};
			bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			if (VkResult res = vkBeginCommandBuffer(cmd, &bi); res != VK_SUCCESS)
			{
				SEDX_CORE_ERROR_TAG("VULKAN", "vkBeginCommandBuffer failed (err {0})", res);
				vkFreeCommandBuffers(device, pool, 1, &cmd);
				return VK_NULL_HANDLE;
			}
		}

		return cmd;
	}

	void CommandPool::FlushCmdBuffer(VkCommandBuffer cmdBuffer) const
	{
		const auto deviceRef = RenderContext::GetCurrentDevice();
		SEDX_CORE_ASSERT(deviceRef, "No VulkanDevice available");
		VkDevice device = deviceRef->GetDevice();
		VkQueue queue = deviceRef->GetGraphicsQueue();

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));

		VkSubmitInfo si{};
		si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		si.commandBufferCount = 1;
		si.pCommandBuffers = &cmdBuffer;

		VkFenceCreateInfo fi{};
		fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		VkFence fence = VK_NULL_HANDLE;
		VK_CHECK_RESULT(vkCreateFence(device, &fi, nullptr, &fence));

		if (VkResult res = vkQueueSubmit(queue, 1, &si, fence); res != VK_SUCCESS)
			SEDX_CORE_ERROR_TAG("VULKAN", "vkQueueSubmit failed (err {0})", res);

		vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
		vkDestroyFence(device, fence, nullptr);

		vkFreeCommandBuffers(device, GraphicsCmdPool, 1, &cmdBuffer);
	}

	void CommandPool::FlushCmdBuffer(VkCommandBuffer cmdBuffer, VkQueue queue) const
	{
		const auto deviceRef = RenderContext::GetCurrentDevice();
		SEDX_CORE_ASSERT(deviceRef, "No VulkanDevice available");
		VkDevice device = deviceRef->GetDevice();

		if (cmdBuffer == VK_NULL_HANDLE)
			return;

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));

		VkSubmitInfo si{};
		si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		si.commandBufferCount = 1;
		si.pCommandBuffers = &cmdBuffer;

		VkFenceCreateInfo fi{};
		fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		VkFence fence = VK_NULL_HANDLE;
		VK_CHECK_RESULT(vkCreateFence(device, &fi, nullptr, &fence));

		if (VkResult res = vkQueueSubmit(queue, 1, &si, fence); res != VK_SUCCESS)
			SEDX_CORE_ERROR_TAG("VULKAN", "vkQueueSubmit (explicit queue) failed (err {0})", res);

		vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
		vkDestroyFence(device, fence, nullptr);

		vkFreeCommandBuffers(device, GraphicsCmdPool, 1, &cmdBuffer);
	}
	*/

}

// -------------------------------------------------------

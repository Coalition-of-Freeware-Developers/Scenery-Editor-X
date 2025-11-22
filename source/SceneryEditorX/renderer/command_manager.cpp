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
#include "debug/debugging.h"
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
    namespace timestamp
    {
        constexpr uint32_t QueryCount = 256;
        std::array<uint64_t, QueryCount> data;

		void Update(void *queryPool)
		{
            VkDevice device = RenderContext::GetCurrentDevice()->GetDevice();
		    if (Debugging::IsGpuTimingEnabled())
                vkGetQueryPoolResults(device,                               // Device
                                      static_cast<VkQueryPool>(queryPool),	// Query Pool
                                      0,                                    // First Query
                                      QueryCount,                           // Query Count
                                      QueryCount * sizeof(uint64_t),        // Data Size
                                      data.data(),                          // pData
                                      sizeof(uint64_t),                     // Stride
                                      VK_QUERY_RESULT_64_BIT                // Flags
                );
        }

		void Reset(void *cmdList, void *&queryPool)
		{
		    if (Debugging::IsGpuTimingEnabled())
                vkCmdResetQueryPool(static_cast<VkCommandBuffer>(cmdList), static_cast<VkQueryPool>(queryPool), 0, QueryCount);
        }

    } // namespace timestamp

    namespace occlusion
    {
		uint32_t index = 0;
		uint32_t indexActive = 0;
		bool occlusionQueryActive = false;
        constexpr uint32_t QueryCount = 4096;
		std::array<uint64_t, QueryCount> data;
		std::unordered_map<uint64_t, uint32_t> idToIndex;

		void Update(void *queryPool)
		{
            VkDevice device = RenderContext::GetCurrentDevice()->GetDevice();
		    vkGetQueryPoolResults(device,												// Device
		                          static_cast<VkQueryPool>(queryPool),					// Query Pool
		                          0,													// First Query
		                          QueryCount,											// Query Count
		                          QueryCount * sizeof(uint64_t),						// Data Size
		                          data.data(),											// pData
		                          sizeof(uint64_t),										// Stride
		                          VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_PARTIAL_BIT	// Flags
		    );
		}

		void Reset(void *cmdList, void *&queryPool)
		{
		    vkCmdResetQueryPool(static_cast<VkCommandBuffer>(cmdList),
		                        static_cast<VkQueryPool>(queryPool),
		                        0,
		                        QueryCount);
		}

    } // namespace occlusion

    void Init(void *&poolTimestamp, void *&poolOcclusion, void *&poolPipelineStats)
	{
        VkDevice device = RenderContext::GetCurrentDevice()->GetDevice();
        // Timestamps
        if (Debugging::IsGpuTimingEnabled())
        {
            VkQueryPoolCreateInfo query_pool_info = {};
            query_pool_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
            query_pool_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
            query_pool_info.queryCount = timestamp::QueryCount;

            auto queryPool = reinterpret_cast<VkQueryPool *>(&poolTimestamp);
            SEDX_ASSERT(vkCreateQueryPool(device, &query_pool_info, nullptr, queryPool));
            RenderContext::Get()->GetLogicDevice()->SetDebugName(poolTimestamp, ResourceType::None,"query_pool_timestamp");

            timestamp::data.fill(0);
        }

        // Occlusion
        {
            VkQueryPoolCreateInfo query_pool_info = {};
            query_pool_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
            query_pool_info.queryType = VK_QUERY_TYPE_OCCLUSION;
            query_pool_info.queryCount = occlusion::QueryCount;

            auto queryPool = reinterpret_cast<VkQueryPool *>(&poolOcclusion);
            SEDX_ASSERT(vkCreateQueryPool(device, &query_pool_info, nullptr, queryPool));
            RenderContext::Get()->GetLogicDevice()->SetDebugName(poolTimestamp, ResourceType::None,"query_pool_occlusion");

            occlusion::data.fill(0);
        }  
	}

    void Shutdown(void *&poolTimestamp, void *&poolOcclusion, void *&poolPipelineStatistics)
    {
        auto device = RenderContext::Get()->GetLogicDevice();
        //QueueManager::DeletionQueueAdd(ResourceType::QueryPool, poolTimestamp);
        //QueueManager::DeletionQueueAdd(ResourceType::QueryPool, poolOcclusion);
        //QueueManager::DeletionQueueAdd(ResourceType::QueryPool, poolPipelineStatistics);
    }

    // -------------------------------------------------------

    CommandManager::CommandManager(Queue *queue, void *cmdPool, const std::string &debugName)
    {
        //m_Queue = queue;

        // Command Buffer
        {
            VkDevice device = RenderContext::GetCurrentDevice()->GetDevice();
            VkCommandBufferAllocateInfo allocateInfo = {};
            allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocateInfo.commandPool = static_cast<VkCommandPool>(cmdPool);
            allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocateInfo.commandBufferCount = 1;

            // Allocate Command Buffer
            SEDX_ASSERT(vkAllocateCommandBuffers(device, &allocateInfo, reinterpret_cast<VkCommandBuffer *>(&m_Resource)));
            RenderContext::Get()->GetLogicDevice()->SetDebugName(m_Resource, ResourceType::None, debugName.c_str());
        }

		// Semaphores
        m_RenderingCompleteSemaphore			= CreateRef<FrameSync>(FrameSyncType::Semaphore, debugName.c_str());
		m_RenderingCompleteSemaphoreTimeline	= CreateRef<FrameSync>(FrameSyncType::SemaphoreTimeline, debugName.c_str());
		Init(m_QueryPool_Timestamps, m_QueryPool_Occlusion, m_QueryPool_PipelineStats);
    }

    CommandManager::~CommandManager()
    {
        Shutdown(m_QueryPool_Timestamps, m_QueryPool_Occlusion, m_QueryPool_PipelineStats);
    }

    /*
    void CommandManager::Begin()
    {
        SEDX_ASSERT(m_State == CommandState::IDLE);

        // Begin Command Buffer
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        SEDX_ASSERT(vkBeginCommandBuffer(static_cast<VkCommandBuffer>(m_Resource), &beginInfo) == VK_SUCCESS,"Failed to begin command buffer");

        // enable breadcrumbs for this command list
        if (Debugging::IsBreadcrumbsEnabled())
        {
            RHI_VendorTechnology::Breadcrumbs_RegisterCommandList(this, m_Queue, m_name.c_str());
        }

        // set states
        m_State = CommandState::RECORDING;
        m_Pso = PipelineState();
        m_CullMode = CullMode::Max;

        // set dynamic states
        if (m_Queue->GetType() == Queue::Graphics)
        {
            // cull mode
            SetCullMode(CullMode::Back);

            // scissor rectangle
            xMath::Rectangle scissorRect;
            scissorRect.x = 0.0f;
            scissorRect.y = 0.0f;
            scissorRect.width = static_cast<float>(m_Pso.GetWidth());
            scissorRect.height = static_cast<float>(m_Pso.GetHeight());
            SetScissorRectangle(scissorRect);
        }

        // queries
        if (m_Queue->GetType() != Queue::Transfer)
        {
            if (m_Timestamp_Index != 0)
            {
                timestamp::Update(m_QueryPool_Timestamps);
            }

            // queries need to be reset before they are first used and they
            // also need to be reset after every use, so we just reset them always
            m_Timestamp_Index = 0;
            timestamp::Reset(m_Resource, m_QueryPool_Timestamps);
            occlusion::Reset(m_Resource, m_QueryPool_Occlusion);
        }

    }
    */

    /*
    void CommandManager::Submit(FrameSync *semaphoreWait, bool immediate)
    {
        SEDX_ASSERT(m_State == CommandState::RECORDING);

        // End
        RenderPassEnd();
        SEDX_ASSERT(vkEndCommandBuffer(static_cast<VkCommandBuffer>(m_Resource)));

        // immediate command lists wait on the CPU using the timeline semaphore
        FrameSync *semaphoreBinary = is_immediate ? nullptr : m_RenderingCompleteSemaphore.Get();

        m_Queue->Submit(static_cast<VkCommandBuffer>(m_Resource),	// cmd buffer
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,          // wait flags
                        semaphoreWait,                              // wait semaphore
                        semaphoreBinary,                           // signal semaphore
                        m_RenderingCompleteSemaphoreTimeline.Get()	// signal semaphore
        );

        if (semaphoreWait)
            semaphoreWait->SetUserCmdList(this);

        m_State = CommandState::SUBMITTED;
    }
    */

    void CommandManager::ExecutionWait(bool waitTime)
    {

    }

    /*
    void CommandManager::SetPipelineState()
    {
        SEDX_ASSERT(m_State == CommandState::RECORDING);

        // early exit if the pipeline state hasn't changed
        pso.Prepare();
        if (m_Pso.GetHash() == pso.GetHash())
            return;

        // determine if the new render pass should clear the render targets or not
        if ((m_Pso.shaders[ShaderStage::Stage::Vertex] != nullptr &&
             m_Pso.shaders[ShaderStage::Stage::Vertex] == pso.shaders[ShaderStage::Stage::Vertex]) &&
            m_Pso.render_target_array_index == pso.render_target_array_index)
        {
            m_Load_Depth_RenderTarget = (pso.render_target_depth_texture == m_Pso.render_target_depth_texture);
            for (uint32_t i = 0; i < m_MaxRenderTargetCount; i++)
            {
                m_LoadColorRenderTargets[i] =
                    (pso.render_target_color_textures[i] == m_Pso.render_TargetColorTextures[i]);
            }
        }
        else
        {
            m_Load_Depth_RenderTarget = false;
            for (uint32_t i = 0; i < m_MaxRenderTargetCount; i++)
            {
                m_LoadColorRenderTargets[i] = false;
            }
        }

        // get (or create) a pipeline which matches the requested pipeline state
        m_Pso = pso;
        RenderContext::GetCurrentDevice()->GetOrCreatePipeline(m_Pso, m_pipeline, m_descriptor_layout_current);

        RenderPassBegin();

        // set pipeline
        {
            // get vulkan pipeline object
            SEDX_ASSERT(m_Pipeline != nullptr);
            VkPipeline vk_pipeline = static_cast<VkPipeline>(m_Pipeline->GetResource());
            SEDX_ASSERT(vk_pipeline != nullptr);

            // bind
            VkPipelineBindPoint pipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
            pipelineBindPoint = m_Pso.IsGraphics() ? VK_PIPELINE_BIND_POINT_GRAPHICS : pipelineBindPoint;
            pipelineBindPoint = m_Pso.IsRayTracing() ? VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR : pipelineBindPoint;
            vkCmdBindPipeline(static_cast<VkCommandBuffer>(m_Resource), pipelineBindPoint, vk_pipeline);
            Profiler::m_rhi_bindings_pipeline++;

            // set some dynamic states
            if (m_Pso.IsGraphics())
            {
                // cull mode
                if (m_Pso.rasterizer_state->GetPolygonMode() == PolygonMode::Line)
                    SetCullMode(CullMode::None);

                // scissor rectangle
                xMath::Rectangle scissorRect;
                scissorRect.x = 0.0f;
                scissorRect.y = 0.0f;
                scissorRect.width = static_cast<float>(m_Pso.GetWidth());
                scissorRect.height = static_cast<float>(m_Pso.GetHeight());
                SetScissorRectangle(scissorRect);

                // vertex and index buffer state
                m_BufferID_Index = 0;
                m_BufferID_Vertex = 0;
                m_BufferID_Instance = 0;
            }

            if (Debugging::IsBreadcrumbsEnabled())
                RHI_VendorTechnology::Breadcrumbs_SetPipelineState(this, m_pipeline);
        }

        // Bind Descriptors
        {
            // Set Bindless Descriptors
            descriptor_sets::set_bindless(m_Pso, m_Resource, m_pipeline->GetRhiResourceLayout());

            // Set standard resources (dynamic descriptors)
            Renderer::SetStandardResources(this);
            descriptor_sets::set_dynamic(m_Pso,
                                         m_Resource,
                                         m_Pipeline->GetResourceLayout(),
                                         m_descriptor_layout_current);
        }
    }
    */

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
        /*SEDX_ASSERT(m_State == CommandState::RECORDING);
        PreDraw();
        vkCmdDispatch(static_cast<VkCommandBuffer>(m_Resource), x, y, z);*/
    }

    /*
    void CommandManager::Blit(Texture *src, Texture *dst, const bool blitMips, const float srcScaling)
    {
        SEDX_ASSERT(src && dst, "Source and destination textures cannot be null");
        SEDX_ASSERT((src->GetFlags() & TextureClearBlit) != 0, "Blit requires the texture to be created with the RHI_Texture_ClearOrBlit flag");
        SEDX_ASSERT((dst->GetFlags() & Texture_ClearBlit) != 0, "Blit requires the texture to be created with the RHI_Texture_ClearOrBlit flag");
        SEDX_ASSERT(src->GetChannelCount() == dst->GetChannelCount(), "Source and destination must have matching channel counts for blit compatibility");
        SEDX_ASSERT(src->GetBitsPerChannel() == dst->GetBitsPerChannel() || (src->IsColorFormat() && dst->IsColorFormat()), "Source and destination bit depths must match or be convertible color formats");
        SEDX_ASSERT(!src->IsDepthFormat() || !dst->IsDepthFormat() || src->GetFormat() == dst->GetFormat(), "Depth formats must be identical for blit");
        if (blitMips)
            SEDX_ASSERT(src->GetMipCount() == dst->GetMipCount(),
                        "If the mips are blitted, then the mip count between the source and the destination textures "
                        "must match");

        // compute a blit region for each mip
        std::array<VkOffset3D, MAX_MIP_COUNT> blitOffsetsSource = {};
        std::array<VkOffset3D, MAX_MIP_COUNT> blitOffsetsDestination = {};
        std::array<VkImageBlit, MAX_MIP_COUNT> blitRegions = {};
        uint32_t blitRegionCount = blitMips ? src->GetMipCount() : 1;
        for (uint32_t mipIndex = 0; mipIndex < blitRegionCount; mipIndex++)
        {
            VkOffset3D &source_blit_size = blitOffsetsSource[mipIndex];
            source_blit_size.x = static_cast<int32_t>(src->GetWidth() * srcScaling) >> mipIndex;
            source_blit_size.y = static_cast<int32_t>(src->GetHeight() * srcScaling) >> mipIndex;
            source_blit_size.z = 1;

            VkOffset3D &destination_blit_size = blitOffsetsDestination[mipIndex];
            destination_blit_size.x = dst->GetWidth() >> mipIndex;
            destination_blit_size.y = dst->GetHeight() >> mipIndex;
            destination_blit_size.z = 1;

            VkImageBlit &blit_region = blitRegions[mipIndex];
            blit_region.srcSubresource.mipLevel = mipIndex;
            blit_region.srcSubresource.baseArrayLayer = 0;
            blit_region.srcSubresource.layerCount = 1;
            blit_region.srcSubresource.aspectMask = get_spect_mask(src->GetFormat());
            blit_region.srcOffsets[0] = {0, 0, 0};
            blit_region.srcOffsets[1] = source_blit_size;
            blit_region.dstSubresource.mipLevel = mipIndex;
            blit_region.dstSubresource.baseArrayLayer = 0;
            blit_region.dstSubresource.layerCount = 1;
            blit_region.dstSubresource.aspectMask = get_aspect_mask(dst->GetFormat());
            blit_region.dstOffsets[0] = {0, 0, 0};
            blit_region.dstOffsets[1] = destination_blit_size;
        }

        // save the initial layouts
        std::array<Layout::ImageLayout, MAX_MIP_COUNT> layouts_initial_source = src->GetLayouts();
        std::array<Layout::ImageLayout, MAX_MIP_COUNT> layouts_initial_destination = dst->GetLayouts();

        // transition to blit appropriate layouts
        src->SetLayout(Layout::ImageLayout::Transfer_Source, this);
        dst->SetLayout(Layout::ImageLayout::Transfer_Destination, this);

        VkFilter filter =
            (src->IsDepthFormat() || dst->IsDepthFormat() ||
             (src->GetWidth() == dst->GetWidth() && src->GetHeight() == dst->GetHeight())) ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;

        // blit
        vkCmdBlitImage(static_cast<VkCommandBuffer>(m_Resource),
                       static_cast<VkImage>(src->GetResource()),
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       static_cast<VkImage>(dst->GetResource()),
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       blitRegionCount,
                       &blitRegions[0],
                       filter);

        // transition to the initial layouts
        if (blitMips)
        {
            for (uint32_t i = 0; i < src->GetMipCount(); i++)
            {
                src->SetLayout(layouts_initial_source[i], this, i, 1);
                dst->SetLayout(layouts_initial_destination[i], this, i, 1);
            }
        }
        else
        {
            src->SetLayout(layouts_initial_source[0], this);
            dst->SetLayout(layouts_initial_destination[0], this);
        }

    }
    */

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

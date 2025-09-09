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
//#include "command_manager.h"
//#include "vulkan/vk_util.h"

/// -------------------------------------------------------

/*
namespace SceneryEditorX
{
    namespace
    {
        std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    }

    namespace queries
    {
        namespace timestamp
        {
            const uint32_t query_count = 128;
            std::array<uint64_t, query_count> data;

            void update(void* query_pool)
            {
                if (Debugging::IsGpuTimingEnabled())
                {
                    vkGetQueryPoolResults(
						RenderContext::Get()->GetLogicDevice()->GetDevice(), // device
                        static_cast<VkQueryPool>(query_pool), // queryPool
                        0,                                    // firstQuery
                        query_count,                          // queryCount
                        query_count * sizeof(uint64_t),       // dataSize
                        data.data(),                          // pData
                        sizeof(uint64_t),                     // stride
                        VK_QUERY_RESULT_64_BIT                // flags
                    );
                }
            }

            void reset(void* cmd_list, void*& query_pool)
            {
                if (Debugging::IsGpuTimingEnabled())
                {
                    vkCmdResetQueryPool(static_cast<VkCommandBuffer>(cmd_list), static_cast<VkQueryPool>(query_pool), 0, query_count);
                }
            }
        }

        namespace occlusion
        {
            uint32_t index              = 0;
            uint32_t index_active       = 0;
            bool occlusion_query_active = false;
            const uint32_t query_count  = 4096;
            std::array<uint64_t, query_count> data;
            std::unordered_map<uint64_t, uint32_t> id_to_index;

            void update(void* query_pool)
            {
                vkGetQueryPoolResults(RenderContext::Get()->GetLogicDevice()->GetDevice(),  // device
                    static_cast<VkQueryPool>(query_pool),                // queryPool
                    0,                                                   // firstQuery
                    query_count,                                         // queryCount
                    query_count * sizeof(uint64_t),                      // dataSize
                    data.data(),                                         // pData
                    sizeof(uint64_t),                                    // stride
                    VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_PARTIAL_BIT // flags
                );
            }

            void reset(void* cmd_list, void*& query_pool)
            {
                vkCmdResetQueryPool(static_cast<VkCommandBuffer>(cmd_list), static_cast<VkQueryPool>(query_pool), 0, query_count);
            }

        }

        void initialize(void*& pool_timestamp, void*& pool_occlusion, void*& pool_pipeline_statistics)
        {
            // timestamps
            if (Debugging::IsGpuTimingEnabled())
            {
                VkQueryPoolCreateInfo query_pool_info = {};
                query_pool_info.sType                 = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
                query_pool_info.queryType             = VK_QUERY_TYPE_TIMESTAMP;
                query_pool_info.queryCount            = timestamp::query_count;

                auto query_pool = reinterpret_cast<VkQueryPool*>(&pool_timestamp);
                VK_CHECK_RESULT(vkCreateQueryPool(RenderContext::Get()->GetLogicDevice()->GetDevice(), &query_pool_info, nullptr, query_pool));
                RHI_Device::SetResourceName(pool_timestamp, ResourceType::QueryPool, "query_pool_timestamp");

                timestamp::data.fill(0);
            }

            // occlusion
            {
                VkQueryPoolCreateInfo query_pool_info = {};
                query_pool_info.sType                 = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
                query_pool_info.queryType             = VK_QUERY_TYPE_OCCLUSION;
                query_pool_info.queryCount            = occlusion::query_count;

                auto query_pool = reinterpret_cast<VkQueryPool*>(&pool_occlusion);
                VK_CHECK_RESULT(vkCreateQueryPool(RenderContext::Get()->GetLogicDevice()->GetDevice(), &query_pool_info, nullptr, query_pool));
                RHI_Device::SetResourceName(pool_occlusion, ResourceType::QueryPool, "query_pool_occlusion");

                occlusion::data.fill(0);
            }
        }

        void Shutdown(void*& pool_timestamp, void*& pool_occlusion, void*& pool_pipeline_statistics)
        {
            VulkanDevice::DeletionQueueAdd(ResourceType::QueryPool, pool_timestamp);
            VulkanDevice::DeletionQueueAdd(ResourceType::QueryPool, pool_occlusion);
            VulkanDevice::DeletionQueueAdd(ResourceType::QueryPool, pool_pipeline_statistics);
        }
    }

    CommandManager::CommandManager(Queue *queue, void *cmd_pool, const std::string &debugName)
    {
        m_queue = queue;

        // command buffer
        {
            // define
            VkCommandBufferAllocateInfo allocate_info = {};
            allocate_info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocate_info.commandPool                 = static_cast<VkCommandPool>(cmd_pool);
            allocate_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocate_info.commandBufferCount          = 1;

            // allocate
            VK_CHECK_RESULT(vkAllocateCommandBuffers(RenderContext::Get()->GetLogicDevice()->GetDevice(), &allocate_info, reinterpret_cast<VkCommandBuffer*>(&m_resource)))

            // name
            RHI_Device::SetResourceName(static_cast<void*>(m_resource), ResourceType::CommandList, debugName);
            m_object_name = debugName;
        }

        // semaphores
        m_rendering_complete_semaphore          = CreateRef<FrameSync>(FrameSyncType::SyncSemaphore, std::string(debugName) + "_binary");
        m_rendering_complete_semaphore_timeline = CreateRef<FrameSync>(FrameSyncType::SyncSemaphoreTimeline, std::string(debugName) + "timeline");

        queries::initialize(m_queryPool_timestamps, m_queryPool_occlusion, m_rhi_query_pool_pipeline_statistics);
    }

    CommandManager::~CommandManager()
    {
        queries::Shutdown(m_queryPool_timestamps,
                          m_queryPool_occlusion,
                          m_rhi_query_pool_pipeline_statistics);
    }

    void CommandManager::WaitForExecution(const bool log_wait_time)
    {
        SEDX_ASSERT(m_state == CommandState::Submitted, "the command list hasn't been submitted, can't wait for it.");

        if (log_wait_time)
        { 
            start_time = std::chrono::high_resolution_clock::now();
        }

        // wait
        uint64_t timeout_nanoseconds = 60'000'000'000; // 60 seconds
        m_rendering_complete_semaphore_timeline->Wait(timeout_nanoseconds);
        m_state = CommandState::Idle;

        if (log_wait_time)
        {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = duration_cast<std::chrono::microseconds>(end_time - start_time).count();
            SEDX_CORE_INFO("wait time: %lld microseconds\n", duration);
        }
    }

	void CommandManager::Dispatch(Texture *texture)
	{
	    // compute dimensions and dispatch
        constexpr uint32_t thread_group_count = 8;
	    const uint32_t thread_group_count_x = (texture->GetWidth() + thread_group_count - 1) / thread_group_count;
	    const uint32_t thread_group_count_y = (texture->GetHeight() + thread_group_count - 1) / thread_group_count;
	    const uint32_t thread_group_count_z = (texture->GetType() == TextureType::Texture3D) ? (texture->GetDepth() + thread_group_count - 1) / thread_group_count : 1;
	
	    Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z);
	
	    // synchronize writes to the texture
	    if (GetImageLayout(texture->GetResource(), 0) == Layout::ImageLayout::General)
	    {
	        InsertBarrierReadWrite(texture, BarrierType::EnsureWriteThenRead);
	    }
	}

    void CommandManager::SetViewport(const Viewport &viewport) const
    {
        SEDX_ASSERT(m_state == CommandState::Recording);
        SEDX_ASSERT(viewport.width != 0);
        SEDX_ASSERT(viewport.height != 0);

        VkViewport vk_viewport = {};
        vk_viewport.x = viewport.x;
        vk_viewport.y = viewport.y;
        vk_viewport.width = viewport.width;
        vk_viewport.height = viewport.height;
        vk_viewport.minDepth = viewport.depth_min;
        vk_viewport.maxDepth = viewport.depth_max;

        vkCmdSetViewport(static_cast<VkCommandBuffer>(m_resource),	// commandBuffer
                         0,                                         // firstViewport
                         1,                                         // viewportCount
                         &vk_viewport                               // pViewports
        );
    }

    void CommandManager::PushConstants(const uint32_t offset, const uint32_t size, const void *data)
    {
       auto device = RenderContext::Get()->GetLogicDevice();
       SEDX_ASSERT(m_state == CommandListState::Recording);
       SEDX_ASSERT(size <= device->GetPhysicalDevice()->GetLimits().maxPushConstantsSize);

        uint32_t stages = 0;

        if (m_pso.shaders[ShaderStage::Stage::Compute])
            stages |= VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;

        if (m_pso.shaders[ShaderStage::Stage::Vertex])
            stages |= VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;

        if (m_pso.shaders[ShaderStage::Stage::TesselationControl])
            stages |= VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

        if (m_pso.shaders[ShaderStage::Stage::TesselationEval])
            stages |= VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

        if (m_pso.shaders[ShaderStage::Stage::Fragment])
            stages |= VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

        vkCmdPushConstants(
            static_cast<VkCommandBuffer>(m_resource),
            static_cast<VkPipelineLayout>(m_pipeline->GetResourceLayout()),
            stages, offset, size, data
        );
    }

}
*/

/// -------------------------------------------------------

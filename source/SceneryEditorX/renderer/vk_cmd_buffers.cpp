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
#include <vulkan/vulkan.h>
#include "vk_util.h"
#include "SceneryEditorX/renderer/render_context.h"
#include "SceneryEditorX/renderer/renderer.h"
#include "SceneryEditorX/renderer/command_manager.h"

#include <SceneryEditorX/logging/logging.hpp>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    /// -------------------------------------------------------
    /// CommandBufferManager Static Members
    /// -------------------------------------------------------

    std::mutex CommandBufferManager::s_Mutex;
    std::map<std::thread::id, std::map<Queue, Ref<CommandPool>>> CommandBufferManager::s_ThreadCommandPools;
    std::atomic<uint32_t> CommandBufferManager::s_GlobalFrameIndex{0};
    bool CommandBufferManager::s_IsInitialized = false;

    /// -------------------------------------------------------
    /// CommandBuffer Implementation
    /// -------------------------------------------------------

    CommandBuffer::CommandBuffer(const CommandBufferInfo& info)
        : m_DebugName(info.debugName)
        , m_QueueType(info.queueType)
        , m_BufferCount(info.count)
        , m_IsPrimary(info.isPrimary)
    {
        SEDX_CORE_INFO_TAG("COMMAND_BUFFER", "Creating command buffer: {}", m_DebugName);

        m_VkDevice = RenderContext::Get()->GetLogicDevice();
        SEDX_CORE_ASSERT(m_VkDevice, "VulkanDevice must be valid");

        // If count is 0, use frames in flight
        if (m_BufferCount == 0)
        {
            m_BufferCount = Renderer::GetRenderData().framesInFlight;
        }

        SEDX_CORE_VERIFY(m_BufferCount > 0, "Command buffer count must be greater than 0");

        if (!Initialize())
        {
            SEDX_CORE_ERROR_TAG("COMMAND_BUFFER", "Failed to initialize command buffer: {}", m_DebugName);
            throw std::runtime_error("CommandBuffer initialization failed");
        }

        SEDX_CORE_INFO_TAG("COMMAND_BUFFER", "Command buffer created successfully: {} ({} buffers)", 
                          m_DebugName, m_BufferCount);
    }

    CommandBuffer::~CommandBuffer()
    {
        SEDX_CORE_INFO_TAG("COMMAND_BUFFER", "Destroying command buffer: {}", m_DebugName);

        if (!m_VkDevice)
            return;

        const VkDevice device = m_VkDevice->GetDevice();

        // Wait for any pending operations
        if (m_IsSubmitted)
        {
            WaitForCompletion();
        }

        // Destroy query pools
        for (auto& pool : m_TimestampQueryPools)
        {
            if (pool != VK_NULL_HANDLE)
            {
                vkDestroyQueryPool(device, pool, nullptr);
            }
        }

        for (auto& pool : m_PipelineStatsQueryPools)
        {
            if (pool != VK_NULL_HANDLE)
            {
                vkDestroyQueryPool(device, pool, nullptr);
            }
        }

        // Destroy fences
        for (auto& fence : m_CompletionFences)
        {
            if (fence != VK_NULL_HANDLE)
            {
                vkDestroyFence(device, fence, nullptr);
            }
        }

        // Command buffers are freed automatically when command pool is destroyed
        SEDX_CORE_DEBUG_TAG("COMMAND_BUFFER", "Command buffer destroyed: {}", m_DebugName);
    }

    bool CommandBuffer::Initialize()
    {
        SEDX_PROFILE_SCOPE("CommandBuffer::Initialize");

        try
        {
            // Get or create command pool for this thread and queue type
            m_CommandPool = CommandBufferManager::GetThreadLocalCommandPool(m_QueueType);
            if (!m_CommandPool)
            {
                SEDX_CORE_ERROR_TAG("COMMAND_BUFFER", "Failed to get command pool for queue type: {}", static_cast<int>(m_QueueType));
                return false;
            }

            const VkDevice device = m_VkDevice->GetDevice();

            // Allocate command buffers
            VkCommandPool cmdPool = (m_QueueType == Queue::Compute) 
                                  ? m_CommandPool->GetComputeCmdPool() 
                                  : m_CommandPool->GetGraphicsCmdPool();

            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = cmdPool;
            allocInfo.level = m_IsPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
            allocInfo.commandBufferCount = m_BufferCount;

            m_CommandBuffers.resize(m_BufferCount);
            VkResult result = vkAllocateCommandBuffers(device, &allocInfo, m_CommandBuffers.data());
            if (result != VK_SUCCESS)
            {
                SEDX_CORE_ERROR_TAG("COMMAND_BUFFER", "Failed to allocate command buffers: {}", static_cast<int>(result));
                return false;
            }

            // Set debug names for command buffers
            for (uint32_t i = 0; i < m_BufferCount; ++i)
            {
                std::string name = std::format("{}_Frame{}", m_DebugName, i);
                SetDebugName(reinterpret_cast<uint64_t>(m_CommandBuffers[i]), 
                           VK_OBJECT_TYPE_COMMAND_BUFFER, name);
            }

            // Create completion fences
            m_CompletionFences.resize(m_BufferCount);
            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled

            for (uint32_t i = 0; i < m_BufferCount; ++i)
            {
                result = vkCreateFence(device, &fenceInfo, nullptr, &m_CompletionFences[i]);
                if (result != VK_SUCCESS)
                {
                    SEDX_CORE_ERROR_TAG("COMMAND_BUFFER", "Failed to create fence {}: {}", i, static_cast<int>(result));
                    return false;
                }

                std::string name = std::format("{}_Fence{}", m_DebugName, i);
                SetDebugName(reinterpret_cast<uint64_t>(m_CompletionFences[i]), 
                           VK_OBJECT_TYPE_FENCE, name);
            }

            // Create query pools
            if (!CreateQueryPools(m_BufferCount))
            {
                SEDX_CORE_ERROR_TAG("COMMAND_BUFFER", "Failed to create query pools");
                return false;
            }

            // Initialize query result storage
            m_TimestampQueries.resize(m_BufferCount);
            m_TimestampResults.resize(m_BufferCount);
            m_PipelineStatsResults.resize(m_BufferCount);
            m_CurrentTimestampIndex.resize(m_BufferCount, 0);
            m_ActiveTimestampQueries.resize(m_BufferCount);

            for (uint32_t i = 0; i < m_BufferCount; ++i)
            {
                m_TimestampResults[i].resize(MAX_TIMESTAMP_QUERIES);
            }

            SEDX_CORE_DEBUG_TAG("COMMAND_BUFFER", "Command buffer initialization complete: {}", m_DebugName);
            return true;
        }
        catch (const std::exception& e)
        {
            SEDX_CORE_ERROR_TAG("COMMAND_BUFFER", "Exception during initialization: {}", e.what());
            return false;
        }
    }

    bool CommandBuffer::CreateQueryPools(uint32_t frameCount)
    {
        SEDX_PROFILE_SCOPE("CommandBuffer::CreateQueryPools");

        const VkDevice device = m_VkDevice->GetDevice();

        // Create timestamp query pools
        m_TimestampQueryPools.resize(frameCount);
        VkQueryPoolCreateInfo queryPoolInfo{};
        queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queryPoolInfo.queryCount = MAX_TIMESTAMP_QUERIES;

        for (uint32_t i = 0; i < frameCount; ++i)
        {
            VkResult result = vkCreateQueryPool(device, &queryPoolInfo, nullptr, &m_TimestampQueryPools[i]);
            if (result != VK_SUCCESS)
            {
                SEDX_CORE_ERROR_TAG("COMMAND_BUFFER", "Failed to create timestamp query pool {}: {}", i, static_cast<int>(result));
                return false;
            }

            std::string name = std::format("{}_TimestampPool{}", m_DebugName, i);
            SetDebugName(reinterpret_cast<uint64_t>(m_TimestampQueryPools[i]), 
                       VK_OBJECT_TYPE_QUERY_POOL, name);
        }

        // Create pipeline statistics query pools
        m_PipelineStatsQueryPools.resize(frameCount);
        queryPoolInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
        queryPoolInfo.queryCount = 1; // One query per frame
        queryPoolInfo.pipelineStatistics = 
            VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
            VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
            VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
            VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
            VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
            VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
            VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;

        for (uint32_t i = 0; i < frameCount; ++i)
        {
            VkResult result = vkCreateQueryPool(device, &queryPoolInfo, nullptr, &m_PipelineStatsQueryPools[i]);
            if (result != VK_SUCCESS)
            {
                SEDX_CORE_ERROR_TAG("COMMAND_BUFFER", "Failed to create pipeline stats query pool {}: {}", i, static_cast<int>(result));
                return false;
            }

            std::string name = std::format("{}_PipelineStatsPool{}", m_DebugName, i);
            SetDebugName(reinterpret_cast<uint64_t>(m_PipelineStatsQueryPools[i]), 
                       VK_OBJECT_TYPE_QUERY_POOL, name);
        }

        return true;
    }

    bool CommandBuffer::Begin(uint32_t frameIndex, VkCommandBufferUsageFlags usage)
    {
        SEDX_PROFILE_SCOPE("CommandBuffer::Begin");

        if (m_IsRecording)
        {
            SEDX_CORE_WARN_TAG("COMMAND_BUFFER", "Command buffer {} is already recording", m_DebugName);
            return false;
        }

        if (frameIndex >= m_BufferCount)
        {
            SEDX_CORE_ERROR_TAG("COMMAND_BUFFER", "Frame index {} out of range (max: {})", frameIndex, m_BufferCount - 1);
            return false;
        }

        m_CurrentFrameIndex = frameIndex;
        m_ActiveCommandBuffer = m_CommandBuffers[frameIndex];

        // Wait for fence and reset it
        const VkDevice device = m_VkDevice->GetDevice();
        VkResult result = vkWaitForFences(device, 1, &m_CompletionFences[frameIndex], VK_TRUE, UINT64_MAX);
        if (result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("COMMAND_BUFFER", "Failed to wait for fence: {}", static_cast<int>(result));
            return false;
        }

        result = vkResetFences(device, 1, &m_CompletionFences[frameIndex]);
        if (result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("COMMAND_BUFFER", "Failed to reset fence: {}", static_cast<int>(result));
            return false;
        }

        // Begin command buffer
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = usage;
        beginInfo.pInheritanceInfo = nullptr;

        result = vkBeginCommandBuffer(m_ActiveCommandBuffer, &beginInfo);
        if (result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("COMMAND_BUFFER", "Failed to begin command buffer: {}", static_cast<int>(result));
            return false;
        }

        // Reset query pools
        vkCmdResetQueryPool(m_ActiveCommandBuffer, m_TimestampQueryPools[frameIndex], 0, MAX_TIMESTAMP_QUERIES);
        vkCmdResetQueryPool(m_ActiveCommandBuffer, m_PipelineStatsQueryPools[frameIndex], 0, 1);

        // Reset frame state
        m_CurrentTimestampIndex[frameIndex] = 0;
        m_ActiveTimestampQueries[frameIndex].clear();
        m_TimestampQueries[frameIndex].clear();

        m_IsRecording = true;
        m_IsSubmitted = false;

        SEDX_CORE_DEBUG_TAG("COMMAND_BUFFER", "Command buffer recording started: {} (frame {})", m_DebugName, frameIndex);
        return true;
    }

    bool CommandBuffer::End()
    {
        SEDX_PROFILE_SCOPE("CommandBuffer::End");

        if (!m_IsRecording)
        {
            SEDX_CORE_WARN_TAG("COMMAND_BUFFER", "Command buffer {} is not recording", m_DebugName);
            return false;
        }

        VkResult result = vkEndCommandBuffer(m_ActiveCommandBuffer);
        if (result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("COMMAND_BUFFER", "Failed to end command buffer: {}", static_cast<int>(result));
            return false;
        }

        m_IsRecording = false;

        SEDX_CORE_DEBUG_TAG("COMMAND_BUFFER", "Command buffer recording ended: {} (frame {})", m_DebugName, m_CurrentFrameIndex);
        return true;
    }

    bool CommandBuffer::Submit(const std::vector<VkSemaphore>& waitSemaphores,
                              const std::vector<VkPipelineStageFlags>& waitStages,
                              const std::vector<VkSemaphore>& signalSemaphores)
    {
        SEDX_PROFILE_SCOPE("CommandBuffer::Submit");

        if (m_IsRecording)
        {
            SEDX_CORE_ERROR_TAG("COMMAND_BUFFER", "Cannot submit command buffer while recording: {}", m_DebugName);
            return false;
        }

        if (m_IsSubmitted)
        {
            SEDX_CORE_WARN_TAG("COMMAND_BUFFER", "Command buffer {} already submitted", m_DebugName);
            return false;
        }

        // Get appropriate queue
        VkQueue queue = VK_NULL_HANDLE;
        switch (m_QueueType)
        {
            case Queue::Graphics:
                queue = m_VkDevice->GetGraphicsQueue();
                break;
            case Queue::Compute:
                queue = m_VkDevice->GetComputeQueue();
                break;
            case Queue::Transfer:
                queue = m_VkDevice->GetGraphicsQueue(); // Fallback to graphics
                break;
            default:
                SEDX_CORE_ERROR_TAG("COMMAND_BUFFER", "Invalid queue type: {}", static_cast<int>(m_QueueType));
                return false;
        }

        // Prepare submit info
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
        submitInfo.pWaitSemaphores = waitSemaphores.empty() ? nullptr : waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStages.empty() ? nullptr : waitStages.data();
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_ActiveCommandBuffer;
        submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
        submitInfo.pSignalSemaphores = signalSemaphores.empty() ? nullptr : signalSemaphores.data();

        // Submit to queue
        VkResult result = vkQueueSubmit(queue, 1, &submitInfo, m_CompletionFences[m_CurrentFrameIndex]);
        if (result != VK_SUCCESS)
        {
            SEDX_CORE_ERROR_TAG("COMMAND_BUFFER", "Failed to submit command buffer: {}", static_cast<int>(result));
            return false;
        }

        m_IsSubmitted = true;

        SEDX_CORE_DEBUG_TAG("COMMAND_BUFFER", "Command buffer submitted: {} (frame {})", m_DebugName, m_CurrentFrameIndex);
        return true;
    }

    bool CommandBuffer::WaitForCompletion(uint64_t timeoutNs)
    {
        SEDX_PROFILE_SCOPE("CommandBuffer::WaitForCompletion");

        if (!m_IsSubmitted)
        {
            return true; // Nothing to wait for
        }

        const VkDevice device = m_VkDevice->GetDevice();
        VkResult result = vkWaitForFences(device, 1, &m_CompletionFences[m_CurrentFrameIndex], VK_TRUE, timeoutNs);
        
        if (result == VK_SUCCESS)
        {
            m_IsSubmitted = false;
            
            // Update query results now that GPU work is complete
            UpdateQueryResults(m_CurrentFrameIndex);
            
            SEDX_CORE_DEBUG_TAG("COMMAND_BUFFER", "Command buffer completion confirmed: {} (frame {})", 
                               m_DebugName, m_CurrentFrameIndex);
            return true;
        }
        else if (result == VK_TIMEOUT)
        {
            SEDX_CORE_WARN_TAG("COMMAND_BUFFER", "Command buffer wait timeout: {} (frame {})", 
                              m_DebugName, m_CurrentFrameIndex);
            return false;
        }
        else
        {
            SEDX_CORE_ERROR_TAG("COMMAND_BUFFER", "Command buffer wait failed: {} (error: {})", 
                               m_DebugName, static_cast<int>(result));
            return false;
        }
    }

    uint32_t CommandBuffer::BeginTimestampQuery(const std::string& name)
    {
        if (!m_IsRecording)
        {
            SEDX_CORE_WARN_TAG("COMMAND_BUFFER", "Cannot begin timestamp query while not recording: {}", name);
            return UINT32_MAX;
        }

        uint32_t& currentIndex = m_CurrentTimestampIndex[m_CurrentFrameIndex];
        if (currentIndex >= MAX_TIMESTAMP_QUERIES - 1)
        {
            SEDX_CORE_WARN_TAG("COMMAND_BUFFER", "Timestamp query limit reached for frame {}", m_CurrentFrameIndex);
            return UINT32_MAX;
        }

        uint32_t queryIndex = currentIndex;
        currentIndex += 2; // Reserve space for start and end

        // Record start timestamp
        vkCmdWriteTimestamp(m_ActiveCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
                           m_TimestampQueryPools[m_CurrentFrameIndex], queryIndex);

        // Store query info
        m_ActiveTimestampQueries[m_CurrentFrameIndex][name] = queryIndex;

        SEDX_CORE_DEBUG_TAG("COMMAND_BUFFER", "Timestamp query started: {} (index: {})", name, queryIndex);
        return queryIndex;
    }

    void CommandBuffer::EndTimestampQuery(uint32_t queryIndex)
    {
        if (!m_IsRecording || queryIndex == UINT32_MAX)
        {
            return;
        }

        if (queryIndex + 1 >= MAX_TIMESTAMP_QUERIES)
        {
            SEDX_CORE_WARN_TAG("COMMAND_BUFFER", "Invalid timestamp query index: {}", queryIndex);
            return;
        }

        // Record end timestamp
        vkCmdWriteTimestamp(m_ActiveCommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 
                           m_TimestampQueryPools[m_CurrentFrameIndex], queryIndex + 1);

        SEDX_CORE_DEBUG_TAG("COMMAND_BUFFER", "Timestamp query ended: index {}", queryIndex);
    }

    void CommandBuffer::BeginPipelineStatistics()
    {
        if (!m_IsRecording)
        {
            SEDX_CORE_WARN_TAG("COMMAND_BUFFER", "Cannot begin pipeline statistics while not recording");
            return;
        }

        vkCmdBeginQuery(m_ActiveCommandBuffer, m_PipelineStatsQueryPools[m_CurrentFrameIndex], 0, 0);
    }

    void CommandBuffer::EndPipelineStatistics()
    {
        if (!m_IsRecording)
        {
            return;
        }

        vkCmdEndQuery(m_ActiveCommandBuffer, m_PipelineStatsQueryPools[m_CurrentFrameIndex], 0);
    }

    void CommandBuffer::UpdateQueryResults(uint32_t frameIndex)
    {
        SEDX_PROFILE_SCOPE("CommandBuffer::UpdateQueryResults");

        const VkDevice device = m_VkDevice->GetDevice();

        // Update timestamp results
        if (!m_ActiveTimestampQueries[frameIndex].empty())
        {
            uint32_t queryCount = m_CurrentTimestampIndex[frameIndex];
            if (queryCount > 0)
            {
                VkResult result = vkGetQueryPoolResults(
                    device,
                    m_TimestampQueryPools[frameIndex],
                    0,
                    queryCount,
                    queryCount * sizeof(uint64_t),
                    m_TimestampResults[frameIndex].data(),
                    sizeof(uint64_t),
                    VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT
                );

                if (result == VK_SUCCESS)
                {
                    // Convert timestamps to timing information
                    float timestampPeriod = m_VkDevice->GetPhysicalDevice()->GetDeviceProperties().properties.limits.timestampPeriod;
                    
                    m_TimestampQueries[frameIndex].clear();
                    for (const auto& [name, startIndex] : m_ActiveTimestampQueries[frameIndex])
                    {
                        if (startIndex + 1 < queryCount)
                        {
                            uint64_t startTime = m_TimestampResults[frameIndex][startIndex];
                            uint64_t endTime = m_TimestampResults[frameIndex][startIndex + 1];
                            
                            if (endTime > startTime)
                            {
                                float timeMs = (endTime - startTime) * timestampPeriod * 1e-6f;
                                
                                TimestampQuery query;
                                query.name = name;
                                query.startIndex = startIndex;
                                query.endIndex = startIndex + 1;
                                query.timeMs = timeMs;
                                
                                m_TimestampQueries[frameIndex].push_back(query);
                            }
                        }
                    }
                }
            }
        }

        // Update pipeline statistics results
        uint64_t statsData[PIPELINE_STATS_COUNT];
        VkResult result = vkGetQueryPoolResults(
            device,
            m_PipelineStatsQueryPools[frameIndex],
            0,
            1,
            sizeof(statsData),
            statsData,
            sizeof(uint64_t),
            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT
        );

        if (result == VK_SUCCESS)
        {
            PipelineStatistics& stats = m_PipelineStatsResults[frameIndex];
            stats.inputAssemblyVertices = statsData[0];
            stats.inputAssemblyPrimitives = statsData[1];
            stats.vertexShaderInvocations = statsData[2];
            stats.clippingInvocations = statsData[3];
            stats.clippingPrimitives = statsData[4];
            stats.fragmentShaderInvocations = statsData[5];
            stats.computeShaderInvocations = statsData[6];
        }
    }

    VkCommandBuffer CommandBuffer::GetCommandBuffer(uint32_t frameIndex) const
    {
        if (frameIndex >= m_BufferCount)
        {
            SEDX_CORE_ERROR_TAG("COMMAND_BUFFER", "Frame index {} out of range", frameIndex);
            return VK_NULL_HANDLE;
        }

        return m_CommandBuffers[frameIndex];
    }

    const std::vector<TimestampQuery>& CommandBuffer::GetTimestampResults(uint32_t frameIndex) const
    {
        static const std::vector<TimestampQuery> empty;
        
        if (frameIndex >= m_BufferCount)
        {
            return empty;
        }

        return m_TimestampQueries[frameIndex];
    }

    const PipelineStatistics& CommandBuffer::GetPipelineStatistics(uint32_t frameIndex) const
    {
        static const PipelineStatistics empty{};
        
        if (frameIndex >= m_BufferCount)
        {
            return empty;
        }

        return m_PipelineStatsResults[frameIndex];
    }

    Ref<CommandBuffer> CommandBuffer::CreateOneShot(Queue queueType, const std::string& debugName)
    {
        CommandBufferInfo info;
        info.debugName = debugName;
        info.queueType = queueType;
        info.count = 1;
        info.autoBegin = true;

        auto cmdBuffer = CreateRef<CommandBuffer>(info);
        
        // Begin recording immediately for one-shot use
        if (!cmdBuffer->Begin(0, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT))
        {
            SEDX_CORE_ERROR_TAG("COMMAND_BUFFER", "Failed to begin one-shot command buffer");
            return nullptr;
        }

        return cmdBuffer;
    }

    void CommandBuffer::SetDebugName(uint64_t object, VkObjectType objectType, const std::string& name) const
    {
        if (m_VkDevice->vkSetDebugUtilsObjectNameEXT)
        {
            VkDebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            nameInfo.objectType = objectType;
            nameInfo.objectHandle = object;
            nameInfo.pObjectName = name.c_str();
            
            m_VkDevice->vkSetDebugUtilsObjectNameEXT(m_VkDevice->GetDevice(), &nameInfo);
        }
    }

    /// -------------------------------------------------------
    /// CommandBufferManager Implementation
    /// -------------------------------------------------------

    bool CommandBufferManager::Initialize()
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        
        if (s_IsInitialized)
        {
            return true;
        }

        SEDX_CORE_INFO_TAG("COMMAND_BUFFER", "Initializing CommandBufferManager");

        s_GlobalFrameIndex.store(0);
        s_IsInitialized = true;

        SEDX_CORE_INFO_TAG("COMMAND_BUFFER", "CommandBufferManager initialized successfully");
        return true;
    }

    void CommandBufferManager::Shutdown()
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        
        if (!s_IsInitialized)
        {
            return;
        }

        SEDX_CORE_INFO_TAG("COMMAND_BUFFER", "Shutting down CommandBufferManager");

        // Clear all thread-local command pools
        s_ThreadCommandPools.clear();
        s_IsInitialized = false;

        SEDX_CORE_INFO_TAG("COMMAND_BUFFER", "CommandBufferManager shutdown complete");
    }

    Ref<CommandPool> CommandBufferManager::GetThreadLocalCommandPool(Queue queueType)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        
        if (!s_IsInitialized)
        {
            SEDX_CORE_ERROR_TAG("COMMAND_BUFFER", "CommandBufferManager not initialized");
            return nullptr;
        }

        std::thread::id threadId = std::this_thread::get_id();
        
        auto threadIt = s_ThreadCommandPools.find(threadId);
        if (threadIt == s_ThreadCommandPools.end())
        {
            // Create new thread entry
            s_ThreadCommandPools[threadId] = std::map<Queue, Ref<CommandPool>>();
            threadIt = s_ThreadCommandPools.find(threadId);
        }

        auto queueIt = threadIt->second.find(queueType);
        if (queueIt == threadIt->second.end())
        {
            // Create new command pool for this thread and queue type
            auto device = RenderContext::Get()->GetLogicDevice();
            auto commandPool = CreateRef<CommandPool>(device, queueType);
            
            threadIt->second[queueType] = commandPool;
            
            SEDX_CORE_DEBUG_TAG("COMMAND_BUFFER", "Created command pool for thread {} queue {}", 
                               std::hash<std::thread::id>{}(threadId), static_cast<int>(queueType));
            
            return commandPool;
        }

        return queueIt->second;
    }

    Ref<CommandBuffer> CommandBufferManager::CreateCommandBuffer(const CommandBufferInfo& info)
    {
        if (!s_IsInitialized)
        {
            SEDX_CORE_ERROR_TAG("COMMAND_BUFFER", "CommandBufferManager not initialized");
            return nullptr;
        }

        return CreateRef<CommandBuffer>(info);
    }

    uint32_t CommandBufferManager::GetGlobalFrameIndex()
    {
        return s_GlobalFrameIndex.load();
    }

}

/// -------------------------------------------------------

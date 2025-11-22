/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* vk_cmd_buffers.h
* -------------------------------------------------------
* Created: 7/4/2025
* -------------------------------------------------------
*/
#pragma once
#include "vk_data.h"
#include "vk_swapchain.h"
#include "SceneryEditorX/renderer/command_manager.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
    /// Forward declarations
    class CommandPool;

    /// -------------------------------------------------------

    /**
     * @struct CommandBufferInfo
     * @brief Configuration structure for command buffer creation
     */
    struct CommandBufferInfo
    {
        std::string debugName = "CommandBuffer";
        Queue queueType = Queue::Graphics;
        uint32_t count = 0; // 0 = one per frame in flight
        bool isPrimary = true;
        bool autoBegin = false;
    };

    /// -------------------------------------------------------

    /**
     * @struct PipelineStatistics
     * @brief Pipeline statistics query results for performance monitoring
     */
    struct PipelineStatistics
    {
        uint64_t inputAssemblyVertices = 0;
        uint64_t inputAssemblyPrimitives = 0;
        uint64_t vertexShaderInvocations = 0;
        uint64_t clippingInvocations = 0;
        uint64_t clippingPrimitives = 0;
        uint64_t fragmentShaderInvocations = 0;
        uint64_t computeShaderInvocations = 0;
    };

    /// -------------------------------------------------------

    /**
     * @struct TimestampQuery
     * @brief Timestamp query information for GPU timing
     */
    struct TimestampQuery
    {
        std::string name;
        uint32_t startIndex;
        uint32_t endIndex;
        float timeMs = 0.0f;
    };

    /// -------------------------------------------------------

    /**
     * @class CommandBuffer
     * @brief Manages Vulkan command buffer lifecycle and synchronization
     *
     * @details This class provides a high-level interface for managing Vulkan command buffers
     * with proper frame-in-flight synchronization, query pools for performance monitoring,
     * and integration with the CommandPool system. It handles:
     *
     * 1. Command buffer allocation and management per frame
     * 2. Timestamp and pipeline statistics queries
     * 3. Proper synchronization with fences and semaphores
     * 4. Debug naming and validation
     * 5. Automatic resource cleanup
     *
     * Usage Pattern:
     * ```cpp
     * auto cmdBuffer = CreateRef<CommandBuffer>(CommandBufferInfo{
     *     .debugName = "MainRender",
     *     .queueType = Queue::Graphics,
     *     .count = 0 // One per frame in flight
     * });
     *
     * cmdBuffer->Begin(frameIndex);
     * // Record commands...
     * cmdBuffer->End();
     * cmdBuffer->Submit(semaphoreWait, semaphoreSignal);
     * ```
     */
    class CommandBuffer : public RefCounted
    {
    public:
        /**
         * @brief Create a command buffer with specified configuration
         * @param info Configuration for the command buffer creation
         */
        explicit CommandBuffer(const CommandBufferInfo& info);

        /**
         * @brief Destructor - cleans up all Vulkan resources
         */
        virtual ~CommandBuffer() override;

        /// -------------------------------------------------------
        /// Lifecycle Management
        /// -------------------------------------------------------

        /**
         * @brief Begin recording commands for the specified frame
         * @param frameIndex Frame index for frame-in-flight synchronization
         * @param usage Command buffer usage flags (default: one-time submit)
         * @return True if begin was successful
         */
        bool Begin(uint32_t frameIndex, VkCommandBufferUsageFlags usage = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        /**
         * @brief End command recording for the current frame
         * @return True if end was successful
         */
        bool End();

        /**
         * @brief Submit the command buffer to the appropriate queue
         * @param waitSemaphores Semaphores to wait on before execution
         * @param waitStages Pipeline stages to wait for
         * @param signalSemaphores Semaphores to signal after execution
         * @return True if submission was successful
         */
        bool Submit(const std::vector<VkSemaphore>& waitSemaphores = {},
                   const std::vector<VkPipelineStageFlags>& waitStages = {},
                   const std::vector<VkSemaphore>& signalSemaphores = {});

        /**
         * @brief Wait for the current frame's command buffer to complete execution
         * @param timeoutNs Timeout in nanoseconds (default: infinite)
         * @return True if wait completed successfully
         */
        bool WaitForCompletion(uint64_t timeoutNs = UINT64_MAX);

        /// -------------------------------------------------------
        /// Query Management
        /// -------------------------------------------------------

        /**
         * @brief Begin a timestamp query with a given name
         * @param name Debug name for the timestamp query
         * @return Query index for ending the query
         */
        uint32_t BeginTimestampQuery(const std::string& name);

        /**
         * @brief End a timestamp query
         * @param queryIndex Query index returned from BeginTimestampQuery
         */
        void EndTimestampQuery(uint32_t queryIndex);

        /**
         * @brief Begin pipeline statistics collection
         */
        void BeginPipelineStatistics();

        /**
         * @brief End pipeline statistics collection
         */
        void EndPipelineStatistics();

        /**
         * @brief Get timestamp query results for the specified frame
         * @param frameIndex Frame index to get results for
         * @return Vector of completed timestamp queries
         */
        [[nodiscard]] const std::vector<TimestampQuery>& GetTimestampResults(uint32_t frameIndex) const;

        /**
         * @brief Get pipeline statistics for the specified frame
         * @param frameIndex Frame index to get statistics for
         * @return Pipeline statistics structure
         */
        [[nodiscard]] const PipelineStatistics& GetPipelineStatistics(uint32_t frameIndex) const;

        /// -------------------------------------------------------
        /// Accessors
        /// -------------------------------------------------------

        /**
         * @brief Get the active command buffer handle for the current frame
         * @return VkCommandBuffer handle
         */
        [[nodiscard]] VkCommandBuffer GetActiveCommandBuffer() const { return m_ActiveCommandBuffer; }

        /**
         * @brief Get command buffer handle for a specific frame
         * @param frameIndex Frame index
         * @return VkCommandBuffer handle for the specified frame
         */
        [[nodiscard]] VkCommandBuffer GetCommandBuffer(uint32_t frameIndex) const;

        /**
         * @brief Get the queue type this command buffer is associated with
         * @return Queue type
         */
        [[nodiscard]] Queue GetQueueType() const { return m_QueueType; }

        /**
         * @brief Get the debug name of this command buffer
         * @return Debug name string
         */
        [[nodiscard]] const std::string& GetDebugName() const { return m_DebugName; }

        /**
         * @brief Check if this command buffer is currently recording
         * @return True if recording is active
         */
        [[nodiscard]] bool IsRecording() const { return m_IsRecording; }

        /**
         * @brief Check if this command buffer has been submitted
         * @return True if submitted and waiting for completion
         */
        [[nodiscard]] bool IsSubmitted() const { return m_IsSubmitted; }

        /// -------------------------------------------------------
        /// Static Utilities
        /// -------------------------------------------------------

        /**
         * @brief Create a one-shot command buffer for immediate operations
         * @param queueType Queue type for the command buffer
         * @param debugName Debug name for the command buffer
         * @return Configured command buffer ready for immediate use
         */
        static Ref<CommandBuffer> CreateOneShot(Queue queueType = Queue::Graphics,
                                               const std::string& debugName = "OneShot");

    private:
        /// -------------------------------------------------------
        /// Internal State
        /// -------------------------------------------------------

        std::string m_DebugName;
        Queue m_QueueType;
        uint32_t m_BufferCount;
        uint32_t m_CurrentFrameIndex = 0;

        bool m_IsRecording = false;
        bool m_IsSubmitted = false;
        bool m_IsPrimary = true;

        /// -------------------------------------------------------
        /// Vulkan Resources
        /// -------------------------------------------------------

        Ref<VulkanDevice> m_VkDevice;
        Ref<CommandPool> m_CommandPool;

        VkCommandBuffer m_ActiveCommandBuffer = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> m_CommandBuffers;
        std::vector<VkFence> m_CompletionFences;

        /// -------------------------------------------------------
        /// Query Pools and Results
        /// -------------------------------------------------------

        static constexpr uint32_t MAX_TIMESTAMP_QUERIES = 64;
        static constexpr uint32_t PIPELINE_STATS_COUNT = 7;

        std::vector<VkQueryPool> m_TimestampQueryPools;
        std::vector<VkQueryPool> m_PipelineStatsQueryPools;

        std::vector<std::vector<TimestampQuery>> m_TimestampQueries;
        std::vector<std::vector<uint64_t>> m_TimestampResults;
        std::vector<PipelineStatistics> m_PipelineStatsResults;

        std::vector<uint32_t> m_CurrentTimestampIndex;
        std::vector<std::map<std::string, uint32_t>> m_ActiveTimestampQueries;

        /// -------------------------------------------------------
        /// Internal Methods
        /// -------------------------------------------------------

        /**
         * @brief Initialize command buffer resources
         * @return True if initialization was successful
         */
        bool Initialize();

        /**
         * @brief Create query pools for the specified frame count
         * @param frameCount Number of frames to create pools for
         * @return True if creation was successful
         */
        bool CreateQueryPools(uint32_t frameCount);

        /**
         * @brief Update query results for the specified frame
         * @param frameIndex Frame to update results for
         */
        void UpdateQueryResults(uint32_t frameIndex);

        /**
         * @brief Set debug name for Vulkan objects
         * @param object Vulkan object handle
         * @param objectType VkObjectType for the object
         * @param name Debug name to set
         */
        void SetDebugName(uint64_t object, VkObjectType objectType, const std::string& name) const;

        friend class Renderer;
        friend class RenderContext;
    };

    /// -------------------------------------------------------
    /// Global Command Buffer Management
    /// -------------------------------------------------------

    /**
     * @class CommandBufferManager
     * @brief Global manager for command buffer pools and allocation
     */
    class CommandBufferManager
    {
    public:
        /**
         * @brief Initialize the command buffer manager
         * @return True if initialization was successful
         */
        static bool Initialize();

        /**
         * @brief Shutdown and cleanup the command buffer manager
         */
        static void Shutdown();

        /**
         * @brief Get or create a command pool for the current thread
         * @param queueType Queue type for the command pool
         * @return Shared reference to the command pool
         */
        static Ref<CommandPool> GetThreadLocalCommandPool(Queue queueType = Queue::Graphics);

        /**
         * @brief Create a new command buffer with the specified configuration
         * @param info Command buffer configuration
         * @return New command buffer instance
         */
        static Ref<CommandBuffer> CreateCommandBuffer(const CommandBufferInfo& info);

        /**
         * @brief Get the global frame index for synchronization
         * @return Current global frame index
         */
        static uint32_t GetGlobalFrameIndex();

    private:
        static std::mutex s_Mutex;
        static std::map<std::thread::id, std::map<Queue, Ref<CommandPool>>> s_ThreadCommandPools;
        static std::atomic<uint32_t> s_GlobalFrameIndex;
        static bool s_IsInitialized;
    };

}

/// -------------------------------------------------------

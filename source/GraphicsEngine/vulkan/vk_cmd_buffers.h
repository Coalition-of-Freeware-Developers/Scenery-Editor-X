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
#include <GraphicsEngine/vulkan/render_data.h>
#include <SceneryEditorX/core/pointers.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    /// Forward declarations
	class CommandPool;
	class GraphicsEngine;

    /// -------------------------------------------------------

    inline InternalQueue queues[Present];
    inline Queue currentQueue = Present;

    enum GPUQueueType : uint8_t
    {
        GPU_QUEUE_GRAPHICS = 0,
        GPU_QUEUE_COMPUTE = 1,
        GPU_QUEUE_TRANSFER = 2,
        GPU_QUEUES_COUNT = 3,
        GPU_QUEUE_UNKNOWN = static_cast<uint8_t>(-1)
    };

    /// -------------------------------------------------------

	class CommandBuffer : public RefCounted
	{
	public:
        CommandBuffer(uint32_t count = 0, std::string debugName = "");
        CommandBuffer(bool swapchain);
        virtual ~CommandBuffer() override;

		GLOBAL Ref<CommandBuffer> Get(); ///< Static accessor method to get the singleton instance

        virtual void Begin(Queue queue);
        virtual void End(VkSubmitInfo submitInfo);
        virtual void Submit();

        [[nodiscard]] CommandResources &GetCurrentCommandResources() const;
        [[nodiscard]] VkCommandBuffer GetActiveCmdBuffer() const { return activeCmdBuffer; }
        [[nodiscard]] VkCommandBuffer GetCommandBuffer(const RenderData &frameIndex) const;

	private:
        INTERNAL Ref<CommandBuffer> cmdBuffers;

        VkCommandBuffer activeCmdBuffer = nullptr;
        VkCommandPool cmdPool = nullptr;

	    std::vector<VkFence> waitFences;
	    std::vector<VkCommandBuffer> commandBuffer;

        std::vector<VkQueryPool> TimestampQueryPools;
        std::vector<VkQueryPool> PipelineQueryPools;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;

        std::map<std::string, float> timeStampTable;

		RenderData data; 
		uint32_t availQuery = 2;
        uint32_t queryCount = 0;
		uint32_t pipelineQueryCount = 0;
        uint32_t timeStampPerPool = 64;
        std::string debugName;

        friend class VulkanDevice;
        friend class GraphicsEngine;
	};

	/// ---------------------------------------------------------

    /*
    class CommandPool
    {
    public:

        /**
         * @brief Allocate a command buffer from the pool
         *
         * @param begin Whether to begin the command buffer
         * @param compute Whether to allocate from the compute pool
         *
         * @return A new command buffer
         #1#
        [[nodiscard]] VkCommandBuffer AllocateCommandBuffer(bool begin, bool compute = false) const;

        /**
         * @brief Submit a command buffer to the graphics queue and wait for completion
         * @param cmdBuffer The command buffer to submit
         #1#
        void FlushCmdBuffer(VkCommandBuffer cmdBuffer) const;

        /**
         * @brief Submit a command buffer to a specific queue and wait for completion
         * @param cmdBuffer The command buffer to submit
         * @param queue The queue to submit to
         #1#
        void FlushCmdBuffer(VkCommandBuffer cmdBuffer, VkQueue queue) const;

        /// Accessor methods
        [[nodiscard]] VkCommandPool GetGraphicsCmdPool() const { return GraphicsCmdPool; }
        [[nodiscard]] VkCommandPool GetComputeCmdPool() const { return ComputeCmdPool; }

    private:
        VkCommandPool GraphicsCmdPool = VK_NULL_HANDLE;
        VkCommandPool ComputeCmdPool = VK_NULL_HANDLE;
    };
    */

} // namespace SceneryEditorX

/// -------------------------------------------------------

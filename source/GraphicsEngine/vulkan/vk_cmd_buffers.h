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

// -------------------------------------------------------

namespace SceneryEditorX
{
    /// Forward declarations
	class CommandPool;
	class GraphicsEngine;

    /// -------------------------------------------------------

    inline InternalQueue queues[Present];
    inline Queue currentQueue = Present;

    /// -------------------------------------------------------

	class CommandBuffer
	{
	public:

        CommandBuffer(uint32_t count = 0);
        CommandBuffer(bool swapchain);
        ~CommandBuffer();

        virtual void Begin(Queue queue);
        virtual void End(VkSubmitInfo submitInfo);
        virtual void Submit();

        [[nodiscard]] CommandResources &GetCurrentCommandResources() const;
        [[nodiscard]] VkCommandBuffer GetActiveCommandBuffer() const { return activeCmdBuffer; }
        [[nodiscard]] VkCommandBuffer GetCommandBuffer(const RenderData &frameIndex) const;
		[[nodiscard]] Ref<CommandPool> GetCommandPool() const { return cmdPool; }

	private:
        Ref<GraphicsEngine> gfxEngine;
        Ref<CommandPool> cmdPool = nullptr;
        VkCommandBuffer activeCmdBuffer = nullptr;

        std::vector<VkFence> waitFences;
        std::vector<VkCommandBuffer> cmdBuffers;
        std::map<std::string, float> timeStampTable;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;

		std::vector<VkQueryPool> TimestampQueryPools;
		std::vector<VkQueryPool> PipelineQueryPools;

		uint32_t availQuery = 2;
        uint32_t queryCount = 0;
		uint32_t pipelineQueryCount = 0;
        uint32_t timeStampPerPool = 64;

		friend class VulkanDevice;
        friend class GraphicsEngine;
	};

	/// ---------------------------------------------------------

    class CommandPool
    {
    public:
        /**
         * @brief Create command pools for a device
         * @param vulkanDevice The device to create command pools for
         */
        explicit CommandPool(const Ref<VulkanDevice> &vulkanDevice);
        virtual ~CommandPool();

        /**
         * @brief Allocate a command buffer from the pool
         *
         * @param begin Whether to begin the command buffer
         * @param compute Whether to allocate from the compute pool
         *
         * @return A new command buffer
         */
        [[nodiscard]] VkCommandBuffer AllocateCommandBuffer(bool begin, bool compute = false) const;

        /**
         * @brief Submit a command buffer to the graphics queue and wait for completion
         * @param cmdBuffer The command buffer to submit
         */
        void FlushCmdBuffer(VkCommandBuffer cmdBuffer) const;

        /**
         * @brief Submit a command buffer to a specific queue and wait for completion
         * @param cmdBuffer The command buffer to submit
         * @param queue The queue to submit to
         */
        void FlushCmdBuffer(VkCommandBuffer cmdBuffer, VkQueue queue) const;

        /// Accessor methods
        [[nodiscard]] VkCommandPool GetGraphicsCmdPool() const { return GraphicsCmdPool; }
        [[nodiscard]] VkCommandPool GetComputeCmdPool() const { return ComputeCmdPool; }

    private:
        Ref<VulkanDevice> device;
        VkCommandPool GraphicsCmdPool = VK_NULL_HANDLE;
        VkCommandPool ComputeCmdPool = VK_NULL_HANDLE;
    };

} // namespace SceneryEditorX

// -------------------------------------------------------

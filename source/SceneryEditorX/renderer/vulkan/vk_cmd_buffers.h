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
#include <SceneryEditorX/renderer/vulkan/vk_data.h>
#include <SceneryEditorX/renderer/vulkan/vk_swapchain.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    /// Forward declarations
	class CommandPool;

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

    struct PipelineStats
    {
        uint64_t InputAssemblyVertices = 0;
        uint64_t InputAssemblyPrimitives = 0;
        uint64_t VertexShaderInvocations = 0;
        uint64_t ClippingInvocations = 0;
        uint64_t ClippingPrimitives = 0;
        uint64_t FragmentShaderInvocations = 0;
        uint64_t ComputeShaderInvocations = 0;
    };

    /// -------------------------------------------------------

	class CommandBuffer : public RefCounted
	{
	public:
        CommandBuffer(uint32_t count = 0, std::string debugName = "");
        CommandBuffer(std::string debugName, bool swapchain);
        virtual ~CommandBuffer() override;

		GLOBAL Ref<CommandBuffer> Get(); ///< Static accessor method to get the singleton instance

        void Begin();
        //void Begin(Queue queue);
        void End();
        //void End(VkSubmitInfo submitInfo);
        void Submit();
		
		const PipelineStats& GetPipelineStatistics(uint32_t frameIndex) const { return pipelineStatsQueryResults[frameIndex]; }

		uint32_t BeginTimestampQuery();
		void EndTimestampQuery(uint32_t queryID);

        //[[nodiscard]] static CommandResources &GetCurrentCommandResources();
        [[nodiscard]] VkCommandBuffer GetActiveCmdBuffer() const { return activeCmdBuffer; }
        [[nodiscard]] VkCommandBuffer GetCommandBuffer(const RenderData &frameIndex) const;

		//GLOBAL Ref<CommandBuffer> CreateFromSwapChain(const std::string &debugName = "");

	private:
        Ref<VulkanDevice> vkDevice;
        SwapChain swapChain;
        VkCommandBuffer activeCmdBuffer = nullptr;
        VkCommandPool cmdPool = nullptr;

        std::vector<VkFence> waitFences;
        std::vector<VkCommandBuffer> cmdBuffers;
        std::vector<PipelineStats> pipelineStatsQueryResults;

        std::vector<VkQueryPool> timestampQueryPools;
        std::vector<VkQueryPool> pipelineQueryPools;
        std::vector<std::vector<uint64_t>> timestampQueryResults;
        std::vector<std::vector<float>> executionGPUTimes;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;

        std::map<std::string, float> timeStampTable;

        RenderData data;
        bool ownedBySwapChain = false;
        uint32_t availTimeQuery = 2;
        uint32_t timeQueryCount = 0;
        uint32_t pipelineQueryCount = 0;
        uint32_t timeStampPerPool = 64;
        std::string debugName;

        friend class RenderContext;
	};

}

/// -------------------------------------------------------

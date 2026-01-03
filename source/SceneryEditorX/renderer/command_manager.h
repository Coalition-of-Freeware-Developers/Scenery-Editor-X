/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* command_manager.h
* -------------------------------------------------------
* Created: 25/8/2025
* -------------------------------------------------------
*/
#pragma once
#include "command_pool.h"
#include "image_data.h"
#include "pipeline_state.h"
#include "texture.h"
#include "viewport.h"
#include "device.h"
#include "enums.h"
#include "pipeline.h"
#include <stack>
#include <thread>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <SceneryEditorX\core\threading\render_thread.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
    class SwapChain;

    // -------------------------------------------------------

    /**
     * @brief Thread-local command pool manager for multithreaded recording
     *
     * Provides one CommandPool per std::thread::id and queue family. Pools are
     * created on first use and destroyed at Shutdown(). Immediate-submit helper
     * remains unchanged in CommandPool.
     */
    class ThreadCommandPools final
    {
    public:
        static Ref<CommandPool> Get(const Ref<VulkanDevice> &device, Queue queueType);
        static void Shutdown();

    private:
        typedef std::unordered_map<int, Ref<CommandPool>> PoolMap;
        static std::unordered_map<std::thread::id, PoolMap>& Pools();
    };

    // -------------------------------------------------------

    /**
     * @enum CommandState
     * @brief Enumeration representing the state of a command buffer
     *
     * This enum is used to track the current state of a command buffer,
     * such as whether it is idle, being recorded, or has been submitted for execution.
     */
	enum class CommandState : uint8_t
	{
	    IDLE,
	    RECORDING,
	    SUBMITTED
	};

    // -------------------------------------------------------

    enum class BarrierType : uint8_t
    {
        EnsureWriteThenRead, // RAW: Make prior write visible before read (e.g., post-dispatch)
        EnsureReadThenWrite, // WAR: Order read before write (execution dep; e.g., pre-dispatch)
        EnsureWriteThenWrite // WAW: Order prior write before new write (e.g., sequential computes on same UAV)
    };

    // -------------------------------------------------------

    struct ImageBarrierInfo
    {
        void *image						= nullptr;
        bool isDepth					= false;
        uint32_t aspect					= 0;
        uint32_t mip					= 0;
        uint32_t mipRange				= 0;
        uint32_t arrayLen				= 0;
        Layout::ImageLayout layoutOld	= Layout::ImageLayout::Max;
        Layout::ImageLayout layoutNew	= Layout::ImageLayout::Max;
    };

    // -------------------------------------------------------

	class CommandManager : public RefCounted
	{
	public:
        typedef void (*RenderCommandFn)(void *);

        CommandManager(/*Queue *queue, */ const Ref<CommandPool> *cmdPool, const std::string &debugName);
        CommandManager(uint32_t count = 0, std::string debugName = "");
        CommandManager(std::string debugName, bool swapchain);
        virtual ~CommandManager() override;
        static Ref<CommandManager> Get(); // Static accessor method to get the singleton instance

        CommandManager(const CommandManager &) 				= default;
        CommandManager(CommandManager &&) 					= delete;
        CommandManager &operator=(const CommandManager &) 	= default;
        CommandManager &operator=(CommandManager &&) 		= delete;

		void Begin();
        void End();
        void Submit();
        void ExecuteCommandQueue();
        void ExecutionWait(bool waitTime = false);
        void SetPipelineState();

		VkCommandBuffer GetActiveCommandBuffer() const { return m_ActiveCommandBuffer; }
        VkCommandBuffer GetCommandBuffer(uint32_t frameIndex) const
        {
            SEDX_CORE_ASSERT(frameIndex < m_CommandBuffers.size());
            return m_CommandBuffers[frameIndex];
        }

		void CreateCommandQueue();
		void DeleteCommandQueue();
        void *Allocate(RenderCommandFn func, uint32_t size);

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Draw Commands																								  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		void Draw(uint32_t count, uint32_t vertexStartIdx = 0);
	    void DrawIndexed(uint32_t count, uint32_t indexOffset = 0, uint32_t vertexOffset = 0, uint32_t instanceIdx = 0, uint32_t instanceCount = 1);

		// Ref<VertexBuffer> SetBuffer();
		// Ref<IndexBuffer>  SetBuffer();

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	    /// Clear Commands																								  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void ClearPipelineStateRenderTargets(Pipeline& pipelineState);
        //void ClearTexture(Texture* texture,const Color& clearColor = colorLoad, const float clearDepth = depthLoad,const uint32_t clearStencil = stencilLoad);

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Dispatch																									  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void Dispatch(Texture *texture, float resolutionScale);
        void Dispatch(uint32_t x, uint32_t y, uint32_t z = 1);

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// BLIT Commands																								  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void Blit(Texture* src, SwapChain* dst);
        void Blit(Texture* src, Texture* dst, bool blitMips, float srcScaling = 1.0f);

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Copy Commands																								  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void Copy(Texture *src, SwapChain *dst);
        void Copy(Texture* src, Texture* dst, bool blitMips);

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Viewport																									  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void SetViewport(Viewport& viewport) const;

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Scissor																										  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void SetScissorRectangle(xMath::Rectangle & scissorRectangle) const;

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Cull Mode																									  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void SetCullMode(const CullMode cullMode);

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Buffers																										  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void SetIndexBuffer(const Buffer *buffer);
        void SetVertexBuffer(const Buffer *vertex, Buffer *instance = nullptr);
        void SetBuffer(uint32_t slot, Buffer* buffer) const;
        //void SetBuffer(const bindingsUav slot, Buffer* buffer) const { SetBuffer(static_cast<uint32_t>(slot), buffer); }
        void UpdateBuffer(Buffer *buffer, uint64_t offset, uint64_t size, const void *data);

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Constant Buffers																							  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void SetConstantBuffer(uint32_t slot, Buffer* constBuffer) const;
        //void SetConstantBuffer(const Renderer_BindingsCb slot, Buffer* constant_buffer) const { SetConstantBuffer(static_cast<uint32_t>(slot), constant_buffer); }

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Push-Constant Buffers																						  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void PushConstants(uint32_t offset, uint32_t size, void* data);

        template<typename T>
        void PushConstants(const T& data)
        {
            PushConstants(0, sizeof(T), &data);
        }

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Texture																										  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // void SetTexture(const uint32_t slot, Texture* texture, const uint32_t mipIdx = allMips, uint32_t mip_range = 0, const bool uav = false);
        // void SetTexture(const bindingsUav slot, Texture* texture,  const uint32_t mipIdx = allMips, uint32_t mip_range = 0) { SetTexture(static_cast<uint32_t>(slot), texture, mipIdx, mip_range, true); }
        // void SetTexture(const Renderer_BindingsSrv slot, Texture* texture,  const uint32_t mipIdx = allMips, uint32_t mip_range = 0) { SetTexture(static_cast<uint32_t>(slot), texture, mipIdx, mip_range, false); }

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Markers																										  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void BeginMarker(std::string debugName);
        void EndMarker();

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Timestamp Queries																							  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        uint32_t BeginTimestamp();
        void EndTimestamp();
        float GetTimestampResult(const uint32_t indexTimestamp);

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Occlusion Queries																							  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void BeginOcclusionQuery(const uint64_t entityId);
        void EndOcclusionQuery();
        bool GetOcclusionQueryResult(const uint64_t entityId);
        void UpdateOcclusionQueries();

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Time-Blocks (cpu and gpu time measurement as well as gpu markers)											  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void BeginTimeBlock(std::string debugName, const bool gpuMarker = true, const bool gpuTiming = true);
        void EndTimeBlock();

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Memory Barriers																								  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void InsertBarrier(void* image, const VkFormat format, const uint32_t mipIdx, const uint32_t mipRange, const uint32_t arrayLength,const Layout::ImageLayout layoutNew);
        void InsertBarrierReadWrite(Texture *texture, const BarrierType type);
        void InsertBarrierReadWrite(Buffer *buffer);
        void InsertPendingBarrierGroup();

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Misc																										  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void RenderPassEnd();
        //FrameSync* GetRenderingCompleteSemaphore()		{ return m_RenderingCompleteSemaphore.Get(); }
        void* GetResource() const						{ return m_Resource; }
	    CommandState GetState() const					{ return m_State; }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Image Layouts																								  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        static void RemoveLayout(void* image);
        static Layout::ImageLayout GetImageLayout(void* image, uint32_t mipIdx);

	private:
        void PreDraw();
        void RenderPassBegin();

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Synchronization																								  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //Ref<FrameSync> m_RenderingCompleteSemaphore;
        //Ref<FrameSync> m_RenderingCompleteSemaphoreTimeline;

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Misc																										  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        uint64_t m_BufferID_Vertex							= 0;
        uint64_t m_BufferID_Index							= 0;
        uint32_t m_Timestamp_Index                          = 0;
        Ref<Pipeline> *m_Pipeline							= nullptr;
        //Ref<PipelineState> m_PSO;
        //DescriptorSetLayout* m_descriptor_layout_current	= nullptr;
        std::atomic<CommandState> m_State					= CommandState::IDLE;
        CullMode m_CullMode									= CullMode::Back;
        bool m_RenderPass_Active							= false;
        uint32_t m_RenderPass_DrawCalls						= 0;
        VkClearValue m_ClearColor;
        std::stack<const char*> m_ActiveTimeblocks;
        std::stack<const char*> m_DebugLabelStack;
        std::mutex m_MutexReset;
        std::vector<ImageBarrierInfo> m_ImageBarriers;
        bool m_Load_Depth_RenderTarget						= false;

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        std::string m_DebugName;
        VkCommandPool m_CommandPool = nullptr;
        std::vector<VkCommandBuffer> m_CommandBuffers;
        VkCommandBuffer m_ActiveCommandBuffer = nullptr;
        std::vector<VkFence> m_WaitFences;
        uint32_t m_TimestampQueryCount = 0;
        uint32_t m_TimestampNextAvailableQuery = 2;
        std::vector<VkQueryPool> m_TimestampQueryPools;
        std::vector<VkQueryPool> m_PipelineStatisticsQueryPools;
        std::vector<std::vector<uint64_t>> m_TimestampQueryResults;
        std::vector<std::vector<float>> m_ExecutionGPUTimes;
        bool m_OwnedBySwapChain = false;
        uint32_t m_PipelineQueryCount = 0;
        std::vector<PipelineStatistics> m_PipelineStatisticsQueryResults;

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        uint8_t *m_CommandBuffer;
        uint8_t *m_CommandBufferPtr;
        uint32_t m_CommandCount = 0;

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Resources																									  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void* m_Resource						= nullptr;
        void* m_CmdPool_Resource				= nullptr;
        void* m_QueryPool_Timestamps			= nullptr;
        void* m_QueryPool_PipelineStats			= nullptr;
        void* m_QueryPool_Occlusion				= nullptr;
	};

}

// -------------------------------------------------------

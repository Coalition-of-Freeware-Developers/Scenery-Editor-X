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
#include "clear_value.h"
#include "command_pool.h"
#include "frame_sync.h"
#include "image_data.h"
#include "texture.h"
#include "viewport.h"
#include "vulkan/vk_device.h"
#include "vulkan/vk_enums.h"
#include "vulkan/vk_pipeline.h"
#include <stack>
#include <thread>
#include <unordered_map>
#include <vulkan/vulkan.h>

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
		CommandManager(Queue *queue, void *cmdPool, const std::string &debugName);
        virtual ~CommandManager() override;

		void Begin();
        void Submit(FrameSync *semaphoreWait, const bool immediate);
        void ExecutionWait(const bool waitTime = false);
        void SetPipelineState();

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Draw Commands																								  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		void Draw(const uint32_t count, const uint32_t vertexStartIdx = 0);
	    void DrawIndexed(const uint32_t count, const uint32_t indexOffset = 0, const uint32_t vertexOffset = 0, const uint32_t instanceIdx = 0, const uint32_t instanceCount = 1);

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
        void Dispatch(Texture *texture);
        void Dispatch(uint32_t x, uint32_t y, uint32_t z = 1);

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// BLIT Commands																								  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void Blit(Texture* src, SwapChain* dst);
        void Blit(Texture* src, Texture* dst, const bool blitMips, const float srcScaling = 1.0f);

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Copy Commands																								  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void Copy(Texture *src, SwapChain *dst);
        void Copy(Texture* src, Texture* dst, const bool blitMips);

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Viewport																									  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void SetViewport(const Viewport& viewport) const;

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Scissor																										  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void SetScissorRectangle(const xMath::Rectangle & scissorRectangle) const;

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Cull Mode																									  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void SetCullMode(const CullMode cullMode);

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Buffers																										  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void SetIndexBuffer(const Buffer *buffer);
        void SetVertexBuffer(const Buffer *vertex, Buffer *instance = nullptr);
        void SetBuffer(const uint32_t slot, Buffer* buffer) const;
        //void SetBuffer(const bindingsUav slot, Buffer* buffer) const { SetBuffer(static_cast<uint32_t>(slot), buffer); }
        void UpdateBuffer(Buffer *buffer, const uint64_t offset, const uint64_t size, const void *data);

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Constant Buffers																							  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void SetConstantBuffer(const uint32_t slot, Buffer* constBuffer) const;
        //void SetConstantBuffer(const Renderer_BindingsCb slot, Buffer* constant_buffer) const { SetConstantBuffer(static_cast<uint32_t>(slot), constant_buffer); }
        
	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Push-Constant Buffers																						  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void PushConstants(const uint32_t offset, const uint32_t size, const void* data);

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
        // void InsertBarrierReadWrite(Texture *texture, const BarrierType type);
        void InsertBarrierReadWrite(Buffer *buffer);
        void InsertPendingBarrierGroup();

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Misc																										  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void RenderPassEnd();
        FrameSync* GetRenderingCompleteSemaphore()		{ return m_RenderingCompleteSemaphore.Get(); }
        void* GetResource() const						{ return m_Resource; }
	    CommandState GetState() const					{ return m_State; }
        //QueueManager* GetQueue() const					{ return m_Queue; }

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
        Ref<FrameSync> m_RenderingCompleteSemaphore;
        Ref<FrameSync> m_RenderingCompleteSemaphoreTimeline;

	    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Misc																										  ///
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        uint64_t m_BufferID_Vertex							= 0;
        uint64_t m_BufferID_Index							= 0;
        uint32_t m_Timestamp_Index                          = 0;
        Ref<Pipeline> *m_Pipeline							= nullptr;
        //DescriptorSetLayout* m_descriptor_layout_current	= nullptr;
        std::atomic<CommandState> m_State					= CommandState::IDLE;
        CullMode m_CullMode									= CullMode::Back;
        bool m_RenderPass_Active							= false;
        uint32_t m_RenderPass_DrawCalls						= 0;
        ClearValue m_ClearColor;
        std::stack<const char*> m_ActiveTimeblocks;
        std::stack<const char*> m_DebugLabelStack;
        std::mutex m_MutexReset;
        std::vector<ImageBarrierInfo> m_ImageBarriers;
        //QueueManager *m_Queue								= nullptr;
        bool m_Load_Depth_RenderTarget						= false;

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

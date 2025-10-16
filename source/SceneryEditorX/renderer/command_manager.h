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
#include "frame_sync.h"
#include "image_data.h"
#include "texture.h"
#include "viewport.h"
#include "SceneryEditorX/utils/pointers.h"
#include "vulkan/vk_device.h"
#include "vulkan/vk_enums.h"
#include "vulkan/vk_pipeline.h"
#include <stack>
#include <thread>
#include <unordered_map>
#include <vulkan/vulkan.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

    class SwapChain;
    class CommandPool : public RefCounted
	{
	public:
	    /**
	     * @brief Create command pools for a device
	     * @param vulkanDevice The device to create command pools for
	     * @param type The type of queue this command pool will be used with (graphics, compute, transfer, etc.)
	     */
	    CommandPool(const Ref<VulkanDevice> &vulkanDevice, Queue type);
	    virtual ~CommandPool() override;

	    /**
	     * @brief Allocate a command buffer from the pool
	     *
	     * @param begin Whether to begin the command buffer
	     * @param compute Whether to allocate from the compute pool
	     *
	     * @return A new command buffer
	     */
	    [[nodiscard]] VkCommandBuffer AllocateCommandBuffer(bool begin = false, bool compute = false) const;

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

	    // Accessor methods
	    [[nodiscard]] VkCommandPool GetGraphicsCmdPool() const { return GraphicsCmdPool; }
	    [[nodiscard]] VkCommandPool GetComputeCmdPool()	 const { return ComputeCmdPool; }
	    [[nodiscard]] VkCommandPool GetTransferCmdPool() const { return TransferCmdPool; }

	    Queue queueType;
	    VkCommandPool commandPool = VK_NULL_HANDLE;

	private:
	    VkCommandPool GraphicsCmdPool = VK_NULL_HANDLE;
	    VkCommandPool ComputeCmdPool = VK_NULL_HANDLE;
	    VkCommandPool TransferCmdPool = VK_NULL_HANDLE;
	};

	/// ---------------------------------------------------------

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
        static Ref<CommandPool> GetOrCreate(const Ref<VulkanDevice>& device, Queue queueType)
        {
            const auto tid = std::this_thread::get_id();
            auto& byQueue = Pools()[tid];
            const auto key = static_cast<int>(queueType);
            auto it = byQueue.find(key);
            if (it != byQueue.end() && it->second)
                return it->second;

            auto pool = CreateRef<CommandPool>(device, queueType);
            byQueue[key] = pool;
            return pool;
        }

        static void Shutdown()
        {
            auto& all = Pools();
            for (auto& entry : all)
            {
                auto& byQueue = entry.second;
                for (auto& qentry : byQueue)
                {
                    auto& pool = qentry.second;
                    pool.Reset();
                }
            }
            all.clear();
        }

    private:
        using PoolMap = std::unordered_map<int, Ref<CommandPool>>;
        static std::unordered_map<std::thread::id, PoolMap>& Pools()
        {
            static std::unordered_map<std::thread::id, PoolMap> s_pools;
            return s_pools;
        }
    };

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

    /// -------------------------------------------------------

    struct ImageBarrierInfo
    {
        void *image = nullptr;
        uint32_t aspect = 0;
        uint32_t mip = 0;
        uint32_t mip_range = 0;
        uint32_t arrayLen = 0;
        Layout::ImageLayout layout_old = Layout::ImageLayout::Max;
        Layout::ImageLayout layout_new = Layout::ImageLayout::Max;
        bool isDepth = false;
    };

    /// -------------------------------------------------------

	class CommandManager : public RefCounted
	{
	public:
		CommandManager(Queue *queue, void *cmdPool, const std::string &debugName);
        virtual ~CommandManager() override;

		void Begin();
        void Submit(FrameSync *semaphoreWait, const bool immediate);
        void ExecutionWait(const bool waitTime = false);
        void PipelineState();

        /// -------------------------------------------------------

		// Draw
		void Draw(const uint32_t count, const uint32_t vertexStartIdx = 0);
	    void DrawIndexed(const uint32_t count, const uint32_t indexOffset = 0, const uint32_t vertex_offset = 0, const uint32_t instance_index = 0, const uint32_t instance_count = 1);

		// Ref<VertexBuffer> SetBuffer();
		// Ref<IndexBuffer>  SetBuffer();

	    // Clear
        void ClearPipelineStateRenderTargets(Pipeline& pipeline_state);

        /*
        void ClearTexture(
            Texture* texture,
            const Color& clear_color     = colorLoad,
            const float clear_depth      = depthLoad,
            const uint32_t clear_stencil = stencilLoad
        );
        */

        // Dispatch
        void Dispatch(uint32_t x, uint32_t y, uint32_t z = 1);
        void Dispatch(Texture* texture);

        // blit
        void Blit(Texture* source, Texture* destination, const bool blit_mips, const float source_scaling = 1.0f);
        void Blit(Texture* source, SwapChain* destination);

        // copy
        void Copy(Texture* source, Texture* destination, const bool blit_mips);
        void Copy(Texture* source, SwapChain* destination);

        // viewport
        void SetViewport(const Viewport& viewport) const;

        // scissor
        void SetScissorRectangle(const xMath::Rectangle & scissor_rectangle) const;

        // cull mode
        void SetCullMode(const CullMode cull_mode);

        // buffers
        void SetBufferVertex(const Buffer* vertex, Buffer* instance = nullptr);
        void SetBufferIndex(const Buffer* buffer);
        void SetBuffer(const uint32_t slot, Buffer* buffer) const;
        //void SetBuffer(const Renderer_BindingsUav slot, Buffer* buffer) const { SetBuffer(static_cast<uint32_t>(slot), buffer); }
        void UpdateBuffer(Buffer *buffer, const uint64_t offset, const uint64_t size, const void *data);

        // constant buffer
        void SetConstantBuffer(const uint32_t slot, Buffer* constant_buffer) const;
        //void SetConstantBuffer(const Renderer_BindingsCb slot, Buffer* constant_buffer) const { SetConstantBuffer(static_cast<uint32_t>(slot), constant_buffer); }

        // push constant buffer
        void PushConstants(const uint32_t offset, const uint32_t size, const void* data);

        template<typename T>
        void PushConstants(const T& data)
        {
            PushConstants(0, sizeof(T), &data);
        }

        // texture
        // void SetTexture(const uint32_t slot, Texture* texture, const uint32_t mipIdx = allMips, uint32_t mip_range = 0, const bool uav = false);
        // void SetTexture(const Renderer_BindingsUav slot, Texture* texture,  const uint32_t mipIdx = allMips, uint32_t mip_range = 0) { SetTexture(static_cast<uint32_t>(slot), texture, mipIdx, mip_range, true); }
        // void SetTexture(const Renderer_BindingsSrv slot, Texture* texture,  const uint32_t mipIdx = allMips, uint32_t mip_range = 0) { SetTexture(static_cast<uint32_t>(slot), texture, mipIdx, mip_range, false); }

        // markers
        void BeginMarker(std::string debugName);
        void EndMarker();

        // timestamp queries
        uint32_t BeginTimestamp();
        void EndTimestamp();
        float GetTimestampResult(const uint32_t index_timestamp);

        // occlusion queries
        void BeginOcclusionQuery(const uint64_t entity_id);
        void EndOcclusionQuery();
        bool GetOcclusionQueryResult(const uint64_t entity_id);
        void UpdateOcclusionQueries();

        // time-blocks (cpu and gpu time measurement as well as gpu markers)
        void BeginTimeblock(std::string debugName, const bool gpu_marker = true, const bool gpu_timing = true);
        void EndTimeblock();

        // memory barriers
        void InsertBarrier(void* image, const VkFormat format, const uint32_t mipIdx, const uint32_t mip_range, const uint32_t arrayLength,const Layout::ImageLayout layoutNew);
        // void InsertBarrierReadWrite(Texture *texture, const BarrierType type);
        void InsertBarrierReadWrite(Buffer *buffer);
        void InsertPendingBarrierGroup();

        // misc
        void RenderPassEnd();
        FrameSync* GetRenderingCompleteSemaphore()		{ return m_rendering_complete_semaphore.Get(); }
        void* GetRhiResource() const                    { return m_resource; }
	    CommandState GetState() const					{ return m_state; }
        Queue* GetQueue() const							{ return m_queue; }

        // layouts
        static void RemoveLayout(void* image);
        static Layout::ImageLayout GetImageLayout(void* image, uint32_t mipIdx);

	private:
        void PreDraw();
        void RenderPassBegin();

        // sync
        Ref<FrameSync> m_rendering_complete_semaphore;
        Ref<FrameSync> m_rendering_complete_semaphore_timeline;

        // misc
        uint64_t m_buffer_id_vertex                         = 0;
        uint64_t m_buffer_id_index                          = 0;
        uint32_t m_timestamp_index                          = 0;
        Ref<Pipeline> * m_pipeline							= nullptr;
        //DescriptorSetLayout* m_descriptor_layout_current	= nullptr;
        std::atomic<CommandState> m_state					= CommandState::IDLE;
        CullMode m_cull_mode								= CullMode::Back;
        bool m_render_pass_active                           = false;
        uint32_t m_render_pass_draw_calls                   = 0;
        std::stack<const char*> m_active_timeblocks;
        std::stack<const char *> m_debug_label_stack;
        std::mutex m_mutex_reset;
        Pipeline m_pso;
        std::vector<ImageBarrierInfo> m_image_barriers;
        Queue *m_queue										= nullptr;
        bool m_load_depth_render_target						= false;
        //std::array<bool, rhi_max_render_target_count> m_load_color_render_targets = { false };

        // resources
        void* m_resource						= nullptr;
        void* m_cmdPool_resource				= nullptr;
        void* m_queryPool_timestamps			= nullptr;
        void* m_queryPool_pipelineStats			= nullptr;
        void* m_queryPool_occlusion				= nullptr;
	};

}

/// -------------------------------------------------------

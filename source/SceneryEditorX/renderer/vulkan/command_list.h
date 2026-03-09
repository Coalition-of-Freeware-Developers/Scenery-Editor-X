/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * command_list.h
 * -------------------------------------------------------
 * Created: 12/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "buffer.h"
#include "command_pool.h"
#include "queue.h"
#include "viewport.h"
#include "SceneryEditorX/renderer/renderer_declarations.h"
#include "pipeline/pipeline.h"
#include "sync/frame_sync.h"
#include <colors.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class DescriptorSet;
	class ImageResource;
	struct Texture;

    enum class CommandState : uint8_t
	{
	    Idle,
		Recording, 
	    Submitted
	};

    class CommandList : public SharedResource
    {
    public:
        CommandList(Queue* queue, const CommandPool &cmdPool, const char* name);
        virtual ~CommandList() override;

		void Begin();
		void Submit(FrameSync *semaphoreWait, const bool isImmediate, FrameSync *semaphoreSignal = nullptr, FrameSync *semaphoreTimeline = nullptr, uint64_t timelineValue = 0);
		void WaitForExecution(const bool logWaitTime = false);

		void ClearDepth(void *img, float clearDepth);
		void ClearStencil(void *img, uint32_t clearStencil);
		void ClearTexture(void* img, const Color &color);
		void ClearTexture(ImageResource* img, const Color &color);

		// Pipeline state
		void SetPipelineState(const PipelineState& pso);
		void PushConstants(const PushConstantBuffer& data);

		//void SetPipelineState(PipelineState &pso);

		Ref<CommandList> Get() { return {this}; }
		VkCommandBuffer GetCommandBuffer() const { return m_CmdBuffer; }

		void EndRenderPass();
		static void RemoveLayout(void* image);
		static Layout::ImageLayout GetImageLayout(void* image, uint32_t mipIndex);

		// Buffer bindings
		void UpdateBuffer(Buffer* buffer, const uint64_t offset, const uint64_t size, const void* data);
		void SetIndexBuffer(const Buffer *indexBuffer);
		void SetVertexBuffer(const Buffer *vertexBuffer, const Buffer *instance);
		void SetBufferVertex(Buffer* vertexBuffer);
		void SetBufferIndex(Buffer* indexBuffer);
		void SetBuffer(Renderer_BindingsUav slot, Buffer* buffer);

		// Barriers - unified interface
		//void InsertBarrier(const Barrier& barrier);
		//void FlushBarriers();

		// Barriers - convenience overloads
		void InsertBarrier(void* img, Layout::ImageLayout layout, uint32_t mip = ALL_MIPS, uint32_t mipRange = 0);
		void InsertBarrier(ImageResource* img, BarrierType type);
		//void InsertBarrier(void* texture, BarrierType syncType);
		void InsertBarrier(Buffer* buffer);
		void InsertBarrier(void* image, VkFormat format, uint32_t mipIndex, uint32_t mipRange, uint32_t arrayLength, Layout::ImageLayout layout);

		// Blit (compute-driven copy with optional resolution scaling)
		void Blit(ImageResource* src, ImageResource* dst, bool keepAspect, float resolutionScale = 1.0f);

		// Immediate Execution
		static CommandList* BeginImmediateExecution(const QueueType type);
		static void EndImmediateExecution(CommandList* cmdList);
		static void ShutdownImmediateExecution();

		const CommandState GetState() const { return m_State; }

		void SetTexture(const uint32_t slot, ImageResource* img, const uint32_t mipIndex = ALL_MIPS, uint32_t mipRange = 0, const bool uav = false);
		void SetTexture(const Renderer_BindingsUav slot, ImageResource* img,  const uint32_t mipIndex = ALL_MIPS, uint32_t mipRange = 0) { SetTexture(static_cast<uint32_t>(slot), img, mipIndex, mipRange, true); }
		void SetTexture(const Renderer_BindingsSrv slot, ImageResource* img,  const uint32_t mipIndex = ALL_MIPS, uint32_t mipRange = 0) { SetTexture(static_cast<uint32_t>(slot), img, mipIndex, mipRange, false); }


		void Draw(const uint32_t vertexCount, const uint32_t vertexOffset = 0);
		void DrawIndexed(uint32_t indexCount, uint32_t instCount = 1, uint32_t indexOffset = 0, uint32_t vertexOffset = 0, uint32_t instIndex = 0);
		void DrawIndexedIndirectCount(Buffer* drawArgs, uint64_t argsOffset, Buffer* countBuffer, uint64_t countOffset, uint32_t maxDrawCount);
		void Dispatch(uint32_t x, uint32_t y, uint32_t z = 1);
		void Dispatch(ImageResource *img, float resolutionScale = 1.0f);

		void SetViewport(const Viewport& viewport) const;
		void SetScissor(const xMath::Rectangle &scissorRect) const;
		void SetCullMode(const CullMode cullMode);

		Ref<Queue> GetQueue() const { return m_Queue; }
		//void CopyImageToBuffer(Image* src, Buffer* dst);
		void CopyBufferToBuffer(void* src, Buffer* dst, uint64_t size);
		void CopyBufferToBuffer(Buffer* src, Buffer* dst, uint64_t size);

		/**
		 * @brief Returns the timeline semaphore value that was last signaled by this command list's most recent submit.
		 * Used by dependent submissions on other queues to build timeline wait info.
		 */
		[[nodiscard]] uint64_t GetLastTimelineSignalValue() const { return m_LastTimelineSignalValue; }

		/**
		 * @brief Returns a raw pointer to the timeline FrameSync owned by this command list.
		 * Callers on other queues can read the VkSemaphore handle and current signal value from it.
		 */
		[[nodiscard]] FrameSync* GetTimelineSemaphore() const { return m_RenderingCompleteTimeline.Get(); }

	private:
		void BeginRenderPass();

		// Per-submission sync objects:
		//   m_RenderingCompleteSemaphore - binary semaphore, signaled when rendering is done (used for swapchain present chains)
		//   m_RenderingCompleteTimeline  - timeline semaphore, always signaled on every submit (used for cross-queue synchronization)
		//   m_SubmitSync                 - fence, used to CPU-block on immediate submissions via WaitForExecution()
		Ref<FrameSync> m_RenderingCompleteSemaphore;
		Ref<FrameSync> m_RenderingCompleteTimeline;
		Ref<FrameSync> m_SubmitSync;
		uint64_t m_LastTimelineSignalValue = 0;
		uint64_t m_NextTimelineSignalValue = 1;

		uint64_t m_BufferID_Vertex   = 0;
		uint64_t m_BufferID_Instance = 0;
		uint64_t m_BufferID_Index    = 0;

		Queue *m_Queue;
		Pipeline m_Pipeline;
		VkCommandBuffer m_CmdBuffer;
		std::atomic<CommandState> m_State = CommandState::Idle;
		VkCullModeFlags m_CullMode = VK_CULL_MODE_BACK_BIT;
		bool m_RenderPassActive = false;
		DescriptorSet* m_DescriptorLayout_Current = nullptr;

	};  

} // namespace SceneryEditorX

// -------------------------------------------------------

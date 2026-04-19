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
#include "pipeline/pipeline.h"
#include "pipeline/pipeline_state.h"
#include "sync/frame_sync.h"
#include <colors.h>
#include <SceneryEditorX/renderer/renderer_declarations.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct PendingBarrierInfo;
	struct PushConstantBuffer_Pass;
	class DescriptorSet;
	class ImageResource;
	struct Texture;

	/**
	 * @enum CommandState
	 * @brief Represents the state of a command list.
	 */
	enum class CommandState : uint8_t
	{
		Idle,
		Recording, 
		Submitted
	};
	
	// -------------------------------------------------------

	/**
	 * @class CommandList
	 * @brief Represents a command list that can be recorded and submitted for execution on the GPU.
	 */
	class CommandList : public SharedResource
	{
	public:
		/**
		 * @brief Construct a new Command List object.
		 * @param queue The queue to submit the command list to.
		 * @param cmdPool The command pool to allocate the command list from.
		 * @param name The name of the command list.
		 */
		CommandList(Queue* queue, const CommandPool &cmdPool, const char* name);
		virtual ~CommandList() override;

		/**
		 * @brief Begin recording commands into the command list.
		 */
		void Begin();

		/**
		 * @brief End recording without submitting to a queue.
		 *
		 * Use when the raw VkCommandBuffer will be included in an external VkSubmitInfo
		 * (e.g. the renderer's swapchain submit in SubmitAndPresent).  Transitions the
		 * internal state from Recording → Idle so that Begin() can be called next frame
		 * without a WaitForExecution stall.
		 */
		void Seal();

		/**
		 * @brief Temporarily redirect this CommandList to record into an externally-owned
		 *        VkCommandBuffer that is already in the recording state.
		 *
		 * The caller is responsible for the lifetime of @p externalCb.  The original
		 * VkCommandBuffer is saved and restored by calling RestoreCommandBuffer().
		 * This is used to inject UI draw calls into the renderer's swapchain command
		 * buffer (which has an active dynamic render pass) without breaking the
		 * CommandList abstraction.
		 *
		 * @param externalCb       An already-recording VkCommandBuffer to redirect draws into.
		 * @param renderPassActive Whether a render pass is currently active on @p externalCb.
		 */
		void SetExternalRecordingBuffer(VkCommandBuffer externalCb, bool renderPassActive);

		/**
		 * @brief Restore the VkCommandBuffer that was replaced by SetExternalRecordingBuffer.
		 *        Also restores the render-pass-active and state flags to their saved values.
		 */
		void RestoreCommandBuffer();

		/**
		 * @brief Submit the command list for execution.
		 * @param semaphoreWait The semaphore to wait on before execution.
		 * @param isImmediate Whether the submission is immediate.
		 * @param semaphoreSignal The semaphore to signal after execution.
		 * @param semaphoreTimeline The timeline semaphore to signal.
		 * @param timelineValue The value to signal on the timeline semaphore.
		 */
		void Submit(FrameSync *semaphoreWait, const bool isImmediate, FrameSync *semaphoreSignal = nullptr, FrameSync *semaphoreTimeline = nullptr, uint64_t timelineValue = 0);

		/**
		 * @brief Wait for the command list to finish execution.
		 * @param logWaitTime Whether to log the wait time.
		 */
		void WaitForExecution(const bool logWaitTime = false);
		
		// -------------------------------------------------------

		/**
		 * @brief Clear the depth buffer of an image.
		 * @param img The image resource.
		 * @param clearDepth The depth value to clear to.
		 */
		void ClearDepth(VkImage img, float clearDepth);

		/**
		 * @brief Clear the stencil buffer of an image.
		 * @param img The image resource.
		 * @param clearStencil The stencil value to clear to.
		 */
		void ClearStencil(VkImage img, uint32_t clearStencil);

		/**
		 * @brief Clear the color buffer of an image.
		 * @param img The image resource.
		 * @param color The color to clear to.
		 */
		void ClearTexture(VkImage img, const Color &color);

		/**
		 * @brief Clear the color buffer of an image.
		 * @param img The image resource.
		 * @param color The color to clear to.
		 */
		void ClearTexture(ImageResource* img, const Color &color);
		
		// -------------------------------------------------------

		/**
		 * @brief Set the pipeline state for the command list.
		 * @param pso The pipeline state object to set.
		 */
		void SetPipelineState(PipelineState& pso);

		/**
		 * @brief Push constants to the pipeline.
		 * @param data The push constant data to set.
		 */
		void PushConstants(const PushConstantBuffer_Pass& data);

		// -------------------------------------------------------

		/**
		 * @brief 
		 * @return 
		 */
		Ref<CommandList> Get() { return {this}; }

		/**
		 * @brief 
		 * @return 
		 */
		VkCommandBuffer GetCommandBuffer() const { return m_CmdBuffer; }

		/* @brief End the current render pass. */
		void EndRenderPass();

		/**
		 * @brief Remove the layout of an image resource.
		 * @param image The image resource (raw pointer to VkImage or ImageResource pointer).
		 */
		static void RemoveLayout(void* image);

		/* @brief Get the tracked layout for an ImageResource pointer. */
		static Layout::ImageLayout GetImageLayout(ImageResource* image, uint32_t mipIndex);

		/* @brief Get the tracked layout for a pointer to a VkImage (e.g. ImageResource::Get()). */
		static Layout::ImageLayout GetImageLayout(VkImage* imagePtr, uint32_t mipIndex);

		/**
		 * @brief Get the layout of an image resource.
		 * @param image The image resource.
		 * @param mipIndex The mip level to query.
		 * @return The image layout.
		 */
		static Layout::ImageLayout GetImageLayout(VkImage image, uint32_t mipIndex);

		// -------------------------------------------------------

		/**
		 * @brief Update the contents of a buffer.
		 * @param buffer The buffer to update.
		 * @param offset The offset within the buffer to start updating.
		 * @param size The size of the data to update.
		 * @param data The data to write to the buffer.
		 */
		static void UpdateBuffer(Buffer* buffer, const uint64_t offset, const uint64_t size, const void* data);

		/**
		 * @brief Bind an index buffer for use in rendering.
		 * @param indexBuffer The index buffer to bind.
		 */
		void SetIndexBuffer(const Buffer *indexBuffer);

		/**
		 * @brief Bind a vertex buffer and an instance buffer for use in rendering.
		 * @param vertexBuffer The vertex buffer to bind.
		 * @param instance The instance buffer to bind.
		 */
		void SetVertexBuffer(const Buffer *vertexBuffer, const Buffer *instance);

		/**
		 * @brief Bind a vertex buffer for use in rendering.
		 * @param vertexBuffer The vertex buffer to bind.
		 */
		void SetVertexBuffer(Buffer* vertexBuffer);

		/**
		 * @brief Bind an index buffer for use in rendering.
		 * @param indexBuffer The index buffer to bind.
		 */
		void SetIndexBuffer(Buffer* indexBuffer);

		/**
		 * @brief Bind a buffer to a specific slot for use in shaders.
		 * @param slot The slot to bind the buffer to.
		 * @param buffer The buffer to bind.
		 */
		void SetBuffer(const uint32_t slot, Buffer* buffer) const;

		// -------------------------------------------------------

		/**
		 * @brief Insert a barrier for an image resource.
		 * @param img The image resource.
		 * @param layout The desired image layout.
		 * @param mip The mip level to apply the barrier to.
		 * @param mipRange The range of mip levels to apply the barrier to.
		 */
		void InsertBarrier(VkImage img, Layout::ImageLayout layout, uint32_t mip = ALL_MIPS, uint32_t mipRange = 0);

		/**
		 * @brief Insert a barrier for an image resource.
		 * @param img The image resource.
		 * @param type The type of barrier to insert.
		 */
		void InsertBarrier(ImageResource* img, BarrierType type);

		/**
		 * @brief Insert a barrier for a buffer resource.
		 * @param buffer The buffer resource.
		 */
		void InsertBarrier(Buffer* buffer);

		/**
		 * @brief Insert a barrier for an image resource with detailed parameters.
		 * @param image The image resource.
		 * @param format The format of the image.
		 * @param mipIndex The mip level to apply the barrier to.
		 * @param mipRange The range of mip levels to apply the barrier to.
		 * @param arrayLength The number of array layers to apply the barrier to.
		 * @param layout The desired image layout.
		 */
		void InsertBarrier(VkImage image, VkFormat format, uint32_t mipIndex, uint32_t mipRange, uint32_t arrayLength, Layout::ImageLayout layout);

		/**
		 * @brief Insert a barrier for an image when you have a pointer to a VkImage (e.g. &vector[index] or ImageResource::Get()).
		 * @param imagePtr Pointer to the VkImage handle.
		 * @param format The format of the image.
		 * @param mipIndex The mip level to apply the barrier to.
		 * @param mipRange The range of mip levels to apply the barrier to.
		 * @param arrayLength The number of array layers to apply the barrier to.
		 * @param layout The desired image layout.
		 */
		void InsertBarrier(VkImage* imagePtr, VkFormat format, uint32_t mipIndex, uint32_t mipRange, uint32_t arrayLength, Layout::ImageLayout layout);

		/**
		 * @brief Insert a barrier for an image when you have a pointer to a VkImage (simple overload).
		 * @param imgPtr Pointer to the VkImage handle.
		 * @param layout The desired image layout.
		 * @param mip The mip level to apply the barrier to (default is ALL_MIPS).
		 * @param mipRange The range of mip levels to apply the barrier to (default is 0).
		 */
		void InsertBarrier(VkImage* imgPtr, Layout::ImageLayout layout, uint32_t mip = ALL_MIPS, uint32_t mipRange = 0);

		/* @brief Flush all pending barriers to ensure proper synchronization before executing draw or dispatch commands. */
		void FlushBarriers();

		// -------------------------------------------------------

		/**
		 * @brief Blit (compute-driven copy with optional resolution scaling)
		 * @param src The source image resource.
		 * @param dst The destination image resource.
		 * @param blitMips Whether to blit all mip levels.
		 * @param sourceScaling The scale factor for the resolution.
		 */
		void Blit(ImageResource *src, ImageResource *dst, const bool blitMips, const float sourceScaling = 1.0f);

		/**
		 * @brief Blit (compute-driven copy without resolution scaling)
		 * @param src The source image resource.
		 * @param dst The destination image resource.
		 */
		void Blit(ImageResource *src, Swapchain *dst);

		/**
		 * @brief 
		 * @param src  
		 * @param dst 
		 * @param dstLayer 
		 */
		void BlitToArrayLayer(ImageResource *src, ImageResource *dst, uint32_t dstLayer);

		// -------------------------------------------------------

		/**
		* @brief Begin immediate command recording on the queue matching @p type.
		* This uses QueueManager's reusable command list pool and starts recording
		* immediately. The returned command list must be completed with
		* EndImmediateExecution().
		*/
		static CommandList* BeginImmediateExecution(const QueueType type);

		/**
		 * @brief End immediate command recording for the given command list.
		 * @param cmdList The command list to end immediate execution for.
		 */
		static void EndImmediateExecution(CommandList* cmdList);

		/* @brief Shutdown immediate execution for all command lists. */
		static void ShutdownImmediateExecution();

		/**
		 * @brief 
		 * @return 
		 */
		const CommandState GetState() const { return m_State.load(); }
		
		// -------------------------------------------------------

		/**
		 * @brief Set a texture for a specific slot.
		 * @param slot The binding slot to set the texture to (can be either UAV or SRV).
		 * @param img The image resource to bind.
		 * @param mipIndex The mip level to bind (default is ALL_MIPS).
		 * @param mipRange The range of mip levels to bind (default is 0).
		 * @param uav Whether the texture is a UAV (default is false).
		 */
		void SetTexture(const uint32_t slot, ImageResource* img, const uint32_t mipIndex = ALL_MIPS, uint32_t mipRange = 0, const bool uav = false);

		/**
		 * @brief Set a texture for a specific slot.
		 * @param slot The binding slot to set the texture to (can be either UAV or SRV).
		 * @param img The image resource to bind.
		 * @param mipIndex The mip level to bind (default is ALL_MIPS).
		 * @param mipRange The range of mip levels to bind (default is 0).
		 */
		void SetTexture(const Renderer_BindingsUav slot, ImageResource* img,  const uint32_t mipIndex = ALL_MIPS, uint32_t mipRange = 0) { SetTexture(static_cast<uint32_t>(slot), img, mipIndex, mipRange, true); }

		/**
		 * @brief Set a texture for a specific slot.
		 * @param slot The binding slot to set the texture to (can be either UAV or SRV).
		 * @param img The image resource to bind.
		 * @param mipIndex The mip level to bind (default is ALL_MIPS).
		 * @param mipRange The range of mip levels to bind (default is 0).
		 */
		void SetTexture(const Renderer_BindingsSrv slot, ImageResource* img,  const uint32_t mipIndex = ALL_MIPS, uint32_t mipRange = 0) { SetTexture(static_cast<uint32_t>(slot), img, mipIndex, mipRange, false); }

		/**
		 * @brief Bind a buffer to a specific UAV slot using the typed binding enum.
		 * @param slot The Renderer_BindingsUav slot to bind the buffer to.
		 * @param buffer The buffer to bind.
		 */
		void SetBuffer(const Renderer_BindingsUav slot, Buffer* buffer) const { SetBuffer(static_cast<uint32_t>(slot), buffer); }

		// -------------------------------------------------------

		/**
		 * @brief Draw non-indexed geometry.
		 * @param vertexCount The number of vertices to draw.
		 * @param vertexOffset The offset within the vertex buffer (default is 0).
		 */
		void Draw(const uint32_t vertexCount, const uint32_t vertexOffset = 0);

		/**
		 * @brief Draw indexed geometry.
		 *
		 * @param indexCount The number of indices to draw.
		 * @param instCount The number of instances to draw (default is 1).
		 * @param indexOffset The offset within the index buffer (default is 0).
		 * @param vertexOffset The offset within the vertex buffer (default is 0).
		 * @param instIndex The starting instance index (default is 0).
		 */
		void DrawIndexed(uint32_t indexCount, uint32_t instCount = 1, uint32_t indexOffset = 0, uint32_t vertexOffset = 0, uint32_t instIndex = 0);

		/**
		 * @brief Draw indexed indirect with count parameters sourced from buffers.
		 *
		 * @param drawArgs The buffer containing the draw arguments.
		 * @param argsOffset The offset within the draw arguments buffer.
		 * @param countBuffer The buffer containing the draw count.
		 * @param countOffset The offset within the count buffer.
		 * @param maxDrawCount The maximum number of draws to execute.
		 */
		void DrawIndexedIndirectCount(Buffer* drawArgs, uint64_t argsOffset, Buffer* countBuffer, uint64_t countOffset, uint32_t maxDrawCount);

		/**
		 * @brief Dispatch a compute shader with the given workgroup dimensions.
		 *
		 * @param x The number of workgroups to dispatch in the X dimension.
		 * @param y The number of workgroups to dispatch in the Y dimension.
		 * @param z The number of workgroups to dispatch in the Z dimension (default is 1).
		 */
		void Dispatch(uint32_t x, uint32_t y, uint32_t z = 1);

		/**
		 * @brief Dispatch a compute shader with the given image resource.
		 *
		 * @param img The image resource to use for the dispatch.
		 * @param resolutionScale The scale factor for the resolution (default is 1.0f).
		 */
		void Dispatch(ImageResource *img, float resolutionScale = 1.0f);
		
		// -------------------------------------------------------

		/**
		 * @brief 
		 * @param viewport 
		 */
		void SetViewport(const Viewport& viewport) const;

		/**
		 * @brief 
		 * @param scissorRect 
		 */
		void SetScissor(const xMath::Rectangle &scissorRect) const;

		/**
		 * @brief 
		 * @param cullMode 
		 */
		void SetCullMode(const CullMode cullMode);

		/**
		 * @brief 
		 * @return 
		 */
		Ref<Queue> GetQueue() const { return m_Queue; }

		/**
		 * @brief 
		 * @param src 
		 * @param dst 
		 */
		void Copy(ImageResource* src, Swapchain* dst);

		/**
		 * @brief 
		 * @param src 
		 * @param dst 
		 */
		void Copy(ImageResource* src, ImageResource* dst, const bool blitMips);

		/**
		 * @brief Copy data from a raw pointer to a buffer.
		 *
		 * @param src The source data pointer.
		 * @param dst The destination buffer.
		 * @param size The size of the data to copy.
		 */
		void CopyBufferToBuffer(void* src, Buffer* dst, uint64_t size);

		/**
		 * @brief Copy data from one buffer to another.
		 *
		 * @param src The source buffer.
		 * @param dst The destination buffer.
		 * @param size The size of the data to copy.
		 */
		void CopyBufferToBuffer(Buffer* src, Buffer* dst, uint64_t size);
		
		// -------------------------------------------------------

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
		/**
		 * @brief Ensure a descriptor layout exists for the provided PSO.
		 *
		 * Builds a DescriptorSet layout from reflected shader descriptors via
		 * BindlessManager::GetDescriptorsFromPipelineState when none is currently bound.
		 *
		 * @param pso Pipeline state to reflect descriptors from.
		 * @return True if a descriptor layout is available after the call.
		 */
		bool EnsureDescriptorLayoutFromPipelineState(PipelineState& pso);

		/**
		 * @brief Prepare the command list for drawing by flushing barriers and ensuring a render pass is active if needed.
		 * This is called at the beginning of Draw/DrawIndexed/Dispatch to ensure the command list is in the correct state for rendering.
		 */
		void PreDraw();

		/**
		 * @brief Begin a render pass if one is not already active.
		 * This is called by PreDraw() to ensure a render pass is active before drawing.
		 */
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
		VkCommandBuffer m_CmdBuffer;
		VkCommandBuffer m_SavedCmdBuffer        = VK_NULL_HANDLE;  // saved by SetExternalRecordingBuffer
		bool            m_SavedRenderPassActive = false;            // saved by SetExternalRecordingBuffer
		std::atomic<CommandState> m_State = CommandState::Idle;
		VkCullModeFlags m_CullMode = VK_CULL_MODE_BACK_BIT;
		bool m_RenderPassActive = false;
		Scope<DescriptorSet> m_DescriptorLayout_Owned;
		DescriptorSet* m_DescriptorLayout_Current = nullptr;

		Pipeline m_Pipeline;
		PipelineState m_pso;
		std::vector<PendingBarrierInfo> m_PendingBarriers;
		std::array<bool, MAX_RENDER_TARGET_COUNT> m_Load_Color_RenderTargets = { false };
		bool m_Load_Depth_RenderTarget = false;
		bool m_ComputePushConstantsSet = false;
		mutable bool m_NeedsDynamicBind = false;

	};  

} // namespace SceneryEditorX

// -------------------------------------------------------

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
#include "barriers.h"
#include "buffer.h"
#include "command_pool.h"
#include "frame_sync.h"
#include "image.h"
#include "pipeline.h"
#include "queue.h"
#include "viewport.h"

#include <colors.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	enum class CommandState : uint8_t
	{
	    Idle,
		Recording, 
	    Submitted
	};

    class CommandList : public RefCounted
    {
    public:
        CommandList(Queue* queue, CommandPool cmdPool, const char* name);
        virtual ~CommandList() override;

        void Begin();
        void Submit(FrameSync *semaphoreWait, const bool isImmediate, FrameSync *semaphoreSignal = nullptr);
        void WaitForExecution(const bool logWaitTime = false);
        void ClearTexture(Image *img, const Color &color, float clearDepth, uint32_t clearStencil);
        void SetVertexBuffer(const Buffer *vertexBuffer, const Buffer *instance);
        //void SetPipelineState(PipelineState &pso);

		Ref<CommandList> Get() { return {this}; }
        VkCommandBuffer GetCommandBuffer() const { return m_CmdBuffer; }

        void EndRenderPass();
        static void RemoveLayout(void* image);
        static Layout::ImageLayout GetImageLayout(void* image, uint32_t mipIndex);

        // Buffer
        void UpdateBuffer(Buffer* buffer, const uint64_t offset, const uint64_t size, const void* data);

        // Barriers - unified interface
        void InsertBarrier(const Barrier& barrier);
        void FlushBarriers();

        // Barriers - convenience overloads
        void InsertBarrier(Image* texture, Layout::ImageLayout layout, uint32_t mip = ALL_MIPS, uint32_t mipRange = 0);
        void InsertBarrier(Image* texture, BarrierType syncType);
        void InsertBarrier(Buffer* buffer);
        void InsertBarrier(void* image, VkFormat format, uint32_t mipIndex, uint32_t mipRange, uint32_t arrayLength, Layout::ImageLayout layout);

		// Immediate Execution
        static CommandList* BeginImmediateExecution(const QueueType type);
        static void EndImmediateExecution(CommandList* cmdList);
        static void ShutdownImmediateExecution();

        const CommandState GetState() const { return m_State; }

		void Draw(const uint32_t vertexCount, const uint32_t vertexOffset = 0);
        void DrawIndexed(uint32_t indexCount, uint32_t instCount, uint32_t indexOffset, uint32_t vertexOffset, uint32_t instIndex);

        void SetViewport(const Viewport& viewport) const;
        void SetScissor(const xMath::Rectangle &scissorRect) const;
        void SetCullMode(const VkCullModeFlags cullMode);

        Ref<Queue> GetQueue() const { return m_Queue; }
        void CopyImageToBuffer(Image* src, Buffer* dst);
        void CopyBufferToBuffer(void* src, Buffer* dst, uint64_t size);
        void CopyBufferToBuffer(Buffer* src, Buffer* dst, uint64_t size);

    private:
        void BeginRenderPass();
        Queue *m_Queue;
        Pipeline m_Pipeline;
        VkCommandBuffer m_CmdBuffer;
        Ref<FrameSync> m_SubmitSync = nullptr;
		std::atomic<CommandState> m_State = CommandState::Idle;
		VkCullModeFlags m_CullMode = VK_CULL_MODE_BACK_BIT;
		bool m_RenderPassActive = false;
    };

} // namespace SceneryEditorX

// -------------------------------------------------------

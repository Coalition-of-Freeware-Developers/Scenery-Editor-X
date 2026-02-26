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
 * command_list.cpp
 * -------------------------------------------------------
 * Created: 12/02/2026
 * -------------------------------------------------------
 */
#include "command_list.h"
#include "buffer.h"
#include "renderer.h"
#include <tracy/Tracy.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{

	CommandList::CommandList(Queue *queue, CommandPool cmdPool, const char *name)
	{
        m_Queue = queue;

	}
	
	CommandList::~CommandList()
	{
	}
	
	void CommandList::Begin()
	{
	}
	
	void CommandList::Submit(Sync *semaphoreWait, const bool isImmediate, Sync *semaphoreSignal)
	{
	}
	
	void CommandList::WaitForExecution(const bool logWaitTime)
	{
	}

    void CommandList::SetVertexBuffer(const Buffer *vertexBuffer,  Buffer* instance)
    {
        /*
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command List must be in Recording state to set vertex buffer.");

		if (!instance)
		{
            instance = Renderer::GetBuffer(Buffer::Instance);
		}

        // prepare buffers and offsets arrays
        VkBuffer vertex_buffers[2] = {

            static_cast<VkBuffer>(vertexBuffer->GetResource()),  // slot 0: vertex buffer
            static_cast<VkBuffer>(instance->GetResource()) // slot 1: instance buffer
        };
        SEDX_CORE_ASSERT(vertex_buffers[0] != nullptr && vertex_buffers[1] != nullptr);

        VkDeviceSize offsets[2] = {0, 0};

        // check if vertex buffer id has changed to trigger binding
        if (m_buffer_id_vertex != vertexBuffer->GetObjectId() || m_buffer_id_instance != instance->GetObjectId())
        {
            vkCmdBindVertexBuffers(static_cast<VkCommandBuffer>(m_rhi_resource), // commandbuffer
                                   0,                                            // firstbinding
                                   2,                                            // bindingcount
                                   vertex_buffers,                               // pbuffers
                                   offsets                                       // poffsets
            );

            // track currently bound buffers
            m_buffer_id_vertex = vertexBuffer->GetObjectId();
            m_buffer_id_instance = instance->GetObjectId();
        }
        */

    }

    void CommandList::Draw(const uint32_t vertexCount, const uint32_t vertexOffset)
    {
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command List must be in Recording state to issue draw calls.");

        vkCmdDraw(m_CmdBuffer, vertexCount, 1, vertexOffset, 0);
    }

    void CommandList::DrawIndexed(const uint32_t indexCount, const uint32_t instCount, const uint32_t indexOffset, const uint32_t vertexOffset, const uint32_t instIndex)
    {
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command List must be in Recording state to issue draw calls.");

        vkCmdDrawIndexed(m_CmdBuffer, indexCount, instCount, indexOffset, static_cast<int32_t>(vertexOffset), instIndex);
    }

    void CommandList::SetViewport(const Viewport &viewport) const
    {
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command List must be in Recording state to set viewport.");
        SEDX_CORE_ASSERT(viewport.width > 0.0f);
        SEDX_CORE_ASSERT(viewport.height > 0.0f);

        VkViewport view;
        view.x			= viewport.x;
        view.y			= viewport.y;
        view.width		= viewport.width;
        view.height		= viewport.height;
        view.minDepth	= viewport.depth_min;
        view.maxDepth	= viewport.depth_max;

        vkCmdSetViewport(m_CmdBuffer, 0, 1, &view);
    }

    void CommandList::SetScissor(const xMath::Rectangle& scissorRect) const
	{
        SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command List must be in Recording state to set scissor.");

        VkRect2D scissor;
        scissor.offset.x = static_cast<int32_t>(scissorRect.x);
        scissor.offset.y = static_cast<int32_t>(scissorRect.y);
        scissor.extent.width = static_cast<uint32_t>(scissorRect.width);
        scissor.extent.height = static_cast<uint32_t>(scissorRect.height);

        vkCmdSetScissor(m_CmdBuffer, 0, 1, &scissor);
    }

    void CommandList::SetCullMode(const VkCullModeFlags cullMode)
    {
		SEDX_CORE_ASSERT(m_State == CommandState::Recording, "Command List must be in Recording state to set cull mode.");

		m_CullMode = cullMode;
        vkCmdSetCullMode(m_CmdBuffer, cullMode);
    }



} // namespace SceneryEditorX

// -------------------------------------------------------

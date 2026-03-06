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
 * frame_sync.cpp
 * -------------------------------------------------------
 * Created: 12/02/2026
 * -------------------------------------------------------
 */
#include "frame_sync.h"
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include <SceneryEditorX/renderer/vulkan/debug/graphics_debug.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	FrameSync::FrameSync(const SyncType type) : m_Type(type)
	{
		if (m_Type == SyncType::Fence)
		{
			m_Fence = CreateRef<Fence>();
			m_Fence->CreateSyncObject();
			Debugging::SetResourceName(m_Fence.Get()->GetFence(), ResourceType::Fence, "Fence");
		}
		else
		{
			// Pass the concrete SyncType (Semaphore or SemaphoreTimeline) so that
			// Semaphore::CreateSyncObject creates the correct Vulkan semaphore type.
			m_RenderSemaphore = CreateRef<Semaphore>(m_Type);
			m_RenderSemaphore->CreateSyncObject();
			Debugging::SetResourceName(m_RenderSemaphore.Get()->GetSemaphore(), ResourceType::Semaphore, "RenderSemaphore");
		}
	}

    FrameSync::~FrameSync()
    {
        if (m_Fence)
        {
            m_Fence->Destroy();
            m_Fence.Reset();
        }

        if (m_RenderSemaphore)
        {
            m_RenderSemaphore->Destroy();
            m_RenderSemaphore.Reset();
        }
    }

	void FrameSync::Create(const uint32_t framesInFlight, const uint32_t swapchainImageCount)
	{
	    // Intentionally minimal for now; keep placeholder for future allocation strategy.
	    (void)framesInFlight;
	    (void)swapchainImageCount;
	}

} // namespace SceneryEditorX

// -------------------------------------------------------

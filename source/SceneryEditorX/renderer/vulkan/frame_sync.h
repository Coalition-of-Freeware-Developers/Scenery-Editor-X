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
 * frame_sync.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "fence.h"
#include "semaphore.h"
#include <cstdint>

// -------------------------------------------------------

namespace SceneryEditorX
{
    class CommandList;
    class RenderContext;
	
    // -------------------------------------------------------

    class FrameSync : public RefCounted
    {
	public:
        FrameSync(const SyncType type);
        ~FrameSync() = default;
        static void Create(uint32_t framesInFlight, uint32_t swapchainImageCount);

        uint64_t GetNextSignalValue() { return ++m_Value; }
        [[nodiscard]] uint64_t GetValue() const { return m_Value; }

        // Signaler command list
        void SetUserCmdList(CommandList *cmdList) { m_User_CmdList = cmdList; }
        [[nodiscard]] CommandList *GetUserCmdList() const { return m_User_CmdList; }

        // Access the underlying Vulkan handles (null-safe)
        [[nodiscard]] VkSemaphore GetVkSemaphore() const { return m_RenderSemaphore ? m_RenderSemaphore->GetSemaphore() : VK_NULL_HANDLE; }
        [[nodiscard]] VkFence GetVkFence() const { return m_Fence ? m_Fence->GetFence() : VK_NULL_HANDLE; }

        // Expose refs if callers need strong access
        [[nodiscard]] Ref<Semaphore> GetSemaphoreRef() const { return m_RenderSemaphore; }
        [[nodiscard]] Ref<Fence> GetFenceRef() const { return m_Fence; }

	private:
		Ref<Fence> m_Fence;
        Ref<Semaphore> m_RenderSemaphore;

		uint64_t m_Value = 0;
		SyncType m_Type = SyncType::MaxEnum;
		CommandList *m_User_CmdList = nullptr;
	};

}

// -------------------------------------------------------

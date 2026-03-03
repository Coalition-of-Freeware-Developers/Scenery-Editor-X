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
 * uniform_buffer_set.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "render_data.h"
#include <array>
#include <vma/vk_mem_alloc.h>

// -------------------------------------------------------


namespace SceneryEditorX
{
	class Device;
	
	// Manages a set of per-frame uniform buffers (ShaderDataBuffer). Uses VMA to
	// allocate buffers that are host-visible and retrieves m_Device addresses for
	// shader m_Device address push-constant usage.
	class UniformBufferSet
	{
	public:
	    UniformBufferSet() = default;
	    UniformBufferSet(VmaAllocator allocator);
        ~UniformBufferSet();

        void Create();
	    void Destroy();
	
	    std::array<ShaderDataBuffer, MAX_FRAMES_IN_FLIGHT> &Buffers() { return m_Buffers; }
	
	private:
        Ref<Device> m_Device;
	    VmaAllocator m_Allocator;
	    std::array<ShaderDataBuffer, MAX_FRAMES_IN_FLIGHT> m_Buffers{};
	    bool m_Destroyed = false;
	};
	
} // namespace SceneryEditorX


// -------------------------------------------------------

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
 * uniform_buffer_set.cpp
 * -------------------------------------------------------
 * Created: 15/02/2026
 * -------------------------------------------------------
 */
#include "uniform_buffer_set.h"
#include "render_context.h"
#include "debug/graphics_debug.h"
#include <SceneryEditorX/utils/pointers.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	UniformBufferSet::UniformBufferSet(VmaAllocator allocator) : m_Allocator(allocator)
	{
		Ref<Device> device = RenderContext::Get()->GetDevice();
		m_Device = device;

		Create();
	}

	UniformBufferSet::~UniformBufferSet()
	{
		// best-effort cleanup; call destroy(m_Device, m_Allocator) explicitly before m_Allocator/m_Device teardown
		if (!m_Destroyed)
		{
			Destroy();
		}

		m_Device.Reset();
		m_Device = nullptr;
	}

	void UniformBufferSet::Create()
	{
		if (!m_Allocator || !m_Device)
			return;

		m_Destroyed = false;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			if (m_BufferObjects[i].IsValid() && m_BufferObjects[i]->Valid())
				continue;

			VkBufferCreateInfo uBufferCI = {};
			uBufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			uBufferCI.size = sizeof(ShaderData);
			uBufferCI.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

			VmaAllocationCreateInfo uBufferAllocCI = {};
			uBufferAllocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
								   VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
								   VMA_ALLOCATION_CREATE_MAPPED_BIT;
			uBufferAllocCI.usage = VMA_MEMORY_USAGE_AUTO;

			// Create Buffer object which will call MemoryAllocator::CreateBuffer internally
			m_BufferObjects[i] = CreateRef<Buffer>(m_Allocator, sizeof(ShaderData), uBufferCI.usage, uBufferAllocCI);
			if (!m_BufferObjects[i] || !m_BufferObjects[i]->Valid())
			{
				SEDX_CORE_WARN_TAG("UniformBufferSet", "Buffer wrapper creation failed for frame {}", i);
				m_BufferObjects[i].Reset();
				continue;
			}

			// Populate ShaderDataBuffer from Buffer
			m_Buffers[i].buffer = m_BufferObjects[i]->Get();
			m_Buffers[i].allocation = m_BufferObjects[i]->Allocation();
			m_Buffers[i].mapped = m_BufferObjects[i]->Map();
			m_Buffers[i].deviceAddress = m_BufferObjects[i]->DeviceAddress();
			Debugging::SetResourceName(m_BufferObjects[i]->Get(), ResourceType::UniformBufferSet, "UniformBufferSet_Frame_" + i);
		}
	}

	void UniformBufferSet::Destroy()
	{
		if (m_Destroyed)
			return;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			if (m_BufferObjects[i].IsValid() && m_BufferObjects[i]->Valid())
			{
				m_BufferObjects[i]->Destroy();
				m_BufferObjects[i].Reset();

				m_Buffers[i].buffer = VK_NULL_HANDLE;
				m_Buffers[i].allocation = VK_NULL_HANDLE;
				m_Buffers[i].mapped = nullptr;
				m_Buffers[i].deviceAddress = 0;
			}
		}

		m_Destroyed = true;
	}

};

// -------------------------------------------------------

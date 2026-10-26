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
 * texture_handle.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "texture_image.h"
#include <string>
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include <vma/vk_mem_alloc.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	// RAII wrapper around loaded Texture. Provides explicit destroy(m_Device, m_Allocator)
	// to ensure correct teardown order relative to m_Allocator/m_Device.
	class TextureHandle 
	{
	public:
		TextureHandle() = default;
		TextureHandle(VmaAllocator allocator, VkCommandPool cmdPool, VkQueue queue, const std::string& filename)
		{
			TextureImage loader;
			m_Texture = loader.Load(allocator, cmdPool, queue, filename);

			SEDX_CORE_ASSERT(m_Texture.image != VK_NULL_HANDLE, "Failed to load texture: {}", filename);
		}
	
		~TextureHandle()
		{
			if (!m_Destroyed)
			{
				// best-effort: user should call destroy explicitly before m_Allocator/m_Device teardown
			}
		}
	
		void Destroy(VmaAllocator allocator)
		{
			Ref<Device> device = RenderContext::Get()->GetDevice();

			if (!m_Destroyed)
			{
				if (m_Texture.view != VK_NULL_HANDLE)
				{
					vkDestroyImageView(device->GetLogicalDevice(), m_Texture.view, nullptr);
				}
				if (m_Texture.sampler != VK_NULL_HANDLE)
				{
					vkDestroySampler(device->GetLogicalDevice(), m_Texture.sampler, nullptr);
				}
				if (m_Texture.image != VK_NULL_HANDLE)
				{
					vmaDestroyImage(allocator, m_Texture.image, m_Texture.allocation);
				}
				m_Destroyed = true;
			}
		}

		[[nodiscard]] const Texture& Get() const { return m_Texture; }
	
	private:
		Texture m_Texture{};
		bool m_Destroyed = false;
	};

}

// -------------------------------------------------------

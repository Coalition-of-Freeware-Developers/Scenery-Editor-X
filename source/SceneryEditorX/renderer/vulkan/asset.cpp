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
 * asset.cpp
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#include "asset.h"
#include "render_context.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

	Asset::Asset()
	{
        m_Device = RenderContext::Get()->GetDevice();
	}

    Asset::~Asset()
    {
        m_Textures.clear(); // TextureHandle destructor will release image/sampler/views if not already destroyed
        m_Device.Reset(); // Release reference to device (if any) before destroying resources
    }

    bool Asset::Load(VmaAllocator allocator, VkCommandPool cmdPool, VkQueue queue, const std::string& modelFile, const std::vector<std::string>& textureFiles, const VmaAllocationCreateInfo& modelAllocInfo)
	{

	    // Load model
	    if (!m_Model.LoadFromObj(modelFile, allocator, Device::GetDevice(), modelAllocInfo))
	    {
	        return false;
	    }

        // Load textures
	    m_Textures.clear();
	    std::vector<VkDescriptorImageInfo> imageInfos;
	    for (auto& tf : textureFiles) 
	    {
	        m_Textures.emplace_back(allocator, cmdPool, queue, tf);
	        const Texture& t = m_Textures.back().Get();
	        if (t.image == VK_NULL_HANDLE) return false;
	        imageInfos.push_back({ 
				    .sampler = t.sampler, 
				    .imageView = t.view, 
				    .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL 
				});
	    }
	
	    // Create descriptor set for these textures
	    m_DescriptorOwned = DescriptorSet(imageInfos);
	    return true;
	}
	
	void Asset::Destroy(VmaAllocator allocator)
	{
	    // Destroy textures
	    for (auto& th : m_Textures)
	    {
	        th.Destroy(allocator);
	    }

	    m_Textures.clear();
	    m_DescriptorOwned.Destroy();			// Destroy descriptor resources
	    m_Model.Destroy(Device::GetDevice());	// Destroy model buffer
	}

}

// -------------------------------------------------------

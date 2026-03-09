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
 * asset.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "model.h"
#include "texture_handle.h"
#include <string>
#include <vector>
#include <SceneryEditorX/renderer/vulkan/descriptor_set.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

    class Asset : public RefCounted
	{
	public:
	    Asset();
        virtual ~Asset() override;
	
	    // Load a model and associated textures. cmdPool and queue are used to
	    // transfer/initialize texture images. Returns true on success.
	    bool Load(VmaAllocator allocator, VkCommandPool cmdPool, VkQueue queue, const std::string& modelFile, const std::vector<std::string>& textureFiles, const VmaAllocationCreateInfo& modelAllocInfo);
	
	    // Destroy resources owned by this asset (textures, descriptors, model buffer)
	    void Destroy(VmaAllocator allocator);
	
	    // Accessors
	    VkBuffer GetModelBuffer() const { return m_Model.GetBuffer(); }
	    VkDeviceSize GetModelVertexSize() const { return m_Model.GetVertexBufferSize(); }
	    VkDeviceSize GetModelIndexSize() const { return m_Model.GetIndexBufferSize(); }
	    uint32_t GetModelIndexCount() const { return m_Model.GetIndexCount(); }
	    VkDescriptorSet GetDescriptorSet() const { return m_DescriptorOwned.GetSet(); }
	    VkDescriptorSetLayout GetDescriptorLayout() const { return m_DescriptorOwned.GetLayout(); }
	
	    // Vertex input helpers (forwarded to Model)
        static VkVertexInputBindingDescription GetVertexBindingDescription() { return Model::BindingDescription(); }
        static std::vector<VkVertexInputAttributeDescription> GetVertexAttributeDescriptions() { return Model::AttributeDescriptions(); }
	
	private:
        Model m_Model = {};
	    Ref<Device> m_Device;
	    DescriptorSet m_DescriptorOwned;
        std::vector<TextureHandle> m_Textures;
	};

}

// -------------------------------------------------------

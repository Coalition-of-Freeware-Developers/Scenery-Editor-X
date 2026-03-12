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
 * model.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "asset.h"
#include "texture_handle.h"
#include <SceneryEditorX/renderer/vulkan/buffer.h>
#include <SceneryEditorX/renderer/vulkan/descriptor_set.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	struct Vertex
	{
		Vec3 pos;
		Vec3 normal;
		Vec2 uv;
	};
	
	// -------------------------------------------------------

	class Model : public Asset
	{
	public:
		Model();
		virtual ~Model() override;
	
		// Load an OBJ and create a single mapped host-visible buffer containing
		// vertices followed by indices. Returns true on success.
		bool LoadFromObj(const std::string& filename, VmaAllocator allocator, const VmaAllocationCreateInfo& allocInfo);

		// Load a model and associated textures. cmdPool and queue are used to
		// transfer/initialize texture images. Returns true on success.
		bool Load(VmaAllocator allocator, VkCommandPool cmdPool, VkQueue queue, const std::string& modelFile, const std::vector<std::string>& textureFiles, const VmaAllocationCreateInfo& modelAllocInfo);
	
		// Destroy resources owned by this asset (textures, descriptors, model buffer)
		void Destroy(VmaAllocator allocator);
	
		// Accessors
		VkDescriptorSet GetDescriptorSet() const { return m_DescriptorOwned.GetSet(); }
		VkDescriptorSetLayout GetDescriptorLayout() const { return m_DescriptorOwned.GetLayout(); }
	
		// Vertex input helpers (forwarded to Model)
		static VkVertexInputBindingDescription GetVertexBindingDescription() { return BindingDescription(); }
		static std::vector<VkVertexInputAttributeDescription> GetVertexAttributeDescriptions() { return AttributeDescriptions(); }
	
		VkBuffer GetBuffer() const { return m_Buffer.Get(); }
		VkDeviceSize GetVertexBufferSize() const { return m_VBufferSize; }
		VkDeviceSize GetIndexBufferSize() const { return m_IBufferSize; }
		uint32_t GetIndexCount() const { return m_IndexCount; }
	
		void Destroy() { m_Buffer.Destroy(); }
	
		// Helpers for vertex input setup
		static VkVertexInputBindingDescription BindingDescription();
		static std::vector<VkVertexInputAttributeDescription> AttributeDescriptions();
	
	private:
		Buffer m_Buffer;
		Ref<Device> m_Device;
		VkDeviceSize m_VBufferSize{ 0 };
		VkDeviceSize m_IBufferSize{ 0 };
		uint32_t m_IndexCount{ 0 };
		std::vector<Vertex> m_Vertices;
		std::vector<uint16_t> m_Indices;
		//Model m_Model = {};
		DescriptorSet m_DescriptorOwned;
		std::vector<TextureHandle> m_Textures;
	};

}

// -------------------------------------------------------

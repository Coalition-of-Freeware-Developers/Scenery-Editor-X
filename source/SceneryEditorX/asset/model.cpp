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
 * model.cpp
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */

// Define implementation BEFORE including the header
#define TINYOBJLOADER_IMPLEMENTATION
#include "model.h"
#include <tiny_obj_loader.h>
#include <SceneryEditorX/renderer/vulkan/device.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	Model::Model()
	{        
		SEDX_CORE_TRACE_TAG("Asset", "Creating Asset instance");
		m_Device = RenderContext::Get()->GetDevice();
	}

	Model::~Model()
	{        
		SEDX_CORE_TRACE_TAG("Asset", "Destroying Asset instance");
		m_Textures.clear(); // TextureHandle destructor will release image/sampler/views if not already destroyed
		m_Device.Reset(); // Release reference to device (if any) before destroying resources
	}


	bool Model::Load(VmaAllocator allocator, VkCommandPool cmdPool, VkQueue queue, const std::string& modelFile, const std::vector<std::string>& textureFiles, const VmaAllocationCreateInfo& modelAllocInfo)
	{

		// Load model
		if (!LoadFromObj(modelFile, allocator, modelAllocInfo))
		{
			return false;
		}

		// Load textures
		m_Textures.clear();
		std::vector<::VkDescriptorImageInfo> imageInfos;
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
		SEDX_CORE_TRACE_TAG("Asset", "Loaded Asset with model and textures");
		return true;
	}
	
	void Model::Destroy(VmaAllocator allocator)
	{
		// Destroy textures
		for (auto& th : m_Textures)
		{
			th.Destroy(allocator);
		}

		m_Textures.clear();
		m_DescriptorOwned.Destroy();	// Destroy descriptor resources
		Destroy();						// Destroy model buffer
		SEDX_CORE_TRACE_TAG("Asset", "Destroyed Asset instance");
	}

	bool Model::LoadFromObj(const std::string& filename, VmaAllocator allocator, const VmaAllocationCreateInfo& allocInfo)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str())) 
		{
			SEDX_CORE_ERROR("tinyobj: {} {}", warn, err);
			return false;
		}
		if (shapes.empty()) 
		{
			SEDX_CORE_ERROR("Model::loadFromOBJ: no shapes in OBJ");
			return false;
		}
	
		m_Vertices.clear();
		m_Indices.clear();
	
		for (auto& index : shapes[0].mesh.indices) 
		{
			Vertex v{
				.pos = { attrib.vertices[index.vertex_index * 3], -attrib.vertices[index.vertex_index * 3 + 1], attrib.vertices[index.vertex_index * 3 + 2] },
				.normal = { attrib.normals[index.normal_index * 3], -attrib.normals[index.normal_index * 3 + 1], attrib.normals[index.normal_index * 3 + 2] },
				.uv = { attrib.texcoords[index.texcoord_index * 2], 1.0f - attrib.texcoords[index.texcoord_index * 2 + 1] }
			};

			m_Vertices.push_back(v);
			m_Indices.push_back(static_cast<uint16_t>(m_Indices.size()));
		}
	
		m_VBufferSize = sizeof(Vertex) * m_Vertices.size();
		m_IBufferSize = sizeof(uint16_t) * m_Indices.size();
		m_IndexCount = static_cast<uint32_t>(m_Indices.size());
	
		// Create combined buffer
		m_Buffer = Buffer(allocator, m_VBufferSize + m_IBufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			allocInfo);
		
		void* ptr = m_Buffer.Map();
		if (!ptr)
		{
			SEDX_CORE_ERROR_TAG("Model", "Failed to map combined model buffer for '{}'", filename);
			return false;
		}
		
		std::memcpy(ptr, m_Vertices.data(), m_VBufferSize);
		std::memcpy(static_cast<char*>(ptr) + m_VBufferSize, m_Indices.data(), m_IBufferSize);
		m_Buffer.Unmap();
	
		return true;
	}
	
	VkVertexInputBindingDescription Model::BindingDescription()
	{
		VkVertexInputBindingDescription binding
		{ 
			.binding = 0, 
			.stride = sizeof(Vertex), 
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX 
		};

		return binding;
	}
	
	std::vector<VkVertexInputAttributeDescription> Model::AttributeDescriptions()
	{
		return std::vector<VkVertexInputAttributeDescription>{
			{ .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, pos) },
			{ .location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, normal) },
			{ .location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, uv) },
		};
	}

}

// -------------------------------------------------------

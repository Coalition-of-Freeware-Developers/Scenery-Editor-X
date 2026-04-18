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
 * gbuffer.h
 * -------------------------------------------------------
 * Created: 27/03/2026
 * -------------------------------------------------------
 */
#include "gbuffer.h"
#include <SceneryEditorX/renderer/vulkan/render_context.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		/**
		 * @struct QuadVertex
		 * @brief Vertex format for the static full-screen quad used in geometry passes. 
		 */
		struct QuadVertex
		{
			xMath::Vec3 position;
			xMath::Vec2 uv;
		};

	}

#pragma region Static Member Definitions

	static Ref<Buffer> s_GeometryQuadVertexBuffer = nullptr;
	static Ref<Buffer> s_GeometryQuadIndexBuffer  = nullptr;

	std::vector<Vertex_PosTexNorTan> GeometryBuffer::m_Vertices;
	std::vector<uint32_t> GeometryBuffer::m_Indices;
	std::unique_ptr<Buffer> GeometryBuffer::m_VertexBuffer;
	std::unique_ptr<Buffer> GeometryBuffer::m_IndexBuffer;
	uint32_t GeometryBuffer::m_VertexCount_Committed = 0;
	uint32_t GeometryBuffer::m_IndexCount_Committed  = 0;
	uint32_t GeometryBuffer::m_Vertex_Capacity        = 0;
	uint32_t GeometryBuffer::m_Index_Capacity         = 0;
	Flag GeometryBuffer::m_Dirty;
	bool GeometryBuffer::m_WasRebuilt;
	std::mutex GeometryBuffer::m_Mutex;
	
#pragma endregion

	void GeometryBuffer::Initialize()
	{
		SEDX_CORE_TRACE_TAG("RendererResources", "Initializing GeometryBuffer/G-Buffer");
		if (s_GeometryQuadVertexBuffer && s_GeometryQuadIndexBuffer)
			return;

		const Ref<Device> device = RenderContext::Get()->GetDevice();
		SEDX_CORE_ASSERT(device.IsValid(), "GeometryBuffer::Initialize requires a valid device");

		constexpr std::array<QuadVertex, 4> quadVertices = {
			QuadVertex{
				.position = {-1.0f, -1.0f, 0.0f}, 
				.uv = {0.0f, 0.0f}},
			QuadVertex{
				.position = { 1.0f, -1.0f, 0.0f}, 
				.uv = {1.0f, 0.0f}},
			QuadVertex{
				.position = { 1.0f,  1.0f, 0.0f}, 
				.uv = {1.0f, 1.0f}},
			QuadVertex{
				.position = {-1.0f,  1.0f, 0.0f}, 
				.uv = {0.0f, 1.0f}},
		};

		constexpr std::array<uint16_t, 6> quadIndices = { 0, 1, 2, 2, 3, 0 };

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

		const VmaAllocator allocator = device->GetMemoryAllocator().GetAllocator();

		s_GeometryQuadVertexBuffer = CreateRef<Buffer>(allocator, sizeof(quadVertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,allocInfo);
		s_GeometryQuadIndexBuffer = CreateRef<Buffer>(allocator, sizeof(quadIndices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, allocInfo);

		SEDX_CORE_ASSERT(s_GeometryQuadVertexBuffer && s_GeometryQuadVertexBuffer->Valid(), "Failed to create static quad vertex buffer");
		SEDX_CORE_ASSERT(s_GeometryQuadIndexBuffer && s_GeometryQuadIndexBuffer->Valid(), "Failed to create static quad index buffer");

		void* vbData = s_GeometryQuadVertexBuffer->Map();
		void* ibData = s_GeometryQuadIndexBuffer->Map();

		SEDX_CORE_ASSERT(vbData != nullptr, "Failed to map static quad vertex buffer");
		SEDX_CORE_ASSERT(ibData != nullptr, "Failed to map static quad index buffer");

		std::memcpy(vbData, quadVertices.data(), sizeof(quadVertices));
		std::memcpy(ibData, quadIndices.data(), sizeof(quadIndices));

		s_GeometryQuadVertexBuffer->Unmap();
		s_GeometryQuadIndexBuffer->Unmap();

		SEDX_CORE_TRACE_TAG("Renderer", "GeometryBuffer initialized (static quad VB/IB)");
	}

	void GeometryBuffer::Shutdown()
	{
		s_GeometryQuadVertexBuffer.Reset();
		s_GeometryQuadIndexBuffer.Reset();
		SEDX_CORE_TRACE_TAG("RendererResources", "Shutting down GeometryBuffer/G-Buffer");
	}

	uint32_t GeometryBuffer::AppendVertices(const Vertex_PosTexNorTan *data, uint32_t count)
	{
		std::scoped_lock lock(m_Mutex);

		uint32_t baseOffset = static_cast<uint32_t>(m_Vertices.size());
		m_Vertices.insert(m_Vertices.end(), data, data + count);
		m_Dirty.SetDirty();
		return baseOffset;
	}

	uint32_t GeometryBuffer::AppendIndices(const uint32_t *data, uint32_t count)
	{
		std::scoped_lock lock(m_Mutex);

		uint32_t baseOffset = static_cast<uint32_t>(m_Indices.size());
		m_Indices.insert(m_Indices.end(), data, data + count);
		m_Dirty.SetDirty();

		return baseOffset;
	}

	void GeometryBuffer::UpdateVertices(const Vertex_PosTexNorTan *data, uint32_t offset, uint32_t count)
	{
		std::scoped_lock lock(m_Mutex);

		SEDX_CORE_ASSERT(offset + count <= static_cast<uint32_t>(m_Vertices.size()), "Vertex update range out of bounds");
		memcpy(m_Vertices.data() + offset, data, count * sizeof(Vertex_PosTexNorTan));

		if (m_VertexBuffer && offset + count <= m_VertexCount_Committed)
		{
			uint64_t byteOffset = static_cast<uint64_t>(offset) * sizeof(Vertex_PosTexNorTan);
			uint64_t byteSize   = static_cast<uint64_t>(count) * sizeof(Vertex_PosTexNorTan);
			m_VertexBuffer->UploadSubRegion(data, byteOffset, byteSize);
		}
	}

	void GeometryBuffer::BuildIfDirty()
	{
		std::scoped_lock lock(m_Mutex);

		if (!m_Dirty.IsDirty() || m_Vertices.empty() || m_Indices.empty())
			return;

		uint32_t vertexCount = static_cast<uint32_t>(m_Vertices.size());
		uint32_t indexCount  = static_cast<uint32_t>(m_Indices.size());

		m_WasRebuilt            = false;
		bool needsFullRebuild = !m_VertexBuffer || !m_IndexBuffer || vertexCount > m_Vertex_Capacity || indexCount > m_Index_Capacity;

		if (needsFullRebuild)
		{
			// destroy existing gpu buffers before creating new ones
			m_VertexBuffer = nullptr;
			m_IndexBuffer  = nullptr;

			// allocate with headroom so late-arriving meshes don't trigger another rebuild
			m_Vertex_Capacity = static_cast<uint32_t>(vertexCount * GROWTH_FACTOR);
			m_Index_Capacity  = static_cast<uint32_t>(indexCount * GROWTH_FACTOR);

			// create vertex buffer with capacity (no initial data - we upload via sub-region)
			m_VertexBuffer = CreateScope<Buffer>(sizeof(Vertex_PosTexNorTan), m_Vertex_Capacity,
				nullptr, // no initial data
				false, "geometry_buffer_vertex");

			// create index buffer with capacity (no initial data)
			m_IndexBuffer = CreateScope<Buffer>(sizeof(uint32_t), m_Index_Capacity,
				nullptr, // no initial data
				false, "geometry_buffer_index");

			// upload all committed data into the newly allocated buffers
			m_VertexBuffer->UploadSubRegion(m_Vertices.data(), 0, vertexCount * sizeof(Vertex_PosTexNorTan));
			m_IndexBuffer->UploadSubRegion(m_Indices.data(), 0, indexCount * sizeof(uint32_t));

			m_VertexCount_Committed = vertexCount;
			m_IndexCount_Committed  = indexCount;
			m_WasRebuilt = true;

			SEDX_CORE_INFO_TAG("GBuffer","Global geometry buffer built: %u vertices (%.2f MB), %u indices (%.2f MB), capacity: %u vertices, %u indices",
				vertexCount, (vertexCount * sizeof(Vertex_PosTexNorTan)) / (1024.0f * 1024.0f),
				indexCount, (indexCount * sizeof(uint32_t)) / (1024.0f * 1024.0f),
				m_Vertex_Capacity, m_Index_Capacity);
		}
		else
		{
			// the new data fits within the pre-allocated capacity, upload only the new portion
			uint32_t newVertices = vertexCount - m_VertexCount_Committed;
			uint32_t newIndices  = indexCount - m_IndexCount_Committed;

			if (newVertices > 0)
			{
				uint64_t offset = static_cast<uint64_t>(m_VertexCount_Committed) * sizeof(Vertex_PosTexNorTan);
				uint64_t size   = static_cast<uint64_t>(newVertices) * sizeof(Vertex_PosTexNorTan);
				m_VertexBuffer->UploadSubRegion(m_Vertices.data() + m_VertexCount_Committed, offset, size);
			}

			if (newIndices > 0)
			{
				uint64_t offset = static_cast<uint64_t>(m_IndexCount_Committed) * sizeof(uint32_t);
				uint64_t size   = static_cast<uint64_t>(newIndices) * sizeof(uint32_t);
				m_IndexBuffer->UploadSubRegion(m_Indices.data() + m_IndexCount_Committed, offset, size);
			}

			m_VertexCount_Committed = vertexCount;
			m_IndexCount_Committed  = indexCount;

			SEDX_CORE_INFO_TAG("GBuffer","Global geometry buffer updated: +%u vertices, +%u indices (sub-region upload, no rebuild)", newVertices, newIndices);
		}

		m_Dirty.Check();
	}

	bool GeometryBuffer::WasRebuilt()
	{
		bool result = m_WasRebuilt;
		m_WasRebuilt = false;
		return result;
	}

	Buffer *GeometryBuffer::GetIndexBuffer() { return m_IndexBuffer.get(); }

	Buffer *GeometryBuffer::GetVertexBuffer() { return m_VertexBuffer.get(); }


}

// -------------------------------------------------------

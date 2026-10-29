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
#include <algorithm>
#include <atomic>
#include <SceneryEditorX/renderer/vulkan/queue_manager.h>
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

		/* @brief Set while the global buffer takes frees: off after Shutdown, when meshes destroyed at exit must not touch it. */
		std::atomic<bool> s_Accepting = true;

		/** @brief Frees a buffer's GPU memory now (~Buffer would too, but logs it as a leak) and drops the owner. */
		template <typename Owner>
		void ReleaseBuffer(Owner &buffer)
		{
			if (buffer)
			{
				buffer->Destroy();
				buffer = nullptr;
			}
		}

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
	uint64_t GeometryBuffer::m_Generation = 0;
	std::mutex GeometryBuffer::m_Mutex;
	std::vector<GeometryBuffer::Range> GeometryBuffer::m_FreeVertexRanges;
	std::vector<GeometryBuffer::Range> GeometryBuffer::m_FreeIndexRanges;
	std::vector<GeometryBuffer::PendingRange> GeometryBuffer::m_PendingRanges;
	std::vector<GeometryBuffer::Range> GeometryBuffer::m_DirtyVertexRanges;
	std::vector<GeometryBuffer::Range> GeometryBuffer::m_DirtyIndexRanges;
	uint64_t GeometryBuffer::m_Frame = 0;
	
#pragma endregion

	void GeometryBuffer::Initialize()
	{
		SEDX_CORE_TRACE_TAG("RendererResources", "Initializing GeometryBuffer/G-Buffer");
		s_Accepting = true;
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
		std::scoped_lock lock(m_Mutex);

		// All of these are statics: release them here, while the allocator and device are alive (the renderer has
		// waited for the GPU). Left to the CRT static-destructor phase, ~Buffer calls into the unloaded driver.
		ReleaseBuffer(s_GeometryQuadVertexBuffer);
		ReleaseBuffer(s_GeometryQuadIndexBuffer);
		ReleaseBuffer(m_VertexBuffer);
		ReleaseBuffer(m_IndexBuffer);

		// Start a re-init from an empty global geometry buffer.
		s_Accepting = false;
		std::vector<Vertex_PosTexNorTan>().swap(m_Vertices);
		std::vector<uint32_t>().swap(m_Indices);
		m_FreeVertexRanges.clear();
		m_FreeIndexRanges.clear();
		m_PendingRanges.clear();
		m_DirtyVertexRanges.clear();
		m_DirtyIndexRanges.clear();
		m_VertexCount_Committed = 0;
		m_IndexCount_Committed  = 0;
		m_Vertex_Capacity       = 0;
		m_Index_Capacity        = 0;
		m_WasRebuilt            = false;

		SEDX_CORE_TRACE_TAG("RendererResources", "Shutting down GeometryBuffer/G-Buffer");
	}

	namespace
	{
		/**
		 * @brief Writes count elements into the smallest freed range that holds them (what is left of it stays free);
		 * returns its offset, or the vector's end when none fits (the elements are appended there).
		 */
		template <typename Element, typename Range>
		uint32_t Place(std::vector<Element> &elements, std::vector<Range> &free, std::vector<Range> &dirty, const uint32_t committed, const Element *data, const uint32_t count)
		{
			auto best = free.end();
			for (auto it = free.begin(); it != free.end(); ++it)
			{
				if (it->count >= count && (best == free.end() || it->count < best->count))
					best = it;
			}
			if (best == free.end() || count == 0)
			{
				const uint32_t offset = static_cast<uint32_t>(elements.size());
				elements.insert(elements.end(), data, data + count);
				return offset;
			}

			const uint32_t offset = best->offset;
			best->offset += count;
			best->count -= count;
			if (best->count == 0)
				free.erase(best);
			std::copy(data, data + count, elements.begin() + offset);
			if (offset < committed)
				dirty.push_back({ offset, std::min(count, committed - offset) });	// Beyond committed, the normal upload covers it
			return offset;
		}

		/* @brief Adds a range to a sorted free list, merging it with its neighbours. */
		template <typename Range>
		void AddFree(std::vector<Range> &free, const Range range)
		{
			auto it = std::lower_bound(free.begin(), free.end(), range.offset, [](const Range &r, const uint32_t offset) { return r.offset < offset; });
			it = free.insert(it, range);
			if (it + 1 != free.end() && it->offset + it->count == (it + 1)->offset)
			{
				it->count += (it + 1)->count;
				free.erase(it + 1);
			}
			if (it != free.begin() && (it - 1)->offset + (it - 1)->count == it->offset)
			{
				(it - 1)->count += it->count;
				free.erase(it);
			}
		}

		/* @brief Drops a free range that reaches the end of the elements: the vector shrinks instead. */
		template <typename Element, typename Range>
		void TrimEnd(std::vector<Element> &elements, std::vector<Range> &free, uint32_t &committed)
		{
			if (free.empty() || free.back().offset + free.back().count != elements.size())
				return;
			elements.resize(free.back().offset);
			free.pop_back();
			committed = std::min(committed, static_cast<uint32_t>(elements.size()));
		}
	}

	uint32_t GeometryBuffer::AppendVertices(const Vertex_PosTexNorTan *data, uint32_t count)
	{
		std::scoped_lock lock(m_Mutex);
		const uint32_t offset = Place(m_Vertices, m_FreeVertexRanges, m_DirtyVertexRanges, m_VertexCount_Committed, data, count);
		m_Dirty.SetDirty();
		return offset;
	}

	uint32_t GeometryBuffer::AppendIndices(const uint32_t *data, uint32_t count)
	{
		std::scoped_lock lock(m_Mutex);
		const uint32_t offset = Place(m_Indices, m_FreeIndexRanges, m_DirtyIndexRanges, m_IndexCount_Committed, data, count);
		m_Dirty.SetDirty();
		return offset;
	}

	void GeometryBuffer::FreeVertices(const uint32_t offset, const uint32_t count)
	{
		if (count == 0 || !s_Accepting.load())
			return;
		std::scoped_lock lock(m_Mutex);
		if (offset + count <= m_Vertices.size())
			m_PendingRanges.push_back({ { offset, count }, m_Frame, false });
	}

	void GeometryBuffer::FreeIndices(const uint32_t offset, const uint32_t count)
	{
		if (count == 0 || !s_Accepting.load())
			return;
		std::scoped_lock lock(m_Mutex);
		if (offset + count <= m_Indices.size())
			m_PendingRanges.push_back({ { offset, count }, m_Frame, true });
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

		// Freed ranges become reusable once the frames that might still draw them are done
		++m_Frame;
		if (!m_PendingRanges.empty())
		{
			std::erase_if(m_PendingRanges, [](const PendingRange &pending)
			{
				if (m_Frame < pending.frame + FREE_DELAY_FRAMES)
					return false;
				AddFree(pending.indices ? m_FreeIndexRanges : m_FreeVertexRanges, pending.range);
				return true;
			});
			TrimEnd(m_Vertices, m_FreeVertexRanges, m_VertexCount_Committed);
			TrimEnd(m_Indices, m_FreeIndexRanges, m_IndexCount_Committed);
		}

		if (!m_Dirty.IsDirty() || m_Vertices.empty() || m_Indices.empty())
			return;

		uint32_t vertexCount = static_cast<uint32_t>(m_Vertices.size());
		uint32_t indexCount  = static_cast<uint32_t>(m_Indices.size());

		m_WasRebuilt            = false;
		bool needsFullRebuild = !m_VertexBuffer || !m_IndexBuffer || vertexCount > m_Vertex_Capacity || indexCount > m_Index_Capacity;

		if (needsFullRebuild)
		{
			// destroy existing gpu buffers before creating new ones. Buffers are freed immediately (no deletion queue)
			// while frames in flight still read them through the bindless geometry descriptors, so drain the GPU first.
			// Capacity grows geometrically, so this only happens when a load outgrows the headroom.
			if (m_VertexBuffer || m_IndexBuffer)
				QueueManager::WaitIdleAll();

			ReleaseBuffer(m_VertexBuffer);
			ReleaseBuffer(m_IndexBuffer);

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
			++m_Generation;
			m_DirtyVertexRanges.clear();
			m_DirtyIndexRanges.clear();


			SEDX_CORE_INFO_TAG("GBuffer","Global geometry buffer built: {} vertices ({:.2f} MB), {} indices ({:.2f} MB), capacity: {} vertices, {} indices",
				vertexCount, (vertexCount * sizeof(Vertex_PosTexNorTan)) / (1024.0f * 1024.0f),
				indexCount, (indexCount * sizeof(uint32_t)) / (1024.0f * 1024.0f),
				m_Vertex_Capacity, m_Index_Capacity);
		}
		else
		{
			// space freed earlier and written again: upload those ranges where they lie
			for (const Range &range : m_DirtyVertexRanges)
			{
				if (range.offset + range.count <= vertexCount)
					m_VertexBuffer->UploadSubRegion(m_Vertices.data() + range.offset, static_cast<uint64_t>(range.offset) * sizeof(Vertex_PosTexNorTan),
						static_cast<uint64_t>(range.count) * sizeof(Vertex_PosTexNorTan));
			}
			for (const Range &range : m_DirtyIndexRanges)
			{
				if (range.offset + range.count <= indexCount)
					m_IndexBuffer->UploadSubRegion(m_Indices.data() + range.offset, static_cast<uint64_t>(range.offset) * sizeof(uint32_t),
						static_cast<uint64_t>(range.count) * sizeof(uint32_t));
			}
			const size_t reused = m_DirtyVertexRanges.size() + m_DirtyIndexRanges.size();
			m_DirtyVertexRanges.clear();
			m_DirtyIndexRanges.clear();

			// the new data fits within the pre-allocated capacity, upload only the new portion
			const uint32_t newVertices = vertexCount > m_VertexCount_Committed ? vertexCount - m_VertexCount_Committed : 0;
			const uint32_t newIndices  = indexCount > m_IndexCount_Committed ? indexCount - m_IndexCount_Committed : 0;

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

			SEDX_CORE_TRACE_TAG("GBuffer","Global geometry buffer updated: +{} vertices, +{} indices, {} reused ranges (sub-region uploads, no rebuild)", newVertices, newIndices, reused);
		}

		m_Dirty.Check();
	}

	bool GeometryBuffer::WasRebuilt()
	{
		bool result = m_WasRebuilt;
		m_WasRebuilt = false;
		return result;
	}

	uint64_t GeometryBuffer::GetGeneration()
	{
		return m_Generation;
	}

	uint32_t GeometryBuffer::GetVertexCount()
	{
		std::scoped_lock lock(m_Mutex);
		return static_cast<uint32_t>(m_Vertices.size());
	}

	Buffer *GeometryBuffer::GetIndexBuffer() { return m_IndexBuffer.get(); }

	Buffer *GeometryBuffer::GetVertexBuffer() { return m_VertexBuffer.get(); }


}

// -------------------------------------------------------

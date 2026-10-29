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
#include <cstring>
#include <SceneryEditorX/renderer/vulkan/buffer.h>
#include <SceneryEditorX/renderer/vulkan/command_list.h>
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

	uint32_t GeometryBuffer::m_VertexCount = 0;
	uint32_t GeometryBuffer::m_IndexCount  = 0;
	std::vector<GeometryBuffer::Upload<Vertex_PosTexNorTan>> GeometryBuffer::m_VertexUploads;
	std::vector<GeometryBuffer::Upload<uint32_t>> GeometryBuffer::m_IndexUploads;
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
		m_VertexCount = 0;
		m_IndexCount  = 0;
		std::vector<Upload<Vertex_PosTexNorTan>>().swap(m_VertexUploads);
		std::vector<Upload<uint32_t>>().swap(m_IndexUploads);
		m_FreeVertexRanges.clear();
		m_FreeIndexRanges.clear();
		m_PendingRanges.clear();
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
		 * @brief Finds room for count elements: the smallest freed range that holds them (what is left of it stays free),
		 * else the end of the buffer (size grows). Returns the offset.
		 */
		template <typename Range>
		uint32_t Place(uint32_t &size, std::vector<Range> &free, const uint32_t count)
		{
			auto best = free.end();
			for (auto it = free.begin(); it != free.end(); ++it)
			{
				if (it->count >= count && (best == free.end() || it->count < best->count))
					best = it;
			}
			if (best == free.end() || count == 0)
			{
				const uint32_t offset = size;
				size += count;
				return offset;
			}

			const uint32_t offset = best->offset;
			best->offset += count;
			best->count -= count;
			if (best->count == 0)
				free.erase(best);
			return offset;
		}

		/* @brief Writes data over the parts of the waiting uploads it covers. */
		template <typename Element, typename Upload>
		void PatchUploads(std::vector<Upload> &uploads, const Element *data, const uint32_t offset, const uint32_t count)
		{
			for (Upload &upload : uploads)
			{
				const uint32_t begin = std::max(offset, upload.offset);
				const uint32_t end = std::min(offset + count, upload.offset + static_cast<uint32_t>(upload.elements.size()));
				if (begin < end)
					std::copy(data + (begin - offset), data + (end - offset), upload.elements.begin() + (begin - upload.offset));
			}
		}

		/* @brief Bytes the waiting uploads take, each placed at a 16-byte boundary of the staging buffer. */
		template <typename Upload>
		VkDeviceSize GetStagingBytes(const std::vector<Upload> &uploads)
		{
			VkDeviceSize total = 0;
			for (const Upload &upload : uploads)
				total += (upload.elements.size() * sizeof(upload.elements[0]) + 15) & ~static_cast<VkDeviceSize>(15);
			return total;
		}

		/**
		 * @brief Puts the waiting uploads that are still inside the buffer (size elements) into the staging memory at
		 * offset and adds their copies; drops them all.
		 */
		template <typename Upload>
		void StageUploads(std::vector<Upload> &uploads, const uint32_t size, uint8_t *staging, VkDeviceSize &offset, std::vector<VkBufferCopy> &copies)
		{
			for (const Upload &upload : uploads)
			{
				// TrimEnd may have dropped the end of the buffer since (the elements were freed)
				const uint32_t count = upload.offset < size ? std::min(static_cast<uint32_t>(upload.elements.size()), size - upload.offset) : 0;
				if (count == 0)
					continue;

				const VkDeviceSize elementBytes = sizeof(upload.elements[0]);
				std::memcpy(staging + offset, upload.elements.data(), count * elementBytes);
				copies.push_back({ offset, upload.offset * elementBytes, count * elementBytes });
				offset += (upload.elements.size() * elementBytes + 15) & ~static_cast<VkDeviceSize>(15);
			}
			std::vector<Upload>().swap(uploads);
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

		/* @brief Drops a free range that reaches the end of the buffer: the buffer's size shrinks instead. */
		template <typename Range>
		void TrimEnd(uint32_t &size, std::vector<Range> &free, uint32_t &committed)
		{
			if (free.empty() || free.back().offset + free.back().count != size)
				return;
			size = free.back().offset;
			free.pop_back();
			committed = std::min(committed, size);
		}
	}

	uint32_t GeometryBuffer::AppendVertices(const Vertex_PosTexNorTan *data, uint32_t count)
	{
		std::scoped_lock lock(m_Mutex);
		const uint32_t offset = Place(m_VertexCount, m_FreeVertexRanges, count);
		if (count > 0)
			m_VertexUploads.push_back({ offset, std::vector<Vertex_PosTexNorTan>(data, data + count) });
		m_Dirty.SetDirty();
		return offset;
	}

	uint32_t GeometryBuffer::AppendIndices(const uint32_t *data, uint32_t count)
	{
		std::scoped_lock lock(m_Mutex);
		const uint32_t offset = Place(m_IndexCount, m_FreeIndexRanges, count);
		if (count > 0)
			m_IndexUploads.push_back({ offset, std::vector<uint32_t>(data, data + count) });
		m_Dirty.SetDirty();
		return offset;
	}

	void GeometryBuffer::FreeVertices(const uint32_t offset, const uint32_t count)
	{
		if (count == 0 || !s_Accepting.load())
			return;
		std::scoped_lock lock(m_Mutex);
		if (offset + count <= m_VertexCount)
			m_PendingRanges.push_back({ { offset, count }, m_Frame, false });
	}

	void GeometryBuffer::FreeIndices(const uint32_t offset, const uint32_t count)
	{
		if (count == 0 || !s_Accepting.load())
			return;
		std::scoped_lock lock(m_Mutex);
		if (offset + count <= m_IndexCount)
			m_PendingRanges.push_back({ { offset, count }, m_Frame, true });
	}

	void GeometryBuffer::UpdateVertices(const Vertex_PosTexNorTan *data, uint32_t offset, uint32_t count)
	{
		std::scoped_lock lock(m_Mutex);

		SEDX_CORE_ASSERT(offset + count <= m_VertexCount, "Vertex update range out of bounds");

		// what waits for its upload is patched there; what the GPU already holds is written now
		PatchUploads(m_VertexUploads, data, offset, count);
		if (m_VertexBuffer && offset < m_VertexCount_Committed)
		{
			const uint32_t uploaded = std::min(count, m_VertexCount_Committed - offset);
			uint64_t byteOffset = static_cast<uint64_t>(offset) * sizeof(Vertex_PosTexNorTan);
			uint64_t byteSize   = static_cast<uint64_t>(uploaded) * sizeof(Vertex_PosTexNorTan);
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
			TrimEnd(m_VertexCount, m_FreeVertexRanges, m_VertexCount_Committed);
			TrimEnd(m_IndexCount, m_FreeIndexRanges, m_IndexCount_Committed);
		}

		if (!m_Dirty.IsDirty() || m_VertexCount == 0 || m_IndexCount == 0)
			return;

		const uint32_t vertexCount = m_VertexCount;
		const uint32_t indexCount  = m_IndexCount;

		m_WasRebuilt            = false;
		bool needsFullRebuild = !m_VertexBuffer || !m_IndexBuffer || vertexCount > m_Vertex_Capacity || indexCount > m_Index_Capacity;

		// a rebuild keeps the old buffers until the new ones hold what they had: the GPU copies it over
		Scope<Buffer> oldVertexBuffer;
		Scope<Buffer> oldIndexBuffer;
		if (needsFullRebuild)
		{
			// Buffers are freed immediately (no deletion queue) while frames in flight still read them through the
			// bindless geometry descriptors, so drain the GPU first. Capacity grows geometrically, so this only happens
			// when a load outgrows the headroom.
			if (m_VertexBuffer || m_IndexBuffer)
				QueueManager::WaitIdleAll();

			oldVertexBuffer = std::move(m_VertexBuffer);
			oldIndexBuffer  = std::move(m_IndexBuffer);

			// allocate with headroom so late-arriving meshes don't trigger another rebuild
			m_Vertex_Capacity = static_cast<uint32_t>(vertexCount * GROWTH_FACTOR);
			m_Index_Capacity  = static_cast<uint32_t>(indexCount * GROWTH_FACTOR);

			m_VertexBuffer = CreateScope<Buffer>(sizeof(Vertex_PosTexNorTan), m_Vertex_Capacity, nullptr, false, "geometry_buffer_vertex");
			m_IndexBuffer  = CreateScope<Buffer>(sizeof(uint32_t), m_Index_Capacity, nullptr, false, "geometry_buffer_index");
		}

		// what the old buffers held and what was put in since the last call go over in one submission
		const uint32_t keptVertices = oldVertexBuffer ? std::min(m_VertexCount_Committed, vertexCount) : 0;
		const uint32_t keptIndices  = oldIndexBuffer ? std::min(m_IndexCount_Committed, indexCount) : 0;
		const size_t uploads = m_VertexUploads.size() + m_IndexUploads.size();
		const VkDeviceSize stagingBytes = GetStagingBytes(m_VertexUploads) + GetStagingBytes(m_IndexUploads);
		std::vector<VkBufferCopy> vertexCopies;
		std::vector<VkBufferCopy> indexCopies;

		Buffer *staging = stagingBytes > 0 ? static_cast<Buffer *>(Buffer::AcquireStagingBuffer(stagingBytes)) : nullptr;
		if (auto *mapped = staging ? static_cast<uint8_t *>(staging->Map()) : nullptr)
		{
			VkDeviceSize stagingOffset = 0;
			StageUploads(m_VertexUploads, vertexCount, mapped, stagingOffset, vertexCopies);
			StageUploads(m_IndexUploads, indexCount, mapped, stagingOffset, indexCopies);
			staging->Unmap();
		}
		else if (stagingBytes > 0)
		{
			SEDX_CORE_ERROR_TAG("GBuffer", "Failed to get a {} byte staging buffer: {} geometry uploads are lost", stagingBytes, uploads);
			std::vector<Upload<Vertex_PosTexNorTan>>().swap(m_VertexUploads);
			std::vector<Upload<uint32_t>>().swap(m_IndexUploads);
		}

		if (keptVertices > 0 || keptIndices > 0 || !vertexCopies.empty() || !indexCopies.empty())
		{
			// use graphics queue to avoid a cross-queue sync hazard with draw commands that read the same buffers
			if (CommandList *cmdList = CommandList::BeginImmediateExecution(QueueType::Graphics))
			{
				const VkCommandBuffer cmd = cmdList->GetCommandBuffer();
				if (keptVertices > 0)
				{
					const VkBufferCopy copy{ 0, 0, static_cast<VkDeviceSize>(keptVertices) * sizeof(Vertex_PosTexNorTan) };
					vkCmdCopyBuffer(cmd, oldVertexBuffer->Get(), m_VertexBuffer->Get(), 1, &copy);
				}
				if (keptIndices > 0)
				{
					const VkBufferCopy copy{ 0, 0, static_cast<VkDeviceSize>(keptIndices) * sizeof(uint32_t) };
					vkCmdCopyBuffer(cmd, oldIndexBuffer->Get(), m_IndexBuffer->Get(), 1, &copy);
				}
				if (keptVertices > 0 || keptIndices > 0)
				{
					// space freed earlier and written again lies inside what was copied: the new content lands after it
					VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
					barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
				}
				if (!vertexCopies.empty())
					vkCmdCopyBuffer(cmd, staging->Get(), m_VertexBuffer->Get(), static_cast<uint32_t>(vertexCopies.size()), vertexCopies.data());
				if (!indexCopies.empty())
					vkCmdCopyBuffer(cmd, staging->Get(), m_IndexBuffer->Get(), static_cast<uint32_t>(indexCopies.size()), indexCopies.data());
				CommandList::EndImmediateExecution(cmdList);
			}
			else
			{
				SEDX_CORE_ERROR_TAG("GBuffer", "No immediate command list available to upload the global geometry buffer");
			}
		}

		if (staging)
			Buffer::ReleaseStagingBuffer(staging);
		ReleaseBuffer(oldVertexBuffer);
		ReleaseBuffer(oldIndexBuffer);

		m_VertexCount_Committed = vertexCount;
		m_IndexCount_Committed  = indexCount;

		if (needsFullRebuild)
		{
			m_WasRebuilt = true;
			++m_Generation;

			SEDX_CORE_INFO_TAG("GBuffer","Global geometry buffer built: {} vertices ({:.2f} MB), {} indices ({:.2f} MB), capacity: {} vertices, {} indices",
				vertexCount, (vertexCount * sizeof(Vertex_PosTexNorTan)) / (1024.0f * 1024.0f),
				indexCount, (indexCount * sizeof(uint32_t)) / (1024.0f * 1024.0f),
				m_Vertex_Capacity, m_Index_Capacity);
		}
		else
		{
			SEDX_CORE_TRACE_TAG("GBuffer","Global geometry buffer updated: {} uploads ({:.2f} MB, no rebuild)", uploads, stagingBytes / (1024.0f * 1024.0f));
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
		return m_VertexCount;
	}

	Buffer *GeometryBuffer::GetIndexBuffer() { return m_IndexBuffer.get(); }

	Buffer *GeometryBuffer::GetVertexBuffer() { return m_VertexBuffer.get(); }


}

// -------------------------------------------------------

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
 * Created: 16/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include "vulkan/vertex.h"
#include <SceneryEditorX/core/identifiers/flag.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Buffer;

	/**
	 * @struct GeometryBuffer
	 * @brief Stub geometry buffer providing bindless index/vertex buffer access.
	 */
	 class GeometryBuffer
	 {
	 public:
		//static Buffer* GetIndexBuffer()  { return nullptr; }
		//static Buffer* GetVertexBuffer() { return nullptr; }
		/** @brief Create static geometry resources used by built-in passes (quad VB/IB). */
		static void Initialize();

		/** @brief Release static geometry resources created by Initialize(). */
		static void Shutdown();

		/**
		 * @brief Puts vertices in the global buffer: in space freed earlier when a range fits, else at the end.
		 * @param data Pointer to the vertex data to append.
		 * @param count Number of vertices to append.
		 * @return The base vertex offset in the global buffer.
		 */
		static uint32_t AppendVertices(const Vertex_PosTexNorTan *data, uint32_t count);

		/**
		 * @brief Puts indices in the global buffer, like AppendVertices.
		 * @param data Pointer to the index data to append.
		 * @param count Number of indices to append.
		 * @return The base index offset in the global buffer.
		 */
		static uint32_t AppendIndices(const uint32_t *data, uint32_t count);

		/**
		 * @brief Gives a mesh's ranges back (the mesh is gone or rebuilt). They are reused only some frames later, once
		 * the frames in flight that might still draw them have finished, so editing never overwrites what the GPU reads.
		 */
		static void FreeVertices(uint32_t offset, uint32_t count);
		static void FreeIndices(uint32_t offset, uint32_t count);

		/**
		 * @brief Updates existing vertices in-place (on the GPU, and in the upload still waiting), used by deformable
		 * meshes like cloth
		 * @param data Pointer to the vertex data to update.
		 * @param offset Offset in the global buffer where the update should start.
		 * @param count Number of vertices to update.
		 */
		static void UpdateVertices(const Vertex_PosTexNorTan *data, uint32_t offset, uint32_t count);

		/**
		 * @brief Uploads what was put in the buffers since the last call, in one copy. The GPU buffers are the only copy
		 * of the geometry: appends wait here only until this uploads them.
		 * @note If no gpu buffer exists, create one with headroom
		 * @note If capacity is exceeded, recreate with headroom and copy the old buffers into the new ones on the GPU
		 */
		static void BuildIfDirty();

		/**
		 * @brief callers should use this to invalidate caches that depend on buffer addresses (e.g. acceleration structures).
		 * @return true if a full buffer rebuild occurred this frame (capacity exceeded).
		 * @note The flag is cleared after being read.
		 */
		static bool WasRebuilt();

		/**
		 * @brief Bumped whenever the GPU buffers are (re)allocated: descriptors pointing at them must be written again.
		 * Unlike WasRebuilt() it isn't consumed by reading, and unlike the buffer pointers it can't repeat.
		 */
		static uint64_t GetGeneration();

		/** @brief Vertices the global buffer spans, live and freed alike: what a full rebuild copies. */
		static uint32_t GetVertexCount();


		/** @brief Get the static quad index buffer. */
		static Buffer *GetIndexBuffer();

		/** @brief Get the static quad vertex buffer. */
		static Buffer *GetVertexBuffer();

	 private:
		// what the buffers span (element counts, live and freed alike) and what was put there since the last upload: no
		// CPU copy of the whole buffers is kept (KDEN's ~2 GB of vertices lived on the CPU and the GPU before)
		template <typename Element>
		struct Upload
		{
			uint32_t offset = 0;
			std::vector<Element> elements;
		};
		static uint32_t m_VertexCount;
		static uint32_t m_IndexCount;
		static std::vector<Upload<Vertex_PosTexNorTan>> m_VertexUploads;
		static std::vector<Upload<uint32_t>> m_IndexUploads;

		// gpu buffers
		static Scope<Buffer> m_VertexBuffer;
		static Scope<Buffer> m_IndexBuffer;

		// capacity tracking (element counts)
		static uint32_t m_VertexCount_Committed; // elements the gpu buffers hold, uploaded or copied over
		static uint32_t m_IndexCount_Committed;
		static uint32_t m_Vertex_Capacity;        // total gpu buffer capacity
		static uint32_t m_Index_Capacity;

		// state
		static Flag m_Dirty;
		static bool m_WasRebuilt;
		static uint64_t m_Generation;
		static std::mutex m_Mutex;

		// freed space: ranges waiting out the frames in flight, then reusable (sorted, merged)
		struct Range
		{
			uint32_t offset = 0;
			uint32_t count = 0;
		};
		struct PendingRange
		{
			Range range;
			uint64_t frame = 0;		// m_Frame when it was freed
			bool indices = false;
		};
		static std::vector<Range> m_FreeVertexRanges;
		static std::vector<Range> m_FreeIndexRanges;
		static std::vector<PendingRange> m_PendingRanges;
		static uint64_t m_Frame;	// BuildIfDirty calls (once a frame)

		// growth factor applied when allocating gpu buffers
		static constexpr float GROWTH_FACTOR = 1.25f;

		// frames a freed range waits before it is reused (more than the frames in flight)
		static constexpr uint64_t FREE_DELAY_FRAMES = 4;
	 };

}

// -------------------------------------------------------

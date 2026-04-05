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
		 * @brief Appends vertices to the global buffer, returns the base vertex offset.
		 * @param data Pointer to the vertex data to append.
		 * @param count Number of vertices to append.
		 * @return The base vertex offset in the global buffer.
		 */
		static uint32_t AppendVertices(const Vertex_PosTexNorTan *data, uint32_t count);

		/**
		 * @brief Appends indices to the global buffer, returns the base index offset.
		 * @param data Pointer to the index data to append.
		 * @param count Number of indices to append.
		 * @return The base index offset in the global buffer.
		 */
		static uint32_t AppendIndices(const uint32_t *data, uint32_t count);

		/**
		 * @brief Updates existing vertices in-place (cpu + gpu), used by deformable meshes like cloth
		 * @param data Pointer to the vertex data to update.
		 * @param offset Offset in the global buffer where the update should start.
		 * @param count Number of vertices to update.
		 */
		static void UpdateVertices(const Vertex_PosTexNorTan *data, uint32_t offset, uint32_t count);

		/**
		 * @brief Synchronizes GPU buffers with CPU data if they are marked as dirty.
		 * @note If no gpu buffer exists, create one with headroom and upload everything
		 * @note If new data fits within existing capacity, upload only the new portion
		 * @note If capacity is exceeded, recreate with headroom
		 */
		static void BuildIfDirty();

		/**
		 * @brief callers should use this to invalidate caches that depend on buffer addresses (e.g. acceleration structures).
		 * @return true if a full buffer rebuild occurred this frame (capacity exceeded).
		 * @note The flag is cleared after being read.
		 */
		static bool WasRebuilt();

		/** @brief Get the static quad index buffer. */
		static Buffer *GetIndexBuffer();

		/** @brief Get the static quad vertex buffer. */
		static Buffer *GetVertexBuffer();

	 private:
		// cpu-side accumulators
		static std::vector<Vertex_PosTexNorTan> m_Vertices;
		static std::vector<uint32_t> m_Indices;

		// gpu buffers
		static Scope<Buffer> m_VertexBuffer;
		static Scope<Buffer> m_IndexBuffer;

		// capacity tracking (element counts)
		static uint32_t m_VertexCount_Committed; // elements uploaded to gpu
		static uint32_t m_IndexCount_Committed;
		static uint32_t m_Vertex_Capacity;        // total gpu buffer capacity
		static uint32_t m_Index_Capacity;

		// state
		static Flag m_Dirty;
		static bool m_WasRebuilt;
		static std::mutex m_Mutex;

		// growth factor applied when allocating gpu buffers
		static constexpr float GROWTH_FACTOR = 1.25f;
	 };

}

// -------------------------------------------------------

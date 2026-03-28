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

	static Ref<Buffer> s_GeometryQuadVertexBuffer = nullptr;
	static Ref<Buffer> s_GeometryQuadIndexBuffer  = nullptr;
	
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

	Buffer* GeometryBuffer::GetIndexBuffer() { return s_GeometryQuadIndexBuffer.Get(); }

	Buffer* GeometryBuffer::GetVertexBuffer() { return s_GeometryQuadVertexBuffer.Get(); }


}

// -------------------------------------------------------

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
#include "vulkan/buffer.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @struct GeometryBuffer
	 * @brief Stub geometry buffer providing bindless index/vertex buffer access.
	 */
	 struct GeometryBuffer
	 {
		//static Buffer* GetIndexBuffer()  { return nullptr; }
		//static Buffer* GetVertexBuffer() { return nullptr; }
		/** @brief Create static geometry resources used by built-in passes (quad VB/IB). */
		static void Initialize();

		/** @brief Release static geometry resources created by Initialize(). */
		static void Shutdown();

		/** @brief Get the static quad index buffer. */
		static Buffer* GetIndexBuffer();

		/** @brief Get the static quad vertex buffer. */
		static Buffer* GetVertexBuffer();
	 };

}

// -------------------------------------------------------

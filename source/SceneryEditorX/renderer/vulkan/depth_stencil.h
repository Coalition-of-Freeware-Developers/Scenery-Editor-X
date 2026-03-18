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
 * depth_stencil.h
 * -------------------------------------------------------
 * Created: 16/03/2026
 * -------------------------------------------------------
 */
#pragma once

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @class DepthStencilState
	 * @brief Immutable depth/stencil configuration returned by Renderer::GetDepthStencilState().
	 */
	class DepthStencilState
	{
	public:
		explicit DepthStencilState(bool depthTestEnable, bool depthWriteEnable, VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS)
			: m_DepthTestEnable(depthTestEnable)
			, m_DepthWriteEnable(depthWriteEnable)
			, m_DepthCompareOp(depthCompareOp)
		{}

		bool          IsDepthTestEnabled()  const { return m_DepthTestEnable;  }
		bool          IsDepthWriteEnabled() const { return m_DepthWriteEnable; }
		VkCompareOp   GetDepthCompareOp()   const { return m_DepthCompareOp;   }

	private:
		bool        m_DepthTestEnable  = true;
		bool        m_DepthWriteEnable = true;
		VkCompareOp m_DepthCompareOp   = VK_COMPARE_OP_LESS;
	};

}

// -------------------------------------------------------

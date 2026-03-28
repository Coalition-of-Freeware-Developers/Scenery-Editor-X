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
	 * @struct DepthStencilSpec
	 * @brief 
	 */
	struct DepthStencilSpec
	{
		bool depth_Test					 = true;
		bool depth_Write				 = true;
		VkCompareOp depth_CompFunc		 = VK_COMPARE_OP_LESS_OR_EQUAL;
		bool stencil_Test				 = false;
		bool stencil_Write				 = false;
		VkCompareOp stencil_CompFunc	 = VK_COMPARE_OP_NEVER;
		VkStencilOp stencil_FailOp		 = VK_STENCIL_OP_ZERO;
		VkStencilOp stencil_DepthFail_Op = VK_STENCIL_OP_ZERO;
		VkStencilOp stencil_PassOp		 = VK_STENCIL_OP_ZERO;
	};

	/**
	 * @class DepthStencilState
	 * @brief Immutable depth/stencil configuration returned by Renderer::GetDepthStencilState().
	 */
	class DepthStencilState : public RefCounted
	{
	public:
		DepthStencilState() = default;
		DepthStencilState(const DepthStencilSpec &spec);
		~DepthStencilState() = default;

		bool          IsDepthTestEnabled()  const { return m_DepthTestEnable;  }
		bool          IsDepthWriteEnabled() const { return m_DepthWriteEnable; }
		VkCompareOp   GetDepthCompareOp()   const { return m_DepthCompareOp;   }
		[[nodiscard]] uint64_t GetHash()    const { return m_Hash; }

	private:
		DepthStencilSpec depthSpec;

		bool        m_DepthTestEnable			= false;
		bool        m_DepthWriteEnable			= false;
		VkCompareOp m_DepthCompareOp			= VK_COMPARE_OP_LESS;
		bool		m_StencilTestEnabled        = false;
		bool		m_StencilWriteEnabled       = false;
		VkCompareOp m_StencilCompFunc			= VK_COMPARE_OP_NEVER;
		VkStencilOp m_StencilFailOp             = VK_STENCIL_OP_KEEP;
		VkStencilOp m_StencilDepthFailOp        = VK_STENCIL_OP_KEEP;
		VkStencilOp m_StencilPassOp             = VK_STENCIL_OP_REPLACE;
		uint8_t		m_StencilReadMask			= 1;
		uint8_t		m_StencilWriteMask			= 1;

		uint64_t	m_Hash						= 0;
	};

}

// -------------------------------------------------------

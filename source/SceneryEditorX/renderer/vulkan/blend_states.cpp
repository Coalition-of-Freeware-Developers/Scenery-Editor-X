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
 * blend_states.cpp
 * -------------------------------------------------------
 * Created: 28/03/2026
 * -------------------------------------------------------
 */
#include "blend_states.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	
	static uint64_t HashCombine(uint64_t a, uint64_t b)
	{
		return a * 31 + b;
	}

	BlendState::BlendState(const BlendStateSpec &spec)
	{
		// save
		m_BlendEnabled      = spec.blendEnabled;
		m_SrcColor			= spec.srcBlend;
		m_DstColor			= spec.dstBlend;
		m_ColorOp           = spec.blendOp;
		m_SrcAlpha			= spec.srcAlpha;
		m_DstAlpha			= spec.dstAlpha;
		m_AlphaOp			= spec.alphaOp;
		m_BlendFactor       = spec.blendFactor;

		// hash
		m_Hash = HashCombine(m_Hash, m_BlendEnabled);
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(m_SrcColor));
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(m_DstColor));
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(m_ColorOp));
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(m_SrcAlpha));
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(m_DstAlpha));
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(m_AlphaOp));
		m_Hash = HashCombine(m_Hash, m_BlendFactor);
	}

}

// -------------------------------------------------------

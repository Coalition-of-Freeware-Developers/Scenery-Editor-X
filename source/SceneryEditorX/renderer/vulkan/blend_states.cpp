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

	BlendState::BlendState(const BlendStateSpec &spec) : m_Spec(spec)
	{
		// hash
		m_Hash = HashCombine(m_Hash, m_Spec.blendEnabled);
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(m_Spec.srcBlend));
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(m_Spec.dstBlend));
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(m_Spec.blendOp));
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(m_Spec.srcAlpha));
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(m_Spec.dstAlpha));
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(m_Spec.alphaOp));
		m_Hash = HashCombine(m_Hash, m_Spec.blendFactor);
	}

	BlendState &BlendState::operator=(const Ref<BlendState> &ref)
	{
		if (ref)
		{
			m_Spec = ref->m_Spec;
			m_Hash = ref->m_Hash;
		}
		else
		{
			m_Spec = {};
			m_Hash = 0;
		}
		return *this;
	}

} // namespace SceneryEditorX

// -------------------------------------------------------

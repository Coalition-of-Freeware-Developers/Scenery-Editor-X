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
 * rasterizer.cpp
 * -------------------------------------------------------
 * Created: 28/03/2026
 * -------------------------------------------------------
 */
#include "rasterizer.h"
#include <functional>

// -------------------------------------------------------

namespace SceneryEditorX
{

	static uint64_t HashCombine(const uint64_t a, const uint64_t b)
	{
		return a * 31 + b;
	}

	RasterizerState::RasterizerState(const RasterStateSpec &spec) : spec(spec)
	{

		// hash
		std::hash<float> hasher;
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(m_Spec.polygonMode));
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(m_Spec.depthClipEnabled));
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(m_Spec.lineWidth));
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(hasher(m_Spec.depthBiasEnabled)));
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(hasher(m_Spec.depthBiasClamp)));
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(hasher(m_Spec.depthBiasSlope)));
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(hasher(m_Spec.lineWidth)));
	}

	RasterizerState::~RasterizerState()
	{
		m_Spec = {};
		m_Hash = 0;
	}

	RasterizerState &RasterizerState::operator=(const Ref<RasterizerState> &ref)
	{
		if (ref.Get() == this)
			return *this;

		if (ref)
		{
			m_Spec = ref->m_Spec;
			m_Hash = ref->m_Hash;
		}
		else
		{
			// Reset to default values if the reference is null
			m_Spec = RasterStateSpec();
			m_Hash = 0; // or some default hash value
		}

		return *this;
	}

} // namespace SceneryEditorX

// -------------------------------------------------------

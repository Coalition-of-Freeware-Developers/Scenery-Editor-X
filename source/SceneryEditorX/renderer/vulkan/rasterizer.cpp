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

	RasterizerState::RasterizerState(const RasterStateSpec &spec)
	{
		// save
		m_PolygonMode           = spec.polygonMode;
		m_DepthClipEnabled      = spec.depthClipEnabled;
		m_DepthBiasEnabled      = spec.depthBiasEnabled;
		m_DepthBiasClamp        = spec.depthBiasClamp;
		m_DepthBiasSlope		= spec.depthBiasSlope;
		m_LineWidth             = spec.lineWidth;

		// hash
		std::hash<float> hasher;
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(m_PolygonMode));
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(m_DepthClipEnabled));
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(m_LineWidth));
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(hasher(m_DepthBiasEnabled)));
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(hasher(m_DepthBiasClamp)));
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(hasher(m_DepthBiasSlope)));
		m_Hash = HashCombine(m_Hash, static_cast<uint64_t>(hasher(m_LineWidth)));
	}

	RasterizerState &RasterizerState::operator=(const Ref<RasterizerState> &ref)
	{
		if (ref.Get() == this)
			return *this;
		if (ref)
		{
			m_Spec = ref->m_Spec;
			m_PolygonMode = ref->m_PolygonMode;
			m_DepthClipEnabled = ref->m_DepthClipEnabled;
			m_DepthBiasEnabled = ref->m_DepthBiasEnabled;
			m_DepthBiasClamp = ref->m_DepthBiasClamp;
			m_DepthBiasSlope = ref->m_DepthBiasSlope;
			m_LineWidth = ref->m_LineWidth;
			m_Hash = ref->m_Hash;
		}
		else
		{
			// Reset to default values if the reference is null
			m_Spec = RasterStateSpec();
			m_PolygonMode = PolygonMode::Solid;
			m_DepthClipEnabled = true;
			m_DepthBiasEnabled = false;
			m_DepthBiasClamp = 0.0f;
			m_DepthBiasSlope = 0.0f;
			m_LineWidth = 1.0f;
			m_Hash = 0; // or some default hash value
		}
		return *this;
	}

} // namespace SceneryEditorX

// -------------------------------------------------------

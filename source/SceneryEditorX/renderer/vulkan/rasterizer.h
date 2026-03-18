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
 * rasterizer.h
 * -------------------------------------------------------
 * Created: 16/03/2026
 * -------------------------------------------------------
 */
#pragma once

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @enum PolygonMode
	 * @brief Vulkan polygon fill mode, exposed on RasterizerState objects.
	 */
	enum class PolygonMode : uint8_t
	{
		Solid,
		Wireframe,
		Point,
		MaxEnum
	};

	/**
	 * @class RasterizerState
	 * @brief Immutable rasterizer configuration object returned by Renderer::GetRasterizerState().
	 */
	class RasterizerState
	{
	public:
		explicit RasterizerState(PolygonMode polygonMode, bool depthBiasEnabled = false,
								 float depthBiasConstant = 0.0f, float depthBiasSlope = 0.0f)
			: m_PolygonMode(polygonMode)
			, m_DepthBiasEnabled(depthBiasEnabled)
			, m_DepthBiasConstant(depthBiasConstant)
			, m_DepthBiasSlope(depthBiasSlope)
		{
		}

		[[nodiscard]] PolygonMode GetPolygonMode()       const { return m_PolygonMode; }
		[[nodiscard]] bool        IsDepthBiasEnabled()   const { return m_DepthBiasEnabled; }
		[[nodiscard]] float       GetDepthBiasConstant() const { return m_DepthBiasConstant; }
		[[nodiscard]] float       GetDepthBiasSlope()    const { return m_DepthBiasSlope; }

	private:
		PolygonMode m_PolygonMode       = PolygonMode::Solid;
		bool        m_DepthBiasEnabled  = false;
		float       m_DepthBiasConstant = 0.0f;
		float       m_DepthBiasSlope    = 0.0f;
	};

}

// -------------------------------------------------------

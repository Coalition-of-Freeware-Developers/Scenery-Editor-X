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
	 * @struct RasterStateSpec
	 * @brief Descriptor struct for creating a RasterizerState object.
	 * This is used to specify the desired rasterization state when requesting a RasterizerState from the Renderer. 
	 * It contains fields for polygon mode, depth bias settings, and line width, which are used to configure how primitives are rasterized. 
	 */
	struct RasterStateSpec
	{
		PolygonMode polygonMode       = PolygonMode::MaxEnum;
		bool        depthBiasEnabled  = false;
		bool		depthClipEnabled  = true;
		float       depthBiasConstant = 0.0f;
		float       depthBiasSlope    = 0.0f;
		float		depthBiasClamp    = 0.0f;
		float		lineWidth         = 1.0f;
	};

	/**
	 * @class RasterizerState
	 * @brief Immutable rasterizer configuration object returned by Renderer::GetRasterizerState().
	 */
	class RasterizerState : public RefCounted
	{
	public:
		RasterizerState() = default;
		RasterizerState(const RasterStateSpec &spec);
		~RasterizerState() = default;

		[[nodiscard]] PolygonMode GetPolygonMode()       const { return m_PolygonMode; }
		[[nodiscard]] bool        IsDepthBiasEnabled()   const { return m_DepthBiasEnabled; }
		[[nodiscard]] bool		  IsDepthClipEnabled()   const { return m_DepthClipEnabled; }
		[[nodiscard]] float       GetDepthBiasConstant() const { return m_DepthBiasConstant; }
		[[nodiscard]] float       GetDepthBiasSlope()    const { return m_DepthBiasSlope; }
		[[nodiscard]] float		  GetDepthBiasClamp()    const { return m_DepthBiasClamp; }
		[[nodiscard]] float		  GetLineWidth()         const { return m_LineWidth; }
		[[nodiscard]] uint64_t	  GetHash()				 const { return m_Hash; }

		bool operator==(const RasterizerState& state)	 const { return m_Hash == state.GetHash(); }

	private:
		RasterStateSpec m_Spec;

		PolygonMode m_PolygonMode       = PolygonMode::Solid;
		bool        m_DepthBiasEnabled  = false;
		bool		m_DepthClipEnabled	= true;
		float       m_DepthBiasConstant = 0.0f;
		float       m_DepthBiasSlope    = 0.0f;
		float		m_DepthBiasClamp    = 0.0f;
		float		m_LineWidth         = 1.0f;
		uint64_t	m_Hash				= 0;
	};

}

// -------------------------------------------------------

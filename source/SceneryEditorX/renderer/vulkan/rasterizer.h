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
#include "enums.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @struct RasterStateSpec
	 * @brief Descriptor struct for creating a RasterizerState object.
	 * This is used to specify the desired rasterization state when requesting a RasterizerState from the Renderer. 
	 * It contains fields for polygon mode, depth bias settings, and line width, which are used to configure how primitives are rasterized. 
	 */
	struct RasterStateSpec
	{
		PolygonMode polygonMode       = PolygonMode::MaxEnum;
		bool		depthClipEnabled  = true;
		bool        depthBiasEnabled  = false;
		float       depthBiasConstant = 0.0f;
		float       depthBiasSlope    = 0.0f;
		float		depthBiasClamp    = 0.0f;
		float		lineWidth         = 1.0f;
	};

	// TODO: Tie in with pipeline state and hashing system, to avoid redundant state objects and Vulkan pipeline creations.

	/**
	 * @class RasterizerState
	 * @brief Immutable rasterizer configuration object returned by Renderer::GetRasterizerState().
	 */
	class RasterizerState : public RefCounted
	{
	public:
		RasterizerState() = default;
		RasterizerState(const RasterStateSpec &spec);
		virtual ~RasterizerState() override;

		[[nodiscard]] PolygonMode GetPolygonMode()       const { return m_Spec.polygonMode; }
		[[nodiscard]] bool        IsDepthBiasEnabled()   const { return m_Spec.depthBiasEnabled; }
		[[nodiscard]] bool		  IsDepthClipEnabled()   const { return m_Spec.depthClipEnabled; }
		[[nodiscard]] float       GetDepthBias()		 const { return m_Spec.depthBiasConstant; }
		[[nodiscard]] float       GetDepthBiasSlope()    const { return m_Spec.depthBiasSlope; }
		[[nodiscard]] float		  GetDepthBiasClamp()    const { return m_Spec.depthBiasClamp; }
		[[nodiscard]] float		  GetLineWidth()         const { return m_Spec.lineWidth; }
		[[nodiscard]] uint64_t	  GetHash()				 const { return m_Hash; }

		/**
		 * @brief Equality operator for RasterizerState. Compares the hash values of two RasterizerState instances to determine if they are equal.
		 * @param state The RasterizerState instance to compare with.
		 * @return True if the hash values are equal, false otherwise.
		 */
		bool operator==(const RasterizerState& state)	 const { return m_Hash == state.GetHash(); }

		/**
		 * @brief Assignment operator for RasterizerState. Allows assigning from a Ref<RasterizerState> to another RasterizerState instance.
		 * @param ref The reference to the RasterizerState to assign from.
		 * @return A reference to the assigned RasterizerState.
		 */
		RasterizerState &operator=(const Ref<RasterizerState> & ref);

	private:
		RasterStateSpec m_Spec;
		uint64_t m_Hash = 0;
		const RasterStateSpec &spec;
	};

}

// -------------------------------------------------------

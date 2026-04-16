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
 * blend_states.h
 * -------------------------------------------------------
 * Created: 16/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/utils/inheritance.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @struct BlendStateSpec
	 * @brief Specifies the blend state configuration for a render target.
	 */
	struct BlendStateSpec
	{
		bool			blendEnabled  = false;
		VkBlendFactor	srcBlend	  = VK_BLEND_FACTOR_SRC_ALPHA;
		VkBlendFactor	dstBlend	  = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		VkBlendOp		blendOp		  = VK_BLEND_OP_ADD;
		VkBlendFactor	srcAlpha      = VK_BLEND_FACTOR_ONE;
		VkBlendFactor	dstAlpha      = VK_BLEND_FACTOR_ONE;
		VkBlendOp		alphaOp       = VK_BLEND_OP_ADD;
		float			blendFactor   = 0.0f;
	};

	/**
	 * @class BlendState
	 * @brief Immutable blend configuration object returned by Renderer::GetBlendState().
	 */
	class BlendState : public SharedObject
	{
	public:
		/**
		 * @brief Constructs a BlendState object with the specified blend state configuration.
		 * @param spec The blend state configuration to use.
		 */
		BlendState(const BlendStateSpec &spec);
		BlendState() = default;
		~BlendState() = default;

		[[nodiscard]] bool          IsBlendEnabled()				const { return m_Spec.blendEnabled; }
		[[nodiscard]] VkBlendFactor GetSrcColor()					const { return m_Spec.srcBlend; }
		[[nodiscard]] VkBlendFactor GetDstColor()					const { return m_Spec.dstBlend; }
		[[nodiscard]] VkBlendOp     GetColorOp()					const { return m_Spec.blendOp;  }
		[[nodiscard]] VkBlendFactor GetSrcAlpha()					const { return m_Spec.srcAlpha; }
		[[nodiscard]] VkBlendFactor GetDstAlpha()					const { return m_Spec.dstAlpha; }
		[[nodiscard]] VkBlendOp     GetAlphaOp()					const { return m_Spec.alphaOp; }
		[[nodiscard]] VkCompareOp	GetDepthComparisonFunction()	const { return m_Depth_CompFunc; }
		[[nodiscard]] float GetBlendFactor()                        const { return m_Spec.blendFactor; }
		[[nodiscard]] uint64_t GetHash()							const { return m_Hash; }
		[[nodiscard]] bool operator==(const BlendState& state)		const { return m_Hash == state.m_Hash; }

		/**
		 * @brief Assignment operator for BlendState. Allows assigning from a Ref<BlendState> to another BlendState instance.
		 * @param ref The reference to the BlendState to assign from.
		 * @return A reference to the assigned BlendState.
		 */
		BlendState &operator=(const Ref<BlendState> & ref);

	private:
		BlendStateSpec m_Spec; // Store the original spec for potential future use (e.g., hashing, debugging)
		VkCompareOp m_Depth_CompFunc = VK_COMPARE_OP_NEVER;
		uint64_t m_Hash = 0;
	};

}

// -------------------------------------------------------

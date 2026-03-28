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
 * pipeline_state.h
 * -------------------------------------------------------
 * Created: 09/03/2026
 * -------------------------------------------------------
 */
#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <SceneryEditorX/renderer/renderer_declarations.h>

// ---------------------------------------------------------

namespace SceneryEditorX
{
	class Swapchain;
	class Shader;
	class ImageResource;
	class RasterizerState;
	class BlendState;
	class DepthStencilState;

	/* 
	 * Color sentinel – matches the Color type used by command lists.
	 * Using a raw float[4] here avoids pulling in <colors.h> from this header. 
	 */
	struct PipelineStateColor { float r = 0, g = 0, b = 0, a = 0; };

	// Opaque "load" sentinel colours (negative alpha = "load, don't clear").
	inline constexpr PipelineStateColor RHI_COLOR_LOAD{.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = -1.0f };

	/**
	 * @class PipelineState
	 * @brief High-level, API-agnostic descriptor for a graphics or compute pipeline.
	 *
	 * Passes build a PipelineState, then call CommandList::SetPipelineState() which
	 * resolves or creates the underlying VkPipeline and starts the render pass.
	 */
	class PipelineState
	{
	public:
		PipelineState();
		~PipelineState();

		void Prepare();
		[[nodiscard]] bool HasClearValues() const;
		[[nodiscard]] bool IsGraphics() const;
		[[nodiscard]] bool IsCompute() const;
		[[nodiscard]] bool HasTessellation();

		[[nodiscard]] uint32_t GetWidth() const  { return m_Width; }
		[[nodiscard]] uint32_t GetHeight() const { return m_Height; }
		[[nodiscard]] uint64_t GetHash() const   { return m_Hash; }

		// Shader stages – indexed by Stage enum (vertex=0, geometry=1, tess_ctrl=2, tess_eval=3, fragment=4, compute=5)
		std::map<uint32_t, Shader*> shaders;

		// Pipeline state objects (nullptr = use defaults)
		RasterizerState*    rasterizerState       = nullptr;
		BlendState*         blendState            = nullptr;
		DepthStencilState*  depthStencil_State    = nullptr;
		VkPrimitiveTopology primitiveTopology	  = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		bool isMultiview                          = false;

		// Render targets
		std::array<ImageResource*, MAX_RENDER_TARGET_COUNT> renderTarget_ColorTextures = {};
		ImageResource*  renderTarget_DepthTexture = nullptr;
		ImageResource*  vrsInputTexture           = nullptr;
		Swapchain*		renderTarget_Swapchain    = nullptr;
		uint32_t renderTarget_ArrayIndex          = 0;

		// Clear values (color_load.a < 0 = load; non-negative = clear to this colour)
		std::array<PipelineStateColor, MAX_RENDER_TARGET_COUNT> clearColor = {};
		float clearDepth		= RHI_DEPTH_LOAD;
		uint32_t clearStencil	= STENCIL_LOAD;

		// Misc flags
		bool resolutionScale = false;
		const char* name = nullptr;

	private:
		[[nodiscard]] bool HasShader(const Stage shaderStage) const;
		uint32_t m_Width  = 0;
		uint32_t m_Height = 0;
		uint64_t m_Hash   = 0;
	};
}

// ---------------------------------------------------------

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

#include <SceneryEditorX/renderer/renderer_declarations.h>
#include <SceneryEditorX/renderer/vulkan/render_data.h>
#include <xMath/includes/colors.h>

// ---------------------------------------------------------

namespace SceneryEditorX
{
	class Swapchain;
	class Shader;
	class ImageResource;
	class RasterizerState;
	class BlendState;
	class DepthStencilState;

	/**
	 * @brief matches xMath::Color used by command lists.
	 */
	struct PipelineStateColor 
	{ 
		float r = 0, g = 0, b = 0, a = 0;

		/**
		 * @brief Create a PipelineStateColor from an xMath::Color.
		 * @param color Source color.
		 * @return PipelineStateColor with copied RGBA components.
		 */
		[[nodiscard]] static PipelineStateColor FromColor(const xMath::Color& color)
		{
		   PipelineStateColor out;
			out.r = color.r;
			out.g = color.g;
			out.b = color.b;
			out.a = color.a;
			return out;
		}

		/**
		 * @brief Assign a Color to the PipelineStateColor.
		 * @param color The Color to assign.
		 * @return Reference to the updated PipelineStateColor.
		 */
		PipelineStateColor& operator=(const xMath::Color& color)
		{
			r = color.r;
			g = color.g;
			b = color.b;
			a = color.a;
			return *this;
		}

		/**
		 * @brief Convert to xMath::Color.
		 * @return xMath::Color with matching RGBA components.
		 */
		[[nodiscard]] xMath::Color ToColor() const
		{
		  return {r, g, b, a};
		}

	};

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
		/* @brief Construct a new PipelineState object. */
		PipelineState();

		/* @brief Destroy the pipeline state. */
		~PipelineState();

		/* @brief Prepares the pipeline state for use. */
		void Prepare();

		[[nodiscard]] bool HasClearValues() const;
		[[nodiscard]] bool IsGraphics() const;
		[[nodiscard]] bool IsCompute() const;
		[[nodiscard]] bool HasTessellation();

		[[nodiscard]] uint32_t GetWidth() const  { return m_Width; }
		[[nodiscard]] uint32_t GetHeight() const { return m_Height; }
		[[nodiscard]] uint64_t GetHash() const   { return m_Hash; }

		/**
		 * @brief Get the current state of the pipeline.
		 * @return The current pipeline state.
		 */
		static PipelineState GetState();

		// Shader stages – indexed by StageType enum (vertex=0, geometry=1, tess_ctrl=2, tess_eval=3, fragment=4, compute=5)
		std::array<Shader*, static_cast<uint32_t>(StageType::MaxEnum)> shaders = {};

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
		/**
		 * @brief Check if a specific shader stage is present in the pipeline state.
		 * @param shaderStage The shader stage to check.
		 * @return True if the shader stage is present, false otherwise.
		 */
		[[nodiscard]] bool HasShader(const StageType shaderStage) const;

		uint32_t m_Width  = 0;
		uint32_t m_Height = 0;
		uint64_t m_Hash   = 0;
	};
}

// ---------------------------------------------------------

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
 * pipeline_state.cpp
 * -------------------------------------------------------
 * Created: 27/03/2026
 * -------------------------------------------------------
 */
#include "pipeline_state.h"
#include <SceneryEditorX/renderer/vulkan/blend_states.h>
#include <SceneryEditorX/renderer/vulkan/depth_stencil.h>
#include <SceneryEditorX/renderer/vulkan/image_resource.h>
#include <SceneryEditorX/renderer/vulkan/rasterizer.h>
#include <SceneryEditorX/renderer/vulkan/swapchain.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	static void Validate(PipelineState& pso)
	{
		bool hasShaderCompute     = pso.shaders[Stage::Compute]					? pso.shaders[Stage::Compute]->IsCompiled()					: false;
		bool hasShaderVertex      = pso.shaders[Stage::Vertex]					? pso.shaders[Stage::Vertex]->IsCompiled()					: false;
		bool hasShaderHull        = pso.shaders[Stage::TessellationControl]     ? pso.shaders[Stage::TessellationControl]->IsCompiled()     : false;
		bool hasShaderDomain      = pso.shaders[Stage::TessellationEvaluation]  ? pso.shaders[Stage::TessellationEvaluation]->IsCompiled()  : false;
		bool hasShaderFragment    = pso.shaders[Stage::Fragment]				? pso.shaders[Stage::Fragment]->IsCompiled()				: false;

		bool hasSomeShader = hasShaderCompute || hasShaderVertex || hasShaderHull || hasShaderDomain;
		SEDX_CORE_ASSERT(hasSomeShader, "There is no shader set, ensure that it compiled successfully and that it has been set");
	
		bool isGraphics = (hasShaderVertex || hasShaderHull || hasShaderDomain || hasShaderFragment) && !hasShaderCompute;
		bool isCompute  = hasShaderCompute && !hasShaderVertex && !hasShaderHull && !hasShaderDomain;
		SEDX_CORE_ASSERT(isGraphics || isCompute, "Invalid pipeline state type, must be graphics or compute");
	
		if (isGraphics)
		{
			bool hasRenderTarget   = pso.renderTarget_ColorTextures[0] || pso.renderTarget_DepthTexture; // ensure at least one render target
			bool hasBackbuffer     = pso.renderTarget_Swapchain; // check that both the swapchain and the color render target are active
			bool hasGraphicsStates = pso.rasterizerState && pso.blendState && pso.depthStencil_State;
			SEDX_CORE_ASSERT(hasGraphicsStates,                 "Graphics states are missing");
			SEDX_CORE_ASSERT(hasRenderTarget || hasBackbuffer,  "A render target is missing");
			SEDX_CORE_ASSERT(pso.blendState,                    "You need to define a blend state");
			SEDX_CORE_ASSERT(pso.depthStencil_State,            "You need to define a depth-stencil state");
			SEDX_CORE_ASSERT(pso.rasterizerState,               "You need to define a rasterizer state");
			SEDX_CORE_ASSERT(pso.GetWidth() != 0 && pso.GetHeight() != 0);
		}
	
		SEDX_CORE_ASSERT(pso.name != nullptr, "Name your pipeline state");
	}

	static uint64_t HashCombine(const uint64_t a, const uint64_t b)
	{
		return a * 31 + b;
	}

	static uint64_t ComputeHash(PipelineState& pso)
	{
		uint64_t hash = 0;
	
		hash = HashCombine(hash, static_cast<uint64_t>(pso.primitiveTopology));
	
		if (pso.renderTarget_Swapchain)
		{
			hash = HashCombine(hash, static_cast<uint64_t>(pso.renderTarget_Swapchain->GetImageFormat()));
		}
	
		if (pso.rasterizerState)
		{
			hash = HashCombine(hash, pso.rasterizerState->GetHash());
		}
	
		if (pso.blendState)
		{
			hash = HashCombine(hash, pso.blendState->GetHash());
		}
	
		if (pso.depthStencil_State)
		{
			hash = HashCombine(hash, pso.depthStencil_State->GetHash());
		}
	
		// shaders
		for (Shader* shader : pso.shaders)
		{
			if (!shader)
				continue;
	
			hash = HashCombine(hash, shader->GetHash());
		}
	
		// render target
		{
			// color
			for (uint32_t i = 0; i < MAX_RENDER_TARGET_COUNT; i++)
			{
				if (ImageResource* img = pso.renderTarget_ColorTextures[i])
				{
					hash = HashCombine(hash, img->GetObjectId());
				}
			}
	
			// depth
			if (ImageResource* img = pso.renderTarget_DepthTexture)
			{
				hash = HashCombine(hash, img->GetObjectId());
			}
	
			// variable rate shading
			if (pso.vrsInputTexture)
			{
				hash = HashCombine(hash, pso.vrsInputTexture->GetObjectId());
			}
	
			hash = HashCombine(hash, pso.renderTarget_ArrayIndex);
			hash = HashCombine(hash, static_cast<uint64_t>(pso.isMultiview));
		}
	
		return hash;
	}
	
	static void GetDimensions(PipelineState& pso, uint32_t* width, uint32_t* height)
	{
		SEDX_CORE_ASSERT(width && height);
	
		*width  = 0;
		*height = 0;
	
		if (pso.renderTarget_Swapchain)
		{
			if (width)  *width  = pso.renderTarget_Swapchain->GetWidth();
			if (height) *height = pso.renderTarget_Swapchain->GetHeight();
		}
		else if (pso.renderTarget_ColorTextures[0])
		{
			if (width)  *width  = pso.renderTarget_ColorTextures[0]->GetWidth();
			if (height) *height = pso.renderTarget_ColorTextures[0]->GetHeight();
		}
		else if (pso.renderTarget_DepthTexture)
		{
			if (width)  *width  = pso.renderTarget_DepthTexture->GetWidth();
			if (height) *height = pso.renderTarget_DepthTexture->GetHeight();
		}
	
		if (pso.resolutionScale)
		{ 
			*width                 = static_cast<uint32_t>(*width * pso.resolutionScale);
			*height                = static_cast<uint32_t>(*height * pso.resolutionScale);
		}
	}


	PipelineState::PipelineState()
	{
	    m_Hash = ComputeHash(*this);
		clearColor.fill(RHI_COLOR_LOAD);
		renderTarget_ColorTextures.fill(nullptr);
	}
	
	PipelineState::~PipelineState()
	{
	}

	void PipelineState::Prepare()
	{
		//m_hash = compute_hash(*this);
		GetDimensions(*this, &m_Width, &m_Height);
		Validate(*this);
	}

	bool PipelineState::HasClearValues() const
	{
		if (clearDepth != RHI_DEPTH_LOAD && clearDepth != DEPTH_DONT_CARE)
			return true;

		if (clearStencil != STENCIL_LOAD && clearStencil != STENCIL_DONT_CARE)
			return true;

		for (const PipelineStateColor& color : clearColor)
		{
			if (color != RHI_COLOR_LOAD && color != COLOR_DONT_CARE)
				return true;
		}

		return false;
	}

	bool PipelineState::IsGraphics() const
	{
		return (HasShader(Stage::Vertex) ||
				HasShader(Stage::TessellationControl) ||
				HasShader(Stage::TessellationEvaluation) ||
				HasShader(Stage::Fragment)) && !HasShader(Stage::Compute);
	}

	bool PipelineState::IsCompute() const
	{
		return HasShader(Stage::Compute) &&
			!HasShader(Stage::Vertex) &&
			!HasShader(Stage::TessellationControl) &&
			!HasShader(Stage::TessellationEvaluation) &&
			!HasShader(Stage::Fragment);
	}

	bool PipelineState::HasTessellation()
	{
		return HasShader(Stage::TessellationControl) && HasShader(Stage::TessellationEvaluation);
	}

	bool PipelineState::HasShader(const Stage shaderStage) const
	{
		return shaders[static_cast<uint32_t>(shaderStage)] != nullptr;
	}

} // namespace SceneryEditorX

// -------------------------------------------------------

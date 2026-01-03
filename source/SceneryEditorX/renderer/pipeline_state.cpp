/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* pipeline_state.cpp
* -------------------------------------------------------
* Created: 28/8/2025
* -------------------------------------------------------
*/
#include "pipeline_state.h"
#include "renderer.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

	/*
	namespace
	{

	    void Validate(PipelineState& pso)
        {
            bool HasComputeShader  = pso.shaders[ShaderStage::Stage::Compute] ? pso.shaders[ShaderStage::Stage::Compute]->IsCompiled() : false;
            bool HasVertexShader   = pso.shaders[ShaderStage::Stage::Vertex]  ? pso.shaders[ShaderStage::Stage::Vertex]->IsCompiled()  : false;
            bool HasFragmentShader    = pso.shaders[ShaderStage::Stage::Fragment]   ? pso.shaders[ShaderStage::Stage::Fragment]->IsCompiled()   : false;
            bool IsGraphics         = (HasVertexShader || HasFragmentShader) && !HasComputeShader;

            SEDX_ASSERT(HasComputeShader || HasVertexShader || HasFragmentShader, "There is no shader set, ensure that it compiled successfully and that it has been set");
            if (IsGraphics)
            {
                bool HasRenderTarget   = pso.renderTargetColorTextures[0] || pso.renderTargetDepthTexture; // ensure at least one render target
                bool HasBackbuffer      = pso.renderTargetSwapchain;                                            // check that no both the swapchain and the color render target are active
                bool HasGraphicsStates = pso.rasterizerState && pso.blendState && pso.depthStencil;

                SEDX_ASSERT(HasGraphicsStates,                 "Graphics states are missing");
                SEDX_ASSERT(HasRenderTarget || HasBackbuffer, "A render target is missing");
                SEDX_ASSERT(pso.blendState,                     "You need to define a blend state");
                SEDX_ASSERT(pso.depthStencil,             "You need to define a depth-stencil state");
                SEDX_ASSERT(pso.rasterizerState,                "You need to define a rasterizer state");
                SEDX_ASSERT(pso.GetWidth() != 0 && pso.GetHeight() != 0);
            }

            SEDX_ASSERT(pso.debugName != nullptr, "Name your pipeline state");
        }

        uint64_t ComputeHash(PipelineState &pso)
        {
            uint64_t hash = 0;

            hash = HashCombine(hash, static_cast<uint64_t>(pso.topology));
            if (pso.renderTargetSwapchain)
                hash = HashCombine(hash, static_cast<uint64_t>(pso.renderTargetSwapchain->Get()->GetColorFormat()));
            if (pso.rasterizerState)
                hash = HashCombine(hash, pso.rasterizerState->GetHash());
            if (pso.blendState)
                hash = HashCombine(hash, pso.blendState->GetHash());
            if (pso.depthStencil)
                hash = HashCombine(hash, pso.depthStencil->GetHash());

            // shaders
            for (Ref<Shader>* shader : pso.shaders)
            {
                if (!shader)
                    continue;

                hash = HashCombine(hash, shader->Get()->GetHash());
            }

            // rt
            {
                // Color
                for (uint32_t i = 0; i < MAX_RENDER_TARGET_COUNT; i++)
                {
                    if (Ref<Texture2D> *texture = pso.renderTargetColorTextures[i])
                        hash = HashCombine(hash, texture->Get()->GetObjectId());
                }

                // Depth
                if (pso.renderTargetDepthTexture)
                    hash = HashCombine(hash, pso.renderTargetDepthTexture->Get()->GetObjectId());

                // Variable Rate Shading
                if (pso.inputTexture)
                    hash = HashCombine(hash, pso.inputTexture->Get->GetObjectId());

                hash = HashCombine(hash, pso.renderTargetArrayIdx);
            }

            return hash;
        }

		
        void GetDimensions(PipelineState& pso, uint32_t* width, uint32_t* height)
        {
            SEDX_ASSERT(width && height);

            *width  = 0;
            *height = 0;

            if (pso.renderTargetSwapchain)
            {
                if (width)  *width  = pso.renderTargetSwapchain->GetWidth();
                if (height) *height = pso.renderTargetSwapchain->GetHeight();
            }
            else if (pso.render_target_color_textures[0])
            {
                if (width)  *width  = pso.render_target_color_textures[0]->GetWidth();
                if (height) *height = pso.render_target_color_textures[0]->GetHeight();
            }
            else if (pso.renderTargetDepthTexture)
            {
                if (width)  *width  = pso.renderTargetDepthTexture->GetWidth();
                if (height) *height = pso.renderTargetDepthTexture->GetHeight();
            }

            if (pso.ResolutionScale)
            { 
                float resolutionScale = Renderer::GetOption<float>(RendererOption::ResolutionScale);
                *width                 = static_cast<uint32_t>(*width * resolutionScale);
                *height                = static_cast<uint32_t>(*height * resolutionScale);
            }
        }

	}

	PipelineState::PipelineState()
	{
        clearColor.fill(COLOR_LOAD);
        render_target_color_textures.fill(nullptr);
	}
	
	PipelineState::~PipelineState() = default;

    void PipelineState::Prepare()
	{
        m_Hash = ComputeHash(*this);
        GetDimensions(*this, &m_Width, &m_Height);
        Validate(*this);
	}
	
	bool PipelineState::HasClearValues() const
	{
        if (clearDepth != DEPTH_LOAD && clearDepth != DEPTH_DONT_CARE)
            return true;

        if (clearStencil != STENCIL_LOAD && clearStencil != STENCIL_DONT_CARE)
            return true;

        for (const Color& color : clearColor)
        {
            if (color != COLOR_LOAD && color != COLOR_DONT_CARE)
                return true;
        }

        return false;
	}
	
	bool PipelineState::IsGraphics() const
	{
        return (HasShader(ShaderStage::Stage::Vertex) || HasShader(ShaderStage::Stage::Fragment)) &&
               !HasShader(ShaderStage::Stage::Compute);

	}
	
	bool PipelineState::IsCompute() const
	{
        return HasShader(ShaderStage::Stage::Compute) && !(HasShader(ShaderStage::Stage::Vertex) || HasShader(ShaderStage::Stage::Fragment));
	}
	
	bool PipelineState::HasTessellation()
	{
	    return false;
	}
	
	bool PipelineState::HasShader(const ShaderStage::Stage shaderStage) const
	{
        return shaders[static_cast<uint32_t>(shaderStage)] != nullptr;
	}
	*/
}

// -------------------------------------------------------

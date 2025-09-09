/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* pipeline_states.cpp
* -------------------------------------------------------
* Created: 28/8/2025
* -------------------------------------------------------
*/
#include "pipeline_states.h"

#include "renderer.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{

	    void validate(PipelineStates& pso)
        {
            bool has_shader_compute  = pso.shaders[ShaderStage::Stage::Compute] ? pso.shaders[ShaderStage::Stage::Compute]->IsCompiled() : false;
            bool has_shader_vertex   = pso.shaders[ShaderStage::Stage::Vertex]  ? pso.shaders[ShaderStage::Stage::Vertex]->IsCompiled()  : false;
            bool has_shader_pixel    = pso.shaders[ShaderStage::Stage::Fragment]   ? pso.shaders[ShaderStage::Stage::Fragment]->IsCompiled()   : false;
            bool is_graphics         = (has_shader_vertex || has_shader_pixel) && !has_shader_compute;

            SEDX_ASSERT(has_shader_compute || has_shader_vertex || has_shader_pixel, "There is no shader set, ensure that it compiled successfully and that it has been set");
            if (is_graphics)
            {
                bool has_render_target   = pso.render_target_color_textures[0] || pso.render_target_depth_texture; // ensure at least one render target
                bool has_backbuffer      = pso.render_target_swapchain;                                            // check that no both the swapchain and the color render target are active
                bool has_graphics_states = pso.rasterizerState && pso.blendState && pso.depthStencil;

                SEDX_ASSERT(has_graphics_states,                 "Graphics states are missing");
                SEDX_ASSERT(has_render_target || has_backbuffer, "A render target is missing");
                SEDX_ASSERT(pso.blendState,                     "You need to define a blend state");
                SEDX_ASSERT(pso.depthStencil,             "You need to define a depth-stencil state");
                SEDX_ASSERT(pso.rasterizerState,                "You need to define a rasterizer state");
                SEDX_ASSERT(pso.GetWidth() != 0 && pso.GetHeight() != 0);
            }

            SEDX_ASSERT(pso.debugName != nullptr, "Name your pipeline state");
        }

        uint64_t compute_hash(PipelineStates &pso)
        {
            uint64_t hash = 0;

            hash = hash_combine(hash, static_cast<uint64_t>(pso.topology));

            if (pso.render_target_swapchain)
                hash = hash_combine(hash, static_cast<uint64_t>(pso.render_target_swapchain->GetFormat()));

            if (pso.rasterizerState)
                hash = hash_combine(hash, pso.rasterizerState->GetHash());

            if (pso.blendState)
                hash = hash_combine(hash, pso.blendState->GetHash());

            if (pso.depthStencil)
                hash = hash_combine(hash, pso.depthStencil->GetHash());

            // shaders
            for (Ref<Shader>* shader : pso.shaders)
            {
                if (!shader)
                    continue;

                hash = hash_combine(hash, shader->GetHash());
            }

            // rt
            {
                // color
                for (uint32_t i = 0; i < max_render_target_count; i++)
                {
                    if (Ref<Texture2D> *texture = pso.render_target_color_textures[i])
                        hash = hash_combine(hash, texture->GetObjectId());
                }

                // depth
                if (pso.render_target_depth_texture)
                    hash = hash_combine(hash, pso.render_target_depth_texture->GetObjectId());

                // variable rate shading
                if (pso.vrs_input_texture)
                    hash = hash_combine(hash, pso.vrs_input_texture->GetObjectId());

                hash = hash_combine(hash, pso.render_target_array_index);
            }

            return hash;
        }

		
        void get_dimensions(PipelineStates& pso, uint32_t* width, uint32_t* height)
        {
            SEDX_ASSERT(width && height);

            *width  = 0;
            *height = 0;

            if (pso.render_target_swapchain)
            {
                if (width)  *width  = pso.render_target_swapchain->GetWidth();
                if (height) *height = pso.render_target_swapchain->GetHeight();
            }
            else if (pso.render_target_color_textures[0])
            {
                if (width)  *width  = pso.render_target_color_textures[0]->GetWidth();
                if (height) *height = pso.render_target_color_textures[0]->GetHeight();
            }
            else if (pso.render_target_depth_texture)
            {
                if (width)  *width  = pso.render_target_depth_texture->GetWidth();
                if (height) *height = pso.render_target_depth_texture->GetHeight();
            }

            if (pso.resolution_scale)
            { 
                float resolution_scale = Renderer::GetOption<float>(Renderer_Option::ResolutionScale);
                *width                 = static_cast<uint32_t>(*width * resolution_scale);
                *height                = static_cast<uint32_t>(*height * resolution_scale);
            }
        }

	}

	PipelineStates::PipelineStates()
	{
        clearColor.fill(color_load);
        render_target_color_textures.fill(nullptr);
	}
	
	PipelineStates::~PipelineStates() = default;

    void PipelineStates::Prepare()
	{
        hash = compute_hash(*this);
        get_dimensions(*this, &m_width, &m_height);
        validate(*this);
	}
	
	bool PipelineStates::HasClearValues() const
	{
        if (clearDepth != depthLoad && clearDepth != depth_dont_care)
            return true;

        if (clearStencil != stencilLoad && clearStencil != stencil_dont_care)
            return true;

        for (const Color& color : clearColor)
        {
            if (color != colorLoad && color != color_dont_care)
                return true;
        }

        return false;
	}
	
	bool PipelineStates::IsGraphics() const
	{
        return (HasShader(ShaderStage::Stage::Vertex) || HasShader(ShaderStage::Stage::Fragment)) &&
               !HasShader(ShaderStage::Stage::Compute);

	}
	
	bool PipelineStates::IsCompute() const
	{
        return HasShader(ShaderStage::Stage::Compute) && !(HasShader(ShaderStage::Stage::Vertex) || HasShader(ShaderStage::Stage::Fragment));
	}
	
	bool PipelineStates::HasTessellation()
	{
	    return false;
	}
	
	bool PipelineStates::HasShader(const ShaderStage::Stage shader_stage) const
	{
        return shaders[static_cast<uint32_t>(shader_stage)] != nullptr;
	}
}

/// -------------------------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* lighting_composition.cpp
* -------------------------------------------------------
* Created: 29/8/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/renderer/command_manager.h>
#include <SceneryEditorX/renderer/render_pass.h>
#include <SceneryEditorX/renderer/texture.h>
#include <SceneryEditorX/renderer/shaders/shader.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	
    void RenderPass::Pass_Light_Composition(CommandManager *cmd_list, const bool is_transparent_pass)
    {
        // acquire resources
        Shader* shader_c              = GetShader(Renderer_Shader::light_composition_c);
        Texture* tex_out              = GetRenderTarget(Renderer_RenderTarget::frame_render);
        Texture* tex_skysphere        = GetRenderTarget(Renderer_RenderTarget::skysphere);
        Texture* tex_light_diffuse    = GetRenderTarget(Renderer_RenderTarget::light_diffuse);
        Texture* tex_light_specular   = GetRenderTarget(Renderer_RenderTarget::light_specular);
        Texture* tex_light_volumetric = GetRenderTarget(Renderer_RenderTarget::light_volumetric);

        cmd_list->InsertBarrierReadWrite(tex_out, RHI_BarrierType::EnsureReadThenWrite);

        cmd_list->BeginTimeblock(is_transparent_pass ? "light_composition_transparent" : "light_composition");
        {
            // set pipeline state
            RHI_PipelineState pso;
            pso.name             = "light_composition";
            pso.shaders[Compute] = shader_c;
            cmd_list->SetPipelineState(pso);

            // push pass constants
            m_pcb_pass_cpu.set_is_transparent_and_material_index(is_transparent_pass);
            m_pcb_pass_cpu.set_f3_value(static_cast<float>(tex_skysphere->GetMipCount()), GetOption<float>(Renderer_Option::Fog), 0.0f);
            cmd_list->PushConstants(m_pcb_pass_cpu);

            // set textures
            SetCommonTextures(cmd_list);
            cmd_list->SetTexture(Renderer_BindingsUav::tex,  tex_out);
            cmd_list->SetTexture(Renderer_BindingsSrv::tex,  GetStandardTexture(Renderer_StandardTexture::Foam));
            cmd_list->SetTexture(Renderer_BindingsSrv::tex2, tex_skysphere);
            cmd_list->SetTexture(Renderer_BindingsSrv::tex3, tex_light_diffuse);
            cmd_list->SetTexture(Renderer_BindingsSrv::tex4, tex_light_specular);
            cmd_list->SetTexture(Renderer_BindingsSrv::tex5, tex_light_volumetric);

            // render
            cmd_list->Dispatch(tex_out);
        }
        cmd_list->EndTimeblock();
    }

}

/// -------------------------------------------------------

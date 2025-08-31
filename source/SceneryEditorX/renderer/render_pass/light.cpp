/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* light.cpp
* -------------------------------------------------------
* Created: 29/8/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/renderer/command_manager.h>
#include <SceneryEditorX/renderer/render_pass.h>
#include <SceneryEditorX/renderer/texture.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	
    void RenderPass::Pass_Light(CommandManager *cmd_list, const bool is_transparent_pass)
    {
        // acquire resources
        Texture* light_diffuse    = GetRenderTarget(RenderTarget::LightDiffuse);
        Texture* light_specular   = GetRenderTarget(RenderTarget::LightSpecular);
        Texture* light_shadow     = GetRenderTarget(RenderTarget::LightShadow);
        Texture* light_volumetric = GetRenderTarget(RenderTarget::LightVolumetric);

        // define pipeline state
        RHI_PipelineState pso;
        pso.name             = is_transparent_pass ? "light_transparent" : "light";
        pso.shaders[Compute] = GetShader(Renderer_Shader::light_c);

        // dispatch on the bindless light array and the shadow atlas
        cmd_list->BeginTimeblock(pso.name);
        {
            cmd_list->SetPipelineState(pso);

            // textures
            SetCommonTextures(cmd_list);
            cmd_list->SetTexture(Renderer_BindingsUav::tex_sss, GetRenderTarget(RenderTarget::SSS));
            cmd_list->SetTexture(Renderer_BindingsSrv::tex,     GetRenderTarget(RenderTarget::SkySphere));
            cmd_list->SetTexture(Renderer_BindingsSrv::tex2,    GetRenderTarget(RenderTarget::ShadowAtlas));
            cmd_list->SetTexture(Renderer_BindingsUav::tex,     light_diffuse);
            cmd_list->SetTexture(Renderer_BindingsUav::tex2,    light_specular);
            cmd_list->SetTexture(Renderer_BindingsUav::tex3,    light_shadow);
            cmd_list->SetTexture(Renderer_BindingsUav::tex4,    light_volumetric);

            // push constants
            m_pcb_pass_cpu.set_is_transparent_and_material_index(is_transparent_pass);
            m_pcb_pass_cpu.set_f3_value(static_cast<float>(static_cast<uint32_t>(World::GetLightCount())), GetOption<float>(Renderer_Option::Fog));
            cmd_list->PushConstants(m_pcb_pass_cpu);

            // dispatch
            cmd_list->Dispatch(light_diffuse); // adds read write barrier for light_diffuse internally
            cmd_list->InsertBarrierReadWrite(light_specular,   RHI_BarrierType::EnsureWriteThenRead);
            cmd_list->InsertBarrierReadWrite(light_shadow,     RHI_BarrierType::EnsureWriteThenRead);
            cmd_list->InsertBarrierReadWrite(light_volumetric, RHI_BarrierType::EnsureWriteThenRead);
        }
        cmd_list->EndTimeblock();
    }

}

/// -------------------------------------------------------

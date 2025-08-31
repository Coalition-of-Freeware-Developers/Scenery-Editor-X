/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* sky_box.cpp
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
	
    void RenderPass::Pass_Skysphere(CommandManager *cmd_list)
    {
        Texture* tex_skysphere              = GetRenderTarget(Renderer_RenderTarget::skysphere);
        Texture* tex_lut_atmosphere_scatter = GetRenderTarget(Renderer_RenderTarget::lut_atmosphere_scatter);

        cmd_list->BeginTimeblock("skysphere");
        {
            // 1. atmospheric scattering
            if (World::GetDirectionalLight())
            {
                RHI_PipelineState pso;
                pso.name             = "skysphere_atmospheric_scattering";
                pso.shaders[Compute] = GetShader(Renderer_Shader::skysphere_c);
                cmd_list->SetPipelineState(pso);
    
                cmd_list->SetTexture(Renderer_BindingsUav::tex, tex_skysphere);
                cmd_list->SetTexture(Renderer_BindingsSrv::tex3d, tex_lut_atmosphere_scatter);
                cmd_list->Dispatch(tex_skysphere);
            }
            else
            {
                cmd_list->ClearTexture(tex_skysphere, Color::standard_black);
            }

            // 2. filter all mip levels
            {
                // filtering can sample from any mip, so we need to generate the mip chain
                Pass_Downscale(cmd_list, tex_skysphere, Renderer_DownsampleFilter::Average);

                RHI_PipelineState pso;
                pso.name             = "skysphere_filter";
                pso.shaders[Compute] = GetShader(Renderer_Shader::light_integration_environment_filter_c);
                cmd_list->SetPipelineState(pso);
            
                cmd_list->SetTexture(Renderer_BindingsSrv::tex, tex_skysphere);
            
                for (uint32_t mip_level = 1; mip_level < tex_skysphere->GetMipCount(); mip_level++)
                {
                    cmd_list->SetTexture(Renderer_BindingsUav::tex, tex_skysphere, mip_level, 1);
            
                    // Set pass constants
                    m_pcb_pass_cpu.set_f3_value(static_cast<float>(mip_level), static_cast<float>(tex_skysphere->GetMipCount()), 0.0f);
                    cmd_list->PushConstants(m_pcb_pass_cpu);
            
                    const uint32_t resolution_x = tex_skysphere->GetWidth() >> mip_level;
                    const uint32_t resolution_y = tex_skysphere->GetHeight() >> mip_level;
                    cmd_list->Dispatch(tex_skysphere);
                    cmd_list->InsertBarrierReadWrite(tex_skysphere, RHI_BarrierType::EnsureWriteThenRead);
                }
            }
        }
        cmd_list->EndTimeblock();
    }

}

/// -------------------------------------------------------

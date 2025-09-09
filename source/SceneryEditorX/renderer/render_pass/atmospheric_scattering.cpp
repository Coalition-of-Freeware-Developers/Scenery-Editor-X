/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* atmospheric_scattering.cpp
* -------------------------------------------------------
* Created: 29/8/2025
* -------------------------------------------------------
*/
//#include <SceneryEditorX/renderer/command_manager.h>
//#include <SceneryEditorX/renderer/render_pass.h>
//#include <SceneryEditorX/renderer/texture.h>

/// -------------------------------------------------------

/*
namespace SceneryEditorX
{
    void RenderPass::Pass_Lut_AtmosphericScattering(CommandManager *cmd_list)
    {
        Ref<Texture2D> *tex_lut_atmosphere_scatter = GetRenderTarget(RenderTarget::LUT_Atmosphere_Scatter);

        cmd_list->BeginTimeblock("lut_atmospheric_scattering");
        {
            // set pipeline state
            RHI_PipelineState pso;
            pso.name             = "lut_atmospheric_scattering";
            pso.shaders[Compute] = GetShader(Renderer_Shader::skysphere_lut_c);
            cmd_list->SetPipelineState(pso);
        
            cmd_list->SetTexture(Renderer_BindingsUav::tex3d, tex_lut_atmosphere_scatter);
            cmd_list->Dispatch(tex_lut_atmosphere_scatter);
        
            // for the lifetime of the engine, this will be read as a srv, so transition here
            tex_lut_atmosphere_scatter->SetLayout(Layout::ImageLayout::ShaderRead, cmd_list);
        }
        cmd_list->EndTimeblock();
    }
}
*/

/// -------------------------------------------------------

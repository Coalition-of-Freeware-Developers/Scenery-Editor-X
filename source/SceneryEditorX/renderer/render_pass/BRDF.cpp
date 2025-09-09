/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* BRDF.cpp
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
	
    void RenderPass::Pass_Lut_BrdfSpecular(CommandManager *cmd_list)
    {
        Texture* tex_lut_brdf_specular = GetRenderTarget(Renderer_RenderTarget::lut_brdf_specular);

        cmd_list->BeginTimeblock("lut_brdf_specular");
        {
            // set pipeline state
            RHI_PipelineState pso;
            pso.name             = "lut_brdf_specular";
            pso.shaders[Compute] = GetShader(Renderer_Shader::light_integration_brdf_specular_lut_c);
            cmd_list->SetPipelineState(pso);

            cmd_list->SetTexture(Renderer_BindingsUav::tex, tex_lut_brdf_specular);
            cmd_list->Dispatch(tex_lut_brdf_specular);

            // for the lifetime of the engine, this will be read as a srv, so transition here
            cmd_list->InsertBarrier(tex_lut_brdf_specular->GetRhiResource(), tex_lut_brdf_specular->GetFormat(), 0, 1, 1, Layout::ImageLayout::ShaderRead);
        }
        cmd_list->EndTimeblock();
    }

}
*/

/// -------------------------------------------------------

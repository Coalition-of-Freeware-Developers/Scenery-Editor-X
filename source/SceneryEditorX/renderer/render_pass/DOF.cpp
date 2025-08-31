/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* DOF.cpp
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
	
    void RenderPass::Pass_DepthOfField(CommandManager *cmd_list, Texture *tex_in, Texture *tex_out)
    {
        // acquire shader
        Shader* shader_c = GetShader(Renderer_Shader::depth_of_field_c);
    
        cmd_list->BeginTimeblock("depth_of_field");

        // set pipeline state
        RHI_PipelineState pso;
        pso.name             = "depth_of_field";
        pso.shaders[Compute] = shader_c;
        cmd_list->SetPipelineState(pso);

        // set pass constants
        m_pcb_pass_cpu.set_f3_value(World::GetCamera()->GetAperture(), 0.0f, 0.0f);
        cmd_list->PushConstants(m_pcb_pass_cpu);

        // set textures
        SetCommonTextures(cmd_list);
        cmd_list->SetTexture(Renderer_BindingsSrv::tex, tex_in);
        cmd_list->SetTexture(Renderer_BindingsUav::tex, tex_out);
        

        // render
        cmd_list->Dispatch(tex_out);

        cmd_list->EndTimeblock();
    }

}

/// -------------------------------------------------------

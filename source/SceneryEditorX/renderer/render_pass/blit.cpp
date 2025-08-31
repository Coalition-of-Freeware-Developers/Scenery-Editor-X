/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* blit.cpp
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
	
    void RenderPass::Pass_Blit(CommandManager *cmd_list, Texture *tex_in, Texture *tex_out)
    {
        // we use a compute shader to blit from depth to float, as Vulkan doesn't support blitting depth to float formats
        // and amd hardware requires UAV textures to be float-based (preventing depth format usage)
        // if the above is not your case, use RHI_CommandList::Blit instead, which is the fastest option

        // acquire resources
        Shader* shader_c = GetShader(Renderer_Shader::blit_c);

        cmd_list->BeginTimeblock("blit");
        {
            // set pipeline state
            RHI_PipelineState pso;
            pso.name             = "blit";
            pso.shaders[Compute] = shader_c;
            cmd_list->SetPipelineState(pso);
            
            // set textures
            cmd_list->SetTexture(Renderer_BindingsUav::tex, tex_out);
            cmd_list->SetTexture(Renderer_BindingsSrv::tex, tex_in);
            
            // render
            cmd_list->Dispatch(tex_out);
        }
        cmd_list->EndTimeblock();
    }

}

/// -------------------------------------------------------

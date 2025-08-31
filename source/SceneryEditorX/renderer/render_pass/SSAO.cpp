/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* SSAO.cpp
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
    void RenderPass::Pass_ScreenSpaceAmbientOcclusion(CommandManager *cmd_list)
    {
        static bool cleared = false;
        Texture* tex_ssao = GetRenderTarget(RenderTarget::SSAO);

        if (GetOption<bool>(Renderer_Option::ScreenSpaceAmbientOcclusion))
        {
            RHI_PipelineState pso;
            pso.name             = "screen_space_ambient_occlusion";
            pso.shaders[Compute] = GetShader(Renderer_Shader::ssao_c);

            cmd_list->BeginTimeblock(pso.name);
            {
                cmd_list->SetPipelineState(pso);
                SetCommonTextures(cmd_list);
                cmd_list->SetTexture(Renderer_BindingsUav::tex, tex_ssao);
                cmd_list->Dispatch(tex_ssao);

                cleared = false;
            }
            cmd_list->EndTimeblock();
        }
        else if (!cleared)
        {
            cmd_list->ClearTexture(tex_ssao, Color::standard_white);
            cleared = true;
        }
    }

}

/// -------------------------------------------------------

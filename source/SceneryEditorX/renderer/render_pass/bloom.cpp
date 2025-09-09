/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* bloom.cpp
* -------------------------------------------------------
* Created: 29/8/2025
* -------------------------------------------------------
*/
//#include <SceneryEditorX/renderer/command_manager.h>
//#include <SceneryEditorX/renderer/render_pass.h>
//#include <SceneryEditorX/renderer/texture.h>
//#include <SceneryEditorX/renderer/shaders/shader.h>

/// -------------------------------------------------------

/*
namespace SceneryEditorX
{
	
    void RenderPass::Pass_Bloom(CommandManager *cmd_list, Texture *tex_in, Texture *tex_out)
    {
        // acquire resources
        Shader* shader_luminance          = GetShader(Renderer_Shader::bloom_luminance_c);
        Shader* shader_upsample_blend_mip = GetShader(Renderer_Shader::bloom_upsample_blend_mip_c);
        Shader* shader_blend_frame        = GetShader(Renderer_Shader::bloom_blend_frame_c);
        Texture* tex_bloom                = GetRenderTarget(Renderer_RenderTarget::bloom);

        cmd_list->BeginTimeblock("bloom");

        // luminance
        cmd_list->BeginMarker("luminance");
        {
            // define pipeline state
            RHI_PipelineState pso;
            pso.name             = "bloom_luminance";
            pso.shaders[Compute] = shader_luminance;

            // set pipeline state
            cmd_list->SetPipelineState(pso);

            // set textures
            cmd_list->SetTexture(Renderer_BindingsUav::tex, tex_bloom);
            cmd_list->SetTexture(Renderer_BindingsSrv::tex, tex_in);

            // render
            cmd_list->Dispatch(tex_bloom);
        }
        cmd_list->EndMarker();

        // generate mips
        Pass_Downscale(cmd_list, tex_bloom, Renderer_DownsampleFilter::Luminance);

        // starting from the lowest mip, upsample and blend with the higher one
        cmd_list->BeginMarker("upsample_and_blend_with_higher_mip");
        {
            // define pipeline state
            RHI_PipelineState pso;
            pso.name             = "bloom_upsample_blend_mip";
            pso.shaders[Compute] = shader_upsample_blend_mip;

            // set pipeline state
            cmd_list->SetPipelineState(pso);

            // render
            for (int i = static_cast<int>(tex_bloom->GetMipCount() - 1); i > 0; i--)
            {
                int mip_index_small   = i;
                int mip_index_big     = i - 1;
                int mip_width_large   = tex_bloom->GetWidth()  >> mip_index_big;
                int mip_height_height = tex_bloom->GetHeight() >> mip_index_big;

                // set textures
                cmd_list->SetTexture(Renderer_BindingsSrv::tex, tex_bloom, mip_index_small, 1);
                cmd_list->SetTexture(Renderer_BindingsUav::tex, tex_bloom, mip_index_big, 1);

                // blend
                uint32_t thread_group_count    = 8;
                uint32_t thread_group_count_x_ = static_cast<uint32_t>(ceil(static_cast<float>(mip_width_large) / thread_group_count));
                uint32_t thread_group_count_y_ = static_cast<uint32_t>(ceil(static_cast<float>(mip_height_height) / thread_group_count));
                cmd_list->Dispatch(thread_group_count_x_, thread_group_count_y_);
            }
        }
        cmd_list->EndMarker();

        // blend with the frame
        cmd_list->BeginMarker("blend_with_frame");
        {
            // define pipeline state
            RHI_PipelineState pso;
            pso.name             = "bloom_blend_frame";
            pso.shaders[Compute] = shader_blend_frame;

            // set pipeline state
            cmd_list->SetPipelineState(pso);

            // set pass constants
            m_pcb_pass_cpu.set_f3_value(GetOption<float>(Renderer_Option::Bloom), 0.0f, 0.0f);
            cmd_list->PushConstants(m_pcb_pass_cpu);

            // set textures
            cmd_list->SetTexture(Renderer_BindingsUav::tex, tex_out);
            cmd_list->SetTexture(Renderer_BindingsSrv::tex, tex_in);
            cmd_list->SetTexture(Renderer_BindingsSrv::tex2, tex_bloom, 0, 1);

            // render
            cmd_list->Dispatch(tex_out);
        }
        cmd_list->EndMarker();

        cmd_list->EndTimeblock();
    }

}
*/

/// -------------------------------------------------------

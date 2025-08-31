/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* text.cpp
* -------------------------------------------------------
* Created: 29/8/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/renderer/command_manager.h>
#include <SceneryEditorX/renderer/render_pass.h>
#include <SceneryEditorX/renderer/texture.h>
#include <SceneryEditorX/renderer/fonts/font.h>
#include <SceneryEditorX/renderer/shaders/shader.h>
#include <SceneryEditorX/renderer/vulkan/vk_enums.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	
    void RenderPass::Pass_Text(CommandManager *cmd_list, Texture *tex_out)
    {
        // acquire resources
        const bool draw       = GetOption<bool>(Renderer_Option::PerformanceMetrics);
        const auto& shader_v  = GetShader(Renderer_Shader::font_v);
        const auto& shader_p  = GetShader(Renderer_Shader::font_p);
        Ref<Font> font = GetFont();

        if (!font->HasText())
            return;

        cmd_list->BeginTimeblock("text");

        font->UpdateVertexAndIndexBuffers(cmd_list);

        // define pipeline state
        RHI_PipelineState pso;
        pso.name								= "text";
        pso.shaders[ShaderType::Vertex]			= shader_v;
        pso.shaders[ShaderType::Fragment]		= shader_p;
        pso.rasterizer_state					= GetRasterizerState(Renderer_RasterizerState::Solid);
        pso.blend_state							= GetBlendState(Renderer_BlendState::Alpha);
        pso.depth_stencil_state					= GetDepthStencilState(Renderer_DepthStencilState::Off);
        pso.render_target_color_textures[0]		= tex_out;
        pso.clear_color[0]						= rhi_color_load;

        // set shared state
        cmd_list->SetPipelineState(pso);
        cmd_list->SetBufferVertex(font->GetVertexBuffer());
        cmd_list->SetBufferIndex(font->GetIndexBuffer());
        cmd_list->SetCullMode(CullMode::Back);

        // draw outline
        if (font->GetOutline() != Font_Outline_None && font->GetOutlineSize() != 0)
        {
            m_pcb_pass_cpu.set_f4_value(font->GetColorOutline());
            cmd_list->PushConstants(m_pcb_pass_cpu);
            cmd_list->SetTexture(Renderer_BindingsSrv::tex, font->GetAtlasOutline().get());
            cmd_list->DrawIndexed(font->GetIndexCount());
        }

        // draw inline
        {
            m_pcb_pass_cpu.set_f4_value(font->GetColor());
            cmd_list->PushConstants(m_pcb_pass_cpu);
            cmd_list->SetTexture(Renderer_BindingsSrv::tex, font->GetAtlas().get());
            cmd_list->DrawIndexed(font->GetIndexCount());
        }

        cmd_list->EndTimeblock();
    }
}

/// -------------------------------------------------------

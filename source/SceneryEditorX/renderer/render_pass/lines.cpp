/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* lines.cpp
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
    void RenderPass::Pass_Lines(CommandManager *cmd_list, Texture *tex_out)
    {
        Shader* shader_v  = GetShader(Renderer_Shader::line_v);
        Shader* shader_p  = GetShader(Renderer_Shader::line_p);
        uint32_t vertex_count = static_cast<uint32_t>(m_lines_vertices.size());

        if (vertex_count != 0)
        {
            cmd_list->BeginTimeblock("lines");

            // set pipeline state
            RHI_PipelineState pso;
            pso.name								= "lines";
            pso.shaders[ShaderType::Vertex]			= shader_v;
            pso.shaders[ShaderType::Fragment]		= shader_p;
            pso.rasterizer_state					= GetRasterizerState(Renderer_RasterizerState::Wireframe);
            pso.blend_state							= GetBlendState(Renderer_BlendState::Alpha);
            pso.depth_stencil_state					= GetDepthStencilState(Renderer_DepthStencilState::ReadGreaterEqual);
            pso.render_target_color_textures[0]		= tex_out;
            pso.clear_color[0]						= rhi_color_load;
            pso.render_target_depth_texture			= GetRenderTarget(Renderer_RenderTarget::gbuffer_depth_opaque_output);
            pso.primitive_toplogy					= PrimitiveTopology::Line;
            cmd_list->SetPipelineState(pso);

            // grow vertex buffer (if needed) 
            if (vertex_count > m_lines_vertex_buffer->GetElementCount())
            {
                m_lines_vertex_buffer = CreateRef<Buffer>(RHI_BufferType::Vertex, sizeof(m_lines_vertices[0]), vertex_count, static_cast<void*>(&m_lines_vertices[0]), true, "lines");
            }

            // update and set vertex buffer
            RHI_Vertex_PosCol* buffer = static_cast<RHI_Vertex_PosCol*>(m_lines_vertex_buffer->GetMappedData());
            memset(buffer, 0, m_lines_vertex_buffer->GetObjectSize());
            CopyBuffer(m_lines_vertices.begin(), m_lines_vertices.end(), buffer);
            cmd_list->SetBufferVertex(m_lines_vertex_buffer.get());

            cmd_list->SetCullMode(CullMode::None);
            cmd_list->Draw(static_cast<uint32_t>(m_lines_vertices.size()));
            cmd_list->SetCullMode(CullMode::Back);

            cmd_list->EndTimeblock();
        }
    }

}
*/

/// -------------------------------------------------------

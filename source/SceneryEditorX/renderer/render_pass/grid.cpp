/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* grid.cpp
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
	
    void RenderPass::Pass_Grid(CommandManager *cmd_list, Texture *tex_out)
    {
        if (!GetOption<bool>(Renderer_Option::Grid))
            return;

        // acquire resources
        Shader* shader_v = GetShader(Renderer_Shader::grid_v);
        Shader* shader_p = GetShader(Renderer_Shader::grid_p);

        cmd_list->BeginTimeblock("grid");

        // set pipeline state
        RHI_PipelineState pso;
        pso.name								= "grid";
        pso.shaders[ShaderType::Vertex]			= shader_v;
        pso.shaders[ShaderType::Fragment]		= shader_p;
        pso.rasterizer_state					= GetRasterizerState(Renderer_RasterizerState::Solid);
        pso.blend_state							= GetBlendState(Renderer_BlendState::Alpha);
        pso.depth_stencil_state					= GetDepthStencilState(Renderer_DepthStencilState::ReadGreaterEqual);
        pso.render_target_color_textures[0]		= tex_out;
        pso.render_target_depth_texture			= GetRenderTarget(Renderer_RenderTarget::gbuffer_depth_opaque_output);
        cmd_list->SetPipelineState(pso);

        // set transform
        {
            // follow camera in world unit increments so that the grid appears stationary in relation to the camera
            const float grid_spacing       = 1.0f;
            const Vec3& camera_position = World::GetCamera()->GetEntity()->GetPosition();
            const Vec3 translation      = Vec3(
                floor(camera_position.x / grid_spacing) * grid_spacing,
                0.0f,
                floor(camera_position.z / grid_spacing) * grid_spacing
            );

            m_pcb_pass_cpu.transform = Matrix::CreateScale(Vec3(1000.0f, 1.0f, 1000.0f)) * Matrix::CreateTranslation(translation);
            cmd_list->PushConstants(m_pcb_pass_cpu);
        }

        cmd_list->SetCullMode(CullMode::Back);
        cmd_list->SetBufferVertex(GetStandardMesh(MeshType::Quad)->GetVertexBuffer());
        cmd_list->SetBufferIndex(GetStandardMesh(MeshType::Quad)->GetIndexBuffer());
        cmd_list->DrawIndexed(6);

        cmd_list->EndTimeblock();
    }

}

/// -------------------------------------------------------

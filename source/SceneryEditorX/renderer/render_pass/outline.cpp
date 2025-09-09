/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* outline.cpp
* -------------------------------------------------------
* Created: 29/8/2025
* -------------------------------------------------------
*/
//#include <SceneryEditorX/renderer/camera.h>
//#include <SceneryEditorX/renderer/command_manager.h>
//#include <SceneryEditorX/renderer/render_pass.h>
//#include <SceneryEditorX/renderer/texture.h>
//#include <SceneryEditorX/renderer/shaders/shader.h>
//#include <SceneryEditorX/scene/entity.h>

/// -------------------------------------------------------

/*
namespace SceneryEditorX
{
	
    void RenderPass::Pass_Outline(CommandManager *cmd_list, Texture *tex_out)
    {
        if (!GetOption<bool>(Renderer_Option::SelectionOutline) || Engine::IsFlagSet(EngineMode::Playing))
            return;

        // acquire shaders
        Shader* shader_v = GetShader(Renderer_Shader::outline_v);
        Shader* shader_p = GetShader(Renderer_Shader::outline_p);
        Shader* shader_c = GetShader(Renderer_Shader::outline_c);

        if (Camera* camera = World::GetCamera())
        {
            if (Ref<Entity> entity_selected = camera->GetSelectedEntity())
            {
                cmd_list->BeginTimeblock("outline");
                {
                    Texture* tex_outline = GetRenderTarget(Renderer_RenderTarget::outline);

                    if (Renderable *renderable = entity_selected->GetComponent<Renderable>())
                    {
                        cmd_list->BeginMarker("color_silhouette");
                        {
                            // set pipeline state
                            RHI_PipelineState pso;
                            pso.name								= "color_silhouette";
                            pso.shaders[ShaderType::Vertex]			= shader_v;
                            pso.shaders[ShaderType::Fragment]		= shader_p;
                            pso.rasterizer_state					= GetRasterizerState(Renderer_RasterizerState::Solid);
                            pso.blend_state							= GetBlendState(Renderer_BlendState::Off);
                            pso.depth_stencil_state					= GetDepthStencilState(Renderer_DepthStencilState::Off);
                            pso.render_target_color_textures[0]		= tex_outline;
                            pso.clear_color[0]						= Color::standard_transparent;
                            cmd_list->SetPipelineState(pso);
                        
                            // render
                            {
                                // push draw data
                                m_pcb_pass_cpu.set_f4_value(Color::standard_renderer_lines);
                                m_pcb_pass_cpu.transform = entity_selected->GetMatrix();
                                cmd_list->PushConstants(m_pcb_pass_cpu);

                                // no mesh (vertex/index buffer) can occur if the mesh is selected but not loaded or the user removed it
                                if (renderable->GetVertexBuffer() && renderable->GetIndexBuffer())
                                {
                                    cmd_list->SetBufferVertex(renderable->GetVertexBuffer());
                                    cmd_list->SetBufferIndex(renderable->GetIndexBuffer());
                                    cmd_list->DrawIndexed(renderable->GetIndexCount(), renderable->GetIndexOffset(), renderable->GetVertexOffset());
                                }
                            }
                        }
                        cmd_list->EndMarker();
                        
                        // blur the color silhouette
                        {
                            const float radius = 30.0f;
                            Pass_Blur(cmd_list, tex_outline, false, radius);
                        }
                        
                        // combine color silhouette with frame
                        cmd_list->BeginMarker("composition");
                        {
                            // set pipeline state
                            RHI_PipelineState pso;
                            pso.name             = "composition";
                            pso.shaders[Compute] = shader_c;
                            cmd_list->SetPipelineState(pso);
                        
                            // set textures
                            cmd_list->SetTexture(Renderer_BindingsUav::tex, tex_out);
                            cmd_list->SetTexture(Renderer_BindingsSrv::tex, tex_outline);
                        
                            // render
                            cmd_list->Dispatch(tex_out);
                        }
                        cmd_list->EndMarker();
                    }
                }
                cmd_list->EndTimeblock();
            }
        }
    }

}
*/

/// -------------------------------------------------------

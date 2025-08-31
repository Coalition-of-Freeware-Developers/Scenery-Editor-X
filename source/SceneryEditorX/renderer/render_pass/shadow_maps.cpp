/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* shadow_maps.cpp
* -------------------------------------------------------
* Created: 29/8/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/renderer/command_manager.h>
#include <SceneryEditorX/renderer/render_pass.h>
#include <SceneryEditorX/renderer/viewport.h>
#include <SceneryEditorX/renderer/shaders/shader.h>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/scene/lights.h>
#include <SceneryEditorX/scene/material.h>

/// -----------------------------------------------------

namespace SceneryEditorX
{
	  
    void RenderPass::Pass_ShadowMaps(CommandManager *cmd_list)
    {
        if (World::GetLightCount() == 0)
            return;

        // define base pipeline state
        RHI_PipelineState pso;
        pso.name                            = "shadow_maps";
        pso.shaders[ShaderType::Vertex]		= GetShader(Renderer_Shader::depth_light_v);
        pso.blend_state                     = GetBlendState(Renderer_BlendState::Off);
        pso.depth_stencil_state             = GetDepthStencilState(Renderer_DepthStencilState::ReadWrite);
        pso.clear_depth                     = 0.0f;
        pso.render_target_depth_texture     = GetRenderTarget(Renderer_RenderTarget::shadow_atlas);
        pso.rasterizer_state                = GetRasterizerState(Renderer_RasterizerState::Light_directional); // the world always starts with the directional light

        cmd_list->BeginTimeblock(pso.name);
        {
            // set base state
            cmd_list->SetPipelineState(pso);

            // render shadow maps using cached renderables
            for (const Ref<Entity>& entity_light : World::GetEntitiesLights())
            {
                Light* light = entity_light->GetComponent<Light>();
                if (!light->GetFlag(LightFlags::Shadows) || light->GetIntensityWatt() == 0.0f)
                    continue;
    
                // set rasterizer state
                RHI_RasterizerState* new_state = (light->GetLightType() == LightType::Directional) ? GetRasterizerState(Renderer_RasterizerState::Light_directional) : GetRasterizerState(Renderer_RasterizerState::Light_point_spot);
                if (pso.rasterizer_state != new_state)
                {
                    pso.rasterizer_state = new_state;
                    cmd_list->SetPipelineState(pso);
                }

                // iterate over slices (all lights are just texture arrays)
                for (uint32_t array_index = 0; array_index < light->GetSliceCount(); array_index++)
                {
                    // get atlas rectangle for this slice
                    const math::Rectangle& rect = light->GetAtlasRectangle(array_index);
                    if (!rect.IsDefined()) // can happen if there is no more atlas space
                        continue;

                    // set atlas rectangle as viewport and scissor
                    cmd_list->SetViewport(Viewport(rect.x, rect.y, rect.width, rect.height));
                    cmd_list->SetScissorRectangle(rect);

                    // render cached renderables
                    for (uint32_t i = 0; i < m_draw_call_count; i++)
                    {
                        const Renderer_DrawCall& draw_call = m_draw_calls[i];
                        Renderable* renderable             = draw_call.renderable;
                        Material* material                 = renderable->GetMaterial();
                        const float shadow_distance        = renderable->GetMaxShadowDistance();
                        if (!material || material->IsTransparent() || !renderable->HasFlag(RenderableFlags::CastsShadows) || draw_call.distance_squared > shadow_distance * shadow_distance)
                            continue;

                        // TODO: this needs to be recalculated only when the light or the renderable moves, not every frame
                        if (!light->IsInViewFrustum(renderable, array_index, draw_call.instance_group_index))
                            continue;

                        // pixel shader
                        {
                            bool is_first_cascade = array_index == 0 && light->GetLightType() == LightType::Directional;
                            bool is_alpha_tested  = material->IsAlphaTested();
                            Shader* ps        = (is_first_cascade && is_alpha_tested) ? GetShader(Renderer_Shader::depth_light_alpha_color_p) : nullptr;
                        
                            if (pso.shaders[ShaderType::Fragment] != ps)
                            {
                                pso.shaders[ShaderType::Fragment] = ps;
                                cmd_list->SetPipelineState(pso);

                                // if the pipeline changed, set the viewport and scissor again
                                cmd_list->SetViewport(Viewport(rect.x, rect.y, rect.width, rect.height));
                                cmd_list->SetScissorRectangle(rect);
                            }
                        }

                        // push constants
                        m_pcb_pass_cpu.transform = renderable->GetEntity()->GetMatrix();
                        m_pcb_pass_cpu.set_f3_value(material->HasTextureOfType(MaterialTextureType::Color) ? 1.0f : 0.0f);
                        m_pcb_pass_cpu.set_f3_value2(static_cast<float>(light->GetIndex()), static_cast<float>(array_index), 0.0f);
                        m_pcb_pass_cpu.set_is_transparent_and_material_index(false, material->GetIndex());
                        cmd_list->PushConstants(m_pcb_pass_cpu);
    
                        // draw
                        {
                            cmd_list->SetCullMode(static_cast<RHI_CullMode>(material->GetProperty(MaterialProperty::CullMode)));
                            cmd_list->SetBufferVertex(renderable->GetVertexBuffer(), renderable->GetInstanceBuffer());
                            cmd_list->SetBufferIndex(renderable->GetIndexBuffer());

                            // bias the lod index to improve performance (for non-directional lights)
                            const uint32_t lod_bias = light->GetLightType() == LightType::Directional ? 0 : 1;
                            uint32_t lod_index      = Math::Clamp<uint32_t>(draw_call.lod_index + lod_bias, 0, renderable->GetLodCount() - 1);

                            cmd_list->DrawIndexed(
                                renderable->GetIndexCount(lod_index),
                                renderable->GetIndexOffset(lod_index),
                                renderable->GetVertexOffset(lod_index),
                                renderable->HasInstancing() ? draw_call.instance_index : 0,
                                renderable->HasInstancing() ? draw_call.instance_count : 1
                            );
                        }
                    }
                }
            }
        }
        cmd_list->EndTimeblock();
    }
}

/// -----------------------------------------------------

/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* icons.cpp
* -------------------------------------------------------
* Created: 29/8/2025
* -------------------------------------------------------
*/
//#include <SceneryEditorX/renderer/command_manager.h>
//#include <SceneryEditorX/renderer/render_pass.h>
//#include <SceneryEditorX/renderer/texture.h>
//#include <SceneryEditorX/scene/entity.h>
//#include <SceneryEditorX/scene/lights.h>

/// -------------------------------------------------------

/*
namespace SceneryEditorX
{
	
    void RenderPass::Pass_Icons(CommandManager *cmd_list, Texture *tex_out)
    {
        // append icons from entities
        if (!Engine::IsFlagSet(EngineMode::Playing))
        { 
            for (const Ref<Entity>& entity : World::GetEntities())
            {
                Vec3 pos_world = entity->GetPosition();
                if (entity->GetComponent<AudioSource>())
                {
                    if (GetOption<bool>(Renderer_Option::AudioSources))
                    {
                        m_icons.emplace_back(make_tuple(GetStandardTexture(Renderer_StandardTexture::Gizmo_audio_source), entity->GetPosition()));
                    }
                }
                else if (Light* light = entity->GetComponent<Light>())
                {
                    if (GetOption<bool>(Renderer_Option::Lights))
                    {
                        // append light icon based on type
                        Texture* texture = nullptr;
                        if (light->GetLightType() == LightType::Directional)
                            texture = GetStandardTexture(Renderer_StandardTexture::Gizmo_light_directional);
                        else if (light->GetLightType() == LightType::Point)
                            texture = GetStandardTexture(Renderer_StandardTexture::Gizmo_light_point);
                        else if (light->GetLightType() == LightType::Spot)
                            texture = GetStandardTexture(Renderer_StandardTexture::Gizmo_light_spot);

                        if (texture)
                        {
                            m_icons.emplace_back(std::make_tuple(texture, entity->GetPosition()));
                        }
                    }
                }
            }
        }

        if (!m_icons.empty())
        { 
            cmd_list->BeginTimeblock("icons");
            {
                // set pipeline state
                RHI_PipelineState pso;
                pso.name                              = "icons";
                pso.shaders[ShaderType::Compute] = GetShader(Renderer_Shader::icon_c);
                cmd_list->SetPipelineState(pso);

                // bind output texture
                cmd_list->SetTexture(Renderer_BindingsUav::tex, tex_out);

                // lambda to dispatch a single icon
                auto dispatch_icon = [&](Texture* texture, const Vec3& pos_world)
                {
                     // set push constants: world position and texture dimensions
                    m_pcb_pass_cpu.set_f3_value(pos_world.x, pos_world.y, pos_world.z);
                    m_pcb_pass_cpu.set_f2_value(static_cast<float>(texture->GetWidth()), static_cast<float>(texture->GetHeight()));
                    cmd_list->PushConstants(m_pcb_pass_cpu);

                    // bind icon texture
                    cmd_list->SetTexture(Renderer_BindingsSrv::tex, texture);

                    // dispatch compute shader
                    uint32_t thread_x = 32;
                    uint32_t thread_y = 32;
                    uint32_t groups_x = (texture->GetWidth() + thread_x - 1) / thread_x;  // ceil(width / thread_x)
                    uint32_t groups_y = (texture->GetHeight() + thread_y - 1) / thread_y; // ceil(height / thread_y)
                    cmd_list->Dispatch(groups_x, groups_y, 1);

                    // this is to avoid out of order UAV access and flickering overlapping icons
                    // ideally, we batch all the icons in one buffer and do a single dispatch, but for now this works
                    cmd_list->InsertBarrierReadWrite(tex_out, RHI_BarrierType::EnsureWriteThenRead);
                };

                // dispatch all icons in m_icons
                for (const auto& [texture, pos_world] : m_icons)
                {
                    if (texture)
                    {
                        dispatch_icon(texture, pos_world);
                    }
                }
            }

            cmd_list->EndTimeblock();
        }
    }

}
*/

/// -------------------------------------------------------

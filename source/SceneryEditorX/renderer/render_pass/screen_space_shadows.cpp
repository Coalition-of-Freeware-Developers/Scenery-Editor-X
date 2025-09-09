/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* screen_space_shadows.cpp
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
	
    void RenderPass::Pass_ScreenSpaceShadows(CommandManager *cmd_list)
    {
        // get resources
        Texture* tex_sss = GetRenderTarget(Renderer_RenderTarget::sss);

        cmd_list->BeginTimeblock("screen_space_shadows");
        {
            cmd_list->InsertBarrierReadWrite(tex_sss, RHI_BarrierType::EnsureReadThenWrite); // ensure any previous reads are complete

            // set pipeline state
            RHI_PipelineState pso;
            pso.name             = "screen_space_shadows";
            pso.shaders[Compute] = GetShader(Renderer_Shader::sss_c_bend);
            cmd_list->SetPipelineState(pso);

            // set textures
            cmd_list->SetTexture(Renderer_BindingsSrv::tex,     GetRenderTarget(Renderer_RenderTarget::gbuffer_depth)); // read
            cmd_list->SetTexture(Renderer_BindingsUav::tex_sss, tex_sss);                                               // write

            // iterate through all the lights
            static float array_slice_index = 0.0f;
            for (const Ref<Entity>& entity : World::GetEntities())
            {
                if (Light* light = entity->GetComponent<Light>())
                {
                    if (!light->GetFlag(LightFlags::ShadowsScreenSpace) || light->GetIntensityWatt() == 0.0f)
                        continue;

                    if (array_slice_index == tex_sss->GetDepth())
                    {
                        SEDX_CORE_WARN_TAG("Render Pass","Render target has reached the maximum number of lights it can hold");
                        break;
                    }

                    math::Matrix view_projection = World::GetCamera()->GetViewProjectionMatrix();
                    Vec4 p = {};
                    if (light->GetLightType() == LightType::Directional)
                    {
                        // TODO: Why do we need to flip sign?
                        p = Vec4(-light->GetEntity()->GetForward(), 0.0f) * view_projection;
                    }
                    else
                    {
                        p = Vec4(light->GetEntity()->GetPosition(), 1.0f) * view_projection;
                    }

                    float in_light_projection[]      = { p.x, p.y, p.z, p.w };
                    int32_t in_viewport_size[]       = { static_cast<int32_t>(tex_sss->GetWidth()), static_cast<int32_t>(tex_sss->GetHeight()) };
                    int32_t in_min_render_bounds[]   = { 0, 0 };
                    int32_t in_max_render_bounds[]   = { static_cast<int32_t>(tex_sss->GetWidth()), static_cast<int32_t>(tex_sss->GetHeight()) };
                    Bend::DispatchList dispatch_list = Bend::BuildDispatchList(in_light_projection, in_viewport_size, in_min_render_bounds, in_max_render_bounds, false);

                    m_pcb_pass_cpu.set_f4_value
                    (
                        dispatch_list.LightCoordinate_Shader[0],
                        dispatch_list.LightCoordinate_Shader[1],
                        dispatch_list.LightCoordinate_Shader[2],
                        dispatch_list.LightCoordinate_Shader[3]
                    );

                    // light index writes into the texture array index
                    light->SetScreenSpaceShadowsSliceIndex(static_cast<uint32_t>(array_slice_index));
                    float near = 1.0f;
                    float far  = 0.0f;
                    m_pcb_pass_cpu.set_f3_value(near, far, array_slice_index++);
                    m_pcb_pass_cpu.set_f3_value2(1.0f / tex_sss->GetWidth(), 1.0f / tex_sss->GetHeight(), 0.0f);

                    for (int32_t dispatch_index = 0; dispatch_index < dispatch_list.DispatchCount; ++dispatch_index)
                    {
                        const Bend::DispatchData& dispatch = dispatch_list.Dispatch[dispatch_index];
                        m_pcb_pass_cpu.set_f2_value(static_cast<float>(dispatch.WaveOffset_Shader[0]), static_cast<float>(dispatch.WaveOffset_Shader[1]));
                        cmd_list->PushConstants(m_pcb_pass_cpu);
                        cmd_list->Dispatch(dispatch.WaveCount[0], dispatch.WaveCount[1], dispatch.WaveCount[2]);
                    }

                    cmd_list->InsertBarrierReadWrite(tex_sss, RHI_BarrierType::EnsureWriteThenRead); // ensure the texture is ready for the next light
                }
            }

            array_slice_index = 0;
        }

        cmd_list->EndTimeblock();
    }
}
*/

/// -------------------------------------------------------

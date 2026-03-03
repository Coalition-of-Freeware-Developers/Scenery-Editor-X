/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * renderer_passes.cpp
 * -------------------------------------------------------
 * Created: 13/02/2026
 * -------------------------------------------------------
 */
//#include "vulkan/command_list.h"
//#include "vulkan/image.h"
//#include "vulkan/renderer.h"
//#include <colors.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
    /*
    std::array<Renderer_DrawCall, renderer_max_draw_calls> Renderer::m_draw_calls;
    uint32_t Renderer::m_draw_call_count;
    std::array<Renderer_DrawCall, renderer_max_draw_calls> Renderer::m_draw_calls_prepass;
    uint32_t Renderer::m_draw_calls_prepass_count;
    std::array<Sb_IndirectDrawArgs, rhi_max_array_size> Renderer::m_indirect_draw_args;
    std::array<Sb_DrawData, rhi_max_array_size> Renderer::m_indirect_draw_data;
    uint32_t Renderer::m_indirect_draw_count = 0;
    */

    /*
    void Renderer::SetStandardResources(CommandList* cmdList)
    {
        cmdList->SetConstantBuffer(Renderer_BindingsCb::frame, GetBuffer(Renderer_Buffer::ConstantFrame));
        cmdList->SetTexture(Renderer_BindingsSrv::tex_perlin, GetStandardTexture(Renderer_StandardTexture::Noise_perlin));
    }
    */

    /*
    void Renderer::ProduceFrame(CommandList* cmd_list_graphics_present, CommandList* cmd_list_compute)
    {
        SP_PROFILE_CPU();

        for (const auto& shader : GetShaders())
        {
            if (!shader || !shader->IsCompiled())
                return;
        }

        Image* rt_render = GetRenderTarget(RendererRenderTarget::frame_render);
        Image* rt_output = GetRenderTarget(RendererRenderTarget::frame_output);

        // brdf lut (once)
        if (!m_pass_state.brdf_lut_produced)
        {
            Pass_Lut_BrdfSpecular(cmd_list_graphics_present);
            m_pass_state.brdf_lut_produced = true;
        }

        // cloud noise (once)
        Pass_CloudNoise(cmd_list_graphics_present);

        // skysphere (re-render on light/coverage changes, converge over several frames)
        bool clouds_visible = cvar_cloud_coverage.GetValue() > 0.0f;
        
        {
            bool update_skysphere = false;
            Light* directional_light = World::GetDirectionalLight();
            
            {
                const uint32_t temporal_convergence_frames = 8;
                
                bool has_directional_light = directional_light != nullptr;
                float current_coverage = cvar_cloud_coverage.GetValue();
                
                bool light_changed = (has_directional_light && directional_light->NeedsSkysphereUpdate()) || 
                                     (has_directional_light != m_pass_state.sky_had_directional_light);
                bool cloud_params_changed = current_coverage != m_pass_state.sky_last_coverage;
                
                if (m_pass_state.sky_first_frame || light_changed || cloud_params_changed)
                {
                    m_pass_state.sky_frames_remaining = temporal_convergence_frames;
                }
                
                update_skysphere = m_pass_state.sky_frames_remaining > 0;
                
                if (m_pass_state.sky_frames_remaining > 0)
                {
                    m_pass_state.sky_frames_remaining--;
                }
                
                m_pass_state.sky_first_frame           = false;
                m_pass_state.sky_had_directional_light = has_directional_light;
                m_pass_state.sky_last_coverage         = current_coverage;
            }
            
            if (update_skysphere)
            {
                // lut (expensive, only on light change)
                if (!m_pass_state.atmosphere_lut_produced || (directional_light && directional_light->NeedsSkysphereUpdate()))
                {
                    Pass_Lut_AtmosphericScattering(cmd_list_graphics_present);
                    m_pass_state.atmosphere_lut_produced = true;
                }
                Pass_Skysphere(cmd_list_graphics_present);
            }
        }

        if (Camera* camera = World::GetCamera())
        {
            Pass_VariableRateShading(cmd_list_graphics_present);

            // graphics phase 1: geometry
            {
                bool is_transparent = false;
                Pass_HiZ(cmd_list_graphics_present);
                Pass_IndirectCull(cmd_list_graphics_present);
                Pass_Depth_Prepass(cmd_list_graphics_present);
                Pass_GBuffer(cmd_list_graphics_present, is_transparent);
            }

            // transition g-buffer to shader-readable before submit
            GetRenderTarget(RendererRenderTarget::gbuffer_color)->SetLayout(Layout::ImageLayout::ShaderRead, cmd_list_graphics_present);
            GetRenderTarget(RendererRenderTarget::gbuffer_normal)->SetLayout(Layout::ImageLayout::ShaderRead, cmd_list_graphics_present);
            GetRenderTarget(RendererRenderTarget::gbuffer_material)->SetLayout(Layout::ImageLayout::ShaderRead, cmd_list_graphics_present);
            GetRenderTarget(RendererRenderTarget::gbuffer_velocity)->SetLayout(Layout::ImageLayout::ShaderRead, cmd_list_graphics_present);
            GetRenderTarget(RendererRenderTarget::gbuffer_depth)->SetLayout(Layout::ImageLayout::ShaderRead, cmd_list_graphics_present);

            // submit phase 1, signal g-buffer ready
            cmd_list_graphics_present->Submit(nullptr, false);
            uint64_t gfx_phase1_timeline_value = cmd_list_graphics_present->GetLastTimelineSignalValue();
            FrameSync* gfx_timeline    = cmd_list_graphics_present->GetTimelineSemaphore();

            // async compute: overlaps with shadow rasterization
            {
                if (clouds_visible)
                {
                    Pass_CloudShadow(cmd_list_compute);
                }

                Pass_ScreenSpaceAmbientOcclusion(cmd_list_compute);
                Pass_ScreenSpaceShadows(cmd_list_compute);

                Pass_RayTracedShadows(cmd_list_compute);
                Pass_ReSTIR_PathTracing(cmd_list_compute);

                // submit compute, wait on phase 1
                cmd_list_compute->Submit(nullptr, false, nullptr, gfx_timeline, gfx_phase1_timeline_value);
            }
            uint64_t compute_timeline_value = cmd_list_compute->GetLastTimelineSignalValue();
            FrameSync* compute_timeline = cmd_list_compute->GetTimelineSemaphore();

            // graphics phase 2: shadow maps
            Ref<Queue>* queue_graphics = QueueManager::GetQueue(QueueType::Graphics);
            cmd_list_graphics_present = queue_graphics->NextCommandList();
            cmd_list_graphics_present->Begin();
            m_CmdList_Present = cmd_list_graphics_present;

            Pass_ShadowMaps(cmd_list_graphics_present);

            // graphics phase 3: lighting and post-process (waits on compute)
            cmd_list_graphics_present->Submit(nullptr, false, nullptr, compute_timeline, compute_timeline_value);

            cmd_list_graphics_present = queue_graphics->NextCommandList();
            cmd_list_graphics_present->Begin();
            m_CmdList_Present = cmd_list_graphics_present;

            {
                bool is_transparent = false;
                Pass_Light(cmd_list_graphics_present, is_transparent);
                Pass_Light_Composition(cmd_list_graphics_present, is_transparent);
                cmd_list_graphics_present->Blit(GetRenderTarget(RendererRenderTarget::frame_render), GetRenderTarget(RendererRenderTarget::frame_render_opaque), false);
            }

            // particles
            Pass_Particles(cmd_list_graphics_present);

            // transparents
            if (m_transparents_present)
            {
                bool is_transparent = true;
                Pass_GBuffer(cmd_list_graphics_present, is_transparent);
                Pass_Light(cmd_list_graphics_present, is_transparent);
                Pass_Light_Composition(cmd_list_graphics_present, is_transparent);
            }

            Pass_Light_ImageBased(cmd_list_graphics_present);
            
            // rt reflections (after transparents so depth includes glass)
            Pass_RayTracedReflections(cmd_list_graphics_present);
            Pass_Light_Reflections(cmd_list_graphics_present);
            
            Pass_TransparencyReflectionRefraction(cmd_list_graphics_present);
            Pass_AA_Upscale(cmd_list_graphics_present);
            Pass_PostProcess(cmd_list_graphics_present);
        }
        else
        {
            cmd_list_graphics_present->ClearTexture(rt_output, Color::Black);
        }

        Pass_Text(cmd_list_graphics_present, rt_output);

        // early transitions for next frame
        rt_output->SetLayout(Layout::ImageLayout::ShaderRead, cmd_list_graphics_present);
        GetRenderTarget(RendererRenderTarget::gbuffer_color)->SetLayout(Layout::ImageLayout::Attachment, cmd_list_graphics_present);
        GetRenderTarget(RendererRenderTarget::gbuffer_normal)->SetLayout(Layout::ImageLayout::Attachment, cmd_list_graphics_present);
        GetRenderTarget(RendererRenderTarget::gbuffer_material)->SetLayout(Layout::ImageLayout::Attachment, cmd_list_graphics_present);
        GetRenderTarget(RendererRenderTarget::gbuffer_velocity)->SetLayout(Layout::ImageLayout::Attachment, cmd_list_graphics_present);
        GetRenderTarget(RendererRenderTarget::gbuffer_depth)->SetLayout(Layout::ImageLayout::Attachment, cmd_list_graphics_present);
    }
    */


}

// -------------------------------------------------------

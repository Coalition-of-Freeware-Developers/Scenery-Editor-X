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
#include "renderer.h"
#include "renderer_declarations.h"
#include "bend_sss_cpu.h"
#include "SceneryEditorX/scene/lights.h"
#include "vulkan/enums.h"
#include "vulkan/render_context.h"
#include <SceneryEditorX/scene/camera.h>
#include <SceneryEditorX/scene/scene.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
    // Static draw call array definitions
    std::array<Renderer_DrawCall, renderer_max_draw_calls> Renderer::m_draw_calls;
    std::array<Renderer_DrawCall, renderer_max_draw_calls> Renderer::m_draw_calls_prepass;

    // Console variable stubs – replace with real CVar system when available
    static CVar cvar_wireframe            { 0.0f };
    static CVar cvar_grid                 { 0.0f };
    static CVar cvar_resolution_scale     { 1.0f };
    static CVar cvar_variable_rate_shading{ 0.0f };
    static CVar cvar_hiz_occlusion        { 1.0f };
    static CVar cvar_ssao                 { 1.0f };
    static CVar cvar_tonemapping          { 0.0f };
    static CVar cvar_auto_exposure_adaptation_speed{ 1.0f };
    static CVar cvar_cloud_coverage       { 0.45f };

    void Renderer::ProduceFrame(CommandList* graphicsPresent, CommandList* compute)
    {
        for (const auto& shader : GetShaders())
        {
            if (!shader)
                return;
        }

        ImageResource* rt_render = GetRenderTarget(Renderer_RenderTarget::frame_render);
        ImageResource* rt_output = GetRenderTarget(Renderer_RenderTarget::frame_output);

        // brdf lut (once)
        if (!m_PassState.m_BRDF_LutProduced)
        {
            Pass_Lut_BrdfSpecular(graphicsPresent);
            m_PassState.m_BRDF_LutProduced = true;
        }

        // cloud noise (once)
        Pass_CloudNoise(graphicsPresent);

        // skysphere (re-render on light/coverage changes, converge over several frames)
        bool cloudsVisible = 0.45f > 0.0f;
        
        /*
        {
            bool updateSkysphere = false;
            Light* directionalLight = Scene::GetDirectionalLight();
            
            {
                const uint32_t temporalConvergenceFrames = 8;
                
                bool hasDirectionalLight = directionalLight != nullptr;
                float currentCoverage = cvar_cloud_coverage.GetValue();
                
                bool lightChanged = (hasDirectionalLight && directionalLight->NeedsSkysphereUpdate()) || 
                                     (hasDirectionalLight != m_PassState.m_SkyHadDirectionalLight);
                bool cloudParamsChanged = currentCoverage != m_PassState.m_SkyLastCoverage;
                
                if (m_PassState.m_SkyFirstFrame || lightChanged || cloudParamsChanged)
                {
                    m_PassState.m_SkyFramesRemaining = temporalConvergenceFrames;
                }
                
                updateSkysphere = m_PassState.m_SkyFramesRemaining > 0;
                
                if (m_PassState.m_SkyFramesRemaining > 0)
                {
                    m_PassState.m_SkyFramesRemaining--;
                }
                
                m_PassState.m_SkyFirstFrame           = false;
                m_PassState.m_SkyHadDirectionalLight = hasDirectionalLight;
                m_PassState.m_SkyLastCoverage         = currentCoverage;
            }
            
            if (updateSkysphere)
            {
                // lut (expensive, only on light change)
                if (!m_PassState.m_Atmosphere_LutProduced || (directionalLight && directionalLight->NeedsSkysphereUpdate()))
                {
                    Pass_Lut_AtmosphericScattering(graphicsPresent);
                    m_PassState.m_Atmosphere_LutProduced = true;
                }

                Pass_Skysphere(graphicsPresent);
            }
        }
        */

        if (Camera* camera = Scene::GetCamera())
        {
            Pass_VariableRateShading(graphicsPresent);

            // graphics phase 1: geometry
            {
                bool isTransparent = false;
                Pass_HiZ(graphicsPresent);
                Pass_IndirectCull(graphicsPresent);
                Pass_Depth_Prepass(graphicsPresent);
                Pass_GBuffer(graphicsPresent, isTransparent);
            }

            // transition g-buffer to shader-readable before submit
            GetRenderTarget(Renderer_RenderTarget::gbuffer_color)->SetLayout(Layout::ImageLayout::ShaderRead, graphicsPresent, 0, 0);
            GetRenderTarget(Renderer_RenderTarget::gbuffer_normal)->SetLayout(Layout::ImageLayout::ShaderRead, graphicsPresent, 0, 0);
            GetRenderTarget(Renderer_RenderTarget::gbuffer_material)->SetLayout(Layout::ImageLayout::ShaderRead, graphicsPresent, 0, 0);
            GetRenderTarget(Renderer_RenderTarget::gbuffer_velocity)->SetLayout(Layout::ImageLayout::ShaderRead, graphicsPresent, 0, 0);
            GetRenderTarget(Renderer_RenderTarget::gbuffer_depth)->SetLayout(Layout::ImageLayout::ShaderRead, graphicsPresent, 0, 0);

            // submit phase 1, signal g-buffer ready
            graphicsPresent->Submit(nullptr, false);
            uint64_t gfxPhase1TimelineValue = graphicsPresent->GetLastTimelineSignalValue();
            FrameSync* gfxTimeline    = graphicsPresent->GetTimelineSemaphore();

            // async compute: overlaps with shadow rasterization
            {
                /*if (cloudsVisible)
                {
                    Pass_CloudShadow(compute);
                }*/

                Pass_ScreenSpaceAO(compute);
                Pass_ScreenSpaceShadows(compute);

                // submit compute, wait on phase 1
                compute->Submit(nullptr, false, nullptr, gfxTimeline, gfxPhase1TimelineValue);
            }
            uint64_t computeTimelineValue = compute->GetLastTimelineSignalValue();
            FrameSync* computeTimeline = compute->GetTimelineSemaphore();

            // graphics phase 2: shadow maps
            Ref<QueueManager> queueManager = RenderContext::Get()->GetDevice()->GetQueueManager();
            graphicsPresent = queueManager->NextCommandList();
            graphicsPresent->Begin();
            m_CmdList_Present = graphicsPresent;

            Pass_ShadowMaps(graphicsPresent);

            // graphics phase 3: lighting and post-process (waits on compute)
            graphicsPresent->Submit(nullptr, false, nullptr, computeTimeline, computeTimelineValue);

            graphicsPresent = queueManager->NextCommandList();
            graphicsPresent->Begin();
            m_CmdList_Present = graphicsPresent;

            {
                bool isTransparent = false;
                Pass_Light(graphicsPresent, isTransparent);
                Pass_Light_Composition(graphicsPresent, isTransparent);
                graphicsPresent->Blit(GetRenderTarget(Renderer_RenderTarget::frame_render), GetRenderTarget(Renderer_RenderTarget::frame_render_opaque), false);
            }

            // particles
            Pass_Particles(graphicsPresent);

            // transparents
            if (m_transparents_present)
            {
                bool isTransparent = true;
                Pass_GBuffer(graphicsPresent, isTransparent);
                Pass_Light(graphicsPresent, isTransparent);
                Pass_Light_Composition(graphicsPresent, isTransparent);
            }

            Pass_Light_ImageBased(graphicsPresent);

            Pass_Light_Reflections(graphicsPresent);
            
            Pass_TransparencyReflectionRefraction(graphicsPresent);
            Pass_AA_Upscale(graphicsPresent);
            Pass_PostProcess(graphicsPresent);
        }
        else
        {
            graphicsPresent->ClearTexture(rt_output, Color::Black());
        }

        Pass_Text(graphicsPresent, rt_output);

        // early transitions for next frame
        rt_output->SetLayout(Layout::ImageLayout::ShaderRead, graphicsPresent, 0, 0);
        GetRenderTarget(Renderer_RenderTarget::gbuffer_color)->SetLayout(Layout::ImageLayout::Attachment, graphicsPresent, 0, 0);
        GetRenderTarget(Renderer_RenderTarget::gbuffer_normal)->SetLayout(Layout::ImageLayout::Attachment, graphicsPresent, 0, 0);
        GetRenderTarget(Renderer_RenderTarget::gbuffer_material)->SetLayout(Layout::ImageLayout::Attachment, graphicsPresent, 0, 0);
        GetRenderTarget(Renderer_RenderTarget::gbuffer_velocity)->SetLayout(Layout::ImageLayout::Attachment, graphicsPresent, 0, 0);
        GetRenderTarget(Renderer_RenderTarget::gbuffer_depth)->SetLayout(Layout::ImageLayout::Attachment, graphicsPresent, 0, 0);
    }

    void Renderer::Pass_Skysphere(CommandList *cmdList)
	{
        ImageResource* tex_skysphere                    = GetRenderTarget(Renderer_RenderTarget::skysphere);
        ImageResource* tex_lut_atmosphere_scatter       = GetRenderTarget(Renderer_RenderTarget::lut_atmosphere_scatter);
        ImageResource* tex_lut_atmosphere_transmittance = GetRenderTarget(Renderer_RenderTarget::lut_atmosphere_transmittance);
        ImageResource* tex_lut_atmosphere_multiscatter  = GetRenderTarget(Renderer_RenderTarget::lut_atmosphere_multiscatter);
        ImageResource* tex_cloud_shape                  = GetRenderTarget(Renderer_RenderTarget::cloud_noise_shape);
        ImageResource* tex_cloud_detail                 = GetRenderTarget(Renderer_RenderTarget::cloud_noise_detail);

        if (Scene::GetDirectionalLight())
        {
            PipelineState pso;
            pso.name             = "skysphere_atmospheric_scattering";
            pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::skysphere_c);
            cmdList->SetPipelineState(pso);
    
            cmdList->SetTexture(Renderer_BindingsUav::tex, tex_skysphere);
            cmdList->SetTexture(Renderer_BindingsSrv::tex, tex_lut_atmosphere_transmittance);
            cmdList->SetTexture(Renderer_BindingsSrv::tex2, tex_lut_atmosphere_multiscatter);
            cmdList->SetTexture(Renderer_BindingsSrv::tex3d, tex_lut_atmosphere_scatter);
            
            if (tex_cloud_shape)
            {
                cmdList->SetTexture(Renderer_BindingsSrv::tex3d_cloud_shape, tex_cloud_shape);
            }
            if (tex_cloud_detail)
            {
                cmdList->SetTexture(Renderer_BindingsSrv::tex3d_cloud_detail, tex_cloud_detail);
            }
            
            cmdList->Dispatch(tex_skysphere);
        }
        else
        {
            cmdList->ClearTexture(tex_skysphere, Color::Black());
        }

        // filter all mip levels
        {
            Pass_Downscale(cmdList, tex_skysphere, Renderer_DownsampleFilter::Average);

            PipelineState pso;
            pso.name             = "skysphere_filter";
            pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::light_integration_environment_filter_c);
            cmdList->SetPipelineState(pso);

            cmdList->SetTexture(Renderer_BindingsSrv::tex, tex_skysphere);

            for (uint32_t mip_level = 1; mip_level < tex_skysphere->GetImageSpec().mipCount; mip_level++)
            {
                cmdList->SetTexture(Renderer_BindingsUav::tex, tex_skysphere, mip_level, 1);

                m_pcb_pass_cpu.set_f3_value(static_cast<float>(mip_level), static_cast<float>(tex_skysphere->GetImageSpec().mipCount), 0.0f);
                cmdList->PushConstants(m_pcb_pass_cpu);

                const uint32_t resolutionX = tex_skysphere->GetWidth() >> mip_level;
                const uint32_t resolutionY = tex_skysphere->GetHeight() >> mip_level;
                cmdList->Dispatch(tex_skysphere);
                cmdList->InsertBarrier(tex_skysphere, BarrierType::EnsureWriteThenRead);
            }
        }
	}

    void Renderer::Pass_Output(CommandList* cmdList, ImageResource* in, ImageResource* out)
    {
        Shader* shader_c = GetShader(Renderer_Shader::output_c);

        PipelineState pso;
        pso.name             = "output";
        pso.shaders[static_cast<uint32_t>(Stage::Compute)] = shader_c;
        cmdList->SetPipelineState(pso);

        m_pcb_pass_cpu.set_f3_value(cvar_tonemapping.GetValue(), cvar_auto_exposure_adaptation_speed.GetValue(), 0.0f);
        cmdList->PushConstants(m_pcb_pass_cpu);

        cmdList->SetTexture(Renderer_BindingsUav::tex, out);
        cmdList->SetTexture(Renderer_BindingsSrv::tex, in);
        cmdList->SetTexture(Renderer_BindingsSrv::tex2, GetRenderTarget(Renderer_RenderTarget::auto_exposure));
        cmdList->Dispatch(out);

    }

    template<typename F>
    void Renderer::Pass_Compute(CommandList* cmdList, const char* name, Renderer_Shader shaderEnum, ImageResource* in, ImageResource* out, F setup)
    {
        {
            PipelineState pso;
            pso.name             = name;
            pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(shaderEnum);
            cmdList->SetPipelineState(pso);

            cmdList->SetTexture(Renderer_BindingsSrv::tex, in);
            cmdList->SetTexture(Renderer_BindingsUav::tex, out);

            if constexpr (!std::is_null_pointer_v<F>)
            {
                setup();
            }

            cmdList->Dispatch(out);
        }
    }

    void Renderer::Pass_Grid(CommandList* cmdList, ImageResource* out)
    {
        if (!cvar_grid.GetValueAs<bool>())
            return;

        Shader* shader_v = GetShader(Renderer_Shader::grid_v);
        Shader* shader_p = GetShader(Renderer_Shader::grid_p);

        PipelineState pso;
        pso.name                             = "grid";
        pso.shaders[static_cast<uint32_t>(Stage::Vertex)] = shader_v;
        pso.shaders[static_cast<uint32_t>(Stage::Fragment)]  = shader_p;
        pso.rasterizer_state                 = GetRasterizerState(Renderer_RasterizerState::Solid);
        pso.blend_state                      = GetBlendState(Renderer_BlendState::Alpha);
        pso.depth_stencil_state              = GetDepthStencilState(Renderer_DepthStencilState::ReadGreaterEqual);
        pso.render_target_color_textures[0]  = out;
        pso.render_target_depth_texture      = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth_opaque_output);
        cmdList->SetPipelineState(pso);

        // follow camera in Scene-unit increments so the grid appears stationary
        {
            const float gridSpacing     = 1.0f;
            Camera* cam = Scene::GetCamera();
            const Vec3 cameraPos        = cam ? cam->GetEntity()->GetPosition() : Vec3{};
            const Vec3 translation      = Vec3(
                floor(cameraPos.x / gridSpacing) * gridSpacing,
                0.0f,
                floor(cameraPos.z / gridSpacing) * gridSpacing
            );

            Matrix grid_transform       = Matrix{}.CreateScale(Vec3(1000.0f, 1.0f, 1000.0f)) * Matrix{}.CreateTranslation(translation);
            m_pcb_pass_cpu.draw_index = WriteDrawData(grid_transform);
            cmdList->PushConstants(m_pcb_pass_cpu);
        }

        cmdList->SetCullMode(CullMode::Back);
        cmdList->SetVertexBuffer(GetStandardMesh(MeshType::Quad)->GetVertexBuffer(), nullptr);
        cmdList->SetIndexBuffer(GetStandardMesh(MeshType::Quad)->GetIndexBuffer());
        cmdList->DrawIndexed(6, 1, GetStandardMesh(MeshType::Quad)->GetGlobalIndexOffset(), GetStandardMesh(MeshType::Quad)->GetGlobalVertexOffset());

    }

    void Renderer::Pass_Depth_Prepass(CommandList *cmdList)
    {
        ImageResource* tex_depth        = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth);
        ImageResource* tex_depth_output = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth_opaque_output);

        bool is_wireframe                     = cvar_wireframe.GetValueAs<bool>();
        RasterizerState* rasterizer_state = GetRasterizerState(Renderer_RasterizerState::Solid);
        rasterizer_state                      = is_wireframe ? GetRasterizerState(Renderer_RasterizerState::Wireframe) : rasterizer_state;

        {
            // indirect prepass (must match g-buffer indirect path)
            if (m_indirect_draw_count > 0)
            {
                PipelineState pso;
                pso.name                             = "depth_prepass_indirect";
                pso.shaders[static_cast<uint32_t>(Stage::Vertex)] = GetShader(Renderer_Shader::depth_prepass_indirect_v);
                pso.rasterizer_state                 = rasterizer_state;
                pso.blend_state                      = GetBlendState(Renderer_BlendState::Off);
                pso.depth_stencil_state              = GetDepthStencilState(Renderer_DepthStencilState::ReadWrite);
                pso.vrs_input_texture                = cvar_variable_rate_shading.GetValueAs<bool>() ? GetRenderTarget(Renderer_RenderTarget::shading_rate) : nullptr;
                pso.render_target_depth_texture      = tex_depth;
                pso.resolution_scale                 = true;
                pso.clear_depth                      = 0.0f;
                cmdList->SetPipelineState(pso);
                cmdList->SetIndexBuffer(GeometryBuffer::GetIndexBuffer());
                cmdList->SetBuffer(Renderer_BindingsUav::indirect_draw_data_out, GetBuffer(Renderer_Buffer::IndirectDrawDataOut));
                cmdList->SetCullMode(CullMode::Back);

                cmdList->DrawIndexedIndirectCount(
                    GetBuffer(Renderer_Buffer::IndirectDrawArgsOut),
                    0,
                    GetBuffer(Renderer_Buffer::IndirectDrawCount),
                    0,
                    m_indirect_draw_count
                );
            }

            // cpu-driven path for remaining draws (tessellated, instanced, alpha-tested)
            {
                PipelineState pso;
                pso.name                             = "depth_prepass";
                pso.shaders[static_cast<uint32_t>(Stage::Vertex)] = GetShader(Renderer_Shader::depth_prepass_v);
                pso.rasterizer_state                 = rasterizer_state;
                pso.blend_state                      = GetBlendState(Renderer_BlendState::Off);
                pso.depth_stencil_state              = GetDepthStencilState(Renderer_DepthStencilState::ReadWrite);
                pso.vrs_input_texture                = cvar_variable_rate_shading.GetValueAs<bool>() ? GetRenderTarget(Renderer_RenderTarget::shading_rate) : nullptr;
                pso.render_target_depth_texture      = tex_depth;
                pso.resolution_scale                 = true;
                pso.clear_depth                      = rhi_depth_load; // load since indirect already wrote depth

                bool pipeline_set = false;

                for (uint32_t i = 0; i < m_draw_calls_prepass_count; i++)
                {
                    const Renderer_DrawCall& draw_call = m_draw_calls_prepass[i];
                    Renderable* renderable             = draw_call.renderable;
                    Material* material                 = renderable->GetMaterial();
                    if (!material || material->IsTransparent() || !draw_call.camera_visible)
                        continue;

                    // skip indirect-path draws
                    if (!IsCpuDrivenDraw(draw_call, material))
                        continue;
                    {
                        bool is_alpha_tested = material->IsAlphaTested();
                        bool is_tessellated  = material->GetProperty(MaterialProperty::Tessellation) > 0.0f;
                        Shader* ps       = is_alpha_tested ? GetShader(Renderer_Shader::depth_prepass_alpha_test_p) : nullptr;
                        Shader* hs       = is_tessellated ? GetShader(Renderer_Shader::tessellation_h) : nullptr;
                        Shader* ds       = is_tessellated ? GetShader(Renderer_Shader::tessellation_d) : nullptr;

                        if (!pipeline_set || pso.shaders[static_cast<uint32_t>(Stage::Fragment)] != ps || pso.shaders[static_cast<uint32_t>(Stage::TessellationControl)] != hs || pso.shaders[static_cast<uint32_t>(Stage::TessellationEvaluation)] != ds)
                        {
                            pso.shaders[static_cast<uint32_t>(Stage::Fragment)]  = ps;
                            pso.shaders[static_cast<uint32_t>(Stage::TessellationControl)]   = hs;
                            pso.shaders[static_cast<uint32_t>(Stage::TessellationEvaluation)] = ds;
                            cmdList->SetPipelineState(pso);
                            pipeline_set = true;
                        }
                    }

                    {
                        bool has_color_texture = material->HasTextureOfType(MaterialTextureType::Color);
                        m_pcb_pass_cpu.draw_index = draw_call.draw_data_index;
                        m_pcb_pass_cpu.is_transparent = 0;
                        m_pcb_pass_cpu.material_index = material->GetIndex();
                        m_pcb_pass_cpu.set_f3_value(0.0f, has_color_texture ? 1.0f : 0.0f, static_cast<float>(i));
                        cmdList->PushConstants(m_pcb_pass_cpu);
                    }

                    {
                        CullMode cull_mode = static_cast<CullMode>(material->GetProperty(MaterialProperty::CullMode));
                        cull_mode              = (pso.rasterizer_state->GetPolygonMode() == PolygonMode::Wireframe) ? CullMode::None : cull_mode;
                        cmdList->SetCullMode(cull_mode);
                        cmdList->SetVertexBuffer(renderable->GetVertexBuffer(), renderable->GetInstanceBuffer());
                        cmdList->SetIndexBuffer(renderable->GetIndexBuffer());

                        cmdList->DrawIndexed(
                            renderable->GetIndexCount(draw_call.lod_index),
                            renderable->GetIndexOffset(draw_call.lod_index),
                            renderable->GetVertexOffset(draw_call.lod_index),
                            draw_call.instance_index,
                            draw_call.instance_count
                        );

                        pso.clear_depth = rhi_depth_load;
                    }
                }
            }

            float resolution_scale = cvar_resolution_scale.GetValue();
            cmdList->Blit(tex_depth, tex_depth_output, false, resolution_scale);

            // early transitions
            {
                tex_depth->SetLayout(Layout::ImageLayout::Attachment, cmdList, 0, 0);
                tex_depth_output->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
            }
        }

    }

    void Renderer::Pass_GBuffer(CommandList *cmdList, const bool isTransparentPass)
    {
        ImageResource* tex_color    = GetRenderTarget(Renderer_RenderTarget::gbuffer_color);
        ImageResource* tex_normal   = GetRenderTarget(Renderer_RenderTarget::gbuffer_normal);
        ImageResource* tex_material = GetRenderTarget(Renderer_RenderTarget::gbuffer_material);
        ImageResource* tex_velocity = GetRenderTarget(Renderer_RenderTarget::gbuffer_velocity);
        ImageResource* tex_depth    = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth);
    
        {
            if (!isTransparentPass && m_indirect_draw_count > 0)
            {
                PipelineState pso;
                pso.name                             = "g_buffer_indirect";
                pso.shaders[static_cast<uint32_t>(Stage::Vertex)] = GetShader(Renderer_Shader::gbuffer_indirect_v);
                pso.shaders[static_cast<uint32_t>(Stage::Fragment)]  = GetShader(Renderer_Shader::gbuffer_indirect_p);
                pso.blend_state                      = GetBlendState(Renderer_BlendState::Off);
                pso.rasterizer_state                 = cvar_wireframe.GetValueAs<bool>() ? GetRasterizerState(Renderer_RasterizerState::Wireframe) : GetRasterizerState(Renderer_RasterizerState::Solid);
                pso.depth_stencil_state              = GetDepthStencilState(Renderer_DepthStencilState::ReadGreaterEqual);
                pso.vrs_input_texture                = cvar_variable_rate_shading.GetValueAs<bool>() ? GetRenderTarget(Renderer_RenderTarget::shading_rate) : nullptr;
                pso.resolution_scale                 = true;
                pso.render_target_color_textures[0]  = tex_color;
                pso.render_target_color_textures[1]  = tex_normal;
                pso.render_target_color_textures[2]  = tex_material;
                pso.render_target_color_textures[3]  = tex_velocity;
                pso.render_target_depth_texture      = tex_depth;
                pso.clear_color[0]                   = PipelineStateColor{0.0f, 0.0f, 0.0f, 0.0f};
                pso.clear_color[1]                   = PipelineStateColor{0.0f, 0.0f, 0.0f, 0.0f};
                pso.clear_color[2]                   = PipelineStateColor{0.0f, 0.0f, 0.0f, 0.0f};
                pso.clear_color[3]                   = PipelineStateColor{0.0f, 0.0f, 0.0f, 0.0f};
                cmdList->SetPipelineState(pso);

                cmdList->SetIndexBuffer(GeometryBuffer::GetIndexBuffer());
                cmdList->SetBuffer(Renderer_BindingsUav::indirect_draw_data_out, GetBuffer(Renderer_Buffer::IndirectDrawDataOut));
                cmdList->SetCullMode(CullMode::Back);

                cmdList->DrawIndexedIndirectCount(
                    GetBuffer(Renderer_Buffer::IndirectDrawArgsOut),
                    0,
                    GetBuffer(Renderer_Buffer::IndirectDrawCount),
                    0,
                    m_indirect_draw_count
                );

                // update previous transforms for motion vectors
                for (uint32_t i = 0; i < m_draw_call_count; i++)
                {
                    Renderer_DrawCall& draw_call = m_draw_calls[i];
                    if (draw_call.renderable->GetMaterial() && !draw_call.renderable->GetMaterial()->IsTransparent())
                    {
                        Entity* entity = draw_call.renderable->GetEntity();
                        entity->SetMatrixPrevious(entity->GetMatrix());
                    }
                }
            }

            {
                PipelineState pso;
                pso.name                             = isTransparentPass ? "g_buffer_transparent" : "g_buffer_tessellated";
                pso.shaders[static_cast<uint32_t>(Stage::Vertex)]			 = GetShader(Renderer_Shader::gbuffer_v);
                pso.shaders[static_cast<uint32_t>(Stage::Fragment)]		 = GetShader(Renderer_Shader::gbuffer_p);
                pso.blend_state                      = GetBlendState(Renderer_BlendState::Off);
                pso.rasterizer_state                 = cvar_wireframe.GetValueAs<bool>() ? GetRasterizerState(Renderer_RasterizerState::Wireframe) : GetRasterizerState(Renderer_RasterizerState::Solid);
                pso.depth_stencil_state              = isTransparentPass ? GetDepthStencilState(Renderer_DepthStencilState::ReadWrite) : GetDepthStencilState(Renderer_DepthStencilState::ReadGreaterEqual);
                pso.vrs_input_texture                = cvar_variable_rate_shading.GetValueAs<bool>() ? GetRenderTarget(Renderer_RenderTarget::shading_rate) : nullptr;
                pso.resolution_scale                 = true;
                pso.render_target_color_textures[0]  = tex_color;
                pso.render_target_color_textures[1]  = tex_normal;
                pso.render_target_color_textures[2]  = tex_material;
                pso.render_target_color_textures[3]  = tex_velocity;
                pso.render_target_depth_texture      = tex_depth;
                pso.clear_color[0]                   = rhi_color_load;
                pso.clear_color[1]                   = rhi_color_load;
                pso.clear_color[2]                   = rhi_color_load;
                pso.clear_color[3]                   = rhi_color_load;

                bool pipeline_set = false;

                for (uint32_t i = 0; i < m_draw_call_count; i++)
                {
                    const Renderer_DrawCall& draw_call = m_draw_calls[i];
                    Renderable* renderable             = draw_call.renderable;
                    Material* material                 = renderable->GetMaterial();
                    if (!material || !draw_call.camera_visible)
                        continue;

                    if (isTransparentPass)
                    {
                        if (!material->IsTransparent())
                            continue;
                    }
                    else
                    {
                        if (material->IsTransparent())
                            continue;

                        if (!IsCpuDrivenDraw(draw_call, material))
                            continue;
                    }

                    {
                        bool is_tessellated = material->GetProperty(MaterialProperty::Tessellation) > 0.0f;
                        Shader* tessControl = is_tessellated ? GetShader(Renderer_Shader::tessellation_h) : nullptr;
                        Shader* tessEval    = is_tessellated ? GetShader(Renderer_Shader::tessellation_d) : nullptr;
                    
                        if (!pipeline_set || pso.shaders[static_cast<uint32_t>(Stage::TessellationControl)] != tessControl || pso.shaders[static_cast<uint32_t>(Stage::TessellationEvaluation)] != tessEval)
                        {
                            pso.shaders[static_cast<uint32_t>(Stage::TessellationControl)]   = tessControl;
                            pso.shaders[static_cast<uint32_t>(Stage::TessellationEvaluation)] = tessEval;
                            cmdList->SetPipelineState(pso);
                            pipeline_set = true;
                        }
                    }

                    {
                        Entity* entity = renderable->GetEntity();
                        m_pcb_pass_cpu.draw_index     = draw_call.draw_data_index;
                        m_pcb_pass_cpu.is_transparent = isTransparentPass ? 1 : 0;
                        m_pcb_pass_cpu.material_index = material->GetIndex();
                        cmdList->PushConstants(m_pcb_pass_cpu);

                        entity->SetMatrixPrevious(entity->GetMatrix());
                    }

                    {
                        cmdList->SetCullMode(cvar_wireframe.GetValueAs<bool>() ? CullMode::None : static_cast<CullMode>(material->GetProperty(MaterialProperty::CullMode)));
                        cmdList->SetVertexBuffer(renderable->GetVertexBuffer(), renderable->GetInstanceBuffer());
                        cmdList->SetIndexBuffer(renderable->GetIndexBuffer());

                        cmdList->DrawIndexed(
                            renderable->GetIndexCount(draw_call.lod_index),
                            renderable->GetIndexOffset(draw_call.lod_index),
                            renderable->GetVertexOffset(draw_call.lod_index),
                            draw_call.instance_index,
                            draw_call.instance_count
                        );

                        pso.clear_depth = depth_load;
                    }
                }
            }
    
            // early transitions
            tex_color->SetLayout(Layout::ImageLayout::General, cmdList, 0, 0);
            tex_normal->SetLayout(Layout::ImageLayout::General, cmdList, 0, 0);
            tex_material->SetLayout(Layout::ImageLayout::General, cmdList, 0, 0);
            tex_velocity->SetLayout(Layout::ImageLayout::General, cmdList, 0, 0);
            tex_depth->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
        }
    }

    void Renderer::Pass_Blit(CommandList* cmdList, ImageResource* in, ImageResource* out)
    {
        // compute blit: vulkan can't blit depth to float, amd uav requires float
        Shader* shader_c = GetShader(Renderer_Shader::blit_c);

        {
            PipelineState pso;
            pso.name             = "blit";
            pso.shaders[static_cast<uint32_t>(Stage::Compute)] = shader_c;
            cmdList->SetPipelineState(pso);

            cmdList->SetTexture(Renderer_BindingsUav::tex, out);
            cmdList->SetTexture(Renderer_BindingsSrv::tex, in);
            cmdList->Dispatch(out);
        }
    }

    void Renderer::Pass_Lut_BrdfSpecular(CommandList* cmdList)
    {
        ImageResource* tex_lut_brdf_specular = GetRenderTarget(Renderer_RenderTarget::lut_brdf_specular);

        {
            PipelineState pso;
            pso.name             = "lut_brdf_specular";
            pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::light_integration_brdf_specular_lut_c);
            cmdList->SetPipelineState(pso);

            cmdList->SetTexture(Renderer_BindingsUav::tex, tex_lut_brdf_specular);
            cmdList->Dispatch(tex_lut_brdf_specular);

            // permanent srv transition
            cmdList->InsertBarrier(tex_lut_brdf_specular->Get(), tex_lut_brdf_specular->GetImageSpec().format, 0, 1, 1, Layout::ImageLayout::ShaderRead);
        }

    }

    void Renderer::Pass_Lut_AtmosphericScattering(CommandList* cmdList)
    {
        ImageResource* tex_lut_atmosphere_scatter      = GetRenderTarget(Renderer_RenderTarget::lut_atmosphere_scatter);
        ImageResource* tex_lut_atmosphere_transmittance = GetRenderTarget(Renderer_RenderTarget::lut_atmosphere_transmittance);
        ImageResource* tex_lut_atmosphere_multiscatter  = GetRenderTarget(Renderer_RenderTarget::lut_atmosphere_multiscatter);

        {
            // transmittance lut
            {
                PipelineState pso;
                pso.name             = "lut_atmosphere_transmittance";
                pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::skysphere_transmittance_lut_c);
                cmdList->SetPipelineState(pso);

                cmdList->SetTexture(Renderer_BindingsUav::tex, tex_lut_atmosphere_transmittance);
                cmdList->Dispatch(tex_lut_atmosphere_transmittance);

                tex_lut_atmosphere_transmittance->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
            }

            // multi-scatter lut
            {
                PipelineState pso;
                pso.name             = "lut_atmosphere_multiscatter";
                pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::skysphere_multiscatter_lut_c);
                cmdList->SetPipelineState(pso);

                cmdList->SetTexture(Renderer_BindingsSrv::tex, tex_lut_atmosphere_transmittance);
                cmdList->SetTexture(Renderer_BindingsUav::tex, tex_lut_atmosphere_multiscatter);
                cmdList->Dispatch(tex_lut_atmosphere_multiscatter);

                tex_lut_atmosphere_multiscatter->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
            }

            // legacy 3d lut
            {
                PipelineState pso;
                pso.name             = "lut_atmospheric_scattering";
                pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::skysphere_lut_c);
                cmdList->SetPipelineState(pso);

                cmdList->SetTexture(Renderer_BindingsUav::tex3d, tex_lut_atmosphere_scatter);
                cmdList->Dispatch(tex_lut_atmosphere_scatter);

                tex_lut_atmosphere_scatter->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
            }
        }
    }

    void Renderer::Pass_HiZ(CommandList* cmdList)
    {
        // renders major occluders to a depth buffer and builds a hi-z mip chain.
        // the indirect cull compute shader samples this for gpu-driven occlusion culling.
        // the depth texture is ALWAYS cleared to 0.0 (far plane, reverse-z) and the mip
        // chain is always rebuilt, even when occlusion is disabled or suppressed. this
        // guarantees the cull shader never reads stale/uninitialized depth, which would
        // cause non-deterministic culling artifacts depending on gpu memory contents.

        ImageResource* tex_occluders     = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth_occluders);
        ImageResource* tex_occluders_hiz = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth_occluders_hiz);

        bool render_occluders = cvar_hiz_occlusion.GetValueAs<bool>() && !m_is_hiz_suppressed;

        // always start the render pass so the depth texture is cleared to far plane (0.0).
        // without this, the blit and downscale would propagate stale depth into the hi-z
        // mip chain, causing the cull shader to incorrectly occlude objects.
        {
            PipelineState pso;
            pso.name                             = "occluders";
            pso.shaders[static_cast<uint32_t>(Stage::Vertex)] = GetShader(Renderer_Shader::depth_prepass_v);
            pso.rasterizer_state                 = GetRasterizerState(Renderer_RasterizerState::Solid);
            pso.blend_state                      = GetBlendState(Renderer_BlendState::Off);
            pso.depth_stencil_state              = GetDepthStencilState(Renderer_DepthStencilState::ReadWrite);
            pso.render_target_depth_texture      = tex_occluders;
            pso.resolution_scale                 = true;
            pso.clear_depth                      = 0.0f;

            cmdList->SetPipelineState(pso);

            if (render_occluders)
            {
                for (uint32_t i = 0; i < m_draw_calls_prepass_count; i++)
                {
                    const Renderer_DrawCall& draw_call = m_draw_calls_prepass[i];

                    if (!draw_call.is_occluder)
                        continue;

                    Renderable* renderable = draw_call.renderable;
                    CullMode cull_mode = static_cast<CullMode>(renderable->GetMaterial()->GetProperty(MaterialProperty::CullMode));
                    cull_mode              = (pso.rasterizer_state->GetPolygonMode() == PolygonMode::Wireframe) ? CullMode::None : cull_mode;
                    cmdList->SetCullMode(cull_mode);

                    m_pcb_pass_cpu.draw_index = draw_call.draw_data_index;
                    cmdList->PushConstants(m_pcb_pass_cpu);

                    cmdList->SetBufferVertex(renderable->GetVertexBuffer());
                    cmdList->SetBufferIndex(renderable->GetIndexBuffer());

                    cmdList->DrawIndexed(
                        renderable->GetIndexCount(draw_call.lod_index),
                        renderable->GetIndexOffset(draw_call.lod_index),
                        renderable->GetVertexOffset(draw_call.lod_index)
                    );
                }
            }
        }

        // hi-z mip chain (min depth downsample, reverse z)
        Pass_Blit(cmdList, tex_occluders, tex_occluders_hiz);
        Pass_Downscale(cmdList, tex_occluders_hiz, Renderer_DownsampleFilter::Min);

    }

    void Renderer::Pass_IndirectCull(CommandList* cmdList)
    {
        if (m_indirect_draw_count == 0)
            return;

        {
            ImageResource* tex_occluders_hiz = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth_occluders_hiz);

            PipelineState pso;
            pso.name             = "indirect_cull";
            pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::indirect_cull_c);
            cmdList->SetPipelineState(pso);

            cmdList->SetTexture(Renderer_BindingsSrv::tex, tex_occluders_hiz);

            // input
            cmdList->SetBuffer(Renderer_BindingsUav::indirect_draw_args, GetBuffer(Renderer_Buffer::IndirectDrawArgs));
            cmdList->SetBuffer(Renderer_BindingsUav::indirect_draw_data, GetBuffer(Renderer_Buffer::IndirectDrawData));

            // output
            cmdList->SetBuffer(Renderer_BindingsUav::indirect_draw_args_out, GetBuffer(Renderer_Buffer::IndirectDrawArgsOut));
            cmdList->SetBuffer(Renderer_BindingsUav::indirect_draw_data_out, GetBuffer(Renderer_Buffer::IndirectDrawDataOut));
            cmdList->SetBuffer(Renderer_BindingsUav::indirect_draw_count,    GetBuffer(Renderer_Buffer::IndirectDrawCount));

            m_pcb_pass_cpu.set_f4_value(static_cast<float>(m_indirect_draw_count), static_cast<float>(tex_occluders_hiz->GetImageSpec().mipCount - 1), 0.0f, 0.0f);
            cmdList->PushConstants(m_pcb_pass_cpu);

            uint32_t thread_group_count = (m_indirect_draw_count + 255) / 256;
            cmdList->Dispatch(thread_group_count, 1, 1);

            // barrier: compute write -> vertex/indirect read
            cmdList->InsertBarrier(GetBuffer(Renderer_Buffer::IndirectDrawArgsOut));
            cmdList->InsertBarrier(GetBuffer(Renderer_Buffer::IndirectDrawDataOut));
            cmdList->InsertBarrier(GetBuffer(Renderer_Buffer::IndirectDrawCount));
        }

    }
	
    void Renderer::Pass_ScreenSpaceShadows(CommandList* cmdList)
    {
        ImageResource* tex_sss = GetRenderTarget(Renderer_RenderTarget::sss);

        {
            cmdList->InsertBarrier(tex_sss, BarrierType::EnsureReadThenWrite);

            PipelineState pso;
            pso.name             = "screen_space_shadows";
            pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::sss_c_bend);
            cmdList->SetPipelineState(pso);

            cmdList->SetTexture(Renderer_BindingsSrv::tex,     GetRenderTarget(Renderer_RenderTarget::gbuffer_depth));
            cmdList->SetTexture(Renderer_BindingsUav::tex_sss, tex_sss);
            static float array_slice_index = 0.0f;
            for (Entity* entity : Scene::GetEntities())
            {
                Light* light = entity->GetComponent<Light>();
                if (!light || !light->GetFlag(LightFlags::ShadowsScreenSpace) || light->GetIntensityWatt() == 0.0f)
                    continue;

                if (array_slice_index == static_cast<float>(tex_sss->GetImageSpec().depth))
                {
                    SEDX_CORE_WARN_TAG("Render Pass", "Render target has reached the maximum number of lights it can hold");
                    break;
                }

                Camera* cam = Scene::GetCamera();
                xMath::Matrix view_projection = cam ? cam->GetViewProjectionMatrix() : xMath::Matrix{};
                Vec4 p = {};
                if (light->GetLightType() == LightType::Directional)
                {
                    p = view_projection * Vec4(-light->GetEntity()->GetForward(), 0.0f);
                }
                else
                {
                    p = view_projection * Vec4(light->GetEntity()->GetPosition(), 1.0f);
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

                    light->SetScreenSpaceShadowsSliceIndex(static_cast<uint32_t>(array_slice_index));
                    float near_val = 1.0f;
                    float far_val  = 0.0f;
                    m_pcb_pass_cpu.set_f3_value(near_val, far_val, array_slice_index++);
                    m_pcb_pass_cpu.set_f3_value2(1.0f / tex_sss->GetWidth(), 1.0f / tex_sss->GetHeight(), 0.0f);

                    for (int32_t dispatch_index = 0; dispatch_index < dispatch_list.DispatchCount; ++dispatch_index)
                    {
                        const Bend::DispatchData& dispatch = dispatch_list.Dispatch[dispatch_index];
                        m_pcb_pass_cpu.set_f2_value(static_cast<float>(dispatch.WaveOffset_Shader[0]), static_cast<float>(dispatch.WaveOffset_Shader[1]));
                        cmdList->PushConstants(m_pcb_pass_cpu);
                        cmdList->Dispatch(dispatch.WaveCount[0], dispatch.WaveCount[1], dispatch.WaveCount[2]);
                    }

                    cmdList->InsertBarrier(tex_sss, BarrierType::EnsureWriteThenRead);
            }

            array_slice_index = 0;
        }
    }

    void Renderer::Pass_ScreenSpaceAO(CommandList* cmdList)
    {
        if (!cvar_ssao.GetValueAs<bool>())
            return;

        ImageResource* tex_ssao = GetRenderTarget(Renderer_RenderTarget::ssao);
        if (!tex_ssao)
            return;

        PipelineState pso;
        pso.name             = "screen_space_ambient_occlusion";
        pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::ssao_c);

        {
            cmdList->SetPipelineState(pso);
            SetCommonTextures(cmdList);
            cmdList->SetTexture(Renderer_BindingsUav::tex, tex_ssao);
            cmdList->Dispatch(tex_ssao, cvar_resolution_scale.GetValue());
        }

    }

    // -------------------------------------------------------
    // Pass_CloudNoise
    // Generates the 3D shape and detail noise textures used by the volumetric cloud system.
    // Runs once and is guarded by m_PassState.m_CloudNoiseProduced.
    // -------------------------------------------------------

    void Renderer::Pass_CloudNoise(CommandList* cmdList)
    {
        if (m_PassState.m_CloudNoiseProduced)
            return;

        ImageResource* tex_shape  = GetRenderTarget(Renderer_RenderTarget::cloud_noise_shape);
        ImageResource* tex_detail = GetRenderTarget(Renderer_RenderTarget::cloud_noise_detail);

        // Shape noise – 128×128×128, RGBA16F
        {
            PipelineState pso;
            pso.name = "cloud_noise_shape";
            pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::cloud_noise_shape_c);
            cmdList->SetPipelineState(pso);

            cmdList->SetTexture(Renderer_BindingsUav::tex3d, tex_shape);
            cmdList->Dispatch(tex_shape);

            tex_shape->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
        }

        // Detail noise – 32×32×32, RGBA16F
        {
            PipelineState pso;
            pso.name = "cloud_noise_detail";
            pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::cloud_noise_detail_c);
            cmdList->SetPipelineState(pso);

            cmdList->SetTexture(Renderer_BindingsUav::tex3d, tex_detail);
            cmdList->Dispatch(tex_detail);

            tex_detail->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
        }

        m_PassState.m_CloudNoiseProduced = true;

        SEDX_CORE_INFO_TAG("Render Pass", "Cloud noise textures generated");
    }

    // -------------------------------------------------------
    // Pass_VariableRateShading
    // Generates the VRS image used to modulate per-pixel shading frequency.
    // Driven by velocity and luminance from the previous frame.
    // -------------------------------------------------------

    void Renderer::Pass_VariableRateShading(CommandList* cmdList)
    {
        ImageResource* tex_vrs = GetRenderTarget(Renderer_RenderTarget::shading_rate);
        if (!tex_vrs)
            return;

        // Clear the VRS texture on first use or after a resolution change
        if (m_PassState.m_VrsLastClearedTexture != tex_vrs)
        {
            cmdList->ClearTexture(tex_vrs, Color::Black());
            m_PassState.m_VrsLastClearedTexture = tex_vrs;
        }

        {
            PipelineState pso;
            pso.name = "variable_rate_shading";
            pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::variable_rate_shading_c);
            cmdList->SetPipelineState(pso);

            cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_depth,    GetRenderTarget(Renderer_RenderTarget::gbuffer_depth));
            cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_velocity,  GetRenderTarget(Renderer_RenderTarget::gbuffer_velocity));
            cmdList->SetTexture(Renderer_BindingsUav::tex_uint,          tex_vrs);

            cmdList->Dispatch(tex_vrs);
        }

        tex_vrs->SetLayout(Layout::ImageLayout::Attachment, cmdList, 0, 0);
    }

    // -------------------------------------------------------
    // Pass_Downscale
    // Builds a full mip chain on img using the AMD FidelityFX SPD compute shader.
    // filter selects between average, min, and max kernels.
    // -------------------------------------------------------

    void Renderer::Pass_Downscale(CommandList* cmdList, ImageResource* img, const Renderer_DownsampleFilter filter)
    {
        if (!img || img->GetImageSpec().mipCount <= 1)
            return;

        Renderer_Shader spdShader;
        switch (filter)
        {
            case Renderer_DownsampleFilter::Min:     spdShader = Renderer_Shader::ffx_spd_min_c;     break;
            case Renderer_DownsampleFilter::Max:     spdShader = Renderer_Shader::ffx_spd_max_c;     break;
            case Renderer_DownsampleFilter::Average:
            default:                                 spdShader = Renderer_Shader::ffx_spd_average_c; break;
        }

        {
            PipelineState pso;
            pso.name = "spd_downscale";
            pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(spdShader);
            cmdList->SetPipelineState(pso);

            // SPD reads mip-0 as SRV and writes mips 1..N as UAVs.
            cmdList->SetTexture(Renderer_BindingsSrv::tex, img, 0, 1);

            for (uint32_t mip = 1; mip < img->GetImageSpec().mipCount; mip++)
            {
                cmdList->SetTexture(Renderer_BindingsUav::tex, img, mip, 1);
            }

            // SPD counter reset buffer
            cmdList->SetBuffer(Renderer_BindingsUav::sb_spd, GetBuffer(Renderer_Buffer::SpdCounter));

            m_pcb_pass_cpu.set_f4_value(
                static_cast<float>(img->GetImageSpec().mipCount - 1),
                static_cast<float>(img->GetWidth()),
                static_cast<float>(img->GetHeight()),
                0.0f
            );
            cmdList->PushConstants(m_pcb_pass_cpu);

            // SPD uses fixed 256-thread groups; one per 64×64 tile at mip-0.
            const uint32_t dispatchX = (img->GetWidth()  + 63) / 64;
            const uint32_t dispatchY = (img->GetHeight() + 63) / 64;
            cmdList->Dispatch(dispatchX, dispatchY, 1);
        }
    }

    // -------------------------------------------------------
    // Pass_CloudShadow
    // Projects cloud shadow onto a 2D shadow map consumed by lighting passes.
    // Requires cloud noise textures to already be in ShaderRead layout.
    // -------------------------------------------------------

    void Renderer::Pass_CloudShadow(CommandList* cmdList)
    {
        ImageResource* tex_cloud_shadow = GetRenderTarget(Renderer_RenderTarget::cloud_shadow);
        ImageResource* tex_cloud_shape  = GetRenderTarget(Renderer_RenderTarget::cloud_noise_shape);
        ImageResource* tex_cloud_detail = GetRenderTarget(Renderer_RenderTarget::cloud_noise_detail);

        if (!tex_cloud_shadow || !tex_cloud_shape)
            return;

        {
            PipelineState pso;
            pso.name = "cloud_shadow";
            pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::cloud_shadow_c);
            cmdList->SetPipelineState(pso);

            cmdList->SetTexture(Renderer_BindingsSrv::tex3d_cloud_shape, tex_cloud_shape);
            if (tex_cloud_detail)
                cmdList->SetTexture(Renderer_BindingsSrv::tex3d_cloud_detail, tex_cloud_detail);

            cmdList->SetTexture(Renderer_BindingsUav::tex, tex_cloud_shadow);
            cmdList->Dispatch(tex_cloud_shadow);

            tex_cloud_shadow->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
        }
    }

    // -------------------------------------------------------
    // Pass_ShadowMaps
    // Renders all shadow-casting lights into the shadow atlas (8192×8192 depth texture).
    // Each light is given a tile inside the atlas; shadow matrices are written to the
    // per-frame constant buffer before this pass runs.
    // -------------------------------------------------------

    void Renderer::Pass_ShadowMaps(CommandList* cmdList)
    {
        ImageResource* tex_shadow_atlas = GetRenderTarget(Renderer_RenderTarget::shadow_atlas);
        if (!tex_shadow_atlas)
            return;

        for (Entity* entity : Scene::GetEntities())
        {
            Light* light = entity->GetComponent<Light>();
            if (!light || !light->GetFlag(LightFlags::Shadows) || light->GetIntensityWatt() == 0.0f)
                continue;

            RasterizerState* rs = (light->GetLightType() == LightType::Directional)
                ? GetRasterizerState(Renderer_RasterizerState::Light_directional)
                : GetRasterizerState(Renderer_RasterizerState::Light_point_spot);

            PipelineState pso;
            pso.name                           = "shadow_map";
            pso.shaders[static_cast<uint32_t>(Stage::Vertex)]   = GetShader(Renderer_Shader::depth_light_v);
            pso.shaders[static_cast<uint32_t>(Stage::Fragment)]  = GetShader(Renderer_Shader::depth_light_alpha_color_p);
            pso.rasterizer_state               = rs;
            pso.blend_state                    = GetBlendState(Renderer_BlendState::Off);
            pso.depth_stencil_state            = GetDepthStencilState(Renderer_DepthStencilState::ReadWrite);
            pso.render_target_depth_texture    = tex_shadow_atlas;
            pso.clear_depth                    = 0.0f; // reverse-z: far = 0

            cmdList->SetPipelineState(pso);
            cmdList->SetCullMode(CullMode::Back);

            for (uint32_t i = 0; i < m_draw_calls_prepass_count; i++)
            {
                const Renderer_DrawCall& draw_call = m_draw_calls_prepass[i];
                Renderable* renderable             = draw_call.renderable;
                Material*   material               = renderable->GetMaterial();
                if (!material || material->IsTransparent() || !draw_call.camera_visible)
                    continue;

                m_pcb_pass_cpu.draw_index     = draw_call.draw_data_index;
                m_pcb_pass_cpu.material_index = material->GetIndex();
                cmdList->PushConstants(m_pcb_pass_cpu);

                cmdList->SetVertexBuffer(renderable->GetVertexBuffer(), renderable->GetInstanceBuffer());
                cmdList->SetIndexBuffer(renderable->GetIndexBuffer());
                cmdList->DrawIndexed(
                    renderable->GetIndexCount(draw_call.lod_index),
                    draw_call.instance_count,
                    renderable->GetIndexOffset(draw_call.lod_index),
                    renderable->GetVertexOffset(draw_call.lod_index),
                    draw_call.instance_index
                );
            }
        }

        tex_shadow_atlas->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
    }

    // -------------------------------------------------------
    // Pass_Light
    // Evaluates direct lighting from all active lights using the G-Buffer inputs.
    // Produces separate diffuse and specular accumulation buffers.
    // -------------------------------------------------------

    void Renderer::Pass_Light(CommandList* cmdList, const bool isTransparentPass)
    {
        ImageResource* tex_diffuse  = GetRenderTarget(Renderer_RenderTarget::light_diffuse);
        ImageResource* tex_specular = GetRenderTarget(Renderer_RenderTarget::light_specular);

        {
            PipelineState pso;
            pso.name = isTransparentPass ? "light_transparent" : "light";
            pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::light_c);
            cmdList->SetPipelineState(pso);

            // G-Buffer inputs
            cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_albedo,   GetRenderTarget(Renderer_RenderTarget::gbuffer_color));
            cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_normal,   GetRenderTarget(Renderer_RenderTarget::gbuffer_normal));
            cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_material, GetRenderTarget(Renderer_RenderTarget::gbuffer_material));
            cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_depth,    GetRenderTarget(Renderer_RenderTarget::gbuffer_depth));

            // Shadow atlas
            cmdList->SetTexture(Renderer_BindingsSrv::tex,  GetRenderTarget(Renderer_RenderTarget::shadow_atlas));

            // Screen-space ambient occlusion (optional)
            if (ImageResource* tex_ssao = GetRenderTarget(Renderer_RenderTarget::ssao))
                cmdList->SetTexture(Renderer_BindingsSrv::ssao, tex_ssao);

            // Screen-space shadows
            cmdList->SetTexture(Renderer_BindingsSrv::tex2, GetRenderTarget(Renderer_RenderTarget::sss));

            // Outputs
            cmdList->SetTexture(Renderer_BindingsUav::tex,  tex_diffuse);
            cmdList->SetTexture(Renderer_BindingsUav::tex2, tex_specular);

            m_pcb_pass_cpu.is_transparent = isTransparentPass ? 1u : 0u;
            cmdList->PushConstants(m_pcb_pass_cpu);

            cmdList->Dispatch(tex_diffuse);
        }
    }

    // -------------------------------------------------------
    // Pass_Light_Composition
    // Combines albedo × diffuse + specular into the HDR frame_render buffer.
    // -------------------------------------------------------

    void Renderer::Pass_Light_Composition(CommandList* cmdList, const bool isTransparentPass)
    {
        ImageResource* tex_frame_render = GetRenderTarget(Renderer_RenderTarget::frame_render);
        ImageResource* tex_diffuse      = GetRenderTarget(Renderer_RenderTarget::light_diffuse);
        ImageResource* tex_specular     = GetRenderTarget(Renderer_RenderTarget::light_specular);

        {
            PipelineState pso;
            pso.name = isTransparentPass ? "light_composition_transparent" : "light_composition";
            pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::light_composition_c);
            cmdList->SetPipelineState(pso);

            cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_albedo,   GetRenderTarget(Renderer_RenderTarget::gbuffer_color));
            cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_material, GetRenderTarget(Renderer_RenderTarget::gbuffer_material));
            cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_depth,    GetRenderTarget(Renderer_RenderTarget::gbuffer_depth));
            cmdList->SetTexture(Renderer_BindingsSrv::tex,              tex_diffuse);
            cmdList->SetTexture(Renderer_BindingsSrv::tex2,             tex_specular);
            cmdList->SetTexture(Renderer_BindingsUav::tex,              tex_frame_render);

            m_pcb_pass_cpu.is_transparent = isTransparentPass ? 1u : 0u;
            cmdList->PushConstants(m_pcb_pass_cpu);

            cmdList->Dispatch(tex_frame_render);
        }
    }

    // -------------------------------------------------------
    // Pass_Light_ImageBased
    // Applies specular IBL from the filtered skysphere and the BRDF split-sum LUT.
    // -------------------------------------------------------

    void Renderer::Pass_Light_ImageBased(CommandList* cmdList)
    {
        ImageResource* tex_skysphere    = GetRenderTarget(Renderer_RenderTarget::skysphere);
        ImageResource* tex_lut_brdf     = GetRenderTarget(Renderer_RenderTarget::lut_brdf_specular);
        ImageResource* tex_frame_render = GetRenderTarget(Renderer_RenderTarget::frame_render);
        if (!tex_skysphere || !tex_lut_brdf)
            return;

        {
            PipelineState pso;
            pso.name = "light_image_based";
            pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::light_image_based_c);
            cmdList->SetPipelineState(pso);

            cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_albedo,   GetRenderTarget(Renderer_RenderTarget::gbuffer_color));
            cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_normal,   GetRenderTarget(Renderer_RenderTarget::gbuffer_normal));
            cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_material, GetRenderTarget(Renderer_RenderTarget::gbuffer_material));
            cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_depth,    GetRenderTarget(Renderer_RenderTarget::gbuffer_depth));
            cmdList->SetTexture(Renderer_BindingsSrv::tex,              tex_skysphere);
            cmdList->SetTexture(Renderer_BindingsSrv::tex2,             tex_lut_brdf);
            cmdList->SetTexture(Renderer_BindingsUav::tex,              tex_frame_render);

            cmdList->Dispatch(tex_frame_render);
        }
    }

    // -------------------------------------------------------
    // Pass_Light_Reflections
    // Screen-space reflections (SSR) that complement IBL for rough materials.
    // Outputs a separate reflections buffer that is blended into frame_render.
    // -------------------------------------------------------

    void Renderer::Pass_Light_Reflections(CommandList* cmdList)
    {
        ImageResource* tex_reflections  = GetRenderTarget(Renderer_RenderTarget::reflections);
        ImageResource* tex_frame_render = GetRenderTarget(Renderer_RenderTarget::frame_render);
        if (!tex_reflections)
            return;

        // Guard: clear reflections if the feature was just toggled off
        if (m_PassState.m_ClearedReflections)
        {
            cmdList->ClearTexture(tex_reflections, Color::Black());
            m_PassState.m_ClearedReflections = false;
        }

        {
            PipelineState pso;
            pso.name = "light_reflections";
            pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::light_reflections_c);
            cmdList->SetPipelineState(pso);

            cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_normal,   GetRenderTarget(Renderer_RenderTarget::gbuffer_normal));
            cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_material, GetRenderTarget(Renderer_RenderTarget::gbuffer_material));
            cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_depth,    GetRenderTarget(Renderer_RenderTarget::gbuffer_depth));
            cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_velocity, GetRenderTarget(Renderer_RenderTarget::gbuffer_velocity));
            cmdList->SetTexture(Renderer_BindingsSrv::tex,              tex_frame_render);
            cmdList->SetTexture(Renderer_BindingsUav::tex,              tex_reflections);

            cmdList->Dispatch(tex_reflections);
        }

        tex_reflections->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);

        // Composite reflections into the frame buffer using the blit pass
        Pass_Compute(cmdList, "reflections_composite", Renderer_Shader::blit_c,
                     tex_reflections, tex_frame_render, nullptr);
    }

    // -------------------------------------------------------
    // Pass_Particles
    // Three-stage GPU-driven particle system: emit → simulate → render.
    // All stages are compute-based; rendering writes directly to frame_render.
    // -------------------------------------------------------

    void Renderer::Pass_Particles(CommandList* cmdList)
    {
        // Emit
        {
            PipelineState pso;
            pso.name = "particle_emit";
            pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::particle_emit_c);
            cmdList->SetPipelineState(pso);

            cmdList->SetBuffer(Renderer_BindingsUav::particle_buffer_a, GetBuffer(Renderer_Buffer::ParticleBufferA));
            cmdList->SetBuffer(Renderer_BindingsUav::particle_counter,  GetBuffer(Renderer_Buffer::ParticleCounter));
            cmdList->SetBuffer(Renderer_BindingsUav::particle_emitter,  GetBuffer(Renderer_Buffer::ParticleEmitter));

            cmdList->Dispatch(1, 1, 1);
            cmdList->InsertBarrier(GetBuffer(Renderer_Buffer::ParticleBufferA));
        }

        // Simulate
        {
            PipelineState pso;
            pso.name = "particle_simulate";
            pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::particle_simulate_c);
            cmdList->SetPipelineState(pso);

            cmdList->SetBuffer(Renderer_BindingsUav::particle_buffer_a, GetBuffer(Renderer_Buffer::ParticleBufferA));
            cmdList->SetBuffer(Renderer_BindingsUav::particle_buffer_b, GetBuffer(Renderer_Buffer::ParticleBufferB));
            cmdList->SetBuffer(Renderer_BindingsUav::particle_counter,  GetBuffer(Renderer_Buffer::ParticleCounter));
            cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_depth,    GetRenderTarget(Renderer_RenderTarget::gbuffer_depth));

            cmdList->Dispatch(1, 1, 1);
            cmdList->InsertBarrier(GetBuffer(Renderer_Buffer::ParticleBufferB));
        }

        // Render
        {
            ImageResource* tex_frame_render = GetRenderTarget(Renderer_RenderTarget::frame_render);
            ImageResource* tex_depth        = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth);

            PipelineState pso;
            pso.name = "particle_render";
            pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::particle_render_c);
            cmdList->SetPipelineState(pso);

            cmdList->SetBuffer(Renderer_BindingsUav::particle_buffer_b, GetBuffer(Renderer_Buffer::ParticleBufferB));
            cmdList->SetTexture(Renderer_BindingsUav::tex,              tex_frame_render);
            cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_depth,    tex_depth);

            cmdList->Dispatch(tex_frame_render);
        }
    }

    // -------------------------------------------------------
    // Pass_TransparencyReflectionRefraction
    // Applies approximate screen-space refraction and reflection on transparent surfaces
    // by sampling the opaque frame buffer through the G-Buffer material channel.
    // -------------------------------------------------------

    void Renderer::Pass_TransparencyReflectionRefraction(CommandList* cmdList)
    {
        ImageResource* tex_frame_render        = GetRenderTarget(Renderer_RenderTarget::frame_render);
        ImageResource* tex_frame_render_opaque = GetRenderTarget(Renderer_RenderTarget::frame_render_opaque);
        ImageResource* tex_skysphere           = GetRenderTarget(Renderer_RenderTarget::skysphere);

        {
            PipelineState pso;
            pso.name = "transparency_reflection_refraction";
            pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::transparency_reflection_refraction_c);
            cmdList->SetPipelineState(pso);

            cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_albedo,   GetRenderTarget(Renderer_RenderTarget::gbuffer_color));
            cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_normal,   GetRenderTarget(Renderer_RenderTarget::gbuffer_normal));
            cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_material, GetRenderTarget(Renderer_RenderTarget::gbuffer_material));
            cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_depth,    GetRenderTarget(Renderer_RenderTarget::gbuffer_depth));
            cmdList->SetTexture(Renderer_BindingsSrv::tex,              tex_frame_render_opaque);

            if (tex_skysphere)
                cmdList->SetTexture(Renderer_BindingsSrv::tex2, tex_skysphere);

            cmdList->SetTexture(Renderer_BindingsUav::tex, tex_frame_render);

            cmdList->Dispatch(tex_frame_render);
        }
    }

    // -------------------------------------------------------
    // Pass_AA_Upscale
    // Applies CAS (Contrast Adaptive Sharpening) upscale from render to output resolution,
    // optionally followed by FXAA, then computes auto-exposure from the output luminance.
    // -------------------------------------------------------

    void Renderer::Pass_AA_Upscale(CommandList* cmdList)
    {
        ImageResource* tex_render  = GetRenderTarget(Renderer_RenderTarget::frame_render);
        ImageResource* tex_output  = GetRenderTarget(Renderer_RenderTarget::frame_output);

        // CAS upscale: render → output
        Pass_Compute(cmdList, "cas_upscale", Renderer_Shader::ffx_cas_c, tex_render, tex_output, nullptr);

        // Auto-exposure: sample output luminance into a 1×1 R32_SFLOAT texture
        {
            ImageResource* tex_auto_exp          = GetRenderTarget(Renderer_RenderTarget::auto_exposure);
            ImageResource* tex_auto_exp_previous = GetRenderTarget(Renderer_RenderTarget::auto_exposure_previous);

            PipelineState pso;
            pso.name = "auto_exposure";
            pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::auto_exposure_c);
            cmdList->SetPipelineState(pso);

            cmdList->SetTexture(Renderer_BindingsSrv::tex,  tex_output);
            cmdList->SetTexture(Renderer_BindingsSrv::tex2, tex_auto_exp_previous);
            cmdList->SetTexture(Renderer_BindingsUav::tex,  tex_auto_exp);

            cmdList->Dispatch(tex_auto_exp);

            // Ping-pong: swap current ↔ previous for temporal smoothing next frame
            std::swap(
                GetRenderTargets()[static_cast<uint32_t>(Renderer_RenderTarget::auto_exposure)],
                GetRenderTargets()[static_cast<uint32_t>(Renderer_RenderTarget::auto_exposure_previous)]
            );
        }
    }

    // -------------------------------------------------------
    // Pass_PostProcess
    // Full post-processing stack: bloom, tonemapping, film grain, dithering, outline.
    // Reads from frame_output and writes back to frame_output.
    // -------------------------------------------------------

    void Renderer::Pass_PostProcess(CommandList* cmdList)
    {
        ImageResource* tex_output   = GetRenderTarget(Renderer_RenderTarget::frame_output);
        ImageResource* tex_output_2 = GetRenderTarget(Renderer_RenderTarget::frame_output_2);
        ImageResource* tex_bloom    = GetRenderTarget(Renderer_RenderTarget::bloom);

        // Bloom -------------------------------------------------------
        {
            // 1. Luminance threshold + downsample to bloom mip-0
            {
                PipelineState pso;
                pso.name = "bloom_luminance";
                pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::bloom_luminance_c);
                cmdList->SetPipelineState(pso);

                cmdList->SetTexture(Renderer_BindingsSrv::tex,  tex_output);
                cmdList->SetTexture(Renderer_BindingsUav::tex,  tex_bloom, 0, 1);
                cmdList->Dispatch(tex_bloom);
            }

            // 2. Build full mip chain via SPD average
            Pass_Downscale(cmdList, tex_bloom, Renderer_DownsampleFilter::Average);

            // 3. Progressive upsample + blend mips (coarse → fine)
            for (uint32_t mip = tex_bloom->GetImageSpec().mipCount - 1; mip > 0; mip--)
            {
                PipelineState pso;
                pso.name = "bloom_upsample_blend";
                pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::bloom_upsample_blend_mip_c);
                cmdList->SetPipelineState(pso);

                cmdList->SetTexture(Renderer_BindingsSrv::tex, tex_bloom, mip,     1);
                cmdList->SetTexture(Renderer_BindingsUav::tex, tex_bloom, mip - 1, 1);
                cmdList->Dispatch(tex_bloom);

                cmdList->InsertBarrier(tex_bloom->Get(), tex_bloom->GetImageSpec().format, mip - 1, 1, 0, Layout::ImageLayout::ShaderRead);
            }

            // 4. Blend full bloom into frame buffer → tex_output_2
            {
                PipelineState pso;
                pso.name = "bloom_blend_frame";
                pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::bloom_blend_frame_c);
                cmdList->SetPipelineState(pso);

                cmdList->SetTexture(Renderer_BindingsSrv::tex,  tex_bloom);
                cmdList->SetTexture(Renderer_BindingsSrv::tex2, tex_output);
                cmdList->SetTexture(Renderer_BindingsUav::tex,  tex_output_2);
                cmdList->Dispatch(tex_output_2);
            }
        }

        // Tonemapping + auto-exposure (output_c reads from tex_output_2, writes to tex_output)
        Pass_Output(cmdList, tex_output_2, tex_output);

        // Dithering
        Pass_Compute(cmdList, "dithering",   Renderer_Shader::dithering_c,    tex_output, tex_output, nullptr);

        // Film grain
        Pass_Compute(cmdList, "film_grain",  Renderer_Shader::film_grain_c,   tex_output, tex_output, nullptr);

        // Selection outline (optional)
        {
            ImageResource* tex_outline = GetRenderTarget(Renderer_RenderTarget::outline);
            if (tex_outline)
            {
                PipelineState pso;
                pso.name = "outline_composite";
                pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::outline_c);
                cmdList->SetPipelineState(pso);

                cmdList->SetTexture(Renderer_BindingsSrv::tex, tex_outline);
                cmdList->SetTexture(Renderer_BindingsUav::tex, tex_output);
                cmdList->Dispatch(tex_output);
            }
        }

        // Debug visualisation (no-op in release)
        {
            PipelineState pso;
            pso.name = "dof";
            pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::depth_of_field_c);
            cmdList->SetPipelineState(pso);

            cmdList->SetTexture(Renderer_BindingsSrv::tex,              tex_output);
            cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_depth,    GetRenderTarget(Renderer_RenderTarget::gbuffer_depth));
            cmdList->SetTexture(Renderer_BindingsUav::tex,              tex_output_2);
            cmdList->Dispatch(tex_output_2);

            // Blit result back to primary output
            Pass_Blit(cmdList, tex_output_2, tex_output);
        }
    }

    // -------------------------------------------------------
    // Pass_Text
    // Rasterises 2D text/font glyphs onto the output render target.
    // Uses a dedicated vertex + fragment shader pair.
    // -------------------------------------------------------

    void Renderer::Pass_Text(CommandList* cmdList, ImageResource* out)
    {
        Shader* shader_v = GetShader(Renderer_Shader::font_v);
        Shader* shader_p = GetShader(Renderer_Shader::font_p);
        if (!shader_v || !shader_p || !out)
            return;

        {
            PipelineState pso;
            pso.name                                       = "text";
            pso.shaders[static_cast<uint32_t>(Stage::Vertex)]   = shader_v;
            pso.shaders[static_cast<uint32_t>(Stage::Fragment)]  = shader_p;
            pso.rasterizer_state                           = GetRasterizerState(Renderer_RasterizerState::Solid);
            pso.blend_state                                = GetBlendState(Renderer_BlendState::Alpha);
            pso.depth_stencil_state                        = GetDepthStencilState(Renderer_DepthStencilState::Off);
            pso.render_target_color_textures[0]            = out;
            cmdList->SetPipelineState(pso);

            // TODO: Issue font glyph draw calls from the text/font subsystem once it is wired in.
        }
    }

    // -------------------------------------------------------
    // SetCommonTextures
    // Binds the per-frame shared resources that every pass can optionally sample.
    // -------------------------------------------------------

    void Renderer::SetCommonTextures(CommandList* cmdList)
    {
        // TODO: Bind Perlin noise, blue noise, and other shared per-frame textures
        // once the StandardTexture registry is connected to the Renderer.
        (void)cmdList;
    }

} // namespace SceneryEditorX

// -------------------------------------------------------

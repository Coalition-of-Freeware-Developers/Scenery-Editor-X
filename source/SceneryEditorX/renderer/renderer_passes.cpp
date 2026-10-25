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
#include "bend_sss_cpu.h"
#include "frame_capture.h"
#include "gpu_timer.h"
#include "gbuffer.h"
#include "gpu_scene.h"
#include "renderer.h"
#include "renderer_declarations.h"
#include "scene_picking.h"
#include "font/font.h"
#include "vulkan/enums.h"
#include "vulkan/rasterizer.h"
#include "vulkan/render_context.h"
#include "vulkan/vertex.h"
#include "vulkan/pipeline/pipeline_state.h"
#include "vulkan/shader/shader_manager.h"
#include <algorithm>
#include <atomic>
#include <fstream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <SceneryEditorX/scene/camera.h>
#include <SceneryEditorX/scene/entity.h>
#include <SceneryEditorX/scene/material.h>
#include <SceneryEditorX/scene/mesh.h>
#include <SceneryEditorX/scene/scene.h>
#include <SceneryEditorX/scene/components/lights.h>
#include <SceneryEditorX/scene/components/renderable.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		/**
		 * @brief Returns a compute shader only when it is present, compiled, and has a compute stage.
		 * @param shaderId Renderer shader identifier.
		 * @param passName Pass/debug name for logging.
		 * @return Valid compute-capable shader pointer, or nullptr when unavailable.
		 */
		[[nodiscard]] Shader* GetValidComputeShader(const Renderer_Shader shaderId, const char* passName)
		{
			Shader* shader = ShaderManager::GetShader(shaderId);
			if (!shader)
			{
				SEDX_CORE_WARN_TAG("Render Pass", "Pass '{}' skipped: shader '{}' is not registered", passName, RendererShaderStageToString(shaderId));
				return nullptr;
			}

			if (!shader->IsCompiled())
			{
				SEDX_CORE_WARN_TAG("Render Pass", "Pass '{}' skipped: shader '{}' is not compiled yet", passName, RendererShaderStageToString(shaderId));
				return nullptr;
			}

			if (!shader->HasStage(StageType::Compute))
			{
				SEDX_CORE_WARN_TAG("Render Pass", "Pass '{}' skipped: shader '{}' has no compute stage", passName, RendererShaderStageToString(shaderId));
				return nullptr;
			}

			return shader;
		}

		/**
		 * @brief Draws one GPU-scene bucket's commands with the bound pipeline: the early pass's and / or, when this frame
		 * culls in two phases, the late pass's (GpuScene::GetLateCommandOffset() further on).
		 */
		void DrawGpuSceneBucket(CommandList *cmdList, const GpuScene::Bucket &bucket, const bool early, const bool late)
		{
			constexpr uint64_t stride = sizeof(VkDrawIndexedIndirectCommand);
			if (early)
				cmdList->DrawIndexedIndirect(GpuScene::GetCommandBuffer(), static_cast<uint64_t>(bucket.firstCommand) * stride, bucket.commandCount);
			if (late && GpuScene::IsOcclusionActive())
			{
				const uint64_t first = static_cast<uint64_t>(GpuScene::GetLateCommandOffset()) + bucket.firstCommand;
				cmdList->DrawIndexedIndirect(GpuScene::GetCommandBuffer(), first * stride, bucket.commandCount);
			}
		}

		std::atomic<bool> s_GridVisible{ true };

		// Viewport reference overlay (Renderer::SetReferenceOverlay), read by the render thread
		std::mutex s_ReferenceMutex;
		Ref<ImageResource> s_ReferenceImage;
		Renderer::ReferenceOverlayMode s_ReferenceMode = Renderer::ReferenceOverlayMode::Off;
		float s_ReferenceSplit = 0.5f;
		float s_ReferenceGain = 4.0f;

		// XP12 emulation state (RENDERER panel, command line); set while the render thread is idle
		RendererFeatures s_Features;
		Renderer_DebugView s_DebugView = Renderer_DebugView::None;
		int s_ToneCurve = 1;	// tonemap.slang: 1 AgX (X-Plane 12.2+), 2 ACES fit, 3 Reinhard
		RendererQualityPreset s_Quality = GetDefaultQualityPreset();
		Renderer::EnvironmentConstants s_EnvironmentConstants;

		// Light billboards (Renderer::SetLightBillboards) and the vertex buffers they stream through. Each frame
		// writes the next buffer of the ring, so a frame still in flight keeps reading its own.
		constexpr uint32_t BILLBOARD_BUFFER_COUNT = 4;
		std::mutex s_LightBillboardMutex;
		std::vector<Renderer_LightBillboardBatch> s_LightBillboards;
		std::array<Ref<Buffer>, BILLBOARD_BUFFER_COUNT> s_BillboardBuffers;
		std::array<uint32_t, BILLBOARD_BUFFER_COUNT> s_BillboardBufferCapacity = {};
		uint32_t s_BillboardBufferIndex = 0;
	}

	// NOTE: Static member definitions are centralized in renderer_resources.cpp
	// to avoid duplicate symbol definitions across translation units.

	// Console variable stubs – replace with real CVar system when available
	static CVar cvar_wireframe            { 0.0f };
	static CVar cvar_grid                 { 1.0f };
	static CVar cvar_resolution_scale     { 1.0f };
	static CVar cvar_variable_rate_shading{ 0.0f };
	static CVar cvar_hiz_occlusion        { 1.0f };
	static CVar cvar_ssao                 { 1.0f };
	static CVar cvar_cloud_coverage       { 0.45f };

	void Renderer::ProduceFrame(CommandList *graphicsPresent, CommandList *compute)
	{
		// The XP12 frame graph (plan TASK-024): one ordered sequence on the graphics list, each step behind its feature
		// flag. Async compute stays off until queue-family ownership transfers exist (plan CON-009).
		(void)compute;
		CommandList *cmd = graphicsPresent;
		ImageResource *rt_render = GetRenderTarget(Renderer_RenderTarget::frame_render);
		ImageResource *rt_output = GetRenderTarget(Renderer_RenderTarget::frame_output);
		const bool hasRtRender = rt_render && rt_render->Get() && *rt_render->Get() != VK_NULL_HANDLE;
		const bool hasRtOutput = rt_output && rt_output->Get() && *rt_output->Get() != VK_NULL_HANDLE;
		if (!cmd || !hasRtRender || !hasRtOutput)
			return;

		const RendererFeatures &features = GetFeatures();
		// An extra view (RenderExtraViews) leaves the main view's timings, exposure metering, probes, picking and captures alone
		const bool mainView = !IsRenderingExtraView();
		if (mainView)
			GpuTimer::BeginFrame(cmd);
		const auto valid = [cmd] { return !cmd->IsStickyInvalid(); };
		BeginReflectionsFrame();

		// (0) Environment constants are uploaded with the frame constants (UpdateEnvironmentConstantBuffer);
		//     environment probes: Phase 6
		if (!m_PassState.m_BRDF_LutProduced)
		{
			GpuTimer::Scope scope(cmd, "brdf lut");
			Pass_Lut_BrdfSpecular(cmd);
			m_PassState.m_BRDF_LutProduced = true;
		}
		if (mainView)
			FrameCapture::RecordDump(cmd, GetRenderTarget(Renderer_RenderTarget::lut_brdf_specular));	// --dump-brdf-lut

		// (1) Atmosphere LUTs (XP12: scatter_compute_*) and the environment probes; whatever the switches, they end in
		//     shader-read layout for the passes that bind them
		if (features.atmosphere && valid())
		{
			{
				GpuTimer::Scope scope(cmd, "atmosphere");
				Pass_Atmosphere(cmd);
			}
			if (features.envProbes && mainView && valid())
			{
				GpuTimer::Scope scope(cmd, "env probes");
				Pass_EnvProbes(cmd);
			}
		}
		if (valid())
			ReadyAtmosphereTextures(cmd);

		// (2) Cloud shadows (XP12: cloud_shadows; renderer_shadows.cpp); cloud weather volumes: Phase 15. Ends shader-readable
		//     whatever the switches.
		if (valid())
		{
			GpuTimer::Scope scope(cmd, "cloud shadows");
			Pass_CloudShadow(cmd);
		}

		Camera *camera = Scene::GetCamera();
		if (!camera)
		{
			cmd->ClearTexture(rt_output, Color::Black());
		}
		else if (valid())
		{
			// (3) Culling, then the sun shadow cascades (XP12: the CSM pass; renderer_shadows.cpp), which end shader-readable
			//     whatever the switches
			{
				GpuTimer::Scope scope(cmd, "cull");
				GpuScene::Cull(cmd);
			}
			if (!valid())
				return;
			{
				GpuTimer::Scope scope(cmd, "shadows");
				Pass_ShadowCascades(cmd);
			}
			if (!valid())
				return;

			// (4) Depth prepass and g-buffer (terrain: Phase 10)
			{
				GpuTimer::Scope scope(cmd, "depth prepass");
				Pass_Depth_Prepass(cmd);
			}
			if (!valid())
				return;
			{
				GpuTimer::Scope scope(cmd, "gbuffer");
				Pass_GBuffer(cmd, false);
				if (!valid())
					return;
				Pass_Drape_Depth(cmd);
			}
			if (!valid())
				return;
			if (mainView)
				GpuScene::RecordReadBack(cmd);	// every cull of the frame has run (stats)

			GetRenderTarget(Renderer_RenderTarget::gbuffer_color)->SetLayout(Layout::ImageLayout::ShaderRead, cmd, 0, 0);
			GetRenderTarget(Renderer_RenderTarget::gbuffer_normal)->SetLayout(Layout::ImageLayout::ShaderRead, cmd, 0, 0);
			GetRenderTarget(Renderer_RenderTarget::gbuffer_material)->SetLayout(Layout::ImageLayout::ShaderRead, cmd, 0, 0);
			GetRenderTarget(Renderer_RenderTarget::gbuffer_velocity)->SetLayout(Layout::ImageLayout::ShaderRead, cmd, 0, 0);
			GetRenderTarget(Renderer_RenderTarget::gbuffer_emissive)->SetLayout(Layout::ImageLayout::ShaderRead, cmd, 0, 0);
			GetRenderTarget(Renderer_RenderTarget::gbuffer_weather)->SetLayout(Layout::ImageLayout::ShaderRead, cmd, 0, 0);
			GetRenderTarget(Renderer_RenderTarget::gbuffer_depth)->SetLayout(Layout::ImageLayout::ShaderRead, cmd, 0, 0);

			// (5) Min / max depth pyramid (XP12: z_minmax; renderer_ssr.cpp) of the finished depth, for the reflections (light
			//     tiling and volumetrics later)
			if (AreReflectionsAvailable())
			{
				GpuTimer::Scope scope(cmd, "depth pyramid");
				Pass_ReflectionDepthPyramid(cmd);
			}
			if (!valid())
				return;
			// (6) Weather apply (XP12: weather_apply_compute): Phase 12
			// (7) Ambient occlusion (XP12: cacao; renderer_ao.cpp): AMD FidelityFX CACAO at the QUALITY AO level into ssao
			{
				GpuTimer::Scope scope(cmd, "ambient occlusion");
				Pass_AmbientOcclusion(cmd);
			}
			if (!valid())
				return;
			// (8) Light tiling (XP12: light_tile, light_spill_compact): Phase 14
			// (9) Screen-space reflections (XP12: ssr_deferred; renderer_ssr.cpp): the glossy g-buffer surfaces' mirrors
			//     into the reflections target, from the previous frame (blended surfaces trace their own in the forward pass)
			if (AreReflectionsAvailable())
			{
				GpuTimer::Scope scope(cmd, "reflections");
				Pass_ScreenSpaceReflections(cmd);
			}
			if (!valid())
				return;
			// (10) Clouds and volumetric fog: Phase 15

			// (11) Deferred lighting: sun, shadows, AO, IBL, SSR, tiled lights, emissive, aerial perspective (later
			//      phases add to it); spilled light volumes: Phase 14
			{
				GpuTimer::Scope scope(cmd, "lighting");
				Pass_Light(cmd, false);
				if (!valid())
					return;
				Pass_Light_Composition(cmd, false);
			}
			if (!valid())
				return;

			// (12) Water (XP12: ocean_shading): Phase 11
			// (13) Sky where nothing was drawn (XP12: sky); volumetric apply: Phase 15
			if (features.atmosphere)
			{
				GpuTimer::Scope scope(cmd, "sky");
				Pass_Sky(cmd, rt_render);
				if (!valid())
					return;
				ReadyAtmosphereTextures(cmd);
			}

			// (14) Forward passes over the composed frame, depth tested against the g-buffer: grid, blended surfaces, lights
			{
				GpuTimer::Scope scope(cmd, "forward");
				ImageResource *depthTarget = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth);
				depthTarget->SetLayout(Layout::ImageLayout::DepthStencilAttachment, cmd, 0, 0);
				Pass_Grid(cmd, rt_render);
				if (!valid())
					return;
				Pass_Forward_Transparent(cmd, rt_render);
				if (!valid())
					return;
				Pass_Light_Billboards(cmd, rt_render);
				if (!valid())
					return;
				depthTarget->SetLayout(Layout::ImageLayout::ShaderRead, cmd, 0, 0);
			}

			// (15) HDR clean-up (XP12: fix_hdr), then the finished frame kept for the next frame's reflections
			{
				GpuTimer::Scope scope(cmd, "fix hdr");
				Pass_FixHdr(cmd, rt_render);
			}
			if (!valid())
				return;
			if (AreReflectionsAvailable())
			{
				GpuTimer::Scope scope(cmd, "reflection history");
				Pass_ReflectionHistory(cmd, rt_render);
			}
			if (!valid())
				return;

			// (16) Metering and adaptation (manual EV100 when auto exposure is off), then the output chain: exposure
			//      fusion, bloom, the tone map to display values, FXAA, FSR 1
			if (mainView)
			{
				GpuTimer::Scope scope(cmd, "exposure");
				Pass_Exposure(cmd, rt_render);	// Extra views use the exposure it adapts
			}
			if (!valid())
				return;
			if (features.tonemap)
			{
				Pass_OutputChain(cmd, rt_render, rt_output);	// Times its own passes (GpuTimer scopes don't nest)
			}
			else
			{
				GpuTimer::Scope scope(cmd, "output");
				cmd->Blit(rt_render, rt_output, false);	// The frame as rendered (pre-exposed)
			}
			if (!valid())
				return;
			if (GetDebugView() != Renderer_DebugView::None)
			{
				GpuTimer::Scope scope(cmd, "debug view");
				Pass_DebugView(cmd, rt_output);
			}

			// Parity captures (--capture-png) read the output before anything editor-only is drawn on it
			if (mainView)
				FrameCapture::Record(cmd, rt_output, GetRenderTarget(Renderer_RenderTarget::gbuffer_depth));

			// (17) Editor overlays on the display image: selection / hover outlines, the reference image
			ScenePicking::RecordOutline(cmd, rt_output);
			if (!valid())
				return;
			if (mainView)
				Pass_ReferenceOverlay(cmd, rt_output);
			if (!valid())
				return;
		}

		if (valid() && mainView)
			Pass_Text(cmd, rt_output);
		if (mainView)
			GpuTimer::EndFrame(cmd);

		// Early transitions for the next frame
		rt_output->SetLayout(Layout::ImageLayout::ShaderRead, cmd, 0, 0);
		GetRenderTarget(Renderer_RenderTarget::gbuffer_color)->SetLayout(Layout::ImageLayout::Attachment, cmd, 0, 0);
		GetRenderTarget(Renderer_RenderTarget::gbuffer_normal)->SetLayout(Layout::ImageLayout::Attachment, cmd, 0, 0);
		GetRenderTarget(Renderer_RenderTarget::gbuffer_material)->SetLayout(Layout::ImageLayout::Attachment, cmd, 0, 0);
		GetRenderTarget(Renderer_RenderTarget::gbuffer_velocity)->SetLayout(Layout::ImageLayout::Attachment, cmd, 0, 0);
		GetRenderTarget(Renderer_RenderTarget::gbuffer_emissive)->SetLayout(Layout::ImageLayout::Attachment, cmd, 0, 0);
		GetRenderTarget(Renderer_RenderTarget::gbuffer_weather)->SetLayout(Layout::ImageLayout::Attachment, cmd, 0, 0);
		GetRenderTarget(Renderer_RenderTarget::gbuffer_depth)->SetLayout(Layout::ImageLayout::Attachment, cmd, 0, 0);
		ScenePicking::EndFrame(cmd);

		// Submission and synchronization are owned by the caller (Renderer::OnRender)
	}

	void Renderer::Pass_Tonemap(CommandList *cmdList, ImageResource *in, ImageResource *out, const uint32_t bloomMips, const bool fusion)
	{
		if (!ShaderManager::IsShaderAvailable(Renderer_Shader::tonemap))
		{
			cmdList->Blit(in, out, false);
			return;
		}
		in->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
		out->SetLayout(Layout::ImageLayout::General, cmdList, 0, 0);
		const EnvironmentConstants &constants = GetEnvironmentConstants();
		Pass_Compute(cmdList, "tonemap", Renderer_Shader::tonemap, in, out, [&]
		{
			// The bloom chain, the fusion guide and the exposures it was made from, depth for the fusion's distance fade,
			// and the exposure state (the residual)
			cmdList->SetTexture(Renderer_BindingsSrv::tex2, GetRenderTarget(Renderer_RenderTarget::bloom), 0, 1);
			cmdList->SetTexture(Renderer_BindingsSrv::tex3, GetRenderTarget(Renderer_RenderTarget::exposure_fusion_guide));
			cmdList->SetTexture(Renderer_BindingsSrv::tex4, GetRenderTarget(Renderer_RenderTarget::exposure_fusion_exposures), 0, 1);
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_depth, GetRenderTarget(Renderer_RenderTarget::gbuffer_depth));
			cmdList->SetBuffer(Renderer_BindingsUav::exposure_state, GetBuffer(Renderer_Buffer::ExposureState));

			// Tone curve: AgX, X-Plane 12's since 12.2 (tonemap.slang: 0 none, 1 AgX, 2 ACES fit, 3 Reinhard); bloom weight
			// and the normalization of the summed mips; fusion strength
			const float bloomWeight = bloomMips > 0 ? std::clamp(constants.bloomIntensity, 0.0f, 1.0f) : 0.0f;
			const float bloomNormalize = bloomMips > 0 ? 1.0f / static_cast<float>(bloomMips) : 0.0f;
			const float fusionStrength = fusion ? std::max(constants.fusionStrength, 0.0f) : 0.0f;
			m_Pcb_Pass_Cpu.SetF4Value(static_cast<float>(s_ToneCurve), bloomWeight, bloomNormalize, fusionStrength);
			cmdList->PushConstants(m_Pcb_Pass_Cpu);
		});
	}

	void Renderer::Pass_DebugView(CommandList *cmdList, ImageResource *out)
	{
		if (!ShaderManager::IsShaderAvailable(Renderer_Shader::debug_view))
			return;
		ImageResource *frame = GetRenderTarget(Renderer_RenderTarget::frame_render);
		frame->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
		out->SetLayout(Layout::ImageLayout::General, cmdList, 0, 0);
		Pass_Compute(cmdList, "debug_view", Renderer_Shader::debug_view, frame, out, [&]
		{
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_albedo, GetRenderTarget(Renderer_RenderTarget::gbuffer_color));
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_normal, GetRenderTarget(Renderer_RenderTarget::gbuffer_normal));
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_material, GetRenderTarget(Renderer_RenderTarget::gbuffer_material));
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_depth, GetRenderTarget(Renderer_RenderTarget::gbuffer_depth));
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_emissive, GetRenderTarget(Renderer_RenderTarget::gbuffer_emissive));
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_weather, GetRenderTarget(Renderer_RenderTarget::gbuffer_weather));
			BindShadows(cmdList);	// the cascade and cloud-shadow views
			BindAmbientOcclusion(cmdList);
			BindReflections(cmdList);
			m_Pcb_Pass_Cpu.SetF4Value(static_cast<float>(GetDebugView()), 0.0f, 0.0f, 0.0f);
			cmdList->PushConstants(m_Pcb_Pass_Cpu);
		});
	}

	void Renderer::Pass_Lut_BrdfSpecular(CommandList *cmdList)
	{
		ImageResource *tex_lut_brdf_specular = GetRenderTarget(Renderer_RenderTarget::lut_brdf_specular);
		Shader* shader_brdf_lut = GetValidComputeShader(Renderer_Shader::light_integration_brdf_specular_lut, "lut_brdf_specular");
		if (!shader_brdf_lut)
			return;

		{
			PipelineState pso;
			pso.name = "lut_brdf_specular";
			pso.shaders[static_cast<uint32_t>(StageType::Compute)] = shader_brdf_lut;
			cmdList->SetPipelineState(pso);

			cmdList->SetTexture(Renderer_BindingsUav::tex, tex_lut_brdf_specular);
			cmdList->Dispatch(tex_lut_brdf_specular);

			// permanent srv transition
			cmdList->InsertBarrier(tex_lut_brdf_specular->Get(),
								   tex_lut_brdf_specular->GetImageSpec().format,
								   0,
								   1,
								   1,
								   Layout::ImageLayout::ShaderRead);
		}
	}

	void Renderer::Pass_HiZ(CommandList *cmdList)
	{
		// renders major occluders to a depth buffer and builds a hi-z mip chain.
		// the indirect cull compute shader samples this for gpu-driven occlusion culling.
		// the depth texture is ALWAYS cleared to 0.0 (far plane, reverse-z) and the mip
		// chain is always rebuilt, even when occlusion is disabled or suppressed. this
		// guarantees the cull shader never reads stale/uninitialized depth, which would
		// cause non-deterministic culling artifacts depending on gpu memory contents.

		ImageResource *tex_occluders = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth_occluders);

		bool render_occluders = cvar_hiz_occlusion.GetValueAs<bool>() && !m_Is_Hiz_Suppressed;

		// always start the render pass so the depth texture is cleared to far plane (0.0).
		// without this, the blit and downscale would propagate stale depth into the hi-z
		// mip chain, causing the cull shader to incorrectly occlude objects.
		{
			PipelineState pso;
			pso.name = "occluders";
			pso.shaders[static_cast<uint32_t>(StageType::Vertex)] = ShaderManager::GetShader(Renderer_Shader::depth_prepass);
			pso.rasterizerState = GetRasterizerState(Renderer_RasterizerState::Solid);
			pso.blendState = GetBlendState(Renderer_BlendState::Off);
			pso.depthStencil_State = GetDepthStencilState(Renderer_DepthStencilState::ReadWrite);
			pso.renderTarget_DepthTexture = tex_occluders;
			pso.resolutionScale = true;
			pso.clearDepth = 0.0f;

			cmdList->SetPipelineState(pso);

			if (render_occluders)
			{
				for (uint32_t i = 0; i < m_DrawCalls_Prepass_Count; i++)
				{
					const Renderer_DrawCall &draw_call = m_DrawCalls_Prepass[i];

					if (!draw_call.isOccluder)
						continue;

				    Renderable *renderable = draw_call.renderable;
					MaterialAsset* materialAsset = renderable->GetMaterialAsset();
					if (!materialAsset)
						continue;

					CullMode cull_mode = static_cast<CullMode>(materialAsset->GetProperty(MaterialProperty::CullMode));
					cull_mode = (pso.rasterizerState->GetPolygonMode() == PolygonMode::Wireframe) ? CullMode::None : cull_mode;
					cmdList->SetCullMode(cull_mode);

					m_Pcb_Pass_Cpu.drawIndex = draw_call.drawData_Index;
					cmdList->PushConstants(m_Pcb_Pass_Cpu);

					cmdList->SetVertexBuffer(renderable->GetVertexBuffer(), nullptr);
					cmdList->SetIndexBuffer(renderable->GetIndexBuffer());

					cmdList->DrawIndexed(renderable->GetIndexCount(draw_call.lodIndex),
										 renderable->GetIndexOffset(draw_call.lodIndex),
										 renderable->GetVertexOffset(draw_call.lodIndex));
				}
			}
		}

		// hi-z mip chain (the min / max depth pyramid, reverse z; z_minmax.slang)
		Pass_DepthPyramid(cmdList, tex_occluders);
	}

	void Renderer::Pass_GpuScene_Occlusion(CommandList *cmdList)
	{
		// Between the GPU scene's two culling phases in the depth prepass: the min / max depth pyramid of the depth drawn so
		// far (the instances visible last frame and the CPU draws), then the late cull against its farthest depths. The
		// pyramid target is shared with the reflections (step 5 of ProduceFrame), which rebuild it from the finished depth.
		ImageResource *tex_depth = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth);
		ImageResource *tex_hiz = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth_occluders_hiz);
		if (Pass_DepthPyramid(cmdList, tex_depth))
			GpuScene::CullLate(cmdList, tex_hiz);
	}

	void Renderer::Pass_IndirectCull(CommandList *cmdList)
	{
		if (m_Indirect_DrawCount == 0)
			return;

		{
			ImageResource *tex_occluders_hiz = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth_occluders_hiz);

			PipelineState pso;
			pso.name = "indirect_cull";
			pso.shaders[static_cast<uint32_t>(StageType::Compute)] = ShaderManager::GetShader(Renderer_Shader::indirect_cull);
			cmdList->SetPipelineState(pso);

			cmdList->SetTexture(Renderer_BindingsSrv::tex, tex_occluders_hiz);

			// input
			cmdList->SetBuffer(Renderer_BindingsUav::indirect_draw_args, GetBuffer(Renderer_Buffer::IndirectDrawArgs));
			cmdList->SetBuffer(Renderer_BindingsUav::indirect_draw_data, GetBuffer(Renderer_Buffer::IndirectDrawData));

			// output
			cmdList->SetBuffer(Renderer_BindingsUav::indirect_draw_args_out,
							   GetBuffer(Renderer_Buffer::IndirectDrawArgsOut));
			cmdList->SetBuffer(Renderer_BindingsUav::indirect_draw_data_out,
							   GetBuffer(Renderer_Buffer::IndirectDrawDataOut));
			cmdList->SetBuffer(Renderer_BindingsUav::indirect_draw_count,
							   GetBuffer(Renderer_Buffer::IndirectDrawCount));

			m_Pcb_Pass_Cpu.SetF4Value(static_cast<float>(m_Indirect_DrawCount),
									  static_cast<float>(tex_occluders_hiz->GetImageSpec().mipCount - 1),
									  0.0f,
									  0.0f);
			cmdList->PushConstants(m_Pcb_Pass_Cpu);

			uint32_t thread_group_count = (m_Indirect_DrawCount + 255) / 256;
			cmdList->Dispatch(thread_group_count, 1, 1);

			// barrier: compute write -> vertex/indirect read
			cmdList->InsertBarrier(GetBuffer(Renderer_Buffer::IndirectDrawArgsOut));
			cmdList->InsertBarrier(GetBuffer(Renderer_Buffer::IndirectDrawDataOut));
			cmdList->InsertBarrier(GetBuffer(Renderer_Buffer::IndirectDrawCount));
		}
	}

	void Renderer::Pass_Depth_Prepass(CommandList *cmdList)
	{
		ImageResource *tex_depth = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth);
		ImageResource *tex_depth_output = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth_opaque_output);

		bool is_wireframe = cvar_wireframe.GetValueAs<bool>();
		RasterizerState *rasterizer_state = GetRasterizerState(Renderer_RasterizerState::Solid);
		rasterizer_state = is_wireframe ? GetRasterizerState(Renderer_RasterizerState::Wireframe) : rasterizer_state;

		{
			// indirect prepass (must match g-buffer indirect path)
			if (m_Indirect_DrawCount > 0)
			{
				PipelineState pso;
				pso.name = "depth_prepass_indirect";
				pso.shaders[static_cast<uint32_t>(StageType::Vertex)] = ShaderManager::GetShader(Renderer_Shader::depth_prepass_indirect);
				pso.rasterizerState = rasterizer_state;
				pso.blendState = GetBlendState(Renderer_BlendState::Off);
				pso.depthStencil_State = GetDepthStencilState(Renderer_DepthStencilState::ReadWrite);
				pso.vrsInputTexture = cvar_variable_rate_shading.GetValueAs<bool>() ? GetRenderTarget(Renderer_RenderTarget::shading_rate) : nullptr;
				pso.renderTarget_DepthTexture = tex_depth;
				pso.resolutionScale = true;
				pso.clearDepth = 0.0f;
				cmdList->SetPipelineState(pso);
				cmdList->SetIndexBuffer(GeometryBuffer::GetIndexBuffer());
				cmdList->SetBuffer(Renderer_BindingsUav::indirect_draw_data_out, GetBuffer(Renderer_Buffer::IndirectDrawDataOut));
				cmdList->SetCullMode(CullMode::Back);

				cmdList->DrawIndexedIndirectCount(GetBuffer(Renderer_Buffer::IndirectDrawArgsOut),
												  0, GetBuffer(Renderer_Buffer::IndirectDrawCount),
												  0, m_Indirect_DrawCount);
			}

			// GPU scene: one instanced indirect draw per bucket (gpu_cull.slang chose the instances and their LODs). With
			// occlusion culling these are the early pass's draws; the late pass's follow the CPU draws (below).
			bool depthWritten = m_Indirect_DrawCount > 0;
			const auto drawGpuScene = [&](const bool late)
			{
				PipelineState pso;
				pso.name = "depth_prepass_gpu";
				pso.shaders[static_cast<uint32_t>(StageType::Vertex)] = ShaderManager::GetShader(Renderer_Shader::depth_prepass_gpu);
				pso.rasterizerState = rasterizer_state;
				pso.blendState = GetBlendState(Renderer_BlendState::Off);
				pso.depthStencil_State = GetDepthStencilState(Renderer_DepthStencilState::ReadWrite);
				pso.vrsInputTexture = cvar_variable_rate_shading.GetValueAs<bool>() ? GetRenderTarget(Renderer_RenderTarget::shading_rate) : nullptr;
				pso.renderTarget_DepthTexture = tex_depth;
				pso.resolutionScale = true;

				for (const GpuScene::Bucket &bucket : GpuScene::GetBuckets())
				{
					if (bucket.commandCount == 0)
						continue;

					// alpha-tested buckets discard cut-out texels, the rest need no fragment shader
					pso.shaders[static_cast<uint32_t>(StageType::Fragment)] = bucket.alphaTested ? ShaderManager::GetShader(Renderer_Shader::depth_prepass_gpu) : nullptr;
					pso.clearDepth = depthWritten ? RHI_DEPTH_LOAD : 0.0f;
					cmdList->SetPipelineState(pso);
					cmdList->SetIndexBuffer(GeometryBuffer::GetIndexBuffer());
					cmdList->SetCullMode(is_wireframe ? CullMode::None : bucket.cullMode);
					DrawGpuSceneBucket(cmdList, bucket, !late, late);
					depthWritten = true;
				}
			};
			if (GpuScene::HasDraws())
				drawGpuScene(false);

			// cpu-driven path for remaining draws (tessellated, instanced, alpha-tested)
			{
				PipelineState pso;
				pso.name = "depth_prepass";
				pso.shaders[static_cast<uint32_t>(StageType::Vertex)] = ShaderManager::GetShader(Renderer_Shader::depth_prepass);
				pso.rasterizerState = rasterizer_state;
				pso.blendState = GetBlendState(Renderer_BlendState::Off);
				pso.depthStencil_State = GetDepthStencilState(Renderer_DepthStencilState::ReadWrite);
				pso.vrsInputTexture = cvar_variable_rate_shading.GetValueAs<bool>() ? GetRenderTarget(Renderer_RenderTarget::shading_rate) : nullptr;
				pso.renderTarget_DepthTexture = tex_depth;
				pso.resolutionScale = true;

				// load when an indirect path already wrote depth, otherwise this path owns the clear (reverse-z: 0 = far)
				const bool indirectActive = m_Indirect_DrawCount > 0;
				pso.clearDepth = depthWritten ? RHI_DEPTH_LOAD : 0.0f;

				bool pipelineSet = false;
				bool drew = false;

				for (uint32_t i = 0; i < m_DrawCalls_Prepass_Count; i++)
				{
					const Renderer_DrawCall &drawCall = m_DrawCalls_Prepass[i];
					Renderable *renderable = drawCall.renderable;
					MaterialAsset* materialAsset = renderable->GetMaterialAsset();
					if (!materialAsset || materialAsset->IsTransparent() || !drawCall.cameraVisible)
						continue;

					// skip draws the indirect path already covered
					if (indirectActive && !IsCpuDrivenDraw(drawCall, materialAsset))
						continue;
					{
						bool isAlphaTested = materialAsset->IsAlphaTested();
						bool isTessellated = materialAsset->GetProperty(MaterialProperty::Tessellation) > 0.0f;
						Shader *ps = isAlphaTested ? ShaderManager::GetShader(Renderer_Shader::depth_prepass) : nullptr;
						Shader *hs = isTessellated ? ShaderManager::GetShader(Renderer_Shader::tessellation) : nullptr;
						Shader *ds = isTessellated ? ShaderManager::GetShader(Renderer_Shader::tessellation) : nullptr;

						if (!pipelineSet || pso.shaders[static_cast<uint32_t>(StageType::Fragment)] != ps ||
							pso.shaders[static_cast<uint32_t>(StageType::TessellationControl)] != hs ||
							pso.shaders[static_cast<uint32_t>(StageType::TessellationEvaluation)] != ds)
						{
							pso.shaders[static_cast<uint32_t>(StageType::Fragment)] = ps;
							pso.shaders[static_cast<uint32_t>(StageType::TessellationControl)] = hs;
							pso.shaders[static_cast<uint32_t>(StageType::TessellationEvaluation)] = ds;
							cmdList->SetPipelineState(pso);
							pipelineSet = true;
						}
					}

					{
						bool hasColorTexture = materialAsset->HasTextureOfType(MaterialTextureType::Color);
						m_Pcb_Pass_Cpu.drawIndex = drawCall.drawData_Index;
						m_Pcb_Pass_Cpu.isTransparent = 0;
						m_Pcb_Pass_Cpu.materialIndex = materialAsset->GetIndex();
						m_Pcb_Pass_Cpu.SetF3Value(0.0f, hasColorTexture ? 1.0f : 0.0f, static_cast<float>(i));
						cmdList->PushConstants(m_Pcb_Pass_Cpu);
					}

					{
						CullMode cullMode = static_cast<CullMode>(materialAsset->GetProperty(MaterialProperty::CullMode));
						cullMode = (pso.rasterizerState->GetPolygonMode() == PolygonMode::Wireframe) ? CullMode::None : cullMode;
						cmdList->SetCullMode(cullMode);
						cmdList->SetVertexBuffer(renderable->GetVertexBuffer(), renderable->GetInstanceBuffer());
						cmdList->SetIndexBuffer(renderable->GetIndexBuffer());

						cmdList->DrawIndexed(renderable->GetIndexCount(drawCall.lodIndex),
											 drawCall.instanceCount,
											 renderable->GetIndexOffset(drawCall.lodIndex),
											 renderable->GetVertexOffset(drawCall.lodIndex),
											 drawCall.instanceIndex);

						pso.clearDepth = RHI_DEPTH_LOAD;
						drew = true;
					}
				}

				// nothing was drawn, but depth must still be cleared for the passes that read it
				if (!drew && !depthWritten)
				{
					pso.shaders[static_cast<uint32_t>(StageType::Fragment)] = nullptr;
					pso.shaders[static_cast<uint32_t>(StageType::TessellationControl)] = nullptr;
					pso.shaders[static_cast<uint32_t>(StageType::TessellationEvaluation)] = nullptr;
					cmdList->ClearPipelineStateRenderTargets(pso);
				}
			}

			// GPU scene, late pass (two-phase occlusion culling): what the early pass held back, culled against a Hi-Z
			// of the depth drawn so far
			if (GpuScene::HasDraws() && GpuScene::IsOcclusionActive())
			{
				Pass_GpuScene_Occlusion(cmdList);
				drawGpuScene(true);
			}

			float resolutionScale = cvar_resolution_scale.GetValue();
			cmdList->Blit(tex_depth, tex_depth_output, false, resolutionScale);

			// early transitions
			{
				tex_depth->SetLayout(Layout::ImageLayout::Attachment, cmdList, 0, 0);
				tex_depth_output->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
			}
		}
	}

	void Renderer::Pass_Drape_Depth(CommandList *cmdList)
	{
		bool any = false;
		for (uint32_t i = 0; i < m_DrawCall_Count && !any; i++)
			any = m_DrawCalls[i].drapeOrder != 0 && m_DrawCalls[i].cameraVisible;
		if (!any)
			return;

		PipelineState pso;
		pso.name = "drape_depth";
		pso.shaders[static_cast<uint32_t>(StageType::Vertex)] = ShaderManager::GetShader(Renderer_Shader::depth_prepass);
		pso.rasterizerState = cvar_wireframe.GetValueAs<bool>() ? GetRasterizerState(Renderer_RasterizerState::Wireframe) : GetRasterizerState(Renderer_RasterizerState::Solid);
		pso.blendState = GetBlendState(Renderer_BlendState::Off);
		pso.depthStencil_State = GetDepthStencilState(Renderer_DepthStencilState::ReadWrite);
		pso.renderTarget_DepthTexture = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth);
		pso.resolutionScale = true;
		pso.clearDepth = RHI_DEPTH_LOAD;

		bool pipelineSet = false;
		for (uint32_t i = 0; i < m_DrawCall_Count; i++)
		{
			const Renderer_DrawCall &drawCall = m_DrawCalls[i];
			Renderable *renderable = drawCall.renderable;
			MaterialAsset *materialAsset = renderable->GetMaterialAsset();
			if (drawCall.drapeOrder == 0 || !drawCall.cameraVisible || !materialAsset || materialAsset->IsTransparent())
				continue;

			// cut-out texels (and the blended texels the forward pass draws) leave the depth to what is below them
			const bool alphaTested = materialAsset->IsAlphaTested() || materialAsset->GetProperty(MaterialProperty::AlphaCutoff) > 0.0f;
			Shader *ps = alphaTested ? ShaderManager::GetShader(Renderer_Shader::depth_prepass) : nullptr;
			if (!pipelineSet || pso.shaders[static_cast<uint32_t>(StageType::Fragment)] != ps)
			{
				pso.shaders[static_cast<uint32_t>(StageType::Fragment)] = ps;
				cmdList->SetPipelineState(pso);
				pipelineSet = true;
				if (cmdList->IsStickyInvalid())
					return;
			}

			m_Pcb_Pass_Cpu.drawIndex = drawCall.drawData_Index;
			m_Pcb_Pass_Cpu.isTransparent = 0;
			m_Pcb_Pass_Cpu.materialIndex = materialAsset->GetIndex();
			m_Pcb_Pass_Cpu.SetF3Value(0.0f, materialAsset->HasTextureOfType(MaterialTextureType::Color) ? 1.0f : 0.0f, static_cast<float>(i));
			cmdList->PushConstants(m_Pcb_Pass_Cpu);

			const CullMode cullMode = static_cast<CullMode>(materialAsset->GetProperty(MaterialProperty::CullMode));
			cmdList->SetCullMode(pso.rasterizerState->GetPolygonMode() == PolygonMode::Wireframe ? CullMode::None : cullMode);
			cmdList->SetVertexBuffer(renderable->GetVertexBuffer(), renderable->GetInstanceBuffer());
			cmdList->SetIndexBuffer(renderable->GetIndexBuffer());
			cmdList->DrawIndexed(renderable->GetIndexCount(drawCall.lodIndex), drawCall.instanceCount, renderable->GetIndexOffset(drawCall.lodIndex),
								 renderable->GetVertexOffset(drawCall.lodIndex), drawCall.instanceIndex);
		}
	}

	void Renderer::Pass_GBuffer(CommandList *cmdList, const bool isTransparentPass)
	{
		ImageResource *texColor = GetRenderTarget(Renderer_RenderTarget::gbuffer_color);
		ImageResource *texNormal = GetRenderTarget(Renderer_RenderTarget::gbuffer_normal);
		ImageResource *texMaterial = GetRenderTarget(Renderer_RenderTarget::gbuffer_material);
		ImageResource *texVelocity = GetRenderTarget(Renderer_RenderTarget::gbuffer_velocity);
		ImageResource *texDepth = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth);
		ImageResource *texPick = GetRenderTarget(Renderer_RenderTarget::gbuffer_pick);	// editor picking ids (ScenePicking)
		ImageResource *texEmissive = GetRenderTarget(Renderer_RenderTarget::gbuffer_emissive);
		ImageResource *texWeather = GetRenderTarget(Renderer_RenderTarget::gbuffer_weather);

		{
			if (!isTransparentPass && m_Indirect_DrawCount > 0)
			{
				PipelineState pso;
				pso.name = "g_buffer_indirect";
				pso.shaders[static_cast<uint32_t>(StageType::Vertex)] = ShaderManager::GetShader(Renderer_Shader::gbuffer_indirect);
				pso.shaders[static_cast<uint32_t>(StageType::Fragment)] = ShaderManager::GetShader(Renderer_Shader::gbuffer_indirect);
				pso.blendState = GetBlendState(Renderer_BlendState::Off);
				pso.rasterizerState = cvar_wireframe.GetValueAs<bool>() ? GetRasterizerState(Renderer_RasterizerState::Wireframe)
										  : GetRasterizerState(Renderer_RasterizerState::Solid);
				pso.depthStencil_State = GetDepthStencilState(Renderer_DepthStencilState::ReadGreaterEqual);
				pso.vrsInputTexture = cvar_variable_rate_shading.GetValueAs<bool>() ? GetRenderTarget(Renderer_RenderTarget::shading_rate) : nullptr;
				pso.resolutionScale = true;
				pso.renderTarget_ColorTextures[0] = texColor;
				pso.renderTarget_ColorTextures[1] = texNormal;
				pso.renderTarget_ColorTextures[2] = texMaterial;
				pso.renderTarget_ColorTextures[3] = texVelocity;
				pso.renderTarget_ColorTextures[4] = texPick;
				pso.renderTarget_ColorTextures[5] = texEmissive;
				pso.renderTarget_ColorTextures[6] = texWeather;
				pso.renderTarget_DepthTexture = texDepth;
				pso.clearColor[0] = PipelineStateColor{.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 0.0f};
				pso.clearColor[1] = PipelineStateColor{.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 0.0f};
				pso.clearColor[2] = PipelineStateColor{.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 0.0f};
				pso.clearColor[3] = PipelineStateColor{.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 0.0f};
				pso.clearColor[4] = PipelineStateColor{.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 0.0f};	// pick id 0: nothing
				pso.clearColor[5] = PipelineStateColor{.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 0.0f};
				pso.clearColor[6] = PipelineStateColor{.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 0.0f};
				cmdList->SetPipelineState(pso);

				cmdList->SetIndexBuffer(GeometryBuffer::GetIndexBuffer());
				cmdList->SetBuffer(Renderer_BindingsUav::indirect_draw_data_out,
								   GetBuffer(Renderer_Buffer::IndirectDrawDataOut));
				cmdList->SetCullMode(CullMode::Back);

				cmdList->DrawIndexedIndirectCount(GetBuffer(Renderer_Buffer::IndirectDrawArgsOut),
												  0, GetBuffer(Renderer_Buffer::IndirectDrawCount),
												  0, m_Indirect_DrawCount);

				// update previous transforms for motion vectors
				for (uint32_t i = 0; i < m_DrawCall_Count; i++)
				{
					Renderer_DrawCall &draw_call = m_DrawCalls[i];
					if (draw_call.renderable->GetMaterialAsset() && !draw_call.renderable->GetMaterialAsset()->IsTransparent())
					{
						Entity *entity = draw_call.renderable->GetEntity();
						entity->SetMatrixPrevious(entity->GetMatrix());
					}
				}
			}

			// GPU scene (opaque pass): one instanced indirect draw per bucket, clearing the targets with the first
			bool gpuDrew = false;
			if (!isTransparentPass && GpuScene::HasDraws())
			{
				PipelineState pso;
				pso.name = "g_buffer_gpu";
				pso.shaders[static_cast<uint32_t>(StageType::Vertex)] = ShaderManager::GetShader(Renderer_Shader::gbuffer_gpu);
				pso.shaders[static_cast<uint32_t>(StageType::Fragment)] = ShaderManager::GetShader(Renderer_Shader::gbuffer_gpu);
				pso.blendState = GetBlendState(Renderer_BlendState::Off);
				pso.rasterizerState = cvar_wireframe.GetValueAs<bool>() ? GetRasterizerState(Renderer_RasterizerState::Wireframe)
										  : GetRasterizerState(Renderer_RasterizerState::Solid);
				pso.depthStencil_State = GetDepthStencilState(Renderer_DepthStencilState::ReadGreaterEqual);
				pso.vrsInputTexture = cvar_variable_rate_shading.GetValueAs<bool>() ? GetRenderTarget(Renderer_RenderTarget::shading_rate) : nullptr;
				pso.resolutionScale = true;
				pso.renderTarget_ColorTextures[0] = texColor;
				pso.renderTarget_ColorTextures[1] = texNormal;
				pso.renderTarget_ColorTextures[2] = texMaterial;
				pso.renderTarget_ColorTextures[3] = texVelocity;
				pso.renderTarget_ColorTextures[4] = texPick;
				pso.renderTarget_ColorTextures[5] = texEmissive;
				pso.renderTarget_ColorTextures[6] = texWeather;
				pso.renderTarget_DepthTexture = texDepth;

				const bool indirectWrote = m_Indirect_DrawCount > 0;
				for (const GpuScene::Bucket &bucket : GpuScene::GetBuckets())
				{
					if (bucket.commandCount == 0)
						continue;

					const PipelineStateColor clearOrLoad = (indirectWrote || gpuDrew) ? RHI_COLOR_LOAD : PipelineStateColor{.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 0.0f};
					for (PipelineStateColor &clearColor : pso.clearColor)
						clearColor = clearOrLoad;
					pso.clearDepth = RHI_DEPTH_LOAD;

					cmdList->SetPipelineState(pso);

					// The draws carry no per-draw constants, but gbuffer.slang reads the pass's opaque flag (cut-out
					// discard). Nothing may have pushed graphics constants yet this frame (the culls push compute ones),
					// and an undefined flag (e.g. the forward transparent pass's 1) wrote cut-out forest texels solid.
					m_Pcb_Pass_Cpu.drawIndex = 0;
					m_Pcb_Pass_Cpu.isTransparent = 0;
					m_Pcb_Pass_Cpu.materialIndex = 0;
					cmdList->PushConstants(m_Pcb_Pass_Cpu);

					cmdList->SetIndexBuffer(GeometryBuffer::GetIndexBuffer());
					cmdList->SetCullMode(cvar_wireframe.GetValueAs<bool>() ? CullMode::None : bucket.cullMode);
					DrawGpuSceneBucket(cmdList, bucket, true, true); // both culling phases' draws
					gpuDrew = true;
				}
			}

			{
				PipelineState pso;
				pso.name = isTransparentPass ? "g_buffer_transparent" : "g_buffer_tessellated";
				pso.shaders[static_cast<uint32_t>(StageType::Vertex)] = ShaderManager::GetShader(Renderer_Shader::gbuffer);
				pso.shaders[static_cast<uint32_t>(StageType::Fragment)] = ShaderManager::GetShader(Renderer_Shader::gbuffer);
				pso.blendState = GetBlendState(Renderer_BlendState::Off);
				pso.rasterizerState = cvar_wireframe.GetValueAs<bool>() ? GetRasterizerState(Renderer_RasterizerState::Wireframe)
										  : GetRasterizerState(Renderer_RasterizerState::Solid);
				pso.depthStencil_State = isTransparentPass ? GetDepthStencilState(Renderer_DepthStencilState::ReadWrite)
											 : GetDepthStencilState(Renderer_DepthStencilState::ReadGreaterEqual);
				pso.vrsInputTexture = cvar_variable_rate_shading.GetValueAs<bool>() ? GetRenderTarget(Renderer_RenderTarget::shading_rate) : nullptr;
				pso.resolutionScale = true;
				pso.renderTarget_ColorTextures[0] = texColor;
				pso.renderTarget_ColorTextures[1] = texNormal;
				pso.renderTarget_ColorTextures[2] = texMaterial;
				pso.renderTarget_ColorTextures[3] = texVelocity;
				pso.renderTarget_ColorTextures[4] = texPick;
				pso.renderTarget_ColorTextures[5] = texEmissive;
				pso.renderTarget_ColorTextures[6] = texWeather;
				pso.renderTarget_DepthTexture = texDepth;
				// load when an indirect path already cleared and wrote the g-buffer, otherwise clear on the first draw
				const bool indirectActive = !isTransparentPass && m_Indirect_DrawCount > 0;
				const PipelineStateColor clearOrLoad = (isTransparentPass || indirectActive || gpuDrew) ? RHI_COLOR_LOAD : PipelineStateColor{.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 0.0f};
				pso.clearColor[0] = clearOrLoad;
				pso.clearColor[1] = clearOrLoad;
				pso.clearColor[2] = clearOrLoad;
				pso.clearColor[3] = clearOrLoad;
				pso.clearColor[4] = clearOrLoad;
				pso.clearColor[5] = clearOrLoad;
				pso.clearColor[6] = clearOrLoad;

				bool pipeline_set = false;

				for (uint32_t i = 0; i < m_DrawCall_Count; i++)
				{
					const Renderer_DrawCall &draw_call = m_DrawCalls[i];
					Renderable *renderable = draw_call.renderable;
					MaterialAsset* materialAsset = renderable->GetMaterialAsset();
					if (!materialAsset || !draw_call.cameraVisible)
						continue;

					if (isTransparentPass)
					{
					    if (!materialAsset->IsTransparent())
							continue;
					}
					else
					{
					    if (materialAsset->IsTransparent() || draw_call.gpuDriven)
							continue;

						// skip draws the indirect path already covered
						if (indirectActive && !IsCpuDrivenDraw(draw_call, materialAsset))
							continue;
					}

					{
						bool is_tessellated = materialAsset->GetProperty(MaterialProperty::Tessellation) > 0.0f;
						Shader *tessControl = is_tessellated ? ShaderManager::GetShader(Renderer_Shader::tessellation) : nullptr;
						Shader *tessEval = is_tessellated ? ShaderManager::GetShader(Renderer_Shader::tessellation) : nullptr;

						if (!pipeline_set ||
							pso.shaders[static_cast<uint32_t>(StageType::TessellationControl)] != tessControl ||
							pso.shaders[static_cast<uint32_t>(StageType::TessellationEvaluation)] != tessEval)
						{
							pso.shaders[static_cast<uint32_t>(StageType::TessellationControl)] = tessControl;
							pso.shaders[static_cast<uint32_t>(StageType::TessellationEvaluation)] = tessEval;
							cmdList->SetPipelineState(pso);
							pipeline_set = true;
						}
					}

					{
						Entity *entity = renderable->GetEntity();
						m_Pcb_Pass_Cpu.drawIndex = draw_call.drawData_Index;
						m_Pcb_Pass_Cpu.isTransparent = isTransparentPass ? 1 : 0;
						m_Pcb_Pass_Cpu.materialIndex = materialAsset->GetIndex();
						cmdList->PushConstants(m_Pcb_Pass_Cpu);

						entity->SetMatrixPrevious(entity->GetMatrix());
					}

					{
						cmdList->SetCullMode(cvar_wireframe.GetValueAs<bool>() ? CullMode::None :
							static_cast<CullMode>(materialAsset->GetProperty(MaterialProperty::CullMode)));
						cmdList->SetVertexBuffer(renderable->GetVertexBuffer(), renderable->GetInstanceBuffer());
						cmdList->SetIndexBuffer(renderable->GetIndexBuffer());

						cmdList->DrawIndexed(renderable->GetIndexCount(draw_call.lodIndex),
											 draw_call.instanceCount,
											 renderable->GetIndexOffset(draw_call.lodIndex),
											 renderable->GetVertexOffset(draw_call.lodIndex),
											 draw_call.instanceIndex);

						// later pipeline changes in this pass must keep what was drawn
						pso.clearDepth = RHI_DEPTH_LOAD;
						for (PipelineStateColor& clearColor : pso.clearColor)
							clearColor = RHI_COLOR_LOAD;
					}
				}
			}

			// early transitions
			texColor->SetLayout(Layout::ImageLayout::General, cmdList, 0, 0);
			texNormal->SetLayout(Layout::ImageLayout::General, cmdList, 0, 0);
			texMaterial->SetLayout(Layout::ImageLayout::General, cmdList, 0, 0);
			texVelocity->SetLayout(Layout::ImageLayout::General, cmdList, 0, 0);
			texDepth->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);

			// editor picking: copy the regions the viewport asked for (the outline pass reads the ids later)
			if (!isTransparentPass && !IsRenderingExtraView())
				ScenePicking::RecordReadbacks(cmdList);
		}
	}

	void Renderer::Pass_Blit(CommandList *cmdList, ImageResource *in, ImageResource *out)
	{
		// compute blit: vulkan can't blit depth to float, amd uav requires float
		Shader *shader_c = ShaderManager::GetShader(Renderer_Shader::blit);

		// Ensure explicit image layout transitions even if descriptor-layout binding
		// is not yet fully wired for the compute pass.
		in->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
		out->SetLayout(Layout::ImageLayout::General, cmdList, 0, 0);

		{
			PipelineState pso;
			pso.name = "blit";
			pso.shaders[static_cast<uint32_t>(StageType::Compute)] = shader_c;
			cmdList->SetPipelineState(pso);

			cmdList->SetTexture(Renderer_BindingsUav::tex, out);
			cmdList->SetTexture(Renderer_BindingsSrv::tex, in);
			cmdList->Dispatch(out);
		}
	}

	void Renderer::Pass_Grid(CommandList *cmdList, ImageResource *out)
	{
		if (!cvar_grid.GetValueAs<bool>() || !s_GridVisible.load(std::memory_order_relaxed))
			return;

		if (!cmdList || cmdList->IsStickyInvalid())
			return;

		Shader *shader_vertex = ShaderManager::GetShader(Renderer_Shader::grid);
		Shader *shader_frag = ShaderManager::GetShader(Renderer_Shader::grid);
		if (!shader_vertex || !shader_frag)
			return;

		Mesh *quad = GetStandardMesh(MeshType::Quad);
		if (!quad)
		{
			GeometryBuffer::Initialize();
			GeometryBuffer::BuildIfDirty();
			quad = GetStandardMesh(MeshType::Quad);
		}

		Buffer* fallbackVb = nullptr;
		Buffer* fallbackIb = nullptr;
		if (!quad)
		{
			fallbackVb = GeometryBuffer::GetVertexBuffer();
			fallbackIb = GeometryBuffer::GetIndexBuffer();
		}

		if ((!quad || !quad->GetVertexBuffer() || !quad->GetIndexBuffer()) && (!fallbackVb || !fallbackIb))
		{
			static bool warnedOnce = false;
			if (!warnedOnce)
			{
				SEDX_CORE_WARN_TAG("Render Pass", "Pass_Grid skipped: quad mesh is not available");
				warnedOnce = true;
			}
			return;
		}

		PipelineState pso;
		pso.name = "grid";
		pso.shaders[static_cast<uint32_t>(StageType::Vertex)] = shader_vertex;
		pso.shaders[static_cast<uint32_t>(StageType::Fragment)] = shader_frag;
		pso.rasterizerState = GetRasterizerState(Renderer_RasterizerState::Solid);
		pso.blendState = GetBlendState(Renderer_BlendState::Alpha);
		pso.depthStencil_State = GetDepthStencilState(Renderer_DepthStencilState::ReadGreaterEqual);
		pso.renderTarget_ColorTextures[0] = out;
		pso.renderTarget_DepthTexture = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth);
		cmdList->SetPipelineState(pso);
		if (cmdList->IsStickyInvalid())
			return;

		// Half-extent of the grid quad in world units.  The vertex shader maps the unit XY quad
		// to the XZ ground plane and scales it by this value, producing a 2*k_GridHalfExtent square
		// centred on the snapped camera position.  Must be >= max_camera_dist in grid.slang (5000)
		// so the quad fully covers the camera-distance fade range.
		static constexpr float k_GridHalfExtent = 5000.0f;
		m_Pcb_Pass_Cpu.SetF3Value(k_GridHalfExtent, 0.0f, 0.0f);
		cmdList->PushConstants(m_Pcb_Pass_Cpu);

		if (!quad)
		{
			cmdList->SetCullMode(CullMode::None);
			cmdList->SetVertexBuffer(fallbackVb, nullptr);
			cmdList->SetIndexBuffer(fallbackIb);
			cmdList->DrawIndexed(6, 1, 0, 0);
			return;
		}

		// CullMode::None because the grid is a horizontal plane (y = 0).  After the
		// XY-to-XZ remap in the vertex shader the winding can appear reversed when the
		// camera goes below ground, so disable culling to keep it visible from both sides.
		cmdList->SetCullMode(CullMode::None);
		cmdList->SetVertexBuffer(quad->GetVertexBuffer(), nullptr);
		cmdList->SetIndexBuffer(quad->GetIndexBuffer());
		cmdList->DrawIndexed(6, 1, quad->GetGlobalIndexOffset(), quad->GetGlobalVertexOffset());
	}

	// -------------------------------------------------------
	// Forward passes over the composed frame
	// -------------------------------------------------------

	void Renderer::SetLightBillboards(std::vector<Renderer_LightBillboardBatch> batches)
	{
		std::scoped_lock lock(s_LightBillboardMutex);
		s_LightBillboards = std::move(batches);
	}

	const RendererFeatures &Renderer::GetFeatures()
	{
		return s_Features;
	}

	void Renderer::SetFeatures(const RendererFeatures &features)
	{
		s_Features = features;
	}

	Renderer_DebugView Renderer::GetDebugView()
	{
		return s_DebugView;
	}

	void Renderer::SetDebugView(const Renderer_DebugView view)
	{
		s_DebugView = view < Renderer_DebugView::Count ? view : Renderer_DebugView::None;
	}

	const RendererQualityPreset &Renderer::GetQualityPreset()
	{
		return s_Quality;
	}

	void Renderer::SetQualityPreset(const RendererQualityPreset &preset)
	{
		s_Quality = preset;
	}

	int Renderer::GetToneCurve()
	{
		return s_ToneCurve;
	}

	void Renderer::SetToneCurve(const int curve)
	{
		s_ToneCurve = std::clamp(curve, 1, 3);
	}

	const Renderer::EnvironmentConstants &Renderer::GetEnvironmentConstants()
	{
		return s_EnvironmentConstants;
	}

	Renderer::EnvironmentConstants &Renderer::EditEnvironmentConstants()
	{
		return s_EnvironmentConstants;
	}

	bool Renderer::LoadEnvironmentConstants(const std::filesystem::path &path, std::string *error)
	{
		// resources/environment/xp12_environment.json: { "controls": { "<art control>": { "value": v, "source": "..." } } }
		std::ifstream file(path);
		if (!file)
		{
			if (error)
				*error = "can't open " + path.string();
			return false;
		}
		try
		{
			const nlohmann::json root = nlohmann::json::parse(file);
			const nlohmann::json &controls = root.at("controls");
			const auto value = [&controls](const char *name, float &out)
			{
				const auto it = controls.find(name);
				if (it != controls.end() && it->contains("value") && (*it)["value"].is_number())
					out = (*it)["value"].get<float>();
			};
			EnvironmentConstants &c = s_EnvironmentConstants;

			// X-Plane art controls, by X-Plane's name (plan GUD-002)
			value("lighting/E_sun_lx", c.sunIlluminanceLux);
			value("lighting/E_moon_lx", c.moonIlluminanceLux);
			float sunDiameterDeg = c.sunAngularRadiusRad * 2.0f / xMath::DEG_TO_RAD;
			value("sun/angular_diameter", sunDiameterDeg);
			c.sunAngularRadiusRad = std::max(sunDiameterDeg, 0.01f) * 0.5f * xMath::DEG_TO_RAD;
			value("atmo/rayleigh_r", c.rayleigh.x);
			value("atmo/rayleigh_g", c.rayleigh.y);
			value("atmo/rayleigh_b", c.rayleigh.z);
			value("atmo/ozone_r", c.ozone.x);
			value("atmo/ozone_g", c.ozone.y);
			value("atmo/ozone_b", c.ozone.z);
			value("atmo/ozone_center", c.ozone.w);
			float ozoneWidth = c.planet.x * 2.0f;
			value("atmo/ozone_width", ozoneWidth);
			c.planet.x = std::max(ozoneWidth * 0.5f, 1.0f);
			value("atmo/fog_albedo", c.fogAlbedo);
			value("scattering/earth_albedo", c.planet.w);
			value("scattering/g_cornette_shanks", c.mie.w);
			float mieAlbedo = c.mie.y > 0.0f ? c.mie.x / c.mie.y : 0.9f;
			value("scattering/mie_albedo", mieAlbedo);
			c.mie.y = c.mie.x / std::clamp(mieAlbedo, 0.01f, 1.0f);
			value("scattering/mie_scattering_exponent", c.mieAngstrom);
			value("scattering/multi_rat", c.multipleScattering);
			value("scattering/vis_cutoff_ratio", c.visibilityContrast);
			value("exposure_fusion/dis_near", c.exposureFusion.z);
			value("exposure_fusion/dis_far", c.exposureFusion.w);
			float maxMip = static_cast<float>(c.fusionMaxMip);
			value("exposure_fusion/max_mip", maxMip);
			c.fusionMaxMip = std::clamp(static_cast<int>(std::lround(maxMip)), 1, 5);	// SEDX's pyramid stops at 5
			value("photometric/K", c.exposure.meterK);
			value("photometric/ev100_min", c.exposure.evMin);
			value("photometric/ev100_max", c.exposure.evMax);
			value("exposure/speed_up", c.exposure.speedUp);
			value("exposure/speed_down", c.exposure.speedDown);
			value("autoexposure/gain_lo", c.exposure.gainLow);
			value("autoexposure/gain_hi", c.exposure.gainHigh);
			value("autoexposure/_exp_radius", c.exposure.centerRadius);
			value("reno/rcas_rat", c.rcasSharpness);
			value("color_management/sdr/slope_red", c.colorSlope.x);
			value("color_management/sdr/slope_green", c.colorSlope.y);
			value("color_management/sdr/slope_blue", c.colorSlope.z);
			value("color_management/sdr/saturation", c.colorSlope.w);
			value("color_management/sdr/offset_red", c.colorOffset.x);
			value("color_management/sdr/offset_green", c.colorOffset.y);
			value("color_management/sdr/offset_blue", c.colorOffset.z);
			value("color_management/sdr/output_gain", c.colorOffset.w);
			value("color_management/sdr/power_red", c.colorPower.x);
			value("color_management/sdr/power_green", c.colorPower.y);
			value("color_management/sdr/power_blue", c.colorPower.z);
			value("color_management/sdr/output_gamma", c.colorPower.w);

			// Sun and cloud shadows (plan Phase 7). The shadow quality sets the cascade count, far limit, fade and map size
			// over these each frame, as X-Plane's renopt_shadow_quality does.
			CsmSettings &shadow = c.shadow;
			const auto valueD = [&value](const char *name, double &out)
			{
				float v = static_cast<float>(out);
				value(name, v);
				out = v;
			};
			float cascades = static_cast<float>(shadow.cascadeCount);
			value("shadow/csm_split_exterior", cascades);
			shadow.cascadeCount = static_cast<uint32_t>(std::clamp(std::lround(cascades), 1l, static_cast<long>(CSM_MAX_CASCADES)));
			valueD("shadow/csm/near_limit_exterior", shadow.nearLimitM);
			valueD("shadow/csm/far_limit_exterior", shadow.farLimitM);
			valueD("shadow/csm/lambda", shadow.lambda);
			valueD("shadow/overlap_ratio", shadow.overlapRatio);
			valueD("shadow/total_fade_ratio", shadow.fadeRatio);
			valueD("shadow/csm/pad_near_const", shadow.padNearConstM);
			valueD("shadow/csm/pad_near_ratio", shadow.padNearRatio);
			float mapSize = static_cast<float>(shadow.mapSize);
			value("fbo/shadow_cam_size", mapSize);
			shadow.mapSize = static_cast<uint32_t>(std::clamp(std::lround(mapSize), 256l, 8192l));
			value("shadow/bias_const", c.shadowBiasConst);
			value("shadow/bias_slope", c.shadowBiasSlope);
			value("shadow/bias_n_constant", c.shadowNormalBias);
			value("shadow/bias_n_slope", c.shadowNormalBiasSlope);
			float soft = c.shadowSoft ? 1.0f : 0.0f;
			value("shadow/want_soft", soft);
			c.shadowSoft = soft > 0.5f;
			value("clouds/cloud_shadow_lighten_ratio", c.cloudShadowStrength);
			value("cloud/shadow_alpha", c.cloudShadowExtinction);
			value("clouds/shadow_extent_min_km", c.cloudShadowExtentMinKm);
			value("clouds/shadow_extent_km", c.cloudShadowExtentKm);

			// Ambient occlusion (plan Phase 8). The AO quality sets ssao/quality_level and downsample over these each frame,
			// as X-Plane's renopt_SSAO_04 does.
			CacaoSettings &ao = c.ao;
			value("ssao/radius", ao.radius);
			value("ssao/shadow_multiplier", ao.shadowMultiplier);
			value("ssao/shadow_power", ao.shadowPower);
			value("ssao/shadow_clamp", ao.shadowClamp);
			value("ssao/horizon_angle_threshold", ao.horizonAngleThreshold);
			value("ssao/fade_out_from", ao.fadeOutFromM);
			value("ssao/fade_out_to", ao.fadeOutToM);
			value("ssao/fade_in_from", ao.fadeInFromM);
			value("ssao/fade_in_to", ao.fadeInToM);
			value("ssao/adaptive_quality_limit", ao.adaptiveQualityLimit);
			float blurPasses = static_cast<float>(ao.blurPassCount);
			value("ssao/blur_pass_count", blurPasses);
			ao.blurPassCount = static_cast<uint32_t>(std::clamp(std::lround(blurPasses), 0l, static_cast<long>(CACAO_MAX_BLUR_PASSES)));
			value("ssao/sharpness", ao.sharpness);
			value("ssao/detail_shadow_strength", ao.detailShadowStrength);
			value("ssao/bilateral_sigma_squared", ao.bilateralSigmaSquared);
			value("ssao/bilateral_similarity_distance_sigma", ao.bilateralSimilarityDistanceSigma);
			value("ssao/exterior_strength", ao.exteriorStrength);
			value("ssao/exterior_curve", ao.exteriorCurve);
			float dynamicValues = ao.dynamicValues ? 1.0f : 0.0f;
			value("ssao/dynamic_values", dynamicValues);
			ao.dynamicValues = dynamicValues > 0.5f;

			// Screen-space reflections (plan Phase 9)
			SsrSettings &ssr = c.ssr;
			value("ssr/min_gloss", ssr.minGloss);
			value("ssr/lod_bias", ssr.lodBias);
			float prefiltered = ssr.usePrefiltered ? 1.0f : 0.0f;
			value("ssr/use_prefiltered", prefiltered);
			ssr.usePrefiltered = prefiltered > 0.5f;
			float samples = static_cast<float>(ssr.sampleCount);
			value("ssr/sample_count", samples);
			ssr.sampleCount = static_cast<uint32_t>(std::clamp(std::lround(samples), 1l, 64l));

			// SEDX's own controls (no X-Plane counterpart, or one whose meaning differs)
			value("sedx/meter_trim_lo", c.exposure.trimLow);
			value("sedx/meter_trim_hi", c.exposure.trimHigh);
			value("sedx/exposure_compensation", c.exposure.compensation);
			value("sedx/fusion_shadows_ev", c.exposureFusion.x);
			value("sedx/fusion_sigma", c.exposureFusion.y);
			value("sedx/fusion_highlights_ev", c.fusionHighlights);
			value("sedx/bloom_intensity", c.bloomIntensity);
			value("sedx/sdr_slope_strength", c.colorSlopeStrength);
			value("sedx/aerial_range_m", c.aerialRangeM);
			value("sedx/airglow_nits", c.airglowNits);
			value("sedx/star_brightness", c.starBrightness);
			value("sedx/probe_specular_bias", c.probeSpecularBias);
			float receiverPlane = c.shadowReceiverPlane ? 1.0f : 0.0f;
			value("sedx/csm_receiver_plane", receiverPlane);
			c.shadowReceiverPlane = receiverPlane > 0.5f;
			value("sedx/csm_receiver_plane_slope_bias", c.shadowReceiverPlaneSlopeBias);
			value("sedx/ssr_gloss_fade", ssr.glossFade);
			float maxSteps = static_cast<float>(ssr.maxSteps);
			value("sedx/ssr_max_steps", maxSteps);
			ssr.maxSteps = static_cast<uint32_t>(std::clamp(std::lround(maxSteps), 1l, 1024l));
			value("sedx/ssr_thickness_m", ssr.thicknessM);
			value("sedx/ssr_thickness_ratio", ssr.thicknessRatio);
			value("sedx/ssr_edge_fade", ssr.edgeFade);
			value("sedx/ssr_history_clamp", ssr.historyClamp);
			value("sedx/ssr_lobe_scale", ssr.lobeScale);
			return true;
		}
		catch (const std::exception &e)
		{
			if (error)
				*error = e.what();
			return false;
		}
	}

	void Renderer::SetGridVisible(const bool visible)
	{
		s_GridVisible.store(visible, std::memory_order_relaxed);
	}

	bool Renderer::IsGridVisible()
	{
		return s_GridVisible.load(std::memory_order_relaxed);
	}

	void Renderer::SetReferenceOverlay(const Ref<ImageResource> &image, const ReferenceOverlayMode mode, const float split, const float differenceGain)
	{
		std::scoped_lock lock(s_ReferenceMutex);
		s_ReferenceImage = image;
		s_ReferenceMode = image ? mode : ReferenceOverlayMode::Off;
		s_ReferenceSplit = std::clamp(split, 0.0f, 1.0f);
		s_ReferenceGain = differenceGain;
	}

	void Renderer::Pass_ReferenceOverlay(CommandList *cmdList, ImageResource *out)
	{
		Ref<ImageResource> image;
		ReferenceOverlayMode mode;
		float split, gain;
		{
			std::scoped_lock lock(s_ReferenceMutex);
			image = s_ReferenceImage;
			mode = s_ReferenceMode;
			split = s_ReferenceSplit;
			gain = s_ReferenceGain;
		}
		if (mode == ReferenceOverlayMode::Off || !image || !image->Get() || *image->Get() == VK_NULL_HANDLE || !out)
			return;
		if (!cmdList || cmdList->IsStickyInvalid() || !ShaderManager::IsShaderAvailable(Renderer_Shader::reference_overlay))
			return;

		Pass_Compute(cmdList, "reference_overlay", Renderer_Shader::reference_overlay, image.Get(), out, [&]
		{
			m_Pcb_Pass_Cpu.SetF4Value(static_cast<float>(mode), split, gain, 0.0f);
			cmdList->PushConstants(m_Pcb_Pass_Cpu);
		});
	}

	void Renderer::DestroyForwardResources()
	{
		{
			std::scoped_lock lock(s_LightBillboardMutex);
			s_LightBillboards.clear();
		}

		for (Ref<Buffer> &buffer : s_BillboardBuffers)
			buffer.Reset();
		s_BillboardBufferCapacity.fill(0);
	}

	void Renderer::Pass_Forward_Transparent(CommandList *cmdList, ImageResource *out)
	{
		if (!cmdList || cmdList->IsStickyInvalid() || !out)
			return;

		// Blended drapes first, in drape order (their depth nudged by rank sorts them against the opaque drapes below),
		// then the rest back to front; equal distances keep creation order (an X-Plane object's file order).
		struct ForwardDraw
		{
			uint32_t drawCall;
			float distanceSquared;
			uint64_t order;
			uint64_t drapeOrder;
		};
		static std::vector<ForwardDraw> draws;
		draws.clear();

		const xMath::Vec3 camera = m_Cb_Frame_Cpu.camera_position;
		for (uint32_t i = 0; i < m_DrawCall_Count; i++)
		{
			const Renderer_DrawCall &drawCall = m_DrawCalls[i];
			MaterialAsset *materialAsset = drawCall.renderable->GetMaterialAsset();
			if (!materialAsset || !drawCall.cameraVisible || materialAsset->GetProperty(MaterialProperty::BlendMode) <= 0.0f)
				continue;

			// sub-mesh bounds center in world space (row-vector world matrix)
			Entity *entity = drawCall.renderable->GetEntity();
			const xMath::Matrix world = entity->GetMatrix();
			const xMath::Vec3 c = drawCall.renderable->GetBoundingBoxMesh().GetCenter();
			const xMath::Vec3 center(c.x * world.m00 + c.y * world.m10 + c.z * world.m20 + world.m30,
									 c.x * world.m01 + c.y * world.m11 + c.z * world.m21 + world.m31,
									 c.x * world.m02 + c.y * world.m12 + c.z * world.m22 + world.m32);
			const xMath::Vec3 d = center - camera;
			draws.push_back({ i, d.x * d.x + d.y * d.y + d.z * d.z, static_cast<uint64_t>(entt::to_integral(static_cast<entt::entity>(*entity))), drawCall.drapeOrder });
		}

		if (draws.empty())
			return;

		std::ranges::sort(draws, [](const ForwardDraw &a, const ForwardDraw &b)
		{
			if ((a.drapeOrder != 0) != (b.drapeOrder != 0))
				return a.drapeOrder != 0;
			if (a.drapeOrder != b.drapeOrder)
				return a.drapeOrder < b.drapeOrder;
			return a.distanceSquared != b.distanceSquared ? a.distanceSquared > b.distanceSquared : a.order < b.order;
		});

		Shader *shader = ShaderManager::GetShader(Renderer_Shader::forward_transparent);
		if (!shader || !shader->IsCompiled())
			return;

		PipelineState pso;
		pso.name = "forward_transparent";
		pso.shaders[static_cast<uint32_t>(StageType::Vertex)] = shader;
		pso.shaders[static_cast<uint32_t>(StageType::Fragment)] = shader;
		pso.rasterizerState = cvar_wireframe.GetValueAs<bool>() ? GetRasterizerState(Renderer_RasterizerState::Wireframe)
								  : GetRasterizerState(Renderer_RasterizerState::Solid);
		pso.blendState = GetBlendState(Renderer_BlendState::Premultiplied);
		pso.depthStencil_State = GetDepthStencilState(Renderer_DepthStencilState::ReadGreaterEqual); // test, don't write
		pso.renderTarget_ColorTextures[0] = out;
		pso.renderTarget_DepthTexture = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth);
		pso.clearColor[0] = RHI_COLOR_LOAD;
		cmdList->SetPipelineState(pso);
		if (cmdList->IsStickyInvalid())
			return;

		// The environment's light, as light_composition.slang gives the deferred surfaces (environment_lighting.slang)
		cmdList->SetTexture(Renderer_BindingsSrv::tex3, GetRenderTarget(Renderer_RenderTarget::lut_brdf_specular));
		BindAtmosphere(cmdList, ATMOSPHERE_LIGHTING);
		BindShadows(cmdList);
		BindReflections(cmdList);	// glossy blended surfaces trace their own (ssr_common.slang)
		for (const ForwardDraw &draw : draws)
		{
			const Renderer_DrawCall &drawCall = m_DrawCalls[draw.drawCall];
			Renderable *renderable = drawCall.renderable;
			MaterialAsset *materialAsset = renderable->GetMaterialAsset();

			m_Pcb_Pass_Cpu.drawIndex = drawCall.drawData_Index;
			m_Pcb_Pass_Cpu.isTransparent = 1;
			m_Pcb_Pass_Cpu.materialIndex = materialAsset->GetIndex();
			m_Pcb_Pass_Cpu.SetF4Value(m_Has_DirectionalLight ? 0.0f : 1.0f, 0.0f, 0.0f, 0.0f);
			cmdList->PushConstants(m_Pcb_Pass_Cpu);

			cmdList->SetCullMode(cvar_wireframe.GetValueAs<bool>() ? CullMode::None :
				static_cast<CullMode>(materialAsset->GetProperty(MaterialProperty::CullMode)));
			cmdList->SetVertexBuffer(renderable->GetVertexBuffer(), renderable->GetInstanceBuffer());
			cmdList->SetIndexBuffer(renderable->GetIndexBuffer());
			cmdList->DrawIndexed(renderable->GetIndexCount(drawCall.lodIndex),
								 drawCall.instanceCount,
								 renderable->GetIndexOffset(drawCall.lodIndex),
								 renderable->GetVertexOffset(drawCall.lodIndex),
								 drawCall.instanceIndex);
		}
	}

	void Renderer::Pass_Light_Billboards(CommandList *cmdList, ImageResource *out)
	{
		if (!cmdList || cmdList->IsStickyInvalid() || !out)
			return;

		// Two triangles per sprite (see light_billboard.slang for the vertex encoding), one range per texture.
		struct Range
		{
			Ref<ImageResource> texture;
			uint32_t first;
			uint32_t count;
		};
		static std::vector<Range> ranges;
		static std::vector<Vertex_PosTexNorTan> vertices;
		ranges.clear();
		vertices.clear();
		{
			std::scoped_lock lock(s_LightBillboardMutex);
			for (const Renderer_LightBillboardBatch &batch : s_LightBillboards)
			{
				if (!batch.texture || batch.billboards.empty())
					continue;

				const uint32_t first = static_cast<uint32_t>(vertices.size());
				for (const Renderer_LightBillboard &billboard : batch.billboards)
				{
					const float mode = billboard.tintLuminance ? 1.0f : (billboard.useAlpha ? 2.0f : 0.0f);
					const float cornerX[4] = { -1.0f, 1.0f, 1.0f, -1.0f };
					const float cornerY[4] = { -1.0f, -1.0f, 1.0f, 1.0f };
					const float s[4] = { billboard.uv[0], billboard.uv[2], billboard.uv[2], billboard.uv[0] };
					const float t[4] = { billboard.uv[1], billboard.uv[1], billboard.uv[3], billboard.uv[3] };
					for (const int k : { 0, 1, 2, 0, 2, 3 })
					{
						Vertex_PosTexNorTan &vertex = vertices.emplace_back();
						std::copy_n(billboard.position, 3, vertex.pos);
						vertex.tex[0] = s[k];
						vertex.tex[1] = t[k];
						std::copy_n(billboard.color, 3, vertex.nor);
						vertex.tan[0] = cornerX[k] * (1.0f + 2.0f * mode);
						vertex.tan[1] = cornerY[k];
						vertex.tan[2] = billboard.radius;
					}
				}
				ranges.push_back({ batch.texture, first, static_cast<uint32_t>(vertices.size()) - first });
			}
		}

		if (vertices.empty())
			return;

		Shader *shader = ShaderManager::GetShader(Renderer_Shader::light_billboard);
		if (!shader || !shader->IsCompiled())
			return;

		const uint32_t vertexCount = static_cast<uint32_t>(vertices.size());
		s_BillboardBufferIndex = (s_BillboardBufferIndex + 1) % BILLBOARD_BUFFER_COUNT;
		Ref<Buffer> &buffer = s_BillboardBuffers[s_BillboardBufferIndex];
		uint32_t &capacity = s_BillboardBufferCapacity[s_BillboardBufferIndex];
		if (!buffer || capacity < vertexCount)
		{
			capacity = std::max(vertexCount + vertexCount / 2, 6u * 256u);
			buffer = CreateRef<Buffer>(sizeof(Vertex_PosTexNorTan), capacity, nullptr, true, "light_billboards");
		}
		if (!buffer || !buffer->Get())
			return;
		cmdList->UpdateBuffer(buffer.Get(), 0, static_cast<uint64_t>(sizeof(Vertex_PosTexNorTan)) * vertexCount, vertices.data());

		PipelineState pso;
		pso.name = "light_billboards";
		pso.shaders[static_cast<uint32_t>(StageType::Vertex)] = shader;
		pso.shaders[static_cast<uint32_t>(StageType::Fragment)] = shader;
		pso.rasterizerState = GetRasterizerState(Renderer_RasterizerState::Solid);
		pso.blendState = GetBlendState(Renderer_BlendState::Additive);
		pso.depthStencil_State = GetDepthStencilState(Renderer_DepthStencilState::ReadGreaterEqual);
		pso.renderTarget_ColorTextures[0] = out;
		pso.renderTarget_DepthTexture = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth);
		pso.clearColor[0] = RHI_COLOR_LOAD;
		cmdList->SetPipelineState(pso);
		if (cmdList->IsStickyInvalid())
			return;

		cmdList->PushConstants(m_Pcb_Pass_Cpu);
		BindAtmosphere(cmdList, ATMOSPHERE_AERIAL);
		cmdList->SetCullMode(CullMode::None);
		cmdList->SetVertexBuffer(buffer.Get(), nullptr);
		for (const Range &range : ranges)
		{
			cmdList->SetTexture(Renderer_BindingsSrv::tex, range.texture.Get());
			cmdList->Draw(range.count, range.first);
		}
		ranges.clear(); // the batches own the textures; don't keep them alive from here
	}

	template <typename F>
	void Renderer::Pass_Compute(CommandList *cmdList, const char *name, Renderer_Shader shaderEnum, ImageResource *in, ImageResource *out, F setup)
	{
		Shader* shader_c = GetValidComputeShader(shaderEnum, name);
		if (!shader_c)
			return;

		{
			PipelineState pso;
			pso.name = name;
			pso.shaders[static_cast<uint32_t>(StageType::Compute)] = shader_c;
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
	
	void Renderer::Pass_ScreenSpaceAO(CommandList *cmdList)
	{
		if (!cvar_ssao.GetValueAs<bool>())
			return;

		ImageResource *tex_ssao = GetRenderTarget(Renderer_RenderTarget::ssao);
		if (!tex_ssao)
			return;

		PipelineState pso;
		pso.name = "screen_space_ambient_occlusion";
		Shader* shader_ssao = GetValidComputeShader(Renderer_Shader::ssao, pso.name);
		if (!shader_ssao)
			return;
		pso.shaders[static_cast<uint32_t>(StageType::Compute)] = shader_ssao;

		{
			// The g-buffer (not SetCommonTextures: it binds ssao too, which this pass writes)
			cmdList->SetPipelineState(pso);
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_albedo,   GetRenderTarget(Renderer_RenderTarget::gbuffer_color));
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_normal,   GetRenderTarget(Renderer_RenderTarget::gbuffer_normal));
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_material, GetRenderTarget(Renderer_RenderTarget::gbuffer_material));
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_velocity, GetRenderTarget(Renderer_RenderTarget::gbuffer_velocity));
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_depth,    GetRenderTarget(Renderer_RenderTarget::gbuffer_depth));
			cmdList->SetTexture(Renderer_BindingsUav::tex, tex_ssao);
			cmdList->Dispatch(tex_ssao, cvar_resolution_scale.GetValue());
		}
	}

	void Renderer::Pass_ScreenSpaceShadows(CommandList *cmdList)
	{
		static bool warnedDisabledSssBlend = false;
		if (!warnedDisabledSssBlend)
		{
			SEDX_CORE_WARN_TAG("Render Pass", "Pass 'screen_space_shadows' is disabled (sss_blend.slang temporarily disabled)");
			warnedDisabledSssBlend = true;
		}

		(void)cmdList;
		return;

		ImageResource *tex_sss = GetRenderTarget(Renderer_RenderTarget::sss);

		{
			cmdList->InsertBarrier(tex_sss, BarrierType::EnsureReadThenWrite);

			PipelineState pso;
			pso.name = "screen_space_shadows";
			Shader* shader_sss = GetValidComputeShader(Renderer_Shader::sss_bend, pso.name);
			if (!shader_sss)
				return;
			pso.shaders[static_cast<uint32_t>(StageType::Compute)] = shader_sss;
			cmdList->SetPipelineState(pso);

			cmdList->SetTexture(Renderer_BindingsSrv::tex, GetRenderTarget(Renderer_RenderTarget::gbuffer_depth));
			cmdList->SetTexture(Renderer_BindingsUav::tex_sss, tex_sss);
			float arraySliceIndex = 0.0f;
			for (Entity *entity : Scene::GetEntities())
			{
				Light *light = entity->GetComponent<Light>();
				if (!light || !light->GetFlag(LightFlags::ShadowsScreenSpace) || light->GetIntensityWatt() == 0.0f)
					continue;

				if (arraySliceIndex == static_cast<float>(tex_sss->GetImageSpec().depth))
				{
					SEDX_CORE_WARN_TAG("Render Pass",
									   "Render target has reached the maximum number of lights it can hold");
					break;
				}

				Camera *cam = Scene::GetCamera();
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

				float in_light_projection[] = {p.x, p.y, p.z, p.w};
				int32_t in_viewport_size[] = {static_cast<int32_t>(tex_sss->GetWidth()),
											  static_cast<int32_t>(tex_sss->GetHeight())};
				int32_t in_min_render_bounds[] = {0, 0};
				int32_t in_max_render_bounds[] = {static_cast<int32_t>(tex_sss->GetWidth()),
												  static_cast<int32_t>(tex_sss->GetHeight())};
				Bend::DispatchList dispatch_list = Bend::BuildDispatchList(in_light_projection,
																		   in_viewport_size,
																		   in_min_render_bounds,
																		   in_max_render_bounds,
																		   false);

				m_Pcb_Pass_Cpu.SetF4Value(dispatch_list.LightCoordinate_Shader[0],
										  dispatch_list.LightCoordinate_Shader[1],
										  dispatch_list.LightCoordinate_Shader[2],
										  dispatch_list.LightCoordinate_Shader[3]);

				light->SetScreenSpaceShadowsSliceIndex(static_cast<uint32_t>(arraySliceIndex));
				float near_val = 1.0f;
				float far_val = 0.0f;
				m_Pcb_Pass_Cpu.SetF3Value(near_val, far_val, arraySliceIndex++);
				m_Pcb_Pass_Cpu.SetF3Value2(1.0f / tex_sss->GetWidth(), 1.0f / tex_sss->GetHeight(), 0.0f);

				for (int32_t dispatch_index = 0; dispatch_index < dispatch_list.DispatchCount; ++dispatch_index)
				{
					const Bend::DispatchData &dispatch = dispatch_list.Dispatch[dispatch_index];
					m_Pcb_Pass_Cpu.SetF2Value(static_cast<float>(dispatch.WaveOffset_Shader[0]),
											  static_cast<float>(dispatch.WaveOffset_Shader[1]));
					cmdList->PushConstants(m_Pcb_Pass_Cpu);
					cmdList->Dispatch(dispatch.WaveCount[0], dispatch.WaveCount[1], dispatch.WaveCount[2]);
				}

				cmdList->InsertBarrier(tex_sss, BarrierType::EnsureWriteThenRead);
			}

			arraySliceIndex = 0;
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
			Shader* shader_cloud_shape = GetValidComputeShader(Renderer_Shader::cloud_noise_shape, pso.name);
			if (!shader_cloud_shape)
				return;
			pso.shaders[static_cast<uint32_t>(StageType::Compute)] = shader_cloud_shape;
			cmdList->SetPipelineState(pso);

			cmdList->SetTexture(Renderer_BindingsUav::tex3d, tex_shape);
			cmdList->Dispatch(tex_shape);

			tex_shape->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
		}

		// Detail noise – 32×32×32, RGBA16F
		{
			PipelineState pso;
			pso.name = "cloud_noise_detail";
			Shader* shader_cloud_detail = GetValidComputeShader(Renderer_Shader::cloud_noise_detail, pso.name);
			if (!shader_cloud_detail)
				return;
			pso.shaders[static_cast<uint32_t>(StageType::Compute)] = shader_cloud_detail;
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
			Shader* shader_vrs = GetValidComputeShader(Renderer_Shader::variable_rate_shading, pso.name);
			if (!shader_vrs)
				return;
			pso.shaders[static_cast<uint32_t>(StageType::Compute)] = shader_vrs;
			cmdList->SetPipelineState(pso);

			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_depth,    GetRenderTarget(Renderer_RenderTarget::gbuffer_depth));
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_velocity, GetRenderTarget(Renderer_RenderTarget::gbuffer_velocity));
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

		// The SPD shaders aren't ported yet (scale.slang has no entry point); depth pyramids are built by
		// Pass_DepthPyramid (z_minmax.slang) instead
		Renderer_Shader spdShader;
		switch (filter)
		{
			case Renderer_DownsampleFilter::Min:     spdShader = Renderer_Shader::ffx_spd_min;     break;
			case Renderer_DownsampleFilter::Max:     spdShader = Renderer_Shader::ffx_spd_max;     break;
			case Renderer_DownsampleFilter::Average:
			default:                                 spdShader = Renderer_Shader::ffx_spd_average; break;
		}

		{
			PipelineState pso;
			pso.name = "spd_downscale";
			Shader* shader_spd = GetValidComputeShader(spdShader, pso.name);
			if (!shader_spd)
				return;
			pso.shaders[static_cast<uint32_t>(StageType::Compute)] = shader_spd;
			cmdList->SetPipelineState(pso);

			// SPD reads mip-0 as SRV and writes mips 1..N as UAVs.
			cmdList->SetTexture(Renderer_BindingsSrv::tex, img, 0, 1);

			for (uint32_t mip = 1; mip < img->GetImageSpec().mipCount; mip++)
			{
				cmdList->SetTexture(Renderer_BindingsUav::tex, img, mip, 1);
			}

			// SPD counter reset buffer
			cmdList->SetBuffer(Renderer_BindingsUav::sb_spd, GetBuffer(Renderer_Buffer::SpdCounter));

			m_Pcb_Pass_Cpu.SetF4Value(
				static_cast<float>(img->GetImageSpec().mipCount - 1),
				static_cast<float>(img->GetWidth()),
				static_cast<float>(img->GetHeight()),
				0.0f
			);
			cmdList->PushConstants(m_Pcb_Pass_Cpu);

			// SPD uses fixed 256-thread groups; one per 64×64 tile at mip-0.
			const uint32_t dispatchX = (img->GetWidth()  + 63) / 64;
			const uint32_t dispatchY = (img->GetHeight() + 63) / 64;
			cmdList->Dispatch(dispatchX, dispatchY, 1);
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
		ImageResource* texShadowAtlas = GetRenderTarget(Renderer_RenderTarget::shadow_atlas);
		if (!texShadowAtlas)
			return;

		for (Entity* entity : Scene::GetEntities())
		{
			Light* light = entity->GetComponent<Light>();
			if (!light || !light->GetFlag(LightFlags::Shadows) || light->GetIntensityWatt() == 0.0f)
				continue;

			RasterizerState* rs = (light->GetLightType() == LightType::Directional)
				? GetRasterizerState(Renderer_RasterizerState::Light_directional) : GetRasterizerState(Renderer_RasterizerState::Light_point_spot);

			PipelineState pso;
			pso.name = "shadow_map";
			pso.shaders[static_cast<uint32_t>(StageType::Vertex)]    = ShaderManager::GetShader(Renderer_Shader::depth_light);
			pso.shaders[static_cast<uint32_t>(StageType::Fragment)]  = ShaderManager::GetShader(Renderer_Shader::depth_light);
			pso.rasterizerState               = rs;
			pso.blendState                    = GetBlendState(Renderer_BlendState::Off);
			pso.depthStencil_State            = GetDepthStencilState(Renderer_DepthStencilState::ReadWrite);
			pso.renderTarget_DepthTexture     = texShadowAtlas;
			pso.clearDepth                    = 0.0f; // reverse-z: far = 0

			cmdList->SetPipelineState(pso);
			cmdList->SetCullMode(CullMode::Back);

			for (uint32_t i = 0; i < m_DrawCalls_Prepass_Count; i++)
			{
				const Renderer_DrawCall& draw_call = m_DrawCalls_Prepass[i];
				Renderable* renderable             = draw_call.renderable;
				MaterialAsset* materialAsset = renderable->GetMaterialAsset();
				if (!materialAsset || materialAsset->IsTransparent() || !draw_call.cameraVisible)
					continue;

				m_Pcb_Pass_Cpu.drawIndex     = draw_call.drawData_Index;
				m_Pcb_Pass_Cpu.materialIndex = materialAsset->GetIndex();
				m_Pcb_Pass_Cpu.isTransparent = 0;
				cmdList->PushConstants(m_Pcb_Pass_Cpu);
				cmdList->SetVertexBuffer(renderable->GetVertexBuffer(), renderable->GetInstanceBuffer());
				cmdList->SetIndexBuffer(renderable->GetIndexBuffer());

				// compute lod index
				bool closeToShadow      = renderable->GetDistanceSquared() < 100.0f * 100.0f; // anything within 100 meters of the shadow caster
				uint32_t lodIndexBias   = light->GetLightType() == LightType::Directional ? 1 : 0; // bias for directional lights
				uint32_t lodIndexShadow = xMath::Clamp(renderable->GetLodIndex() + lodIndexBias, 0u, renderable->GetLodCount() - 1); // lod index biased towards lower quality lod
				uint32_t lodIndex       = closeToShadow ? draw_call.lodIndex : lodIndexShadow; // use normal lod if close to shadow caster, otherwise use light specific lod

				cmdList->DrawIndexed(
					renderable->GetIndexCount(draw_call.lodIndex),
					renderable->GetIndexOffset(draw_call.lodIndex),
					renderable->GetVertexOffset(draw_call.lodIndex),
					draw_call.instanceIndex,
					draw_call.instanceCount
				);
			}
		}

		texShadowAtlas->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
	}

	// -------------------------------------------------------
	// Pass_Light
	// Evaluates direct lighting from all active lights using the G-Buffer inputs.
	// Produces separate diffuse and specular accumulation buffers.
	// -------------------------------------------------------

	void Renderer::Pass_Light(CommandList* cmdList, const bool isTransparentPass)
	{
		ImageResource* texDiffuse  = GetRenderTarget(Renderer_RenderTarget::light_diffuse);
		ImageResource* texSpecular = GetRenderTarget(Renderer_RenderTarget::light_specular);
		Shader* shader_light = GetValidComputeShader(Renderer_Shader::light, isTransparentPass ? "light_transparent" : "light");
		if (!shader_light)
			return;

		{
			PipelineState pso;
			pso.name = isTransparentPass ? "light_transparent" : "light";
			pso.shaders[static_cast<uint32_t>(StageType::Compute)] = shader_light;
			cmdList->SetPipelineState(pso);

			// G-Buffer inputs
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_albedo,   GetRenderTarget(Renderer_RenderTarget::gbuffer_color));
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_normal,   GetRenderTarget(Renderer_RenderTarget::gbuffer_normal));
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_material, GetRenderTarget(Renderer_RenderTarget::gbuffer_material));
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_depth,    GetRenderTarget(Renderer_RenderTarget::gbuffer_depth));

			// Shadow atlas
			cmdList->SetTexture(Renderer_BindingsSrv::tex,  GetRenderTarget(Renderer_RenderTarget::shadow_atlas));

			// Ambient occlusion (white when none ran this frame)
			BindAmbientOcclusion(cmdList);

			// Screen-space shadows
			cmdList->SetTexture(Renderer_BindingsSrv::tex2, GetRenderTarget(Renderer_RenderTarget::sss));

			// Outputs
			cmdList->SetTexture(Renderer_BindingsUav::tex,  texDiffuse);
			cmdList->SetTexture(Renderer_BindingsUav::tex2, texSpecular);
			cmdList->SetTexture(Renderer_BindingsUav::tex3, GetRenderTarget(Renderer_RenderTarget::light_volumetric));

			// light.slang loops over pass_get_f3_value().x lights
			m_Pcb_Pass_Cpu.isTransparent = isTransparentPass ? 1u : 0u;
			m_Pcb_Pass_Cpu.SetF3Value(static_cast<float>(m_Count_ActiveLights), 0.0f, 0.0f);
			cmdList->PushConstants(m_Pcb_Pass_Cpu);

			cmdList->Dispatch(texDiffuse);
		}
	}

	// -------------------------------------------------------
	// Pass_Light_Composition
	// Combines albedo × diffuse + specular into the HDR frame_render buffer.
	// -------------------------------------------------------

	void Renderer::Pass_Light_Composition(CommandList* cmdList, const bool isTransparentPass)
	{
		ImageResource* texFrameRender = GetRenderTarget(Renderer_RenderTarget::frame_render);
		ImageResource* texDiffuse      = GetRenderTarget(Renderer_RenderTarget::light_diffuse);
		ImageResource* texSpecular     = GetRenderTarget(Renderer_RenderTarget::light_specular);
		Shader* shader_light_composition = GetValidComputeShader(Renderer_Shader::light_composition, isTransparentPass ? "light_composition_transparent" : "light_composition");
		if (!shader_light_composition)
			return;

		{
			PipelineState pso;
			pso.name = isTransparentPass ? "light_composition_transparent" : "light_composition";
			pso.shaders[static_cast<uint32_t>(StageType::Compute)] = shader_light_composition;
			cmdList->SetPipelineState(pso);

			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_albedo,   GetRenderTarget(Renderer_RenderTarget::gbuffer_color));
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_normal,   GetRenderTarget(Renderer_RenderTarget::gbuffer_normal));
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_material, GetRenderTarget(Renderer_RenderTarget::gbuffer_material));
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_depth,    GetRenderTarget(Renderer_RenderTarget::gbuffer_depth));
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_emissive, GetRenderTarget(Renderer_RenderTarget::gbuffer_emissive));
			cmdList->SetTexture(Renderer_BindingsSrv::tex,              texDiffuse);
			cmdList->SetTexture(Renderer_BindingsSrv::tex2,             texSpecular);
			cmdList->SetTexture(Renderer_BindingsSrv::tex3,             GetRenderTarget(Renderer_RenderTarget::lut_brdf_specular));
			cmdList->SetTexture(Renderer_BindingsUav::tex,              texFrameRender);
			BindAtmosphere(cmdList, ATMOSPHERE_LIGHTING);
			BindShadows(cmdList);
			BindAmbientOcclusion(cmdList);	// the ambient light's and the reflections' occlusion
			BindReflections(cmdList);		// the screen-space reflections over the probes

			// The environment's sun and moon light the scene unless it has a directional light of its own (Pass_Light
			// draws that one, in the same units); the sky and the ground always add their ambient light
			m_Pcb_Pass_Cpu.isTransparent = isTransparentPass ? 1u : 0u;
			m_Pcb_Pass_Cpu.SetF4Value(m_Has_DirectionalLight ? 0.0f : 1.0f, 0.0f, 0.0f, 0.0f);
			cmdList->PushConstants(m_Pcb_Pass_Cpu);

			cmdList->Dispatch(texFrameRender);
		}
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
			Shader* shader_particle_emit = GetValidComputeShader(Renderer_Shader::particle_emit, pso.name);
			if (!shader_particle_emit)
				return;
			pso.shaders[static_cast<uint32_t>(StageType::Compute)] = shader_particle_emit;
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
			Shader* shader_particle_sim = GetValidComputeShader(Renderer_Shader::particle_simulate, pso.name);
			if (!shader_particle_sim)
				return;
			pso.shaders[static_cast<uint32_t>(StageType::Compute)] = shader_particle_sim;
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
			Shader* shader_particle_render = GetValidComputeShader(Renderer_Shader::particle_render, pso.name);
			if (!shader_particle_render)
				return;
			pso.shaders[static_cast<uint32_t>(StageType::Compute)] = shader_particle_render;
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

		{
			PipelineState pso;
			pso.name = "transparency_reflection_refraction";
			Shader* shader_transparency = GetValidComputeShader(Renderer_Shader::transparency_reflection_refraction, pso.name);
			if (!shader_transparency)
				return;
			pso.shaders[static_cast<uint32_t>(StageType::Compute)] = shader_transparency;
			cmdList->SetPipelineState(pso);

			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_albedo,   GetRenderTarget(Renderer_RenderTarget::gbuffer_color));
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_normal,   GetRenderTarget(Renderer_RenderTarget::gbuffer_normal));
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_material, GetRenderTarget(Renderer_RenderTarget::gbuffer_material));
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_depth,    GetRenderTarget(Renderer_RenderTarget::gbuffer_depth));
			cmdList->SetTexture(Renderer_BindingsSrv::tex,              tex_frame_render_opaque);

			// tex2 held the old skysphere (gone with plan Phase 6); this pass isn't in the frame graph

			cmdList->SetTexture(Renderer_BindingsUav::tex, tex_frame_render);

			cmdList->Dispatch(tex_frame_render);
		}
	}

	// -------------------------------------------------------
	// Pass_Text
	// Rasterises 2D text/font glyphs onto the output render target.
	// Uses a dedicated vertex + fragment shader pair.
	// -------------------------------------------------------

	void Renderer::Pass_Text(CommandList* cmdList, ImageResource* out)
	{
		Shader* shader_v = ShaderManager::GetShader(Renderer_Shader::font);
		Shader* shader_f = ShaderManager::GetShader(Renderer_Shader::font);

		if (!shader_v || !shader_f || !out)
			return;

		Ref<Font> font = GetFont();
		if (!font || !font->HasText())
			return;

		font->UpdateVertexAndIndexBuffers(cmdList);

		{
			PipelineState pso;
			pso.name											= "text";
			pso.shaders[static_cast<uint32_t>(StageType::Vertex)]   = shader_v;
			pso.shaders[static_cast<uint32_t>(StageType::Fragment)] = shader_f;
			pso.rasterizerState									= GetRasterizerState(Renderer_RasterizerState::Solid);
			pso.blendState										= GetBlendState(Renderer_BlendState::Alpha);
			pso.depthStencil_State								= GetDepthStencilState(Renderer_DepthStencilState::Off);
			pso.renderTarget_ColorTextures[0]					= out;
			pso.clearColor[0]									= RHI_COLOR_LOAD;
			cmdList->SetPipelineState(pso);
			cmdList->SetVertexBuffer(font->GetVertexBuffer());
			cmdList->SetIndexBuffer(font->GetIndexBuffer());
			cmdList->SetCullMode(CullMode::Back);

			// draw outline
			if (font->GetOutline() != Font_Outline_None && font->GetOutlineSize() != 0)
			{
				m_Pcb_Pass_Cpu.SetF4Value(font->GetColorOutline());
				cmdList->PushConstants(m_Pcb_Pass_Cpu);
				cmdList->SetTexture(Renderer_BindingsSrv::tex, font->GetAtlasOutline().Get());
				cmdList->DrawIndexed(font->GetIndexCount());
			}

			// draw inline
			{
				m_Pcb_Pass_Cpu.SetF4Value(font->GetColor());
				cmdList->PushConstants(m_Pcb_Pass_Cpu);
				cmdList->SetTexture(Renderer_BindingsSrv::tex, font->GetAtlas().Get());
				cmdList->DrawIndexed(font->GetIndexCount());
			}
		}
	}

} // namespace SceneryEditorX

// -------------------------------------------------------

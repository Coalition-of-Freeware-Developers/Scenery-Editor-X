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
#include "gbuffer.h"
#include "renderer.h"
#include "renderer_declarations.h"
#include "SceneryEditorX/scene/entity.h"
#include "SceneryEditorX/scene/mesh.h"
#include "font/font.h"
#include "vulkan/enums.h"
#include "vulkan/rasterizer.h"
#include "vulkan/render_context.h"
#include "vulkan/pipeline/pipeline_state.h"
#include <SceneryEditorX/scene/camera.h>
#include <SceneryEditorX/scene/material.h>
#include <SceneryEditorX/scene/scene.h>
#include <SceneryEditorX/scene/components/lights.h>
#include <SceneryEditorX/scene/components/renderable.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	// Static draw call array definitions
	std::array<Renderer_DrawCall, RENDERER_MAX_DRAW_CALLS> Renderer::m_DrawCalls;
	std::array<Renderer_DrawCall, RENDERER_MAX_DRAW_CALLS> Renderer::m_DrawCalls_Prepass;

	// Console variable stubs – replace with real CVar system when available
	static CVar cvar_wireframe            { 0.0f };
	static CVar cvar_grid                 { 1.0f };
	static CVar cvar_resolution_scale     { 1.0f };
	static CVar cvar_variable_rate_shading{ 0.0f };
	static CVar cvar_hiz_occlusion        { 1.0f };
	static CVar cvar_ssao                 { 1.0f };
	static CVar cvar_tonemapping          { 0.0f };
	static CVar cvar_auto_exposure_adaptation_speed{ 1.0f };
	static CVar cvar_cloud_coverage       { 0.45f };

	void Renderer::ProduceFrame(CommandList *graphicsPresent, CommandList *compute)
	{
		ImageResource *rt_render = GetRenderTarget(Renderer_RenderTarget::frame_render);
		ImageResource *rt_output = GetRenderTarget(Renderer_RenderTarget::frame_output);

		const bool hasRtRender = rt_render && rt_render->Get() && *rt_render->Get() != VK_NULL_HANDLE;
		const bool hasRtOutput = rt_output && rt_output->Get() && *rt_output->Get() != VK_NULL_HANDLE;

		if (!hasRtRender || !hasRtOutput)
			return;

		const auto hasShader = [](const Renderer_Shader shader) { return GetShader(shader) != nullptr; };

		// Temporary bootstrap path while the full deferred shader table is still being wired.
		// This avoids a hard early-return and allows at least camera/grid visibility during integration.
		const bool deferredReady = false; // Force bootstrap path during integration.

		if (!deferredReady)
		{
			// Diagnostic clear so we can verify pass output is actually presented.
			graphicsPresent->ClearTexture(rt_render, Color(0.8f, 0.1f, 0.1f, 1.0f));

			const bool hasGridShaders = hasShader(Renderer_Shader::grid_vertex) && hasShader(Renderer_Shader::grid_frag);
			const bool hasBlitShader = false; // Temporarily disabled until CommandList pipeline binding is fully wired.
			static bool s_LoggedBootstrapState = false;
			if (!s_LoggedBootstrapState)
			{
				SEDX_CORE_WARN_TAG("Renderer", "Bootstrap state: grid shaders={}, blit shader={}", hasGridShaders, hasBlitShader);
				s_LoggedBootstrapState = true;
			}

			// Safety gate: graphics PSO binding (SetPipelineState for non-basic-pipeline PSOs) is
			// not yet fully wired in the bootstrap path.  Push-constant requirements and vertex
			// format are now correct — enable this once SetPipelineState correctly creates and
			// binds graphics pipelines from a PipelineState descriptor.
			const bool enableBootstrapGridPass = false;
			if (enableBootstrapGridPass && Scene::GetCamera() && hasGridShaders)
			{
				ImageResource *depthTarget = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth);
				rt_render->SetLayout(Layout::ImageLayout::Attachment, graphicsPresent, ALL_MIPS, 0);
				depthTarget->SetLayout(Layout::ImageLayout::Attachment, graphicsPresent, 0, 0);
				
				Pass_Grid(graphicsPresent, rt_render);

				depthTarget->SetLayout(Layout::ImageLayout::ShaderRead, graphicsPresent, 0, 0);
				rt_render->SetLayout(Layout::ImageLayout::ShaderRead, graphicsPresent, ALL_MIPS, 0);
			}

			if (hasBlitShader)
			{
				Pass_Blit(graphicsPresent, rt_render, rt_output);
			}
			else
			{
				graphicsPresent->Blit(rt_render, rt_output, false);
			}

			Pass_Text(graphicsPresent, rt_output);
			rt_output->SetLayout(Layout::ImageLayout::ShaderRead, graphicsPresent, 0, 0);

			// Keep command list recording open here.
			// Final frame orchestration owns submission/presentation ordering.
			return;
		}

		// brdf lut (once)
		if (!m_PassState.m_BRDF_LutProduced)
		{
			Pass_Lut_BrdfSpecular(graphicsPresent);
			m_PassState.m_BRDF_LutProduced = true;
		}

		// cloud noise (once)
		Pass_CloudNoise(graphicsPresent);

		// skysphere (re-render on light/coverage changes, converge over several frames)
		bool cloudsVisible = cvar_cloud_coverage.GetValue() > 0.0f;

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

		if (Camera *camera = Scene::GetCamera())
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
			GetRenderTarget(Renderer_RenderTarget::gbuffer_color)
				->SetLayout(Layout::ImageLayout::ShaderRead, graphicsPresent, 0, 0);
			GetRenderTarget(Renderer_RenderTarget::gbuffer_normal)
				->SetLayout(Layout::ImageLayout::ShaderRead, graphicsPresent, 0, 0);
			GetRenderTarget(Renderer_RenderTarget::gbuffer_material)
				->SetLayout(Layout::ImageLayout::ShaderRead, graphicsPresent, 0, 0);
			GetRenderTarget(Renderer_RenderTarget::gbuffer_velocity)
				->SetLayout(Layout::ImageLayout::ShaderRead, graphicsPresent, 0, 0);
			GetRenderTarget(Renderer_RenderTarget::gbuffer_depth)
				->SetLayout(Layout::ImageLayout::ShaderRead, graphicsPresent, 0, 0);

			// submit phase 1, signal g-buffer ready
			graphicsPresent->Submit(nullptr, false);

			uint64_t gfxPhase1TimelineValue = graphicsPresent->GetLastTimelineSignalValue();
			FrameSync *gfxTimeline = graphicsPresent->GetTimelineSemaphore();

			// async compute: overlaps with shadow rasterization.
			// Queue ownership transfers between graphics/compute image usage are not fully wired yet,
			// so only use async compute when both command lists target the same queue family.
			uint64_t computeTimelineValue = gfxPhase1TimelineValue;
			FrameSync *computeTimeline = gfxTimeline;

			const bool canUseAsyncCompute = compute && compute->GetQueue() && graphicsPresent->GetQueue() &&
				(compute->GetQueue()->GetFamilyIndex() == graphicsPresent->GetQueue()->GetFamilyIndex());

			if (canUseAsyncCompute)
			{
				/*if (cloudsVisible)
				{
					Pass_CloudShadow(compute);
				}*/

				Pass_ScreenSpaceAO(compute);
				Pass_ScreenSpaceShadows(compute);

				// submit compute, wait on phase 1
				compute->Submit(nullptr, false, nullptr, gfxTimeline, gfxPhase1TimelineValue);
				computeTimelineValue = compute->GetLastTimelineSignalValue();
				computeTimeline = compute->GetTimelineSemaphore();
			}
			else
			{
				static bool warnedNoOwnershipSync = false;
				if (!warnedNoOwnershipSync)
				{
					SEDX_CORE_WARN_TAG(
						"Renderer",
						"Async compute disabled: queue-family ownership/layout sync is not fully implemented yet");
					warnedNoOwnershipSync = true;
				}
			}

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
				graphicsPresent->Blit(GetRenderTarget(Renderer_RenderTarget::frame_render),
									  GetRenderTarget(Renderer_RenderTarget::frame_render_opaque),
									  false);
			}

			// particles
			Pass_Particles(graphicsPresent);

			// transparents
			if (m_Transparents_Present)
			{
				bool isTransparent = true;
				Pass_GBuffer(graphicsPresent, isTransparent);
				Pass_Light(graphicsPresent, isTransparent);
				Pass_Light_Composition(graphicsPresent, isTransparent);
			}

			Pass_Light_ImageBased(graphicsPresent);

			Pass_Light_Reflections(graphicsPresent);

			Pass_TransparencyReflectionRefraction(graphicsPresent);

			// Infinite editor grid: depth-test against render-resolution depth
			{
				ImageResource *depthTarget = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth);
				depthTarget->SetLayout(Layout::ImageLayout::Attachment, graphicsPresent, 0, 0);
				Pass_Grid(graphicsPresent, GetRenderTarget(Renderer_RenderTarget::frame_render));
				depthTarget->SetLayout(Layout::ImageLayout::ShaderRead, graphicsPresent, 0, 0);
			}
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
		GetRenderTarget(Renderer_RenderTarget::gbuffer_color)
			->SetLayout(Layout::ImageLayout::Attachment, graphicsPresent, 0, 0);
		GetRenderTarget(Renderer_RenderTarget::gbuffer_normal)
			->SetLayout(Layout::ImageLayout::Attachment, graphicsPresent, 0, 0);
		GetRenderTarget(Renderer_RenderTarget::gbuffer_material)
			->SetLayout(Layout::ImageLayout::Attachment, graphicsPresent, 0, 0);
		GetRenderTarget(Renderer_RenderTarget::gbuffer_velocity)
			->SetLayout(Layout::ImageLayout::Attachment, graphicsPresent, 0, 0);
		GetRenderTarget(Renderer_RenderTarget::gbuffer_depth)
			->SetLayout(Layout::ImageLayout::Attachment, graphicsPresent, 0, 0);
	}

	void Renderer::Pass_Lut_BrdfSpecular(CommandList *cmdList)
	{
		ImageResource *tex_lut_brdf_specular = GetRenderTarget(Renderer_RenderTarget::lut_brdf_specular);

		{
			PipelineState pso;
			pso.name = "lut_brdf_specular";
			pso.shaders[static_cast<uint32_t>(Stage::Compute)] =
				GetShader(Renderer_Shader::light_integration_brdf_specular_lut_c);
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

	void Renderer::Pass_Lut_AtmosphericScattering(CommandList *cmdList)
	{
		ImageResource *tex_lut_atmosphere_scatter = GetRenderTarget(Renderer_RenderTarget::lut_atmosphere_scatter);
		ImageResource *tex_lut_atmosphere_transmittance =
			GetRenderTarget(Renderer_RenderTarget::lut_atmosphere_transmittance);
		ImageResource *tex_lut_atmosphere_multiscatter =
			GetRenderTarget(Renderer_RenderTarget::lut_atmosphere_multiscatter);

		{
			// transmittance lut
			{
				PipelineState pso;
				pso.name = "lut_atmosphere_transmittance";
				pso.shaders[static_cast<uint32_t>(Stage::Compute)] =
					GetShader(Renderer_Shader::skysphere_transmittance_lut_c);
				cmdList->SetPipelineState(pso);

				cmdList->SetTexture(Renderer_BindingsUav::tex, tex_lut_atmosphere_transmittance);
				cmdList->Dispatch(tex_lut_atmosphere_transmittance);

				tex_lut_atmosphere_transmittance->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
			}

			// multi-scatter lut
			{
				PipelineState pso;
				pso.name = "lut_atmosphere_multiscatter";
				pso.shaders[static_cast<uint32_t>(Stage::Compute)] =
					GetShader(Renderer_Shader::skysphere_multiscatter_lut_c);
				cmdList->SetPipelineState(pso);

				cmdList->SetTexture(Renderer_BindingsSrv::tex, tex_lut_atmosphere_transmittance);
				cmdList->SetTexture(Renderer_BindingsUav::tex, tex_lut_atmosphere_multiscatter);
				cmdList->Dispatch(tex_lut_atmosphere_multiscatter);

				tex_lut_atmosphere_multiscatter->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
			}

			// legacy 3d lut
			{
				PipelineState pso;
				pso.name = "lut_atmospheric_scattering";
				pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::skysphere_lut_comp);
				cmdList->SetPipelineState(pso);

				cmdList->SetTexture(Renderer_BindingsUav::tex3d, tex_lut_atmosphere_scatter);
				cmdList->Dispatch(tex_lut_atmosphere_scatter);

				tex_lut_atmosphere_scatter->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
			}
		}
	}

	void Renderer::Pass_Skysphere(CommandList *cmdList)
	{
		ImageResource *tex_skysphere = GetRenderTarget(Renderer_RenderTarget::skysphere);
		ImageResource *tex_lut_atmosphere_scatter = GetRenderTarget(Renderer_RenderTarget::lut_atmosphere_scatter);
		ImageResource *tex_lut_atmosphere_transmittance = GetRenderTarget(Renderer_RenderTarget::lut_atmosphere_transmittance);
		ImageResource *tex_lut_atmosphere_multiscatter = GetRenderTarget(Renderer_RenderTarget::lut_atmosphere_multiscatter);
		ImageResource *tex_cloud_shape = GetRenderTarget(Renderer_RenderTarget::cloud_noise_shape);
		ImageResource *tex_cloud_detail = GetRenderTarget(Renderer_RenderTarget::cloud_noise_detail);

		if (Scene::GetDirectionalLight())
		{
			PipelineState pso;
			pso.name = "skysphere_atmospheric_scattering";
			pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::skysphere_comp);
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
			pso.name = "skysphere_filter";
			pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::light_integration_environment_filter_c);
			cmdList->SetPipelineState(pso);
			cmdList->SetTexture(Renderer_BindingsSrv::tex, tex_skysphere);

			for (uint32_t mip_level = 1; mip_level < tex_skysphere->GetImageSpec().mipCount; mip_level++)
			{
				cmdList->SetTexture(Renderer_BindingsUav::tex, tex_skysphere, mip_level, 1);

				m_Pcb_Pass_Cpu.SetF3Value(static_cast<float>(mip_level),
										  static_cast<float>(tex_skysphere->GetImageSpec().mipCount),
										  0.0f);
				cmdList->PushConstants(m_Pcb_Pass_Cpu);

				const uint32_t resolutionX = tex_skysphere->GetWidth() >> mip_level;
				const uint32_t resolutionY = tex_skysphere->GetHeight() >> mip_level;
				cmdList->Dispatch(resolutionX, resolutionY, 1);
				cmdList->InsertBarrier(tex_skysphere, BarrierType::EnsureWriteThenRead);
			}
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
		ImageResource *tex_occluders_hiz = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth_occluders_hiz);

		bool render_occluders = cvar_hiz_occlusion.GetValueAs<bool>() && !m_Is_Hiz_Suppressed;

		// always start the render pass so the depth texture is cleared to far plane (0.0).
		// without this, the blit and downscale would propagate stale depth into the hi-z
		// mip chain, causing the cull shader to incorrectly occlude objects.
		{
			PipelineState pso;
			pso.name = "occluders";
			pso.shaders[static_cast<uint32_t>(Stage::Vertex)] = GetShader(Renderer_Shader::depth_prepass_vertex);
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
					MaterialAsset* materialAsset = renderable->GetMaterial();
					Material* material = materialAsset ? materialAsset->GetMaterial().Get() : nullptr;
					if (!material)
						continue;

					CullMode cull_mode = static_cast<CullMode>(material->GetProperty(MaterialProperty::CullMode));
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

		// hi-z mip chain (min depth downsample, reverse z)
		Pass_Blit(cmdList, tex_occluders, tex_occluders_hiz);
		Pass_Downscale(cmdList, tex_occluders_hiz, Renderer_DownsampleFilter::Min);
	}

	void Renderer::Pass_IndirectCull(CommandList *cmdList)
	{
		if (m_Indirect_DrawCount == 0)
			return;

		{
			ImageResource *tex_occluders_hiz = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth_occluders_hiz);

			PipelineState pso;
			pso.name = "indirect_cull";
			pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::indirect_cull_c);
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
				pso.shaders[static_cast<uint32_t>(Stage::Vertex)] =
					GetShader(Renderer_Shader::depth_prepass_indirect_vertex);
				pso.rasterizerState = rasterizer_state;
				pso.blendState = GetBlendState(Renderer_BlendState::Off);
				pso.depthStencil_State = GetDepthStencilState(Renderer_DepthStencilState::ReadWrite);
				pso.vrsInputTexture = cvar_variable_rate_shading.GetValueAs<bool>()
										  ? GetRenderTarget(Renderer_RenderTarget::shading_rate)
										  : nullptr;
				pso.renderTarget_DepthTexture = tex_depth;
				pso.resolutionScale = true;
				pso.clearDepth = 0.0f;
				cmdList->SetPipelineState(pso);
				cmdList->SetIndexBuffer(GeometryBuffer::GetIndexBuffer());
				cmdList->SetBuffer(Renderer_BindingsUav::indirect_draw_data_out,
								   GetBuffer(Renderer_Buffer::IndirectDrawDataOut));
				cmdList->SetCullMode(CullMode::Back);

				cmdList->DrawIndexedIndirectCount(GetBuffer(Renderer_Buffer::IndirectDrawArgsOut),
												  0,
												  GetBuffer(Renderer_Buffer::IndirectDrawCount),
												  0,
												  m_Indirect_DrawCount);
			}

			// cpu-driven path for remaining draws (tessellated, instanced, alpha-tested)
			{
				PipelineState pso;
				pso.name = "depth_prepass";
				pso.shaders[static_cast<uint32_t>(Stage::Vertex)] = GetShader(Renderer_Shader::depth_prepass_vertex);
				pso.rasterizerState = rasterizer_state;
				pso.blendState = GetBlendState(Renderer_BlendState::Off);
				pso.depthStencil_State = GetDepthStencilState(Renderer_DepthStencilState::ReadWrite);
				pso.vrsInputTexture = cvar_variable_rate_shading.GetValueAs<bool>()
										  ? GetRenderTarget(Renderer_RenderTarget::shading_rate)
										  : nullptr;
				pso.renderTarget_DepthTexture = tex_depth;
				pso.resolutionScale = true;
				pso.clearDepth = RHI_DEPTH_LOAD; // load since indirect already wrote depth

				bool pipeline_set = false;

				for (uint32_t i = 0; i < m_DrawCalls_Prepass_Count; i++)
				{
					const Renderer_DrawCall &draw_call = m_DrawCalls_Prepass[i];
					Renderable *renderable = draw_call.renderable;
					MaterialAsset* materialAsset = renderable->GetMaterial();
					Material* material = materialAsset ? materialAsset->GetMaterial().Get() : nullptr;
					if (!materialAsset || !material || materialAsset->IsTransparent() || !draw_call.cameraVisible)
						continue;

					// skip indirect-path draws
					if (!IsCpuDrivenDraw(draw_call, material))
						continue;
					{
						bool is_alpha_tested = material->IsAlphaTested();
						bool is_tessellated = material->GetProperty(MaterialProperty::Tessellation) > 0.0f;
						Shader *ps = is_alpha_tested ? GetShader(Renderer_Shader::depth_prepass_alpha_test_frag) : nullptr;
						Shader *hs = is_tessellated ? GetShader(Renderer_Shader::tessellation_h) : nullptr;
						Shader *ds = is_tessellated ? GetShader(Renderer_Shader::tessellation_d) : nullptr;

						if (!pipeline_set || pso.shaders[static_cast<uint32_t>(Stage::Fragment)] != ps ||
							pso.shaders[static_cast<uint32_t>(Stage::TessellationControl)] != hs ||
							pso.shaders[static_cast<uint32_t>(Stage::TessellationEvaluation)] != ds)
						{
							pso.shaders[static_cast<uint32_t>(Stage::Fragment)] = ps;
							pso.shaders[static_cast<uint32_t>(Stage::TessellationControl)] = hs;
							pso.shaders[static_cast<uint32_t>(Stage::TessellationEvaluation)] = ds;
							cmdList->SetPipelineState(pso);
							pipeline_set = true;
						}
					}

					{
						bool has_color_texture = material->HasTextureOfType(MaterialTextureType::Color);
						m_Pcb_Pass_Cpu.drawIndex = draw_call.drawData_Index;
						m_Pcb_Pass_Cpu.isTransparent = 0;
						m_Pcb_Pass_Cpu.materialIndex = material->GetIndex();
						m_Pcb_Pass_Cpu.SetF3Value(0.0f, has_color_texture ? 1.0f : 0.0f, static_cast<float>(i));
						cmdList->PushConstants(m_Pcb_Pass_Cpu);
					}

					{
						CullMode cull_mode = static_cast<CullMode>(material->GetProperty(MaterialProperty::CullMode));
						cull_mode = (pso.rasterizerState->GetPolygonMode() == PolygonMode::Wireframe) ? CullMode::None
																									  : cull_mode;
						cmdList->SetCullMode(cull_mode);
						cmdList->SetVertexBuffer(renderable->GetVertexBuffer(), renderable->GetInstanceBuffer());
						cmdList->SetIndexBuffer(renderable->GetIndexBuffer());

						cmdList->DrawIndexed(renderable->GetIndexCount(draw_call.lodIndex),
											 renderable->GetIndexOffset(draw_call.lodIndex),
											 renderable->GetVertexOffset(draw_call.lodIndex),
											 draw_call.instanceIndex,
											 draw_call.instanceCount);

						pso.clearDepth = RHI_DEPTH_LOAD;
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
		ImageResource *tex_color = GetRenderTarget(Renderer_RenderTarget::gbuffer_color);
		ImageResource *tex_normal = GetRenderTarget(Renderer_RenderTarget::gbuffer_normal);
		ImageResource *tex_material = GetRenderTarget(Renderer_RenderTarget::gbuffer_material);
		ImageResource *tex_velocity = GetRenderTarget(Renderer_RenderTarget::gbuffer_velocity);
		ImageResource *tex_depth = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth);

		{
			if (!isTransparentPass && m_Indirect_DrawCount > 0)
			{
				PipelineState pso;
				pso.name = "g_buffer_indirect";
				pso.shaders[static_cast<uint32_t>(Stage::Vertex)] = GetShader(Renderer_Shader::gbuffer_indirect_vertex);
				pso.shaders[static_cast<uint32_t>(Stage::Fragment)] = GetShader(Renderer_Shader::gbuffer_indirect_frag);
				pso.blendState = GetBlendState(Renderer_BlendState::Off);
				pso.rasterizerState = cvar_wireframe.GetValueAs<bool>()
										  ? GetRasterizerState(Renderer_RasterizerState::Wireframe)
										  : GetRasterizerState(Renderer_RasterizerState::Solid);
				pso.depthStencil_State = GetDepthStencilState(Renderer_DepthStencilState::ReadGreaterEqual);
				pso.vrsInputTexture = cvar_variable_rate_shading.GetValueAs<bool>()
										  ? GetRenderTarget(Renderer_RenderTarget::shading_rate)
										  : nullptr;
				pso.resolutionScale = true;
				pso.renderTarget_ColorTextures[0] = tex_color;
				pso.renderTarget_ColorTextures[1] = tex_normal;
				pso.renderTarget_ColorTextures[2] = tex_material;
				pso.renderTarget_ColorTextures[3] = tex_velocity;
				pso.renderTarget_DepthTexture = tex_depth;
				pso.clearColor[0] = PipelineStateColor{0.0f, 0.0f, 0.0f, 0.0f};
				pso.clearColor[1] = PipelineStateColor{0.0f, 0.0f, 0.0f, 0.0f};
				pso.clearColor[2] = PipelineStateColor{0.0f, 0.0f, 0.0f, 0.0f};
				pso.clearColor[3] = PipelineStateColor{0.0f, 0.0f, 0.0f, 0.0f};
				cmdList->SetPipelineState(pso);

				cmdList->SetIndexBuffer(GeometryBuffer::GetIndexBuffer());
				cmdList->SetBuffer(Renderer_BindingsUav::indirect_draw_data_out,
								   GetBuffer(Renderer_Buffer::IndirectDrawDataOut));
				cmdList->SetCullMode(CullMode::Back);

				cmdList->DrawIndexedIndirectCount(GetBuffer(Renderer_Buffer::IndirectDrawArgsOut),
												  0,
												  GetBuffer(Renderer_Buffer::IndirectDrawCount),
												  0,
												  m_Indirect_DrawCount);

				// update previous transforms for motion vectors
				for (uint32_t i = 0; i < m_DrawCall_Count; i++)
				{
					Renderer_DrawCall &draw_call = m_DrawCalls[i];
					if (draw_call.renderable->GetMaterial() && !draw_call.renderable->GetMaterial()->IsTransparent())
					{
						Entity *entity = draw_call.renderable->GetEntity();
						entity->SetMatrixPrevious(entity->GetMatrix());
					}
				}
			}

			{
				PipelineState pso;
				pso.name = isTransparentPass ? "g_buffer_transparent" : "g_buffer_tessellated";
				pso.shaders[static_cast<uint32_t>(Stage::Vertex)] = GetShader(Renderer_Shader::gbuffer_vertex);
				pso.shaders[static_cast<uint32_t>(Stage::Fragment)] = GetShader(Renderer_Shader::gbuffer_frag);
				pso.blendState = GetBlendState(Renderer_BlendState::Off);
				pso.rasterizerState = cvar_wireframe.GetValueAs<bool>()
										  ? GetRasterizerState(Renderer_RasterizerState::Wireframe)
										  : GetRasterizerState(Renderer_RasterizerState::Solid);
				pso.depthStencil_State = isTransparentPass
											 ? GetDepthStencilState(Renderer_DepthStencilState::ReadWrite)
											 : GetDepthStencilState(Renderer_DepthStencilState::ReadGreaterEqual);
				pso.vrsInputTexture = cvar_variable_rate_shading.GetValueAs<bool>()
										  ? GetRenderTarget(Renderer_RenderTarget::shading_rate)
										  : nullptr;
				pso.resolutionScale = true;
				pso.renderTarget_ColorTextures[0] = tex_color;
				pso.renderTarget_ColorTextures[1] = tex_normal;
				pso.renderTarget_ColorTextures[2] = tex_material;
				pso.renderTarget_ColorTextures[3] = tex_velocity;
				pso.renderTarget_DepthTexture = tex_depth;
				pso.clearColor[0] = RHI_COLOR_LOAD;
				pso.clearColor[1] = RHI_COLOR_LOAD;
				pso.clearColor[2] = RHI_COLOR_LOAD;
				pso.clearColor[3] = RHI_COLOR_LOAD;

				bool pipeline_set = false;

				for (uint32_t i = 0; i < m_DrawCall_Count; i++)
				{
					const Renderer_DrawCall &draw_call = m_DrawCalls[i];
					Renderable *renderable = draw_call.renderable;
					MaterialAsset* materialAsset = renderable->GetMaterial();
					Material* material = materialAsset ? materialAsset->GetMaterial().Get() : nullptr;
					if (!materialAsset || !material || !draw_call.cameraVisible)
						continue;

					if (isTransparentPass)
					{
					 if (!materialAsset->IsTransparent())
							continue;
					}
					else
					{
					  if (materialAsset->IsTransparent())
							continue;

						if (!IsCpuDrivenDraw(draw_call, material))
							continue;
					}

					{
						bool is_tessellated = material->GetProperty(MaterialProperty::Tessellation) > 0.0f;
						Shader *tessControl = is_tessellated ? GetShader(Renderer_Shader::tessellation_h) : nullptr;
						Shader *tessEval = is_tessellated ? GetShader(Renderer_Shader::tessellation_d) : nullptr;

						if (!pipeline_set ||
							pso.shaders[static_cast<uint32_t>(Stage::TessellationControl)] != tessControl ||
							pso.shaders[static_cast<uint32_t>(Stage::TessellationEvaluation)] != tessEval)
						{
							pso.shaders[static_cast<uint32_t>(Stage::TessellationControl)] = tessControl;
							pso.shaders[static_cast<uint32_t>(Stage::TessellationEvaluation)] = tessEval;
							cmdList->SetPipelineState(pso);
							pipeline_set = true;
						}
					}

					{
						Entity *entity = renderable->GetEntity();
						m_Pcb_Pass_Cpu.drawIndex = draw_call.drawData_Index;
						m_Pcb_Pass_Cpu.isTransparent = isTransparentPass ? 1 : 0;
						m_Pcb_Pass_Cpu.materialIndex = material->GetIndex();
						cmdList->PushConstants(m_Pcb_Pass_Cpu);

						entity->SetMatrixPrevious(entity->GetMatrix());
					}

					{
						cmdList->SetCullMode(
							cvar_wireframe.GetValueAs<bool>()
								? CullMode::None
								: static_cast<CullMode>(material->GetProperty(MaterialProperty::CullMode)));
						cmdList->SetVertexBuffer(renderable->GetVertexBuffer(), renderable->GetInstanceBuffer());
						cmdList->SetIndexBuffer(renderable->GetIndexBuffer());

						cmdList->DrawIndexed(renderable->GetIndexCount(draw_call.lodIndex),
											 renderable->GetIndexOffset(draw_call.lodIndex),
											 renderable->GetVertexOffset(draw_call.lodIndex),
											 draw_call.instanceIndex,
											 draw_call.instanceCount);

						pso.clearDepth = RHI_DEPTH_LOAD;
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

	void Renderer::Pass_Output(CommandList *cmdList, ImageResource *in, ImageResource *out)
	{
		Shader *shader_c = GetShader(Renderer_Shader::output_c);

		PipelineState pso;
		pso.name = "output";
		pso.shaders[static_cast<uint32_t>(Stage::Compute)] = shader_c;
		cmdList->SetPipelineState(pso);

		m_Pcb_Pass_Cpu.SetF3Value(cvar_tonemapping.GetValue(), cvar_auto_exposure_adaptation_speed.GetValue(), 0.0f);
		cmdList->PushConstants(m_Pcb_Pass_Cpu);

		cmdList->SetTexture(Renderer_BindingsUav::tex, out);
		cmdList->SetTexture(Renderer_BindingsSrv::tex, in);
		cmdList->SetTexture(Renderer_BindingsSrv::tex2, GetRenderTarget(Renderer_RenderTarget::auto_exposure));
		cmdList->Dispatch(out);
	}

	void Renderer::Pass_Blit(CommandList *cmdList, ImageResource *in, ImageResource *out)
	{
		// compute blit: vulkan can't blit depth to float, amd uav requires float
		Shader *shader_c = GetShader(Renderer_Shader::blit_c);

		// Ensure explicit image layout transitions even if descriptor-layout binding
		// is not yet fully wired for the compute pass.
		in->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
		out->SetLayout(Layout::ImageLayout::General, cmdList, 0, 0);

		{
			PipelineState pso;
			pso.name = "blit";
			pso.shaders[static_cast<uint32_t>(Stage::Compute)] = shader_c;
			cmdList->SetPipelineState(pso);

			cmdList->SetTexture(Renderer_BindingsUav::tex, out);
			cmdList->SetTexture(Renderer_BindingsSrv::tex, in);
			cmdList->Dispatch(out);
		}
	}

	void Renderer::Pass_Grid(CommandList *cmdList, ImageResource *out)
	{
		if (!cvar_grid.GetValueAs<bool>())
			return;

		Shader *shader_vertex = GetShader(Renderer_Shader::grid_vertex);
		Shader *shader_frag = GetShader(Renderer_Shader::grid_frag);
		if (!shader_vertex || !shader_frag)
			return;

		PipelineState pso;
		pso.name = "grid";
		pso.shaders[static_cast<uint32_t>(Stage::Vertex)] = shader_vertex;
		pso.shaders[static_cast<uint32_t>(Stage::Fragment)] = shader_frag;
		pso.rasterizerState = GetRasterizerState(Renderer_RasterizerState::Solid);
		pso.blendState = GetBlendState(Renderer_BlendState::Alpha);
		pso.depthStencil_State = GetDepthStencilState(Renderer_DepthStencilState::ReadGreaterEqual);
		pso.renderTarget_ColorTextures[0] = out;
		pso.renderTarget_DepthTexture = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth);
		cmdList->SetPipelineState(pso);

		// Half-extent of the grid quad in world units.  The vertex shader maps the unit XY quad
		// to the XZ ground plane and scales it by this value, producing a 2*k_GridHalfExtent square
		// centred on the snapped camera position.  Must be >= max_camera_dist in grid.slang (5000)
		// so the quad fully covers the camera-distance fade range.
		static constexpr float k_GridHalfExtent = 5000.0f;
		m_Pcb_Pass_Cpu.SetF3Value(k_GridHalfExtent, 0.0f, 0.0f);
		cmdList->PushConstants(m_Pcb_Pass_Cpu);

		Mesh *quad = GetStandardMesh(MeshType::Quad);
		if (!quad || !quad->GetVertexBuffer() || !quad->GetIndexBuffer())
		{
			static bool warnedOnce = false;
			if (!warnedOnce)
			{
				SEDX_CORE_WARN_TAG("Render Pass", "Pass_Grid skipped: quad mesh is not available");
				warnedOnce = true;
			}
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

	template <typename F>
	void Renderer::Pass_Compute(CommandList *cmdList, const char *name, Renderer_Shader shaderEnum, ImageResource *in, ImageResource *out, F setup)
	{
		{
			PipelineState pso;
			pso.name = name;
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
	
	void Renderer::Pass_ScreenSpaceAO(CommandList *cmdList)
	{
		if (!cvar_ssao.GetValueAs<bool>())
			return;

		ImageResource *tex_ssao = GetRenderTarget(Renderer_RenderTarget::ssao);
		if (!tex_ssao)
			return;

		PipelineState pso;
		pso.name = "screen_space_ambient_occlusion";
		pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::ssao_comp);

		{
			cmdList->SetPipelineState(pso);
			SetCommonTextures(cmdList);
			cmdList->SetTexture(Renderer_BindingsUav::tex, tex_ssao);
			cmdList->Dispatch(tex_ssao, cvar_resolution_scale.GetValue());
		}
	}

	void Renderer::Pass_ScreenSpaceShadows(CommandList *cmdList)
	{
		ImageResource *tex_sss = GetRenderTarget(Renderer_RenderTarget::sss);

		{
			cmdList->InsertBarrier(tex_sss, BarrierType::EnsureReadThenWrite);

			PipelineState pso;
			pso.name = "screen_space_shadows";
			pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::sss_c_bend);
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
	// Pass_CloudShadow
	// Projects cloud shadow onto a 2D shadow map consumed by lighting passes.
	// Requires cloud noise textures to already be in ShaderRead layout.
	// -------------------------------------------------------

	void Renderer::Pass_CloudShadow(CommandList* cmdList)
	{
		ImageResource* texCloudShadow = GetRenderTarget(Renderer_RenderTarget::cloud_shadow);
		ImageResource* texCloudShape  = GetRenderTarget(Renderer_RenderTarget::cloud_noise_shape);
		ImageResource* texCloudDetail = GetRenderTarget(Renderer_RenderTarget::cloud_noise_detail);

		if (!texCloudShadow || !texCloudShape)
			return;

		{
			PipelineState pso;
			pso.name = "cloud_shadow";
			pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::cloud_shadow_c);
			cmdList->SetPipelineState(pso);

			cmdList->SetTexture(Renderer_BindingsSrv::tex3d_cloud_shape, texCloudShape);
			if (texCloudDetail)
				cmdList->SetTexture(Renderer_BindingsSrv::tex3d_cloud_detail, texCloudDetail);

			cmdList->SetTexture(Renderer_BindingsUav::tex, texCloudShadow);
			cmdList->Dispatch(texCloudShadow);

			texCloudShadow->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
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
				? GetRasterizerState(Renderer_RasterizerState::Light_directional)
				: GetRasterizerState(Renderer_RasterizerState::Light_point_spot);

			PipelineState pso;
			pso.name = "shadow_map";
			pso.shaders[static_cast<uint32_t>(Stage::Vertex)]    = GetShader(Renderer_Shader::depth_light_vertex);
			pso.shaders[static_cast<uint32_t>(Stage::Fragment)]  = GetShader(Renderer_Shader::depth_light_alpha_color_frag);
			pso.rasterizerState               = rs;
			pso.blendState                    = GetBlendState(Renderer_BlendState::Off);
			pso.depthStencil_State            = GetDepthStencilState(Renderer_DepthStencilState::ReadWrite);
			pso.renderTarget_DepthTexture    = texShadowAtlas;
			pso.clearDepth                    = 0.0f; // reverse-z: far = 0

			cmdList->SetPipelineState(pso);
			cmdList->SetCullMode(CullMode::Back);

			for (uint32_t i = 0; i < m_DrawCalls_Prepass_Count; i++)
			{
				const Renderer_DrawCall& draw_call = m_DrawCalls_Prepass[i];
				Renderable* renderable             = draw_call.renderable;
				MaterialAsset* materialAsset = renderable->GetMaterial();
				Material* material = materialAsset ? materialAsset->GetMaterial().Get() : nullptr;
				if (!materialAsset || !material || materialAsset->IsTransparent() || !draw_call.cameraVisible)
					continue;

				m_Pcb_Pass_Cpu.drawIndex     = draw_call.drawData_Index;
				m_Pcb_Pass_Cpu.materialIndex = material->GetIndex();
				cmdList->PushConstants(m_Pcb_Pass_Cpu);

				cmdList->SetVertexBuffer(renderable->GetVertexBuffer(), renderable->GetInstanceBuffer());
				cmdList->SetIndexBuffer(renderable->GetIndexBuffer());
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
			if (ImageResource* texSsao = GetRenderTarget(Renderer_RenderTarget::ssao))
				cmdList->SetTexture(Renderer_BindingsSrv::ssao, texSsao);

			// Screen-space shadows
			cmdList->SetTexture(Renderer_BindingsSrv::tex2, GetRenderTarget(Renderer_RenderTarget::sss));

			// Outputs
			cmdList->SetTexture(Renderer_BindingsUav::tex,  texDiffuse);
			cmdList->SetTexture(Renderer_BindingsUav::tex2, texSpecular);

			m_Pcb_Pass_Cpu.isTransparent = isTransparentPass ? 1u : 0u;
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

		{
			PipelineState pso;
			pso.name = isTransparentPass ? "light_composition_transparent" : "light_composition";
			pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::light_composition_c);
			cmdList->SetPipelineState(pso);

			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_albedo,   GetRenderTarget(Renderer_RenderTarget::gbuffer_color));
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_material, GetRenderTarget(Renderer_RenderTarget::gbuffer_material));
			cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_depth,    GetRenderTarget(Renderer_RenderTarget::gbuffer_depth));
			cmdList->SetTexture(Renderer_BindingsSrv::tex,              texDiffuse);
			cmdList->SetTexture(Renderer_BindingsSrv::tex2,             texSpecular);
			cmdList->SetTexture(Renderer_BindingsUav::tex,              texFrameRender);

			m_Pcb_Pass_Cpu.isTransparent = isTransparentPass ? 1u : 0u;
			cmdList->PushConstants(m_Pcb_Pass_Cpu);

			cmdList->Dispatch(texFrameRender);
		}
	}

	// -------------------------------------------------------
	// Pass_Light_ImageBased
	// Applies specular IBL from the filtered skysphere and the BRDF split-sum LUT.
	// -------------------------------------------------------

	void Renderer::Pass_Light_ImageBased(CommandList* cmdList)
	{
		ImageResource* texSkysphere    = GetRenderTarget(Renderer_RenderTarget::skysphere);
		ImageResource* texLutBrdf     = GetRenderTarget(Renderer_RenderTarget::lut_brdf_specular);
		ImageResource* texFrameRender = GetRenderTarget(Renderer_RenderTarget::frame_render);
		if (!texSkysphere || !texLutBrdf)
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
			cmdList->SetTexture(Renderer_BindingsSrv::tex,              texSkysphere);
			cmdList->SetTexture(Renderer_BindingsSrv::tex2,             texLutBrdf);
			cmdList->SetTexture(Renderer_BindingsUav::tex,              texFrameRender);

			cmdList->Dispatch(texFrameRender);
		}
	}

	// -------------------------------------------------------
	// Pass_Light_Reflections
	// Screen-space reflections (SSR) that complement IBL for rough materials.
	// Outputs a separate reflections buffer that is blended into frame_render.
	// -------------------------------------------------------

	void Renderer::Pass_Light_Reflections(CommandList* cmdList)
	{
		ImageResource* texReflections  = GetRenderTarget(Renderer_RenderTarget::reflections);
		ImageResource* texFrameRender = GetRenderTarget(Renderer_RenderTarget::frame_render);
		if (!texReflections)
			return;

		// Guard: clear reflections if the feature was just toggled off
		if (m_PassState.m_ClearedReflections)
		{
			cmdList->ClearTexture(texReflections, Color::Black());
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
			cmdList->SetTexture(Renderer_BindingsSrv::tex,              texFrameRender);
			cmdList->SetTexture(Renderer_BindingsUav::tex,              texReflections);

			cmdList->Dispatch(texReflections);
		}

		texReflections->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);

		// Composite reflections into the frame buffer using the blit pass
		Pass_Compute(cmdList, "reflections_composite", Renderer_Shader::blit_c, texReflections, texFrameRender, nullptr);
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
	// Full post-processing stack: bloom, tone-mapping, film grain, dithering, outline.
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
		Pass_Compute(cmdList, "film_grain",  Renderer_Shader::film_grain_comp,   tex_output, tex_output, nullptr);

		// Selection outline (optional)
		{
			ImageResource* tex_outline = GetRenderTarget(Renderer_RenderTarget::outline);
			if (tex_outline)
			{
				PipelineState pso;
				pso.name = "outline_composite";
				pso.shaders[static_cast<uint32_t>(Stage::Compute)] = GetShader(Renderer_Shader::outline_comp);
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
		Shader* shader_v = GetShader(Renderer_Shader::font_vertex);
		Shader* shader_f = GetShader(Renderer_Shader::font_frag);

		if (!shader_v || !shader_f || !out)
			return;

		Ref<Font> font = GetFont();
		if (!font->HasText())
			return;

		font->UpdateVertexAndIndexBuffers(cmdList);

		{
			PipelineState pso;
			pso.name											= "text";
			pso.shaders[static_cast<uint32_t>(Stage::Vertex)]   = shader_v;
			pso.shaders[static_cast<uint32_t>(Stage::Fragment)] = shader_f;
			pso.rasterizerState									= GetRasterizerState(Renderer_RasterizerState::Solid);
			pso.blendState										= GetBlendState(Renderer_BlendState::Alpha);
			pso.depthStencil_State								= GetDepthStencilState(Renderer_DepthStencilState::Off);
			pso.renderTarget_ColorTextures[0]					= out;
			pso.clearColor[0]									= RHI_COLOR_LOAD;
			cmdList->SetPipelineState(pso);
			cmdList->SetBufferVertex(font->GetVertexBuffer());
			cmdList->SetBufferIndex(font->GetIndexBuffer());
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

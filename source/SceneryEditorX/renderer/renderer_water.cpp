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
 * renderer_water.cpp
 * -------------------------------------------------------
 * Created: 22/10/2026
 * -------------------------------------------------------
 *
 * Water of the XP12 emulation (plan Phase 11; XP12: ocean_spectrum, compute_fft, ocean_bake_textures, ocean_shading). The
 * environment's wave amplitude and direction become a sea state (renderer/ocean.h); once a frame the FFT ocean's four
 * cascades are made at the ocean's time (ocean.slang: spectrum when the sea changes, time step, the 2D inverse FFT as
 * rows then columns, bake, mips), and after the sky the water draws (materials with MaterialProperty::Water, kept out of
 * every other pass by UpdateDrawCalls) are tessellated, displaced and lit by water.slang over the frame, their depth
 * written so the forward passes are hidden under them. The frame as composed so far is copied first: what stands in the
 * water is seen through it.
 */
#include "gpu_timer.h"
#include "renderer.h"
#include "renderer_declarations.h"
#include "vulkan/image_resource.h"
#include "vulkan/pipeline/pipeline_state.h"
#include "vulkan/shader/shader_manager.h"
#include <SceneryEditorX/environment/environment.h>
#include <SceneryEditorX/scene/components/renderable.h>
#include <SceneryEditorX/scene/material.h>
#include <algorithm>
#include <cmath>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		struct OceanState
		{
			OceanSpectrum requested;			// The sea state before NormalizeOceanSpectrum (what changes are detected on)
			OceanSpectrum spectrum;				// Normalized: what the constants carry
			std::array<OceanCascade, OCEAN_CASCADES> cascades;
			bool spectrumDirty = true;			// ocean_h0 doesn't hold `spectrum` yet
			bool foamReset = true;				// The baked cascades hold no foam history yet
			double time = 0.0;					// The ocean's time (s)
			float step = 0.0f;					// This frame's time step (s)
			float fixedStep = 0.0f;				// SetWaterFixedTimeStep (0: real time)
			uint64_t lastFrame = UINT64_MAX;	// The frame the time last advanced on
			uint64_t bakedFrame = UINT64_MAX;	// The frame the cascades were last made on (BindOcean: this frame's sea)
		};
		OceanState s_Ocean;
		Renderer::WaterReadout s_WaterReadout;

		bool IsUsable(ImageResource *image)
		{
			return image && image->Get() && *image->Get() != VK_NULL_HANDLE && image->GetResourceState() == ResourceState::PreparedForGpu;
		}

		Shader *GetBuiltShader(const Renderer_Shader id)
		{
			if (!ShaderManager::IsShaderAvailable(id))
				return nullptr;
			Shader *shader = ShaderManager::GetShader(id);
			return shader && shader->IsCompiled() ? shader : nullptr;
		}

		bool SetComputePipeline(CommandList *cmdList, const char *name, const Renderer_Shader id)
		{
			Shader *shader = GetBuiltShader(id);
			if (!shader)
				return false;
			PipelineState pso;
			pso.name = name;
			pso.shaders[static_cast<uint32_t>(StageType::Compute)] = shader;
			cmdList->SetPipelineState(pso);
			return !cmdList->IsStickyInvalid();
		}

		bool WaterShadersBuilt()
		{
			return GetBuiltShader(Renderer_Shader::ocean_spectrum) && GetBuiltShader(Renderer_Shader::ocean_evolve) &&
				   GetBuiltShader(Renderer_Shader::ocean_fft) && GetBuiltShader(Renderer_Shader::ocean_bake) &&
				   GetBuiltShader(Renderer_Shader::ocean_mip) && GetBuiltShader(Renderer_Shader::water);
		}

		/* @brief X-Plane's perceptual roughness for a gloss: 1 - 0.96875 gloss. */
		float RoughnessFromWaterGloss(const float gloss)
		{
			return std::clamp(1.0f - 0.96875f * gloss, 0.0f, 1.0f);
		}
	}

	const Renderer::WaterReadout &Renderer::GetWaterReadout()
	{
		return s_WaterReadout;
	}

	void Renderer::SetWaterFixedTimeStep(const float seconds)
	{
		s_Ocean.fixedStep = std::max(seconds, 0.0f);
	}

	void Renderer::ResetWaterTime()
	{
		s_Ocean.time = 0.0;
		s_Ocean.foamReset = true;
	}

	void Renderer::WriteWaterConstants(ConstantBuffer_Environment &env)
	{
		const OceanSettings &settings = GetEnvironmentConstants().water;

		// The ocean's time, once a frame: the frame's real step (a long stall doesn't jump the sea), or the fixed one
		if (m_FrameNumber != s_Ocean.lastFrame)
		{
			s_Ocean.lastFrame = m_FrameNumber;
			s_Ocean.step = s_Ocean.fixedStep > 0.0f ? s_Ocean.fixedStep : std::clamp(m_Cb_Frame_Cpu.delta_time, 0.0f, 0.1f);
			s_Ocean.time += s_Ocean.step;
		}

		// The sea state for the environment's waves; normalizing it sums the cascades' cells, so only when it changes
		const Environment::EnvironmentState state = Environment::Get().GetSnapshot().state;
		const OceanSpectrum requested = ComputeOceanSpectrum(state.waveAmplitudeM, state.waveDirectionDegT, settings);
		const std::array<OceanCascade, OCEAN_CASCADES> cascades = ComputeOceanCascades(settings);
		const bool cascadesChanged = std::ranges::any_of(std::array<uint32_t, OCEAN_CASCADES>{ 0, 1, 2, 3 }, [&](const uint32_t c)
		{
			return cascades[c].sizeM != s_Ocean.cascades[c].sizeM || cascades[c].kMin != s_Ocean.cascades[c].kMin ||
				   cascades[c].kMax != s_Ocean.cascades[c].kMax || cascades[c].scale != s_Ocean.cascades[c].scale;
		});
		if (!(requested == s_Ocean.requested) || cascadesChanged || s_Ocean.spectrum.peakOmega <= 0.0f)
		{
			s_Ocean.requested = requested;
			s_Ocean.cascades = cascades;
			s_Ocean.spectrum = requested;
			NormalizeOceanSpectrum(s_Ocean.spectrum, cascades);
			s_Ocean.spectrumDirty = true;
		}
		const OceanSpectrum &sea = s_Ocean.spectrum;

		env.ocean = xMath::Vec4(static_cast<float>(s_Ocean.time), settings.displaceLambda, sea.heightScale, sea.directionRad);
		for (uint32_t c = 0; c < OCEAN_CASCADES; ++c)
			env.ocean_cascade[c] = xMath::Vec4(cascades[c].sizeM, cascades[c].kMin, cascades[c].kMax, cascades[c].scale);
		env.ocean_sea = xMath::Vec4(sea.windSpeed, sea.swell, sea.spreadBlend, sea.shortWavesFade);
		env.ocean_jonswap = xMath::Vec4(sea.alpha, sea.peakOmega, sea.gamma, sea.depthM);
		env.water_surface = xMath::Vec4(settings.f0, RoughnessFromWaterGloss(settings.gloss), settings.refractiveIndex, settings.enableTurbidity ? 1.0f : 0.0f);
		env.water_volume = xMath::Vec4(settings.turbidity, settings.turbidityCutoff, settings.clarityMinM, settings.shoreDepthM);
		env.water_foam = xMath::Vec4(settings.foamBias, settings.foamRatio, settings.foamScale, settings.foamDecay);
		env.water_floor = xMath::Vec4(settings.floorAlbedo[0], settings.floorAlbedo[1], settings.floorAlbedo[2], settings.seaFloorCoef);
		env.water_tessellation = xMath::Vec4(settings.tessellationPixels, settings.maxTessellation, settings.useFetch ? std::max(settings.pondWaveScale, 1e-3f) : 0.0f, s_Ocean.step);

		if (!IsRenderingExtraView())
		{
			s_WaterReadout.spectrum = sea;
			s_WaterReadout.time = s_Ocean.time;
		}
	}

	bool Renderer::IsWaterVisible()
	{
		return GetFeatures().water && m_DrawCalls_Water_Count > 0 && WaterShadersBuilt();
	}

	void Renderer::Pass_Ocean(CommandList *cmdList)
	{
		// Once a frame, by the first view with water in sight (extra views reuse the main view's); none without water
		if (!IsRenderingExtraView())
		{
			s_WaterReadout.ran = false;
			s_WaterReadout.draws = m_DrawCalls_Water_Count;
		}
		if (!cmdList || !IsWaterVisible() || s_Ocean.bakedFrame == m_FrameNumber)
			return;

		ImageResource *h0 = GetRenderTarget(Renderer_RenderTarget::ocean_h0);
		ImageResource *fft = GetRenderTarget(Renderer_RenderTarget::ocean_fft);
		ImageResource *displacement = GetRenderTarget(Renderer_RenderTarget::ocean_displacement);
		ImageResource *slopes = GetRenderTarget(Renderer_RenderTarget::ocean_slopes);
		if (!IsUsable(h0) || !IsUsable(fft) || !IsUsable(displacement) || !IsUsable(slopes))
			return;

		constexpr uint32_t groups = OCEAN_FFT_SIZE / 8;

		// h0 for a new sea state
		if (s_Ocean.spectrumDirty)
		{
			GpuTimer::Scope scope(cmdList, "ocean spectrum");
			if (!SetComputePipeline(cmdList, "ocean_spectrum", Renderer_Shader::ocean_spectrum))
				return;
			cmdList->SetTexture(Renderer_BindingsUav::ocean_a, h0);
			cmdList->Dispatch(groups, groups, OCEAN_CASCADES);
			cmdList->InsertComputeMemoryBarrier();
			s_Ocean.spectrumDirty = false;
		}

		// The cells at the ocean's time, then the inverse transform: rows, then columns (in place, in General layout:
		// memory barriers between the dispatches)
		{
			GpuTimer::Scope scope(cmdList, "ocean fft");
			if (!SetComputePipeline(cmdList, "ocean_evolve", Renderer_Shader::ocean_evolve))
				return;
			cmdList->SetTexture(Renderer_BindingsUav::ocean_a, h0);
			cmdList->SetTexture(Renderer_BindingsUav::ocean_b, fft);
			cmdList->Dispatch(groups, groups, OCEAN_CASCADES);
			cmdList->InsertComputeMemoryBarrier();

			if (!SetComputePipeline(cmdList, "ocean_fft", Renderer_Shader::ocean_fft))
				return;
			cmdList->SetTexture(Renderer_BindingsUav::ocean_b, fft);
			for (const float columns : { 0.0f, 1.0f })
			{
				m_Pcb_Pass_Cpu.SetF4Value(columns, 0.0f, 0.0f, 0.0f);
				cmdList->PushConstants(m_Pcb_Pass_Cpu);
				cmdList->Dispatch(1, OCEAN_FFT_SIZE, OCEAN_CASCADES);
				cmdList->InsertComputeMemoryBarrier();
			}
		}

		// Displacement, foam (kept from the last frame, unless there's none yet) and slopes at the top mip, then the mips,
		// each from the one above (binding the level above as a texture is its barrier)
		{
			GpuTimer::Scope scope(cmdList, "ocean bake");
			if (!SetComputePipeline(cmdList, "ocean_bake", Renderer_Shader::ocean_bake))
				return;
			cmdList->SetTexture(Renderer_BindingsUav::ocean_b, fft);
			cmdList->SetTexture(Renderer_BindingsUav::ocean_c, displacement, 0, 1);
			cmdList->SetTexture(Renderer_BindingsUav::ocean_d, slopes, 0, 1);
			m_Pcb_Pass_Cpu.SetF4Value(s_Ocean.foamReset ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f);
			cmdList->PushConstants(m_Pcb_Pass_Cpu);
			cmdList->Dispatch(groups, groups, OCEAN_CASCADES);
			s_Ocean.foamReset = false;
		}
		{
			GpuTimer::Scope scope(cmdList, "ocean mips");
			if (!SetComputePipeline(cmdList, "ocean_mip", Renderer_Shader::ocean_mip))
				return;
			for (uint32_t mip = 1; mip < displacement->GetMipCount(); ++mip)
			{
				cmdList->SetTexture(Renderer_BindingsSrv::ocean_displacement, displacement, mip - 1, 1);
				cmdList->SetTexture(Renderer_BindingsSrv::ocean_slopes, slopes, mip - 1, 1);
				cmdList->SetTexture(Renderer_BindingsUav::ocean_c, displacement, mip, 1);
				cmdList->SetTexture(Renderer_BindingsUav::ocean_d, slopes, mip, 1);
				const uint32_t size = std::max(OCEAN_FFT_SIZE >> mip, 1u);
				cmdList->Dispatch((size + 7) / 8, (size + 7) / 8, OCEAN_CASCADES);
			}
		}

		// Readers sample every level through one view: all of them shader-readable
		for (uint32_t mip = 0; mip < displacement->GetMipCount(); ++mip)
		{
			displacement->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, mip, 1);
			slopes->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, mip, 1);
		}
		if (cmdList->IsStickyInvalid())
			return;
		s_Ocean.bakedFrame = m_FrameNumber;
		s_WaterReadout.ran = true;
	}

	void Renderer::BindOcean(CommandList *cmdList)
	{
		ImageResource *black = GetStandardTexture(Renderer_StandardTexture::Black);
		ImageResource *displacement = GetRenderTarget(Renderer_RenderTarget::ocean_displacement);
		ImageResource *slopes = GetRenderTarget(Renderer_RenderTarget::ocean_slopes);
		const bool baked = s_Ocean.bakedFrame == m_FrameNumber && IsUsable(displacement) && IsUsable(slopes);
		if (cmdList->UsesTexture(static_cast<uint32_t>(Renderer_BindingsSrv::ocean_displacement), false))
			cmdList->SetTexture(Renderer_BindingsSrv::ocean_displacement, baked ? displacement : black);
		if (cmdList->UsesTexture(static_cast<uint32_t>(Renderer_BindingsSrv::ocean_slopes), false))
			cmdList->SetTexture(Renderer_BindingsSrv::ocean_slopes, baked ? slopes : black);
	}

	void Renderer::Pass_Water(CommandList *cmdList, ImageResource *frame)
	{
		if (!cmdList || cmdList->IsStickyInvalid() || !IsUsable(frame) || !IsWaterVisible() || s_Ocean.bakedFrame != m_FrameNumber)
			return;
		Shader *shader = GetBuiltShader(Renderer_Shader::water);
		ImageResource *depth = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth);
		ImageResource *opaque = GetRenderTarget(Renderer_RenderTarget::frame_render_opaque);
		if (!shader || !IsUsable(depth))
			return;

		// The frame as composed so far: what stands in the water is seen through it
		const bool hasOpaque = IsUsable(opaque) && opaque->GetWidth() == frame->GetWidth() && opaque->GetHeight() == frame->GetHeight();
		if (hasOpaque)
		{
			if (CommandList::GetImageLayout(opaque, 0) == Layout::ImageLayout::Undefined)
				opaque->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);	// Copy() puts back the layout it found
			cmdList->Copy(frame, opaque, false);
			opaque->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
		}
		depth->SetLayout(Layout::ImageLayout::DepthStencilAttachment, cmdList, 0, 0);

		PipelineState pso;
		pso.name = "water";
		pso.shaders[static_cast<uint32_t>(StageType::Vertex)] = shader;
		pso.shaders[static_cast<uint32_t>(StageType::TessellationControl)] = shader;
		pso.shaders[static_cast<uint32_t>(StageType::TessellationEvaluation)] = shader;
		pso.shaders[static_cast<uint32_t>(StageType::Fragment)] = shader;
		pso.rasterizerState = GetRasterizerState(Renderer_RasterizerState::Solid);
		pso.blendState = GetBlendState(Renderer_BlendState::Off);
		pso.depthStencil_State = GetDepthStencilState(Renderer_DepthStencilState::ReadWrite);
		pso.renderTarget_ColorTextures[0] = frame;
		pso.renderTarget_DepthTexture = depth;
		pso.clearColor[0] = RHI_COLOR_LOAD;
		cmdList->SetPipelineState(pso);
		if (cmdList->IsStickyInvalid())
			return;

		cmdList->SetTexture(Renderer_BindingsSrv::tex3, GetRenderTarget(Renderer_RenderTarget::lut_brdf_specular));
		cmdList->SetTexture(Renderer_BindingsSrv::water_opaque, hasOpaque ? opaque : GetStandardTexture(Renderer_StandardTexture::Black));
		BindAtmosphere(cmdList, ATMOSPHERE_LIGHTING);
		BindShadows(cmdList);
		BindReflections(cmdList);	// The pyramid's level 0 is the opaque depth under the water; the mirror traces over it
		BindOcean(cmdList);
		cmdList->SetCullMode(CullMode::None);

		for (uint32_t i = 0; i < m_DrawCalls_Water_Count; ++i)
		{
			const Renderer_DrawCall &drawCall = m_DrawCalls_Water[i];
			Renderable *renderable = drawCall.renderable;
			MaterialAsset *material = renderable ? renderable->GetMaterialAsset() : nullptr;
			if (!material)
				continue;
			m_Pcb_Pass_Cpu.drawIndex = drawCall.drawData_Index;
			m_Pcb_Pass_Cpu.isTransparent = 0;
			m_Pcb_Pass_Cpu.materialIndex = material->GetIndex();
			m_Pcb_Pass_Cpu.SetF4Value(m_Has_DirectionalLight ? 0.0f : 1.0f, 0.0f, 0.0f, 0.0f);
			cmdList->PushConstants(m_Pcb_Pass_Cpu);
			cmdList->SetVertexBuffer(renderable->GetVertexBuffer(), renderable->GetInstanceBuffer());
			cmdList->SetIndexBuffer(renderable->GetIndexBuffer());
			cmdList->DrawIndexed(renderable->GetIndexCount(drawCall.lodIndex), drawCall.instanceCount, renderable->GetIndexOffset(drawCall.lodIndex),
								 renderable->GetVertexOffset(drawCall.lodIndex), drawCall.instanceIndex);
		}
		depth->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
	}
}

// -------------------------------------------------------

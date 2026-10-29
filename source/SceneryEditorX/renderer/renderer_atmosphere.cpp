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
 * renderer_atmosphere.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 *
 * The atmosphere of the XP12 emulation (plan Phase 6; XP12: scatter_compute_*): Hillaire's LUTs (atmosphere_luts,
 * sky_view_lut, sky_ambient_lut, aerial_perspective), the environment probes (cube_filter) and the sky pass (sky). The
 * medium comes from the environment buffer (UpdateEnvironmentConstantBuffer); the LUTs that depend on it alone are
 * redone only when it changes.
 */
#include "env_probes.h"
#include "renderer.h"
#include "renderer_declarations.h"
#include "vulkan/image_resource.h"
#include "vulkan/pipeline/pipeline_state.h"
#include "vulkan/shader/shader_manager.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		uint64_t s_MediumHash = 0;			// Of the medium the transmittance, multi-scattering and sky-ambient LUTs hold
		EnvProbeState s_ProbeState;

		bool IsUsable(ImageResource *image)
		{
			return image && image->Get() && *image->Get() != VK_NULL_HANDLE;
		}

		Shader *GetComputeShader(const Renderer_Shader id)
		{
			if (!ShaderManager::IsShaderAvailable(id))
				return nullptr;
			Shader *shader = ShaderManager::GetShader(id);
			return shader && shader->IsCompiled() ? shader : nullptr;
		}

		bool SetComputePipeline(CommandList *cmdList, const char *name, const Renderer_Shader id)
		{
			Shader *shader = GetComputeShader(id);
			if (!shader)
				return false;
			PipelineState pso;
			pso.name = name;
			pso.shaders[static_cast<uint32_t>(StageType::Compute)] = shader;
			cmdList->SetPipelineState(pso);
			return !cmdList->IsStickyInvalid();
		}

		uint32_t Groups(const uint32_t size, const uint32_t groupSize = 8)
		{
			return (std::max(size, 1u) + groupSize - 1) / groupSize;
		}

		uint64_t HashVec4(uint64_t hash, const xMath::Vec4 &v)
		{
			hash = HashCombine(hash, v.x);
			hash = HashCombine(hash, v.y);
			hash = HashCombine(hash, v.z);
			return HashCombine(hash, v.w);
		}
	}

	void Renderer::BindAtmosphere(CommandList *cmdList, const uint32_t textures)
	{
		// Every shader that imports atmosphere_common declares all of these (reflection keeps unused globals): bind only
		// what the pass reads, never what it writes
		static constexpr std::array<std::pair<Renderer_BindingsSrv, Renderer_RenderTarget>, 7> TEXTURES = { {
			{ Renderer_BindingsSrv::env_probes, Renderer_RenderTarget::env_probes },
			{ Renderer_BindingsSrv::aerial_inscatter, Renderer_RenderTarget::lut_aerial_inscatter },
			{ Renderer_BindingsSrv::aerial_transmittance, Renderer_RenderTarget::lut_aerial_transmittance },
			{ Renderer_BindingsSrv::sky_view, Renderer_RenderTarget::lut_sky_view },
			{ Renderer_BindingsSrv::transmittance, Renderer_RenderTarget::lut_atmosphere_transmittance },
			{ Renderer_BindingsSrv::multiscatter, Renderer_RenderTarget::lut_atmosphere_multiscatter },
			{ Renderer_BindingsSrv::sky_ambient, Renderer_RenderTarget::lut_sky_ambient },
		} };
		for (uint32_t i = 0; i < TEXTURES.size(); ++i)
		{
			const auto &[slot, target] = TEXTURES[i];
			if ((textures & (1u << i)) != 0 && cmdList->UsesTexture(slot))
				cmdList->SetTexture(slot, GetRenderTarget(target));
		}
	}

	void Renderer::ReadyAtmosphereTextures(CommandList *cmdList)
	{
		for (const Renderer_RenderTarget target : { Renderer_RenderTarget::env_probes, Renderer_RenderTarget::lut_aerial_inscatter, Renderer_RenderTarget::lut_aerial_transmittance,
				 Renderer_RenderTarget::lut_sky_view, Renderer_RenderTarget::lut_atmosphere_transmittance, Renderer_RenderTarget::lut_atmosphere_multiscatter,
				 Renderer_RenderTarget::lut_sky_ambient })
		{
			ImageResource *image = GetRenderTarget(target);
			if (IsUsable(image))
				image->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
		}
	}

	void Renderer::Pass_Atmosphere(CommandList *cmdList)
	{
		ImageResource *transmittance = GetRenderTarget(Renderer_RenderTarget::lut_atmosphere_transmittance);
		ImageResource *multiscatter = GetRenderTarget(Renderer_RenderTarget::lut_atmosphere_multiscatter);
		ImageResource *skyAmbient = GetRenderTarget(Renderer_RenderTarget::lut_sky_ambient);
		ImageResource *skyView = GetRenderTarget(Renderer_RenderTarget::lut_sky_view);
		ImageResource *aerialInscatter = GetRenderTarget(Renderer_RenderTarget::lut_aerial_inscatter);
		ImageResource *aerialTransmittance = GetRenderTarget(Renderer_RenderTarget::lut_aerial_transmittance);
		if (!IsUsable(transmittance) || !IsUsable(multiscatter) || !IsUsable(skyAmbient) || !IsUsable(skyView) || !IsUsable(aerialInscatter) ||
			!IsUsable(aerialTransmittance))
			return;

		// The medium: redo the LUTs that depend on it alone when it changed
		const ConstantBuffer_Environment &env = m_Cb_Environment_Cpu;
		uint64_t medium = 0;
		for (const xMath::Vec4 &v : { env.rayleigh, env.mie, env.ozone, env.planet, env.atmosphere_haze })
			medium = HashVec4(medium, v);
		medium = HashCombine(medium, env.camera.y);
		if (medium != s_MediumHash)
		{
			const auto lut = [&](const char *name, const Renderer_Shader shader, ImageResource *target, const uint32_t reads)
			{
				if (!SetComputePipeline(cmdList, name, shader))
					return false;
				BindAtmosphere(cmdList, reads);
				cmdList->SetTexture(Renderer_BindingsUav::tex, target);
				cmdList->Dispatch(Groups(target->GetWidth()), Groups(target->GetHeight()));
				return true;
			};
			if (lut("atmosphere_transmittance_lut", Renderer_Shader::atmosphere_transmittance_lut, transmittance, 0) &&
				lut("atmosphere_multiscatter_lut", Renderer_Shader::atmosphere_multiscatter_lut, multiscatter, ATMOSPHERE_TRANSMITTANCE) &&
				lut("sky_ambient_lut", Renderer_Shader::sky_ambient_lut, skyAmbient, ATMOSPHERE_TRANSMITTANCE | ATMOSPHERE_MULTISCATTER))
				s_MediumHash = medium;
		}
		if (cmdList->IsStickyInvalid())
			return;

		// What the camera sees, every frame
		if (SetComputePipeline(cmdList, "sky_view_lut", Renderer_Shader::sky_view_lut))
		{
			BindAtmosphere(cmdList, ATMOSPHERE_TRANSMITTANCE | ATMOSPHERE_MULTISCATTER);
			cmdList->SetTexture(Renderer_BindingsUav::tex, skyView);
			cmdList->Dispatch(Groups(skyView->GetWidth()), Groups(skyView->GetHeight()));
		}
		if (GetFeatures().aerialPerspective && SetComputePipeline(cmdList, "aerial_perspective", Renderer_Shader::aerial_perspective))
		{
			BindAtmosphere(cmdList, ATMOSPHERE_TRANSMITTANCE | ATMOSPHERE_MULTISCATTER);
			cmdList->SetTexture(Renderer_BindingsUav::tex3d, aerialInscatter);
			cmdList->SetTexture(Renderer_BindingsUav::tex3d_2, aerialTransmittance);
			cmdList->Dispatch(Groups(aerialInscatter->GetWidth()), Groups(aerialInscatter->GetHeight()));
		}
	}

	void Renderer::Pass_EnvProbes(CommandList *cmdList)
	{
		ImageResource *probes = GetRenderTarget(Renderer_RenderTarget::env_probes);
		if (!IsUsable(probes) || !GetComputeShader(Renderer_Shader::cube_filter_capture) || !GetComputeShader(Renderer_Shader::cube_filter_ggx) ||
			!GetComputeShader(Renderer_Shader::cube_filter_irradiance))
			return;

		// Probe 0 at the camera: captured again when what it sees changed
		const ConstantBuffer_Environment &env = m_Cb_Environment_Cpu;
		EnvProbeInputs inputs;
		inputs.camera = { m_Cb_Frame_Cpu.camera_position.x, m_Cb_Frame_Cpu.camera_position.y, m_Cb_Frame_Cpu.camera_position.z };
		inputs.sun = { env.sun_direction.x, env.sun_direction.y, env.sun_direction.z };
		inputs.moon = { env.moon_direction.x, env.moon_direction.y, env.moon_direction.z };
		inputs.mediumHash = HashCombine(HashVec4(HashVec4(s_MediumHash, env.light_toa), env.atmosphere_view), env.moon_direction.w);
		if (!EnvProbeNeedsCapture(s_ProbeState, inputs))
			return;

		const uint32_t mips = std::min(probes->GetImageSpec().mipCount, ENV_PROBE_MIPS);
		for (uint32_t mip = 0; mip < mips; ++mip)
		{
			const bool irradiance = mip + 1 == ENV_PROBE_MIPS;
			const Renderer_Shader shader = mip == 0 ? Renderer_Shader::cube_filter_capture : irradiance ? Renderer_Shader::cube_filter_irradiance : Renderer_Shader::cube_filter_ggx;
			if (!SetComputePipeline(cmdList, "cube_filter", shader))
				return;
			BindAtmosphere(cmdList, ATMOSPHERE_SKY_VIEW | ATMOSPHERE_TRANSMITTANCE | ATMOSPHERE_SKY_AMBIENT);
			cmdList->SetTexture(Renderer_BindingsUav::tex_array, probes, mip, 1);
			m_Pcb_Pass_Cpu.SetF4Value(static_cast<float>(mip) / 5.0f, 0.0f, 0.0f, 0.0f);	// roughness; probe 0's first layer
			cmdList->PushConstants(m_Pcb_Pass_Cpu);
			const uint32_t size = std::max(probes->GetWidth() >> mip, 1u);
			cmdList->Dispatch(Groups(size), Groups(size), 6);
		}
		s_ProbeState = CaptureEnvProbe(inputs);
	}

	void Renderer::Pass_Sky(CommandList *cmdList, ImageResource *frame)
	{
		if (!IsUsable(frame) || !SetComputePipeline(cmdList, "sky", Renderer_Shader::sky))
			return;
		cmdList->SetTexture(Renderer_BindingsUav::tex, frame);
		cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_depth, GetRenderTarget(Renderer_RenderTarget::gbuffer_depth));
		BindAtmosphere(cmdList, ATMOSPHERE_SKY_VIEW | ATMOSPHERE_TRANSMITTANCE | ATMOSPHERE_SKY_AMBIENT);
		cmdList->Dispatch(frame);
	}
}

// -------------------------------------------------------

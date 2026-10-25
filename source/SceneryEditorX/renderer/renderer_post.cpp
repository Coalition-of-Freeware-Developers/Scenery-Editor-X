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
 * renderer_post.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 *
 * The XP12 emulation's HDR end of the frame (plan Phase 5, REQ-007): fix-HDR, histogram metering and adaptation, then
 * the output chain: exposure fusion, bloom, the tone map, FXAA and FSR 1. Lighting is written pre-exposed (the EV the
 * GPU last reported, read back here); the tone map applies the residual to this frame's EV.
 */
#include "gpu_timer.h"
#include "renderer.h"
#include "renderer_declarations.h"
#include "vulkan/buffer.h"
#include "vulkan/image_resource.h"
#include "vulkan/pipeline/pipeline_state.h"
#include "vulkan/shader/shader_manager.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <mutex>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		std::mutex s_ExposureMutex;
		Renderer::ExposureReadout s_ExposureReadout;
		std::atomic<bool> s_ExposureReset{ true };	// The first frame jumps to its metered exposure

		/* @brief A compute shader that is present and compiled, or nullptr. */
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

		/* @brief Shader writes before reads and writes of the same buffers in later dispatches (and the host). */
		void ComputeBarrier(CommandList *cmdList, const bool host = false)
		{
			cmdList->EndRenderPass();
			VkMemoryBarrier2 barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
			barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
			barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | (host ? VK_PIPELINE_STAGE_2_HOST_BIT : VK_PIPELINE_STAGE_2_NONE);
			barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT | (host ? VK_ACCESS_2_HOST_READ_BIT : VK_ACCESS_2_NONE);

			VkDependencyInfo dependency{};
			dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dependency.memoryBarrierCount = 1;
			dependency.pMemoryBarriers = &barrier;
			vkCmdPipelineBarrier2(cmdList->GetCommandBuffer(), &dependency);
		}

		uint32_t Groups(const uint32_t size, const uint32_t groupSize)
		{
			return (std::max(size, 1u) + groupSize - 1) / groupSize;
		}

		uint32_t MipSize(const uint32_t size, const uint32_t mip)
		{
			return std::max(size >> mip, 1u);
		}

		bool IsUsable(ImageResource *image)
		{
			return image && image->Get() && *image->Get() != VK_NULL_HANDLE;
		}
	}

	Renderer::ExposureReadout Renderer::GetExposureReadout()
	{
		std::scoped_lock lock(s_ExposureMutex);
		return s_ExposureReadout;
	}

	void Renderer::ResetExposure()
	{
		s_ExposureReset.store(true, std::memory_order_relaxed);
	}

	void Renderer::SetExposurePreExposure(const float preExposure)
	{
		std::scoped_lock lock(s_ExposureMutex);
		s_ExposureReadout.preExposure = preExposure;
	}

	void Renderer::UpdateExposureReadback()
	{
		// The state buffer is host visible; the last frame the GPU finished wrote it (a float or two may be a frame
		// newer than the rest, which only matters for the readout)
		Buffer *buffer = GetBuffer(Renderer_Buffer::ExposureState);
		if (!buffer)
			return;
		const void *mapped = buffer->GetMappedData() ? buffer->GetMappedData() : buffer->Map();	// the initial upload unmapped it
		if (!mapped)
			return;
		std::array<float, EXPOSURE_STATE_FLOATS> state{};
		std::memcpy(state.data(), mapped, sizeof(state));

		std::scoped_lock lock(s_ExposureMutex);
		ExposureReadout &readout = s_ExposureReadout;
		readout.valid = state[EXPOSURE_STATE_VALID] > 0.5f && std::isfinite(state[EXPOSURE_STATE_EV]);
		if (!readout.valid)
			return;
		readout.ev100 = state[EXPOSURE_STATE_EV];
		readout.meteredEv100 = state[EXPOSURE_STATE_METERED];
		readout.targetEv100 = state[EXPOSURE_STATE_TARGET];
		readout.meanLog2Nits = state[EXPOSURE_STATE_MEAN_LOG2];
		readout.rawEv100 = state[EXPOSURE_STATE_RAW];
	}

	void Renderer::Pass_FixHdr(CommandList *cmdList, ImageResource *frame)
	{
		if (!IsUsable(frame) || !SetComputePipeline(cmdList, "fix_hdr", Renderer_Shader::fix_hdr))
			return;
		cmdList->SetTexture(Renderer_BindingsUav::tex, frame);
		cmdList->Dispatch(frame);
	}

	void Renderer::Pass_Exposure(CommandList *cmdList, ImageResource *frame)
	{
		Buffer *histogram = GetBuffer(Renderer_Buffer::ExposureHistogram);
		Buffer *state = GetBuffer(Renderer_Buffer::ExposureState);
		if (!IsUsable(frame) || !histogram || !state || !GetComputeShader(Renderer_Shader::luminance_histogram) || !GetComputeShader(Renderer_Shader::exposure_adapt))
			return;

		// The histogram of the frame at half resolution (the last frame's adapt pass cleared it)
		ComputeBarrier(cmdList);
		if (!SetComputePipeline(cmdList, "luminance_histogram", Renderer_Shader::luminance_histogram))
			return;
		cmdList->SetTexture(Renderer_BindingsSrv::tex, frame);
		cmdList->SetBuffer(Renderer_BindingsUav::exposure_histogram, histogram);
		cmdList->Dispatch(Groups(MipSize(frame->GetWidth(), 1), 16), Groups(MipSize(frame->GetHeight(), 1), 16));
		ComputeBarrier(cmdList);

		// Meter, adapt, clear the histogram; manual mode (or auto exposure switched off) holds the manual EV100
		if (!SetComputePipeline(cmdList, "exposure_adapt", Renderer_Shader::exposure_adapt))
			return;
		const ExposureSettings &settings = GetEnvironmentConstants().exposure;
		const bool manual = !settings.automatic || !GetFeatures().autoExposure;
		const bool reset = s_ExposureReset.exchange(false, std::memory_order_relaxed);
		cmdList->SetBuffer(Renderer_BindingsUav::exposure_histogram, histogram);
		cmdList->SetBuffer(Renderer_BindingsUav::exposure_state, state);
		m_Pcb_Pass_Cpu.SetF4Value(reset ? 1.0f : 0.0f, settings.manualEv100, manual ? 1.0f : 0.0f, 0.0f);
		cmdList->PushConstants(m_Pcb_Pass_Cpu);
		cmdList->Dispatch(1, 1, 1);
		ComputeBarrier(cmdList, true);
	}

	uint32_t Renderer::Pass_Bloom(CommandList *cmdList, ImageResource *frame)
	{
		ImageResource *bloom = GetRenderTarget(Renderer_RenderTarget::bloom);
		if (!IsUsable(frame) || !IsUsable(bloom) || !GetComputeShader(Renderer_Shader::bloom_luminance) ||
			!GetComputeShader(Renderer_Shader::bloom_downsample) || !GetComputeShader(Renderer_Shader::bloom_upsample_blend_mip))
			return 0;

		const uint32_t mips = bloom->GetImageSpec().mipCount;
		const uint32_t width = bloom->GetWidth();
		const uint32_t height = bloom->GetHeight();
		const auto dispatch = [&](const uint32_t mip)
		{
			cmdList->Dispatch(Groups(MipSize(width, mip), 8), Groups(MipSize(height, mip), 8));
		};

		// The frame into mip 0 (13-tap, firefly-weighted; no threshold: plan TASK-042), then down the chain
		if (!SetComputePipeline(cmdList, "bloom_luminance", Renderer_Shader::bloom_luminance))
			return 0;
		cmdList->SetTexture(Renderer_BindingsSrv::tex, frame);
		cmdList->SetTexture(Renderer_BindingsUav::tex, bloom, 0, 1);
		dispatch(0);

		for (uint32_t mip = 0; mip + 1 < mips; ++mip)
		{
			if (!SetComputePipeline(cmdList, "bloom_downsample", Renderer_Shader::bloom_downsample))
				return 0;
			cmdList->SetTexture(Renderer_BindingsSrv::tex, bloom, mip, 1);
			cmdList->SetTexture(Renderer_BindingsUav::tex, bloom, mip + 1, 1);
			dispatch(mip + 1);
		}

		// Back up, each level adding the blurrier one below: mip 0 ends up the sum of every level (the tone map
		// divides by the count)
		for (uint32_t mip = mips - 1; mip > 0; --mip)
		{
			if (!SetComputePipeline(cmdList, "bloom_upsample_blend_mip", Renderer_Shader::bloom_upsample_blend_mip))
				return 0;
			cmdList->SetTexture(Renderer_BindingsSrv::tex, bloom, mip, 1);
			cmdList->SetTexture(Renderer_BindingsUav::tex, bloom, mip - 1, 1);
			dispatch(mip - 1);
		}
		bloom->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 1);
		return mips;
	}

	void Renderer::Pass_ExposureFusion(CommandList *cmdList, ImageResource *frame)
	{
		ImageResource *exposures = GetRenderTarget(Renderer_RenderTarget::exposure_fusion_exposures);
		ImageResource *weights = GetRenderTarget(Renderer_RenderTarget::exposure_fusion_weights);
		ImageResource *result = GetRenderTarget(Renderer_RenderTarget::exposure_fusion_result);
		ImageResource *guide = GetRenderTarget(Renderer_RenderTarget::exposure_fusion_guide);
		Buffer *state = GetBuffer(Renderer_Buffer::ExposureState);
		if (!IsUsable(frame) || !IsUsable(exposures) || !IsUsable(weights) || !IsUsable(result) || !IsUsable(guide) || !state)
			return;

		const uint32_t width = exposures->GetWidth();
		const uint32_t height = exposures->GetHeight();
		const uint32_t levels = std::min(static_cast<uint32_t>(std::max(GetEnvironmentConstants().fusionMaxMip, 1)), exposures->GetImageSpec().mipCount - 1);
		const auto dispatch = [&](const uint32_t mip)
		{
			cmdList->Dispatch(Groups(MipSize(width, mip), 8), Groups(MipSize(height, mip), 8));
		};

		// Three exposures and their weights at half resolution
		if (!SetComputePipeline(cmdList, "exposure_fusion_exposures", Renderer_Shader::exposure_fusion_exposures))
			return;
		cmdList->SetTexture(Renderer_BindingsSrv::tex, frame);
		cmdList->SetTexture(Renderer_BindingsUav::tex, exposures, 0, 1);
		cmdList->SetTexture(Renderer_BindingsUav::tex2, weights, 0, 1);
		cmdList->SetBuffer(Renderer_BindingsUav::exposure_state, state);
		dispatch(0);

		// Their Gaussian pyramids
		for (uint32_t mip = 0; mip < levels; ++mip)
		{
			if (!SetComputePipeline(cmdList, "exposure_fusion_downsample", Renderer_Shader::exposure_fusion_downsample))
				return;
			cmdList->SetTexture(Renderer_BindingsSrv::tex, exposures, mip, 1);
			cmdList->SetTexture(Renderer_BindingsSrv::tex2, weights, mip, 1);
			cmdList->SetTexture(Renderer_BindingsUav::tex, exposures, mip + 1, 1);
			cmdList->SetTexture(Renderer_BindingsUav::tex2, weights, mip + 1, 1);
			dispatch(mip + 1);
		}

		// The blended Laplacian pyramid, collapsed from the coarsest level up
		for (int mip = static_cast<int>(levels); mip >= 0; --mip)
		{
			if (!SetComputePipeline(cmdList, "exposure_fusion_collapse", Renderer_Shader::exposure_fusion_collapse))
				return;
			const uint32_t level = static_cast<uint32_t>(mip);
			const bool coarsest = level == levels;
			cmdList->SetTexture(Renderer_BindingsSrv::tex, exposures, level, 1);
			cmdList->SetTexture(Renderer_BindingsSrv::tex2, weights, level, 1);
			cmdList->SetTexture(Renderer_BindingsSrv::tex3, exposures, coarsest ? level : level + 1, 1);
			cmdList->SetTexture(Renderer_BindingsSrv::tex4, coarsest ? weights : result, coarsest ? level : level + 1, 1);
			cmdList->SetTexture(Renderer_BindingsUav::tex, result, level, 1);
			m_Pcb_Pass_Cpu.SetF4Value(coarsest ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f);
			cmdList->PushConstants(m_Pcb_Pass_Cpu);
			dispatch(level);
		}

		// The gain the fused image asks for
		if (!SetComputePipeline(cmdList, "exposure_fusion_guide", Renderer_Shader::exposure_fusion_guide))
			return;
		cmdList->SetTexture(Renderer_BindingsSrv::tex, result, 0, 1);
		cmdList->SetTexture(Renderer_BindingsSrv::tex2, exposures, 0, 1);
		cmdList->SetTexture(Renderer_BindingsUav::tex, guide);
		dispatch(0);
		guide->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
	}

	void Renderer::Pass_OutputChain(CommandList *cmdList, ImageResource *frame, ImageResource *out)
	{
		const RendererFeatures &features = GetFeatures();

		// Local tone mapping and bloom read the frame; the tone map brings them together
		const bool fusion = features.exposureFusion && GetEnvironmentConstants().fusionStrength > 0.0f &&
							GetComputeShader(Renderer_Shader::exposure_fusion_exposures) && GetComputeShader(Renderer_Shader::exposure_fusion_downsample) &&
							GetComputeShader(Renderer_Shader::exposure_fusion_collapse) && GetComputeShader(Renderer_Shader::exposure_fusion_guide);
		if (fusion)
		{
			GpuTimer::Scope scope(cmdList, "exposure fusion");
			Pass_ExposureFusion(cmdList, frame);
		}
		if (cmdList->IsStickyInvalid())
			return;

		uint32_t bloomMips = 0;
		if (features.bloom && GetEnvironmentConstants().bloomIntensity > 0.0f)
		{
			GpuTimer::Scope scope(cmdList, "bloom");
			bloomMips = Pass_Bloom(cmdList, frame);
		}
		if (cmdList->IsStickyInvalid())
			return;

		// Tone map, then FXAA and FSR 1 through the two output-sized scratch targets
		ImageResource *scratchA = GetRenderTarget(Renderer_RenderTarget::frame_output_2);
		ImageResource *scratchB = GetRenderTarget(Renderer_RenderTarget::debug_output);
		const bool scratch = IsUsable(scratchA) && IsUsable(scratchB);
		const RendererQualityPreset &quality = GetQualityPreset();
		const bool fxaa = features.fxaa && quality.fxaa && scratch && GetComputeShader(Renderer_Shader::fxaa);
		// FSR 1 follows its switch; the QUALITY preset's FSR level picks the mode (texture LOD bias; the render scale it
		// asks for needs render targets below the output size, which the editor doesn't make yet, so EASU runs 1:1)
		const bool fsr = features.fsr1 && scratch && GetComputeShader(Renderer_Shader::fsr1_easu) && GetComputeShader(Renderer_Shader::fsr1_rcas);

		ImageResource *current = fxaa || fsr ? scratchA : out;
		{
			GpuTimer::Scope scope(cmdList, "tonemap");
			Pass_Tonemap(cmdList, frame, current, bloomMips, fusion);
		}
		if (cmdList->IsStickyInvalid())
			return;

		const auto run = [&](const char *name, const Renderer_Shader shader, ImageResource *in, ImageResource *target, const float parameter)
		{
			if (!SetComputePipeline(cmdList, name, shader))
				return;
			cmdList->SetTexture(Renderer_BindingsSrv::tex, in);
			cmdList->SetTexture(Renderer_BindingsUav::tex, target);
			m_Pcb_Pass_Cpu.SetF4Value(parameter, 0.0f, 0.0f, 0.0f);
			cmdList->PushConstants(m_Pcb_Pass_Cpu);
			cmdList->Dispatch(target);
		};

		if (fxaa)
		{
			GpuTimer::Scope scope(cmdList, "fxaa");
			ImageResource *target = fsr ? scratchB : out;
			run("fxaa", Renderer_Shader::fxaa, current, target, 0.0f);
			current = target;
		}
		if (fsr)
		{
			GpuTimer::Scope scope(cmdList, "fsr1");
			ImageResource *upscaled = current == scratchA ? scratchB : scratchA;
			run("fsr1_easu", Renderer_Shader::fsr1_easu, current, upscaled, 0.0f);
			run("fsr1_rcas", Renderer_Shader::fsr1_rcas, upscaled, out, std::clamp(GetEnvironmentConstants().rcasSharpness, 0.0f, 1.0f));
		}
	}
}

// -------------------------------------------------------

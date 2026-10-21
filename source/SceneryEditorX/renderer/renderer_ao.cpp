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
 * renderer_ao.cpp
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 *
 * Ambient occlusion of the XP12 emulation (plan Phase 8; XP12: cacao). After the g-buffer, Pass_AmbientOcclusion runs
 * AMD FidelityFX CACAO (cacao.slang; the constants and the dispatches come from renderer/cacao.h) at the quality and
 * resolution the QUALITY AO level gives, X-Plane's renopt_SSAO_04, into the ssao target; the deferred lighting reads it
 * for the ambient light and the reflections only (environment_lighting.slang). The older GTAO pass (ssao.slang) can
 * stand in for CACAO (AoTechnique::Gtao).
 */
#include "cacao.h"
#include "renderer.h"
#include "renderer_declarations.h"
#include "vulkan/image_resource.h"
#include "vulkan/queue_manager.h"
#include "vulkan/pipeline/pipeline_state.h"
#include "vulkan/shader/shader_manager.h"
#include <algorithm>
#include <cmath>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		Renderer::AoTechnique s_AoTechnique = Renderer::AoTechnique::Cacao;
		Renderer::AoReadout s_AoReadout;
		bool s_AoTargetsHalfResolution = true;	// What CACAO's buffers were made for
		bool s_AoValid = false;					// The ssao target holds this frame's occlusion

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

		Renderer_Shader GetGenerateShader(const CacaoQuality quality)
		{
			switch (quality)
			{
				case CacaoQuality::Medium:	return Renderer_Shader::cacao_generate_q1;
				case CacaoQuality::High:	return Renderer_Shader::cacao_generate_q2;
				default:					return Renderer_Shader::cacao_generate_q3;
			}
		}

		bool AllCacaoShadersBuilt(const CacaoQuality quality, const bool halfResolution)
		{
			std::vector<Renderer_Shader> shaders = { Renderer_Shader::cacao_prepare_depths, Renderer_Shader::cacao_prepare_normals, GetGenerateShader(quality),
				Renderer_Shader::cacao_blur, halfResolution ? (quality == CacaoQuality::Medium ? Renderer_Shader::cacao_upscale : Renderer_Shader::cacao_upscale_smart)
															: Renderer_Shader::cacao_apply };
			if (quality == CacaoQuality::Highest)
				shaders.insert(shaders.end(), { Renderer_Shader::cacao_generate_q3_base, Renderer_Shader::cacao_importance_map,
					Renderer_Shader::cacao_importance_map_a, Renderer_Shader::cacao_importance_map_b });
			return std::ranges::all_of(shaders, [](const Renderer_Shader id) { return GetComputeShader(id) != nullptr; });
		}

		const char *GetKernelName(const CacaoKernel kernel)
		{
			switch (kernel)
			{
				case CacaoKernel::PrepareDepths:	return "cacao_prepare_depths";
				case CacaoKernel::PrepareNormals:	return "cacao_prepare_normals";
				case CacaoKernel::GenerateBase:		return "cacao_generate_base";
				case CacaoKernel::ImportanceMap:	return "cacao_importance_map";
				case CacaoKernel::ImportanceMapA:	return "cacao_importance_map_a";
				case CacaoKernel::ImportanceMapB:	return "cacao_importance_map_b";
				case CacaoKernel::Generate:			return "cacao_generate";
				case CacaoKernel::Blur:				return "cacao_blur";
				case CacaoKernel::Apply:			return "cacao_apply";
				case CacaoKernel::Upscale:			return "cacao_upscale";
			}
			return "cacao";
		}
	}

	Renderer::AoTechnique Renderer::GetAoTechnique()
	{
		return s_AoTechnique;
	}

	void Renderer::SetAoTechnique(const AoTechnique technique)
	{
		s_AoTechnique = technique;
	}

	const Renderer::AoReadout &Renderer::GetAoReadout()
	{
		return s_AoReadout;
	}

	void Renderer::CreateAmbientOcclusionTargets(const bool force)
	{
		const bool halfResolution = GetAoQuality(GetQualityPreset().ao).halfResolution;
		const xMath::Vec2 resolution = GetRendererResolution();
		const uint32_t width = std::max(static_cast<uint32_t>(resolution.x), 1u);
		const uint32_t height = std::max(static_cast<uint32_t>(resolution.y), 1u);

		auto &targets = GetRenderTargets();
		Ref<ImageResource> &ssao = targets[static_cast<size_t>(Renderer_RenderTarget::ssao)];
		if (!force && ssao && halfResolution == s_AoTargetsHalfResolution && ssao->GetWidth() == width && ssao->GetHeight() == height)
			return;
		if (!force && ssao)
			QueueManager::WaitIdleAll();	// frames in flight still read the old ones
		s_AoTargetsHalfResolution = halfResolution;

		const auto make = [&targets](const Renderer_RenderTarget target, const ImgResourceSpec &spec)
		{
			targets[static_cast<size_t>(target)] = CreateRef<ImageResource>(spec);
			SEDX_CORE_ASSERT(targets[static_cast<size_t>(target)] != nullptr, "Failed to create an ambient occlusion render target");
		};

		// The result, read by the lighting: the visibility (1 unoccluded)
		make(Renderer_RenderTarget::ssao, { ImageType::Type2D, width, height, 1, 1, VK_FORMAT_R16_SFLOAT, UnorderedAccessView | ShaderViews | BlitClear, "ssao" });

		// CACAO's buffers (ffxCacaoContextCreate's formats): four 2 x 2 deinterleaved layers
		const CacaoBufferSizes sizes = ComputeCacaoBufferSizes(width, height, halfResolution);
		const uint32_t largest = std::max(sizes.deinterleavedDepthWidth, sizes.deinterleavedDepthHeight);
		const uint32_t depthMips = std::min(CACAO_DEPTH_MIPS, static_cast<uint32_t>(std::floor(std::log2(static_cast<float>(std::max(largest, 1u))))) + 1);
		constexpr uint32_t flags = UnorderedAccessView | ShaderViews;
		make(Renderer_RenderTarget::cacao_depths, { ImageType::Type2DArray, sizes.deinterleavedDepthWidth, sizes.deinterleavedDepthHeight, CACAO_LAYERS, depthMips,
			VK_FORMAT_R16_SFLOAT, flags | PerMipViews, "cacao_depths" });
		make(Renderer_RenderTarget::cacao_normals, { ImageType::Type2DArray, sizes.ssaoWidth, sizes.ssaoHeight, CACAO_LAYERS, 1, VK_FORMAT_R8G8B8A8_SNORM, flags, "cacao_normals" });
		make(Renderer_RenderTarget::cacao_ssao_ping, { ImageType::Type2DArray, sizes.ssaoWidth, sizes.ssaoHeight, CACAO_LAYERS, 1, VK_FORMAT_R8G8_UNORM, flags, "cacao_ssao_ping" });
		make(Renderer_RenderTarget::cacao_ssao_pong, { ImageType::Type2DArray, sizes.ssaoWidth, sizes.ssaoHeight, CACAO_LAYERS, 1, VK_FORMAT_R8G8_UNORM, flags, "cacao_ssao_pong" });
		make(Renderer_RenderTarget::cacao_importance, { ImageType::Type2D, sizes.importanceWidth, sizes.importanceHeight, 1, 1, VK_FORMAT_R8_UNORM, flags, "cacao_importance" });
		make(Renderer_RenderTarget::cacao_importance_pong, { ImageType::Type2D, sizes.importanceWidth, sizes.importanceHeight, 1, 1, VK_FORMAT_R8_UNORM, flags,
			"cacao_importance_pong" });
		make(Renderer_RenderTarget::cacao_load_counter, { ImageType::Type2D, 1, 1, 1, 1, VK_FORMAT_R32_UINT, UnorderedAccessView | BlitClear, "cacao_load_counter" });
	}

	void Renderer::BindAmbientOcclusion(CommandList *cmdList)
	{
		if (!cmdList->UsesTexture(Renderer_BindingsSrv::ssao))
			return;
		ImageResource *ssao = GetRenderTarget(Renderer_RenderTarget::ssao);
		cmdList->SetTexture(Renderer_BindingsSrv::ssao, s_AoValid && IsUsable(ssao) ? ssao : GetStandardTexture(Renderer_StandardTexture::White));
	}

	bool Renderer::Pass_AmbientOcclusion(CommandList *cmdList)
	{
		s_AoValid = false;
		s_AoReadout.ran = false;

		const AoQuality quality = GetAoQuality(GetQualityPreset().ao);
		if (!GetFeatures().ao || !quality.enabled)
			return false;
		CreateAmbientOcclusionTargets(false);
		ImageResource *output = GetRenderTarget(Renderer_RenderTarget::ssao);
		if (!IsUsable(output))
			return false;

		// The fallback: SEDX's GTAO pass (full resolution, its own constants)
		if (s_AoTechnique == AoTechnique::Gtao)
		{
			if (!GetComputeShader(Renderer_Shader::ssao))
				return false;
			Pass_ScreenSpaceAO(cmdList);
			if (cmdList->IsStickyInvalid())
				return false;
			output->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
			s_AoValid = true;
			s_AoReadout = { true, AoTechnique::Gtao, CacaoQuality::High, false, {}, output->GetWidth(), output->GetHeight() };
			return true;
		}

		const CacaoQuality requested = static_cast<CacaoQuality>(std::clamp(quality.cacaoQuality, 0, 4));
		const CacaoQuality run = GetCacaoRunQuality(requested);
		if (!AllCacaoShadersBuilt(run, quality.halfResolution))
			return false;

		// The camera in CACAO's view space (x right, y up, z forward) and X-Plane's dynamic values for its height. SEDX's
		// world origin is the airport's elevation, so the camera's y stands for the height above the ground.
		const ConstantBuffer_Frame &frame = m_Cb_Frame_Cpu;
		const float aspect = frame.resolution_render.y > 0.0f ? frame.resolution_render.x / frame.resolution_render.y : 1.0f;
		CacaoCamera camera;
		camera.tanHalfFovX = std::tan(0.5 * static_cast<double>(frame.camera_fov));
		camera.tanHalfFovY = camera.tanHalfFovX / std::max(static_cast<double>(aspect), 1e-3);
		camera.nearPlane = frame.camera_near;
		camera.farPlane = frame.camera_far;
		const xMath::Vec3 forward = xMath::Normalize(frame.camera_forward);
		const xMath::Vec3 right = xMath::Normalize(frame.camera_right);
		const xMath::Vec3 up = xMath::Cross(right, forward);
		camera.right = { right.x, right.y, right.z };
		camera.up = { up.x, up.y, up.z };
		camera.forward = { forward.x, forward.y, forward.z };

		const CacaoSettings settings = ApplyCacaoDynamicValues(GetEnvironmentConstants().ao, std::max(frame.camera_position.y, 0.0f));
		const CacaoBufferSizes sizes = ComputeCacaoBufferSizes(output->GetWidth(), output->GetHeight(), quality.halfResolution);
		CacaoConstants constants = ComputeCacaoConstants(settings, run, sizes, camera);
		Buffer *constantBuffer = GetBuffer(Renderer_Buffer::ConstantCacao);
		if (!constantBuffer)
			return false;
		constantBuffer->Update(cmdList, &constants);

		ImageResource *depth = GetRenderTarget(Renderer_RenderTarget::gbuffer_depth);
		ImageResource *normals = GetRenderTarget(Renderer_RenderTarget::gbuffer_normal);
		ImageResource *depths = GetRenderTarget(Renderer_RenderTarget::cacao_depths);
		ImageResource *viewNormals = GetRenderTarget(Renderer_RenderTarget::cacao_normals);
		ImageResource *ping = GetRenderTarget(Renderer_RenderTarget::cacao_ssao_ping);
		ImageResource *pong = GetRenderTarget(Renderer_RenderTarget::cacao_ssao_pong);
		ImageResource *importance = GetRenderTarget(Renderer_RenderTarget::cacao_importance);
		ImageResource *importancePong = GetRenderTarget(Renderer_RenderTarget::cacao_importance_pong);
		ImageResource *loadCounter = GetRenderTarget(Renderer_RenderTarget::cacao_load_counter);
		for (ImageResource *image : { depths, viewNormals, ping, pong, importance, importancePong, loadCounter })
		{
			if (!IsUsable(image))
				return false;
		}

		for (const CacaoDispatch &dispatch : PlanCacaoDispatches(run, sizes, settings.blurPassCount))
		{
			const char *name = GetKernelName(dispatch.kernel);
			bool bound = false;
			switch (dispatch.kernel)
			{
				case CacaoKernel::PrepareDepths:
					if ((bound = SetComputePipeline(cmdList, name, Renderer_Shader::cacao_prepare_depths)))
					{
						cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_depth, depth);
						cmdList->SetTexture(Renderer_BindingsUav::cacao_depth_mips, depths, 0, depths->GetMipCount());
					}
					break;
				case CacaoKernel::PrepareNormals:
					if ((bound = SetComputePipeline(cmdList, name, Renderer_Shader::cacao_prepare_normals)))
					{
						cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_normal, normals);
						cmdList->SetTexture(Renderer_BindingsUav::cacao_normals, viewNormals);
					}
					break;
				case CacaoKernel::GenerateBase:
					if ((bound = SetComputePipeline(cmdList, name, Renderer_Shader::cacao_generate_q3_base)))
					{
						cmdList->SetTexture(Renderer_BindingsSrv::cacao_depth, depths);
						cmdList->SetTexture(Renderer_BindingsSrv::cacao_normals, viewNormals);
						cmdList->SetTexture(Renderer_BindingsUav::cacao_ssao, pong);
					}
					break;
				case CacaoKernel::ImportanceMap:
					if ((bound = SetComputePipeline(cmdList, name, Renderer_Shader::cacao_importance_map)))
					{
						cmdList->SetTexture(Renderer_BindingsSrv::cacao_ssao, pong);
						cmdList->SetTexture(Renderer_BindingsUav::cacao_importance, importance);
						cmdList->SetTexture(Renderer_BindingsUav::tex_uint, loadCounter);	// zeroed here
					}
					break;
				case CacaoKernel::ImportanceMapA:
					if ((bound = SetComputePipeline(cmdList, name, Renderer_Shader::cacao_importance_map_a)))
					{
						cmdList->SetTexture(Renderer_BindingsSrv::cacao_importance, importance);
						cmdList->SetTexture(Renderer_BindingsUav::cacao_importance, importancePong);
					}
					break;
				case CacaoKernel::ImportanceMapB:
					if ((bound = SetComputePipeline(cmdList, name, Renderer_Shader::cacao_importance_map_b)))
					{
						cmdList->SetTexture(Renderer_BindingsSrv::cacao_importance, importancePong);
						cmdList->SetTexture(Renderer_BindingsUav::cacao_importance, importance);
						cmdList->SetTexture(Renderer_BindingsUav::tex_uint, loadCounter);
						cmdList->InsertComputeMemoryBarrier();	// the load counter stays in General: the zeroing before the sums
					}
					break;
				case CacaoKernel::Generate:
					if ((bound = SetComputePipeline(cmdList, name, GetGenerateShader(run))))
					{
						cmdList->SetTexture(Renderer_BindingsSrv::cacao_depth, depths);
						cmdList->SetTexture(Renderer_BindingsSrv::cacao_normals, viewNormals);
						if (run == CacaoQuality::Highest)
						{
							cmdList->SetTexture(Renderer_BindingsSrv::cacao_ssao, pong);	// the base pass
							cmdList->SetTexture(Renderer_BindingsSrv::cacao_importance, importance);
							cmdList->SetTexture(Renderer_BindingsUav::tex_uint, loadCounter);
							cmdList->InsertComputeMemoryBarrier();	// the summed load before it's read
						}
						cmdList->SetTexture(Renderer_BindingsUav::cacao_ssao, ping);
					}
					break;
				case CacaoKernel::Blur:
					if ((bound = SetComputePipeline(cmdList, name, Renderer_Shader::cacao_blur)))
					{
						cmdList->SetTexture(Renderer_BindingsSrv::cacao_ssao, ping);
						cmdList->SetTexture(Renderer_BindingsUav::cacao_ssao, pong);
					}
					break;
				case CacaoKernel::Apply:
					if ((bound = SetComputePipeline(cmdList, name, Renderer_Shader::cacao_apply)))
					{
						cmdList->SetTexture(Renderer_BindingsSrv::cacao_ssao, dispatch.readsBlurred ? pong : ping);
						cmdList->SetTexture(Renderer_BindingsUav::tex, output);
					}
					break;
				case CacaoKernel::Upscale:
					if ((bound = SetComputePipeline(cmdList, name, run == CacaoQuality::Medium ? Renderer_Shader::cacao_upscale : Renderer_Shader::cacao_upscale_smart)))
					{
						cmdList->SetTexture(Renderer_BindingsSrv::cacao_ssao, dispatch.readsBlurred ? pong : ping);
						cmdList->SetTexture(Renderer_BindingsSrv::cacao_depth, depths);
						cmdList->SetTexture(Renderer_BindingsSrv::gbuffer_depth, depth);
						cmdList->SetTexture(Renderer_BindingsUav::tex, output);
					}
					break;
			}
			if (!bound || cmdList->IsStickyInvalid())
				return false;
			cmdList->Dispatch(dispatch.groupsX, dispatch.groupsY, dispatch.groupsZ);
		}

		output->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
		s_AoValid = true;
		s_AoReadout = { true, AoTechnique::Cacao, run, quality.halfResolution, settings, sizes.ssaoWidth, sizes.ssaoHeight };
		return true;
	}
}

// -------------------------------------------------------

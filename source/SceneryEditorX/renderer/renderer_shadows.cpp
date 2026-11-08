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
 * renderer_shadows.cpp
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 *
 * Sun and cloud shadows of the XP12 emulation (plan Phase 7; XP12: the CSM pass and cloud_shadows). Each frame
 * UpdateDrawCalls fits the cascades to the camera and the shadow light (renderer/csm.h) before it collects the CPU
 * casters; the environment buffer carries them to the shaders (WriteShadowConstants); Pass_ShadowCascades draws the
 * casters into shadow_csm, one layer per cascade; Pass_CloudShadow marches the cloud layers into cloud_shadow; the
 * lighting reads both through csm_sample.slang.
 */
#include "csm.h"
#include "env_probes.h"
#include "gbuffer.h"
#include "gpu_scene.h"
#include "renderer.h"
#include "renderer_declarations.h"
#include "vulkan/image_resource.h"
#include "vulkan/queue_manager.h"
#include "vulkan/rasterizer.h"
#include "vulkan/pipeline/pipeline_state.h"
#include "vulkan/shader/shader_manager.h"
#include <SceneryEditorX/environment/environment.h>
#include <SceneryEditorX/scene/material.h>
#include <SceneryEditorX/scene/components/renderable.h>
#include <algorithm>
#include <cmath>
#include <cstring>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		CsmSetup s_Cascades;						// This frame's (render thread)
		uint32_t s_ShadowLight = 0;					// 0 none, 1 sun, 2 moon
		std::array<CloudShadowCascade, CLOUD_SHADOW_CASCADES> s_CloudCascades{};
		uint32_t s_CloudCascadeCount = 0;
		uint64_t s_CloudShadowHash = 0;				// What cloud_shadow holds (0: nothing yet)

		// The casters' rasterizer state: depth clamp (casters nearer the light than a cascade land on its near plane) and
		// X-Plane's depth bias, made again when the bias controls change
		Ref<RasterizerState> s_CasterRasterizer;
		float s_CasterBiasConst = 0.0f;
		float s_CasterBiasSlope = 0.0f;

		bool IsUsable(ImageResource *image)
		{
			return image && image->Get() && *image->Get() != VK_NULL_HANDLE;
		}

		Shader *GetBuiltShader(const Renderer_Shader id)
		{
			if (!ShaderManager::IsShaderAvailable(id))
				return nullptr;
			Shader *shader = ShaderManager::GetShader(id);
			return shader && shader->IsCompiled() ? shader : nullptr;
		}

		/*
		 * Reverse-Z: the casters' depth is pushed away from the light (smaller), so a surface doesn't shadow itself:
		 * shadow/bias_const units of the depth format plus shadow/bias_slope times the depth slope. The pipeline takes
		 * the constant in units of 2^-24 (Pipeline: factor = floor(constant x 2^24)).
		 */
		RasterizerState *GetCasterRasterizer(const float biasConst, const float biasSlope)
		{
			if (!s_CasterRasterizer || biasConst != s_CasterBiasConst || biasSlope != s_CasterBiasSlope)
			{
				RasterStateSpec spec;
				spec.polygonMode = PolygonMode::Solid;
				spec.depthClipEnabled = false;	// depth clamp
				spec.depthBiasEnabled = true;
				spec.depthBiasConstant = -std::max(biasConst, 1.0f) / static_cast<float>(1 << 24);
				spec.depthBiasSlope = -std::max(biasSlope, 0.0f);
				spec.depthBiasClamp = 0.0f;
				spec.lineWidth = 1.0f;
				s_CasterRasterizer = CreateRef<RasterizerState>(spec);
				s_CasterBiasConst = biasConst;
				s_CasterBiasSlope = biasSlope;
			}
			return s_CasterRasterizer.Get();
		}

		uint64_t HashCloudShadow(const ConstantBuffer_Environment &env)
		{
			uint64_t hash = HashCombine(0, env.cloud_shadow.x);
			for (const float value : { env.cloud_shadow.y, env.cloud_shadow.z, env.cloud_shadow.w, env.csm_params.w, env.camera.y })
				hash = HashCombine(hash, value);
			for (const xMath::Vec4 &cascade : env.cloud_shadow_cascades)
			{
				hash = HashCombine(hash, cascade.x);
				hash = HashCombine(hash, cascade.y);
				hash = HashCombine(hash, cascade.z);
			}
			for (const xMath::Vec4 &layer : env.clouds)
			{
				hash = HashCombine(hash, layer.x);
				hash = HashCombine(hash, layer.y);
				hash = HashCombine(hash, layer.z);
				hash = HashCombine(hash, layer.w);
			}
			// the light, to about 0.01 degrees: the shadows move with the sun but aren't redone every frame for nothing
			const xMath::Vec4 &light = env.csm_params.w > 1.5f ? env.moon_direction : env.sun_direction;
			for (const float value : { light.x, light.y, light.z })
				hash = HashCombine(hash, std::round(value * 5000.0f));
			return hash == 0 ? 1 : hash;
		}
	}

	const CsmSetup &Renderer::GetShadowCascades()
	{
		return s_Cascades;
	}

	uint32_t Renderer::GetShadowLight()
	{
		return s_ShadowLight;
	}

	void Renderer::CreateShadowCascadeTarget(const uint32_t mapSize)
	{
		Ref<ImageResource> &target = GetRenderTargets()[static_cast<size_t>(Renderer_RenderTarget::shadow_csm)];
		if (target && target->GetWidth() == mapSize)
			return;
		if (target)
			QueueManager::WaitIdleAll();	// frames in flight still read the old one

		target = CreateRef<ImageResource>(ImgResourceSpec{ ImageType::Type2DArray, mapSize, mapSize, CSM_MAX_CASCADES, 1, VK_FORMAT_D32_SFLOAT,
			RenderTargetViews | ShaderViews, "shadow_csm" });
		SEDX_CORE_ASSERT(target != nullptr, "Failed to create shadow_csm render target");
	}

	void Renderer::UpdateShadowCascades(const CsmCamera *camera)
	{
		s_Cascades = CsmSetup{};
		s_ShadowLight = 0;
		s_CloudCascadeCount = 0;

		// A scene directional light replaces the environment's sun and moon (light composition); its own shadows aren't
		// drawn by the cascades
		if (!camera || m_Has_DirectionalLight)
			return;

		// The light: the sun while it's up, else the moon
		const Environment::World::Snapshot snapshot = Environment::Get().GetSnapshot();
		const Environment::EnvironmentDerived &derived = snapshot.derived;
		CsmVec3 towardLight{};
		if (derived.sunDirection.y > 0.0f)
		{
			s_ShadowLight = 1;
			towardLight = { derived.sunDirection.x, derived.sunDirection.y, derived.sunDirection.z };
		}
		else if (derived.moonDirection.y > 0.0f && derived.moon.illuminatedFraction > 0.0)
		{
			s_ShadowLight = 2;
			towardLight = { derived.moonDirection.x, derived.moonDirection.y, derived.moonDirection.z };
		}
		else
		{
			return;
		}

		const RendererFeatures &features = GetFeatures();
		const EnvironmentConstants &constants = GetEnvironmentConstants();
		if (features.csmShadows)
		{
			// the shadow quality sets these over the art controls, as X-Plane's renopt_shadow_quality does
			const ShadowQuality quality = GetShadowQuality(GetQualityPreset().shadows);
			if (quality.sceneryShadows)
			{
				CsmSettings settings = constants.shadow;
				settings.cascadeCount = static_cast<uint32_t>(std::clamp(quality.exteriorCascades, 1, static_cast<int>(CSM_MAX_CASCADES)));
				settings.farLimitM = quality.exteriorFarM;
				settings.fadeRatio = quality.fadeRatio;
				settings.mapSize = quality.mapSize;

				// X-Plane 12.4.4 keeps the ground's shadows out to kilometers when the eye is high, with the same 250 m far
				// limit (600 m up, buildings 3 km away still cast them): the split range grows with the camera's height above
				// the airport (world y), so the cascades cover the ground the view reaches (ASSUMPTION, plan TASK-096)
				const double heightM = std::max(0.0, camera->position[1]);
				settings.nearLimitM = std::max(settings.nearLimitM, heightM * constants.shadowHeightNearRatio);
				settings.farLimitM = std::max(settings.farLimitM, heightM * constants.shadowHeightFarRatio);
				FitCsm(settings, *camera, towardLight, s_Cascades);
			}
		}

		if (features.cloudShadows)
		{
			const bool clouds = std::ranges::any_of(snapshot.state.clouds, [](const Environment::CloudLayer &layer) { return layer.coverage > 0.0f; });
			if (clouds)
			{
				for (uint32_t i = 0; i < CLOUD_SHADOW_CASCADES; ++i)
					s_CloudCascades[i] = FitCloudShadowCascade(i, CLOUD_SHADOW_CASCADES, constants.cloudShadowExtentMinKm * 1000.0, constants.cloudShadowExtentKm * 1000.0,
						camera->position[0], camera->position[2]);
				s_CloudCascadeCount = CLOUD_SHADOW_CASCADES;
			}
		}
	}

	void Renderer::WriteShadowConstants(ConstantBuffer_Environment &env)
	{
		const EnvironmentConstants &constants = GetEnvironmentConstants();
		const CsmSetup &setup = s_Cascades;
		static_assert(sizeof(xMath::Matrix) == 16 * sizeof(float), "xMath::Matrix must be 16 floats");

		float split[CSM_MAX_CASCADES] = {}, blend[CSM_MAX_CASCADES] = {}, texel[CSM_MAX_CASCADES] = {};
		for (uint32_t i = 0; i < CSM_MAX_CASCADES; ++i)
		{
			float rows[16] = {};
			if (i < setup.count)
			{
				const CsmCascade &cascade = setup.cascades[i];
				for (size_t k = 0; k < 16; ++k)
					rows[k] = static_cast<float>(cascade.viewProjection[k]);
				split[i] = static_cast<float>(setup.splits[i + 1]);
				blend[i] = static_cast<float>(setup.blendBand[i]);
				texel[i] = static_cast<float>(cascade.texelSize);
			}
			std::memcpy(&env.csm_view_projection[i], rows, sizeof(rows));
		}
		env.csm_split = xMath::Vec4(split[0], split[1], split[2], split[3]);
		env.csm_blend = xMath::Vec4(blend[0], blend[1], blend[2], blend[3]);
		env.csm_texel = xMath::Vec4(texel[0], texel[1], texel[2], texel[3]);
		env.csm_params = xMath::Vec4(static_cast<float>(setup.count), static_cast<float>(setup.fadeStart), static_cast<float>(setup.fadeEnd),
			static_cast<float>(s_ShadowLight));
		ImageResource *target = GetRenderTarget(Renderer_RenderTarget::shadow_csm);
		const float filter = constants.shadowSoft ? (constants.shadowReceiverPlane ? 2.0f : 1.0f) : 0.0f;	// csm_sample.slang
		env.csm_filter = xMath::Vec4(constants.shadowNormalBias, constants.shadowNormalBiasSlope, filter, target ? static_cast<float>(target->GetWidth()) : 0.0f);

		const int steps = std::max(GetCloudQuality(GetQualityPreset().clouds).shadowSteps, 1);
		env.cloud_shadow = xMath::Vec4(static_cast<float>(s_CloudCascadeCount), constants.cloudShadowStrength, constants.cloudShadowExtinction,
			static_cast<float>(steps));
		for (uint32_t i = 0; i < CLOUD_SHADOW_CASCADES; ++i)
		{
			const CloudShadowCascade &cascade = s_CloudCascades[i];
			env.cloud_shadow_cascades[i] = i < s_CloudCascadeCount
				? xMath::Vec4(static_cast<float>(cascade.centerX), static_cast<float>(cascade.centerZ), static_cast<float>(cascade.halfExtentM),
							  static_cast<float>(cascade.texelM))
				: xMath::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
		}
	}

	void Renderer::BindShadows(CommandList *cmdList)
	{
		// Every shader that imports csm_sample declares both: bind what this one has
		if (cmdList->UsesTexture(Renderer_BindingsSrv::shadow_csm))
		{
			if (ImageResource *csm = GetRenderTarget(Renderer_RenderTarget::shadow_csm); IsUsable(csm))
				cmdList->SetTexture(Renderer_BindingsSrv::shadow_csm, csm);
		}
		if (cmdList->UsesTexture(Renderer_BindingsSrv::cloud_shadow))
		{
			if (ImageResource *clouds = GetRenderTarget(Renderer_RenderTarget::cloud_shadow); IsUsable(clouds))
				cmdList->SetTexture(Renderer_BindingsSrv::cloud_shadow, clouds);
		}
	}

	void Renderer::Pass_CloudShadow(CommandList *cmdList)
	{
		ImageResource *target = GetRenderTarget(Renderer_RenderTarget::cloud_shadow);
		if (!IsUsable(target))
			return;

		// Marched again only when the clouds, the light or the cascades moved (HashCloudShadow)
		const uint64_t hash = HashCloudShadow(m_Cb_Environment_Cpu);
		Shader *shader = GetBuiltShader(Renderer_Shader::cloud_shadow);
		if (s_CloudCascadeCount > 0 && s_ShadowLight != 0 && shader && hash != s_CloudShadowHash)
		{
			PipelineState pso;
			pso.name = "cloud_shadow";
			pso.shaders[static_cast<uint32_t>(StageType::Compute)] = shader;
			cmdList->SetPipelineState(pso);
			if (!cmdList->IsStickyInvalid())
			{
				target->SetLayout(Layout::ImageLayout::General, cmdList, 0, 0);
				cmdList->SetTexture(Renderer_BindingsUav::tex_sss, target);	// the RWTexture2DArray (binding 105)
				cmdList->Dispatch((target->GetWidth() + 7) / 8, (target->GetHeight() + 7) / 8, 1);
				s_CloudShadowHash = hash;
			}
		}
		target->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
	}

	void Renderer::Pass_ShadowCascades(CommandList *cmdList)
	{
		const uint32_t mapSize = GetShadowQuality(GetQualityPreset().shadows).mapSize;
		ImageResource *target = GetRenderTarget(Renderer_RenderTarget::shadow_csm);
		if (!IsUsable(target) || target->GetWidth() != mapSize)
		{
			CreateShadowCascadeTarget(mapSize);
			target = GetRenderTarget(Renderer_RenderTarget::shadow_csm);
		}
		if (!IsUsable(target))
			return;

		const CsmSetup &setup = s_Cascades;
		Shader *gpuShader = GetBuiltShader(Renderer_Shader::depth_light_csm_gpu);
		Shader *cpuShader = GetBuiltShader(Renderer_Shader::depth_light_csm);
		if (setup.count == 0 || (!gpuShader && !cpuShader))
		{
			// nothing samples it (no cascades), but every pass that declares it finds it readable
			target->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
			return;
		}

		const bool gpuScene = gpuShader && GpuScene::HasDraws();
		if (gpuScene)
			GpuScene::CullShadows(cmdList, setup.count);

		const EnvironmentConstants &constants = GetEnvironmentConstants();
		const bool receiverPlane = constants.shadowSoft && constants.shadowReceiverPlane;
		RasterizerState *rasterizer = GetCasterRasterizer(constants.shadowBiasConst, receiverPlane ? constants.shadowReceiverPlaneSlopeBias : constants.shadowBiasSlope);
		constexpr uint64_t commandStride = sizeof(VkDrawIndexedIndirectCommand);

		for (uint32_t cascade = 0; cascade < setup.count; ++cascade)
		{
			PipelineState pso;
			pso.name = "csm_casters";
			pso.rasterizerState = rasterizer;
			pso.blendState = GetBlendState(Renderer_BlendState::Off);
			pso.depthStencil_State = GetDepthStencilState(Renderer_DepthStencilState::ReadWrite);
			pso.renderTarget_DepthTexture = target;
			pso.renderTarget_ArrayIndex = cascade;

			// the first pipeline of a cascade clears its layer (reverse-Z: 0 = far), the rest load
			bool cleared = false;
			const auto setPipeline = [&](Shader *vertex, Shader *fragment)
			{
				pso.shaders[static_cast<uint32_t>(StageType::Vertex)] = vertex;
				pso.shaders[static_cast<uint32_t>(StageType::Fragment)] = fragment;
				pso.clearDepth = cleared ? RHI_DEPTH_LOAD : 0.0f;
				cmdList->SetPipelineState(pso);
				cleared = true;
				return !cmdList->IsStickyInvalid();
			};
			const auto pushCascade = [&](const uint32_t drawIndex, const uint32_t materialIndex)
			{
				m_Pcb_Pass_Cpu.drawIndex = drawIndex;
				m_Pcb_Pass_Cpu.isTransparent = 0;
				m_Pcb_Pass_Cpu.materialIndex = materialIndex;
				m_Pcb_Pass_Cpu.SetF4Value(static_cast<float>(cascade), 0.0f, 0.0f, 0.0f);
				cmdList->PushConstants(m_Pcb_Pass_Cpu);
			};

			// GPU scene: one instanced indirect draw per bucket, from this cascade's commands (GpuScene::CullShadows).
			// No culling: a single-sided wall still blocks the light coming from behind it.
			if (gpuScene)
			{
				for (const GpuScene::Bucket &bucket : GpuScene::GetBuckets())
				{
					if (bucket.commandCount == 0)
						continue;
					if (!setPipeline(gpuShader, bucket.alphaTested ? gpuShader : nullptr))
						return;
					pushCascade(0, 0);
					cmdList->SetIndexBuffer(GeometryBuffer::GetIndexBuffer());
					cmdList->SetCullMode(CullMode::None);
					const uint64_t first = static_cast<uint64_t>(GpuScene::GetShadowCommandOffset(cascade)) + bucket.firstCommand;
					cmdList->DrawIndexedIndirect(GpuScene::GetCommandBuffer(), first * commandStride, bucket.commandCount);
				}
			}

			// The casters the CPU draws (instanced, tessellated, everything when the GPU scene is off)
			if (cpuShader)
			{
				Shader *currentFragment = nullptr;
				bool cpuPipeline = false;
				for (uint32_t i = 0; i < m_DrawCalls_Shadow_Count; ++i)
				{
					const Renderer_DrawCall &drawCall = m_DrawCalls_Shadow[i];
					if ((drawCall.shadowCascades & (1u << cascade)) == 0)
						continue;
					Renderable *renderable = drawCall.renderable;
					MaterialAsset *material = renderable->GetMaterialAsset();
					if (!material)
						continue;

					// cut-out materials test their texels (as the GPU scene's alpha-tested buckets do), the rest need no fragment shader
					const bool alphaTested = material->IsAlphaTested() || material->GetProperty(MaterialProperty::AlphaCutoff) > 0.0f;
					Shader *fragment = alphaTested ? cpuShader : nullptr;
					if (!cpuPipeline || fragment != currentFragment)
					{
						if (!setPipeline(cpuShader, fragment))
							return;
						cpuPipeline = true;
						currentFragment = fragment;
					}

					pushCascade(drawCall.drawData_Index, material->GetIndex());
					cmdList->SetCullMode(CullMode::None);
					cmdList->SetVertexBuffer(renderable->GetVertexBuffer(), renderable->GetInstanceBuffer());
					cmdList->SetIndexBuffer(renderable->GetIndexBuffer());
					cmdList->DrawIndexed(renderable->GetIndexCount(drawCall.lodIndex), drawCall.instanceCount, renderable->GetIndexOffset(drawCall.lodIndex),
										 renderable->GetVertexOffset(drawCall.lodIndex), drawCall.instanceIndex);
				}
			}

			// a cascade nothing was drawn into still starts clear
			if (!cleared)
			{
				pso.shaders = {};
				pso.clearDepth = 0.0f;
				cmdList->ClearPipelineStateRenderTargets(pso);
			}
		}

		target->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
	}
}

// -------------------------------------------------------

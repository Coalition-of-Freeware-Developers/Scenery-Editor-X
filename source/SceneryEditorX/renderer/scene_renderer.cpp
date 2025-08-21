/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* scene_renderer.cpp
* -------------------------------------------------------
* Created: 23/7/2025
* -------------------------------------------------------
*/
//#include "scene_renderer.h"
//#include "compute_pass.h"
//#include "debug_renderer.h"
//#include "primitives.h"
//#include "renderer.h"

//#include "SceneryEditorX/core/application/application.h"
//#include "SceneryEditorX/core/time/timer.h"
//#include "SceneryEditorX/logging/profiler.hpp"

/// -------------------------------------------------------

namespace SceneryEditorX
{

	/*
	LOCAL std::vector<std::thread> s_ThreadPool;

    /// -------------------------------------------------------

	SceneRenderer::SceneRenderer(const Ref<Scene> &scene, const SceneRendererSpecification &specification)
        : m_Scene(scene), m_Specification(specification), m_SceneData()
    {
        Init();
    }

	SceneRenderer::~SceneRenderer()
	{
		Shutdown();
	}

    /// -------------------------------------------------------

	void SceneRenderer::Init()
	{
		SEDX_SCOPE_TIMER("SceneRenderer::Init")

		m_ViewportWidth = m_Specification.ViewportWidth;
		m_ViewportHeight = m_Specification.ViewportHeight;
		if (m_ViewportWidth > 0 && m_ViewportHeight > 0)
			m_NeedsResize = true;

		m_ShadowCascadeSplits[0] = 0.1f;
		m_ShadowCascadeSplits[1] = 0.2f;
		m_ShadowCascadeSplits[2] = 0.3f;
		m_ShadowCascadeSplits[3] = 1.0f;

		///< Tiering
		{
			//using namespace Tiering::Renderer;

			const auto& tiering = m_Specification.Tiering;
			RendererDataUB.SoftShadows = tiering.ShadowQuality == Tiering::Renderer::ShadowQualitySetting::High;
			m_Options.EnableGTAO = false;

			switch (tiering.AOQuality)
			{
				case Tiering::Renderer::AmbientOcclusionQualitySetting::High:
					m_Options.EnableGTAO = true;
					GTAODataCB.HalfRes = true;
					break;
				case Tiering::Renderer::AmbientOcclusionQualitySetting::Ultra:
					m_Options.EnableGTAO = true;
					GTAODataCB.HalfRes = false;
					break;
			}

			if (tiering.EnableAO)
			{
				switch (tiering.AOType)
				{
					case Tiering::Renderer::AmbientOcclusionTypeSetting::GTAO:
						m_Options.EnableGTAO = true;
						break;
				}
			}

			switch (tiering.SSRQuality)
			{
				case Tiering::Renderer::SSRQualitySetting::Medium:
					m_Options.EnableSSR = true;
					m_SSROptions.HalfRes = true;
					break;
				case Tiering::Renderer::SSRQualitySetting::High:
					m_Options.EnableSSR = true;
					m_SSROptions.HalfRes = false;
					break;
			}

		}

		m_CommandBuffer = CreateRef<CommandBuffer>(0, "SceneRenderer");

		uint32_t framesInFlight = Renderer::GetRenderData().framesInFlight;

		m_UBSCamera = CreateRef<UniformBufferSet>(sizeof(UBOCamera));
		m_UBSShadow = CreateRef<UniformBufferSet>(sizeof(UBOShadow));
		m_UBSScene = CreateRef<UniformBufferSet>(sizeof(UBOScene));
		m_UBSRendererData = CreateRef<UniformBufferSet>(sizeof(UBORendererData));
		m_UBSPointLights = CreateRef<UniformBufferSet>(sizeof(UBOPointLights));
		m_UBSScreenData = CreateRef<UniformBufferSet>(sizeof(UBOScreenData));
		m_UBSSpotLights = CreateRef<UniformBufferSet>(sizeof(UBOSpotLights));
		m_UBSSpotShadowData = CreateRef<UniformBufferSet>(sizeof(UBOSpotShadowData));

		{
			StorageBufferSpec spec;
			spec.debugName = "BoneTransforms";
			spec.GPUOnly = false;
            constexpr size_t BoneTransformBufferCount = 1024 * 50; ///< This provides enough for 1024 animated entities at an average of 50 bones each
            m_SBSBoneTransforms = CreateRef<StorageBufferSet>(spec, sizeof(Mat4) * BoneTransformBufferCount);
			m_BoneTransformsData = hnew Mat4[BoneTransformBufferCount];
		}

		/**
		 * Create passes and specify "flow" -> this can (and should) include outputs and chain together,
		 * for eg. shadow pass output goes into geo pass input
		 #1#
		m_Renderer2D = CreateRef<Renderer2D>();
		m_Renderer2DScreenSpace = CreateRef<Renderer2D>();
		m_DebugRenderer = CreateRef<DebugRenderer>();

		m_CompositeShader = Renderer::GetShaderLibrary()->Get("SceneComposite");
        m_CompositeMaterial = CreateRef<Material>(m_CompositeShader);

		/**
		 * Descriptor Set Layout
         * Light culling compute pipeline
		 #1#
		{
			StorageBufferSpec spec;
			spec.debugName = "VisiblePointLightIndices";
            m_SBSVisiblePointLightIndicesBuffer = CreateRef<StorageBufferSet>(spec, 1); ///< Resized later

			spec.debugName = "VisibleSpotLightIndices";
            m_SBSVisibleSpotLightIndicesBuffer = CreateRef<StorageBufferSet>(spec, 1); ///< Resized later

			m_LightCullingMaterial = CreateRef<Material>(Renderer::GetShaderLibrary()->Get("LightCulling"), "LightCulling");
			Ref<Shader> lightCullingShader = Renderer::GetShaderLibrary()->Get("LightCulling");
            m_LightCullingPipeline = CreateRef<ComputePipeline>(lightCullingShader);
		}

		VertexBufferLayout vertexLayout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float3, "a_Normal" },
			{ ShaderDataType::Float3, "a_Tangent" },
			{ ShaderDataType::Float3, "a_Binormal" },
			{ ShaderDataType::Float2, "a_TexCoord" }
		};

		VertexBufferLayout instanceLayout = {
			{ ShaderDataType::Float4, "a_MRow0" },
			{ ShaderDataType::Float4, "a_MRow1" },
			{ ShaderDataType::Float4, "a_MRow2" },
		};

		VertexBufferLayout boneInfluenceLayout = {
			{ ShaderDataType::Int4,   "a_BoneIDs" },
			{ ShaderDataType::Float4, "a_BoneWeights" },
		};

		uint32_t shadowMapResolution = 4096;
		switch (m_Specification.Tiering.ShadowResolution)
		{
			case Tiering::Renderer::ShadowResolutionSetting::Low:
				shadowMapResolution = 1024;
				break;
			case Tiering::Renderer::ShadowResolutionSetting::Medium:
				shadowMapResolution = 2048;
				break;
			case Tiering::Renderer::ShadowResolutionSetting::High:
				shadowMapResolution = 4096;
				break;
		}

		///< Bloom Compute
		{
			auto shader = Renderer::GetShaderLibrary()->Get("Bloom");
            m_BloomComputePipeline = CreateRef<ComputePipeline>(shader);
			{
				TextureSpecification spec;
                spec.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
				spec.width = 1;
				spec.height = 1;
				spec.samplerWrap = SamplerWrap::Clamp;
				spec.storage = true;
				spec.generateMips = true;
				spec.debugName = "BloomCompute-0";
                m_BloomComputeTextures[0].Texture = CreateRef<Texture2D>(spec);
				spec.debugName = "BloomCompute-1";
                m_BloomComputeTextures[1].Texture = CreateRef<Texture2D>(spec);
				spec.debugName = "BloomCompute-2";
                m_BloomComputeTextures[2].Texture = CreateRef<Texture2D>(spec);
			}

			///< Image Views (per-mip)
			ImageViewData imageViewSpec;
			for (int i = 0; i < 3; i++)
			{
				imageViewSpec.debugName = std::format("BloomCompute-{}", i);
				uint32_t mipCount = m_BloomComputeTextures[i].Texture->GetMipLevelCount();
				m_BloomComputeTextures[i].ImageViews.resize(mipCount);
				for (uint32_t mip = 0; mip < mipCount; mip++)
				{
					imageViewSpec.image = m_BloomComputeTextures[i].Texture->GetImage();
					imageViewSpec.mip = mip;
                    m_BloomComputeTextures[i].ImageViews[mip] = CreateRef<ImageView>(imageViewSpec);
				}
			}

			{
				ComputePassSpecification spec;
				spec.debugName = "Bloom-Compute";
				auto shaderRef = Renderer::GetShaderLibrary()->Get("Bloom");
                spec.pipeline = CreateRef<ComputePipeline>(shaderRef);
                m_BloomComputePass = CreateRef<ComputePass>(spec);
				SEDX_CORE_VERIFY(m_BloomComputePass->Validate());
				m_BloomComputePass->Bake();
			}

			m_BloomDirtTexture = Renderer::GetBlackTexture();

			/**
			 * TODO: m_BloomComputePrefilterMaterial = Material::Create(shader);
             #1#
		}

		///< Directional Shadow pass
		{
			ImageSpecification spec;
            spec.format = VkFormat::VK_FORMAT_D32_SFLOAT;
			spec.usage = Layout::Attachment;
			spec.width = shadowMapResolution;
			spec.height = shadowMapResolution;
			spec.layers = m_Specification.NumShadowCascades;
			spec.debugName = "ShadowCascades";
            Ref<Image2D> cascadedDepthImage = CreateRef<Image2D>(spec);
			cascadedDepthImage->Invalidate();
			if (m_Specification.NumShadowCascades > 1)
				cascadedDepthImage->CreatePerLayerImageViews();

			FramebufferSpecification shadowMapFramebufferSpec;
			shadowMapFramebufferSpec.debugName = "Dir Shadow Map";
			shadowMapFramebufferSpec.width = shadowMapResolution;
			shadowMapFramebufferSpec.height = shadowMapResolution;
            shadowMapFramebufferSpec.attachments = {VkFormat::VK_FORMAT_D32_SFLOAT};
			shadowMapFramebufferSpec.clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
			shadowMapFramebufferSpec.depthClearValue = 1.0f;
			shadowMapFramebufferSpec.noResize = true;
			shadowMapFramebufferSpec.existingImage = cascadedDepthImage;

			auto shadowPassShader = Renderer::GetShaderLibrary()->Get("DirShadowMap");
			auto shadowPassShaderAnim = Renderer::GetShaderLibrary()->Get("DirShadowMap_Anim");

			PipelineData pipelineSpec;
			pipelineSpec.debugName = "DirShadowPass";
			pipelineSpec.shader = shadowPassShader;
			pipelineSpec.depthOperator = DepthCompareOperator::LessOrEqual;
			pipelineSpec.layout = vertexLayout;
			pipelineSpec.instanceLayout = instanceLayout;

			PipelineData pipelineSpecAnim = pipelineSpec;
			pipelineSpecAnim.debugName = "DirShadowPass-Anim";
			pipelineSpecAnim.shader = shadowPassShaderAnim;
			pipelineSpecAnim.boneInfluenceLayout = boneInfluenceLayout;

			RenderSpec shadowMapRenderPassSpec;
			shadowMapRenderPassSpec.debugName = shadowMapFramebufferSpec.debugName;

			///< 4 cascades by default
			m_DirectionalShadowMapPass.resize(m_Specification.NumShadowCascades);
			m_DirectionalShadowMapAnimPass.resize(m_Specification.NumShadowCascades);
			for (uint32_t i = 0; i < m_Specification.NumShadowCascades; i++)
			{
				shadowMapFramebufferSpec.existingImageLayers.clear();
				shadowMapFramebufferSpec.existingImageLayers.emplace_back(i);

				shadowMapFramebufferSpec.clearDepthOnLoad = true;
                pipelineSpec.dstFramebuffer = CreateRef<Framebuffer>(shadowMapFramebufferSpec);

				m_ShadowPassPipelines[i] = CreateRef<Pipeline>(pipelineSpec);
				shadowMapRenderPassSpec.Pipeline = m_ShadowPassPipelines[i];

				shadowMapFramebufferSpec.clearDepthOnLoad = false;
                pipelineSpecAnim.dstFramebuffer = CreateRef<Framebuffer>(shadowMapFramebufferSpec);
                m_ShadowPassPipelinesAnim[i] = CreateRef<Pipeline>(pipelineSpecAnim);

				m_DirectionalShadowMapPass[i] = CreateRef<RenderPass>(shadowMapRenderPassSpec);
				m_DirectionalShadowMapPass[i]->AddInput("ShadowData", m_UBSShadow);
				SEDX_CORE_VERIFY(m_DirectionalShadowMapPass[i]->Validate());
				m_DirectionalShadowMapPass[i]->Bake();

				shadowMapRenderPassSpec.Pipeline = m_ShadowPassPipelinesAnim[i];
                m_DirectionalShadowMapAnimPass[i] = CreateRef<RenderPass>(shadowMapRenderPassSpec);
				m_DirectionalShadowMapAnimPass[i]->AddInput("ShadowData", m_UBSShadow);
				m_DirectionalShadowMapAnimPass[i]->AddInput("BoneTransforms", m_SBSBoneTransforms);
				SEDX_CORE_VERIFY(m_DirectionalShadowMapAnimPass[i]->Validate());
				m_DirectionalShadowMapAnimPass[i]->Bake();
			}
            m_ShadowPassMaterial = CreateRef<Material>(shadowPassShader, "DirShadowPass");
		}

		///< Non-directional shadow mapping pass
		{
			FramebufferSpecification framebufferSpec;
			framebufferSpec.width = shadowMapResolution; // TODO: could probably halve these
			framebufferSpec.height = shadowMapResolution;
            framebufferSpec.attachments = {VK_FORMAT_D32_SFLOAT};
			framebufferSpec.depthClearValue = 1.0f;
			framebufferSpec.noResize = true;
			framebufferSpec.debugName = "Spot Shadow Map";

			auto shadowPassShader = Renderer::GetShaderLibrary()->Get("SpotShadowMap");
			auto shadowPassShaderAnim = Renderer::GetShaderLibrary()->Get("SpotShadowMap_Anim");

			PipelineData pipelineSpec;
			pipelineSpec.debugName = "SpotShadowPass";
			pipelineSpec.shader = shadowPassShader;
            pipelineSpec.dstFramebuffer = CreateRef<Framebuffer>(framebufferSpec);
			pipelineSpec.depthOperator = DepthCompareOperator::LessOrEqual;

			pipelineSpec.layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float3, "a_Normal" },
				{ ShaderDataType::Float3, "a_Tangent" },
				{ ShaderDataType::Float3, "a_Binormal" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpec.instanceLayout = {
				{ ShaderDataType::Float4, "a_MRow0" },
				{ ShaderDataType::Float4, "a_MRow1" },
				{ ShaderDataType::Float4, "a_MRow2" },
			};

			PipelineData pipelineSpecAnim = pipelineSpec;
			pipelineSpecAnim.debugName = "SpotShadowPass-Anim";
			pipelineSpecAnim.shader = shadowPassShaderAnim;
			pipelineSpecAnim.boneInfluenceLayout = boneInfluenceLayout;

			m_SpotShadowPassPipeline = CreateRef<Pipeline>(pipelineSpec);
            m_SpotShadowPassAnimPipeline = CreateRef<Pipeline>(pipelineSpecAnim);

			m_SpotShadowPassMaterial = CreateRef<Material>(shadowPassShader, "SpotShadowPass");

			RenderSpec spotShadowPassSpec;
			spotShadowPassSpec.debugName = "SpotShadowMap";
			spotShadowPassSpec.Pipeline = m_SpotShadowPassPipeline;
            m_SpotShadowPass = CreateRef<RenderPass>(spotShadowPassSpec);

			m_SpotShadowPass->AddInput("SpotShadowData", m_UBSSpotShadowData);

			SEDX_CORE_VERIFY(m_SpotShadowPass->Validate());
			m_SpotShadowPass->Bake();
		}

		///< Define/setup renderer resources that are provided by CPU (meaning not GPU pass outputs)
		///< eg. camera buffer, environment textures, BRDF LUT, etc.

		///< CHANGES:
		///< - RenderPass class becomes implemented instead of just placeholder for data
		///< - Pipeline no longer contains render pass in spec (NOTE: double check this is okay)
		///< - RenderPass contains Pipeline in spec

		{
			///
			/// A render pass should provide context and initiate (prepare) certain layout transitions for all
			/// required resources. This should be easy to check and ensure everything is ready.
			///
			/// Passes should be somewhat pre-baked to contain ready-to-go descriptors that aren't draw-call
			/// specific. Hazel defines Set 0 as per-draw - so usually materials. Sets 1-3 are scene/renderer owned.
			///
			/// This means that when you define a render pass, you need to set-up required inputs from Sets 1-3
			/// and based on what is used here, these need to get baked into ready-to-go allocated descriptor sets,
			/// for that specific render pass draw call (so we can use vkCmdBindDescriptorSets).
			///
			/// API could look something like:

			/// Ref<RenderPass> shadowMapRenderPass[4]; // Four shadow map passes, into single layered framebuffer
			///
			/// {
			/// 	for (int i = 0; i < 4; i++)
			/// 	{
			/// 		RenderSpec spec;
			/// 		spec.debugName = "ShadowMapPass";
			/// 		spec.Pipeline = m_ShadowPassPipelines[i];
			/// 		//spec.dstFramebuffer = m_ShadowPassPipelines[i]->GetSpecification().RenderPass->GetSpecification().dstFramebuffer; // <- set framebuffer here
			///
			/// 		shadowMapRenderPass[i] = RenderPass::Create(spec);
			/// 		// shadowMapRenderPass[i]->GetRequiredInputs(); // Returns list of sets+bindings of required resources from descriptor layout
			///
			/// 		// NOTE:
			/// 		// AddInput needs to potentially take in the set + binding of the resource
			/// 		// We (currently) don't store the actual variable name, just the struct type (eg. ShadowData and not u_ShadowData),
			/// 		// so if there are multiple instances of the ShadowData struct then it's ambiguous.
			/// 		// I suspect clashes will be rare - usually we only have one input per type/buffer
			/// 		shadowMapRenderPass[i]->AddInput("ShadowData", m_UBSShadow);
			/// 		// Note: outputs are automatically set by framebuffer
			///
			/// 		// Bake will create descriptor sets and ensure everything is ready for rendering
			/// 		// If resources (eg. storage buffers/images) resize, passes need to be invalidated
			/// 		// so we can re-create proper descriptors to the newly created replacement resources
			/// 		SEDX_CORE_VERIFY(shadowMapRenderPass[i]->Validate());
			/// 		shadowMapRenderPass[i]->Bake();
			/// 	}
			/// }

			/// PreDepth
			{
				FramebufferSpecification preDepthFramebufferSpec;
				preDepthFramebufferSpec.width = m_Specification.ViewportWidth;
				preDepthFramebufferSpec.height = m_Specification.ViewportHeight;
				preDepthFramebufferSpec.debugName = "PreDepth-Opaque";

                ///< Linear depth, reversed device depth
                preDepthFramebufferSpec.attachments = {/*VkFormat::VK_FORMAT_R32_SFLOAT, #1# VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT};
				preDepthFramebufferSpec.clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
				preDepthFramebufferSpec.depthClearValue = 0.0f;

				Ref<Framebuffer> clearFramebuffer = CreateRef<Framebuffer>(preDepthFramebufferSpec);
				preDepthFramebufferSpec.clearDepthOnLoad = false;
				preDepthFramebufferSpec.existingImages[0] = clearFramebuffer->GetDepthImage();
                Ref<Framebuffer> loadFramebuffer = CreateRef<Framebuffer>(preDepthFramebufferSpec);

				PipelineData pipelineSpec;
				pipelineSpec.debugName = preDepthFramebufferSpec.debugName;
				pipelineSpec.dstFramebuffer = clearFramebuffer;
				pipelineSpec.shader = Renderer::GetShaderLibrary()->Get("PreDepth");
				pipelineSpec.layout = vertexLayout;
				pipelineSpec.instanceLayout = instanceLayout;
                m_PreDepthPipeline = CreateRef<Pipeline>(pipelineSpec);
                m_PreDepthMaterial = CreateRef<Material>(pipelineSpec.shader, pipelineSpec.debugName);

				///< Change to loading framebuffer so we don't clear
				pipelineSpec.dstFramebuffer = loadFramebuffer;

				pipelineSpec.debugName = "PreDepth-Anim";
				pipelineSpec.shader = Renderer::GetShaderLibrary()->Get("PreDepth_Anim");
				pipelineSpec.boneInfluenceLayout = boneInfluenceLayout;
                m_PreDepthPipelineAnim = CreateRef<Pipeline>(pipelineSpec); ///< same renderpass as Predepth-Opaque

				pipelineSpec.debugName = "PreDepth-Transparent";
				pipelineSpec.shader = Renderer::GetShaderLibrary()->Get("PreDepth");
				preDepthFramebufferSpec.debugName = pipelineSpec.debugName;
                m_PreDepthTransparentPipeline = CreateRef<Pipeline>(pipelineSpec);

				///< TODO: Need PreDepth-Transparent-Animated pipeline

				///< Static
				RenderSpec preDepthRenderPassSpec;
				preDepthRenderPassSpec.debugName = preDepthFramebufferSpec.debugName;
				preDepthRenderPassSpec.Pipeline = m_PreDepthPipeline;

				m_PreDepthPass = CreateRef<RenderPass>(preDepthRenderPassSpec);
				m_PreDepthPass->AddInput("Camera", m_UBSCamera);
				SEDX_CORE_VERIFY(m_PreDepthPass->Validate());
				m_PreDepthPass->Bake();

				///< Animated
				preDepthRenderPassSpec.debugName = "PreDepth-Anim";
				preDepthRenderPassSpec.Pipeline = m_PreDepthPipelineAnim;
                m_PreDepthAnimPass = CreateRef<RenderPass>(preDepthRenderPassSpec);
				m_PreDepthAnimPass->AddInput("Camera", m_UBSCamera);
				m_PreDepthAnimPass->AddInput("BoneTransforms", m_SBSBoneTransforms);
				SEDX_CORE_VERIFY(m_PreDepthAnimPass->Validate());
				m_PreDepthAnimPass->Bake();

				///< Transparent
				preDepthRenderPassSpec.debugName = "PreDepth-Transparent";
				preDepthRenderPassSpec.Pipeline = m_PreDepthTransparentPipeline;
                m_PreDepthTransparentPass = CreateRef<RenderPass>(preDepthRenderPassSpec);
				m_PreDepthTransparentPass->AddInput("Camera", m_UBSCamera);

				SEDX_CORE_VERIFY(m_PreDepthTransparentPass->Validate());
				m_PreDepthTransparentPass->Bake();
			}

			///< Geometry
		    {
				ImageSpecification imageSpec;
				imageSpec.width = Application::Get().GetWindow().GetWidth();
				imageSpec.height = Application::Get().GetWindow().GetHeight();
                imageSpec.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
				imageSpec.usage = Layout::Attachment;
				imageSpec.debugName = "GeometryPass-ColorAttachment0";
                m_GeometryPassColorAttachmentImage = CreateRef<Image2D>(imageSpec);
				m_GeometryPassColorAttachmentImage->Invalidate();

				FramebufferSpecification geoFramebufferSpec;
				geoFramebufferSpec.width = m_Specification.ViewportWidth;
				geoFramebufferSpec.height = m_Specification.ViewportHeight;
                geoFramebufferSpec.attachments = {VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT,
                                                  VkFormat::VK_FORMAT_R16G16_SFLOAT,
                                                  VkFormat::VK_FORMAT_R8G8B8A8_UNORM,
                                                  VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT};
				geoFramebufferSpec.existingImages[3] = m_PreDepthPass->GetDepthOutput();

				///< Don't clear primary color attachment (skybox pass writes into it)
				geoFramebufferSpec.attachments.Attachments[0].LoadOp = AttachmentLoadOp::Load;
                ///< Don't blend with luminance in the alpha channel.
				geoFramebufferSpec.attachments.Attachments[1].Blend = false;
				geoFramebufferSpec.samples = 1;
				geoFramebufferSpec.clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
				geoFramebufferSpec.debugName = "Geometry";
				geoFramebufferSpec.clearDepthOnLoad = false;
                Ref<Framebuffer> clearFramebuffer = CreateRef<Framebuffer>(geoFramebufferSpec);
				geoFramebufferSpec.clearColorOnLoad = false;
				geoFramebufferSpec.existingImages[0] = clearFramebuffer->GetImage(0);
				geoFramebufferSpec.existingImages[1] = clearFramebuffer->GetImage(1);
				geoFramebufferSpec.existingImages[2] = clearFramebuffer->GetImage(2);
				geoFramebufferSpec.existingImages[3] = m_PreDepthPass->GetDepthOutput();
                Ref<Framebuffer> loadFramebuffer = CreateRef<Framebuffer>(geoFramebufferSpec);

				PipelineData pipelineSpecification;
				pipelineSpecification.debugName = "PBR-Static";
				pipelineSpecification.shader = Renderer::GetShaderLibrary()->Get("HazelPBR_Static");
				pipelineSpecification.dstFramebuffer = clearFramebuffer;
				pipelineSpecification.depthOperator = DepthCompareOperator::Equal;
				pipelineSpecification.depthWrite = false;
				pipelineSpecification.layout = vertexLayout;
				pipelineSpecification.instanceLayout = instanceLayout;
				pipelineSpecification.lineWidth = m_LineWidth;
                m_GeometryPipeline = CreateRef<Pipeline>(pipelineSpecification);

				///< Switch to load framebuffer to not clear subsequent passes
				pipelineSpecification.dstFramebuffer = loadFramebuffer;

				///<
				///< Transparent Geometry
				///<
				pipelineSpecification.debugName = "PBR-Transparent";
				pipelineSpecification.shader = Renderer::GetShaderLibrary()->Get("HazelPBR_Transparent");
				pipelineSpecification.depthOperator = DepthCompareOperator::GreaterOrEqual;
                m_TransparentGeometryPipeline = CreateRef<Pipeline>(pipelineSpecification);

				///<
				///< Animated Geometry
				///<
				pipelineSpecification.debugName = "PBR-Anim";
				pipelineSpecification.shader = Renderer::GetShaderLibrary()->Get("HazelPBR_Anim");
				pipelineSpecification.depthOperator = DepthCompareOperator::Equal;
				pipelineSpecification.boneInfluenceLayout = boneInfluenceLayout;
                m_GeometryPipelineAnim = CreateRef<Pipeline>(pipelineSpecification);

				///< TODO: Need Transparent-Animated geometry pipeline
			}

			///< Selected Geometry isolation (for outline pass)
		    {
				FramebufferSpecification framebufferSpec;
				framebufferSpec.width = m_Specification.ViewportWidth;
				framebufferSpec.height = m_Specification.ViewportHeight;
				framebufferSpec.debugName = "SelectedGeometry";
                framebufferSpec.attachments = {VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, RenderContext::GetCurrentDevice()->GetPhysicalDevice()->GetDepthFormat()};
				framebufferSpec.samples = 1;
				framebufferSpec.clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
				framebufferSpec.depthClearValue = 1.0f;

				PipelineData pipelineSpecification;
				pipelineSpecification.debugName = framebufferSpec.debugName;
                pipelineSpecification.dstFramebuffer = CreateRef<Framebuffer>(framebufferSpec);
				pipelineSpecification.shader = Renderer::GetShaderLibrary()->Get("SelectedGeometry");
				pipelineSpecification.layout = vertexLayout;
				pipelineSpecification.instanceLayout = instanceLayout;
				pipelineSpecification.depthOperator = DepthCompareOperator::LessOrEqual;

				RenderSpec rpSpec;
				rpSpec.debugName = "SelectedGeometry";
                rpSpec.Pipeline = CreateRef<Pipeline>(pipelineSpecification);
                m_SelectedGeometryPass = CreateRef<RenderPass>(rpSpec);
				m_SelectedGeometryPass->AddInput("Camera", m_UBSCamera);
				SEDX_CORE_VERIFY(m_SelectedGeometryPass->Validate());
				m_SelectedGeometryPass->Bake();

				m_SelectedGeometryMaterial = CreateRef<Material>(pipelineSpecification.shader, pipelineSpecification.debugName);

				pipelineSpecification.debugName = "SelectedGeometry-Anim";
				pipelineSpecification.shader = Renderer::GetShaderLibrary()->Get("SelectedGeometry_Anim");
				pipelineSpecification.boneInfluenceLayout = boneInfluenceLayout;
				framebufferSpec.existingFramebuffer = m_SelectedGeometryPass->GetTargetFramebuffer();
				framebufferSpec.clearColorOnLoad = false;
				framebufferSpec.clearDepthOnLoad = false;
                pipelineSpecification.dstFramebuffer = CreateRef<Framebuffer>(framebufferSpec);
                rpSpec.Pipeline = CreateRef<Pipeline>(pipelineSpecification);
				m_SelectedGeometryAnimPass = CreateRef<RenderPass>(rpSpec); ///< Note: same framebuffer and render pass as m_SelectedGeometryPipeline
				m_SelectedGeometryAnimPass->AddInput("Camera", m_UBSCamera);
				m_SelectedGeometryAnimPass->AddInput("BoneTransforms", m_SBSBoneTransforms);
				SEDX_CORE_VERIFY(m_SelectedGeometryAnimPass->Validate());
				m_SelectedGeometryAnimPass->Bake();
			}

			{
				RenderSpec spec;
				spec.debugName = "GeometryPass";
				spec.Pipeline = m_GeometryPipeline;

				m_GeometryPass = CreateRef<RenderPass>(spec);
				//m_GeometryPass->GetRequiredInputs(); // Returns list of sets+bindings of required resources from descriptor layout (currently unused)

				m_GeometryPass->AddInput("Camera", m_UBSCamera);
				m_GeometryPass->AddInput("SpotShadowData", m_UBSSpotShadowData);
				m_GeometryPass->AddInput("ShadowData", m_UBSShadow);
				m_GeometryPass->AddInput("PointLightData", m_UBSPointLights);
				m_GeometryPass->AddInput("SpotLightData", m_UBSSpotLights);
				m_GeometryPass->AddInput("SceneData", m_UBSScene);
				m_GeometryPass->AddInput("VisiblePointLightIndicesBuffer", m_SBSVisiblePointLightIndicesBuffer);
				m_GeometryPass->AddInput("VisibleSpotLightIndicesBuffer", m_SBSVisibleSpotLightIndicesBuffer);

				m_GeometryPass->AddInput("RendererData", m_UBSRendererData);

				///< Some resources that are scene specific cannot be set before
				///< SceneRenderer::BeginScene. As such these should be placeholders that
				///< when invalidated are replaced with real resources
				m_GeometryPass->AddInput("u_EnvRadianceTex", Renderer::GetBlackCubeTexture());
				m_GeometryPass->AddInput("u_EnvIrradianceTex", Renderer::GetBlackCubeTexture());

				///< Set 3
				m_GeometryPass->AddInput("u_BRDFLUTTexture", Renderer::GetBRDFLutTexture());

				///< Some resources are the results of previous passes. This will enforce
				///< a layout transition when RenderPass::Begin() is called. GetOutput(0)
				///< refers to the first output of the pass - since the pass is depth-only,
				///< this will be the depth image
				m_GeometryPass->AddInput("u_ShadowMapTexture", m_DirectionalShadowMapPass[0]->GetOutput(0));
				m_GeometryPass->AddInput("u_SpotShadowTexture", m_SpotShadowPass->GetOutput(0));

				///< Note: outputs are automatically set by framebuffer

				///< Bake will create descriptor sets and ensure everything is ready for rendering
				///< If resources (eg. storage buffers/images) resize, passes need to be invalidated
				///< so we can re-create proper descriptors to the newly created replacement resources
				SEDX_CORE_VERIFY(m_GeometryPass->Validate());
				m_GeometryPass->Bake();
			}

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            /// Animated Geometry Pass
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			/*
			{
				RenderSpec spec;
				spec.debugName = "GeometryPass-Animated";
				spec.Pipeline = m_GeometryPipelineAnim;
                m_GeometryAnimPass = CreateRef<RenderPass>(spec);

				m_GeometryAnimPass->AddInput("Camera", m_UBSCamera);
				m_GeometryAnimPass->AddInput("SpotShadowData", m_UBSSpotShadowData);
				m_GeometryAnimPass->AddInput("ShadowData", m_UBSShadow);
				m_GeometryAnimPass->AddInput("PointLightData", m_UBSPointLights);
				m_GeometryAnimPass->AddInput("SpotLightData", m_UBSSpotLights);
				m_GeometryAnimPass->AddInput("SceneData", m_UBSScene);
				m_GeometryAnimPass->AddInput("VisiblePointLightIndicesBuffer", m_SBSVisiblePointLightIndicesBuffer);
				m_GeometryAnimPass->AddInput("VisibleSpotLightIndicesBuffer", m_SBSVisibleSpotLightIndicesBuffer);
				m_GeometryAnimPass->AddInput("BoneTransforms", m_SBSBoneTransforms);
				m_GeometryAnimPass->AddInput("RendererData", m_UBSRendererData);
				m_GeometryAnimPass->AddInput("u_EnvRadianceTex", Renderer::GetBlackCubeTexture());
				m_GeometryAnimPass->AddInput("u_EnvIrradianceTex", Renderer::GetBlackCubeTexture());

				///< Set 3
				m_GeometryAnimPass->AddInput("u_BRDFLUTTexture", Renderer::GetBRDFLutTexture());
				m_GeometryAnimPass->AddInput("u_ShadowMapTexture", m_DirectionalShadowMapPass[0]->GetOutput(0));
				m_GeometryAnimPass->AddInput("u_SpotShadowTexture", m_SpotShadowPass->GetOutput(0));

				SEDX_CORE_VERIFY(m_GeometryAnimPass->Validate());
				m_GeometryAnimPass->Bake();
			}
			#1#

            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            /// Light Culling Pass
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			{
				ComputePassSpecification spec;
				spec.debugName = "LightCulling";
				spec.pipeline = m_LightCullingPipeline;
                m_LightCullingPass = CreateRef<ComputePass>(spec);

				m_LightCullingPass->AddInput("Camera", m_UBSCamera);
				m_LightCullingPass->AddInput("ScreenData", m_UBSScreenData);
				m_LightCullingPass->AddInput("PointLightData", m_UBSPointLights);
				m_LightCullingPass->AddInput("SpotLightData", m_UBSSpotLights);
				m_LightCullingPass->AddInput("VisiblePointLightIndicesBuffer", m_SBSVisiblePointLightIndicesBuffer);
				m_LightCullingPass->AddInput("VisibleSpotLightIndicesBuffer", m_SBSVisibleSpotLightIndicesBuffer);
				m_LightCullingPass->AddInput("u_DepthMap", m_PreDepthPass->GetDepthOutput());
				SEDX_CORE_VERIFY(m_LightCullingPass->Validate());
				m_LightCullingPass->Bake();
			}

#if 0
			///< Render example

			///< BeginRenderPass needs to do the following:
			///< - insert layout transitions for input resources that are required
			///< - call vkCmdBeginRenderPass
			///< - bind pipeline - one render pass is only allowed one pipeline, and
			///<   the actual render pass should be able to contain it
			///< - bind descriptor sets that are not per-draw-call, meaning Sets 1-3
			///<     - this makes sense because they are "global" for entire render pass,
			///<       so these can easily be bound up-front

			Renderer::BeginRenderPass(m_CommandBuffer, shadowMapRenderPass);

			///< Render functions (which issue draw calls) should no longer be concerned with
			///< binding the pipeline or descriptor sets 1-3, they only:
			///< - bind vertex/index buffers
			///< - bind descriptor set 0, which contains material resources
			///< - push constants
			///< - actual draw call

			Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, ...);
			Renderer::EndRenderPass(m_CommandBuffer);

			Renderer::BeginRenderPass(m_CommandBuffer, m_GeometryPass);
			Renderer::RenderStaticMesh(m_CommandBuffer, ...);
			Renderer::EndRenderPass(m_CommandBuffer);
#endif
		}

#if 0
		///< Deinterleaving
		{
			ImageSpecification imageSpec;
			imageSpec.format = VkFormat::VK_FORMAT_R32_SFLOAT;
			imageSpec.layers = 16;
			imageSpec.usage = ImageUsage::Attachment;
			imageSpec.debugName = "Deinterleaved";
			Ref<Image2D> image = CreateRef<Image2D>(imageSpec);
			image->Invalidate();
			image->CreatePerLayerImageViews();

			FramebufferSpecification deinterleavingFramebufferSpec;
			deinterleavingFramebufferSpec.attachments.Attachments = { 8, FramebufferTextureSpecification{ VkFormat::VK_FORMAT_R32_SFLOAT } };
			deinterleavingFramebufferSpec.clearColor = { 1.0f, 0.0f, 0.0f, 1.0f };
			deinterleavingFramebufferSpec.debugName = "Deinterleaving";
			deinterleavingFramebufferSpec.existingImage = image;

			Ref<Shader> shader = Renderer::GetShaderLibrary()->Get("Deinterleaving");
			PipelineData pipelineSpec;
			pipelineSpec.debugName = "Deinterleaving";
			pipelineSpec.DepthWrite = false;
			pipelineSpec.DepthTest = false;

			pipelineSpec.Shader = shader;
			pipelineSpec.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};

			// 2 frame buffers, 2 render passes .. 8 attachments each
			for (int rp = 0; rp < 2; rp++)
			{
				deinterleavingFramebufferSpec.existingImageLayers.clear();
				for (int layer = 0; layer < 8; layer++)
					deinterleavingFramebufferSpec.existingImageLayers.emplace_back(rp * 8 + layer);

				pipelineSpec.dstFramebuffer = CreateRef<Framebuffer>(deinterleavingFramebufferSpec);
				m_DeinterleavingPipelines[rp] = CreateRef<Pipeline>(pipelineSpec);

				RenderSpec spec;
				spec.debugName = "Deinterleaving";
				spec.Pipeline = m_DeinterleavingPipelines[rp];
				m_DeinterleavingPass[rp] = CreateRef<RenderPass>(spec);

				m_DeinterleavingPass[rp]->AddInput("u_Depth", m_PreDepthPass->GetDepthOutput());
				m_DeinterleavingPass[rp]->AddInput("Camera", m_UBSCamera);
				m_DeinterleavingPass[rp]->AddInput("ScreenData", m_UBSScreenData);
				SEDX_CORE_VERIFY(m_DeinterleavingPass[rp]->Validate());
				m_DeinterleavingPass[rp]->Bake();
			}
			m_DeinterleavingMaterial = Material::Create(pipelineSpec.Shader, pipelineSpec.debugName);
		}
#endif

		///< Hierarchical Z buffer
		{
			TextureSpecification spec;
			spec.format = VkFormat::VK_FORMAT_R32_SFLOAT;
			spec.width = 1;
			spec.height = 1;
			spec.samplerWrap = SamplerWrap::Clamp;
			spec.samplerFilter = SamplerFilter::Nearest;
			spec.debugName = "HierarchicalZ";

			m_HierarchicalDepthTexture.Texture = CreateRef<Texture2D>(spec);

			Ref<Shader> shader = Renderer::GetShaderLibrary()->Get("HZB");

			ComputePassSpecification hdPassSpec;
			hdPassSpec.debugName = "HierarchicalDepth";
            hdPassSpec.pipeline = CreateRef<ComputePipeline>(shader);
            m_HierarchicalDepthPass = CreateRef<ComputePass>(hdPassSpec);

			SEDX_CORE_VERIFY(m_HierarchicalDepthPass->Validate());
			m_HierarchicalDepthPass->Bake();
		}

		
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Screem Space Reflection (SSR) Composite Pass
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		{
			ImageSpecification imageSpec;
			imageSpec.format = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT;
			imageSpec.usage = ImageUsage::Storage;
			imageSpec.debugName = "SSR";
            m_SSRImage = CreateRef<Image2D>(imageSpec);
			m_SSRImage->Invalidate();

			PipelineData pipelineSpecification;
			pipelineSpecification.layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" },
			};
			pipelineSpecification.backfaceCulling = false;
			pipelineSpecification.depthTest = false;
			pipelineSpecification.depthWrite = false;
			pipelineSpecification.debugName = "SSR-Composite";
			auto shader = Renderer::GetShaderLibrary()->Get("SSR-Composite");
			pipelineSpecification.shader = shader;

			FramebufferSpecification framebufferSpec;
			framebufferSpec.width = m_Specification.ViewportWidth;
			framebufferSpec.height = m_Specification.ViewportHeight;
			framebufferSpec.clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			framebufferSpec.attachments.Attachments.emplace_back(VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT);
			framebufferSpec.existingImages[0] = m_GeometryPass->GetOutput(0);
			framebufferSpec.debugName = "SSR-Composite";
			framebufferSpec.blendMode = FramebufferBlendMode::SrcAlphaOneMinusSrcAlpha;
			framebufferSpec.clearColorOnLoad = false;
            pipelineSpecification.dstFramebuffer = CreateRef<Framebuffer>(framebufferSpec);

			RenderSpec renderPassSpec;
			renderPassSpec.debugName = "SSR-Composite";
            renderPassSpec.Pipeline = CreateRef<Pipeline>(pipelineSpecification);
            m_SSRCompositePass = CreateRef<RenderPass>(renderPassSpec);
			m_SSRCompositePass->AddInput("u_SSR", m_SSRImage);
			SEDX_CORE_VERIFY(m_SSRCompositePass->Validate());
			m_SSRCompositePass->Bake();
		}
		
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Pre-Integration Pass
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		{
			TextureSpecification spec;
			spec.format = VkFormat::VK_FORMAT_R8_UNORM;
			spec.width = 1;
			spec.height = 1;
			spec.debugName = "Pre-Integration";

			m_PreIntegrationVisibilityTexture.Texture = CreateRef<Texture2D>(spec);

			Ref<Shader> shader = Renderer::GetShaderLibrary()->Get("Pre-Integration");
			ComputePassSpecification passSpec;
			passSpec.debugName = "Pre-Integration";
            passSpec.pipeline = CreateRef<ComputePipeline>(shader);
            m_PreIntegrationPass = CreateRef<ComputePass>(passSpec);
		}

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Pre-convolution Compute Pass
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		{
			TextureSpecification spec;
			spec.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
			spec.width = 1;
			spec.height = 1;
			spec.samplerWrap = SamplerWrap::Clamp;
			spec.debugName = "Pre-Convoluted";
			spec.storage = true;
            m_PreConvolutedTexture.Texture = CreateRef<Texture2D>(spec);

			Ref<Shader> shader = Renderer::GetShaderLibrary()->Get("Pre-Convolution");
			ComputePassSpecification passSpec;
			passSpec.debugName = "Pre-Integration";
            passSpec.pipeline = CreateRef<ComputePipeline>(shader);
            m_PreConvolutionComputePass = CreateRef<ComputePass>(passSpec);
			SEDX_CORE_VERIFY(m_PreConvolutionComputePass->Validate());
			m_PreConvolutionComputePass->Bake();
		}

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Edge Detection Pass
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		if (m_Specification.EnableEdgeOutlineEffect)
		{
			FramebufferSpecification compFramebufferSpec;
			compFramebufferSpec.width = m_Specification.ViewportWidth;
			compFramebufferSpec.height = m_Specification.ViewportHeight;
			compFramebufferSpec.debugName = "POST-EdgeDetection";
			compFramebufferSpec.clearColor = { 0.5f, 0.1f, 0.1f, 1.0f };
			compFramebufferSpec.attachments = { VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, RenderContext::GetCurrentDevice()->GetPhysicalDevice()->GetDepthFormat() };
			compFramebufferSpec.transfer = true;

			Ref<Framebuffer> framebuffer = CreateRef<Framebuffer>(compFramebufferSpec);

			PipelineData pipelineSpecification;
			pipelineSpecification.layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpecification.backfaceCulling = false;
			pipelineSpecification.shader = Renderer::GetShaderLibrary()->Get("EdgeDetection");
			pipelineSpecification.dstFramebuffer = framebuffer;
			pipelineSpecification.debugName = compFramebufferSpec.debugName;
			pipelineSpecification.depthWrite = false;

			RenderSpec renderPassSpec;
			renderPassSpec.debugName = compFramebufferSpec.debugName;
            renderPassSpec.Pipeline = CreateRef<Pipeline>(pipelineSpecification);
            m_EdgeDetectionPass = CreateRef<RenderPass>(renderPassSpec);
			m_EdgeDetectionPass->AddInput("u_ViewNormalsTexture", m_GeometryPass->GetOutput(1));
			m_EdgeDetectionPass->AddInput("u_DepthTexture", m_PreDepthPass->GetDepthOutput());
			m_EdgeDetectionPass->AddInput("Camera", m_UBSCamera);
			SEDX_CORE_VERIFY(m_EdgeDetectionPass->Validate());
			m_EdgeDetectionPass->Bake();
		}

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Scene Composite Pass
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		{
			FramebufferSpecification compFramebufferSpec;
			compFramebufferSpec.width = m_Specification.ViewportWidth;
			compFramebufferSpec.height = m_Specification.ViewportHeight;
			compFramebufferSpec.debugName = "SceneComposite";
			compFramebufferSpec.clearColor = { 0.5f, 0.1f, 0.1f, 1.0f };
			compFramebufferSpec.attachments = { VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, RenderContext::GetCurrentDevice()->GetPhysicalDevice()->GetDepthFormat() };
			compFramebufferSpec.transfer = true;

			Ref<Framebuffer> framebuffer = CreateRef<Framebuffer>(compFramebufferSpec);

			PipelineData pipelineSpecification;
			pipelineSpecification.layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpecification.backfaceCulling = false;
			pipelineSpecification.shader = Renderer::GetShaderLibrary()->Get("SceneComposite");
			pipelineSpecification.dstFramebuffer = framebuffer;
			pipelineSpecification.debugName = "SceneComposite";
			pipelineSpecification.depthWrite = false;
			pipelineSpecification.depthTest = false;

			RenderSpec renderPassSpec;
			renderPassSpec.debugName = "SceneComposite";
            renderPassSpec.Pipeline = CreateRef<Pipeline>(pipelineSpecification);
            m_CompositePass = CreateRef<RenderPass>(renderPassSpec);
			m_CompositePass->AddInput("u_Texture", m_GeometryPass->GetOutput(0));
			m_CompositePass->AddInput("u_BloomTexture", m_BloomComputeTextures[2].Texture);
			m_CompositePass->AddInput("u_BloomDirtTexture", m_BloomDirtTexture);
			m_CompositePass->AddInput("u_DepthTexture", m_PreDepthPass->GetDepthOutput());
			m_CompositePass->AddInput("u_TransparentDepthTexture", m_PreDepthTransparentPass->GetDepthOutput());

			if (m_Specification.EnableEdgeOutlineEffect)
				m_CompositePass->AddInput("u_EdgeTexture", m_EdgeDetectionPass->GetOutput(0));

			m_CompositePass->AddInput("Camera", m_UBSCamera);

			SEDX_CORE_VERIFY(m_CompositePass->Validate());
			m_CompositePass->Bake();
		}

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Depth Of Feild (DOF) Pass 
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		{
			FramebufferSpecification compFramebufferSpec;
			compFramebufferSpec.width = m_Specification.ViewportWidth;
			compFramebufferSpec.height = m_Specification.ViewportHeight;
			compFramebufferSpec.debugName = "POST-DepthOfField";
			compFramebufferSpec.clearColor = { 0.5f, 0.1f, 0.1f, 1.0f };
			compFramebufferSpec.attachments = { VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, RenderContext::GetCurrentDevice()->GetPhysicalDevice()->GetDepthFormat() };
			compFramebufferSpec.transfer = true;

			Ref<Framebuffer> framebuffer = CreateRef<Framebuffer>(compFramebufferSpec);

			PipelineData pipelineSpecification;
			pipelineSpecification.layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpecification.backfaceCulling = false;
			pipelineSpecification.shader = Renderer::GetShaderLibrary()->Get("DOF");
			pipelineSpecification.debugName = compFramebufferSpec.debugName;
			pipelineSpecification.depthWrite = false;
			pipelineSpecification.dstFramebuffer = framebuffer;
			m_DOFMaterial = Material::Create(pipelineSpecification.shader, pipelineSpecification.debugName);

			RenderSpec renderPassSpec;
			renderPassSpec.debugName = "POST-DOF";
            renderPassSpec.Pipeline = CreateRef<Pipeline>(pipelineSpecification);
            m_DOFPass = CreateRef<RenderPass>(renderPassSpec);
			m_DOFPass->AddInput("u_Texture", m_CompositePass->GetOutput(0));
			m_DOFPass->AddInput("u_DepthTexture", m_PreDepthPass->GetDepthOutput());
			m_DOFPass->AddInput("Camera", m_UBSCamera);
			SEDX_CORE_VERIFY(m_DOFPass->Validate());
			m_DOFPass->Bake();
		}

		FramebufferSpecification fbSpec;
		fbSpec.width = m_Specification.ViewportWidth;
		fbSpec.height = m_Specification.ViewportHeight;
		fbSpec.attachments = { VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT };
		fbSpec.clearColorOnLoad = false;
		fbSpec.clearDepthOnLoad = false;
		fbSpec.existingImages[0] = m_CompositePass->GetOutput(0);
		fbSpec.existingImages[1] = m_PreDepthPass->GetDepthOutput();
        m_CompositingFramebuffer = CreateRef<Framebuffer>(fbSpec);

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Wireframe Pass
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		{
			PipelineData pipelineSpecification;
			pipelineSpecification.debugName = "Wireframe";
			pipelineSpecification.dstFramebuffer = m_CompositingFramebuffer;
			pipelineSpecification.shader = Renderer::GetShaderLibrary()->Get("Wireframe");
			pipelineSpecification.backfaceCulling = false;
			pipelineSpecification.wireframe = true;
			pipelineSpecification.depthTest = true;
			pipelineSpecification.lineWidth = 2.0f;
			pipelineSpecification.layout = vertexLayout;
			pipelineSpecification.instanceLayout = instanceLayout;

			RenderSpec renderPassSpec;
			renderPassSpec.debugName = "Geometry-Wireframe";
            renderPassSpec.Pipeline = CreateRef<Pipeline>(pipelineSpecification);
            m_GeometryWireframePass = CreateRef<RenderPass>(renderPassSpec);
			m_GeometryWireframePass->AddInput("Camera", m_UBSCamera);
			SEDX_CORE_VERIFY(m_GeometryWireframePass->Validate());
			m_GeometryWireframePass->Bake();

			pipelineSpecification.depthTest = false;
			pipelineSpecification.debugName = "Wireframe-OnTop";
            renderPassSpec.Pipeline = CreateRef<Pipeline>(pipelineSpecification);
			renderPassSpec.debugName = pipelineSpecification.debugName;
            m_GeometryWireframeOnTopPass = CreateRef<RenderPass>(renderPassSpec);
			m_GeometryWireframeOnTopPass->AddInput("Camera", m_UBSCamera);
			SEDX_CORE_VERIFY(m_GeometryWireframeOnTopPass->Validate());
			m_GeometryWireframeOnTopPass->Bake();

			pipelineSpecification.depthTest = true;
			pipelineSpecification.debugName = "Wireframe-Anim";
			pipelineSpecification.shader = Renderer::GetShaderLibrary()->Get("Wireframe_Anim");
			pipelineSpecification.boneInfluenceLayout = boneInfluenceLayout;
			renderPassSpec.Pipeline = CreateRef<Pipeline>(pipelineSpecification); ///< Note: same framebuffer and renderpass as m_GeometryWireframePipeline
			renderPassSpec.debugName = pipelineSpecification.debugName;
            m_GeometryWireframeAnimPass = CreateRef<RenderPass>(renderPassSpec);
			m_GeometryWireframeAnimPass->AddInput("Camera", m_UBSCamera);
			m_GeometryWireframeAnimPass->AddInput("BoneTransforms", m_SBSBoneTransforms);
			SEDX_CORE_VERIFY(m_GeometryWireframeAnimPass->Validate());
			m_GeometryWireframeAnimPass->Bake();

			pipelineSpecification.depthTest = false;
			pipelineSpecification.debugName = "Wireframe-Anim-OnTop";
            renderPassSpec.Pipeline = CreateRef<Pipeline>(pipelineSpecification);
			m_GeometryWireframeOnTopAnimPass = CreateRef<RenderPass>(renderPassSpec);
			m_GeometryWireframeOnTopAnimPass->AddInput("Camera", m_UBSCamera);
			m_GeometryWireframeOnTopAnimPass->AddInput("BoneTransforms", m_SBSBoneTransforms);
			SEDX_CORE_VERIFY(m_GeometryWireframeOnTopAnimPass->Validate());
			m_GeometryWireframeOnTopAnimPass->Bake();
		}

		///< Read-back Image
		if (false) ///< WIP
		{
			ImageSpecification spec;
			spec.format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
			spec.usage = ImageUsage::HostRead;
			spec.transfer = true;
			spec.debugName = "ReadBack";
            m_ReadBackImage = CreateRef<Image2D>(spec);
		}

		///< Temporary framebuffers for re-use
		{
			FramebufferSpecification framebufferSpec;
			framebufferSpec.width = m_Specification.ViewportWidth;
			framebufferSpec.height = m_Specification.ViewportHeight;
			framebufferSpec.attachments = { VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT };
			framebufferSpec.samples = 1;
			framebufferSpec.clearColor = { 0.5f, 0.1f, 0.1f, 1.0f };
			framebufferSpec.blendMode = FramebufferBlendMode::OneZero;
			framebufferSpec.debugName = "Temporaries";

			for (uint32_t i = 0; i < 2; i++)
                m_TempFramebuffers.emplace_back(CreateRef<Framebuffer>(framebufferSpec));
		}

		///< Jump Flood (outline)
		{
			PipelineData pipelineSpecification;
			pipelineSpecification.debugName = "JumpFlood-Init";
			pipelineSpecification.shader = Renderer::GetShaderLibrary()->Get("JumpFlood_Init");
			pipelineSpecification.dstFramebuffer = m_TempFramebuffers[0];
			pipelineSpecification.layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
            m_JumpFloodInitMaterial = CreateRef<Material>(pipelineSpecification.shader, pipelineSpecification.debugName);

			RenderSpec renderPassSpec;
			renderPassSpec.debugName = "JumpFlood-Init";
            renderPassSpec.Pipeline = CreateRef<Pipeline>(pipelineSpecification);
            m_JumpFloodInitPass = CreateRef<RenderPass>(renderPassSpec);
			m_JumpFloodInitPass->AddInput("u_Texture", m_SelectedGeometryPass->GetOutput(0));
			SEDX_CORE_VERIFY(m_JumpFloodInitPass->Validate());
			m_JumpFloodInitPass->Bake();

			const char* passName[2] = { "EvenPass", "OddPass" };
			for (uint32_t i = 0; i < 2; i++)
			{
				pipelineSpecification.debugName = renderPassSpec.debugName;
				pipelineSpecification.dstFramebuffer = m_TempFramebuffers[(i + 1) % 2];
				pipelineSpecification.shader = Renderer::GetShaderLibrary()->Get("JumpFlood_Pass");

				renderPassSpec.debugName = std::format("JumpFlood-{0}", passName[i]);
                renderPassSpec.Pipeline = CreateRef<Pipeline>(pipelineSpecification);
                m_JumpFloodPass[i] = CreateRef<RenderPass>(renderPassSpec);
				m_JumpFloodPass[i]->AddInput("u_Texture", m_TempFramebuffers[i]->GetImage());
				SEDX_CORE_VERIFY(m_JumpFloodPass[i]->Validate());
				m_JumpFloodPass[i]->Bake();

				m_JumpFloodPassMaterial[i] = CreateRef<Material>(pipelineSpecification.shader, pipelineSpecification.debugName);
			}

			///< Outline compositing
			if (m_Specification.JumpFloodPass)
			{
				FramebufferSpecification fbJumpSpec;
                fbJumpSpec.width = m_Specification.ViewportWidth;
				fbJumpSpec.height = m_Specification.ViewportHeight;
				fbJumpSpec.attachments = { VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT};
				fbJumpSpec.existingImages[0] = m_CompositePass->GetOutput(0);
				fbJumpSpec.clearColorOnLoad = false;
				pipelineSpecification.dstFramebuffer = CreateRef<Framebuffer>(fbJumpSpec); // TODO: move this and skybox FB to be central, can use same
				pipelineSpecification.debugName = "JumpFlood-Composite";
				pipelineSpecification.shader = Renderer::GetShaderLibrary()->Get("JumpFlood_Composite");
				pipelineSpecification.depthTest = false;
                renderPassSpec.Pipeline = CreateRef<Pipeline>(pipelineSpecification);
                m_JumpFloodCompositePass = CreateRef<RenderPass>(renderPassSpec);
				m_JumpFloodCompositePass->AddInput("u_Texture", m_TempFramebuffers[1]->GetImage());
				SEDX_CORE_VERIFY(m_JumpFloodCompositePass->Validate());
				m_JumpFloodCompositePass->Bake();

				m_JumpFloodCompositeMaterial = CreateRef<Material>(pipelineSpecification.shader, pipelineSpecification.debugName);
			}
		}

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Ground Grid/ Floor Grid Pass
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		{
			PipelineData pipelineSpec;
			pipelineSpec.debugName = "Grid";
			pipelineSpec.shader = Renderer::GetShaderLibrary()->Get("Grid");
			pipelineSpec.backfaceCulling = false;
			pipelineSpec.layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpec.dstFramebuffer = m_CompositingFramebuffer;

			RenderSpec renderPassSpec;
			renderPassSpec.debugName = "Grid";
            renderPassSpec.Pipeline = CreateRef<Pipeline>(pipelineSpec);
            m_GridRenderPass = CreateRef<RenderPass>(renderPassSpec);
			m_GridRenderPass->AddInput("Camera", m_UBSCamera);
			SEDX_CORE_VERIFY(m_GridRenderPass->Validate());
			m_GridRenderPass->Bake();

            constexpr float gridScale = 16.025f;
            constexpr float gridSize = 0.025f;
            m_GridMaterial = CreateRef<Material>(pipelineSpec.shader, pipelineSpec.debugName);
			m_GridMaterial->Set("u_Settings.Scale", gridScale);
			m_GridMaterial->Set("u_Settings.Size", gridSize);
		}


        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Debug Rendering Pass
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        m_SelectedBoneMaterial = CreateRef<Material>(Renderer::GetShaderLibrary()->Get("Wireframe"), "SelectedBone");
		m_SelectedBoneMaterial->Set("u_MaterialUniforms.Color", m_Options.SelectedBoneColor);
        m_BoneMaterial = CreateRef<Material>(Renderer::GetShaderLibrary()->Get("Wireframe"), "Bone");
		m_BoneMaterial->Set("u_MaterialUniforms.Color", m_Options.BoneColor);
		m_BoneMesh = AssetManager::GetAsset<StaticMesh>(Primitives::CreateSphere(0.1f)); // must hold a Ref<Mesh> here, not an asset handle (so that if project is reloaded (and asset manager destroyed) we still hold a ref to the bone mesh, so bone mesh is not destroyed)
		m_BoneMeshSource = AssetManager::GetAsset<MeshSource>(m_BoneMesh->GetMeshSource());
        m_SimpleColliderMaterial = CreateRef<Material>(Renderer::GetShaderLibrary()->Get("Wireframe"), "SimpleCollider");
		m_SimpleColliderMaterial->Set("u_MaterialUniforms.Color", m_Options.SimplePhysicsCollidersColor);
		m_ComplexColliderMaterial = CreateRef<Material>(Renderer::GetShaderLibrary()->Get("Wireframe"), "ComplexCollider");
		m_ComplexColliderMaterial->Set("u_MaterialUniforms.Color", m_Options.ComplexPhysicsCollidersColor);

		m_WireframeMaterial = CreateRef<Material>(Renderer::GetShaderLibrary()->Get("Wireframe"), "Wireframe");
		m_WireframeMaterial->Set("u_MaterialUniforms.Color", Vec4{ 1.0f, 0.5f, 0.0f, 1.0f });
		
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Skybox Pass
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		{
			auto skyboxShader = Renderer::GetShaderLibrary()->Get("Skybox");

			FramebufferSpecification spec;
			spec.width = m_Specification.ViewportWidth;
			spec.height = m_Specification.ViewportHeight;
			spec.attachments = { VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT };
			spec.existingImages[0] = m_GeometryPass->GetOutput(0);
            Ref<Framebuffer> skyboxFB = CreateRef<Framebuffer>(spec);

			PipelineData pipelineSpec;
			pipelineSpec.debugName = "Skybox";
			pipelineSpec.shader = skyboxShader;
			pipelineSpec.depthWrite = false;
			pipelineSpec.depthTest = false;
			pipelineSpec.depthWrite = false;
			pipelineSpec.layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float2, "a_TexCoord" }
			};
			pipelineSpec.dstFramebuffer = skyboxFB;
            m_SkyboxPipeline = CreateRef<Pipeline>(pipelineSpec);
            m_SkyboxMaterial = CreateRef<Material>(pipelineSpec.shader, pipelineSpec.debugName);
			m_SkyboxMaterial->SetFlag(MaterialFlag::DepthTest, false);

			RenderSpec renderPassSpec;
			renderPassSpec.debugName = "Skybox";
			renderPassSpec.Pipeline = m_SkyboxPipeline;
            m_SkyboxPass = CreateRef<RenderPass>(renderPassSpec);
			m_SkyboxPass->AddInput("Camera", m_UBSCamera);
			SEDX_CORE_VERIFY(m_SkyboxPass->Validate());
			m_SkyboxPass->Bake();
		}

		///< TODO: resizeable/flushable
        constexpr size_t TransformBufferCount = 100 * 1024; ///< 102400 transforms (~4.9MB)
		m_SubmeshTransformBuffers.resize(framesInFlight);
		for (uint32_t i = 0; i < framesInFlight; i++)
		{
            m_SubmeshTransformBuffers[i].Buffer = CreateRef<VertexBuffer>(sizeof(TransformVertexData) * TransformBufferCount);
			m_SubmeshTransformBuffers[i].Data = hnew TransformVertexData[TransformBufferCount];
		}

		Renderer::Submit([instance = Ref(this)]() mutable { instance->m_ResourcesCreatedGPU = true; });

		InitOptions();

		////////////////////////////////////////
		/// COMPUTE
		////////////////////////////////////////

		///< GTAO
		{
			{
				ImageSpecification imageSpec;
				if (m_Options.GTAOBentNormals)
					imageSpec.format = VkFormat::VK_FORMAT_R32_UINT;
				else
					imageSpec.format = VkFormat::VK_FORMAT_R8_UINT;
				imageSpec.usage = ImageUsage::Storage;
				imageSpec.layers = 1;
				imageSpec.debugName = "GTAO";
                m_GTAOOutputImage = CreateRef<Image2D>(imageSpec);
				m_GTAOOutputImage->Invalidate();
			}

			///< GTAO-Edges
			{
				ImageSpecification imageSpec;
				imageSpec.format = VkFormat::VK_FORMAT_R8_UNORM;
				imageSpec.usage = ImageUsage::Storage;
				imageSpec.debugName = "GTAO-Edges";
                m_GTAOEdgesOutputImage = CreateRef<Image2D>(imageSpec);
				m_GTAOEdgesOutputImage->Invalidate();
			}

			Ref<Shader> shader = Renderer::GetShaderLibrary()->Get("GTAO");

			ComputePassSpecification spec;
			spec.debugName = "GTAO-ComputePass";
            spec.pipeline = CreateRef<ComputePipeline>(shader);
            m_GTAOComputePass = CreateRef<ComputePass>(spec);
			m_GTAOComputePass->AddInput("u_HiZDepth", m_HierarchicalDepthTexture.Texture);
			m_GTAOComputePass->AddInput("u_HilbertLut", Renderer::GetHilbertLut());
			m_GTAOComputePass->AddInput("u_ViewNormal", m_GeometryPass->GetOutput(1));
			m_GTAOComputePass->AddInput("o_AOwBentNormals", m_GTAOOutputImage);
			m_GTAOComputePass->AddInput("o_Edges", m_GTAOEdgesOutputImage);

			m_GTAOComputePass->AddInput("Camera", m_UBSCamera);
			m_GTAOComputePass->AddInput("ScreenData", m_UBSScreenData);
			SEDX_CORE_VERIFY(m_GTAOComputePass->Validate());
			m_GTAOComputePass->Bake();
		}

		///< GTAO Denoise
		{

			{
				ImageSpecification imageSpec;
				if (m_Options.GTAOBentNormals)
					imageSpec.format = VkFormat::VK_FORMAT_R32_UINT;
				else
					imageSpec.format = VkFormat::VK_FORMAT_R8_UINT;
				imageSpec.usage = ImageUsage::Storage;
				imageSpec.layers = 1;
				imageSpec.debugName = "GTAO-Denoise";
                m_GTAODenoiseImage = CreateRef<Image2D>(imageSpec);
				m_GTAODenoiseImage->Invalidate();

				Ref<Shader> shader = Renderer::GetShaderLibrary()->Get("GTAO-Denoise");
                m_GTAODenoiseMaterial[0] = CreateRef<Material>(shader, "GTAO-Denoise-Ping");
                m_GTAODenoiseMaterial[1] = CreateRef<Material>(shader, "GTAO-Denoise-Pong");

				ComputePassSpecification spec;
				spec.debugName = "GTAO-Denoise";
                spec.pipeline = CreateRef<ComputePipeline>(shader);

				m_GTAODenoisePass[0] = CreateRef<ComputePass>(spec);
				m_GTAODenoisePass[0]->AddInput("u_Edges", m_GTAOEdgesOutputImage);
				m_GTAODenoisePass[0]->AddInput("u_AOTerm", m_GTAOOutputImage);
				m_GTAODenoisePass[0]->AddInput("o_AOTerm", m_GTAODenoiseImage);
				m_GTAODenoisePass[0]->AddInput("ScreenData", m_UBSScreenData);
				SEDX_CORE_VERIFY(m_GTAODenoisePass[0]->Validate());
				m_GTAODenoisePass[0]->Bake();

				m_GTAODenoisePass[1] = CreateRef<ComputePass>(spec);
				m_GTAODenoisePass[1]->AddInput("u_Edges", m_GTAOEdgesOutputImage);
				m_GTAODenoisePass[1]->AddInput("u_AOTerm", m_GTAODenoiseImage);
				m_GTAODenoisePass[1]->AddInput("o_AOTerm", m_GTAOOutputImage);
				m_GTAODenoisePass[1]->AddInput("ScreenData", m_UBSScreenData);
				SEDX_CORE_VERIFY(m_GTAODenoisePass[1]->Validate());
				m_GTAODenoisePass[1]->Bake();
			}

			///< GTAO Composite
			 {
				PipelineData aoCompositePipelineSpec;
				aoCompositePipelineSpec.debugName = "AO-Composite";
				FramebufferSpecification framebufferSpec;
				framebufferSpec.width = m_Specification.ViewportWidth;
				framebufferSpec.height = m_Specification.ViewportHeight;
				framebufferSpec.debugName = "AO-Composite";
				framebufferSpec.attachments = { VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT };
				framebufferSpec.existingImages[0] = m_GeometryPass->GetOutput(0);
				framebufferSpec.blend = true;
				framebufferSpec.clearColorOnLoad = false;
				framebufferSpec.blendMode = FramebufferBlendMode::Zero_SrcColor;

				aoCompositePipelineSpec.dstFramebuffer = CreateRef<Framebuffer>(framebufferSpec);
				aoCompositePipelineSpec.depthTest = false;
				aoCompositePipelineSpec.layout = {
					{ ShaderDataType::Float3, "a_Position" },
					{ ShaderDataType::Float2, "a_TexCoord" },
				};
				aoCompositePipelineSpec.shader = Renderer::GetShaderLibrary()->Get("AO-Composite");

				///< Create RenderPass
				RenderSpec renderPassSpec;
				renderPassSpec.debugName = "AO-Composite";
                renderPassSpec.Pipeline = CreateRef<Pipeline>(aoCompositePipelineSpec);
                m_AOCompositePass = CreateRef<RenderPass>(renderPassSpec);
				m_AOCompositePass->AddInput("u_GTAOTex", m_GTAOOutputImage);
				SEDX_CORE_VERIFY(m_AOCompositePass->Validate());
				m_AOCompositePass->Bake();

				m_AOCompositeMaterial = CreateRef<Material>(aoCompositePipelineSpec.shader, "GTAO-Composite");
			}

		}

		m_GTAOFinalImage = m_Options.GTAODenoisePasses && m_Options.GTAODenoisePasses % 2 != 0 ? m_GTAODenoiseImage : m_GTAOOutputImage;

		///< SSR
		{
			Ref<Shader> shader = Renderer::GetShaderLibrary()->Get("SSR");

			ComputePassSpecification spec;
			spec.debugName = "SSR-Compute";
            spec.pipeline = CreateRef<ComputePipeline>(shader);
            m_SSRPass = CreateRef<ComputePass>(spec);
			m_SSRPass->AddInput("outColor", m_SSRImage);
			m_SSRPass->AddInput("u_InputColor", m_PreConvolutedTexture.Texture);
			m_SSRPass->AddInput("u_VisibilityBuffer", m_PreIntegrationVisibilityTexture.Texture);
			m_SSRPass->AddInput("u_HiZBuffer", m_HierarchicalDepthTexture.Texture);
			m_SSRPass->AddInput("u_Normal", m_GeometryPass->GetOutput(1));
			m_SSRPass->AddInput("u_MetalnessRoughness", m_GeometryPass->GetOutput(2));
			m_SSRPass->AddInput("u_GTAOTex", m_GTAOFinalImage);
			m_SSRPass->AddInput("Camera", m_UBSCamera);
			m_SSRPass->AddInput("ScreenData", m_UBSScreenData);
            ///< TODO: HBAO texture as well maybe
			SEDX_CORE_VERIFY(m_SSRPass->Validate());
			m_SSRPass->Bake();
		}

	}

	void SceneRenderer::Shutdown() const
    {
		hdelete[] m_BoneTransformsData;
		for (const auto&[Buffer, Data] : m_SubmeshTransformBuffers)
			hdelete[] Data;
	}


	void SceneRenderer::InitOptions()
	{
        ///<TODO: Deserialization?
		if (m_Options.EnableGTAO)
			*(int*)&m_Options.ReflectionOcclusionMethod |= (int)ShaderDef::AOMethod::GTAO;

		///< OVERRIDE
		m_Options.ReflectionOcclusionMethod = ShaderDef::AOMethod::None;

		///< Special macros are strictly starting with "__SEDX_"
		Renderer::SetGlobalMacroInShaders("__SEDX_REFLECTION_OCCLUSION_METHOD", std::format("{}", (int)m_Options.ReflectionOcclusionMethod));
        ///<Renderer::SetGlobalMacroInShaders("__SEDX_GTAO_COMPUTE_BENT_NORMALS", std::format("{}", (int)m_Options.GTAOBentNormals));
	}

	void SceneRenderer::InsertGPUPerfMarker(Ref<CommandBuffer> renderCommandBuffer, const std::string& label, const Vec4& markerColor)
	{
		Renderer::Submit([=]
		{
			Renderer::RT_InsertGPUPerfMarker(renderCommandBuffer, label, markerColor);
		});
	}

	void SceneRenderer::BeginGPUPerfMarker(Ref<CommandBuffer> renderCommandBuffer, const std::string& label, const Vec4& markerColor)
	{
		Renderer::Submit([=]
		{
			Renderer::RT_BeginGPUPerfMarker(renderCommandBuffer, label, markerColor);
		});
	}

	void SceneRenderer::EndGPUPerfMarker(const Ref<CommandBuffer> &renderCommandBuffer)
	{
		Renderer::Submit([=]
		{
			Renderer::RT_EndGPUPerfMarker(renderCommandBuffer);
		});
	}

	void SceneRenderer::SetScene(const Ref<Scene> &scene)
	{
		SEDX_CORE_ASSERT(!m_Active, "Can't change scenes while rendering");
		m_Scene = scene;
	}

	void SceneRenderer::SetViewportSize(uint32_t width, uint32_t height)
	{
		width = (uint32_t)(width * m_Specification.Tiering.RendererScale);
		height = (uint32_t)(height * m_Specification.Tiering.RendererScale);

		if (m_ViewportWidth != width || m_ViewportHeight != height)
		{
			m_ViewportWidth = width;
			m_ViewportHeight = height;
			m_InvViewportWidth = 1.f / (float)width;
			m_InvViewportHeight = 1.f / (float)height;
			m_NeedsResize = true;
		}
	}

	///< Some other settings are directly set in gui
	void SceneRenderer::UpdateGTAOData()
	{
		CBGTAOData& gtaoData = GTAODataCB;
		gtaoData.NDCToViewMul_x_PixelSize = { CameraDataUB.NDCToViewMul * (gtaoData.HalfRes ? m_ScreenDataUB.InvHalfResolution : m_ScreenDataUB.InvFullResolution) };
		gtaoData.BUVFactor = m_SSROptions.BUvFactor;
		gtaoData.ShadowTolerance = m_Options.AOShadowTolerance;
	}

	void SceneRenderer::BeginScene(const SceneRendererCamera& camera)
	{
		SEDX_PROFILE_FUNC();
		SEDX_CORE_ASSERT(m_Scene);
		SEDX_CORE_ASSERT(!m_Active);
		m_Active = true;

		if (m_ResourcesCreatedGPU)
			m_ResourcesCreated = true;

		if (!m_ResourcesCreated)
			return;

		m_GTAOFinalImage = m_Options.GTAODenoisePasses && m_Options.GTAODenoisePasses % 2 != 0 ? m_GTAODenoiseImage : m_GTAOOutputImage;
        ///< TODO: enable if shader uses this: m_SSRPass->AddInput("u_GTAOTex", m_GTAOFinalImage);

		m_SceneData.SceneCamera = camera;
		m_SceneData.SceneEnvironment = m_Scene->m_Environment;
		m_SceneData.SceneEnvironmentIntensity = m_Scene->m_EnvironmentIntensity;
		m_SceneData.ActiveLight = m_Scene->m_Light;
		m_SceneData.SceneLightEnvironment = m_Scene->m_LightEnvironment;
		m_SceneData.SkyboxLod = m_Scene->m_SkyboxLod;

		m_GeometryPass->AddInput("u_EnvRadianceTex", m_SceneData.SceneEnvironment->RadianceMap);
		m_GeometryPass->AddInput("u_EnvIrradianceTex", m_SceneData.SceneEnvironment->IrradianceMap);

		m_GeometryAnimPass->AddInput("u_EnvRadianceTex", m_SceneData.SceneEnvironment->RadianceMap);
		m_GeometryAnimPass->AddInput("u_EnvIrradianceTex", m_SceneData.SceneEnvironment->IrradianceMap);

		if (m_NeedsResize)
		{
			m_NeedsResize = false;

			const UVec2 viewportSize = { m_ViewportWidth, m_ViewportHeight };

			m_ScreenSpaceProjectionMatrix = Ortho(0.0f, (float)m_ViewportWidth, 0.0f, (float)m_ViewportHeight, -1.0f, 1.0f);

			m_ScreenDataUB.FullResolution = { m_ViewportWidth, m_ViewportHeight };
			m_ScreenDataUB.InvFullResolution = { m_InvViewportWidth,  m_InvViewportHeight };
			m_ScreenDataUB.HalfResolution = iVec2{ m_ViewportWidth,  m_ViewportHeight } / 2;
			m_ScreenDataUB.InvHalfResolution = { m_InvViewportWidth * 2.0f,  m_InvViewportHeight * 2.0f };

			///< Both Pre-depth and geometry framebuffers need to be resized first.
			///< Note the _Anim variants of these pipelines share the same framebuffer
			m_PreDepthPass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);
			m_PreDepthAnimPass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);
			m_PreDepthTransparentPass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);
			m_GeometryPass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);
			m_GeometryAnimPass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);
			m_SkyboxPass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);
			m_SelectedGeometryPass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);
			m_SelectedGeometryAnimPass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);
			m_GeometryPassColorAttachmentImage->Resize({ m_ViewportWidth, m_ViewportHeight });

			///< Dependent on Geometry
			m_SSRCompositePass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);

			m_PreIntegrationVisibilityTexture.Texture->Resize(m_ViewportWidth, m_ViewportHeight);
            ///< TODO: m_AOCompositePipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->Resize(m_ViewportWidth, m_ViewportHeight); //Only resize after geometry framebuffer
			m_AOCompositePass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);
			m_CompositePass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);

			m_GridRenderPass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);

			if (m_JumpFloodCompositePass)
				m_JumpFloodCompositePass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);

			if (m_DOFPass)
				m_DOFPass->GetTargetFramebuffer()->Resize(m_ViewportWidth, m_ViewportHeight);

			if (m_ReadBackImage)
				m_ReadBackImage->Resize({ m_ViewportWidth, m_ViewportHeight });

			///< HZB
			{
                ///< HZB size must be power of 2's
				const Vec2 log2Size = Vec2(std::log2((float)viewportSize.x), std::log2((float)viewportSize.y));
				const UVec2 numMips = UVec2((uint32_t)std::ceil(log2Size.x), (uint32_t)std::ceil(log2Size.y));
				m_SSROptions.NumDepthMips = Math::Max(numMips.x, numMips.y);

				const UVec2 hzbSize = BIT(numMips);
				m_HierarchicalDepthTexture.Texture->Resize(hzbSize.x, hzbSize.y);

				const Vec2 hzbUVFactor = { (Vec2)viewportSize / (Vec2)hzbSize };
				m_SSROptions.BUvFactor = hzbUVFactor;

				///< Image Views (per-mip)
				ImageViewData imageViewSpec;
				uint32_t mipCount = m_HierarchicalDepthTexture.Texture->GetMipLevelCount();
				m_HierarchicalDepthTexture.ImageViews.resize(mipCount);
				for (uint32_t mip = 0; mip < mipCount; mip++)
				{
					imageViewSpec.debugName = std::format("HierarchicalDepthTexture-{}", mip);
					imageViewSpec.image = m_HierarchicalDepthTexture.Texture->GetImage();
					imageViewSpec.mip = mip;
                    m_HierarchicalDepthTexture.ImageViews[mip] = CreateRef<ImageView>(imageViewSpec);
				}

				CreateHZBPassMaterials();
			}

			///< Pre-Integration
			{
                ///< Image Views (per-mip)
				ImageViewData imageViewSpec;
				uint32_t mipCount = m_PreIntegrationVisibilityTexture.Texture->GetMipLevelCount();
				m_PreIntegrationVisibilityTexture.ImageViews.resize(mipCount);
				for (uint32_t mip = 0; mip < mipCount - 1; mip++)
				{
					imageViewSpec.debugName = std::format("PreIntegrationVisibilityTexture-{}", mip);
					imageViewSpec.image = m_PreIntegrationVisibilityTexture.Texture->GetImage();
					imageViewSpec.mip = mip + 1; ///< Start from mip 1 not 0
                    m_PreIntegrationVisibilityTexture.ImageViews[mip] = CreateRef<ImageView>(imageViewSpec);
				}

				CreatePreIntegrationPassMaterials();
			}

			///< Light culling
			{
				constexpr uint32_t TILE_SIZE = 16u;
				UVec2 size = viewportSize;
				size += TILE_SIZE - viewportSize % TILE_SIZE;
				m_LightCullingWorkGroups = { size / TILE_SIZE, 1 };
				RendererDataUB.TilesCountX = m_LightCullingWorkGroups.x;

				m_SBSVisiblePointLightIndicesBuffer->Resize(m_LightCullingWorkGroups.x * m_LightCullingWorkGroups.y * 4 * 1024);
				m_SBSVisibleSpotLightIndicesBuffer->Resize(m_LightCullingWorkGroups.x * m_LightCullingWorkGroups.y * 4 * 1024);
			}

			///< GTAO
			{
				UVec2 gtaoSize = GTAODataCB.HalfRes ? (viewportSize + 1u) / 2u : viewportSize;
				UVec2 denoiseSize = gtaoSize;
				const VkFormat gtaoImageFormat = m_Options.GTAOBentNormals ? VkFormat::VK_FORMAT_R32_UINT : VkFormat::VK_FORMAT_R8_UINT;
				m_GTAOOutputImage->GetSpecification().format = gtaoImageFormat;
				m_GTAODenoiseImage->GetSpecification().format = gtaoImageFormat;

				m_GTAOOutputImage->Resize(gtaoSize);
				m_GTAODenoiseImage->Resize(gtaoSize);
				m_GTAOEdgesOutputImage->Resize(gtaoSize);

				constexpr uint32_t WORK_GROUP_SIZE = 16u;
				gtaoSize += WORK_GROUP_SIZE - gtaoSize % WORK_GROUP_SIZE;
				m_GTAOWorkGroups.x = gtaoSize.x / WORK_GROUP_SIZE;
				m_GTAOWorkGroups.y = gtaoSize.y / WORK_GROUP_SIZE;

				constexpr uint32_t DENOISE_WORK_GROUP_SIZE = 8u;
				denoiseSize += DENOISE_WORK_GROUP_SIZE - denoiseSize % DENOISE_WORK_GROUP_SIZE;
				m_GTAODenoiseWorkGroups.x = (denoiseSize.x + 2u * DENOISE_WORK_GROUP_SIZE - 1u) / (DENOISE_WORK_GROUP_SIZE * 2u); // 2 horizontal pixels at a time.
				m_GTAODenoiseWorkGroups.y = denoiseSize.y / DENOISE_WORK_GROUP_SIZE;
			}

			///< SSR
			{
				constexpr uint32_t WORK_GROUP_SIZE = 8u;
				UVec2 ssrSize = m_SSROptions.HalfRes ? (viewportSize + 1u) / 2u : viewportSize;
				m_SSRImage->Resize(ssrSize);

				ssrSize += WORK_GROUP_SIZE - ssrSize % WORK_GROUP_SIZE;
				m_SSRWorkGroups.x = ssrSize.x / WORK_GROUP_SIZE;
				m_SSRWorkGroups.y = ssrSize.y / WORK_GROUP_SIZE;

				///< Pre-Convolution
				m_PreConvolutedTexture.Texture->Resize(ssrSize.x, ssrSize.y);

				///< Image Views (per-mip)
				ImageViewData imageViewSpec;
				imageViewSpec.debugName = std::format("PreConvolutionCompute");
				uint32_t mipCount = m_PreConvolutedTexture.Texture->GetMipLevelCount();
				m_PreConvolutedTexture.ImageViews.resize(mipCount);
				for (uint32_t mip = 0; mip < mipCount; mip++)
				{
					imageViewSpec.image = m_PreConvolutedTexture.Texture->GetImage();
					imageViewSpec.mip = mip;
                    m_PreConvolutedTexture.ImageViews[mip] = CreateRef<ImageView>(imageViewSpec);
				}

				///< Re-setup materials with new image views
				CreatePreConvolutionPassMaterials();
			}

			///< Bloom
			{
				UVec2 bloomSize = (viewportSize + 1u) / 2u;
				bloomSize += m_BloomComputeWorkgroupSize - bloomSize % m_BloomComputeWorkgroupSize;

				m_BloomComputeTextures[0].Texture->Resize(bloomSize);
				m_BloomComputeTextures[1].Texture->Resize(bloomSize);
				m_BloomComputeTextures[2].Texture->Resize(bloomSize);

				///< Image Views (per-mip)
				ImageViewData imageViewSpec;
				for (int i = 0; i < 3; i++)
				{
					imageViewSpec.debugName = std::format("BloomCompute-{}", i);
					uint32_t mipCount = m_BloomComputeTextures[i].Texture->GetMipLevelCount();
					m_BloomComputeTextures[i].ImageViews.resize(mipCount);
					for (uint32_t mip = 0; mip < mipCount; mip++)
					{
						imageViewSpec.image = m_BloomComputeTextures[i].Texture->GetImage();
						imageViewSpec.mip = mip;
                        m_BloomComputeTextures[i].ImageViews[mip] = CreateRef<ImageView>(imageViewSpec);
					}
				}

				///< Re-setup materials with new image views
				CreateBloomPassMaterials();
			}

			for (auto& tempFB : m_TempFramebuffers)
				tempFB->Resize(m_ViewportWidth, m_ViewportHeight);

			///< TODO: if (m_ExternalCompositeRenderPass)
            ///< TODO: 	m_ExternalCompositeRenderPass->GetSpecification().TargetFramebuffer->Resize(m_ViewportWidth, m_ViewportHeight);
		}

		///< Update uniform buffers
		UBOCamera& cameraData = CameraDataUB;
		UBOScene& sceneData = SceneDataUB;
		UBOShadow& shadowData = ShadowData;
		UBORendererData& rendererData = RendererDataUB;
		UBOPointLights& pointLightData = PointLightsUB;
		UBOScreenData& screenData = m_ScreenDataUB;
		UBOSpotLights& spotLightData = SpotLightUB;
		UBOSpotShadowData& spotShadowData = SpotShadowDataUB;

		auto& sceneCamera = m_SceneData.SceneCamera;
		const auto viewProjection = sceneCamera.camera.GetProjectionMatrix() * sceneCamera.ViewMatrix;
		const Mat4 viewInverse = sceneCamera.ViewMatrix.GetInverse();
		const Mat4 projectionInverse = sceneCamera.camera.GetProjectionMatrix().GetInverse();
		const Vec3 cameraPosition = viewInverse[3];

		cameraData.ViewProjection = viewProjection;
		cameraData.Projection = sceneCamera.camera.GetProjectionMatrix();
		cameraData.InverseProjection = projectionInverse;
		cameraData.View = sceneCamera.ViewMatrix;
		cameraData.InverseView = viewInverse;
		cameraData.InverseViewProjection = viewInverse * cameraData.InverseProjection;

		float depthLinearizeMul = (-cameraData.Projection[3][2]);    ///< float depthLinearizeMul = ( clipFar * clipNear ) / ( clipFar - clipNear );
		float depthLinearizeAdd = (cameraData.Projection[2][2]);     ///< float depthLinearizeAdd = clipFar / ( clipFar - clipNear );
        ///< correct the handedness issue.
		if (depthLinearizeMul * depthLinearizeAdd < 0)
			depthLinearizeAdd = -depthLinearizeAdd;
		cameraData.DepthUnpackConsts = { depthLinearizeMul, depthLinearizeAdd };
		const float* P = m_SceneData.SceneCamera.camera.GetProjectionMatrix().Data();
		const Vec4 projInfoPerspective = {
				 2.0f / (P[4 * 0 + 0]),                 ///< (x) * (R - L)/N
				 2.0f / (P[4 * 1 + 1]),                 ///< (y) * (T - B)/N
				-(1.0f - P[4 * 2 + 0]) / P[4 * 0 + 0],  ///< L/N
				-(1.0f + P[4 * 2 + 1]) / P[4 * 1 + 1],  ///< B/N
		};
		float tanHalfFOVY = 1.0f / cameraData.Projection[1][1];    ///< = tanf( drawContext.Camera.GetYFOV( ) * 0.5f );
		float tanHalfFOVX = 1.0f / cameraData.Projection[0][0];    ///< = tanHalfFOVY * drawContext.Camera.GetAspect( );
		cameraData.CameraTanHalfFOV = { tanHalfFOVX, tanHalfFOVY };
		cameraData.NDCToViewMul = { projInfoPerspective[0], projInfoPerspective[1] };
		cameraData.NDCToViewAdd = { projInfoPerspective[2], projInfoPerspective[3] };

		Ref<SceneRenderer> instance(this);
		Renderer::Submit([instance, cameraData]() mutable
		{
			instance->m_UBSCamera->GetRenderThread()->SetRenderThreadData(&cameraData, sizeof(cameraData));
		});

		const auto lightEnvironment = m_SceneData.SceneLightEnvironment;
		const std::vector<PointLightComponent>& pointLightsVec = lightEnvironment.PointLights;
		pointLightData.Count = static_cast<int>(pointLightsVec.size());
		if(pointLightsVec.data()) std::memcpy(pointLightData.PointLights, pointLightsVec.data(), lightEnvironment.GetPointLightsSize()); /// Do we really have to copy that?
		Renderer::Submit([instance, &pointLightData]() mutable
		{
			const Ref<UniformBuffer> uniformBuffer = instance->m_UBSPointLights->GetRenderThread();
			uniformBuffer->SetRenderThreadData(&pointLightData, 16ull + sizeof(PointLightComponent) * pointLightData.Count);
		});

		const std::vector<SpotLightComponent>& spotLightsVec = lightEnvironment.SpotLights;
		spotLightData.Count = static_cast<int>(spotLightsVec.size());
		if(spotLightsVec.data()) std::memcpy(spotLightData.SpotLights, spotLightsVec.data(), lightEnvironment.GetSpotLightsSize()); /// Do we really have to copy that?
		Renderer::Submit([instance, &spotLightData]() mutable
		{
			const Ref<UniformBuffer> uniformBuffer = instance->m_UBSSpotLights->GetRenderThread();
			uniformBuffer->SetRenderThreadData(&spotLightData, 16ull + sizeof(SpotLightComponent) * spotLightData.Count);
		});

		for (size_t i = 0; i < spotLightsVec.size(); ++i)
		{
			auto& light = spotLightsVec[i];
			if (!light.CastsShadows)
				continue;

			Mat4 projection = Perspective(ToRadians(light.Angle), 1.f, 0.1f, light.Range);
            ///< NOTE: ShadowMatrices[0] because we only support ONE shadow casting spot light at the moment and it MUST be index 0
			spotShadowData.ShadowMatrices[0] = projection * Mat4::LookAt(light.Position,  light.Position - light.Direction, Vec3(0.0f, 1.0f, 0.0f));
		}

		Renderer::Submit([instance, spotShadowData, spotLightsVec]() mutable
		{
			Ref<UniformBuffer> uniformBuffer = instance->m_UBSSpotShadowData->GetRenderThread();
			uniformBuffer->SetRenderThreadData(&spotShadowData, (uint32_t)(sizeof(Mat4) * spotLightsVec.size()));
		});

		const auto& directionalLight = m_SceneData.SceneLightEnvironment.DirectionalLights[0];
		sceneData.Lights.Direction = directionalLight.Direction;
		sceneData.Lights.Radiance = directionalLight.Radiance;
		sceneData.Lights.Intensity = directionalLight.Intensity;
		sceneData.Lights.ShadowAmount = directionalLight.ShadowAmount;

		sceneData.CameraPosition = cameraPosition;
		sceneData.EnvironmentMapIntensity = m_SceneData.SceneEnvironmentIntensity;
		Renderer::Submit([instance, sceneData]() mutable
		{
			instance->m_UBSScene->GetRenderThread()->SetRenderThreadData(&sceneData, sizeof(sceneData));
		});

		if (m_Options.EnableGTAO)
			UpdateGTAOData();

		Renderer::Submit([instance, screenData]() mutable
		{
			instance->m_UBSScreenData->GetRenderThread()->SetRenderThreadData(&screenData, sizeof(screenData));
		});

		CascadeData cascades[4];
		if (m_UseManualCascadeSplits)
			CalculateCascadesManualSplit(cascades, sceneCamera, directionalLight.Direction);
		else
			CalculateCascades(cascades, sceneCamera, directionalLight.Direction);


		///< TODO: four cascades for now
		for (int i = 0; i < 4; i++)
		{
			CascadeSplits[i] = cascades[i].SplitDepth;
			shadowData.ViewProjection[i] = cascades[i].ViewProj;
		}
		Renderer::Submit([instance, shadowData]() mutable
		{
			instance->m_UBSShadow->GetRenderThread()->SetRenderThreadData(&shadowData, sizeof(shadowData));
		});

		rendererData.CascadeSplits = CascadeSplits;
		Renderer::Submit([instance, rendererData]() mutable
		{
			instance->m_UBSRendererData->GetRenderThread()->SetRenderThreadData(&rendererData, sizeof(rendererData));
		});

		///< NOTE: shouldn't be necessary anymore with new Render Passes
		///< Renderer::SetSceneEnvironment(this, m_SceneData.SceneEnvironment,
		///< 	m_ShadowPassPipelines[0]->GetSpecification().RenderPass->GetSpecification().dstFramebuffer->GetDepthImage(),
		///< 	m_SpotShadowPassPipeline->GetSpecification().RenderPass->GetSpecification().dstFramebuffer->GetDepthImage());
	}

	void SceneRenderer::EndScene()
	{
		SEDX_PROFILE_FUNC();
		SEDX_CORE_ASSERT(m_Active);
#if MULTI_THREAD
        Ref<SceneRenderer> instance(this);
		s_ThreadPool.emplace_back(([instance]() mutable
		{
			instance->FlushDrawList();
		}));
#else
		FlushDrawList();
#endif

		m_Active = false;
	}

	void SceneRenderer::WaitForThreads()
	{
		for (auto &i : s_ThreadPool)
            i.join();

		s_ThreadPool.clear();
	}

	void SceneRenderer::SubmitMesh(Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<MaterialTable> materialTable, const Mat4& transform, const std::vector<Mat4>& boneTransforms, Ref<Material> overrideMaterial)
	{
		SEDX_PROFILE_FUNC();

		///< TODO: Culling, sorting, etc.

		const auto& submeshes = meshSource->GetSubmeshes();
		const auto& submesh = submeshes[submeshIndex];
		uint32_t materialIndex = submesh.MaterialIndex;
		bool isRigged = submesh.IsRigged;

		AssetHandle materialHandle = materialTable->HasMaterial(materialIndex) ? materialTable->GetMaterial(materialIndex) : mesh->GetMaterials()->GetMaterial(materialIndex);
		const Ref<MaterialAsset> material = AssetManager::GetAsset<MaterialAsset>(materialHandle);

		MeshKey meshKey = { mesh->Handle, materialHandle, submeshIndex, false };
		auto&[MRow] = m_MeshTransformMap[meshKey].Transforms.emplace_back();

		MRow[0] = { transform[0][0], transform[1][0], transform[2][0], transform[3][0] };
		MRow[1] = { transform[0][1], transform[1][1], transform[2][1], transform[3][1] };
		MRow[2] = { transform[0][2], transform[1][2], transform[2][2], transform[3][2] };

		if (isRigged)
		{
			CopyToBoneTransformStorage(meshKey, meshSource, boneTransforms);
		}
        ///< Main geo
		{
			bool isTransparent = material->IsTransparent();
			auto& destDrawList = !isTransparent ? m_DrawList : m_TransparentDrawList;
			auto& dc = destDrawList[meshKey];
			dc.Mesh = mesh;
			dc.MeshSource = meshSource;
			dc.SubmeshIndex = submeshIndex;
			dc.MaterialTable = materialTable;
			dc.OverrideMaterial = overrideMaterial;
			dc.InstanceCount++;
			dc.IsRigged = isRigged;  ///< TODO: would it be better to have separate draw list for rigged meshes, or this flag is OK?
		}

		///< Shadow pass
		if (material->IsShadowCasting())
		{
			auto& dc = m_ShadowPassDrawList[meshKey];
			dc.Mesh = mesh;
			dc.MeshSource = meshSource;
			dc.SubmeshIndex = submeshIndex;
			dc.MaterialTable = materialTable;
			dc.OverrideMaterial = overrideMaterial;
			dc.InstanceCount++;
			dc.IsRigged = isRigged;
		}
	}

	void SceneRenderer::SubmitStaticMesh(Ref<StaticMesh> staticMesh, Ref<MeshSource> meshSource, const Ref<MaterialTable> &materialTable, const Mat4& transform, const Ref<Material> &overrideMaterial)
	{
		SEDX_PROFILE_FUNC();

		const auto& submeshData = meshSource->GetSubmeshes();
		for (uint32_t submeshIndex : staticMesh->GetSubmeshes())
		{
			Mat4 submeshTransform = transform * submeshData[submeshIndex].Transform;

			uint32_t materialIndex = submeshData[submeshIndex].MaterialIndex;

			AssetHandle materialHandle = materialTable->HasMaterial(materialIndex) ? materialTable->GetMaterial(materialIndex) : staticMesh->GetMaterials()->GetMaterial(materialIndex);
			SEDX_CORE_VERIFY(materialHandle);
			const Ref<MaterialAsset> material = AssetManager::GetAsset<MaterialAsset>(materialHandle);

			MeshKey meshKey = { staticMesh->Handle, materialHandle, submeshIndex, false };
			auto& transformStorage = m_MeshTransformMap[meshKey].Transforms.emplace_back();

			transformStorage.MRow[0] = { submeshTransform[0][0], submeshTransform[1][0], submeshTransform[2][0], submeshTransform[3][0] };
			transformStorage.MRow[1] = { submeshTransform[0][1], submeshTransform[1][1], submeshTransform[2][1], submeshTransform[3][1] };
			transformStorage.MRow[2] = { submeshTransform[0][2], submeshTransform[1][2], submeshTransform[2][2], submeshTransform[3][2] };

			///< Main geo
			{
				bool isTransparent = material->IsTransparent();
				auto& destDrawList = !isTransparent ? m_StaticMeshDrawList : m_TransparentStaticMeshDrawList;
				auto& dc = destDrawList[meshKey];
				dc.StaticMesh = staticMesh;
				dc.MeshSource = meshSource;
				dc.SubmeshIndex = submeshIndex;
				dc.MaterialTable = materialTable;
				dc.OverrideMaterial = overrideMaterial;
				dc.InstanceCount++;
			}

			///< Shadow pass
			if (material->IsShadowCasting())
			{
				auto& dc = m_StaticMeshShadowPassDrawList[meshKey];
				dc.StaticMesh = staticMesh;
				dc.MeshSource = meshSource;
				dc.SubmeshIndex = submeshIndex;
				dc.MaterialTable = materialTable;
				dc.OverrideMaterial = overrideMaterial;
				dc.InstanceCount++;
			}
		}

	}

	void SceneRenderer::SubmitSelectedMesh(Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<MaterialTable> materialTable, const Mat4& transform, const std::vector<Mat4>& boneTransforms, Ref<Material> overrideMaterial)
	{
		SEDX_PROFILE_FUNC();

		///< TODO: Culling, sorting, etc.
		const auto& submeshes = meshSource->GetSubmeshes();
		const auto& submesh = submeshes[submeshIndex];
		uint32_t materialIndex = submesh.MaterialIndex;
		bool isRigged = submesh.IsRigged;

		AssetHandle materialHandle = materialTable->HasMaterial(materialIndex) ? materialTable->GetMaterial(materialIndex) : mesh->GetMaterials()->GetMaterial(materialIndex);
		SEDX_CORE_VERIFY(materialHandle);
		const Ref<MaterialAsset> material = AssetManager::GetAsset<MaterialAsset>(materialHandle);

		MeshKey meshKey = { mesh->Handle, materialHandle, submeshIndex, true };
		auto& transformStorage = m_MeshTransformMap[meshKey].Transforms.emplace_back();

		transformStorage.MRow[0] = { transform[0][0], transform[1][0], transform[2][0], transform[3][0] };
		transformStorage.MRow[1] = { transform[0][1], transform[1][1], transform[2][1], transform[3][1] };
		transformStorage.MRow[2] = { transform[0][2], transform[1][2], transform[2][2], transform[3][2] };

		if (isRigged)
		{
			CopyToBoneTransformStorage(meshKey, meshSource, boneTransforms);
		}

		uint32_t instanceIndex = 0;

		///< Main geo
		{
			bool isTransparent = material->IsTransparent();
			auto& destDrawList = !isTransparent ? m_DrawList : m_TransparentDrawList;
			auto& dc = destDrawList[meshKey];
			dc.Mesh = mesh;
			dc.MeshSource = meshSource;
			dc.SubmeshIndex = submeshIndex;
			dc.MaterialTable = materialTable;
			dc.OverrideMaterial = overrideMaterial;

			instanceIndex = dc.InstanceCount;
			dc.InstanceCount++;
			dc.IsRigged = isRigged;
		}

		///< Selected mesh list
		{
			auto& dc = m_SelectedMeshDrawList[meshKey];
			dc.Mesh = mesh;
			dc.MeshSource = meshSource;
			dc.SubmeshIndex = submeshIndex;
			dc.MaterialTable = materialTable;
			dc.OverrideMaterial = overrideMaterial;
			dc.InstanceCount++;
			dc.InstanceOffset = instanceIndex;
			dc.IsRigged = isRigged;
		}

		///< Shadow pass
		if (material->IsShadowCasting())
		{
			auto& dc = m_ShadowPassDrawList[meshKey];
			dc.Mesh = mesh;
			dc.MeshSource = meshSource;
			dc.SubmeshIndex = submeshIndex;
			dc.MaterialTable = materialTable;
			dc.OverrideMaterial = overrideMaterial;
			dc.InstanceCount++;
			dc.IsRigged = isRigged;
		}
	}

	void SceneRenderer::SubmitSelectedStaticMesh(Ref<StaticMesh> staticMesh, Ref<MeshSource> meshSource, const Ref<MaterialTable> &materialTable, const Mat4& transform, const Ref<Material> &overrideMaterial)
	{
		SEDX_PROFILE_FUNC();

		const auto& submeshData = meshSource->GetSubmeshes();
		for (uint32_t submeshIndex : staticMesh->GetSubmeshes())
		{
			Mat4 submeshTransform = transform * submeshData[submeshIndex].Transform;

			uint32_t materialIndex = submeshData[submeshIndex].MaterialIndex;

			AssetHandle materialHandle = materialTable->HasMaterial(materialIndex) ? materialTable->GetMaterial(materialIndex) : staticMesh->GetMaterials()->GetMaterial(materialIndex);
			SEDX_CORE_VERIFY(materialHandle);
			const Ref<MaterialAsset> material = AssetManager::GetAsset<MaterialAsset>(materialHandle);

			MeshKey meshKey = { staticMesh->Handle, materialHandle, submeshIndex, true };
			auto&[MRow] = m_MeshTransformMap[meshKey].Transforms.emplace_back();

			MRow[0] = { submeshTransform[0][0], submeshTransform[1][0], submeshTransform[2][0], submeshTransform[3][0] };
			MRow[1] = { submeshTransform[0][1], submeshTransform[1][1], submeshTransform[2][1], submeshTransform[3][1] };
			MRow[2] = { submeshTransform[0][2], submeshTransform[1][2], submeshTransform[2][2], submeshTransform[3][2] };

			///< Main geo
			{
				bool isTransparent = material->IsTransparent();
				auto& destDrawList = !isTransparent ? m_StaticMeshDrawList : m_TransparentStaticMeshDrawList;
				auto&[StaticMesh, MeshSource, SubmeshIndex, MaterialTable, OverrideMaterial, InstanceCount] = destDrawList[meshKey];
				StaticMesh = staticMesh;
				MeshSource = meshSource;
				SubmeshIndex = submeshIndex;
				MaterialTable = materialTable;
				OverrideMaterial = overrideMaterial;
				InstanceCount++;
			}

			///< Selected mesh list
			{
				auto&[StaticMesh, MeshSource, SubmeshIndex, MaterialTable, OverrideMaterial, InstanceCount] = m_SelectedStaticMeshDrawList[meshKey];
				StaticMesh = staticMesh;
				MeshSource = meshSource;
				SubmeshIndex = submeshIndex;
				MaterialTable = materialTable;
				OverrideMaterial = overrideMaterial;
				InstanceCount++;
			}

			///< Shadow pass
			if (material->IsShadowCasting())
			{
				auto&[StaticMesh, MeshSource, SubmeshIndex, MaterialTable, OverrideMaterial, InstanceCount] = m_StaticMeshShadowPassDrawList[meshKey];
				StaticMesh = staticMesh;
				MeshSource = meshSource;
				SubmeshIndex = submeshIndex;
				MaterialTable = materialTable;
				OverrideMaterial = overrideMaterial;
				InstanceCount++;
			}
		}
	}

	void SceneRenderer::SubmitPhysicsDebugMesh(Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, const Mat4& transform, const bool isSimpleCollider)
	{
		SEDX_CORE_VERIFY(mesh);
		SEDX_CORE_VERIFY(meshSource);

		const auto& submeshData = meshSource->GetSubmeshes();
		Mat4 submeshTransform = transform * submeshData[submeshIndex].Transform;

		MeshKey meshKey = { mesh->Handle, 5, submeshIndex, false };
		auto&[MRow] = m_MeshTransformMap[meshKey].Transforms.emplace_back();

		MRow[0] = { transform[0][0], transform[1][0], transform[2][0], transform[3][0] };
		MRow[1] = { transform[0][1], transform[1][1], transform[2][1], transform[3][1] };
		MRow[2] = { transform[0][2], transform[1][2], transform[2][2], transform[3][2] };

		{
			auto& dc = m_ColliderDrawList[meshKey];
			dc.Mesh = mesh;
			dc.MeshSource = meshSource;
			dc.SubmeshIndex = submeshIndex;
			dc.OverrideMaterial = isSimpleCollider ? m_SimpleColliderMaterial : m_ComplexColliderMaterial;
			dc.InstanceCount++;
		}
	}

	void SceneRenderer::SubmitAnimationDebugMesh(const Mat4& transform, const bool isSelected)
	{
		SubmitStaticDebugMesh(m_AnimationDebugDrawList, m_BoneMesh, m_BoneMeshSource, transform, isSelected ? m_SelectedBoneMaterial : m_BoneMaterial);

		///< Draw a line, 1 uint along y-axis  (the transform has been set such that this line will go to next bone)
		Vec3 p0 = transform[3];
		Vec3 p1 = transform * Vec4(0.0f, 1.0f, 0.0f, 1.0f);
		GetDebugRenderer()->DrawLine(p0, p1, { 0.0f, 1.0f, 0.0f, 1.0f }, true);
	}

	void SceneRenderer::SubmitPhysicsStaticDebugMesh(Ref<StaticMesh> staticMesh, Ref<MeshSource> meshSource, const Mat4& transform, const bool isSimpleCollider)
	{
		SubmitStaticDebugMesh(m_StaticColliderDrawList, staticMesh, meshSource, transform, isSimpleCollider ? m_SimpleColliderMaterial : m_ComplexColliderMaterial);
	}

	void SceneRenderer::SubmitStaticDebugMesh(std::map<SceneRenderer::MeshKey, SceneRenderer::StaticDrawCommand>& drawList, Ref<StaticMesh> staticMesh, Ref<MeshSource> meshSource, const Mat4& transform, const Ref<Material> &material)
	{
		SEDX_PROFILE_FUNC();
		SEDX_CORE_VERIFY(staticMesh);
		SEDX_CORE_VERIFY(meshSource);

		const auto& submeshData = meshSource->GetSubmeshes();
		for (uint32_t submeshIndex : staticMesh->GetSubmeshes())
		{
			Mat4 submeshTransform = transform * submeshData[submeshIndex].Transform;

			///< HACK: Correct instancing of draw calls relies on a unique material handle
			///<       in the MeshKey.
			///<       We do not have a MaterialAsset here and hence no handle.
			///<       We fake a handle from the Material object's address.
			AssetHandle fakeHandle = (uint64_t)material.Get();

			MeshKey meshKey = { staticMesh->Handle, fakeHandle, submeshIndex, false };
			auto&[MRow] = m_MeshTransformMap[meshKey].Transforms.emplace_back();

			MRow[0] = { submeshTransform[0][0], submeshTransform[1][0], submeshTransform[2][0], submeshTransform[3][0] };
			MRow[1] = { submeshTransform[0][1], submeshTransform[1][1], submeshTransform[2][1], submeshTransform[3][1] };
			MRow[2] = { submeshTransform[0][2], submeshTransform[1][2], submeshTransform[2][2], submeshTransform[3][2] };

			{
				auto& dc = drawList[meshKey];
				dc.StaticMesh = staticMesh;
				dc.MeshSource = meshSource;
				dc.SubmeshIndex = submeshIndex;
				dc.OverrideMaterial = material;
				dc.InstanceCount++;
			}
		}
	}

	void SceneRenderer::ClearPass(const Ref<RenderPass> &renderPass, bool explicitClear) const
    {
		SEDX_PROFILE_FUNC();
		Renderer::BeginFrame(m_CommandBuffer, renderPass, explicitClear);
		Renderer::EndFrame(m_CommandBuffer);
	}

	void SceneRenderer::ShadowMapPass()
	{
		SEDX_PROFILE_FUNC();

		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		m_GPUTimeQueries.DirShadowMapPassQuery = m_CommandBuffer->BeginTimestampQuery();

		auto& directionalLights = m_SceneData.SceneLightEnvironment.DirectionalLights;
		if (directionalLights[0].Intensity == 0.0f || !directionalLights[0].CastShadows)
		{
            ///< Clear shadow maps
			for (uint32_t i = 0; i < m_Specification.NumShadowCascades; i++)
				ClearPass(m_DirectionalShadowMapPass[i]);

			return;
		}

		for (uint32_t i = 0; i < m_Specification.NumShadowCascades; i++)
		{
			Renderer::BeginFrame(m_CommandBuffer, m_DirectionalShadowMapPass[i]);

			///< Render entities
			const Buffer cascade(&i, sizeof(uint32_t));
			for (auto& [mk, dc] : m_StaticMeshShadowPassDrawList)
			{
                SEDX_CORE_VERIFY(m_MeshTransformMap.contains(mk));
				const auto&[Transforms, TransformOffset] = m_MeshTransformMap.at(mk);
				Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, m_ShadowPassPipelines[i], dc.StaticMesh, dc.MeshSource, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, TransformOffset, dc.InstanceCount, m_ShadowPassMaterial, cascade);
			}
			for (auto& [mk, dc] : m_ShadowPassDrawList)
			{
                SEDX_CORE_VERIFY(m_MeshTransformMap.contains(mk));
				const auto&[Transforms, TransformOffset] = m_MeshTransformMap.at(mk);
				if (!dc.IsRigged)
					Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_ShadowPassPipelines[i], dc.Mesh, dc.MeshSource, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, TransformOffset, 0, 0, dc.InstanceCount, m_ShadowPassMaterial, cascade);
			}

			Renderer::EndFrame(m_CommandBuffer);
		}

		for (uint32_t i = 0; i < m_Specification.NumShadowCascades; i++)
		{
			Renderer::BeginFrame(m_CommandBuffer, m_DirectionalShadowMapAnimPass[i]);

			///< Render entities
			const Buffer cascade(&i, sizeof(uint32_t));
			for (auto& [mk, dc] : m_ShadowPassDrawList)
			{
                SEDX_CORE_VERIFY(m_MeshTransformMap.contains(mk));
				const auto&[Transforms, TransformOffset] = m_MeshTransformMap.at(mk);
				if (dc.IsRigged)
				{
					const auto& boneTransformsData = m_MeshBoneTransformsMap.at(mk);
					Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_ShadowPassPipelinesAnim[i], dc.Mesh, dc.MeshSource, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, TransformOffset, boneTransformsData.BoneTransformsBaseIndex, boneTransformsData.BoneTransformsStride, dc.InstanceCount, m_ShadowPassMaterial, cascade);
				}
			}

			Renderer::EndFrame(m_CommandBuffer);
		}

		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.DirShadowMapPassQuery);
	}

	void SceneRenderer::SpotShadowMapPass()
	{
		SEDX_PROFILE_FUNC();

		const uint64_t frameIndex = Renderer::GetCurrentFrameIndex();
		m_GPUTimeQueries.SpotShadowMapPassQuery = m_CommandBuffer->BeginTimestampQuery();

		///< Spot shadow maps
		Renderer::BeginFrame(m_CommandBuffer, m_SpotShadowPass);
		for (uint32_t i = 0; i < 1; i++)
		{

			const Buffer lightIndex(&i, sizeof(uint32_t));
			for (auto& [mk, dc] : m_StaticMeshShadowPassDrawList)
			{
                SEDX_CORE_VERIFY(m_MeshTransformMap.contains(mk));
				const auto&[Transforms, TransformOffset] = m_MeshTransformMap.at(mk);
				Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, m_SpotShadowPassPipeline, dc.StaticMesh, dc.MeshSource, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, TransformOffset, dc.InstanceCount, m_SpotShadowPassMaterial, lightIndex);
			}
			for (auto& [mk, dc] : m_ShadowPassDrawList)
			{
                SEDX_CORE_VERIFY(m_MeshTransformMap.contains(mk));
				const auto&[Transforms, TransformOffset] = m_MeshTransformMap.at(mk);
				if (dc.IsRigged)
				{
					const auto& boneTransformsData = m_MeshBoneTransformsMap.at(mk);
					Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_SpotShadowPassAnimPipeline, dc.Mesh, dc.MeshSource, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, TransformOffset, boneTransformsData.BoneTransformsBaseIndex, boneTransformsData.BoneTransformsStride, dc.InstanceCount, m_SpotShadowPassMaterial, lightIndex);
				}
				else
				{
					Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_SpotShadowPassPipeline, dc.Mesh, dc.MeshSource, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, TransformOffset, 0, 0, dc.InstanceCount, m_SpotShadowPassMaterial, lightIndex);
				}
			}

		}
		Renderer::EndFrame(m_CommandBuffer);
		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.SpotShadowMapPassQuery);
	}

	void SceneRenderer::PreDepthPass()
	{
		SEDX_PROFILE_FUNC();

		uint64_t frameIndex = Renderer::GetCurrentFrameIndex();
		m_GPUTimeQueries.DepthPrePassQuery = m_CommandBuffer->BeginTimestampQuery();
		SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "PreDepthPass");
		Renderer::BeginFrame(m_CommandBuffer, m_PreDepthPass);
		for (auto& [mk, dc] : m_StaticMeshDrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, m_PreDepthPipeline, dc.StaticMesh, dc.MeshSource, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, dc.InstanceCount, m_PreDepthMaterial);
		}
		for (auto& [mk, dc] : m_DrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			if (!dc.IsRigged)
				Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_PreDepthPipeline, dc.Mesh, dc.MeshSource, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, 0, 0, dc.InstanceCount, m_PreDepthMaterial);
		}
		Renderer::EndFrame(m_CommandBuffer);

		Renderer::BeginFrame(m_CommandBuffer, m_PreDepthAnimPass);
		for (auto& [mk, dc] : m_DrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			if (dc.IsRigged)
			{
				const auto& boneTransformsData = m_MeshBoneTransformsMap.at(mk);
				Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_PreDepthPipelineAnim, dc.Mesh, dc.MeshSource, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, boneTransformsData.BoneTransformsBaseIndex, boneTransformsData.BoneTransformsStride, dc.InstanceCount, m_PreDepthMaterial);
			}
		}

		Renderer::EndFrame(m_CommandBuffer);

    #if 0
		Renderer::BeginRenderPass(m_CommandBuffer, m_PreDepthTransparentPass);
		for (auto& [mk, dc] : m_TransparentStaticMeshDrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_PreDepthTransparentPipeline, m_UniformBufferSet, nullptr, dc.StaticMesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, 0, 0, dc.InstanceCount, m_PreDepthMaterial);
		}
		for (auto& [mk, dc] : m_TransparentDrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			if (dc.IsRigged)
			{
				const auto& boneTransformsData = m_MeshBoneTransformsMap.at(mk);

				///< TODO: This needs to be pre-depth transparent-anim pipeline
				Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_PreDepthPipelineAnim, m_UniformBufferSet, nullptr, dc.Mesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, m_BoneTransformStorageBuffers, boneTransformsData.BoneTransformsBaseIndex, boneTransformsData.BoneTransformsStride, dc.InstanceCount, m_PreDepthMaterial);
			}
			else
			{
				Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_PreDepthTransparentPipeline, m_UniformBufferSet, nullptr, dc.Mesh, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, 0, 0, dc.InstanceCount, m_PreDepthMaterial);
			}
		}

		Renderer::EndRenderPass(m_CommandBuffer);
    #endif
		SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);

    #if 0 // Is this necessary?
		Renderer::Submit([cb = m_CommandBuffer, image = m_PreDepthPipeline->GetSpecification().RenderPass->GetSpecification().dstFramebuffer->GetDepthImage().As<VulkanImage2D>()]()
		{
			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			imageMemoryBarrier.image = image->GetImageInfo().Image;
			imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, image->GetSpecification().Mips, 0, 1 };
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(
				cb.As<CommandBuffer>()->GetCommandBuffer(Renderer::GetCurrentFrameIndex()),
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);
	    });
    #endif

		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.DepthPrePassQuery);
    }

	void SceneRenderer::Compute()
	{
		SEDX_PROFILE_FUNC();

		m_GPUTimeQueries.HierarchicalDepthQuery = m_CommandBuffer->BeginTimestampQuery();

		constexpr uint32_t maxMipBatchSize = 4;
		const uint32_t hzbMipCount = m_HierarchicalDepthTexture.Texture->GetMipLevelCount();

		Renderer::BeginGPUPerfMarker(m_CommandBuffer, "HZB");
		Renderer::BeginComputePass(m_CommandBuffer, m_HierarchicalDepthPass);

		auto ReduceHZB = [commandBuffer = m_CommandBuffer, hierarchicalDepthPass = m_HierarchicalDepthPass, hierarchicalDepthTexture = m_HierarchicalDepthTexture.Texture, hzbMaterials = m_HZBMaterials]
		(const uint32_t startDestMip, const uint32_t parentMip, const Vec2& DispatchThreadIdToBufferUV, const Vec2& InputViewportMaxBound, const bool isFirstPass)
		{
			struct HierarchicalZComputePushConstants
			{
				Vec2 DispatchThreadIdToBufferUV;
				Vec2 InputViewportMaxBound;
				Vec2 InvSize;
				int FirstLod;
				bool IsFirstPass;
				char Padding[3]{ 0, 0, 0 };
			} hierarchicalZComputePushConstants;

			hierarchicalZComputePushConstants.IsFirstPass = isFirstPass;
			hierarchicalZComputePushConstants.FirstLod = (int)startDestMip;
			hierarchicalZComputePushConstants.DispatchThreadIdToBufferUV = DispatchThreadIdToBufferUV;
			hierarchicalZComputePushConstants.InputViewportMaxBound = InputViewportMaxBound;

			const auto srcSize = Math::DivideAndRoundUp(hierarchicalDepthTexture->GetSize(), 1u << parentMip);
			const auto dstSize = Math::DivideAndRoundUp(hierarchicalDepthTexture->GetSize(), 1u << startDestMip);
			hierarchicalZComputePushConstants.InvSize = Vec2{ 1.0f / (float)srcSize.x, 1.0f / (float)srcSize.y };

			UVec3 workGroups(Math::DivideAndRoundUp(dstSize.x, 8), Math::DivideAndRoundUp(dstSize.y, 8), 1);
			Renderer::DispatchCompute(commandBuffer, hierarchicalDepthPass, hzbMaterials[startDestMip / 4], workGroups, Buffer(&hierarchicalZComputePushConstants, sizeof(hierarchicalZComputePushConstants)));
		};

		Renderer::BeginGPUPerfMarker(m_CommandBuffer, "HZB-FirstPass");

		///< Reduce first 4 mips
		auto srcSize = m_PreDepthPass->GetDepthOutput()->GetSize();
		ReduceHZB(0, 0, { 1.0f / Vec2{srcSize} }, { (Vec2{srcSize} - 0.5f) / Vec2{srcSize} }, true);
		Renderer::EndGPUPerfMarker(m_CommandBuffer);

		///< Reduce the next mips
		for (uint32_t startDestMip = maxMipBatchSize; startDestMip < hzbMipCount; startDestMip += maxMipBatchSize)
		{
			Renderer::BeginGPUPerfMarker(m_CommandBuffer, std::format("HZB-Pass-({})", startDestMip));
			srcSize = Utils::Math::DivideAndRoundUp(m_HierarchicalDepthTexture.Texture->GetSize(), 1u << static_cast<uint32_t>(startDestMip - 1));
			ReduceHZB(startDestMip, startDestMip - 1, { 2.0f / Vec2{ srcSize } }, Vec2{ 1.0f }, false);
			Renderer::EndGPUPerfMarker(m_CommandBuffer);
		}

		Renderer::EndGPUPerfMarker(m_CommandBuffer);

		Renderer::EndComputePass(m_CommandBuffer, m_HierarchicalDepthPass);
		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.HierarchicalDepthQuery);
	}

	void SceneRenderer::PreIntegration()
	{
		SEDX_PROFILE_FUNC();

		m_GPUTimeQueries.PreIntegrationQuery = m_CommandBuffer->BeginTimestampQuery();
		Vec2 projectionParams = { m_SceneData.SceneCamera.Far, m_SceneData.SceneCamera.Near }; // Reversed

		Ref<Texture2D> visibilityTexture = m_PreIntegrationVisibilityTexture.Texture;

		ImageClearValue clearValue = { Vec4(1.0f) };
		ImageSubresourceRange subresourceRange{};
		subresourceRange.mip = 0;
		subresourceRange.mipCount = 1;
		Renderer::ClearImage(m_CommandBuffer, visibilityTexture->GetImage(), clearValue, subresourceRange);

		struct PreIntegrationComputePushConstants
		{
			Vec2 HZBResFactor;
			Vec2 ResFactor;
			Vec2 ProjectionParams; ///<(x) = Near plane, (y) = Far plane
			int PrevLod = 0;
		} preIntegrationComputePushConstants;

		Renderer::BeginGPUPerfMarker(m_CommandBuffer, "PreIntegration");

		Renderer::BeginComputePass(m_CommandBuffer, m_PreIntegrationPass);

		for (uint32_t mip = 1; mip < visibilityTexture->GetMipLevelCount(); mip++)
		{
			Renderer::BeginGPUPerfMarker(m_CommandBuffer, std::format("PreIntegration-Pass({})", mip));
			auto [mipWidth, mipHeight] = visibilityTexture->GetMipSize(mip);
			UVec3 workGroups = { (uint32_t)std::ceil((float)mipWidth / 8.0f), (uint32_t)std::ceil((float)mipHeight / 8.0f), 1 };

			auto [width, height] = visibilityTexture->GetMipSize(mip);
			Vec2 resFactor = 1.0f / Vec2{ width, height };
			preIntegrationComputePushConstants.HZBResFactor = resFactor * m_SSROptions.BUvFactor;
			preIntegrationComputePushConstants.ResFactor = resFactor;
			preIntegrationComputePushConstants.ProjectionParams = projectionParams;
			preIntegrationComputePushConstants.PrevLod = (int)mip - 1;

			Buffer pushConstants(&preIntegrationComputePushConstants, sizeof(PreIntegrationComputePushConstants));
			Renderer::DispatchCompute(m_CommandBuffer, m_PreIntegrationPass, m_PreIntegrationMaterials[mip - 1], workGroups, pushConstants);

			Renderer::EndGPUPerfMarker(m_CommandBuffer);
		}
		Renderer::EndComputePass(m_CommandBuffer, m_PreIntegrationPass);
		Renderer::EndGPUPerfMarker(m_CommandBuffer);

		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.PreIntegrationQuery);
	}

	void SceneRenderer::LightCullingPass()
	{
		m_GPUTimeQueries.LightCullingPassQuery = m_CommandBuffer->BeginTimestampQuery();
		SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "LightCulling", { 1.0f, 1.0f, 1.0f, 1.0f });

		Renderer::BeginComputePass(m_CommandBuffer, m_LightCullingPass);
		Renderer::DispatchCompute(m_CommandBuffer, m_LightCullingPass, m_LightCullingMaterial, m_LightCullingWorkGroups);
		Renderer::EndComputePass(m_CommandBuffer, m_LightCullingPass);

		///< NOTE: ideally this would be done automatically by RenderPass/ComputePass system
		Ref<ComputePipeline> pipeline = m_LightCullingPass->GetPipeline();
		pipeline->BufferMemoryBarrier(m_CommandBuffer, m_SBSVisiblePointLightIndicesBuffer->Get(),
			PipelineStage::ComputeShader, ResourceAccessFlags::ShaderWrite,
			PipelineStage::FragmentShader, ResourceAccessFlags::ShaderRead);
		pipeline->BufferMemoryBarrier(m_CommandBuffer, m_SBSVisibleSpotLightIndicesBuffer->Get(),
			PipelineStage::ComputeShader, ResourceAccessFlags::ShaderWrite,
			PipelineStage::FragmentShader, ResourceAccessFlags::ShaderRead);

		SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);
		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.LightCullingPassQuery);
	}

	void SceneRenderer::SkyboxPass() const
    {
		SEDX_PROFILE_FUNC();

		Renderer::BeginFrame(m_CommandBuffer, m_SkyboxPass);

		///< Skybox
		m_SkyboxMaterial->Set("u_Uniforms.TextureLod", m_SceneData.SkyboxLod);
		m_SkyboxMaterial->Set("u_Uniforms.Intensity", m_SceneData.SceneEnvironmentIntensity);

		const Ref<TextureCube> radianceMap = m_SceneData.SceneEnvironment ? m_SceneData.SceneEnvironment->RadianceMap : Renderer::GetBlackCubeTexture();
		m_SkyboxMaterial->Set("u_Texture", radianceMap);

		SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "Skybox", { 0.3f, 0.0f, 1.0f, 1.0f });
		Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_SkyboxPipeline, m_SkyboxMaterial);
		SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);
		Renderer::EndFrame(m_CommandBuffer);
	}

	void SceneRenderer::GeometryPass()
	{
		SEDX_PROFILE_FUNC();

		uint64_t frameIndex = Renderer::GetCurrentFrameIndex();

		m_GPUTimeQueries.GeometryPassQuery = m_CommandBuffer->BeginTimestampQuery();

		Renderer::BeginFrame(m_CommandBuffer, m_SelectedGeometryPass);
		for (auto& [mk, dc] : m_SelectedStaticMeshDrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, m_SelectedGeometryPass->GetSpecification().Pipeline, dc.StaticMesh, dc.MeshSource, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, dc.InstanceCount, m_SelectedGeometryMaterial);
		}
		for (auto& [mk, dc] : m_SelectedMeshDrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			if (!dc.IsRigged)
				Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_SelectedGeometryPass->GetPipeline(), dc.Mesh, dc.MeshSource, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset + dc.InstanceOffset * sizeof(TransformVertexData), 0, 0, dc.InstanceCount, m_SelectedGeometryMaterial);
		}
		Renderer::EndFrame(m_CommandBuffer);

		Renderer::BeginFrame(m_CommandBuffer, m_SelectedGeometryAnimPass);
		for (auto& [mk, dc] : m_SelectedMeshDrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			if (dc.IsRigged)
			{
				const auto& boneTransformsData = m_MeshBoneTransformsMap.at(mk);
				Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_SelectedGeometryAnimPass->GetPipeline(), dc.Mesh, dc.MeshSource, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset + dc.InstanceOffset * sizeof(TransformVertexData), boneTransformsData.BoneTransformsBaseIndex + dc.InstanceOffset * boneTransformsData.BoneTransformsStride, boneTransformsData.BoneTransformsStride, dc.InstanceCount, m_SelectedGeometryMaterial);
			}
		}
		Renderer::EndFrame(m_CommandBuffer);

		Renderer::BeginFrame(m_CommandBuffer, m_GeometryPass);
        ///< Render static meshes
		SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "Static Meshes");
		for (auto& [mk, dc] : m_StaticMeshDrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			Renderer::RenderStaticMesh(m_CommandBuffer, m_GeometryPipeline, dc.StaticMesh, dc.MeshSource, dc.SubmeshIndex, dc.MaterialTable ? dc.MaterialTable : dc.StaticMesh->GetMaterials(), m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, dc.InstanceCount);
		}
		SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);

		///< Render dynamic meshes
		SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "Dynamic Meshes");
		for (auto& [mk, dc] : m_DrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			if (!dc.IsRigged)
				Renderer::RenderSubmeshInstanced(m_CommandBuffer, m_GeometryPipeline, dc.Mesh, dc.MeshSource, dc.SubmeshIndex, dc.MaterialTable ? dc.MaterialTable : dc.Mesh->GetMaterials(), m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, 0, 0, dc.InstanceCount);
		}
		SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);

    #if 0
		{
			///< Render static meshes
			SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "Static Transparent Meshes");
			for (auto& [mk, dc] : m_TransparentStaticMeshDrawList)
			{
				const auto& transformData = m_MeshTransformMap.at(mk);
				Renderer::RenderStaticMesh(m_CommandBuffer, m_TransparentGeometryPipeline, dc.StaticMesh, dc.SubmeshIndex, dc.MaterialTable ? dc.MaterialTable : dc.StaticMesh->GetMaterials(), m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, dc.InstanceCount);
			}
			SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);

			///< Render dynamic meshes
			SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "Dynamic Transparent Meshes");
			for (auto& [mk, dc] : m_TransparentDrawList)
			{
				const auto& transformData = m_MeshTransformMap.at(mk);
				//Renderer::RenderSubmesh(m_CommandBuffer, m_GeometryPipeline, m_UniformBufferSet, m_StorageBufferSet, dc.Mesh, dc.SubmeshIndex, dc.MaterialTable ? dc.MaterialTable : dc.Mesh->GetMaterials(), dc.Transform);
				Renderer::RenderSubmeshInstanced(m_CommandBuffer, m_TransparentGeometryPipeline, dc.Mesh, dc.SubmeshIndex, dc.MaterialTable ? dc.MaterialTable : dc.Mesh->GetMaterials(), m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, 0, 0, dc.InstanceCount);
			}
			SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);
		}
    #endif
		Renderer::EndFrame(m_CommandBuffer);

		Renderer::BeginFrame(m_CommandBuffer, m_GeometryAnimPass);
		for (auto& [mk, dc] : m_DrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			if (dc.IsRigged)
			{
				const auto& boneTransformsData = m_MeshBoneTransformsMap.at(mk);
				Renderer::RenderSubmeshInstanced(m_CommandBuffer, m_GeometryPipelineAnim, dc.Mesh, dc.MeshSource, dc.SubmeshIndex, dc.MaterialTable ? dc.MaterialTable : dc.Mesh->GetMaterials(), m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, boneTransformsData.BoneTransformsBaseIndex, boneTransformsData.BoneTransformsStride, dc.InstanceCount);
			}
		}
		Renderer::EndFrame(m_CommandBuffer);
	}

	void SceneRenderer::PreConvolutionCompute()
	{
		SEDX_PROFILE_FUNC();

		///< TODO: Other techniques might need it in the future
		if (!m_Options.EnableSSR)
			return;

		struct PreConvolutionComputePushConstants
		{
			int PrevLod = 0;
			int Mode = 0; ///< 0 = Copy, 1 = GaussianHorizontal, 2 = GaussianVertical
		} preConvolutionComputePushConstants;

		///< Might change to be maximum res used by other techniques other than SSR
		int halfRes = static_cast<int>(m_SSROptions.HalfRes);

		m_GPUTimeQueries.PreConvolutionQuery = m_CommandBuffer->BeginTimestampQuery();

		UVec3 workGroups(0);

		Renderer::BeginComputePass(m_CommandBuffer, m_PreConvolutionComputePass);

		auto inputImage = m_SkyboxPass->GetOutput(0);
		workGroups = { (uint32_t)std::ceil((float)inputImage->GetWidth() / 16.0f), (uint32_t)std::ceil((float)inputImage->GetHeight() / 16.0f), 1 };
		Renderer::DispatchCompute(m_CommandBuffer, m_PreConvolutionComputePass, m_PreConvolutionMaterials[0], workGroups, Buffer(&preConvolutionComputePushConstants, sizeof(preConvolutionComputePushConstants)));

		const uint32_t mipCount = m_PreConvolutedTexture.Texture->GetMipLevelCount();
		for (uint32_t mip = 1; mip < mipCount; mip++)
		{
			Renderer::BeginGPUPerfMarker(m_CommandBuffer, std::format("Pre-Convolution-Mip({})", mip));

			auto [mipWidth, mipHeight] = m_PreConvolutedTexture.Texture->GetMipSize(mip);
			workGroups = { (uint32_t)std::ceil((float)mipWidth / 16.0f), (uint32_t)std::ceil((float)mipHeight / 16.0f), 1 };
			preConvolutionComputePushConstants.PrevLod = (int)mip - 1;

			auto blur = [&](const int mip, const int mode)
			{
				Renderer::BeginGPUPerfMarker(m_CommandBuffer, std::format("Pre-Convolution-Mode({})", mode));
				preConvolutionComputePushConstants.Mode = (int)mode;
				Renderer::DispatchCompute(m_CommandBuffer, m_PreConvolutionComputePass, m_PreConvolutionMaterials[mip], workGroups, Buffer(&preConvolutionComputePushConstants, sizeof(preConvolutionComputePushConstants)));
				Renderer::EndGPUPerfMarker(m_CommandBuffer);
			};

			blur(mip, 1); // Horizontal blur
			blur(mip, 2); // Vertical Blur

			Renderer::EndGPUPerfMarker(m_CommandBuffer);
		}

		Renderer::EndComputePass(m_CommandBuffer, m_PreConvolutionComputePass);
	}

	void SceneRenderer::GTAOCompute()
	{
		const Buffer pushConstantBuffer(&GTAODataCB, sizeof GTAODataCB);

		m_GPUTimeQueries.GTAOPassQuery = m_CommandBuffer->BeginTimestampQuery();
		Renderer::BeginComputePass(m_CommandBuffer, m_GTAOComputePass);
		Renderer::DispatchCompute(m_CommandBuffer, m_GTAOComputePass, nullptr, m_GTAOWorkGroups, pushConstantBuffer);
		Renderer::EndComputePass(m_CommandBuffer, m_GTAOComputePass);
		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.GTAOPassQuery);
	}

	void SceneRenderer::GTAODenoiseCompute()
	{
		m_GTAODenoiseConstants.DenoiseBlurBeta = GTAODataCB.DenoiseBlurBeta;
		m_GTAODenoiseConstants.HalfRes = GTAODataCB.HalfRes;
		const Buffer pushConstantBuffer(&m_GTAODenoiseConstants, sizeof(GTAODenoiseConstants));

		m_GPUTimeQueries.GTAODenoisePassQuery = m_CommandBuffer->BeginTimestampQuery();
		for (uint32_t pass = 0; std::cmp_less(pass, m_Options.GTAODenoisePasses); pass++)
		{
			auto renderPass = m_GTAODenoisePass[static_cast<uint32_t>(pass % 2 != 0)];
			Renderer::BeginComputePass(m_CommandBuffer, renderPass);
			Renderer::DispatchCompute(m_CommandBuffer, renderPass, nullptr, m_GTAODenoiseWorkGroups, pushConstantBuffer);
			Renderer::EndComputePass(m_CommandBuffer, renderPass);
		}
		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.GTAODenoisePassQuery);
	}

	void SceneRenderer::AOComposite()
	{
		// if (m_Options.EnableHBAO)
		//	m_AOCompositeMaterial->Set("u_HBAOTex", m_HBAOBlurPipelines[1]->GetSpecification().RenderPass->GetSpecification().dstFramebuffer->GetImage());
		m_GPUTimeQueries.AOCompositePassQuery = m_CommandBuffer->BeginTimestampQuery();
		Renderer::BeginFrame(m_CommandBuffer, m_AOCompositePass);
		Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_AOCompositePass->GetSpecification().Pipeline, m_AOCompositeMaterial);
		Renderer::EndFrame(m_CommandBuffer);
		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.AOCompositePassQuery);
	}

	void SceneRenderer::JumpFloodPass()
	{
		SEDX_PROFILE_FUNC();

		m_GPUTimeQueries.JumpFloodPassQuery = m_CommandBuffer->BeginTimestampQuery();
		Renderer::BeginFrame(m_CommandBuffer, m_JumpFloodInitPass);

		Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_JumpFloodInitPass->GetSpecification().Pipeline, m_JumpFloodInitMaterial);
		Renderer::EndFrame(m_CommandBuffer);

        constexpr int steps = 2;
		int step = (int)std::round(std::pow((float)(steps - 1), 2.0f));
		int index = 0;
		Buffer vertexOverrides;
		const Ref<Framebuffer> passFB = m_JumpFloodPass[0]->GetTargetFramebuffer();
		Vec2 texelSize = { 1.0f / (float)passFB->GetWidth(), 1.0f / (float)passFB->GetHeight() };
		vertexOverrides.Allocate(sizeof(Vec2) + sizeof(int));
		vertexOverrides.Write(&texelSize, sizeof(Vec2));
		while (step != 0)
		{
			vertexOverrides.Write(&step, sizeof(int), sizeof(Vec2));

			Renderer::BeginFrame(m_CommandBuffer, m_JumpFloodPass[index]);
			Renderer::SubmitFullscreenQuadWithOverrides(m_CommandBuffer, m_JumpFloodPass[index]->GetSpecification().Pipeline, m_JumpFloodPassMaterial[index], vertexOverrides, Buffer());
			Renderer::EndFrame(m_CommandBuffer);

			index = (index + 1) % 2;
			step /= 2;
		}

		vertexOverrides.Release();

		//m_JumpFloodCompositeMaterial->Set("u_Texture", m_TempFramebuffers[1]->GetImage());
		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.JumpFloodPassQuery);
	}

	void SceneRenderer::SSRCompute()
	{
		SEDX_PROFILE_FUNC();

		const Buffer pushConstantsBuffer(&m_SSROptions, sizeof(SSROptionsUB));

		m_GPUTimeQueries.SSRQuery = m_CommandBuffer->BeginTimestampQuery();
		Renderer::BeginComputePass(m_CommandBuffer, m_SSRPass);
		Renderer::DispatchCompute(m_CommandBuffer, m_SSRPass, nullptr, m_SSRWorkGroups, pushConstantsBuffer);
		Renderer::EndComputePass(m_CommandBuffer, m_SSRPass);
		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.SSRQuery);
	}

	void SceneRenderer::SSRCompositePass()
	{
		///< Currently scales the SSR, renders with transparency.
		///< The alpha channel is the confidence.

		m_GPUTimeQueries.SSRCompositeQuery = m_CommandBuffer->BeginTimestampQuery();
		Renderer::BeginFrame(m_CommandBuffer, m_SSRCompositePass);
		Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_SSRCompositePass->GetPipeline(), nullptr);
		Renderer::EndFrame(m_CommandBuffer);
		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.SSRCompositeQuery);
	}

	void SceneRenderer::BloomCompute()
	{
		UVec3 workGroups;

		struct BloomComputePushConstants
		{
			Vec4 Params;
			float LOD = 0.0f;
			int Mode = 0; ///< 0 = prefilter, 1 = downsample, 2 = firstUpsample, 3 = upsample
		} bloomComputePushConstants;
		bloomComputePushConstants.Params = { m_BloomSettings.Threshold, m_BloomSettings.Threshold - m_BloomSettings.Knee, m_BloomSettings.Knee * 2.0f, 0.25f / m_BloomSettings.Knee };
		bloomComputePushConstants.Mode = 0;

		m_GPUTimeQueries.BloomComputePassQuery = m_CommandBuffer->BeginTimestampQuery();

		Renderer::BeginComputePass(m_CommandBuffer, m_BloomComputePass);

		///< Prefilter
		Renderer::BeginGPUPerfMarker(m_CommandBuffer, "Bloom-Prefilter");
		{
			workGroups = { m_BloomComputeTextures[0].Texture->GetWidth() / m_BloomComputeWorkgroupSize, m_BloomComputeTextures[0].Texture->GetHeight() / m_BloomComputeWorkgroupSize, 1 };
			Renderer::DispatchCompute(m_CommandBuffer, m_BloomComputePass, m_BloomComputeMaterials.PrefilterMaterial, workGroups, Buffer(&bloomComputePushConstants, sizeof(bloomComputePushConstants)));
			m_BloomComputePipeline->ImageMemoryBarrier(m_CommandBuffer, m_BloomComputeTextures[0].Texture->GetImage(), ResourceAccessFlags::ShaderWrite, ResourceAccessFlags::ShaderRead);
		}
		Renderer::EndGPUPerfMarker(m_CommandBuffer);

		///< Image Downsample
		bloomComputePushConstants.Mode = 1;
		const uint32_t mips = m_BloomComputeTextures[0].Texture->GetMipLevelCount() - 2;
		Renderer::BeginGPUPerfMarker(m_CommandBuffer, "Bloom-DownSample");
		{
			for (uint32_t i = 1; i < mips; i++)
			{
				auto [mipWidth, mipHeight] = m_BloomComputeTextures[0].Texture->GetMipSize(i);
				workGroups = { (uint32_t)std::ceil((float)mipWidth / (float)m_BloomComputeWorkgroupSize) ,(uint32_t)std::ceil((float)mipHeight / (float)m_BloomComputeWorkgroupSize), 1 };

				bloomComputePushConstants.LOD = i - 1.0f;
				Renderer::DispatchCompute(m_CommandBuffer, m_BloomComputePass, m_BloomComputeMaterials.DownsampleAMaterials[i], workGroups, Buffer(&bloomComputePushConstants, sizeof(bloomComputePushConstants)));

				m_BloomComputePipeline->ImageMemoryBarrier(m_CommandBuffer, m_BloomComputeTextures[1].Texture->GetImage(), ResourceAccessFlags::ShaderWrite, ResourceAccessFlags::ShaderRead);

				bloomComputePushConstants.LOD = (float)i;
				Renderer::DispatchCompute(m_CommandBuffer, m_BloomComputePass, m_BloomComputeMaterials.DownsampleBMaterials[i], workGroups, Buffer(&bloomComputePushConstants, sizeof(bloomComputePushConstants)));

				m_BloomComputePipeline->ImageMemoryBarrier(m_CommandBuffer, m_BloomComputeTextures[0].Texture->GetImage(), ResourceAccessFlags::ShaderWrite, ResourceAccessFlags::ShaderRead);
			}
		}
		Renderer::EndGPUPerfMarker(m_CommandBuffer);


		///< Image First Upsample
		bloomComputePushConstants.Mode = 2;
		bloomComputePushConstants.LOD--;
		Renderer::BeginGPUPerfMarker(m_CommandBuffer, "Bloom-FirstUpsamle");
		{
			auto [mipWidth, mipHeight] = m_BloomComputeTextures[2].Texture->GetMipSize(mips - 2);
			workGroups.x = (uint32_t)std::ceil((float)mipWidth / (float)m_BloomComputeWorkgroupSize);
			workGroups.y = (uint32_t)std::ceil((float)mipHeight / (float)m_BloomComputeWorkgroupSize);

			Renderer::DispatchCompute(m_CommandBuffer, m_BloomComputePass, m_BloomComputeMaterials.FirstUpsampleMaterial, workGroups, Buffer(&bloomComputePushConstants, sizeof(bloomComputePushConstants)));
			m_BloomComputePipeline->ImageMemoryBarrier(m_CommandBuffer, m_BloomComputeTextures[2].Texture->GetImage(), ResourceAccessFlags::ShaderWrite, ResourceAccessFlags::ShaderRead);
		}
		Renderer::EndGPUPerfMarker(m_CommandBuffer);

		///< Image Upsample
		Renderer::BeginGPUPerfMarker(m_CommandBuffer, "Bloom-Upsample");
		{
			bloomComputePushConstants.Mode = 3;
			for (int32_t mip = mips - 3; mip >= 0; mip--)
			{
				auto [mipWidth, mipHeight] = m_BloomComputeTextures[2].Texture->GetMipSize(mip);
				workGroups.x = (uint32_t)std::ceil((float)mipWidth / (float)m_BloomComputeWorkgroupSize);
				workGroups.y = (uint32_t)std::ceil((float)mipHeight / (float)m_BloomComputeWorkgroupSize);

				bloomComputePushConstants.LOD = (float)mip;
				Renderer::DispatchCompute(m_CommandBuffer, m_BloomComputePass, m_BloomComputeMaterials.UpsampleMaterials[mip], workGroups, Buffer(&bloomComputePushConstants, sizeof(bloomComputePushConstants)));

				m_BloomComputePipeline->ImageMemoryBarrier(m_CommandBuffer, m_BloomComputeTextures[2].Texture->GetImage(), ResourceAccessFlags::ShaderWrite, ResourceAccessFlags::ShaderRead);
			}
		}
		Renderer::EndGPUPerfMarker(m_CommandBuffer);

		Renderer::EndComputePass(m_CommandBuffer, m_BloomComputePass);
		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.BloomComputePassQuery);
	}

	void SceneRenderer::EdgeDetectionPass() const
    {
		Renderer::BeginFrame(m_CommandBuffer, m_EdgeDetectionPass);
		Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_EdgeDetectionPass->GetPipeline(), nullptr);
		Renderer::EndFrame(m_CommandBuffer);
	}

	void SceneRenderer::CompositePass()
	{
		SEDX_PROFILE_FUNC();

		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		m_GPUTimeQueries.CompositePassQuery = m_CommandBuffer->BeginTimestampQuery();

		Renderer::BeginFrame(m_CommandBuffer, m_CompositePass);

		//auto framebuffer = m_GeometryPass->GetTargetFramebuffer();
		auto framebuffer = m_SkyboxPass->GetTargetFramebuffer();
		float exposure = m_SceneData.SceneCamera.Camera.GetExposure();
		int textureSamples = framebuffer->GetSpecification().samples;

		m_CompositeMaterial->Set("u_Uniforms.Exposure", exposure);
		if (m_BloomSettings.Enabled)
		{
			m_CompositeMaterial->Set("u_Uniforms.BloomIntensity", m_BloomSettings.Intensity);
			m_CompositeMaterial->Set("u_Uniforms.BloomDirtIntensity", m_BloomSettings.DirtIntensity);
		}
		else
		{
			m_CompositeMaterial->Set("u_Uniforms.BloomIntensity", 0.0f);
			m_CompositeMaterial->Set("u_Uniforms.BloomDirtIntensity", 0.0f);
		}

		m_CompositeMaterial->Set("u_Uniforms.Opacity", m_Opacity);
		m_CompositeMaterial->Set("u_Uniforms.Time", Application::Get().GetTime());

		SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "Composite");
		Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_CompositePass->GetPipeline(), m_CompositeMaterial);
		SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);

		Renderer::EndFrame(m_CommandBuffer);

		if (m_DOFSettings.Enabled)
		{
			//Renderer::EndRenderPass(m_CommandBuffer);

			Renderer::BeginFrame(m_CommandBuffer, m_DOFPass);
			//m_DOFMaterial->Set("u_Texture", m_CompositePipeline->GetSpecification().RenderPass->GetSpecification().dstFramebuffer->GetImage());
			//m_DOFMaterial->Set("u_DepthTexture", m_PreDepthPipeline->GetSpecification().RenderPass->GetSpecification().dstFramebuffer->GetDepthImage());
			m_DOFMaterial->Set("u_Uniforms.DOFParams", Vec2(m_DOFSettings.FocusDistance, m_DOFSettings.BlurSize));

			Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_DOFPass->GetPipeline(), m_DOFMaterial);

			//SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "JumpFlood-Composite");
			//Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_JumpFloodCompositePipeline, nullptr, m_JumpFloodCompositeMaterial);
			//SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);

			Renderer::EndFrame(m_CommandBuffer);

			///< Copy DOF image to composite pipeline
			Renderer::CopyImage(m_CommandBuffer,
				m_DOFPass->GetTargetFramebuffer()->GetImage(),
				m_CompositePass->GetTargetFramebuffer()->GetImage());

			///< WIP - will later be used for debugging/editor mouse picking
    #if 0
			if (m_ReadBackImage)
			{
				Renderer::CopyImage(m_CommandBuffer,
					m_DOFPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetImage(),
					m_ReadBackImage);

				{
					auto alloc = m_ReadBackImage.As<VulkanImage2D>()->GetImageInfo().MemoryAlloc;
					VulkanAllocator allocator("SceneRenderer");
					Vec4* mappedMem = allocator.MapMemory<Vec4>(alloc);
					delete[] m_ReadBackBuffer;
					m_ReadBackBuffer = new Vec4[m_ReadBackImage->GetWidth() * m_ReadBackImage->GetHeight()];
					memcpy(m_ReadBackBuffer, mappedMem, sizeof(Vec4) * m_ReadBackImage->GetWidth() * m_ReadBackImage->GetHeight());
					allocator.UnmapMemory(alloc);
				}
			}
    #endif
		}

		///< Grid
		if (GetOptions().ShowGrid)
		{
			Renderer::BeginFrame(m_CommandBuffer, m_GridRenderPass);
			const static Mat4 transform = Rotation::AxisAngleRadians({1.0f, 0.0f, 0.0f}, ToRadians(-90.0f)) * Mat4::Scale(Vec3(8.0f));
			SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "Grid");
			Renderer::RenderQuad(m_CommandBuffer, m_GridRenderPass->GetPipeline(), m_GridMaterial, transform);
			SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);
			Renderer::EndFrame(m_CommandBuffer);
		}

		///< TODO: don't do this in runtime!
		if (m_Specification.JumpFloodPass)
		{
			SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "JumpFlood-Composite");

			Renderer::BeginFrame(m_CommandBuffer, m_JumpFloodCompositePass);
			Renderer::SubmitFullscreenQuad(m_CommandBuffer, m_JumpFloodCompositePass->GetSpecification().Pipeline, m_JumpFloodCompositeMaterial);
			Renderer::EndFrame(m_CommandBuffer);

			SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);
		}

		m_CommandBuffer->EndTimestampQuery(m_GPUTimeQueries.CompositePassQuery);

		if (m_Options.ShowSelectedInWireframe)
		{
			Renderer::BeginFrame(m_CommandBuffer, m_GeometryWireframePass);

			SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "Static Meshes Wireframe");
			for (auto& [mk, dc] : m_SelectedStaticMeshDrawList)
			{
				const auto&[Transforms, TransformOffset] = m_MeshTransformMap.at(mk);
				Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, m_GeometryWireframePass->GetPipeline(), dc.StaticMesh, dc.MeshSource, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, TransformOffset, dc.InstanceCount, m_WireframeMaterial);
			}

			for (auto& [mk, dc] : m_SelectedMeshDrawList)
			{
				if (!dc.IsRigged)
				{
					const auto&[Transforms, TransformOffset] = m_MeshTransformMap.at(mk);
					Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_GeometryWireframePass->GetPipeline(), dc.Mesh, dc.MeshSource, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, TransformOffset + dc.InstanceOffset * sizeof(TransformVertexData), 0, 0, dc.InstanceCount, m_WireframeMaterial);
				}
			}

			SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);
			Renderer::EndFrame(m_CommandBuffer);

			Renderer::BeginFrame(m_CommandBuffer, m_GeometryWireframeAnimPass);
			SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "Dynamic Meshes Wireframe");
			for (auto& [mk, dc] : m_SelectedMeshDrawList)
			{
				const auto& transformData = m_MeshTransformMap.at(mk);
				if (dc.IsRigged)
				{
					const auto& boneTransformsData = m_MeshBoneTransformsMap.at(mk);
					Renderer::RenderMeshWithMaterial(m_CommandBuffer, m_GeometryWireframeAnimPass->GetPipeline(), dc.Mesh, dc.MeshSource, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset + dc.InstanceOffset * sizeof(TransformVertexData), boneTransformsData.BoneTransformsBaseIndex + dc.InstanceOffset * boneTransformsData.BoneTransformsStride, boneTransformsData.BoneTransformsStride, dc.InstanceCount, m_WireframeMaterial);
				}
			}
			SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);

			Renderer::EndFrame(m_CommandBuffer);
		}

		if (m_Options.ShowPhysicsColliders)
		{
			auto staticPass = m_Options.ShowPhysicsCollidersOnTop ? m_GeometryWireframeOnTopPass : m_GeometryWireframePass;
			auto animPass = m_Options.ShowPhysicsCollidersOnTop? m_GeometryWireframeOnTopAnimPass : m_GeometryWireframeAnimPass;

			SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "Static Meshes Collider");
			Renderer::BeginFrame(m_CommandBuffer, staticPass);
			for (auto& [mk, dc] : m_StaticColliderDrawList)
			{
				SEDX_CORE_VERIFY(m_MeshTransformMap.contains(mk));
				const auto& transformData = m_MeshTransformMap.at(mk);
				Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, staticPass->GetPipeline(), dc.StaticMesh, dc.MeshSource, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, dc.InstanceCount, dc.OverrideMaterial? dc.OverrideMaterial : m_SimpleColliderMaterial);
			}

			for (auto& [mk, dc] : m_ColliderDrawList)
			{
				SEDX_CORE_VERIFY(m_MeshTransformMap.contains(mk));
				const auto& transformData = m_MeshTransformMap.at(mk);
				if (!dc.IsRigged)
					Renderer::RenderMeshWithMaterial(m_CommandBuffer, staticPass->GetPipeline(), dc.Mesh, dc.MeshSource, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, 0, 0, dc.InstanceCount, dc.OverrideMaterial ? dc.OverrideMaterial : m_SimpleColliderMaterial);
			}

			Renderer::EndFrame(m_CommandBuffer);
			SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);

			SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "Animated Meshes Collider");
			Renderer::BeginFrame(m_CommandBuffer, animPass);
			for (auto& [mk, dc] : m_ColliderDrawList)
			{
				SEDX_CORE_VERIFY(m_MeshTransformMap.contains(mk));
				const auto& transformData = m_MeshTransformMap.at(mk);
				if (dc.IsRigged)
				{
					const auto& boneTransformsData = m_MeshBoneTransformsMap.at(mk);
					Renderer::RenderMeshWithMaterial(m_CommandBuffer, animPass->GetPipeline(), dc.Mesh, dc.MeshSource, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, boneTransformsData.BoneTransformsBaseIndex, boneTransformsData.BoneTransformsStride, dc.InstanceCount, m_SimpleColliderMaterial);
				}
				else
				{
					Renderer::RenderMeshWithMaterial(m_CommandBuffer, animPass->GetPipeline(), dc.Mesh, dc.MeshSource, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, 0, 0, dc.InstanceCount, m_SimpleColliderMaterial);
				}
			}

			Renderer::EndFrame(m_CommandBuffer);
			SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);
		}

		if (m_Options.ShowAnimationDebug)
		{
			SceneRenderer::BeginGPUPerfMarker(m_CommandBuffer, "Animation Debug");
			Renderer::BeginFrame(m_CommandBuffer, m_GeometryWireframeOnTopPass);
			for (auto& [mk, dc] : m_AnimationDebugDrawList)
			{
				const auto&[Transforms, TransformOffset] = m_MeshTransformMap.at(mk);
				Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, m_GeometryWireframeOnTopPass->GetPipeline(), dc.StaticMesh, dc.MeshSource, dc.SubmeshIndex, m_SubmeshTransformBuffers[frameIndex].Buffer, TransformOffset, dc.InstanceCount, dc.OverrideMaterial);
			}
			Renderer::EndFrame(m_CommandBuffer);
			SceneRenderer::EndGPUPerfMarker(m_CommandBuffer);
		}
	}

	void SceneRenderer::FlushDrawList()
	{
		if (m_ResourcesCreated && m_ViewportWidth > 0 && m_ViewportHeight > 0)
		{
            ///< Reset GPU time queries
			m_GPUTimeQueries = SceneRenderer::GPUTimeQueries();

			PreRender();

			m_CommandBuffer->Begin();

			///< Main render passes
			ShadowMapPass();
			SpotShadowMapPass();
			PreDepthPass();
			Compute();
			PreIntegration();
			LightCullingPass();
			SkyboxPass();
			GeometryPass();


			///< GTAO
			if (m_Options.EnableGTAO)
			{
				GTAOCompute();
				GTAODenoiseCompute();
			}

			if (m_Options.EnableGTAO)
				AOComposite();

			PreConvolutionCompute();

			///< Post-processing
			if (m_Specification.JumpFloodPass)
				JumpFloodPass();

			///< SSR
			if (m_Options.EnableSSR)
			{
				SSRCompute();
				SSRCompositePass();
			}

			if (m_Specification.EnableEdgeOutlineEffect)
				EdgeDetectionPass();

			BloomCompute();
			CompositePass();

			m_CommandBuffer->End();
			m_CommandBuffer->Submit();
		}
		else
		{
			///< Empty pass
			m_CommandBuffer->Begin();

			ClearPass();

			m_CommandBuffer->End();
			m_CommandBuffer->Submit();
		}

		UpdateStatistics();

		m_DrawList.clear();
		m_TransparentDrawList.clear();
		m_SelectedMeshDrawList.clear();
		m_ShadowPassDrawList.clear();
		m_StaticMeshDrawList.clear();
		m_TransparentStaticMeshDrawList.clear();
		m_SelectedStaticMeshDrawList.clear();
		m_StaticMeshShadowPassDrawList.clear();
		m_AnimationDebugDrawList.clear();
		m_ColliderDrawList.clear();
		m_StaticColliderDrawList.clear();
		m_SceneData = {};
		m_MeshTransformMap.clear();
		m_MeshBoneTransformsMap.clear();
	}

	void SceneRenderer::PreRender()
	{
		SEDX_PROFILE_FUNC();

		const uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		uint32_t offset = 0;
		for (auto &transformData : m_MeshTransformMap | std::views::values)
		{
			transformData.TransformOffset = offset * sizeof(TransformVertexData);
			for (const auto& transform : transformData.Transforms)
			{
				m_SubmeshTransformBuffers[frameIndex].Data[offset] = transform;
				offset++;
			}

		}

		m_SubmeshTransformBuffers[frameIndex].Buffer->SetData(m_SubmeshTransformBuffers[frameIndex].Data, offset * sizeof(TransformVertexData));

		uint32_t index = 0;
		for (auto &boneTransformsData : m_MeshBoneTransformsMap | std::views::values)
		{
			boneTransformsData.BoneTransformsBaseIndex = index;
			memcpy(&m_BoneTransformsData[index], boneTransformsData.BoneTransformsData.data(), boneTransformsData.BoneTransformsData.size() * sizeof(Mat4));
			index += static_cast<uint32_t>(boneTransformsData.BoneTransformsData.size());
		}

		if (index > 0)
		{
            Ref<SceneRenderer> instance(this);
			Renderer::Submit([instance, index]() mutable
			{
				instance->m_SBSBoneTransforms->GetRenderThread()->SetRenderThreadData(instance->m_BoneTransformsData, static_cast<uint32_t>(index * sizeof(Mat4)));
			});
		}
	}

	void SceneRenderer::CopyToBoneTransformStorage(const MeshKey& meshKey, const Ref<MeshSource>& meshSource, const std::vector<Mat4>& boneTransforms)
	{
		auto& boneTransformsMap = m_MeshBoneTransformsMap[meshKey];
		size_t boneCount = boneTransformsMap.BoneTransformsData.size();
		size_t stride = meshSource->m_BoneInfo.size();

		boneTransformsMap.BoneTransformsStride = static_cast<uint32_t>(stride);
		boneTransformsMap.BoneTransformsData.reserve(boneCount + stride);

		if (boneTransforms.empty())
		{
			std::fill_n(std::back_inserter(boneTransformsMap.BoneTransformsData), stride, Mat4(1.0f));
		}
		else
		{
			for (size_t i = 0; i < stride; ++i)
                boneTransformsMap.BoneTransformsData.push_back(boneTransforms[meshSource->m_BoneInfo[i].BoneIndex] * meshSource->m_BoneInfo[i].InverseBindPose);
        }
	}

	void SceneRenderer::CreateBloomPassMaterials()
	{
		const auto inputImage = m_SkyboxPass->GetOutput(0);

		///< Prefilter
		m_BloomComputeMaterials.PrefilterMaterial = Material::Create(m_BloomComputePass->GetShader());
		m_BloomComputeMaterials.PrefilterMaterial->Set("o_Image", m_BloomComputeTextures[0].ImageViews[0]);
		m_BloomComputeMaterials.PrefilterMaterial->Set("u_Texture", inputImage);
		m_BloomComputeMaterials.PrefilterMaterial->Set("u_BloomTexture", inputImage);

		///< Downsample
		const uint32_t mips = m_BloomComputeTextures[0].Texture->GetMipLevelCount() - 2;
		m_BloomComputeMaterials.DownsampleAMaterials.resize(mips);
		m_BloomComputeMaterials.DownsampleBMaterials.resize(mips);
		for (uint32_t i = 1; i < mips; i++)
		{
			m_BloomComputeMaterials.DownsampleAMaterials[i] = Material::Create(m_BloomComputePass->GetShader());
			m_BloomComputeMaterials.DownsampleAMaterials[i]->Set("o_Image", m_BloomComputeTextures[1].ImageViews[i]);
			m_BloomComputeMaterials.DownsampleAMaterials[i]->Set("u_Texture", m_BloomComputeTextures[0].Texture);
			m_BloomComputeMaterials.DownsampleAMaterials[i]->Set("u_BloomTexture", inputImage);

			m_BloomComputeMaterials.DownsampleBMaterials[i] = Material::Create(m_BloomComputePass->GetShader());
			m_BloomComputeMaterials.DownsampleBMaterials[i]->Set("o_Image", m_BloomComputeTextures[0].ImageViews[i]);
			m_BloomComputeMaterials.DownsampleBMaterials[i]->Set("u_Texture", m_BloomComputeTextures[1].Texture);
			m_BloomComputeMaterials.DownsampleBMaterials[i]->Set("u_BloomTexture", inputImage);
		}

		///< Upsample
		m_BloomComputeMaterials.FirstUpsampleMaterial = Material::Create(m_BloomComputePass->GetShader());
		m_BloomComputeMaterials.FirstUpsampleMaterial->Set("o_Image", m_BloomComputeTextures[2].ImageViews[mips - 2]);
		m_BloomComputeMaterials.FirstUpsampleMaterial->Set("u_Texture", m_BloomComputeTextures[0].Texture);
		m_BloomComputeMaterials.FirstUpsampleMaterial->Set("u_BloomTexture", inputImage);

		m_BloomComputeMaterials.UpsampleMaterials.resize(mips - 3 + 1);
		for (int32_t mip = mips - 3; mip >= 0; mip--)
		{
			m_BloomComputeMaterials.UpsampleMaterials[mip] = Material::Create(m_BloomComputePass->GetShader());
			m_BloomComputeMaterials.UpsampleMaterials[mip]->Set("o_Image", m_BloomComputeTextures[2].ImageViews[mip]);
			m_BloomComputeMaterials.UpsampleMaterials[mip]->Set("u_Texture", m_BloomComputeTextures[0].Texture);
			m_BloomComputeMaterials.UpsampleMaterials[mip]->Set("u_BloomTexture", m_BloomComputeTextures[2].Texture);
		}
	}

	void SceneRenderer::CreatePreConvolutionPassMaterials()
	{
		const auto inputImage = m_SkyboxPass->GetOutput(0);

		const uint32_t mips = m_PreConvolutedTexture.Texture->GetMipLevelCount();
		m_PreConvolutionMaterials.resize(mips);

		for (uint32_t i = 0; i < mips; i++)
		{
			m_PreConvolutionMaterials[i] = Material::Create(m_PreConvolutionComputePass->GetShader());
			m_PreConvolutionMaterials[i]->Set("o_Image", m_PreConvolutedTexture.ImageViews[i]);
			m_PreConvolutionMaterials[i]->Set("u_Input", i == 0 ? inputImage : m_PreConvolutedTexture.Texture->GetImage());
		}

	}

	void SceneRenderer::CreateXPlaneMaterialsPass()
	{
		constexpr uint32_t maxMipBatchSize = 4;
		const uint32_t hzbMipCount = m_HierarchicalDepthTexture.Texture->GetMipLevelCount();

		const Ref<Shader> hzbShader = Renderer::GetShaderLibrary()->Get("HZB");

		uint32_t materialIndex = 0;
		m_HZBMaterials.resize(Utils::Math::DivideAndRoundUp(hzbMipCount, 4u));
		for (uint32_t startDestMip = 0; startDestMip < hzbMipCount; startDestMip += maxMipBatchSize)
		{
            const Ref<Material> material = CreateRef<Material>(hzbShader);
			m_HZBMaterials[materialIndex++] = material;

			if (startDestMip == 0)
				material->Set("u_InputDepth", m_PreDepthPass->GetDepthOutput());
			else
				material->Set("u_InputDepth", m_HierarchicalDepthTexture.Texture);

			const uint32_t endDestMip = Utils::Math::Min(startDestMip + maxMipBatchSize, hzbMipCount);
			uint32_t destMip;
			for (destMip = startDestMip; destMip < endDestMip; ++destMip)
			{
				uint32_t index = destMip - startDestMip;
				material->Set("o_HZB", m_HierarchicalDepthTexture.ImageViews[destMip], index);
			}

			///< Fill the rest .. or we could enable the null descriptor feature
			destMip -= startDestMip;
			for (; destMip < maxMipBatchSize; ++destMip)
			{
				material->Set("o_HZB", m_HierarchicalDepthTexture.ImageViews[hzbMipCount - 1], destMip); ///< could be white texture?
			}
		}
	}

	void SceneRenderer::CreatePreIntegrationPassMaterials()
	{
		const Ref<Shader> preIntegrationShader = Renderer::GetShaderLibrary()->Get("Pre-Integration");

		const Ref<Texture2D> visibilityTexture = m_PreIntegrationVisibilityTexture.Texture;
		m_PreIntegrationMaterials.resize(visibilityTexture->GetMipLevelCount() - 1);
		for (uint32_t i = 0; i < visibilityTexture->GetMipLevelCount() - 1; i++)
		{
            const Ref<Material> material = CreateRef<Material>(preIntegrationShader);
			m_PreIntegrationMaterials[i] = material;

			material->Set("o_VisibilityImage", m_PreIntegrationVisibilityTexture.ImageViews[i]);
			material->Set("u_VisibilityTex", visibilityTexture);
			material->Set("u_HZB", m_HierarchicalDepthTexture.Texture);
		}
	}

	void SceneRenderer::ClearPass() const
    {
		SEDX_PROFILE_FUNC();

		Renderer::BeginFrame(m_CommandBuffer, m_PreDepthPass, true);
		Renderer::EndFrame(m_CommandBuffer);

		Renderer::BeginFrame(m_CommandBuffer, m_CompositePass, true);
		Renderer::EndFrame(m_CommandBuffer);

		//Renderer::BeginFrame(m_CommandBuffer, m_DOFPipeline->GetSpecification().RenderPass, true);
		//Renderer::EndFrame(m_CommandBuffer);
	}

	Ref<Pipeline> SceneRenderer::GetFinalPipeline() const
    {
		return m_CompositePass->GetSpecification().Pipeline;
	}

	Ref<RenderPass> SceneRenderer::GetFinalRenderPass()
	{
		return m_CompositePass;
	}

	Ref<Image2D> SceneRenderer::GetFinalPassImage() const
    {
		if (!m_ResourcesCreated)
			return nullptr;

        //return GetFinalPipeline()->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetImage();
		return m_CompositePass->GetOutput(0);
	}

	SceneRendererOptions& SceneRenderer::GetOptions()
	{
		return m_Options;
	}

	void SceneRenderer::CalculateCascades(CascadeData* cascades, const SceneRendererCamera& sceneCamera, const Vec3& lightDirection) const
	{
        ///< TODO: Reversed Z projection?

		float scaleToOrigin = m_ScaleShadowCascadesToOrigin;

		Mat4 viewMatrix = sceneCamera.ViewMatrix;
		constexpr Vec4 origin = Vec4(Vec3(0.0f), 1.0f);
		viewMatrix[3] = viewMatrix[3] + (origin - viewMatrix[3]) * scaleToOrigin;

		auto viewProjection = sceneCamera.Camera.GetUnReversedProjectionMatrix() * viewMatrix;

        constexpr int SHADOW_MAP_CASCADE_COUNT = 4;
		float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];

		float nearClip = sceneCamera.Near;
		float farClip = sceneCamera.Far;
		float clipRange = farClip - nearClip;

		float minZ = nearClip;
		float maxZ = nearClip + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		///< Calculate split depths based on view camera frustum
		///< Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
		{
			float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
			float log = minZ * std::pow(ratio, p);
			float uniform = minZ + range * p;
			float d = CascadeSplitLambda * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
		}

		cascadeSplits[3] = 0.3f;

		///< Manually set cascades here
		///< cascadeSplits[0] = 0.05f;
		///< cascadeSplits[1] = 0.15f;
		///< cascadeSplits[2] = 0.3f;
		///< cascadeSplits[3] = 1.0f;

		///< Calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
		{
			float splitDist = cascadeSplits[i];

			Vec3 frustumCorners[8] =
			{
				Vec3(-1.0f,  1.0f, -1.0f),
				Vec3(1.0f,  1.0f, -1.0f),
				Vec3(1.0f, -1.0f, -1.0f),
				Vec3(-1.0f, -1.0f, -1.0f),
				Vec3(-1.0f,  1.0f,  1.0f),
				Vec3(1.0f,  1.0f,  1.0f),
				Vec3(1.0f, -1.0f,  1.0f),
				Vec3(-1.0f, -1.0f,  1.0f),
			};

			///< Project frustum corners into world space
			Mat4 invCam = viewProjection.GetInverse();
			for (auto &frustumCorner : frustumCorners)
            {
				Vec4 invCorner = invCam * Vec4(frustumCorner, 1.0f);
                frustumCorner = invCorner / invCorner.w;
			}

			for (uint32_t i = 0; i < 4; i++)
			{
				Vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
			}

			///< Get frustum center
			Vec3 frustumCenter = Vec3(0.0f);
			for (auto frustumCorner : frustumCorners)
                frustumCenter += frustumCorner;

			frustumCenter /= 8.0f;

			///<frustumCenter *= 0.01f;

			float radius = 0.0f;
			for (auto frustumCorner : frustumCorners)
            {
				float distance = Length(frustumCorner - frustumCenter);
				radius = Utils::Math::Max(radius, distance);
			}
			radius = std::ceil(radius * 16.0f) / 16.0f;

			Vec3 maxExtents = Vec3(radius);
			Vec3 minExtents = -maxExtents;

			Vec3 lightDir = -lightDirection;
			Mat4 lightViewMatrix = Mat4::LookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, Vec3(0.0f, 0.0f, 1.0f));
			Mat4 lightOrthoMatrix = ::SceneryEditorX::Ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f + CascadeNearPlaneOffset, maxExtents.z - minExtents.z + CascadeFarPlaneOffset);

			///< Offset to texel space to avoid shimmering (from https://stackoverflow.com/questions/33499053/cascaded-shadow-map-shimmering)
			Mat4 shadowMatrix = lightOrthoMatrix * lightViewMatrix;
			float ShadowMapResolution = (float)m_DirectionalShadowMapPass[0]->GetTargetFramebuffer()->GetWidth();

			Vec4 shadowOrigin = (shadowMatrix * Vec4(0.0f, 0.0f, 0.0f, 1.0f)) * ShadowMapResolution / 2.0f;
			Vec4 roundedOrigin = { std::round(shadowOrigin.x), std::round(shadowOrigin.y), std::round(shadowOrigin.z), std::round(shadowOrigin.w) };
			Vec4 roundOffset = roundedOrigin - shadowOrigin;
			roundOffset = roundOffset * 2.0f / ShadowMapResolution;
			roundOffset.z = 0.0f;
			roundOffset.w = 0.0f;

			lightOrthoMatrix[3] += roundOffset;

			///< Store split distance and matrix in cascade
			cascades[i].SplitDepth = (nearClip + splitDist * clipRange) * -1.0f;
			cascades[i].ViewProj = lightOrthoMatrix * lightViewMatrix;
			cascades[i].View = lightViewMatrix;

			lastSplitDist = cascadeSplits[i];
		}
	}

	void SceneRenderer::CalculateCascadesManualSplit(CascadeData* cascades, const SceneRendererCamera& sceneCamera, const Vec3& lightDirection) const
	{
        ///<TODO: Reversed Z projection?

		float scaleToOrigin = m_ScaleShadowCascadesToOrigin;

		Mat4 viewMatrix = sceneCamera.ViewMatrix;
		constexpr Vec4 origin = Vec4(Vec3(0.0f), 1.0f);
		viewMatrix[3] = viewMatrix[3] + (origin - viewMatrix[3]) * scaleToOrigin;

		auto viewProjection = sceneCamera.Camera.GetUnReversedProjectionMatrix() * viewMatrix;

        constexpr int SHADOW_MAP_CASCADE_COUNT = 4;

		float nearClip = sceneCamera.Near;
		float farClip = sceneCamera.Far;
		float clipRange = farClip - nearClip;

		float minZ = nearClip;
		float maxZ = nearClip + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		///< Calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
		{
			float splitDist = m_ShadowCascadeSplits[0];
			lastSplitDist = 0.0;

			Vec3 frustumCorners[8] =
			{
				Vec3(-1.0f,  1.0f, -1.0f),
				Vec3(1.0f,  1.0f, -1.0f),
				Vec3(1.0f, -1.0f, -1.0f),
				Vec3(-1.0f, -1.0f, -1.0f),
				Vec3(-1.0f,  1.0f,  1.0f),
				Vec3(1.0f,  1.0f,  1.0f),
				Vec3(1.0f, -1.0f,  1.0f),
				Vec3(-1.0f, -1.0f,  1.0f),
			};

			///< Project frustum corners into world space
			Mat4 invCam = viewProjection.GetInverse();
			for (auto &frustumCorner : frustumCorners)
            {
				Vec4 invCorner = invCam * Vec4(frustumCorner, 1.0f);
                frustumCorner = invCorner / invCorner.w;
			}

			for (uint32_t i = 0; i < 4; i++)
			{
				Vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
			}

			///< Get frustum center
			Vec3 frustumCenter = Vec3(0.0f);
			for (auto frustumCorner : frustumCorners)
                frustumCenter += frustumCorner;

			frustumCenter /= 8.0f;

			///<frustumCenter *= 0.01f;

			float radius = 0.0f;
			for (auto frustumCorner : frustumCorners)
            {
				float distance = Length(frustumCorner - frustumCenter);
				radius = Utils::Math::Max(radius, distance);
			}
			radius = std::ceil(radius * 16.0f) / 16.0f;
			radius *= m_ShadowCascadeSplits[1];

			Vec3 maxExtents = Vec3(radius);
			Vec3 minExtents = -maxExtents;

			Vec3 lightDir = -lightDirection;
			Mat4 lightViewMatrix = Mat4::LookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, Vec3(0.0f, 0.0f, 1.0f));
			Mat4 lightOrthoMatrix = ::SceneryEditorX::Ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f + CascadeNearPlaneOffset, maxExtents.z - minExtents.z + CascadeFarPlaneOffset);

			///< Offset to texel space to avoid shimmering (from https://stackoverflow.com/questions/33499053/cascaded-shadow-map-shimmering)
			Mat4 shadowMatrix = lightOrthoMatrix * lightViewMatrix;
			float ShadowMapResolution = (float)m_DirectionalShadowMapPass[0]->GetTargetFramebuffer()->GetWidth();
			Vec4 shadowOrigin = (shadowMatrix * Vec4(0.0f, 0.0f, 0.0f, 1.0f)) * ShadowMapResolution / 2.0f;
			Vec4 roundedOrigin = { std::round(shadowOrigin.x), std::round(shadowOrigin.y), std::round(shadowOrigin.z), std::round(shadowOrigin.w) };
			Vec4 roundOffset = roundedOrigin - shadowOrigin;
			roundOffset = roundOffset * 2.0f / ShadowMapResolution;
			roundOffset.z = 0.0f;
			roundOffset.w = 0.0f;

			lightOrthoMatrix[3] += roundOffset;

			///< Store split distance and matrix in cascade
			cascades[i].SplitDepth = (nearClip + splitDist * clipRange) * -1.0f;
			cascades[i].ViewProj = lightOrthoMatrix * lightViewMatrix;
			cascades[i].View = lightViewMatrix;

			lastSplitDist = m_ShadowCascadeSplits[0];
		}
	}

	void SceneRenderer::UpdateStatistics()
	{
		m_Statistics.DrawCalls = 0;
		m_Statistics.Instances = 0;
		m_Statistics.Meshes = 0;

		for (auto &dc : m_SelectedStaticMeshDrawList | std::views::values)
		{
			m_Statistics.Instances += dc.InstanceCount;
			m_Statistics.DrawCalls++;
			m_Statistics.Meshes++;
		}

		for (auto &dc : m_StaticMeshDrawList | std::views::values)
		{
			m_Statistics.Instances += dc.InstanceCount;
			m_Statistics.DrawCalls++;
			m_Statistics.Meshes++;
		}

		for (auto &dc : m_SelectedMeshDrawList | std::views::values)
		{
			m_Statistics.Instances += dc.InstanceCount;
			m_Statistics.DrawCalls++;
			m_Statistics.Meshes++;
		}

		for (auto &dc : m_DrawList | std::views::values)
		{
			m_Statistics.Instances += dc.InstanceCount;
			m_Statistics.DrawCalls++;
			m_Statistics.Meshes++;
		}

		m_Statistics.SavedDraws = m_Statistics.Instances - m_Statistics.DrawCalls;

		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		m_Statistics.TotalGPUTime = m_CommandBuffer->GetExecutionGPUTime(frameIndex);
	}

	void SceneRenderer::SetLineWidth(const float width)
	{
		m_LineWidth = width;

		if (m_GeometryWireframePass)
			m_GeometryWireframePass->GetPipeline()->GetSpecification().lineWidth = width;
		if (m_GeometryWireframeOnTopPass)
			m_GeometryWireframeOnTopPass->GetPipeline()->GetSpecification().lineWidth = width;
		if (m_GeometryWireframeAnimPass)
			m_GeometryWireframeAnimPass->GetPipeline()->GetSpecification().lineWidth = width;
		if (m_GeometryWireframeOnTopAnimPass)
			m_GeometryWireframeOnTopAnimPass->GetPipeline()->GetSpecification().lineWidth = width;

		if (m_Renderer2D)
			m_Renderer2D->SetLineWidth(width);
	}
	*/

}

/// -------------------------------------------------------

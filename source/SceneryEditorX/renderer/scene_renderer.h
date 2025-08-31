/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* scene_renderer.h
* -------------------------------------------------------
* Created: 23/7/2025
* -------------------------------------------------------
*/
#pragma once
/*#include <cstdint>
#include <map>

#include <Math/includes/matrix.h>
#include <Math/includes/vector.h>

#include "SceneryEditorX/scene/components.h"
#include "SceneryEditorX/scene/scene.h"

#include "2d_renderer.h"
#include "camera.h"
#include "compute_pipeline.h"
#include "debug_renderer.h"
#include "texture.h"
#include "SceneryEditorX/asset/mesh/mesh.h"
#include "SceneryEditorX/project/project_settings.h"
#include "SceneryEditorX/scene/material.h"
#include "buffers/framebuffer.h"
#include "buffers/vertex_buffer.h"
#include "shaders/shader_definitions.h"
#include "vulkan/vk_image.h"
#include "vulkan/vk_render_pass.h"*/

/// -------------------------------------------------------

namespace SceneryEditorX
{

	/*
	class ComputePass; // forward declaration

	struct SceneRendererOptions
	{
		bool ShowGrid = true;
		bool ShowSelectedInWireframe = false;
		bool ShowAnimationDebug = false;
		Vec4 BoneColor = { 0.325f, 0.909f, 0.996f, 1.0f };
		Vec4 SelectedBoneColor = { 1.0f, 0.5f, 0.0f, 1.0f };

		enum class PhysicsColliderView : uint8_t
		{
			SelectedEntity = 0,
		    All = 1
		};

		bool ShowPhysicsColliders = false;
		PhysicsColliderView PhysicsColliderMode = PhysicsColliderView::SelectedEntity;
		bool ShowPhysicsCollidersOnTop = false;
		Vec4 SimplePhysicsCollidersColor = { 0.2f, 1.0f, 0.2f, 1.0f };
		Vec4 ComplexPhysicsCollidersColor = { 0.5f, 0.5f, 1.0f, 1.0f };

		///< General AO
		float AOShadowTolerance = 1.0f;

		///< GTAO
		bool EnableGTAO = true;
		bool GTAOBentNormals = false;
		int GTAODenoisePasses = 4;

	    ///< SSR
		bool EnableSSR = false;
		ShaderSpecs::AOMethod ReflectionOcclusionMethod = ShaderSpecs::AOMethod::None;
	};

    /// -------------------------------------------------------

	struct SSROptionsUB
	{
		///< Screen Space Reflections
		Vec2 BUvFactor;
		Vec2 FadeIn = {0.1f, 0.15f };
		float Brightness = 0.7f;
		float DepthTolerance = 0.8f;
		float FacingReflectionsFading = 0.1f;
		int MaxSteps = 70;
		uint32_t NumDepthMips;
		float RoughnessDepthTolerance = 1.0f;
		bool HalfRes = true;
		char Padding[3] {0, 0, 0 };
		bool EnableConeTracing = true;
		char Padding1[3] {0, 0, 0 };
		float LuminanceFactor = 1.0f;
	};

    /// -------------------------------------------------------

	struct SceneRendererCamera
	{
		Camera camera;
		Mat4 ViewMatrix;
        float Near;
	    float Far; ///<Non-reversed
		float FOV;
	};

    /// -------------------------------------------------------

	struct BloomSettings
	{
		bool Enabled = true;
		float Threshold = 1.0f;
		float Knee = 0.1f;
		float UpsampleScale = 1.0f;
		float Intensity = 1.0f;
		float DirtIntensity = 1.0f;
	};

    /// -------------------------------------------------------

	struct CBGTAOData
	{
		Vec2 NDCToViewMul_x_PixelSize;
		float EffectRadius = 0.5f;
		float EffectFalloffRange = 0.62f;

		float RadiusMultiplier = 1.46f;
		float FinalValuePower = 2.2f;
		float DenoiseBlurBeta = 1.2f;
		bool HalfRes = false;
		char Padding0[3]{ 0, 0, 0 };

		float SampleDistributionPower = 2.0f;
		float ThinOccluderCompensation = 0.0f;
		float DepthMIPSamplingOffset = 3.3f;
		int NoiseIndex = 0;

		Vec2 BUVFactor;
		float ShadowTolerance;
		float Padding;
	};

    /// -------------------------------------------------------

	struct DOFSettings
	{
		bool Enabled = false;
		float FocusDistance = 0.0f;
		float BlurSize = 1.0f;
	};

    /// -------------------------------------------------------

	struct SceneRendererSpecification
	{
		Tiering::Renderer::RendererTieringSettings Tiering;
		uint32_t NumShadowCascades = 4;

		bool EnableEdgeOutlineEffect = false;
		bool JumpFloodPass = true;

		uint32_t ViewportWidth = 0, ViewportHeight = 0; ///< 0 means application window size
	};
	*/

    /// -------------------------------------------------------

	/*
	class SceneRenderer : public RefCounted
	{
	public:
		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t Meshes = 0;
			uint32_t Instances = 0;
			uint32_t SavedDraws = 0;

			float TotalGPUTime = 0.0f;
		};

        SceneRenderer(const Ref<Scene> &scene, const SceneRendererSpecification &specification = SceneRendererSpecification());
		~SceneRenderer();

		void Init();
		void Shutdown() const;
        void BeginScene(const SceneRendererCamera &camera);
        void EndScene();

		///< Should only be called at initialization.
		void InitOptions();

		void SetScene(const Ref<Scene> &scene);
		void SetViewportSize(uint32_t width, uint32_t height);

        void UpdateGTAOData();
		static void InsertGPUPerfMarker(Ref<CommandBuffer> renderCommandBuffer, const std::string& label, const Vec4& markerColor = {});
		static void BeginGPUPerfMarker(Ref<CommandBuffer> renderCommandBuffer, const std::string& label, const Vec4& markerColor = {});
		static void EndGPUPerfMarker(const Ref<CommandBuffer> &renderCommandBuffer);

		void SubmitMesh(Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<MaterialTable> materialTable, const Mat4& transform = Mat4(1.0f), const std::vector<Mat4>& boneTransforms = {}, Ref<Material> overrideMaterial = nullptr);
		void SubmitStaticMesh(Ref<StaticMesh> staticMesh, Ref<MeshSource> meshSource, Ref<MaterialTable> materialTable, const Mat4& transform = Mat4(1.0f), Ref<Material> overrideMaterial = nullptr);

		void SubmitSelectedMesh(Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<MaterialTable> materialTable, const Mat4& transform = Mat4(1.0f), const std::vector<Mat4>& boneTransforms = {}, Ref<Material> overrideMaterial = nullptr);
		void SubmitSelectedStaticMesh(Ref<StaticMesh> staticMesh, Ref<MeshSource> meshSource, const Ref<MaterialTable> &materialTable, const Mat4& transform = Mat4(1.0f), const Ref<Material> &overrideMaterial = nullptr);

		void SubmitAnimationDebugMesh(const Mat4& transform = Mat4(1.0f), const bool isSelected = false);

		void SubmitPhysicsDebugMesh(Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, const Mat4& transform = Mat4(1.0f), const bool isSimpleCollider = true);
		void SubmitPhysicsStaticDebugMesh(Ref<StaticMesh> mesh, Ref<MeshSource> meshSource, const Mat4& transform = Mat4(1.0f), const bool isSimpleCollider = true);


		Ref<Pipeline> GetFinalPipeline() const;
		Ref<RenderPass> GetFinalRenderPass();
        /// TODO: Ref<RenderPass> GetCompositeRenderPass() { return m_CompositePipeline->GetSpecification().RenderPass; }
		Ref<RenderPass> GetCompositeRenderPass() { return nullptr; }
		Ref<Framebuffer> GetExternalCompositeFramebuffer() { return m_CompositingFramebuffer; }
		Ref<Image2D> GetFinalPassImage() const;

		Ref<Renderer2D> GetRenderer2D() { return m_Renderer2D; }
		Ref<Renderer2D> GetScreenSpaceRenderer2D() { return m_Renderer2DScreenSpace; }
		Ref<DebugRenderer> GetDebugRenderer() { return m_DebugRenderer; }

		SceneRendererOptions& GetOptions();
		const SceneRendererSpecification& GetSpecification() const { return m_Specification; }

		void SetShadowSettings(float nearPlane, float farPlane, float lambda, float scaleShadowToOrigin = 0.0f)
		{
			CascadeNearPlaneOffset = nearPlane;
			CascadeFarPlaneOffset = farPlane;
			CascadeSplitLambda = lambda;
			m_ScaleShadowCascadesToOrigin = scaleShadowToOrigin;
		}

		void SetShadowCascades(float a, float b, float c, float d)
		{
			m_UseManualCascadeSplits = true;
			m_ShadowCascadeSplits[0] = a;
			m_ShadowCascadeSplits[1] = b;
			m_ShadowCascadeSplits[2] = c;
			m_ShadowCascadeSplits[3] = d;
		}

		BloomSettings& GetBloomSettings() { return m_BloomSettings; }
		CBGTAOData& GetGTAOSettings() { return GTAODataCB; }
		DOFSettings& GetDOFSettings() { return m_DOFSettings; }

		void SetLineWidth(float width);

		static void WaitForThreads();

		uint32_t GetViewportWidth() const { return m_ViewportWidth; }
		uint32_t GetViewportHeight() const { return m_ViewportHeight; }

		float GetOpacity() const { return m_Opacity; }
		void SetOpacity(float opacity) { m_Opacity = opacity; }

		const Mat4& GetScreenSpaceProjectionMatrix() const { return m_ScreenSpaceProjectionMatrix; }
		const Statistics& GetStatistics() const { return m_Statistics; }

		bool IsReady() const { return m_ResourcesCreatedGPU; }

	private:
		void FlushDrawList();
		void PreRender();

		struct MeshKey
		{
			AssetHandle MeshHandle;
			AssetHandle MaterialHandle;
			uint32_t SubmeshIndex;
			bool IsSelected;

			MeshKey(const AssetHandle &meshHandle, const AssetHandle &materialHandle, uint32_t submeshIndex, bool isSelected) : MeshHandle(meshHandle), MaterialHandle(materialHandle), SubmeshIndex(submeshIndex), IsSelected(isSelected) {}

			bool operator<(const MeshKey& other) const
			{
				if (MeshHandle < other.MeshHandle)
					return true;

				if (MeshHandle > other.MeshHandle)
					return false;

				if (SubmeshIndex < other.SubmeshIndex)
					return true;

				if (SubmeshIndex > other.SubmeshIndex)
					return false;

				if (MaterialHandle < other.MaterialHandle)
					return true;

				if (MaterialHandle > other.MaterialHandle)
					return false;

				return IsSelected < other.IsSelected;

			}
		};


        /// -------------------------------------------------------

		struct DrawCommand
		{
			Ref<Mesh> Mesh;
			Ref<MeshSource> MeshSource;
			uint32_t SubmeshIndex;
			Ref<MaterialTable> MaterialTable;
			Ref<Material> OverrideMaterial;

			uint32_t InstanceCount = 0;
			uint32_t InstanceOffset = 0;
			bool IsRigged = false;
		};

        /// -------------------------------------------------------

		struct StaticDrawCommand
		{
			Ref<StaticMesh> StaticMesh;
			Ref<MeshSource> MeshSource;
			uint32_t SubmeshIndex;
			Ref<MaterialTable> MaterialTable;
			Ref<Material> OverrideMaterial;
			uint32_t InstanceCount = 0;
		};

        /// -------------------------------------------------------

		void SubmitStaticDebugMesh(std::map<SceneRenderer::MeshKey, SceneRenderer::StaticDrawCommand>& drawList, Ref<StaticMesh> staticMesh, Ref<MeshSource> meshSource, const Mat4& transform, Ref<Material> material);
		void CopyToBoneTransformStorage(const MeshKey& meshKey, const Ref<MeshSource>& meshSource, const std::vector<Mat4>& boneTransforms);
		void CreateBloomPassMaterials();
		void CreatePreConvolutionPassMaterials();
		void CreateXPlaneMaterialsPass();
		void CreatePreIntegrationPassMaterials();

		///< Render Passes
		void ClearPass() const;
		void ClearPass(const Ref<RenderPass> &renderPass, bool explicitClear = false) const;
		void GTAOCompute();
		void AOComposite();
		void GTAODenoiseCompute();
		void ShadowMapPass();
		void SpotShadowMapPass();
		void PreDepthPass();
		void Compute();
		void PreIntegration();
		void LightCullingPass();
		void SkyboxPass() const;
		void GeometryPass();
		void PreConvolutionCompute();
		void JumpFloodPass();

		///< Post-processing
		void BloomCompute();
		void SSRCompute();
		void SSRCompositePass();
		void EdgeDetectionPass() const;
		void CompositePass();

		struct CascadeData
		{
			Mat4 ViewProj;
			Mat4 View;
			float SplitDepth;
		};
		void CalculateCascades(CascadeData* cascades, const SceneRendererCamera& sceneCamera, const Vec3 &lightDirection) const;
		void CalculateCascadesManualSplit(CascadeData* cascades, const SceneRendererCamera& sceneCamera, const Vec3 &lightDirection) const;
		void UpdateStatistics();

		Ref<Scene> m_Scene;
		SceneRendererSpecification m_Specification;
		Ref<CommandBuffer> m_CommandBuffer;

		Ref<Renderer2D> m_Renderer2D, m_Renderer2DScreenSpace;
		Ref<DebugRenderer> m_DebugRenderer;

		Mat4 m_ScreenSpaceProjectionMatrix{ 1.0f };

        /// -------------------------------------------------------

		struct SceneInfo
		{
			SceneRendererCamera SceneCamera;

			///< Resources
			Ref<Environment> SceneEnvironment;
			float SkyboxLod = 0.0f;
			float SceneEnvironmentIntensity;
            SkyLightComponent SceneLightEnvironment;
            DirectionalLightComponent ActiveLight;
		} m_SceneData;

        /// -------------------------------------------------------

		struct UBOCamera
		{
            ///< Projection with near and far inverted
			Mat4 ViewProjection;
			Mat4 InverseViewProjection;
			Mat4 Projection;
			Mat4 InverseProjection;
			Mat4 View;
			Mat4 InverseView;
			Vec2 NDCToViewMul;
			Vec2 NDCToViewAdd;
			Vec2 DepthUnpackConsts;
			Vec2 CameraTanHalfFOV;
		} CameraDataUB;

		CBGTAOData GTAODataCB;

        /// -------------------------------------------------------

		struct UBOScreenData
		{
			Vec2 InvFullResolution;
			Vec2 FullResolution;
			Vec2 InvHalfResolution;
			Vec2 HalfResolution;
		} m_ScreenDataUB;

        /// -------------------------------------------------------

		struct UBOShadow
		{
			Mat4 ViewProjection[4];
		} ShadowData;

        /// -------------------------------------------------------

		struct DirLight
		{
			Vec3 Direction;
			float ShadowAmount;
			Vec3 Radiance;
			float Intensity;
		};

        /// -------------------------------------------------------

		struct UBOPointLights
		{
			uint32_t Count { 0 };
			Vec3 Padding {};
            PointLightComponent PointLights[1024]{};
		} PointLightsUB;

        /// -------------------------------------------------------

		struct UBOSpotLights
		{
			uint32_t Count{ 0 };
			Vec3 Padding{};
            SpotLightComponent SpotLights[1000]{};
		} SpotLightUB;

        /// -------------------------------------------------------

		struct UBOSpotShadowData
		{
			Mat4 ShadowMatrices[1000]{};
		} SpotShadowDataUB;

        /// -------------------------------------------------------

		struct UBOScene
		{
			DirLight Lights;
			Vec3 CameraPosition;
			float EnvironmentMapIntensity = 1.0f;
		} SceneDataUB;

        /// -------------------------------------------------------

		struct UBORendererData
		{
			Vec4 CascadeSplits;
			uint32_t TilesCountX { 0 };
			bool ShowCascades = false;
			char Padding0[3] = { 0,0,0 }; ///< Bools are 4-bytes in GLSL
			bool SoftShadows = true;
			char Padding1[3] = { 0,0,0 };
			float Range = 0.5f;
			float MaxShadowDistance = 200.0f;
			float ShadowFade = 1.0f;
			bool CascadeFading = true;
			char Padding2[3] = { 0,0,0 };
			float CascadeTransitionFade = 1.0f;
			bool ShowLightComplexity = false;
			char Padding3[3] = { 0,0,0 };
		} RendererDataUB;

        /// -------------------------------------------------------

		///< GTAO
		Ref<ComputePass> m_GTAOComputePass;
		Ref<ComputePass> m_GTAODenoisePass[2];
		struct GTAODenoiseConstants
		{
			float DenoiseBlurBeta;
			bool HalfRes;
			char Padding1[3]{ 0, 0, 0 };
		} m_GTAODenoiseConstants;
		Ref<Image2D> m_GTAOOutputImage;
		Ref<Image2D> m_GTAODenoiseImage;

        /// -------------------------------------------------------

        ///< Points to m_GTAOOutputImage or m_GTAODenoiseImage!
		Ref<Image2D> m_GTAOFinalImage; ///TODO: WeakRef!
		Ref<Image2D> m_GTAOEdgesOutputImage;
		UVec3 m_GTAOWorkGroups{ 1 };
		Ref<Material> m_GTAODenoiseMaterial[2]; ///Ping, Pong
		Ref<Material> m_AOCompositeMaterial;
		UVec3 m_GTAODenoiseWorkGroups{ 1 };
		Ref<Shader> m_CompositeShader;

        /// -------------------------------------------------------

		///< Shadows
		Ref<Pipeline> m_SpotShadowPassPipeline;
		Ref<Pipeline> m_SpotShadowPassAnimPipeline;
		Ref<Material> m_SpotShadowPassMaterial;

		UVec3 m_LightCullingWorkGroups;

		Ref<UniformBufferSet> m_UBSCamera;
		Ref<UniformBufferSet> m_UBSShadow;
		Ref<UniformBufferSet> m_UBSScene;
		Ref<UniformBufferSet> m_UBSRendererData;
		Ref<UniformBufferSet> m_UBSPointLights;
		Ref<UniformBufferSet> m_UBSScreenData;
		Ref<UniformBufferSet> m_UBSSpotLights;
		Ref<UniformBufferSet> m_UBSSpotShadowData;

        /// -------------------------------------------------------

		Ref<StorageBufferSet> m_SBSVisiblePointLightIndicesBuffer; ///< Visible point light indices per-tile
		Ref<StorageBufferSet> m_SBSVisibleSpotLightIndicesBuffer;  ///< Visible spot light indices per-tile

		std::vector<Ref<RenderPass>> m_DirectionalShadowMapPass; ///< Per-cascade
		std::vector<Ref<RenderPass>> m_DirectionalShadowMapAnimPass; ///< Per-cascade
		Ref<RenderPass> m_GeometryPass;
		Ref<RenderPass> m_GeometryAnimPass;
		Ref<RenderPass> m_PreDepthPass, m_PreDepthAnimPass, m_PreDepthTransparentPass;
		Ref<RenderPass> m_SpotShadowPass;
		Ref<RenderPass> m_DeinterleavingPass[2];
		Ref<RenderPass> m_AOCompositePass;

		Ref<ComputePass> m_LightCullingPass; ///< Light culling compute (HZB based)

        /// -------------------------------------------------------

		float LightDistance = 0.1f;
		float CascadeSplitLambda = 0.92f;
		Vec4 CascadeSplits;
		float CascadeFarPlaneOffset = 50.0f, CascadeNearPlaneOffset = -50.0f;
		float m_ScaleShadowCascadesToOrigin = 0.0f;
		float m_ShadowCascadeSplits[4];
		bool m_UseManualCascadeSplits = false;

		Ref<ComputePass> m_HierarchicalDepthPass; ///< HZB reduction pass

        /// -------------------------------------------------------

		///< SSR
		Ref<RenderPass> m_SSRCompositePass;
		Ref<ComputePass> m_SSRPass; ///< Screen space reflections ray march
		Ref<ComputePass> m_PreConvolutionComputePass; ///< Blur pyramid for reflections
		Ref<ComputePass> m_SSRUpscalePass; ///< (Optional) upscale pass if half-res enabled (not currently used)
		Ref<Image2D> m_SSRImage;

        /// -------------------------------------------------------

		///< Pre-Integration
		Ref<ComputePass> m_PreIntegrationPass; ///< Visibility pre-integration for SSR
		struct PreIntegrationVisibilityTexture
		{
			Ref<Texture2D> Texture;
			std::vector<Ref<ImageView>> ImageViews; ///< per-mip
		} m_PreIntegrationVisibilityTexture;
		std::vector<Ref<Material>> m_PreIntegrationMaterials; ///< per-mip

        /// -------------------------------------------------------

		///< Hierarchical Depth
		struct HierarchicalDepthTexture
		{
			Ref<Texture2D> Texture;
			std::vector<Ref<ImageView>> ImageViews; ///< per-mip
		} m_HierarchicalDepthTexture;
		std::vector<Ref<Material>> m_HZBMaterials; ///< per-mip

        /// -------------------------------------------------------

		struct PreConvolutionComputeTexture
		{
			Ref<Texture2D> Texture;
			std::vector<Ref<ImageView>> ImageViews; ///< per-mip
		} m_PreConvolutedTexture;
		std::vector<Ref<Material>> m_PreConvolutionMaterials; ///< per-mip
		Ref<Material> m_SSRCompositeMaterial;
		UVec3 m_SSRWorkGroups { 1 };

        /// -------------------------------------------------------

		Vec2 FocusPoint = { 0.5f, 0.5f };
        Ref<Material> m_WireframeMaterial;
		Ref<Material> m_CompositeMaterial;
		Ref<Material> m_LightCullingMaterial;

		Ref<Pipeline> m_GeometryPipeline;
		Ref<Pipeline> m_TransparentGeometryPipeline;
		Ref<Pipeline> m_GeometryPipelineAnim;
        Ref<Pipeline> m_PreDepthPipeline;
        Ref<Pipeline> m_PreDepthTransparentPipeline;
        Ref<Pipeline> m_PreDepthPipelineAnim;
        Ref<Pipeline> m_ShadowPassPipelines[4];
        Ref<Pipeline> m_ShadowPassPipelinesAnim[4];
        Ref<Pipeline> m_SkyboxPipeline;

        Ref<Material> m_PreDepthMaterial;
		Ref<Material> m_SelectedGeometryMaterial;
		Ref<Material> m_SelectedGeometryMaterialAnim;
        Ref<Material> m_ShadowPassMaterial;
        Ref<Material> m_SkyboxMaterial;
        Ref<Material> m_DOFMaterial;

		Ref<RenderPass> m_GeometryWireframePass;
		Ref<RenderPass> m_GeometryWireframeAnimPass;
		Ref<RenderPass> m_GeometryWireframeOnTopPass;
		Ref<RenderPass> m_GeometryWireframeOnTopAnimPass;
        Ref<RenderPass> m_SelectedGeometryPass;
        Ref<RenderPass> m_SelectedGeometryAnimPass;
		Ref<RenderPass> m_CompositePass;
		Ref<RenderPass> m_EdgeDetectionPass;
		Ref<RenderPass> m_SkyboxPass;
		Ref<RenderPass> m_DOFPass;

		Ref<ComputePipeline> m_LightCullingPipeline;

        /// -------------------------------------------------------

		///< Jump Flood Pass
		Ref<RenderPass> m_JumpFloodInitPass;
		Ref<RenderPass> m_JumpFloodPass[2];
		Ref<RenderPass> m_JumpFloodCompositePass;
		Ref<Material> m_JumpFloodInitMaterial, m_JumpFloodPassMaterial[2];
		Ref<Material> m_JumpFloodCompositeMaterial;

        /// -------------------------------------------------------

		///< Bloom compute
		Ref<ComputePass> m_BloomComputePass; ///< Compute based bloom pipeline
		uint32_t m_BloomComputeWorkgroupSize = 4;
		Ref<ComputePipeline> m_BloomComputePipeline;

        /// -------------------------------------------------------

		struct BloomComputeTextures
		{
			Ref<Texture2D> Texture;
			std::vector<Ref<ImageView>> ImageViews; ///< per-mip
		};
		std::vector<BloomComputeTextures> m_BloomComputeTextures{ 3 };

        /// -------------------------------------------------------

		struct BloomComputeMaterials
		{
			Ref<Material> PrefilterMaterial;
			std::vector<Ref<Material>> DownsampleAMaterials;
			std::vector<Ref<Material>> DownsampleBMaterials;
			Ref<Material> FirstUpsampleMaterial;
			std::vector<Ref<Material>> UpsampleMaterials;
		} m_BloomComputeMaterials;

        /// -------------------------------------------------------

		struct TransformVertexData
		{
			Vec4 MRow[3];
		};

        /// -------------------------------------------------------

		struct TransformBuffer
		{
			Ref<VertexBuffer> Buffer;
			TransformVertexData* Data = nullptr;
		};

        /// -------------------------------------------------------

		std::vector<TransformBuffer> m_SubmeshTransformBuffers;
		Ref<StorageBufferSet> m_SBSBoneTransforms; ///< Bone transforms storage
		Mat4 *m_BoneTransformsData = nullptr;
		std::vector<Ref<Framebuffer>> m_TempFramebuffers;

        /// -------------------------------------------------------

		struct TransformMapData
		{
			std::vector<TransformVertexData> Transforms;
			uint32_t TransformOffset = 0;
		};

        /// -------------------------------------------------------

		struct BoneTransformsMapData
		{
			std::vector<Mat4> BoneTransformsData;
			uint32_t BoneTransformsBaseIndex = 0;
			uint32_t BoneTransformsStride = 0;
		};

        /// -------------------------------------------------------

		std::map<MeshKey, TransformMapData> m_MeshTransformMap; ///< Aggregated instance transforms per mesh/material/submesh
		std::map<MeshKey, BoneTransformsMapData> m_MeshBoneTransformsMap; ///< Bone transform ranges

		std::map<MeshKey, DrawCommand> m_DrawList;
		std::map<MeshKey, DrawCommand> m_TransparentDrawList;
		std::map<MeshKey, DrawCommand> m_SelectedMeshDrawList;
		std::map<MeshKey, DrawCommand> m_ShadowPassDrawList;
		std::map<MeshKey, StaticDrawCommand> m_StaticMeshDrawList;
		std::map<MeshKey, StaticDrawCommand> m_TransparentStaticMeshDrawList;
		std::map<MeshKey, StaticDrawCommand> m_SelectedStaticMeshDrawList;
		std::map<MeshKey, StaticDrawCommand> m_StaticMeshShadowPassDrawList;

        /// -------------------------------------------------------

		///< Debug
		std::map<MeshKey, StaticDrawCommand> m_StaticColliderDrawList;
		std::map<MeshKey, DrawCommand> m_ColliderDrawList;
		std::map<MeshKey, StaticDrawCommand> m_AnimationDebugDrawList;
		Ref<Material> m_SimpleColliderMaterial;
		Ref<Material> m_ComplexColliderMaterial;
		Ref<Material> m_SelectedBoneMaterial;
		Ref<Material> m_BoneMaterial;
		Ref<StaticMesh> m_BoneMesh;
		Ref<MeshSource> m_BoneMeshSource;

        /// -------------------------------------------------------

		///< Grid
		Ref<RenderPass> m_GridRenderPass;
		Ref<Material> m_GridMaterial;
		Ref<Material> m_OutlineMaterial;
		Ref<Framebuffer> m_CompositingFramebuffer;

		SceneRendererOptions m_Options;
		SSROptionsUB m_SSROptions;

		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		float m_InvViewportWidth = 0.f, m_InvViewportHeight = 0.f;
		bool m_NeedsResize = false;
		bool m_Active = false;
		bool m_ResourcesCreatedGPU = false;
		bool m_ResourcesCreated = false;
		float m_LineWidth = 2.0f;

		BloomSettings m_BloomSettings;
		DOFSettings m_DOFSettings;
		Ref<Texture2D> m_BloomDirtTexture;

		Ref<Image2D> m_ReadBackImage;
		Ref<Image2D> m_GeometryPassColorAttachmentImage;
		Vec4* m_ReadBackBuffer = nullptr;
		float m_Opacity = 1.0f;

        /// -------------------------------------------------------

		struct GPUTimeQueries
		{
			uint32_t DirShadowMapPassQuery = 0;
			uint32_t SpotShadowMapPassQuery = 0;
			uint32_t DepthPrePassQuery = 0;
			uint32_t HierarchicalDepthQuery = 0;
			uint32_t PreIntegrationQuery = 0;
			uint32_t LightCullingPassQuery = 0;
			uint32_t GeometryPassQuery = 0;
			uint32_t PreConvolutionQuery = 0;
			uint32_t GTAOPassQuery = 0;
			uint32_t GTAODenoisePassQuery = 0;
			uint32_t AOCompositePassQuery = 0;
			uint32_t SSRQuery = 0;
			uint32_t SSRCompositeQuery = 0;
			uint32_t BloomComputePassQuery = 0;
			uint32_t JumpFloodPassQuery = 0;
			uint32_t CompositePassQuery = 0;
		} m_GPUTimeQueries;

        /// -------------------------------------------------------

		Statistics m_Statistics;
		friend class SceneRendererPanel;
	};
	*/

}

/// -------------------------------------------------------

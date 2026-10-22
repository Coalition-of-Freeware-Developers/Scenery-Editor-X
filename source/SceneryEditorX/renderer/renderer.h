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
 * renderer.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "cacao.h"
#include "csm.h"
#include "exposure.h"
#include "ocean.h"
#include "renderer_buffers.h"
#include "renderer_features.h"
#include "renderer_quality.h"
#include "ssr.h"
#include "font/font.h"
#include "vulkan/blend_states.h"
#include "vulkan/command_list.h"
#include "vulkan/command_pool.h"
#include "vulkan/image_resource.h"
#include "vulkan/push_constant_buffer.h"
#include "vulkan/render_context.h"
#include "vulkan/sampler.h"
#include "vulkan/viewport.h"
#include "vulkan/sync/frame_sync.h"
#include <array>
#include <filesystem>
#include <string>
#include <utility>
#include <SceneryEditorX/core/identifiers/flag.h>
#include <SceneryEditorX/core/threading/render_thread.h>
#include <SceneryEditorX/renderer/gpu_stats.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct PersistentLine;
	class MaterialAsset;
	class Mesh;
	enum class MeshType : uint8_t;
	class AssetManager;
	struct RendererProperties;
	class Swapchain;
	class ShaderManager;
	class Pipeline;
	class Camera;

	/**
	 * @struct ShadowSlice
	 * @brief Represents a slice of a shadow map for a specific light source.
	 */
	struct ShadowSlice
	{
		Light* light;
		uint32_t slice_Index;
		uint32_t res;
		xMath::Rectangle rect;
	};

	/**
	 * @struct Renderer_LightBillboard
	 * @brief A camera-facing light sprite, added onto the composed frame by Pass_Light_Billboards.
	 */
	struct Renderer_LightBillboard
	{
		float position[3] = { 0.0f, 0.0f, 0.0f };	// World-space center
		float radius = 0.0f;						// World-space half size
		float color[3] = { 1.0f, 1.0f, 1.0f };		// Linear HDR tint, brightness included
		float uv[4] = { 0.0f, 0.0f, 1.0f, 1.0f };	// s0 t0 s1 t1 (t = 0 is the texture's first row)
		bool tintLuminance = false;					// Tint the texel's brightness instead of its color
		bool useAlpha = false;						// Weight by the texel's alpha (sprites cut from a color texture)
	};

	/* @brief Light billboards sharing one texture. The batch keeps the texture alive while it is drawn. */
	struct Renderer_LightBillboardBatch
	{
		Ref<ImageResource> texture;
		std::vector<Renderer_LightBillboard> billboards;
	};

	/**
	 * @brief Static renderer class managing Vulkan rendering lifecycle.
	 * 
	 * The Renderer integrates with the Application main loop, providing:
	 * - Frame synchronization (fences, semaphores)
	 * - Command buffer management
	 * - Swapchain presentation
	 * 
	 * Usage pattern (called by Application::Run):
	 *   Renderer::BeginFrame();  // Acquire swapchain image, wait for fence
	 *   // ... module Tick() calls and rendering ...
	 *   Renderer::EndFrame();    // End command recording
	 *   Renderer::SubmitAndPresent(); // Submit to GPU and present
	 */
	class Renderer 
	{
	public:
		Renderer() = default;
		~Renderer() = default;

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Lifecycle Methods - Called by Application                                                                     ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @brief Initialize the renderer subsystem.
		 * 
		 * Creates Vulkan instance, device, swapchain, command pools, sync objects,
		 * and all per-frame resources. Must be called once before any other Renderer method.
		 */
		static void Init();

		/**
		 * @brief Shutdown the renderer and release all resources.
		 * 
		 * Waits for GPU idle, destroys all Vulkan objects in correct order.
		 * After calling Shutdown, Init must be called again before rendering.
		 */
		static void Shutdown();

		/**
		 * @brief Per-frame tick for memory/resource management.
		 * 
		 * Performs housekeeping tasks like memory allocator updates.
		 * Called once per frame by Application.
		 */
		static void Tick();

		/**
		 * @brief Submit a function to be executed on the render thread.
		 * @tparam FuncT The type of the function to submit.
		 * @param func The function to submit.
		 */
		template<typename FuncT>
		static void Submit(FuncT&& func)
		{
			/*
			auto renderCmd = [](void* ptr) {
				auto pFunc = (FuncT*)ptr;
				(*pFunc)();

				/**
				 * NOTE: Instead of destroying we could try and enforce all items to be trivially destructible
				 * however some items like uniforms which contain std::strings still exist for now
				 * static_assert(std::is_trivially_destructible_v<FuncT>, "FuncT must be trivially destructible");
				 #1#
				pFunc->~FuncT();
			};
			auto storageBuffer = QueueManager::AllocateQueue(renderCmd, sizeof(func));
			new (storageBuffer) FuncT(std::forward<FuncT>(func));
			*/

			// QueueManager::AllocateQueue API is queue-type allocation only.
			// Keep this helper deterministic and compatible by executing the callable inline.
			std::forward<FuncT>(func)();
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Frame Rendering Methods - Called each frame in sequence                                                       ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @brief Begin a new frame.
		 * 
		 * Acquires the next swapchain image, waits for the previous frame's fence,
		 * resets the fence, and begins command buffer recording. This must be called
		 * at the start of each frame before any draw calls.
		 * 
		 * @return true if frame can proceed, false if rendering should be skipped
		 *         (e.g., window minimized or swapchain out of date)
		 */
		static bool BeginFrame();

		/**
		 * @brief End command buffer recording for the current frame.
		 * 
		 * Finalizes command buffer recording and transitions swapchain image
		 * to present layout. Must be called after all draw commands are recorded.
		 */
		static void EndFrame();

		/**
		 * @brief Submit command buffers and present the swapchain image.
		 * 
		 * Submits the recorded command buffer to the graphics queue with proper
		 * synchronization, then presents the image. Handles swapchain recreation
		 * if VK_ERROR_OUT_OF_DATE_KHR is returned.
		 */
		static void SubmitAndPresent();

		/**
		 * @brief Record draw commands to the current frame's command buffer.
		 * @param cmdList Graphics command list for 3D rendering
		 * @param computeCmdList Compute command list (optional, may be nullptr)
		 */
		static void DrawFrame(CommandList *cmdList, CommandList *computeCmdList);

		/**
		 * @brief Blit a texture to the back buffer.
		 * @param cmdList Graphics command list for 3D rendering
		 * @param img Texture to blit
		 */
		static void BlitToBackBuffer(CommandList* cmdList, ImageResource* img);

		/**
		 * @brief Get a render target by type.
		 * @param type The type of render target to retrieve.
		 * @return Pointer to the requested render target.
		 */
		static ImageResource* GetRenderTarget(Renderer_RenderTarget type);

		/**
		 * @brief Write draw data for a mesh instance.
		 * @param transform The current transformation matrix of the mesh.
		 * @param prevTransform The previous transformation matrix of the mesh (for motion blur, etc.).
		 * @param matIndex The index of the material to use.
		 * @param isTransparent Whether the mesh is transparent.
		 * @param pickId Editor pick id of the drawing entity (ScenePicking::GetPickId), 0 when it can't be picked.
		 * @return Index of the written draw data.
		 */
		static uint32_t WriteDrawData(const xMath::Matrix& transform, const xMath::Matrix &prevTransform = xMath::Matrix::IDENTITY, uint32_t matIndex = 0, uint32_t isTransparent = 0, uint32_t pickId = 0);

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Render Context Management                                                                                     ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @brief Retrieve the global render context instance.
		 * @return Shared reference to RenderContext.
		 */
		static Ref<RenderContext> GetRenderContext();

		/**
		 * @brief Get the current frame-in-flight index (ring buffer slot).
		 * @return Frame index (0 to MAX_FRAMES_IN_FLIGHT-1).
		 */
		static uint32_t GetCurrentFrameIndex();

		/**
		 * @brief Get the total frame number since renderer initialization.
		 * @return Total frame count.
		 */
		static uint64_t GetFrameNumber();

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Render Thread Operations                                                                                      ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @brief Function executed by the render thread.
		 * @param renderThread Pointer to the RenderThread instance.
		 */
		static void RenderThreadFunc(RenderThread* renderThread);

		/**
		 * @brief Wait for the render thread to complete its work and then render the next frame.
		 * @param renderThread Pointer to the RenderThread instance.
		 */
		static void WaitAndRender(RenderThread *renderThread);

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Swapchain Management                                                                                          ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @brief Get the SwapChain instance managed by the Renderer.
		 * @return Pointer to the active SwapChain, or nullptr if not initialized.
		 */
		static Swapchain* GetSwapChain();

		/**
		 * @brief Get the current swapchain image index.
		 * @return Index of the acquired swapchain image.
		 */
		static uint32_t GetSwapchainImageIndex();

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Viewport & Image Management                                                                                   ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @brief Get the current viewport settings.
		 * @return Reference to the current Viewport.
		 */
		static const Viewport &GetViewport();

		/**
		 * @brief Set the current viewport settings.
		 * @param width The width of the viewport.
		 * @param height The height of the viewport.
		 */
		static void SetViewport(float width, float height);

		/**
		 * @brief Get the current renderer resolution.
		 * @return Reference to the current renderer resolution.
		 */
		static const Vec2 &GetRendererResolution();

		/**
		 * @brief Set the renderer resolution.
		 * @param width The width of the renderer resolution.
		 * @param height The height of the renderer resolution.
		 * @param recreateResources Whether to recreate resources after changing the resolution.
		 */
		static void SetRendererResolution(uint32_t width, uint32_t height, bool recreateResources = true);

		/**
		 * @brief Get the current output resolution.
		 * @return Reference to the current output resolution.
		 */
		static const Vec2 &GetOutputResolution();

		/**
		 * @brief Set the output resolution.
		 * @param width The width of the output resolution.
		 * @param height The height of the output resolution.
		 * @param recreateResources Whether to recreate resources after changing the resolution.
		 */
		static void SetOutputResolution(uint32_t width, uint32_t height, bool recreateResources = true);

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Command Buffer Access                                                                                         ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @brief Get the command buffer for the current frame.
		 * @return VkCommandBuffer for recording draw commands.
		 */
		static VkCommandBuffer GetCurrentCommandBuffer();

		/**
		 * @brief Get the active graphics-present command list for the current frame.
		 * @return Pointer to the current present command list, or nullptr if unavailable.
		 */
		static CommandList* GetCommandListPresent();

		/**
		 * @brief Returns nullptr — UI rendering is now performed inside RecordRenderCommands
		 *        via SetExternalRecordingBuffer, so no separate frame command list is needed.
		 * @deprecated Use GetCommandListPresent() with SetExternalRecordingBuffer instead.
		 */
		[[deprecated]] static CommandList* GetCommandListFrame();
		
		/* 
		 * @brief Create shader modules and pipelines. 
		 */
		static void LoadShaders();

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Util Functions																								  ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/* 
		 * @brief Capture a screenshot of the current frame and save it to disk. 
		 */
		static void Screenshot();

		/**
		 * @brief Returns a pointer to the standard mesh for the given type.
		 * Meshes are GPU-resident and available after renderer standard resource initialization.
		 *
		 * @param type The type of standard mesh to retrieve.
		 * @return Pointer to the requested standard mesh.
		 */
		static Mesh* GetStandardMesh(MeshType type);

		/**
		 * @brief Returns a pointer to the standard material for the given type.
		 * @return Pointer to the requested standard material.
		 */
		static Ref<MaterialAsset>& GetStandardMaterial();
		
		/**
		 * @brief Returns the pre-built Font object for the given preset.
		 * @return Reference to the standard font
		 */
		static Ref<Font>& GetFont();

		/* 
		 * @brief Retrieve current GPU memory usage statistics. 
		 * @return GPUMemoryStats struct containing current memory usage details.
		 */
		static GPUMemoryStats GetGPUMemoryStats();

		/**
		 * @brief Set the active camera used to drive view / projection for every frame.
		 * @param camera Raw pointer to a Camera instance; the Renderer does not take ownership.
		 *               Pass nullptr to revert to the identity-matrix fallback.
		 */
		static void SetCamera(Camera* camera);

		/* 
		 * @brief Returns the currently active camera, or nullptr if none has been set. 
		 */
		static Camera* GetCamera();

		/**
		 * Extra views (a second viewport): each has its own camera, size and resolution-dependent render targets and is
		 * drawn after the main view in the same frame, through the same passes. The main view keeps what only it does:
		 * exposure metering (extra views use its exposure), the environment probes, picking readbacks, GPU timings and
		 * captures. Shared, view-independent targets (LUTs, probes, the shadow maps, which each view draws again) aren't
		 * duplicated. Main thread, while the render thread is idle (it builds the UI then).
		 */

		/* @brief What an extra view looks through: the eye, its yaw / pitch in radians (forward = (cos p sin y, sin p, cos p cos y)), the field of view and projection. */
		struct ViewCamera
		{
			Vec3 position = Vec3(0.0f, 0.0f, 0.0f);
			float yaw = 0.0f;
			float pitch = 0.0f;
			float horizontalFov = 1.0471976f;
			bool orthographic = false;
			float orthographicHeight = 100.0f;		// Meters top to bottom
		};

		/* @brief Extra views at most: each takes a frame and an environment constant buffer update per frame. */
		static constexpr uint32_t MAX_EXTRA_VIEWS = 3;

		/* @brief Adds a view; it is drawn once it has a size. Returns its id, 0 when MAX_EXTRA_VIEWS exist. */
		static uint32_t CreateView();

		/* @brief Removes a view and frees its render targets (waits for the GPU). */
		static void DestroyView(uint32_t view);

		static void SetViewCamera(uint32_t view, const ViewCamera &camera);

		/* @brief The view's size in pixels; its render targets are made again when it changes (waits for the GPU). */
		static void SetViewResolution(uint32_t view, uint32_t width, uint32_t height);

		/* @brief The view's display image, shader-readable after a frame drew it; null before it has a size. */
		static ImageResource *GetViewOutput(uint32_t view);

		/* @brief Extra views that exist. */
		static uint32_t GetViewCount();

		/* @brief True while an extra view's passes are recorded (what only the main view does checks it). */
		static bool IsRenderingExtraView();

		/**
		 * @brief Get a structured buffer by type.
		 * @param type The type of buffer to retrieve.
		 * @return Pointer to the requested Buffer.
		 */
		static Buffer *GetBuffer(Renderer_Buffer type);

		/**
		 * @brief Update the camera uniform buffer object (UBO) for the current frame.
		 * @param frameIndex Index of the current frame in flight (0 to MAX_FRAMES_IN_FLIGHT - 1) for double/triple buffering.
		 * @note This should be called once per frame after setting the camera and before recording draw commands.
		 * @note This is used to determine which UBO instance to update in a ring buffer setup. 
		 * @note The camera data should be updated before recording draw commands that use it.
		 */
		static void UpdateCameraUBO(uint32_t frameIndex);

		/**
		 * @brief Update the shadow atlas render target based on current shadow-casting lights and their required resolutions.
		 */
		static void UpdateShadowAtlas();

		/**
		 * @brief Create depth-stencil states used for depth testing and stencil operations in the renderer.
		 */
		static void CreateDepthStencilStates();

		/**
		 * @brief Create rasterizer states used for configuring how polygons are rasterized in the renderer.
		 */
		static void CreateRasterizerStates();

		/**
		 * @brief Create blend states used for configuring how colors are blended in the renderer.
		 */
		static void CreateBlendStates();

		/**
		 * @brief Returns a pointer to the standard texture for the given type.
		 * @param type The type of standard texture to retrieve.
		 * @return Pointer to the requested standard texture.
		 */
		static ImageResource* GetStandardTexture(Renderer_StandardTexture type);

		/**
		 * @brief Replaces the light billboards drawn from the next frame on (Pass_Light_Billboards).
		 * @param batches Sprites grouped by texture; an empty list stops drawing them.
		 */
		static void SetLightBillboards(std::vector<Renderer_LightBillboardBatch> batches);

		/* @brief The XP12 emulation passes that are switched on (RENDERER panel, --no-<feature>). */
		static const RendererFeatures &GetFeatures();
		static void SetFeatures(const RendererFeatures &features);

		/* @brief What the frame shows instead of the image (RENDERER panel DEBUG VIEW). */
		static Renderer_DebugView GetDebugView();
		static void SetDebugView(Renderer_DebugView view);

		/* @brief X-Plane's quality levels the passes follow (shadows, AO, clouds, FSR...); defaults = X-Plane's vkmtl_async. */
		static const RendererQualityPreset &GetQualityPreset();
		static void SetQualityPreset(const RendererQualityPreset &preset);

		/* @brief The curve Tone mapping uses: 1 AgX (X-Plane 12.2+, default), 2 ACES fit, 3 Reinhard (A/B only). */
		static int GetToneCurve();
		static void SetToneCurve(int curve);

		/**
		 * @brief Constants the environment buffer carries besides the world state: X-Plane art-control values (sun and
		 * moon illuminance, atmosphere coefficients) and the exposure. Loaded by art-control name from
		 * resources/environment/xp12_environment.json (plan GUD-002); the defaults are the plan's initial values.
		 */
		struct EnvironmentConstants
		{
			// X-Plane 12.4.4's values as the reference probe dumped them (environment/atmosphere_params.h has the same)
			float sunIlluminanceLux = 133000.0f;		// lighting/E_sun_lx
			float moonIlluminanceLux = 0.6f;			// lighting/E_moon_lx (full moon)
			float sunAngularRadiusRad = 0.26f * 3.14159265f / 180.0f;	// sun/angular_diameter 0.52
			xMath::Vec4 rayleigh = xMath::Vec4(5.596e-6f, 11.75e-6f, 27.68e-6f, 8000.0f);	// atmo/rayleigh_*, scale height
			xMath::Vec4 mie = xMath::Vec4(3.996e-6f, 4.44e-6f, 1200.0f, 0.76f);	// scattering (green), extinction (mie_albedo 0.9), scale height, g_cornette_shanks
			xMath::Vec4 ozone = xMath::Vec4(2.427e-6f, 1.508e-6f, 0.105e-6f, 22300.0f);	// atmo/ozone_*, ozone_center
			xMath::Vec4 planet = xMath::Vec4(17800.0f, 6360000.0f, 6460000.0f, 0.15f);	// ozone_width / 2, radii, scattering/earth_albedo
			float mieAngstrom = -1.3f;					// scattering/mie_scattering_exponent (and _absorption_)
			float multipleScattering = 1.25f;			// scattering/multi_rat
			float visibilityContrast = 0.01f;			// scattering/vis_cutoff_ratio
			float fogAlbedo = 0.9f;						// atmo/fog_albedo: the visibility haze's scattering albedo
			float hazeTopM = 1500.0f;
			float hazeBandM = 1000.0f;
			ExposureSettings exposure;					// photometric/*, exposure/*, autoexposure/* (renderer/exposure.h)

			// exposure_fusion/shadows (EV), sigma, dis_near, dis_far (km); shadows 1 EV is an ASSUMPTION
			xMath::Vec4 exposureFusion = xMath::Vec4(1.0f, 0.2f, 1.0f, 4.0f);
			float fusionHighlights = 1.0f;				// EV the highlights exposure is lowered by (ASSUMPTION)
			float fusionStrength = 1.0f;				// 0 off .. 1 the full fused gain
			int fusionMaxMip = 5;						// exposure_fusion/max_mip: pyramid levels below the first
			float bloomIntensity = 0.04f;				// Share of the bloom chain in the frame (ASSUMPTION, plan TASK-042)
			float rcasSharpness = 1.0f;					// _rcas_ratio: FSR 1 RCAS scale, 1 the sharpest

			// The atmosphere (plan Phase 6)
			float aerialRangeM = 128000.0f;				// Aerial perspective volume depth (ASSUMPTION; X-Plane scattering/cache_*)
			float airglowNits = 2e-4f;					// Night sky floor: airglow and starlight (ASSUMPTION)
			float starBrightness = 1.0f;				// Procedural stars (1: magnitudes as given)
			float probeSpecularBias = 0.0f;				// REQ-005 specular LOD bias

			// color_management/sdr/*: ASC-CDL after the tone curve, identity by default
			xMath::Vec4 colorSlope = xMath::Vec4(1.0f, 1.0f, 1.0f, 1.0f);		// rgb slope, w saturation
			xMath::Vec4 colorOffset = xMath::Vec4(0.0f, 0.0f, 0.0f, 1.0f);	// rgb offset, w output gain
			xMath::Vec4 colorPower = xMath::Vec4(1.0f, 1.0f, 1.0f, 1.0f);		// rgb power, w output gamma
			// How much of the slope applies (slope^strength): X-Plane's tone curve keeps bright neutrals warm and its grade
			// cools them back; after SEDX's AgX, which whitens them, half the slope balances sky and ground
			float colorSlopeStrength = 0.5f;

			// Sun shadows (plan Phase 7): X-Plane's shadow controls. The quality preset's shadow level sets the cascade
			// count, far limit, fade and map size over these (renopt_shadow_quality does the same in X-Plane).
			CsmSettings shadow;							// shadow/csm/*, shadow/overlap_ratio, total_fade_ratio, fbo/shadow_cam_size
			float shadowBiasConst = 1.0f;				// shadow/bias_const: constant depth bias on the casters (depth units)
			float shadowBiasSlope = 2.0f;				// shadow/bias_slope: slope-scaled depth bias on the casters
			float shadowNormalBias = 0.0f;				// shadow/bias_n_constant: receivers moved along their normal (texels)
			float shadowNormalBiasSlope = 0.0f;			// shadow/bias_n_slope: plus this per unit of tan(angle to the light)
			bool shadowSoft = true;						// shadow/want_soft: 3 x 3 PCF
			// SEDX: the soft lookups compare against the receiver's plane (csm_sample.slang), so the casters' slope bias
			// only covers half a texel each way; X-Plane's factor 2 would lose a panel 5 cm above the ground (which its
			// references show casting a shadow) at cascade 1's 4 cm texels
			bool shadowReceiverPlane = true;			// sedx/csm_receiver_plane
			float shadowReceiverPlaneSlopeBias = 1.0f;	// sedx/csm_receiver_plane_slope_bias: the casters' slope bias with it
			// Cloud shadows (plan TASK-055)
			float cloudShadowStrength = 0.75f;			// clouds/cloud_shadow_lighten_ratio (ASSUMPTION: share of the light clouds can take)
			float cloudShadowExtinction = 0.0125f;		// cloud/shadow_alpha (ASSUMPTION: extinction per meter of cloud cover)
			float cloudShadowExtentMinKm = 64.0f;		// clouds/shadow_extent_min_km: the first cascade's width
			float cloudShadowExtentKm = 256.0f;			// clouds/shadow_extent_km: the last cascade's width

			// Ambient occlusion (plan Phase 8): X-Plane's CACAO controls (ssao/*); the QUALITY AO level sets the CACAO
			// quality and resolution over these, as renopt_SSAO_04 does in X-Plane
			CacaoSettings ao;

			// Screen-space reflections (plan Phase 9): X-Plane's ssr/* controls and SEDX's own (sedx/ssr_*)
			SsrSettings ssr;

			// Water (plan Phase 11): X-Plane's water/* controls and SEDX's own (sedx/water_*)
			OceanSettings water;
		};
		static const EnvironmentConstants &GetEnvironmentConstants();
		static EnvironmentConstants &EditEnvironmentConstants();
		static bool LoadEnvironmentConstants(const std::filesystem::path &path, std::string *error = nullptr);

		/* @brief This frame's sun-shadow cascades (renderer/csm.h; count 0: none) and the light they are for (0 none, 1 sun, 2 moon). */
		static const CsmSetup &GetShadowCascades();
		static uint32_t GetShadowLight();

		/* @brief How ambient occlusion is made: X-Plane's CACAO, or SEDX's older GTAO pass (ssao.slang) as a fallback. */
		enum class AoTechnique : uint8_t
		{
			Cacao,
			Gtao,
		};
		static AoTechnique GetAoTechnique();
		static void SetAoTechnique(AoTechnique technique);

		/* @brief What the last frame's ambient occlusion ran with (RENDERER panel). */
		struct AoReadout
		{
			bool ran = false;
			AoTechnique technique = AoTechnique::Cacao;
			CacaoQuality quality = CacaoQuality::High;
			bool halfResolution = true;
			CacaoSettings settings;					// After X-Plane's dynamic values
			uint32_t width = 0;						// The SSAO buffer's layers
			uint32_t height = 0;
		};
		static const AoReadout &GetAoReadout();

		/* @brief What the last frame's water ran with (RENDERER panel). */
		struct WaterReadout
		{
			bool ran = false;						// The ocean's cascades were made this frame
			uint32_t draws = 0;						// Water draws in the main view
			OceanSpectrum spectrum;					// The sea state (after NormalizeOceanSpectrum)
			double time = 0.0;						// The ocean's time (s)
		};
		static const WaterReadout &GetWaterReadout();

		/**
		 * @brief Advances the ocean's waves by this step every frame instead of the frame's real time (0: real time), and
		 * starts them over: parity captures (--env-scenario) see the same sea every run.
		 */
		static void SetWaterFixedTimeStep(float seconds);
		static void ResetWaterTime();

		/* @brief What the last frame's screen-space reflections ran with (RENDERER panel). */
		struct SsrReadout
		{
			bool ran = false;
			bool history = false;					// The previous frame was there to reflect
			uint32_t width = 0;						// The reflections target
			uint32_t height = 0;
			uint32_t pyramidLevels = 0;
			uint32_t historyLevels = 0;
		};
		static const SsrReadout &GetSsrReadout();
		/* @brief Debug (--ssr-trace-all): every deferred surface traces as a mirror, for the Reflections debug view. */
		static void SetSsrTraceAll(bool all);
		static bool GetSsrTraceAll();

		/* @brief The exposure the GPU last reported (exposure_adapt.slang), read back a frame or two late. */
		struct ExposureReadout
		{
			float ev100 = 0.0f;				// What the frame is exposed at (adapted, bias applied)
			float meteredEv100 = 0.0f;		// Metered, clamped to the range
			float targetEv100 = 0.0f;
			float rawEv100 = 0.0f;			// Metered before the clamp
			float meanLog2Nits = 0.0f;
			float preExposure = 1.0f;		// What this frame's lighting writes were multiplied by
			bool valid = false;
		};
		static ExposureReadout GetExposureReadout();

		/* @brief Jump straight to the metered exposure on the next frame instead of adapting (scenario loads, teleports). */
		static void ResetExposure();

		/* @brief Draws the editor grid (on by default; parity captures hide it, X-Plane has none). */
		static void SetGridVisible(bool visible);
		static bool IsGridVisible();

		/* @brief Terrain is drawn (SceneTerrain): the grid, a plane at y = 0, stays hidden as it would cut through the ground. */
		static void SetGroundShown(bool shown);

		/* @brief What the viewport reference overlay shows over frame_output (reference_overlay.slang). */
		enum class ReferenceOverlayMode : uint8_t
		{
			Off,
			Reference,		// The reference image only (flicker alternates this with Off)
			Split,			// The reference left of a vertical line, the frame right of it
			Difference		// |frame - reference| x gain, on sRGB-encoded values
		};

		/**
		 * @brief A reference image (an X-Plane 12 screenshot, loaded by TextureLoader) drawn over the output after any
		 * frame capture, so captures never include it.
		 * @param split Position of the split line, 0-1 of the width.
		 */
		static void SetReferenceOverlay(const Ref<ImageResource> &image, ReferenceOverlayMode mode, float split = 0.5f, float differenceGain = 4.0f);

	private:

		/**
		 * @brief Create render targets (swapchain images, depth buffer) based on current swapchain configuration.
		 * @param createRender Whether to create render targets (color/depth) for the swapchain images.
		 * @param createOutput Whether to create output render targets.
		 * @param createDynamic Whether to create dynamic render targets.
		 */
		static void CreateRenderTargets(const bool createRender, const bool createOutput, const bool createDynamic);

		/**
		 * @brief An extra view's resolution-dependent render targets at its size: the entries CreateRenderTargets makes
		 * for the render and output resolutions, without the shared ones (the shadow atlas). The main view's are kept.
		 * @param out Receives them; the other entries are left null.
		 */
		static void CreateViewRenderTargets(uint32_t width, uint32_t height, std::array<Ref<ImageResource>, static_cast<uint32_t>(Renderer_RenderTarget::MaxEnum)> &out);

		/* 
		 * @brief Update optional render targets based on current renderer configuration. 
		 */
		static void UpdateOptionalRenderTargets();

		/* 
		 * @brief Create per-frame resources such as command buffers and synchronization objects. 
		 */
		static void CreateFrameResources();

		/* 
		 * @brief Destroy per-frame resources such as command buffers and synchronization objects. 
		 * @note This should be called during renderer shutdown and whenever the number of frames in flight changes (e.g., swapchain recreation).
		 */
		static void DestroyFrameResources();

		/**
		 * @brief Release the render targets, standard textures and samplers held in the renderer's static arrays.
		 * @note Called from Shutdown(): left to the static arrays, they would only be released in the CRT
		 * static-destructor phase, after the device is gone.
		 */
		static void DestroyResources();

		/**
		 * @brief Record draw commands for the current frame using the provided command lists.
		 * @param cb Command buffer to record into (retrieved via GetCurrentCommandBuffer())
		 * @param imageIndex Index of the swapchain image being rendered to (for resource binding)
		 */
		static void RecordRenderCommands(VkCommandBuffer cb, uint32_t imageIndex);

		/**
		 * @brief Creates all Vulkan samplers used by the renderer.
		 *
		 * Non-anisotropic samplers are created once on first call and guarded by Ref validity.
		 * The anisotropic sampler is recreated whenever the output/render resolution ratio
		 * changes so that the negative mip LOD bias can be updated accordingly.
		 */
		static void CreateSamplers();

		/**
		 * @brief Creates standard meshes used by the renderer.
		 */
		static void CreateStandardMeshes();

		/**
		 * @brief Creates fonts used by the renderer.
		 */
		static void CreateFonts();

		/* 
		 * @brief Creates standard materials used by the renderer. 
		 */
		static void CreateStandardMaterials();

		/* 
		 * @brief Creates standard textures used by the renderer. 
		 */
		static void CreateStandardTextures();

		/**
		 * @brief Creates structured buffers used for per-frame data and other purposes.
		 */
		static void CreateBuffers();

		/**
		 * @brief Get a sampler by type.
		 * @param type The type of sampler to retrieve.
		 * @return Pointer to the requested sampler.
		 */
		static Sampler* GetSampler(Renderer_Sampler type);

		/**
		 * @brief Get a render target by type.
		 * @return Reference to the array of render targets, indexed by Renderer_RenderTarget enum.
		 */
		static std::array<Ref<ImageResource>, static_cast<uint32_t>(Renderer_RenderTarget::MaxEnum)>& GetRenderTargets();

		/**
		 * @brief Get a structured buffer by type.
		 * @return Reference to the array of structured buffers, indexed by Renderer_Buffer enum.
		 */
		static std::array<Ref<Buffer>,  static_cast<uint32_t>(Renderer_Buffer::MaxEnum)>& GetStructuredBuffers();

		/**
		 * @brief Get a sampler by type.
		 * @return Reference to the array of samplers, indexed by Renderer_Sampler enum.
		 */
		static std::array<Ref<Sampler>, static_cast<uint32_t>(Renderer_Sampler::MaxEnum)>& GetSamplers();

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Render Passes                                                                                                 ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @brief Record the render passes for the current frame.
		 * @param graphicsPresent Command list for graphics and presentation operations.
		 * @param compute Command list for compute operations (optional, may be nullptr).
		 */
		static void ProduceFrame(CommandList* graphicsPresent, CommandList* compute);

		/* @brief Records every extra view after the main one: its targets, camera and constants swapped in, its draw lists and shadow cascades built again. */
		static void RenderExtraViews(CommandList *cmdList);

#pragma region LUT generation passes
		/**
		 * @brief Generate the BRDF specular lookup table.
		 * @param cmdList Command list to record the pass into.
		 */
		static void Pass_Lut_BrdfSpecular(CommandList *cmdList);

#pragma endregion
#pragma region Noise generation passes
		/**
		 * @brief Generate the cloud noise texture.
		 * @param cmdList Command list to record the pass into.
		 */
		static void Pass_CloudNoise(CommandList *cmdList);

#pragma endregion
#pragma region Per-frame Sky & Atmosphere

		/**
		 * @brief Render the cloud shadows for the current frame.
		 * @param cmdList Command list to record the pass into.
		 */
		static void Pass_CloudShadow(CommandList *cmdList);

#pragma endregion
#pragma region Variable - rate shading
		/**
		 * @brief Perform variable-rate shading for the current frame.
		 * @param cmdList Command list to record the pass into.
		 */
		static void Pass_VariableRateShading(CommandList *cmdList);

#pragma endregion
#pragma region Geometry passes

		/**
		 * @brief Generate the hierarchical Z-buffer for the current frame.
		 * @param cmdList Command list to record the pass into.
		 */
		static void Pass_HiZ(CommandList *cmdList);

		/**
		 * @brief Perform indirect culling for the current frame.
		 * @param cmdList Command list to record the pass into.
		 */
		static void Pass_IndirectCull(CommandList *cmdList);

		/**
		 * @brief GPU-scene occlusion culling between the depth prepass's two phases: builds a min-depth hi-z pyramid of the
		 * depth drawn so far and runs GpuScene::CullLate() against it.
		 * @param cmdList Command list to record the pass into.
		 */
		static void Pass_GpuScene_Occlusion(CommandList *cmdList);

		/**
		 * @brief Perform the depth pre-pass for the current frame.
		 * @param cmdList Command list to record the pass into.
		 */
		static void Pass_Depth_Prepass(CommandList *cmdList);

		/**
		 * @brief Render the G-buffer for the current frame.
		 * @param cmdList Command list to record the pass into.
		 * @param isTransparentPass Indicates whether this is a transparent pass.
		 */
		static void Pass_GBuffer(CommandList *cmdList, const bool isTransparentPass);

		/**
		 * @brief Writes the depth of the opaque drapes the g-buffer just drew (in drape order, tested against everything
		 * else only), each nudged nearer by its rank, so lighting sees their surface and the forward pass's blended drapes
		 * sort against them by rank.
		 */
		static void Pass_Drape_Depth(CommandList *cmdList);

#pragma endregion
#pragma region Shadow passes

		/**
		 * @brief Render the shadow maps for the current frame.
		 * @param cmdList Command list to record the pass into.
		 */
		static void Pass_ShadowMaps(CommandList *cmdList);

		// Sun and cloud shadows (plan Phase 7, renderer_shadows.cpp)
		/* @brief The casters into each cascade of shadow_csm: the GPU scene's instances (GpuScene::CullShadows) and the CPU
		 * shadow draw list; shadow_csm ends in shader-read layout whatever happens. */
		static void Pass_ShadowCascades(CommandList *cmdList);
		/* @brief (Re)creates shadow_csm, 4 layers of mapSize squared D32F (waits for the GPU when it replaces one). */
		static void CreateShadowCascadeTarget(uint32_t mapSize);
		/* @brief Fits this frame's cascades to the camera and the shadow light (UpdateDrawCalls, before it collects casters). */
		static void UpdateShadowCascades(const CsmCamera *camera);
		/* @brief Writes the cascades and the cloud shadow cascades into the environment constants. */
		static void WriteShadowConstants(ConstantBuffer_Environment &env);
		/* @brief Binds shadow_csm and cloud_shadow to a pass whose shaders read them (csm_sample.slang). */
		static void BindShadows(CommandList *cmdList);

#pragma endregion
#pragma region Ambient occlusion

		// Ambient occlusion (plan Phase 8, renderer_ao.cpp; XP12: cacao)
		/* @brief CACAO (or the GTAO fallback) into the ssao target, which ends shader-readable; false when nothing ran. */
		static bool Pass_AmbientOcclusion(CommandList *cmdList);
		/* @brief (Re)creates the ssao target and CACAO's buffers for the render resolution and the AO quality's resolution;
		 * force: the render resolution changed (CreateRenderTargets), else only when the AO resolution did (waits for the GPU). */
		static void CreateAmbientOcclusionTargets(bool force);
		/* @brief Binds the ambient occlusion (white when none ran this frame) to a pass whose shaders read tex_ssao. */
		static void BindAmbientOcclusion(CommandList *cmdList);

#pragma endregion
#pragma region Screen-space reflections

		// The depth pyramid and screen-space reflections (plan Phase 9, renderer_ssr.cpp; XP12: z_minmax, ssr_deferred)
		/* @brief The min / max depth pyramid (z_minmax.slang) of depth into gbuffer_depth_occluders_hiz, every level
		 * shader-readable after; false when it couldn't be built. */
		static bool Pass_DepthPyramid(CommandList *cmdList, ImageResource *depth);
		/* @brief Forgets the reflections of the frame before (BindReflections binds black until this frame's are traced). */
		static void BeginReflectionsFrame();
		/* @brief The reflections run this frame: the ssr feature is on and their shaders are built. */
		static bool AreReflectionsAvailable();
		/* @brief Step (5) of the frame graph: the pyramid of the finished depth, for the reflections' march. */
		static bool Pass_ReflectionDepthPyramid(CommandList *cmdList);
		/* @brief Step (9): the deferred surfaces' reflections (ssr_trace.slang) into the reflections target, which ends
		 * shader-readable; false when nothing ran. */
		static bool Pass_ScreenSpaceReflections(CommandList *cmdList);
		/* @brief After fix_hdr: the finished frame into frame_render_previous and its blurred mips, for the next frame. */
		static void Pass_ReflectionHistory(CommandList *cmdList, ImageResource *frame);
		/* @brief Binds the reflections, the pyramid and the history (black where they aren't ready) to a pass whose shaders
		 * import ssr_common.slang, and the g-buffer normals the march checks hits against. */
		static void BindReflections(CommandList *cmdList);

#pragma endregion
#pragma region Water

		// Water (plan Phase 11, renderer_water.cpp; XP12: ocean_spectrum, compute_fft, ocean_bake_textures, ocean_shading)
		/* @brief The sea state from the environment's waves and the water constants (renderer/ocean.h); the ocean's time
		 * advances once a frame (extra views reuse it). */
		static void WriteWaterConstants(ConstantBuffer_Environment &env);
		/* @brief Water is drawn in this view: the feature is on, water draws are visible and the shaders are built. */
		static bool IsWaterVisible();
		/* @brief The FFT ocean's cascades at this frame's time (ocean.slang), once a frame; they end shader-readable. */
		static void Pass_Ocean(CommandList *cmdList);
		/* @brief Step (13): the water draws over the composed frame (water.slang), opaque, their depth written. */
		static void Pass_Water(CommandList *cmdList, ImageResource *frame);
		/* @brief Binds the baked cascades to a pass whose shaders import ocean_common.slang. */
		static void BindOcean(CommandList *cmdList);

#pragma endregion
#pragma region Lighting passes

		static void Pass_Light(CommandList *cmdList, const bool isTransparentPass);
		static void Pass_Light_Composition(CommandList *cmdList, const bool isTransparentPass);

#pragma endregion
#pragma region Particle passes

		static void Pass_Particles(CommandList *cmdList);

#pragma endregion
#pragma region Transparency

		static void Pass_TransparencyReflectionRefraction(CommandList *cmdList);

#pragma endregion
#pragma region Output / utility
		static void Pass_Blit(CommandList *cmdList, ImageResource *in, ImageResource *out);
		static void Pass_Downscale(CommandList *cmdList, ImageResource *img, Renderer_DownsampleFilter filter);
		static void Pass_Grid(CommandList *cmdList, ImageResource *out);
		static void Pass_Text(CommandList *cmdList, ImageResource *out);
		static void Pass_ReferenceOverlay(CommandList *cmdList, ImageResource *out);
		/* @brief Output transform: the pre-exposed HDR frame to display-referred linear values (tonemap.slang). */
		static void Pass_Tonemap(CommandList *cmdList, ImageResource *in, ImageResource *out, uint32_t bloomMips = 0, bool fusion = false);
		/* @brief The DEBUG VIEW over the output (debug_view.slang). */
		static void Pass_DebugView(CommandList *cmdList, ImageResource *out);

		// The output chain of plan Phase 5 (renderer_post.cpp)
		/* @brief NaN / Inf / negative values out of frame_render (fix_hdr.slang). */
		static void Pass_FixHdr(CommandList *cmdList, ImageResource *frame);
		/* @brief Histogram metering and adaptation (luminance_histogram.slang, exposure_adapt.slang). */
		static void Pass_Exposure(CommandList *cmdList, ImageResource *frame);
		/* @brief The bloom mip chain from frame_render; returns the number of mips summed in mip 0 (0: none). */
		static uint32_t Pass_Bloom(CommandList *cmdList, ImageResource *frame);
		/* @brief Exposure fusion down to exposure_fusion_guide (exposure_fusion.slang). */
		static void Pass_ExposureFusion(CommandList *cmdList, ImageResource *frame);
		/* @brief The output chain: tone map, FXAA, FSR 1 (EASU + RCAS) into out. */
		static void Pass_OutputChain(CommandList *cmdList, ImageResource *frame, ImageResource *out);
		/* @brief Reads the exposure the GPU last wrote, for the next frame's pre-exposure. */
		static void UpdateExposureReadback();
		/* @brief Records the pre-exposure this frame's lighting is written with (GetExposureReadout().preExposure). */
		static void SetExposurePreExposure(float preExposure);

		// The atmosphere and environment probes (plan Phase 6, renderer_atmosphere.cpp)
		/* @brief Transmittance, multi-scattering and sky-ambient LUTs when the medium changed; sky-view and aerial perspective. */
		static void Pass_Atmosphere(CommandList *cmdList);
		/* @brief Captures and filters probe 0 when what it sees changed (env_probes.h). */
		static void Pass_EnvProbes(CommandList *cmdList);
		/* @brief The sky where nothing was drawn (sky.slang). */
		static void Pass_Sky(CommandList *cmdList, ImageResource *frame);
		/* @brief The atmosphere textures a pass reads (BindAtmosphere). */
		enum AtmosphereTextures : uint32_t
		{
			ATMOSPHERE_PROBES = 1u << 0,
			ATMOSPHERE_AERIAL = (1u << 1) | (1u << 2),
			ATMOSPHERE_SKY_VIEW = 1u << 3,
			ATMOSPHERE_TRANSMITTANCE = 1u << 4,
			ATMOSPHERE_MULTISCATTER = 1u << 5,
			ATMOSPHERE_SKY_AMBIENT = 1u << 6,
			ATMOSPHERE_LIGHTING = ATMOSPHERE_PROBES | ATMOSPHERE_AERIAL | ATMOSPHERE_TRANSMITTANCE,	// environment_lighting.slang
		};
		/* @brief Binds the atmosphere LUTs and probes a pass reads (and its shaders declare). */
		static void BindAtmosphere(CommandList *cmdList, uint32_t textures);
		/* @brief Puts the atmosphere LUTs and probes in shader-read layout (before render passes that read them). */
		static void ReadyAtmosphereTextures(CommandList *cmdList);

		/**
		 * @brief Draws alpha-blended materials (MaterialProperty::BlendMode) back to front over the composed frame.
		 * Texels at or above the material's alpha cutoff were already drawn by the deferred passes.
		 * Expects the g-buffer depth in DepthStencilAttachment layout (tested, not written).
		 */
		static void Pass_Forward_Transparent(CommandList *cmdList, ImageResource *out);

		/* @brief Adds the light billboards (SetLightBillboards) onto the frame, depth tested against the g-buffer. */
		static void Pass_Light_Billboards(CommandList *cmdList, ImageResource *out);

		/* @brief Releases the forward passes' streaming buffers and billboard textures (renderer shutdown). */
		static void DestroyForwardResources();

		template <typename F = std::nullptr_t>
		static void Pass_Compute(CommandList *cmdList, const char *name, Renderer_Shader shaderEnum, ImageResource *in, ImageResource *out, F setup);

#pragma endregion
#pragma region Screen-space effects (async-compute passes)

		static void Pass_ScreenSpaceAO(CommandList *cmdList);
		static void Pass_ScreenSpaceShadows(CommandList *cmdList);

#pragma endregion

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Pipeline State Helpers                                                                                        ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @brief Returns the pre-built RasterizerState object for the given preset.
		 * Objects are created once during renderer init and are never mutated.
		 */
		static RasterizerState *GetRasterizerState(Renderer_RasterizerState type);

		/**
		 * @brief Returns the pre-built BlendState object for the given preset.
		 */
		static BlendState *GetBlendState(Renderer_BlendState type);

		/**
		 * @brief Returns the pre-built DepthStencilState object for the given preset.
		 */
		static DepthStencilState *GetDepthStencilState(Renderer_DepthStencilState type);

		/**
		 * @brief Returns true when the given draw call should be submitted via the CPU-driven path.
		 * GPU-indirect draws are handled separately and should be skipped in CPU loops.
		 */
		static bool IsCpuDrivenDraw(const Renderer_DrawCall &drawCall, const class MaterialAsset *material);

		/**
		 * @brief Rotates the per-frame buffers to avoid CPU-GPU race conditions without stalling.
		 * @note This allows the CPU to write to one buffer while the GPU reads from another, with a safe number of buffers in flight as a cushion.
		 */
		static void RotateFrameBuffers();

		/**
		 * @brief Binds the common per-frame textures (noise, depth, etc.) that every pass needs.
		 */
		static void SetCommonTextures(CommandList *cmdList);

		/**
		 * @brief Writes per-frame data (camera matrices, lighting info, etc.) into the GPU frame-constant buffer.
		 * @param cmdList 
		 */
		static void UpdateFrameConstantBuffer(CommandList *cmdList);
		/* @brief Fills ConstantBuffer_Environment from the world environment (Environment::Get()) and uploads it. */
		static void UpdateEnvironmentConstantBuffer(CommandList *cmdList);

		/**
		 * @brief Updates the draw calls for the current frame.
		 * @param cmdList The command list to record the draw calls into.
		 */
		static void UpdateDrawCalls(CommandList *cmdList);

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Debug Primitives																							  ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @brief Draw a line between two points with specified colors and duration.
		 *
		 * This function adds a line to the renderer's debug draw list. If duration_sec is greater than 0, 
		 * the line will persist for that many seconds; otherwise, it will only be drawn for the current frame.
		 * @param from Starting point of the line in world space.
		 * @param to Ending point of the line in world space.
		 * @param color_from Color at the starting point of the line.
		 * @param color_to Color at the ending point of the line.
		 * @param duration_sec Duration in seconds for which the line should persist. If 0 or less, the line will only be drawn for the current frame.
		 */
		static void DrawLine(const Vec3 &from, const Vec3 &to, const Color &color_from, const Color &color_to, float duration_sec);

		/**
		 * @brief Draw a line between two points with default color and duration.
		 * @param from Starting point of the line in world space.
		 * @param to Ending point of the line in world space.
		 */
		static void DrawLine(const Vec3 &from, const Vec3 &to);

		/**
		 * @brief Draw a line between two points with a specified color and duration.
		 * @param from Starting point of the line in world space.
		 * @param to Ending point of the line in world space.
		 * @param color Color of the line.
		 * @param duration_sec Duration in seconds for which the line should persist. If 0 or less, the line will only be drawn for the current frame.
		 */
		static void DrawLine(const Vec3 &from, const Vec3 &to, const Color &color, float duration_sec);

		/**
		 * @brief Draw a triangle with specified vertices, color, and duration.
		 * @param v0 First vertex of the triangle in world space.
		 * @param v1 Second vertex of the triangle in world space.
		 * @param v2 Third vertex of the triangle in world space.
		 * @param color Color of the triangle (applied to all vertices).
		 * @param duration_sec Duration in seconds for which the triangle should persist. If 0 or less, it will only be drawn for the current frame.
		 */
		static void DrawTriangle(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2, const Color &color, float duration_sec);

		/**
		 * @brief Draw a box with specified bounding box, color, and duration.
		 * @param box The bounding box to draw.
		 * @param color Color of the box.
		 * @param duration_sec Duration in seconds for which the box should persist. If 0 or less, it will only be drawn for the current frame.
		 */
		static void DrawBox(const BoundingBox &box, const Color &color, float duration_sec);

		/**
		 * @brief Draw a circle with specified center, axis, radius, segment count, color, and duration.
		 * @param center Center of the circle in world space.
		 * @param axis Normal vector of the circle's plane.
		 * @param radius Radius of the circle.
		 * @param segment_count Number of segments to approximate the circle.
		 * @param color Color of the circle.
		 * @param duration_sec Duration in seconds for which the circle should persist. If 0 or less, it will only be drawn for the current frame.
		 */
		static void DrawCircle(const Vec3 &center, const Vec3 &axis, float radius, uint32_t segment_count, const Color &color, float duration_sec);

		/**
		 * @brief Draw a sphere with specified center, radius, segment count, color, and duration.
		 * @param center Center of the sphere in world space.
		 * @param radius Radius of the sphere.
		 * @param segment_count Number of segments to approximate the sphere.
		 * @param color Color of the sphere.
		 * @param duration_sec Duration in seconds for which the sphere should persist. If 0 or less, it will only be drawn for the current frame.
		 */
		static void DrawSphere(const Vec3 &center, float radius, uint32_t segment_count, const Color &color, float duration_sec);

		/**
		 * @brief Draw a directional arrow with specified start and end points, arrow size, color, and duration.
		 * @param start Starting point of the arrow in world space.
		 * @param end Ending point of the arrow in world space.
		 * @param arrow_size Size of the arrowhead.
		 * @param color Color of the arrow.
		 * @param duration_sec Duration in seconds for which the arrow should persist. If 0 or less, it will only be drawn for the current frame.
		 */
		static void DrawDirectionalArrow(const Vec3 &start, const Vec3 &end, float arrow_size, const Color &color, float duration_sec);

		/**
		 * @brief Draw a plane with specified plane equation, color, and duration.
		 * @param plane The plane to draw.
		 * @param color Color of the plane.
		 * @param duration_sec Duration in seconds for which the plane should persist. If 0 or less, it will only be drawn for the current frame.
		 */
		static void DrawPlane(const xMath::Plane &plane, const Color &color, float duration_sec);

		/**
		 * @brief Update the persistent lines, removing any that have expired.
		 */
		static void UpdatePersistentLines();

		/**
		 * @brief Add lines to be rendered for the current frame.
		 */
		static void AddLinesToBeRendered();

	public:
		/* @brief Light gizmos and renderable bounding boxes as debug lines (off: costly for big scenes). */
		static void SetDebugGizmos(bool enabled);

	private:

		// line and icon rendering
		static Ref<Buffer> m_Lines_VertexBuffer;
		static std::vector<Vertex_PosCol> m_Lines_Vertices;
		static std::vector<PersistentLine> m_Persistent_Lines;
		static std::vector<std::tuple<ImageResource*, xMath::Vec3>> m_Icons;

		// -------------------------------------------------------

		/**
		 * @brief Update materials for the current frame.
		 * @param cmdList Command list to record the update commands.
		 */
		static void UpdateMaterials(CommandList* cmdList);

		/**
		 * @brief Update lights for the current frame.
		 * @param cmdList Command list to record the update commands.
		 */
		static void UpdateLights(CommandList* cmdList);

		/**
		 * @brief Update bounding boxes for the current frame.
		 * @param cmdList Command list to record the update commands.
		 */
		static void UpdateBoundingBoxes(CommandList* cmdList);

		CommandList *m_CurrentCmdList; // Set at the beginning of each frame, used for resource updates and utility functions.
		AssetManager *m_AssetManager;  // Set during Init, used for loading models, textures, etc.

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/// Static State																								  ///
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		static RendererProperties *m_Data;
		static std::atomic<bool> m_ResourcesInitialized; // Flag to indicate when resources are ready for use, set to true at the end of Init()

		// Bindless
		// bindless draw data
		static std::array<ShaderBuffer_DrawData, RENDERER_MAX_DRAW_CALLS> m_DrawData_CPU; // Staging area for draw data written by the CPU; copied to GPU buffer each frame
		static std::mutex m_MutexRenderables; // Mutex to protect access to m_DrawData_CPU and m_DrawData_Count during scene submission from multiple threads
		static uint32_t m_DrawData_Count; // Number of draw data entries written for the current frame; used to determine how many to copy to GPU and how many draw calls to issue

		// Array of pointers to all textures used by the renderer, indexed by material parameters; bound as a bindless array in shaders
		static std::array<ImageResource*, MAX_MATERIAL_TEXTURES> m_Bindless_Textures;
		static uint32_t m_Bindless_TextureCount; // Slots of m_Bindless_Textures in use (written to the descriptor set)

		// Array of light data for all active lights in the scene, indexed by a per-light index; bound as a bindless array in shaders
		static std::array<ShaderBuffer_Light, MAX_ARRAY_SIZE> m_Bindless_Lights; 

		// Array of AABB data for all renderables, indexed by a per-renderable index; used for GPU-driven culling and other operations
		static std::array<ShaderBuffer_Aabb, MAX_ARRAY_SIZE> m_Bindless_Aabbs;

		static Flag m_BindlessSamplers_Dirty; // Flag to indicate when bindless samplers need to be updated in shaders, set whenever sampler states change (e.g., anisotropy level changes)

		/**
		 * @struct PassState
		 * @brief Tracks one-shot initialization and feature-toggle state for various render passes and resources. 
		 */
		struct PassState
		{
			// one-shot initialization (run once, never again unless reset)
			bool m_BRDF_LutProduced        = false;
			bool m_Atmosphere_LutProduced  = false;
			bool m_CloudNoiseProduced      = false;

			// feature-toggle clear flags (set when feature disabled, reset when re-enabled)
			bool m_ClearedRtReflections   = false;
			bool m_ClearedRtShadows       = false;
			bool m_ClearedRestir          = false;

			// vrs
			ImageResource* m_VrsLastClearedTexture = nullptr;
			void Reset() { *this = PassState(); }

		};

		static PassState m_PassState; // Tracks one-shot initialization and feature-toggle state for various render passes and resources; used to conditionally execute certain passes or initialization steps
		static PushConstantBuffer_Pass m_Pcb_Pass_Cpu; // Per-pass push constant staging buffer (written by passes, uploaded by PushConstants())
		static ConstantBuffer_Frame m_Cb_Frame_Cpu; // Staging area for per-frame constants written by the CPU; copied to GPU buffer each frame
		static ConstantBuffer_Environment m_Cb_Environment_Cpu; // Same for the world environment (binding 201)

		// CPU-side draw call arrays (populated by scene submission, consumed by passes)
		static std::array<Renderer_DrawCall, RENDERER_MAX_DRAW_CALLS> m_DrawCalls;
		static std::array<Renderer_DrawCall, RENDERER_MAX_DRAW_CALLS> m_DrawCalls_Prepass;
		static std::array<Renderer_DrawCall, RENDERER_MAX_DRAW_CALLS> m_DrawCalls_Shadow; // CPU-drawn sun-shadow casters (shadowCascades set)
		static std::array<Renderer_DrawCall, RENDERER_MAX_WATER_DRAW_CALLS> m_DrawCalls_Water; // Visible water (materials with MaterialProperty::Water): Pass_Water only
		static uint32_t m_DrawCall_Count;
		static uint32_t m_DrawCalls_Prepass_Count;
		static uint32_t m_DrawCalls_Shadow_Count;
		static uint32_t m_DrawCalls_Water_Count;
		static uint32_t m_Indirect_DrawCount;
		static bool     m_Transparents_Present;
		static bool     m_Is_Hiz_Suppressed;

		static CommandList *m_CmdList_Compute;
		static CommandList *m_CmdList_Present;
		// @brief Kept as nullptr — UI is now rendered via SetExternalRecordingBuffer inside RecordRenderCommands.
		static CommandList *m_CmdList_Frame;
		static Scope<AssetManager> s_AssetManager;
		static uint32_t m_ResourceIndex;

		// Frame synchronization
		static Scope<FrameSync> m_FrameSync;
		static Scope<CommandPool> m_CommandPool;
		static std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> m_CommandBuffers;

		// Frame tracking
		static uint32_t m_CurrentFrameIndex;     // Ring buffer index (0 to MAX_FRAMES_IN_FLIGHT-1)
		static uint64_t m_FrameNumber;           // Total frames rendered
		static uint32_t m_SwapchainImageIndex;   // Current swapchain image
		static bool m_FrameInProgress;           // True between BeginFrame and EndFrame

		/**
		 * @struct IndirectFrameResource
		 * @brief Holds per-frame GPU resources to avoid race conditions between in-flight frames.
		 */
		struct IndirectFrameResource
		{
			Ref<Buffer> m_DrawArgs;
			Ref<Buffer> m_DrawData;
			Ref<Buffer> m_DrawArgs_Out;
			Ref<Buffer> m_DrawData_Out;
			Ref<Buffer> m_DrawCount;
		};
		static std::array<IndirectFrameResource, DRAW_DATA_BUFFER_COUNT> m_FrameResources;
		static uint32_t m_FrameResource_Index;
		static std::array<ShaderBuffer_IndirectDrawArgs, MAX_ARRAY_SIZE> m_Indirect_DrawArgs;
		static std::array<ShaderBuffer_DrawData, MAX_ARRAY_SIZE> m_Indirect_DrawData;
		static std::vector<ShadowSlice> m_ShadowSlices;

		/* Active camera providing view / projection for every frame */
		static Camera* m_Camera;
		static xMath::Frustum m_Frustum;

		/* Per-frame camera uniform buffers (host-visible, mapped), matching CameraShaderData */
		static VkDescriptorSetLayout m_CameraDescriptorSetLayout;
		static VkDescriptorPool m_CameraDescriptorPool;
		static std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_CameraDescriptorSets;
		static std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> m_CameraUboBuffers;
		static std::array<VmaAllocation, MAX_FRAMES_IN_FLIGHT> m_CameraUboAllocations;
		static std::array<void*, MAX_FRAMES_IN_FLIGHT> m_CameraUboMapped;

		static uint32_t m_Count_ActiveLights;
		static bool m_Has_DirectionalLight; // false: light composition supplies a placeholder sun and sky

		/* 
		 * @brief Creates per-frame camera UBOs, descriptor pool and sets. 
		 */
		static void CreateCameraResources();
	};

}

/**
 * NOTE: Changing from deferred queue-based execution to immediate inline execution
 * fundamentally alters the threading model and removes the render command queue mechanism.
 * This breaks the intended design of deferring render commands to a dedicated render thread.
 * 
 * If QueueManager is being removed or refactored, this change should be clearly documented
 * and all callers should be audited to ensure thread safety.
 * 
 * TODO: If this is a temporary workaround, document the expected final solution and ensure
 * that all render command invocations are thread-safe or only called from the render thread.
 */

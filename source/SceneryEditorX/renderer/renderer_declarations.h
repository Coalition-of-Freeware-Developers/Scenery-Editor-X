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
 * renderer_declarations.h
 * -------------------------------------------------------
 * Created: 04/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include "SceneryEditorX/scene/material.h"
#include <SceneryEditorX/scene/lights.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Buffer;

	/**
	 * @enum Renderer_RenderTarget
	 * @brief Enumeration of renderer render targets.
	 *
	 * This enum provides a type-safe way to identify and reference
	 * the different render targets used in the rendering system.
	 *
	 * @note - The values in this enum are designed to be compatible with Vulkan's render target types.
	 */
	enum class Renderer_RenderTarget : uint8_t
	{
		gbuffer_color,
		gbuffer_normal,
		gbuffer_material,
		gbuffer_velocity,
		gbuffer_depth,
		gbuffer_depth_occluders,
		gbuffer_depth_occluders_hiz,
		gbuffer_depth_opaque_output,
		lut_brdf_specular,
		lut_atmosphere_scatter,
		lut_atmosphere_transmittance,
		lut_atmosphere_multiscatter,
		light_diffuse,
		light_specular,
		light_volumetric,
		frame_render,
		frame_render_opaque,
		frame_output,
		frame_output_2,
		ssao,
		reflections,
		gbuffer_reflections_position,
		gbuffer_reflections_normal,
		gbuffer_reflections_albedo,
		sss,
		skysphere,
		bloom,
		blur,
		outline,
		shading_rate,
		shadow_atlas,
		auto_exposure,
		auto_exposure_previous,
		// restir reservoir buffers (current frame)
		restir_reservoir0,
		restir_reservoir1,
		restir_reservoir2,
		restir_reservoir3,
		restir_reservoir4,
		// restir reservoir buffers (previous frame for temporal)
		restir_reservoir_prev0,
		restir_reservoir_prev1,
		restir_reservoir_prev2,
		restir_reservoir_prev3,
		restir_reservoir_prev4,
		// restir reservoir buffers (spatial ping-pong)
		restir_reservoir_spatial0,
		restir_reservoir_spatial1,
		restir_reservoir_spatial2,
		restir_reservoir_spatial3,
		restir_reservoir_spatial4,
		// volumetric clouds
		cloud_noise_shape,
		cloud_noise_detail,
		cloud_shadow,
		// nrd denoiser textures
		nrd_viewz,
		nrd_normal_roughness,
		nrd_diff_radiance_hitdist,
		nrd_spec_radiance_hitdist,
		nrd_out_diff_radiance_hitdist,
		nrd_out_spec_radiance_hitdist,
		// debug
		debug_output,
		MaxEnum
	};

	/**
	 * @enum Renderer_Shader
	 * @brief Enumeration of renderer shaders.
	 *
	 * This enum provides a type-safe way to identify and reference
	 * the different shaders used in the rendering system.
	 */
	enum class Renderer_Shader : uint8_t
	{
		tessellation_h,
		tessellation_d,
		gbuffer_vertex,
		gbuffer_frag,
		depth_prepass_vertex,
		depth_prepass_alpha_test_frag,
		depth_light_vertex,
		depth_light_alpha_color_frag,
		fxaa_comp,
		film_grain_comp,
		motion_blur_c,
		depth_of_field_c,
		chromatic_aberration_c,
		vhs_c,
		bloom_luminance_c,
		bloom_blend_frame_c,
		bloom_upsample_blend_mip_c,
		bloom_downsample_c,
		output_c,
		light_integration_brdf_specular_lut_c,
		light_integration_environment_filter_c,
		light_c,
		light_composition_c,
		light_image_based_c,
		line_vertex,
		line_frag,
		grid_vertex,
		grid_frag,
		outline_vertex,
		outline_frag,
		outline_comp,
		font_vertex,
		font_frag,
		ssao_comp,
		sss_c_bend,
		skysphere_comp,
		skysphere_lut_comp,
		skysphere_transmittance_lut_c,
		skysphere_multiscatter_lut_c,
		blur_gaussian_c,
		blur_gaussian_bilaterial_c,
		variable_rate_shading_c,
		ffx_cas_c,
		ffx_spd_average_c,
		ffx_spd_min_c,
		ffx_spd_max_c,
		blit_c,
		icon_c,
		dithering_c,
		transparency_reflection_refraction_c,
		auto_exposure_c,
		// volumetric clouds
		cloud_noise_shape_c,
		cloud_noise_detail_c,
		cloud_shadow_c,
		light_reflections_c,
		// nrd denoiser
		nrd_prepare_c,
		// gpu-driven indirect rendering
		indirect_cull_c,
		gbuffer_indirect_vertex,
		gbuffer_indirect_frag,
		depth_prepass_indirect_vertex,
		// gpu-driven particles
		particle_emit_c,
		particle_simulate_c,
		particle_render_c,
		// gpu texture compression
		texture_compress_bc1_c,
		texture_compress_bc3_c,
		texture_compress_bc5_c,
		MaxEnum
	};

	/**
	 * @enum Renderer_Sampler
	 * @brief Enumeration of renderer samplers.
	 *
	 * This enum provides a type-safe way to identify and reference
	 * the different samplers used in the rendering system.
	 */
	enum class Renderer_Sampler : uint8_t
	{
		Compare_depth,
		Point_clamp_edge,
		Point_clamp_border,
		Point_wrap,
		Bilinear_clamp_edge,
		Bilinear_clamp_border,
		Bilinear_wrap,
		Trilinear_clamp,
		Anisotropic_wrap,
		MaxEnum
	};

	/**
	 * @enum Renderer_Buffer
	 * @brief Enumeration of renderer buffers.
	 *
	 * This enum provides a type-safe way to identify and reference
	 * the different buffers used in the rendering system.
	 */
	enum class Renderer_Buffer : uint8_t
	{
		ConstantFrame,
		SpdCounter,
		MaterialParameters,
		LightParameters,
		DummyInstance,
		AABBs,
		GeometryInfo,
		IndirectDrawArgs,
		IndirectDrawData,
		IndirectDrawDataOut,
		IndirectDrawArgsOut,
		IndirectDrawCount,
		DrawData, // bindless per-draw data (transforms, material index, etc.)
		// gpu-driven particles
		ParticleBufferA,
		ParticleBufferB,
		ParticleCounter,
		ParticleEmitter,
		MaxEnum
	};

	/**
	 * @enum StandardTexture
	 * @brief Enumeration of standard textures provided by the renderer.
	 * 
	 * This enum provides a type-safe way to identify and reference
	 * the different standard textures used in the rendering system.
	 */
	enum class StandardTexture : uint8_t
	{
		Noise_perlin,
		Noise_blue, // single blue noise texture (was 8, only 1 used)
		Checkerboard,
		Gizmo_light_directional,
		Gizmo_light_point,
		Gizmo_light_spot,
		Gizmo_audio_source,
		Black,
		White,
		MaxEnum
	};

	/**
	 * @enum Renderer_RasterizerState
	 * @brief Enumerates the rasterizer pipeline state presets available to the renderer.
	 */
	enum class Renderer_RasterizerState : uint8_t
	{
		Solid,
		Wireframe,
		Light_point_spot,
		Light_directional,
		MaxEnum
	};

	/**
	 * @enum Renderer_DepthStencilState
	 * @brief Enumerates depth/stencil test and write configuration presets.
	 */
	enum class Renderer_DepthStencilState : uint8_t
	{
		Off,
		ReadEqual,
		ReadGreaterEqual,
		ReadWrite,
		MaxEnum
	};

	/**
	 * @enum Renderer_BindingsSrv
	 *  @brief Enumerates shader resource view (SRV) bindings used by the renderer.
	 *	
	 * This enum provides a type-safe way to identify and reference
	 * the different SRV bindings used in the rendering system.
	 */
	enum class Renderer_BindingsSrv
	{
		// g-buffer
		gbuffer_albedo   = 0,
		gbuffer_normal   = 1,
		gbuffer_material = 2,
		gbuffer_velocity = 3,
		gbuffer_depth    = 4,

		// other
		ssao = 5,
	
		// misc
		tex   = 6,
		tex2  = 7,
		tex3  = 8,
		tex4  = 9,
		tex5  = 10,
		tex6  = 11,
		tex3d = 12,

		// noise
		tex_perlin = 13,
		// bindless
		bindless_material_textures   = 14,
		bindless_material_parameters = 15,
		bindless_light_parameters    = 16,
		bindless_aabbs               = 17,
		bindless_draw_data           = 18,
		
		// volumetric clouds 3D noise
		tex3d_cloud_shape  = 19,
		tex3d_cloud_detail = 20,
		// restir reservoir srv bindings (for temporal/spatial read)
		reservoir_prev0    = 21,
		reservoir_prev1    = 22,
		reservoir_prev2    = 23,
		reservoir_prev3    = 24,
		reservoir_prev4    = 25,
	};

	/**
	 * @enum Renderer_BindingsUav
	 *  @brief Enumerates unordered access view (UAV) bindings used by the renderer.
	 *	
	 * This enum provides a type-safe way to identify and reference
	 * the different UAV bindings used in the rendering system.
	 */
	enum class Renderer_BindingsUav
	{
		tex           = 0,
		tex2          = 1,
		tex3          = 2,
		tex4          = 3,
		tex3d         = 4,
		tex_sss       = 5,
		sb_spd        = 7,
		tex_spd       = 8,
		// restir reservoir uav bindings
		reservoir0    = 20,
		reservoir1    = 21,
		reservoir2    = 22,
		reservoir3    = 23,
		reservoir4    = 24,
		// nrd output bindings
		nrd_viewz              = 25,
		nrd_normal_roughness   = 26,
		nrd_diff_radiance      = 27,
		nrd_spec_radiance      = 28,
		// integer format textures (vrs, etc)
		tex_uint               = 29,
		// gpu-driven indirect drawing
		indirect_draw_args     = 30,
		indirect_draw_data     = 31,
		indirect_draw_args_out = 32,
		indirect_draw_data_out = 33,
		indirect_draw_count    = 34,
		// gpu-driven particles
		particle_buffer_a      = 35,
		particle_buffer_b      = 36,
		particle_counter       = 37,
		particle_emitter       = 38,
		// gpu texture compression
		compress_input         = 39,
		compress_output        = 40,
		compress_output_bc1    = 41,
	};

	// -------------------------------------------------------
	// Renderer_BlendState: Alpha-blend preset enumeration
	// -------------------------------------------------------

	/**
	 * @enum Renderer_BlendState
	 * @brief Enumerates blending mode presets used when configuring render passes.
	 */
	enum class Renderer_BlendState : uint8_t
	{
		Off,
		Alpha,
		Additive,
		Premultiplied,
		MaxEnum
	};

	// -------------------------------------------------------
	// Renderer_DownsampleFilter: mip-chain downscale filter modes
	// -------------------------------------------------------

	/**
	 * @enum Renderer_DownsampleFilter
	 * @brief Filter kernel used by Pass_Downscale when building mip chains via SPD.
	 */
	enum class Renderer_DownsampleFilter : uint8_t
	{
		Average,
		Min,
		Max,
		MaxEnum
	};

	// -------------------------------------------------------
	// BarrierType: simplified image barrier intent
	// -------------------------------------------------------

	/**
	 * @enum BarrierType
	 * @brief Coarse-grained barrier intent used by CommandList::InsertBarrier(ImageResource*, BarrierType).
	 * The command list maps these to the correct Vulkan pipeline-stage and access-mask pairs.
	 */
	enum class BarrierType : uint8_t
	{
		EnsureWriteThenRead,   // Make previous UAV writes visible to subsequent reads
		EnsureReadThenWrite,   // Ensure prior reads complete before next UAV write
		EnsureWriteThenWrite,  // Serialise two consecutive UAV writes
		MaxEnum
	};

	// -------------------------------------------------------
	// MeshType: standard geometry used in built-in passes
	// -------------------------------------------------------

	/**
	 * @enum MeshType
	 * @brief Identifies built-in primitive meshes managed by the Renderer.
	 */
	enum class MeshType : uint8_t
	{
		Quad,
		Cube,
		Sphere,
		MaxEnum
	};

	// -------------------------------------------------------
	// Stage alias for shader-type shorthand
	// -------------------------------------------------------
	// Stage is defined in vulkan/enums.h; ShaderType is an alias used in pass code.
	// The unscoped alias below lets pass code write `Stage::Pixel` as a synonym for
	// `Stage::Fragment` and index PipelineState::shaders with the Stage enum directly.
	// -------------------------------------------------------

#pragma region Constants
	/** Sentinel passed to pso.clear_depth to instruct the pipeline to LOAD existing depth rather than clear it. */
	inline constexpr float RHI_DEPTH_LOAD  = -1.0f;
	inline constexpr float DEPTH_LOAD      = RHI_DEPTH_LOAD;

	/** Maximum simultaneous colour render targets in a PipelineState. */
	inline constexpr uint32_t MAX_RENDER_TARGET_COUNT  = 8;
	inline constexpr uint32_t MAX_SHADER_STAGES        = 8;
	inline constexpr uint32_t RENDERER_MAX_DRAW_CALLS  = 4096;
	inline constexpr uint32_t MAX_MIP_COUNT            = 16;
#pragma endregion

	// -------------------------------------------------------
	// Minimal scene-object stubs used by renderer passes.
	// Full definitions live in scene/ once those subsystems are complete.
	// -------------------------------------------------------

	/**
	 * @class Renderable
	 * @brief Stub renderable class providing the interface consumed by renderer passes.
	 */
	class Renderable : public RefCounted
	{
	public:
		virtual ~Renderable() = default;
		virtual Material*   GetMaterial() const                          { return nullptr; }
		virtual Entity*     GetEntity() const                            { return nullptr; }
		virtual Buffer*     GetVertexBuffer() const                      { return nullptr; }
		virtual Buffer*     GetInstanceBuffer() const                    { return nullptr; }
		virtual Buffer*     GetIndexBuffer() const                       { return nullptr; }
		virtual uint32_t    GetIndexCount(uint32_t /*lod*/ = 0) const    { return 0; }
		virtual uint32_t    GetIndexOffset(uint32_t /*lod*/ = 0) const   { return 0; }
		virtual uint32_t    GetVertexOffset(uint32_t /*lod*/ = 0) const  { return 0; }
	};

	/**
	 * @class Mesh
	 * @brief Stub mesh class providing the interface consumed by renderer passes.
	 */
	class Mesh
	{
	public:
		virtual ~Mesh() = default;
		virtual Buffer*   GetVertexBuffer() const    { return nullptr; }
		virtual Buffer*   GetIndexBuffer() const     { return nullptr; }
		virtual uint32_t  GetGlobalIndexOffset() const  { return 0; }
		virtual uint32_t  GetGlobalVertexOffset() const { return 0; }
	};

	// -------------------------------------------------------
	// Renderer_DrawCall: per-draw submission record
	// -------------------------------------------------------

	/**
	 * @struct Renderer_DrawCall
	 * @brief Represents one opaque or transparent draw call submitted to the renderer.
	 * Populated by scene traversal, consumed by render passes.
	 */
	struct Renderer_DrawCall
	{
		Renderable* renderable       = nullptr; // Scene renderable to draw
		bool        cameraVisible    = false;   // Passes frustum / occlusion cull
		bool        isOccluder       = false;   // Used as Hi-Z occluder mesh
		uint32_t    drawData_Index   = 0;       // Index into the GPU draw-data buffer
		uint32_t    lodIndex         = 0;       // LOD level to render
		uint32_t    instanceIndex    = 0;       // First instance offset
		uint32_t    instanceCount    = 1;       // Number of instances
	};

	// -------------------------------------------------------
	// CVar: lightweight console-variable stub
	// Provides GetValue() / GetValueAs<T>() used by renderer passes.
	// Replace with the real console-variable system when available.
	// -------------------------------------------------------

	/**
	 * @struct CVar
	 * @brief Lightweight console-variable stub consumed by renderer pass code.
	 */
	struct CVar
	{
		float m_Value = 0.0f;

		float GetValue() const { return m_Value; }

		template<typename T>
		T GetValueAs() const { return static_cast<T>(m_Value != 0.0f); }

		void SetValue(float v) { m_Value = v; }
	};

}

// -------------------------------------------------------

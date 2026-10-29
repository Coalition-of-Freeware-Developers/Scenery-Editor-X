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
 * renderer_buffers.h
 * -------------------------------------------------------
 * Created: 16/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include <colors.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @struct ConstantBuffer_Frame
	 * @brief Constant buffer for per-frame data
	 * @note low frequency, updates once per frame
	 */
	struct ConstantBuffer_Frame
	{
		xMath::Matrix view;
		xMath::Matrix view_inv;
		xMath::Matrix view_previous;
		xMath::Matrix projection;
		xMath::Matrix projection_inv;
		xMath::Matrix projection_previous;
		xMath::Matrix view_projection;
		xMath::Matrix view_projection_inv;
		xMath::Matrix view_projection_ortho;
		xMath::Matrix view_projection_unjittered;
		xMath::Matrix view_projection_previous;
		xMath::Matrix view_projection_previous_unjittered;

		xMath::Vec2 resolution_render;
		xMath::Vec2 resolution_output;

		xMath::Vec2 taa_jitter_current;
		xMath::Vec2 taa_jitter_previous;

		float camera_aperture;
		float delta_time;
		uint32_t frame;
		uint32_t options;

		xMath::Vec3 camera_position;
		float camera_near;

		xMath::Vec3 camera_forward;
		float camera_far;

		float camera_last_movement_time;
		float hdr_enabled;
		float hdr_max_nits;
		float padding;

		xMath::Vec3 camera_position_previous;
		float resolution_scale;

		double time;
		float camera_fov;
		float padding2;

		xMath::Vec3 wind;
		float gamma;

		xMath::Vec3 camera_right;
		float camera_exposure;

		// clouds
		float cloud_coverage;
		float cloud_shadows;
		float padding3;
		float padding4;

		// pads the struct to 1024 bytes so every element of the per-frame ring buffer starts on a
		// minUniformBufferOffsetAlignment boundary (dynamic offsets); mirrored in resources.slang
		xMath::Vec4 padding_alignment[5];

		/**
		 * @brief Sets or clears a specific bit in the options field
		 * @param set Whether to set (true) or clear (false) the bit
		 * @param bit The bit to modify
		 */
		void SetBit(const bool set, const uint32_t bit)
		{
			options = set ? (options |= bit) : (options & ~bit);
		}
	};

	/**
	 * @struct ShaderBuffer_Material
	 * @brief Material parameters for GPU shaders, designed to match the layout of the Material struct in HLSL.
	 */
	struct ShaderBuffer_Material
	{
		xMath::Vec4 color = xMath::Vec4{0.0f, 0.0f, 0.0f, 0.0f};

		xMath::Vec2 tiling_uv = xMath::Vec2::ZERO;
		xMath::Vec2 offset_uv = xMath::Vec2::ZERO;
		xMath::Vec2 invert_uv = xMath::Vec2::ZERO;
		float roughness_mul = 0.0f;
		float metallic_mul  = 0.0f;
		float normal_mul    = 0.0f;
		float height_mul    = 0.0f;

		uint32_t flags    = 0;
		float local_width = 0.0f;
		float alpha_cutoff = 0.0f;	// MaterialProperty::AlphaCutoff
		float subsurface_scattering;

		float sheen;
		float local_height   = 0.0f;
		float world_space_uv = 0.0f;
		float blend_mode     = 0.0f; // MaterialProperty::BlendMode

		float anisotropic;
		float anisotropic_rotation;
		float clearcoat;
		float clearcoat_roughness;

		// SNOW_ALBEDO_LUMA (flags bit 24): the snow receptivity's weights for the linear albedo's r and g (weather.slang)
		float snow_luma[2] = {};

		// X-Plane materials (xp_material.slang): specular gain, super roughness floor, F0 when the normal map carries
		// none, emissive luminance in nits (0: emissive texels are display values)
		xMath::Vec4 xp = xMath::Vec4{0.0f, 0.0f, 0.0f, 0.0f};

		// Draped polygons (xp_material.slang xp_drape_*, mode in flags bits 19-20): tile atlas (tiles x / y, mapping texture
		// cells x / y), detail decals (scale, strength x 2), normal decals (scale, gain x 2), misc (decal aspects, meters per
		// u / v; markings: paint color, border opacity)
		xMath::Vec4 drape_tiles = xMath::Vec4{0.0f, 0.0f, 0.0f, 0.0f};
		xMath::Vec4 drape_decal = xMath::Vec4{0.0f, 0.0f, 0.0f, 0.0f};
		xMath::Vec4 drape_normal_decal = xMath::Vec4{0.0f, 0.0f, 0.0f, 0.0f};
		xMath::Vec4 drape_misc = xMath::Vec4{0.0f, 0.0f, 0.0f, 0.0f};

		// X-Plane terrain (.ter, xp_terrain.slang): base (projected s / t meters, normal map repeats per base repeat, flags),
		// composite (s / t, noise mask center / half width), slopes (hill / cliff start and end degrees), slope textures (s / t
		// meters), lit (s / t meters, fade start / end), far (s / t meters, start / end)
		xMath::Vec4 terrain_base = xMath::Vec4{0.0f, 0.0f, 0.0f, 0.0f};
		xMath::Vec4 terrain_composite = xMath::Vec4{0.0f, 0.0f, 0.0f, 0.0f};
		xMath::Vec4 terrain_slope = xMath::Vec4{0.0f, 0.0f, 0.0f, 0.0f};
		xMath::Vec4 terrain_slope_scale = xMath::Vec4{0.0f, 0.0f, 0.0f, 0.0f};
		xMath::Vec4 terrain_lit = xMath::Vec4{0.0f, 0.0f, 0.0f, 0.0f};
		xMath::Vec4 terrain_far = xMath::Vec4{0.0f, 0.0f, 0.0f, 0.0f};

		// Detail decal keys (xp_drape_apply_detail_decals): decal 0 RGB, decal 0 alpha, decal 1 RGB, decal 1 alpha; their biases
		xMath::Vec4 drape_decal_key[4] = {};
		xMath::Vec4 drape_decal_bias = xMath::Vec4{0.0f, 0.0f, 0.0f, 0.0f};
	};

	/**
	 * @struct ShaderBuffer_Light
	 * @brief Light parameters for GPU shaders
	 */
	struct ShaderBuffer_Light
	{
		xMath::Vec4 color; // rgba in memory order (xMath::Color stores its components reversed, a b g r)
		xMath::Vec3 position;
		float intensity;
		xMath::Vec3 direction;
		float range;
		float angle;
		uint32_t flags;
		uint32_t screen_space_shadows_slice_index;
		float area_width;  // area light width in meters
		float area_height; // area light height in meters
		float padding_matrices[3] = {}; // std430 aligns the matrix array to 16 bytes
		xMath::Matrix view_projection[6];
		xMath::Vec2 atlas_offsets[6];
		xMath::Vec2 atlas_scales[6];
		xMath::Vec2 atlas_texel_sizes[6];
	};

	/**
	 * @struct ShaderBuffer_Aabb
	 * @brief Axis-aligned bounding box (AABB) data for culling and collision detection
	 */
	struct ShaderBuffer_Aabb
	{
		xMath::Vec3 min;
		float is_occluder;
		xMath::Vec3 max;
		float padding2;
	};

	/**
	 * @struct ShaderBuffer_GeometryInfo
	 * @brief per-blas-instance offsets into the global geometry buffer (indexed by InstanceIndex() in rt shaders)
	 */
	struct ShaderBuffer_GeometryInfo
	{
		uint32_t vertex_offset;
		uint32_t index_offset;
	};

	/**
	 * @struct ShaderBuffer_IndirectDrawArgs
	 * @brief gpu-driven indirect draw arguments (matches VkDrawIndexedIndirectCommand layout) 
	 */
	struct ShaderBuffer_IndirectDrawArgs
	{
		uint32_t index_count    = 0;
		uint32_t instance_count = 0;
		uint32_t first_index    = 0;
		int32_t  vertex_offset  = 0;
		uint32_t first_instance = 0;
	};

	/**
	 * @struct ShaderBuffer_DrawData
	 * @brief per-draw data for gpu-driven rendering (indexed by draw_id in shaders)
	 */
	struct ShaderBuffer_DrawData
	{
		Matrix transform;          // current world transform
		Matrix transform_previous; // previous frame world transform
		uint32_t material_index = 0;     // index into the bindless material parameters array
		uint32_t is_transparent = 0;     // transparency flag
		uint32_t aabb_index     = 0;     // index into the aabb buffer for culling
		uint32_t pick_id        = 0;     // editor picking: entity index + 1 (ScenePicking::GetPickId), 0 = not pickable
		uint32_t drape_rank     = 0;     // position among this frame's visible drapes (1 = lowest), 0 = not draped
		uint32_t padding[3]     = {};
	};

	/**
	 * @struct ShaderBuffer_Particle
	 * @brief GPU particle parameters (matches hlsl Particle struct, 64 bytes)
	 */
	struct ShaderBuffer_Particle
	{
		xMath::Vec3 position;
		float lifetime      = 0.0f; // remaining lifetime
		xMath::Vec3 velocity;
		float max_lifetime  = 0.0f; // initial lifetime
		xMath::Vec4 color;			// current rgba
		float size          = 0.0f; // current size
		float padding1      = 0.0f;
		float padding2      = 0.0f;
		float padding3      = 0.0f;
	};

	/**
	 * @struct PersistentLine
	 * @brief Represents a line that persists for a specified duration, 
	 * used for debug drawing in the editor. 
	 * Each line has a start and end point, colors for both ends, and an expiration time after which it will be removed from the render list.
	 */
	struct PersistentLine
	{
		xMath::Vec3 from;
		xMath::Vec3 to;
		Color color_from;
		Color color_to;
		double expire_time;
	};

	/**
	 * @struct ShaderBuffer_EmitterParams
	 * @brief GPU emitter parameters (matches hlsl EmitterParams struct) 
	 */
	struct ShaderBuffer_EmitterParams
	{
		xMath::Vec3 position;
		float emission_rate   = 0.0f;
		float lifetime        = 0.0f;
		float start_speed     = 0.0f;
		float start_size      = 0.0f;
		float end_size        = 0.0f;
		xMath::Color start_color;
		xMath::Color end_color;
		float gravity_modifier = 0.0f;
		float radius           = 0.0f;
		float delta_time       = 0.0f;
		uint32_t max_particles = 0;
		uint32_t frame         = 0;
		uint32_t emitter_count = 0; // number of active emitters this frame
		float padding1         = 0.0f;
		float padding2         = 0.0f;
	};

	/**
	 * @struct ConstantBuffer_Environment
	 * @brief The world environment for the shaders (plan TASK-020), filled once per frame from Environment::Get():
	 * sun and moon, sky-condition multipliers, atmosphere, haze, surface weather, season, exposure, clouds, wind, the
	 * sun-shadow cascades. Vec4 and matrix members only (std140). Mirrors EnvironmentData in resources.slang (binding
	 * 201). World frame: +X east, +Y up, +Z south. Units: lux, meters, 1/m.
	 */
	struct ConstantBuffer_Environment
	{
		xMath::Vec4 sun_direction;			// xyz toward the sun, w angular radius (rad)
		xMath::Vec4 sun_illuminance;		// rgb illuminance at the observer, after the atmosphere (lux), w apparent elevation (deg)
		xMath::Vec4 moon_direction;			// xyz toward the moon, w illuminated fraction
		xMath::Vec4 moon_illuminance;		// rgb at the observer (lux, full moon x fraction x atmosphere), w phase angle (rad)
		xMath::Vec4 sky_multipliers;		// ambient, direct, tone, shadow level (sky-condition blend)
		xMath::Vec4 sky_multipliers2;		// rayleigh, mie, shadow offset, daylight (0 night .. 1 day)
		xMath::Vec4 rayleigh;				// rgb scattering (1/m), w scale height (m)
		xMath::Vec4 mie;					// scattering, extinction (1/m), scale height (m), phase g
		xMath::Vec4 ozone;					// rgb absorption (1/m), w tent center altitude (m)
		xMath::Vec4 planet;					// ozone tent half width (m), earth radius (m), atmosphere top radius (m), ground albedo
		xMath::Vec4 haze;					// visibility (m), haze layer top (m), haze fade band (m), time (s)
		xMath::Vec4 weather;				// thresholds a surface's receptivity must pass (weather.slang): rain, ice, snow, puddles
		xMath::Vec4 snow_projection_x;		// world xz -> snow coverage map u (Phase 12)
		xMath::Vec4 snow_projection_z;		// world xz -> snow coverage map v
		xMath::Vec4 season;					// ratio toward the next season, season index (0 spr .. 3 win), day of year, latitude
		xMath::Vec4 exposure;				// exposure scale, pre-exposure, metered EV100, applied EV
		xMath::Vec4 exposure_fusion;		// shadows, sigma, near distance, far distance
		xMath::Vec4 camera;					// camera altitude MSL (m), ground elevation (m), night factor, rain percent
		xMath::Vec4 clouds[3];				// per layer: base MSL (m), top MSL (m), coverage, type
		xMath::Vec4 wind;					// direction from (deg true), speed (m/s), wave amplitude (m), wave direction (deg true)
		xMath::Vec4 debug;					// x debug view (Renderer_DebugView), y feature bits, z temperature (C), w unused

		// The sky (environment/sky_light.h): radiance without the Mie lobe and the Mie single scattering with its
		// phase factored out (nits), straight up, at the horizon toward / 90 degrees off / away from the sun's azimuth,
		// then 30 degrees up at the same azimuths; w unused
		xMath::Vec4 sky_radiance[7];
		xMath::Vec4 sky_mie[7];
		xMath::Vec4 sky_irradiance;			// rgb sky irradiance on a level surface (lux), w ground albedo

		// Exposure (exposure_adapt.slang, renderer/exposure.h)
		xMath::Vec4 exposure_meter;			// EV100 min, EV100 max, meter constant K, centre-weight radius (screen heights)
		xMath::Vec4 exposure_adapt;			// speed up, speed down (EV/s), lower trim, upper trim (histogram fractions)
		xMath::Vec4 exposure_target;		// photometric/ev100_mid and ev100_bias for the sun's elevation, gain_lo, gain_hi
		xMath::Vec4 exposure_extra;			// x exposure compensation (EV, positive brightens), yzw unused
		xMath::Vec4 atmosphere_scattering;	// rgb Mie spectral factor (scattering/mie_*_exponent; green 1), w scattering/multi_rat

		// The SDR grade (color_management/sdr/*, ASC-CDL in scene linear around middle grey: the day, night and snow grades
		// already blended on the CPU) and the post chain
		xMath::Vec4 color_slope;			// rgb slope, w saturation
		xMath::Vec4 color_offset;			// rgb offset, w output gain
		xMath::Vec4 color_power;			// rgb power, w output gamma (1: none)
		xMath::Vec4 post;					// bloom intensity, exposure fusion highlights (EV), fusion strength, unused

		// The atmosphere (plan Phase 6, atmosphere_common.slang)
		xMath::Vec4 light_toa;				// rgb sun illuminance outside the atmosphere (lux), w the moon's (lux, x illuminated fraction)
		xMath::Vec4 atmosphere_haze;		// haze extinction at the ground (1/m, extra Mie, green), fog scattering albedo, layer top above the ground (m), fade band (m)
		xMath::Vec4 atmosphere_view;		// aerial perspective range (m), airglow (nits), star brightness scale, fog extinction at the ground (1/m)
		xMath::Vec4 probe;					// xyz environment probe 0 (world), w specular LOD bias
		xMath::Vec4 celestial[3];			// rows of the world -> equatorial rotation (x vernal equinox, z north pole): stars

		// Sun shadows (plan Phase 7, renderer/csm.h, csm_sample.slang): the light's view-projection per cascade, stored like
		// the frame's matrices (the rows of the column-vector matrix, see GpuMatrix), where each cascade ends and blends
		// into the next, and the filter. The cloud shadow map's cascades follow (Pass_CloudShadow).
		xMath::Matrix csm_view_projection[4];
		xMath::Vec4 csm_split;				// view depth (m) where cascade 0..3 ends
		xMath::Vec4 csm_blend;				// depth (m) over which cascade 0..3 blends into the next before it ends
		xMath::Vec4 csm_texel;				// shadow-map texel size (m) of cascade 0..3
		xMath::Vec4 csm_params;				// cascade count (0: none), fade start and end (view depth, m), shadow light (0 none, 1 sun, 2 moon)
		xMath::Vec4 csm_filter;				// normal offset (texels), slope-scaled normal offset (texels), soft (3x3 PCF), map size
		xMath::Vec4 cloud_shadow;			// cascade count (0: none), strength (0 none .. 1 full), extinction (1/m of cloud cover), march steps per layer
		xMath::Vec4 cloud_shadow_cascades[2];	// per cascade: center x, center z (world, m), half extent (m), texel size (m)

		// Screen-space reflections (plan Phase 9, renderer/ssr.h, ssr_common.slang)
		xMath::Vec4 ssr;					// ssr/min_gloss, sedx/ssr_gloss_fade, ssr/lod_bias, ssr/use_prefiltered (0 / 1)
		xMath::Vec4 ssr_trace;				// max Hi-Z steps, thickness (m), thickness per meter of view depth, edge fade (share of the screen)
		xMath::Vec4 ssr_history;			// history clamp (pre-exposed), lobe half width per GGX alpha, trace every pixel (debug), SSR active (0 / 1)

		// Water (plan Phase 11, renderer/ocean.h, ocean_common.slang, water.slang): the FFT ocean's sea state and cascades, the
		// water's surface, volume and foam
		xMath::Vec4 ocean;					// time (s), choppiness (water/h_displace_lambda), height scale, wave travel direction (rad, atan2(z, x))
		xMath::Vec4 ocean_cascade[4];		// per cascade: tile size (m), first and past-last wave number (rad/m), water/cascade_N_scale
		xMath::Vec4 ocean_sea;				// wind speed (m/s), swell, spread blend, short waves fade (m)
		xMath::Vec4 ocean_jonswap;			// alpha, peak omega (rad/s), gamma, spectrum depth (m)
		xMath::Vec4 water_surface;			// F0, surface roughness (perceptual, from water/gloss), refractive index, turbidity on (0 / 1)
		xMath::Vec4 water_volume;			// water/turbidity, turbidity cutoff (ln), clearest-water floor (m), shore depth (m)
		xMath::Vec4 water_foam;				// foam bias, ratio, scale, share fading per second
		xMath::Vec4 water_floor;			// floor albedo rgb, water/sea_floor_coef (share of the floor seen)
		xMath::Vec4 water_tessellation;		// target edge pixels, max factor, pond wave scale (0: fetch ignored), frame time step (s)

		// X-Plane 12's display transform (tonemap.slang xp12_display)
		xMath::Vec4 tonemap;				// middle grey, working-space compression (color_management/compression), display maximum, unused

		// Weather on surfaces (plan Phase 12, renderer/weather_surface.h, weather.slang); the thresholds are in weather
		xMath::Vec4 weather_rain;			// 1 / noise repeat (1/m), threshold edge width, receptivity added before the noise, darkening scale
		xMath::Vec4 weather_darkening;		// darkening start, end (wetness), bright-surface relief, albedo ratio
		xMath::Vec4 weather_darkening_gloss;	// gloss ramp start, end, multiplier for dark albedo, for light albedo
		xMath::Vec4 weather_water;			// share of the water film's gloss a wet surface takes, the film's gloss, unused, unused
		xMath::Vec4 weather_snow;			// 1 / noise repeat (1/m), threshold edge width, noise depth, unused
		xMath::Vec4 weather_snow_detail;	// slope min, slope max (up . normal), decal 1 (colour) weight, decal 2 (edge) weight
		xMath::Vec4 weather_snow_scale;		// 1 / repeat (1/m) of the albedo, normal and decal textures, unused
		xMath::Vec4 weather_ice;			// 1 / noise repeat (1/m), threshold edge width, decal 1 weight, decal 2 weight
		xMath::Vec4 weather_ice_scale;		// 1 / repeat (1/m) of the albedo, normal and decal textures, unused
		xMath::Vec4 weather_snow_luma;		// snow receptivity without a WEATHER mask: weights for the linear albedo's r, g, b, a
		xMath::Vec4 weather_textures;		// bindless material slot of the weather art (Renderer weather material), art loaded (0 / 1), unused, unused
		xMath::Vec4 padding[7];				// to 1792 bytes
	};

	// sizes the shaders see (resources.slang: cbuffer std140, structured buffers std430)
	static_assert(sizeof(ConstantBuffer_Frame) == 1024, "ConstantBuffer_Frame must match FrameBufferData in resources.slang");
	static_assert(sizeof(ConstantBuffer_Environment) == 1792, "ConstantBuffer_Environment must match EnvironmentData in resources.slang");
	static_assert(sizeof(ShaderBuffer_Material) == 368, "ShaderBuffer_Material must match MaterialParameters in resources.slang");
	static_assert(sizeof(ShaderBuffer_Light) == 608, "ShaderBuffer_Light must match LightParameters in resources.slang");
	static_assert(sizeof(ShaderBuffer_DrawData) == 160, "ShaderBuffer_DrawData must match DrawData in resources.slang");

}

// -------------------------------------------------------

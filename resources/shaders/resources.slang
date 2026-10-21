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
 * resources.slang
 * -------------------------------------------------------
 * Created: 06/03/2026
 * -------------------------------------------------------
 */

// a constant buffer that updates once per frame
public struct FrameBufferData
{
    public matrix view;
    public matrix view_inverted;
    public matrix view_previous;
    public matrix projection;
    public matrix projection_inverted;
    public matrix projection_previous;
    public matrix view_projection;
    public matrix view_projection_inverted;
    public matrix view_projection_orthographic;
    public matrix view_projection_unjittered;
    public matrix view_projection_previous;
    public matrix view_projection_previous_unjittered;

    public float2 resolution_render;
    public float2 resolution_output;

    public float2 taa_jitter_current;
    public float2 taa_jitter_previous;

    public float camera_aperture;
    public float delta_time;
    public uint frame;
    public uint options;

    public float3 camera_position;
    public float camera_near;

    public float3 camera_forward;
    public float camera_far;

    public float camera_last_movement_time;
    public float hdr_enabled;
    public float hdr_max_nits;
    public float padding;

    public float3 camera_position_previous;
    public float resolution_scale;

    public double time;
    public float camera_fov;
    public float padding2;

    public float3 wind;
    public float gamma;

    public float3 camera_right;
    public float camera_exposure;

    // clouds
    public float cloud_coverage;
    public float cloud_shadows;
    public float padding3;
    public float padding4;

    // pads the struct to 1024 bytes (dynamic uniform offset alignment); mirrors ConstantBuffer_Frame
    public float4 padding_alignment[5];
};

// push constant buffer - carries per-draw and per-pass data
// draw_index indexes into the bindless draw data buffer for transforms and material info
// material_index and is_transparent are pass-level state for compute shaders
// values[] carries generic per-pass parameters (3 x float4)
public struct PassBufferData
{
    public uint draw_index;
    public uint material_index;
    public uint is_transparent;
    public uint padding;
    public float4 values[3];
};

// struct which forms the bindless material parameters array
public struct MaterialParameters
{
    public float4 color;

    public float2 tiling;
    public float2 offset;
    public float2 invert_uv;

    public float roughness;
    public float metallness;
    public float normal;
    public float height;

    public uint flags;
    public float local_width;
    public float alpha_cutoff; // alpha below this is cut out of the deferred passes, 0 = get_alpha_threshold()
    public float subsurface_scattering;

    public float sheen;
    public float local_height;
    public float world_space_uv;
    public float blend_mode; // 0 opaque / alpha tested, 1 alpha blended, 2 glass (forward_transparent.slang)

    public float anisotropic;
    public float anisotropic_rotation;
    public float clearcoat;
    public float clearcoat_roughness;
    public float2 padding3;

    // X-Plane materials (xp_material.slang): specular gain, super roughness floor, F0 when the normal map carries
    // none, emissive luminance in nits (0: emissive texels are display values)
    public float4 xp;

    public bool has_texture_albedo() { return (flags & (1 << 2)) != 0; }
    public bool has_texture_normal() { return (flags & (1 << 1)) != 0; }
    public bool has_texture_occlusion() { return (flags & (1 << 7)) != 0; }
    public bool has_texture_roughness() { return (flags & (1 << 3)) != 0; }
    public bool has_texture_metalness() { return (flags & (1 << 4)) != 0; }
    public bool has_texture_emissive() { return (flags & (1 << 6)) != 0; }
    public bool emissive_from_albedo() { return (flags & (1 << 15)) != 0; }
    public uint xp_normal_mode() { return (flags >> 16) & 3u; }	// xp_material.slang xp_normal_*
    public bool is_xp_material() { return (flags & (1 << 18)) != 0; }
};

// struct which forms the bindless light parameters array
public struct LightParameters
{
    public float4 color;
    public float3 position;
    public float intensity;
    public float3 direction;
    public float range;
    public float angle;
    public uint flags;
    public uint screen_space_shadow_slice_index;
    public float area_width;  // area light width in meters
    public float area_height; // area light height in meters
    public matrix transform[6];
    public float2 atlas_offsets[6];
    public float2 atlas_scales[6];
    public float2 atlas_texel_sizes[6];
};

public struct aabb
{
    public float3 min;
    public float is_occluder;
    public float3 max;
    public float padding2;
};

// g-buffer
public [[vk::binding(0)]] Texture2D tex_albedo : register(t0);
public [[vk::binding(1)]] Texture2D tex_normal : register(t1);
public [[vk::binding(2)]] Texture2D tex_material : register(t2);
public [[vk::binding(3)]] Texture2D tex_velocity : register(t3);
public [[vk::binding(4)]] Texture2D tex_depth : register(t4);
public [[vk::binding(5)]] Texture2D tex_emissive : register(t5);	// emissive nits x pre-exposure
public [[vk::binding(54)]] Texture2D tex_weather : register(t54);	// weather receptivity (rain, puddles, snow, ice)

// other
public [[vk::binding(6)]] Texture2D tex_ssao : register(t6);

// misc
public [[vk::binding(7)]] Texture2D tex : register(t7);
public [[vk::binding(8)]] Texture2D tex2 : register(t8);
public [[vk::binding(9)]] Texture2D tex3 : register(t9);
public [[vk::binding(10)]] Texture2D tex4 : register(t10);
public [[vk::binding(11)]] Texture2D tex5 : register(t11);
public [[vk::binding(12)]] Texture2D tex6 : register(t12);
public [[vk::binding(13)]] Texture3D tex3d : register(t13);

// noise
public [[vk::binding(14)]] Texture2D tex_perlin : register(t14);

// volumetric cloud 3D noise textures
public [[vk::binding(52)]] Texture3D tex3d_cloud_shape : register(t52);  // 128^3 Perlin-Worley + Worley FBM
public [[vk::binding(53)]] Texture3D tex3d_cloud_detail : register(t53); // 32^3 high-frequency detail

// bindless arrays
public [[vk::binding(15, 1)]] Texture2D material_textures[] : register(t15, space1);
public [[vk::binding(16, 2)]] StructuredBuffer<MaterialParameters> material_parameters : register(t16, space2);
public [[vk::binding(17, 3)]] StructuredBuffer<LightParameters> light_parameters : register(t17, space3);
public [[vk::binding(18, 4)]] StructuredBuffer<aabb> aabbs : register(t18, space4);
public [[vk::binding(0, 6)]] SamplerComparisonState samplers_comparison[] : register(s0, space6);
public [[vk::binding(1, 7)]] SamplerState samplers[] : register(s1, space7);

// storage textures/buffers (image_format unknown allows flexible format binding)
public [[vk::binding(100)]] [[vk::image_format("unknown")]] RWTexture2D<float4> tex_uav : register(u100);
public [[vk::binding(101)]] [[vk::image_format("unknown")]] RWTexture2D<float4> tex_uav2 : register(u101);
public [[vk::binding(102)]] [[vk::image_format("unknown")]] RWTexture2D<float4> tex_uav3 : register(u102);
public [[vk::binding(103)]] [[vk::image_format("unknown")]] RWTexture2D<float4> tex_uav4 : register(u103);
public [[vk::binding(104)]] [[vk::image_format("unknown")]] RWTexture3D<float4> tex3d_uav : register(u104);
public [[vk::binding(105)]] [[vk::image_format("unknown")]] RWTexture2DArray<float4> tex_uav_sss : register(u105);
public [[vk::binding(143)]] RWStructuredBuffer<uint> visibility : register(u143);                        // unused, kept for descriptor layout stability
public [[vk::binding(144)]] globallycoherent RWStructuredBuffer<uint> g_atomic_counter : register(u144); // used by FidelityFX SPD
public [[vk::binding(108)]] [[vk::image_format("unknown")]] globallycoherent RWTexture2D<float4> tex_uav_mips[12] : register(u108); // used by FidelityFX SPD

// integer format textures (vrs, etc)
public [[vk::binding(130)]] RWTexture2D<uint> tex_uav_uint : register(u130);

// gpu-driven indirect drawing
public struct IndirectDrawArgs
{
    public uint index_count;
    public uint instance_count;
    public uint first_index;
    public int vertex_offset;
    public uint first_instance;
};

public struct DrawData
{
    public matrix transform;
    public matrix transform_previous;
    public uint material_index;
    public uint is_transparent;
    public uint aabb_index;
    public uint pick_id;    // editor picking: entity index + 1, 0 = not pickable (gbuffer_pick, ScenePicking)
    public uint drape_rank; // position among the frame's visible drapes (1 = lowest), 0 = not draped (apply_drape_depth)
    public uint padding0;   // scalars: a uint3 would be 16-byte aligned under std430 and make the stride 176, not 160
    public uint padding1;
    public uint padding2;
};

// bindless draw data - per-draw transforms, material indices, etc.
// set 5 is the BindlessResource::DrawData set bound by BindlessManager (a bare vk::binding(19) would put it in set 0)
public [[vk::binding(19, 5)]] StructuredBuffer<DrawData> draw_data : register(t19, space5);

// vertex pulling - the global geometry buffer as raw floats: Vertex_PosTexNorTan is 11 tightly packed floats
// (position, uv, normal, tangent; 44 bytes), which a std430 struct of float3s (64-byte stride) can't express
public static const uint PULLED_VERTEX_FLOATS = 11;

// vertex pulling - instance buffer exposed as packed uint data (10 bytes per instance)
public struct PackedInstance
{
    public uint pos_xy;     // position_x (half16) | position_y (half16)
    public uint pos_z_norm; // position_z (half16) | normal_oct (uint16)
    public uint yaw_scale;  // yaw_packed (uint8) | scale_packed (uint8) | padding (uint16)
};

public [[vk::binding(20, 8)]] StructuredBuffer<float> geometry_vertices : register(t20, space8);
public [[vk::binding(22, 9)]] StructuredBuffer<uint> geometry_indices : register(t22, space9);
public [[vk::binding(23, 10)]] StructuredBuffer<PackedInstance> geometry_instances : register(t23, space10);

// gpu-driven indirect drawing uav bindings
// input: populated by cpu, read by the cull compute shader
public [[vk::binding(131)]] RWStructuredBuffer<IndirectDrawArgs> indirect_draw_args : register(u131);
public [[vk::binding(132)]] RWStructuredBuffer<DrawData> indirect_draw_data : register(u132);
// output: written by the cull compute shader, read by vertex shaders
public [[vk::binding(133)]] RWStructuredBuffer<IndirectDrawArgs> indirect_draw_args_out : register(u133);
public [[vk::binding(134)]] RWStructuredBuffer<DrawData> indirect_draw_data_out : register(u134);
public [[vk::binding(135)]] RWStructuredBuffer<uint> indirect_draw_count : register(u135);

// gpu-driven particle system
public struct Particle
{
    float3 position;
    float lifetime; // remaining
    float3 velocity;
    float max_lifetime; // initial
    float4 color;       // current rgba
    float size;         // current
    float3 padding;
};

public struct EmitterParams
{
    float3 position;
    float emission_rate;
    float lifetime;
    float start_speed;
    float start_size;
    float end_size;
    float4 start_color;
    float4 end_color;
    float gravity_modifier;
    float radius;
    float delta_time;
    uint max_particles;
    uint frame;
    uint emitter_count;
    float padding1;
    float padding2;
};

[[vk::binding(136)]] RWStructuredBuffer<Particle> particle_buffer_a : register(u136);
[[vk::binding(138)]] RWStructuredBuffer<uint> particle_counter : register(u138);
[[vk::binding(139)]] RWStructuredBuffer<EmitterParams> particle_emitter : register(u139);

// gpu texture compression
[[vk::binding(140)]] RWStructuredBuffer<uint> tex_compress_in : register(u140);
[[vk::binding(141)]] RWStructuredBuffer<uint4> tex_compress_out : register(u141);     // bc3, bc5 (16 bytes per block)
[[vk::binding(142)]] RWStructuredBuffer<uint2> tex_compress_out_bc1 : register(u142); // bc1 (8 bytes per block)

// buffers
[[vk::push_constant]]
public PassBufferData buffer_pass;
public [[vk::binding(200)]] cbuffer BufferFrame : register(b200) { FrameBufferData buffer_frame; };

// the world environment (ConstantBuffer_Environment, renderer_buffers.h): sun, moon, sky, atmosphere, weather, exposure
// world frame +X east, +Y up, +Z south; lux, meters, 1/m
public struct EnvironmentData
{
    public float4 sun_direction;     // xyz toward the sun, w angular radius (rad)
    public float4 sun_illuminance;   // rgb lux outside the atmosphere, w apparent elevation (deg)
    public float4 moon_direction;    // xyz, w illuminated fraction
    public float4 moon_illuminance;  // rgb lux, w phase angle (rad)
    public float4 sky_multipliers;   // ambient, direct, tone, shadow level
    public float4 sky_multipliers2;  // rayleigh, mie, shadow offset, daylight
    public float4 rayleigh;          // rgb scattering, w scale height
    public float4 mie;               // scattering, extinction, scale height, g
    public float4 ozone;             // rgb absorption, w tent center altitude
    public float4 planet;            // ozone half width, earth radius, atmosphere radius, ground albedo
    public float4 haze;              // visibility, haze top, haze band, time
    public float4 weather;           // rain wetness, ice, snow level, puddles
    public float4 snow_projection_x;
    public float4 snow_projection_z;
    public float4 season;            // ratio, index, day of year, latitude
    public float4 exposure;          // exposure scale, pre-exposure, metered EV100, applied EV
    public float4 exposure_fusion;   // shadows, sigma, near, far
    public float4 camera;            // altitude MSL, ground elevation, night factor, rain percent
    public float4 clouds[3];         // base MSL, top MSL, coverage, type
    public float4 wind;              // direction from (deg), speed (m/s), wave amplitude (m), wave direction (deg)
    public float4 debug;             // debug view, feature bits, temperature (C)
    public float4 sky_radiance[7];   // nits without the Mie lobe: zenith, horizon toward / side / away from the sun, 30 deg up (same)
    public float4 sky_mie[7];        // Mie single scattering with the phase factored out (same directions)
    public float4 sky_irradiance;    // rgb lux on a level surface, w ground albedo
    public float4 exposure_meter;    // EV100 min, max, K, centre-weight radius
    public float4 exposure_adapt;    // speed up, speed down (EV/s), lower trim, upper trim
    public float4 exposure_target;   // ev100_mid and ev100_bias for the sun's elevation, gain below mid, gain above
    public float4 exposure_extra;    // x compensation (EV, positive brightens)
    public float4 atmosphere_scattering; // rgb Mie spectral factor (Angstrom exponent, green 1), w multiple-scattering ratio
    public float4 color_slope;       // ASC-CDL rgb slope, w saturation
    public float4 color_offset;      // rgb offset, w output gain
    public float4 color_power;       // rgb power, w output gamma
    public float4 post;              // bloom intensity, exposure fusion highlights (EV), fusion strength
    public float4 light_toa;         // rgb sun illuminance outside the atmosphere (lux), w the moon's (lux, x fraction)
    public float4 atmosphere_haze;   // haze extinction (1/m), scattering albedo, layer top above the ground (m), fade band (m)
    public float4 atmosphere_view;   // aerial perspective range (m), airglow (nits), star brightness, unused
    public float4 probe;             // xyz environment probe 0 (world), w specular LOD bias
    public float4 celestial[3];      // rows of the world -> equatorial rotation (x vernal equinox, z north pole)
    public float4 padding_environment[10];
};
public [[vk::binding(201)]] cbuffer BufferEnvironment : register(b201) { public EnvironmentData buffer_environment; };

public float3 get_sun_direction() { return buffer_environment.sun_direction.xyz; }
public float3 get_sun_illuminance() { return buffer_environment.sun_illuminance.rgb; }
public float3 get_moon_direction() { return buffer_environment.moon_direction.xyz; }
public float3 get_moon_illuminance() { return buffer_environment.moon_illuminance.rgb; }
public float get_exposure_scale() { return buffer_environment.exposure.x; }
public float get_pre_exposure() { return buffer_environment.exposure.y; }

// Exposure (renderer/exposure.h): the log2-luminance histogram luminance_histogram.slang fills and exposure_adapt.slang
// meters and clears, and the exposure state exposure_adapt.slang keeps (EXPOSURE_STATE_*)
public [[vk::binding(146)]] RWStructuredBuffer<uint> exposure_histogram : register(u146);
public [[vk::binding(147)]] RWStructuredBuffer<float> exposure_state : register(u147);
public static const uint EXPOSURE_HISTOGRAM_BINS = 256;
public static const float EXPOSURE_LOG2_MIN = -2.0f;
public static const float EXPOSURE_LOG2_MAX = 18.0f;
public static const uint EXPOSURE_STATE_EV = 0;          // EV100 the frame is exposed at (adapted, bias applied)
public static const uint EXPOSURE_STATE_METERED = 1;     // metered EV100, clamped to the range (before the bias)
public static const uint EXPOSURE_STATE_TARGET = 2;      // EV100 the adaptation moves toward
public static const uint EXPOSURE_STATE_MEAN_LOG2 = 3;   // mean log2 luminance (nits) between the trims
public static const uint EXPOSURE_STATE_VALID = 4;       // 1 once the first frame has been metered
public static const uint EXPOSURE_STATE_RAW = 5;         // metered EV100 before the clamp
public static const uint EXPOSURE_STATE_FLOATS = 8;

/* @brief What a frame exposed at an EV100 is multiplied by: 1 / (1.2 x 2^EV) (renderer/exposure.h). */
public float exposure_scale_from_ev(float ev100) { return 18.0f / (max(buffer_environment.exposure_meter.z, 1e-3f) * exp2(ev100)); }
public uint get_debug_view() { return uint(buffer_environment.debug.x + 0.5f); }
public bool is_feature_enabled(uint bit) { return (uint(buffer_environment.debug.y + 0.5f) & (1u << bit)) != 0; }

// easy access to buffer_frame members
public bool is_hdr_enabled() { return buffer_frame.hdr_enabled != 0.0f; }
public uint get_frame_index() { return buffer_frame.frame; }
public float get_camera_near_plane() { return buffer_frame.camera_near; }
public float get_camera_far_plane() { return buffer_frame.camera_far; }
public float2 get_resolution_render() { return buffer_frame.resolution_render; }
public float get_resolution_scale() { return buffer_frame.resolution_scale; }
public float3 get_camera_position() { return buffer_frame.camera_position; }
public float3 get_camera_forward() { return buffer_frame.camera_forward; }
public float3 get_wind_vector() { return buffer_frame.wind; }
public float get_cloud_coverage() { return buffer_frame.cloud_coverage; }
public float get_cloud_shadows_strength() { return buffer_frame.cloud_shadows; }
public float get_hdr_max_nits() { return buffer_frame.hdr_max_nits; }
public float get_camera_exposure() { return buffer_frame.camera_exposure; }
public double get_time_seconds() { return buffer_frame.time; }
public float get_delta_time_seconds() { return buffer_frame.delta_time; }
public matrix get_view_matrix() { return buffer_frame.view; }
public matrix get_view_inverted_matrix() { return buffer_frame.view_inverted; }
public matrix get_projection_matrix() { return buffer_frame.projection; }
public matrix get_view_projection_matrix() { return buffer_frame.view_projection; }
public matrix get_view_projection_ortho_matrix() { return buffer_frame.view_projection_orthographic; }
public matrix get_view_projection_unjittered_matrix() { return buffer_frame.view_projection_unjittered; }
public matrix get_view_projection_previous_matrix() { return buffer_frame.view_projection_previous; }
public matrix get_view_projection_inverted() { return buffer_frame.view_projection_inverted; }
public float2 get_taa_jitter_current() { return buffer_frame.taa_jitter_current; }
public float2 get_taa_jitter_previous() { return buffer_frame.taa_jitter_previous; }
public bool is_taa_enabled() { return any(buffer_frame.taa_jitter_current); }
public bool is_ray_traced_reflections_enabled() { return (buffer_frame.options & uint(1U << 0)) != 0; }
public bool is_ssao_enabled() { return (buffer_frame.options & uint(1U << 1)) != 0; }

// per-draw data is stored in a static so both vertex and pixel shaders can access it
// vertex shaders populate this from the appropriate buffer (draw_data or indirect_draw_data_out)
public static DrawData _draw;

// per-draw accessors - read from the static draw data populated by the vertex shader entry point
public matrix pass_get_transform() { return _draw.transform; }
public matrix pass_get_transform_previous() { return _draw.transform_previous; }
public uint pass_get_material_index() { return _draw.material_index; }

// pass-level state - read from push constant (works in both raster and compute shaders)
public bool pass_is_transparent() { return buffer_pass.is_transparent != 0; }
public bool pass_is_opaque() { return buffer_pass.is_transparent == 0; }

// generic pass parameter accessors - read from push constant values[]
// values[0].xyz = f3_value, values[0].w = f2_value.x
// values[1].xyz = f3_value2, values[1].w = f2_value.y
// values[2]     = f4_value
public float3 pass_get_f3_value() { return buffer_pass.values[0].xyz; }
public float3 pass_get_f3_value2() { return buffer_pass.values[1].xyz; }
public float4 pass_get_f4_value() { return buffer_pass.values[2]; }
public float2 pass_get_f2_value() { return float2(buffer_pass.values[0].w, buffer_pass.values[1].w); }

// helper to populate _draw from the appropriate source
public void pass_load_draw_data_from_buffer() { _draw = draw_data[buffer_pass.draw_index]; }
public void pass_load_draw_data_from_vertex(uint mi) { _draw.material_index = mi; } // pixel shader: restore material_index from vertex output

// bindless array indices
public static const uint material_texture_slots_per_type = 4;
public static const uint material_texture_index_albedo = 0 * material_texture_slots_per_type;
public static const uint material_texture_index_roughness = 1 * material_texture_slots_per_type;
public static const uint material_texture_index_metalness = 2 * material_texture_slots_per_type;
public static const uint material_texture_index_normal = 3 * material_texture_slots_per_type;
public static const uint material_texture_index_occlusion = 4 * material_texture_slots_per_type;
public static const uint material_texture_index_emission = 5 * material_texture_slots_per_type;
public static const uint material_texture_index_height = 6 * material_texture_slots_per_type;
public static const uint material_texture_index_mask = 7 * material_texture_slots_per_type;
public static const uint material_texture_index_packed = 8 * material_texture_slots_per_type;

public static const uint sampler_compare_depth = 0;
public static const uint sampler_point_clamp = 0;
public static const uint sampler_point_clamp_border = 1;
public static const uint sampler_point_wrap = 2;
public static const uint sampler_bilinear_clamp = 3;
public static const uint sampler_bilinear_clamp_border = 4;
public static const uint sampler_bilinear_wrap = 5;
public static const uint sampler_trilinear_clamp = 6;
public static const uint sampler_anisotropic_wrap = 7;

// bindless array access
#define GET_TEXTURE(index_texture) material_textures[pass_get_material_index() + index_texture]
public MaterialParameters GetMaterial() { return material_parameters[pass_get_material_index()]; }
#define GET_SAMPLER(index_sampler) samplers[index_sampler]



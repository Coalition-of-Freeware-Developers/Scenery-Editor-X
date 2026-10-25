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
 * g_buffer.slang
 * -------------------------------------------------------
 * Created: 06/03/2026
 * -------------------------------------------------------
 */
import constants;
import common;
import tesselation;
import vertex_processing;
import resources;
import noise;
import math;
import luminance;
import position;
import transforms;
import structs;
import color;
import terrain;
import xp_material;
import xp_terrain;
import weather;
#ifdef GPU_SCENE
import gpu_scene;
#endif


struct gbuffer
{
    float4 albedo : SV_Target0;
    float4 normal : SV_Target1;
    float4 material : SV_Target2;  // R lighting factor (0), G perceptual roughness, B metalness / F0 value, A occlusion
    float2 velocity : SV_Target3;
    uint pick : SV_Target4;        // gbuffer_pick (R32_UINT): editor picking id of the surface, 0 = none
    float4 emissive : SV_Target5;  // gbuffer_emissive: emissive luminance (nits) x pre-exposure
    float4 weather : SV_Target6;   // gbuffer_weather: rain / damp, puddles, snow (0.5 + 0.5 s), ice receptivity
};

// constants
static const float3 vegetation_greener = float3(0.05f, 0.4f, 0.03f);
static const float3 vegetation_yellower = float3(0.45f, 0.4f, 0.15f);
static const float3 vegetation_browner = float3(0.3f, 0.15f, 0.08f);
static const float3 grass_base = float3(0.03f, 0.055f, 0.02f);
static const float3 grass_tip = float3(0.05f, 0.09f, 0.03f);
static const float3 grass_var1 = float3(0.04f, 0.065f, 0.02f);
static const float3 grass_var2 = float3(0.035f, 0.04f, 0.015f);
static const float3 flower_base = float3(0.05f, 0.07f, 0.03f);
static const float3 flower_blue = float3(0.529f, 0.808f, 0.922f);
static const float3 flower_red = float3(0.8f, 0.2f, 0.2f);
static const float3 flower_yellow = float3(0.9f, 0.8f, 0.1f);

// rotate uv around center (0.5, 0.5) by angle
static float2 rotate_uv(float2 uv, float angle)
{
    float cos_a = cos(angle);
    float sin_a = sin(angle);
    float2 centered = uv - 0.5f;
    return float2(centered.x * cos_a - centered.y * sin_a, centered.x * sin_a + centered.y * cos_a) + 0.5f;
}

static float4 sample_material_texture(uint texture_index, float2 uv)
{
    return material_textures[pass_get_material_index() + texture_index].Sample(samplers[sampler_anisotropic_wrap], uv);
}

static float4 sample_reduce_tiling(uint texture_index, float2 uv, float3 world_pos)
{
    int2 tile_coords = int2(floor(world_pos.x), floor(world_pos.z));
    float angle = floor(hash(tile_coords) * 4.0f) * PI_HALF;
    float2 final_uv = frac(float2(tile_coords) + rotate_uv(frac(uv), angle));
    return sample_material_texture(texture_index, final_uv);
}

static float4 sample_texture(gbuffer_vertex vertex, uint texture_index, Surface surface, float3 world_pos)
{
    float4 color;

    if (surface.is_terrain() && !GetMaterial().is_xp_material())	// X-Plane terrain (.ter) shades in xp_terrain.slang
    {
        color = sample_reduce_tiling(texture_index, vertex.uv_misc.xy, world_pos);
        float4 tex_rock = sample_reduce_tiling(texture_index + 1, vertex.uv_misc.xy, world_pos);
        float4 tex_sand = sample_reduce_tiling(texture_index + 2, vertex.uv_misc.xy, world_pos);

        float surface_angle = acos(dot(vertex.normal, float3(0.0f, 1.0f, 0.0f)));
        float slope = saturate((surface_angle - 50.0f * DEG_TO_RAD) * 5.0f);
        float sand_factor = saturate((world_pos.y - sea_level) * 1.333f);

        color = lerp(lerp(tex_rock, color, 1.0f - slope), tex_sand, 1.0f - sand_factor);
    }
    else if (texture_index == material_texture_index_albedo && GetMaterial().xp_drape_mode() != xp_drape_plain)
    {
        color = xp_drape_sample_albedo(GetMaterial(), pass_get_material_index(), vertex.uv_misc.xy);	// tile / marking atlases
    }
    else if (texture_index == material_texture_index_normal && GetMaterial().is_xp_material())
    {
        color = xp_drape_sample_normal(GetMaterial(), pass_get_material_index(), vertex.uv_misc.xy);	// ratio, tile atlases
    }
    else
    {
        color = sample_material_texture(texture_index, vertex.uv_misc.xy);
    }

    return color;
}

// compute grass blade color with variation
float3 compute_grass_color(float height_percent, float variation)
{
    float t = smoothstep(0.2f, 1.0f, height_percent);
    float3 grass_tint = lerp(grass_base, grass_tip, t);

    // branchless color variation
    float3 var_color = lerp(grass_tint, grass_var1, step(0.33f, variation));
    var_color = lerp(var_color, grass_var2, step(0.66f, variation));
    return lerp(grass_tint, var_color, 0.045f); // 0.3 * 0.15 = 0.045
}

// compute flower color with cluster-based hue
float3 compute_flower_color(float height_percent, uint instance_id)
{
    uint cluster_id = instance_id / 5000u;
    float cluster_variation = hash(cluster_id);

    // branchless hue selection
    float3 tip = lerp(flower_blue, flower_red, step(0.33f, cluster_variation));
    tip = lerp(tip, flower_yellow, step(0.66f, cluster_variation));
    tip *= 0.9f + 0.1f * hash(instance_id * 13u);

    return lerp(flower_base, tip, smoothstep(0.2f, 1.0f, height_percent));
}

#if defined(GPU_SCENE)
// instanced indirect draws of the GPU scene (gpu_cull.slang picked the instances), vertices pulled from the global buffer
[shader("vertex")]
gbuffer_vertex main_vs(uint vertex_id: SV_VertexID, uint base_vertex: SV_StartVertexLocation, uint draw_instance: SV_InstanceID, uint base_instance: SV_StartInstanceLocation)
{
    const uint instance_id = gpu_scene_load_draw(draw_instance, base_instance);
    Vertex_PosUvNorTan input = pull_vertex(vertex_id + base_vertex);
#elif defined(INDIRECT_DRAW)
[shader("vertex")]
gbuffer_vertex main_vs(uint vertex_id: SV_VertexID, uint base_vertex: SV_StartVertexLocation, uint instance_id: SV_InstanceID, [[vk::builtin("DrawIndex")]] uint draw_id: DRAW_INDEX)
{
    _draw = indirect_draw_data_out[draw_id];
    Vertex_PosUvNorTan input = pull_vertex(vertex_id + base_vertex);
#else
[shader("vertex")]
gbuffer_vertex main_vs(Vertex_PosUvNorTan input, uint instance_id: SV_InstanceID)
{
    _draw = draw_data[buffer_pass.draw_index];
#endif

    float3 position_world = 0.0f;
    float3 position_world_previous = 0.0f;
    gbuffer_vertex vertex = transform_to_world_space(input, instance_id, _draw.transform, position_world, position_world_previous);
    vertex.material_index = _draw.material_index;
    vertex.pick_id = _draw.pick_id;
    return transform_to_clip_space(vertex, position_world, position_world_previous);
}

[shader("fragment")]
gbuffer main_frag(gbuffer_vertex vertex, bool is_front_face: SV_IsFrontFace)
{
    // restore material index from vertex output (works for both indirect and cpu-driven draws)
    pass_load_draw_data_from_vertex(vertex.material_index);

    // material setup
    MaterialParameters material = GetMaterial();
    Surface surface;
    surface.flags = material.flags;

    float4 albedo = material.color;
    float3 normal = vertex.normal.xyz;
    float occlusion = 1.0f;

    // velocity computation
    float2 position_ndc = uv_to_ndc(vertex.position.xy / (get_resolution_render() * get_resolution_scale()));
    float2 position_ndc_previous = vertex.position_previous.xy / vertex.position_previous.w;
    float2 position_ndc_jittered = position_ndc;
    position_ndc -= get_taa_jitter_current();
    position_ndc_previous -= get_taa_jitter_previous();
    float2 velocity = position_ndc - position_ndc_previous;

    // world position and distance
    float3 position_world = get_position(vertex.position.z, ndc_to_uv(position_ndc_jittered));
    float3 camera_to_pixel = position_world - get_camera_position();
    float distance = fast_sqrt(dot(camera_to_pixel, camera_to_pixel));

    // world space uv transformation
    if (any(material.world_space_uv))
    {
        float3 abs_normal = abs(normal);
        float2 uv_world = position_world.yz * abs_normal.x +
                          position_world.xz * abs_normal.y +
                          position_world.xy * abs_normal.z;
        uv_world = uv_world * material.tiling + material.offset;

        // branchless inversion
        float2 invert_mask = step(0.5f, material.invert_uv);
        vertex.uv_misc.xy = lerp(uv_world, 1.0f - frac(uv_world) + floor(uv_world), invert_mask);
    }

    // X-Plane terrain: its land class's textures, projected from above (xp_terrain.slang)
    const bool xp_terrain = xp_is_terrain(material);
    XpTerrainSurface terrain_surface;
    if (xp_terrain)
    {
        terrain_surface = xp_terrain_shade(material, pass_get_material_index(), position_world, vertex.normal, vertex.uv_misc.xy, distance);
        if (terrain_surface.cut)
            discard;
    }

    // albedo sampling
    float4 albedo_sample = 1.0f;
    if (xp_terrain)
    {
        albedo.rgb *= terrain_surface.albedo;
        albedo.a = 1.0f;
    }
    else if (surface.has_texture_albedo())
    {
        albedo_sample = sample_texture(vertex, material_texture_index_albedo, surface, position_world);
        albedo_sample.rgb = srgb_to_linear(albedo_sample.rgb);
        albedo *= albedo_sample;
        albedo.rgb = xp_drape_apply_detail_decals(material, pass_get_material_index(), vertex.uv_misc.xy, albedo.rgb);

        // Cut-out texels: the depth prepass discards them, but this pass only tests depth (greater-equal, no write),
        // so a cut-out texel with nothing nearer behind it would still be written solid (a decal over pavement, a
        // leaf against the sky). Apply the prepass's test here too.
        if (pass_is_opaque())
        {
            const float alpha_threshold = material.alpha_cutoff > 0.0f ? material.alpha_cutoff : get_alpha_threshold(position_world);
            if (material.alpha_cutoff > 0.0f ? albedo_sample.a < alpha_threshold : albedo_sample.a <= alpha_threshold)
                discard;
        }
    }

    // vegetation coloring
    if (surface.is_grass_blade())
    {
        uint instance_id = vertex.uv_misc.w;
        float height_percent = vertex.uv_misc.z;
        albedo.rgb = compute_grass_color(height_percent, hash(instance_id));
    }
    else if (surface.is_flower())
    {
        uint instance_id = vertex.uv_misc.w;
        float height_percent = vertex.uv_misc.z;
        albedo.rgb = compute_flower_color(height_percent, instance_id);
    }
    else if (surface.color_variation_from_instance())
    {
        float variation = hash((uint)vertex.uv_misc.w);
        float3 variation_tint = lerp(vegetation_greener, vegetation_yellower, step(0.25f, variation));
        variation_tint = lerp(variation_tint, vegetation_browner, step(0.5f, variation));
        albedo.rgb = lerp(albedo.rgb, variation_tint, 0.15f);
    }

    // alpha: opaque pass forces alpha to 1 for non-transparent pixels
    albedo.a = lerp(albedo.a, 1.0f, step(albedo_sample.a, 1.0f) * pass_is_opaque());

    // emission: X-Plane LIT textures are luminance in nits, pre-exposed like the lighting (never dimmed by daylight)
    float3 emissive = 0.0f;
    if (xp_terrain)
        emissive = terrain_surface.emissive;
    else
    {
        const float emissive_scale = xp_emissive_scale(material);
        if (surface.has_texture_emissive())
            emissive = srgb_to_linear(sample_material_texture(material_texture_index_emission, vertex.uv_misc.xy).rgb) * emissive_scale;
        if (material.emissive_from_albedo())
            emissive += albedo.rgb * emissive_scale;
    }

    // normal map: normal, roughness and metalness / F0 (xp_material.slang)
    const float4 normal_texel = xp_terrain ? terrain_surface.normal_texel
                              : (surface.has_texture_normal() ? sample_texture(vertex, material_texture_index_normal, surface, position_world) : float4(0.5f, 0.5f, 1.0f, 1.0f));
    const XpSurfaceMaterial surface_material = xp_decode_material(material, normal_texel);
    float roughness = surface_material.roughness;
    float metalness = surface_material.v;

    // normal mapping
    float distance_fade = 1.0f;
    if (surface.has_texture_normal())
    {
        float3 tangent_normal = surface_material.tangent_normal;

        // rotate normals to fake water waves/ripples
        if (surface.is_water())
        {
            static const float2 direction = float2(1.0f, 0.5f);
            static const float speed = 0.5f;
            float time = (float)get_time_seconds();
            float2 uv_offset = direction * time * speed;
            float2 noise_uv = vertex.uv_misc.xy + uv_offset;
            float noise = noise_perlin(noise_uv);
            float angle = noise * PI2;

            // rotate tangent normal.xy around z-axis (tangent space)
            float cos_a = cos(angle);
            float sin_a = sin(angle);
            float2 rotated_xy = float2(
                tangent_normal.x * cos_a - tangent_normal.y * sin_a,
                tangent_normal.x * sin_a + tangent_normal.y * cos_a
            );

            tangent_normal.xy = rotated_xy;
            float2 tangent_xy_sq = tangent_normal.xy * tangent_normal.xy;
            tangent_normal.z = fast_sqrt(max(0.0f, 1.0f - tangent_xy_sq.x - tangent_xy_sq.y));

            // flip if normal points down
            tangent_normal *= sign(tangent_normal.z);

            // fade normal texture beyond a certain distance to avoid high frequency noise from lower mips
            static const float fade_start = 200.0f;
            static const float fade_end = 500.0f;
            static const float fade_range = fade_end - fade_start;
            distance_fade = saturate((fade_end - distance) / fade_range);
        }

        tangent_normal.xy *= distance_fade;

        // normal decals (terrain, OBJ8) add their slopes to the normal map's (whiteout blend); terrain repeats them per meter of ground
        if (xp_drape_has_normal_decals(material))
        {
            const float2 decal_uv = xp_terrain ? float2(position_world.x, -position_world.z) : vertex.uv_misc.xy;
            const float3 decal = xp_drape_normal_decals(material, pass_get_material_index(), decal_uv);
            tangent_normal = normalize(float3(tangent_normal.xy + decal.xy, tangent_normal.z * decal.z));
        }

        float3x3 tangent_to_world = make_tangent_to_world_matrix(vertex.normal, vertex.tangent);
        normal = normalize(mul(tangent_normal, tangent_to_world).xyz);
    }

    // draped polygons (and terrain without a normal map): normal decals alone; terrain repeats them per meter of ground
    if (!surface.has_texture_normal() && xp_drape_has_normal_decals(material))
    {
        const float2 decal_uv = xp_terrain ? float2(position_world.x, -position_world.z) : vertex.uv_misc.xy;
        const float3 tangent_normal = xp_drape_normal_decals(material, pass_get_material_index(), decal_uv);
        normal = normalize(mul(tangent_normal, make_tangent_to_world_matrix(vertex.normal, vertex.tangent)).xyz);
    }

    // foliage curved normals
    if (surface.is_grass_blade() || surface.is_flower())
    {
        float curve_angle = clamp((vertex.width_percent - 0.5f) * 120.0f * DEG_TO_RAD, -PI * 0.5f, PI * 0.5f);

        float3 rotation_axis = normalize(cross(vertex.normal, vertex.tangent));
        float3x3 curvature_rotation = rotation_matrix(rotation_axis, curve_angle);
        normal = normalize(mul(curvature_rotation, normal));
        vertex.tangent = normalize(mul(curvature_rotation, vertex.tangent));

        // flip for back-faces (grass has no back-face geometry)
        float face_sign = is_front_face * 2.0f - 1.0f;
        normal *= face_sign;
        vertex.tangent *= face_sign;
    }

    // the editor's own materials: packed material texture (occlusion, roughness, metalness)
    if (!material.is_xp_material())
    {
        float4 packed = sample_texture(vertex, material_texture_index_packed, surface, position_world);
        occlusion = lerp(occlusion, packed.r, (float)material.has_texture_occlusion());
        roughness *= lerp(1.0f, packed.g, (float)material.has_texture_roughness());
        metalness = max(metalness * lerp(1.0f, packed.b, (float)material.has_texture_metalness()), 0.04f);
    }

    // specular anti-aliasing
    if (surface.has_texture_normal())
    {
        float3 dndu = ddx(normal);
        float3 dndv = ddy(normal);

        float variance = (dot(dndu, dndu) + dot(dndv, dndv)) / max(0.001f, dot(normal, normal));
        float adaptive = lerp(1.0f, 0.3f, saturate(distance * 0.1f));
        float roughness2 = roughness * roughness;
        roughness = fast_sqrt(saturate(roughness2 + min(variance * adaptive, 0.02f)));
    }

    // Double-sided geometry (ATTR_no_cull, two-sided facade walls) seen from behind: its normal faces the camera too, as
    // it would from the front, else the lighting and the ambient occlusion see a surface turned away from the viewer
    // (grass and flowers flipped theirs above; X-Plane trees keep their canopy normals on both faces)
    if (!is_front_face && !(surface.is_grass_blade() || surface.is_flower()) && !material.is_xp_foliage())
        normal = -normal;

    // Weather (weather.slang): the art's receptivity, then the environment's wet ground, puddles, snow and ice. A
    // WEATHER_TRANSPARENT surface takes none of its own (it keeps what lies under it only in the forward pass).
    float3 receptivity = float3(0.0f, 0.0f, -1.0f);
    if (!material.is_weather_transparent())
    {
        receptivity = weather_receptivity(material, pass_get_material_index(), vertex.uv_misc.xy, albedo);
        if (weather_active())
        {
            WeatherSurface weathered = weather_surface(albedo.rgb, roughness, metalness, normal);
            apply_surface_weather(weathered, position_world, receptivity);
            albedo.rgb = weathered.albedo;
            roughness = weather_roughness(weathered);
            metalness = weathered.v;
            normal = weathered.normal;
        }
    }

    // output
    gbuffer g_buffer;
    g_buffer.albedo = albedo;
    g_buffer.normal = float4(normal, pass_get_material_index());
    g_buffer.material = float4(0.0f, roughness, metalness, occlusion);
    g_buffer.velocity = velocity;
    g_buffer.pick = vertex.pick_id;
    g_buffer.emissive = float4(emissive, 1.0f);
    g_buffer.weather = weather_encode(receptivity);
    return g_buffer;
}

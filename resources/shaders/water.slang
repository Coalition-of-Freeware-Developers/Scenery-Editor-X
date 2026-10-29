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
 * water.slang
 * -------------------------------------------------------
 * Created: 22/10/2026
 * -------------------------------------------------------
 *
 * The water surface (plan TASK-073; XP12: ocean_shading, "mix down water"), drawn by Renderer::Pass_Water after the sky:
 * the water meshes (DSF water patches, cut into 64 m cells on the CPU) tessellated by their size on screen and displaced
 * by the FFT cascades (ocean_common.slang), then lit, opaque, with its depth written:
 *   - the surface: the cascades' slopes as the normal, their unresolved slope variance added to water/gloss's roughness
 *     (the sun's glint widens with distance instead of aliasing), F0 water/F0; the sky and scenery it mirrors from the
 *     screen-space reflections over the probes (ssr_common.slang, XP12: tex_ssr in ocean_shading), the sun and moon
 *     through their shadows;
 *   - under it, through (1 - Fresnel): the water body's colour (the tile's water map, rgb) lit from above, and through the
 *     water what is behind it: the frame's opaque surfaces (tex_water_opaque) where something stands in the water, else
 *     the sea floor at the DSF's bathymetry depth (water/floor decal over sedx/water_floor_albedo), faded by the
 *     transmittance of the path (clarity from the map's alpha and water/turbidity, renderer/ocean.cpp);
 *   - foam where the waves break, then aerial perspective.
 * Whatever lies on the flat water or stands in front of it (drapes, shoreline edges, beaches, piers) wins: water is only
 * drawn where the opaque surface is behind its flat plane (plan TASK-074).
 * Material: color slot 0 the tile's water map (u = x scale + offset, v = z scale + offset: terrain_base), slot 1 the floor
 * decal (drape_decal: repeats per meter, strength); color the fallback water colour (linear) and clarity (alpha).
 * Vertex uv: DSF fetch ratio, floor depth (m). pass_get_f4_value(): x 1 when the environment's sun and moon light the scene.
 */
import resources;
import constants;
import common;
import color;
import position;
import vertex_processing;
import csm_sample;
import environment_lighting;
import ssr_common;
import ocean_common;

[[vk::binding(73)]] Texture2D tex_water_opaque : register(t73);	// The frame as composed before the water (pre-exposed)

static const uint SSR_NORMAL_CASCADES = 2;		// The screen-space mirror's normal: the long waves' cascades
static const float SSR_NORMAL_BLUR = 4.0f;		// ... averaged over this many pixels

struct WaterControlPoint
{
    float3 ground : GROUND;							// World position on the flat water
    float2 water : WATER;							// DSF fetch ratio, floor depth (m)
    nointerpolation uint material_index : MATERIAL;
};

struct WaterPatch
{
    float edges[3] : SV_TessFactor;
    float inside : SV_InsideTessFactor;
};

struct WaterPixel
{
    precise float4 position : SV_POSITION;
    float3 world : WORLD;							// Displaced
    float3 ground : GROUND;
    float2 water : WATER;
    nointerpolation uint material_index : MATERIAL;
};

/* @brief renderer/ocean.cpp WaterWaveScale: the fetch ratio (ponds calmer; pond scale 0: off) and shoaling toward the shore. */
float water_wave_scale(float2 water)
{
    const float pond = buffer_environment.water_tessellation.z;
    const float fetch = pond > 0.0f ? lerp(pond, 1.0f, saturate(water.x)) : 1.0f;
    const float shore_depth = buffer_environment.water_volume.w;
    const float shore = shore_depth > 0.0f ? smoothstep(0.0f, 1.0f, saturate(water.y / shore_depth)) : 1.0f;
    return fetch * shore;
}

/* @brief Width (m) of one pixel at a world point: grows with distance for a perspective camera, fixed for an orthographic one. */
float pixel_size_at(float3 world)
{
    const float w = max(abs(mul(float4(world, 1.0f), get_view_projection_matrix()).w), get_camera_near_plane());
    return 2.0f * w / (max(abs(get_projection_matrix()[1][1]), 1e-6f) * get_resolution_render().y);
}

// -------------------------------------------------------
// Geometry

[shader("vertex")]
WaterControlPoint main_vs(Vertex_PosUvNorTan input)
{
    _draw = draw_data[buffer_pass.draw_index];
    WaterControlPoint point;
    point.ground = mul(float4(input.position.xyz, 1.0f), _draw.transform).xyz;
    point.water = input.uv;
    point.material_index = _draw.material_index;
    return point;
}

/* @brief renderer/ocean.cpp WaterTessellationFactor: from the edge's own end points, so both triangles sharing it agree. */
float edge_factor(float3 a, float3 b)
{
    const float target = pixel_size_at((a + b) * 0.5f) * max(buffer_environment.water_tessellation.x, 1.0f);
    return clamp(length(b - a) / max(target, 1e-4f), 1.0f, max(buffer_environment.water_tessellation.y, 1.0f));
}

WaterPatch water_patch(InputPatch<WaterControlPoint, 3> patch)
{
    // Edge i lies opposite control point i
    WaterPatch result;
    result.edges[0] = edge_factor(patch[1].ground, patch[2].ground);
    result.edges[1] = edge_factor(patch[2].ground, patch[0].ground);
    result.edges[2] = edge_factor(patch[0].ground, patch[1].ground);
    result.inside = max(max(result.edges[0], result.edges[1]), result.edges[2]);
    return result;
}

[shader("hull")]
[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(3)]
[patchconstantfunc("water_patch")]
[maxtessfactor(64.0f)]
WaterControlPoint main_tesc(InputPatch<WaterControlPoint, 3> patch, uint id : SV_OutputControlPointID)
{
    return patch[id];
}

[shader("domain")]
[domain("tri")]
[partitioning("fractional_odd")]	// Repeated from the hull shader: SPIR-V's evaluation stage takes its own spacing / winding
[outputtopology("triangle_ccw")]
WaterPixel main_tese(WaterPatch constants, float3 location : SV_DomainLocation, const OutputPatch<WaterControlPoint, 3> patch)
{
    WaterPixel pixel;
    precise float3 ground = patch[0].ground * location.x + patch[1].ground * location.y + patch[2].ground * location.z;
    pixel.ground = ground;
    pixel.water = patch[0].water * location.x + patch[1].water * location.y + patch[2].water * location.z;
    pixel.material_index = patch[0].material_index;

    // The cascades' displacement, filtered for the size of a tessellated edge here
    const float footprint = pixel_size_at(ground) * max(buffer_environment.water_tessellation.x, 1.0f);
    const float3 displacement = is_feature_enabled(FEATURE_WATER) ? ocean_displacement(ground.xz, footprint).xyz * water_wave_scale(pixel.water) : 0.0f;
    pixel.world = ground + displacement;
    pixel.position = mul(float4(pixel.world, 1.0f), get_view_projection_matrix());
    return pixel;
}

// -------------------------------------------------------
// Shading

static float4 sample_material_texture(uint texture_index, float2 uv, uint sampler_index)
{
    return material_textures[pass_get_material_index() + texture_index].Sample(samplers[sampler_index], uv);
}

/* @brief The water body's colour (linear) and clarity at a ground point: the tile's water map, or the material's colour. */
float4 water_tile_color(MaterialParameters material, float2 xz)
{
    float4 color = material.color;
    if (material.has_texture_albedo())
    {
        const float2 uv = float2(xz.x * material.terrain_base.x + material.terrain_base.y, xz.y * material.terrain_base.z + material.terrain_base.w);
        const float4 texel = sample_material_texture(material_texture_index_albedo, uv, sampler_bilinear_clamp);
        color = float4(srgb_to_linear(texel.rgb), texel.a);
    }
    return color;
}

/* @brief renderer/ocean.cpp WaterClarityDistance and WaterTransmittance. */
float water_clarity_distance(float alpha)
{
    return max(buffer_environment.water_volume.x / max(1.0f - saturate(alpha), 0.01f), buffer_environment.water_volume.z);
}

float water_transmittance(float path_m, float clarity_m)
{
    if (buffer_environment.water_surface.w < 0.5f)
        return 1.0f;
    return exp(buffer_environment.water_volume.y * max(path_m, 0.0f) / max(clarity_m, 1e-3f));
}

static float fresnel(float f0, float cos_theta)
{
    return f0 + (1.0f - f0) * pow(1.0f - saturate(cos_theta), 5.0f);
}

/* @brief Cosine of a ray's angle to the normal once refracted into the water. */
static float refracted_cosine(float cos_theta, float ior)
{
    return sqrt(saturate(1.0f - (1.0f - cos_theta * cos_theta) / (ior * ior)));
}

[shader("fragment")]
float4 main_frag(WaterPixel pixel) : SV_TARGET
{
    pass_load_draw_data_from_vertex(pixel.material_index);
    const MaterialParameters material = GetMaterial();
    const float2 screen_uv = pixel.position.xy / (get_resolution_render() * get_resolution_scale());
    const int2 texel = int2(pixel.position.xy);
    const float3 eye = get_camera_position();
    const float3 ray = normalize(pixel.world - eye);
    const float distance_surface = length(pixel.world - eye);

    // Where this pixel's ray meets the flat water (from the pixel's position alone, so its screen derivatives are smooth
    // across the tessellated cells: the waves' filter footprint), and the opaque surface's distance (the depth pyramid's
    // level 0, before the water: 0 where nothing is drawn). What lies on the flat water or in front of it shows instead.
    const float3 ray_near = get_position(1.0f, screen_uv);
    const float3 ray_step = get_position(0.5f, screen_uv) - ray_near;
    const float3 flat_point = ray_near + ray_step * (abs(ray_step.y) > 1e-9f ? (pixel.ground.y - ray_near.y) / ray_step.y : 0.0f);
    const float2 footprint_x = ddx(flat_point.xz);
    const float2 footprint_y = ddy(flat_point.xz);
    const float distance_flat = abs(ray.y) > 1e-5f ? (pixel.ground.y - eye.y) / ray.y : distance_surface;
    uint2 pyramid_size;
    uint pyramid_levels;
    tex_hiz.GetDimensions(0, pyramid_size.x, pyramid_size.y, pyramid_levels);
    const bool has_pyramid = all(uint2(texel) < pyramid_size);	// A placeholder is bound when no pyramid was built this frame
    const float opaque_depth = has_pyramid ? tex_hiz.Load(int3(texel, 0)).g : 0.0f;
    const float distance_opaque = opaque_depth > 0.0f ? length(get_position(opaque_depth, screen_uv) - eye) : 1e30f;
    if (distance_flat > 0.0f && distance_opaque < distance_flat)
        discard;

    // The surface: the waves' slopes as the normal, their unresolved variance as roughness
    const bool waves = is_feature_enabled(FEATURE_WATER);
    const float wave_scale = water_wave_scale(pixel.water);
    float4 slopes = waves ? ocean_slopes(pixel.ground.xz, footprint_x, footprint_y) : 0.0f;
    slopes.xy *= wave_scale;
    slopes.zw *= wave_scale * wave_scale;
    const float3 normal = normalize(float3(-slopes.x, 1.0f, -slopes.y));
    const float base_roughness = buffer_environment.water_surface.y;
    const float alpha2 = base_roughness * base_roughness * base_roughness * base_roughness + slopes.z + slopes.w;
    const float roughness = clamp(sqrt(sqrt(alpha2)), 0.02f, 1.0f);

    const float3 v = -ray;
    float n_dot_v = dot(normal, v);
    if (n_dot_v < 1e-3f)
        n_dot_v = 1e-3f;	// Wave faces turned away at grazing angles: as seen edge on
    const float f0 = buffer_environment.water_surface.x;
    const float pre_exposure = get_pre_exposure();
    const bool sun_and_moon = pass_get_f4_value().x > 0.5f;

    // Reflection (XP12: ocean_shading's tex_ssr and u_skydome): the sun's and moon's glint through their shadows, the
    // mirrored scenery over the probes' sky. A wave face turned away from the camera mirrors the water beyond it, not the
    // ground under the horizon: its mirror direction stays at the horizon or above.
    XpLitSurface surface;
    surface.albedo = 0.0f;
    surface.normal = normal;
    surface.geometric_normal = float3(0.0f, 1.0f, 0.0f);
    surface.view = v;
    surface.roughness = roughness;
    surface.v = 0.0f;
    surface.occlusion = 1.0f;
    surface.env_brdf = tex3.SampleLevel(samplers[sampler_bilinear_clamp], float2(n_dot_v, roughness), 0.0f).rg;
    float3 unused = 0.0f;
    float3 specular = 0.0f;
    float3 sun_light = 0.0f;	// Light reaching the water from above (lux)
    float sun_cosine = 1.0f;
    if (sun_and_moon)
    {
        const float2 shadows = xp_light_shadows(pixel.world, float3(0.0f, 1.0f, 0.0f));
        const float3 sun = xp_sun_illuminance_at(pixel.world) * shadows.x;
        const float3 moon = xp_moon_illuminance_at(pixel.world) * shadows.y;
        xp_directional_light(surface, f0.xxx, get_sun_direction(), sun, unused, specular);
        xp_directional_light(surface, f0.xxx, get_moon_direction(), moon, unused, specular);
        sun_light = sun * saturate(get_sun_direction().y) + moon * saturate(get_moon_direction().y);
        sun_cosine = max(get_sun_direction().y, get_moon_direction().y);
    }
    const bool probes = is_feature_enabled(FEATURE_ENV_PROBES) && is_feature_enabled(FEATURE_ATMOSPHERE);
    float3 mirror = reflect(-v, normal);
    mirror.y = max(mirror.y, 0.01f);
    mirror = normalize(mirror);
    // The screen-space mirror follows the long waves, smoothed over a few pixels: traced along every ripple, its rays hit
    // and miss the shore pixel by pixel (speckles); the ripples' share is in the roughness and the probes' sky
    float4 ssr = 0.0f;
    if (xp_ssr_active())
    {
        float2 long_slopes = waves ? ocean_slopes(pixel.ground.xz, footprint_x * SSR_NORMAL_BLUR, footprint_y * SSR_NORMAL_BLUR, SSR_NORMAL_CASCADES).xy : 0.0f;
        long_slopes *= wave_scale;
        const float3 ssr_normal = normalize(float3(-long_slopes.x, 1.0f, -long_slopes.y));
        ssr = xp_ssr_trace(pixel.world, ssr_normal, v, roughness);
        ssr.rgb /= max(pre_exposure, 1e-20f);
    }
    const float3 sky = probes ? xp_probe_specular(mirror, roughness) : xp_ambient_reflection(mirror, roughness);
    specular += (f0 * surface.env_brdf.x + surface.env_brdf.y) * lerp(sky, ssr.rgb, saturate(ssr.a));
    const float3 sky_light = probes ? xp_probe_diffuse(float3(0.0f, 1.0f, 0.0f)) * PI : xp_ambient_irradiance(float3(0.0f, 1.0f, 0.0f));

    // Under the surface: the water body, and through it the floor or what stands in the water
    const float ior = max(buffer_environment.water_surface.z, 1.0f);
    const float4 body = water_tile_color(material, pixel.ground.xz);
    const float clarity = water_clarity_distance(body.a);
    const float3 body_radiance = body.rgb * (sun_light + sky_light) * INV_PI;
    float3 behind;
    float path;
    uint2 opaque_size;
    tex_water_opaque.GetDimensions(opaque_size.x, opaque_size.y);
    if (opaque_depth > 0.0f && all(uint2(texel) < opaque_size))
    {
        behind = tex_water_opaque.Load(int3(texel, 0)).rgb / max(pre_exposure, 1e-20f);
        path = max(distance_opaque - distance_surface, 0.0f);
    }
    else if (buffer_environment.water_floor.w > 0.0f)
    {
        // The sea floor at the bathymetry's depth, as much of it as water/sea_floor_coef shows
        const float floor_depth = max(pixel.water.y, 0.0f);
        float3 albedo = buffer_environment.water_floor.rgb;
        if (material.drape_decal.x > 0.0f)
        {
            const float decal = sample_material_texture(material_texture_index_albedo + 1, pixel.ground.xz * material.drape_decal.x, sampler_anisotropic_wrap).r;
            albedo *= lerp(1.0f, 2.0f * decal, saturate(material.drape_decal.y));
        }
        const float sun_path = floor_depth / max(refracted_cosine(saturate(sun_cosine), ior), 0.2f);
        const float3 floor_light = sun_light * water_transmittance(sun_path, clarity) + sky_light * water_transmittance(floor_depth * 1.2f, clarity);
        behind = lerp(body_radiance, albedo * floor_light * INV_PI, saturate(buffer_environment.water_floor.w));
        path = floor_depth / max(refracted_cosine(n_dot_v, ior), 0.2f);
    }
    else
    {
        behind = body_radiance;	// No floor: the water body all the way down
        path = 0.0f;
    }
    const float transmittance = water_transmittance(path, clarity);
    const float3 under = lerp(body_radiance, behind, transmittance);

    float3 color = specular + under * (1.0f - fresnel(f0, n_dot_v));

    // Foam where the waves break: white, lit from above; calm water (ponds, shallows) doesn't break
    if (waves)
    {
        const float foam = saturate(ocean_displacement(pixel.ground.xz, pixel_size_at(pixel.world)).w * smoothstep(0.25f, 1.0f, wave_scale));
        color = lerp(color, 0.85f * (sun_light + sky_light) * INV_PI, foam);
    }

    color *= pre_exposure;
    color = xp_apply_aerial_perspective(color, screen_uv, distance_surface, 1.0f);
    return validate_output(float4(color, 1.0f));
}

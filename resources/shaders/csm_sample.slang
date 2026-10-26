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
 * csm_sample.slang
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 *
 * Sun shadows at a surface (plan TASK-054; XP12: the CSM lookup of the deferred lighting) and the cloud shadow over it
 * (TASK-055). The cascades come from renderer/csm.h through buffer_environment.csm_*; Pass_ShadowCascades draws
 * shadow_csm (reverse-Z: 1 nearest the light). Both shadows dim the one light that casts them, the sun by day, the moon
 * when the sun is down (csm_params.w).
 *
 * Every shader that imports this declares both textures (Slang keeps unused globals): passes bind them with
 * Renderer::BindShadows, or leave them unbound when the code that reads them can't run.
 */
module csm_sample;

import resources;
import position;

public [[vk::binding(62)]] Texture2DArray tex_shadow_csm : register(t62);	// one layer per cascade, D32F reverse-Z
public [[vk::binding(63)]] Texture2DArray tex_cloud_shadow : register(t63);	// sunlight through the clouds, one layer per cascade

// RendererFeatures bits (renderer_features.cpp table order)
public static const uint FEATURE_CLOUD_SHADOWS = 3;
public static const uint FEATURE_CSM = 4;

public static const uint SHADOW_LIGHT_NONE = 0;
public static const uint SHADOW_LIGHT_SUN = 1;
public static const uint SHADOW_LIGHT_MOON = 2;

/* @brief The light the shadow maps are made for: SHADOW_LIGHT_NONE, _SUN or _MOON. */
public uint xp_shadow_light() { return uint(buffer_environment.csm_params.w + 0.5f); }

/* @brief Unit vector toward the light the shadow maps are made for. */
public float3 xp_shadow_light_direction()
{
    return xp_shadow_light() == SHADOW_LIGHT_MOON ? buffer_environment.moon_direction.xyz : buffer_environment.sun_direction.xyz;
}

/* @brief Cascades in use this frame (0: no sun shadows). */
public uint xp_csm_cascade_count()
{
    return is_feature_enabled(FEATURE_CSM) ? min(uint(buffer_environment.csm_params.x + 0.5f), 4u) : 0u;
}

/* @brief A world point's distance along the camera's view axis: what the cascades split. */
public float xp_view_depth(float3 world) { return dot(world - get_camera_position(), get_camera_forward()); }

/* @brief The cascade that serves a view depth, or the cascade count past the last one. */
public uint xp_csm_select(float view_depth)
{
    const uint count = xp_csm_cascade_count();
    for (uint i = 0; i < count; ++i)
    {
        if (view_depth < buffer_environment.csm_split[i])
            return i;
    }
    return count;
}

/**
 * @brief The receiver's depth change per shadow-map texel (u, v) in a cascade, if it lies in the plane through it with
 * this normal: the light's frame and scales come back out of the cascade's matrix (rows right / r, up / r,
 * -forward / depth range). Grazing planes are held to the slope of 84 degrees.
 */
float2 receiver_plane_slope(uint cascade, float3 normal, float size)
{
    const float4x4 rows = transpose(buffer_environment.csm_view_projection[cascade]);
    const float radius = 1.0f / max(length(rows[0].xyz), 1e-20f);
    const float range = 1.0f / max(length(rows[2].xyz), 1e-20f);
    const float3 right = rows[0].xyz * radius;
    const float3 up = rows[1].xyz * radius;
    const float3 forward = -rows[2].xyz * range;

    // along the plane, depth (away from the light) changes by -(n.right / n.forward) per meter across the light
    float n_forward = dot(normal, forward);
    n_forward = n_forward < 0.0f ? min(n_forward, -0.1f) : max(n_forward, 0.1f);
    const float scale = 2.0f * radius / (range * size);	// clip depth per meter of depth x meters per texel
    return float2(dot(normal, right), -dot(normal, up)) / n_forward * scale;
}

/**
 * @brief Visibility (0 shadowed .. 1 lit) of a world point in one cascade. The receiver moves along its normal by
 * csm_filter.x texels plus csm_filter.y texels per unit of tan(angle to the light) (shadow/bias_n_constant,
 * bias_n_slope). Soft (shadow/want_soft): a 3 x 3 box of bilinear comparisons, i.e. a tent over 4 x 4 texels, read with
 * four 2 x 2 gathers; with the receiver-plane bias (csm_filter.z 2) each gather compares against the receiver's plane
 * at its own place instead of the receiver's depth, so the casters' slope bias only has to cover half a texel each way
 * and thin gaps (a panel a few centimeters above the ground) keep their shadow.
 */
float csm_sample_cascade(uint cascade, float3 world, float3 normal)
{
    const float4 filter = buffer_environment.csm_filter;
    const float texel = buffer_environment.csm_texel[cascade];
    const float n_dot_l = saturate(dot(normal, xp_shadow_light_direction()));
    const float tangent = sqrt(max(1.0f - n_dot_l * n_dot_l, 0.0f)) / max(n_dot_l, 0.1f);
    world += normal * texel * (filter.x + filter.y * tangent);

    // reverse-Z orthographic: w is 1; clip +y is the top of the map (v = 0)
    const float4 clip = mul(float4(world, 1.0f), buffer_environment.csm_view_projection[cascade]);
    const float2 uv = float2(clip.x * 0.5f + 0.5f, 0.5f - clip.y * 0.5f);
    const float depth = clip.z;
    if (any(uv <= 0.0f) || any(uv >= 1.0f) || depth <= 0.0f)
        return 1.0f;

    // lit where the receiver is at least as near the light as what the map holds (the sampler compares GREATER)
    const SamplerComparisonState compare = samplers_comparison[sampler_compare_depth];
    if (filter.z < 0.5f)
        return tex_shadow_csm.SampleCmpLevelZero(compare, float3(uv, float(cascade)), depth);

    // the 4 x 4 texels around the point (base - 1 .. base + 2) and their tent weights (they sum to 3 per axis)
    const float size = max(filter.w, 1.0f);
    const float2 position = uv * size - 0.5f;
    const float2 base = floor(position);
    const float2 f = position - base;
    const float2 slope = filter.z > 1.5f ? receiver_plane_slope(cascade, normal, size) : float2(0.0f, 0.0f);
    const float weight_x[4] = { 1.0f - f.x, 1.0f, 1.0f, f.x };
    const float weight_y[4] = { 1.0f - f.y, 1.0f, 1.0f, f.y };

    float lit = 0.0f;
    [unroll]
    for (uint j = 0; j < 2; ++j)
    {
        [unroll]
        for (uint i = 0; i < 2; ++i)
        {
            // a gather on the corner between texels base + 2i - 1 and base + 2i (and the same in y) returns those four:
            // x (left, bottom), y (right, bottom), z (right, top), w (left, top), bottom = the larger v
            const float2 corner = base + float2(2.0f * i, 2.0f * j);
            const float block_depth = depth + dot(slope, corner - (position + 0.5f));	// the plane at the block's center
            const float4 g = tex_shadow_csm.GatherCmp(compare, float3(corner / size, float(cascade)), block_depth);
            const float left = weight_x[2 * i], right = weight_x[2 * i + 1];
            const float top = weight_y[2 * j], bottom = weight_y[2 * j + 1];
            lit += g.w * left * top + g.z * right * top + g.x * left * bottom + g.y * right * bottom;
        }
    }
    return lit / 9.0f;
}

/**
 * @brief Sun-shadow visibility (0 shadowed .. 1 lit) at a world point with a surface normal: the cascade for its view
 * depth, blended into the next one over the band before the cascade ends (shadow/overlap_ratio), faded to lit from the
 * fade start to the far limit (shadow/total_fade_ratio, csm/far_limit_exterior). 1 without cascades.
 */
public float xp_csm_visibility(float3 world, float3 normal)
{
    const uint count = xp_csm_cascade_count();
    if (count == 0)
        return 1.0f;

    const float depth = xp_view_depth(world);
    const float fade_start = buffer_environment.csm_params.y;
    const float fade_end = buffer_environment.csm_params.z;
    if (depth < 0.0f || depth >= fade_end)
        return 1.0f;
    const uint cascade = xp_csm_select(depth);
    if (cascade >= count)
        return 1.0f;

    float visibility = csm_sample_cascade(cascade, world, normal);
    const float band = buffer_environment.csm_blend[cascade];
    const float band_start = buffer_environment.csm_split[cascade] - band;
    if (cascade + 1 < count && band > 0.0f && depth > band_start)
        visibility = lerp(visibility, csm_sample_cascade(cascade + 1, world, normal), saturate((depth - band_start) / band));

    return lerp(visibility, 1.0f, saturate((depth - fade_start) / max(fade_end - fade_start, 1e-3f)));
}

/**
 * @brief The geometric normal at a pixel of the g-buffer, from the world positions of its neighbours in the depth
 * buffer (on each axis the one nearer to it, so edges don't bend it), facing the camera: the plane the receiver-plane
 * bias needs (normal maps tilt the shading normal off it). The shading normal where nothing is next to it.
 */
public float3 xp_receiver_normal(int2 pixel, float3 world, float3 shading_normal)
{
    float2 resolution;
    tex_depth.GetDimensions(resolution.x, resolution.y);
    const int2 last = int2(resolution) - 1;
    float3 offsets[4];
    float distances[4];
    const int2 steps[4] = { int2(-1, 0), int2(1, 0), int2(0, -1), int2(0, 1) };
    [unroll]
    for (uint k = 0; k < 4; ++k)
    {
        const int2 p = clamp(pixel + steps[k], int2(0, 0), last);
        const float depth = tex_depth[p].r;
        offsets[k] = float3(0.0f, 0.0f, 0.0f);
        distances[k] = 1e30f;
        if (depth > 0.0f && any(p != pixel))
        {
            offsets[k] = get_position(depth, (float2(p) + 0.5f) / resolution) - world;
            distances[k] = dot(offsets[k], offsets[k]);
        }
    }
    if (min(distances[0], distances[1]) > 1e29f || min(distances[2], distances[3]) > 1e29f)
        return shading_normal;
    const float3 dx = distances[1] <= distances[0] ? offsets[1] : -offsets[0];
    const float3 dy = distances[3] <= distances[2] ? offsets[3] : -offsets[2];

    float3 n = cross(dy, dx);
    const float length_n = length(n);
    if (length_n < 1e-12f)
        return shading_normal;
    n /= length_n;
    return dot(n, get_camera_position() - world) < 0.0f ? -n : n;
}

/**
 * @brief Share of the shadow light that comes through the clouds at a world point (1: clear). The map holds it for
 * points on the ground (y = 0) under each texel; a point above the ground is moved down its light ray onto the ground
 * first. Cascade 0 is the finer one; past it cascade 1.
 */
public float xp_cloud_shadow(float3 world)
{
    const uint count = is_feature_enabled(FEATURE_CLOUD_SHADOWS) ? min(uint(buffer_environment.cloud_shadow.x + 0.5f), 2u) : 0u;
    if (count == 0)
        return 1.0f;

    const float3 l = xp_shadow_light_direction();
    const float2 ground = world.xz - l.xz * (world.y / max(l.y, 0.05f));
    for (uint i = 0; i < count; ++i)
    {
        const float4 cascade = buffer_environment.cloud_shadow_cascades[i];
        const float2 uv = (ground - cascade.xy) / (2.0f * cascade.z) + 0.5f;
        if (all(uv > 0.005f) && all(uv < 0.995f))
        {
            const float transmittance = tex_cloud_shadow.SampleLevel(samplers[sampler_bilinear_clamp], float3(uv, float(i)), 0.0f).r;
            return lerp(1.0f, saturate(transmittance), saturate(buffer_environment.cloud_shadow.y));
        }
    }
    return 1.0f;
}

/**
 * @brief How much of the sun (x) and the moon (y) reaches a surface with this plane normal (geometric, see
 * xp_receiver_normal): the cascades and the clouds dim the shadow light, the other one is never shadowed (it is below
 * the horizon, or too faint to cast).
 */
public float2 xp_light_shadows(float3 world, float3 normal)
{
    const uint light = xp_shadow_light();
    if (light == SHADOW_LIGHT_NONE)
        return float2(1.0f, 1.0f);
    const float shadow = xp_csm_visibility(world, normal) * xp_cloud_shadow(world);
    return light == SHADOW_LIGHT_SUN ? float2(shadow, 1.0f) : float2(1.0f, shadow);
}

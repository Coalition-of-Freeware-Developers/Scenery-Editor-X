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
 * ssr_common.slang
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 *
 * Screen-space reflections (plan Phase 9; XP12: ssr_deferred for the g-buffer, ssr_mesh for glass): a surface's mirror
 * ray marched through the min / max depth pyramid (z_minmax.slang) and, where it ends on a surface, that surface as the
 * previous frame lit it (the reflection history, reprojected), blurred for the surface's gloss by reading a coarser mip.
 * Shared by ssr_trace.slang (the deferred surfaces, into the reflections target the lighting reads) and
 * forward_transparent.slang (blended surfaces, glass and drapes, which aren't in the g-buffer). renderer/ssr.cpp holds the
 * CPU twin of the march the tests check; keep the two alike.
 *
 * The hierarchical-Z march is adapted from AMD FidelityFX SSSR 1.5 (FidelityFX SDK 1.1.4, ffx_sssr_intersect.h):
 *   Copyright (C) 2024 Advanced Micro Devices, Inc.
 *   Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 *   documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 *   rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 *   persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice
 *   and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS
 *   PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND (the full text: docs/third_party/LICENSE-FidelityFX-MIT.txt).
 * SEDX differences: the pyramid holds the farthest and the nearest depth (the march reads the nearest); the march stops
 * when the ray leaves the screen or the depth range; one ray along the mirror direction per pixel, its gloss lobe taken
 * from blurred mips of the previous frame (X-Plane's ssr/sample_count 1, ssr/use_prefiltered 1) instead of SSSR's
 * stochastic rays and denoiser; rays start a pixel off their surface (renderer/ssr.h SSR_ORIGIN_OFFSET_PIXELS).
 *
 * Every shader that imports this declares the three textures (Slang keeps unused globals): passes bind them with
 * Renderer::BindReflections.
 */
module ssr_common;

import resources;

public [[vk::binding(68)]] Texture2D tex_ssr : register(t68);				// The deferred surfaces' reflections: rgb pre-exposed, a weight
public [[vk::binding(69)]] Texture2D tex_hiz : register(t69);				// Min / max depth pyramid: r farthest, g nearest (reverse-Z)
public [[vk::binding(70)]] Texture2D tex_ssr_history : register(t70);		// The previous frame lit, with mips: rgb pre-exposed, a its pre-exposure

// RendererFeatures bit (renderer_features.cpp table order)
public static const uint FEATURE_SSR = 10;

static const float XP_GLOSS_SCALE = 0.96875f;		// renderer/ssr.h
static const float SSR_ORIGIN_OFFSET_PIXELS = 1.0f;	// renderer/ssr.h
static const float SSR_FLOAT_MAX = 3.402823466e+38f;
static const float3 SSR_LUMA = float3(0.2126f, 0.7152f, 0.0722f);

/* @brief SSR runs this frame: the feature is on and the pyramid and history passes are there (Renderer::BindReflections). */
public bool xp_ssr_active() { return buffer_environment.ssr_history.w > 0.5f; }

/* @brief Debug (--ssr-trace-all): the deferred pass traces every surface as a mirror. */
public bool xp_ssr_trace_all() { return buffer_environment.ssr_history.z > 0.5f; }

/* @brief X-Plane's gloss for a perceptual roughness (roughness = 1 - 0.96875 gloss). */
public float xp_gloss_from_roughness(float roughness) { return saturate((1.0f - roughness) / XP_GLOSS_SCALE); }

/**
 * @brief The share of a surface's specular reflection SSR may replace (renderer/ssr.cpp ReflectionGlossWeight): 0 below
 * ssr/min_gloss, fading in over sedx/ssr_gloss_fade above it.
 */
public float xp_ssr_gloss_weight(float roughness)
{
    const float gloss = xp_gloss_from_roughness(roughness);
    const float min_gloss = buffer_environment.ssr.x;
    if (gloss < min_gloss)
        return 0.0f;
    const float fade = buffer_environment.ssr.y;
    return fade > 0.0f ? smoothstep(0.0f, 1.0f, saturate((gloss - min_gloss) / fade)) : 1.0f;
}

// Screen space: (u, v, reverse-Z depth), u / v 0-1 with v down; view space through the frame's projection (perspective or
// orthographic), the camera looking down -z
float3 view_to_screen(float3 view)
{
    const float4 clip = mul(float4(view, 1.0f), get_projection_matrix());
    const float3 ndc = clip.xyz / clip.w;
    return float3(ndc.xy * float2(0.5f, -0.5f) + 0.5f, ndc.z);
}

float3 screen_to_view(float3 screen)
{
    const float4 view = mul(float4(screen.xy * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), screen.z, 1.0f), get_projection_inverted_matrix());
    return view.xyz / view.w;
}

// clip.w of a view-space point: its view depth with a perspective camera, 1 with an orthographic one
float clip_w(float3 view) { return mul(float4(view, 1.0f), get_projection_matrix()).w; }

/* @brief Unit vector from a world point toward the eye: toward the camera, or against its axis for an orthographic one. */
public float3 xp_ssr_view_direction(float3 world)
{
    const bool orthographic = abs(clip_w(float3(0.0f, 0.0f, -1.0f)) - clip_w(float3(0.0f, 0.0f, -2.0f))) < 1e-6f;
    return orthographic ? -normalize(get_camera_forward()) : normalize(get_camera_position() - world);
}

// Width (m) of one pixel at a view-space point
float pixel_size_at(float3 view, float screen_height)
{
    const float4x4 projection = get_projection_matrix();
    return 2.0f * clip_w(view) / (max(abs(projection[1][1]), 1e-6f) * screen_height);
}

float hiz_nearest(int2 cell, int level, int2 size)
{
    const int2 level_size = max(size >> level, int2(1, 1));
    return tex_hiz.Load(int3(clamp(cell, int2(0, 0), level_size - 1), level)).g;
}

/**
 * @brief The hierarchical-Z march (renderer/ssr.cpp MarchHiZ): a screen-space ray (origin, direction of any length) steps
 * to its cell's far edge (a free step: up a level) or to the cell's nearest surface (down a level) while it is in front
 * of that surface, and ends on a surface when it is blocked at level 0. False when it leaves the screen or the depth
 * range, or runs out of steps.
 */
public bool xp_ssr_march(float3 origin, float3 direction, uint max_steps, out float3 position)
{
    uint2 size;
    uint levels;
    tex_hiz.GetDimensions(0, size.x, size.y, levels);
    const float2 screen = float2(size);
    const int top = int(levels) - 1;
    const float3 inv = float3(direction.x != 0.0f ? 1.0f / direction.x : SSR_FLOAT_MAX,
                              direction.y != 0.0f ? 1.0f / direction.y : SSR_FLOAT_MAX,
                              direction.z != 0.0f ? 1.0f / direction.z : SSR_FLOAT_MAX);

    // Cell edges ahead of the ray, nudged past by 0.005 texel so the next lookup lands in the next cell
    const float2 floor_offset = float2(direction.x < 0.0f ? 0.0f : 1.0f, direction.y < 0.0f ? 0.0f : 1.0f);
    const float2 nudge = float2(direction.x < 0.0f ? -0.005f : 0.005f, direction.y < 0.0f ? -0.005f : 0.005f) / screen;

    int level = 0;
    float2 resolution = screen;

    // Out of the origin's own texel first
    const float2 t_start = ((floor(origin.xy * resolution) + floor_offset) / resolution + nudge - origin.xy) * inv.xy;
    float t = min(t_start.x, t_start.y);
    position = origin + t * direction;

    for (uint i = 0; i < max_steps && level >= 0; ++i)
    {
        if (any(position.xy < 0.0f) || any(position.xy > 1.0f) || position.z <= 0.0f || position.z >= 1.0f)
            return false;

        const float2 cell = floor(position.xy * resolution);
        const float surface = hiz_nearest(int2(cell), level, int2(size));
        const float2 t_xy = ((cell + floor_offset) / resolution + nudge - origin.xy) * inv.xy;
        const float t_z = direction.z < 0.0f ? (surface - origin.z) * inv.z : SSR_FLOAT_MAX;
        const float t_min = min(min(t_xy.x, t_xy.y), t_z);

        // Reverse-Z: in front of the surface while the ray's depth is the larger
        const bool in_front = surface < position.z;
        const bool free_step = asuint(t_min) != asuint(t_z) && in_front;
        if (in_front)
            t = t_min;
        position = origin + t * direction;

        if (free_step)
        {
            if (level < top)
            {
                ++level;
                resolution *= 0.5f;
            }
        }
        else
        {
            --level;
            resolution *= 2.0f;
        }
    }
    return level < 0;
}

/**
 * @brief The reflection a surface at world (unit normal, perceptual roughness) shows along the mirror of view (unit, from
 * the surface toward the camera): rgb the radiance pre-exposed for this frame, a its weight (0 nothing found .. 1 all of
 * the reflection), the gloss weight included. Blurred for the roughness by the history mip its lobe covers.
 */
public float4 xp_ssr_trace(float3 world, float3 normal, float3 view, float roughness)
{
    const float gloss_weight = xp_ssr_gloss_weight(roughness);
    if (gloss_weight <= 0.0f)
        return 0.0f;

    uint2 size;
    uint levels;
    tex_hiz.GetDimensions(0, size.x, size.y, levels);
    const float2 screen = float2(size);

    // The ray in view space: from a pixel's width off the surface (the depth buffer's staircase), along the mirror
    const float3 reflected = reflect(-view, normal);
    const float3 origin_view = mul(float4(world, 1.0f), get_view_matrix()).xyz;
    const float3 normal_view = mul(float4(normal, 0.0f), get_view_matrix()).xyz;
    const float3 direction_view = mul(float4(reflected, 0.0f), get_view_matrix()).xyz;
    const float3 start_view = origin_view + normal_view * (SSR_ORIGIN_OFFSET_PIXELS * pixel_size_at(origin_view, screen.y));

    // Its screen-space direction from a point half the surface's distance along it, kept in front of the near plane
    float step = 0.5f * max(length(origin_view), 1.0f);
    const float w_origin = clip_w(start_view);
    const float w_rate = clip_w(start_view + direction_view) - w_origin;
    if (w_rate < 0.0f)
        step = min(step, 0.9f * (w_origin - get_camera_near_plane()) / -w_rate);
    if (step <= 0.0f)
        return 0.0f;
    const float3 origin = view_to_screen(start_view);
    const float3 direction = view_to_screen(start_view + direction_view * step) - origin;

    float3 hit;
    if (!xp_ssr_march(origin, direction, uint(buffer_environment.ssr_trace.x), hit))
        return 0.0f;

    // Where it ended: on the screen, a few pixels away (not its own surface), a surface there (not the sky)
    if (any(hit.xy < 0.0f) || any(hit.xy > 1.0f) || all(abs(hit.xy - origin.xy) < 2.0f / screen))
        return 0.0f;
    const int2 texel = min(int2(hit.xy * screen), int2(size) - 1);
    const float surface_depth = tex_hiz.Load(int3(texel, 0)).g;
    if (surface_depth <= 0.0f)
        return 0.0f;

    // Facing the ray (a surface seen from behind isn't what the mirror shows), and the ray no deeper behind it than the
    // thickness the depth buffer is taken to have
    const float3 hit_normal = tex_normal.Load(int3(texel, 0)).xyz;
    if (dot(hit_normal, reflected) > 0.0f)
        return 0.0f;
    const float3 surface_view = screen_to_view(float3(hit.xy, surface_depth));
    const float3 ray_view = screen_to_view(hit);
    const float thickness = buffer_environment.ssr_trace.y + buffer_environment.ssr_trace.z * max(-surface_view.z, 0.0f);
    float confidence = 1.0f - smoothstep(0.0f, max(thickness, 1e-4f), length(surface_view - ray_view));
    confidence *= confidence;

    // The surface in the previous frame (the camera's motion; moving objects aren't followed)
    const float3 surface_world = mul(float4(surface_view, 1.0f), get_view_inverted_matrix()).xyz;
    const float4 clip_previous = mul(float4(surface_world, 1.0f), get_view_projection_previous_matrix());
    if (clip_previous.w <= 0.0f)
        return 0.0f;
    const float2 uv_previous = clip_previous.xy / clip_previous.w * float2(0.5f, -0.5f) + 0.5f;
    if (any(uv_previous < 0.0f) || any(uv_previous > 1.0f))
        return 0.0f;

    // Fade toward the screen's edges, where what the mirror sees runs out (on both frames)
    const float2 edge = buffer_environment.ssr_trace.w * float2(screen.y / screen.x, 1.0f);
    const float2 border = smoothstep(0.0f, edge, hit.xy) * (1.0f - smoothstep(1.0f - edge, 1.0f, hit.xy)) *
                          smoothstep(0.0f, edge, uv_previous) * (1.0f - smoothstep(1.0f - edge, 1.0f, uv_previous));
    confidence *= border.x * border.y;
    if (confidence <= 0.0f)
        return 0.0f;

    // The lobe's footprint at the hit in history pixels (renderer/ssr.cpp ReflectionConeMip)
    uint2 history_size;
    uint history_levels;
    tex_ssr_history.GetDimensions(0, history_size.x, history_size.y, history_levels);
    float mip = max(buffer_environment.ssr.z, 0.0f);
    if (buffer_environment.ssr.w > 0.5f)
    {
        const float alpha = roughness * roughness;
        const float footprint = 2.0f * buffer_environment.ssr_history.y * alpha * length(surface_world - world);
        const float pixels = footprint / max(pixel_size_at(surface_view, float(history_size.y)), 1e-6f);
        mip = max(log2(max(pixels, 1.0f)) + buffer_environment.ssr.z, 0.0f);
    }
    mip = min(mip, float(history_levels - 1));

    // The history keeps the pre-exposure it was lit with in alpha (0: none yet)
    const float4 history = tex_ssr_history.SampleLevel(samplers[sampler_trilinear_clamp], uv_previous, mip);
    if (!(history.a > 1e-12f && history.a < 1.0f) || any(isnan(history.rgb)) || any(isinf(history.rgb)))
        return 0.0f;
    const float3 radiance = max(history.rgb, 0.0f) * (get_pre_exposure() / history.a);
    return float4(radiance, saturate(confidence * gloss_weight));
}

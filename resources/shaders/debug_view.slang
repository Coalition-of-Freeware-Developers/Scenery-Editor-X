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
 * debug_view.slang
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 *
 * The RENDERER panel's DEBUG VIEW (plan TASK-026): one g-buffer channel or a measurement of the frame instead of the
 * image. Writes display-referred linear values into frame_output (tex_uav). Views whose data a later phase adds
 * (AO, SSR, tile lights) show black until then.
 *   tex           frame_render (radiance x pre-exposure) for the luminance views
 *   tex_ssao      the ambient occlusion (Renderer::BindAmbientOcclusion; white when none ran)
 *   the shadow maps (csm_sample.slang, Renderer::BindShadows) for the cascade and cloud-shadow views
 *   pass_get_f4_value(): x view (Renderer_DebugView)
 */

import common;
import resources;
import constants;
import position;
import csm_sample;

static const uint VIEW_ALBEDO = 1;
static const uint VIEW_ROUGHNESS = 2;
static const uint VIEW_METALNESS = 3;
static const uint VIEW_NORMALS = 4;
static const uint VIEW_EMISSIVE = 5;
static const uint VIEW_WEATHER = 6;
static const uint VIEW_AO = 7;
static const uint VIEW_CASCADES = 8;
static const uint VIEW_CLOUD_SHADOW = 9;
static const uint VIEW_EV = 12;
static const uint VIEW_NITS = 13;
static const uint VIEW_DEPTH = 14;

float3 to_linear(float3 display) { return pow(saturate(display), 2.2); }

// log10 of a luminance from 0.01 to 100000 nits as a grey ramp
float3 nits_ramp(float nits) { return to_linear(((log10(max(nits, 1e-4)) + 2.0) / 7.0).xxx); }

// Colour bands for a value 0-1 (blue, cyan, green, yellow, red, magenta)
float3 false_color(float t)
{
    const float3 bands[6] = { float3(0, 0, 1), float3(0, 1, 1), float3(0, 1, 0), float3(1, 1, 0), float3(1, 0, 0), float3(1, 0, 1) };
    t = saturate(t) * 5.0;
    const int i = min(int(t), 4);
    return lerp(bands[i], bands[i + 1], frac(t));
}

[shader("compute")]
[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void main_comp(uint3 thread_id: SV_DispatchThreadID)
{
    float2 resolution;
    tex_uav.GetDimensions(resolution.x, resolution.y);
    if (any(thread_id.xy >= uint2(resolution)))
        return;

    const uint view = uint(pass_get_f4_value().x + 0.5);
    const int3 pixel = int3(thread_id.xy, 0);
    float3 color = float3(0.0, 0.0, 0.0);

    if (view == VIEW_ALBEDO)
        color = tex_albedo.Load(pixel).rgb;
    else if (view == VIEW_ROUGHNESS)
        color = tex_material.Load(pixel).ggg;	// Perceptual roughness
    else if (view == VIEW_METALNESS)
        color = tex_material.Load(pixel).bbb;	// Metalness / F0 value
    else if (view == VIEW_NORMALS)
        color = to_linear(normalize(tex_normal.Load(pixel).xyz) * 0.5 + 0.5);
    else if (view == VIEW_EMISSIVE)
    {
        const float3 emissive = tex_emissive.Load(pixel).rgb / max(get_pre_exposure(), 1e-20);
        const float nits = dot(emissive, float3(0.2126, 0.7152, 0.0722));
        color = nits > 0.0 ? nits_ramp(nits) : float3(0.0, 0.0, 0.0);
    }
    else if (view == VIEW_WEATHER)
        color = tex_weather.Load(pixel).rgb;	// R rain / damp, G puddles, B snow (0.5 + 0.5 s)
    else if (view == VIEW_AO)
    {
        // The visibility the ambient light gets as a grey level (128: half of it), nothing where the sky is
        const float2 uv = (float2(thread_id.xy) + 0.5) / resolution;
        if (tex_depth.Load(pixel).r > 0.0)
            color = to_linear(tex_ssao.SampleLevel(samplers[sampler_point_clamp], uv, 0.0).rrr);
    }
    else if (view == VIEW_DEPTH)
        color = to_linear(pow(tex_depth.Load(pixel).r, 0.25).xxx);	// Reverse-Z: near is white
    else if (view == VIEW_CASCADES || view == VIEW_CLOUD_SHADOW)
    {
        // the g-buffer's surface (nothing where the sky is): its cascade in red, green, blue, yellow (white past the far
        // limit), darkened where shadowed; or the share of the light the clouds let through
        const float depth = tex_depth.Load(pixel).r;
        if (depth > 0.0)
        {
            const float3 world = get_position(depth, (float2(thread_id.xy) + 0.5) / resolution);
            const float3 normal = xp_receiver_normal(int2(thread_id.xy), world, normalize(tex_normal.Load(pixel).xyz));
            if (view == VIEW_CLOUD_SHADOW)
            {
                color = xp_cloud_shadow(world).xxx;
            }
            else
            {
                const float3 tints[5] = { float3(1.0, 0.25, 0.25), float3(0.25, 1.0, 0.25), float3(0.3, 0.45, 1.0), float3(1.0, 0.9, 0.2), float3(1.0, 1.0, 1.0) };
                const uint cascade = min(xp_csm_select(xp_view_depth(world)), 4u);
                color = tints[cascade] * (0.2 + 0.8 * xp_csm_visibility(world, normal));
            }
        }
    }
    else if (view == VIEW_EV || view == VIEW_NITS)
    {
        const float3 radiance = tex.Load(pixel).rgb / max(get_pre_exposure(), 1e-20);
        const float nits = dot(radiance, float3(0.2126, 0.7152, 0.0722));
        if (view == VIEW_NITS)
        {
            color = nits_ramp(nits);
        }
        else
        {
            // EV100 of the pixel's luminance (K = 12.5): -2 .. 18 in bands
            const float ev = log2(max(nits, 1e-6) * 100.0 / 12.5);
            color = to_linear(false_color((ev + 2.0) / 20.0));
        }
    }

    tex_uav[thread_id.xy] = float4(color, 1.0);
}

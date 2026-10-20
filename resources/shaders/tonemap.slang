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
 * tonemap.slang
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 *
 * The output transform (plan TASK-025, TASK-041): the pre-exposed HDR frame -> residual exposure -> bloom ->
 * exposure fusion -> tone curve -> ASC-CDL grade -> output gain / gamma -> display-referred LINEAR values in 0-1
 * (the sRGB swapchain does the encoding; ImGui samples frame_output as is).
 *   tex       frame_render (radiance x pre-exposure)
 *   tex2      bloom, mip 0 (the summed mip chain, pre-exposed)
 *   tex3      exposure_fusion_guide (log2 gain, half resolution)
 *   tex4      exposure_fusion_exposures, mip 0 (a: the neutral exposed luminance the guide was made from)
 *   tex_depth g-buffer depth (the fusion fades with distance)
 *   exposure_state  EXPOSURE_STATE_EV: the EV100 this frame is shown at; the residual is its scale / get_pre_exposure()
 *   tex_uav   output
 *   pass_get_f4_value(): x curve (0 none, 1 AgX, 2 ACES fit, 3 Reinhard), y bloom weight (0: none), z 1 / bloom mip
 *   count, w exposure fusion strength (0: none)
 *
 * XP12: hdr (AgX since X-Plane 12.2). AgX after Troy Sobotka's AgX; the contrast curve is Benjamin Wrensch's 6th order
 * polynomial fit (MIT), as published in "Minimal AgX implementation" (iolite engine, 2023).
 */

import common;
import resources;
import constants;
import position;

static const uint CURVE_NONE = 0;
static const uint CURVE_AGX = 1;
static const uint CURVE_ACES = 2;
static const uint CURVE_REINHARD = 3;

float3 agx_contrast(float3 x)
{
    const float3 x2 = x * x;
    const float3 x4 = x2 * x2;
    return 15.5 * x4 * x2 - 40.14 * x4 * x + 31.96 * x4 - 6.868 * x2 * x + 0.4298 * x2 + 0.1191 * x - 0.00232;
}

// AgX base: inset into the AgX working space, log2 encode over [-12.47, 4.03] EV around middle grey, sigmoid
float3 agx(float3 color)
{
    // Columns of the inset matrix (the matrices are written as in the column-major reference: mul(v, M) applies them)
    const float3x3 inset = float3x3(
        0.842479062253094, 0.0423282422610123, 0.0423756549057051,
        0.0784335999999992, 0.878468636469772, 0.0784336,
        0.0792237451477643, 0.0791661274605434, 0.879142973793104);
    const float min_ev = -12.47393;
    const float max_ev = 4.026069;
    color = mul(max(color, 1e-10), inset);
    color = clamp(log2(color), min_ev, max_ev);
    color = (color - min_ev) / (max_ev - min_ev);
    return agx_contrast(color);
}

// AgX outset and display EOTF: the curve's output is display-encoded (power 2.2); return linear for the sRGB target
float3 agx_eotf(float3 color)
{
    const float3x3 outset = float3x3(
        1.19687900512017, -0.0528968517574562, -0.0529716355144438,
        -0.0980208811401368, 1.15190312990417, -0.0980434501171241,
        -0.0990297440797205, -0.0989611768448433, 1.15107367264116);
    color = mul(color, outset);
    return pow(saturate(color), 2.2);
}

// Krzysztof Narkowicz's ACES filmic fit (input pre-scaled by 0.6, as in his post)
float3 aces_fit(float3 x)
{
    x *= 0.6;
    return saturate((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14));
}

static const float3 LUMA = float3(0.2126f, 0.7152f, 0.0722f);

// The fusion guide at full resolution: the four nearest guide texels, bilinear weights times how close their neutral
// luminance is to this pixel's (log2, 1 EV wide), so gains don't bleed across edges (joint bilateral upsample)
float fusion_gain(float2 uv, float luminance)
{
    float2 guide_resolution;
    tex3.GetDimensions(guide_resolution.x, guide_resolution.y);
    const float2 position = uv * guide_resolution - 0.5f;
    const int2 base = int2(floor(position));
    const float2 f = position - float2(base);
    const float log_luminance = log2(max(luminance, 1e-9f));

    float sum = 0.0f;
    float weight = 0.0f;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        const int2 offset = int2(i & 1, i >> 1);
        const int2 texel = clamp(base + offset, int2(0, 0), int2(guide_resolution) - 1);
        const float bilinear = (offset.x != 0 ? f.x : 1.0f - f.x) * (offset.y != 0 ? f.y : 1.0f - f.y);
        const float difference = log_luminance - log2(max(tex4.Load(int3(texel, 0)).a, 1e-9f));
        const float w = bilinear * exp(-0.5f * difference * difference) + 1e-5f;
        sum += tex3.Load(int3(texel, 0)).r * w;
        weight += w;
    }
    return sum / weight;
}

// How much of the fusion a pixel gets: all of it nearer than exposure_fusion.z km, none past exposure_fusion.w km
// (ASSUMPTION: X-Plane's exposure_fusion/dis_near and dis_far), none on the sky
float fusion_distance_weight(uint2 pixel, float2 uv)
{
    const float depth = tex_depth[pixel].r;
    if (depth <= 0.0f)
        return 0.0f;
    const float km = length(get_position(depth, uv) - get_camera_position()) * 0.001f;
    const float near_km = buffer_environment.exposure_fusion.z;
    const float far_km = max(buffer_environment.exposure_fusion.w, near_km + 1e-3f);
    return 1.0f - saturate((km - near_km) / (far_km - near_km));
}

// ASC-CDL on display-encoded values (color_management/sdr/*): slope, offset, power, then saturation (Rec. 709 luma)
float3 grade(float3 display)
{
    const float4 slope = buffer_environment.color_slope;
    const float4 offset = buffer_environment.color_offset;
    const float4 power = buffer_environment.color_power;
    float3 c = pow(max(display * slope.rgb + offset.rgb, 0.0f), power.rgb);
    const float luma = dot(c, LUMA);
    c = luma + slope.w * (c - luma);

    // Output gain and gamma
    return pow(max(c * offset.w, 0.0f), 1.0f / max(power.w, 0.01f));
}

[shader("compute")]
[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void main_comp(uint3 thread_id: SV_DispatchThreadID)
{
    float2 resolution;
    tex_uav.GetDimensions(resolution.x, resolution.y);
    if (any(thread_id.xy >= uint2(resolution)))
        return;

    const float4 controls = pass_get_f4_value();
    const uint curve = uint(controls.x + 0.5);
    const float2 uv = (thread_id.xy + 0.5f) / resolution;

    // Pre-exposed radiance, energy-conserving bloom (the frame and the normalized bloom chain mixed), then the
    // residual: this frame's exposure over the pre-exposure its lighting was written with
    float2 render_resolution;
    tex.GetDimensions(render_resolution.x, render_resolution.y);
    float3 color = tex.SampleLevel(samplers[sampler_bilinear_clamp], uv, 0.0f).rgb;
    if (controls.y > 0.0f)
        color = lerp(color, tex2.SampleLevel(samplers[sampler_bilinear_clamp], uv, 0.0f).rgb * controls.z, controls.y);
    const float residual = exposure_scale_from_ev(exposure_state[EXPOSURE_STATE_EV]) / max(get_pre_exposure(), 1e-30f);
    color = max(select(isfinite(color), color, 0.0) * residual, 0.0f);

    // Local tone mapping
    if (controls.w > 0.0f)
    {
        const uint2 render_pixel = min(uint2(uv * render_resolution), uint2(render_resolution) - 1);
        const float strength = controls.w * fusion_distance_weight(render_pixel, uv);
        if (strength > 0.0f)
            color *= exp2(fusion_gain(uv, dot(color, LUMA)) * strength);
    }

    if (curve == CURVE_AGX)
        color = agx_eotf(agx(color));
    else if (curve == CURVE_ACES)
        color = aces_fit(color);
    else if (curve == CURVE_REINHARD)
        color = color / (1.0 + color);
    // CURVE_NONE: the frame as rendered (the sRGB target clips above 1)

    // The grade works on display-encoded values (power 2.2, as the curves' outputs are before their EOTF)
    color = pow(grade(pow(saturate(color), 1.0f / 2.2f)), 2.2f);

    tex_uav[thread_id.xy] = float4(saturate(color), 1.0);
}

/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * fsr1_rcas.slang
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 *
 * AMD FidelityFX Super Resolution 1.0, Robust Contrast Adaptive Sharpening (RCAS), ported to Slang from ffx_fsr1.h
 * (plan TASK-044): a 5-tap cross whose negative lobe is as strong as the neighbourhood allows without clipping,
 * scaled by the sharpness, with AMD's noise detection (FSR_RCAS_DENOISE) on.
 *   tex      fsr1_easu.slang's output (perceptual: square roots)
 *   tex_uav  output, display-referred linear (squared back)
 *   pass_get_f4_value(): x sharpness as FsrRcasCon's scale, exp2(-stops): 1 is the sharpest (X-Plane's _rcas_ratio)
 *
 * Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

import resources;
import constants;

static const float RCAS_LIMIT = 0.25f - 1.0f / 16.0f;

float3 load(int2 pixel, int2 size)
{
    return saturate(tex.Load(int3(clamp(pixel, int2(0, 0), size - 1), 0)).rgb);
}

float luma2(float3 c) { return c.b * 0.5f + (c.r * 0.5f + c.g); }

[shader("compute")]
[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void main_comp(uint3 thread_id: SV_DispatchThreadID)
{
    float2 resolution;
    tex_uav.GetDimensions(resolution.x, resolution.y);
    if (any(thread_id.xy >= uint2(resolution)))
        return;
    const int2 size = int2(resolution);
    const int2 p = int2(thread_id.xy);

    //    b
    //  d e f
    //    h
    const float3 b = load(p + int2(0, -1), size);
    const float3 d = load(p + int2(-1, 0), size);
    const float3 e = load(p, size);
    const float3 f = load(p + int2(1, 0), size);
    const float3 h = load(p + int2(0, 1), size);

    // Noise detection
    const float bl = luma2(b), dl = luma2(d), el = luma2(e), fl = luma2(f), hl = luma2(h);
    float nz = 0.25f * (bl + dl + fl + hl) - el;
    const float luma_range = max(max(max(bl, dl), max(el, fl)), hl) - min(min(min(bl, dl), min(el, fl)), hl);
    nz = saturate(abs(nz) / max(luma_range, 1e-6f));
    nz = -0.5f * nz + 1.0f;

    // The ring's min and max per channel, and the lobe that keeps the result inside 0-1
    const float3 mn4 = min(min(b, d), min(f, h));
    const float3 mx4 = max(max(b, d), max(f, h));
    const float2 peak = float2(1.0f, -4.0f);
    const float3 hit_min = min(mn4, e) / max(4.0f * mx4, 1e-6f);
    const float3 hit_max = (peak.x - max(mx4, e)) / min(4.0f * mn4 + peak.y, -1e-6f);
    const float3 lobe_rgb = max(-hit_min, hit_max);
    float lobe = max(-RCAS_LIMIT, min(max(max(lobe_rgb.r, lobe_rgb.g), lobe_rgb.b), 0.0f)) * pass_get_f4_value().x;
    lobe *= nz;

    const float3 color = (lobe * (b + d + f + h) + e) / (4.0f * lobe + 1.0f);
    tex_uav[thread_id.xy] = float4(color * color, 1.0f);
}

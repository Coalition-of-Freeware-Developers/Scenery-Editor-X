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
 * The output transform (plan TASK-025, extended in Phase 5): pre-exposed HDR frame -> exposure -> tone curve ->
 * display-referred LINEAR values in 0-1 for frame_output (the sRGB swapchain does the encoding; ImGui samples
 * frame_output as is).
 *   tex      frame_render (radiance x pre-exposure)
 *   tex_uav  frame_output
 *   pass_get_f4_value(): x curve (0 none, 1 AgX, 2 ACES fit, 3 Reinhard)
 *
 * XP12: hdr (AgX since X-Plane 12.2). AgX after Troy Sobotka's AgX; the contrast curve is Benjamin Wrensch's 6th order
 * polynomial fit (MIT), as published in "Minimal AgX implementation" (iolite engine, 2023).
 */

import common;
import resources;
import constants;

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

[shader("compute")]
[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void main_comp(uint3 thread_id: SV_DispatchThreadID)
{
    float2 resolution;
    tex_uav.GetDimensions(resolution.x, resolution.y);
    if (any(thread_id.xy >= uint2(resolution)))
        return;

    const uint curve = uint(pass_get_f4_value().x + 0.5);
    float3 color = tex.Load(int3(thread_id.xy, 0)).rgb * get_exposure_scale();
    color = select(isfinite(color), color, 0.0);

    if (curve == CURVE_AGX)
        color = agx_eotf(agx(color));
    else if (curve == CURVE_ACES)
        color = aces_fit(color);
    else if (curve == CURVE_REINHARD)
        color = color / (1.0 + color);
    // CURVE_NONE: the frame as rendered (the sRGB target clips above 1)

    tex_uav[thread_id.xy] = float4(color, 1.0);
}
